import re
import os

INPUT_LOG = "spike_featuremaps_raw.txt"
OUT_TXT = "formatted_featuremaps_raw.txt"
OUT_DIR = "formatted_layers"

os.makedirs(OUT_DIR, exist_ok=True)

# TFLite BuiltinOperator enum values commonly used by person_detection
# Your log has [OP2]b00000004 etc.
OP_NAMES = {
    1:  "AveragePool2D",
    3:  "Conv2D",
    4:  "DepthwiseConv2D",
    22: "Reshape",
    25: "Softmax",
}

header_re = re.compile(r"\[(IA|TO|OB|IW|IB)\](.*)")
te_re = re.compile(r"\[TE\](.*)")
tb_byte_re = re.compile(r"x([0-9A-Fa-f]{8})")
tag_re = re.compile(r"([a-z])([0-9A-Fa-f]{8})")
op2_re = re.compile(r"\[OP2\].*?i([0-9A-Fa-f]{8}).*?b([0-9A-Fa-f]{8})")


def parse_tags(s):
    """
    Parses fields like:
    i00000000 t00000058 p000A3470 y00000009 b00002400 d00000004 z00000001 ...
    Returns dict where repeated tags like z become lists.
    """
    tags = {}
    for key, val in tag_re.findall(s):
        num = int(val, 16)
        if key in tags:
            if not isinstance(tags[key], list):
                tags[key] = [tags[key]]
            tags[key].append(num)
        else:
            tags[key] = num
    return tags


def bytes_to_hex_line(byte_list, bytes_per_line=32):
    lines = []
    for i in range(0, len(byte_list), bytes_per_line):
        lines.append(" ".join(byte_list[i:i + bytes_per_line]))
    return "\n".join(lines)


def safe_name(name):
    return name.replace("/", "_").replace(" ", "_").replace(":", "_")


with open(INPUT_LOG, "r", errors="ignore") as f:
    lines = f.readlines()

# op index -> builtin name
op_names = {}

# Tensor dump records:
# each record = {
#   kind: IA/TO/IW/IB/OB,
#   op: int,
#   tensor: int,
#   bytes_expected: int,
#   dims: list,
#   data: ["8B", "90", ...]
# }
records = []

current = None

for line in lines:
    line = line.strip()

    # Capture operator type from [OP2]
    m_op2 = op2_re.search(line)
    if m_op2:
        op_idx = int(m_op2.group(1), 16)
        builtin_code = int(m_op2.group(2), 16)
        op_names[op_idx] = OP_NAMES.get(builtin_code, f"Builtin_{builtin_code}")
        continue

    # Start of tensor dump
    m_head = header_re.search(line)
    if m_head:
        kind = m_head.group(1)
        rest = m_head.group(2)
        tags = parse_tags(rest)

        current = {
            "kind": kind,
            "op": tags.get("i", -1),
            "tensor": tags.get("t", -1),
            "type": tags.get("y", None),
            "bytes_expected": tags.get("b", None),
            "dims": tags.get("z", []),
            "data": [],
        }

        if not isinstance(current["dims"], list):
            current["dims"] = [current["dims"]]

        continue

    # Tensor byte body
    if current is not None and line.startswith("[TB]"):
        for full_hex in tb_byte_re.findall(line):
            # x0000008B -> 8B
            current["data"].append(full_hex[-2:].upper())
        continue

    # End of tensor dump
    if current is not None and line.startswith("[TE]"):
        records.append(current)
        current = None
        continue


# Group records by layer/operator index
layers = {}

for r in records:
    op = r["op"]
    if op < 0:
        continue

    if op not in layers:
        layers[op] = {
            "name": op_names.get(op, "UnknownOp"),
            "IA": None,
            "TO": None,
            "all": [],
        }

    layers[op]["all"].append(r)

    # Only store main input/output feature maps
    if r["kind"] == "IA":
        layers[op]["IA"] = r
    elif r["kind"] == "TO":
        layers[op]["TO"] = r


def format_tensor(title, r):
    if r is None:
        return f"{title}:\n<missing>\n"

    dims = "x".join(str(x) for x in r["dims"]) if r["dims"] else "unknown"
    expected = r["bytes_expected"]
    actual = len(r["data"])

    text = []
    text.append(f"{title}:")
    text.append(f"Tensor index: {r['tensor']}")
    text.append(f"Shape: {dims}")
    text.append(f"Bytes expected: {expected}")
    text.append(f"Bytes parsed: {actual}")

    if expected is not None and expected != actual:
        text.append(f"WARNING: expected {expected} bytes but parsed {actual} bytes")

    text.append("Hex bytes:")
    text.append(bytes_to_hex_line(r["data"], bytes_per_line=32))
    text.append("")
    return "\n".join(text)


# Write one combined formatted file
with open(OUT_TXT, "w") as f:
    for op in sorted(layers.keys()):
        layer = layers[op]
        name = layer["name"]

        f.write("=" * 80 + "\n")
        f.write(f"Layer {op}: {name}\n")
        f.write("=" * 80 + "\n\n")

        f.write(format_tensor("Input Feature Map", layer["IA"]))
        f.write(format_tensor("Output Feature Map", layer["TO"]))
        f.write("\n")

# Write one file per layer
for op in sorted(layers.keys()):
    layer = layers[op]
    name = safe_name(layer["name"])
    path = os.path.join(OUT_DIR, f"layer_{op:02d}_{name}.txt")

    with open(path, "w") as f:
        f.write(f"Layer {op}: {layer['name']}\n")
        f.write("=" * 80 + "\n\n")
        f.write(format_tensor("Input Feature Map", layer["IA"]))
        f.write(format_tensor("Output Feature Map", layer["TO"]))

print(f"Parsed tensor records: {len(records)}")
print(f"Parsed layers: {len(layers)}")
print(f"Wrote combined file: {OUT_TXT}")
print(f"Wrote per-layer files to: {OUT_DIR}/")
