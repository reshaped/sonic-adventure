#!/bin/sh
# run.sh [SECONDS [SETTING...]]: build the Xbox host, the disc's folder
# around it, the disc image from that, and run it under xemu for SECONDS at
# most (default 120), printing the host's log.
# run.sh stage [SETTING...] builds the console's folder alone,
# build/xbox/iso/, for a transfer to a console's hard disk (make xbox);
# run.sh iso [SETTING...] the folder and build/xbox/sonic.iso from it (make
# xbox-disc). both hold the game's code and data, so they are for your own
# console only (XBOX.md). each SETTING is a KEY=VALUE line of D:\host.txt,
# as the PC host's environment: HOST_PADS=..., HOST_CLOCK=frames,
# HOST_QUIT=...
#
# run it from the checkout's root, with build/disc extracted, as for the PC
# host. the folder holds default.xbe, host.txt, textures.pak (xboxtex.c), sounds.pak (xboxsnd.c) and
# the packs (pack.py):
# modules.pak, which every build changes, and disc.pak, music.pak and
# movies.pak, which none does. PACK_STUBS=0 keeps the Dreamcast's textures
# in disc.pak (remove build/xbox/packs/disc.pak first).
#
# xemu (XEMU_DIR, the Windows folder that holds xemu.exe, which a run needs
# and make xbox doesn't) runs through WSL's
# interop and reads the image through \\wsl.localhost. the log comes from
# the guest's memory, as for probe/run.sh, read from its top down to the
# log's start and no further (xemu dies when pmemsave reads the antialiased
# surface, which lies below the log): every 15 seconds QMP's pmemsave
# dumps the 64 MB and this prints the host's "HOST LOG" buffer, until it
# ends with "HOST END" or the time is up. QMP listens on Windows'
# 127.0.0.1, which WSL can't reach, so PowerShell talks to it.
set -eu
seconds=${1:-120}
[ $# -gt 0 ] && shift
root=$(pwd)
xemu=${XEMU_DIR:-}
distro=${WSL_DISTRO_NAME:-Ubuntu}
NXDK_DIR=${NXDK_DIR:-$HOME/refs/nxdk}
PATH=$NXDK_DIR/bin:$PATH
export NXDK_DIR PATH

# the x86 images as the disc's files, build/pc/disc/, which modules.pak
# packs, brought up to date with the linked images. the host loads
# 1ST_READ.BIN's from its link, and the others from these files, whose
# imports of 1ST_READ.BIN's names are bound at the link: files older than
# a link that moved 1ST_READ.BIN's data read the wrong words, as a stage's
# texture lists. make xbox and make xbox-disc have made them already
if [ "$seconds" != stage ] && [ "$seconds" != iso ]; then
    make --no-print-directory build/pc/disc/images.txt
fi

# the port's patch to nxdk's pbkit (pb_back_buffers: the host draws on its
# own surfaces and needs two frame buffers, not pbkit's three), applied
# once to the nxdk checkout: its objects then build anew
if ! grep -q pb_back_buffers "$NXDK_DIR/lib/pbkit/pbkit.h"; then
    git -C "$NXDK_DIR" apply "$PWD/xbox/third_party/nxdk-pbkit-back-buffers.patch"
    rm -f "$NXDK_DIR"/lib/pbkit/*.obj
    echo "run.sh: xbox/third_party/nxdk-pbkit-back-buffers.patch applied to $NXDK_DIR"
fi
# and its fix of a lost swap (pb_vbl_handler retired a frame the ISR never
# swapped in, and the back buffer was the one on the screen: XBOX.md), once
if ! grep -q pb_lost_swap "$NXDK_DIR/lib/pbkit/pbkit.c"; then
    git -C "$NXDK_DIR" apply "$PWD/xbox/third_party/nxdk-pbkit-lost-swap.patch"
    rm -f "$NXDK_DIR"/lib/pbkit/*.obj
    echo "run.sh: xbox/third_party/nxdk-pbkit-lost-swap.patch applied to $NXDK_DIR"
fi
# nxdk-audio (MIT), the host's APU library, from its own repository at the
# revision the port was made with, cloned once to NXDK_AUDIO_DIR. the port's
# one change to it, nxAudioVoiceSetLoopStart (a voice that plays its start
# once, then loops a later part, as the AICA's channels do), is a patch
# applied once to the checkout
NXDK_AUDIO_DIR=${NXDK_AUDIO_DIR:-$HOME/refs/nxdk-audio}
NXDK_AUDIO_REV=fc2deca2cc1e434805ac03ca7c2f500b3b028f36
export NXDK_AUDIO_DIR
if [ ! -d "$NXDK_AUDIO_DIR" ]; then
    git clone -q https://github.com/Ryzee119/nxdk-audio "$NXDK_AUDIO_DIR"
    git -C "$NXDK_AUDIO_DIR" checkout -q "$NXDK_AUDIO_REV"
    echo "run.sh: nxdk-audio $NXDK_AUDIO_REV cloned to $NXDK_AUDIO_DIR"
fi
if ! grep -q nxAudioVoiceSetLoopStart "$NXDK_AUDIO_DIR/include/nxaudio.h"; then
    git -C "$NXDK_AUDIO_DIR" apply "$PWD/xbox/third_party/nxdk-audio-loop-start.patch"
    rm -f "$NXDK_AUDIO_DIR"/*.obj
    echo "run.sh: xbox/third_party/nxdk-audio-loop-start.patch applied to $NXDK_AUDIO_DIR"
fi
make -C xbox/host/xbox -j8 > /dev/null
# the textures' pack, converted ahead from the disc's, and the texture
# archives cut down to their keys beside it (xbox/host/xbox/xboxtex.c;
# XBOXTEX_FLAGS=-raw for no DXT)
if [ ! -x build/xbox/xboxtex ] || [ xbox/host/xbox/xboxtex.c -nt build/xbox/xboxtex ] ||
    [ xbox/port/texhash.h -nt build/xbox/xboxtex ]; then
    mkdir -p build/xbox
    cc -O2 -o build/xbox/xboxtex xbox/host/xbox/xboxtex.c
    rm -f build/xbox/textures.pak
fi
if [ ! -f build/xbox/textures.pak ] || [ ! -d build/xbox/stubs ]; then
    rm -rf build/xbox/stubs build/xbox/packs/disc.pak
    build/xbox/xboxtex build/xbox/textures.pak build/disc -stubs build/xbox/stubs ${XBOXTEX_FLAGS:-}
fi
# the sound pack, the banks' samples converted ahead (xbox/host/xbox/xboxsnd.c)
if [ ! -x build/xbox/xboxsnd ] || [ xbox/host/xbox/xboxsnd.c -nt build/xbox/xboxsnd ] ||
    [ xbox/host/xbox/sndpack.h -nt build/xbox/xboxsnd ] || [ ! -f build/xbox/sounds.pak ]; then
    cc -O2 -o build/xbox/xboxsnd xbox/host/xbox/xboxsnd.c
    build/xbox/xboxsnd build/xbox/sounds.pak build/disc/SONICADV
fi
# the packs (pack.py), in build/xbox/packs: modules.pak with each build; the
# Dreamcast disc's three where one is missing or older than what makes it
mkdir -p build/xbox/packs
which=modules
for pack in disc music movies; do
    if [ ! -f "build/xbox/packs/$pack.pak" ] || [ xbox/host/xbox/pack.py -nt "build/xbox/packs/$pack.pak" ] ||
        { [ "$pack" = disc ] && [ build/xbox/textures.pak -nt build/xbox/packs/disc.pak ]; }; then
        which="$which $pack"
    fi
done
# shellcheck disable=SC2086
python3 xbox/host/xbox/pack.py build/xbox/packs $which
# the disc's folder, by hard links, each file's date its source's: the
# console's in build/xbox/iso, a run's under xemu in build/xbox/run, so that
# a test's settings never reach the console's host.txt. host.txt is written
# only where it changes
if [ "$seconds" = stage ] || [ "$seconds" = iso ]; then
    stage=build/xbox/iso
else
    stage=build/xbox/run
fi
mkdir -p "$stage"
for old in link images disc check.txt; do
    rm -rf "${stage:?}/$old"
done
ln -f xbox/host/xbox/bin/default.xbe build/xbox/textures.pak build/xbox/sounds.pak build/xbox/packs/modules.pak \
    build/xbox/packs/disc.pak build/xbox/packs/music.pak build/xbox/packs/movies.pak "$stage/"
{
    # the console's image logs nothing on its own: the pad's Back writes a
    # report to E:\SONICADV\reportNN.txt and the log to log.txt
    # (HOST_REPORT), and a fault, a halt or a stall writes the log
    # (HOST_CRASH_LOG). HOST_PROFILE=1 and HOST_LOG_FILE=1 bring back the
    # profile every 600 blanks, written every 10 seconds. the host shows the
    # port's figures on the screen itself, unless HOST_OVERLAY=0
    if [ "$stage" = build/xbox/iso ]; then
        for default in HOST_PROFILE=0 HOST_LOG_FILE=0 HOST_REPORT=1 HOST_CRASH_LOG=1; do
            printf '%s\n' "$@" | grep -q "^${default%%=*}=" || echo "$default"
        done
    else
        # xemu's emulated CPU is too slow for the movies: a run skips them
        # unless it says HOST_MOVIES=1
        printf '%s\n' "$@" | grep -q "^HOST_MOVIES=" || echo "HOST_MOVIES=0"
    fi
    for setting in "$@"; do
        echo "$setting"
    done
} > build/xbox/host.txt.new
if cmp -s build/xbox/host.txt.new "$stage/host.txt"; then
    rm build/xbox/host.txt.new
else
    mv build/xbox/host.txt.new "$stage/host.txt"
fi
if [ "$seconds" = stage ]; then
    echo "run.sh: $stage/, $(du -sh "$stage" | cut -f1): copy all of it to the console, or the files newer than its own"
    exit 0
fi
rm -f build/xbox/sonic.iso
"$NXDK_DIR/tools/extract-xiso/build/extract-xiso" -c "$stage" build/xbox/sonic.iso > /dev/null
if [ "$seconds" = iso ]; then
    echo "run.sh: build/xbox/sonic.iso, $(du -h build/xbox/sonic.iso | cut -f1)"
    exit 0
fi
iso="\\\\wsl.localhost\\$distro$(realpath build/xbox/sonic.iso | tr / '\\')"

: "${XEMU_DIR:?set XEMU_DIR to the folder that holds xemu.exe, under /mnt/c}"
win=$(wslpath -w "$xemu")
mkdir -p "$xemu/run"
cat > "$xemu/run/qmp.ps1" << 'PS'
param([string]$command, [string]$out, [string]$extra)
for ($i = 0; $i -lt 60; $i++) { try { $c = New-Object System.Net.Sockets.TcpClient('127.0.0.1', 4444); break } catch { Start-Sleep 1 } }
$s = $c.GetStream(); $r = New-Object System.IO.StreamReader($s); $w = New-Object System.IO.StreamWriter($s); $w.AutoFlush = $true
$r.ReadLine() | Out-Null
$w.WriteLine('{"execute":"qmp_capabilities"}'); $r.ReadLine() | Out-Null
if ($command -eq 'dump') {
    # memory a megabyte at a time from the top down, as $out.N, to the one
    # that holds the log's start, whose number goes to $out.base
    for ($n = 63; $n -ge 32; $n--) {
        $file = $out + '.' + $n
        $w.WriteLine('{"execute":"pmemsave","arguments":{"val":' + ($n * 1048576) + ',"size":1048576,"filename":"' + $file.Replace('\','\\') + '"}}'); $r.ReadLine() | Out-Null
        $text = [System.Text.Encoding]::ASCII.GetString([System.IO.File]::ReadAllBytes($file))
        if ($text.Contains("HOST LOG`n")) { Set-Content -Path ($out + '.base') -Value $n; break }
    }
} elseif ($command -eq 'range') {
    $part = $extra -split ','
    $w.WriteLine('{"execute":"pmemsave","arguments":{"val":' + $part[0] + ',"size":' + $part[1] + ',"filename":"' + $out.Replace('\','\\') + '"}}'); $r.ReadLine() | Out-Null
} elseif ($command -eq 'regs') {
    $w.WriteLine('{"execute":"human-monitor-command","arguments":{"command-line":"info registers"}}')
    $r.ReadLine() | Out-File -Append -Encoding ascii $out
    $w.WriteLine('{"execute":"human-monitor-command","arguments":{"command-line":"x /96xw $esp"}}')
    $r.ReadLine() | Out-File -Append -Encoding ascii $out
    foreach ($line in ($extra -split ';')) {
        if ($line -ne '') {
            $w.WriteLine('{"execute":"human-monitor-command","arguments":{"command-line":"' + $line + '"}}')
            $r.ReadLine() | Out-File -Append -Encoding ascii $out
        }
    }
} else {
    $w.WriteLine('{"execute":"quit"}')
}
$c.Close()
PS
qmp() {
    timeout 90 powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$win\\run\\qmp.ps1" "$@" > /dev/null 2>&1 || true
}
show() {
    python3 -c '
import sys
d = open(sys.argv[1], "rb").read()
best = b""
at = d.find(b"HOST LOG\n")
while at >= 0:
    text = d[at:d.find(b"\0", at)]
    if len(text) > len(best):
        best = text
    at = d.find(b"HOST LOG\n", at + 1)
sys.stdout.write(best.decode("latin1"))
' "$xemu/run/ram.bin"
}

taskkill.exe /IM xemu.exe /F > /dev/null 2>&1 || true
sleep 2
rm -f "$xemu/run/ram.bin" "$xemu/run/ram.new" || true
(cd "$xemu" && ./xemu.exe -bios "$win\\bios\\bios.bin" -dvd_path "$iso" \
    -qmp tcp:127.0.0.1:4444,server=on,wait=off > run/xemu.log 2>&1 &)
waited=0
while [ "$waited" -lt "$seconds" ]; do
    sleep 15
    waited=$((waited + 15))
    # each dump beside the last, which stays if xemu is gone: its log is
    # then what the host said before
    rm -f "$xemu"/run/ram.new* || true
    qmp dump "$win\\run\\ram.new"
    if [ -s "$xemu/run/ram.new.base" ]; then
        base=$(tr -d '\r\n' < "$xemu/run/ram.new.base")
        for n in $(seq "$base" 63); do cat "$xemu/run/ram.new.$n"; done > "$xemu/run/ram.bin"
    fi
    if ! tasklist.exe 2> /dev/null | grep -qi xemu; then
        echo "run.sh: xemu is gone; the log is its last dump's"
        break
    fi
    [ -f "$xemu/run/ram.bin" ] && show | grep -q "^HOST END" && break
done
[ -f "$xemu/run/ram.bin" ] && show > build/xbox/log.txt
# the shots: each "shot fN at physical 0xADDR, WxH R5G6B5" line's frame, as
# build/xbox/shot_fN.png
# while xemu still runs: each is read from the guest's memory by its address
grep -o 'shot f[0-9]* at physical 0x[0-9a-f]*, [0-9]*x[0-9]*' build/xbox/log.txt | while read -r _ frame _ _ at size; do
    rm -f "$xemu/run/shot.bin"
    w=${size%x*}
    h=${size#*x}
    qmp range "$win\\run\\shot.bin" "$((${at%,})),$((w * h * 2))" < /dev/null
    [ -s "$xemu/run/shot.bin" ] || continue
    python3 -c '
import struct, sys, zlib
d = open(sys.argv[1], "rb").read()
frame, w, h = sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
rows = []
for y in range(h):
    row = bytearray(b"\0")
    for x in range(w):
        c = d[2 * (y * w + x)] | d[2 * (y * w + x) + 1] << 8
        row += bytes(((c >> 11) * 255 // 31, ((c >> 5) & 63) * 255 // 63, (c & 31) * 255 // 31))
    rows.append(bytes(row))
def chunk(kind, body):
    return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xffffffff)
png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
png += chunk(b"IDAT", zlib.compress(b"".join(rows))) + chunk(b"IEND", b"")
open("build/xbox/shot_%s.png" % frame, "wb").write(png)
print("run.sh: build/xbox/shot_%s.png" % frame)
' "$xemu/run/shot.bin" "$frame" "$w" "$h"
done
# the CPU's registers and the stack's top, three times a second apart, for
# where a hang is, with RUN_MONITOR's monitor commands, ;-separated, after
# each
rm -f "$xemu/run/regs.txt"
for k in 1 2 3; do
    qmp regs "$win\\run\\regs.txt" "${RUN_MONITOR:-}"
    sleep 1
done
qmp quit
sleep 2
taskkill.exe /IM xemu.exe /F > /dev/null 2>&1 || true
[ -f "$xemu/run/ram.bin" ] && show > build/xbox/log.txt
if [ -f "$xemu/run/regs.txt" ]; then
    sed 's/\\r\\n/\n/g' "$xemu/run/regs.txt" > build/xbox/regs.txt
fi
cat build/xbox/log.txt
