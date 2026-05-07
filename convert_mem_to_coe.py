import os
import sys

# Maximum words allowed
MEMORY_SIZE = 4096  

# Ensure correct usage
if len(sys.argv) != 3:
    print("Usage: python convert_mem_to_coe.py <input.mem> <output.coe>")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]

if not os.path.exists(input_file):
    print(f"Error: The file {input_file} does not exist.")
    sys.exit(1)

# Read input file and keep only valid lines
words = []
with open(input_file, "r") as mem_file:
    for line in mem_file:
        line = line.strip()
        if line:  # Ignore empty lines
            words.append(line)
        if len(words) >= MEMORY_SIZE:  
            break  # Stop at 4096 words max

# Write only existing words (no padding)
with open(output_file, "w") as coe_file:
    coe_file.write("memory_initialization_radix=16;\n")
    coe_file.write("memory_initialization_vector=\n")

    for i, word in enumerate(words):
        coe_file.write(word)
        if i < len(words) - 1:
            coe_file.write(",\n")
        else:
            coe_file.write(";\n")  # Last entry ends with ';'

print(f"Converted {input_file} to {output_file} with {len(words)} words (no padding).")
