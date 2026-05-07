#include <cstdio>
#include <cstdint>

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"

static void PrintPtr(const char* name, const void* p) {
  std::printf("%s = %p\n", name, p);
}

int main() {
  std::puts("START");

  tflite::MicroMutableOpResolver<5> resolver;

const TFLMRegistration reg = tflite::Register_AVERAGE_POOL_2D();

  std::puts("=== Register_AVERAGE_POOL_2D_INT8 ===");
  PrintPtr("init", reinterpret_cast<const void*>(reg.init));
  PrintPtr("free", reinterpret_cast<const void*>(reg.free));
  PrintPtr("prepare", reinterpret_cast<const void*>(reg.prepare));
  PrintPtr("invoke", reinterpret_cast<const void*>(reg.invoke));
  std::printf("builtin_code = %d\n", reg.builtin_code);
  std::printf("custom_name  = %s\n", reg.custom_name ? reg.custom_name : "(null)");

  std::puts("Calling AddAveragePool2D...");
  TfLiteStatus s = resolver.AddAveragePool2D(reg);
  std::printf("AddAveragePool2D status = %d\n", s);

  std::puts("DONE");
  return 0;
}