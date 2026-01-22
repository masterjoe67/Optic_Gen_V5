library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity adc128s022_reader is
    port(
        clk     : in  std_logic;   -- es. 50 MHz
        rst_n   : in  std_logic;

        sclk    : out std_logic;
        cs_n    : out std_logic;
        mosi    : out std_logic;
        miso    : in  std_logic;

        ch0     : out unsigned(11 downto 0);
        ch1     : out unsigned(11 downto 0);
        ch2     : out unsigned(11 downto 0);
		  ch3     : out unsigned(11 downto 0);
		  ch4     : out unsigned(11 downto 0);
		  ch5     : out unsigned(11 downto 0);
		  ch6     : out unsigned(11 downto 0);
		  ch7     : out unsigned(11 downto 0)
    );
end entity;

architecture rtl of adc128s022_reader is

type   ADC_STATE is ( ADC_SEL, TRIGGER, ACQUIRE, STORE );
signal NEXT_STATE   : ADC_STATE;
signal oCLK	        : STD_LOGIC := '0';
signal CS_BUF       : STD_LOGIC := '0';
signal TEST_OUT_BUF : STD_LOGIC := '0';

	--- ADC Datas ---
signal	ADC_CH0  : INTEGER RANGE 0 TO 4095;
signal	ADC_CH1  : INTEGER RANGE 0 TO 4095;
signal	ADC_CH2  : INTEGER RANGE 0 TO 4095;
signal	ADC_CH3  : INTEGER RANGE 0 TO 4095;
signal	ADC_CH4  : INTEGER RANGE 0 TO 4095;
signal	ADC_CH5  : INTEGER RANGE 0 TO 4095;
signal	ADC_CH6  : INTEGER RANGE 0 TO 4095;
signal	ADC_CH7  : INTEGER RANGE 0 TO 4095;


begin
cs_n 	  <=  CS_BUF;

ch0 <= to_unsigned(ADC_CH0, 12);
ch1 <= to_unsigned(ADC_CH1, 12);
ch2 <= to_unsigned(ADC_CH4, 12);
ch3 <= to_unsigned(ADC_CH3, 12);
ch4 <= to_unsigned(ADC_CH2, 12);
ch5 <= to_unsigned(ADC_CH5, 12);
ch6 <= to_unsigned(ADC_CH6, 12);
ch7 <= to_unsigned(ADC_CH7, 12);

--- SCLK Selection ---

with NEXT_STATE select
   sclk    <=  not oCLK  when ACQUIRE,
		'1'       when others;	
    
---------------------------------------------------------
--- 	         2.5MHz Clock for ADC 		      ---
---------------------------------------------------------
				 
CLK_DELAY : process(CLK)

	variable CLK_DELAY_COUNT : INTEGER range 0 to 20;
	
begin
	if(rising_edge(CLK)) then
		
		if(CLK_DELAY_COUNT  = 20)
		then
			oCLK <= not oCLK;
			CLK_DELAY_COUNT := 0;
		else
			CLK_DELAY_COUNT := CLK_DELAY_COUNT + 1;
		end if;
		
	end if;
		
end process CLK_DELAY;				 


---------------------------------------------------------				 
--- 	          ADC State Machine 	              ---
---------------------------------------------------------
				 
ADC_FSM : process( oCLK )
	
	variable  DATA_COUNT   : INTEGER RANGE 0 TO 15;
	variable  ADC_CH       : INTEGER RANGE 0 TO 7;
	variable  ADC_DATA_BUF : INTEGER RANGE 0 TO 4095;
	variable  iCH 	       : STD_LOGIC_VECTOR(2 DOWNTO 0);
	variable  ADC_DATA     : STD_LOGIC_VECTOR(11 DOWNTO 0);
	
begin

	if(rising_edge( oCLK )) 
	then
		
		case (NEXT_STATE) is
				
			--- ADC Channel Selection ---
		
			when ADC_SEL =>  

				iCH := STD_LOGIC_VECTOR(TO_UNSIGNED( ADC_CH, 3 ));

				if(ADC_CH = 7) then
					ADC_CH := 0;  TEST_OUT_BUF <= not TEST_OUT_BUF;
				else 
					ADC_CH := ADC_CH + 1;
				end if;

				NEXT_STATE <= TRIGGER;
				
		
			--- Trigger ADC Conversion ---
		
			when TRIGGER  =>
			
				if(CS_BUF = '0') then
					CS_BUF     <= '1';
				   NEXT_STATE <= TRIGGER;
				else 
					CS_BUF     <= '0';
					NEXT_STATE <= ACQUIRE;
				end if;	
				
				
			--- Sample ADC Data ---	
		
			when ACQUIRE  =>
		
				case (DATA_COUNT) is 
					
					when 1   	=>  	mosi         <=  iCH(2);
					when 2   	=>  	mosi  	     <=  iCH(1);
					when 3   	=>  	mosi  	     <=  iCH(0);  
					when 4		=>		ADC_DATA(11) :=  miso;
					when 5   	=>  	ADC_DATA(10) :=  miso;
					when 6   	=>  	ADC_DATA(9)  :=  miso;
					when 7   	=>  	ADC_DATA(8)  :=  miso;
					when 8   	=>  	ADC_DATA(7)  :=  miso;
					when 9   	=>  	ADC_DATA(6)  :=  miso;
					when 10   	=>  	ADC_DATA(5)  :=  miso;
					when 11  	=>  	ADC_DATA(4)  :=  miso;
					when 12  	=>  	ADC_DATA(3)  :=  miso;
					when 13  	=>  	ADC_DATA(2)  :=  miso;
					when 14  	=>  	ADC_DATA(1)  :=  miso;
					when 15  	=>  	ADC_DATA(0)  :=  miso;
					when others =>
					
			   end case;
		
		
				if(DATA_COUNT = 15) 
				then 
					DATA_COUNT   :=  0;
					NEXT_STATE   <=  STORE;
				else 
					DATA_COUNT   :=  DATA_COUNT + 1;
					NEXT_STATE   <=  ACQUIRE;
				end if;
					
			
			--- Copy ADC Data ---
	
			when STORE   =>
			
				ADC_DATA_BUF :=  to_integer(UNSIGNED(ADC_DATA));
				
				case (ADC_CH) is
					
					when 0  	=>  	ADC_CH6 <= ADC_DATA_BUF;
					when 1  	=>  	ADC_CH7 <= ADC_DATA_BUF;
					when 2 	=>  	ADC_CH0 <= ADC_DATA_BUF;
					when 3  	=>  	ADC_CH1 <= ADC_DATA_BUF;
					when 4  	=>  	ADC_CH2 <= ADC_DATA_BUF;
					when 5  	=>  	ADC_CH3 <= ADC_DATA_BUF;
					when 6  	=>  	ADC_CH4 <= ADC_DATA_BUF;
					when 7  	=>  	ADC_CH5 <= ADC_DATA_BUF;
					when others =>
					
				end case;
	
				NEXT_STATE <= ADC_SEL;
				
	
			--- Other Invalid cases ---	
				
			when others   =>  NEXT_STATE <= ADC_SEL;
				
		end case;
	end if;
end process ADC_FSM;
		

end architecture;



