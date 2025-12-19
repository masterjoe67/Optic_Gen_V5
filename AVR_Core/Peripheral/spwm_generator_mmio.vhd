library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.AVRuCPackage.all; -- se serve per OFF_DB ecc.

entity spwm_generator_mmio is
    port (
        clk_sys   : in  std_logic;  -- bus/core clock
        clk_pwm   : in  std_logic;  -- PWM clock 50MHz
        rst_n     : in  std_logic;

        -- MMIO interface
        bus_addr  : in  std_logic_vector(5 downto 0);
        bus_wr    : in  std_logic;
        bus_rd    : in  std_logic;
        bus_wdata : in  std_logic_vector(7 downto 0);
        bus_rdata : out std_logic_vector(7 downto 0);
        out_en    : out std_logic;

        -- PWM outputs con deadtime
        AH, AL,
        BH, BL,
        CH, CL : out std_logic
    );
end entity;

architecture rtl of spwm_generator_mmio is

    ----------------------------------------------------------------
    -- MMIO offsets
    ----------------------------------------------------------------
--    constant CARRIER_0 : std_logic_vector(5 downto 0) := "000000";
--    constant CARRIER_1 : std_logic_vector(5 downto 0) := "000001";
--    constant CARRIER_2 : std_logic_vector(5 downto 0) := "000010";
--    constant CARRIER_3 : std_logic_vector(5 downto 0) := "000011";
--    constant MOD_0     : std_logic_vector(5 downto 0) := "000100";
--    constant MOD_1     : std_logic_vector(5 downto 0) := "000101";
--    constant DEADTIME  : std_logic_vector(5 downto 0) := "000110";
--    constant CTRL      : std_logic_vector(5 downto 0) := "000111";
--    constant MODE      : std_logic_vector(5 downto 0) := "001000";
--    constant STATUS    : std_logic_vector(5 downto 0) := "001001";

    ----------------------------------------------------------------
    -- MMIO registers (sul clock sys)
    ----------------------------------------------------------------
    signal reg_carrier  : unsigned(31 downto 0) := (others=>'0');
    signal reg_mod      : unsigned(15 downto 0) := (others=>'0');
    signal reg_deadtime : unsigned(7 downto 0)  := (others=>'0');
    signal reg_ctrl     : std_logic := '0';
    signal reg_mode     : unsigned(1 downto 0) := "00";

    ----------------------------------------------------------------
    -- SPWM LUT
    ----------------------------------------------------------------
    constant N       : integer := 2048;
    constant OFF120  : integer := N/3;
    constant OFF240  : integer := 2*N/3;

    signal addrA, addrB, addrC : integer range 0 to N-1 := 0;
    signal sinA_reg, sinB_reg, sinC_reg : unsigned(9 downto 0);
    signal sinA_out, sinB_out, sinC_out : std_logic_vector(9 downto 0);
    signal sinA_scaled, sinB_scaled, sinC_scaled : unsigned(9 downto 0);

    ----------------------------------------------------------------
    -- PWM carrier
    ----------------------------------------------------------------
    signal carrier   : unsigned(9 downto 0) := (others=>'0');
    signal pwmA, pwmB, pwmC     : std_logic;
    signal pwmA_i, pwmB_i, pwmC_i : std_logic;

    ----------------------------------------------------------------
    -- Deadtime
    ----------------------------------------------------------------
    signal dtA, dtB, dtC : unsigned(7 downto 0);
    signal stA, stB, stC : std_logic;
	 signal init_done : std_logic := '0';

    ----------------------------------------------------------------
    -- sincronizzazione MMIO su clk_pwm
    ----------------------------------------------------------------
    signal car_s2, mod_s2 : unsigned(31 downto 0);
    signal dt_s2          : unsigned(7 downto 0);
    signal en_s2          : std_logic;
    signal mode_s2        : unsigned(1 downto 0);
	 


begin

    ----------------------------------------------------------------
    -- MMIO WRITE
    ----------------------------------------------------------------
    process(clk_sys)
    begin
        if rising_edge(clk_sys) then
            if rst_n='0' then
                reg_carrier  <= (others=>'0');
                reg_mod      <= (others=>'0');
                reg_deadtime <= (others=>'0');
                reg_ctrl     <= '0';
                reg_mode     <= "00";
            else
				if bus_wr='1' then
					case bus_addr is
						when CTRL => reg_ctrl <= bus_wdata(0);

						when others =>
							if reg_ctrl='0' then
								case bus_addr is
								  when CARRIER_0 => reg_carrier(7 downto 0)   <= unsigned(bus_wdata);
								  when CARRIER_1 => reg_carrier(15 downto 8)  <= unsigned(bus_wdata);
								  when CARRIER_2 => reg_carrier(23 downto 16) <= unsigned(bus_wdata);
								  when CARRIER_3 => reg_carrier(31 downto 24) <= unsigned(bus_wdata);
								  when MOD_0     => reg_mod(7 downto 0)  <= unsigned(bus_wdata);
								  when MOD_1     => reg_mod(15 downto 8) <= unsigned(bus_wdata);
								  when DEADTIME  => reg_deadtime <= unsigned(bus_wdata);
								  when MODE      => reg_mode <= unsigned(bus_wdata(1 downto 0));
								  when others    => null;
								end case;
							end if;
					end case;
				end if;

--                if bus_wr='1' and reg_ctrl='0' then
--                    case bus_addr is
--                        when CARRIER_0 => reg_carrier(7 downto 0)   <= unsigned(bus_wdata);
--                        when CARRIER_1 => reg_carrier(15 downto 8)  <= unsigned(bus_wdata);
--                        when CARRIER_2 => reg_carrier(23 downto 16) <= unsigned(bus_wdata);
--                        when CARRIER_3 => reg_carrier(31 downto 24) <= unsigned(bus_wdata);
--                        when MOD_0     => reg_mod(7 downto 0)  <= unsigned(bus_wdata);
--                        when MOD_1     => reg_mod(15 downto 8) <= unsigned(bus_wdata);
--                        when DEADTIME  => reg_deadtime <= unsigned(bus_wdata);
--                        when CTRL      => reg_ctrl <= bus_wdata(0);
--                        when MODE      => reg_mode <= unsigned(bus_wdata(1 downto 0));
--                        when others    => null;
--                    end case;
--                end if;
					 
            end if;
        end if;
    end process;
	 
	 -- ============================================================
    -- MMIO enable (AVR style)
    -- ============================================================
    out_en <= '1' when bus_rd='1' and
        (bus_addr=CARRIER_0 or bus_addr=CARRIER_1 or
         bus_addr=CARRIER_2 or bus_addr=CARRIER_3 or
			bus_addr=MOD_0 or bus_addr=MOD_1 or
			bus_addr=DEADTIME or
			bus_addr=CTRL or
			bus_addr=MODE )
        else '0';

    ----------------------------------------------------------------
    -- MMIO READ
    ----------------------------------------------------------------
    process(all)
    begin
        bus_rdata <= (others=>'0');
        --out_en    <= '0';
        if bus_rd='1' then
            --out_en <= '1';
            case bus_addr is
                when CARRIER_0 => bus_rdata <= std_logic_vector(reg_carrier(7 downto 0));
                when CARRIER_1 => bus_rdata <= std_logic_vector(reg_carrier(15 downto 8));
                when CARRIER_2 => bus_rdata <= std_logic_vector(reg_carrier(23 downto 16));
                when CARRIER_3 => bus_rdata <= std_logic_vector(reg_carrier(31 downto 24));
                when MOD_0     => bus_rdata <= std_logic_vector(reg_mod(7 downto 0));
                when MOD_1     => bus_rdata <= std_logic_vector(reg_mod(15 downto 8));
                when DEADTIME  => bus_rdata <= std_logic_vector(reg_deadtime);
                when CTRL      => bus_rdata(0) <= reg_ctrl;
                when MODE      => bus_rdata(1 downto 0) <= std_logic_vector(reg_mode);
                when STATUS    => bus_rdata(0) <= en_s2;
                when others    => null;
            end case;
        end if;
    end process;
	 
	 
	 
	 
	 
	 
    ----------------------------------------------------------------
    -- sincronizzazione MMIO su clk_pwm
    ----------------------------------------------------------------
    process(clk_pwm)
    begin
        if rising_edge(clk_pwm) then
            car_s2  <= reg_carrier;
            mod_s2  <= resize(reg_mod,32);
            dt_s2   <= reg_deadtime;
            en_s2   <= reg_ctrl;
            mode_s2 <= reg_mode;
        end if;
    end process;

    ----------------------------------------------------------------
    -- contatore address LUT
    ----------------------------------------------------------------
    process(clk_pwm)
    begin
        if rising_edge(clk_pwm) then
            if rst_n='0' then
                addrA <= 0;
            elsif en_s2='1' then
                addrA <= (addrA + 1) mod N;
            end if;
        end if;
    end process;

    addrB <= (addrA + OFF120) mod N;
    addrC <= (addrA + OFF240) mod N;

    ----------------------------------------------------------------
    -- instanziazione ROM esterna SIN_ROM
    ----------------------------------------------------------------
--    U_ROM_A : entity work.SIN_ROM port map(address => std_logic_vector(to_unsigned(addrA,11)), q => sinA_out);
--    U_ROM_B : entity work.SIN_ROM port map(address => std_logic_vector(to_unsigned(addrB,11)), q => sinB_out);
--    U_ROM_C : entity work.SIN_ROM port map(address => std_logic_vector(to_unsigned(addrC,11)), q => sinC_out);
	 U_ROM_A : entity work.SIN_ROM
    port map(
        clock     => clk_pwm,
        address => std_logic_vector(to_unsigned(addrA,11)),
        q       => sinA_out
    );
	 U_ROM_B : entity work.SIN_ROM
    port map(
        clock     => clk_pwm,
        address => std_logic_vector(to_unsigned(addrB,11)),
        q       => sinB_out
    );
	 U_ROM_C : entity work.SIN_ROM
    port map(
        clock     => clk_pwm,
        address => std_logic_vector(to_unsigned(addrC,11)),
        q       => sinC_out
    );

    ----------------------------------------------------------------
    -- registrazione valori LUT
    ----------------------------------------------------------------
    process(clk_pwm)
    begin
        if rising_edge(clk_pwm) then
            sinA_reg <= unsigned(sinA_out);
            sinB_reg <= unsigned(sinB_out);
            sinC_reg <= unsigned(sinC_out);
        end if;
    end process;


    -- apply modulation
    ----------------------------------------------------------------
    sinA_scaled <= resize((sinA_reg * mod_s2(9 downto 0)) / 1023, 10);
    sinB_scaled <= resize((sinB_reg * mod_s2(9 downto 0)) / 1023, 10);
    sinC_scaled <= resize((sinC_reg * mod_s2(9 downto 0)) / 1023, 10);
	 
	 

    ----------------------------------------------------------------
    -- PWM carrier
    ----------------------------------------------------------------
    process(clk_pwm)
    begin
        if rising_edge(clk_pwm) then
            if rst_n='0' then
                carrier <= (others=>'0');
            elsif en_s2='1' then
                if carrier >= unsigned(car_s2(9 downto 0)) then
                    carrier <= (others=>'0');
                else
                    carrier <= carrier + 1;
                end if;
            end if;
        end if;
    end process;

    pwmA <= '1' when sinA_scaled > carrier else '0';
    pwmB <= '1' when sinB_scaled > carrier else '0';
    pwmC <= '1' when sinC_scaled > carrier else '0';

    ----------------------------------------------------------------
    -- selezione modalità
    ----------------------------------------------------------------
    process(all)
    begin
        case mode_s2 is
            when "00" =>  -- trifase
                pwmA_i <= pwmA;
                pwmB_i <= pwmB;
                pwmC_i <= pwmC;
            when "01" =>  -- mono mezzo ponte
                pwmA_i <= pwmA;
                pwmB_i <= '0';
                pwmC_i <= '0';
            when "10" =>  -- mono ponte intero
                pwmA_i <= pwmA;
                pwmB_i <= not pwmA;
                pwmC_i <= '0';
            when others =>
                pwmA_i <= '0';
                pwmB_i <= '0';
                pwmC_i <= '0';
        end case;
    end process;

    ----------------------------------------------------------------
    -- deadtime driver
    ----------------------------------------------------------------
--    process(clk_pwm)
--    begin
--        if rising_edge(clk_pwm) then
--            if rst_n='0' then
--                dtA <= (others=>'0'); stA <= '0';
--                dtB <= (others=>'0'); stB <= '0';
--                dtC <= (others=>'0'); stC <= '0';
--                AH <= '0'; AL <= '0';
--                BH <= '0'; BL <= '0';
--                CH <= '0'; CL <= '0';
--            else
--                -- Phase A
--                if pwmA_i /= stA then
--                    stA <= pwmA_i;
--                    if dt_s2 = 0 then
--                        AH <= pwmA_i;
--                        AL <= not pwmA_i;
--                    else
--                        dtA <= dt_s2;
--                        AH <= '0'; AL <= '0';
--                    end if;
--                elsif dtA /= 0 then
--                    dtA <= dtA - 1;
--                else
--                    AH <= stA;
--                    AL <= not stA;
--                end if;
--                -- Phase B
--                if pwmB_i /= stB then
--                    stB <= pwmB_i;
--                    if dt_s2 = 0 then
--                        BH <= pwmB_i;
--                        BL <= not pwmB_i;
--                    else
--                        dtB <= dt_s2;
--                        BH <= '0'; BL <= '0';
--                    end if;
--                elsif dtB /= 0 then
--                    dtB <= dtB - 1;
--                else
--                    BH <= stB;
--                    BL <= not stB;
--                end if;
--                -- Phase C
--                if pwmC_i /= stC then
--                    stC <= pwmC_i;
--                    if dt_s2 = 0 then
--                        CH <= pwmC_i;
--                        CL <= not pwmC_i;
--                    else
--                        dtC <= dt_s2;
--                        CH <= '0'; CL <= '0';
--                    end if;
--                elsif dtC /= 0 then
--                    dtC <= dtC - 1;
--                else
--                    CH <= stC;
--                    CL <= not stC;
--                end if;
--            end if;
--        end if;
--    end process;

 ----------------------------------------------------------------
    -- deadtime driver con blocco uscite CTRL=0
    ----------------------------------------------------------------
    process(clk_pwm)
    begin
        if rising_edge(clk_pwm) then
            if rst_n='0' or en_s2='0' then
                dtA <= (others=>'0'); stA <= '0';
                dtB <= (others=>'0'); stB <= '0';
                dtC <= (others=>'0'); stC <= '0';
                AH <= '0'; AL <= '0';
                BH <= '0'; BL <= '0';
                CH <= '0'; CL <= '0';
                init_done <= '0';
            else
                if init_done = '0' then
                    stA <= pwmA_i;
                    stB <= pwmB_i;
                    stC <= pwmC_i;
                    AH <= pwmA_i; AL <= not pwmA_i;
                    BH <= pwmB_i; BL <= not pwmB_i;
                    CH <= pwmC_i; CL <= not pwmC_i;
                    init_done <= '1';
                else
                    -- Phase A
                    if pwmA_i /= stA then
                        stA <= pwmA_i;
                        dtA <= dt_s2;
                        AH <= '0'; AL <= '0';
                    elsif dtA /= 0 then
                        dtA <= dtA - 1;
                    else
                        AH <= stA;
                        AL <= not stA;
                    end if;
                    -- Phase B
                    if pwmB_i /= stB then
                        stB <= pwmB_i;
                        dtB <= dt_s2;
                        BH <= '0'; BL <= '0';
                    elsif dtB /= 0 then
                        dtB <= dtB - 1;
                    else
                        BH <= stB;
                        BL <= not stB;
                    end if;
                    -- Phase C
                    if pwmC_i /= stC then
                        stC <= pwmC_i;
                        dtC <= dt_s2;
                        CH <= '0'; CL <= '0';
                    elsif dtC /= 0 then
                        dtC <= dtC - 1;
                    else
                        CH <= stC;
                        CL <= not stC;
                    end if;
                end if;
            end if;
        end if;
    end process;

end architecture;
