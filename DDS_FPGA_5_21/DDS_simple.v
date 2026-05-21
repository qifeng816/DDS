module DDS_simple(
    input         clk,        // 系统时钟50MHz
    input         rst_n,      // 低电平复位
    input         uart_rx,    // MCU TX 输入
    input         switch,     // 拨码开关: 1=1kHz, 0=MCU
    output        led,        // LED指示灯
    output [13:0] sine_out,   // 14位DAC输出
    output [7:0]  seg,        // 数码管段选
    output [3:0]  sel,        // 数码管位选
    output        out_dacclk,  // DAC时钟输出
	 output        locked       //PLL稳定标志
);

// 内部信号定义
wire [31:0]  freq_hz_uart;   
wire [31:0]  freq_final;     
wire [31:0]  freq_ctrl_word; 
reg  [31:0]  phase;          
wire [11:0]  rom_data;
wire mcu_active;       

assign freq_final = switch ? 32'd1000 : freq_hz_uart;
assign led = switch;  

assign sine_out = {2'b00, rom_data};

dds_freq_ctrlword u_freq_ctrlword(
    .clk            (clk),
    .rst_n          (rst_n),
    .freq_target    (freq_final),
    .freq_ctrl_word (freq_ctrl_word)
);


always @(posedge clk or negedge rst_n) begin
    if(!rst_n)
        phase <= 32'd0;
    else
        phase <= phase + freq_ctrl_word;
end


sine_12bit_256 u_rom_sine (
    .address (phase[31:24]), // 取高8位作为地址
    .clock   (clk),
    .q       (rom_data)
);


//DAC_clk u_dac_clk(
//    .clk_50m (clk),
//    .dac_clk (out_dacclk)
//);
dac904_pll u_pll (
    .areset ( 1'b0 ),  // 复位：接 0 = 不复位（最常用）
    .inclk0 ( clk  ),  // 输入：50MHz 晶振
    .c0     ( out_dacclk),  // 输出：200MHz 给 DAC
    .locked ( locked   )   // 输出：时钟稳定标志
);


// 1. 实例化 MCU_uart，接出 mcu_active
MCU_uart u_mcu_uart(
    .clk        (clk),
    .rst_n      (rst_n),
    .uart_rx    (uart_rx),
    .freq_out   (freq_hz_uart),
    .mcu_active (mcu_active) // 这个信号告诉我们 MCU 说话了没
);

// 2. 定义数码管使能逻辑：(开关打开) 或者 (MCU已激活)
wire display_en = switch | mcu_active;

// 3. 实例化数码管，接入 display_en
seg_display u_seg_display(
    .clk     (clk),
    .rst_n   (rst_n),
    .en      (display_en), // 接入使能信号
    .freq    (freq_final),
    .sel     (sel),
    .seg     (seg)
);

endmodule 