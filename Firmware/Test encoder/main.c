#include <stdint.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>
//#include "glcdfont.h"
#include "uart.h"
#include <avr/io.h>


/* ================= IRQ DEBUG ================= */

volatile uint8_t irq_id = 0xFF;

/* ---- External interrupts INT0–INT7 ---- */
ISR(INT0_vect) { irq_id = 0x00; }
ISR(INT1_vect) { irq_id = 0x01; }
ISR(INT2_vect) { irq_id = 0x02; }
ISR(INT3_vect) { irq_id = 0x03; }
ISR(INT4_vect) { irq_id = 0x04; }
ISR(INT5_vect) { irq_id = 0x05; }
ISR(INT6_vect) { irq_id = 0x06; }
ISR(INT7_vect) { irq_id = 0x07; }

/* ---- Timers ---- */
ISR(TIMER0_OVF_vect) { irq_id = 0x10; }
ISR(TIMER1_OVF_vect) { irq_id = 0x11; }
ISR(TIMER2_OVF_vect) { irq_id = 0x12; }
ISR(TIMER3_OVF_vect) { irq_id = 0x13; }

/* ---- UART / ADC ---- */
ISR(USART0_RX_vect) { irq_id = 0x20; }
ISR(ADC_vect)       { irq_id = 0x21; }

/* ---- Catch-all ---- */
ISR(BADISR_vect)    { irq_id = 0xEE; }



void uart_puts(const char *s)
{
    while (*s)
        uart_putc(*s++);
}

void uart_puthex(uint8_t v)
{
    const char hex[] = "0123456789ABCDEF";
    uart_putc(hex[v >> 4]);
    uart_putc(hex[v & 0x0F]);
}

/* ================= MAIN ================= */

int main(void)
{
    uart_init(19200);

    uart_puts("\r\n=== ATmega128 IRQ TEST ===\r\n");

    /* 🔒 DISABILITA TUTTO */
    EIMSK  = 0;
    TIMSK  = 0;
    ETIMSK = 0;
    UCSR0B &= ~(1 << RXCIE0);
    ADCSRA &= ~(1 << ADIE);

    /* 🧪 ABILITA SOLO UN IRQ PER VOLTA */
    EICRA = (1 << ISC01);   // INT0 falling edge
    EICRB = 0;
    EIMSK = (1 << INT0);    // INT0 ON

    sei();

    while (1) {
        if (irq_id != 0xFF) {

            uart_puts("IRQ: 0x");
            uart_puthex(irq_id);
            uart_puts("\r\n");

            irq_id = 0xFF;
        }
    }
}