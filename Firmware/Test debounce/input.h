#ifndef INPUT_H
#define INPUT_H

#define F_CPU 16000000UL

#include <stdint.h>
#include <stdbool.h>

// -------------------------------------------------------------
//  CONFIGURA PIN (modifica secondo collegamento hardware)
// -------------------------------------------------------------
#define DB_REG   (*(volatile uint8_t *)0x30)
//#define EVT_REG  _SFR_IO8(0x11)
//#define MASK_REG _SFR_IO8(0x12)
#define CLR_REG  (*(volatile uint8_t *)0x3D)

#define MASK_REG  (*(volatile uint8_t *)0x32)
#define EVT_REG   (*(volatile uint8_t *)0x31)


void debounce_init(uint8_t mask);

uint8_t debounce_get_state(void);

uint8_t debounce_get_events(void);

void debounce_clear(uint8_t mask);




// encoder movement: returns -1,0,+1 for rotation since last poll
int8_t encoder_get_delta(void);

#endif
