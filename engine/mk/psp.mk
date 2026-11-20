# The PSP slice's rules: the game's C built for the PSP, and the programs its
# loader reads

# The slice compiles what the native slice compiles (native.mk), by its
# lists and through make dc's float pass, with the PSP's GCC. The PSP's
# pointers hold 4 bytes, as the SH-4's do, so no pass marks one: marks.py
# writes the slice a narrow copy of the game's C, which names the console's
# RAM through engine/psp/psp.h, and gives each unit the hooks and the float
# spellings alone. The FPU has no fused multiply and add, so no product
# fuses, where the console's FMAC rounds once
#
# The PC keeps every program in its image. The PSP has no room for them, so
# each links where the memory contract places it, as make dc's do, and the
# loader's read of its file brings it in (engine/tools/psplink.py). The
# menus stay the program's own, as the native slice has them
#
# PSPDEV, from the environment, names the toolchain, pspdev's, ~/pspdev by
# default. PSP_OPT is the game's optimization
PSPDEV ?= $(HOME)/pspdev
PSP_OPT ?= -O2
# PSP_MATH, from the environment, is play math's build (engine/mk/play_math.mk):
# fpu, four operations by the PSP's own FPU, which the PSP has the speed for,
# or fast, every host's bits
PSP_MATH ?= fpu
# PSP_EXPLORE=1, from the environment, builds past a name stubs.txt has no
# row for, as NATIVE_EXPLORE does
PSP_EXPLORE ?=
PSP_EXPLORING := $(if $(filter 1,$(PSP_EXPLORE)),--explore)
PSP     := $(or $(PSP_OUT),build/engine/psp)
PSP_CC  := $(PSPDEV)/bin/psp-gcc
PSP_CXX := $(PSPDEV)/bin/psp-g++
PSP_SDK := $(PSPDEV)/psp/sdk
PSP_GOALS := $(filter psp $(PSP)/%,$(MAKECMDGOALS))
ifneq ($(PSP_GOALS),)
ifeq ($(wildcard $(PSP_CC)),)
$(error $(PSP_CC): no PSP compiler there; PSPDEV, from the environment, names pspdev's toolchain)
endif
endif
PSP_MK := engine/engine.mk engine/mk/play_math.mk engine/mk/native.mk engine/mk/psp.mk
PSP_NEEDS := $(PSP_MK) $(PSP)/request.txt
PSP_GCC_INCLUDE := $(shell $(PSP_CC) -print-file-name=include 2> /dev/null)
ifneq ($(PSP_GOALS),)
$(call record,$(PSP)/request.txt,opt $(PSP_OPT) math $(PSP_MATH) gcc $(shell $(PSP_CC) --version 2> /dev/null))
endif

# The narrow copy of the game's C, as native.mk keeps the wide one
PSP_TREE := $(PSP)/src
ifneq ($(PSP_GOALS),)
$(call record,$(PSP)/sources.txt,$(sort $(NATIVE_SOURCES)))
$(PSP)/marked.mk: engine/tools/marks.py engine/dc/dreamcast.h $(NATIVE_SOURCES) $(PSP)/sources.txt $(PSP_NEEDS)
	@$(call status,marks,$(PSP_TREE))
	$(Q)python3 engine/tools/marks.py tree --narrow $(PSP_TREE)
	$(Q)find $(PSP_TREE) -name '*.[ch]' | while read -r copy; do \
		[ -e "$${copy#$(PSP_TREE)/}" ] || rm "$$copy" || exit 1; done
	$(Q)echo '# $(PSP_TREE) is current' > $@
include $(PSP)/marked.mk
endif

# How the slice compiles the game: as native.mk does, but for the passes'
# words. An event's code runs once, and its image has the least room to
# spare, so it compiles for size. A function takes a section of its own, and
# so does each datum of an image's content, in the content's order, as make
# dc's take them (config/gcc.mk): a program's link then leaves out what
# nothing of the program reaches. summary.c's entries take the section the
# memory contract names for them
PSP_CFLAGS = -std=gnu89 -ffreestanding -nostdinc -isystem $(PSP_GCC_INCLUDE) -DNONMATCHING \
	-include engine/psp/prelude.h $(PSP_OPT) -G0 -fcommon -fno-strict-aliasing -fno-delete-null-pointer-checks \
	-fwrapv -ffp-contract=off -fno-builtin -fms-extensions -frounding-math -ffunction-sections $(PSP_SECTIONS) -w
$(PSP)/obj/events/%.o: PSP_OPT := -Os
$(PSP)/obj/gen/%.o: PSP_SECTIONS := -fdata-sections -fno-toplevel-reorder
$(PSP)/obj/modules/summary/summary.o: PSP_SECTIONS := -fdata-sections
psp_pass_flags = $(PSP_CFLAGS) $(if $(filter -fpu=single -double=float,$1),-Ddouble=float -Dfrexp=native_frexp)
# $(call psp_pass,INCLUDES,RELEASE,SHC'S FLAGS) compiles $< to $@ through the
# float pass and marks.py's narrow one
psp_pass = $(PSP_CC) $(call psp_pass_flags,$3) $1 -E -MD -MP -MT $@ -MF $(@:.o=.d) -o $(@:.o=.i) $< && \
	python3 tools/floats.py --shc $2 $(filter -fpu=single -double=float -aggressive=2,$3) $(@:.o=.i) && \
	python3 engine/tools/marks.py narrow $(@:.o=.i) && \
	$(PSP_CC) $(call psp_pass_flags,$3) -c -o $@ $(@:.o=.i)
PSP_GAME_INC := sdk/include $(addprefix $(PSP_TREE)/,src modules events) build/gen/src build/gen/modules \
	build/gen/events build/gen/sdk
PSP_GEN_INC  := sdk/include $(addprefix $(PSP_TREE)/,src modules)

# The slice's objects, by native.mk's lists. The runner, the names and the
# loads are the PSP's own, in engine/psp/
PSP_SHIM_C := $(filter-out engine/native/runner.c engine/native/symbols.c engine/native/loads.c,$(NATIVE_SHIM_C)) \
	$(wildcard engine/psp/*.c)
PSP_OBJ := $(NATIVE_GAME_C:%.c=$(PSP)/obj/%.o) $(NATIVE_GEN_C:build/%.c=$(PSP)/obj/%.o) \
	$(NATIVE_RUNTIME_C:%.c=$(PSP)/obj/%.o) $(PSP_SHIM_C:%.c=$(PSP)/obj/%.o) $(NATIVE_SA1_C:%.c=$(PSP)/obj/%.o) \
	$(NATIVE_SDK_C:%.c=$(PSP)/obj/%.o) $(call play_math_$(PSP_MATH),$(PSP)/obj/engine/numeric) $(PSP)/asm.o $(PSP)/disc.o
$(PSP)/obj/src/main.o: private PSP_DEFS := -Dmain=sa1_main
$(PSP)/obj/src/draw/model.o: private PSP_DEFS := -Iengine -I$(ENGINE_INCLUDE) -Iengine/sa1 -I.
$(PSP)/obj/src/system/pace.o: private PSP_DEFS := -I$(ENGINE_INCLUDE)
$(PSP)/obj/src/system/present.o: private PSP_DEFS := -I$(ENGINE_INCLUDE) -I.
$(PSP)/obj/sdk/ninja/debug.o: private PSP_DEFS := -Ibuild/gen/sdk
$(PSP)/obj/sdk/ninja/njSystem.o: private PSP_DEFS := -DnjInitSystem=ninja_init_system \
	-DnjChangeSystem=ninja_change_system -DnjInitVertexBuffer=ninja_init_vertex_buffer \
	-DnjWaitVSync=ninja_wait_vsync -DnjSetVSyncFunction=ninja_set_vsync_function -DnjExitSystem=ninja_exit_system
$(PSP)/obj/sdk/ninja/njTexture.o: private PSP_DEFS := -DnjLoadTexture=ninja_load_texture \
	-DnjInitTexture=ninja_init_texture
$(PSP)/obj/sdk/ninja/kmglobal_kamui_lib.o: private PSP_DEFS := -DkmSetBackGroundPlane=kamui_set_back_ground_plane
$(PSP)/obj/sdk/ninja/njDrawPolygon2D.o $(PSP)/obj/sdk/ninja/njDrawLine2D.o \
$(PSP)/obj/sdk/ninja/njDrawLine3D.o $(PSP)/obj/sdk/ninja/njModelE.o: private PSP_DEFS := -UNONMATCHING
$(foreach s,$(NATIVE_DOUBLE_FLOAT),$(eval $(PSP)/obj/$s.o: private SDK += -double=float))
PSP_PASSES := engine/psp/prelude.h engine/psp/psp.h $(NATIVE_PRELUDE) config/compilers.mk $(FLOATS) \
	engine/tools/marks.py $(PSP_NEEDS)
$(NATIVE_GAME_C:%.c=$(PSP)/obj/%.o): $(PSP)/obj/%.o: $(PSP_TREE)/%.c $(PSP_PASSES)
	@$(call status,psp,$*.c)
	$(Q)mkdir -p $(@D)
	$(Q)$(call psp_pass,$(PSP_DEFS) $(addprefix -I,$(PSP_GAME_INC)),$(if $(call native_r10,$*.c),r10,r32),$(if \
		$(call native_r10,$*.c),$(SDK),$(GAME)))
$(NATIVE_RUNTIME_C:%.c=$(PSP)/obj/%.o): $(PSP)/obj/%.o: %.c $(PSP_PASSES)
	@$(call status,psp,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(call psp_pass,-Isdk/include -Isdk,r32,$(if $(filter $(NATIVE_WIDE_C),$<),,-fpu=single))
$(NATIVE_SDK_C:%.c=$(PSP)/obj/%.o): $(PSP)/obj/%.o: %.c $(PSP_PASSES)
	@$(call status,psp,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(call psp_pass,$(PSP_DEFS) -Isdk/include -Isdk,$(if $(call native_r32,$<),r32,r10),$(if \
		$(call native_r32,$<),$(GAME),$(SDK)))
$(PSP)/obj/gen/%.o: build/gen/%.c $(PSP_PASSES)
	@$(call status,psp,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(call psp_pass,$(addprefix -I,$(PSP_GEN_INC)),r32,$(GAME))

# The runner and the shim are the host's C, against the PSP's headers. A
# word's marks, which a wider host's objects take, mean nothing here, and
# mips, GCC's own name for the processor, is a name of the adapter's. The
# PSP's uint32_t is a long, which the host's C prints as an int
PSP_HOST_CFLAGS := -std=gnu17 -O2 -G0 -fno-strict-aliasing -ffp-contract=off -frounding-math -DNATIVE_DOMAIN=0U \
	-D__ptr32= -D__uptr= -Umips -Wno-format -Iengine/psp -Iengine/native -Iengine/sa1 -I$(ENGINE_INCLUDE) $(PLAY_MATH_INCLUDE) \
	-I$(PSP_SDK)/include
$(PSP)/obj/engine/native/%.o: engine/native/%.c $(PSP_NEEDS)
	@$(call status,psp,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(PSP_CC) $(PSP_HOST_CFLAGS) -Wall -Wextra -Werror -MD -MP -c -o $@ $<
$(PSP)/obj/engine/psp/%.o: engine/psp/%.c $(PSP_NEEDS)
	@$(call status,psp,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(PSP_CC) $(PSP_HOST_CFLAGS) -Wall -Wextra -Werror -MD -MP -c -o $@ $<
# The adapter reads Ninja's models as the game lays them out. The C
# library's string.h declares a strupr of its own, and ninja.h Ninja's:
# engine/psp/libc.h reads the header first, under another name for it
PSP_SA1_CFLAGS := $(PSP_HOST_CFLAGS) -fms-extensions -DNONMATCHING -include engine/psp/libc.h \
	-include engine/psp/prelude.h -idirafter sdk/include -Isdk -Isrc -I$(dir $(PL_MPEG)) -DNATIVE_BUILD_IMAGES='"."'
$(PSP)/obj/engine/sa1/%.o: engine/sa1/%.c $(PSP_NEEDS)
	@$(call status,psp,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(PSP_CC) $(PSP_SA1_CFLAGS) $(PSP_SINGLE) -Wall -Wextra -MD -MP -c -o $@ $<
# The adapter's records take a float where the host's take a double
# (engine/psp/single.h), a constant among them
$(PSP)/obj/engine/sa1/records.o: PSP_SINGLE := -include engine/psp/single.h -fsingle-precision-constant
$(PSP)/obj/engine/sa1/movie.o: $(PL_MPEG_FETCHED)
$(eval $(call play_math,$(PSP)/obj/engine/numeric,$$(PSP_CC) -std=c17 -O2 -G0 $$(PLAY_MATH_INCLUDE),$(PSP_NEEDS),psp))
$(PSP)/asm.c: engine/tools/asm2c.py $(foreach f,$(NATIVE_ASM),$(firstword $(subst :, ,$f))) $(PSP_MK)
	@$(call status,asm2c,$@)
	$(Q)mkdir -p $(@D)
	$(Q)python3 engine/tools/asm2c.py $@ $(NATIVE_ASM)
$(PSP)/asm.o: $(PSP)/asm.c $(PSP_NEEDS)
	@$(call status,psp,$<)
	$(Q)$(PSP_CC) $(PSP_HOST_CFLAGS) -O1 -w -MD -MP -c -o $@ $<
# The build's disc, as native.mk's table of it, which gdfs.c holds a disc to
$(PSP)/disc.c: SHA256SUMS $(PSP_MK)
	@$(call status,sums,$@)
	$(Q)mkdir -p $(@D)
	$(Q)awk 'BEGIN { print "#include \"native.h\"\nconst struct native_disc_sum native_disc_sums[] = {" } \
		sub(/^orig\//, "", $$2) { printf "    {\"%s\", \"%s\"},\n", $$2, $$1; rows++ } \
		END { print "};\nconst int native_disc_sum_count = sizeof native_disc_sums / sizeof *native_disc_sums;"; \
			exit !rows }' $< > $@.tmp || { echo '$<: no line names a file of orig/'; exit 1; }
	$(Q)mv $@.tmp $@
$(PSP)/disc.o: $(PSP)/disc.c engine/native/native.h $(PSP_NEEDS)
	$(Q)$(PSP_CC) $(PSP_HOST_CFLAGS) -c -o $@ $<
PSP_READ := $(filter-out $(call play_math_$(PSP_MATH),$(PSP)/obj/engine/numeric),$(filter $(PSP)/obj/%,$(PSP_OBJ)))
$(PSP_READ): $(PSP)/obj/%.o: $(PSP)/obj/%.d
$(PSP_READ:.o=.d) $(PSP)/asm.d: ;
-include $(PSP_READ:.o=.d) $(PSP)/asm.d
ifneq ($(PSP_GOALS),)
$(filter $(PSP_OBJ),$(BODY_C:%.c=$(PSP)/obj/%.o)): $(PSP)/obj/%.o: build/gen/%/bodies.txt
endif

# The engine's C++: the runtime's core, SA1's client, and the PC backend
# but for its device and its window, which are the PSP's own. The C
# library's headers give POSIX's names under GNU's dialect alone, and this
# GCC warns where the host's compilers don't, so a warning stops no build
PSP_CXXFLAGS := -std=gnu++17 -fno-exceptions -fno-rtti -Wall -Wextra -Wno-format -Umips -O2 -G0 -I$(ENGINE_INCLUDE) -Iengine/sa1 -Iengine/pc -Iengine/runtime -Iengine/psp \
	-I$(PSP_SDK)/include
PSP_CXX_SOURCES := engine/runtime/engine.cpp \
	$(addprefix engine/pc/,backend.cpp world.cpp scene.cpp pvr.cpp ninja.cpp sound.cpp dsp.cpp disc.cpp \
	storage.cpp png.cpp) $(wildcard engine/psp/*.cpp)
PSP_CXX_OBJ := $(PSP_CXX_SOURCES:%.cpp=$(PSP)/obj/%.o)
$(PSP_CXX_OBJ): $(PSP)/obj/%.o: %.cpp $(PSP_NEEDS)
	@$(call status,psp,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(PSP_CXX) $(PSP_CXXFLAGS) -MD -MP -c -o $@ $<
-include $(PSP_CXX_OBJ:.o=.d) $(PSP)/obj/engine/sa1/play.d

# A program links alone first, as native.mk's does, and keeps global only the
# names something else reads, by programs.py's rules
psp_program_obj = $(filter $(addprefix $(PSP)/obj/,$(patsubst %.c,%.o,$(filter-out src/%,$(filter %.c,$(shell \
	awk '!/^#/ && NF { print $$1 }' config/$1/splits.txt))))) $(PSP)/obj/gen/$1/%,$(PSP_OBJ))
PSP_PROGRAM_OBJ := $(foreach s,$(NATIVE_PROGRAMS),$(call psp_program_obj,$s))
PSP_REST_OBJ := $(filter-out $(PSP_PROGRAM_OBJ),$(PSP_OBJ))
PSP_MEMORY := $(MEMORY_TOOLS) $(if $(PSP_GOALS),$(call from_tools,__import__("memory").SOURCES))
$(NATIVE_PROGRAMS:%=$(PSP)/program-%.r): $(PSP)/program-%.r: $(PSP_OBJ) engine/tools/psplink.py $(PSP_MEMORY) \
		$(PSP_NEEDS)
	@$(call status,link,$@)
	$(Q)PSPDEV=$(PSPDEV) python3 engine/tools/psplink.py partial $@ $* $(call psp_program_obj,$*)
$(PSP)/programs.c: engine/tools/programs.py config/imports.txt config/1st_read/imports.txt $(PSP_MEMORY) \
		$(PSP_REST_OBJ) $(NATIVE_PROGRAMS:%=$(PSP)/program-%.r) $(PSP_NEEDS)
	@$(call status,programs,$@)
	$(Q)NM=$(PSPDEV)/bin/psp-nm python3 engine/tools/programs.py $(PSP) $(NATIVE_PROGRAMS) -- $(PSP_REST_OBJ)
$(NATIVE_PROGRAMS:%=$(PSP)/program-%.o): $(PSP)/program-%.o: $(PSP)/program-%.r $(PSP)/programs.c
	$(Q)$(PSPDEV)/bin/psp-objcopy --redefine-syms=$(PSP)/rename-$*.txt --keep-global-symbols=$(PSP)/keep-$*.txt $< $@

# The names the program's objects need that nothing defines, by their rows
# of engine/native/stubs.txt, as native.mk's stubs. The PSP's own libraries
# stand for the host's
PSP_LIBS := -lpspgum -lpspgu -lpspge -lpspdisplay -lpspctrl -lpspaudio -lpsppower -lpspdebug -lstdc++ -latomic -lm
PSP_LIBRARIES := $(addprefix $(PSPDEV)/psp/lib/,libc.a libm.a libcglue.a libpthreadglue.a libstdc++.a) \
	$(addprefix $(PSP_SDK)/lib/,libpspgu.a libpspgum.a libpspge.a libpspdisplay.a libpspctrl.a libpspaudio.a \
	libpsppower.a libpspdebug.a libpspuser.a libpspsdk.a) $(shell $(PSP_CC) -print-libgcc-file-name 2> /dev/null)
PSP_MAIN_OBJ := $(PSP_REST_OBJ) $(PSP_CXX_OBJ) $(PSP)/obj/engine/sa1/play.o
$(PSP)/stubs.c: $(PSP_REST_OBJ) $(NATIVE_PROGRAMS:%=$(PSP)/program-%.o) engine/tools/stubs.py \
		engine/native/stubs.txt $(PSP_MEMORY) config/1st_read/symbols.txt config/1st_read/imports.txt $(PSP_NEEDS)
	@$(call status,stubs,$@)
	$(Q)NM=$(PSPDEV)/bin/psp-nm READELF=$(PSPDEV)/bin/psp-readelf python3 engine/tools/stubs.py $@ $(PSP_REST_OBJ) \
		$(NATIVE_PROGRAMS:%=$(PSP)/program-%.o) --aliases $(PSP)/aliases.txt --host psp --placed $(PSP)/placed.txt \
		--libraries $(PSP_LIBRARIES) $(PSP_EXPLORING)
$(PSP)/stubs.o: $(PSP)/stubs.c $(PSP_NEEDS)
	$(Q)$(PSP_CC) -std=gnu17 -O1 -G0 -w -c -o $@ $<

# The program: the system's own format, which it loads where it chooses and
# moves, and beside it what a run reads. A second name of 1ST_READ.BIN's is
# its symbol's by the link's word
PSP_WRAPS := pad_scan frame_wait njDrawTriangle3D njDrawPolygon2D njDrawLine3D njDrawTexture njDrawPolygon kmRender
PSP_LINK = $(PSP_CXX) -G0 -L$(PSP_SDK)/lib -specs=$(PSP_SDK)/lib/prxspecs -Wl,-q,-T$(PSP_SDK)/lib/linkfile.prx \
	-Wl,-zmax-page-size=128 $(PSP_WRAPS:%=-Wl$(comma)--wrap=%) \
	$$(awk '{ printf " -Wl,--defsym,%s=%s", $$1, $$2 }' $(PSP)/aliases.txt) $(PSP_MAIN_OBJ) $(PSP)/stubs.o $(PSP_LIBS)
$(PSP)/main.elf: $(PSP_MAIN_OBJ) $(PSP)/stubs.o $(NATIVE_PROGRAMS:%=$(PSP)/program-%.o) engine/tools/psplink.py \
		$(PSP_MEMORY) $(PSP_NEEDS)
	@$(call status,link,$@)
	$(Q)PSPDEV=$(PSPDEV) python3 engine/tools/psplink.py link $(PSP) $(PSP)/placed.txt $(NATIVE_PROGRAMS) -- $(PSP_LINK)
$(PSP)/game/EBOOT.PBP: $(PSP)/main.elf
	@$(call status,pbp,$@)
	$(Q)cp $< $(PSP)/fixed.elf && $(PSPDEV)/bin/psp-fixup-imports $(PSP)/fixed.elf
	$(Q)$(PSPDEV)/bin/psp-prxgen $(PSP)/fixed.elf $(PSP)/main.prx
	$(Q)$(PSPDEV)/bin/mksfoex 'Sonic Adventure' $(PSP)/PARAM.SFO
	$(Q)$(PSPDEV)/bin/pack-pbp $@ $(PSP)/PARAM.SFO NULL NULL NULL NULL NULL $(PSP)/main.prx NULL > /dev/null
psp: $(PSP)/game/EBOOT.PBP
