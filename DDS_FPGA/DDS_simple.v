//module DDS_simple(
//    input         clk,        // 系统时钟 50MHz
//    input         rst_n,      // 低电平复位
//    input         uart_rx,    // MCU TX 输入
//    input         switch,     // 拨码开关：1=测试，0=控制
//    output        led,        
//    output [13:0] sine_out,   // 14 位 DAC 输出
//    output [7:0]  seg,        // 数码管段选
//    output [5:0]  sel,        // 确认修改为 6 位位选
//    output        out_dacclk, 
//    output        locked      
//);
//
//// ============================================================
//// 1. 信号定义
//// ============================================================
//wire [31:0] uart_data;
//wire        uart_data_valid;
//wire [2:0]  uart_mode;
//wire        uart_mode_valid;
//wire [1:0]  uart_data_idx;    // 必须确保连接
//
//reg [2:0]   work_mode;
//reg [31:0]  sine_freq;
//reg [31:0]  am_fc;
//reg [7:0]   am_ma;
//reg [31:0]  fm_fc;
//reg [31:0]  fm_dev;
//
//reg  [31:0] phase;
//reg  [31:0] mod_phase;
//wire [13:0] rom_data; 
//wire [13:0] mod_data; 
//
//wire [31:0] freq_ctrl_word;
//reg  [31:0] freq_target;
//reg  [31:0] display_freq;
//
//// ============================================================
//// 2. 模块实例化
//// ============================================================
//// 频率控制字计算
//dds_freq_ctrlword u_freq_ctrlword(
//    .clk(clk), .rst_n(rst_n), .freq_target(freq_target), .freq_ctrl_word(freq_ctrl_word)
//);
//
//// MCU UART (修复点：补全 data_idx 引脚)
//MCU_uart u_mcu_uart(
//    .clk        (clk),
//    .rst_n      (rst_n),
//    .uart_rx    (uart_rx),
//    .data_out   (uart_data),
//    .data_valid (uart_data_valid),
//    .data_idx   (uart_data_idx), // 连接索引信号
//    .mode_sel   (uart_mode),
//    .mode_valid (uart_mode_valid)
//);
//
//// 数码管显示 (6位版本)
//seg_display u_seg_display(
//    .clk     (clk),
//    .rst_n   (rst_n),
//    .en      (1'b1),
//    .freq    (display_freq),
//    .ma      ({24'd0, am_ma}), 
//    .sel     (sel), // 直接连接 6 位输出
//    .seg     (seg)
//);
//
//// ROM (14bit)
//sine_14bit_256 u_rom_carrier (
//    .address (phase[31:24]), .clock(clk), .q(rom_data)
//);
//
//sine_14bit_256 u_rom_mod (
//    .address (mod_phase[31:24]), .clock(clk), .q(mod_data)
//);
//
//// ============================================================
//// 3. 参数与模式管理
//// ============================================================
//always @(posedge clk or negedge rst_n) begin
//    if(!rst_n) begin
//        work_mode <= 3'd1; sine_freq <= 32'd1000;
//        am_fc <= 32'd1_000_000; am_ma <= 8'd10;
//        fm_fc <= 32'd100_000;   fm_dev <= 32'd5000;
//    end else begin
//        if(uart_mode_valid) work_mode <= uart_mode;
//
//        if(uart_data_valid) begin
//            case(work_mode)
//                3'd1: sine_freq <= uart_data;
//                3'd2: begin // AM：0=频率, 1=调制度
//                    if(uart_data_idx == 2'd0)      am_fc <= uart_data;
//                    else if(uart_data_idx == 2'd1) am_ma <= uart_data[7:0];
//                end
//                3'd3: begin // FM：0=频率, 1=频偏
//                    if(uart_data_idx == 2'd0)      fm_fc <= uart_data;
//                    else if(uart_data_idx == 2'd1) fm_dev <= uart_data;
//                end
//            endcase
//        end
//    end
//end
//
//// ============================================================
//// 4. DDS 核心逻辑
//// ============================================================
//// 1kHz 调制波相位
//always @(posedge clk or negedge rst_n) begin
//    if(!rst_n) mod_phase <= 0;
//    else mod_phase <= mod_phase + 32'd86_000;
//end
//
//// FM 控制字计算
//wire signed [14:0] fm_mod_signed = {1'b0, mod_data} - 15'sd8192;
//wire signed [46:0] fm_dev_mult   = $signed({1'b0, fm_dev}) * fm_mod_signed;
//wire signed [31:0] fm_deviation  = fm_dev_mult >>> 13;
//wire [31:0] fm_ctrl_word = ($signed(fm_fc) + fm_deviation) * 32'sd86;
//
//// 相位累加器
//reg [31:0] phase_step;
//always @(*) begin
//    if(!switch && work_mode == 3'd3) phase_step = fm_ctrl_word;
//    else phase_step = freq_ctrl_word;
//end
//
//always @(posedge clk or negedge rst_n) begin
//    if(!rst_n) phase <= 0;
//    else phase <= phase + phase_step;
//end
//
//// ============================================================
//// 5. AM 调制算法 (14bit)
//// ============================================================
//wire signed [14:0] carrier_signed = $signed({1'b0, rom_data}) - 15'sd8192;
//wire signed [14:0] mod_signed     = $signed({1'b0, mod_data}) - 15'sd8192;
//wire signed [23:0] mod_depth_long = mod_signed * $signed({1'b0, am_ma});
//wire signed [14:0] mod_depth      = mod_depth_long / 100;
//wire signed [15:0] envelope       = 16'sd8192 + mod_depth; 
//wire signed [30:0] am_mult        = carrier_signed * envelope;
//wire signed [15:0] am_scaled      = am_mult >>> 13; 
//wire signed [15:0] am_final_signed = am_scaled + 16'sd8192;
//
//wire [13:0] am_out = (am_final_signed > 16'sd16383) ? 14'd16383 : 
//                     (am_final_signed < 16'sd0)     ? 14'd0     : 
//                      am_final_signed[13:0];
//
//// ============================================================
//// 6. 输出与显示切换
//// ============================================================
//reg [13:0] dac_comb;
//always @(*) begin
//    if(switch) begin
//        dac_comb = rom_data; freq_target = 32'd1000; display_freq = 32'd1000;
//    end else begin
//        case(work_mode)
//            3'd1: begin dac_comb = rom_data; freq_target = sine_freq; display_freq = sine_freq; end
//            3'd2: begin dac_comb = am_out;   freq_target = am_fc;     display_freq = am_fc;     end
//            3'd3: begin dac_comb = rom_data; freq_target = fm_fc;     display_freq = fm_fc;     end
//            3'd4: begin dac_comb = rom_data; freq_target = sine_freq; display_freq = sine_freq; end
//            default: begin dac_comb = rom_data; freq_target = sine_freq; display_freq = sine_freq; end
//        endcase
//    end
//end
//
//assign sine_out = dac_comb;
//assign led = 1'b1;
//
//dac904_pll u_pll (.areset(1'b0), .inclk0(clk), .c0(out_dacclk), .locked(locked));
//
//endmodule

//module DDS_simple(
//    input         clk,        // 系统时钟 50MHz
//    input         rst_n,      // 低电平复位
//    input         uart_rx,    // MCU TX 输入
//    input         switch,     // 拨码开关：1=测试，0=控制
//    output        led,        
//    output [13:0] sine_out,   // 14 位 DAC 输出
//    output [7:0]  seg,        // 数码管段选
//    output [5:0]  sel,        // 6 位位选
//    output        out_dacclk, 
//    output        locked      
//);
//
//// ============================================================
//// 1. 信号定义
//// ============================================================
//wire [31:0] uart_data;
//wire        uart_data_valid;
//wire [2:0]  uart_mode;
//wire        uart_mode_valid;
//wire [1:0]  uart_data_idx;    
//
//reg [2:0]   work_mode;
//reg [31:0]  sine_freq;
//reg [31:0]  am_fc;
//reg [7:0]   am_ma;
//reg [31:0]  fm_fc;
//reg [31:0]  fm_dev;
//
//reg  [31:0] phase;
//reg  [31:0] mod_phase;
//wire [13:0] rom_data; 
//wire [13:0] mod_data; 
//
//wire [31:0] freq_ctrl_word;
//reg  [31:0] freq_target;
//reg  [31:0] display_freq;
//
//// ============================================================
//// 2. 模块实例化
//// ============================================================
//dds_freq_ctrlword u_freq_ctrlword(
//    .clk(clk), .rst_n(rst_n), .freq_target(freq_target), .freq_ctrl_word(freq_ctrl_word)
//);
//
//MCU_uart u_mcu_uart(
//    .clk        (clk),
//    .rst_n      (rst_n),
//    .uart_rx    (uart_rx),
//    .data_out   (uart_data),
//    .data_valid (uart_data_valid),
//    .data_idx   (uart_data_idx), 
//    .mode_sel   (uart_mode),
//    .mode_valid (uart_mode_valid)
//);
//
//seg_display u_seg_display(
//    .clk     (clk),
//    .rst_n   (rst_n),
//    .en      (1'b1),
//    .freq    (display_freq),
//    .ma      ({24'd0, am_ma}), 
//    .sel     (sel), 
//    .seg     (seg)
//);
//
//sine_14bit_256 u_rom_carrier (
//    .address (phase[31:24]), .clock(clk), .q(rom_data)
//);
//
//sine_14bit_256 u_rom_mod (
//    .address (mod_phase[31:24]), .clock(clk), .q(mod_data)
//);
//
//// ============================================================
//// 3. 参数与模式管理
//// ============================================================
//always @(posedge clk or negedge rst_n) begin
//    if(!rst_n) begin
//        work_mode <= 3'd1; sine_freq <= 32'd1000;
//        am_fc <= 32'd1_000_000; am_ma <= 8'd10;
//        fm_fc <= 32'd100_000;   fm_dev <= 32'd5000;
//    end else begin
//        if(uart_mode_valid) work_mode <= uart_mode;
//
//        if(uart_data_valid) begin
//            case(work_mode)
//                3'd1: sine_freq <= uart_data;
//                3'd2: begin 
//                    if(uart_data_idx == 2'd0)      am_fc <= uart_data;
//                    else if(uart_data_idx == 2'd1) am_ma <= uart_data[7:0];
//                end
//                3'd3: begin 
//                    if(uart_data_idx == 2'd0)      fm_fc <= uart_data;
//                    else if(uart_data_idx == 2'd1) fm_dev <= uart_data;
//                end
//            endcase
//        end
//    end
//end
//
//// ============================================================
//// 4. DDS 核心逻辑
//// ============================================================
//always @(posedge clk or negedge rst_n) begin
//    if(!rst_n) mod_phase <= 0;
//    else mod_phase <= mod_phase + 32'd86_000; // 1kHz 调制波
//end
//
//wire signed [14:0] fm_mod_signed = $signed({1'b0, mod_data}) - 15'sd8192;
//wire signed [46:0] fm_dev_mult   = $signed({1'b0, fm_dev}) * fm_mod_signed;
//wire signed [31:0] fm_deviation  = fm_dev_mult >>> 13;
//wire [31:0] fm_ctrl_word = ($signed(fm_fc) + fm_deviation) * 32'sd86;
//
//reg [31:0] phase_step;
//always @(*) begin
//    if(!switch && work_mode == 3'd3) phase_step = fm_ctrl_word;
//    else phase_step = freq_ctrl_word;
//end
//
//always @(posedge clk or negedge rst_n) begin
//    if(!rst_n) phase <= 0;
//    else phase <= phase + phase_step;
//end
//
//// ============================================================
//// 5. AM 调制算法 (修复削顶问题)
//// ============================================================
//wire signed [14:0] carrier_signed;
//wire signed [14:0] mod_signed;
//wire signed [23:0] mod_depth_long;
//wire signed [14:0] mod_depth;
//wire signed [15:0] envelope;       
//wire signed [30:0] am_mult;
//wire signed [15:0] am_scaled;      
//wire signed [15:0] am_final_signed;
//
//assign carrier_signed = $signed({1'b0, rom_data}) - 15'sd8192; // -8192~+8191
//assign mod_signed     = $signed({1'b0, mod_data}) - 15'sd8192;
//
//assign mod_depth_long = mod_signed * $signed({1'b0, am_ma}); 
//assign mod_depth      = mod_depth_long / 100;
//assign envelope       = 16'sd8192 + mod_depth; // 基准 1.0 = 8192
//
//assign am_mult        = carrier_signed * envelope;
//
//// 【关键修改】：右移位数改为 14 位
//// 8192 * 16384 = 2^27。右移 14 位结果为 8192。
//// 这样加偏置后最大值为 8192 + 8192 = 16384，正好不削顶。
//assign am_scaled      = am_mult >>> 14; 
//
//assign am_final_signed = am_scaled + 16'sd8192;
//
//wire [13:0] am_out = (am_final_signed > 16'sd16383) ? 14'd16383 : 
//                     (am_final_signed < 16'sd0)     ? 14'd0     : 
//                      am_final_signed[13:0];
//
//// ============================================================
//// 6. 输出与显示切换
//// ============================================================
//reg [13:0] dac_comb;
//always @(*) begin
//    if(switch) begin
//        dac_comb = rom_data; freq_target = 32'd1000; display_freq = 32'd1000;
//    end else begin
//        case(work_mode)
//            3'd1: begin dac_comb = rom_data; freq_target = sine_freq; display_freq = sine_freq; end
//            3'd2: begin dac_comb = am_out;   freq_target = am_fc;     display_freq = am_fc;     end
//            3'd3: begin dac_comb = rom_data; freq_target = fm_fc;     display_freq = fm_fc;     end
//            default: begin dac_comb = rom_data; freq_target = sine_freq; display_freq = sine_freq; end
//        endcase
//    end
//end
//
//assign sine_out = dac_comb;
//assign led = 1'b1;
//
//dac904_pll u_pll (.areset(1'b0), .inclk0(clk), .c0(out_dacclk), .locked(locked));
//
//endmodule
//上面这个有-100Mv偏置
module DDS_simple(
    input         clk,        // 系统时钟 50MHz
    input         rst_n,      // 低电平复位
    input         uart_rx,    // MCU TX 输入
    input         switch,     // 拨码开关：1=测试，0=控制
    output        led,        
    output [13:0] sine_out,   // 14 位 DAC 输出
    output [7:0]  seg,        // 数码管段选
    output [5:0]  sel,        // 6 位位选
    output        out_dacclk, 
    output        locked      
);

// ============================================================
// 1. 信号定义
// ============================================================
wire [31:0] uart_data;
wire        uart_data_valid;
wire [2:0]  uart_mode;
wire        uart_mode_valid;
wire [1:0]  uart_data_idx;    

reg [2:0]   work_mode;
reg [31:0]  sine_freq;
reg [31:0]  am_fc;
reg [7:0]   am_ma;
reg [31:0]  fm_fc;
reg [31:0]  fm_dev;

reg  [31:0] phase;
reg  [31:0] mod_phase;
wire [13:0] rom_data; 
wire [13:0] mod_data; 

wire [31:0] freq_ctrl_word;
reg  [31:0] freq_target;
reg  [31:0] display_freq;

// ============================================================
// 2. 模块实例化
// ============================================================
dds_freq_ctrlword u_freq_ctrlword(
    .clk(clk), .rst_n(rst_n), .freq_target(freq_target), .freq_ctrl_word(freq_ctrl_word)
);

MCU_uart u_mcu_uart(
    .clk        (clk),
    .rst_n      (rst_n),
    .uart_rx    (uart_rx),
    .data_out   (uart_data),
    .data_valid (uart_data_valid),
    .data_idx   (uart_data_idx), 
    .mode_sel   (uart_mode),
    .mode_valid (uart_mode_valid)
);

seg_display u_seg_display(
    .clk     (clk),
    .rst_n   (rst_n),
    .en      (1'b1),
    .freq    (display_freq),
    .ma      ({24'd0, am_ma}), 
    .sel     (sel), 
    .seg     (seg)
);

sine_14bit_256 u_rom_carrier (
    .address (phase[31:24]), .clock(clk), .q(rom_data)
);

sine_14bit_256 u_rom_mod (
    .address (mod_phase[31:24]), .clock(clk), .q(mod_data)
);

// ============================================================
// 3. 参数与模式管理
// ============================================================
always @(posedge clk or negedge rst_n) begin
    if(!rst_n) begin
        work_mode <= 3'd1; sine_freq <= 32'd1000;
        am_fc <= 32'd1_000_000; am_ma <= 8'd10;
        fm_fc <= 32'd100_000;   fm_dev <= 32'd5000;
    end else begin
        if(uart_mode_valid) work_mode <= uart_mode;

        if(uart_data_valid) begin
            case(work_mode)
                3'd1: sine_freq <= uart_data;
                3'd2: begin 
                    if(uart_data_idx == 2'd0)      am_fc <= uart_data;
                    else if(uart_data_idx == 2'd1) am_ma <= uart_data[7:0];
                end
                3'd3: begin 
                    if(uart_data_idx == 2'd0)      fm_fc <= uart_data;
                    else if(uart_data_idx == 2'd1) fm_dev <= uart_data;
                end
            endcase
        end
    end
end

// ============================================================
// 4. DDS 核心逻辑
// ============================================================
always @(posedge clk or negedge rst_n) begin
    if(!rst_n) mod_phase <= 0;
    else mod_phase <= mod_phase + 32'd86_000; // 1kHz 调制波
end

wire signed [14:0] fm_mod_signed = {1'b0, mod_data} - 15'sd8192;
wire signed [46:0] fm_dev_mult   = $signed({1'b0, fm_dev}) * fm_mod_signed;
wire signed [31:0] fm_deviation  = fm_dev_mult >>> 13;
wire [31:0] fm_ctrl_word = ($signed(fm_fc) + fm_deviation) * 32'sd86;

reg [31:0] phase_step;
always @(*) begin
    if(!switch && work_mode == 3'd3) phase_step = fm_ctrl_word;
    else phase_step = freq_ctrl_word;
end

always @(posedge clk or negedge rst_n) begin
    if(!rst_n) phase <= 0;
    else phase <= phase + phase_step;
end

// ============================================================
// 5. AM 调制算法 (修复偏置优化版)
// ============================================================
wire signed [14:0] carrier_signed;
wire signed [14:0] mod_signed;
wire signed [23:0] mod_depth_long;
wire signed [14:0] mod_depth;
wire signed [15:0] envelope;       
wire signed [30:0] am_mult;
wire signed [15:0] am_scaled;      
wire signed [15:0] am_final_signed;

assign carrier_signed = $signed({1'b0, rom_data}) - 15'sd8192; 
assign mod_signed     = $signed({1'b0, mod_data}) - 15'sd8192;

// 1. 除法舍入校正
assign mod_depth_long = mod_signed * $signed({1'b0, am_ma}); 
assign mod_depth      = (mod_depth_long + 24'sd50) / 100;
assign envelope       = 16'sd8192 + mod_depth; 

assign am_mult        = carrier_signed * envelope;

// 2. 【核心修复】：带舍入的右移 (Rounding)
// 在右移 14 位前加 2^13 (8192)，将截断误差转为舍入误差，消除数字偏置
assign am_scaled      = (am_mult + 31'sd8192) >>> 14; 

// 3. 【偏置补偿】：由于实测有 -100mV 偏置，我们手动增加直流偏置值
// 如果偏置仍然偏低，可以将 8230 继续调大（例如 8250）
// 8192 是理论中点，8230 是增加了约 38 个 LSB 的偏置补偿
assign am_final_signed = am_scaled + 16'sd8050; 

wire [13:0] am_out = (am_final_signed > 16'sd16383) ? 14'd16383 : 
                     (am_final_signed < 16'sd0)     ? 14'd0     : 
                      am_final_signed[13:0];

// ============================================================
// 6. 输出与显示切换
// ============================================================
reg [13:0] dac_comb;
always @(*) begin
    if(switch) begin
        dac_comb = rom_data; freq_target = 32'd1000; display_freq = 32'd1000;
    end else begin
        case(work_mode)
            3'd1: begin dac_comb = rom_data; freq_target = sine_freq; display_freq = sine_freq; end
            3'd2: begin dac_comb = am_out;   freq_target = am_fc;     display_freq = am_fc;     end
            3'd3: begin dac_comb = rom_data; freq_target = fm_fc;     display_freq = fm_fc;     end
            default: begin dac_comb = rom_data; freq_target = sine_freq; display_freq = sine_freq; end
        endcase
    end
end

assign sine_out = dac_comb;
assign led = 1'b1;

dac904_pll u_pll (.areset(1'b0), .inclk0(clk), .c0(out_dacclk), .locked(locked));

endmodule