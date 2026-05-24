module MCU_uart(
    input              clk,
    input              rst_n,
    input              uart_rx,
    output reg [31:0]  data_out,   // 输出解析后的数值（频率或调制度）
    output reg         data_valid, // 数值有效脉冲
    output reg [1:0]   data_idx, 
	 output reg [2:0]   mode_sel,   // 模式选择：A=1, B=2, C=3, D=4
    output reg         mode_valid  // 模式切换有效脉冲
);

parameter CLK_FREQ = 50_000_000;
parameter UART_BPS = 115200;
localparam BAUD_END = CLK_FREQ / UART_BPS;
localparam BAUD_MID = BAUD_END / 2;

reg [15:0] baud_cnt;
reg [3:0]  bit_cnt;
reg [1:0]  state;
reg [7:0]  rx_data_reg;
reg        rx_done_sig;

localparam S_IDLE  = 2'd0, S_START = 2'd1, S_DATA  = 2'd2, S_STOP  = 2'd3;

// ================== 1. 标准 UART 接收状态机 ==================
always @(posedge clk or negedge rst_n) begin
    if(!rst_n) begin
        state <= S_IDLE; baud_cnt <= 0; bit_cnt <= 0;
        rx_data_reg <= 0; rx_done_sig <= 0;
    end else begin
        rx_done_sig <= 0;
        case(state)
            S_IDLE:  if(uart_rx == 0) begin state <= S_START; baud_cnt <= 0; end
            S_START: if(baud_cnt == BAUD_MID) begin
                        if(uart_rx == 0) begin baud_cnt <= 0; state <= S_DATA; bit_cnt <= 0; end
                        else state <= S_IDLE;
                     end else baud_cnt <= baud_cnt + 1'b1;
            S_DATA:  if(baud_cnt == BAUD_END - 1) begin
                        baud_cnt <= 0; rx_data_reg[bit_cnt] <= uart_rx;
                        if(bit_cnt == 7) state <= S_STOP;
                        else bit_cnt <= bit_cnt + 1'b1;
                     end else baud_cnt <= baud_cnt + 1'b1;
            S_STOP:  if(baud_cnt == BAUD_END - 1) begin
                        state <= S_IDLE; rx_done_sig <= 1;
                     end else baud_cnt <= baud_cnt + 1'b1;
            default: state <= S_IDLE;
        endcase
    end
end

// ================== 2. 协议解析逻辑 ==================
reg [31:0] freq_temp;
reg [1:0]  idx_cnt;

always @(posedge clk or negedge rst_n) begin
    if(!rst_n) begin
        freq_temp <= 0; data_out <= 0; data_valid <= 0;
        mode_sel <= 0; mode_valid <= 0; idx_cnt <= 0;
    end else begin
        data_valid <= 0;
        mode_valid <= 0;
        if(rx_done_sig) begin
            if(rx_data_reg >= "A" && rx_data_reg <= "D") begin
                mode_sel   <= (rx_data_reg == "A") ? 3'd1 :
                              (rx_data_reg == "B") ? 3'd2 :
                              (rx_data_reg == "C") ? 3'd3 : 3'd4;
                mode_valid <= 1'b1;
                idx_cnt    <= 0;    // 切换模式时，重置参数计数器
                freq_temp  <= 0;
            end else if(rx_data_reg >= "0" && rx_data_reg <= "9") begin
                freq_temp <= freq_temp * 10 + (rx_data_reg - "0");
            end else if(rx_data_reg == 8'h0A) begin // 收到回车
                data_out   <= freq_temp;
                data_valid <= 1'b1;
                data_idx   <= idx_cnt;
                idx_cnt    <= idx_cnt + 1'b1; // 参数序号递增
                freq_temp  <= 0;
            end
        end
    end
end
endmodule