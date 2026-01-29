#include "ui.h"
#include "Peripheral/input.h"
#include "Peripheral/leds.h"
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "ili9341.h"
#include "Peripheral/ext_register.h"
#include "Peripheral/XPT2046.h"
#include "logo/logo_mini.h"
//#include "logo/nerkia.h"
//#include "logo/nerk.h"
#include "logo/k_icon.h"
#include "scope.h"

#define MIN_CARRIER_HZ 150U
#define MAX_CARRIER_HZ 50000U
#define MIN_MOD_HZ 1U
#define MAX_MOD_HZ 500U
#define MIN_MAG 0U
#define MAX_MAG 100U
#define MIN_DEAD_NS 0U
#define MAX_DEAD_NS 2000U

#define pwm_f_clk 200000000U

#define CAR_VALUE_CUR_Y 50  //60
#define MOD_VALUE_CUR_Y 86
#define MAG_VALUE_CUR_Y 123
#define DEA_VALUE_CUR_Y 160  //147
#define MODE_VALUE_CUR_Y 202

static bool lastOutput;

static uint32_t carrierHz = 20000U; // initial
static uint32_t modHz     = 500U;
static uint8_t magnitude = 100U;
static uint32_t deadNs    = 20U;

static uint32_t lastCarrier=0xFFFFFFFF;
static uint32_t lastMod=0xFFFFFFFF;
static uint8_t lastMagnitude=0xFF;
static uint32_t lastDead=0xFFFFFFFF;
static pwm_mode_t lastMode = (pwm_mode_t)0xFF;

static pwm_mode_t currentMode = MODE_HALF_BRIDGE;
static bool outputEnabled = false;

typedef enum { FIELD_NONE=0, FIELD_CARRIER, FIELD_MOD, FIELD_MAG, FIELD_DEAD } field_t;
static field_t selectedField = FIELD_NONE;

// digit selection: 0 = ones, 1 = tens, 2 = hundreds, etc.
static uint8_t digit_pos = 0;
static uint8_t last_digit_pos = 0;
uint8_t field_locked = 0;
uint8_t ui_locked;
uint32_t step = 0;
uint8_t cursor_pos = 1;     // 0=unità, 1=decine, 2=centinaia…
uint8_t cursor_on = 0;      // stato ON/OFF del lampeggio
uint8_t cursor_blink = 0;   
uint8_t refresh = 0;
bool flash_on  = false;
volatile uint16_t tick_count = 0;
char carrier_buf[16] = "                ";
char mod_buf[16] = "                ";
char mag_buf[16] = "                ";
char dea_buf[16] = "                ";

volatile uint16_t refresh_count = 0;
/* Timer0 Compare Match ISR */
ISR(TIMER0_OVF_vect)
{
    tick_count++;
    refresh_count++;

    // ---- toggle refresh ogni 500 tick ----
    if (refresh_count >= 500) {
        refresh_count = 0;
        refresh ^= 1;
        //PORTA ^= (1 << 5);
    }


    if (tick_count >= 1500) {   // 2000 × 0.5 ms = 1 s
        tick_count = 0;

        if(flash_on){
            PORTA ^= (1 << 4);
            
  
        }else PORTA &= ~(1 << 4);
        
        cursor_on ^= 1;
    }
    
}

static void draw_static_layout(void)
{
    
    fillScreen(0x0000); // black
    setTextFont(2);
    
    drawRoundRect(1, 1, 315, 40, 6, ILI9341_YELLOW);

   // ILI9341_draw_rle(k_rle, 4, 4, 24);
    draw_rle_bw(4, 2, 24, 38, k_rle);
    drawRoundRect(1, 45, 315, 149, 6, ILI9341_YELLOW);

    drawRoundRect(1, 198, 315, 40, 6, ILI9341_YELLOW);
// Title bar
    setTextSize(2);
    ILI9341_set_cursor(60,6);
    setTextColor(ILI9341_RED, 0x0000);
    ILI9341_Print("PWM MULTIMODE");

    
 
    // Mode & Output labels 
    setTextSize(1);
    setTextColor(ILI9341_WHITE, 0x0000);
    ILI9341_set_cursor(12, MODE_VALUE_CUR_Y + 8);
    ILI9341_Print("MODE:");
    ILI9341_set_cursor(200, MODE_VALUE_CUR_Y +8);
    ILI9341_Print("OUT:");
    // boxes for fields
    //ILI9341_Draw_Filled_Rectangle(6,36,308,46, 0x0000); // empty boxes (black bg)
    ILI9341_set_cursor(12, CAR_VALUE_CUR_Y + 8);
    ILI9341_Print("Carrier Freq:");

    ILI9341_set_cursor(12, MOD_VALUE_CUR_Y + 8);
    ILI9341_Print("Modulation Freq:");

    ILI9341_set_cursor(12, MAG_VALUE_CUR_Y + 8);
    ILI9341_Print("Magnitude:");

    ILI9341_set_cursor(12, DEA_VALUE_CUR_Y + 8);
    ILI9341_Print("Dead Time:");
}

static void blink_cursor(char *buffer, int y_pos, int digit){
    int16_t x = 140;
    digit--;
        for(int i=0;i<5;i++)
        {
            if(i == (digit - digit_pos) && cursor_on && cursor_blink){
                //drawChar(' ', x, CAR_VALUE_CUR_Y, 2);

                fillRect(x, y_pos, 18, 32, 0x0000);
                //ILI9341_write(buf[i]);  // disegna cifra
            }else if((i == (digit - digit_pos) && !cursor_on) || (last_digit_pos != digit_pos)){
                drawChar(buffer[i], x, y_pos, 2);
            }    
            x += 18;
        }
        last_digit_pos = digit_pos;
}

// converte un uint32_t in stringa a 5 cifre con zeri iniziali
void uint32_to_str5(uint32_t val, char *buf) {
    for(int i=4; i>=0; i--) {
        buf[i] = '0' + (val % 10);
        val /= 10;
    }
    buf[5] = '\0';
}

void uint32_to_str3(uint32_t val, char *buf) {
    for(int i=2; i>=0; i--) {
        buf[i] = '0' + (val % 10);
        val /= 10;
    }
    buf[3] = '\0';
}

uint8_t digits_u32(uint32_t v)
{
    if (v < 10)       return 1;
    if (v < 100)      return 2;
    if (v < 1000)     return 3;
    if (v < 10000)    return 4;
    return 5;   // fino a 50000
}


static void render_values(bool force)
{
    setTextSize(2);
    setTextColor(0xFFFF, 0x0000);
    lastOutput = !outputEnabled;

    if(force || carrierHz != lastCarrier) {

        uint32_to_str5(carrierHz, carrier_buf);
        carrier_buf[5] = ' ';
        carrier_buf[6] = 'H';
        carrier_buf[7] = 'z';
        carrier_buf[8] = '\0';
        lastCarrier = carrierHz;
        int16_t x = 140;
        fillRect(140, CAR_VALUE_CUR_Y, 170, 32, 0x0000);
        for(int i=0;i<8;i++)
        {
            drawChar(carrier_buf[i], x, CAR_VALUE_CUR_Y, 2);
            x += 18;
        }

    }

    if(force || modHz != lastMod) {
        fillRect(140,MOD_VALUE_CUR_Y, 170,32, 0x0000);

        uint32_to_str3(modHz, mod_buf);
        mod_buf[3] = ' ';
        mod_buf[4] = 'H';
        mod_buf[5] = 'z';
        mod_buf[6] = '\0';
        //ILI9341_Print(buf);
        lastMod = modHz;
        int16_t x = 140;
        for(int i=0;i<6;i++)
        {
            drawChar(mod_buf[i], x, MOD_VALUE_CUR_Y, 2);
            x += 18;
        }
    }

    if(force || magnitude != lastMagnitude) {
        fillRect(140,MAG_VALUE_CUR_Y,170,32, 0x0000);
        ILI9341_set_cursor(140, MAG_VALUE_CUR_Y);

        //snprintf(mag_buf, sizeof(mag_buf), "%03u %%", magnitude);
        uint32_to_str3(magnitude, mag_buf);
        mag_buf[3] = ' ';
        mag_buf[4] = '%';
        mag_buf[5] = '\0';

        lastMagnitude = magnitude;
        int16_t x = 140;
        for(int i=0;i<6;i++)
        {
            drawChar(mag_buf[i], x, MAG_VALUE_CUR_Y, 2);
            x += 18;
        }
    }

    if(force || deadNs != lastDead) {
        fillRect(140, DEA_VALUE_CUR_Y, 170, 32, 0x0000);
        ILI9341_set_cursor(140, DEA_VALUE_CUR_Y);

        //snprintf(dea_buf, sizeof(dea_buf), "%05lu ns", (unsigned long)deadNs);
        uint32_to_str5(deadNs, dea_buf);
        dea_buf[5] = ' ';
        dea_buf[6] = 'n';
        dea_buf[7] = 'S';
        dea_buf[8] = '\0';

        lastDead = deadNs;
        int16_t x = 140;
        for(int i=0;i<8;i++)
        {
            drawChar(dea_buf[i], x, DEA_VALUE_CUR_Y, 2);
            x += 18;
        }
    }
    if(force || currentMode != lastMode) {
        setTextSize(2);
        setTextColor(0x07E0, 0x0000); // green
        //fillRect(110,219,80,20, 0xC000);
        ILI9341_set_cursor(65, MODE_VALUE_CUR_Y);
        switch(currentMode) {
            case MODE_HALF_BRIDGE: ILI9341_Print("HALF"); break;
            case MODE_FULL_BRIDGE: ILI9341_Print("FULL"); break;
            case MODE_3PHASE: ILI9341_Print("3PH  "); break;
        }
        lastMode = currentMode;
    }
    if(force || outputEnabled != lastOutput) {
        setTextSize(2);
        setTextColor(outputEnabled?0xF800:0xFFFF, 0x0000);
        //fillRect(260,219,59,20, 0xD000);
        ILI9341_set_cursor(240, MODE_VALUE_CUR_Y);
        ILI9341_Print(outputEnabled?"ON  ":"OFF");
        lastOutput = outputEnabled;
    }

}

static void highlight_selected_field(field_t f)
{
    // draw border around selected field
    // clear previous by redrawing static boxes; for simplicity redraw both boxes each time
    setTextSize(1);
    setTextColor(0xFFFF, 0x0000);
    // Carrier box
    if(f == FIELD_CARRIER) {
        setTextColor(ILI9341_GREENYELLOW, 0x0000);
        ILI9341_set_cursor(12, CAR_VALUE_CUR_Y + 8); ILI9341_Print("Carrier Freq:");
        
        if(digit_pos > digits_u32(carrierHz)) digit_pos = digits_u32(carrierHz);
        cursor_blink = 1;
        
    } else {
        setTextColor(0xFFFF, 0x0000);
        ILI9341_set_cursor(12, CAR_VALUE_CUR_Y + 8); ILI9341_Print("Carrier Freq:");
        
    }
    // Mod box
    if(f == FIELD_MOD) {
        setTextColor(ILI9341_GREENYELLOW, 0x0000);
        ILI9341_set_cursor(12, MOD_VALUE_CUR_Y + 8); ILI9341_Print("Modulation Freq:");

        if(digit_pos > digits_u32(modHz)) digit_pos = digits_u32(modHz);
        cursor_blink = 1;

    } else {
        setTextColor(0xFFFF, 0x0000);
        ILI9341_set_cursor(12, MOD_VALUE_CUR_Y + 8); ILI9341_Print("Modulation Freq:");
    }
    // Mag box
    if(f == FIELD_MAG) {
        setTextColor(ILI9341_GREENYELLOW, 0x0000);
        ILI9341_set_cursor(12, MAG_VALUE_CUR_Y + 8); ILI9341_Print("Magnitude:");
        if(digit_pos > digits_u32(magnitude)) digit_pos = digits_u32(magnitude);
        cursor_blink = 1;
    } else {
        setTextColor(0xFFFF, 0x0000);
        ILI9341_set_cursor(12, MAG_VALUE_CUR_Y + 8); ILI9341_Print("Magnitude:");
    }
    // Dead box
    if(f == FIELD_DEAD) {
        setTextColor(ILI9341_GREENYELLOW, 0x0000);
        ILI9341_set_cursor(12, DEA_VALUE_CUR_Y + 8); ILI9341_Print("Dead Time:");
        if(digit_pos > digits_u32(deadNs)) digit_pos = digits_u32(deadNs);
        cursor_blink = 1;
    } else {
        setTextColor(0xFFFF, 0x0000);
        ILI9341_set_cursor(12, DEA_VALUE_CUR_Y + 8); ILI9341_Print("Dead Time:");
        
    }
    
    setTextColor(0xFFFF, 0x0000);
    render_values(true);
}

static void apply_limits_and_update(void)
{
    if(carrierHz < MIN_CARRIER_HZ) carrierHz = MIN_CARRIER_HZ;
    if(carrierHz > MAX_CARRIER_HZ) carrierHz = MAX_CARRIER_HZ;

    if(modHz < MIN_MOD_HZ) modHz = MIN_MOD_HZ;
    if(modHz > MAX_MOD_HZ) modHz = MAX_MOD_HZ;

    if(magnitude < MIN_MAG) magnitude = MIN_MAG;
    if(magnitude > MAX_MAG) magnitude = MAX_MAG;

    if(deadNs < MIN_DEAD_NS) deadNs = MIN_DEAD_NS;
    if(deadNs > MAX_DEAD_NS) deadNs = MAX_DEAD_NS;

    pwm_set_carrier_hz(carrierHz);
    pwm_set_mod_hz(modHz);
    pwm_set_magnitude(magnitude);
    pwm_set_deadtime_ns(deadNs);
    pwm_set_mode(currentMode);
    pwm_enable(outputEnabled);
}

void ui_init(void)
{
    // Timer0 prescaler 64
    TCCR0 = (1 << CS01) | (1 << CS00);
    TCNT0 = 0;
    TIMSK |= (1 << TOIE0);

    /* Prescaler 64 */
    TCCR0 |= (1 << CS01) | (1 << CS00);

    sei();                    // enable global interrupts
   
    //pwm_enable(false);
    carrierHz = pwm_get_carrier_hz();
    modHz = pwm_get_mod_hz();
    magnitude = pwm_get_magnitude();
    deadNs = pwm_get_deadtime_ns();
    ILI9341_Fill_Screen(0x780F);
    draw_static_layout();
    render_values(true);
    highlight_selected_field(FIELD_NONE);
}

void ui_splash(void)
{
    ILI9341_Fill_Screen(0x0000);
    setTextSize(3);
    setTextColor(0xFFFF, 0x0000);
    ILI9341_set_cursor(80,80);

    //draw_rle_bw(5, 10, 114, 182, nerkia_rle);
    //ILI9341_draw_rle(nerk_rle, 120, 10, 192);
    ILI9341_draw_rle(logo_mini_rle, 150, 55, 110);
    // add messages
    setTextSize(2);

    ILI9341_set_cursor(170,165);
    setTextColor(ILI9341_BLUE, 0x0000);
    ILI9341_Print("2026");

    ILI9341_set_cursor(50,200);
    setTextColor(ILI9341_GREENYELLOW, 0x0000);
    ILI9341_Print("Booting");
    
    for(int i = 0; i < 8; i++){
        ILI9341_Print(".");
       _delay_ms(600); 
    }
}

void ui_update(void)
{
    int ev = debounce_get_events();

    uint16_t tx, ty;
    uint8_t  td;
    static uint8_t lastMagnitude=0xFF;
    ui_locked = outputEnabled;

    td = XPT2046_TouchGetCoordinates(&tx, &ty);   // 1 = tocco valido

int z = touch_process_zones(tx, ty, td, zones, NUM_ZONES);

if (z >= 0) {
    if(!ev){
         ev = 1 << z;
    }
    if(z == 7) ev <<= 1;
}

if (field_locked) {
    ev &= (1 << 6) | (1 << 7);   // lascia solo CONFIRM
}

if (outputEnabled) {
    ev &= (1 << 5) | (1 << 2) | (1 << 6) | (1 << 8);   // solo ON/OFF
}

    if(ev) {
    }    
    if (ev & (1<<0)) {
        field_locked = 1;
        selectedField = FIELD_CARRIER;
        digit_pos = 3;
        flash_on = true;
        highlight_selected_field(selectedField);
        leds_field_carrier_on();
    }
    if (ev & (1<<1)) {
        field_locked = 1;
        selectedField = FIELD_MOD;
        digit_pos = 0;
        flash_on = true;
        highlight_selected_field(selectedField);
        leds_field_mod_on();
    }
    if (ev & (1<<2)) {
        field_locked = 1;
        selectedField = FIELD_MAG;
        digit_pos = 0;
        flash_on = true;
        highlight_selected_field(selectedField);
        leds_field_mag_on();
    }

    if (ev & (1<<3)) {
        field_locked = 1;
        selectedField = FIELD_DEAD;
        digit_pos = 0;
        flash_on = true;
        highlight_selected_field(selectedField);
        leds_field_dead_on();
    }

    // mode button
    if (ev & (1<<4))  {
        if (currentMode == MODE_3PHASE) currentMode = MODE_HALF_BRIDGE;
        else currentMode = (pwm_mode_t)(currentMode + 1);
        pwm_set_mode(currentMode);
        render_values(false);
    }

    // output toggle
    if (ev & (1<<5))  {
        outputEnabled = !outputEnabled;
        leds_output_set(outputEnabled);
        pwm_enable(outputEnabled);
        render_values(false);
    }

    // confirm button applies values immediately
    if (ev & (1<<6))  {
        apply_limits_and_update();
        if (selectedField == FIELD_CARRIER) {
            leds_field_carrier_off();
        } else if (selectedField == FIELD_MOD) {
            leds_field_mod_off();
        } else if (selectedField == FIELD_MAG) {
            leds_field_mag_off();
        } else if (selectedField == FIELD_DEAD) {
            leds_field_dead_off();
        }
        cursor_blink = 0;
        flash_on = false;
        highlight_selected_field(FIELD_NONE);
        render_values(true);
        field_locked = 0;
        selectedField = FIELD_NONE;

    }

    // encoder switch: cycle digit position
    if (ev & (1<<7))  {
        // cycle positions max reasonable value
        if (selectedField == FIELD_CARRIER) {
            // carrier ranges up to 50k -> digits up to 4 (units..10000)
            digit_pos = (digit_pos + 1) % 5;

        } else if (selectedField == FIELD_MOD) {
            digit_pos = (digit_pos + 1) % 3;
        } else if (selectedField == FIELD_DEAD) {
            digit_pos = (digit_pos + 1) % 5; // dead in ns, allow up to 10k
        }
        // blink LED or draw indicator near encoder
    }

    if (ev & (1<<8))  {
        //ui_splash();
        scope_main();
        draw_static_layout();
        render_values(true);
        highlight_selected_field(FIELD_NONE);
    }

    // encoder rotation changes selected digit
    int8_t delta = encoder_get_delta();
    if (delta != 0 && selectedField != FIELD_NONE) {
        step = 1;
        for (uint8_t i=0;i<digit_pos;i++) step *= 10;
        if (selectedField == FIELD_CARRIER) {
            carrierHz = update_param_32(carrierHz, MIN_CARRIER_HZ, MAX_CARRIER_HZ, step);
        } else if (selectedField == FIELD_MOD) {
            modHz = update_param_32(modHz, MIN_MOD_HZ, MAX_MOD_HZ, step);
        } else if (selectedField == FIELD_MAG) {
            lastMagnitude = magnitude;
            magnitude = update_param_8(magnitude, 0, 100, step);
            if(magnitude != lastMagnitude){
                if(magnitude < MIN_MAG) magnitude = MIN_MAG;
                if(magnitude > MAX_MAG) magnitude = MAX_MAG;
                pwm_set_magnitude(magnitude);
            }
        } else if (selectedField == FIELD_DEAD) {
            deadNs = update_param_16(deadNs, 0, 2000, step);
        }
        render_values(false);
    }
    if ((selectedField == FIELD_CARRIER) & refresh) blink_cursor(carrier_buf, CAR_VALUE_CUR_Y, 5);
    if ((selectedField == FIELD_MOD) & refresh) blink_cursor(mod_buf, MOD_VALUE_CUR_Y, 3);
    if ((selectedField == FIELD_MAG) & refresh) blink_cursor(mag_buf, MAG_VALUE_CUR_Y, 3);
    if ((selectedField == FIELD_DEAD) & refresh) blink_cursor(dea_buf, DEA_VALUE_CUR_Y, 5);
}




