

module pwm_freq_gen #(
    parameter PHASE_BITS = 32,
    parameter PWM_BITS   = 12,
    parameter CLK_FREQ_HZ = 200_000_000
)(
    input  wire clk,
    input  wire rst_n,

    input  wire [31:0] pwm_freq_hz,     // es. 20000

    output reg  [PWM_BITS-1:0] pwm_cnt, // 0..4095
	 output wire tick_o
);

    // ===============================
    // DDS per generare tick PWM
    // ===============================
    reg  [PHASE_BITS-1:0] phase_acc;
    wire [63:0] phase_inc_64;

    // frequenza target = pwm_freq * 2^PWM_BITS
    wire [63:0] target_clk_hz =
        (pwm_freq_hz == 0) ? 1 : (pwm_freq_hz * (1<<PWM_BITS));

    assign phase_inc_64 =
        (target_clk_hz << PHASE_BITS) / CLK_FREQ_HZ;

    wire [PHASE_BITS-1:0] phase_inc = phase_inc_64[PHASE_BITS-1:0];

    wire msb = phase_acc[PHASE_BITS-1];
    reg  msb_d;

    // ===============================
    // Accumulatore DDS
    // ===============================
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            phase_acc <= 0;
            msb_d     <= 0;
        end else begin
            phase_acc <= phase_acc + phase_inc;
            msb_d     <= msb;
        end
    end

    // tick quando il MSB commuta (½ periodo → 2 tick per ciclo)
    wire tick = (msb ^ msb_d);
	 assign tick_o = tick;


    // ===============================
    // Contatore PWM sincrono
    // ===============================
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            pwm_cnt <= 0;
        else if (tick) begin
            if (pwm_cnt == (1<<PWM_BITS)-1)
                pwm_cnt <= 0;
            else
                pwm_cnt <= pwm_cnt + 1;
        end
    end
endmodule

