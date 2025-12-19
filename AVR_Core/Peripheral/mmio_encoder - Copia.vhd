library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity mmio_encoder is
    port (
        clk        : in  std_logic;
        reset_n    : in  std_logic; -- RESET ATTIVO BASSO

        -- Encoder fisico
        enc_a      : in  std_logic;
        enc_b      : in  std_logic;

        -- MMIO bus
        bus_addr   : in  std_logic_vector(5 downto 0);
        bus_wdata  : in  std_logic_vector(7 downto 0);
        bus_rdata  : out std_logic_vector(7 downto 0);
        bus_wr     : in  std_logic;
        bus_rd     : in  std_logic;

        out_en     : out std_logic
    );
end entity;

architecture rtl of mmio_encoder is

    -- decoder base (bus_addr[5:2])
    constant ENC_BASE : std_logic_vector(3 downto 0) := "0111"; -- 0x1C–0x1F

    signal sel_enc : std_logic;

    -- encoder core
    signal enc_val  : unsigned(15 downto 0);
    signal enc_lat  : unsigned(15 downto 0);
    signal enc_res  : unsigned(7 downto 0);

    signal enc_en   : std_logic;

    signal ab       : std_logic_vector(1 downto 0);
    signal ab_prev  : std_logic_vector(1 downto 0);

    signal latch_done : std_logic;

    -- modalità quadratura: 00=X1, 01=X2, 10=X4
    signal quad_mode : std_logic_vector(1 downto 0);

begin

    sel_enc <= '1' when bus_addr(5 downto 2) = ENC_BASE else '0';
    ab <= enc_a & enc_b;

    ----------------------------------------------------------------
    -- Encoder + MMIO write + control
    ----------------------------------------------------------------
    process(clk)
        variable quad : std_logic_vector(3 downto 0);
    begin
        if rising_edge(clk) then
            if reset_n = '0' then
                enc_val    <= (others => '0');
                enc_lat    <= (others => '0');
                enc_res    <= (others => '1');
                enc_en     <= '1';
                ab_prev    <= (others => '0');
                latch_done <= '0';
                quad_mode  <= "00"; -- default X1
            else

                ----------------------------------------------------------------
                -- Encoder count
                ----------------------------------------------------------------
                if enc_en = '1' then
                    quad := ab_prev & ab;

                    case quad_mode is
                        when "00" =>  -- X1
                            if ab_prev(1) = '0' and ab(1) = '1' then  -- A rising
                                if ab(0) = '0' then
                                    enc_val <= enc_val + enc_res;
                                else
                                    enc_val <= enc_val - enc_res;
                                end if;
                            end if;

                        when "01" =>  -- X2
                            if ab_prev(1) /= ab(1) then  -- fronte A
                                if ab(0) = ab(1) then
                                    enc_val <= enc_val + enc_res;
                                else
                                    enc_val <= enc_val - enc_res;
                                end if;
                            end if;

                        when "10" =>  -- X4
                            case quad is
                                when "0001" | "0111" | "1110" | "1000" =>
                                    enc_val <= enc_val + enc_res;
                                when "0010" | "0100" | "1101" | "1011" =>
                                    enc_val <= enc_val - enc_res;
                                when others =>
                                    null;
                            end case;

                        when others =>
                            null;
                    end case;
                end if;

                ab_prev <= ab;

                ----------------------------------------------------------------
                -- MMIO write
                ----------------------------------------------------------------
                if sel_enc = '1' and bus_wr = '1' then
                    case bus_addr(1 downto 0) is
                        when "10" => -- RES
                            enc_res <= unsigned(bus_wdata);

                        when "11" => -- CTRL
                            enc_en <= bus_wdata(0);

                            if bus_wdata(1) = '1' then
                                enc_val <= (others => '0'); -- CLEAR
                            end if;

                            if bus_wdata(2) = '1' then
                                enc_lat    <= enc_val;     -- LATCH
                                latch_done <= '1';
                            end if;

                            quad_mode <= bus_wdata(4 downto 3); -- bits 3-4 = mode X1/X2/X4

                        when others =>
                            null;
                    end case;
                end if;

                ----------------------------------------------------------------
                -- Latch reset after full read
                ----------------------------------------------------------------
                if sel_enc = '1' and bus_rd = '1' and bus_addr(1 downto 0) = "01" then
                    latch_done <= '0';
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
        out_en    <= '0';

        if sel_enc = '1' and bus_rd = '1' then
            out_en <= '1';

            case bus_addr(1 downto 0) is
                when "00" => bus_rdata <= std_logic_vector(enc_lat(7 downto 0));
                when "01" => bus_rdata <= std_logic_vector(enc_lat(15 downto 8));
					 when "10" => bus_rdata <= std_logic_vector(enc_res(7 downto 0));
		
                when others => bus_rdata <= (others => '0');
            end case;
        end if;
    end process;

end architecture;
