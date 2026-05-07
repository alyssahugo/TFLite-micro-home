import re

def extract(path, run_id, out_path):
    data = open(path, "rb").read()

    # Find "AK_BEGIN run_id=X len=Y\n"
    m = re.search(rb"AK_BEGIN run_id=%d len=(\d+)\n" % run_id, data)
    if not m:
        raise SystemExit(f"AK_BEGIN for run_id={run_id} not found")

    length = int(m.group(1))
    start = m.end()  # binary starts immediately after newline
    blob = data[start:start+length]

    if len(blob) != length:
        raise SystemExit(f"Not enough bytes: expected {length}, got {len(blob)}")

    open(out_path, "wb").write(blob)
    print(f"Wrote {out_path}: {length} bytes")

extract("spike_out.bin", 1, "dut_person.bin")
extract("spike_out.bin", 2, "dut_noperson.bin")