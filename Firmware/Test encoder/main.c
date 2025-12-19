#include <stdint.h>
#include <stdbool.h>
#include <util/delay.h>
#include "ili9341.h"
//#include "glcdfont.h"
#include "uart.h"

#include "pwm_iface.h"
#include "input.h"

// ==========================================
// helper per stampare valori su UART
// ==========================================
static void uart_print_carrier(uint32_t hz)
{
    uart_print("Carrier: ");
    if(hz > 0xFFFF)
    {
        uart_print_hex16((uint16_t)(hz >> 16));
        uart_print_hex16((uint16_t)(hz & 0xFFFF));
    }
    else
        uart_print_hex16((uint16_t)hz);
    uart_print("\r\n");
}

static void uart_print_mod(uint32_t hz)
{
    uart_print("Modulation: ");
    uart_print_hex16((uint16_t)hz);
    uart_print("\r\n");
}

static void uart_print_dead(uint32_t ns)
{
    uart_print("Deadtime: ");
    uart_print_hex16((uint16_t)ns);
    uart_print("\r\n");
}



int main(void)
{
    // inizializza PWM disattivato
    pwm_enable(false);
	uart_init(19200);
    uart_print("\r\nBoot AVR + ILI9341\r\n");

    // valori iniziali
    uint32_t carrier_hz = 20000;   // 20 kHz
    uint32_t mod_hz     = 110;   // 10 kHz
    uint32_t dead_ns    = 200;     // 200 ns

    pwm_set_carrier_hz(carrier_hz);
    //pwm_set_mod_hz(mod_hz);
    pwm_set_deadtime_ns(dead_ns);
	pwm_set_mod_hz(mod_hz);
    pwm_enable(false);

    uart_print("Modulation Test start\r\n");
    uart_print_mod(pwm_get_mod());

    while(1)
    {
        int8_t delta = encoder_get_delta();

        if(delta != 0)
        {
            // l'encoder varia la frequenza di modulazione
            if(delta > 0) mod_hz += 1;  // step 100 Hz
            else           mod_hz -= 1;

            // limiti min/max
            if(mod_hz < 1)  mod_hz = 1;
            if(mod_hz > 500) mod_hz = 500;

            pwm_set_mod_hz(mod_hz);

			uart_print("MOD_0= ");
			uart_print_hex16(pwm_get_mod());
			uart_print("\r\n");

			
            uart_print_mod(pwm_get_mod_hz());
        }
    }

    return 0;
}
