// #include "tensorflow/lite/micro/examples/person_detection/main_functions.h"

// #include "tensorflow/lite/micro/examples/person_detection/detection_responder.h"
// #include "tensorflow/lite/micro/examples/person_detection/image_provider.h"
// #include "tensorflow/lite/micro/examples/person_detection/model_settings.h"
// #include "tensorflow/lite/micro/micro_interpreter.h"
// #include "tensorflow/lite/micro/micro_log.h"
// #include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
// #include "tensorflow/lite/micro/system_setup.h"
// #include "tensorflow/lite/schema/schema_generated.h"

// #include "tensorflow/lite/micro/models/person_detect_model_data.h"

// #include <new>
// #include <stddef.h>
// #include <stdint.h>

// static inline void RawPutc(char c) {
//   volatile uint32_t* const uart_tx =
//       reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
//   volatile uint32_t* const uart_status =
//       reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x08u);

//   while ((*uart_status) & 0x08u) {
//   }

//   *uart_tx = static_cast<uint32_t>(static_cast<uint8_t>(c));
// }

// static inline void RawPuts(const char* s) {
//   while (*s) {
//     RawPutc(*s++);
//   }
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

// static inline uint32_t RawSubPower10(uint32_t* value, uint32_t power10) {
//   uint32_t digit = 0;
//   while (*value >= power10) {
//     *value -= power10;
//     ++digit;
//   }
//   return digit;
// }

// static inline void RawPutUnsignedDec32_NoDiv(uint32_t v) {
//   static const uint32_t powers10[] = {
//       1000000000u,
//       100000000u,
//       10000000u,
//       1000000u,
//       100000u,
//       10000u,
//       1000u,
//       100u,
//       10u,
//       1u,
//   };

//   bool started = false;

//   for (int i = 0; i < 10; ++i) {
//     uint32_t digit = RawSubPower10(&v, powers10[i]);

//     if (digit != 0 || started || powers10[i] == 1u) {
//       RawPutc(static_cast<char>('0' + digit));
//       started = true;
//     }
//   }
// }

// static inline void RawPutSignedDec32_NoDiv(int32_t v) {
//   if (v < 0) {
//     RawPutc('-');

//     // Safe for INT32_MIN.
//     uint32_t mag = static_cast<uint32_t>(-(v + 1)) + 1u;
//     RawPutUnsignedDec32_NoDiv(mag);
//   } else {
//     RawPutUnsignedDec32_NoDiv(static_cast<uint32_t>(v));
//   }
// }

// static inline void RawTagDec_NoDiv(const char* tag, int32_t v) {
//   RawPuts(tag);
//   RawPutSignedDec32_NoDiv(v);
//   RawPutc(' ');
// }

// static const char* TfLiteTypeNameSimple(TfLiteType type) {
//   switch (type) {
//     case kTfLiteInt8: return "int8";
//     case kTfLiteUInt8: return "uint8";
//     case kTfLiteInt16: return "int16";
//     case kTfLiteInt32: return "int32";
//     case kTfLiteFloat32: return "float32";
//     case kTfLiteBool: return "bool";
//     default: return "other";
//   }
// }

// static void PrintDims(const TfLiteIntArray* dims) {
//   if (dims == nullptr) {
//     MicroPrintf("    dims: null");
//     return;
//   }

//   MicroPrintf("    dims count: %d", dims->size);

//   for (int i = 0; i < dims->size; i++) {
//     MicroPrintf("      dim[%d] = %d", i, dims->data[i]);
//   }
// }

// static void DumpTensorAddresses(tflite::MicroInterpreter* interpreter,
//                                 const tflite::Model* model,
//                                 uint8_t* tensor_arena,
//                                 size_t tensor_arena_size) {
//   const tflite::SubGraph* subgraph = model->subgraphs()->Get(0);
//   const auto* tensors = subgraph->tensors();

//   const uint32_t model_base =
//       static_cast<uint32_t>(reinterpret_cast<uintptr_t>(g_person_detect_model_data));

//   const uint32_t arena_base =
//       static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tensor_arena));

//   const uint32_t arena_end =
//       static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tensor_arena + tensor_arena_size));

//   MicroPrintf("");
//   MicroPrintf("========== TENSOR ADDRESS DUMP ==========");
//   MicroPrintf("tensor count = %d", tensors->size());
//   MicroPrintf("");

//   for (int i = 0; i < tensors->size(); i++) {
//     TfLiteEvalTensor* rt = interpreter->GetTensor(i);
//     const tflite::Tensor* fb = tensors->Get(i);

//     const uint32_t addr =
//         static_cast<uint32_t>(reinterpret_cast<uintptr_t>(rt->data.raw));

//     const uint32_t bytes =
//         static_cast<uint32_t>(rt->bytes);

//     const uint32_t buffer_index =
//         static_cast<uint32_t>(fb->buffer());

//     const char* location = "unknown";

//     if (addr >= arena_base && addr < arena_end) {
//       location = "tensor_arena / activation";
//     } else if (addr >= model_base && addr < arena_base) {
//       location = "model_blob / constant";
//     } else if (addr == 0) {
//       location = "null";
//     }

//     MicroPrintf("--------------------------------------");
//     MicroPrintf("TENSOR %d", i);
//     MicroPrintf("    addr        = 0x%08X", addr);
//     MicroPrintf("    bytes       = %u", bytes);
//     MicroPrintf("    type        = %s", TfLiteTypeNameSimple(rt->type));
//     MicroPrintf("    buffer idx  = %u", buffer_index);
//     MicroPrintf("    location    = %s", location);

//     PrintDims(rt->dims);
//   }

//   MicroPrintf("========== END TENSOR DUMP ==========");
//   MicroPrintf("");
// }

// static void DumpOperatorTensorMap(const tflite::Model* model) {
//   const tflite::SubGraph* subgraph = model->subgraphs()->Get(0);
//   const auto* operators = subgraph->operators();

//   MicroPrintf("");
//   MicroPrintf("========== OPERATOR TENSOR MAP ==========");
//   MicroPrintf("operator count = %d", operators->size());
//   MicroPrintf("");

//   for (int op_i = 0; op_i < operators->size(); op_i++) {
//     const tflite::Operator* op = operators->Get(op_i);

//     MicroPrintf("--------------------------------------");
//     MicroPrintf("OP %d", op_i);

//     const auto* inputs = op->inputs();
//     const auto* outputs = op->outputs();

//     MicroPrintf("  inputs:");

//     for (int j = 0; j < inputs->size(); j++) {
//       const int tensor_id = inputs->Get(j);

//       if (j == 0) {
//         MicroPrintf("    input activation : T%d", tensor_id);
//       } else if (j == 1) {
//         MicroPrintf("    weights/kernel   : T%d", tensor_id);
//       } else if (j == 2) {
//         MicroPrintf("    bias             : T%d", tensor_id);
//       } else {
//         MicroPrintf("    extra input[%d]   : T%d", j, tensor_id);
//       }
//     }

//     MicroPrintf("  outputs:");

//     for (int j = 0; j < outputs->size(); j++) {
//       MicroPrintf("    output[%d]        : T%d", j, outputs->Get(j));
//     }
//   }

//   MicroPrintf("========== END OPERATOR MAP ==========");
//   MicroPrintf("");
// }

// namespace {
// const tflite::Model* model = nullptr;
// tflite::MicroInterpreter* interpreter = nullptr;
// TfLiteTensor* input = nullptr;

// constexpr int kTensorArenaSize = 136 * 1024;

// alignas(16) __attribute__((section(".arena")))
// static uint8_t tensor_arena[kTensorArenaSize];

// alignas(tflite::MicroInterpreter)
// static uint8_t interpreter_buf[sizeof(tflite::MicroInterpreter)];

// tflite::MicroMutableOpResolver<5> resolver;
// }  // namespace

// __attribute__((noinline))
// static tflite::MicroInterpreter* CreateInterpreterNoInline(
//     void* buf,
//     const tflite::Model* model_ptr,
//     tflite::MicroOpResolver& resolver_ref,
//     uint8_t* arena_ptr,
//     size_t arena_size) {
//   RawTagHex('a', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(model_ptr)));
//   RawTagHex('b', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&resolver_ref)));
//   RawTagHex('c', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(arena_ptr)));
//   RawTagHex('d', static_cast<uint32_t>(arena_size));
//   RawNewline();

//   return new (buf) tflite::MicroInterpreter(
//       model_ptr, resolver_ref, arena_ptr, arena_size);
// }

// void setup() {
//   RawPuts("START!");
//   RawNewline();
//   MicroPrintf("This is Microprintf!\n");
//   RawPutc('1');
//   tflite::InitializeTarget();
//   RawPutc('2');
//   RawNewline();
//   MicroPrintf("A=%08x S=%08x\n",
//               static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tensor_arena)),
//               static_cast<uint32_t>(kTensorArenaSize));
//   RawTagHex('A', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tensor_arena)));
//   RawTagHex('S', static_cast<uint32_t>(kTensorArenaSize));
//   RawNewline();

//   RawTagHex('B', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(interpreter_buf)));
//   RawNewline();

//   model = tflite::GetModel(g_person_detect_model_data);
//   RawPutc('m');
//   RawTagHex('M', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(model)));
//   RawNewline();

//   if (model == nullptr) {
//     RawPutc('X');
//     RawNewline();
//     return;
//   }

//   RawTagHex('v', static_cast<uint32_t>(model->version()));
//   RawTagHex('t', static_cast<uint32_t>(TFLITE_SCHEMA_VERSION));
//   if (model->version() != TFLITE_SCHEMA_VERSION) {
//     RawPutc('V');
//   } else {
//     RawPutc('v');
//   }
//   RawNewline();

//   RawPutc('C');
//   RawNewline();

//   const auto* subgraphs = model->subgraphs();
//   RawTagHex('s', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(subgraphs)));
//   if (subgraphs == nullptr) {
//     RawPutc('S');
//     RawNewline();
//     return;
//   }

//   RawTagHex('T', static_cast<uint32_t>(subgraphs->size()));
//   RawNewline();

//   const auto* sg0 = (subgraphs->size() > 0) ? subgraphs->Get(0) : nullptr;
//   RawTagHex('u', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(sg0)));
//   if (sg0 == nullptr) {
//     RawPutc('U');
//     RawNewline();
//     return;
//   }
//   RawNewline();

//   const auto* inputs = sg0->inputs();
//   RawTagHex('w', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(inputs)));
//   if (inputs == nullptr) {
//     RawPutc('W');
//     RawNewline();
//     return;
//   }

//   RawTagHex('i', static_cast<uint32_t>(inputs->size()));
//   RawNewline();

//   const auto* outputs = sg0->outputs();
//   RawTagHex('o', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(outputs)));
//   if (outputs != nullptr) {
//     RawTagHex('O', static_cast<uint32_t>(outputs->size()));
//   }
//   RawNewline();

//   const auto* operators = sg0->operators();
//   RawTagHex('r', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(operators)));
//   if (operators != nullptr) {
//     RawTagHex('R', static_cast<uint32_t>(operators->size()));
//   }
//   RawNewline();

//   RawPutc('q');
//   RawNewline();

//   RawPutc('u');
//   resolver.AddReshape();
//   RawPutc('v');
//   RawNewline();

//   RawPutc('w');
//   auto softmax_reg = tflite::Register_SOFTMAX_INT8();
//   resolver.AddSoftmax(softmax_reg);
//   RawPutc('x');
//   RawNewline();

//   RawPutc('y');
//   auto avg_reg = tflite::Register_AVERAGE_POOL_2D_INT8();
//   resolver.AddAveragePool2D(avg_reg);
//   RawPutc('z');
//   RawNewline();

//   RawPutc('J');
//   auto conv_reg = tflite::Register_CONV_2D_INT8();
//   resolver.AddConv2D(conv_reg);
//   RawPutc('K');
//   RawNewline();

//   auto dw_reg = tflite::Register_DEPTHWISE_CONV_2D_INT8();
//   resolver.AddDepthwiseConv2D(dw_reg);
//   RawPutc('L');
//   RawNewline();

//   const tflite::Model* model_ptr = model;
//   tflite::MicroOpResolver& resolver_ref = resolver;
//   uint8_t* arena_ptr = tensor_arena;
//   const size_t arena_size = kTensorArenaSize;

//   RawTagHex('P', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(arena_ptr)));
//   RawTagHex('Z', static_cast<uint32_t>(arena_size));
//   RawNewline();

//   RawPutc('M');
//   RawNewline();

//   interpreter = CreateInterpreterNoInline(
//       interpreter_buf, model_ptr, resolver_ref, arena_ptr, arena_size);

//   RawPutc('N');
//   RawTagHex('I', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(interpreter)));
//   RawNewline();

//   // TfLiteStatus alloc_status = interpreter->AllocateTensors();
//   // RawTagHex('G', static_cast<uint32_t>(alloc_status));
//   // RawNewline();

//   RawPutc('['); RawPutc('A'); RawPutc('T'); RawPutc('0'); RawPutc(']');
//   RawTagHex('i', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(interpreter)));
//   RawNewline();

//   TfLiteStatus alloc_status = interpreter->AllocateTensors();

//   RawPutc('['); RawPutc('A'); RawPutc('T'); RawPutc('1'); RawPutc(']');
//   RawTagHex('g', static_cast<uint32_t>(alloc_status));
//   RawNewline();

//   if (alloc_status != kTfLiteOk) {
//     RawPutc('F');
//     RawNewline();
//     return;
//   }


//   MicroPrintf("AllocateTensors OK");
//   MicroPrintf("");
//   MicroPrintf("========== MODEL MEMORY MAP ==========");

//   MicroPrintf("model blob base        = 0x%08X",
//               static_cast<uint32_t>(
//                   reinterpret_cast<uintptr_t>(g_person_detect_model_data)));

//   MicroPrintf("tensor_arena base      = 0x%08X",
//               static_cast<uint32_t>(
//                   reinterpret_cast<uintptr_t>(tensor_arena)));

//   MicroPrintf("tensor_arena size      = %u bytes",
//               static_cast<uint32_t>(kTensorArenaSize));

//   MicroPrintf("tensor_arena end       = 0x%08X",
//               static_cast<uint32_t>(
//                   reinterpret_cast<uintptr_t>(tensor_arena + kTensorArenaSize)));

//   MicroPrintf("======================================");
//   MicroPrintf("");

//   DumpTensorAddresses(interpreter, model, tensor_arena, kTensorArenaSize);
//   DumpOperatorTensorMap(model);

//   TfLiteTensor* tmp_input = interpreter->input(0);
//   RawTagHex('H', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tmp_input)));
//   RawNewline();

//   input = tmp_input;
//   if (input == nullptr) {
//     RawPutc('H');
//     RawNewline();
//     while (1) {
//     }
//   }

//   RawPutc('I');
//   RawTagHex('d', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(input->dims)));
//   RawTagHex('b', static_cast<uint32_t>(input->bytes));
//   RawTagHex('y', static_cast<uint32_t>(input->type));
//   RawTagHex('D', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(input->data.data)));
//   RawNewline();

//   if (input->dims != nullptr) {
//     RawTagHex('0', static_cast<uint32_t>(input->dims->size));
//     if (input->dims->size > 0) RawTagHex('1', static_cast<uint32_t>(input->dims->data[0]));
//     if (input->dims->size > 1) RawTagHex('2', static_cast<uint32_t>(input->dims->data[1]));
//     if (input->dims->size > 2) RawTagHex('3', static_cast<uint32_t>(input->dims->data[2]));
//     if (input->dims->size > 3) RawTagHex('4', static_cast<uint32_t>(input->dims->data[3]));
//     RawNewline();
//   } else {
//     RawPutc('D');
//     RawNewline();
//   }

//   TfLiteTensor* tmp_output = interpreter->output(0);
//   RawTagHex('Q', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tmp_output)));
//   RawNewline();

//   if (tmp_output != nullptr) {
//     RawTagHex('q', static_cast<uint32_t>(tmp_output->type));
//     RawTagHex('e', static_cast<uint32_t>(tmp_output->bytes));
//     RawTagHex('E', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tmp_output->dims)));
//     RawNewline();

//     if (tmp_output->dims != nullptr) {
//       RawTagHex('5', static_cast<uint32_t>(tmp_output->dims->size));
//       if (tmp_output->dims->size > 0) RawTagHex('6', static_cast<uint32_t>(tmp_output->dims->data[0]));
//       if (tmp_output->dims->size > 1) RawTagHex('7', static_cast<uint32_t>(tmp_output->dims->data[1]));
//       RawNewline();
//     }
//   }

//   RawTagHex('Y', static_cast<uint32_t>(
//                reinterpret_cast<uintptr_t>(tmp_output->data.data)));
// RawNewline();

//   RawPutc('Z');
//   RawNewline();
//   // while(1){}
// }



// void loop() {
//   RawPutc('['); RawPutc('L'); RawPutc('0'); RawPutc(']');
//   RawNewline();

//   if (input == nullptr || interpreter == nullptr) {
//     RawPutc('['); RawPutc('L'); RawPutc('E'); RawPutc('0'); RawPutc(']');
//     RawTagHex('i', reinterpret_cast<uintptr_t>(input));
//     RawTagHex('p', reinterpret_cast<uintptr_t>(interpreter));
//     RawNewline();
//     return;
//   }

//   RawPutc('['); RawPutc('L'); RawPutc('1'); RawPutc(']');
//   RawTagHex('D', reinterpret_cast<uintptr_t>(input->data.data));
//   RawTagHex('b', input->bytes);
//   RawTagHex('t', input->type);
//   RawNewline();

//   RawPutc('['); RawPutc('I'); RawPutc('M'); RawPutc('0'); RawPutc(']');
//   RawNewline();

//   TfLiteStatus img_status =
//       GetImage(kNumCols, kNumRows, kNumChannels, input->data.int8);

//   RawPutc('['); RawPutc('I'); RawPutc('M'); RawPutc('1'); RawPutc(']');
//   RawTagHex('s', img_status);
//   RawNewline();

//   if (img_status != kTfLiteOk) {
//     RawPutc('b');
//     RawNewline();
//     return;
//   }

//   // Print a few input bytes to prove GetImage actually wrote data.
//   RawPutc('['); RawPutc('I'); RawPutc('N'); RawPutc('0'); RawPutc(']');
//   for (int k = 0; k < 8; ++k) {
//     RawTagHex('x', static_cast<uint8_t>(input->data.int8[k]));
//   }
//   RawNewline();

//   RawPutc('['); RawPutc('V'); RawPutc('0'); RawPutc(']');
//   RawNewline();

//   TfLiteStatus invoke_status = interpreter->Invoke();

//   RawPutc('['); RawPutc('V'); RawPutc('1'); RawPutc(']');
//   RawTagHex('s', invoke_status);
//   RawNewline();

//   if (invoke_status != kTfLiteOk) {
//     RawPutc('c');
//     RawNewline();
//     return;
//   }

//   TfLiteTensor* output = interpreter->output(0);

//   RawPutc('['); RawPutc('O'); RawPutc('0'); RawPutc(']');
//   RawTagHex('o', reinterpret_cast<uintptr_t>(output));
//   RawNewline();

//   if (output == nullptr) {
//     RawPutc('d');
//     RawNewline();
//     return;
//   }

//   RawPutc('['); RawPutc('O'); RawPutc('1'); RawPutc(']');
//   RawTagHex('D', reinterpret_cast<uintptr_t>(output->data.data));
//   RawTagHex('b', output->bytes);
//   RawTagHex('t', output->type);
//   RawNewline();

//   // const int8_t person_score = output->data.int8[kPersonIndex];
//   // const int8_t no_person_score = output->data.int8[kNotAPersonIndex];

//   // RawPutc('['); RawPutc('O'); RawPutc('2'); RawPutc(']');
//   // RawTagHex('p', static_cast<uint8_t>(person_score));
//   // RawTagHex('n', static_cast<uint8_t>(no_person_score));
//   // RawNewline();

//   const int8_t person_score = output->data.int8[kPersonIndex];
//   const int8_t no_person_score = output->data.int8[kNotAPersonIndex];

//   RawPuts("[OUTPUT] ");
//   RawTagDec_NoDiv("person=", static_cast<int32_t>(person_score));
//   RawTagDec_NoDiv("no_person=", static_cast<int32_t>(no_person_score));

//   RawPuts("pred=");
//   if (person_score > no_person_score) {
//     RawPuts("PERSON");
//   } else {
//     RawPuts("NO_PERSON");
//   }

//   RawPuts(" margin=");
//   RawPutSignedDec32_NoDiv(static_cast<int32_t>(person_score) -
//                           static_cast<int32_t>(no_person_score));
//   RawNewline();

//   // RespondToDetection(person_score, no_person_score);

//   RawPuts("INFERENCE DONE! CONGRATULATIONS");
//   RawNewline();

//   while (1) {}
// }


#include "tensorflow/lite/micro/examples/person_detection/main_functions.h"

#include "tensorflow/lite/micro/examples/person_detection/detection_responder.h"
#include "tensorflow/lite/micro/examples/person_detection/image_provider.h"
#include "tensorflow/lite/micro/examples/person_detection/model_settings.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "tensorflow/lite/micro/models/person_detect_model_data.h"

#include <new>
#include <stddef.h>
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

static inline void RawPuts(const char* s) {
  while (*s) {
    RawPutc(*s++);
  }
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

static inline uint32_t RawSubPower10(uint32_t* value, uint32_t power10) {
  uint32_t digit = 0;
  while (*value >= power10) {
    *value -= power10;
    ++digit;
  }
  return digit;
}

static inline void RawPutUnsignedDec32_NoDiv(uint32_t v) {
  static const uint32_t powers10[] = {
      1000000000u,
      100000000u,
      10000000u,
      1000000u,
      100000u,
      10000u,
      1000u,
      100u,
      10u,
      1u,
  };

  bool started = false;

  for (int i = 0; i < 10; ++i) {
    uint32_t digit = RawSubPower10(&v, powers10[i]);

    if (digit != 0 || started || powers10[i] == 1u) {
      RawPutc(static_cast<char>('0' + digit));
      started = true;
    }
  }
}

static inline void RawPutSignedDec32_NoDiv(int32_t v) {
  if (v < 0) {
    RawPutc('-');

    // Safe for INT32_MIN.
    uint32_t mag = static_cast<uint32_t>(-(v + 1)) + 1u;
    RawPutUnsignedDec32_NoDiv(mag);
  } else {
    RawPutUnsignedDec32_NoDiv(static_cast<uint32_t>(v));
  }
}

static inline void RawTagDec_NoDiv(const char* tag, int32_t v) {
  RawPuts(tag);
  RawPutSignedDec32_NoDiv(v);
  RawPutc(' ');
}

// -----------------------------------------------------------------------------
// FlatBuffer tensor helpers.
// Your interpreter->GetTensor(i) returns TfLiteEvalTensor*.
// TfLiteEvalTensor has runtime data pointer, but not full bytes/dims/type.
// So bytes/dims/type are read from the model FlatBuffer tensor metadata.
// -----------------------------------------------------------------------------

static uint32_t FlatbufferTypeSize(tflite::TensorType type) {
  switch (type) {
    case tflite::TensorType_INT8: return 1;
    case tflite::TensorType_UINT8: return 1;
    case tflite::TensorType_INT16: return 2;
    case tflite::TensorType_UINT16: return 2;
    case tflite::TensorType_INT32: return 4;
    case tflite::TensorType_UINT32: return 4;
    case tflite::TensorType_FLOAT32: return 4;
    case tflite::TensorType_BOOL: return 1;
    default: return 1;
  }
}

static const char* FlatbufferTypeName(tflite::TensorType type) {
  switch (type) {
    case tflite::TensorType_INT8: return "int8";
    case tflite::TensorType_UINT8: return "uint8";
    case tflite::TensorType_INT16: return "int16";
    case tflite::TensorType_UINT16: return "uint16";
    case tflite::TensorType_INT32: return "int32";
    case tflite::TensorType_UINT32: return "uint32";
    case tflite::TensorType_FLOAT32: return "float32";
    case tflite::TensorType_BOOL: return "bool";
    default: return "other";
  }
}

static uint32_t FlatbufferTensorBytes(const tflite::Tensor* tensor) {
  if (tensor == nullptr) {
    return 0;
  }

  const auto* shape = tensor->shape();

  if (shape == nullptr) {
    return 0;
  }

  uint32_t elements = 1;
  const uint32_t dims_count = static_cast<uint32_t>(shape->size());

  for (uint32_t i = 0; i < dims_count; i++) {
    const int dim = shape->Get(i);

    if (dim <= 0) {
      return 0;
    }

    elements *= static_cast<uint32_t>(dim);
  }

  return elements * FlatbufferTypeSize(tensor->type());
}

static void PrintFlatbufferDims(const tflite::Tensor* tensor) {
  if (tensor == nullptr) {
    MicroPrintf("    dims        = null tensor");
    return;
  }

  const auto* shape = tensor->shape();

  if (shape == nullptr) {
    MicroPrintf("    dims        = null");
    return;
  }

  const uint32_t dims_count = static_cast<uint32_t>(shape->size());

  MicroPrintf("    dims count  = %u", dims_count);

  for (uint32_t i = 0; i < dims_count; i++) {
    MicroPrintf("      dim[%u] = %d", i, shape->Get(i));
  }
}

static void DumpTensorAddresses(tflite::MicroInterpreter* interpreter,
                                const tflite::Model* model,
                                uint8_t* tensor_arena,
                                size_t tensor_arena_size) {
  if (interpreter == nullptr || model == nullptr) {
    MicroPrintf("DumpTensorAddresses: null interpreter/model");
    return;
  }

  const tflite::SubGraph* subgraph = model->subgraphs()->Get(0);

  if (subgraph == nullptr) {
    MicroPrintf("DumpTensorAddresses: subgraph null");
    return;
  }

  const auto* tensors = subgraph->tensors();

  if (tensors == nullptr) {
    MicroPrintf("DumpTensorAddresses: tensors null");
    return;
  }

  const uint32_t model_base =
      static_cast<uint32_t>(reinterpret_cast<uintptr_t>(g_person_detect_model_data));

  const uint32_t arena_base =
      static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tensor_arena));

  const uint32_t arena_end =
      static_cast<uint32_t>(
          reinterpret_cast<uintptr_t>(tensor_arena + tensor_arena_size));

  const uint32_t tensor_count = static_cast<uint32_t>(tensors->size());

 
}

static void DumpOperatorTensorMap(const tflite::Model* model) {
  if (model == nullptr) {
    MicroPrintf("DumpOperatorTensorMap: model null");
    return;
  }

  const tflite::SubGraph* subgraph = model->subgraphs()->Get(0);

  if (subgraph == nullptr) {
    MicroPrintf("DumpOperatorTensorMap: subgraph null");
    return;
  }

  const auto* operators = subgraph->operators();

  if (operators == nullptr) {
    MicroPrintf("DumpOperatorTensorMap: operators null");
    return;
  }

  const uint32_t operator_count = static_cast<uint32_t>(operators->size());

  MicroPrintf("");
  MicroPrintf("========== OPERATOR TENSOR MAP ==========");
  MicroPrintf("operator count = %u", operator_count);
  MicroPrintf("");

  for (uint32_t op_i = 0; op_i < operator_count; op_i++) {
    const tflite::Operator* op = operators->Get(op_i);

    MicroPrintf("--------------------------------------");
    MicroPrintf("OP %u", op_i);

    if (op == nullptr) {
      MicroPrintf("  ERROR: op null");
      continue;
    }

    const auto* inputs = op->inputs();
    const auto* outputs = op->outputs();

    MicroPrintf("  inputs:");

    if (inputs == nullptr) {
      MicroPrintf("    null");
    } else {
      const uint32_t input_count = static_cast<uint32_t>(inputs->size());

      for (uint32_t j = 0; j < input_count; j++) {
        const int tensor_id = inputs->Get(j);

        if (j == 0) {
          MicroPrintf("    input activation : T%d", tensor_id);
        } else if (j == 1) {
          MicroPrintf("    weights/kernel   : T%d", tensor_id);
        } else if (j == 2) {
          MicroPrintf("    bias             : T%d", tensor_id);
        } else {
          MicroPrintf("    extra input[%u]   : T%d", j, tensor_id);
        }
      }
    }

    MicroPrintf("  outputs:");

    if (outputs == nullptr) {
      MicroPrintf("    null");
    } else {
      const uint32_t output_count = static_cast<uint32_t>(outputs->size());

      for (uint32_t j = 0; j < output_count; j++) {
        MicroPrintf("    output[%u]        : T%d", j, outputs->Get(j));
      }
    }
  }

  MicroPrintf("========== END OPERATOR MAP ==========");
  MicroPrintf("");
}

namespace {
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;

constexpr int kTensorArenaSize = 136 * 1024;

alignas(16) __attribute__((section(".arena")))
static uint8_t tensor_arena[kTensorArenaSize];

alignas(tflite::MicroInterpreter)
static uint8_t interpreter_buf[sizeof(tflite::MicroInterpreter)];

tflite::MicroMutableOpResolver<5> resolver;
}  // namespace

__attribute__((noinline))
static tflite::MicroInterpreter* CreateInterpreterNoInline(
    void* buf,
    const tflite::Model* model_ptr,
    tflite::MicroOpResolver& resolver_ref,
    uint8_t* arena_ptr,
    size_t arena_size) {
  RawTagHex('a', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(model_ptr)));
  RawTagHex('b', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&resolver_ref)));
  RawTagHex('c', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(arena_ptr)));
  RawTagHex('d', static_cast<uint32_t>(arena_size));
  RawNewline();

  return new (buf) tflite::MicroInterpreter(
      model_ptr, resolver_ref, arena_ptr, arena_size);
}

void setup() {
  RawPuts("START!");
  RawNewline();

  MicroPrintf("This is Microprintf!");

  RawPutc('1');
  tflite::InitializeTarget();
  RawPutc('2');
  RawNewline();

  MicroPrintf("A=%08x S=%08x",
              static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tensor_arena)),
              static_cast<uint32_t>(kTensorArenaSize));

  RawTagHex('A', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tensor_arena)));
  RawTagHex('S', static_cast<uint32_t>(kTensorArenaSize));
  RawNewline();

  RawTagHex('B', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(interpreter_buf)));
  RawNewline();

  model = tflite::GetModel(g_person_detect_model_data);

  RawPutc('m');
  RawTagHex('M', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(model)));
  RawNewline();

  if (model == nullptr) {
    RawPutc('X');
    RawNewline();
    return;
  }

  RawTagHex('v', static_cast<uint32_t>(model->version()));
  RawTagHex('t', static_cast<uint32_t>(TFLITE_SCHEMA_VERSION));

  if (model->version() != TFLITE_SCHEMA_VERSION) {
    RawPutc('V');
  } else {
    RawPutc('v');
  }

  RawNewline();

  RawPutc('C');
  RawNewline();

  const auto* subgraphs = model->subgraphs();

  RawTagHex('s', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(subgraphs)));

  if (subgraphs == nullptr) {
    RawPutc('S');
    RawNewline();
    return;
  }

  RawTagHex('T', static_cast<uint32_t>(subgraphs->size()));
  RawNewline();

  const auto* sg0 = (subgraphs->size() > 0) ? subgraphs->Get(0) : nullptr;

  RawTagHex('u', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(sg0)));

  if (sg0 == nullptr) {
    RawPutc('U');
    RawNewline();
    return;
  }

  RawNewline();

  const auto* inputs = sg0->inputs();

  RawTagHex('w', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(inputs)));

  if (inputs == nullptr) {
    RawPutc('W');
    RawNewline();
    return;
  }

  RawTagHex('i', static_cast<uint32_t>(inputs->size()));
  RawNewline();

  const auto* outputs = sg0->outputs();

  RawTagHex('o', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(outputs)));

  if (outputs != nullptr) {
    RawTagHex('O', static_cast<uint32_t>(outputs->size()));
  }

  RawNewline();

  const auto* operators = sg0->operators();

  RawTagHex('r', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(operators)));

  if (operators != nullptr) {
    RawTagHex('R', static_cast<uint32_t>(operators->size()));
  }

  RawNewline();

  RawPutc('q');
  RawNewline();

  RawPutc('u');
  resolver.AddReshape();
  RawPutc('v');
  RawNewline();

  RawPutc('w');
  auto softmax_reg = tflite::Register_SOFTMAX_INT8();
  resolver.AddSoftmax(softmax_reg);
  RawPutc('x');
  RawNewline();

  RawPutc('y');
  auto avg_reg = tflite::Register_AVERAGE_POOL_2D_INT8();
  resolver.AddAveragePool2D(avg_reg);
  RawPutc('z');
  RawNewline();

  RawPutc('J');
  auto conv_reg = tflite::Register_CONV_2D_INT8();
  resolver.AddConv2D(conv_reg);
  RawPutc('K');
  RawNewline();

  auto dw_reg = tflite::Register_DEPTHWISE_CONV_2D_INT8();
  resolver.AddDepthwiseConv2D(dw_reg);
  RawPutc('L');
  RawNewline();

  const tflite::Model* model_ptr = model;
  tflite::MicroOpResolver& resolver_ref = resolver;
  uint8_t* arena_ptr = tensor_arena;
  const size_t arena_size = kTensorArenaSize;

  RawTagHex('P', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(arena_ptr)));
  RawTagHex('Z', static_cast<uint32_t>(arena_size));
  RawNewline();

  RawPutc('M');
  RawNewline();

  interpreter = CreateInterpreterNoInline(
      interpreter_buf, model_ptr, resolver_ref, arena_ptr, arena_size);

  RawPutc('N');
  RawTagHex('I', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(interpreter)));
  RawNewline();

  RawPutc('['); RawPutc('A'); RawPutc('T'); RawPutc('0'); RawPutc(']');
  RawTagHex('i', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(interpreter)));
  RawNewline();

  TfLiteStatus alloc_status = interpreter->AllocateTensors();

  RawPutc('['); RawPutc('A'); RawPutc('T'); RawPutc('1'); RawPutc(']');
  RawTagHex('g', static_cast<uint32_t>(alloc_status));
  RawNewline();

  if (alloc_status != kTfLiteOk) {
    RawPutc('F');
    RawNewline();
    return;
  }

  MicroPrintf("AllocateTensors OK");
  // MicroPrintf("");
  // MicroPrintf("========== MODEL MEMORY MAP ==========");

  // MicroPrintf("model blob base        = 0x%08X",
  //             static_cast<uint32_t>(
  //                 reinterpret_cast<uintptr_t>(g_person_detect_model_data)));

  // MicroPrintf("tensor_arena base      = 0x%08X",
  //             static_cast<uint32_t>(
  //                 reinterpret_cast<uintptr_t>(tensor_arena)));

  // MicroPrintf("tensor_arena size      = %u bytes",
  //             static_cast<uint32_t>(kTensorArenaSize));

  // MicroPrintf("tensor_arena end       = 0x%08X",
  //             static_cast<uint32_t>(
  //                 reinterpret_cast<uintptr_t>(tensor_arena + kTensorArenaSize)));

  // MicroPrintf("======================================");
  // MicroPrintf("");

  // DumpTensorAddresses(interpreter, model, tensor_arena, kTensorArenaSize);
  // DumpOperatorTensorMap(model);

  TfLiteTensor* tmp_input = interpreter->input(0);

  RawTagHex('H', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tmp_input)));
  RawNewline();

  input = tmp_input;

  if (input == nullptr) {
    RawPutc('H');
    RawNewline();
    while (1) {
    }
  }

  RawPutc('I');
  RawTagHex('d', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(input->dims)));
  RawTagHex('b', static_cast<uint32_t>(input->bytes));
  RawTagHex('y', static_cast<uint32_t>(input->type));
  RawTagHex('D', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(input->data.data)));
  RawNewline();

  if (input->dims != nullptr) {
    RawTagHex('0', static_cast<uint32_t>(input->dims->size));

    if (input->dims->size > 0) {
      RawTagHex('1', static_cast<uint32_t>(input->dims->data[0]));
    }

    if (input->dims->size > 1) {
      RawTagHex('2', static_cast<uint32_t>(input->dims->data[1]));
    }

    if (input->dims->size > 2) {
      RawTagHex('3', static_cast<uint32_t>(input->dims->data[2]));
    }

    if (input->dims->size > 3) {
      RawTagHex('4', static_cast<uint32_t>(input->dims->data[3]));
    }

    RawNewline();
  } else {
    RawPutc('D');
    RawNewline();
  }

  TfLiteTensor* tmp_output = interpreter->output(0);

  RawTagHex('Q', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tmp_output)));
  RawNewline();

  if (tmp_output != nullptr) {
    RawTagHex('q', static_cast<uint32_t>(tmp_output->type));
    RawTagHex('e', static_cast<uint32_t>(tmp_output->bytes));
    RawTagHex('E', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tmp_output->dims)));
    RawNewline();

    if (tmp_output->dims != nullptr) {
      RawTagHex('5', static_cast<uint32_t>(tmp_output->dims->size));

      if (tmp_output->dims->size > 0) {
        RawTagHex('6', static_cast<uint32_t>(tmp_output->dims->data[0]));
      }

      if (tmp_output->dims->size > 1) {
        RawTagHex('7', static_cast<uint32_t>(tmp_output->dims->data[1]));
      }

      RawNewline();
    }

    RawTagHex('Y', static_cast<uint32_t>(
                   reinterpret_cast<uintptr_t>(tmp_output->data.data)));
    RawNewline();
  }

  RawPutc('Z');
  RawNewline();
}

void loop() {
  RawPutc('['); RawPutc('L'); RawPutc('0'); RawPutc(']');
  RawNewline();

  if (input == nullptr || interpreter == nullptr) {
    RawPutc('['); RawPutc('L'); RawPutc('E'); RawPutc('0'); RawPutc(']');
    RawTagHex('i', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(input)));
    RawTagHex('p', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(interpreter)));
    RawNewline();
    return;
  }

  RawPutc('['); RawPutc('L'); RawPutc('1'); RawPutc(']');
  RawTagHex('D', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(input->data.data)));
  RawTagHex('b', static_cast<uint32_t>(input->bytes));
  RawTagHex('t', static_cast<uint32_t>(input->type));
  RawNewline();

  RawPutc('['); RawPutc('I'); RawPutc('M'); RawPutc('0'); RawPutc(']');
  RawNewline();

  TfLiteStatus img_status =
      GetImage(kNumCols, kNumRows, kNumChannels, input->data.int8);

  RawPutc('['); RawPutc('I'); RawPutc('M'); RawPutc('1'); RawPutc(']');
  RawTagHex('s', static_cast<uint32_t>(img_status));
  RawNewline();

  if (img_status != kTfLiteOk) {
    RawPutc('b');
    RawNewline();
    return;
  }

  RawPutc('['); RawPutc('I'); RawPutc('N'); RawPutc('0'); RawPutc(']');

  for (int k = 0; k < 8; ++k) {
    RawTagHex('x', static_cast<uint32_t>(
                   static_cast<uint8_t>(input->data.int8[k])));
  }

  RawNewline();

  RawPutc('['); RawPutc('V'); RawPutc('0'); RawPutc(']');
  RawNewline();

  TfLiteStatus invoke_status = interpreter->Invoke();

  RawPutc('['); RawPutc('V'); RawPutc('1'); RawPutc(']');
  RawTagHex('s', static_cast<uint32_t>(invoke_status));
  RawNewline();

  if (invoke_status != kTfLiteOk) {
    RawPutc('c');
    RawNewline();
    return;
  }

  TfLiteTensor* output = interpreter->output(0);

  RawPutc('['); RawPutc('O'); RawPutc('0'); RawPutc(']');
  RawTagHex('o', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(output)));
  RawNewline();

  if (output == nullptr) {
    RawPutc('d');
    RawNewline();
    return;
  }

  RawPutc('['); RawPutc('O'); RawPutc('1'); RawPutc(']');
  RawTagHex('D', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(output->data.data)));
  RawTagHex('b', static_cast<uint32_t>(output->bytes));
  RawTagHex('t', static_cast<uint32_t>(output->type));
  RawNewline();

  const int8_t person_score = output->data.int8[kPersonIndex];
  const int8_t no_person_score = output->data.int8[kNotAPersonIndex];

  RawPuts("[OUTPUT] ");
  RawTagDec_NoDiv("person=", static_cast<int32_t>(person_score));
  RawTagDec_NoDiv("no_person=", static_cast<int32_t>(no_person_score));

  RawPuts("pred=");

  if (person_score > no_person_score) {
    RawPuts("PERSON");
  } else {
    RawPuts("NO_PERSON");
  }

  RawPuts(" margin=");
  RawPutSignedDec32_NoDiv(static_cast<int32_t>(person_score) -
                          static_cast<int32_t>(no_person_score));
  RawNewline();

  // RespondToDetection(person_score, no_person_score);

  RawPuts("INFERENCE DONE! CONGRATULATIONS");
  RawNewline();
}