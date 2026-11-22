/* the Xbox host's video: host_api's video calls on the NV2A, through nxdk's
 * pbkit, as xbox/host/pc/video.c makes them on OpenGL.
 *
 * vertices. a list's vertices go once (video_vertices) to a ring of
 * contiguous memory, which the NV2A reads, as the renderer's own
 * host_vertex: x and y in the console's pixels, rhw, u and v, the base and
 * offset colors as D3D colors, and a draw names its triangles' vertices by
 * index, 16 bits each in the push buffer (video_draw). the CPU
 * copies them and notes their rhw for the depth's mapping; the vertex
 * program, shaders/vertex.vp, does the rest: the depth, linear in rhw
 * (depth_next), which the NV2A interpolates across the screen as the
 * PowerVR interpolates rhw, so that the 24-bit depth buffer orders it as
 * rhw and each of the PowerVR's compares is the same compare here; w =
 * 1 / rhw, by which colors and texture coordinates interpolate in
 * perspective; the texture coordinates scaled to the stored texture; the
 * base color's alpha where the polygon ignores it, the offset color where it
 * has none, and the fog. its constants are the frame's and the draw's
 * (push_frame_constants, draw_batch's).
 *
 * the pixels. the TSP's texture/shading instruction, with the texture's
 * alpha or without, is one of nine combiner programs, shaders/NAME.ps, and
 * fog mode 3's polygon a tenth. each blends the fog in last, a pixel's own,
 * as the PowerVR's table fog is: its share is the alpha of a texture on
 * stage 1 (the fog, below), and its color the combiners' constant.
 * punch-through is the NV2A's alpha test, the user clip its window clip,
 * which can draw inside a rectangle or outside it.
 *
 * textures. most come from the pack xbox/host/xbox/xboxtex.c makes of the disc's
 * (D:\textures.pak, video_texture_packed): DXT1, DXT3 or swizzled
 * A1R5G5B5, with their mipmaps, read as they are into the pool. the rest
 * the renderer decodes and gives as ARGB (video_texture), and those are,
 * as the PowerVR's, 16 bits a texel, without mipmaps: R5G6B5
 * where every texel is opaque, A1R5G5B5 where they're opaque or clear,
 * A4R4G4B4 else, swizzled, as the NV2A's wrap and mirror need, each in a
 * square of powers of two, a texture that isn't one, as a strided one or a
 * movie's frame, padded. they live in a pool of fixed size: when it fills,
 * the host drops them all, and host_api's video_textures_lost tells the
 * renderer, which sends each again as it next draws it.
 *
 * the frame. the game draws on a surface of the host's own, antialiased:
 * two samples a pixel, side by side (the NV2A's center-corner pattern), so
 * 1280 by 480, with a depth buffer as large. positions stay in the
 * screen's pixels: the NV2A scales them by the surface's antialiasing
 * itself (doubled in the vertex program, the scene's left half filled the
 * screen under xemu). at the present the surface is a texture, drawn over pbkit's back
 * buffer with the bilinear filter between each pixel's two samples
 * (resolve). HOST_MSAA=0 draws on the back buffer itself. pbkit shows the
 * back buffer at the display's next blank, and the present waits for that
 * blank (HOST_VSYNC=0 doesn't), so one frame is in flight at most: double
 * buffering, though pbkit keeps its third buffer.
 *
 * HOST_720P=1, where the dashboard allows 720p (XVideoListModes checks its
 * flags), shows 1280 by 720: the game still draws its 640 by 480, which the
 * vertex programs scale by c[78] to the display's pixels, the port's 16:9
 * on (host.c), so that the picture is the anamorphic 16:9 the TV would
 * have stretched. no antialiased surface: the scene draws on pbkit's back
 * buffer, of two, with pbkit's depth buffer. its color is R5G6B5, which
 * halves the frame buffers (the NV2A's rule on a 16-bit color with a 32-bit
 * depth is untried on a console); where the dashboard refuses 720p, the
 * picture is 480p at 32 bits.
 *
 * HOST_SHOT frames are copied, as R5G6B5, to buffers that run.sh finds in
 * the guest's memory by the log's "shot" lines, at 640 by 480: a 720p
 * frame's taken a pixel in two across and in one and a half down */
#include <hal/video.h>
#include <pbkit/pbkit.h>
#include <pbkit/nv_regs.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <windows.h>
#include <xboxkrnl/xboxkrnl.h>

#include "../../port/host.h"
#include "../../port/texhash.h"
#include "video.h"

#define WIDTH 640
#define HEIGHT 480
/* the mask's lowest bit by the compiler: ffs was a call a field */
#define MASK(mask, val) (((val) << __builtin_ctz(mask)) & (mask))

/* pbkit's pb_push1 and pb_push, written in place: because each was three
 * calls into pbkit for two words, a hot spot in busy stages, the words are
 * written here: a method's count, its subchannel, 3D's 0, and its address,
 * then its parameter */
static inline uint32_t *push1_inline(uint32_t *p, DWORD command, DWORD parameter)
{
    p[0] = (1u << 18) | command;
    p[1] = parameter;
    return p + 2;
}
#define pb_push1(p, command, parameter) push1_inline((p), (command), (parameter))
#define pb_push(p, command, count) (*(p) = ((DWORD)(count) << 18) | (DWORD)(command))

void host_log_line(const char *format, ...);
/* pbkit's, not in its header (pbkit.c) */
void pb_assign_tile(int tile_index, DWORD tile_addr, DWORD tile_size, DWORD tile_pitch, DWORD tile_z_start_tag,
                    DWORD tile_z_offset, DWORD tile_flags);

/* the vertex layout the NV2A reads: 13 words */

/* the ring (video_vertices) and the two arenas for the models' vertices
 * (video_arena) are rooms of contiguous memory of their own, as the store
 * is: the vertex arrays point at the one the draws name (set_attributes),
 * and an index, 16 bits, counts from its room's start. the arenas share
 * one room, whose 16-bit indices reach both. the lists' vertices took 2,832
 * at the most in Speed Highway's third act, now that the models and
 * sprites go to the host as runs, but Ice Cap once sent 8,703 at once
 * (video_vertices), so the ring is 10,000, 273 KB. a list may take the ring
 * but 1,024, and its clips what is left. the arenas, which a busy act filled
 * to 23,394 of 26,768, are 29,768 each */
#define ARENA_VERTICES 29768u
#define ARENAS_SIZE (2 * ARENA_VERTICES * sizeof(struct host_vertex))
#define RING_VERTICES 10000u
#define RING_SIZE (RING_VERTICES * sizeof(struct host_vertex))
static struct host_vertex *ring;
static struct host_vertex *arenas;
/* the kept models' vertices (video_store), which hold from frame to frame,
 * in a room of their own: the vertex arrays point at the one room or the
 * other (set_attributes), as the draws name vertices of each */
/* 45,000: a busy stage keeps 22,000 to 30,000, with room for the models lit
 * by the host. where it fills, the store takes back records unused for two
 * frames (render.c's kept models), so a scene past it keeps fewer models */
#define STORE_VERTICES 45000u
static struct host_vertex *store;
static const struct host_vertex *arrays_at; /* where the vertex arrays point: ring, store, or null for nowhere yet */
static unsigned ring_used; /* in vertices */

/* the push buffer's room, which a frame may outgrow: the host counts what it
 * pushes, and waits and starts it again near its end, a frame's hitch.
 * because pbkit's default 512 KB filled in busy stages, and 1 MB came to
 * 937 KB in Speed Highway's third act, it is 2 MB, a power of two as pbkit
 * needs, from the room Kamui's lists gave up (xbox/port/port.h) */
#define PUSH_SIZE 0x200000
static unsigned pushed;
/* for the report: the most a frame pushed and put in the ring, and the
 * times either filled within a frame and waited for the NV2A to start over */
static unsigned most_pushed, most_ring, push_restarts, ring_restarts;
/* clipped triangles the ring had no room for, left undrawn, for the report */
static unsigned ring_clips_dropped;
static unsigned texture_drops_all;

/* textures, by the renderer's id */
#define TEXTURES 2048
struct texture {
    UCHAR *texels; /* null for none */
    unsigned bytes;
    unsigned format; /* the NV2A's color format */
    unsigned log_w, log_h;
    unsigned levels; /* its mipmap levels, 1 for none */
    float u_scale, v_scale;
    /* a texture that isn't swizzled, as the antialiased surface: its
     * pitch, or 0, and its sizes, which its coordinates count in texels */
    unsigned pitch, rect_w, rect_h;
    int shared; /* texels of the pack's, resident (pack_load), which other ids may name too: never written */
    int block; /* its texels' block of the pool, plus 1, or 0 */
    int pack_index; /* the pack's entry it names, plus 1, or 0: read again if its block is evicted */
};

/* the scene's surface and its depth buffer, which every mode draws the
 * scene on (target_scene), and the resolve then copies to pbkit's back
 * buffer: twice the game's width with antialiasing (msaa, two samples a
 * pixel), else the display's size, in the display's color. so only the
 * resolve and the bloom draw on the back buffer, once the display has
 * left it (finish_frame) */
#define RESOLVE_TEXTURE (TEXTURES - 1)
static unsigned scene_w, scene_h, scene_pitch;
/* the two buffers' sizes, each a whole number of 16 KB, as a tile wants */
#define TILE_ROUND(bytes) (((bytes) + 0x3fffu) & ~0x3fffu)
static unsigned scene_color_size, scene_depth_size;
static int msaa, vsync;
/* the display: 640 by 480, or 1280 by 720 (HOST_720P), its back buffer
 * R5G6B5 where out_16 */
static int hd, out_16;
static unsigned out_w = WIDTH, out_h = HEIGHT;
static DWORD shown_queued_at = ~0u; /* pbkit's blank count as the last frame was queued to show (show_pending) */
static unsigned off_screen_waits; /* swaps that came more than a blank late (wait_off_screen), for the report */
static int frame_unfinished; /* the last present's frame awaits its resolve and bloom (finish_frame) */
static UCHAR *scene_color, *scene_depth;
static int depth_noting = 1; /* video_vertices notes the vertices' rhw */
/* the guard band's reach past the screen's sides: narrower on the
 * antialiased surface, where the NV2A doubles x */
static float guard_x = 512.0F;
/* the guard band above and below */
static float guard_y = 512.0F;
static struct texture textures[TEXTURES];
const char *setting(const char *key);
static UCHAR *pool;
static unsigned pool_size, pool_used;
/* what the pool has given back for the host's other needs (video_give_memory),
 * left free past the 4 MB each time it's taken again */
static unsigned pool_given;
/* the pool's blocks, in address order (a list through next): each holds a
 * texture's texels, the frame that last drew it, and whether it's the
 * pack's entry's or a decoded texture's own (pinned while its id names
 * it). where a texture finds no room, the blocks drawn least lately are
 * evicted, never one the frame being made has drawn: video_begin waited
 * for the NV2A to finish the frames before. a texture of the pack whose
 * block went is read again at its next draw (texture_bound). because
 * dropping every texture when full left the frame's remaining strips
 * untextured (a flicker) and reread them (a slowdown), it evicts instead */
#define BLOCKS 4096
static struct block {
    unsigned offset, size;
    unsigned last_used; /* video_frame of its last draw; 0 for none yet */
    int entry; /* the pack's entry, or -1 */
    int pinned; /* a decoded texture's, while its id names it */
    int next; /* the next by address, or -1 */
    int in_use;
} blocks[BLOCKS];
static int block_first = -1;
static unsigned video_frame = 1;
static unsigned texture_evictions;
static int *pack_slot; /* each pack entry's block, plus 1, or 0 */
static void block_evict(int b);

/* the textures' pool: what's free, but 1.5 MB for the host and 2.5 MB for
 * the video memory the game's writes may yet take (the host's vram_commit:
 * the textures the pack lacks), and 10 MB at the most.
 * Speed Highway's third act used 6,637 KB of 12,672 with none evicted: the
 * rest stays free, for the host's other rooms and a movie's. the report
 * counts the evictions of a stage that needs more */
#define POOL_MOST (10u << 20)
static void pool_take(void)
{
    MM_STATISTICS stats;
    unsigned most = POOL_MOST;

    memset(&stats, 0, sizeof stats);
    stats.Length = sizeof stats;
    MmQueryStatistics(&stats);
    unsigned leave = 0x400000 + pool_given;

    pool_size = stats.AvailablePages * 4096 > leave + 0x100000 ? stats.AvailablePages * 4096 - leave : 0;
    if (pool_size > most) {
        pool_size = most;
    }
    pool_size &= ~0xffffu;
    pool = NULL;
    /* the largest piece that's in one, if all of it isn't */
    while (pool_size >= 0x100000 &&
           (pool = MmAllocateContiguousMemoryEx(pool_size, 0, 0x03ffb000, 0x10000,
                                                PAGE_READWRITE | PAGE_WRITECOMBINE)) == NULL) {
        pool_size -= 0x100000;
    }
    if (pool == NULL) {
        pool_size = 0;
    }
}
static int textures_lost;
/* counts the log reports */
static unsigned texture_uploads, texture_uploads_new, texture_drops, texture_packed, texture_preloads, texture_reloads;

/* the pack: its entries, in their keys' order, and the file */
static struct texpack_entry *pack_entries;
static unsigned pack_count;
/* each entry's texels in the pool, read as the game loads the texture
 * (video_texture_known, from kmLoadTexture) or at its first draw, and kept
 * by key until the pool empties: a draw of a texture of the pack names them
 * and reads nothing. because reading at the first draw made a camera's
 * sweep over new textures hitch, the reads happen at the game's loads */
/* the pool emptied: no block holds anything */
static void pack_forget(void)
{
    if (pack_slot != NULL) {
        memset(pack_slot, 0, pack_count * sizeof *pack_slot);
    }
    memset(blocks, 0, sizeof blocks);
    block_first = -1;
    pool_used = 0;
}
static HANDLE pack_file = INVALID_HANDLE_VALUE;
static void pack_open(void);

/* the fog, from video_fog */
static unsigned fog_table_color, fog_vertex_color;
static float fog_density;
static unsigned short fog_table[128];
/* the fog. the PowerVR's table fog takes a pixel's rhw times the density,
 * from 1 to 256, on a log scale of 16 entries to a doubling, each entry two
 * bytes the fraction blends between. here that is a texture, 4096 texels of
 * alpha in a row, texel j the table's value at j / 16, which the bilinear
 * filter blends between as the fraction does: the vertex program gives the
 * coordinate (shaders/vertex.vp), which the NV2A divides a pixel at a time.
 * a polygon under vertex fog, or none, takes a second texture in its place,
 * a ramp of 256, by the vertex's fog. both are in a room of their own,
 * which holds as the textures' pool comes and goes. fog_stage is which the
 * NV2A's stage 1 has, 1 the table and 2 the ramp, and fog_factor the fog's
 * color in its combiners.
 *
 * the vertex program's constants are sent as they change: the frame's, and
 * a draw's */
/* the bloom, HOST_BLOOM: a soft glow around what is bright in the finished
 * frame, as many games of the Xbox's years had. four passes, each a quad
 * with four taps of the bilinear filter (shaders/blit.vp, bloom_*.ps): the
 * frame's mean over each 4 by 4 pixels, less a threshold and times four,
 * onto a surface a quarter its size each way; that blurred onto a second
 * such surface, and back onto the first, the taps wider each time; and the
 * result added to the frame, times the strength. the strength and the
 * threshold are percents, HOST_BLOOM (0 for none) and
 * HOST_BLOOM_THRESHOLD. only a stage's or a field's frames take it
 * (video_bloom_on) */
#define BLOOM_W (WIDTH / 4)
#define BLOOM_H (HEIGHT / 4)
#define BLOOM_PITCH (BLOOM_W * 4)
/* the bloom's pictures: a fourth of 640 by 480, or an eighth of 1280 by
 * 720, 160 by 90, in the same memory */
static unsigned bloom_w = BLOOM_W, bloom_h = BLOOM_H;
#define BLOOM_STRENGTH 40
#define BLOOM_THRESHOLD 75
/* set by the host's chores (host.c) while the game is in a stage or a
 * field: the logo, the title, the menus and the movies, whose pictures are
 * bright all over, take no bloom */
int video_bloom_on;
/* the bloom's two working surfaces, and a third that holds its glow from
 * frame to frame (bloom) */
static UCHAR *bloom_surface[3];
static int bloom_was_on;
static unsigned bloom_strength, bloom_threshold; /* of 255 */

#define FOG_TEXELS 4096
#define FOG_RAMP 256
static UCHAR *fog_texels; /* the table's texture, then the ramp */
static int last_fog_stage = -1;
static unsigned last_fog_factor = 1; /* no color: an alpha of 0 */
static int frame_constants = 1, fog_constants = 1; /* fog_constants: the table changed, and its texture is to fill */
static float last_draw_constants[12];
static int draw_constants_set;
/* what a draw's constants come from, as last built: the texture, the fog's
 * mode and the two flags. a draw with the same key has the same constants,
 * until a texture or the fog's colors change (constants_stale) */
static unsigned last_constants_key = ~0u;
#define constants_stale() (last_constants_key = ~0u, last_lit = -1, last_vertex_program = -1)

/* the models the host lights (host.h's host_item): each palette a texture
 * of 256 by 2 ARGB texels, swizzled, the base colors' row and the offset
 * colors', which stages 2 and 3 sample (shaders/lit.vp, shaders/lit_NAME.ps).
 * a palette takes a texture of the same 512 words where one holds them, or
 * the one drawn with longest ago: video_begin waits for the frame before,
 * so only the frame's own are in use */
#define LIT_TEXTURES 16
#define LIT_SLOT 40 /* shaders/lit.vp's place among the vertex programs' */
static uint32_t *lit_texels; /* LIT_TEXTURES of 512 words */
static uint32_t lit_words[LIT_TEXTURES][512]; /* each texture's palette, as given */
static unsigned lit_frame[LIT_TEXTURES]; /* the video_frame each was last drawn with, 0 for none */
static const unsigned *lit_from[LIT_TEXTURES]; /* the palette that frame gave for it */
static int last_lit = -1; /* the texture stages 2 and 3 hold, or -1 */
static int last_vertex_program = -1; /* 0 vertex.vp's, 1 lit.vp's */
static float last_light[3];
static unsigned lit_uploads, lit_draws; /* for the profile */
/* a palette's slot taken from a draw of the same frame, which may then draw
 * in the new one's colors, and the most slots one frame took: the report's */
static unsigned lit_overwrites, lit_most_frame, lit_this_frame, lit_frame_counted;
/* the lit item video_draw_items is drawing, or null */
static const struct host_item *lit_item;

/* the state last pushed, so that a draw pushes what changes */
static int last_program = -1;
static unsigned last_texture = ~0u;
/* the wrap, clamp and mirror modes and the filter set with it: a draw of the
 * same texture in another mode sets them again (Station Square's awning,
 * whose stripes repeat three times, drew them once, clamped as the strip
 * before it) */
static unsigned last_sampling = ~0u;
static int last_blend = -1, last_depth = -1, last_alpha = -1, last_clip = -1;
static unsigned last_clip_rect[4];

/* the shots */
#define SHOTS 8
static unsigned shot_frames[SHOTS];
static int shots_due; /* shots of the frame presented, for video_begin to take */
static void take_shot(unsigned frame, USHORT *shot);
static USHORT *shot_buffers[SHOTS];
static int shot_count, shots_done;
unsigned host_script_now(void);

/* the push buffer's open block: a draw's pushes continue it, and it goes to
 * the NV2A (pb_end) once KICK_BYTES have gathered, and before any wait. each
 * pb_end flushes the write-combining buffers through the NV2A and waits for
 * it (pb_cache_flush), which, three times a draw and up to 3300 draws a
 * frame, was among the host's largest costs on a real Xbox */
#define KICK_BYTES 0x4000
static uint32_t *open_block; /* where the open block's next push goes */
static unsigned unkicked; /* its bytes */

/* HOST_GPU_PROFILE=1: the NV2A's own time for each part of a picture. each
 * kick of the push buffer (push_kick) waits until the NV2A is idle, and the
 * wait counts in the part its pushes were for; a change of part kicks, and
 * nothing else does (no KICK_BYTES). so a part's time is the NV2A's alone,
 * with no wait for the CPU in it, but the two no longer overlap: the frame
 * rate falls, and the parts' sum is more than an unprofiled picture's by
 * the NV2A's start and drain at each wait. so each picture also times an
 * empty wait, a push of one NO_OPERATION, and the report takes that mean
 * off each wait of a part. the parts: the clear and the frame's setup, the
 * movie, the three lists, each split into draws of the lists' vertices and
 * of models, the stage's sky (host.h's sky, in any list), then the resolve
 * and the bloom. the translucent's depth passes
 * (draw_strips) count in it: they alternate with its draws, strip by strip,
 * and a part of their own waited at each, which made the profile mostly
 * waits. =2 also times each draw of one picture a second alone, the depth
 * passes named apart, and keeps the costliest for the report */
enum { GPU_CLEAR, GPU_MOVIE, GPU_OPAQUE, GPU_PUNCH, GPU_TRANSLUCENT, GPU_DEPTH, GPU_SKY, GPU_RESOLVE, GPU_BLOOM, GPU_SECTIONS };
#define GPU_PARTS (GPU_SECTIONS * 2) /* a section's draws of the lists' vertices, then of models */
#define GPU_TOP 16
struct gpu_draw {
    ULONGLONG ticks;
    unsigned waits;
    unsigned texture;
    unsigned triangles, vertices;
    unsigned char part, src_blend, dst_blend, punch_through, depth_write, cull, clip_mode, lit;
};
static int gpu_profile;
static int gpu_part = -1; /* the part the pushes now count in, or none */
static ULONGLONG gpu_frequency; /* the performance counter's */
static ULONGLONG gpu_frame_ticks[GPU_PARTS], gpu_sum_ticks[GPU_PARTS], gpu_most_ticks[GPU_PARTS];
static unsigned gpu_frame_draws[GPU_PARTS], gpu_sum_draws[GPU_PARTS];
static unsigned gpu_frame_waits[GPU_PARTS], gpu_sum_waits[GPU_PARTS];
static ULONGLONG gpu_empty_ticks; /* the empty waits' (gpu_picture_end), one a picture */
static unsigned gpu_empty_count;
static unsigned gpu_frame_triangles[GPU_PARTS], gpu_sum_triangles[GPU_PARTS];
static ULONGLONG gpu_sum_all, gpu_most_all;
static unsigned gpu_pictures, gpu_waits;
static int gpu_each; /* this picture's draws are timed each alone (=2) */
static struct gpu_draw gpu_open; /* the draw being timed */
static int gpu_open_on;
static struct gpu_draw gpu_top[GPU_TOP], gpu_kept[GPU_TOP]; /* this picture's costliest, and the last timed one's */
static int gpu_top_count, gpu_kept_count;
static unsigned gpu_each_draws, gpu_kept_draws, gpu_kept_frame;
static ULONGLONG gpu_each_ticks, gpu_kept_ticks;

/* wait for the NV2A to finish what was kicked, at PASSIVE_LEVEL as
 * wait_idle, and count the wait in the part and the draw being timed */
static void gpu_wait(void)
{
    KIRQL was = KeGetCurrentIrql();
    ULONGLONG before;
    ULONGLONG took;

    if (was > PASSIVE_LEVEL) {
        KfLowerIrql(PASSIVE_LEVEL);
    }
    before = KeQueryPerformanceCounter();
    while (pb_busy()) {
    }
    took = KeQueryPerformanceCounter() - before;
    if (was > PASSIVE_LEVEL) {
        KfRaiseIrql(was);
    }
    gpu_waits++;
    if (gpu_part >= 0) {
        gpu_frame_ticks[gpu_part] += took;
        gpu_frame_waits[gpu_part]++;
    }
    if (gpu_open_on) {
        gpu_open.ticks += took;
        gpu_open.waits++;
    }
}

static uint32_t *push_begin(void)
{
    if (open_block == NULL) {
        open_block = pb_begin();
    }
    return open_block;
}

static void push_kick(void)
{
    if (open_block != NULL) {
        pb_end(open_block);
        open_block = NULL;
        unkicked = 0;
        if (gpu_profile) {
            gpu_wait();
        }
    }
}

static void push_end(uint32_t *start, uint32_t *p)
{
    pushed += (unsigned)(p - start) * 4;
    unkicked += (unsigned)(p - start) * 4;
    open_block = p;
    if (unkicked >= KICK_BYTES && !gpu_profile) {
        push_kick();
    }
}

/* the vertices the draws name by index (video_vertices): where the renderer
 * has them, where they are in the ring, and which of them lie outside the
 * guard band */
static const struct host_vertex *block_source;
static unsigned block_base, block_count;
static int block_any_outside;
static int block_short; /* fewer than the renderer gave: some indices name none */

/* a model's vertices (video_vertices3), which the vertex program turns to
 * the screen by rows: where they are in the ring. the rows, and whether the
 * vertices are a model's, are the vertex program's c[72] to c[75], as last
 * pushed by last_rows_id. a model's triangles are culled by the NV2A, as
 * last_cull says */
static unsigned model_base, model_count;
static const struct host_vertex *model_room; /* the ring, the arenas or the store */
/* the arena the port writes the next frame's models into, 0 or 1: the
 * other holds the models of the frame being drawn, which the NV2A may
 * still read after the present. video_begin swaps them */
static unsigned arena_write;
/* a present leaves the NV2A drawing while the game makes the next frame, and the frame is
 * shown (pbkit's pb_finished) as the next one begins, when the NV2A has
 * finished it: so at the blank after the game's own frame deadline, each
 * frame alike. (shown as the NV2A finished it, some blanks came before it
 * was done and some after, and a real Xbox's picture moved unevenly, at 30
 * frames a second by the count.) frame_pending is that frame, and
 * pending_blanks the blanks since its present: where no frame follows, as
 * across a load, the host's chores show it (video_idle_present) */
static int frame_pending;
static unsigned pending_blanks;
static float model_rows[16]; /* the rows, then 1, 0, 0, 0 */
/* which rows the vertex program has: 0 for those that leave a vertex on the
 * screen where it is, else model_rows as they were when rows_id had that
 * value, which goes up as they change */
static unsigned rows_id = 1;
static unsigned last_rows_id = ~0u;
static const float *model_rows_from; /* the transform they were copied from */
static int last_cull = -1;
static unsigned model_vertices_in, model_draws; /* for the profile */
static unsigned char block_outside[0x10000];

/* the draw that's open: its begin pushed, and its triangles' indices as
 * they come, two a word, which consecutive draws of one state add to (draw
 * batching), until flush_pending ends it. an odd index waits in
 * pending_carry for the next. the vertex arrays point at the ring's start
 * once a push buffer (attributes_set) */
static int pending_open;
static int pending_carry = -1;
static unsigned draws_in, draws_out; /* draw_batch's calls, and the draws pushed, for the profile */
static void set_attributes(const struct host_vertex *room, int formats);

static void flush_pending(void)
{
    uint32_t *p;
    uint32_t *start;

    if (!pending_open) {
        return;
    }
    p = start = push_begin();
    if (pending_carry >= 0) {
        p = pb_push1(p, NV097_ARRAY_ELEMENT32, (unsigned)pending_carry);
        pending_carry = -1;
    }
    p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
    push_end(start, p);
    pending_open = 0;
}

/* time less waits empty waits' mean (gpu_picture_end), to none at the least */
static ULONGLONG gpu_less_waits(ULONGLONG ticks, unsigned waits)
{
    ULONGLONG cost = gpu_empty_count != 0 ? gpu_empty_ticks * waits / gpu_empty_count : 0;

    return ticks > cost ? ticks - cost : 0;
}

/* the draw timed alone, done: among the picture's costliest, by insertion */
static void gpu_close_draw(void)
{
    int k;

    if (!gpu_open_on) {
        return;
    }
    gpu_open_on = 0;
    gpu_open.ticks = gpu_less_waits(gpu_open.ticks, gpu_open.waits);
    gpu_each_draws++;
    gpu_each_ticks += gpu_open.ticks;
    for (k = gpu_top_count < GPU_TOP ? gpu_top_count++ : GPU_TOP; k > 0 && gpu_top[k - 1].ticks < gpu_open.ticks; k--) {
        if (k < GPU_TOP) {
            gpu_top[k] = gpu_top[k - 1];
        }
    }
    if (k < GPU_TOP) {
        gpu_top[k] = gpu_open;
    }
}

/* the pushes from here count in part, or in none (-1): what's pushed so far
 * goes to the NV2A, and push_kick's wait counts it in the last part */
static void gpu_part_set(int part)
{
    if (!gpu_profile || (part == gpu_part && !gpu_open_on)) {
        return;
    }
    flush_pending();
    push_kick();
    gpu_close_draw();
    gpu_part = part;
}

/* a draw of video_draw or video_draw3 begins: its part, by the list it came
 * from, and with =2 a draw timed alone. the resolve's and the bloom's own
 * draws stay theirs */
static void gpu_draw_begin(const struct host_draw *draw, int model, int count)
{
    int section;
    int part;

    if (!gpu_profile || gpu_part >= GPU_RESOLVE * 2) {
        return;
    }
    section = draw->sky         ? GPU_SKY
              : draw->list == 1 ? GPU_OPAQUE
              : draw->list == 2 ? GPU_PUNCH
              : draw->list == 3 ? (draw->src_blend == 0 && draw->dst_blend == 1 ? GPU_DEPTH : GPU_TRANSLUCENT)
                                : GPU_MOVIE;
    /* the depth passes in the translucent's part; a draw keeps its name */
    part = (section == GPU_DEPTH ? GPU_TRANSLUCENT : section) * 2 + (model != 0);
    gpu_part_set(part);
    gpu_frame_draws[part]++;
    gpu_frame_triangles[part] += (unsigned)count / 3;
    if (gpu_each) {
        gpu_open.ticks = 0;
        gpu_open.waits = 0;
        gpu_open.texture = draw->texture;
        gpu_open.triangles = (unsigned)count / 3;
        gpu_open.vertices = model ? model_count : 0;
        gpu_open.part = (unsigned char)(section * 2 + (model != 0));
        gpu_open.src_blend = draw->src_blend;
        gpu_open.dst_blend = draw->dst_blend;
        gpu_open.punch_through = draw->punch_through;
        gpu_open.depth_write = draw->depth_write;
        gpu_open.cull = draw->cull;
        gpu_open.clip_mode = draw->clip_mode;
        gpu_open.lit = model && lit_item != NULL;
        gpu_open_on = 1;
    }
}

/* a picture's parts are done (video_begin, its resolve and bloom drawn):
 * into the sums, and the next picture's draws timed alone, one a second */
static void gpu_picture_end(void)
{
    ULONGLONG all = 0;
    int k;

    if (!gpu_profile) {
        return;
    }
    gpu_part_set(-1);
    {
        /* an empty wait, the NV2A idle: what a wait costs of itself */
        uint32_t *p = push_begin();
        uint32_t *start = p;
        ULONGLONG before = gpu_frame_ticks[0];
        unsigned waits = gpu_waits;

        p = pb_push1(p, NV097_NO_OPERATION, 0);
        push_end(start, p);
        gpu_part = 0;
        push_kick();
        gpu_part = -1;
        gpu_empty_ticks += gpu_frame_ticks[0] - before;
        gpu_empty_count++;
        gpu_frame_ticks[0] = before;
        gpu_frame_waits[0]--;
        gpu_waits = waits;
    }
    for (k = 0; k < GPU_PARTS; k++) {
        gpu_frame_ticks[k] = gpu_less_waits(gpu_frame_ticks[k], gpu_frame_waits[k]);
        all += gpu_frame_ticks[k];
    }
    if (all != 0 || gpu_frame_waits[GPU_CLEAR * 2] != 0) {
        for (k = 0; k < GPU_PARTS; k++) {
            gpu_sum_ticks[k] += gpu_frame_ticks[k];
            if (gpu_frame_ticks[k] > gpu_most_ticks[k]) {
                gpu_most_ticks[k] = gpu_frame_ticks[k];
            }
            gpu_sum_draws[k] += gpu_frame_draws[k];
            gpu_sum_waits[k] += gpu_frame_waits[k];
            gpu_sum_triangles[k] += gpu_frame_triangles[k];
        }
        gpu_sum_all += all;
        if (all > gpu_most_all) {
            gpu_most_all = all;
        }
        gpu_pictures++;
    }
    memset(gpu_frame_ticks, 0, sizeof gpu_frame_ticks);
    memset(gpu_frame_draws, 0, sizeof gpu_frame_draws);
    memset(gpu_frame_waits, 0, sizeof gpu_frame_waits);
    memset(gpu_frame_triangles, 0, sizeof gpu_frame_triangles);
    if (gpu_each) {
        gpu_each = 0;
        memcpy(gpu_kept, gpu_top, sizeof gpu_top);
        gpu_kept_count = gpu_top_count;
        gpu_kept_draws = gpu_each_draws;
        gpu_kept_ticks = gpu_each_ticks;
        gpu_kept_frame = video_frame;
    } else if (gpu_profile == 2 && video_frame % 60 == 0) {
        gpu_each = 1;
        gpu_top_count = 0;
        gpu_each_draws = 0;
        gpu_each_ticks = 0;
    }
}

/* count indices, each from room's vertex base, room the ring or the store,
 * onto the open draw, which it begins if none is */
static void push_indices(const unsigned short *indices, int count, unsigned base, const struct host_vertex *room)
{
    uint32_t *p;
    uint32_t *start;
    int k = 0;

    if (count <= 0) {
        return;
    }
    if (room != arrays_at) {
        flush_pending();
        /* the formats once a push buffer, as arrays_at is null after a
         * reset; a move between the rooms, which the sorted translucent
         * draws make some hundreds of times a frame, only the offsets */
        set_attributes(room, arrays_at == NULL);
        arrays_at = room;
    }
    if (!pending_open) {
        p = start = push_begin();
        /* the NV2A caches the vertices it fetches, and the ring's memory
         * holds other ones each frame: a real NV2A then draws stale ones,
         * which xemu, with no such cache, never shows */
        p = pb_push1(p, NV097_BREAK_VERTEX_BUFFER_CACHE, 0);
        p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_TRIANGLES);
        push_end(start, p);
        pending_open = 1;
        draws_out++;
    }
    if (pending_carry >= 0) {
        p = start = push_begin();
        p = pb_push1(p, NV097_ARRAY_ELEMENT16, (unsigned)pending_carry | (indices[0] + base) << 16);
        push_end(start, p);
        pending_carry = -1;
        k = 1;
    }
    while (count - k >= 2) {
        /* a command's words are 2047 at most. a word is two indices, the
         * first in its low half: the two as they lie in memory, plus base
         * in each half, which one 32-bit add gives, as an index plus base
         * never passes 16 bits (the vertex arrays' rooms are under 65,536
         * vertices). an unaligned load is the Pentium III's to take.
         * because three loads and shifts a word made this loop a hot spot,
         * it adds a pair at a time */
        typedef uint32_t __attribute__((may_alias, aligned(2))) pair_t;
        int words = (count - k) / 2 > 1024 ? 1024 : (count - k) / 2;
        const pair_t *pairs = (const pair_t *)&indices[k];
        uint32_t add = base * 0x10001u;
        int j;

        p = start = push_begin();
        pb_push(p++, 0x40000000 | NV097_ARRAY_ELEMENT16, (unsigned)words);
        if (add == 0) {
            memcpy(p, (const void *)pairs, (size_t)words * 4);
            p += words;
        } else {
            for (j = 0; j < words; j++) {
                *p++ = pairs[j] + add;
            }
        }
        k += words * 2;
        push_end(start, p);
    }
    if (k < count) {
        pending_carry = (int)(indices[k] + base);
    }
}

/* the performance counter's ticks spent waiting for the NV2A to finish what
 * it was sent, and for the display's blank at a present, for host_api's
 * stats */
ULONGLONG video_gpu_wait_ticks, video_blank_wait_ticks;

/* wait for the NV2A to finish what it has, the open block with it. pbkit's
 * DPC moves it on, so the wait is at PASSIVE_LEVEL: the host's fast calls
 * come at DISPATCH_LEVEL (host.c's host_fast), where it never would */
static void wait_idle(void)
{
    KIRQL was = KeGetCurrentIrql();

    flush_pending();
    push_kick();
    if (was > PASSIVE_LEVEL) {
        KfLowerIrql(PASSIVE_LEVEL);
    }
    {
        ULONGLONG before = KeQueryPerformanceCounter();

        while (pb_busy()) {
        }
        video_gpu_wait_ticks += KeQueryPerformanceCounter() - before;
    }
    if (was > PASSIVE_LEVEL) {
        KfRaiseIrql(was);
    }
}

static void push_control(uint32_t **pp);
static void push_constants(uint32_t **pp, unsigned n, const float *v, unsigned count);

/* the display's color format, as pbkit's back buffer has it */
static unsigned out_color_format(void)
{
    return out_16 ? NV097_SET_SURFACE_FORMAT_COLOR_LE_R5G6B5 : NV097_SET_SURFACE_FORMAT_COLOR_LE_A8R8G8B8;
}

/* the scene's surface as the target, and its depth buffer, both through
 * the DMA object of all of memory, pbkit's 3, which its textures and
 * vertices go through. the clip is in the display's pixels: with two
 * samples a pixel the NV2A doubles x itself */
static void target_scene(void)
{
    uint32_t *p;
    uint32_t *start;

    p = start = push_begin();
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, 3);
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_ZETA, 3);
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 MASK(NV097_SET_SURFACE_PITCH_COLOR, scene_pitch) | MASK(NV097_SET_SURFACE_PITCH_ZETA, scene_w * 4));
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, MmGetPhysicalAddress(scene_color) & 0x03ffffff);
    p = pb_push1(p, NV097_SET_SURFACE_ZETA_OFFSET, MmGetPhysicalAddress(scene_depth) & 0x03ffffff);
    p = pb_push1(p, NV097_SET_SURFACE_FORMAT,
                 MASK(NV097_SET_SURFACE_FORMAT_COLOR, out_color_format()) |
                     MASK(NV097_SET_SURFACE_FORMAT_ZETA, NV097_SET_SURFACE_FORMAT_ZETA_Z24S8) |
                     MASK(NV097_SET_SURFACE_FORMAT_TYPE, NV097_SET_SURFACE_FORMAT_TYPE_PITCH) |
                     MASK(NV097_SET_SURFACE_FORMAT_ANTI_ALIASING,
                          msaa ? NV097_SET_SURFACE_FORMAT_ANTI_ALIASING_CENTER_CORNER_2
                               : NV097_SET_SURFACE_FORMAT_ANTI_ALIASING_CENTER_1));
    p = pb_push1(p, NV097_SET_SURFACE_CLIP_HORIZONTAL, out_w << 16);
    p = pb_push1(p, NV097_SET_SURFACE_CLIP_VERTICAL, out_h << 16);
    push_end(start, p);
}

/* near the push buffer's end, with bytes more to come: wait, and start it
 * again. the ring stays as it is: the draws name its vertices */
static void make_room(unsigned bytes)
{
    if (pushed + bytes > PUSH_SIZE - 0x10000) {
        uint32_t *p;
        uint32_t *start;

        push_restarts++;
        wait_idle();
        arrays_at = NULL;
        pb_reset();
        pb_target_back_buffer();
        pushed = 0;
        target_scene();
        p = start = push_begin();
        push_control(&p);
        push_end(start, p);
        last_program = -1;
        last_texture = ~0u, constants_stale();
        last_fog_stage = -1;
        last_fog_factor = 1;
        last_blend = last_depth = last_alpha = last_clip = -1;
        last_cull = -1;
    }
}

/* the combiner programs, each from its shaders/NAME.ps */
/* the draws' ten, and then a lit model's ten (shaders/lit.py), in the same
 * order */
#define PROGRAMS 20
#define PROGRAM_LIT 10
static void program_untextured(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/untextured.inl"
    *pp = p; }
static void program_decal(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/decal.inl"
    *pp = p; }
static void program_decal_opaque(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/decal_opaque.inl"
    *pp = p; }
static void program_modulate(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/modulate.inl"
    *pp = p; }
static void program_modulate_opaque(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/modulate_opaque.inl"
    *pp = p; }
static void program_decal_alpha(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/decal_alpha.inl"
    *pp = p; }
static void program_decal_alpha_opaque(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/decal_alpha_opaque.inl"
    *pp = p; }
static void program_modulate_alpha(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/modulate_alpha.inl"
    *pp = p; }
static void program_modulate_alpha_opaque(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/modulate_alpha_opaque.inl"
    *pp = p; }
static void program_fog_polygon(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/fog_polygon.inl"
    *pp = p; }
static void program_lit_untextured(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/lit_untextured.inl"
    *pp = p; }
static void program_lit_decal(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/lit_decal.inl"
    *pp = p; }
static void program_lit_decal_opaque(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/lit_decal_opaque.inl"
    *pp = p; }
static void program_lit_modulate(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/lit_modulate.inl"
    *pp = p; }
static void program_lit_modulate_opaque(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/lit_modulate_opaque.inl"
    *pp = p; }
static void program_lit_decal_alpha(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/lit_decal_alpha.inl"
    *pp = p; }
static void program_lit_decal_alpha_opaque(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/lit_decal_alpha_opaque.inl"
    *pp = p; }
static void program_lit_modulate_alpha(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/lit_modulate_alpha.inl"
    *pp = p; }
static void program_lit_modulate_alpha_opaque(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/lit_modulate_alpha_opaque.inl"
    *pp = p; }
static void program_lit_fog_polygon(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/lit_fog_polygon.inl"
    *pp = p; }
static void program_bloom_bright(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/bloom_bright.inl"
    *pp = p; }
static void program_bloom_blur(uint32_t **pp) { uint32_t *p = *pp;
#include "shaders/bloom_blur.inl"
    *pp = p; }
/* by shading * 2 + ignore, after the untextured one, and fog mode 3's last */
#define PROGRAM_FOG_POLYGON 9
static void (*const programs[PROGRAMS])(uint32_t **) = {
    program_untextured,  program_decal,           program_decal_opaque,         program_modulate,
    program_modulate_opaque, program_decal_alpha, program_decal_alpha_opaque, program_modulate_alpha,
    program_modulate_alpha_opaque, program_fog_polygon,
    program_lit_untextured,  program_lit_decal,           program_lit_decal_opaque,         program_lit_modulate,
    program_lit_modulate_opaque, program_lit_decal_alpha, program_lit_decal_alpha_opaque, program_lit_modulate_alpha,
    program_lit_modulate_alpha_opaque, program_lit_fog_polygon,
};

#define BLIT_SLOT 100
static void load_vertex_program(void)
{
    static const uint32_t program[] = {
#include "shaders/vertex.inl"
    };
    static const uint32_t blit[] = {
#include "shaders/blit.inl"
    };
    static const uint32_t lit[] = {
#include "shaders/lit.inl"
    };
    uint32_t *p;
    uint32_t *start;
    unsigned k;

    p = start = push_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, 0);
    p = pb_push1(p, NV097_SET_TRANSFORM_EXECUTION_MODE,
                 MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_PROGRAM) |
                     MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE,
                          NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 0);
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, 0);
    push_end(start, p);
    for (k = 0; k < sizeof program / 16; k++) {
        p = start = push_begin();
        pb_push(p++, NV097_SET_TRANSFORM_PROGRAM, 4);
        memcpy(p, &program[k * 4], 16);
        p += 4;
        push_end(start, p);
    }
    /* a lit model's, shaders/lit.vp, and the bloom's, shaders/blit.vp,
     * further on in the NV2A's 136 slots */
    _Static_assert(sizeof program / 16 <= LIT_SLOT && LIT_SLOT + sizeof lit / 16 <= BLIT_SLOT &&
                       BLIT_SLOT + sizeof blit / 16 <= 136,
                   "the vertex programs outgrow their slots");
    p = start = push_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, LIT_SLOT);
    push_end(start, p);
    for (k = 0; k < sizeof lit / 16; k++) {
        p = start = push_begin();
        pb_push(p++, NV097_SET_TRANSFORM_PROGRAM, 4);
        memcpy(p, &lit[k * 4], 16);
        p += 4;
        push_end(start, p);
    }
    p = start = push_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, BLIT_SLOT);
    push_end(start, p);
    for (k = 0; k < sizeof blit / 16; k++) {
        p = start = push_begin();
        pb_push(p++, NV097_SET_TRANSFORM_PROGRAM, 4);
        memcpy(p, &blit[k * 4], 16);
        p += 4;
        push_end(start, p);
    }
}

/* the attributes the vertex program reads, at room's start: their formats
 * too where formats is TRUE, which every room shares */
static void set_attributes(const struct host_vertex *room, int formats)
{
    static const struct {
        int index, type, size, offset;
    } used[5] = {
        /* host_vertex: x, y, rhw; u, v; the base and offset colors; and the
         * offset field again as a lit model's normal, CMP, which only
         * shaders/lit.vp reads */
        {0, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 3, 0},
        {9, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 2, 12},
        {3, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_UB_D3D, 4, 20},
        {4, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_UB_D3D, 4, 24},
        {2, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_CMP, 1, 24},
    };
    uint32_t *p;
    uint32_t *start;
    ULONG base = MmGetPhysicalAddress((void *)room);
    int k;

    p = start = push_begin();
    if (formats) {
        pb_push(p++, NV097_SET_VERTEX_DATA_ARRAY_FORMAT, 16);
        for (k = 0; k < 16; k++) {
            *p++ = NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F;
        }
    }
    for (k = 0; k < 5; k++) {
        if (formats) {
            p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_FORMAT + used[k].index * 4,
                         MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE, used[k].type) |
                             MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_SIZE, used[k].size) |
                             MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_STRIDE, sizeof(struct host_vertex)));
        }
        p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_OFFSET + used[k].index * 4,
                     (base + (ULONG)used[k].offset) & 0x03ffffff);
    }
    push_end(start, p);
}

int video_open(void)
{
    int status;
    char *list;
    const char *setting(const char *key);

    pb_size(PUSH_SIZE);
    /* pbkit's three frame buffers in every mode: the front buffer and two
     * back buffers, so that a frame can be queued while the last one waits
     * for its swap, as a split frame's two pictures a blank apart are. two,
     * which a pbkit patch gave, saved 1.2 MB the memory no longer needs */
    int want_hd = setting("HOST_720P") != NULL && strtoul(setting("HOST_720P"), NULL, 10) != 0;
    DWORD encoder = XVideoGetEncoderSettings();

    if (want_hd) {
        /* 720p where the dashboard allows it: XVideoListModes, which
         * XVideoSetMode walks, lists it only then (and for the HDTV pack) */
        out_16 = 1;
        pb_set_color_format(out_16 ? NV097_SET_SURFACE_FORMAT_COLOR_LE_R5G6B5 : NV097_SET_SURFACE_FORMAT_COLOR_LE_A8R8G8B8,
                            false);
        hd = (encoder & VIDEO_MODE_720P) != 0 && XVideoSetMode(1280, 720, out_16 ? 16 : 32, REFRESH_DEFAULT);
        if (!hd) {
            out_16 = 0;
            pb_set_color_format(NV097_SET_SURFACE_FORMAT_COLOR_LE_A8R8G8B8, false);
        }
        host_log_line("host: video: 720p asked for: %s (the dashboard's flags 0x%08lx)\n",
                      hd ? (out_16 ? "on, 16-bit color" : "on") : "not allowed, 640 by 480", (unsigned long)encoder);
    }
    if (hd) {
        out_w = 1280;
        out_h = 720;
        bloom_w = 160;
        bloom_h = 90;
    }
    if (!hd) {
        XVideoSetMode(WIDTH, HEIGHT, 32, REFRESH_DEFAULT);
    }
    status = pb_init();
    if (status != 0) {
        host_log_line("host: pb_init failed: %d\n", status);
        return -1;
    }
    pb_show_front_screen();
    {
        /* nxdk's own frame buffer, which XVideoSetMode made and the display
         * showed until pbkit's: nothing shows it now, and only debugPrint
         * draws on it, which the host never calls and pbkit and nxdk's
         * assert call only as they halt. 1.2 MB, or 3.6 MB at 720p, back to
         * the textures' pool. the host sets no mode again, which would free
         * it a second time (XVideoInit) */
        unsigned char *fb = XVideoGetFB();

        if (fb != NULL) {
            MmFreeContiguousMemory(fb);
        }
    }
    ring = MmAllocateContiguousMemoryEx(RING_SIZE, 0, 0x03ffb000, 0, PAGE_READWRITE | PAGE_WRITECOMBINE);
    arenas = MmAllocateContiguousMemoryEx(ARENAS_SIZE, 0, 0x03ffb000, 0, PAGE_READWRITE | PAGE_WRITECOMBINE);
    store = MmAllocateContiguousMemoryEx(STORE_VERTICES * sizeof(struct host_vertex), 0, 0x03ffb000, 0,
                                         PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (store == NULL) {
        host_log_line("host: video: no memory for the kept models' vertices: none kept\n");
    }
    lit_texels = MmAllocateContiguousMemoryEx(LIT_TEXTURES * 512 * 4, 0, 0x03ffb000, 0,
                                              PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (lit_texels == NULL) {
        host_log_line("host: video: no memory for the lit models' palettes: none lit\n");
    }
    {
        unsigned long strength = setting("HOST_BLOOM") != NULL ? strtoul(setting("HOST_BLOOM"), NULL, 10) : BLOOM_STRENGTH;
        unsigned long threshold = setting("HOST_BLOOM_THRESHOLD") != NULL
                                      ? strtoul(setting("HOST_BLOOM_THRESHOLD"), NULL, 10)
                                      : BLOOM_THRESHOLD;

        bloom_strength = (unsigned)((strength > 100 ? 100 : strength) * 255 / 100);
        bloom_threshold = (unsigned)((threshold > 100 ? 100 : threshold) * 255 / 100);
        if (bloom_strength != 0) {
            bloom_surface[0] = MmAllocateContiguousMemoryEx(BLOOM_PITCH * BLOOM_H, 0, 0x03ffb000, 0x1000,
                                                            PAGE_READWRITE | PAGE_WRITECOMBINE);
            bloom_surface[1] = MmAllocateContiguousMemoryEx(BLOOM_PITCH * BLOOM_H, 0, 0x03ffb000, 0x1000,
                                                            PAGE_READWRITE | PAGE_WRITECOMBINE);
            bloom_surface[2] = MmAllocateContiguousMemoryEx(BLOOM_PITCH * BLOOM_H, 0, 0x03ffb000, 0x1000,
                                                            PAGE_READWRITE | PAGE_WRITECOMBINE);
            if (bloom_surface[0] == NULL || bloom_surface[1] == NULL || bloom_surface[2] == NULL) {
                host_log_line("host: video: no memory for the bloom: none\n");
                bloom_strength = 0;
            }
        }
        host_log_line("host: video: bloom %lu%%, over %lu%%\n", bloom_strength != 0 ? strength : 0, threshold);
    }
    fog_texels = MmAllocateContiguousMemoryEx(FOG_TEXELS + FOG_RAMP, 0, 0x03ffb000, 0x1000, PAGE_READWRITE);
    if (fog_texels != NULL) {
        int k;

        memset(fog_texels, 0, FOG_TEXELS);
        for (k = 0; k < FOG_RAMP; k++) {
            fog_texels[FOG_TEXELS + k] = (UCHAR)k;
        }
    }
    list = (char *)setting("HOST_SHOT");
    while (list != NULL && *list != '\0' && shot_count < SHOTS) {
        char *end;

        shot_frames[shot_count] = (unsigned)strtoul(list, &end, 10);
        shot_buffers[shot_count] = MmAllocateContiguousMemoryEx(WIDTH * HEIGHT * 2, 0, 0x03ffffff, 0, PAGE_READWRITE);
        if (shot_buffers[shot_count] != NULL) {
            shot_count++;
        }
        list = *end == ',' ? end + 1 : (char *)"";
    }
    vsync = setting("HOST_VSYNC") == NULL || strtoul(setting("HOST_VSYNC"), NULL, 10) != 0;
    /* the scene's surface: with antialiasing (not at 720p) twice the game's
     * width, else the display's size; where memory refuses the doubled one,
     * the display's */
    msaa = !hd && (setting("HOST_MSAA") == NULL || strtoul(setting("HOST_MSAA"), NULL, 10) != 0);
    for (;;) {
        scene_w = msaa ? WIDTH * 2 : out_w;
        scene_h = out_h;
        scene_pitch = scene_w * (out_16 ? 2 : 4);
        scene_color_size = TILE_ROUND(scene_pitch * scene_h);
        scene_depth_size = TILE_ROUND(scene_w * 4 * scene_h);
        /* 16 KB aligned, for their tiles (scene_tiles) */
        scene_color = MmAllocateContiguousMemoryEx(scene_color_size, 0, 0x03ffb000, 0x4000,
                                                   PAGE_READWRITE | PAGE_WRITECOMBINE);
        scene_depth = MmAllocateContiguousMemoryEx(scene_depth_size, 0, 0x03ffb000, 0x4000,
                                                   PAGE_READWRITE | PAGE_WRITECOMBINE);
        if (scene_color != NULL && scene_depth != NULL) {
            break;
        }
        if (scene_color != NULL) {
            MmFreeContiguousMemory(scene_color);
        }
        if (scene_depth != NULL) {
            MmFreeContiguousMemory(scene_depth);
        }
        scene_color = scene_depth = NULL;
        if (!msaa) {
            host_log_line("host: no memory for the scene's surface, %ux%u\n", scene_w, scene_h);
            return -1;
        }
        host_log_line("host: video: no memory for the antialiased surface: one sample a pixel\n");
        msaa = 0;
    }
    if (msaa) {
        guard_x = 256.0F;
    }
    if (hd) {
        /* the guard band past the screen's sides, as with antialiasing: the
         * NV2A's pixels are twice the game's across */
        guard_x = 256.0F;
    }
    host_log_line("host: video: %ux%u, %s, %s\n", out_w, out_h, msaa ? "two samples a pixel" : "one sample a pixel",
                  vsync ? "each frame waits for its blank" : "frames don't wait for the blank");
    {
        /* pbkit's depth buffer, 1.2 MB (3.7 MB at 720p), which nothing draws
         * on: the scene has its surface's (target_scene), and the passes on
         * pbkit's back buffer after it, the resolve and the bloom, neither
         * test nor write depth. pbkit gives out no address for it, but shows
         * it on the screen for its debugging, and the display's start then
         * holds it: read so, before the first frame shows, and the front
         * buffer shown again. its tile, which pbkit set for Z compression
         * over it, is turned off first (pb_assign_tile, valid bit clear):
         * left on, the NV2A would read what the pool puts there through it.
         * pbkit's pb_kill would free it again, but the host never calls it */
        ULONG at;

        pb_show_depth_screen();
        at = *(volatile ULONG *)(0xfd000000u + 0x00600800u) & 0x0fffffffu;
        pb_show_front_screen();
        if (at != 0 && at < 0x04000000u) {
            pb_assign_tile(1, 0, 0x4000, 0, 0, 0, 0);
            MmFreeContiguousMemory((PVOID)(at | 0x80000000u));
            host_log_line("host: video: pbkit's depth buffer at 0x%08lx freed\n", at);
        }
    }
    {
        /* the scene's buffers as the NV2A's tiles, as pbkit sets its own:
         * the depth buffer as tile 1, pbkit's depth buffer's, with Z
         * compression (flags 0x80000000, compression tags from 0; 0x04000000,
         * 32-bit depth), which reads and writes a 4 by 4 block of like depth
         * as a few words, and clears it fast; the color as tile 2, which
         * pbkit leaves free without extra buffers, for the memory's banks.
         * a tile is the memory controller's: the CPU and the textures that
         * read the color (the resolve) see it as plain lines. the pitches,
         * 2560 or 5120 bytes, are among those a tile takes */
        pb_assign_tile(1, MmGetPhysicalAddress(scene_depth) & 0x03ffffff, scene_depth_size, scene_w * 4, 0, 0,
                       0x84000001);
        pb_assign_tile(2, MmGetPhysicalAddress(scene_color) & 0x03ffffff, scene_color_size, scene_pitch, 0, 0, 0);
        host_log_line("host: video: the scene's depth buffer tiled with Z compression, its color tiled\n");
    }
    pack_open();
    pool_take();
    if (ring == NULL || arenas == NULL || pool == NULL || fog_texels == NULL) {
        host_log_line("host: no memory for the vertices or the textures\n");
        return -1;
    }
    if (setting("HOST_GPU_PROFILE") != NULL) {
        gpu_profile = (int)strtoul(setting("HOST_GPU_PROFILE"), NULL, 10);
        gpu_frequency = KeQueryPerformanceFrequency();
        if (gpu_profile != 0) {
            host_log_line("host: video: the GPU profile on (HOST_GPU_PROFILE=%d): each part of a picture drawn alone, "
                          "the frame rate lower%s\n",
                          gpu_profile, gpu_profile == 2 ? "; each draw of a picture a second timed alone" : "");
        }
    }
    host_log_line("host: video: %u KB for textures, %u KB for the lists' vertices, %u KB for the models' arenas, %u KB "
                  "for the kept models', %u KB of push buffer, %d shots\n",
                  pool_size >> 10, (unsigned)(RING_SIZE >> 10), (unsigned)(ARENAS_SIZE >> 10),
                  store != NULL ? (unsigned)(STORE_VERTICES * sizeof(struct host_vertex) >> 10) : 0,
                  (unsigned)(PUSH_SIZE >> 10), shot_count);
    load_vertex_program();
    return 0;
}

static void push_control(uint32_t **pp)
{
    uint32_t *p = *pp;

    /* depth in 24 bits, fixed: depth_next's mapping of rhw */
    p = pb_push1(p, NV097_SET_CONTROL0, NV097_SET_CONTROL0_TEXTURE_PERSPECTIVE_ENABLE);
    /* the depth range, 0 to 2^24 - 1, clamped there, not culled */
    p = pb_push1(p, NV097_SET_CLIP_MIN, 0);
    p = pb_push1(p, NV097_SET_CLIP_MAX, 0x4b7fffff);
    p = pb_push1(p, NV097_SET_ZMIN_MAX_CONTROL,
                 NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_CLAMP | NV097_SET_ZMIN_MAX_CONTROL_CULL_IGNORE_W);
    p = pb_push1(p, NV097_SET_CULL_FACE_ENABLE, 0);
    /* COLOR1, the offset color, the characters' highlights among it: with
     * specular off, the NV2A passes 0 for it */
    p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, 1);
    p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, 0);
    p = pb_push1(p, NV097_SET_SHADE_MODEL, NV097_SET_SHADE_MODEL_SMOOTH);
    p = pb_push1(p, NV097_SET_COLOR_MASK,
                 NV097_SET_COLOR_MASK_BLUE_WRITE_ENABLE | NV097_SET_COLOR_MASK_GREEN_WRITE_ENABLE |
                     NV097_SET_COLOR_MASK_RED_WRITE_ENABLE | NV097_SET_COLOR_MASK_ALPHA_WRITE_ENABLE);
    *pp = p;
}

/* show the frame the last present left to the NV2A, which has finished it,
 * and turn to the next back buffer: pbkit swaps the frame in at the
 * display's next blank. pbkit's way to both is the NV2A's software
 * interrupts (pb_finished's PB_FINISHED, pb_target_back_buffer's four
 * PB_SETOUTER), at each of which the NV2A stops until pbkit's DPC has
 * served it, and while one waits the blank's interrupt is off: pbkit's ISR
 * turns it off until the DPC turns it on again. so at PASSIVE_LEVEL, where
 * the DPC runs at once, until the NV2A has run them all, which takes it
 * little: it has finished the frame before. because at DISPATCH_LEVEL the
 * DPC waited for the game's thread, the blank after found its interrupt
 * off and swapped no frame in, and pictures stayed one blank or three
 * where two were meant */
/* the frame the last present left, onto pbkit's back buffer: the
 * antialiased surface's resolve, then the bloom, as the next frame begins,
 * before it is shown (show_pending, video_idle_present). of the three
 * frame buffers the back buffer is the one the display shows only where
 * two swaps are still queued, as a frame ran ahead of the blanks: the
 * resolve then waits for the display to leave it (wait_off_screen), which
 * is no wait otherwise, where a wait at the present cost some 10 ms a
 * frame. the NV2A draws the next frame's scene on the
 * antialiased surface after the resolve has read it, in the order sent.
 * (the NV2A's own wait for the swap, FLIP_STALL, froze xemu) */
static void resolve(void);
static void bloom(void);

static void wait_off_screen(void);

static void finish_frame(void)
{
    if (!frame_unfinished) {
        return;
    }
    frame_unfinished = 0;
    if (shown_queued_at != ~0u) {
        /* the resolve and the bloom draw on the back buffer, which the
         * display may show until a queued swap: wait until it shows
         * another (wait_off_screen), not for one blank. the swap comes at
         * the first blank after the NV2A has finished the last frame:
         * because a swap can slip, a wait of one blank let the resolve draw
         * over the picture on the screen, a flash */
        KIRQL was = KeGetCurrentIrql();

        if (was > PASSIVE_LEVEL) {
            KfLowerIrql(PASSIVE_LEVEL);
        }
        wait_off_screen();
        if (was > PASSIVE_LEVEL) {
            KfRaiseIrql(was);
        }
    }
    shown_queued_at = ~0u;
    gpu_part_set(GPU_RESOLVE * 2);
    resolve();
    gpu_part_set(GPU_BLOOM * 2);
    bloom();
    flush_pending();
    push_kick();
    gpu_part_set(-1);
}

/* of the three frame buffers the back buffer is the screen's only where
 * two swaps are still queued: pbkit swaps at the first blank after the
 * NV2A has finished a frame, which may be blanks after the queue where the
 * frame was heavy, and 60 frames a second queues a picture a blank. drawn
 * on while the screen showed it, half the screen flickered (when the scene
 * drew on the back buffer, as it did without antialiasing), so the resolve
 * waits until the display starts elsewhere: no wait where the queue hasn't
 * run ahead */
#define DISPLAY_START (*(volatile ULONG *)(0xfd000000u + 0x00600800u))
/* the swap's wait by design, in timestamp counts, since video_begin's
 * start: because at 720p every frame waits for its swap, host.c leaves it
 * out of its hitches, which would fill the log */
unsigned long long video_swap_wait_tsc;
static unsigned off_screen_late; /* swaps more than 8 blanks late, for the report */

static void wait_off_screen(void)
{
    ULONG back = MmGetPhysicalAddress(pb_back_buffer()) & 0x0fffffffu;
    unsigned long long t0 = __builtin_ia32_rdtsc();
    int tries;

    /* until the swap, however late: because a cap of 8 blanks let a heavy
     * frame's next frame draw on the picture on the screen, a flash, it
     * waits up to 60 blanks, a second, a limit for a swap that never comes */
    for (tries = 0; tries < 60 && (DISPLAY_START & 0x0fffffffu) == back; tries++) {
        pb_wait_for_vbl();
    }
    if (tries > 1) {
        off_screen_waits++;
    }
    if (tries > 8) {
        off_screen_late++;
        if (off_screen_late <= 32) {
            /* where the display starts and the back buffer pbkit names: the
             * same at a swap that never came says pbkit's back buffer is
             * stale (nxdk-pbkit-lost-swap.patch's fault) */
            host_log_line("host: video: the swap came %d blanks after the frame%s, video frame %u (display at "
                          "0x%08lx, back buffer 0x%08lx)\n",
                          tries, tries >= 60 ? ", or never: the next frame draws on the screen's" : "", video_frame,
                          (unsigned long)(DISPLAY_START & 0x0fffffffu), (unsigned long)back);
        }
    }
    video_swap_wait_tsc += __builtin_ia32_rdtsc() - t0;
}

static void show_pending(void)
{
    KIRQL was = KeGetCurrentIrql();
    ULONGLONG before = KeQueryPerformanceCounter();

    flush_pending();
    push_kick();
    if (was > PASSIVE_LEVEL) {
        KfLowerIrql(PASSIVE_LEVEL);
    }
    if (frame_pending) {
        finish_frame();
        /* a shot of the frame, the back buffer finished */
        if (shots_due > 0) {
            while (pb_busy()) {
            }
            for (; shots_due > 0; shots_due--) {
                take_shot(shot_frames[shots_done], shot_buffers[shots_done]);
                shots_done++;
            }
        }
        /* it turns to the next back buffer itself */
        while (pb_finished()) {
        }
        frame_pending = 0;
        shown_queued_at = pb_get_vbl_counter();
    } else {
        pb_target_back_buffer();
    }
    while (pb_busy()) {
    }
    if (was > PASSIVE_LEVEL) {
        KfRaiseIrql(was);
    }
    video_blank_wait_ticks += KeQueryPerformanceCounter() - before;
}

/* pbkit's back buffer as the surface, without pbkit's software interrupts
 * (show_pending): through DMA 3, all of memory, as the scene's surface is,
 * its depth the scene's, which the passes on it neither test nor write */
static void target_back_buffer(uint32_t **pp)
{
    uint32_t *p = *pp;

    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, 3);
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_ZETA, 3);
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 MASK(NV097_SET_SURFACE_PITCH_COLOR, pb_back_buffer_pitch()) | MASK(NV097_SET_SURFACE_PITCH_ZETA, scene_w * 4));
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, MmGetPhysicalAddress(pb_back_buffer()) & 0x03ffffff);
    p = pb_push1(p, NV097_SET_SURFACE_ZETA_OFFSET, MmGetPhysicalAddress(scene_depth) & 0x03ffffff);
    p = pb_push1(p, NV097_SET_SURFACE_FORMAT,
                 MASK(NV097_SET_SURFACE_FORMAT_COLOR, out_color_format()) |
                     MASK(NV097_SET_SURFACE_FORMAT_ZETA, NV097_SET_SURFACE_FORMAT_ZETA_Z24S8) |
                     MASK(NV097_SET_SURFACE_FORMAT_TYPE, NV097_SET_SURFACE_FORMAT_TYPE_PITCH));
    *pp = p;
}

/* from the host's chores, each blank: a frame no other has followed for
 * three blanks is shown, once the NV2A has finished it */
void video_idle_present(void)
{
    if (frame_pending && pending_blanks + 1 >= 3 && open_block == NULL && !pb_busy() && frame_unfinished) {
        finish_frame();
    }
    if (frame_pending && ++pending_blanks >= 3 && open_block == NULL && !pb_busy() && pb_finished() == 0) {
        frame_pending = 0;
        shown_queued_at = pb_get_vbl_counter();
    }
}

void video_begin(unsigned clear)
{
    uint32_t *p;
    uint32_t *start;

    video_swap_wait_tsc = 0;
    wait_idle();
    /* the frames before are drawn: their textures' blocks may go */
    video_frame++;
    if (ring_used > most_ring) {
        most_ring = ring_used;
    }
    if (pushed > most_pushed) {
        most_pushed = pushed;
    }
    pb_reset();
    show_pending();
    /* the last picture's parts are all drawn, its resolve and bloom with
     * them: this one's begin */
    gpu_picture_end();
    gpu_part_set(GPU_CLEAR * 2);
    pushed = 0;
    ring_used = 0;
    block_count = 0;
    /* the models written since the last frame are this frame's: the port
     * writes the next frame's into the other arena. a transform's address
     * says nothing across frames */
    arena_write ^= 1;
    model_rows_from = NULL;
    arrays_at = NULL;
    last_program = -1;
    last_texture = ~0u, constants_stale();
    last_fog_stage = -1;
    last_fog_factor = 1;
    last_blend = last_depth = last_alpha = last_clip = -1;
    last_cull = -1;
    target_scene();
    p = start = push_begin();
    push_control(&p);
    {
        /* the vertex programs' c[78]: the game's pixels to the display's */
        const float scale[4] = {(float)out_w / (float)WIDTH, (float)out_h / (float)HEIGHT, 1.0F, 1.0F};

        push_constants(&p, 78, scale, 1);
    }
    /* R5G6B5 at 720p: dithered, as the PowerVR's 16-bit frames are */
    p = pb_push1(p, NV097_SET_DITHER_ENABLE, out_16);
    p = pb_push1(p, NV097_SET_SURFACE_CLIP_HORIZONTAL, out_w << 16);
    p = pb_push1(p, NV097_SET_SURFACE_CLIP_VERTICAL, out_h << 16);
    p = pb_push1(p, NV097_SET_COLOR_CLEAR_VALUE, 0xff000000u | clear);
    p = pb_push1(p, NV097_SET_ZSTENCIL_CLEAR_VALUE, 0); /* rhw 0, the farthest */
    p = pb_push1(p, NV097_SET_CLEAR_RECT_HORIZONTAL, (out_w - 1) << 16);
    p = pb_push1(p, NV097_SET_CLEAR_RECT_VERTICAL, (out_h - 1) << 16);
    p = pb_push1(p, NV097_CLEAR_SURFACE,
                 NV097_CLEAR_SURFACE_Z | NV097_CLEAR_SURFACE_STENCIL | NV097_CLEAR_SURFACE_COLOR);
    push_end(start, p);
}

static unsigned log2_up(int n)
{
    unsigned k = 0;

    while ((1 << k) < n) {
        k++;
    }
    return k;
}

/* room for bytes in the pool, at a 256-byte boundary as the NV2A's
 * textures start: a block's number, or -1 where nothing more can be
 * evicted. the first gap that fits, else the least lately drawn block that
 * the frame being made hasn't drawn goes, and the gaps are looked at again */
static int block_take(unsigned bytes, int entry, int pinned)
{
    unsigned need = (bytes + 255) & ~255u;

    if (need > pool_size) {
        return -1;
    }
    for (;;) {
        unsigned end = 0;
        int prev = -1, at = block_first;

        while (at >= 0 && blocks[at].offset - end < need) {
            end = blocks[at].offset + blocks[at].size;
            prev = at;
            at = blocks[at].next;
        }
        if (at >= 0 || pool_size - end >= need) {
            int b;

            for (b = 0; b < BLOCKS && blocks[b].in_use; b++) {
            }
            if (b == BLOCKS) {
                return -1;
            }
            blocks[b].offset = end;
            blocks[b].size = need;
            blocks[b].last_used = 0;
            blocks[b].entry = entry;
            blocks[b].pinned = pinned;
            blocks[b].in_use = 1;
            blocks[b].next = at;
            if (prev < 0) {
                block_first = b;
            } else {
                blocks[prev].next = b;
            }
            pool_used += need;
            return b;
        }
        {
            /* none fits: the least lately drawn goes, a texture read as the
             * game loaded it and not drawn since among the first */
            int best = -1;

            for (at = block_first; at >= 0; at = blocks[at].next) {
                if (!blocks[at].pinned && blocks[at].last_used < video_frame &&
                    (best < 0 || blocks[at].last_used < blocks[best].last_used)) {
                    best = at;
                }
            }
            if (best < 0) {
                return -1;
            }
            block_evict(best);
        }
    }
}

static void block_evict(int b)
{
    int prev = -1, at;
    unsigned k;

    for (at = block_first; at >= 0 && at != b; at = blocks[at].next) {
        prev = at;
    }
    if (at < 0) {
        return;
    }
    if (prev < 0) {
        block_first = blocks[b].next;
    } else {
        blocks[prev].next = blocks[b].next;
    }
    if (blocks[b].entry >= 0 && pack_slot != NULL) {
        pack_slot[blocks[b].entry] = 0;
    }
    /* the ids that named it: a texture of the pack keeps its entry, and is
     * read again as it's next drawn */
    for (k = 0; k < TEXTURES; k++) {
        if (textures[k].block == b + 1) {
            textures[k].texels = NULL;
            textures[k].block = 0;
            if (last_texture == k) {
                last_texture = ~0u, constants_stale();
            }
        }
    }
    pool_used -= blocks[b].size;
    blocks[b].in_use = 0;
    texture_evictions++;
}

/* every texture dropped, as when nothing more can be evicted: in the
 * NV2A's own time, and the renderer told to send its own again */
static void textures_drop_all(void)
{
    wait_idle();
    memset(textures, 0, sizeof textures);
    pack_forget();
    textures_lost = 1;
    texture_drops++, texture_drops_all++;
    last_texture = ~0u, constants_stale();
}

/* memory for the host, from the textures' pool, where the kernel has none
 * left: because video memory's backing (host.c's vram_commit) can fail
 * once the pool holds every texture, and the renderer then faulted, every
 * texture is dropped and the pool taken again with bytes and 1 MB more
 * left free. 0 where there's no pool to give */
int video_give_memory(unsigned bytes)
{
    if (pool == NULL) {
        return 0;
    }
    textures_drop_all();
    MmFreeContiguousMemory(pool);
    pool = NULL;
    pool_given += bytes + 0x100000;
    pool_take();
    host_log_line("host: video: the textures' pool gave memory back: %u KB for textures now\n", pool_size >> 10);
    return 1;
}

/* the movie player's room, lent from the textures' (host_api's
 * movie_memory): the pool goes, the room is taken, and the pool comes back
 * in what's left, its textures dropped, and the other way at 0 */
void *video_movie_memory(unsigned size)
{
    static void *room;

    wait_idle();
    memset(textures, 0, sizeof textures);
    pack_forget();
    textures_lost = 1;
    last_texture = ~0u, constants_stale();
    if (pool != NULL) {
        MmFreeContiguousMemory(pool);
        pool = NULL;
    }
    if (room != NULL) {
        MmFreeContiguousMemory(room);
        room = NULL;
    }
    if (size != 0) {
        room = MmAllocateContiguousMemoryEx(size, 0, 0x03ffffff, 0x1000, PAGE_READWRITE);
        if (room == NULL) {
            host_log_line("host: video: no %u KB for the movie\n", size >> 10);
        }
    }
    pool_take();
    host_log_line("host: video: %u KB for textures, %s\n", pool_size >> 10,
                  room != NULL ? "a movie playing" : "no movie");
    return room;
}

int video_textures_lost(void)
{
    int lost = textures_lost;

    textures_lost = 0;
    return lost;
}

void video_texture(unsigned id, int width, int height, const unsigned *argb)
{
    struct texture *t;
    unsigned log_w = log2_up(width), log_h = log2_up(height);
    unsigned bytes = (2u << log_w) << log_h;
    unsigned any_partial = 0, any_clear = 0;
    unsigned format;
    USHORT *to;
    int x, y;

    if (id == 0 || id >= TEXTURES || bytes > pool_size) {
        return;
    }
    texture_uploads++;
    t = &textures[id];
    if (t->shared) {
        /* the pack's texels, which aren't this id's to write: room of its own */
        t->texels = NULL;
        t->shared = 0;
        t->block = 0;
    }
    t->pack_index = 0;
    if (t->texels == NULL || t->bytes < bytes) {
        int b;

        /* its old block, if any, is the pool's to take back once no frame
         * in flight draws it */
        if (t->block != 0) {
            blocks[t->block - 1].pinned = 0;
            blocks[t->block - 1].last_used = video_frame;
            t->block = 0;
        }
        t->texels = NULL;
        b = block_take(bytes, -1, 1);
        if (b < 0) {
            textures_drop_all();
            b = block_take(bytes, -1, 1);
            if (b < 0) {
                return;
            }
        }
        t->texels = pool + blocks[b].offset;
        t->block = b + 1;
        t->bytes = bytes;
        texture_uploads_new++;
    } else if (last_texture == id) {
        /* the NV2A may still read its old texels */
        wait_idle();
    }
    for (y = 0; y < width * height; y++) {
        unsigned a = argb[y] >> 24;

        any_clear |= a == 0;
        any_partial |= a != 0 && a != 255;
    }
    format = any_partial ? NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A4R4G4B4
             : any_clear ? NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A1R5G5B5
                         : NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R5G6B5;
    /* the pool is write-combined, which takes writes in order: so the walk is
     * in the swizzled order, each texel's coordinates from its offset */
    to = (USHORT *)t->texels;
    {
        unsigned n = 1u << (log_w + log_h);
        unsigned i;

        for (i = 0; i < n; i++) {
            unsigned bit = 0;
            unsigned k;
            unsigned c;

            x = y = 0;
            for (k = 0; k < log_w || k < log_h; k++) {
                if (k < log_w) {
                    x |= (int)(((i >> bit++) & 1) << k);
                }
                if (k < log_h) {
                    y |= (int)(((i >> bit++) & 1) << k);
                }
            }
            c = argb[(y < height ? y : height - 1) * width + (x < width ? x : width - 1)];
            unsigned r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255, a = c >> 24;
            USHORT texel;

            if (format == NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R5G6B5) {
                texel = (USHORT)((r >> 3) << 11 | (g >> 2) << 5 | b >> 3);
            } else if (format == NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A1R5G5B5) {
                texel = (USHORT)((a ? 0x8000 : 0) | (r >> 3) << 10 | (g >> 3) << 5 | b >> 3);
            } else {
                texel = (USHORT)((a >> 4) << 12 | (r >> 4) << 8 | (g >> 4) << 4 | b >> 4);
            }
            to[i] = texel;
        }
    }
    t->format = format;
    t->log_w = log_w;
    t->log_h = log_h;
    t->levels = 1;
    t->u_scale = (float)width / (float)(1 << log_w);
    t->v_scale = (float)height / (float)(1 << log_h);
    if (last_texture == id) {
        last_texture = ~0u, constants_stale();
    }
}

/* the pack's header and entries, where the disc has one; without it, the
 * renderer decodes every texture */
static void pack_open(void)
{
    struct texpack_header header;
    DWORD got;

    pack_file = CreateFileA("D:\\textures.pak", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, NULL);
    if (pack_file == INVALID_HANDLE_VALUE) {
        host_log_line("host: video: no D:\\textures.pak: every texture decoded as it's drawn\n");
        return;
    }
    if (!ReadFile(pack_file, &header, sizeof header, &got, NULL) || got != sizeof header ||
        header.magic != TEXPACK_MAGIC || header.count > 0x100000) {
        host_log_line("host: video: D:\\textures.pak isn't a pack\n");
        return;
    }
    pack_entries = malloc(header.count * sizeof *pack_entries);
    if (pack_entries == NULL || !ReadFile(pack_file, pack_entries, header.count * sizeof *pack_entries, &got, NULL) ||
        got != header.count * sizeof *pack_entries) {
        host_log_line("host: video: the pack's entries don't read\n");
        return;
    }
    pack_count = header.count;
    pack_slot = calloc(pack_count, sizeof *pack_slot);
    host_log_line("host: video: %u textures in the pack\n", pack_count);
}

static const struct texpack_entry *pack_find(unsigned key_lo, unsigned key_hi)
{
    unsigned low = 0, high = pack_count;

    while (low < high) {
        unsigned mid = (low + high) / 2;
        const struct texpack_entry *m = &pack_entries[mid];

        if (m->hi == key_hi && m->lo == key_lo) {
            return m;
        }
        if (m->hi < key_hi || (m->hi == key_hi && m->lo < key_lo)) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    return NULL;
}

/* an entry's texels in the pool, read from the pack where they aren't
 * there yet: into new room, never over texels the NV2A may still read. the
 * pool full, every texture is dropped first. at PASSIVE_LEVEL, for the read */
static UCHAR *pack_load(const struct texpack_entry *e)
{
    unsigned index = (unsigned)(e - pack_entries);
    UCHAR *at;
    DWORD got;
    int b;

    if (pack_slot != NULL && pack_slot[index] != 0) {
        return pool + blocks[pack_slot[index] - 1].offset;
    }
    if (e->bytes > pool_size || e->format > TEXPACK_DXT3) {
        return NULL;
    }
    b = block_take(e->bytes, (int)index, 0);
    if (b < 0) {
        textures_drop_all();
        b = block_take(e->bytes, (int)index, 0);
        if (b < 0) {
            return NULL;
        }
    }
    at = pool + blocks[b].offset;
    if (SetFilePointer(pack_file, (LONG)e->offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER ||
        !ReadFile(pack_file, at, e->bytes, &got, NULL) || got != e->bytes) {
        static int logged;

        if (!logged) {
            logged = 1;
            host_log_line("host: video: a texture of the pack doesn't read\n");
        }
        block_evict(b);
        texture_evictions--;
        return NULL;
    }
    texture_preloads++;
    if (pack_slot != NULL) {
        pack_slot[index] = b + 1;
    }
    return at;
}

/* kmLoadTexture's question, as the game loads a texture: the pack's
 * texels are read now, in the game's load, not at the texture's first draw */
int video_texture_known(unsigned key_lo, unsigned key_hi)
{
    const struct texpack_entry *e = pack_find(key_lo, key_hi);

    if (e == NULL) {
        return 0;
    }
    pack_load(e);
    return 1;
}

int video_texture_packed(unsigned id, unsigned key_lo, unsigned key_hi)
{
    static const unsigned formats[5] = {
        NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R5G6B5,       NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A1R5G5B5,
        NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A4R4G4B4,     NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT1_A1R5G5B5,
        NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT23_A8R8G8B8,
    };
    const struct texpack_entry *e = pack_find(key_lo, key_hi);
    struct texture *t;
    UCHAR *texels;

    if (e == NULL || id == 0 || id >= TEXTURES) {
        return 0;
    }
    texels = pack_load(e);
    if (texels == NULL) {
        return 0;
    }
    /* the id names the resident texels: what it named before stays where
     * it was, for the NV2A to finish with */
    t = &textures[id];
    if (t->block != 0 && !t->shared) {
        /* a decoded texture's own block, the pool's again */
        blocks[t->block - 1].pinned = 0;
        blocks[t->block - 1].last_used = video_frame;
    }
    t->texels = texels;
    t->bytes = e->bytes;
    t->shared = 1;
    t->block = pack_slot != NULL ? pack_slot[e - pack_entries] : 0;
    t->pack_index = (int)(e - pack_entries) + 1;
    if (t->block != 0) {
        /* the frame being made draws it: not to be evicted before then */
        blocks[t->block - 1].last_used = video_frame;
    }
    t->format = formats[e->format];
    t->log_w = e->log_w;
    t->log_h = e->log_h;
    t->levels = e->levels;
    t->u_scale = t->v_scale = 1.0F;
    t->pitch = 0;
    if (last_texture == id) {
        last_texture = ~0u, constants_stale();
    }
    texture_packed++;
    return 1;
}

void video_fog(unsigned table_color, unsigned vertex_color, float density, const unsigned short table[128])
{
    if (memcmp(fog_table, table, sizeof fog_table) != 0) {
        memcpy(fog_table, table, sizeof fog_table);
        fog_constants = 1;
    }
    if (density != fog_density) {
        constants_stale();
    }
    fog_table_color = table_color;
    fog_vertex_color = vertex_color;
    fog_density = density;
}

/* the depth the NV2A compares for rhw: rhw times a scale, in the fixed
 * 24-bit depth buffer. linear in rhw, it interpolates across the screen as
 * the PowerVR interpolates rhw, so that a decal on a surface ties or wins as
 * there (but for a triangle across the knee). it is in two pieces, fitted
 * to the last frame's rhw (depth_next): 15/16 of the depth up to a knee well
 * above 99.9% of the vertices, the rest up to the largest, as a frame's
 * one strip at rhw 1 among geometry under 0.01 (Station Square) would
 * otherwise leave the geometry 1/100 of the depth, and the awning's stripes
 * equal to the awning under them. the
 * PowerVR compares rhw as a 32-bit float, and the game orders 2D layers by
 * rhw that differ in the sixth digit (the file screen's boxes, 1.695154e-5
 * against their frame's 1.695145e-5). two mappings that failed: rhw in the
 * NV2A's float depth, with 16 bits of mantissa (and less under xemu), held
 * those layers equal and lost buildings behind the sky; rhw's float bits,
 * 19 bits an octave, linear only within an octave, broke decals on surfaces
 * whose corners lie in other octaves (Station Square's crosswalk). below
 * the two, a third piece holds the rhw under DEPTH_BOTTOM_RHW, past any
 * far clip, where the game draws 2D layers: the trial menu's, whose frames
 * the two pieces put on the items' depth */
static float depth_knee = 1.0F; /* the rhw where the steeper piece ends */
static float depth_low = (16777215.0F - 16777215.0F / 8.0F) / (1.0F - 3.0e-5F); /* its scale */
static float depth_high = 0.0F; /* the other piece's, above the knee */
static float depth_most; /* this frame's largest rhw */
/* the bottom piece, below DEPTH_BOTTOM_RHW: an eighth of the depth for the
 * 2D layers the game draws far behind any 3D (z past 33,333, where the far
 * clip is 10,000 at most). the trial menu's list is at rhw 1.7244e-5, its
 * items' frames 8.9e-11 behind the items: because on one depth value the
 * frames drew over the items, the piece's steps are 1.4e-11 */
#define DEPTH_BOTTOM_RHW 3.0e-5F
#define DEPTH_BOTTOM_Z (16777215.0F / 8.0F)
static float depth_bottom = DEPTH_BOTTOM_RHW, depth_bottom_scale = DEPTH_BOTTOM_Z / DEPTH_BOTTOM_RHW,
             depth_bottom_z = DEPTH_BOTTOM_Z;
static unsigned depth_octaves[256]; /* this frame's vertices by rhw's exponent */
static unsigned depth_count;

#define DEPTH_TOP 16777215.0F
#define DEPTH_KNEE_Z (DEPTH_TOP * 15.0F / 16.0F)

/* a vertex's rhw, for the next frame's mapping: the vertex program maps it
 * (shaders/vertex.vp), by the constants depth_next sets */
static void depth_note(float rhw)
{
    union {
        float f;
        uint32_t u;
    } bits;

    bits.f = rhw;
    depth_octaves[(bits.u >> 23) & 255]++;
    depth_count++;
    if (rhw > depth_most) {
        depth_most = rhw;
    }
}

/* at a frame's end, the next's mapping: the knee three octaves above the
 * top of the octave that holds 99.9% of the frame's vertices, with 15/16 of
 * the depth below
 * it, the rest up to the largest rhw and a quarter more. a frame whose
 * largest is near the knee has one piece */
static void depth_next(void)
{
    unsigned seen = 0;
    int e;

    if (depth_count != 0 && depth_most > 0.0F) {
        union {
            float f;
            uint32_t u;
        } knee;
        float most = depth_most * 1.25F;

        for (e = 0; e < 255; e++) {
            seen += depth_octaves[e];
            if (seen * 1000ull >= depth_count * 999ull) {
                break;
            }
        }
        /* 2^(e + 4 - 127): three octaves above that octave's top, so that
         * few triangles cross it, as the nearest geometry at the octave's top
         * did (Station Square's awning, rhw 0.0069 to 0.0082, at 2^-7) */
        knee.u = (uint32_t)(e + 4 < 254 ? e + 4 : 254) << 23;
        /* the bottom piece, where the lower piece holds well above it */
        float bottom = (knee.f >= most * 0.5F ? most : knee.f) > DEPTH_BOTTOM_RHW * 4.0F
                           ? DEPTH_BOTTOM_RHW
                           : 0.0F;

        depth_bottom = bottom;
        depth_bottom_z = bottom != 0.0F ? DEPTH_BOTTOM_Z : 0.0F;
        depth_bottom_scale = bottom != 0.0F ? DEPTH_BOTTOM_Z / bottom : 0.0F;
        /* the lower piece from the bottom piece's top */
        if (knee.f >= most * 0.5F) {
            depth_knee = most;
            depth_low = (DEPTH_TOP - depth_bottom_z) / (most - bottom);
            depth_high = 0.0F;
        } else {
            depth_knee = knee.f;
            depth_low = (DEPTH_KNEE_Z - depth_bottom_z) / (knee.f - bottom);
            depth_high = (DEPTH_TOP - DEPTH_KNEE_Z) / (most - knee.f);
        }
    }
    memset(depth_octaves, 0, sizeof depth_octaves);
    depth_count = 0;
    depth_most = 0.0F;
    frame_constants = 1;
}

/* the vertex program's constants (shaders/vertex.vp's head), from c[n] at
 * 96 + n, eight a command */
static void push_constants(uint32_t **pp, unsigned n, const float *v, unsigned count)
{
    uint32_t *p = *pp;
    unsigned k;

    p = pb_push1(p, NV097_SET_TRANSFORM_CONSTANT_LOAD, 96 + n);
    for (k = 0; k < count; k += 8) {
        unsigned m = count - k > 8 ? 8 : count - k;

        pb_push(p++, NV097_SET_TRANSFORM_CONSTANT, m * 4);
        memcpy(p, &v[k * 4], m * 16);
        p += m * 4;
    }
    *pp = p;
}

/* the fog table's texture, as the table changes: texel j the table's value
 * at rhw times the density of j / 16, held to the table's 1 to 256. the
 * NV2A may be reading it, so it finishes what it has first */
static void fill_fog_texture(void)
{
    int j;

    wait_idle();
    for (j = 0; j < FOG_TEXELS; j++) {
        int z16 = j < 16 ? 16 : j; /* z times 16 */
        int e = 0;
        int index, part;
        int high, low;

        while ((32 << e) <= z16) {
            e++;
        }
        /* z = 2^e * (1 + (step + part / 2^e) / 16) */
        index = e * 16 + ((z16 >> e) - 16);
        part = z16 & ((1 << e) - 1);
        high = fog_table[index] >> 8;
        low = fog_table[index] & 255;
        fog_texels[j] = (UCHAR)(high + (((low - high) * part + (1 << e) / 2) >> e));
    }
}

/* a frame's: c[0] to c[2], the depth's mapping; and the fog table's
 * texture, as the table changes */
static void push_frame_constants(uint32_t **pp)
{
    float c[3 * 4];

    if (frame_constants) {
        c[0] = 1.0F, c[1] = 0.0F, c[2] = 0.5F, c[3] = 16.0F;
        c[4] = depth_knee, c[5] = depth_low, c[6] = depth_high, c[7] = DEPTH_KNEE_Z;
        c[8] = DEPTH_TOP, c[9] = 0.0F, c[10] = 1.0e-7F, c[11] = 255.9999F;
        push_constants(pp, 0, c, 3);
        c[0] = depth_bottom, c[1] = depth_bottom_scale, c[2] = depth_bottom_z, c[3] = 0.0F;
        push_constants(pp, 6, c, 1);
        frame_constants = 0;
    }
}

/* stage 1's texture: the fog's table, which, or its ramp */
static void push_fog_stage(uint32_t **pp, int which)
{
    uint32_t *p = *pp;
    const UCHAR *texels = which == 1 ? fog_texels : fog_texels + FOG_TEXELS;
    unsigned log_w = which == 1 ? 12 : 8;

    p = pb_push1(p, NV097_SET_TEXTURE_OFFSET + 64, MmGetPhysicalAddress((void *)texels) & 0x03ffffff);
    p = pb_push1(p, NV097_SET_TEXTURE_FORMAT + 64,
                 MASK(NV097_SET_TEXTURE_FORMAT_CONTEXT_DMA, 1) |
                     MASK(NV097_SET_TEXTURE_FORMAT_BORDER_SOURCE, NV097_SET_TEXTURE_FORMAT_BORDER_SOURCE_COLOR) |
                     MASK(NV097_SET_TEXTURE_FORMAT_DIMENSIONALITY, 2) |
                     MASK(NV097_SET_TEXTURE_FORMAT_COLOR, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8) |
                     MASK(NV097_SET_TEXTURE_FORMAT_MIPMAP_LEVELS, 1) |
                     MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_U, log_w) |
                     MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_V, 0) |
                     MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_P, 0));
    /* clamped, and bilinear, which blends between the table's entries */
    p = pb_push1(p, NV097_SET_TEXTURE_ADDRESS + 64, 3 | 3 << 8 | 1 << 16);
    p = pb_push1(p, NV097_SET_TEXTURE_CONTROL0 + 64, NV097_SET_TEXTURE_CONTROL0_ENABLE | 0x0003ffc0);
    p = pb_push1(p, NV097_SET_TEXTURE_FILTER + 64, 0x02022000);
    *pp = p;
}


static unsigned blend_factor(int code, int is_src)
{
    static const unsigned factors[8] = {
        NV097_SET_BLEND_FUNC_SFACTOR_V_ZERO,          NV097_SET_BLEND_FUNC_SFACTOR_V_ONE,
        NV097_SET_BLEND_FUNC_SFACTOR_V_DST_COLOR,     NV097_SET_BLEND_FUNC_SFACTOR_V_ONE_MINUS_DST_COLOR,
        NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA,     NV097_SET_BLEND_FUNC_SFACTOR_V_ONE_MINUS_SRC_ALPHA,
        NV097_SET_BLEND_FUNC_SFACTOR_V_DST_ALPHA,     NV097_SET_BLEND_FUNC_SFACTOR_V_ONE_MINUS_DST_ALPHA,
    };

    if (!is_src && code == 2) {
        return NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_COLOR;
    }
    if (!is_src && code == 3) {
        return NV097_SET_BLEND_FUNC_SFACTOR_V_ONE_MINUS_SRC_COLOR;
    }
    return factors[code & 7];
}

/* the window clip, in the target's pixels: inside the rectangle (mode 2)
 * or outside it (3). the NV2A's eight rectangles, of which the rest are
 * empty */
static void push_clip_pixels(uint32_t **pp, int mode, unsigned x0, unsigned y0, unsigned x1, unsigned y1)
{
    uint32_t *p = *pp;
    int k;

    p = pb_push1(p, NV097_SET_WINDOW_CLIP_TYPE, mode == 3 ? 1 : 0);
    {
        /* the rectangle in all eight of the NV2A's, which change nothing as
         * copies. a real NV2A takes an empty one, min past max, for invalid
         * data and halts (xemu doesn't): an empty clip is a pixel at 0, 0 */
        unsigned h = 0, v = 0;

        if (x1 > x0 && y1 > y0) {
            h = x0 | (x1 - 1) << 16;
            v = y0 | (y1 - 1) << 16;
        }
        for (k = 0; k < 8; k++) {
            p = pb_push1(p, NV097_SET_WINDOW_CLIP_HORIZONTAL + k * 4, h);
            p = pb_push1(p, NV097_SET_WINDOW_CLIP_VERTICAL + k * 4, v);
        }
    }
    *pp = p;
}

/* the window clip of a draw: inside the rectangle, outside it, or the
 * whole screen, the rectangle in the game's pixels, which 720p doubles
 * across and takes one and a half times down */
static void push_clip(uint32_t **pp, int mode, unsigned x0, unsigned y0, unsigned x1, unsigned y1)
{
    if (mode != 2 && mode != 3) {
        mode = 2;
        x0 = y0 = 0;
        x1 = WIDTH;
        y1 = HEIGHT;
    }
    x1 = x1 > WIDTH ? WIDTH : x1;
    y1 = y1 > HEIGHT ? HEIGHT : y1;
    push_clip_pixels(pp, mode, x0 * out_w / WIDTH, y0 * out_h / HEIGHT, x1 * out_w / WIDTH, y1 * out_h / HEIGHT);
}

/* the texture a draw binds, its block marked drawn this frame, or null:
 * a texture of the pack whose block was evicted is read again here, at
 * PASSIVE_LEVEL for the read, as wait_idle lowers it */
static UCHAR *pack_load(const struct texpack_entry *e);

static const struct texture *texture_bound(unsigned id)
{
    struct texture *t;

    if (id == 0 || id >= TEXTURES) {
        return NULL;
    }
    t = &textures[id];
    if (t->texels == NULL && t->pack_index != 0) {
        KIRQL was = KeGetCurrentIrql();
        UCHAR *texels;

        flush_pending();
        push_kick();
        if (was > PASSIVE_LEVEL) {
            KfLowerIrql(PASSIVE_LEVEL);
        }
        texels = pack_load(&pack_entries[t->pack_index - 1]);
        if (was > PASSIVE_LEVEL) {
            KfRaiseIrql(was);
        }
        if (texels != NULL) {
            t->texels = texels;
            t->block = pack_slot[t->pack_index - 1];
            texture_reloads++;
        }
    }
    if (t->texels == NULL) {
        return NULL;
    }
    if (t->block != 0) {
        blocks[t->block - 1].last_used = video_frame;
    }
    return t;
}

/* texel (u, v) of a texture 256 by 2, swizzled: u's bit 0, then v's, then
 * u's others */
static unsigned lit_texel(unsigned u, unsigned v)
{
    return (u & 1) | v << 1 | (u >> 1) << 2;
}

/* the texture that holds palette's 512 words (host.h's host_item) */
static int lit_texture(const unsigned *palette)
{
    int k;
    int oldest = 0;

    for (k = 0; k < LIT_TEXTURES; k++) {
        if (lit_from[k] == palette && lit_frame[k] == video_frame) {
            return k;
        }
    }
    for (k = 0; k < LIT_TEXTURES; k++) {
        if (lit_frame[k] != 0 && memcmp(lit_words[k], palette, sizeof lit_words[k]) == 0) {
            lit_from[k] = palette;
            lit_frame[k] = video_frame;
            return k;
        }
        if (lit_frame[k] < lit_frame[oldest]) {
            oldest = k;
        }
    }
    /* the one drawn with longest ago, which no draw of this frame has
     * taken unless all of them have: its texels then change under the
     * frame's earlier draws, which the NV2A may not have drawn */
    k = oldest;
    if (lit_frame[k] == video_frame) {
        lit_overwrites++;
    }
    if (lit_frame_counted != video_frame) {
        lit_frame_counted = video_frame;
        lit_this_frame = 0;
    }
    if (++lit_this_frame > lit_most_frame) {
        lit_most_frame = lit_this_frame;
    }
    memcpy(lit_words[k], palette, sizeof lit_words[k]);
    {
        uint32_t *texels = lit_texels + k * 512;
        unsigned u;

        for (u = 0; u < 256; u++) {
            texels[lit_texel(u, 0)] = palette[u];
            texels[lit_texel(u, 1)] = palette[256 + u];
        }
    }
    lit_from[k] = palette;
    lit_frame[k] = video_frame;
    lit_uploads++;
    if (last_lit == k) {
        last_lit = -1;
    }
    return k;
}

static void draw_batch(const struct host_draw *draw, const unsigned short *indices, int count, unsigned base, int model,
                       const struct host_vertex *room)
{
    /* the rows that leave a vertex on the screen where it is, and say its
     * third coordinate is rhw */
    static const float flat_rows[16] = {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
                                        0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F};
    int cull = model && (draw->cull == 2 || draw->cull == 3) ? draw->cull : 0;
    unsigned want_rows = model ? rows_id : 0;
    int new_rows = want_rows != last_rows_id;
    const struct texture *t = texture_bound(draw->texture);
    const struct host_item *lit = model ? lit_item : NULL;
    int program = (lit != NULL ? PROGRAM_LIT : 0) +
                  (draw->fog == 3 ? PROGRAM_FOG_POLYGON
                   : t == NULL    ? 0
                                  : 1 + (draw->shading & 3) * 2 + (draw->ignore_texture_alpha ? 1 : 0));
    int vertex_program = lit != NULL;
    int lit_index = lit != NULL ? lit_texture(lit->palette) : last_lit;
    int new_light = lit != NULL && (last_vertex_program != 1 || lit->light[0] != last_light[0] ||
                                    lit->light[1] != last_light[1] || lit->light[2] != last_light[2]);
    /* the fog: the table's texture and color under modes 0 and 3, the
     * ramp's and the vertex fog's under 1, and the ramp at 0 under 2 */
    int fog_stage = draw->fog == 0 || draw->fog == 3 ? 1 : 2;
    unsigned fog_factor = 0xff000000u | (draw->fog == 1 ? fog_vertex_color : fog_table_color);
    int blend = draw->src_blend << 4 | draw->dst_blend;
    int depth = draw->depth_compare << 1 | draw->depth_write;
    int alpha = draw->punch_through ? (int)(draw->alpha_ref * 255.0F + 0.5F) : -1;
    int clip = draw->clip_mode == 2 || draw->clip_mode == 3 ? draw->clip_mode : 0;
    /* the vertex program's c[3] to c[5]: the texture's scales, the fog's
     * mode, the base color's alpha, the fog's color and the offset color.
     * built only where the key says they may differ from the last draw's */
    unsigned constants_key = (t != NULL ? draw->texture : 0) | (unsigned)(draw->fog & 3) << 12 |
                             (draw->use_alpha ? 1u << 14 : 0) | (draw->use_offset ? 1u << 15 : 0);
    float constants[12];
    int new_constants = 0;
    uint32_t *p;
    uint32_t *start;

    if (count <= 0) {
        return;
    }
    /* the indices' words, and a state's at most */
    make_room((unsigned)count * 2 + 0x1000);
    draws_in++;
    unsigned address_u = draw->clamp_u ? 3 : draw->flip_u ? 2 : 1;
    unsigned address_v = draw->clamp_v ? 3 : draw->flip_v ? 2 : 1;
    unsigned sampling = address_u | address_v << 8 | (draw->filter ? 1u << 16 : 0);
    int changes;

    if (constants_key != last_constants_key || !draw_constants_set) {
        /* a density that isn't a number, or is past one, as none */
        float density = fog_density >= 0.0F && fog_density < 1.0e30F ? fog_density : 0.0F;
        int k;

        constants[0] = t != NULL ? t->u_scale : 1.0F;
        constants[1] = t != NULL ? t->v_scale : 1.0F;
        constants[2] = 0.0F;
        constants[3] = 1.0F;
        if (fog_stage == 1) {
            /* the table's texel j is at j / 16 of rhw times the density, its
             * center half a texel on */
            constants[4] = density * 16.0F / (float)FOG_TEXELS;
            constants[5] = 0.5F / (float)FOG_TEXELS;
            constants[6] = 0.0F;
            constants[8] = 1.0F;
            constants[9] = 0.0F;
        } else {
            /* the ramp's texel j is a share of j / 255 */
            constants[4] = 0.5F / (float)FOG_RAMP;
            constants[5] = 0.0F;
            constants[6] = draw->fog == 1 ? 255.0F / (float)FOG_RAMP : 0.0F;
            constants[8] = 0.0F;
            constants[9] = 1.0F;
        }
        constants[7] = draw->use_alpha ? 0.0F : 1.0F;
        constants[10] = 0.0F;
        constants[11] = draw->use_offset ? 1.0F : 0.0F;
        new_constants = !draw_constants_set;
        for (k = 0; k < 12 && !new_constants; k++) {
            new_constants = constants[k] != last_draw_constants[k];
        }
        last_constants_key = constants_key;
    }
    changes = new_constants || new_rows || cull != last_cull || frame_constants || fog_constants ||
              vertex_program != last_vertex_program || new_light || lit_index != last_lit ||
              fog_stage != last_fog_stage || fog_factor != last_fog_factor || program != last_program ||
                  (t != NULL && (last_texture != draw->texture || last_sampling != sampling)) ||
                  (t == NULL && last_texture != 0) || blend != last_blend || depth != last_depth ||
                  alpha != last_alpha || clip != last_clip ||
                  (clip != 0 && (last_clip_rect[0] != draw->clip_x0 || last_clip_rect[1] != draw->clip_y0 ||
                                 last_clip_rect[2] != draw->clip_x1 || last_clip_rect[3] != draw->clip_y1));

    if (!changes) {
        /* the state the open draw has: this one joins it */
        push_indices(indices, count, base, room);
        return;
    }
    flush_pending();
    if (fog_constants) {
        fill_fog_texture();
        fog_constants = 0;
    }
    p = start = push_begin();
    push_frame_constants(&p);
    if (fog_stage != last_fog_stage) {
        push_fog_stage(&p, fog_stage);
        last_fog_stage = fog_stage;
    }
    if (fog_factor != last_fog_factor) {
        /* the fog's color: the combiners' constant 0, and the last
         * combiner's, which fog mode 3's program reads */
        p = pb_push1(p, NV097_SET_COMBINER_FACTOR0, fog_factor);
        p = pb_push1(p, NV097_SET_SPECULAR_FOG_FACTOR, fog_factor);
        last_fog_factor = fog_factor;
    }
    if (new_constants) {
        int k;

        push_constants(&p, 3, constants, 3);
        for (k = 0; k < 12; k++) {
            last_draw_constants[k] = constants[k];
        }
        draw_constants_set = 1;
    }
    if (new_rows) {
        push_constants(&p, 72, model ? model_rows : flat_rows, 4);
        last_rows_id = want_rows;
    }
    if (cull != last_cull) {
        /* 2 drops the counterclockwise as the screen shows them, the front
         * faces here, 3 the clockwise */
        p = pb_push1(p, NV097_SET_CULL_FACE_ENABLE, cull != 0);
        if (cull != 0) {
            p = pb_push1(p, NV097_SET_FRONT_FACE, NV097_SET_FRONT_FACE_V_CCW);
            p = pb_push1(p, NV097_SET_CULL_FACE,
                         cull == 2 ? NV097_SET_CULL_FACE_V_FRONT : NV097_SET_CULL_FACE_V_BACK);
        }
        last_cull = cull;
    }
    if (program != last_program) {
        programs[program](&p);
        last_program = program;
    }
    if (vertex_program != last_vertex_program) {
        /* vertex.vp's, or a lit model's, shaders/lit.vp */
        p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, vertex_program ? LIT_SLOT : 0);
        last_vertex_program = vertex_program;
    }
    if (new_light) {
        /* lit.vp's c[76] and c[77]: the light, and the palette's place */
        const float light[8] = {lit->light[0], lit->light[1], lit->light[2], 0.0F,
                                127.5F / 256.0F, 127.5F / 256.0F, 0.25F, 0.75F};

        push_constants(&p, 76, light, 2);
        last_light[0] = lit->light[0];
        last_light[1] = lit->light[1];
        last_light[2] = lit->light[2];
    }
    if (lit != NULL && lit_index != last_lit) {
        /* the palette on stages 2 and 3, each a row of it: clamped, and
         * bilinear, which blends between entries as the PowerVR blends
         * between vertices */
        int k;

        for (k = 2; k < 4; k++) {
            p = pb_push1(p, NV097_SET_TEXTURE_OFFSET + 64 * k,
                         MmGetPhysicalAddress(lit_texels + lit_index * 512) & 0x03ffffff);
            p = pb_push1(p, NV097_SET_TEXTURE_FORMAT + 64 * k,
                         MASK(NV097_SET_TEXTURE_FORMAT_CONTEXT_DMA, 1) |
                             MASK(NV097_SET_TEXTURE_FORMAT_BORDER_SOURCE, NV097_SET_TEXTURE_FORMAT_BORDER_SOURCE_COLOR) |
                             MASK(NV097_SET_TEXTURE_FORMAT_DIMENSIONALITY, 2) |
                             MASK(NV097_SET_TEXTURE_FORMAT_COLOR, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8) |
                             MASK(NV097_SET_TEXTURE_FORMAT_MIPMAP_LEVELS, 1) |
                             MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_U, 8) |
                             MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_V, 1) |
                             MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_P, 0));
            p = pb_push1(p, NV097_SET_TEXTURE_ADDRESS + 64 * k, 3 | 3 << 8 | 1 << 16);
            p = pb_push1(p, NV097_SET_TEXTURE_CONTROL0 + 64 * k, NV097_SET_TEXTURE_CONTROL0_ENABLE | 0x0003ffc0);
            p = pb_push1(p, NV097_SET_TEXTURE_FILTER + 64 * k, 0x02022000);
        }
        last_lit = lit_index;
    }
    if (lit != NULL) {
        lit_draws++;
    }

    if (t != NULL && (last_texture != draw->texture || last_sampling != sampling)) {
        /* point and bilinear, as nxdk's samples set them, and with
         * mipmaps the nearest level's texel or both levels' blend */
        static const unsigned filters[4] = {0x01012000, 0x02022000, 0x01032000, 0x02062000};

        p = pb_push1(p, NV097_SET_TEXTURE_OFFSET, MmGetPhysicalAddress(t->texels) & 0x03ffffff);
        p = pb_push1(p, NV097_SET_TEXTURE_FORMAT,
                     MASK(NV097_SET_TEXTURE_FORMAT_CONTEXT_DMA, 1) |
                         MASK(NV097_SET_TEXTURE_FORMAT_BORDER_SOURCE, NV097_SET_TEXTURE_FORMAT_BORDER_SOURCE_COLOR) |
                         MASK(NV097_SET_TEXTURE_FORMAT_DIMENSIONALITY, 2) |
                         MASK(NV097_SET_TEXTURE_FORMAT_COLOR, t->format) |
                         MASK(NV097_SET_TEXTURE_FORMAT_MIPMAP_LEVELS, t->levels) |
                         MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_U, t->log_w) |
                         MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_V, t->log_h) |
                         MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_P, 0));
        if (t->pitch != 0) {
            p = pb_push1(p, NV097_SET_TEXTURE_CONTROL1, MASK(NV097_SET_TEXTURE_CONTROL1_IMAGE_PITCH, t->pitch));
            p = pb_push1(p, NV097_SET_TEXTURE_IMAGE_RECT,
                         MASK(NV097_SET_TEXTURE_IMAGE_RECT_WIDTH, t->rect_w) |
                             MASK(NV097_SET_TEXTURE_IMAGE_RECT_HEIGHT, t->rect_h));
        }
        p = pb_push1(p, NV097_SET_TEXTURE_ADDRESS, address_u | address_v << 8 | 1 << 16);
        p = pb_push1(p, NV097_SET_TEXTURE_CONTROL0, NV097_SET_TEXTURE_CONTROL0_ENABLE | 0x0003ffc0);
        p = pb_push1(p, NV097_SET_TEXTURE_FILTER, filters[(draw->filter ? 1 : 0) + (t->levels > 1 ? 2 : 0)]);
        last_texture = draw->texture;
        last_sampling = sampling;
    } else if (t == NULL && last_texture != 0) {
        p = pb_push1(p, NV097_SET_TEXTURE_CONTROL0, 0x0003ffc0);
        last_texture = 0;
    }
    if (blend != last_blend) {
        if (draw->src_blend == 1 && draw->dst_blend == 0) {
            p = pb_push1(p, NV097_SET_BLEND_ENABLE, 0);
        } else {
            p = pb_push1(p, NV097_SET_BLEND_ENABLE, 1);
            p = pb_push1(p, NV097_SET_BLEND_FUNC_SFACTOR, blend_factor(draw->src_blend, 1));
            p = pb_push1(p, NV097_SET_BLEND_FUNC_DFACTOR, blend_factor(draw->dst_blend, 0));
        }
        last_blend = blend;
    }
    if (depth != last_depth) {
        /* the PowerVR's compare on rhw, which the depth buffer holds. an
         * always that writes nothing is no test at all, and reads no depth:
         * the resolve's quad on pbkit's back buffer, whose depth buffer the
         * host has freed (video_open) */
        p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, (draw->depth_compare & 7) != 7 || draw->depth_write);
        p = pb_push1(p, NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_NEVER + (draw->depth_compare & 7));
        p = pb_push1(p, NV097_SET_DEPTH_MASK, draw->depth_write ? 1 : 0);
        last_depth = depth;
    }
    if (alpha != last_alpha) {
        p = pb_push1(p, NV097_SET_ALPHA_TEST_ENABLE, alpha >= 0);
        if (alpha >= 0) {
            p = pb_push1(p, NV097_SET_ALPHA_FUNC, NV097_SET_ALPHA_FUNC_V_GEQUAL);
            p = pb_push1(p, NV097_SET_ALPHA_REF, (unsigned)alpha);
        }
        last_alpha = alpha;
    }
    if (clip != last_clip || (clip != 0 && (last_clip_rect[0] != draw->clip_x0 || last_clip_rect[1] != draw->clip_y0 ||
                                            last_clip_rect[2] != draw->clip_x1 || last_clip_rect[3] != draw->clip_y1))) {
        push_clip(&p, clip, draw->clip_x0, draw->clip_y0, draw->clip_x1, draw->clip_y1);
        last_clip = clip;
        last_clip_rect[0] = draw->clip_x0;
        last_clip_rect[1] = draw->clip_y0;
        last_clip_rect[2] = draw->clip_x1;
        last_clip_rect[3] = draw->clip_y1;
    }
    push_end(start, p);
    push_indices(indices, count, base, room);
}

/* a shot: the back buffer as R5G6B5, in contiguous memory the log names */
static void take_shot(unsigned frame, USHORT *shot)
{
    const UCHAR *from = (const UCHAR *)pb_back_buffer();
    unsigned pitch = pb_back_buffer_pitch();
    int x, y;

    if (shot == NULL) {
        host_log_line("host: no memory for shot f%u\n", frame);
        return;
    }
    /* 640 by 480 of the display's pixels: at 720p, each shot pixel the
     * display's nearest */
    for (y = 0; y < HEIGHT; y++) {
        const UCHAR *row = from + (unsigned)y * out_h / HEIGHT * pitch;

        for (x = 0; x < WIDTH; x++) {
            unsigned at = (unsigned)x * out_w / WIDTH;

            if (out_16) {
                shot[y * WIDTH + x] = ((const USHORT *)row)[at];
            } else {
                uint32_t c = ((const uint32_t *)row)[at];

                shot[y * WIDTH + x] = (USHORT)(((c >> 19) & 31) << 11 | ((c >> 10) & 63) << 5 | ((c >> 3) & 31));
            }
        }
    }
    host_log_line("host: shot f%u at physical 0x%08lx, %dx%d R5G6B5\n", frame, MmGetPhysicalAddress(shot), WIDTH,
                  HEIGHT);
}

/* the scene's surface onto pbkit's back buffer: a quad over the screen
 * with the surface as its texture. with antialiasing each pixel lies
 * between its two samples, where the bilinear filter gives their mean;
 * else each pixel takes its own texel, at its middle, by point sampling */
static void resolve(void)
{
    static const unsigned short corners[6] = {0, 1, 2, 1, 3, 2};
    static struct host_vertex quad[4];
    struct texture *t = &textures[RESOLVE_TEXTURE];
    struct host_draw draw;
    uint32_t *p;
    uint32_t *start;
    int k;

    flush_pending();
    p = start = push_begin();
    target_back_buffer(&p);
    p = pb_push1(p, NV097_SET_SURFACE_CLIP_HORIZONTAL, out_w << 16);
    p = pb_push1(p, NV097_SET_SURFACE_CLIP_VERTICAL, out_h << 16);
    push_end(start, p);
    memset(t, 0, sizeof *t);
    t->texels = scene_color;
    t->bytes = scene_color_size;
    t->format = out_16 ? NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_R5G6B5 : NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_X8R8G8B8;
    t->levels = 1;
    t->u_scale = (float)scene_w;
    t->v_scale = (float)scene_h;
    t->pitch = scene_pitch;
    t->rect_w = scene_w;
    t->rect_h = scene_h;
    last_texture = ~0u, constants_stale();
    for (k = 0; k < 4; k++) {
        quad[k].x = (k & 1) ? (float)WIDTH : 0.0F;
        quad[k].y = (k & 2) ? (float)HEIGHT : 0.0F;
        quad[k].rhw = 1.0F;
        quad[k].u = (k & 1) ? 1.0F : 0.0F;
        quad[k].v = (k & 2) ? 1.0F : 0.0F;
        quad[k].base = 0xffffffffu;
        quad[k].offset = 0;
    }
    memset(&draw, 0, sizeof draw);
    draw.texture = RESOLVE_TEXTURE;
    draw.src_blend = 1;
    draw.depth_compare = 7;
    draw.ignore_texture_alpha = 1;
    draw.clamp_u = draw.clamp_v = 1;
    draw.filter = (unsigned char)msaa;
    draw.fog = 2;
    depth_noting = 0;
    video_vertices(quad, 4);
    depth_noting = 1;
    video_draw(&draw, corners, 6);
    flush_pending();
    t->texels = NULL;
    last_texture = ~0u, constants_stale();
}

/* whether the host's blanks are the display's own: with vsync, on a
 * display of 60 blanks a second, as the console's. and the display's
 * blanks so far, which pbkit counts */
/* a picture the bloom reads or draws on: its memory, pitch and sizes */
struct bloom_image {
    const void *texels; /* null for pbkit's back buffer as the target */
    unsigned pitch, w, h;
    unsigned format; /* as a texture: the NV2A's linear color format */
};

/* one of the bloom's passes: from, through four taps spread texels apart
 * each way from each pixel's own place, onto to, by the bright program or
 * the blur's, with the combiners' constant factor; added to what to holds,
 * or in its place */
/* add: 0 writes the pass's result, 1 adds it to what's there, 2 mixes it
 * with what's there, by mix_alpha of 255 of the result */
static void bloom_pass(const struct bloom_image *from, const struct bloom_image *to, int bright, float spread,
                       unsigned factor, int add, unsigned mix_alpha)
{
    static const unsigned short corners[6] = {0, 1, 2, 1, 3, 2};
    float constants[5 * 4];
    struct host_vertex *quad;
    uint32_t *p;
    uint32_t *start;
    int k;

    flush_pending();
    make_room(0x1000);
    if (ring_used + 4 > RING_VERTICES) {
        wait_idle();
        ring_used = 0;
    }
    p = start = push_begin();
    if (to->texels == NULL) {
        target_back_buffer(&p);
    }
    if (to->texels != NULL) {
        p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, 3);
        p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                     MASK(NV097_SET_SURFACE_PITCH_COLOR, to->pitch) |
                         MASK(NV097_SET_SURFACE_PITCH_ZETA, pb_back_buffer_pitch()));
        p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, MmGetPhysicalAddress((void *)to->texels) & 0x03ffffff);
        p = pb_push1(p, NV097_SET_SURFACE_FORMAT,
                     MASK(NV097_SET_SURFACE_FORMAT_COLOR, NV097_SET_SURFACE_FORMAT_COLOR_LE_A8R8G8B8) |
                         MASK(NV097_SET_SURFACE_FORMAT_ZETA, NV097_SET_SURFACE_FORMAT_ZETA_Z24S8) |
                         MASK(NV097_SET_SURFACE_FORMAT_TYPE, NV097_SET_SURFACE_FORMAT_TYPE_PITCH));
    }
    p = pb_push1(p, NV097_SET_SURFACE_CLIP_HORIZONTAL, to->w << 16);
    p = pb_push1(p, NV097_SET_SURFACE_CLIP_VERTICAL, to->h << 16);
    p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, 0);
    p = pb_push1(p, NV097_SET_DEPTH_MASK, 0);
    p = pb_push1(p, NV097_SET_ALPHA_TEST_ENABLE, 0);
    p = pb_push1(p, NV097_SET_CULL_FACE_ENABLE, 0);
    p = pb_push1(p, NV097_SET_BLEND_ENABLE, add != 0);
    if (add == 1) {
        p = pb_push1(p, NV097_SET_BLEND_FUNC_SFACTOR, NV097_SET_BLEND_FUNC_SFACTOR_V_ONE);
        p = pb_push1(p, NV097_SET_BLEND_FUNC_DFACTOR, NV097_SET_BLEND_FUNC_SFACTOR_V_ONE);
    } else if (add == 2) {
        p = pb_push1(p, NV097_SET_BLEND_COLOR, mix_alpha << 24);
        p = pb_push1(p, NV097_SET_BLEND_FUNC_SFACTOR, NV097_SET_BLEND_FUNC_SFACTOR_V_CONSTANT_ALPHA);
        p = pb_push1(p, NV097_SET_BLEND_FUNC_DFACTOR, NV097_SET_BLEND_FUNC_DFACTOR_V_ONE_MINUS_CONSTANT_ALPHA);
    }
    push_clip_pixels(&p, 2, 0, 0, to->w, to->h);
    /* the bloom's vertex program, and its four taps */
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, BLIT_SLOT);
    for (k = 0; k < 4; k++) {
        constants[k * 4] = (k & 1) ? spread : -spread;
        constants[k * 4 + 1] = (k & 2) ? spread : -spread;
        constants[k * 4 + 2] = constants[k * 4 + 3] = 0.0F;
    }
    constants[16] = 0.0F, constants[17] = 1.0F, constants[18] = constants[19] = 0.0F;
    push_constants(&p, 80, constants, 5);
    for (k = 0; k < 4; k++) {
        p = pb_push1(p, NV097_SET_TEXTURE_OFFSET + 64 * k, MmGetPhysicalAddress((void *)from->texels) & 0x03ffffff);
        p = pb_push1(p, NV097_SET_TEXTURE_FORMAT + 64 * k,
                     MASK(NV097_SET_TEXTURE_FORMAT_CONTEXT_DMA, 1) |
                         MASK(NV097_SET_TEXTURE_FORMAT_BORDER_SOURCE, NV097_SET_TEXTURE_FORMAT_BORDER_SOURCE_COLOR) |
                         MASK(NV097_SET_TEXTURE_FORMAT_DIMENSIONALITY, 2) |
                         MASK(NV097_SET_TEXTURE_FORMAT_COLOR, from->format) |
                         MASK(NV097_SET_TEXTURE_FORMAT_MIPMAP_LEVELS, 1));
        p = pb_push1(p, NV097_SET_TEXTURE_CONTROL1 + 64 * k, MASK(NV097_SET_TEXTURE_CONTROL1_IMAGE_PITCH, from->pitch));
        p = pb_push1(p, NV097_SET_TEXTURE_IMAGE_RECT + 64 * k,
                     MASK(NV097_SET_TEXTURE_IMAGE_RECT_WIDTH, from->w) |
                         MASK(NV097_SET_TEXTURE_IMAGE_RECT_HEIGHT, from->h));
        p = pb_push1(p, NV097_SET_TEXTURE_ADDRESS + 64 * k, 3 | 3 << 8 | 1 << 16);
        p = pb_push1(p, NV097_SET_TEXTURE_CONTROL0 + 64 * k, NV097_SET_TEXTURE_CONTROL0_ENABLE | 0x0003ffc0);
        p = pb_push1(p, NV097_SET_TEXTURE_FILTER + 64 * k, 0x02022000);
    }
    if (bright) {
        program_bloom_bright(&p);
    } else {
        program_bloom_blur(&p);
    }
    p = pb_push1(p, NV097_SET_COMBINER_FACTOR0, factor);
    push_end(start, p);
    quad = &ring[ring_used];
    for (k = 0; k < 4; k++) {
        quad[k].x = (k & 1) ? (float)to->w : 0.0F;
        quad[k].y = (k & 2) ? (float)to->h : 0.0F;
        quad[k].rhw = 1.0F;
        quad[k].u = (k & 1) ? (float)from->w : 0.0F;
        quad[k].v = (k & 2) ? (float)from->h : 0.0F;
        quad[k].base = 0xffffffffu;
        quad[k].offset = 0;
    }
    push_indices(corners, 6, ring_used, ring);
    flush_pending();
    ring_used += 4;
}

/* the bloom over the finished frame, which pbkit's back buffer holds */
static void bloom(void)
{
    const unsigned linear = NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_X8R8G8B8;
    struct bloom_image frame = {pb_back_buffer(), pb_back_buffer_pitch(), out_w, out_h,
                                out_16 ? NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_R5G6B5 : linear};
    struct bloom_image small[3] = {{bloom_surface[0], BLOOM_PITCH, bloom_w, bloom_h, linear},
                                   {bloom_surface[1], BLOOM_PITCH, bloom_w, bloom_h, linear},
                                   {bloom_surface[2], BLOOM_PITCH, bloom_w, bloom_h, linear}};
    struct bloom_image back = {NULL, 0, out_w, out_h, linear};
    uint32_t *p;
    uint32_t *start;
    int k;

    if (bloom_strength == 0 || !video_bloom_on) {
        bloom_was_on = 0;
        return;
    }
    /* the taps a texel either side of each 4 by 4 block's middle: its mean */
    /* at 720p each small pixel is 8 by 8 of the frame's: the taps two
     * either side */
    bloom_pass(&frame, &small[0], 1, hd ? 2.0F : 1.0F, 0xff000000u | bloom_threshold * 0x010101u, 0, 0);
    bloom_pass(&small[0], &small[1], 0, 1.5F, 0xffffffffu, 0, 0);
    bloom_pass(&small[1], &small[0], 0, 2.5F, 0xffffffffu, 0, 0);
    /* the glow held from frame to frame: this frame's mixed half and half
     * into the last frames', and that added to the frame. the bright pass
     * reads 4 by 4 blocks, less the threshold and times four, and the sand
     * and the water's glints lie just over it: because their glow changed
     * with each frame's fraction of a pixel, a shimmer, the glow is held.
     * the first frame of a stage or a field takes its own whole, the held
     * glow being the last stage's */
    bloom_pass(&small[0], &small[2], 0, 0.5F, 0xffffffffu, 2, bloom_was_on ? 0x80u : 0xffu);
    bloom_was_on = 1;
    bloom_pass(&small[2], &back, 0, 0.5F, 0xff000000u | bloom_strength * 0x010101u, 1, 0);
    /* the draws' own vertex program again, stages 2 and 3 off, and every
     * state the draws keep track of as unknown */
    p = start = push_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, 0);
    for (k = 2; k < 4; k++) {
        p = pb_push1(p, NV097_SET_TEXTURE_CONTROL0 + 64 * k, 0x0003ffc0);
    }
    push_end(start, p);
    last_program = -1;
    last_texture = ~0u, constants_stale();
    last_fog_stage = -1;
    last_fog_factor = 1;
    last_blend = last_depth = last_alpha = last_clip = -1;
    last_cull = -1;
}

/* the textures' pool, in KB: used and its size, for the log's free memory
 * lines (host.c) */
void video_pool_kb(unsigned *used, unsigned *size)
{
    *used = pool_used >> 10;
    *size = pool_size >> 10;
}

/* TRUE where the display is 720p (HOST_720P), whose picture is 16:9 */
int video_hd(void)
{
    return hd;
}

int video_blank_locked(void)
{
    VIDEO_MODE mode = XVideoGetMode();

    return vsync && mode.refresh >= 59 && mode.refresh <= 61;
}

unsigned video_blank_count(void)
{
    return pb_get_vbl_counter();
}

/* the display's blank as the NV2A flags it, read in place: set at each
 * blank and cleared by pbkit's DPC, which counts it (video_blank_count).
 * because the game's thread runs at DISPATCH_LEVEL, which holds the DPC
 * off, the count alone came late and two at a time, and the frames uneven,
 * so the flag is read from the clock's interrupt */
int video_blank_flagged(void)
{
    return (*(volatile uint32_t *)(0xfd000000u + 0x00600100u) & 1u) != 0;
}

void video_present(void)
{
    depth_next();
    /* the resolve and the bloom onto pbkit's back buffer as the next frame
     * begins (finish_frame). the frame's last draws go to the NV2A, which
     * draws on while the game makes the next frame: video_begin shows it
     * then. a shot is taken when the next frame begins, once the NV2A has
     * drawn this one, which video_begin waits for in any case: a wait here
     * would hide what only shows while the NV2A draws a frame as the game
     * makes the next (shots_due) */
    frame_unfinished = 1;
    shots_due = 0;
    while (shots_done + shots_due < shot_count && host_script_now() >= shot_frames[shots_done + shots_due]) {
        shots_due++;
    }
    flush_pending();
    push_kick();
    frame_pending = 1;
    pending_blanks = 0;
}

/* the textures' log line, every ten seconds or so, from the host's chores */
static void gpu_report(const char *prefix);

void video_report(void)
{
    static unsigned last_evictions, last_reloads, last_preloads;

    host_log_line("host: textures: %u from the pack, %u decoded, %u of them new, %u drops, %u of %u KB used; %u read "
                  "from the pack, %u evicted, %u read again at a draw\n",
                  texture_packed, texture_uploads, texture_uploads_new, texture_drops, pool_used >> 10, pool_size >> 10,
                  texture_preloads - last_preloads, texture_evictions - last_evictions, texture_reloads - last_reloads);
    last_preloads = texture_preloads;
    last_evictions = texture_evictions;
    last_reloads = texture_reloads;
    texture_uploads = texture_uploads_new = texture_drops = texture_packed = 0;
    host_log_line("host: draws: %u a second in, %u a second to the NV2A; models: %u draws a second, %u vertices; "
                  "%u lit draws a second, %u palettes sent\n",
                  draws_in / 10, draws_out / 10, model_draws / 10, model_vertices_in / 10, lit_draws / 10, lit_uploads);
    draws_in = draws_out = 0;
    model_draws = model_vertices_in = 0;
    lit_draws = lit_uploads = 0;
    gpu_report("host:");
    memset(gpu_sum_ticks, 0, sizeof gpu_sum_ticks);
    memset(gpu_most_ticks, 0, sizeof gpu_most_ticks);
    memset(gpu_sum_draws, 0, sizeof gpu_sum_draws);
    memset(gpu_sum_waits, 0, sizeof gpu_sum_waits);
    memset(gpu_sum_triangles, 0, sizeof gpu_sum_triangles);
    gpu_sum_all = gpu_most_all = 0;
    gpu_pictures = gpu_waits = 0;
}

int video_gpu_profiling(void)
{
    return gpu_profile != 0;
}

/* the GPU profile's lines, under prefix: each part's mean and most a
 * picture, in microseconds, its draws' and triangles' mean, and with =2
 * the costliest draws of the last picture timed draw by draw */
static void gpu_report(const char *prefix)
{
    static const char *const names[GPU_SECTIONS] = {"clear and setup", "movie", "opaque", "punch-through",
                                                    "translucent", "translucent depth passes", "sky", "resolve",
                                                    "bloom"};
    static const char *const kinds[2] = {"list vertices", "models"};
    unsigned n = gpu_pictures ? gpu_pictures : 1;
    ULONGLONG hz = gpu_frequency ? gpu_frequency : 1;
    int k;

#define US(ticks) ((unsigned long)((ticks) * 1000000ull / hz))
    if (!gpu_profile) {
        return;
    }
    host_log_line("%s GPU profile, each part drawn alone: %u pictures, %u waits a picture, an empty wait %lu.%lu us, "
                  "taken off each; the NV2A's us a picture, mean / most: all %lu / %lu\n",
                  prefix, gpu_pictures, gpu_waits / n,
                  gpu_empty_count ? US(gpu_empty_ticks * 10 / gpu_empty_count) / 10 : 0,
                  gpu_empty_count ? US(gpu_empty_ticks * 10 / gpu_empty_count) % 10 : 0, US(gpu_sum_all / n),
                  US(gpu_most_all));
    for (k = 0; k < GPU_PARTS; k++) {
        if (gpu_sum_ticks[k] == 0 && gpu_sum_draws[k] == 0) {
            continue;
        }
        if (k / 2 == GPU_CLEAR || k / 2 >= GPU_RESOLVE) {
            host_log_line("%s   %s: %lu / %lu\n", prefix, names[k / 2], US(gpu_sum_ticks[k] / n),
                          US(gpu_most_ticks[k]));
        } else {
            host_log_line("%s   %s, %s: %lu / %lu; %u draws, %u triangles, %u waits a picture\n", prefix,
                          names[k / 2], kinds[k & 1], US(gpu_sum_ticks[k] / n), US(gpu_most_ticks[k]),
                          gpu_sum_draws[k] / n, gpu_sum_triangles[k] / n, gpu_sum_waits[k] / n);
        }
    }
    if (gpu_kept_count != 0) {
        host_log_line("%s GPU profile, each draw alone, video frame %u (%u frames ago): %u draws, %lu us; the "
                      "costliest:\n",
                      prefix, gpu_kept_frame, video_frame - gpu_kept_frame, gpu_kept_draws,
                      US(gpu_kept_ticks));
        for (k = 0; k < gpu_kept_count; k++) {
            const struct gpu_draw *d = &gpu_kept[k];
            const struct texture *t = d->texture != 0 && d->texture < TEXTURES ? &textures[d->texture] : NULL;

            host_log_line("%s   %lu us: %s, %s%s, %u triangles, %u vertices; texture %u %ux%u; blend %u/%u, alpha test %u, "
                          "depth write %u, cull %u, clip %u\n",
                          prefix, US(d->ticks), names[d->part / 2], kinds[d->part & 1],
                          d->lit ? " lit" : "", d->triangles, d->vertices, d->texture,
                          t != NULL && t->texels != NULL ? 1u << t->log_w : 0,
                          t != NULL && t->texels != NULL ? 1u << t->log_h : 0, d->src_blend, d->dst_blend,
                          d->punch_through, d->depth_write, d->cull, d->clip_mode);
        }
    }
}
#undef US

/* the report's lines (host.c): the textures' pool and the vertices' rooms */
void video_report_memory(void)
{
    unsigned resident = 0, k;

    for (k = 0; k < TEXTURES; k++) {
        resident += textures[k].texels != NULL;
    }
    host_log_line("report: textures' pool: %u of %u KB used, %u textures in it; %u read from the pack so far, %u "
                  "evicted, %u read again at a draw; all dropped %u times (nothing left to evict)\n",
                  pool_used >> 10, pool_size >> 10, resident, texture_preloads, texture_evictions, texture_reloads,
                  texture_drops_all);
    host_log_line("report: vertex ring: the most a frame used %u of %u vertices, full within a frame %u times, %u "
                  "clipped triangles left undrawn for its room; push buffer: the most a frame used %u of %u KB, full "
                  "within a frame %u times\n",
                  most_ring, (unsigned)RING_VERTICES, ring_restarts, ring_clips_dropped, most_pushed >> 10,
                  (unsigned)(PUSH_SIZE >> 10), push_restarts);
    host_log_line("report: lit models' palettes: the most uploaded in a frame %u of %u slots, a slot taken from a draw of "
                  "the same frame %u times\n",
                  lit_most_frame, (unsigned)LIT_TEXTURES, lit_overwrites);
    host_log_line("report: models' arenas 2 x %u vertices, kept models' store %u vertices%s; antialiasing %s; "
                  "%ux%u, %u swaps waited for past a blank so far, %u past 8 blanks\n",
                  (unsigned)ARENA_VERTICES, store != NULL ? (unsigned)STORE_VERTICES : 0, store != NULL ? "" : " (none)",
                  msaa ? "on" : "off", out_w, out_h, off_screen_waits, off_screen_late);
    gpu_report("report:");
}

/* the guard band: a real NV2A takes the vertex program's position in screen
 * coordinates, as this host's are, and draws a triangle with a vertex far
 * off the screen as a spike across it (xemu, which clips in floats, draws it
 * right). the PowerVR takes them, and the game sends them thousands of
 * pixels out (x -5049 in Station Square), so such triangles are clipped
 * here to a box around the screen, in perspective: along an edge, the
 * position times w, w and the attributes are linear */

struct clip_vertex {
    float x, y, w; /* x and y in pixels, w = 1/rhw */
    float u, v;
    float base[4], offset[4]; /* a, r, g, b */
};

static void to_clip(const struct host_vertex *in, struct clip_vertex *out)
{
    int k;

    out->x = in->x;
    out->y = in->y;
    out->w = 1.0F / (in->rhw > 1.0e-7F ? in->rhw : 1.0e-7F);
    out->u = in->u;
    out->v = in->v;
    for (k = 0; k < 4; k++) {
        out->base[k] = (float)((in->base >> (24 - 8 * k)) & 255);
        out->offset[k] = (float)((in->offset >> (24 - 8 * k)) & 255);
    }
}

static unsigned to_byte(float f)
{
    return f <= 0.0F ? 0 : f >= 255.0F ? 255 : (unsigned)(f + 0.5F);
}

static void from_clip(const struct clip_vertex *in, struct host_vertex *out)
{
    out->x = in->x;
    out->y = in->y;
    out->rhw = 1.0F / in->w;
    out->u = in->u;
    out->v = in->v;
    out->base = to_byte(in->base[0]) << 24 | to_byte(in->base[1]) << 16 | to_byte(in->base[2]) << 8 |
                to_byte(in->base[3]);
    out->offset = to_byte(in->offset[0]) << 24 | to_byte(in->offset[1]) << 16 | to_byte(in->offset[2]) << 8 |
                  to_byte(in->offset[3]);
}

/* how far inside side of the box a vertex is, times its w: side 0 left,
 * 1 right, 2 top, 3 bottom */
static float inside(const struct clip_vertex *c, int side)
{
    switch (side) {
    case 0:
        return (c->x + guard_x) * c->w;
    case 1:
        return ((float)WIDTH + guard_x - c->x) * c->w;
    case 2:
        return (c->y + guard_y) * c->w;
    default:
        return ((float)HEIGHT + guard_y - c->y) * c->w;
    }
}

static void clip_lerp(const struct clip_vertex *a, const struct clip_vertex *b, float t, struct clip_vertex *out)
{
    float xw = a->x * a->w + (b->x * b->w - a->x * a->w) * t;
    float yw = a->y * a->w + (b->y * b->w - a->y * a->w) * t;
    int k;

    out->w = a->w + (b->w - a->w) * t;
    out->x = xw / out->w;
    out->y = yw / out->w;
    out->u = a->u + (b->u - a->u) * t;
    out->v = a->v + (b->v - a->v) * t;
    for (k = 0; k < 4; k++) {
        out->base[k] = a->base[k] + (b->base[k] - a->base[k]) * t;
        out->offset[k] = a->offset[k] + (b->offset[k] - a->offset[k]) * t;
    }
}

/* the triangle clipped to the box, as a convex polygon of up to 7 vertices
 * at poly: its count */
static int clip_triangle(const struct host_vertex *tri, struct clip_vertex *poly)
{
    struct clip_vertex in[8];
    int n = 3, side, k;

    for (k = 0; k < 3; k++) {
        to_clip(&tri[k], &poly[k]);
    }
    for (side = 0; side < 4 && n > 0; side++) {
        int m = 0;

        memcpy(in, poly, sizeof(struct clip_vertex) * (size_t)n);
        for (k = 0; k < n; k++) {
            const struct clip_vertex *a = &in[k], *b = &in[(k + 1) % n];
            float da = inside(a, side), db = inside(b, side);

            if (da >= 0.0F) {
                poly[m++] = *a;
            }
            if ((da >= 0.0F) != (db >= 0.0F)) {
                clip_lerp(a, b, da / (da - db), &poly[m++]);
            }
        }
        n = m;
    }
    return n;
}

static int outside_guard(const struct host_vertex *v)
{
    /* written so that a coordinate of no number is outside, where each
     * compare with it is false */
    return !(v->x >= -guard_x && v->x <= (float)WIDTH + guard_x && v->y >= -guard_y && v->y <= (float)HEIGHT + guard_y);
}

/* a list's vertices, which the draws after it name by index: into the ring,
 * as they are, with their rhw noted for the next frame's depth and those
 * outside the guard band marked */
/* the room a list of n vertices keeps in the ring for its clips */
static unsigned clip_room(unsigned n)
{
    unsigned want = n / 2 + 1024;

    return n + want <= RING_VERTICES ? want : RING_VERTICES - n;
}

void video_vertices(const struct host_vertex *vertices, int count)
{
    struct host_vertex *out;
    unsigned n = count > 0 ? (unsigned)count : 0;
    int any = 0;
    unsigned k;

    flush_pending();
    if (n > 0x10000) {
        n = 0x10000;
    }
    /* a list as long as leaves the ring 1,024 vertices for its clips: half
     * the ring was the bound once, and a list past it lost its other
     * vertices, which drew the 3D gone. with the ring of 10,000, a list may
     * take more than the clips' room of half again, which it took before
     * (Ice Cap's 8,703 at once): its clips then take what is left */
    if (n + 1024 > RING_VERTICES) {
        static unsigned logged;

        n = RING_VERTICES - 1024;
        if (count > (int)logged) {
            logged = (unsigned)count;
            host_log_line("host: %d vertices at once, more than the ring takes: the rest dropped\n", count);
        }
    }
    /* room for them and for the triangles the guard band clips, half again
     * and 1,024 or what the ring has past the list, or the ring starts
     * again, once the NV2A has drawn what it holds */
    if (ring_used + n + clip_room(n) > RING_VERTICES) {
        ring_restarts++;
        wait_idle();
        ring_used = 0;
    }
    out = &ring[ring_used];
    memcpy(out, vertices, n * sizeof(struct host_vertex));
    for (k = 0; k < n; k++) {
        const struct host_vertex *v = &vertices[k];
        float rhw = v->rhw;
        int outside = outside_guard(v);

        /* an rhw that isn't a number, or is out of all reason, as the
         * farthest: the vertex program's fog and depth take it as it is */
        if (!(rhw > 1.0e-7F && rhw < 1.0e7F)) {
            out[k].rhw = rhw = 1.0e-7F;
        }
        if (depth_noting) {
            depth_note(rhw);
        }
        block_outside[k] = (unsigned char)outside;
        any |= outside;
    }
    block_source = vertices;
    block_base = ring_used;
    block_count = n;
    block_any_outside = any;
    block_short = n < (unsigned)(count > 0 ? count : 0);
    ring_used += n;
}

/* the vertex program's rows for a model's vertices: a new id, unless they
 * are the transform the rows hold already, by its address, which the
 * renderer keeps the same for a model's runs within a list */
static void model_set_rows(const float transform[12])
{
    int k;

    if (transform == model_rows_from) {
        return;
    }
    /* or the same rows from another place: the land's models, which lie
     * where they are in the world, all take the camera's, and their draws
     * of one state then join */
    if (model_rows_from != NULL && memcmp(transform, model_rows, 12 * sizeof(float)) == 0) {
        model_rows_from = transform;
        return;
    }
    for (k = 0; k < 12; k++) {
        model_rows[k] = transform[k];
    }
    model_rows[12] = 1.0F;
    model_rows[13] = model_rows[14] = model_rows[15] = 0.0F;
    model_rows_from = transform;
    rows_id++;
}

/* a model's vertices, as they are: the vertex program turns them to the
 * screen by transform. they follow what the ring holds, past the list's own
 * vertices, which the draws between the models still name */
void video_vertices3(const struct host_vertex *vertices, int count, const float transform[12], float rhw_near,
                     float rhw_mid)
{
    unsigned n = count > 0 ? (unsigned)count : 0;
    union {
        float f;
        uint32_t u;
    } bits;

    model_count = 0;
    if (n == 0) {
        return;
    }
    if (arenas != NULL && vertices >= arenas && vertices + n <= arenas + 2 * ARENA_VERTICES) {
        /* in an arena, where the port wrote them: drawn from there */
        model_set_rows(transform);
        model_room = arenas;
        model_base = (unsigned)(vertices - arenas);
        model_count = n;
        model_vertices_in += n;
        goto noted;
    }
    if (store != NULL && vertices >= store && vertices + n <= store + STORE_VERTICES) {
        /* a kept model's, in the store */
        model_set_rows(transform);
        model_room = store;
        model_base = (unsigned)(vertices - store);
        model_count = n;
        model_vertices_in += n;
        goto noted;
    }
    if (n > RING_VERTICES / 2) {
        return;
    }
    flush_pending();
    if (ring_used + n > RING_VERTICES) {
        /* the ring starts again, once the NV2A has drawn what it holds,
         * but not over the list's vertices */
        wait_idle();
        ring_used = block_count != 0 && block_base < n ? block_base + block_count : 0;
        if (ring_used + n > RING_VERTICES) {
            static int logged;

            if (!logged) {
                logged = 1;
                host_log_line("host: no room in the ring for a model's %u vertices: dropped\n", n);
            }
            return;
        }
    }
    memcpy(&ring[ring_used], vertices, n * sizeof(struct host_vertex));
    model_set_rows(transform);
    model_room = ring;
    model_base = ring_used;
    model_count = n;
    ring_used += n;
    model_vertices_in += n;
noted:
    /* their rhw for the next frame's depth: all at the model's midst, and
     * the largest it may hold */
    if (!(rhw_mid > 1.0e-7F && rhw_mid < 1.0e7F)) {
        rhw_mid = 1.0e-7F;
    }
    bits.f = rhw_mid;
    depth_octaves[(bits.u >> 23) & 255] += n;
    depth_count += n;
    if (rhw_near < 1.0e7F && rhw_near > depth_most) {
        depth_most = rhw_near;
    }
}

/* a list's draws, in their order: a model's with its vertices and
 * transform, the others by video_vertices' */
void video_draw_items(const struct host_item *items, int count)
{
    const struct host_vertex *have = NULL;
    const float *have_transform = NULL;
    int k;

    for (k = 0; k < count; k++) {
        const struct host_item *item = &items[k];

        if (item->vertices == NULL) {
            video_draw(&item->draw, item->indices, item->count);
            continue;
        }
        /* because a kept model's vertices may be drawn in two places, under
         * two transforms, the vertices are sent again where the transform
         * differs: taken by the vertices alone, the second drew on the first */
        if (item->vertices != have || item->transform != have_transform) {
            video_vertices3(item->vertices, item->vertex_count, item->transform, item->rhw_near, item->rhw_mid);
            have = item->vertices;
            have_transform = item->transform;
        }
        lit_item = item->palette != NULL && lit_texels != NULL ? item : NULL;
        video_draw3(&item->draw, item->indices, item->count);
        lit_item = NULL;
    }
}

struct host_vertex *video_store(int *vertices)
{
    *vertices = store != NULL ? (int)STORE_VERTICES : 0;
    return store;
}

struct host_vertex *video_arena(int *vertices)
{
    *vertices = (int)ARENA_VERTICES;
    return arenas == NULL ? NULL : arenas + arena_write * ARENA_VERTICES;
}

void video_draw3(const struct host_draw *draw, const unsigned short *indices, int count)
{
    count -= count % 3;
    if (model_count == 0 || count <= 0) {
        return;
    }
    model_draws++;
    gpu_draw_begin(draw, 1, count);
    draw_batch(draw, indices, count, model_base, 1, model_room);
}

void video_draw(const struct host_draw *draw, const unsigned short *indices, int count)
{
    int run = 0; /* the first triangle of the run inside the guard band */
    int k;

    count -= count % 3;
    if (block_count == 0) {
        return;
    }
    gpu_draw_begin(draw, 0, count);
    if (!block_any_outside && !block_short) {
        draw_batch(draw, indices, count, block_base, 0, ring);
        return;
    }
    /* runs of triangles inside the guard band go as they are, and each
     * outside it clipped, its pieces new vertices at the ring's end: the
     * draws of one state join in draw_batch */
    for (k = 0; k < count; k += 3) {
        static const unsigned short fan_indices[15] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
        unsigned i0 = indices[k], i1 = indices[k + 1], i2 = indices[k + 2];
        struct clip_vertex poly[8];
        struct host_vertex tri[3];
        struct host_vertex fan[15];
        int n, j, used = 0;

        if (i0 >= block_count || i1 >= block_count || i2 >= block_count) {
            /* a vertex the ring doesn't hold: the triangle dropped */
        } else if (!(block_outside[i0] | block_outside[i1] | block_outside[i2])) {
            continue;
        }
        if (k > run) {
            draw_batch(draw, &indices[run], k - run, block_base, 0, ring);
        }
        run = k + 3;
        if (i0 >= block_count || i1 >= block_count || i2 >= block_count) {
            continue;
        }
        tri[0] = block_source[i0];
        tri[1] = block_source[i1];
        tri[2] = block_source[i2];
        /* wholly past one edge of the screen: nothing of it shows, and it
         * needs no clip, which most of those outside the guard band are */
        if ((tri[0].x < 0.0F && tri[1].x < 0.0F && tri[2].x < 0.0F) ||
            (tri[0].x > (float)WIDTH && tri[1].x > (float)WIDTH && tri[2].x > (float)WIDTH) ||
            (tri[0].y < 0.0F && tri[1].y < 0.0F && tri[2].y < 0.0F) ||
            (tri[0].y > (float)HEIGHT && tri[1].y > (float)HEIGHT && tri[2].y > (float)HEIGHT)) {
            continue;
        }
        n = clip_triangle(tri, poly);
        /* the polygon as a fan */
        for (j = 1; j + 1 < n; j++) {
            from_clip(&poly[0], &fan[used++]);
            from_clip(&poly[j], &fan[used++]);
            from_clip(&poly[j + 1], &fan[used++]);
        }
        if (used != 0 && ring_used + (unsigned)used > RING_VERTICES) {
            if (ring_clips_dropped++ == 0) {
                host_log_line("host: video: no room in the ring for a clipped triangle: left undrawn (counted in "
                              "the report)\n");
            }
        }
        if (used != 0 && ring_used + (unsigned)used <= RING_VERTICES) {
            memcpy(&ring[ring_used], fan, (size_t)used * sizeof(struct host_vertex));
            draw_batch(draw, fan_indices, used, ring_used, 0, ring);
            ring_used += (unsigned)used;
        }
    }
    if (count > run) {
        draw_batch(draw, &indices[run], count - run, block_base, 0, ring);
    }
}

_Static_assert(RING_VERTICES <= 0x10000 && 2 * ARENA_VERTICES <= 0x10000, "an index is 16 bits, from its room's start");
