module seg_display(
    input             clk,        
    input             rst_n,
    input             en,         // 显示使能
    input      [31:0] freq,       // 频率输入
    input      [31:0] ma,         // 新增：调制度输入 (0-100)
    output reg [5:0]  sel,        // 修改为 6 位位选
    output reg [7:0]  seg         
);

reg [3:0] x_show, y_show, z_show;
reg [3:0] ma_tens, ma_ones;       // 新增：调制度的十位和个位缓存
reg [31:0] temp_f, base;
reg [3:0]  x_cnt, y_cnt, z_reg;
reg [2:0]  state;

// ================== 1. 频率与调制度数字计算逻辑 ==================
always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        state <= 0; x_show <= 0; y_show <= 0; z_show <= 0;
        ma_tens <= 0; ma_ones <= 0;
    end else begin
        case(state)
            0: begin 
                temp_f <= freq; x_cnt <= 0; y_cnt <= 0;
                // 计算调制度的十位和个位 (简单取模除法)
                ma_tens <= (ma % 100) / 10;
                ma_ones <= ma % 10;

                // 原有频率显示逻辑：判定量程
                if      (freq >= 10_000_000) begin z_reg <= 7; base <= 10_000_000; state <= 1; end
                else if (freq >= 1_000_000)  begin z_reg <= 6; base <= 1_000_000;  state <= 1; end
                else if (freq >= 100_000)    begin z_reg <= 5; base <= 100_000;    state <= 1; end
                else if (freq >= 10_000)     begin z_reg <= 4; base <= 10_000;     state <= 1; end
                else if (freq >= 1_000)      begin z_reg <= 3; base <= 1_000;      state <= 1; end
                else                         begin z_reg <= 2; base <= 100;        state <= 1; end
            end
            1: begin 
                if (temp_f >= base) begin 
                    temp_f <= temp_f - base; x_cnt <= x_cnt + 1'b1; 
                end else state <= 2; 
            end
            2: begin 
                if (temp_f >= (base/10)) begin 
                    temp_f <= temp_f - (base/10); y_cnt <= y_cnt + 1'b1; 
                end else begin 
                    x_show <= x_cnt; y_show <= y_cnt; z_show <= z_reg; 
                    state <= 0; 
                end 
            end
            default: state <= 0;
        endcase
    end
end

// ================== 2. 六位数码管扫描逻辑 ==================
reg [16:0] scan_cnt; 
always @(posedge clk) scan_cnt <= scan_cnt + 1'b1;

// 扫描状态：0-2显示频率，3为空格/分隔，4-5显示调制度
always @(*) begin
    if (!en) begin
        sel = 6'b111111; // 全部关断
        seg = 8'h00;
    end else begin
        case(scan_cnt[16:14]) // 使用高3位，支持 0-7 状态循环（实际用到0-5）
            3'd0: begin 
                sel = 6'b111110; // 第一位：频率整数部分
                seg = decode(x_show) | 8'h80; // 带小数点
            end
            3'd1: begin 
                sel = 6'b111101; // 第二位：频率小数部分
                seg = decode(y_show); 
            end
            3'd2: begin 
                sel = 6'b111011; // 第三位：频率指数(Z)
                seg = decode(z_show); 
            end
            3'd3: begin 
                sel = 6'b110111; // 第四位：分隔位（空白）
                seg = 8'h00; 
            end
            3'd4: begin 
                sel = 6'b101111; // 第五位：调制度十位
                seg = decode(ma_tens); 
            end
            3'd5: begin 
                sel = 6'b011111; // 第六位：调制度个位
                seg = decode(ma_ones); 
            end
            default: begin 
                sel = 6'b111111; 
                seg = 8'h00; 
            end
        endcase
    end
end

// ================== 3. 译码函数 ==================
function [7:0] decode(input [3:0] num);
    case(num)
        4'd0: decode = 8'h3F; 4'd1: decode = 8'h06; 4'd2: decode = 8'h5B;
        4'd3: decode = 8'h4F; 4'd4: decode = 8'h66; 4'd5: decode = 8'h6D;
        4'd6: decode = 8'h7D; 4'd7: decode = 8'h07; 4'd8: decode = 8'h7F;
        4'd9: decode = 8'h6F; default: decode = 8'h00;
    endcase
endfunction

endmodule 