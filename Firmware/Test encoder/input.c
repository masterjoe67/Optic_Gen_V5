#include "input.h"
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>

void debounce_init(uint8_t mask) {
    MASK_REG = mask & 0x7F;
    (void)EVT_REG; // clear pending
}

uint8_t debounce_get_state(void) {
    return DB_REG & 0x7F;
}

uint8_t debounce_get_events(void) {
    return EVT_REG & 0x7F; // read clears pending events
}

void debounce_clear(uint8_t mask) {
    CLR_REG = mask & 0x7F;
}

int8_t encoder_get_delta(){
    static uint8_t prev = 0;
    uint8_t cur = ENC_VAL_L;   // macro o funzione MMIO

    int8_t delta = (int8_t)(cur - prev);

    prev = cur;
    return delta;
}

uint16_t encoder_read(void) {
    uint16_t val = ((uint16_t)ENC_VAL_H << 8) | ENC_VAL_L;
    return val;
}

