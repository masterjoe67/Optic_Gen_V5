
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
        bus_addr  : in  std_logic_vector(6 downto 0);
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
    -- MMIO registers (sul clock sys)
    ----------------------------------------------------------------

	 signal sel_wr  : unsigned(1 downto 0) := (others=>'0');
    signal idx_wr  : unsigned(1 downto 0) := (others=>'0');

    signal tmp32   : unsigned(31 downto 0) := (others=>'0');
    signal tmp16   : unsigned(15 downto 0) := (others=>'0');

    signal reg_carrier 		 : std_logic_vector(31 downto 0) := (others=>'0');
    signal reg_mod     		 : std_logic_vector(31 downto 0) := (others=>'0');
    signal reg_dead    		 : std_logic_vector(15 downto 0) := (others=>'0');
	 signal reg_magnitude    : std_logic_vector(9 downto 0) := (others=>'0');

    signal sel_rd  : unsigned(1 downto 0) := (others=>'0');
    signal idx_rd  : unsigned(1 downto 0) := (others=>'0');

    signal rdata_i : std_logic_vector(7 downto 0) := (others=>'0');
	 
	 
    signal reg_ctrl     : std_logic := '0';
    signal reg_mode     : unsigned(1 downto 0) := "00";  -- MODE 0 = 1phase half bridge, 1 = 1phase full bridge, 2 3phase

    signal module_sel   : std_logic;
	 
	 signal buf_car  : unsigned(31 downto 0);
	 signal buf_mod  : unsigned(31 downto 0);
	 signal buf_dead : unsigned(15 downto 0);
	 
	 signal buf32  	: unsigned(31 downto 0);
	 signal rd_buf32  : unsigned(31 downto 0);

    signal en_s2          : std_logic;
	 
	 signal H   : std_logic;
	 signal L   : std_logic;
	 signal H1   : std_logic;
	 signal L1   : std_logic;
	 signal H2   : std_logic;
	 signal L2   : std_logic;
	 
	 signal tri_H_A   : std_logic;
	 signal tri_L_A   : std_logic;
	 signal tri_H_B   : std_logic;
	 signal tri_L_B   : std_logic;
	 signal tri_H_C   : std_logic;
	 signal tri_L_C   : std_logic;
	 
	 signal pwm_x   : std_logic;
	 signal pwm_y   : std_logic;

component top_spwm_dynamic port(
    clk				: in  std_logic;              -- 200 MHz
    rst_n			: in  std_logic;
	 
	 PWM_FREQ_REG  : in  std_logic_vector(31 downto 0);   -- Hz
	 FREQ_CTR_REG  : in  std_logic_vector(31 downto 0);   -- DDS divisore sinusoide
	 deadtime_reg  : in  std_logic_vector(15 downto 0);  -- tick di pwm_clk
	 MAGNITUDE_REG	: in  std_logic_vector(9 downto 0);

-- ===== uscite full bridge =====
    AH : out std_logic;
    AL : out std_logic;
    BH : out std_logic;
    BL : out std_logic;
    CH : out std_logic;
    CL : out std_logic
);
end component;

component top_spwm_single port( 
    clk				: in  std_logic;              -- 200 MHz
    rst_n			: in  std_logic;
	 
	 PWM_FREQ_REG  : in  std_logic_vector(31 downto 0);   -- Hz
	 FREQ_CTR_REG  : in  std_logic_vector(31 downto 0);   -- DDS divisore sinusoide
	 deadtime_reg  : in  std_logic_vector(15 downto 0);  	-- tick di pwm_clk
	 MAGNITUDE_REG	: in  std_logic_vector(9 downto 0);
    mode 			: in  std_logic;         					-- 0 = Mezzo ponte, 1 = Ponte intero

	 -- ===== uscite  =====
    H : out std_logic;
    L : out std_logic;
    H1 : out std_logic;
    L1 : out std_logic;
    H2 : out std_logic;
    L2 : out std_logic

);
end component;

component top_spwm_selector port (
    clk				: in  std_logic;              -- 200 MHz
    rst_n			: in  std_logic;
    mode_sel 		: in  std_logic_vector(1 downto 0);
    out_en 			: in  std_logic;
    -- MONOFASE
    mono_H : in  std_logic;
    mono_L : in  std_logic;
    mono_H1 : in  std_logic;
    mono_L1 : in  std_logic;
    mono_H2 : in  std_logic;
    mono_L2 : in  std_logic;

    -- TRIFASE
    tri_H_A : in  std_logic;
    tri_L_A : in  std_logic;
    tri_H_B : in  std_logic;
    tri_L_B : in  std_logic;
    tri_H_C : in  std_logic;
    tri_L_C : in  std_logic;

    -- USCITE
    out_H0 : out std_logic;
    out_L0 : out std_logic;
    out_H1 : out std_logic;
    out_L1 : out std_logic;
    out_H2 : out std_logic;
    out_L2 : out std_logic
);
end component;


begin
    ----------------------------------------------------------------
    -- MMIO WRITE
    ----------------------------------------------------------------
process(clk_sys)
begin
    if rising_edge(clk_sys) then
        
        if rst_n='0' then
            reg_carrier   <= (others=>'0');
            reg_mod       <= (others=>'0');
            reg_dead      <= (others=>'0');
            reg_magnitude <= "1111111111";   -- 1023 in binario
            buf32         <= (others=>'0');
            reg_ctrl      <= '0';
            reg_mode      <= "00";

        else
            if module_sel='1' then

                case bus_addr is

                    when CTRL =>
                        reg_ctrl <= bus_wdata(0);

                    when BUF_0 =>
                        buf32(7 downto 0) <= unsigned(bus_wdata);

                    when BUF_1 =>
                        buf32(15 downto 8) <= unsigned(bus_wdata);

                    when BUF_2 =>
                        buf32(23 downto 16) <= unsigned(bus_wdata);

                    when BUF_3 =>
                        buf32(31 downto 24) <= unsigned(bus_wdata);

                    when COMMIT =>
                        case bus_wdata(1 downto 0) is
                            when "00" =>
                                if reg_ctrl='0' then
                                    reg_carrier <= std_logic_vector(buf32);
                                end if;

                            when "01" =>
                                if reg_ctrl='0' then
                                    reg_mod <= std_logic_vector(buf32);
                                end if;

                            when "10" =>
                                if reg_ctrl='0' then
                                    reg_dead <= std_logic_vector(buf32(15 downto 0));
                                end if;

                            when "11" =>
                                reg_magnitude <= std_logic_vector(buf32(9 downto 0));

                            when others =>
                                null;
                        end case;

                        buf32 <= (others=>'0');

                    when MODE =>
                        if reg_ctrl='0' then
                            reg_mode <= unsigned(bus_wdata(1 downto 0));
                        end if;

                    when RSEL =>
                        case bus_wdata(1 downto 0) is
                            when "00" =>
                                rd_buf32 <= unsigned(reg_carrier);

                            when "01" =>
                                rd_buf32 <= unsigned(reg_mod);

                            when "10" =>
                                rd_buf32(15 downto 0)  <= unsigned(reg_dead);
                                rd_buf32(31 downto 16) <= (others => '0');
										  
									 when "11" =>
                                rd_buf32(9 downto 0)  <= unsigned(reg_magnitude);
                                rd_buf32(31 downto 10) <= (others => '0');

                            when others =>
                                rd_buf32 <= (others => '0');
                        end case;

                    when others =>
                        null;

                end case;
            end if;
        end if;
    end if;
end process;

	 
	 -- ============================================================
    -- MMIO enable (AVR style)
    -- ============================================================
    out_en <= '1' when bus_rd='1' and
        (bus_addr=BUF_0 or bus_addr=BUF_1 or
         bus_addr=BUF_2 or bus_addr=BUF_3 or
			bus_addr=COMMIT or bus_addr=RSEL or
			bus_addr=CTRL  or bus_addr=MODE )
        else '0';
	module_sel <= '1' when bus_wr='1' and
        (bus_addr=BUF_0 or bus_addr=BUF_1 or
         bus_addr=BUF_2 or bus_addr=BUF_3 or
			bus_addr=COMMIT or bus_addr=RSEL or
			bus_addr=CTRL or bus_addr=MODE )
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
					when BUF_0 => bus_rdata <= std_logic_vector(rd_buf32(7 downto 0));
					when BUF_1 => bus_rdata <= std_logic_vector(rd_buf32(15 downto 8));
					when BUF_2 => bus_rdata <= std_logic_vector(rd_buf32(23 downto 16));
					when BUF_3 => bus_rdata <= std_logic_vector(rd_buf32(31 downto 24));
						
                when CTRL      => bus_rdata(0) <= reg_ctrl;
                when MODE      => bus_rdata(1 downto 0) <= std_logic_vector(reg_mode);

                when others    => null;
            end case;
        end if;
    end process;
	 
	 
	



SVPWM_inst : component top_spwm_dynamic port map(
    clk				=> clk_pwm,              -- 200 MHz
    rst_n 			=> rst_n,
	 
	 pwm_freq_reg  => reg_carrier,   -- Hz
	 freq_ctr_reg  => reg_mod,   -- DDS divisore sinusoide
	 deadtime_reg  => reg_dead,  -- tick di pwm_clk
	 MAGNITUDE_REG => reg_magnitude,
   -- input  wire [4:0]  deadtime,

-- ===== uscite full bridge =====
    AH => tri_H_A,
    AL => tri_L_A,
    BH => tri_H_B,
    BL => tri_L_B,
    CH => tri_H_C,
    CL => tri_L_C
);



SVPWM_mono_inst  : component top_spwm_single port map( 
    clk				=> clk_pwm,              -- 200 MHz
    rst_n 			=> rst_n,
	 
	 pwm_freq_reg  => reg_carrier,   -- Hz
	 freq_ctr_reg  => reg_mod,   -- DDS divisore sinusoide
	 deadtime_reg  => reg_dead,  -- tick di pwm_clk
	 MAGNITUDE_REG => reg_magnitude,
    mode 			=> reg_mode(0),         					-- 0 = Mezzo ponte, 1 = Ponte intero

	 -- ===== uscite  =====
    H => H,
    L => L,
    H1 => H1,
    L1 => L1,
    H2 => H2,
    L2 => L2

);

PWM_mux_inst : component top_spwm_selector port map(
    clk				=> clk_pwm,              -- 200 MHz
    rst_n 			=> rst_n,
	 
    mode_sel      => std_logic_vector(reg_mode),
    out_en 			=> reg_ctrl,
    -- MONOFASE
    mono_H 	=> H,
    mono_L 	=> L,
    mono_H1 => H1,
    mono_L1 => L1,
    mono_H2 => H2,
    mono_L2 => L2,

    -- TRIFASE
    tri_H_A => tri_H_A,
    tri_L_A => tri_L_A,
    tri_H_B => tri_H_B,
    tri_L_B => tri_L_B,
    tri_H_C => tri_H_C,
    tri_L_C => tri_L_C,

    -- USCITE
    out_H0 => AH,
    out_L0 => AL,
    out_H1 => BH,
    out_L1 => BL,
    out_H2 => CH,
    out_L2 => CL
);

 

end architecture;
