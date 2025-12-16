#include <stdint.h>
#include "ext_register.h"




// Scrive un registro a 16 bit sul bus MMIO
void write_mmio_16(uint8_t addr_low, uint8_t addr_high, uint16_t val) {
    *((volatile uint8_t *)addr_low)  = val & 0xFF;       // byte basso
    *((volatile uint8_t *)addr_high) = (val >> 8) & 0xFF; // byte alto
}

// Legge un registro a 16 bit dal bus MMIO
uint16_t read_mmio_16(uint8_t addr_low, uint8_t addr_high) {
    uint16_t val = *((volatile uint8_t *)addr_low);      // byte basso
    val |= ((uint16_t)(*((volatile uint8_t *)addr_high))) << 8; // byte alto
    return val;
}