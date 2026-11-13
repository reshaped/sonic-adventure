# Ports

The tree builds two ports beside the matching reconstruction: the Dreamcast port (`make dc`) and the PC game (`make pc`). Both compile non-matching C for fixes, new layouts and features the original game doesn't have. `make` never compiles that code. This page covers those sources and how the Dreamcast port builds and links them with GCC 16.2. [ENGINE.md](ENGINE.md) covers the engine and PC game.

GCC rounds `0.1F` where SHC truncates it, and the disc stores pointers as numbers where a new link needs names. Much of the port's work is preserving what the shipped code computes.

[README.md](README.md#build) gives the commands and host requirements, [TOOLCHAIN.md](TOOLCHAIN.md) the matching build, and [CONTRIBUTING.md](CONTRIBUTING.md) how to make a change.

- **Non-matching code:** [Switches](#switches), [What it keeps](#what-it-keeps), [Where the code lives](#where-the-code-lives).
- **Compiling:** [Build model](#build-model), [Toolchain](#toolchain), [Flags and warnings](#flags-and-warnings), [SHC's C under GCC](#shcs-c-under-gcc), [Floats](#floats), [Inline assembly](#inline-assembly), [SH4ZAM and the runtime](#sh4zam-and-the-runtime).
- **Linking:** [Content](#content), [Imports and the census](#imports-and-the-census), [Coordinated link](#coordinated-link), [Memory contract and disc](#memory-contract-and-disc).

## Switches

Where the matching C won't do, a guard keeps the non-matching code beside it. Builds set two switches:

| Switch | Its side holds | Set by |
|---|---|---|
| `NONMATCHING` | GNU C for the ports, not compiled by SHC | `make dc`, the [native slice](ENGINE.md#native-game), and the host builds of `pace.c` and [`easy.h`](engine/dc/sa1/easy.h) in `make engine-test` |
| `DREAMCAST` | SH-4 instructions, inline assembly and extensions that need the console | SHC's builds and `make dc` |

The matching side is `NONMATCHING`'s `#else`, which SHC and clang-tidy compile: [`compile_flags.txt`](compile_flags.txt) sets neither switch. The native slice sets only `NONMATCHING`, so it takes the non-matching C without the SH-4 instructions. Those instructions sit under `#ifdef DREAMCAST`, with the shipped C or nothing on the other side, as in [`njCos.c`](sdk/ninja/njCos.c) and [`njtypes.h`](sdk/include/njtypes.h). The native slice takes the shipped side of the nine console-only guards in [`polygon.c`](src/collision/polygon.c), as [`marks.py`](engine/tools/marks.py)'s `CONSOLE` names them. It also takes the shipped side of Ninja's 2D polygons, 2D and 3D lines, and easy models, whose non-matching sides call the Dreamcast engine ([`native.mk`](engine/mk/native.mk)).

A guard uses `#ifdef`, with an optional `#else`, never `#ifndef`, `#if` or `#elif`. The non-matching side comes first, as in [`stdarg.h`](sdk/include/stdarg.h). Code that exists only in the matching build sits under an empty `#ifdef NONMATCHING` and its `#else`. A guard names the build's purpose, not its compiler: [`tools/guards.py`](tools/guards.py), run by `make lint`, rejects `MATCHING`, `PORT`, `GCC`, `__clang__`, other tests of either switch, and definitions or removals of the switches.

Keep a guard around the fewest whole declarations, statements or initializer rows, with no directive inside an expression. It may cover just an `if`, `while`, `for` or `else if` head, or a function's name, linkage and parameters where both sides share the body ([`boulder_roll`](modules/stg07/boulder.c), [`search_tex_list`](sdk/ninja/njTexList.c)). Where every C call needs another body, a guarded macro can rename the function at its declaration, as [`polygon_find_sides`](src/collision/polygon.h) does; the matching definition undoes the macro.

A tag or comment outside a guard describes the matching side ([Hacks and bugs](TAGS.md#hacks-and-bugs)). The non-matching side may preserve the result the tag describes ([`ball_slide_along_walls`](src/player/player.c)) or fix the defect ([`download_item_draw`](src/object/download.c)). A comment inside the guard says which, and refers to *this side*. clang-tidy reads the matching side, not `DREAMCAST`'s; format, tag and guard checks read every side. SHC extensions rejected by GCC need a `NONMATCHING` guard too ([`kmsurfac_workaround_for_sh_lnk.c`](loader/sdk/kmsurfac_workaround_for_sh_lnk.c)).

Storage the tables place without a source definition takes one under `#ifdef NONMATCHING`, as [`texture_buffer`](src/main.c) does. A definition must cover any bytes the code reads past it, with a comment explaining what the matching code writes there ([`tails_tail_face_normals`](src/player/tails.c)). Where the game reads several definitions as one object, the non-matching side declares one object instead, as the blink's color in [`piece.c`](src/object/piece.c) does. GCC need not keep separate definitions adjacent.

## What it keeps

- **The matching build.** `make` and `make check` read neither `dc.mk`, `config/gcc.mk`, `config/dreamcast.mk` nor `build/dc/`, and compile no `NONMATCHING` side.
- **The game's behavior.** A guard that replaces an invalid read says whether it repairs the defect or preserves its shipped result. Tails' flight probe preserves the result of the original reach test: no floor, even where the original test succeeds by reaching only a `WORLD_HUGE` object ([`polygon.c`](src/collision/polygon.c)).
- **SHC's order, where it matters.** The source spells SHC's grouping where C gives another floating-point result, the float pass handles fusion ([Floats](#floats)), and a guard preserves a call's argument order in `gacha_robot.c`. No check establishes every other evaluation order.

Even with every feature off, the non-matching code differs:

- Some guards repair shipped defects, as the sai's treads taking their mesh's UVs ([`sai.c`](src/enemy/sai.c)). Others preserve a shipped result by different means, as the flight probe does.
- SA1's model draws differ from `model.src` at 30 fps, as [`src/draw/model.c`](src/draw/model.c) and the Dreamcast [model walk](engine/dc/sa1/model.c) document. E-101 mkII's arm trails and blur copies use Ninja's forms through that walk, rather than the shipped `njDrawObject` and `njAction` paths ([`model.h`](src/draw/model.h)).
- `MOVIE.BIN`'s `BSG` and the bss past what `event_load` zeroes in 17 events start as zeros, rather than the RAM contents they replace.

The collision index, texture-search memo and streaming DMA run with the features off too. They preserve the shipped searches' answers and copy the same bytes ([`polygon.c`](src/collision/polygon.c), [`njTexList.c`](sdk/ninja/njTexList.c), [`mw_sdcs.c`](sdk/sofdec/mw_sdcs.c)).

## Where the code lives

Non-matching code lives behind guards in the game's sources, and in five complete files: [`pace.c`](src/system/pace.c), [`port.c`](src/system/port.c), [`present.c`](src/system/present.c), [`skip.c`](src/event/skip.c) and [`model.c`](src/draw/model.c). [`NONMATCHING_C`](config/compilers.mk) keeps them out of SHC's routes, and [`config/dreamcast.mk`](config/dreamcast.mk) links them into `1ST_READ.BIN`. The game reaches them and the Dreamcast engine through guarded hooks.

[`port.h`](src/system/port.h) declares the game's toggles, curtain, wide window, 2D pivots, frame end, split frame, model shading and skip. `ninja.h` and `frame.h` include it through guarded relative paths. It includes [`dreamcast.h`](engine/dc/dreamcast.h). [`chord.h`](src/system/chord.h) carries the chord, readout page, note and curtain count between `port.c` and the Dreamcast and PC overlays; only those three sources include it.

`present.c` owns the split frame, its pacing, records and replays. `pace.c` owns `frame_end` and asks the engine each frame which modes the game can use. `port.c` owns the toggles, chord, curtain count and `port_view_x`. [`wide.c`](engine/dc/wide.c) sets the Dreamcast picture's aspect. The link places `port.o` after the original code and data, ahead of the Dreamcast [`overlay.c`](engine/dc/overlay.c), so changes to the overlay don't move its code ([`kernel.txt`](config/1st_read/kernel.txt)).

SHC, `make progress` and the matching build don't compile the five files. `make review` sees their tags, clang-tidy sees their empty matching sides, and `make format` and lint's format, tag and guard checks see all of them. The import check and census read them as GCC does.

## Build model

`make dc` builds the tree's C and assembly with GCC 16.2 and GNU as, placing each object at its source's path under `build/dc/`. shckit translates the assembly GCC can't read. The [grafted donor](TOOLCHAIN.md#grafted-r10), whose four functions appear only in SHC's images, is left out. [`config/dreamcast.mk`](config/dreamcast.mk) replaces five `.src` files and fourteen sources' inline assembly, and adds seven sources. SH4ZAM's SH-4 backend becomes `build/dc/sh4zam/libsh4zam.a`. **Image**, **route**, **object** and **`gen/` range** have their [matching-build meanings](TOOLCHAIN.md#build-model).

The **float pass** gives GCC the floating constants, initializers and fusion that SHC's code produces. An image's **content** is the disc data of all its `gen/` ranges in one C file, with pointers into RAM expressed as names rather than fixed numbers.

From source to manifest:

1. GCC compiles the game's C after [`tools/floats.py`](tools/floats.py) and its modules run. Added and replacement C, and SH4ZAM's C, skip that pass. [`tools/inline.py`](tools/inline.py) separates inline assembly; [`tools/tags.py`](tools/tags.py) checks ABI contracts, [`floats.py`](tools/floats.py) checks assembly constants, and `build/shckit` translates the assembly for GNU as. GNU as builds SH4ZAM's assembly unchanged; `ar` archives its backend.
2. [`tools/content.py`](tools/content.py) writes `build/gen/IMAGE/content.c` from the image's imported data, using [`tools/stages.py`](tools/stages.py) to identify the images each stage reads. GCC compiles the content after the float pass. GNU ld links it at the disc's addresses and `cmp` checks its bytes.
3. [`tools/imports.py`](tools/imports.py) checks uses and providers of names taken from other images, and [`tools/census.py`](tools/census.py) checks the address classifications in `config/1st_read/census.txt` ([Imports and the census](#imports-and-the-census)).
4. The **coordinated link** resolves each image's imports against their providers: [`tools/spec.py`](tools/spec.py) writes a spec, [`tools/ld.py`](tools/ld.py) links, and [`tools/verify.py`](tools/verify.py) checks the result, including declarations through [`tools/decls.py`](tools/decls.py) and GCC's DWARF types through [`tools/dwarf.py`](tools/dwarf.py). They share [`tools/gcclink.py`](tools/gcclink.py) and [`tools/elf.py`](tools/elf.py).
5. [`tools/publish.py`](tools/publish.py) writes the images under `build/dc/`, checks their RAM use against the [memory contract](#memory-contract-and-disc) from [`tools/memory.py`](tools/memory.py), then writes `build/dc/manifest.txt` in [`tools/manifest.py`](tools/manifest.py)'s form. It records hashes of the toolchain programs, `build/prskit`, the spec, inputs, link-time objects and output files.
6. `make dc-disc` uses [`tools/disc.py`](tools/disc.py) to replace the disc's binaries according to the manifest, writing `build/dc/disc/` from the original `.gdi` and its tracks.

The link removes the old manifest and disc when it starts. If a dependency fails before that, either may remain from an earlier build; use `make dc`'s exit status, not their presence, to decide whether the build passed. `make accept` ties files to a commit whose gates passed; `sed -n 's/^sum *//p' build/accept.txt | sha256sum -c -` checks its recorded sums ([Checks](CONTRIBUTING.md#checks)).

## Toolchain

With the default `SH_ELF=tools/sh-elf`, make downloads binutils 2.47, GCC 16.2.0, GMP, MPFR and MPC, checks their hashes against [`tools/SHA256SUMS`](tools/SHA256SUMS), and builds a little-endian, single-precision SH-4 compiler and libgcc. Its receipt, `tools/sh-elf/request.txt`, records the inputs, hashes, host (`uname -sm`) and configure flags. A missing or mismatched receipt, including one from another host, stops the build until the toolchain is removed and rebuilt.

GCC is configured with `--enable-lto` and binutils with `--enable-plugins`. `make dc`'s C objects carry GCC's intermediate code, and the link compiles each image whole through the linker's plugin. The toolchain build fails, and installs nothing, unless it produced `lto1`, `lto-wrapper`, `collect2` and `liblto_plugin.so`.

Three more flags keep the host out of the result. Both configures use `--without-zstd`, so the toolchain doesn't link the host's zstd. Binutils uses `--enable-deterministic-archives`, giving each `libgcc.a` member zero dates, owners and groups. GCC uses `--with-debug-prefix-map`, writing `.` in place of the checkout path in libgcc's debug information. The receipt records `CURDIR` rather than an absolute checkout path. On the same host, separate checkouts can produce identical toolchains and receipts.

A toolchain supplied through `SH_ELF` must have `sh-elf-gcc`, `sh-elf-as`, `sh-elf-ld`, `sh-elf-ar`, one each of `cc1`, `lto1`, `lto-wrapper`, `collect2` and `liblto_plugin.so`, `libgcc.a` and GCC's headers, plus `sh-elf-objcopy` and `sh-elf-objdump` for `make test`. Make doesn't check its version; only GCC 16.2.0 is tested, and the float pass depends on its behavior.

## Flags and warnings

[`config/gcc.mk`](config/gcc.mk) defines `GCCFLAGS` for every route, with overrides beside the objects that need them. Most C builds at `-Os`; `GCC_HOT` names profiled hot code built at `-O2`. The model walk uses `-O3`, but disables unswitching and path splitting, whose copies could push its kernels past the instruction cache.

GCC reads only `-optimize=0` from SHC's flags. The float pass also reads `-fpu=single`, `-double=float` and `-aggressive=2`. A change to any of those in [`config/compilers.mk`](config/compilers.mk), including removing `-fpu=single`, must name the affected GCC object. Every GCC or GNU as warning fails the compile. GCC ignores `#pragma inline` and `#pragma section`; [`test_make_dc.py`](tests/test_make_dc.py) checks that no other unknown pragma reaches the game's GCC sources.

## SHC's C under GCC

Where a clearer spelling produces the same SHC bytes, use it on the matching side. Otherwise a guard keeps the non-matching C apart ([Switches](#switches)). Code only the Dreamcast port needs belongs behind a guard, in a header reached through one, or under `engine/dc/`; [`ninja_zam.h`](sdk/include/ninja_zam.h) is such a header.

GCC reads the port's C as GNU C17, not SHC's C89. The sources [`config/dreamcast.mk`](config/dreamcast.mk) adds or substitutes, from the game or engine, may use GNU C17; [`sa1/sprite.c`](engine/dc/sa1/sprite.c) declares variables in `for` heads. They take `GCCFLAGS`, not SHC flags or the float pass. GCC rounds their constants normally and, with `-ffp-contract=off`, doesn't fuse products into sums. Their includes search `sdk/include`, `src` and the root; individual objects add `engine/include`, `engine/sa1` or `engine/dc` where their headers need it ([`config/gcc.mk`](config/gcc.mk)).

GCC can drop a static that nothing in C names. A static kept only by a memory contract or table needs `__attribute__((used))` under a guard, as an event's head does in [`head.h`](events/shared/head.h).

## Floats

SHC truncates decimal `float` constants toward zero; GCC rounds them to nearest. For `0.1F`, SHC uses `0x3dcccccc` and GCC `0x3dcccccd`. The sources keep `0.1F`; [`tools/floats.py`](tools/floats.py) replaces the constant in GCC's preprocessed `.i` with `0x1.999998p-4F`, the hexadecimal spelling of SHC's value. It uses each source's release and flags, and changes the tree's lines, not SH4ZAM's headers. Its docstring and those of its modules give the rules and refusals.

Static initializers need the same treatment. For `e102_actions`, SHC folds `1.0F / 6.0F` to `0x3e2aaaaa`, while GCC gives `0x3e2aaaab`. The pass writes SHC's value. Runtime arithmetic still uses the console FPU, which rounds toward zero. `-frounding-math` prevents GCC from folding an operation whose result depends on rounding; because GCC calls the flag experimental, [`test_floats.py`](tests/test_floats.py) tests its behavior under GCC 16.2.0.

SHC groups a run of `*`, or of `+` and `-`, folds its constants together and applies them last. Where C's grouping gives another result, the source spells SHC's order, as in [`race_rival_init`](modules/al_race/race_rival.c) and [`path_curve_sample`](src/path/curve.c). The float pass cannot distinguish every run that differs from one that doesn't; no other check covers new runs.

Under `-aggressive=2`, the pass refuses division of a nonconstant by a constant whose reciprocal a normal float cannot represent exactly.

The pass models R26 and R32 fusion: a product fuses into a sum only where the run adds it, with the rule in [`tools/float_fuse.py`](tools/float_fuse.py). Some of SHC's `FMAC`s differ from that rule; R10 fuses none. GCC compiles R26 and R32 objects with `-ffp-contract=on`, the other C with `off`. For R26 and R32, the pass prevents fusions SHC doesn't make and rewrites additions SHC does fuse but GCC wouldn't, as in `board_fixed_view`'s target calculation. Its rewrites change neither the lvalue nor the number of reads and writes.

`floats.py --assembly` rejects a `.FDATA.S` constant at or near a rounding tie, where asmsh and GNU as can differ.

## Inline assembly

[`tools/inline.py`](tools/inline.py) splits a C source that contains inline assembly. It blanks the assembly in the C, writes the bodies and `#pragma asm` blocks separately, and includes shckit's GNU-as translation through file-scope `asm` ([translate.md](https://github.com/reshaped/shckit/blob/e0b8412e337103b59ea2f0d58dd7be52699222b9/docs/translate.md)). Assembly hidden from the non-matching side stays out of the translated text.

GCC doesn't know an `@asm abi` contract or what an inline body takes from its host. `tools/tags.py --abi` therefore refuses such functions and bodies that may be unsafe to call, unless their objects' `CONTRACTS` in [`config/gcc.mk`](config/gcc.mk) explain how GCC meets the contract or that GCC never calls them. Passing isn't proof of call safety. Each `.src` file also goes through `tags.py --abi` and `floats.py --assembly` before shckit translates it for GNU as.

## SH4ZAM and the runtime

Make fetches the pinned [SH4ZAM](https://github.com/gyrovorbis/sh4zam) headers, sources and license listed in `tools/SHA256SUMS`, checking each file and changing none of them. Its C builds under `GCC_SH4ZAM_FLAGS`, without the game's layout rules, `GCC_AS_SHC` or the float pass. [`ninja_zam.h`](sdk/include/ninja_zam.h) supplies Ninja's matrix-stack, transform, vector and trigonometry helpers; [`engine/dc/ninja/`](engine/dc/ninja) supplies the other matrix and vector routines. GCC's `memcpy` and `memset`, and the Dreamcast drawing code, call SH4ZAM.

Where a public helper could fold a call of constants in C, it calls SH4ZAM's SH-4 backend directly, so the instruction runs with the actual arguments. SH4ZAM's single dot products use C multiply-adds and don't reproduce `FIPR`'s bits; the helpers provide their own assembly `FIPR`. [`test_zam.py`](tests/test_zam.py) checks a set of entries for the console's results or byte moves. It cannot show that a caller uses those entries exactly as the original did: `njMultiMatrix`, for example, uses `FTRV` where the shipped code uses sixteen `FIPR`s ([`njMultiMatrix.c`](engine/dc/ninja/njMultiMatrix.c)).

GCC's libgcc returns `x` for integer `x / 0` and `x % 0`, where SHC's runtime returns zero. A division or remainder whose divisor may be zero needs a guard that preserves zero, as [`egg_walker_blast_create`](modules/b_egm2/blast.c) does, or repairs the error. No check catches a new such division. `NJS_MATRIX` needs 8-byte alignment for paired `FMOV`s on a console. `-fpack-struct=4` preserves SHC's member layout, with `pack(8)` for `NJS_VIEW` and `NJS_LIGHT` ([`njtypes.h`](sdk/include/njtypes.h)).

## Content

`make dc` doesn't compile the original `gen/` ranges, whose C names pointers by address. Instead, [`tools/content.py`](tools/content.py) writes each image's content from `config/<image>/data.txt` ([Generated ranges](TOOLCHAIN.md#generated-ranges)). A word becomes a pointer only with evidence: a type or structure in its row, or a `vouched` declaration. An untyped RAM word outside a text fails. A pointer into RAM becomes a symbol, a copy of text a source declares, or an unresolved name that fails the link. Other pointers, including null, stay numbers. Content for `1ST_READ.BIN` can name another image only through a `path_lists` key that identifies the images a stage loads.

Each definition compiles in its own section. GNU ld links the content at the disc's addresses and checks that its bytes still match the disc; the link resolves each pointer. Where the game reads several rows as one array, `data.txt` groups them, as the missile's seven meshsets in [`missile.c`](modules/stg04/missile.c) do. An array declared by a source must likewise remain whole if content reads it that way, as the signboard's eight materials in [`object_textures.c`](modules/minicart/object_textures.c) do. The final link keeps the content definitions in image order, so a read past one meets the same bytes as in the original.

## Imports and the census

An **import** is a name one image takes from another. [`tools/imports.py`](tools/imports.py) checks its uses and definitions separately:

- `make dc` and `make dc-check` run `--uses`, without the disc or SHC. Each use in `1ST_READ.BIN`'s C, on either side of a guard, must be covered by the stages whose loader reads the provider's image. Only a table gives a use its stages, never a test of the current stage. Other uses need rows in [`config/1st_read/uses.txt`](config/1st_read/uses.txt); missing and unused rows fail. An import whose address no `splits.txt` row of its image holds fails too.
- `make check` runs `--definitions`. The source object that SHC or asmsh builds, or its stub object for disc-derived initializers, must define the imported name at its recorded address in the provider image. This doesn't inspect definitions in generated ranges or an address no source row holds.

The **census**, [`tools/census.py`](tools/census.py), assigns each address-looking number a disposition: `anchor` stays fixed, `placed` moves with the link, and `data` isn't an address. A number without a classification fails: in `1ST_READ.BIN`'s sources, included files, tables and generated data for RAM past its image, or in another image's non-matching code at an address its tables name. With `--disc`, the census checks motion files as well; an address into `1ST_READ.BIN` that a file's content doesn't relocate fails. The host C compiler records the includes in `build/reads/`, without the disc or SHC. `make dc-check` checks the main executable's sources and tables; `make dc` additionally checks generated data, motion files and every image's non-matching code. [Checks](CONTRIBUTING.md#checks) lists the limits of each.

## Coordinated link

Each image links at the base in its tables, in relink's object order. Records loaded at fixed offsets, such as an event's head, open their images; `ADVERTISE`'s `DEND` and `STG08`'s fish models close theirs.

`1ST_READ.BIN` starts with the draw kernels named in [`config/1st_read/kernel.txt`](config/1st_read/kernel.txt). Five kernels occupy different sets of the SH-4's 8 KB instruction cache. Other kernels hold the object walks, collision sweep, land cull, near clip and sprites, with less frequently executed code between them. Each kernel owns a window of whole cache lines, including room for growth without moving later code. The link refuses a kernel that outgrows its window or has two of its own lines in one set. Kernels can share sets only where an `over` row permits it.

The table's `warm` rows put frequently executed functions after the kernel windows, before other code, each at a recorded place. [`tools/order.py`](tools/order.py) chooses the places to reduce collisions between hot instruction-cache lines, including the kernels' lines. [`heat.txt`](config/1st_read/heat.txt) records how often the profiler entered each stretch at Emerald Coast's start, Windy Valley's four rings and Sky Deck's third act. [`order.txt`](config/1st_read/order.txt) records the heat hash, where `.text` starts, each kernel row's place and size, and each warm row's place. The link checks that record, not the warm rows' cache sets, and fails when a warm row outgrows the space before the next row's place. `make dc-order` then writes the record again, moving only a row that no longer fits or a new one. `make dc-order SEARCH=1` searches every row's place again and `make dc-order PACK=1` lays the rows end to end; both move code, so measure what they write. A `window warm` row in the table reserves room after the warm rows, so a row that grows or moves there doesn't move the code that follows. Code linked after the warm rows may move without moving them.

`.rodata` and `.bss` start on the operand cache's 16 KB boundary. The `data` rows put Ninja globals used by drawing first in `.bss`. For each kernel literal that points into one of those data sections, or the last data, the link considers the lines near its target: 64 bytes from the address within its object, or only that address's line when the object exceeds 256 bytes. It shifts those sections by the fewest whole lines that keep the target's lines off the operand-cache sets used by the literal pool, except where an `over` row allows overlap. It calculates the shift from the unshifted link and checks the result, since alignment can swallow a move. This protects the named direct accesses, not data reached through further pointers or callees.

The kernel windows and warm code leave every other section's relative order intact, though code after them moves when they grow. A function can take [`port.h`](src/system/port.h)'s `LINKED_LAST` to sit after an image's other code, or a datum `LINKED_LAST_DATA` to sit after its data. The `last` rows of the kernel table put the last code of named sources in order, so additions after them don't move it. A `cold` row moves a source's code, data, strings and bss past those last placements. The readout's [`overlay.c`](engine/dc/overlay.c) is cold, followed by [`owner.c`](engine/dc/owner.c): changing a readout page moves no code or data outside the cold sources.

`STG01` has its own [`kernel.txt`](config/stg01/kernel.txt), putting the broken pier's pieces and the rock the orca topples on instruction-cache sets chosen alongside the main executable's draw code. Kernels of two live images may share a set only where either table permits it. The model walk places projected points away from the collision sweep's instruction-cache set, and [`model.c`](src/draw/model.c) aligns `draw_colors` on the operand cache's 16 KB boundary so palettes 0 to 3 occupy its first 256 sets.

A name an image refers to but doesn't define binds to the first applicable provider: an imports-table row, an anchor in the memory contract, or a live image whose span holds the address and whose object defines the name. [`config/imports.txt`](config/imports.txt) records providers the build cannot derive, such as `skip_owed` in [`skip.c`](src/event/skip.c). A provider must be live alongside the consumer, except during a [hand-off](TOOLCHAIN.md#image-tables). Unbound names fail and appear in `build/dc/link/unbound.txt`.

[`tools/ld.py`](tools/ld.py) links every image twice. The first link determines its new layout and the addresses imported from providers; the final link must reproduce that layout, including `STARTOF` and `SIZEOF` section bounds. [`tools/verify.py`](tools/verify.py) checks bindings, anchors and archive members, and compares declarations with definitions using GCC's DWARF types and required alignment. [`config/declarations.txt`](config/declarations.txt) admits exceptions. A guard repairs calls whose original declarations don't agree with the definition reached by address, as `ground_below_int`'s does.

## Memory contract and disc

[`tools/memory.py`](tools/memory.py) derives RAM owners from the sources that fix their areas, such as the work area in `loader.h`, and states of what can be live together from the loader's reads. Owners include work beyond an image: `ADVERTISE`'s backup and Zero's and E-101 mkII's banks. The contract rejects an image outside its RAM, owners that overlap while live together, PRS files too big for their work area, and event bss beyond both its file and what `event_load` zeros. [`tools/publish.py`](tools/publish.py) requires `_INITSCT` to zero all bss and no other bytes in `1ST_READ.BIN` and `LOADER.BIN`, and the `PSGSFD` sections of `MOVIE.BIN` to meet the layout `MPV_Init` expects.

The collision index borrows up to 32 KB of `syMalloc`'s largest free block after a land loads, and releases it before the next land read. That heap otherwise holds Kamui's vertex buffer and the land file. The index ordinarily leaves the file's allocation unchanged, unless other code frees or takes memory meanwhile, as the movie player does. The borrowed block also makes the index builder Kamui's wait callback, which only the movie player otherwise replaces between stages ([`polygon.c`](src/collision/polygon.c)).

[`tools/disc.py`](tools/disc.py) rejects a manifest missing an image from `config/`, or a file whose hash differs from the manifest. `1ST_READ.BIN` keeps the retail boot file's last sector and cannot exceed its original sector count; a smaller file begins later. Other files keep their first sector unless they need more room, then move in path order to the first free space that fits. The disc builder enforces the memory contract and original disc map, reads back every replaced file, and publishes `build/dc/disc/` only after its checks pass.
