#include <cstdio>
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"

int main() {
  tflite::MicroMutableOpResolver<5> resolver;

  const TFLMRegistration reg_avg = tflite::Register_AVERAGE_POOL_2D();

  printf("AVG init    = %p\n", (void*)reg_avg.init);
  printf("AVG free    = %p\n", (void*)reg_avg.free);
  printf("AVG prepare = %p\n", (void*)reg_avg.prepare);
  printf("AVG invoke  = %p\n", (void*)reg_avg.invoke);

  resolver.AddAveragePool2D(reg_avg);
  printf("AFTER AddAveragePool2D\n");
  return 0;
}