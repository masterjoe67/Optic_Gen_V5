// ======================================================================
//   SPWM MONOFASE compatibile Verilog-2001
//   Selezione: mezzo ponte / ponte intero
//   Clock di ingresso: 50 MHz
// ======================================================================

// Nota: il modulo clk_divider_ce NON è incluso qui (evitato duplicato).
// Assicurati di avere clk_divider_ce.v nel progetto (come nel tuo trifase).

// =============================================================
// 1) Generatore sinusoide singola da ROM 2048x10
// =============================================================
module spwm_single (
    input  wire       clk_sin,
    input  wire       rst_n,
    output reg  [9:0] sinX
);

    parameter N = 2048;
    reg [10:0] addr;

    reg [9:0] sin_rom [0:N-1];

    initial begin
        // Carica la LUT (file da mettere nella directory di simulazione/sintesi)
        $readmemh("sin_lut_2048.hex", sin_rom);
    end

    always @(posedge clk_sin or negedge rst_n) begin
        if (!rst_n)
            addr <= 0;
        else
            addr <= (addr == N-1) ? 0 : addr + 1;
    end

    always @(posedge clk_sin) begin
        sinX <= sin_rom[addr];
    end

endmodule


// =============================================================
// 2) Carrier PWM monofase
// =============================================================
module pwm_single (
    input  wire       clk_carrier,
    input  wire       rst_n,
    input  wire [9:0] sinX,
    output reg        pwmX
);

    parameter CMAX = 1023;
    reg [9:0] carrier;

    always @(posedge clk_carrier or negedge rst_n) begin
        if (!rst_n)
            carrier <= 0;
        else
            carrier <= (carrier == CMAX) ? 0 : carrier + 1;
    end

    always @(posedge clk_carrier) begin
        pwmX <= (sinX > carrier);
    end
endmodule


// =============================================================
// 3) Deadtime monofase + modalità half-bridge / full-bridge
// =============================================================
module deadtime_single (
    input  wire       clk,
    input  wire       rst_n,
    input  wire       pwmX,
    input  wire [4:0] deadtime,
    input  wire       mode,       // 0 = mezzo ponte, 1 = ponte intero

    output reg        H,
    output reg        L,
    output reg        H1,
    output reg        L1,
    output reg        H2,
    output reg        L2
);

    reg [4:0] dt;
    reg       state;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            H  <= 0; L  <= 0;
            H1 <= 0; L1 <= 0;
            H2 <= 0; L2 <= 0;
            dt <= 0;
            state <= 0;
        end else begin
            if (pwmX != state) begin
                state <= pwmX;
                dt <= deadtime;

                // Spegni tutte le uscite durante il deadtime
                H  <= 0; L  <= 0;
                H1 <= 0; L1 <= 0;
                H2 <= 0; L2 <= 0;

            end else if (dt != 0) begin
                dt <= dt - 1;

            end else begin
                if (mode == 1'b0) begin
                    // MEZZO PONTE
                    H <= state;
                    L <= ~state;
                    // mantieni gli altri a 0 per chiarezza
                    H1 <= 0; L1 <= 0;
                    H2 <= 0; L2 <= 0;
                end else begin
                    // PONTE INTERO
                    H  <= 0; L  <= 0; // mezzo ponte non usato
                    H1 <= state;
                    L1 <= ~state;
                    H2 <= ~state;
                    L2 <= state;
                end
            end
        end
    end
endmodule


// =============================================================
// 4) TOP MODULE COMPLETO SPWM MONOFASE
// =============================================================
module top_spwm_single (
    input  wire        clk,          // 50 MHz
    input  wire        rst_n,

    input  wire [15:0] freq_carrier_div,
    input  wire [15:0] freq_mod_div,
    input  wire [4:0]  deadtime,
    input  wire        mode,         // 0 = Mezzo ponte, 1 = Ponte intero

    output wire H, L,
    output wire H1, L1,
    output wire H2, L2
);

    // Clock enable modulazione (istanzia il clk_divider_ce presente nel progetto)
    wire ce_carrier;
    wire ce_mod;

    clk_divider_ce #(.WIDTH(16)) DIV_CAR (
        .clk(clk),
        .rst_n(rst_n),
        .divider(freq_carrier_div),
        .ce(ce_carrier)
    );

    clk_divider_ce #(.WIDTH(16)) DIV_MOD (
        .clk(clk),
        .rst_n(rst_n),
        .divider(freq_mod_div),
        .ce(ce_mod)
    );

    // Sinusoide
    wire [9:0] sinX;

    spwm_single SIN_GEN (
        .clk_sin(ce_mod),
        .rst_n(rst_n),
        .sinX(sinX)
    );

    // PWM
    wire pwmX;

    pwm_single PWM_GEN (
        .clk_carrier(ce_carrier),
        .rst_n(rst_n),
        .sinX(sinX),
        .pwmX(pwmX)
    );

    // Driver con deadtime + selezione mezzo/ponte intero
    deadtime_single DRV (
        .clk(clk),
        .rst_n(rst_n),
        .pwmX(pwmX),
        .deadtime(deadtime),
        .mode(mode),
        .H(H), .L(L),
        .H1(H1), .L1(L1),
        .H2(H2), .L2(L2)
    );

endmodule
