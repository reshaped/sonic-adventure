"""Write make pc's images as the disc's files, for the port's host.

Usage: pcdisc.py LINK OUT

For each image tools/pclink.py linked under LINK but 1st_read, OUT gets
the file the disc holds the image in, at its path on the disc, as
tools/images.py's files() gives it: the image's data, from its origin to
its last stored byte, as the game's loader reads it into the image's area,
PRS-compressed by build/prskit where the disc's file is a .PRS. The
image's code stays in LINK/IMAGE/final.elf, which the host loads into the
image's code area as the file opens. OUT/images.txt names each file's
image, a line each, and the events the disc stores twice take a copy
under each name. The host loads 1st_read itself, from its ELF.
"""

import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import images  # noqa: E402

CODE_BASE = 0x10000000


def data_bytes(elf):
    """the image's stored data: its sections below CODE_BASE, from the
    origin to the last stored byte, gaps zeroed"""
    out = subprocess.run(['llvm-readelf', '-S', '-W', str(elf)], capture_output=True, text=True).stdout
    sections = []
    for line in out.splitlines():
        fields = line.replace('[ ', '[').split()
        if len(fields) > 6 and fields[0].startswith('[') and fields[2] == 'PROGBITS':
            address, offset, size = int(fields[3], 16), int(fields[4], 16), int(fields[5], 16)
            if 0 < address < CODE_BASE and size:
                sections.append((address, offset, size))
    symbols = subprocess.run(['llvm-nm', str(elf)], capture_output=True, text=True).stdout
    origin = next(int(line.split()[0], 16) for line in symbols.splitlines() if line.endswith(' image_origin'))
    raw = elf.read_bytes()
    end = max((a + s for a, _, s in sections), default=origin)
    data = bytearray(end - origin)
    for address, offset, size in sections:
        data[address - origin:address - origin + size] = raw[offset:offset + size]
    return bytes(data)


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__.split('\n\n')[1])
    link, out = Path(sys.argv[1]), Path(sys.argv[2])
    files = images.files()
    names = []
    aliases = {v: k for k, v in images.EVENT_ALIASES.items()}
    for image_dir in sorted(p for p in link.iterdir() if (p / 'final.elf').exists()):
        image = image_dir.name
        if image == '1st_read' or image not in files:
            continue
        path = files[image]
        data = data_bytes(image_dir / 'final.elf')
        target = out / path
        target.parent.mkdir(parents=True, exist_ok=True)
        if path.upper().endswith('.PRS'):
            with tempfile.NamedTemporaryFile(suffix='.bin') as raw:
                raw.write(data)
                raw.flush()
                subprocess.run(['build/prskit', 'compress', raw.name, str(target)], check=True)
        else:
            target.write_bytes(data)
        names.append((path.upper(), image))
        stem = Path(path).stem.upper()
        if stem in aliases:
            twin = str(Path(path).with_name(aliases[stem] + Path(path).suffix)).upper()
            (out / twin).write_bytes(target.read_bytes())
            names.append((twin, image))
    (out / 'images.txt').write_text(''.join(f'{path} {image}\n' for path, image in names))
    print(f'pcdisc.py: {len(names)} files under {out}')


if __name__ == '__main__':
    main()
