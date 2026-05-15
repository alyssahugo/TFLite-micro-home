// ================================================================
//
// Spatial-tiled pointwise Conv2D version.
//
// Configuration:
//   - input / weight / bias SPADs are NOT AXI-mapped.
//   - DMA writes into those SPADs through custom top.sv routing using M_SPAD_SEL.
//   - DMA DST is treated as a LOCAL SPAD offset/address.
//   - output SPAD IS AXI/CPU-readable.
//   - CPU copies output SPAD values into TFLM output_data.
//   - Accelerator already applies quantization.
//   - CSR has one quant_sh and one quant_mult.
//   - Software runs one output channel at a time.
//   - Software tiles spatially, so full input feature map does NOT need to fit
// ================================================================

#ifndef TFLM_USE_CONV_ACCEL
#define TFLM_USE_CONV_ACCEL 1
#endif

#if TFLM_USE_CONV_ACCEL


#define CONV_DMA_BASE             0x40000000u

#define CONV_DMA_CH_OFFSET(ch)    ((ch) * 0x20u)

#define CONV_DMA_SRC              0x00u
#define CONV_DMA_DST              0x04u
#define CONV_DMA_LEN              0x08u
#define CONV_DMA_CTRL             0x0Cu
#define CONV_DMA_STATUS           0x10u
#define CONV_DMA_SPAD_SEL         0x14u

#define CONV_DMA_CTRL_START       0x00000001u

// STATUS bit0 = BUSY, bit1 = DONE.
// DONE is W1C.
#define CONV_DMA_STATUS_BUSY      0x00000001u
#define CONV_DMA_STATUS_DONE      0x00000002u

// ================================================================
// DMA SPAD select values
// ================================================================

#define CONV_SPAD_SEL_WEIGHTS     0u  // 000
#define CONV_SPAD_SEL_INPUTS      1u  // 001
#define CONV_SPAD_SEL_BIAS        2u  // 010
#define CONV_SPAD_SEL_SCALE       3u  // 011
#define CONV_SPAD_SEL_SHIFT       4u  // 100

// ================================================================
// CSR base address
// ================================================================

#define CONV_CSR_BASE             0x40001000u

// ================================================================
// SPAD ADDRESSING MODEL
//
// Input / weight / bias SPADs are NOT AXI-mapped.
// Therefore these are LOCAL SPAD offsets, not CPU/AXI addresses.
// top.sv should route DMA WDATA into the selected SPAD using M_SPAD_SEL.
//
// Output SPAD IS AXI/CPU-readable, so CPU can read CONV_SPAD_O_AXI_BASE.
// ================================================================

#define CONV_SPAD_W_LOCAL_BASE    0x00000000u
#define CONV_SPAD_I_LOCAL_BASE    0x00000000u
#define CONV_SPAD_B_LOCAL_BASE    0x00000000u

//////////// THIS ONE MUST BE AXI
#define CONV_SPAD_O_AXI_BASE      0x40060000u

// ================================================================
// CSR offsets
// ================================================================

#define CONV_CSR_CONV_MODE        0x00u  // [0] PW=0, DW=1
#define CONV_CSR_P_MODE           0x04u  // [1:0]
#define CONV_CSR_I_SIZE           0x08u
#define CONV_CSR_I_C_SIZE         0x0Cu
#define CONV_CSR_O_C_SIZE         0x10u
#define CONV_CSR_O_SIZE           0x14u
#define CONV_CSR_STRIDE           0x18u
#define CONV_CSR_DEPTH_MULT       0x1Cu
#define CONV_CSR_I_START_ADDR     0x20u
#define CONV_CSR_I_ADDR_END       0x24u
#define CONV_CSR_W_START_ADDR     0x28u
#define CONV_CSR_W_ADDR_END       0x2Cu
#define CONV_CSR_CTRL             0x30u  // [0] route_en/START, [1] reg_clear
#define CONV_CSR_STATUS           0x34u  // [0] i_done/o_done
#define CONV_CSR_QUANT_SH         0x38u  // DATA_WIDTH bits, default 8 bits
#define CONV_CSR_QUANT_MULT       0x3Cu  // 2*DATA_WIDTH bits, default 16 bits

#define CONV_MODE_POINTWISE       0u
#define CONV_MODE_DEPTHWISE       1u

// p_mode encoding is not fully described in the docs.
// Use the value your pointwise systolic RTL expects.
#define CONV_P_MODE_POINTWISE     0u

#define CONV_CTRL_START           0x00000001u
#define CONV_CTRL_REG_CLEAR       0x00000002u
#define CONV_STATUS_DONE          0x00000001u

#define CONV_DMA_BEAT_BYTES       4u

// ================================================================
// SPAD capacity limits
// ================================================================

#define CONV_MAX_INPUT_BYTES      (64u * 1024u)
#define CONV_MAX_WEIGHT_BYTES     (4u  * 1024u)
#define CONV_MAX_BIAS_BYTES       (4u  * 1024u)
#define CONV_MAX_OUTPUT_BYTES     (4u  * 1024u)

#ifndef CONV_ACCEL_LOAD_BIAS_SPAD
#define CONV_ACCEL_LOAD_BIAS_SPAD 1
#endif


alignas(16) static int8_t g_conv_accel_input_tile[
    CONV_MAX_INPUT_BYTES + CONV_DMA_BEAT_BYTES];

alignas(16) static int8_t g_conv_accel_weight_tile[
    CONV_MAX_WEIGHT_BYTES + CONV_DMA_BEAT_BYTES];

alignas(16) static int32_t g_conv_accel_bias_tile;

// ================================================================
// UART helpers
// ================================================================

static inline void ConvAccelPrintTag4(char a, char b, char c, char d) {
  RawPutcD('[');
  RawPutcD(a);
  RawPutcD(b);
  RawPutcD(c);
  RawPutcD(d);
  RawPutcD(']');
  RawNewlineD();
}

static inline void ConvAccelPrintSummary(int input_h,
                                         int input_w,
                                         int input_c,
                                         int output_h,
                                         int output_w,
                                         int output_c,
                                         int tile_h,
                                         int tile_w) {
  RawPutcD('['); RawPutcD('A'); RawPutcD('C'); RawPutcD('S'); RawPutcD(']');
  RawPutcD(' ');

  RawPutcD('i'); RawPutcD('h');
  RawPutHex32D(static_cast<uint32_t>(input_h));
  RawPutcD(' ');

  RawPutcD('i'); RawPutcD('w');
  RawPutHex32D(static_cast<uint32_t>(input_w));
  RawPutcD(' ');

  RawPutcD('i'); RawPutcD('c');
  RawPutHex32D(static_cast<uint32_t>(input_c));
  RawPutcD(' ');

  RawPutcD('o'); RawPutcD('h');
  RawPutHex32D(static_cast<uint32_t>(output_h));
  RawPutcD(' ');

  RawPutcD('o'); RawPutcD('w');
  RawPutHex32D(static_cast<uint32_t>(output_w));
  RawPutcD(' ');

  RawPutcD('o'); RawPutcD('c');
  RawPutHex32D(static_cast<uint32_t>(output_c));
  RawPutcD(' ');

  RawPutcD('t'); RawPutcD('h');
  RawPutHex32D(static_cast<uint32_t>(tile_h));
  RawPutcD(' ');

  RawPutcD('t'); RawPutcD('w');
  RawPutHex32D(static_cast<uint32_t>(tile_w));

  RawNewlineD();
}

// ================================================================
// Basic MMIO helpers
// ================================================================

static inline uint32_t ConvPtrToU32(const void* p) {
  return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p));
}

static inline volatile uint32_t* ConvReg32(uint32_t addr) {
  return reinterpret_cast<volatile uint32_t*>(addr);
}

static inline void ConvWrite32(uint32_t addr, uint32_t value) {
  *ConvReg32(addr) = value;
}

static inline uint32_t ConvRead32(uint32_t addr) {
  return *ConvReg32(addr);
}

static inline volatile uint32_t* ConvDmaReg(int ch, uint32_t reg) {
  return reinterpret_cast<volatile uint32_t*>(
      CONV_DMA_BASE + CONV_DMA_CH_OFFSET(ch) + reg);
}

static inline uint32_t ConvRoundUpToDmaBeat(uint32_t len) {
  const uint32_t mask = CONV_DMA_BEAT_BYTES - 1u;
  return (len + mask) & ~mask;
}

static inline int ConvMinInt(int a, int b) {
  return (a < b) ? a : b;
}

// ================================================================
// DMA helpers
// ================================================================

static inline bool ConvDmaWaitDone(int ch) {
  volatile uint32_t* status = ConvDmaReg(ch, CONV_DMA_STATUS);

  for (uint32_t timeout = 0; timeout < 0x00FFFFFFu; ++timeout) {
    const uint32_t s = *status;

    if (s & CONV_DMA_STATUS_DONE) {
      // W1C clear DONE.
      *status = CONV_DMA_STATUS_DONE;
      return true;
    }
  }

  return false;
}

// DMA copy into a custom-routed SPAD.
//
// src      = real DDR / TFLM memory address.
// dst      = local SPAD offset/address, not necessarily an AXI address.
// len      = useful byte count.
// spad_sel = which SPAD top.sv should route WDATA into.
static inline bool ConvDmaCopyToLocalSpad(int ch,
                                          uint32_t src,
                                          uint32_t local_spad_dst,
                                          uint32_t len_bytes,
                                          uint32_t spad_sel) {
  if (len_bytes == 0u) {
    return true;
  }


  const uint32_t dma_len = ConvRoundUpToDmaBeat(len_bytes);

  *ConvDmaReg(ch, CONV_DMA_SRC)      = src;
  *ConvDmaReg(ch, CONV_DMA_DST)      = local_spad_dst;
  *ConvDmaReg(ch, CONV_DMA_LEN)      = dma_len;
  *ConvDmaReg(ch, CONV_DMA_SPAD_SEL) = spad_sel;

  *ConvDmaReg(ch, CONV_DMA_CTRL) = CONV_DMA_CTRL_START;

  return ConvDmaWaitDone(ch);
}

// ================================================================
// Accelerator wait/start helpers
// ================================================================

static inline bool ConvAccelWaitDone() {
  for (uint32_t timeout = 0; timeout < 0x00FFFFFFu; ++timeout) {
    if (ConvRead32(CONV_CSR_BASE + CONV_CSR_STATUS) & CONV_STATUS_DONE) {
      return true;
    }
  }

  return false;
}

static inline void ConvAccelClearRouters() {
  // In systolic_csr.v, o_reg_clear is a one-cycle pulse and auto-clears.
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_CTRL, CONV_CTRL_REG_CLEAR);
}

static inline void ConvAccelStart() {
  // In systolic_csr.v, o_route_en is a one-cycle pulse and auto-clears.
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_CTRL, CONV_CTRL_START);
}

// ================================================================
// Quant helper
//
// CSR width:
//   quant_sh   = DATA_WIDTH bits, default 8 bits
//   quant_mult = 2*DATA_WIDTH bits, default 16 bits
// ================================================================

static inline uint32_t ConvAccelQuantShiftForCsr(int32_t output_shift) {
  if (output_shift < 0) {
    output_shift = -output_shift;
  }
  return static_cast<uint32_t>(output_shift) & 0xFFu;
}

static inline uint32_t ConvAccelQuantMultForCsr(int32_t output_multiplier) {
  return static_cast<uint32_t>(output_multiplier) & 0xFFFFu;
}

// ================================================================
// Support check
// ================================================================

static inline bool ConvAccelCanRunPointwise(
    const ConvParams& params,
    const RuntimeShape& input_shape,
    const RuntimeShape& filter_shape,
    const RuntimeShape& output_shape,
    const int32_t* output_multiplier,
    const int32_t* output_shift) {
  if (input_shape.DimensionsCount() != 4 ||
      filter_shape.DimensionsCount() != 4 ||
      output_shape.DimensionsCount() != 4) {
    return false;
  }

  if (output_multiplier == nullptr || output_shift == nullptr) {
    return false;
  }

  const int batches = input_shape.Dims(0);
  const int input_h = input_shape.Dims(1);
  const int input_w = input_shape.Dims(2);
  const int input_c = input_shape.Dims(3);

  const int filter_out_c = filter_shape.Dims(0);
  const int filter_h = filter_shape.Dims(1);
  const int filter_w = filter_shape.Dims(2);
  const int filter_in_c = filter_shape.Dims(3);

  const int output_h = output_shape.Dims(1);
  const int output_w = output_shape.Dims(2);
  const int output_c = output_shape.Dims(3);

  // This accelerator path is pointwise Conv2D only.
  if (filter_h != 1 || filter_w != 1) {
    return false;
  }

  // No grouped Conv.
  if (filter_in_c != input_c) {
    return false;
  }

  if (filter_out_c != output_c) {
    return false;
  }

  // Dilation has no useful effect for 1x1, but reject unusual cases.
  if (params.dilation_height_factor != 1 ||
      params.dilation_width_factor != 1) {
    return false;
  }

  if (batches <= 0 || input_h <= 0 || input_w <= 0 || input_c <= 0 ||
      output_h <= 0 || output_w <= 0 || output_c <= 0) {
    return false;
  }

  if (input_c > static_cast<int>(CONV_MAX_WEIGHT_BYTES)) {
    return false;
  }

  // Need at least one spatial pixel to fit in input and output SPAD.
  if (input_c > static_cast<int>(CONV_MAX_INPUT_BYTES)) {
    return false;
  }

  if (CONV_MAX_OUTPUT_BYTES < 1u) {
    return false;
  }

  return true;
}

// ================================================================
// Choose spatial tile shape
//
// For pointwise Conv, each output pixel needs one input pixel's channels.
// Therefore input tile bytes:
//   tile_h * tile_w * input_c
//
// Output tile bytes for one output channel:
//   tile_h * tile_w
// ================================================================

static inline bool ConvAccelChoosePointwiseTileShape(
    int output_h,
    int output_w,
    int input_c,
    int* tile_h_out,
    int* tile_w_out) {
  if (output_h <= 0 || output_w <= 0 || input_c <= 0) {
    return false;
  }

  const uint32_t max_pixels_by_input =
      CONV_MAX_INPUT_BYTES / static_cast<uint32_t>(input_c);
  const uint32_t max_pixels_by_output = CONV_MAX_OUTPUT_BYTES;

  uint32_t max_pixels =
      (max_pixels_by_input < max_pixels_by_output)
          ? max_pixels_by_input
          : max_pixels_by_output;

  if (max_pixels == 0u) {
    return false;
  }

  int tile_w = output_w;
  if (static_cast<uint32_t>(tile_w) > max_pixels) {
    tile_w = static_cast<int>(max_pixels);
  }

  if (tile_w <= 0) {
    return false;
  }

  int tile_h = static_cast<int>(max_pixels / static_cast<uint32_t>(tile_w));

  if (tile_h <= 0) {
    tile_h = 1;
  }

  if (tile_h > output_h) {
    tile_h = output_h;
  }

  *tile_h_out = tile_h;
  *tile_w_out = tile_w;
  return true;
}

// ================================================================
// Build input tile in normal memory.
//
// This is the corrected version of your input tile calculator.
//
// For pointwise 1x1:
//   output pixel (out_y, out_x) maps to:
//   input_y = out_y * stride_h - pad_h
//   input_x = out_x * stride_w - pad_w
//
// The tile is packed as:
//   [tile_y][tile_x][input_channel]
//
// Then DMA sends this packed tile to local input SPAD.
// ================================================================

static inline void ConvAccelBuildPointwiseInputTile(
    const ConvParams& params,
    const RuntimeShape& input_shape,
    const int8_t* input_data,
    int batch,
    int tile_out_y_start,
    int tile_out_x_start,
    int tile_out_h,
    int tile_out_w,
    int input_c,
    int8_t* tile_buffer) {
  const int input_h = input_shape.Dims(1);
  const int input_w = input_shape.Dims(2);

  const int stride_h = params.stride_height;
  const int stride_w = params.stride_width;
  const int pad_h = params.padding_values.height;
  const int pad_w = params.padding_values.width;

  // TFLM uses input_val + input_offset.
  // The padding value should be the input zero point = -input_offset.
  const int32_t zero_point_i32 = -params.input_offset;
  const int8_t pad_value = static_cast<int8_t>(zero_point_i32);

  int tile_index = 0;

  for (int ty = 0; ty < tile_out_h; ++ty) {
    const int out_y = tile_out_y_start + ty;
    const int in_y = out_y * stride_h - pad_h;

    for (int tx = 0; tx < tile_out_w; ++tx) {
      const int out_x = tile_out_x_start + tx;
      const int in_x = out_x * stride_w - pad_w;

      const bool inside =
          (in_y >= 0) && (in_y < input_h) &&
          (in_x >= 0) && (in_x < input_w);

      for (int ic = 0; ic < input_c; ++ic) {
        if (inside) {
          tile_buffer[tile_index++] =
              input_data[Offset(input_shape, batch, in_y, in_x, ic)];
        } else {
          tile_buffer[tile_index++] = pad_value;
        }
      }
    }
  }

  // Clear a few extra bytes so DMA beat rounding is safe.
  const uint32_t useful_bytes =
      static_cast<uint32_t>(tile_out_h * tile_out_w * input_c);
  const uint32_t rounded_bytes = ConvRoundUpToDmaBeat(useful_bytes);

  for (uint32_t i = useful_bytes; i < rounded_bytes; ++i) {
    tile_buffer[i] = 0;
  }
}

// ================================================================
// Build one output channel's pointwise weights in normal memory.
//
// filter layout:
//   [output_c, 1, 1, input_c]
//
// one channel weights:
//   filter_data[oc * input_c + ic]
// ================================================================

static inline void ConvAccelBuildPointwiseWeightTile(
    const int8_t* filter_data,
    int output_channel,
    int input_c,
    int8_t* weight_buffer) {
  const int8_t* weight_for_oc = filter_data + (output_channel * input_c);

  for (int ic = 0; ic < input_c; ++ic) {
    weight_buffer[ic] = weight_for_oc[ic];
  }

  // Clear extra bytes so DMA beat rounding is safe.
  const uint32_t useful_bytes = static_cast<uint32_t>(input_c);
  const uint32_t rounded_bytes = ConvRoundUpToDmaBeat(useful_bytes);

  for (uint32_t i = useful_bytes; i < rounded_bytes; ++i) {
    weight_buffer[i] = 0;
  }
}

// ================================================================
// Program CSR for one output channel and one spatial tile
// ================================================================

static inline void ConvAccelProgramCsrsForPointwiseTile(
    int input_c,
    int tile_h,
    int tile_w,
    int32_t quant_shift,
    int32_t quant_mult,
    uint32_t input_spad_start,
    uint32_t input_spad_end,
    uint32_t weight_spad_start,
    uint32_t weight_spad_end) {
  const uint32_t tile_pixels = static_cast<uint32_t>(tile_h * tile_w);

  ConvWrite32(CONV_CSR_BASE + CONV_CSR_CONV_MODE, CONV_MODE_POINTWISE);
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_P_MODE, CONV_P_MODE_POINTWISE);

  // Since software packs the tile as a compact pointwise input tile,
  // the accelerator sees this tile as its entire input image.
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_I_SIZE, tile_pixels);
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_I_C_SIZE,
              static_cast<uint32_t>(input_c));

  // One output channel per run.
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_O_C_SIZE, 1u);
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_O_SIZE, tile_pixels);

  // Software already handled original stride/padding when packing the tile.
  // So accelerator should process packed tile with stride 1.
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_STRIDE, 1u);

  // Pointwise Conv does not use depth multiplier.
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_DEPTH_MULT, 1u);

  // These are LOCAL SPAD addresses, not AXI addresses.
  // This code uses inclusive end addresses: start + bytes - 1.
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_I_START_ADDR, input_spad_start);
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_I_ADDR_END, input_spad_end);
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_W_START_ADDR, weight_spad_start);
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_W_ADDR_END, weight_spad_end);

  // Accelerator applies quantization.
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_QUANT_SH,
              ConvAccelQuantShiftForCsr(quant_shift));
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_QUANT_MULT,
              ConvAccelQuantMultForCsr(quant_mult));
}

// ================================================================
// Output AXI SPAD -> TFLM output_data
//
// Output SPAD is AXI/CPU-readable.
// Accelerator output is already final int8 for one output channel and tile.
//
// Output SPAD layout assumed:
//   spad_out[ty * tile_w + tx]
//
// TFLM output layout:
//   output_data[batch, out_y, out_x, output_channel]
// ================================================================

static inline bool ConvAccelStorePointwiseOutputTileFromAxiSpad(
    const RuntimeShape& output_shape,
    int batch,
    int output_channel,
    int tile_out_y_start,
    int tile_out_x_start,
    int tile_out_h,
    int tile_out_w,
    int8_t* output_data) {
  const int output_h = output_shape.Dims(1);
  const int output_w = output_shape.Dims(2);
  const int output_c = output_shape.Dims(3);

  if (batch < 0 || batch >= output_shape.Dims(0)) {
    return false;
  }

  if (output_channel < 0 || output_channel >= output_c) {
    return false;
  }

  if (tile_out_y_start < 0 || tile_out_x_start < 0) {
    return false;
  }

  if ((tile_out_y_start + tile_out_h) > output_h ||
      (tile_out_x_start + tile_out_w) > output_w) {
    return false;
  }

  volatile int8_t* spad_out =
      reinterpret_cast<volatile int8_t*>(CONV_SPAD_O_AXI_BASE);

  for (int ty = 0; ty < tile_out_h; ++ty) {
    const int out_y = tile_out_y_start + ty;

    for (int tx = 0; tx < tile_out_w; ++tx) {
      const int out_x = tile_out_x_start + tx;
      const int spad_index = ty * tile_out_w + tx;

      output_data[Offset(output_shape, batch, out_y, out_x, output_channel)] =
          spad_out[spad_index];
    }
  }

  return true;
}

// ================================================================
// Main spatial-tiled accelerator path
// ================================================================

static inline bool TryConvAccelPointwisePerChannel(
    const ConvParams& params,
    const int32_t* output_multiplier,
    const int32_t* output_shift,
    const RuntimeShape& input_shape,
    const int8_t* input_data,
    const RuntimeShape& filter_shape,
    const int8_t* filter_data,
    const RuntimeShape& bias_shape,
    const int32_t* bias_data,
    const RuntimeShape& output_shape,
    int8_t* output_data) {
  if (!ConvAccelCanRunPointwise(params, input_shape, filter_shape,
                                output_shape, output_multiplier,
                                output_shift)) {
    return false;
  }

  const int batches = MatchingDim(input_shape, 0, output_shape, 0);

  const int input_h = input_shape.Dims(1);
  const int input_w = input_shape.Dims(2);
  const int input_c = input_shape.Dims(3);

  const int output_h = output_shape.Dims(1);
  const int output_w = output_shape.Dims(2);
  const int output_c = MatchingDim(filter_shape, 0, output_shape, 3);

  if (bias_data) {
    TFLITE_DCHECK_EQ(bias_shape.FlatSize(), output_c);
  }

  int tile_h_max = 0;
  int tile_w_max = 0;

  if (!ConvAccelChoosePointwiseTileShape(output_h, output_w, input_c,
                                         &tile_h_max, &tile_w_max)) {
    return false;
  }

  ConvAccelPrintTag4('A', 'C', '0', ' ');
  ConvAccelPrintSummary(input_h, input_w, input_c,
                        output_h, output_w, output_c,
                        tile_h_max, tile_w_max);

  for (int batch = 0; batch < batches; ++batch) {
    for (int oc = 0; oc < output_c; ++oc) {
      // Build and load one output channel's weights once per oc.
      ConvAccelBuildPointwiseWeightTile(
          filter_data, oc, input_c, g_conv_accel_weight_tile);

      const uint32_t weight_bytes = static_cast<uint32_t>(input_c);

      if (!ConvDmaCopyToLocalSpad(1,
                                  ConvPtrToU32(g_conv_accel_weight_tile),
                                  CONV_SPAD_W_LOCAL_BASE,
                                  weight_bytes,
                                  CONV_SPAD_SEL_WEIGHTS)) {
        ConvAccelPrintTag4('A', 'W', 'F', ' ');
        return false;
      }

#if CONV_ACCEL_LOAD_BIAS_SPAD
      if (bias_data) {
        g_conv_accel_bias_tile = bias_data[oc];

        if (!ConvDmaCopyToLocalSpad(2,
                                    ConvPtrToU32(&g_conv_accel_bias_tile),
                                    CONV_SPAD_B_LOCAL_BASE,
                                    sizeof(int32_t),
                                    CONV_SPAD_SEL_BIAS)) {
          ConvAccelPrintTag4('A', 'B', 'F', ' ');
          return false;
        }
      }
#else
      (void)bias_shape;
      (void)bias_data;
#endif

      for (int tile_y = 0; tile_y < output_h; tile_y += tile_h_max) {
        const int tile_h = ConvMinInt(tile_h_max, output_h - tile_y);

        for (int tile_x = 0; tile_x < output_w; tile_x += tile_w_max) {
          const int tile_w = ConvMinInt(tile_w_max, output_w - tile_x);

          const uint32_t input_tile_bytes =
              static_cast<uint32_t>(tile_h * tile_w * input_c);
          const uint32_t output_tile_bytes =
              static_cast<uint32_t>(tile_h * tile_w);

          if (input_tile_bytes > CONV_MAX_INPUT_BYTES ||
              output_tile_bytes > CONV_MAX_OUTPUT_BYTES) {
            ConvAccelPrintTag4('A', 'T', 'F', ' ');
            return false;
          }

          // Build compact spatial input tile in normal memory.
          ConvAccelBuildPointwiseInputTile(
              params,
              input_shape,
              input_data,
              batch,
              tile_y,
              tile_x,
              tile_h,
              tile_w,
              input_c,
              g_conv_accel_input_tile);

          // DMA input tile to local input SPAD.
          if (!ConvDmaCopyToLocalSpad(0,
                                      ConvPtrToU32(g_conv_accel_input_tile),
                                      CONV_SPAD_I_LOCAL_BASE,
                                      input_tile_bytes,
                                      CONV_SPAD_SEL_INPUTS)) {
            ConvAccelPrintTag4('A', 'I', 'F', ' ');
            return false;
          }

          // LOCAL SPAD address ranges written to CSR.
          const uint32_t input_start = CONV_SPAD_I_LOCAL_BASE;
          const uint32_t input_end =
              CONV_SPAD_I_LOCAL_BASE + input_tile_bytes - 1u;

          const uint32_t weight_start = CONV_SPAD_W_LOCAL_BASE;
          const uint32_t weight_end =
              CONV_SPAD_W_LOCAL_BASE + weight_bytes - 1u;

          ConvAccelProgramCsrsForPointwiseTile(
              input_c,
              tile_h,
              tile_w,
              output_shift[oc],
              output_multiplier[oc],
              input_start,
              input_end,
              weight_start,
              weight_end);

          // Clear routers before each tile run.
          ConvAccelClearRouters();

          // Start accelerator. CTRL pulse auto-clears in systolic_csr.v.
          ConvAccelStart();

          if (!ConvAccelWaitDone()) {
            ConvAccelPrintTag4('A', 'T', 'O', ' ');
            RawPutcD('o'); RawPutcD('c');
            RawPutHex32D(static_cast<uint32_t>(oc));
            RawPutcD(' ');
            RawPutcD('y');
            RawPutHex32D(static_cast<uint32_t>(tile_y));
            RawPutcD(' ');
            RawPutcD('x');
            RawPutHex32D(static_cast<uint32_t>(tile_x));
            RawPutcD(' ');
            RawPutcD('s'); RawPutcD('t');
            RawPutHex32D(ConvRead32(CONV_CSR_BASE + CONV_CSR_STATUS));
            RawNewlineD();
            return false;
          }

          if (!ConvAccelStorePointwiseOutputTileFromAxiSpad(
                  output_shape,
                  batch,
                  oc,
                  tile_y,
                  tile_x,
                  tile_h,
                  tile_w,
                  output_data)) {
            ConvAccelPrintTag4('A', 'O', 'F', ' ');
            return false;
          }
        }
      }
    }
  }

  ConvAccelPrintTag4('A', 'C', '1', ' ');
  return true;
}

#endif  // TFLM_USE_CONV_ACCEL


// ================================================================
// int8 ConvPerChannel replacement
// ================================================================

inline void ConvPerChannel(
    const ConvParams& params, const int32_t* output_multiplier,
    const int32_t* output_shift, const RuntimeShape& input_shape,
    const int8_t* input_data, const RuntimeShape& filter_shape,
    const int8_t* filter_data, const RuntimeShape& bias_shape,
    const int32_t* bias_data, const RuntimeShape& output_shape,
    int8_t* output_data) {
  // Get parameters.
  const int32_t input_offset = params.input_offset;
  const int stride_width = params.stride_width;
  const int stride_height = params.stride_height;
  const int dilation_width_factor = params.dilation_width_factor;
  const int dilation_height_factor = params.dilation_height_factor;
  const int pad_width = params.padding_values.width;
  const int pad_height = params.padding_values.height;
  const int32_t output_offset = params.output_offset;

  const int32_t output_activation_min = params.quantized_activation_min;
  const int32_t output_activation_max = params.quantized_activation_max;

  TFLITE_DCHECK_LE(output_activation_min, output_activation_max);
  TFLITE_DCHECK_EQ(input_shape.DimensionsCount(), 4);
  TFLITE_DCHECK_EQ(filter_shape.DimensionsCount(), 4);
  TFLITE_DCHECK_EQ(output_shape.DimensionsCount(), 4);

  const int batches = MatchingDim(input_shape, 0, output_shape, 0);
  const int input_depth = input_shape.Dims(3);
  const int output_depth = MatchingDim(filter_shape, 0, output_shape, 3);

  if (bias_data) {
    TFLITE_DCHECK_EQ(bias_shape.FlatSize(), output_depth);
  }

  const int input_height = input_shape.Dims(1);
  const int input_width = input_shape.Dims(2);
  const int filter_height = filter_shape.Dims(1);
  const int filter_width = filter_shape.Dims(2);
  const int filter_input_depth = filter_shape.Dims(3);

  const int groups = input_depth / filter_input_depth;
  TFLITE_DCHECK_NE(groups, 0);
  TFLITE_DCHECK_EQ(input_depth % filter_input_depth, 0);

  const int filters_per_group = output_depth / groups;
  TFLITE_DCHECK_NE(filters_per_group, 0);

  const int output_height = output_shape.Dims(1);
  const int output_width = output_shape.Dims(2);

#if TFLM_USE_CONV_ACCEL
  // Try accelerator only for supported pointwise Conv.
  // If unsupported or failed, fall through to CPU Conv.
  if (TryConvAccelPointwisePerChannel(params, output_multiplier, output_shift,
                                      input_shape, input_data,
                                      filter_shape, filter_data,
                                      bias_shape, bias_data,
                                      output_shape, output_data)) {
    return;
  }
#endif

  // ================================================================
  // Original CPU reference path.
  // Keeps non-pointwise Conv, 3x3 Conv, padded Conv, grouped Conv,
  // or failed accelerator runs working normally.
  // ================================================================

  for (int batch = 0; batch < batches; ++batch) {
    for (int out_y = 0; out_y < output_height; ++out_y) {
      const int in_y_origin = (out_y * stride_height) - pad_height;

      for (int out_x = 0; out_x < output_width; ++out_x) {
        const int in_x_origin = (out_x * stride_width) - pad_width;

        for (int out_channel = 0; out_channel < output_depth; ++out_channel) {
          const int group = out_channel / filters_per_group;
          int32_t acc = 0;

          for (int filter_y = 0; filter_y < filter_height; ++filter_y) {
            const int in_y = in_y_origin + dilation_height_factor * filter_y;

            for (int filter_x = 0; filter_x < filter_width; ++filter_x) {
              const int in_x = in_x_origin + dilation_width_factor * filter_x;

              const bool is_point_inside_image =
                  (in_x >= 0) && (in_x < input_width) &&
                  (in_y >= 0) && (in_y < input_height);

              if (!is_point_inside_image) {
                continue;
              }

              for (int in_channel = 0; in_channel < filter_input_depth;
                   ++in_channel) {
                const int32_t input_val =
                    input_data[Offset(input_shape, batch, in_y, in_x,
                                      in_channel +
                                          group * filter_input_depth)];

                const int32_t filter_val =
                    filter_data[Offset(filter_shape, out_channel, filter_y,
                                       filter_x, in_channel)];

                acc += filter_val * (input_val + input_offset);
              }
            }
          }

          if (bias_data) {
            acc += bias_data[out_channel];
          }

          acc = MultiplyByQuantizedMultiplier(
              acc, output_multiplier[out_channel], output_shift[out_channel]);

          acc += output_offset;
          acc = std::max(acc, output_activation_min);
          acc = std::min(acc, output_activation_max);

          output_data[Offset(output_shape, batch, out_y, out_x, out_channel)] =
              static_cast<int8_t>(acc);
        }
      }
    }
  }
}