# Contributing

Contributions are welcome! There's plenty of logic in this repo that we need to understand better.

A better name, a comment that explains the game's behavior or unusual code, or a clearer C spelling for a function that already matches is worth sharing. So is a typed declaration for data a `gen/` range imports from the disc, matching C in place of open assembly (`make review` lists each `@asm open` tag), stronger evidence for a [tag](TAGS.md), or an improvement to the build and its tools.

Not sure about something? Open an issue, or a draft pull request with whatever you've got. It doesn't have to be finished. [README.md](README.md#build) gets you to a first build.

## Sending a change

Send pull requests against `main`, one change per commit, with a subject that says what changes. In the pull request, say what changed, which binaries it affects, which [checks](#checks) you ran, and what's still uncertain. A sentence or two is plenty for a small fix.

By sending a pull request, you dedicate whatever rights you hold in your change to the public domain under [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/). Never commit the disc's files, or anything make extracts from them.

No disc? You can still read the code, run the disc-free checks, and open a draft that says what's unverified. Changes to matching sources, tables or tools get the full build with the disc before merging: `make check` can't stand in for it. Changes confined to non-matching code don't need a matching rebuild; run the checks for each port they affect (`make dc` and `make dc-disc`, or `make pc` and `make native`), and `make engine-test` for engine changes.

Call out changes to recorded hashes, code accounting, splice rules or other verification tables separately ([What passing means](#what-passing-means)).

## Checks

| Command | Result |
|---|---|
| `make` | Every binary rebuilds byte for byte against the disc, PRS compression and all. |
| `make relink` | Each binary's objects link in one pass and match its splice byte for byte ([Build model](TOOLCHAIN.md#build-model)). |
| `make shift` | Each whole link is tested at two shifted bases. Every aligned 32-bit word moves by both shifts or stays unchanged under both; `config/kept.txt` accounts for unchanged values in the binary's span ([Build model](TOOLCHAIN.md#build-model)). |
| `make check` | Without the disc, checks linked-source hashes, code coverage, imported definitions, tracked offset tables and README.md's numbers. It builds no binary. |
| `make offsets` | Writes the offset tables measured by each body source's stub build ([Stub and full builds](TOOLCHAIN.md#stub-and-full-builds)). |
| `make test` | The tools' tests pass, and the vendored tools match their pins. |
| `make lint` | C and engine C++ formatting, C diagnostics, `.src` checks, tags ([What lint checks](TAGS.md#what-lint-checks)) and guards ([Switches](PORTS.md#switches)). |
| `make review` | Tag checks and a list of the `@asm open` tags ([Assembly evidence](TAGS.md#assembly-evidence)). |
| `make dc` | The GCC Dreamcast port compiles without warnings, checks the disc-derived content, imports and memory limits, and links every image anew ([Build model](PORTS.md#build-model)). |
| `make dc-disc` | Builds a disc from the port's link, checks replaced files against the manifest and preserves the other files' placements ([Memory contract and disc](PORTS.md#memory-contract-and-disc)). |
| `make dc-check` | Without the disc, builds and hashes the port's disc-independent objects and supporting tools in `build/dc/check.txt`, and checks the imports, census and offset tables it can read. Compare the sums across hosts at the same commit and with the same `SH_ELF` ([Build model](PORTS.md#build-model)). |
| `make dc-order` | Writes the record of where each warm row lies, moving only a row that no longer fits its place. `SEARCH=1` searches every place again and `PACK=1` lays the rows end to end; both move code. It writes files, not a passing check ([Coordinated link](PORTS.md#coordinated-link)). |
| `make pc` | Builds the PC game, `play`, from the disc; it doesn't run it ([Native game](ENGINE.md#native-game)). |
| `make native` | Runs `play` headless for 600 vsyncs from Emerald Coast's first act, and checks that missing startup symbols are rejected ([Starting the game](ENGINE.md#starting-the-game)). |
| `make engine-test` | The engine's C ABI compiles and runs as C17 and C++17, and its tests pass ([Build goals](ENGINE.md#build-goals)). |
| `make accept` | On a tree matching `HEAD`, runs the matching, Dreamcast and PC gates and writes their results and output hashes to `build/accept.txt`. It doesn't run either game's display. |

`make test` and `make review` need no disc, SHC or C compiler; `make test` needs git. `make check` and `make offsets` use SHC and wibo but no disc. `make`, `make relink` and `make shift` also need the disc. `make lint` downloads clang-format and clang-tidy on first use. `make dc` and `make dc-disc` need the disc, but neither SHC nor wibo; they build their GCC toolchain and fetch SH4ZAM on first use. `make dc-disc` needs the disc's `.gdi` and tracks, not just its extracted files. `make dc-check` needs the toolchain but no disc. `make test` skips toolchain-dependent tests where the tools aren't installed; setting `SH_ELF` makes a missing tool an error. `make accept` needs everything its gates need. Where `make check` finds README.md's numbers stale, `make progress` rewrites them.

`make pc` needs the disc, a C compiler, `clang`, `readelf`, `nm`, `objcopy`, a C++17 compiler and its standard library. It downloads Vulkan's headers, glslang and pl_mpeg on first use, and needs Python 3.9 or later. `make native` runs after `make pc` and also needs `strip`; it needs no display. `make play` needs SDL3 and a Vulkan device for its window, input and sound. `make engine-test` needs Clang and a C++17 compiler, but no disc; some tests also need SDL3, SH4ZAM or the SH-4 toolchain. [Build](README.md#build) gives the supported hosts and installation requirements.

What passing doesn't show:

- **`make relink` doesn't check every reference between sources.** Where no object defines a name the tables give inside the binary, the whole link binds it at that address, as each source's own link does. `make relink V=1 -W tools/relink.py` lists each one ([Build model](TOOLCHAIN.md#build-model)).
- **`make shift` doesn't show a binary holds no fixed address.** A word that keeps a value inside the binary passes where `config/kept.txt` gives a reason, and a reason is a reviewer's reading of the bytes, not a proof. It leaves other binaries at their original addresses, so it can't tell a hard-coded address into one from a linked reference ([Build model](TOOLCHAIN.md#build-model)).
- **`make check` is not a full rebuild.** It builds no binary and no `gen/` range, and checks a source whose initializers come from the disc through its [stub build](TOOLCHAIN.md#build-model), with placeholders for the data. Only `make` checks the disc's data.
- **Lint and review check a tag's form, not its argument** ([Reviewing a tag](TAGS.md#reviewing-a-tag)).
- **Clang's diagnostics help us read the C; they don't establish a match.** Clang reads the stub's placeholders and never sees inline assembly. SHC's output and the byte comparisons decide whether the reconstruction matches ([C for SHC](TOOLCHAIN.md#c-for-shc)).
- **The import and census checks don't prove every name or address is right.** Different images can define a name at the same address. The import check uses the recorded stage loads and `uses.txt`, which still needs review; the census catches literal addresses and simple expressions, not every address constructed at runtime ([Imports and the census](PORTS.md#imports-and-the-census)).
- **`make dc-check` is not `make dc`.** It links no image and checks no disc-derived content. Its sums aren't reference hashes: comparison across hosts shows only whether they built the same covered objects and tools. Only `make check` verifies offset tables against stub builds.
- **The gates don't show how either port plays.** `make dc` and `make accept` don't boot the Dreamcast disc; `make native` runs without pictures, sound or a pad. Test the disc on a console or emulator, and the PC game with `make play`. `make accept` also runs neither `make relink` nor `make shift`.

CI runs `make test`, `make check` and `make lint`, and builds gdi and prskit. It has no disc, so it runs no `make dc`. A second job runs `make test` with the Dreamcast toolchain and SH4ZAM, for the tests that need them.

## One source, end to end

`modules/stg05/camera_sweep.c` holds a camera sweep, three functions that two stages use: Windy Valley (`STG02`) and Red Mountain (`STG05`). It compiles once, and each stage's table places the object at that stage's own addresses:

```sh
grep 'stg05/camera_sweep' config/*/splits.txt
```

```text
config/stg02/splits.txt:modules/stg05/camera_sweep.c           P        0x0c9032e0  0x0c9034c0
config/stg02/splits.txt:modules/stg05/camera_sweep.c           D        0x0c9ec528  0x0c9ec534
config/stg02/splits.txt:modules/stg05/camera_sweep.c           B        0x0ca15ba0  0x0ca15bc0
config/stg05/splits.txt:modules/stg05/camera_sweep.c           P        0x0c902a00  0x0c902be0
config/stg05/splits.txt:modules/stg05/camera_sweep.c           D        0x0caaf1a0  0x0caaf1ac
config/stg05/splits.txt:modules/stg05/camera_sweep.c           B        0x0cadd690  0x0cadd6b0
```

`P` holds the three functions, `D` the 12 bytes of `sweep_unit`, and `B` the 32 bytes of the other variables. A source is a whole object, data and all ([Image tables](TOOLCHAIN.md#image-tables)).

Without the disc, build both linked sources and check their recorded hashes:

```sh
make -W modules/stg05/camera_sweep.c build/stg02/modules/stg05/camera_sweep.bin build/stg05/modules/stg05/camera_sweep.bin
grep 'modules/stg05/camera_sweep.bin$' SHA256SUMS | sha256sum -c -
```

`-W` treats the source as modified, so make compiles it again. Both lines should end in `OK`.

With the disc, rebuild both stages and compare them with the originals:

```sh
make build/SONICADV/STG02.PRS build/SONICADV/STG05.PRS
cmp build/SONICADV/STG02.PRS orig/SONICADV/STG02.PRS
cmp build/SONICADV/STG05.PRS orig/SONICADV/STG05.PRS
```

`cmp` prints nothing when the files match. Had the source compiled to one different byte, the build would stop with:

```text
splice: modules/stg05/camera_sweep.c: its byte at 0x0c903460 differs from the disc's
```

Look at the source and its rows; don't change a hash just to make a mismatch go away ([What passing means](#what-passing-means)).

## Adding a source

A new source takes its bytes from whatever writes them now: another source, or a `gen/` range.

1. **Find every binary that holds the bytes.** Replace `WRITER` in `grep -l '^WRITER ' config/*/splits.txt` with the source path to list the binaries that place it. A `gen/` range belongs to one binary, so a copy elsewhere won't show.
2. **Write the whole object**: every section, in declaration order ([C for SHC](TOOLCHAIN.md#c-for-shc), [Objects](TOOLCHAIN.md#objects)).
3. **Give it a compiler configuration** only if it needs one. Most sources use their kind of code's release and flags. Record exceptions in [`config/compilers.mk`](config/compilers.mk), with their evidence ([Flags and warnings](PORTS.md#flags-and-warnings)).
4. **Update the tables.** Add its placements to each affected binary's `splits.txt` and its names to `symbols.txt`; shorten or remove the old writer's rows. Split any remaining `gen/` range and update its definitions in `data.txt`, using the bytes and the code that reads them as evidence ([Generated ranges](TOOLCHAIN.md#generated-ranges)). If `1ST_READ.BIN` takes a name from another image, record the provider in `config/1st_read/imports.txt`. Update `uses.txt` or `census.txt` only when the evidence calls for it ([Imports and the census](PORTS.md#imports-and-the-census)). Where fill and alignment don't settle an object boundary, record the evidence beside its row ([Seams in the tables](TOOLCHAIN.md#seams-in-the-tables)).
5. **Build with the disc.** The build fails on any byte nothing writes, or one that two sources write. A source importing data needs an offset table first; `make offsets` writes it on x86-64 Linux ([Stub and full builds](TOOLCHAIN.md#stub-and-full-builds)).
6. **Record the new linked sources' hashes.** Once every binary builds, `sums` names each linked source missing from `SHA256SUMS` and each line for one no longer built. Remove those lines, and add each missing one:

   ```sh
   sha256sum LINK >> SHA256SUMS
   LC_ALL=C sort -k 2 -o SHA256SUMS SHA256SUMS
   ```

   If a source keeps its path but changes size, update its existing linked and stub hashes as well, with an explanation ([What passing means](#what-passing-means)). Then run `make` again to check the lines.

7. **Run the [checks](#checks).**

## Writing for this tree

SHC compiles the game's matching sources as C89, where declaration order decides where data lands ([C for SHC](TOOLCHAIN.md#c-for-shc)). The Dreamcast port compiles non-matching C as GNU C17; the PC port builds its native slice as GNU C89 under Clang. [Switches](PORTS.md#switches) gives the rules for code shared with these ports.

A comment says what the code can't: at a declaration, its contract; in the code, a reason, a precondition, a unit, a sentinel, an invariant or an exception. It doesn't narrate the statements, or claim an intent the bytes don't show. In C, a comment is `/* */`, lowercase, in the present tense, with no closing period; names, instructions and registers keep their case:

```c
/* a stub build's zero body would make an array declared [] one element
 * long. the extent stays in the declaration, so both builds lay it out alike */
```

An odd construct kept for the match, a bug the game shipped with, or assembly kept as assembly gets a tag ([TAGS.md](TAGS.md)).

`make format` formats the C, and the engine's C++, in place and keeps each file's include order; run `make check` after it, since even a mechanical edit can move SHC's output. clangd and clang-tidy read `compile_flags.txt`, and `make stub-includes` writes the stub `.inc` files they need. Clang has no SH-4 target, so the flags use `i686-unknown-none-elf`, whose `int`, `long` and pointers are 32 bits, as SH-4's are.

### Where sources live

| Code | Directory |
|---|---|
| The main executable's SDK libraries and C runtime | `sdk/` |
| The movie player's own SDK libraries | `modules/movie/sdk/` |
| The loader's own SDK libraries | `loader/sdk/` |
| The events' shared C runtime | `events/shared/sdk/` |

The events' `r_ftou.src` and `r_utof.src` are exceptions, under `sdk/runtime/`. A source several binaries share stays in one place, and each binary's table places it.

SDK sources follow their members' names. Drop a whole member's trailing `_` (`njControl_.obj` becomes `sdk/ninja/njControl.c`), and replace a runtime member's leading `__` with `r_` (`__divls.obj` becomes `r_divls.src`). The exceptions:

- `events/shared/sdk/_calcfpow.c` takes its function's name; the member is `_calcfpo.obj`.
- Identical single-precision functions take the name their importer needs: ADX imports `cos`, so the source is `sdk/runtime/cos.c`.
- Sound and ADX members missing from SDK 1.20J take their 1.30J `sg_sd` and `sofdec` names, such as `sd_memblk_dma_transfer.c` and `adx_tlk2.c`.
- Code behind the `uf Ver 1.04` banner takes Shinobi 1.42's member names, such as `sg_playtime.c`.
- SDK 1.30J's Kamui members of `ninja.lib` stay in `sdk/ninja/`, with `_kamui_lib` names.

Source paths can't contain `-`, and file names take one dot ([Running SHC](TOOLCHAIN.md#running-shc)).

## What passing means

`SHA256SUMS`, what counts as code, the splice rules and `config/kept.txt` decide what the matching build accepts. The Dreamcast port also relies on its import, census, declaration and ABI tables ([PORTS.md](PORTS.md)); the PC build relies on its recorded stubs and source boundaries ([ENGINE.md](ENGINE.md)). Changing these rules or their exceptions deserves its own review.

Never update a hash to accept a mismatch. An original file's hash changes only with corrected input evidence. A linked source's or stub build's hash changes only with an explained change, recorded once `make` matches every binary that places the source.

| Change | Evidence |
|---|---|
| An original file's hash, or a new original input | The corrected disc input and where it came from, or the code that reads a newly tracked file. Never a rebuilt file's hash. |
| A linked source's or stub build's hash | Why its bytes changed, and the full matching build passing. |
| What counts as code | `make progress`'s totals and each affected binary's code counts from `make V=1`, before and after. |
| The splice rules | A splice the change turns from pass to fail or fail to pass, and every binary's `make V=1` counts before and after. |
| A row of `config/kept.txt` | What the word is, read from the bytes, and why no name fits. |
| An import, hand-off or `uses.txt` row | Which image supplies the name, and which code loads or uses it; for a hand-off, the code that loads the file and starts it. |
| A `census.txt` row or `vouched` in `data.txt` | What the word represents, and what the code that reads it establishes about its address or value. |
| A `config/declarations.txt` row or a `CONTRACTS` entry | For a declaration, why GCC still gives the image's value or the game never uses it; for a contract, how GCC's callers meet it or never call it. |
| A `config/parts.txt` row | What the source belongs to, and what its output a game step can read ([Numbers](ENGINE.md#numbers)). |
| An `engine/native/stubs.txt` row | What the symbol does on the console, what uses it, and why the host may replace it or can't support it. |

`make V=1 -W tools/splice.py` splices every binary again and prints its counts. They should change only where the change intends. [Accounting](TOOLCHAIN.md#accounting) defines what each count measures.
