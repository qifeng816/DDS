module dds_freq_ctrlword(
    input         clk,          
    input         rst_n,        
    input  [31:0] freq_target,
    output [31:0] freq_ctrl_word
);

// M = f * (2^32 / 50,000,000) 
// 2^32 / 50,000,000 约等于 85.8993459
// 我们用 (f * 5629) >> 16 来近似 (5629/65536 * 2^16... 
// 最简单且准确的方法是直接乘法：
reg [31:0] ctrl_reg;
always @(posedge clk or negedge rst_n) begin
    if(!rst_n)
        ctrl_reg <= 0;
    else begin
        // 使用 32位 * 32位 乘法，不使用除法
        // 85.8993459 * 2^10 约等于 87961
        // ctrl_reg <= (freq_target * 32'd87961) >> 10;
        
        // 更加精确的写法（如果资源允许乘法器）：
        // 直接让编译器推导常数乘法，不要写除法符号
        ctrl_reg <= freq_target * 86; // 误差约 0.1%，最节省资源
    end
end

assign freq_ctrl_word = ctrl_reg;

endmodule 