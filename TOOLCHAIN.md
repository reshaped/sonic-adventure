# Toolchain

The build reconstructs each of the game's binaries from source and disc-derived data, verifies it against the original, and accounts for every byte. Much of the complexity comes from SHC: moving an include can move a variable, and combining two sources can move a literal pool.

[README.md](README.md#build) gives the commands and what a build needs, and [CONTRIBUTING.md](CONTRIBUTING.md) how to make a change.

- **How a binary is built:** [Build model](#build-model), [What writes each byte](#what-writes-each-byte).
- **What the numbers measure:** [Accounting](#accounting).
- **Compilers:** [Releases](#releases), [Flags](#flags), [Running SHC](#running-shc), [C for SHC](#c-for-shc), [Assembly sources](#assembly-sources).
- **Binary images:** [Images](#images), [Image tables](#image-tables).
- **Objects:** [Fill](#fill), [Alignment](#alignment), [Literal pools](#literal-pools), [Inlining](#inlining), [Strings](#strings), [Seams in the tables](#seams-in-the-tables).
- **Data from the disc:** [Declared data](#declared-data), [Generated ranges](#generated-ranges).
- **Maintenance:** [Hashes](#hashes), [Tools](#tools).

## Build model

A **binary** is a file on the disc whose SH-4 code the build rebuilds: the main executable, `1ST_READ.BIN`, or one of the 246 overlays that load beside it, four of which the disc stores a second time, under another name. Its **binary image** is its bytes as the build links them, decompressed where the disc stores the file as PRS. A binary image is not a GDI disc image, and the build writes none. Below, "image" means a binary image, and "the disc" means your disc's files, in `orig/`.

Each image has a directory in `config/` with its two tables, `splits.txt` and `symbols.txt` ([Image tables](#image-tables)). A **`gen/` range** places data that no source in the tree holds: its row names `gen/<image>/d_<address>.c`, C the build writes from the disc ([Generated ranges](#generated-ranges)).

Each source has a **route**, its compiler configuration. A C source's is the release of SHC, the flags, whether SHC writes an object or assembly text for asmsh, and any patches applied to that text, as [`config/compilers.mk`](config/compilers.mk) gives it ([Releases](#releases), [Flags](#flags)). A `.src` file's is R32's asmsh ([Assembly sources](#assembly-sources)).

From source to checked file:

1. Each source follows its route to one **object**, containing every section produced by its compilation or assembly.
2. `tools/link.py` links the object at the addresses each image's tables give it, and writes one **linked source** for each image that places the source: `modules/stg05/camera_sweep.c` links to `build/stg02/modules/stg05/camera_sweep.bin` and `build/stg05/modules/stg05/camera_sweep.bin`. A linked source's hash is recorded in `SHA256SUMS`, except a `gen/` range's, whose bytes the splice checks against the original.
3. prskit decompresses a `.PRS` binary's original.
4. `tools/splice.py`, the splicer, checks the linked sources, the padding `gen/` ranges declare, and the linker's **fill** between objects against the original, then builds the image from zeros out of those bytes alone ([What writes each byte](#what-writes-each-byte)).
5. prskit compresses the rebuilt image for a `.PRS` binary, and `make` checks the result against the original.

Sources with disc-derived initializers are compiled first with placeholders. The resulting layout locates those initializers in the original binary. A second compile uses the extracted values. Verification permits differences only within the declared data, never in code. The build calls such an initializer a **body**, and the two compiles the **stub build** and the **full build** ([Declared data](#declared-data)).

The [Makefile](Makefile) schedules every step. Two `make` invocations that share `build/` write the same files without waiting for each other, which is why [Build](README.md#build) says to run one make at a time.

`make relink` checks each image a second way, beside the splice. [`tools/relink.py`](tools/relink.py) links all of the image's objects in one pass, the **whole link**, and compares what it writes with the spliced image, byte for byte. A source's own link binds each name it takes from outside to the address the tables give it. The whole link resolves each name some object defines, and lays out each section's objects from where its row starts the section, or, without a row, where `tools/config.py` starts it, so a difference shows a fill rule the splicer models wrongly, a seam the tables place wrongly, or a reference that resolves elsewhere. The [section-boundary tables](#section-boundary-tables) come out the same from its `STARTOF` and `SIZEOF`, which take those starts and, where `tools/config.py` ends a section's extent, that end: the whole link reserves the rest of the extent past the section's objects, as a source's own link does.

Where the tree lacks what one link of every object needs, the whole link stands in for it. A name the tables give inside the image that no object defines, such as a static variable a `gen/` range points into or a label `strt1.src` doesn't export, is bound at its address, as a source's own link binds it, and the whole link checks the references to it no further than the splice does. Bss that no object holds, ahead of an object the tables place, is reserved, so the objects after it link where their rows put them. relink.py's docstring gives each kind of stand-in, and `make relink V=1 -W tools/relink.py` links every image again and lists each one.

`make shift` runs the **shift test** on each whole link that matches its splice. The test links the image twice more, each time shifted by a different multiple of 32 bytes, with the names the image takes from other images left where they are. Every aligned 32-bit word must move by each shift, or keep its value under both.

The image's span runs from its first section's start to its last's end, in either of RAM's two address ranges ([Images](#images)). A word that keeps a value in the span is a candidate **fixed address**: a number where a relocatable build needs a name, an address the game fixes whatever the image's layout, such as where a file loads over the image, or data that only looks like one. The first takes a name in its image's `symbols.txt`, which the source that holds it declares `extern`, so the link writes the address. [`config/kept.txt`](config/kept.txt) gives the reason each other candidate takes no name, such as a glyph's bits or two instructions, and the test fails on a candidate it gives no reason, and on a row that holds no candidate. `build/relink/fixed.txt` lists each candidate, with what writes it and the reason.

## What writes each byte

The splicer builds each image from a buffer of zeros, never from a copy of the original. It writes only these, each checked against the original:

| Writer | Bytes |
|---|---|
| A source | The code, constants and data its object compiles from the tree's C or assembly, where its rows place them. |
| A declared input | Data `tools/data.py` extracts from the disc: a `gen/` range, or a body, text or value a source imports. A range's `continues` declaration adds the padding before it ([Fill](#fill)). |
| Storage | A source's bss, which the file stores as zeros. |
| Fill | The linker's fill, under the rules of [Fill](#fill). |

A stored byte none of these writes fails the splice, which names the image, the range, its section and whether it is code or data, and leaves no image. So does a byte two of them write, even where their values agree. Without the disc, `make check` and `make progress` still fail on code the placements and fill leave unwritten.

Naming each byte's writer doesn't say how the original was written or built:

- Imported data is the disc's: the accounting counts it as `generated`, whatever the route of the source that imports it.
- Some sources named by address, `dat_<address>.c`, keep bytes whose declaration and owner are unknown, and their comments say so.
- `src/draw/action_link.c`'s alignment and `sdk/ninja/njCameraMotion.c`'s grafted object are accepted reconstructions, not records of how the original was built ([Patched R10](#patched-r10), [Grafted R10](#grafted-r10)).

## Accounting

Three views of the build answer different questions, and none stands in for another:

| Question | Run |
|---|---|
| Does each rebuilt file equal its original, every byte written as the rule above requires? | `make`, with the disc |
| What writes each byte, and how much of the code is C or assembly? | `make V=1 -W tools/splice.py` for each image's bytes; `make progress` for all images' code and README.md's numbers |
| Which assembly is still open, and how many inline bodies, `#pragma asm` blocks and `.src` functions take each `@asm` kind? | `make review`, which checks no claim ([Assembly evidence](TAGS.md#assembly-evidence)) |

A C byte is code SHC compiled from C, not always its output unchanged: patched and grafted code count as C. An `@asm` tag's [basis](TAGS.md#assembly) is evidence, not proof.

### Units

- A count is of stored bytes, per image. Bss past an image's file isn't stored, and counts nowhere.
- A source placed in several images counts in each, so totals measure images, not distinct sources. A repeated event has no separate image or code count, though its disc file is checked.
- An image's **code regions** are its stored code bytes, as `Config.code()` in `tools/config.py` gives them: fixed ranges in `1ST_READ.BIN` and `LOADER.BIN`, which open with the entry's 48 bytes in the data section `DSGLH`, and elsewhere the sections whose names start with `P`. Of those, the `PSG` sections and an event's C runtime are SDK code, as are, in `1ST_READ.BIN` and `LOADER.BIN`, the entry, `P` up to `sbinit.c`, and the SDK members that end `P`. The rest is game code.
- The code regions hold instructions, literal pools, the switch tables SHC writes, SHC's `00 09` padding, data that assembly keeps in code, the space assembly reserves with `.RES`, and the linker's fill. README.md's **Code bytes** leave that fill out ([C and assembly](#c-and-assembly)).
- A change to what counts as code changes the denominator, and carries its own evidence ([What passing means](CONTRIBUTING.md#what-passing-means)).

### Categories

Each byte an image stores falls in one category. In each image the categories sum to its size, and over its code to the code's size; the splicer fails where they don't. `make V=1` lists them in this order; `make progress` counts code in all but `generated`:

| Category | Bytes |
|---|---|
| `c` | A source SHC compiles straight to an object. |
| `shckit` | A source in `PATCHED_C`: R10's assembly text, patched by `build/shckit` and, where it has an `ALIGN`, realigned by `tools/align.py` ([Patched R10](#patched-r10)). |
| `grafted` | The source in `GRAFTED_C`: R10's text with four of its donor's functions set in by `tools/graft.py`, then patched ([Grafted R10](#grafted-r10)). |
| `inline` | A source in `INLINE_C`, neither patched nor grafted: C with inline assembly, whose R10 text asmsh assembles unchanged. |
| `assembly` | `.src` files. |
| `generated` | Data from the disc that `tools/data.py` writes as C, with the padding `continues` declares. Never code. |
| `fill` | The linker's fill between objects and sections, including the gaps a `gen/` range's `end` or `start` bounds ([Fill](#fill)). |

Each source counts in its route's category, less any data it imports, so `inline` holds a source's C and its inline assembly alike. Patched and grafted sources can hold inline assembly too.

### C and assembly

`make progress` writes README.md's numbers between its `<!-- progress -->` and `<!-- /progress -->` lines, and `make check` fails where they're stale. Each row counts the game code of the images `AREAS` in `tools/progress.py` assigns it; the SDK libraries' row counts every image's SDK code, and gives no binary count, since the other rows count those images.

- **Code bytes** are a row's code regions less the linker's fill: the bytes that are C or assembly. The headline's 100% covers the whole code regions, fill included: source placements and justified fill must account for every byte.
- **C** is the categories `c`, `shckit`, `grafted` and `inline`, less the assembly they hold. SHC's own lines around inline text are C: the alignment after each copy of an inline body and after each block, and the `RTS` and `NOP` after an inline body's own definition ([Inline text](https://github.com/reshaped/shckit/blob/e0b8412e337103b59ea2f0d58dd7be52699222b9/docs/patches.md#inline-text-and-the-alignment-after-it)).
- **Assembly** is the `.src` files, and each inline body and `#pragma asm` block by its own bytes, at every copy SHC writes. Space assembly reserves with `.RES` counts as its bytes.
- C's share of the code bytes is rounded down to a tenth of a percent, and assembly's is the rest, so the two make 100.0%, and C shows 100.0% only where there is no assembly.
- An assembly instruction or data byte has a basis where its `@asm` tag names one, and is open otherwise. A `.src` function's tag covers its bytes from its first label to the next function's, so a file's instructions and data ahead of its first function are open. The space a `.RES` reserves is neither, under any tag: it holds no instruction or datum a basis could speak to. The basis's share counts the assembly's instructions and data, and leaves that space out. It is rounded down too, and open's is the rest. A share under a tenth but not none shows as `<0.1%`, and none as `0%`.

`tools/progress.py` takes each line's bytes from asmsh's listing beside each object it assembles: `build/PATH.lis`, or `build/stub/PATH.lis` where the stub's object is the newer:

- A pool word belongs to the lines that load it. The branch past a pool, its slot and its alignment belong to the line asmsh puts the pool after.
- An inline body or block the listing lacks fails the numbers, and so does a pool word that both C and assembly, or both a basis and open assembly, load.

## Releases

The build runs three releases of Hitachi's SHC 5.0 under wibo. R32's asmsh, lnk and cnvs assemble, link and convert to S-records.

| Release | `shc.exe` PE timestamp | Builds |
|---|---|---|
| R32 | 1998-10-20 | Game code, events, the loader's own code, the C runtime, the `gen/` ranges, and, of the main executable's SDK 1.30J members, Kamui, the GD file system, and the GD and ADX decoders, `sdk/shinobi/gdDec.c` and `sdk/sofdec/adx_dcda.c` |
| R10 | 1998-04-17 | The other SDK members, among them Ninja, Shinobi, sound, Sofdec and 1.20J's `kmutil_workaround_for_sh_lnk_`; the movie's SDK members and the loader's own, under `loader/sdk/`; and every source with inline assembly |
| R26 | 1998-06-25 | Shinobi's overlay library, `sdk/shinobi/ov_main.c` |

C normally compiles straight to an object, since where SHC aligns code to 32 it pads it with `00 09`, and asmsh with `09 00` ([Fill](#fill)). R10 writes assembly text only where the build needs it: for inline assembly, which R10 accepts only then; for a patched source; and for the grafted one.

The bytes don't always pick one compiler:

- **R32.** R31 wrote the same assembly as R32 for every R32-built C source in `src/`, `modules/`, `events/` and `loader/`, so the bytes don't choose between them. The build pins R32.
- **R10.** R10 compiles `sdk/shinobi/strt2.c` to the image's bytes; R26, R27, R28, R31 and R32 don't. R10's object orders its sections `P`, `C`, `D`, and SDK 1.20J's member `P`, `D`, `C`, as R10's assembly text does. Linking erases the difference, so the image can't decide.
- **R26.** R26, R27 and R28 match `ov_main.c`. R31 and R32 agree with each other, not with the image.

Library banners date the SDK members on their own. The main executable has SDK 1.20J's `Shinobi Ver 1.25 Build:Nov 05 1998` and 1.30J's `gdFs Ver 1.05 Build:Dec 02 1998`; the loader has 1.20J's `gdFs Ver 1.02 Build:Oct 27 1998`.

### Patched R10

A patched source compiles with R10 to assembly text. `build/shckit` applies the source's named patches, and asmsh assembles the result. A patch reads only the text, never the image. `PATCHED_C` in `config/compilers.mk` names these sources, and `PATCHES` lists each one's patches; shckit's [patches.md](https://github.com/reshaped/shckit/blob/e0b8412e337103b59ea2f0d58dd7be52699222b9/docs/patches.md) gives what each does, refuses and rests on.

Each source's patches answer a concrete difference between R10's text and the image. Unqualified names are under `sdk/ninja/`.

- **`njControl.c`** gets [align-pool](https://github.com/reshaped/shckit/blob/e0b8412e337103b59ea2f0d58dd7be52699222b9/docs/patches.md#align-pool). R10 writes every instruction, but its pool starts differently: at `0x8c6298c6` the image has a `NOP` and four zero bytes before the long at `0x8c6298cc`.
- **`njModel.c`, `njModelE.c` and `njModelS.c`** get [drop-inline-align](https://github.com/reshaped/shckit/blob/e0b8412e337103b59ea2f0d58dd7be52699222b9/docs/patches.md#drop-inline-align), then align-pool. The image has no `NOP` after inline text that ends on a half word: at `0x8c6299b2` and `0x8c62bbfa`, R10's `RTS` follows the inline body directly.
- **The motion members**, `njMotion.c`, `njMotionE.c`, `njGetMotion.c`, `njMotLinkS.c`, `njMotLinkSrc.c`, `njMotSrc.c` and `njMotionS.c`, get [use-assembler-pools](https://github.com/reshaped/shckit/blob/e0b8412e337103b59ea2f0d58dd7be52699222b9/docs/patches.md#use-assembler-pools), then [drop-r0-move-nop](https://github.com/reshaped/shckit/blob/e0b8412e337103b59ea2f0d58dd7be52699222b9/docs/patches.md#drop-r0-move-nop). The image has R10's instructions with literals where asmsh lays them out, and no `NOP` after a `MOV Rm,R0` outside a delay slot.
- **`njMotionE.c`, `njMotLinkSrc.c` and `njMotSrc.c`** get drop-inline-align first. The image runs straight on to the `MOVA` at `0x8c615c3a`, the `FMOV.S @R12+,FR6` at `0x8c65424e` and the `ADD #8,R15` at `0x8c655f32`. Their inline bodies give their sizes, `size=n`: without one, R10 can put a pool ahead of the body, which use-assembler-pools refuses.
- **`njMotLinkSrc.c`** gets [fill-call-slot](https://github.com/reshaped/shckit/blob/e0b8412e337103b59ea2f0d58dd7be52699222b9/docs/patches.md#fill-call-slot) last. R10 puts a `NOP` in the slot of each shape node step's `JSR` and the record pointer's `ADD` after it; the image has the `ADD` in the slot, at `0x8c6556b4` and `0x8c6556f2`. `njMotSrc.c` keeps its slot `NOP`s, at `0x8c656bba` and `0x8c656be4`.

`src/draw/action.c` and `src/draw/action_link.c` are game code, and get use-assembler-pools alone. shckit documents that patch for Ninja's motion members, whose literals sit where asmsh lays them out, and rules it out for code that holds SHC's own pools ([Where it applies](https://github.com/reshaped/shckit/blob/e0b8412e337103b59ea2f0d58dd7be52699222b9/docs/patches.md#where-it-applies-and-where-it-must-not-2)). These two hold asmsh's layout too, and the build shows it: the patch has asmsh lay out every literal anew, and both still match the disc. As with any match, that proves the bytes, not how the original was built. Unlike the motion members, they keep the `NOP` after `MOV Rm,R0`, at `0x8c040f4e` and `0x8c041b7c`; their sized inline bodies end on 4, so the alignment before the `MOVA`s at `0x8c04126c` and `0x8c041fc0` adds nothing.

`src/draw/action_link.c` also has `ALIGN = P 32`, for the 28 zero bytes between `action.c`'s code and its own, which starts at `0x8c0419e0`. `tools/align.py` declares the section on 32 in the patched text, and the splicer writes the 28 bytes as the linker's fill. `config/compilers.mk` gives the evidence beside the setting. Only a patched source's full build runs `tools/align.py`, not its stub build, so an `ALIGN` on any other object fails the build.

### Grafted R10

The grafted route combines R10's assembly text of two C sources. `GRAFTED_C` names `sdk/ninja/njCameraMotion.c`, and `DONOR_C` its donor, `sdk/ninja/njCameraMotionOld.c`, SDK 1.00J's camera motion. The object at `0x8c6537e8` holds the source's fourteen functions, then from `0x8c653df2` four of the donor's, renamed:

| In the image | In the donor |
|---|---|
| `apply_old_channels` | `apply_channels` |
| `old_roll_channel` | `roll_channel` |
| `njCameraActionOld` | `njCameraAction` |
| `njCameraMotionOld` | `njCameraMotion` |

R10 writes assembly text for both sources. `tools/graft.py` inserts the donor's four functions after the source's own `njCameraMotion`, then the source's `PATCHES` apply, and asmsh assembles the result. [`tools/graft.py`](tools/graft.py)'s docstring gives how it binds their symbols and pools, and what it refuses. The object is checked like any other, and its bytes count as `grafted` in the accounting.

## Flags

[`config/compilers.mk`](config/compilers.mk) records the `GAME`, `SDK` and `RUNTIME` flag sets and each source's overrides. They reproduce the bytes; a match doesn't identify the original options. `njTexLoad.c`'s `b=BSG32` also keeps its SDK member's section names: the build starts that section on 4, after `DSG`, where the member's starts on 32, ahead of it.

| Flag | Effect |
|---|---|
| `-code=machinecode` | Write an object. |
| `-code=asmcode` | Write assembly text, for asmsh. |
| `-cpu=sh4`, `-endian=little` | SH-4, little-endian. Every compile uses them. |
| `-fpu=single` | `double` behaves as `float`. |
| `-double=float` | R10's spelling of it. R10 rejects `-fpu`. |
| `-speed`, `-size` | Optimize for speed, or for size. SHC warns when given both. |
| `-optimize=0` | No optimization. |
| `-noinline`, `-inline=10` | No automatic inlining, or inlining up to ten nodes. |
| `-noloop` | No loop unrolling. Without it, `mpvadec.c`'s `PSG` runs 576 bytes longer. |
| `-macsave=0` | Don't save and restore `MACL` and `MACH`. |
| `-aggressive=2` | Multiply by a floating constant's reciprocal in place of dividing by it. |
| `-extra=a=800` | Under R26 and R32, align code, pools and the code after pools to 32 bytes. R10 ignores it. |
| `-extra=a=400` | The SH-4 workaround: insert `NOP`s and reorder adjacent instructions. `syMalloc.c` gets 14 `NOP`s with it, and five without. |
| `-extra=a=c00` | `a=400` and `a=800` together. |
| `-extra=a=440` | `a=400` with bit 0x40. Only `njMem.c` uses it, and `config/compilers.mk` says why. |
| `-section=p=PSG,c=CSG,d=DSG,b=BSG` | Rename the code, constant, data and bss sections. `ov_main.c` uses `d=DSGLH`, and `njTexLoad.c` `b=BSG32`. |
| `-listfile=FILE`, `-show=object` | Write a listing with the object's bytes, from which a stub build's bodies are located. |
| `-sub=FILE` | Read the options from FILE. |

asmsh assembles each source's text with `ASMFLAGS`: `-cpu=sh4 -endian=little`, `-sjis` for Shift JIS text, and `-private`, without which it rejects `FSCA` and `FSRRA`.

## Running SHC

SHC and R32's asmsh, lnk and cnvs are 32-bit Windows programs, and the build runs them under wibo 1.2.0. `WIBO` names wibo itself. `R10`, `R26` and `R32` each name a release's directory, which holds `shc.exe`, and `R32`'s also `asmsh.exe`, `lnk.exe` and `cnvs.exe`, in lowercase. A goal that compiles stops where a release lacks `shc.exe`, but make checks R32's other three names only in a release it downloads, so an `R32` you supply must hold them too.

- Each compile sets `SHC_LIB`, `SHC_INC` and `SHC_TMP`, the last a directory of the object's own under `build/tmp/`: R10 names its temporary files from its process id's digits, so compiles that share a directory can clash.
- asmsh, lnk and cnvs take `/` for an option prefix, so the build gives them DOS paths.
- A command line past SHC's parameter buffer fails with error 3320, so a stub compiled to an object reads its options from a `-sub=FILE` response file.
- These tools report some warnings with status 0, so a compile or an assembly fails on any error or warning it reports, except the one warning a source's `ADMITTED` names, which it may report once. Linking fails on any message.

Paths have limits:

- Source paths must not contain hyphens: SHC rejects them with error 3305.
- A source's file name holds one dot: asmsh and cnvs reject more in the names the build gives their files.
- R32 rejects an `SHC_LIB` of 111 characters or more, and R10 and R26 one of 116 or more, with error 3321. Keep the release directories' paths short.
- The build's paths are relative to the checkout, so the checkout's own path may hold spaces.

## C for SHC

The game's sources are C89, as SHC 5.0 accepts it: no `//` comments, and no declaration after a statement. The vendored tools are C99, for the host's compiler.

**Order is layout.** Bss follows the first declaration of each name, `extern` declarations in headers among them. Initialized data follows definition order. Constants and a function's strings follow source order, and strings an initializer names follow them all, in definition order. So moving a declaration or an include can move data.

`Sint32` is `long`. `int` has the same width but can compile otherwise: SHC compares a `long` with an `unsigned int` as signed, where C89 converts both to `unsigned long`. It compares an `int` with an `unsigned int` as unsigned, as C89 does.

Inline assembly, `#pragma inline_asm` and `#pragma asm`:

- R10 accepts it only when it writes assembly text, and writes lines of its own after each piece ([Inline text](https://github.com/reshaped/shckit/blob/e0b8412e337103b59ea2f0d58dd7be52699222b9/docs/patches.md#inline-text-and-the-alignment-after-it)).
- `#pragma inline_asm(f(size=n))` gives an inline body's size, by which R10 places pools. A larger size can move a pool ahead of the expansion.
- A nonempty `#pragma asm` block, even one holding only an assembler comment, flushes the pending pool behind a branch. An empty block writes nothing.
- SHC writes a block ahead of the section line of the function after it, so a block ahead of a section's first function names that section itself.
- An inline body is hidden from Clang with `#ifndef __clang__`, with formatting off. A block outside a function is hidden the same way, and ends in an empty assembler comment, whose `;` ends the text clang-format reads as C.
- An SHC extension in C is hidden the same way, with its ISO C spelling under `#else` for Clang. `loader/sdk/kmsurfac_workaround_for_sh_lnk.c` sets a pointer through a cast used as an lvalue, which R10 accepts with warning 1022; its `ADMITTED` permits that warning once.

R10 builds inline assembly in `sdk/`, `src/` and `modules/movie/sdk/`. Anywhere else, a source with inline assembly needs a rule of its own, its release and flags shown by a byte match. Without one, make stops rather than guess.

## Assembly sources

R32's asmsh assembles each `.src` file under `ASMFLAGS`. A single-precision constant uses an `S` exponent, as in `.FDATA.S F'3.4S+38`; an `E` exponent draws warning 826, `ILLEGAL PRECISION`. `make lint` holds every `.src` file to shckit's [check](https://github.com/reshaped/shckit/blob/e0b8412e337103b59ea2f0d58dd7be52699222b9/docs/check.md) and [lint](https://github.com/reshaped/shckit/blob/e0b8412e337103b59ea2f0d58dd7be52699222b9/docs/lint.md).

An exported label's comment gives its C declaration, where it has one, and what it does. A function's one `@asm` tag ends the comment above one of its labels ([Where the tag goes](TAGS.md#where-the-tag-goes)), as it ends `__SetSP`'s in `src/system/crt0.src`:

```text
; void _SetSP(void *sp)
; set the stack pointer to sp
; @asm abi: stack-written, MOV R4,R15 at 0x8c010388 sets the stack pointer
__SetSP:
```

For exported labels, `shckit lint` requires a comment but doesn't check its contents.

## Images

Each binary loads at one of these addresses:

| Load address | Binaries |
|---|---|
| `0x8c010000` | `1ST_READ.BIN`, the main executable |
| `0x0c900000` | a stage, a boss, a field or a minigame; `AL_MAIN`, the code the gardens and the Chao race share; the menus or `SUMMARY` |
| `0x0c920000` | the Egg Carrier, outside or inside, beside `ADV01OBJ` |
| `0x0cb00000` | Tikal, beside Perfect Chaos or the Past |
| `0x0cb80000` | a Chao garden or the Chao race, beside `AL_MAIN`; or an event |
| `0x8ce00000` | `LOADER.BIN`, the hand-off to Dream Passport |
| `0x8ceb0000` | `MOVIE.BIN`, the movie player |

The Dreamcast's RAM appears at both `0x0c000000` and `0x8c000000`, and the game uses both. `ADVERTISE.PRS` (the menus) and `SUMMARY.PRS` link at `0x8c900000`, not where they load: the two addresses reach the same RAM.

- `1ST_READ.BIN`, `LOADER.BIN` and `MOVIE.BIN` are stored as they load. Every other binary is a `.PRS`, and its image is the file decompressed.
- Each image has a lowercase directory in `config/`, whose `splits.txt` opens with the binary's name: `# STG01.PRS, Emerald Coast`. Its file is `SONICADV/NAME.PRS` unless `FILES` in `tools/config.py` names another: `1ST_READ.BIN`, `LOADER.BIN` or `SONICADV/MOVIE.BIN`.
- An image links where its first section starts. `tools/config.py` links `1ST_READ.BIN` at `0x8c010000`, ends its file at `0x8c688100` and runs its bss on to `0x8c8fd160`.
- Four of the disc's 208 events repeat others byte for byte, as `EVENT_ALIASES` in `tools/config.py` records. Each is checked as an original, and has no image of its own.

Most of the loader's SDK members are the main executable's sources, linked again at the loader's addresses. The rest are its own SDK 1.20J members under `loader/sdk/`, which R10 builds. Eight of them include a source R32 builds for the main executable, to compile the same C with R10 and different flags: `loader/sdk/gdDec.c` includes `sdk/shinobi/gdDec.c`, and seven `*_workaround_for_sh_lnk.c` members include their `sdk/ninja/*_kamui_lib.c` counterparts.

An event's file opens with its head, in `DHEAD`, from the event's `head.c`: the entry and texture list `src/event/play.c` reads. The file ends with `D`. `P`, and `C` where there is one, end on 8, and `B` starts at the file's end rounded up to 8.

### Stage loading

`loader_read_stage_files` in `src/system/loader.c` picks binaries by the stage numbers `src/stage/stage.h` defines. Names are PRS files in `SONICADV/`.

| Stage numbers | Binaries |
|---|---|
| 0-10, 12 | `STG00`-`STG10`, `STG12` |
| 15-18 | `B_CHAOS0`, `B_CHAOS2`, `B_CHAOS4`, `B_CHAOS6` |
| 19 | `B_CHAOS7` and `TIKAL_PROG` |
| 20-22 | `B_EGM1`, `B_EGM2`, `B_EGM3` |
| 23 | `B_ROBO` |
| 24-25 | `B_E101`, `B_E101_R` |
| 26-28 | `ADV00` |
| 29 | `ADV01OBJ` and `ADV0100` |
| 32 | `ADV01OBJ` and `ADV0130` |
| 33 | `ADV02` |
| 34 | `ADV03` and `TIKAL_PROG` |
| 35 | `MINICART` |
| 36-37 | `SHOOTING` |
| 38 | `SBOARD` |
| 39-41 | `AL_MAIN` and the corresponding `AL_GARDEN00`-`AL_GARDEN02` |
| 42 | `AL_MAIN` and `AL_RACE` |

`src/main.c` loads `LOADER`, `ADVERTISE` and `SUMMARY`; `src/progress/movie.c` loads `MOVIE`; and `src/event/play.c` loads each event by its number in four hex digits, as `EV0001.PRS`.

## Image tables

An image's `splits.txt` places its sources' sections, and its `symbols.txt` names addresses. Every Python tool reads them through `tools/config.py`, which fails a malformed row, naming its file and line. A `splits.txt` row has one of three forms:

```text
SECTION START END
SOURCE SECTION START END [BOUNDARY...]
fill BYTE
```

From `EV0001`'s:

```text
fill                    0xff
P                       0x0cb80008  0x0cb83018
events/shared/fade.c    P           0x0cb82880  0x0cb82a0c
```

- A range is `[START, END)`, in linked addresses. An address is `0x` and eight lowercase hex digits, and a fill byte `0x` and two. `#` starts a comment.
- A source's first placement in a section starts its object's section, and no later one in that section starts below it; the highest ends where the object's section does. Linking fails on a missing section, on an unplaced section that holds bytes, and on a byte written outside the source's ranges.
- A `BOUNDARY` applies only to a `gen/` range: `end`, `start N` or `continues N`, `N` a power-of-two alignment in decimal, each at most once, and not both `start` and `continues` ([Fill](#fill)).

```text
gen/stg08/d_0cac2838.c  C  0x0cac2838  0x0cac2f5f  continues 4  end
```

The section rows, together with justified fill, cover the stored image. Each section has one row; the rows run from the first one's start to the file's end, with only bss past it. Gaps between rows must be justified by [Fill](#fill). A stored byte that no row or justified fill covers fails, and so does a row that doesn't span the extent `tools/config.py` gives its section ([Section-boundary tables](#section-boundary-tables)).

`1ST_READ.BIN` has no section rows: its sections, and the code ranges of it and `LOADER.BIN`, are fixed in `tools/config.py`. In every other image, a section's name decides whether its bytes are code ([Units](#units)).

A `symbols.txt` row is `ADDRESS NAME`, the name as in C, without the object format's leading underscore. Each name has one row; several names may share an address. `make relink` fails where the whole link defines a name at another address than its row; it reads only the names objects export, so a static's row goes unchecked. A table may name a binary loaded beside its own, as a garden's names `AL_MAIN`'s. Every image but the loader (`ALONE` in `tools/config.py`) also takes `1ST_READ.BIN`'s names outside its own span, in linked addresses, where its own table doesn't give them.

### Sections and fill bytes

- An image stores no section names. The tables use SHC's `P`, `C`, `D` and `B`, and the names `-section`, `#pragma section` and `.SECTION` give, such as `PSG`, `PHEAD`, `CHEAD`, `DHEAD`, `DSGLE` and `ADVERTISE`'s `DEND`.
- The fill byte is `0x00`, and an event's `fill` row gives `0xff`. It fills what linking leaves unwritten: the linker's gaps, and the space an object reserves.
- Bss, `B`, `B32`, `BSG` and `BSG32`, holds zeros whatever the fill byte.
- An image's bss may run past its file; `1ST_READ.BIN`'s isn't stored at all. The splicer checks the part the file stores, and requires a linked source's bss past it to be zero. A `B` row past the file ends with the last source's bss, unless its table records evidence for another end.

### Section-boundary tables

SHC does not compute linked section bounds, so binaries that store those bounds use assembly tables, with `STARTOF` and `SIZEOF`:

| Table | Image | What it holds |
|---|---|---|
| `sdk/shinobi/strt1.src`, at the starts of `C` and `CSG` | `1ST_READ.BIN`, `LOADER.BIN` | Sections' bounds |
| `modules/movie/sdk/mpvasec.src` | `MOVIE.BIN` | The bounds of `PSGSFD00` to `PSGSFD03` |
| `bounds.src` at the end of `C` | `B_ROBO`, `B_E101_R` | The starts of `D`, `R` and `B`, and the ends of `R` and `B` |
| `bounds.src` as `DEND` | `ADVERTISE` | `B`'s start and `DEND`'s own end |

`tools/config.py` records these extents, and the rows keep to them. Except in `1ST_READ.BIN`, a section whose extent holds bytes has a row that starts at the extent's start and spans it, running on at most to the next section. Where `D32` or `DSG32` follows `D` or `DSG`, the latter's extent ends at its counterpart's start, and a counterpart that holds bytes has its own row or lies in its section's. A placement in a section with an extent lies inside it. [`tools/link.py`](tools/link.py)'s docstring gives how it starts and reserves such sections, so that `STARTOF` and `SIZEOF` give the original values.

## Objects

A source is one complete object, and its rows place every section of it that holds bytes. The boundary between objects matters even where two arrangements make the same instructions: padding, constants, inlining and strings can show a boundary code alone doesn't. Where fill and alignment don't show it, the comment beside the row records the evidence ([Seams in the tables](#seams-in-the-tables)).

### Fill

An object's section ends at its start plus its declared size, the space it reserves included. The linker starts the next object at that end rounded up to the next object's alignment, and the gap holds the image's fill byte, or zero in bss.

A section starts on its first object's alignment. The splicer's candidates for it:

| Evidence | Alignment |
|---|---|
| A row places an object at the section's start | That object's |
| No row does | Each placed object's, standing in for the first |
| None is placed, in `1ST_READ.BIN` | That of `FIRST_EMPTY` in `tools/config.py`: `strt1.src`, which declares the empty `D32` and `CSG32` on 32 |
| None is placed, elsewhere | Both 4 and 8, on which R10 and R32 start data; none for a code section |

Every candidate must give the same start. Standing in assumes each later object's alignment gives the start the first object's does, which can fail: `1ST_READ.BIN`'s `C` starts on 4, at `0x8c1283cc`, with strt1's constants, though the R32 objects after them are on 8. A row places `strt1.src` there, so its alignment decides. In `STG12`, the last `P` placement ends six bytes before a `gen/` range starts `D`, and the objects placed in `D` give the alignment, 8.

The splicer writes a gap as the linker's fill only where the tables show it:

- **Between two sources in a section,** where the second starts exactly where its object's alignment puts it after the first's last row, and no object's section spans the gap.
- **From a section's last placement to the next section,** where that placement is a source's and every candidate alignment for the next section rounds its end to the next section's start. The gap may lie partly inside the section's row and partly between rows. Inside the section's row, no fill follows a `gen/` range that is its last placement.
- **Between section rows,** where every candidate alignment gives the later start, or an extent does ([Section-boundary tables](#section-boundary-tables)). The loader's `D` starts at `0x8ce06cc0`, 24 bytes after `C`, where strt1's table puts it, though its first object declares 4.
- **At `1ST_READ.BIN`'s fixed boundaries:** from the last placements in `P` and `PSG` to `C` and `CSG`, and from those in `D`, `CSG` and `DSG` to the starts of `D32`, `CSG32` and `DSG32`.

A gap not justified by these rules must be emitted by a source; otherwise the build fails.

A `gen/` range's bytes show no object's size or alignment, so no gap runs from one, or to one within a section, unless its row declares the boundary, with its evidence in the comment beside the row:

| Declaration | Meaning | The gap it allows |
|---|---|---|
| `end` | The range ends at its object's section end. | Fill from the range to the next object's aligned start, or to a range that declares `start N`. |
| `start N` | The range starts an object aligned to `N`. | Fill before it, from a source's last row or a range that declares `end`. |
| `continues N` | The range continues the preceding source's object, whose strings are padded to `N`. | Padding inside that object, not linker fill. |

Each gap must match the rounding exactly and hold the fill byte, and an unused declaration fails. `start N` also needs `N` among the section's candidates in the table above: an object placed in the section on `N`, standing in for the range's, or, with none placed, 4 or 8. `continues N` can't exceed 4: SHC aligns no single constant, datum or bss item past 4. **Alignment validates a declared boundary; it does not establish one.**

SHC, where it aligns code to 32 under `-extra=a=800`, pads inside an object with `00 09`, a byte-swapped `NOP`, and asmsh with the true `NOP`, `09 00`, so assembled code shows fewer boundaries. Elsewhere, as in events, an object SHC writes pads code only by reserving space, which holds the fill byte ([Sections and fill bytes](#sections-and-fill-bytes)). The splicer fails a game-code boundary beside SHC's `00 09` padding. Outside events, which build without `-extra=a=800`, it also fails a game-code start off 32, and an end off 32 that no fill follows. It checks neither SDK code nor either end of a game-code region ([Units](#units)), such as where `PHEAD` ends and where `P` starts, in 20 images.

### Alignment

- With `-extra=a=800`, R26's and R32's code sections, pools and the code after pools align to 32. Even a final pool with no code after it rounds the object's code section up to 32, so two functions can meet right after a pool, across an object boundary or not.
- Without it, as in events, or under R10, code sections align to 4, and a pool aligns only its long words to 4, so a pool, and the code after it, can start off 4.
- R10's machine-code objects start every section on 4. R32 starts constants, data and bss on 8, but aligns the items in them to at most 4. So a 4-byte gap before an item on 8 can mark an object boundary, and where R32 builds every object in a section, two items meeting off 8 belong to one object.
- An assembled object's sections align as its text's `.SECTION` lines say. A patched source's `ALIGN` can raise one ([Patched R10](#patched-r10)).

### Literal pools

SHC can defer a function's literals to a pool after later functions, so combining sources can move a pool and the code after it. A section change ends the code before it with a pool. A literal load reads its own object's pool, and a call to another object loads a pool word rather than using `BSR`, so a literal load or a `BSR` across a function boundary shows both functions are in one object.

### Inlining

Under `-speed` or `-inline`, SHC may expand a small function defined anywhere in the same file. `-noinline` stops it, and `-optimize=0` stops `-speed`'s. `#pragma inline` expands the functions it names even under `-size`, `-noinline` or `-optimize=0`. Neither expands a call in an operand that `&&`, `||` or `?:` may skip. A `static` function expanded at every call, whose address is never taken, leaves the object. So combining files can bring an expansion across a proposed boundary.

### Strings

SHC merges identical string literals within one file, and separate objects keep separate copies. So combining sources can move strings without changing them.

### Seams in the tables

A **seam** is a boundary between two objects that fill and alignment don't decide. The comment beside the row records the evidence, mostly from two experiments:

- **As one file:** concatenate the sources, keeping any generated data between them and reconciling declarations, compile with the first source's flags, and link each section at the lower of their two starts. "N of M code bytes differ" counts bytes, where an `@hack` tag's count is in 2-byte words. M counts the original's bytes in each code section, from the lower start to the last byte the two separate links write. The space an object reserves after its code, to end its section on 32, lies inside M only where the other object's code follows it. N counts the bytes among those M that the one file builds differently. Where the one file's code runs shorter or longer, the comment says by how much, then counts among the one file's own bytes: all of them, M less the difference, where it runs shorter, and its first M where it runs longer. Where only pool words differ, the comment names the data that moved. "One file or two build the same" leaves the seam open.
- **Split there:** cut an `-extra=a=800` source at an unpadded function on 32, keeping declarations and data in both files and declaring the calls between them. "The code builds the same" compares functions, not data, and alone doesn't justify a split: keep the source whole and record what else there is.

Other images help. A shared source is one object everywhere, so a seam one image shows constrains the others. Event objects often meet without fill, since their code aligns to 4, but other neighbours or a known SDK member's extent can show the seam. `events/shared/move.c` and `mover.c` always appear together, so no layout decides theirs. A boundary on 8 in constants, data or bss can move between items without changing a byte. Record what each experiment distinguishes, not just whether it matched.

## Declared data

A source imports data from the disc through a typed declaration whose initializer is a **body**, a generated `.inc` file the declaration includes alone between its `= {` and `};` lines. Its path is the source's without its first directory or `.c`, then the datum's name: `src/system/util.c`'s `font_wide` includes `system/util/font_wide.inc`. A source that includes one, or a `text.inc` or `values.inc` ([Text](#text), [Values](#values)), is a **body source**. (A body is data, unrelated to an inline body.)

```c
static const Uint32 font_wide[WIDE_GLYPHS * GLYPH_WORDS] = {
#include "system/util/font_wide.inc"
};
```

Keep array extents in the declaration: with `[]`, the stub's zero body would make the array one element long. A function's `static` imports a body the same way. An automatic array needs constant extents; SHC initializes it from a template in the constants, which the stub listing locates by its size and the function that loads it.

### Stub and full builds

The [Makefile](Makefile)'s `BODY_C` finds the body sources by a line ending `include "PATH.inc"`. Only the game's, R10's and the patched routes have a stub rule, and make stops on a body source on R26's, the runtime's, R32's SDK, the grafted or the inline-assembly route, and on one in the loader's own code or a patched source in `src/`, for which the game's and the patched stub rules search other headers than its route does. Each compiles twice, both times as its route says:

- The stub build compiles with zero bodies, zero values and placeholder texts, from `build/stub/`, so it needs no disc. Its listing, SHC's or for a patched source asmsh's, locates each body.
- The full build compiles with the bodies, texts and values `tools/data.py --bodies` extracts from the original, using the stub's listing to locate them. The generated `.inc` files and `bodies.txt`, which gives each item's address, size and type, go under `build/gen/<source path>/`.

The splicer requires the two builds' linked sources to differ only inside the bodies, texts and values `bodies.txt` lists, and fails one that lies in code. A source several images place reads its bodies from its **home**, the first of those images by name; every image is still checked against its own original. Lint reads the stub bodies, so a header only the full bodies need keeps its include with `/* NOLINT(misc-include-cleaner) */`.

### Types and pointers

A body may use Ninja's types, integers, `Float`, `char` and the types the source or its headers declare. `tools/data.py` lays them out as SHC does, following structs, unions, enums, typedefs and object-like macros, and rejects bit-fields, `double` and a name defined two ways.

- Struct padding must be zero in the original. A union is initialized through its first member. A multidimensional array of records keeps its inner braces, one of scalars is written flat, and a pointer into either keeps its subscripts.
- A pointer keeps its declared type. A cast drops `const` or `volatile` only where the destination needs it, as for `lap_label_frame` in `modules/minicart/lap.c`.
- A pointer into an `extern` array can be an element, where no other named address lies between. A pointer to a union's first array member can name that member, as `dome_uvs.uv` does.
- A value outside memory, such as an offset a loader relocates, stays a cast number.

A body points only at the source's own data, at a prototyped function's start by its name, or at a symbol in the image's tables declared `extern` with a one-word or `struct` type. A datum of the source's own that a body points at is the first declarator on its line, with a one-word or `struct` type, or a file's union or untagged struct: `tools/data.py` refuses a pointer to any other. A function only a body names must not be `static`: the zero-body stub would leave it out and move the code after it. The stub would leave out a string literal only a body points at, too, so:

- a table of strings and pointers stays C, its strings `TEXT_n` ([Text](#text));
- a table that mixes strings and numbers stays C as well, its numbers `VALUE_n` ([Values](#values));
- a string a body points at becomes a named `char` array with a body of its own. SHC aligns that array to 1 but a string literal to 4, so this works only where both place the bytes alike, as for the land's name in `modules/b_e101/land.c`, its object's only `CHEAD` constant.

### Text

A source's strings from the disc are `TEXT_n`, numbered from 1. The source defines each `TEXT_n_LENGTH`, without the terminating null, and includes its generated `text.inc`, whose path is the source's without its first directory or `.c`: `src/progress/playable.c` includes `progress/playable/text.inc`. The generator requires both.

A stub text keeps its declared length, with contents of its own so SHC can't merge two: its number, then zeros. Past 255, the generator picks a leading byte no other text of that length uses.

### Values

A record that mixes strings and numbers keeps its initializer in C: its pointers name symbols, its strings are `TEXT_n`, and each number is `VALUE_n`, numbered from 1, standing for one arithmetic member or element, as in `modules/stg01/kinds.c`:

```c
{VALUE_1, VALUE_2, VALUE_3, VALUE_4, NULL, ring_step, TEXT_1},
```

- A value appears once, directly in the initializer of a datum the stub listing locates: never in a macro, in code or for a pointer.
- Every aggregate has braces of its own, except a `char` array a string initializes, so the type alone decides which member each value stands for.
- The source includes `values.inc` by the same path rule: `modules/stg01/kinds.c` includes `stg01/kinds/values.inc`.
- A table of numbers and symbols without strings uses a body instead.

The stub defines each value as 0. `data.py --bodies` lays out the datum as it does a body, finds each value's member, and records it in `bodies.txt` with that member's size. Both builds name the same texts, so they differ only in the bodies, the values and the texts' contents.

## Generated ranges

`tools/data.py` reads the disc's data, Ninja's models, motions and texture lists among it, and writes it as C. Ninja here is Sega's 3D library, not the build system.

- A `gen/<image>/d_<address>.c` placement imports a data range of the original image that starts at `<address>`, on an 8-byte boundary. `tools/config.py` rejects one in code or at another address, and `tools/data.py` one off 8.
- Its row lies in `D`; in `DHEAD`, with `#pragma section HEAD`; or in `C`, with `const` data, as for strings the generated data names and no source holds, such as a land's texture names. Any other section fails.
- `build/gen/` holds the ranges' C under `build/gen/<image>/`, the bodies under `build/gen/<source path>/`, and each `.PRS` original decompressed under `build/gen/img/`.
- A range's bytes come from the disc, so only `make` checks them, as it splices.

To propose rows for data no source holds yet:

```sh
python3 tools/data.py --rows build/gen/img/STG01.BIN
```

The input is a decompressed image, here from `make build/gen/img/STG01.BIN`; for a binary the disc doesn't store as PRS, `1ST_READ.BIN`, `LOADER.BIN` or `MOVIE.BIN`, use its file in `orig/`.

An inferred type or boundary is provisional. Matching bytes don't say what the data means: the code that uses it is the evidence for a better declaration. When changing the generator, compare the generated C as well as the hashes, since equal bytes can hide a poorer declaration. Leave SHC's objects out of that comparison: they hold the time they were built.

## Hashes

Two files of hashes have different jobs.

**`tools/SHA256SUMS`** pins what the build gets from outside the tree: wibo, the three SHC archives, the clang-format and clang-tidy wheels, and each vendored tool, as a `# REPOSITORY COMMIT` line followed by the hash of each file copied into `tools/TOOL/`. The vendored tools, gdi, prskit and shckit, change in their own repositories. To take a new commit of one:

```sh
python3 tools/vendor.py TOOL COMMIT
```

[`tools/vendor.py`](tools/vendor.py)'s docstring describes the refresh and its `--check`, which `make test` runs.

**`SHA256SUMS`**, at the root, records what the build must reproduce, one `HASH  PATH` line per file, in path order as `LC_ALL=C sort -k 2` sorts them:

- each original binary file in `orig/`;
- each repeated event's file, at the hash of the event it repeats;
- each linked source under `build/`, from both full and stub builds, except a `gen/` range's;
- each `.PRS` binary's image, `build/SONICADV/NAME.BIN`.

A rebuilt file such as `build/SONICADV/STG01.PRS` is checked against its original's line, so each hash lives in one place. A `gen/` range's linked source has no line: the splice checks its bytes against the original. [`tools/sums.py`](tools/sums.py) checks that the file lists exactly these, in order. [What passing means](CONTRIBUTING.md#what-passing-means) says when a line may change.

## Tools

Each of the build's own tools is a Python file under `tools/` whose docstring gives its contract.

| Tool | Job |
|---|---|
| [`config.py`](tools/config.py) | Reads and checks the image tables and the shift test's reasons. |
| [`deps.py`](tools/deps.py) | Writes `build/deps.mk`, make's dependencies of each image and declaration scan, and `build/sources.txt`, the list of the C and the tables. |
| [`link.py`](tools/link.py) | Links one object at its image's addresses, with R32's asmsh, lnk and cnvs. |
| [`splice.py`](tools/splice.py) | Checks an image's linked sources and fill against its original, and builds the image from them. |
| [`relink.py`](tools/relink.py) | Links all of an image's objects in one pass with R32's lnk, compares the result with the splice, and runs the shift test. |
| [`code_ranges.py`](tools/code_ranges.py) | Says what writes each of an image's bytes, and finds the bytes nothing writes. |
| [`progress.py`](tools/progress.py) | Counts the code by what writes it, and writes README.md's numbers. |
| [`graft.py`](tools/graft.py) | Sets the donor's four functions into the grafted source's R10 text. |
| [`align.py`](tools/align.py) | Raises one section's alignment in a patched text. |
| [`data.py`](tools/data.py) | Lays out C types as SHC does, finds and decodes Ninja's structures, and writes C that reproduces the disc's data. |
| [`sums.py`](tools/sums.py) | Checks `SHA256SUMS` against the files make expects it to list. |
| [`vendor.py`](tools/vendor.py) | Checks the vendored tools' files, or refreshes one from its repository. |
| [`tags.py`](tools/tags.py) | Checks the tags, lists each `@asm open` tag, and names the C sources that hold inline assembly, for the routes in `config/compilers.mk`. |

The host's C compiler builds `build/prskit`, which decompresses and compresses PRS; `build/gdi`, which extracts the disc's files from a `.gdi` and its tracks; and `build/shckit`, which patches R10's assembly text and checks and lints `.src` files.

The Python command-line tools under `tools/` share one contract, which `tests/test_cli.py` checks: each runs from the repository's root; `-h` prints its usage and exits 0; a command line its parser refuses exits 2, with a `usage:` line and a `TOOL: error:` line; and a handled failure, including an operand refused after parsing, exits 1, naming each problem on a line that opens `TOOL: `.
