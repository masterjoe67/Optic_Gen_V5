#include "pwm_iface.h"
#include <avr/io.h>   // o quello che usi tu

// ==========================================================================
//  MAPPATURA REGISTRI (sostituisci con i tuoi indirizzi reali)
// ==========================================================================

// Registro frequenza portante (32 bit)
#define REG_CARRIER_FREQ_LO   (*(volatile uint16_t*)0x1000)
#define REG_CARRIER_FREQ_HI   (*(volatile uint16_t*)0x1002)

// Registro frequenza modulazione (32 bit)
#define REG_MOD_FREQ_LO       (*(volatile uint16_t*)0x1004)
#define REG_MOD_FREQ_HI       (*(volatile uint16_t*)0x1006)

// Registro deadtime in ns (32 bit)
#define REG_DEADTIME_LO       (*(volatile uint16_t*)0x1008)
#define REG_DEADTIME_HI       (*(volatile uint16_t*)0x100A)

// Registro modalità (8 bit)
#define REG_MODE              (*(volatile uint8_t*)0x100C)

// Registro output on/off (8 bit)
#define REG_OUTPUT_ENABLE     (*(volatile uint8_t*)0x100D)

// Registro encoder (32 bit)
#define REG_ENCODER_LO        (*(volatile uint16_t*)0x1010)
#define REG_ENCODER_HI        (*(volatile uint16_t*)0x1012)


// ==========================================================================
//  FUNZIONI DI UTILITÀ PER SCRIVERE 32 BIT
// ==========================================================================

static inline void write32(volatile uint16_t *lo, volatile uint16_t *hi, uint32_t v)
{
    *lo = (uint16_t)(v & 0xFFFF);
    *hi = (uint16_t)(v >> 16);
}

static inline uint32_t read32(volatile uint16_t *lo, volatile uint16_t *hi)
{
    return ((uint32_t)(*hi) << 16) | (*lo);
}


// ==========================================================================
//  FUNZIONI API
// ==========================================================================

void pwm_set_carrier_frequency(uint32_t hz)
{
    write32(&REG_CARRIER_FREQ_LO, &REG_CARRIER_FREQ_HI, hz);

    // aggiorno anche posizione encoder per mantenere coerenza
    write32(&REG_ENCODER_LO, &REG_ENCODER_HI, hz);
}

void pwm_set_modulation_frequency(uint32_t hz)
{
    write32(&REG_MOD_FREQ_LO, &REG_MOD_FREQ_HI, hz);
    write32(&REG_ENCODER_LO, &REG_ENCODER_HI, hz);
}

void pwm_set_deadtime_ns(uint32_t ns)
{
    write32(&REG_DEADTIME_LO, &REG_DEADTIME_HI, ns);
    write32(&REG_ENCODER_LO, &REG_ENCODER_HI, ns);
}

void pwm_set_mode(pwm_mode_t mode)
{
    REG_MODE = (uint8_t)mode;
}

void pwm_set_output(bool enable)
{
    REG_OUTPUT_ENABLE = enable ? 1 : 0;
}

// opzionale: leggere il valore aggiornato dall'encoder
uint32_t pwm_get_encoder_value(void)
{
    return read32(&REG_ENCODER_LO, &REG_ENCODER_HI);
}
