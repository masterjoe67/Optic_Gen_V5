
#ifndef ILI9341_DRIVER_H
#define ILI9341_DRIVER_H

// ILI9341 -------------------------------------
#define CS_LOW()   (PORTB &= ~(1<<PB0))
#define CS_HIGH()  (PORTB |=  (1<<PB0))
#define DC_CMD()   (PORTB &= ~(1<<PB2))
#define DC_DATA()  (PORTB |=  (1<<PB2))
#define RST_LOW()  (PORTB &= ~(1<<PB1))
#define RST_HIGH() (PORTB |=  (1<<PB1))

// --- Definizioni colori RGB565 stile ILI9341 ---
#define ILI9341_BLACK       0x0000
#define ILI9341_NAVY        0x000F
#define ILI9341_DARKGREEN   0x03E0
#define ILI9341_DARKCYAN    0x03EF
#define ILI9341_MAROON      0x7800
#define ILI9341_PURPLE      0x780F
#define ILI9341_OLIVE       0x7BE0
#define ILI9341_LIGHTGREY   0xC618
#define ILI9341_DARKGREY    0x7BEF
#define ILI9341_BLUE        0x001F
#define ILI9341_GREEN       0x07E0
#define ILI9341_CYAN        0x07FF
#define ILI9341_RED         0xF800
#define ILI9341_MAGENTA     0xF81F
#define ILI9341_YELLOW      0xFFE0
#define ILI9341_WHITE       0xFFFF
#define ILI9341_ORANGE      0xFD20
#define ILI9341_GREENYELLOW 0xAFE5
#define ILI9341_PINK        0xF81F    // variazione magenta/rosa




void ILI9341_Init();
void ILI9341_Set_Rotation(unsigned char rotation);
void ILI9341_Set_Address(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2);
void ILI9341_Fill_Screen(unsigned int color);
void ILI9341_Draw_Pixel(int x, int y, unsigned int color);
void ILI9341_Draw_Line(unsigned int color, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
void ILI9341_Draw_Filled_Rectangle(unsigned int color,unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
void ILI9341_Draw_Empty_Rectangle(unsigned int color,unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
void ILI9341_Draw_Circle(unsigned int x0, unsigned int y0, int r, unsigned int color, unsigned char flood);
void ILI9341_Draw_Char(int x, int y, uint16_t color, uint16_t bgcolor, uint8_t charcode, uint8_t size);
void ILI9341_set_cursor(int x, int y);
void ILI9341_set_text_color(uint16_t color, uint16_t bgcolor);
void ILI9341_set_text_size(uint8_t size);
void ILI9341_putc(char c);
void ILI9341_print(const char *str);
void ILI9341_printf(const char *fmt, ...);



void ILI9341_Draw_String(unsigned int x, unsigned int y, unsigned int color, unsigned int phone, char *str, unsigned char size);
uint16_t ILI9341_RGB565(uint8_t r, uint8_t g, uint8_t b);


#endif