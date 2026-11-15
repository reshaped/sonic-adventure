#!/bin/sh
# tools/xemu/capture.sh NAME SECONDS [SETTING...]: an xemu run
# (xbox/host/xbox/run.sh SECONDS SETTING...) with xemu's window recorded
# about 60 times a second (capture.exe), then each picture named by the
# overlay's FRM number (frames.exe): XEMU_DIR\capture\NAME\frames\fNNNNN.jpg,
# the first picture of each frame. the window is read off the screen: keep
# it uncovered. run from the checkout's root. each report of the run (the
# pad's Back, xemu's Backspace, or HOST_REPORT_AT) also gets the 45 frames
# before it copied to NAME\pressK\: for a run played by hand, where a press
# marks what was just seen. give HOST_CLOCK=frames, so that the reports
# count the frames the pictures show
set -eu
name=$1 seconds=$2
shift 2
xemu=${XEMU_DIR:?set XEMU_DIR to the folder that holds xemu.exe, under /mnt/c}
here=$(cd "$(dirname "$0")" && pwd)
csc=/mnt/c/Windows/Microsoft.NET/Framework64/v4.0.30319/csc.exe
mkdir -p "$xemu/capture"
for tool in capture frames; do
    if [ ! -f "$xemu/capture/$tool.exe" ] || [ "$here/$tool.cs" -nt "$xemu/capture/$tool.exe" ]; then
        cp "$here/$tool.cs" "$xemu/capture/"
        (cd "$xemu/capture" && "$csc" /nologo /optimize /out:$tool.exe /r:System.Drawing.dll $tool.cs)
    fi
done
cp "$here/digits.txt" "$xemu/capture/digits.txt"
win=$(wslpath -w "$xemu/capture/$name")
rm -rf "${xemu:?}/capture/$name" "$xemu/run/xemu.log"
xbox/host/xbox/run.sh "$seconds" "$@" &
run=$!
# run.sh writes xemu.log as it starts xemu, after ending any xemu running
while [ ! -f "$xemu/run/xemu.log" ] && kill -0 "$run" 2> /dev/null; do sleep 1; done
(cd "$xemu" && ./capture/capture.exe "$win" "$((seconds + 60))" 60) &
capture=$!
wait "$run" || true
mkdir -p "$xemu/capture/$name"
touch "$xemu/capture/$name/stop"
wait "$capture" || true
cp "$xemu/capture/digits.txt" "$xemu/capture/$name/digits.txt"
(cd "$xemu/capture" && ./frames.exe "$win" sort '0684792351?')
printf "capture.sh: %s\\frames\n" "$win"
# the frames before each report
grep -o '^HOST REPORT [0-9]* of this run, at blank [0-9]*, frame [0-9]*' build/xbox/log.txt |
    while read -r _ _ press _ _ _ _ _ _ frame; do
        mkdir -p "$xemu/capture/$name/press$press"
        for f in $(seq $((frame - 45)) "$frame"); do
            to=$(printf 'f%05d.jpg' "$f")
            [ -f "$xemu/capture/$name/frames/$to" ] && cp "$xemu/capture/$name/frames/$to" "$xemu/capture/$name/press$press/"
        done
        printf "capture.sh: report %s at frame %s: %s\\press%s\n" "$press" "$frame" "$win" "$press"
    done
