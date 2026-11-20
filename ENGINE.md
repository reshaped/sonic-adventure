# Engine

The engine runs *Sonic Adventure* on the Dreamcast and PC without replacing the game's C. `make` still reconstructs SEGA's binaries byte for byte; `make dc` builds the Dreamcast port with GCC, and `make pc` builds the native game. A PSP backend and Windows build are planned.

The game keeps its tasks, collision, camera, animation and rules. The engine supplies input, storage, sound, resources and rendering: all five on PC, and rendering on Dreamcast, where the game keeps the Katana SDK for the rest.

[PORTS.md](PORTS.md) documents the non-matching code and Dreamcast port's compiler and link; [TOOLCHAIN.md](TOOLCHAIN.md) documents the matching reconstruction.

- **Architecture:** [Source boundaries](#source-boundaries), [C ABI](#c-abi), [Memory and lifetimes](#memory-and-lifetimes).
- **Game behavior:** [Numbers](#numbers), [Visibility](#visibility).
- **Platforms:** [Dreamcast](#dreamcast), [Native game](#native-game), [PC rendering](#pc-rendering), [Sound and services](#sound-and-services).
- **Building:** [Build goals](#build-goals).

## Source boundaries

| Part | Responsibility | Source |
|---|---|---|
| Game | Tasks, collision, animation, camera, drawing decisions and game state | `src/`, `sdk/`, `modules/`, `events/` |
| Game adapter | Translates SA1's calls and data into engine records | `engine/sa1/` |
| Core | C ABI, arenas, handles and events | `engine/include/`, `engine/runtime/` |
| Play math | SH-4 arithmetic that game behavior reads | `engine/numeric/` |
| Dreamcast | PowerVR submission, rendering, presentation and frame pacing | `engine/dc/` |
| PC | Vulkan rendering, SDL3, input, sound and storage | `engine/pc/` |
| Native runner | Compiles the game's C for a host and loads its programs | `engine/native/`, `engine/tools/` |

The game reaches the engine through [`sa_engine.h`](engine/include/sa_engine.h) and SA1's [`sa_sa1.h`](engine/sa1/sa_sa1.h). On the Dreamcast it also uses the headers and model walk under [`engine/dc/`](engine/dc). Where [`config/dreamcast.mk`](config/dreamcast.mk) replaces a source, calls retain the game's, Ninja's or Kamui's names. Backends provide services; they don't call the game's scheduler or read its task memory.

The Dreamcast sends draws immediately through inline, game-specific adapters, and links no C++ runtime core. The PC links the core and a backend, and converts the game's drawing calls into resources and records.

[`boundary.py`](engine/tests/boundary.py) restricts includes and symbol references between the runtime, backends, adapter, game and numeric library. [`config/parts.txt`](config/parts.txt) classifies each source by part and by play or presentation; [`tools/parts.py`](tools/parts.py) supplies those rows to the boundary, fast-math, make and rules tests.

The runtime core and PC backend use C++17, without exceptions or RTTI. The game's source remains C: compiling it as C++ would change its language rules, and `extern "C"` changes only linkage.

## C ABI

[`sa_engine.h`](engine/include/sa_engine.h) defines version 0 of the ABI. Clients build against headers from the same revision. The header compiles as C17 and C++17, with no C++ class, template, reference or exception.

The ABI has two kinds of data:

- A **record** may cross a process or machine boundary. It uses fixed-width integers and 32-bit floats, with a defined little-endian layout, natural scalar alignment and a size divisible by four. It contains no pointers.
- A **descriptor** passes memory within one process. It may contain pointers and is never written as a portable record.

The header asserts record layouts. `make engine-test` compiles the ABI as C and C++, and for SH-4 when the cross compiler is available.

### Results and handles

A call that can fail returns `sa_result`. `SA_OK` means success, nonnegative values may describe other outcomes, and negative values report errors.

A failing call leaves the engine's state unchanged and does not write its output parameters. The failures are:

| Result | Meaning |
|---|---|
| `SA_ERR_INVALID` | Invalid arguments, record values or call order |
| `SA_ERR_HANDLE` | Null, released, wrong-kind or otherwise invalid handle |
| `SA_ERR_FULL` | A fixed-capacity store has no room |
| `SA_ERR_UNSUPPORTED` | The backend lacks the service or mode |
| `SA_ERR_VERSION` | The client's ABI version or descriptor size does not match |
| `SA_ERR_IO` | Storage failed |
| `SA_ERR_MISSING` | Storage holds no such name |

A handle is 32 bits, containing its kind, slot and generation. Releasing it invalidates the handle immediately. Reusing the slot advances its generation, which wraps after 255 reuses.

The ABI takes no callback from the game. The game polls events and runs its own scheduler. One thread calls an engine.

## Memory and lifetimes

The client calls `sa_engine_memory` to size the block it supplies at creation. The runtime divides it into bounded arenas, handle tables and event storage; none grows afterward.

Platform allocations are separate. Vulkan may allocate device memory, textures, pipelines and staging buffers as resources are created or first used.

| Data | Lifetime |
|---|---|
| A draw's matrix and shade | Copied during its call |
| An asset supplied to `sa_resource_create` | Borrowed until `SA_EVENT_RETIRED` |
| Procedural vertices supplied to `sa_present_copy` | Copied into the picture's arena until it retires |
| A scene's part or lighting | Copied when supplied; kept until its scene or relevant resources end |
| A game's geometry passed to `sa_present_live` | Consumed during the call on Dreamcast; copied by SA1's adapter on PC |
| GPU resources | Reused only after the last picture reading them has completed |

A picture outlives the task that drew it. Deleting a task doesn't erase draws already submitted to a picture. Releasing a resource prevents new uses of its handle, but resources borrowed by earlier pictures remain alive until those pictures retire.

With a display rate set, the PC backend may reuse a picture's resources for the pictures between it and the next game picture, so release waits one picture longer. If no next picture will come, `sa_pc_retire` waits for pictures already begun, drops the kept scene and retires released resources.

Each backend declares its borrowing rule. Dreamcast uses `SA_BORROW_CALL`; PC uses `SA_BORROW_PICTURE`. The game's adapter must satisfy the selected rule before the call returns.

The engine refuses an overfull arena, event ring or resource table. It doesn't silently increase a bound or drop an operation that the ABI promises to retain.

## Numbers

Math whose result affects play gives the same bits on every host. Math used only to produce a picture may use the backend's native arithmetic.

The Dreamcast executes the SH-4's instructions. A host uses play math, declared by [`play_math.h`](engine/numeric/play_math.h), wherever its own arithmetic would give other bits: FTRV, FIPR, FSCA, FSRRA, FMAC, conversions, comparisons and the scalar operations, under the game's rounding. The library depends on neither the engine nor the game, and gives the same bits whatever the host FPU's mode.

[`math.c`](engine/numeric/math.c) is the readable reference. [`fast.c`](engine/numeric/fast.c) gives the same results with integer arithmetic, tables and bounded approximations, and the native game links it. Built with `SA_NUMERIC_CONSOLE`, both take the console's rules for a denormal operand, a NaN's form, FIPR, FTRV, FSCA and FSRRA; `make pc` links the default build.

No fast-math setting crosses the play/presentation boundary. Dreamcast code may use SH4ZAM's `SHZ_FAST_MATH` only in sources [`config/parts.txt`](config/parts.txt) marks as presentation.

## Visibility

The engine distinguishes two kinds of visibility.

**Compatibility visibility** is a decision the game reads. It stays in the game's code, under play math. A dash panel, for example, advances its animation only where the game's view test sees it. A renderer cannot change that test without changing play.

**Presentation visibility** decides what a backend draws. It may cull more geometry, render another view or keep offscreen objects, but its decisions never advance tasks, consume random numbers or otherwise alter play.

## Dreamcast

`make dc` builds the port with GCC 16.2, replacing selected assembly and SDK routines while keeping the game's C and its guarded non-matching code. [PORTS.md](PORTS.md) gives the compiler, flags and link rules. [`config/dreamcast.mk`](config/dreamcast.mk) names the replacements and added sources.

The Dreamcast backend uses SH4ZAM where its SH-4 math routines fit. [`batch.h`](engine/dc/batch.h) supplies the batched projection, dot products, lighting and store-queue output its single-vector routines don't. Ninja and Kamui remain where the game needs their state and services.

### Drawing

| Drawing path | Implementation |
|---|---|
| Basic models and land | [`engine/dc/sa1/model.c`](engine/dc/sa1/model.c) |
| Game sprites | [`engine/dc/sprite.c`](engine/dc/sprite.c) |
| Ninja sprites | [`engine/dc/njSprite.c`](engine/dc/njSprite.c) |
| Easy models | [`engine/dc/sa1/easy.h`](engine/dc/sa1/easy.h) |
| Screen polygons, lines and text | The format-specific drawers under `engine/dc/sa1/` |
| Vector and matrix operations | `engine/dc/ninja/`, [`math.c`](engine/dc/math.c) |
| Collision mesh operations | [`mesh.c`](engine/dc/mesh.c) |

The model walk, sprites and screen draws send PowerVR geometry during the call. The model walk caches land models' baked palette entries across pictures, keyed by model address and light; it clears the cache when another land loads or the cache fills.

The adapters are inline because calls and loading Ninja's state through pointers cost measurably on SH-4. They read the game's state by its original symbols and reach the drawing kernels without another dispatch. Format-specific headers keep incompatible drawers out of one object. The batch kernels take points, colors and matrices, with no dependency on SA1's tasks or Ninja's object types.

### Submission

Kamui owns the frame in the retail image. The port's [`owner.c`](engine/dc/owner.c) replaces `kmRender`, `kmFlipFrameBuffer` and `kmResetPointer`, including their interrupt handling, but keeps Kamui's texture management, DMA, vertex buffers and register helpers. It refuses `kmRenderDirect` and the two texture renders, which the game never reaches, while preserving the visible statistics and callback order.

The opaque list goes to the tile accelerator as the game writes it. The buffered lists are submitted when the picture ends, in the game's order.

### 16:9

16:9 widens the view by a third while the Dreamcast still draws into 640 by 480 pixels. A 16:9 display stretches them back to shape. Ninja's projection aspect stays unchanged; the window used by the game's view tests widens during play but remains 4:3 in attract demos and recordings ([16:9](README.md#169)). 2D draws keep their pivot at the middle or an edge, so full-screen covers still cover. [`wide.c`](engine/dc/wide.c) sets the picture's aspect and draws the side bands when the window stays 4:3. The PC uses the same rules through SA1's adapter.

### 60 fps

A split frame shows two pictures for one game frame, not twice as many simulation steps. The first takes step 0's drawing; the second takes step 1's and replays the draws it needs from step 0, restoring any game state the callbacks touch.

The presenter splits frames after enough consecutive frames have time to spare. It returns to one picture when the frame, upload queue or fixed drawing storage can't hold two.

### The readout

[`overlay.c`](engine/dc/overlay.c) draws the readout: pictures per second, frame times, the 16:9 and 60 fps modes, and pages for per-step times and SH-4 cache counters. The game's chord changes pages ([The readout](README.md#the-readout)).

## Native game

`make pc` builds the game for a host from the original disc. It uses the non-matching C chosen for the Dreamcast port, except for code that needs the SH-4 itself or the Dreamcast engine's sends ([Switches](PORTS.md#switches)).

The native slice compiles the game's sources as C. It uses Clang because its passes operate on LLVM's typed intermediate representation.

Four tools adapt the game's code for the host:

| Tool | Responsibility |
|---|---|
| [`marks.py`](engine/tools/marks.py) | Keeps 32-bit pointers in records whose layout the game depends on |
| [`refs.py`](engine/tools/refs.py) | Lowers marked pointer operations the host backend can't select directly |
| [`tools/floats.py`](tools/floats.py) | Reproduces SHC's constants and floating-point expressions |
| [`asm2c.py`](engine/tools/asm2c.py) | Translates selected shipped SH-4 assembly to C over a register model and play math |

The slice also provides the original fixed-address regions, program loading, the game's heap and thread switching, and the SDK behavior the game needs.

### Pointers and program loading

The game stores pointers in 32-bit records. Widening them on x86-64 would change task layouts, arrays and disc data. The native build marks the fields that must keep the SH-4 layout and places their targets within a mapped 4 GB domain.

The loader's programs overlap in memory on Dreamcast. The native build links them separately, resolves names shared with the main executable, and manages their data and bss as they load and unload. Reading a program's file restores its initial data and zeros its bss.

The game's threads run on separate low stacks, under the game's own scheduler and task records.

### Starting the game

A run starts from the disc, with a VMU image for its card. The PC stores the player's settings and card locally, or reads paths supplied on the command line. If no card exists, it creates and formats one. The game keeps its own menus, save format and progress rules; an existing card carries story progress, clear records and Chao data.

`play` finds some game statics by name in its own symbol table. `make pc` leaves the program unstripped; a copy without that table or a required name refuses to start and says what's missing.

## PC rendering

[`engine/pc/`](engine/pc) supplies a Vulkan raster renderer, with SDL3 for the window, pads and sound. Without SDL3 it builds for headless rendering.

Headless and windowed modes use the same picture path. The window shows the latest completed picture at the display's refresh without changing the game's clock. A picture may render above 640 by 480 and be filtered down to the display's resolution.

SA1's adapter converts the game's geometry into engine resources, separate from the disc's format and Vulkan's. The backend reads PVR textures, meshes, materials, palettes, fog and draw records. It reads no task records and runs no stage logic.

### The game adapter

[`engine/sa1/`](engine/sa1) gives the native game its drawing interface. Each draw records the game's state at the call: mesh, transform, materials, shade, view and the instance's game-defined name. Drawing tests that affect play still run in the game.

The adapter keeps the differences between the game's drawers:

- Basic models use SA1's palettes, indexed by each point's normal against the light.
- Easy and simple models use their own light calculations.
- The full Ninja drawer uses its light queue and materials.
- Sprites keep their facing, scale, depth and texture frame.
- Screen polygons and lines keep their widths, blending and screen coordinates.
- Procedural geometry carries the bytes made at the call.

A model the game's view test rejects is still recorded, with `SA_DRAW_OUT`, so the renderer knows its state while the faithful picture leaves it out.

SA1's shared rules and constants live in [`rules.h`](engine/sa1/rules.h), which both the Dreamcast drawers and the native adapter use. [`test_rules.py`](tests/test_rules.py) keeps copies of those rules from diverging.

### Transparency

PowerVR sorts translucent fragments per pixel. Sorting whole draws or triangles cannot reproduce crossings, so the PC backend sorts each pixel's fragments before composing them. It composes the nearest 64; `sa_pc_stats` counts fragments beyond that limit separately from fragments lost when a picture's lists run out of room.

### The render-owned scene

The faithful renderer draws each picture from the game's records. With `sa_pc_options.rate` set, the backend also keeps a scene across pictures.

A kept instance carries its game-defined identity, current and previous transforms, mesh, materials, shade and resource lifetimes. The game gives the backend an act's land through `sa_scene_begin`, `sa_scene_part` and related calls, including distant land omitted from its own draw list.

Scene parts outlive individual pictures and end with the act. Releasing a resource removes the parts that depend on it. A picture may hold draws made under several game views; each record names its view, and the backend carries it into the picture's view. The scene keeps the act's lights, but a picture takes its shading from the game's draws.

### Pictures between game frames

At a requested display rate, the PC draws pictures between the game's own. It interpolates paired instances' positions, scale and rotations, along with procedural geometry and views. Texture frames, HUD text, some colors and visibility have no useful midpoint, so they come from the nearer game picture. Each draw shades from the last palettes supplied before it in its own game picture. Without a requested rate, the game's pictures draw unchanged.

The scene keeps a game picture whole or not at all. If its stores fill, that game picture still draws complete in place of interpolated pictures; the next starts afresh, as after a cut. If an interpolated picture runs out of room for a draw, the game picture replaces it and each later interpolated picture. `sa_pc_stats` names the store that filled and counts the pictures shown this way.

## Sound and services

### Sound

The game's sound queues remain in [`src/sound/`](src/sound). The SA1 adapter implements the Shinobi and CRI calls the game makes; the PC backend plays them through [`sound.cpp`](engine/pc/sound.cpp). SEGA's `MASARU.DRV` ARM7 program doesn't run on PC: the backend handles the commands it would have received.

It covers the game's AICA sample formats, instrument banks, note envelopes, channel levels and panning, the game's DSP program, and ADX streams. The AICA envelope follows MAME's, whose licence notice the source carries.

The game's sound-dependent decisions use its own clock and stream state, not SDL's audio timing. Headless playback can write WAV output at the game's vsyncs; windowed audio renders on its own thread. The queue has fixed bounds and reports when it's full rather than dropping commands.

### Input

SDL3 combines the keyboard and connected gamepads into the game's port 0 pad. Buttons, analog triggers and sticks map to the game's values, with remapping, dead zones and rumble in the settings. The game samples the pad when it asks; the runtime never advances it between samples.

### Saves

The PC keeps the game's VMU data in files. From the game's point of view, storage requests finish synchronously and report their result through the engine. A write goes to a temporary file beside the save and is renamed into place, so an interrupted write leaves the previous save intact. The game's save format and checksums are unchanged.

## Build goals

[`engine/engine.mk`](engine/engine.mk) and the fragments under [`engine/mk/`](engine/mk) define the engine goals. The matching and Dreamcast build goals (`make`, `make check`, `make dc`, `make dc-disc` and `make dc-check`) don't read those files.

| Command | What it builds or checks |
|---|---|
| `make` | The retail matching reconstruction |
| `make dc` | The GCC Dreamcast port |
| `make dc-disc` | A disc image of the Dreamcast port |
| `make pc` | The native game, `play`, from the disc |
| `make play` | The native game in a window, from Emerald Coast's start |
| `make native` | A headless run of the native game for a fixed count of frames |
| `make engine-test` | The ABI, runtime, numeric and backend tests |
| `make accept` | Every gate a change must pass, recorded in `build/accept.txt` |
