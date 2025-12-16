library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity btn_debounce_mmio is
    generic (
        N              : integer := 7;
        CLK_FREQ_HZ    : integer := 50000000;
        SAMPLE_MS      : integer := 2;
        THRESH         : integer := 10;

        OFF_DB         : std_logic_vector(7 downto 0) := x"00";
        OFF_EVT        : std_logic_vector(7 downto 0) := x"04";
        OFF_CNT        : std_logic_vector(7 downto 0) := x"08";
        OFF_CLEAR      : std_logic_vector(7 downto 0) := x"0C"
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
        bus_ready  : out std_logic;

        irq        : out std_logic
    );
end entity;

architecture rtl of btn_debounce_mmio is

    --------------------------------------------------------------------
    -- Derived parameters
    --------------------------------------------------------------------
    constant CLK_PER_SAMPLE : integer := (CLK_FREQ_HZ/1000) * SAMPLE_MS;
    constant SAMPLE_CNTW    : integer := integer(ceil(log2(real(CLK_PER_SAMPLE))));
    constant CNTW           : integer := integer(ceil(log2(real(THRESH+1))));

    --------------------------------------------------------------------
    -- Internal signals
    --------------------------------------------------------------------
    signal sample_cnt     : unsigned(SAMPLE_CNTW-1 downto 0) := (others => '0');
    signal sample_tick    : std_logic := '0';

    signal sync_0         : std_logic_vector(N-1 downto 0) := (others => '0');
    signal sync_1         : std_logic_vector(N-1 downto 0) := (others => '0');
    signal sample_val     : std_logic_vector(N-1 downto 0) := (others => '0');

    type counter_array is array (0 to N-1) of unsigned(CNTW-1 downto 0);
    signal cnt           : counter_array := (others => (others => '0'));

    signal db            : std_logic_vector(N-1 downto 0) := (others => '0');
    signal press_pulse   : std_logic_vector(N-1 downto 0) := (others => '0');
    signal evt_latch     : std_logic_vector(N-1 downto 0) := (others => '0');

    signal bus_rdata_i   : std_logic_vector(7 downto 0) := (others => '0');
    signal bus_ready_i   : std_logic := '0';

begin
    bus_rdata <= bus_rdata_i;
    bus_ready <= bus_ready_i;

    --------------------------------------------------------------------
    -- Sample tick generator
    --------------------------------------------------------------------
    process(clk, rstn)
    begin
        if rstn = '0' then
            sample_cnt  <= (others => '0');
            sample_tick <= '0';
        elsif rising_edge(clk) then
            if sample_cnt = CLK_PER_SAMPLE-1 then
                sample_cnt  <= (others => '0');
                sample_tick <= '1';
            else
                sample_cnt  <= sample_cnt + 1;
                sample_tick <= '0';
            end if;
        end if;
    end process;

    --------------------------------------------------------------------
    -- Double synchronizer
    --------------------------------------------------------------------
    process(clk, rstn)
    begin
        if rstn = '0' then
            sync_0 <= (others => '0');
            sync_1 <= (others => '0');
        elsif rising_edge(clk) then
            sync_0 <= raw_in;
            sync_1 <= sync_0;
        end if;
    end process;

    --------------------------------------------------------------------
    -- Sampled value
    --------------------------------------------------------------------
    process(clk, rstn)
    begin
        if rstn = '0' then
            sample_val <= (others => '0');
        elsif rising_edge(clk) then
            if sample_tick = '1' then
                sample_val <= sync_1;
            end if;
        end if;
    end process;

    --------------------------------------------------------------------
    -- Debounce logic, counters, pulses
    --------------------------------------------------------------------
    process(clk, rstn)
    begin
        if rstn = '0' then
            db          <= (others => '0');
            press_pulse <= (others => '0');

            for i in 0 to N-1 loop
                cnt(i) <= (others => '0');
            end loop;

        elsif rising_edge(clk) then
            press_pulse <= (others => '0');

            if sample_tick = '1' then
                for i in 0 to N-1 loop

                    if sample_val(i) = '1' then
                        if cnt(i) < THRESH then
                            cnt(i) <= cnt(i) + 1;
                        end if;
                    else
                        if cnt(i) > 0 then
                            cnt(i) <= cnt(i) - 1;
                        end if;
                    end if;

                    if cnt(i) = THRESH and db(i) = '0' then
                        db(i) <= '1';
                        press_pulse(i) <= '1';

                    elsif cnt(i) = 0 and db(i) = '1' and sample_val(i) = '0' then
                        db(i) <= '0';
                    end if;

                end loop;
            end if;
        end if;
    end process;

    --------------------------------------------------------------------
    -- Latch events on press_pulse
    --------------------------------------------------------------------
    process(clk, rstn)
    begin
        if rstn = '0' then
            evt_latch <= (others => '0');

        elsif rising_edge(clk) then
            for i in 0 to N-1 loop
                if press_pulse(i) = '1' then
                    evt_latch(i) <= '1';
                end if;
            end loop;
        end if;
    end process;

    --------------------------------------------------------------------
    -- IRQ output
    --------------------------------------------------------------------
    irq <= '1' when evt_latch /= (others => '0') else '0';

    --------------------------------------------------------------------
    -- MMIO Interface
    --------------------------------------------------------------------
    process(clk, rstn)
    begin
        if rstn = '0' then
            bus_rdata_i <= (others => '0');
            bus_ready_i <= '0';

        elsif rising_edge(clk) then
            bus_ready_i <= '0';

            if bus_read = '1' then

                if bus_addr = OFF_DB(5 downto 0) then
                    bus_rdata_i <= db;
                    bus_ready_i <= '1';

                elsif bus_addr = OFF_EVT(5 downto 0) then
                    bus_rdata_i <= evt_latch;
                    evt_latch   <= (others => '0'); -- clear-on-read
                    bus_ready_i <= '1';

                elsif bus_addr = OFF_CNT(5 downto 0) then
                    bus_rdata_i <= (others => '0');
                    bus_ready_i <= '1';

                else
                    bus_rdata_i <= x"EF"; -- invalid
                    bus_ready_i <= '1';
                end if;

            elsif bus_write = '1' then

                if bus_addr = OFF_CLEAR(5 downto 0) then
                    evt_latch <= evt_latch and not bus_wdata(N-1 downto 0);
                    bus_ready_i <= '1';
                else
                    bus_ready_i <= '1';
                end if;

            end if;
        end if;
    end process;

end architecture;
