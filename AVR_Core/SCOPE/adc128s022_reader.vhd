library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity adc128s022_reader is
    port(
        clk     : in  std_logic;   -- 40 MHz
        rst_n   : in  std_logic;

        sclk    : out std_logic;
        cs_n    : out std_logic;
        mosi    : out std_logic;
        miso    : in  std_logic;

        ch0     : out unsigned(11 downto 0);
        ch1     : out unsigned(11 downto 0);
        ch2     : out unsigned(11 downto 0)
    );
end entity;

architecture rtl of adc128s022_reader is

    type ADC_STATE is (ADC_SEL, TRIGGER, ACQUIRE, STORE);
    signal STATE : ADC_STATE := ADC_SEL;
    
    signal oCLK   : STD_LOGIC := '0';
    signal CS_BUF : STD_LOGIC := '1';
    
    signal reg_ch0, reg_ch1, reg_ch2 : unsigned(11 downto 0) := (others => '0');
    
    -- Pipeline a due stadi per non sbagliare mai il canale
    signal ch_req_d1 : integer range 0 to 3 := 0; -- Canale chiesto ora
    signal ch_req_d2 : integer range 0 to 3 := 0; -- Canale chiesto nel ciclo precedente

begin
    cs_n <= CS_BUF;
    ch0  <= reg_ch0;
    ch1  <= reg_ch1;
    ch2  <= reg_ch2;

    sclk <= not oCLK when STATE = ACQUIRE else '1';

    -- Clock ADC a 4MHz (40MHz / 10)
    process(clk, rst_n)
        variable count : integer range 0 to 6 := 0;
    begin
        if rst_n = '0' then
            count := 0; oCLK <= '0';
        elsif rising_edge(clk) then
            if count = 6 then
                oCLK <= not oCLK; count := 0;
            else
                count := count + 1;
            end if;
        end if;
    end process;

    process(oCLK, rst_n)
        variable DATA_COUNT : integer range 0 to 15 := 0;
        variable ADC_CH_INDEX : integer range 0 to 4 := 0; -- Contatore 0,1,2,3
        variable ADC_DATA   : std_logic_vector(11 downto 0);
        variable iCH        : std_logic_vector(3 downto 0);
    begin
        if rst_n = '0' then
            STATE <= ADC_SEL; 
            CS_BUF <= '1';
            ADC_CH_INDEX := 0;
            ch_req_d1 <= 0;
            ch_req_d2 <= 0;
        elsif rising_edge(oCLK) then
            case STATE is

                when ADC_SEL =>
                    -- Selezioniamo il canale (0, 1, 2 o 3 fantasma)
                    iCH := std_logic_vector(to_unsigned(ADC_CH_INDEX, 4));
                    
                

                    STATE <= TRIGGER;

                when TRIGGER =>
                    CS_BUF <= '0';
                    STATE  <= ACQUIRE;

                when ACQUIRE =>
                    case DATA_COUNT is
                        when 1  => mosi <= iCH(2);
                        when 2  => mosi <= iCH(1);
                        when 3  => mosi <= iCH(0);
                        when 4  => ADC_DATA(11) := miso;
                        when 5  => ADC_DATA(10) := miso;
                        when 6  => ADC_DATA(9)  := miso;
                        when 7  => ADC_DATA(8)  := miso;
                        when 8  => ADC_DATA(7)  := miso;
                        when 9  => ADC_DATA(6)  := miso;
                        when 10 => ADC_DATA(5)  := miso;
                        when 11 => ADC_DATA(4)  := miso;
                        when 12 => ADC_DATA(3)  := miso;
                        when 13 => ADC_DATA(2)  := miso;
                        when 14 => ADC_DATA(1)  := miso;
                        when 15 => ADC_DATA(0)  := miso;
                        when others => null;
                    end case;

                    if DATA_COUNT = 15 then
                        DATA_COUNT := 0; 
                        STATE <= STORE;
                    else
                        DATA_COUNT := DATA_COUNT + 1;
                    end if;

                when STORE =>
                    CS_BUF <= '1';
                    
                    -- Il dato ricevuto ADESSO è quello chiesto DUE CICLI FA (ch_req_d2)
                    case ADC_CH_INDEX is
                        when 0 => reg_ch0 <= unsigned(ADC_DATA);
                        when 1 => reg_ch1 <= unsigned(ADC_DATA);
                        when 2 => reg_ch2 <= unsigned(ADC_DATA);
                        when others => null; -- Ignoriamo il canale 3
                    end case;

                    -- Incremento circolare su 4 canali (0,1,2,3)
                    if ADC_CH_INDEX = 4 then 
                        ADC_CH_INDEX := 0;
                    else 
                        ADC_CH_INDEX := ADC_CH_INDEX + 1;
                    end if;

                    STATE <= ADC_SEL;
                    
            end case;
        end if;
    end process;
end architecture;