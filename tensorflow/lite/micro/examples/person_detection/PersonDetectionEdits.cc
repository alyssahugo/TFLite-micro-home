// main.cc

#include "tensorflow/lite/micro/examples/person_detection/main_functions.h"

extern "C" int main(void) {
  setup();
  while (1) {
    loop();
  }
  return 0;
}


// main_functions.cc

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


namespace {
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;

constexpr int kTensorArenaSize = 136 * 1024;
alignas(16) static uint8_t tensor_arena[kTensorArenaSize];
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


void setup() {
  RawPutc('1');
  tflite::InitializeTarget();
  RawPutc('2');

  // RawPutc('A');
  model = tflite::GetModel(g_person_detect_model_data);
  RawPutc('B');

  if (model == nullptr) {
    RawPutc('X');
    while (1) {}
  }

  if (model->version() != TFLITE_SCHEMA_VERSION) {
    RawPutc('Y');
    while (1) {}
  }

  RawPutc('C');

  static tflite::MicroMutableOpResolver<5> resolver;
   RawPutc('O');
  resolver.AddAveragePool2D(tflite::Register_AVERAGE_POOL_2D_INT8());
   RawPutc('P');
  resolver.AddConv2D(tflite::Register_CONV_2D_INT8());
  //  RawPutc('Q');
  resolver.AddDepthwiseConv2D(tflite::Register_DEPTHWISE_CONV_2D_INT8());
  //  RawPutc('R');
  resolver.AddReshape();
  //  RawPutc('S');
  resolver.AddSoftmax(tflite::Register_SOFTMAX_INT8());

  RawPutc('D');

  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  RawPutc('E');

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
}

// image_provider.cc

#include "tensorflow/lite/micro/examples/person_detection/image_provider.h"

#include <cstring>

#include "tensorflow/lite/micro/examples/person_detection/model_settings.h"
#include "tensorflow/lite/micro/examples/person_detection/testdata/no_person_image_data.h"
#include "tensorflow/lite/micro/examples/person_detection/testdata/person_image_data.h"
#include "tensorflow/lite/micro/micro_log.h"

static inline void RawPutu(char c) {
  volatile uint32_t* const uart_tx =
      reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
  *uart_tx = static_cast<uint32_t>(static_cast<uint8_t>(c));
}


TfLiteStatus GetImage(int image_width, int image_height, int channels,
                      int8_t* image_data) {
  if (image_data == nullptr) {
    RawPutu('Q');
    // MicroPrintf("GetImage: null buffer\n");
    return kTfLiteError;
  }

  if (image_width != kNumCols || image_height != kNumRows ||
      channels != kNumChannels) {
      RawPutu('R');
    // MicroPrintf("GetImage: bad shape %d x %d x %d\n",
    //             image_width, image_height, channels);
    return kTfLiteError;
  }

  static int which = 0;
  const int image_size = image_width * image_height * channels;

  if (which == 0) {
    if (g_person_image_data_size != image_size) {
      RawPutu('S');
      // MicroPrintf("person image size mismatch\n");
      return kTfLiteError;
    }
    memcpy(image_data, g_person_image_data, image_size);
      RawPutu('T');
    // MicroPrintf("GetImage: person test image\n");
  } else {
    if (g_no_person_image_data_size != image_size) {
      RawPutu('U');
      // MicroPrintf("no_person image size mismatch\n");
      return kTfLiteError;
    }
    memcpy(image_data, g_no_person_image_data, image_size);
    RawPutu('V');
    // MicroPrintf("GetImage: no_person test image\n");
  }

  which ^= 1;
  return kTfLiteOk;
}


// detection_responder.cc

#include "tensorflow/lite/micro/examples/person_detection/detection_responder.h"

#include "tensorflow/lite/micro/micro_log.h"

void RespondToDetection(int8_t person_score, int8_t no_person_score) {
  MicroPrintf("person score: %d, no person score: %d\n",
              person_score, no_person_score);

  if (person_score > no_person_score) {
    MicroPrintf("RESULT: PERSON\n");
  } else {
    MicroPrintf("RESULT: NO PERSON\n");
  }
}
