module sintopwm (
    input         clk,      // 系统时钟 (50MHz)
    input  [13:0] sine_val, // 来自 ROM 的 14 位正弦波数据
    output reg    pwm       // 输出到 FPGA 引脚的 PWM 信号
);

    reg [13:0] cnt; // 14位计数器，与正弦波精度一致

    always @(posedge clk) begin
        cnt <= cnt + 1'b1; // 计数器不停累加
        
        // 核心比较逻辑：
        // 如果计数器的值小于正弦波当前的数值，输出高电平
        // 这样正弦波数值越大，高电平时间越长，平均电压就越高
        if (cnt < sine_val)
            pwm <= 1'b1;
        else
            pwm <= 1'b0;
    end

endmodule
