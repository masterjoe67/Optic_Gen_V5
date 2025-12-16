library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use WORK.AVRuCPackage.all;

entity mmio_regs_16bit_direct is port(
    clk           : in  std_logic;
    rst           : in  std_logic;

    -- Bus AVR a 8 bit
    core_write    : in  std_logic;
    core_read     : in  std_logic;
    core_addr     : in  std_logic_vector(5 downto 0);
    core_data_in  : in  std_logic_vector(7 downto 0);
    core_data_out : out std_logic_vector(7 downto 0);
    out_en        : out std_logic;

    -- Uscite dirette a 16 bit
    ext_reg0      : out std_logic_vector(15 downto 0);
    ext_reg1      : out std_logic_vector(15 downto 0);
    ext_reg2      : out std_logic_vector(15 downto 0);
    ext_reg3      : out std_logic_vector(15 downto 0)
);
end mmio_regs_16bit_direct;

architecture rtl of mmio_regs_16bit_direct is

    signal r0_l, r0_h : std_logic_vector(7 downto 0) := (others => '0');
    signal r1_l, r1_h : std_logic_vector(7 downto 0) := (others => '0');
    signal r2_l, r2_h : std_logic_vector(7 downto 0) := (others => '0');
    signal r3_l, r3_h : std_logic_vector(7 downto 0) := (others => '0');

    signal r0_l_Sel, r0_h_Sel : std_logic;
    signal r1_l_Sel, r1_h_Sel : std_logic;
    signal r2_l_Sel, r2_h_Sel : std_logic;
    signal r3_l_Sel, r3_h_Sel : std_logic;

begin

    --------------------------------------------------------------------
    -- out_en (abilita l'uscita solo su indirizzi validi)
    --------------------------------------------------------------------
    out_en <= '1' when (
               (core_addr = REG0_L or core_addr = REG0_H or
                core_addr = REG1_L or core_addr = REG1_H or
                core_addr = REG2_L or core_addr = REG2_H or
                core_addr = REG3_L or core_addr = REG3_H)
                and core_read = '1')
            else '0';

    --------------------------------------------------------------------
    -- Decoder di indirizzi
    --------------------------------------------------------------------
    r0_l_Sel <= '1' when core_addr = REG0_L else '0';
    r0_h_Sel <= '1' when core_addr = REG0_H else '0';

    r1_l_Sel <= '1' when core_addr = REG1_L else '0';
    r1_h_Sel <= '1' when core_addr = REG1_H else '0';

    r2_l_Sel <= '1' when core_addr = REG2_L else '0';
    r2_h_Sel <= '1' when core_addr = REG2_H else '0';

    r3_l_Sel <= '1' when core_addr = REG3_L else '0';
    r3_h_Sel <= '1' when core_addr = REG3_H else '0';

    --------------------------------------------------------------------
    -- SCRITTURA TUTTI I 4 REGISTRI (16 bit → due accessi da 8 bit)
    --------------------------------------------------------------------

    -- REG0
    process(clk, rst)
    begin
        if rst = '0' then
            r0_l <= x"03";
        elsif rising_edge(clk) then
            if core_write = '1' and r0_l_Sel = '1' then
                r0_l <= core_data_in;
            end if;
        end if;
    end process;

    process(clk, rst)
    begin
        if rst = '0' then
            r0_h <= x"00";
        elsif rising_edge(clk) then
            if core_write = '1' and r0_h_Sel = '1' then
                r0_h <= core_data_in;
            end if;
        end if;
    end process;


    -- REG1
    process(clk, rst)
    begin
        if rst = '0' then
            r1_l <= x"E7";
        elsif rising_edge(clk) then
            if core_write = '1' and r1_l_Sel = '1' then
                r1_l <= core_data_in;
            end if;
        end if;
    end process;

    process(clk, rst)
    begin
        if rst = '0' then
            r1_h <= x"01";
        elsif rising_edge(clk) then
            if core_write = '1' and r1_h_Sel = '1' then
                r1_h <= core_data_in;
            end if;
        end if;
    end process;


    -- REG2
    process(clk, rst)
    begin
        if rst = '0' then
            r2_l <= x"05";
        elsif rising_edge(clk) then
            if core_write = '1' and r2_l_Sel = '1' then
                r2_l <= core_data_in;
            end if;
        end if;
    end process;

    process(clk, rst)
    begin
        if rst = '0' then
            r2_h <= x"00";
        elsif rising_edge(clk) then
            if core_write = '1' and r2_h_Sel = '1' then
                r2_h <= core_data_in;
            end if;
        end if;
    end process;


    -- REG3
    process(clk, rst)
    begin
        if rst = '0' then
            r3_l <= x"00";
        elsif rising_edge(clk) then
            if core_write = '1' and r3_l_Sel = '1' then
                r3_l <= core_data_in;
            end if;
        end if;
    end process;

    process(clk, rst)
    begin
        if rst = '0' then
            r3_h <= x"00";
        elsif rising_edge(clk) then
            if core_write = '1' and r3_h_Sel = '1' then
                r3_h <= core_data_in;
            end if;
        end if;
    end process;

    --------------------------------------------------------------------
    -- LETTURA (mux combinatorio)
    --------------------------------------------------------------------
    process(core_read, core_addr, r0_l, r0_h, r1_l, r1_h, r2_l, r2_h, r3_l, r3_h)
    begin
        if core_read = '1' then
            case core_addr is
                when REG0_L => core_data_out <= r0_l;
                when REG0_H => core_data_out <= r0_h;

                when REG1_L => core_data_out <= r1_l;
                when REG1_H => core_data_out <= r1_h;

                when REG2_L => core_data_out <= r2_l;
                when REG2_H => core_data_out <= r2_h;

                when REG3_L => core_data_out <= r3_l;
                when REG3_H => core_data_out <= r3_h;

                when others => core_data_out <= (others => '0');
            end case;
        else
            core_data_out <= (others => '0');
        end if;
    end process;

    --------------------------------------------------------------------
    -- USCITE ASINCRONE A 16 BIT
    --------------------------------------------------------------------
    ext_reg0 <= r0_h & r0_l;
    ext_reg1 <= r1_h & r1_l;
    ext_reg2 <= r2_h & r2_l;
    ext_reg3 <= r3_h & r3_l;

end rtl;
