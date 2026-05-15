import os
import re

INPUT_LOG = "spike_featuremaps_raw.txt"
OUT_TXT = "formatted_featuremaps_new.txt"
OUT_DIR = "formatted_layers_new"

os.makedirs(OUT_DIR, exist_ok=True)

# TFLite BuiltinOperator codes that appear in your log.
# Your [OP2] line has b00000004 for DepthwiseConv2D.
OP_NAMES = {
    1:  "AveragePool2D",
    3:  "Conv2D",
    4:  "DepthwiseConv2D",
    9:  "FullyConnected",
    22: "Reshape",
    25: "Softmax",
}

# Example:
# [OP0]i00000000
op0_re = re.compile(r"\[OP0\]i([0-9A-Fa-f]{8})")

# Example:
# [OP2]b00000004 v0001C6A8 d000188E8 f00000000
op2_re = re.compile(r"\[OP2\].*?b([0-9A-Fa-f]{8})")

# Example:
# [IA]i00000000 t00000058 p000A3470 y00000009 b00002400 d00000004 z00000001 ...
header_re = re.compile(r"^\[(IA|TO|OB|IW|IB)\](.*)$")

# Tags like i00000000, t00000058, b00002400, z00000060
tag_re = re.compile(r"([A-Za-z])([0-9A-Fa-f]{8})")

# Compact byte tokens like 8B, 90, FF, 00
byte_re = re.compile(r"\b[0-9A-Fa-f]{2}\b")


def parse_tags(s):
    tags = {}

    for key, val in tag_re.findall(s):
        num = int(val, 16)

        # z repeats for dimensions
        if key in tags:
            if not isinstance(tags[key], list):
                tags[key] = [tags[key]]
            tags[key].append(num)
        else:
            tags[key] = num

    return tags


def parse_tb_line(line):
    # Remove [TB], keep only clean 2-digit hex tokens
    line = line.replace("[TB]", " ")
    return [tok.upper() for tok in byte_re.findall(line)]


def hex_lines(byte_list, bytes_per_line=32):
    out = []
    for i in range(0, len(byte_list), bytes_per_line):
        out.append(" ".join(byte_list[i:i + bytes_per_line]))
    return "\n".join(out)


def safe_filename(s):
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", s)


def format_tensor(title, record):
    if record is None:
        return f"{title}:\n<missing>\n\n"

    dims = record.get("dims", [])
    shape = "x".join(str(x) for x in dims) if dims else "unknown"

    expected = record.get("bytes_expected")
    parsed = len(record["data"])

    text = []
    text.append(f"{title}:")
    text.append(f"Tensor index: {record.get('tensor')}")
    text.append(f"Shape: {shape}")
    text.append(f"Bytes expected: {expected}")
    text.append(f"Bytes parsed: {parsed}")

    if expected is not None and expected != parsed:
        text.append(f"WARNING: expected {expected} bytes but parsed {parsed} bytes")

    text.append("Hex bytes:")
    text.append(hex_lines(record["data"], bytes_per_line=32))
    text.append("")
    return "\n".join(text)


with open(INPUT_LOG, "r", errors="ignore") as f:
    lines = f.readlines()

records = []
op_names = {}

current_op = None
current = None

for raw in lines:
    line = raw.strip()

    # Track current operator index
    m = op0_re.search(line)
    if m:
        current_op = int(m.group(1), 16)
        continue

    # Track current operator type
    m = op2_re.search(line)
    if m and current_op is not None:
        builtin_code = int(m.group(1), 16)
        op_names[current_op] = OP_NAMES.get(
            builtin_code,
            f"Builtin_{builtin_code}"
        )
        continue

    # Start of a tensor record: IA, TO, OB, IW, IB
    m = header_re.match(line)
    if m:
        kind = m.group(1)
        rest = m.group(2)

        tags = parse_tags(rest)

        dims = tags.get("z", [])
        if dims and not isinstance(dims, list):
            dims = [dims]

        current = {
            "kind": kind,
            "op": tags.get("i", current_op),
            "tensor": tags.get("t"),
            "type": tags.get("y"),
            "bytes_expected": tags.get("b"),
            "dims": dims,
            "data": [],
        }
        continue

    # Tensor byte rows
    if current is not None and line.startswith("[TB]"):
        current["data"].extend(parse_tb_line(line))
        continue

    # End of tensor record
    if current is not None and line.startswith("[TE]"):
        records.append(current)
        current = None
        continue


# Group by op/layer
layers = {}

for r in records:
    op = r.get("op")
    if op is None:
        continue

    if op not in layers:
        layers[op] = {
            "name": op_names.get(op, "UnknownOp"),
            "IA": None,
            "TO": None,
            "OB": None,
            "all": [],
        }

    layers[op]["all"].append(r)

    # Main input feature map
    if r["kind"] == "IA":
        layers[op]["IA"] = r

    # Main output feature map after operator
    elif r["kind"] == "TO":
        layers[op]["TO"] = r

    # Optional: output buffer before op, usually not what you want
    elif r["kind"] == "OB":
        layers[op]["OB"] = r


# Write combined file
with open(OUT_TXT, "w") as f:
    for op in sorted(layers.keys()):
        layer = layers[op]
        name = layer["name"]

        f.write("=" * 100 + "\n")
        f.write(f"Layer {op}: {name}\n")
        f.write("=" * 100 + "\n\n")

        f.write(format_tensor("Input Feature Map", layer["IA"]))
        f.write(format_tensor("Output Feature Map", layer["TO"]))

        # In case TO is missing but OB exists, show OB separately
        if layer["TO"] is None and layer["OB"] is not None:
            f.write(format_tensor("Output Buffer Before Op", layer["OB"]))

        f.write("\n")


# Write one file per layer
for op in sorted(layers.keys()):
    layer = layers[op]
    name = safe_filename(layer["name"])
    path = os.path.join(OUT_DIR, f"layer_{op:02d}_{name}.txt")

    with open(path, "w") as f:
        f.write("=" * 100 + "\n")
        f.write(f"Layer {op}: {layer['name']}\n")
        f.write("=" * 100 + "\n\n")
        f.write(format_tensor("Input Feature Map", layer["IA"]))
        f.write(format_tensor("Output Feature Map", layer["TO"]))

        if layer["TO"] is None and layer["OB"] is not None:
            f.write(format_tensor("Output Buffer Before Op", layer["OB"]))


print(f"Parsed tensor records: {len(records)}")
print(f"Parsed layers: {len(layers)}")
print(f"Wrote combined file: {OUT_TXT}")
print(f"Wrote per-layer files to: {OUT_DIR}/")

# Print a small summary
for op in sorted(layers.keys()):
    layer = layers[op]
    ia = layer["IA"]
    to = layer["TO"]

    ia_count = len(ia["data"]) if ia else 0
    to_count = len(to["data"]) if to else 0

    print(
        f"Layer {op:02d}: {layer['name']:<16} "
        f"IA={ia_count} bytes, TO={to_count} bytes"
    )
