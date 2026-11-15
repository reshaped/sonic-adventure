#!/usr/bin/env python3
"""arm2c.py DRIVER.DRV OUT.inc [--idle ADDRESS] [--root ADDRESS ...] [--roots FILE]

the sound driver's ARM code as C, for the port's AICA to run natively in
place of interpreting it (xbox/third_party/flycast/core/hw/arm7/arm7.cpp's
runStatic, which includes OUT.inc).

a driver file is a 32-byte header and the image the sound library loads at
sound RAM's address 0. this follows its code from the ARM's vectors: each
instruction's branch target and, unless it leaves for good, the instruction
after it; --root adds places only an indirect jump reaches, and --roots a
file of them (the interpreter runs any the walk missed, and sndreplay lists
them: xbox/host/xbox/driver_roots.txt is what two recordings found). each instruction found
becomes a line

    ARM_OP(ADDRESS, OPCODE, HASH, NEXT)

HASH the interpreter's decode index, NEXT 1 where the next line is the next
address's and the code may run on into it. arm7.cpp gives ARM_OP its
meaning: the interpreter's own code for the instruction (arm-new.h, the
decode folded away for the constant OPCODE), between the interpreter's own
checks, so that a run of it is the interpreter's run, tick for tick. OUT.inc
starts with ARM_IMAGE(SIZE, SUM), the image's size and the FNV-1a sum of the
instructions' words in their order, which arm7.cpp checks sound RAM against
before it trusts the translation.

the output holds the driver's code, so it belongs under build/ only.
"""
import struct
import sys

HEADER = 32


def main():
    args = sys.argv[1:]
    roots = []
    idle = None
    plain = []
    while args:
        a = args.pop(0)
        if a == "--root":
            roots.append(int(args.pop(0), 0))
        elif a == "--roots":
            roots += [int(line, 0) for line in open(args.pop(0)).read().split()]
        elif a == "--idle":
            idle = int(args.pop(0), 0)
        else:
            plain.append(a)
    data = open(plain[0], "rb").read()
    (size,) = struct.unpack_from("<I", data, 8)
    image = data[HEADER:HEADER + size]
    size = len(image) & ~3
    words = struct.unpack_from("<%dI" % (size // 4), image)

    def word(at):
        return words[at >> 2]

    # the vectors, then what the walk finds
    todo = [0x00, 0x04, 0x08, 0x0C, 0x10, 0x18, 0x1C] + roots
    code = set()
    while todo:
        at = todo.pop()
        while 0 <= at < size and at % 4 == 0 and at not in code:
            op = word(at)
            cond = op >> 28
            if cond == 0xF:
                break
            code.add(at)
            always = cond == 0xE
            kind = (op >> 25) & 7
            leaves = False
            if kind == 5:
                # B, BL
                offset = op & 0xFFFFFF
                if offset & 0x800000:
                    offset -= 0x1000000
                todo.append(at + 8 + offset * 4)
                leaves = always and not (op >> 24) & 1
            elif kind in (0, 1) and (op >> 12) & 15 == 15 and not ((op >> 23) & 3 == 2 and not (op >> 20) & 1):
                # a data operation into the PC (not a compare, MRS or MSR)
                leaves = always
            elif kind in (2, 3) and (op >> 20) & 1 and (op >> 12) & 15 == 15:
                # LDR PC
                leaves = always
            elif kind == 4 and (op >> 20) & 1 and (op >> 15) & 1:
                # LDM with the PC
                leaves = always
            elif (op >> 24) & 15 == 15:
                # SWI: comes back
                pass
            if leaves:
                break
            at += 4
    # of the instructions alone: the image's data changes as the driver runs
    sum_ = 2166136261
    for at in sorted(code):
        sum_ = ((sum_ ^ word(at)) * 16777619) & 0xFFFFFFFF
    out = ["/* arm2c.py's, of %s: %d instructions */" % (plain[0].split("/")[-1], len(code)),
           "ARM_IMAGE(0x%x, 0x%08xu)" % (size, sum_)]
    for at in sorted(code):
        op = word(at)
        out.append("ARM_%s(0x%x, 0x%08xu, 0x%03x, %d)" % ("IDLE" if at == idle else "OP", at, op,
                                                         ((op >> 16) & 0xFF0) | ((op >> 4) & 0xF),
                                                         1 if at + 4 in code else 0))
    open(plain[1], "w").write("\n".join(out) + "\n")
    print("arm2c.py: %s: %d instructions of %d words" % (plain[1], len(code), size // 4))


main()
