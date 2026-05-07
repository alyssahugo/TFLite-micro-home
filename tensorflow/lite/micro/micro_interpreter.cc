/* Copyright 2022 The TensorFlow Authors. All Rights Reserved.

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
#include "tensorflow/lite/micro/micro_interpreter.h"

#include <cstdarg>
#include <cstddef>
#include <cstdint>

#include "flatbuffers/flatbuffers.h"  // from @flatbuffers
#include "tensorflow/lite/c/c_api_types.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/flatbuffer_utils.h"
#include "tensorflow/lite/micro/memory_helpers.h"
#include "tensorflow/lite/micro/micro_allocator.h"
#include "tensorflow/lite/micro/micro_interpreter_context.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_op_resolver.h"
#include "tensorflow/lite/micro/micro_profiler_interface.h"
#include "tensorflow/lite/micro/tflite_bridge/flatbuffer_conversions_bridge.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/schema/schema_utils.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/core/api/error_reporter.h"
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"
#include "tensorflow/lite/core/api/flatbuffer_conversions.h"

#include <stdint.h>

// static inline void RawPutc(char c) {
//   volatile uint32_t* const uart_tx =
//       reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
//   volatile uint32_t* const uart_status =
//       reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x08u);
//   while ((*uart_status) & 0x08u) {}
//   *uart_tx = static_cast<uint32_t>(static_cast<uint8_t>(c));
// }

// static inline void RawNewline() {
//   RawPutc('\r');
//   RawPutc('\n');
// }

// static inline void RawPutHexNibble(uint32_t v) {
//   v &= 0xFu;
//   RawPutc((v < 10u) ? static_cast<char>('0' + v)
//                     : static_cast<char>('A' + (v - 10u)));
// }

// static inline void RawPutHex32(uint32_t v) {
//   for (int shift = 28; shift >= 0; shift -= 4) {
//     RawPutHexNibble(v >> shift);
//   }
// }

// static inline void RawTagHex(char tag, uint32_t v) {
//   RawPutc(tag);
//   RawPutHex32(v);
//   RawPutc(' ');
// }

namespace tflite {
namespace {
MemoryPlannerType FlagToMemoryPlannerType(bool preserve_all_tensors) {
  if (preserve_all_tensors) {
    return MemoryPlannerType::kLinear;
  } else {
    return MemoryPlannerType::kGreedy;
  }
}
__attribute__((noinline))
static SubgraphAllocations* StartModelAllocationNoInline(
    MicroAllocator& allocator, const Model* model) {


  SubgraphAllocations* result = allocator.StartModelAllocationDirect(model);


  return result;
}

__attribute__((noinline))
static TfLiteStatus FinishModelAllocationNoInline(
    MicroAllocator& allocator, const Model* model,
    SubgraphAllocations* allocs, ScratchBufferHandle** handles) {


  TfLiteStatus st =
      allocator.FinishModelAllocation(model, allocs, handles);


  return st;
}

__attribute__((noinline))
static TfLiteTensor* AllocatePersistentTfLiteTensorNoInline(
    MicroAllocator& allocator, const Model* model,
    const SubgraphAllocations* allocs, int tensor_index, int subgraph_index) {


  TfLiteTensor* result =
      allocator.AllocatePersistentTfLiteTensor(model, allocs, tensor_index,
                                               subgraph_index);


  return result;
}
}  // namespace

MicroInterpreter::MicroInterpreter(const Model* model,
                                   const MicroOpResolver& op_resolver,
                                   uint8_t* tensor_arena,
                                   size_t tensor_arena_size,
                                   MicroResourceVariables* resource_variables,
                                   MicroProfilerInterface* profiler,
                                   bool preserve_all_tensors)
    : model_(model),
      op_resolver_(op_resolver),
      allocator_(*MicroAllocator::Create(
          tensor_arena, tensor_arena_size,
          FlagToMemoryPlannerType(preserve_all_tensors))),
      graph_(&context_, model, &allocator_, resource_variables),
      tensors_allocated_(false),
      initialization_status_(kTfLiteError),
      input_tensors_(nullptr),
      output_tensors_(nullptr),
      micro_context_(&allocator_, model_, &graph_) {
  Init(profiler);
}

MicroInterpreter::MicroInterpreter(const Model* model,
                                   const MicroOpResolver& op_resolver,
                                   MicroAllocator* allocator,
                                   MicroResourceVariables* resource_variables,
                                   MicroProfilerInterface* profiler)
    : model_(model),
      op_resolver_(op_resolver),
      allocator_(*allocator),
      graph_(&context_, model, allocator, resource_variables),
      tensors_allocated_(false),
      initialization_status_(kTfLiteError),
      input_tensors_(nullptr),
      output_tensors_(nullptr),
      micro_context_(&allocator_, model_, &graph_) {
  Init(profiler);
}

MicroInterpreter::~MicroInterpreter() {
  if (graph_.GetAllocations() != nullptr) {
    graph_.FreeSubgraphs();
  }
}

__attribute__((noinline, used))
static TfLitePadding ConvertPaddingDirect(Padding padding) {
  switch (padding) {
    case Padding_SAME:
      return kTfLitePaddingSame;
    case Padding_VALID:
      return kTfLitePaddingValid;
  }
  return kTfLitePaddingUnknown;
}

__attribute__((noinline, used))
static TfLiteFusedActivation ConvertActivationDirect(
    ActivationFunctionType activation) {
  switch (activation) {
    case ActivationFunctionType_NONE:
      return kTfLiteActNone;
    case ActivationFunctionType_RELU:
      return kTfLiteActRelu;
    case ActivationFunctionType_RELU_N1_TO_1:
      return kTfLiteActReluN1To1;
    case ActivationFunctionType_RELU6:
      return kTfLiteActRelu6;
    case ActivationFunctionType_TANH:
      return kTfLiteActTanh;
    case ActivationFunctionType_SIGN_BIT:
      return kTfLiteActSignBit;
  }
  return kTfLiteActNone;
}

__attribute__((noinline, used))
static TfLiteStatus ConvertTensorTypeDirect(
    TensorType tensor_type,
    TfLiteType* type) {
  if (type == nullptr) {
    return kTfLiteError;
  }

  switch (tensor_type) {
    case TensorType_INT8:
      *type = kTfLiteInt8;
      return kTfLiteOk;
    case TensorType_INT16:
      *type = kTfLiteInt16;
      return kTfLiteOk;
    case TensorType_INT32:
      *type = kTfLiteInt32;
      return kTfLiteOk;
    case TensorType_UINT8:
      *type = kTfLiteUInt8;
      return kTfLiteOk;
    case TensorType_FLOAT32:
      *type = kTfLiteFloat32;
      return kTfLiteOk;
    case TensorType_BOOL:
      *type = kTfLiteBool;
      return kTfLiteOk;
    default:
      *type = kTfLiteNoType;
      return kTfLiteError;
  }
}




void MicroInterpreter::Init(MicroProfilerInterface* profiler) {
  micro_context_.SetInterpreterState(
      MicroInterpreterContext::InterpreterState::kInit);
  context_.impl_ = static_cast<void*>(&micro_context_);
  context_.ReportError = MicroContextReportOpError;
  context_.GetTensor = MicroContextGetTensor;
  context_.GetEvalTensor = MicroContextGetEvalTensor;
  context_.profiler = profiler;
  context_.RequestScratchBufferInArena =
      MicroContextRequestScratchBufferInArena;
  context_.GetExternalContext = MicroContextGetExternalContext;
  context_.AllocatePersistentBuffer = MicroContextAllocatePersistentBuffer;
  context_.GetScratchBuffer = MicroContextGetScratchBuffer;

  initialization_status_ = kTfLiteOk;
}


__attribute__((noinline, used))
static TfLiteIntArray* FlatBufferVectorToTfLiteTypeArrayDirectSafe(
    const flatbuffers::Vector<int32_t>* flatbuffer_array) {


  if (flatbuffer_array == nullptr) {

    return nullptr;
  }




  TfLiteIntArray* out =
      const_cast<TfLiteIntArray*>(
          reinterpret_cast<const TfLiteIntArray*>(flatbuffer_array));



  return out;
}


__attribute__((noinline, used))
static void* AllocateBuiltinDataDirect(
    MicroAllocator& allocator,
    size_t bytes,
    size_t alignment) {


  void* raw = allocator.AllocatePersistentBufferFromPersistentDirect(
      bytes, alignment);



  return raw;
}

__attribute__((noinline, used))
static TfLiteStatus ParseDepthwiseConv2DManualDirect(
    MicroAllocator& allocator,
    const Operator* op,
    void** builtin_data) {


  if (op == nullptr || builtin_data == nullptr) {
    return kTfLiteError;
  }

  *builtin_data = nullptr;

  void* raw = AllocateBuiltinDataDirect(
      allocator,
      sizeof(TfLiteDepthwiseConvParams),
      alignof(TfLiteDepthwiseConvParams));



  if (raw == nullptr) {
    return kTfLiteError;
  }

  TfLiteDepthwiseConvParams* params =
      reinterpret_cast<TfLiteDepthwiseConvParams*>(raw);

  params->padding = kTfLitePaddingUnknown;
  params->stride_width = 0;
  params->stride_height = 0;
  params->depth_multiplier = 0;
  params->activation = kTfLiteActNone;
  params->dilation_width_factor = 0;
  params->dilation_height_factor = 0;

  const DepthwiseConv2DOptions* schema_params =
      op->builtin_options_as_DepthwiseConv2DOptions();



  if (schema_params != nullptr) {
    params->padding =
        ConvertPaddingDirect(schema_params->padding());
    params->stride_width = schema_params->stride_w();
    params->stride_height = schema_params->stride_h();
    params->depth_multiplier = schema_params->depth_multiplier();
    params->activation =
        ConvertActivationDirect(schema_params->fused_activation_function());
    params->dilation_width_factor = schema_params->dilation_w_factor();
    params->dilation_height_factor = schema_params->dilation_h_factor();
  }

  *builtin_data = params;



  return kTfLiteOk;
}

__attribute__((noinline, used))
static TfLiteStatus ParseConv2DManualDirect(
    MicroAllocator& allocator,
    const Operator* op,
    void** builtin_data) {


  if (op == nullptr || builtin_data == nullptr) {
    return kTfLiteError;
  }

  *builtin_data = nullptr;

  void* raw = AllocateBuiltinDataDirect(
      allocator,
      sizeof(TfLiteConvParams),
      alignof(TfLiteConvParams));



  if (raw == nullptr) {
    return kTfLiteError;
  }

  TfLiteConvParams* params =
      reinterpret_cast<TfLiteConvParams*>(raw);

  params->padding = kTfLitePaddingUnknown;
  params->stride_width = 0;
  params->stride_height = 0;
  params->activation = kTfLiteActNone;
  params->dilation_width_factor = 0;
  params->dilation_height_factor = 0;
  params->quantized_bias_type = kTfLiteNoType;

  const Conv2DOptions* schema_params =
      op->builtin_options_as_Conv2DOptions();



  if (schema_params != nullptr) {
    params->padding =
        ConvertPaddingDirect(schema_params->padding());
    params->stride_width = schema_params->stride_w();
    params->stride_height = schema_params->stride_h();
    params->activation =
        ConvertActivationDirect(schema_params->fused_activation_function());
    params->dilation_width_factor = schema_params->dilation_w_factor();
    params->dilation_height_factor = schema_params->dilation_h_factor();

    TfLiteStatus st = ConvertTensorTypeDirect(
        schema_params->quantized_bias_type(),
        &params->quantized_bias_type);
    if (st != kTfLiteOk) {

      *builtin_data = nullptr;
      return st;
    }
  }

  *builtin_data = params;



  return kTfLiteOk;
}

__attribute__((noinline, used))
static TfLiteStatus ParsePoolManualDirect(
    MicroAllocator& allocator,
    const Operator* op,
    void** builtin_data) {


  if (op == nullptr || builtin_data == nullptr) {
    return kTfLiteError;
  }

  *builtin_data = nullptr;

  void* raw = AllocateBuiltinDataDirect(
      allocator,
      sizeof(TfLitePoolParams),
      alignof(TfLitePoolParams));



  if (raw == nullptr) {
    return kTfLiteError;
  }

  TfLitePoolParams* params =
      reinterpret_cast<TfLitePoolParams*>(raw);

  params->padding = kTfLitePaddingUnknown;
  params->stride_width = 0;
  params->stride_height = 0;
  params->filter_width = 0;
  params->filter_height = 0;
  params->activation = kTfLiteActNone;

  const Pool2DOptions* schema_params =
      op->builtin_options_as_Pool2DOptions();



  if (schema_params != nullptr) {
    params->padding =
        ConvertPaddingDirect(schema_params->padding());
    params->stride_width = schema_params->stride_w();
    params->stride_height = schema_params->stride_h();
    params->filter_width = schema_params->filter_width();
    params->filter_height = schema_params->filter_height();
    params->activation =
        ConvertActivationDirect(schema_params->fused_activation_function());
  }

  *builtin_data = params;



  return kTfLiteOk;
}

__attribute__((noinline, used))
static TfLiteStatus ParseSoftmaxManualDirect(
    MicroAllocator& allocator,
    const Operator* op,
    void** builtin_data) {


  if (op == nullptr || builtin_data == nullptr) {
    return kTfLiteError;
  }

  *builtin_data = nullptr;

  void* raw = AllocateBuiltinDataDirect(
      allocator,
      sizeof(TfLiteSoftmaxParams),
      alignof(TfLiteSoftmaxParams));



  if (raw == nullptr) {
    return kTfLiteError;
  }

  TfLiteSoftmaxParams* params =
      reinterpret_cast<TfLiteSoftmaxParams*>(raw);

  params->beta = 0.0f;

  const SoftmaxOptions* schema_params =
      op->builtin_options_as_SoftmaxOptions();


  if (schema_params != nullptr) {
    params->beta = schema_params->beta();
  }

  *builtin_data = params;


  return kTfLiteOk;
}

__attribute__((noinline, used))
static TfLiteStatus ParseReshapeManualDirect(
    MicroAllocator& allocator,
    const Operator* op,
    void** builtin_data) {


  if (op == nullptr || builtin_data == nullptr) {
    return kTfLiteError;
  }

  *builtin_data = nullptr;

  void* raw = AllocateBuiltinDataDirect(
      allocator,
      sizeof(TfLiteReshapeParams),
      alignof(TfLiteReshapeParams));



  if (raw == nullptr) {
    return kTfLiteError;
  }

  TfLiteReshapeParams* params =
      reinterpret_cast<TfLiteReshapeParams*>(raw);

  params->num_dimensions = 0;
  for (int j = 0; j < 8; ++j) {
    params->shape[j] = 0;
  }

  const ReshapeOptions* schema_params =
      op->builtin_options_as_ReshapeOptions();



  if (schema_params != nullptr) {
    const flatbuffers::Vector<int32_t>* new_shape =
        schema_params->new_shape();



    if (new_shape != nullptr) {
      size_t n = new_shape->size();



      if (n > 8) {
        *builtin_data = nullptr;
        return kTfLiteError;
      }

      for (size_t j = 0; j < n; ++j) {
        params->shape[j] = new_shape->Get(j);
      }
      params->num_dimensions = static_cast<int>(n);
    }
  }

  *builtin_data = params;



  return kTfLiteOk;
}

__attribute__((noinline))
TfLiteStatus MicroInterpreter::PrepareNodeAndRegistrationDataFromFlatbuffer() {


  int num_subgraphs = graph_.NumSubgraphs();


  for (int subgraph_idx = 0; subgraph_idx < num_subgraphs; subgraph_idx++) {


    const auto* subgraphs = model_->subgraphs();

 

    if (subgraphs == nullptr) {

      return kTfLiteError;
    }

    const SubGraph* subgraph = subgraphs->Get(subgraph_idx);


    if (subgraph == nullptr) {

      return kTfLiteError;
    }

    auto* opcodes = model_->operator_codes();


    if (opcodes == nullptr) {
      return kTfLiteError;
    }

    TfLiteBridgeBuiltinDataAllocator* builtin_data_allocator =
        allocator_.GetBuiltinDataAllocator();



    uint32_t operators_size = NumSubgraphOperators(subgraph);


    auto* allocations = graph_.GetAllocations();

    if (allocations == nullptr) {
      return kTfLiteError;
    }

    NodeAndRegistration* node_regs =
        allocations[subgraph_idx].node_and_registrations;



    if (node_regs == nullptr) {
      return kTfLiteError;
    }

    for (size_t i = 0; i < operators_size; ++i) {


      const auto* operators = subgraph->operators();


      if (operators == nullptr) {

        return kTfLiteError;
      }

      const auto* op = operators->Get(i);


      if (op == nullptr) {

        return kTfLiteError;
      }

      const size_t index = op->opcode_index();
      if (index >= opcodes->size()) {
        MicroPrintf("Missing registration for opcode_index %d\n", index);
        return kTfLiteError;
      }

      const auto* opcode = opcodes->Get(index);

      if (opcode == nullptr) {
        return kTfLiteError;
      }

      // RawPutc('['); RawPutc('G'); RawPutc('R'); RawPutc('0'); RawPutc(']');
      // RawNewline();

      // TfLiteStatus status =
      //     GetRegistrationFromOpCode(opcode, op_resolver_,
      //                               &(node_regs[i].registration));

      // RawPutc('['); RawPutc('G'); RawPutc('R'); RawPutc('1'); RawPutc(']');
      // RawTagHex('s', static_cast<uint32_t>(status));
      // RawTagHex('r', static_cast<uint32_t>(
      //                    reinterpret_cast<uintptr_t>(
      //                        node_regs[i].registration)));
      // RawNewline();



      BuiltinOperator builtin_code = GetBuiltinCode(opcode);


      TfLiteStatus status = kTfLiteOk;
      node_regs[i].registration = nullptr;

      if (builtin_code > BuiltinOperator_MAX) {

        status = kTfLiteError;
      } else if (builtin_code == BuiltinOperator_CUSTOM) {

        // person_detection should not need custom ops.
        // Avoid custom-name resolver lookup for now.
        status = kTfLiteError;
      } else {


        const MicroMutableOpResolver<5>& mutable_resolver =
            static_cast<const MicroMutableOpResolver<5>&>(op_resolver_);


        node_regs[i].registration =
            mutable_resolver.FindOpDirect(builtin_code);


        if (node_regs[i].registration == nullptr) {
          status = kTfLiteError;
        }
      }

      if (status != kTfLiteOk) {
        MicroPrintf("Failed to get registration from op code %s\n ",
                    EnumNameBuiltinOperator(GetBuiltinCode(opcode)));
        return status;
      }

      const auto* registration = node_regs[i].registration;


      if (registration == nullptr) {
        MicroPrintf("Skipping op for opcode_index %d\n", index);
        return kTfLiteError;
      }

      BuiltinOperator op_type =
          static_cast<BuiltinOperator>(registration->builtin_code);
      const char* custom_data = nullptr;
      size_t custom_data_size = 0;
      unsigned char* builtin_data = nullptr;

      if (op_type == BuiltinOperator_CUSTOM) {
 

        if (op->custom_options() != nullptr) {
          custom_data =
              reinterpret_cast<const char*>(op->custom_options()->data());
          custom_data_size = op->custom_options()->size();
        }

      } else {


        if (op->custom_options() != nullptr) {

          MicroPrintf(
              "Unsupported behavior: found builtin operator %s with custom "
              "options.\n",
              EnumNameBuiltinOperator(op_type));
          return kTfLiteError;
        }


        // TfLiteBridgeBuiltinParseFunction parser =
        //     op_resolver_.GetOpDataParser(op_type);


        const MicroMutableOpResolver<5>& mutable_resolver_for_parser =
            static_cast<const MicroMutableOpResolver<5>&>(op_resolver_);

        TfLiteBridgeBuiltinParseFunction parser =
            mutable_resolver_for_parser.GetOpDataParserDirect(op_type);



        if (parser == nullptr) {

          MicroPrintf("Did not find a parser for %s",
                      EnumNameBuiltinOperator(op_type));
          return kTfLiteError;
        }

        // RawPutc('['); RawPutc('C'); RawPutc('P'); RawPutc('0'); RawPutc(']');
        // RawNewline();

        // // TfLiteStatus parse_status =
        // //     CallBuiltinParseFunction(
        // //         parser, op, builtin_data_allocator,
        // //         reinterpret_cast<void**>(&builtin_data));



        // RawPutc('['); RawPutc('C'); RawPutc('D'); RawPutc('0'); RawPutc(']');
        // RawTagHex('p', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(parser)));
        // RawTagHex('o', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(op)));
        // RawTagHex('a', static_cast<uint32_t>(
        //                     reinterpret_cast<uintptr_t>(builtin_data_allocator)));
        // RawNewline();

        // void* parsed_data = nullptr;

        // RawPutc('['); RawPutc('C'); RawPutc('D'); RawPutc('1'); RawPutc(']');
        // RawNewline();

        // ErrorReporter* reporter = tflite::GetMicroErrorReporter();

        // RawPutc('['); RawPutc('C'); RawPutc('D'); RawPutc('2'); RawPutc(']');
        // RawTagHex('e', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(reporter)));
        // RawNewline();

        // TfLiteStatus parse_status =
        //     parser(op, reporter, builtin_data_allocator, &parsed_data);

        // RawPutc('['); RawPutc('C'); RawPutc('D'); RawPutc('3'); RawPutc(']');
        // RawTagHex('s', static_cast<uint32_t>(parse_status));
        // RawTagHex('d', static_cast<uint32_t>(
        //                     reinterpret_cast<uintptr_t>(parsed_data)));
        // RawNewline();

        // builtin_data = reinterpret_cast<unsigned char*>(parsed_data);

        // RawPutc('['); RawPutc('C'); RawPutc('P'); RawPutc('1'); RawPutc(']');
        // RawTagHex('s', static_cast<uint32_t>(parse_status));
        // RawTagHex('d', static_cast<uint32_t>(
        //                    reinterpret_cast<uintptr_t>(builtin_data)));
        // RawNewline();

        // if (parse_status != kTfLiteOk) {
        //   RawPutc('['); RawPutc('C'); RawPutc('P'); RawPutc('X'); RawPutc(']');
        //   RawNewline();
        //   return parse_status;
        // }

        // TfLiteStatus parse_status =
        //     CallBuiltinParseFunction(
        //         parser, op, builtin_data_allocator,
        //         reinterpret_cast<void**>(&builtin_data));

        // void* parsed_data = nullptr;
        // TfLiteStatus parse_status = kTfLiteError;

        // RawPutc('['); RawPutc('C'); RawPutc('S'); RawPutc('0'); RawPutc(']');
        // RawTagHex('b', static_cast<uint32_t>(op_type));
        // RawNewline();

        // ErrorReporter* reporter = tflite::GetMicroErrorReporter();

        // RawPutc('['); RawPutc('C'); RawPutc('S'); RawPutc('1'); RawPutc(']');
        // RawTagHex('e', static_cast<uint32_t>(
        //                   reinterpret_cast<uintptr_t>(reporter)));
        // RawNewline();

        // switch (op_type) {
        //   case BuiltinOperator_AVERAGE_POOL_2D:
        //     RawPutc('['); RawPutc('A'); RawPutc('P'); RawPutc('0'); RawPutc(']');
        //     RawNewline();

        //     parse_status =
        //         ParsePool(op, reporter, builtin_data_allocator, &parsed_data);

        //     RawPutc('['); RawPutc('A'); RawPutc('P'); RawPutc('1'); RawPutc(']');
        //     RawTagHex('s', static_cast<uint32_t>(parse_status));
        //     RawTagHex('d', static_cast<uint32_t>(
        //                       reinterpret_cast<uintptr_t>(parsed_data)));
        //     RawNewline();
        //     break;

        //   case BuiltinOperator_CONV_2D:
        //     RawPutc('['); RawPutc('C'); RawPutc('V'); RawPutc('0'); RawPutc(']');
        //     RawNewline();

        //     parse_status =
        //         ParseConv2D(op, reporter, builtin_data_allocator, &parsed_data);

        //     RawPutc('['); RawPutc('C'); RawPutc('V'); RawPutc('1'); RawPutc(']');
        //     RawTagHex('s', static_cast<uint32_t>(parse_status));
        //     RawTagHex('d', static_cast<uint32_t>(
        //                       reinterpret_cast<uintptr_t>(parsed_data)));
        //     RawNewline();
        //     break;

        //   // case BuiltinOperator_DEPTHWISE_CONV_2D:
        //   //   RawPutc('['); RawPutc('D'); RawPutc('W'); RawPutc('0'); RawPutc(']');
        //   //   RawNewline();

        //   //   parse_status =
        //   //       ParseDepthwiseConv2D(op, reporter, builtin_data_allocator,
        //   //                           &parsed_data);

        //   //   RawPutc('['); RawPutc('D'); RawPutc('W'); RawPutc('1'); RawPutc(']');
        //   //   RawTagHex('s', static_cast<uint32_t>(parse_status));
        //   //   RawTagHex('d', static_cast<uint32_t>(
        //   //                     reinterpret_cast<uintptr_t>(parsed_data)));
        //   //   RawNewline();
        //   //   break;

        //   case BuiltinOperator_DEPTHWISE_CONV_2D:
        //     RawPutc('['); RawPutc('D'); RawPutc('W'); RawPutc('0'); RawPutc(']');
        //     RawNewline();

        //     parse_status =
        //         ParseDepthwiseConv2DManualDirect(
        //             allocator_, op, &parsed_data);

        //     RawPutc('['); RawPutc('D'); RawPutc('W'); RawPutc('1'); RawPutc(']');
        //     RawTagHex('s', static_cast<uint32_t>(parse_status));
        //     RawTagHex('d', static_cast<uint32_t>(
        //                       reinterpret_cast<uintptr_t>(parsed_data)));
        //     RawNewline();
        //     break;


        //   case BuiltinOperator_RESHAPE:
        //     RawPutc('['); RawPutc('R'); RawPutc('S'); RawPutc('0'); RawPutc(']');
        //     RawNewline();

        //     parse_status =
        //         ParseReshape(op, reporter, builtin_data_allocator, &parsed_data);

        //     RawPutc('['); RawPutc('R'); RawPutc('S'); RawPutc('1'); RawPutc(']');
        //     RawTagHex('s', static_cast<uint32_t>(parse_status));
        //     RawTagHex('d', static_cast<uint32_t>(
        //                       reinterpret_cast<uintptr_t>(parsed_data)));
        //     RawNewline();
        //     break;

        //   case BuiltinOperator_SOFTMAX:
        //     RawPutc('['); RawPutc('S'); RawPutc('M'); RawPutc('0'); RawPutc(']');
        //     RawNewline();

        //     parse_status =
        //         ParseSoftmax(op, reporter, builtin_data_allocator, &parsed_data);

        //     RawPutc('['); RawPutc('S'); RawPutc('M'); RawPutc('1'); RawPutc(']');
        //     RawTagHex('s', static_cast<uint32_t>(parse_status));
        //     RawTagHex('d', static_cast<uint32_t>(
        //                       reinterpret_cast<uintptr_t>(parsed_data)));
        //     RawNewline();
        //     break;

        //   default:
        //     RawPutc('['); RawPutc('C'); RawPutc('S'); RawPutc('X'); RawPutc(']');
        //     RawTagHex('b', static_cast<uint32_t>(op_type));
        //     RawNewline();
        //     parse_status = kTfLiteError;
        //     break;
        // }

        // builtin_data = reinterpret_cast<unsigned char*>(parsed_data);

        void* parsed_data = nullptr;
        TfLiteStatus parse_status = kTfLiteError;

        switch (op_type) {
          case BuiltinOperator_DEPTHWISE_CONV_2D:

            parse_status =
                ParseDepthwiseConv2DManualDirect(allocator_, op, &parsed_data);

            break;

          case BuiltinOperator_CONV_2D:

            parse_status =
                ParseConv2DManualDirect(allocator_, op, &parsed_data);

            break;

          case BuiltinOperator_AVERAGE_POOL_2D:

            parse_status =
                ParsePoolManualDirect(allocator_, op, &parsed_data);

            break;

          case BuiltinOperator_RESHAPE:

            parse_status =
                ParseReshapeManualDirect(allocator_, op, &parsed_data);

            break;

          case BuiltinOperator_SOFTMAX:
            parse_status =
                ParseSoftmaxManualDirect(allocator_, op, &parsed_data);

            break;

          default:

            parse_status = kTfLiteError;
            break;
        }



        builtin_data = reinterpret_cast<unsigned char*>(parsed_data);



        if (parse_status != kTfLiteOk) {

          return parse_status;
        }
      }



      // TfLiteIntArray* inputs_array =
      //     FlatBufferVectorToTfLiteTypeArray(op->inputs());

      const auto* fb_inputs = op->inputs();



      TfLiteIntArray* inputs_array =
          FlatBufferVectorToTfLiteTypeArrayDirectSafe(fb_inputs);



      // TfLiteIntArray* outputs_array =
      //     FlatBufferVectorToTfLiteTypeArray(op->outputs());

      const auto* fb_outputs = op->outputs();



      TfLiteIntArray* outputs_array =
          FlatBufferVectorToTfLiteTypeArrayDirectSafe(fb_outputs);



      TfLiteNode* node = &(node_regs[i].node);


      // Avoid *node = {}; for now. Explicitly clear fields instead.
      // Match the TfLiteNode fields available in this TFLM version.
      node->inputs = nullptr;
      node->outputs = nullptr;
      node->intermediates = nullptr;
      node->user_data = nullptr;
      node->builtin_data = nullptr;
      node->custom_initial_data = nullptr;
      node->custom_initial_data_size = 0;



      node->inputs = inputs_array;
      node->outputs = outputs_array;
      node->builtin_data = reinterpret_cast<void*>(builtin_data);
      node->custom_initial_data = custom_data;
      node->custom_initial_data_size = custom_data_size;



      // if (op->intermediates() && (op->intermediates()->size() > 0)) {
      //   RawPutc('['); RawPutc('F'); RawPutc('B'); RawPutc('3'); RawPutc(']');
      //   RawNewline();

      //   node->intermediates =
      //       FlatBufferVectorToTfLiteTypeArray(op->intermediates());

      //   RawPutc('['); RawPutc('F'); RawPutc('B'); RawPutc('4'); RawPutc(']');
      //   RawTagHex('t', static_cast<uint32_t>(
      //                      reinterpret_cast<uintptr_t>(
      //                          node->intermediates)));
      //   RawNewline();
      // }

        const auto* fb_intermediates = op->intermediates();


        if (fb_intermediates != nullptr && fb_intermediates->size() > 0) {
          node->intermediates =
              FlatBufferVectorToTfLiteTypeArrayDirectSafe(fb_intermediates);

        }

    }
  }

  return kTfLiteOk;
}


// TfLiteStatus MicroInterpreter::PrepareNodeAndRegistrationDataFromFlatbuffer() {
//   for (int subgraph_idx = 0; subgraph_idx < graph_.NumSubgraphs();
//        subgraph_idx++) {
//     const SubGraph* subgraph = model_->subgraphs()->Get(subgraph_idx);
//     TFLITE_DCHECK(subgraph != nullptr);

//     auto* opcodes = model_->operator_codes();
//     TfLiteBridgeBuiltinDataAllocator* builtin_data_allocator =
//         allocator_.GetBuiltinDataAllocator();
//     uint32_t operators_size = NumSubgraphOperators(subgraph);
//     for (size_t i = 0; i < operators_size; ++i) {
//       const auto* op = subgraph->operators()->Get(i);
//       const size_t index = op->opcode_index();
//       if (index >= opcodes->size()) {
//         MicroPrintf("Missing registration for opcode_index %d\n", index);
//         return kTfLiteError;
//       }
//       const auto* opcode = opcodes->Get(index);
//       TfLiteStatus status =
//           GetRegistrationFromOpCode(opcode, op_resolver_,
//                                     &(graph_.GetAllocations()[subgraph_idx]
//                                           .node_and_registrations[i]
//                                           .registration));
//       if (status != kTfLiteOk) {
//         MicroPrintf("Failed to get registration from op code %s\n ",
//                     EnumNameBuiltinOperator(GetBuiltinCode(opcode)));
//         return status;
//       }
//       const auto* registration = graph_.GetAllocations()[subgraph_idx]
//                                      .node_and_registrations[i]
//                                      .registration;
//       if (registration == nullptr) {
//         MicroPrintf("Skipping op for opcode_index %d\n", index);
//         return kTfLiteError;
//       }
//       BuiltinOperator op_type =
//           static_cast<BuiltinOperator>(registration->builtin_code);

//       const char* custom_data = nullptr;
//       size_t custom_data_size = 0;
//       unsigned char* builtin_data = nullptr;

//       if (op_type == BuiltinOperator_CUSTOM) {
//         // Custom Ops may or may not have a non-null custom_options field.
//         if (op->custom_options() != nullptr) {
//           custom_data =
//               reinterpret_cast<const char*>(op->custom_options()->data());
//           custom_data_size = op->custom_options()->size();
//         }
//       } else {
//         if (op->custom_options() != nullptr) {
//           MicroPrintf(
//               "Unsupported behavior: found builtin operator %s with custom "
//               "options.\n",
//               EnumNameBuiltinOperator(op_type));
//           return kTfLiteError;
//         }

//         TfLiteBridgeBuiltinParseFunction parser =
//             op_resolver_.GetOpDataParser(op_type);
//         if (parser == nullptr) {
//           MicroPrintf("Did not find a parser for %s",
//                       EnumNameBuiltinOperator(op_type));

//           return kTfLiteError;
//         }
//         TF_LITE_ENSURE_STATUS(CallBuiltinParseFunction(
//             parser, op, builtin_data_allocator, (void**)(&builtin_data)));
//       }

//       TfLiteIntArray* inputs_array =
//           FlatBufferVectorToTfLiteTypeArray(op->inputs());
//       TfLiteIntArray* outputs_array =
//           FlatBufferVectorToTfLiteTypeArray(op->outputs());

//       TfLiteNode* node = &(
//           graph_.GetAllocations()[subgraph_idx].node_and_registrations[i].node);
//       *node = {};
//       node->inputs = inputs_array;
//       node->outputs = outputs_array;
//       node->builtin_data = reinterpret_cast<void*>(builtin_data);
//       node->custom_initial_data = custom_data;
//       node->custom_initial_data_size = custom_data_size;

//       if (op->intermediates() && (op->intermediates()->size() > 0)) {
//         node->intermediates =
//             FlatBufferVectorToTfLiteTypeArray(op->intermediates());
//       }
//     }
//   }
//   return kTfLiteOk;
// }

// TfLiteStatus MicroInterpreter::AllocateTensors() {
//   SubgraphAllocations* allocations = allocator_.StartModelAllocation(model_);

//   if (allocations == nullptr) {
//     MicroPrintf("Failed starting model allocation.\n");
//     initialization_status_ = kTfLiteError;
//     return kTfLiteError;
//   }

//   graph_.SetSubgraphAllocations(allocations);

//   TF_LITE_ENSURE_STATUS(PrepareNodeAndRegistrationDataFromFlatbuffer());

//   micro_context_.SetInterpreterState(
//       MicroInterpreterContext::InterpreterState::kInit);
//   TF_LITE_ENSURE_STATUS(graph_.InitSubgraphs());

//   micro_context_.SetInterpreterState(
//       MicroInterpreterContext::InterpreterState::kPrepare);

//   TF_LITE_ENSURE_STATUS(graph_.PrepareSubgraphs());

//   micro_context_.SetInterpreterState(
//       MicroInterpreterContext::InterpreterState::kMemoryPlanning);

//   TF_LITE_ENSURE_OK(&context_, allocator_.FinishModelAllocation(
//                                    model_, graph_.GetAllocations(),
//                                    &scratch_buffer_handles_));

//   micro_context_.SetScratchBufferHandles(scratch_buffer_handles_);

//   // TODO(b/162311891): Drop these allocations when the interpreter supports
//   // handling buffers from TfLiteEvalTensor.
//   input_tensors_ =
//       reinterpret_cast<TfLiteTensor**>(allocator_.AllocatePersistentBuffer(
//           sizeof(TfLiteTensor*) * inputs_size()));
//   if (input_tensors_ == nullptr) {
//     MicroPrintf(
//         "Failed to allocate memory for context->input_tensors_, "
//         "%d bytes required",
//         sizeof(TfLiteTensor*) * inputs_size());
//     return kTfLiteError;
//   }

//   for (size_t i = 0; i < inputs_size(); ++i) {
//     input_tensors_[i] = allocator_.AllocatePersistentTfLiteTensor(
//         model_, graph_.GetAllocations(), inputs().Get(i), 0);
//     if (input_tensors_[i] == nullptr) {
//       MicroPrintf("Failed to initialize input tensor %d", i);
//       return kTfLiteError;
//     }
//   }

//   // TODO(b/162311891): Drop these allocations when the interpreter supports
//   // handling buffers from TfLiteEvalTensor.
//   output_tensors_ =
//       reinterpret_cast<TfLiteTensor**>(allocator_.AllocatePersistentBuffer(
//           sizeof(TfLiteTensor*) * outputs_size()));
//   if (output_tensors_ == nullptr) {
//     MicroPrintf(
//         "Failed to allocate memory for context->output_tensors_, "
//         "%d bytes required",
//         sizeof(TfLiteTensor*) * outputs_size());
//     return kTfLiteError;
//   }

//   for (size_t i = 0; i < outputs_size(); ++i) {
//     output_tensors_[i] = allocator_.AllocatePersistentTfLiteTensor(
//         model_, graph_.GetAllocations(), outputs().Get(i), 0);
//     if (output_tensors_[i] == nullptr) {
//       MicroPrintf("Failed to initialize output tensor %d", i);
//       return kTfLiteError;
//     }
//   }

//   TF_LITE_ENSURE_STATUS(Reset());

//   tensors_allocated_ = true;
//   micro_context_.SetInterpreterState(
//       MicroInterpreterContext::InterpreterState::kInvoke);
//   return kTfLiteOk;
// }

TfLiteStatus MicroInterpreter::AllocateTensors() {

  SubgraphAllocations* allocations = allocator_.StartModelAllocationDirect(model_);


  if (allocations == nullptr) {
    MicroPrintf("Failed starting model allocation.\n");
    initialization_status_ = kTfLiteError;
    return kTfLiteError;
  }

  graph_.SetSubgraphAllocations(allocations);

  TF_LITE_ENSURE_STATUS(PrepareNodeAndRegistrationDataFromFlatbuffer());


  // micro_context_.SetInterpreterState(
  //     MicroInterpreterContext::InterpreterState::kInit);
  // TF_LITE_ENSURE_STATUS(graph_.InitSubgraphs());
  // RawPutc('['); RawPutc('T'); RawPutc('A'); RawPutc('4'); RawPutc(']');
  // RawNewline();

  micro_context_.SetInterpreterState(
      MicroInterpreterContext::InterpreterState::kInit);



  TfLiteStatus init_status = graph_.InitSubgraphs();



  TF_LITE_ENSURE_STATUS(init_status);



  micro_context_.SetInterpreterState(
      MicroInterpreterContext::InterpreterState::kPrepare);
  TF_LITE_ENSURE_STATUS(graph_.PrepareSubgraphs());

  micro_context_.SetInterpreterState(
      MicroInterpreterContext::InterpreterState::kMemoryPlanning);

  TF_LITE_ENSURE_OK(&context_,
      FinishModelAllocationNoInline(allocator_, model_,
                                    graph_.GetAllocations(),
                                    &scratch_buffer_handles_));


  micro_context_.SetScratchBufferHandles(scratch_buffer_handles_);

  input_tensors_ =
      reinterpret_cast<TfLiteTensor**>(allocator_.AllocatePersistentBuffer(
          sizeof(TfLiteTensor*) * inputs_size()));


  if (input_tensors_ == nullptr) {
    MicroPrintf("Failed to allocate memory for context->input_tensors_\n");
    return kTfLiteError;
  }

  for (size_t i = 0; i < inputs_size(); ++i) {


    input_tensors_[i] = AllocatePersistentTfLiteTensorNoInline(
        allocator_, model_, graph_.GetAllocations(), inputs().Get(i), 0);



    if (input_tensors_[i] == nullptr) {
      MicroPrintf("Failed to initialize input tensor %d", i);
      return kTfLiteError;
    }
  }

  output_tensors_ =
      reinterpret_cast<TfLiteTensor**>(allocator_.AllocatePersistentBuffer(
          sizeof(TfLiteTensor*) * outputs_size()));


  if (output_tensors_ == nullptr) {
    MicroPrintf("Failed to allocate memory for context->output_tensors_\n");
    return kTfLiteError;
  }

  for (size_t i = 0; i < outputs_size(); ++i) {


    output_tensors_[i] = AllocatePersistentTfLiteTensorNoInline(
        allocator_, model_, graph_.GetAllocations(), outputs().Get(i), 0);



    if (output_tensors_[i] == nullptr) {
      MicroPrintf("Failed to initialize output tensor %d", i);
      return kTfLiteError;
    }
  }

  TF_LITE_ENSURE_STATUS(Reset());


  tensors_allocated_ = true;
  micro_context_.SetInterpreterState(
      MicroInterpreterContext::InterpreterState::kInvoke);
  return kTfLiteOk;
}



///////////////////////////////////////////////

// TfLiteStatus MicroInterpreter::Invoke() {
//   if (initialization_status_ != kTfLiteOk) {
//     MicroPrintf("Invoke() called after initialization failed\n");
//     return kTfLiteError;
//   }

//   // Ensure tensors are allocated before the interpreter is invoked to avoid
//   // difficult to debug segfaults.
//   if (!tensors_allocated_) {
//     TF_LITE_ENSURE_OK(&context_, AllocateTensors());
//   }
//   return graph_.InvokeSubgraph(0);
// }

TfLiteStatus MicroInterpreter::Invoke() {


  if (initialization_status_ != kTfLiteOk) {

    MicroPrintf("Invoke() called after initialization failed\n");
    return kTfLiteError;
  }

  // Ensure tensors are allocated before the interpreter is invoked.
  if (!tensors_allocated_) {
  
    TfLiteStatus alloc_status = AllocateTensors();


    if (alloc_status != kTfLiteOk) {
      return alloc_status;
    }
  }



  TfLiteStatus invoke_status = graph_.InvokeSubgraph(0);


  return invoke_status;
}

TfLiteTensor* MicroInterpreter::input(size_t index) {
  const size_t length = inputs_size();
  if (index >= length) {
    MicroPrintf("Input index %d out of range (length is %d)", index, length);
    return nullptr;
  }
  return input_tensors_[index];
}

TfLiteTensor* MicroInterpreter::output(size_t index) {
  const size_t length = outputs_size();
  if (index >= length) {
    MicroPrintf("Output index %d out of range (length is %d)", index, length);
    return nullptr;
  }
  return output_tensors_[index];
}

TfLiteStatus MicroInterpreter::Reset() {
  TfLiteStatus status = graph_.ResetSubgraphs();
  if (status != kTfLiteOk) {
    return status;
  }
  return graph_.ResetVariableTensors();
}

TfLiteEvalTensor* MicroInterpreter::GetTensor(int tensor_index,
                                              int subgraph_index) {
  if (!allocator_.preserves_all_tensor()) {
    MicroPrintf("GetTensor requires all tensors to be preserved");
    return nullptr;
  }
  return &graph_.GetAllocations()[subgraph_index].tensors[tensor_index];
}

TfLiteStatus MicroInterpreter::ResetVariableTensor(int tensor_index,
                                                   int subgraph_index) {
  return graph_.ResetVariableTensor(tensor_index, subgraph_index);
}

TfLiteStatus MicroInterpreter::SetMicroExternalContext(
    void* external_context_payload) {
  return micro_context_.set_external_context(external_context_payload);
}

TfLiteStatus MicroInterpreter::SetAlternateProfiler(
    MicroProfilerInterface* alt_profiler) {
  return micro_context_.SetAlternateProfiler(alt_profiler);
}

TfLiteStatus MicroInterpreter::SetDecompressionMemory(
    const MicroContext::AlternateMemoryRegion* regions, size_t count) {
  return micro_context_.SetDecompressionMemory(regions, count);
}

}  // namespace tflite
