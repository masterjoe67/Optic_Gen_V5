#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>


#define BAUD 9600
#define UBRR_VALUE ((F_CPU/16/BAUD)-1)

volatile uint32_t overflow_count = 0;
volatile uint16_t div_1hz = 0;

void uart_tx(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

void uart_print(const char *s) {
    while (*s) uart_tx(*s++);
}

void uart_print_uint32(uint32_t v) {
    char buf[12];
    uint8_t i = 0;

    if (v == 0) {
        uart_tx('0');
        return;
    }
    while (v > 0) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }
    while (i--) uart_tx(buf[i]);
}

ISR(TIMER0_OVF_vect)
{
    overflow_count++;

    // divisore per generare 1 Hz
    div_1hz++;
    if (div_1hz >= 488) {         // circa 0.5s
        div_1hz = 0;
        PORTA ^= (1 << PA0);      // toggle visibile (1 Hz)
    }
}

int main(void)
{
    // UART0
    UBRR0H = (UBRR_VALUE >> 8);
    UBRR0L = (UBRR_VALUE & 0xFF);
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

    // PORTA output
    DDRA |= (1 << PA0);  // LED
    PORTA &= ~(1 << PA0);

    // Timer0 prescaler 64
    TCCR0 = (1 << CS01) | (1 << CS00);
    TCNT0 = 0;
    TIMSK |= (1 << TOIE0);

    sei();

    uart_print("START\r\n");

    uint32_t last_overflow = 0;

    while (1)
    {
        // stampa ogni ~1000 overflow (~1s)
        if (overflow_count - last_overflow >= 1000)
        {
            last_overflow = overflow_count;

            uart_print("Overflow count = ");
            uart_print_uint32(overflow_count);
            uart_print("\r\n");
        }
    }
}
