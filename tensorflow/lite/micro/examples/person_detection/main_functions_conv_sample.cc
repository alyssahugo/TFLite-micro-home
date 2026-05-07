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


// UART helpers (AXI UART Lite style)
// Base:
//   TX FIFO     = 0x40600004
//   STATUS REG  = 0x40600008
//
// Bit 3 (0x08) is treated as TX FIFO full.

static inline void RawPutc(char c) {
  volatile uint32_t* uart_status = (volatile uint32_t*)0x40600008;
  volatile uint8_t* uart_tx = (volatile uint8_t*)0x40600004;

  while ((*uart_status) & 0x08) {
  }
  *uart_tx = (uint8_t)c;
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

// static inline void RawPutDecInt(int32_t x) {
//   if (x == 0) {
//     RawPutc('0');
//     return;
//   }

//   uint32_t mag;
//   if (x < 0) {
//     RawPutc('-');
//     mag = (uint32_t)(-(int64_t)x);
//   } else {
//     mag = (uint32_t)x;
//   }

//   char buf[16];
//   int n = 0;
//   while (mag > 0) {
//     buf[n++] = '0' + (mag % 10);
//     mag /= 10;
//   }
//   while (n--) {
//     RawPutc(buf[n]);
//   }
// }

static inline void RawPutDecInt(int32_t x) {
  static const int32_t places[10] = {
      1000000000, 100000000, 10000000, 1000000, 100000,
      10000, 1000, 100, 10, 1
  };

  if (x == 0) {
    RawPutc('0');
    return;
  }

  if (x < 0) {
    RawPutc('-');
    if (x == (int32_t)0x80000000) {
      RawPuts("2147483648");
      return;
    }
    x = -x;
  }

  int started = 0;
  for (int i = 0; i < 10; ++i) {
    int digit = 0;
    while (x >= places[i]) {
      x -= places[i];
      digit++;
    }
    if (digit != 0 || started) {
      RawPutc((char)('0' + digit));
      started = 1;
    }
  }
}



static int32_t input_data[4][4] = {
    { 1,  2,  3,  4},
    { 5,  6,  7,  8},
    { 9, 10, 11, 12},
    {13, 14, 15, 16}
};

static int32_t kernel_data[3][3] = {
    { 1,  0,  2},
    {-1,  3,  1},
    { 2, -2,  0}
};

static volatile int32_t bias_data = 1;

static int32_t expected_data[2][2] = {
    {26, 32},
    {50, 56}
};

static volatile int32_t output_data[2][2];


static volatile int32_t runtime_guard = 0;

__attribute__((noinline))
static void Conv2DValid3x3(
    int32_t input[4][4],
    int32_t kernel[3][3],
    volatile int32_t* bias,
    volatile int32_t output[2][2]) {
  for (int oy = 0; oy < 2; ++oy) {
    for (int ox = 0; ox < 2; ++ox) {
      int32_t acc = *bias;
      for (int ky = 0; ky < 3; ++ky) {
        for (int kx = 0; kx < 3; ++kx) {
          acc += input[oy + ky][ox + kx] * kernel[ky][kx];
        }
      }
      output[oy][ox] = acc;
    }
  }
}

static void PrintMatrix4x4(const char* name, int32_t m[4][4]) {
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

static void PrintMatrix3x3(const char* name, int32_t m[3][3]) {
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

static void PrintMatrix2x2Volatile(const char* name, volatile int32_t m[2][2]) {
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

static void PrintMatrix2x2(const char* name, int32_t m[2][2]) {
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

// Hex dump helpers for debugging memory contents exactly.
static void PrintMatrix4x4Hex(const char* name, int32_t m[4][4]) {
  RawPuts(name);
  RawPuts(" (hex) =");
  RawNewline();

  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      RawPutHex32((uint32_t)m[y][x]);
      RawPutc(' ');
    }
    RawNewline();
  }
}

static void PrintMatrix3x3Hex(const char* name, int32_t m[3][3]) {
  RawPuts(name);
  RawPuts(" (hex) =");
  RawNewline();

  for (int y = 0; y < 3; ++y) {
    for (int x = 0; x < 3; ++x) {
      RawPutHex32((uint32_t)m[y][x]);
      RawPutc(' ');
    }
    RawNewline();
  }
}

static void PrintMatrix2x2HexVolatile(const char* name, volatile int32_t m[2][2]) {
  RawPuts(name);
  RawPuts(" (hex) =");
  RawNewline();

  for (int y = 0; y < 2; ++y) {
    for (int x = 0; x < 2; ++x) {
      RawPutHex32((uint32_t)m[y][x]);
      RawPutc(' ');
    }
    RawNewline();
  }
}

void setup() {
  RawPuts("=== SIMPLE CONV TEST START ===");
  RawNewline();

  // Print addresses first, to help debug where data lives.
  RawPuts("ADDR input_data    = ");
  RawPutHex32((uint32_t)input_data);
  RawNewline();

  RawPuts("ADDR kernel_data   = ");
  RawPutHex32((uint32_t)kernel_data);
  RawNewline();

  RawPuts("ADDR bias_data     = ");
  RawPutHex32((uint32_t)&bias_data);
  RawNewline();

  RawPuts("ADDR expected_data = ");
  RawPutHex32((uint32_t)expected_data);
  RawNewline();

  RawPuts("ADDR output_data   = ");
  RawPutHex32((uint32_t)output_data);
  RawNewline();

  RawNewline();

  // Print decimal and hex so you can tell whether the issue is
  // actual zero data or bad sign/representation.
  PrintMatrix4x4("INPUT", input_data);
  PrintMatrix4x4Hex("INPUT", input_data);

  PrintMatrix3x3("KERNEL", kernel_data);
  PrintMatrix3x3Hex("KERNEL", kernel_data);

  RawPuts("BIAS = ");
  RawPutDecInt(bias_data);
  RawNewline();

  RawPuts("BIAS (hex) = ");
  RawPutHex32((uint32_t)bias_data);
  RawNewline();

  RawNewline();

  // Clear output first so you know it changed because of the function.
  output_data[0][0] = 0;
  output_data[0][1] = 0;
  output_data[1][0] = 0;
  output_data[1][1] = 0;

  // Runtime touch to discourage optimization.
  runtime_guard++;

  // Real runtime function call.
  Conv2DValid3x3(input_data, kernel_data, &bias_data, output_data);

  PrintMatrix2x2Volatile("OUTPUT", output_data);
  PrintMatrix2x2HexVolatile("OUTPUT", output_data);

  PrintMatrix2x2("EXPECTED", expected_data);

  RawPuts("COMPARE:");
  RawNewline();

  int pass = 1;
  for (int y = 0; y < 2; ++y) {
    for (int x = 0; x < 2; ++x) {
      int32_t outv = output_data[y][x];
      int32_t expv = expected_data[y][x];

      RawPuts("out[");
      RawPutDecInt(y);
      RawPuts("][");
      RawPutDecInt(x);
      RawPuts("] = ");
      RawPutDecInt(outv);
      RawPuts(" expected ");
      RawPutDecInt(expv);

      if (outv == expv) {
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