#include <avr/io.h>
#include <stdbool.h>

#ifndef XPT2046_H
#define XPT2046_H
/* =====================================================
 *  GPIO MACROS
 * ===================================================== */
#define SW_MOSI_HIGH()  (PORTB |=  (1<<PB5))
#define SW_MOSI_LOW()   (PORTB &= ~(1<<PB5))

#define SW_SCK_HIGH()   (PORTB |=  (1<<PB3))
#define SW_SCK_LOW()    (PORTB &= ~(1<<PB3))

#define SW_MISO_READ()  (PINB & (1<<PB6))

#define TOUCH_CS_LOW()  (PORTB &= ~(1<<PB4))
#define TOUCH_CS_HIGH() (PORTB |=  (1<<PB4))

#define TP_INT_IN()     (PINB & (1<<PB7))


/* AD channel selection command */
#define	CHX 	        0x90 	/* Channel X+ command*/	
#define	CHY 	        0xd0	/* Channel Y+ command* */

#define TOUCH_ORIENTATION_PORTRAIT 			(0U)
#define TOUCH_ORIENTATION_LANDSCAPE 		(1U)
#define TOUCH_ORIENTATION_PORTRAIT_MIRROR 	(2U)
#define TOUCH_ORIENTATION_LANDSCAPE_MIRROR 	(3U)

#define ORIENTATION	(TOUCH_ORIENTATION_LANDSCAPE_MIRROR)

// change depending on screen orientation
#if (ORIENTATION == 0)
#define XPT2046_SCALE_X 480
#define XPT2046_SCALE_Y 800
#elif (ORIENTATION == 1)
#define XPT2046_SCALE_X 800
#define XPT2046_SCALE_Y 480
#elif (ORIENTATION == 2)
#define XPT2046_SCALE_X 480
#define XPT2046_SCALE_Y 800
#elif (ORIENTATION == 3)
#define XPT2046_SCALE_X 320
#define XPT2046_SCALE_Y 240
#endif

// to calibrate uncomment UART_Printf line in ili9341_touch.c
#define XPT2046_MIN_RAW_X 200
#define XPT2046_MAX_RAW_X 3830
#define XPT2046_MIN_RAW_Y 400
#define XPT2046_MAX_RAW_Y 3800

// call before initializing any SPI devices
void xpt2046_init(void);
extern bool XPT2046_TouchPressed(void);
extern bool XPT2046_TouchGetCoordinates(uint16_t* x, uint16_t* y);


#endif