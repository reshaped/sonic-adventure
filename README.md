# sonic-adventure

The first complete matching decompilation of a retail Dreamcast game, including all of the Katana SDK code it links: *Sonic Adventure*, the original Japanese release, HDR-0001, version 1.007.

<!-- progress -->
**100% of the game's code builds from source, byte for byte.**

| | Binaries | Code bytes | C\* | Assembly |
|---|---:|---:|---:|---:|
| Main executable | 1 | 1,090,074 | 95.5% | 4.5% |
| SDK libraries | | 779,826 | 89.4% | 10.6% |
| Stages | 15 | 1,100,478 | 100.0% | 0% |
| Bosses | 11 | 331,606 | 100.0% | 0% |
| Fields and Chao | 12 | 507,358 | 99.9% | 0.1% |
| Events | 204 | 1,947,476 | 100.0% | 0% |
| Menus and system | 4 | 63,640 | 99.9% | 0.1% |
| **All** | **247** | **5,820,458** | **97.7%** | **2.3%** |

*\*SHC's assembly for 14 of the C sources matches once [shckit](https://github.com/reshaped/shckit) patches it ([Patched R10](TOOLCHAIN.md#patched-r10)). [Accounting](TOOLCHAIN.md#accounting) explains the percentages.*

Most of the assembly was handwritten in the original, not compiled from C. Compiler analysis and original symbol names already provide [evidence](TAGS.md#assembly) of handwritten origins for 62.4% of its instructions and data.
<!-- /progress -->

## Build

Tested on x86-64 Linux with GNU make 4.3 and Python 3.8. You also need a C compiler (`cc` by default, or set `CC`), `sha256sum` and `timeout`. The first build downloads wibo and three releases of Hitachi's SHC, so it also needs `curl`, `tar` and `gzip`. The first `make lint` or `make format` installs clang-format and clang-tidy into a Python venv, so it needs `curl` and Python's `ensurepip`, which Debian and Ubuntu ship as `python3-venv`; the pip it installs must be 20.3 or later. Each download is installed only once its hash matches [`tools/SHA256SUMS`](tools/SHA256SUMS).

Bring your own disc: the Japanese release, HDR-0001, version 1.007. None of the disc's files are in this repository. Put `1ST_READ.BIN`, `LOADER.BIN` and `SONICADV/` in `orig/`, named as on the disc, or put the disc's `.gdi` and its tracks there. Then:

```sh
make          # rebuild every binary and check it against your disc
make check    # check the linked sources without the disc
make gcc      # build the port with GCC, in build/gcc/
make gcc-disc # build a disc of the port, in build/gcc/disc/
```

`make` compiles every binary's code, reads the data its sources declare from your disc, and checks each binary it rebuilds against the original, PRS compression and all. They land under `build/`, at the disc's own paths. `make` checks `1ST_READ.BIN`, `LOADER.BIN` and each file it reads from `SONICADV/` against [`SHA256SUMS`](SHA256SUMS) before reading it. This is a matching build, so an edit that changes a byte of output fails it.

`make check` needs no disc. It compiles the sources it can check without the disc, using placeholders for imported data where needed, and checks each linked source against its recorded hash, but builds no complete binary. [Checks](CONTRIBUTING.md#checks) says what each goal shows, and `make help` lists them all.

`make gcc` builds the same sources with GCC 16.2 and links every image anew, under `build/gcc/` at the disc's paths. It needs what `make` needs, the disc and SHC included. Its first run downloads and builds GCC's toolchain, so it also needs `xz` and `c++`, and both GCC goals need `cmp`. `make gcc-disc` needs the disc's `.gdi` and its tracks in `orig/`, not its files alone. The port adds 16:9, 60 fps and a frame counter, each off until you switch it on, and skips for movies and cutscenes ([Features](GCC.md#features)).

Run one `make` invocation at a time per checkout; `-j` within it is fine. SHC is a Windows program, and [wibo](https://github.com/decompals/wibo) runs it. SHC sometimes hangs under wibo, so `make` stops a tool after ten minutes. Run `make` again and it carries on. To use tools you already have, set `WIBO`, `R10`, `R26`, `R32` and `SH_ELF`: [Running SHC](TOOLCHAIN.md#running-shc) and [Toolchain](GCC.md#toolchain) say what each must hold.

## How the matching build works

- Each binary has two tables in `config/`: `splits.txt` places each source's sections at their original addresses, and `symbols.txt` names the addresses the code refers to. A binary with data no source holds has a third, `data.txt`, which types that data ([Generated ranges](TOOLCHAIN.md#generated-ranges)).
- Each source compiles with a release of Hitachi's SHC 5.0 and flags chosen to reproduce its bytes. Some sources require documented assembly patches or grafts to achieve a match ([Releases](TOOLCHAIN.md#releases)). Shared sources compile once and link at each binary's original addresses.
- Each binary is rebuilt from compiled source, explicitly declared disc data and linker fill. Any unaccounted-for byte fails the build.
- For PRS files, the original is decompressed before the byte check and the rebuilt image is compressed afterward. Every binary `make` rebuilds is checked against the original.

Matching binaries establish identical output, not the original source text or build settings.

## The binaries

*Sonic Adventure* isn't one binary. Its main executable, `1ST_READ.BIN`, stays in memory, and 246 overlays take turns beside it, among them the stages, the bosses, the fields, the Chao gardens, the menus, the movie player and the story's scenes. Even Sonic's opening scene in Station Square, `EV0001`, is an overlay of its own. [Images](TOOLCHAIN.md#images) maps where each one loads.

## Layout

```text
src/       the main executable
sdk/       the main executable's SDK libraries and C runtime, and two of the events' runtime helpers
modules/   stages, bosses, fields, Chao, menus and the movie player
events/    the story's scenes, and the code they share
loader/    the hand-off to Dream Passport
gcc/       the port's own sources, which only GCC compiles
config/    each binary's and motion file's tables, make gcc's link tables, the compilers' settings and the shift test's kept words
tools/     the build's tools: its own, vendored and downloaded
tests/     the tools' tests
```

## Contributing

Contributions are welcome. [CONTRIBUTING.md](CONTRIBUTING.md) says what helps, and follows one source from its placement table to the checked binary.

## Read on

- [TOOLCHAIN.md](TOOLCHAIN.md) documents the build, compiler releases, disc data and accounting.
- [GCC.md](GCC.md) documents the port to GCC: its build, its features and the C it needs.
- [TAGS.md](TAGS.md) defines the source annotations and their evidence requirements.
- [prskit](https://github.com/reshaped/prskit), [gdi](https://github.com/reshaped/gdi) and [shckit](https://github.com/reshaped/shckit) are companion tools for PRS compression, disc extraction, and SHC assembly analysis and patching.

## Acknowledgements

Special thanks to some amazing engineers:

- [wibo](https://github.com/decompals/wibo), by decompals, runs Hitachi's Windows toolchain on Linux. It makes the matching build possible.
- [decomp.me](https://decomp.me) maintains the [compiler archive](https://github.com/decompme/compilers) used by decompilation projects, including the three SHC releases this build downloads.
- [SH4ZAM](https://github.com/gyrovorbis/sh4zam), by Falco Girgis, provides hand-tuned SH-4 math routines that the GCC build uses.

## License

The Makefile, `gcc.mk`, `config/compilers.mk`, `config/gcc.mk` and the files committed to `tools/` and `tests/` are under the [MIT license](tools/LICENSE). The rest of the repository has no license.

This project isn't affiliated with or endorsed by SEGA.
