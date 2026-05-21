/*------------------------------------------------------------------------------
 * File Name：DDS_up_control_MCU
 * Description：FPGA_DDS_CONTROLLER
 * 修改点：将频率数值转换为 ASCII 字符串发送，以匹配 FPGA 逻辑
 *----------------------------------------------------------------------------*/
#include "ti_msp_dl_config.h"
#include "bsp.h"
#include <stdio.h>  // 必须引入，用于处理字符串转换

// 新增：辅助函数，将频率转换为字符串发送给 FPGA
void UART_send_freq(uint32_t freq) {
    char str[12];
    // 将数字转换为字符串，例如 1000 变为 "1000\n"
    int len = sprintf(str, "%u\n", freq); 
    for (int i = 0; i < len; i++) {
        // 逐个字符发送给 FPGA
        DL_UART_transmitDataBlocking(UART_FPGA_INST, str[i]);
    }
}

int main(void) 
{
    uint8_t key;
    uint32_t uart_freq = 100;
    uint32_t temp_freq = 0;
	
    SYSCFG_DL_init();
    W25Q64_Init();      
    OLED_Init();
    OLED_DrawBMP(9, 0, 119, 8, Genshin);
    delay_ms(500);
    OLED_Clear();
	
    OLED_ShowChar(0,2,'A');
    OLED_ShowChar(8,2,':');
    OLED_ShowString(16,2,"Hz");
    OLED_ShowChar(0,4,'B');
    OLED_ShowChar(8,4,':');
    OLED_ShowString(16,4,"kHz");
    OLED_ShowChar(0,6,'C');
    OLED_ShowChar(8,6,':');
    OLED_ShowString(16,6,"MHz");

    while(1)
    {
        key = getKeyValue();
        if(key != 'N')
        {
            if(key >= '0' && key <= '9')
            {
                // 限制输入位数，防止溢出
                if(temp_freq < 10000000) {
                    temp_freq = temp_freq * 10 + (key - '0');
                    OLED_ShowNum(0,0,temp_freq,6,16);
                }
            }
            else if(key == 'A' || key == 'B' || key == 'C')
            {
                OLED_ShowNum(0,0,temp_freq,6,16);
                switch(key)
                {
                    case 'A':
                        OLED_ShowString(54,0,"Hz ");
                        uart_freq = temp_freq;
                        break;
                    case 'B':
                        OLED_ShowString(54,0,"kHz");
                        uart_freq = temp_freq * 1000;
                        break;
                    case 'C':
                        OLED_ShowString(54,0,"MHz");
                        uart_freq = temp_freq * 1000000;
                        break;
                }
                temp_freq = 0;
                // --- 修改点：调用转换函数发送 ---
                UART_send_freq(uart_freq);
            }
            else if(key == '*')
            {
                uart_freq = uart_freq + 100;
                OLED_ShowNum(0,0,uart_freq,6,16);
                // --- 修改点：调用转换函数发送 ---
                UART_send_freq(uart_freq);
                temp_freq = 0;
            }
            else if(key == '#')
            {
                if(uart_freq >= 100) {
                    uart_freq = uart_freq - 100;
                }
                OLED_ShowNum(0,0,uart_freq,6,16);
                // --- 修改点：调用转换函数发送 ---
                UART_send_freq(uart_freq);
                temp_freq = 0;
            }
            else if(key == 'D')
            {
                OLED_Clear();
                // ... (保持原有的界面刷新代码)
                OLED_ShowChar(0,2,'A');
                OLED_ShowChar(8,2,':');
                OLED_ShowString(16,2,"Hz");
                OLED_ShowChar(0,4,'B');
                OLED_ShowChar(8,4,':');
                OLED_ShowString(16,4,"kHz");
                OLED_ShowChar(0,6,'C');
                OLED_ShowChar(8,6,':');
                OLED_ShowString(16,6,"MHz");
                temp_freq = 0;
                uart_freq = 100;
                // 可选：重置时是否也发一次给FPGA
                // UART_send_freq(uart_freq);
            }
            
            delay_ms(50);
            while(getKeyValue() != 'N')
                delay_ms(10);
        }
    }
}