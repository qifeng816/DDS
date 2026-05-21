module MCU_uart(
    input         clk,
    input         rst_n,      
    input         uart_rx,    
    output reg [31:0] freq_out,
    output reg    mcu_active    // 新增：标志 MCU 是否发送过有效数据
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

// 串口接收字节逻辑 (保持不变)
always @(posedge clk or negedge rst_n) begin
    if(!rst_n) begin
        state <= S_IDLE; baud_cnt <= 0; bit_cnt <= 0; rx_data_reg <= 0; rx_done_sig <= 0;
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
                        if(bit_cnt == 7) state <= S_STOP; else bit_cnt <= bit_cnt + 1'b1;
                     end else baud_cnt <= baud_cnt + 1'b1;
            S_STOP:  if(baud_cnt == BAUD_END - 1) begin state <= S_IDLE; rx_done_sig <= 1; end
                     else baud_cnt <= baud_cnt + 1'b1;
        endcase
    end
end

// ASCII 转数字及有效标志逻辑
reg [31:0] freq_temp;

always @(posedge clk or negedge rst_n) begin
    if(!rst_n) begin
        freq_temp <= 0;
        freq_out <= 1000;
        mcu_active <= 0; // 初始状态：MCU 未激活
    end else if(rx_done_sig) begin
        if(rx_data_reg >= "0" && rx_data_reg <= "9") begin
            freq_temp <= freq_temp * 10 + (rx_data_reg - "0");
        end else if(rx_data_reg == 8'h0A || rx_data_reg == 8'h0D) begin // 收到 \n 或 \r
            if(freq_temp >= 100) begin
                freq_out <= freq_temp;
                mcu_active <= 1; // 接收到一次完整频率后，激活显示
            end
            freq_temp <= 0;
        end
    end
end

endmodule 