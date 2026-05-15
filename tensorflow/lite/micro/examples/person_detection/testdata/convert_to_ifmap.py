# from PIL import Image
# import numpy as np

# bmp_path = "person.bmp"

# img = Image.open(bmp_path)

# img = img.convert("L")

# u8_ifmap = np.array(img, dtype=np.uint8)

# print("Shape:", u8_ifmap.shape)
# print("Original dtype:", u8_ifmap.dtype)
# print("Original uint8 first 64:")
# print(u8_ifmap.flatten()[:64])

# if u8_ifmap.shape != (96, 96):
#     raise ValueError(f"Expected 96x96 image, got {u8_ifmap.shape}")


# i8_ifmap = (u8_ifmap.astype(np.int16) - 128).astype(np.int8)

# print(i8_ifmap.flatten()[:64])

# np.savetxt("input_ifmap_int8.txt", i8_ifmap, fmt="%d")


from PIL import Image
import numpy as np

bmp_path = "person.bmp"

img = Image.open(bmp_path)
img = img.convert("L")

u8_ifmap = np.array(img, dtype=np.uint8)

print("Shape:", u8_ifmap.shape)
print("Original dtype:", u8_ifmap.dtype)
print("Original uint8 first 64:")
print(u8_ifmap.flatten()[:64])

if u8_ifmap.shape != (96, 96):
    raise ValueError(f"Expected 96x96 image, got {u8_ifmap.shape}")

# uint8 0..255 -> signed int8 -128..127
i8_ifmap = (u8_ifmap.astype(np.int16) - 128).astype(np.int8)

print("Signed int8 first 64:")
print(i8_ifmap.flatten()[:64])

print("Raw hex first 64:")
print([f"{np.uint8(v).item():02X}" for v in i8_ifmap.flatten()[:64]])

# 1. Signed decimal text
np.savetxt("input_ifmap_int8.txt", i8_ifmap, fmt="%d")

# 2. Raw byte hex text
with open("input_ifmap_int8_hex.txt", "w") as f:
    for row in i8_ifmap:
        for v in row:
            f.write(f"{np.uint8(v).item():02X} ")
        f.write("\n")

# 3. Raw binary bytes
i8_ifmap.tofile("input_ifmap_int8.bin")

# 4. NumPy file
np.save("input_ifmap_int8.npy", i8_ifmap)

print("Saved:")
print("  input_ifmap_int8.txt      signed decimal")
print("  input_ifmap_int8_hex.txt  raw int8 bytes as hex")
print("  input_ifmap_int8.bin      raw bytes")
print("  input_ifmap_int8.npy      NumPy array")