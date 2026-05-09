// #include <stdint.h>
// #include <stddef.h>

// // ============================================================
// // UART helpers
// // ============================================================

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

// static inline void RawSpace() {
//   RawPutc(' ');
// }

// static inline void RawHexNibble(uint32_t v) {
//   v &= 0xFu;
//   RawPutc((v < 10u) ? static_cast<char>('0' + v)
//                     : static_cast<char>('A' + (v - 10u)));
// }

// static inline void RawHex8(uint32_t v) {
//   RawHexNibble(v >> 4);
//   RawHexNibble(v);
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

// static inline void RawPrintI8Flat(const char* name,
//                                   const volatile int8_t* data,
//                                   int n) {
//   RawPuts(name);
//   RawNewline();

//   for (int i = 0; i < n; i++) {
//     RawHex8(static_cast<uint32_t>(static_cast<uint8_t>(data[i])));
//     RawSpace();
//   }

//   RawNewline();
//   RawNewline();
// }

// static inline void RawPrintI32Flat(const char* name,
//                                    const volatile int32_t* data,
//                                    int n) {
//   RawPuts(name);
//   RawNewline();

//   for (int i = 0; i < n; i++) {
//     RawHex32(static_cast<uint32_t>(data[i]));
//     RawSpace();
//   }

//   RawNewline();
//   RawNewline();
// }

// // ============================================================
// // DONE mailbox
// // ============================================================

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

// // ============================================================
// // DMA register map
// // ============================================================

// #define DMA_BASE 0x00010000u

// #define DMA_CH_STRIDE 0x20u
// #define DMA_SRC_ADDR  0x00u
// #define DMA_DST_ADDR  0x04u
// #define DMA_BYTE_LEN  0x08u
// #define DMA_CTRL      0x0Cu
// #define DMA_STATUS    0x10u
// #define DMA_SPAD_SEL  0x14u

// #define DMA_CTRL_START  0x00000001u
// #define DMA_STATUS_DONE 0x00000002u

// #define SPAD_WEIGHTS 0x00000000u
// #define SPAD_IFMAPS  0x00000001u

// static inline void mmio_write32(uint32_t addr, uint32_t value) {
//   *(volatile uint32_t*)addr = value;
// }

// static inline uint32_t mmio_read32(uint32_t addr) {
//   return *(volatile uint32_t*)addr;
// }

// static inline uint32_t dma_reg(uint32_t ch, uint32_t off) {
//   return DMA_BASE + ch * DMA_CH_STRIDE + off;
// }

// static inline uint32_t bswap32(uint32_t v) {
//   return ((v & 0x000000FFu) << 24) |
//          ((v & 0x0000FF00u) << 8)  |
//          ((v & 0x00FF0000u) >> 8)  |
//          ((v & 0xFF000000u) >> 24);
// }

// static inline void dma_write32(uint32_t ch, uint32_t off, uint32_t value) {
//   mmio_write32(dma_reg(ch, off), bswap32(value));
// }

// static inline uint32_t dma_read32(uint32_t ch, uint32_t off) {
//   return bswap32(mmio_read32(dma_reg(ch, off)));
// }

// static int DmaCopyBlocking(uint32_t ch,
//                            uint32_t src_addr,
//                            uint32_t dst_addr,
//                            uint32_t len_bytes,
//                            uint32_t spad_sel,
//                            const char* name) {
//   RawPuts("DMA ");
//   RawPuts(name);
//   RawNewline();

//   RawTagHex("SRC=", src_addr);
//   RawTagHex("DST=", dst_addr);
//   RawTagHex("LEN=", len_bytes);
//   RawTagHex("SPAD=", spad_sel);

//   dma_write32(ch, DMA_STATUS, DMA_STATUS_DONE);

//   for (volatile int d = 0; d < 1000; d++) {
//   }

//   dma_write32(ch, DMA_SRC_ADDR, src_addr);
//   dma_write32(ch, DMA_DST_ADDR, dst_addr);
//   dma_write32(ch, DMA_BYTE_LEN, len_bytes);
//   dma_write32(ch, DMA_SPAD_SEL, spad_sel);
//   dma_write32(ch, DMA_CTRL, 0);

//   RawTagHex("R_SRC=", dma_read32(ch, DMA_SRC_ADDR));
//   RawTagHex("R_DST=", dma_read32(ch, DMA_DST_ADDR));
//   RawTagHex("R_LEN=", dma_read32(ch, DMA_BYTE_LEN));
//   RawTagHex("R_STA=", dma_read32(ch, DMA_STATUS));
//   RawTagHex("R_SPAD=", dma_read32(ch, DMA_SPAD_SEL));

//   dma_write32(ch, DMA_CTRL, DMA_CTRL_START);

//   uint32_t timeout = 10000000u;
//   uint32_t status = 0;

//   while (timeout--) {
//     status = dma_read32(ch, DMA_STATUS);

//     if (status & DMA_STATUS_DONE) {
//       break;
//     }
//   }

//   RawTagHex("STATUS=", status);

//   dma_write32(ch, DMA_CTRL, 0);

//   if (!(status & DMA_STATUS_DONE)) {
//     RawPuts("DMA TIMEOUT ");
//     RawPuts(name);
//     RawNewline();
//     return 0;
//   }

//   RawPuts("DMA DONE ");
//   RawPuts(name);
//   RawNewline();
//   RawNewline();

//   return 1;
// }

// // ============================================================
// // Fixed 3x3 Conv2D test shapes
// //
// // Input:  [1, 5, 5, 2]
// // Filter: [2, 3, 3, 2] OHWI
// // Output: [1, 3, 3, 2]
// // ============================================================

// #define B   1
// #define H   5
// #define W   5
// #define C   2

// #define OC  2
// #define KH  3
// #define KW  3
// #define IC  2

// #define OH  3
// #define OW  3

// #define INPUT_SIZE   (B * H * W * C)
// #define FILTER_SIZE  (OC * KH * KW * IC)
// #define OUTPUT_SIZE  (B * OH * OW * OC)

// #define TILE_USEFUL_BYTES 9
// #define TILE_DMA_BYTES    12

// alignas(8) static volatile int8_t g_input[INPUT_SIZE];
// alignas(8) static volatile int8_t g_filter[FILTER_SIZE];

// alignas(8) static volatile int32_t g_golden[OUTPUT_SIZE];
// alignas(8) static volatile int32_t g_tiled[OUTPUT_SIZE];
// alignas(8) static volatile int32_t g_output_tile[1];

// // Fixed DRAM tile buffer addresses.
// // Source buffers receive extracted tiles.
// // Destination buffers receive DMA-copied tiles.
// // Compute uses the destination buffers.
// #define INPUT_TILE_SRC_ADDR   0x80001000u
// #define INPUT_TILE_DST_ADDR   0x80002000u
// #define WEIGHT_TILE_SRC_ADDR  0x80003000u
// #define WEIGHT_TILE_DST_ADDR  0x80004000u

// static volatile int8_t* const input_tile_src =
//     reinterpret_cast<volatile int8_t*>(INPUT_TILE_SRC_ADDR);
// static volatile int8_t* const input_tile_dst =
//     reinterpret_cast<volatile int8_t*>(INPUT_TILE_DST_ADDR);
// static volatile int8_t* const weight_tile_src =
//     reinterpret_cast<volatile int8_t*>(WEIGHT_TILE_SRC_ADDR);
// static volatile int8_t* const weight_tile_dst =
//     reinterpret_cast<volatile int8_t*>(WEIGHT_TILE_DST_ADDR);

// // ============================================================
// // Index helpers
// // ============================================================

// static inline int IndexNHWC(int b, int h, int w, int c,
//                             int full_h, int full_w, int full_c) {
//   return ((b * full_h + h) * full_w + w) * full_c + c;
// }

// static inline int IndexOHWI(int oc, int kh, int kw, int ic,
//                             int full_kh, int full_kw, int full_ic) {
//   return (((oc * full_kh + kh) * full_kw + kw) * full_ic) + ic;
// }

// // ============================================================
// // Basic helpers
// // ============================================================

// static void ClearI32(volatile int32_t* data, int n) {
//   for (int i = 0; i < n; i++) {
//     data[i] = 0;
//   }
// }

// static void ClearI8(volatile int8_t* data, int n) {
//   for (int i = 0; i < n; i++) {
//     data[i] = 0;
//   }
// }

// static void FillFixedData() {
//   int8_t v = -4;

//   for (int i = 0; i < INPUT_SIZE; i++) {
//     g_input[i] = v;

//     v++;
//     if (v > 4) {
//       v = -4;
//     }
//   }

//   v = -3;

//   for (int i = 0; i < FILTER_SIZE; i++) {
//     g_filter[i] = v;

//     v++;
//     if (v > 3) {
//       v = -3;
//     }
//   }
// }

// static int CompareI8Buffers(const char* name,
//                             const volatile int8_t* a,
//                             const volatile int8_t* b,
//                             int n) {
//   int pass = 1;

//   for (int i = 0; i < n; i++) {
//     if (a[i] != b[i]) {
//       pass = 0;

//       RawPuts("MISMATCH ");
//       RawPuts(name);
//       RawPuts(" i=");
//       RawHex32(static_cast<uint32_t>(i));
//       RawPuts(" A=");
//       RawHex32(static_cast<uint32_t>(static_cast<uint8_t>(a[i])));
//       RawPuts(" B=");
//       RawHex32(static_cast<uint32_t>(static_cast<uint8_t>(b[i])));
//       RawNewline();
//     }
//   }

//   if (!pass) {
//     RawPuts(name);
//     RawPuts(" FAIL");
//     RawNewline();
//   }

//   return pass;
// }

// // ============================================================
// // Full 3x3 Conv2D reference
// // ============================================================

// static void FullConv3x3() {
//   RawPuts("FULL A");
//   RawNewline();

//   ClearI32(g_golden, OUTPUT_SIZE);

//   RawPuts("FULL B");
//   RawNewline();

//   for (int b = 0; b < B; b++) {
//     for (int oh = 0; oh < OH; oh++) {
//       for (int ow = 0; ow < OW; ow++) {
//         for (int oc = 0; oc < OC; oc++) {
//           int32_t acc = 0;

//           for (int kh = 0; kh < KH; kh++) {
//             for (int kw = 0; kw < KW; kw++) {
//               for (int ic = 0; ic < IC; ic++) {
//                 const int input_idx =
//                     IndexNHWC(b, oh + kh, ow + kw, ic, H, W, C);

//                 const int filter_idx =
//                     IndexOHWI(oc, kh, kw, ic, KH, KW, IC);

//                 acc += static_cast<int32_t>(g_input[input_idx]) *
//                        static_cast<int32_t>(g_filter[filter_idx]);
//               }
//             }
//           }

//           const int out_idx = IndexNHWC(b, oh, ow, oc, OH, OW, OC);
//           g_golden[out_idx] = acc;
//         }
//       }
//     }
//   }

//   RawPuts("FULL C");
//   RawNewline();
// }

// // ============================================================
// // Extract one 3x3 input tile for one input channel
// // ============================================================

// static void ExtractInput3x3TileToSrc(int b, int oh, int ow, int ic) {
//   ClearI8(input_tile_src, TILE_DMA_BYTES);

//   for (int kh = 0; kh < KH; kh++) {
//     for (int kw = 0; kw < KW; kw++) {
//       const int tile_idx = kh * KW + kw;
//       const int input_idx = IndexNHWC(b, oh + kh, ow + kw, ic, H, W, C);

//       input_tile_src[tile_idx] = g_input[input_idx];
//     }
//   }
// }

// // ============================================================
// // Extract one 3x3 weight tile for one OC and one IC
// // ============================================================

// static void ExtractWeight3x3TileToSrc(int oc, int ic) {
//   ClearI8(weight_tile_src, TILE_DMA_BYTES);

//   for (int kh = 0; kh < KH; kh++) {
//     for (int kw = 0; kw < KW; kw++) {
//       const int tile_idx = kh * KW + kw;
//       const int filter_idx = IndexOHWI(oc, kh, kw, ic, KH, KW, IC);

//       weight_tile_src[tile_idx] = g_filter[filter_idx];
//     }
//   }
// }

// // ============================================================
// // Compute one output-tile partial from DMA destination buffers
// // ============================================================

// static int32_t ComputeTilePartialFromDmaDst() {
//   int32_t partial = 0;

//   for (int k = 0; k < TILE_USEFUL_BYTES; k++) {
//     partial += static_cast<int32_t>(input_tile_dst[k]) *
//                static_cast<int32_t>(weight_tile_dst[k]);
//   }

//   return partial;
// }

// // ============================================================
// // Write output tile back to full output
// // ============================================================

// static void WriteBackOutputTile(int b, int oh, int ow, int oc) {
//   const int out_idx = IndexNHWC(b, oh, ow, oc, OH, OW, OC);

//   g_tiled[out_idx] = g_output_tile[0];
// }

// // ============================================================
// // Tiled 3x3 Conv2D with DMA tile movement
// //
// // For each output element:
// //   clear output tile
// //   for each input channel:
// //     extract input tile to source DRAM buffer
// //     DMA input tile source -> input tile destination
// //     extract weight tile to source DRAM buffer
// //     DMA weight tile source -> weight tile destination
// //     compute from DMA destination buffers
// //     accumulate into output tile
// //   write output tile back
// // ============================================================

// static int TiledConv3x3WithDmaTiles() {
//   RawPuts("TILE A");
//   RawNewline();

//   ClearI32(g_tiled, OUTPUT_SIZE);

//   RawPuts("TILE B");
//   RawNewline();

//   int pass = 1;

//   for (int b = 0; b < B; b++) {
//     for (int oh = 0; oh < OH; oh++) {
//       for (int ow = 0; ow < OW; ow++) {
//         for (int oc = 0; oc < OC; oc++) {
//           ClearI32(g_output_tile, 1);

//           for (int ic = 0; ic < IC; ic++) {
//             ExtractInput3x3TileToSrc(b, oh, ow, ic);
//             ClearI8(input_tile_dst, TILE_DMA_BYTES);

//             if (!DmaCopyBlocking(0,
//                                  INPUT_TILE_SRC_ADDR,
//                                  INPUT_TILE_DST_ADDR,
//                                  TILE_DMA_BYTES,
//                                  SPAD_IFMAPS,
//                                  "INPUT_TILE")) {
//               pass = 0;
//             }

//             if (!CompareI8Buffers("INPUT_TILE_DMA",
//                                   input_tile_src,
//                                   input_tile_dst,
//                                   TILE_USEFUL_BYTES)) {
//               pass = 0;
//             }

//             ExtractWeight3x3TileToSrc(oc, ic);
//             ClearI8(weight_tile_dst, TILE_DMA_BYTES);

//             if (!DmaCopyBlocking(0,
//                                  WEIGHT_TILE_SRC_ADDR,
//                                  WEIGHT_TILE_DST_ADDR,
//                                  TILE_DMA_BYTES,
//                                  SPAD_WEIGHTS,
//                                  "WEIGHT_TILE")) {
//               pass = 0;
//             }

//             if (!CompareI8Buffers("WEIGHT_TILE_DMA",
//                                   weight_tile_src,
//                                   weight_tile_dst,
//                                   TILE_USEFUL_BYTES)) {
//               pass = 0;
//             }

//             const int32_t partial = ComputeTilePartialFromDmaDst();

//             g_output_tile[0] += partial;
//           }

//           WriteBackOutputTile(b, oh, ow, oc);
//         }
//       }
//     }
//   }

//   RawPuts("TILE C");
//   RawNewline();

//   return pass;
// }

// // ============================================================
// // Compare final Conv2D outputs
// // ============================================================

// static int CompareOutputs() {
//   int pass = 1;

//   for (int i = 0; i < OUTPUT_SIZE; i++) {
//     if (g_golden[i] != g_tiled[i]) {
//       pass = 0;

//       RawPuts("MISMATCH OUT i=");
//       RawHex32(static_cast<uint32_t>(i));
//       RawPuts(" G=");
//       RawHex32(static_cast<uint32_t>(g_golden[i]));
//       RawPuts(" T=");
//       RawHex32(static_cast<uint32_t>(g_tiled[i]));
//       RawNewline();
//     }
//   }

//   RawPuts("COMPARE ");
//   RawPuts(pass ? "PASS" : "FAIL");
//   RawNewline();

//   return pass;
// }

// // ============================================================
// // Main
// // ============================================================

// extern "C" int main() {
//   RawNewline();
//   RawPuts("=== 3X3 CONV2D DMA TILE SOFTWARE TEST ===");
//   RawNewline();
//   RawNewline();

//   RawTagHex("IN_SRC_ADDR=", INPUT_TILE_SRC_ADDR);
//   RawTagHex("IN_DST_ADDR=", INPUT_TILE_DST_ADDR);
//   RawTagHex("WT_SRC_ADDR=", WEIGHT_TILE_SRC_ADDR);
//   RawTagHex("WT_DST_ADDR=", WEIGHT_TILE_DST_ADDR);
//   RawNewline();

//   RawPuts("M0");
//   RawNewline();

//   FillFixedData();

//   RawPuts("M1");
//   RawNewline();

//   RawPrintI8Flat("INPUT", g_input, INPUT_SIZE);
//   RawPrintI8Flat("FILTER", g_filter, FILTER_SIZE);

//   RawPuts("M2");
//   RawNewline();

//   FullConv3x3();

//   RawPuts("M3");
//   RawNewline();

//   const int dma_tile_pass = TiledConv3x3WithDmaTiles();

//   RawPuts("M4");
//   RawNewline();

//   RawPrintI32Flat("GOLDEN", g_golden, OUTPUT_SIZE);
//   RawPrintI32Flat("TILED", g_tiled, OUTPUT_SIZE);

//   RawPuts("M5");
//   RawNewline();

//   const int conv_pass = CompareOutputs();

//   RawPuts("DMA_TILE ");
//   RawPuts(dma_tile_pass ? "PASS" : "FAIL");
//   RawNewline();

//   RawPuts("OVERALL ");
//   RawPuts((dma_tile_pass && conv_pass) ? "PASS" : "FAIL");
//   RawNewline();

//   WriteDoneMailbox();

//   while (1) {
//   }

//   return 0;
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
  loop();   // run exactly once
  // while (1) {}
  return 0;
}