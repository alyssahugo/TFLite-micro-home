#include <cstdio>
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/models/person_detect_model_data.h"

int main() {
  auto model = tflite::GetModel(g_person_detect_model_data);
  if (!model) {
    printf("model is null\n");
    return 1;
  }
  printf("model->version() = %d\n", model->version());
  printf("TFLITE_SCHEMA_VERSION = %d\n", TFLITE_SCHEMA_VERSION);
  return 0;
}