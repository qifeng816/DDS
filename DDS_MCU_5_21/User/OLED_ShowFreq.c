#include "OLED_ShowFreq.h"

static void OLED_ShowFreq(unsigned long f)
  {
      char buf[32] = {0};
      char numStr[16];
      uint32_t displayNum = f;
      const char* unit = "Hz";

      // 如果能整除，用kHz/MHz显示，否则显示完整Hz
      if(f >= 1000000 && (f % 1000000) == 0) {
          displayNum = f / 1000000;
          unit = "MHz";
      }
      else if(f >= 1000 && (f % 1000) == 0) {
          displayNum = f / 1000;
          unit = "kHz";
      }

      U32ToStr(displayNum, numStr);

      // 手动拼接字符串
      StrAppend(buf, numStr);
      StrAppend(buf, unit);

      OLED_ShowString(80, 6, buf);
  }