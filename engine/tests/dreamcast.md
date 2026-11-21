# Dreamcast checks at 82f80e5a

SH-4 GCC 16.2.0 and SH4ZAM `810f9b4f` built the release disc. It ran in Flycast with its HLE BIOS. For the mesh, readout and stick test ELFs, see [dreamcast.py](dreamcast.py).

## Speed Highway

Sonic’s attract recording supplied the input and camera in 4:3 with 60 fps enabled. Each run covered 400 game frames between recording samples 400 and 1199. A timer around the readout draw calls measured their total cost per game frame. Busy time came from the game’s frame timer. Both timers use the emulated SH-4 clock.

| Page | Mean draw time | Mean busy time | 95th percentile busy time |
| --- | ---: | ---: | ---: |
| OFF | 0.7 µs | 13.893 ms | 16.497 ms |
| FPS | 8.4 µs | 13.897 ms | 16.500 ms |
| PERF | 182.9 µs | 13.985 ms | 16.585 ms |

All three runs showed both pictures on 399 frames and dropped the second on one. Editing the band submitter and its state preserved all 26,494 ordinary input sections’ addresses and sizes in the link map.

The lighting-cache probe shrank from 1,744 to 284 bytes and the weld tree search from 92 to 80. With just the two older function bodies substituted at their linked addresses and their four address relocations resolved, the same recording averaged 13.923 ms busy with a 16.552 ms 95th percentile. The paired run with the current functions averaged 13.893 ms with a 16.495 ms 95th percentile. Each dropped one second picture.

## Playback and geometry

The largest stick heading difference was one angle unit (1/65,536 of a turn). Of 64,911 active stick positions, 148 changed heading. The six shipped recordings contained 11,845 active samples. One Amy sample and two Big samples changed. Five demos reached their end markers. Knuckles restarted after 1,046, 654 and 654 samples on both the release build and the original disc.

A nonuniformly scaled translucent strip crossed the screen edge and near plane with MODEL_CLIP clear. Each case emitted geometry. With MODEL_CLIP set, the original sphere test culled the fixture. An ordinary model drawn before and after the test emitted identical defined header and vertex words.

Twinkle Park’s kart ran through boarding, driving, rails and release in both modes. The harness positioned it for boarding and rail entry, then induced a wreck. Physical console timing, DMA interrupt ordering and a frame-by-frame comparison of the rider and smoke remain open.
