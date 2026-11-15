#!/bin/sh
# run every translation test of xbox/test/, each by run.sh with its
# assembly and its C, and fail where any shows a real difference. Run it
# from the checkout's root after make gcc has written the assembly
set -u
b=build/gcc/sdk/ninja x=xbox/sdk/ninja m=xbox/test/xmtrx.c
status=0
run() {
    xbox/test/run.sh "$@" > build/pc/test/run.log 2>&1
    line=$(grep 'lines;' build/pc/test/run.log)
    echo "${line:-$1: failed to run, as build/pc/test/run.log shows}"
    case $line in
    *" 0 real,"*) ;;
    *) status=1 ;;
    esac
}
# NAME and its Ninja sources, each by its inline body and its C in xbox/
ninja() {
    name=$1
    shift
    asm= c=
    for f in "$@"; do
        case $f in
        +*) c="$c $x/${f#+}.c" ;;
        *) asm="$asm $b/${f}_inline.s" c="$c $x/$f.c" ;;
        esac
    done
    run "$name" "$asm" "$c $m"
}
mkdir -p build/pc/test
run math build/gcc/src/system/math.s xbox/src/system/math.c
run vector build/gcc/src/system/vector.s xbox/src/system/vector.c
run mesh build/gcc/src/collision/mesh.s "xbox/src/collision/mesh.c $m"
run palette build/gcc/src/draw/palette.s xbox/src/draw/palette.c
run adv0100_palette build/gcc/modules/adv0100/palette.s xbox/modules/adv0100/palette.c
run xmtrx "build/gcc/src/system/crt0.s build/gcc/src/system/cache.s" "xbox/src/system/crt0.c xbox/src/system/cache.c"
ninja ninja1 njAddVector njSubVector njOuterProduct njFraction njRoundOff njGetMatrix
ninja ninja2 njUnitMatrix njSetMatrix njInitMatrix
ninja ninja3 njRotateX njRotateY njRotateZ
# the combined rotations call the single ones, whose C the x86 run links
ninja ninja4 njRotateXYZ njRotateZXY njRotate +njRotateX +njRotateY +njRotateZ
ninja ninja5 njSetPerspective njRotateViewXAbsolute njRotateViewXRelative njRotateViewYRelative \
    njRotateViewZAbsolute njRotateViewZRelative njTranslateViewAbsolute njTranslateViewRelative
ninja ninja6 njCalcVector njClipZ njProjectScreen
# these call the rotations, which they link as they are, and standins.h's
ninja ninja7 njMultiMatrix njMirror njRotateX njRotateY
ninja ninja8 njSetView njInitView njRotateX njRotateY njRotateZ
ninja ninja10 njInvertMatrix
ninja ninja11 njDetMatrix njClearMatrix njMultiMatrix
ninja ninja9 njSetLightAngle njSetLightRange njLight njMultiMatrix njCalcVector
# njCreateLight: make gcc's split C, whose bodies run through pairs.py and
# emulate.py, on the SH-4
SH_FLOATS="--shc r10 -double=float" run ninja12 "build/gcc/sdk/ninja/njCreateLight.c $b/njUnitMatrix_inline.s" \
    "$x/njCreateLight.c $x/njUnitMatrix.c $m"
# njLtSrcs.c's calculators: make gcc's split C on the SH-4, make pc's, its
# bodies translated by tools/sh2c.py and its text through the float pass,
# on x86, with the port's njSqrt, which they call
SH_FLOATS="--shc r10 -double=float" run lights "build/gcc/sdk/ninja/njLtSrcs.c build/gcc/sdk/ninja/njLtMath.o" \
    "build/pc/sdk/ninja/njLtSrcs.i build/pc/sdk/ninja/njLtMath.i $x/njSqrt.c"
# the camera functions, whose bodies tools/sh2c.py translates, by make pc's
# text, with the rotations and the rest they call
p=build/pc/sdk/ninja
run camera "$b/njSetCamera_inline.s $b/njSetCameraAngle_inline.s $b/njPointCamereInterest_inline.s \
    $b/njRollCameraInterest_inline.s $b/njRotateX_inline.s $b/njRotateY_inline.s $b/njRotateZ_inline.s \
    $b/njRotate_inline.s $b/njMultiMatrix_inline.s $b/njCalcVector_inline.s $b/njUnitMatrix_inline.s" \
    "$p/njSetCamera.i $p/njSetCameraAngle.i $p/njPointCamereInterest.i $p/njRollCameraInterest.i \
    $x/njRotateX.c $x/njRotateY.c $x/njRotateZ.c $x/njRotate.c $x/njMultiMatrix.c $x/njCalcVector.c \
    $x/njUnitMatrix.c $m"
# the bodies whose make gcc side is GCC's own code, which make gcc never splits:
# shc_side.py keeps each source's SHC side, which inline.py splits and
# shckit translates, as make gcc does a body it compiles, on the SH-4; the
# port's C on x86
o=build/pc/test/orig
mkdir -p $o
asm= c=
for f in njPushMatrix njPopMatrix njTranslate njTranslateV njScale njScaleV njGetTranslation njCalcPoint \
    njInnerProduct njUnitVector njScalor njScalor2 njSqrt njSin njCos njTan; do
    python3 xbox/test/shc_side.py sdk/ninja/$f.c > $o/$f.c && python3 tools/inline.py $o/$f.c $o/${f}_split.c \
        $o/${f}_inline.src && python3 tools/floats.py --assembly $o/${f}_inline.src && \
        build/shckit translate -o $o/${f}_inline.s $o/${f}_inline.src
    asm="$asm $o/${f}_inline.s" c="$c $x/$f.c"
done
run ninja13 "$asm" "$c $m"
# the threads: make gcc's split C of thread.c on the SH-4, make pc's on x86
SH_FLOATS="--shc r32" run thread build/gcc/src/system/thread.c build/pc/src/system/thread.c
exit $status
