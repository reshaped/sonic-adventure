# GCC

The GCC build compiles the game's sources with stock GCC 16.2, links every image anew, and adds 16:9, 60 fps, a frame counter and skips for movies and cutscenes. Much of the complexity comes from keeping what SHC's code computes: GCC rounds `0.1F` where SHC truncates it, and the disc's data holds addresses as numbers where a new layout needs names.

[README.md](README.md#build) gives the commands and what the build needs, [TOOLCHAIN.md](TOOLCHAIN.md) the matching build it starts from, and [CONTRIBUTING.md](CONTRIBUTING.md) how to make a change.

- **How the port is built:** [Build model](#build-model), [What it keeps](#what-it-keeps).
- **What it adds:** [Features](#features).
- **Compiling:** [Toolchain](#toolchain), [Flags and warnings](#flags-and-warnings), [Guards](#guards), [Sources in `gcc/`](#sources-in-gcc), [Floats](#floats), [Inline assembly](#inline-assembly), [SH4ZAM and the runtime](#sh4zam-and-the-runtime).
- **Linking:** [Content](#content), [Imports and the census](#imports-and-the-census), [Coordinated link](#coordinated-link), [Memory contract and disc](#memory-contract-and-disc).

## Build model

`make gcc` builds each of the tree's sources to one object under `build/gcc/`, at the source's path: GCC 16.2 compiles the C, and GNU as assembles each `.src` file and inline body as shckit translates it. It leaves out the [grafted source's donor](TOOLCHAIN.md#grafted-r10), whose four functions only SHC's images hold. `gcc/` replaces three `.src` files and adds five sources ([Sources in `gcc/`](#sources-in-gcc)), and SH4ZAM's SH-4 back end goes to `build/gcc/sh4zam/libsh4zam.a`. Image, route, object and `gen/` range mean what TOOLCHAIN.md's [Build model](TOOLCHAIN.md#build-model) says.

A **guard** holds GCC's side of some C beside the shipped side, which `make` still matches ([Guards](#guards)). Before GCC compiles one of the tree's C sources outside `gcc/`, or an image's content, the **float pass** gives the floating constants and static initializers on the tree's lines SHC's values, and models SHC's fusing. An image's **content** is the data of all its `gen/` ranges as one C file whose pointers into RAM are names, so the link can move what they point at.

From source to manifest:

1. GCC compiles the tree's C sources outside `gcc/` after [`tools/floats.py`](tools/floats.py) and its `float_*.py` modules run. Sources in `gcc/` and SH4ZAM's C compile without the pass. GNU as assembles the back end's assembly as it is, and `ar` archives the back end. [`tools/inline.py`](tools/inline.py) splits a source that holds inline assembly. `tools/tags.py --abi` and `floats.py --assembly` check each `.src` file and each source's inline assembly, which `build/shckit` translates for GNU as.
2. [`tools/content.py`](tools/content.py) writes each image's content, `build/gen/IMAGE/content.c`; [`tools/stages.py`](tools/stages.py) gives it the images each of `path_lists`' stages reads. GCC compiles it after the float pass, GNU ld links it at the disc's addresses, and `cmp` holds the link to the disc's bytes.
3. An **import** is a name one image takes from another. The **import check**, [`tools/imports.py`](tools/imports.py), holds `1ST_READ.BIN`'s uses of imports to the images their stages read, as `stages.py` gives them; [`tools/deps.py`](tools/deps.py) with `--gcc` makes the import check depend on the objects it examines. [`tools/census.py`](tools/census.py) checks the **census**, `config/1st_read/census.txt`, which explains each number `1ST_READ.BIN` holds for RAM past its image, and each number any image's GCC side holds at an address a table names.
4. The **coordinated link** lays out every image anew and binds each import to its provider's definition in three stages: [`tools/spec.py`](tools/spec.py) writes the **spec**; [`tools/ld.py`](tools/ld.py) links each image by it; and [`tools/verify.py`](tools/verify.py) checks the final links. `verify.py` also checks declarations through [`tools/decls.py`](tools/decls.py), which reads GCC's DWARF types through [`tools/dwarf.py`](tools/dwarf.py). The stages share [`tools/gcclink.py`](tools/gcclink.py), which reads the spec, and [`tools/elf.py`](tools/elf.py), which reads objects and links.
5. [`tools/publish.py`](tools/publish.py) writes each file under `build/gcc/` at its path on the disc, held to the **memory contract**, [`tools/memory.py`](tools/memory.py): what RAM each owner holds, and when. Last it writes the **manifest**, `build/gcc/manifest.txt`, in [`tools/manifest.py`](tools/manifest.py)'s form, with the hashes of four toolchain programs, `build/prskit`, the spec, each input and each file.
6. `make gcc-disc`: [`tools/disc.py`](tools/disc.py) builds a disc from the original `.gdi` and its tracks, replacing the original binaries with the manifest's files in `build/gcc/disc/`.

The link removes the old manifest and disc when it starts. If a dependency fails before then, both may remain from the last build; only `make gcc`'s exit status tells whether this run passed.

## What it keeps

- **The matching build.** `make` and `make check` read neither `gcc.mk`, `config/gcc.mk` nor anything under `build/gcc/`.
- **The game's behavior.** Where a guard replaces a read past an object or of an unset register, its comment says whether GCC repairs the read or keeps a shipped result that depends on it. GCC's side of Tails' flight probe keeps the image's result, no floor ([`polygon.c`](src/collision/polygon.c)). It finds none near the world's origin either, where the image's can find one over a `WORLD_HUGE` object.
- **SHC's order, where something spells it.** The source spells SHC's grouping where C's gives a floating run another value, the float pass models SHC's fusing, and a guard keeps a call's argument order, as `gacha_robot.c`'s does. Nothing checks any other order.

With every switch off, the port still differs, beyond its skips:

- each guard that repairs, by its comment, as the sai's treads take their mesh's uvs ([`sai.c`](src/enemy/sai.c));
- each guard that keeps the play, where its comment says it parts from the image, as the flight probe near the world's origin;
- each way the model renderer in `gcc/` departs from `model.src` at 30 fps, as [`model.c`](gcc/src/draw/model.c)'s head lists;
- `MOVIE.BIN`'s `BSG`, and in 17 events the bss past what `event_load` zeroes, start as zeros, not as RAM held them.

## Features

| Press, on the first pad | Does |
|---|---|
| Hold L and R, then up | Steps the readout |
| Hold L and R, then right | Turns 16:9 on or off |
| Hold L and R, then down | Turns 60 fps on or off |
| Start, in a movie, or outside a demo in an event that holds control | Skips it |

After the d-pad's press, the game sees none of L, R and the d-pad until all are up, so L and R alone still reach it. Each press of the chord names what it set near the top of the screen for two seconds. The switches start off and none is saved; off, a switch changes nothing. In an attract demo or recording 16:9 pillarboxes, holding the window at 4:3, so replays stay in step. 60 fps splits their frames as it does play's.

### The readout

Up steps through off, the counter, and the counter over SEGA's debug pages. `30 FPS 14.4 GPU  7.5` gives pictures presented per second, then two half-second averages in milliseconds: the time outside `frame_wait` and a split frame's middle wait, and the last completed render's time. The readout turns amber when the busiest frame exceeds 90 percent of its budget, and red when it exceeds the budget. Below, `16:9 ON  60 FPS HELD` gives each mode, HELD where play holds back what its switch asks. A held 60 fps can add why: `R` where the PVR couldn't take the first picture in time, `T` where step 0, or step 1 and the pass, didn't fit their half of the frame, or the frame ran late ([`overlay.c`](gcc/src/system/overlay.c)).

### Skips

Start skips any movie; the shipped game skips only the opening. In an event, a black curtain falls while the scene plays on. Behind it nothing renders or sounds, and no frame waits for a vsync. The rest of the script runs with each wait one step long, so each flag and placement still happens: a runner stands at its target, and a mover takes the steps its wait stood for. The event ends through SEGA's own ending, and the curtain lifts as control returns ([`skip.c`](gcc/src/event/skip.c)).

### 16:9

16:9 sets the picture's aspect to 3/4: the screen's 640 pixels hold a view a third wider, which a 16:9 screen stretches back to its shape. Ninja's aspect, which the game projects by, stays 1.

2D layers squeeze toward a pivot, the middle or an edge, and one that covers the screen keeps its width: an untextured polygon with corners on both edges by rule, and any other by a flag. A movie, or an event's still that covers the scene, shows between bands, and an event's backdrop, or a still whose color adds to the scene's, fills the screen, cropped at top and bottom.

The game's logic reads 16:9 only through the window, in its four tests of what is on screen and in Sky Chase's reticle. In play the window is the picture, so objects in the wide sides act as in view and `rand()` parts from a 4:3 game's. Under pillarbox each 2D layer squeezes about the middle, and black bands cover the wide sides. [`port.h`](gcc/src/system/port.h) says who may read each aspect and how a 2D draw sets its pivot, and [`test_wide.py`](tests/test_wide.py) holds the code to both.

### 60 fps

60 fps adds pictures, never steps. Play runs the steps it runs at 30, under the same `frame_step` and step count, and each picture is to show what a picture at 30 shows at its step. No picture is to change what a step reads.

A split frame shows two pictures. The first holds what step 0 drew, at the frame's middle vsync. The second, after the last step, holds what step 1 drew, and draws each other task as SEGA's pause does: by its callback, or where it has none or its callback is its step, by what its step drew on step 0, moved to the second picture's view. That pass keeps `rand()`'s state, and puts back each task's work after its callback, and a player's motion, record and shapes. Six of SEGA's entries refuse it, and some draws work from a copy, as [`enemy_spring_scale`](src/enemy/enemy.c) does. A draw that writes play's state anywhere else still breaks the rule, and no test checks it ([`present.c`](gcc/src/system/present.c)).

Frames split in play and in the fade from black that opens it: not in Twinkle Circuit, Sky Chase, the Chao gardens or the race, nor in pause, the other fades or game over. Fifteen frames in a row that fit engage the split, and three that don't end it. A frame the pacing sees won't fit, or one with a bitmap waiting to upload, shows one picture. An event runs one step a frame, as SEGA's code does, so its second picture has no step to show, and an event at 60 fps looks as it does at 30.

At 30, a sprite a task draws on every step, as a rain splash in Chaos 0, shows once for each step in the one picture. Each picture replays 2D draws and `njDrawSprite3D` sprites from steps it doesn't show. The first replays the previous frame's later steps; the second replays step 0, except where the task's callback draws them in the pass. A replayed sprite moves to the picture's view, and a 2D draw keeps its place on the screen. No picture replays a draw of a task killed or deleted with a destructor, which may release what the draw takes. What SEGA moves once a frame moves at 30 against a camera at 60.

A path camera poses itself once a frame, so from a move's second frame on, the second picture's camera stands halfway between the first picture's pose and the one the later steps take, but not across a cut. In such a picture the pass draws every task, in place of step 1's models, and what step 1 draws past them, Chaos 0's rain splashes among it, keeps step 1's camera. While Chaos 4's camera follows the player, it moves on step 0 alone, so the first picture holds, at 30.

Five things still part from 30:

- The rain's drops, which are lines, show once in the first picture, and twice at one place in the second.
- Chaos 6's cloud sheet and the ripples on water, which each step adds over what lies behind, show once in each picture, where a picture at 30 holds two.
- Under a path camera's halfway pose, what a step draws on step 0 alone, and the pass neither replays nor draws by a callback, shows in the first picture alone. Invincibility's ball, Knuckles' homing trail and the Master Emerald's pieces are such draws.
- An opaque model appears twice in the second picture when its task draws it through Ninja on every step, draws nothing through the port's renderer, and its callback draws it too, as Speed Highway's barriers do. A picture at 30 holds two as well. The copies part only under the halfway pose, where step 1's keeps step 1's camera.
- Step 0's sounds start a vsync later than at 30.

On a console, standing at Windy Valley's start with 16:9 on, step 0 took 17.0 ms, more than its half of the frame, so 60 fps held at 30 there and split only in lighter views.

## Toolchain

Where `SH_ELF` names `tools/sh-elf` and it's missing, make fetches binutils 2.47, GCC 16.2.0, GMP, MPFR and MPC, checks each against [`tools/SHA256SUMS`](tools/SHA256SUMS), and builds a C compiler and libgcc for little-endian SH-4, single-only, writing last a receipt of what it built from. make refuses a `tools/sh-elf` whose receipt is missing or another's until you remove it. A directory you name must hold `sh-elf-gcc`, `sh-elf-as`, `sh-elf-ld`, `sh-elf-ar`, one `cc1` and one `libgcc.a`. make checks no version there, and only GCC 16.2.0 is tested: the float pass and its tests hold to it.

## Flags and warnings

`GCCFLAGS`, in [`config/gcc.mk`](config/gcc.mk), is one set for every route, each flag beside its reason; an object that needs more takes a line of its own. A source compiles at `-Os`, which buys the images their room, or at `-O2` where `GCC_HOT` names it for its share of a profiled busy frame. Of SHC's flags GCC reads only `-optimize=0`, and the float pass `-fpu=single`, `-double=float` and `-aggressive=2`, so a `config/compilers.mk` line that gives an object one names its GCC object too. A compile fails on any GNU as or GCC warning. GCC ignores SHC's `#pragma inline` and `#pragma section`, and [`test_make_gcc.py`](tests/test_make_gcc.py) holds that no other pragma GCC doesn't know reaches it.

## Guards

Where GCC needs other C, a spelling SHC compiles to the same bytes replaces the shipped one, as `make` shows. Otherwise a guard holds GCC's side. A name or behavior only the port has lives in a guard, in `gcc/`, or in a header reached only through a guard, as [`ninja_zam.h`](sdk/include/ninja_zam.h) is. make gcc defines the macro `GCC`, and a guard tests it with `#ifdef` or `#ifndef`, never `#if`; nothing tests `__GNUC__`, which Clang defines too. A guard spans the fewest whole declarations, statements or initializer rows, and no directive sits inside an expression. Where GCC's side replaces shipped C, it comes first, and the shipped side follows under `#else`, as in [`sdk/include/stdarg.h`](sdk/include/stdarg.h). Where GCC changes only the head of an `if`, `while`, `for` or `} else if`, the guard may hold the head alone, as in [`boulder_roll`](modules/stg07/boulder.c).

A tag, like any comment outside a guard, describes the shipped side ([Hacks and bugs](TAGS.md#hacks-and-bugs)). Under a tag, GCC's side keeps the play the tag describes, as [`ball_slide_along_walls`](src/player/player.c) does, or repairs the defect, as [`download_item_draw`](src/object/download.c)'s default case does; a comment inside the guard says which.

clang-tidy reads only the shipped side; lint's format and tag checks read both. A source that hides an SHC extension in C from Clang under `#ifndef __clang__` takes a GCC guard too.

Storage the tables place where no source holds it takes a definition under `#ifdef GCC`, as [`texture_buffer`](src/main.c) does. Where code reaches past it, the definition holds what it reaches, and its guard's comment says what the shipped code writes there, as [`tails_tail_face_normals`](src/player/tails.c)'s does.

GCC orders an object's definitions as it will, and drops a static that nothing in C names. So where the shipped code reads a run of definitions as one, through the first's address, GCC's side defines the run as one object, as the blink's color in [`piece.c`](src/object/piece.c) is, and a static that only the memory contract or a table names takes `__attribute__((used))` under a guard, as each event's head does in [`head.h`](events/shared/head.h).

## Sources in `gcc/`

`gcc/` holds C that only GCC compiles. Three of its sources replace a `.src` file, `gcc/P.c` compiling to the object `P.src` would: [`model.c`](gcc/src/draw/model.c) and [`sprite.c`](gcc/src/draw/sprite.c) under `gcc/src/draw/`, and [`njSprite.c`](gcc/sdk/ninja/njSprite.c) under `gcc/sdk/ninja/`. Five more are `config/gcc.mk`'s `GCC_ADDED`, which `1ST_READ.BIN` links after its own sources: `overlay.c`, `wide.c`, `pace.c` and `present.c` under [`gcc/src/system/`](gcc/src/system), and [`skip.c`](gcc/src/event/skip.c). The game reaches them through hooks under guards. make stops on any other source in `gcc/`.

Its C is GNU C17, not SHC's C89, as `njSprite.c`'s declarations in a `for` show. It takes `GCCFLAGS` whole, since it reads and writes the game's structures in their layout, and no SHC flag or float pass: GCC rounds its constants to the nearest, and under `-ffp-contract=off` fuses no product into a sum. GCC searches `sdk/include`, `src` and the root, so a source names a header of `gcc/` by its path from the root. A game or SDK header reaches `gcc/` by a path from its own directory, as [`frame.h`](src/system/frame.h) does. No SHC route, clang-tidy, `make progress` or `make review` reads `gcc/`; lint's format and tag checks and `make format` do. The census and the import check read its C as GCC's side.

## Floats

SHC truncates a decimal `float` constant toward zero, and GCC rounds it to the nearest: `0.1F` is `0x3dcccccc` under SHC and `0x3dcccccd` under GCC. The sources keep their constants, so the pass respells each decimal one in a source's preprocessed `.i` as the hexadecimal one that names SHC's value under the source's release and flags, on the tree's lines and not those of SH4ZAM's headers, and GCC compiles the `.i`: `0.1F` becomes `0x1.999998p-4F`. [`tools/floats.py`](tools/floats.py)'s docstring, and those of its modules, give each rule and refusal.

GCC folds a static initializer to the nearest, where SHC folds it as it reads a constant, so the pass writes SHC's value in its place: `e102_actions`' `1.0F / 6.0F` is `0x3e2aaaaa` on the disc, where GCC's is `0x3e2aaaab`.

Code keeps its arithmetic for the console's FPU, which rounds toward zero, since `-frounding-math` stops GCC folding an operation whose value rounds. GCC calls the option experimental, so [`test_floats.py`](tests/test_floats.py)'s `Gcc` tests hold GCC 16.2 to it, for each operation of constants the tree's code holds.

SHC reads a run of `*`, or of `+` and `-`, as one: it folds the run's constants into one, applied last, and orders the other operands its own way. Where C's order gives another value, the source spells SHC's, as [`race_rival_init`](modules/al_race/race_rival.c)'s random skills and [`path_curve_sample`](src/path/curve.c)'s weights do. The tree's other runs were checked once to give SHC's value; nothing checks a new one.

Under `-aggressive=2`, the pass rejects division of a nonconstant value by a constant whose reciprocal no normal float represents exactly.

The pass models R26's and R32's fusion by a rule, which probes under R32 bear out: a product fuses into a sum only where the run adds it. Some of SHC's FMACs part from the rule. R10 fuses none. So an object R26 or R32 builds compiles with `-ffp-contract=on`, and the tree's other C with `off`. On R26's and R32's objects the pass wraps each product the rule leaves unfused, as where `board_fixed_view`'s target subtracts the `dir.x * distance` its position adds by an `FMAC`, and writes as an addition a `+=` SHC fuses and GCC wouldn't.

No form the pass writes changes what an lvalue designates or how often an object is read or written. `floats.py --assembly` fails on a `.FDATA.S` constant on or just past a tie, where asmsh and GNU as can part.

## Inline assembly

A source with inline assembly compiles in two parts. [`tools/inline.py`](tools/inline.py) blanks the assembly in the C and writes its inline bodies and `#pragma asm` blocks to a separate file. After shckit translates that file, a file-scope `asm` in the C includes the translation ([translate.md](https://github.com/reshaped/shckit/blob/e0b8412e337103b59ea2f0d58dd7be52699222b9/docs/translate.md)). A piece a guard keeps from GCC stays out of the text.

GCC sees no `@asm abi` contract, nor what a body takes from its host, so `tools/tags.py --abi` fails on each such function, and on each body that may not be call-safe, unless its object's `CONTRACTS` in [`config/gcc.mk`](config/gcc.mk) names it, with how GCC's callers meet its contract, or that no GCC code calls it. Passing doesn't make a body call-safe. Each `.src` file passes `tags.py --abi` and `floats.py --assembly` ([Floats](#floats)) too, before shckit translates it for GNU as.

## SH4ZAM and the runtime

make fetches [SH4ZAM](https://github.com/gyrovorbis/sh4zam), an SH-4 math library, whole at a pinned commit, and checks each file against `tools/SHA256SUMS`. Its C compiles under `GCC_SH4ZAM_FLAGS`, without the tree's layout, `GCC_AS_SHC` or the float pass. [`sdk/include/ninja_zam.h`](sdk/include/ninja_zam.h) gives helpers for Ninja's matrix stack, transforms, vector math and trig that run SH4ZAM's instructions where C has none. GCC's memcpy and memset call SH4ZAM too, and the renderer in `gcc/` calls it directly.

GCC's integer division through libgcc answers `x / 0` and `x % 0` with `x`, where SHC's runtime answers 0. So each integer division or remainder whose divisor can be 0 takes a guard that keeps SHC's 0, as [`egg_walker_blast_create`](modules/b_egm2/blast.c)'s does, or repairs the division. Nothing checks a new division. `NJS_MATRIX` aligns to 8, since its paired `FMOV`s fault off 8 on a console, and `-fpack-struct=4` keeps SHC's member layout, with `pack(8)` for `NJS_VIEW` and `NJS_LIGHT` ([`sdk/include/njtypes.h`](sdk/include/njtypes.h)).

## Content

make gcc compiles no `gen/` range's C, which binds names by address; [`tools/content.py`](tools/content.py) writes the content instead, each definition as `config/<image>/data.txt` gives it ([Generated ranges](TOOLCHAIN.md#generated-ranges)). A word becomes a pointer only on evidence: a structure or type its row gives, or `vouched` on the row. An untyped word in RAM, outside a text, fails. Each pointer into RAM becomes a name, a copy of a text a source declares, or an unresolved spelling the link fails on. Any other pointer, null among them, stays a number. `1ST_READ.BIN`'s content takes another image's name only through a `path_lists` key, whose stage gives the images.

GCC compiles each content with each definition in a section of its own, and GNU ld links it at the disc's addresses, to a binary that must equal the disc's bytes. The link binds each name, so the check holds each pointer too.

Where the game reads several rows as one, as the missile's draw reads its model's seven meshsets, `data.txt` gives them as one array, and a source names an element of it, as [`missile.c`](modules/stg04/missile.c) does. A source's array that a content reads whole is one definition too, as the signboard's eight materials in [`object_textures.c`](modules/minicart/object_textures.c) are. GCC writes a content's definitions in the image's order, and the final link keeps it, so a read that runs past a definition meets the next, as on the disc.

## Imports and the census

The import check holds each import to its image's definition, and each use in `1ST_READ.BIN`'s C, on each side of its guards, to the images the loader always reads for the use's stages. Only a table's row gives a use stages, never a test of the current stage. Any other use takes a row of [`config/1st_read/uses.txt`](config/1st_read/uses.txt); a use no row covers fails, as does a row that covers none.

The census gives each number it explains a disposition: an `anchor` keeps its address, a `placed` name moves with the link, and `data` is no address. [`tools/census.py`](tools/census.py) fails on each number no row explains: in `1ST_READ.BIN`'s sources, tables and generated data, for RAM past its image, and on any image's GCC side, at an address a table names. With `--disc` it reads each motion file too, and fails on a word that holds an address in `1ST_READ.BIN`'s image where the file's content places no pointer, since the link leaves that word as the disc holds it. [Checks](CONTRIBUTING.md#checks) says what neither check shows.

## Coordinated link

Each image links at its tables' base, from its objects in relink's order. Each record a loader reads at a fixed offset, as an event's head, opens the image, and `ADVERTISE`'s `DEND` and `STG08`'s fish models close theirs. `1ST_READ.BIN`'s code opens with its draw kernel, the functions [`config/1st_read/kernel.txt`](config/1st_read/kernel.txt) lists, grouped by the step of a draw that calls them. The kernel fits within the SH-4's 8 KB instruction cache, so that no two of its lines take one set of it, and a larger one fails the link. It changes the order of no other section, but moves the code it goes ahead of.

Each name an input refers to and none defines binds to the first that fits: an imports table's row, an anchor of the memory contract, or the live image whose span holds its address and that defines it there. A row of [`config/imports.txt`](config/imports.txt) gives what the build can't derive, as `skip_owed`, which only `gcc/` defines; its provider must be live with the image, except in a [hand-off](TOOLCHAIN.md#image-tables). A name nothing binds fails the link, listed in `build/gcc/link/unbound.txt`.

ld.py links every image twice: the first link lays every image out, and each import takes its provider's address there; the second, the final link, must keep the first's layout. `STARTOF` and `SIZEOF`, by which SHC's assembly reads a run of sections' bounds, take the final link's.

verify.py then checks each binding, anchor and archive member of the final links, and holds each declaration of a name to the definition the link binds, as GCC's DWARF gives their types, and to the alignment its address needs; [`config/declarations.txt`](config/declarations.txt) admits the rest. In the matching build a second name binds by address, so a guard repairs a call linked over a declaration that isn't the definition's, as `ground_below_int`'s.

## Memory contract and disc

[`tools/memory.py`](tools/memory.py) reads each area from the source that fixes it, as `loader.h`'s work area, and each state, what can be live together, from the loaders' reads. Its owners include the work past an image, whose size it gives: `ADVERTISE`'s backup work, and Zero's and E-101 mkII's banks. It fails on an image past the RAM it may take, overlapping owners in a state, a PRS file too big for the work area, and an event whose bss runs past its file and what `event_load` zeroes. In `1ST_READ.BIN` and `LOADER.BIN`, publish.py holds the startup's `_INITSCT` to zero all of the bss and nothing else, and in `MOVIE.BIN` each `PSGSFD` section to start near the end of the one before, as `MPV_Init` checks.

[`tools/disc.py`](tools/disc.py) fails on a manifest that leaves out an image `config/` describes, or on a file whose hash isn't the manifest's. The boot file, `1ST_READ.BIN`, must keep the original's last sector and cannot grow beyond its sector count; if it uses fewer sectors, it starts later. Any other file keeps its original's first sector unless it needs more, and then moves, in path order, to the first free run that holds it. disc.py holds the disc to the contract and the original's map, reading back each file it replaced, and `build/gcc/disc/` appears only once every check passes.
