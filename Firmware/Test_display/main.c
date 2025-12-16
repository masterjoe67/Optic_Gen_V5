#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include"ili9341.h"
#include "ext_register.h"
// UART ---------------------------------------
#define BAUD 9600
#define MY_UBRR (F_CPU/16/BAUD-1)
void uart_init(void) {
      // Imposta baud rate
    UBRR0H = (MY_UBRR >> 8);
    UBRR0L = MY_UBRR & 0xFF;
    
    // Abilita TX e RX
    UCSR0B = (1<<TXEN0) | (1<<RXEN0);
    
    // Formato frame: 8N1
    UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);
}

void uart_tx(char c) {
    while(!(UCSR0A & (1<<UDRE0)));
    UDR0 = c;
}

void uart_print(const char *s) {
    while(*s) uart_tx(*s++);
}

void uart_print_hex16(uint16_t v)
{
    const char hex[] = "0123456789ABCDEF";

    uart_putc(hex[(v >> 12) & 0x0F]);  // nibble alto del byte alto
    uart_putc(hex[(v >> 8)  & 0x0F]);  // nibble basso del byte alto
    uart_putc(hex[(v >> 4)  & 0x0F]);  // nibble alto del byte basso
    uart_putc(hex[v & 0x0F]);          // nibble basso del byte basso
}


// LED Debug: su PORTA -------------------------
void leds(uint8_t v) {
    PORTA = v;
}

// SPI -----------------------------------------
void spi_init(void) {
    // MOSI (PB2), SCK (PB1), SS (PB4) come output
    DDRB |= (1<<PB2)|(1<<PB1)|(1<<PB4);

    // SPI: Master, clk/2, mode 0
    SPCR = (1<<SPE)|(1<<MSTR);
    SPSR = (1<<SPI2X);
}





void ILI9341_Test_All(void)
{
 
    ILI9341_Fill_Screen(ILI9341_BLACK);
    //_delay_ms(2000);

    ILI9341_Fill_Screen(ILI9341_BLACK);
    ILI9341_Draw_Filled_Rectangle(ILI9341_RED, 20, 20, 100, 100);
    ILI9341_Draw_Filled_Rectangle(ILI9341_GREEN, 120, 20, 200, 100);
    ILI9341_Draw_Filled_Rectangle(ILI9341_BLUE, 220, 20, 300, 100);

    ILI9341_Draw_Filled_Rectangle(ILI9341_CYAN, 20, 100, 100, 180);
    ILI9341_Draw_Filled_Rectangle(ILI9341_WHITE, 120, 100, 200, 180);
    ILI9341_Draw_Filled_Rectangle(ILI9341_YELLOW, 220, 100, 300, 180);
    _delay_ms(2000);


    // ============================
    // 1) Test Pixel
    // ============================
    for (int i = 0; i < 200; i++)
        ILI9341_Draw_Pixel(i, i, ILI9341_RED);
    _delay_ms(2000);

    // ============================
    // 2) Test Linee
    // ============================
    ILI9341_Fill_Screen(ILI9341_BLACK);

    ILI9341_Draw_Line(ILI9341_RED,   0, 0, 319, 0);     // alto
    ILI9341_Draw_Line(ILI9341_GREEN, 0, 239, 319, 239); // basso
    ILI9341_Draw_Line(ILI9341_BLUE,  0, 0, 0, 239);     // sinistra
    ILI9341_Draw_Line(ILI9341_WHITE, 319,0,319,239);    // destra
    ILI9341_Draw_Line(ILI9341_CYAN,  0, 0, 319,239);    // diagonale
    _delay_ms(2000);

    // ============================
    // 3) Rettangolo pieno
    // ============================
    ILI9341_Fill_Screen(ILI9341_BLACK);
    ILI9341_Draw_Filled_Rectangle(ILI9341_RED, 20, 20, 100, 100);
    _delay_ms(500);

    // ============================
    // 4) Rettangolo vuoto
    // ============================
    ILI9341_Draw_Empty_Rectangle(ILI9341_GREEN, 10, 10, 150, 150);
    _delay_ms(500);

    // ============================
    // 5) Cerchi
    // ============================
    ILI9341_Fill_Screen(ILI9341_BLACK);

    ILI9341_Draw_Circle(120, 160, 30, ILI9341_YELLOW, 0); // bordato
    ILI9341_Draw_Circle(120, 160, 20, ILI9341_BLUE,   1); // pieno
    _delay_ms(800);

    // ============================
    // 6) Test singoli caratteri
    // ============================
    ILI9341_Fill_Screen(ILI9341_BLACK);

    ILI9341_Draw_Char(10, 10, ILI9341_WHITE, ILI9341_BLACK, 'A', 1);
    ILI9341_Draw_Char(30, 10, ILI9341_GREEN, ILI9341_BLACK, 'B', 2);
    ILI9341_Draw_Char(60, 10, ILI9341_RED,   ILI9341_BLACK, 'C', 3);
    _delay_ms(1000);

    // ============================
    // 7) Stringhe
    // ============================
    ILI9341_Fill_Screen(ILI9341_BLACK);

    ILI9341_Draw_String(10, 30, ILI9341_WHITE, ILI9341_BLACK, "Test Display ILI9341", 1);
    ILI9341_Draw_String(10, 60, ILI9341_YELLOW, ILI9341_BLACK, "Font size 2", 2);
    ILI9341_Draw_String(10,110, ILI9341_CYAN,   ILI9341_BLACK, "Font size 3", 3);

    _delay_ms(1500);

    // ============================
    // 8) Test combinato
    // ============================
    ILI9341_Fill_Screen(ILI9341_BLACK);

    for (int r = 5; r < 150; r += 10)
        //ILI9341_Draw_Circle(120,160,r, ILI9341_RGB565(r, 255-r, 128), 0);

    for (int i = 0; i < 240; i += 20)
        ILI9341_Draw_Line(ILI9341_MAGENTA, i, 0, 239-i, 319);

    _delay_ms(1500);

    // Fine test
    ILI9341_Fill_Screen(ILI9341_BLACK);
    ILI9341_Draw_String(20,150, ILI9341_GREEN, ILI9341_BLACK,
                        "TEST COMPLETATO", 2);
}

// ------------------------------------------------
// Main
// ------------------------------------------------
int main(void) {

    DDRA = 0xFF;    // LED debug
    PORTA = 0;

    DDRB |= (1<<PB0)|(1<<PB1)|(1<<PB2); // CS / RESET / DC
    CS_HIGH();

    uart_init();
    uart_print("\r\nBoot AVR + ILI9341\r\n");

    leds(0x20);
    spi_init();

    // Leggo SPCR e SPSR
    char buf[64];
    sprintf(buf, "SPCR=%02X  SPSR=%02X\r\n", SPCR, SPSR);
    uart_print(buf);

    uart_print("Inizializzo display...\r\n");
    //ili_init();
	ILI9341_Init();
	ILI9341_Set_Rotation(3);
	
	ILI9341_Fill_Screen(ILI9341_BLACK);
/*ILI9341_Draw_String(0, 30, ILI9341_WHITE, ILI9341_BLACK, "Test Display ILI9341", 1);
ILI9341_Draw_String(10, 50, ILI9341_WHITE, ILI9341_BLACK, "Test Display ILI9341", 1);
ILI9341_Draw_String(10, 70, ILI9341_GREEN, ILI9341_BLACK, "DIO PORCO", 1);*/

//ILI9341_Draw_String(10, 100, ILI9341_RED, ILI9341_BLACK, "MADONNA Vacca", 2);

/*ILI9341_Fill_Screen(ILI9341_RGB565(255,0,0));   // rosso
_delay_ms(500);
ILI9341_Fill_Screen(ILI9341_RGB565(0,255,0));   // verde
_delay_ms(500);
ILI9341_Fill_Screen(ILI9341_RGB565(0,0,255));   // blu*/



   

    REG0L = 0xab;
     
    //r0_l_val = REG0L;
    //r0_h_val = REG0H;

    uint16_t r0_val = REG0L;      // byte basso
    r0_val |= REG0H << 8; // byte alto
    uint16_t r1_val = REG1L;      // byte basso
    r1_val |= REG1H << 8; // byte alto
    uint16_t r2_val = REG2L;      // byte basso
    r2_val |= REG2H << 8; // byte alto
    uint16_t r3_val = REG3L;      // byte basso
    r3_val |= REG3H << 8; // byte alto

    uart_print("r0_val ");
    uart_print(": 0x");
    uart_print_hex16(r0_val);
    uart_print("\r\n");

    uart_print("r1_val ");
    uart_print(": 0x");
    uart_print_hex16(r1_val);
    uart_print("\r\n");

    uart_print("r2_val ");
    uart_print(": 0x");
    uart_print_hex16(r2_val);
    uart_print("\r\n");

    uart_print("r3_val ");
    uart_print(": 0x");
    uart_print_hex16(r3_val);
    uart_print("\r\n");

    _delay_ms(800);

    REG3L = 0x01;

    uint16_t reg0 = REG0;
    uint16_t reg1 = REG1;
    uint16_t reg2 = REG2;
    uint16_t reg3 = REG3;

    unsigned int color = 0xFFFF; // bianco
    unsigned char size = 1;

    sprintf(buf, "REG0 = %04X", reg0);

    //sprintf(buf, "R0 = %04X", r0_val);

    ILI9341_Draw_String(0, 0, color, 0, buf, size);

    sprintf(buf, "R1 = %04X", reg1);
    ILI9341_Draw_String(0, 16, color, 0, buf, size);

    sprintf(buf, "R2 = %04X", reg2);
    ILI9341_Draw_String(0, 32, color, 0, buf, size);

    sprintf(buf, "R3 = %04X", reg3);
    ILI9341_Draw_String(0, 48, color, 0, buf, size);

    ILI9341_Draw_String(10, 100, ILI9341_RED, ILI9341_BLACK, "MADONNA Vacca", 2);
    uart_print("FINE.\r\n");

    leds(0x01);

    while(1);
}
