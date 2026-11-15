# Xbox

The port of this byte-matching decompilation of *Sonic Adventure* (JP, HDR-0001, v1.007) to the original Xbox with [nxdk](https://github.com/XboxDev/nxdk), by way of a native build. `dependencies.md` lists the software to install.

- [Goal and limits](#goal-and-limits)
- [What has been implemented](#what-has-been-implemented)
- [Architecture](#architecture)
- [Target resolution](#target-resolution)
- [New features](#new-features)
- [Known issues](#known-issues)
- [Debug flags](#debug-flags)
- [Experimental flags](#experimental-flags)
- [Building](#building)
- [Future plans](#future-plans)
- [How the port was made](#how-the-port-was-made)

## Goal and limits

- **Goal:** the game running natively on a real Xbox, looking and sounding as close to the Dreamcast as possible, while being Xbox-native in how it gets there: no emulation of the Dreamcast's internals where a native design does the job (a GPU renderer instead of the tile accelerator's lists, x87 and SSE math instead of an emulated XMTRX, the APU's voices instead of a software mixer).
- **The game's own code:** the decompilation's C, built for x86, with its math made more native to and compatible with the Xbox: C over the port's own models of the SH-4's FPU instructions (`xbox/include/sh4.h`). No SH-4 code runs on the Xbox.
- **Out of scope:** the VMU selection screen, the internet menu and mail, the hand-off to Dream Passport. Their code compiles and stays unreachable, but are not priorities for this port for now.
- **The matching build is untouched:** `make` and `make check` read nothing the port adds. Every edit to a shared source is under `XBOX`, or `GCC` and `XBOX`.
- **License:** the port links Flycast's GPL-2.0 AICA and ARM7 code, so the port is GPL-2.0. The console's folder, the ISO and the packs hold the game's code and data: for the builder's own console only.

Checks that must keep passing:

- `make check`: "4309 files match SHA256SUMS".
- `xbox/test/all.sh`: 22 SH-4 translation tests, 0 real differences (a zero's sign and a NaN's bits don't count).
- `xbox/test/sh4_sse.sh`: the x87 FIPR and FTRV against the C they replaced, 0 differences.
- Docs: `git add -N XBOX.md && PYTHONPATH=tools:tests python3 -m unittest tests.test_docs; git reset -q XBOX.md`

## What has been implemented

- **The whole game builds and runs:** all 253 images (1ST_READ.BIN and its overlays) linked for i386 at their console addresses and loaded as on the console. Played on a real Xbox: logo, title, menus, Sonic's story, Emerald Coast, Speed Highway, Red Mountain and other stages; saves go to a memory card image on the hard disk.
- **The port layer** (`xbox/port/`) stands in for the SDK's hardware layers: interrupts and timers, disc files, pads, fibers, Kamui's lists and video memory, the renderer, movies, math, the overlay and the reports.
- **Two hosts:** a native PC program (SDL2 and OpenGL, `xbox/host/pc/`) for debugging and headless runs, and the Xbox XBE (nxdk and pbkit, `xbox/host/xbox/`).
- **Graphics on the NV2A:** the PowerVR's lists drawn through pbkit with a vertex program and ten combiner programs; per-pixel table fog; two-sample antialiasing; a subtle bloom in stages; textures converted ahead of time to the NV2A's formats (DXT1/DXT3, swizzled 16-bit for 2D art), with mipmaps where the Dreamcast has them.
- **Models on the NV2A:** a model that needs no clipping is transformed by the NV2A from vertices the drawer writes into an arena (2 × 26,768 vertices). Models that haven't changed are kept in a store and neither shaded nor written again; kept models with normals are lit by the NV2A through a palette texture. A meshset that doesn't fit the arena goes through the lists whole.
- **Translucency:** the translucent list sorted back to front per triangle; alpha-blended strips draw with a separate depth pass; additive strips draw last.
- **Sound:** the disc's own sound driver (masaru.drv) runs, translated ahead of time from ARM to native code, over Flycast's AICA registers and timers. On the Xbox the channels play on the MCPX APU's hardware voices, from samples converted ahead of time into a sound pack. The music's ADX streams are decoded by the game's own ADX library.
- **Movies:** Sofdec's player replaced by pl_mpeg, with the ADX audio as the clock (off by default on the Xbox, for now).
- **Packaging:** `make xbox` updates the console's folder in place: eight files, of which a build changes one or two. `make xbox-disc` creates an ISO, but this is untested at the moment.
- **SH-4 math, original behavior:** Ninja's math bodies, the game's assembly (`math.src`, `vector.src`, `mesh.src` and the rest) and SHC's inline bodies are C in `xbox/`, each checked bit for bit against the original assembly under `qemu-sh4` (`xbox/test/`).

## Architecture

- **Images:** each image is linked for i386 at its console address by `tools/pclink.py` from make gcc's link spec. Data keeps its RAM address; code goes to code areas past `0x10000000`. An overlay's unbound name binds to 1ST_READ.BIN's, the port's XMTRX among them (`sh4_xmtrx`, one for the game, as the console's one register).
- **Memory:** the port's RAM is area 3, the console's 16 MB then the port layer's own memory, visible at `0x0c000000`, `0x8c000000`, `0xac000000` and the store queues' `0xe0000000`.
- **`struct host_api`** (`xbox/port/host.h`) is the line between the shared port and the two hosts: files, time, the blank, pads, cards, video, AICA registers, stats.
- **Floats:** SSE single precision in the SH-4's mode (rounding toward zero, flush to zero, denormals as zero), set by each host before `main`; the x87 at 53 bits and rounding toward zero for FIPR's and FTRV's double sums, as Flycast computes them. FSRRA is exact here; FSCA uses the console's own table.
- **`xbox/include/sh4.h`:** the port's XMTRX and its models of FIPR, FTRV, FSRRA, FSCA, FTRC and FMAC. `ninja.h` includes it under `XBOX` in place of `ninja_zam.h`. FIPR and FTRV are x87 asm in `xbox/include/sse/sh4_sse.h`, bit-identical to the C they replaced.
- **The blank** interrupts the game's thread through the port's interrupt entry, as the console's does; the game's speed follows the SH-4's timer, which the port models.

## Target resolution

- **640×480**, 4:3, the Dreamcast's own resolution, with **two-sample antialiasing** (a 1280×480 surface resolved to the screen), double-buffered with vsync, at the game's 30 frames a second.
- **16:9:** the port can squeeze the picture for a widescreen TV (`port_toggles[TOGGLE_WIDE]`, flipped from the overlay).
- **720p** (`HOST_720P=1`, where the dashboard allows it): the game's 640×480 scene scaled to 1280×720, with 16:9 on and no antialiasing (experimental).

## New features

Features the Dreamcast version doesn't have:

- **Right stick camera:** left and right turn the camera as the triggers do; up and down raise and lower it around a point above the player, easing back when released.
- **Analog dead zone** on the left stick (12% by default, `HOST_DEADZONE=PERCENT`), as the Xbox joystick tends to drift more than the DC's.
- **Bloom** in stages and fields (`HOST_BLOOM`, `HOST_BLOOM_THRESHOLD`).
- **On-screen performance overlay** (`HOST_OVERLAY`, on by default) and **reports** on the pad's Back button (see Debug flags).
- **Return to the dashboard:** both triggers with Start and Back.

## Known issues

- **Scenery drawn wrong for a frame:** some scenery can flicker or appear in the wrong place for a single frame. This has been noted, and a fix is being worked on.
- **Sound:** the sound engine is an early work in progress: it may play incorrectly (crackling in the music, for one), and DSP features such as reverb are missing.
- **Movies** are off by default: untested on the console.
- **The translucent sort** is per triangle, where the PowerVR's is per pixel: crossing triangles can sort wrong.
- **make gcc's census** fails on `src/progress/movie.c`'s `XBOX` address `0x8ceb0000`; the rest of make gcc builds.

## Debug flags

Settings are `KEY=VALUE` lines in `host.txt` on the Xbox (`E:\SONICADV\host.txt`), or environment variables for the PC host. The console's `host.txt` defaults to `HOST_PROFILE=0`, `HOST_LOG_FILE=0`, `HOST_REPORT=1`, `HOST_CRASH_LOG=1`. The log is `E:\SONICADV\log.txt`, the reports `E:\SONICADV\reportNN.txt`.

Logging and reports:

| Setting | Effect |
| --- | --- |
| `HOST_REPORT=0` | the pad's Back no longer writes a report (on by default) |
| `HOST_REPORT_AT=FRAME` | writes a report at that frame, as Back does |
| `HOST_CRASH_LOG=0` | no log written at a fault, halt or stall (on by default) |
| `HOST_LOG_FILE=1` | writes the log every 10 seconds |
| `HOST_PROFILE=1` | the profile every 600 blanks: frame rate, sampled functions, counts, the sound's cost |
| `HOST_SOUND_PROFILE=1`, `HOST_SOUND_LOG=1`, `HOST_AICA_READS=1` | the sound's cost, its state once a second, the ARM7's register reads |
| `HOST_WATCH=SYMBOL` | logs a 1ST_READ.BIN variable when it changes |
| `HOST_BREAK=SYMBOL[+OFFSET]` | a hardware write watch on a variable (Xbox) |
| `HOST_WHERE=1` | prints where the game is every five seconds (PC) |

Drawing checks:

| Setting | Effect |
| --- | --- |
| `HOST_VERTEX_CHECK=1` | checks the models' vertices against what the NV2A reads, and kept models against their data (slow) |
| `HOST_VIDEO_DUMP=FRAME`, `HOST_VIDEO_DUMP_LINES=N`, `HOST_VIDEO_DUMP_LIT=1` | logs the draws' state from that frame (Xbox) |
| `HOST_VIDEO_DUMP_REPORT=1` | each report also dumps the next frame's draws (Xbox) |
| `HOST_DUMP=1` / `2` | with `HOST_SHOT`, logs the next frame's strips / and vertices (PC) |
| `HOST_GPU_MODELS=N` | which models the NV2A takes: bit 0 opaque, 1 translucent, 2 kept, 3 the box test, 4 lit by the NV2A (31 by default; 0 sends every model through the lists) |
| `HOST_ARENA=N` | the models' arena in vertices, to test the drawer past its room (PC) |
| `HOST_FOG_TABLE=0`, `HOST_DEPTH_BOTTOM=0`, `HOST_GUARD=N`, `HOST_CULL_FLIP=1` | the fog table, the depth's bottom piece, the guard band, the culled face (for tests) |
| `HOST_NO_PACK=1`, `HOST_PACK=PATH`, `HOST_TEXTURE_KEYS=1` | textures without the pack, the pack to use (PC), the keys asked for |

Scripting and capture:

| Setting | Effect |
| --- | --- |
| `HOST_CLOCK=frames` | pads, shots, pokes and quit count rendered frames, not blanks |
| `HOST_PADS=600:8,610:0,2400:0/128/0` | scripts controller A: `PDD_DGT_` bits in hex, optional `/X/Y` stick |
| `HOST_POKE=300:menu_state+0:1:1` | writes a 1ST_READ.BIN variable at a blank (this one opens every stage); a report's `port_place` line restarts at its place |
| `HOST_SHOT=F,...` | saves those frames (`build/pc/shot_N.ppm` on the PC, PNGs from xemu) |
| `HOST_SHOT_BURST=N` | the first Back also shoots the N frames after it (Xbox) |
| `HOST_QUIT=N` | quits at blank or frame N |
| `HOST_HEADLESS=1`, `HOST_SCALE=2`, `HOST_SLOW=3` | no window, a doubled window, console time three times slower (PC) |
| `HOST_SAVES=DIR`, `HOST_CARD_DELAY=US` | where the card files go, a delay per card call (PC) |
| `HOST_WAV=PATH`, `HOST_SOUND_RECORD=PATH` | the sound to a WAV, or the game's side of the AICA recorded for `sndreplay` (PC) |
| `HOST_AUDIO_TEST=1`, `HOST_VIDEO_TEST=1` | the hosts' audio and video tests |
| `HOST_DEBUG_PAGE=N` | the game's own debug page (1 collisions and particles, 3 sound slots) |
| `HOST_SILENT=1`, `HOST_SOUND_FREEZE_AT=N`, `HOST_SOUND_NO_IDLE=1`, `HOST_DSP_CHECK=SECOND`, `HOST_SOUND_PACK=0` | sound tests: no AICA (hangs the game), freeze the sound, run the driver's idle loop, check the DSP, no sound pack |

Pokes of note: `port_movie_skip+0:1`, `port_overlay+0:1`, `port_profile+0:1`, `port_report+0:1`, `port_gpu_models+0:N`, `port_sweep_cap+0:1` (widen far collision sweeps), `port_sort_triangles+0:N` (the translucent sort: 0 by strip, 1 by a triangle's nearest vertex, 2 by its middle).

The watch (on whenever reports are): logs frames in play that draw under four fifths of the strips or models of both frames beside them, with the models missing; and dumps player 0's last 32 frames and makes a report when its place or speed leaves all reason.

## Experimental flags

| Setting | Effect |
| --- | --- |
| `HOST_60FPS=1` | 60 frames a second: each stage frame split into two pictures |
| `HOST_720P=1`, `HOST_720P_COLOR=16` | 1280×720 output, 16:9, no antialiasing; 16-bit color |
| `HOST_MOVIES=1` | plays the movies (needs `movies.pak`) |
| `HOST_MSAA=0` | no antialiasing |
| `HOST_TRIPLE_BUFFER=1` | three frame buffers instead of two |
| `HOST_SYNC_PRESENT=1` | the old waiting present (uneven on the console) |
| `HOST_VSYNC=0`, `HOST_BLANK_LOCK=0` | doesn't wait for the blank; the clock's blanks instead of the display's |
| `HOST_BLOOM=PERCENT`, `HOST_BLOOM_THRESHOLD=PERCENT` | the bloom's strength (40, 0 for none) and threshold (75) |
| `HOST_DEADZONE=PERCENT` | the left stick's dead zone (12) |
| `HOST_OVERLAY=0` | no on-screen figures |

## Building

The build runs on Linux; on Windows it runs under WSL2. Install what `dependencies.md` lists (32-bit SDL2 and Mesa, clang and lld 21 or later, qemu-user, Python 3) and clone nxdk with its submodules to `~/refs/nxdk`. Your own disc goes in `orig/` (the `.gdi` and its tracks): it can't be downloaded. Use `-j8` at most: more runs the machine out of memory.

### Linux

```sh
make check              # downloads wibo and SHC, builds the matching images
make gcc                # GCC 16.2 for SH-4 (long, first time), make gcc's images and link spec
build/gdi extract "orig/sonic.gdi" build/disc
make pc pc-link pc-disc pc-host   # the x86 images and the PC host
make pc-run                 # the game on the PC
make xbox               # the Xbox host and the console's folder, build/xbox/iso/
make xbox-disc          # and build/xbox/sonic.iso from it
```

- Expected: `pc` reports `compiled 3353 of 3356 objects` (two `kmsurfac` sources and `syCache`, which the port replaces); `pc-link` reports `253 images linked; 0 over their areas`.
- The Xbox host builds with nxdk at `~/refs/nxdk` (`NXDK_DIR`). `xbox/host/xbox/run.sh` applies the port's two pbkit patches (`xbox/third_party/nxdk-pbkit-back-buffers.patch` and `xbox/third_party/nxdk-pbkit-lost-swap.patch`) to that checkout once. It clones nxdk-audio from https://github.com/Ryzee119/nxdk-audio to `~/refs/nxdk-audio` (`NXDK_AUDIO_DIR`) at the port's revision and applies `xbox/third_party/nxdk-audio-loop-start.patch` once.
- On native Linux, run `build/xbox/sonic.iso` in a Linux build of xemu by hand: `run.sh`'s automated xemu runs drive the Windows xemu through PowerShell.
- The PC host's keyboard: arrows the stick; I, J, K, L the d-pad; Z, X, A, S are A, B, X, Y; Q, W the triggers; Enter Start; Esc quits.

### Windows (WSL2)

1. Install WSL2 (WSLg gives the PC host its window), then the packages of `dependencies.md` inside it.
2. Work in the Linux file system (for example `~/sonic-adventure`), not under `/mnt/c`: the build is far slower on Windows' drives. Move a checkout with `tar`, which keeps executable bits.
3. Build as under Linux above.
4. **xemu runs:** put xemu for Windows in a folder of your own and set `XEMU_DIR` to its WSL path (for example `export XEMU_DIR=/mnt/c/Users/<you>/Tools/xemu`), with its BIOS, MCPX ROM and hard disk image. `xbox/host/xbox/run.sh SECONDS [KEY=VALUE...]` builds the host, packs an ISO, runs xemu, then prints the log and converts the shots to `build/xbox/shot_fN.png`. One run at a time.
5. **A real Xbox:** copy `build/xbox/iso/` to `E:\SONICADV\` on a softmodded console (FTP the files newer than the console's). `default.xbe` and `modules.pak` are what a build changes.

## Future plans

- **A renderer without the tile accelerator's lists:** runs handed straight to the host instead of written into Kamui's lists and read back.
- **More work on the NV2A:** clipping by the NV2A instead of the drawer.
- **Performance:** a profiled console run after the x87 FIPR and FTRV; packed SSE where the original's operations are separate floats; the push buffer (937 of 1,024 KB in Speed Highway's third act).
- **Sound:** the sound engine is only in its early stages and needs more work.
- **Memory:** account for the free pages; give the textures' pool and the arenas more room.
- **Movies:** their room, and a run on the console.
- **Tooling:** a set of frames compared on the PC by one command; the sound compared with Flycast again; an FTP push to the console.

## How the port was made

The port started from the GCC port, not the matching build: [GCC.md](GCC.md) already replaces three `.src` files with C and keeps SHC's results under guards. The port defines `XBOX` and compiles with `GCC` defined too.

- **Stage 0, the PC build:** `make pc` compiles every C source for i386 with Clang, SSE math, the float pass and the guards.
- **Stage 1, no SH-4 code:** every game `.src` file but the movie player's became C in `xbox/`, each checked by `xbox/test/run.sh` under `qemu-sh4` against the original assembly. Ninja's inline bodies became C by `tools/sh4lift.py` and `tools/sh2c.py`.
- **Stage 2, the images on x86:** each image linked at its area and loaded as a file; the host and the port layer.
- **Stage 3, graphics:** Ninja's interface kept, Kamui's lists drawn by the port's renderer; movies through pl_mpeg.
- **Stage 4, sound:** the AICA emulated and the disc's driver run on it.
- **Stage 5, the Xbox:** the nxdk host, pbkit in place of OpenGL, the APU in place of the mixer, the packs, then the console work: pacing and vsync, the kept and lit models, fog, bloom, the translucent sort, the sound pack, the math made more native to the Xbox, the pbkit lost-swap fix, and the bigger models' arena.
