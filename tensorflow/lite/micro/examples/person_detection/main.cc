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



extern "C" int main(void) {
  
  setup();
  loop();   // run exactly once
  // while (1) {}
  return 0;
}


// #include <stdint.h>
// #include <stddef.h>

// static inline void RawPutc(char c) {
//   volatile uint32_t* const uart_tx =
//       reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
//   volatile uint32_t* const uart_status =
//       reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x08u);

//   while ((*uart_status) & 0x08u) {
//   }

//   *uart_tx = static_cast<uint32_t>(static_cast<uint8_t>(c));
// }

// static inline void RawPuts(const char* s) {
//   while (*s) RawPutc(*s++);
// }

// static inline void RawNewline() {
//   RawPutc('\r');
//   RawPutc('\n');
// }

// static inline void RawHexNibble(uint32_t v) {
//   v &= 0xF;
//   RawPutc((v < 10) ? ('0' + v) : ('A' + (v - 10)));
// }

// static inline void RawHex32(uint32_t v) {
//   for (int sh = 28; sh >= 0; sh -= 4) {
//     RawHexNibble(v >> sh);
//   }
// }

// static inline void RawTagHex(const char* tag, uint32_t v) {
//   RawPuts(tag);
//   RawHex32(v);
//   RawNewline();
// }

// // CHANGE THIS to whatever address Vivado Address Editor assigned
// // to DMA_Controller_0/s_axi.
// #define DMA_BASE 0x00010000u

// #define DMA_CH_STRIDE 0x20u
// #define DMA_SRC_ADDR  0x00u
// #define DMA_DST_ADDR  0x04u
// #define DMA_BYTE_LEN  0x08u
// #define DMA_CTRL      0x0Cu
// #define DMA_STATUS    0x10u
// #define DMA_DEBUG 0x14u

// #define DMA_CTRL_START 0x00000001u
// #define DMA_STATUS_BUSY 0x00000001u
// #define DMA_STATUS_DONE 0x00000002u

// static inline void mmio_write32(uint32_t addr, uint32_t value) {
//   *(volatile uint32_t*)addr = value;
// }

// static inline uint32_t mmio_read32(uint32_t addr) {
//   return *(volatile uint32_t*)addr;
// }

// static inline uint32_t dma_reg(uint32_t ch, uint32_t off) {
//   return DMA_BASE + ch * DMA_CH_STRIDE + off;
// }

// alignas(8) static volatile uint64_t dma_src[8];
// alignas(8) static volatile uint64_t dma_dst[8];

// extern "C" int main() {
//   RawNewline();
//   RawPuts("=== DMA COPY TEST ===");
//   RawNewline();

//   for (int i = 0; i < 8; i++) {
//     dma_src[i] = 0x1111000000000000ULL + (uint64_t)i;
//     dma_dst[i] = 0xDEADBEEFDEADBEEFULL;
//   }

//   RawTagHex("SRC=", (uint32_t)(uintptr_t)dma_src);
//   RawTagHex("DST=", (uint32_t)(uintptr_t)dma_dst);

//   const uint32_t ch = 0;
//   const uint32_t len_bytes = sizeof(dma_src);  // 8 words * 8 bytes = 64 bytes

//   // Optional: clear DONE using W1C.
//   mmio_write32(dma_reg(ch, DMA_STATUS), DMA_STATUS_DONE);

//   // RawPuts("Programming DMA...");
//   // RawNewline();

//   // mmio_write32(dma_reg(ch, DMA_SRC_ADDR),  (uint32_t)(uintptr_t)dma_src);
//   // mmio_write32(dma_reg(ch, DMA_DST_ADDR),  (uint32_t)(uintptr_t)dma_dst);
//   // mmio_write32(dma_reg(ch, DMA_BYTE_LEN),  len_bytes);
//   // mmio_write32(dma_reg(ch, DMA_CTRL),      DMA_CTRL_START);

//   // RawPuts("Waiting DMA...");
//   // RawNewline();

//   RawPuts("DMA REG READBACK");
//   RawNewline();

//   mmio_write32(dma_reg(ch, DMA_SRC_ADDR),  (uint32_t)(uintptr_t)dma_src);
//   mmio_write32(dma_reg(ch, DMA_DST_ADDR),  (uint32_t)(uintptr_t)dma_dst);
//   mmio_write32(dma_reg(ch, DMA_BYTE_LEN),  len_bytes);
//   mmio_write32(dma_reg(ch, DMA_CTRL),      0);

//   RawTagHex("R_SRC=", mmio_read32(dma_reg(ch, DMA_SRC_ADDR)));
//   RawTagHex("R_DST=", mmio_read32(dma_reg(ch, DMA_DST_ADDR)));
//   RawTagHex("R_LEN=", mmio_read32(dma_reg(ch, DMA_BYTE_LEN)));
//   RawTagHex("R_CTL=", mmio_read32(dma_reg(ch, DMA_CTRL)));
//   RawTagHex("R_STA=", mmio_read32(dma_reg(ch, DMA_STATUS)));

//   // mmio_write32(dma_reg(ch, DMA_CTRL), 1);

//   // RawTagHex("R_CTL2=", mmio_read32(dma_reg(ch, DMA_CTRL)));
//   // RawTagHex("R_STA2=", mmio_read32(dma_reg(ch, DMA_STATUS)));



//   mmio_write32(dma_reg(ch, DMA_CTRL), DMA_CTRL_START);

//   for (volatile int d = 0; d < 1000; d++) {
//   }

//   RawTagHex("R_CTL2=", mmio_read32(dma_reg(ch, DMA_CTRL)));
//   RawTagHex("R_STA2=", mmio_read32(dma_reg(ch, DMA_STATUS)));

//   for (int k = 0; k < 8; k++) {
//     for (volatile int d = 0; d < 1000; d++) {
//     }
//     RawTagHex("DBG=", mmio_read32(dma_reg(ch, DMA_DEBUG)));
//     RawTagHex("STA=", mmio_read32(dma_reg(ch, DMA_STATUS)));
//   }

//   uint32_t timeout = 10000000;
//   uint32_t status = 0;

//   while (timeout--) {
//     status = mmio_read32(dma_reg(ch, DMA_STATUS));

//     if (status & DMA_STATUS_DONE) {
//       break;
//     }
//   }

//   RawTagHex("STATUS=", status);

//   if (!(status & DMA_STATUS_DONE)) {
//     RawPuts("DMA TIMEOUT");
//     RawNewline();
//     while (1) {}
//   }

//   int pass = 1;

//   for (int i = 0; i < 8; i++) {
//     uint64_t s = dma_src[i];
//     uint64_t d = dma_dst[i];

//     if (s != d) {
//       pass = 0;
//       RawPuts("MISMATCH i=");
//       RawHex32((uint32_t)i);
//       RawPuts(" SRC_LO=");
//       RawHex32((uint32_t)s);
//       RawPuts(" DST_LO=");
//       RawHex32((uint32_t)d);
//       RawNewline();
//     }
//   }

//   if (pass) {
//     RawPuts("DMA PASS");
//     RawNewline();
//   } else {
//     RawPuts("DMA FAIL");
//     RawNewline();
//   }

//   while (1) {}
// }