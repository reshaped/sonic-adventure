#!/bin/sh
# run.sh: build the probe, run it under xemu, and print its report.
#
# xemu (XEMU_DIR, the Windows folder that holds xemu.exe) runs through WSL's
# interop. xemu has no screenshot over QMP and its serial port stays empty,
# so the report comes from the guest's memory: QMP's pmemsave dumps the 64 MB
# and this finds the probe's "PROBE REPORT" buffer in it. QMP listens on
# Windows' 127.0.0.1, which WSL can't reach, so PowerShell talks to it.
set -eu
cd "$(dirname "$0")"
xemu=${XEMU_DIR:?set XEMU_DIR to the folder that holds xemu.exe, under /mnt/c}
NXDK_DIR=${NXDK_DIR:-$HOME/refs/nxdk}
PATH=$NXDK_DIR/bin:$PATH
export NXDK_DIR PATH
make -j8 > /dev/null
mkdir -p "$xemu/probe"
cp probe.iso "$xemu/probe/"
win=$(wslpath -w "$xemu")
cat > "$xemu/probe/qmp.ps1" << 'PS'
param([string]$out)
for ($i = 0; $i -lt 60; $i++) { try { $c = New-Object System.Net.Sockets.TcpClient('127.0.0.1', 4444); break } catch { Start-Sleep 1 } }
Start-Sleep 35
$s = $c.GetStream(); $r = New-Object System.IO.StreamReader($s); $w = New-Object System.IO.StreamWriter($s); $w.AutoFlush = $true
$r.ReadLine() | Out-Null
$w.WriteLine('{"execute":"qmp_capabilities"}'); $r.ReadLine() | Out-Null
$w.WriteLine('{"execute":"pmemsave","arguments":{"val":0,"size":67108864,"filename":"' + $out.Replace('\','\\') + '"}}'); $r.ReadLine() | Out-Null
$w.WriteLine('{"execute":"quit"}')
$c.Close()
PS
taskkill.exe /IM xemu.exe /F > /dev/null 2>&1 || true
sleep 2
rm -f "$xemu/probe/ram.bin"
(cd "$xemu" && ./xemu.exe -bios "$win\\bios\\bios.bin" -dvd_path "$win\\probe\\probe.iso" \
    -qmp tcp:127.0.0.1:4444,server=on,wait=off > probe/xemu.log 2>&1 &)
timeout 120 powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$win\\probe\\qmp.ps1" -out "$win\\probe\\ram.bin" || true
sleep 2
taskkill.exe /IM xemu.exe /F > /dev/null 2>&1 || true
python3 -c '
import sys
d = open(sys.argv[1], "rb").read()
at = d.find(b"PROBE REPORT\n")
while at >= 0 and d[at + 13] == 0:
    at = d.find(b"PROBE REPORT\n", at + 1)
print(d[at:d.find(b"\0", at)].decode("latin1") if at >= 0 else "run.sh: no report in the dump")
' "$xemu/probe/ram.bin"
