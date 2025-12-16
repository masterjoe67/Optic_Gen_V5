

#define F_CPU 16000000
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