# the port's Xbox host, default.xbe, by nxdk: see host.c and video.c.
# run.sh builds it and the disc image around it
XBE_TITLE = Sonic\ Adventure
# nxdk-audio's checkout, which run.sh clones and patches
NXAUDIO = $(or $(NXDK_AUDIO_DIR),$(HOME)/refs/nxdk-audio)
FLYCAST = $(CURDIR)/../../third_party/flycast
SRCS = $(CURDIR)/host.c $(CURDIR)/video.c $(CURDIR)/audio.c $(CURDIR)/string.c \
	$(NXAUDIO)/audio_core.c $(NXAUDIO)/audio_buffer.c $(NXAUDIO)/audio_voice.c
# SOUND=y, the default, builds the AICA in (sound_xbox.cpp, aica_apu.c,
# Flycast's core). SOUND=n leaves it out and its registers read 0, under
# which the game hangs: the ADX library waits on the sound driver's play
# position (XBOX.md)
SOUND ?= y
ifeq ($(SOUND),y)
SRCS += $(CURDIR)/aica_apu.c $(CURDIR)/sound_xbox.cpp \
	$(addprefix $(FLYCAST)/core/hw/,aica/aica.cpp aica/aica_if.cpp aica/aica_mem.cpp aica/sgc_if.cpp aica/dsp.cpp \
	aica/dsp_interp.cpp arm7/arm7.cpp arm7/arm_mem.cpp)
NXDK_CXX = y
CFLAGS += -DHOST_WITH_SOUND
endif
# the combiner programs a lit model's draws take (video.c's lit runs), each
# made from its own program by shaders/lit.py
LIT_PROGRAMS = untextured decal decal_opaque modulate modulate_opaque decal_alpha decal_alpha_opaque \
	modulate_alpha modulate_alpha_opaque fog_polygon
SHADER_OBJS = $(addprefix $(CURDIR)/shaders/,vertex.inl untextured.inl decal.inl decal_opaque.inl modulate.inl \
	modulate_opaque.inl decal_alpha.inl decal_alpha_opaque.inl modulate_alpha.inl modulate_alpha_opaque.inl \
	fog_polygon.inl bloom_bright.inl bloom_blur.inl blit.inl lit.inl) \
	$(addprefix $(CURDIR)/shaders/lit_,$(addsuffix .inl,$(LIT_PROGRAMS)))
# main.map: the host's functions by address, for its profile's host lines
LDFLAGS += -map:$(CURDIR)/main.map
# the pads, through nxdk's SDL2 game controllers (host.c's raw_pad_read)
NXDK_SDL = y
NXDK_DIR ?= $(HOME)/refs/nxdk
# the game calls the host by cdecl on 4-byte aligned stacks, as config/pc.mk
# builds it, so each of the host's functions aligns its own
CFLAGS += -mstackrealign -Wall -I$(NXAUDIO)/include
# optimised: because nxdk passes no -O, and at -O0 the host's C, nxdk-audio's
# and pbkit's (which this build compiles with these flags) were too slow.
# frame pointers kept, for the fault report's walk. nxdk doesn't
# rebuild an object when flags change: delete the objects, pbkit's too
# (NXDK_DIR/lib/pbkit/*.obj), after changing these
CFLAGS += -O2 -fno-omit-frame-pointer
# Flycast's AICA, as xbox/host/pc's, but with FLYCAST_APU: the channels
# modelled without samples, played on the APU (aica_apu.c), the DSP not run.
# optimised: the ARM7 runs here a sample at a time. no stack probes: nxdk's
# __chkstk takes the thread's kernel stack for the only stack, and the host
# calls the AICA on its own (host.c's host stack), which it takes for an
# overflow, and halts
# ARM_STATIC: the sound driver's ARM code translated to C ahead
# (tools/arm2c.py, into build/sound/driver.inc: it holds the driver's code,
# so it stays under build/), which arm7.cpp runs in the interpreter's place
SOUND_BUILD = $(CURDIR)/../../../build/sound
CXXFLAGS += -O2 -mstack-probe-size=1048576 -mstackrealign -std=c++17 -w -DFLYCAST_AICA_ONLY -DFLYCAST_APU -DFLYCAST_DSP_INTERPRETER -DFLYCAST_DEFER_ONLOAD -DFLYCAST_PROFILE \
	-DARM_STATIC -I$(SOUND_BUILD) \
	-I$(FLYCAST)/shim -I$(FLYCAST)/core -I$(CURDIR) -I$(NXAUDIO)/include

# nxdk-audio's GP DSP program, which audio_core.c embeds, by dsp56300-asm
# (DSP56300_ASM, cargo install dsp56300-asm --version 1.0.0)
DSP56300_ASM ?= $(HOME)/refs/dsp-tools/bin/dsp56300-asm

include $(NXDK_DIR)/Makefile

$(NXAUDIO)/passthrough.out: $(NXAUDIO)/passthrough.a56
	$(DSP56300_ASM) -f lod -o $@ $< 2>/dev/null
$(NXAUDIO)/audio_core.obj: $(NXAUDIO)/passthrough.out

$(SOUND_BUILD)/driver.inc: $(CURDIR)/../../../tools/arm2c.py $(CURDIR)/driver_roots.txt \
		$(CURDIR)/../../../build/disc/SONICADV/MASARU.DRV
	mkdir -p $(SOUND_BUILD)
	python3 $(CURDIR)/../../../tools/arm2c.py $(CURDIR)/../../../build/disc/SONICADV/MASARU.DRV $@ --idle 0x194 \
		--roots $(CURDIR)/driver_roots.txt
$(FLYCAST)/core/hw/arm7/arm7.obj: $(SOUND_BUILD)/driver.inc $(FLYCAST)/core/hw/arm7/arm-new.h

# host.c by the SOUND it was built with
$(CURDIR)/.sound_$(SOUND):
	rm -f $(CURDIR)/.sound_*
	touch $@
$(CURDIR)/host.obj: $(CURDIR)/.sound_$(SOUND)

# each compiler exits 0 on an error and writes nothing: an empty program
# fails the build, with the compiler's message
# the vertex program, by hand in NV_vertex_program 1.1, for its constants
# (shaders/vertex.vp's head): vp20compiler takes it as it takes Cg's output
$(CURDIR)/shaders/vertex.inl: $(CURDIR)/shaders/vertex.vp $(VP20COMPILER)
	$(VP20COMPILER) $< > $@ 2>/dev/null; test -s $@ || { $(VP20COMPILER) $< > /dev/null; rm -f $@; false; }
# the bloom's, the same way
$(CURDIR)/shaders/blit.inl: $(CURDIR)/shaders/blit.vp $(VP20COMPILER)
	$(VP20COMPILER) $< > $@ 2>/dev/null; test -s $@ || { $(VP20COMPILER) $< > /dev/null; rm -f $@; false; }
# a lit model's, the same way
$(CURDIR)/shaders/lit.inl: $(CURDIR)/shaders/lit.vp $(VP20COMPILER)
	$(VP20COMPILER) $< > $@ 2>/dev/null; test -s $@ || { $(VP20COMPILER) $< > /dev/null; rm -f $@; false; }

# the combiner programs, in nvparse's text, as Cg wrote them from the C for
# graphics in each one's head, but for the fog's line, which Cg 3.1 gets
# wrong (shaders/NAME.ps): fp20compiler takes them as it takes Cg's output
# a lit model's (shaders/lit.py), from each one's own
$(CURDIR)/shaders/lit_%.ps: $(CURDIR)/shaders/%.ps $(CURDIR)/shaders/lit.py
	python3 $(CURDIR)/shaders/lit.py $< > $@
$(CURDIR)/shaders/%.inl: $(CURDIR)/shaders/%.ps $(FP20COMPILER)
	$(FP20COMPILER) $< > $@ 2>/dev/null; test -s $@ || { $(FP20COMPILER) $< > /dev/null; rm -f $@; false; }
