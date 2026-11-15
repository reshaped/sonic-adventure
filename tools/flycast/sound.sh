#!/bin/sh
# sound.sh NAME PADS QUIT [POKES]: record the sound of the PC port and of
# Flycast on the same pad script, up to frame QUIT, and compare the two.
#
# PADS and POKES are HOST_PADS and HOST_POKE, counted in rendered frames
# (HOST_CLOCK=frames), as compare.sh takes them; a poke's symbol is
# 1ST_READ.BIN's. Results go to build/pc/compare/NAME/: port.wav,
# flycast.wav and report.txt (sound.py's).
#
# The port runs headless, its sound to a WAV file (HOST_WAV), and stops at
# QUIT (HOST_QUIT). Flycast plays the script through compare.lua, which
# quits at QUIT, and its sound goes through SDL2's disk driver to a raw file
# of 44.1 kHz 16-bit stereo, as it would go to the speakers: Flycast's
# emu.cfg needs "backend = sdl2" under [audio], since its other back ends
# don't use SDL. Flycast plays its BIOS's boot first, and pads its sound
# with silence when its emulation falls behind, so sound.py lines the two up
# window by window.
#
# Run it from the checkout's root, after make pc's pc-link, pc-disc and
# pc-host. FLYCAST_DIR moves Flycast, as for compare.sh.
set -eu
name=$1 pads=$2 quit=$3 pokes=${4:-}
fly=${FLYCAST_DIR:?set FLYCAST_DIR to the folder that holds flycast.exe, under /mnt/c}
distro=${WSL_DISTRO_NAME:-Ubuntu}
out=build/pc/compare/$name

grep -q '^backend *= *sdl2' "$fly/emu.cfg" || {
    echo "sound.sh: $fly/emu.cfg needs \"backend = sdl2\" under [audio]" >&2
    exit 1
}
rm -rf "$out" && mkdir -p "$out"

# the port first, alone, so that Flycast has the machine to itself
HOST_HEADLESS=1 HOST_CLOCK=frames HOST_PADS="$pads" HOST_POKE="$pokes" HOST_QUIT="$quit" \
    HOST_WAV="$out/port.wav" build/pc/host build/pc/link build/pc/disc build/disc > "$out/port.log" 2>&1 || true
grep -q "host: quit with" "$out/port.log" || { echo "sound.sh: the port didn't reach frame $quit" >&2; exit 1; }

# each poke's SYMBOL[+OFFSET] as its console address, as compare.sh has it
flypokes=
for poke in $(echo "$pokes" | tr ',' ' '); do
    at=${poke%%:*} rest=${poke#*:}
    sym=${rest%%:*} tail=${rest#*:}
    off=0
    case $sym in *+*) off=${sym#*+} sym=${sym%%+*} ;; esac
    addr=$(awk -v s="$sym" '$2 == s { print $1; exit }' config/1st_read/symbols.txt)
    [ -n "$addr" ] || { echo "sound.sh: no symbol $sym in config/1st_read/symbols.txt" >&2; exit 1; }
    flypokes="$flypokes${flypokes:+,}$at:$(printf '0x%08x' $((addr + off))):$tail"
done
mkdir -p "$fly/compare"
rm -f "$fly/compare/"*.fb "$fly/compare/timeline.txt" "$fly/compare/flycast.raw"
printf 'pads=%s\npokes=%s\nshots=\nwindow=0\nquit=%s\n' "$pads" "$flypokes" "$quit" > "$fly/compare/plan.txt"
{ printf 'COMPARE_DIR = [[%s\\compare\\]]\n' "$(wslpath -w "$fly")"; cat tools/flycast/compare.lua; } > "$fly/flycast.lua"
gdi=$(ls orig/*.gdi | head -1)
wingdi="\\\\wsl.localhost\\$distro$(realpath "$gdi" | tr / '\\')"
winfly=$(cd "$fly" && wslpath -w "$PWD")
(cd "$fly" && SDL_AUDIODRIVER=disk SDL_DISKAUDIOFILE="$winfly\\compare\\flycast.raw" \
    WSLENV="SDL_AUDIODRIVER:SDL_DISKAUDIOFILE${WSLENV:+:$WSLENV}" timeout 900 ./flycast.exe "$wingdi" \
    > /dev/null 2>&1) || true
[ -s "$fly/compare/flycast.raw" ] || { echo "sound.sh: Flycast wrote no sound" >&2; exit 1; }
mv "$fly/compare/flycast.raw" "$out/"
cp "$fly/compare/timeline.txt" "$out/flycast_timeline.txt" 2> /dev/null || true

python3 tools/flycast/sound.py "$out"
