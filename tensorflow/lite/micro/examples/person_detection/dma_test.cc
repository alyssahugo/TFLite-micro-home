#include <stdint.h>
#include <stddef.h>


static inline void RawPutc(char c) {
  volatile uint32_t* const uart_tx =
      reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
  volatile uint32_t* const uart_status =
      reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x08u);

  // Wait while the UART TX FIFO is full.
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

static inline void RawPrintFlat8(const char* name,
                                 const volatile uint8_t* data,
                                 int n) {
  RawPuts(name);
  RawNewline();

  for (int i = 0; i < n; i++) {
    RawHex8(static_cast<uint32_t>(data[i]));
    RawSpace();
  }

  RawNewline();
  RawNewline();
}


// DMA register map
// These offsets must match the DMA_Controller Verilog file

#define DMA_BASE 0x00010000u

#define DMA_CH_STRIDE 0x20u
#define DMA_SRC_ADDR  0x00u
#define DMA_DST_ADDR  0x04u
#define DMA_BYTE_LEN  0x08u
#define DMA_CTRL      0x0Cu
#define DMA_STATUS    0x10u
#define DMA_SPAD_SEL  0x14u

#define DMA_CTRL_START  0x00000001u
#define DMA_STATUS_BUSY 0x00000001u
#define DMA_STATUS_DONE 0x00000002u

// These match the SPAD_SEL encoding in the DMA Verilog.
#define SPAD_WEIGHTS 0x00000000u
#define SPAD_IFMAPS  0x00000001u
#define SPAD_BIAS    0x00000002u
#define SPAD_SCALE   0x00000003u
#define SPAD_SHIFT   0x00000004u

static inline void mmio_write32(uint32_t addr, uint32_t value) {
  *(volatile uint32_t*)addr = value;
}

static inline uint32_t mmio_read32(uint32_t addr) {
  return *(volatile uint32_t*)addr;
}

static inline uint32_t dma_reg(uint32_t ch, uint32_t off) {
  return DMA_BASE + ch * DMA_CH_STRIDE + off;
}

// The current AXI/MMIO path is byte-swapped, so register writes and reads
// go through this helper to make the values look correct in C++.
static inline uint32_t bswap32(uint32_t v) {
  return ((v & 0x000000FFu) << 24) |
         ((v & 0x0000FF00u) << 8)  |
         ((v & 0x00FF0000u) >> 8)  |
         ((v & 0xFF000000u) >> 24);
}

static inline void dma_write32(uint32_t ch, uint32_t off, uint32_t value) {
  mmio_write32(dma_reg(ch, off), bswap32(value));
}

static inline uint32_t dma_read32(uint32_t ch, uint32_t off) {
  return bswap32(mmio_read32(dma_reg(ch, off)));
}


// DRAM Addresses
// These are the scratch addresses we use for the test.
// For now, both source and destination are still DRAM.
// Later, the destination can become the real SPAD/accelerator path.


#define INPUT_TILE_SRC_ADDR   0x80001000u
#define INPUT_TILE_DST_ADDR   0x80002000u
#define WEIGHT_TILE_SRC_ADDR  0x80003000u
#define WEIGHT_TILE_DST_ADDR  0x80004000u

static volatile int8_t* const input_tile_src =
    reinterpret_cast<volatile int8_t*>(INPUT_TILE_SRC_ADDR);

static volatile int8_t* const input_tile_dst =
    reinterpret_cast<volatile int8_t*>(INPUT_TILE_DST_ADDR);

static volatile int8_t* const weight_tile_src =
    reinterpret_cast<volatile int8_t*>(WEIGHT_TILE_SRC_ADDR);

static volatile int8_t* const weight_tile_dst =
    reinterpret_cast<volatile int8_t*>(WEIGHT_TILE_DST_ADDR);

// These are the fake "full tensors" used for testing.
alignas(8) static volatile int8_t full_input_5x5x4[1 * 5 * 5 * 4];
alignas(8) static volatile int8_t full_weight_pw[4 * 1 * 1 * 4];
alignas(8) static volatile int8_t full_weight_3x3[2 * 3 * 3 * 2];

// NHWC means:
//   batch, height, width, channel
//
// OHWI means:
//   output channel, kernel height, kernel width, input channel

static inline int IndexNHWC(int b, int h, int w, int c,
                            int H, int W, int C) {
  return ((b * H + h) * W + w) * C + c;
}

static inline int IndexOHWI(int oc, int kh, int kw, int ic,
                            int KH, int KW, int IC) {
  return (((oc * KH + kh) * KW + kw) * IC) + ic;
}



static void ClearVolatileI8(volatile int8_t* data, int n, int8_t value) {
  for (int i = 0; i < n; i++) {
    data[i] = value;
  }
}

static void FillSequentialVolatileI8(volatile int8_t* data, int n) {
  for (int i = 0; i < n; i++) {
    data[i] = static_cast<int8_t>(i);
  }
}

// Compare two memory regions byte by byte.
// This checks whether the DMA destination matches the source.
static int CompareBytesAtAddr(const char* name,
                              uint32_t a_addr,
                              uint32_t b_addr,
                              int n,
                              int verbose) {
  volatile uint8_t* a = reinterpret_cast<volatile uint8_t*>(a_addr);
  volatile uint8_t* b = reinterpret_cast<volatile uint8_t*>(b_addr);

  int pass = 1;

  for (int i = 0; i < n; i++) {
    const uint8_t av = a[i];
    const uint8_t bv = b[i];

    if (av != bv) {
      pass = 0;

      RawPuts("MISMATCH ");
      RawPuts(name);
      RawPuts(" i=");
      RawHex32(static_cast<uint32_t>(i));
      RawPuts(" A=");
      RawHex32(static_cast<uint32_t>(av));
      RawPuts(" B=");
      RawHex32(static_cast<uint32_t>(bv));
      RawNewline();
    }
  }

  if (verbose) {
    RawPuts(name);
    RawSpace();
    RawPuts(pass ? "PASS" : "FAIL");
    RawNewline();
    RawNewline();
  }

  return pass;
}


//Notice: Had some help from copilot.
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


// This programs the DMA, starts it, waits for DONE, and reports pass/fail.
// The SPAD select is written too, even though this test still writes to DRAM.


static int DmaCopyBlocking(uint32_t ch,
                           uint32_t src_addr,
                           uint32_t dst_addr,
                           uint32_t len_bytes,
                           uint32_t spad_sel,
                           const char* name,
                           int verbose) {
  if (verbose) {
    RawPuts("DMA COPY ");
    RawPuts(name);
    RawNewline();

    RawTagHex("SRC=", src_addr);
    RawTagHex("DST=", dst_addr);
    RawTagHex("LEN=", len_bytes);
    RawTagHex("SPAD=", spad_sel);
  }

  // Clear old DONE before starting a new transfer.
  dma_write32(ch, DMA_STATUS, DMA_STATUS_DONE);

  for (volatile int d = 0; d < 1000; d++) {
  }

  // Program the DMA descriptor.
  dma_write32(ch, DMA_SRC_ADDR, src_addr);
  dma_write32(ch, DMA_DST_ADDR, dst_addr);
  dma_write32(ch, DMA_BYTE_LEN, len_bytes);
  dma_write32(ch, DMA_SPAD_SEL, spad_sel);
  dma_write32(ch, DMA_CTRL, 0);

  if (verbose) {
    RawTagHex("R_SRC=", dma_read32(ch, DMA_SRC_ADDR));
    RawTagHex("R_DST=", dma_read32(ch, DMA_DST_ADDR));
    RawTagHex("R_LEN=", dma_read32(ch, DMA_BYTE_LEN));
    RawTagHex("R_CTL=", dma_read32(ch, DMA_CTRL));
    RawTagHex("R_STA=", dma_read32(ch, DMA_STATUS));
    RawTagHex("R_SPAD=", dma_read32(ch, DMA_SPAD_SEL));
  }

  // Start the transfer.
  dma_write32(ch, DMA_CTRL, DMA_CTRL_START);

  uint32_t timeout = 10000000u;
  uint32_t status = 0;

  while (timeout--) {
    status = dma_read32(ch, DMA_STATUS);

    if (status & DMA_STATUS_DONE) {
      break;
    }
  }

  if (verbose) {
    RawTagHex("STATUS=", status);
  }

  dma_write32(ch, DMA_CTRL, 0);

  if (!(status & DMA_STATUS_DONE)) {
    RawPuts("DMA TIMEOUT ");
    RawPuts(name);
    RawNewline();
    return 0;
  }

  if (verbose) {
    RawPuts("DMA DONE ");
    RawPuts(name);
    RawNewline();
    RawNewline();
  }

  return 1;
}

static int RunOneInputTile(uint32_t iter, int verbose) {
  // Pick different tile starts depending on iter.
  // The -1 cases intentionally test padding behavior.
  const int h_start = static_cast<int>(iter % 4u) - 1;
  const int w_start = static_cast<int>((iter / 2u) % 4u) - 1;

  // Alternate between channel group 0-1 and channel group 2-3.
  const int c_start = (iter & 1u) ? 2 : 0;

  ClearVolatileI8(input_tile_src, 16, 0);
  ClearVolatileI8(input_tile_dst, 16, static_cast<int8_t>(0xEE));

  // Extract one [1, 2, 2, 2] input tile.
  ExtractInputTileNHWC(
      full_input_5x5x4,
      1, 5, 5, 4,
      0, h_start, w_start, c_start,
      1, 2, 2, 2,
      input_tile_src);

  if (verbose) {
    RawPuts("INPUT TILE ITER");
    RawNewline();
    RawTagHex("iter=", iter);
    RawTagHex("h=", static_cast<uint32_t>(h_start));
    RawTagHex("w=", static_cast<uint32_t>(w_start));
    RawTagHex("c=", static_cast<uint32_t>(c_start));

    RawPrintFlat8("INPUT TILE SRC",
                  reinterpret_cast<volatile uint8_t*>(INPUT_TILE_SRC_ADDR),
                  8);
    RawPrintFlat8("INPUT TILE DST BEFORE",
                  reinterpret_cast<volatile uint8_t*>(INPUT_TILE_DST_ADDR),
                  8);
  }

  // DMA-copy the extracted tile.
  int dma_ok = DmaCopyBlocking(
      0,
      INPUT_TILE_SRC_ADDR,
      INPUT_TILE_DST_ADDR,
      8,
      SPAD_IFMAPS,
      "INPUT_TILE",
      verbose);

  if (verbose) {
    RawPrintFlat8("INPUT TILE DST AFTER",
                  reinterpret_cast<volatile uint8_t*>(INPUT_TILE_DST_ADDR),
                  8);
  }

  // Check that DMA copied the tile correctly.
  int cmp_ok = CompareBytesAtAddr(
      "INPUT TILE DMA COPY",
      INPUT_TILE_SRC_ADDR,
      INPUT_TILE_DST_ADDR,
      8,
      verbose);

  if (!dma_ok || !cmp_ok) {
    RawPuts("INPUT TILE ITER FAIL ");
    RawTagHex("iter=", iter);
    RawTagHex("dma_ok=", static_cast<uint32_t>(dma_ok));
    RawTagHex("cmp_ok=", static_cast<uint32_t>(cmp_ok));
    RawNewline();

    RawPrintFlat8("INPUT SRC FAIL",
                  reinterpret_cast<volatile uint8_t*>(INPUT_TILE_SRC_ADDR),
                  8);
    RawPrintFlat8("INPUT DST FAIL",
                  reinterpret_cast<volatile uint8_t*>(INPUT_TILE_DST_ADDR),
                  8);

    return 0;
  }

  return 1;
}

// One pointwise weight-tile DMA test
//
// This tests 1x1 / pointwise Conv2D weight tiles.
// Full shape is [4, 1, 1, 4].
// Tile shape is [2, 1, 1, 2].

static int RunOnePointwiseWeightTile(uint32_t iter, int verbose) {
  // Alternate which output-channel group and input-channel group we select.
  const int oc_start = (iter & 1u) ? 2 : 0;
  const int ic_start = (iter & 2u) ? 2 : 0;

  ClearVolatileI8(weight_tile_src, 16, 0);
  ClearVolatileI8(weight_tile_dst, 16, static_cast<int8_t>(0xEE));

  // Extract one small pointwise weight tile.
  ExtractConvWeightTileOHWI(
      full_weight_pw,
      4, 1, 1, 4,
      oc_start, 0, 0, ic_start,
      2, 1, 1, 2,
      weight_tile_src);

  if (verbose) {
    RawPuts("PW WEIGHT TILE ITER");
    RawNewline();
    RawTagHex("iter=", iter);
    RawTagHex("oc=", static_cast<uint32_t>(oc_start));
    RawTagHex("ic=", static_cast<uint32_t>(ic_start));

    RawPrintFlat8("PW WEIGHT TILE SRC",
                  reinterpret_cast<volatile uint8_t*>(WEIGHT_TILE_SRC_ADDR),
                  4);
    RawPrintFlat8("PW WEIGHT TILE DST BEFORE",
                  reinterpret_cast<volatile uint8_t*>(WEIGHT_TILE_DST_ADDR),
                  4);
  }

  // DMA-copy the pointwise weight tile.
  int dma_ok = DmaCopyBlocking(
      0,
      WEIGHT_TILE_SRC_ADDR,
      WEIGHT_TILE_DST_ADDR,
      4,
      SPAD_WEIGHTS,
      "PW_WEIGHT_TILE",
      verbose);

  if (verbose) {
    RawPrintFlat8("PW WEIGHT TILE DST AFTER",
                  reinterpret_cast<volatile uint8_t*>(WEIGHT_TILE_DST_ADDR),
                  4);
  }

  // Check that DMA copied the weight tile correctly.
  int cmp_ok = CompareBytesAtAddr(
      "PW WEIGHT TILE DMA COPY",
      WEIGHT_TILE_SRC_ADDR,
      WEIGHT_TILE_DST_ADDR,
      4,
      verbose);

  if (!dma_ok || !cmp_ok) {
    RawPuts("PW WEIGHT TILE ITER FAIL ");
    RawTagHex("iter=", iter);
    RawTagHex("dma_ok=", static_cast<uint32_t>(dma_ok));
    RawTagHex("cmp_ok=", static_cast<uint32_t>(cmp_ok));
    RawNewline();

    RawPrintFlat8("PW WEIGHT SRC FAIL",
                  reinterpret_cast<volatile uint8_t*>(WEIGHT_TILE_SRC_ADDR),
                  4);
    RawPrintFlat8("PW WEIGHT DST FAIL",
                  reinterpret_cast<volatile uint8_t*>(WEIGHT_TILE_DST_ADDR),
                  4);

    return 0;
  }

  return 1;
}



static int RunOne3x3WeightTile(uint32_t iter, int verbose) {
  // Alternate between output channel 0 and output channel 1.
  const int oc_start = (iter & 1u) ? 1 : 0;

  ClearVolatileI8(weight_tile_src, 32, 0);
  ClearVolatileI8(weight_tile_dst, 32, static_cast<int8_t>(0xEE));

  // Extract one 3x3 Conv2D weight tile.
  ExtractConvWeightTileOHWI(
      full_weight_3x3,
      2, 3, 3, 2,
      oc_start, 0, 0, 0,
      1, 3, 3, 2,
      weight_tile_src);

  if (verbose) {
    RawPuts("3X3 WEIGHT TILE ITER");
    RawNewline();
    RawTagHex("iter=", iter);
    RawTagHex("oc=", static_cast<uint32_t>(oc_start));

    RawPrintFlat8("3X3 WEIGHT TILE SRC",
                  reinterpret_cast<volatile uint8_t*>(WEIGHT_TILE_SRC_ADDR),
                  18);
    RawPrintFlat8("3X3 WEIGHT TILE DST BEFORE",
                  reinterpret_cast<volatile uint8_t*>(WEIGHT_TILE_DST_ADDR),
                  18);
  }

  // The useful tile is 18 bytes, but the DMA length is 20 because
  // the 32-bit DMA should use lengths that are multiples of 4.
  int dma_ok = DmaCopyBlocking(
      0,
      WEIGHT_TILE_SRC_ADDR,
      WEIGHT_TILE_DST_ADDR,
      20,
      SPAD_WEIGHTS,
      "3X3_WEIGHT_TILE",
      verbose);

  if (verbose) {
    RawPrintFlat8("3X3 WEIGHT TILE DST AFTER",
                  reinterpret_cast<volatile uint8_t*>(WEIGHT_TILE_DST_ADDR),
                  18);
  }

  // Compare only the 18 useful bytes.
  int cmp_ok = CompareBytesAtAddr(
      "3X3 WEIGHT TILE DMA COPY",
      WEIGHT_TILE_SRC_ADDR,
      WEIGHT_TILE_DST_ADDR,
      18,
      verbose);

  if (!dma_ok || !cmp_ok) {
    RawPuts("3X3 WEIGHT TILE ITER FAIL ");
    RawTagHex("iter=", iter);
    RawTagHex("dma_ok=", static_cast<uint32_t>(dma_ok));
    RawTagHex("cmp_ok=", static_cast<uint32_t>(cmp_ok));
    RawNewline();

    RawPrintFlat8("3X3 WEIGHT SRC FAIL",
                  reinterpret_cast<volatile uint8_t*>(WEIGHT_TILE_SRC_ADDR),
                  18);
    RawPrintFlat8("3X3 WEIGHT DST FAIL",
                  reinterpret_cast<volatile uint8_t*>(WEIGHT_TILE_DST_ADDR),
                  18);

    return 0;
  }

  return 1;
}


extern "C" int main() {
  RawNewline();
  RawPuts("=== DMA TILING STRESS TEST 32B ===");
  RawNewline();
  RawNewline();

  RawTagHex("IN_SRC_ADDR=", INPUT_TILE_SRC_ADDR);
  RawTagHex("IN_DST_ADDR=", INPUT_TILE_DST_ADDR);
  RawTagHex("WT_SRC_ADDR=", WEIGHT_TILE_SRC_ADDR);
  RawTagHex("WT_DST_ADDR=", WEIGHT_TILE_DST_ADDR);
  RawNewline();

  // Fill fake full tensors with simple predictable values.
  FillSequentialVolatileI8(full_input_5x5x4, 1 * 5 * 5 * 4);
  FillSequentialVolatileI8(full_weight_pw, 4 * 1 * 1 * 4);
  FillSequentialVolatileI8(full_weight_3x3, 2 * 3 * 3 * 2);

  int overall_pass = 1;
  uint32_t pass_count = 0;
  uint32_t fail_count = 0;

  if (RunOneInputTile(0, 1)) {
    pass_count++;
  } else {
    fail_count++;
    overall_pass = 0;
  }

  if (RunOnePointwiseWeightTile(0, 1)) {
    pass_count++;
  } else {
    fail_count++;
    overall_pass = 0;
  }

  if (RunOne3x3WeightTile(0, 1)) {
    pass_count++;
  } else {
    fail_count++;
    overall_pass = 0;
  }

  // Stress run: many repeated tile extractions and DMA transfers.
  for (uint32_t i = 1; i < 25; i++) {
    if (RunOneInputTile(i, 0)) {
      pass_count++;
    } else {
      fail_count++;
      overall_pass = 0;
      break;
    }

    RawPuts(".");

    if (RunOnePointwiseWeightTile(i, 0)) {
      pass_count++;
    } else {
      fail_count++;
      overall_pass = 0;
      break;
    }

    RawPuts(".");

    if (RunOne3x3WeightTile(i, 0)) {
      pass_count++;
    } else {
      fail_count++;
      overall_pass = 0;
      break;
    }

    RawPuts(".");
  }

  RawNewline();
  RawNewline();

  RawPuts("TILING STRESS SUMMARY");
  RawNewline();
  RawTagHex("pass_count=", pass_count);
  RawTagHex("fail_count=", fail_count);
  RawTagHex("overall_pass=", static_cast<uint32_t>(overall_pass));

  RawPuts("OVERALL ");

  if (overall_pass) {
    RawPuts("PASS");
  } else {
    RawPuts("FAIL");
  }

  RawNewline();

  while (1) {
  }

  return 0;
}