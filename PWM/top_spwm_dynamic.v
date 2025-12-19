// ======================================================================
//   FILE UNICO: SPWM trifase + carrier + deadtime + divisori dinamici
//   Clock ingresso: 50MHz
// ======================================================================

// =============================================================
// TOP MODULE SPWM COMPLETO CON MMIO + SINCRONIZZAZIONE
// =============================================================
module top_spwm_dynamic (
    input  wire        clk_avr,      // Clock del core AVR
    input  wire        clk_pwm,      // 50 MHz per PWM
    input  wire        rst_n,

    // Interfaccia MMIO
    input  wire        bus_wr,
    input  wire        bus_rd,
    input  wire [5:0]  bus_addr,
    input  wire [7:0]  bus_wdata,
    output reg  [7:0]  bus_rdata,
    output wire        out_en,       // segnala dati pronti in lettura

    // Uscite PWM trifase con deadtime
    output wire AH, AL,
    output wire BH, BL,
    output wire CH, CL
);

    // =============================================================
    // 1) Registri MMIO nel dominio AVR
    // =============================================================
    reg [31:0] carrier_tuning;    // 32 bit
    reg [15:0] freq_mod_div;      // 16 bit
    reg [4:0]  deadtime_reg;      // 5 bit
    reg out_en_reg;

    assign out_en = out_en_reg;

    // Scrittura MMIO nel dominio AVR
    always @(posedge clk_avr or negedge rst_n) begin
        if(!rst_n) begin
            carrier_tuning <= 0;
            freq_mod_div   <= 0;
            deadtime_reg   <= 0;
            out_en_reg     <= 0;
        end else begin
            out_en_reg <= 0;

            if(bus_wr) begin
                case(bus_addr)
                    6'h00: carrier_tuning[7:0]   <= bus_wdata;
                    6'h01: carrier_tuning[15:8]  <= bus_wdata;
                    6'h02: carrier_tuning[23:16] <= bus_wdata;
                    6'h03: carrier_tuning[31:24] <= bus_wdata;

                    6'h04: freq_mod_div[7:0]   <= bus_wdata;
                    6'h05: freq_mod_div[15:8]  <= bus_wdata;

                    6'h06: deadtime_reg <= bus_wdata[4:0];
                    default: ;
                endcase
            end

            if(bus_rd)
                out_en_reg <= 1;
        end
    end

    // Lettura MMIO nel dominio AVR
    always @(*) begin
        case(bus_addr)
            6'h00: bus_rdata = carrier_tuning[7:0];
            6'h01: bus_rdata = carrier_tuning[15:8];
            6'h02: bus_rdata = carrier_tuning[23:16];
            6'h03: bus_rdata = carrier_tuning[31:24];
            6'h04: bus_rdata = freq_mod_div[7:0];
            6'h05: bus_rdata = freq_mod_div[15:8];
            6'h06: bus_rdata = {3'b000, deadtime_reg};
            default: bus_rdata = 8'h00;
        endcase
    end

    // =============================================================
    // 2) Sincronizzazione dei registri per dominio PWM
    // =============================================================
    reg [31:0] carrier_latched;
    reg [15:0] freq_mod_latched;
    reg [4:0]  deadtime_latched;

    // segnali di strobe (write sincronizzato) per ogni parametro
    reg wr_carrier_sync1, wr_carrier_sync2;
    reg wr_mod_sync1, wr_mod_sync2;
    reg wr_dt_sync1, wr_dt_sync2;

    // sincronizzazione write
    always @(posedge clk_pwm or negedge rst_n) begin
        if(!rst_n) begin
            wr_carrier_sync1 <= 0; wr_carrier_sync2 <= 0;
            wr_mod_sync1     <= 0; wr_mod_sync2     <= 0;
            wr_dt_sync1      <= 0; wr_dt_sync2      <= 0;
        end else begin
            wr_carrier_sync1 <= bus_wr & (bus_addr>=6'h00 & bus_addr<=6'h03);
            wr_carrier_sync2 <= wr_carrier_sync1;

            wr_mod_sync1     <= bus_wr & (bus_addr==6'h04 | bus_addr==6'h05);
            wr_mod_sync2     <= wr_mod_sync1;

            wr_dt_sync1      <= bus_wr & (bus_addr==6'h06);
            wr_dt_sync2      <= wr_dt_sync1;
        end
    end

    // latch PWM
    always @(posedge clk_pwm or negedge rst_n) begin
        if(!rst_n) begin
            carrier_latched  <= 0;
            freq_mod_latched <= 0;
            deadtime_latched <= 0;
        end else begin
            if(wr_carrier_sync2) carrier_latched  <= carrier_tuning;
            if(wr_mod_sync2)     freq_mod_latched <= freq_mod_div;
            if(wr_dt_sync2)      deadtime_latched <= deadtime_reg;
        end
    end

    // =============================================================
    // 3) Clock-enable per LUT (modulazione)
    // =============================================================
    wire ce_mod;
    clk_divider_ce #(.WIDTH(16)) DIV_SINUS (
        .clk(clk_pwm),
        .rst_n(rst_n),
        .divider(freq_mod_latched),
        .ce(ce_mod)
    );

    // =============================================================
    // 4) ROM sinusoide trifase
    // =============================================================
    wire [9:0] sinA, sinB, sinC;

    spwm_trifase ROM_TRIFASE (
        .clk_sin(ce_mod),
        .rst_n(rst_n),
        .sinA(sinA),
        .sinB(sinB),
        .sinC(sinC)
    );

    // =============================================================
    // 5) PWM carrier con phase accumulator 32 bit
    // =============================================================
    wire pwmA, pwmB, pwmC;

    pwm_carrier_accu PWM_CARRIER (
        .clk(clk_pwm),
        .rst_n(rst_n),
        .sinA(sinA),
        .sinB(sinB),
        .sinC(sinC),
        .tuning_word(carrier_latched),
        .pwmA(pwmA),
        .pwmB(pwmB),
        .pwmC(pwmC)
    );

    // =============================================================
    // 6) Driver deadtime
    // =============================================================
    deadtime_driver DEADTIME_DRV (
        .clk(clk_pwm),
        .rst_n(rst_n),
        .pwmA(pwmA),
        .pwmB(pwmB),
        .pwmC(pwmC),
        .deadtime(deadtime_latched),
        .AH(AH), .AL(AL),
        .BH(BH), .BL(BL),
        .CH(CH), .CL(CL)
    );

endmodule

// =============================================================
// ROM trifase LUT
// =============================================================
module spwm_trifase (
    input  wire clk_sin,
    input  wire rst_n,
    output reg  [9:0] sinA,
    output reg  [9:0] sinB,
    output reg  [9:0] sinC
);
    parameter N = 2048;
    parameter OFFSET120 = N/3;
    parameter OFFSET240 = 2*N/3;

    reg [10:0] addr = 0;
    reg [9:0] sin_rom [0:N-1];

    initial begin
        $readmemh("sin_lut_2048.hex", sin_rom);
    end

    always @(posedge clk_sin or negedge rst_n) begin
        if(!rst_n)
            addr <= 0;
        else
            addr <= (addr == N-1) ? 0 : addr + 1;
    end

    always @(posedge clk_sin) begin
        sinA <= sin_rom[addr];
        sinB <= sin_rom[(addr + OFFSET120) % N];
        sinC <= sin_rom[(addr + OFFSET240) % N];
    end
endmodule

