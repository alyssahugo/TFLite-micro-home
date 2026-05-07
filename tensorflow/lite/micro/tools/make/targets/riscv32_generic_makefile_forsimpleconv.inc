# Settings for RISCV 32-bit toolchain.
TARGET_ARCH := riscv32
TARGET_TOOLCHAIN_PREFIX := riscv64-unknown-elf-

RISCV_ARCH := rv32im
RISCV_ABI := ilp32
RISCV_CODE_MODEL := medany

RISCV_EXTRA_CFLAGS :=

TARGET_DEFAULT_TOOLCHAIN_ROOT := $(DOWNLOADS_DIR)/riscv_toolchain/bin/
TARGET_TOOLCHAIN_ROOT := $(TARGET_DEFAULT_TOOLCHAIN_ROOT)
$(eval $(call add_third_party_download,$(RISCV_TOOLCHAIN_URL),$(RISCV_TOOLCHAIN_MD5),riscv_toolchain,))

export PATH := $(TARGET_TOOLCHAIN_ROOT):$(PATH)

PLATFORM_FLAGS = \
  -march=$(RISCV_ARCH) \
  -mabi=$(RISCV_ABI) \
  -mcmodel=$(RISCV_CODE_MODEL) \
  -mexplicit-relocs \
  -fno-builtin-printf \
  -DTF_LITE_MCU_DEBUG_LOG \
  -DTF_LITE_USE_GLOBAL_CMATH_FUNCTIONS \
  -funsigned-char \
  -fno-delete-null-pointer-checks \
  -fomit-frame-pointer \
  -ffunction-sections \
  -fdata-sections \
  -fno-inline \
  -fno-inline-functions \
  -fno-inline-small-functions \
  -fno-tree-sra \
  -fno-tree-ccp \
  -fno-tree-dce \
  -fno-tree-ter \
  -fno-ipa-cp \
  -fno-ipa-sra

CXXFLAGS += $(PLATFORM_FLAGS) \
  -O0 \
  -g3 \
  -fpermissive \
  -fno-use-cxa-atexit \
  -fno-exceptions \
  -fno-rtti \
  -fno-threadsafe-statics \
  -fno-unwind-tables \
  -fno-asynchronous-unwind-tables \
  -DTF_LITE_USE_GLOBAL_MIN \
  -DTF_LITE_USE_GLOBAL_MAX

CCFLAGS += $(PLATFORM_FLAGS) \
  -O0 \
  -g3 \
  -fno-unwind-tables \
  -fno-asynchronous-unwind-tables

BUILD_TYPE := micro

# Use your linker script and your startup, not the default CRT startup.
LDFLAGS += -nostartfiles
LDFLAGS += -Wl,-e,_start
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -T /home/alyssahugo/tflite-micro-home/linker_run.ld
LDFLAGS += -mno-relax

EXCLUDED_TESTS := \
  $(TENSORFLOW_ROOT)tensorflow/lite/micro/memory_arena_threshold_test.cc

MICROLITE_TEST_SRCS := $(filter-out $(EXCLUDED_TESTS), $(MICROLITE_TEST_SRCS))

CCFLAGS += $(RISCV_EXTRA_CFLAGS)
CXXFLAGS += $(RISCV_EXTRA_CFLAGS)

TEST_SCRIPT := $(TENSORFLOW_ROOT)tensorflow/lite/micro/testing/test_with_qemu.sh riscv32 rv32
SIZE_SCRIPT := ${TENSORFLOW_ROOT}tensorflow/lite/micro/testing/size_riscv32_binary.sh

MICROLITE_CC_SRCS += \
  /home/alyssahugo/tflite-micro-home/startup.S

include $(MAKEFILE_DIR)/ext_libs/eyalroz_printf.inc