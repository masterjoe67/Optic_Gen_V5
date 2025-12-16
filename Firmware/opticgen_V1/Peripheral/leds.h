// leds.h
#ifndef LEDS_H
#define LEDS_H
#include <stdbool.h>

#define led_carrier 0

void leds_init(void);
void leds_field_carrier_on(void);
void leds_field_carrier_off(void);
void leds_field_mod_on(void);
void leds_field_dead_on(void);
void leds_output_set(bool on);
#endif


