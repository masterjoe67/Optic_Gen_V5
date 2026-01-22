#include <stdint.h>
#include <util/delay.h>
#include <stdbool.h>
#include "ili9341.h"
#include "scope.h"
#include "Peripheral/XPT2046.h"

// offset verticale per le tre tracce
#define CH0_Y  60
#define CH1_Y  120
#define CH2_Y  180

#define PRE_TRIGGER       150
#define POST_TRIGGER      150
#define BUFFER_TOTAL      (PRE_TRIGGER + POST_TRIGGER)
uint8_t buffer_a[BUFFER_TOTAL];
uint8_t buffer_b[BUFFER_TOTAL];
uint8_t buffer_c[BUFFER_TOTAL];
uint8_t old_buffer_a[300];
uint8_t old_buffer_b[300];
uint8_t old_buffer_c[300];

bool freeze = false;

static trigger_mode_t trigger_mode = TRIG_MODE_AUTO;
static trig_slope_t trigger_slope = TRIG_SLOPE_RISING;

void set_base_time(uint32_t value)
{
    REG_BASETIME = (uint8_t)(value & 0xFF);          // byte 0  LSB
    REG_BASETIME = (uint8_t)((value >> 8) & 0xFF);   // byte 1
    REG_BASETIME = (uint8_t)((value >> 16) & 0xFF);  // byte 2
    REG_BASETIME = (uint8_t)((value >> 24) & 0xFF);  // byte 3  MSB
}

void set_trigger_level(uint16_t level12)
{
    level12 &= 0x0FFF;   // sicurezza
    uint8_t b0 = (uint8_t)(level12 & 0xFF);
    uint8_t b1 = (uint8_t)((level12 >> 8) & 0x03);

    REG_TRIGGER_LEVEL = b0;
    REG_TRIGGER_LEVEL = b1;
}

void draw_trig_mode_btn(){
    switch (trigger_mode) {
        case TRIG_MODE_SINGLE:
            setTextSize(1);
            setTextColor(ILI9341_WHITE, 0x0000);
            ILI9341_set_cursor(270, 24);
            ILI9341_Print("SING");
            break;

        case TRIG_MODE_NORMAL:
            setTextSize(1);
            setTextColor(ILI9341_WHITE, 0x0000);
            ILI9341_set_cursor(270, 24);
            ILI9341_Print("NORM");
            break;

        case TRIG_MODE_AUTO:
            setTextSize(1);
            setTextColor(ILI9341_WHITE, 0x0000);
            ILI9341_set_cursor(270, 24);
            ILI9341_Print("AUTO");
            break;
    }
}

void draw_trig_slope_btn(){
    fillRect(266, 57, 42, 42, 0x0000 );
    switch (trigger_slope) {
        case TRIG_SLOPE_RISING:
            
            ILI9341_Draw_Line(ILI9341_CYAN, 276, 95, 286, 75);
            ILI9341_Draw_Line(ILI9341_CYAN, 286, 75, 300, 75);
            break;

        case TRIG_SLOPE_FALLING:
            ILI9341_Draw_Line(ILI9341_CYAN, 276, 75, 286, 75);
            ILI9341_Draw_Line(ILI9341_CYAN, 286, 75, 300, 95);

            break;

    }
}

/* controlla se il core è pronto */
static inline bool osc_is_ready(void)
{
    return (REG_TRIG & 0x02) != 0;  // bit READY
}

void set_trigger_mode(trigger_mode_t mode, trig_slope_t slope)
{
    uint8_t v = 0;

    v |= (mode & 0x3) << 6;        // bits 7..6 = mode
    v |= (slope & 1) << 3;        // bit 3 = edge
    v |= (1 << 2);                // trig_enable = 1
    v |= (0 << 0);                // rearm = 0

    REG_CHC = v;
    trigger_mode = mode;
    trigger_slope = slope;
    draw_trig_mode_btn();
    draw_trig_slope_btn();
}

void osc_init_trigger(uint16_t trig_level, trigger_mode_t mode,
                      trig_channel_t chan, uint8_t edge_rising) {
    // Livello trigger 10 bit
    set_trigger_level(trig_level);

    // Modalità e canale trigger
    REG_TRIG = ((mode & 0x03) << 6) |      // bit 7-6: mode
               ((chan & 0x03) << 4) |      // bit 5-4: channel
               ((edge_rising?0:1) << 3) |  // bit3: edge (0=rising,1=falling)
               (1 << 2) |                  // bit2: trigger enable
               (1 << 0);                   // bit0: rearm
}







// funzione per disegnare la traccia sul TFT
void draw_trace(uint8_t *buffer, uint8_t *old_buffer, uint16_t length, uint16_t y_offset, uint16_t color)
{
    for (uint16_t i=10; i<length; i++) {
        // x ciclico su display
        uint16_t x = i - 8;

        uint8_t y = (buffer[i] / 2) + y_offset;  
        if (y >= Y_SIZE) y = Y_SIZE-1;
  
        drawPixel(x, old_buffer[i], 0x0000);
        drawPixel(x, y, color);
        old_buffer[i] = y;
    
   } 
}

void osc_wait_ready(void)
{
    // Bit READY già implementato in lettura da REG_TRIG o bit dedicato
    while (!(REG_TRIG & (1 << READY_BIT))) {
        // attesa attiva finché READY non diventa 1
        
    }
}

void osc_read_triggered_frame(
    uint8_t *buf_a,
    uint8_t *buf_b,
    uint8_t *buf_c
)
{
    /* 1️⃣ attende che il core sia in HOLD (trigger o AUTO) */
    osc_wait_ready();

    /* 2️⃣ posiziona indice di lettura all’inizio frame */
    REG_INDEX = 0;

    /* 3️⃣ lettura sequenziale */
    for (uint16_t i = 0; i < 300; i++) {

        /* ORDINE OBBLIGATORIO */
        buf_b[i] = REG_CHB;   // stesso indice
        buf_c[i] = REG_CHC;   // stesso indice
        buf_a[i] = REG_CHA;   // AVANZA INDICE
    }

    /* 4️⃣ rearm (NORMAL / SINGLE) */
    REG_TRIG = 0x01;        // bit rearm
}



void rearm(){
    if (trigger_mode == TRIG_MODE_SINGLE & freeze){
        REG_TRIG = 0x01;
        freeze = false;
    }
}

void ToggleTriggerMode(void)
{
    switch (trigger_mode) {
        case TRIG_MODE_SINGLE:
            trigger_mode = TRIG_MODE_NORMAL;
            break;

        case TRIG_MODE_NORMAL:
            trigger_mode = TRIG_MODE_AUTO;
            break;

        case TRIG_MODE_AUTO:
        default:
            trigger_mode = TRIG_MODE_SINGLE;
            break;
    }

    set_trigger_mode(trigger_mode, trigger_slope);
    

}

void ToggleTriggerSlope(void)
{
    switch (trigger_slope) {
        case TRIG_SLOPE_RISING:
            trigger_slope = TRIG_SLOPE_FALLING;
            break;

        case TRIG_SLOPE_FALLING:
        default:
            trigger_slope = TRIG_SLOPE_RISING;
            break;

    }

    set_trigger_mode(trigger_mode, trigger_slope);
}

static inline void osc_arm_readout(void)
{
    REG_INDEX = 0;
}

/*void osc_read_triggered(uint8_t *a, uint8_t *b, uint8_t *c)
{
    
    if (trigger_mode == TRIG_MODE_SINGLE & freeze) {
        return;     
    }
    if (trigger_mode == TRIG_MODE_SINGLE | trigger_mode == TRIG_MODE_AUTO) {
        osc_wait_ready();      // SOLO in single
        freeze = true;
    }

    osc_arm_readout();       // indice = inizio pre-trigger


    for (int i = 0; i < 300; i++) {   // 150 pre + 150 post
        b[i] = REG_CHB;
        c[i] = REG_CHC;
        a[i] = REG_CHA;   // QUESTO AVANZA L’INDICE
    }
    if(trigger_mode == TRIG_MODE_NORMAL)  REG_TRIG = 0x01;        // bit rearm
}*/

void osc_read_triggered(uint8_t *a, uint8_t *b, uint8_t *c)
{
    /* SINGLE: se già congelato, non fare nulla */
    if (trigger_mode == TRIG_MODE_SINGLE && freeze) {
        return;
    }

    /* NORMAL e SINGLE aspettano il trigger */
   /* if (trigger_mode == TRIG_MODE_SINGLE || trigger_mode == TRIG_MODE_NORMAL) {
        
    }*/
osc_wait_ready();
    /* AUTO NON aspetta mai */

    /* blocca lo stato in SINGLE */
    if (trigger_mode == TRIG_MODE_SINGLE) {
        freeze = true;
    }

    /* prepara la lettura (buffer congelato!) */
    osc_arm_readout();

    for (int i = 0; i < 300; i++) {
        b[i] = REG_CHB;
        c[i] = REG_CHC;
        a[i] = REG_CHA;
    }
REG_TRIG = 0x01; 
    /* NORMAL: riarmo SOLO dopo aver finito */
    if (trigger_mode == TRIG_MODE_NORMAL) {
          // write-only, no |= !
    }
}


void oscilloscope_init(void)
{
    fillScreen(0x0000);
    drawRoundRect(0, 1, 255, 239, 6, ILI9341_WHITE);
    drawRoundRect(263, 1, 48, 48, 6, ILI9341_YELLOW);
    drawRoundRect(263, 54, 48, 48, 6, ILI9341_YELLOW);
    drawRoundRect(263, 108, 48, 48, 6, ILI9341_YELLOW);
    drawRoundRect(263, 162, 48, 48, 6, ILI9341_YELLOW);
    drawRoundRect(263, 216, 48, 24, 6, ILI9341_YELLOW);
    draw_trig_mode_btn();
}

// --- main loop ---
void scope_main(void)
{
    uint16_t tx, ty;
    uint8_t  td;

    oscilloscope_init();
    

    set_base_time(4000UL);
    set_trigger_level(100);   // metà scala

    set_trigger_mode(TRIG_MODE_AUTO, TRIG_SLOPE_RISING);
    //set_trigger_mode(TRIG_MODE_SINGLE, TRIG_SLOPE_FALLING);
    
    while(1)
    {
       osc_read_triggered(buffer_a, buffer_b, buffer_c);
       draw_trace(buffer_a, old_buffer_a, 254, CH0_Y, ILI9341_GREEN);
       draw_trace(buffer_b, old_buffer_b, 254, CH0_Y, ILI9341_RED);
       draw_trace(buffer_c, old_buffer_c, 254, CH0_Y, ILI9341_BLUE);
           


        td = XPT2046_TouchGetCoordinates(&tx, &ty);   // 1 = tocco valido

        int z = touch_process_zones(tx, ty, td, zones_osc, NUM_ZONES_OSC);

        if(z >= 0){
            switch (z)
            {
            case 0:
                ToggleTriggerMode();
                break;
            case 1:
                ToggleTriggerSlope();
                break;
            case 2:
                rearm();
                break;
            case 3:
                return;
                break;
            default:
                break;
            }
        }

        /*if(z == 1) return;   // 1 = tocco valido
        
        if(z == 0){
            ToggleTriggerMode();
        }*/
        _delay_ms(50);
        

    }
}
