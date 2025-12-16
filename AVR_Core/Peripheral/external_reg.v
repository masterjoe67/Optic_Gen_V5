`timescale 1ns / 1ps

module mmio_regs_16bit_direct (
    input  wire        clk,         // clock del core
    input  wire        rst,         // reset sincrono attivo basso
    // Bus interno del core
    input  wire        core_write,
    input  wire        core_read,        // read enable (IO read)
    input  wire [5:0]  core_addr,
    input  wire [15:0] core_data_in,
    output reg  [15:0] core_data_out,
    // Uscite dirette verso l'esterno
    output wire [15:0] ext_reg0,
    output wire [15:0] ext_reg1,
    output wire [15:0] ext_reg2,
    output wire [15:0] ext_reg3
);

    // --- Registri interni ---
    reg [15:0] reg0;  //Carrier
    reg [15:0] reg1;  //Modulation
    reg [15:0] reg2;  //Dead Time
    reg [15:0] reg3;  //Mode

    // --- Scrittura dal core (uso solo core_write) ---
    always @(posedge clk or negedge rst) begin
        if (!rst) begin   // reset attivo basso
            reg0 <= 16'h0002;
            reg1 <= 16'h0000;
            reg2 <= 16'h0004;
            reg3 <= 16'h0000;
        end else if (core_write) begin
            case(core_addr)
                8'h00: reg0 <= core_data_in;
                8'h01: reg1 <= core_data_in;
                8'h02: reg2 <= core_data_in;
                8'h03: reg3 <= core_data_in;
            endcase
        end
    end

    // --- Lettura dal core (abilitata solo se core_read = 1) ---
    always @(*) begin
        if (core_read) begin
            case(core_addr)
                8'h00: core_data_out = reg0;
                8'h01: core_data_out = reg1;
                8'h02: core_data_out = reg2;
                8'h03: core_data_out = reg3;
                default: core_data_out = 16'h0000;
            endcase
        end else begin
            core_data_out = 16'h0000;
        end
    end

    // --- Uscite dirette verso l'esterno ---
    assign ext_reg0 = reg0;
    assign ext_reg1 = reg1;
    assign ext_reg2 = reg2;
    assign ext_reg3 = reg3;

endmodule
