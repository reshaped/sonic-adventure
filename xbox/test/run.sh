#!/bin/sh
# run.sh NAME SH_ASSEMBLY C_SOURCES: build xbox/test/NAME_test.c twice, then
# compare the two runs' output line by line.
#
# The SH-4 run links SH_ASSEMBLY, the GNU as text build/shckit writes of the
# original assembly under make gcc, as build/gcc/src/system/math.s, or an
# object make gcc built, as build/gcc/src/system/thread.o, and runs under
# qemu-sh4. emulate.py first replaces each FIPR, FTRV, FSCA and FSRRA,
# which qemu-sh4 can't run, by a call to emulate.c's model of it.
#
# The x86 run links C_SOURCES, the port's C, compiled as make pc compiles
# it, and runs on the host.
#
# Where xbox/test/NAME.sed exists, sed applies it to SH_ASSEMBLY first,
# for what qemu-sh4's layout of memory can't run as the console does; its
# comments say why.
#
# Both link libm.c, so the tests' models of FSCA and FSRRA give the same
# bits on both. It prints whether the runs match, and leaves both
# outputs and their diff under build/pc/test/NAME/. Run it from the
# checkout's root, after make gcc has written SH_ASSEMBLY
set -eu
name=$1 sh_assembly=$2 c_sources=$3
out=build/pc/test/$name
sh_elf=${SH_ELF:-tools/sh-elf}
cc=${PC_CC:-clang}
mkdir -p "$out"

sh_objects=
for source in $sh_assembly; do
    case $source in
    *.o)
        # an object make gcc built, C and its assembly, linked as it is
        sh_objects="$sh_objects $source"
        continue
        ;;
    *.c)
        # make gcc's split C of a source with inline assembly, whose
        # file-scope asm includes its translated bodies, compiled for the
        # SH-4 after NAME.sed adapts it. -ffunction-sections puts each function
        # at its section's start, where a branch from the bodies lands, as
        # tools/inline.py says
        base=$out/$(echo "$source" | tr / _)
        if [ -f "xbox/test/$name.sed" ]; then
            sed -f "xbox/test/$name.sed" "$source" > "$base.adapted.c"
        else
            cp "$source" "$base.adapted.c"
        fi
        source=$base.adapted.c
        # the bodies the C includes run as SH_ASSEMBLY's do: through
        # pairs.py and emulate.py, and the include names the result
        included=$(sed -n 's/^__asm__(".include \\"\(.*\)\\"".*/\1/p' "$source" | head -n 1)
        if [ -n "$included" ]; then
            python3 xbox/test/pairs.py "$included" "$base.included.paired.s"
            python3 xbox/test/emulate.py "$base.included.paired.s" "$base.included.s"
            sed -i "s|\.include \\\\\"$included\\\\\"|.include \\\\\"$base.included.s\\\\\"|" "$source"
        fi
        # as make gcc compiles it: its preprocessed text through the float
        # pass, for the release and flags SH_FLOATS gives, as --shc r10
        # -double=float, and no product fused into a sum, with make gcc's
        # own headers
        gcc_flags="-ml -m4-single-only -O1 -std=gnu17 -ffreestanding -fno-builtin -fno-strict-aliasing -fwrapv
            -frounding-math -ffp-contract=off -ffunction-sections -nostdinc -DGCC -DSHZ_TLS_MODEL=SHZ_TLS_DISABLED
            -isystem tools/sh4zam/include -iwithprefix include -Isdk/include -Isdk -Isrc -I."
        "$sh_elf/bin/sh-elf-gcc" $gcc_flags -E -o "$base.i" "$source"
        python3 tools/floats.py ${SH_FLOATS:---shc r32} --system tools/sh4zam/include "$base.i"
        "$sh_elf/bin/sh-elf-gcc" $gcc_flags -c -o "$base.o" "$base.i"
        sh_objects="$sh_objects $base.o"
        continue
        ;;
    esac
    base=$out/$(echo "$source" | tr / _)
    if [ -f "xbox/test/$name.sed" ]; then
        sed -f "xbox/test/$name.sed" "$source" > "$base.adapted.s"
        source=$base.adapted.s
    fi
    python3 xbox/test/pairs.py "$source" "$base.paired.s"
    python3 xbox/test/emulate.py "$base.paired.s" "$base.emulated.s"
    "$sh_elf/bin/sh-elf-as" --isa=sh4 --little -o "$base.o" "$base.emulated.s"
    sh_objects="$sh_objects $base.o"
done
"$sh_elf/bin/sh-elf-gcc" -ml -m4-single-only -O1 -ffreestanding -fno-builtin -nostdlib -static \
    -ffp-contract=off -fno-math-errno -Isdk/include \
    -Wl,-T,xbox/test/sh4.ld -Wl,-z,max-page-size=0x1000 -o "$out/sh4.elf" \
    xbox/test/start.s xbox/test/emulate.s xbox/test/emulate.c xbox/test/libm.c \
    "xbox/test/${name}_test.c" $sh_objects -lgcc

# the x86 side takes the tests' models of FIPR, FTRV, FSCA and FSRRA from
# libm.c (sh4_test_sums: float sums; port_sincos16: libm.c's sines), as
# emulate.c runs them on the SH-4 side, where the game takes the console's
# (Flycast's sums, the console's FSCA table): so a difference is the
# translation's. the port's C is make pc's
resource=$($cc -print-resource-dir)/include
flags="--target=i386-linux-gnu -msse2 -mfpmath=sse -std=gnu17 -ffreestanding -fno-builtin -nostdinc
    -isystem $resource -DGCC -DXBOX -DNDEBUG
    -U__clang__ -fgnuc-version=4.2.1 -fpack-struct=4 -fno-strict-aliasing -fwrapv -frounding-math -fno-math-errno
    -ffp-contract=off -w -O2 -Isdk/include -Isrc -I."
objects=
for source in $c_sources xbox/test/libm.c; do
    object=$out/$(echo "$source" | tr / _).o
    $cc $flags -c -o "$object" "$source"
    objects="$objects $object"
done
$cc -m32 -O1 -w -o "$out/x86" "xbox/test/${name}_test.c" $objects

qemu-sh4 "$out/sh4.elf" > "$out/sh4.txt"
"$out/x86" > "$out/x86.txt"
printf '%s: ' "$name"
python3 xbox/test/compare.py "$out/sh4.txt" "$out/x86.txt"
