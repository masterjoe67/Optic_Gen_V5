library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.AVRuCPackage.all; -- se serve per OFF_DB ecc.

entity btn_debounce_mmio is
    generic (
        N              : integer := 7;
        CLK_FREQ_HZ    : integer := 16000000;
        SAMPLE_MS      : integer := 2;
        THRESH         : integer := 10
    );
    port (
        clk        : in  std_logic;
        rstn       : in  std_logic;

        raw_in     : in  std_logic_vector(N-1 downto 0);

        bus_addr   : in  std_logic_vector(5 downto 0);
        bus_read   : in  std_logic;
        bus_write  : in  std_logic;
        bus_wdata  : in  std_logic_vector(7 downto 0);
        bus_rdata  : out std_logic_vector(7 downto 0);
        out_en     : out std_logic;

        irq        : out std_logic
    );
end entity;

architecture rtl of btn_debounce_mmio is

    -- ============================================================
    -- Helper functions
    -- ============================================================
    function clog2(x : integer) return integer is
        variable r : integer := 0;
        variable v : integer := x-1;
    begin
        while v > 0 loop
            v := v / 2;
            r := r + 1;
        end loop;
        return r;
    end function;

    function any_high(v : std_logic_vector) return std_logic is
    begin
        for i in v'range loop
            if v(i) = '1' then return '1'; end if;
        end loop;
        return '0';
    end function;

    -- ============================================================
    -- Parameters
    -- ============================================================
    constant CLK_PER_SAMPLE : integer := (CLK_FREQ_HZ/1000)*SAMPLE_MS;
    constant SAMPLE_CNTW    : integer := clog2(CLK_PER_SAMPLE);
    constant CNTW           : integer := clog2(THRESH+1);

    -- ============================================================
    -- Signals
    -- ============================================================
    signal sample_cnt   : unsigned(SAMPLE_CNTW-1 downto 0);
    signal sample_tick  : std_logic;

    signal sync0, sync1 : std_logic_vector(N-1 downto 0);
    signal sample_val   : std_logic_vector(N-1 downto 0);

    type cnt_array is array (0 to N-1) of unsigned(CNTW-1 downto 0);
    signal cnt : cnt_array;

    signal db          : std_logic_vector(N-1 downto 0);
    signal press_pulse : std_logic_vector(N-1 downto 0);

    signal evt_latch   : std_logic_vector(N-1 downto 0);
    signal clear_req   : std_logic_vector(N-1 downto 0);

    signal mask_reg    : std_logic_vector(N-1 downto 0);

    signal bus_rdata_i : std_logic_vector(7 downto 0);
	 
	 signal mask_reg_Sel  : std_logic;

begin
    bus_rdata <= bus_rdata_i;

    -- ============================================================
    -- MMIO enable (AVR style)
    -- ============================================================
    out_en <= '1' when bus_read='1' and
        (bus_addr=OFF_DB or bus_addr=OFF_EVT or
         bus_addr=OFF_MASK or bus_addr=OFF_CLEAR)
        else '0';
		  
	 mask_reg_Sel <= '1' when bus_addr = OFF_MASK else '0';

    -- ============================================================
    -- Sample tick
    -- ============================================================
    process(clk, rstn)
    begin
        if rstn='0' then
            sample_cnt  <= (others=>'0');
            sample_tick <= '0';
        elsif rising_edge(clk) then
            if sample_cnt = CLK_PER_SAMPLE-1 then
                sample_cnt  <= (others=>'0');
                sample_tick <= '1';
            else
                sample_cnt  <= sample_cnt + 1;
                sample_tick <= '0';
            end if;
        end if;
    end process;

    -- ============================================================
    -- Synchronizer
    -- ============================================================
    process(clk, rstn)
    begin
        if rstn='0' then
            sync0 <= (others=>'0');
            sync1 <= (others=>'0');
        elsif rising_edge(clk) then
            sync0 <= raw_in;
            sync1 <= sync0;
        end if;
    end process;

    -- ============================================================
    -- Sample register
    -- ============================================================
    process(clk, rstn)
    begin
        if rstn='0' then
            sample_val <= (others=>'0');
        elsif rising_edge(clk) then
            if sample_tick='1' then
                sample_val <= sync1;
            end if;
        end if;
    end process;

    -- ============================================================
    -- Debounce core
    -- ============================================================
    process(clk, rstn)
    begin
        if rstn='0' then
            db <= (others=>'0');
            press_pulse <= (others=>'0');
            for i in 0 to N-1 loop
                cnt(i) <= (others=>'0');
            end loop;

        elsif rising_edge(clk) then
            press_pulse <= (others=>'0');

            if sample_tick='1' then
                for i in 0 to N-1 loop
                    if sample_val(i)='1' then
                        if cnt(i)<THRESH then cnt(i)<=cnt(i)+1; end if;
                    else
                        if cnt(i)>0 then cnt(i)<=cnt(i)-1; end if;
                    end if;

                    if cnt(i)=THRESH and db(i)='0' then
                        db(i)<='1';
                        press_pulse(i)<='1';
                    elsif cnt(i)=0 and db(i)='1' then
                        db(i)<='0';
                    end if;
                end loop;
            end if;
        end if;
    end process;

    -- ============================================================
    -- Event latch (EIFR-like)
    -- ============================================================
    process(clk, rstn)
        variable v : std_logic_vector(N-1 downto 0);
    begin
        if rstn='0' then
            evt_latch <= (others=>'0');
        elsif rising_edge(clk) then
            v := evt_latch or (press_pulse and mask_reg);
            v := v and not clear_req;
            evt_latch <= v;
        end if;
    end process;

    irq <= any_high(evt_latch and mask_reg);

    -- ============================================================
    -- MASK register (DDR/PCMSK style)
    -- ============================================================
    process(clk, rstn)
    begin
        if rstn='0' then
            mask_reg <= "0000011"; --(others=>'1');
        elsif rising_edge(clk) then
            if bus_write='1' and mask_reg_Sel = '1' then
                mask_reg <= bus_wdata(6 downto 0);
            end if;
        end if;
    end process;

    -- ============================================================
    -- CLEAR pulse register
    -- ============================================================
    process(clk, rstn)
    begin
        if rstn='0' then
            clear_req <= (others=>'0');
        elsif rising_edge(clk) then
            clear_req <= (others=>'0');

            if bus_write='1' and bus_addr=OFF_CLEAR then
                clear_req <= bus_wdata(6 downto 0) and mask_reg;
            elsif bus_read='1' and bus_addr=OFF_EVT then
                clear_req <= evt_latch and mask_reg;
            end if;
        end if;
    end process;

    -- ============================================================
    -- MMIO READ (pure combinational, AVR-style)
    -- ============================================================
    process(all)
    begin
        bus_rdata_i <= (others=>'0');

        if bus_read='1' then
            case bus_addr is
                when OFF_DB   => bus_rdata_i(6 downto 0) <= db and mask_reg;
                when OFF_EVT  => bus_rdata_i(6 downto 0) <= evt_latch and mask_reg;
                when OFF_MASK => bus_rdata_i(6 downto 0) <= mask_reg;
                when others   => bus_rdata_i <= x"EF";
            end case;
        end if;
    end process;

end architecture;

