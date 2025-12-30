#include "pwm_iface.h"
#include <avr/io.h>   // o quello che usi tu
#include <util/delay.h>


// ==========================================================================
//  FUNZIONI DI UTILITÀ PER SCRIVERE 32 BIT
// ==========================================================================

void write32(volatile uint16_t *lo, volatile uint16_t *hi, uint32_t v)
{
    *lo = (uint16_t)(v & 0xFFFF);
    *hi = (uint16_t)(v >> 16);
}

uint32_t read32(volatile uint16_t *lo, volatile uint16_t *hi)
{
    return ((uint32_t)(*hi) << 16) | (*lo);
}


// ==========================================================================
//  FUNZIONI API
// ==========================================================================


// =====================================================
// Controllo abilitazione
// =====================================================
void pwm_enable(bool en)
{
    CTRL = en ? 1 : 0;
}

bool pwm_is_enabled(void)
{
    return (CTRL & 0x01) != 0;
}

// =====================================================
// Modalità: 00 trifase, 01 mono mezzo ponte, 10 mono ponte intero
// =====================================================
void pwm_set_mode(uint8_t mode)
{
    MODE = mode & 0x03;
}

uint8_t pwm_get_mode(void)
{
    return MODE & 0x03;
}



// =====================================
// Imposta frequenza carrier in Hz
// =====================================
void pwm_set_carrier_hz(uint32_t hz)
{
    //uint32_t inc_car = round(hz * 4294967296.0 / 50000000.0);
    //uint32_t inc_car = hz * 2^32 / 100000000;
    //write_reg32(0, inc_car);
    write_reg32(0, hz / 2);
}

// =====================================
// Legge frequenza carrier in Hz
// =====================================
uint32_t pwm_get_carrier_hz(void)
{
    /*uint32_t reg = pwm_get_carrier();
    if(reg == 0) reg = 1; // evita divisione per zero
    return PWM_F_CLK / (reg + 1);*/
    //uint32_t value = read_reg32(0);
    //return (uint32_t)((uint64_t)value * 50000000 / 4294967296);
    return read_reg32(0) * 2;
}

// =====================================
// Imposta frequenza modulazione in Hz
// =====================================
void pwm_set_mod_hz(uint16_t hz)
{
    //uint32_t inc_mod = round(hz * 4294967296.0 / 50000000.0);
   // uint32_t inc_mod = hz * 2^32 / 100000000;
    write_reg32(1, hz / 2);
}

// =====================================
// Legge frequenza modulazione in Hz
// =====================================
uint32_t pwm_get_mod_hz(void)
{
    //uint16_t mod = pwm_get_mod();
    //return (uint16_t)(PWM_F_CLK / (MOD_LUT_SIZE * (uint32_t)(mod + 1)));
    //uint32_t value = read_reg32(1);
    //return (uint32_t)((uint64_t)value * 50000000 / 4294967296);
    return read_reg32(1) * 2;
}

// =====================================
// Imposta deadtime in nanosecondi
// =====================================
void pwm_set_deadtime_ns(uint16_t ns)
{
    float tick = 1 / PWM_F_CLK;
    uint16_t dt_cycles = ns / tick;
    //uint16_t dt_cycles = round( ns / 20 );
    write_reg16(2, ns * 5);
}

// =====================================
// Legge deadtime in nanosecondi
// =====================================
uint16_t pwm_get_deadtime_ns(void)
{
    //uint16_t dt = read_reg16();
    //return dt * 20;
    return read_reg16(2) / 5;
}

long map_long(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min)
           + out_min;
}

// =====================================
// Imposta magnitude in %
// =====================================
void pwm_set_magnitude(uint8_t v)
{
    uint16_t val = map_long(v, 0, 100, 0, 1023);
    write_reg16(3, val);
 
}

// =====================================
// Legge magnitude in %
// =====================================
uint16_t pwm_get_magnitude(void)
{

    return map_long(read_reg16(3), 0, 1023, 0, 100);
}

void write_reg32(uint8_t commit_sel, uint32_t value)
{
    MMIO_B0 = (value >> 0)  & 0xFF;
    MMIO_B1 = (value >> 8)  & 0xFF;
    MMIO_B2 = (value >> 16) & 0xFF;
    MMIO_B3 = (value >> 24) & 0xFF;
    _delay_ms(100);
    MMIO_COMMIT = commit_sel;
}

uint32_t read_reg32(uint8_t sel)
{
    MMIO_RSEL = sel;
    _delay_ms(100);
    uint32_t v = 0;
    v |= (uint32_t)MMIO_B0 << 0;
    v |= (uint32_t)MMIO_B1 << 8;
    v |= (uint32_t)MMIO_B2 << 16;
    v |= (uint32_t)MMIO_B3 << 24;

    return v;
}

void write_reg16(uint8_t commit_sel, uint32_t value)
{
MMIO_B0 = value & 0xFF;
MMIO_B1 = (value >> 8) & 0xFF;
_delay_ms(100);
MMIO_COMMIT = commit_sel;
}

uint16_t read_reg16(uint8_t sel)
{
    MMIO_RSEL = sel;
    _delay_ms(100);
    uint16_t v = 0;
    v |= (uint32_t)MMIO_B0 << 0;
    v |= (uint32_t)MMIO_B1 << 8;

    return v;
}