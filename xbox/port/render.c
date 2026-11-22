/* the port's renderer: the PowerVR's side of kmRender. it reads the lists
 * Kamui's callers wrote, in the tile accelerator's parameter format, and
 * hands their triangles to the host, batched by state.
 *
 * the lists are 32-byte parameters. a global parameter, a polygon or sprite
 * header, sets the state the vertex parameters after it draw with, and a
 * vertex with its end-of-strip bit ends a triangle strip. the opaque list
 * draws first, then the punch-through list, then the translucent list,
 * whose strips the PowerVR sorts per pixel: the port sorts their triangles
 * back to front by their nearest vertex (draw_triangles), and a strip that blends by its alpha writes
 * its depth in a second draw, of its opaque half alone (header_state). a user clip parameter sets the rectangle a
 * header's clip mode draws inside or outside of. the fog registers go to
 * the host with each frame, for each polygon's fog mode. the modifier lists, the shadows, draw
 * nothing yet.
 *
 * textures are decoded from video memory, as the texture control word and
 * the TSP's sizes name them, into ARGB, and cached by those words: an entry
 * holds while no write has touched its pages since, and while the palettes
 * are as they were for a palettized one */
#include <kamui.h>
#include <stdio.h>
#include <sg_xpt.h>

#include "xbox/port/host.h"
#include "xbox/port/port.h"
#include "xbox/port/texhash.h"

#define LISTS 5
#define LIST_OPAQUE 0
#define LIST_OPAQUE_MODIFIER 1
#define LIST_TRANSLUCENT 2
#define LIST_TRANSLUCENT_MODIFIER 3
#define LIST_PUNCH_THROUGH 4

/* the parameter control word */
#define PCW_TYPE(pcw) ((pcw) >> 29)
#define PARA_END_OF_LIST 0
#define PARA_USER_CLIP 1
#define PARA_OBJECT_LIST 2
#define PARA_POLYGON 4
#define PARA_SPRITE 5
#define PARA_VERTEX 7
/* the port's own, which the tile accelerator has none of: where the game's
 * drawer (gcc/src/draw/model.c) draws a model's meshset that the host
 * transforms. its vertices and its triangles' indices aren't in the list:
 * the drawer wrote them into this file's tables as a run (port_run_begin),
 * and the parameter's word 1 is the run's number. it draws with the state
 * of the header before it, at its place in the list's order */
#define PARA_MODEL 6
#define PCW_END_OF_STRIP 0x10000000u
#define PCW_UV16 0x01
#define PCW_GOURAUD 0x02
#define PCW_OFFSET 0x04
#define PCW_TEXTURE 0x08
#define PCW_COLOR(pcw) (((pcw) >> 4) & 3)
#define PCW_VOLUME 0x40
#define PCW_CLIP(pcw) (((pcw) >> 16) & 3)
#define TILE 32
#define COLOR_PACKED 0
#define COLOR_FLOAT 1
#define COLOR_INTENSITY 2
#define COLOR_INTENSITY_PREVIOUS 3

/* the ISP word */
#define ISP_DEPTH(isp) ((isp) >> 29)
#define ISP_CULL(isp) (((isp) >> 27) & 3)
#define ISP_Z_WRITE_OFF 0x04000000u

/* the TSP word */
#define TSP_SRC(tsp) ((tsp) >> 29)
#define TSP_DST(tsp) (((tsp) >> 26) & 7)
#define TSP_FOG(tsp) (((tsp) >> 22) & 3)
#define TSP_USE_ALPHA 0x00100000u
#define TSP_IGNORE_TEX_ALPHA 0x00080000u
#define TSP_FLIP_U 0x00040000u
#define TSP_FLIP_V 0x00020000u
#define TSP_CLAMP_U 0x00010000u
#define TSP_CLAMP_V 0x00008000u
#define TSP_FILTER(tsp) (((tsp) >> 13) & 3)
#define TSP_SHADING(tsp) (((tsp) >> 6) & 3)
#define TSP_U_SIZE(tsp) (8u << (((tsp) >> 3) & 7))
#define TSP_V_SIZE(tsp) (8u << ((tsp) & 7))

/* the texture control word */
#define TCW_MIPMAP 0x80000000u
#define TCW_VQ 0x40000000u
#define TCW_FORMAT(tcw) (((tcw) >> 27) & 7)
#define TCW_NOT_TWIDDLED 0x04000000u
#define TCW_STRIDE 0x02000000u
#define TCW_PALETTE(tcw) (((tcw) >> 21) & 63)
#define TCW_ADDRESS(tcw) (((tcw) & 0x1fffff) << 3)
#define FORMAT_1555 0
#define FORMAT_565 1
#define FORMAT_4444 2
#define FORMAT_YUV422 3
#define FORMAT_BUMP 4
#define FORMAT_PAL4 5
#define FORMAT_PAL8 6

/* the registers the renderer reads, by offset */
#define REG_PALETTE_FORMAT 0x108
#define REG_TEXT_CONTROL 0x0e4
#define REG_PALETTE 0x1000

KMDWORD port_register(KMDWORD reg);
extern Uint32 port_vram_pages[PORT_VRAM_SIZE / PORT_VRAM_PAGE];
extern Uint32 port_palette_generation;
extern volatile struct kmi_device_extension kmiDeviceExtension;

/* the renderer's room in the port's memory: the decoded texture, the
 * frame's vertices, each once as its list gives it, the triangles as
 * indices into them, then the strips, each with the state it draws with,
 * and their drawing order */
#define DECODED ((Uint32 *)PORT_RENDER)
/* the tables' sizes, which PORT_RENDER_USED holds (the assert below).
 * full_once logs one that fills, and the report gives the most a frame
 * used of each. since the models and sprites go to the host as runs, the
 * lists hold few vertices: 2,832 at the most in Speed Highway's third act,
 * on the console, with 1,703 strips and 40,362 indices */
#define DECODED_MAX (512 * 512)
#define VERTICES ((struct host_vertex *)(PORT_RENDER + DECODED_MAX * 4))
/* a frame's vertices in the lists: 12,288, room for a list as long as Ice
 * Cap's 8,703 at once, before the runs. an index is 16 bits, from its
 * list's first vertex */
#define VERTICES_MAX 0x3000
/* the models' vertices, which the host transforms, are in the host's
 * memory, host_api's video_arena, asked for as a frame's first run begins:
 * the drawer writes them where the host's graphics chip reads them */
static struct host_vertex *arena;
static int arena_room;
#define VERTICES3 arena
#define VERTICES3_MAX arena_room
#define INDICES ((Uint16 *)(VERTICES + VERTICES_MAX))
#define INDICES_MAX 0x20000
#define STRIPS_MAX 0x4000

struct strip {
    struct host_draw draw;
    int first; /* its triangles' indices, three each, in INDICES */
    int count;
    float depth; /* its nearest vertex's rhw, for the translucent sort */
    int run; /* the model run its indices count in, or -1: its list's vertices */
    int depth_pass; /* drawn once more for its depth alone (header_state) */
};

/* the model runs the drawer has made since the last render: each one's
 * vertices in VERTICES3, its triangles' indices in INDICES, from its first
 * vertex, and its transform, as host_api's video_vertices3 takes them */
#define RUNS_MAX 4096
extern volatile int port_sky_drawing;

static struct run {
    const struct host_vertex *vertices; /* in the host's arena, or its store for a kept model's */
    int count;
    int first_index, index_count; /* from INDICES, which the kept models' indices follow */
    int first_part, part_count; /* a translucent run's strips, in parts */
    int shared; /* the first of the runs of its model, whose transform is its own too */
    unsigned serial; /* its model's draw */
    float transform[12];
    float rhw_near, rhw_mid;
    /* a run the host lights (port_run_lit): its palette, a snapshot below,
     * and the light in the model's space; else null */
    const Uint32 *palette;
    float light[3];
    /* its vertices' depths in DEPTHS (port_run_depths_take), or null */
    const float *depths;
    int sky; /* made while the stage's sky task drew (port_sky_drawing) */
} runs[RUNS_MAX];
static int run_count;
static int depth_count; /* in DEPTHS */
static int vertex3_count;
/* a translucent run's strips, each with the 1/z of its nearest vertex: the
 * translucent list sorts each by it, as it sorts the strips it reads */
/* the palettes of the runs the host lights, as they were when each was
 * drawn: a palette's base colors and then its offset colors (host.h's
 * host_item), until the frame's render has drawn them */
#define LIT_PALETTES 32
static Uint32 lit_palettes[LIT_PALETTES][512];
static const Uint32 *lit_from[LIT_PALETTES][2]; /* the palette and the offsets each was taken from */
static int lit_count;
/* runs lit by another's palette, the table full, and the most palettes one
 * frame took: the report's */
static unsigned lit_full;
static int lit_most;
#define PARTS_MAX 8192
static struct run_part {
    int first_index, index_count; /* in the run's indices */
    float rhw_near;
} parts[PARTS_MAX];
static int part_count;

#define STRIPS ((struct strip *)(INDICES + INDICES_MAX))
#define ORDER ((int *)(STRIPS + STRIPS_MAX))
/* the kept models (below): their triangles' indices, three for each vertex
 * of the host's store at most, at three times the vertex's place there, and
 * their records */
#define STORE_MAX 65520
#define KEPT_MAX 2048
#define KEPT_SETS 16
#define KEPT_INDICES ((Uint16 *)(ORDER + STRIPS_MAX))
#define KEPT ((struct kept *)(KEPT_INDICES + STORE_MAX * 3))
/* the translucent runs' vertices' depths in view space, a float each, as
 * the drawer computes them from the model's points (port_run_depths): the
 * sort by triangle keys by them, here in cached memory, where the runs'
 * vertices lie in the host's arena or store, memory written through
 * write-combining, which the CPU reads uncached. a run past the table has
 * none, and the sort reads its vertices */
#define DEPTHS_MAX 0x4000
#define DEPTHS ((float *)(KEPT + KEPT_MAX))
/* a kept model's meshset: its vertices, from the model's first, its
 * indices, and its polygons' words and uvs, for the drawer's checks */
struct keep_set {
    Uint16 first, count;
    Uint32 indices;
    Uint32 mesh_words;
};
/* what the drawer keeps of a kept model (gcc/src/draw/model.c, which has
 * the same struct): the light it was shaded by, in its own space, what else
 * its colors came from, sums over its data, its box, and its meshsets */
struct keep_info {
    float light[3];
    Uint32 signature, probe, full;
    Uint32 built; /* the frame it was built in */
    Uint32 quiet_until; /* not kept before this frame: its data changes */
    Uint8 strikes, sets, held, spare;
    float box[6];
    struct keep_set set[KEPT_SETS];
};
struct kept {
    const void *model; /* null for a record that's free, or holds vertices no model names now */
    Uint16 chain; /* the next of its bucket, or of the free ones */
    Uint16 before, after; /* its neighbors in the store, by address, among those that hold vertices */
    Uint8 in_use, in_store;
    int first, count; /* its vertices in the store */
    Uint32 used; /* the frame that last drew it */
    struct keep_info info;
};
_Static_assert(DECODED_MAX * 4 + VERTICES_MAX * sizeof(struct host_vertex) + INDICES_MAX * 2 +
                       STRIPS_MAX * (sizeof(struct strip) + 4) + STORE_MAX * 3 * 2 +
                       KEPT_MAX * sizeof(struct kept) + DEPTHS_MAX * sizeof(float) <=
                   PORT_RENDER_USED,
               "the renderer's tables outgrow its room");
_Static_assert(VERTICES_MAX <= 0x10000, "an index is 16 bits");
#define strips STRIPS
#define order ORDER
unsigned port_vertices_read; /* the lists' vertices so far, for the profile's counts */
/* the drawer's meshsets that didn't fit the run's room whole, sent through
 * their lists instead (gcc/src/draw/model.c's gpu_fits), for the report */
unsigned port_run_unfit;
/* their vertices this frame: with the arena's, what the frame wanted of it */
unsigned port_run_unfit_vertices;
static unsigned most_arena_wanted;
static int strip_count;
static int vertex_count;
static int index_count;
/* the list being read: its first vertex, which its indices count from, and
 * the first vertex of the strip being read, whose vertices are those from
 * it on */
static int list_first;
static int strip_first;

static void full_once(const char *what)
{
    static const char *logged[4];
    int k;

    for (k = 0; k < 4 && logged[k] != NULL; k++) {
        if (logged[k] == what) {
            return;
        }
    }
    if (k < 4) {
        logged[k] = what;
    }
    port_log(what);
}
/* the most a frame has needed of the tables, which the frame's log line
 * gives, for their sizes */
static int most_strips, most_vertices, most_indices, most_runs, most_parts, most_arena;
/* the last frame's bytes in each of Kamui's lists, and the most */
static int list_bytes[LISTS], most_list_bytes[LISTS];
static Uint32 most_texels;

/* the parameter state of a list as its parameters set it */
struct state {
    Uint32 pcw, isp, tsp, tcw;
    int sprite;
    int list;
    int vertex_words; /* a vertex parameter's words: 8 or 16 */
    float face[4], face_offset[4]; /* intensity's colors, A R G B */
    Uint32 sprite_base, sprite_offset;
    Uint32 clip[4]; /* the user clip's rectangle, in pixels */
    struct host_draw draw;
    int depth_pass; /* a translucent strip's depth goes in a draw of its own (header_state) */
};

/* the texture cache */
#define CACHE_SIZE 1024
struct cached {
    Uint32 tcw, tsp_size;
    Uint32 generation; /* the newest of its pages' generations */
    Uint32 palette_generation;
    Uint32 last_used;
    unsigned id;
    struct texhash key; /* of what the host has as id, where keyed is set */
    int keyed;
};
static struct cached cache[CACHE_SIZE];
static Uint32 frame_number;
/* a texture decoded from video memory is read again only when a write marks
 * its pages (newest_page) */

static float word_float(Uint32 word)
{
    union {
        Uint32 u;
        float f;
    } bits;

    bits.u = word;
    return bits.f;
}

static Uint32 float_color(float a, float r, float g, float b)
{
    int ia = (int)(a * 255.0F + 0.5F);
    int ir = (int)(r * 255.0F + 0.5F);
    int ig = (int)(g * 255.0F + 0.5F);
    int ib = (int)(b * 255.0F + 0.5F);

    ia = ia < 0 ? 0 : ia > 255 ? 255 : ia;
    ir = ir < 0 ? 0 : ir > 255 ? 255 : ir;
    ig = ig < 0 ? 0 : ig > 255 ? 255 : ig;
    ib = ib < 0 ? 0 : ib > 255 ? 255 : ib;
    return ((Uint32)ia << 24) | ((Uint32)ir << 16) | ((Uint32)ig << 8) | (Uint32)ib;
}

/* textures */

static Uint32 from_1555(Uint32 t)
{
    Uint32 r = (t >> 10) & 31, g = (t >> 5) & 31, b = t & 31;

    return (t & 0x8000 ? 0xff000000u : 0) | ((r << 3 | r >> 2) << 16) | ((g << 3 | g >> 2) << 8) | (b << 3 | b >> 2);
}

static Uint32 from_565(Uint32 t)
{
    Uint32 r = (t >> 11) & 31, g = (t >> 5) & 63, b = t & 31;

    return 0xff000000u | ((r << 3 | r >> 2) << 16) | ((g << 2 | g >> 4) << 8) | (b << 3 | b >> 2);
}

static Uint32 from_4444(Uint32 t)
{
    Uint32 a = (t >> 12) & 15, r = (t >> 8) & 15, g = (t >> 4) & 15, b = t & 15;

    return (a * 17) << 24 | (r * 17) << 16 | (g * 17) << 8 | b * 17;
}

static Uint32 from_16(Uint32 format, Uint32 t)
{
    switch (format) {
    case FORMAT_565:
        return from_565(t);
    case FORMAT_4444:
        return from_4444(t);
    case FORMAT_1555:
    default:
        return from_1555(t);
    }
}

/* a palette entry, in the palettes' format */
static Uint32 palette_color(Uint32 index)
{
    Uint32 entry = port_register(REG_PALETTE + (index & 1023) * 4);

    switch (port_register(REG_PALETTE_FORMAT) & 3) {
    case 0:
        return from_1555(entry & 0xffff);
    case 1:
        return from_565(entry & 0xffff);
    case 2:
        return from_4444(entry & 0xffff);
    default:
        return entry;
    }
}

/* a texel's place in a twiddled texture: x's and y's bits interleaved, y's
 * lowest first, in squares of the smaller side along the larger */
static Uint32 twiddle(Uint32 x, Uint32 y, Uint32 width, Uint32 height)
{
    Uint32 side = width < height ? width : height;
    Uint32 block = width < height ? y / side : x / side;
    Uint32 inside = 0;
    Uint32 bit;
    Uint32 sx = x % side;
    Uint32 sy = y % side;

    for (bit = 0; (1u << bit) < side; bit++) {
        inside |= ((sy >> bit) & 1) << (2 * bit);
        inside |= ((sx >> bit) & 1) << (2 * bit + 1);
    }
    return block * side * side + inside;
}

/* where a mipmapped texture's largest level starts, past the smaller
 * ones, in bytes for 16 bits a texel, by the level's log2 size */
static const Uint32 mip_offsets[11] = {0x00006, 0x00008, 0x00010, 0x00030, 0x000b0, 0x002b0,
                                       0x00ab0, 0x02ab0, 0x0aab0, 0x2aab0, 0xaaab0};
/* the same for a VQ texture's indices */
static const Uint32 vq_mip_offsets[11] = {0x00000, 0x00001, 0x00002, 0x00006, 0x00016, 0x00056,
                                          0x00156, 0x00556, 0x01556, 0x05556, 0x15556};

static Uint32 log2_of(Uint32 n)
{
    Uint32 k = 0;

    while ((1u << k) < n) {
        k++;
    }
    return k;
}

static Uint32 yuv_to_argb(int y, int u, int v)
{
    int r = y + ((v - 128) * 359 >> 8);
    int g = y - ((u - 128) * 88 >> 8) - ((v - 128) * 183 >> 8);
    int b = y + ((u - 128) * 454 >> 8);

    r = r < 0 ? 0 : r > 255 ? 255 : r;
    g = g < 0 ? 0 : g > 255 ? 255 : g;
    b = b < 0 ? 0 : b > 255 ? 255 : b;
    return 0xff000000u | (Uint32)r << 16 | (Uint32)g << 8 | (Uint32)b;
}

/* decode the texture into DECODED */
static void decode(Uint32 tcw, Uint32 tsp, Uint32 width, Uint32 height)
{
    const Uint8 *vram = (const Uint8 *)PORT_VRAM;
    Uint32 base = TCW_ADDRESS(tcw) & (PORT_VRAM_SIZE - 1);
    Uint32 format = TCW_FORMAT(tcw);
    Uint32 level = log2_of(width);
    Uint32 x;
    Uint32 y;

    (void)tsp;
#define AT(offset) vram[((offset)) & (PORT_VRAM_SIZE - 1)]
#define HALF(offset) ((Uint32)AT(offset) | (Uint32)AT((offset) + 1) << 8)
    if (tcw & TCW_VQ) {
        Uint32 indices = base + 2048 + ((tcw & TCW_MIPMAP) ? vq_mip_offsets[level] : 0);

        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                Uint32 index = AT(indices + twiddle(x / 2, y / 2, width / 2, height / 2));
                Uint32 texel = (x & 1) * 2 + (y & 1);

                DECODED[y * width + x] = from_16(format, HALF(base + index * 8 + texel * 2));
            }
        }
        return;
    }
    if (format == FORMAT_PAL4 || format == FORMAT_PAL8) {
        Uint32 palette = TCW_PALETTE(tcw);
        Uint32 start = base;

        if (tcw & TCW_MIPMAP) {
            start += format == FORMAT_PAL4 ? mip_offsets[level] >> 2 : mip_offsets[level] >> 1;
        }
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                Uint32 i = twiddle(x, y, width, height);
                Uint32 index;

                if (format == FORMAT_PAL4) {
                    index = (AT(start + i / 2) >> ((i & 1) * 4)) & 15;
                    index += palette << 4;
                } else {
                    index = AT(start + i) + ((palette >> 4) << 8);
                }
                DECODED[y * width + x] = palette_color(index);
            }
        }
        return;
    }
    if (tcw & TCW_NOT_TWIDDLED) {
        Uint32 stride = width;

        if (tcw & TCW_STRIDE) {
            stride = (port_register(REG_TEXT_CONTROL) & 31) * 32;
        }
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                Uint32 offset = base + (y * stride + x) * 2;

                if (format == FORMAT_YUV422) {
                    Uint32 pair = base + (y * stride + (x & ~1u)) * 2;
                    int u = AT(pair);
                    int yy = AT(pair + 1 + (x & 1) * 2);
                    int v = AT(pair + 2);

                    DECODED[y * width + x] = yuv_to_argb(yy, u, v);
                } else {
                    DECODED[y * width + x] = format == FORMAT_BUMP ? 0xff808080u : from_16(format, HALF(offset));
                }
            }
        }
        return;
    }
    {
        Uint32 start = base + ((tcw & TCW_MIPMAP) ? mip_offsets[level] : 0);

        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                Uint32 t = HALF(start + twiddle(x, y, width, height) * 2);

                DECODED[y * width + x] = format == FORMAT_BUMP ? 0xff808080u
                                         : format == FORMAT_YUV422 ? yuv_to_argb(t >> 8, 128, 128)
                                                                   : from_16(format, t);
            }
        }
    }
#undef AT
#undef HALF
}

/* the texture's key, texhash.h's, over the bytes decode reads: whether it
 * has one. palettized textures, whose colors the palette gives, strided
 * ones and those video memory's end wraps have none */
static int texture_key(Uint32 tcw, Uint32 width, Uint32 height, struct texhash *key)
{
    const Uint8 *vram = (const Uint8 *)PORT_VRAM;
    Uint32 base = TCW_ADDRESS(tcw) & (PORT_VRAM_SIZE - 1);
    Uint32 format = TCW_FORMAT(tcw);
    Uint32 level = log2_of(width);
    int vq = (tcw & TCW_VQ) != 0;
    int raster = !vq && (tcw & TCW_NOT_TWIDDLED) != 0;
    Uint32 start;
    Uint32 top = vq ? width * height / 4 : width * height * 2;

    if (format > FORMAT_4444 || (raster && (tcw & TCW_STRIDE)) || ((tcw & TCW_MIPMAP) && width != height)) {
        return 0;
    }
    start = vq ? 2048 + ((tcw & TCW_MIPMAP) ? vq_mip_offsets[level] : 0)
               : (!raster && (tcw & TCW_MIPMAP)) ? mip_offsets[level] : 0;
    if (base + start + top > PORT_VRAM_SIZE) {
        return 0;
    }
    texhash_start(key, format, vq, raster, width, height);
    if (vq) {
        texhash_bytes(key, vram + base, 2048);
    }
    texhash_bytes(key, vram + base + start, top);
    return 1;
}

/* the bytes a texture spans in video memory, for its pages' generations */
static Uint32 texture_bytes(Uint32 tcw, Uint32 width, Uint32 height)
{
    Uint32 format = TCW_FORMAT(tcw);
    Uint32 texels = width * height;
    Uint32 bytes;

    if (tcw & TCW_VQ) {
        bytes = 2048 + texels / 4;
    } else if (format == FORMAT_PAL4) {
        bytes = texels / 2;
    } else if (format == FORMAT_PAL8) {
        bytes = texels;
    } else if (tcw & TCW_STRIDE) {
        bytes = height * (port_register(REG_TEXT_CONTROL) & 31) * 64;
    } else {
        bytes = texels * 2;
    }
    return (tcw & TCW_MIPMAP) ? bytes * 2 : bytes;
}

static Uint32 newest_page(Uint32 base, Uint32 bytes)
{
    Uint32 newest = 0;
    Uint32 page;

    for (page = base / PORT_VRAM_PAGE; page <= (base + bytes - 1) / PORT_VRAM_PAGE && page < PORT_VRAM_SIZE / PORT_VRAM_PAGE;
         page++) {
        if (port_vram_pages[page] > newest) {
            newest = port_vram_pages[page];
        }
    }
    return newest;
}

/* the host's texture for the words, decoded anew where it has changed */
static unsigned texture_for(Uint32 tcw, Uint32 tsp)
{
    Uint32 width = TSP_U_SIZE(tsp);
    Uint32 height = TSP_V_SIZE(tsp);
    Uint32 size = tsp & 0x3f;
    Uint32 base = TCW_ADDRESS(tcw) & (PORT_VRAM_SIZE - 1);
    Uint32 format = TCW_FORMAT(tcw);
    struct texhash placed_key;
    unsigned placed_serial;
    /* a texture kmLoadTexture gave the pack isn't in video memory: its key
     * is its place's, and the serial of its load stands for the pages'
     * generation, with the top bit, which no generation has */
    int placed = port_texture_at(base, format, (tcw & TCW_VQ) != 0, !(tcw & TCW_VQ) && (tcw & TCW_NOT_TWIDDLED) != 0,
                                 width, height, &placed_key, &placed_serial);
    Uint32 generation = placed ? 0x80000000u | placed_serial : newest_page(base, texture_bytes(tcw, width, height));
    Uint32 palette_generation =
        !placed && (format == FORMAT_PAL4 || format == FORMAT_PAL8) ? port_palette_generation : 0;
    Uint32 hash = ((tcw * 2654435761u) ^ size) % CACHE_SIZE;
    Uint32 k;
    struct cached *slot = NULL;
    struct cached *oldest = NULL;

    if (width * height > DECODED_MAX) {
        full_once("render: a texture larger than DECODED_MAX: drawn without it");
        return 0;
    }
    if (width * height > most_texels) {
        most_texels = width * height;
    }
    for (k = 0; k < 8; k++) {
        struct cached *entry = &cache[(hash + k) % CACHE_SIZE];

        if (entry->id != 0 && entry->tcw == tcw && entry->tsp_size == size) {
            slot = entry;
            break;
        }
        if (oldest == NULL || entry->id == 0 || (oldest->id != 0 && entry->last_used < oldest->last_used)) {
            oldest = entry;
        }
    }
    if (slot == NULL) {
        slot = oldest;
        if (slot->id == 0) {
            slot->id = (unsigned)(slot - cache) + 1;
        }
        slot->tcw = tcw;
        slot->tsp_size = size;
        slot->generation = ~0u;
        slot->keyed = 0;
    }
    slot->last_used = frame_number;
    if (placed && (slot->generation != generation || slot->palette_generation != palette_generation)) {
        /* the host's, from its pack; if it can't read it, the draw goes
         * without a texture */
        HOST->video_texture_packed(slot->id, placed_key.lo, placed_key.hi);
        slot->key = placed_key;
        slot->keyed = 1;
        slot->generation = generation;
        slot->palette_generation = palette_generation;
    }
    if (slot->generation != generation || slot->palette_generation != palette_generation) {
        struct texhash key;
        int keyed;

        /* reading an unbacked page faults, so video memory is backed
         * before it's read: a texture nothing loaded reads as zeros, and
         * where the host has no memory the draw goes without and the next
         * draw tries again */
        if (!port_vram_commit(base, texture_bytes(tcw, width, height))) {
            full_once("render: a texture's video memory has no backing: drawn without it");
            return 0;
        }
        keyed = texture_key(tcw, width, height, &key);

        /* a write to its pages that left its bytes as they were, as a
         * neighbor's load does, leaves the host's texture as it is. else
         * the host's own, converted ahead, where it has one for the key,
         * or the texture decoded here */
        if (keyed && slot->keyed && slot->key.lo == key.lo && slot->key.hi == key.hi) {
        } else if (!keyed || !HOST->video_texture_packed(slot->id, key.lo, key.hi)) {
            decode(tcw, tsp, width, height);
            HOST->video_texture(slot->id, (int)width, (int)height, DECODED);
        }
        slot->key = key;
        slot->keyed = keyed;
        slot->generation = generation;
        slot->palette_generation = palette_generation;
    }
    return slot->id;
}

/* the state a header sets */

static void header_state(struct state *st)
{
    struct host_draw *d = &st->draw;
    Uint32 tsp = st->tsp;
    int textured = (st->pcw & PCW_TEXTURE) != 0;

    d->texture = textured ? texture_for(st->tcw, tsp) : 0;
    d->src_blend = (unsigned char)TSP_SRC(tsp);
    d->dst_blend = (unsigned char)TSP_DST(tsp);
    if (st->list == LIST_OPAQUE || st->list == LIST_PUNCH_THROUGH) {
        d->src_blend = 1;
        d->dst_blend = 0;
    }
    d->depth_compare = (unsigned char)ISP_DEPTH(st->isp);
    d->depth_write = !(st->isp & ISP_Z_WRITE_OFF);
    d->shading = (unsigned char)TSP_SHADING(tsp);
    d->use_alpha = (tsp & TSP_USE_ALPHA) != 0;
    d->ignore_texture_alpha = (tsp & TSP_IGNORE_TEX_ALPHA) != 0;
    d->use_offset = (st->pcw & PCW_OFFSET) != 0;
    d->punch_through = st->list == LIST_PUNCH_THROUGH;
    d->list = (unsigned char)(st->list == LIST_OPAQUE        ? 1
                              : st->list == LIST_PUNCH_THROUGH ? 2
                              : st->list == LIST_TRANSLUCENT   ? 3
                                                               : 0);
    d->clamp_u = (tsp & TSP_CLAMP_U) != 0;
    d->clamp_v = (tsp & TSP_CLAMP_V) != 0;
    d->flip_u = (tsp & TSP_FLIP_U) != 0;
    d->flip_v = (tsp & TSP_FLIP_V) != 0;
    d->filter = (unsigned char)TSP_FILTER(tsp);
    d->fog = (unsigned char)TSP_FOG(tsp);
    d->alpha_ref = 0.5F;
    st->depth_pass = 0;
    if (st->list == LIST_TRANSLUCENT && d->src_blend == 4 && (d->dst_blend == 5 || d->dst_blend == 1)) {
        /* source alpha over its inverse or over one. the PowerVR sorts the
         * list per pixel; the port sorts strips, coarsely, and a strip that
         * wrote its depth as it drew would leave a hole of its whole shape,
         * clear texels and all, in a farther one drawn after it (the water
         * behind a splash's or the HUD's squares). so its colors draw
         * without depth, less the fragments of no alpha, which change
         * nothing; then, where it writes depth, its triangles draw again
         * for the depth alone (draw_strips), of the fragments at least half
         * opaque: those hide a farther strip, and the fainter ones let it
         * blend over them */
        d->punch_through = 1;
        d->alpha_ref = 1.0F / 255.0F;
        /* because an additive strip (over one) written among the others
         * left holes in what sorted after it (Emerald Coast's sun glare in
         * the water), it draws no depth and draws after every other
         * translucent strip: adding is the same in any order, and the
         * others' depth says which lie in front of it */
        st->depth_pass = d->dst_blend == 5 ? d->depth_write : 0;
        d->depth_write = 0;
    }
    d->clip_mode = (unsigned char)PCW_CLIP(st->pcw);
    d->cull = 0;
    d->clip_x0 = (unsigned short)st->clip[0];
    d->clip_y0 = (unsigned short)st->clip[1];
    d->clip_x1 = (unsigned short)st->clip[2];
    d->clip_y1 = (unsigned short)st->clip[3];
}


/* the words a vertex parameter takes, by the header's control word */
static int vertex_words(Uint32 pcw, int sprite)
{
    if (sprite) {
        return 16;
    }
    if (pcw & PCW_VOLUME) {
        return (pcw & PCW_TEXTURE) ? 16 : 8;
    }
    if ((pcw & PCW_TEXTURE) && PCW_COLOR(pcw) == COLOR_FLOAT) {
        return 16;
    }
    return 8;
}

/* the words a polygon header takes */
static int header_words(Uint32 pcw)
{
    int color = PCW_COLOR(pcw);

    if (pcw & PCW_VOLUME) {
        return color == COLOR_INTENSITY ? 16 : 8;
    }
    return color == COLOR_INTENSITY && (pcw & PCW_OFFSET) ? 16 : 8;
}

static void read_header(struct state *st, const Uint32 *p)
{
    int color = PCW_COLOR(p[0]);

    st->pcw = p[0];
    st->isp = p[1];
    st->tsp = p[2];
    st->tcw = p[3];
    st->sprite = PCW_TYPE(p[0]) == PARA_SPRITE;
    st->vertex_words = vertex_words(p[0], st->sprite);
    if (st->sprite) {
        st->sprite_base = p[4];
        st->sprite_offset = p[5];
    } else if (color == COLOR_INTENSITY) {
        int k;
        const Uint32 *face = header_words(p[0]) == 16 ? p + 8 : p + 4;

        for (k = 0; k < 4; k++) {
            st->face[k] = word_float(face[k]);
            st->face_offset[k] = header_words(p[0]) == 16 ? word_float(p[12 + k]) : 0.0F;
        }
    }
    header_state(st);
}

/* triangles */

/* the strip read so far ends: its triangles, by index, less those its
 * culling drops. a strip that leaves none gives its vertices back */
static unsigned nonfinite_triangles; /* dropped by flush_strip, for the profile's line */

static void flush_strip(const struct state *st, int strip_ends)
{
    struct strip *s;
    const struct host_vertex *v = &VERTICES[strip_first];
    Uint16 *out = &INDICES[index_count];
    int length = vertex_count - strip_first;
    int base = strip_first - list_first;
    int cull = ISP_CULL(st->isp);
    int count = 0;
    int k;
    float nearest;

    (void)strip_ends;
    if (length >= 3 && strip_count >= STRIPS_MAX) {
        full_once("render: the strips' table is full: a frame's strips dropped");
    }
    if (length < 3 || strip_count >= STRIPS_MAX) {
        vertex_count = strip_first;
        return;
    }
    if (index_count + (length - 2) * 3 > INDICES_MAX) {
        full_once("render: the indices' table is full: a frame's triangles dropped");
        vertex_count = strip_first;
        return;
    }
    nearest = v[0].rhw > v[1].rhw ? v[0].rhw : v[1].rhw;
    for (k = 2; k < length; k++) {
        /* the strip's triangles turn alternately: the odd ones swapped,
         * every one turns as the first */
        int ia = k - 2 + (k & 1);
        int ib = k - 1 - (k & 1);
        const struct host_vertex *a = &v[ia];
        const struct host_vertex *b = &v[ib];
        const struct host_vertex *c = &v[k];

        if (c->rhw > nearest) {
            nearest = c->rhw;
        }
        /* because the NV2A draws a triangle with a vertex of no number
         * (infinite or NaN x or y) as a wedge across the screen, where the
         * PowerVR shows nothing, such a triangle is dropped */
        if (!__builtin_isfinite(a->x) || !__builtin_isfinite(a->y) || !__builtin_isfinite(a->rhw) ||
            !__builtin_isfinite(b->x) || !__builtin_isfinite(b->y) || !__builtin_isfinite(b->rhw) ||
            !__builtin_isfinite(c->x) || !__builtin_isfinite(c->y) || !__builtin_isfinite(c->rhw)) {
            nonfinite_triangles++;
            continue;
        }
        if (cull >= 2) {
            float area = (b->x - a->x) * (c->y - a->y) - (c->x - a->x) * (b->y - a->y);

            /* culling: 2 drops counterclockwise triangles, 3 clockwise, as
             * the screen shows them, y down */
            if ((cull == 2 && area < 0.0F) || (cull == 3 && area > 0.0F)) {
                continue;
            }
        }
        out[count++] = (Uint16)(base + ia);
        out[count++] = (Uint16)(base + ib);
        out[count++] = (Uint16)(base + k);
    }
    if (count == 0) {
        vertex_count = strip_first;
        return;
    }
    s = &strips[strip_count++];
    s->draw = st->draw;
    s->depth_pass = st->depth_pass;
    s->first = index_count;
    s->count = count;
    s->depth = nearest;
    s->run = -1;
    index_count += count;
    strip_first = vertex_count;
}

static void read_vertex(struct state *st, const Uint32 *p)
{
    struct host_vertex *v;
    int color = PCW_COLOR(st->pcw);
    int textured = (st->pcw & PCW_TEXTURE) != 0;

    port_vertices_read++;
    if (vertex_count >= VERTICES_MAX) {
        full_once("render: the vertices' table is full: a frame's triangles dropped");
        return;
    }
    v = &VERTICES[vertex_count++];
    v->x = word_float(p[1]);
    v->y = word_float(p[2]);
    v->rhw = word_float(p[3]);
    v->u = v->v = 0.0F;
    v->offset = 0;
    if (textured) {
        if (st->pcw & PCW_UV16) {
            v->u = word_float(p[4] & 0xffff0000u);
            v->v = word_float(p[4] << 16);
        } else {
            v->u = word_float(p[4]);
            v->v = word_float(p[5]);
        }
    }
    if (st->pcw & PCW_VOLUME) {
        v->base = textured ? p[6] : p[4];
        v->offset = textured ? p[7] : 0;
        return;
    }
    switch (color) {
    case COLOR_PACKED:
        v->base = textured ? p[6] : p[6];
        v->offset = textured ? p[7] : 0;
        break;
    case COLOR_FLOAT:
        if (textured) {
            v->base = float_color(word_float(p[8]), word_float(p[9]), word_float(p[10]), word_float(p[11]));
            v->offset = float_color(word_float(p[12]), word_float(p[13]), word_float(p[14]), word_float(p[15]));
        } else {
            v->base = float_color(word_float(p[4]), word_float(p[5]), word_float(p[6]), word_float(p[7]));
        }
        break;
    default: {
        float i = word_float(p[6]);
        float o = textured ? word_float(p[7]) : 0.0F;

        v->base = float_color(st->face[0], st->face[1] * i, st->face[2] * i, st->face[3] * i);
        v->offset = float_color(st->face_offset[0], st->face_offset[1] * o, st->face_offset[2] * o,
                                st->face_offset[3] * o);
        break;
    }
    }
}

/* a sprite's four corners, a quad: A, B, C given, D's depth from them */
static void read_sprite(struct state *st, const Uint32 *p)
{
    float ax = word_float(p[1]), ay = word_float(p[2]), az = word_float(p[3]);
    float bx = word_float(p[4]), by = word_float(p[5]), bz = word_float(p[6]);
    float cx = word_float(p[7]), cy = word_float(p[8]), cz = word_float(p[9]);
    float dx = word_float(p[10]), dy = word_float(p[11]);
    float au = 0, av = 0, bu = 0, bv = 0, cu = 0, cv = 0;
    struct host_vertex *v;
    int k;

    if (st->pcw & PCW_TEXTURE) {
        au = word_float(p[13] & 0xffff0000u);
        av = word_float(p[13] << 16);
        bu = word_float(p[14] & 0xffff0000u);
        bv = word_float(p[14] << 16);
        cu = word_float(p[15] & 0xffff0000u);
        cv = word_float(p[15] << 16);
    }
    vertex_count = strip_first;
    if (vertex_count + 4 > VERTICES_MAX) {
        full_once("render: the vertices' table is full: a frame's triangles dropped");
        return;
    }
    v = &VERTICES[vertex_count];
    vertex_count += 4;
    /* the strip A, B, D, C draws the quad A B C D */
    v[0].x = ax, v[0].y = ay, v[0].rhw = az, v[0].u = au, v[0].v = av;
    v[1].x = bx, v[1].y = by, v[1].rhw = bz, v[1].u = bu, v[1].v = bv;
    v[2].x = dx, v[2].y = dy, v[2].rhw = az + cz - bz, v[2].u = au + cu - bu, v[2].v = av + cv - bv;
    v[3].x = cx, v[3].y = cy, v[3].rhw = cz, v[3].u = cu, v[3].v = cv;
    for (k = 0; k < 4; k++) {
        v[k].base = st->sprite_base;
        v[k].offset = st->sprite_offset;
    }
    flush_strip(st, 1);
}

static unsigned headers_same; /* the headers read_list found as the last, for the profile */

static void read_list(int list, const Uint32 *start, const Uint32 *end)
{
    struct state st;
    const Uint32 *p = start;
    int have_header = 0;

    st.list = list;
    st.pcw = 0;
    st.clip[0] = st.clip[1] = 0;
    st.clip[2] = 640;
    st.clip[3] = 480;
    st.face[0] = st.face[1] = st.face[2] = st.face[3] = 1.0F;
    st.face_offset[0] = st.face_offset[1] = st.face_offset[2] = st.face_offset[3] = 0.0F;
    list_first = strip_first = vertex_count;
    while (p < end) {
        Uint32 pcw = p[0];

        switch (PCW_TYPE(pcw)) {
        case PARA_POLYGON:
        case PARA_SPRITE:
            flush_strip(&st, 0);
            if (list == LIST_OPAQUE_MODIFIER || list == LIST_TRANSLUCENT_MODIFIER) {
                have_header = 0;
                p += 8;
                break;
            }
            /* a header whose words are the last one's, as consecutive
             * meshsets of one material send, leaves the state as it is:
             * header_state is a function of them, the list and the user
             * clip, which drops the memo */
            if (have_header && !st.sprite && PCW_TYPE(pcw) == PARA_POLYGON && PCW_COLOR(pcw) != COLOR_INTENSITY &&
                pcw == st.pcw && p[1] == st.isp && p[2] == st.tsp && p[3] == st.tcw) {
                headers_same++;
            } else {
                read_header(&st, p);
            }
            have_header = 1;
            p += st.sprite ? 8 : header_words(pcw);
            break;
        case PARA_VERTEX:
            if (list == LIST_OPAQUE_MODIFIER || list == LIST_TRANSLUCENT_MODIFIER) {
                p += 16;
                break;
            }
            if (!have_header) {
                p += 8;
                break;
            }
            if (st.sprite) {
                read_sprite(&st, p);
            } else {
                read_vertex(&st, p);
                if (pcw & PCW_END_OF_STRIP) {
                    flush_strip(&st, 1);
                }
            }
            p += st.vertex_words;
            break;
        case PARA_USER_CLIP:
            /* the rectangle in tiles, its last tile's included */
            flush_strip(&st, 0);
            st.clip[0] = p[4] * TILE;
            st.clip[1] = p[5] * TILE;
            st.clip[2] = (p[6] + 1) * TILE;
            st.clip[3] = (p[7] + 1) * TILE;
            st.pcw = ~0u; /* the next header is read anew */
            p += 8;
            break;
        case PARA_MODEL:
            flush_strip(&st, 0);
            if (have_header && p[1] < (Uint32)run_count && runs[p[1]].index_count > 0) {
                const struct run *run = &runs[p[1]];
                int cull = ISP_CULL(st.isp);
                int pieces = run->part_count > 0 ? run->part_count : 1;
                int k;

                /* one strip for the run, or one for each of its parts, which
                 * the translucent list then sorts apart */
                for (k = 0; k < pieces; k++) {
                    struct strip *s;

                    if (strip_count >= STRIPS_MAX) {
                        full_once("render: the strips' table is full: a frame's strips dropped");
                        break;
                    }
                    s = &strips[strip_count++];
                    s->draw = st.draw;
                    s->depth_pass = st.depth_pass;
                    s->draw.cull = (unsigned char)(cull >= 2 ? cull : 0);
                    s->run = (int)p[1];
                    if (run->part_count > 0) {
                        const struct run_part *part = &parts[run->first_part + k];

                        s->first = run->first_index + part->first_index;
                        s->count = part->index_count;
                        s->depth = part->rhw_near;
                    } else {
                        s->first = run->first_index;
                        s->count = run->index_count;
                        s->depth = run->rhw_near;
                    }
                }
            }
            p += 8;
            break;
        case PARA_END_OF_LIST:
            flush_strip(&st, 0);
            have_header = 0;
            p += 8;
            break;
        default:
            p += 8;
            break;
        }
    }
    flush_strip(&st, 0);
}

/* the lists start again, after a render or without one, as a frame the game
 * discards: the runs their parameters named go with them */
void port_runs_reset(void)
{
    index_count = 0;
    vertex3_count = 0;
    run_count = 0;
    part_count = 0;
    lit_count = 0;
    depth_count = 0;
}

/* where the drawer writes a translucent run's vertices' depths, a float
 * each in view space, by the run's transform's third row, and the room
 * for them */
float *port_run_depths(int *room)
{
    *room = DEPTHS_MAX - depth_count;
    return DEPTHS + depth_count;
}

/* the depths port_run_depths gave, vertices of them, are the run's, which
 * port_run_end or port_run_kept just made */
void port_run_depths_take(int run, int vertices)
{
    if (run < 0 || run != run_count - 1 || vertices <= 0 || vertices > DEPTHS_MAX - depth_count) {
        return;
    }
    runs[run].depths = DEPTHS + depth_count;
    depth_count += vertices;
}

/* the identity rows of a run of points already on the screen
 * (xbox/engine/sa1/model.c's meshset_run). they lie in the port layer, which
 * links after the game's own data, so that they move none of it */
const float port_screen_rows[12] = {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F};

/* a run for the game's drawer to fill: where its vertices and its
 * triangles' indices go, an index counting from the run's first vertex, and
 * the room for each. its number, for the list's PARA_MODEL, or -1 where the
 * tables are full */
int port_run_begin(struct host_vertex **vertices, int *vertex_room, unsigned short **indices, int *index_room)
{
    if (vertex3_count == 0) {
        /* the frame's arena, asked for until a run has vertices in it: not
         * by the runs' count, which a kept model's run, with none here, may
         * have raised first. the drawer then wrote this frame's models over
         * the last frame's, which the host's chip was still drawing: models
         * torn across the screen, every other frame */
        arena = HOST->video_arena(&arena_room);
        if (arena == NULL) {
            arena_room = 0;
        }
    }
    if (run_count >= RUNS_MAX || vertex3_count >= VERTICES3_MAX || index_count >= INDICES_MAX / 2) {
        full_once("render: the model runs' tables are full: a model's meshsets projected by the drawer");
        return -1;
    }
    *vertices = &VERTICES3[vertex3_count];
    *vertex_room = VERTICES3_MAX - vertex3_count;
    if (*vertex_room > 0xffff) {
        *vertex_room = 0xffff;
    }
    *indices = &INDICES[index_count];
    /* half the indices' table at most, which the lists' strips share */
    *index_room = INDICES_MAX / 2 - index_count;
    return run_count;
}

/* where the drawer writes a translucent run's parts, one for each strip,
 * and the room for them: 0 where the table is full */
struct run_part *port_run_parts(int *room)
{
    *room = PARTS_MAX - part_count;
    return &parts[part_count];
}

/* the run filled, with its transform, as host_api's video_vertices3 takes
 * it with rhw_near and rhw_mid, and the parts port_run_parts gave that the
 * drawer filled, 0 for a run that draws as one. serial names the draw of
 * the model, the same for each of its runs */
void port_run_end(int run, int vertices, int indices, int part_total, const float transform[12], float rhw_near,
                  float rhw_mid, unsigned serial)
{
    struct run *r = &runs[run];
    int k;

    if (run != run_count) {
        return;
    }
    r->vertices = &VERTICES3[vertex3_count];
    r->count = vertices;
    r->first_index = index_count;
    r->index_count = indices;
    r->first_part = part_count;
    r->part_count = part_total;
    part_count += part_total;
    for (k = 0; k < 12; k++) {
        r->transform[k] = transform[k];
    }
    r->rhw_near = rhw_near;
    r->rhw_mid = rhw_mid;
    /* the runs of one draw of a model follow each other, and share the
     * first's transform, which the host then loads once */
    r->serial = serial;
    r->shared = run > 0 && runs[run - 1].serial == serial ? runs[run - 1].shared : run;
    r->palette = NULL;
    r->depths = NULL;
    r->sky = port_sky_drawing;
    vertex3_count += vertices;
    index_count += indices;
    run_count++;
}

/* the kept models. a model the drawer finds as it was when last drawn, by
 * the same light and palette, needn't be shaded or written again: its
 * vertices stay in the host's store (host_api's video_store), memory the
 * host's graphics chip reads, which holds from frame to frame, and its
 * triangles' indices here. the drawer asks for a model's records
 * (port_keep_next), builds one where none serves (port_keep_begin, the
 * vertices and indices written where it says, its info's set for each
 * meshset), and draws a meshset of it as a run (port_run_kept). a model may
 * have several records, each built under another light, and one that holds
 * no vertices, which the drawer keeps notes in. a record the drawer finds
 * stale it drops (port_keep_drop): its vertices stay until no frame the
 * chip may still be drawing names them.
 *
 * the store is first fit by address. where it has no gap for a model, the
 * records no frame has drawn lately make one: first those no model names
 * and those unused for KEPT_OLD frames, then any that neither this frame
 * nor the last drew, which the chip may still be reading */
#define KEPT_NONE 0xffffu
#define KEPT_BUCKETS 1024
#define KEPT_OLD 300
static struct host_vertex *store;
static int store_room;
static int kept_ready;
static Uint16 kept_buckets[KEPT_BUCKETS];
static Uint16 kept_free = KEPT_NONE;
static Uint16 store_head = KEPT_NONE;
static unsigned kept_made, kept_dropped, kept_evicted; /* for the log */
unsigned port_keep_why[6]; /* the drawer's, for the log too */
static int store_used;

static unsigned kept_bucket(const void *model)
{
    return (((Uint32)model >> 4) * 0x9e3779b1u) >> 22;
}

/* the frame the drawer is making: the renders so far, and one */
unsigned port_keep_frame(void)
{
    return frame_number + 1;
}

static void kept_open(void)
{
    int k;

    kept_ready = 1;
    store = HOST->video_store(&store_room);
    if (store == NULL) {
        store_room = 0;
    }
    if (store_room > STORE_MAX) {
        store_room = STORE_MAX;
    }
    for (k = 0; k < KEPT_BUCKETS; k++) {
        kept_buckets[k] = KEPT_NONE;
    }
    kept_free = KEPT_NONE;
    for (k = KEPT_MAX - 1; k >= 0; k--) {
        KEPT[k].model = NULL;
        KEPT[k].in_use = 0;
        KEPT[k].in_store = 0;
        KEPT[k].chain = kept_free;
        kept_free = (Uint16)k;
    }
    store_head = KEPT_NONE;
}

static void kept_unchain(int id)
{
    struct kept *r = &KEPT[id];
    Uint16 *link;

    if (r->model == NULL) {
        return;
    }
    for (link = &kept_buckets[kept_bucket(r->model)]; *link != KEPT_NONE; link = &KEPT[*link].chain) {
        if (*link == id) {
            *link = r->chain;
            break;
        }
    }
    r->model = NULL;
}

static void kept_release(int id)
{
    struct kept *r = &KEPT[id];

    kept_unchain(id);
    if (r->in_store) {
        if (r->before != KEPT_NONE) {
            KEPT[r->before].after = r->after;
        } else {
            store_head = r->after;
        }
        if (r->after != KEPT_NONE) {
            KEPT[r->after].before = r->before;
        }
        store_used -= r->count;
        r->in_store = 0;
    }
    r->in_use = 0;
    r->chain = kept_free;
    kept_free = (Uint16)id;
}

/* a record for model, with no vertices yet: -1 where none is free */
static int kept_new(const void *model)
{
    int id = kept_free;
    struct kept *r;
    unsigned bucket = kept_bucket(model);

    if (id == KEPT_NONE) {
        return -1;
    }
    r = &KEPT[id];
    kept_free = r->chain;
    __builtin_memset(r, 0, sizeof *r);
    r->model = model;
    r->in_use = 1;
    r->chain = kept_buckets[bucket];
    kept_buckets[bucket] = (Uint16)id;
    r->used = port_keep_frame();
    return id;
}

/* whether the store may take a record's vertices back: no frame the chip
 * may still draw names them, and at pass 0 none has for a while, or no
 * model names them */
static int kept_spare(const struct kept *r, int pass)
{
    unsigned now = port_keep_frame();

    if (r->used + 2 > now) {
        return 0;
    }
    return pass != 0 || r->model == NULL || r->used + KEPT_OLD <= now;
}

/* room for count vertices in the store, for record id: 0 where there is
 * none, even with every spare record gone */
static int store_take(int id, int count)
{
    int pass;

    for (pass = 0; pass < 2; pass++) {
        int start = 0;
        Uint16 last = KEPT_NONE; /* the record before the gap, which stays */
        Uint16 r = store_head;

        for (;;) {
            int gap_end = r != KEPT_NONE ? KEPT[r].first : store_room;

            if (gap_end - start >= count) {
                /* the gap from start: the spare records in its way go */
                Uint16 k = last != KEPT_NONE ? KEPT[last].after : store_head;
                struct kept *me = &KEPT[id];

                while (k != KEPT_NONE && KEPT[k].first < start + count) {
                    Uint16 next = KEPT[k].after;

                    kept_release(k);
                    kept_evicted++;
                    k = next;
                }
                me->first = start;
                me->count = count;
                me->before = last;
                me->after = k;
                if (last != KEPT_NONE) {
                    KEPT[last].after = (Uint16)id;
                } else {
                    store_head = (Uint16)id;
                }
                if (k != KEPT_NONE) {
                    KEPT[k].before = (Uint16)id;
                }
                me->in_store = 1;
                store_used += count;
                return 1;
            }
            if (r == KEPT_NONE) {
                break;
            }
            if (!kept_spare(&KEPT[r], pass)) {
                start = KEPT[r].first + KEPT[r].count;
                last = r;
            }
            r = KEPT[r].after;
        }
    }
    return 0;
}

/* the model's records, one after another: the first after -1, and -1
 * after the last */
int port_keep_next(const void *model, int after)
{
    Uint16 id;

    if (!kept_ready) {
        kept_open();
    }
    for (id = after < 0 ? kept_buckets[kept_bucket(model)] : KEPT[after].chain; id != KEPT_NONE; id = KEPT[id].chain) {
        if (KEPT[id].model == model) {
            return id;
        }
    }
    return -1;
}

struct keep_info *port_keep_info(int id)
{
    return &KEPT[id].info;
}

/* a record for model that holds no vertices, for the drawer's notes: -1
 * where none is free */
int port_keep_note(const void *model)
{
    return kept_new(model);
}

/* vertices of them in the store for a record of model's: reuse, one that
 * holds none, or a new one where reuse is -1. *out and *indices are where
 * they go, null where the store has no room, or no record is free: the
 * record, or -1. the store isn't asked again in a frame for as many as it
 * has just refused */
int port_keep_begin(const void *model, int reuse, int vertices, struct host_vertex **out, unsigned short **indices)
{
    static unsigned refused_frame;
    static int refused_count;
    int id = reuse;

    *out = NULL;
    *indices = NULL;
    if (!kept_ready) {
        kept_open();
    }
    if (vertices <= 0 || vertices > store_room ||
        (refused_frame == port_keep_frame() && vertices >= refused_count)) {
        return id;
    }
    if (id < 0) {
        id = kept_new(model);
        if (id < 0) {
            return -1;
        }
    }
    if (KEPT[id].in_store) {
        return id;
    }
    if (!store_take(id, vertices)) {
        refused_frame = port_keep_frame();
        refused_count = vertices;
        return id;
    }
    KEPT[id].used = port_keep_frame();
    *out = store + KEPT[id].first;
    *indices = KEPT_INDICES + KEPT[id].first * 3;
    kept_made++;
    return id;
}

/* the record's vertices are stale: no model names them now, and they stay
 * in the store until it wants their room */
void port_keep_drop(int id)
{
    kept_dropped++;
    if (KEPT[id].in_store) {
        kept_unchain(id);
    } else {
        KEPT[id].info.held = 0;
    }
}

/* a run of a kept model's meshset: its number, or -1 where the runs' table
 * is full. the transform, the depths, the parts and the serial are as
 * port_run_end's */
int port_run_kept(int id, int set, int part_total, const float transform[12], float rhw_near, float rhw_mid,
                  unsigned serial)
{
    struct kept *k = &KEPT[id];
    const struct keep_set *s = &k->info.set[set];
    struct run *r;
    int run = run_count;
    int n;

    if (run_count >= RUNS_MAX) {
        full_once("render: the model runs' table is full: a kept model's meshset dropped");
        return -1;
    }
    r = &runs[run];
    r->vertices = store + k->first + s->first;
    r->count = s->count;
    r->first_index = (int)(KEPT_INDICES - INDICES) + (k->first + s->first) * 3;
    r->index_count = (int)s->indices;
    r->first_part = part_count;
    r->part_count = part_total;
    part_count += part_total;
    for (n = 0; n < 12; n++) {
        r->transform[n] = transform[n];
    }
    r->rhw_near = rhw_near;
    r->rhw_mid = rhw_mid;
    r->serial = serial;
    r->shared = run > 0 && runs[run - 1].serial == serial ? runs[run - 1].shared : run;
    r->palette = NULL;
    r->depths = NULL;
    r->sky = port_sky_drawing;
    k->used = port_keep_frame();
    run_count++;
    return run;
}

/* run, a kept model's meshset, for the host to light (host.h's host_item):
 * its vertices hold their normals, and light is the light in the model's
 * space, as shade_points makes it. palette is the drawer's, 256 pairs of a
 * base color and an offset color, or null for a white base where an earlier
 * meshset turned the points white, and offsets the pairs whose offset
 * colors the run takes, the palette's or the next's, or null for none. the
 * snapshot of the two that the run draws with is the frame's first of
 * them, which the palettes' sums the drawer compares say holds for the
 * frame */
void port_run_lit(int run, const float light[3], const Uint32 *palette, const Uint32 *offsets)
{
    struct run *r;
    int k;

    if (run < 0 || run >= run_count) {
        return;
    }
    for (k = 0; k < lit_count; k++) {
        if (lit_from[k][0] == palette && lit_from[k][1] == offsets) {
            break;
        }
    }
    if (k == lit_count) {
        int n;

        if (lit_count == LIT_PALETTES) {
            /* more palettes in one frame than were ever seen: the last
             * one's, so the run draws, if in the wrong colors */
            full_once("render: the lit palettes' table is full: a model lit by another's palette");
            lit_full++;
            k = LIT_PALETTES - 1;
        } else {
            k = lit_count++;
            if (lit_count > lit_most) {
                lit_most = lit_count;
            }
        }
        lit_from[k][0] = palette;
        lit_from[k][1] = offsets;
        for (n = 0; n < 256; n++) {
            lit_palettes[k][n] = palette != NULL ? palette[n * 2] : 0x00ffffffu;
            lit_palettes[k][256 + n] = offsets != NULL ? offsets[n * 2] : 0;
        }
    }
    r = &runs[run];
    r->palette = lit_palettes[k];
    r->light[0] = light[0];
    r->light[1] = light[1];
    r->light[2] = light[2];
}

/* the models a weld writes (src/draw/motion.c's motion_weld): a few of
 * their points change each frame, which model.c's probe of a few points
 * may miss, so they are never kept (port_keep_never). a set by address,
 * open addressing; full, a model isn't added, logged once */
#define WELDED 512
static const void *welded[WELDED];
static int welded_count;

static unsigned welded_slot(const void *model)
{
    return (((Uint32)model >> 2) * 0x9e3779b1u) >> 23;
}

void port_keep_welded(const void *model)
{
    unsigned k = welded_slot(model);

    if (model == NULL) {
        return;
    }
    while (welded[k] != NULL) {
        if (welded[k] == model) {
            return;
        }
        k = (k + 1) % WELDED;
    }
    if (welded_count >= WELDED / 2) {
        full_once("render: the welded models' set is full: a welded model may be kept");
        return;
    }
    welded[k] = model;
    welded_count++;
}

/* whether model is a weld's, and never kept */
int port_keep_never(const void *model)
{
    unsigned k = welded_slot(model);

    while (welded[k] != NULL) {
        if (welded[k] == model) {
            return 1;
        }
        k = (k + 1) % WELDED;
    }
    return 0;
}


/* a kept record whose few probed words matched its model but whose whole
 * data didn't: the first 32 of a run logged */
/* the store's records, checked once a frame: in address order, none over
 * another or past the store, each linked both ways, their sum store_used,
 * and every record that says it is in the store among them. a record's
 * vertices written over another's would draw a model in another's shape:
 * the first fault of a run is logged, with the records it names */
static void kept_verify(void)
{
    static int logged;
    static char line[384];
    Uint16 r = store_head, before = KEPT_NONE;
    int end = 0, used = 0, listed = 0, in_store = 0, k;
    const char *fault = NULL;

    if (logged || !kept_ready) {
        return;
    }
    while (r != KEPT_NONE && fault == NULL) {
        const struct kept *e = &KEPT[r];

        if (r >= KEPT_MAX) {
            fault = "a link past the records";
            break;
        }
        if (!e->in_store) {
            fault = "a record in the list that isn't in the store";
        } else if (e->before != before) {
            fault = "a record whose link back is wrong";
        } else if (e->first < end) {
            fault = "a record over the one before it";
        } else if (e->count <= 0 || e->first + e->count > store_room) {
            fault = "a record past the store";
        }
        if (fault == NULL) {
            end = e->first + e->count;
            used += e->count;
            listed++;
            before = r;
            r = e->after;
        }
        if (listed > KEPT_MAX) {
            fault = "a loop in the list";
        }
    }
    for (k = 0; k < KEPT_MAX && fault == NULL; k++) {
        in_store += KEPT[k].in_store;
    }
    if (fault == NULL && in_store != listed) {
        fault = "a record in the store that isn't in the list";
    }
    if (fault == NULL && used != store_used) {
        fault = "the store's count, not its records' sum";
    }
    if (fault == NULL) {
        return;
    }
    logged = 1;
    sprintf(line, "render: kept models: %s, at record %d (first %d, count %d, before %d); the one before %d ends at %d; "
                  "%d listed, %d in the store, %d of %d used by the sum, %d by the count; frame %u",
            fault, r == KEPT_NONE ? -1 : (int)r, r < KEPT_MAX ? KEPT[r].first : -1, r < KEPT_MAX ? KEPT[r].count : -1,
            r < KEPT_MAX ? (int)KEPT[r].before : -1, before == KEPT_NONE ? -1 : (int)before, end, listed, in_store,
            used, store_room, store_used, frame_number);
    port_log(line);
}

/* the records no model has asked for in a long while go, a few a frame */
static void kept_sweep(void)
{
    static int at;
    unsigned now = port_keep_frame();
    int k;

    if (!kept_ready) {
        return;
    }
    for (k = 0; k < 32; k++, at = (at + 1) % KEPT_MAX) {
        struct kept *r = &KEPT[at];

        if (r->in_use && r->used + 4 * KEPT_OLD <= now) {
            kept_release(at);
        }
    }
}

/* the fog registers Kamui's kmglobal_kamui_lib.c writes: the table's color,
 * the vertex fog's, the density, a mantissa over 128 in bits 8 to 15 and a
 * signed power of 2 in bits 0 to 7, and the table */
#define REG_FOG_TABLE_COLOR 0x0b0
#define REG_FOG_VERTEX_COLOR 0x0b4
#define REG_FOG_DENSITY 0x0b8
#define REG_FOG_TABLE 0x200

static void send_fog(void)
{
    static unsigned short table[128];
    KMDWORD density = port_register(REG_FOG_DENSITY);
    int exponent = (signed char)(density & 0xff);
    float scale = (float)((density >> 8) & 0xff) / 128.0F;
    int k;

    for (; exponent > 0; exponent--) {
        scale *= 2.0F;
    }
    for (; exponent < 0; exponent++) {
        scale *= 0.5F;
    }
    for (k = 0; k < 128; k++) {
        table[k] = (unsigned short)port_register(REG_FOG_TABLE + k * 4);
    }
    HOST->video_fog(port_register(REG_FOG_TABLE_COLOR), port_register(REG_FOG_VERTEX_COLOR), scale, table);
}

/* the movie's frame, which xbox/port/movie.c keeps, as a quad under the
 * lists at the player's brightness */
int port_movie_frame(unsigned *texture, int *width, int *height, int *bright);

static void draw_movie(void)
{
    static struct host_vertex quad[4];
    static const Uint16 corners[6] = {0, 1, 2, 1, 3, 2};
    struct host_draw d;
    unsigned texture;
    int width, height, bright;
    float w, h;
    float left, top;
    Uint32 color;
    int k;

    if (!port_movie_frame(&texture, &width, &height, &bright) || width <= 0) {
        return;
    }
    /* Sofdec's movies are half the screen's width, which the console shows
     * doubled across, and their own height, centered */
    w = width <= 320 ? width * 2.0F : (float)width;
    h = (float)height;
    left = (640.0F - w) / 2.0F;
    top = (480.0F - h) / 2.0F;
    bright = bright < 0 ? 0 : bright > 255 ? 255 : bright;
    color = 0xff000000u | (Uint32)bright << 16 | (Uint32)bright << 8 | (Uint32)bright;
    {
        const float xs[4] = {0, 640, 0, 640};
        const float ys[4] = {0, 0, 1, 1};

        for (k = 0; k < 4; k++) {
            quad[k].x = left + xs[k] / 640.0F * w;
            quad[k].y = top + ys[k] * h;
            quad[k].rhw = 1.0F / 65536.0F;
            quad[k].u = xs[k] / 640.0F;
            quad[k].v = ys[k];
            quad[k].base = color;
            quad[k].offset = 0;
        }
    }
    __builtin_memset(&d, 0, sizeof d);
    d.texture = texture;
    d.src_blend = 1;
    d.dst_blend = 0;
    d.depth_compare = 7;
    d.depth_write = 0;
    d.shading = 1;
    d.use_alpha = 1;
    d.clamp_u = d.clamp_v = 1;
    d.filter = 1;
    d.fog = 2;
    HOST->video_vertices(quad, 4);
    HOST->video_draw(&d, corners, 6);
}

/* the background plane's color, which clears the frame */
static Uint32 background_color(void)
{
    return kmiDeviceExtension.background_packet[6] | 0xff000000u;
}

#define ITEMS 256
static struct host_item items[ITEMS];
static int item_count;

/* back to front, stable: order's first n strips by their nearest rhw,
 * smallest first, a merge sort from runs of one up. because an insertion
 * sort's cost goes as the square of the count, and a busy stage's
 * translucent list holds a part for each translucent meshset, a merge sort
 * is used. its scratch is the
 * sort by triangle's room (below) */
#define TRIS_MAX 8192
/* the sorts' room, in the image's data, since the renderer's is full: the
 * sort by triangle's keys and their scratch, or the sort by strip's
 * scratch. 1st_read's data has some 180 KB to spare past it, of the area
 * pclink.py holds it to: past the area it ran into the sound driver's */
static unsigned long long sort_room[2 * TRIS_MAX];
#define sort_scratch ((int *)sort_room)
_Static_assert(STRIPS_MAX * sizeof(int) <= sizeof sort_room, "the sort by strip's scratch outgrows the sorts' room");
static unsigned long long sort_tsc; /* the frame's, for the report */

static void sort_back_to_front(int n)
{
    int *from = order, *to = sort_scratch;
    int width;

    for (width = 1; width < n; width *= 2) {
        int lo;

        for (lo = 0; lo < n; lo += 2 * width) {
            int mid = lo + width < n ? lo + width : n;
            int hi = lo + 2 * width < n ? lo + 2 * width : n;
            int i = lo, j = mid, k = lo;

            while (i < mid && j < hi) {
                /* the left's on a tie: stable. an additive strip's after
                 * every other (header_state) */
                const struct strip *a = &strips[from[i]], *b = &strips[from[j]];
                int later = (b->draw.dst_blend == 1) < (a->draw.dst_blend == 1) ||
                            ((b->draw.dst_blend == 1) == (a->draw.dst_blend == 1) && b->depth < a->depth);

                to[k++] = later ? from[j++] : from[i++];
            }
            while (i < mid) {
                to[k++] = from[i++];
            }
            while (j < hi) {
                to[k++] = from[j++];
            }
        }
        {
            int *t = from;

            from = to;
            to = t;
        }
    }
    if (from != order) {
        __builtin_memcpy(order, from, (unsigned)n * sizeof *order);
    }
}

/* hand the host the draw of count indices from indices, by s's state,
 * and its depth pass where s has one */
static void draw_item(const struct strip *s, const Uint16 *indices, int count)
{
    /* the draws go to the host some hundreds at a time */
    struct host_item *item = &items[item_count++];

    item->draw = s->draw;
    item->indices = indices;
    item->count = count;
    if (s->run >= 0) {
        const struct run *run = &runs[s->run];

        item->vertices = run->vertices;
        item->vertex_count = run->count;
        item->transform = runs[run->shared].transform;
        item->rhw_near = run->rhw_near;
        item->rhw_mid = run->rhw_mid;
        item->palette = run->palette;
        item->draw.sky = (unsigned char)run->sky;
        item->light[0] = run->light[0];
        item->light[1] = run->light[1];
        item->light[2] = run->light[2];
    } else {
        item->vertices = NULL;
        item->vertex_count = 0;
        item->transform = NULL;
        item->rhw_near = item->rhw_mid = 0.0F;
        item->palette = NULL;
    }
    if (item_count == ITEMS) {
        HOST->video_draw_items(items, item_count);
        item_count = 0;
    }
    if (s->depth_pass) {
        /* once more for the depth: no color (zero times the source,
         * one times what's there), of the fragments half opaque */
        struct host_item *depth = &items[item_count++];

        *depth = *item;
        depth->draw.src_blend = 0;
        depth->draw.dst_blend = 1;
        depth->draw.depth_write = 1;
        depth->draw.alpha_ref = 0.5F;
        if (item_count == ITEMS) {
            HOST->video_draw_items(items, item_count);
            item_count = 0;
        }
    }
}

/* the translucent list's sort: 0 by strip, by its nearest vertex; 1 by
 * triangle, by its nearest vertex; 2 by triangle, by its middle (the mean
 * of its vertices' z). poked by name (HOST_POKE) to compare */
volatile int port_sort_triangles = 1;

/* the triangles the sort by triangle takes in a frame, TRIS_MAX: past them
 * the frame sorts by strip, logged once. each has a key, its depth's bits
 * above and its strip's place in the list and its own in the strip below,
 * so that a tie keeps the list's order; the scratch then holds the indices
 * in the sorted order */
#define TRI_BITS 18
#define tri_keys sort_room
#define tri_scratch (sort_room + TRIS_MAX)
static unsigned tris_sorted, tris_batches; /* for the profile's line */

/* the bits of a positive rhw, which order as it does; 0 for one that isn't */
static Uint32 depth_bits(float rhw)
{
    union {
        float f;
        Uint32 u;
    } bits;

    bits.f = rhw;
    return rhw > 0.0F ? bits.u : 0;
}

/* the triangle's depth, as the sort by triangle keys it: the rhw of its
 * nearest vertex, or 1 over the mean of its vertices' z */
static float triangle_depth(const struct strip *s, const Uint16 *at, int middle)
{
    float z[3];
    int i;

    if (s->run >= 0 && runs[s->run].depths != NULL) {
        /* the depths the drawer noted, in cached memory */
        const float *d = runs[s->run].depths;

        z[0] = d[at[0]];
        z[1] = d[at[1]];
        z[2] = d[at[2]];
    } else if (s->run >= 0) {
        /* a model's point, in its space: its z by the transform's third
         * row, read from the host's memory, which the CPU reads uncached */
        const struct run *run = &runs[s->run];
        const float *t = runs[run->shared].transform + 8;

        for (i = 0; i < 3; i++) {
            const struct host_vertex *v = &run->vertices[at[i]];

            z[i] = t[0] * v->x + t[1] * v->y + t[2] * v->rhw + t[3];
        }
    } else {
        for (i = 0; i < 3; i++) {
            float rhw = VERTICES[list_first + at[i]].rhw;

            if (!middle) {
                z[i] = rhw; /* the largest rhw is the nearest */
                continue;
            }
            z[i] = rhw > 0.0F ? 1.0F / rhw : 1.0e30F;
        }
        if (!middle) {
            float most = z[0] > z[1] ? z[0] : z[1];

            return z[2] > most ? z[2] : most;
        }
    }
    if (middle) {
        float mean = (z[0] + z[1] + z[2]) * (1.0F / 3.0F);

        return mean > 0.0F ? 1.0F / mean : 0.0F;
    } else {
        float least = z[0] < z[1] ? z[0] : z[1];

        least = z[2] < least ? z[2] : least;
        return least > 0.0F ? 1.0F / least : 1.0e30F;
    }
}

/* back to front by triangle: the keys of strips first to last, sorted by
 * their depth bits, a stable radix sort of four 8-bit passes from the
 * low bits up, which leaves a tie in the list's order. FALSE, with nothing drawn,
 * where the triangles are more than TRIS_MAX */
static int draw_triangles(int first, int last)
{
    static unsigned counts[4][256];
    unsigned long long *from = tri_keys, *to = tri_scratch;
    Uint16 *out = (Uint16 *)tri_scratch;
    int middle = port_sort_triangles == 2;
    int n = 0;
    int k, pass;

    for (k = first; k < last; k++) {
        n += strips[k].count / 3;
    }
    if (n > TRIS_MAX || last - first > (1 << (32 - TRI_BITS))) {
        full_once("render: the translucent triangles are more than the sort's room: the frame sorts by strip");
        return 0;
    }
    __builtin_memset(counts, 0, sizeof counts);
    n = 0;
    for (k = first; k < last; k++) {
        const struct strip *s = &strips[k];
        const Uint16 *at = INDICES + s->first;
        int tris = s->count / 3;
        int t;

        for (t = 0; t < tris; t++, at += 3) {
            /* an additive strip's after every other: see header_state */
            Uint32 bits = depth_bits(triangle_depth(s, at, middle)) | (s->draw.dst_blend == 1 ? 0x80000000u : 0);

            counts[0][bits & 0xff]++;
            counts[1][(bits >> 8) & 0xff]++;
            counts[2][(bits >> 16) & 0xff]++;
            counts[3][bits >> 24]++;
            from[n++] = (unsigned long long)bits << 32 | (Uint32)((k - first) << TRI_BITS | t);
        }
    }
    for (pass = 0; pass < 4; pass++) {
        unsigned *c = counts[pass];
        unsigned sum = 0;
        int shift = 32 + pass * 8;
        int i;

        /* a pass whose digit is the same in every key moves nothing */
        if (c[(from[0] >> shift) & 0xff] == (unsigned)n) {
            continue;
        }
        for (i = 0; i < 256; i++) {
            unsigned here = c[i];

            c[i] = sum;
            sum += here;
        }
        for (i = 0; i < n; i++) {
            to[c[(from[i] >> shift) & 0xff]++] = from[i];
        }
        {
            unsigned long long *t = from;

            from = to;
            to = t;
        }
    }
    if (from != tri_keys) {
        __builtin_memcpy(tri_keys, from, (unsigned)n * sizeof *tri_keys);
    }
    /* the sorted triangles' indices, into the scratch, and consecutive ones
     * of one state drawn as one batch */
    k = 0;
    while (k < n) {
        int place = (int)((Uint32)tri_keys[k] >> TRI_BITS);
        const struct strip *s = &strips[first + place];
        Uint16 *batch = out;
        int next = k;

        while (next < n) {
            int other = (int)((Uint32)tri_keys[next] >> TRI_BITS);
            const struct strip *o = &strips[first + other];
            const Uint16 *at = INDICES + o->first + ((Uint32)tri_keys[next] & ((1u << TRI_BITS) - 1)) * 3;

            if (other != place && (o->run != s->run || o->depth_pass != s->depth_pass ||
                                   __builtin_memcmp(&o->draw, &s->draw, sizeof s->draw) != 0)) {
                break;
            }
            out[0] = at[0];
            out[1] = at[1];
            out[2] = at[2];
            out += 3;
            next++;
        }
        draw_item(s, batch, (int)(out - batch));
        tris_batches++;
        k = next;
    }
    tris_sorted += (unsigned)n;
    return 1;
}

/* the stage's sky task (src/stage/stage.c's stage_create_sky), and whether
 * its draws are running now (src/system/present.c): the model runs made
 * then are the backdrop's */
void *port_sky_task;
volatile int port_sky_drawing;
static unsigned sky_moved; /* frames whose sky sky_order moved, for the profile */

static int sky_strip(const struct strip *s)
{
    return s->run >= 0 && runs[s->run].sky;
}

/* a strip whose place in the order counts: no depth written, or a test
 * other than the near one (rhw greater, or greater or equal) */
static int order_counts(const struct strip *s)
{
    return !s->draw.depth_write || (s->draw.depth_compare != 4 && s->draw.depth_compare != 6);
}

/* an unsorted list's order with its sky later: the PowerVR draws what's
 * hidden for nothing, the NV2A pays for each pixel. the sky, drawn mid
 * frame (its task is level 2), was drawn over by the strips after it, and
 * drew its own pixels over those before, which then failed the depth test.
 * drawn after the other strips it fails the test where they lie, which the
 * NV2A tells sooner than it draws them. the order holds to the first strip
 * after the sky whose place counts (order_counts), which draws over the sky
 * by its order: the sky's strips before it go just before it, and it and
 * everything after it as they were. where a sky strip's own place counts
 * (a sky drawn without depth, which everything draws over), nothing moves.
 * FALSE where nothing moved, and order is left for the caller */
static int sky_order(int first, int last, int *count)
{
    int sky_first = -1;
    int cut;
    int n = 0;
    int k;

    for (k = first; k < last; k++) {
        if (sky_strip(&strips[k])) {
            if (order_counts(&strips[k])) {
                return FALSE;
            }
            if (sky_first < 0) {
                sky_first = k;
            }
        }
    }
    if (sky_first < 0) {
        return FALSE;
    }
    for (cut = sky_first; cut < last; cut++) {
        if (!sky_strip(&strips[cut]) && order_counts(&strips[cut])) {
            break;
        }
    }
    for (k = first; k < cut; k++) {
        if (!sky_strip(&strips[k])) {
            order[n++] = k;
        }
    }
    for (k = sky_first; k < cut; k++) {
        if (sky_strip(&strips[k])) {
            order[n++] = k;
        }
    }
    for (k = cut; k < last; k++) {
        order[n++] = k;
    }
    *count = n;
    sky_moved++;
    return TRUE;
}

static void draw_strips(int first, int last, int sorted)
{
    int k;
    int n = 0;

    if (sorted && port_sort_triangles != 0 && last > first) {
        unsigned long long at = port_profile ? __builtin_ia32_rdtsc() : 0;
        int drawn = draw_triangles(first, last);

        if (port_profile) {
            sort_tsc += __builtin_ia32_rdtsc() - at;
        }
        if (drawn) {
            if (item_count != 0) {
                HOST->video_draw_items(items, item_count);
                item_count = 0;
            }
            return;
        }
    }
    if (sorted || !sky_order(first, last, &n)) {
        for (k = first; k < last; k++) {
            order[n++] = k;
        }
    }
    if (sorted) {
        unsigned long long at = port_profile ? __builtin_ia32_rdtsc() : 0;

        sort_back_to_front(n);
        if (port_profile) {
            sort_tsc += __builtin_ia32_rdtsc() - at;
        }
    }
    k = 0;
    while (k < n) {
        const struct strip *s = &strips[order[k]];
        int count = s->count;
        int next = k + 1;

        /* consecutive strips of one state draw as one batch, where their
         * indices lie together */
        while (next < n && strips[order[next]].first == s->first + count && strips[order[next]].run == s->run &&
               strips[order[next]].depth_pass == s->depth_pass && __builtin_memcmp(&strips[order[next]].draw, &s->draw, sizeof s->draw) == 0) {
            count += strips[order[next]].count;
            next++;
        }
        draw_item(s, INDICES + s->first, count);
        k = next;
    }
    if (item_count != 0) {
        HOST->video_draw_items(items, item_count);
        item_count = 0;
    }
}

/* kmRender's hand-off: draw a frame of the lists */
void port_render(PKMDWORD starts[LISTS], PKMDWORD ends[LISTS])
{
    unsigned long long tsc = port_profile ? __builtin_ia32_rdtsc() : 0;
    unsigned long long lists_tsc = 0, draws_tsc = 0;

    static const int lists_in_order[3] = {LIST_OPAQUE, LIST_PUNCH_THROUGH, LIST_TRANSLUCENT};
    int k;

    frame_number++;
    /* a host whose room for textures filled has dropped them all: each is
     * decoded again as it's next drawn */
    if (HOST->video_textures_lost()) {
        int k;

        for (k = 0; k < CACHE_SIZE; k++) {
            cache[k].id = 0;
        }
    }
    /* the indices so far are the model runs', made since the last render:
     * the lists' follow them */
    strip_count = 0;
    vertex_count = 0;
    HOST->video_begin(background_color());
    send_fog();
    draw_movie();
    for (k = 0; k < 3; k++) {
        int list = lists_in_order[k];
        int first = strip_count;
        unsigned long long at = port_profile ? __builtin_ia32_rdtsc() : 0;

        /* the list's vertices to the host, then its strips, which name
         * them by index */
        read_list(list, (const Uint32 *)starts[list], (const Uint32 *)ends[list]);
        if (port_profile) {
            unsigned long long read = __builtin_ia32_rdtsc();

            lists_tsc += read - at;
            at = read;
        }
        if (strip_count > first) {
            HOST->video_vertices(VERTICES + list_first, vertex_count - list_first);
            draw_strips(first, strip_count, list == LIST_TRANSLUCENT);
        }
        if (port_profile) {
            draws_tsc += __builtin_ia32_rdtsc() - at;
        }
    }
    /* the runs are drawn: the drawer starts the next render's */
    if (index_count > most_indices) {
        most_indices = index_count;
    }
    if (run_count > most_runs) {
        most_runs = run_count;
    }
    if (part_count > most_parts) {
        most_parts = part_count;
    }
    if ((unsigned)vertex3_count + port_run_unfit_vertices > most_arena_wanted) {
        most_arena_wanted = (unsigned)vertex3_count + port_run_unfit_vertices;
    }
    port_run_unfit_vertices = 0;
    if (vertex3_count > most_arena) {
        most_arena = vertex3_count;
    }
    for (k = 0; k < LISTS; k++) {
        list_bytes[k] = (int)((char *)ends[k] - (char *)starts[k]);
        if (list_bytes[k] > most_list_bytes[k]) {
            most_list_bytes[k] = list_bytes[k];
        }
    }
    port_runs_reset();
    kept_sweep();
    kept_verify();
    if (strip_count > most_strips) {
        most_strips = strip_count;
    }
    if (vertex_count > most_vertices) {
        most_vertices = vertex_count;
    }
    if (port_profile) {
        unsigned long long spent = __builtin_ia32_rdtsc() - tsc;

        port_counts.render_tsc += spent;
        port_report_frame(spent, lists_tsc, draws_tsc, sort_tsc, strip_count, vertex_count);
        sort_tsc = 0;
    }
    /* the tables' use and the counts every 60 frames, in the log, where the
     * host asks for the profile (port_profile's bit 2: HOST_PROFILE=1); the
     * report gives them on demand */
    if (frame_number % 60 == 0) {
        static char line[192];
        static char most[192];
        int logging = (port_profile & 4) != 0;

        sprintf(most, "render: the most so far: %d strips, %d vertices, %d indices, %d model runs, a texture of %u texels",
                most_strips, most_vertices, most_indices, most_runs, (unsigned)most_texels);
        if (logging) {
            port_log(most);
        }

        sprintf(line, "render: frame %u: %d strips, %d vertices, lists %d %d %d bytes, modifiers %d %d",
                           frame_number, strip_count, vertex_count, (int)((char *)ends[0] - (char *)starts[0]),
                           (int)((char *)ends[4] - (char *)starts[4]), (int)((char *)ends[2] - (char *)starts[2]),
                           (int)((char *)ends[1] - (char *)starts[1]), (int)((char *)ends[3] - (char *)starts[3]));
        if (logging) {
            port_log(line);
        }
        /* the profile's counts, a frame's mean over the 60: where the
         * vertices come from, and the models the game changed in place */
        if (port_profile) {
            static char counts[384];
            static unsigned vertices_read;
            static unsigned last_us;
            static unsigned long long last_tsc;
            extern unsigned port_ninja_vertices;
            unsigned now_us = HOST->time_us();
            unsigned long long now_tsc = __builtin_ia32_rdtsc();
            unsigned us = now_us - last_us;
            unsigned long long per_ms = us != 0 ? (now_tsc - last_tsc) * 1000 / us : 0;
            unsigned host_us[4] = {0, 0, 0, 0};

            port_report_host_us(host_us);
            port_shown.gpu_wait10 = host_us[0] / 6000;
            port_shown.blank_wait10 = host_us[1] / 6000;
            port_shown.sound10 = host_us[2] / 6000;

            /* the same for the screen (port_overlay_draw): the 60 frames'
             * time, by the host's clock, and the timestamp counter's rate
             * over them, for the drawer's and the renderer's counts */
            if (last_us != 0 && us != 0 && per_ms != 0) {
                port_shown.fps10 = (unsigned)(600000000ull / us);
                port_shown.frame10 = us / 6000;
                port_shown.drawer10 = (unsigned)(port_counts.drawer_tsc * 10 / 60 / per_ms);
                port_shown.render10 = (unsigned)(port_counts.render_tsc * 10 / 60 / per_ms);
                port_shown.vertices = (port_vertices_read - vertices_read) / 60 + port_counts.gpu_vertices / 60;
                port_shown.gpu_vertices = port_counts.gpu_vertices / 60;
                port_shown.models = port_counts.drawer_models / 60;
            }
            {
                unsigned worst = 0, k;

                for (k = 1; k <= 60 && k <= port_frame_count; k++) {
                    unsigned v = port_frames[(port_frame_count - k) % PORT_FRAMES].us[PHASE_FRAME];

                    worst = v > worst ? v : worst;
                }
                port_shown.worst10 = worst / 100;
            }
            last_us = now_us;
            last_tsc = now_tsc;
            port_counts.ninja_vertices = port_ninja_vertices;
            port_ninja_vertices = 0;

            sprintf(counts,
                    "render: a frame's vertices: %u in the lists, %u from the game's drawer (%u models, %u points; "
                    "%u models, %u points changed), %u of them for the host to transform, %u from njModel; textures "
                    "loaded: %u by the pack, %u copied",
                    (port_vertices_read - vertices_read) / 60, port_counts.drawer_vertices / 60,
                    port_counts.drawer_models / 60, port_counts.drawer_points / 60, port_counts.changed_models / 60,
                    port_counts.changed_points / 60, port_counts.gpu_vertices / 60, port_counts.ninja_vertices / 60,
                    port_counts.packed_loads,
                    port_counts.copied_loads);
            if (logging) {
                port_log(counts);
            }
            sprintf(counts, "render: a frame's timestamp counts, in thousands: %u in the game's drawer, %u in the renderer",
                    (unsigned)(port_counts.drawer_tsc / 60000), (unsigned)(port_counts.render_tsc / 60000));
            if (logging) {
                port_log(counts);
            }
            sprintf(counts,
                    "render: kept models: a frame's %u vertices drawn from the store, %u built into it, %u of other "
                    "models written anew; the store holds %d of %d; %u builds, %u drops, %u evictions so far",
                    port_counts.kept_vertices / 60, port_counts.built_vertices / 60,
                    (port_counts.gpu_vertices - port_counts.kept_vertices - port_counts.built_vertices) / 60, store_used,
                    store_room, kept_made, kept_dropped, kept_evicted);
            if (logging) {
                port_log(counts);
            }
            sprintf(counts,
                    "render: kept models: in 60 frames, dropped for the signature %u, the whole sum %u, the sampled sum "
                    "%u; draws under a light past the model's records %u, of models not kept for now %u, that can't "
                    "be kept %u",
                    port_keep_why[0], port_keep_why[2], port_keep_why[3], port_keep_why[1], port_keep_why[4],
                    port_keep_why[5]);
            if (logging) {
                port_log(counts);
            }
            __builtin_memset(port_keep_why, 0, sizeof port_keep_why);
            sprintf(counts, "render: a frame's headers the same as the last one's, not read again: %u",
                    headers_same / 60);
            if (logging) {
                port_log(counts);
            }
            headers_same = 0;
            sprintf(counts, "render: the translucent sort: %u triangles a frame, in %u draws; %u triangles of no number "
                            "dropped in 60 frames; the sky drawn last of its list %u times",
                    tris_sorted / 60, tris_batches / 60, nonfinite_triangles, sky_moved);
            nonfinite_triangles = 0;
            sky_moved = 0;
            if (logging) {
                port_log(counts);
            }
            tris_sorted = tris_batches = 0;
            vertices_read = port_vertices_read;
            __builtin_memset(&port_counts, 0, sizeof port_counts);
            port_report_counts_reset();
        }
    }
}

/* the report's lines for the renderer's tables (report.c): the most a frame
 * has used of each against its size, the last frame's use, and the kept
 * models' store */
void port_render_tables(void)
{
    static const int list_sizes[LISTS] = PORT_LIST_SIZES;
    static const char *const list_names[LISTS] = {"opaque", "opaque modifiers", "translucent", "translucent modifiers",
                                                  "punch-through"};
    /* because a line past 200 characters overran the buffer into the
     * state after it, the buffer is 320 */
    static char line[320];
    int k;
    int used = 0;

    for (k = 0; k < CACHE_SIZE; k++) {
        used += cache[k].id != 0;
    }
    sprintf(line, "report: tables, the most a frame used of each since the start, and its size:");
    port_log(line);
    sprintf(line, "report:   strips %d of %d, list vertices %d of %d, indices %d of %d", most_strips, STRIPS_MAX,
            most_vertices, VERTICES_MAX, most_indices, INDICES_MAX);
    port_log(line);
    sprintf(line, "report:   model runs %d of %d, translucent parts %d of %d, the models' arena %d of %d vertices",
            most_runs, RUNS_MAX, most_parts, PARTS_MAX, most_arena, arena_room);
    port_log(line);
    sprintf(line, "report:   the most a frame wanted of the arena %u vertices; %u meshsets past it, sent through "
            "their lists, so far",
            most_arena_wanted, port_run_unfit);
    port_log(line);
    sprintf(line, "report:   a decoded texture %u of %d texels; the texture cache holds %d of %d", (unsigned)most_texels,
            DECODED_MAX, used, CACHE_SIZE);
    port_log(line);
    sprintf(line, "report:   lit palettes %d of %d in a frame at the most; runs lit by another's palette, the table full, %u",
            lit_most, LIT_PALETTES, lit_full);
    port_log(line);
    sprintf(line, "report:   kept models' store: %d of %d vertices now; %u builds, %u drops, %u evictions so far",
            store_used, store_room, kept_made, kept_dropped, kept_evicted);
    port_log(line);
    for (k = 0; k < LISTS; k++) {
        if (k == LIST_OPAQUE_MODIFIER || k == LIST_TRANSLUCENT_MODIFIER) {
            continue;
        }
        sprintf(line, "report:   Kamui's %s list: %d KB the last frame, %d KB the most, of %d KB", list_names[k],
                list_bytes[k] >> 10, most_list_bytes[k] >> 10, list_sizes[k] >> 10);
        port_log(line);
    }
}
