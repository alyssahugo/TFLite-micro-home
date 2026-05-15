import os
import numpy as np
from PIL import Image
import tflite_runtime.interpreter as tflite

MODEL_PATH = "person_detect.tflite"
BMP_PATH = "input.bmp"
OUT_DIR = "feature_maps_dump"

os.makedirs(OUT_DIR, exist_ok=True)

# Create interpreter
interpreter = tflite.Interpreter(
    model_path=MODEL_PATH,
    experimental_preserve_all_tensors=True
)

# Allocate tensors
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()
tensor_details = interpreter.get_tensor_details()

print("Input details:")
for d in input_details:
    print(d)

# Load your 96x96 grayscale BMP
img = Image.open(BMP_PATH).convert("L")
u8 = np.array(img, dtype=np.uint8)

if u8.shape != (96, 96):
    raise ValueError(f"Expected 96x96 grayscale image, got {u8.shape}")

inp = input_details[0]
input_index = inp["index"]
input_shape = inp["shape"]
input_dtype = inp["dtype"]

print("Model input shape:", input_shape)
print("Model input dtype:", input_dtype)

# Prepare input data
if input_dtype == np.int8:
    input_data = (u8.astype(np.int16) - 128).astype(np.int8)
elif input_dtype == np.uint8:
    input_data = u8.astype(np.uint8)
elif input_dtype == np.float32:
    input_data = u8.astype(np.float32) / 255.0
else:
    raise TypeError(f"Unsupported input dtype: {input_dtype}")

input_data = input_data.reshape(input_shape)

# Set input
interpreter.set_tensor(input_index, input_data)

# Run inference
interpreter.invoke()

summary_lines = []

# Dump all readable tensors
for t in tensor_details:
    idx = t["index"]
    name = t["name"]
    shape = t["shape"]

    safe_name = name.replace("/", "_").replace(":", "_").replace(";", "_")
    if safe_name == "":
        safe_name = f"tensor_{idx}"

    try:
        arr = interpreter.get_tensor(idx)
    except Exception as e:
        print(f"Skipping tensor {idx}: {name} because {e}")
        continue

    base = f"{idx:03d}_{safe_name}"

    np.save(os.path.join(OUT_DIR, base + ".npy"), arr)

    np.savetxt(
        os.path.join(OUT_DIR, base + ".txt"),
        arr.flatten(),
        fmt="%d" if np.issubdtype(arr.dtype, np.integer) else "%.8f"
    )

    line = (
        f"{idx:03d} | name={name} | shape={list(arr.shape)} | "
        f"dtype={arr.dtype} | min={arr.min() if arr.size else 'NA'} | "
        f"max={arr.max() if arr.size else 'NA'}"
    )
    summary_lines.append(line)
    print(line)

with open(os.path.join(OUT_DIR, "summary.txt"), "w") as f:
    f.write("\n".join(summary_lines))

print()
print(f"Dumped tensors to: {OUT_DIR}")
print(f"Total dumped tensors: {len(summary_lines)}")