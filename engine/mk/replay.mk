# The PC backend's rules: Vulkan's headers and glslang, the shaders, the
# backend's objects and SA1's client, and the engine's tests of them

# The PC backend builds with the host's C++, against Vulkan's headers, and
# its shaders with glslang, each fetched at the release or commit
# tools/SHA256SUMS sums. A program that draws opens Vulkan's loader,
# libvulkan.so.1, as it runs, and links nothing of Vulkan's.
# glslang's release is Linux's, x86-64. Each keeps the receipt of its pin,
# as engine.mk says, and what make builds with either waits on its receipt
VULKAN_AT := https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/8864cdc896bbc2a9b6eb36b3218fc9ef57908d77
VULKAN := $(shell awk '$$2 ~ "^tools/vulkan/" { print $$2 }' tools/SHA256SUMS)
VULKAN_PIN := $(call pinned,tools/vulkan,$(VULKAN_AT))
VULKAN_FETCHED := tools/vulkan/fetched.txt
GLSLANG_AT := https://github.com/KhronosGroup/glslang/releases/download/16.5.0
GLSLANG_TAR := tools/glslang/glslang-16.5.0-linux-x86_64-release.tar.gz
GLSLANG := tools/glslang/bin/glslang
GLSLANG_PIN := $(call pinned,tools/glslang,$(GLSLANG_AT))
GLSLANG_FETCHED := tools/glslang/fetched.txt
$(VULKAN): $(call unfetched,tools/vulkan,$(VULKAN_PIN))
	@$(call status,fetch,$@)
	$(Q)rm -f $(VULKAN_FETCHED) && mkdir -p $(@D) && $(call fetch,$(VULKAN_AT)/$(@:tools/vulkan/%=%),$@)
# Once every header is in place, the receipt's recipe removes each other
# file under tools/vulkan/, one an earlier pin listed
$(VULKAN_FETCHED): $(VULKAN)
	$(Q)mkdir -p $(@D) && rm -f $(filter-out $(VULKAN) $@,$(shell [ ! -d tools/vulkan ] || find tools/vulkan -type f))
	$(Q)$(call receipt,tools/vulkan,$(VULKAN_PIN))
$(GLSLANG_TAR): $(call unfetched,tools/glslang,$(GLSLANG_PIN))
	@$(call status,fetch,$@)
	$(Q)rm -f $(GLSLANG_FETCHED) && mkdir -p $(@D) && $(call fetch,$(GLSLANG_AT)/$(@F),$@)
$(GLSLANG): $(GLSLANG_TAR)
	$(Q)rm -f $(GLSLANG_FETCHED) && tar -xzf $< -C tools/glslang bin/glslang && touch $@
$(GLSLANG_FETCHED): $(GLSLANG)
	$(Q)$(call receipt,tools/glslang,$(GLSLANG_PIN))

# Each shader becomes a header of its SPIR-V words, draw.vert's draw_vert,
# which the raster compiles in
REPLAY_SHADERS := $(patsubst engine/pc/shaders/%,build/engine/replay/%.h,$(wildcard engine/pc/shaders/*))
build/engine/replay/%.h: engine/pc/shaders/% $(GLSLANG_FETCHED)
	$(Q)mkdir -p $(@D)
	$(Q)$(GLSLANG) --quiet -V --target-env vulkan1.0 --vn $(subst .,_,$*) -o $@ $<
# make before 4.4 takes a file that only pattern rules name for an
# intermediate one, and removes it once it has built what reads it. The
# backend's objects read these by a pattern rule, so this line names each
$(REPLAY_SHADERS):

# The PC backend: every program of the engine's that draws links these
# beside the runtime's core. The window and its pads are SDL3's where the
# host has it, and a build without it runs headless alone. pkg-config gives
# SDL3's flags as the host has them, which no file's date tracks, so
# build/engine/replay/sdl3.txt records them, and each object and program
# that compiles under them depends on it. Each depends on its compiler's
# record too
SDL3_LIBS := $(shell pkg-config --libs sdl3 2>/dev/null)
SDL3_CFLAGS := $(shell pkg-config --cflags sdl3 2>/dev/null)
PC_WINDOW := $(if $(SDL3_LIBS),window,window_none)
PC_SOURCES := backend storage scene world raster disc pvr ninja png sound dsp $(PC_WINDOW) \
	$(if $(SDL3_LIBS),input)
PC_OBJ := $(PC_SOURCES:%=build/engine/replay/%.o) $(ENGINE_RUNTIME_OBJ)
PC_LIBS := $(SDL3_LIBS) -ldl
REPLAY_HOST := build/engine/replay/sdl3.txt
$(call record,$(REPLAY_HOST),$(SDL3_CFLAGS) $(SDL3_LIBS))
REPLAY_DEPS := $(wildcard engine/pc/*.h) $(wildcard engine/include/*.h) engine/sa1/sa_sa1.h \
	engine/sa1/sa_capture.h engine/sa1/sa_client.h engine/sa1/client.h engine/sa1/basic.h engine/sa1/rules.h \
	engine/runtime/runtime.h engine/engine.mk engine/mk/replay.mk $(ENGINE_CXX)
build/engine/replay/%.o: engine/pc/%.cpp $(REPLAY_DEPS) $(VULKAN) $(VULKAN_FETCHED) $(REPLAY_SHADERS)
	$(Q)mkdir -p $(@D)
	$(Q)$(CXX) $(ENGINE_CXXFLAGS) -O2 -I$(ENGINE_INCLUDE) -Iengine/sa1 -Iengine/runtime -Itools/vulkan/include \
		-Ibuild/engine/replay $(if $(filter window input,$*),$(SDL3_CFLAGS)) -c -o $@ $<
build/engine/replay/window.o build/engine/replay/input.o: $(REPLAY_HOST)
# SA1's client of the backend, engine/sa1/'s C++: a capture's records, the
# meshes they hold and the textures they name from the disc, given through
# the contracts. The play program links it beside PC_OBJ
SA1_CLIENT_OBJ := build/engine/sa1/client.o build/engine/sa1/basic.o
# Each directory's C++ builds into a directory of objects of its own, so a
# source of one may take the stem of a source of another's
build/engine/sa1/%.o: engine/sa1/%.cpp $(REPLAY_DEPS)
	$(Q)mkdir -p $(@D)
	$(Q)$(CXX) $(ENGINE_CXXFLAGS) -O2 -I$(ENGINE_INCLUDE) -Iengine/sa1 -Iengine/pc -c -o $@ $<

# engine-test runs the PC backend's readers of the disc on bytes of their
# own: PRS, a PVR's layouts and a Ninja model, and a disc's roots and the
# textures' index over files the test writes under TMPDIR and removes. They
# need neither Vulkan nor the disc
REPLAY_TESTED := engine/pc/disc.cpp engine/pc/pvr.cpp engine/pc/ninja.cpp engine/pc/scene.cpp engine/sa1/basic.cpp
build/engine/tests/replay: engine/tests/replay.cpp $(REPLAY_TESTED) $(REPLAY_DEPS)
	$(Q)mkdir -p $(@D)
	$(Q)$(CXX) $(ENGINE_CXXFLAGS) -I$(ENGINE_INCLUDE) -Iengine/sa1 -Iengine/pc -o $@ $< $(REPLAY_TESTED)
# and the PC backend's resources and pictures, with SA1's client feeding it,
# over a raster of the test's own, which needs no device. Its client takes
# textures from two files the test writes under TMPDIR and removes
PICTURES_TESTED := engine/pc/backend.cpp engine/pc/storage.cpp engine/sa1/client.cpp engine/sa1/basic.cpp \
	engine/pc/scene.cpp engine/pc/world.cpp engine/pc/ninja.cpp engine/pc/pvr.cpp engine/pc/disc.cpp engine/pc/png.cpp \
	engine/pc/sound.cpp engine/pc/dsp.cpp engine/runtime/engine.cpp
build/engine/tests/pictures: engine/tests/pictures.cpp $(PICTURES_TESTED) $(REPLAY_DEPS)
	$(Q)mkdir -p $(@D)
	$(Q)$(CXX) $(ENGINE_CXXFLAGS) -I$(ENGINE_INCLUDE) -Iengine/sa1 -Iengine/pc -Iengine/runtime -o $@ $< \
		$(PICTURES_TESTED)
# and the raster itself, over a device of the test's own: a library named as
# Vulkan's loader is, which the test's run finds ahead of any other, on Linux
# by LD_LIBRARY_PATH and on a Mac by DYLD_LIBRARY_PATH. It needs Vulkan's
# headers and no Vulkan. The device reads no shader, so here each shader is
# one word, and the test needs no glslang
DEVICE_LOADER := build/engine/tests/vulkan/libvulkan.so.1
DEVICE_WORDS := $(patsubst engine/pc/shaders/%,build/engine/tests/words/%.h,$(wildcard engine/pc/shaders/*))
DEVICE_RUN := LD_LIBRARY_PATH=$(dir $(DEVICE_LOADER))$${LD_LIBRARY_PATH:+:$$LD_LIBRARY_PATH} \
	DYLD_LIBRARY_PATH=$(dir $(DEVICE_LOADER))
build/engine/tests/words/%.h: engine/mk/replay.mk
	$(Q)mkdir -p $(@D)
	$(Q)echo 'const uint32_t $(subst .,_,$*)[] = {0};' > $@
$(DEVICE_LOADER): engine/tests/vulkan.cpp engine/tests/vulkan.h $(VULKAN) $(VULKAN_FETCHED) engine/engine.mk \
		engine/mk/replay.mk $(ENGINE_CXX)
	$(Q)mkdir -p $(@D)
	$(Q)$(CXX) $(ENGINE_CXXFLAGS) -fPIC -shared -Itools/vulkan/include -o $@ $<
build/engine/tests/device: engine/tests/device.cpp engine/pc/raster.cpp engine/tests/vulkan.h $(DEVICE_LOADER) \
		$(DEVICE_WORDS) $(REPLAY_DEPS) $(VULKAN) $(VULKAN_FETCHED)
	$(Q)mkdir -p $(@D)
	$(Q)$(CXX) $(ENGINE_CXXFLAGS) -I$(ENGINE_INCLUDE) -Iengine/pc -Itools/vulkan/include -Ibuild/engine/tests/words \
		-o $@ $< engine/pc/raster.cpp -ldl
# and the blend list's resolve on the host: the shader's own text, less the
# line that names GLSL's version, which the test reads as C++ over GLSL's
# types as far as the shader uses them. It needs no device and no glslang
build/engine/tests/glsl/resolve.h: engine/pc/shaders/resolve.comp engine/mk/replay.mk
	$(Q)mkdir -p $(@D)
	$(Q)sed '/^#version/d' $< > $@
build/engine/tests/blend: engine/tests/blend.cpp build/engine/tests/glsl/resolve.h $(REPLAY_DEPS)
	$(Q)mkdir -p $(@D)
	$(Q)$(CXX) $(ENGINE_CXXFLAGS) -I$(ENGINE_INCLUDE) -Ibuild/engine/tests/glsl -o $@ $<
# and the PC backend's sound, beside the game's own ADX decoder, which the
# slice's clang compiles as the slice would
build/engine/tests/adx_dcd2.o: sdk/sofdec/adx_dcd2.c $(NATIVE_PRELUDE) engine/mk/replay.mk $(ENGINE_CLANG)
	$(Q)mkdir -p $(@D)
	$(Q)$(NATIVE_CC) -std=gnu89 -O1 -w -include engine/native/prelude.h -Isdk/include -c -o $@ $<
build/engine/tests/sound: engine/tests/sound.cpp engine/pc/sound.cpp engine/pc/dsp.cpp build/engine/tests/adx_dcd2.o \
		$(REPLAY_DEPS)
	$(Q)mkdir -p $(@D)
	$(Q)$(CXX) $(ENGINE_CXXFLAGS) -I$(ENGINE_INCLUDE) -Iengine/pc -o $@ $< engine/pc/sound.cpp engine/pc/dsp.cpp \
		build/engine/tests/adx_dcd2.o
# and the pads, over SDL's virtual gamepads, which need no device or display,
# with the game's vibration through the slice's own calls: where SDL3 is
build/engine/tests/vibration.o: engine/native/vibration.c engine/native/native.h engine/mk/replay.mk $(ENGINE_CC)
	$(Q)mkdir -p $(@D)
	$(Q)$(CC) -std=gnu17 -Wall -Wextra -Werror -Iengine/native -c -o $@ $<
build/engine/tests/input: engine/tests/input.cpp engine/pc/input.cpp build/engine/tests/vibration.o $(REPLAY_DEPS) \
		$(REPLAY_HOST)
	$(Q)mkdir -p $(@D)
	$(Q)$(CXX) $(ENGINE_CXXFLAGS) $(SDL3_CFLAGS) -I$(ENGINE_INCLUDE) -Iengine/pc -o $@ \
		$< engine/pc/input.cpp build/engine/tests/vibration.o $(SDL3_LIBS)
# and compiles the capture's header as C17, and for SH-4 where SH_ELF's GCC
# is in place, so its asserts and sa_sa1.h's hold each record's layout there
# too. The SH-4 check keeps a stamp of its own, as the C ABI's does, so it
# runs once the GCC is in place where the C17 stamp stands
build/engine/abi/capture.ok build/engine/abi/capture-sh4.ok: engine/sa1/sa_capture.h engine/sa1/sa_sa1.h \
		engine/include/sa_engine.h engine/engine.mk engine/mk/replay.mk
build/engine/abi/capture.ok: $(ENGINE_CC)
build/engine/abi/capture-sh4.ok: $(ENGINE_SH4_CC)
build/engine/abi/capture.ok:
	$(Q)mkdir -p $(@D)
	$(Q)$(CC) $(ENGINE_CFLAGS) -I$(ENGINE_INCLUDE) -Iengine/sa1 -x c -fsyntax-only $<
	$(Q)touch $@
build/engine/abi/capture-sh4.ok:
	$(Q)mkdir -p $(@D)
	$(Q)$(SH_ELF)/bin/sh-elf-gcc -ml -m4-single-only -ffreestanding $(ENGINE_CFLAGS) -I$(ENGINE_INCLUDE) -Iengine/sa1 \
		-x c -fsyntax-only $<
	$(Q)touch $@
engine-test: engine-test-replay
engine-test-replay: build/engine/tests/replay build/engine/tests/pictures build/engine/tests/device \
		build/engine/tests/blend build/engine/tests/sound build/engine/abi/capture.ok \
		$(if $(ENGINE_SH4),build/engine/abi/capture-sh4.ok) \
		$(if $(SDL3_LIBS),build/engine/tests/input)
	$(Q)build/engine/tests/replay
	$(Q)build/engine/tests/pictures
	$(Q)$(DEVICE_RUN) build/engine/tests/device
	$(Q)build/engine/tests/blend
	$(Q)build/engine/tests/sound
	$(Q)$(if $(SDL3_LIBS),build/engine/tests/input)
	@$(call status,pc,the disc's readers$(comma) a picture's lifetimes$(comma) the raster as its device fails$(comma) the blend list's resolve$(comma) sound$(comma)$(if $(SDL3_LIBS), the pads$(comma)) and the capture's layout in C17$(if $(ENGINE_SH4),$(comma) C++17 and SH-4,$(comma) C++17))
.PHONY: engine-test-replay
