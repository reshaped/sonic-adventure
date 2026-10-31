# make gcc's rules: GCC's objects, each image's link and its checks, make
# gcc-disc's disc, and the toolchain and SH4ZAM's files. The Makefile reads
# this file for GCC_MK_GOALS alone, and no SHC object depends on it

# PINNED_GCC is the toolchain's driver where make builds the toolchain:
# where SH_ELF is tools/sh-elf, its default. SH4ZAM is the files
# tools/SHA256SUMS lists under tools/sh4zam/, which make fetches. FETCHED
# holds the lines of the last fetch that finished. Where they aren't the
# lines tools/SHA256SUMS gives now, every file of SH4ZAM's is UNFETCHED:
# make fetches every file again, and removes any other file under
# tools/sh4zam/, before GCC compiles any C, so that none stays from another
# pin. For make gcc and gcc-disc, DOWNLOADS takes each of these files that
# is missing or UNFETCHED
PINNED_GCC := $(filter tools/sh-elf/bin/sh-elf-gcc,$(SH_ELF)/bin/sh-elf-gcc)
sh4zam_sums = awk '$$2 ~ "^tools/sh4zam/"' tools/SHA256SUMS
SH4ZAM     := $(shell $(sh4zam_sums) | awk '{ print $$2 }')
FETCHED    := tools/sh4zam/fetched.txt
ifneq ($(strip $(file <$(FETCHED))),$(strip $(shell $(sh4zam_sums))))
UNFETCHED  := $(SH4ZAM)
endif
DOWNLOADS  += $(filter-out $(wildcard $(PINNED_GCC) $(filter-out $(UNFETCHED),$(SH4ZAM))), \
	$(if $(filter gcc gcc-disc,$(MAKECMDGOALS)),$(PINNED_GCC) $(SH4ZAM)))
# The toolchain's sources unpack with tar, gzip and xz, and build with c++.
# Where the driver is a download, UNPACKED has the host checked for tar and
# gzip. make gcc and gcc-disc check each image's content with cmp
UNPACKED   += %/sh-elf-gcc
$(call check_host,$(if $(filter %/sh-elf-gcc,$(DOWNLOADS)),xz c++) $(if $(filter gcc gcc-disc,$(MAKECMDGOALS)),cmp))

# make reads config/gcc.mk for GCC_GOALS alone, so a fault in it stops only
# a run that names one of them. For a goal under tools/sh-elf/ or
# tools/sh4zam/, make reads gcc.mk and not config/gcc.mk
ifneq ($(GCC_GOALS),)
include config/gcc.mk
endif

# GCC's stamp holds the hashes of its driver, its C compiler, cc1, and its
# assembler
GCC_ID := build/tools/gcc.id
$(eval $(call stamp,gcc,$(SH_ELF)/bin/sh-elf-gcc,$(SH_ELF)/bin/sh-elf-gcc $(SH_ELF)/libexec/gcc/sh-elf/*/cc1 $(SH_ELF)/bin/sh-elf-as,$(GCC_GOALS)))
# The linker, the archiver and libgcc.a take a stamp of their own. A change to
# one rebuilds only each image's content object, the archive and the link.
# GCC's version is part of the paths of cc1 and libgcc.a, so the stamps name
# them by a shell glob. A refresh expands the glob as it hashes, once the
# toolchain is in place, and fails where it matches no file
LD_ID  := build/tools/ld.id
$(eval $(call stamp,ld,$(SH_ELF)/bin/sh-elf-gcc,$(SH_ELF)/bin/sh-elf-ld $(SH_ELF)/bin/sh-elf-ar $(SH_ELF)/lib/gcc/sh-elf/*/libgcc.a,$(GCC_GOALS)))

# make gcc's objects lie under build/gcc/, each at its source's path. GCC
# compiles each of the tree's C sources but the grafted source's donor,
# whose four functions only SHC's images hold, and each image's content, the
# data of its gen/ ranges. A C source in gcc/ is GCC's alone. gcc/P.c
# compiles to build/gcc/P.o, the object P.src would assemble to, and so
# replaces P.src. Or GCC_ADDED names it, and its image links its object
# after its sources'. GNU as assembles each .src file that no C source in
# gcc/ replaces. gcc/ lies outside ROOTS, so no SHC route, clang-tidy,
# progress or review reads it
GCC_PORT_C := $(filter %.c,$(PORT_FILES))
GCC_PORT_OBJ := $(GCC_PORT_C:gcc/%.c=build/gcc/%.o)
GCC_ADDED_C := $(foreach a,$(GCC_ADDED),$(word 2,$(subst :, ,$a)))
GCC_ADDED_OBJ := $(GCC_ADDED_C:gcc/%.c=build/gcc/%.o)
# $(call gcc_of,OBJECTS) gives the GCC object of each of OBJECTS, which are
# SHC's objects under build/. It leaves out each whose source holds inline
# assembly, which the split's rules build
gcc_of   = $(patsubst build/%.obj,build/gcc/%.o,$(filter-out $(INLINE_C:%.c=build/%.obj),$1))
GCC_C_OBJ := $(call gcc_of,$(C_SOURCE_OBJ))
GCC_INLINE_OBJ := $(patsubst %.c,build/gcc/%.o,$(filter-out $(DONOR_C),$(INLINE_C)))
GCC_ASM_OBJ := $(filter-out $(GCC_PORT_OBJ),$(ASM_SRC:%.src=build/gcc/%.o))
GCC_CONTENT_OBJ := $(GEN_CONF:%=build/gcc/gen/%/content.o)
# SH4ZAM's library is its SH-4 back end, source/sh4/, which its headers take
# under -m4-single-only, as config/gcc.mk says. The back end defines each
# function the headers declare out of line under SHZ_BACKEND == SHZ_SH4, but
# shz_version.h's two, which no object calls. The software back end,
# source/sw/, stays out of the library: it calls sqrtf, fabsf, sinf and
# cosf, which the SDK's math.h doesn't declare. GCC_SH4ZAM_C_OBJ is the back
# end's C objects, which alone of SH4ZAM's objects write .d files
SH4ZAM_SOURCES := $(filter tools/sh4zam/source/sh4/%,$(SH4ZAM))
GCC_SH4ZAM_OBJ := $(patsubst tools/sh4zam/source/%,build/gcc/sh4zam/%.o,$(basename $(SH4ZAM_SOURCES)))
GCC_SH4ZAM_C_OBJ := $(patsubst tools/sh4zam/source/%.c,build/gcc/sh4zam/%.o,$(filter %.c,$(SH4ZAM_SOURCES)))
# Each image's link takes from the archive the members its objects call
GCC_SH4ZAM_LIB := build/gcc/sh4zam/libsh4zam.a
# GCC_OBJ is every object make gcc builds, from the lists GCC_OBJ_LISTS names
GCC_OBJ_LISTS := GCC_C_OBJ GCC_PORT_OBJ GCC_INLINE_OBJ GCC_ASM_OBJ GCC_CONTENT_OBJ GCC_SH4ZAM_OBJ
GCC_OBJ  := $(foreach l,$(GCC_OBJ_LISTS),$($l))
# Each C source in gcc/ either replaces a .src file or is added to an image
# by an entry of GCC_ADDED, IMAGE:SOURCE. Stop on any other entry or source:
# its object would be another source's, or no image's
ifneq ($(GCC_GOALS),)
UNADDED := $(strip $(foreach a,$(GCC_ADDED),$(if $(and $(filter 2,$(words $(subst :, ,$a))),$(filter $(GCC_PORT_C),\
	$(word 2,$(subst :, ,$a)))),,$a)))
ifneq ($(UNADDED),)
$(error $(UNADDED): an entry of GCC_ADDED, and not IMAGE:SOURCE, SOURCE a C source in gcc/)
endif
OVERLAID := $(filter $(GCC_C_OBJ) $(GCC_INLINE_OBJ) $(ASM_SRC:%.src=build/gcc/%.o),$(GCC_ADDED_OBJ))
ifneq ($(OVERLAID),)
$(error $(OVERLAID:build/gcc/%.o=gcc/%.c): GCC_ADDED names it, and it compiles to a source's object)
endif
UNREPLACED := $(filter-out $(ASM_SRC:%.src=build/gcc/%.o) $(GCC_ADDED_OBJ),$(GCC_PORT_OBJ))
ifneq ($(UNREPLACED),)
$(error $(UNREPLACED:build/gcc/%.o=gcc/%.c): replaces no .src file, and GCC_ADDED doesn't name it)
endif
# Stop on an object two of the lists give, as two of SH4ZAM's sources of one
# stem would: make would build it by one rule alone. A replacement's object
# is no such object, since GCC_ASM_OBJ leaves it out
TWICE := $(shell printf '%s\n' $(GCC_OBJ) | sort | uniq -d)
ifneq ($(TWICE),)
$(error $(foreach o,$(TWICE),$o: two of make gcc's rules build it, by $(strip $(foreach l,$(GCC_OBJ_LISTS),\
	$(if $(filter $o,$($l)),$l)))))
endif
endif

# $(call shc_image_obj,IMAGE) gives the SHC object of each source IMAGE's
# splits.txt places
shc_image_obj = $(foreach l,$(filter build/$1/%,$(LINKS)),$(call object,$(l:build/%.bin=%)))
# $(call gcc_image_obj,IMAGE) gives IMAGE's GCC objects: one for each source
# its splits.txt places, with its content in place of its gen/ ranges'.
# GCC_LINKED is the GCC objects make gcc links: every image's, and GCC_ADDED's
gcc_image_obj = $(patsubst build/%.obj,build/gcc/%.o,$(filter-out $(GEN_OBJ),$(call shc_image_obj,$1))) \
	$(filter build/gcc/gen/$1/%,$(GCC_CONTENT_OBJ))
GCC_LINKED := $(sort $(GCC_ADDED_OBJ) $(foreach i,$(sort $(foreach l,$(LINKS),$(call image,$(l:build/%=%)))),\
	$(call gcc_image_obj,$i)))

# $(call gcc_flags,FLAGS,RELEASE) is GCC's flags for a source: GCCFLAGS,
# then -O0 where FLAGS, SHC's flags for the source, hold -optimize=0, then
# -ffp-contract=on where RELEASE, the SHC that builds it, is r26 or r32,
# which fuse products into sums. -ffp-contract=on comes after GCCFLAGS'
# -ffp-contract=off, so no object's line can undo it
gcc_flags = $(GCCFLAGS) $(if $(filter -optimize=0,$1),-O0) $(if $(filter r26 r32,$2),-ffp-contract=on)
# $(call gcc,INCLUDES,FLAGS,RELEASE[,C]) compiles the source to $@ under
# gcc_flags. INCLUDES is the route's include directories, split by commas,
# which GCC searches as SHC does. FLAGS is SHC's flags for the source, and
# RELEASE, r10, r26 or r32, the SHC that builds it. C, where given, is the C
# split from a source with inline assembly, which compiles in the source's
# place and finds its quoted includes in the source's directory. With
# RELEASE, GCC's preprocessor writes $@'s .i, which tools/floats.py rewrites
# for RELEASE and the flags of FLAGS it reads, -fpu=single, -double=float
# and -aggressive=2. The pass gives the .i's floating constants and static
# initializers SHC's values, and on r26 and r32 wraps the products and
# writes the += that tools/float_fuse.py names, so that GCC fuses as SHC does.
# It leaves the lines of SH4ZAM's headers, under SH4ZAM_INCLUDE, as they are.
# GCC then compiles the .i. A source no SHC builds names no RELEASE, and
# compiles in one run, with no float pass. GCC fuses no product into a sum in
# gcc/'s sources, under GCCFLAGS' -ffp-contract=off, and fuses where it can in
# SH4ZAM's library, under GNU C's default, -ffp-contract=fast. GCC runs in the
# C locale with plain diagnostics, so its log reads the same on any host. It
# writes the .d file as a .tmp, which replaces the last one once the compile
# passes, so make reads the last whole one after a compile that fails. The .i
# goes last, once the .d file is in place, so it stays for a compile that
# fails
gcc = mkdir -p $(@D) && (LC_ALL=C $(SH_ELF)/bin/sh-elf-gcc $(call gcc_flags,$2,$3) $(if $4,-iquote $(<D)) \
	$(addprefix -I,$(subst $(comma), ,$1)) -fdiagnostics-plain-output -MD -MP -MT $@ -MF $(@:.o=.d).tmp $(if $3,-E \
	$(or $4,$<) > $(@:.o=.i) && python3 tools/floats.py \
	$(strip --shc $3 $(filter -fpu=single -double=float -aggressive=2,$2)) \
	--system $(SH4ZAM_INCLUDE) $(@:.o=.i) && LC_ALL=C $(SH_ELF)/bin/sh-elf-gcc $(call gcc_flags,$2,$3) \
	-fdiagnostics-plain-output -c -o $@ $(@:.o=.i),-c -o $@ $(or $4,$<))) $(gcc_quiet) && \
	mv $(@:.o=.d).tmp $(@:.o=.d) && rm -f $(@:.o=.i)
# Keep GCC's output in $@.log, and print it and fail on a nonzero status,
# which GCC_WARNINGS' -Werror gives each warning
gcc_quiet = > $@.log 2>&1 || (cat $@.log; false)

# The root, searched last, lets a source in gcc/ name a header by its path
# from the root, as it names gcc/sdk/ninja/njsprite.h and sdk/ninja/njkamui.h
PORT_INC    := sdk/include,src,.
# A movie SDK member with inline assembly sees the SDK's headers under GCC
# too, as under SHC
$(patsubst %.c,build/gcc/%.o,$(filter $(MOVIE_SDK),$(ASMCODE))): ASMCODE_INC := $(SDK_INC)

# make gcc builds every GCC object, and links the images
gcc: $(GCC_OBJ) build/gcc/manifest.txt

# build/gcc/port.txt records GCC_PORT_OBJ. make rewrites it as it reads
# gcc.mk, where it is missing or differs, and only then. gcc/P.c's rule names
# gcc/P.c alone, and GCC_ASM_OBJ's names P.src alone. So once P.c joins gcc/
# or leaves it, build/gcc/P.o would look up to date by the rule that builds
# it now. Each object of either rule depends on the record too, and builds
# again once the record changes. A moved object's .d file can name a source
# that is gone, which make can't remake, so make removes it. With no record,
# every object of gcc/'s counts as moved
ifneq ($(GCC_GOALS),)
PORT_KEPT := $(if $(wildcard build/gcc/port.txt),$(sort $(file <build/gcc/port.txt)),none)
ifneq ($(PORT_KEPT),$(sort $(GCC_PORT_OBJ)))
PORT_MOVED := $(filter build/gcc/%.o,$(filter-out $(PORT_KEPT),$(GCC_PORT_OBJ)) $(filter-out $(GCC_PORT_OBJ),$(PORT_KEPT)))
$(shell mkdir -p build/gcc && rm -f $(PORT_MOVED:.o=.d) && echo $(sort $(GCC_PORT_OBJ)) > build/gcc/port.txt.tmp && \
	mv build/gcc/port.txt.tmp build/gcc/port.txt)
ifneq ($(.SHELLSTATUS),0)
$(error build/gcc/port.txt: make can't remove a moved object's .d file or write the record)
endif
endif
$(GCC_PORT_OBJ) $(GCC_ASM_OBJ): build/gcc/port.txt
endif
# GCC_COMPILED is the objects GCC compiles, each from C. Each depends on its
# .d file, which keeps its header dependencies, as an SHC object's does, and
# build/gcc/headers.txt holds gcc/'s headers as build/headers.txt holds
# those under ROOTS
GCC_COMPILED := $(GCC_C_OBJ) $(GCC_PORT_OBJ) $(GCC_INLINE_OBJ) $(GCC_CONTENT_OBJ) $(GCC_SH4ZAM_C_OBJ)
$(GCC_COMPILED): %.o: %.d
$(GCC_COMPILED:.o=.d): ;
-include $(GCC_COMPILED:.o=.d)
ifneq ($(GCC_GOALS),)
$(call record,build/gcc/headers.txt,$(sort $(filter %.h,$(PORT_FILES))))
endif
$(GCC_COMPILED): build/headers.txt build/gcc/headers.txt

# The float pass: tools/floats.py and its modules
FLOATS := tools/floats.py $(wildcard tools/float_*.py)
# A GCC object depends on the files that give its command: gcc.mk and
# config/gcc.mk, beside an SHC object's. Each whose .i the float pass reads
# depends on FLOATS too, and on the modules they import
$(GCC_OBJ): Makefile config/compilers.mk gcc.mk config/gcc.mk
$(GCC_C_OBJ) $(GCC_INLINE_OBJ) $(GCC_CONTENT_OBJ): $(FLOATS) tools/csource.py tools/tool.py tools/layout.py

# GCC compiles each C source with its route's include directories, flags and
# release. Each waits for SH4ZAM's files, whose headers GCC's side of the code
# includes
$(call gcc_of,$(R32_GAME_OBJ)): build/gcc/%.o: %.c $(GCC_ID) | $(FETCHED)
	@$(call status,gcc,$<)
	$(Q)$(call gcc,$(GAME_INC),$(GAME),r32)

$(call gcc_of,$(R10_SDK_OBJ)): build/gcc/%.o: %.c $(GCC_ID) | $(FETCHED)
	@$(call status,gcc,$<)
	$(Q)$(call gcc,$(SDK_INC),$(SDK),r10)

$(call gcc_of,$(R32_RUNTIME_OBJ)): build/gcc/%.o: %.c $(GCC_ID) | $(FETCHED)
	@$(call status,gcc,$<)
	$(Q)$(call gcc,$(R32_SDK_INC),$(RUNTIME),r32)

$(call gcc_of,$(PATCHED_OBJ)): build/gcc/%.o: %.c $(GCC_ID) | $(FETCHED)
	@$(call status,gcc,$<)
	$(Q)$(call gcc,$(PATCHED_INC),$(SDK),r10)

$(call gcc_of,$(R26_SDK_OBJ)): build/gcc/%.o: %.c $(GCC_ID) | $(FETCHED)
	@$(call status,gcc,$<)
	$(Q)$(call gcc,$(R26_SDK_INC),$(SDK),r26)

$(call gcc_of,$(R32_SDK_OBJ)): build/gcc/%.o: %.c $(GCC_ID) | $(FETCHED)
	@$(call status,gcc,$<)
	$(Q)$(call gcc,$(R32_SDK_INC),$(SDK),r32)

# No SHC builds a source in gcc/, so it takes none of SHC's flags and no
# release
$(GCC_PORT_OBJ): build/gcc/%.o: gcc/%.c $(GCC_ID) | $(FETCHED)
	@$(call status,gcc,$<)
	$(Q)$(call gcc,$(PORT_INC))

# SH4ZAM's C compiles under GCC_SH4ZAM_FLAGS, which config/gcc.mk gives its
# objects, and with no release. It takes assert.h from the SDK's headers: a
# freestanding GCC has none
build/gcc/sh4zam/%.o: tools/sh4zam/source/%.c $(GCC_ID) | $(FETCHED)
	@$(call status,gcc,$<)
	$(Q)$(call gcc,sdk/include)

# GNU as assembles SH4ZAM's assembly as it is, little-endian under GAS_CHECKS,
# not GASFLAGS. It pads each section to its alignment, which GASFLAGS'
# --no-pad-sections would not: make gcc's 1ST_READ.BIN and LOADER.BIN hold the
# 7 NOPs, 14 bytes, that pad .text._shz_memcpy32_sh4_ to a multiple of 32. The
# assembly includes no file, so it waits for no fetch but its own
build/gcc/sh4zam/%.o: tools/sh4zam/source/%.s $(GCC_ID)
	@$(call status,gas,$<)
	$(Q)mkdir -p $(@D) && $(SH_ELF)/bin/sh-elf-as $(GAS_CHECKS) --little -o $@ $<

# make removes the archive before each write, so it holds no member another
# pin left. ar's D records no dates or user ids, so the same objects make the
# same archive. Its members' inventory writes it again when a pin drops one
ifneq ($(GCC_GOALS),)
$(call record,build/gcc/sh4zam/members.txt,$(sort $(GCC_SH4ZAM_OBJ)))
endif
$(GCC_SH4ZAM_LIB): $(GCC_SH4ZAM_OBJ) build/gcc/sh4zam/members.txt $(LD_ID)
	@$(call status,ar,$@)
	$(Q)mkdir -p $(@D) && rm -f $@ && $(SH_ELF)/bin/sh-elf-ar rcsD $@ $(GCC_SH4ZAM_OBJ)

# Two checks read a .src file before build/shckit translates it for GNU as.
# tools/tags.py --abi refuses a function tagged @asm abi whose name the
# object's CONTRACTS lacks. tools/floats.py --assembly refuses an .FDATA.S
# constant that asmsh and GNU as can read as two floats, as
# tools/float_asm.py says
$(GCC_ASM_OBJ): build/gcc/%.o: %.src build/shckit tools/tags.py tools/csource.py $(FLOATS) tools/layout.py \
		$(CONFIG_TOOLS) $(TABLES) $(GCC_ID)
	@$(call status,gas,$<)
	$(Q)mkdir -p $(@D)
	$(Q)python3 tools/tags.py --abi $(CONTRACTS:%=--contract %) $<
	$(Q)python3 tools/floats.py --assembly $<
	$(Q)build/shckit translate -o build/gcc/$*.s $<
	$(Q)$(SH_ELF)/bin/sh-elf-as $(GASFLAGS) -o $@ build/gcc/$*.s

# A source with inline assembly passes the same checks. tools/inline.py
# splits it into C and the text of its inline assembly, which build/shckit
# translates for the C's file-scope asm to include. Where every block lies on
# SHC's side of a GCC guard, the text is empty. shckit refuses an empty text,
# so the split translates none, and the C includes none
split = mkdir -p $(@D) && python3 tools/tags.py --abi $(CONTRACTS:%=--contract %) $< && \
	python3 tools/inline.py $< build/gcc/$*.c build/gcc/$*_inline.src && \
	python3 tools/floats.py --assembly build/gcc/$*_inline.src && \
	if [ -s build/gcc/$*_inline.src ]; then build/shckit translate -o build/gcc/$*_inline.s build/gcc/$*_inline.src; \
	else rm -f build/gcc/$*_inline.s; fi
SPLIT_NEEDS := tools/inline.py $(FLOATS) tools/tags.py tools/csource.py tools/layout.py $(CONFIG_TOOLS) \
	$(TABLES) build/shckit $(GCC_ID)

$(ASMCODE:%.c=build/gcc/%.o): build/gcc/%.o: %.c $(SPLIT_NEEDS) | $(FETCHED)
	@$(call status,gcc,$< (split))
	$(Q)$(split)
	$(Q)$(call gcc,$(ASMCODE_INC),$(SDK),r10,build/gcc/$*.c)

$(patsubst %.c,build/gcc/%.o,$(filter $(INLINE_C),$(PATCHED_C))): build/gcc/%.o: %.c $(SPLIT_NEEDS) | $(FETCHED)
	@$(call status,gcc,$< (split))
	$(Q)$(split)
	$(Q)$(call gcc,$(PATCHED_INC),$(SDK),r10,build/gcc/$*.c)

$(patsubst %.c,build/gcc/%.o,$(filter $(INLINE_C),$(GRAFTED_C))): build/gcc/%.o: %.c $(SPLIT_NEEDS) | $(FETCHED)
	@$(call status,gcc,$< (split))
	$(Q)$(split)
	$(Q)$(call gcc,$(SDK_INC),$(SDK),r10,build/gcc/$*.c)

# An empty recipe lets a split C go missing, though its object's .d file names
# it: the object then splits its source again
$(GCC_INLINE_OBJ:.o=.c): ;

# GCC compiles an image's content under the game's flags, and the float pass
# reads its constants as R32 reads a gen/ range's C. GNU ld then links it as
# content.ld says: each name it defines, and each it uses from elsewhere, at
# its address in the image. cmp holds the link's bytes to content.bin, the
# image's. The content writes each pointer into RAM as a name, which the link
# resolves, so the check holds those pointers too
$(GCC_CONTENT_OBJ): build/gcc/%/content.o: build/%/content.c $(GCC_ID) $(LD_ID) | $(FETCHED)
	@$(call status,gcc,build/$*/content.c)
	$(Q)$(call gcc,$(GEN_INC),$(GAME),r32)
	$(Q)$(SH_ELF)/bin/sh-elf-ld -m shlelf --orphan-handling=error --fatal-warnings --oformat=binary \
		-T build/$*/content.ld -o $(@:.o=.bin) $@ > $(@:.o=.bin).log 2>&1 || (cat $(@:.o=.bin).log; false)
	$(Q)cmp -s $(@:.o=.bin) build/$*/content.bin || \
		(echo "$@: linked at the disc's addresses, it differs from build/$*/content.bin"; false)

# MEMORY_SOURCES is the sources tools/memory.py reads, as its SOURCES lists
# them. The link depends on each, so a change to one links again, and a
# missing one stops make even where the manifest is current
MEMORY_SOURCES := $(if $(GCC_GOALS),$(call from_tools,__import__("memory").SOURCES))
# GCC_PUBLISHED is what the link writes before the manifest: each image's
# final ELF, and its file under build/gcc/ at its original's path, with a .PRS
# file's image beside it as a .BIN. An empty recipe lets a published file go
# missing without stopping make. The manifest depends on each, so the link
# then runs again
GCC_PUBLISHED := $(if $(GCC_GOALS),$(call from_tools,(each for name, path in images.files().items() \
	for each in (f"build/gcc/link/{name}/final.elf", f"build/gcc/{path}", \
	*([f"build/gcc/{path[:-4]}.BIN"] if path.endswith(".PRS") else [])))))
$(GCC_PUBLISHED): ;
# make gcc's link. tools/spec.py plans it, writing the spec by which
# tools/ld.py links each image. tools/verify.py checks the final links.
# Then tools/publish.py writes the images' files, and the manifest
# last. The link waits for the census and the import check, so a disc comes
# only from a link both passed. The manifest doesn't depend on either check's
# date, so a check that runs again and passes links nothing again. The link
# first removes the manifest and the disc made from it, so a link that fails
# or stops leaves neither. A check or an object that fails before the link
# leaves both as they were, and make's status says the run failed
build/gcc/manifest.txt: $(GCC_LINKED) $(GCC_SH4ZAM_LIB) $(MEMORY_SOURCES) $(GCC_PUBLISHED) \
		config/imports.txt config/declarations.txt $(TABLES) tools/spec.py tools/ld.py tools/verify.py \
		tools/decls.py tools/dwarf.py tools/publish.py tools/manifest.py tools/gcclink.py tools/elf.py \
		tools/memory.py tools/csource.py tools/layout.py $(CONFIG_TOOLS) build/prskit $(LD_ID) | \
		build/gcc/census.ok build/gcc/imports.ok
	@$(call status,link,build/gcc/link)
	$(Q)rm -rf $@ build/gcc/disc build/gcc/disc.partial
	$(Q)mkdir -p build/gcc/link
	$(Q)SH_ELF=$(SH_ELF) python3 tools/spec.py --archive $(GCC_SH4ZAM_LIB)$(if \
		$(GCC_ADDED), $(subst :gcc/, build/gcc/,$(GCC_ADDED:%.c=--add %.o))) --list build/gcc/link/unbound.txt build/gcc/link/spec.txt
	$(Q)SH_ELF=$(SH_ELF) python3 tools/ld.py build/gcc/link/spec.txt build/gcc/link
	$(Q)SH_ELF=$(SH_ELF) python3 tools/verify.py --abi='$(GCC_ABI)' build/gcc/link/spec.txt build/gcc/link
	$(Q)SH_ELF=$(SH_ELF) python3 tools/publish.py build/gcc/link/spec.txt build/gcc/link $@

# make gcc-disc builds its disc from the one .gdi in orig/, the Makefile's
# GDI_IMAGE, and its tracks
ifneq ($(filter gcc-disc,$(MAKECMDGOALS)),)
ifneq ($(FOUND),1)
$(error gcc-disc needs the disc's .gdi and its tracks in orig/, and orig/ holds $(FOUND) .gdi files)
endif
endif
gcc-disc: build/gcc/manifest.txt build/gdi
	@$(call status,disc,build/gcc/disc)
	$(Q)rm -rf build/gcc/disc build/gcc/disc.partial
	$(Q)python3 tools/disc.py $(call quoted,$(GDI_IMAGE)) build/gcc/manifest.txt build/gcc/disc

# The census check. The link lays out 1ST_READ.BIN anew, so it waits for the
# check. Its first pass, census.py --generated --disc, reads each decoded
# motion file and 1ST_READ.BIN's SHC objects, whose .d files name what each
# compile read. Its --gcc pass reads every image's GCC side, so the check
# waits for each object the link takes. That pass reads the SHC objects' .d
# files too, to tell which rows of census.txt the first pass counts, and
# counts the others. The check can't run without census.txt, which it names,
# so a deleted census.txt stops make
MOTION_DECODED := $(patsubst SONICADV/%.PRS,build/gen/img/%.BIN,$(MOTIONS))
FIRST_READ_OBJ := $(call shc_image_obj,1st_read)
build/gcc/census.ok: $(FIRST_READ_OBJ) $(MOTION_DECODED) $(GCC_LINKED) config/1st_read/census.txt $(TABLES) \
		tools/census.py tools/content.py tools/stages.py tools/memory.py tools/bodies.py $(DATA_TOOLS) tools/gcclink.py tools/elf.py
	@$(call status,census,config/1st_read/census.txt)
	$(Q)python3 tools/census.py --generated --disc
	$(Q)python3 tools/census.py --gcc$(if $(GCC_ADDED), $(subst :, ,$(GCC_ADDED:%=--add %)))
	$(Q)mkdir -p $(@D) && touch $@

# The import check. SHC's build links each of 1ST_READ.BIN's imports at the
# address its image's symbols.txt gives. make gcc's link binds each by name
# instead, so tools/imports.py holds each import to the definition at that
# address. It reads the SHC objects build/gcc/deps.mk names, which define the
# imports, and 1ST_READ.BIN's objects of either compiler, whose .d files name
# what each compile read. It reads each source GCC_ADDED gives 1ST_READ.BIN
# too, so it waits for the objects of GCC_ADDED's sources, whose .d files
# name their headers. make reads build/gcc/deps.mk for GCC_GOALS alone,
# and tools/deps.py --gcc rewrites it on each run where its text changes
ifneq ($(GCC_GOALS),)
include build/gcc/deps.mk
endif
build/gcc/deps.mk: FORCE
	+$(Q)python3 tools/deps.py --gcc $@

build/gcc/imports.ok: $(FIRST_READ_OBJ) $(call gcc_image_obj,1st_read) $(GCC_ADDED_OBJ) $(TABLES) tools/imports.py \
		$(CONFIG_TOOLS) tools/stages.py tools/csource.py tools/shc_object.py tools/layout.py tools/bodies.py
	@$(call status,import,config/1st_read/imports.txt)
	$(Q)python3 tools/imports.py$(if $(GCC_ADDED), $(subst :, ,$(GCC_ADDED:%=--add %)))
	$(Q)mkdir -p $(@D) && touch $@

# A body source's GCC object waits for its bodies, as its SHC object does
$(BODY_C:%.c=build/gcc/%.o): build/gcc/%.o: build/gen/%/bodies.txt

# The toolchain make builds where SH_ELF names tools/sh-elf: binutils, and
# GCC for C alone with the libgcc of its one multilib, whose division, copy
# and double routines GCC's code calls. GMP, MPFR and MPC build inside GCC's
# tree, which builds GMP for no CPU in particular, so GMP needs no m4. The
# recipe fetches the sources itself, as SHC's does. It builds under
# build/toolchain/ and installs in tools/sh-elf.part, where it writes the
# toolchain's receipt, request.txt: the sources, their lines of
# tools/SHA256SUMS and each configure's flags. tools/sh-elf.part then takes
# tools/sh-elf's place, whole, and the recipe removes the sources and the
# build. The tool stamps hold what the binaries are, and the receipt what
# they were built from, so make refuses a toolchain whose receipt differs,
# and leaves it in place
SH_ELF_SOURCES := binutils/binutils-2.47.tar.xz gcc/gcc-16.2.0/gcc-16.2.0.tar.xz gmp/gmp-6.3.0.tar.xz \
	mpc/mpc-1.3.1.tar.gz mpfr/mpfr-4.2.1.tar.xz
SH_ELF_PREFIX := $(call quoted,$(CURDIR)/tools/sh-elf.part)
SH_ELF_BINUTILS := --target=sh-elf --disable-nls --disable-werror
SH_ELF_GCC := --target=sh-elf --enable-languages=c --with-newlib --without-headers --with-endian=little \
	--with-cpu=m4-single-only --disable-multilib --disable-nls --disable-shared --disable-threads --disable-lto \
	--without-isl --disable-libssp --disable-libquadmath --disable-libgomp
SH_ELF_REQUEST := $(strip $(SH_ELF_SOURCES) $(shell awk '$$2 ~ "^tools/sh-elf.src/"' tools/SHA256SUMS) \
	binutils $(SH_ELF_BINUTILS) gcc $(SH_ELF_GCC))
ifneq ($(wildcard $(PINNED_GCC)),)
ifneq ($(strip $(file <tools/sh-elf/request.txt)),$(SH_ELF_REQUEST))
$(error tools/sh-elf: its request.txt names another build than gcc.mk's; remove tools/sh-elf, and make builds it again)
endif
endif
# $(call sh_elf,PACKAGE,COMMAND) runs COMMAND in PACKAGE's build directory and
# adds its output to PACKAGE's log, which a failure names. No make that
# COMMAND runs inherits this make's flags, its jobserver among them.
# SH_ELF_MAKE and SH_ELF_INSTALL run this make's own executable, $(MAKE). The
# first builds a package with this make's -j, if it has one, and the second
# installs it with one job. MAKEINFO=true keeps each from writing a manual,
# which would need makeinfo
sh_elf = (mkdir -p build/toolchain/$1.build && cd build/toolchain/$1.build && unset MAKEFLAGS MFLAGS MAKELEVEL && \
	$2) >> build/toolchain/$1.log 2>&1 || (echo 'build/toolchain/$1.log: $1 failed'; false)
SH_ELF_MAKE = $(MAKE) $(filter -j%,$(MAKEFLAGS))
SH_ELF_INSTALL = $(MAKE)
tools/sh-elf/bin/sh-elf-gcc:
	@$(call status,fetch,tools/sh-elf.src)
	$(Q)rm -rf tools/sh-elf.src && mkdir -p tools/sh-elf.src$(foreach s,$(SH_ELF_SOURCES), && $(call fetch,https://ftp.gnu.org/gnu/$s,tools/sh-elf.src/$(notdir $s)))
	@$(call status,build,tools/sh-elf)
	$(Q)rm -rf build/toolchain tools/sh-elf.part
	$(Q)for p in binutils gcc gcc/gmp gcc/mpfr gcc/mpc; do mkdir -p build/toolchain/$$p && \
		tar -xf tools/sh-elf.src/$${p#gcc/}-*.tar.* -C build/toolchain/$$p --strip-components=1 || exit 1; done
	$(Q)$(call sh_elf,binutils,../binutils/configure --prefix=$(SH_ELF_PREFIX) $(SH_ELF_BINUTILS))
	$(Q)$(call sh_elf,binutils,$(SH_ELF_MAKE) MAKEINFO=true && $(SH_ELF_INSTALL) install MAKEINFO=true)
	$(Q)$(call sh_elf,gcc,PATH=$(SH_ELF_PREFIX)/bin:$$PATH && export PATH && \
		../gcc/configure --prefix=$(SH_ELF_PREFIX) $(SH_ELF_GCC) && \
		$(SH_ELF_MAKE) all-gcc all-target-libgcc MAKEINFO=true && \
		$(SH_ELF_INSTALL) install-gcc install-target-libgcc MAKEINFO=true)
	$(Q)printf '%s\n' $(call quoted,$(SH_ELF_REQUEST)) > tools/sh-elf.part/request.txt
	$(Q)rm -rf tools/sh-elf && mv tools/sh-elf.part tools/sh-elf
	$(Q)rm -rf tools/sh-elf.src build/toolchain

# SH4ZAM's library whole: each file of its include/ and source/, and its
# LICENSE, fetched by itself from the commit SH4ZAM_AT names. SH4ZAM's
# AGENTS.md, as of its commit ad2f890, accepts a copy only by its third
# route, a synchronized copy: the whole library, pulled from upstream by a
# build mechanism that records the commit. GitHub's archive of a commit
# holds the same files, but the archive's compressed bytes can change
SH4ZAM_AT := https://raw.githubusercontent.com/gyrovorbis/sh4zam/70dbb177b20c1734fbe4f97df0f8c6ec87a8e8d3
$(SH4ZAM): $(if $(UNFETCHED),FORCE)
	@$(call status,fetch,$@)
	$(Q)rm -f $(FETCHED) && mkdir -p $(@D) && $(call fetch,$(SH4ZAM_AT)/$(@:tools/sh4zam/%=%),$@)
# Each fetch removes FETCHED, which make writes only once every file is in
# place. After a run of fetches cut short, make fetches every file again.
# Once every file is in place, FETCHED's recipe removes each other file
# under tools/sh4zam/, one an earlier pin listed, then writes FETCHED
$(FETCHED): $(SH4ZAM)
	$(Q)rm -f $(filter-out $(SH4ZAM) $(FETCHED),$(shell [ ! -d tools/sh4zam ] || find tools/sh4zam -type f))
	$(Q)mkdir -p $(@D) && $(sh4zam_sums) > $@
