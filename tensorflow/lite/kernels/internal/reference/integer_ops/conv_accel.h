
// Modified conv.h
//   - Accelerator already applies quantization.
//   - CSR has one quant_sh and one quant_mult.
//   - Therefore software runs one output channel at a time.
//   - Output SPAD contains final int8 values for one output channel.
//   - Software scatters that channel into TFLM NHWC output_data.
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


#define CONV_SPAD_SEL_WEIGHTS     0u  // 000
#define CONV_SPAD_SEL_INPUTS      1u  // 001
#define CONV_SPAD_SEL_BIAS        2u  // 010
#define CONV_SPAD_SEL_SCALE       3u  // 011
#define CONV_SPAD_SEL_SHIFT       4u  // 100

// ================================================================
// BEWARE!! MMIO base addresses
// Must match Vivado Address Editor.
// ================================================================

#define CONV_CSR_BASE             0x40001000u

#define CONV_SPAD_W_BASE          0x40010000u
#define CONV_SPAD_I_BASE          0x40020000u
#define CONV_SPAD_B_BASE          0x40030000u
#define CONV_SPAD_M_BASE          0x40040000u
#define CONV_SPAD_S_BASE          0x40050000u
#define CONV_SPAD_O_BASE          0x40060000u

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

// Check p_mode
#define CONV_P_MODE_POINTWISE     0u

#define CONV_CTRL_START           0x00000001u
#define CONV_CTRL_REG_CLEAR       0x00000002u
#define CONV_STATUS_DONE          0x00000001u

// ================================================================
// DMA beat size: 4 bytes for 32-bit.
// ================================================================

#define CONV_DMA_BEAT_BYTES       4u

// ================================================================
// SPAD capacity limits
// ================================================================

#define CONV_MAX_INPUT_BYTES      (128u * 1024u)
#define CONV_MAX_WEIGHT_BYTES     (128u * 1024u)
#define CONV_MAX_BIAS_BYTES       (16u  * 1024u)
#define CONV_MAX_OUTPUT_BYTES     (128u * 1024u)


#ifndef CONV_ACCEL_LOAD_BIAS_SPAD
#define CONV_ACCEL_LOAD_BIAS_SPAD 1
#endif

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
                                         int output_c) {
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

static inline bool ConvDmaCopy(int ch,
                               uint32_t src,
                               uint32_t dst,
                               uint32_t len_bytes,
                               uint32_t spad_sel) {
  if (len_bytes == 0u) {
    return true;
  }

  // Your DMA computes burst length as len / BPB.
  // So len must be at least one full beat and beat-aligned.
  const uint32_t dma_len = ConvRoundUpToDmaBeat(len_bytes);

  *ConvDmaReg(ch, CONV_DMA_SRC)      = src;
  *ConvDmaReg(ch, CONV_DMA_DST)      = dst;
  *ConvDmaReg(ch, CONV_DMA_LEN)      = dma_len;
  *ConvDmaReg(ch, CONV_DMA_SPAD_SEL) = spad_sel;

  // START must be written last.
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
//
// FILL RTL DETAIL HERE:
// This assumes output_shift[] already contains the right-shift amount
// expected by the accelerator, and output_multiplier[] already contains the
// 16-bit fixed-point multiplier expected by the accelerator.
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

  // First integration: pointwise Conv only.
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

  // No dilation.
  if (params.dilation_height_factor != 1 ||
      params.dilation_width_factor != 1) {
    return false;
  }

  // No padding for this pointwise path.
  if (params.padding_values.height != 0 ||
      params.padding_values.width != 0) {
    return false;
  }

  if (batches <= 0 || input_h <= 0 || input_w <= 0 || input_c <= 0 ||
      output_h <= 0 || output_w <= 0 || output_c <= 0) {
    return false;
  }

  const uint32_t single_batch_input_bytes =
      static_cast<uint32_t>(input_h * input_w * input_c);

  const uint32_t one_channel_weight_bytes =
      static_cast<uint32_t>(input_c);

  const uint32_t one_channel_output_bytes =
      static_cast<uint32_t>(output_h * output_w);

  if (single_batch_input_bytes > CONV_MAX_INPUT_BYTES) {
    return false;
  }

  if (one_channel_weight_bytes > CONV_MAX_WEIGHT_BYTES) {
    return false;
  }

  if (one_channel_output_bytes > CONV_MAX_OUTPUT_BYTES) {
    return false;
  }

  return true;
}

// ================================================================
// Program CSR for one output channel
// ================================================================

static inline void ConvAccelProgramCsrsForOneOutputChannel(
    const ConvParams& params,
    int input_h,
    int input_w,
    int input_c,
    int output_h,
    int output_w,
    int output_channel,
    int32_t quant_shift,
    int32_t quant_mult,
    uint32_t input_spad_start,
    uint32_t input_spad_end,
    uint32_t weight_spad_start,
    uint32_t weight_spad_end) {
  const uint32_t i_size = static_cast<uint32_t>(input_h * input_w);
  const uint32_t o_size = static_cast<uint32_t>(output_h * output_w);

  ConvWrite32(CONV_CSR_BASE + CONV_CSR_CONV_MODE, CONV_MODE_POINTWISE);
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_P_MODE, CONV_P_MODE_POINTWISE);

  ConvWrite32(CONV_CSR_BASE + CONV_CSR_I_SIZE, i_size);
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_I_C_SIZE,
              static_cast<uint32_t>(input_c));

  // One output channel per accelerator run because CSR has only one
  // quant_mult and one quant_sh.
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_O_C_SIZE, 1u);
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_O_SIZE, o_size);

  // CSR has one stride register.
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_STRIDE,
              static_cast<uint32_t>(params.stride_height));

  // Pointwise Conv does not use depth multiplier.
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_DEPTH_MULT, 1u);

  // Your RTL exposes start/end addresses.
  // This code uses inclusive end addresses: start + bytes - 1.
  //
  // FILL RTL DETAIL HERE:
  // If your top.sv/router expects exclusive end, change these to
  // start + bytes instead.
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_I_START_ADDR, input_spad_start);
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_I_ADDR_END, input_spad_end);
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_W_START_ADDR, weight_spad_start);
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_W_ADDR_END, weight_spad_end);

  // Accelerator applies quantization.
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_QUANT_SH,
              ConvAccelQuantShiftForCsr(quant_shift));
  ConvWrite32(CONV_CSR_BASE + CONV_CSR_QUANT_MULT,
              ConvAccelQuantMultForCsr(quant_mult));

  (void)output_channel;
}

// ================================================================
// Output SPAD copies to TFLM output_data
//
// Accelerator output is already final int8 for one output channel.
// Output SPAD layout assumed:
//   spad_out[out_y * output_w + out_x]
// ================================================================

static inline bool ConvAccelStoreOneChannelFromSpadToOutputData(
    const RuntimeShape& output_shape,
    int batch,
    int output_channel,
    int output_h,
    int output_w,
    int8_t* output_data) {
  if (batch < 0 || batch >= output_shape.Dims(0)) {
    return false;
  }

  if (output_channel < 0 || output_channel >= output_shape.Dims(3)) {
    return false;
  }

  if (output_h > output_shape.Dims(1) ||
      output_w > output_shape.Dims(2)) {
    return false;
  }

  volatile int8_t* spad_out =
      reinterpret_cast<volatile int8_t*>(CONV_SPAD_O_BASE);

  for (int out_y = 0; out_y < output_h; ++out_y) {
    for (int out_x = 0; out_x < output_w; ++out_x) {
      const int spad_index = out_y * output_w + out_x;

      output_data[Offset(output_shape, batch, out_y, out_x, output_channel)] =
          spad_out[spad_index];
    }
  }

  return true;
}

// ================================================================
// Main accelerator path
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

  const uint32_t single_batch_input_bytes =
      static_cast<uint32_t>(input_h * input_w * input_c);

  const uint32_t one_channel_weight_bytes =
      static_cast<uint32_t>(input_c);

  const uint32_t one_channel_output_bytes =
      static_cast<uint32_t>(output_h * output_w);

  if (one_channel_output_bytes > CONV_MAX_OUTPUT_BYTES) {
    return false;
  }

  ConvAccelPrintTag4('A', 'C', '0', ' ');
  ConvAccelPrintSummary(input_h, input_w, input_c,
                        output_h, output_w, output_c);

  for (int batch = 0; batch < batches; ++batch) {
    const int8_t* batch_input =
        input_data + (batch * static_cast<int>(single_batch_input_bytes));

    // Load input for this batch once.
    if (!ConvDmaCopy(0,
                     ConvPtrToU32(batch_input),
                     CONV_SPAD_I_BASE,
                     single_batch_input_bytes,
                     CONV_SPAD_SEL_INPUTS)) {
      ConvAccelPrintTag4('A', 'I', 'F', ' ');
      return false;
    }

    for (int oc = 0; oc < output_c; ++oc) {
      // Pointwise filter layout:
      //   filter_shape = [output_c, 1, 1, input_c]
      // Therefore one output channel's weights are contiguous:
      //   &filter_data[oc * input_c]
      const int8_t* weight_for_oc = filter_data + (oc * input_c);

      if (!ConvDmaCopy(1,
                       ConvPtrToU32(weight_for_oc),
                       CONV_SPAD_W_BASE,
                       one_channel_weight_bytes,
                       CONV_SPAD_SEL_WEIGHTS)) {
        ConvAccelPrintTag4('A', 'W', 'F', ' ');
        return false;
      }

#if CONV_ACCEL_LOAD_BIAS_SPAD
      // Bias is optional. CSR has no bias address, so this assumes your
      // top.sv/systolic reads bias from a fixed bias SPAD location.
      if (bias_data) {
        const int32_t* bias_for_oc = bias_data + oc;

        if (!ConvDmaCopy(2,
                         ConvPtrToU32(bias_for_oc),
                         CONV_SPAD_B_BASE,
                         sizeof(int32_t),
                         CONV_SPAD_SEL_BIAS)) {
          ConvAccelPrintTag4('A', 'B', 'F', ' ');
          return false;
        }
      }
#endif

      // SPAD address range written to CSR.
      // Actual useful bytes are used for the end address, not rounded DMA len.
      const uint32_t input_start = CONV_SPAD_I_BASE;
      const uint32_t input_end =
          CONV_SPAD_I_BASE + single_batch_input_bytes - 1u;

      const uint32_t weight_start = CONV_SPAD_W_BASE;
      const uint32_t weight_end =
          CONV_SPAD_W_BASE + one_channel_weight_bytes - 1u;

      ConvAccelProgramCsrsForOneOutputChannel(
          params,
          input_h,
          input_w,
          input_c,
          output_h,
          output_w,
          oc,
          output_shift[oc],
          output_multiplier[oc],
          input_start,
          input_end,
          weight_start,
          weight_end);

      // Clear routers before each run.
      ConvAccelClearRouters();

      // Start accelerator. CTRL pulse auto-clears in systolic_csr.v.
      ConvAccelStart();

      if (!ConvAccelWaitDone()) {
        ConvAccelPrintTag4('A', 'T', 'O', ' ');
        return false;
      }

      if (!ConvAccelStoreOneChannelFromSpadToOutputData(
              output_shape,
              batch,
              oc,
              output_h,
              output_w,
              output_data)) {
        ConvAccelPrintTag4('A', 'O', 'F', ' ');
        return false;
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