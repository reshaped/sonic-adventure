#!/bin/sh
# compare.sh NAME PADS SHOTS [WINDOW [POKES]]: run the PC port and Flycast on
# the same pad script, capture the same frames in both, and compare them.
#
# PADS, SHOTS and POKES are HOST_PADS, HOST_SHOT and HOST_POKE, counted in
# rendered frames (HOST_CLOCK=frames). A poke's symbol is 1ST_READ.BIN's; for
# Flycast this script puts its console address from config/1st_read/
# symbols.txt in its place. Flycast captures WINDOW frames either side of
# each shot (default 6), and compare.py keeps the closest. Results go to
# build/pc/compare/NAME/: fN.png for each shot (Flycast, the port, their
# difference) and report.txt.
#
# Flycast is Windows' own, run through WSL's interop: FLYCAST_DIR (the
# Windows folder, which it must give) holds flycast.exe, its portable emu.cfg
# with rend.EmulateFramebuffer = yes, and the BIOS. This script copies
# compare.lua there as flycast.lua, and its plan into compare\. The disc is
# the .gdi in orig/, which Flycast reads through \\wsl.localhost.
#
# Run it from the checkout's root, after make pc's pc-link, pc-disc and
# pc-host. The port runs in a window, since its shots read the window, and
# under HOST_SLOW (default 3), so that Mesa's software GL drops no frame.
set -eu
name=$1 pads=$2 shots=$3 window=${4:-6} pokes=${5:-}
fly=${FLYCAST_DIR:?set FLYCAST_DIR to the folder that holds flycast.exe, under /mnt/c}
distro=${WSL_DISTRO_NAME:-Ubuntu}
out=build/pc/compare/$name
last=$(echo "$shots" | tr ',' '\n' | sort -n | tail -1)
quit=$((last + window + 2))

rm -rf "$out" && mkdir -p "$out/port" "$out/flycast"

# Flycast, in the background
mkdir -p "$fly/compare"
rm -f "$fly/compare/"*.fb "$fly/compare/timeline.txt"
# each poke's SYMBOL[+OFFSET] as its console address
flypokes=
for poke in $(echo "$pokes" | tr ',' ' '); do
    at=${poke%%:*} rest=${poke#*:}
    sym=${rest%%:*} tail=${rest#*:}
    off=0
    case $sym in *+*) off=${sym#*+} sym=${sym%%+*} ;; esac
    addr=$(awk -v s="$sym" '$2 == s { print $1; exit }' config/1st_read/symbols.txt)
    [ -n "$addr" ] || { echo "compare.sh: no symbol $sym in config/1st_read/symbols.txt" >&2; exit 1; }
    flypokes="$flypokes${flypokes:+,}$at:$(printf '0x%08x' $((addr + off))):$tail"
done
printf 'pads=%s\npokes=%s\nshots=%s\nwindow=%s\nquit=%s\n' "$pads" "$flypokes" "$shots" "$window" "$quit" \
    > "$fly/compare/plan.txt"
{ printf 'COMPARE_DIR = [[%s\\compare\\]]\n' "$(wslpath -w "$fly")"; cat tools/flycast/compare.lua; } > "$fly/flycast.lua"
gdi=$(ls orig/*.gdi | head -1)
wingdi="\\\\wsl.localhost\\$distro$(realpath "$gdi" | tr / '\\')"
(cd "$fly" && ./flycast.exe "$wingdi" > /dev/null 2>&1) &
flycast=$!

# the port, until its last shot
HOST_SLOW=${HOST_SLOW:-3} HOST_CLOCK=frames HOST_PADS="$pads" HOST_SHOT="$shots" HOST_POKE="$pokes" \
    build/pc/host build/pc/link build/pc/disc build/disc > "$out/port/log.txt" 2>&1 &
port=$!
while kill -0 $port 2> /dev/null && ! grep -q "shot_f$last.ppm at" "$out/port/log.txt"; do
    sleep 1
done
kill $port 2> /dev/null || true
for n in $(echo "$shots" | tr ',' ' '); do
    mv "build/pc/shot_f$n.ppm" "$out/port/" 2> /dev/null || true
done

# Flycast quits itself at frame $quit; give it ten minutes
t=0
while kill -0 $flycast 2> /dev/null && [ $t -lt 600 ]; do
    sleep 1
    t=$((t + 1))
done
if kill -0 $flycast 2> /dev/null; then
    echo "compare.sh: Flycast didn't reach frame $quit; stopping it" >&2
    taskkill.exe /IM flycast.exe /F > /dev/null 2>&1 || true
fi
mv "$fly/compare/"*.fb "$out/flycast/" 2> /dev/null || true
cp "$fly/compare/timeline.txt" "$out/flycast/" 2> /dev/null || true

python3 tools/flycast/compare.py "$out" "$shots" "$window"
