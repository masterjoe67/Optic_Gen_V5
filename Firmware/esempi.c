#include <avr/io.h>
#include <util/delay.h>

// Definizione LED collegato al pin PA0
#define LED_PIN 0

// Funzione di inizializzazione PORTA
void porta_init(void) {
    // Configuro PA0 come output, gli altri come input
    DDRA = (1 << LED_PIN);  

    // Pulisco il valore iniziale su PORTA
    PORTA = 0x00;  
}

// Funzione per leggere un pin specifico di PORTA
uint8_t porta_read_pin(uint8_t pin) {
    return (PINA >> pin) & 0x01;
}

// Funzione per scrivere un valore su un pin specifico di PORTA
void porta_write_pin(uint8_t pin, uint8_t value) {
    if(value)
        PORTA |= (1 << pin);
    else
        PORTA &= ~(1 << pin);
}

// Funzione di ritardo software (circa 1 secondo per 16MHz)
void delay_1s(void) {
    for(volatile uint32_t i=0; i<1000000; i++);
}

int main(void) {
    porta_init();

    while(1) {
        // Lampeggio LED su PA0
        porta_write_pin(LED_PIN, 1);
        _delay_ms(500);
        porta_write_pin(LED_PIN, 0);
        _delay_ms(500);

        // Leggo il valore del pin PA1
        if(porta_read_pin(1)) {
            // Se PA1 è alto, accendo PA2
            porta_write_pin(2, 1);
        } else {
            porta_write_pin(2, 0);
        }
    }
}

//****************************************************************************************

#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 4000000UL  // 4 MHz clock, come nel core
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

void uart_send(char c) {
    while (!(UCSR0A & (1<<UDRE0))); // aspetta buffer libero
    UDR0 = c;
}

char uart_receive(void) {
    while (!(UCSR0A & (1<<RXC0))); // aspetta dato ricevuto
    return UDR0;
}

int main(void) {
    uart_init();
    
    while(1) {
        char c = uart_receive();  // ricevi carattere
        uart_send(c);             // rimanda indietro (echo)
    }
}

//**********************************************************************************************


#define F_CPU 16000000
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
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
    while (!(UCSR0A & (1<<UDRE0)));
    UDR0 = c;
}

void uart_print(const char *s)
{
    const char *p = s;  // evita modifiche a registri o stack instabile

    while (*p) {
        uart_tx(*p);
        p++;
    }
}

void uart_print_hex8(uint8_t v)
{
    const char hex[] = "0123456789ABCDEF";
    uart_tx(hex[(v >> 4) & 0xF]);
    uart_tx(hex[v & 0xF]);
}

void uart_print_hex16(uint16_t v)
{
    uart_print_hex8((v >> 8) & 0xFF);
    uart_print_hex8(v & 0xFF);
}


void uart_print_debug(const char *s)
{
    uart_print("DEBUG STRING:\n");

    // 1) Stampa indirizzo del puntatore
    uart_print("PTR = 0x");
    uart_print_hex16((uint16_t)s);
    uart_tx('\n');

    // 2) Stampa i primi 16 bytes in esadecimale
    uart_print("DATA = ");
    for(uint8_t i = 0; i < 16; i++)
    {
        uart_print_hex8((uint8_t)s[i]);
        uart_tx(' ');
    }
    uart_tx('\n');

    // 3) Cerca null-terminator
    uart_print("NULL @ index: ");
    int8_t idx = -1;

    for(uint8_t i = 0; i < 40; i++)
    {
        if(s[i] == 0) {
            idx = i;
            break;
        }
    }

    if(idx < 0) uart_print("NOT FOUND\n");
    else {
        uart_print_hex8(idx);
        uart_tx('\n');
    }
}


void ok()   { uart_print("OK\r\n"); }
void fail() { uart_print("FAIL\r\n"); }

uint8_t test_reg(volatile uint8_t *reg, uint8_t value) {
    *reg = value;
    _delay_ms(1);           // evita race o write-back
    uint8_t r = *reg;
    return (r == value);
}

int main(void) {

    uart_init();
	uart_tx('D');
	uart_tx('i');
	uart_tx('o');
	uart_tx(' ');
	char *x = "PORCO";

uart_tx(x[0]);
uart_tx(x[1]);
uart_tx(x[2]);
uart_tx(x[3]);
uart_tx(x[4]);

uart_print_debug("CIAO");


char t[] = "TEST";
uart_print_debug(t);
char z[4] = {'A','B','C','D'};  // NO '\0'
uart_print_debug(z);






    uart_print("TEST TIMER0 REGISTERS...\r\n");

    DDRB |= (1<<0); // LED su PB0

    uint8_t result = 1;

    uart_print("TCCR0 = 0x55 -> ");
    if (!test_reg(&TCCR0, 0x55)) { fail(); result = 0; } else ok();

    uart_print("TCCR0 = 0xAA -> ");
    if (!test_reg(&TCCR0, 0xAA)) { fail(); result = 0; } else ok();

    uart_print("OCR0 = 0x33 -> ");
    if (!test_reg(&OCR0, 0x33)) { fail(); result = 0; } else ok();

    uart_print("OCR0 = 0xF0 -> ");
    if (!test_reg(&OCR0, 0xF0)) { fail(); result = 0; } else ok();

    uart_print("TCNT0 = 0x00 -> ");
    if (!test_reg(&TCNT0, 0x00)) { fail(); result = 0; } else ok();

    uart_print("TCNT0 = 0xAB -> ");
    if (!test_reg(&TCNT0, 0xAB)) { fail(); result = 0; } else ok();

    if (result) {
        uart_print("TIMER0 REGISTERS OK!\r\n");
    } else {
        uart_print("TIMER0 REGISTER ERROR!\r\n");
    }

    // Loop: LED blink se tutto OK, fisso acceso se errore
    while (1) {
        if (result) {
            PORTB ^= (1<<0);
            _delay_ms(250);
        } else {
            PORTB |= (1<<0);
        }
    }
}

//********************************************************************



#define F_CPU 16000000
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define BAUD 9600
#define MY_UBRR (F_CPU/16/BAUD-1)
#define LED_PIN 0
void delay() {
    for (volatile uint32_t i = 0; i < 50000; i++);
}

void porta_init(void) {
    // Configuro PA0 come output, gli altri come input
    DDRA = (1 << LED_PIN);  

    // Pulisco il valore iniziale su PORTA
    PORTA = 0x00;  
}

// Funzione per scrivere un valore su un pin specifico di PORTA
void porta_write_pin(uint8_t pin, uint8_t value) {
    if(value)
        PORTA |= (1 << pin);
    else
        PORTA &= ~(1 << pin);
}

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
    while (!(UCSR0A & (1<<UDRE0)));
    UDR0 = c;
}


int main() {
    porta_init();
	
	uart_init();
	uart_tx('D');
	uart_tx('i');
	uart_tx('o');
	uart_tx(' ');
	char *x = "PORCO";

	uart_tx(x[0]);
	uart_tx(x[1]);
	uart_tx(x[2]);
	uart_tx(x[3]);
	uart_tx(x[4]);

    while (1) {
        porta_write_pin(LED_PIN, 1);
		uart_tx('D');
	uart_tx('i');
	uart_tx('o');
	uart_tx(' ');
        _delay_ms(500);
        porta_write_pin(LED_PIN, 0);
        _delay_ms(500);
    }
}


//****************************************************************************



#define F_CPU 16000000
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
// Definizione LED collegato al pin PA0
#define LED_PIN 0
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


// Funzione di inizializzazione PORTA
void porta_init(void) {
    // Configuro PA0 come output, gli altri come input
    DDRA = (1 << LED_PIN);  

    // Pulisco il valore iniziale su PORTA
    PORTA = 0x00;  
}

// Funzione per leggere un pin specifico di PORTA
uint8_t porta_read_pin(uint8_t pin) {
    return (PINA >> pin) & 0x01;
}

// Funzione per scrivere un valore su un pin specifico di PORTA
void porta_write_pin(uint8_t pin, uint8_t value) {
    if(value)
        PORTA |= (1 << pin);
    else
        PORTA &= ~(1 << pin);
}

// Funzione di ritardo software (circa 1 secondo per 16MHz)
void delay_1s(void) {
    for(volatile uint32_t i=0; i<1000000; i++);
}

void uart_tx(char c) {
    while (!(UCSR0A & (1<<UDRE0)));
    UDR0 = c;
}

void uart_print(const char *s)
{
    const char *p = s;  // evita modifiche a registri o stack instabile

    while (*p) {
        uart_tx(*p);
        p++;
    }
}

void uart0_putstr(char *string)
	{
		char c;
		while ((c = *string++)) uart_tx(c);
	}
const char message[] PROGMEM = "Hello";
const char *p_rom = message;

int main(void) {
    porta_init();
	uart_init();
	
	uart_tx('D');
	uart_tx('i');
	uart_tx('o');
	uart_tx(' ');
	
	char *x = "PORCO";
	
	for (uint16_t i = 0; i < sizeof(message)-1; i++) {
		char c = pgm_read_byte(p_rom + i);
    uart_tx(c);
}
	
    while(1) {
        // Lampeggio LED su PA0
        porta_write_pin(LED_PIN, 1);
		uart_tx(x[0]);
	uart_tx(x[1]);
	uart_tx(x[2]);
	uart_tx(x[3]);
	uart_tx(x[4]);
        _delay_ms(500);
        porta_write_pin(LED_PIN, 0);
        _delay_ms(500);

        
    }
}

//*************************************************************************


#define F_CPU 16000000
#include <avr/io.h>
#include <util/delay.h>

void uart0_init(void) {
    UBRR0H = 0;
    UBRR0L = 8;               // 115200 baud @ 16MHz
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart0_tx(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

void uart0_print(const char *s) {
    while (*s) uart0_tx(*s++);
}

void uart0_u32(uint32_t v) {
    char buf[12];
    uint8_t i = 0;
    if (v == 0) { uart0_tx('0'); return; }
    while (v > 0) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i--) uart0_tx(buf[i]);
}

int main(void) {
    uart0_init();
    uart0_print("Prescaler REAL test (Timer0)\r\n");

    // Timer0 → prescaler = 64
    TCCR0 = (1 << CS01) | (1 << CS00);   // clk/64

    // reset flags overflow
    TIFR = (1 << TOV0);

    uint32_t ovf_count = 0;

    // misura per 100 ms
    _delay_ms(10); // stabilizzazione
    uart0_print("Starting measurement...\r\n");

    // Set finestra di campionamento = 100 ms
    const uint8_t samples = 100;

    for (uint8_t t = 0; t < samples; t++) {
        _delay_ms(1);     // 1 ms esatto (in ATmega128 reale)
        if (TIFR & (1 << TOV0)) {
            ovf_count++;
            TIFR = (1 << TOV0); // clear
        }
    }

    uart0_print("Overflows in 100 ms = ");
    uart0_u32(ovf_count);
    uart0_print("\r\n");

    // Frequenza calcolata
    uint32_t f = ovf_count * 10; // in Hz
    uart0_print("Measured overflow freq ≈ ");
    uart0_u32(f);
    uart0_print(" Hz\r\n");

    while (1);
}


//******************************************************************************************

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include"ili9341.h"

// UART ---------------------------------------
void uart_init(void) {
    UBRR0H = 0;
    UBRR0L = 8;     // 115200 baud @ 16 MHz
    UCSR0B = (1<<TXEN0);
    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
}

void uart_tx(char c) {
    while(!(UCSR0A & (1<<UDRE0)));
    UDR0 = c;
}

void uart_print(const char *s) {
    while(*s) uart_tx(*s++);
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
    ILI9341_Fill_Screen(ILI9341_GREEN);
    _delay_ms(2000);
    ILI9341_Fill_Screen(ILI9341_BLACK);
    _delay_ms(2000);
    // ============================
    // 1) Test Pixel
    // ============================
    for (int i = 0; i < 200; i++)
        ILI9341_Draw_Pixel(i, i, ILI9341_YELLOW);
    _delay_ms(2000);

    // ============================
    // 2) Test Linee
    // ============================
    ILI9341_Fill_Screen(ILI9341_BLACK);

    ILI9341_Draw_Line(ILI9341_RED,   0, 0, 239, 0);     // alto
    ILI9341_Draw_Line(ILI9341_GREEN, 0, 319, 239, 319); // basso
    ILI9341_Draw_Line(ILI9341_BLUE,  0, 0, 0, 319);     // sinistra
    ILI9341_Draw_Line(ILI9341_WHITE, 239,0,239,319);    // destra
    ILI9341_Draw_Line(ILI9341_CYAN,  0, 0, 239,319);    // diagonale
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
        ILI9341_Draw_Circle(120,160,r, ILI9341_RGB565(r, 255-r, 128), 0);

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
	ILI9341_Set_Rotation(1);
	
	ILI9341_Fill_Screen(ILI9341_BLACK);

	ILI9341_Test_All();
	uart_print("Scrivo testo sul display...\r\n");
ILI9341_Draw_String(10, 30, ILI9341_WHITE, ILI9341_BLACK, "Test Display ILI9341", 1);


    uart_print("FINE.\r\n");

    leds(0xFF);

    while(1);
}
