# make dc's composition of the Dreamcast port: the C its link takes in
# place of a source, the C it adds to an image, and the output sections
# of an image's data it starts on the operand cache's size. make reads it
# through dc.mk for DC_GOALS alone, and no SHC object depends on it.
# GCC compiles each source here under config/gcc.mk's flags, with no float
# pass (PORTS.md#shcs-c-under-gcc)

# DC_REPLACED, the C that replaces a source at the link, each as P.src:C,
# P.src a .src file, or P.c:C, P.c one of ASMCODE's. C compiles to
# build/dc/P.o, the object P.src would assemble to or P.c compile to, which
# the source's image links in its place. SA1's model draws, whose layer, of
# NONMATCHING_C, includes the Dreamcast engine's model walk, and the engine's
# drawers of sprite_draw_3d's sprites and njSprite's
DC_REPLACED := src/draw/model.src:src/draw/model.c src/draw/sprite.src:engine/dc/sprite.c \
    sdk/ninja/njSprite.src:engine/dc/njSprite.c
# Ninja's matrix copies, products and turns, njCalcVector, and its vector
# sums and cross product, on SH4ZAM in the shipped bodies' operand roles
# but for njMultiMatrix's product in memory (njMultiMatrix.c), each in
# place of a C source whose functions are inline assembly
NINJA_ZAM := njAddVector njCalcVector njGetMatrix njMultiMatrix njOuterProduct njRotate njRotateX njRotateXYZ \
    njRotateY njRotateZ njRotateZXY njSetMatrix njSubVector njUnitMatrix
DC_REPLACED += $(foreach f,$(NINJA_ZAM),sdk/ninja/$f.c:engine/dc/ninja/$f.c)
# The game's vector, angle and color math, and its collision meshes' pools
# and searches, on SH4ZAM in the shipped operand roles
DC_REPLACED += src/system/math.src:engine/dc/math.c src/collision/mesh.src:engine/dc/mesh.c

# DC_ADDED, the C an image adds, each as IMAGE:C. C compiles to
# build/dc/C.o, which IMAGE links after its sources', in this order. The
# Dreamcast engine's readout, 16:9 and frame owner, which takes Kamui's
# frame, then config/compilers.mk's NONMATCHING_C but the C DC_REPLACED
# names: the game's frame end, its toggles and curtain, split frame and
# skip, which the game reaches through its guards
DC_ADDED := 1st_read:engine/dc/overlay.c 1st_read:engine/dc/wide.c 1st_read:engine/dc/owner.c \
    $(patsubst %,1st_read:%,$(filter-out $(foreach r,$(DC_REPLACED),$(word 2,$(subst :, ,$r))),$(NONMATCHING_C)))

# DC_STARTS, the output sections of its data, of .rodata, .data and .bss,
# joined by commas, that each image whose tables place code starts on the
# 16 KB of the SH-4's operand cache, as tools/spec.py's --starts takes
# them, so that no change to the size of what the link lays ahead of one
# moves its data to another set of the cache. An image's kernel table can
# name its own, or none where its load area has no room. Empty, only the
# tables start any
DC_STARTS :=
