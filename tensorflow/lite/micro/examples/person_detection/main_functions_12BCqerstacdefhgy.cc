// // /* Copyright 2022 The TensorFlow Authors. All Rights Reserved.

// // Licensed under the Apache License, Version 2.0 (the "License");
// // you may not use this file except in compliance with the License.
// // You may obtain a copy of the License at

// //     http://www.apache.org/licenses/LICENSE-2.0

// // Unless required by applicable law or agreed to in writing, software
// // distributed under the License is distributed on an "AS IS" BASIS,
// // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// // See the License for the specific language governing permissions and
// // limitations under the License.
// // ==============================================================================*/

// // #include "tensorflow/lite/micro/examples/person_detection/main_functions.h"

// // #include "tensorflow/lite/micro/examples/person_detection/detection_responder.h"
// // #include "tensorflow/lite/micro/examples/person_detection/image_provider.h"
// // #include "tensorflow/lite/micro/examples/person_detection/model_settings.h"
// // #include "tensorflow/lite/micro/micro_interpreter.h"
// // #include "tensorflow/lite/micro/micro_log.h"
// // #include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
// // #include "tensorflow/lite/micro/models/person_detect_model_data.h"
// // #include "tensorflow/lite/micro/system_setup.h"
// // #include "tensorflow/lite/schema/schema_generated.h"

// // // Globals, used for compatibility with Arduino-style sketches.
// // namespace {
// // const tflite::Model* model = nullptr;
// // tflite::MicroInterpreter* interpreter = nullptr;
// // TfLiteTensor* input = nullptr;

// // // In order to use optimized tensorflow lite kernels, a signed int8_t quantized
// // // model is preferred over the legacy unsigned model format. This means that
// // // throughout this project, input images must be converted from unisgned to
// // // signed format. The easiest and quickest way to convert from unsigned to
// // // signed 8-bit integers is to subtract 128 from the unsigned value to get a
// // // signed value.

// // // An area of memory to use for input, output, and intermediate arrays.
// // constexpr int kTensorArenaSize = 136 * 1024;
// // alignas(16) static uint8_t tensor_arena[kTensorArenaSize];
// // }  // namespace

// // // The name of this function is important for Arduino compatibility.
// // void setup() {
// //   tflite::InitializeTarget();

// //   // Map the model into a usable data structure. This doesn't involve any
// //   // copying or parsing, it's a very lightweight operation.
// //   model = tflite::GetModel(g_person_detect_model_data);
// //   if (model->version() != TFLITE_SCHEMA_VERSION) {
// //     MicroPrintf(
// //         "Model provided is schema version %d not equal "
// //         "to supported version %d.",
// //         model->version(), TFLITE_SCHEMA_VERSION);
// //     return;
// //   }

// //   // Pull in only the operation implementations we need.
// //   // This relies on a complete list of all the ops needed by this graph.

// //   // NOLINTNEXTLINE(runtime-global-variables)
// //   static tflite::MicroMutableOpResolver<5> micro_op_resolver;
// //   micro_op_resolver.AddAveragePool2D(tflite::Register_AVERAGE_POOL_2D_INT8());
// //   micro_op_resolver.AddConv2D(tflite::Register_CONV_2D_INT8());
// //   micro_op_resolver.AddDepthwiseConv2D(
// //       tflite::Register_DEPTHWISE_CONV_2D_INT8());
// //   micro_op_resolver.AddReshape();
// //   micro_op_resolver.AddSoftmax(tflite::Register_SOFTMAX_INT8());

// //   // Build an interpreter to run the model with.
// //   // NOLINTNEXTLINE(runtime-global-variables)
// //   static tflite::MicroInterpreter static_interpreter(
// //       model, micro_op_resolver, tensor_arena, kTensorArenaSize);
// //   interpreter = &static_interpreter;

// //   // Allocate memory from the tensor_arena for the model's tensors.
// //   TfLiteStatus allocate_status = interpreter->AllocateTensors();
// //   if (allocate_status != kTfLiteOk) {
// //     MicroPrintf("AllocateTensors() failed");
// //     return;
// //   }

// //   // Get information about the memory area to use for the model's input.
// //   input = interpreter->input(0);
// // }

// // // The name of this function is important for Arduino compatibility.
// // void loop() {
// //   // Get image from provider.
// //   if (kTfLiteOk !=
// //       GetImage(kNumCols, kNumRows, kNumChannels, input->data.int8)) {
// //     MicroPrintf("Image capture failed.");
// //   }

// //   // Run the model on this input and make sure it succeeds.
// //   if (kTfLiteOk != interpreter->Invoke()) {
// //     MicroPrintf("Invoke failed.");
// //   }

// //   TfLiteTensor* output = interpreter->output(0);

// //   // Process the inference results.
// //   int8_t person_score = output->data.uint8[kPersonIndex];
// //   int8_t no_person_score = output->data.uint8[kNotAPersonIndex];
// //   RespondToDetection(person_score, no_person_score);
// // }


// void setup() {
//   RawPutc('1');
//   tflite::InitializeTarget();
//   RawPutc('2');
//   MicroPrintf("person_detection setup begin\n");

//   model = tflite::GetModel(g_person_detect_model_data);
//   if (model == nullptr) {
//     MicroPrintf("GetModel() failed\n");
//     return;
//   }

//   if (model->version() != TFLITE_SCHEMA_VERSION) {
//     MicroPrintf("Model schema %d != supported %d\n",
//                 model->version(), TFLITE_SCHEMA_VERSION);
//     return;
//   }

//   static tflite::MicroMutableOpResolver<5> resolver;
//   resolver.AddAveragePool2D(tflite::Register_AVERAGE_POOL_2D_INT8());
//   resolver.AddConv2D(tflite::Register_CONV_2D_INT8());
//   resolver.AddDepthwiseConv2D(tflite::Register_DEPTHWISE_CONV_2D_INT8());
//   resolver.AddReshape();
//   resolver.AddSoftmax(tflite::Register_SOFTMAX_INT8());

//   static tflite::MicroInterpreter static_interpreter(
//       model, resolver, tensor_arena, kTensorArenaSize);
//   interpreter = &static_interpreter;

//   if (interpreter->AllocateTensors() != kTfLiteOk) {
//     MicroPrintf("AllocateTensors failed\n");
//     return;
//   }

//   input = interpreter->input(0);
//   if (input == nullptr) {
//     MicroPrintf("input is null\n");
//     return;
//   }

//   MicroPrintf("setup done\n");
// }


// void loop() {
//   RawPutc('3');
//   if (input == nullptr || interpreter == nullptr) {
//     MicroPrintf("setup incomplete\n");
//     return;
//   }

//   if (GetImage(kNumCols, kNumRows, kNumChannels, input->data.int8) != kTfLiteOk) {
//     MicroPrintf("GetImage failed\n");
//     return;
//   }

//   if (interpreter->Invoke() != kTfLiteOk) {
//     MicroPrintf("Invoke failed\n");
//     return;
//   }

//   TfLiteTensor* output = interpreter->output(0);
//   if (output == nullptr) {
//     MicroPrintf("output is null\n");
//     return;
//   }

//   const int8_t person_score = output->data.int8[kPersonIndex];
//   const int8_t no_person_score = output->data.int8[kNotAPersonIndex];
//   RawPutc('4');
//   RespondToDetection(person_score, no_person_score);
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


static inline void RawPutc(char c) {
  volatile uint32_t* const uart_tx =
      reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
  *uart_tx = static_cast<uint32_t>(static_cast<uint8_t>(c));
}


// namespace {
// const tflite::Model* model = nullptr;
// tflite::MicroInterpreter* interpreter = nullptr;
// TfLiteTensor* input = nullptr;

// constexpr int kTensorArenaSize = 136 * 1024;
// alignas(16) static uint8_t tensor_arena[kTensorArenaSize];
// }  // namespace

namespace {
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;

constexpr int kTensorArenaSize = 136 * 1024;
alignas(16) __attribute__((section(".arena")))
static uint8_t tensor_arena[kTensorArenaSize];

tflite::MicroMutableOpResolver<5> resolver;
}  // namespace
void loop() {
  RawPutc('3');
  if (input == nullptr || interpreter == nullptr) {
    RawPutc('j');
    return;
  }

  if (GetImage(kNumCols, kNumRows, kNumChannels, input->data.int8) != kTfLiteOk) {
    RawPutc('K');
    return;
  }

  if (interpreter->Invoke() != kTfLiteOk) {
    RawPutc('L');
    return;
  }

  TfLiteTensor* output = interpreter->output(0);
  if (output == nullptr) {
    RawPutc('N');
    return;
  }

  const int8_t person_score = output->data.int8[kPersonIndex];
  const int8_t no_person_score = output->data.int8[kNotAPersonIndex];
  RawPutc('4');
  RespondToDetection(person_score, no_person_score);
}


// void setup() {
//   RawPutc('1');
//   tflite::InitializeTarget();
//   RawPutc('2');

//   // RawPutc('A');
//   model = tflite::GetModel(g_person_detect_model_data);
//   RawPutc('B');

//   if (model == nullptr) {
//     RawPutc('X');
//     while (1) {}
//   }

//   if (model->version() != TFLITE_SCHEMA_VERSION) {
//     RawPutc('Y');
//     while (1) {}
//   }

//   RawPutc('C');

//    RawPutc('O');
//   static tflite::MicroMutableOpResolver<5> resolver;
//   resolver.AddAveragePool2D(tflite::Register_AVERAGE_POOL_2D_INT8());
//    RawPutc('P');
//   resolver.AddConv2D(tflite::Register_CONV_2D_INT8());
//   //  RawPutc('Q');
//   resolver.AddDepthwiseConv2D(tflite::Register_DEPTHWISE_CONV_2D_INT8());
//   //  RawPutc('R');
//   resolver.AddReshape();
//   //  RawPutc('S');
// //   resolver.AddSoftmax(tflite::Register_SOFTMAX_INT8());

// //   RawPutc('D');

// //   static tflite::MicroInterpreter static_interpreter(
// //       model, resolver, tensor_arena, kTensorArenaSize);
// //   interpreter = &static_interpreter;

// //   RawPutc('E');

// //   if (interpreter->AllocateTensors() != kTfLiteOk) {
// //     RawPutc('F');
// //     while (1) {}
// //   }

// //   RawPutc('G');

// //   input = interpreter->input(0);
// //   if (input == nullptr) {
// //     RawPutc('H');
// //     while (1) {}
// //   }

// //   RawPutc('I');
// // }



void setup() {
  struct TestStruct {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint8_t bytes[32];
  };

  static TestStruct src;
  static TestStruct dst;

  RawPutc('1');
  tflite::InitializeTarget();
  RawPutc('2');

  model = tflite::GetModel(g_person_detect_model_data);
  RawPutc('B');

  if (model == nullptr) {
    RawPutc('X');
    while (1) {}
  }

  RawPutc('C');

  // TEST 1: direct byte write into resolver storage
  RawPutc('q');
  volatile uint8_t* rp = reinterpret_cast<volatile uint8_t*>(&resolver);
  rp[0] = 0x00;
  RawPutc('r');

  // TEST 2: registration function alone
  RawPutc('s');
  auto avg_reg = tflite::Register_AVERAGE_POOL_2D_INT8();
  RawPutc('t');

  // TEST 3: plain scalar stores into a normal struct
  RawPutc('a');
  src.a = 0x11111111u;
  RawPutc('b');

  src.b = 0x22222222u;
  RawPutc('c');

  src.c = 0x33333333u;
  RawPutc('d');

  src.d = 0x44444444u;
  RawPutc('e');

  // TEST 4: byte array stores
  for (int i = 0; i < 32; ++i) {
    src.bytes[i] = static_cast<uint8_t>(i);
  }
  RawPutc('f');

  // TEST 5: struct copy
  dst = src;
  RawPutc('g');

  // Optional extra write after copy
  dst.bytes[0] = 0xAA;
  RawPutc('h');

  // TEST 6: minimal resolver mutation
  RawPutc('u');
  resolver.AddReshape();
  RawPutc('v');

  // TEST 7: another resolver mutation
  RawPutc('w');
  resolver.AddSoftmax(tflite::Register_SOFTMAX_INT8());
  RawPutc('x');

  // TEST 8: original failing resolver mutation
  RawPutc('y');
  resolver.AddAveragePool2D(avg_reg);
  RawPutc('z');

  // TEST 9: add the rest only if all above pass
  RawPutc('J');
  resolver.AddConv2D(tflite::Register_CONV_2D_INT8());
  RawPutc('K');

  resolver.AddDepthwiseConv2D(tflite::Register_DEPTHWISE_CONV_2D_INT8());
  RawPutc('L');

  // TEST 10: interpreter construction
  RawPutc('M');
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;
  RawPutc('N');

  // TEST 11: AllocateTensors
  if (interpreter->AllocateTensors() != kTfLiteOk) {
    RawPutc('F');
    while (1) {}
  }
  RawPutc('G');

  // TEST 12: input tensor fetch
  input = interpreter->input(0);
  if (input == nullptr) {
    RawPutc('H');
    while (1) {}
  }
  RawPutc('I');

  while (1) {}
}