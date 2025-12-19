#include "pwm_iface.h"
#include <avr/io.h>   // o quello che usi tu



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
// Scrittura carrier 32bit
// =====================================================
void pwm_set_carrier(uint32_t value)
{

    CARRIER_0 = (uint8_t)(value & 0xFF);
    CARRIER_1 = (uint8_t)((value>>8) & 0xFF);
    CARRIER_2 = (uint8_t)((value>>16) & 0xFF);
    CARRIER_3 = (uint8_t)((value>>24) & 0xFF);

}

// Lettura carrier 32bit
uint32_t pwm_get_carrier(void)
{
    uint32_t val = 0;
    val |= CARRIER_0;
    val |= ((uint32_t)CARRIER_1) << 8;
    val |= ((uint32_t)CARRIER_2) << 16;
    val |= ((uint32_t)CARRIER_3) << 24;
    return val;
}

// =====================================================
// Scrittura modulazione 16bit
// =====================================================
void pwm_set_mod(uint16_t value)
{
    MOD_0 = (uint8_t)(value & 0xFF);
    MOD_1 = (uint8_t)((value>>8) & 0xFF);
    /*uart_print("MOD_0= ");
    uart_print_hex(MOD_0);
    uart_print("\r\n");
    uart_print("MOD_1= ");
    uart_print_hex(MOD_1);
	uart_print("\r\n");*/
}

// Lettura modulazione 16bit
uint16_t pwm_get_mod(void)
{
    uint16_t val = 0;
    val |= MOD_0;
    val |= ((uint16_t)MOD_1) << 8;
    return val;
}

// =====================================================
// Deadtime 8bit
// =====================================================
void pwm_set_deadtime(uint8_t value)
{
    DEADTIME = value;
}

uint8_t pwm_get_deadtime(void)
{
    return DEADTIME;
}

// =====================================================
// Controllo abilitazione
// =====================================================
void pwm_enable(bool en)
{
    CTRL = en ? 1 : 0;
}

bool pwm_is_enabled(void)
{
    return (STATUS & 0x01) != 0;
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
    if(hz == 0) hz = 1; // evita divisione per zero

    // calcolo registro divider
    uint32_t reg = (PWM_F_CLK / hz);
    if(reg > 0) reg -= 1;

    pwm_set_carrier(reg);
}

// =====================================
// Legge frequenza carrier in Hz
// =====================================
uint32_t pwm_get_carrier_hz(void)
{
    uint32_t reg = pwm_get_carrier();
    if(reg == 0) reg = 1; // evita divisione per zero
    return PWM_F_CLK / (reg + 1);
}

// =====================================
// Imposta frequenza modulazione in Hz
// =====================================
void pwm_set_mod_hz(uint16_t hz)
{
    if(hz < 1) hz = 1;

    uint16_t tmp = PWM_F_CLK / (MOD_LUT_SIZE * (uint16_t)hz);

    if(tmp == 0)
        tmp = 1;

    tmp -= 1;

    if(tmp > MOD_MAX)
        tmp = MOD_MAX;

    pwm_set_mod((uint16_t)tmp);
}

// =====================================
// Legge frequenza modulazione in Hz
// =====================================
uint16_t pwm_get_mod_hz(void)
{
    uint16_t mod = pwm_get_mod();
    return (uint16_t)(PWM_F_CLK / (MOD_LUT_SIZE * (uint32_t)(mod + 1)));
}

// =====================================
// Imposta deadtime in nanosecondi
// =====================================
void pwm_set_deadtime_ns(uint32_t ns)
{
    // converte ns in step di clock
    uint8_t dt = (uint8_t)((ns + (CLK_PERIOD_NS-1)) / CLK_PERIOD_NS); // round up
    if(dt > 31) dt = 31; // max valore 5 bit
    pwm_set_deadtime(dt);
}

// =====================================
// Legge deadtime in nanosecondi
// =====================================
uint32_t pwm_get_deadtime_ns(void)
{
    uint8_t dt = pwm_get_deadtime();
    return dt * CLK_PERIOD_NS;
}
