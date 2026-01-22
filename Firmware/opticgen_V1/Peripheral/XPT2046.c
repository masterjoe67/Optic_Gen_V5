#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include "XPT2046.h"
#include"../ili9341.h"

/* Private define ------------------------------------------------------------*/
#if (ORIENTATION == 0)
#define READ_X 0xD0
#define READ_Y 0x90
#elif (ORIENTATION == 1)
#define READ_Y 0x90
#define READ_X 0xd0
#elif (ORIENTATION == 2)
#define READ_X 0xD0
#define READ_Y 0x90
#elif (ORIENTATION == 3)
#define READ_X 0xD0
#define READ_Y 0x90
#endif

TouchZone_t zones[NUM_ZONES] = {
    {  0,   50, 320,  32,  0 },   // zona 0  carrier
    {  0,   86, 320,  32,  0 },   // zona 1  modulation
    {  0,   123, 320,  32, 0 },  // zona 2  magnitude
    {  0,   160, 320,  32, 0 },  // zona 3  deadtime
    {  0,   202, 160,  32, 0 },  // zona 4  mode
    {  170, 202, 150,  32, 0 },  // zona 5  output
    {  290,   0,  30,  40, 0 },  // zona 6  ok
    {   30,   0,  30,  40, 0 },  // zona 7  about
};

TouchZone_t zones_osc[NUM_ZONES_OSC] = {
    {  263,   1, 48,  48,  0 },   // zona 0  trigger
    {  263,  54, 48,  48,  0 },
    {  263,  108, 48,  48,  0 },
    {   30,   0, 30,  40,  0 },
};

static void delay_nus(int cnt)
{
	int i, us;
	for (i = 0; i < cnt; i++)
	{
		us = 40;
		while (us--)     /* delay	*/
		{
		}
	}
}

// Funzione per rimappare un valore da un intervallo a un altro
long map(long x, long in_min, long in_max, long out_min, long out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
 
 void xpt2046_init(void)
{
    DDRB |= (1<<PB3) | (1<<PB4) | (1<<PB5); // MOSI, SCK, CS
    DDRB &= ~(1<<PB6);                      // MISO input
    DDRB &= ~(1<<PB7);                      // TP_INT input
    SW_SCK_LOW();
    SW_MOSI_LOW();
    TOUCH_CS_HIGH();
}

uint8_t spi_sw_txrx(uint8_t data)
{
    uint8_t rx = 0;

    for (uint8_t i = 0; i < 8; i++) {
        if (data & 0x80) SW_MOSI_HIGH();
        else             SW_MOSI_LOW();

        data <<= 1;

        SW_SCK_HIGH();

        rx <<= 1;
        if (SW_MISO_READ())
            rx |= 1;

        SW_SCK_LOW();
    }
    return rx;
}

uint16_t xpt2046_read_x(void)
{
    uint16_t x;
    uint8_t hi, lo;

    TOUCH_CS_LOW();

    spi_sw_txrx(0xD0);          // READ X command

    hi = spi_sw_txrx(0x00);
    lo = spi_sw_txrx(0x00);

    TOUCH_CS_HIGH();

    x = ((uint16_t)hi << 8) | lo;
    x >>= 3;                     // 12 bit validi

    return x;
}


uint16_t xpt2046_read_y(void)
{
    uint16_t y;

    TOUCH_CS_LOW();

    spi_sw_txrx(0x90);          // READ Y command

    uint8_t hi = spi_sw_txrx(0x00);
    uint8_t lo = spi_sw_txrx(0x00);

    TOUCH_CS_HIGH();

    y = ((uint16_t)hi << 8) | lo;
    y >>= 3;                     // 12 bit validi

    return y;
}

bool XPT2046_TouchGetCoordinates(uint16_t* x, uint16_t* y)
{
#ifndef SOFTWARE_SPI

	static const uint8_t cmd_read_x[] = { READ_X };
	static const uint8_t cmd_read_y[] = { READ_Y };
	static const uint8_t zeroes_tx[] = { 0x00, 0x00 };

#endif /* SOFTWARE_SPI */

	TOUCH_CS_LOW();

	uint32_t avg_x = 0;
	uint32_t avg_y = 0;
	uint8_t nsamples = 0;
	
	uint16_t ADC_X, ADC_Y;

	for (uint8_t i = 0; i < 16; i++)
	{
		if (!XPT2046_TouchPressed())
			break;

		nsamples++;

		uint8_t y_raw[2];
		uint8_t x_raw[2];


 
            ADC_X = xpt2046_read_x();
            ADC_Y = xpt2046_read_y();

        

		avg_x += ADC_X; //(((uint16_t)x_raw[0]) << 8) | ((uint16_t)x_raw[1]);
		avg_y += ADC_Y; //(((uint16_t)y_raw[0]) << 8) | ((uint16_t)y_raw[1]);
	}

	TOUCH_CS_HIGH();

	if (nsamples < 16)
		return false;
	uint32_t raw_x = (avg_x / 16);
	uint32_t raw_y = (avg_y / 16);
	


	
	if (raw_x < XPT2046_MIN_RAW_X) raw_x = XPT2046_MIN_RAW_X;
	if (raw_x > XPT2046_MAX_RAW_X) raw_x = XPT2046_MAX_RAW_X;

	
	if (raw_y < XPT2046_MIN_RAW_Y) raw_y = XPT2046_MIN_RAW_Y;
	if (raw_y > XPT2046_MAX_RAW_Y) raw_y = XPT2046_MAX_RAW_Y;

	// Uncomment this line to calibrate touchscreen:
	//    printf("raw_x = %6d, raw_y = %6d\r\n", (int) raw_x, (int) raw_y);
	//    printf("\x1b[1F");
	
	
#if (ORIENTATION == 0)
	*x = (raw_x - XPT2046_MIN_RAW_X) * XPT2046_SCALE_X / (XPT2046_MAX_RAW_X - XPT2046_MIN_RAW_X);
	*y = (raw_y - XPT2046_MIN_RAW_Y) * XPT2046_SCALE_Y / (XPT2046_MAX_RAW_Y - XPT2046_MIN_RAW_Y);
#elif (ORIENTATION == 1)
	*x = (raw_x - XPT2046_MIN_RAW_X) * XPT2046_SCALE_X / (XPT2046_MAX_RAW_X - XPT2046_MIN_RAW_X);
	*y = (raw_y - XPT2046_MIN_RAW_Y) * XPT2046_SCALE_Y / (XPT2046_MAX_RAW_Y - XPT2046_MIN_RAW_Y);
#elif (ORIENTATION == 2)
	*x = (raw_x - XPT2046_MIN_RAW_X) * XPT2046_SCALE_X / (XPT2046_MAX_RAW_X - XPT2046_MIN_RAW_X);
	*y = XPT2046_SCALE_Y - (raw_y - XPT2046_MIN_RAW_Y) * XPT2046_SCALE_Y / (XPT2046_MAX_RAW_Y - XPT2046_MIN_RAW_Y);
#elif (ORIENTATION == 3)
	*y = (uint16_t) map(raw_x, XPT2046_MIN_RAW_Y, XPT2046_MAX_RAW_Y,  0, XPT2046_SCALE_Y);
	//*x = XPT2046_SCALE_X - (raw_y - XPT2046_MIN_RAW_X) * XPT2046_SCALE_X / (XPT2046_MAX_RAW_X - XPT2046_MIN_RAW_X);
	*x = (uint16_t) map(raw_y, XPT2046_MIN_RAW_X, XPT2046_MAX_RAW_X, 0, XPT2046_SCALE_X);
	//*y = (raw_y - XPT2046_MIN_RAW_Y) * XPT2046_SCALE_Y / (XPT2046_MAX_RAW_Y - XPT2046_MIN_RAW_Y);
#endif

	return true;
}


bool XPT2046_TouchPressed() {
	return !TP_INT_IN();
	
}



static int point_in_zone(uint16_t x, uint16_t y, TouchZone_t *z)
{
    return (x >= z->x) &&
           (x <  (z->x + z->w)) &&
           (y >= z->y) &&
           (y <  (z->y + z->h));
}

/*int touch_process_zones(uint16_t x, uint16_t y, uint8_t touch_down)
{
    static int last_touch = 0;

    int hit = -1;


    if (touch_down && !last_touch) {
        // fronte di salita: nuovo tocco
        for (int i = 0; i < NUM_ZONES; i++) {
            if (point_in_zone(x, y, &zones[i])) {
                zones[i].pressed = 1;
                hit = i;
                break;
            }
        }
    }

    if (!touch_down) {
        // rilascio → reset feedback
        for (int i = 0; i < NUM_ZONES; i++)
            zones[i].pressed = 0;
    }

    last_touch = touch_down;
    return hit;   // -1 = nessuna zona
}*/

int touch_process_zones(uint16_t x, uint16_t y, uint8_t touch_down, TouchZone_t *zon, int n)
{
    static int last_touch = 0;

    int hit = -1;


    if (touch_down && !last_touch) {
        // fronte di salita: nuovo tocco
        for (int i = 0; i < n; i++) {
            if (point_in_zone(x, y, &zon[i])) {
                zon[i].pressed = 1;
                hit = i;
                break;
            }
        }
    }

    if (!touch_down) {
        // rilascio → reset feedback
        for (int i = 0; i < NUM_ZONES; i++)
            zones[i].pressed = 0;
    }

    last_touch = touch_down;
    return hit;   // -1 = nessuna zona
}


