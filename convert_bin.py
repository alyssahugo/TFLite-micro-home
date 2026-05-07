import struct

def read_u32(f):
    return struct.unpack("<I", f.read(4))[0]

with open("answerkey_person.bin", "rb") as f:
    magic = read_u32(f)
    version = read_u32(f)
    run_id = read_u32(f)
    num_ops = read_u32(f)

    print("Header:", hex(magic), version, run_id, num_ops)

    while True:
        pos = f.tell()
        chunk = f.read(4)
        if not chunk:
            break
        f.seek(pos)

        rec_magic = read_u32(f)
        if rec_magic != 0xA11E0A0B:
            print("Reached footer or unknown record")
            break

        op_idx = read_u32(f)
        kind = read_u32(f)
        tensor_idx = read_u32(f)
        ttype = read_u32(f)
        ndims = read_u32(f)

        dims = [read_u32(f) for _ in range(ndims)]
        nbytes = read_u32(f)

        data = f.read(nbytes)

        # pad to 4 bytes
        if nbytes % 4 != 0:
            f.read(4 - (nbytes % 4))

        print("OP", op_idx, "Kind", kind, "Dims", dims, "Bytes", nbytes)