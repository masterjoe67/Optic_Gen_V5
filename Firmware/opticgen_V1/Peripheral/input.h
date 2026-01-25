#ifndef INPUT_H
#define INPUT_H

//#define F_CPU 16000000UL

#include <stdint.h>
#include <stdbool.h>

// -------------------------------------------------------------
//  CONFIGURA PIN (modifica secondo collegamento hardware)
// -------------------------------------------------------------
#define DB_REG   (*(volatile uint8_t *)0x30)
#define CLR_REG  (*(volatile uint8_t *)0x28)
#define MASK_REG  (*(volatile uint8_t *)0x32)
#define EVT_REG   (*(volatile uint8_t *)0x31)

#define ENC_VAL_L   (*(volatile uint8_t *)0x3c)
#define ENC_VAL_H  (*(volatile uint8_t *)0x3D)


void debounce_init(uint8_t mask);

uint8_t debounce_get_state(void);

uint8_t debounce_get_events(void);

void debounce_clear(uint8_t mask);

uint16_t encoder_read(void);

uint32_t update_param_32(uint32_t param, uint32_t min, uint32_t max, uint32_t step);

uint16_t update_param_16(uint16_t param, uint16_t min, uint16_t max, uint16_t step);

uint8_t update_param_8(uint8_t param, uint8_t min, uint8_t max, uint8_t step);


// encoder movement: returns -1,0,+1 for rotation since last poll
int8_t encoder_get_delta(void);

#endif
