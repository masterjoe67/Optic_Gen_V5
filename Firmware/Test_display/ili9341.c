
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include"ili9341.h"
#include "characters.h"
//#include "glcdfont.c"

unsigned int X_SIZE = 320;
unsigned int Y_SIZE = 240;

static int cursor_x = 0;
static int cursor_y = 0;
static uint16_t text_color = 0xFFFF;   // default bianco
static uint16_t text_bg = 0x0000;      // default nero
static uint8_t text_size = 1;

unsigned char hh;


#if defined(__GNUC__)
    // Versione migliore: usa typeof() e mantiene il tipo esatto
    #define swap(a, b)  do { typeof(a) _t = (a); (a) = (b); (b) = _t; } while(0)
#else
    // Versione compatibile, usa int
    #define swap(a, b)  do { int _t = (a); (a) = (b); (b) = _t; } while(0)
#endif




void spi_write(uint8_t v) {
    SPDR = v;
    while(!(SPSR & (1<<SPIF)));
}

static inline void spi_send16(uint16_t v)
{
    spi_write(v >> 8);
    spi_write(v & 0xFF);
}

void ili9341_sendCmd(uint8_t cmd) {
    DC_CMD();
    CS_LOW();
    spi_write(cmd);
    CS_HIGH();
}

void ILI9341_Send_Data(uint8_t d) {
    DC_DATA();
    CS_LOW();
    spi_write(d);
    CS_HIGH();
}

void ILI9341_Reset(void) {
    //leds(1);
    RST_LOW();
    _delay_ms(20);
    RST_HIGH();
    _delay_ms(150);
    //leds(2);
}

void ILI9341_Send_Burst(uint16_t color, uint32_t repetitions)
{
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    DC_DATA();   // dati
    CS_LOW();    // seleziona display

    // Invio ripetuto: ottimizzato
    while (repetitions--)
    {
        spi_write(hi);
        spi_write(lo);
    }

    CS_HIGH();
}

void ILI9341_Draw_Filled_Rectangle(unsigned int color,unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
{
	ILI9341_Set_Address(x1, y1, x2, y2);
	ILI9341_Send_Burst(color, (long)(x2-x1+1) * (long)(y2-y1+1));
}

void ILI9341_Draw_Empty_Rectangle(unsigned int color,unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
{
	ILI9341_Draw_Line(color, x1, y1, x2, y1);
	ILI9341_Draw_Line(color, x2, y1, x2, y2);
	ILI9341_Draw_Line(color, x1, y1, x1, y2);
	ILI9341_Draw_Line(color, x1, y2, x2, y2);
}

void ILI9341_Draw_Line(unsigned int color, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
{
	int steep = abs(y2-y1) > abs(x2-x1);

	if (steep)
	{
		swap(x1,y1);
		swap(x2,y2);
	}

	if(x1>x2)
	{
		swap(x1,x2);
		swap(y1,y2);
	}

	int dx,dy;
	dx = (x2 - x1);
	dy = abs(y2 - y1);
	int err = dx / 2;
	int ystep;
	if(y1 < y2)
	{
		ystep = 1;
	}
	else
	{
		ystep = -1;
	}
	for (; x1 <= x2; x1++)
	{
		if (steep)
		{
			ILI9341_Draw_Pixel(y1, x1, color);
		}
		else
		{
			ILI9341_Draw_Pixel(x1, y1, color);
		}
		err -= dy;
		if (err < 0)
		{
			y1 += ystep;
			err += dx;
		}
	}
}

void ILI9341_Init() {
	//ili9341_spi_init();
	//SPI_Cmd(SPI2, ENABLE);
	/* Reset The Screen */
	ILI9341_Reset();
	ili9341_sendCmd(0x01);
	_delay_ms(500);

	/* Power Control A */
	ili9341_sendCmd(0xCB);
	ILI9341_Send_Data(0x39);
	ILI9341_Send_Data(0x2C);
	ILI9341_Send_Data(0x00);
	ILI9341_Send_Data(0x34);
	ILI9341_Send_Data(0x02);

	/* Power Control B */
	ili9341_sendCmd(0xCF);
	ILI9341_Send_Data(0x00);
	ILI9341_Send_Data(0xC1);
	ILI9341_Send_Data(0x30);

	/* Driver timing control A */
	ili9341_sendCmd(0xE8);
	ILI9341_Send_Data(0x85);
	ILI9341_Send_Data(0x00);
	ILI9341_Send_Data(0x78);

	/* Driver timing control B */
	ili9341_sendCmd(0xEA);
	ILI9341_Send_Data(0x00);
	ILI9341_Send_Data(0x00);

	/* Power on Sequence control */
	ili9341_sendCmd(0xED);
	ILI9341_Send_Data(0x64);
	ILI9341_Send_Data(0x03);
	ILI9341_Send_Data(0x12);
	ILI9341_Send_Data(0x81);

	/* Pump ratio control */
	ili9341_sendCmd(0xF7);
	ILI9341_Send_Data(0x20);

	/* Power Control 1 */
	ili9341_sendCmd(0xC0);
	ILI9341_Send_Data(0x23);

	/* Power Control 2 */
	ili9341_sendCmd(0xC1);
	ILI9341_Send_Data(0x10);

	/* VCOM Control 1 */
	ili9341_sendCmd(0xC5);
	ILI9341_Send_Data(0x3E);
	ILI9341_Send_Data(0x28);

	/* VCOM Control 2 */
	ili9341_sendCmd(0xC7);
	ILI9341_Send_Data(0x86);

	/* VCOM Control 2 */
	ili9341_sendCmd(0x36);
	ILI9341_Send_Data(0x48);

	/* Pixel Format Set */
	ili9341_sendCmd(0x3A);
	ILI9341_Send_Data(0x55);    //16bit

	ili9341_sendCmd(0xB1);
	ILI9341_Send_Data(0x00);
	ILI9341_Send_Data(0x18);

	/* Display Function Control */
	ili9341_sendCmd(0xB6);
	ILI9341_Send_Data(0x08);
	ILI9341_Send_Data(0x82);
	ILI9341_Send_Data(0x27);

	/* 3GAMMA FUNCTION DISABLE */
	ili9341_sendCmd(0xF2);
	ILI9341_Send_Data(0x00);

	/* GAMMA CURVE SELECTED */
	ili9341_sendCmd(0x26);  //Gamma set
	ILI9341_Send_Data(0x01); 	//Gamma Curve (G2.2)

	//Positive Gamma  Correction
	ili9341_sendCmd(0xE0);
	ILI9341_Send_Data(0x0F);
	ILI9341_Send_Data(0x31);
	ILI9341_Send_Data(0x2B);
	ILI9341_Send_Data(0x0C);
	ILI9341_Send_Data(0x0E);
	ILI9341_Send_Data(0x08);
	ILI9341_Send_Data(0x4E);
	ILI9341_Send_Data(0xF1);
	ILI9341_Send_Data(0x37);
	ILI9341_Send_Data(0x07);
	ILI9341_Send_Data(0x10);
	ILI9341_Send_Data(0x03);
	ILI9341_Send_Data(0x0E);
	ILI9341_Send_Data(0x09);
	ILI9341_Send_Data(0x00);

	//Negative Gamma  Correction
	ili9341_sendCmd(0xE1);
	ILI9341_Send_Data(0x00);
	ILI9341_Send_Data(0x0E);
	ILI9341_Send_Data(0x14);
	ILI9341_Send_Data(0x03);
	ILI9341_Send_Data(0x11);
	ILI9341_Send_Data(0x07);
	ILI9341_Send_Data(0x31);
	ILI9341_Send_Data(0xC1);
	ILI9341_Send_Data(0x48);
	ILI9341_Send_Data(0x08);
	ILI9341_Send_Data(0x0F);
	ILI9341_Send_Data(0x0C);
	ILI9341_Send_Data(0x31);
	ILI9341_Send_Data(0x36);
	ILI9341_Send_Data(0x0F);

	//EXIT SLEEP
	ili9341_sendCmd(0x11);
	_delay_ms(120);
	//TURN ON DISPLAY
	ili9341_sendCmd(0x29);
	//ILI9341_Send_Data(0x2C);
	//SPI_Cmd(SPI2, DISABLE);
}

void ILI9341_Set_Rotation(unsigned char rotation) {
	ili9341_sendCmd(0x36);
	switch (rotation) {
	case 0:
		ILI9341_Send_Data(0x48);
		X_SIZE = 240;
		Y_SIZE = 320;
		break;
	case 1:
		ILI9341_Send_Data(0x28);
		X_SIZE = 320;
		Y_SIZE = 240;
		break;
	case 2:
		ILI9341_Send_Data(0x88);
		X_SIZE = 240;
		Y_SIZE = 320;
		break;
	case 3:
		
		
		ILI9341_Send_Data(0xE8);
		X_SIZE = 320;
		Y_SIZE = 240;
		break;
	}
}

void ILI9341_Set_Address(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2)
{
ili9341_sendCmd(0x2A);
ILI9341_Send_Data(X1>>8);
ILI9341_Send_Data(X1);
ILI9341_Send_Data(X2>>8);
ILI9341_Send_Data(X2);

ili9341_sendCmd(0x2B);
ILI9341_Send_Data(Y1>>8);
ILI9341_Send_Data(Y1);
ILI9341_Send_Data(Y2>>8);
ILI9341_Send_Data(Y2);

ili9341_sendCmd(0x2C);
}

void ILI9341_Fill_Screen(unsigned int color)
{

	ILI9341_Set_Address(0, 0, X_SIZE-1, Y_SIZE-1);
	DC_DATA();
    CS_LOW();
	for(int16_t row = 0; row < Y_SIZE; row++)
    {
        for(int16_t col = 0; col < X_SIZE; col++)
        {
            ILI9341_Send_Data(color >> 8);
            ILI9341_Send_Data(color & 0xFF);
        }
    }
	CS_HIGH();
}

void ILI9341_Draw_Pixel(int x, int y, unsigned int color)
{
	if((x < 0) || (y < 0) || (x >= X_SIZE) || (y >= Y_SIZE))
	{
		return;
	}
	ILI9341_Set_Address(x, y, X_SIZE-1, Y_SIZE-1);
	DC_CMD();;
	ILI9341_Send_Data(0x2C);
	DC_DATA();
	ILI9341_Send_Data(color>>8);
	ILI9341_Send_Data(color);
}

void ILI9341_Draw_Circle(unsigned int x0, unsigned int y0, int r, unsigned int color, unsigned char flood) {
	int f = 1 - r;
	int ddF_x = 1;
	int ddF_y = -2 * r;
	int x = 0;
	int y = r;
	if (flood == 0) {
		ILI9341_Draw_Pixel(x0, y0 + r, color);
		ILI9341_Draw_Pixel(x0, y0 - r, color);
		ILI9341_Draw_Pixel(x0 + r, y0, color);
		ILI9341_Draw_Pixel(x0 - r, y0, color);
		while (x < y) {
			if (f >= 0) {
				y--;
				ddF_y += 2;
				f += ddF_y;
			}
			x++;
			ddF_x += 2;
			f += ddF_x;
			ILI9341_Draw_Pixel(x0 + x, y0 + y, color);
			ILI9341_Draw_Pixel(x0 - x, y0 + y, color);
			ILI9341_Draw_Pixel(x0 + x, y0 - y, color);
			ILI9341_Draw_Pixel(x0 - x, y0 - y, color);
			ILI9341_Draw_Pixel(x0 + y, y0 + x, color);
			ILI9341_Draw_Pixel(x0 - y, y0 + x, color);
			ILI9341_Draw_Pixel(x0 + y, y0 - x, color);
			ILI9341_Draw_Pixel(x0 - y, y0 - x, color);
		}
	} else {
		ILI9341_Draw_Pixel(x0, y0 + r, color);
		ILI9341_Draw_Pixel(x0, y0 - r, color);
		ILI9341_Set_Address(x0 - r, y0, x0 + r, y0);
		DC_CMD();
		spi_write(0x2C);
		DC_DATA();
		for (uint32_t fff = 0; fff < r * 2 + 1; fff++) {
			spi_write(color >> 8);
			spi_write(color);
		}
		while (x < y) {
			if (f >= 0) {
				y--;
				ddF_y += 2;
				f += ddF_y;
			}
			x++;
			ddF_x += 2;
			f += ddF_x;
			ILI9341_Set_Address(x0 - x, y0 + y, x0 + x, y0 + y);
			DC_CMD();
			spi_write(0x2C);
			DC_DATA();
			for (uint32_t fff = 0; fff < x * 2 + 1; fff++) {
				spi_write(color >> 8);
				spi_write(color);
			}
			ILI9341_Set_Address(x0 - x, y0 - y, x0 + x, y0 - y);
			DC_CMD();
			spi_write(0x2C);
			DC_DATA();
			for (uint32_t fff = 0; fff < x * 2 + 1; fff++) {
				spi_write(color >> 8);
				spi_write(color);
			}
			ILI9341_Set_Address(x0 - y, y0 + x, x0 + y, y0 + x);
			DC_CMD();
			spi_write(0x2C);
			DC_DATA();
			for (uint32_t fff = 0; fff < y * 2 + 1; fff++) {
				spi_write(color >> 8);
				spi_write(color);
			}
			ILI9341_Set_Address(x0 - y, y0 - x, x0 + y, y0 - x);
			DC_CMD();
			spi_write(0x2C);
			DC_DATA();
			for (uint32_t fff = 0; fff < y * 2 + 1; fff++) {
				spi_write(color >> 8);
				spi_write(color);
			}
		}
	}
}

void ILI9341_Draw_Char(int x, int y, uint16_t color, uint16_t bgcolor, uint8_t charcode, uint8_t size)
{
    if (charcode < 0x20 || charcode > 0x7F) return;

    const uint8_t *glyph = chars8[charcode - 0x20];

    int fw = 8;   // 8 colonne reali + 1 di spazio
    int fh = 8;

    int x2 = x + fw * size - 1;
    int y2 = y + fh * size - 1;
       /*uart_print("x ");
        uart_print(": 0x");
        uart_print_hex(x);
        uart_print("\r\n");
        uart_print("x2 ");
        uart_print(": 0x");
        uart_print_hex(x2);
        uart_print("\r\n");
        uart_print("y ");
        uart_print(": 0x");
        uart_print_hex(y);
        uart_print("\r\n");
        uart_print("y2 ");
        uart_print(": 0x");
        uart_print_hex(y2);
        uart_print("\r\n");*/
    ILI9341_Set_Address(x, y, x2, y2);

    DC_DATA();
    CS_LOW();
    uint8_t mask = 0x1;
    for (int row = 0; row < fh; row++)
    {
        uint8_t rowbyte = glyph[row];
 
        //uint8_t xss = 0;
        // scaling verticale
        for (int ys = 0; ys < size; ys++)
        {
            // --- 8 colonne reali del carattere ---
            for (int col = 0; col < 8; col++)
            {
                uint8_t bit = (rowbyte >> (7 - col)) & 1;
                /*uart_print("col ");
                    
                    uart_print_hex(col);
                    uart_print("\r\n");*/
                for (int xs = 0; xs < size; xs++){
                    //spi_send16(bit ? color : bgcolor);
                    /*xss = xs;
                    uart_print("xs ");
                    
                    uart_print_hex(xss);
                    uart_print("\r\n");
                    
                    uart_print("bit ");
                    
                    uart_print_hex(bit);
                    uart_print("\r\n");*/
                    if (bit) {
                        SPDR = color >> 8; asm volatile( "nop\n\t" ::); // Sync to SPIF bit
                        while (!(SPSR & _BV(SPIF)));
                        SPDR = color;
                        while (!(SPSR & _BV(SPIF)));
                    }
                    else {
                        SPDR = bgcolor >> 8; asm volatile( "nop\n\t" ::);    
                        while (!(SPSR & _BV(SPIF)));
                        SPDR = bgcolor;
                        while (!(SPSR & _BV(SPIF)));
                    
                    }
                }
            }

            // --- colonna di spaziatura ---
            //for (int xs = 0; xs < size; xs++) spi_send16(bgcolor);
        }
    }

    CS_HIGH();
}

void ILI9341_set_cursor(int x, int y) {
    cursor_x = x;
    cursor_y = y;
}

void ILI9341_set_text_color(uint16_t color, uint16_t bgcolor) {
    text_color = color;
    text_bg = bgcolor;
}

void ILI9341_set_text_size(uint8_t size) {
    text_size = size;
}

void ILI9341_putc(char c) {
    if(c == '\n') {
        cursor_y += 8 * text_size;
        cursor_x = 0;
    } else if(c == '\r') {
        cursor_x = 0;
    } else {
        ILI9341_Draw_Char(cursor_x, cursor_y, text_color, text_bg, c, text_size);
        cursor_x += 8 * text_size;  // passo orizzontale di 8 pixel per carattere
    }
}

void ILI9341_print(const char *str) {
    while(*str) {
        ILI9341_putc(*str++);
    }
}

void ILI9341_printf(const char *fmt, ...) {
    char buffer[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    ILI9341_print(buffer);
}


// stampa su UART i 8 byte di 'A'

void uart_putc(char c)
{
    // aspetta che il buffer di trasmissione sia vuoto
    while (!(UCSR0A & (1 << UDRE0)));

    // scrivi il byte nel registro di trasmissione
    UDR0 = c;
}

void uart_print_hex(uint8_t v)
{
    const char hex[] = "0123456789ABCDEF";
    uart_putc(hex[(v >> 4) & 0x0F]);
    uart_putc(hex[v & 0x0F]);
}




void ILI9341_Draw_String(unsigned int x, unsigned int y, unsigned int color, unsigned int phone, char *str, unsigned char size)
{
	switch (size)
	{
	case 1:
		while (*str)
		{
			if ((x+(size*8))>X_SIZE)
			{
				x = 1;
				y = y + (size*8);
			}
			ILI9341_Draw_Char(x, y, color, phone, *str, size);
			x += size*8;
			*str++;
		}
	break;
	case 2:
		hh=2;
		while (*str)
		{
			if ((x+(size*8))>X_SIZE)
			{
				x = 1;
				y = y + (size*8);
			}
			ILI9341_Draw_Char(x,y,color,phone,*str,size);
			x += hh*8;
			*str++;
		}
	break;
	}
}

uint16_t ILI9341_RGB565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
}


