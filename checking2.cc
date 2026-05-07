#include <cstdio>
#include "tensorflow/lite/schema/schema_generated.h"
#include "person_detect_model_data.h"
#include "tensorflow/lite/version.h"

int main() {
  const tflite::Model* model = tflite::GetModel(g_person_detect_model_data);
  if (!model) {
    std::printf("model is null\n");
    return 1;
  }

  std::printf("model->version() = %d\n", model->version());
  std::printf("TFLITE_SCHEMA_VERSION = %d\n", TFLITE_SCHEMA_VERSION);
  return 0;
}