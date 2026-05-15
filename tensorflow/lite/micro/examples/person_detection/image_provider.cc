/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

// #include "tensorflow/lite/micro/examples/person_detection/image_provider.h"

// #include "tensorflow/lite/micro/examples/person_detection/model_settings.h"

// TfLiteStatus GetImage(int image_width, int image_height, int channels,
//                       int8_t* image_data) {
//   for (int i = 0; i < image_width * image_height * channels; ++i) {
//     image_data[i] = 0;
//   }
//   return kTfLiteOk;
// }


#include "tensorflow/lite/micro/examples/person_detection/image_provider.h"

#include <cstring>

#include "tensorflow/lite/micro/examples/person_detection/model_settings.h"
#include "tensorflow/lite/micro/examples/person_detection/testdata/no_person_image_data.h"
#include "tensorflow/lite/micro/examples/person_detection/testdata/person_image_data.h"
#include "tensorflow/lite/micro/examples/person_detection/testdata/image_3_image_data.h"
#include "tensorflow/lite/micro/examples/person_detection/testdata/image_4_image_data.h"
#include "tensorflow/lite/micro/examples/person_detection/testdata/image_5_image_data.h"
#include "tensorflow/lite/micro/micro_log.h"

static inline void RawPutu(char c) {
  volatile uint32_t* const uart_tx =
      reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
  *uart_tx = static_cast<uint32_t>(static_cast<uint8_t>(c));
}


TfLiteStatus GetImage(int image_width, int image_height, int channels,
                      int8_t* image_data) {
  if (image_data == nullptr) {
    // RawPutu('Q');
    // MicroPrintf("GetImage: null buffer\n");
    return kTfLiteError;
  }

  if (image_width != kNumCols || image_height != kNumRows ||
      channels != kNumChannels) {
      // RawPutu('R');
    // MicroPrintf("GetImage: bad shape %d x %d x %d\n",
    //             image_width, image_height, channels);
    return kTfLiteError;
  }

  static int which = 2;
  const int image_size = image_width * image_height * channels;

  if (which == 0) {
    // RawPutu('P');
    if (g_person_image_data_size != image_size) {
      // RawPutu('S');
      // MicroPrintf("person image size mismatch\n");
      return kTfLiteError;
    }
    memcpy(image_data, g_person_image_data, image_size);
      // RawPutu('T');
    // MicroPrintf("GetImage: person test image\n");
  } else if (which == 1) {
    // RawPutu('U');
    if (g_no_person_image_data_size != image_size) {
      // RawPutu('U');
      // MicroPrintf("no_person image size mismatch\n");
      return kTfLiteError;
    }
    memcpy(image_data, g_no_person_image_data, image_size);
    // RawPutu('V');
    // MicroPrintf("GetImage: no_person test image\n");
  }
  else if (which == 2) {
    //   if (g_image_3_image_data_size != image_size) {
    //   // RawPutu('U');
    //   // MicroPrintf("no_person image size mismatch\n");
    //   return kTfLiteError;
    // }
    memcpy(image_data, g_image_3_image_data, image_size);
  }

  else if (which == 3) {
    //   if (g_image_3_image_data_size != image_size) {
    //   // RawPutu('U');
    //   // MicroPrintf("no_person image size mismatch\n");
    //   return kTfLiteError;
    // }
    memcpy(image_data, g_image_4_image_data, image_size);
  }

  else if (which == 4) {
    //   if (g_image_3_image_data_size != image_size) {
    //   // RawPutu('U');
    //   // MicroPrintf("no_person image size mismatch\n");
    //   return kTfLiteError;
    // }
    memcpy(image_data, g_image_5_image_data, image_size);
  }

  // which ^= 1;

  which++;
  if (which >= 4) {
    which = 0;
  }
  return kTfLiteOk;
}