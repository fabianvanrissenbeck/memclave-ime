"""
opdiff.py - A script that displays differences (in bytecode) between different DPU instructions.

The script needs to be invoked without parameters. Input one valid DPU instruction per line.
Terminate inputting new instructions by sending an EOF signal, usually by entering Ctrl+D.
The diff is then computed with the help of opcode.sh.
"""

import sys
import subprocess

def get_opcode_for(inst):
    out = subprocess.check_output(["./opcode.sh", inst])
    return int(str(out, "utf-8").strip(), 16)


def bitequal(ls, n_bits = 48):
    res = ""

    for i in range(n_bits):
        bits = [(n >> i & 1) == 1 for n in ls]

        if all(bits) or not any(bits):
            res = "0" + res
        else:
            res = "1" + res

    return int(res, 2)


def report_opcode_diff(s, code, diff):
    print(s, end = " \t")

    for i in range(6):
        for j in range(8):
            n_bit = (5 - i) * 8 + (7 - j)
            bit = (code >> n_bit & 1) == 1
            is_diff = (diff >> n_bit & 1) == 1

            if is_diff:
                s = "{}[31m{}{}[0m".format(chr(27), "1" if bit else "0", chr(27))
            else:
                s = "1" if bit else "0"

            print(s, end = "")

        print(" ", end = "")

    print(" \t", end = "")

    for i in range(6):
        for j in range(2):
            n_nibble = (5 - i) * 2 + (1 - j)
            nibble = code >> (n_nibble * 4) & 0xF
            is_diff = (diff >> (n_nibble * 4) & 0xFF) != 0

            if is_diff:
                s = "{}[31m{:x}{}[0m".format(chr(27), nibble, chr(27))
            else:
                s = "{:x}".format(nibble)

            print(s, end = "")

        print(" ", end = "")

    print()


if __name__ == "__main__":
    lines = [line.strip() for line in sys.stdin]
    opcodes = [get_opcode_for(line) for line in lines]
    diff = bitequal(opcodes)

    max_len = max([len(line) for line in lines])

    for line, opcode in zip(lines, opcodes):
        report_opcode_diff(line.ljust(max_len), opcode, diff)

