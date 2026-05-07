// /* Copyright 2022 The TensorFlow Authors. All Rights Reserved.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ==============================================================================*/

// #include "tensorflow/lite/c/common.h"
// #include "tensorflow/lite/micro/examples/person_detection/model_settings.h"
// #include "tensorflow/lite/micro/examples/person_detection/testdata/no_person_image_data.h"
// #include "tensorflow/lite/micro/examples/person_detection/testdata/person_image_data.h"
// #include "tensorflow/lite/micro/micro_interpreter.h"
// #include "tensorflow/lite/micro/micro_log.h"
// #include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
// #include "tensorflow/lite/micro/models/person_detect_model_data.h"
// #include "tensorflow/lite/micro/testing/micro_test_v2.h"
// #include "tensorflow/lite/schema/schema_generated.h"

// // Create an area of memory to use for input, output, and intermediate arrays.
// #if defined(XTENSA) && defined(VISION_P6)
// constexpr int tensor_arena_size = 352 * 1024;
// #else
// constexpr int tensor_arena_size = 136 * 1024;
// #endif  // defined(XTENSA) && defined(VISION_P6)
// uint8_t tensor_arena[tensor_arena_size];

// TEST(PersonDetectionTest, TestInvoke) {
//   // Map the model into a usable data structure. This doesn't involve any
//   // copying or parsing, it's a very lightweight operation.
//   const tflite::Model* model = ::tflite::GetModel(g_person_detect_model_data);
//   if (model->version() != TFLITE_SCHEMA_VERSION) {
//     MicroPrintf(
//         "Model provided is schema version %d not equal "
//         "to supported version %d.\n",
//         model->version(), TFLITE_SCHEMA_VERSION);
//   }

//   // Pull in only the operation implementations we need.
//   // This relies on a complete list of all the ops needed by this graph.
//   // An easier approach is to just use the AllOpsResolver, but this will
//   // incur some penalty in code space for op implementations that are not
//   // needed by this graph.
//   tflite::MicroMutableOpResolver<5> micro_op_resolver;
//   micro_op_resolver.AddAveragePool2D(tflite::Register_AVERAGE_POOL_2D_INT8());
//   micro_op_resolver.AddConv2D(tflite::Register_CONV_2D_INT8());
//   micro_op_resolver.AddDepthwiseConv2D(
//       tflite::Register_DEPTHWISE_CONV_2D_INT8());
//   micro_op_resolver.AddReshape();
//   micro_op_resolver.AddSoftmax(tflite::Register_SOFTMAX_INT8());

//   // Build an interpreter to run the model with.
//   tflite::MicroInterpreter interpreter(model, micro_op_resolver, tensor_arena,
//                                        tensor_arena_size);
//   interpreter.AllocateTensors();

//   // Get information about the memory area to use for the model's input.
//   TfLiteTensor* input = interpreter.input(0);

//   // Make sure the input has the properties we expect.
//   EXPECT_NE(input, nullptr);
//   EXPECT_EQ(4, input->dims->size);
//   EXPECT_EQ(1, input->dims->data[0]);
//   EXPECT_EQ(kNumRows, input->dims->data[1]);
//   EXPECT_EQ(kNumCols, input->dims->data[2]);
//   EXPECT_EQ(kNumChannels, input->dims->data[3]);
//   EXPECT_EQ(kTfLiteInt8, input->type);

//   // Copy an image with a person into the memory area used for the input.
//   TFLITE_DCHECK_EQ(input->bytes, static_cast<size_t>(g_person_image_data_size));
//   memcpy(input->data.int8, g_person_image_data, input->bytes);

//   // Run the model on this input and make sure it succeeds.
//   TfLiteStatus invoke_status = interpreter.Invoke();
//   if (invoke_status != kTfLiteOk) {
//     MicroPrintf("Invoke failed\n");
//   }
//   EXPECT_EQ(kTfLiteOk, invoke_status);

//   // Get the output from the model, and make sure it's the expected size and
//   // type.
//   TfLiteTensor* output = interpreter.output(0);
//   EXPECT_EQ(2, output->dims->size);
//   EXPECT_EQ(1, output->dims->data[0]);
//   EXPECT_EQ(kCategoryCount, output->dims->data[1]);
//   EXPECT_EQ(kTfLiteInt8, output->type);

//   // Make sure that the expected "Person" score is higher than the other class.
//   int8_t person_score = output->data.int8[kPersonIndex];
//   int8_t no_person_score = output->data.int8[kNotAPersonIndex];
//   MicroPrintf("person data.  person score: %d, no person score: %d\n",
//               person_score, no_person_score);
//   EXPECT_GT(person_score, no_person_score);

//   memcpy(input->data.int8, g_no_person_image_data, input->bytes);

//   // Run the model on this "No Person" input.
//   invoke_status = interpreter.Invoke();
//   if (invoke_status != kTfLiteOk) {
//     MicroPrintf("Invoke failed\n");
//   }
//   EXPECT_EQ(kTfLiteOk, invoke_status);

//   // Get the output from the model, and make sure it's the expected size and
//   // type.
//   output = interpreter.output(0);
//   EXPECT_EQ(2, output->dims->size);
//   EXPECT_EQ(1, output->dims->data[0]);
//   EXPECT_EQ(kCategoryCount, output->dims->data[1]);
//   EXPECT_EQ(kTfLiteInt8, output->type);

//   // Make sure that the expected "No Person" score is higher.
//   person_score = output->data.int8[kPersonIndex];
//   no_person_score = output->data.int8[kNotAPersonIndex];
//   MicroPrintf("no person data.  person score: %d, no person score: %d\n",
//               person_score, no_person_score);
//   EXPECT_GT(no_person_score, person_score);

//   MicroPrintf("Ran successfully\n");
// }

// TF_LITE_MICRO_TESTS_MAIN


// ////////////////////////////////////////////////////////


// #include "tensorflow/lite/c/common.h"
// #include "tensorflow/lite/kernels/internal/tensor_ctypes.h"  // GetTensorData()
// #include "tensorflow/lite/micro/examples/person_detection/model_settings.h"
// #include "tensorflow/lite/micro/examples/person_detection/testdata/no_person_image_data.h"
// #include "tensorflow/lite/micro/examples/person_detection/testdata/person_image_data.h"
// #include "tensorflow/lite/micro/micro_interpreter.h"
// #include "tensorflow/lite/micro/micro_log.h"
// #include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
// #include "tensorflow/lite/micro/models/person_detect_model_data.h"
// #include "tensorflow/lite/micro/testing/micro_test.h"
// #include "tensorflow/lite/schema/schema_generated.h"
// #include "tensorflow/lite/schema/schema_utils.h"  // add this include

// #include <cstdint>
// #include <cstddef>
// #include <cstring>
// #include <cstdio>

// // -------------------------
// // Toggles
// // -------------------------
// #define AK_ENABLE_CONSOLE_SUMMARY 1   // 1 = prints per-op summary (light)
// #define AK_ENABLE_CONSOLE_PREVIEW 0   // 1 = prints first few tensor values (very noisy)
// #define AK_DUMP_ACTIVATION_INPUT  1   // dump input[0] per op into answerkey.bin

// // -------------------------
// // Helpers: type sizing, dims, bytes
// // -------------------------
// static inline const char* TfLiteTypeName(TfLiteType t) {
//   switch (t) {
//     case kTfLiteNoType:  return "NoType";
//     case kTfLiteFloat32: return "float32";
//     case kTfLiteInt32:   return "int32";
//     case kTfLiteUInt8:   return "uint8";
//     case kTfLiteInt64:   return "int64";
//     case kTfLiteString:  return "string";
//     case kTfLiteBool:    return "bool";
//     case kTfLiteInt16:   return "int16";
//     case kTfLiteComplex64:return "complex64";
//     case kTfLiteInt8:    return "int8";
//     case kTfLiteFloat16: return "float16";
//     case kTfLiteFloat64: return "float64";
//     case kTfLiteComplex128:return "complex128";
//     case kTfLiteUInt64:  return "uint64";
//     case kTfLiteResource:return "resource";
//     case kTfLiteVariant: return "variant";
//     case kTfLiteUInt32:  return "uint32";
//     case kTfLiteUInt16:  return "uint16";
//     default:             return "unknown";
//   }
// }

// static inline uint32_t TypeSizeBytes(TfLiteType type) {
//   switch (type) {
//     case kTfLiteInt8:    return 1;
//     case kTfLiteUInt8:   return 1;
//     case kTfLiteInt16:   return 2;
//     case kTfLiteUInt16:  return 2;
//     case kTfLiteInt32:   return 4;
//     case kTfLiteUInt32:  return 4;
//     case kTfLiteFloat32: return 4;
//     case kTfLiteInt64:   return 8;
//     case kTfLiteUInt64:  return 8;
//     default:             return 1;  // fallback (conservative)
//   }
// }

// static inline uint32_t NumElements(const TfLiteIntArray* dims) {
//   if (!dims) return 0;
//   uint32_t n = 1;
//   for (int i = 0; i < dims->size; ++i) n *= (uint32_t)dims->data[i];
//   return n;
// }

// static inline uint32_t EvalTensorBytes(const TfLiteEvalTensor* t) {
//   if (!t || !t->dims) return 0;
//   return NumElements(t->dims) * TypeSizeBytes(t->type);
// }

// // -------------------------
// // Operator name lookup (kernel per layer)
// // -------------------------
// static const char* BuiltinOpName(const tflite::Model* model, uint32_t op_idx) {
//   const tflite::SubGraph* sg = model->subgraphs()->Get(0);
//   const auto* ops = sg->operators();
//   const auto* op = ops->Get(op_idx);

//   const auto* opcodes = model->operator_codes();
//   // const auto* code = opcodes->Get(op->opcode_index());
//   // const int builtin = code->builtin_code();  // BuiltinOperator enum value
//   const auto* code = opcodes->Get(op->opcode_index());
//   const int builtin = tflite::GetBuiltinCode(code);

//   switch (builtin) {
//     case tflite::BuiltinOperator_CONV_2D:            return "CONV_2D";
//     case tflite::BuiltinOperator_DEPTHWISE_CONV_2D:  return "DEPTHWISE_CONV_2D";
//     case tflite::BuiltinOperator_AVERAGE_POOL_2D:    return "AVERAGE_POOL_2D";
//     case tflite::BuiltinOperator_MAX_POOL_2D:        return "MAX_POOL_2D";
//     case tflite::BuiltinOperator_RESHAPE:            return "RESHAPE";
//     case tflite::BuiltinOperator_SOFTMAX:            return "SOFTMAX";
//     case tflite::BuiltinOperator_ADD:                return "ADD";
//     case tflite::BuiltinOperator_MUL:                return "MUL";
//     case tflite::BuiltinOperator_LOGISTIC:           return "LOGISTIC";
//     case tflite::BuiltinOperator_FULLY_CONNECTED:    return "FULLY_CONNECTED";
//     default:                                         return "OTHER";
//   }
// }

// // -------------------------
// // Answer-key binary format
// // -------------------------
// // File header: 4x u32
// //   magic0 = 0xA11E0A0C
// //   version = 1
// //   run_id  (1=person, 2=noperson)
// //   num_ops
// //
// // Record: variable
// //   rec_magic = 0xA11E0A0B
// //   op_idx
// //   rec_kind  (1=act_in, 2=weights, 3=bias, 4=act_out)
// //   tensor_idx
// //   type (TfLiteType)
// //   ndims
// //   dims[ndims]  (u32 each)
// //   data_bytes
// //   data[data_bytes]
// //   pad to 4 bytes
// //
// // Footer:
// //   magic1 = 0xA11E0A0D

// enum AkKind : uint32_t {
//   AK_ACT_IN  = 1,
//   AK_WEIGHTS = 2,
//   AK_BIAS    = 3,
//   AK_ACT_OUT = 4,
// };

// // Big buffer in RAM; for host we then write it out to a file.
// constexpr uint32_t kAnswerKeyMaxBytes = 1024u * 1024u;  // 1 MB; adjust if needed
// __attribute__((aligned(16))) static uint8_t  g_answerkey[kAnswerKeyMaxBytes];
// __attribute__((aligned(4)))  static uint32_t g_answerkey_len = 0;
// __attribute__((aligned(4)))  static uint32_t g_answerkey_overflow = 0;

// static inline void AK_Reset(uint8_t fill = 0xA5) {
//   g_answerkey_len = 0;
//   g_answerkey_overflow = 0;
//   memset(g_answerkey, fill, sizeof(g_answerkey));
// }

// static inline void AK_Append(const void* src, uint32_t nbytes) {
//   if (g_answerkey_len + nbytes > kAnswerKeyMaxBytes) {
//     g_answerkey_overflow = 1;
//     return;
//   }
//   memcpy(&g_answerkey[g_answerkey_len], src, nbytes);
//   g_answerkey_len += nbytes;
// }

// static inline void AK_U32(uint32_t v) { AK_Append(&v, sizeof(v)); }

// static inline void AK_Pad4() {
//   uint32_t r = g_answerkey_len & 3u;
//   if (r) {
//     uint32_t zero = 0;
//     AK_Append(&zero, 4u - r);
//   }
// }

// static inline void AK_WriteTensorRecord(uint32_t op_idx, AkKind kind, int tensor_idx,
//                                         const TfLiteEvalTensor* t) {
//   if (!t || tensor_idx < 0) return;

//   AK_U32(0xA11E0A0Bu);           // record magic
//   AK_U32(op_idx);
//   AK_U32((uint32_t)kind);
//   AK_U32((uint32_t)tensor_idx);
//   AK_U32((uint32_t)t->type);

//   uint32_t nd = t->dims ? (uint32_t)t->dims->size : 0;
//   AK_U32(nd);
//   for (uint32_t i = 0; i < nd; ++i) AK_U32((uint32_t)t->dims->data[i]);

//   uint32_t nbytes = EvalTensorBytes(t);
//   AK_U32(nbytes);

//   const void* data_ptr = t->data.raw;
//   if (data_ptr && nbytes) AK_Append(data_ptr, nbytes);

//   AK_Pad4();
// }

// static void AK_WriteFile(const char* path) {
//   FILE* f = std::fopen(path, "wb");
//   if (!f) { MicroPrintf("AK: failed to open %s", path); return; }
//   std::fwrite(g_answerkey, 1, g_answerkey_len, f);
//   std::fclose(f);
//   MicroPrintf("AK: wrote %d bytes -> %s (overflow=%d)",
//               (int)g_answerkey_len, path, (int)g_answerkey_overflow);
// }

// static void Layers_WriteTextHeader(FILE* f, const tflite::Model* model, uint32_t run_id) {
//   if (!f) return;
//   std::fprintf(f, "TFLM Person Detection Layer Report\n");
//   std::fprintf(f, "run_id=%u (1=person,2=noperson)\n", run_id);
//   std::fprintf(f, "schema_version=%d\n", model->version());
//   std::fprintf(f, "------------------------------------------------------------\n");
// }

// static void PrintDimsToFile(FILE* f, const TfLiteIntArray* dims) {
//   if (!f) return;
//   if (!dims) { std::fprintf(f, "[]"); return; }
//   std::fprintf(f, "[");
//   for (int i = 0; i < dims->size; ++i) {
//     std::fprintf(f, "%d%s", dims->data[i], (i == dims->size - 1) ? "" : ",");
//   }
//   std::fprintf(f, "]");
// }

// static void DescribeTensorToFile(FILE* f,
//                                  const char* label,
//                                  int tensor_idx,
//                                  const TfLiteEvalTensor* t) {
//   if (!f) return;
//   if (tensor_idx < 0 || !t) {
//     std::fprintf(f, "  %s: (none)\n", label);
//     return;
//   }
//   uint32_t bytes = EvalTensorBytes(t);
//   std::fprintf(f, "  %s: t%d type=%s ", label, tensor_idx, TfLiteTypeName(t->type));
//   PrintDimsToFile(f, t->dims);
//   std::fprintf(f, " bytes=%u\n", bytes);
// }

// // Optional console preview (very noisy)
// #if AK_ENABLE_CONSOLE_PREVIEW
// static void ConsolePreviewSome(const TfLiteEvalTensor* t, int max_elems = 16) {
//   if (!t) { MicroPrintf("  (null)"); return; }
//   const uint32_t bytes = EvalTensorBytes(t);
//   if (bytes == 0 || !t->data.raw) { MicroPrintf("  (empty)"); return; }

//   if (t->type == kTfLiteInt8) {
//     const int8_t* d = tflite::micro::GetTensorData<int8_t>(t);
//     int n = (int)bytes;
//     if (n < max_elems) max_elems = n;
//     MicroPrintf("  int8[0..%d):", max_elems);
//     for (int i = 0; i < max_elems; ++i) MicroPrintf(" %d", (int)d[i]);
//   }
// }
// #endif

// static void DumpAndRecordPerOp(const tflite::Model* model,
//                                tflite::MicroInterpreter* interp,
//                                FILE* layers_txt) {
//   const tflite::SubGraph* sg = model->subgraphs()->Get(0);
//   const auto* ops = sg->operators();

// #if AK_ENABLE_CONSOLE_SUMMARY
//   MicroPrintf("Total ops: %d", (int)ops->size());
// #endif

//   for (uint32_t op_idx = 0; op_idx < (uint32_t)ops->size(); ++op_idx) {
//     const auto* op = ops->Get(op_idx);
//     const char* opname = BuiltinOpName(model, op_idx);

//     // Identify common roles:
//     // input[0] = activation input
//     // input[1] = weights (often)
//     // input[2] = bias or params (often)
//     int in0 = -1, w = -1, b = -1, out0 = -1;
//     if (op->inputs() && op->inputs()->size() > 0) in0 = op->inputs()->Get(0);
//     if (op->inputs() && op->inputs()->size() > 1) w   = op->inputs()->Get(1);
//     if (op->inputs() && op->inputs()->size() > 2) b   = op->inputs()->Get(2);
//     if (op->outputs() && op->outputs()->size() > 0) out0 = op->outputs()->Get(0);

//     const TfLiteEvalTensor* tin0  = (in0  >= 0) ? interp->GetTensor(in0)  : nullptr;
//     const TfLiteEvalTensor* tw    = (w    >= 0) ? interp->GetTensor(w)    : nullptr;
//     const TfLiteEvalTensor* tb    = (b    >= 0) ? interp->GetTensor(b)    : nullptr;
//     const TfLiteEvalTensor* tout0 = (out0 >= 0) ? interp->GetTensor(out0) : nullptr;

//     // Write to layers.txt
//     if (layers_txt) {
//       std::fprintf(layers_txt, "OP %u: %s\n", op_idx, opname);
//       DescribeTensorToFile(layers_txt, "in0 (activation)", in0, tin0);
//       DescribeTensorToFile(layers_txt, "w   (kernel)",     w,   tw);
//       DescribeTensorToFile(layers_txt, "b   (bias/params)",b,   tb);
//       DescribeTensorToFile(layers_txt, "out0 (activation)",out0,tout0);
//       std::fprintf(layers_txt, "\n");
//     }

// #if AK_ENABLE_CONSOLE_SUMMARY
//     MicroPrintf("OP %d: %s", (int)op_idx, opname);
// #endif

//     // Record into answerkey buffer
//     if (AK_DUMP_ACTIVATION_INPUT && tin0)  AK_WriteTensorRecord(op_idx, AK_ACT_IN,  in0,  tin0);
//     if (tw)                                AK_WriteTensorRecord(op_idx, AK_WEIGHTS, w,    tw);
//     if (tb)                                AK_WriteTensorRecord(op_idx, AK_BIAS,    b,    tb);
//     if (tout0)                             AK_WriteTensorRecord(op_idx, AK_ACT_OUT, out0, tout0);

// #if AK_ENABLE_CONSOLE_PREVIEW
//     if (tout0) ConsolePreviewSome(tout0, 16);
// #endif
//   }
// }

// // -------------------------
// // Tensor arena
// // -------------------------
// #if defined(XTENSA) && defined(VISION_P6)
// constexpr int tensor_arena_size = 352 * 1024;
// #else
// constexpr int tensor_arena_size = 352 * 1024;
// #endif
// static uint8_t tensor_arena[tensor_arena_size];

// // -------------------------
// // Test
// // -------------------------
// TF_LITE_MICRO_TESTS_BEGIN

// TF_LITE_MICRO_TEST(TestInvoke) {
//   const tflite::Model* model = ::tflite::GetModel(g_person_detect_model_data);
//   if (model->version() != TFLITE_SCHEMA_VERSION) {
//     MicroPrintf("Model schema %d != supported %d",
//                 model->version(), TFLITE_SCHEMA_VERSION);
//     // Still continue; tests may fail later if incompatible.
//   }

//   // Ops used by person detection model (int8)
//   tflite::MicroMutableOpResolver<6> micro_op_resolver;
//   micro_op_resolver.AddAveragePool2D(tflite::Register_AVERAGE_POOL_2D_INT8());
//   micro_op_resolver.AddConv2D(tflite::Register_CONV_2D_INT8());
//   micro_op_resolver.AddDepthwiseConv2D(tflite::Register_DEPTHWISE_CONV_2D_INT8());
//   micro_op_resolver.AddReshape();
//   micro_op_resolver.AddSoftmax(tflite::Register_SOFTMAX_INT8());
//   micro_op_resolver.AddAdd();

//   // preserve_all_tensors=true so dumping AFTER Invoke() is meaningful.
//   tflite::MicroInterpreter interpreter(
//       model, micro_op_resolver, tensor_arena, tensor_arena_size,
//       /*resource_variables=*/nullptr,
//       /*profiler=*/nullptr,
//       /*preserve_all_tensors=*/true);

//   TfLiteStatus alloc_status = interpreter.AllocateTensors();
//   TF_LITE_MICRO_EXPECT_EQ(kTfLiteOk, alloc_status);

//   MicroPrintf("arena_used_bytes=%d", (int)interpreter.arena_used_bytes());

//   TfLiteTensor* input = interpreter.input(0);
//   TF_LITE_MICRO_EXPECT(input != nullptr);
//   TF_LITE_MICRO_EXPECT_EQ(4, input->dims->size);
//   TF_LITE_MICRO_EXPECT_EQ(1, input->dims->data[0]);
//   TF_LITE_MICRO_EXPECT_EQ(kNumRows, input->dims->data[1]);
//   TF_LITE_MICRO_EXPECT_EQ(kNumCols, input->dims->data[2]);
//   TF_LITE_MICRO_EXPECT_EQ(kNumChannels, input->dims->data[3]);
//   TF_LITE_MICRO_EXPECT_EQ(kTfLiteInt8, input->type);

//   // -------------------------
//   // Run 1: Person image
//   // -------------------------
//   memcpy(input->data.int8, g_person_image_data, input->bytes);

//   AK_Reset();
//   // Answerkey file header
//   const uint32_t num_ops = (uint32_t)model->subgraphs()->Get(0)->operators()->size();
//   AK_U32(0xA11E0A0Cu);
//   AK_U32(1);        // version
//   AK_U32(1);        // run_id=1 (person)
//   AK_U32(num_ops);

//   TfLiteStatus invoke_status = interpreter.Invoke();
//   TF_LITE_MICRO_EXPECT_EQ(kTfLiteOk, invoke_status);

//   FILE* f_layers_person = std::fopen("layers_person.txt", "w");
//   Layers_WriteTextHeader(f_layers_person, model, 1);

//   DumpAndRecordPerOp(model, &interpreter, f_layers_person);

//   if (f_layers_person) std::fclose(f_layers_person);

//   // Footer
//   AK_U32(0xA11E0A0Du);

//   // Check final output scores
//   TfLiteTensor* output = interpreter.output(0);
//   int8_t person_score = output->data.int8[kPersonIndex];
//   int8_t no_person_score = output->data.int8[kNotAPersonIndex];
//   MicroPrintf("With Person => person=%d no_person=%d", person_score, no_person_score);
//   TF_LITE_MICRO_EXPECT_GT(person_score, no_person_score);

//   // Write answerkey bin
//   AK_WriteFile("answerkey_person.bin");

//   // -------------------------
//   // Run 2: No-person image
//   // -------------------------
//   memcpy(input->data.int8, g_no_person_image_data, input->bytes);

//   AK_Reset();
//   AK_U32(0xA11E0A0Cu);
//   AK_U32(1);        // version
//   AK_U32(2);        // run_id=2 (no-person)
//   AK_U32(num_ops);

//   invoke_status = interpreter.Invoke();
//   TF_LITE_MICRO_EXPECT_EQ(kTfLiteOk, invoke_status);

//   FILE* f_layers_noperson = std::fopen("layers_noperson.txt", "w");
//   Layers_WriteTextHeader(f_layers_noperson, model, 2);

//   DumpAndRecordPerOp(model, &interpreter, f_layers_noperson);

//   if (f_layers_noperson) std::fclose(f_layers_noperson);

//   AK_U32(0xA11E0A0Du);

//   output = interpreter.output(0);
//   person_score = output->data.int8[kPersonIndex];
//   no_person_score = output->data.int8[kNotAPersonIndex];
//   MicroPrintf("No Person => person=%d no_person=%d", person_score, no_person_score);
//   TF_LITE_MICRO_EXPECT_GT(no_person_score, person_score);

//   AK_WriteFile("answerkey_noperson.bin");

//   MicroPrintf("Done.");
// }

// TF_LITE_MICRO_TESTS_END

///////////////////////////////////////////////////////////////


// SPIKE CODE

// #include "tensorflow/lite/c/common.h"
// #include "tensorflow/lite/micro/examples/person_detection/model_settings.h"
// #include "tensorflow/lite/micro/examples/person_detection/testdata/no_person_image_data.h"
// #include "tensorflow/lite/micro/examples/person_detection/testdata/person_image_data.h"
// #include "tensorflow/lite/micro/micro_interpreter.h"
// #include "tensorflow/lite/micro/micro_log.h"
// #include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
// #include "tensorflow/lite/micro/models/person_detect_model_data.h"
// #include "tensorflow/lite/micro/testing/micro_test_v2.h"
// #include "tensorflow/lite/schema/schema_generated.h"

// // Create an area of memory to use for input, output, and intermediate arrays.
// #if defined(XTENSA) && defined(VISION_P6)
// constexpr int tensor_arena_size = 352 * 1024;
// #else
// constexpr int tensor_arena_size = 136 * 1024;
// #endif  // defined(XTENSA) && defined(VISION_P6)
// uint8_t tensor_arena[tensor_arena_size];

// TEST(PersonDetectionTest, TestInvoke) {
//   // Map the model into a usable data structure. This doesn't involve any
//   // copying or parsing, it's a very lightweight operation.
//   const tflite::Model* model = ::tflite::GetModel(g_person_detect_model_data);
//   if (model->version() != TFLITE_SCHEMA_VERSION) {
//     MicroPrintf(
//         "Model provided is schema version %d not equal "
//         "to supported version %d.\n",
//         model->version(), TFLITE_SCHEMA_VERSION);
//   }

//   // Pull in only the operation implementations we need.
//   // This relies on a complete list of all the ops needed by this graph.
//   // An easier approach is to just use the AllOpsResolver, but this will
//   // incur some penalty in code space for op implementations that are not
//   // needed by this graph.
//   tflite::MicroMutableOpResolver<5> micro_op_resolver;
//   micro_op_resolver.AddAveragePool2D(tflite::Register_AVERAGE_POOL_2D_INT8());
//   micro_op_resolver.AddConv2D(tflite::Register_CONV_2D_INT8());
//   micro_op_resolver.AddDepthwiseConv2D(
//       tflite::Register_DEPTHWISE_CONV_2D_INT8());
//   micro_op_resolver.AddReshape();
//   micro_op_resolver.AddSoftmax(tflite::Register_SOFTMAX_INT8());

//   // Build an interpreter to run the model with.
//   tflite::MicroInterpreter interpreter(model, micro_op_resolver, tensor_arena,
//                                        tensor_arena_size);
//   interpreter.AllocateTensors();

//   // Get information about the memory area to use for the model's input.
//   TfLiteTensor* input = interpreter.input(0);

//   // Make sure the input has the properties we expect.
//   EXPECT_NE(input, nullptr);
//   EXPECT_EQ(4, input->dims->size);
//   EXPECT_EQ(1, input->dims->data[0]);
//   EXPECT_EQ(kNumRows, input->dims->data[1]);
//   EXPECT_EQ(kNumCols, input->dims->data[2]);
//   EXPECT_EQ(kNumChannels, input->dims->data[3]);
//   EXPECT_EQ(kTfLiteInt8, input->type);

//   // Copy an image with a person into the memory area used for the input.
//   TFLITE_DCHECK_EQ(input->bytes, static_cast<size_t>(g_person_image_data_size));
//   memcpy(input->data.int8, g_person_image_data, input->bytes);

//   // Run the model on this input and make sure it succeeds.
//   TfLiteStatus invoke_status = interpreter.Invoke();
//   if (invoke_status != kTfLiteOk) {
//     MicroPrintf("Invoke failed\n");
//   }
//   EXPECT_EQ(kTfLiteOk, invoke_status);

//   // Get the output from the model, and make sure it's the expected size and
//   // type.
//   TfLiteTensor* output = interpreter.output(0);
//   EXPECT_EQ(2, output->dims->size);
//   EXPECT_EQ(1, output->dims->data[0]);
//   EXPECT_EQ(kCategoryCount, output->dims->data[1]);
//   EXPECT_EQ(kTfLiteInt8, output->type);

//   // Make sure that the expected "Person" score is higher than the other class.
//   int8_t person_score = output->data.int8[kPersonIndex];
//   int8_t no_person_score = output->data.int8[kNotAPersonIndex];
//   MicroPrintf("person data.  person score: %d, no person score: %d\n",
//               person_score, no_person_score);
//   EXPECT_GT(person_score, no_person_score);

//   memcpy(input->data.int8, g_no_person_image_data, input->bytes);

//   // Run the model on this "No Person" input.
//   invoke_status = interpreter.Invoke();
//   if (invoke_status != kTfLiteOk) {
//     MicroPrintf("Invoke failed\n");
//   }
//   EXPECT_EQ(kTfLiteOk, invoke_status);

//   // Get the output from the model, and make sure it's the expected size and
//   // type.
//   output = interpreter.output(0);
//   EXPECT_EQ(2, output->dims->size);
//   EXPECT_EQ(1, output->dims->data[0]);
//   EXPECT_EQ(kCategoryCount, output->dims->data[1]);
//   EXPECT_EQ(kTfLiteInt8, output->type);

//   // Make sure that the expected "No Person" score is higher.
//   person_score = output->data.int8[kPersonIndex];
//   no_person_score = output->data.int8[kNotAPersonIndex];
//   MicroPrintf("no person data.  person score: %d, no person score: %d\n",
//               person_score, no_person_score);
//   EXPECT_GT(no_person_score, person_score);

//   MicroPrintf("Ran successfully\n");
// }

// TF_LITE_MICRO_TESTS_MAIN


////////////////////////////////////////////////////////


// #include "tensorflow/lite/c/common.h"
// #include "tensorflow/lite/kernels/internal/tensor_ctypes.h"  // GetTensorData()
// #include "tensorflow/lite/micro/examples/person_detection/model_settings.h"
// #include "tensorflow/lite/micro/examples/person_detection/testdata/no_person_image_data.h"
// #include "tensorflow/lite/micro/examples/person_detection/testdata/person_image_data.h"
// #include "tensorflow/lite/micro/micro_interpreter.h"
// #include "tensorflow/lite/micro/micro_log.h"
// #include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
// #include "tensorflow/lite/micro/models/person_detect_model_data.h"
// #include "tensorflow/lite/micro/testing/micro_test.h"
// #include "tensorflow/lite/schema/schema_generated.h"
// #include "tensorflow/lite/schema/schema_utils.h"  // add this include

// #include <cstdint>
// #include <cstddef>
// #include <cstring>
// #include <cstdio>

// // -------------------------
// // Toggles
// // -------------------------
// #define AK_ENABLE_CONSOLE_SUMMARY 0   // 1 = prints per-op summary (light)
// #define AK_ENABLE_CONSOLE_PREVIEW 0   // 1 = prints first few tensor values (very noisy)
// #define AK_DUMP_ACTIVATION_INPUT  1   // dump input[0] per op into answerkey.bin

// // -------------------------
// // Helpers: type sizing, dims, bytes
// // -------------------------
// static inline const char* TfLiteTypeName(TfLiteType t) {
//   switch (t) {
//     case kTfLiteNoType:  return "NoType";
//     case kTfLiteFloat32: return "float32";
//     case kTfLiteInt32:   return "int32";
//     case kTfLiteUInt8:   return "uint8";
//     case kTfLiteInt64:   return "int64";
//     case kTfLiteString:  return "string";
//     case kTfLiteBool:    return "bool";
//     case kTfLiteInt16:   return "int16";
//     case kTfLiteComplex64:return "complex64";
//     case kTfLiteInt8:    return "int8";
//     case kTfLiteFloat16: return "float16";
//     case kTfLiteFloat64: return "float64";
//     case kTfLiteComplex128:return "complex128";
//     case kTfLiteUInt64:  return "uint64";
//     case kTfLiteResource:return "resource";
//     case kTfLiteVariant: return "variant";
//     case kTfLiteUInt32:  return "uint32";
//     case kTfLiteUInt16:  return "uint16";
//     default:             return "unknown";
//   }
// }

// static inline uint32_t TypeSizeBytes(TfLiteType type) {
//   switch (type) {
//     case kTfLiteInt8:    return 1;
//     case kTfLiteUInt8:   return 1;
//     case kTfLiteInt16:   return 2;
//     case kTfLiteUInt16:  return 2;
//     case kTfLiteInt32:   return 4;
//     case kTfLiteUInt32:  return 4;
//     case kTfLiteFloat32: return 4;
//     case kTfLiteInt64:   return 8;
//     case kTfLiteUInt64:  return 8;
//     default:             return 1;  // fallback (conservative)
//   }
// }

// static inline uint32_t NumElements(const TfLiteIntArray* dims) {
//   if (!dims) return 0;
//   uint32_t n = 1;
//   for (int i = 0; i < dims->size; ++i) n *= (uint32_t)dims->data[i];
//   return n;
// }

// static inline uint32_t EvalTensorBytes(const TfLiteEvalTensor* t) {
//   if (!t || !t->dims) return 0;
//   return NumElements(t->dims) * TypeSizeBytes(t->type);
// }

// // -------------------------
// // Operator name lookup (kernel per layer)
// // -------------------------
// static const char* BuiltinOpName(const tflite::Model* model, uint32_t op_idx) {
//   const tflite::SubGraph* sg = model->subgraphs()->Get(0);
//   const auto* ops = sg->operators();
//   const auto* op = ops->Get(op_idx);

//   const auto* opcodes = model->operator_codes();
//   // const auto* code = opcodes->Get(op->opcode_index());
//   // const int builtin = code->builtin_code();  // BuiltinOperator enum value
//   const auto* code = opcodes->Get(op->opcode_index());
//   const int builtin = tflite::GetBuiltinCode(code);

//   switch (builtin) {
//     case tflite::BuiltinOperator_CONV_2D:            return "CONV_2D";
//     case tflite::BuiltinOperator_DEPTHWISE_CONV_2D:  return "DEPTHWISE_CONV_2D";
//     case tflite::BuiltinOperator_AVERAGE_POOL_2D:    return "AVERAGE_POOL_2D";
//     case tflite::BuiltinOperator_MAX_POOL_2D:        return "MAX_POOL_2D";
//     case tflite::BuiltinOperator_RESHAPE:            return "RESHAPE";
//     case tflite::BuiltinOperator_SOFTMAX:            return "SOFTMAX";
//     case tflite::BuiltinOperator_ADD:                return "ADD";
//     case tflite::BuiltinOperator_MUL:                return "MUL";
//     case tflite::BuiltinOperator_LOGISTIC:           return "LOGISTIC";
//     case tflite::BuiltinOperator_FULLY_CONNECTED:    return "FULLY_CONNECTED";
//     default:                                         return "OTHER";
//   }
// }

// // -------------------------
// // Answer-key binary format
// // -------------------------
// // File header: 4x u32
// //   magic0 = 0xA11E0A0C
// //   version = 1
// //   run_id  (1=person, 2=noperson)
// //   num_ops
// //
// // Record: variable
// //   rec_magic = 0xA11E0A0B
// //   op_idx
// //   rec_kind  (1=act_in, 2=weights, 3=bias, 4=act_out)
// //   tensor_idx
// //   type (TfLiteType)
// //   ndims
// //   dims[ndims]  (u32 each)
// //   data_bytes
// //   data[data_bytes]
// //   pad to 4 bytes
// //
// // Footer:
// //   magic1 = 0xA11E0A0D

// enum AkKind : uint32_t {
//   AK_ACT_IN  = 1,
//   AK_WEIGHTS = 2,
//   AK_BIAS    = 3,
//   AK_ACT_OUT = 4,
// };

// // Big buffer in RAM; for host we then write it out to a file.
// constexpr uint32_t kAnswerKeyMaxBytes = 1024u * 1024u;  // 1 MB; adjust if needed
// __attribute__((aligned(16))) static uint8_t  g_answerkey[kAnswerKeyMaxBytes];
// __attribute__((aligned(4)))  static uint32_t g_answerkey_len = 0;
// __attribute__((aligned(4)))  static uint32_t g_answerkey_overflow = 0;

// static inline void AK_Reset(uint8_t fill = 0xA5) {
//   g_answerkey_len = 0;
//   g_answerkey_overflow = 0;
//   memset(g_answerkey, fill, sizeof(g_answerkey));
// }

// static inline void AK_Append(const void* src, uint32_t nbytes) {
//   if (g_answerkey_len + nbytes > kAnswerKeyMaxBytes) {
//     g_answerkey_overflow = 1;
//     return;
//   }
//   memcpy(&g_answerkey[g_answerkey_len], src, nbytes);
//   g_answerkey_len += nbytes;
// }

// static inline void AK_U32(uint32_t v) { AK_Append(&v, sizeof(v)); }

// static inline void AK_Pad4() {
//   uint32_t r = g_answerkey_len & 3u;
//   if (r) {
//     uint32_t zero = 0;
//     AK_Append(&zero, 4u - r);
//   }
// }

// static inline void AK_WriteTensorRecord(uint32_t op_idx, AkKind kind, int tensor_idx,
//                                         const TfLiteEvalTensor* t) {
//   if (!t || tensor_idx < 0) return;

//   AK_U32(0xA11E0A0Bu);           // record magic
//   AK_U32(op_idx);
//   AK_U32((uint32_t)kind);
//   AK_U32((uint32_t)tensor_idx);
//   AK_U32((uint32_t)t->type);

//   uint32_t nd = t->dims ? (uint32_t)t->dims->size : 0;
//   AK_U32(nd);
//   for (uint32_t i = 0; i < nd; ++i) AK_U32((uint32_t)t->dims->data[i]);

//   uint32_t nbytes = EvalTensorBytes(t);
//   AK_U32(nbytes);

//   const void* data_ptr = t->data.raw;
//   if (data_ptr && nbytes) AK_Append(data_ptr, nbytes);

//   AK_Pad4();
// }

// // static void AK_WriteFile(const char* path) {
// //   FILE* f = std::fopen(path, "wb");
// //   if (!f) { MicroPrintf("AK: failed to open %s", path); return; }
// //   std::fwrite(g_answerkey, 1, g_answerkey_len, f);
// //   std::fclose(f);
// //   MicroPrintf("AK: wrote %d bytes -> %s (overflow=%d)",
// //               (int)g_answerkey_len, path, (int)g_answerkey_overflow);
// // }

// static void AK_WriteStdoutBinary(uint32_t run_id) {
//   // Print a short ASCII header so you can find the binary block in logs
//   // (keep it very short to avoid clutter)
//   std::printf("AK_BEGIN run_id=%u len=%u\n", run_id, (unsigned)g_answerkey_len);

//   // Write raw bytes
//   std::fwrite(g_answerkey, 1, g_answerkey_len, stdout);
//   std::fflush(stdout);

//   std::printf("\nAK_END run_id=%u\n", run_id);
// }

// static void Layers_WriteTextHeader(FILE* f, const tflite::Model* model, uint32_t run_id) {
//   if (!f) return;
//   std::fprintf(f, "TFLM Person Detection Layer Report\n");
//   std::fprintf(f, "run_id=%u (1=person,2=noperson)\n", run_id);
//   std::fprintf(f, "schema_version=%d\n", model->version());
//   std::fprintf(f, "------------------------------------------------------------\n");
// }

// static void PrintDimsToFile(FILE* f, const TfLiteIntArray* dims) {
//   if (!f) return;
//   if (!dims) { std::fprintf(f, "[]"); return; }
//   std::fprintf(f, "[");
//   for (int i = 0; i < dims->size; ++i) {
//     std::fprintf(f, "%d%s", dims->data[i], (i == dims->size - 1) ? "" : ",");
//   }
//   std::fprintf(f, "]");
// }

// static void DescribeTensorToFile(FILE* f,
//                                  const char* label,
//                                  int tensor_idx,
//                                  const TfLiteEvalTensor* t) {
//   if (!f) return;
//   if (tensor_idx < 0 || !t) {
//     std::fprintf(f, "  %s: (none)\n", label);
//     return;
//   }
//   uint32_t bytes = EvalTensorBytes(t);
//   std::fprintf(f, "  %s: t%d type=%s ", label, tensor_idx, TfLiteTypeName(t->type));
//   PrintDimsToFile(f, t->dims);
//   std::fprintf(f, " bytes=%u\n", bytes);
// }

// // Optional console preview (very noisy)
// #if AK_ENABLE_CONSOLE_PREVIEW
// static void ConsolePreviewSome(const TfLiteEvalTensor* t, int max_elems = 16) {
//   if (!t) { MicroPrintf("  (null)"); return; }
//   const uint32_t bytes = EvalTensorBytes(t);
//   if (bytes == 0 || !t->data.raw) { MicroPrintf("  (empty)"); return; }

//   if (t->type == kTfLiteInt8) {
//     const int8_t* d = tflite::micro::GetTensorData<int8_t>(t);
//     int n = (int)bytes;
//     if (n < max_elems) max_elems = n;
//     MicroPrintf("  int8[0..%d):", max_elems);
//     for (int i = 0; i < max_elems; ++i) MicroPrintf(" %d", (int)d[i]);
//   }
// }
// #endif

// static void DumpAndRecordPerOp(const tflite::Model* model,
//                                tflite::MicroInterpreter* interp,
//                                FILE* layers_txt) {
//   const tflite::SubGraph* sg = model->subgraphs()->Get(0);
//   const auto* ops = sg->operators();

// #if AK_ENABLE_CONSOLE_SUMMARY
//   MicroPrintf("Total ops: %d", (int)ops->size());
// #endif

//   for (uint32_t op_idx = 0; op_idx < (uint32_t)ops->size(); ++op_idx) {
//     const auto* op = ops->Get(op_idx);
//     const char* opname = BuiltinOpName(model, op_idx);

//     // Identify common roles:
//     // input[0] = activation input
//     // input[1] = weights (often)
//     // input[2] = bias or params (often)
//     int in0 = -1, w = -1, b = -1, out0 = -1;
//     if (op->inputs() && op->inputs()->size() > 0) in0 = op->inputs()->Get(0);
//     if (op->inputs() && op->inputs()->size() > 1) w   = op->inputs()->Get(1);
//     if (op->inputs() && op->inputs()->size() > 2) b   = op->inputs()->Get(2);
//     if (op->outputs() && op->outputs()->size() > 0) out0 = op->outputs()->Get(0);

//     const TfLiteEvalTensor* tin0  = (in0  >= 0) ? interp->GetTensor(in0)  : nullptr;
//     const TfLiteEvalTensor* tw    = (w    >= 0) ? interp->GetTensor(w)    : nullptr;
//     const TfLiteEvalTensor* tb    = (b    >= 0) ? interp->GetTensor(b)    : nullptr;
//     const TfLiteEvalTensor* tout0 = (out0 >= 0) ? interp->GetTensor(out0) : nullptr;

//     // Write to layers.txt
//     if (layers_txt) {
//       std::fprintf(layers_txt, "OP %u: %s\n", op_idx, opname);
//       DescribeTensorToFile(layers_txt, "in0 (activation)", in0, tin0);
//       DescribeTensorToFile(layers_txt, "w   (kernel)",     w,   tw);
//       DescribeTensorToFile(layers_txt, "b   (bias/params)",b,   tb);
//       DescribeTensorToFile(layers_txt, "out0 (activation)",out0,tout0);
//       std::fprintf(layers_txt, "\n");
//     }

// #if AK_ENABLE_CONSOLE_SUMMARY
//     MicroPrintf("OP %d: %s", (int)op_idx, opname);
// #endif

//     // Record into answerkey buffer
//     if (AK_DUMP_ACTIVATION_INPUT && tin0)  AK_WriteTensorRecord(op_idx, AK_ACT_IN,  in0,  tin0);
//     if (tw)                                AK_WriteTensorRecord(op_idx, AK_WEIGHTS, w,    tw);
//     if (tb)                                AK_WriteTensorRecord(op_idx, AK_BIAS,    b,    tb);
//     if (tout0)                             AK_WriteTensorRecord(op_idx, AK_ACT_OUT, out0, tout0);

// #if AK_ENABLE_CONSOLE_PREVIEW
//     if (tout0) ConsolePreviewSome(tout0, 16);
// #endif
//   }
// }

// // -------------------------
// // Tensor arena
// // -------------------------
// #if defined(XTENSA) && defined(VISION_P6)
// constexpr int tensor_arena_size = 352 * 1024;
// #else
// constexpr int tensor_arena_size = 352 * 1024;
// #endif
// static uint8_t tensor_arena[tensor_arena_size];

// // -------------------------
// // Test
// // -------------------------
// TF_LITE_MICRO_TESTS_BEGIN

// TF_LITE_MICRO_TEST(TestInvoke) {
//   const tflite::Model* model = ::tflite::GetModel(g_person_detect_model_data);
//   if (model->version() != TFLITE_SCHEMA_VERSION) {
//     MicroPrintf("Model schema %d != supported %d",
//                 model->version(), TFLITE_SCHEMA_VERSION);
//     // Still continue; tests may fail later if incompatible.
//   }

//   // Ops used by person detection model (int8)
//   tflite::MicroMutableOpResolver<6> micro_op_resolver;
//   micro_op_resolver.AddAveragePool2D(tflite::Register_AVERAGE_POOL_2D_INT8());
//   micro_op_resolver.AddConv2D(tflite::Register_CONV_2D_INT8());
//   micro_op_resolver.AddDepthwiseConv2D(tflite::Register_DEPTHWISE_CONV_2D_INT8());
//   micro_op_resolver.AddReshape();
//   micro_op_resolver.AddSoftmax(tflite::Register_SOFTMAX_INT8());
//   micro_op_resolver.AddAdd();

//   // preserve_all_tensors=true so dumping AFTER Invoke() is meaningful.
//   tflite::MicroInterpreter interpreter(
//       model, micro_op_resolver, tensor_arena, tensor_arena_size,
//       /*resource_variables=*/nullptr,
//       /*profiler=*/nullptr,
//       /*preserve_all_tensors=*/true);

//   TfLiteStatus alloc_status = interpreter.AllocateTensors();
//   TF_LITE_MICRO_EXPECT_EQ(kTfLiteOk, alloc_status);

//   MicroPrintf("arena_used_bytes=%d", (int)interpreter.arena_used_bytes());

//   TfLiteTensor* input = interpreter.input(0);
//   TF_LITE_MICRO_EXPECT(input != nullptr);
//   TF_LITE_MICRO_EXPECT_EQ(4, input->dims->size);
//   TF_LITE_MICRO_EXPECT_EQ(1, input->dims->data[0]);
//   TF_LITE_MICRO_EXPECT_EQ(kNumRows, input->dims->data[1]);
//   TF_LITE_MICRO_EXPECT_EQ(kNumCols, input->dims->data[2]);
//   TF_LITE_MICRO_EXPECT_EQ(kNumChannels, input->dims->data[3]);
//   TF_LITE_MICRO_EXPECT_EQ(kTfLiteInt8, input->type);

//   // -------------------------
//   // Run 1: Person image
//   // -------------------------
//   memcpy(input->data.int8, g_person_image_data, input->bytes);

//   AK_Reset();
//   // Answerkey file header
//   const uint32_t num_ops = (uint32_t)model->subgraphs()->Get(0)->operators()->size();
//   AK_U32(0xA11E0A0Cu);
//   AK_U32(1);        // version
//   AK_U32(1);        // run_id=1 (person)
//   AK_U32(num_ops);

//   TfLiteStatus invoke_status = interpreter.Invoke();
//   TF_LITE_MICRO_EXPECT_EQ(kTfLiteOk, invoke_status);

//   FILE* f_layers_person = std::fopen("layers_person.txt", "w");
//   Layers_WriteTextHeader(f_layers_person, model, 1);

//   DumpAndRecordPerOp(model, &interpreter, f_layers_person);

//   if (f_layers_person) std::fclose(f_layers_person);

//   // Footer
//   AK_U32(0xA11E0A0Du);

//   // Check final output scores
//   TfLiteTensor* output = interpreter.output(0);
//   int8_t person_score = output->data.int8[kPersonIndex];
//   int8_t no_person_score = output->data.int8[kNotAPersonIndex];
//   MicroPrintf("With Person => person=%d no_person=%d", person_score, no_person_score);
//   TF_LITE_MICRO_EXPECT_GT(person_score, no_person_score);

//   // Write answerkey bin
//   // AK_WriteFile("answerkey_person.bin");
//   AK_WriteStdoutBinary(1);

//   // -------------------------
//   // Run 2: No-person image
//   // -------------------------
//   memcpy(input->data.int8, g_no_person_image_data, input->bytes);

//   AK_Reset();
//   AK_U32(0xA11E0A0Cu);
//   AK_U32(1);        // version
//   AK_U32(2);        // run_id=2 (no-person)
//   AK_U32(num_ops);

//   invoke_status = interpreter.Invoke();
//   TF_LITE_MICRO_EXPECT_EQ(kTfLiteOk, invoke_status);

//   FILE* f_layers_noperson = std::fopen("layers_noperson.txt", "w");
//   Layers_WriteTextHeader(f_layers_noperson, model, 2);

//   DumpAndRecordPerOp(model, &interpreter, f_layers_noperson);

//   if (f_layers_noperson) std::fclose(f_layers_noperson);

//   AK_U32(0xA11E0A0Du);

//   output = interpreter.output(0);
//   person_score = output->data.int8[kPersonIndex];
//   no_person_score = output->data.int8[kNotAPersonIndex];
//   MicroPrintf("No Person => person=%d no_person=%d", person_score, no_person_score);
//   TF_LITE_MICRO_EXPECT_GT(no_person_score, person_score);

//   // AK_WriteFile("answerkey_noperson.bin");
//   AK_WriteStdoutBinary(2);

//   MicroPrintf("Done.");
// }

// TF_LITE_MICRO_TESTS_END















/////////////////////////////////////////////


// person_detection_test_with_ak_switch.cc
// Copy-paste replacement for your existing test file.
// Adds AK_ENABLE_BINARY compile-time switch to avoid allocating 1MB on RTL.

// Required TFLM headers (same as before)
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/kernels/internal/tensor_ctypes.h"  // GetTensorData()
#include "tensorflow/lite/micro/examples/person_detection/model_settings.h"
#include "tensorflow/lite/micro/examples/person_detection/testdata/no_person_image_data.h"
#include "tensorflow/lite/micro/examples/person_detection/testdata/person_image_data.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/models/person_detect_model_data.h"
#include "tensorflow/lite/micro/testing/micro_test.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/schema/schema_utils.h"  // add this include

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>

// Define RTL_BUILD when compiling for Vivado simulation
#ifndef RTL_BUILD
#define RTL_BUILD 0
#endif


// =====================
// RTL mailbox (DMEM)
// =====================
#if defined(RTL_BUILD)
static inline void mmio_write32(uint32_t addr, uint32_t v) {
  *(volatile uint32_t*)addr = v;
}

static void rtl_write_sig_done(uint32_t sig0, uint32_t sig1) {
  // 0x0,0x4,0x8 are DMEM words (SIG0,SIG1,DONE)
  mmio_write32(0x00000000u, sig0);
  mmio_write32(0x00000004u, sig1);
  mmio_write32(0x00000008u, 1u);     // DONE
  while (1) { }                      // HALT so CPU doesn't run garbage
}
#endif

// -------------------------
// User toggle: set at compile time if desired
// -------------------------
// Default: enable full binary answerkey (host/Spike).
// For RTL builds, pass -DAK_ENABLE_BINARY=0 to the compiler to disable the 1MB buffer.
#ifndef AK_ENABLE_BINARY
#define AK_ENABLE_BINARY 1
#endif

// -------------------------
// Toggles (original)
// -------------------------
#define AK_ENABLE_CONSOLE_SUMMARY 0   // 1 = prints per-op summary (light)
#define AK_ENABLE_CONSOLE_PREVIEW 0   // 1 = prints first few tensor values (very noisy)
#define AK_DUMP_ACTIVATION_INPUT  1   // dump input[0] per op into answerkey.bin

// -------------------------
// Helpers: type sizing, dims, bytes
// -------------------------
static inline const char* TfLiteTypeName(TfLiteType t) {
  switch (t) {
    case kTfLiteNoType:  return "NoType";
    case kTfLiteFloat32: return "float32";
    case kTfLiteInt32:   return "int32";
    case kTfLiteUInt8:   return "uint8";
    case kTfLiteInt64:   return "int64";
    case kTfLiteString:  return "string";
    case kTfLiteBool:    return "bool";
    case kTfLiteInt16:   return "int16";
    case kTfLiteComplex64:return "complex64";
    case kTfLiteInt8:    return "int8";
    case kTfLiteFloat16: return "float16";
    case kTfLiteFloat64: return "float64";
    case kTfLiteComplex128:return "complex128";
    case kTfLiteUInt64:  return "uint64";
    case kTfLiteResource:return "resource";
    case kTfLiteVariant: return "variant";
    case kTfLiteUInt32:  return "uint32";
    case kTfLiteUInt16:  return "uint16";
    default:             return "unknown";
  }
}

static inline uint32_t TypeSizeBytes(TfLiteType type) {
  switch (type) {
    case kTfLiteInt8:    return 1;
    case kTfLiteUInt8:   return 1;
    case kTfLiteInt16:   return 2;
    case kTfLiteUInt16:  return 2;
    case kTfLiteInt32:   return 4;
    case kTfLiteUInt32:  return 4;
    case kTfLiteFloat32: return 4;
    case kTfLiteInt64:   return 8;
    case kTfLiteUInt64:  return 8;
    default:             return 1;  // fallback (conservative)
  }
}

static inline uint32_t NumElements(const TfLiteIntArray* dims) {
  if (!dims) return 0;
  uint32_t n = 1;
  for (int i = 0; i < dims->size; ++i) n *= (uint32_t)dims->data[i];
  return n;
}

static inline uint32_t EvalTensorBytes(const TfLiteEvalTensor* t) {
  if (!t || !t->dims) return 0;
  return NumElements(t->dims) * TypeSizeBytes(t->type);
}

// -------------------------
// Operator name lookup (kernel per layer)
// -------------------------
static const char* BuiltinOpName(const tflite::Model* model, uint32_t op_idx) {
  const tflite::SubGraph* sg = model->subgraphs()->Get(0);
  const auto* ops = sg->operators();
  const auto* op = ops->Get(op_idx);

  const auto* opcodes = model->operator_codes();
  const auto* code = opcodes->Get(op->opcode_index());
  const int builtin = tflite::GetBuiltinCode(code);

  switch (builtin) {
    case tflite::BuiltinOperator_CONV_2D:            return "CONV_2D";
    case tflite::BuiltinOperator_DEPTHWISE_CONV_2D:  return "DEPTHWISE_CONV_2D";
    case tflite::BuiltinOperator_AVERAGE_POOL_2D:    return "AVERAGE_POOL_2D";
    case tflite::BuiltinOperator_MAX_POOL_2D:        return "MAX_POOL_2D";
    case tflite::BuiltinOperator_RESHAPE:            return "RESHAPE";
    case tflite::BuiltinOperator_SOFTMAX:            return "SOFTMAX";
    case tflite::BuiltinOperator_ADD:                return "ADD";
    case tflite::BuiltinOperator_MUL:                return "MUL";
    case tflite::BuiltinOperator_LOGISTIC:           return "LOGISTIC";
    case tflite::BuiltinOperator_FULLY_CONNECTED:    return "FULLY_CONNECTED";
    default:                                         return "OTHER";
  }
}

// -------------------------
// Answer-key binary format (unchanged)
// -------------------------
enum AkKind : uint32_t {
  AK_ACT_IN  = 1,
  AK_WEIGHTS = 2,
  AK_BIAS    = 3,
  AK_ACT_OUT = 4,
};

#if AK_ENABLE_BINARY
// Full-size answerkey buffer (host builds)
constexpr uint32_t kAnswerKeyMaxBytes = 1024u * 1024u;  // 1 MB
__attribute__((aligned(16))) static uint8_t  g_answerkey[kAnswerKeyMaxBytes];
__attribute__((aligned(4)))  static uint32_t g_answerkey_len = 0;
__attribute__((aligned(4)))  static uint32_t g_answerkey_overflow = 0;
#else
// RTL-friendly small answerkey buffer: hold a tiny signature (person_score, no_person_score)
constexpr uint32_t kAnswerKeyMaxBytes = 16u;  // tiny buffer
__attribute__((aligned(4)))  static uint8_t  g_answerkey[kAnswerKeyMaxBytes];
__attribute__((aligned(4)))  static uint32_t g_answerkey_len = 0;
__attribute__((aligned(4)))  static uint32_t g_answerkey_overflow = 0;
// We'll store the final person/no_person scores (1 byte each) into g_answerkey[0..1]
// so the testbench or extractor can still read them.
#endif

static inline void AK_Reset(uint8_t fill = 0xA5) {
  g_answerkey_len = 0;
  g_answerkey_overflow = 0;
#if AK_ENABLE_BINARY
  memset(g_answerkey, fill, sizeof(g_answerkey));
#else
  (void)fill;
  // zero small buffer for clarity
  if (kAnswerKeyMaxBytes > 0) memset(g_answerkey, 0, kAnswerKeyMaxBytes);
#endif
}

static inline void AK_Append(const void* src, uint32_t nbytes) {
  if (g_answerkey_len + nbytes > kAnswerKeyMaxBytes) {
    g_answerkey_overflow = 1;
    return;
  }
  if (src && nbytes) {
    memcpy(&g_answerkey[g_answerkey_len], src, nbytes);
    g_answerkey_len += nbytes;
  }
}

static inline void AK_U32(uint32_t v) { AK_Append(&v, sizeof(v)); }

static inline void AK_Pad4() {
  uint32_t r = g_answerkey_len & 3u;
  if (r) {
    uint32_t zero = 0;
    AK_Append(&zero, 4u - r);
  }
}

static inline void AK_WriteTensorRecord(uint32_t op_idx, AkKind kind, int tensor_idx,
                                        const TfLiteEvalTensor* t) {
  if (!t || tensor_idx < 0) return;

  AK_U32(0xA11E0A0Bu);           // record magic
  AK_U32(op_idx);
  AK_U32((uint32_t)kind);
  AK_U32((uint32_t)tensor_idx);
  AK_U32((uint32_t)t->type);

  uint32_t nd = t->dims ? (uint32_t)t->dims->size : 0;
  AK_U32(nd);
  for (uint32_t i = 0; i < nd; ++i) AK_U32((uint32_t)t->dims->data[i]);

  uint32_t nbytes = EvalTensorBytes(t);
  AK_U32(nbytes);

  const void* data_ptr = t->data.raw;
  if (data_ptr && nbytes) AK_Append(data_ptr, nbytes);

  AK_Pad4();
}

// Print/write the buffer to stdout (same format as before). Works for both full and small buffer.
static void AK_WriteStdoutBinary(uint32_t run_id) {
  // Print a short ASCII header so you can find the binary block in logs
  // (keep it very short to avoid clutter)
  std::printf("AK_BEGIN run_id=%u len=%u\n", (unsigned)run_id, (unsigned)g_answerkey_len);

  // Write raw bytes
  std::fwrite(g_answerkey, 1, g_answerkey_len, stdout);
  std::fflush(stdout);

  std::printf("\nAK_END run_id=%u\n", (unsigned)run_id);
}

static void Layers_WriteTextHeader(FILE* f, const tflite::Model* model, uint32_t run_id) {
  if (!f) return;
  std::fprintf(f, "TFLM Person Detection Layer Report\n");
  std::fprintf(f, "run_id=%u (1=person,2=noperson)\n", (unsigned)run_id);
  std::fprintf(f, "schema_version=%d\n", model->version());
  std::fprintf(f, "------------------------------------------------------------\n");
}

static void PrintDimsToFile(FILE* f, const TfLiteIntArray* dims) {
  if (!f) return;
  if (!dims) { std::fprintf(f, "[]"); return; }
  std::fprintf(f, "[");
  for (int i = 0; i < dims->size; ++i) {
    std::fprintf(f, "%d%s", dims->data[i], (i == dims->size - 1) ? "" : ",");
  }
  std::fprintf(f, "]");
}

static void DescribeTensorToFile(FILE* f,
                                 const char* label,
                                 int tensor_idx,
                                 const TfLiteEvalTensor* t) {
  if (!f) return;
  if (tensor_idx < 0 || !t) {
    std::fprintf(f, "  %s: (none)\n", label);
    return;
  }
  uint32_t bytes = EvalTensorBytes(t);
  std::fprintf(f, "  %s: t%d type=%s ", label, tensor_idx, TfLiteTypeName(t->type));
  PrintDimsToFile(f, t->dims);
  std::fprintf(f, " bytes=%u\n", bytes);
}

// Optional console preview (very noisy)
#if AK_ENABLE_CONSOLE_PREVIEW
static void ConsolePreviewSome(const TfLiteEvalTensor* t, int max_elems = 16) {
  if (!t) { MicroPrintf("  (null)"); return; }
  const uint32_t bytes = EvalTensorBytes(t);
  if (bytes == 0 || !t->data.raw) { MicroPrintf("  (empty)"); return; }

  if (t->type == kTfLiteInt8) {
    const int8_t* d = tflite::micro::GetTensorData<int8_t>(t);
    int n = (int)bytes;
    if (n < max_elems) max_elems = n;
    MicroPrintf("  int8[0..%d):", max_elems);
    for (int i = 0; i < max_elems; ++i) MicroPrintf(" %d", (int)d[i]);
  }
}
#endif

static void DumpAndRecordPerOp(const tflite::Model* model,
                               tflite::MicroInterpreter* interp,
                               FILE* layers_txt) {
  const tflite::SubGraph* sg = model->subgraphs()->Get(0);
  const auto* ops = sg->operators();

#if AK_ENABLE_CONSOLE_SUMMARY
  MicroPrintf("Total ops: %d", (int)ops->size());
#endif

  for (uint32_t op_idx = 0; op_idx < (uint32_t)ops->size(); ++op_idx) {
    const auto* op = ops->Get(op_idx);
    const char* opname = BuiltinOpName(model, op_idx);

    // Identify common roles:
    // input[0] = activation input
    // input[1] = weights (often)
    // input[2] = bias or params (often)
    int in0 = -1, w = -1, b = -1, out0 = -1;
    if (op->inputs() && op->inputs()->size() > 0) in0 = op->inputs()->Get(0);
    if (op->inputs() && op->inputs()->size() > 1) w   = op->inputs()->Get(1);
    if (op->inputs() && op->inputs()->size() > 2) b   = op->inputs()->Get(2);
    if (op->outputs() && op->outputs()->size() > 0) out0 = op->outputs()->Get(0);

    const TfLiteEvalTensor* tin0  = (in0  >= 0) ? interp->GetTensor(in0)  : nullptr;
    const TfLiteEvalTensor* tw    = (w    >= 0) ? interp->GetTensor(w)    : nullptr;
    const TfLiteEvalTensor* tb    = (b    >= 0) ? interp->GetTensor(b)    : nullptr;
    const TfLiteEvalTensor* tout0 = (out0 >= 0) ? interp->GetTensor(out0) : nullptr;

    // Write to layers.txt
    if (layers_txt) {
      std::fprintf(layers_txt, "OP %u: %s\n", op_idx, opname);
      DescribeTensorToFile(layers_txt, "in0 (activation)", in0, tin0);
      DescribeTensorToFile(layers_txt, "w   (kernel)",     w,   tw);
      DescribeTensorToFile(layers_txt, "b   (bias/params)",b,   tb);
      DescribeTensorToFile(layers_txt, "out0 (activation)",out0,tout0);
      std::fprintf(layers_txt, "\n");
    }

#if AK_ENABLE_CONSOLE_SUMMARY
    MicroPrintf("OP %d: %s", (int)op_idx, opname);
#endif

    // Record into answerkey buffer
    if (AK_DUMP_ACTIVATION_INPUT && tin0)  AK_WriteTensorRecord(op_idx, AK_ACT_IN,  in0,  tin0);
    if (tw)                                AK_WriteTensorRecord(op_idx, AK_WEIGHTS, w,    tw);
    if (tb)                                AK_WriteTensorRecord(op_idx, AK_BIAS,    b,    tb);
    if (tout0)                             AK_WriteTensorRecord(op_idx, AK_ACT_OUT, out0, tout0);

#if AK_ENABLE_CONSOLE_PREVIEW
    if (tout0) ConsolePreviewSome(tout0, 16);
#endif
  }
}

// -------------------------
// Tensor arena
// -------------------------
#if defined(XTENSA) && defined(VISION_P6)
constexpr int tensor_arena_size = 352 * 1024;
#else
constexpr int tensor_arena_size = 352 * 1024;
#endif
static uint8_t tensor_arena[tensor_arena_size];

// -------------------------
// Test
// -------------------------
TF_LITE_MICRO_TESTS_BEGIN

TF_LITE_MICRO_TEST(TestInvoke) {
  const tflite::Model* model = ::tflite::GetModel(g_person_detect_model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    MicroPrintf("Model schema %d != supported %d",
                model->version(), TFLITE_SCHEMA_VERSION);
    // Still continue; tests may fail later if incompatible.
  }

  // Ops used by person detection model (int8)
  tflite::MicroMutableOpResolver<6> micro_op_resolver;
  micro_op_resolver.AddAveragePool2D(tflite::Register_AVERAGE_POOL_2D_INT8());
  micro_op_resolver.AddConv2D(tflite::Register_CONV_2D_INT8());
  micro_op_resolver.AddDepthwiseConv2D(tflite::Register_DEPTHWISE_CONV_2D_INT8());
  micro_op_resolver.AddReshape();
  micro_op_resolver.AddSoftmax(tflite::Register_SOFTMAX_INT8());
  micro_op_resolver.AddAdd();

  // preserve_all_tensors=true so dumping AFTER Invoke() is meaningful.
  tflite::MicroInterpreter interpreter(
      model, micro_op_resolver, tensor_arena, tensor_arena_size,
      /*resource_variables=*/nullptr,
      /*profiler=*/nullptr,
      /*preserve_all_tensors=*/true);

  TfLiteStatus alloc_status = interpreter.AllocateTensors();
  TF_LITE_MICRO_EXPECT_EQ(kTfLiteOk, alloc_status);

  MicroPrintf("arena_used_bytes=%d", (int)interpreter.arena_used_bytes());

  TfLiteTensor* input = interpreter.input(0);
  TF_LITE_MICRO_EXPECT(input != nullptr);
  TF_LITE_MICRO_EXPECT_EQ(4, input->dims->size);
  TF_LITE_MICRO_EXPECT_EQ(1, input->dims->data[0]);
  TF_LITE_MICRO_EXPECT_EQ(kNumRows, input->dims->data[1]);
  TF_LITE_MICRO_EXPECT_EQ(kNumCols, input->dims->data[2]);
  TF_LITE_MICRO_EXPECT_EQ(kNumChannels, input->dims->data[3]);
  TF_LITE_MICRO_EXPECT_EQ(kTfLiteInt8, input->type);

  // -------------------------
  // Run 1: Person image
  // -------------------------
  memcpy(input->data.int8, g_person_image_data, input->bytes);

  AK_Reset();
  // Answerkey file header
  const uint32_t num_ops = (uint32_t)model->subgraphs()->Get(0)->operators()->size();
  AK_U32(0xA11E0A0Cu);
  AK_U32(1);        // version
  AK_U32(1);        // run_id=1 (person)
  AK_U32(num_ops);

  TfLiteStatus invoke_status = interpreter.Invoke();
  TF_LITE_MICRO_EXPECT_EQ(kTfLiteOk, invoke_status);

  FILE* f_layers_person = std::fopen("layers_person.txt", "w");
  Layers_WriteTextHeader(f_layers_person, model, 1);

  DumpAndRecordPerOp(model, &interpreter, f_layers_person);

  if (f_layers_person) std::fclose(f_layers_person);

  // Footer
  AK_U32(0xA11E0A0Du);

  // Check final output scores
  TfLiteTensor* output = interpreter.output(0);
  int8_t person_score = output->data.int8[kPersonIndex];
  int8_t no_person_score = output->data.int8[kNotAPersonIndex];

  #if defined(RTL_BUILD)
  // Pack both int8 scores into a 32-bit word for easy TB reading.
  // Lower byte = person, next byte = no_person
  uint32_t sig0 = (uint8_t)person_score | ((uint32_t)(uint8_t)no_person_score << 8);
  uint32_t sig1 = 0xA11E0001u;  // magic marker: run1 finished
  rtl_write_sig_done(sig0, sig1);
  #endif
  MicroPrintf("With Person => person=%d no_person=%d", person_score, no_person_score);
  TF_LITE_MICRO_EXPECT_GT(person_score, no_person_score);

  // If AK is disabled (RTL), create a tiny signature so extractor can still verify
#if !AK_ENABLE_BINARY
  // store scores as two signed bytes (person, no_person)
  g_answerkey_len = 0;
  int8_t ps = person_score;
  int8_t ns = no_person_score;
  AK_Append(&ps, 1);
  AK_Append(&ns, 1);
#endif

  // Write answerkey bin (or stdout block)
  // AK_WriteFile("answerkey_person.bin");
  AK_WriteStdoutBinary(1);

  // -------------------------
  // Run 2: No-person image
  // -------------------------
  memcpy(input->data.int8, g_no_person_image_data, input->bytes);

  AK_Reset();
  AK_U32(0xA11E0A0Cu);
  AK_U32(1);        // version
  AK_U32(2);        // run_id=2 (no-person)
  AK_U32(num_ops);

  invoke_status = interpreter.Invoke();
  TF_LITE_MICRO_EXPECT_EQ(kTfLiteOk, invoke_status);

  FILE* f_layers_noperson = std::fopen("layers_noperson.txt", "w");
  Layers_WriteTextHeader(f_layers_noperson, model, 2);

  DumpAndRecordPerOp(model, &interpreter, f_layers_noperson);

  if (f_layers_noperson) std::fclose(f_layers_noperson);

  AK_U32(0xA11E0A0Du);

  output = interpreter.output(0);
  person_score = output->data.int8[kPersonIndex];
  no_person_score = output->data.int8[kNotAPersonIndex];
  MicroPrintf("No Person => person=%d no_person=%d", person_score, no_person_score);
  TF_LITE_MICRO_EXPECT_GT(no_person_score, person_score);

#if !AK_ENABLE_BINARY
  // second run tiny signature
  g_answerkey_len = 0;
  ps = person_score;
  ns = no_person_score;
  AK_Append(&ps, 1);
  AK_Append(&ns, 1);
#endif

  // AK_WriteFile("answerkey_noperson.bin");
  AK_WriteStdoutBinary(2);

  MicroPrintf("Done.");


  #if defined(RTL_BUILD)
// Write DONE flag for Vivado testbench
  volatile uint32_t* DONE = (uint32_t*)0x00000008;
  *DONE = 1;

  // Stay here forever so core doesn't run garbage
  while (1) {}
#endif
}

TF_LITE_MICRO_TESTS_END