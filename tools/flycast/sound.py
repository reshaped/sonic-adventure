"""Compare the PC port's sound with Flycast's (tools/flycast/sound.sh).

    sound.py DIR

DIR holds port.wav, the port's HOST_WAV, and flycast.raw, Flycast's sound
through SDL2's disk driver: both 44.1 kHz 16-bit stereo. This writes
flycast.wav beside them, for listening, and report.txt.

The two don't start together: Flycast plays its BIOS's boot first, its loads
take their own time, and it pads its sound with silence when its emulation
falls behind. So each window of WINDOW seconds of the port's sound that
isn't silent is placed in Flycast's by itself:

- coarsely, by the loudness envelope at 100 Hz (10 ms steps): the best
  normalized correlation near the last window's offset, where it reaches
  NEAR_GOOD, since music that repeats can match better a bar away;
  otherwise the best over every offset;
- then to the sample, by the waveform's correlation within 10 ms of there.

For each window the report gives where it lies in each recording, the offset
between them, and three measures:

- wave: the waveforms' normalized correlation, stereo, at the best offset.
  The port's AICA is Flycast's code, so the same input gives the same
  samples, and wave is 1. Effects started a frame apart, a different mix, or
  a stream decoded from another point lower it;
- env: the loudness envelopes' correlation, which ignores phase: the same
  sounds at the same moments, if not the same samples;
- level: the port's loudness against Flycast's, in decibels.

A drift of the offset across windows in one stretch, with no gap in either,
is a difference of speed: the report's last lines give it in parts per
million, a least-squares slope over the windows whose wave reaches
SPEED_WAVE.
"""

import array
import math
import sys
from pathlib import Path

RATE = 44100
HOP = 441  # the envelope's step, 10 ms
WINDOW = 2.0  # seconds
SILENT = 60.0  # a window quieter than this RMS is left out
REFINE = 441  # the fine search, either side, in samples
NEAR = 300  # the coarse search near the last offset, either side, in hops
NEAR_GOOD = 0.5  # the envelope correlation that keeps a window near the last
SPEED_WAVE = 0.95  # the wave a window needs to count for the speed


def read_wav(path):
    data = path.read_bytes()
    if data[:4] == b"RIFF":
        # skip to the data chunk
        at = 12
        while at + 8 <= len(data):
            kind, size = data[at:at + 4], int.from_bytes(data[at + 4:at + 8], "little")
            if kind == b"data":
                data = data[at + 8:at + 8 + size]
                break
            at += 8 + size
    samples = array.array("h")
    samples.frombytes(data[: len(data) // 4 * 4])
    if sys.byteorder != "little":
        samples.byteswap()
    return samples


def write_wav(path, samples):
    if sys.byteorder != "little":
        samples = array.array("h", samples)
        samples.byteswap()
    body = samples.tobytes()
    head = b"RIFF" + (36 + len(body)).to_bytes(4, "little") + b"WAVEfmt " + (16).to_bytes(4, "little")
    head += (1).to_bytes(2, "little") + (2).to_bytes(2, "little") + RATE.to_bytes(4, "little")
    head += (RATE * 4).to_bytes(4, "little") + (4).to_bytes(2, "little") + (16).to_bytes(2, "little")
    head += b"data" + len(body).to_bytes(4, "little")
    path.write_bytes(head + body)


def mono(stereo):
    left, right = stereo[0::2], stereo[1::2]
    return array.array("i", (a + b for a, b in zip(left, right)))


def envelope(m):
    """the RMS of each HOP samples"""
    out = []
    for k in range(0, len(m) - HOP + 1, HOP):
        chunk = m[k:k + HOP]
        out.append(math.sqrt(sum(x * x for x in chunk) / HOP) / 2)
    return out


def ncc(a, b):
    """normalized correlation of two equal-length sequences"""
    n = len(a)
    if n == 0:
        return 0.0
    ma, mb = sum(a) / n, sum(b) / n
    sa = sb = sab = 0.0
    for x, y in zip(a, b):
        x -= ma
        y -= mb
        sa += x * x
        sb += y * y
        sab += x * y
    return sab / math.sqrt(sa * sb) if sa > 0 and sb > 0 else 0.0


def zero_ncc(a, b):
    """correlation about zero, for waveforms"""
    sa = sum(x * x for x in a)
    sb = sum(y * y for y in b)
    return sum(x * y for x, y in zip(a, b)) / math.sqrt(sa * sb) if sa > 0 and sb > 0 else 0.0


def coarse(ep, ef, start, length, prefer):
    """the hop offset of ep[start:start+length] in ef: the best, or the best
    near prefer unless the best is clearly better"""
    piece = ep[start:start + length]
    best, best_at = -2.0, None
    near, near_at = -2.0, None
    for at in range(0, len(ef) - length + 1):
        c = ncc(piece, ef[at:at + length])
        if c > best:
            best, best_at = c, at
        if prefer is not None and abs(at - start - prefer) <= NEAR and c > near:
            near, near_at = c, at
    if near_at is not None and (near >= NEAR_GOOD or near >= best - 0.1):
        return near_at - start, near
    return (best_at - start, best) if best_at is not None else (None, 0.0)


def refine(mp, mf, at_p, at_f):
    """the sample offset near at_f - at_p where the waveforms best agree, over
    a quarter second from at_p, first at a quarter of the rate"""
    span = RATE // 4
    piece = mp[at_p:at_p + span]
    best, best_lag = -2.0, 0
    for lag in range(-REFINE, REFINE + 1, 4):
        f = at_f + lag
        if f < 0 or f + span > len(mf):
            continue
        c = zero_ncc(piece[::4], mf[f:f + span:4])
        if c > best:
            best, best_lag = c, lag
    centre = best_lag
    for lag in range(centre - 4, centre + 5):
        f = at_f + lag
        if f < 0 or f + span > len(mf):
            continue
        c = zero_ncc(piece, mf[f:f + span])
        if c > best:
            best, best_lag = c, lag
    return at_f - at_p + best_lag


def main():
    out = Path(sys.argv[1])
    port = read_wav(out / "port.wav")
    fly = read_wav(out / "flycast.raw")
    write_wav(out / "flycast.wav", fly)
    mp, mf = mono(port), mono(fly)
    ep, ef = envelope(mp), envelope(mf)
    hops = int(WINDOW * RATE / HOP)
    lines = [
        f"port {len(mp) / RATE:.2f} s, flycast {len(mf) / RATE:.2f} s; windows of {WINDOW:g} s",
        "  port s  flycast s   offset s    wave    env  level dB",
    ]
    prefer = None
    rows = []
    for start in range(0, len(ep) - hops + 1, hops):
        loud = math.sqrt(sum(x * x for x in ep[start:start + hops]) / hops)
        if loud < SILENT:
            lines.append(f"{start * HOP / RATE:8.2f}  (silent in the port)")
            continue
        lag, env = coarse(ep, ef, start, hops, prefer)
        if lag is None:
            lines.append(f"{start * HOP / RATE:8.2f}  (past Flycast's end)")
            continue
        prefer = lag
        at_p = start * HOP
        offset = refine(mp, mf, at_p, at_p + lag * HOP)
        at_f = at_p + offset
        n = int(WINDOW * RATE) * 2
        a = port[2 * at_p:2 * at_p + n]
        b = fly[2 * at_f:2 * at_f + n] if at_f >= 0 else array.array("h")
        if len(b) < len(a):
            a = a[: len(b)]
        wave = zero_ncc(a, b)
        sa = math.sqrt(sum(x * x for x in a) / max(len(a), 1))
        sb = math.sqrt(sum(x * x for x in b) / max(len(b), 1))
        level = 20 * math.log10(sa / sb) if sa > 0 and sb > 0 else float("nan")
        rows.append((at_p / RATE, offset, wave))
        lines.append(
            f"{at_p / RATE:8.2f} {at_f / RATE:10.2f} {offset / RATE:10.4f} {wave:7.3f} {env:6.3f} {level:9.2f}"
        )
    # the speed, from consecutive windows with good wave and offsets within
    # 50 ms of each other
    good = [r for r in rows if r[2] >= SPEED_WAVE]
    stretch, stretches = [], []
    for r in good:
        if stretch and abs(r[1] - stretch[-1][1]) > RATE // 20:
            stretches.append(stretch)
            stretch = []
        stretch.append(r)
    if stretch:
        stretches.append(stretch)
    lines.append("")
    for s in stretches:
        if len(s) < 3:
            continue
        mt = sum(r[0] for r in s) / len(s)
        mo = sum(r[1] for r in s) / len(s)
        slope = sum((r[0] - mt) * (r[1] - mo) for r in s) / sum((r[0] - mt) ** 2 for r in s)
        drift = slope / RATE * 1e6
        lines.append(
            f"speed from {s[0][0]:.1f} to {s[-1][0]:.1f} s of the port: Flycast's offset drifts {drift:+.0f} ppm"
        )
    waves = [r[2] for r in rows]
    if waves:
        lines.append(
            f"{len(rows)} windows compared; wave: median {sorted(waves)[len(waves) // 2]:.3f}, "
            f"{sum(1 for w in waves if w >= 0.9)} at 0.9 or more"
        )
    report = "\n".join(lines) + "\n"
    (out / "report.txt").write_text(report)
    sys.stdout.write(report)


if __name__ == "__main__":
    main()
