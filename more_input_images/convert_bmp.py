from PIL import Image
import os
import sys


def convert_to_96x96_grayscale_bmp(input_path, output_path=None):
    # Open image
    img = Image.open(input_path)

    # Convert to grayscale
    img = img.convert("L")

    # Resize to 96x96
    img = img.resize((96, 96), Image.Resampling.LANCZOS)

    # If no output path is given, create one automatically
    if output_path is None:
        base_name = os.path.splitext(input_path)[0]
        output_path = base_name + "_96x96_gray.bmp"

    # Save as BMP
    img.save(output_path, format="BMP")

    print(f"Saved: {output_path}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python convert_image.py input_image.jpg")
        print("  python convert_image.py input_image.png output_image.bmp")
        sys.exit(1)

    input_file = sys.argv[1]

    if len(sys.argv) >= 3:
        output_file = sys.argv[2]
    else:
        output_file = None

    convert_to_96x96_grayscale_bmp(input_file, output_file)
