# Dependencies: Ubuntu on WSL

This is what the project needs on Ubuntu under WSL2 for the matching build (`make`, `make check`), the GCC build (`make gcc`), the port's PC build (`make -f Makefile -f pc.mk …`) and its tests. It was tested on Ubuntu 26.04 LTS, with the versions listed at the end.

## WSL itself

- **WSL2**, with WSLg for the game's window. WSLg comes with current Windows 10 and 11. Check that `echo $DISPLAY $WAYLAND_DISPLAY` prints values (`:0 wayland-0`).
- **Disk space:** about 10 GB free for `build/`, the toolchains and the disc.

## 32-bit packages

The port's host is a 32-bit program, so it needs the i386 architecture enabled first:

```sh
sudo dpkg --add-architecture i386
sudo apt update
```

## Packages

```sh
sudo apt install -y \
  build-essential git make curl tar gzip xz-utils coreutils \
  python3 python3-venv \
  clang lld llvm \
  gcc-multilib g++-multilib libc6-dev-i386 \
  bison flex \
  qemu-user \
  xxd \
  libsdl2-dev:i386 libgl-dev:i386 libegl-dev:i386 libgl1-mesa-dri:i386 \
  libx11-dev:i386 libwayland-dev:i386 \
  libasound2-dev:i386 libpulse-dev:i386
```

What each group is for:

| Packages | Used by |
| --- | --- |
| `build-essential`, `make`, `git`, `coreutils` (`sha256sum`, `timeout`) | everything: the README's requirements |
| `curl`, `tar`, `gzip`, `xz-utils` | the first `make`, which downloads wibo and three SHC releases, and the first `make gcc`, which downloads binutils, GCC, GMP, MPFR and MPC. Each download is checked against `tools/SHA256SUMS`. |
| `python3`, `python3-venv` | every build tool in `tools/` and the tests; `make lint`/`make format` install clang-format and clang-tidy into a venv |
| `g++`, `bison`, `flex` (`build-essential` brings `g++`) | building GCC 16.2 for SH-4 in `tools/sh-elf` (`make gcc`) |
| `clang`, `lld`, `llvm` (version 21 or later) | make pc: `clang --target=i386-elf`, `ld.lld`, `llvm-nm`, `llvm-objdump`, `llvm-readelf`, `llvm-objcopy` |
| `gcc-multilib`, `g++-multilib`, `libc6-dev-i386` | compiling the 32-bit host with `cc -m32` |
| `libsdl2-dev:i386`, `libgl-dev:i386`, `libegl-dev:i386`, `libgl1-mesa-dri:i386`, `libx11-dev:i386`, `libwayland-dev:i386` | the host's window and OpenGL (Mesa's llvmpipe under WSLg) |
| `libasound2-dev:i386`, `libpulse-dev:i386` | SDL2's audio, for the port's sound |
| `qemu-user` (`qemu-sh4`) | `xbox/test/`, which runs SH-4 code against its x86 translations |
| `xxd` | inspecting binaries; optional |

On an Ubuntu release whose default clang is older than 21, install a versioned one (`clang-21 lld-21 llvm-21`, from the distribution or apt.llvm.org) and make sure `clang`, `ld.lld` and the `llvm-*` tools on `PATH` are that version.

## For stage 5, the Xbox

- **nxdk**, cloned with its submodules: `git clone --recursive https://github.com/XboxDev/nxdk`. It needs `make`, `clang`, `lld`, `git` and CMake. Where CMake can't be installed system-wide, a venv's will do: `python3 -m venv nxdk-venv && nxdk-venv/bin/pip install cmake`, with `nxdk-venv/bin` on `PATH`. The port patches its pbkit with `xbox/third_party/nxdk-pbkit-back-buffers.patch` (`pb_back_buffers`, two frame buffers in place of three) and `xbox/third_party/nxdk-pbkit-lost-swap.patch` (a frame pbkit retired without showing it): `xbox/host/xbox/run.sh` applies each to the checkout the first time, with `git apply`, and rebuilds pbkit's objects.
- **xemu**, to run the XBE, with the Xbox's BIOS, MCPX ROM and a hard disk image.

## What the build downloads itself

None of these need installing by hand:

- **wibo and Hitachi's SHC** (three releases, from decomp.me's compiler archive): downloaded by the first `make` into `tools/`. If `tools/wibo-i686` ends up not executable, run `chmod +x tools/wibo-i686`.
- **GCC 16.2.0 for SH-4** (`tools/sh-elf`): fetched and built by the first `make gcc`. This takes a while.
- **gdi, prskit and shckit:** vendored, and built into `build/` by make.
- **nxdk-audio** (MIT; its HRTF table, `audio_hrtf.h`, is the SONICOM dataset, CC BY 4.0): cloned from https://github.com/Ryzee119/nxdk-audio by `xbox/host/xbox/run.sh` into `~/refs/nxdk-audio` (`NXDK_AUDIO_DIR`), at revision `fc2deca2cc1e434805ac03ca7c2f500b3b028f36`, which it then patches with `xbox/third_party/nxdk-audio-loop-start.patch` (`nxAudioVoiceSetLoopStart`). Its DSP program, `passthrough.a56`, is assembled by `dsp56300-asm` 1.0.0 (`cargo install dsp56300-asm --version 1.0.0`; `DSP56300_ASM`, default `~/refs/dsp-tools/bin/dsp56300-asm`).
- **pl_mpeg** (MIT): already in `xbox/third_party/pl_mpeg.h`.
- **Flycast's AICA, ARM7 and x86 DSP recompiler** (GPL-2.0, from Flycast v2.7), with **Xbyak** (BSD-3): already in `xbox/third_party/flycast/`. The PC host builds them in, and so does the Xbox host's XBE (`xbox/host/xbox/Makefile`, with the interpreter DSP and without Xbyak).

## Not installable: the disc

`orig/` must hold your own copy of the disc: the `.gdi` file and its `track01.bin`, `track02.raw` and `track03.bin`. The matching build reads the files it extracts from it, and the port reads the full disc extracted into `build/disc/`. Copy `orig/` from the old machine, since it can't be downloaded.

## Checking the setup

```sh
clang --version | head -1          # 21.x or later
ld.lld --version
qemu-sh4 --version | head -1
echo 'int main(void){return 0;}' > /tmp/t.c && cc -m32 /tmp/t.c -o /tmp/t32 -lSDL2 -lGL && echo "32-bit SDL2/GL ok"
```

## Versions this was tested with

| Software | Version |
| --- | --- |
| Ubuntu | 26.04.1 LTS (WSL2, WSLg) |
| clang, lld, LLVM | 21.1.8 |
| GCC (host) | 15.2.0 |
| Python | 3.14 |
| GNU make | 4.4.1 |
| git | 2.53 |
| qemu-sh4 | 10.2.1 |
| SDL2 (i386) | 2.32.10 |
| Mesa (i386) | 26.0.8 |
