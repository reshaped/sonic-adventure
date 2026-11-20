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

Tested on x86-64 Linux with GNU make 4.3 and Python 3.8. The matching build and PC game run on x86-64 Linux. macOS supports `make dc`, `make dc-disc`, `make dc-check`, `make test`, `make lint` and `make format`, tested on Apple silicon with macOS 26.6, GNU make 4.4.1 and Xcode's Python 3.9.6. The Dreamcast port reads disc data through offsets the matching build records and checks on Linux ([Stub and full builds](TOOLCHAIN.md#stub-and-full-builds)).

On macOS, install Xcode's command-line tools and GNU make. Apple's `make` is 3.81; Homebrew installs GNU make as `gmake`. Use `gmake` wherever the commands below say `make`, from a terminal running natively rather than under Rosetta 2:

```sh
brew install make
```

The builds need a C compiler (`cc` by default, or set `CC`), `sha256sum` and `cmp`. The matching build also needs `timeout`; its first run downloads wibo and three releases of Hitachi's SHC. The Dreamcast port builds its GCC toolchain on first use and needs `c++`. Downloads need `curl` and `tar`, with `gzip` or `xz` where `tar` uses them. The first `make lint` or `make format` installs pinned clang-format and clang-tidy wheels into a Python venv, requiring `curl`, Python's `ensurepip` (`python3-venv` on Debian and Ubuntu) and pip 20.3 or later. The wheels support x86-64 Linux and macOS. Downloads are checked against [`tools/SHA256SUMS`](tools/SHA256SUMS) before installation.

Bring your own disc: the Japanese release, HDR-0001, version 1.007. None of the disc's files are in this repository. Put `1ST_READ.BIN`, `LOADER.BIN` and `SONICADV/` in `orig/`, named as on the disc, or put the disc's `.gdi` and its tracks there. Then:

```sh
make           # rebuild every binary and check it against your disc
make check     # check the linked sources without the disc
make dc        # build the port with GCC, in build/dc/
make dc-disc   # build a disc of the port, in build/dc/disc/
make dc-check  # build and sum the port's objects that need no disc
make pc        # build the PC game from your disc, in build/engine/
make play      # play the PC game in a window, from Emerald Coast's start
```

`make` compiles every binary's code, reads the data its sources declare from your disc, and checks each binary it rebuilds against the original, PRS compression and all. They land under `build/`, at the disc's own paths. `make` checks `1ST_READ.BIN`, `LOADER.BIN` and each file it reads from `SONICADV/` against [`SHA256SUMS`](SHA256SUMS) before reading it. This is a matching build, so an edit that changes a byte of output fails it.

`make check` needs no disc. It compiles the sources it can check without the disc, using placeholders for imported data where needed, and checks each linked source against its recorded hash, but builds no complete binary. [Checks](CONTRIBUTING.md#checks) says what each goal shows, and `make help` lists them all.

`make dc` builds the sources with GCC 16.2 and links every image anew, under `build/dc/` at the disc's paths. It needs the disc but neither SHC nor wibo. Its first run builds binutils and GCC from source ([Toolchain](PORTS.md#toolchain)); their logs are in `build/toolchain/`. `make dc-disc` needs the disc's `.gdi` and its tracks in `orig/`, not its files alone. [The port's features](#the-ports-features) describes what it adds.

`make dc-check` needs neither disc nor SHC. It builds the port's disc-independent objects and writes `build/dc/check.txt`: hashes of those objects, SH4ZAM's archive, the toolchain's `libgcc.a` and two compression results. Compare the file across hosts at the same commit, with the same `SH_ELF` ([Checks](CONTRIBUTING.md#checks)). Its first run builds the toolchain and fetches SH4ZAM, as `make dc` does.

`make pc` builds the game from your disc, and `make play` runs it in a window. The engine's build goals need Python 3.9 or later; a window needs SDL3 and Vulkan. [Checks](CONTRIBUTING.md#checks) lists the other host requirements, and [Build goals](ENGINE.md#build-goals) lists the engine's commands.

Run one `make` invocation at a time per checkout; `-j` within it is fine. SHC is a Windows program, and [wibo](https://github.com/decompals/wibo) runs it. SHC sometimes hangs under wibo, so `make` stops a tool after ten minutes. Run `make` again and it carries on. To use tools you already have, set `WIBO`, `R10`, `R26`, `R32` and `SH_ELF`: [Running SHC](TOOLCHAIN.md#running-shc) and [Toolchain](PORTS.md#toolchain) say what each must hold.

## The port's features

`make dc` adds 16:9, 60 fps, skips for movies and cutscenes, and a performance readout. The game decides when each may apply, and the engine draws them ([Dreamcast](ENGINE.md#dreamcast)).

| Press, on the first pad | Does |
|---|---|
| Hold L and R, then up | Steps the readout |
| Hold L and R, then right | Turns 16:9 on or off |
| Hold L and R, then down | Turns 60 fps on or off |
| Start, in a movie, or outside a demo in an event that holds control | Skips it |

The readout, 16:9 and 60 fps start off. The game saves the 16:9 and 60 fps settings, but not the readout. A chord's effect appears on screen for two seconds.

### The readout

Up steps through off, a page of frame times and a page of cache counters. On the PC it shows or hides one page ([The readout](ENGINE.md#the-readout)).

### Skips

Start skips any movie; the shipped game skips only the opening. During an event, a black curtain covers the scene while the script runs quickly, so its flags and placements still happen before control returns ([`skip.c`](src/event/skip.c)).

### 16:9

16:9 widens the picture by a third while the Dreamcast still draws at 640 by 480 pixels. Attract demos and recordings stay at 4:3, and the game's visibility tests use the wider view during play ([16:9](ENGINE.md#169)).

### 60 fps

60 fps can split a game frame into two pictures without doubling the game's steps. It applies in play and the fade into it, not in Twinkle Circuit, Sky Chase, the Chao gardens or race, pause, other fades or game over. The PC uses the same split frame ([60 fps](ENGINE.md#60-fps)).

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
engine/    the portable engine, Dreamcast and PC backends, game adapter and native runner
config/    image and motion tables, compiler and port settings, source boundaries and fixed-address exceptions
offsets/   disc data offsets for the GCC Dreamcast port
tools/     the build's tools: its own, vendored and downloaded
tests/     the tools' tests
```

## Contributing

Contributions are welcome. [CONTRIBUTING.md](CONTRIBUTING.md) says what helps, and follows one source from its placement table to the checked binary.

## Read on

- [TOOLCHAIN.md](TOOLCHAIN.md) documents the build, compiler releases, disc data and accounting.
- [PORTS.md](PORTS.md) documents the non-matching code and the Dreamcast port's compiler and link.
- [ENGINE.md](ENGINE.md) documents the portable engine, game adapter, and Dreamcast and PC backends.
- [TAGS.md](TAGS.md) defines the source annotations and their evidence requirements.
- [prskit](https://github.com/reshaped/prskit), [gdi](https://github.com/reshaped/gdi) and [shckit](https://github.com/reshaped/shckit) are companion tools for PRS compression, disc extraction, and SHC assembly analysis and patching.

## Acknowledgements

Special thanks to some amazing engineers:

- [wibo](https://github.com/decompals/wibo), by decompals, runs Hitachi's Windows toolchain on Linux. It makes the matching build possible.
- [decomp.me](https://decomp.me) maintains the [compiler archive](https://github.com/decompme/compilers) used by decompilation projects, including the three SHC releases this build downloads.
- [SH4ZAM](https://github.com/gyrovorbis/sh4zam) by Falco Girgis for hand-tuned SH-4 math functions used in the GCC Dreamcast port.
- [sh4_fpu_approx.h](https://gist.github.com/skmp/4bd7b38149fc455978b5104201573236) by Stefanos Kornilios Mitsis Poiitidis for bit exact SH-4 FPU math.
- [MAME](https://github.com/mamedev/mame)'s AICA emulation by ElSemi, Deunan Knute and R. Belmont for the PC port's sound tables.

## License

The Makefile, `dc.mk`, `config/compilers.mk`, `config/gcc.mk`, `config/dreamcast.mk` and the files committed to `engine/`, `tools/` and `tests/` are under the [MIT license](tools/LICENSE), except where a file carries its own notice. The rest of the repository has no license.

This project isn't affiliated with or endorsed by SEGA.
