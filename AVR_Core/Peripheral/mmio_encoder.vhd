library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.AVRuCPackage.all; -- se serve per OFF_DB ecc.

entity mmio_encoder is
    port (
        clk       : in  std_logic;
        reset     : in  std_logic;

        -- Encoder signals
        enc_a     : in  std_logic;
        enc_b     : in  std_logic;

        -- MMIO bus
        bus_addr  : in  std_logic_vector(5 downto 0);
        bus_wr    : in  std_logic;
        bus_rd    : in  std_logic;
        bus_wdata : in  std_logic_vector(7 downto 0);
        bus_rdata : out std_logic_vector(7 downto 0);
        out_en    : out std_logic
    );
end entity;

architecture rtl of mmio_encoder is

    ----------------------------------------------------------------
    -- Address decode
    ----------------------------------------------------------------
    constant ENC_BASE : std_logic_vector(3 downto 0) := "0111"; -- 0x1C..0x1F
    signal sel_enc    : std_logic;

    ----------------------------------------------------------------
    -- Encoder registers
    ----------------------------------------------------------------
    signal enc_val : unsigned(15 downto 0) := (others => '0');
    signal enc_res : unsigned(7 downto 0)  := (others => '1'); -- default 1

    ----------------------------------------------------------------
    -- Encoder filtering and state
    ----------------------------------------------------------------
    signal a_ff, b_ff : std_logic_vector(1 downto 0) := (others => '0');
    signal enc_a_f, enc_b_f : std_logic;

    signal ab      : std_logic_vector(1 downto 0);
    signal ab_prev : std_logic_vector(1 downto 0);
    signal quad    : std_logic_vector(3 downto 0);

begin

    ----------------------------------------------------------------
    -- Peripheral select
    ----------------------------------------------------------------
    sel_enc <= '1' when bus_addr(5 downto 2) = ENC_BASE else '0';

    ----------------------------------------------------------------
    -- Output enable
    ----------------------------------------------------------------
    out_en <= '1' when (bus_rd = '1' and sel_enc = '1') else '0';

    ----------------------------------------------------------------
    -- Glitch filter (2 FF)
    ----------------------------------------------------------------
    process(clk)
    begin
        if rising_edge(clk) then
            a_ff <= a_ff(0) & enc_a;
            b_ff <= b_ff(0) & enc_b;
        end if;
    end process;

    enc_a_f <= a_ff(1);
    enc_b_f <= b_ff(1);

    ab   <= (1 => enc_a_f, 0 => enc_b_f);
    quad <= ab_prev & ab;

    ----------------------------------------------------------------
    -- Encoder + MMIO write in unico processo
    ----------------------------------------------------------------
    process(clk)
    begin
        if rising_edge(clk) then
            if reset = '1' then
                enc_val  <= (others => '0');
                enc_res  <= (others => '1');
                ab_prev  <= (others => '0');
            else
                -- Quadrature decode
                case quad is
                    when "0001" | "0111" | "1110" | "1000" =>
                        enc_val <= enc_val + enc_res;
                    when "0010" | "0100" | "1101" | "1011" =>
                        enc_val <= enc_val - enc_res;
                    when others =>
                        null;
                end case;

                ab_prev <= ab;

                -- MMIO write
                if sel_enc = '1' and bus_wr = '1' then
                    case bus_addr(1 downto 0) is
                        when "00" => enc_val(7 downto 0)  <= unsigned(bus_wdata);
                        when "01" => enc_val(15 downto 8) <= unsigned(bus_wdata);
                        when "10" => enc_res <= unsigned(bus_wdata); -- write-only
                        when others => null;
                    end case;
                end if;
            end if;
        end if;
    end process;

    ----------------------------------------------------------------
    -- MMIO read
    ----------------------------------------------------------------
    process(all)
    begin
        bus_rdata <= (others => '0');

        if sel_enc = '1' and bus_rd = '1' then
            case bus_addr(1 downto 0) is
                when "00" => bus_rdata <= std_logic_vector(enc_val(7 downto 0));
                when "01" => bus_rdata <= std_logic_vector(enc_val(15 downto 8));
                when others => bus_rdata <= (others => '0');
            end case;
        end if;
    end process;

end architecture;
