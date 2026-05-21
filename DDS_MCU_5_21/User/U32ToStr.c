#include "U32ToStr.h"

// 无符号长整型转字符串
static void U32ToStr(uint32_t num, char* str)
  {
      int i = 0;
      char tmp[12];

      if(num == 0) {
          str[0] = '0';
          str[1] = '\0';
          return;
      }

      while(num > 0) {
          tmp[i++] = '0' + (num % 10);
          num /= 10;
      }

      // 反转
      for(int j = 0; j < i; j++) {
          str[j] = tmp[i - 1 - j];
      }
      str[i] = '\0';
  }