#include "input.h"
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>

void debounce_init(uint8_t mask) {
    MASK_REG = mask & 0xFF;
    (void)EVT_REG; // clear pending
}

uint8_t debounce_get_state(void) {
    return DB_REG & 0xFF;
}

uint8_t debounce_get_events(void) {
    return EVT_REG & 0xFF; // read clears pending events
}

void debounce_clear(uint8_t mask) {
    CLR_REG = mask & 0xFF;
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

static int16_t prev_det = 0;
int16_t prev_det_signed = 0;  // versione signed del detent

uint32_t update_param_32(uint32_t param, uint32_t min, uint32_t max, uint32_t step)
{
    int16_t pos = encoder_read();   // 0..1023

    // riduci a detent dividendo per 4
    int16_t det = pos;

    int16_t diff = det - prev_det;
    prev_det = det;

    if(diff == 0) return param;

    int32_t new_param = (int32_t)param + (int32_t)diff * step;

    if(new_param < min) new_param = min;
    if(new_param > max) new_param = max;

    return (uint32_t)new_param;
}



uint16_t update_param_16(uint16_t param, uint16_t min, uint16_t max, uint16_t step)
{
    int16_t pos = encoder_read();   // 0..1023

    // riduci a detent dividendo per 4
    int16_t det = pos;

    int16_t diff = det - prev_det;
    prev_det = det;

    if(diff == 0) return param;

    int16_t new_param = (int16_t)param + (int16_t)diff * step;

    if(new_param < min) new_param = min;
    if(new_param > max) new_param = max;

    return (uint16_t)new_param;
}



int16_t update_param_16_signed(int16_t param, int16_t min, int16_t max, int16_t step)
{
    int16_t pos = encoder_read();   // 0..1023

    // riduci a detent dividendo per 4 (o come vuoi)
    int16_t det = pos;

    int16_t diff = det - prev_det_signed;
    prev_det_signed = det;

    if(diff == 0) return param;

    int32_t new_param = (int32_t)param + (int32_t)diff * step; // usa int32_t per overflow temporaneo

    if(new_param < min) new_param = min;
    if(new_param > max) new_param = max;

    return (int16_t)new_param;
}


uint8_t update_param_8(uint8_t param, uint8_t min, uint8_t max, uint8_t step)
{
    int16_t pos = encoder_read();   // 0..1023

    // riduci a detent dividendo per 4
    int16_t det = pos;

    int16_t diff = det - prev_det;
    prev_det = det;

    if (diff > 0)
        diff = 1;
    else if (diff < 0)
        diff = -1;
    else
        return param;

    int8_t new_param = (int8_t)param + (int8_t)diff * step;

    if(new_param < min) new_param = min;
    if(new_param > max) new_param = max;

    return (uint8_t)new_param;
}