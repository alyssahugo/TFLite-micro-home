#!/usr/bin/env python3
import ast
import os
import re
import subprocess
import sys
from typing import List, Tuple

# ------------------------------------------------------------
# Configuration
# ------------------------------------------------------------
INST_DEPTH_WORDS = 4096
DATA_DEPTH_WORDS = 4096

ASSEMBLER = "assembler.py"          # existing assembler
INST_RAW_MEM = "instmem_raw.mem"
INST_MEM = "instmem.mem"
INST_COE = "instmem.coe"
DATA_MEM = "datamem.mem"
DATA_COE = "datamem.coe"

# ------------------------------------------------------------
# Helpers
# ------------------------------------------------------------
def strip_comment(line: str) -> str:
    return line.split("#", 1)[0].strip()

def parse_number(tok: str) -> int:
    tok = tok.strip()
    if not tok:
        raise ValueError("empty numeric token")
    return int(tok, 0)

def split_csv_respecting_strings(s: str) -> List[str]:
    tokens = []
    cur = []
    in_str = False
    escape = False
    quote = None

    for ch in s:
        if in_str:
            cur.append(ch)
            if escape:
                escape = False
            elif ch == "\\":
                escape = True
            elif ch == quote:
                in_str = False
        else:
            if ch in ("'", '"'):
                in_str = True
                quote = ch
                cur.append(ch)
            elif ch == ",":
                token = "".join(cur).strip()
                if token:
                    tokens.append(token)
                cur = []
            else:
                cur.append(ch)

    token = "".join(cur).strip()
    if token:
        tokens.append(token)

    return tokens

def decode_quoted_string(tok: str) -> bytes:
    tok = tok.strip()
    if not tok:
        return b""
    try:
        value = ast.literal_eval(tok)
    except Exception as e:
        raise ValueError(f"invalid string literal: {tok}") from e
    if not isinstance(value, str):
        raise ValueError(f"expected string literal, got: {tok}")
    return value.encode("latin1")

def write_mem(words: List[int], filename: str, depth_words: int) -> None:
    with open(filename, "w") as f:
        for i in range(min(len(words), depth_words)):
            f.write(f"{words[i] & 0xFFFFFFFF:08x}\n")

def write_coe(words: List[int], filename: str, depth_words: int) -> None:
    trimmed = words[:depth_words]
    with open(filename, "w") as f:
        f.write("memory_initialization_radix=16;\n")
        f.write("memory_initialization_vector=\n")
        for i, w in enumerate(trimmed):
            if i == len(trimmed) - 1:
                f.write(f"{w & 0xFFFFFFFF:08x};\n")
            else:
                f.write(f"{w & 0xFFFFFFFF:08x},\n")

def pack_bytes_to_words_le(data: bytes) -> List[int]:
    if len(data) % 4 != 0:
        data += b"\x00" * (4 - (len(data) % 4))
    words = []
    for i in range(0, len(data), 4):
        b0 = data[i + 0]
        b1 = data[i + 1]
        b2 = data[i + 2]
        b3 = data[i + 3]
        word = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24)
        words.append(word)
    return words

# ------------------------------------------------------------
# Data section parser
# ------------------------------------------------------------
def parse_data_section(asm_path: str) -> bytes:
    """
    Parses a simple .data section and returns raw bytes.
    Supported directives:
      .word  val1, val2, ...
      .byte  val1, val2, ...
      .asciz "..."
      .ascii "..."
      .space N
      .zero  N
    Labels are ignored for byte emission.
    """
    data = bytearray()
    in_data = False

    with open(asm_path, "r") as f:
        for raw_line in f:
            line = strip_comment(raw_line)
            if not line:
                continue

            # Handle labels at line start, possibly repeated
            while ":" in line:
                left, right = line.split(":", 1)
                if re.match(r"^[A-Za-z_.$][\w.$]*$", left.strip()):
                    line = right.strip()
                    if not line:
                        break
                else:
                    break
            if not line:
                continue

            if line.startswith(".data"):
                in_data = True
                continue
            if line.startswith(".text"):
                in_data = False
                continue

            if not in_data:
                continue

            if not line.startswith("."):
                # non-directive inside .data; ignore
                continue

            parts = line.split(None, 1)
            directive = parts[0]
            args = parts[1].strip() if len(parts) > 1 else ""

            if directive == ".word":
                for tok in split_csv_respecting_strings(args.replace(" ", ",")):
                    if not tok:
                        continue
                    val = parse_number(tok) & 0xFFFFFFFF
                    data.extend(val.to_bytes(4, byteorder="little", signed=False))

            elif directive == ".byte":
                for tok in split_csv_respecting_strings(args.replace(" ", ",")):
                    if not tok:
                        continue
                    val = parse_number(tok) & 0xFF
                    data.append(val)

            elif directive == ".asciz":
                s = decode_quoted_string(args)
                data.extend(s)
                data.append(0)

            elif directive == ".ascii":
                s = decode_quoted_string(args)
                data.extend(s)

            elif directive in (".space", ".zero"):
                n = parse_number(args)
                if n < 0:
                    raise ValueError(f"{directive} requires nonnegative size")
                data.extend(b"\x00" * n)

            else:
                # silently ignore unsupported directives for now
                pass

    return bytes(data)

# ------------------------------------------------------------
# Instruction conversion
# ------------------------------------------------------------
def run_external_assembler(asm_path: str) -> None:
    if not os.path.exists(ASSEMBLER):
        raise FileNotFoundError(
            f"Could not find {ASSEMBLER} in current directory.\n"
            f"Put this script in the same folder as your assembler.py."
        )

    cmd = [sys.executable, ASSEMBLER, asm_path, INST_RAW_MEM]
    print("Running:", " ".join(cmd))
    subprocess.run(cmd, check=True)

def convert_inst_raw_16_to_32(inst_raw_path: str) -> List[int]:
    with open(inst_raw_path, "r") as f:
        halfwords = [line.strip() for line in f if line.strip()]

    if not halfwords:
        return []

    # Expect 16-bit hex lines from your assembler
    bad = [h for h in halfwords if not re.fullmatch(r"[0-9a-fA-F]{4}", h)]
    if bad:
        raise ValueError(
            "instmem_raw.mem does not look like 16-bit chunks.\n"
            f"Example bad line: {bad[0]}"
        )

    if len(halfwords) % 2 != 0:
        raise ValueError("Odd number of 16-bit lines in instmem_raw.mem")

    words = []
    for i in range(0, len(halfwords), 2):
        low16 = int(halfwords[i], 16)
        high16 = int(halfwords[i + 1], 16)
        word = ((high16 & 0xFFFF) << 16) | (low16 & 0xFFFF)
        words.append(word)

    return words

# ------------------------------------------------------------
# Main
# ------------------------------------------------------------
def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: python3 asm_to_coe.py <input.S>")
        return 1

    asm_path = sys.argv[1]
    if not os.path.exists(asm_path):
        print(f"Error: assembly file not found: {asm_path}")
        return 1

    try:
        # 1) Assemble .text using your existing assembler.py
        run_external_assembler(asm_path)

        # 2) Convert assembler output to 32-bit words
        inst_words = convert_inst_raw_16_to_32(INST_RAW_MEM)
        if len(inst_words) > INST_DEPTH_WORDS:
            raise ValueError(
                f"Instruction image too large: {len(inst_words)} words > {INST_DEPTH_WORDS}"
            )

        write_mem(inst_words, INST_MEM, INST_DEPTH_WORDS)
        write_coe(inst_words, INST_COE, INST_DEPTH_WORDS)

        # 3) Parse .data and pack to little-endian 32-bit words
        data_bytes = parse_data_section(asm_path)
        data_words = pack_bytes_to_words_le(data_bytes)
        if len(data_words) > DATA_DEPTH_WORDS:
            raise ValueError(
                f"Data image too large: {len(data_words)} words > {DATA_DEPTH_WORDS}"
            )

        write_mem(data_words, DATA_MEM, DATA_DEPTH_WORDS)
        write_coe(data_words, DATA_COE, DATA_DEPTH_WORDS)

        print(f"Generated {INST_RAW_MEM}")
        print(f"Generated {INST_MEM}")
        print(f"Generated {INST_COE}")
        print(f"Generated {DATA_MEM}")
        print(f"Generated {DATA_COE}")
        print(f"Instruction words: {len(inst_words)}")
        print(f"Data bytes:         {len(data_bytes)}")
        print(f"Data words:         {len(data_words)}")
        return 0

    except subprocess.CalledProcessError as e:
        print("Assembler failed.")
        return e.returncode
    except Exception as e:
        print("Error:", e)
        return 1

if __name__ == "__main__":
    raise SystemExit(main())