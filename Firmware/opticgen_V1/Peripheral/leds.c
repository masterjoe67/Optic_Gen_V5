
// leds.c (skeleton)
#include "leds.h"
#include <avr/io.h>
// map pins and implement set/clear
void leds_init(void) { DDRA = 0xFF;    
    PORTA = 0x80; 
}
void leds_field_carrier_on(void) { 
    PORTA |= (1 << led_carrier);
}

void leds_field_carrier_off(void) { 
    PORTA &= ~(1 << led_carrier);
}

void leds_field_mod_on(void) { /* set LED pin */ }
void leds_field_dead_on(void) { /* set LED pin */ }
void leds_output_set(bool on) { /* set/clear output LED */ }