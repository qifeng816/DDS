module sintopwm (
    input         clk,      // 系统时钟 (50MHz)
    input  [13:0] sine_val, // 来自 ROM 的 14 位正弦波数据
    output reg    pwm       // 输出到 FPGA 引脚的 PWM 信号
);

    reg [16:0] sigma; // 17 位累加器，防止溢出

    always @(posedge clk) begin
        // 反馈值 = 2^14 = 16384，必须比输入最大值(16383)大 1
        // 否则 sine_val=16383 时 sigma 不变，输出削顶
        sigma <= sigma + sine_val - (pwm ? 16'd16384 : 16'd0);
        pwm   <= sigma[16];  // 取最高位作为 PWM 输出
    end

endmodule
