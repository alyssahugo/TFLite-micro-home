/* Copyright 2021 The TensorFlow Authors. All Rights Reserved.

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
#include "tensorflow/lite/micro/tflite_bridge/flatbuffer_conversions_bridge.h"

#include "tensorflow/lite/c/c_api_types.h"
#include "tensorflow/lite/core/api/error_reporter.h"
#include "tensorflow/lite/core/api/flatbuffer_conversions.h"
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"
#include "tensorflow/lite/schema/schema_generated.h"


#include <stdint.h>

static inline void RawPutc(char c) {
  volatile uint32_t* const uart_tx =
      reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
  volatile uint32_t* const uart_status =
      reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x08u);

  while ((*uart_status) & 0x08u) {}

  *uart_tx = static_cast<uint32_t>(static_cast<uint8_t>(c));
}

static inline void RawNewline() {
  RawPutc('\r');
  RawPutc('\n');
}

static inline void RawPutHexNibble(uint32_t v) {
  v &= 0xFu;
  RawPutc((v < 10u) ? static_cast<char>('0' + v)
                    : static_cast<char>('A' + (v - 10u)));
}

static inline void RawPutHex32(uint32_t v) {
  for (int shift = 28; shift >= 0; shift -= 4) {
    RawPutHexNibble(v >> shift);
  }
}

static inline void RawTagHex(char tag, uint32_t v) {
  RawPutc(tag);
  RawPutHex32(v);
  RawPutc(' ');
}

namespace tflite {
TfLiteStatus ConvertTensorType(TensorType tensor_type, TfLiteType* type) {
  return ConvertTensorType(tensor_type, type, tflite::GetMicroErrorReporter());
}

// TfLiteStatus CallBuiltinParseFunction(TfLiteBridgeBuiltinParseFunction parser,
//                                       const Operator* op,
//                                       BuiltinDataAllocator* allocator,
//                                       void** builtin_data) {
//   return parser(op, tflite::GetMicroErrorReporter(), allocator, builtin_data);
// }

__attribute__((noinline))
TfLiteStatus CallBuiltinParseFunction(TfLiteBridgeBuiltinParseFunction parser,
                                      const Operator* op,
                                      BuiltinDataAllocator* allocator,
                                      void** builtin_data) {
  RawPutc('['); RawPutc('B'); RawPutc('C'); RawPutc('0'); RawPutc(']');
  RawTagHex('p', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(parser)));
  RawTagHex('o', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(op)));
  RawTagHex('a', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(allocator)));
  RawTagHex('d', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(builtin_data)));
  RawNewline();

  ErrorReporter* reporter = tflite::GetMicroErrorReporter();

  RawPutc('['); RawPutc('B'); RawPutc('C'); RawPutc('1'); RawPutc(']');
  RawTagHex('e', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(reporter)));
  RawNewline();

  TfLiteStatus status = parser(op, reporter, allocator, builtin_data);

  RawPutc('['); RawPutc('B'); RawPutc('C'); RawPutc('2'); RawPutc(']');
  RawTagHex('s', static_cast<uint32_t>(status));
  RawTagHex('b', static_cast<uint32_t>(
                     reinterpret_cast<uintptr_t>(
                         builtin_data ? *builtin_data : nullptr)));
  RawNewline();

  return status;
}

}  // namespace tflite
