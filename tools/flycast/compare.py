"""Compare the PC port's shots with Flycast's framebuffer dumps.

    compare.py OUT SHOTS WINDOW

OUT holds port/shot_fN.ppm, from the host under HOST_CLOCK=frames, and
flycast/fN.fb, from tools/flycast/compare.lua. For each frame N of SHOTS the
port's shot is matched against Flycast's frames N-WINDOW to N+WINDOW, since
the two count a frame or two apart through the boot. The closest one is kept.
Each match writes OUT/fN.png: Flycast, the port, then their difference, 4x
amplified. OUT/report.txt lists each match's offset, its mean absolute
difference per channel (0 to 255) and the share of pixels differing by more
than 24 in any channel.
"""
import os
import struct
import sys
import zlib


def read_ppm(path):
    data = open(path, 'rb').read()
    parts = data.split(maxsplit=4)  # P6, width, height, maxval, pixels
    w, h = int(parts[1]), int(parts[2])
    return w, h, parts[4][:w * h * 3]


def read_fb(path):
    data = open(path, 'rb').read()
    head, px = data.split(b'\n', 1)
    _, depth, words, lines, _stride, _ctrl = head.split()
    depth, words, h = int(depth), int(words), int(lines)
    out = bytearray()
    if depth == 3:  # 0888: B, G, R, x
        w = words
        out[:] = bytes(b for i in range(0, len(px), 4) for b in (px[i + 2], px[i + 1], px[i]))
    elif depth == 2:  # 888 packed: B, G, R
        w = words * 4 // 3
        out[:] = bytes(b for i in range(0, len(px), 3) for b in (px[i + 2], px[i + 1], px[i]))
    else:  # 565 or 0555
        w = words * 2
        for (v,) in struct.iter_unpack('<H', px):
            if depth == 1:
                r, g, b = v >> 11, (v >> 5) & 63, v & 31
                out += bytes((r << 3 | r >> 2, g << 2 | g >> 4, b << 3 | b >> 2))
            else:
                r, g, b = (v >> 10) & 31, (v >> 5) & 31, v & 31
                out += bytes((r << 3 | r >> 2, g << 3 | g >> 2, b << 3 | b >> 2))
    return w, h, bytes(out)


def shrink(w, h, rgb, to_w, to_h):
    """a window larger by a whole factor, as HOST_SCALE gives, to to_w x to_h"""
    if (w, h) == (to_w, to_h):
        return rgb
    fx, fy = w // to_w, h // to_h
    if fx * to_w != w or fy * to_h != h:
        sys.exit(f'compare.py: a {w}x{h} shot against a {to_w}x{to_h} frame')
    return bytes(rgb[((y * fy) * w + x * fx) * 3 + c] for y in range(to_h) for x in range(to_w) for c in range(3))


def write_png(path, w, h, rgb):
    raw = b''.join(b'\0' + rgb[y * w * 3:(y + 1) * w * 3] for y in range(h))

    def chunk(kind, body):
        return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body))

    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
                + chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))


def score(a, b):
    total = sum(abs(x - y) for x, y in zip(a, b))
    return total / len(a)


def far_share(a, b):
    far = 0
    for i in range(0, len(a), 3):
        if max(abs(a[i] - b[i]), abs(a[i + 1] - b[i + 1]), abs(a[i + 2] - b[i + 2])) > 24:
            far += 1
    return far / (len(a) // 3)


def main():
    out, shots, window = sys.argv[1], [int(s) for s in sys.argv[2].split(',') if s], int(sys.argv[3])
    lines = []
    for n in shots:
        port_path = os.path.join(out, 'port', f'shot_f{n}.ppm')
        if not os.path.exists(port_path):
            lines.append(f'f{n}: the port has no shot')
            continue
        pw, ph, port = read_ppm(port_path)
        best = None
        for k in range(n - window, n + window + 1):
            fb = os.path.join(out, 'flycast', f'f{k:05d}.fb')
            if not os.path.exists(fb):
                continue
            w, h, fly = read_fb(fb)
            scaled = shrink(pw, ph, port, w, h)
            s = score(fly, scaled)
            if best is None or s < best[0]:
                best = (s, k, w, h, fly, scaled)
        if best is None:
            lines.append(f'f{n}: Flycast has no frame within {window}')
            continue
        s, k, w, h, fly, scaled = best
        diff = bytes(min(255, 4 * abs(x - y)) for x, y in zip(fly, scaled))
        side = b''.join(fly[y * w * 3:(y + 1) * w * 3] + scaled[y * w * 3:(y + 1) * w * 3]
                        + diff[y * w * 3:(y + 1) * w * 3] for y in range(h))
        write_png(os.path.join(out, f'f{n}.png'), w * 3, h, side)
        lines.append(f'f{n}: Flycast frame {k} ({k - n:+d}): mean difference {s:.2f}, '
                     f'{100 * far_share(fly, scaled):.1f}% of pixels over 24')
    with open(os.path.join(out, 'report.txt'), 'w') as f:
        f.write('\n'.join(lines) + '\n')
    print('\n'.join(lines))


if __name__ == '__main__':
    main()
