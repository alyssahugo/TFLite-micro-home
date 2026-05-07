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
  volatile uint8_t* uart_tx = (volatile uint8_t*)0x40600004;
  *uart_tx = (uint8_t)c;
}

static inline void RawPuts(const char* s) {
  while (*s) RawPutc(*s++);
}

static inline void RawPutHex4(uint8_t x) {
  RawPutc("0123456789ABCDEF"[x & 0xF]);
}

static inline void RawPutHex8(uint8_t x) {
  RawPutHex4((x >> 4) & 0xF);
  RawPutHex4(x & 0xF);
}

static inline void RawPutHex32(uint32_t x) {
  for (int i = 7; i >= 0; --i) {
    RawPutc("0123456789ABCDEF"[(x >> (i * 4)) & 0xF]);
  }
}

static inline void RawPutDecInt(int32_t x) {
  if (x == 0) {
    RawPutc('0');
    return;
  }

  if (x < 0) {
    RawPutc('-');
    x = -x;
  }

  char buf[16];
  int n = 0;
  while (x > 0) {
    buf[n++] = '0' + (x % 10);
    x /= 10;
  }
  while (n--) RawPutc(buf[n]);
}

static inline void RawNewline() {
  RawPutc('\r');
  RawPutc('\n');
}

// ------------------------------------------------------------
// Simple valid convolution:
// input:  4x4
// kernel: 3x3
// output: 2x2
//
// output[y][x] = bias + sum(input[y+ky][x+kx] * kernel[ky][kx])
// ------------------------------------------------------------
static void Conv2DValid3x3(
    const int32_t input[4][4],
    const int32_t kernel[3][3],
    int32_t bias,
    int32_t output[2][2]) {
  for (int oy = 0; oy < 2; ++oy) {
    for (int ox = 0; ox < 2; ++ox) {
      int32_t acc = bias;
      for (int ky = 0; ky < 3; ++ky) {
        for (int kx = 0; kx < 3; ++kx) {
          acc += input[oy + ky][ox + kx] * kernel[ky][kx];
        }
      }
      output[oy][ox] = acc;
    }
  }
}

static void PrintMatrix4x4(const char* name, const int32_t m[4][4]) {
  RawPuts(name);
  RawPuts(" =");
  RawNewline();
  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      RawPutDecInt(m[y][x]);
      RawPutc(' ');
    }
    RawNewline();
  }
}

static void PrintMatrix3x3(const char* name, const int32_t m[3][3]) {
  RawPuts(name);
  RawPuts(" =");
  RawNewline();
  for (int y = 0; y < 3; ++y) {
    for (int x = 0; x < 3; ++x) {
      RawPutDecInt(m[y][x]);
      RawPutc(' ');
    }
    RawNewline();
  }
}

static void PrintMatrix2x2(const char* name, const int32_t m[2][2]) {
  RawPuts(name);
  RawPuts(" =");
  RawNewline();
  for (int y = 0; y < 2; ++y) {
    for (int x = 0; x < 2; ++x) {
      RawPutDecInt(m[y][x]);
      RawPutc(' ');
    }
    RawNewline();
  }
}

void setup() {
  // ----------------------------------------------------------
  // Sample input, kernel, bias
  // ----------------------------------------------------------
  static const int32_t input[4][4] = {
      { 1,  2,  3,  4},
      { 5,  6,  7,  8},
      { 9, 10, 11, 12},
      {13, 14, 15, 16}
  };

  static const int32_t kernel[3][3] = {
      { 1,  0,  2},
      {-1,  3,  1},
      { 2, -2,  0}
  };

  static const int32_t bias = 1;

  // Expected/theoretical outputs:
  // out[0][0] = 26
  // out[0][1] = 32
  // out[1][0] = 50
  // out[1][1] = 56
  static const int32_t expected[2][2] = {
      {26, 32},
      {50, 56}
  };

  int32_t output[2][2];

  RawPuts("=== SIMPLE CONV TEST START ===");
  RawNewline();

  PrintMatrix4x4("INPUT", input);
  PrintMatrix3x3("KERNEL", kernel);

  RawPuts("BIAS = ");
  RawPutDecInt(bias);
  RawNewline();

  Conv2DValid3x3(input, kernel, bias, output);

  PrintMatrix2x2("OUTPUT", output);
  PrintMatrix2x2("EXPECTED", expected);

  RawPuts("COMPARE:");
  RawNewline();

  int pass = 1;
  for (int y = 0; y < 2; ++y) {
    for (int x = 0; x < 2; ++x) {
      RawPuts("out[");
      RawPutDecInt(y);
      RawPuts("][");
      RawPutDecInt(x);
      RawPuts("] = ");
      RawPutDecInt(output[y][x]);
      RawPuts(" expected ");
      RawPutDecInt(expected[y][x]);

      if (output[y][x] == expected[y][x]) {
        RawPuts(" PASS");
      } else {
        RawPuts(" FAIL");
        pass = 0;
      }
      RawNewline();
    }
  }

  if (pass) {
    RawPuts("FINAL: PASS");
  } else {
    RawPuts("FINAL: FAIL");
  }
  RawNewline();

  RawPuts("=== SIMPLE CONV TEST END ===");
  RawNewline();

  while (1) {}
}

void loop() {
}