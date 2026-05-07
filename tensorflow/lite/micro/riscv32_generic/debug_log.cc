/* Copyright 2023 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

// #include "tensorflow/lite/micro/debug_log.h"

// #include <cstdio>

// #ifndef TF_LITE_STRIP_ERROR_STRINGS
// #include "eyalroz_printf/src/printf/printf.h"
// #endif

// extern "C" void DebugLog(const char* format, va_list args) {
// #ifndef TF_LITE_STRIP_ERROR_STRINGS
//   constexpr int kMaxLogLen = 256;
//   char log_buffer[kMaxLogLen];

//   vsnprintf_(log_buffer, kMaxLogLen, format, args);
//   std::fputs(log_buffer, stdout);
// #endif
// }

// #ifndef TF_LITE_STRIP_ERROR_STRINGS
// // Only called from MicroVsnprintf (micro_log.h)
// extern "C" int DebugVsnprintf(char* buffer, size_t buf_size, const char* format,
//                               va_list vlist) {
//   return vsnprintf_(buffer, buf_size, format, vlist);
// }
// #endif


// #include <cstdarg>
// #include <cstddef>
// #include <cstdio>
// #include <stdint.h>

// namespace {
// inline void UartPutc(char c) {
//   volatile uint32_t* const uart_tx_fifo =
//       reinterpret_cast<volatile uint32_t*>(0x40600000 + 0x04);
//   volatile uint32_t* const uart_status =
//       reinterpret_cast<volatile uint32_t*>(0x40600000 + 0x08);

//   while ((*uart_status) & 0x08) {}
//   *uart_tx_fifo = static_cast<uint32_t>(static_cast<uint8_t>(c));
// }

// inline void UartWriteString(const char* s) {
//   if (!s) return;
//   while (*s) {
//     if (*s == '\n') UartPutc('\r');
//     UartPutc(*s++);
//   }
// }
// }  // namespace

// extern "C" void DebugLog(const char* format, va_list args) {
// #ifndef TF_LITE_STRIP_ERROR_STRINGS
//   char buffer[256];
//   vsnprintf(buffer, sizeof(buffer), format, args);
//   UartWriteString(buffer);
// #endif
// }

// #ifndef TF_LITE_STRIP_ERROR_STRINGS
// extern "C" int DebugVsnprintf(char* buffer, size_t buf_size,
//                               const char* format, va_list vlist) {
//   return vsnprintf(buffer, buf_size, format, vlist);
// }
// #endif

// #include "tensorflow/lite/micro/debug_log.h"

// #ifndef TF_LITE_STRIP_ERROR_STRINGS
// #include <cstdarg>
// #include <cstddef>
// #include <cstdio>
// #include <stdint.h>
// #endif

// namespace {
// inline void UartPutc(char c) {
//   volatile uint32_t* const uart_tx_fifo =
//       reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
//   volatile uint32_t* const uart_status =
//       reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x08u);

//   while ((*uart_status) & 0x08u) {
//   }

//   *uart_tx_fifo = static_cast<uint32_t>(static_cast<uint8_t>(c));
// }

// inline void UartWriteString(const char* s) {
//   if (!s) return;
//   while (*s) {
//     if (*s == '\n') UartPutc('\r');
//     UartPutc(*s++);
//   }
// }
// }  // namespace

// extern "C" void DebugLog(const char* format, va_list args) {
// #ifndef TF_LITE_STRIP_ERROR_STRINGS
//   char buffer[256];
//   vsnprintf(buffer, sizeof(buffer), format, args);
//   UartWriteString(buffer);
// #endif
// }

// #ifndef TF_LITE_STRIP_ERROR_STRINGS
// extern "C" int DebugVsnprintf(char* buffer, size_t buf_size,
//                               const char* format, va_list vlist) {
//   return vsnprintf(buffer, buf_size, format, vlist);
// }
// #endif


// #include "tensorflow/lite/micro/debug_log.h"

// #ifndef TF_LITE_STRIP_ERROR_STRINGS
// #include <cstdarg>
// #include <cstddef>
// #include <cstdio>
// #include <stdint.h>
// #endif

// namespace {
// inline void UartPutcRaw(char c) {
//   volatile uint32_t* const uart_tx_fifo =
//       reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
//   *uart_tx_fifo = static_cast<uint32_t>(static_cast<uint8_t>(c));
// }

// inline void UartWriteStringRaw(const char* s) {
//   if (!s) return;
//   while (*s) {
//     if (*s == '\n') UartPutcRaw('\r');
//     UartPutcRaw(*s++);
//   }
// }
// }  // namespace

// extern "C" void DebugLog(const char* format, va_list args) {
// #ifndef TF_LITE_STRIP_ERROR_STRINGS
//   char buffer[256];
//   vsnprintf(buffer, sizeof(buffer), format, args);
//   UartWriteStringRaw(buffer);
// #endif
// }

// #ifndef TF_LITE_STRIP_ERROR_STRINGS
// extern "C" int DebugVsnprintf(char* buffer, size_t buf_size,
//                               const char* format, va_list vlist) {
//   return vsnprintf(buffer, buf_size, format, vlist);
// }
// #endif


#include "tensorflow/lite/micro/debug_log.h"

#include <cstdarg>
#include <cstddef>
#include <stdint.h>

namespace {

constexpr uint32_t UART_BASE_ADDR   = 0x40600000u;
constexpr uint32_t UART_TX_OFFSET   = 0x04u;
constexpr uint32_t UART_STAT_OFFSET = 0x08u;
constexpr uint32_t UART_TX_FULL_BIT = 0x08u;

inline void UartPutcRaw(char c) {
  volatile uint32_t* const uart_tx_fifo =
      reinterpret_cast<volatile uint32_t*>(UART_BASE_ADDR + UART_TX_OFFSET);

  volatile uint32_t* const uart_status =
      reinterpret_cast<volatile uint32_t*>(UART_BASE_ADDR + UART_STAT_OFFSET);

  while ((*uart_status) & UART_TX_FULL_BIT) {
  }

  *uart_tx_fifo = static_cast<uint32_t>(static_cast<uint8_t>(c));
}

inline void UartWriteStringRaw(const char* s) {
  if (s == nullptr) return;

  while (*s) {
    if (*s == '\n') {
      UartPutcRaw('\r');
    }
    UartPutcRaw(*s++);
  }
}

inline bool HasFormatSpecifier(const char* s) {
  if (s == nullptr) return false;

  while (*s) {
    if (*s == '%') {
      return true;
    }
    ++s;
  }

  return false;
}

inline void UartHexNibble(uint32_t v) {
  v &= 0xFu;
  UartPutcRaw((v < 10u) ? static_cast<char>('0' + v)
                        : static_cast<char>('A' + (v - 10u)));
}

inline void UartHex32(uint32_t v) {
  for (int shift = 28; shift >= 0; shift -= 4) {
    UartHexNibble(v >> shift);
  }
}

inline void UartDecUnsigned(uint32_t v) {
  char buf[11];
  int i = 0;

  if (v == 0) {
    UartPutcRaw('0');
    return;
  }

  while (v != 0 && i < 10) {
    buf[i++] = static_cast<char>('0' + (v % 10u));
    v /= 10u;
  }

  while (i > 0) {
    UartPutcRaw(buf[--i]);
  }
}

inline void UartDecSigned(int32_t v) {
  if (v < 0) {
    UartPutcRaw('-');

    // Safe even for INT32_MIN.
    uint32_t mag = static_cast<uint32_t>(-(v + 1)) + 1u;
    UartDecUnsigned(mag);
  } else {
    UartDecUnsigned(static_cast<uint32_t>(v));
  }
}

// Small safe formatter for the common MicroPrintf cases.
// Supports: %d, %u, %x, %08x, %s, %c, %%
// Small safe formatter for the common MicroPrintf cases.
// Supports: %d, %u, %x, %X, %08x, %08X, %p, %s, %c, %%
inline void UartTinyVprintf(const char* fmt, va_list args) {
  if (fmt == nullptr) return;

  while (*fmt) {
    if (*fmt != '%') {
      if (*fmt == '\n') UartPutcRaw('\r');
      UartPutcRaw(*fmt++);
      continue;
    }

    ++fmt;

    if (*fmt == '\0') {
      UartPutcRaw('%');
      break;
    }

    if (*fmt == '%') {
      UartPutcRaw('%');
      ++fmt;
      continue;
    }

    bool zero_pad = false;
    int width = 0;

    if (*fmt == '0') {
      zero_pad = true;
      ++fmt;
    }

    while (*fmt >= '0' && *fmt <= '9') {
      width = width * 10 + (*fmt - '0');
      ++fmt;
    }

    switch (*fmt) {
      case 'd': {
        int32_t v = va_arg(args, int32_t);
        UartDecSigned(v);
        break;
      }

      case 'u': {
        uint32_t v = va_arg(args, uint32_t);
        UartDecUnsigned(v);
        break;
      }

      case 'x':
      case 'X': {
        uint32_t v = va_arg(args, uint32_t);

        // For now, all hex prints are 8 digits.
        // This matches your RawTagHex style and keeps parsing easy.
        (void)zero_pad;
        (void)width;
        UartHex32(v);
        break;
      }

      case 'p': {
        void* ptr = va_arg(args, void*);
        uintptr_t v = reinterpret_cast<uintptr_t>(ptr);

        UartWriteStringRaw("0x");
        UartHex32(static_cast<uint32_t>(v));
        break;
      }

      case 's': {
        const char* s = va_arg(args, const char*);
        UartWriteStringRaw(s ? s : "(null)");
        break;
      }

      case 'c': {
        char c = static_cast<char>(va_arg(args, int));
        UartPutcRaw(c);
        break;
      }

      default: {
        // Unknown specifier. Print it literally.
        // Do not consume va_arg here because we do not know its type.
        UartPutcRaw('%');
        UartPutcRaw(*fmt);
        break;
      }
    }

    if (*fmt) {
      ++fmt;
    }
  }
}

}  // namespace

extern "C" void DebugLog(const char* format, va_list args) {
#ifndef TF_LITE_STRIP_ERROR_STRINGS
  if (format == nullptr) {
    return;
  }

  // Important:
  // Plain MicroPrintf("Hello\n") bypasses vsnprintf completely.
  // This is much safer on your custom core.
  if (!HasFormatSpecifier(format)) {
    UartWriteStringRaw(format);
    return;
  }

  // For formatted strings, use a small formatter instead of libc vsnprintf.
  UartTinyVprintf(format, args);

#else
  (void)format;
  (void)args;
#endif
}

#ifndef TF_LITE_STRIP_ERROR_STRINGS
extern "C" int DebugVsnprintf(char* buffer,
                              size_t buf_size,
                              const char* format,
                              va_list vlist) {
  // Avoid pulling in full vsnprintf for now.
  // This function is only here because some TFLM code may expect the symbol.
  (void)vlist;

  if (buffer == nullptr || buf_size == 0) {
    return 0;
  }

  size_t i = 0;
  if (format != nullptr) {
    while (format[i] && i + 1 < buf_size) {
      buffer[i] = format[i];
      ++i;
    }
  }

  buffer[i] = '\0';
  return static_cast<int>(i);
}
#endif