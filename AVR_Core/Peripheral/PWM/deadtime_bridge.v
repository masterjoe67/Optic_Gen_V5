module deadtime_bridge(
    input  wire clk,
    input  wire rst_n,
    input  wire pwm_in,
    input  wire [15:0] dead_ticks,
    output reg  hi,
    output reg  lo
);

    reg pwm_d;
    reg [15:0] dt_cnt;
    reg inhibit;

    always @(posedge clk or negedge rst_n) begin
        if(!rst_n) begin
            pwm_d   <= 0;
            dt_cnt  <= 0;
            inhibit <= 0;
            hi      <= 0;
            lo      <= 0;
        end else begin

            // edge detect SOLO quando NON in dead-time
            if(!inhibit && (pwm_in != pwm_d)) begin
                inhibit <= 1;
                dt_cnt  <= dead_ticks;
                hi      <= 0;
                lo      <= 0;
            end
            else if(inhibit) begin
                if(dt_cnt == 0) begin
                    inhibit <= 0;
                end else begin
                    dt_cnt <= dt_cnt - 1;
                end
            end
            else begin
                // normale funzionamento
                hi <= pwm_in;
                lo <= ~pwm_in;
            end

            pwm_d <= pwm_in;
        end
    end
endmodule
