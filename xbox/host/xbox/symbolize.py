#!/usr/bin/env python3
"""symbolize.py [LOG]: the host's log (build/xbox/log.txt, or a log.txt
fetched from a real Xbox's E:\\SONICADV), with names added: each
"host 0xADDRESS" of the profile by the host's function, from main.map,
which the Makefile's linker writes beside the XBE it builds, and each
game code address in a fault's stack dump by the function of 1st_read or
of an image the log shows loaded, from build/pc/link's ELF files"""
import bisect
import os
import re
import subprocess
import sys

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.abspath(os.path.join(here, "..", "..", ".."))


def table(pairs):
    pairs.sort()
    return [a for a, _ in pairs], pairs


host = []
for line in open(os.path.join(here, "main.map"), encoding="latin1"):
    m = re.match(r"\s*0001:[0-9a-f]+\s+(\S+)\s+([0-9a-f]{8,16})\s", line)
    if m:
        host.append((int(m.group(2), 16), m.group(1)))
host_starts, host = table(host)


def host_name(match):
    address = int(match.group(1), 16)
    first = bisect.bisect_right(host_starts, address) - 1
    last = bisect.bisect_right(host_starts, address + 0xff) - 1
    if last < 0:
        return match.group(0)
    # the bucket's 256 bytes may hold the end of one function and another
    found = sorted({host[j][1] for j in range(max(first, 0), last + 1)})
    return "host 0x%08x (%s)" % (address, ", ".join(found))


def image_table(image):
    elf = os.path.join(root, "build", "pc", "link", image, "final.elf")
    if not os.path.exists(elf):
        return None
    out = subprocess.run(["nm", "-n", elf], capture_output=True, text=True).stdout
    pairs = []
    for line in out.split("\n"):
        parts = line.split()
        if len(parts) == 3 and parts[1] in "Tt":
            pairs.append((int(parts[0], 16), parts[2]))
    return table(pairs)


lines = open(sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "build", "xbox", "log.txt"),
             encoding="latin1").read().split("\n")
images = ["1st_read"] + [m.group(1) for line in lines for m in [re.search(r"loaded (\S+)'s code", line)] if m]
tables = {}


def game_name(word):
    best = None
    # the latest image loaded at an address wins: overlays share code areas
    for image in reversed(images):
        if image not in tables:
            tables[image] = image_table(image)
        if tables[image] is None:
            continue
        starts, pairs = tables[image]
        k = bisect.bisect_right(starts, word) - 1
        if k >= 0 and word - starts[k] < 0x4000 and pairs[k][1] != "":
            if best is None or word - starts[k] < best[2]:
                best = (image, pairs[k][1], word - starts[k])
    return best


for line in lines:
    line = re.sub(r"host 0x([0-9a-f]{8})", host_name, line)
    if re.match(r"host:\s+\[esp\+", line):
        names = []
        for word in re.findall(r"\b(1[0-9a-f]{7})\b", line):
            found = game_name(int(word, 16))
            if found:
                names.append("%s=%s:%s+0x%x" % (word, found[0], found[1], found[2]))
        if names:
            line += "   ; " + ", ".join(names)
    print(line)
