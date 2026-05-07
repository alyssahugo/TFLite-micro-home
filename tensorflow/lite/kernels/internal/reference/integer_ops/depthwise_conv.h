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
#ifndef TENSORFLOW_LITE_KERNELS_INTERNAL_REFERENCE_INTEGER_OPS_DEPTHWISE_CONV_H_
#define TENSORFLOW_LITE_KERNELS_INTERNAL_REFERENCE_INTEGER_OPS_DEPTHWISE_CONV_H_

#include <algorithm>

#include "tensorflow/lite/kernels/internal/common.h"


#include <stdint.h>

static inline void RawPutcD(char c) {
  volatile uint32_t* const uart_tx =
      reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
  volatile uint32_t* const uart_status =
      reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x08u);

  while ((*uart_status) & 0x08u) {
  }

  *uart_tx = static_cast<uint32_t>(static_cast<uint8_t>(c));
}

static inline void RawNewlineD() {
  RawPutcD('\r');
  RawPutcD('\n');
}

static inline void RawPutHexNibbleD(uint32_t v) {
  v &= 0xFu;
  RawPutcD((v < 10u) ? static_cast<char>('0' + v)
                    : static_cast<char>('A' + (v - 10u)));
}

static inline void RawPutHex32D(uint32_t v) {
  for (int shift = 28; shift >= 0; shift -= 4) {
    RawPutHexNibbleD(v >> shift);
  }
}

static inline void RawTagHexD(char tag, uint32_t v) {
  RawPutcD(tag);
  RawPutHex32D(v);
  RawPutcD(' ');
}


namespace tflite {
namespace reference_integer_ops {
// inline void DepthwiseConvPerChannel(
//     const DepthwiseParams& params, const int32_t* output_multiplier,
//     const int32_t* output_shift, const RuntimeShape& input_shape,
//     const int8_t* input_data, const RuntimeShape& filter_shape,
//     const int8_t* filter_data, const RuntimeShape& bias_shape,
//     const int32_t* bias_data, const RuntimeShape& output_shape,
//     int8_t* output_data) {
//   // Get parameters.
//   // TODO(b/141565753): Re-introduce ScopedProfilingLabel on Micro.
//   const int stride_width = params.stride_width;
//   const int stride_height = params.stride_height;
//   const int dilation_width_factor = params.dilation_width_factor;
//   const int dilation_height_factor = params.dilation_height_factor;
//   const int pad_width = params.padding_values.width;
//   const int pad_height = params.padding_values.height;
//   const int depth_multiplier = params.depth_multiplier;
//   const int32_t input_offset = params.input_offset;
//   const int32_t output_offset = params.output_offset;
//   const int32_t output_activation_min = params.quantized_activation_min;
//   const int32_t output_activation_max = params.quantized_activation_max;

//   // Check dimensions of the tensors.
//   TFLITE_DCHECK_EQ(input_shape.DimensionsCount(), 4);
//   TFLITE_DCHECK_EQ(filter_shape.DimensionsCount(), 4);
//   TFLITE_DCHECK_EQ(output_shape.DimensionsCount(), 4);

//   TFLITE_DCHECK_LE(output_activation_min, output_activation_max);
//   const int batches = MatchingDim(input_shape, 0, output_shape, 0);
//   const int output_depth = MatchingDim(filter_shape, 3, output_shape, 3);
//   const int input_height = input_shape.Dims(1);
//   const int input_width = input_shape.Dims(2);
//   const int input_depth = input_shape.Dims(3);
//   // const int input_height = input_shape.Dims(1);
//   // volatile int input_width_v = input_shape.Dims(2);
//   // const int input_width = input_width_v;
//   // const int input_depth = input_shape.Dims(3);
//   const int filter_height = filter_shape.Dims(1);
//   const int filter_width = filter_shape.Dims(2);
//   const int output_height = output_shape.Dims(1);
//   const int output_width = output_shape.Dims(2);
//   TFLITE_DCHECK_EQ(output_depth, input_shape.Dims(3) * depth_multiplier);
//   TFLITE_DCHECK_EQ(bias_shape.FlatSize(), output_depth);
//   RawPutcD('['); RawPutcD('D'); RawPutcD('W'); RawPutcD('S'); RawPutcD(']');
//   RawTagHexD('b', static_cast<uint32_t>(batches));
//   RawTagHexD('h', static_cast<uint32_t>(input_height));
//   RawTagHexD('w', static_cast<uint32_t>(input_width));
//   RawTagHexD('W', static_cast<uint32_t>(input_shape.Dims(2)));
//   RawTagHexD('c', static_cast<uint32_t>(input_depth));
//   RawTagHexD('H', static_cast<uint32_t>(output_height));
//   RawTagHexD('W', static_cast<uint32_t>(output_width));
//   RawTagHexD('C', static_cast<uint32_t>(output_depth));
//   RawTagHexD('o', static_cast<uint32_t>(
//                      reinterpret_cast<uintptr_t>(output_data)));
//   RawNewlineD();
//   RawPutcD('['); RawPutcD('D'); RawPutcD('S'); RawPutcD('I'); RawPutcD(']');
//   RawTagHexD('0', static_cast<uint32_t>(input_shape.Dims(0)));
//   RawTagHexD('1', static_cast<uint32_t>(input_shape.Dims(1)));
//   RawTagHexD('2', static_cast<uint32_t>(input_shape.Dims(2)));
//   RawTagHexD('3', static_cast<uint32_t>(input_shape.Dims(3)));
//   RawNewlineD();

//   RawPutcD('['); RawPutcD('D'); RawPutcD('S'); RawPutcD('O'); RawPutcD(']');
//   RawTagHexD('0', static_cast<uint32_t>(output_shape.Dims(0)));
//   RawTagHexD('1', static_cast<uint32_t>(output_shape.Dims(1)));
//   RawTagHexD('2', static_cast<uint32_t>(output_shape.Dims(2)));
//   RawTagHexD('3', static_cast<uint32_t>(output_shape.Dims(3)));
//   RawNewlineD();

//   RawPutcD('['); RawPutcD('D'); RawPutcD('S'); RawPutcD('F'); RawPutcD(']');
//   RawTagHexD('0', static_cast<uint32_t>(filter_shape.Dims(0)));
//   RawTagHexD('1', static_cast<uint32_t>(filter_shape.Dims(1)));
//   RawTagHexD('2', static_cast<uint32_t>(filter_shape.Dims(2)));
//   RawTagHexD('3', static_cast<uint32_t>(filter_shape.Dims(3)));
//   RawNewlineD();
//   for (int batch = 0; batch < batches; ++batch) {
//     for (int out_y = 0; out_y < output_height; ++out_y) {
//       for (int out_x = 0; out_x < output_width; ++out_x) {
//         for (int in_channel = 0; in_channel < input_depth; ++in_channel) {
//           for (int m = 0; m < depth_multiplier; ++m) {
//             const int output_channel = m + in_channel * depth_multiplier;
//             const int in_x_origin = (out_x * stride_width) - pad_width;
//             const int in_y_origin = (out_y * stride_height) - pad_height;
//             int32_t acc = 0;
//             for (int filter_y = 0; filter_y < filter_height; ++filter_y) {
//               for (int filter_x = 0; filter_x < filter_width; ++filter_x) {
//                 const int in_x = in_x_origin + dilation_width_factor * filter_x;
//                 const int in_y =
//                     in_y_origin + dilation_height_factor * filter_y;
//                 // Zero padding by omitting the areas outside the image.
//                 const bool is_point_inside_image =
//                     (in_x >= 0) && (in_x < input_shape.Dims(2)) && (in_y >= 0) &&
//                     (in_y < input_height);
//                 if (is_point_inside_image) {
//                   int32_t input_val = input_data[Offset(
//                       input_shape, batch, in_y, in_x, in_channel)];
//                   int32_t filter_val = filter_data[Offset(
//                       filter_shape, 0, filter_y, filter_x, output_channel)];
//                   // Accumulate with 32 bits accumulator.
//                   // In the nudging process during model quantization, we force
//                   // real value of 0.0 be represented by a quantized value. This
//                   // guarantees that the input_offset is a int8_t, even though
//                   // it is represented using int32_t. int32_t += int8_t *
//                   // (int8_t - int8_t) so the highest value we can get from each
//                   // accumulation is [-127, 127] * ([-128, 127] -
//                   // [-128, 127]), which is [-32512, 32512]. log2(32512)
//                   // = 14.98, which means we can accumulate at least 2^16
//                   // multiplications without overflow. The accumulator is
//                   // applied to a filter so the accumulation logic will hold as
//                   // long as the filter size (filter_y * filter_x * in_channel)
//                   // does not exceed 2^16, which is the case in all the models
//                   // we have seen so far.
//                   // TODO(b/174275578): Add a check to make sure the
//                   // accumulator depth is smaller than 2^16.
//                   acc += filter_val * (input_val + input_offset);
//                 }
//               }
//             }
//             if (bias_data) {
//               acc += bias_data[output_channel];
//             }
//             acc = MultiplyByQuantizedMultiplier(
//                 acc, output_multiplier[output_channel],
//                 output_shift[output_channel]);
//             acc += output_offset;
//             acc = std::max(acc, output_activation_min);
//             acc = std::min(acc, output_activation_max);
//                         const int out_offset =
//                 Offset(output_shape, batch, out_y, out_x, output_channel);

//             if (batch == 0 && out_y == 0 && out_x == 0 &&
//                 in_channel == 0 && m == 0) {
//               RawPutcD('['); RawPutcD('D'); RawPutcD('W'); RawPutcD('W'); RawPutcD(']');
//               RawTagHexD('a', static_cast<uint32_t>(acc));
//               RawTagHexD('p', static_cast<uint32_t>(
//                               reinterpret_cast<uintptr_t>(&output_data[out_offset])));
//               RawNewlineD();
//             }

//             output_data[out_offset] = static_cast<int8_t>(acc);
//             // output_data[Offset(output_shape, batch, out_y, out_x,
//             //                    output_channel)] = static_cast<int8_t>(acc);
//           }
//         }
//       }
//     }
//   }
// }

// inline void DepthwiseConvPerChannel(
//     const DepthwiseParams& params, const int32_t* output_multiplier,
//     const int32_t* output_shift, const RuntimeShape& input_shape,
//     const int8_t* input_data, const RuntimeShape& filter_shape,
//     const int8_t* filter_data, const RuntimeShape& bias_shape,
//     const int32_t* bias_data, const RuntimeShape& output_shape,
//     int8_t* output_data) {
//   // Get parameters.
//   // TODO(b/141565753): Re-introduce ScopedProfilingLabel on Micro.
//   const int stride_width = params.stride_width;
//   const int stride_height = params.stride_height;
//   const int dilation_width_factor = params.dilation_width_factor;
//   const int dilation_height_factor = params.dilation_height_factor;
//   const int pad_width = params.padding_values.width;
//   const int pad_height = params.padding_values.height;
//   const int depth_multiplier = params.depth_multiplier;
//   const int32_t input_offset = params.input_offset;
//   const int32_t output_offset = params.output_offset;
//   const int32_t output_activation_min = params.quantized_activation_min;
//   const int32_t output_activation_max = params.quantized_activation_max;

//   // Check dimensions of the tensors.
//   TFLITE_DCHECK_EQ(input_shape.DimensionsCount(), 4);
//   TFLITE_DCHECK_EQ(filter_shape.DimensionsCount(), 4);
//   TFLITE_DCHECK_EQ(output_shape.DimensionsCount(), 4);

//   TFLITE_DCHECK_LE(output_activation_min, output_activation_max);
//   const int batches = MatchingDim(input_shape, 0, output_shape, 0);
//   const int output_depth = MatchingDim(filter_shape, 3, output_shape, 3);
//   const int input_height = input_shape.Dims(1);
//   const int input_width = input_shape.Dims(2);
//   const int input_depth = input_shape.Dims(3);
//   const int filter_height = filter_shape.Dims(1);
//   const int filter_width = filter_shape.Dims(2);
//   const int output_height = output_shape.Dims(1);
//   const int output_width = output_shape.Dims(2);
//   TFLITE_DCHECK_EQ(output_depth, input_shape.Dims(3) * depth_multiplier);
//   TFLITE_DCHECK_EQ(bias_shape.FlatSize(), output_depth);

//   // RawPutcD('['); RawPutcD('D'); RawPutcD('W'); RawPutcD('S'); RawPutcD(']');
//   // RawTagHexD('b', static_cast<uint32_t>(batches));
//   // RawTagHexD('h', static_cast<uint32_t>(input_height));
//   // RawTagHexD('w', static_cast<uint32_t>(input_width));
//   // RawTagHexD('c', static_cast<uint32_t>(input_depth));
//   // RawTagHexD('f', static_cast<uint32_t>(filter_height));
//   // RawTagHexD('g', static_cast<uint32_t>(filter_width));
//   // RawTagHexD('H', static_cast<uint32_t>(output_height));
//   // RawTagHexD('W', static_cast<uint32_t>(output_width));
//   // RawTagHexD('C', static_cast<uint32_t>(output_depth));
//   // RawTagHexD('o', static_cast<uint32_t>(
//   //                    reinterpret_cast<uintptr_t>(output_data)));
//   // RawNewlineD();

//   // RawPutcD('['); RawPutcD('D'); RawPutcD('S'); RawPutcD('I'); RawPutcD(']');
//   // RawTagHexD('0', static_cast<uint32_t>(input_shape.Dims(0)));
//   // RawTagHexD('1', static_cast<uint32_t>(input_shape.Dims(1)));
//   // RawTagHexD('2', static_cast<uint32_t>(input_shape.Dims(2)));
//   // RawTagHexD('3', static_cast<uint32_t>(input_shape.Dims(3)));
//   // RawNewlineD();

//   // RawPutcD('['); RawPutcD('D'); RawPutcD('S'); RawPutcD('O'); RawPutcD(']');
//   // RawTagHexD('0', static_cast<uint32_t>(output_shape.Dims(0)));
//   // RawTagHexD('1', static_cast<uint32_t>(output_shape.Dims(1)));
//   // RawTagHexD('2', static_cast<uint32_t>(output_shape.Dims(2)));
//   // RawTagHexD('3', static_cast<uint32_t>(output_shape.Dims(3)));
//   // RawNewlineD();

//   // RawPutcD('['); RawPutcD('D'); RawPutcD('S'); RawPutcD('F'); RawPutcD(']');
//   // RawTagHexD('0', static_cast<uint32_t>(filter_shape.Dims(0)));
//   // RawTagHexD('1', static_cast<uint32_t>(filter_shape.Dims(1)));
//   // RawTagHexD('2', static_cast<uint32_t>(filter_shape.Dims(2)));
//   // RawTagHexD('3', static_cast<uint32_t>(filter_shape.Dims(3)));
//   // RawNewlineD();

//   for (int batch = 0; batch < MatchingDim(input_shape, 0, output_shape, 0);
//        ++batch) {
//     for (int out_y = 0; out_y < output_shape.Dims(1); ++out_y) {
//       for (int out_x = 0; out_x < output_shape.Dims(2); ++out_x) {
//         for (int in_channel = 0; in_channel < input_shape.Dims(3);
//              ++in_channel) {
//           for (int m = 0; m < depth_multiplier; ++m) {
//             const int output_channel = m + in_channel * depth_multiplier;
//             const int in_x_origin = (out_x * stride_width) - pad_width;
//             const int in_y_origin = (out_y * stride_height) - pad_height;
//             int32_t acc = 0;
//             for (int filter_y = 0; filter_y < filter_shape.Dims(1);
//                  ++filter_y) {
//               for (int filter_x = 0; filter_x < filter_shape.Dims(2);
//                    ++filter_x) {
//                 const int in_x = in_x_origin + dilation_width_factor * filter_x;
//                 const int in_y =
//                     in_y_origin + dilation_height_factor * filter_y;
//                 // Zero padding by omitting the areas outside the image.
//                 const bool is_point_inside_image =
//                     (in_x >= 0) && (in_x < input_shape.Dims(2)) &&
//                     (in_y >= 0) && (in_y < input_shape.Dims(1));
//                 if (is_point_inside_image) {
//                   int32_t input_val = input_data[Offset(
//                       input_shape, batch, in_y, in_x, in_channel)];
//                   int32_t filter_val = filter_data[Offset(
//                       filter_shape, 0, filter_y, filter_x, output_channel)];
//                   // Accumulate with 32 bits accumulator.
//                   // In the nudging process during model quantization, we force
//                   // real value of 0.0 be represented by a quantized value. This
//                   // guarantees that the input_offset is a int8_t, even though
//                   // it is represented using int32_t. int32_t += int8_t *
//                   // (int8_t - int8_t) so the highest value we can get from each
//                   // accumulation is [-127, 127] * ([-128, 127] -
//                   // [-128, 127]), which is [-32512, 32512]. log2(32512)
//                   // = 14.98, which means we can accumulate at least 2^16
//                   // multiplications without overflow. The accumulator is
//                   // applied to a filter so the accumulation logic will hold as
//                   // long as the filter size (filter_y * filter_x * in_channel)
//                   // does not exceed 2^16, which is the case in all the models
//                   // we have seen so far.
//                   // TODO(b/174275578): Add a check to make sure the
//                   // accumulator depth is smaller than 2^16.
//                   acc += filter_val * (input_val + input_offset);
//                 }
//               }
//             }
//             if (bias_data) {
//               acc += bias_data[output_channel];
//             }
//             acc = MultiplyByQuantizedMultiplier(
//                 acc, output_multiplier[output_channel],
//                 output_shift[output_channel]);
//             acc += output_offset;
//             acc = std::max(acc, output_activation_min);
//             acc = std::min(acc, output_activation_max);

//             const int out_offset =
//                 Offset(output_shape, batch, out_y, out_x, output_channel);

//             if (batch == 0 && out_y == 0 && out_x == 0 &&
//                 in_channel == 0 && m == 0) {
//               RawPutcD('['); RawPutcD('D'); RawPutcD('W'); RawPutcD('W'); RawPutcD(']');
//               RawTagHexD('a', static_cast<uint32_t>(acc));
//               RawTagHexD('p', static_cast<uint32_t>(
//                                   reinterpret_cast<uintptr_t>(
//                                       &output_data[out_offset])));
//               RawNewlineD();
//             }

//             output_data[out_offset] = static_cast<int8_t>(acc);

//             if (batch == 0 && out_y == 0 && out_x == 0 &&
//                 in_channel == 0 && m == 0) {
//               RawPutcD('['); RawPutcD('D'); RawPutcD('W'); RawPutcD('A'); RawPutcD(']');
//               RawTagHexD('v', static_cast<uint32_t>(
//                                   static_cast<uint8_t>(
//                                       output_data[out_offset])));
//               RawNewlineD();
//             }

//             // output_data[Offset(output_shape, batch, out_y, out_x,
//             //                    output_channel)] = static_cast<int8_t>(acc);
//           }
//         }
//       }
//     }
//   }
// }

inline void DepthwiseConvPerChannel(
    const DepthwiseParams& params, const int32_t* output_multiplier,
    const int32_t* output_shift, const RuntimeShape& input_shape,
    const int8_t* input_data, const RuntimeShape& filter_shape,
    const int8_t* filter_data, const RuntimeShape& bias_shape,
    const int32_t* bias_data, const RuntimeShape& output_shape,
    int8_t* output_data) {
  // Get parameters.
  // TODO(b/141565753): Re-introduce ScopedProfilingLabel on Micro.
  const int stride_width = params.stride_width;
  const int stride_height = params.stride_height;
  const int dilation_width_factor = params.dilation_width_factor;
  const int dilation_height_factor = params.dilation_height_factor;
  const int pad_width = params.padding_values.width;
  const int pad_height = params.padding_values.height;
  const int depth_multiplier = params.depth_multiplier;
  const int32_t input_offset = params.input_offset;
  const int32_t output_offset = params.output_offset;
  const int32_t output_activation_min = params.quantized_activation_min;
  const int32_t output_activation_max = params.quantized_activation_max;

  // Check dimensions of the tensors.
  TFLITE_DCHECK_EQ(input_shape.DimensionsCount(), 4);
  TFLITE_DCHECK_EQ(filter_shape.DimensionsCount(), 4);
  TFLITE_DCHECK_EQ(output_shape.DimensionsCount(), 4);

  TFLITE_DCHECK_LE(output_activation_min, output_activation_max);
  const int batches = MatchingDim(input_shape, 0, output_shape, 0);
  const int output_depth = MatchingDim(filter_shape, 3, output_shape, 3);
  const int input_height = input_shape.Dims(1);
  const int input_width = input_shape.Dims(2);
  const int input_depth = input_shape.Dims(3);
  const int filter_height = filter_shape.Dims(1);
  const int filter_width = filter_shape.Dims(2);
  const int output_height = output_shape.Dims(1);
  const int output_width = output_shape.Dims(2);
  TFLITE_DCHECK_EQ(output_depth, input_shape.Dims(3) * depth_multiplier);
  TFLITE_DCHECK_EQ(bias_shape.FlatSize(), output_depth);

  RawPutcD('['); RawPutcD('D'); RawPutcD('W'); RawPutcD('S'); RawPutcD(']');
  RawTagHexD('b', static_cast<uint32_t>(batches));
  RawTagHexD('h', static_cast<uint32_t>(input_height));
  RawTagHexD('w', static_cast<uint32_t>(input_width));
  RawTagHexD('c', static_cast<uint32_t>(input_depth));
  RawTagHexD('f', static_cast<uint32_t>(filter_height));
  RawTagHexD('g', static_cast<uint32_t>(filter_width));
  RawTagHexD('m', static_cast<uint32_t>(depth_multiplier));
  RawTagHexD('H', static_cast<uint32_t>(output_height));
  RawTagHexD('W', static_cast<uint32_t>(output_width));
  RawTagHexD('C', static_cast<uint32_t>(output_depth));
  RawTagHexD('o', static_cast<uint32_t>(
                     reinterpret_cast<uintptr_t>(output_data)));
  RawNewlineD();

  RawPutcD('['); RawPutcD('D'); RawPutcD('S'); RawPutcD('I'); RawPutcD(']');
  RawTagHexD('0', static_cast<uint32_t>(input_shape.Dims(0)));
  RawTagHexD('1', static_cast<uint32_t>(input_shape.Dims(1)));
  RawTagHexD('2', static_cast<uint32_t>(input_shape.Dims(2)));
  RawTagHexD('3', static_cast<uint32_t>(input_shape.Dims(3)));
  RawNewlineD();

  RawPutcD('['); RawPutcD('D'); RawPutcD('S'); RawPutcD('O'); RawPutcD(']');
  RawTagHexD('0', static_cast<uint32_t>(output_shape.Dims(0)));
  RawTagHexD('1', static_cast<uint32_t>(output_shape.Dims(1)));
  RawTagHexD('2', static_cast<uint32_t>(output_shape.Dims(2)));
  RawTagHexD('3', static_cast<uint32_t>(output_shape.Dims(3)));
  RawNewlineD();

  RawPutcD('['); RawPutcD('D'); RawPutcD('S'); RawPutcD('F'); RawPutcD(']');
  RawTagHexD('0', static_cast<uint32_t>(filter_shape.Dims(0)));
  RawTagHexD('1', static_cast<uint32_t>(filter_shape.Dims(1)));
  RawTagHexD('2', static_cast<uint32_t>(filter_shape.Dims(2)));
  RawTagHexD('3', static_cast<uint32_t>(filter_shape.Dims(3)));
  RawNewlineD();

  for (int batch = 0; batch < MatchingDim(input_shape, 0, output_shape, 0);
       ++batch) {
    for (int out_y = 0; out_y < output_shape.Dims(1); ++out_y) {
      for (int out_x = 0; out_x < output_shape.Dims(2); ++out_x) {
        for (int in_channel = 0; in_channel < input_shape.Dims(3);
             ++in_channel) {
          for (int m = 0; m < depth_multiplier; ++m) {
            const int output_channel = m + in_channel * depth_multiplier;
            const int in_x_origin = (out_x * stride_width) - pad_width;
            const int in_y_origin = (out_y * stride_height) - pad_height;
            int32_t acc = 0;

            const bool dump_dw =
                (batch == 0 && out_y == 0 && out_x == 0 &&
                 in_channel < 4 && m == 0);

            if (dump_dw) {
              RawPutcD('['); RawPutcD('D'); RawPutcD('0'); RawPutcD(']');
              RawTagHexD('c', static_cast<uint32_t>(in_channel));
              RawTagHexD('m', static_cast<uint32_t>(m));
              RawTagHexD('o', static_cast<uint32_t>(output_channel));
              RawTagHexD('x', static_cast<uint32_t>(out_x));
              RawTagHexD('y', static_cast<uint32_t>(out_y));
              RawTagHexD('a', static_cast<uint32_t>(acc));
              RawNewlineD();
            }

            for (int filter_y = 0; filter_y < filter_shape.Dims(1);
                 ++filter_y) {
              for (int filter_x = 0; filter_x < filter_shape.Dims(2);
                   ++filter_x) {
                const int in_x = in_x_origin + dilation_width_factor * filter_x;
                const int in_y =
                    in_y_origin + dilation_height_factor * filter_y;

                // Zero padding by omitting the areas outside the image.
                const bool is_point_inside_image =
                    (in_x >= 0) && (in_x < input_shape.Dims(2)) &&
                    (in_y >= 0) && (in_y < input_shape.Dims(1));

                if (is_point_inside_image) {
                  const int input_offset_index =
                      Offset(input_shape, batch, in_y, in_x, in_channel);
                  const int filter_offset_index =
                      Offset(filter_shape, 0, filter_y, filter_x,
                             output_channel);

                  int32_t input_val = input_data[input_offset_index];
                  int32_t filter_val = filter_data[filter_offset_index];

                  const int32_t input_plus_offset = input_val + input_offset;
                  const int32_t product = filter_val * input_plus_offset;

                  acc += product;

                  if (dump_dw) {
                    RawPutcD('['); RawPutcD('D'); RawPutcD('1'); RawPutcD(']');
                    RawTagHexD('c', static_cast<uint32_t>(in_channel));
                    RawTagHexD('f', static_cast<uint32_t>(filter_y));
                    RawTagHexD('g', static_cast<uint32_t>(filter_x));
                    RawTagHexD('y', static_cast<uint32_t>(in_y));
                    RawTagHexD('x', static_cast<uint32_t>(in_x));
                    RawTagHexD('I', static_cast<uint32_t>(input_offset_index));
                    RawTagHexD('F', static_cast<uint32_t>(filter_offset_index));
                    RawTagHexD('i', static_cast<uint32_t>(input_val));
                    RawTagHexD('w', static_cast<uint32_t>(filter_val));
                    RawTagHexD('z', static_cast<uint32_t>(input_offset));
                    RawTagHexD('p', static_cast<uint32_t>(product));
                    RawTagHexD('a', static_cast<uint32_t>(acc));
                    RawNewlineD();
                  }
                } else {
                  if (dump_dw) {
                    RawPutcD('['); RawPutcD('D'); RawPutcD('P'); RawPutcD(']');
                    RawTagHexD('c', static_cast<uint32_t>(in_channel));
                    RawTagHexD('f', static_cast<uint32_t>(filter_y));
                    RawTagHexD('g', static_cast<uint32_t>(filter_x));
                    RawTagHexD('y', static_cast<uint32_t>(in_y));
                    RawTagHexD('x', static_cast<uint32_t>(in_x));
                    RawNewlineD();
                  }
                }
              }
            }

            if (dump_dw) {
              RawPutcD('['); RawPutcD('D'); RawPutcD('2'); RawPutcD(']');
              RawTagHexD('c', static_cast<uint32_t>(in_channel));
              RawTagHexD('a', static_cast<uint32_t>(acc));
              RawNewlineD();
            }

            if (bias_data) {
              const int32_t bias = bias_data[output_channel];

              if (dump_dw) {
                RawPutcD('['); RawPutcD('D'); RawPutcD('3'); RawPutcD(']');
                RawTagHexD('c', static_cast<uint32_t>(in_channel));
                RawTagHexD('b', static_cast<uint32_t>(bias));
                RawTagHexD('a', static_cast<uint32_t>(acc));
                RawNewlineD();
              }

              acc += bias;

              if (dump_dw) {
                RawPutcD('['); RawPutcD('D'); RawPutcD('4'); RawPutcD(']');
                RawTagHexD('c', static_cast<uint32_t>(in_channel));
                RawTagHexD('a', static_cast<uint32_t>(acc));
                RawNewlineD();
              }
            }

            const int32_t mult = output_multiplier[output_channel];
            const int32_t shift = output_shift[output_channel];

            if (dump_dw) {
              RawPutcD('['); RawPutcD('D'); RawPutcD('5'); RawPutcD(']');
              RawTagHexD('c', static_cast<uint32_t>(in_channel));
              RawTagHexD('a', static_cast<uint32_t>(acc));
              RawTagHexD('m', static_cast<uint32_t>(mult));
              RawTagHexD('s', static_cast<uint32_t>(shift));
              RawNewlineD();
            }

            acc = MultiplyByQuantizedMultiplier(acc, mult, shift);

            if (dump_dw) {
              RawPutcD('['); RawPutcD('D'); RawPutcD('6'); RawPutcD(']');
              RawTagHexD('c', static_cast<uint32_t>(in_channel));
              RawTagHexD('a', static_cast<uint32_t>(acc));
              RawTagHexD('o', static_cast<uint32_t>(output_offset));
              RawNewlineD();
            }

            acc += output_offset;

            if (dump_dw) {
              RawPutcD('['); RawPutcD('D'); RawPutcD('7'); RawPutcD(']');
              RawTagHexD('c', static_cast<uint32_t>(in_channel));
              RawTagHexD('a', static_cast<uint32_t>(acc));
              RawTagHexD('n', static_cast<uint32_t>(output_activation_min));
              RawTagHexD('x', static_cast<uint32_t>(output_activation_max));
              RawNewlineD();
            }

            acc = std::max(acc, output_activation_min);
            acc = std::min(acc, output_activation_max);

            if (dump_dw) {
              RawPutcD('['); RawPutcD('D'); RawPutcD('8'); RawPutcD(']');
              RawTagHexD('c', static_cast<uint32_t>(in_channel));
              RawTagHexD('a', static_cast<uint32_t>(acc));
              RawNewlineD();
            }

            const int out_offset =
                Offset(output_shape, batch, out_y, out_x, output_channel);

            if (batch == 0 && out_y == 0 && out_x == 0 &&
                in_channel == 0 && m == 0) {
              RawPutcD('['); RawPutcD('D'); RawPutcD('W'); RawPutcD('W'); RawPutcD(']');
              RawTagHexD('a', static_cast<uint32_t>(acc));
              RawTagHexD('p', static_cast<uint32_t>(
                                  reinterpret_cast<uintptr_t>(
                                      &output_data[out_offset])));
              RawNewlineD();
            }

            output_data[out_offset] = static_cast<int8_t>(acc);

            if (batch == 0 && out_y == 0 && out_x == 0 &&
                in_channel == 0 && m == 0) {
              RawPutcD('['); RawPutcD('D'); RawPutcD('W'); RawPutcD('A'); RawPutcD(']');
              RawTagHexD('v', static_cast<uint32_t>(
                                  static_cast<uint8_t>(
                                      output_data[out_offset])));
              RawNewlineD();
            }
          }
        }
      }
    }
  }
}


inline void DepthwiseConvPerChannel(
    const DepthwiseParams& params, const int32_t* output_multiplier,
    const int32_t* output_shift, const RuntimeShape& input_shape,
    const int16_t* input_data, const RuntimeShape& filter_shape,
    const int8_t* filter_data, const RuntimeShape& bias_shape,
    const std::int64_t* bias_data, const RuntimeShape& output_shape,
    int16_t* output_data) {
  // Get parameters.
  const int stride_width = params.stride_width;
  const int stride_height = params.stride_height;
  const int dilation_width_factor = params.dilation_width_factor;
  const int dilation_height_factor = params.dilation_height_factor;
  const int pad_width = params.padding_values.width;
  const int pad_height = params.padding_values.height;
  const int depth_multiplier = params.depth_multiplier;
  const int32_t output_activation_min = params.quantized_activation_min;
  const int32_t output_activation_max = params.quantized_activation_max;

  // Check dimensions of the tensors.
  TFLITE_DCHECK_EQ(input_shape.DimensionsCount(), 4);
  TFLITE_DCHECK_EQ(filter_shape.DimensionsCount(), 4);
  TFLITE_DCHECK_EQ(output_shape.DimensionsCount(), 4);

  TFLITE_DCHECK_LE(output_activation_min, output_activation_max);
  const int batches = MatchingDim(input_shape, 0, output_shape, 0);
  const int output_depth = MatchingDim(filter_shape, 3, output_shape, 3);
  const int input_height = input_shape.Dims(1);
  const int input_width = input_shape.Dims(2);
  const int input_depth = input_shape.Dims(3);
  const int filter_height = filter_shape.Dims(1);
  const int filter_width = filter_shape.Dims(2);
  const int output_height = output_shape.Dims(1);
  const int output_width = output_shape.Dims(2);
  TFLITE_DCHECK_EQ(output_depth, input_depth * depth_multiplier);
  TFLITE_DCHECK_EQ(bias_shape.FlatSize(), output_depth);

  for (int batch = 0; batch < batches; ++batch) {
    for (int out_y = 0; out_y < output_height; ++out_y) {
      for (int out_x = 0; out_x < output_width; ++out_x) {
        for (int in_channel = 0; in_channel < input_depth; ++in_channel) {
          for (int m = 0; m < depth_multiplier; ++m) {
            const int output_channel = m + in_channel * depth_multiplier;
            const int in_x_origin = (out_x * stride_width) - pad_width;
            const int in_y_origin = (out_y * stride_height) - pad_height;
            std::int64_t acc = 0;
            for (int filter_y = 0; filter_y < filter_height; ++filter_y) {
              for (int filter_x = 0; filter_x < filter_width; ++filter_x) {
                const int in_x = in_x_origin + dilation_width_factor * filter_x;
                const int in_y =
                    in_y_origin + dilation_height_factor * filter_y;
                // Zero padding by omitting the areas outside the image.
                const bool is_point_inside_image =
                    (in_x >= 0) && (in_x < input_width) && (in_y >= 0) &&
                    (in_y < input_height);
                if (is_point_inside_image) {
                  int32_t input_val = input_data[Offset(
                      input_shape, batch, in_y, in_x, in_channel)];
                  int32_t filter_val = filter_data[Offset(
                      filter_shape, 0, filter_y, filter_x, output_channel)];
                  // Accumulate with 64 bits accumulator.
                  // We assume maximum of 2^16 accumulations as with the 8-bit
                  // case so actually the value in the accumulator should not
                  // exceed 40 bits
                  acc += static_cast<int64_t>(filter_val) *
                         static_cast<int64_t>(input_val);
                }
              }
            }
            if (bias_data) {
              acc += bias_data[output_channel];
            }
            int32_t scaled_acc = MultiplyByQuantizedMultiplier(
                acc, output_multiplier[output_channel],
                output_shift[output_channel]);
            scaled_acc = std::max(scaled_acc, output_activation_min);
            scaled_acc = std::min(scaled_acc, output_activation_max);
            output_data[Offset(output_shape, batch, out_y, out_x,
                               output_channel)] =
                static_cast<int16_t>(scaled_acc);
          }
        }
      }
    }
  }
}

inline void DepthwiseConvHybridPerChannel(
    const DepthwiseParams& params, float* scaling_factors_ptr,
    const RuntimeShape& input_shape, const int8_t* input_data,
    const RuntimeShape& filter_shape, const int8_t* filter_data,
    const RuntimeShape& bias_shape, const float* bias_data,
    const RuntimeShape& output_shape, float* output_data,
    const float* per_channel_scale, int32_t* input_offset) {
  const int stride_width = params.stride_width;
  const int stride_height = params.stride_height;
  const int dilation_width_factor = params.dilation_width_factor;
  const int dilation_height_factor = params.dilation_height_factor;
  const int pad_width = params.padding_values.width;
  const int pad_height = params.padding_values.height;
  const int depth_multiplier = params.depth_multiplier;
  const float output_activation_min = params.float_activation_min;
  const float output_activation_max = params.float_activation_max;
  // Check dimensions of the tensors.
  TFLITE_DCHECK_EQ(input_shape.DimensionsCount(), 4);
  TFLITE_DCHECK_EQ(filter_shape.DimensionsCount(), 4);
  TFLITE_DCHECK_EQ(output_shape.DimensionsCount(), 4);

  const int batches = MatchingDim(input_shape, 0, output_shape, 0);
  const int output_depth = MatchingDim(filter_shape, 3, output_shape, 3);
  const int input_height = input_shape.Dims(1);
  const int input_width = input_shape.Dims(2);
  const int input_depth = input_shape.Dims(3);
  const int filter_height = filter_shape.Dims(1);
  const int filter_width = filter_shape.Dims(2);
  const int output_height = output_shape.Dims(1);
  const int output_width = output_shape.Dims(2);
  const int bias_depth = bias_shape.FlatSize();
  TFLITE_DCHECK_EQ(output_depth, input_depth * depth_multiplier);
  TFLITE_DCHECK_EQ(bias_depth, output_depth);

  for (int batch = 0; batch < batches; ++batch) {
    for (int out_y = 0; out_y < output_height; ++out_y) {
      for (int out_x = 0; out_x < output_width; ++out_x) {
        for (int in_channel = 0; in_channel < input_depth; ++in_channel) {
          for (int m = 0; m < depth_multiplier; ++m) {
            const int output_channel = m + in_channel * depth_multiplier;
            const int in_x_origin = (out_x * stride_width) - pad_width;
            const int in_y_origin = (out_y * stride_height) - pad_height;
            int32_t acc = 0;
            for (int filter_y = 0; filter_y < filter_height; ++filter_y) {
              for (int filter_x = 0; filter_x < filter_width; ++filter_x) {
                const int in_x = in_x_origin + dilation_width_factor * filter_x;
                const int in_y =
                    in_y_origin + dilation_height_factor * filter_y;
                // Zero padding by omitting the areas outside the image.
                const bool is_point_inside_image =
                    (in_x >= 0) && (in_x < input_width) && (in_y >= 0) &&
                    (in_y < input_height);
                if (is_point_inside_image) {
                  int32_t input_val = input_data[Offset(
                      input_shape, batch, in_y, in_x, in_channel)];
                  int32_t filter_val = filter_data[Offset(
                      filter_shape, 0, filter_y, filter_x, output_channel)];
                  acc += filter_val * (input_val - input_offset[batch]);
                }
              }
            }
            float acc_float = static_cast<float>(acc);
            acc_float *=
                per_channel_scale[output_channel] * scaling_factors_ptr[batch];
            if (bias_data && output_channel < bias_depth) {
              acc_float += bias_data[output_channel];
            }
            output_data[Offset(output_shape, batch, out_y, out_x,
                               output_channel)] =
                ActivationFunctionWithMinMax(acc_float, output_activation_min,
                                             output_activation_max);
          }
        }
      }
    }
  }
}

}  // namespace reference_integer_ops
}  // namespace tflite

#endif  // TENSORFLOW_LITE_KERNELS_INTERNAL_REFERENCE_INTEGER_OPS_DEPTHWISE_CONV_H_
