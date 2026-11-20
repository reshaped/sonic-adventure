/* the PSP's raster: engine/pc/raster.h's, drawn by the PSP's graphics engine
 * through the SDK's sceGu calls. a picture is the batches in the order
 * given: the engine blends in that order, where the PowerVR and the PC
 * sort a pixel's fragments, so a picture whose blends cross shows them in
 * the game's order.
 *
 * what the engine has no part for is left out: a batch's offset color and
 * its fog, a texture's mirrored repeat, which repeats, and the levels below
 * a texture's first. a texture is 16 bits a texel: 5650 where every texel
 * is opaque, 5551 where each is opaque or clear, else 4444.
 *
 * depth is 1/z, as the PowerVR compares it and as State's depth names its
 * compares: z = 1 is the nearest and fills the buffer's 16 bits */
#include <malloc.h>

#include <algorithm>
#include <pspdisplay.h>
#include <pspge.h>
#include <pspgu.h>
#include <pspkernel.h>
#include <stdio.h>
#include <string.h>

#include "draw.h"
#include "pvr.h"
#include "raster.h"

extern "C" {
#include "psp.h"
}

namespace pc {

namespace {

constexpr int SCREEN_WIDTH = 480;
constexpr int SCREEN_HEIGHT = 272;
constexpr int BUFFER_WIDTH = 512;
/* a 4:3 picture's width on the screen, in its middle */
constexpr int NARROW_WIDTH = 362;
/* the two color buffers and the depth buffer, in video memory */
constexpr uintptr_t DRAW_AT = 0;
constexpr uintptr_t SHOW_AT = BUFFER_WIDTH * SCREEN_HEIGHT * 4;
constexpr uintptr_t DEPTH_AT = SHOW_AT * 2;
constexpr float DEPTH_MOST = 65535.0F;
/* a picture's commands and corners lie in the game's own memory: the
 * loader's work area, 2 MB of the console's RAM that the game writes while
 * it loads a file and leaves alone between, but for a memory card's file,
 * which it builds from the area's start and from its middle
 * (src/save/card.h). LIST is the list the engine reads, and KEPT holds the
 * corners of the strips kept for the picture's end, each clear of both. a
 * load says it is about to write the area (psp_draw_yield), and the engine
 * draws what the list holds before it does. so the PSP's heap holds no
 * megabyte for the list */
constexpr unsigned long LIST_AT = 0x0cd40000UL;
constexpr uint32_t LIST_BYTES = 0xc0000;
constexpr unsigned long KEPT_AT = 0x0ce40000UL;
constexpr uint32_t KEPT_BYTES = 0xc0000;
/* the commands a batch takes, at most, in bytes */
constexpr uint32_t BATCH_BYTES = 256;
/* the corners one draw call takes, whole triangles, a fraction of the
 * list */
constexpr uint32_t CALL_MOST = 16383;
constexpr uint32_t SIDE_MOST = 512;
/* a texture of this many texels a side, or more, keeps half of them */
constexpr uint32_t HALVED = 128;
/* a run keeps one picture in this many */
constexpr uint32_t PICTURE_EVERY = 30;

/* a corner as the engine reads it under CORNER_FORMAT */
struct Corner {
    float u, v;
    uint32_t color;
    float x, y, z;
};
constexpr int CORNER_FORMAT =
    GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF;
/* a psp_corner's, as the walk's strips come */
constexpr int STRIP_FORMAT =
    GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D;

struct Texture {
    void *texels = nullptr;
    uint16_t width = 0, height = 0;
    uint8_t format = 0;
    bool swizzled = false;
    /* its texels are clear or solid, but for an edge's few between: a
     * triangle of it cuts at half, and needs no blend */
    bool cuts = false;
    uint32_t bytes = 0; /* its texels' */
    bool video = false; /* they lie in video memory */
};

/* State's depth compares, on a buffer where the nearer is the greater */
const int DEPTH[8] = {GU_NEVER,   GU_LESS,     GU_EQUAL,  GU_LEQUAL,
                      GU_GREATER, GU_NOTEQUAL, GU_GEQUAL, GU_ALWAYS};

/* a blend's factor as the engine names it. its 0 and 1 are the other side's
 * color and that color's complement, on either side */
void factor(uint8_t instruction, int &name, uint32_t &fixed)
{
    static const int NAMES[8] = {GU_FIX,       GU_FIX,
                                 GU_SRC_COLOR, GU_ONE_MINUS_SRC_COLOR,
                                 GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA,
                                 GU_DST_ALPHA, GU_ONE_MINUS_DST_ALPHA};

    name = NAMES[instruction & 7];
    fixed = instruction == SA_BLEND_ONE ? 0xffffffU : 0;
}

/* the engine drops a triangle whole where a corner falls outside the 4096
 * pixels its coordinates span, as a corner far to a side of the view does,
 * and clips at the near plane alone. so a triangle with a corner past REACH
 * times the view's half-width or half-height is cut here to within it:
 * well past the screen's edge, and well within those pixels. Cut holds a
 * view's terms for that */
constexpr float REACH = 8.0F;
constexpr int SIDES = 4;

struct Cut {
    const float *view;
    float ax, bx, ay, by; /* a corner's place over w: ax * x + bx * z, so */

    /* how far inside each side a world point lies, negative outside */
    void inside(const float *at, float out[SIDES]) const
    {
        const float *v = view;
        float x = v[0] * at[0] + v[1] * at[1] + v[2] * at[2] + v[3];
        float y = v[4] * at[0] + v[5] * at[1] + v[6] * at[2] + v[7];
        float z = v[8] * at[0] + v[9] * at[1] + v[10] * at[2] + v[11];
        float cx = ax * x + bx * z, cy = ay * y + by * z;

        out[0] = REACH * z - cx;
        out[1] = REACH * z + cx;
        out[2] = REACH * z - cy;
        out[3] = REACH * z + cy;
    }
};

/* the corner a part of the way from a to b: a batch's Corner, or a strip's
 * psp_corner */
template <typename C> C between(const C &a, const C &b, float part)
{
    C made;
    uint32_t color = 0;

    made.u = static_cast<decltype(made.u)>(a.u + (b.u - a.u) * part);
    made.v = static_cast<decltype(made.v)>(a.v + (b.v - a.v) * part);
    made.x = a.x + (b.x - a.x) * part;
    made.y = a.y + (b.y - a.y) * part;
    made.z = a.z + (b.z - a.z) * part;
    for (int shift = 0; shift < 32; shift += 8) {
        float from = static_cast<float>(a.color >> shift & 0xff);
        float to = static_cast<float>(b.color >> shift & 0xff);

        color |= static_cast<uint32_t>(from + (to - from) * part + 0.5F)
                 << shift;
    }
    made.color = color;
    return made;
}

/* add to cut the triangles of what of the triangle at corner lies inside
 * every side, as cut's terms place its corners */
template <typename C>
void cut_triangle(const Cut &cut, const C *corner, std::vector<C> &out)
{
    /* a triangle cut by four sides has seven corners at most */
    C now[8], next[8];
    int count = 3;

    memcpy(now, corner, 3 * sizeof *now);
    for (int side = 0; side < SIDES && count >= 3; side++) {
        int made = 0;

        for (int i = 0; i < count; i++) {
            const C &a = now[i], &b = now[(i + 1) % count];
            float in_a[SIDES], in_b[SIDES];

            cut.inside(&a.x, in_a);
            cut.inside(&b.x, in_b);
            if (in_a[side] >= 0) {
                next[made++] = a;
            }
            if ((in_a[side] >= 0) != (in_b[side] >= 0)) {
                next[made++] =
                    between(a, b, in_a[side] / (in_a[side] - in_b[side]));
            }
        }
        memcpy(now, next, static_cast<size_t>(made) * sizeof *now);
        count = made;
    }
    for (int i = 1; i + 1 < count; i++) {
        out.push_back(now[0]);
        out.push_back(now[i]);
        out.push_back(now[i + 1]);
    }
}

/* the PSP has one screen, and so one raster. what it holds lies here, where
 * draw.h's calls reach it as its own do */
struct Held {
    bool opened = false;
    bool wide = false;
    bool vsync = true;
    std::vector<Texture> textures; /* by index, less 1 */
    /* the color buffer the next picture draws to, and the pictures drawn */
    uintptr_t draws_at = DRAW_AT;
    uint32_t pictures = 0;
    bool keeps = true; /* until a picture finds no folder to keep it in */
    std::vector<Corner> cut; /* a batch's triangles that were cut */
    std::vector<psp_corner> strip_cut; /* a strip's, whole or cut */
    void *list = nullptr;
    uint8_t *kept = nullptr;
    uint32_t kept_used = 0;
    /* the most bytes a list took, how many lists the pictures took past
     * one each, and the textures' bytes */
    uint32_t most_list = 0, more_lists = 0, texel_bytes = 0, most_texels = 0;
    std::string why;
    /* a picture's commands lie open, from psp_picture_open or submit's start
     * to submit's end. started says the engine holds a list of them now:
     * a picture takes another once one is full, or once a load took the
     * list's memory */
    bool list_open = false;
    bool started = false;
    /* the game wrote the work area, or may have, since the raster last
     * settled it (settle_work) */
    bool written = true;
    /* the walk's last state and screen, where it holds one */
    psp_state walked{};
    bool walking = false;
    float screen[4] = {0.0F, 0.0F, 0.0F, 0.0F};
    /* the walk's place and screen of the moment, and the strips it keeps
     * for the picture's end, each under the kept place its number names */
    struct Placed {
        float matrix[16];
        float screen[4];
    };
    struct Later {
        float depth;
        uint32_t place;
        psp_state state;
        const psp_corner *corners;
        uint32_t count;
        bool triangles; /* else a strip */
    };
    struct Span {
        uint32_t at, bytes;
    };
    std::vector<Span> video_free;
    uint32_t video_bytes = 0, most_video = 0;
    Placed place{};
    bool place_kept = false;
    std::vector<Placed> places;
    std::vector<Later> later;
};
Held held;

/* the bytes the engine's list has taken */
uint32_t list_used()
{
    return static_cast<uint32_t>(sceGuCheckList());
}

/* a list for the engine where it holds none. the engine keeps its state
 * from one list to the next */
/* the game writes the work area through the processor's data cache. the
 * raster writes the list and the kept strips there past the cache, where the
 * engine reads them. what the cache still holds of the game's bytes would
 * go to memory later, over the raster's: it goes now, and leaves the cache */
void settle_work(Held &r)
{
    if (r.written) {
        sceKernelDcacheWritebackInvalidateRange(r.list, LIST_BYTES);
        sceKernelDcacheWritebackInvalidateRange(r.kept, KEPT_BYTES);
        r.written = false;
    }
}

void start_list(Held &r)
{
    if (!r.started) {
        settle_work(r);
        sceGuStart(GU_DIRECT, r.list);
        r.started = true;
    }
}

/* the engine draws what its list holds, and the list's memory is free */
void end_list(Held &r)
{
    if (r.started) {
        r.most_list = list_used() > r.most_list ? list_used() : r.most_list;
        sceGuFinish();
        sceGuSync(0, 0);
        r.started = false;
    }
}

/* a list with room for bytes of corners and a batch's commands: where the
 * list is full, the engine draws it and takes another. false for more than
 * a list holds */
bool list_room(Held &r, uint32_t bytes)
{
    start_list(r);
    if (list_used() + bytes + BATCH_BYTES > LIST_BYTES) {
        end_list(r);
        start_list(r);
        r.more_lists++;
    }
    return bytes + 2 * BATCH_BYTES <= LIST_BYTES;
}

/* video memory past the buffers, which holds textures while it has room:
 * the engine reads them fastest there, and they leave the program's memory
 * to the game. free holds what is free, by address, no two spans touching */
constexpr uint32_t VIDEO_FIRST = DEPTH_AT + BUFFER_WIDTH * SCREEN_HEIGHT * 2;

void *video_take(Held &r, uint32_t bytes)
{
    bytes = (bytes + 63) & ~63U;
    for (size_t i = 0; i < r.video_free.size(); i++) {
        Held::Span &span = r.video_free[i];

        if (span.bytes >= bytes) {
            void *at = static_cast<uint8_t *>(sceGeEdramGetAddr()) + span.at;

            span.at += bytes;
            span.bytes -= bytes;
            if (span.bytes == 0) {
                r.video_free.erase(r.video_free.begin() +
                                   static_cast<ptrdiff_t>(i));
            }
            r.video_bytes += bytes;
            r.most_video =
                r.video_bytes > r.most_video ? r.video_bytes : r.most_video;
            return at;
        }
    }
    return nullptr;
}

void video_give(Held &r, void *texels, uint32_t bytes)
{
    Held::Span span = {
        static_cast<uint32_t>(static_cast<uint8_t *>(texels) -
                              static_cast<uint8_t *>(sceGeEdramGetAddr())),
        (bytes + 63) & ~63U};
    size_t i = 0;

    while (i < r.video_free.size() && r.video_free[i].at < span.at) {
        i++;
    }
    r.video_bytes -= span.bytes;
    if (i < r.video_free.size() && span.at + span.bytes == r.video_free[i].at) {
        r.video_free[i].at = span.at;
        r.video_free[i].bytes += span.bytes;
    } else {
        r.video_free.insert(r.video_free.begin() + static_cast<ptrdiff_t>(i),
                            span);
    }
    if (i > 0 && r.video_free[i - 1].at + r.video_free[i - 1].bytes ==
                     r.video_free[i].at) {
        r.video_free[i - 1].bytes += r.video_free[i].bytes;
        r.video_free.erase(r.video_free.begin() + static_cast<ptrdiff_t>(i));
    }
}

/* a texture's texels: video memory's where it has room, else the heap's */
void *texels_take(Held &r, Texture &made)
{
    made.texels = video_take(r, made.bytes);
    made.video = made.texels != nullptr;
    if (!made.video) {
        made.texels = memalign(16, made.bytes);
    }
    return made.texels;
}

/* a view's point shows at center + focal * (x, y) / z, p's four, and the
 * engine's y runs up. z less the nearest's twice over w runs from -1 at the
 * nearest, z = 1, to 1 at the farthest, which the depth range turns into
 * 1/z */
ScePspFMatrix4 projection(const float *p)
{
    return {{p[0] / (WIDTH / 2), 0, 0, 0},
            {0, -p[1] / (HEIGHT / 2), 0, 0},
            {(p[2] - WIDTH / 2) / (WIDTH / 2),
             -(p[3] - HEIGHT / 2) / (HEIGHT / 2), 1, 1},
            {0, 0, -2, 0}};
}

const ScePspFMatrix4 ONE = {
    {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};

/* open a picture's commands: the screen whole to black and its depth to the
 * farthest, with no scissor, then the picture's place on it. the walk's
 * strips come in view space, so the view starts as no turn at all */
void open_list(Held &r)
{
    int width = r.wide ? SCREEN_WIDTH : NARROW_WIDTH;

    end_list(r);
    start_list(r);
    sceGuDisable(GU_SCISSOR_TEST);
    sceGuClearColor(0xff000000U);
    sceGuClearDepth(0);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuScissor((SCREEN_WIDTH - width) / 2, 0, width, SCREEN_HEIGHT);
    sceGuViewport(2048, 2048, width, SCREEN_HEIGHT);
    sceGuSetMatrix(GU_VIEW, &ONE);
    sceGuSetMatrix(GU_MODEL, &ONE);
    r.list_open = true;
    r.walking = false;
    r.screen[0] = 0.0F;
    r.place_kept = false;
    r.places.clear();
    r.later.clear();
    r.kept_used = 0;
}

/* draw the strips the walk kept, the farthest first, each under its own
 * place and screen in view space */
void draw_later(Held &r)
{
    uint32_t placed = UINT32_MAX;

    if (r.later.empty()) {
        return;
    }
    std::stable_sort(r.later.begin(), r.later.end(),
                     [](const Held::Later &a, const Held::Later &b) {
                         return a.depth > b.depth;
                     });
    sceGuSetMatrix(GU_VIEW, &ONE);
    r.walking = false;
    for (const Held::Later &l : r.later) {
        list_room(r, 2 * BATCH_BYTES);
        if (l.place != placed) {
            const Held::Placed &p = r.places[l.place];
            ScePspFMatrix4 project = projection(p.screen);

            sceGuSetMatrix(GU_PROJECTION, &project);
            sceGuSetMatrix(GU_MODEL,
                           reinterpret_cast<const ScePspFMatrix4 *>(p.matrix));
            placed = l.place;
        }
        psp_draw_state(&l.state);
        if (l.triangles) {
            sceGuDrawArray(GU_TRIANGLES, STRIP_FORMAT,
                           static_cast<int>(l.count), nullptr, l.corners);
        } else {
            psp_draw_strip(l.corners, l.count);
        }
    }
    r.later.clear();
    r.kept_used = 0;
}

void free_texture(Held &r, uint32_t texture)
{
    if (texture != 0 && texture <= r.textures.size()) {
        Texture &t = r.textures[texture - 1];

        if (t.texels != nullptr) {
            r.texel_bytes -= t.bytes;
        }
        if (t.video) {
            video_give(r, t.texels, t.bytes);
        } else {
            free(t.texels);
        }
        t = Texture{};
    }
}

} // namespace

struct Raster::Impl {};

Raster::~Raster()
{
    if (impl_ == nullptr) {
        return;
    }
    if (held.opened) {
        sceGuTerm();
    }
    for (Texture &t : held.textures) {
        if (!t.video) {
            free(t.texels);
        }
    }
    delete impl_;
}

bool Raster::open(const RasterOptions &options, std::string &why)
{
    if (impl_ != nullptr) {
        why = "the raster is open already";
        return false;
    }
    impl_ = new Impl;
    held.vsync = options.vsync;
    held.list = NATIVE_RAM(LIST_AT);
    held.kept = reinterpret_cast<uint8_t *>(NATIVE_RAM(KEPT_AT));
    sceGuInit();
    sceGuStart(GU_DIRECT, held.list);
    sceGuDrawBuffer(GU_PSM_8888, reinterpret_cast<void *>(DRAW_AT),
                    BUFFER_WIDTH);
    sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT,
                    reinterpret_cast<void *>(SHOW_AT), BUFFER_WIDTH);
    sceGuDepthBuffer(reinterpret_cast<void *>(DEPTH_AT), BUFFER_WIDTH);
    sceGuOffset(2048 - SCREEN_WIDTH / 2, 2048 - SCREEN_HEIGHT / 2);
    sceGuDepthRange(65535, 0);
    sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuEnable(GU_CLIP_PLANES);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuShadeModel(GU_SMOOTH);
    /* a batch that culls drops what turns clockwise on the screen */
    sceGuFrontFace(GU_CCW);
    sceGuTexScale(1.0F, 1.0F);
    sceGuTexOffset(0.0F, 0.0F);
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
    held.video_free = {{VIDEO_FIRST, sceGeEdramGetSize() - VIDEO_FIRST}};
    held.opened = true;
    return true;
}

std::string Raster::device_name() const
{
    return "PSP";
}

namespace {

/* a block of DXT1 as the engine reads one: four lines of four texels, two
 * bits each, then two colors of 16 bits, red highest. where the first color
 * is the greater, a texel's bits pick the first, the second, or a third or
 * two thirds of the way between. else they pick the first, the second, the
 * mean, or no color at all, a clear texel. from holds the block's 16 texels,
 * RGBA, and clear says whether any of them may be clear, as the half of
 * alpha decides. the two colors are the ends of the box the solid texels
 * fill, drawn in a sixteenth */
void code_block(const uint8_t from[16][4], bool clear, uint8_t out[8])
{
    int low[3] = {255, 255, 255}, high[3] = {0, 0, 0};
    bool any_clear = false, any_solid = false;
    uint8_t colors[4][3];

    for (int i = 0; i < 16; i++) {
        if (clear && from[i][3] < 128) {
            any_clear = true;
            continue;
        }
        any_solid = true;
        for (int c = 0; c < 3; c++) {
            low[c] = from[i][c] < low[c] ? from[i][c] : low[c];
            high[c] = from[i][c] > high[c] ? from[i][c] : high[c];
        }
    }
    if (!any_solid) {
        memset(out, 0xff, 4);
        memset(out + 4, 0, 4);
        return;
    }
    for (int c = 0; c < 3; c++) {
        int inset = (high[c] - low[c]) >> 4;

        low[c] += inset;
        high[c] -= inset;
    }
    uint16_t most = static_cast<uint16_t>((high[0] >> 3) << 11 |
                                          (high[1] >> 2) << 5 | high[2] >> 3);
    uint16_t least = static_cast<uint16_t>((low[0] >> 3) << 11 |
                                           (low[1] >> 2) << 5 | low[2] >> 3);
    /* four colors where the first is the greater, and three with a clear
     * one else: a block of one color takes the three, and its texels the
     * first */
    bool four = !any_clear && most > least;
    uint16_t first = four ? most : least, second = four ? least : most;

    if (!four && first > second) {
        uint16_t swap = first;

        first = second;
        second = swap;
    }
    for (int c = 0; c < 3; c++) {
        static const int SHIFT[3] = {11, 5, 0}, BITS[3] = {5, 6, 5};
        int a = (first >> SHIFT[c]) & ((1 << BITS[c]) - 1);
        int b = (second >> SHIFT[c]) & ((1 << BITS[c]) - 1);

        a = a << (8 - BITS[c]) | a >> (2 * BITS[c] - 8);
        b = b << (8 - BITS[c]) | b >> (2 * BITS[c] - 8);
        colors[0][c] = static_cast<uint8_t>(a);
        colors[1][c] = static_cast<uint8_t>(b);
        colors[2][c] =
            static_cast<uint8_t>(four ? (2 * a + b) / 3 : (a + b) / 2);
        colors[3][c] = static_cast<uint8_t>(four ? (a + 2 * b) / 3 : 0);
    }
    for (int line = 0; line < 4; line++) {
        uint8_t bits = 0;

        for (int x = 0; x < 4; x++) {
            const uint8_t *texel = from[line * 4 + x];
            int best = 3, nearest = 1 << 30;

            if (!(clear && texel[3] < 128)) {
                for (int k = 0; k < (four ? 4 : 3); k++) {
                    int far = 0;

                    for (int c = 0; c < 3; c++) {
                        int d = texel[c] - colors[k][c];

                        far += d * d;
                    }
                    if (far < nearest) {
                        nearest = far;
                        best = k;
                    }
                }
            }
            bits = static_cast<uint8_t>(bits | best << (x * 2));
        }
        out[line] = bits;
    }
    memcpy(out + 4, &first, 2);
    memcpy(out + 6, &second, 2);
}

/* make a texture of a source of texels: its sides, how its alphas lie, and
 * texel(x, y, rgba), a texel's RGBA. clear says a texel may be clear, partly
 * that one lies between clear and solid, and cuts that few do. the texture
 * is made from the source a block at a time, so it takes no memory but its
 * own.
 *
 * a texel is the mean of a square of the source's, step a side. the PSP
 * shows the picture's 480 lines in 272, and has memory for a quarter of the
 * game's texels, so a texture of HALVED a side or more takes half its
 * sides. a side past the engine's longest halves until it fits */
template <typename Texel>
uint32_t make_texture(Held &r, uint32_t width, uint32_t height, bool clear,
                      bool partly, bool cuts, Texel texel, std::string &why)
{
    uint32_t step = 1;
    Texture made;

    if (width == 0 || height == 0 || (width & (width - 1)) != 0 ||
        (height & (height - 1)) != 0) {
        why = "a texture whose sides are no powers of two";
        return 0;
    }
    if (width >= HALVED && height >= HALVED) {
        step = 2;
    }
    while (width / step > SIDE_MOST || height / step > SIDE_MOST) {
        step *= 2;
    }
    made.width = static_cast<uint16_t>(width / step);
    made.height = static_cast<uint16_t>(height / step);
    made.cuts = cuts;
    made.format = static_cast<uint8_t>(partly  ? GU_PSM_4444
                                       : clear ? GU_PSM_5551
                                               : GU_PSM_5650);
    auto mean = [&](uint32_t x, uint32_t y, uint8_t out[4]) {
        if (step == 1) {
            texel(x, y, out);
            return;
        }
        uint32_t sum[4] = {0, 0, 0, 0};

        for (uint32_t j = 0; j < step; j++) {
            for (uint32_t i = 0; i < step; i++) {
                uint8_t p[4];

                texel(x * step + i, y * step + j, p);
                sum[0] += p[0];
                sum[1] += p[1];
                sum[2] += p[2];
                sum[3] += p[3];
            }
        }
        for (int i = 0; i < 4; i++) {
            out[i] = static_cast<uint8_t>(sum[i] / (step * step));
        }
    };

    /* a texture with no texel between clear and solid, or an edge's few,
     * takes DXT1's 4 bits a texel, which the PSP's memory has room for where
     * it has none for 16: the console holds most of the game's at 2. a
     * texture that blends by its texels keeps 16 bits, 4 of them alpha */
    if ((!partly || cuts) && made.width >= 8 && made.height >= 8) {
        made.format = GU_PSM_DXT1;
        made.bytes = made.width / 4 * (made.height / 4) * 8;
        if (texels_take(r, made) == nullptr) {
            why = "no memory for a texture";
            return 0;
        }
        uint8_t *out = static_cast<uint8_t *>(made.texels);

        for (uint32_t y = 0; y < made.height; y += 4) {
            for (uint32_t x = 0; x < made.width; x += 4) {
                uint8_t block[16][4];

                for (uint32_t i = 0; i < 16; i++) {
                    mean(x + i % 4, y + i / 4, block[i]);
                }
                code_block(block, clear, out);
                out += 8;
            }
        }
    } else {
        /* the engine reads a row of at least 8 texels, and a swizzled
         * texture in blocks of 8 texels by 8 rows, which the texels go
         * straight into */
        uint32_t row = made.width < 8 ? 8 : made.width;
        uint32_t rows = made.height < 8 ? 8 : made.height;

        made.bytes = row * rows * 2;
        if (texels_take(r, made) == nullptr) {
            why = "no memory for a texture";
            return 0;
        }
        uint16_t *out = static_cast<uint16_t *>(made.texels);

        memset(out, 0, made.bytes);
        made.swizzled = true;
        for (uint32_t y = 0; y < made.height; y++) {
            for (uint32_t x = 0; x < made.width; x++) {
                uint8_t p[4];
                uint16_t word;

                mean(x, y, p);
                if (made.format == GU_PSM_4444) {
                    word = static_cast<uint16_t>(p[0] >> 4 | (p[1] >> 4) << 4 |
                                                 (p[2] >> 4) << 8 |
                                                 (p[3] >> 4) << 12);
                } else if (made.format == GU_PSM_5551) {
                    word = static_cast<uint16_t>(p[0] >> 3 | (p[1] >> 3) << 5 |
                                                 (p[2] >> 3) << 10 |
                                                 (p[3] >> 7) << 15);
                } else {
                    word = static_cast<uint16_t>(p[0] >> 3 | (p[1] >> 2) << 5 |
                                                 (p[2] >> 3) << 11);
                }
                out[((y / 8) * (row / 8) + x / 8) * 64 + (y % 8) * 8 + x % 8] =
                    word;
            }
        }
    }
    sceKernelDcacheWritebackRange(made.texels, made.bytes);
    r.texel_bytes += made.bytes;
    r.most_texels =
        r.texel_bytes > r.most_texels ? r.texel_bytes : r.most_texels;
    for (size_t i = 0; i < r.textures.size(); i++) {
        if (r.textures[i].texels == nullptr) {
            r.textures[i] = made;
            return static_cast<uint32_t>(i + 1);
        }
    }
    r.textures.push_back(made);
    return static_cast<uint32_t>(r.textures.size());
}

uint32_t add_texture(Held &r, const Image &image, std::string &why)
{
    size_t between = 0;
    bool clear = false;

    if (image.rgba.size() <
        static_cast<size_t>(image.width) * image.height * 4) {
        why = "a texture that lacks texels";
        return 0;
    }
    for (size_t i = 3; i < image.rgba.size(); i += 4) {
        clear |= image.rgba[i] < 255;
        between += image.rgba[i] != 0 && image.rgba[i] != 255;
    }
    /* an eighth of the texels at most lie between, as a cut shape's soft
     * edge does and a pane of glass or a puff of smoke doesn't */
    return make_texture(
        r, image.width, image.height, clear, between != 0,
        between * 32 <= image.rgba.size(),
        [&](uint32_t x, uint32_t y, uint8_t out[4]) {
            memcpy(out,
                   &image.rgba[(static_cast<size_t>(y) * image.width + x) * 4],
                   4);
        },
        why);
}

/* a texture straight from a PVR's chunk, a GBIX chunk before its PVRT or
 * not: the PSP has no memory for the chunk's texels as an image between.
 * the layouts are engine/pc/pvr.cpp's, which says what each is. tried is
 * false for a format this reads none of, YUV422's, which a caller decodes
 * as an image */
uint32_t add_pvr(Held &r, const uint8_t *chunk, size_t size, bool &tried,
                 std::string &why)
{
    enum { ARGB1555 = 0, RGB565 = 1, ARGB4444 = 2 };
    enum {
        TWIDDLED = 0x01,
        TWIDDLED_MIPS = 0x02,
        VQ = 0x03,
        VQ_MIPS = 0x04,
        RECTANGLE = 0x09,
        STRIDE = 0x0b,
        RECTANGLE_TWIDDLED = 0x0d,
        SMALL_VQ = 0x10,
        SMALL_VQ_MIPS = 0x11,
        TWIDDLED_MIPS_ALT = 0x12
    };
    /* a number's bits apart, each at twice its place: a twiddled place is
     * y's so spread, and x's a bit higher */
    static uint32_t spread[1024];

    if (spread[1] == 0) {
        for (uint32_t n = 0; n < 1024; n++) {
            for (uint32_t bit = 0; bit < 10; bit++) {
                spread[n] |= (n >> bit & 1) << (2 * bit);
            }
        }
    }
    tried = false;
    if (size >= 8 && memcmp(chunk, "GBIX", 4) == 0) {
        uint32_t length;

        memcpy(&length, chunk + 4, sizeof length);
        if (length > size - 8) {
            return 0;
        }
        chunk += 8 + length;
        size -= 8 + length;
    }
    if (size < 16 || memcmp(chunk, "PVRT", 4) != 0) {
        return 0;
    }
    uint32_t length;

    memcpy(&length, chunk + 4, sizeof length);
    uint32_t pixel = chunk[8], layout = chunk[9];
    uint32_t w = chunk[12] | chunk[13] << 8, h = chunk[14] | chunk[15] << 8;
    bool small_vq = layout == SMALL_VQ || layout == SMALL_VQ_MIPS;
    bool vq = small_vq || layout == VQ || layout == VQ_MIPS;
    bool mips = layout == TWIDDLED_MIPS || layout == TWIDDLED_MIPS_ALT ||
                layout == VQ_MIPS || layout == SMALL_VQ_MIPS;
    bool rows = layout == RECTANGLE || layout == STRIDE;
    bool twiddled =
        layout == TWIDDLED || layout == RECTANGLE_TWIDDLED || (mips && !vq);

    if (length < 8 || length > size - 8 || w == 0 || h == 0 || w > 1024 ||
        h > 1024 || pixel > ARGB4444 || (!vq && !rows && !twiddled) ||
        (w & (w - 1)) != 0 || (h & (h - 1)) != 0 || (mips && w != h) ||
        (vq && (w < 2 || h < 2))) {
        return 0;
    }
    uint32_t codes = 256, largest = 0, taken = w * h * 2;

    if (small_vq) {
        codes = w <= 16            ? 16
                : w == 32          ? (mips ? 64 : 32)
                : w == 64 && !mips ? 128
                                   : 256;
    }
    if (vq) {
        largest = codes * 8;
        taken = (w / 2) * (h / 2);
        if (mips) {
            largest += 1;
            for (uint32_t s = 2; s < w; s <<= 1) {
                largest += (s / 2) * (s / 2);
            }
        }
    } else if (mips) {
        largest = 2;
        for (uint32_t s = 1; s < w; s <<= 1) {
            largest += s * s * 2;
        }
    }
    if (largest > length - 8 || taken > length - 8 - largest) {
        return 0;
    }
    tried = true;
    const uint8_t *data = chunk + 16;
    const uint8_t *level = data + largest;
    /* a twiddled rectangle is squares of its shorter side, in a row along
     * its longer one. a VQ's places are its blocks', 2 texels a side */
    uint32_t across = vq ? w / 2 : w, down = vq ? h / 2 : h;
    uint32_t side = across < down ? across : down;
    auto place = [&](uint32_t x, uint32_t y) {
        uint32_t square = across < down ? y / side : x / side;

        return square * side * side +
               (spread[y % side] | spread[x % side] << 1);
    };
    auto word_at = [&](uint32_t x, uint32_t y) -> uint32_t {
        const uint8_t *at;

        if (vq) {
            at = data + (level[place(x / 2, y / 2)] % codes) * 8 + (x & 1) * 4 +
                 (y & 1) * 2;
        } else {
            at = level + (twiddled ? place(x, y) : y * w + x) * 2;
        }
        return static_cast<uint32_t>(at[0] | at[1] << 8);
    };
    /* how the alphas lie. ARGB1555's are clear or solid. ARGB4444's are
     * counted: each texel's, or each code's by the blocks that take it */
    size_t between = 0;
    bool clear = pixel == ARGB1555;

    if (pixel == ARGB4444 && !vq) {
        for (uint32_t i = 0; i < w * h; i++) {
            uint32_t alpha = level[i * 2 + 1] >> 4;

            clear |= alpha < 15;
            between += alpha != 0 && alpha != 15;
        }
    } else if (pixel == ARGB4444) {
        uint8_t partly[256] = {};
        bool any[256] = {};

        for (uint32_t code = 0; code < codes; code++) {
            for (uint32_t i = 0; i < 4; i++) {
                uint32_t alpha = data[code * 8 + i * 2 + 1] >> 4;

                any[code] |= alpha < 15;
                partly[code] += alpha != 0 && alpha != 15;
            }
        }
        for (uint32_t i = 0; i < taken; i++) {
            clear |= any[level[i] % codes];
            between += partly[level[i] % codes];
        }
    }
    return make_texture(
        r, w, h, clear, between != 0, between * 8 <= static_cast<size_t>(w) * h,
        [&](uint32_t x, uint32_t y, uint8_t out[4]) {
            uint32_t word = word_at(x, y);
            uint32_t red, green, blue, alpha = 255;

            if (pixel == ARGB1555) {
                alpha = word & 0x8000 ? 255 : 0;
                red = word >> 10 & 31;
                green = word >> 5 & 31;
                blue = word & 31;
                red = red << 3 | red >> 2;
                green = green << 3 | green >> 2;
                blue = blue << 3 | blue >> 2;
            } else if (pixel == RGB565) {
                red = word >> 11 & 31;
                green = word >> 5 & 63;
                blue = word & 31;
                red = red << 3 | red >> 2;
                green = green << 2 | green >> 4;
                blue = blue << 3 | blue >> 2;
            } else {
                alpha = (word >> 12 & 15) * 17;
                red = (word >> 8 & 15) * 17;
                green = (word >> 4 & 15) * 17;
                blue = (word & 15) * 17;
            }
            out[0] = static_cast<uint8_t>(red);
            out[1] = static_cast<uint8_t>(green);
            out[2] = static_cast<uint8_t>(blue);
            out[3] = static_cast<uint8_t>(alpha);
        },
        why);
}

} // namespace

uint32_t Raster::add_texture(const Image &image, std::string &why)
{
    return pc::add_texture(held, image, why);
}

uint32_t Raster::add_pvr(const void *data, uint32_t bytes, bool &tried,
                         std::string &why)
{
    return pc::add_pvr(held, static_cast<const uint8_t *>(data), bytes, tried,
                       why);
}

/* a picture is drawn by the time submit returns, so none reads a texture
 * past its free */
void Raster::free_texture(uint32_t texture)
{
    if (impl_ != nullptr) {
        pc::free_texture(held, texture);
    }
}

/* the engine carries a batch's corners to the world itself, so a batch
 * that lies whole within the cut's reach needs no look at its corners */
float Raster::reach() const
{
    return REACH;
}

uint32_t psp_wait_us;

/* Raster::submit, for the raster and for the PSP's own scene alike
 * (direct.cpp): the batches go after what the walk drew into the picture's
 * commands, and the picture shows */
bool psp_submit(const PictureUniform &picture, uint32_t clear,
                const std::vector<Vertex> &vertices,
                const std::vector<Batch> &batches, bool sorted,
                const std::vector<float> *places)
{

    Held &r = held;
    int width = r.wide ? SCREEN_WIDTH : NARROW_WIDTH;
    float across = static_cast<float>(width) / WIDTH;
    float down = static_cast<float>(SCREEN_HEIGHT) / HEIGHT;
    const float *v = picture.view;
    ScePspFMatrix4 project = projection(picture.project);
    ScePspFMatrix4 view = {{v[0], v[4], v[8], 0},
                           {v[1], v[5], v[9], 0},
                           {v[2], v[6], v[10], 0},
                           {v[3], v[7], v[11], 1}};
    const ScePspFMatrix4 &one = ONE;
    uint32_t placed = 0;
    Cut cut = {v, project.x.x, project.z.x, project.y.y, project.z.y};

    if (!r.list_open) {
        open_list(r);
    }
    list_room(r, 4 * BATCH_BYTES);
    r.walking = false;
    r.screen[0] = 0.0F;
    sceGuTexScale(1.0F, 1.0F);
    sceGuFrontFace(GU_CCW);
    sceGuDisable(GU_ALPHA_TEST);
    /* the picture's own color shows where nothing drew: a plane at the
     * farthest depth, which anything drawn lies nearer than */
    {
        Corner *plane =
            static_cast<Corner *>(sceGuGetMemory(2 * sizeof(Corner)));
        uint32_t color =
            (clear & 0xff00ff00U) | (clear >> 16 & 0xff) | (clear & 0xff) << 16;

        plane[0] = {0, 0, color, static_cast<float>((SCREEN_WIDTH - width) / 2),
                    0, 0};
        plane[1] = {0,
                    0,
                    color,
                    static_cast<float>((SCREEN_WIDTH + width) / 2),
                    SCREEN_HEIGHT,
                    0};
        sceGuDisable(GU_TEXTURE_2D);
        sceGuDisable(GU_BLEND);
        sceGuDisable(GU_CULL_FACE);
        sceGuDepthFunc(GU_GEQUAL);
        sceGuDepthMask(GU_TRUE);
        sceGuDrawArray(GU_SPRITES, CORNER_FORMAT | GU_TRANSFORM_2D, 2, nullptr,
                       plane);
    }
    sceGuSetMatrix(GU_PROJECTION, &project);
    sceGuSetMatrix(GU_VIEW, &view);
    sceGuSetMatrix(GU_MODEL, &one);
    /* the walk's kept strips blend, so they draw ahead of the records'
     * first batch that does, or after the last where none does. the
     * records' batches then find their own terms again */
    bool kept_drawn = false;
    auto draw_kept = [&]() {
        if (!r.later.empty()) {
            draw_later(r);
            sceGuTexScale(1.0F, 1.0F);
            sceGuFrontFace(GU_CCW);
            sceGuDisable(GU_ALPHA_TEST);
            sceGuSetMatrix(GU_PROJECTION, &project);
            sceGuSetMatrix(GU_VIEW, &view);
            sceGuSetMatrix(GU_MODEL, &one);
            placed = 0;
            r.walking = false;
            r.screen[0] = 0.0F;
        }
        kept_drawn = true;
    };

    for (const Batch &batch : batches) {
        const State &s = batch.state;
        const Texture *texture = nullptr;
        bool screen = (s.flags & BATCH_SCREEN) != 0;

        if (s.blend != 0 && !kept_drawn) {
            draw_kept();
        }

        if (s.texture != 0 && s.texture <= r.textures.size() &&
            r.textures[s.texture - 1].texels != nullptr &&
            (s.flags & BATCH_TEXTURE) != 0) {
            texture = &r.textures[s.texture - 1];
        }
        if (texture != nullptr) {
            bool opaque = (s.flags & BATCH_OPAQUE) != 0;
            int filter = (s.sample & SAMPLE_FILTER) == SA_FILTER_POINT
                             ? GU_NEAREST
                             : GU_LINEAR;

            sceGuEnable(GU_TEXTURE_2D);
            sceGuTexMode(texture->format, 0, 0, texture->swizzled);
            sceGuTexImage(0, texture->width, texture->height,
                          texture->width < 8 ? 8 : texture->width,
                          texture->texels);
            sceGuTexFunc((s.flags & BATCH_DECAL) != 0 ? GU_TFX_REPLACE
                                                      : GU_TFX_MODULATE,
                         opaque ? GU_TCC_RGB : GU_TCC_RGBA);
            sceGuTexFilter(filter, filter);
            sceGuTexWrap(
                (s.sample & SAMPLE_CLAMP_U) != 0 ? GU_CLAMP : GU_REPEAT,
                (s.sample & SAMPLE_CLAMP_V) != 0 ? GU_CLAMP : GU_REPEAT);
        } else {
            sceGuDisable(GU_TEXTURE_2D);
        }
        if (s.blend != 0) {
            int src, dst;
            uint32_t src_fixed, dst_fixed;

            factor(s.src, src, src_fixed);
            factor(s.dst, dst, dst_fixed);
            sceGuEnable(GU_BLEND);
            sceGuBlendFunc(GU_ADD, src, dst, src_fixed, dst_fixed);
        } else {
            sceGuDisable(GU_BLEND);
        }
        /* the scene's own plane behind the picture is its first batch, at
         * the farthest depth: it shows where nothing drew */
        sceGuDepthFunc(&batch == &batches.front() && screen &&
                               s.depth == SA_DEPTH_ALWAYS &&
                               s.depth_write == 0 && s.blend == 0
                           ? GU_GEQUAL
                           : DEPTH[s.depth & 7]);
        /* a blend in the game's order writes no depth, so one it draws
         * before another behind it doesn't hide that one */
        sceGuDepthMask(s.depth_write != 0 && !(sorted && s.blend != 0)
                           ? GU_FALSE
                           : GU_TRUE);
        if (s.cull != 0 && !screen) {
            sceGuEnable(GU_CULL_FACE);
        } else {
            sceGuDisable(GU_CULL_FACE);
        }
        /* the batch's place, where it names one the picture has: the
         * scene found its corners within reach, so they go as they are */
        uint32_t place =
            !screen && places != nullptr && s.place * 12U <= places->size()
                ? s.place
                : 0;

        /* a batch's commands, its state's and its place's */
        list_room(r, 2 * BATCH_BYTES);
        if (place != placed) {
            if (place != 0) {
                const float *m = &(*places)[(place - 1) * 12];
                ScePspFMatrix4 model = {{m[0], m[4], m[8], 0},
                                        {m[1], m[5], m[9], 0},
                                        {m[2], m[6], m[10], 0},
                                        {m[3], m[7], m[11], 1}};

                sceGuSetMatrix(GU_MODEL, &model);
            } else {
                sceGuSetMatrix(GU_MODEL, &one);
            }
            placed = place;
        }
        for (uint32_t first = batch.first, left = batch.count; left != 0;) {
            uint32_t count = left < CALL_MOST ? left : CALL_MOST;
            uint32_t bytes = count * sizeof(Corner);

            if (first + count > vertices.size() || !list_room(r, bytes)) {
                break;
            }
            Corner *corners =
                static_cast<Corner *>(sceGuGetMemory(static_cast<int>(bytes)));

            uint32_t kept = 0;

            r.cut.clear();
            for (uint32_t i = 0; i < count; i++) {
                const Vertex &from = vertices[first + i];
                Corner &to = corners[kept++];

                to.color = from.base;
                if (screen) {
                    float depth = from.at[2] > 1.0F ? DEPTH_MOST / from.at[2]
                                                    : DEPTH_MOST;

                    to.u = texture != nullptr ? from.uv[0] * texture->width : 0;
                    to.v =
                        texture != nullptr ? from.uv[1] * texture->height : 0;
                    to.x = SCREEN_WIDTH / 2 + (from.at[0] - WIDTH / 2) * across;
                    to.y = from.at[1] * down;
                    to.z = depth;
                    continue;
                }
                to.u = from.uv[0];
                to.v = from.uv[1];
                to.x = from.at[0];
                to.y = from.at[1];
                to.z = from.at[2];
                if (i % 3 != 2 || place != 0) {
                    continue;
                }
                /* the triangle's third corner: it stays where every corner
                 * lies inside every side, goes where all three lie outside
                 * one, and else is cut */
                float in[3][SIDES];
                bool whole = true, gone = false;

                for (int c = 0; c < 3; c++) {
                    cut.inside(&corners[kept - 3 + c].x, in[c]);
                }
                for (int side = 0; side < SIDES; side++) {
                    bool a = in[0][side] >= 0, b = in[1][side] >= 0,
                         c = in[2][side] >= 0;

                    whole &= a && b && c;
                    gone |= !a && !b && !c;
                }
                if (!whole) {
                    kept -= 3;
                    if (!gone) {
                        cut_triangle(cut, &corners[kept], r.cut);
                    }
                }
            }
            if (kept != 0) {
                sceGuDrawArray(GU_TRIANGLES,
                               CORNER_FORMAT |
                                   (screen ? GU_TRANSFORM_2D : GU_TRANSFORM_3D),
                               static_cast<int>(kept), nullptr, corners);
            }
            /* the cut triangles follow the batch's whole ones */
            uint32_t more =
                static_cast<uint32_t>(r.cut.size() * sizeof(Corner));

            if (more != 0 && r.cut.size() <= CALL_MOST && list_room(r, more)) {
                void *room = sceGuGetMemory(static_cast<int>(more));

                memcpy(room, r.cut.data(), more);
                sceGuDrawArray(GU_TRIANGLES, CORNER_FORMAT | GU_TRANSFORM_3D,
                               static_cast<int>(r.cut.size()), nullptr, room);
            }
            first += count;
            left -= count;
        }
    }
    if (!kept_drawn) {
        draw_kept();
    }
    end_list(r);
    /* every PICTURE_EVERY-th picture stays in the folder pictures, beside
     * the program, where a run has made that folder: a BMP of 24 bits, a
     * row at a write, as the picture's rows need no memory then */
    if (r.keeps && r.pictures % PICTURE_EVERY == 0) {
        char path[64];
        const uint8_t *drawn =
            static_cast<const uint8_t *>(sceGeEdramGetAddr()) + r.draws_at;
        uint32_t bytes = SCREEN_WIDTH * SCREEN_HEIGHT * 3;
        uint8_t head[54] = {'B', 'M'};
        uint8_t row[SCREEN_WIDTH * 3];
        FILE *file;

        snprintf(path, sizeof path, "pictures/%05u.bmp",
                 static_cast<unsigned>(r.pictures));
        file = fopen(path, "wb");
        r.keeps = file != nullptr;
        if (file != nullptr) {
            const uint32_t fields[][2] = {{2, 54 + bytes},
                                          {10, 54},
                                          {14, 40},
                                          {18, SCREEN_WIDTH},
                                          {22, SCREEN_HEIGHT},
                                          {26, 1 | 24 << 16},
                                          {34, bytes}};

            for (const auto &field : fields) {
                memcpy(head + field[0], &field[1], 4);
            }
            fwrite(head, 1, sizeof head, file);
            /* a BMP's rows run from the bottom, each pixel blue first */
            for (int y = SCREEN_HEIGHT - 1; y >= 0; y--) {
                const uint8_t *from =
                    drawn + static_cast<size_t>(y) * BUFFER_WIDTH * 4;

                for (int x = 0; x < SCREEN_WIDTH; x++) {
                    row[x * 3] = from[x * 4 + 2];
                    row[x * 3 + 1] = from[x * 4 + 1];
                    row[x * 3 + 2] = from[x * 4];
                }
                fwrite(row, 1, sizeof row, file);
            }
            fclose(file);
        }
    }
    r.pictures++;
    r.list_open = false;
    if (r.vsync) {
        uint32_t began = sceKernelGetSystemTimeLow();

        sceDisplayWaitVblankStart();
        psp_wait_us += sceKernelGetSystemTimeLow() - began;
    }
    sceGuSwapBuffers();
    r.draws_at = r.draws_at == DRAW_AT ? SHOW_AT : DRAW_AT;
    return true;
}

bool Raster::submit(uint32_t, const PictureUniform &picture, uint32_t clear,
                    const std::vector<Vertex> &vertices,
                    const std::vector<Batch> &batches, bool sorted,
                    std::string &, const std::vector<float> *places)
{
    return psp_submit(picture, clear, vertices, batches, sorted, places);
}

void Raster::wide(bool on)
{
    if (impl_ != nullptr) {
        held.wide = on;
    }
}

bool Raster::broken(std::string &) const
{
    return false;
}

/* a picture is drawn by the time submit returns */
bool Raster::done(uint32_t)
{
    return true;
}

void Raster::wait(uint32_t)
{
}

uint32_t Raster::lost(uint32_t)
{
    return 0;
}

uint32_t Raster::cut(uint32_t)
{
    return 0;
}

uint64_t Raster::gpu_ns(uint32_t)
{
    return 0;
}

bool Raster::read(uint32_t, void *)
{
    return false;
}

/* draw.h's calls, for the PSP's model walk */
extern "C" int psp_picture_open(void)
{
    Held &r = held;

    if (!r.opened) {
        return 0;
    }
    open_list(r);
    return 1;
}

extern "C" void psp_draw_screen(float focal_x, float focal_y, float center_x,
                                float center_y)
{
    Held &r = held;
    const float p[4] = {focal_x, focal_y, center_x, center_y};

    if (memcmp(p, r.place.screen, sizeof p) != 0) {
        memcpy(r.place.screen, p, sizeof p);
        r.place_kept = false;
    }
    if (r.list_open && memcmp(p, r.screen, sizeof p) != 0 && list_room(r, 0)) {
        ScePspFMatrix4 project = projection(p);

        sceGuSetMatrix(GU_PROJECTION, &project);
        memcpy(r.screen, p, sizeof p);
    }
}

/* the engine takes a matrix by the images of the axes and of the origin,
 * three numbers each, which Ninja's rows are */
extern "C" void psp_draw_place(const float *matrix)
{
    Held &r = held;

    memcpy(r.place.matrix, matrix, sizeof r.place.matrix);
    r.place_kept = false;
    if (r.list_open && list_room(r, 0)) {
        sceGuSetMatrix(GU_MODEL,
                       reinterpret_cast<const ScePspFMatrix4 *>(matrix));
    }
}

/* a state goes to the engine by what it changes of the walk's last. the
 * first of a picture sets all of it, and the strips' own terms: no blend,
 * and a corner's u and v in 255ths */
extern "C" void psp_draw_state(const psp_state *state)
{
    Held &r = held;
    const psp_state &s = *state, &was = r.walked;
    bool all = !r.walking;
    const Texture *texture = s.texture != 0 && s.texture <= r.textures.size() &&
                                     r.textures[s.texture - 1].texels != nullptr
                                 ? &r.textures[s.texture - 1]
                                 : nullptr;

    if (!r.list_open || !list_room(r, 0)) {
        return;
    }
    if (all) {
        sceGuTexScale(32768.0F / 255.0F, 32768.0F / 255.0F);
    }
    if (all || s.blend != was.blend || s.src != was.src || s.dst != was.dst) {
        if (s.blend != 0) {
            int src, dst;
            uint32_t src_fixed, dst_fixed;

            factor(s.src, src, src_fixed);
            factor(s.dst, dst, dst_fixed);
            sceGuEnable(GU_BLEND);
            sceGuBlendFunc(GU_ADD, src, dst, src_fixed, dst_fixed);
        } else {
            sceGuDisable(GU_BLEND);
        }
    }
    if (all || s.texture != was.texture || s.decal != was.decal ||
        s.clamp_u != was.clamp_u || s.clamp_v != was.clamp_v ||
        s.point != was.point || s.cut != was.cut || s.blend != was.blend) {
        if (texture != nullptr) {
            int filter = s.point != 0 ? GU_NEAREST : GU_LINEAR;

            sceGuEnable(GU_TEXTURE_2D);
            sceGuTexMode(texture->format, 0, 0, texture->swizzled);
            sceGuTexImage(0, texture->width, texture->height,
                          texture->width < 8 ? 8 : texture->width,
                          texture->texels);
            sceGuTexFunc(s.decal != 0 ? GU_TFX_REPLACE : GU_TFX_MODULATE,
                         s.cut != 0 || s.blend != 0 ? GU_TCC_RGBA : GU_TCC_RGB);
            sceGuTexFilter(filter, filter);
            sceGuTexWrap(s.clamp_u != 0 ? GU_CLAMP : GU_REPEAT,
                         s.clamp_v != 0 ? GU_CLAMP : GU_REPEAT);
        } else {
            sceGuDisable(GU_TEXTURE_2D);
        }
    }
    if (all || s.cut != was.cut) {
        if (s.cut != 0) {
            sceGuEnable(GU_ALPHA_TEST);
            sceGuAlphaFunc(GU_GREATER, 0x7f, 0xff);
        } else {
            sceGuDisable(GU_ALPHA_TEST);
        }
    }
    if (all || s.depth != was.depth) {
        sceGuDepthFunc(DEPTH[s.depth & 7]);
    }
    if (all || s.depth_write != was.depth_write || s.blend != was.blend) {
        sceGuDepthMask(s.depth_write != 0 && s.blend == 0 ? GU_FALSE : GU_TRUE);
    }
    if (all || s.cull != was.cull) {
        if (s.cull != 0) {
            sceGuEnable(GU_CULL_FACE);
            sceGuFrontFace(s.cull == 1 ? GU_CCW : GU_CW);
        } else {
            sceGuDisable(GU_CULL_FACE);
        }
    }
    r.walked = s;
    r.walking = true;
}

extern "C" psp_corner *psp_draw_room(uint32_t corners, int later)
{
    Held &r = held;
    uint32_t bytes = corners * static_cast<uint32_t>(sizeof(psp_corner));

    if (!r.list_open || corners == 0 || corners > 65535) {
        return nullptr;
    }
    /* a kept strip's corners lie clear of the list, which may start again
     * before they draw. the engine reads memory past the processor's
     * cache, so they are written past it too */
    if (later != 0) {
        if (r.kept_used + bytes > KEPT_BYTES) {
            return nullptr;
        }
        uintptr_t at = reinterpret_cast<uintptr_t>(r.kept + r.kept_used);

        settle_work(r);
        r.kept_used += bytes;
        return reinterpret_cast<psp_corner *>(at | 0x40000000U);
    }
    if (!list_room(r, bytes)) {
        return nullptr;
    }
    return static_cast<psp_corner *>(sceGuGetMemory(static_cast<int>(bytes)));
}

extern "C" void psp_draw_strip(const psp_corner *corners, uint32_t count)
{
    list_room(held, 0);
    sceGuDrawArray(GU_TRIANGLE_STRIP, STRIP_FORMAT, static_cast<int>(count),
                   nullptr, corners);
}

extern "C" void psp_draw_later(float depth, const psp_state *state,
                               const psp_corner *corners, uint32_t count)
{
    Held &r = held;

    if (!r.list_open) {
        return;
    }
    if (!r.place_kept) {
        r.places.push_back(r.place);
        r.place_kept = true;
    }
    r.later.push_back({depth, static_cast<uint32_t>(r.places.size() - 1),
                       *state, corners, count, false});
}

/* a strip whose model may reach past the raster's reach of the frame: its
 * triangles, each whole where it lies within, cut where it crosses, and left
 * out where it lies past a side. they draw now under the state last set, or
 * where later is given stay for the picture's end under it, as
 * psp_draw_later's do */
extern "C" void psp_draw_cut(const psp_corner *corners, uint32_t count,
                             int reversed, const psp_state *later, float depth)
{
    Held &r = held;
    const float *m = r.place.matrix;
    /* Ninja's matrix takes a point as a row: its columns are the cut's
     * rows */
    const float rows[12] = {m[0], m[4],  m[8], m[12], m[1],  m[5],
                            m[9], m[13], m[2], m[6],  m[10], m[14]};
    ScePspFMatrix4 project = projection(r.place.screen);
    Cut cut = {rows, project.x.x, project.z.x, project.y.y, project.z.y};

    if (!r.list_open || count < 3) {
        return;
    }
    r.strip_cut.clear();
    for (uint32_t i = 0; i + 2 < count; i++) {
        /* a strip's triangles turn by turns, and a reversed strip's the
         * other way: each goes as the first of a strip that isn't */
        bool flip = ((i & 1) != 0) != (reversed != 0);
        const psp_corner tri[3] = {corners[flip ? i + 1 : i],
                                   corners[flip ? i : i + 1], corners[i + 2]};
        float in[3][SIDES];
        bool whole = true, gone = false;

        for (int c = 0; c < 3; c++) {
            cut.inside(&tri[c].x, in[c]);
        }
        for (int side = 0; side < SIDES; side++) {
            bool a = in[0][side] >= 0, b = in[1][side] >= 0,
                 c = in[2][side] >= 0;

            whole &= a && b && c;
            gone |= !a && !b && !c;
        }
        if (whole) {
            r.strip_cut.insert(r.strip_cut.end(), tri, tri + 3);
        } else if (!gone) {
            cut_triangle(cut, tri, r.strip_cut);
        }
    }
    uint32_t made = static_cast<uint32_t>(r.strip_cut.size());
    uint32_t bytes = made * static_cast<uint32_t>(sizeof(psp_corner));

    if (made == 0 || made > 65535) {
        return;
    }
    if (later != nullptr) {
        if (r.kept_used + bytes > KEPT_BYTES) {
            return;
        }
        uintptr_t at = reinterpret_cast<uintptr_t>(r.kept + r.kept_used);
        psp_corner *kept = reinterpret_cast<psp_corner *>(at | 0x40000000U);

        settle_work(r);

        r.kept_used += bytes;
        memcpy(kept, r.strip_cut.data(), bytes);
        if (!r.place_kept) {
            r.places.push_back(r.place);
            r.place_kept = true;
        }
        r.later.push_back({depth, static_cast<uint32_t>(r.places.size() - 1),
                           *later, kept, made, true});
        return;
    }
    if (!list_room(r, bytes)) {
        return;
    }
    void *room = sceGuGetMemory(static_cast<int>(bytes));

    memcpy(room, r.strip_cut.data(), bytes);
    sceGuDrawArray(GU_TRIANGLES, STRIP_FORMAT, static_cast<int>(made), nullptr,
                   room);
}

/* the open picture is dropped: the engine ends what it drew of it, and the
 * screen keeps the picture before */
extern "C" void psp_picture_drop(void)
{
    Held &r = held;

    end_list(r);
    r.list_open = false;
    r.later.clear();
    r.kept_used = 0;
}

/* the game is about to write the loader's work area, where the list and
 * the kept strips lie: the engine draws both now. the kept strips so draw
 * ahead of what the picture has yet to draw, which a picture drawn across
 * a load may show */
extern "C" void psp_draw_yield(void)
{
    Held &r = held;

    r.written = true;
    if (!r.opened) {
        return;
    }
    if (r.list_open && !r.later.empty()) {
        start_list(r);
        draw_later(r);
        r.walking = false;
        r.screen[0] = 0.0F;
    }
    end_list(r);
}

extern "C" float psp_draw_reach(void)
{
    return REACH;
}

extern "C" void psp_draw_report(void)
{
    uint32_t count = 0, plain = 0;

    for (const Texture &texture : held.textures) {
        count += texture.texels != nullptr;
        plain += texture.texels != nullptr && texture.format != GU_PSM_DXT1
                     ? texture.bytes
                     : 0;
    }
    fprintf(
        stderr,
        "sa1: a list of commands took at most %u bytes of %u, the "
        "pictures %u lists past one each, and the textures at most %u "
        "bytes, %u of them in video memory. %u textures hold %u bytes "
        "now, %u of them of 16 bits a texel\n",
        static_cast<unsigned>(held.most_list),
        static_cast<unsigned>(LIST_BYTES),
        static_cast<unsigned>(held.more_lists),
        static_cast<unsigned>(held.most_texels),
        static_cast<unsigned>(held.most_video), static_cast<unsigned>(count),
        static_cast<unsigned>(held.texel_bytes), static_cast<unsigned>(plain));
}

/* the engine may still read a texture a list names, so it draws what it
 * holds first */
extern "C" void psp_texture_free(uint32_t texture)
{
    end_list(held);
    free_texture(held, texture);
}

/* a texture of texels the game made in memory, as a PVR's chunk holds them:
 * pixel and layout are the chunk's two bytes. 0 where the raster reads no
 * such format or has no memory */
extern "C" uint32_t psp_texture_texels(uint32_t pixel, uint32_t layout,
                                       uint32_t width, uint32_t height,
                                       const void *texels, uint32_t bytes)
{
    static std::vector<uint8_t> chunk;
    uint32_t length = 8 + bytes;
    bool tried;
    std::string why;

    if (!held.opened || texels == nullptr || bytes == 0) {
        return 0;
    }
    chunk.assign(16, 0);
    memcpy(chunk.data(), "PVRT", 4);
    memcpy(chunk.data() + 4, &length, 4);
    chunk[8] = static_cast<uint8_t>(pixel);
    chunk[9] = static_cast<uint8_t>(layout);
    chunk[12] = static_cast<uint8_t>(width);
    chunk[13] = static_cast<uint8_t>(width >> 8);
    chunk[14] = static_cast<uint8_t>(height);
    chunk[15] = static_cast<uint8_t>(height >> 8);
    chunk.insert(chunk.end(), static_cast<const uint8_t *>(texels),
                 static_cast<const uint8_t *>(texels) + bytes);
    tried = false;
    uint32_t made = add_pvr(held, chunk.data(), chunk.size(), tried, why);

    /* a format add_pvr reads none of, as a movie's picture is, goes by an
     * image made of it, as a disc's does (textures.cpp) */
    if (made == 0 && !tried) {
        Image image;

        if (pvr_decode(chunk, 0, image, why)) {
            made = add_texture(held, image, why);
        }
    }
    if (made == 0) {
        static int said;

        if (said++ < 4) {
            fprintf(stderr, "sa1: no texture of the game's texels, %s: %s\n",
                    tried ? "which the raster reads" : "by an image",
                    why.c_str());
        }
    }
    /* a large texture's room goes back */
    if (chunk.capacity() > (64U << 10)) {
        std::vector<uint8_t>().swap(chunk);
    }
    return made;
}

extern "C" int psp_texture_cuts(uint32_t texture)
{
    return texture != 0 && texture <= held.textures.size() &&
           held.textures[texture - 1].texels != nullptr &&
           held.textures[texture - 1].cuts;
}

/* the raster's texture of image, or 0, for psp_texture_load */
uint32_t psp_texture_add(const Image &image)
{
    std::string why;

    return held.opened ? add_texture(held, image, why) : 0;
}

/* the raster's texture of a PVR's chunk, or 0. tried is false where the
 * chunk's format is one add_pvr reads none of */
uint32_t psp_texture_add_pvr(const std::vector<uint8_t> &chunk, bool &tried)
{
    std::string why;

    tried = false;
    return held.opened ? add_pvr(held, chunk.data(), chunk.size(), tried, why)
                       : 0;
}

} // namespace pc
