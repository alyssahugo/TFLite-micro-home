/* Copyright 2025 The TensorFlow Authors. All Rights Reserved.

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

#include "tensorflow/lite/micro/micro_interpreter_graph.h"

#include <algorithm>

#include "flatbuffers/flatbuffers.h"  // from @flatbuffers
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/kernels/internal/compatibility.h"
#include "tensorflow/lite/micro/flatbuffer_utils.h"
#include "tensorflow/lite/micro/memory_helpers.h"
#include "tensorflow/lite/micro/micro_context.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_profiler.h"
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

#ifdef USE_TFLM_COMPRESSION

#include "tensorflow/lite/micro/micro_context.h"

#endif  // USE_TFLM_COMPRESSION



namespace tflite {
namespace {

const char* OpNameFromRegistration(const TFLMRegistration* registration) {
  if (registration->builtin_code == BuiltinOperator_CUSTOM) {
    return registration->custom_name;
  } else {
    return EnumNameBuiltinOperator(BuiltinOperator(registration->builtin_code));
  }
}

// Check tensor shapes to determine if there are dynamic tensors present.
// Returns the index of the first dynamic tensor found, otherwise returns -1.
int CheckDynamicTensors(const TfLiteIntArray* const tensor_indices,
                        const TfLiteEvalTensor* const eval_tensors) {
  // some operators have no tensors, so node->inputs and/or node->outputs
  // can be <nullptr>.  This occurs in the MicroInterpreter unit tests.
  if (tensor_indices == nullptr) {
    return -1;
  }

  for (int i = 0; i < tensor_indices->size; i++) {
    const int tensor_index = tensor_indices->data[i];
    // Skip optional tensors
    if (tensor_index < 0) {
      continue;
    }

    // Check shape for dims <= 0.
    const TfLiteEvalTensor* const tp = eval_tensors + tensor_index;
    if (tp->dims->size == 1 && tp->dims->data[0] == 0) {
      // Legacy scalar shapes (dims->size == 1 && dims->data[0] == 0)
      continue;
    }
    // This code can handle scalar tensors (dims->size == 0)
    if (!std::all_of(tp->dims->data, tp->dims->data + tp->dims->size,
                     [](int dim) { return dim > 0; })) {
      return tensor_index;
    }
  }

  return -1;
}

}  // namespace

MicroInterpreterGraph::MicroInterpreterGraph(
    TfLiteContext* context, const Model* model, MicroAllocator* allocator,
    MicroResourceVariables* resource_variables)
    : context_(context),
      model_(model),
      allocator_(allocator),
      current_subgraph_index_(0),
      current_operator_index_(0),
      resource_variables_(resource_variables) {
  if (model != nullptr) {
    subgraphs_ = model->subgraphs();
  }
}

MicroInterpreterGraph::~MicroInterpreterGraph() {}

TfLiteStatus MicroInterpreterGraph::InitSubgraphs() {
  int previous_subgraph_idx = current_subgraph_index_;
  uint32_t previous_operator_idx = current_operator_index_;

  for (size_t subgraph_idx = 0; subgraph_idx < subgraphs_->size();
       subgraph_idx++) {
    current_subgraph_index_ = subgraph_idx;
    uint32_t operators_size = NumSubgraphOperators(model_, subgraph_idx);

    // RawPutc('['); RawPutc('I'); RawPutc('G'); RawPutc('0'); RawPutc(']');
    // RawTagHex('g', static_cast<uint32_t>(subgraph_idx));
    // RawNewline();

    // const SubGraph* subgraph = subgraphs_->Get(subgraph_idx);

    // RawPutc('['); RawPutc('I'); RawPutc('G'); RawPutc('1'); RawPutc(']');
    // RawTagHex('s', static_cast<uint32_t>(
    //                   reinterpret_cast<uintptr_t>(subgraph)));
    // RawNewline();

    // if (subgraph == nullptr || subgraph->operators() == nullptr) {
    //   RawPutc('['); RawPutc('I'); RawPutc('G'); RawPutc('X'); RawPutc(']');
    //   RawNewline();
    //   return kTfLiteError;
    // }

    // uint32_t operators_size =
    //     static_cast<uint32_t>(subgraph->operators()->size());

    // RawPutc('['); RawPutc('I'); RawPutc('G'); RawPutc('2'); RawPutc(']');
    // RawTagHex('n', static_cast<uint32_t>(operators_size));
    // RawNewline();
    for (current_operator_index_ = 0; current_operator_index_ < operators_size;
         ++current_operator_index_) {
      TfLiteNode* node = &(subgraph_allocations_[subgraph_idx]
                               .node_and_registrations[current_operator_index_]
                               .node);
      const TFLMRegistration* registration =
          subgraph_allocations_[subgraph_idx]
              .node_and_registrations[current_operator_index_]
              .registration;
      size_t init_data_size;
      const char* init_data;
      if (registration->builtin_code == BuiltinOperator_CUSTOM) {
        init_data = reinterpret_cast<const char*>(node->custom_initial_data);
        init_data_size = node->custom_initial_data_size;
      } else {
        init_data = reinterpret_cast<const char*>(node->builtin_data);
        init_data_size = 0;
      }
      // if (registration->init) {
      //   node->user_data =
      //       registration->init(context_, init_data, init_data_size);
      // }

      RawPutc('['); RawPutc('I'); RawPutc('N'); RawPutc('0'); RawPutc(']');
      RawTagHex('g', static_cast<uint32_t>(subgraph_idx));
      RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
      RawTagHex('b', static_cast<uint32_t>(registration->builtin_code));
      RawTagHex('f', static_cast<uint32_t>(
                        reinterpret_cast<uintptr_t>(registration->init)));
      RawNewline();

      if (registration->init) {
        RawPutc('['); RawPutc('I'); RawPutc('N'); RawPutc('1'); RawPutc(']');
        RawNewline();

        node->user_data =
            registration->init(context_, init_data, init_data_size);

        RawPutc('['); RawPutc('I'); RawPutc('N'); RawPutc('2'); RawPutc(']');
        RawTagHex('u', static_cast<uint32_t>(
                          reinterpret_cast<uintptr_t>(node->user_data)));
        RawNewline();
      } else {
        RawPutc('['); RawPutc('I'); RawPutc('N'); RawPutc('N'); RawPutc(']');
        RawNewline();
      }
    }
  }
  current_subgraph_index_ = previous_subgraph_idx;
  current_operator_index_ = previous_operator_idx;

  return kTfLiteOk;
}

// TfLiteStatus MicroInterpreterGraph::PrepareSubgraphs() {
//   int previous_subgraph_idx = current_subgraph_index_;
//   uint32_t previous_operator_idx = current_operator_index_;
//   for (size_t subgraph_idx = 0; subgraph_idx < subgraphs_->size();
//        subgraph_idx++) {
//     current_subgraph_index_ = subgraph_idx;
//     uint32_t operators_size = NumSubgraphOperators(model_, subgraph_idx);
//     for (current_operator_index_ = 0; current_operator_index_ < operators_size;
//          ++current_operator_index_) {
//       TfLiteNode* node = &(subgraph_allocations_[subgraph_idx]
//                                .node_and_registrations[current_operator_index_]
//                                .node);
//       const TFLMRegistration* registration =
//           subgraph_allocations_[subgraph_idx]
//               .node_and_registrations[current_operator_index_]
//               .registration;
//       if (registration->prepare != nullptr) {
//         TfLiteStatus prepare_status = registration->prepare(context_, node);
//         if (prepare_status != kTfLiteOk) {
//           MicroPrintf("Node %s (number %u) failed to prepare with status %d",
//                       OpNameFromRegistration(registration),
//                       current_operator_index_, prepare_status);
//           return kTfLiteError;
//         }
// #ifdef USE_TFLM_COMPRESSION
//         GetMicroContext(context_)->ResetDecompressionMemoryAllocations();
// #endif  // USE_TFLM_COMPRESSION
//       }

//       const int dynamic_tensor_index = CheckDynamicTensors(
//           node->outputs, subgraph_allocations_[subgraph_idx].tensors);
//       if (dynamic_tensor_index != -1) {
//         MicroPrintf(
//             "Op#%u (%s) of subgraph %u has dynamic tensor #%d\n"
//             "Dynamic tensors are not supported",
//             current_operator_index_, OpNameFromRegistration(registration),
//             current_subgraph_index_, dynamic_tensor_index);
//         return kTfLiteError;
//       }

//       allocator_->FinishPrepareNodeAllocations(
//           /*node_id=*/current_operator_index_);
//     }
//   }
//   current_subgraph_index_ = previous_subgraph_idx;
//   current_operator_index_ = previous_operator_idx;
//   return kTfLiteOk;
// }

TfLiteStatus MicroInterpreterGraph::PrepareSubgraphs() {
  RawPutc('['); RawPutc('G'); RawPutc('P'); RawPutc('E'); RawPutc('0'); RawPutc(']');
  RawNewline();

  int previous_subgraph_idx = current_subgraph_index_;
  uint32_t previous_operator_idx = current_operator_index_;

  for (size_t subgraph_idx = 0; subgraph_idx < subgraphs_->size();
       subgraph_idx++) {
    current_subgraph_index_ = subgraph_idx;

    const SubGraph* subgraph = subgraphs_->Get(subgraph_idx);
    if (subgraph == nullptr || subgraph->operators() == nullptr) {
      RawPutc('['); RawPutc('G'); RawPutc('P'); RawPutc('E'); RawPutc('X'); RawPutc(']');
      RawNewline();
      return kTfLiteError;
    }

    uint32_t operators_size =
        static_cast<uint32_t>(subgraph->operators()->size());

    RawPutc('['); RawPutc('G'); RawPutc('P'); RawPutc('E'); RawPutc('1'); RawPutc(']');
    RawTagHex('n', static_cast<uint32_t>(operators_size));
    RawNewline();

    for (current_operator_index_ = 0; current_operator_index_ < operators_size;
         ++current_operator_index_) {
      TfLiteNode* node =
          &(subgraph_allocations_[subgraph_idx]
                .node_and_registrations[current_operator_index_]
                .node);

      const TFLMRegistration* registration =
          subgraph_allocations_[subgraph_idx]
              .node_and_registrations[current_operator_index_]
              .registration;

      RawPutc('['); RawPutc('P'); RawPutc('R'); RawPutc('0'); RawPutc(']');
      RawTagHex('g', static_cast<uint32_t>(subgraph_idx));
      RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
      RawTagHex('b', static_cast<uint32_t>(registration->builtin_code));
      RawTagHex('f', static_cast<uint32_t>(
                         reinterpret_cast<uintptr_t>(registration->prepare)));
      RawNewline();

      if (registration->prepare != nullptr) {
        RawPutc('['); RawPutc('P'); RawPutc('R'); RawPutc('1'); RawPutc(']');
        RawNewline();

        TfLiteStatus prepare_status =
            registration->prepare(context_, node);

        RawPutc('['); RawPutc('P'); RawPutc('R'); RawPutc('2'); RawPutc(']');
        RawTagHex('s', static_cast<uint32_t>(prepare_status));
        RawNewline();

        if (prepare_status != kTfLiteOk) {
          MicroPrintf("Node %s (number %u) failed to prepare with status %d",
                      OpNameFromRegistration(registration),
                      current_operator_index_, prepare_status);
          return kTfLiteError;
        }
      } else {
        RawPutc('['); RawPutc('P'); RawPutc('R'); RawPutc('N'); RawPutc(']');
        RawNewline();
      }

      RawPutc('['); RawPutc('D'); RawPutc('Y'); RawPutc('0'); RawPutc(']');
      RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
      RawNewline();

      const int dynamic_tensor_index = CheckDynamicTensors(
          node->outputs, subgraph_allocations_[subgraph_idx].tensors);

      RawPutc('['); RawPutc('D'); RawPutc('Y'); RawPutc('1'); RawPutc(']');
      RawTagHex('d', static_cast<uint32_t>(dynamic_tensor_index));
      RawNewline();

      if (dynamic_tensor_index != -1) {
        MicroPrintf(
            "Op#%u (%s) of subgraph %u has dynamic tensor #%d\n"
            "Dynamic tensors are not supported",
            current_operator_index_, OpNameFromRegistration(registration),
            current_subgraph_index_, dynamic_tensor_index);
        return kTfLiteError;
      }

      RawPutc('['); RawPutc('F'); RawPutc('P'); RawPutc('0'); RawPutc(']');
      RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
      RawNewline();

      allocator_->FinishPrepareNodeAllocations(
          /*node_id=*/current_operator_index_);

      RawPutc('['); RawPutc('F'); RawPutc('P'); RawPutc('1'); RawPutc(']');
      RawNewline();
    }
  }

  current_subgraph_index_ = previous_subgraph_idx;
  current_operator_index_ = previous_operator_idx;

  RawPutc('['); RawPutc('G'); RawPutc('P'); RawPutc('E'); RawPutc('2'); RawPutc(']');
  RawNewline();

  return kTfLiteOk;
}

TfLiteStatus MicroInterpreterGraph::ResetSubgraphs() {
  int previous_subgraph_idx = current_subgraph_index_;
  uint32_t previous_operator_idx = current_operator_index_;

  for (size_t subgraph_idx = 0; subgraph_idx < subgraphs_->size();
       subgraph_idx++) {
    current_subgraph_index_ = subgraph_idx;
    uint32_t operators_size = NumSubgraphOperators(model_, subgraph_idx);
    for (current_operator_index_ = 0; current_operator_index_ < operators_size;
         ++current_operator_index_) {
      TfLiteNode* node = &(subgraph_allocations_[subgraph_idx]
                               .node_and_registrations[current_operator_index_]
                               .node);
      const TFLMRegistration* registration =
          subgraph_allocations_[subgraph_idx]
              .node_and_registrations[current_operator_index_]
              .registration;
      // registration is allocated outside the interpreter, so double check to
      // make sure it's not nullptr;
      if (registration != nullptr && registration->reset != nullptr) {
        registration->reset(context_, node->user_data);
      }
    }
  }
  current_subgraph_index_ = previous_subgraph_idx;
  current_operator_index_ = previous_operator_idx;

  return kTfLiteOk;
}

TfLiteStatus MicroInterpreterGraph::FreeSubgraphs() {
  int previous_subgraph_idx = current_subgraph_index_;
  uint32_t previous_operator_idx = current_operator_index_;

  for (size_t subgraph_idx = 0; subgraph_idx < subgraphs_->size();
       subgraph_idx++) {
    current_subgraph_index_ = subgraph_idx;
    uint32_t operators_size = NumSubgraphOperators(model_, subgraph_idx);
    for (current_operator_index_ = 0; current_operator_index_ < operators_size;
         ++current_operator_index_) {
      TfLiteNode* node = &(subgraph_allocations_[subgraph_idx]
                               .node_and_registrations[current_operator_index_]
                               .node);
      const TFLMRegistration* registration =
          subgraph_allocations_[subgraph_idx]
              .node_and_registrations[current_operator_index_]
              .registration;
      // registration is allocated outside the interpreter, so double check to
      // make sure it's not nullptr;
      if (registration != nullptr && registration->free != nullptr) {
        registration->free(context_, node->user_data);
      }
    }
  }
  current_subgraph_index_ = previous_subgraph_idx;
  current_operator_index_ = previous_operator_idx;

  return kTfLiteOk;
}

// TfLiteStatus MicroInterpreterGraph::InvokeSubgraph(int subgraph_idx) {
//   int previous_subgraph_idx = current_subgraph_index_;
//   uint32_t previous_operator_idx = current_operator_index_;
//   current_subgraph_index_ = subgraph_idx;

//   if (static_cast<size_t>(subgraph_idx) >= subgraphs_->size()) {
//     MicroPrintf("Accessing subgraph %d but only %d subgraphs found",
//                 subgraph_idx, subgraphs_->size());
//     return kTfLiteError;
//   }
//   TfLiteStatus invoke_status = kTfLiteOk;
//   uint32_t operators_size = NumSubgraphOperators(model_, subgraph_idx);
//   for (current_operator_index_ = 0; current_operator_index_ < operators_size;
//        ++current_operator_index_) {
//     TfLiteNode* node = &(subgraph_allocations_[subgraph_idx]
//                              .node_and_registrations[current_operator_index_]
//                              .node);
//     const TFLMRegistration* registration =
//         subgraph_allocations_[subgraph_idx]
//             .node_and_registrations[current_operator_index_]
//             .registration;

// // This ifdef is needed (even though ScopedMicroProfiler itself is a no-op with
// // -DTF_LITE_STRIP_ERROR_STRINGS) because the function OpNameFromRegistration is
// // only defined for builds with the error strings.
// #if !defined(TF_LITE_STRIP_ERROR_STRINGS)
//     ScopedMicroProfiler scoped_profiler(
//         OpNameFromRegistration(registration),
//         reinterpret_cast<MicroProfilerInterface*>(context_->profiler));
// #endif

//     TFLITE_DCHECK(registration->invoke);
//     invoke_status = registration->invoke(context_, node);
// #ifdef USE_TFLM_COMPRESSION
//     GetMicroContext(context_)->ResetDecompressionMemoryAllocations();
// #endif  // USE_TFLM_COMPRESSION

//     // All TfLiteTensor structs used in the kernel are allocated from temp
//     // memory in the allocator. This creates a chain of allocations in the
//     // temp section. The call below resets the chain of allocations to
//     // prepare for the next call.
//     allocator_->ResetTempAllocations();

//     if (invoke_status != kTfLiteOk) {
//       if (invoke_status != kTfLiteAbort) {
//         MicroPrintf("Node %s (number %d) failed to invoke with status %d",
//                     OpNameFromRegistration(registration),
//                     current_operator_index_, invoke_status);
//       }
//       // make sure to restore subgraph and operator indices
//       break;
//     }
//   }

//   current_subgraph_index_ = previous_subgraph_idx;
//   current_operator_index_ = previous_operator_idx;

//   return invoke_status;
// }

static inline void RawDumpEvalTensorBytes(const char* label,
                                          uint32_t op_index,
                                          int tensor_index,
                                          TfLiteEvalTensor* tensor,
                                          int max_bytes) {
  RawPutc('[');
  while (*label) {
    RawPutc(*label++);
  }
  RawPutc(']');

  RawTagHex('i', static_cast<uint32_t>(op_index));
  RawTagHex('t', static_cast<uint32_t>(tensor_index));

  if (tensor == nullptr) {
    RawPutc('N');
    RawNewline();
    return;
  }

  RawTagHex('p', static_cast<uint32_t>(
                     reinterpret_cast<uintptr_t>(tensor->data.data)));
  RawTagHex('y', static_cast<uint32_t>(tensor->type));
  RawNewline();

  if (tensor->data.data == nullptr) {
    RawPutc('['); RawPutc('T'); RawPutc('Z'); RawPutc(']');
    RawNewline();
    return;
  }

  RawPutc('['); RawPutc('T'); RawPutc('B'); RawPutc(']');

  const uint8_t* bytes =
      reinterpret_cast<const uint8_t*>(tensor->data.data);

  for (int k = 0; k < max_bytes; ++k) {
    RawTagHex('x', static_cast<uint32_t>(bytes[k]));
  }

  RawNewline();
}

// TfLiteStatus MicroInterpreterGraph::InvokeSubgraph(int subgraph_idx) {
//   RawPutc('['); RawPutc('I'); RawPutc('S'); RawPutc('0'); RawPutc(']');
//   RawTagHex('s', static_cast<uint32_t>(subgraph_idx));
//   RawTagHex('c', static_cast<uint32_t>(current_subgraph_index_));
//   RawTagHex('o', static_cast<uint32_t>(current_operator_index_));
//   RawNewline();

//   int previous_subgraph_idx = current_subgraph_index_;
//   uint32_t previous_operator_idx = current_operator_index_;
//   current_subgraph_index_ = subgraph_idx;

//   RawPutc('['); RawPutc('I'); RawPutc('S'); RawPutc('1'); RawPutc(']');
//   RawTagHex('p', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(subgraphs_)));
//   if (subgraphs_ != nullptr) {
//     RawTagHex('n', static_cast<uint32_t>(subgraphs_->size()));
//   }
//   RawNewline();

//   if (static_cast<size_t>(subgraph_idx) >= subgraphs_->size()) {
//     MicroPrintf("Accessing subgraph %d but only %d subgraphs found",
//                 subgraph_idx, subgraphs_->size());
//     return kTfLiteError;
//   }

//   TfLiteStatus invoke_status = kTfLiteOk;
//   uint32_t operators_size = NumSubgraphOperators(model_, subgraph_idx);

//   RawPutc('['); RawPutc('I'); RawPutc('S'); RawPutc('2'); RawPutc(']');
//   RawTagHex('m', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(model_)));
//   RawTagHex('z', static_cast<uint32_t>(operators_size));
//   RawNewline();

//   for (current_operator_index_ = 0; current_operator_index_ < operators_size;
//        ++current_operator_index_) {

//     RawPutc('['); RawPutc('O'); RawPutc('P'); RawPutc('0'); RawPutc(']');
//     RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
//     RawNewline();

//     TfLiteNode* node = &(subgraph_allocations_[subgraph_idx]
//                              .node_and_registrations[current_operator_index_]
//                              .node);

//     const TFLMRegistration* registration =
//         subgraph_allocations_[subgraph_idx]
//             .node_and_registrations[current_operator_index_]
//             .registration;

//     RawPutc('['); RawPutc('O'); RawPutc('P'); RawPutc('1'); RawPutc(']');
//     RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
//     RawTagHex('n', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(node)));
//     RawTagHex('r', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(registration)));
//     RawNewline();

//     if (registration == nullptr) {
//       RawPutc('['); RawPutc('O'); RawPutc('P'); RawPutc('E'); RawPutc(']');
//       RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
//       RawNewline();
//       invoke_status = kTfLiteError;
//       break;
//     }

//     RawPutc('['); RawPutc('O'); RawPutc('P'); RawPutc('2'); RawPutc(']');
//     RawTagHex('b', static_cast<uint32_t>(registration->builtin_code));
//     RawTagHex('v', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(registration->invoke)));
//     RawTagHex('d', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(registration->init)));
//     RawTagHex('f', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(registration->free)));
//     RawNewline();

//     RawPutc('['); RawPutc('N'); RawPutc('D'); RawPutc('0'); RawPutc(']');
//     RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
//     RawTagHex('u', static_cast<uint32_t>(node->inputs ? node->inputs->size : 0xFFFFFFFFu));
//     RawTagHex('o', static_cast<uint32_t>(node->outputs ? node->outputs->size : 0xFFFFFFFFu));
//     RawTagHex('b', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(node->builtin_data)));
//     RawTagHex('a', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(node->user_data)));
//     RawNewline();

//     if (node->inputs != nullptr) {
//       RawPutc('['); RawPutc('N'); RawPutc('D'); RawPutc('1'); RawPutc(']');
//       for (int k = 0; k < node->inputs->size && k < 4; ++k) {
//         RawTagHex('x', static_cast<uint32_t>(node->inputs->data[k]));
//       }
//       RawNewline();
//     }

//     if (node->outputs != nullptr) {
//       RawPutc('['); RawPutc('N'); RawPutc('D'); RawPutc('2'); RawPutc(']');
//       for (int k = 0; k < node->outputs->size && k < 4; ++k) {
//         RawTagHex('y', static_cast<uint32_t>(node->outputs->data[k]));
//       }
//       RawNewline();
//     }

// #if !defined(TF_LITE_STRIP_ERROR_STRINGS)
//     ScopedMicroProfiler scoped_profiler(
//         OpNameFromRegistration(registration),
//         reinterpret_cast<MicroProfilerInterface*>(context_->profiler));
// #endif

//     TFLITE_DCHECK(registration->invoke);

//     RawPutc('['); RawPutc('O'); RawPutc('P'); RawPutc('3'); RawPutc(']');
//     RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
//     RawNewline();

//     invoke_status = registration->invoke(context_, node);

//     RawPutc('['); RawPutc('O'); RawPutc('P'); RawPutc('4'); RawPutc(']');
//     RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
//     RawTagHex('s', static_cast<uint32_t>(invoke_status));
//     RawNewline();

// #ifdef USE_TFLM_COMPRESSION
//     GetMicroContext(context_)->ResetDecompressionMemoryAllocations();
// #endif

//     RawPutc('['); RawPutc('R'); RawPutc('T'); RawPutc('0'); RawPutc(']');
//     RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
//     RawNewline();

//     TfLiteStatus reset_status = allocator_->ResetTempAllocations();

//     RawPutc('['); RawPutc('R'); RawPutc('T'); RawPutc('1'); RawPutc(']');
//     RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
//     RawTagHex('s', static_cast<uint32_t>(reset_status));
//     RawNewline();

//     if (reset_status != kTfLiteOk) {
//       invoke_status = reset_status;
//       break;
//     }

//     if (invoke_status != kTfLiteOk) {
//       if (invoke_status != kTfLiteAbort) {
//         MicroPrintf("Node %s (number %d) failed to invoke with status %d",
//                     OpNameFromRegistration(registration),
//                     current_operator_index_, invoke_status);
//       }
//       break;
//     }
//   }

//   RawPutc('['); RawPutc('I'); RawPutc('S'); RawPutc('3'); RawPutc(']');
//   RawTagHex('s', static_cast<uint32_t>(invoke_status));
//   RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
//   RawNewline();

//   current_subgraph_index_ = previous_subgraph_idx;
//   current_operator_index_ = previous_operator_idx;

//   RawPutc('['); RawPutc('I'); RawPutc('S'); RawPutc('4'); RawPutc(']');
//   RawTagHex('s', static_cast<uint32_t>(invoke_status));
//   RawNewline();

//   return invoke_status;
// }

TfLiteStatus MicroInterpreterGraph::InvokeSubgraph(int subgraph_idx) {
  RawPutc('['); RawPutc('I'); RawPutc('S'); RawPutc('0'); RawPutc(']');
  RawTagHex('s', static_cast<uint32_t>(subgraph_idx));
  RawTagHex('c', static_cast<uint32_t>(current_subgraph_index_));
  RawTagHex('o', static_cast<uint32_t>(current_operator_index_));
  RawNewline();

  int previous_subgraph_idx = current_subgraph_index_;
  uint32_t previous_operator_idx = current_operator_index_;
  current_subgraph_index_ = subgraph_idx;

  RawPutc('['); RawPutc('I'); RawPutc('S'); RawPutc('1'); RawPutc(']');
  RawTagHex('p', static_cast<uint32_t>(
                     reinterpret_cast<uintptr_t>(subgraphs_)));
  if (subgraphs_ != nullptr) {
    RawTagHex('n', static_cast<uint32_t>(subgraphs_->size()));
  }
  RawNewline();

  if (subgraphs_ == nullptr ||
      static_cast<size_t>(subgraph_idx) >= subgraphs_->size()) {
    MicroPrintf("Accessing subgraph %d but only %d subgraphs found",
                subgraph_idx,
                subgraphs_ ? static_cast<int>(subgraphs_->size()) : 0);
    return kTfLiteError;
  }

  TfLiteStatus invoke_status = kTfLiteOk;
  uint32_t operators_size = NumSubgraphOperators(model_, subgraph_idx);

  RawPutc('['); RawPutc('I'); RawPutc('S'); RawPutc('2'); RawPutc(']');
  RawTagHex('m', static_cast<uint32_t>(
                     reinterpret_cast<uintptr_t>(model_)));
  RawTagHex('z', static_cast<uint32_t>(operators_size));
  RawNewline();

  for (current_operator_index_ = 0; current_operator_index_ < operators_size;
       ++current_operator_index_) {

    // constexpr uint32_t kDumpOpIndex = 0x1C;
    const bool dump_this_op = true;
    // constexpr uint32_t kDumpOpIndex = 0x00;
    // const bool dump_this_op = (current_operator_index_ == kDumpOpIndex);
      // const bool dump_this_op =
      //     (current_operator_index_ == 0x00) ||
      //     (current_operator_index_ == 0x01) ||
      //     (current_operator_index_ == 0x02) ||
      //     (current_operator_index_ == 0x1C) ||
      //     (current_operator_index_ == 0x1D) ||
      //     (current_operator_index_ == 0x1E);
    
    RawPutc('['); RawPutc('O'); RawPutc('P'); RawPutc('0'); RawPutc(']');
    RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
    RawNewline();

    TfLiteNode* node = &(subgraph_allocations_[subgraph_idx]
                             .node_and_registrations[current_operator_index_]
                             .node);

    const TFLMRegistration* registration =
        subgraph_allocations_[subgraph_idx]
            .node_and_registrations[current_operator_index_]
            .registration;

    RawPutc('['); RawPutc('O'); RawPutc('P'); RawPutc('1'); RawPutc(']');
    RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
    RawTagHex('n', static_cast<uint32_t>(
                       reinterpret_cast<uintptr_t>(node)));
    RawTagHex('r', static_cast<uint32_t>(
                       reinterpret_cast<uintptr_t>(registration)));
    RawNewline();

    if (registration == nullptr) {
      RawPutc('['); RawPutc('O'); RawPutc('P'); RawPutc('E'); RawPutc(']');
      RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
      RawNewline();
      invoke_status = kTfLiteError;
      break;
    }

    RawPutc('['); RawPutc('O'); RawPutc('P'); RawPutc('2'); RawPutc(']');
    RawTagHex('b', static_cast<uint32_t>(registration->builtin_code));
    RawTagHex('v', static_cast<uint32_t>(
                       reinterpret_cast<uintptr_t>(registration->invoke)));
    RawTagHex('d', static_cast<uint32_t>(
                       reinterpret_cast<uintptr_t>(registration->init)));
    RawTagHex('f', static_cast<uint32_t>(
                       reinterpret_cast<uintptr_t>(registration->free)));
    RawNewline();

    RawPutc('['); RawPutc('N'); RawPutc('D'); RawPutc('0'); RawPutc(']');
    RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
    RawTagHex('u', static_cast<uint32_t>(
                       node->inputs ? node->inputs->size : 0xFFFFFFFFu));
    RawTagHex('o', static_cast<uint32_t>(
                       node->outputs ? node->outputs->size : 0xFFFFFFFFu));
    RawTagHex('b', static_cast<uint32_t>(
                       reinterpret_cast<uintptr_t>(node->builtin_data)));
    RawTagHex('a', static_cast<uint32_t>(
                       reinterpret_cast<uintptr_t>(node->user_data)));
    RawNewline();

    if (node->inputs != nullptr) {
      RawPutc('['); RawPutc('N'); RawPutc('D'); RawPutc('1'); RawPutc(']');
      for (int k = 0; k < node->inputs->size && k < 4; ++k) {
        RawTagHex('x', static_cast<uint32_t>(node->inputs->data[k]));
      }
      RawNewline();
    }

    if (node->outputs != nullptr) {
      RawPutc('['); RawPutc('N'); RawPutc('D'); RawPutc('2'); RawPutc(']');
      for (int k = 0; k < node->outputs->size && k < 4; ++k) {
        RawTagHex('y', static_cast<uint32_t>(node->outputs->data[k]));
      }
      RawNewline();
    }

#if !defined(TF_LITE_STRIP_ERROR_STRINGS)
    ScopedMicroProfiler scoped_profiler(
        OpNameFromRegistration(registration),
        reinterpret_cast<MicroProfilerInterface*>(context_->profiler));
#endif

    TFLITE_DCHECK(registration->invoke);

    RawPutc('['); RawPutc('O'); RawPutc('P'); RawPutc('3'); RawPutc(']');
    RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
    RawNewline();

    // ------------------------------------------------------------
    // Dump selected op inputs BEFORE invoke.
    //
    // input[0] = activation  -> [IA]
    // input[1] = weights     -> [IW]
    // input[2] = bias        -> [IB]
    //
    // Does NOT use interpreter->GetTensor().
    // Does NOT require preserve_all_tensors=true.
    // ------------------------------------------------------------
    if (dump_this_op && node->inputs != nullptr) {
      for (int dump_input_i = 0;
           dump_input_i < node->inputs->size && dump_input_i < 3;
           ++dump_input_i) {
        const int tensor_index = node->inputs->data[dump_input_i];

        if (tensor_index < 0) {
          RawPutc('['); RawPutc('T'); RawPutc('I'); RawPutc('N'); RawPutc(']');
          RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
          RawTagHex('k', static_cast<uint32_t>(dump_input_i));
          RawTagHex('t', static_cast<uint32_t>(tensor_index));
          RawNewline();
          continue;
        }

        TfLiteEvalTensor* eval =
            &subgraph_allocations_[subgraph_idx].tensors[tensor_index];

        if (dump_input_i == 0) {
          RawDumpEvalTensorBytes("IA",
                                 current_operator_index_,
                                 tensor_index,
                                 eval,
                                 16);
        } else if (dump_input_i == 1) {
          RawDumpEvalTensorBytes("IW",
                                 current_operator_index_,
                                 tensor_index,
                                 eval,
                                 16);
        } else if (dump_input_i == 2) {
          RawDumpEvalTensorBytes("IB",
                                 current_operator_index_,
                                 tensor_index,
                                 eval,
                                 16);
        }
      }
    }
    if (dump_this_op && node->outputs != nullptr && node->outputs->size > 0) {
      const int out_tensor_index = node->outputs->data[0];

      if (out_tensor_index >= 0) {
        TfLiteEvalTensor* out_eval =
            &subgraph_allocations_[subgraph_idx].tensors[out_tensor_index];

        RawDumpEvalTensorBytes("OB",   // output before invoke
                              current_operator_index_,
                              out_tensor_index,
                              out_eval,
                              16);
      }
    }
    invoke_status = registration->invoke(context_, node);

    if (current_operator_index_ == 0x1D && invoke_status == kTfLiteOk) {
      int in_tensor_index = node->inputs->data[0];
      int out_tensor_index = node->outputs->data[0];

      TfLiteEvalTensor* in_eval =
          &subgraph_allocations_[subgraph_idx].tensors[in_tensor_index];

      TfLiteEvalTensor* out_eval =
          &subgraph_allocations_[subgraph_idx].tensors[out_tensor_index];

      RawPutc('['); RawPutc('R'); RawPutc('S'); RawPutc('H'); RawPutc(']');
      RawTagHex('i', static_cast<uint32_t>(
                    reinterpret_cast<uintptr_t>(in_eval->data.int8)));
      RawTagHex('o', static_cast<uint32_t>(
                    reinterpret_cast<uintptr_t>(out_eval->data.int8)));
      RawNewline();

      out_eval->data.int8[0] = in_eval->data.int8[0];
      out_eval->data.int8[1] = in_eval->data.int8[1];

      RawDumpEvalTensorBytes("RX", current_operator_index_,
                            out_tensor_index, out_eval, 16);
    }

    RawPutc('['); RawPutc('O'); RawPutc('P'); RawPutc('4'); RawPutc(']');
    RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
    RawTagHex('s', static_cast<uint32_t>(invoke_status));
    RawNewline();

    // ------------------------------------------------------------
    // Dump selected op output AFTER invoke.
    //
    // Must be before ResetTempAllocations().
    // Does NOT require preserve_all_tensors=true.
    // ------------------------------------------------------------
    if (dump_this_op && invoke_status == kTfLiteOk) {
      if (node->outputs != nullptr && node->outputs->size > 0) {
        const int out_tensor_index = node->outputs->data[0];

        if (out_tensor_index >= 0) {
          TfLiteEvalTensor* out_eval =
              &subgraph_allocations_[subgraph_idx].tensors[out_tensor_index];

          RawDumpEvalTensorBytes("TO",
                                 current_operator_index_,
                                 out_tensor_index,
                                 out_eval,
                                 16);
        } else {
          RawPutc('['); RawPutc('T'); RawPutc('O'); RawPutc('N'); RawPutc(']');
          RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
          RawTagHex('t', static_cast<uint32_t>(out_tensor_index));
          RawNewline();
        }
      }
    }

#ifdef USE_TFLM_COMPRESSION
    GetMicroContext(context_)->ResetDecompressionMemoryAllocations();
#endif

    RawPutc('['); RawPutc('R'); RawPutc('T'); RawPutc('0'); RawPutc(']');
    RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
    RawNewline();

    TfLiteStatus reset_status = allocator_->ResetTempAllocations();

    RawPutc('['); RawPutc('R'); RawPutc('T'); RawPutc('1'); RawPutc(']');
    RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
    RawTagHex('s', static_cast<uint32_t>(reset_status));
    RawNewline();

    if (reset_status != kTfLiteOk) {
      invoke_status = reset_status;
      break;
    }

    if (invoke_status != kTfLiteOk) {
      if (invoke_status != kTfLiteAbort) {
        MicroPrintf("Node %s (number %d) failed to invoke with status %d",
                    OpNameFromRegistration(registration),
                    current_operator_index_,
                    invoke_status);
      }
      break;
    }
  }

  RawPutc('['); RawPutc('I'); RawPutc('S'); RawPutc('3'); RawPutc(']');
  RawTagHex('s', static_cast<uint32_t>(invoke_status));
  RawTagHex('i', static_cast<uint32_t>(current_operator_index_));
  RawNewline();

  current_subgraph_index_ = previous_subgraph_idx;
  current_operator_index_ = previous_operator_idx;

  RawPutc('['); RawPutc('I'); RawPutc('S'); RawPutc('4'); RawPutc(']');
  RawTagHex('s', static_cast<uint32_t>(invoke_status));
  RawNewline();

  return invoke_status;
}
TfLiteStatus MicroInterpreterGraph::ResetVariableTensors() {
  for (size_t subgraph_idx = 0; subgraph_idx < subgraphs_->size();
       subgraph_idx++) {
    const SubGraph* subgraph = (*subgraphs_)[subgraph_idx];
    for (size_t i = 0; i < subgraph->tensors()->size(); ++i) {
      auto* tensor = subgraph->tensors()->Get(i);
      if (tensor->is_variable()) {
        TF_LITE_ENSURE_STATUS(ResetTensorData(
            tensor, &subgraph_allocations_[subgraph_idx].tensors[i]));
      }
    }
  }
  if (resource_variables_ != nullptr) {
    resource_variables_->ResetAll();
  }

  return kTfLiteOk;
}

TfLiteStatus MicroInterpreterGraph::ResetVariableTensor(int tensor_index,
                                                        int subgraph_index) {
  if (static_cast<size_t>(subgraph_index) >= subgraphs_->size()) {
    MicroPrintf("Accessing subgraph %d but only %d subgraphs found",
                subgraph_index, subgraphs_->size());
    return kTfLiteError;
  }
  const SubGraph* subgraph = (*subgraphs_)[subgraph_index];
  if (subgraph->tensors() == nullptr ||
      static_cast<size_t>(tensor_index) >= subgraph->tensors()->size()) {
    MicroPrintf(
        "Accessing tensor %d but only %d tensors found in subgraph %d",
        tensor_index,
        (subgraph->tensors() != nullptr ? subgraph->tensors()->size() : 0),
        subgraph_index);
    return kTfLiteError;
  }
  auto* tensor = subgraph->tensors()->Get(tensor_index);
  if (!tensor->is_variable()) {
    MicroPrintf("Accessing tensor %d in subgraph %d which is not a variable",
                tensor_index, subgraph_index);
    return kTfLiteError;
  }

  return ResetTensorData(
      tensor, &subgraph_allocations_[subgraph_index].tensors[tensor_index]);
}

TfLiteStatus MicroInterpreterGraph::ResetTensorData(
    const tflite::Tensor* tensor, TfLiteEvalTensor* eval_tensor) {
  size_t buffer_size;
  TF_LITE_ENSURE_STATUS(TfLiteEvalTensorByteLength(eval_tensor, &buffer_size));

  int value = 0;
  if (tensor->type() == tflite::TensorType_INT8 && tensor->quantization() &&
      tensor->quantization()->zero_point() &&
      tensor->quantization()->zero_point()->size() > 0) {
    value = tensor->quantization()->zero_point()->Get(0);
  }
  memset(eval_tensor->data.raw, value, buffer_size);

  return kTfLiteOk;
}

int MicroInterpreterGraph::NumSubgraphs() {
  return model_->subgraphs()->size();
}

void MicroInterpreterGraph::SetSubgraphAllocations(
    SubgraphAllocations* subgraph_allocations) {
  subgraph_allocations_ = subgraph_allocations;
}

size_t MicroInterpreterGraph::NumSubgraphInputs(int subgraph_idx) {
  return model_->subgraphs()->Get(subgraph_idx)->inputs()->size();
}

TfLiteEvalTensor* MicroInterpreterGraph::GetSubgraphInput(int subgraph_idx,
                                                          int input_idx) {
  int tensor_idx =
      model_->subgraphs()->Get(subgraph_idx)->inputs()->Get(input_idx);
  return &subgraph_allocations_[subgraph_idx].tensors[tensor_idx];
}

size_t MicroInterpreterGraph::NumSubgraphOutputs(int subgraph_idx) {
  return model_->subgraphs()->Get(subgraph_idx)->outputs() == nullptr
             ? 0
             : model_->subgraphs()->Get(subgraph_idx)->outputs()->size();
}

TfLiteEvalTensor* MicroInterpreterGraph::GetSubgraphOutput(int subgraph_idx,
                                                           int output_idx) {
  int tensor_idx =
      model_->subgraphs()->Get(subgraph_idx)->outputs()->Get(output_idx);
  return &subgraph_allocations_[subgraph_idx].tensors[tensor_idx];
}

}  // namespace tflite
