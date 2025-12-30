
// =============================================================
// 2) Generatore trifase da ROM 2048x10 bit
// =============================================================

module spwm_trifase_mag (
    input  wire        clk_sin,
	 input  wire        clk,
    input  wire        rst_n,
    input  wire [9:0]  magnitude,   // 0..1023

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

    wire [9:0] romA = sin_rom[addr];
    wire [9:0] romB = sin_rom[(addr + OFFSET120) % N];
    wire [9:0] romC = sin_rom[(addr + OFFSET240) % N];

    // moltiplicazione 10x10 → 20 bit
    wire [19:0] mulA = romA * magnitude;
    wire [19:0] mulB = romB * magnitude;
    wire [19:0] mulC = romC * magnitude;

//    always @(posedge clk_sin or negedge rst_n) begin
//        if (!rst_n)
//            addr <= 0;
//        else
//            addr <= (addr == N-1) ? 0 : addr + 1;
//    end

always @(posedge clk or negedge rst_n) begin
    if(!rst_n)
        addr <= 0;
    else if(clk_sin)
        addr <= (addr == N-1) ? 0 : addr + 1;
end

//    always @(posedge clk_sin) begin
//        sinA <= mulA[19:10];   // divide by 1024
//        sinB <= mulB[19:10];
//        sinC <= mulC[19:10];
//    end

always @(posedge clk) begin
    if(clk_sin) begin
        sinA <= mulA[19:10];
        sinB <= mulB[19:10];
        sinC <= mulC[19:10];
    end
end

endmodule



// =============================================================
// 3) PWM carrier con confronto sinx > carrier
// =============================================================
module pwm_carrier (
    input  wire        clk_carrier, // clock-enable
	 input  wire        clk,
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

//    always @(posedge clk_carrier or negedge rst_n) begin
//        if (!rst_n)
//            carrier <= 0;
//        else
//            carrier <= (carrier == CMAX) ? 0 : carrier + 1;
//    end
//
//    always @(posedge clk_carrier) begin
//        pwmA <= (sinA > carrier);
//        pwmB <= (sinB > carrier);
//        pwmC <= (sinC > carrier);
//    end

always @(posedge clk or negedge rst_n) begin
    if(!rst_n)
        carrier <= 0;
    else if(clk_carrier)
        carrier <= (carrier == CMAX) ? 0 : carrier + 1;
end

always @(posedge clk) begin
    if(clk_carrier) begin
        pwmA <= (sinA > carrier);
        pwmB <= (sinB > carrier);
        pwmC <= (sinC > carrier);
    end
end

endmodule




// =============================================================
// 5) TOP MODULE UNICO (ingresso clock 200MHz)
// =============================================================
module top_spwm_dynamic #(
    parameter CLK_FREQ = 200_000_000
)(
    input  wire        clk,              // 200 MHz
    input  wire        rst_n,

    input  wire [31:0]  PWM_FREQ_REG,     // es. 20000
    input  wire [31:0]  FREQ_CTR_REG,     //  divisore sinusoide
    input  wire [15:0]  DEADTIME_REG,
	 input  wire [9:0] 	MAGNITUDE_REG,
	 
    output wire AH, AL,
    output wire BH, BL,
    output wire CH, CL
);

    // Clock-enable per carrier & modulazione
    wire ce_carrier;
    wire ce_mod;
	 
	 // =========================
    //  PWM carrier
    // =========================
    wire [11:0] carrier;
    wire tick;

    pwm_freq_gen #(
        .CLK_FREQ_HZ(CLK_FREQ),
        .PWM_BITS(10)
    ) PWMGEN (
        .clk(clk),
        .rst_n(rst_n),
        .pwm_freq_hz(PWM_FREQ_REG),
        .pwm_cnt(carrier),
        .tick_o(ce_carrier)
    );
	 
	 mod_freq_gen #(
        .CLK_FREQ_HZ(CLK_FREQ),
        .MOD_BITS(11)
    ) MODGEN (
        .clk(clk),
        .rst_n(rst_n),
        .mod_freq_hz(FREQ_CTR_REG),
        .mod_cnt(carrier),
        .tick_o(ce_mod)
    );
	 
	 


    // Uscite sinusoidali
    wire [9:0] sinA, sinB, sinC;

 spwm_trifase_mag U1 (
    .clk_sin(ce_mod),
	 .clk(clk),
    .rst_n(rst_n),
    .magnitude(MAGNITUDE_REG),
    .sinA(sinA),
    .sinB(sinB),
    .sinC(sinC)
);


    // PWM carrier
    wire pwmA, pwmB, pwmC;

    pwm_carrier U2 (
        .clk_carrier(ce_carrier),
		  .clk(clk),
        .rst_n(rst_n),
        .sinA(sinA),
        .sinB(sinB),
        .sinC(sinC),
        .pwmA(pwmA),
        .pwmB(pwmB),
        .pwmC(pwmC)
    );
	 
	 // =========================
    //  Dead-time + lockout
    // =========================
    deadtime_bridge DA(.clk(clk), .rst_n(rst_n), .pwm_in(pwmA),
                       .dead_ticks(DEADTIME_REG), .hi(AH), .lo(AL));

    deadtime_bridge DB(.clk(clk), .rst_n(rst_n), .pwm_in(pwmB),
                       .dead_ticks(DEADTIME_REG), .hi(BH), .lo(BL));

    deadtime_bridge DC(.clk(clk), .rst_n(rst_n), .pwm_in(pwmC),
                       .dead_ticks(DEADTIME_REG), .hi(CH), .lo(CL));





endmodule
