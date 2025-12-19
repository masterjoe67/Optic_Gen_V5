//#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "ili9341.h"
#include "Peripheral/input.h"
#include "Peripheral/uart.h"
#include "Peripheral/leds.h"
#include "Peripheral/pwm_iface.h"
#include "ui.h"


// ------------------------------------------------
// Main
// ------------------------------------------------
int main(void) {

    DDRB |= (1<<PB0)|(1<<PB1)|(1<<PB2); // CS / RESET / DC
    CS_HIGH();

    spi_init();
    uart_init(19200);
    uart_print("\r\nBoot AVR + ILI9341\r\n");

    uart_print("Inizializzo display...\r\n");
	ILI9341_Init();
	ILI9341_Set_Rotation(3);
    debounce_init(0x01);   // abilita tutti i 7 pulsanti
    
    leds_init();
 
    setTextFont(2);
    setTextColor(ILI9341_WHITE,ILI9341_BLACK);

    // apply initial PWM settings
    pwm_set_mode(MODE_3PHASE);
    pwm_set_carrier_hz(20000);
    pwm_set_mod_hz(50);
    pwm_set_deadtime_ns(20);
    pwm_enable(false);

    ui_splash();
    
    ui_init();


    uart_print("FINE.\r\n");


    while(1) {
        ui_update();
        // small delay to avoid busy spin
        _delay_ms(5);
    }

    return 0;
}
