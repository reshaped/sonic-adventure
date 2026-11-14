# make dc's rules: GCC's objects, each image's link and its checks, make
# dc-disc's disc, make dc-check's sums, and the toolchain and SH4ZAM's files.
# The Makefile reads this file for DC_MK_GOALS alone, and no SHC object
# depends on it

# PINNED_GCC is the toolchain's driver where make builds the toolchain:
# where SH_ELF is tools/sh-elf, its default. SH4ZAM is the files
# tools/SHA256SUMS lists under tools/sh4zam/, which make fetches. FETCHED
# holds their lines as of the last fetch that finished. Where they aren't
# the lines tools/SHA256SUMS gives now, every file of SH4ZAM's is UNFETCHED:
# make fetches every file again, and removes any other file under
# tools/sh4zam/, before GCC compiles any C, so that none stays from another
# pin. For make dc, dc-disc, dc-check and dc-order, DOWNLOADS takes each of
# these files that is missing or UNFETCHED
PINNED_GCC := $(filter tools/sh-elf/bin/sh-elf-gcc,$(SH_ELF)/bin/sh-elf-gcc)
sh4zam_sums = awk '$$2 ~ "^tools/sh4zam/"' tools/SHA256SUMS
SH4ZAM     := $(shell awk '$$2 ~ "^tools/sh4zam/" { print $$2 }' tools/SHA256SUMS)
FETCHED    := tools/sh4zam/fetched.txt
ifneq ($(strip $(file <$(FETCHED))),$(strip $(shell $(sh4zam_sums))))
UNFETCHED  := $(SH4ZAM)
endif
DOWNLOADS  += $(filter-out $(wildcard $(PINNED_GCC) $(filter-out $(UNFETCHED),$(SH4ZAM))), \
	$(if $(filter dc dc-disc dc-check dc-order,$(MAKECMDGOALS)),$(PINNED_GCC) $(SH4ZAM)))
# The toolchain's sources unpack with tar, one that gzip compressed and four
# that xz did, and build with c++. Where the driver is a download, UNPACKED
# has the host checked for tar, and for gzip where tar runs it, and the check
# here asks for xz where tar runs that. make dc, dc-disc and dc-order check
# each image's content with cmp, and make dc-check builds no content
UNPACKED   += %/sh-elf-gcc
$(call check_host,$(if $(filter %/sh-elf-gcc,$(DOWNLOADS)),$(call unpacker,liblzma,xz) c++) \
	$(if $(filter dc dc-disc dc-order,$(MAKECMDGOALS)),cmp))

# make reads config/gcc.mk, GCC's flags, and config/dreamcast.mk, the
# port's composition, for DC_GOALS alone, so a fault in either stops only a
# run that names one of them. For a goal under tools/sh-elf/ or
# tools/sh4zam/, make reads dc.mk and neither of them
ifneq ($(DC_GOALS),)
include config/gcc.mk config/dreamcast.mk
endif

# The toolchain make builds has a receipt, request.txt, which its recipe
# writes last: the sources, their lines of tools/SHA256SUMS, the host and
# each configure's flags. SH_ELF_REQUEST is the receipt this file's recipe
# writes. The tool stamps hold what the binaries are, and the receipt what
# they were built from and on which host, so make refuses a toolchain whose
# receipt differs, as one copied from a host uname names otherwise does, and
# leaves it in place. It refuses before it refreshes a stamp. A toolchain
# another recipe built can lack a file the stamps hash, as one built without
# link-time optimization lacks lto1 and the linker's plugin, and the refresh
# would stop make on that file and name no remedy
SH_ELF_SOURCES := binutils/binutils-2.47.tar.xz gcc/gcc-16.2.0/gcc-16.2.0.tar.xz gmp/gmp-6.3.0.tar.xz \
	mpc/mpc-1.3.1.tar.gz mpfr/mpfr-4.2.1.tar.xz
# Each configure links the host's zstd where it finds one, and --without-zstd
# has it look for none, so the toolchain's programs need no library the host
# may lack. --enable-deterministic-archives has ar write each member's date,
# owner and group as zero and its mode as 644 unless a command says
# otherwise, and GCC's build archives libgcc.a with that ar, so libgcc.a
# holds no time or user of its build. make dc's C objects carry GCC's
# intermediate code, which its link compiles, as config/gcc.mk's GCC_LTO
# says: --enable-lto has GCC build what writes and compiles that code, lto1
# and the linker's plugin among it, and --enable-plugins has the linker
# load the plugin, and fails binutils' configure on a host where it can't
SH_ELF_BINUTILS := --target=sh-elf --disable-nls --disable-werror --without-zstd --enable-deterministic-archives \
	--enable-plugins
SH_ELF_GCC := --target=sh-elf --enable-languages=c --with-newlib --without-headers --with-endian=little \
	--with-cpu=m4-single-only --disable-multilib --disable-nls --disable-shared --disable-threads --enable-lto \
	--without-isl --without-zstd --disable-libssp --disable-libquadmath --disable-libgomp
# GCC compiles the target's libraries with debugging information, which names
# the directory of each compile. $(call sh_elf_map,CHECKOUT) is the flag that
# has it compile them with -fdebug-prefix-map=CHECKOUT=., which writes . there
# wherever a path opens with CHECKOUT. The configure takes the flag with the
# checkout's path, so libgcc.a holds no path of the host, and is the same
# file wherever the checkout lies. The receipt takes it with the word CURDIR:
# it holds each flag that shapes libgcc.a, and is one text in every checkout
# of a host
sh_elf_map = --with-debug-prefix-map=$1=.
SH_ELF_REQUEST := $(strip $(SH_ELF_SOURCES) $(shell awk '$$2 ~ "^tools/sh-elf.src/"' tools/SHA256SUMS) \
	host $(HOST) binutils $(SH_ELF_BINUTILS) gcc $(SH_ELF_GCC) $(call sh_elf_map,CURDIR))
ifneq ($(wildcard $(PINNED_GCC)),)
ifneq ($(strip $(file <tools/sh-elf/request.txt)),$(SH_ELF_REQUEST))
$(error tools/sh-elf: its request.txt names another build than dc.mk's; remove tools/sh-elf, and make builds it again)
endif
endif

# GCC's stamp holds the hashes of its driver, its C compiler, cc1, and its
# assembler
GCC_ID := build/tools/gcc.id
$(eval $(call stamp,gcc,$(SH_ELF)/bin/sh-elf-gcc,$(SH_ELF)/bin/sh-elf-gcc $(SH_ELF)/libexec/gcc/sh-elf/*/cc1 $(SH_ELF)/bin/sh-elf-as,$(DC_GOALS)))
# The linker, the archiver and libgcc.a take a stamp of their own. So do
# LINK_TIME_TOOLS, the programs of GCC's that only the link runs, for its
# link-time compile: lto1, which compiles the intermediate code make dc's C
# objects carry, lto-wrapper and collect2, through which the driver runs
# lto1 and the linker, and the plugin by which the linker hands GCC that
# code. A change to one rebuilds only each image's content object, the
# archive, the link and make dc-check's sums. GCC's version is part of the
# paths of cc1, of LINK_TIME_TOOLS and of libgcc.a, so the stamps name them
# by a shell glob, and LIBGCC is libgcc.a's. A refresh expands the glob as it
# hashes, once the toolchain is in place, and fails where it matches no
# file. A toolchain built without link-time optimization holds no lto1 and
# no plugin. Where SH_ELF's driver is in place and a program of
# LINK_TIME_TOOLS is not, make stops ahead of the refresh, and names each
# such program and what settles it: for SH_ELF, a GCC configured with
# --enable-lto, and for tools/sh-elf, whose receipt is this file's and
# which has lost the program since, its removal, so that make builds it again
LD_ID  := build/tools/ld.id
LIBGCC := $(SH_ELF)/lib/gcc/sh-elf/*/libgcc.a
LINK_TIME_TOOLS := lto1 lto-wrapper collect2 liblto_plugin.so
ifneq ($(DC_GOALS),)
ifneq ($(wildcard $(SH_ELF)/bin/sh-elf-gcc),)
LINK_TIME_MISSING := $(strip $(foreach t,$(LINK_TIME_TOOLS),$(if $(wildcard $(SH_ELF)/libexec/gcc/sh-elf/*/$t),,$t)))
ifneq ($(LINK_TIME_MISSING),)
$(error $(SH_ELF): its libexec/gcc/sh-elf/*/ holds no $(LINK_TIME_MISSING), which make dc's link-time compile runs; $(if \
	$(PINNED_GCC),remove tools/sh-elf$(comma) and make builds it again,SH_ELF must name a toolchain whose GCC was \
	configured with --enable-lto))
endif
# GCC's version is part of the paths of its own headers, of cc1 and of
# libgcc.a, which the rules name by a glob that takes one version
ifneq ($(word 2,$(wildcard $(SH_ELF)/lib/gcc/sh-elf/*/include)),)
$(error $(SH_ELF): its lib/gcc/sh-elf/ holds more than one version of GCC, $(patsubst $(SH_ELF)/lib/gcc/sh-elf/%/include,%,$(wildcard \
	$(SH_ELF)/lib/gcc/sh-elf/*/include)), and make dc takes a toolchain of one)
endif
endif
endif
$(eval $(call stamp,ld,$(SH_ELF)/bin/sh-elf-gcc,$(SH_ELF)/bin/sh-elf-ld $(SH_ELF)/bin/sh-elf-ar $(LIBGCC) \
	$(LINK_TIME_TOOLS:%=$(SH_ELF)/libexec/gcc/sh-elf/*/%),$(DC_GOALS)))

# make dc's objects lie under build/dc/, each at its source's path. GCC
# compiles each of the tree's C sources but the grafted source's donor,
# whose four functions only SHC's images hold, and each image's content, the
# data of its gen/ ranges. config/dreamcast.mk composes the rest. An entry
# P.src:C of DC_REPLACED compiles C to build/dc/P.o, the object P.src would
# assemble to, so C replaces P.src. An entry P.c:C replaces P.c alike, P.c
# one of ASMCODE's sources, whose functions are inline assembly. An entry
# IMAGE:C of DC_ADDED compiles C to build/dc/C.o, which IMAGE links after
# its sources'. GNU as assembles each .src file that no entry of DC_REPLACED
# names. No SHC builds C either names
DC_REPLACED_SRC := $(foreach r,$(DC_REPLACED),$(word 1,$(subst :, ,$r)))
DC_REPLACED_OBJ := $(patsubst %.c,build/dc/%.o,$(DC_REPLACED_SRC:%.src=build/dc/%.o))
DC_ADDED_C := $(foreach a,$(DC_ADDED),$(word 2,$(subst :, ,$a)))
DC_ADDED_OBJ := $(DC_ADDED_C:%.c=build/dc/%.o)
DC_PAIRED_OBJ := $(DC_REPLACED_OBJ) $(DC_ADDED_OBJ)
# each object of DC_PAIRED_OBJ and its C, as OBJECT:C
DC_PAIRS := $(join $(DC_REPLACED_OBJ),$(patsubst %,:%,$(foreach r,$(DC_REPLACED),$(word 2,$(subst :, ,$r))))) \
	$(join $(DC_ADDED_OBJ),$(DC_ADDED_C:%=:%))
# $(call gcc_of,OBJECTS) gives the GCC object of each of OBJECTS, which are
# SHC's objects under build/. It leaves out each whose source holds inline
# assembly, which the split's rules build
gcc_of   = $(patsubst build/%.obj,build/dc/%.o,$(filter-out $(INLINE_C:%.c=build/%.obj),$1))
DC_C_OBJ := $(call gcc_of,$(C_SOURCE_OBJ))
DC_INLINE_OBJ := $(filter-out $(DC_REPLACED_OBJ),$(patsubst %.c,build/dc/%.o,$(filter-out $(DONOR_C),$(INLINE_C))))
DC_ASM_OBJ := $(filter-out $(DC_REPLACED_OBJ),$(ASM_SRC:%.src=build/dc/%.o))
# DC_LTO_OBJ is the objects that carry GCC's intermediate code, under
# config/gcc.mk's GCC_LTO: each C source's and each of DC_PAIRED_OBJ, but
# the object of each source a row of a kernel table names, which keeps its
# own code. The kernels' windows and the warm rows' order are laid for that
# code, and code the link-time compile inlined into a kernel would take it
# past its window. A last or cold row's object keeps its code where the row
# puts it, so that a change to it moves no code ahead of it while it takes
# the same names of the compiled C, as config/1st_read/kernel.txt says, and
# a data row's keeps its section. DC_KERNEL_SOURCES is those sources: the
# second field of each row but an over row's and a window row's, which name
# kernels, and a starts row's, which names an output section. A packed row
# names nothing. The object of a source config/gcc.mk's GCC_APART names
# keeps its own code too
DC_KERNEL_SOURCES := $(shell awk '{ sub(/#.*/, "") } NF > 1 && $$1 !~ /^(over|window|starts|packed)$$/ { print $$2 }' \
	/dev/null $(wildcard config/*/kernel.txt))
DC_LTO_OBJ := $(filter-out $(addprefix build/dc/,$(addsuffix .o,$(basename $(DC_KERNEL_SOURCES) $(GCC_APART)))), \
	$(DC_C_OBJ) $(DC_PAIRED_OBJ))
$(DC_LTO_OBJ): private GCCFLAGS += $(GCC_LTO)
# build/dc/rows.txt records DC_KERNEL_SOURCES. No object depends on a kernel
# table, so that an edit to a row compiles nothing. Whether a C object takes
# GCC_LTO follows the tables all the same, so each depends on the record, and
# compiles again once a source joins or leaves the rows: an incremental
# build then holds the objects a clean one would. The Makefile's record
# dates a new file the epoch, so that its first write rebuilds nothing.
# With no record, though, nothing says which rows the objects were compiled
# under, so make writes a missing one itself, dated now, and each C object
# compiles again: a tree with no object loses nothing by it. tools/ld.py
# fails a link on an object that carries the code where a row names it
ifneq ($(DC_GOALS),)
ifeq ($(wildcard build/dc/rows.txt),)
$(shell mkdir -p build/dc)$(file >build/dc/rows.txt,$(sort $(DC_KERNEL_SOURCES)))
else
$(call record,build/dc/rows.txt,$(sort $(DC_KERNEL_SOURCES)))
endif
endif
$(DC_C_OBJ) $(DC_PAIRED_OBJ): build/dc/rows.txt
DC_CONTENT_OBJ := $(GEN_CONF:%=build/dc/gen/%/content.o)
# SH4ZAM's library is its SH-4 back end, source/sh4/, which its headers take
# under -m4-single-only, as config/gcc.mk says. The back end defines each
# function the headers declare out of line under SHZ_BACKEND == SHZ_SH4, but
# shz_version.h's two, which no object calls. The software back end,
# source/sw/, stays out of the library: it calls sqrtf, fabsf, sinf and
# cosf, which the SDK's math.h doesn't declare. DC_SH4ZAM_C_OBJ is the back
# end's C objects, which alone of SH4ZAM's objects write .d files
SH4ZAM_SOURCES := $(filter tools/sh4zam/source/sh4/%,$(SH4ZAM))
DC_SH4ZAM_OBJ := $(patsubst tools/sh4zam/source/%,build/dc/sh4zam/%.o,$(basename $(SH4ZAM_SOURCES)))
DC_SH4ZAM_C_OBJ := $(patsubst tools/sh4zam/source/%.c,build/dc/sh4zam/%.o,$(filter %.c,$(SH4ZAM_SOURCES)))
# Each image's link takes from the archive the members its objects call
DC_SH4ZAM_LIB := build/dc/sh4zam/libsh4zam.a
# DC_OBJ is every object make dc builds, from the lists DC_OBJ_LISTS names
DC_OBJ_LISTS := DC_C_OBJ DC_PAIRED_OBJ DC_INLINE_OBJ DC_ASM_OBJ DC_CONTENT_OBJ DC_SH4ZAM_OBJ
DC_OBJ := $(foreach l,$(DC_OBJ_LISTS),$($l))
# Stop on an entry of DC_REPLACED that isn't P.src:C or P.c:C, P.src a
# .src file an image places, P.c one of ASMCODE's and C a C source, and on
# one of DC_ADDED that isn't IMAGE:C, C a C source. Stop too where an added
# C compiles to another source's object
ifneq ($(DC_GOALS),)
UNREPLACED := $(strip $(foreach r,$(DC_REPLACED),$(if $(and $(filter 2,$(words $(subst :, ,$r))),\
	$(filter $(ASM_SRC) $(ASMCODE),$(word 1,$(subst :, ,$r))),$(filter %.c,$(wildcard $(word 2,$(subst :, ,$r))))),,$r)))
ifneq ($(UNREPLACED),)
$(error $(UNREPLACED): an entry of DC_REPLACED, and not P.src:C or P.c:C, P.src a .src file an image places, \
	P.c one of ASMCODE's and C a C source)
endif
UNADDED := $(strip $(foreach a,$(DC_ADDED),$(if $(and $(filter 2,$(words $(subst :, ,$a))),\
	$(filter %.c,$(wildcard $(word 2,$(subst :, ,$a))))),,$a)))
ifneq ($(UNADDED),)
$(error $(UNADDED): an entry of DC_ADDED, and not IMAGE:C, C a C source)
endif
OVERLAID := $(filter $(DC_C_OBJ) $(DC_INLINE_OBJ) $(ASM_SRC:%.src=build/dc/%.o),$(DC_ADDED_OBJ))
ifneq ($(OVERLAID),)
$(error $(OVERLAID:build/dc/%.o=%.c): DC_ADDED names it, and it compiles to a source's object)
endif
# Stop on an object two of the lists give, as two of SH4ZAM's sources of one
# stem would: make would build it by one rule alone. A replacement's object
# is no such object, since DC_ASM_OBJ leaves it out
TWICE := $(shell printf '%s\n' $(DC_OBJ) | sort | uniq -d)
ifneq ($(TWICE),)
$(error $(foreach o,$(TWICE),$o: two of make dc's rules build it, by $(strip $(foreach l,$(DC_OBJ_LISTS),\
	$(if $(filter $o,$($l)),$l)))))
endif
endif

# $(call shc_image_obj,IMAGE) gives the SHC object of each source IMAGE's
# splits.txt places
shc_image_obj = $(foreach l,$(filter build/$1/%,$(LINKS)),$(call object,$(l:build/%.bin=%)))
# $(call gcc_image_obj,IMAGE) gives IMAGE's GCC objects: one for each source
# its splits.txt places, with its content in place of its gen/ ranges'.
# DC_LINKED is the GCC objects make dc links: every image's, and DC_ADDED's
gcc_image_obj = $(patsubst build/%.obj,build/dc/%.o,$(filter-out $(GEN_OBJ),$(call shc_image_obj,$1))) \
	$(filter build/dc/gen/$1/%,$(DC_CONTENT_OBJ))
DC_LINKED := $(sort $(DC_ADDED_OBJ) $(foreach i,$(sort $(foreach l,$(LINKS),$(call image,$(l:build/%=%)))),\
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
# place and finds its quoted includes in the source's directory. With RELEASE,
# GCC's preprocessor writes $@'s .i, which tools/floats.py rewrites for
# RELEASE and the flags of FLAGS it reads, -fpu=single, -double=float and
# -aggressive=2. The pass gives the .i's floating constants and static
# initializers SHC's values, and on r26 and r32 wraps the products and writes
# the += that tools/float_fuse.py names, so that GCC fuses as SHC does. It
# leaves the lines of SH4ZAM's headers, under SH4ZAM_INCLUDE, as they are. GCC
# then compiles the .i. A source no SHC builds names no RELEASE, and compiles
# in one run, with no float pass. GCC fuses no product into a sum in
# DC_PAIRED_OBJ's sources, under GCCFLAGS' -ffp-contract=off, and fuses where
# it can in SH4ZAM's library, under GNU C's default, -ffp-contract=fast. GCC
# runs in the C locale with plain diagnostics, so its log reads the same on
# any host. It writes the .d file as a .tmp, which replaces the last one once
# the compile passes, so make reads the last whole one after a compile that
# fails. The .i goes last, once the .d file is in place, so it stays for a
# compile that fails
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

# The root, searched last, lets a source of DC_PAIRED_OBJ's name a header by
# its path from the root, as it names engine/dc/njsprite.h and
# sdk/ninja/njkamui.h
DC_INC    := sdk/include,src,.
# A movie SDK member with inline assembly sees the SDK's headers under GCC
# too, as under SHC
$(patsubst %.c,build/dc/%.o,$(filter $(MOVIE_SDK),$(ASMCODE))): ASMCODE_INC := $(SDK_INC)

# make dc builds every GCC object, and links the images
dc: $(DC_OBJ) build/dc/manifest.txt

# build/dc/pairs.txt records DC_PAIRS. make rewrites it as it reads dc.mk,
# where it is missing or differs, and only then. An object of DC_PAIRED_OBJ's
# rule names its C alone, and DC_ASM_OBJ's and the split's name P.src or P.c
# alone. So once an entry of DC_REPLACED joins or leaves, or names another C,
# build/dc/P.o would look up to date by the rule that builds it now. Each
# object of those rules depends on the record too, and builds again once the
# record changes. A moved object's .d file can name a source that is gone,
# which make can't remake, so make removes it. With no record, every object of
# DC_PAIRED_OBJ's counts as moved
ifneq ($(DC_GOALS),)
DC_KEPT := $(if $(wildcard build/dc/pairs.txt),$(sort $(file <build/dc/pairs.txt)),none)
ifneq ($(DC_KEPT),$(sort $(DC_PAIRS)))
DC_MOVED := $(filter build/dc/%.o,$(foreach p,$(filter-out $(DC_KEPT),$(DC_PAIRS)) \
	$(filter-out $(DC_PAIRS),$(DC_KEPT)),$(firstword $(subst :, ,$p))))
$(shell mkdir -p build/dc && rm -f $(DC_MOVED:.o=.d) && echo $(sort $(DC_PAIRS)) > build/dc/pairs.txt.tmp && \
	mv build/dc/pairs.txt.tmp build/dc/pairs.txt)
ifneq ($(.SHELLSTATUS),0)
$(error build/dc/pairs.txt: make can't remove a moved object's .d file or write the record)
endif
endif
$(DC_PAIRED_OBJ) $(DC_ASM_OBJ) $(DC_INLINE_OBJ): build/dc/pairs.txt
endif
# DC_COMPILED is the objects GCC compiles, each from C. Each depends on its
# .d file, which keeps its header dependencies, as an SHC object's does, and
# build/dc/headers.txt holds the headers of the engine's directories a
# compile here searches, engine/include/, engine/sa1/, engine/dc/ and
# engine/dc/sa1/, as build/headers.txt holds those under ROOTS
DC_COMPILED := $(DC_C_OBJ) $(DC_PAIRED_OBJ) $(DC_INLINE_OBJ) $(DC_CONTENT_OBJ) $(DC_SH4ZAM_C_OBJ)
$(DC_COMPILED): %.o: %.d
$(DC_COMPILED:.o=.d): ;
-include $(DC_COMPILED:.o=.d)
ifneq ($(DC_GOALS),)
$(call record,build/dc/headers.txt,$(sort $(wildcard engine/include/*.h engine/sa1/*.h engine/dc/*.h \
	engine/dc/sa1/*.h)))
endif
$(DC_COMPILED): build/headers.txt build/dc/headers.txt

# A GCC object depends on the files that give its command: dc.mk and
# config/gcc.mk, beside an SHC object's. Each whose .i the float pass reads
# depends on the Makefile's FLOATS too: the pass, its modules and the
# modules they import
$(DC_OBJ): Makefile config/compilers.mk dc.mk config/gcc.mk config/dreamcast.mk
$(DC_C_OBJ) $(DC_INLINE_OBJ) $(DC_CONTENT_OBJ): $(FLOATS)

# GCC compiles each C source with its route's include directories, flags and
# release. Each waits for SH4ZAM's files, whose headers the non-matching code
# includes
$(call gcc_of,$(R32_GAME_OBJ)): build/dc/%.o: %.c $(GCC_ID) | $(FETCHED)
	@$(call status,gcc,$<)
	$(Q)$(call gcc,$(GAME_INC),$(GAME),r32)

$(call gcc_of,$(R10_SDK_OBJ)): build/dc/%.o: %.c $(GCC_ID) | $(FETCHED)
	@$(call status,gcc,$<)
	$(Q)$(call gcc,$(SDK_INC),$(SDK),r10)

$(call gcc_of,$(R32_RUNTIME_OBJ)): build/dc/%.o: %.c $(GCC_ID) | $(FETCHED)
	@$(call status,gcc,$<)
	$(Q)$(call gcc,$(R32_SDK_INC),$(RUNTIME),r32)

$(call gcc_of,$(PATCHED_OBJ)): build/dc/%.o: %.c $(GCC_ID) | $(FETCHED)
	@$(call status,gcc,$<)
	$(Q)$(call gcc,$(PATCHED_INC),$(SDK),r10)

$(call gcc_of,$(R26_SDK_OBJ)): build/dc/%.o: %.c $(GCC_ID) | $(FETCHED)
	@$(call status,gcc,$<)
	$(Q)$(call gcc,$(R26_SDK_INC),$(SDK),r26)

$(call gcc_of,$(R32_SDK_OBJ)): build/dc/%.o: %.c $(GCC_ID) | $(FETCHED)
	@$(call status,gcc,$<)
	$(Q)$(call gcc,$(R32_SDK_INC),$(SDK),r32)

# No SHC builds a source of DC_PAIRED_OBJ's, so it takes none of SHC's flags
# and no release
define dc_object
$1: $2 $$(GCC_ID) | $$(FETCHED)
	@$$(call status,gcc,$$<)
	$$(Q)$$(call gcc,$$(DC_INC))
endef
$(foreach p,$(DC_PAIRS),$(eval $(call dc_object,$(firstword $(subst :, ,$p)),$(lastword $(subst :, ,$p)))))

# SH4ZAM's C compiles under GCC_SH4ZAM_FLAGS, which config/gcc.mk gives its
# objects, and with no release. It takes assert.h, math.h and string.h from
# the SDK's headers: a freestanding GCC has none of them
build/dc/sh4zam/%.o: tools/sh4zam/source/%.c $(GCC_ID) | $(FETCHED)
	@$(call status,gcc,$<)
	$(Q)$(call gcc,sdk/include)

# GNU as assembles SH4ZAM's assembly as it is, little-endian under GAS_CHECKS,
# not GASFLAGS. It pads each section to its alignment, which GASFLAGS'
# --no-pad-sections would not: make dc's 1ST_READ.BIN and LOADER.BIN hold the
# 7 NOPs, 14 bytes, that pad .text._shz_memcpy32_sh4_ to a multiple of 32. The
# assembly includes no file, so it waits for no fetch but its own
build/dc/sh4zam/%.o: tools/sh4zam/source/%.s $(GCC_ID)
	@$(call status,gas,$<)
	$(Q)mkdir -p $(@D) && $(SH_ELF)/bin/sh-elf-as $(GAS_CHECKS) --little -o $@ $<

# make removes the archive before each write, so it holds no member another
# pin left. ar's D records no dates or user ids, so the same objects make the
# same archive. Its members' inventory writes it again when a pin drops one
ifneq ($(DC_GOALS),)
$(call record,build/dc/sh4zam/members.txt,$(sort $(DC_SH4ZAM_OBJ)))
endif
$(DC_SH4ZAM_LIB): $(DC_SH4ZAM_OBJ) build/dc/sh4zam/members.txt $(LD_ID)
	@$(call status,ar,$@)
	$(Q)mkdir -p $(@D) && rm -f $@ && $(SH_ELF)/bin/sh-elf-ar rcsD $@ $(DC_SH4ZAM_OBJ)

# Two checks read a .src file before build/shckit translates it for GNU as.
# tools/tags.py --abi refuses a function tagged @asm abi whose name the
# object's CONTRACTS lacks. tools/floats.py --assembly refuses an .FDATA.S
# constant that asmsh and GNU as can read as two floats, as
# tools/float_asm.py says
$(DC_ASM_OBJ): build/dc/%.o: %.src build/shckit tools/tags.py tools/csource.py $(FLOATS) tools/layout.py \
		$(CONFIG_TOOLS) $(TABLES) $(GCC_ID)
	@$(call status,gas,$<)
	$(Q)mkdir -p $(@D)
	$(Q)python3 tools/tags.py --abi $(CONTRACTS:%=--contract %) $<
	$(Q)python3 tools/floats.py --assembly $<
	$(Q)build/shckit translate -o build/dc/$*.s $<
	$(Q)$(SH_ELF)/bin/sh-elf-as $(GASFLAGS) -o $@ build/dc/$*.s

# A source with inline assembly passes the same checks. tools/inline.py
# splits it into C and the text of its inline assembly, which build/shckit
# translates for the C's file-scope asm to include. Where every block lies on
# the matching side of a guard, the text is empty. shckit refuses an empty text,
# so the split translates none, and the C includes none
split = mkdir -p $(@D) && python3 tools/tags.py --abi $(CONTRACTS:%=--contract %) $< && \
	python3 tools/inline.py $< build/dc/$*.c build/dc/$*_inline.src && \
	python3 tools/floats.py --assembly build/dc/$*_inline.src && \
	if [ -s build/dc/$*_inline.src ]; then build/shckit translate -o build/dc/$*_inline.s build/dc/$*_inline.src; \
	else rm -f build/dc/$*_inline.s; fi
SPLIT_NEEDS := tools/inline.py $(FLOATS) tools/tags.py tools/csource.py tools/layout.py $(CONFIG_TOOLS) \
	$(TABLES) build/shckit $(GCC_ID)

$(filter-out $(DC_REPLACED_OBJ),$(ASMCODE:%.c=build/dc/%.o)): build/dc/%.o: %.c $(SPLIT_NEEDS) | $(FETCHED)
	@$(call status,gcc,$< (split))
	$(Q)$(split)
	$(Q)$(call gcc,$(ASMCODE_INC),$(SDK),r10,build/dc/$*.c)

$(patsubst %.c,build/dc/%.o,$(filter $(INLINE_C),$(PATCHED_C))): build/dc/%.o: %.c $(SPLIT_NEEDS) | $(FETCHED)
	@$(call status,gcc,$< (split))
	$(Q)$(split)
	$(Q)$(call gcc,$(PATCHED_INC),$(SDK),r10,build/dc/$*.c)

$(patsubst %.c,build/dc/%.o,$(filter $(INLINE_C),$(GRAFTED_C))): build/dc/%.o: %.c $(SPLIT_NEEDS) | $(FETCHED)
	@$(call status,gcc,$< (split))
	$(Q)$(split)
	$(Q)$(call gcc,$(SDK_INC),$(SDK),r10,build/dc/$*.c)

# An empty recipe lets a split C go missing, though its object's .d file names
# it: the object then splits its source again
$(DC_INLINE_OBJ:.o=.c): ;

# GCC compiles an image's content under the game's flags, and the float pass
# reads its constants as R32 reads a gen/ range's C. GNU ld then links it as
# content.ld says: each name it defines, and each it uses from elsewhere, at
# its address in the image. cmp holds the link's bytes to content.bin, the
# image's. The content writes each pointer into RAM as a name, which the link
# resolves, so the check holds those pointers too
$(DC_CONTENT_OBJ): build/dc/%/content.o: build/%/content.c $(GCC_ID) $(LD_ID) | $(FETCHED)
	@$(call status,gcc,build/$*/content.c)
	$(Q)$(call gcc,$(GEN_INC),$(GAME),r32)
	$(Q)$(SH_ELF)/bin/sh-elf-ld -m shlelf --orphan-handling=error --fatal-warnings --oformat=binary \
		-T build/$*/content.ld -o $(@:.o=.bin) $@ > $(@:.o=.bin).log 2>&1 || (cat $(@:.o=.bin).log; false)
	$(Q)cmp -s $(@:.o=.bin) build/$*/content.bin || \
		(echo "$@: linked at the disc's addresses, it differs from build/$*/content.bin"; false)

# MEMORY_SOURCES is the sources tools/memory.py reads, as its SOURCES lists
# them. The link depends on each, so a change to one links again, and a
# missing one stops make even where the manifest is current
MEMORY_SOURCES := $(if $(DC_GOALS),$(call from_tools,__import__("memory").SOURCES))
# DC_PUBLISHED is what the link writes before the manifest: each image's
# final ELF, and its file under build/dc/ at its original's path, with a .PRS
# file's image beside it as a .BIN. An empty recipe lets a published file go
# missing without stopping make. The manifest depends on each, so the link
# then runs again
DC_PUBLISHED := $(if $(DC_GOALS),$(call from_tools,(each for name, path in images.files().items() \
	for each in (f"build/dc/link/{name}/final.elf", f"build/dc/{path}", \
	*([f"build/dc/{path[:-4]}.BIN"] if path.endswith(".PRS") else [])))))
$(DC_PUBLISHED): ;
# $(call dc_spec,DIR) plans make dc's link: tools/spec.py writes DIR/spec.txt,
# and each name nothing binds to DIR/unbound.txt. It takes
# config/dreamcast.mk's DC_STARTS as --starts, where that names any
dc_spec = SH_ELF=$(SH_ELF) python3 tools/spec.py --archive $(DC_SH4ZAM_LIB)$(if \
	$(DC_ADDED), $(subst :, build/dc/,$(DC_ADDED:%.c=--add %.o)))$(if \
	$(DC_STARTS), --starts $(DC_STARTS)) --list $1/unbound.txt $1/spec.txt
# make dc's link. tools/spec.py plans it, writing the spec by which
# tools/ld.py links each image. The link-time compile's flags reach it as
# one word, --lto=FLAGS, so that a FLAGS of one flag is no option of
# ld.py's own. tools/verify.py checks the final links.
# Then tools/publish.py writes the images' files, and the manifest
# last. The link waits for the census and the import check, so a disc comes
# only from a link both passed. The manifest doesn't depend on either check's
# date, so a check that runs again and passes links nothing again. The link
# first removes the manifest and the disc made from it, so a link that fails
# or stops leaves neither. A check or an object that fails before the link
# leaves both as they were, and make's status says the run failed
build/dc/manifest.txt: $(DC_LINKED) $(DC_SH4ZAM_LIB) $(MEMORY_SOURCES) $(DC_PUBLISHED) \
		config/imports.txt config/declarations.txt $(TABLES) tools/spec.py tools/ld.py tools/verify.py \
		tools/decls.py tools/dwarf.py tools/publish.py tools/manifest.py tools/gcclink.py tools/elf.py \
		$(MEMORY_TOOLS) build/prskit $(LD_ID) | \
		build/dc/census.ok build/dc/imports.ok
	@$(call status,link,build/dc/link)
	$(Q)rm -rf $@ build/dc/disc build/dc/disc.partial
	$(Q)mkdir -p build/dc/link
	$(Q)$(call dc_spec,build/dc/link)
	$(Q)SH_ELF=$(SH_ELF) python3 tools/ld.py --lto='$(GCC_LTO_LINK)' build/dc/link/spec.txt build/dc/link
	$(Q)SH_ELF=$(SH_ELF) python3 tools/verify.py --abi='$(GCC_ABI)' build/dc/link/spec.txt build/dc/link
	$(Q)SH_ELF=$(SH_ELF) python3 tools/publish.py build/dc/link/spec.txt build/dc/link $@

# make dc-order writes config/1st_read/order.txt, the record of where each
# of the kernel table's warm rows lies, which make dc's link lays the rows
# by and holds itself to, for the objects as they are built. It moves no
# warm row that fits where the record has it, and gives a row that fits no
# longer, or a new one, a place, and writes the table's warm rows in their
# places' order. With SEARCH=1 it searches the whole block too, and writes
# what it finds where that costs less, which moves every row and is a
# change to measure. With PACK=1 it lays the rows in the table's order
# with no byte free among them, which frees the most room in the image,
# moves each row after the first free byte, and is a change to measure
# too. Only the command line's SEARCH and PACK ask for these, so that none
# in the environment moves a row. It plans the link as make dc does,
# into a spec of its own, by which tools/order.py links the layout pass it
# reads, so it needs no link of make dc's, which a record out of date
# fails. It checks nothing: the make dc that follows links by the tables
# it wrote
DC_SEARCH := $(if $(filter command line,$(origin SEARCH)),$(if $(filter-out 0,$(SEARCH)), --search))
DC_PACK   := $(if $(filter command line,$(origin PACK)),$(if $(filter-out 0,$(PACK)), --pack))
dc-order: $(DC_LINKED) $(DC_SH4ZAM_LIB)
	@$(call status,order,config/1st_read/kernel.txt)
	$(Q)mkdir -p build/dc/order
	$(Q)$(call dc_spec,build/dc/order)
	$(Q)SH_ELF=$(SH_ELF) python3 tools/order.py$(DC_SEARCH)$(DC_PACK) build/dc/order/spec.txt build/dc/order

# make dc-disc builds its disc from the one .gdi in orig/, the Makefile's
# GDI_IMAGE, and its tracks
ifneq ($(filter dc-disc,$(MAKECMDGOALS)),)
ifneq ($(FOUND),1)
$(error dc-disc needs the disc's .gdi and its tracks in orig/, and orig/ holds $(FOUND) .gdi files)
endif
endif
dc-disc: build/dc/manifest.txt build/gdi
	@$(call status,disc,build/dc/disc)
	$(Q)rm -rf build/dc/disc build/dc/disc.partial
	$(Q)python3 tools/disc.py $(call quoted,$(GDI_IMAGE)) build/dc/manifest.txt build/dc/disc

# The census check. The link lays out 1ST_READ.BIN anew, so it waits for the
# check. Its first pass, census.py --generated --disc, reads each decoded
# motion file and the files 1ST_READ.BIN's SHC build reads. CENSUSED stands
# for those files. For each source 1ST_READ.BIN's splits.txt places, which
# FIRST_READ_OBJ names by its SHC object, it holds a .src file itself; a C
# source's reads, which depend on the source and on each file it includes;
# a gen/ range's C; and a body source's bodies.txt, which stands for the
# bodies make writes from the disc. The check's --dc pass reads every image's
# non-matching build, so the check waits for each object the link takes.
# That pass reads the C sources' reads too, to tell which rows of census.txt
# the first pass counts, and counts the others. The check can't run without
# census.txt, which it names, so a deleted census.txt stops make
MOTION_DECODED := $(patsubst SONICADV/%.PRS,build/gen/img/%.BIN,$(MOTIONS))
FIRST_READ_OBJ := $(call shc_image_obj,1st_read)
CENSUSED := $(filter $(ASM_SRC),$(FIRST_READ_OBJ:build/%.obj=%.src)) \
	$(filter $(READS),$(FIRST_READ_OBJ:build/%.obj=build/reads/%.d)) \
	$(patsubst %.obj,%.c,$(filter $(GEN_OBJ),$(FIRST_READ_OBJ))) \
	$(patsubst %.c,build/gen/%/bodies.txt,$(filter $(BODY_C),$(FIRST_READ_OBJ:build/%.obj=%.c)))
build/dc/census.ok: $(CENSUSED) $(MOTION_DECODED) $(DC_LINKED) config/1st_read/census.txt $(TABLES) \
		tools/census.py tools/content.py tools/stages.py tools/memory.py tools/bodies.py $(DATA_TOOLS) tools/gcclink.py tools/elf.py
	@$(call status,census,config/1st_read/census.txt)
	$(Q)python3 tools/census.py --generated --disc
	$(Q)python3 tools/census.py --dc$(if $(DC_REPLACED), $(subst :, ,$(DC_REPLACED:%=--replace %)))$(if \
		$(DC_ADDED), $(subst :, ,$(DC_ADDED:%=--add %)))
	$(Q)mkdir -p $(@D) && touch $@

# The import check. SHC's build links each of 1ST_READ.BIN's imports at the
# address its image's symbols.txt gives. make dc's link binds each by name
# instead, so tools/imports.py --uses holds each use of an import in
# 1ST_READ.BIN's C to the images its stages read, and each import's address to
# a row of its image's splits.txt. It reads the tables and C alone, so it
# needs neither the disc nor SHC and waits for no object, and make dc-check
# runs it as make dc does. The check's other half, tools/imports.py
# --definitions, which holds each import to its definition in an object SHC or
# asmsh writes, is make check's. The rule names what --uses reads: every C
# file under ROOTS, 1ST_READ.BIN's sources and what they include among them,
# the C of DC_PAIRS, which DC_REPLACED and DC_ADDED give make dc, and
# engine/dc/'s headers, which that C includes. A file the check came to read
# from elsewhere would need naming here. A header that joins or leaves can
# change the file an #include finds, so the rule names the headers' two
# inventories. The C of an entry that joins DC_PAIRS is older than the last
# check, so the rule names build/dc/pairs.txt, which is not, and it names
# dc.mk and config/dreamcast.mk, which give the check's command its operands
build/dc/imports.ok: $(C_FILES) $(foreach p,$(DC_PAIRS),$(lastword $(subst :, ,$p))) $(wildcard engine/dc/*.h) \
		build/headers.txt build/dc/headers.txt build/dc/pairs.txt dc.mk config/dreamcast.mk $(TABLES) \
		tools/imports.py $(CONFIG_TOOLS) tools/stages.py tools/csource.py tools/shc_object.py tools/layout.py \
		tools/bodies.py
	@$(call status,import,config/1st_read/imports.txt)
	$(Q)python3 tools/imports.py --uses$(if $(DC_REPLACED), $(subst :, ,$(DC_REPLACED:%=--replace %)))$(if \
		$(DC_ADDED), $(subst :, ,$(DC_ADDED:%=--add %)))
	$(Q)mkdir -p $(@D) && touch $@

# A body source's GCC object, one of DC_BODY_OBJ, waits for its bodies, as
# its SHC object does. The Makefile's rule writes them from the disc by the
# source's table of offsets and its reads, so they need no SHC
DC_BODY_OBJ := $(BODY_C:%.c=build/dc/%.o)
$(DC_BODY_OBJ): build/dc/%.o: build/gen/%/bodies.txt

# make dc-check builds make dc's objects that need no disc, so a host with
# none builds them, and sums them, so two hosts can compare what they built.
# DC_CHECKED is its objects: each of DC_OBJ but a body source's, which waits
# for its bodies, and an image's content, whose C the disc's data gives. The
# Makefile makes an object wait for the disc for make dc, dc-disc and dc-order,
# not for dc-check. The check archives SH4ZAM's objects too, and writes the
# Makefile's READS, each source's reads, which need no disc. It then runs the
# census check's first pass without --generated and --disc, which reads
# 1ST_READ.BIN's sources, the files their reads name and its tables, and
# nothing make writes from the disc. The pass is a line of the goal's recipe,
# as splice.py --check is of make check's, so it runs each time. The check
# links no image, and doesn't run the census check's --dc pass. It waits for
# the import check, which needs neither the disc nor SHC, and names it ahead
# of the sums, so that make runs it as the build starts, not once every
# object is built
DC_CHECKED := $(filter-out $(DC_BODY_OBJ) $(DC_CONTENT_OBJ),$(DC_OBJ))
dc-check: build/dc/imports.ok build/dc/check.txt $(READS) $(if $(BODY_C),build/dc/offsets.ok)
	@$(call status,census,config/1st_read/census.txt)
	$(Q)python3 tools/census.py

# make dc-check reads each body source's table of offsets, one of the
# Makefile's OFFSETS, which needs no SHC either. tools/offsets.py fails on a
# table with a row that isn't one, and a source with no table stops make by
# the Makefile's rule. A table records nothing of its source's text, so the
# check reads no source. Only make check, where the host runs SHC, holds a
# table's rows to the ones the source's stub build gives. A tree with no
# body source has no table to check. build/dc/offsets.txt is the tables'
# inventory: a source renamed with its table keeps the table's date, and the
# inventory has the check run again. So does a change to this file, which
# gives its recipe
ifneq ($(DC_GOALS),)
$(call record,build/dc/offsets.txt,$(OFFSETS))
endif
build/dc/offsets.ok: build/dc/offsets.txt $(OFFSETS) tools/offsets.py tools/tables.py tools/tool.py dc.mk
	@$(call status,offset,$(words $(OFFSETS)) tables)
	$(Q)python3 tools/offsets.py $(OFFSETS:offsets/%.txt=%.c)
	$(Q)mkdir -p $(@D) && touch $@

# make dc's link compresses each .PRS image with build/prskit, which the
# host's C compiler builds. DC_COMPRESSED is two files as build/prskit
# compresses them, so the sums show whether two hosts' builds of prskit
# compress alike: prskit's library, a text whose bytes tools/SHA256SUMS pins,
# and the archive, SH-4 code as an image is. Each names its source first, and
# the one recipe compresses that
DC_COMPRESSED := build/dc/check/prskit.c.prs build/dc/check/libsh4zam.a.prs
build/dc/check/prskit.c.prs: tools/prskit/prskit.c build/prskit
build/dc/check/libsh4zam.a.prs: $(DC_SH4ZAM_LIB) build/prskit
$(DC_COMPRESSED):
	$(Q)mkdir -p $(@D) && build/prskit compress $< $@

# build/dc/check.txt holds sha256sum's line, a hash and a path, for each of
# DC_CHECKED, for the archive, for each of DC_COMPRESSED and for the
# toolchain's libgcc.a, whose members make dc's link takes. The lines lie in
# the order of their paths in the C locale, as SHA256SUMS's do, so two hosts
# that built the same files under one SH_ELF write the same text, and diff
# names each file that differs. libgcc.a's path opens with SH_ELF, so under
# another SH_ELF its line names another path, and takes another place in the
# order. The lines reach sort through a file, not a pipe, which would end
# with sort's status: the recipe fails where sha256sum can't read a file.
# build/dc/check/objects.txt is DC_CHECKED's inventory. It has the sums
# written again once an object joins or leaves, and it gives the recipe the
# objects' paths: make hands the shell a command as one argument, and the
# paths would fill half the 128 KB Linux allows one. The linker's stamp holds
# libgcc.a's hash, so the sums are written again with another libgcc.a
ifneq ($(DC_GOALS),)
$(call record,build/dc/check/objects.txt,$(sort $(DC_CHECKED)))
endif
build/dc/check.txt: build/dc/check/objects.txt $(DC_CHECKED) $(DC_SH4ZAM_LIB) $(DC_COMPRESSED) $(LD_ID)
	@$(call status,sums,$@)
	$(Q)sha256sum $$(cat $<) $(DC_SH4ZAM_LIB) $(DC_COMPRESSED) $(LIBGCC) > $@.tmp
	$(Q)LC_ALL=C sort -k 2 $@.tmp > $@ && rm $@.tmp

# The toolchain make builds where SH_ELF names tools/sh-elf: binutils, and
# GCC for C alone with the libgcc of its one multilib, whose division, copy
# and double routines GCC's code calls. GMP, MPFR and MPC build inside GCC's
# tree, which builds GMP for no CPU in particular, so GMP needs no m4. The
# recipe fetches the sources itself, as SHC's does. It builds under
# build/toolchain/ and installs in tools/sh-elf.part. It fails there where
# GCC's build installed no file of one of LINK_TIME_TOOLS' names, so no
# toolchain that can't run make dc's link-time compile takes tools/sh-elf's
# place. Then it writes the toolchain's receipt, request.txt, which is
# SH_ELF_REQUEST. tools/sh-elf.part then takes tools/sh-elf's place, whole,
# and the recipe removes the sources and the build
SH_ELF_PREFIX := $(call quoted,$(CURDIR)/tools/sh-elf.part)
# $(call sh_elf,PACKAGE,COMMAND) runs COMMAND in PACKAGE's build directory and
# adds its output to PACKAGE's log, which a failure names. It enters the
# directory with cd -P, which names it in PWD through no symlink, as CURDIR
# names the checkout: GCC takes the directory of a compile from PWD, and
# sh_elf_map's path must open it, where a symlink led to the checkout too. No
# make that COMMAND runs inherits this make's flags, its jobserver among them.
# SH_ELF_MAKE and SH_ELF_INSTALL run this make's own executable, $(MAKE). The
# first builds a package with this make's -j, if it has one, and the second
# installs it with one job. MAKEINFO=true keeps each from writing a manual,
# which would need makeinfo
sh_elf = (mkdir -p build/toolchain/$1.build && cd -P build/toolchain/$1.build && unset MAKEFLAGS MFLAGS MAKELEVEL && \
	$2) >> build/toolchain/$1.log 2>&1 || (echo 'build/toolchain/$1.log: $1 failed'; false)
SH_ELF_MAKE = $(MAKE) $(filter -j%,$(MAKEFLAGS))
SH_ELF_INSTALL = $(MAKE)
# libgcc_tm.h is made before libgcc: its t-sh doesn't make
# unwind-dw2-Os-4-200.o wait for it
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
		../gcc/configure --prefix=$(SH_ELF_PREFIX) $(SH_ELF_GCC) $(call sh_elf_map,$(call quoted,$(CURDIR))) && \
		$(SH_ELF_MAKE) all-gcc configure-target-libgcc MAKEINFO=true && \
		$(SH_ELF_MAKE) -C sh-elf/libgcc libgcc_tm.h && $(SH_ELF_MAKE) all-target-libgcc MAKEINFO=true && \
		$(SH_ELF_INSTALL) install-gcc install-target-libgcc MAKEINFO=true)
	$(Q)for f in $(LINK_TIME_TOOLS); do ls tools/sh-elf.part/libexec/gcc/sh-elf/*/$$f > /dev/null 2>&1 || \
		{ echo "tools/sh-elf.part: GCC's build installed no $$f, which make dc's link-time compile runs"; exit 1; }; done
	$(Q)printf '%s\n' $(call quoted,$(SH_ELF_REQUEST)) > tools/sh-elf.part/request.txt
	$(Q)rm -rf tools/sh-elf && mv tools/sh-elf.part tools/sh-elf
	$(Q)rm -rf tools/sh-elf.src build/toolchain

# SH4ZAM's library whole: each file of its include/ and source/, and its
# LICENSE, fetched by itself from the commit SH4ZAM_AT names. SH4ZAM's
# AGENTS.md, as of its commit ad2f890, accepts a copy only by its third
# route, a synchronized copy: the whole library, pulled from upstream by a
# build mechanism that records the commit, with local changes kept to a
# minimum and sent upstream. The tree makes none of its own: SH4ZAM_AT
# names a merge, in reshaped/sh4zam, of upstream's master and the changes
# the tree offers upstream, until upstream takes them. GitHub's archive of a
# commit holds the same files, but the archive's compressed bytes can change
SH4ZAM_AT := https://raw.githubusercontent.com/reshaped/sh4zam/68ce191cec3f7521893e0d84e84095e5faceb5e3
$(SH4ZAM): $(if $(UNFETCHED),FORCE)
	@$(call status,fetch,$@)
	$(Q)rm -f $(FETCHED) && mkdir -p $(@D) && $(call fetch,$(SH4ZAM_AT)/$(@:tools/sh4zam/%=%),$@)
# Each fetch removes FETCHED, which make writes only once every file is in
# place. After a run of fetches cut short, make fetches every file again.
# Once every file is in place, FETCHED's recipe removes each other file
# under tools/sh4zam/, one an earlier pin listed, then writes FETCHED
$(FETCHED): $(SH4ZAM)
	$(Q)rm -f $@ $(filter-out $(SH4ZAM) $(FETCHED),$(shell [ ! -d tools/sh4zam ] || find tools/sh4zam -type f))
	$(Q)mkdir -p $(@D) && $(sh4zam_sums) > $@
