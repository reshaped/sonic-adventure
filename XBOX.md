# Xbox

The port of this byte-matching decompilation of *Sonic Adventure* (JP, HDR-0001, v1.007) to the original Xbox with [nxdk](https://github.com/XboxDev/nxdk), by way of a native build. `dependencies.md` lists the software to install.

- [Goal and limits](#goal-and-limits)
- [What has been implemented](#what-has-been-implemented)
- [Architecture](#architecture)
- [Target resolution](#target-resolution)
- [New features](#new-features)
- [Known issues](#known-issues)
- [Fixed bugs](#fixed-bugs)
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
- **Models on the NV2A:** a model that needs no clipping is transformed by the NV2A from vertices the drawer writes into an arena (2 × 29,768 vertices, in a room of their own beside the lists' vertex ring). Models that haven't changed are kept in a store and neither shaded nor written again; kept models with normals are lit by the NV2A through a palette texture. A meshset that doesn't fit the arena goes through the lists whole.
- **Translucency:** the translucent list sorted back to front per triangle; alpha-blended strips draw with a separate depth pass; additive strips draw last.
- **Sound:** the disc's own sound driver (masaru.drv) runs, translated ahead of time from ARM to native code, over Flycast's AICA registers and timers. On the Xbox the channels play on the MCPX APU's hardware voices, from samples converted ahead of time into a sound pack. The music's ADX streams are decoded by the game's own ADX library.
- **Movies:** Sofdec's player replaced by pl_mpeg, with the ADX audio as the clock (off by default on the Xbox, for now).
- **Packaging:** `make xbox` updates the console's folder in place: eight files, of which a build changes one or two. `make xbox-disc` creates an ISO, but this is untested at the moment.
- **SH-4 math, original behavior:** Ninja's math bodies, the game's assembly (`math.src`, `vector.src`, `mesh.src` and the rest) and SHC's inline bodies are C in `xbox/`, each checked bit for bit against the original assembly under `qemu-sh4` (`xbox/test/`).

## Architecture

- **Images:** each image is linked for i386 at its console address by `tools/pclink.py` from make dc's link spec. Data keeps its RAM address; code goes to code areas past `0x10000000`. An overlay's unbound name binds to 1ST_READ.BIN's, the port's XMTRX among them (`sh4_xmtrx`, one for the game, as the console's one register).
- **Memory:** the port's RAM is area 3, the console's 16 MB then the port layer's own memory, visible at `0x0c000000`, `0x8c000000`, `0xac000000` and the store queues' `0xe0000000`.
- **`struct host_api`** (`xbox/port/host.h`) is the line between the shared port and the two hosts: files, time, the blank, pads, cards, video, AICA registers, stats.
- **Floats:** SSE single precision in the SH-4's mode (rounding toward zero, flush to zero, denormals as zero), set by each host before `main`; the x87 at 53 bits and rounding toward zero for FIPR's and FTRV's double sums, as Flycast computes them. FSRRA is exact here; FSCA uses the console's own table.
- **`xbox/include/sh4.h`:** the port's XMTRX and its models of FIPR, FTRV, FSRRA, FSCA, FTRC and FMAC. `ninja.h` includes it under `XBOX` in place of `ninja_zam.h`. FIPR and FTRV are x87 asm in `xbox/include/sse/sh4_sse.h`, bit-identical to the C they replaced.
- **The blank** interrupts the game's thread through the port's interrupt entry, as the console's does; the game's speed follows the SH-4's timer, which the port models. On the Xbox the host breaks in from the kernel's clock interrupt, wherever the game is; its trampoline saves the x87/SSE state (FXSAVE) before its own chores, whose sound emulation computes in those registers. Without that, the game resumed on the chores' values, and objects and land pieces dropped out for a frame on the console alone. Host code at interrupt level must not touch x87/SSE outside that save.

## Target resolution

- **640×480**, 4:3, the Dreamcast's own resolution, with **two-sample antialiasing** (a 1280×480 surface resolved to the screen), triple-buffered with vsync, at the game's 30 frames a second.
- **16:9:** the port can squeeze the picture for a widescreen TV (`port_toggles[TOGGLE_WIDE]`, flipped from the overlay).
- **720p** (`HOST_720P=1`, where the dashboard allows it): the game's 640×480 scene scaled to 1280×720 on a 1280×720 surface copied to the screen, with 16:9 on and no antialiasing (experimental).

## New features

Features the Dreamcast version doesn't have:

- **Right stick camera:** left and right turn the camera as the triggers do, past a 20% dead zone; up and down do nothing.
- **Analog dead zone** on the left stick (12% by default, `HOST_DEADZONE=PERCENT`), as the Xbox joystick tends to drift more than the DC's.
- **Bloom** in stages and fields (`HOST_BLOOM`, `HOST_BLOOM_THRESHOLD`).
- **On-screen performance overlay** (`HOST_OVERLAY`, on by default) and **reports** on the pad's Back button (see Debug flags).
- **Return to the dashboard:** both triggers with Start and Back.

## Known issues

- **Land at the draw radius:** a land piece right at the edge of the stage's draw radius can blink for a frame while the camera hovers on that edge, as on the Dreamcast (a mountain's piece on Emerald Coast's last cliff, with the camera pitched down). It is the game's own land selection, left as the console has it.
- **Movies** are off by default: untested on the console.
- **The translucent sort** is per triangle, where the PowerVR's is per pixel: crossing triangles can sort wrong.

## Fixed bugs

- **Objects and land vanishing for a frame (console only).** Springs, palms, rocks, rings, item boxes, the land's pieces and the sea dropped out for a single frame, mid-screen, with their steps run; on Emerald Coast up to 23 objects at once, or a piece of the sea drawn black. The PC host never showed it. The cause was the blank's trampoline on the Xbox host (`xbox/host/xbox/host.c`): it ran the host's own chores, the sound's ARM7 and AICA emulation above all, before the port's interrupt entry saved the x87/SSE state, so the chores' float work overwrote whatever the game had in its XMM and x87 registers wherever the blank broke in, and the game resumed on those values (a view test or a matrix wrong for a frame). The PC was spared because Linux saves the FPU state for a signal handler. The trampoline now saves the state with FXSAVE before the chores, runs them with an empty x87 stack and the game's control words, and restores it after. Confirmed on a console at 60 frames a second, with antialiasing off.
- **The split frame's pacing at 60 frames a second.** Kamui's statistics left the render's time at 0, so `src/system/present.c`'s split judged frames to fit that ran late, and the split ended and engaged by turns. `xbox/port/kamui.c` now gives the render's length, the lists read and handed to the host, as the console's Kamui gives the tile accelerator's intake.
- **The translucent sort's reads of write-combined memory.** The sort by triangle and the strips' checks of their vertices read the runs' vertices back from the host's write-combined memory, which the CPU reads uncached. The drawer now keeps the depths and the vertices' validity in cached memory as it writes them (`xbox/engine/sa1/emit.h`, `port_run_depths`).
- **Memory.** Kamui's lists take 512 KB instead of 4.1 MB, the arena of models has a room of its own beside the lists' vertex ring, the push buffer is 2 MB, and pbkit's own three frame buffers are used, so the back-buffer patch to nxdk is gone.

Changes made with these fixes:

- **The right stick's up and down** no longer raise and lower the camera: they do nothing, as the console's pad has no such stick. Left and right still turn it, past a 20% dead zone.
- **The sky** is drawn last of the opaque list, and profiled apart (`port_sky_task`).
- **720p** always uses 16-bit color; where the dashboard refuses 720p, the picture is 480p at 32 bits.
- **The draw distance setting** (`HOST_DRAW_DISTANCE`) is gone: a distance other than the console's broke some levels. The land, the far clip and the range objects are made at are the console's again.
- **Test switches removed** once the bugs above were found: the blank's holds (`HOST_BLANK_DEFER`, `HOST_BLANK_PARTS`), the flash and land detector (`HOST_FLASH_WATCH`, which freed about 18 KB of `1ST_READ.BIN`'s data), the old waiting present (`HOST_SYNC_PRESENT`), the vertex check, the draw dumps, the hardware write watches, the audio and video tests, the sound tests and probes, and the renderer's and present's on/off switches (`HOST_KEEP`, `HOST_SORT`, `HOST_GPU_MODELS`, `HOST_SKY_LAST`, `HOST_TILES`, `HOST_FOG_TABLE`, `HOST_DEPTH_BOTTOM`, `HOST_GUARD`, `HOST_CULL_FLIP`, `HOST_DEPTH_FIXED`, `HOST_TEXTURE_RECHECK`, `HOST_TEXTURE_POOL`, `HOST_IDLE_PRESENT`, `HOST_PRESENT_WAIT`, `HOST_BLANK_LOCK`, among others). What they turned on or off is now fixed at its default. The renderer's model paths and the translucent sort's mode can still be set with `HOST_POKE` (`port_gpu_models`, `port_sort_triangles`).

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
| `HOST_GPU_PROFILE=1` | the NV2A's time for each part of a picture (clear, movie, each list's draws of list vertices and of models, with the translucent depth passes in the translucent's, resolve, bloom), each part drawn alone, an empty wait's cost taken off each wait, in the log every 10 s and in reports; the frame rate falls, and no slow host calls are logged (Xbox) |
| `HOST_GPU_PROFILE=2` | as 1, and one picture a second timed draw by draw: the 16 costliest draws in the report (Xbox) |
| `HOST_WATCH=SYMBOL` | logs a 1ST_READ.BIN variable when it changes |

Scripting and capture:

| Setting | Effect |
| --- | --- |
| `HOST_CLOCK=frames` | pads, shots, pokes and quit count rendered frames, not blanks |
| `HOST_PADS=600:8,610:0,2400:0/128/0` | scripts controller A: `PDD_DGT_` bits in hex, optional `/X/Y` stick |
| `HOST_POKE=300:menu_state+0:1:1` | writes a 1ST_READ.BIN variable at a blank (this one opens every stage); a report's `port_place` line restarts at its place |
| `HOST_SHOT=F,...` | saves those frames (`build/pc/shot_N.ppm` on the PC, PNGs from xemu) |
| `HOST_QUIT=N` | quits at blank or frame N |
| `HOST_HEADLESS=1`, `HOST_SCALE=2`, `HOST_SLOW=3` | no window, a doubled window, console time three times slower (PC) |
| `HOST_SAVES=DIR` | where the card files go (PC) |
| `HOST_PACK=PATH` | the texture pack to use (PC) |
| `HOST_WAV=PATH`, `HOST_SOUND_RECORD=PATH` | the sound to a WAV, or the game's side of the AICA recorded for `sndreplay` (PC) |

Pokes of note: `port_movie_skip+0:1`, `port_overlay+0:1`, `port_profile+0:1`, `port_report+0:1`, `port_gpu_models+0:N`, `port_sweep_cap+0:1` (widen far collision sweeps), `port_sort_triangles+0:N` (the translucent sort: 0 by strip, 1 by a triangle's nearest vertex, 2 by its middle).

The watch (on whenever reports are): logs frames in play that draw under four fifths of the strips or models of both frames beside them, with the models missing; and dumps player 0's last 32 frames and makes a report when its place or speed leaves all reason.

## Experimental flags

| Setting | Effect |
| --- | --- |
| `HOST_60FPS=1` | 60 frames a second: each stage frame split into two pictures |
| `HOST_720P=1` | 1280×720 output, 16:9, no antialiasing, 16-bit color |
| `HOST_MOVIES=1` | plays the movies (needs `movies.pak`) |
| `HOST_MSAA=0` | no antialiasing: the scene on a 640×480 surface, copied to the screen as the antialiased one is resolved |
| `HOST_VSYNC=0` | doesn't wait for the blank: the clock's blanks instead of the display's |
| `HOST_BLOOM=PERCENT`, `HOST_BLOOM_THRESHOLD=PERCENT` | the bloom's strength (40, 0 for none) and threshold (75) |
| `HOST_DEADZONE=PERCENT` | the left stick's dead zone (12) |
| `HOST_OVERLAY=0` | no on-screen figures |

## Building

The build runs on Linux; on Windows it runs under WSL2. Install what `dependencies.md` lists (32-bit SDL2 and Mesa, clang and lld 21 or later, qemu-user, Python 3) and clone nxdk with its submodules to `~/refs/nxdk`. Your own disc goes in `orig/` (the `.gdi` and its tracks): it can't be downloaded. Use `-j8` at most: more runs the machine out of memory.

### Linux

```sh
make check              # downloads wibo and SHC, builds the matching images
make dc                 # GCC 16.2 for SH-4 (long, first time), the Dreamcast port's images and link spec
build/gdi extract "orig/sonic.gdi" build/disc
make xbox-pc-test xbox-pc-test-link xbox-pc-test-host   # the x86 images and the PC test host
make xbox-pc-test-run       # the game on the PC test host
make xbox               # the Xbox host and the console's folder, build/xbox/iso/
make xbox-disc          # and build/xbox/sonic.iso from it
```

- Expected: `xbox-pc-test` reports `compiled 3355 of 3357 objects` (the loader's `kmsurfac` workaround and `syCache`, which the port replaces); `xbox-pc-test-link` reports `253 images linked; 0 over their areas`.
- The Xbox host builds with nxdk at `~/refs/nxdk` (`NXDK_DIR`). `xbox/host/xbox/run.sh` applies the port's pbkit patch, `xbox/third_party/nxdk-pbkit-lost-swap.patch`, to that checkout once. It clones nxdk-audio from https://github.com/Ryzee119/nxdk-audio to `~/refs/nxdk-audio` (`NXDK_AUDIO_DIR`) at the port's revision and applies `xbox/third_party/nxdk-audio-loop-start.patch` once.
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
- **Performance:** a profiled console run after the x87 FIPR and FTRV; packed SSE where the original's operations are separate floats; the push buffer, now 2 MB (937 KB of the old 1 MB in Speed Highway's third act).
- **Memory:** account for the free pages; give the textures' pool and the arenas more room.
- **Movies:** their room, and a run on the console.
- **Tooling:** a set of frames compared on the PC by one command; the sound compared with Flycast again; an FTP push to the console.

## Upstream

The tree is upstream's 6e5a40e (reshaped/sonic-adventure), merged on 2026-11-20 into the port's first commit, deb4c2a, which forked from 25a3d31. The port follows upstream's Dreamcast port, `make dc`, not its native PC game: its C, guards and composition (`config/dreamcast.mk`), compiled for x86 under `NONMATCHING` and `XBOX`, never `DREAMCAST`. To take a newer upstream, apply `git diff 6e5a40e NEW` to the tree, then check `make check`, `make dc`, `make xbox-pc-test xbox-pc-test-link` and an xemu run.

- **Shipped sides:** `polygon.c`'s console rewrites (`engine/tools/marks.py`'s `CONSOLE`, by `tools/pcconsole.py`) take their shipped side, as upstream's native game does.
- **The drawer:** every one of upstream's live draws (`sa_present_live`'s formats) goes to the renderer as a run, the host's own vertices, rather than as the tile accelerator's in Kamui's lists; the lists keep each draw's header and the run's mark, in the draws' order (`xbox/engine/sa1/emit.h`). Models draw through upstream's layer, `src/draw/model.c`, into the port's walk, `xbox/engine/sa1/model.c`, the old make gcc drawer with its GPU path, which projects and clips on the CPU what the NV2A can't take whole. Sprites, screen polygons, 2D polygons and lines, 3D lines and easy models run upstream's sends unchanged, through thin wrappers in `xbox/engine/sa1/` and `xbox/engine/sa1/strip.h`, with small `XBOX` branches in `engine/dc/sa1/sprite.h`, `engine/dc/sa1/easy.h` and `engine/dc/njsprite.h`; the easy models' kernel is `xbox/engine/batch.h`'s C. Ninja's print is one run a print (`sdk/ninja/njDebug.c`). `xbox/engine/` comes before `engine/dc/` on the include path, so a header there takes the place of the Dreamcast's. Still in the lists: Ninja's own draws outside those formats, as `njDrawTriangle3D` and `njDrawPolygon3D`, a few dozen vertices a frame in the demos.
- **SH4ZAM:** the Dreamcast engine's sprites and screen polygons run SH4ZAM's software back end, on the port's XMTRX (`xbox/port/xmtrx.c`), with `xbox/include/zam_xbox.h` in place of `ninja_zam.h`'s helpers.
- **Left out:** `engine/dc/owner.c` (the port's Kamui owns the frame) and `engine/dc/crash.c` (`port_stop` logs and exits).
- **Not yet re-applied:** the port's split-frame view fix (a far sai, the amenbo's start step) on upstream's reworked `present.c`; re-apply it if those return.

## How the port was made

The port started from the GCC port, now upstream's Dreamcast port, not the matching build: [PORTS.md](PORTS.md) replaces `.src` files with C and keeps SHC's results under guards. The port defines `XBOX` and compiles with `NONMATCHING` defined too, never `DREAMCAST`.

- **Stage 0, the PC build:** `make xbox-pc-test` (then `make pc`) compiles every C source for i386 with Clang, SSE math, the float pass and the guards.
- **Stage 1, no SH-4 code:** every game `.src` file but the movie player's became C in `xbox/`, each checked by `xbox/test/run.sh` under `qemu-sh4` against the original assembly. Ninja's inline bodies became C by `tools/sh4lift.py` and `tools/sh2c.py`.
- **Stage 2, the images on x86:** each image linked at its area and loaded as a file; the host and the port layer.
- **Stage 3, graphics:** Ninja's interface kept, Kamui's lists drawn by the port's renderer; movies through pl_mpeg.
- **Stage 4, sound:** the AICA emulated and the disc's driver run on it.
- **Stage 5, the Xbox:** the nxdk host, pbkit in place of OpenGL, the APU in place of the mixer, the packs, then the console work: pacing and vsync, the kept and lit models, fog, bloom, the translucent sort, the sound pack, the math made more native to the Xbox, the pbkit lost-swap fix, and the bigger models' arena.
