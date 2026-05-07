# #!/usr/bin/env python3

# def byte_swap32(w: int) -> int:
#     w &= 0xFFFFFFFF
#     return (
#         ((w & 0x000000FF) << 24) |
#         ((w & 0x0000FF00) << 8)  |
#         ((w & 0x00FF0000) >> 8)  |
#         ((w & 0xFF000000) >> 24)
#     )

# with open("ddr.bin", "rb") as f:
#     data = f.read()

# words = []
# for i in range(0, len(data), 4):
#     word = data[i:i+4]
#     if len(word) < 4:
#         word = word + b'\x00' * (4 - len(word))

#     # same as pack_bytes_to_words_le()
#     val = int.from_bytes(word, byteorder="little")

#     # same as write_coe()
#     words.append(f"{byte_swap32(val):08x}")

# with open("datamem_run.coe", "w") as f:
#     f.write("memory_initialization_radix=16;\n")
#     f.write("memory_initialization_vector=\n")
#     if not words:
#         f.write("00000000;\n")
#     else:
#         for i, w in enumerate(words):
#             if i == len(words) - 1:
#                 f.write(f"{w};\n")
#             else:
#                 f.write(f"{w},\n")


#!/usr/bin/env python3

from pathlib import Path


def byte_swap32(w: int) -> int:
    w &= 0xFFFFFFFF
    return (
        ((w & 0x000000FF) << 24) |
        ((w & 0x0000FF00) << 8)  |
        ((w & 0x00FF0000) >> 8)  |
        ((w & 0xFF000000) >> 24)
    )


def bin_to_coe(input_bin: str, output_coe: str) -> None:
    in_path = Path(input_bin)
    out_path = Path(output_coe)

    if not in_path.exists():
        raise FileNotFoundError(f"Input file not found: {input_bin}")

    data = in_path.read_bytes()

    words = []
    for i in range(0, len(data), 4):
        word = data[i:i+4]
        if len(word) < 4:
            word = word + b"\x00" * (4 - len(word))

        val = int.from_bytes(word, byteorder="little")
        words.append(f"{byte_swap32(val):08x}")

    with out_path.open("w") as f:
        f.write("memory_initialization_radix=16;\n")
        f.write("memory_initialization_vector=\n")
        if not words:
            f.write("00000000;\n")
        else:
            for i, w in enumerate(words):
                if i == len(words) - 1:
                    f.write(f"{w};\n")
                else:
                    f.write(f"{w},\n")

    print(f"Wrote {output_coe} from {input_bin} ({len(words)} words)")


def main() -> None:
    jobs = [
        ("ddr.bin", "datamem_run.coe"),
        ("imem.bin", "instmem_run.coe"),
    ]

    for input_bin, output_coe in jobs:
        bin_to_coe(input_bin, output_coe)


if __name__ == "__main__":
    main()