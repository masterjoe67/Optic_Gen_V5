#include <stdint.h>
#include <stdio.h>
#include "input.h" // header con definizione registri: DB_REG, EVT_REG, CLR_REG
#include "ili9341.h"
#include "uart.h"
#include <util/delay.h>

#define BUTTON_MASK 0x01 //0x7F  // 7 pulsanti

// inizializza debounce
void init_buttons(void) {
    debounce_init(BUTTON_MASK);
}

// funzione di visualizzazione semplice (console)
void display_buttons(uint8_t state) {
    printf("Buttons: ");
    for(int i=6; i>=0; i--) {
        printf("%c", (state & (1<<i)) ? '1' : '0');
    }
    printf("\r\n");
}

// ciclo principale
int main(void) {
	 DDRB |= (1<<PB0)|(1<<PB1)|(1<<PB2); // CS / RESET / DC
    CS_HIGH();

    spi_init();
    uart_init(19200);
    uart_print("\r\nBoot AVR + ILI9341\r\n");

	init_buttons();
    uint8_t btn_state, btn_events;
	DDRB |= (1<<PB0)|(1<<PB1)|(1<<PB2); // CS / RESET / DC
    CS_HIGH();

    spi_init();
    //debounce_init(0x7f);
	
	ILI9341_Init();
	ILI9341_Set_Rotation(3);

    fillScreen(0x0000); // black

	
	
	setTextFont(2);
    setTextSize(2);
    setTextColor(0xFFFF, 0x0000);
    ILI9341_set_cursor(0,100);
    //drawString("My Board Logo", 60, 100, 2);
    //ILI9341_Print("My Board Logo");
	

    while(1) {
        // leggi stato attuale
        btn_state = debounce_get_state();

        // leggi eventi premuti (si azzerano automaticamente)
        btn_events = debounce_get_events();

        // visualizza stato
        display_buttons(btn_state);

        // processa eventi premuti
        if(btn_events) {
            uart_print("Events detected: ");
            uart_print_hex(btn_events);
            uart_print("\r\n");
            //ILI9341_Print("Events detected: ");
            
                /*if(btn_events & (1<<0)) {
                    uart_print("Tasto 1");
                    uart_print("\r\n");
					//ILI9341_Print("Tasto 1");
                    //printf("%d ", i);
                }*/
            
            ILI9341_Print("\r\n");

            // eventualmente cancella manualmente se serve
            // debounce_clear(btn_events);
            
        }else{
            uart_print("No Events");
            uart_print("\r\n");
        }
        // piccolo delay software
        //for(volatile int d=0; d<100000; d++);
        _delay_ms(1000);
    }

    return 0;
}
