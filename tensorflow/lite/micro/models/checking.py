from pathlib import Path

p = Path("tensorflow/lite/micro/models/person_detect.tflite")
data = p.read_bytes()

root = int.from_bytes(data[0:4], "little")
vtable = root - int.from_bytes(data[root:root+2], "little")
vlen = int.from_bytes(data[vtable:vtable+2], "little")

if vlen < 6:
    print("Vtable too short")
else:
    field_off = int.from_bytes(data[vtable+4:vtable+6], "little")
    if field_off == 0:
        print("Version field missing")
    else:
        version = int.from_bytes(data[root+field_off:root+field_off+4], "little")
        print("Model schema version:", version)