/* Copyright 2023 The TensorFlow Authors. All Rights Reserved.

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

#include "tensorflow/lite/micro/micro_op_resolver.h"

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/schema/schema_utils.h"


#include <stdint.h>

static inline void RawPutc(char c) {
  volatile uint32_t* const uart_tx =
      reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
  volatile uint32_t* const uart_status =
      reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x08u);

  while ((*uart_status) & 0x08u) {
  }

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

// namespace tflite {

// TfLiteStatus GetRegistrationFromOpCode(const OperatorCode* opcode,
//                                        const MicroOpResolver& op_resolver,
//                                        const TFLMRegistration** registration) {
//   TfLiteStatus status = kTfLiteOk;
//   *registration = nullptr;
//   auto builtin_code = GetBuiltinCode(opcode);

//   if (builtin_code > BuiltinOperator_MAX) {
//     MicroPrintf("Op builtin_code out of range: %d.", builtin_code);
//     status = kTfLiteError;
//   } else if (builtin_code != BuiltinOperator_CUSTOM) {
//     *registration = op_resolver.FindOp(builtin_code);
//     if (*registration == nullptr) {
//       MicroPrintf("Didn't find op for builtin opcode '%s'",
//                   EnumNameBuiltinOperator(builtin_code));
//       status = kTfLiteError;
//     }
//   } else if (!opcode->custom_code()) {
//     MicroPrintf("Operator with CUSTOM builtin_code has no custom_code.\n");
//     status = kTfLiteError;
//   } else {
//     const char* name = opcode->custom_code()->c_str();
//     *registration = op_resolver.FindOp(name);
//     if (*registration == nullptr) {
//       // Do not report error for unresolved custom op, we do the final check
//       // while preparing ops.
//       status = kTfLiteError;
//     }
//   }
//   return status;
// }
// }  // namespace tflite


namespace tflite {

__attribute__((noinline))
TfLiteStatus GetRegistrationFromOpCode(
    const OperatorCode* opcode,
    const MicroOpResolver& op_resolver,
    const TFLMRegistration** registration) {
  RawPutc('['); RawPutc('R'); RawPutc('O'); RawPutc('0'); RawPutc(']');
  RawTagHex('c', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(opcode)));
  RawTagHex('r', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(registration)));
  RawNewline();

  TfLiteStatus status = kTfLiteOk;
  *registration = nullptr;

  RawPutc('['); RawPutc('R'); RawPutc('O'); RawPutc('1'); RawPutc(']');
  RawNewline();

  auto builtin_code = GetBuiltinCode(opcode);

  RawPutc('['); RawPutc('R'); RawPutc('O'); RawPutc('2'); RawPutc(']');
  RawTagHex('b', static_cast<uint32_t>(builtin_code));
  RawNewline();

  if (builtin_code > BuiltinOperator_MAX) {
    RawPutc('['); RawPutc('R'); RawPutc('O'); RawPutc('X'); RawPutc(']');
    RawTagHex('b', static_cast<uint32_t>(builtin_code));
    RawNewline();
    status = kTfLiteError;
  } else if (builtin_code != BuiltinOperator_CUSTOM) {
    RawPutc('['); RawPutc('R'); RawPutc('O'); RawPutc('3'); RawPutc(']');
    RawNewline();

    *registration = op_resolver.FindOp(builtin_code);

    RawPutc('['); RawPutc('R'); RawPutc('O'); RawPutc('4'); RawPutc(']');
    RawTagHex('p', static_cast<uint32_t>(
                       reinterpret_cast<uintptr_t>(*registration)));
    RawNewline();

    if (*registration == nullptr) {
      RawPutc('['); RawPutc('R'); RawPutc('O'); RawPutc('Y'); RawPutc(']');
      RawNewline();
      status = kTfLiteError;
    }
  } else if (!opcode->custom_code()) {
    RawPutc('['); RawPutc('R'); RawPutc('O'); RawPutc('Z'); RawPutc(']');
    RawNewline();
    status = kTfLiteError;
  } else {
    RawPutc('['); RawPutc('R'); RawPutc('O'); RawPutc('5'); RawPutc(']');
    RawNewline();

    const char* name = opcode->custom_code()->c_str();

    RawPutc('['); RawPutc('R'); RawPutc('O'); RawPutc('6'); RawPutc(']');
    RawTagHex('n', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(name)));
    RawNewline();

    *registration = op_resolver.FindOp(name);

    RawPutc('['); RawPutc('R'); RawPutc('O'); RawPutc('7'); RawPutc(']');
    RawTagHex('p', static_cast<uint32_t>(
                       reinterpret_cast<uintptr_t>(*registration)));
    RawNewline();

    if (*registration == nullptr) {
      status = kTfLiteError;
    }
  }

  RawPutc('['); RawPutc('R'); RawPutc('O'); RawPutc('8'); RawPutc(']');
  RawTagHex('s', static_cast<uint32_t>(status));
  RawNewline();

  return status;
}

}