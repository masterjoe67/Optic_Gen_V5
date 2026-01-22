library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.AVRuCPackage.all;

entity oscilloscope_top is
    port(
        clk        : in  std_logic;
        rst_n      : in  std_logic;

        -- SPI ADC
        sclk       : out std_logic;
        cs_n       : out std_logic;
        miso       : in  std_logic;
        mosi       : out std_logic;

        -- MMIO interface
        iore       : in  std_logic;
        mmio_addr  : in  std_logic_vector(6 downto 0);
        mmio_wdata : in  std_logic_vector(7 downto 0);
        mmio_we    : in  std_logic;
        mmio_rdata : out std_logic_vector(7 downto 0);
        out_en     : out std_logic;

        -- Debug / testbench
        tb_wr_ptr     : out unsigned(9 downto 0);
        tb_trig_index : out unsigned(9 downto 0);
        tb_pre_cnt    : out unsigned(9 downto 0);
        tb_post_cnt   : out unsigned(9 downto 0);
        tb_auto_cnt   : out unsigned(15 downto 0);
        tb_trig_hit   : out std_logic
    );
end entity;

architecture rtl of oscilloscope_top is

    ------------------------------------------------------------------
    -- Costanti di sistema
    ------------------------------------------------------------------
    constant BUFFER_SIZE      : integer := 1024;
    constant PRE_TRIGGER      : integer := 150;
    constant POST_TRIGGER_LEN : integer := 150;
    constant AUTO_TIMEOUT     : unsigned(15 downto 0) := to_unsigned(2050, 16);

    ------------------------------------------------------------------
    -- Registri MMIO
    ------------------------------------------------------------------
    signal reg_index_int  : unsigned(9 downto 0)  := (others => '0');
    signal reg_base_time  : unsigned(31 downto 0) := (others => '0');
    signal reg_trig_level : unsigned(11 downto 0) := (others => '0');
    signal reg_trig_ctrl  : std_logic_vector(7 downto 0) := (others => '0');

    ------------------------------------------------------------------
    -- Decode scritture MMIO
    ------------------------------------------------------------------
    signal index_reg_sel : std_logic;
    signal trig_reg_sel  : std_logic;
    signal base_reg_sel  : std_logic;
    signal trig_ctrl_sel : std_logic;
    signal trig_cmd_sel  : std_logic;

    ------------------------------------------------------------------
    -- FSM
    ------------------------------------------------------------------
    type fsm_state_t is (IDLE, PRE_FILL, ARMED, POST_TRIGGER, HOLD);
    signal state, next_state : fsm_state_t;

    ------------------------------------------------------------------
    -- Contatori e puntatori
    ------------------------------------------------------------------
    signal post_cnt         : unsigned(9 downto 0) := (others => '0');
    signal rd_index         : unsigned(9 downto 0);
    signal pre_cnt          : unsigned(9 downto 0);
    signal auto_cnt         : unsigned(15 downto 0);
    signal wr_ptr           : unsigned(9 downto 0);
    signal trig_index       : unsigned(9 downto 0);
    signal rd_base_latched  : unsigned(9 downto 0) := (others => '0');
    signal trig_wr_ptr      : unsigned(9 downto 0) := (others => '0');
    signal rd_cha_strobe    : std_logic := '0';
    signal ready_latched    : std_logic := '0';

    ------------------------------------------------------------------
    -- Base tempi / tick
    ------------------------------------------------------------------
    signal base_time_cnt : unsigned(31 downto 0) := (others => '0');
    signal tick          : std_logic := '0';
    signal tick_en       : std_logic := '0';

    ------------------------------------------------------------------
    -- Trigger / controllo acquisizione
    ------------------------------------------------------------------
    signal trig_hit           : std_logic;
    signal mode               : std_logic_vector(1 downto 0);
    signal write_enable       : std_logic;
    signal save_trig          : std_logic;
    signal freeze             : std_logic;
    signal ready              : std_logic := '0';
    signal rearm_pulse        : std_logic := '0';
    signal auto_timeout_hit   : std_logic;

    ------------------------------------------------------------------
    -- ADC samples
    ------------------------------------------------------------------
    signal trig_sample      : unsigned(9 downto 0);
    signal prev_sample      : unsigned(9 downto 0);
    signal trig_sample_sync : unsigned(9 downto 0);

    ------------------------------------------------------------------
    -- MMIO byte counters
    ------------------------------------------------------------------
    signal base_bytecnt : unsigned(2 downto 0);
    signal base_shift   : unsigned(31 downto 0);
    signal trig_bytecnt : unsigned(1 downto 0);
    signal trig_shift   : unsigned(23 downto 0);
    signal wr_timeout   : unsigned(15 downto 0) := (others => '0');

    ------------------------------------------------------------------
    -- ADC channels / RAM
    ------------------------------------------------------------------
    signal adc_a      : unsigned(11 downto 0);
    signal adc_b      : unsigned(11 downto 0);
    signal adc_c      : unsigned(11 downto 0);

    signal ram_a_out  : unsigned(11 downto 0);
    signal ram_b_out  : unsigned(11 downto 0);
    signal ram_c_out  : unsigned(11 downto 0);

    ------------------------------------------------------------------
    -- Trigger configuration
    ------------------------------------------------------------------
    signal trig_level     : unsigned(11 downto 0);
    signal trig_chan_sel  : std_logic_vector(1 downto 0);
    signal trig_edge      : std_logic;
    signal trig_enable    : std_logic;

    ------------------------------------------------------------------
    -- ADC reader control (presenti anche se non usati direttamente)
    ------------------------------------------------------------------
    signal adc_start : std_logic := '0';
    signal adc_busy  : std_logic;
    signal adc_tick  : std_logic := '0';
    signal adc_div   : unsigned(15 downto 0) := (others => '0');

    ------------------------------------------------------------------
    -- Utility function
    ------------------------------------------------------------------
    function wrap_sub(a, b : unsigned; size : integer) return unsigned is
    begin
        if a < b then
            return a + to_unsigned(size - to_integer(b), a'length);
        else
            return a - b;
        end if;
    end function;


    ------------------------------------------------------------------
    -- ADC Reader component
    ------------------------------------------------------------------
    component adc128s022_reader is
        port (
            clk     : in  std_logic;
            rst_n   : in  std_logic;
            sclk    : out std_logic;
            cs_n    : out std_logic;
            mosi    : out std_logic;
            miso    : in  std_logic;
            ch0     : out unsigned(11 downto 0);
            ch1     : out unsigned(11 downto 0);
            ch2     : out unsigned(11 downto 0)
        );
    end component;

begin

    ------------------------------------------------------------------
    -- MMIO register select
    -- Decodifica indirizzi MMIO per scrittura registri interni
    ------------------------------------------------------------------
    index_reg_sel <= '1' when (mmio_addr = REG_INDEX and mmio_we = '1') else '0';
    trig_reg_sel  <= '1' when (mmio_addr = REG_CHA   and mmio_we = '1') else '0';
    base_reg_sel  <= '1' when (mmio_addr = REG_CHB   and mmio_we = '1') else '0';
    trig_ctrl_sel <= '1' when (mmio_addr = REG_CHC   and mmio_we = '1') else '0';
    trig_cmd_sel  <= '1' when (mmio_addr = REG_TRIG  and mmio_we = '1') else '0';

    ------------------------------------------------------------------
    -- Read index calculation
    -- Calcolo indice di lettura RAM con wrap su BUFFER_SIZE
    ------------------------------------------------------------------
    rd_index <= (rd_base_latched + reg_index_int) -
                to_unsigned(BUFFER_SIZE, rd_index'length)
                when (rd_base_latched + reg_index_int) >=
                     to_unsigned(BUFFER_SIZE, rd_index'length)
                else
                rd_base_latched + reg_index_int;

    ------------------------------------------------------------------
    -- Trigger sample selection
    -- Selezione canale ADC per la comparazione di trigger
    ------------------------------------------------------------------
    with trig_chan_sel select
        trig_sample <= adc_a(11 downto 2) when "00",
                       adc_b(11 downto 2) when "01",
                       adc_c(11 downto 2) when "10",
                       (others => '0')    when others;

    -- Decodifica registro trigger control
    mode          <= reg_trig_ctrl(7 downto 6);
    trig_chan_sel <= reg_trig_ctrl(5 downto 4);
    trig_edge     <= reg_trig_ctrl(3);
    trig_enable   <= reg_trig_ctrl(2);
    trig_level    <= reg_trig_level;

    -- Segnale READY combinatorio (stato HOLD)
    ready <= '1' when state = HOLD else '0';

    ------------------------------------------------------------------
    -- Debug outputs
    ------------------------------------------------------------------
    tb_wr_ptr     <= wr_ptr;
    tb_trig_index <= trig_index;
    tb_pre_cnt    <= pre_cnt;
    tb_post_cnt   <= post_cnt;
    tb_auto_cnt   <= auto_cnt;
    tb_trig_hit   <= trig_hit;

    -- Timeout AUTO valido solo in modalità AUTO (mode = "00")
    auto_timeout_hit <= '1'
        when (mode = "00") and (auto_cnt >= AUTO_TIMEOUT)
        else '0';

    ------------------------------------------------------------------
    -- Rearm command latch (1 ciclo)
    ------------------------------------------------------------------
    process(clk, rst_n)
    begin
        if rst_n = '0' then
            rearm_pulse <= '0';
        elsif rising_edge(clk) then
            rearm_pulse <= '0';
            if mmio_we = '1' and trig_cmd_sel = '1' and mmio_wdata(0) = '1' then
                rearm_pulse <= '1';
            end if;
        end if;
    end process;

    ------------------------------------------------------------------
    -- ADC sample synchronization
    -- Pipeline a 2 stadi per rilevamento fronte
    ------------------------------------------------------------------
    process(clk, rst_n)
    begin
        if rst_n = '0' then
            prev_sample      <= (others => '0');
            trig_sample_sync <= (others => '0');
        elsif rising_edge(clk) then
            trig_sample_sync <= trig_sample;
            prev_sample      <= trig_sample_sync;
        end if;
    end process;

    ------------------------------------------------------------------
    -- Trigger detection
    -- Rilevamento fronte di salita o discesa sul livello di trigger
    ------------------------------------------------------------------
    process(clk, rst_n)
    begin
        if rst_n = '0' then
            trig_hit    <= '0';
            trig_wr_ptr <= (others => '0');
        elsif rising_edge(clk) then
            trig_hit <= '0';

            if trig_enable = '1' then
                if trig_edge = '0' then
                    -- Rising edge
                    if (prev_sample < trig_level) and
                       (trig_sample_sync >= trig_level) then
                        trig_hit <= '1';
                        if wr_ptr = 0 then
                            trig_wr_ptr <= to_unsigned(BUFFER_SIZE - 1, wr_ptr'length);
                        else
                            trig_wr_ptr <= wr_ptr - 1;
                        end if;
                    end if;
                else
                    -- Falling edge
                    if (prev_sample > trig_level) and
                       (trig_sample_sync <= trig_level) then
                        trig_hit <= '1';
                        if wr_ptr = 0 then
                            trig_wr_ptr <= to_unsigned(BUFFER_SIZE - 1, wr_ptr'length);
                        else
                            trig_wr_ptr <= wr_ptr - 1;
                        end if;
                    end if;
                end if;
            end if;
        end if;
    end process;

    ------------------------------------------------------------------
    -- Tick generator
    -- Genera tick_en a frequenza impostata da reg_base_time
    ------------------------------------------------------------------
    process(clk, rst_n)
    begin
        if rst_n = '0' then
            tick          <= '0';
            tick_en       <= '0';
            base_time_cnt <= (others => '0');
        elsif rising_edge(clk) then
            if base_time_cnt >= reg_base_time then
                base_time_cnt <= (others => '0');
                tick          <= '1';
                tick_en       <= '1';
            else
                base_time_cnt <= base_time_cnt + 1;
                tick          <= '0';
                tick_en       <= '0';
            end if;
        end if;
    end process;

    ------------------------------------------------------------------
    -- Write pointer increment
    -- Avanza il puntatore di scrittura RAM
    ------------------------------------------------------------------
    process(clk, rst_n)
    begin
        if rst_n = '0' then
            wr_ptr <= (others => '0');
        elsif rising_edge(clk) then
            if write_enable = '1' and tick_en = '1' then
                if wr_ptr = to_unsigned(BUFFER_SIZE - 1, wr_ptr'length) then
                    wr_ptr <= (others => '0');
                else
                    wr_ptr <= wr_ptr + 1;
                end if;
            end if;
        end if;
    end process;

    ------------------------------------------------------------------
    -- Dual-port RAM instantiation (3 canali)
    ------------------------------------------------------------------
    ram_a : entity work.dp_ram_1024x12
        port map(
            clk_wr   => clk,
            clk_rd   => clk,
            wr_en    => write_enable,
            addr_wr  => wr_ptr,
            data_in  => adc_a,
            addr_rd  => rd_index,
            data_out => ram_a_out
        );

    ram_b : entity work.dp_ram_1024x12
        port map(
            clk_wr   => clk,
            clk_rd   => clk,
            wr_en    => write_enable,
            addr_wr  => wr_ptr,
            data_in  => adc_b,
            addr_rd  => rd_index,
            data_out => ram_b_out
        );

    ram_c : entity work.dp_ram_1024x12
        port map(
            clk_wr   => clk,
            clk_rd   => clk,
            wr_en    => write_enable,
            addr_wr  => wr_ptr,
            data_in  => adc_c,
            addr_rd  => rd_index,
            data_out => ram_c_out
        );

    ------------------------------------------------------------------
    -- MMIO write logic
    -- Gestione registri multi-byte e timeout di scrittura
    ------------------------------------------------------------------
    process(clk, rst_n)
    begin
        if rst_n = '0' then
            reg_index_int  <= (others => '0');
            reg_trig_level <= to_unsigned(512, 12);
            reg_trig_ctrl  <= (others => '0');
            base_bytecnt   <= (others => '0');
            base_shift     <= (others => '0');
            trig_shift     <= (others => '0');
            trig_bytecnt   <= (others => '0');
            reg_base_time  <= to_unsigned(20000, 32);

        elsif rising_edge(clk) then

            -- Timeout sequenze MMIO multi-byte
            if mmio_we = '1' then
                wr_timeout <= to_unsigned(20000, wr_timeout'length);
            elsif wr_timeout /= 0 then
                wr_timeout <= wr_timeout - 1;
            else
                trig_bytecnt <= (others => '0');
                base_bytecnt <= (others => '0');
            end if;

            -- Registro indice
            if index_reg_sel = '1' and mmio_we = '1' then
                reg_index_int <= "00" & unsigned(mmio_wdata);

            -- Aggiornamento automatico post-trigger
            elsif trig_hit = '1' then
                if trig_wr_ptr < to_unsigned(PRE_TRIGGER, 10) then
                    reg_index_int <= trig_wr_ptr +
                                     to_unsigned(BUFFER_SIZE - PRE_TRIGGER, 10);
                else
                    reg_index_int <= trig_wr_ptr -
                                     to_unsigned(PRE_TRIGGER, 10);
                end if;

            -- Auto-incremento lettura CHA
            elsif rd_cha_strobe = '1' then
                reg_index_int <= reg_index_int + 1;
            end if;

            -- Scrittura BASE (32 bit)
            if mmio_we = '1' and base_reg_sel = '1' then
                if base_bytecnt = "000" then
                    base_shift(7 downto 0) <= unsigned(mmio_wdata);
                elsif base_bytecnt = "001" then
                    base_shift(15 downto 8) <= unsigned(mmio_wdata);
                elsif base_bytecnt = "010" then
                    base_shift(23 downto 16) <= unsigned(mmio_wdata);
                elsif base_bytecnt = "011" then
                    base_shift(31 downto 24) <= unsigned(mmio_wdata);
                    reg_base_time <= base_shift;
                end if;

                if base_bytecnt = "011" then
                    base_bytecnt <= (others => '0');
                else
                    base_bytecnt <= base_bytecnt + 1;
                end if;
            end if;

            -- Scrittura livello trigger (12 bit)
            if mmio_we = '1' and trig_reg_sel = '1' then
                if trig_bytecnt = "00" then
                    trig_shift(7 downto 0) <= unsigned(mmio_wdata);
                elsif trig_bytecnt = "01" then
                    trig_shift(15 downto 8) <= unsigned(mmio_wdata);
                elsif trig_bytecnt = "10" then
                    trig_shift(23 downto 16) <= unsigned(mmio_wdata);
                    reg_trig_level <= trig_shift(11 downto 0);
                end if;

                if trig_bytecnt = "10" then
                    trig_bytecnt <= (others => '0');
                else
                    trig_bytecnt <= trig_bytecnt + 1;
                end if;
            end if;

            -- Registro trigger control
            if mmio_we = '1' and trig_ctrl_sel = '1' then
                reg_trig_ctrl <= mmio_wdata;
            end if;

        end if;
    end process;

    ------------------------------------------------------------------
    -- MMIO read logic
    ------------------------------------------------------------------
    process(mmio_addr, iore)
    begin
        mmio_rdata    <= (others => '0');
        out_en        <= '0';
        rd_cha_strobe <= '0';

        if iore = '1' then
            case mmio_addr is
                when REG_CHA =>
                    mmio_rdata    <= std_logic_vector(
                                        to_unsigned(to_integer(ram_a_out) / 16, 8)
                                     );
                    out_en        <= '1';
                    rd_cha_strobe <= '1';

                when REG_CHB =>
                    mmio_rdata <= std_logic_vector(
                                     to_unsigned(to_integer(ram_b_out) / 16, 8)
                                  );
                    out_en <= '1';

                when REG_CHC =>
                    mmio_rdata <= std_logic_vector(
                                     to_unsigned(to_integer(ram_c_out) / 16, 8)
                                  );
                    out_en <= '1';

                when REG_INDEX =>
                    mmio_rdata <= std_logic_vector(reg_index_int(7 downto 0));
                    out_en <= '1';

                when REG_TRIG =>
                    mmio_rdata <= reg_trig_ctrl or
                                  ("000000" & ready_latched & '0');
                    out_en <= '1';

                when others =>
                    out_en <= '0';
            end case;
        end if;
    end process;

    ------------------------------------------------------------------
    -- FSM state register
    ------------------------------------------------------------------
    process(clk, rst_n)
    begin
        if rst_n = '0' then
            state <= IDLE;
        elsif rising_edge(clk) then
            state <= next_state;
        end if;
    end process;

    ------------------------------------------------------------------
    -- READY latch (sticky fino a lettura REG_TRIG)
    ------------------------------------------------------------------
    process(clk, rst_n)
    begin
        if rst_n = '0' then
            ready_latched <= '0';
        elsif rising_edge(clk) then
            if state = HOLD then
                ready_latched <= '1';
            elsif mmio_addr = REG_TRIG and iore = '1' then
                ready_latched <= '0';
            end if;
        end if;
    end process;

    ------------------------------------------------------------------
    -- FSM next state logic
    ------------------------------------------------------------------
    process(state, trig_hit, auto_timeout_hit,
            pre_cnt, post_cnt, rearm_pulse)
    begin
        next_state <= state;

        case state is
            when IDLE =>
                next_state <= PRE_FILL;

            when PRE_FILL =>
                if pre_cnt = PRE_TRIGGER then
                    next_state <= ARMED;
                end if;

            when ARMED =>
                if trig_hit = '1' or auto_timeout_hit = '1' then
                    next_state <= POST_TRIGGER;
                end if;

            when POST_TRIGGER =>
                if post_cnt = to_unsigned(POST_TRIGGER_LEN, post_cnt'length) then
                    next_state <= HOLD;
                end if;

            when HOLD =>
                if rearm_pulse = '1' then
                    next_state <= IDLE;
                end if;

            when others =>
                next_state <= IDLE;
        end case;
    end process;

    ------------------------------------------------------------------
    -- save_trig pulse (1 ciclo)
    ------------------------------------------------------------------
    process(clk, rst_n)
    begin
        if rst_n = '0' then
            save_trig <= '0';
        elsif rising_edge(clk) then
            if state = ARMED and next_state = POST_TRIGGER then
                save_trig <= '1';
            else
                save_trig <= '0';
            end if;
        end if;
    end process;

    ------------------------------------------------------------------
    -- Write enable / freeze logic
    ------------------------------------------------------------------
    process(state, post_cnt)
    begin
        write_enable <= '0';
        freeze       <= '0';

        case state is
            when PRE_FILL | ARMED =>
                write_enable <= '1';

            when POST_TRIGGER =>
                if post_cnt < to_unsigned(POST_TRIGGER_LEN, post_cnt'length) then
                    write_enable <= '1';
                end if;

            when HOLD =>
                freeze <= '1';

            when others =>
                null;
        end case;
    end process;

    ------------------------------------------------------------------
    -- PRE / POST / AUTO counters
    ------------------------------------------------------------------
    process(clk, rst_n)
    begin
        if rst_n = '0' then
            pre_cnt  <= (others => '0');
            post_cnt <= (others => '0');
            auto_cnt <= (others => '0');

        elsif rising_edge(clk) then
            if tick_en = '1' then
                case state is
                    when PRE_FILL =>
                        pre_cnt <= pre_cnt + 1;

                    when ARMED =>
                        auto_cnt <= auto_cnt + 1;

                    when POST_TRIGGER =>
                        post_cnt <= post_cnt + 1;

                    when others =>
                        pre_cnt  <= (others => '0');
                        post_cnt <= (others => '0');
                        auto_cnt <= (others => '0');
                end case;
            end if;
        end if;
    end process;

    ------------------------------------------------------------------
    -- Trigger index and read base latch
    ------------------------------------------------------------------
    process(clk, rst_n)
    begin
        if rst_n = '0' then
            trig_index      <= (others => '0');
            rd_base_latched <= (others => '0');

        elsif rising_edge(clk) then
            if save_trig = '1' then
                trig_index <= wr_ptr;

                if trig_hit = '1' then
                    if trig_wr_ptr < to_unsigned(PRE_TRIGGER, trig_wr_ptr'length) then
                        rd_base_latched <= trig_wr_ptr +
                                           to_unsigned(BUFFER_SIZE - PRE_TRIGGER,
                                                       rd_base_latched'length);
                    else
                        rd_base_latched <= trig_wr_ptr -
                                           to_unsigned(PRE_TRIGGER,
                                                       rd_base_latched'length);
                    end if;
                else
                    if wr_ptr < to_unsigned(PRE_TRIGGER, wr_ptr'length) then
                        rd_base_latched <= wr_ptr +
                                           to_unsigned(BUFFER_SIZE - PRE_TRIGGER,
                                                       rd_base_latched'length);
                    else
                        rd_base_latched <= wr_ptr -
                                           to_unsigned(PRE_TRIGGER,
                                                       rd_base_latched'length);
                    end if;
                end if;
            end if;
        end if;
    end process;

    ------------------------------------------------------------------
    -- ADC reader instantiation
    ------------------------------------------------------------------
    adc_reader_inst : entity work.adc128s022_reader
        port map(
            clk   => clk,
            rst_n => rst_n,
            miso  => miso,
            mosi  => mosi,
            sclk  => sclk,
            cs_n  => cs_n,
            ch0   => adc_a,
            ch1   => adc_b,
            ch2   => adc_c
        );

end architecture;

