/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

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

// #include "tensorflow/lite/micro/examples/person_detection/main_functions.h"

// // This is the default main used on systems that have the standard C entry
// // point. Other devices (for example FreeRTOS or ESP32) that have different
// // requirements for entry code (like an app_main function) should specialize
// // this main.cc file in a target-specific subfolder.
// int main(int argc, char* argv[]) {
//   setup();
//   while (true) {
//     loop();
//   }
// }


#include "tensorflow/lite/micro/examples/person_detection/main_functions.h"

// extern "C" int main(void) {
//   setup();
//   while (1) {
//     loop();
//   }
//   return 0;
// }



// extern "C" int main(void) {
//   setup();
//   loop();   // run exactly once
//   while (1) {}
//   return 0;
// }

/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

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

// #include "tensorflow/lite/micro/examples/person_detection/main_functions.h"

// // This is the default main used on systems that have the standard C entry
// // point. Other devices (for example FreeRTOS or ESP32) that have different
// // requirements for entry code (like an app_main function) should specialize
// // this main.cc file in a target-specific subfolder.
// int main(int argc, char* argv[]) {
//   setup();
//   while (true) {
//     loop();
//   }
// }


#include "tensorflow/lite/micro/examples/person_detection/main_functions.h"

// extern "C" int main(void) {
//   setup();
//   while (1) {
//     loop();
//   }
//   return 0;
// }



// extern "C" int main(void) {
//   setup();
//   loop();   // run exactly once
//   while (1) {}
//   return 0;
// }

// #include <stdint.h>

// static constexpr uintptr_t UART_BASE = 0x40600000;
// static constexpr uintptr_t QSPI_BASE = 0x44A00000;

// static constexpr uint32_t REG_SRR    = 0x40;
// static constexpr uint32_t REG_SPICR  = 0x60;
// static constexpr uint32_t REG_SPISR  = 0x64;
// static constexpr uint32_t REG_DTR    = 0x68;
// static constexpr uint32_t REG_DRR    = 0x6C;
// static constexpr uint32_t REG_SSR    = 0x70;

// static constexpr uint32_t CR_SPE    = (1u << 1);
// static constexpr uint32_t CR_MASTER = (1u << 2);
// static constexpr uint32_t CR_TXRST  = (1u << 5);
// static constexpr uint32_t CR_RXRST  = (1u << 6);
// static constexpr uint32_t CR_MT_INH = (1u << 8);

// static constexpr uint32_t SR_RX_EMPTY = (1u << 0);
// static constexpr uint32_t SR_TX_EMPTY = (1u << 2);

// static constexpr uint32_t SSR_ASSERT_SLAVE0 = 0x00000000;
// static constexpr uint32_t SSR_DEASSERT_ALL  = 0x00000001;

// static constexpr uint32_t SPICR_IDLE = CR_SPE | CR_MASTER | CR_MT_INH;
// static constexpr uint32_t SPICR_GO   = CR_SPE | CR_MASTER;

// static constexpr uint8_t CMD_READ = 0x03;

// static inline uint32_t bswap32(uint32_t x) {
//   return ((x & 0x000000FFu) << 24) |
//          ((x & 0x0000FF00u) <<  8) |
//          ((x & 0x00FF0000u) >>  8) |
//          ((x & 0xFF000000u) >> 24);
// }

// static inline uint32_t mmio_read32(uintptr_t addr) {
//   return *reinterpret_cast<volatile uint32_t*>(addr);
// }

// static inline void mmio_write32(uintptr_t addr, uint32_t value) {
//   *reinterpret_cast<volatile uint32_t*>(addr) = value;
// }

// static inline uint32_t qspi_read32(uintptr_t addr) {
//   return bswap32(mmio_read32(addr));
// }

// static inline void qspi_write32(uintptr_t addr, uint32_t value) {
//   mmio_write32(addr, bswap32(value));
// }

// static inline void uart_putc(char c) {
//   volatile uint32_t* status =
//       reinterpret_cast<volatile uint32_t*>(UART_BASE + 0x08);
//   volatile uint8_t* txfifo =
//       reinterpret_cast<volatile uint8_t*>(UART_BASE + 0x04);

//   while ((*status) & 0x08u) {
//   }

//   *txfifo = static_cast<uint8_t>(c);
// }

// static void uart_puts(const char* s) {
//   while (*s) {
//     uart_putc(*s++);
//   }
// }

// static void uart_puthex8(uint8_t v) {
//   const char* hex = "0123456789ABCDEF";
//   uart_putc(hex[(v >> 4) & 0xF]);
//   uart_putc(hex[v & 0xF]);
// }

// static void uart_puthex32(uint32_t v) {
//   const char* hex = "0123456789ABCDEF";
//   for (int i = 7; i >= 0; --i) {
//     uart_putc(hex[(v >> (i * 4)) & 0xF]);
//   }
// }

// static void uart_newline() {
//   uart_puts("\r\n");
// }

// static void qspi_init_only() {
//   qspi_write32(QSPI_BASE + REG_SRR, 0x0000000A);

//   qspi_write32(QSPI_BASE + REG_SSR, SSR_DEASSERT_ALL);

//   qspi_write32(QSPI_BASE + REG_SPICR,
//                SPICR_IDLE | CR_TXRST | CR_RXRST);

//   qspi_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE);
// }

// static void qspi_push_byte(uint8_t b) {
//   qspi_write32(QSPI_BASE + REG_DTR, b);
// }

// static uint8_t qspi_pop_byte() {
//   return static_cast<uint8_t>(qspi_read32(QSPI_BASE + REG_DRR) & 0xFF);
// }

// static void qspi_flush_rx_fifo() {
//   int guard = 1024;

//   while (((qspi_read32(QSPI_BASE + REG_SPISR) & SR_RX_EMPTY) == 0) &&
//          --guard) {
//     volatile uint8_t dummy = qspi_pop_byte();
//     (void)dummy;
//   }
// }

// static void qspi_read_burst(uint32_t flash_addr, uint8_t* out, int n) {
//   qspi_write32(QSPI_BASE + REG_SSR, SSR_DEASSERT_ALL);

//   qspi_write32(QSPI_BASE + REG_SPICR,
//                SPICR_IDLE | CR_TXRST | CR_RXRST);

//   qspi_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE);

//   qspi_flush_rx_fifo();

//   qspi_write32(QSPI_BASE + REG_SSR, SSR_ASSERT_SLAVE0);

//   qspi_push_byte(CMD_READ);
//   qspi_push_byte(static_cast<uint8_t>((flash_addr >> 16) & 0xFF));
//   qspi_push_byte(static_cast<uint8_t>((flash_addr >> 8) & 0xFF));
//   qspi_push_byte(static_cast<uint8_t>((flash_addr >> 0) & 0xFF));

//   for (int i = 0; i < n; ++i) {
//     qspi_push_byte(0x00);
//   }

//   qspi_write32(QSPI_BASE + REG_SPICR, SPICR_GO);

//   int txto = 100000;
//   while (((qspi_read32(QSPI_BASE + REG_SPISR) & SR_TX_EMPTY) == 0) &&
//          --txto) {
//   }

//   qspi_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE);
//   qspi_write32(QSPI_BASE + REG_SSR, SSR_DEASSERT_ALL);

//   // Drain command/address echo bytes.
//   for (int i = 0; i < 4; ++i) {
//     int rxto = 10000;
//     while ((qspi_read32(QSPI_BASE + REG_SPISR) & SR_RX_EMPTY) &&
//            --rxto) {
//     }

//     if (!rxto) {
//       out[0] = 0xE1;
//       return;
//     }

//     volatile uint8_t dummy = qspi_pop_byte();
//     (void)dummy;
//   }

//   // Read actual flash data bytes.
//   for (int i = 0; i < n; ++i) {
//     int rxto = 10000;
//     while ((qspi_read32(QSPI_BASE + REG_SPISR) & SR_RX_EMPTY) &&
//            --rxto) {
//     }

//     if (!rxto) {
//       out[i] = 0xE2;
//       return;
//     }

//     out[i] = qspi_pop_byte();
//   }
// }

// static void dump16_burst(uint32_t base) {
//   uint8_t buf[16];

//   for (int i = 0; i < 16; ++i) {
//     buf[i] = 0x00;
//   }

//   qspi_read_burst(base, buf, 16);

//   uart_puts("BURST ");
//   uart_puthex32(base);
//   uart_puts(": ");

//   for (int i = 0; i < 16; ++i) {
//     uart_puthex8(buf[i]);
//     uart_putc(' ');
//   }

//   uart_newline();
// }

// static uint8_t qspi_read_byte_old(uint32_t flash_addr) {
//   qspi_write32(QSPI_BASE + REG_SSR, SSR_ASSERT_SLAVE0);
//   qspi_write32(QSPI_BASE + REG_SPICR,
//                SPICR_IDLE | CR_TXRST | CR_RXRST);

//   qspi_push_byte(CMD_READ);
//   qspi_push_byte(static_cast<uint8_t>((flash_addr >> 16) & 0xFF));
//   qspi_push_byte(static_cast<uint8_t>((flash_addr >> 8) & 0xFF));
//   qspi_push_byte(static_cast<uint8_t>((flash_addr >> 0) & 0xFF));
//   qspi_push_byte(0x00);

//   qspi_write32(QSPI_BASE + REG_SPICR, SPICR_GO);

//   int timeout = 10000;
//   while (((qspi_read32(QSPI_BASE + REG_SPISR) & SR_TX_EMPTY) == 0) &&
//          --timeout) {
//   }

//   qspi_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE);
//   qspi_write32(QSPI_BASE + REG_SSR, SSR_DEASSERT_ALL);

//   uint8_t last = 0xFF;

//   for (int i = 0; i < 5; ++i) {
//     int rxto = 10000;
//     while ((qspi_read32(QSPI_BASE + REG_SPISR) & SR_RX_EMPTY) &&
//            --rxto) {
//     }

//     if (!rxto) {
//       return 0xEE;
//     }

//     last = qspi_pop_byte();
//   }

//   return last;
// }

// static void dump4_old(uint32_t base) {
//   uart_puts("OLD ");
//   uart_puthex32(base);
//   uart_puts(": ");

//   for (int i = 0; i < 4; ++i) {
//     uint8_t b = qspi_read_byte_old(base + i);
//     uart_puthex8(b);
//     uart_putc(' ');
//   }

//   uart_newline();
// }

// int main() {
//   uart_puts("QSPI BURST TEST START\r\n");

//   qspi_init_only();

//   uart_puts("Old single-byte reads:\r\n");
//   dump4_old(0x00100000);
//   dump4_old(0x00100001);

//   uart_puts("New burst reads:\r\n");
//   dump16_burst(0x00100000);
//   dump16_burst(0x00100001);
//   dump16_burst(0x00080000);
//   dump16_burst(0x00000000);

//   while (1) {
//   }

//   return 0;
// }


#include <stdint.h>
#include <stddef.h>

static constexpr uintptr_t UART_BASE = 0x40600000u;
static constexpr uintptr_t QSPI_BASE = 0x44A00000u;
static constexpr uintptr_t DDR_BASE  = 0x80000000u;

static constexpr uint32_t QSPI_FLASH_BASE = 0x00100000u;

static constexpr uint32_t REG_SRR    = 0x40u;
static constexpr uint32_t REG_SPICR  = 0x60u;
static constexpr uint32_t REG_SPISR  = 0x64u;
static constexpr uint32_t REG_DTR    = 0x68u;
static constexpr uint32_t REG_DRR    = 0x6Cu;
static constexpr uint32_t REG_SSR    = 0x70u;

static constexpr uint32_t CR_SPE    = (1u << 1);
static constexpr uint32_t CR_MASTER = (1u << 2);
static constexpr uint32_t CR_TXRST  = (1u << 5);
static constexpr uint32_t CR_RXRST  = (1u << 6);
static constexpr uint32_t CR_MT_INH = (1u << 8);

static constexpr uint32_t SR_RX_EMPTY = (1u << 0);
static constexpr uint32_t SR_TX_EMPTY = (1u << 2);

static constexpr uint32_t SSR_ASSERT_SLAVE0 = 0x00000000u;
static constexpr uint32_t SSR_DEASSERT_ALL  = 0x00000001u;

static constexpr uint32_t SPICR_IDLE = CR_SPE | CR_MASTER | CR_MT_INH;
static constexpr uint32_t SPICR_GO   = CR_SPE | CR_MASTER;

static constexpr uint8_t CMD_READ = 0x03u;

static inline uint32_t bswap32(uint32_t x) {
  return ((x & 0x000000FFu) << 24) |
         ((x & 0x0000FF00u) <<  8) |
         ((x & 0x00FF0000u) >>  8) |
         ((x & 0xFF000000u) >> 24);
}

static inline uint32_t mmio_read32(uintptr_t addr) {
  return *reinterpret_cast<volatile uint32_t*>(addr);
}

static inline void mmio_write32(uintptr_t addr, uint32_t value) {
  *reinterpret_cast<volatile uint32_t*>(addr) = value;
}

static inline uint32_t qspi_read32(uintptr_t addr) {
  return bswap32(mmio_read32(addr));
}

static inline void qspi_write32(uintptr_t addr, uint32_t value) {
  mmio_write32(addr, bswap32(value));
}

static inline void uart_putc(char c) {
  volatile uint32_t* status =
      reinterpret_cast<volatile uint32_t*>(UART_BASE + 0x08u);
  volatile uint8_t* txfifo =
      reinterpret_cast<volatile uint8_t*>(UART_BASE + 0x04u);

  while ((*status) & 0x08u) {
  }

  *txfifo = static_cast<uint8_t>(c);
}

static void uart_newline() {
  uart_putc('\r');
  uart_putc('\n');
}

static char hex_digit(uint8_t v) {
  v &= 0xFu;
  if (v < 10u) {
    return static_cast<char>('0' + v);
  }
  return static_cast<char>('A' + (v - 10u));
}

static void uart_puthex8(uint8_t v) {
  uart_putc(hex_digit(static_cast<uint8_t>(v >> 4)));
  uart_putc(hex_digit(v));
}

static void uart_puthex32(uint32_t v) {
  for (int shift = 28; shift >= 0; shift -= 4) {
    uart_putc(hex_digit(static_cast<uint8_t>(v >> shift)));
  }
}

static void tag_qspi() {
  uart_putc('Q');
  uart_putc('S');
  uart_putc('P');
  uart_putc('I');
  uart_putc(' ');
}

static void tag_ddr() {
  uart_putc('D');
  uart_putc('D');
  uart_putc('R');
  uart_putc(' ');
}

static void tag_word() {
  uart_putc('W');
  uart_putc('O');
  uart_putc('R');
  uart_putc('D');
  uart_putc(' ');
}

static void tag_match() {
  uart_putc('M');
  uart_putc('A');
  uart_putc('T');
  uart_putc('C');
  uart_putc('H');
  uart_putc(' ');
}

static void tag_mismatch() {
  uart_putc('M');
  uart_putc('I');
  uart_putc('S');
  uart_putc('M');
  uart_putc('A');
  uart_putc('T');
  uart_putc('C');
  uart_putc('H');
  uart_putc(' ');
}

static void qspi_init_only() {
  qspi_write32(QSPI_BASE + REG_SRR, 0x0000000Au);

  qspi_write32(QSPI_BASE + REG_SSR, SSR_DEASSERT_ALL);

  qspi_write32(QSPI_BASE + REG_SPICR,
               SPICR_IDLE | CR_TXRST | CR_RXRST);

  qspi_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE);
}

static void qspi_push_byte(uint8_t b) {
  qspi_write32(QSPI_BASE + REG_DTR, b);
}

static uint8_t qspi_pop_byte() {
  return static_cast<uint8_t>(qspi_read32(QSPI_BASE + REG_DRR) & 0xFFu);
}

static void qspi_flush_rx_fifo() {
  int guard = 1024;

  while (((qspi_read32(QSPI_BASE + REG_SPISR) & SR_RX_EMPTY) == 0u) &&
         --guard) {
    volatile uint8_t dummy = qspi_pop_byte();
    (void)dummy;
  }
}

// Safe 8-byte QSPI burst read.
// Total transfer = 4 command/address bytes + 8 dummy bytes = 12 bytes.
// This avoids the 16-byte FIFO timeout you observed.
static void qspi_read8(uint32_t flash_addr, uint8_t* out) {
  qspi_write32(QSPI_BASE + REG_SSR, SSR_DEASSERT_ALL);

  qspi_write32(QSPI_BASE + REG_SPICR,
               SPICR_IDLE | CR_TXRST | CR_RXRST);

  qspi_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE);

  qspi_flush_rx_fifo();

  qspi_write32(QSPI_BASE + REG_SSR, SSR_ASSERT_SLAVE0);

  qspi_push_byte(CMD_READ);
  qspi_push_byte(static_cast<uint8_t>((flash_addr >> 16) & 0xFFu));
  qspi_push_byte(static_cast<uint8_t>((flash_addr >> 8) & 0xFFu));
  qspi_push_byte(static_cast<uint8_t>((flash_addr >> 0) & 0xFFu));

  for (int i = 0; i < 8; ++i) {
    qspi_push_byte(0x00u);
  }

  qspi_write32(QSPI_BASE + REG_SPICR, SPICR_GO);

  int txto = 100000;
  while (((qspi_read32(QSPI_BASE + REG_SPISR) & SR_TX_EMPTY) == 0u) &&
         --txto) {
  }

  qspi_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE);
  qspi_write32(QSPI_BASE + REG_SSR, SSR_DEASSERT_ALL);

  // Drain 4 command/address response bytes.
  for (int i = 0; i < 4; ++i) {
    int rxto = 10000;
    while ((qspi_read32(QSPI_BASE + REG_SPISR) & SR_RX_EMPTY) &&
           --rxto) {
    }

    if (!rxto) {
      for (int j = 0; j < 8; ++j) {
        out[j] = 0xE1u;
      }
      return;
    }

    volatile uint8_t dummy = qspi_pop_byte();
    (void)dummy;
  }

  // Read 8 actual data bytes.
  for (int i = 0; i < 8; ++i) {
    int rxto = 10000;
    while ((qspi_read32(QSPI_BASE + REG_SPISR) & SR_RX_EMPTY) &&
           --rxto) {
    }

    if (!rxto) {
      for (int j = i; j < 8; ++j) {
        out[j] = 0xE2u;
      }
      return;
    }

    out[i] = qspi_pop_byte();
  }
}

static uint8_t read_ddr8(uint32_t offset) {
  volatile uint8_t* p =
      reinterpret_cast<volatile uint8_t*>(DDR_BASE + offset);
  return *p;
}

static uint32_t read_ddr32(uint32_t offset) {
  volatile uint32_t* p =
      reinterpret_cast<volatile uint32_t*>(DDR_BASE + offset);
  return *p;
}

static uint32_t pack_le32(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) {
  return ((uint32_t)b0 << 0)  |
         ((uint32_t)b1 << 8)  |
         ((uint32_t)b2 << 16) |
         ((uint32_t)b3 << 24);
}

static void dump_qspi8(uint32_t offset) {
  uint8_t q[8];

  for (int i = 0; i < 8; ++i) {
    q[i] = 0;
  }

  qspi_read8(QSPI_FLASH_BASE + offset, q);

  tag_qspi();
  uart_puthex32(QSPI_FLASH_BASE + offset);
  uart_putc(':');
  uart_putc(' ');

  for (int i = 0; i < 8; ++i) {
    uart_puthex8(q[i]);
    uart_putc(' ');
  }

  uart_newline();
}

static void dump_ddr8(uint32_t offset) {
  tag_ddr();
  uart_puthex32(DDR_BASE + offset);
  uart_putc(':');
  uart_putc(' ');

  for (int i = 0; i < 8; ++i) {
    uart_puthex8(read_ddr8(offset + static_cast<uint32_t>(i)));
    uart_putc(' ');
  }

  uart_newline();
}

static void compare8(uint32_t offset) {
  uint8_t q[8];
  int mismatch = 0;

  for (int i = 0; i < 8; ++i) {
    q[i] = 0;
  }

  qspi_read8(QSPI_FLASH_BASE + offset, q);

  for (int i = 0; i < 8; ++i) {
    uint8_t d = read_ddr8(offset + static_cast<uint32_t>(i));
    if (q[i] != d) {
      mismatch = 1;
    }
  }

  if (mismatch) {
    tag_mismatch();
  } else {
    tag_match();
  }

  uart_puthex32(offset);
  uart_newline();
}

static void dump_words(uint32_t offset) {
  uint8_t q[8];

  for (int i = 0; i < 8; ++i) {
    q[i] = 0;
  }

  qspi_read8(QSPI_FLASH_BASE + offset, q);

  uint32_t qword0 = pack_le32(q[0], q[1], q[2], q[3]);
  uint32_t qword1 = pack_le32(q[4], q[5], q[6], q[7]);

  uint32_t dword0 = read_ddr32(offset + 0u);
  uint32_t dword1 = read_ddr32(offset + 4u);

  tag_word();
  uart_puthex32(offset);
  uart_putc(' ');
  uart_putc('Q');
  uart_putc('0');
  uart_putc('=');
  uart_puthex32(qword0);
  uart_putc(' ');
  uart_putc('D');
  uart_putc('0');
  uart_putc('=');
  uart_puthex32(dword0);
  uart_newline();

  tag_word();
  uart_puthex32(offset + 4u);
  uart_putc(' ');
  uart_putc('Q');
  uart_putc('1');
  uart_putc('=');
  uart_puthex32(qword1);
  uart_putc(' ');
  uart_putc('D');
  uart_putc('1');
  uart_putc('=');
  uart_puthex32(dword1);
  uart_newline();
}

extern "C" int main() {
  uart_putc('S');
  uart_newline();

  qspi_init_only();

  uart_putc('Q');
  uart_newline();

  // These offsets correspond to:
  // QSPI address = 0x00100000 + offset
  // DDR address  = 0x80000000 + offset

  dump_qspi8(0x00000000u);
  dump_ddr8 (0x00000000u);
  compare8  (0x00000000u);
  dump_words(0x00000000u);

  dump_qspi8(0x00000008u);
  dump_ddr8 (0x00000008u);
  compare8  (0x00000008u);
  dump_words(0x00000008u);

  dump_qspi8(0x00000010u);
  dump_ddr8 (0x00000010u);
  compare8  (0x00000010u);
  dump_words(0x00000010u);

  dump_qspi8(0x00000100u);
  dump_ddr8 (0x00000100u);
  compare8  (0x00000100u);
  dump_words(0x00000100u);

  dump_qspi8(0x00001000u);
  dump_ddr8 (0x00001000u);
  compare8  (0x00001000u);
  dump_words(0x00001000u);

  uart_putc('E');
  uart_newline();

  while (1) {
  }

  return 0;
}



// #include <stdint.h>

// static constexpr uintptr_t UART_BASE = 0x40600000;   // adjust if needed
// static constexpr uintptr_t QSPI_BASE = 0x44A00000;   // set from BD Address Editor

// static constexpr uint32_t REG_SRR    = 0x40;
// static constexpr uint32_t REG_SPICR  = 0x60;
// static constexpr uint32_t REG_SPISR  = 0x64;
// static constexpr uint32_t REG_DTR    = 0x68;
// static constexpr uint32_t REG_DRR    = 0x6C;
// static constexpr uint32_t REG_SSR    = 0x70;

// static constexpr uint32_t CR_SPE    = (1u << 1);
// static constexpr uint32_t CR_MASTER = (1u << 2);
// static constexpr uint32_t CR_TXRST  = (1u << 5);
// static constexpr uint32_t CR_RXRST  = (1u << 6);
// static constexpr uint32_t CR_MT_INH = (1u << 8);

// static constexpr uint32_t SR_RX_EMPTY       = (1u << 0);
// static constexpr uint32_t SR_TX_EMPTY       = (1u << 2);
// static constexpr uint32_t SR_CPOL_CPHA_ERR  = (1u << 6);
// static constexpr uint32_t SR_SLAVE_MODE_ERR = (1u << 7);
// static constexpr uint32_t SR_MSB_ERR        = (1u << 8);
// static constexpr uint32_t SR_LOOPBACK_ERR   = (1u << 9);
// static constexpr uint32_t SR_COMMAND_ERR    = (1u << 10);

// static constexpr uint32_t SSR_ASSERT_SLAVE0 = 0x00000000;
// static constexpr uint32_t SSR_DEASSERT_ALL  = 0x00000001;

// static constexpr uint32_t SPICR_IDLE = CR_SPE | CR_MASTER | CR_MT_INH;
// static constexpr uint32_t SPICR_GO   = CR_SPE | CR_MASTER;

// // Keep this for now just to diagnose; later you may switch to 0x03 standard read.
// static constexpr uint8_t CMD_READ = 0x6B;

// static inline void mmio_write32(uintptr_t addr, uint32_t value) {
//   *reinterpret_cast<volatile uint32_t*>(addr) = value;
// }

// static inline uint32_t mmio_read32(uintptr_t addr) {
//   return *reinterpret_cast<volatile uint32_t*>(addr);
// }

// static inline void uart_putc(char c) {
//   volatile uint32_t* status = reinterpret_cast<volatile uint32_t*>(UART_BASE + 0x08);
//   volatile uint8_t*  txfifo = reinterpret_cast<volatile uint8_t*>(UART_BASE + 0x04);
//   while ((*status) & 0x08) { }
//   *txfifo = static_cast<uint8_t>(c);
// }

// static void uart_puts(const char* s) {
//   while (*s) uart_putc(*s++);
// }

// static void uart_puthex8(uint8_t v) {
//   const char* hex = "0123456789ABCDEF";
//   uart_putc(hex[(v >> 4) & 0xF]);
//   uart_putc(hex[v & 0xF]);
// }

// static void uart_puthex32(uint32_t v) {
//   const char* hex = "0123456789ABCDEF";
//   for (int i = 7; i >= 0; --i) {
//     uart_putc(hex[(v >> (i * 4)) & 0xF]);
//   }
// }

// static void uart_newline() {
//   uart_puts("\r\n");
// }

// static void print_spisr(const char* tag) {
//   uint32_t sr = mmio_read32(QSPI_BASE + REG_SPISR);
//   uart_puts(tag);
//   uart_puts(" SPISR=0x");
//   uart_puthex32(sr);
//   uart_puts(" [");
//   uart_putc((sr & SR_RX_EMPTY)       ? 'R' : '-'); // RX_EMPTY
//   uart_putc((sr & SR_TX_EMPTY)       ? 'T' : '-'); // TX_EMPTY
//   uart_putc((sr & SR_CPOL_CPHA_ERR)  ? 'C' : '-');
//   uart_putc((sr & SR_SLAVE_MODE_ERR) ? 'S' : '-');
//   uart_putc((sr & SR_MSB_ERR)        ? 'M' : '-');
//   uart_putc((sr & SR_LOOPBACK_ERR)   ? 'L' : '-');
//   uart_putc((sr & SR_COMMAND_ERR)    ? 'X' : '-'); // command error
//   uart_puts("]");
//   uart_newline();
// }

// static void qspi_push_byte(uint8_t b) {
//   mmio_write32(QSPI_BASE + REG_DTR, b);
// }

// static void qspi_init_only() {
//   mmio_write32(QSPI_BASE + REG_SRR, 0x0000000A);
//   mmio_write32(QSPI_BASE + REG_SSR, SSR_DEASSERT_ALL);
//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE | CR_TXRST | CR_RXRST);
//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE);
// }

// int main() {
//   uart_puts("QSPI TEST START\r\n");

//   qspi_init_only();
//   print_spisr("after init");

//   const uint32_t flash_addr = 0x00100000;
//   uart_puts("Reading flash @ ");
//   uart_puthex32(flash_addr);
//   uart_newline();

//   mmio_write32(QSPI_BASE + REG_SSR, SSR_ASSERT_SLAVE0);
//   print_spisr("after ss assert");

//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE | CR_TXRST | CR_RXRST);
//   print_spisr("after fifo reset");

//   qspi_push_byte(CMD_READ);
//   print_spisr("after cmd");

//   qspi_push_byte((flash_addr >> 16) & 0xFF);
//   qspi_push_byte((flash_addr >> 8)  & 0xFF);
//   qspi_push_byte((flash_addr >> 0)  & 0xFF);
//   qspi_push_byte(0x00);
//   qspi_push_byte(0x00);
//   print_spisr("after addr+dummies");

//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_GO);
//   print_spisr("after go");

//   for (int i = 0; i < 1000; ++i) {
//     uint32_t sr = mmio_read32(QSPI_BASE + REG_SPISR);
//     if (sr & SR_TX_EMPTY) {
//       uart_puts("TX empty reached\r\n");
//       break;
//     }
//     if (i == 999) {
//       uart_puts("TX empty timeout\r\n");
//       uart_puts("final ");
//       print_spisr("poll");
//     }
//   }

//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE);
//   mmio_write32(QSPI_BASE + REG_SSR, SSR_DEASSERT_ALL);
//   print_spisr("after stop");

//   while (1) { }
//   return 0;
// }


// #include <stdint.h>

// // -----------------------------
// // Change these to your real addresses
// // -----------------------------
// static constexpr uintptr_t UART_BASE = 0x40600000;   // adjust if needed
// static constexpr uintptr_t QSPI_BASE = 0x44A00000;   // set from BD Address Editor

// // -----------------------------
// // AXI Quad SPI register offsets
// // from your xci
// // -----------------------------
// static constexpr uint32_t REG_SRR    = 0x40;
// static constexpr uint32_t REG_SPICR  = 0x60;
// static constexpr uint32_t REG_SPISR  = 0x64;
// static constexpr uint32_t REG_DTR    = 0x68;
// static constexpr uint32_t REG_DRR    = 0x6C;
// static constexpr uint32_t REG_SSR    = 0x70;

// // -----------------------------
// // SPICR bits from your xci
// // -----------------------------
// static constexpr uint32_t CR_SPE    = (1u << 1);
// static constexpr uint32_t CR_MASTER = (1u << 2);
// static constexpr uint32_t CR_TXRST  = (1u << 5);
// static constexpr uint32_t CR_RXRST  = (1u << 6);
// static constexpr uint32_t CR_MT_INH = (1u << 8);

// // -----------------------------
// // SPISR bits from your xci
// // -----------------------------
// static constexpr uint32_t SR_RX_EMPTY = (1u << 0);
// static constexpr uint32_t SR_TX_EMPTY = (1u << 2);

// // one slave, active low
// static constexpr uint32_t SSR_ASSERT_SLAVE0 = 0x00000000;
// static constexpr uint32_t SSR_DEASSERT_ALL  = 0x00000001;

// static constexpr uint32_t SPICR_IDLE = CR_SPE | CR_MASTER | CR_MT_INH;
// static constexpr uint32_t SPICR_GO   = CR_SPE | CR_MASTER;

// // common Micron quad fast read command
// static constexpr uint8_t CMD_READ = 0x6B;

// // -----------------------------
// // MMIO helpers
// // -----------------------------
// static inline void mmio_write32(uintptr_t addr, uint32_t value) {
//   *reinterpret_cast<volatile uint32_t*>(addr) = value;
// }

// static inline uint32_t mmio_read32(uintptr_t addr) {
//   return *reinterpret_cast<volatile uint32_t*>(addr);
// }

// // -----------------------------
// // UART helpers
// // -----------------------------
// static inline void uart_putc(char c) {
//   volatile uint32_t* status = reinterpret_cast<volatile uint32_t*>(UART_BASE + 0x08);
//   volatile uint8_t*  txfifo = reinterpret_cast<volatile uint8_t*>(UART_BASE + 0x04);
//   while ((*status) & 0x08) { }
//   *txfifo = static_cast<uint8_t>(c);
// }

// static void uart_puts(const char* s) {
//   while (*s) uart_putc(*s++);
// }

// static void uart_puthex8(uint8_t v) {
//   const char* hex = "0123456789ABCDEF";
//   uart_putc(hex[(v >> 4) & 0xF]);
//   uart_putc(hex[v & 0xF]);
// }

// static void uart_puthex32(uint32_t v) {
//   const char* hex = "0123456789ABCDEF";
//   for (int i = 7; i >= 0; --i) {
//     uart_putc(hex[(v >> (i * 4)) & 0xF]);
//   }
// }

// // -----------------------------
// // QSPI helpers
// // -----------------------------
// static void qspi_reset_and_init() {
//   mmio_write32(QSPI_BASE + REG_SRR, 0x0000000A);                  // reset
//   mmio_write32(QSPI_BASE + REG_SSR, SSR_DEASSERT_ALL);            // deassert SS
//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE | CR_TXRST | CR_RXRST);
//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE);
// }

// static void qspi_push_byte(uint8_t b) {
//   mmio_write32(QSPI_BASE + REG_DTR, b);
// }

// static void qspi_wait_tx_empty() {
//   while ((mmio_read32(QSPI_BASE + REG_SPISR) & SR_TX_EMPTY) == 0) { }
// }

// static uint8_t qspi_wait_and_pop_rx() {
//   while (mmio_read32(QSPI_BASE + REG_SPISR) & SR_RX_EMPTY) { }
//   return static_cast<uint8_t>(mmio_read32(QSPI_BASE + REG_DRR) & 0xFF);
// }

// // Reads one byte from flash at 24-bit address.
// // This mirrors the simple hardware test sequence.
// static uint8_t qspi_read_byte(uint32_t flash_addr) {
//   mmio_write32(QSPI_BASE + REG_SSR, SSR_ASSERT_SLAVE0);

//   // reset FIFOs before transaction
//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE | CR_TXRST | CR_RXRST);

//   // cmd + 24-bit address + 2 dummy bytes
//   qspi_push_byte(CMD_READ);
//   qspi_push_byte((flash_addr >> 16) & 0xFF);
//   qspi_push_byte((flash_addr >>  8) & 0xFF);
//   qspi_push_byte((flash_addr >>  0) & 0xFF);
//   qspi_push_byte(0x00);
//   qspi_push_byte(0x00);

//   // start transaction
//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_GO);

//   qspi_wait_tx_empty();

//   // stop/inhibit further shifting
//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE);
//   mmio_write32(QSPI_BASE + REG_SSR, SSR_DEASSERT_ALL);

//   // Drain 6 RX bytes; last one is assumed to be the returned data byte
//   (void)qspi_wait_and_pop_rx();
//   (void)qspi_wait_and_pop_rx();
//   (void)qspi_wait_and_pop_rx();
//   (void)qspi_wait_and_pop_rx();
//   (void)qspi_wait_and_pop_rx();
//   return qspi_wait_and_pop_rx();
// }

// // -----------------------------
// // Test entry
// // -----------------------------
// extern "C" int main() {
//   uart_puts("QSPI TEST START\r\n");

//   qspi_reset_and_init();
//   uart_puts("QSPI init done\r\n");

//   const uint32_t test_flash_addr = 0x00100000; // payload offset in flash
//   uart_puts("Reading flash @ ");
//   uart_puthex32(test_flash_addr);
//   uart_puts("\r\n");

//   uint8_t b = qspi_read_byte(test_flash_addr);

//   uart_puts("Read byte = 0x");
//   uart_puthex8(b);
//   uart_puts("\r\n");

//   while (1) { }
// }


// // #include "tensorflow/lite/micro/examples/person_detection/main_functions.h"
// #include <new>

// #include "tensorflow/lite/micro/examples/person_detection/detection_responder.h"
// #include "tensorflow/lite/micro/examples/person_detection/image_provider.h"
// #include "tensorflow/lite/micro/examples/person_detection/model_settings.h"
// #include "tensorflow/lite/micro/micro_interpreter.h"
// #include "tensorflow/lite/micro/micro_log.h"
// #include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
// #include "tensorflow/lite/micro/system_setup.h"
// #include "tensorflow/lite/schema/schema_generated.h"
// #include "tensorflow/lite/micro/models/person_detect_model_data.h"
// #include <stdint.h>

// #include <stdint.h>

// // #include <stdint.h>

// // ============================================================
// // Toggle this between 0 and 1 for testing
// // ============================================================
// #define USE_MMIO_BSWAP 0

// // ============================================================
// // UART Lite
// // ============================================================
// static inline volatile uint32_t* const UART_TX =
//     reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
// static inline volatile uint32_t* const UART_ST =
//     reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x08u);

// static inline void RawPutc(char c) {
//   while ((*UART_ST) & 0x08u) { }
//   *UART_TX = static_cast<uint32_t>(static_cast<uint8_t>(c));
// }

// static inline void RawNL() {
//   RawPutc('\r');
//   RawPutc('\n');
// }

// static inline void RawPutHex4(uint8_t x) {
//   x &= 0xF;
//   RawPutc((x < 10) ? ('0' + x) : ('A' + (x - 10)));
// }

// static inline void RawPutHex8(uint8_t x) {
//   RawPutHex4(x >> 4);
//   RawPutHex4(x);
// }

// static inline void RawPutHex32(uint32_t x) {
//   for (int i = 7; i >= 0; --i) {
//     RawPutHex4((x >> (i * 4)) & 0xF);
//   }
// }

// static inline void Fail(char code) {
//   RawPutc('F');
//   RawPutc(code);
//   RawNL();
//   while (1) { }
// }

// // ============================================================
// // AXI Quad SPI
// // ============================================================
// static constexpr uintptr_t QSPI_BASE = 0x44A00000u;

// static constexpr uintptr_t XSP_DGIER_OFFSET      = 0x1Cu;
// static constexpr uintptr_t XSP_IISR_OFFSET       = 0x20u;
// static constexpr uintptr_t XSP_IIER_OFFSET       = 0x28u;
// static constexpr uintptr_t XSP_SRR_OFFSET        = 0x40u;
// static constexpr uintptr_t XSP_CR_OFFSET         = 0x60u;
// static constexpr uintptr_t XSP_SR_OFFSET         = 0x64u;
// static constexpr uintptr_t XSP_DTR_OFFSET        = 0x68u;
// static constexpr uintptr_t XSP_DRR_OFFSET        = 0x6Cu;
// static constexpr uintptr_t XSP_SSR_OFFSET        = 0x70u;
// static constexpr uintptr_t XSP_TXFIFO_OR_OFFSET  = 0x74u;
// static constexpr uintptr_t XSP_RXFIFO_OR_OFFSET  = 0x78u;

// // SPICR bits
// static constexpr uint32_t XSP_CR_ENABLE_MASK         = 0x00000002u;
// static constexpr uint32_t XSP_CR_MASTER_MODE_MASK    = 0x00000004u;
// static constexpr uint32_t XSP_CR_TXFIFO_RESET_MASK   = 0x00000020u;
// static constexpr uint32_t XSP_CR_RXFIFO_RESET_MASK   = 0x00000040u;
// static constexpr uint32_t XSP_CR_MANUAL_SS_MASK      = 0x00000080u;
// static constexpr uint32_t XSP_CR_TRANS_INHIBIT_MASK  = 0x00000100u;

// // SPISR bits
// static constexpr uint32_t XSP_SR_RX_EMPTY_MASK       = 0x00000001u;
// static constexpr uint32_t XSP_SR_TX_FULL_MASK        = 0x00000008u;

// static inline uint32_t bswap32(uint32_t x) {
//   return ((x & 0x000000FFu) << 24) |
//          ((x & 0x0000FF00u) <<  8) |
//          ((x & 0x00FF0000u) >>  8) |
//          ((x & 0xFF000000u) >> 24);
// }

// static inline uint32_t raw_mmio_read(uintptr_t addr) {
//   return *reinterpret_cast<volatile uint32_t*>(addr);
// }

// static inline void raw_mmio_write(uintptr_t addr, uint32_t value) {
//   *reinterpret_cast<volatile uint32_t*>(addr) = value;
// }

// static inline uint32_t mmio_read(uintptr_t addr) {
//   uint32_t v = raw_mmio_read(addr);
// #if USE_MMIO_BSWAP
//   return bswap32(v);
// #else
//   return v;
// #endif
// }

// static inline void mmio_write(uintptr_t addr, uint32_t value) {
// #if USE_MMIO_BSWAP
//   raw_mmio_write(addr, bswap32(value));
// #else
//   raw_mmio_write(addr, value);
// #endif
// }

// static void PrintReg(char tag, uintptr_t off) {
//   uint32_t raw = raw_mmio_read(QSPI_BASE + off);
//   uint32_t swp = bswap32(raw);

//   RawPutc(tag);
//   RawPutc('=');
//   RawPutHex32(raw);
//   RawPutc('/');
//   RawPutHex32(swp);
//   RawPutc(' ');
// }

// static void DumpRegs(char tag) {
//   RawPutc(tag);
//   RawPutc(':');
//   RawPutc(' ');
//   PrintReg('C', XSP_CR_OFFSET);
//   PrintReg('S', XSP_SR_OFFSET);
//   PrintReg('R', XSP_SSR_OFFSET);
//   PrintReg('T', XSP_TXFIFO_OR_OFFSET);
//   PrintReg('Q', XSP_RXFIFO_OR_OFFSET);
//   RawNL();
// }

// static void qspi_init() {
//   RawPutc('1');

//   mmio_write(QSPI_BASE + XSP_SRR_OFFSET, 0x0000000Au);
//   RawPutc('2');

//   mmio_write(QSPI_BASE + XSP_DGIER_OFFSET, 0x00000000u);
//   mmio_write(QSPI_BASE + XSP_IIER_OFFSET,  0x00000000u);
//   mmio_write(QSPI_BASE + XSP_IISR_OFFSET,  0xFFFFFFFFu);
//   RawPutc('3');

//   uint32_t cr =
//       XSP_CR_ENABLE_MASK |
//       XSP_CR_MASTER_MODE_MASK |
//       XSP_CR_MANUAL_SS_MASK |
//       XSP_CR_TRANS_INHIBIT_MASK |
//       XSP_CR_TXFIFO_RESET_MASK |
//       XSP_CR_RXFIFO_RESET_MASK;

//   mmio_write(QSPI_BASE + XSP_CR_OFFSET, cr);
//   RawPutc('4');

//   // mmio_write(QSPI_BASE + XSP_SSR_OFFSET, 0xFFFFFFFFu);
//   RawPutc('5');
//   // RawNL();

//   // DumpRegs('I');
// }

// static inline void qspi_cs_assert() {
//   mmio_write(QSPI_BASE + XSP_SSR_OFFSET, 0xFFFFFFFEu);
// }

// static inline void qspi_cs_deassert() {
//   mmio_write(QSPI_BASE + XSP_SSR_OFFSET, 0xFFFFFFFFu);
// }

// static void qspi_drain_rx() {
//   for (int i = 0; i < 16; ++i) {
//     uint32_t sr = mmio_read(QSPI_BASE + XSP_SR_OFFSET);
//     if (sr & XSP_SR_RX_EMPTY_MASK) break;
//     (void)mmio_read(QSPI_BASE + XSP_DRR_OFFSET);
//   }
// }

// static uint8_t qspi_transfer_byte(uint8_t tx) {
//   RawPutc('a');

//   qspi_drain_rx();
//   RawPutc('b');

//   uint32_t guard = 1000000;

//   while ((mmio_read(QSPI_BASE + XSP_SR_OFFSET) & XSP_SR_TX_FULL_MASK) != 0u) {
//     if (--guard == 0u) {
//       DumpRegs('X');
//       Fail('T');
//     }
//   }
//   RawPutc('c');

//   mmio_write(QSPI_BASE + XSP_DTR_OFFSET, static_cast<uint32_t>(tx));
//   RawPutc('d');

//   uint32_t cr = mmio_read(QSPI_BASE + XSP_CR_OFFSET);
//   cr &= ~XSP_CR_TRANS_INHIBIT_MASK;
//   mmio_write(QSPI_BASE + XSP_CR_OFFSET, cr);
//   RawPutc('e');

//   guard = 1000000;
//   while ((mmio_read(QSPI_BASE + XSP_SR_OFFSET) & XSP_SR_RX_EMPTY_MASK) != 0u) {
//     if (--guard == 0u) {
//       DumpRegs('Y');
//       Fail('R');
//     }
//   }
//   RawPutc('f');

//   cr = mmio_read(QSPI_BASE + XSP_CR_OFFSET);
//   cr |= XSP_CR_TRANS_INHIBIT_MASK;
//   mmio_write(QSPI_BASE + XSP_CR_OFFSET, cr);
//   RawPutc('g');

//   uint8_t rx = static_cast<uint8_t>(mmio_read(QSPI_BASE + XSP_DRR_OFFSET) & 0xFFu);
//   RawPutc('h');

//   return rx;
// }

// static void qspi_read_jedec_id(uint8_t* id) {
//   RawPutc('J');
//   qspi_cs_assert();
//   RawPutc('K');
//   DumpRegs('A');

//   (void)qspi_transfer_byte(0x9Fu);
//   RawPutc('L');

//   id[0] = qspi_transfer_byte(0xFFu);
//   RawPutc('M');

//   id[1] = qspi_transfer_byte(0xFFu);
//   RawPutc('N');

//   id[2] = qspi_transfer_byte(0xFFu);
//   RawPutc('O');

//   qspi_cs_deassert();
//   RawPutc('P');
//   RawNL();
// }

// static void qspi_read_data(uint32_t addr, uint8_t* out, uint32_t len) {
//   RawPutc('D');
//   qspi_cs_assert();
//   RawPutc('E');

//   (void)qspi_transfer_byte(0x03u);
//   (void)qspi_transfer_byte(static_cast<uint8_t>((addr >> 16) & 0xFFu));
//   (void)qspi_transfer_byte(static_cast<uint8_t>((addr >>  8) & 0xFFu));
//   (void)qspi_transfer_byte(static_cast<uint8_t>((addr >>  0) & 0xFFu));
//   RawPutc('F');

//   for (uint32_t i = 0; i < len; ++i) {
//     out[i] = qspi_transfer_byte(0xFFu);
//   }

//   qspi_cs_deassert();
//   RawPutc('G');
//   RawNL();
// }

// extern "C" int main() {
//   RawPutc('S');
//   // RawNL();

//   qspi_init();

//   RawPutc('6');

//   RawPutc('C');
//   RawPutHex32(raw_mmio_read(QSPI_BASE + XSP_CR_OFFSET));
//   RawPutc('/');
//   RawPutHex32(bswap32(raw_mmio_read(QSPI_BASE + XSP_CR_OFFSET)));
//   RawPutc(' ');

//   uint8_t jedec[3] = {0, 0, 0};
//   qspi_read_jedec_id(jedec);

//   RawPutc('I');
//   RawPutc(':');
//   RawPutc(' ');
//   RawPutHex8(jedec[0]); RawPutc(' ');
//   RawPutHex8(jedec[1]); RawPutc(' ');
//   RawPutHex8(jedec[2]);
//   // RawNL();

//   uint8_t buf[16];
//   for (int i = 0; i < 16; ++i) buf[i] = 0;

//   qspi_read_data(0x000000u, buf, 16);

//   RawPutc('B');
//   RawPutc(':');
//   RawPutc(' ');
//   for (int i = 0; i < 16; ++i) {
//     RawPutHex8(buf[i]);
//     RawPutc(' ');
//   }
//   // RawNL();

//   RawPutc('Z');
//   RawNL();

//   while (1) { }
//   return 0;
// }


// div_rem_test.cc
// Bare-metal RV32IMC DIV/REM test with AXI UART Lite prints.
//
// This directly executes:
//   div, divu, rem, remu
//
// Expected output style:
//   === RV32IM DIV/REM TEST ===
//   S
//   D0 A0000000A B00000002 G00000005 E00000005 P
//   ...
//   PASS
//
// If something fails, it prints F beside that test.

// #include <stdint.h>

// // ------------------------------------------------------------
// // UART Lite helpers
// // ------------------------------------------------------------
// static inline void RawPutc(char c) {
//   volatile uint32_t* const uart_tx =
//       reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
//   volatile uint32_t* const uart_status =
//       reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x08u);

//   // AXI UART Lite: bit 3 = TX FIFO full
//   while ((*uart_status) & 0x08u) {
//   }

//   *uart_tx = static_cast<uint32_t>(static_cast<uint8_t>(c));
// }

// static inline void RawPuts(const char* s) {
//   while (*s) {
//     RawPutc(*s++);
//   }
// }

// static inline void RawNewline() {
//   RawPutc('\r');
//   RawPutc('\n');
// }

// static inline void RawPutHexNibble(uint32_t v) {
//   v &= 0xFu;
//   RawPutc((v < 10u) ? static_cast<char>('0' + v)
//                     : static_cast<char>('A' + (v - 10u)));
// }

// static inline void RawPutHex32(uint32_t v) {
//   for (int shift = 28; shift >= 0; shift -= 4) {
//     RawPutHexNibble(v >> shift);
//   }
// }

// static inline void RawTagHex(char tag, uint32_t v) {
//   RawPutc(tag);
//   RawPutHex32(v);
//   RawPutc(' ');
// }

// // ------------------------------------------------------------
// // Force actual RISC-V M-extension instructions
// // ------------------------------------------------------------
// __attribute__((noinline))
// static int32_t DoDiv(int32_t a, int32_t b) {
//   int32_t r;
//   asm volatile("div %0, %1, %2"
//                : "=r"(r)
//                : "r"(a), "r"(b));
//   return r;
// }

// __attribute__((noinline))
// static int32_t DoRem(int32_t a, int32_t b) {
//   int32_t r;
//   asm volatile("rem %0, %1, %2"
//                : "=r"(r)
//                : "r"(a), "r"(b));
//   return r;
// }

// __attribute__((noinline))
// static uint32_t DoDivu(uint32_t a, uint32_t b) {
//   uint32_t r;
//   asm volatile("divu %0, %1, %2"
//                : "=r"(r)
//                : "r"(a), "r"(b));
//   return r;
// }

// __attribute__((noinline))
// static uint32_t DoRemu(uint32_t a, uint32_t b) {
//   uint32_t r;
//   asm volatile("remu %0, %1, %2"
//                : "=r"(r)
//                : "r"(a), "r"(b));
//   return r;
// }

// // ------------------------------------------------------------
// // Test print format
// // A = operand A
// // B = operand B
// // G = got result
// // E = expected result
// // P = pass
// // F = fail
// // ------------------------------------------------------------
// static int g_fail_count = 0;

// static void Check32(char group,
//                     int index,
//                     uint32_t a,
//                     uint32_t b,
//                     uint32_t got,
//                     uint32_t expected) {
//   RawPutc(group);
//   RawPutc(static_cast<char>('0' + index));
//   RawPutc(' ');

//   RawTagHex('A', a);
//   RawTagHex('B', b);
//   RawTagHex('G', got);
//   RawTagHex('E', expected);

//   if (got == expected) {
//     RawPutc('P');
//   } else {
//     RawPutc('F');
//     g_fail_count++;
//   }

//   RawNewline();
// }

// // ------------------------------------------------------------
// // Optional DONE mailbox for your Verilog testbench
// // ------------------------------------------------------------
// static inline void WriteDoneMailbox() {
//   volatile uint32_t* const sig0 =
//       reinterpret_cast<volatile uint32_t*>(0x000FFFF4u);
//   volatile uint32_t* const sig1 =
//       reinterpret_cast<volatile uint32_t*>(0x000FFFF8u);
//   volatile uint32_t* const done =
//       reinterpret_cast<volatile uint32_t*>(0x000FFFFCu);

//   *sig0 = 0x11111111u;
//   *sig1 = 0x22222222u;
//   *done = 0x00000001u;
// }

// // ------------------------------------------------------------
// // Main test
// // ------------------------------------------------------------
// extern "C" int main() {
//   RawNewline();
//   RawPuts("=== RV32IM DIV/REM TEST ===");
//   RawNewline();

//   RawPutc('S');
//   RawNewline();

//   // ----------------------------------------------------------
//   // Signed DIV tests
//   // RISC-V signed division truncates toward zero.
//   // Special cases:
//   //   div by zero      => -1 / 0xFFFFFFFF
//   //   INT_MIN / -1     => INT_MIN / 0x80000000
//   // ----------------------------------------------------------
//   Check32('D', 0, 10, 2,
//           static_cast<uint32_t>(DoDiv(10, 2)),
//           5);

//   Check32('D', 1, static_cast<uint32_t>(-10), 2,
//           static_cast<uint32_t>(DoDiv(-10, 2)),
//           static_cast<uint32_t>(-5));

//   Check32('D', 2, 10, static_cast<uint32_t>(-2),
//           static_cast<uint32_t>(DoDiv(10, -2)),
//           static_cast<uint32_t>(-5));

//   Check32('D', 3, static_cast<uint32_t>(-10), static_cast<uint32_t>(-2),
//           static_cast<uint32_t>(DoDiv(-10, -2)),
//           5);

//   Check32('D', 4, 7, 3,
//           static_cast<uint32_t>(DoDiv(7, 3)),
//           2);

//   Check32('D', 5, static_cast<uint32_t>(-7), 3,
//           static_cast<uint32_t>(DoDiv(-7, 3)),
//           static_cast<uint32_t>(-2));

//   // RISC-V defined behavior: divide by zero gives all 1s.
//   Check32('D', 6, 123, 0,
//           static_cast<uint32_t>(DoDiv(123, 0)),
//           0xFFFFFFFFu);

//   // RISC-V defined behavior: INT_MIN / -1 gives INT_MIN.
//   Check32('D', 7, 0x80000000u, 0xFFFFFFFFu,
//           static_cast<uint32_t>(DoDiv(static_cast<int32_t>(0x80000000u), -1)),
//           0x80000000u);

//   // ----------------------------------------------------------
//   // Signed REM tests
//   // Remainder has the sign of the dividend.
//   // Special cases:
//   //   rem by zero      => dividend
//   //   INT_MIN % -1     => 0
//   // ----------------------------------------------------------
//   Check32('R', 0, 10, 3,
//           static_cast<uint32_t>(DoRem(10, 3)),
//           1);

//   Check32('R', 1, static_cast<uint32_t>(-10), 3,
//           static_cast<uint32_t>(DoRem(-10, 3)),
//           static_cast<uint32_t>(-1));

//   Check32('R', 2, 10, static_cast<uint32_t>(-3),
//           static_cast<uint32_t>(DoRem(10, -3)),
//           1);

//   Check32('R', 3, static_cast<uint32_t>(-10), static_cast<uint32_t>(-3),
//           static_cast<uint32_t>(DoRem(-10, -3)),
//           static_cast<uint32_t>(-1));

//   // RISC-V defined behavior: rem by zero returns dividend.
//   Check32('R', 4, 123, 0,
//           static_cast<uint32_t>(DoRem(123, 0)),
//           123);

//   // RISC-V defined behavior: INT_MIN % -1 gives 0.
//   Check32('R', 5, 0x80000000u, 0xFFFFFFFFu,
//           static_cast<uint32_t>(DoRem(static_cast<int32_t>(0x80000000u), -1)),
//           0);

//   // ----------------------------------------------------------
//   // Unsigned DIVU tests
//   // ----------------------------------------------------------
//   Check32('U', 0, 10, 2,
//           DoDivu(10u, 2u),
//           5u);

//   Check32('U', 1, 0xFFFFFFFFu, 2u,
//           DoDivu(0xFFFFFFFFu, 2u),
//           0x7FFFFFFFu);

//   Check32('U', 2, 0x80000000u, 2u,
//           DoDivu(0x80000000u, 2u),
//           0x40000000u);

//   // RISC-V defined behavior: unsigned divide by zero gives all 1s.
//   Check32('U', 3, 123u, 0u,
//           DoDivu(123u, 0u),
//           0xFFFFFFFFu);

//   // ----------------------------------------------------------
//   // Unsigned REMU tests
//   // ----------------------------------------------------------
//   Check32('M', 0, 10u, 3u,
//           DoRemu(10u, 3u),
//           1u);

//   Check32('M', 1, 0xFFFFFFFFu, 2u,
//           DoRemu(0xFFFFFFFFu, 2u),
//           1u);

//   Check32('M', 2, 0x80000000u, 3u,
//           DoRemu(0x80000000u, 3u),
//           2u);

//   // RISC-V defined behavior: unsigned rem by zero returns dividend.
//   Check32('M', 3, 123u, 0u,
//           DoRemu(123u, 0u),
//           123u);

//   RawNewline();

//   if (g_fail_count == 0) {
//     RawPuts("PASS");
//   } else {
//     RawPuts("FAIL ");
//     RawTagHex('N', static_cast<uint32_t>(g_fail_count));
//   }

//   RawNewline();

//   WriteDoneMailbox();

//   while (1) {
//   }

//   return 0;
// }


//   mmio_write32(QSPI_BASE + REG_SRR, 0x0000000A);
//   mmio_write32(QSPI_BASE + REG_SSR, SSR_DEASSERT_ALL);
//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE | CR_TXRST | CR_RXRST);
//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE);
// }

// int main() {
//   uart_puts("QSPI TEST START\r\n");

//   qspi_init_only();
//   print_spisr("after init");

//   const uint32_t flash_addr = 0x00100000;
//   uart_puts("Reading flash @ ");
//   uart_puthex32(flash_addr);
//   uart_newline();

//   mmio_write32(QSPI_BASE + REG_SSR, SSR_ASSERT_SLAVE0);
//   print_spisr("after ss assert");

//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE | CR_TXRST | CR_RXRST);
//   print_spisr("after fifo reset");

//   qspi_push_byte(CMD_READ);
//   print_spisr("after cmd");

//   qspi_push_byte((flash_addr >> 16) & 0xFF);
//   qspi_push_byte((flash_addr >> 8)  & 0xFF);
//   qspi_push_byte((flash_addr >> 0)  & 0xFF);
//   qspi_push_byte(0x00);
//   qspi_push_byte(0x00);
//   print_spisr("after addr+dummies");

//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_GO);
//   print_spisr("after go");

//   for (int i = 0; i < 1000; ++i) {
//     uint32_t sr = mmio_read32(QSPI_BASE + REG_SPISR);
//     if (sr & SR_TX_EMPTY) {
//       uart_puts("TX empty reached\r\n");
//       break;
//     }
//     if (i == 999) {
//       uart_puts("TX empty timeout\r\n");
//       uart_puts("final ");
//       print_spisr("poll");
//     }
//   }

//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE);
//   mmio_write32(QSPI_BASE + REG_SSR, SSR_DEASSERT_ALL);
//   print_spisr("after stop");

//   while (1) { }
//   return 0;
// }


// #include <stdint.h>

// // -----------------------------
// // Change these to your real addresses
// // -----------------------------
// static constexpr uintptr_t UART_BASE = 0x40600000;   // adjust if needed
// static constexpr uintptr_t QSPI_BASE = 0x44A00000;   // set from BD Address Editor

// // -----------------------------
// // AXI Quad SPI register offsets
// // from your xci
// // -----------------------------
// static constexpr uint32_t REG_SRR    = 0x40;
// static constexpr uint32_t REG_SPICR  = 0x60;
// static constexpr uint32_t REG_SPISR  = 0x64;
// static constexpr uint32_t REG_DTR    = 0x68;
// static constexpr uint32_t REG_DRR    = 0x6C;
// static constexpr uint32_t REG_SSR    = 0x70;

// // -----------------------------
// // SPICR bits from your xci
// // -----------------------------
// static constexpr uint32_t CR_SPE    = (1u << 1);
// static constexpr uint32_t CR_MASTER = (1u << 2);
// static constexpr uint32_t CR_TXRST  = (1u << 5);
// static constexpr uint32_t CR_RXRST  = (1u << 6);
// static constexpr uint32_t CR_MT_INH = (1u << 8);

// // -----------------------------
// // SPISR bits from your xci
// // -----------------------------
// static constexpr uint32_t SR_RX_EMPTY = (1u << 0);
// static constexpr uint32_t SR_TX_EMPTY = (1u << 2);

// // one slave, active low
// static constexpr uint32_t SSR_ASSERT_SLAVE0 = 0x00000000;
// static constexpr uint32_t SSR_DEASSERT_ALL  = 0x00000001;

// static constexpr uint32_t SPICR_IDLE = CR_SPE | CR_MASTER | CR_MT_INH;
// static constexpr uint32_t SPICR_GO   = CR_SPE | CR_MASTER;

// // common Micron quad fast read command
// static constexpr uint8_t CMD_READ = 0x6B;

// // -----------------------------
// // MMIO helpers
// // -----------------------------
// static inline void mmio_write32(uintptr_t addr, uint32_t value) {
//   *reinterpret_cast<volatile uint32_t*>(addr) = value;
// }

// static inline uint32_t mmio_read32(uintptr_t addr) {
//   return *reinterpret_cast<volatile uint32_t*>(addr);
// }

// // -----------------------------
// // UART helpers
// // -----------------------------
// static inline void uart_putc(char c) {
//   volatile uint32_t* status = reinterpret_cast<volatile uint32_t*>(UART_BASE + 0x08);
//   volatile uint8_t*  txfifo = reinterpret_cast<volatile uint8_t*>(UART_BASE + 0x04);
//   while ((*status) & 0x08) { }
//   *txfifo = static_cast<uint8_t>(c);
// }

// static void uart_puts(const char* s) {
//   while (*s) uart_putc(*s++);
// }

// static void uart_puthex8(uint8_t v) {
//   const char* hex = "0123456789ABCDEF";
//   uart_putc(hex[(v >> 4) & 0xF]);
//   uart_putc(hex[v & 0xF]);
// }

// static void uart_puthex32(uint32_t v) {
//   const char* hex = "0123456789ABCDEF";
//   for (int i = 7; i >= 0; --i) {
//     uart_putc(hex[(v >> (i * 4)) & 0xF]);
//   }
// }

// // -----------------------------
// // QSPI helpers
// // -----------------------------
// static void qspi_reset_and_init() {
//   mmio_write32(QSPI_BASE + REG_SRR, 0x0000000A);                  // reset
//   mmio_write32(QSPI_BASE + REG_SSR, SSR_DEASSERT_ALL);            // deassert SS
//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE | CR_TXRST | CR_RXRST);
//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE);
// }

// static void qspi_push_byte(uint8_t b) {
//   mmio_write32(QSPI_BASE + REG_DTR, b);
// }

// static void qspi_wait_tx_empty() {
//   while ((mmio_read32(QSPI_BASE + REG_SPISR) & SR_TX_EMPTY) == 0) { }
// }

// static uint8_t qspi_wait_and_pop_rx() {
//   while (mmio_read32(QSPI_BASE + REG_SPISR) & SR_RX_EMPTY) { }
//   return static_cast<uint8_t>(mmio_read32(QSPI_BASE + REG_DRR) & 0xFF);
// }

// // Reads one byte from flash at 24-bit address.
// // This mirrors the simple hardware test sequence.
// static uint8_t qspi_read_byte(uint32_t flash_addr) {
//   mmio_write32(QSPI_BASE + REG_SSR, SSR_ASSERT_SLAVE0);

//   // reset FIFOs before transaction
//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE | CR_TXRST | CR_RXRST);

//   // cmd + 24-bit address + 2 dummy bytes
//   qspi_push_byte(CMD_READ);
//   qspi_push_byte((flash_addr >> 16) & 0xFF);
//   qspi_push_byte((flash_addr >>  8) & 0xFF);
//   qspi_push_byte((flash_addr >>  0) & 0xFF);
//   qspi_push_byte(0x00);
//   qspi_push_byte(0x00);

//   // start transaction
//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_GO);

//   qspi_wait_tx_empty();

//   // stop/inhibit further shifting
//   mmio_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE);
//   mmio_write32(QSPI_BASE + REG_SSR, SSR_DEASSERT_ALL);

//   // Drain 6 RX bytes; last one is assumed to be the returned data byte
//   (void)qspi_wait_and_pop_rx();
//   (void)qspi_wait_and_pop_rx();
//   (void)qspi_wait_and_pop_rx();
//   (void)qspi_wait_and_pop_rx();
//   (void)qspi_wait_and_pop_rx();
//   return qspi_wait_and_pop_rx();
// }

// // -----------------------------
// // Test entry
// // -----------------------------
// extern "C" int main() {
//   uart_puts("QSPI TEST START\r\n");

//   qspi_reset_and_init();
//   uart_puts("QSPI init done\r\n");

//   const uint32_t test_flash_addr = 0x00100000; // payload offset in flash
//   uart_puts("Reading flash @ ");
//   uart_puthex32(test_flash_addr);
//   uart_puts("\r\n");

//   uint8_t b = qspi_read_byte(test_flash_addr);

//   uart_puts("Read byte = 0x");
//   uart_puthex8(b);
//   uart_puts("\r\n");

//   while (1) { }
// }


// // #include "tensorflow/lite/micro/examples/person_detection/main_functions.h"
// #include <new>

// #include "tensorflow/lite/micro/examples/person_detection/detection_responder.h"
// #include "tensorflow/lite/micro/examples/person_detection/image_provider.h"
// #include "tensorflow/lite/micro/examples/person_detection/model_settings.h"
// #include "tensorflow/lite/micro/micro_interpreter.h"
// #include "tensorflow/lite/micro/micro_log.h"
// #include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
// #include "tensorflow/lite/micro/system_setup.h"
// #include "tensorflow/lite/schema/schema_generated.h"
// #include "tensorflow/lite/micro/models/person_detect_model_data.h"
// #include <stdint.h>

// #include <stdint.h>

// // #include <stdint.h>

// // ============================================================
// // Toggle this between 0 and 1 for testing
// // ============================================================
// #define USE_MMIO_BSWAP 0

// // ============================================================
// // UART Lite
// // ============================================================
// static inline volatile uint32_t* const UART_TX =
//     reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
// static inline volatile uint32_t* const UART_ST =
//     reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x08u);

// static inline void RawPutc(char c) {
//   while ((*UART_ST) & 0x08u) { }
//   *UART_TX = static_cast<uint32_t>(static_cast<uint8_t>(c));
// }

// static inline void RawNL() {
//   RawPutc('\r');
//   RawPutc('\n');
// }

// static inline void RawPutHex4(uint8_t x) {
//   x &= 0xF;
//   RawPutc((x < 10) ? ('0' + x) : ('A' + (x - 10)));
// }

// static inline void RawPutHex8(uint8_t x) {
//   RawPutHex4(x >> 4);
//   RawPutHex4(x);
// }

// static inline void RawPutHex32(uint32_t x) {
//   for (int i = 7; i >= 0; --i) {
//     RawPutHex4((x >> (i * 4)) & 0xF);
//   }
// }

// static inline void Fail(char code) {
//   RawPutc('F');
//   RawPutc(code);
//   RawNL();
//   while (1) { }
// }

// // ============================================================
// // AXI Quad SPI
// // ============================================================
// static constexpr uintptr_t QSPI_BASE = 0x44A00000u;

// static constexpr uintptr_t XSP_DGIER_OFFSET      = 0x1Cu;
// static constexpr uintptr_t XSP_IISR_OFFSET       = 0x20u;
// static constexpr uintptr_t XSP_IIER_OFFSET       = 0x28u;
// static constexpr uintptr_t XSP_SRR_OFFSET        = 0x40u;
// static constexpr uintptr_t XSP_CR_OFFSET         = 0x60u;
// static constexpr uintptr_t XSP_SR_OFFSET         = 0x64u;
// static constexpr uintptr_t XSP_DTR_OFFSET        = 0x68u;
// static constexpr uintptr_t XSP_DRR_OFFSET        = 0x6Cu;
// static constexpr uintptr_t XSP_SSR_OFFSET        = 0x70u;
// static constexpr uintptr_t XSP_TXFIFO_OR_OFFSET  = 0x74u;
// static constexpr uintptr_t XSP_RXFIFO_OR_OFFSET  = 0x78u;

// // SPICR bits
// static constexpr uint32_t XSP_CR_ENABLE_MASK         = 0x00000002u;
// static constexpr uint32_t XSP_CR_MASTER_MODE_MASK    = 0x00000004u;
// static constexpr uint32_t XSP_CR_TXFIFO_RESET_MASK   = 0x00000020u;
// static constexpr uint32_t XSP_CR_RXFIFO_RESET_MASK   = 0x00000040u;
// static constexpr uint32_t XSP_CR_MANUAL_SS_MASK      = 0x00000080u;
// static constexpr uint32_t XSP_CR_TRANS_INHIBIT_MASK  = 0x00000100u;

// // SPISR bits
// static constexpr uint32_t XSP_SR_RX_EMPTY_MASK       = 0x00000001u;
// static constexpr uint32_t XSP_SR_TX_FULL_MASK        = 0x00000008u;

// static inline uint32_t bswap32(uint32_t x) {
//   return ((x & 0x000000FFu) << 24) |
//          ((x & 0x0000FF00u) <<  8) |
//          ((x & 0x00FF0000u) >>  8) |
//          ((x & 0xFF000000u) >> 24);
// }

// static inline uint32_t raw_mmio_read(uintptr_t addr) {
//   return *reinterpret_cast<volatile uint32_t*>(addr);
// }

// static inline void raw_mmio_write(uintptr_t addr, uint32_t value) {
//   *reinterpret_cast<volatile uint32_t*>(addr) = value;
// }

// static inline uint32_t mmio_read(uintptr_t addr) {
//   uint32_t v = raw_mmio_read(addr);
// #if USE_MMIO_BSWAP
//   return bswap32(v);
// #else
//   return v;
// #endif
// }

// static inline void mmio_write(uintptr_t addr, uint32_t value) {
// #if USE_MMIO_BSWAP
//   raw_mmio_write(addr, bswap32(value));
// #else
//   raw_mmio_write(addr, value);
// #endif
// }

// static void PrintReg(char tag, uintptr_t off) {
//   uint32_t raw = raw_mmio_read(QSPI_BASE + off);
//   uint32_t swp = bswap32(raw);

//   RawPutc(tag);
//   RawPutc('=');
//   RawPutHex32(raw);
//   RawPutc('/');
//   RawPutHex32(swp);
//   RawPutc(' ');
// }

// static void DumpRegs(char tag) {
//   RawPutc(tag);
//   RawPutc(':');
//   RawPutc(' ');
//   PrintReg('C', XSP_CR_OFFSET);
//   PrintReg('S', XSP_SR_OFFSET);
//   PrintReg('R', XSP_SSR_OFFSET);
//   PrintReg('T', XSP_TXFIFO_OR_OFFSET);
//   PrintReg('Q', XSP_RXFIFO_OR_OFFSET);
//   RawNL();
// }

// static void qspi_init() {
//   RawPutc('1');

//   mmio_write(QSPI_BASE + XSP_SRR_OFFSET, 0x0000000Au);
//   RawPutc('2');

//   mmio_write(QSPI_BASE + XSP_DGIER_OFFSET, 0x00000000u);
//   mmio_write(QSPI_BASE + XSP_IIER_OFFSET,  0x00000000u);
//   mmio_write(QSPI_BASE + XSP_IISR_OFFSET,  0xFFFFFFFFu);
//   RawPutc('3');

//   uint32_t cr =
//       XSP_CR_ENABLE_MASK |
//       XSP_CR_MASTER_MODE_MASK |
//       XSP_CR_MANUAL_SS_MASK |
//       XSP_CR_TRANS_INHIBIT_MASK |
//       XSP_CR_TXFIFO_RESET_MASK |
//       XSP_CR_RXFIFO_RESET_MASK;

//   mmio_write(QSPI_BASE + XSP_CR_OFFSET, cr);
//   RawPutc('4');

//   // mmio_write(QSPI_BASE + XSP_SSR_OFFSET, 0xFFFFFFFFu);
//   RawPutc('5');
//   // RawNL();

//   // DumpRegs('I');
// }

// static inline void qspi_cs_assert() {
//   mmio_write(QSPI_BASE + XSP_SSR_OFFSET, 0xFFFFFFFEu);
// }

// static inline void qspi_cs_deassert() {
//   mmio_write(QSPI_BASE + XSP_SSR_OFFSET, 0xFFFFFFFFu);
// }

// static void qspi_drain_rx() {
//   for (int i = 0; i < 16; ++i) {
//     uint32_t sr = mmio_read(QSPI_BASE + XSP_SR_OFFSET);
//     if (sr & XSP_SR_RX_EMPTY_MASK) break;
//     (void)mmio_read(QSPI_BASE + XSP_DRR_OFFSET);
//   }
// }

// static uint8_t qspi_transfer_byte(uint8_t tx) {
//   RawPutc('a');

//   qspi_drain_rx();
//   RawPutc('b');

//   uint32_t guard = 1000000;

//   while ((mmio_read(QSPI_BASE + XSP_SR_OFFSET) & XSP_SR_TX_FULL_MASK) != 0u) {
//     if (--guard == 0u) {
//       DumpRegs('X');
//       Fail('T');
//     }
//   }
//   RawPutc('c');

//   mmio_write(QSPI_BASE + XSP_DTR_OFFSET, static_cast<uint32_t>(tx));
//   RawPutc('d');

//   uint32_t cr = mmio_read(QSPI_BASE + XSP_CR_OFFSET);
//   cr &= ~XSP_CR_TRANS_INHIBIT_MASK;
//   mmio_write(QSPI_BASE + XSP_CR_OFFSET, cr);
//   RawPutc('e');

//   guard = 1000000;
//   while ((mmio_read(QSPI_BASE + XSP_SR_OFFSET) & XSP_SR_RX_EMPTY_MASK) != 0u) {
//     if (--guard == 0u) {
//       DumpRegs('Y');
//       Fail('R');
//     }
//   }
//   RawPutc('f');

//   cr = mmio_read(QSPI_BASE + XSP_CR_OFFSET);
//   cr |= XSP_CR_TRANS_INHIBIT_MASK;
//   mmio_write(QSPI_BASE + XSP_CR_OFFSET, cr);
//   RawPutc('g');

//   uint8_t rx = static_cast<uint8_t>(mmio_read(QSPI_BASE + XSP_DRR_OFFSET) & 0xFFu);
//   RawPutc('h');

//   return rx;
// }

// static void qspi_read_jedec_id(uint8_t* id) {
//   RawPutc('J');
//   qspi_cs_assert();
//   RawPutc('K');
//   DumpRegs('A');

//   (void)qspi_transfer_byte(0x9Fu);
//   RawPutc('L');

//   id[0] = qspi_transfer_byte(0xFFu);
//   RawPutc('M');

//   id[1] = qspi_transfer_byte(0xFFu);
//   RawPutc('N');

//   id[2] = qspi_transfer_byte(0xFFu);
//   RawPutc('O');

//   qspi_cs_deassert();
//   RawPutc('P');
//   RawNL();
// }

// static void qspi_read_data(uint32_t addr, uint8_t* out, uint32_t len) {
//   RawPutc('D');
//   qspi_cs_assert();
//   RawPutc('E');

//   (void)qspi_transfer_byte(0x03u);
//   (void)qspi_transfer_byte(static_cast<uint8_t>((addr >> 16) & 0xFFu));
//   (void)qspi_transfer_byte(static_cast<uint8_t>((addr >>  8) & 0xFFu));
//   (void)qspi_transfer_byte(static_cast<uint8_t>((addr >>  0) & 0xFFu));
//   RawPutc('F');

//   for (uint32_t i = 0; i < len; ++i) {
//     out[i] = qspi_transfer_byte(0xFFu);
//   }

//   qspi_cs_deassert();
//   RawPutc('G');
//   RawNL();
// }

// extern "C" int main() {
//   RawPutc('S');
//   // RawNL();

//   qspi_init();

//   RawPutc('6');

//   RawPutc('C');
//   RawPutHex32(raw_mmio_read(QSPI_BASE + XSP_CR_OFFSET));
//   RawPutc('/');
//   RawPutHex32(bswap32(raw_mmio_read(QSPI_BASE + XSP_CR_OFFSET)));
//   RawPutc(' ');

//   uint8_t jedec[3] = {0, 0, 0};
//   qspi_read_jedec_id(jedec);

//   RawPutc('I');
//   RawPutc(':');
//   RawPutc(' ');
//   RawPutHex8(jedec[0]); RawPutc(' ');
//   RawPutHex8(jedec[1]); RawPutc(' ');
//   RawPutHex8(jedec[2]);
//   // RawNL();

//   uint8_t buf[16];
//   for (int i = 0; i < 16; ++i) buf[i] = 0;

//   qspi_read_data(0x000000u, buf, 16);

//   RawPutc('B');
//   RawPutc(':');
//   RawPutc(' ');
//   for (int i = 0; i < 16; ++i) {
//     RawPutHex8(buf[i]);
//     RawPutc(' ');
//   }
//   // RawNL();

//   RawPutc('Z');
//   RawNL();

//   while (1) { }
//   return 0;
// }