import re
import sys

path = sys.argv[1]

with open(path, "r", encoding="utf-8") as f:
    text = f.read()

# remove header
m = re.search(r"memory_initialization_vector\s*=\s*(.*)", text, re.S | re.I)
if not m:
    raise ValueError("Could not find memory_initialization_vector")

data = m.group(1)

# remove final semicolon
data = data.split(";")[0]

# split by commas, strip whitespace, keep nonempty tokens
tokens = [t.strip() for t in data.split(",") if t.strip()]

print("WORDS_TO_COPY =", len(tokens))