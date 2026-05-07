#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/examples/person_detection/model_settings.h"
#include "tensorflow/lite/micro/examples/person_detection/testdata/no_person_image_data.h"
#include "tensorflow/lite/micro/examples/person_detection/testdata/person_image_data.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/models/person_detect_model_data.h"
#include "tensorflow/lite/micro/testing/micro_test.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include <cstdint>
#include <cstring>
#include <cstdio>


#ifndef RTL_BUILD
#define RTL_BUILD 0
#endif

#if RTL_BUILD
static inline void mmio_write32(uint32_t addr, uint32_t v) {
  *(volatile uint32_t*)addr = v;
}
static inline void rtl_mailbox_done(uint32_t sig0, uint32_t sig1) {
  mmio_write32(0x000FFFF4u, sig0);  // SIG0
  mmio_write32(0x000FFFF8u, sig1);  // SIG1
  mmio_write32(0x000FFFFCu, 1u);    // DONE
  while (1) { }                     // Halt for RTL
}
#endif


static uint8_t  g_answerkey[2];
static uint32_t g_answerkey_len = 0;

static inline void AK_Reset() { g_answerkey_len = 0; }
static inline void AK_AppendByte(uint8_t v) {
  if (g_answerkey_len < sizeof(g_answerkey)) g_answerkey[g_answerkey_len++] = v;
}

static void AK_WriteStdoutBinary(uint32_t run_id) {
  std::printf("AK_BEGIN run_id=%u len=%u\n", (unsigned)run_id, (unsigned)g_answerkey_len);
  std::fwrite(g_answerkey, 1, g_answerkey_len, stdout);
  std::fflush(stdout);
  std::printf("\nAK_END run_id=%u\n", (unsigned)run_id);
}


constexpr int kTensorArenaSize = 352 * 1024;
static uint8_t tensor_arena[kTensorArenaSize];


TF_LITE_MICRO_TESTS_BEGIN

TF_LITE_MICRO_TEST(TestInvoke) {
  const tflite::Model* model = ::tflite::GetModel(g_person_detect_model_data);
  if (!model) {
    MicroPrintf("Model is null");
    return;
  }
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    MicroPrintf("Model schema %d != supported %d", model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  tflite::MicroMutableOpResolver<6> r;
  r.AddAveragePool2D(tflite::Register_AVERAGE_POOL_2D_INT8());
  r.AddConv2D(tflite::Register_CONV_2D_INT8());
  r.AddDepthwiseConv2D(tflite::Register_DEPTHWISE_CONV_2D_INT8());
  r.AddReshape();
  r.AddSoftmax(tflite::Register_SOFTMAX_INT8());
  r.AddAdd();

  tflite::MicroInterpreter interpreter(
      model, r, tensor_arena, kTensorArenaSize,
      /*resource_variables=*/nullptr,
      /*profiler=*/nullptr,
      /*preserve_all_tensors=*/false);

  if (interpreter.AllocateTensors() != kTfLiteOk) {
    MicroPrintf("AllocateTensors failed");
    return;
  }

  MicroPrintf("arena_used_bytes=%d", (int)interpreter.arena_used_bytes());

  TfLiteTensor* input = interpreter.input(0);
  if (!input || input->type != kTfLiteInt8) {
    MicroPrintf("Bad input tensor");
    return;
  }


  // Run 1: Person image

  std::memcpy(input->data.int8, g_person_image_data, input->bytes);

  if (interpreter.Invoke() != kTfLiteOk) {
    MicroPrintf("Invoke failed (person)");
    return;
  }

  TfLiteTensor* output = interpreter.output(0);
  int8_t person_score = output->data.int8[kPersonIndex];
  int8_t no_person_score = output->data.int8[kNotAPersonIndex];

  MicroPrintf("With Person => person=%d no_person=%d", person_score, no_person_score);

  AK_Reset();
  AK_AppendByte((uint8_t)person_score);
  AK_AppendByte((uint8_t)no_person_score);
  AK_WriteStdoutBinary(1);


  // Run 2: No-person image

  std::memcpy(input->data.int8, g_no_person_image_data, input->bytes);

  if (interpreter.Invoke() != kTfLiteOk) {
    MicroPrintf("Invoke failed (noperson)");
    return;
  }

  output = interpreter.output(0);
  person_score = output->data.int8[kPersonIndex];
  no_person_score = output->data.int8[kNotAPersonIndex];

  MicroPrintf("No Person => person=%d no_person=%d", person_score, no_person_score);

  AK_Reset();
  AK_AppendByte((uint8_t)person_score);
  AK_AppendByte((uint8_t)no_person_score);
  AK_WriteStdoutBinary(2);

  MicroPrintf("Done.");

#if RTL_BUILD
  rtl_mailbox_done(0x11111111, 0x22222222);
#endif
}

TF_LITE_MICRO_TESTS_END