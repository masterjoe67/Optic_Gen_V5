#ifndef PWM_IFACE_H
#define PWM_IFACE_H
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MODE_HALF_BRIDGE = 0,
    MODE_FULL_BRIDGE,
    MODE_3PHASE,
} pwm_mode_t;

void pwm_set_carrier_frequency(uint32_t hz);
void pwm_set_modulation_frequency(uint32_t hz);
void pwm_set_deadtime_ns(uint32_t ns);
void pwm_set_mode(pwm_mode_t mode);
void pwm_set_output(bool enable);

#endif
