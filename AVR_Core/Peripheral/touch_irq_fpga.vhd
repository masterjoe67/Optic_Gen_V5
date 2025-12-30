library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity touch_irq_fpga is
    Port (
        clk          : in  std_logic;  -- clock FPGA
        rst_n        : in  std_logic;  -- reset attivo basso
        penirq_in    : in  std_logic;  -- pin fisico PENIRQ (attivo basso)
        int0_out     : out std_logic   -- output per core
    );
end touch_irq_fpga;

architecture Behavioral of touch_irq_fpga is

    -- sincronizzazione su 2 FF
    signal penirq_sync : std_logic_vector(1 downto 0) := (others=>'1');
    signal penirq_clean : std_logic := '1';

    -- debounce counter
    signal counter : unsigned(7 downto 0) := (others=>'0');

    -- edge detect
    signal penirq_ff : std_logic := '1';

begin

    -- ================= sincronia e debounce =================
    process(clk)
    begin
        if rising_edge(clk) then
            if rst_n='0' then               -- reset attivo basso
                penirq_sync <= (others=>'1');
                penirq_clean <= '1';
                counter <= (others=>'0');
            else
                -- sincronizza il pin in 2 FF
                penirq_sync(0) <= penirq_in;
                penirq_sync(1) <= penirq_sync(0);

                -- debounce semplice: cambia stato solo se stabile per 16 cicli
                if penirq_sync(1) /= penirq_clean then
                    counter <= counter + 1;
                    if counter = 16 then
                        penirq_clean <= penirq_sync(1);
                        counter <= (others=>'0');
                    end if;
                else
                    counter <= (others=>'0');
                end if;
            end if;
        end if;
    end process;

    -- ================= edge/pulse generator =================
    process(clk)
    begin
        if rising_edge(clk) then
            if rst_n='0' then               -- reset attivo basso
                penirq_ff <= '1';
                int0_out <= '0';
            else
                -- rileva transizione HIGH (dito premuto invertito)
                int0_out <= penirq_clean and not penirq_ff;
                penirq_ff <= penirq_clean;
            end if;
        end if;
    end process;

end Behavioral;
