# Sonic Adventure (Japan) build. See README.md and TOOLCHAIN.md

.DEFAULT_GOAL := all

## CC=command: use this C compiler instead of cc
## WIBO=path: use this wibo instead of downloading one
WIBO ?= tools/wibo-i686
## R10=dir: use this SHC v5.0r10 instead of downloading it
R10  ?= tools/shc-v5.0r10
## R26=dir: use this SHC v5.0r26 instead of downloading it
R26  ?= tools/shc-v5.0r26
## R32=dir: use this SHC v5.0r32 instead of downloading it
R32  ?= tools/shc-v5.0r32
# tools/link.py and tools/relink.py run R32's asmsh, lnk and cnvs under wibo
export WIBO R32
## V=1: print commands, bytes by category and relink's stand-ins
VERBOSE := $(filter-out 0,$(V))
Q := $(if $(VERBOSE),,@)
# Python bytecode for tools/ goes under build/, not beside the sources.
# make before 4.4 passes no exported variable to $(shell), so a $(shell)
# that runs Python sets it there too
export PYTHONPYCACHEPREFIX := build/pycache

# Stop before all, check, progress, relink, shift, lint or format runs on a
# host that lacks what the goal needs, and name what it lacks; README.md#build
# lists it. A goal that names a file gets no such check. make -n checks the
# versions of make and Python, and no command. It still writes build/deps.mk,
# build/sources.txt, build/expected.txt, build/checked.txt and, with
# sha256sum, the stamps of the tools in place
ifeq ($(filter grouped-target,$(.FEATURES)),)
$(error the build needs GNU make 4.3 or later, and this is $(MAKE_VERSION))
endif
ifneq ($(shell PYTHONPYCACHEPREFIX=$(PYTHONPYCACHEPREFIX) python3 -c 'import sys; print(sys.version_info >= (3, 8))'),True)
$(error the build needs Python 3.8 or later, as python3)
endif
PINNED     := $(filter tools/%,$(WIBO) $(R10)/shc.exe $(R26)/shc.exe $(R32)/shc.exe)
LLVM       := tools/llvm/bin
CLANG_TOOLS := $(LLVM)/clang-format $(LLVM)/clang-tidy
# clang-format's output and clang-tidy's checks change between releases, so
# both are pinned: to these wheels, and to the lines tools/SHA256SUMS gives
# them, which an install writes to INSTALLED once both tools are in place.
# Unless both tools are there and INSTALLED holds those lines, make installs
# the wheels before it runs either tool
CLANG_FORMAT_WHEEL := tools/llvm/clang_format-22.1.8-py2.py3-none-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl
CLANG_TIDY_WHEEL   := tools/llvm/clang_tidy-22.1.8-py2.py3-none-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl
wheel_sums = awk '$$2 == "$(CLANG_FORMAT_WHEEL)" || $$2 == "$(CLANG_TIDY_WHEEL)"' tools/SHA256SUMS
INSTALLED  := tools/llvm/wheels.txt
ifneq ($(wildcard $(CLANG_TOOLS) $(INSTALLED)) $(strip $(file <$(INSTALLED))),$(CLANG_TOOLS) $(INSTALLED) $(strip $(shell $(wheel_sums))))
UNINSTALLED := $(CLANG_TOOLS)
endif
BUILDING   := $(filter all check progress relink shift,$(or $(MAKECMDGOALS),all))
DOWNLOADS  := $(filter-out $(wildcard $(PINNED)),$(if $(BUILDING),$(PINNED)) \
	$(if $(filter lint format,$(MAKECMDGOALS)),$(UNINSTALLED)))
# MAKEFLAGS opens with the single-letter flags, n among them under make -n
DRY_RUN    := $(findstring n,$(firstword -$(MAKEFLAGS)))
HOST_NEEDS := $(if $(DRY_RUN),,$(if $(BUILDING)$(filter lint,$(MAKECMDGOALS)),$(firstword $(CC))) \
	$(if $(BUILDING)$(DOWNLOADS),sha256sum) $(if $(BUILDING),timeout) $(if $(DOWNLOADS),curl) \
	$(if $(filter %/shc.exe,$(DOWNLOADS)),tar gzip))
# Installing clang-format and clang-tidy runs python3 -m venv, which needs
# Python's ensurepip; Debian and Ubuntu ship it apart, in python3-venv
HOST_LACKS := $(shell for c in $(HOST_NEEDS); do command -v $$c > /dev/null || echo $$c; done; \
	$(if $(DRY_RUN),,$(if $(filter $(CLANG_TOOLS),$(DOWNLOADS)), \
	PYTHONPYCACHEPREFIX=$(PYTHONPYCACHEPREFIX) python3 -c 'import ensurepip' 2> /dev/null || echo python3-venv)))
ifneq ($(HOST_LACKS),)
$(error $(HOST_LACKS): not found; see README.md#build)
endif

include config/compilers.mk

# The directories that hold the sources, as tools/config.py's ROOTS names them
ROOTS := src sdk modules events loader

# Inline assembly in a module, an event or the loader needs its release and
# flags shown by a byte match and given by a rule here; stop rather than
# choose one
UNSETTLED := $(filter-out $(MOVIE_SDK),$(call inline_asm,modules events loader))
ifneq ($(UNSETTLED),)
$(error $(UNSETTLED): no rule gives this inline assembly its release and flags; see TOOLCHAIN.md#c-for-shc)
endif
R32_GAME_OBJ := $(patsubst %.c,build/%.obj,$(filter-out $(ASMCODE) $(PATCHED_C) $(MOVIE_SDK) $(LOADER_SDK),$(shell find src modules events -name '*.c')) $(LOADER_C) $(KAMUI_C))
R10_SDK_OBJ := $(patsubst %.c,build/%.obj,$(filter-out $(PATCHED_C) $(GRAFTED_C) $(DONOR_C) $(R26_C) $(R32_C) $(ASMCODE) $(RUNTIME_C) $(KAMUI_C),$(shell find sdk -name '*.c') $(MOVIE_SDK) $(LOADER_SDK)))
R32_RUNTIME_OBJ := $(patsubst %.c,build/%.obj,$(RUNTIME_C))
PATCHED_OBJ := $(patsubst %.c,build/%.obj,$(PATCHED_C))
GRAFTED_OBJ := $(patsubst %.c,build/%.obj,$(GRAFTED_C))
R26_SDK_OBJ := $(patsubst %.c,build/%.obj,$(R26_C))
R32_SDK_OBJ := $(patsubst %.c,build/%.obj,$(R32_C))
ASMCODE_OBJ := $(patsubst %.c,build/%.obj,$(ASMCODE))
# The objects of the tree's C sources, one class for each route
C_SOURCE_OBJ := $(R32_GAME_OBJ) $(R10_SDK_OBJ) $(R32_RUNTIME_OBJ) $(PATCHED_OBJ) $(GRAFTED_OBJ) $(R26_SDK_OBJ) $(R32_SDK_OBJ) $(ASMCODE_OBJ)
ASM_SRC  := $(shell find $(ROOTS) -name '*.src')
ASM_OBJ  := $(patsubst %.src,build/%.obj,$(ASM_SRC))
# Stop on a C source and an assembly source of one stem: every image links
# build/<stem>.obj, which make would build by whichever rule it reads last
OBJECT_CONFLICTS := $(filter $(ASM_OBJ),$(C_SOURCE_OBJ))
ifneq ($(OBJECT_CONFLICTS),)
$(error $(OBJECT_CONFLICTS:build/%.obj=%) has a .c and a .src, which would build one object)
endif
# $(call from_config,ITEMS) gives each item of ITEMS, a Python iterable in
# which config names tools/config.py, and stops make where the query fails.
# A comma outside parentheses in ITEMS would end make's argument, and a
# single quote the shell's word
from_config = $(shell PYTHONPYCACHEPREFIX=$(PYTHONPYCACHEPREFIX) python3 -c 'import sys; sys.path[:0] = ["tools"]; import config; print(*$1)')$(if $(filter-out 0,$(.SHELLSTATUS)),$(error tools/config.py: $1 failed))
# The gen/ ranges the tables place, whose C the build writes from the disc,
# and the images that place them
GEN      := $(sort $(call from_config,(row[0] for path in config.CONFIG.glob("*/splits.txt") \
	for row in config.table(path) if row[0].startswith("gen/"))))
GEN_OBJ  := $(GEN:%.c=build/%.obj)
GEN_CONF := $(sort $(foreach f,$(GEN),$(word 2,$(subst /, ,$f))))

# The source groups that give make V=1's and make progress's reports each
# source's route
ROUTES := --patched $(PATCHED_C) --grafted $(GRAFTED_C) --inline $(INLINE_C)

# One link for each source in each image whose splits.txt places it, at that
# image's addresses. image, object and stub_object take a link's stem,
# <image>/<source less its extension>
LINKS    := $(sort $(shell awk '{ sub(/#.*/, "") } NF >= 4 && $$1 ~ "/" { \
	split(FILENAME, conf, "/"); sub(/\.[a-z]+$$/, ".bin", $$1); print "build/" conf[2] "/" $$1 }' config/*/splits.txt))
image    = $(firstword $(subst /, ,$1))
object   = build/$(patsubst $(call image,$1)/%,%,$1).obj
stub_object = build/stub/$(patsubst $(call image,$1)/%,%,$1).obj
GEN_LINKS := $(foreach f,$(GEN),$(filter build/%/$(f:.c=.bin),$(LINKS)))
# Stop on a source no splits.txt places: no goal would build it
UNPLACED := $(filter-out $(foreach l,$(LINKS),$(call object,$(l:build/%.bin=%))),$(C_SOURCE_OBJ) $(ASM_OBJ))
ifneq ($(UNPLACED),)
$(error $(UNPLACED:build/%.obj=%) has no row in any splits.txt)
endif

# The original files the build reads: each image's, which make rebuilds
# under build/, along with the .BIN each .PRS decodes to, and each event that
# repeats another byte for byte, which make checks but never rebuilds
DISC     := $(call from_config,config.files().values())
ALIASES  := $(call from_config,(f"SONICADV/{alias}.PRS" for alias in config.EVENT_ALIASES))
ORIG     := $(addprefix orig/,$(DISC) $(ALIASES))
DECODED  := $(addprefix build/,$(patsubst %.PRS,%.BIN,$(filter %.PRS,$(DISC))))
IMAGES   := $(addprefix build/,$(DISC)) $(DECODED)

# The sources that include bodies of data from the disc, .inc files make
# writes under build/gen/. Their stub builds take zero bodies from build/stub/
# instead, and need no disc
BODY_C   := $(shell grep -rl --include='*.c' 'include "[A-Za-z0-9_/]*\.inc"$$' $(ROOTS))
STUB_INC_FILES := $(shell grep -o 'include "[A-Za-z0-9_/]*\.inc"$$' /dev/null $(BODY_C) | sed 's|^\([a-z]*\)/[^:]*:include "\(.*\)"|build/stub/\1/\2|')
# A text.inc stub is not zeros: tools/data.py --texts writes a distinct
# placeholder for each of its source's strings, at the string's declared length
TEXTS    := $(filter %/text.inc,$(STUB_INC_FILES))
# A values.inc stub defines each VALUE_n its source writes, as 0: tools/data.py
# --values reads them from the source
VALUES   := $(filter %/values.inc,$(STUB_INC_FILES))
ZERO_INC_FILES := $(filter-out $(TEXTS) $(VALUES),$(STUB_INC_FILES))
STUB_OBJ := $(BODY_C:%.c=build/stub/%.obj)
R10_SDK_STUB_OBJ := $(filter $(R10_SDK_OBJ:build/%=build/stub/%),$(STUB_OBJ))
PATCHED_STUB_OBJ := $(filter $(PATCHED_OBJ:build/%=build/stub/%),$(STUB_OBJ))
# Stop on a body source whose route has no stub rule: only the game's, R10's
# and the patched routes have one, and on any other its stub would build as
# game code. Stop too on one in the loader's own code or a patched source in
# src/, for which the game's and the patched stub rules search other headers
# than its route does
UNSTUBBED := $(filter $(R26_C) $(R32_C) $(RUNTIME_C) events/shared/sdk/% $(GRAFTED_C) \
	$(ASMCODE) $(LOADER_C) $(filter src/%,$(PATCHED_C)),$(BODY_C))
ifneq ($(UNSTUBBED),)
$(error $(UNSTUBBED): no rule builds this body source's stub on its route; see TOOLCHAIN.md#stub-and-full-builds)
endif
# The .inc files tools/data.py --bodies writes from the disc, one for each
# stub include
GEN_INC_FILES := $(STUB_INC_FILES:build/stub/%=build/gen/%)
C_OBJ    := $(C_SOURCE_OBJ) $(STUB_OBJ) $(GEN_OBJ)
BODY_LINKS := $(foreach f,$(BODY_C),$(filter build/%/$(f:.c=.bin),$(LINKS)))
STUB_LINKS := $(BODY_LINKS:build/%=build/stub/%)

# The links make check builds, without the disc: each source's, or its stub's
# where it has bodies, and no gen/ range's, whose C comes from the disc
CHECKED  := $(filter-out $(BODY_LINKS) $(GEN_LINKS),$(LINKS)) $(STUB_LINKS)
# The files SHA256SUMS records, one line each, and no others: the original
# files, and the build/ files make checks against their own lines, the links
# and the decoded images
EXPECTED := $(ORIG) $(CHECKED) $(BODY_LINKS) $(DECODED)

C_FILES  := $(shell find $(ROOTS) -name '*.[ch]')
TIDIED   := $(patsubst %,build/lint/%.tidy,$(filter %.c,$(C_FILES)))

# An image's tables, and 1ST_READ.BIN's symbols, which other images can use
tables = config/$1/splits.txt config/$1/symbols.txt config/1st_read/symbols.txt
# The image a source reads its bodies from: the first, by name, that links it
home = $(firstword $(patsubst build/%/$(1:.c=.bin),%,$(filter build/%/$(1:.c=.bin),$(LINKS))))
# Each image's name and its file on the disc. disc_image gives the original
# image tools/data.py and tools/splice.py read: the file in orig/, or the
# .BIN build/gen/img/ decodes its .PRS to
CONFS    := $(join $(addsuffix :,$(shell echo $(basename $(notdir $(DISC))) | tr A-Z a-z)),$(DISC))
disc_image = $(foreach f,$(patsubst $1:%,%,$(filter $1:%,$(CONFS))),$(if $(filter %.PRS,$f),build/gen/img/$(notdir $(f:.PRS=.BIN)),orig/$f))
# The image the splicer builds for each image's name: its file under build/,
# or for a .PRS file the .BIN it decodes to
spliced = $(foreach f,$(patsubst $1:%,%,$(filter $1:%,$(CONFS))),build/$(f:.PRS=.BIN))
# Each image linked whole, which make relink compares with its splice
RELINKED := $(foreach c,$(CONFS),build/relink/$(firstword $(subst :, ,$c)).bin)
# Each image's candidate fixed addresses, which make shift lists
FIXED    := $(RELINKED:.bin=/fixed.txt)
# An original file's check, which each step reading it waits for
verified = $(patsubst orig/%,build/verified/%.ok,$(filter orig/%,$1))

# A tool's stamp holds the hashes of its files, so replacing one, or
# pointing at another copy, rebuilds what it built. SHC runs under wibo, so a
# release's stamps include wibo's; asmsh, lnk and cnvs are R32's, so what they
# build depends on R32's stamp
WIBO_ID := build/tools/wibo.id
R10_ID  := build/tools/r10.id $(WIBO_ID)
R26_ID  := build/tools/r26.id $(WIBO_ID)
R32_ID  := build/tools/r32.id $(WIBO_ID)

comma := ,
space := $(empty) $(empty)
# $1 as one shell word, whatever it holds, as a person's .gdi name may
quoted = '$(subst ','\'',$1)'
# Print a step's status line: its label, $1, padded to the longest label's
# six characters, then $2 as given. tools/splice.py prints its line in this
# column too
status = printf '  %-6s %s\n' '$1' $(call quoted,$2)
# SHC under wibo now and then deadlocks. A tool still running after HUNG
# seconds is taken for hung: timeout sends TERM to it and what it started,
# and KILL to the tool if it still runs GRACE seconds on. wibo ends on TERM,
# as does the wibo SHC starts for each pass, so none outlives the timeout
# and make can be run again. HUNG and GRACE are variables so that a test
# can shorten them; tools/link.py keeps its own
HUNG  := 600
GRACE := 10
wibo = timeout --verbose -k $(GRACE) $(HUNG) $(WIBO)
# asmsh interprets '/' as an option prefix
dos = '$(subst /,\,$1)'
# Keep SHC's or asmsh's output in $@.log, and print it and fail on a nonzero
# status or on any error or warning it reports, since some warnings come with
# status 0. ADMITTED, set for a source in config/compilers.mk, is the text of
# one warning its compile may report once
quiet = > $@.log 2>&1 && ! grep ' ([EWF]) ' $@.log $(if $(ADMITTED),| awk -v admitted='$(ADMITTED)' 'index($$0$(comma) admitted) && !seen++ { next } 1') | grep -q . || (cat $@.log; false)
# R10 names its temporary files from its process id's digits, so two
# compiles sharing SHC_TMP can clash; each object takes a directory of its own
shc_tmp = $(@:build/%.obj=build/tmp/%)
# $(call shc,RELEASE,INCLUDES,SOURCE,OPTIONS) runs RELEASE's shc.exe on
# SOURCE with OPTIONS, searching INCLUDES, directories split by commas, for
# headers
shc = mkdir -p $(shc_tmp) && SHC_LIB=$1 SHC_TMP=$(shc_tmp) SHC_INC=$2 $(wibo) $1/shc.exe $3 $4 $(quiet)
# $(call asmsh,SOURCE,OPTIONS) runs R32's asmsh on SOURCE with OPTIONS,
# writing $@ and its listing, $(@:.obj=.lis). Only SOURCE, $@ and its listing
# go through dos, so a path in OPTIONS takes its own $(call dos,...)
asmsh = $(wibo) $(R32)/asmsh.exe $(call dos,$1) -object=$(call dos,$@) -list=$(call dos,$(@:.obj=.lis)) $2 $(quiet)

GAME_INC    := sdk/include,src,modules,events,build/gen/src,build/gen/modules,build/gen/events,build/gen/sdk
STUB_INC    := sdk/include,src,modules,events,build/stub/src,build/stub/modules,build/stub/events,build/stub/sdk
GEN_INC     := sdk/include
SDK_INC     := sdk/include,sdk,modules,.,build/gen/sdk,build/gen/modules,build/gen/loader
SDK_STUB_INC := sdk/include,sdk,modules,.,build/stub/sdk,build/stub/modules,build/stub/loader
R26_SDK_INC := sdk/include
R32_SDK_INC := sdk/include,sdk
ASMCODE_INC := sdk/include,src
# A movie SDK member with inline assembly sees the SDK's headers, as the
# movie's other members do
$(filter $(MOVIE_SDK:%.c=build/%.obj),$(ASMCODE_OBJ)): ASMCODE_INC := $(SDK_INC)
# A patched source sees the SDK's headers, but one in src/ the game's, as a
# source there with inline assembly does
PATCHED_INC = $(if $(filter src/%,$<),$(ASMCODE_INC),$(SDK_INC))
# The suffix of the text asmsh assembles for a patched object: shckit's
# _patched, or _aligned where config/compilers.mk gives the object an ALIGN,
# a section and an alignment, which tools/align.py declares for the section
# in a copy of shckit's text
assembled = $(if $(ALIGN),_aligned,_patched)
# Only the recipe of a patched source's object, not its stub's, runs
# tools/align.py, so any other object fails where config/compilers.mk gives
# it an ALIGN, which its recipe would ignore. Its rule's secondary expansion,
# below, sees the object's ALIGN
unaligned = $(if $(ALIGN),$(error $@: its recipe runs no tools/align.py, so it takes no ALIGN))
# Write an object's header dependencies to its .d file. The host compiler's
# preprocessor searches the same directories SHC does, in the same order
depend = $(CC) -MM -MP -nostdinc $(addprefix -I,$(subst $(comma), ,$1)) -MT $@ -MF $(@:.obj=.d).tmp $< && \
	mv $(@:.obj=.d).tmp $(@:.obj=.d)
# A grafted object's dependencies on its donor's headers go to a .d file of
# their own
depend_donor = $(CC) -MM -MP -nostdinc $(addprefix -I,$(subst $(comma), ,$1)) -MT $@ -MF $(@:.obj=_donor.d).tmp \
	$(DONOR_C) && mv $(@:.obj=_donor.d).tmp $(@:.obj=_donor.d)

# Download $1 to $2 only if one line of tools/SHA256SUMS names $2 itself,
# and move it into place only once its hash matches that line's
fetch = sum=$$(awk '$$2 == "$2" { n++; sum = $$1 } END { if (n == 1) print sum }' tools/SHA256SUMS) && \
	{ [ -n "$$sum" ] || { echo '$2: not in tools/SHA256SUMS'; false; }; } && \
	curl -fsSL -o $2.part $1 && echo "$$sum  $2.part" | sha256sum -c --quiet && \
	mv $2.part $2

# sha256sum -c reads the events that repeat another, which no other rule
# asks for. It checks each file make rebuilds from an original against the
# original's line
## build every binary and check each against SHA256SUMS
all: $(IMAGES) $(STUB_LINKS) $(addprefix orig/,$(ALIASES))
	$(file >build/expected.txt,$(EXPECTED))
	$(Q)python3 tools/sums.py build/expected.txt build/sums.txt
	$(Q)sha256sum -c --quiet build/sums.txt
	@$(call status,sums,$(words $(EXPECTED) $(DISC)) files match SHA256SUMS)

## link each binary whole, and compare it with its splice
relink: $(RELINKED)

## link each binary at two shifted bases, and check its words
shift: build/relink/fixed.txt

## check disc-free linked sources and README.md's numbers
check: $(CHECKED)
	$(file >build/expected.txt,$(EXPECTED))
	$(Q)python3 tools/sums.py build/expected.txt
	$(Q)python3 tools/splice.py --check
	$(file >build/checked.txt,$(CHECKED))
	$(Q)awk 'NR == FNR { for (i = 1; i <= NF; i++) checked[$$i] = 1; next } $$2 in checked' build/checked.txt SHA256SUMS | sha256sum -c --quiet
	@$(call status,sums,$(words $(CHECKED)) files match SHA256SUMS)
	$(Q)python3 tools/progress.py --check $(ROUTES)

# Needs neither the disc nor SHC
## run the tests, and check the vendored tools' files
test:
	$(Q)PYTHONPATH=tools:tests python3 -m unittest discover -s tests $(if $(VERBOSE),-v,-q)
	$(Q)python3 tools/vendor.py --check

# Lint what changed since the last run that passed: clang-format the files
# edited since, or all of them when the style or the tool changes, and
# clang-tidy each C source whose text, headers or settings changed, against
# the stub bodies. make -k lint goes on past the first failure
## check C formatting and diagnostics, assembly and tags
lint: build/lint/format.ok $(TIDIED) build/lint/asm.ok build/lint/tags.ok

## list each open @asm tag, and count the @asm tags by kind
review:
	$(Q)python3 tools/tags.py --review $(C_FILES) $(ASM_SRC)

## format every C file in place
format: | $(LLVM)/clang-format
	$(Q)$(LLVM)/clang-format -i $(C_FILES)

# tools/progress.py reads each source's sections from its object, or its
# stub's, and asmsh's listing of each that holds assembly, so it waits for
# the links make check builds
## report what writes the code, and write README.md's numbers
progress: $(CHECKED)
	$(Q)python3 tools/progress.py $(ROUTES)
	$(Q)python3 tools/progress.py --write $(ROUTES)

## remove build/
clean:
	$(Q)rm -rf build
# make -j runs the goals at once, and rm -rf build would race their writes
ifneq ($(and $(filter clean,$(MAKECMDGOALS)),$(filter-out clean,$(MAKECMDGOALS))),)
$(error make clean takes no other goal)
endif

# The ## line directly above a goal says what it does. A variable's opens
# with the form a person gives it
## list these goals and variables
help:
	$(Q)awk -F: '/^## [A-Z0-9]+=/ { variables = variables sprintf("%-20s%s\n", substr($$1, 4), substr($$0, length($$1) + 3)); next } \
		/^## / { does = substr($$0, 4); getline; printf "%-20s%s\n", $$1, does } \
		END { printf "%s", variables }' $(MAKEFILE_LIST)

# make clean, format, help, lint, review and test compile no object, so they
# skip tools/deps.py and the .d files, and refresh no tool's stamp
COMPILING := $(filter-out clean format help lint review test,$(or $(MAKECMDGOALS),all))

# An empty recipe lets a side file its owner writes go missing without
# stopping make: an object's .d file, which keeps its header dependencies, a
# listing, .syms or an imported body. The owner, which depends on it, then
# runs again and writes it
ifneq ($(COMPILING),)
include build/deps.mk
$(C_OBJ): %.obj: %.d
$(C_OBJ:.obj=.d): ;
-include $(C_OBJ:.obj=.d)
$(GRAFTED_OBJ): %.obj: %_donor.d
$(GRAFTED_OBJ:.obj=_donor.d): ;
-include $(GRAFTED_OBJ:.obj=_donor.d)
endif
build/deps.mk build/sources.txt &: FORCE
	+$(Q)python3 tools/deps.py build/deps.mk build/sources.txt

$(C_OBJ) $(ASM_OBJ): Makefile config/compilers.mk

$(R32_GAME_OBJ): build/%.obj: %.c $(R32_ID)
	@$(call status,r32,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(call depend,$(GAME_INC))
	$(Q)$(call shc,$(R32),$(GAME_INC),$<,-objectfile=$@ -code=machinecode $(GAME))

$(GEN_OBJ): build/%.obj: build/%.c $(R32_ID)
	@$(call status,r32,build/$*.c)
	$(Q)$(call depend,$(GEN_INC))
	$(Q)$(call shc,$(R32),$(GEN_INC),$<,-objectfile=$@ -code=machinecode $(GAME))

$(R10_SDK_OBJ): build/%.obj: %.c $(R10_ID)
	@$(call status,r10,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(call depend,$(SDK_INC))
	$(Q)$(call shc,$(R10),$(SDK_INC),$<,-objectfile=$@ -code=machinecode $(SDK))

$(R32_RUNTIME_OBJ): build/%.obj: %.c $(R32_ID)
	@$(call status,r32,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(call depend,$(R32_SDK_INC))
	$(Q)$(call shc,$(R32),$(R32_SDK_INC),$<,-objectfile=$@ -code=machinecode $(RUNTIME))

$(PATCHED_OBJ): build/%.obj: %.c build/shckit tools/align.py tools/config.py $(R10_ID) $(R32_ID)
	$(if $(PATCHES),,$(error $<: no PATCHES names the patches shckit applies to it))
	@$(call status,r10,$< (shckit $(PATCHES)$(if $(ALIGN),$(comma) align $(ALIGN))))
	$(Q)mkdir -p $(@D)
	$(Q)$(call depend,$(PATCHED_INC))
	$(Q)$(call shc,$(R10),$(PATCHED_INC),$<,-objectfile=build/$*.src -code=asmcode $(SDK))
	$(Q)build/shckit apply $(PATCHES:%=-p %) -o build/$*_patched.src build/$*.src
	$(if $(ALIGN),$(Q)python3 tools/align.py $(ALIGN) build/$*_patched.src build/$*_aligned.src)
	$(Q)$(call asmsh,build/$*$(assembled).src,$(ASMFLAGS))
	$(Q)touch $@

# R10 compiles a grafted source and its donor alike, and tools/graft.py sets
# four of the donor's functions in the first text, which the patches then read
$(GRAFTED_OBJ): build/%.obj: %.c $(DONOR_C) tools/graft.py tools/config.py build/shckit $(R10_ID) $(R32_ID)
	$(if $(PATCHES),,$(error $<: no PATCHES names the patches shckit applies to it))
	@$(call status,r10,$< and $(DONOR_C) (graft$(comma) shckit $(PATCHES)))
	$(Q)mkdir -p $(@D)
	$(Q)$(call depend,$(SDK_INC))
	$(Q)$(call depend_donor,$(SDK_INC))
	$(Q)$(call shc,$(R10),$(SDK_INC),$<,-objectfile=build/$*.src -code=asmcode $(SDK))
	$(Q)$(call shc,$(R10),$(SDK_INC),$(DONOR_C),-objectfile=build/$*_donor.src -code=asmcode $(SDK))
	$(Q)python3 tools/graft.py build/$*.src build/$*_donor.src build/$*_grafted.src
	$(Q)build/shckit apply $(PATCHES:%=-p %) -o build/$*_patched.src build/$*_grafted.src
	$(Q)$(call asmsh,build/$*_patched.src,$(ASMFLAGS))
	$(Q)touch $@

$(R26_SDK_OBJ): build/%.obj: %.c $(R26_ID)
	@$(call status,r26,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(call depend,$(R26_SDK_INC))
	$(Q)$(call shc,$(R26),$(R26_SDK_INC),$<,-objectfile=$@ -code=machinecode $(SDK))

$(R32_SDK_OBJ): build/%.obj: %.c $(R32_ID)
	@$(call status,r32,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(call depend,$(R32_SDK_INC))
	$(Q)$(call shc,$(R32),$(R32_SDK_INC),$<,-objectfile=$@ -code=machinecode $(SDK))

$(ASMCODE_OBJ): build/%.obj: %.c $(R10_ID) $(R32_ID)
	@$(call status,r10,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(call depend,$(ASMCODE_INC))
	$(Q)$(call shc,$(R10),$(ASMCODE_INC),$<,-objectfile=build/$*.src -code=asmcode $(SDK))
	$(Q)$(call asmsh,build/$*.src,$(ASMFLAGS))
	$(Q)touch $@

$(ASM_OBJ): build/%.obj: %.src $(R32_ID)
	@$(call status,asm,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(call asmsh,$<,$(ASMFLAGS))
	$(Q)touch $@

# Each object asmsh assembles depends on its listing, which
# tools/progress.py reads. asmsh can write the listing after the object, so
# each recipe touches its object last. The pattern is the recipes',
# build/%.obj, since $* takes the stem of a target's last static pattern rule
$(ASMCODE_OBJ) $(PATCHED_OBJ) $(GRAFTED_OBJ) $(ASM_OBJ): build/%.obj: build/%.lis
$(patsubst %.obj,%.lis,$(ASMCODE_OBJ) $(PATCHED_OBJ) $(GRAFTED_OBJ) $(ASM_OBJ)): ;

$(ZERO_INC_FILES):
	$(Q)mkdir -p $(@D)
	$(Q)echo 0 > $@

# Every stub object waits for the zero bodies, and every lint target for all
# the stub includes, through one target each, so make keeps one list of them,
# not a copy for each of thousands of targets. Each object's .d file names the
# includes its source reads, which then rebuild it
stub-bodies: $(ZERO_INC_FILES)
## write the stub .inc files the sources include, for clangd
stub-includes: stub-bodies $(TEXTS) $(VALUES)
$(STUB_OBJ): | stub-bodies

$(TEXTS): build/stub/%/text.inc: %.c tools/data.py tools/config.py
	$(Q)python3 tools/data.py --texts $< $@

$(VALUES): build/stub/%/values.inc: %.c tools/data.py tools/config.py
	$(Q)python3 tools/data.py --values $< $@

# A stub builds with zero bodies and values and placeholder texts, and writes
# the listing whose offsets tools/data.py --bodies reads. This rule's stubs
# pass their options in a response file, since a Kamui stub's command line
# overflows SHC's parameter buffer. SHC, or asmsh for a patched stub, can
# write the listing after the object, which depends on it, so the object is
# touched last
$(filter-out $(R10_SDK_STUB_OBJ) $(PATCHED_STUB_OBJ),$(STUB_OBJ)): build/stub/%.obj: %.c $(R32_ID)
	@$(call status,r32,$< (stubs))
	$(Q)mkdir -p $(@D)
	$(Q)$(call depend,$(STUB_INC))
	$(Q)printf '%s\n' '-objectfile=$@' '-listfile=$(@:.obj=.lis)' '-show=object' '-code=machinecode' $(GAME) > $@.sub
	$(Q)$(call shc,$(R32),$(STUB_INC),$<,-sub=$@.sub)
	$(Q)touch $@
# A loader SDK member's command line overflows SHC's parameter buffer too
$(R10_SDK_STUB_OBJ): build/stub/%.obj: %.c $(R10_ID)
	@$(call status,r10,$< (stubs))
	$(Q)mkdir -p $(@D)
	$(Q)$(call depend,$(SDK_STUB_INC))
	$(Q)printf '%s\n' '-objectfile=$@' '-listfile=$(@:.obj=.lis)' '-show=object' '-code=machinecode' $(SDK) > $@.sub
	$(Q)$(call shc,$(R10),$(SDK_STUB_INC),$<,-sub=$@.sub)
	$(Q)touch $@
# Patched sources' stubs take their route, and asmsh's listing
$(PATCHED_STUB_OBJ): build/stub/%.obj: %.c build/shckit tools/config.py $(R10_ID) $(R32_ID)
	$(if $(PATCHES),,$(error $<: no PATCHES names the patches shckit applies to its stub))
	@$(call status,r10,$< (stubs$(comma) shckit $(PATCHES)))
	$(Q)mkdir -p $(@D)
	$(Q)$(call depend,$(SDK_STUB_INC))
	$(Q)$(call shc,$(R10),$(SDK_STUB_INC),$<,-objectfile=$(@:.obj=.src) -code=asmcode $(SDK))
	$(Q)build/shckit apply $(PATCHES:%=-p %) -o $(@:.obj=_patched.src) $(@:.obj=.src)
	$(Q)$(call asmsh,$(@:.obj=_patched.src),$(ASMFLAGS))
	$(Q)touch $@
# A text.inc or values.inc changes with its source, so only that source's
# stub depends on it
$(TEXTS:build/stub/%/text.inc=build/stub/%.obj): build/stub/%.obj: build/stub/%/text.inc
$(VALUES:build/stub/%/values.inc=build/stub/%.obj): build/stub/%.obj: build/stub/%/values.inc
$(STUB_OBJ): %.obj: %.lis
$(STUB_OBJ:.obj=.lis): ;

# A source's object waits for its bodies, which tools/data.py --bodies
# reads from the disc, finding each by the offset its stub's listing gives.
# Its bodies.txt records their spans, and stands for them as a target
$(BODY_C:%.c=build/%.obj): build/%.obj: build/gen/%/bodies.txt
$(foreach i,$(GEN_INC_FILES),$(eval $(dir $i)bodies.txt: $i))
$(GEN_INC_FILES): ;
define bodies
build/gen/$(1:.c=)/bodies.txt: $1 build/stub/$(1:.c=.obj) $(call disc_image,$2) $(call verified,$(call disc_image,$2)) $(call tables,$2) tools/data.py tools/config.py
	@$$(call status,body,$$<)
	$$(Q)python3 tools/data.py --bodies $(call disc_image,$2) $$< build/stub/$(1:.c=.lis) $$(@D) $2
endef
$(foreach f,$(BODY_C),$(eval $(call bodies,$f,$(call home,$f))))

define generate
build/gen/$1/%.c: $(call disc_image,$1) $(call verified,$(call disc_image,$1)) $(call tables,$1) build/gen/$1/declared.txt tools/data.py tools/config.py
	@$$(call status,data,$$@)
	$$(Q)python3 tools/data.py $$< gen/$1/$$*.c $$@
endef
$(foreach conf,$(GEN_CONF),$(eval $(call generate,$(conf))))

$(GEN:%.c=build/%.c): build/%.c: build/%.syms
$(GEN:%.c=build/%.syms): ;

# The declaration scan reruns when its sources or tables change, but writes
# declared.txt only if its content changed; recreate the file if deleted
$(GEN_CONF:%=build/gen/%/declared.txt): build/gen/%/declared.txt: build/gen/%/declared.stamp
	$(Q)[ -f $@ ] || python3 tools/data.py --declared $* $@
$(GEN_CONF:%=build/gen/%/declared.stamp): build/gen/%/declared.stamp: build/sources.txt tools/data.py tools/config.py
	$(Q)python3 tools/data.py --declared $* $(@D)/declared.txt
	$(Q)touch $@

build/gen/img/%.BIN: orig/SONICADV/%.PRS build/verified/SONICADV/%.PRS.ok build/prskit
	$(Q)mkdir -p $(@D)
	$(Q)build/prskit decompress $< $@

.SECONDEXPANSION:
$(filter-out $(PATCHED_OBJ),$(C_OBJ) $(ASM_OBJ)): $$(unaligned)
$(LINKS): build/%.bin: $$(call object,$$*) $$(call tables,$$(call image,$$*)) tools/link.py tools/config.py $(R32_ID)
	@$(call status,link,$@)
	$(Q)python3 tools/link.py $< $(call image,$*)

$(STUB_LINKS): build/stub/%.bin: $$(call stub_object,$$*) $$(call tables,$$(call image,$$*)) tools/link.py tools/config.py $(R32_ID)
	@$(call status,link,$@)
	$(Q)python3 tools/link.py $< $(call image,$*)

# Build each image from its linked bytes and fill, which tools/splice.py
# checks against the original: the file in orig/, or for a .PRS file the .BIN
# build/gen/img/ decodes it to. A stored byte nothing writes fails the splice
# rather than fall back to the original's. The disc's data comes in through
# the sources and gen/ ranges that import it
splice = python3 tools/splice.py $< $@ $(if $(VERBOSE),--verbose) $(ROUTES)
SPLICE_TOOLS := tools/splice.py tools/config.py tools/link.py tools/code_ranges.py
$(addprefix build/,$(filter %.BIN,$(DISC))): build/%: orig/% build/verified/%.ok $(SPLICE_TOOLS)
	$(Q)mkdir -p $(@D)
	$(Q)$(splice)

$(DECODED): build/SONICADV/%.BIN: build/gen/img/%.BIN $(SPLICE_TOOLS)
	$(Q)mkdir -p $(@D)
	$(Q)$(splice)

# Link each image's objects in one pass, apart from the splice, and compare
# the result with the spliced image, which the splicer has checked against
# the disc. make relink runs it, and make doesn't. The objects come up to
# date with the spliced image, whose links read them
$(RELINKED): build/relink/%.bin: $$(call spliced,$$*) $$(call tables,$$*) tools/relink.py tools/link.py tools/config.py \
		tools/code_ranges.py $(R32_ID)
	@$(call status,relink,$(call spliced,$*))
	$(Q)python3 tools/relink.py $* $(call spliced,$*) $(if $(VERBOSE),--verbose)

# The shift test, once an image's whole link matches its splice: link it at
# two shifted bases too, and list the words that keep a value in its span,
# each with config/kept.txt's reason. relink.py links it at its base again
# and rewrites build/relink/IMAGE.bin, so it compares that link with the
# splice again
$(FIXED): build/relink/%/fixed.txt: build/relink/%.bin config/kept.txt
	$(Q)python3 tools/relink.py $* $(call spliced,$*) --shift

build/relink/fixed.txt: $(FIXED)
	$(Q)cat $^ > $@
	@$(call status,fixed,$@)

build/%.PRS: build/%.BIN build/prskit
	$(Q)build/prskit compress $< $@

# Check an original file against SHA256SUMS before anything reads it, so a
# wrong or partial one is named as such
.PRECIOUS: build/verified/%.ok
build/verified/%.ok: orig/% SHA256SUMS
	$(Q)mkdir -p $(@D)
	$(Q)awk '$$2 == "$<"' SHA256SUMS | grep -q . || (echo '$<: not in SHA256SUMS'; false)
	$(Q)awk '$$2 == "$<"' SHA256SUMS | sha256sum -c --quiet || (echo '$<: not the original file; replace it'; false)
	$(Q)touch $@

# Take the original files from orig/, extracting those it lacks from a GDI
# image there
MISSING := $(filter-out $(wildcard $(ORIG)),$(ORIG))
ifneq ($(MISSING),)
GDI_IMAGE := $(shell find orig -maxdepth 1 -iname '*.gdi')
# Count the words that open with orig/: a name can hold spaces, but no slash
FOUND   := $(words $(filter orig/%,$(GDI_IMAGE)))
ifeq ($(FOUND),0)
# $1's words, as a sentence lists them: A, B and C
listed = $(if $(word 3,$1),$(firstword $1)$(comma) $(call listed,$(wordlist 2,$(words $1),$1)),$(subst $(space), and ,$1))
LACKS   := $(MISSING:orig/%=%)
# With none of the files in orig/, name those at its top and count those
# under SONICADV/; with some, name the first three it lacks and count the rest
ifeq ($(MISSING),$(ORIG))
LACKING := holds none of the $(words $(ORIG)) files the build needs: $(call listed,$(filter-out SONICADV/%,$(LACKS)) $(words $(filter SONICADV/%,$(LACKS)))) under SONICADV/
IN_ORIG := no disc
else
LACKING := lacks $(words $(LACKS)) of the $(words $(ORIG)) files the build needs: $(if $(word 4,$(LACKS)),$(call listed,$(wordlist 1,3,$(LACKS)) $(words $(wordlist 4,$(words $(LACKS)),$(LACKS)))) more,$(call listed,$(LACKS)))
IN_ORIG := incomplete disc
endif
define ORIG_HELP
orig/ $(LACKING).
  Put them in orig/ as the disc names them, or put the disc's .gdi and its tracks there.
  Without the disc, make check checks the linked sources it can build.
endef
$(MISSING):
	$(info $(ORIG_HELP))
	$(error $(IN_ORIG) in orig/)
# Building the images, say what orig/ lacks before any compile, not after
# the images whose files it holds
ifneq ($(filter all relink shift,$(or $(MAKECMDGOALS),all)),)
$(C_OBJ) $(ASM_OBJ): | $(MISSING)
endif
else ifneq ($(FOUND),1)
$(MISSING):
	$(error orig/ has $(FOUND) .gdi files, and make takes one: $(GDI_IMAGE))
else
# Check every extracted file against SHA256SUMS before moving any into
# orig/, so a failed check leaves orig/ as it was. Building the images, make
# extracts before it compiles, so a wrong .gdi fails first
$(MISSING): unpack
ifneq ($(filter all relink shift,$(or $(MAKECMDGOALS),all)),)
$(C_OBJ) $(ASM_OBJ): | unpack
endif
unpack: build/gdi
	@$(call status,gdi,$(GDI_IMAGE))
	$(Q)rm -rf build/unpack
	$(Q)build/gdi extract $(call quoted,$(GDI_IMAGE)) build/unpack $(MISSING:orig/%=%)
	$(Q)for f in $(MISSING:orig/%=%); do \
		awk -v f="orig/$$f" '$$2 == f { print $$1 "  build/unpack/" substr(f, 6) }' SHA256SUMS | \
			sha256sum -c --quiet --status || { echo $(call quoted,$(GDI_IMAGE))": its $$f isn't the original"; exit 1; }; \
	done
	$(Q)for f in $(MISSING:orig/%=%); do \
		mkdir -p "$$(dirname "orig/$$f")" && mv "build/unpack/$$f" "orig/$$f" || exit 1; \
	done
endif
endif

# The host utilities vendored under tools/, as tools/vendor.py's TOOLS names
# them. vendor.py --check reads each one's directory, so the directories are
# prerequisites too: adding or removing a file reruns it
VENDORED := gdi prskit shckit
build/tools/vendored.ok: tools/SHA256SUMS $(VENDORED:%=tools/%) $(wildcard $(VENDORED:%=tools/%/*)) \
		tools/vendor.py tools/sums.py tools/config.py
	$(Q)python3 tools/vendor.py --check
	$(Q)mkdir -p $(@D) && touch $@

# The vendored tools build with their repositories' flags, once the check
# passes; a rerun of the check rebuilds none of them
HOSTFLAGS := -std=c99 -Wall -Wextra -Wpedantic -O2
$(VENDORED:%=build/%): build/%: tools/%/main.c $$(wildcard tools/$$*/*) Makefile | build/tools/vendored.ok
	@$(call status,cc,$@)
	$(Q)mkdir -p $(@D)
	$(Q)$(CC) $(HOSTFLAGS) -o $@ tools/$*/$*.c $<

# $(call refresh,NAME,FILES) writes sha256sum's lines for FILES, each a hash
# and a path, to NAME's stamp unless it holds them already, so an unchanged
# tool in the same place rebuilds nothing. A glob in FILES sorts in the C
# locale, so a change of locale leaves a stamp alone
refresh = LC_ALL=C; export LC_ALL; mkdir -p build/tools && id="$$(sha256sum $2)" && \
	{ [ "$$(cat build/tools/$1.id 2>/dev/null)" = "$$id" ] || echo "$$id" > build/tools/$1.id; }
# $(call stamp,NAME,TOOL,FILES,GOALS) makes NAME's stamp of TOOL from its
# FILES. GOALS are this run's goals that may use TOOL, and for none make leaves
# the stamp alone. For any, make refreshes it as it reads the Makefile, so
# make -n shows what a changed tool rebuilds, and a refresh that fails
# stops make. So does a missing TOOL, unless make fetches or builds it
# under tools/: then make refreshes the stamp once TOOL is in place, and
# what depends on the stamp builds again in the same run if it changed
define stamp
build/tools/$1.id: | $(filter tools/%,$2)
	$$(Q)$$(call refresh,$1,$3)
ifneq ($4,)
ifneq ($$(wildcard $2),)
$$(shell $$(call refresh,$1,$3))
ifneq ($$(.SHELLSTATUS),0)
$$(error build/tools/$1.id: can't refresh it from $3)
endif
else ifneq ($(filter tools/%,$2),)
build/tools/$1.id: FORCE
else
$$(error $2: no such file)
endif
endif
endef
$(eval $(call stamp,wibo,$(WIBO),$(WIBO),$(COMPILING)))
$(eval $(call stamp,r10,$(R10)/shc.exe,$(R10)/*,$(COMPILING)))
$(eval $(call stamp,r26,$(R26)/shc.exe,$(R26)/*,$(COMPILING)))
$(eval $(call stamp,r32,$(R32)/shc.exe,$(R32)/*,$(COMPILING)))

tools/wibo-i686:
	@$(call status,fetch,$@)
	$(Q)$(call fetch,https://github.com/decompals/wibo/releases/download/1.2.0/wibo-i686,$@)
	$(Q)chmod +x $@

# Unpack beside the release, and put it in place only once whole. Its file
# names go to lowercase: R32's archive holds LNK.EXE and LBR.EXE
tools/shc-%/shc.exe:
	@$(call status,fetch,tools/shc-$*.tar.gz)
	$(Q)$(call fetch,https://github.com/decompme/compilers/releases/download/compilers/shc-$*.tar.gz,tools/shc-$*.tar.gz)
	@$(call status,tar,$(@D))
	$(Q)rm -rf $(@D).part && mkdir -p $(@D).part
	$(Q)tar -xzf tools/shc-$*.tar.gz -C $(@D).part --strip-components=1
	$(Q)cd $(@D).part && for f in *; do l=$$(echo "$$f" | tr A-Z a-z); [ "$$f" = "$$l" ] || mv "$$f" "$$l"; done
	$(Q)for f in shc asmsh lnk cnvs; do [ -f $(@D).part/$$f.exe ] || { echo "$(@D): no $$f.exe"; exit 1; }; done
	$(Q)rm -rf $(@D) && mv $(@D).part $(@D)
	$(Q)rm tools/shc-$*.tar.gz

# pip installs the wheels tools/SHA256SUMS lists, and no other package. It
# is forced, since it leaves alone a wheel it finds installed, even one whose
# tool was deleted. INSTALLED is removed just before pip and written once
# both tools are in place, so an install cut short is made again, and a
# recipe that stops before pip leaves the install it found. make 4.3 deletes
# a grouped target whose recipe fails, so the tools are precious
.PRECIOUS: $(CLANG_TOOLS)
$(CLANG_TOOLS) &: $(if $(UNINSTALLED),FORCE)
	@$(call status,fetch,$(CLANG_FORMAT_WHEEL))
	$(Q)mkdir -p tools/llvm && $(call fetch,https://files.pythonhosted.org/packages/e5/88/b82c066fa807da4ca2518fecf79071361f6324b77375e5e92c059c0697fd/$(notdir $(CLANG_FORMAT_WHEEL)),$(CLANG_FORMAT_WHEEL))
	@$(call status,fetch,$(CLANG_TIDY_WHEEL))
	$(Q)$(call fetch,https://files.pythonhosted.org/packages/82/19/0f2668f8f5e2452b096a2b898f2b6bcecbceb6dd0c7f75d1755ce1f18d8b/$(notdir $(CLANG_TIDY_WHEEL)),$(CLANG_TIDY_WHEEL))
	@$(call status,venv,tools/llvm)
	$(Q)python3 -m venv tools/llvm
	$(Q)rm -f $(INSTALLED)
	$(Q)$(LLVM)/pip install -q --no-index --no-deps --force-reinstall $(CLANG_FORMAT_WHEEL) $(CLANG_TIDY_WHEEL)
	$(Q)rm $(CLANG_FORMAT_WHEEL) $(CLANG_TIDY_WHEEL)
	$(Q)for t in $(notdir $(CLANG_TOOLS)); do [ -x $(LLVM)/$$t ] || { echo "$(LLVM): no $$t"; exit 1; }; done
	$(Q)$(wheel_sums) > $(INSTALLED)

TABLES   := $(wildcard config/*/*.txt)
TIDY_INC := $(filter -I%,$(file <compile_flags.txt))
# clang-tidy takes a source's settings from the nearest .clang-tidy in its
# directory or above, and one that sets InheritParentConfig applies over the
# next one up
TIDY_CONFIGS := .clang-tidy $(shell find $(ROOTS) -name .clang-tidy)

# The files format.ok checks: every C file when .clang-format or clang-format
# changed, else the C files edited since its last pass
formatted = $(if $(filter-out %.c %.h,$?),$(C_FILES),$(filter %.c %.h,$?))
build/lint/format.ok: $(C_FILES) .clang-format $(LLVM)/clang-format
	@$(call status,format,$(words $(formatted)) of $(words $(C_FILES)) files)
	$(Q)$(LLVM)/clang-format --dry-run -Werror $(formatted)
	$(Q)mkdir -p $(@D) && touch $@

$(TIDIED): build/lint/%.tidy: % compile_flags.txt $(TIDY_CONFIGS) $(LLVM)/clang-tidy | stub-includes
	@$(call status,tidy,$<)
	$(Q)mkdir -p $(@D)
	$(Q)$(CC) -MM -MP -nostdinc $(TIDY_INC) -MT $@ -MF $(@:.tidy=.d).tmp $<
	$(Q)mv $(@:.tidy=.d).tmp $(@:.tidy=.d)
	$(Q)$(LLVM)/clang-tidy --quiet $<
	$(Q)touch $@
# A lint target's .d file keeps its header dependencies, as an object's does,
# and a deleted one reruns the check that writes it. CC writes these
# dependencies, not Clang, so compiler-specific #include branches can name
# different headers. make reads them for lint, or for any build/lint/ target
# named on the command line
ifneq ($(filter lint build/lint/%,$(MAKECMDGOALS)),)
$(TIDIED): %.tidy: %.d
$(TIDIED:.tidy=.d): ;
-include $(TIDIED:.tidy=.d)
endif

build/lint/asm.ok: $(ASM_SRC) build/shckit
	@$(call status,shckit,$(words $(ASM_SRC)) files)
	$(Q)build/shckit check $(ASM_SRC)
	$(Q)build/shckit lint $(ASM_SRC)
	$(Q)mkdir -p $(@D) && touch $@

build/lint/tags.ok: $(C_FILES) $(ASM_SRC) $(TABLES) tools/tags.py tools/config.py
	@$(call status,tags,$(words $(C_FILES) $(ASM_SRC)) files)
	$(Q)python3 tools/tags.py $(C_FILES) $(ASM_SRC)
	$(Q)mkdir -p $(@D) && touch $@

# A cached check depends too on the recipe and options this file gives it
build/lint/format.ok build/lint/asm.ok build/lint/tags.ok $(TIDIED): Makefile

.PHONY: all relink shift check test progress lint review format clean help unpack stub-bodies stub-includes FORCE
.DELETE_ON_ERROR:
