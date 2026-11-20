# make xbox-pc-test's flags: Clang's for the tree's C, for 32-bit x86, under the flags
# nxdk gives an Xbox build, so that the PC build and the Xbox build compile
# the same C the same way (XBOX.md#how-the-port-was-made). pc.mk reads this
# file for PC_GOALS alone

# PC_CC is the compiler. Only Clang is supported: nxdk builds with it
PC_CC ?= clang
# Clang's own headers, stddef.h and stdarg.h among them, which -nostdinc
# leaves out
PC_RESOURCE := $(shell $(PC_CC) -print-resource-dir 2> /dev/null)/include

# PC_TARGET is the Xbox's CPU, the Pentium III, and the images' format.
# Floating arithmetic takes SSE, not x87, whose wider registers would give
# other values than the SH-4's single precision (XBOX.md#architecture)
PC_TARGET := --target=i386-elf -march=pentium3 -msse -mfpmath=sse
# the images are ELF, which ld.lld links at fixed addresses by linker
# scripts, as make dc links its own. the host that loads them on the Xbox
# is nxdk's PE, which calls them by cdecl and keeps its stack 4-byte
# aligned, so their code assumes no more and realigns where it needs to
PC_TARGET += -mstack-alignment=4 -mstackrealign
# The C is C17 with GNU's extensions, as make dc reads it
PC_TARGET += -std=gnu17
# Clang reads no host header and takes no library function for a builtin, as
# nxdk compiles: its C library comes from nxdk, and later from the port
PC_TARGET += -ffreestanding -fno-builtin -nostdinc -isystem $(PC_RESOURCE)
# SH4ZAM's headers, as make dc's GCC_TARGET gives them, for the Dreamcast
# engine's drawers, which run its software back end on x86 (sdk/include/zam.h)
PC_TARGET += -isystem tools/sh4zam/include
# its XMTRX is the port's, one for the game (xbox/port/xmtrx.c), so it keeps
# none per thread, and takes no pthreads, which nxdk lacks
PC_TARGET += -DSHZ_TLS_MODEL=SHZ_TLS_DISABLED
# Each object keeps DWARF, which tools can read as make dc's read its
PC_TARGET += -g -gdwarf-4 $(call quoted,-fdebug-prefix-map=$(CURDIR)=.)
# NONMATCHING's guards hold the ports' side of the C, so the x86 build takes
# each of them, and XBOX the port's own (XBOX.md#how-the-port-was-made). It
# takes no DREAMCAST, whose guards hold the SH-4's instructions, so their
# other side, the shipped C, or the port's under XBOX, is the port's
# (PORTS.md#switches)
# pc.mk's pc_double defines double as float where SHC's flags make it a
# float. clang's stddef.h would define max_align_t with long double, which
# then reads long float, and nothing takes it, so its guard is set
PC_TARGET += -DNONMATCHING -DXBOX -DNDEBUG -D__CLANG_MAX_ALIGN_T_DEFINED
# Clang reads the tree as make dc's GCC does, the reading tools/csource.py
# and tools/inline.py model: __clang__ undefined, since the tree's #ifdef
# __clang__ declarations serve clang-tidy's reading of the shipped side, and
# a GNU C compiler
PC_TARGET += -U__clang__ -fgnuc-version=4.2.1
# a quoted include searches the root last, which holds no header, so that
# what tools/sh2c.py writes, and xbox/'s C, reach xbox/include/ by its path
PC_TARGET += -iquote .

# PC_ZEROS_IN_DATA and PC_AS_SHC keep what config/gcc.mk's GCC_ZEROS_IN_DATA
# and GCC_AS_SHC keep, each for the reason config/gcc.mk gives it
PC_ZEROS_IN_DATA := -fno-zero-initialized-in-bss
PC_AS_SHC := -fno-strict-aliasing -fno-delete-null-pointer-checks -fwrapv -fno-math-errno -frounding-math
# The Pentium III has no fused multiply-add, so no product fuses into a sum,
# where R26's and R32's FMACs do: a known difference from the console's values
PC_AS_SHC += -ffp-contract=off

# PC_LAYOUT keeps SHC's member layout, as config/gcc.mk's GCC_LAYOUT does.
# The MSVC target lays out bitfields as MSVC does unless told not to; the tree
# declares none, and -mno-ms-bitfields keeps a new one as GCC lays it out
PC_LAYOUT := -fpack-struct=4 -mno-ms-bitfields -ffunction-sections

# Stage 0 reports warnings without failing on them. The link's unresolved
# names, not warnings, are its list (XBOX.md#how-the-port-was-made)
PC_WARNINGS := -Wall -Wno-unused -Wno-missing-braces -Wno-address -Wno-int-in-bool-context -Wno-switch \
	-Wno-unknown-pragmas -Wno-c23-extensions

# as config/gcc.mk's GCC_OPTIMIZE: -Os, which keeps the images within their
# areas, and -O2 where its GCC_HOT names a source for its share of a busy
# frame
# PC_PORT_HOT adds the port's own: its renderer, which reads every list a
# frame, and because at -Os port_render and flush_strip were a fifth of a
# busy stage's samples, it compiles at -O2
PC_PORT_HOT := xbox/port/render.c
PC_OPTIMIZE = $(if $(filter $(GCC_HOT) $(PC_PORT_HOT),$<),-O2,-Os)
PCFLAGS = $(PC_TARGET) $(PC_OPTIMIZE) $(PC_ZEROS_IN_DATA) $(PC_AS_SHC) $(PC_LAYOUT) $(PC_WARNINGS)

# PC_ADDED is the sources in xbox/ that add to a source of the tree, where
# replacing it would copy its C, as SOURCE:ADDED. each defines what the
# source's assembly defines and make xbox-pc-test can't build, and links where the
# source does, as config/dreamcast.mk's DC_ADDED adds to an image
PC_ADDED := sdk/ninja/njGlobal.c:xbox/sdk/ninja/njGlobal_data.c

# PC_SH2C is the sources whose inline bodies tools/sh2c.py translates, which
# make xbox-pc-test appends to the source's split C. SH2C_SOURCE gives a source's
# arguments: the calls its bodies make, as register templates
PC_SH2C := sdk/ninja/njLtSrcs.c sdk/ninja/njSetCamera.c sdk/ninja/njSetCameraAngle.c \
	sdk/ninja/njPointCamereInterest.c sdk/ninja/njRollCameraInterest.c sdk/ninja/njModel.c sdk/ninja/njModelS.c \
	sdk/ninja/njModelE.c
# Ninja's model drawers keep their store queue cursor in R8, a C global
# under XBOX, which their bodies share. they load QACR0's address, 0xff000038,
# which is port_qacr's under XBOX (xbox/port/system.c)
SH2C_QACR := --literal "H'FF000038=&port_qacr[0]" --declare 'extern volatile Uint32 port_qacr[2];'
SH2C_sdk/ninja/njModel.c := --global R8=queue_cursor --declare 'extern Float _nj_vertex_offset_;' $(SH2C_QACR)
SH2C_sdk/ninja/njModelS.c := --global R8=sq_cursor --declare 'extern Float _nj_vertex_offset_;' $(SH2C_QACR)
# the calls Ninja's bodies make, by SHC's convention: R4 to R7 and FR4 on,
# and FR0 for a Float. each names its callee in parentheses, so that it
# reaches the function, as the bodies' JSRs do, and not a macro of its name
SH2C_NINJA_CALLS := --declare '\#include <mathf.h>' --call '_acosf=FR0 = (acosf)(FR4)' \
	--call '_njTan=FR0 = (njTan)((Angle)R4)' \
	--call '_njMultiMatrix=(njMultiMatrix)((NJS_MATRIX *)R4, (NJS_MATRIX *)R5)' \
	--call '_njRotateX=(njRotateX)((NJS_MATRIX *)R4, (Angle)R5)' \
	--call '_njRotateY=(njRotateY)((NJS_MATRIX *)R4, (Angle)R5)' \
	--call '_njRotateZ=(njRotateZ)((NJS_MATRIX *)R4, (Angle)R5)' \
	--call '_njRotate=(njRotate)((NJS_MATRIX *)R4, (NJS_VECTOR *)R5, (Angle)R6)' \
	--call '_njScale=(njScale)((NJS_MATRIX *)R4, FR4, FR5, FR6)' \
	--call '_njTranslate=(njTranslate)((NJS_MATRIX *)R4, FR4, FR5, FR6)' \
	--call '_njCalcVector=(njCalcVector)((NJS_MATRIX *)R4, (NJS_VECTOR *)R5, (NJS_VECTOR *)R6)' \
	--call '_njUnitVector=FR0 = (njUnitVector)((NJS_VECTOR *)R4)' \
	--call '_njUnitMatrix=(njUnitMatrix)((NJS_MATRIX *)R4)' \
	--declare 'extern NJS_MATRIX *_nj_base_matrix_ptr_;' --declare 'extern NJS_MATRIX _nj_camera_matrix_;'
SH2C_sdk/ninja/njSetCamera.c := $(SH2C_NINJA_CALLS)
SH2C_sdk/ninja/njSetCameraAngle.c := $(SH2C_NINJA_CALLS)
SH2C_sdk/ninja/njPointCamereInterest.c := $(SH2C_NINJA_CALLS)
SH2C_sdk/ninja/njRollCameraInterest.c := $(SH2C_NINJA_CALLS)
SH2C_sdk/ninja/njLtSrcs.c := --declare 'extern const Float fpow_tbl[17][256];' \
	--call '_lt_power=FR0 = lt_power(FR4, (Int)R4)' \
	--dynamic '((void (*)(Uint32, Uint32, Uint32, Uint32))CALLEE)(R4, R5, R6, R7)'

# PC_DROPPED is the SDK's sources that the port layer, xbox/port/, takes the
# place of, which tools/pclink.py leaves out of every image: Shinobi, the
# disc, pads, cards, timers, interrupts and sound, and Kamui, the PowerVR's
# driver. PC_SKIPPED is the images the port leaves out: loader, the hand-off
# to Dream Passport (XBOX.md#goal-and-limits). PC_STUBBED is the images whose
# names nothing defines yet take tools/pclink.py's stubs
# Kamui's sources but those that build the polygon headers' words and set
# the renderer's state, which the port keeps, as xbox/port/kamui.c says
PC_KAMUI_KEPT := sdk/ninja/kmvertex_kamui_lib.c sdk/ninja/kmglobal_kamui_lib.c sdk/ninja/bgplane_kamui_lib.c \
	sdk/ninja/kmtex_kamui_lib.c sdk/ninja/fballoc_kamui_lib.c sdk/ninja/inifile_kamui_lib.c \
	sdk/ninja/kmvtxlst_kamui_lib.c
# Shinobi's sources but the overlay library's, whose table is plain RAM, the
# backup library's and its fibers', whose block I/O alone,
# xbox/sdk/shinobi/vmsio.c, the port replaces, and the sound library's and
# the G2 bus's, which reach the AICA, the host's, through the G2 reads,
# writes and DMA xbox/sdk/shinobi/sy_g2_read.c, sy_g2_write.c and
# sy_g2_dma_trigger.c replace
PC_SHINOBI_KEPT := sdk/shinobi/ov_main.c sdk/shinobi/rfiber.c sdk/shinobi/vmsapi.c sdk/shinobi/vmscmd.c \
	sdk/shinobi/vmsfib.c sdk/shinobi/vms_rw.c sdk/shinobi/vmsutil.c sdk/shinobi/vmsphys.c sdk/shinobi/vmsio.c \
	sdk/shinobi/vmsasm.src $(wildcard sdk/shinobi/sd_*.c) $(wildcard sdk/shinobi/sy_g2_*.c)
# the movie image's Sofdec, which xbox/port/movie.c takes the place of
PC_DROPPED := modules/movie/sdk/* $(filter-out $(PC_SHINOBI_KEPT),$(wildcard sdk/shinobi/*)) $(filter-out $(PC_KAMUI_KEPT),$(wildcard sdk/ninja/*_kamui_lib.c)) \
	sdk/ninja/kmutil_workaround_for_sh_lnk.c
PC_SKIPPED := loader
PC_STUBBED := 1st_read
