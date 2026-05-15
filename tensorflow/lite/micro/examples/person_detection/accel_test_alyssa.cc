#include <stdint.h>
#include <stddef.h>

// ================================================================
// UART helpers
// ================================================================

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
  while (*s) {
    RawPutc(*s++);
  }
}

static inline void RawNewline() {
  RawPutc('\r');
  RawPutc('\n');
}

static inline void RawHexNibble(uint32_t v) {
  v &= 0xFu;
  RawPutc((v < 10u) ? static_cast<char>('0' + v)
                    : static_cast<char>('A' + (v - 10u)));
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

// ================================================================
// Address map
// ================================================================

#define DMA_BASE                  0x00010000u
#define CSR_BASE                  0x00001000u

#define SPAD_WRITE_ADAPTER_BASE   0xC0000000u
#define OUTPUT_SPAD_READ_BASE     0x00020000u

// ================================================================
// DMA register map
// ================================================================

#define DMA_CH_STRIDE             0x20u

#define DMA_SRC_ADDR              0x00u
#define DMA_DST_ADDR              0x04u
#define DMA_BYTE_LEN              0x08u
#define DMA_CTRL                  0x0Cu
#define DMA_STATUS                0x10u
#define DMA_SPAD_SEL              0x14u

#define DMA_CTRL_START            0x00000001u
#define DMA_STATUS_BUSY           0x00000001u
#define DMA_STATUS_DONE           0x00000002u

#define SPAD_SEL_WEIGHTS          0u
#define SPAD_SEL_INPUTS           1u
#define SPAD_SEL_BIAS             2u
#define SPAD_SEL_SCALE            3u
#define SPAD_SEL_SHIFT            4u

// ================================================================
// CSR register map
//
// systolic_csr.v expected map:
//   0x30 CTRL   [0]=START/route_en, [1]=reg_clear
//   0x34 STATUS [0]=i_done
//   0x38 quant_sh
//   0x40 DDR_STATUS [0]=i_ddr_calib_done
//
// IMPORTANT done path:
//   top.sv/top_accel.o_done
//       -> top_systolic_wrapper.o_done
//       -> systolic_csr.i_done
//       -> CSR_STATUS[0]
// ================================================================

#define CSR_CONV_MODE             0x00u
#define CSR_P_MODE                0x04u
#define CSR_I_SIZE                0x08u
#define CSR_I_C_SIZE              0x0Cu
#define CSR_O_C_SIZE              0x10u
#define CSR_O_SIZE                0x14u
#define CSR_STRIDE                0x18u
#define CSR_DEPTH_MULT            0x1Cu
#define CSR_I_START_ADDR          0x20u
#define CSR_I_ADDR_END            0x24u
#define CSR_W_START_ADDR          0x28u
#define CSR_W_ADDR_END            0x2Cu
#define CSR_CTRL                  0x30u
#define CSR_STATUS                0x34u
#define CSR_QUANT_SH              0x38u
#define CSR_DDR_STATUS            0x40u

#define CSR_CTRL_START            0x00000001u
#define CSR_CTRL_REG_CLEAR        0x00000002u
#define CSR_STATUS_DONE           0x00000001u

#define CONV_MODE_POINTWISE       0u
#define P_MODE_INT8               0u

// Keep route_en high while accelerator runs.
#define CSR_KEEP_START_HIGH       1

// ================================================================
// Input:
//   i_i_size   = 4
//   i_i_c_size = 16
//   total input = 4 * 4 * 16 = 256 int8 bytes = 64 words
//
// Weights:
//   o_c_size * i_c_size = 16 * 16 = 256 int8 bytes = 64 words
//
// Output:
//   o_size * o_size * o_c_size = 4 * 4 * 16 = 256 int8 bytes = 64 words
// ================================================================

#define TEST_I_SIZE               4u
#define TEST_I_C_SIZE             16u
#define TEST_O_C_SIZE             16u
#define TEST_O_SIZE               4u
#define TEST_STRIDE               1u
#define TEST_DEPTH_MULT           1u

#define TEST_INPUT_WORDS          64u
#define TEST_WEIGHT_WORDS         64u
#define TEST_OUTPUT_WORDS         64u
#define TEST_BIAS_WORDS           16u
#define TEST_SCALE_WORDS          16u
#define TEST_SHIFT_WORDS          16u

#define TEST_INPUT_BYTES          (TEST_INPUT_WORDS  * 4u)
#define TEST_WEIGHT_BYTES         (TEST_WEIGHT_WORDS * 4u)
#define TEST_OUTPUT_BYTES         (TEST_OUTPUT_WORDS * 4u)
#define TEST_BIAS_BYTES           (TEST_BIAS_WORDS   * 4u)
#define TEST_SCALE_BYTES          (TEST_SCALE_WORDS  * 4u)
#define TEST_SHIFT_BYTES          (TEST_SHIFT_WORDS  * 4u)


#define TEST_I_START_ADDR         0u
#define TEST_I_ADDR_END           63u
#define TEST_W_START_ADDR         0u
#define TEST_W_ADDR_END           63u

// ================================================================
// MMIO helpers
// ================================================================

static inline void mmio_write32(uint32_t addr, uint32_t value) {
  *(volatile uint32_t*)addr = value;
}

static inline uint32_t mmio_read32(uint32_t addr) {
  return *(volatile uint32_t*)addr;
}

static inline uint32_t bswap32(uint32_t v) {
  return ((v & 0x000000FFu) << 24) |
         ((v & 0x0000FF00u) << 8)  |
         ((v & 0x00FF0000u) >> 8)  |
         ((v & 0xFF000000u) >> 24);
}

static inline uint32_t ToSpadWord(uint32_t logical_word) {
  return bswap32(logical_word);
}

static inline uint32_t dma_reg(uint32_t ch, uint32_t off) {
  return DMA_BASE + ch * DMA_CH_STRIDE + off;
}

static inline void dma_write32(uint32_t ch, uint32_t off, uint32_t value) {
  mmio_write32(dma_reg(ch, off), bswap32(value));
}

static inline uint32_t dma_read32(uint32_t ch, uint32_t off) {
  return bswap32(mmio_read32(dma_reg(ch, off)));
}

static inline void csr_write32(uint32_t off, uint32_t value) {
  mmio_write32(CSR_BASE + off, bswap32(value));
}

static inline uint32_t csr_read32(uint32_t off) {
  return bswap32(mmio_read32(CSR_BASE + off));
}

static inline void SmallDelay() {
  for (volatile int d = 0; d < 2000; d++) {
  }
}

static inline void BigDelay() {
  for (volatile int d = 0; d < 20000; d++) {
  }
}

// ================================================================
// Test buffers
// ================================================================

alignas(4) static volatile uint32_t weights_src[TEST_WEIGHT_WORDS];
alignas(4) static volatile uint32_t inputs_src[TEST_INPUT_WORDS];
alignas(4) static volatile uint32_t bias_src[TEST_BIAS_WORDS];
alignas(4) static volatile uint32_t scale_src[TEST_SCALE_WORDS];
alignas(4) static volatile uint32_t shift_src[TEST_SHIFT_WORDS];

alignas(4) static volatile uint32_t output_dst[TEST_OUTPUT_WORDS];

static void FillBuffers() {

  for (uint32_t i = 0; i < TEST_INPUT_WORDS; i++) {
    uint32_t logical = 0x01010101u + i;
    inputs_src[i] = ToSpadWord(logical);
  }

  for (uint32_t i = 0; i < TEST_WEIGHT_WORDS; i++) {
    uint32_t logical = 0x01010101u;
    weights_src[i] = ToSpadWord(logical);
  }

  for (uint32_t i = 0; i < TEST_BIAS_WORDS; i++) {
    uint32_t logical = 0x00000000u;
    bias_src[i] = ToSpadWord(logical);
  }

  for (uint32_t i = 0; i < TEST_SCALE_WORDS; i++) {
    uint32_t logical = 0x00009C8Cu;
    scale_src[i] = ToSpadWord(logical);
  }

  for (uint32_t i = 0; i < TEST_SHIFT_WORDS; i++) {
    uint32_t logical = 0x00000005u;
    shift_src[i] = ToSpadWord(logical);
  }

  for (uint32_t i = 0; i < TEST_OUTPUT_WORDS; i++) {
    output_dst[i] = 0xDEADBEEFu;
  }

  RawNewline();
  RawPuts("=== BYTE-SWAPPED INPUT-SIDE TEST DATA ===");
  RawNewline();
  RawTagHex("IN0_DDR=", inputs_src[0]);
  RawTagHex("IN63_DDR=", inputs_src[63]);
  RawTagHex("WT0_DDR=", weights_src[0]);
  RawTagHex("SC0_DDR=", scale_src[0]);
  RawTagHex("SH0_DDR=", shift_src[0]);
}

// ================================================================
// DMA helpers
// ================================================================

static bool DmaWaitDone(uint32_t ch, const char* name) {
  uint32_t timeout = 10000000u;
  uint32_t status = 0;

  while (timeout--) {
    status = dma_read32(ch, DMA_STATUS);
    if (status & DMA_STATUS_DONE) {
      RawTagHex("STATUS=", status);
      return true;
    }
  }

  RawPuts("DMA TIMEOUT: ");
  RawPuts(name);
  RawNewline();
  RawTagHex("STATUS=", status);
  return false;
}

static bool DmaStart(uint32_t ch,
                     uint32_t src_addr,
                     uint32_t dst_addr,
                     uint32_t len_bytes,
                     uint32_t spad_sel,
                     const char* name) {
  RawNewline();
  RawPuts("=== DMA ");
  RawPuts(name);
  RawPuts(" ===");
  RawNewline();

  RawTagHex("SRC=", src_addr);
  RawTagHex("DST=", dst_addr);
  RawTagHex("LEN=", len_bytes);
  RawTagHex("SEL=", spad_sel);

  // Clear previous DONE.
  dma_write32(ch, DMA_STATUS, DMA_STATUS_DONE);

  // Program DMA.
  dma_write32(ch, DMA_SRC_ADDR, src_addr);
  dma_write32(ch, DMA_DST_ADDR, dst_addr);
  dma_write32(ch, DMA_BYTE_LEN, len_bytes);
  dma_write32(ch, DMA_SPAD_SEL, spad_sel);
  dma_write32(ch, DMA_CTRL, 0u);

  RawPuts("READBACK");
  RawNewline();
  RawTagHex("R_SRC=", dma_read32(ch, DMA_SRC_ADDR));
  RawTagHex("R_DST=", dma_read32(ch, DMA_DST_ADDR));
  RawTagHex("R_LEN=", dma_read32(ch, DMA_BYTE_LEN));
  RawTagHex("R_SEL=", dma_read32(ch, DMA_SPAD_SEL));
  RawTagHex("R_STA=", dma_read32(ch, DMA_STATUS));

  RawPuts("START");
  RawNewline();

  dma_write32(ch, DMA_CTRL, DMA_CTRL_START);

  bool ok = DmaWaitDone(ch, name);

  dma_write32(ch, DMA_CTRL, 0u);

  if (ok) {
    RawPuts("DMA PASS: ");
    RawPuts(name);
    RawNewline();
  }

  return ok;
}

static bool DmaCopyToInputSideSpad(uint32_t ch,
                                   const volatile uint32_t* src,
                                   uint32_t len_bytes,
                                   uint32_t spad_sel,
                                   const char* name) {
  return DmaStart(ch,
                  static_cast<uint32_t>(reinterpret_cast<uintptr_t>(src)),
                  SPAD_WRITE_ADAPTER_BASE,
                  len_bytes,
                  spad_sel,
                  name);
}

static bool DmaCopyOutputSpadToMemory(uint32_t ch,
                                      volatile uint32_t* dst,
                                      uint32_t len_bytes) {
  // For output copy-back:
  //   DMA reads from output read adapter at 0xC0010000
  //   DMA writes into DDR/global output_dst.
  return DmaStart(ch,
                  OUTPUT_SPAD_READ_BASE,
                  static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dst)),
                  len_bytes,
                  0u,
                  "OUTPUT_SPAD_TO_DDR");
}

// ================================================================
// CSR helpers
// ================================================================

static bool WaitDdrReady() {
  RawNewline();
  RawPuts("=== WAIT DDR READY ===");
  RawNewline();

  uint32_t timeout = 10000000u;
  uint32_t ddr = 0;

  while (timeout--) {
    ddr = csr_read32(CSR_DDR_STATUS);
    if (ddr & 1u) {
      RawTagHex("DDR=", ddr);
      RawPuts("DDR READY");
      RawNewline();
      return true;
    }
  }

  RawTagHex("DDR=", ddr);
  RawPuts("DDR NOT READY");
  RawNewline();
  return false;
}

static bool CsrProgramWorkingPointwise() {
  RawNewline();
  RawPuts("=== CSR PROGRAM ===");
  RawNewline();

  // testbench config:
  //
  //   i_i_size   = 4
  //   i_i_c_size = 16
  //   i_o_c_size = 16
  //   i_o_size   = 4
  //
  //   input words  0..63 = 256 bytes
  //   weight words 0..63 = 256 bytes

  csr_write32(CSR_CONV_MODE,    CONV_MODE_POINTWISE);
  csr_write32(CSR_P_MODE,       P_MODE_INT8);

  csr_write32(CSR_I_SIZE,       TEST_I_SIZE);
  csr_write32(CSR_I_C_SIZE,     TEST_I_C_SIZE);
  csr_write32(CSR_O_C_SIZE,     TEST_O_C_SIZE);
  csr_write32(CSR_O_SIZE,       TEST_O_SIZE);
  csr_write32(CSR_STRIDE,       TEST_STRIDE);
  csr_write32(CSR_DEPTH_MULT,   TEST_DEPTH_MULT);

  csr_write32(CSR_I_START_ADDR, TEST_I_START_ADDR);
  csr_write32(CSR_I_ADDR_END,   TEST_I_ADDR_END);
  csr_write32(CSR_W_START_ADDR, TEST_W_START_ADDR);
  csr_write32(CSR_W_ADDR_END,   TEST_W_ADDR_END);

  csr_write32(CSR_QUANT_SH,     5u);

  RawTagHex("CONV=", csr_read32(CSR_CONV_MODE));
  RawTagHex("PMOD=", csr_read32(CSR_P_MODE));
  RawTagHex("ISIZ=", csr_read32(CSR_I_SIZE));
  RawTagHex("IC  =", csr_read32(CSR_I_C_SIZE));
  RawTagHex("OC  =", csr_read32(CSR_O_C_SIZE));
  RawTagHex("OSIZ=", csr_read32(CSR_O_SIZE));
  RawTagHex("STRD=", csr_read32(CSR_STRIDE));
  RawTagHex("DMUL=", csr_read32(CSR_DEPTH_MULT));
  RawTagHex("ISTA=", csr_read32(CSR_I_START_ADDR));
  RawTagHex("IEND=", csr_read32(CSR_I_ADDR_END));
  RawTagHex("WSTA=", csr_read32(CSR_W_START_ADDR));
  RawTagHex("WEND=", csr_read32(CSR_W_ADDR_END));
  RawTagHex("QSH =", csr_read32(CSR_QUANT_SH));

  bool pass = true;
  pass = (csr_read32(CSR_CONV_MODE)    == CONV_MODE_POINTWISE) && pass;
  pass = (csr_read32(CSR_P_MODE)       == P_MODE_INT8) && pass;
  pass = (csr_read32(CSR_I_SIZE)       == TEST_I_SIZE) && pass;
  pass = (csr_read32(CSR_I_C_SIZE)     == TEST_I_C_SIZE) && pass;
  pass = (csr_read32(CSR_O_C_SIZE)     == TEST_O_C_SIZE) && pass;
  pass = (csr_read32(CSR_O_SIZE)       == TEST_O_SIZE) && pass;
  pass = (csr_read32(CSR_STRIDE)       == TEST_STRIDE) && pass;
  pass = (csr_read32(CSR_DEPTH_MULT)   == TEST_DEPTH_MULT) && pass;
  pass = (csr_read32(CSR_I_START_ADDR) == TEST_I_START_ADDR) && pass;
  pass = (csr_read32(CSR_I_ADDR_END)   == TEST_I_ADDR_END) && pass;
  pass = (csr_read32(CSR_W_START_ADDR) == TEST_W_START_ADDR) && pass;
  pass = (csr_read32(CSR_W_ADDR_END)   == TEST_W_ADDR_END) && pass;
  pass = (csr_read32(CSR_QUANT_SH)     == 5u) && pass;

  RawPuts(pass ? "CSR PROGRAM PASS" : "CSR PROGRAM FAIL");
  RawNewline();

  return pass;
}

static void CsrPulseRegClear() {
  RawNewline();
  RawPuts("=== CSR REG_CLEAR PULSE ===");
  RawNewline();

  RawTagHex("STATUS_BEFORE_CLEAR=", csr_read32(CSR_STATUS));

  // Make sure CTRL starts from 0.
  csr_write32(CSR_CTRL, 0u);
  SmallDelay();

  // Pulse reg_clear.
  csr_write32(CSR_CTRL, CSR_CTRL_REG_CLEAR);
  SmallDelay();

  // Deassert reg_clear.
  csr_write32(CSR_CTRL, 0u);
  BigDelay();

  RawTagHex("STATUS_AFTER_CLEAR =", csr_read32(CSR_STATUS));
  RawTagHex("CTRL_AFTER_CLEAR   =", csr_read32(CSR_CTRL));
}

static void CsrStartAccel() {
  RawNewline();
  RawPuts("=== CSR START ACCEL ===");
  RawNewline();

  RawTagHex("STATUS_BEFORE_START=", csr_read32(CSR_STATUS));
  RawTagHex("CTRL_BEFORE_START  =", csr_read32(CSR_CTRL));

  // Assert START / route_en.
  // ILA should show systolic_csr.o_route_en going high here.
  csr_write32(CSR_CTRL, CSR_CTRL_START);
  SmallDelay();

#if CSR_KEEP_START_HIGH
  // Keep route_en high while waiting.
  RawPuts("START HELD HIGH");
  RawNewline();
#else
  // Deassert start so it is a clean pulse.
  csr_write32(CSR_CTRL, 0u);
  SmallDelay();
  RawPuts("START PULSED");
  RawNewline();
#endif

  RawTagHex("CTRL_AFTER_START   =", csr_read32(CSR_CTRL));
  RawTagHex("STATUS_AFTER_START =", csr_read32(CSR_STATUS));
}

static bool CsrWaitDone() {
  RawNewline();
  RawPuts("=== WAIT ACCEL DONE ===");
  RawNewline();

  uint32_t timeout = 10000000u;
  uint32_t status = 0;
  uint32_t last_status = 0xFFFFFFFFu;

  while (timeout--) {
    status = csr_read32(CSR_STATUS);

    // Print only when status changes, so UART does not spam too much.
    if (status != last_status) {
      RawTagHex("CSR_STATUS=", status);
      last_status = status;
    }

    if (status & CSR_STATUS_DONE) {
      RawPuts("ACCEL DONE SEEN IN CSR_STATUS[0]");
      RawNewline();

#if CSR_KEEP_START_HIGH
      // If START was held high, deassert after done.
      csr_write32(CSR_CTRL, 0u);
      SmallDelay();
      RawTagHex("CTRL_AFTER_DONE=", csr_read32(CSR_CTRL));
#endif

      return true;
    }
  }

  RawTagHex("CSR_STATUS_FINAL=", status);
  RawPuts("ACCEL DONE TIMEOUT");
  RawNewline();
  RawPuts("CHECK THIS CONNECTION:");
  RawNewline();
  RawPuts("top_accel.o_done -> top_systolic_wrapper.o_done -> systolic_csr.i_done");
  RawNewline();

#if CSR_KEEP_START_HIGH
  csr_write32(CSR_CTRL, 0u);
  SmallDelay();
#endif

  return false;
}

// ================================================================
// Output print
// ================================================================

static void PrintOutputBuffer() {
  RawNewline();
  RawPuts("=== OUTPUT BUFFER ===");
  RawNewline();

  for (uint32_t i = 0; i < TEST_OUTPUT_WORDS; i++) {
    RawPuts("OUT[");
    RawHex32(i);
    RawPuts("]=");
    RawHex32(output_dst[i]);
    RawNewline();
  }
}

// ================================================================
// Optional expected-output hint from testbanch
// ================================================================

static void PrintExpectedHint() {
  RawNewline();
  RawPuts("EXPECTED SIM-LIKE FIRST OUTPUTS:");
  RawNewline();
  RawPuts("OUT[0..3]   around 000D0000");
  RawNewline();
  RawPuts("OUT[4..7]   around 00170000");
  RawNewline();
  RawPuts("OUT[8..11]  around 00210001");
  RawNewline();
  RawPuts("OUT[12..15] around 002A0001");
  RawNewline();
}

// ================================================================
// Main
// ================================================================

extern "C" int main() {
  RawNewline();
  RawPuts("==========================================");
  RawNewline();
  RawPuts(" DMA SPADS + CSR + TOP DONE TEST");
  RawNewline();
  RawPuts(" WORKING 4x4x16 POINTWISE CONFIG");
  RawNewline();
  RawPuts("==========================================");
  RawNewline();

  FillBuffers();

  bool pass = true;

  const uint32_t ch = 0;

  pass = WaitDdrReady() && pass;

  // 1. Load all input-side SPADs through write adapter.
  pass = DmaCopyToInputSideSpad(ch, weights_src, TEST_WEIGHT_BYTES,
                                SPAD_SEL_WEIGHTS, "LOAD_WEIGHTS_64W") && pass;

  pass = DmaCopyToInputSideSpad(ch, inputs_src, TEST_INPUT_BYTES,
                                SPAD_SEL_INPUTS, "LOAD_INPUTS_64W") && pass;

  pass = DmaCopyToInputSideSpad(ch, bias_src, TEST_BIAS_BYTES,
                                SPAD_SEL_BIAS, "LOAD_BIAS_16W") && pass;

  pass = DmaCopyToInputSideSpad(ch, scale_src, TEST_SCALE_BYTES,
                                SPAD_SEL_SCALE, "LOAD_SCALE_16W") && pass;

  pass = DmaCopyToInputSideSpad(ch, shift_src, TEST_SHIFT_BYTES,
                                SPAD_SEL_SHIFT, "LOAD_SHIFT_16W") && pass;

  // 2. Program CSR config registers.
  pass = CsrProgramWorkingPointwise() && pass;

  // 3. Clear accelerator / CSR done state.
  CsrPulseRegClear();

  // 4. Start accelerator.
  CsrStartAccel();

  // 5. Wait for accelerator done.
  // This proves:
  //   top_accel.o_done -> systolic_csr.i_done -> CSR_STATUS[0]
  bool done = CsrWaitDone();

  // 6. Copy output SPAD back through output read adapter.
  if (done) {
    pass = DmaCopyOutputSpadToMemory(ch, output_dst, TEST_OUTPUT_BYTES) && pass;
    PrintOutputBuffer();
    PrintExpectedHint();
  } else {
    pass = false;
    RawPuts("SKIP OUTPUT DMA READ because accelerator did not finish");
    RawNewline();
  }

  RawNewline();
  RawPuts("==========================================");
  RawNewline();

  if (pass) {
    RawPuts("FULL TEST PASS");
    RawNewline();
  } else {
    RawPuts("FULL TEST FAIL");
    RawNewline();
  }

  RawNewline();
  RawPuts("ILA CHECKS:");
  RawNewline();
  RawPuts("1. DMA LOAD_WEIGHTS_64W len should be 00000100");
  RawNewline();
  RawPuts("2. DMA LOAD_INPUTS_64W len should be 00000100");
  RawNewline();
  RawPuts("3. CSR: ISIZ=4 IC=16 OC=16 OSIZ=4 IEND=63 WEND=63");
  RawNewline();
  RawPuts("4. top: state should go ACT_ROUTING -> FIFO_POP -> COMPUTE -> OUTPUT_ROUTING -> DONE");
  RawNewline();
  RawPuts("5. output_router/top should produce o_word_valid for addr 0..63");
  RawNewline();
  RawPuts("6. Critical done path: top.o_done -> CSR.i_done -> CSR_STATUS[0]");
  RawNewline();

  while (1) {
  }

  return 0;
}