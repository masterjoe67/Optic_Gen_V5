#ifndef UI_H
#define UI_H
#include <stdint.h>
#include <stdbool.h>
#include "Peripheral/pwm_iface.h"

void ui_init(void);
void ui_splash(void);
void ui_update(void); // call frequently from main loop
uint32_t update_param_32(uint32_t param, uint32_t min, uint32_t max);
uint16_t update_param_16(uint16_t param, uint16_t min, uint16_t max);
uint8_t update_param_8(uint8_t param, uint8_t min, uint8_t max);


#endif
