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

#define MMIO_B0         _SFR_IO8(0x00)
#define MMIO_B1         _SFR_IO8(0x01)
#define MMIO_B2         _SFR_IO8(0x02)
#define MMIO_B3         _SFR_IO8(0x03)
#define MMIO_COMMIT     _SFR_IO8(0x04)
#define MMIO_RSEL       _SFR_IO8(0x05)

#define CTRL        _SFR_IO8(0x06)
#define MODE        _SFR_IO8(0x07)




#define PWM_F_CLK 50000000UL  // 50 MHz
#define CLK_PERIOD_NS 20UL
#define MOD_LUT_SIZE  2048UL
#define MOD_MAX       0xFFFF

typedef enum {
    MODE_HALF_BRIDGE = 0,
    MODE_FULL_BRIDGE,
    MODE_3PHASE,
} pwm_mode_t;

void write_reg32(uint8_t commit_sel, uint32_t value);
uint32_t read_reg32(uint8_t sel);
void write_reg16(uint8_t commit_sel, uint32_t value);
uint16_t read_reg16(uint8_t sel);


void pwm_enable(bool en);
void pwm_set_mode(uint8_t mode);
void pwm_set_carrier_hz(uint32_t hz);
void pwm_set_mod_hz(uint16_t hz);
void pwm_set_deadtime_ns(uint16_t ns);
void pwm_set_magnitude(uint8_t v);

uint32_t pwm_get_carrier_hz(void);
uint32_t pwm_get_mod_hz(void);
uint16_t pwm_get_deadtime_ns(void);
uint16_t pwm_get_magnitude(void);

bool pwm_is_enabled(void);
uint8_t pwm_get_mode(void);


#endif
