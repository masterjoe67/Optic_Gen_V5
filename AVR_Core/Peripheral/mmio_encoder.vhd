library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;


ENTITY mmio_encoder IS
  GENERIC(
    positions                : INTEGER := 1023;       --size of the position counter (i.e. number of positions counted)
    debounce_time            : INTEGER := 32_000;   --number of clock cycles required to register a new position = debounce_time + 2
    set_origin_debounce_time : INTEGER := 320_000); --number of clock cycles required to register a new set_origin_n value = set_origin_debounce_time + 2
  PORT(
    clk          : IN     STD_LOGIC;                            --system clock
    enc_a            : IN     STD_LOGIC;                            --quadrature encoded signal a
    enc_b            : IN     STD_LOGIC;                            --quadrature encoded signal b
    reset_n    : in  std_logic; -- RESET ATTIVO BASSO
    direction    : OUT    STD_LOGIC;                            --direction of last change, 1 = positive, 0 = negative

	 
	 -- MMIO bus
	  bus_addr   : in  std_logic_vector(5 downto 0);
	  bus_wdata  : in  std_logic_vector(7 downto 0);
	  bus_rdata  : out std_logic_vector(7 downto 0);
	  bus_wr     : in  std_logic;
	  bus_rd     : in  std_logic;
	  out_en     : out std_logic
	  );
	 
END entity;

ARCHITECTURE logic OF mmio_encoder IS
	-- decoder base (bus_addr[5:2])
  constant ENC_BASE : std_logic_vector(3 downto 0) := "0111"; -- 0x1C–0x1F
  
  SIGNAL position     : INTEGER RANGE 0 TO positions-1 := 0; --current position relative to index or initial value
  
  SIGNAL  a_new            : STD_LOGIC_VECTOR(1 DOWNTO 0);                --synchronizer/debounce registers for encoded signal a
  SIGNAL  b_new            : STD_LOGIC_VECTOR(1 DOWNTO 0);                --synchronizer/debounce registers for encoded signal b
  SIGNAL  a_prev           : STD_LOGIC;                                   --last previous stable value of encoded signal a
  SIGNAL  b_prev           : STD_LOGIC;                                   --last previous stable value of encoded signal b
  SIGNAL  debounce_cnt     : INTEGER RANGE 0 TO debounce_time;            --timer to remove glitches and validate stable values of inputs
  SIGNAL  set_origin_n_new : STD_LOGIC_VECTOR(1 DOWNTO 0);                --synchronizer/debounce registers for the set_origin_n input
  SIGNAL  set_origin_n_int : STD_LOGIC;                                   --last debounced value of set_origin_n signal
  SIGNAL  set_origin_cnt   : INTEGER RANGE 0 TO set_origin_debounce_time; --debounce counter for set_origin_n signal
  
  signal sel_enc : std_logic;
  signal enc_lat  : unsigned(15 downto 0);
  
BEGIN

sel_enc <= '1' when bus_addr(5 downto 2) = ENC_BASE else '0';

  PROCESS(clk)
  BEGIN
    IF(clk'EVENT AND clk = '1') THEN                                    --rising edge of system clock
    
      --synchronize and debounce a and b inputs
      a_new <= a_new(0) & enc_a;                                              --shift in new values of 'a'  
      b_new <= b_new(0) & enc_b;                                              --shift in new values of 'b'
      IF(((a_new(0) XOR a_new(1)) OR (b_new(0) XOR b_new(1))) = '1') THEN --a input or b input is changing
        debounce_cnt <= 0;                                                  --clear debounce counter
      ELSIF(debounce_cnt = debounce_time) THEN                            --debounce time is met
        a_prev <= a_new(1);                                                 --update value of a_prev
        b_prev <= b_new(1);                                                 --update value of b_prev
      ELSE                                                                --debounce time is not yet met    
        debounce_cnt <= debounce_cnt + 1;                                   --increment debounce counter
      END IF;
     
      
      --determine direction and position
      IF(reset_n = '0') THEN                                     --inital position is being set
        position <= 0;                                                      --clear position counter
      ELSIF(debounce_cnt = debounce_time                                  --debounce time for a and b is met
          AND ((a_prev XOR a_new(1)) OR (b_prev XOR b_new(1))) = '1') THEN    --AND the new value is different than the previous value
        direction <= b_prev XOR a_new(1);                                   --update the direction
        IF((b_prev XOR a_new(1)) = '1') THEN                                --clockwise direction
          IF(position < positions-1) THEN                                     --not at position limit
            position <= position + 1;                                           --advance position counter
          ELSE                                                                --at position limit
            position <= 0;                                                      --roll over position counter to zero
          END IF;
        ELSE                                                                --counter-clockwise direction
          IF(position > 0) THEN                                               --not at position limit
            position <= position - 1;                                           --decrement position counter
          ELSE                                                                --at position limit
            position <= positions-1;                                            --roll over position counter maximum
          END IF;
        END IF;
      END IF;
      
    END IF;

END PROCESS;
  
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
					 --when "10" => bus_rdata <= std_logic_vector(enc_res(7 downto 0));
		
                when others => bus_rdata <= (others => '0');
            end case;
        end if;
    end process;
	 
	 enc_lat <= to_unsigned(position, 16);

END logic;
