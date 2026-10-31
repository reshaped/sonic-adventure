# make gcc's configuration: the sources it adds, GCC's and GNU as's
# flags, the pragmas GCC may ignore and the contracts of abi functions.
# make reads it through gcc.mk, make gcc's rules, for GCC_GOALS alone,
# and no SHC object depends on it. SHC's flags for a source, which
# GCC reads too, are config/compilers.mk's

# The sources make gcc adds, which replace no .src file, each as
# IMAGE:SOURCE, SOURCE a C source in gcc/ whose object the image IMAGE links
# after its sources'
GCC_ADDED := 1st_read:gcc/src/system/overlay.c 1st_read:gcc/src/system/wide.c \
    1st_read:gcc/src/system/pace.c 1st_read:gcc/src/system/present.c \
    1st_read:gcc/src/event/skip.c

# GAS_CHECKS, which GNU as takes wherever it assembles: the .src files,
# SH4ZAM's assembly, and through GCC's -Wa, GCC's code and the inline
# assembly's text. GNU as takes SH-4's instructions alone, as asmsh does,
# where GCC's driver would ask it for SH-4A's, and fails on a warning, as
# asmsh's assemblies do
GAS_CHECKS := --isa=sh4 --fatal-warnings
# GNU as assembles the .src files little-endian, with no padding after a
# section's last byte, which asmsh doesn't write
GASFLAGS := $(GAS_CHECKS) --little --no-pad-sections
# SH4ZAM_INCLUDE, SH4ZAM's include directory, whose headers GCC reads as
# system headers. tools/floats.py leaves their constants and arithmetic as
# GCC reads them
SH4ZAM_INCLUDE := tools/sh4zam/include

# GCC_TARGET, the target and the toolchain, which every compile takes. GCC
# writes code for little-endian SH-4 under one FPU model: floats pass in FR4
# on, where SHC's code and assembly take them, and double is float, as
# -fpu=single and -double=float make it for SHC. Under SHC, an SDK source
# with neither flag, and each of the five runtime members config/compilers.mk
# builds without -fpu=single, keeps a 64-bit double. The five spell theirs
# long double, 64 bits for both compilers. SH4ZAM's headers take its SH-4
# back end, SHZ_SH4, whose sources gcc.mk archives, only under a
# single-precision FPU model, as this one is. No -D pins SHZ_BACKEND: a pin
# would take that back end under any model
GCC_TARGET := -ml -m4-single-only
# GCC reads the C as C17 with GNU's extensions: sg_xpt.h's enum bool doesn't
# parse as C23
GCC_TARGET += -std=gnu17
# GCC's own headers need no C library's, and GCC takes no library function
# for a builtin, so it folds no call of one. Its code may still call memcpy,
# memmove, memset or memcmp where the C doesn't: GCC requires those of a
# freestanding environment
GCC_TARGET += -ffreestanding
# GCC reads no host header: the route's include directories come first, as
# SHC searches them, then SH4ZAM's, which GCC's side of the code includes,
# and GCC's own
GCC_TARGET += -nostdinc -isystem $(SH4ZAM_INCLUDE) -iwithprefix include
GCC_TARGET += $(GAS_CHECKS:%=-Wa,%)
# Each object keeps GCC's debugging information, DWARF, which writes no byte
# of the code or data: the final ELFs keep it for a debugger, and make gcc
# reads each declaration's type from it
GCC_TARGET += -g
# DWARF names the checkout's directory as ., so no object, nor an ELF that
# links it, holds the checkout's path; a GCC outside the checkout still
# names its own include directory
GCC_TARGET += $(call quoted,-fdebug-prefix-map=$(CURDIR)=.)
# GCC is the macro a guard tests, as GCC.md#guards says
GCC_TARGET += -DGCC
# SH4ZAM keeps no thread-local storage, in its library or in its callers:
# under C17, its SH-4 default has its headers include threads.h, which
# neither the SDK nor GCC's own headers hold, and its one thread-local, the
# software back end's XMTRX, isn't built
GCC_TARGET += -DSHZ_TLS_MODEL=SHZ_TLS_DISABLED
# The shipping build defines NDEBUG, so assert checks nothing, as SH4ZAM's
# documentation asks of a release build: its copies assert their arguments'
# alignment and size. Nothing else in the tree or SH4ZAM reads NDEBUG. A
# checked build leaves it out, and a false assertion then traps, as
# sdk/include/assert.h says
GCC_TARGET += -DNDEBUG
# GCC_ZEROS_IN_DATA, which each C object make gcc compiles takes, keeps an
# object initialized to zero in .data, as SHC keeps it in D, so the file a
# loader reads of an image that links it holds its zeros: 25 images' files
# end before a bss that neither their startup nor their loader zeroes
GCC_ZEROS_IN_DATA := -fno-zero-initialized-in-bss
# GCC_AS_SHC, the tree's C as SHC compiles it. An access may reach an object
# through another type, as a view's do, and GCC, like SHC, doesn't assume
# that accesses of two types reach two objects
GCC_AS_SHC := -fno-strict-aliasing
# A null test after a dereference stays, as SHC keeps it
GCC_AS_SHC += -fno-delete-null-pointer-checks
# Signed addition, subtraction and multiplication wrap, as SHC's do
GCC_AS_SHC += -fwrapv
# sqrtf is FSQRT alone, with no call on its error path
GCC_AS_SHC += -fno-math-errno
# GCC leaves inexact float arithmetic to run time, where the console rounds
# it toward zero, as SHC's folding does and GCC's folding to nearest
# wouldn't. GCC calls the option experimental, so tests/test_floats.py
# holds GCC 16.2 to it for each operation of constants the tree's code
# holds. GCC must still fold each static initializer. make gcc's float
# pass, tools/floats.py, gives each decimal floating constant on the tree's
# lines SHC's value, folds by SHC's rules each static initializer there
# that holds a floating constant, names a floating type or initializes a
# floating object, and fails where it can't, as its docstrings say
GCC_AS_SHC += -frounding-math
# A product rounds before a sum takes it, as R10's code rounds it at every
# site, njDistancePL2PL's FMUL, FMUL, FSUB at 0x8c62268c in 1ST_READ.BIN
# among them, whose @bug arm's FDIVs give the image's infinity or NaN only
# while each product rounds before the subtract. R26 and R32 fuse a product
# into an FMAC where a sum adds it, as in a*b+c, c+a*b, x += a*b, a dot
# product or a*b-0.5F, whose constant they add negated, but not where it
# subtracts the product or from it: the rule tools/float_fuse.py gives, which
# probes bear out and some of SHC's FMACs part from. The gcc macro in gcc.mk
# gives an object R26 or R32 builds -ffp-contract=on after this, which fuses
# a product into a sum within one expression. So GCC fuses splash_step's sum
# as R32's FMAC at 0x8c049742 in 1ST_READ.BIN does, which takes speed.y from
# modules/stg07/slide.c's 0.2F, 0x3e4ccccc, to the image's 0x3e560d5e at the
# sixth step, where an FMUL and an FADD give 0x3e560d5d. Under on, GCC fuses
# where a sum subtracts too, as FNEG and FMAC, so make gcc's float pass
# wraps each product the rule leaves unfused, and writes a += that GCC
# wouldn't fuse as an addition it does (GCC.md#floats)
GCC_AS_SHC += -ffp-contract=off
# A bitwise CRC loop stays a loop: GCC's 256-entry tables for save.c's and
# mail.c's loops are 1,024 bytes of constants, and with them the two files'
# code is 912 bytes longer
GCC_AS_SHC += -fno-optimize-crc
# GCC_LAYOUT, how the tree's objects lay out what they hold, as its C and its
# tools read it. Each function starts a section of its own, at offset 0, so a
# branch to it from the inline assembly's P lands on it: GNU as relocates a
# branch to a symbol of another section past the symbol by its offset there.
# tools/inline.py lets a body branch to its file's C functions on that alone
GCC_LAYOUT := -ffunction-sections
# A structure's member aligns to at most 4, as under SHC. No type the
# structures hold aligns past 4 but NJS_MATRIX, which sdk/include/njtypes.h
# aligns to 8 under GCC for its own objects and in a view and a light, which
# each start with one, so each layout stays SHC's
GCC_LAYOUT += -fpack-struct=4
# GCC_ABI, the options of GCC_TARGET's and GCC_LAYOUT's that give how a call
# passes each value and how a type lays out, which tools/decls.py's model of
# GCC's SH ABI takes and holds each C unit to
GCC_ABI = $(filter -m% -fpack-struct=%,$(GCC_TARGET) $(GCC_LAYOUT))
# GCC_WARNINGS, which every compile takes. Each warning fails the compile.
# GCC warns, but not of these classes, which flag well-defined C, whose
# meaning is the same under either compiler: an unused name, braces left
# out, an address tested as true, a product's or a shift's truth, an
# enumerator no case names. Nor of a pragma GCC doesn't know: GCC may ignore
# SHC's #pragma inline, which steers SHC's inlining alone, and #pragma
# section, since the images' tables place what it places.
# test_make_gcc.Pragmas holds that no other reaches GCC: #pragma
# global_register, whose variable the inline bodies take in a register, sits
# on the shipped side of its guard
GCC_WARNINGS := -Wall -Wno-unused -Wno-missing-braces -Wno-address -Wno-int-in-bool-context -Wno-switch \
	-Wno-unknown-pragmas -Werror
# GCC_HOT, the sources GCC compiles at -O2: the code a busy frame spends its
# time in, by the cosim's profiles at 16:9 and 60 fps. Each comment gives a
# pattern's largest share of a scene's busy frame, its time less the vsync
# waits, or what -Os added to Emerald Coast's busy frame, as the cosim's
# profiled replay of the owner's recording measured it. The renderer, each
# model's points, strips and clips: 67%, Station Square
GCC_HOT := gcc/src/draw/%
# The sprite drawer: 4.0%, Chaos 0
GCC_HOT += gcc/sdk/ninja/%
# Kamui's vertex render state, set for each mesh: 8.4%, Chaos 0
GCC_HOT += sdk/ninja/kmvertex_kamui_lib.c
# Ninja's mesh headers: 2.9%, Chaos 0
GCC_HOT += sdk/ninja/njkm.c
# Ninja's matrix stack and transforms, per node: njPushMatrix's 3.0%, Chaos 0
GCC_HOT += sdk/ninja/nj%Matrix.c sdk/ninja/njTranslate% sdk/ninja/njRotate% sdk/ninja/njScale% sdk/ninja/njCalc%
# Ninja's primitive draws and their near clip: 1.6%, Speed Highway
GCC_HOT += sdk/ninja/njDraw% sdk/ninja/njNearClipping%
# The game's motion and pose walks, per node: 3.1%, Chaos 0
GCC_HOT += src/draw/action.c src/draw/pose.c
# Collision's sweeps and searches: 6.9%, Station Square
GCC_HOT += src/collision/polygon.c src/collision/world.c src/collision/collision.c
# The placement walk: 3.0%, Speed Highway
GCC_HOT += src/object/placement.c
# Path sampling: 2.1%, Station Square
GCC_HOT += src/path/path.c
# ADX's audio decode: 2.3%, Station Square
GCC_HOT += sdk/sofdec/adx_dcda.c
# The arctangent: 1.9%, Speed Highway
GCC_HOT += sdk/runtime/_i_atans.c
# The rest of the math runtime the game calls a step, fmod, the arcsine, the
# arccosine and the arctangent of a quotient, and memcpy, whose helpers -O2
# inlines: 0.8% added at -Os, Emerald Coast
GCC_HOT += sdk/runtime/_i_fmds.c sdk/runtime/_i_asins.c sdk/runtime/acosf.c sdk/runtime/_i_atn2s.c \
	sdk/runtime/memcpy.c
# The player's step and welds, the camera, the rings' attraction, and
# util.c's sphere tests: 0.3% added at -Os, Emerald Coast
GCC_HOT += src/player/player.c src/camera/camera.c src/object/ring.c src/system/util.c
# The arctangent's and sine's series, the VMU screen's pack and unpack, and
# Emerald Coast's footprints, which -O2 inlines or unrolls: 0.4% added at
# -Os, Emerald Coast
GCC_HOT += sdk/runtime/_a_atans.c sdk/runtime/_a_sins.c src/vmu/draw.c modules/stg01/footprints.c
# GCC_OPTIMIZE, a source's level: -O2 for GCC_HOT's, -Os for the rest, whose
# smaller code is the images' room in retail's memory map. Where SHC takes
# -optimize=0, GCC takes -O0 after either, which compiles each statement in
# the C's order and keeps every loop
GCC_OPTIMIZE = $(if $(filter $(GCC_HOT),$<),-O2,-Os)
# GCCFLAGS, GCC's flags for the tree's C. gcc.mk's gcc_flags follows them
# with -O0 and -ffp-contract=on where a source's SHC flags and release call
# for them. A source in gcc/, which no SHC builds, takes them too: it reads
# and writes the game's structures, in their layout
GCCFLAGS = $(GCC_TARGET) $(GCC_OPTIMIZE) $(GCC_ZEROS_IN_DATA) $(GCC_AS_SHC) $(GCC_LAYOUT) $(GCC_WARNINGS)
# GCC_SH4ZAM_FLAGS, SH4ZAM's library's flags, which take neither of the
# tree's sets: SHC never compiled the library, and its types keep the layout
# their headers declare, in its objects without -fpack-struct=4 as in the
# game's under it. A warning on its sources' lines fails as on a game
# source's. Its C, complex and matrix math, compiles at -O2, as the hot
# set's does
GCC_SH4ZAM_FLAGS := $(GCC_TARGET) -O2 $(GCC_ZEROS_IN_DATA) $(GCC_WARNINGS)
build/gcc/sh4zam/%.o: private GCCFLAGS := $(GCC_SH4ZAM_FLAGS)
# GCC 16.2 fills a BF/S's slot in car_drive with the taken arm's
# instruction, while the fall-through reads the register it writes
build/gcc/modules/adv00/traffic.o: private GCCFLAGS += -fno-delayed-branch
# CONTRACTS names each of an object's @asm abi functions, and each inline
# body that tools/tags.py --abi finds may not be call-safe, with how its GCC
# callers meet its contract beside it, as GCC.md#inline-assembly says.
# No GCC code enters these: the hardware enters the two vector stubs,
# which jump to the two masters, and the masters call the vector table's
# handlers, syIntBrkRteRte among them. C copies the two stubs to VBR, and
# stores only the masters' and syIntBrkRteRte's addresses
build/gcc/sdk/shinobi/syIntIsr.o: CONTRACTS := syIntExpISRTop syIntISRTop syIntMasterISR syIntMasterISR_2 \
    syIntBrkRteRte
# An interrupt that asked for a switch returns to syFbrSwitchToFiber_ISR_START,
# and the file's switches jump to save_context and patch_interrupt_return
build/gcc/sdk/shinobi/sg_syf2.o: CONTRACTS := syFbrSwitchToFiber_ISR_START save_context patch_interrupt_return
# No source calls _SetSP
build/gcc/src/system/crt0.o build/gcc/loader/stack.o: CONTRACTS := _SetSP
# thread_yield returns to its caller on the thread's next turn with R8 to
# R15, FR12 to FR15, GBR, MACH and MACL as the caller left them, every
# register GCC keeps across a call among them. resume and run return to no
# caller: each goes on in the running thread, run with the entry and the
# argument GCC passes in R4 and R5. prepare stores the GBR, MACH, MACL and
# FR12 to FR15 that thread_start's caller left, which seed only the new
# thread's first turn, and GCC's code sets each before it reads it
build/gcc/src/system/thread.o: CONTRACTS := thread_yield resume run prepare
# longjmp returns to no caller: it goes on past the setjmp that filled its
# env, with that setjmp's R15, or in a new fiber's function on the fiber's
# own stack, as rfCreateFiber writes them. GCC takes setjmp as returning
# twice, so rfiber.c's code keeps what lives across it in its frame, which
# stays live while the scheduler or the sleeping fiber is away
build/gcc/sdk/runtime/longjmp.o: CONTRACTS := longjmp
# queue_cursor and sq_cursor take R8 as global register variables, under GCC
# as under SHC, so GCC's code in these objects keeps nothing else in R8.
# save_queue_cursor stores the R8 an entry point's caller left, and
# restore_queue_cursor gives it back
build/gcc/sdk/ninja/njModel.o: CONTRACTS := restore_queue_cursor
build/gcc/sdk/ninja/njModelS.o: CONTRACTS := save_queue_cursor restore_queue_cursor
# Only lt_rgb_lights_body's calls through _nj_rgb_lt_func_, which
# njSetLightQueue fills, enter these, as under SHC. It clears FR3 and FR11
# for each call, which the FIPRs take as fourth terms times the 0 in FR7, and
# keeps R5 to R7 across it, which each leaves. lt_rgb_08, lt_rgb_10,
# lt_rgb_20, lt_rgb_13 and lt_rgb_25 keep R5, and the last four FR1, across
# their calls of _lt_power, lt_power_body's label, and the body writes only
# R0, R1, R4, FR0 and FR4
build/gcc/sdk/ninja/njLtSrcs.o: CONTRACTS := lt_rgb_02 lt_rgb_82 lt_rgb_04 lt_rgb_c4 lt_rgb_08 lt_rgb_88 \
    lt_rgb_c8 lt_rgb_10 lt_rgb_90 lt_rgb_20 lt_rgb_a0 lt_rgb_e0 lt_rgb_03 lt_rgb_83 lt_rgb_05 lt_rgb_c5 \
    lt_rgb_13 lt_rgb_93 lt_rgb_25 lt_rgb_a5 lt_rgb_e5
# njCreateLight ends at its call of create_light, as the if (0) after it
# never runs, so the body's early RTS and its closing JMP to njUnitMatrix end
# njCreateLight, as in SHC's expansion. The body keeps &l->attr in R7 and
# &l->ltcal in R6 across its calls of light_power and light_scale_color:
# light_power_body writes neither, and under GCC _light_scale_color is a
# veneer that keeps both around its call of the C
build/gcc/sdk/ninja/njCreateLight.o: CONTRACTS := create_light
# An image's content is its gen/ ranges' data, which make gcc links at the
# disc's addresses to check it: each definition takes a section of its own,
# which the check's link places at its address. Its zeros stay data, as the
# disc stores them, as GCCFLAGS keeps every object's. GCC writes the
# definitions in their source's order, the image's, and the final link keeps
# that order within .rodata and within .data. A range is all .rodata or all
# .data, so a read that runs past a definition meets the next of its range, as
# on the disc
build/gcc/gen/%/content.o: private GCCFLAGS += -fdata-sections -fno-toplevel-reorder
# Each variable gdfs.c defines takes a section of its own, so that the memory
# contract can place gdg_fs_libhdl, its library handle, in the head, where the
# startup reads handles. SHC's #pragma section LH puts the handle in DSGLH
build/gcc/sdk/shinobi/gdfs.o: private GCCFLAGS += -fdata-sections
build/gcc/loader/sdk/gdfs.o: private GCCFLAGS += -fdata-sections
# Each variable summary.c defines takes a section of its own, so that the
# memory contract can place summary_entries, its entry table, at the module's
# start, where the main loop reads it. SHC's #pragma section HEAD puts the
# table in DHEAD
build/gcc/modules/summary/summary.o: private GCCFLAGS += -fdata-sections
