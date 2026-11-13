# The engine's rules: the C ABI's check, the runtime's core, and a fragment
# for each of play math, the Dreamcast's frame owner, the PC backend and the
# native slice. The Makefile reads this file for ENGINE_GOALS and for
# make pc alone, so make all, check, dc, dc-disc and dc-check read no make
# file under engine/. A run that names one of DC_GOALS beside a goal this
# file is read for reads both sets of rules. So that it builds what make dc
# alone builds, no rule here or in a fragment has a target under build/dc/
# or sets GCCFLAGS, DC_ADDED or DC_REPLACED, as a test holds

# The engine's scripts need Python 3.9: marks.py joins two dicts with |,
# census.py calls str.removeprefix, and boundary.py functools.cache
ifneq ($(shell PYTHONPYCACHEPREFIX=$(PYTHONPYCACHEPREFIX) python3 -c 'import sys; print(sys.version_info >= (3, 9))'),True)
$(error the engine's goals need Python 3.9 or later, as python3)
endif

# The engine's host code builds with the host's compilers: CC for C, and
# CXX, make's g++ unless the environment names another, for C++. The flags
# hold what a compiler can of ENGINE.md#c-abi and of the C++ the
# runtime may use
ENGINE_CFLAGS   := -std=c17 -Wall -Wextra -Werror -pedantic
ENGINE_CXXFLAGS := -std=c++17 -fno-exceptions -fno-rtti -Wall -Wextra -Werror -pedantic
ENGINE_INCLUDE  := engine/include
$(call check_host,$(firstword $(CC)) $(firstword $(CXX)))
# CC comes from the command line, the environment or make's own name for
# it, and CXX from the environment or make's own: the Makefile's SETTINGS
# take no CXX on the command line. The host gives each its release, so no
# file's date tracks either. A record holds each compiler's command and the version it
# gives, as the Makefile's record of CC holds its command, and each object or
# program a compiler builds depends on that compiler's record
ENGINE_CC  := build/engine/cc.txt
ENGINE_CXX := build/engine/c++.txt
$(call record,$(ENGINE_CC),$(CC) $(shell $(CC) --version 2> /dev/null))
$(call record,$(ENGINE_CXX),$(CXX) $(shell $(CXX) --version 2> /dev/null))
# SH_ELF's GCC, where it is in place, compiles the two checks of SH-4's
# layout, and a record holds it the same way: another SH_ELF, or another
# GCC there, runs each check again
ENGINE_SH4 := $(if $(wildcard $(SH_ELF)/bin/sh-elf-gcc),build/engine/abi/sh4.ok)
ENGINE_SH4_CC := build/engine/sh4-cc.txt
ifneq ($(ENGINE_SH4),)
$(call record,$(ENGINE_SH4_CC),$(SH_ELF)/bin/sh-elf-gcc $(shell $(SH_ELF)/bin/sh-elf-gcc --version 2> /dev/null))
endif

# engine-test compiles engine/tests/abi.c as C and as C++ and runs each.
# Where SH_ELF's GCC is in place, it compiles abi.c for SH-4 as well, and
# the header's asserts hold each record's layout on a 32-bit target. A
# fragment adds its own tests to engine-test
ENGINE_ABI_DEPS := engine/tests/abi.c engine/include/sa_engine.h engine/engine.mk
build/engine/abi/c: $(ENGINE_ABI_DEPS) $(ENGINE_CC)
	$(Q)mkdir -p $(@D)
	$(Q)$(CC) $(ENGINE_CFLAGS) -I$(ENGINE_INCLUDE) -o $@ $<
build/engine/abi/c++: $(ENGINE_ABI_DEPS) $(ENGINE_CXX)
	$(Q)mkdir -p $(@D)
	$(Q)$(CXX) $(ENGINE_CXXFLAGS) -I$(ENGINE_INCLUDE) -x c++ -o $@ $<
build/engine/abi/sh4.ok: $(ENGINE_ABI_DEPS) $(ENGINE_SH4_CC)
	$(Q)mkdir -p $(@D)
	$(Q)$(SH_ELF)/bin/sh-elf-gcc -ml -m4-single-only -ffreestanding $(ENGINE_CFLAGS) -I$(ENGINE_INCLUDE) \
		-fsyntax-only $<
	$(Q)touch $@
engine-test: build/engine/abi/c build/engine/abi/c++ $(ENGINE_SH4) build/engine/tests/runtime
	$(Q)build/engine/abi/c && build/engine/abi/c++
	@$(call status,abi,C17 and C++17$(if $(ENGINE_SH4),$(comma) and SH-4's layout,$(comma) but no SH-4 GCC at $(SH_ELF)))
	$(Q)build/engine/tests/runtime
	@$(call status,engine,the runtime's core)

# The game's side of the ABI's mode. engine-test builds the game's frame end,
# src/system/pace.c, for the host, under NONMATCHING and with the Dreamcast
# engine's sa_dc.h, and links it with engine/tests/pace.c, an engine of the
# test's own, in place of the game and a backend. The host's headers come
# ahead of the SDK's, which holds a C library's own
PACE_TESTED := engine/tests/pace.c src/system/pace.c
build/engine/tests/pace: $(PACE_TESTED) src/system/port.h src/system/frame.h src/progress/demo.h \
		src/progress/recorder.h src/stage/stage.h sdk/include/ninja.h sdk/include/njtypes.h sdk/include/sg_xpt.h \
		engine/dc/dreamcast.h engine/dc/sa_dc.h $(ENGINE_INCLUDE)/sa_engine.h engine/engine.mk $(ENGINE_CC)
	$(Q)mkdir -p $(@D)
	$(Q)$(CC) -std=gnu17 -Wall -Wextra -Werror -DNONMATCHING -I$(ENGINE_INCLUDE) -Iengine/dc -Isrc \
		-idirafter sdk/include -o $@ $(PACE_TESTED)
engine-test: engine-pace
engine-pace: build/engine/tests/pace
	$(Q)build/engine/tests/pace
	@$(call status,pace,the frame's end under an engine that applies$(comma) holds or refuses the mode)
.PHONY: engine-pace

# The runtime's core, engine/runtime/, which every program of the engine's
# links beside one backend. Its test links it beside a backend of its own
ENGINE_RUNTIME_OBJ := build/engine/runtime/engine.o
build/engine/runtime/%.o: engine/runtime/%.cpp engine/runtime/runtime.h engine/include/sa_engine.h engine/engine.mk \
		$(ENGINE_CXX)
	$(Q)mkdir -p $(@D)
	$(Q)$(CXX) $(ENGINE_CXXFLAGS) -I$(ENGINE_INCLUDE) -c -o $@ $<
build/engine/tests/runtime: engine/tests/runtime.cpp $(ENGINE_RUNTIME_OBJ)
	$(Q)mkdir -p $(@D)
	$(Q)$(CXX) $(ENGINE_CXXFLAGS) -I$(ENGINE_INCLUDE) -Iengine/runtime -o $@ $^

# Play math's fragment: every program that plays on a host links the
# library. Make reads it ahead of the other fragments, whose rules take its
# objects' names as make reads them
include engine/mk/play_math.mk

# The native slice's prelude and the headers it includes, which each host
# compile of the game's C reads ahead of its source
NATIVE_PRELUDE := engine/native/prelude.h engine/native/intrinsics.h engine/native/intrinsics_machine.h
# The slice's compiler, clang, which two of the engine's tests compile with
# too. A record holds its command and the version it gives, as ENGINE_CC's
# holds CC's, and each of those tests depends on it
NATIVE_CC := clang
ENGINE_CLANG := build/engine/clang.txt
$(call record,$(ENGINE_CLANG),$(NATIVE_CC) $(shell $(NATIVE_CC) --version 2> /dev/null))

# A download of the engine's keeps a receipt beside it, DIR/fetched.txt, as
# SH4ZAM's files keep dc.mk's: its pin, written once every file of the
# download is in place. $(call pinned,DIR,AT) is the pin as make reads it
# now: AT, the address the files come from, then the lines tools/SHA256SUMS
# gives the files under DIR. $(call unfetched,DIR,PIN) is FORCE where DIR's
# receipt holds another text than PIN, or DIR has none. The rule that
# fetches a file of DIR's takes it as a prerequisite, so a changed address
# or sum fetches every file again and holds each to its line, and a file
# another pin left never passes for this one's. Each fetch removes the
# receipt first, so a run of fetches cut short leaves none.
# $(call receipt,DIR,PIN) is the command that writes it. It removes the
# receipt, holds each file tools/SHA256SUMS lists under DIR to its line, and
# writes the receipt where each matches. So a receipt older than a file of
# its download, as a file changed in place or a cache put back leaves it, is
# written again only over the pin's own bytes. Over any other there is none,
# and the next run fetches the download again. What a download builds
# depends on the receipt
pinned    = $(strip $2 $(shell awk '$$2 ~ "^$1/"' tools/SHA256SUMS))
unfetched = $(if $(subst $(strip $(file <$1/fetched.txt)),,$2)$(subst $2,,$(strip $(file <$1/fetched.txt))),FORCE)
receipt   = rm -f $1/fetched.txt && sums=$$(awk '$$2 ~ "^$1/"' tools/SHA256SUMS) && \
	{ [ -z "$$sums" ] || echo "$$sums" | sha256sum -c --quiet -; } && printf '%s\n' $(call quoted,$2) > $1/fetched.txt

include engine/mk/owner.mk engine/mk/drawers.mk engine/mk/replay.mk engine/mk/native.mk
