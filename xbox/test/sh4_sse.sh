#!/bin/sh
# builds and runs xbox/test/sh4_sse.c: the x87 FIPR and FTRV of
# xbox/include/sse/sh4_sse.h against the C they replaced, as make pc
# compiles the game (32-bit, Pentium III, SSE math, strict rounding)
set -e
cd "$(dirname "$0")/../.."
out=build/pc/test/sh4_sse
mkdir -p "$out"
clang -m32 -march=pentium3 -msse -mfpmath=sse ${SH4_SSE_OPT:--O2} -frounding-math -ffp-contract=off -DXBOX -iquote . \
    -o "$out/sh4_sse" xbox/test/sh4_sse.c
"$out/sh4_sse"
