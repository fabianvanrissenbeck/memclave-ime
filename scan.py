### Tool to scan subkernel files for banned instructions

import sys


def is_ldmai(inst):
    if inst >> 39 != 0xe0:
        return False

    if ((inst >> 20) & 0xF) != 0:
        return False

    if (inst & 0x1FF) != 0x1:
        return False

    return True


def is_thread_ctrl(inst):
    if inst >> 41 != 0x3e:
        return False

    if (inst >> 39 & 0x3) == 0:
        return False

    if (inst >> 28 & 0x3F) != 0x32:
        return False

    return True

## rri instructions place their target register (5 bits) at the lowest 4 bits
## of the most significant byte and the highest of the next most significant byte

## rrici instructions place their target register (5 bits) at bit 5 and 6 of the most
## significant byte, the next to at bit 0 and 1 of the msb and the last bit at bit 7 of
## the next most significant byte

## rri and rrici instructions differ in bit 2 and 3 of the most significant byte
## So if the rri write register is >= 24 it is actually a rrici instruction.

def is_banned_reg(inst):
    wr = inst >> 39 & 0x1F

    if wr < 24 and (wr == 20 or wr == 21):
        return True

    if wr < 24:
        if wr == 20 or wr == 21:
            return True
        else:
            return False

    wr = (inst >> 44 & 0x3) << 3 | (inst >> 39 & 0x7)
    return wr == 20 or wr == 21


def banned(inst):
    if is_ldmai(inst):
        return True, "code loading"

    if is_thread_ctrl(inst):
        return True, "thread control"

    if is_banned_reg(inst):
        return True, "banned register"

    return False, ""


def main(sk_file):
    with open(sk_file, "rb") as f:
        bytes = f.read()

    header = bytes[0:64]
    text_size = int.from_bytes(header[40:44], "little")
    text_bytes = bytes[64:64 + text_size * 2048]

    inst = [int.from_bytes(text_bytes[i:i+8], "little") for i in range(0, text_size * 2048, 8)]

    for i, n in enumerate(inst):
        is_banned, reason = banned(n)

        if is_banned:
            print("Banned instruction at 0x{:x} (index {}). Reason: \"{}\"".format(0x80002800 + i * 8, i, reason))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: scan.py <subkernel>")
        sys.exit(1)

    main(sys.argv[1])