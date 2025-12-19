#include "ui.h"
//#include "ILI9341_text.h" // your text functions: ILI9341_set_cursor, Print, Fill_Rect...
#include "Peripheral/input.h"
#include "Peripheral/leds.h"
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include "ili9341.h"
#include "Peripheral/ext_register.h"
#include "logo/logo_mje.h"


#define MIN_CARRIER_HZ 500U
#define MAX_CARRIER_HZ 50000U
#define MIN_MOD_HZ 1U
#define MAX_MOD_HZ 500U
#define MIN_DEAD_NS 0U
#define MAX_DEAD_NS 2000U

#define pwm_f_clk 50000000U

#define CAR_VALUE_CUR_Y 60
#define MOD_VALUE_CUR_Y 103
#define DEA_VALUE_CUR_Y 147
#define MODE_VALUE_CUR_Y 202

static bool lastOutput;

static uint32_t carrierHz = 20000U; // initial
static uint32_t modHz     = 500U;
static uint32_t deadNs    = 20U;

static pwm_mode_t currentMode = MODE_HALF_BRIDGE;
static bool outputEnabled = false;

typedef enum { FIELD_NONE=0, FIELD_CARRIER, FIELD_MOD, FIELD_DEAD } field_t;
static field_t selectedField = FIELD_NONE;

// digit selection: 0 = ones, 1 = tens, 2 = hundreds, etc.
static uint8_t digit_pos = 0;



static void draw_static_layout(void)
{
    
    fillScreen(0x0000); // black
    setTextFont(2);
    
    drawRoundRect(1, 1, 315, 40, 6, ILI9341_YELLOW);

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
    //ILI9341_Draw_Filled_Rectangle(6,86,308,46, 0x0000);
    ILI9341_set_cursor(12, MOD_VALUE_CUR_Y + 8);
    ILI9341_Print("Modulation Freq:");
    //ILI9341_Draw_Filled_Rectangle(6,136,308,28, 0x0000);
    ILI9341_set_cursor(12, DEA_VALUE_CUR_Y + 8);
    ILI9341_Print("Dead Time:");
}




static void render_values(bool force)
{
    static uint32_t lastCarrier=0xFFFFFFFF;
    static uint32_t lastMod=0xFFFFFFFF;
    static uint32_t lastDead=0xFFFFFFFF;
    static pwm_mode_t lastMode = (pwm_mode_t)0xFF;
    lastOutput = !outputEnabled;

    carrierHz = pwm_get_carrier_hz();
    modHz = pwm_get_mod_hz();
    deadNs = pwm_get_deadtime_ns();


    if(force || carrierHz != lastCarrier) {
        
        char buf[24];
        setTextSize(2);
        setTextColor(0xFFFF, 0x0000);
        // clear area
        //ILI9341_Draw_Filled_Rectangle(120,40,294,28, 0xFF00);
        fillRect(120, 46, 190, 25, 0x0000);
        //uart_print("Debug-500_a\r\n");
        ILI9341_set_cursor(140, CAR_VALUE_CUR_Y);
        //uart_print("Debug-500_b\r\n");
        snprintf(buf,sizeof(buf), "%lu Hz", (unsigned long)carrierHz);
        ILI9341_Print(buf);
        lastCarrier = carrierHz;
    }

    //uart_print("Debug-501\r\n");
    if(force || modHz != lastMod) {
        char buf[24];
        setTextSize(2);
        setTextColor(0xFFFF, 0x0000);
        fillRect(120,96,190,25, 0x0000);
        ILI9341_set_cursor(140, MOD_VALUE_CUR_Y);
        snprintf(buf,sizeof(buf), "%lu Hz", (unsigned long)modHz);
        ILI9341_Print(buf);
        lastMod = modHz;
    }

    if(force || deadNs != lastDead) {
        char buf[24];
        int i = 0;
        setTextSize(2);
        setTextColor(0xFFFF, 0x0000);
        //ILI9341_Draw_Filled_Rectangle(160,138,160,18, 0xB000);
        fillRect(120, 146, 190, 25, 0x0000);
        ILI9341_set_cursor(140, DEA_VALUE_CUR_Y);
        //snprintf(buf,sizeof(buf), "%lu ns", (unsigned long)deadNs);
        i = u32_to_decstr(deadNs, buf);
        buf[i++] = ' ';
        buf[i++] = 'n';
        buf[i++] = 's';
        buf[i]   = '\0';

        //uart_print(buf);
        ILI9341_Print(buf);
        lastDead = deadNs;
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

    uart_print("Debug-600_a\r\n");
}

static void highlight_selected_field(field_t f)
{
    // draw border around selected field
    // clear previous by redrawing static boxes; for simplicity redraw both boxes each time
    // Carrier box
    if(f == FIELD_CARRIER) {
        ILI9341_Draw_Filled_Rectangle(6,36,308,46, 0x001F); // blue-ish highlight background
    } else {
        ILI9341_Draw_Filled_Rectangle(6,36,308,46, 0x0000);
    }
    // Mod box
    if(f == FIELD_MOD) {
        ILI9341_Draw_Filled_Rectangle(6,86,308,46, 0x03E0); // green-ish
    } else {
        ILI9341_Draw_Filled_Rectangle(6,86,308,46, 0x0000);
    }
    // Dead box
    if(f == FIELD_DEAD) {
        ILI9341_Draw_Filled_Rectangle(6,136,308,28, 0xF800); // red-ish
    } else {
        ILI9341_Draw_Filled_Rectangle(6,136,308,28, 0x0000);
    }

    // After coloring, redraw field labels and values
    setTextSize(1);
    setTextColor(0xFFFF, 0x0000);
    ILI9341_set_cursor(12, CAR_VALUE_CUR_Y + 8); ILI9341_Print("Carrier Freq:");
    ILI9341_set_cursor(12, MOD_VALUE_CUR_Y + 8); ILI9341_Print("Modulation Freq:");
    ILI9341_set_cursor(12, DEA_VALUE_CUR_Y + 8); ILI9341_Print("Dead Time:");
    render_values(true);
}

static void apply_limits_and_update(void)
{
    if(carrierHz < MIN_CARRIER_HZ) carrierHz = MIN_CARRIER_HZ;
    if(carrierHz > MAX_CARRIER_HZ) carrierHz = MAX_CARRIER_HZ;
    if(modHz < MIN_MOD_HZ) modHz = MIN_MOD_HZ;
    if(modHz > MAX_MOD_HZ) modHz = MAX_MOD_HZ;
    if(deadNs < MIN_DEAD_NS) deadNs = MIN_DEAD_NS;
    if(deadNs > MAX_DEAD_NS) deadNs = MAX_DEAD_NS;

    pwm_set_carrier(carrierHz);
    pwm_set_mod_hz(modHz);
    pwm_set_deadtime_ns(deadNs);
    pwm_set_mode(currentMode);
    pwm_enable(outputEnabled);
}

void ui_init(void)
{
    //pwm_enable(false);
    carrierHz = pwm_get_carrier_hz();
    modHz = pwm_get_mod_hz();
    deadNs = pwm_get_deadtime_ns();
    ILI9341_Fill_Screen(0x780F);
    //uart_print("Debug-1\r\n");
    draw_static_layout();
    //uart_print("Debug-11\r\n");
    render_values(true);
    //uart_print("Debug-122\r\n");
    highlight_selected_field(FIELD_NONE);
    //uart_print("Debug-200\r\n");
}

void ui_splash(void)
{
    ILI9341_Fill_Screen(0x0000);
    setTextSize(3);
    setTextColor(0xFFFF, 0x0000);
    ILI9341_set_cursor(80,80);
    //drawString("My Board Logo", 60, 100, 2);
    //ILI9341_Print("M.J.E");
    //draw_rle_ili9341(logo_mje, 0, 0, 929);
    ILI9341_draw_rle(logo_mje, 60, 20, 200);
    // add messages
    setTextSize(2);
    ILI9341_set_cursor(100,200);
    setTextColor(ILI9341_GREENYELLOW, 0x0000);
    ILI9341_Print("Booting...");
    // small delay (blocking ok at startup)
    _delay_ms(600);
}

void ui_update(void)
{
    uint8_t ev = debounce_get_events();
    uint8_t btn_state = debounce_get_state();
    if(ev) {
        uart_print("Events detected: ");
            uart_print_hex(ev);
            uart_print("\r\n");
            // field selection buttons
    }    
    if (ev & (1<<4)) {
        selectedField = FIELD_CARRIER;
        digit_pos = 0;
        highlight_selected_field(selectedField);
        leds_field_carrier_on();
    }
    if (ev & (1<<1)) {
        selectedField = FIELD_MOD;
        digit_pos = 0;
        highlight_selected_field(selectedField);
        leds_field_mod_on();
    }
    if (ev & (1<<2)) {
        selectedField = FIELD_DEAD;
        digit_pos = 0;
        highlight_selected_field(selectedField);
        leds_field_dead_on();
    }

    // mode button
    if (ev & (1<<3))  {
        if (currentMode == MODE_3PHASE) currentMode = MODE_HALF_BRIDGE;
        else currentMode = (pwm_mode_t)(currentMode + 1);
        render_values(false);
    }

    // output toggle
    if (ev & (1<<0))  {
        outputEnabled = !outputEnabled;
        leds_output_set(outputEnabled);
        pwm_enable(outputEnabled);
        render_values(false);
    }

    // confirm button applies values immediately
    if (ev & (1<<5))  {
        apply_limits_and_update();
        // optional: show confirmation flash
    }

    // encoder switch: cycle digit position
    if (ev & (1<<6))  {
        // cycle positions max reasonable value
        if (selectedField == FIELD_CARRIER) {
            // carrier ranges up to 50k -> digits up to 4 (units..10000)
            digit_pos = (digit_pos + 1) % 5;
        } else if (selectedField == FIELD_MOD) {
            digit_pos = (digit_pos + 1) % 4;
        } else if (selectedField == FIELD_DEAD) {
            digit_pos = (digit_pos + 1) % 5; // dead in ns, allow up to 10k
        }
        // blink LED or draw indicator near encoder
    }

    // encoder rotation changes selected digit
    int8_t delta = encoder_get_delta();
    if (delta != 0 && selectedField != FIELD_NONE) {
        uint32_t step = 1;
        for (uint8_t i=0;i<digit_pos;i++) step *= 10;
        if (selectedField == FIELD_CARRIER) {
            pwm_set_carrier(update_param_carrier(pwm_get_carrier()));
            carrierHz = pwm_get_carrier_hz();
        } else if (selectedField == FIELD_MOD) {
            pwm_set_mod(update_param_mod(pwm_get_mod()));
            modHz = (uint32_t)pwm_get_mod_hz();
        } else if (selectedField == FIELD_DEAD) {
            pwm_set_deadtime(update_param_dead(pwm_get_deadtime()));
            deadNs = pwm_get_deadtime_ns();
        }
        render_values(false);
    }
}

uint32_t update_param_carrier(uint32_t param)
{
    static uint8_t enc_prev;
    static int8_t acc = 0;
    static uint16_t speed_cnt = 0;

    uint8_t enc = encoder_read();
    int8_t delta = (int8_t)(enc - enc_prev);
    enc_prev = enc;

    if(delta == 0) {
        speed_cnt = 0;
        return param;
    }

    speed_cnt++;

    uint8_t div = 2;          // base sensitivity
    if(speed_cnt > 20) div = 2;
    if(speed_cnt > 50) div = 1;

    acc += delta;

    if(acc >= div) {
        acc = 0;
        if(param < 50000) param++;
    }
    else if(acc <= -div) {
        acc = 0;
        if(param > 500) param--;
    }

    return param;
}

uint16_t update_param_mod(uint16_t param)
{
    static uint8_t enc_prev;
    static int8_t acc = 0;
    static uint16_t speed_cnt = 0;

    uint8_t enc = encoder_read();
    int8_t delta = (int8_t)(enc - enc_prev);
    enc_prev = enc;

    if(delta == 0) {
        speed_cnt = 0;
        return param;
    }

    speed_cnt++;

    uint8_t div = 1;          // base sensitivity
    if(speed_cnt > 20) div = 2;
    if(speed_cnt > 50) div = 1;

    acc += delta;

    if(acc >= div) {
        acc = 0;
        if(param < 0xffff) param++;
    }
    else if(acc <= -div) {
        acc = 0;
        if(param > 1) param--;
    }


return param;
}

uint8_t update_param_dead(uint8_t param)
{
    static uint8_t enc_prev;
    static int8_t acc = 0;
    static uint16_t speed_cnt = 0;

    uint8_t enc = encoder_read();
    int8_t delta = (int8_t)(enc - enc_prev);
    enc_prev = enc;

    if(delta == 0) {
        speed_cnt = 0;
        return param;
    }

    speed_cnt++;

    uint8_t div = 6;          // base sensitivity
    if(speed_cnt > 20) div = 2;
    if(speed_cnt > 50) div = 1;

    acc += delta;

    if(acc >= div) {
        acc = 0;
        if(param < 0xff) param++;
    }
    else if(acc <= -div) {
        acc = 0;
        if(param > 1) param--;
    }


return param;
}

