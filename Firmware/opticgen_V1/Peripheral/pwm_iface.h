#ifndef PWM_IFACE_H
#define PWM_IFACE_H
#include <stdint.h>
#include <stdbool.h>

#define R0_ADDR_LOW   0x00
#define R0_ADDR_HIGH  0x01
#define R1_ADDR_LOW   0x02
#define R1_ADDR_HIGH  0x03
#define R2_ADDR_LOW   0x04
#define R2_ADDR_HIGH  0x05
#define R3_ADDR_LOW   0x06
#define R3_ADDR_HIGH  0x07

#define CARRIER_0   _SFR_IO8(0x00)
#define CARRIER_1   _SFR_IO8(0x01)
#define CARRIER_2   _SFR_IO8(0x02)
#define CARRIER_3   _SFR_IO8(0x03)
#define MOD_0       _SFR_IO8(0x04)
#define MOD_1       _SFR_IO8(0x05)
#define DEADTIME    _SFR_IO8(0x06)
#define CTRL        _SFR_IO8(0x07)
#define MODE        _SFR_IO8(0x1e)
#define STATUS      _SFR_IO8(0x1f)

#define PWM_CARRIER _SFR_DWORD(0x00)

#define PWM_MOD     _SFR_IO16(0x04)

#define PWM_F_CLK 50000000UL  // 50 MHz
#define CLK_PERIOD_NS 20UL
#define MOD_LUT_SIZE  2048UL
#define MOD_MAX       0xFFFF

typedef enum {
    MODE_HALF_BRIDGE = 0,
    MODE_FULL_BRIDGE,
    MODE_3PHASE,
} pwm_mode_t;

void pwm_set_carrier(uint32_t value);
void pwm_set_mod(uint16_t value);
void pwm_set_deadtime(uint8_t value);
void pwm_enable(bool en);
void pwm_set_mode(uint8_t mode);
void pwm_set_carrier_hz(uint32_t hz);
void pwm_set_mod_hz(uint16_t hz);
void pwm_set_deadtime_ns(uint32_t ns);

uint32_t pwm_get_carrier_hz(void);
uint16_t pwm_get_mod_hz(void);
uint32_t pwm_get_deadtime_ns(void);

uint32_t pwm_get_carrier(void);
uint16_t pwm_get_mod(void);
uint8_t pwm_get_deadtime(void);
bool pwm_is_enabled(void);
uint8_t pwm_get_mode(void);


#endif
