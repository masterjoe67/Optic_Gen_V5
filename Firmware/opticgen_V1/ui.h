#ifndef UI_H
#define UI_H
#include <stdint.h>
#include <stdbool.h>
#include "Peripheral/pwm_iface.h"

void ui_init(void);
void ui_splash(void);
void ui_update(void); // call frequently from main loop
uint32_t update_param_carrier(uint32_t param);
uint16_t update_param_mod(uint16_t param);
uint8_t update_param_dead(uint8_t param);


#endif
