import sys
from typing import List

def to_le_64(b: bytes) -> List[int]:
    return [int.from_bytes(b[i:i + 8], "little") for i in range(0, len(b), 8)]

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python arrfy <input file> <symbol> <output file>")
        sys.exit(1)

    with open(sys.argv[1], "rb") as f:
        content = f.read()

    words = to_le_64(content)

    with open(sys.argv[3], "w") as f:
        f.write("#include <mram.h>\n")
        f.write("#include <stdint.h>\n")
        f.write(f"__mram uint64_t {sys.argv[2]}[] = {{\n")

        for word in words:
            f.write("\t0x{:016x},\n".format(word))

        f.write("};\n")
