module top_spwm_selector (
    input  wire       clk,
    input  wire       rst_n,
    input  wire [1:0] mode_sel,
    input  wire       out_en,     // <--- NUOVO

    // MONOFASE
    input  wire mono_H,
    input  wire mono_L,
    input  wire mono_H1,
    input  wire mono_L1,
    input  wire mono_H2,
    input  wire mono_L2,

    // TRIFASE
    input  wire tri_H_A,
    input  wire tri_L_A,
    input  wire tri_H_B,
    input  wire tri_L_B,
    input  wire tri_H_C,
    input  wire tri_L_C,

    // USCITE
    output reg  out_H0,
    output reg  out_L0,
    output reg  out_H1,
    output reg  out_L1,
    output reg  out_H2,
    output reg  out_L2
);

reg h0,l0,h1,l1,h2,l2;

always @(*) begin
    // default
    h0 = 0; l0 = 0;
    h1 = 0; l1 = 0;
    h2 = 0; l2 = 0;

    case (mode_sel)

        2'd0: begin
            // MONOFASE CH0
            h0 = mono_H;
            l0 = mono_L;
        end

        2'd1: begin
            // DUE CH MONOFASE (CH1 + CH2)
            h0 = mono_H1;
            l0 = mono_L1;
            h1 = mono_H2;
            l1 = mono_L2;
        end

        2'd2: begin
            // TRIFASE
            h0 = tri_H_A;  l0 = tri_L_A;
            h1 = tri_H_B;  l1 = tri_L_B;
            h2 = tri_H_C;  l2 = tri_L_C;
        end
    endcase
end

// ---- GATE FINALE CON out_en ----
always @(*) begin
    if (!out_en) begin
        out_H0 = 0; out_L0 = 0;
        out_H1 = 0; out_L1 = 0;
        out_H2 = 0; out_L2 = 0;
    end else begin
        out_H0 = h0; out_L0 = l0;
        out_H1 = h1; out_L1 = l1;
        out_H2 = h2; out_L2 = l2;
    end
end

endmodule
