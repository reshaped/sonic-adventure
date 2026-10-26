# Contributing

Contributions are welcome! There's plenty of logic in this repo that we need to understand better.

A better name, a comment that explains the game's behavior or unusual code, or a clearer C spelling for a function that already matches is worth sharing. So is a typed declaration for data a `gen/` range imports from the disc, matching C in place of open assembly (`make review` lists each `@asm open` tag), stronger evidence for a [tag](TAGS.md), or an improvement to the build and its tools.

Not sure about something? Open an issue, or a draft pull request with whatever you've got. It doesn't have to be finished. [README.md](README.md#build) gets you to a first build.

## Sending a change

Send pull requests against `main`, one change per commit, with a subject that says what changes. In the pull request, say what changed, which binaries it affects, which [checks](#checks) you ran, and what's still uncertain. A sentence or two is plenty for a small fix.

By sending a pull request, you dedicate whatever rights you hold in your change to the public domain under [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/). Never commit the disc's files, or anything make extracts from them.

No disc? You can still read the code, run the disc-free checks, and open a draft that says what's unverified. A change to sources, tables or tools gets the full build with the disc before it merges: `make check` can't stand in for it.

Call out a change to a recorded hash, to what counts as code, to the splice rules or to `config/kept.txt` separately: those change what passing means ([What passing means](#what-passing-means)).

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

`make test` and `make review` need no disc, SHC, C compiler or network. `make check` needs a C compiler, and downloads SHC and wibo on first use. `make`, `make relink` and `make shift` need all of that and the disc. `make lint` needs a C compiler, and downloads clang-format and clang-tidy on first use. Where `make check` finds README.md's numbers stale, `make progress` rewrites them.

What passing doesn't show:

- **`make relink` doesn't check every reference between sources.** Where no object defines a name the tables give inside the binary, the whole link binds it at that address, as each source's own link does. `make relink V=1 -W tools/relink.py` lists each one ([Build model](TOOLCHAIN.md#build-model)).
- **`make shift` doesn't show a binary holds no fixed address.** A word that keeps a value inside the binary passes where `config/kept.txt` gives a reason, and a reason is a reviewer's reading of the bytes, not a proof. It leaves other binaries at their original addresses, so it can't tell a hard-coded address into one from a linked reference ([Build model](TOOLCHAIN.md#build-model)).
- **`make check` is not a full rebuild.** It builds no binary and no `gen/` range, and checks a source whose initializers come from the disc through its [stub build](TOOLCHAIN.md#build-model), with placeholders for the data. Only `make` checks the disc's data.
- **Lint and review check a tag's form, not its argument** ([Reviewing a tag](TAGS.md#reviewing-a-tag)).
- **Clang's diagnostics help us read the C; they don't establish a match.** Clang reads the stub's placeholders and never sees inline assembly. SHC's output and the byte comparisons decide whether the reconstruction matches ([C for SHC](TOOLCHAIN.md#c-for-shc)).

CI runs `make test`, `make check` and `make lint`, and builds the vendored gdi and prskit. It has no disc.

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
3. **Give it a compiler configuration** only if it needs one. Most sources compile with their kind of code's release and flags. Anything else goes in [`config/compilers.mk`](config/compilers.mk) with the evidence for it.
4. **Update the tables.** Add its rows to each binary's `splits.txt` and its names to `symbols.txt`, and shorten or remove the old writer's rows. Split what's left of a `gen/` range into rows at each remaining run's start ([Generated ranges](TOOLCHAIN.md#generated-ranges)). Where fill and alignment don't decide a boundary between two objects, record the evidence beside the row ([Seams in the tables](TOOLCHAIN.md#seams-in-the-tables)).
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

The C is SHC 5.0's C89, where the order of declarations decides where data lands ([C for SHC](TOOLCHAIN.md#c-for-shc)).

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

`SHA256SUMS`, what counts as code, the splice rules and `config/kept.txt` decide what passing means. Changing them deserves its own review.

Never update a hash to accept a mismatch. An original file's hash changes only with corrected input evidence. A linked source's or a stub build's hash changes only with an explained change, recorded once `make` matches every binary that places the source.

| Change | Evidence |
|---|---|
| An original file's hash | The corrected input and where it comes from. Never a rebuilt file's hash. |
| A linked source's or stub build's hash | Why its bytes changed, and the full build passing. |
| What counts as code | `make progress`'s totals, and each changed binary's code from `make V=1`, before and after. |
| The splice rules | A splice the change turns from pass to fail or fail to pass, and every binary's `make V=1` counts before and after. |
| A row of `config/kept.txt` | What the word is, read from the bytes, and why no name fits. |

`make V=1 -W tools/splice.py` splices every binary again and prints its counts. They should change only where the change intends. [Accounting](TOOLCHAIN.md#accounting) defines what each count measures.
