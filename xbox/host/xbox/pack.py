#!/usr/bin/env python3
"""pack.py OUT_DIR [modules] [disc] [music] [movies]: the Xbox disc's packs.

the Xbox's disc holds a few large files in place of the Dreamcast disc's two
thousand, which is what an FTP transfer to a console wants: each is a file
pack, host.c's struct filepack: a header (magic, count, the file's whole
size, which the host checks, and a build number, a sum of modules.pak's
files), count entries sorted by
name (a 48-byte name, an offset from the file's start, a size), then the
files, each on a 2048-byte boundary.

  modules.pak  all that a build changes: the x86 images' data as the disc's
               files (build/pc/disc: LOADER.BIN, SONICADV/*.PRS), images.txt
               and code_areas.txt, and each image's code as code/IMAGE, an
               ELF cut down to what the host loads: the segments (a module's
               code alone: its data comes from its .PRS) and, for 1st_read,
               the symbols. one file, so a copy can't mix two builds.
  disc.pak     the Dreamcast disc's SONICADV/ files (build/disc) the port
               reads, less the modules' images, the music and the movies,
               with the texture archives cut down to their keys
               (build/xbox/stubs, by xboxtex -stubs) where PACK_STUBS isn't
               0. the disc's root (the browser's pages and pictures,
               1ST_READ.BIN) is left out: the port reads nothing there but
               LOADER.BIN, which is modules.pak's.
  music.pak    SONICADV/*.ADX and *.AFS: the music and the voices.
  movies.pak   SONICADV/*.SFD. the host plays no movie without it.

names are as the game asks for them: capitals, forward slashes. a pack is
written only where its bytes differ from the file's, so that a transfer by
date leaves the unchanged ones.
"""
import os
import struct
import sys
import zlib

MAGIC = 0x4B504153  # "SAPK"
NAME = 48
ALIGN = 2048
CODE = 0x10000000


def cut_elf(path, code_only):
    """the ELF's loaded segments and, unless code_only, its symbols"""
    d = open(path, "rb").read()
    (phoff, shoff) = struct.unpack_from("<II", d, 28)
    (phentsize, phnum, shentsize, shnum) = struct.unpack_from("<HHHH", d, 42)
    segments = []
    for k in range(phnum):
        (kind, offset, vaddr, paddr, filesz, memsz, flags, align) = struct.unpack_from("<8I", d, phoff + k * phentsize)
        if kind == 1 and memsz and not (code_only and vaddr < CODE):
            segments.append((vaddr, paddr, d[offset:offset + filesz], memsz, flags))
    sections = []
    if not code_only:
        for k in range(shnum):
            s = struct.unpack_from("<10I", d, shoff + k * shentsize)
            if s[1] == 2:  # SHT_SYMTAB
                strings = struct.unpack_from("<10I", d, shoff + s[6] * shentsize)
                sections = [(2, d[s[4]:s[4] + s[5]], 1, 16), (3, d[strings[4]:strings[4] + strings[5]], 0, 0)]
    at = 52 + 32 * len(segments) + 40 * len(sections)
    out = bytearray(d[:52])
    body = bytearray()
    heads = bytearray()
    for (vaddr, paddr, data, memsz, flags) in segments:
        heads += struct.pack("<8I", 1, at + len(body), vaddr, paddr, len(data), memsz, flags, 4)
        body += data + bytes(-len(data) % 4)
    for (kind, data, link, entsize) in sections:
        heads += struct.pack("<10I", 0, kind, 0, 0, at + len(body), len(data), link, 0, 4, entsize)
        body += data + bytes(-len(data) % 4)
    struct.pack_into("<II", out, 28, 52, 52 + 32 * len(segments) if sections else 0)
    struct.pack_into("<HHHHH", out, 42, 32, len(segments), 40, len(sections), 0)
    return bytes(out + heads + body)


def tree(root, under=""):
    """each file below root/under: its name, as the game's, and its path"""
    for (folder, _, files) in os.walk(os.path.join(root, under)):
        for name in files:
            path = os.path.join(folder, name)
            yield os.path.relpath(path, root).replace(os.sep, "/").upper(), path


def write(out, files, build):
    """files: name to a path or to bytes"""
    names = sorted(files)
    for name in names:
        if len(name.encode()) >= NAME:
            sys.exit("pack.py: %s: a name over %d bytes" % (name, NAME - 1))
    at = 16 + (NAME + 8) * len(names)
    entries = bytearray()
    sizes = []
    for name in names:
        size = len(files[name]) if isinstance(files[name], bytes) else os.path.getsize(files[name])
        at += -at % ALIGN
        entries += name.encode().ljust(NAME, b"\0") + struct.pack("<II", at, size)
        sizes.append(size)
        at += size
    if at >= 1 << 32:
        sys.exit("pack.py: %s: over 4 GB" % out)
    with open(out + ".tmp", "wb") as pack:
        pack.write(struct.pack("<IIII", MAGIC, len(names), at, build) + entries)
        for name in names:
            pack.write(bytes(-pack.tell() % ALIGN))
            pack.write(files[name] if isinstance(files[name], bytes) else open(files[name], "rb").read())
    same = os.path.exists(out) and os.path.getsize(out) == at
    if same:
        with open(out, "rb") as a, open(out + ".tmp", "rb") as b:
            while same:
                x = a.read(1 << 20)
                same = x == b.read(1 << 20)
                if not x:
                    break
    if same:
        os.remove(out + ".tmp")
    else:
        os.replace(out + ".tmp", out)
    print("pack.py: %s: %d files, %d MB%s" % (out, len(names), (at + (1 << 19)) >> 20, ", as it was" if same else ""))


def main():
    out = sys.argv[1]
    which = sys.argv[2:] or ["modules", "disc", "music", "movies"]
    images = dict(tree("build/pc/disc"))
    images_txt = images.pop("IMAGES.TXT")
    if "modules" in which:
        files = dict(images)
        files["images.txt"] = images_txt
        files["code_areas.txt"] = "build/pc/link/code_areas.txt"
        for image in sorted(os.listdir("build/pc/link")):
            elf = os.path.join("build/pc/link", image, "final.elf")
            if os.path.exists(elf):
                files["code/" + image] = cut_elf(elf, image != "1st_read")
        # the build's number: a sum of its files, so that the same build
        # again is the same pack
        build = 0
        for name in sorted(files):
            data = files[name] if isinstance(files[name], bytes) else open(files[name], "rb").read()
            build = zlib.crc32(data, zlib.crc32(name.encode(), build))
        write(os.path.join(out, "modules.pak"), files, build)
    disc = {name: path for (name, path) in tree("build/disc", "SONICADV") if name not in images}
    if "disc" in which:
        stubs = dict(tree("build/xbox/stubs")) if os.environ.get("PACK_STUBS", "1") != "0" else {}
        files = {name: stubs.get(name, path) for (name, path) in disc.items()
                 if not name.endswith((".ADX", ".AFS", ".SFD"))}
        write(os.path.join(out, "disc.pak"), files, 0)
    if "music" in which:
        write(os.path.join(out, "music.pak"), {n: p for (n, p) in disc.items() if n.endswith((".ADX", ".AFS"))}, 0)
    if "movies" in which:
        write(os.path.join(out, "movies.pak"), {n: p for (n, p) in disc.items() if n.endswith(".SFD")}, 0)


main()
