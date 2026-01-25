#ifndef UI_H
#define UI_H
#include <stdint.h>
#include <stdbool.h>
#include "Peripheral/pwm_iface.h"

void ui_init(void);
void ui_splash(void);
void ui_update(void); // call frequently from main loop



#endif
