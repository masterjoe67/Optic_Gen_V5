//#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "ili9341.h"
#include "Peripheral/ext_register.h"

#include "Peripheral/input.h"
#include "Peripheral/uart.h"
#include "Peripheral/leds.h"
#include "Peripheral/pwm_iface.h"
#include "ui.h"



// LED Debug: su PORTA -------------------------
void leds(uint8_t v) {
    PORTA = v;
}









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

    ILI9341_Draw_Filled_Rectangle(120,40,294,28, 0xF800);
    
    leds_init();
    //input_init();
    
    
_delay_ms(2000);


    setTextFont(2);
    setTextColor(ILI9341_WHITE,ILI9341_BLACK);

    
   




    ui_splash();
    
    ui_init();
//uart_print("Debug-13\r\n");
    
    // apply initial PWM settings
    pwm_set_mode(MODE_HALF_BRIDGE);
    pwm_set_carrier_frequency(20000);
    pwm_set_modulation_frequency(50);
    pwm_set_deadtime_ns(20);
    pwm_set_output(false);



	
	/*setTextSize(2);
    fillScreen(ILI9341_BLACK);
    setTextColor(ILI9341_WHITE,ILI9341_BLACK);

    //ILI9341_set_cursor(100, 100);
    drawChar('B', 100, 100, 2);

    drawString("Dio anubi", 20, 20, 2);

    fillRect(200, 120, 100, 100, ILI9341_YELLOW);

    drawRoundRect(5, 5, 64, 32, 4, ILI9341_CYAN);
    _delay_ms(8000);*/
  

   
    uart_print("FINE.\r\n");


    while(1) {
        ui_update();
        // small delay to avoid busy spin
        _delay_ms(5);
    }

    return 0;
}
