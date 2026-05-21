module seg_display(
    input             clk,        
    input             rst_n,
    input             en,         // 新增：显示使能
    input      [31:0] freq,       
    output reg [3:0]  sel,        
    output reg [7:0]  seg         
);

reg [3:0] x_show, y_show, z_show;
reg [31:0] temp_f, base;
reg [3:0]  x_cnt, y_cnt, z_reg;
reg [2:0]  state;

// 状态机部分保持不变，负责在后台计算频率
always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        state <= 0; x_show <= 0; y_show <= 0; z_show <= 0;
    end else begin
        case(state)
            0: begin 
                temp_f <= freq; x_cnt <= 0; y_cnt <= 0;
                if      (freq >= 10_000_000) begin z_reg <= 7; base <= 10_000_000; state <= 1; end
                else if (freq >= 1_000_000)  begin z_reg <= 6; base <= 1_000_000;  state <= 1; end
                else if (freq >= 100_000)    begin z_reg <= 5; base <= 100_000;    state <= 1; end
                else if (freq >= 10_000)     begin z_reg <= 4; base <= 10_000;     state <= 1; end
                else if (freq >= 1_000)      begin z_reg <= 3; base <= 1_000;      state <= 1; end
                else                         begin z_reg <= 2; base <= 100;        state <= 1; end
            end
            1: begin if (temp_f >= base) begin temp_f <= temp_f - base; x_cnt <= x_cnt + 1'b1; end else state <= 2; end
            2: begin if (temp_f >= (base/10)) begin temp_f <= temp_f - (base/10); y_cnt <= y_cnt + 1'b1; end 
                     else begin x_show <= x_cnt; y_show <= y_cnt; z_show <= z_reg; state <= 0; end end
            default: state <= 0;
        endcase
    end
end

// 修改扫描逻辑：增加使能判断
reg [16:0] scan_cnt; 
always @(posedge clk) scan_cnt <= scan_cnt + 1'b1;

always @(*) begin
    if (!en) begin
        sel = 4'b1111; // 使能关闭时，位选全部关断（高电平不选通）
        seg = 8'h00;
    end else begin
        case(scan_cnt[16:15]) 
            2'd0: begin sel = 4'b1110; seg = decode(x_show) | 8'h80; end
            2'd1: begin sel = 4'b1101; seg = decode(y_show); end
            2'd2: begin sel = 4'b1011; seg = 8'h00; end
            2'd3: begin sel = 4'b0111; seg = decode(z_show); end
        endcase
    end
end

function [7:0] decode(input [3:0] num);
    case(num)
        4'd0: decode = 8'h3F; 4'd1: decode = 8'h06; 4'd2: decode = 8'h5B;
        4'd3: decode = 8'h4F; 4'd4: decode = 8'h66; 4'd5: decode = 8'h6D;
        4'd6: decode = 8'h7D; 4'd7: decode = 8'h07; 4'd8: decode = 8'h7F;
        4'd9: decode = 8'h6F; default: decode = 8'h00;
    endcase
endfunction

endmodule