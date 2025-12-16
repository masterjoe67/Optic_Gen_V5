// ======================================================================
// TOP UNICO PER SPWM
// Selezione monofase / trifase per mapping su pin fisici
// ======================================================================
module top_spwm_selector (
    input  wire       clk,
    input  wire       rst_n,
    input  wire       mode_sel,  // 0 = monofase, 1 = trifase

    // --- MONOFASE --- (istanziare pwm_single + deadtime altrove)
    input  wire       mono_H,
    input  wire       mono_L,
    input  wire       mono_H1,
    input  wire       mono_L1,
    input  wire       mono_H2,
    input  wire       mono_L2,

    // --- TRIFASE --- (istanziare pwm_trifase + deadtime altrove)
    input  wire       tri_H_A,
    input  wire       tri_L_A,
    input  wire       tri_H_B,
    input  wire       tri_L_B,
    input  wire       tri_H_C,
    input  wire       tri_L_C,

    // --- OUTPUT PIN FISICI ---
    output reg        out_H,
    output reg        out_L,
    output reg        out_H1,
    output reg        out_L1,
    output reg        out_H2,
    output reg        out_L2
);

    // Multiplexer combinatorio per selezione modalità
    always @(*) begin
        if (mode_sel == 1'b0) begin
            // Monofase
            out_H  = mono_H;
            out_L  = mono_L;
            out_H1 = mono_H1;
            out_L1 = mono_L1;
            out_H2 = mono_H2;
            out_L2 = mono_L2;
        end else begin
            // Trifase
            out_H  = tri_H_A;
            out_L  = tri_L_A;
            out_H1 = tri_H_B;
            out_L1 = tri_L_B;
            out_H2 = tri_H_C;
            out_L2 = tri_L_C;
        end
    end

endmodule
