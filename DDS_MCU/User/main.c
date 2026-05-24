/*------------------------------------------------------------------------------
 * File Name：DDS_up_control_MCU
 * Description：FPGA_DDS_CONTROLLER
 * 修改点：
 * 1. 主界面显示改为 A: B: C: D:
 * 2. 模式切换按键由 1-4 改为 A-D
 * 3. 发送给 FPGA 的模式指令改为字符 'A'-'D'
 * 4. 统一了各模式下的 'D' 键为返回主界面逻辑，避免逻辑冲突
 *----------------------------------------------------------------------------*/

#include "ti_msp_dl_config.h"
#include "bsp.h"
#include <stdio.h>
#include <string.h> 

uint32_t fm_deviation = 5000;   // FM 默认最大频偏 5kHz

// 辅助函数：将频率转换为字符串发送给 FPGA
void UART_send_freq(uint32_t freq) {
    char str[12];
    int len = sprintf(str, "%u\n", freq);
    for (int i = 0; i < len; i++) {
        DL_UART_transmitDataBlocking(UART_FPGA_INST, str[i]);
    }
}

// 主界面显示：改为 A, B, C, D
void main_show(){
    OLED_ShowChar(0,0,'A');
    OLED_ShowChar(8,0,':');
    OLED_ShowString(16,0,"Sine wave signal");
    OLED_ShowChar(0,2,'B');
    OLED_ShowChar(8,2,':');
    OLED_ShowString(16,2,"AM signal");
    OLED_ShowChar(0,4,'C');
    OLED_ShowChar(8,4,':');
    OLED_ShowString(16,4,"FM signal");
    OLED_ShowChar(0,6,'D');
    OLED_ShowChar(8,6,':');
    OLED_ShowString(16,6,"PSK/ASK signal");
}

// 产生正弦波界面
void sine_wave_show(){
    OLED_ShowChar(0,2,'A');
    OLED_ShowChar(8,2,':');
    OLED_ShowString(16,2,"Hz");
    OLED_ShowChar(0,4,'B');
    OLED_ShowChar(8,4,':');
    OLED_ShowString(16,4,"kHz");
    OLED_ShowChar(0,6,'C');
    OLED_ShowChar(8,6,':');
    OLED_ShowString(16,6,"MHz");
}

void AM_show(){
    OLED_ShowString(0,0,"AM signal");
    OLED_ShowString(0,2,"Fc:");
    OLED_ShowString(72,2,"MHz");
    OLED_ShowString(0,4,"ma:");
    OLED_ShowChar(48,4,'%');
}

void FM_show(){
    OLED_ShowString(0,0,"FM signal");
    OLED_ShowString(0,2,"Fc:");
    OLED_ShowString(0,4,"Dev:");
    OLED_ShowString(48,4,"5kHz ");
}

void PSK_ASK_show(){
    OLED_ShowString(0,0,"PSK/ASK signal");
}

int main(void)
{
    uint8_t key;
    uint8_t first_key;
    uint32_t uart_freq = 100;
    uint32_t temp_freq = 0;
    int OLED_state = 0;
	
    // AM相关
    uint32_t am_fc = 1000000;
    uint8_t  am_ma = 10;
    char     am_freq_str[8] = {0};
    uint8_t  am_has_dot = 0;

    SYSCFG_DL_init();
    W25Q64_Init();
    OLED_Init();
    OLED_DrawBMP(9, 0, 119, 8, Genshin);
    delay_ms(500);
    OLED_Clear();

    main_show();

    while(1)
    {
        // ------------------ 主界面逻辑 (State 0) ------------------
        if(OLED_state == 0)
        {
            first_key = getKeyValue();
            if(first_key != 'N'){
    
    if(first_key >= 'A' && first_key <= 'D') {
        DL_UART_transmitDataBlocking(UART_FPGA_INST, first_key);
    }
    delay_ms(20); // 给 FPGA 一点时间切换状态机
                if(first_key == 'A')
                {
                    OLED_Clear();
                    OLED_state = 1;
                    sine_wave_show();
                }
                else if(first_key == 'B')
                {
                    OLED_Clear();
                    OLED_state = 2;
                    am_fc = 1000000;
                    am_ma = 10;
                    strcpy(am_freq_str, "1.0");
                    am_has_dot = 1;
                    AM_show();
                    OLED_ShowString(24, 2, am_freq_str);
                    OLED_ShowNum(24, 4, am_ma, 3, 16);
                    UART_send_freq(am_fc);
                    delay_ms(20);
                    UART_send_freq(am_ma);
                }
                else if(first_key == 'C')
                {
                    OLED_Clear();
                    OLED_state = 3;
                    FM_show();
                }
                else if(first_key == 'D')
                {
                    OLED_Clear();
                    OLED_state = 4;
                    PSK_ASK_show();
                }


                delay_ms(50);
                while(getKeyValue() != 'N') delay_ms(10);
            }
        }

        // ------------------ 正弦波界面 (State 1) ------------------
        else if(OLED_state == 1){
            key = getKeyValue();
            if(key != 'N')
            {
                if(key >= '0' && key <= '9')
                {
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
                        case 'A': OLED_ShowString(54,0,"Hz "); uart_freq = temp_freq; break;
                        case 'B': OLED_ShowString(54,0,"kHz"); uart_freq = temp_freq * 1000; break;
                        case 'C': OLED_ShowString(54,0,"MHz"); uart_freq = temp_freq * 1000000; break;
                    }
                    temp_freq = 0;
                    UART_send_freq(uart_freq);
                }
                else if(key == '*')
                {
                    uart_freq = uart_freq + 100;
                    OLED_ShowNum(0,0,uart_freq,6,16);
                    UART_send_freq(uart_freq);
                }
                else if(key == '#')
                {
                    if(uart_freq >= 100) uart_freq = uart_freq - 100;
                    OLED_ShowNum(0,0,uart_freq,6,16);
                    UART_send_freq(uart_freq);
                }
                // 注意：'D' 键由底部的通用逻辑处理返回主界面

                delay_ms(50);
                while(getKeyValue() != 'N') delay_ms(10);
            }
        }

        // ------------------ AM 界面 (State 2) ------------------
//        else if(OLED_state == 2)
//        {
//            key = getKeyValue();
//            if(key != 'N')
//            {
//                if(key >= '0' && key <= '9')
//                {
//                    uint8_t len = strlen(am_freq_str);
//                    if(len < 4)
//                    {
//                        uint8_t can_add = 1;
//                        if(am_has_dot)
//                        {
//                            uint8_t dot_pos = 0;
//                            while(am_freq_str[dot_pos] != '.' && dot_pos < len) dot_pos++;
//                            if(len - dot_pos - 1 >= 1) can_add = 0;
//                        }
//                        else
//                        {
//                            int tmp_val = 0;
//                            for(int i = 0; i < len; i++) tmp_val = tmp_val * 10 + (am_freq_str[i] - '0');
//                            tmp_val = tmp_val * 10 + (key - '0');
//                            if(tmp_val > 10) can_add = 0;
//                        }

//                        if(can_add)
//                        {
//                            am_freq_str[len] = key;
//                            am_freq_str[len + 1] = '\0';
//                            OLED_ShowString(24, 2, am_freq_str);
//                            OLED_ShowString(24 + (len + 1) * 8, 2, " ");
//                        }
//                    }
//                }
//                else if(key == 'A') // 小数点
//                {
//                    if(!am_has_dot && strlen(am_freq_str) > 0)
//                    {
//                        uint8_t len = strlen(am_freq_str);
//                        if(!(len == 2 && am_freq_str[0] == '1' && am_freq_str[1] == '0'))
//                        {
//                            am_has_dot = 1;
//                            am_freq_str[len] = '.';
//                            am_freq_str[len + 1] = '\0';
//                            OLED_ShowString(24, 2, am_freq_str);
//                        }
//                    }
//                }
//                else if(key == 'C') // 确认
//                {
//                    uint32_t integer_part = 0, decimal_part = 0;
//                    uint8_t is_decimal = 0, dec_digits = 0;
//                    for(int i = 0; am_freq_str[i] != '\0'; i++)
//                    {
//                        if(am_freq_str[i] == '.') is_decimal = 1;
//                        else {
//                            uint8_t digit = am_freq_str[i] - '0';
//                            if(!is_decimal) integer_part = integer_part * 10 + digit;
//                            else { decimal_part = decimal_part * 10 + digit; dec_digits++; }
//                        }
//                    }
//                    am_fc = integer_part * 1000000;
//                    if(dec_digits == 1) am_fc += decimal_part * 100000;
//                    else if(dec_digits == 2) am_fc += decimal_part * 10000;

//                    if(am_fc < 1000000) am_fc = 1000000;
//                    if(am_fc > 10000000) am_fc = 10000000;

//                    UART_send_freq(am_fc);
//                    delay_ms(20);
//                    UART_send_freq(am_ma);
//                    am_freq_str[0] = '\0';
//                    am_has_dot = 0;
//                }
//                else if(key == '*') // ma +10%
//                {
//                    if(am_ma < 100) am_ma += 10;
//                    OLED_ShowNum(24, 4, am_ma, 3, 16);
//                    UART_send_freq(am_fc);
//                    delay_ms(20);
//                    UART_send_freq(am_ma);
//                }
//                else if(key == '#') // ma -10%
//                {
//                    if(am_ma > 10) am_ma -= 10;
//                    OLED_ShowNum(24, 4, am_ma, 3, 16);
//                    UART_send_freq(am_fc);
//                    delay_ms(20);
//                    UART_send_freq(am_ma);
//                }

//                delay_ms(50);
//                while(getKeyValue() != 'N') delay_ms(10);
//            }
//        }
// ====================== 状态 2：AM 界面逻辑 ======================
        else if(OLED_state == 2)
        {
            key = getKeyValue();
            if(key != 'N')
            {
                // 如果当前显示的是默认的 "1.0" 且用户开始输入数字，则清空缓存开始新输入
                if(strcmp(am_freq_str, "1.0") == 0 && (key >= '0' && key <= '9')) {
                    am_freq_str[0] = '\0';
                    am_has_dot = 0;
                    OLED_ShowString(24, 2, "    "); // 清除屏幕上的 "1.0"
                }

                if(key >= '0' && key <= '9')
                {
                    uint8_t len = strlen(am_freq_str);
                    if(len < 4) // 限制总长度（如 "10.0" 是4位）
                    {
                        uint8_t can_add = 1;
                        if(am_has_dot) {
                            // 已有小数点，限制只能有1位小数
                            char *dot_ptr = strchr(am_freq_str, '.');
                            if(strlen(dot_ptr) > 1) can_add = 0;
                        } else{ // 不用 atoi 版本，手动计算数字值
int val = 0;
if (len > 0) {
    val = am_freq_str[0] - '0';          // 第一位转数字
    if (len >= 2) {
        val = val * 10 + (am_freq_str[1] - '0'); // 有第二位就组合
    }
}

// 原来的逻辑完全不变
if(val >= 10 && key != '0') can_add = 0;     // 已经 >=10，不能再加
if(val == 1 && len == 1 && key > '0') can_add = 1; // 允许 1.x
}
                        if(can_add) {
                            am_freq_str[len] = key;
                            am_freq_str[len + 1] = '\0';
                            // 更新 OLED 显示
                            OLED_ShowString(24, 2, "    "); // 先清空 4 个字符位
                            OLED_ShowString(24, 2, am_freq_str);
                        }
                    }
                }
                else if(key == 'A') // 小数点逻辑
                {
                    uint8_t len = strlen(am_freq_str);
                    if(!am_has_dot && len > 0 && len < 3) {
                        am_freq_str[len] = '.';
                        am_freq_str[len + 1] = '\0';
                        am_has_dot = 1;
                        OLED_ShowString(24, 2, am_freq_str);
                    }
                }
                else if(key == 'C') // 确认逻辑
                {
                    if(strlen(am_freq_str) > 0) {
                        // 1. 解析并计算频率 (MHz -> Hz)
                        float f_val;
                        sscanf(am_freq_str, "%f", &f_val);
                        am_fc = (uint32_t)(f_val * 1000000);

                        // 2. 发送给 FPGA
                        UART_send_freq(am_fc);
                        delay_ms(50);
                        UART_send_freq(am_ma);

                        // 3. 屏幕反馈：在 MHz (坐标72,2) 后面显示 OK
                        OLED_ShowString(96, 2, "OK"); 
                        delay_ms(500);           // 亮 0.5 秒
                        OLED_ShowString(96, 2, "  "); // 清除 OK

                        // 4. 重置输入状态，方便下次直接输入
                        am_freq_str[0] = '\0';
                        am_has_dot = 0;
                    }
                }
                else if(key == '*' || key == '#') // 调节调制度 ma
                {
                    if(key == '*' && am_ma < 100) am_ma += 10;
                    if(key == '#' && am_ma > 0)   am_ma -= 10;
                    
                    OLED_ShowNum(24, 4, am_ma, 3, 16);
                    UART_send_freq(am_fc);
                    delay_ms(20);
                    UART_send_freq(am_ma);
                }

                delay_ms(50);
                while(getKeyValue() != 'N') delay_ms(10);
            }
        }
        // ------------------ FM 界面 (State 3) ------------------
        else if(OLED_state == 3)
        {
            key = getKeyValue();
            if(key != 'N')
            {
                if(key >= '0' && key <= '9')
                {
                    if(temp_freq < 10000000) {
                        temp_freq = temp_freq * 10 + (key - '0');
                        OLED_ShowNum(24,2,temp_freq,6,16);
                    }
                }
                else if(key == 'A' || key == 'B' || key == 'C')
                {
                    switch(key)
                    {
                        case 'A': uart_freq = temp_freq; break;
                        case 'B': uart_freq = temp_freq * 1000; break;
                        case 'C': uart_freq = temp_freq * 1000000; break;
                    }
                    temp_freq = 0;
                    UART_send_freq(uart_freq);
										delay_ms(50);
                }
                else if(key == '*')
                {
                    if(fm_deviation == 5000) {
                        fm_deviation = 10000;
                        OLED_ShowString(48,4,"10kHz");
                    } else {
                        fm_deviation = 5000;
                        OLED_ShowString(48,4,"5kHz ");
                    }
                    UART_send_freq(fm_deviation);
                }

                delay_ms(50);
                while(getKeyValue() != 'N') delay_ms(10);
            }
        }

        // ------------------ 通用返回主界面逻辑 ------------------
        if(OLED_state != 0) 
        {
            // 此时如果在模式中按下 'D'，则退出到主界面
            if(getKeyValue() == 'D') 
            {
                OLED_Clear();        
                OLED_state = 0;      
                main_show();         
                temp_freq = 0;       // 清除临时频率，防止切回正弦波时数据残留
                delay_ms(50);
                while(getKeyValue() != 'N') delay_ms(10);
            }
        }
    }
}