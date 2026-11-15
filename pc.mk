# make pc's rules: Clang's objects for 32-bit x86, stage 0 of the port to
# the Xbox (XBOX.md#how-the-port-was-made). Run it beside the Makefile, as
#
#   make -f Makefile -f pc.mk pc
#
# The Makefile doesn't include it yet: every SHC object depends on the
# Makefile, so a hook there would rebuild the matching build. No SHC or GCC
# object depends on this file.
#
# make pc compiles the C make gcc compiles, by the same routes, include
# directories and float pass, to build/pc/ at each source's path. It
# assembles no SH-4 assembly: a .src file, or the inline assembly a source
# holds, compiles to nothing yet, and what it defines stays unresolved, for
# stage 1. It links nothing yet either. build/pc/report.txt lists the sources
# that fail to compile, and why, so a failure doesn't stop the others

# tools/floats.py takes the port's models of the SH-4's instructions
# (xbox/include/, which ninja.h includes under XBOX), which only x86
# compiles, as system headers: as the compiler reads them. Where make gcc's
# C runs SH-4 code, XBOX takes the original's bodies as native C
include config/pc.mk
$(call check_host,$(PC_CC))

# build/pc/compilers.mk gives each PC object the SHC flags
# config/compilers.mk gives its GCC object, which the float pass reads, as
# tools/pcflags.py writes them. make remakes it before it reads it
PC_COMPILERS := build/pc/compilers.mk
$(PC_COMPILERS): config/compilers.mk tools/pcflags.py
	$(Q)mkdir -p $(@D) && python3 tools/pcflags.py $< $@.tmp && mv $@.tmp $@
include $(PC_COMPILERS)

# build/pc/hot.mk holds config/gcc.mk's GCC_HOT, the sources make gcc
# compiles at -O2, which config/pc.mk's PC_OPTIMIZE reads
PC_HOT := build/pc/hot.mk
$(PC_HOT): config/gcc.mk
	$(Q)mkdir -p $(@D) && grep '^GCC_HOT' $< > $@.tmp && mv $@.tmp $@
include $(PC_HOT)

# $(call pc_of,OBJECTS) gives the PC object of each of OBJECTS, SHC's objects
# under build/, leaving out each whose source holds inline assembly, which
# the split's rules build
# less the objects xbox/ replaces, once PC_XBOX_OBJ is set below, so that the
# compilers' rules leave those to xbox/'s
pc_of = $(filter-out $(PC_XBOX_OBJ),$(patsubst build/%.obj,build/pc/%.o,$(filter-out $(INLINE_C:%.c=build/%.obj),$1)))
PC_C_OBJ := $(call pc_of,$(C_SOURCE_OBJ))
PC_INLINE_OBJ := $(patsubst %.c,build/pc/%.o,$(filter-out $(DONOR_C),$(INLINE_C)))
PC_PORT_C := $(filter %.c,$(PORT_FILES))
PC_PORT_OBJ := $(PC_PORT_C:gcc/%.c=build/pc/%.o)
PC_CONTENT_OBJ := $(GEN_CONF:%=build/pc/gen/%/content.o)
# xbox/ holds C that only make pc compiles, as gcc/ holds C that only make gcc
# compiles. xbox/P.c compiles to build/pc/P.o, in place of what any rule
# above would build there: P.src's assembly, which make pc can't build, a C
# source P.c whose SH-4 assembly it can't, or gcc/P.c. the tests, the port
# layer, the hosts and xbox/third_party's vendored libraries have rules of
# their own
PC_XBOX_C := $(shell find xbox \( -path xbox/test -o -path xbox/port -o -path xbox/host -o -path xbox/third_party \) -prune -o -name '*.c' \
	-print 2> /dev/null)
PC_XBOX_OBJ := $(PC_XBOX_C:xbox/%.c=build/pc/%.o)
PC_C_OBJ := $(filter-out $(PC_XBOX_OBJ),$(PC_C_OBJ))
PC_INLINE_OBJ := $(filter-out $(PC_XBOX_OBJ),$(PC_INLINE_OBJ))
PC_PORT_OBJ := $(filter-out $(PC_XBOX_OBJ),$(PC_PORT_OBJ))
# Stop on a source in xbox/ that replaces nothing and that PC_ADDED doesn't
# name: its object would be no image's
PC_UNREPLACED := $(filter-out $(ASM_SRC:%.src=xbox/%.c) $(INLINE_C:%=xbox/%) $(C_SOURCE_OBJ:build/%.obj=xbox/%.c) \
	$(PC_PORT_C:gcc/%=xbox/%) $(foreach a,$(PC_ADDED),$(lastword $(subst :, ,$a))),$(PC_XBOX_C))
ifneq ($(PC_UNREPLACED),)
$(error $(PC_UNREPLACED): replaces no .src file, C source or source in gcc/)
endif
PC_OBJ := $(PC_C_OBJ) $(PC_INLINE_OBJ) $(PC_PORT_OBJ) $(PC_XBOX_OBJ) $(PC_CONTENT_OBJ)

# The root, searched last, lets a source in gcc/ name a header by its path
# from the root, as gcc.mk's PORT_INC does
PORT_INC := sdk/include,src,.

# $(call pc,INCLUDES,FLAGS,RELEASE[,C]) compiles a source to $@ as gcc.mk's
# gcc macro does, with Clang and PCFLAGS: the preprocessor writes the .i, the
# float pass gives its constants SHC's values for RELEASE and FLAGS, and
# Clang compiles the .i. Where -optimize=0 is among FLAGS, Clang compiles at
# -O0. A compile that fails writes $@.failed, holding its log, and no
# object, and the run goes on: build/pc/report.txt gathers the failures
# double is float where SHC's flags make it so, -fpu=single for the game and
# the runtime and -double=float for the Ninja sources that take it, and in
# the sources of gcc/ and xbox/, which take no SHC flags and keep make gcc's
# -m4-single-only, whose double is float. the rest of the SDK keeps SHC's
# 64-bit double, as njLtMath's light math does
pc_double = $(if $(or $(filter -fpu=single -double=float,$1),$(if $(strip $1),,none)),-Ddouble=float)
pc_flags = $(PCFLAGS) $(if $(filter -optimize=0,$1),-O0) $(call pc_double,$1)
pc = mkdir -p $(@D) && rm -f $@.failed && (LC_ALL=C $(PC_CC) $(call pc_flags,$2) $(if $4,-iquote $(<D)) \
	$(addprefix -I,$(subst $(comma), ,$1)) -fno-color-diagnostics -fno-caret-diagnostics -MD -MP -MT $@ \
	-MF $(@:.o=.d) $(if $3,-E $(or $4,$<) > $(@:.o=.i) && python3 tools/floats.py \
	$(strip --shc $3 $(filter -fpu=single -double=float -aggressive=2,$2)) --system xbox/include $(@:.o=.i) && \
	LC_ALL=C $(PC_CC) $(call pc_flags,$2) -fno-color-diagnostics -fno-caret-diagnostics -c -o $@ $(@:.o=.i), \
	-c -o $@ $(or $4,$<))) > $@.log 2>&1 || (mv $@.log $@.failed; rm -f $@; true)

$(PC_OBJ): Makefile config/compilers.mk pc.mk config/pc.mk
$(PC_C_OBJ) $(PC_INLINE_OBJ) $(PC_CONTENT_OBJ): tools/floats.py $(wildcard tools/float_*.py) tools/csource.py \
	tools/tool.py tools/layout.py
$(PC_OBJ): build/headers.txt
-include $(PC_OBJ:.o=.d)

$(call pc_of,$(R32_GAME_OBJ)): build/pc/%.o: %.c
	@$(call status,clang,$<)
	$(Q)$(call pc,$(GAME_INC),$(GAME),r32)

$(call pc_of,$(R10_SDK_OBJ)): build/pc/%.o: %.c
	@$(call status,clang,$<)
	$(Q)$(call pc,$(SDK_INC),$(SDK),r10)

$(call pc_of,$(R32_RUNTIME_OBJ)): build/pc/%.o: %.c
	@$(call status,clang,$<)
	$(Q)$(call pc,$(R32_SDK_INC),$(RUNTIME),r32)

$(call pc_of,$(PATCHED_OBJ)): build/pc/%.o: %.c
	@$(call status,clang,$<)
	$(Q)$(call pc,$(PATCHED_INC),$(SDK),r10)

$(call pc_of,$(R26_SDK_OBJ)): build/pc/%.o: %.c
	@$(call status,clang,$<)
	$(Q)$(call pc,$(R26_SDK_INC),$(SDK),r26)

$(call pc_of,$(R32_SDK_OBJ)): build/pc/%.o: %.c
	@$(call status,clang,$<)
	$(Q)$(call pc,$(R32_SDK_INC),$(SDK),r32)

$(PC_PORT_OBJ): build/pc/%.o: gcc/%.c
	@$(call status,clang,$<)
	$(Q)$(call pc,$(PORT_INC))

# A source in xbox/ takes no SHC flags or float pass, as gcc/'s don't, and
# finds headers as gcc/'s do
$(PC_XBOX_OBJ): build/pc/%.o: xbox/%.c
	@$(call status,clang,$<)
	$(Q)$(call pc,$(PORT_INC))

# A source with inline assembly splits as under make gcc, and its C compiles
# without the file-scope asm the split ends in, which includes SH-4
# assembly: each function an inline body defines stays undefined, unless
# config/pc.mk's PC_SH2C names the source, whose bodies tools/sh2c.py
# translates onto the split C's end
$(patsubst %.c,build/pc/%.o,$(filter $(INLINE_C),$(PATCHED_C))): PC_INC = $(PATCHED_INC)
$(patsubst %.c,build/pc/%.o,$(filter $(INLINE_C),$(GRAFTED_C))): PC_INC = $(SDK_INC)
$(patsubst %.c,build/pc/%.o,$(filter $(MOVIE_SDK),$(ASMCODE))): PC_INC = $(SDK_INC)
PC_INC = $(ASMCODE_INC)
$(PC_INLINE_OBJ): build/pc/%.o: %.c tools/inline.py tools/sh2c.py
	@$(call status,clang,$< (split))
	$(Q)mkdir -p $(@D) && python3 tools/inline.py $< build/pc/$*.c build/pc/$*_inline.src && \
		sed -i '/^__asm__(".include /,$$d' build/pc/$*.c $(if $(filter $<,$(PC_SH2C)),&& \
		python3 tools/sh2c.py $< $(SH2C_$<) --skip-defined build/pc/$*.c > build/pc/$*.sh2c && \
		cat build/pc/$*.sh2c >> build/pc/$*.c)
	$(Q)$(call pc,$(PC_INC),$(SDK),r10,build/pc/$*.c)

# the objects make gcc gives sections of their own for their data, as
# config/gcc.mk says: an image's content keeps its definitions in the
# image's order, each in a section of its own, so a read past one meets the
# next, and the link's head rows place gdfs.o's and summary.o's by name.
# Clang has no -fno-toplevel-reorder: it writes definitions in their
# source's order, which tools/pclink.py checks
$(PC_CONTENT_OBJ): PCFLAGS += -fdata-sections
build/pc/sdk/shinobi/gdfs.o build/pc/loader/sdk/gdfs.o build/pc/modules/summary/summary.o: PCFLAGS += -fdata-sections

# An image's content compiles under the game's flags, as under make gcc
$(PC_CONTENT_OBJ): build/pc/%/content.o: build/%/content.c
	@$(call status,clang,build/$*/content.c)
	$(Q)$(call pc,$(GAME_INC),$(GAME),r32)

# build/pc/report.txt counts the objects that compiled and lists each that
# failed with its first error, then the errors by kind
PC_FAILED := $(PC_OBJ:=.failed)
build/pc/report.txt: $(PC_OBJ)
	@$(call status,report,$@)
	$(Q){ echo "compiled $(words $(wildcard $(PC_OBJ))) of $(words $(PC_OBJ)) objects"; echo; \
		echo "errors by kind:"; cat /dev/null $(wildcard $(PC_FAILED)) | grep -oE 'error: .*' | \
		sed -E "s/'[^']*'/'X'/g" | sort | uniq -c | sort -rn; echo; echo "failed objects:"; \
		for f in $(wildcard $(PC_FAILED)); do echo "$${f%.failed}: $$(grep -m1 -E 'error:' $$f)"; done; } > $@
	@head -1 $@

pc: build/pc/report.txt

# the port layer, xbox/port/, which takes the place of the SDK's hardware
# layers, PC_DROPPED, and links into 1ST_READ.BIN's image
PC_PORT_C := $(wildcard xbox/port/*.c)
PC_PORT_OBJ := $(PC_PORT_C:xbox/port/%.c=build/pc/port/%.o)
$(PC_PORT_OBJ): build/pc/port/%.o: xbox/port/%.c xbox/port/host.h xbox/port/port.h pc.mk config/pc.mk
	@$(call status,clang,$<)
	$(Q)$(call pc,$(PORT_INC))
-include $(PC_PORT_OBJ:.o=.d)

# make pc-link links every image for x86 by make gcc's spec, with
# tools/pclink.py, under build/pc/link/, which build/pc/link/report.txt
# sums up: config/pc.mk's PC_DROPPED out, the port layer into 1st_read, and
# stubs for what nothing defines yet. make gcc must have written the spec
build/pc/link/report.txt: $(PC_OBJ) $(PC_PORT_OBJ) build/gcc/link/spec.txt tools/pclink.py
	@$(call status,link,build/pc/link/)
	$(Q)python3 tools/pclink.py build/gcc/link/spec.txt build/pc/link $(PC_DROPPED:%=--drop '%') \
		$(PC_SKIPPED:%=--skip %) $(PC_PORT_OBJ:%=--add 1st_read=%) $(PC_STUBBED:%=--stubs %)
pc-link: build/pc/link/report.txt

# make pc-disc writes the x86 images as the disc's files, by tools/pcdisc.py,
# under build/pc/disc/, for the host to read in the disc's place. make pc-host
# builds the PC's host, a 32-bit Linux program, with the system's C compiler.
# the game calls it with the 4-byte stack alignment config/pc.mk builds for,
# so each of its functions aligns its own stack
build/pc/disc/images.txt: build/pc/link/report.txt tools/pcdisc.py tools/images.py build/prskit
	@$(call status,disc,build/pc/disc/)
	$(Q)python3 tools/pcdisc.py build/pc/link build/pc/disc
pc-disc: build/pc/disc/images.txt
# the host's sound: Flycast's AICA, DSP and ARM7 (xbox/third_party/flycast,
# GPL-2.0), with the DSP's recompiler for 32-bit x86 and Xbyak (BSD-3), and
# sound.cpp, its glue, in C++. -DFLYCAST_AICA_ONLY leaves out the SH-4's
# side of aica_if.cpp, its G2 DMA and RTC, which the port serves.
# PC_SOUND_FLAGS=-DFLYCAST_DSP_INTERPRETER interprets the DSP instead
PC_FLYCAST := xbox/third_party/flycast
PC_FLYCAST_SOURCES := $(addprefix $(PC_FLYCAST)/core/hw/,aica/aica.cpp aica/aica_if.cpp aica/aica_mem.cpp \
	aica/sgc_if.cpp aica/dsp.cpp aica/dsp_interp.cpp aica/dsp_x86.cpp arm7/arm7.cpp arm7/arm_mem.cpp) \
	xbox/host/pc/sound.cpp xbox/host/pc/dsp_check.cpp
PC_FLYCAST_OBJECTS := $(patsubst %.cpp,build/pc/host_sound/%.o,$(PC_FLYCAST_SOURCES))
PC_CXX ?= clang++
build/pc/host_sound/%.o: %.cpp $(wildcard $(PC_FLYCAST)/shim/*.h $(PC_FLYCAST)/shim/*/*.h $(PC_FLYCAST)/shim/*/*/*.h \
		$(PC_FLYCAST)/core/hw/aica/*.h $(PC_FLYCAST)/core/hw/arm7/*.h) xbox/host/pc/sound.h pc.mk
	@$(call status,c++,$<)
	@mkdir -p $(@D)
	$(Q)$(PC_CXX) -m32 -std=c++17 -fno-pie -O2 -g -w -DFLYCAST_AICA_ONLY -DFLYCAST_PROFILE $(PC_SOUND_FLAGS) -I$(PC_FLYCAST)/shim -I$(PC_FLYCAST)/core -I$(PC_FLYCAST)/core/deps/xbyak \
		-Ixbox/host/pc -c -o $@ $<

build/pc/host: xbox/host/pc/host.c xbox/host/pc/video.c xbox/host/pc/video.h xbox/host/pc/sound.h xbox/port/host.h pc.mk \
		$(PC_FLYCAST_OBJECTS)
	@$(call status,cc,$<)
	$(Q)$(PC_CC) -m32 -fno-pie -mstackrealign -O1 -g -Wall -c -o build/pc/host_sound/host.o xbox/host/pc/host.c
	$(Q)$(PC_CC) -m32 -fno-pie -mstackrealign -O1 -g -Wall -c -o build/pc/host_sound/video.o xbox/host/pc/video.c
	$(Q)$(PC_CXX) -m32 -fno-pie -no-pie -o $@ build/pc/host_sound/host.o build/pc/host_sound/video.o \
		$(PC_FLYCAST_OBJECTS) -lSDL2 -lGL -lrt -lpthread
pc-host: build/pc/host

# make sndreplay builds the sound core's test, xbox/host/pc/sndreplay.cpp:
# the core as the Xbox host builds it (FLYCAST_APU), run against a recording
# of the game's side (HOST_SOUND_RECORD). SNDREPLAY_FLAGS are the core's
# switches, and build/pc/sndreplay$(SNDREPLAY_NAME) the program, so that two
# builds can be compared
SNDREPLAY_SOURCES := $(addprefix $(PC_FLYCAST)/core/hw/,aica/aica.cpp aica/aica_if.cpp aica/aica_mem.cpp \
	aica/sgc_if.cpp aica/dsp.cpp aica/dsp_interp.cpp arm7/arm7.cpp arm7/arm_mem.cpp) xbox/host/pc/sndreplay.cpp \
	$(SNDREPLAY_EXTRA)
build/pc/sndreplay$(SNDREPLAY_NAME): $(SNDREPLAY_SOURCES) xbox/host/xbox/aica_channel.h pc.mk \
		$(wildcard build/sound/driver.inc $(PC_FLYCAST)/core/hw/arm7/*.h $(PC_FLYCAST)/core/hw/aica/*.h)
	@$(call status,c++,$@)
	$(Q)$(PC_CXX) -m32 -std=c++17 -fno-pie -no-pie -O2 -g -w -DFLYCAST_AICA_ONLY -DFLYCAST_APU -DFLYCAST_PROFILE \
		-DFLYCAST_DSP_INTERPRETER -DFLYCAST_TRACE $(SNDREPLAY_FLAGS) -I$(PC_FLYCAST)/shim -I$(PC_FLYCAST)/core \
		-Ixbox/host/pc -Ixbox/host/xbox -o $@ $(SNDREPLAY_SOURCES)
sndreplay: build/pc/sndreplay$(SNDREPLAY_NAME)

# make pc-run runs the game on the PC, from build/disc/, the disc's files,
# which make extracts from the .gdi in orig/
pc-run: build/pc/disc/images.txt build/pc/host
	$(Q)build/pc/host build/pc/link build/pc/disc build/disc

# make xbox builds the Xbox's host and the console's folder, build/xbox/iso/,
# for a transfer to a console's hard disk: default.xbe, host.txt and the
# packs, of which a build changes modules.pak alone (xbox/host/xbox/run.sh,
# pack.py). make xbox-disc makes build/xbox/sonic.iso from it as well.
# XBOX_SETTINGS="KEY=VALUE ..." are host.txt's lines
xbox: build/pc/disc/images.txt
	$(Q)xbox/host/xbox/run.sh stage $(XBOX_SETTINGS)
xbox-disc: build/pc/disc/images.txt
	$(Q)xbox/host/xbox/run.sh iso $(XBOX_SETTINGS)
.PHONY: pc pc-link pc-disc pc-host pc-run xbox xbox-disc sndreplay
