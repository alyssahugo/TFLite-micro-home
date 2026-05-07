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


// static inline void RawPutc(char c) {
//   volatile uint32_t* const uart_tx =
//       reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
//   *uart_tx = static_cast<uint32_t>(static_cast<uint8_t>(c));
// }

// namespace {
// const tflite::Model* model = nullptr;
// tflite::MicroInterpreter* interpreter = nullptr;
// TfLiteTensor* input = nullptr;

// constexpr int kTensorArenaSize = 136 * 1024;
// alignas(16) __attribute__((section(".arena")))
// static uint8_t tensor_arena[kTensorArenaSize];

// tflite::MicroMutableOpResolver<5> resolver;
// }  // namespace
// void loop() {
//   RawPutc('3');
//   if (input == nullptr || interpreter == nullptr) {
//     RawPutc('a');
//     return;
//   }

//   if (GetImage(kNumCols, kNumRows, kNumChannels, input->data.int8) != kTfLiteOk) {
//     RawPutc('b');
//     return;
//   }

//   if (interpreter->Invoke() != kTfLiteOk) {
//     RawPutc('c');
//     return;
//   }

//   TfLiteTensor* output = interpreter->output(0);
//   if (output == nullptr) {
//     RawPutc('d');
//     return;
//   }

//   const int8_t person_score = output->data.int8[kPersonIndex];
//   const int8_t no_person_score = output->data.int8[kNotAPersonIndex];
//   RawPutc('4');
//   RespondToDetection(person_score, no_person_score);
// }


void setup() {
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
  RawPutc('q');
  RawPutc('u');
  resolver.AddReshape();
  RawPutc('v');

  RawPutc('w');
  resolver.AddSoftmax(tflite::Register_SOFTMAX_INT8());
  RawPutc('x');

  RawPutc('y');
  auto avg_reg = tflite::Register_AVERAGE_POOL_2D_INT8();
  resolver.AddAveragePool2D(avg_reg);
  RawPutc('z');

  RawPutc('J');
  resolver.AddConv2D(tflite::Register_CONV_2D_INT8());
  RawPutc('K');

  resolver.AddDepthwiseConv2D(tflite::Register_DEPTHWISE_CONV_2D_INT8());
  RawPutc('L');

  RawPutc('M');
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;
  RawPutc('N');

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    RawPutc('F');
    while (1) {}
  }
  RawPutc('G');

  input = interpreter->input(0);
  if (input == nullptr) {
    RawPutc('H');
    while (1) {}
  }
  RawPutc('I');

  while (1) {}
}


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

#include <stdint.h>

static inline void RawPutc(char c) {
  volatile uint32_t* const uart_tx =
      reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
  volatile uint32_t* const uart_status =
      reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x08u);

  // Wait while TX FIFO is full (bit 3 set)
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
  v &= 0xF;
  RawPutc((v < 10) ? ('0' + v) : ('A' + (v - 10)));
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
    RawPutc('a');
    RawNewline();
    return;
  }

  RawPutc('g');
  if (GetImage(kNumCols, kNumRows, kNumChannels, input->data.int8) != kTfLiteOk) {
    RawPutc('b');
    RawNewline();
    return;
  }

  RawPutc('h');
  if (interpreter->Invoke() != kTfLiteOk) {
    RawPutc('c');
    RawNewline();
    return;
  }

  RawPutc('i');
  TfLiteTensor* output = interpreter->output(0);
  if (output == nullptr) {
    RawPutc('d');
    RawNewline();
    return;
  }

  const int8_t person_score = output->data.int8[kPersonIndex];
  const int8_t no_person_score = output->data.int8[kNotAPersonIndex];

  RawPutc('4');
  RawTagHex('p', static_cast<uint32_t>(static_cast<uint8_t>(person_score)));
  RawTagHex('n', static_cast<uint32_t>(static_cast<uint8_t>(no_person_score)));
  RawNewline();

  RespondToDetection(person_score, no_person_score);
}

void setup() {
  RawPutc('1');
  tflite::InitializeTarget();
  RawPutc('2');
  RawNewline();

  // Arena base + size
  RawTagHex('A', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tensor_arena)));
  RawTagHex('S', static_cast<uint32_t>(kTensorArenaSize));
  RawNewline();

  // Model pointer
  model = tflite::GetModel(g_person_detect_model_data);
  RawPutc('B');
  RawTagHex('m', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(model)));
  RawNewline();

  if (model == nullptr) {
    RawPutc('X');
    RawNewline();
    return;
  }

  // Schema version check: print only, do not hang
  RawTagHex('v', static_cast<uint32_t>(model->version()));
  RawTagHex('t', static_cast<uint32_t>(TFLITE_SCHEMA_VERSION));
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    RawPutc('V');  // mismatch, but continue
  } else {
    RawPutc('v');  // match
  }
  RawNewline();

  RawPutc('C');
  RawNewline();

  // Inspect model flatbuffer structure before interpreter construction
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

  // Resolver registration tracing
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

  // Constructor path
  RawPutc('M');
  RawNewline();

  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  RawPutc('N');
  RawTagHex('I', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(interpreter)));
  RawNewline();

  // Allocate tensors
  TfLiteStatus alloc_status = interpreter->AllocateTensors();
  RawTagHex('G', static_cast<uint32_t>(alloc_status));
  RawNewline();

  if (alloc_status != kTfLiteOk) {
    RawPutc('F');
    RawNewline();
    return;
  }

  // input(0)
  TfLiteTensor* tmp_input = interpreter->input(0);
  RawTagHex('P', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tmp_input)));
  RawNewline();

  input = tmp_input;
  if (input == nullptr) {
    RawPutc('H');
    RawNewline();
    while (1) {}
  }

  // Tensor details
  RawPutc('I');
  RawTagHex('d', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(input->dims)));
  RawTagHex('b', static_cast<uint32_t>(input->bytes));
  RawTagHex('y', static_cast<uint32_t>(input->type));
  RawTagHex('D', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(input->data.data)));
  RawNewline();

  if (input->dims != nullptr) {
    RawTagHex('0', static_cast<uint32_t>(input->dims->size));
    if (input->dims->size > 0) RawTagHex('1', static_cast<uint32_t>(input->dims->data[0]));
    if (input->dims->size > 1) RawTagHex('2', static_cast<uint32_t>(input->dims->data[1]));
    if (input->dims->size > 2) RawTagHex('3', static_cast<uint32_t>(input->dims->data[2]));
    if (input->dims->size > 3) RawTagHex('4', static_cast<uint32_t>(input->dims->data[3]));
    RawNewline();
  } else {
    RawPutc('D');
    RawNewline();
  }

  // Also inspect output(0) right away
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
      if (tmp_output->dims->size > 0) RawTagHex('6', static_cast<uint32_t>(tmp_output->dims->data[0]));
      if (tmp_output->dims->size > 1) RawTagHex('7', static_cast<uint32_t>(tmp_output->dims->data[1]));
      RawNewline();
    }
  }

  RawPutc('Z');
  RawNewline();

  while (1) {}
}