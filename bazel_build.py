from PIL import Image
import os


IMAGES = [
    ("tensorflow/lite/micro/examples/person_detection/testdata/image_3.bmp",
     "tensorflow/lite/micro/examples/person_detection/testdata/image_3_image_data",
     "g_image_3_image_data"),

    ("tensorflow/lite/micro/examples/person_detection/testdata/image_4.bmp",
     "tensorflow/lite/micro/examples/person_detection/testdata/image_4_image_data",
     "g_image_4_image_data"),

    ("tensorflow/lite/micro/examples/person_detection/testdata/image_5.bmp",
     "tensorflow/lite/micro/examples/person_detection/testdata/image_5_image_data",
     "g_image_5_image_data"),
]


def write_image_array(input_bmp, output_base, var_name):
    img = Image.open(input_bmp)

    # Make sure it is exactly the model input format.
    img = img.convert("L")
    img = img.resize((96, 96), Image.Resampling.LANCZOS)

    pixels = list(img.getdata())

    h_path = output_base + ".h"
    cc_path = output_base + ".cc"

    header_guard = os.path.basename(output_base).upper().replace(".", "_") + "_H_"

    with open(h_path, "w") as f:
        f.write(f"""#ifndef {header_guard}
#define {header_guard}

extern const unsigned char {var_name}[];
extern const int {var_name}_size;

#endif  // {header_guard}
""")

    with open(cc_path, "w") as f:
        f.write(f'#include "tensorflow/lite/micro/examples/person_detection/testdata/{os.path.basename(output_base)}.h"\n\n')
        f.write(f"const unsigned char {var_name}[] = {{\n")

        for i in range(0, len(pixels), 12):
            chunk = pixels[i:i + 12]
            line = ", ".join(f"0x{b:02X}" for b in chunk)
            f.write(f"  {line},\n")

        f.write("};\n\n")
        f.write(f"const int {var_name}_size = {len(pixels)};\n")

    print(f"Generated: {h_path}")
    print(f"Generated: {cc_path}")
    print(f"Size: {len(pixels)} bytes")


for input_bmp, output_base, var_name in IMAGES:
    write_image_array(input_bmp, output_base, var_name)
