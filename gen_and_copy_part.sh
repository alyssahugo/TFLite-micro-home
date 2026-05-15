#!/usr/bin/env bash
set -e

cd ~/tflite-micro-home

ELF="gen/riscv32_generic_x86_64_default_gcc/bin/person_detection"
DEST_ROOT="/mnt/d/new_jeric_core/coefiles"

# Find next partXXXX folder number
mkdir -p "$DEST_ROOT"

last_num=$(
  find "$DEST_ROOT" -maxdepth 1 -type d -name "part[0-9][0-9][0-9][0-9]" \
  | sed -E 's/.*part([0-9]{4})$/\1/' \
  | sort -n \
  | tail -n 1
)

if [ -z "$last_num" ]; then
  next_num=1
else
  next_num=$((10#$last_num + 1))
fi

PART_NAME=$(printf "part%04d" "$next_num")
DEST="$DEST_ROOT/$PART_NAME"

echo "Creating folder: $DEST"
mkdir -p "$DEST"

echo "Generating objdump..."
riscv32-unknown-elf-objdump -d gen/riscv32_generic_x86_64_default_gcc/bin/person_detection > disassembly.txt

echo "Generating imem.bin..."
riscv32-unknown-elf-objcopy -O binary \
  --only-section=.text \
  gen/riscv32_generic_x86_64_default_gcc/bin/person_detection \
  imem.bin

echo "Generating ddr.bin..."
riscv32-unknown-elf-objcopy -O binary \
  --only-section=.rodata \
  --only-section=.data \
  gen/riscv32_generic_x86_64_default_gcc/bin/person_detection \
  ddr.bin

echo "Running bin_to_coe.py..."
python3 bin_to_coe.py

echo "Copying files to $DEST..."

cp disassembly.txt "$DEST/"
cp imem.bin "$DEST/"
cp ddr.bin "$DEST/"
cp "$ELF" "$DEST/"

# Copy generated COE files if bin_to_coe.py creates them
cp datamem_run.coe "$DEST/" 2>/dev/null || true
cp instmem_run.coe "$DEST/" 2>/dev/null || true

echo ""
echo "DONE."
echo "Copied to WSL path:"
echo "$DEST"
echo ""
echo "Windows path:"
echo "D:\\new_jeric_core\\coefiles\\$PART_NAME"
