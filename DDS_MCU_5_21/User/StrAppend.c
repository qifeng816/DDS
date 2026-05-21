#include "StrAppend.h"

// ×Ö·û´®×·¼Ó
  static void StrAppend(char* dst, const char* src)
  {
      while(*dst) dst++;           // ÕÒµ½½áÎ²
      while(*src) {
          *dst = *src;
          dst++;
          src++;
      }
      *dst = '\0';
  }