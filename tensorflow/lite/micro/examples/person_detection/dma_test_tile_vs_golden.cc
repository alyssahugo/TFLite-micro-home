#include <stdint.h>
#include <stddef.h>

// ============================================================
// UART helpers
// ============================================================

static inline void RawPutc(char c) {
  volatile uint32_t* const uart_tx =
      reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
  volatile uint32_t* const uart_status =
      reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x08u);

  while ((*uart_status) & 0x08u) {
  }

  *uart_tx = static_cast<uint32_t>(static_cast<uint8_t>(c));
}

static inline void RawPuts(const char* s) {
  while (*s) RawPutc(*s++);
}

static inline void RawNewline() {
  RawPutc('\r');
  RawPutc('\n');
}

static inline void RawSpace() {
  RawPutc(' ');
}

static inline void RawHexNibble(uint32_t v) {
  v &= 0xFu;
  RawPutc((v < 10u) ? static_cast<char>('0' + v)
                    : static_cast<char>('A' + (v - 10u)));
}

static inline void RawHex8(uint32_t v) {
  RawHexNibble(v >> 4);
  RawHexNibble(v);
}

static inline void RawHex32(uint32_t v) {
  for (int sh = 28; sh >= 0; sh -= 4) {
    RawHexNibble(v >> sh);
  }
}

static inline void RawTagHex(const char* tag, uint32_t v) {
  RawPuts(tag);
  RawHex32(v);
  RawNewline();
}

static inline void RawPrintI32Flat(const char* name,
                                   const volatile int32_t* data,
                                   int n) {
  RawPuts(name);
  RawNewline();

  for (int i = 0; i < n; i++) {
    RawHex32(static_cast<uint32_t>(data[i]));
    RawSpace();
  }

  RawNewline();
  RawNewline();
}

// ============================================================
// DONE mailbox
//
// Useful if your testbench watches these addresses.
// ============================================================

static inline void WriteDoneMailbox() {
  volatile uint32_t* const sig0 =
      reinterpret_cast<volatile uint32_t*>(0x000FFFF4u);
  volatile uint32_t* const sig1 =
      reinterpret_cast<volatile uint32_t*>(0x000FFFF8u);
  volatile uint32_t* const done =
      reinterpret_cast<volatile uint32_t*>(0x000FFFFCu);

  *sig0 = 0x11111111u;
  *sig1 = 0x22222222u;
  *done = 0x00000001u;
}

// ============================================================
// Index helpers
//
// NHWC:
//   input/output[b][h][w][c]
//
// OHWI:
//   filter[oc][kh][kw][ic]
// ============================================================

static inline int IndexNHWC(int b, int h, int w, int c,
                            int H, int W, int C) {
  return ((b * H + h) * W + w) * C + c;
}

static inline int IndexOHWI(int oc, int kh, int kw, int ic,
                            int KH, int KW, int IC) {
  return (((oc * KH + kh) * KW + kw) * IC) + ic;
}

static inline int MinInt(int a, int b) {
  return (a < b) ? a : b;
}

// ============================================================
// Small helpers
// ============================================================

static void ClearI32(volatile int32_t* data, int n) {
  for (int i = 0; i < n; i++) {
    data[i] = 0;
  }
}

static void ClearI8(volatile int8_t* data, int n) {
  for (int i = 0; i < n; i++) {
    data[i] = 0;
  }
}

static void FillInputPattern(volatile int8_t* data, int n) {
  for (int i = 0; i < n; i++) {
    data[i] = static_cast<int8_t>((i % 9) - 4);
  }
}

static void FillWeightPattern(volatile int8_t* data, int n) {
  for (int i = 0; i < n; i++) {
    data[i] = static_cast<int8_t>((i % 7) - 3);
  }
}

// ============================================================
// Input tiling
//
// Copies one NHWC tile from the full input.
// If the tile goes outside the input, it writes 0.
// ============================================================

static void ExtractInputTileNHWC(const volatile int8_t* input,
                                 int input_b,
                                 int input_h,
                                 int input_w,
                                 int input_c,
                                 int b_start,
                                 int h_start,
                                 int w_start,
                                 int c_start,
                                 int tile_b,
                                 int tile_h,
                                 int tile_w,
                                 int tile_c,
                                 volatile int8_t* tile) {
  for (int tb = 0; tb < tile_b; tb++) {
    for (int th = 0; th < tile_h; th++) {
      for (int tw = 0; tw < tile_w; tw++) {
        for (int tc = 0; tc < tile_c; tc++) {
          const int ib = b_start + tb;
          const int ih = h_start + th;
          const int iw = w_start + tw;
          const int ic = c_start + tc;

          const int tile_idx =
              ((tb * tile_h + th) * tile_w + tw) * tile_c + tc;

          if (ib < 0 || ib >= input_b ||
              ih < 0 || ih >= input_h ||
              iw < 0 || iw >= input_w ||
              ic < 0 || ic >= input_c) {
            tile[tile_idx] = 0;
          } else {
            const int input_idx =
                IndexNHWC(ib, ih, iw, ic, input_h, input_w, input_c);
            tile[tile_idx] = input[input_idx];
          }
        }
      }
    }
  }
}

// ============================================================
// Conv2D weight tiling
//
// Copies one OHWI tile from the full filter.
// ============================================================

static void ExtractConvWeightTileOHWI(const volatile int8_t* filter,
                                      int full_oc,
                                      int full_kh,
                                      int full_kw,
                                      int full_ic,
                                      int oc_start,
                                      int kh_start,
                                      int kw_start,
                                      int ic_start,
                                      int tile_oc,
                                      int tile_kh,
                                      int tile_kw,
                                      int tile_ic,
                                      volatile int8_t* weight_tile) {
  for (int toc = 0; toc < tile_oc; toc++) {
    for (int tkh = 0; tkh < tile_kh; tkh++) {
      for (int tkw = 0; tkw < tile_kw; tkw++) {
        for (int tic = 0; tic < tile_ic; tic++) {
          const int oc = oc_start + toc;
          const int kh = kh_start + tkh;
          const int kw = kw_start + tkw;
          const int ic = ic_start + tic;

          const int tile_idx =
              (((toc * tile_kh + tkh) * tile_kw + tkw) * tile_ic) + tic;

          if (oc < 0 || oc >= full_oc ||
              kh < 0 || kh >= full_kh ||
              kw < 0 || kw >= full_kw ||
              ic < 0 || ic >= full_ic) {
            weight_tile[tile_idx] = 0;
          } else {
            const int full_idx = IndexOHWI(oc, kh, kw, ic,
                                           full_kh, full_kw, full_ic);
            weight_tile[tile_idx] = filter[full_idx];
          }
        }
      }
    }
  }
}

// ============================================================
// Full Conv2D reference
//
// This is the normal non-tiled Conv2D.
// It computes the golden result.
// Valid padding only.
// Stride = 1.
// No quantization yet.
// ============================================================

static void FullConv2DValidOHWI(const volatile int8_t* input,
                                int B,
                                int H,
                                int W,
                                int C,
                                const volatile int8_t* filter,
                                int OC,
                                int KH,
                                int KW,
                                int IC,
                                int OH,
                                int OW,
                                volatile int32_t* output) {
  for (int b = 0; b < B; b++) {
    for (int oh = 0; oh < OH; oh++) {
      for (int ow = 0; ow < OW; ow++) {
        for (int oc = 0; oc < OC; oc++) {
          int32_t acc = 0;

          for (int kh = 0; kh < KH; kh++) {
            for (int kw = 0; kw < KW; kw++) {
              for (int ic = 0; ic < IC; ic++) {
                const int input_idx =
                    IndexNHWC(b, oh + kh, ow + kw, ic, H, W, C);
                const int filter_idx =
                    IndexOHWI(oc, kh, kw, ic, KH, KW, IC);

                acc += static_cast<int32_t>(input[input_idx]) *
                       static_cast<int32_t>(filter[filter_idx]);
              }
            }
          }

          const int out_idx = IndexNHWC(b, oh, ow, oc, OH, OW, OC);
          output[out_idx] = acc;
        }
      }
    }
  }
}

// ============================================================
// Tiled Conv2D software version
//
// This mimics the accelerator tiling idea:
//   1. extract input tile
//   2. extract weight tile
//   3. compute small tile
//   4. accumulate into full output
//
// Still software only.
// No DMA.
// No accelerator.
// ============================================================

static void TiledConv2DValidOHWI(const volatile int8_t* input,
                                 int B,
                                 int H,
                                 int W,
                                 int C,
                                 const volatile int8_t* filter,
                                 int OC,
                                 int KH,
                                 int KW,
                                 int IC,
                                 int OH,
                                 int OW,
                                 int tile_oh,
                                 int tile_ow,
                                 int tile_oc,
                                 int tile_ic,
                                 volatile int32_t* output) {
  static volatile int8_t input_tile[128];
  static volatile int8_t weight_tile[128];

  ClearI32(output, B * OH * OW * OC);

  for (int b = 0; b < B; b++) {
    for (int oh_start = 0; oh_start < OH; oh_start += tile_oh) {
      for (int ow_start = 0; ow_start < OW; ow_start += tile_ow) {
        for (int oc_start = 0; oc_start < OC; oc_start += tile_oc) {
          for (int ic_start = 0; ic_start < IC; ic_start += tile_ic) {
            const int actual_oh = MinInt(tile_oh, OH - oh_start);
            const int actual_ow = MinInt(tile_ow, OW - ow_start);
            const int actual_oc = MinInt(tile_oc, OC - oc_start);
            const int actual_ic = MinInt(tile_ic, IC - ic_start);

            const int input_tile_h = actual_oh + KH - 1;
            const int input_tile_w = actual_ow + KW - 1;

            ClearI8(input_tile, 128);
            ClearI8(weight_tile, 128);

            ExtractInputTileNHWC(
                input,
                B, H, W, C,
                b, oh_start, ow_start, ic_start,
                1, input_tile_h, input_tile_w, actual_ic,
                input_tile);

            ExtractConvWeightTileOHWI(
                filter,
                OC, KH, KW, IC,
                oc_start, 0, 0, ic_start,
                actual_oc, KH, KW, actual_ic,
                weight_tile);

            for (int toh = 0; toh < actual_oh; toh++) {
              for (int tow = 0; tow < actual_ow; tow++) {
                for (int toc = 0; toc < actual_oc; toc++) {
                  int32_t partial = 0;

                  for (int kh = 0; kh < KH; kh++) {
                    for (int kw = 0; kw < KW; kw++) {
                      for (int tic = 0; tic < actual_ic; tic++) {
                        const int in_tile_idx =
                            ((0 * input_tile_h + (toh + kh)) *
                                 input_tile_w +
                             (tow + kw)) *
                                actual_ic +
                            tic;

                        const int wt_tile_idx =
                            (((toc * KH + kh) * KW + kw) *
                                 actual_ic) +
                            tic;

                        partial +=
                            static_cast<int32_t>(input_tile[in_tile_idx]) *
                            static_cast<int32_t>(weight_tile[wt_tile_idx]);
                      }
                    }
                  }

                  const int full_out_idx =
                      IndexNHWC(b,
                                oh_start + toh,
                                ow_start + tow,
                                oc_start + toc,
                                OH,
                                OW,
                                OC);

                  output[full_out_idx] += partial;
                }
              }
            }
          }
        }
      }
    }
  }
}

// ============================================================
// Compare output buffers
// ============================================================

static int CompareI32Buffers(const char* name,
                             const volatile int32_t* golden,
                             const volatile int32_t* tiled,
                             int n) {
  int pass = 1;

  for (int i = 0; i < n; i++) {
    if (golden[i] != tiled[i]) {
      pass = 0;

      RawPuts("MISMATCH ");
      RawPuts(name);
      RawPuts(" i=");
      RawHex32(static_cast<uint32_t>(i));
      RawPuts(" G=");
      RawHex32(static_cast<uint32_t>(golden[i]));
      RawPuts(" T=");
      RawHex32(static_cast<uint32_t>(tiled[i]));
      RawNewline();
    }
  }

  RawPuts(name);
  RawSpace();
  RawPuts(pass ? "PASS" : "FAIL");
  RawNewline();
  RawNewline();

  return pass;
}

// ============================================================
// Test 1: 1x1 Conv2D
//
// Smallest useful Conv2D.
// Good first test because KH=KW=1.
// ============================================================

static int Test1x1Conv2D() {
  RawPuts("TEST 1: 1x1 CONV2D FULL VS TILED");
  RawNewline();

  const int B = 1;
  const int H = 2;
  const int W = 2;
  const int C = 2;

  const int OC = 2;
  const int KH = 1;
  const int KW = 1;
  const int IC = 2;

  const int OH = 2;
  const int OW = 2;

  static volatile int8_t input[B * H * W * C];
  static volatile int8_t filter[OC * KH * KW * IC];
  static volatile int32_t golden[B * OH * OW * OC];
  static volatile int32_t tiled[B * OH * OW * OC];

  FillInputPattern(input, B * H * W * C);
  FillWeightPattern(filter, OC * KH * KW * IC);
  ClearI32(golden, B * OH * OW * OC);
  ClearI32(tiled, B * OH * OW * OC);

  FullConv2DValidOHWI(
      input,
      B, H, W, C,
      filter,
      OC, KH, KW, IC,
      OH, OW,
      golden);

  TiledConv2DValidOHWI(
      input,
      B, H, W, C,
      filter,
      OC, KH, KW, IC,
      OH, OW,
      1, 1, 1, 1,
      tiled);

  RawPrintI32Flat("GOLDEN 1X1", golden, B * OH * OW * OC);
  RawPrintI32Flat("TILED  1X1", tiled, B * OH * OW * OC);

  return CompareI32Buffers("TEST 1 1X1 CONV2D", golden, tiled,
                           B * OH * OW * OC);
}

// ============================================================
// Test 2: 3x3 Conv2D
//
// This adds real kernel movement.
// Input  [1,5,5,2]
// Filter [2,3,3,2]
// Output [1,3,3,2]
// ============================================================

static int Test3x3Conv2D() {
  RawPuts("TEST 2: 3x3 CONV2D FULL VS TILED");
  RawNewline();

  const int B = 1;
  const int H = 5;
  const int W = 5;
  const int C = 2;

  const int OC = 2;
  const int KH = 3;
  const int KW = 3;
  const int IC = 2;

  const int OH = 3;
  const int OW = 3;

  static volatile int8_t input[B * H * W * C];
  static volatile int8_t filter[OC * KH * KW * IC];
  static volatile int32_t golden[B * OH * OW * OC];
  static volatile int32_t tiled[B * OH * OW * OC];

  FillInputPattern(input, B * H * W * C);
  FillWeightPattern(filter, OC * KH * KW * IC);
  ClearI32(golden, B * OH * OW * OC);
  ClearI32(tiled, B * OH * OW * OC);

  FullConv2DValidOHWI(
      input,
      B, H, W, C,
      filter,
      OC, KH, KW, IC,
      OH, OW,
      golden);

  TiledConv2DValidOHWI(
      input,
      B, H, W, C,
      filter,
      OC, KH, KW, IC,
      OH, OW,
      1, 2, 1, 1,
      tiled);

  RawPrintI32Flat("GOLDEN 3X3", golden, B * OH * OW * OC);
  RawPrintI32Flat("TILED  3X3", tiled, B * OH * OW * OC);

  return CompareI32Buffers("TEST 2 3X3 CONV2D", golden, tiled,
                           B * OH * OW * OC);
}

// ============================================================
// Main
// ============================================================

extern "C" int main() {
  RawNewline();
  RawPuts("=== SOFTWARE TILED CONV2D TEST ===");
  RawNewline();
  RawNewline();

  int overall_pass = 1;

  if (!Test1x1Conv2D()) {
    overall_pass = 0;
  }

  if (!Test3x3Conv2D()) {
    overall_pass = 0;
  }

  RawPuts("OVERALL ");

  if (overall_pass) {
    RawPuts("PASS");
  } else {
    RawPuts("FAIL");
  }

  RawNewline();

  WriteDoneMailbox();

  while (1) {
  }

  return 0;
}