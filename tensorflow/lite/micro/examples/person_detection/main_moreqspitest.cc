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

#include <stdint.h>

static constexpr uintptr_t UART_BASE = 0x40600000;
static constexpr uintptr_t QSPI_BASE = 0x44A00000;

static constexpr uint32_t REG_SRR    = 0x40;
static constexpr uint32_t REG_SPICR  = 0x60;
static constexpr uint32_t REG_SPISR  = 0x64;
static constexpr uint32_t REG_DTR    = 0x68;
static constexpr uint32_t REG_DRR    = 0x6C;
static constexpr uint32_t REG_SSR    = 0x70;

static constexpr uint32_t CR_SPE    = (1u << 1);
static constexpr uint32_t CR_MASTER = (1u << 2);
static constexpr uint32_t CR_TXRST  = (1u << 5);
static constexpr uint32_t CR_RXRST  = (1u << 6);
static constexpr uint32_t CR_MT_INH = (1u << 8);

static constexpr uint32_t SR_RX_EMPTY = (1u << 0);
static constexpr uint32_t SR_TX_EMPTY = (1u << 2);

static constexpr uint32_t SSR_ASSERT_SLAVE0 = 0x00000000;
static constexpr uint32_t SSR_DEASSERT_ALL  = 0x00000001;

static constexpr uint32_t SPICR_IDLE = CR_SPE | CR_MASTER | CR_MT_INH;
static constexpr uint32_t SPICR_GO   = CR_SPE | CR_MASTER;

static constexpr uint8_t CMD_READ = 0x03;

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
  volatile uint32_t* status = reinterpret_cast<volatile uint32_t*>(UART_BASE + 0x08);
  volatile uint8_t*  txfifo = reinterpret_cast<volatile uint8_t*>(UART_BASE + 0x04);
  while ((*status) & 0x08) { }
  *txfifo = static_cast<uint8_t>(c);
}

static void uart_puts(const char* s) {
  while (*s) uart_putc(*s++);
}

static void uart_puthex8(uint8_t v) {
  const char* hex = "0123456789ABCDEF";
  uart_putc(hex[(v >> 4) & 0xF]);
  uart_putc(hex[v & 0xF]);
}

static void uart_puthex32(uint32_t v) {
  const char* hex = "0123456789ABCDEF";
  for (int i = 7; i >= 0; --i) {
    uart_putc(hex[(v >> (i * 4)) & 0xF]);
  }
}

static void uart_newline() {
  uart_puts("\r\n");
}

static void qspi_init_only() {
  qspi_write32(QSPI_BASE + REG_SRR, 0x0000000A);
  qspi_write32(QSPI_BASE + REG_SSR, SSR_DEASSERT_ALL);
  qspi_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE | CR_TXRST | CR_RXRST);
  qspi_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE);
}

static void qspi_push_byte(uint8_t b) {
  qspi_write32(QSPI_BASE + REG_DTR, b);
}

static uint8_t qspi_pop_byte() {
  return static_cast<uint8_t>(qspi_read32(QSPI_BASE + REG_DRR) & 0xFF);
}

static uint8_t qspi_read_byte(uint32_t flash_addr) {
  qspi_write32(QSPI_BASE + REG_SSR, SSR_ASSERT_SLAVE0);
  qspi_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE | CR_TXRST | CR_RXRST);

  qspi_push_byte(CMD_READ);
  qspi_push_byte((flash_addr >> 16) & 0xFF);
  qspi_push_byte((flash_addr >>  8) & 0xFF);
  qspi_push_byte((flash_addr >>  0) & 0xFF);
  qspi_push_byte(0x00);  // clock in one data byte

  qspi_write32(QSPI_BASE + REG_SPICR, SPICR_GO);

  int timeout = 10000;
  while (((qspi_read32(QSPI_BASE + REG_SPISR) & SR_TX_EMPTY) == 0) && --timeout) { }

  qspi_write32(QSPI_BASE + REG_SPICR, SPICR_IDLE);
  qspi_write32(QSPI_BASE + REG_SSR, SSR_DEASSERT_ALL);

  // Drain 5 RX bytes; the last one should be the returned data byte.
  uint8_t last = 0xFF;
  for (int i = 0; i < 5; ++i) {
    int rxto = 10000;
    while ((qspi_read32(QSPI_BASE + REG_SPISR) & SR_RX_EMPTY) && --rxto) { }
    if (!rxto) return 0xEE;
    last = qspi_pop_byte();
  }
  return last;
}

static void dump4(uint32_t base) {
  uart_puts("ADDR ");
  uart_puthex32(base);
  uart_puts(": ");
  for (int i = 0; i < 4; ++i) {
    uint8_t b = qspi_read_byte(base + i);
    uart_puthex8(b);
    uart_putc(' ');
  }
  uart_newline();
}

int main() {
  uart_puts("QSPI SCAN START\r\n");
  qspi_init_only();

  // expected location
  dump4(0x00100000);
  dump4(0x00100001);

  // suspicious half-address location
  dump4(0x00080000);
  dump4(0x00080001);

  // nearby sanity checks
  dump4(0x00000000);
  dump4(0x00000010);

  while (1) { }
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