// ======================================================================
//   FILE UNICO: SPWM trifase + carrier + deadtime + divisori dinamici
//   Clock ingresso: 50MHz
// ======================================================================


// =============================================================
// 1) Divisore di clock sicuro (genera clock-enable)
// =============================================================
module clk_divider_ce #(
    parameter WIDTH = 16
)(
    input  wire clk,
    input  wire rst_n,
    input  wire [WIDTH-1:0] divider,   // valore dinamico
    output reg  ce                      // impulso 1 ciclo
);

    reg [WIDTH-1:0] cnt = 0;

    always @(posedge clk or negedge rst_n) begin
        if(!rst_n) begin
            cnt <= 0;
            ce  <= 0;
        end else begin
            if(cnt == divider) begin
                cnt <= 0;
                ce  <= 1;
            end else begin
                cnt <= cnt + 1;
                ce  <= 0;
            end
        end
    end

endmodule



// =============================================================
// 2) Generatore trifase da ROM 2048x10 bit
// =============================================================
module spwm_trifase (
    input  wire        clk_sin,   // clock-enable della sinusoide
    input  wire        rst_n,
    output reg  [9:0]  sinA,
    output reg  [9:0]  sinB,
    output reg  [9:0]  sinC
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
        if (!rst_n)
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



// =============================================================
// 3) PWM carrier con confronto sinx > carrier
// =============================================================
module pwm_carrier (
    input  wire        clk_carrier, // clock-enable
    input  wire        rst_n,
    input  wire [9:0]  sinA,
    input  wire [9:0]  sinB,
    input  wire [9:0]  sinC,
    output reg         pwmA,
    output reg         pwmB,
    output reg         pwmC
);

    parameter CMAX = 1023;
    reg [9:0] carrier = 0;

    always @(posedge clk_carrier or negedge rst_n) begin
        if (!rst_n)
            carrier <= 0;
        else
            carrier <= (carrier == CMAX) ? 0 : carrier + 1;
    end

    always @(posedge clk_carrier) begin
        pwmA <= (sinA > carrier);
        pwmB <= (sinB > carrier);
        pwmC <= (sinC > carrier);
    end

endmodule



// =============================================================
// 4) Deadtime con 6 uscite complementari
// =============================================================
module deadtime_driver (
    input  wire        clk,      // clock reale 50MHz
    input  wire        rst_n,
    input  wire        pwmA,
    input  wire        pwmB,
    input  wire        pwmC,
    input  wire [4:0]  deadtime,
    output reg         AH,
    output reg         AL,
    output reg         BH,
    output reg         BL,
    output reg         CH,
    output reg         CL
);

    reg [4:0] dtA = 0, dtB = 0, dtC = 0;
    reg stateA = 0, stateB = 0, stateC = 0;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            {AH,AL,BH,BL,CH,CL} <= 0;
            dtA <= 0; dtB <= 0; dtC <= 0;
        end else begin

            // Fase A
            if (pwmA != stateA) begin
                stateA <= pwmA;
                dtA <= deadtime;
                AH <= 0; AL <= 0;
            end else if (dtA != 0)
                dtA <= dtA - 1;
            else begin
                AH <= stateA;
                AL <= ~stateA;
            end

            // Fase B
            if (pwmB != stateB) begin
                stateB <= pwmB;
                dtB <= deadtime;
                BH <= 0; BL <= 0;
            end else if (dtB != 0)
                dtB <= dtB - 1;
            else begin
                BH <= stateB;
                BL <= ~stateB;
            end

            // Fase C
            if (pwmC != stateC) begin
                stateC <= pwmC;
                dtC <= deadtime;
                CH <= 0; CL <= 0;
            end else if (dtC != 0)
                dtC <= dtC - 1;
            else begin
                CH <= stateC;
                CL <= ~stateC;
            end
        end
    end

endmodule



// =============================================================
// 5) TOP MODULE UNICO (ingresso clock 50MHz)
// =============================================================
module top_spwm_dynamic (
    input  wire        clk,              // 50 MHz
    input  wire        rst_n,

    input  wire [15:0] freq_carrier_div, // divisore portante
    input  wire [15:0] freq_mod_div,     // divisore sinusoide
    input  wire [4:0]  deadtime,

    output wire AH, AL,
    output wire BH, BL,
    output wire CH, CL
);

    // Clock-enable per carrier & modulazione
    wire ce_carrier;
    wire ce_mod;

    clk_divider_ce #(.WIDTH(16)) DIV_CARRIER (
        .clk(clk),
        .rst_n(rst_n),
        .divider(freq_carrier_div),
        .ce(ce_carrier)
    );

    clk_divider_ce #(.WIDTH(16)) DIV_SINUS (
        .clk(clk),
        .rst_n(rst_n),
        .divider(freq_mod_div),
        .ce(ce_mod)
    );


    // Uscite sinusoidali
    wire [9:0] sinA, sinB, sinC;

    spwm_trifase U1 (
        .clk_sin(ce_mod),
        .rst_n(rst_n),
        .sinA(sinA),
        .sinB(sinB),
        .sinC(sinC)
    );


    // PWM carrier
    wire pwmA, pwmB, pwmC;

    pwm_carrier U2 (
        .clk_carrier(ce_carrier),
        .rst_n(rst_n),
        .sinA(sinA),
        .sinB(sinB),
        .sinC(sinC),
        .pwmA(pwmA),
        .pwmB(pwmB),
        .pwmC(pwmC)
    );


    // Driver deadtime (clock reale)
    deadtime_driver U3 (
        .clk(clk),
        .rst_n(rst_n),
        .pwmA(pwmA),
        .pwmB(pwmB),
        .pwmC(pwmC),
        .deadtime(deadtime),
        .AH(AH), .AL(AL),
        .BH(BH), .BL(BL),
        .CH(CH), .CL(CL)
    );

endmodule
