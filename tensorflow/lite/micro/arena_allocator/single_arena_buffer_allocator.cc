/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

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

// #include "tensorflow/lite/micro/arena_allocator/single_arena_buffer_allocator.h"

// #include <cstddef>
// #include <cstdint>
// #include <new>

// #include "tensorflow/lite/c/c_api_types.h"
// #include "tensorflow/lite/c/common.h"
// #include "tensorflow/lite/kernels/internal/compatibility.h"
// #include "tensorflow/lite/kernels/op_macros.h"
// #include "tensorflow/lite/micro/memory_helpers.h"
// #include "tensorflow/lite/micro/micro_log.h"

// namespace tflite {

// SingleArenaBufferAllocator::SingleArenaBufferAllocator(uint8_t* buffer_head,
//                                                        uint8_t* buffer_tail)
//     : buffer_head_(buffer_head),
//       buffer_tail_(buffer_tail),
//       head_(buffer_head),
//       tail_(buffer_tail),
//       temp_(buffer_head_) {}

// SingleArenaBufferAllocator::SingleArenaBufferAllocator(uint8_t* buffer,
//                                                        size_t buffer_size)
//     : SingleArenaBufferAllocator(buffer, buffer + buffer_size) {}

// /* static */
// SingleArenaBufferAllocator* SingleArenaBufferAllocator::Create(
//     uint8_t* buffer_head, size_t buffer_size) {
//   TFLITE_DCHECK(buffer_head != nullptr);
//   SingleArenaBufferAllocator tmp =
//       SingleArenaBufferAllocator(buffer_head, buffer_size);

//   // Allocate enough bytes from the buffer to create a
//   // SingleArenaBufferAllocator. The new instance will use the current adjusted
//   // tail buffer from the tmp allocator instance.
//   uint8_t* allocator_buffer = tmp.AllocatePersistentBuffer(
//       sizeof(SingleArenaBufferAllocator), alignof(SingleArenaBufferAllocator));
//   // Use the default copy constructor to populate internal states.
//   return new (allocator_buffer) SingleArenaBufferAllocator(tmp);
// }

// SingleArenaBufferAllocator::~SingleArenaBufferAllocator() {}

// uint8_t* SingleArenaBufferAllocator::AllocateResizableBuffer(size_t size,
//                                                              size_t alignment) {
//   // Only supports one resizable buffer, which starts at the buffer head.
//   uint8_t* expect_resizable_buf = AlignPointerUp(buffer_head_, alignment);
//   if (ResizeBuffer(expect_resizable_buf, size, alignment) == kTfLiteOk) {
//     return expect_resizable_buf;
//   }
//   return nullptr;
// }

// TfLiteStatus SingleArenaBufferAllocator::DeallocateResizableBuffer(
//     uint8_t* resizable_buf) {
//   return ResizeBuffer(resizable_buf, 0, 1);
// }

// TfLiteStatus SingleArenaBufferAllocator::ReserveNonPersistentOverlayMemory(
//     size_t size, size_t alignment) {
//   uint8_t* expect_resizable_buf = AlignPointerUp(buffer_head_, alignment);
//   return ResizeBuffer(expect_resizable_buf, size, alignment);
// }

// TfLiteStatus SingleArenaBufferAllocator::ResizeBuffer(uint8_t* resizable_buf,
//                                                       size_t size,
//                                                       size_t alignment) {
//   // Only supports one resizable buffer, which starts at the buffer head.
//   uint8_t* expect_resizable_buf = AlignPointerUp(buffer_head_, alignment);
//   if (head_ != temp_ || resizable_buf != expect_resizable_buf) {
//     MicroPrintf(
//         "Internal error: either buffer is not resizable or "
//         "ResetTempAllocations() is not called before ResizeBuffer().");
//     return kTfLiteError;
//   }

//   uint8_t* const aligned_result = AlignPointerUp(buffer_head_, alignment);
//   const size_t available_memory = tail_ - aligned_result;
//   if (available_memory < size) {
//     MicroPrintf(
//         "Failed to resize buffer. Requested: %u, available %u, missing: %u",
//         size, available_memory, size - available_memory);
//     return kTfLiteError;
//   }
//   head_ = aligned_result + size;
//   temp_ = head_;

//   return kTfLiteOk;
// }

// uint8_t* SingleArenaBufferAllocator::AllocatePersistentBuffer(
//     size_t size, size_t alignment) {
//   uint8_t* const aligned_result = AlignPointerDown(tail_ - size, alignment);
//   if (aligned_result < head_) {
// #ifndef TF_LITE_STRIP_ERROR_STRINGS
//     const size_t missing_memory = head_ - aligned_result;
//     MicroPrintf(
//         "Failed to allocate tail memory. Requested: %u, "
//         "available %u, missing: %u",
//         size, size - missing_memory, missing_memory);
// #endif
//     return nullptr;
//   }
//   tail_ = aligned_result;
//   return aligned_result;
// }

// uint8_t* SingleArenaBufferAllocator::AllocateTemp(size_t size,
//                                                   size_t alignment) {
//   uint8_t* const aligned_result = AlignPointerUp(temp_, alignment);
//   const size_t available_memory = tail_ - aligned_result;
//   if (available_memory < size) {
//     MicroPrintf(
//         "Failed to allocate temp memory. Requested: %u, "
//         "available %u, missing: %u",
//         size, available_memory, size - available_memory);
//     return nullptr;
//   }
//   temp_ = aligned_result + size;
//   temp_buffer_ptr_check_sum_ ^= (reinterpret_cast<intptr_t>(aligned_result));
//   temp_buffer_count_++;
//   return aligned_result;
// }

// void SingleArenaBufferAllocator::DeallocateTemp(uint8_t* temp_buf) {
//   temp_buffer_ptr_check_sum_ ^= (reinterpret_cast<intptr_t>(temp_buf));
//   temp_buffer_count_--;
// }

// bool SingleArenaBufferAllocator::IsAllTempDeallocated() {
//   if (temp_buffer_count_ != 0 || temp_buffer_ptr_check_sum_ != 0) {
//     MicroPrintf(
//         "Number of allocated temp buffers: %d. Checksum passing status: %d",
//         temp_buffer_count_, !temp_buffer_ptr_check_sum_);
//     return false;
//   }
//   return true;
// }

// TfLiteStatus SingleArenaBufferAllocator::ResetTempAllocations() {
//   // TODO(b/209453859): enable error check based on IsAllTempDeallocated after
//   // all AllocateTemp have been paird with DeallocateTemp
//   if (!IsAllTempDeallocated()) {
//     MicroPrintf(
//         "All temp buffers must be freed before calling ResetTempAllocations()");
//     return kTfLiteError;
//   }
//   temp_ = head_;
//   return kTfLiteOk;
// }

// uint8_t* SingleArenaBufferAllocator::GetOverlayMemoryAddress() const {
//   return buffer_head_;
// }

// size_t SingleArenaBufferAllocator::GetNonPersistentUsedBytes() const {
//   return std::max(head_ - buffer_head_, temp_ - buffer_head_);
// }

// size_t SingleArenaBufferAllocator::GetPersistentUsedBytes() const {
//   return buffer_tail_ - tail_;
// }

// size_t SingleArenaBufferAllocator::GetAvailableMemory(size_t alignment) const {
//   uint8_t* const aligned_temp = AlignPointerUp(temp_, alignment);
//   uint8_t* const aligned_tail = AlignPointerDown(tail_, alignment);
//   return aligned_tail - aligned_temp;
// }

// size_t SingleArenaBufferAllocator::GetUsedBytes() const {
//   return GetPersistentUsedBytes() + GetNonPersistentUsedBytes();
// }

// size_t SingleArenaBufferAllocator::GetBufferSize() const {
//   return buffer_tail_ - buffer_head_;
// }

// uint8_t* SingleArenaBufferAllocator::head() const { return head_; }

// uint8_t* SingleArenaBufferAllocator::tail() const { return tail_; }

// }  // namespace tflite


#include "tensorflow/lite/micro/arena_allocator/single_arena_buffer_allocator.h"

#include <cstddef>
#include <cstdint>
#include <new>

#include "tensorflow/lite/c/c_api_types.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/kernels/internal/compatibility.h"
#include "tensorflow/lite/kernels/op_macros.h"
#include "tensorflow/lite/micro/memory_helpers.h"
#include "tensorflow/lite/micro/micro_log.h"

#include <stdint.h>

static inline void RawPutc(char c) {
  volatile uint32_t* const uart_tx =
      reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x04u);
  volatile uint32_t* const uart_status =
      reinterpret_cast<volatile uint32_t*>(0x40600000u + 0x08u);

  while ((*uart_status) & 0x08u) {
  }

  *uart_tx = static_cast<uint32_t>(static_cast<uint8_t>(c));
}

static inline void RawNewline() {
  RawPutc('\r');
  RawPutc('\n');
}

static inline void RawPutHexNibble(uint32_t v) {
  v &= 0xFu;
  RawPutc((v < 10u) ? static_cast<char>('0' + v)
                    : static_cast<char>('A' + (v - 10u)));
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

namespace tflite {

// SingleArenaBufferAllocator::SingleArenaBufferAllocator(uint8_t* buffer_head,
//                                                        uint8_t* buffer_tail)
//     : buffer_head_(buffer_head),
//       buffer_tail_(buffer_tail),
//       head_(buffer_head),
//       tail_(buffer_tail),
//       temp_(buffer_head_) {}

SingleArenaBufferAllocator::SingleArenaBufferAllocator(uint8_t* buffer_head,
                                                       uint8_t* buffer_tail)
    : buffer_head_(buffer_head),
      buffer_tail_(buffer_tail),
      head_(buffer_head),
      tail_(buffer_tail),
      temp_(buffer_head_),
      temp_buffer_ptr_check_sum_(0),
      temp_buffer_count_(0) {}

SingleArenaBufferAllocator::SingleArenaBufferAllocator(uint8_t* buffer,
                                                       size_t buffer_size)
    : SingleArenaBufferAllocator(buffer, buffer + buffer_size) {}

/* static */
// SingleArenaBufferAllocator* SingleArenaBufferAllocator::Create(
//     uint8_t* buffer_head, size_t buffer_size) {
//   TFLITE_DCHECK(buffer_head != nullptr);
//   SingleArenaBufferAllocator tmp =
//       SingleArenaBufferAllocator(buffer_head, buffer_size);

//   // Allocate enough bytes from the buffer to create a
//   // SingleArenaBufferAllocator. The new instance will use the current adjusted
//   // tail buffer from the tmp allocator instance.
//   uint8_t* allocator_buffer = tmp.AllocatePersistentBuffer(
//       sizeof(SingleArenaBufferAllocator), alignof(SingleArenaBufferAllocator));
//   // Use the default copy constructor to populate internal states.
//   return new (allocator_buffer) SingleArenaBufferAllocator(tmp);
// }


// __attribute__((noinline))
// SingleArenaBufferAllocator* SingleArenaBufferAllocator::Create(
//     uint8_t* buffer_head, size_t buffer_size) {
//   uint8_t* local_buffer_head = buffer_head;
//   size_t local_buffer_size = buffer_size;

//   RawPutc('['); RawPutc('S'); RawPutc('A'); RawPutc('0'); RawPutc(']');
//   RawTagHex('a', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(local_buffer_head)));
//   RawTagHex('b', static_cast<uint32_t>(local_buffer_size));
//   RawNewline();

//   if (local_buffer_head == nullptr) {
//     MicroPrintf("Null arena\n");
//     while (1) {
//     }
//   }

//   SingleArenaBufferAllocator tmp(local_buffer_head, local_buffer_size);


//   SingleArenaBufferAllocator* allocator =
//       reinterpret_cast<SingleArenaBufferAllocator*>(
//           tmp.AllocatePersistentBuffer(sizeof(SingleArenaBufferAllocator),
//                                        alignof(SingleArenaBufferAllocator)));



//   if (allocator == nullptr) {

//     return nullptr;
//   }

//   allocator->head_ = tmp.head_;
//   allocator->tail_ = tmp.tail_;
//   allocator->buffer_head_ = tmp.buffer_head_;
//   allocator->buffer_tail_ = tmp.buffer_tail_;
//   allocator->temp_ = tmp.temp_;
//   allocator->temp_buffer_ptr_check_sum_ = tmp.temp_buffer_ptr_check_sum_;
//   allocator->temp_buffer_count_ = tmp.temp_buffer_count_;



//   return allocator;
// }

__attribute__((noinline))
SingleArenaBufferAllocator* SingleArenaBufferAllocator::Create(
    uint8_t* buffer_head, size_t buffer_size) {
  uint8_t* local_buffer_head = buffer_head;
  size_t local_buffer_size = buffer_size;

  // RawPutc('['); RawPutc('S'); RawPutc('A'); RawPutc('0'); RawPutc(']');
  // RawTagHex('a', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(local_buffer_head)));
  // RawTagHex('b', static_cast<uint32_t>(local_buffer_size));
  // RawNewline();

  if (local_buffer_head == nullptr) {
    MicroPrintf("Null arena\n");
    while (1) {}
  }

  SingleArenaBufferAllocator tmp(local_buffer_head, local_buffer_size);

  uint8_t* allocator_buffer =
      tmp.AllocatePersistentBuffer(sizeof(SingleArenaBufferAllocator),
                                   alignof(SingleArenaBufferAllocator));

  if (allocator_buffer == nullptr) {
    return nullptr;
  }

  SingleArenaBufferAllocator* allocator =
      new (allocator_buffer) SingleArenaBufferAllocator(tmp);

  // Extra explicit bring-up safety.
  allocator->temp_buffer_ptr_check_sum_ = 0;
  allocator->temp_buffer_count_ = 0;

  return allocator;
}

SingleArenaBufferAllocator::~SingleArenaBufferAllocator() {}

uint8_t* SingleArenaBufferAllocator::AllocateResizableBuffer(size_t size,
                                                             size_t alignment) {
  // Only supports one resizable buffer, which starts at the buffer head.
  uint8_t* expect_resizable_buf = AlignPointerUp(buffer_head_, alignment);
  if (ResizeBuffer(expect_resizable_buf, size, alignment) == kTfLiteOk) {
    return expect_resizable_buf;
  }
  return nullptr;
}

__attribute__((noinline))
size_t SingleArenaBufferAllocator::GetAvailableMemoryDirect(
    size_t alignment) const {
  uint8_t* const aligned_temp = AlignPointerUp(temp_, alignment);
  uint8_t* const aligned_tail = AlignPointerDown(tail_, alignment);
  return aligned_tail - aligned_temp;
}

__attribute__((noinline))
uint8_t* SingleArenaBufferAllocator::GetOverlayMemoryAddressDirect() const {
  return buffer_head_;
}

TfLiteStatus SingleArenaBufferAllocator::DeallocateResizableBuffer(
    uint8_t* resizable_buf) {
  return ResizeBuffer(resizable_buf, 0, 1);
}

__attribute__((noinline))
TfLiteStatus SingleArenaBufferAllocator::DeallocateResizableBufferDirect(
    uint8_t* resizable_buf) {
  return ResizeBufferDirect(resizable_buf, 0, 1);
}

TfLiteStatus SingleArenaBufferAllocator::ReserveNonPersistentOverlayMemory(
    size_t size, size_t alignment) {
  uint8_t* expect_resizable_buf = AlignPointerUp(buffer_head_, alignment);
  return ResizeBuffer(expect_resizable_buf, size, alignment);
}

__attribute__((noinline))
TfLiteStatus SingleArenaBufferAllocator::ReserveNonPersistentOverlayMemoryDirect(
    size_t size, size_t alignment) {
  uint8_t* expect_resizable_buf = AlignPointerUp(buffer_head_, alignment);
  return ResizeBufferDirect(expect_resizable_buf, size, alignment);
}

TfLiteStatus SingleArenaBufferAllocator::ResizeBuffer(uint8_t* resizable_buf,
                                                      size_t size,
                                                      size_t alignment) {
  // Only supports one resizable buffer, which starts at the buffer head.
  uint8_t* expect_resizable_buf = AlignPointerUp(buffer_head_, alignment);
  if (head_ != temp_ || resizable_buf != expect_resizable_buf) {
    MicroPrintf(
        "Internal error: either buffer is not resizable or "
        "ResetTempAllocations() is not called before ResizeBuffer().");
    return kTfLiteError;
  }



  uint8_t* const aligned_result = AlignPointerUp(buffer_head_, alignment);
  const size_t available_memory = tail_ - aligned_result;
  if (available_memory < size) {
    MicroPrintf(
        "Failed to resize buffer. Requested: %u, available %u, missing: %u",
        size, available_memory, size - available_memory);
    return kTfLiteError;
  }
  head_ = aligned_result + size;
  temp_ = head_;

  return kTfLiteOk;
}


  __attribute__((noinline))
TfLiteStatus SingleArenaBufferAllocator::ResizeBufferDirect(
    uint8_t* resizable_buf, size_t size, size_t alignment) {
  // RawPutc('['); RawPutc('R'); RawPutc('Z'); RawPutc('0'); RawPutc(']');
  // RawTagHex('b', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(buffer_head_)));
  // RawTagHex('t', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(buffer_tail_)));
  // RawTagHex('h', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(head_)));
  // RawTagHex('l', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tail_)));
  // RawTagHex('m', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(temp_)));
  // RawTagHex('r', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(resizable_buf)));
  // RawTagHex('s', static_cast<uint32_t>(size));
  // RawTagHex('a', static_cast<uint32_t>(alignment));
  // RawNewline();

  uint8_t* expect_resizable_buf = AlignPointerUp(buffer_head_, alignment);

  // RawPutc('['); RawPutc('R'); RawPutc('Z'); RawPutc('1'); RawPutc(']');
  // RawTagHex('e', static_cast<uint32_t>(
  //                    reinterpret_cast<uintptr_t>(expect_resizable_buf)));
  // RawNewline();

  if (head_ != temp_ || resizable_buf != expect_resizable_buf) {
    // RawPutc('['); RawPutc('R'); RawPutc('Z'); RawPutc('X'); RawPutc(']');
    // RawTagHex('h', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(head_)));
    // RawTagHex('m', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(temp_)));
    // RawTagHex('r', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(resizable_buf)));
    // RawTagHex('e', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(expect_resizable_buf)));
    // RawNewline();
    return kTfLiteError;
  }

  uint8_t* const aligned_result = AlignPointerUp(buffer_head_, alignment);

  // RawPutc('['); RawPutc('R'); RawPutc('Z'); RawPutc('2'); RawPutc(']');
  // RawTagHex('u', static_cast<uint32_t>(
  //                    reinterpret_cast<uintptr_t>(aligned_result)));
  // RawNewline();

  const size_t available_memory = tail_ - aligned_result;

  // RawPutc('['); RawPutc('R'); RawPutc('Z'); RawPutc('3'); RawPutc(']');
  // RawTagHex('v', static_cast<uint32_t>(available_memory));
  // RawNewline();

  if (available_memory < size) {
    // RawPutc('['); RawPutc('R'); RawPutc('Z'); RawPutc('M'); RawPutc(']');
    // RawTagHex('s', static_cast<uint32_t>(size));
    // RawTagHex('v', static_cast<uint32_t>(available_memory));
    // RawNewline();
    return kTfLiteError;
  }

  head_ = aligned_result + size;
  temp_ = head_;

  // RawPutc('['); RawPutc('R'); RawPutc('Z'); RawPutc('4'); RawPutc(']');
  // RawTagHex('h', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(head_)));
  // RawTagHex('m', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(temp_)));
  // RawNewline();

  return kTfLiteOk;
}


// uint8_t* SingleArenaBufferAllocator::AllocatePersistentBuffer(
//     size_t size, size_t alignment) {
//   uint8_t* const aligned_result = AlignPointerDown(tail_ - size, alignment);
//   if (aligned_result < head_) {
// #ifndef TF_LITE_STRIP_ERROR_STRINGS
//     const size_t missing_memory = head_ - aligned_result;
//     MicroPrintf(
//         "Failed to allocate tail memory. Requested: %u, "
//         "available %u, missing: %u",
//         size, size - missing_memory, missing_memory);
// #endif
//     return nullptr;
//   }
//   tail_ = aligned_result;
//   return aligned_result;
// }

// __attribute__((noinline))
// uint8_t* SingleArenaBufferAllocator::AllocatePersistentBufferDirect(
//     size_t size, size_t alignment) {
//   size_t local_size = size;
//   size_t local_alignment = alignment;
//   uint8_t* local_head = head_;
//   uint8_t* local_tail = tail_;

//   RawPutc('['); RawPutc('A'); RawPutc('D'); RawPutc('0'); RawPutc(']');
//   RawTagHex('h', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(local_head)));
//   RawTagHex('t', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(local_tail)));
//   RawTagHex('s', static_cast<uint32_t>(local_size));
//   RawTagHex('a', static_cast<uint32_t>(local_alignment));
//   RawNewline();

//   uint8_t* const aligned_result = AlignPointerUp(local_head, local_alignment);
//   uint8_t* const next_head = aligned_result + local_size;

//   RawPutc('['); RawPutc('A'); RawPutc('D'); RawPutc('1'); RawPutc(']');
//   RawTagHex('r', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(aligned_result)));
//   RawTagHex('n', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(next_head)));
//   RawNewline();

//   if (next_head > local_tail) {
//     RawPutc('['); RawPutc('A'); RawPutc('D'); RawPutc('X'); RawPutc(']');
//     RawNewline();
//     return nullptr;
//   }

//   head_ = next_head;

//   RawPutc('['); RawPutc('A'); RawPutc('D'); RawPutc('2'); RawPutc(']');
//   RawTagHex('h', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(head_)));
//   RawNewline();

//   return aligned_result;
// }


__attribute__((noinline))
uint8_t* SingleArenaBufferAllocator::AllocatePersistentBufferDirect(
    size_t size, size_t alignment) {
  size_t local_size = size;
  size_t local_alignment = alignment;
  uint8_t* local_head = head_;
  uint8_t* local_tail = tail_;

  // RawPutc('['); RawPutc('A'); RawPutc('D'); RawPutc('0'); RawPutc(']');
  // RawTagHex('h', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(local_head)));
  // RawTagHex('t', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(local_tail)));
  // RawTagHex('s', static_cast<uint32_t>(local_size));
  // RawTagHex('a', static_cast<uint32_t>(local_alignment));
  // RawNewline();

  uint8_t* const aligned_result =
      AlignPointerDown(local_tail - local_size, local_alignment);

  // RawPutc('['); RawPutc('A'); RawPutc('D'); RawPutc('1'); RawPutc(']');
  // RawTagHex('r', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(aligned_result)));
  // RawNewline();

  if (aligned_result < local_head) {
    // RawPutc('['); RawPutc('A'); RawPutc('D'); RawPutc('X'); RawPutc(']');
    // RawNewline();
    return nullptr;
  }

  tail_ = aligned_result;

  // RawPutc('['); RawPutc('A'); RawPutc('D'); RawPutc('2'); RawPutc(']');
  // RawTagHex('t', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tail_)));
  // RawNewline();

  return aligned_result;
}

// uint8_t* SingleArenaBufferAllocator::AllocatePersistentBuffer(size_t size,
//                                                               size_t alignment) {
//   RawPutc('['); RawPutc('A'); RawPutc('P'); RawPutc('0'); RawPutc(']');
//   RawTagHex('h', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(head_)));
//   RawTagHex('t', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tail_)));
//   RawTagHex('s', static_cast<uint32_t>(size));
//   RawTagHex('a', static_cast<uint32_t>(alignment));
//   RawNewline();

//   uint8_t* const aligned_result = AlignPointerUp(head_, alignment);
//   uint8_t* const next_head = aligned_result + size;



//   if (next_head > tail_) {

//     MicroPrintf("Failed to allocate memory for persistent buffer data.");
//     return nullptr;
//   }

//   head_ = next_head;

//   RawPutc('['); RawPutc('A'); RawPutc('P'); RawPutc('2'); RawPutc(']');
//   RawTagHex('h', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(head_)));
//   RawNewline();

//   return aligned_result;
// }


uint8_t* SingleArenaBufferAllocator::AllocatePersistentBuffer(
    size_t size, size_t alignment) {
  // RawPutc('['); RawPutc('A'); RawPutc('P'); RawPutc('0'); RawPutc(']');
  // RawTagHex('h', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(head_)));
  // RawTagHex('t', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tail_)));
  // RawTagHex('s', static_cast<uint32_t>(size));
  // RawTagHex('a', static_cast<uint32_t>(alignment));
  // RawNewline();

  uint8_t* const aligned_result =
      AlignPointerDown(tail_ - size, alignment);

  // RawPutc('['); RawPutc('A'); RawPutc('P'); RawPutc('1'); RawPutc(']');
  // RawTagHex('r', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(aligned_result)));
  // RawNewline();

  if (aligned_result < head_) {
    // RawPutc('['); RawPutc('A'); RawPutc('P'); RawPutc('X'); RawPutc(']');
    // RawNewline();
    return nullptr;
  }

  tail_ = aligned_result;

  // RawPutc('['); RawPutc('A'); RawPutc('P'); RawPutc('2'); RawPutc(']');
  // RawTagHex('t', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tail_)));
  // RawNewline();

  return aligned_result;
}

__attribute__((noinline))
uint8_t* SingleArenaBufferAllocator::AllocateResizableBufferDirect(
    size_t size, size_t alignment) {
  size_t local_size = size;
  size_t local_alignment = alignment;

  // RawPutc('['); RawPutc('R'); RawPutc('D'); RawPutc('0'); RawPutc(']');
  // RawTagHex('b', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(buffer_head_)));
  // RawTagHex('t', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(buffer_tail_)));
  // RawTagHex('h', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(head_)));
  // RawTagHex('l', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tail_)));
  // RawTagHex('m', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(temp_)));
  // RawTagHex('s', static_cast<uint32_t>(local_size));
  // RawTagHex('a', static_cast<uint32_t>(local_alignment));
  // RawNewline();

  uint8_t* expect_resizable_buf = AlignPointerUp(buffer_head_, local_alignment);

  // RawPutc('['); RawPutc('R'); RawPutc('D'); RawPutc('1'); RawPutc(']');
  // RawTagHex('e', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(expect_resizable_buf)));
  // RawNewline();

  TfLiteStatus st = ResizeBufferDirect(expect_resizable_buf, local_size, local_alignment);

  // RawPutc('['); RawPutc('R'); RawPutc('D'); RawPutc('2'); RawPutc(']');
  // RawTagHex('s', static_cast<uint32_t>(st));
  // RawTagHex('h', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(head_)));
  // RawTagHex('m', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(temp_)));
  // RawNewline();

  if (st == kTfLiteOk) {
    return expect_resizable_buf;
  }
  return nullptr;
}

__attribute__((noinline))
uint8_t* SingleArenaBufferAllocator::AllocateTempDirect(
    size_t size, size_t alignment) {
  // RawPutc('['); RawPutc('A'); RawPutc('T'); RawPutc('0'); RawPutc(']');
  // RawTagHex('h', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(head_)));
  // RawTagHex('t', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tail_)));
  // RawTagHex('m', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(temp_)));
  // RawTagHex('s', static_cast<uint32_t>(size));
  // RawTagHex('a', static_cast<uint32_t>(alignment));
  // RawNewline();

  uint8_t* const aligned_result = AlignPointerUp(temp_, alignment);

  // RawPutc('['); RawPutc('A'); RawPutc('T'); RawPutc('1'); RawPutc(']');
  // RawTagHex('r', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(aligned_result)));
  // RawNewline();

  const size_t available_memory = tail_ - aligned_result;
  if (available_memory < size) {
    // RawPutc('['); RawPutc('A'); RawPutc('T'); RawPutc('X'); RawPutc(']');
    // RawTagHex('v', static_cast<uint32_t>(available_memory));
    // RawNewline();
    return nullptr;
  }

  temp_ = aligned_result + size;
  temp_buffer_ptr_check_sum_ ^= reinterpret_cast<intptr_t>(aligned_result);
  temp_buffer_count_++;

  // RawPutc('['); RawPutc('A'); RawPutc('T'); RawPutc('2'); RawPutc(']');
  // RawTagHex('m', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(temp_)));
  // RawNewline();

  return aligned_result;
}

__attribute__((noinline))
void SingleArenaBufferAllocator::DeallocateTempDirect(uint8_t* temp_buf) {
  // RawPutc('['); RawPutc('A'); RawPutc('F'); RawPutc('0'); RawPutc(']');
  // RawTagHex('p', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(temp_buf)));
  // RawNewline();

  temp_buffer_ptr_check_sum_ ^= reinterpret_cast<intptr_t>(temp_buf);
  temp_buffer_count_--;

  // RawPutc('['); RawPutc('A'); RawPutc('F'); RawPutc('1'); RawPutc(']');
  // RawNewline();
}

__attribute__((noinline))
TfLiteStatus SingleArenaBufferAllocator::ResetTempAllocationsDirect() {
  // RawPutc('['); RawPutc('A'); RawPutc('R'); RawPutc('0'); RawPutc(']');
  // RawTagHex('c', static_cast<uint32_t>(temp_buffer_count_));
  // RawTagHex('x', static_cast<uint32_t>(temp_buffer_ptr_check_sum_));
  // RawNewline();

  if (!IsAllTempDeallocated()) {
    // RawPutc('['); RawPutc('A'); RawPutc('R'); RawPutc('X'); RawPutc(']');
    // RawNewline();
    return kTfLiteError;
  }

  temp_ = head_;

  // RawPutc('['); RawPutc('A'); RawPutc('R'); RawPutc('1'); RawPutc(']');
  // RawTagHex('m', static_cast<uint32_t>(reinterpret_cast<uintptr_t>(temp_)));
  // RawNewline();

  return kTfLiteOk;
}

uint8_t* SingleArenaBufferAllocator::AllocateTemp(size_t size,
                                                  size_t alignment) {
  uint8_t* const aligned_result = AlignPointerUp(temp_, alignment);
  const size_t available_memory = tail_ - aligned_result;
  if (available_memory < size) {
    MicroPrintf(
        "Failed to allocate temp memory. Requested: %u, "
        "available %u, missing: %u",
        size, available_memory, size - available_memory);
    return nullptr;
  }
  temp_ = aligned_result + size;
  temp_buffer_ptr_check_sum_ ^= (reinterpret_cast<intptr_t>(aligned_result));
  temp_buffer_count_++;
  return aligned_result;
}

void SingleArenaBufferAllocator::DeallocateTemp(uint8_t* temp_buf) {
  temp_buffer_ptr_check_sum_ ^= (reinterpret_cast<intptr_t>(temp_buf));
  temp_buffer_count_--;
}

bool SingleArenaBufferAllocator::IsAllTempDeallocated() {
  if (temp_buffer_count_ != 0 || temp_buffer_ptr_check_sum_ != 0) {
    MicroPrintf(
        "Number of allocated temp buffers: %d. Checksum passing status: %d",
        temp_buffer_count_, !temp_buffer_ptr_check_sum_);
    return false;
  }
  return true;
}

TfLiteStatus SingleArenaBufferAllocator::ResetTempAllocations() {
  // TODO(b/209453859): enable error check based on IsAllTempDeallocated after
  // all AllocateTemp have been paird with DeallocateTemp
  if (!IsAllTempDeallocated()) {
    MicroPrintf(
        "All temp buffers must be freed before calling ResetTempAllocations()");
    return kTfLiteError;
  }
  temp_ = head_;
  return kTfLiteOk;
}

uint8_t* SingleArenaBufferAllocator::GetOverlayMemoryAddress() const {
  return buffer_head_;
}

size_t SingleArenaBufferAllocator::GetNonPersistentUsedBytes() const {
  return std::max(head_ - buffer_head_, temp_ - buffer_head_);
}

size_t SingleArenaBufferAllocator::GetPersistentUsedBytes() const {
  return buffer_tail_ - tail_;
}

size_t SingleArenaBufferAllocator::GetAvailableMemory(size_t alignment) const {
  uint8_t* const aligned_temp = AlignPointerUp(temp_, alignment);
  uint8_t* const aligned_tail = AlignPointerDown(tail_, alignment);
  return aligned_tail - aligned_temp;
}

size_t SingleArenaBufferAllocator::GetUsedBytes() const {
  return GetPersistentUsedBytes() + GetNonPersistentUsedBytes();
}

size_t SingleArenaBufferAllocator::GetBufferSize() const {
  return buffer_tail_ - buffer_head_;
}

uint8_t* SingleArenaBufferAllocator::head() const { return head_; }

uint8_t* SingleArenaBufferAllocator::tail() const { return tail_; }

}  // namespace tflite
