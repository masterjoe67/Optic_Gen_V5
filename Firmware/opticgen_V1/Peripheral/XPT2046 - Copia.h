#include <avr/io.h>
#include <stdbool.h>

#ifndef XPT2046_H
#define XPT2046_H
/* =====================================================
 *  GPIO MACROS
 * ===================================================== */
#define SW_MOSI_HIGH()  (PORTB |=  (1<<PB3))
#define SW_MOSI_LOW()   (PORTB &= ~(1<<PB3))

#define SW_SCK_HIGH()   (PORTB |=  (1<<PB4))
#define SW_SCK_LOW()    (PORTB &= ~(1<<PB4))

#define SW_MISO_READ()  (PINB & (1<<PB5))

#define TOUCH_CS_LOW()  (PORTB &= ~(1<<PB6))
#define TOUCH_CS_HIGH() (PORTB |=  (1<<PB6))

#define TP_INT_IN()     (PINB & (1<<PB7))


/* =====================================================
 *  TOUCH COMMANDS (XPT2046)
 * ===================================================== */
#define CMD_X  0xD0
#define CMD_Y  0x90

/* =====================================================
 *  CALIBRATION (ESMPIO – DA REGOLARE)
 * ===================================================== */
#define TS_MINX  300
#define TS_MAXX  3800
#define TS_MINY  250
#define TS_MAXY  3900

#define TFT_W 320
#define TFT_H 240

//******************************************************************************************** */

#define MATRIX_AN 5992
#define MATRIX_BN -35
#define MATRIX_CN -2611392
#define MATRIX_DN 28
#define MATRIX_EN 4460
#define MATRIX_FN -1300816
#define MATRIX_DIV 65536


typedef struct Matrix 
{
	/* This arrangement of values facilitates  calculations within getDisplayPoint() */
	int  
	An, /* A = An/Divider */
	Bn, /* B = Bn/Divider */   
	Cn, /* C = Cn/Divider */   
	Dn, /* D = Dn/Divider */   
	En, /* E = En/Divider */   
	Fn, /* F = Fn/Divider */   
	Divider;   
} Matrix ;

typedef struct {
	int x[5], xfb[5];
	int y[5], yfb[5];
	int a[7];
} calibration;

typedef	struct POINT 
{
	int x;
	int y;
}Coordinate;

extern Matrix matrix;
extern Coordinate  display;

/* Private define ------------------------------------------------------------*/
/* AD channel selection command */
#define	CHX 	        0x90 	/* Channel X+ command*/	
#define	CHY 	        0xd0	/* Channel Y+ command* */

void xpt2046_init(void);	
Coordinate *Read_Value(void);
int getDisplayPoint(Coordinate * displayPtr, Coordinate * screenPtr, Matrix * matrixPtr);
int setCalibrationMatrix(Coordinate * displayPtr, Coordinate * screenPtr, Matrix * matrixPtr);
void ts_draw_point(int x, int y, int color);
void ts_draw_cross(int Xpos, int Ypos);
void ts_calibrate(int x_size, int y_size);
bool TouchPressed();

//*****************************************************************************************************/
void touchInit();
bool touch_flag();
void clear_thouch_flag();
void touch_Apply_Rotation(uint16_t *x, uint16_t *y, uint8_t rotation);

#endif