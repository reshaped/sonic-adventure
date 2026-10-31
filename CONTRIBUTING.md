# Contributing

Contributions are welcome! There's plenty of logic in this repo that we need to understand better.

A better name, a comment that explains the game's behavior or unusual code, or a clearer C spelling for a function that already matches is worth sharing. So is a typed declaration for data a `gen/` range imports from the disc, or a truer row for it in `data.txt`, matching C in place of open assembly (`make review` lists each `@asm open` tag), stronger evidence for a [tag](TAGS.md), or an improvement to the build and its tools.

Not sure about something? Open an issue, or a draft pull request with whatever you've got. It doesn't have to be finished. [README.md](README.md#build) gets you to a first build.

## Sending a change

Send pull requests against `main`, one change per commit, with a subject that says what changes. In the pull request, say what changed, which binaries it affects, which [checks](#checks) you ran, and what's still uncertain. A sentence or two is plenty for a small fix.

By sending a pull request, you dedicate whatever rights you hold in your change to the public domain under [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/). Never commit the disc's files, or anything make extracts from them.

No disc? You can still read the code, run the disc-free checks, and open a draft that says what's unverified. A change to sources, tables or tools gets the full build with the disc before it merges: `make check` can't stand in for it. A change to what `make gcc` reads gets `make gcc` too, and one to GCC's side alone, in a guard or in `gcc/`, gets `make gcc` in place of the full build.

Call out a change to a recorded hash, to what counts as code, to the splice rules or to `config/kept.txt` separately: those change what passing means ([What passing means](#what-passing-means)). For `make gcc`, so does a change to anything that section names for it.

## Checks

| Command | Passing shows |
|---|---|
| `make` | Every binary rebuilds byte for byte, PRS compression and all. |
| `make relink` | Every binary's objects link in one pass, and the result matches its splice byte for byte ([Build model](TOOLCHAIN.md#build-model)). |
| `make shift` | Each binary's whole link is tested at two shifted bases. Every aligned 32-bit word moves by each shift or keeps its value under both; `config/kept.txt` gives a reason for any unchanged value in the binary's span ([Build model](TOOLCHAIN.md#build-model)). |
| `make check` | Every linked source that builds without the disc matches its recorded hash, every byte of code has a writer, and README.md's numbers are current. |
| `make test` | The tools' tests pass, and the vendored tools match their pins. |
| `make lint` | C formatting and diagnostics, `.src` checks, and tags ([What lint checks](TAGS.md#what-lint-checks)). |
| `make review` | The tag checks, plus a list of the `@asm open` tags ([Assembly evidence](TAGS.md#assembly-evidence)). |
| `make gcc` | Every object it builds compiles or assembles with no warning, and each image's content, linked at the disc's addresses, matches the disc's data byte for byte. The census and import checks pass, and every image links anew with each name bound and checked, within the memory contract ([Build model](GCC.md#build-model)). |
| `make gcc-disc` | A disc of that link builds, each file it replaces reads back at the manifest's hash, and each file it keeps lies where it did ([Memory contract and disc](GCC.md#memory-contract-and-disc)). |
| `make accept` | `make lint`, `make`, `make check`, `make gcc`, `make gcc-disc` and `make test` pass in turn at `HEAD`, on a tree that matches it, the last with the GCC gates' toolchain, and `build/accept.txt` ends `accepted`. |

`make test` and `make review` need no disc, SHC, C compiler or network. `make check` needs a C compiler, and downloads SHC and wibo on first use. `make`, `make relink` and `make shift` need all of that and the disc. `make lint` needs a C compiler, and downloads clang-format and clang-tidy on first use. `make gcc` and `make gcc-disc` need what `make` does, and on first use download SH4ZAM and build their toolchain ([Toolchain](GCC.md#toolchain)). `make gcc-disc` needs the disc's `.gdi` and its tracks, not its files alone. `make test` skips a test that needs `make gcc`'s toolchain or SH4ZAM where what it needs is missing. Where you set `SH_ELF`, a test that needs a tool its toolchain lacks fails instead. `make accept` needs what its gates need, and git. Where `make check` finds README.md's numbers stale, `make progress` rewrites them.

What passing doesn't show:

- **`make relink` doesn't check every reference between sources.** Where no object defines a name the tables give inside the binary, the whole link binds it at that address, as each source's own link does. `make relink V=1 -W tools/relink.py` lists each one ([Build model](TOOLCHAIN.md#build-model)).
- **`make shift` doesn't show a binary holds no fixed address.** A word that keeps a value inside the binary passes where `config/kept.txt` gives a reason, and a reason is a reviewer's reading of the bytes, not a proof. It leaves other binaries at their original addresses, so it can't tell a hard-coded address into one from a linked reference ([Build model](TOOLCHAIN.md#build-model)).
- **`make check` is not a full rebuild.** It builds no binary and no `gen/` range, and checks a source whose initializers come from the disc through its [stub build](TOOLCHAIN.md#build-model), with placeholders for the data. Only `make` checks the disc's data.
- **Lint and review check a tag's form, not its argument** ([Reviewing a tag](TAGS.md#reviewing-a-tag)).
- **Clang's diagnostics help us read the C; they don't establish a match.** Clang reads the stub's placeholders and never sees inline assembly. SHC's output and the byte comparisons decide whether the reconstruction matches ([C for SHC](TOOLCHAIN.md#c-for-shc)).
- **The matching build doesn't show that `1ST_READ.BIN` names the right image's definition.** Several images define something at one address, as `STG01` and `STG02` each define their step at `0x0c900000`, so naming one's step where `stage_steps` gives the other's would build the same bytes. `make gcc`'s import check holds each use in `1ST_READ.BIN`'s C to the images the loader always reads for its stages, and the rest to rows of `config/1st_read/uses.txt`, each a reviewer's reading of the evidence. A use takes the stages of where its source names it, not of where it runs ([Imports and the census](GCC.md#imports-and-the-census)).
- **`make gcc`'s census doesn't show that `1ST_READ.BIN` holds no other address past its image.** Its scans read literal constants, and a macro plus or minus a constant, not an address built at run time, encoded or split across constants. A row with a span counts its numbers, not which they are ([Imports and the census](GCC.md#imports-and-the-census)).
- **`make gcc` and `make accept` don't show the port plays as shipped, or at all.** No gate boots the disc or plays it, in either aspect at either rate, and `make accept` runs neither `make relink` nor `make shift`. Only running the disc outside the tree, in an emulator or on a console, shows that it plays.

CI runs `make test`, `make check` and `make lint`, and builds the vendored gdi and prskit. It has no disc, so it runs no `make gcc`. A second job runs `make test` with `make gcc`'s toolchain and SH4ZAM, so the tests that need them run too.

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
grep 'modules/stg05/camera_sweep.bin$' SHA256SUMS | sha256sum -c
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
3. **Give it a compiler configuration** only if it needs one. Most sources compile with their kind of code's release and flags. Anything else goes in [`config/compilers.mk`](config/compilers.mk) with the evidence for it. [Flags and warnings](GCC.md#flags-and-warnings) says which of its flags GCC reads.
4. **Update the tables.** Add its rows to each binary's `splits.txt` and its names to `symbols.txt`, and shorten or remove the old writer's rows. A name `1ST_READ.BIN` takes from an image beside it goes in `config/1st_read/imports.txt` instead, with the image's own name for it where the two differ. The matching build links it at the address the image's `symbols.txt` gives the image's name ([Image tables](TOOLCHAIN.md#image-tables)). Where `make gcc`'s import check or census fails, change the rows of `config/1st_read/uses.txt` or `config/1st_read/census.txt` ([Imports and the census](GCC.md#imports-and-the-census)). `make test` checks `uses.txt` against `1ST_READ.BIN`'s C too. Split what's left of a `gen/` range into rows at each remaining run's start. In the image's `data.txt`, drop each row that lies wholly in the bytes the source takes, and cut each that runs across their start or end, writing each new row by hand from the bytes and the code that reads them ([Generated ranges](TOOLCHAIN.md#generated-ranges)). Where fill and alignment don't decide a boundary between two objects, record the evidence beside the row ([Seams in the tables](TOOLCHAIN.md#seams-in-the-tables)).
5. **Build with the disc.** The build fails on a byte nothing writes, or one that two sources write.
6. **Record the new linked sources' hashes.** Once every binary builds, `sums` names each linked source missing from `SHA256SUMS` and each line for one no longer built. Remove those lines, and add each missing one:

   ```sh
   sha256sum LINK >> SHA256SUMS
   LC_ALL=C sort -k 2 -o SHA256SUMS SHA256SUMS
   ```

   Shortening a source keeps its paths, so `sums` won't name its lines; `make`'s hash check fails on them instead. That's expected: replace those lines the same way, for its linked and stub builds alike, and say why its bytes changed ([What passing means](#what-passing-means)).

   Then run `make` again to check the new lines.

7. **Run the [checks](#checks).**

## Writing for this tree

The C SHC compiles is SHC 5.0's C89, where the order of declarations decides where data lands ([C for SHC](TOOLCHAIN.md#c-for-shc)). The C only GCC compiles is GNU C17: `gcc/`'s, GCC's side of each guard, and each header only that C includes ([Sources in `gcc/`](GCC.md#sources-in-gcc)). Where `make gcc` needs other C, a spelling SHC compiles to the same bytes replaces the shipped one, or a guard holds GCC's beside it ([Guards](GCC.md#guards)). A name or a behavior only the port has stays in the C only GCC compiles, even where SHC would build the same bytes with it.

A comment says what the code can't: at a declaration, its contract; in the code, a reason, a precondition, a unit, a sentinel, an invariant or an exception. It doesn't narrate the statements, or claim an intent the bytes don't show. In C, a comment is `/* */`, lowercase, in the present tense, with no closing period; names, instructions and registers keep their case:

```c
/* a stub build's zero body would make an array declared [] one element
 * long. the extent stays in the declaration, so both builds lay it out alike */
```

An odd construct kept for the match, a bug the game shipped with, or assembly kept as assembly gets a tag ([TAGS.md](TAGS.md)).

`make format` formats the C in place and keeps each file's include order; run `make check` after it, since even a mechanical edit can move SHC's output. clangd and clang-tidy read `compile_flags.txt`, and `make stub-includes` writes the stub `.inc` files they need. Clang has no SH-4 target, so the flags use `i686-unknown-none-elf`, whose `int`, `long` and pointers are 32 bits, as SH-4's are.

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
- `sdk/ninja/njCameraMotionOld.c`, the grafted route's donor, takes the name SDK 1.20J's `njCameraMotion_.obj` gives the donor's `njCameraMotion`; the member is SDK 1.00J's `njCameraMotion_.obj` ([Grafted R10](TOOLCHAIN.md#grafted-r10)).
- Identical single-precision functions take the name their importer needs: ADX imports `cos`, so the source is `sdk/runtime/cos.c`.
- Sound and ADX members missing from SDK 1.20J take their 1.30J `sg_sd` and `sofdec` names, such as `sd_memblk_dma_transfer.c` and `adx_tlk2.c`.
- Code behind the `uf Ver 1.04` banner takes Shinobi 1.42's member names, such as `sg_playtime.c`.
- SDK 1.30J's Kamui members of `ninja.lib` stay in `sdk/ninja/`, with `_kamui_lib` names.

Source paths can't contain `-`, and file names take one dot ([Running SHC](TOOLCHAIN.md#running-shc)).

## What passing means

`SHA256SUMS`, what counts as code, the splice rules and `config/kept.txt` decide what passing means. For `make gcc`, so do `config/1st_read/uses.txt`, `config/1st_read/census.txt`, `config/imports.txt`, a hand-off row of `config/1st_read/imports.txt`, `vouched` on a row of `data.txt`, `config/declarations.txt` and the `CONTRACTS` in `config/gcc.mk`. Changing them deserves its own review.

Never update a hash to accept a mismatch. An original file's hash changes only with corrected input evidence. A linked source's or a stub build's hash changes only with an explained change, recorded once `make` matches every binary that places the source. A line for an original the build comes to read, as each motion file's, adds an input: make's count of files that match rises by one, and no output changes.

| Change | Evidence |
|---|---|
| An original file's hash | The corrected input and where it comes from. Never a rebuilt file's hash. |
| A new original file's line | The step that reads the file, and the file as make extracts it from the disc. |
| A linked source's or stub build's hash | Why its bytes changed, and the full build passing. |
| What counts as code | `make progress`'s totals, and each changed binary's code from `make V=1`, before and after. |
| The splice rules | A splice the change turns from pass to fail or fail to pass, and every binary's `make V=1` counts before and after. |
| A row of `config/kept.txt` | What the word is, read from the bytes, and why no name fits. |
| A row of `config/1st_read/uses.txt` | The code that shows the use runs only while the row's image is loaded. |
| A row of `config/1st_read/census.txt` | What the number is, read from the code that holds it, and why its disposition fits. |
| A row of `config/imports.txt` | Why the build can't bind the name by itself, and for a hand-off, the code that reads the provider's file and then starts it. |
| A hand-off row of `config/1st_read/imports.txt` | The code that reads the image's file and then starts it. |
| `vouched` on a row of `data.txt` | Why each of the row's words in RAM is an address, read from the bytes and the code that reads them. |
| A row of `config/declarations.txt` | Why GCC's code still gives the image's value, or why the game never runs it. |
| An object's `CONTRACTS` in `config/gcc.mk` | How GCC's callers meet the contract of each function it names, or that no GCC code calls it. |

`make V=1 -W tools/splice.py` splices every binary again and prints its counts. They should change only where the change intends. [Accounting](TOOLCHAIN.md#accounting) defines what each count measures.
