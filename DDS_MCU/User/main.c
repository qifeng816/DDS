/*------------------------------------------------------------------------------
 * File Name: DDS_up_control_MCU
 * Description: FPGA_DDS_CONTROLLER
 *----------------------------------------------------------------------------*/

#include "ti_msp_dl_config.h"
#include "bsp.h"
#include <stdio.h>
#include <string.h>

uint32_t fm_deviation = 5000;   // FM default deviation 5kHz
uint32_t fm_fc = 100000;        // FM default carrier 100kHz

// Convert frequency to string and send to FPGA
void UART_send_freq(uint32_t freq) {
    char str[12];
    int len = sprintf(str, "%u\n", freq);
    for (int i = 0; i < len; i++) {
        DL_UART_transmitDataBlocking(UART_FPGA_INST, str[i]);
    }
}

// Send AM params: 'B' resets FPGA idx_cnt so data_idx stays correct
void UART_send_am_params(uint32_t fc, uint32_t ma) {
    DL_UART_transmitDataBlocking(UART_FPGA_INST, 'B');
    delay_ms(20);
    UART_send_freq(fc);
    delay_ms(50);
    UART_send_freq(ma);
}

// Send FM params: 'C' resets FPGA idx_cnt, then send fc and deviation
void UART_send_fm_params(uint32_t fc, uint32_t dev) {
    DL_UART_transmitDataBlocking(UART_FPGA_INST, 'C');
    delay_ms(20);
    UART_send_freq(fc);
    delay_ms(50);
    UART_send_freq(dev);
}

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
    char sine_unit = 'A';  // Sine wave display unit: A=Hz, B=kHz, C=MHz

    // AM params
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
        // ------------------ Main Menu (State 0) ------------------
        if(OLED_state == 0)
        {
            first_key = getKeyValue();
            if(first_key != 'N')
            {
                if(first_key >= 'A' && first_key <= 'D') {
                    DL_UART_transmitDataBlocking(UART_FPGA_INST, first_key);
                    delay_ms(20);
                    if(first_key == 'A')
                    {
                        OLED_Clear();
                        OLED_state = 1;
                        sine_unit = 'A';
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
                        UART_send_am_params(am_fc, am_ma);
                    }
                    else if(first_key == 'C')
                    {
                        OLED_Clear();
                        OLED_state = 3;
                        fm_fc = 100000;
                        fm_deviation = 5000;
                        temp_freq = 0;
                        FM_show();
                        UART_send_fm_params(fm_fc, fm_deviation);
                    }
                    else if(first_key == 'D')
                    {
                        OLED_Clear();
                        OLED_state = 4;
                        PSK_ASK_show();
                    }
                    delay_ms(50);
                }
            }
        }

        // ------------------ Sine Wave (State 1) ------------------
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
                    sine_unit = key;
                    temp_freq = 0;
                    UART_send_freq(uart_freq);
                }
                else if(key == '*')
                {
                    uart_freq = uart_freq + 100;
                    OLED_ShowNum(0,0,uart_freq,6,16);
                    OLED_ShowString(54,0,"Hz ");
                    sine_unit = 'A';
                    UART_send_freq(uart_freq);
                }
                else if(key == '#')
                {
                    if(uart_freq >= 100) uart_freq = uart_freq - 100;
                    OLED_ShowNum(0,0,uart_freq,6,16);
                    OLED_ShowString(54,0,"Hz ");
                    sine_unit = 'A';
                    UART_send_freq(uart_freq);
                }

                delay_ms(50);
                while(getKeyValue() != 'N') delay_ms(10);
            }
        }

        // ====================== State 2: AM logic ======================
        else if(OLED_state == 2)
        {
            key = getKeyValue();
            if(key != 'N')
            {
                if(strcmp(am_freq_str, "1.0") == 0 && (key >= '0' && key <= '9')) {
                    am_freq_str[0] = '\0';
                    am_has_dot = 0;
                    OLED_ShowString(24, 2, "    ");
                }

                if(key >= '0' && key <= '9')
                {
                    uint8_t len = strlen(am_freq_str);
                    if(len < 4)
                    {
                        uint8_t can_add = 1;
                        if(am_has_dot) {
                            char *dot_ptr = strchr(am_freq_str, '.');
                            if(strlen(dot_ptr) > 1) can_add = 0;
                        } else {
                            int val = 0;
                            if (len > 0) {
                                val = am_freq_str[0] - '0';
                                if (len >= 2) {
                                    val = val * 10 + (am_freq_str[1] - '0');
                                }
                            }
                            if(val >= 10 && key != '0') can_add = 0;
                            if(val == 1 && len == 1 && key > '0') can_add = 1;
                        }
                        if(can_add) {
                            am_freq_str[len] = key;
                            am_freq_str[len + 1] = '\0';
                            OLED_ShowString(24, 2, "    ");
                            OLED_ShowString(24, 2, am_freq_str);
                        }
                    }
                }
                else if(key == 'A')
                {
                    uint8_t len = strlen(am_freq_str);
                    if(!am_has_dot && len > 0 && len < 3) {
                        am_freq_str[len] = '.';
                        am_freq_str[len + 1] = '\0';
                        am_has_dot = 1;
                        OLED_ShowString(24, 2, am_freq_str);
                    }
                }
                else if(key == 'C')
                {
                    if(strlen(am_freq_str) > 0) {
                        float f_val;
                        sscanf(am_freq_str, "%f", &f_val);
                        am_fc = (uint32_t)(f_val * 1000000);
                        UART_send_am_params(am_fc, am_ma);
                        OLED_ShowString(96, 2, "OK");
                        delay_ms(500);
                        OLED_ShowString(96, 2, "  ");
                        am_freq_str[0] = '\0';
                        am_has_dot = 0;
                    }
                }
                else if(key == '*' || key == '#')
                {
                    if(key == '*' && am_ma < 100) am_ma += 10;
                    if(key == '#' && am_ma > 0)   am_ma -= 10;
                    OLED_ShowNum(24, 4, am_ma, 3, 16);
                    UART_send_am_params(am_fc, am_ma);
                }

                delay_ms(50);
                while(getKeyValue() != 'N') delay_ms(10);
            }
        }

        // ------------------ FM Mode (State 3) ------------------
        else if(OLED_state == 3)
        {
            key = getKeyValue();

            if(key != 'N')
            {
                if(key >= '0' && key <= '9')
                {
                    if(temp_freq < 10000000) {
                        temp_freq = temp_freq * 10 + (key - '0');
                        OLED_ShowString(24, 2, "           ");
                        OLED_ShowNum(24, 2, temp_freq, 6, 16);
                    }
                }
                else if((temp_freq > 0) && (key == 'A' || key == 'B'||key == 'C'))
                {
                    switch(key)
                    {
                        case 'A': fm_fc = temp_freq; break;
                        case 'B': fm_fc = temp_freq * 1000; break;
											  case 'C': fm_fc = temp_freq * 1000000; break;
                    }
                    OLED_ShowString(24, 2, "           ");
                    OLED_ShowNum(24, 2, temp_freq, 6, 16);
                    switch(key)
                    {
                        case 'A': OLED_ShowString(72, 2, "Hz  "); break;
                        case 'B': OLED_ShowString(72, 2, "kHz "); break;
												case 'C': OLED_ShowString(72, 2, "MHz "); break;
                    }
                    temp_freq = 0;
                    UART_send_fm_params(fm_fc, fm_deviation);
                }
                else if(key == '*')
                {
                    if(fm_deviation == 5000) {
                        fm_deviation = 10000;
                        OLED_ShowString(48, 4, "10kHz");
                    } else {
                        fm_deviation = 5000;
                        OLED_ShowString(48, 4, "5kHz ");
                    }
                    UART_send_fm_params(fm_fc, fm_deviation);
                }

                delay_ms(50);
                while(getKeyValue() != 'N') delay_ms(10);
            }
        }

        // ------------------ Global exit logic ------------------
        if(OLED_state != 0)
        {
            if(getKeyValue() == 'D')
            {
                OLED_Clear();
                OLED_state = 0;
                main_show();
                temp_freq = 0;
                delay_ms(50);
                while(getKeyValue() != 'N') delay_ms(10);
            }
        }
    }
}
