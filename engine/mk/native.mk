# The native slice's rules: the game's C built for a 64-bit host
# (ENGINE.md#native-game)

# The slice compiles the game's C as make dc does, with NONMATCHING defined,
# but not DREAMCAST, whose side holds the SH-4's own instructions, so the PC
# builds what make dc builds but for those: asm2c.py writes the shipped bodies
# they stand in for, and engine/native/intrinsics.h declares SHC's
# intrinsics, each in the SH-4's arithmetic. marks.py's CONSOLE names the
# guards whose console side has a matching side the slice takes instead. The
# compiler is clang, whatever CC names, as the passes below read its IR. It
# reads the disc's data as make dc does, as the content and bodies under
# build/gen/, which make pc writes from the disc, as make and make dc do
#
# NATIVE_HOST=windows builds the 64-bit slice for Windows, on Linux, in its
# own directory: the game's C and the host's for x86-64 Windows, against
# llvm-mingw's headers, C runtime and tools, fetched at the release
# tools/SHA256SUMS sums. The image is lld's, at the domain's 0: COFF keeps an
# absolute address in 32 bits, and Windows' code reaches a name by a 32-bit
# displacement. lld links no partial object, so a program's partial link and
# its copy are coff.py's
NATIVE_HOST ?= linux
NATIVE_WINDOWS := $(filter windows,$(NATIVE_HOST))
NATIVE    ?= build/engine/native$(if $(NATIVE_WINDOWS),-windows)
NATIVE_EXE := $(if $(NATIVE_WINDOWS),.exe)
LLVM_MINGW_RELEASE := 20261020
LLVM_MINGW_AT := https://github.com/mstorsjo/llvm-mingw/releases/download/$(LLVM_MINGW_RELEASE)
LLVM_MINGW := tools/llvm-mingw/llvm-mingw-$(LLVM_MINGW_RELEASE)-ucrt-ubuntu-22.04-x86_64
# llvm-mingw keeps the receipt of its pin, as engine.mk says. Its archive
# unpacks where nothing lies, so no file stays that another archive left
# under the release's name. NATIVE_TOOLCHAIN, the receipt, is what a compile
# or a link for Windows waits on: another llvm-mingw builds the slice again
LLVM_MINGW_PIN := $(call pinned,tools/llvm-mingw,$(LLVM_MINGW_AT))
LLVM_MINGW_FETCHED := tools/llvm-mingw/fetched.txt
$(LLVM_MINGW).tar.xz: $(call unfetched,tools/llvm-mingw,$(LLVM_MINGW_PIN))
	@$(call status,fetch,$@)
	$(Q)rm -f $(LLVM_MINGW_FETCHED) && mkdir -p $(@D) && $(call fetch,$(LLVM_MINGW_AT)/$(@F),$@)
$(LLVM_MINGW)/bin/ld.lld: $(LLVM_MINGW).tar.xz
	$(Q)rm -f $(LLVM_MINGW_FETCHED) && rm -rf $(LLVM_MINGW) && tar -xJf $< -C $(dir $(LLVM_MINGW)) && touch $@
$(LLVM_MINGW_FETCHED): $(LLVM_MINGW)/bin/ld.lld
	$(Q)$(call receipt,tools/llvm-mingw,$(LLVM_MINGW_PIN))
NATIVE_TOOLCHAIN := $(if $(NATIVE_WINDOWS),$(LLVM_MINGW_FETCHED))
NATIVE_NM := $(if $(NATIVE_WINDOWS),$(LLVM_MINGW)/bin/llvm-nm,nm)
NATIVE_COFF := $(if $(NATIVE_WINDOWS),engine/tools/coff.py)
ifneq ($(NATIVE_WINDOWS),)
ifneq ($(filter-out 0,$(NATIVE_DOMAIN)),)
$(error NATIVE_DOMAIN=$(NATIVE_DOMAIN): NATIVE_HOST=windows links the game at the domain's 0 alone, as COFF keeps an \
	absolute address in 32 bits: leave NATIVE_DOMAIN unset, or give it 0)
endif
endif
# What gives the slice's commands their words, beside the files each reads:
# the make files that write them, $(NATIVE)/request.txt, the record of
# what no file's date tracks, which NATIVE_REQUEST below gives its text, and
# for Windows the toolchain's receipt. Each compile and each link of the
# slice depends on them all, so a tree built under one request and then
# under another holds what a build from nothing under the second holds
NATIVE_MK := engine/engine.mk engine/mk/play_math.mk engine/mk/native.mk
NATIVE_NEEDS := $(NATIVE_MK) $(NATIVE)/request.txt $(NATIVE_TOOLCHAIN)
# The images the slice links: 1ST_READ.BIN, the menus, and every program the
# loader reads beside them, each stage, field, boss, garden and minigame,
# each event and each character's motions. The movie player and the hand-off
# to Dream Passport are the shim's. An image's content comes from build/gen/
# where its data.txt says it has one
NATIVE_PROGRAMS := $(filter-out 1st_read advertise movie loader,$(patsubst config/%/splits.txt,%,$(wildcard \
	config/*/splits.txt)))
NATIVE_IMAGES := 1st_read advertise $(NATIVE_PROGRAMS)
NATIVE_GEN_C  := $(patsubst config/%/data.txt,build/gen/%/content.c,$(wildcard $(NATIVE_IMAGES:%=config/%/data.txt)))
ifneq ($(filter native pc play $(NATIVE)/%,$(MAKECMDGOALS)),)
$(call check_host,$(NATIVE_CC) readelf $(if $(NATIVE_WINDOWS),,nm))
endif
ifneq ($(filter native play $(NATIVE)/%,$(MAKECMDGOALS)),)
ifneq ($(filter-out $(wildcard $(NATIVE_GEN_C)),$(NATIVE_GEN_C)),)
$(error $(filter-out $(wildcard $(NATIVE_GEN_C)),$(NATIVE_GEN_C)): the native slice reads the disc's data from \
	build/gen/; run make pc first)
endif
endif

# The slice compiles a copy of the game's C that marks.py writes under
# $(NATIVE)/src. Make rereads this fragment once the copy is current, so a
# rule below sees it. The copy holds a file for each of NATIVE_SOURCES, the
# C and headers under marks.py's roots, and $(NATIVE)/sources.txt is their
# inventory, so make writes the copy again once a source joins or leaves.
# marks.py removes no file, so the recipe removes each copy whose source is
# gone: a header that left the tree would still be the first one a compile
# finds. The copy waits on the request and on no toolchain: make brings
# marked.mk up to date as it starts, under make -n too, and would fetch
# llvm-mingw to do it. The record is read by include, which stops
# make where the copy can't be made again: -include would go on and build an
# older copy
NATIVE_TREE := $(NATIVE)/src
NATIVE_SOURCES := $(filter src/% modules/% events/%,$(C_FILES))
ifneq ($(filter native pc play $(NATIVE)/%,$(MAKECMDGOALS)),)
$(call record,$(NATIVE)/sources.txt,$(sort $(NATIVE_SOURCES)))
$(NATIVE)/marked.mk: engine/tools/marks.py engine/dc/dreamcast.h $(NATIVE_SOURCES) $(NATIVE)/sources.txt \
		$(NATIVE_MK) $(NATIVE)/request.txt
	@$(call status,marks,$(NATIVE_TREE))
	$(Q)python3 engine/tools/marks.py tree $(NATIVE_TREE)
	$(Q)find $(NATIVE_TREE) -name '*.[ch]' | while read -r copy; do \
		[ -e "$${copy#$(NATIVE_TREE)/}" ] || rm "$$copy" || exit 1; done
	$(Q)echo '# $(NATIVE_TREE) is current' > $@
include $(NATIVE)/marked.mk
endif

# How the slice compiles the game. The prelude gives the SDK's 32-bit
# scalars their width on the host, and __ptr32 needs -fms-extensions. The
# game's C keeps SHC's rules, as GCC_AS_SHC in config/gcc.mk gives them: no
# strict aliasing, null tests kept, signed arithmetic that wraps. Common
# symbols link as SHC's do. The census, not the build, keeps the warnings.
# On Windows the game's errno, SHC's word _errno, takes another name, as the
# UCRT's _errno is a function
NATIVE_ARCH := $(if $(NATIVE_WINDOWS),--target=x86_64-w64-mingw32)
# NATIVE_OPT, NATIVE, NATIVE_DOMAIN, NATIVE_HOST and
# NATIVE_EXPLORE come from the environment, as make refuses a setting it
# doesn't list, and a record holds each: the request, or NATIVE_EXPLORE's own.
# NATIVE_DOMAIN places the game's 4 GB of addresses on the host, and the image
# with them; NATIVE_CHECK=--check refuses a pointer outside it
NATIVE_OPT ?= -O2
NATIVE_DOMAIN ?= 0
NATIVE_CHECK ?=
NATIVE_REFS := --base $(NATIVE_DOMAIN) $(NATIVE_CHECK)
# NATIVE_EXPLORE=1, from the environment too, builds past what the slice's
# tables don't allow, so that a build reaches its next missing name. stubs.py
# writes a stub for a name with no row of engine/native/stubs.txt, asm2c.py
# leaves out a function it can't write as C, and programs.py gives a name no
# rule binds to the program its readers read most. Each says what it passed
# over. With any other value, 0 among them, or with none, each of them fails
# there. Their outputs wait on a record of the setting, so a build without it
# links nothing that one with it wrote
NATIVE_EXPLORE ?=
NATIVE_EXPLORING := $(if $(filter 1,$(NATIVE_EXPLORE)),--explore)
NATIVE_EXPLORED := $(NATIVE)/explore.id
ifneq ($(filter native pc play $(NATIVE)/%,$(MAKECMDGOALS)),)
$(call record,$(NATIVE_EXPLORED),$(if $(NATIVE_EXPLORING),explore,fail))
endif
# The non-matching code takes the compiler's own stddef.h and stdint.h, as
# make dc's takes GCC's
NATIVE_RESOURCE := $(shell $(NATIVE_CC) -print-resource-dir 2>/dev/null)
NATIVE_CFLAGS := $(NATIVE_ARCH) -std=gnu89 -ffreestanding -nostdinc -isystem $(NATIVE_RESOURCE)/include -DNONMATCHING \
	-include engine/native/prelude.h $(NATIVE_OPT) -g -fcommon \
	-fno-strict-aliasing -fno-delete-null-pointer-checks -fwrapv -ffp-contract=off -fno-builtin -fms-extensions \
	-Wno-error=return-mismatch -w $(if $(NATIVE_WINDOWS),-D_errno=sa1_errno)
# The game's C, its data and the SDK's pass through make dc's float pass,
# which gives each constant and static initializer SHC's value and wraps each
# product SHC wouldn't fuse, so -ffp-contract=on fuses as R26 and R32 do and FMA
# gives FMAC's single rounding, while R10 fuses nothing. -fpu=single makes double
# a float, and so does R10's -double=float. marks.py then marks what the pass
# has read, and refs.py lowers what the front end made of the marks, so the
# optimizer runs on its IR
NATIVE_PASS_CFLAGS := $(filter-out -ffp-contract=off,$(NATIVE_CFLAGS)) -mfma -frounding-math
# $(call native_pass,INCLUDES,RELEASE,SHC'S FLAGS) compiles $< to $@ through
# both passes, under $(call native_pass_flags,RELEASE,SHC'S FLAGS)
native_pass_flags = $(NATIVE_PASS_CFLAGS) -ffp-contract=$(if $(filter r26 r32,$1),on,off) \
	$(if $(filter -fpu=single -double=float,$2),-Ddouble=float)
# The pass reads each source under the release and the flags SHC builds it
# with, as make dc's float pass reads it. The Makefile's routes give them:
# R32 and GAME for the game's C and R10 and SDK for the SDK's, but R10 and
# SDK for a patched source and one with inline assembly, wherever it lies,
# and R32 and GAME for Kamui's members. $(call native_r10,SOURCE) is SOURCE
# where R10 builds it though it is the game's, and $(call native_r32,SOURCE)
# where R32 builds it though it is the SDK's
native_r10 = $(filter $(PATCHED_C) $(ASMCODE),$1)
native_r32 = $(filter $(KAMUI_C),$1)
# config/compilers.mk gives a source flags of its own by naming its matching
# and Dreamcast objects, which the slice's object is not. Its own() names
# the slice's object too. A line of its that adds -double=float to SDK for
# a Dreamcast object adds it here for the slice's object of that source, so
# the two float passes read the source alike
NATIVE_DOUBLE_FLOAT := $(shell awk '/SDK \+= .*-double=float/ { for (i = 1; i <= NF; i++) \
	if (sub(/^build\/dc\//, "", $$i) && sub(/\.o:?$$/, "", $$i)) print $$i }' config/compilers.mk)
$(foreach s,$(NATIVE_DOUBLE_FLOAT),$(eval $(NATIVE)/obj/$s.o: private SDK += -double=float))
native_pass = $(NATIVE_CC) $(call native_pass_flags,$2,$3) $1 -E -MD -MP -MT $@ -MF $(@:.o=.d) \
	-o $(@:.o=.i) $< && \
	python3 tools/floats.py --shc $2 $(filter -fpu=single -double=float -aggressive=2,$3) $(@:.o=.i) && \
	python3 engine/tools/marks.py mark $(NATIVE_TREE) $(@:.o=.i) && \
	$(NATIVE_CC) $(call native_pass_flags,$2,$3) -S -emit-llvm -Xclang -disable-llvm-passes -o $(@:.o=.ll) $(@:.o=.i) && \
	python3 engine/tools/refs.py $(@:.o=.ll) $(NATIVE_REFS) && \
	$(NATIVE_CC) $(call native_pass_flags,$2,$3) -c -o $@ $(@:.o=.ll)
NATIVE_GAME_INC := sdk/include $(addprefix $(NATIVE_TREE)/,src modules events) build/gen/src build/gen/modules \
	build/gen/events build/gen/sdk
NATIVE_GEN_INC  := sdk/include $(addprefix $(NATIVE_TREE)/,src modules)

# The slice's sources: its images' C outside the SDK, as their splits.txt
# places them, and the C make dc adds to 1ST_READ.BIN, NONMATCHING_C; their
# content in place of their gen/ ranges; and the runner and the SDK's shim
# under engine/native/
NATIVE_GAME_C := $(sort $(filter src/%.c modules/%.c events/%.c,$(shell awk '!/^#/ && NF { print $$1 }' \
	$(NATIVE_IMAGES:%=config/%/splits.txt))) $(NONMATCHING_C))
NATIVE_SHIM_C := $(wildcard engine/native/*.c)
# The game adapter, which turns the game's draws into the contracts' records.
# It reads Ninja's state by the game's own headers, as the slice compiles
# them. play.c is the front end's, which play's link names itself
NATIVE_SA1_C := $(filter-out engine/sa1/play.c,$(wildcard engine/sa1/*.c))
# The game's assembly and the SDK's that play reads, which asm2c.py writes as
# C over the compatible math. Each of these Ninja sources gives a C of its
# own too, where it has one
NATIVE_NINJA  := njAddVector njCalcPoint njCalcVector njClearMatrix njCos njDetMatrix njFraction njGetMatrix \
	njGetTranslation njInitMatrix njInnerProduct njInvertMatrix njMirror njMultiMatrix njOuterProduct njPopMatrix \
	njPushMatrix njRotate njRotateX njRotateXYZ njRotateY njRotateZ njRotateZXY njRoundOff njScale njScaleV \
	njScalor njScalor2 njSetMatrix njSin njSqrt njSubVector njTan njTranslate njTranslateV njUnitMatrix \
	njUnitVector njProjectScreen njRotateViewXAbsolute njRotateViewXRelative njRotateViewYRelative \
	njRotateViewZAbsolute njRotateViewZRelative njTranslateViewAbsolute njTranslateViewRelative njSetView \
	njInitView njSetPerspective njClipZ njCreateLight njSetLightAngle njSetLightRange njDraw njMotionE njMotSrc \
	njCameraMotion njSetCamera njPointCamereInterest njRollCameraInterest njSetCameraAngle njMotLinkSrc
NATIVE_ASM    := src/system/math.src src/system/vector.src src/collision/mesh.src src/system/matrix_push.c \
	src/draw/action.c src/draw/action_link.c src/system/cache.src:matrix_load_parent \
	src/draw/palette.src $(NATIVE_NINJA:%=sdk/ninja/%.c)
# Ninja's C, for those and for the math and globals that are C alone
NATIVE_SDK_C  := $(addprefix sdk/ninja/,$(NATIVE_NINJA:%=%.c) njDistanceL2PL.c njDistanceP2L.c njDistanceP2P.c \
	njDistanceP2PL.c njDistancePL2PL.c njCollisionCheckSS.c njMem.c njIsParalellL2PL.c njIsParalellPL2PL.c \
	njSetScreen.c njGlobal.c njkm.c njControl.c njSystem.c kmvertex_kamui_lib.c kmglobal_kamui_lib.c \
	njLightSystemInit.c njDeleteLight.c njSetLightAlpha.c njSetLightLocation.c \
	njLightOn.c njLightOff.c njSetLightColor.c njSetLightDirection.c njSetLightIntensity.c njTranslateLight.c \
	njTranslateLightV.c njUnitLightMatrix.c njGenerateFogDensity.c njGenerateFogTable.c njGenerateFogTable3.c \
	njSetFogColor.c njSetFogDensity.c njSetFogTable.c njFogEnable.c njFogDisable.c njModelE.c njModelS.c \
	njDrawTriangle3D.c njtri_filled.c njtri_filled_with_tex.c njtri_wiredpoly.c njtri_wiredpoly_with_tex.c \
	njNearClipping.c njNearClippingLine.c njFarClipping.c njTexture.c njSetTextureInfo.c njDrawPolygon2D.c \
	njDrawPolygon3D.c njDrawLine2D.c njDrawLine3D.c njMotion.c njMotionS.c njMotLinkS.c njGetMotion.c njLinear.c \
	debug.c) \
	sdk/shinobi/vmsutil.c
# Play math, which the assembly's C and the adapter run on, as the slice's
# compiler builds it under the flags of the host's C. A program that plays
# links its fast path
NATIVE_PLAY_MATH := $(NATIVE)/obj/engine/numeric
# The C runtime's rand and float math, which play reads, are SHC's, where
# the host's library would give its own: all of sdk/runtime/ but its
# strings, its prints, and its double math, which the host's C needs as
# its own
NATIVE_HOST_LIB := memcmp memcpy memset strcat strcmp strcpy strlen strncpy sprintf vsprintf _fmtout toupper \
	_ctype atan2 cos frexp
# Five of them keep a 64-bit double, built without -fpu=single
NATIVE_WIDE_C := $(addprefix sdk/runtime/,_duchek.c _log10.c _unpack.c _xti.c _xtostr.c)
NATIVE_RUNTIME_C := $(filter-out $(NATIVE_HOST_LIB:%=sdk/runtime/%.c),$(sort $(wildcard sdk/runtime/*.c)))
NATIVE_OBJ := $(NATIVE_GAME_C:%.c=$(NATIVE)/obj/%.o) $(NATIVE_GEN_C:build/%.c=$(NATIVE)/obj/%.o) \
	$(NATIVE_RUNTIME_C:%.c=$(NATIVE)/obj/%.o) $(NATIVE_SHIM_C:%.c=$(NATIVE)/obj/%.o) \
	$(NATIVE_SA1_C:%.c=$(NATIVE)/obj/%.o) \
	$(NATIVE_SDK_C:%.c=$(NATIVE)/obj/%.o) $(call play_math_fast,$(NATIVE_PLAY_MATH)) $(NATIVE)/asm.o

# The host's main is the runner's, so the game's main links as sa1_main
$(NATIVE)/obj/src/main.o: private NATIVE_DEFS := -Dmain=sa1_main
# SA1's layer for its model draws, model.c, finds the backend's header for
# them, sa1/live.h, by name: on a host, SA1's adapter's, and Ninja's Kamui
# words by their path from the tree's root. The frame's end finds the core's
# header and the backend's, the PC's, as marks.py names it
$(NATIVE)/obj/src/draw/model.o: private NATIVE_DEFS := -Iengine -I$(ENGINE_INCLUDE) -Iengine/sa1 -I.
$(NATIVE)/obj/src/system/pace.o: private NATIVE_DEFS := -I$(ENGINE_INCLUDE)
# The split frame keeps its draws as the core's kept draw, and finds Ninja's
# Kamui words by their path from the root
$(NATIVE)/obj/src/system/present.o: private NATIVE_DEFS := -I$(ENGINE_INCLUDE) -I.
# Ninja's built-in font, which the overlay prints in, is the disc's data
$(NATIVE)/obj/sdk/ninja/debug.o: private NATIVE_DEFS := -Ibuild/gen/sdk
# What each compile through the passes waits on beside its source: the
# prelude, the three passes, the float pass as the Makefile's FLOATS lists
# it, config/compilers.mk, which gives the float pass SHC's flags for the
# source, NATIVE_NEEDS, and the copy's list of overlaid structs.
# $(NATIVE)/headers.txt is the inventory of the headers a compile of the
# slice's can find by a search: the game's, the SDK's, and those of the
# engine's directories the host's C searches. A header that joins a
# directory searched ahead of another's changes what a search finds, and no
# object's .d file names it
NATIVE_HEADERS := $(NATIVE)/headers.txt
$(call record,$(NATIVE_HEADERS),$(sort $(filter %.h,$(C_FILES)) $(wildcard $(addsuffix /*.h,engine/native engine/sa1 \
	$(ENGINE_INCLUDE) engine/numeric))))
NATIVE_PASSES := $(NATIVE_PRELUDE) config/compilers.mk $(FLOATS) engine/tools/marks.py engine/tools/refs.py \
	$(NATIVE_NEEDS) $(NATIVE_HEADERS) $(NATIVE_TREE)/overlaid.txt
$(NATIVE_GAME_C:%.c=$(NATIVE)/obj/%.o): $(NATIVE)/obj/%.o: $(NATIVE_TREE)/%.c $(NATIVE_PASSES)
	@$(call status,native,$*.c)
	$(Q)mkdir -p $(@D)
	$(Q)$(call native_pass,$(NATIVE_DEFS) $(addprefix -I,$(NATIVE_GAME_INC)),$(if $(call native_r10,$*.c),r10,r32),$(if \
		$(call native_r10,$*.c),$(SDK),$(GAME)))
$(NATIVE_RUNTIME_C:%.c=$(NATIVE)/obj/%.o): $(NATIVE)/obj/%.o: %.c $(NATIVE_PASSES)
	@$(call status,native,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(call native_pass,-Isdk/include -Isdk,r32,$(if $(filter $(NATIVE_WIDE_C),$<),,-fpu=single))
$(NATIVE_SDK_C:%.c=$(NATIVE)/obj/%.o): $(NATIVE)/obj/%.o: %.c $(NATIVE_PASSES)
	@$(call status,native,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(call native_pass,$(NATIVE_DEFS) -Isdk/include -Isdk,$(if $(call native_r32,$<),r32,r10),$(if \
		$(call native_r32,$<),$(GAME),$(SDK)))
$(NATIVE)/obj/gen/%.o: build/gen/%.c $(NATIVE_PASSES)
	@$(call status,native,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(call native_pass,$(addprefix -I,$(NATIVE_GEN_INC)),r32,$(GAME))
# The runner and the shim are the host's C, against the host's headers
NATIVE_SYSTEM := $(if $(NATIVE_WINDOWS),--sysroot=$(LLVM_MINGW))
NATIVE_HOST_CFLAGS := $(NATIVE_ARCH) $(NATIVE_SYSTEM) -std=gnu17 -O1 -g -fms-extensions -fno-strict-aliasing -ffp-contract=off -frounding-math \
	-DNATIVE_DOMAIN=$(NATIVE_DOMAIN)ULL -Iengine/native -Iengine/sa1 -I$(ENGINE_INCLUDE) $(PLAY_MATH_INCLUDE)
# Each object's .d file names the headers its compile read, as an object of
# the game's C does
$(NATIVE)/obj/engine/native/%.o: engine/native/%.c engine/tools/refs.py $(NATIVE_NEEDS) $(NATIVE_HEADERS)
	@$(call status,native,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(NATIVE_CC) $(NATIVE_HOST_CFLAGS) -Wall -Wextra -Werror -Wno-microsoft-anon-tag -MD -MP -MT $@ \
		-MF $(@:.o=.d) -S -emit-llvm -Xclang -disable-llvm-passes -o $(@:.o=.ll) $<
	$(Q)python3 engine/tools/refs.py $(@:.o=.ll) --base $(NATIVE_DOMAIN) --host
	$(Q)$(NATIVE_CC) $(NATIVE_HOST_CFLAGS) -w -c -o $@ $(@:.o=.ll)
# The movies' MPEG-1 decoder, pl_mpeg's one header, fetched at the commit
# tools/SHA256SUMS sums, under the MIT licence the header gives. It keeps the
# receipt of its pin, as engine.mk says, which the movie's object waits on
PL_MPEG_AT := https://raw.githubusercontent.com/phoboslab/pl_mpeg/c871f2be022ece7ef4f64230b4fb8e1fb9eb6023
PL_MPEG := tools/pl_mpeg/pl_mpeg.h
PL_MPEG_PIN := $(call pinned,tools/pl_mpeg,$(PL_MPEG_AT))
PL_MPEG_FETCHED := tools/pl_mpeg/fetched.txt
$(PL_MPEG): $(call unfetched,tools/pl_mpeg,$(PL_MPEG_PIN))
	@$(call status,fetch,$@)
	$(Q)rm -f $(PL_MPEG_FETCHED) && mkdir -p $(@D) && $(call fetch,$(PL_MPEG_AT)/$(@F),$@)
$(PL_MPEG_FETCHED): $(PL_MPEG)
	$(Q)$(call receipt,tools/pl_mpeg,$(PL_MPEG_PIN))
$(NATIVE)/obj/engine/sa1/movie.o: $(PL_MPEG_FETCHED)
# The adapter takes the game's rounding as the game's own C does, where it
# decides state a later draw reads. On Windows it takes none of Microsoft's
# old names, as mingw's string.h declares strupr, and ninja.h Ninja's own
NATIVE_SA1_CFLAGS := $(NATIVE_ARCH) $(NATIVE_SYSTEM) $(if $(NATIVE_WINDOWS),-DNO_OLDNAMES) \
	-DNATIVE_DOMAIN=$(NATIVE_DOMAIN)ULL -std=gnu17 -O2 -g -fms-extensions \
	-fno-strict-aliasing -ffp-contract=off -frounding-math \
	-DNONMATCHING -include engine/native/prelude.h -idirafter sdk/include -Isdk -Isrc -Iengine/native -Iengine/sa1 \
	-I$(ENGINE_INCLUDE) $(PLAY_MATH_INCLUDE) \
	-I$(dir $(PL_MPEG))
# It reads Ninja's models and motions as the game lays them out, in the SH-4's
# layout: $(call native_ninja,FLAGS) compiles $< to $@ through marks.py's
# marks of them and refs.py, and writes $@'s .d file, which names the
# headers it read, the game's and the SDK's among them
native_ninja = $(NATIVE_CC) $(NATIVE_SA1_CFLAGS) -E -MD -MP -MT $@ -MF $(@:.o=.d) -o $(@:.o=.i) $< && \
	python3 engine/tools/marks.py ninja $(@:.o=.i) && \
	$(NATIVE_CC) $(NATIVE_SA1_CFLAGS) $1 -Wno-unused-command-line-argument -S -emit-llvm -Xclang -disable-llvm-passes \
		-o $(@:.o=.ll) $(@:.o=.i) && \
	python3 engine/tools/refs.py $(@:.o=.ll) --base $(NATIVE_DOMAIN) --host && \
	$(NATIVE_CC) $(NATIVE_SA1_CFLAGS) -w -Wno-unused-command-line-argument -c -o $@ $(@:.o=.ll)
$(NATIVE)/obj/engine/sa1/%.o: engine/sa1/%.c $(NATIVE_PRELUDE) engine/tools/marks.py engine/tools/refs.py \
		$(NATIVE_NEEDS) $(NATIVE_HEADERS)
	@$(call status,native,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(call native_ninja,-Wall -Wextra -Wno-microsoft-anon-tag)
$(eval $(call play_math,$(NATIVE_PLAY_MATH),$$(NATIVE_CC) $$(NATIVE_HOST_CFLAGS) -O2,$(NATIVE_NEEDS),native))
# asm2c.py's C, which reaches the game by 32-bit addresses. A function it
# can't write fails the build
$(NATIVE)/asm.c: engine/tools/asm2c.py $(foreach f,$(NATIVE_ASM),$(firstword $(subst :, ,$f))) engine/mk/native.mk \
		$(NATIVE_EXPLORED)
	@$(call status,asm2c,$@)
	$(Q)mkdir -p $(@D)
	$(Q)python3 engine/tools/asm2c.py $@ $(NATIVE_ASM) $(NATIVE_EXPLORING)
# $(call native_reads,FLAGS) writes $@'s .d file, the headers $< reads under
# FLAGS, for an object one command compiles. It runs ahead of the compile:
# clang writes a .d file after the object of the same command, and an object
# older than its .d file compiles again on every run
native_reads = $(NATIVE_CC) $1 -M -MP -MT $@ -MF $(@:.o=.d) $<
$(NATIVE)/asm.o: $(NATIVE)/asm.c $(NATIVE_NEEDS) $(NATIVE_HEADERS)
	@$(call status,native,$<)
	$(Q)$(call native_reads,$(NATIVE_HOST_CFLAGS)) && $(NATIVE_CC) $(NATIVE_HOST_CFLAGS) -w -c -o $@ $<
# Ninja's system source runs but for what waits on the hardware, whose names
# the shim takes
$(NATIVE)/obj/sdk/ninja/njSystem.o: private NATIVE_DEFS := -DnjInitSystem=ninja_init_system \
	-DnjChangeSystem=ninja_change_system -DnjInitVertexBuffer=ninja_init_vertex_buffer \
	-DnjWaitVSync=ninja_wait_vsync -DnjSetVSyncFunction=ninja_set_vsync_function -DnjExitSystem=ninja_exit_system
$(NATIVE)/obj/sdk/ninja/njTexture.o: private NATIVE_DEFS := -DnjLoadTexture=ninja_load_texture \
	-DnjInitTexture=ninja_init_texture
$(NATIVE)/obj/sdk/ninja/kmglobal_kamui_lib.o: private NATIVE_DEFS := -DkmSetBackGroundPlane=kamui_set_back_ground_plane
# Ninja's 2D polygons and lines, 3D lines and easy models are SA1's layer
# over the Dreamcast engine's sends on their non-matching side, and the
# shipped C on the other, which the slice takes, as SHC's builds do
$(NATIVE)/obj/sdk/ninja/njDrawPolygon2D.o $(NATIVE)/obj/sdk/ninja/njDrawLine2D.o \
$(NATIVE)/obj/sdk/ninja/njDrawLine3D.o $(NATIVE)/obj/sdk/ninja/njModelE.o: private NATIVE_DEFS := -UNONMATCHING
# Each object's .d file keeps the headers it read, but play math's, whose
# fragment names what its sources include. An object depends on its .d file,
# so one whose .d file is gone compiles again and writes it, and the empty
# recipe lets make go on without it until then. The pattern is the game's
# objects', $(NATIVE)/obj/%.o, since $* takes the stem of a target's last
# static pattern rule
NATIVE_READ := $(filter-out $(call play_math_fast,$(NATIVE_PLAY_MATH)),$(filter $(NATIVE)/obj/%,$(NATIVE_OBJ))) \
	$(NATIVE)/obj/engine/sa1/play.o
$(NATIVE_READ): $(NATIVE)/obj/%.o: $(NATIVE)/obj/%.d
$(NATIVE)/asm.o: $(NATIVE)/asm.d
$(NATIVE_READ:.o=.d) $(NATIVE)/asm.d: ;
-include $(NATIVE_READ:.o=.d) $(NATIVE)/asm.d
# Under make pc, each body source's object waits for its bodies, as its SHC
# object does
ifneq ($(filter pc,$(MAKECMDGOALS)),)
$(filter $(NATIVE_OBJ),$(BODY_C:%.c=$(NATIVE)/obj/%.o)): $(NATIVE)/obj/%.o: build/gen/%/bodies.txt
endif

# A program loads where others do, so the names its data take by address
# meet another's. Each program links alone first, as its splits.txt places its
# sources, and keeps global only the names something else reads. Where two
# programs define one, programs.py gives it to the one make dc's link binds
# its readers to, by the imports' rows and the memory contract, and fails
# where those name none. Its data and bss take sections of their own, which
# loads.c gives back as the loader reads the program's file again, by
# programs.c's table
native_program_obj = $(filter $(addprefix $(NATIVE)/obj/,$(patsubst %.c,%.o,$(filter-out src/%,$(filter %.c,$(shell \
	awk '!/^#/ && NF { print $$1 }' config/$1/splits.txt))))) $(NATIVE)/obj/gen/$1/%,$(NATIVE_OBJ))
# NATIVE_PROGRAM_OBJ is each program's name, with a colon, then its objects
NATIVE_PROGRAM_OBJ := $(foreach s,$(NATIVE_PROGRAMS),$s: $(call native_program_obj,$s))
NATIVE_REST_OBJ := $(filter-out $(NATIVE_PROGRAM_OBJ),$(NATIVE_OBJ))
NATIVE_LINK_OBJ := $(NATIVE_REST_OBJ) $(NATIVE_PROGRAMS:%=$(NATIVE)/program-%.o) $(NATIVE)/programs.o \
	$(NATIVE)/disc.o
# A link names its objects by make's lists, which the tables and the host
# give, so an object that leaves a list changes no file's date.
# $(NATIVE)/objects.txt is the inventory of each link's objects, which make
# records below, beside the request, and NATIVE_LINKED what each link, and
# each file written from a link's objects, waits on beside them
NATIVE_LINKED := $(NATIVE)/objects.txt $(NATIVE_NEEDS)
# tools/memory.py with the modules it imports, as the Makefile's
# MEMORY_TOOLS lists them, and the sources it reads, as its SOURCES lists
# them, which the anchors and the stubs read beside the tables
NATIVE_MEMORY := $(MEMORY_TOOLS) \
	$(if $(filter native pc play $(NATIVE)/%,$(MAKECMDGOALS)),$(call from_tools,__import__("memory").SOURCES))
# An anchor of the memory contract, a name make dc's link binds to the
# address its image's tables give, binds there in the domain, in the program
# that reads it: a field's message file holds pointers into itself at its
# anchor
NATIVE_ANCHORS := $(NATIVE)/anchors.txt
$(NATIVE_ANCHORS): $(NATIVE_MEMORY) $(filter config/%,$(TABLES)) engine/mk/native.mk
	$(Q)mkdir -p $(@D)
	$(Q)python3 -c 'import sys; sys.path[:0] = ["tools"]; import images, memory; \
		[print(image, name, at) for image, names in memory.contract(images.images()).anchored.items() \
			for name, at in names.items()]' > $@.tmp && mv $@.tmp $@
NATIVE_DEFSYM := $(if $(NATIVE_WINDOWS),--defsym ,-Wl$(comma)--defsym$(comma))
native_anchors = $$(awk -v p=$1 '$$1 == p { print $$2, $$3 }' $(NATIVE_ANCHORS) | \
	while read name at; do printf ' $(NATIVE_DEFSYM)%s=0x%x' $$name $$(($(NATIVE_DOMAIN) + at)); done)
NATIVE_PARTIAL := $(if $(NATIVE_WINDOWS),python3 engine/tools/coff.py -r,$(NATIVE_CC) $(NATIVE_ARCH) -r -nostdlib \
	-Wl$(comma)-d)
$(NATIVE_PROGRAMS:%=$(NATIVE)/program-%.r): $(NATIVE)/program-%.r: $(NATIVE_OBJ) $(NATIVE_ANCHORS) $(NATIVE_COFF) \
		$(NATIVE_LINKED)
	@$(call status,link,$@)
	$(Q)$(NATIVE_PARTIAL) $(call native_anchors,$*) -o $@ $(call native_program_obj,$*)
$(NATIVE)/programs.c: engine/tools/programs.py config/imports.txt config/1st_read/imports.txt $(NATIVE_MEMORY) \
		$(NATIVE_REST_OBJ) $(NATIVE_PROGRAMS:%=$(NATIVE)/program-%.r) $(NATIVE_LINKED) $(NATIVE_EXPLORED)
	@$(call status,programs,$@)
	$(Q)NM=$(NATIVE_NM) python3 engine/tools/programs.py $(NATIVE_EXPLORING) $(NATIVE) $(NATIVE_PROGRAMS) -- \
		$(NATIVE_REST_OBJ)
# PE gives no section's bounds by name: there a program's section sorts
# between programs.c's two marks of it, by the name after its $
native_section = $(if $(NATIVE_WINDOWS),'.$1$$sa1_$2$$m',sa1_$1_$2)
NATIVE_OBJCOPY := $(if $(NATIVE_WINDOWS),python3 engine/tools/coff.py,objcopy)
$(NATIVE_PROGRAMS:%=$(NATIVE)/program-%.o): $(NATIVE)/program-%.o: $(NATIVE)/program-%.r $(NATIVE)/programs.c \
		$(NATIVE_COFF)
	$(Q)$(NATIVE_OBJCOPY) --redefine-syms=$(NATIVE)/rename-$*.txt --keep-global-symbols=$(NATIVE)/keep-$*.txt \
		--rename-section .data=$(call native_section,data,$*) --rename-section .bss=$(call native_section,bss,$*) $< $@
$(NATIVE)/programs.o: $(NATIVE)/programs.c engine/native/native.h $(NATIVE_NEEDS)
	$(Q)$(NATIVE_CC) $(NATIVE_HOST_CFLAGS) -c -o $@ $<
# The build's disc, which the runner holds the disc it is given to: each
# original file's line of SHA256SUMS, as a row of the table gdfs.c reads. A
# SHA256SUMS with no such line fails, as a table of no rows would pass any
# disc
$(NATIVE)/disc.c: SHA256SUMS $(NATIVE_MK)
	@$(call status,sums,$@)
	$(Q)mkdir -p $(@D)
	$(Q)awk 'BEGIN { print "#include \"native.h\"\nconst struct native_disc_sum native_disc_sums[] = {" } \
		sub(/^orig\//, "", $$2) { printf "    {\"%s\", \"%s\"},\n", $$2, $$1; rows++ } \
		END { print "};\nconst int native_disc_sum_count = sizeof native_disc_sums / sizeof *native_disc_sums;"; \
			exit !rows }' $< > $@.tmp || { echo '$<: no line names a file of orig/'; exit 1; }
	$(Q)mv $@.tmp $@
$(NATIVE)/disc.o: $(NATIVE)/disc.c engine/native/native.h $(NATIVE_NEEDS)
	$(Q)$(NATIVE_CC) $(NATIVE_HOST_CFLAGS) -c -o $@ $<

# Each name the slice calls or reads that no source of its defines, and the
# host's C library doesn't, links to what its row of engine/native/stubs.txt
# allows, which stubs.py writes: a call reports its name once and returns 0,
# and a read finds zeros. The tables give what stubs.py places in the domain,
# and play's link gives the engine's calls. A name with no row fails the
# build, and so does a row no object needs
$(NATIVE)/stubs.c: $(NATIVE_LINK_OBJ) engine/tools/stubs.py engine/native/stubs.txt engine/tools/coff.py \
		$(NATIVE_MEMORY) config/1st_read/symbols.txt config/1st_read/imports.txt \
		$(foreach i,advertise b_robo b_e101_r loader,config/$i/splits.txt) $(NATIVE_LINKED) $(NATIVE_EXPLORED)
	@$(call status,stubs,$@)
	$(Q)NM=$(NATIVE_NM) python3 engine/tools/stubs.py $@ $(NATIVE_LINK_OBJ) --aliases $(NATIVE)/aliases.txt \
		--base $(NATIVE_DOMAIN) $(if $(NATIVE_WINDOWS),--libraries $(NATIVE_LIBRARIES)) $(NATIVE_EXPLORING)
# Windows' C library: the UCRT's, mingw's, its POSIX clocks' and compiler-rt's
NATIVE_LIBRARIES = $(addprefix $(LLVM_MINGW)/x86_64-w64-mingw32/lib/,libucrt.a libmingwex.a libmingw32.a \
	libwinpthread.a libkernel32.a) $(wildcard $(LLVM_MINGW)/lib/clang/*/lib/windows/libclang_rt.builtins-x86_64.a)
# The calls the pads and the adapter hang on
NATIVE_WRAPS := pad_scan frame_wait njDrawTriangle3D njDrawPolygon2D njDrawLine3D njDrawTexture njDrawPolygon kmRender
$(NATIVE)/stubs.o: $(NATIVE)/stubs.c $(NATIVE_NEEDS)
	$(Q)$(NATIVE_CC) $(NATIVE_ARCH) -std=gnu17 -O1 -w -c -o $@ $<
# Past 0, the image lies in the domain too, and syCacheICI's P2 call with it.
# An image above 2 GB starts from crt files that address it relatively, as a
# PIE's do
native_at = $(shell printf '%#x' $$(($(NATIVE_DOMAIN) + $1)))
native_crt = $(if $(filter-out 0,$(NATIVE_DOMAIN)),$(foreach f,$1,$(shell $(NATIVE_CC) -print-file-name=$f)))
NATIVE_LINK := $(if $(filter-out 0,$(NATIVE_DOMAIN)),-nostartfiles -Wl$(comma)-Ttext-segment=$(call native_at,0x400000))
NATIVE_LINK_FLAGS = $(NATIVE_LINK) -no-pie -Wl,--defsym,syCacheICI_P2_=$(call native_at,0xb0000000) \
	$(call native_crt,Scrt1.o crti.o crtbeginS.o) $(NATIVE_WRAPS:%=-Wl$(comma)--wrap=%) \
	$$(awk '{ printf " -Wl,--defsym,%s=%s", $$1, $$2 }' $(NATIVE)/aliases.txt)
NATIVE_CRT_END = $(call native_crt,crtendS.o crtn.o)
NATIVE_LINKER := $(NATIVE_CC)
# Windows' image keeps its base, with no relocations, where Linux's turns
# randomizing off as it runs, and holds mingw's own libraries, so it needs
# no DLL Windows lacks. lld takes no address for a name: the runner places
# syCacheICI_P2_, and an alias is its symbol's alternate name
ifneq ($(NATIVE_WINDOWS),)
NATIVE_LINKER := $(LLVM_MINGW)/bin/x86_64-w64-mingw32-clang
NATIVE_LINK_FLAGS = -static -lwinpthread -Wl,--image-base=0x400000 -Wl,--disable-dynamicbase \
	$(NATIVE_WRAPS:%=-Wl$(comma)--wrap=%) \
	$$(awk '{ printf " -Wl,--Xlink=-alternatename:%s=%s", $$1, $$2 }' $(NATIVE)/aliases.txt)
endif

# The game: the slice's objects with the PC backend. The adapter's picture
# goes to the backend in the process through SA1's client, and its pads come
# from it. Run with a disc and a count of vsyncs alone, it opens no engine
NATIVE_PLAY_CFLAGS := $(NATIVE_ARCH) $(NATIVE_SYSTEM) -std=gnu17 -O2 -g -Wall -Wextra -Werror -Iengine/native \
	-Iengine/sa1 -I$(ENGINE_INCLUDE) -DNATIVE_BUILD_IMAGES='"$(abspath build/dc)"'
$(NATIVE)/obj/engine/sa1/play.o: engine/sa1/play.c $(NATIVE_NEEDS) $(NATIVE_HEADERS)
	@$(call status,native,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(call native_reads,$(NATIVE_PLAY_CFLAGS)) && $(NATIVE_CC) $(NATIVE_PLAY_CFLAGS) -c -o $@ $<
# The link takes the objects among its prerequisites, and waits on their
# inventory and on what gives its command too
NATIVE_PLAY_OBJ := $(NATIVE)/stubs.o $(NATIVE)/obj/engine/sa1/play.o $(SA1_CLIENT_OBJ) $(PC_OBJ)
# As it starts, play finds some of the game's statics by name in its own
# symbol table (symbols.c), and ends there, naming what it lacks, where the
# table or a name is gone. So the link keeps the table, and no rule strips
# the program: make native strips copies of it alone, and holds each to
# that end
$(NATIVE)/play: $(NATIVE_LINK_OBJ) $(NATIVE_PLAY_OBJ) $(NATIVE_LINKED)
	@$(call status,link,$@)
	$(Q)$(NATIVE_CC) $(NATIVE_LINK_FLAGS) -o $@ $(filter %.o,$^) $(PC_LIBS) -lstdc++ -lm $(NATIVE_CRT_END)
# On Windows the backend, SA1's client and the runtime's core are
# llvm-mingw's C++, each in a directory of objects of its own, headless as a
# build without SDL3 is, and play.exe holds C++'s runtime itself
ifneq ($(NATIVE_WINDOWS),)
NATIVE_CXX := $(LLVM_MINGW)/bin/x86_64-w64-mingw32-clang++
NATIVE_PC_OBJ := $(patsubst %,$(NATIVE)/pc/%.o,$(filter-out window input,$(PC_SOURCES)) window_none) \
	$(NATIVE)/sa1/client.o $(NATIVE)/sa1/basic.o $(NATIVE)/runtime/engine.o
$(NATIVE)/pc/%.o: engine/pc/%.cpp $(REPLAY_DEPS) $(VULKAN) $(VULKAN_FETCHED) $(REPLAY_SHADERS) $(NATIVE_NEEDS)
	@$(call status,c++,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(NATIVE_CXX) $(ENGINE_CXXFLAGS) -O2 -I$(ENGINE_INCLUDE) -Iengine/sa1 -Iengine/runtime -Itools/vulkan/include \
		-Ibuild/engine/replay -c -o $@ $<
$(NATIVE)/sa1/%.o: engine/sa1/%.cpp $(REPLAY_DEPS) $(NATIVE_NEEDS)
	@$(call status,c++,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(NATIVE_CXX) $(ENGINE_CXXFLAGS) -O2 -I$(ENGINE_INCLUDE) -Iengine/sa1 -Iengine/pc -c -o $@ $<
$(NATIVE)/runtime/%.o: engine/runtime/%.cpp engine/runtime/runtime.h engine/include/sa_engine.h $(NATIVE_NEEDS)
	@$(call status,c++,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(NATIVE_CXX) $(ENGINE_CXXFLAGS) -I$(ENGINE_INCLUDE) -c -o $@ $<
$(NATIVE)/play.exe: $(NATIVE_LINK_OBJ) $(NATIVE)/stubs.o $(NATIVE)/obj/engine/sa1/play.o $(NATIVE_PC_OBJ) \
		$(NATIVE_LINKED)
	@$(call status,link,$@)
	$(Q)$(NATIVE_CXX) $(NATIVE_LINK_FLAGS) -o $@ $(filter %.o,$^) -lm
endif

# The census of what stands between the game's C and the host
$(NATIVE)/census.txt: engine/tools/census.py $(NATIVE_MK) $(NATIVE)/request.txt
	@$(call status,census,$@)
	$(Q)mkdir -p $(@D)
	$(Q)python3 engine/tools/census.py $(NATIVE_CC) $@

# How many vsyncs the slice runs headless, the stage and act it begins
# after the opening, Emerald Coast's first, and the disc it
# reads them from: the directory orig/1ST_READ.BIN lies in, which holds the
# whole disc where orig/ links to an extract, or orig/ itself, where make
# extracts the file from a .gdi as it starts
NATIVE_FRAMES := 600
NATIVE_TARGET := 1 0
NATIVE_DISC   := $(patsubst %/,%,$(dir $(or $(realpath orig/1ST_READ.BIN),$(abspath orig/1ST_READ.BIN))))
# make's functions split a path at a space, so a disc under such a path has
# no one name here
ifneq ($(words $(NATIVE_DISC)),1)
$(error orig/1ST_READ.BIN lies under a directory whose path holds a space, which make can't name: link orig/ to the \
	disc by a path without one)
endif
# The front door's disc where its settings name none: this build's
$(NATIVE)/obj/engine/native/runner.o: NATIVE_HOST_CFLAGS += -DNATIVE_BUILD_DISC='"$(NATIVE_DISC)"'
# The front door's two objects hold this checkout's paths, the disc's
# directory and the images', so a record apart from the request holds both,
# and a disc that lies elsewhere compiles those two again and nothing else
$(call record,$(NATIVE)/paths.txt,disc $(NATIVE_DISC) images $(abspath build/dc))
$(NATIVE)/obj/engine/native/runner.o $(NATIVE)/obj/engine/sa1/play.o: $(NATIVE)/paths.txt
# The request, which make records as it reads this file: each setting the
# environment gives the slice, and clang as the host has it, by the version
# it gives and the directory of its own headers. A word a compile above
# takes from outside the make files belongs here. The inventory goes beside
# it: each program's objects, the rest of the slice's and the game's own,
# then the libraries the host gives the game's link
NATIVE_REQUEST := $(strip host $(NATIVE_HOST) opt $(NATIVE_OPT) \
	domain $(NATIVE_DOMAIN) check $(NATIVE_CHECK) clang $(shell $(NATIVE_CC) --version 2> /dev/null) \
	resource $(NATIVE_RESOURCE))
$(call record,$(NATIVE)/request.txt,$(NATIVE_REQUEST))
$(call record,$(NATIVE)/objects.txt,$(strip $(NATIVE_PROGRAM_OBJ) rest: $(NATIVE_REST_OBJ) play: $(NATIVE_PLAY_OBJ) \
	$(NATIVE_PC_OBJ) libraries: $(PC_LIBS)))
# The run keeps its log whether it reaches its frames or stops at a fault,
# which names the stub or the blocker the census counts. It opens no engine,
# so it needs no device. The log's last line is the run's status: exit 0
# where it reached its frames. A line NATIVE_UNREAD matches is gdfs.c's for
# a file of the disc that the host failed a read of, which the game goes on
# from as it does from a sector the drive can't read. NATIVE_REACHED is the
# command that passes on a log whose last line is exit 0 and that holds no
# such line, and fails on any other log, an empty one too. A log it fails is
# never current, so the next make runs the slice again
NATIVE_ENDED := tail -n 1 $(NATIVE)/run.txt 2> /dev/null | grep -qx 'exit 0'
NATIVE_UNREAD := ^sa1: can't read .* on the disc:
NATIVE_REACHED := $(NATIVE_ENDED) && ! grep -q "$(NATIVE_UNREAD)" $(NATIVE)/run.txt
$(NATIVE)/run.txt: $(NATIVE)/play $(shell $(NATIVE_REACHED) || echo FORCE)
	@$(call status,run,$@)
	$(Q)$(NATIVE)/play $(NATIVE_DISC) $(NATIVE_FRAMES) $(NATIVE_TARGET) > $@.tmp 2>&1; \
		echo "exit $$?" >> $@.tmp; mv $@.tmp $@
	$(Q)grep -v '^sa1: stub' $@ | tail -n 4

# A play whose file lacks its symbol table, or whose table lacks a name it
# finds a static by, starts no game: it says which, in runner.c's words, and
# ends with 2. strip makes a copy of play without the table, play.stripped,
# and one whose table lacks iamb alone, the simple drawer's ambient,
# play.lacking. Each copy's run on the disc keeps its log as play's does,
# with its status as the last line. The rule fails on a log that lacks the
# copy's words or ends otherwise, and names it. Such a log stays under its
# .tmp name, so it is never current
NATIVE_STRIP_stripped :=
NATIVE_SAYS_stripped  := holds no symbol table
NATIVE_STRIP_lacking  := -N iamb
NATIVE_SAYS_lacking   := holds no object named iamb
$(NATIVE)/stripped.txt $(NATIVE)/lacking.txt: $(NATIVE)/%.txt: $(NATIVE)/play
	@$(call status,strip,$@)
	$(Q)strip $(NATIVE_STRIP_$*) -o $(NATIVE)/play.$* $<
	$(Q)$(NATIVE)/play.$* $(NATIVE_DISC) 1 > $@.tmp 2>&1; echo "exit $$?" >> $@.tmp
	$(Q)grep -q '$(NATIVE_SAYS_$*)' $@.tmp && tail -n 1 $@.tmp | grep -qx 'exit 2' || { \
		echo "$@.tmp: the copy of play that strip made ended with $$(tail -n 1 $@.tmp), where one that" \
		"$(NATIVE_SAYS_$*) says so and ends with exit 2; read the log for what it did"; exit 1; }
	$(Q)mv $@.tmp $@

# make native fails on a log NATIVE_REACHED fails, and names the log
native: $(NATIVE)/census.txt $(NATIVE)/run.txt $(NATIVE)/stripped.txt $(NATIVE)/lacking.txt
	$(Q)$(NATIVE_ENDED) || { echo "$(NATIVE)/run.txt: the run ended with $$(tail -n 1 $(NATIVE)/run.txt), where one" \
		"that reaches its frames ends with exit 0; read the log for where it stopped"; exit 1; }
	$(Q)$(NATIVE_REACHED) || { echo "$(NATIVE)/run.txt: the run reached its frames past a read of the disc that the" \
		"host failed, which the log's first such line names:" \
		"$$(grep -m 1 "$(NATIVE_UNREAD)" $(NATIVE)/run.txt)"; exit 1; }

# The PC game, play, from the disc: the Makefile's rules write
# build/gen/ from orig/, the bodies where their sources' tables of offsets
# place them, and nothing compiles a stub, runs SHC, reads make dc's link or
# runs SH_ELF's tools
pc: $(NATIVE)/play$(NATIVE_EXE)
# Where the disc is orig/ itself, from its .gdi, the game reads every file of
# it there, so make pc extracts those the build doesn't read too, once those
# it reads have checked. build/disc.txt lists the .gdi's files
ifeq ($(NATIVE_DISC)$(FOUND),$(abspath orig)1)
pc: build/disc.txt
build/disc.txt: build/gdi | $(ORIG)
	@$(call status,gdi,$(GDI_IMAGE))
	$(Q)listed=$$(build/gdi list $(call quoted,$(GDI_IMAGE))) && printf '%s\n' "$$listed" | awk '{ print $$2 }' > $@.tmp
	$(Q)rm -rf build/unpack && files=$$(while read f; do [ -e "orig/$$f" ] || echo "$$f"; done < $@.tmp) && \
		{ [ -z "$$files" ] || build/gdi extract $(call quoted,$(GDI_IMAGE)) build/unpack $$files; } && \
		for f in $$files; do mkdir -p "$$(dirname "orig/$$f")" && mv "build/unpack/$$f" "orig/$$f" || exit 1; done
	$(Q)mv $@.tmp $@
endif

# The game in a window on the first Vulkan device, from the target's start,
# for as long as the window stays open. The build's images come first.
# PLAY_RATE=HZ, from the environment as NATIVE_OPT is, shows pictures at the
# display's rate, between the game's
play: $(NATIVE)/play
	$(Q)$(NATIVE)/play --window build/dc $(if $(PLAY_RATE),--rate $(PLAY_RATE)) $(NATIVE_DISC) 0 $(NATIVE_TARGET)


# The adapter's contracts, on models of the test's own and with its sound
# calls going to an engine of the test's own, beside Ninja's njkm.c and the
# slice's objects of play math's reference: no disc, and clang as for the
# slice. The test writes the capture of its draws, and the pictures test then
# gives that capture to a client, which holds the backend to the records the
# adapter writes
NATIVE_SA1_TEST_C := engine/tests/sa1.c engine/sa1/live.c engine/sa1/sprites.c engine/sa1/records.c \
	engine/sa1/ninja.c engine/sa1/sound.c sdk/ninja/njkm.c
NATIVE_SA1_TEST_OBJ := $(NATIVE_SA1_TEST_C:%.c=build/engine/tests/sa1-obj/%.o)
# The test's objects lie outside $(NATIVE), in one directory whatever the
# request, so the directory keeps the request's record itself
$(call record,build/engine/tests/sa1-obj/request.txt,$(NATIVE_REQUEST))
$(NATIVE_SA1_TEST_OBJ): build/engine/tests/sa1-obj/%.o: %.c $(wildcard engine/sa1/*.h) $(wildcard engine/include/*.h) \
		engine/native/native.h $(PLAY_MATH_HEADER) $(NATIVE_PRELUDE) engine/tools/marks.py engine/tools/refs.py \
		$(NATIVE_MK) build/engine/tests/sa1-obj/request.txt
	$(Q)mkdir -p $(@D)
	$(Q)$(call native_ninja,-w)
-include $(NATIVE_SA1_TEST_OBJ:.o=.d)
build/engine/tests/sa1: $(NATIVE_SA1_TEST_OBJ) $(call play_math_reference,$(NATIVE_PLAY_MATH))
	$(Q)$(NATIVE_CC) $(NATIVE_SA1_CFLAGS) -no-pie -o $@ $^ -lm
engine-test: engine-sa1
engine-sa1: build/engine/tests/sa1 build/engine/tests/pictures
	$(Q)$< build/engine/tests/sa1.capture
	$(Q)build/engine/tests/pictures build/engine/tests/sa1.capture
	$(Q)python3 engine/tests/boundary.py
	@$(call status,sa1,the adapter's contracts$(comma) its capture through a client$(comma) and what a backend may read)
.PHONY: engine-sa1

# The slice's passes, the shim's texture records, its reads of the disc, the
# memory card, the threads and the reader of a program's symbols, on sources
# and files of their own: no disc, and any C compiler. asm2c.py's C runs on
# play math, and its test links the host's objects of the reference, which
# PLAY_MATH names it. It gives its last line, or all it said where a test
# fails
engine-test: engine-native-tools
engine-native-tools: $(call play_math_reference,$(PLAY_MATH_HOST))
	$(Q)out=$$(PLAY_MATH='$^' python3 engine/tests/native_tools.py 2>&1) || { echo "$$out"; exit 1; }; \
		echo "$$out" | tail -n 1
	@$(call status,native,the slice's passes$(comma) its textures$(comma) its reads of the disc$(comma) the memory card$(comma) the threads and its symbols)
.PHONY: engine-native-tools
