/* SA1's compatibility layer for its model draws: the names model.src gives,
 * in C. make dc compiles it in place of model.src. the layer keeps Ninja's
 * state where Ninja keeps it. it hands each draw to the Dreamcast backend as
 * an SA_SA1_BASIC sa_draw_live, whose state says how it goes.
 * engine/dc/sa1/live.h gives sa_present_live inline, so each call compiles
 * into the walk's entry for that state. the walk, engine/dc/sa1/model.c,
 * draws the model and sends it before sa_present_live returns. so the state
 * it leaves is in place for the next draw, as model.src leaves it.
 *
 * the layer keeps model.src's arithmetic in draw_in_view's test, which play
 * reads. it departs from model.src in these ways.
 * - the draws send nothing while event_skipping is TRUE, when a skip's
 *   curtain covers the screen.
 * - draw_model and draw_model_nested draw on step 1 too where port.h's
 *   FRAME_DRAWS says, for a frame's second picture.
 * - draw_in_view takes the window's aspect, port_view_aspect, where
 *   model.src takes _nj_scr_aspect_x_. the draws take the picture's,
 *   port_draw_aspect.
 * - draw_model_init leaves GBR as it was, since no draw here reads GBR.
 *   beside what model.h says it copies, it takes the screen's width and
 *   height, and where _nj_vertex_buf_ lies. model.src's project_points reads
 *   _nj_vertex_buf_ at each draw. a change to any of the three takes another
 *   call.
 * - draw_tree_like_ninja and draw_action_like_ninja draw nested models in
 *   Ninja's forms, as model.h says. model.src has no such draws */
#ifdef NONMATCHING
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/polygon.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "sa1/live.h"
#include "sdk/ninja/njkamui.h"
#include "system/frame.h"

/* the walk lays out its palettes and scratch by SA1's numbers, which must
 * be model.h's */
_Static_assert(DRAW_PALETTES == SA_PALETTES &&
                   DRAW_PALETTE_ENTRIES == SA_PALETTE_ENTRIES &&
                   DRAW_MOST_POINTS == SA_UNLIT_OFFSETS,
               "model.h's palettes or points aren't sa_sa1.h's");

#ifdef DREAMCAST
/* the Ninja words the walk reads and leaves, and the calls that open and
 * close Ninja's context. each of live.h's names is Ninja's own symbol, so
 * the walk reads each word in place */
extern Uint32 sa1_live_control __asm__("__nj_control_3d_flag_");
extern Uint32 sa1_live_attr_and __asm__("__nj_constant_attr_and_");
extern Uint32 sa1_live_attr_or __asm__("__nj_constant_attr_or_");
extern struct nj_parameterkm sa1_live_header __asm__("__nj_parameterkm_");
extern Uint32 sa1_live_texture __asm__("__nj_current_texture");
extern NJS_TEXLIST *sa1_live_texlist __asm__("__nj_current_texlist");
extern NJS_ARGB sa1_live_material __asm__("__nj_constant_material_");
extern Uint32 sa1_live_sq_base __asm__("__nj_sq_base_");
extern Uint32 **sa1_live_list __asm__("__nj_listtype_");
extern NJS_SCREEN sa1_live_ninja_screen __asm__("__nj_screen_");
extern Float sa1_live_far_clip __asm__("__nj_far_clip_");
extern NJS_POINT2 sa1_live_clip[2] __asm__("__nj_2d_clip_");
extern Float sa1_live_aspect_y __asm__("__nj_scr_aspect_y_");
extern Float sa1_live_near_clip __asm__("__nj_near_clip_");
extern void sa1_live_open(void) __asm__("_njGetkmPointer");
extern void sa1_live_close(void) __asm__("_njSetkmPointer");

/* the walk compiles in the layer's object, so its code and data link where
 * model.src's lay. it names nothing of the layer's, as
 * engine/tests/boundary.py holds. the layer reaches it through
 * sa_present_live, sa1_live_screen, sa1_live_run, sa1_live_forms and
 * sa1_live_forget alone */
#include "sa1/model.c"
#endif

/* the palette the models shade by, the light in view space, and the light
 * draw_set_light gave */
const Uint32 *sa1_live_palette;
NJS_VECTOR sa1_live_light;
static NJS_VECTOR light_base;

/* palette p takes the operand cache's sets 64p to 64p + 63, and
 * draw_unlit_offsets the first 256. while polygon_sweep's first whole line
 * lies past them, the draws' scratch misses palettes 0 to 3's sets. it
 * takes them only by running on past the cache's last set. GCC lays out
 * .bss last definition first, after the walk's bakes. so 124 bytes of the
 * light, the palette and the walk's other data follow draw_colors, as
 * model.src's did. what this file defines below lies in the room
 * draw_colors' alignment leaves */
#ifdef DREAMCAST
union draw_colors draw_colors __attribute__((aligned(OC_SIZE), nocommon));
#else
/* a host has no operand cache to place it by, and COFF aligns 8 KB at most */
union draw_colors draw_colors __attribute__((nocommon));
#endif
/* draw_colors' words, as live.h lends them. an alias gives each name one
 * type. an alias must name a definition, which a common symbol isn't, so
 * draw_colors is nocommon */
extern Uint32
    sa1_live_colors[SA_PALETTES * SA_PALETTE_ENTRIES * 2 + SA_UNLIT_OFFSETS]
    __attribute__((alias("draw_colors")));
int draw_debug_counts[DRAW_DEBUG_COUNTS];

/* the scratch draw_model_init lays out, for DRAW_MOST_POINTS points, fits
 * in DRAW_SCRATCH_SIZE */
_Static_assert(OC_SIZE + (DRAW_MOST_POINTS * 32) <= DRAW_SCRATCH_SIZE &&
                   DRAW_MOST_POINTS >= 4,
               "the draws' scratch may run past DRAW_SCRATCH_SIZE");

/* have the walk take Ninja's screen, with port_draw_aspect for Ninja's x
 * aspect, and place its scratch in _nj_vertex_buf_ */
void draw_model_init(void)
{
    Uint32 buffer = (Uint32)_nj_vertex_buf_;
    Uint32 sweep = ((Uint32)polygon_sweep + OC_LINE - 1) & ~(OC_LINE - 1);
    /* sweep is polygon_sweep's first whole line. no draw reads its literal
     * pools, so the scratch takes its sets first. project_points' first pass
     * writes the buffer's line in sweep's set. the scratch starts on the
     * next, at most OC_SIZE bytes in. a model's n points take 16n bytes,
     * and their lit and unlit colors 8n each after them. while
     * project_points runs, its two lines of XMTRX lie past the points. they
     * pass the colors' end only for n under 4, and stay within the
     * scratch's first 128 bytes. Ninja's draws and texture loads, which run
     * between the draws here, take the buffer from its start */
    void *scratch =
        (void *)(buffer + ((sweep - buffer) & (OC_SIZE - 1)) + OC_LINE);

    sa1_live_screen(port_draw_aspect, scratch);
    sa1_live_palette = draw_colors.words;
}

ALWAYS_INLINE void draw_live(NJS_MODEL *model, Uint32 how)
{
    struct sa1_live_state state = {how};
    sa_draw_live draw = {model, &state, SA_SA1_BASIC, 0};

    sa_present_live(SA_DC_ENGINE, &draw);
}

/* draw_model's and draw_model_nested's draw, as how says */
ALWAYS_INLINE void draw_framed(NJS_MODEL *model, Uint32 how)
{
    if (!FRAME_DRAWS() || event_skipping) {
        return;
    }
    present_untimed(TRUE);
    present_record(PRESENT_MODEL, model, 0, 0, 0.0F, 0);
    draw_live(model, how);
    present_untimed(FALSE);
}

void draw_model(NJS_MODEL *model)
{
    draw_framed(model, SA1_LIVE_MODEL);
}

void draw_model_nested(NJS_MODEL *model)
{
    draw_framed(model, SA1_LIVE_NESTED);
}

void draw_model_unculled(NJS_MODEL *model)
{
    if (event_skipping) {
        return;
    }
    draw_live(model, SA1_LIVE_LAND);
}

void draw_models_begin(void)
{
    sa1_live_run();
    njGetkmPointer();
}

void draw_models_end(void)
{
    /* the walk's set_header may have cleared MODULATE_ALPHA from the TSP
     * word */
    _nj_parameterkm_.TSPPARAMBUFFER |= MODULATE_ALPHA;
    njSetkmPointer();
}

void draw_bake_forget(void)
{
    sa1_live_forget();
}

/* whether model.h's draw_in_view test passes the sphere of radius r about
 * center, in view space. the edge tests project the sphere's sides at its
 * far z, center.z + r. the walk's model_in_view tests alike, on the
 * picture's aspect, but for the departures its file lists */
static inline Bool sphere_in_window(const NJS_POINT3 *center, Float r,
                                    Float side, Float bottom, Float top)
{
    Float inv;

    if (!(center->z > _nj_near_clip_ - r) || center->z > _nj_far_clip_ + r) {
        return FALSE;
    }
    inv = 1.0F / (center->z + r);
    side *= inv;
    bottom *= inv;
    top *= inv;
    if (!((center->x + r) * side + _nj_screen_.cx > _nj_2d_clip_[0].x) ||
        (center->x - r) * side + _nj_screen_.cx > _nj_2d_clip_[1].x) {
        return FALSE;
    }
    /* @bug logic: the bottom test compares y with the clip's right edge,
     * _nj_2d_clip_[1].x, not its bottom, _nj_2d_clip_[1].y. model.src loads
     * the bottom into FR9 and compares y with FR8, the right edge, in
     * draw_in_view at 0x8c039d94 and 0x8c039d9c */
    return (center->y + r) * top + _nj_screen_.cy > _nj_2d_clip_[0].y &&
           !((center->y - r) * bottom + _nj_screen_.cy > _nj_2d_clip_[1].x);
}

Bool draw_in_view_any_step(Float x, Float y, Float z, Float radius)
{
    NJS_POINT3 center = {x, y, z};
    Float side = port_view_aspect * _nj_screen_.dist;

    /* @bug logic: the bottom test scales y by the side tests' scale, and the
     * top test by _nj_scr_aspect_y_ times _nj_screen_.cx, neither by dist
     * times _nj_scr_aspect_y_. model.src's FMUL FR2,FR11 at 0x8c039d98 scales
     * the bottom test's y by the side tests' scale, and its FMUL FR7,FR3 at
     * 0x8c039d82 multiplies _nj_scr_aspect_y_ by cx for the top test's
     * scale */
    njCalcPoint(NULL, &center, &center);
    return sphere_in_window(&center, radius, side, side,
                            _nj_scr_aspect_y_ * _nj_screen_.cx);
}

Bool draw_in_view(Float x, Float y, Float z, Float radius)
{
    return frame_step == 0 && draw_in_view_any_step(x, y, z, radius);
}

void draw_set_light(Float x, Float y, Float z)
{
    sa1_live_light.x = light_base.x = x;
    sa1_live_light.y = light_base.y = y;
    sa1_live_light.z = light_base.z = z;
}

void draw_divide_light(Float scale)
{
    Float inv = 1.0F / scale;

    sa1_live_light.x = light_base.x * inv;
    sa1_live_light.y = light_base.y * inv;
    sa1_live_light.z = light_base.z * inv;
}

void draw_reset_light(void)
{
    sa1_live_light = light_base;
}

void draw_set_palette(int palette)
{
    sa1_live_palette = &draw_colors.words[palette * DRAW_PALETTE_ENTRIES * 2];
}

void draw_get_shading(struct draw_shading *out)
{
    out->dir = sa1_live_light;
    out->base = light_base;
    out->palette = sa1_live_palette;
}

void draw_put_shading(const struct draw_shading *in)
{
    sa1_live_light = in->dir;
    light_base = in->base;
    sa1_live_palette = in->palette;
}

/* whether Ninja's 3D control flags and constant attributes ask for the
 * forms model.h gives draw_tree_like_ninja */
static inline Bool in_ninja_forms(void)
{
    return (_nj_control_3d_flag_ & NINJA_FORMS_CONTROL) ==
               NINJA_FORMS_CONTROL &&
           (_nj_constant_attr_or_ & UNLIT_TRANSLUCENT) == UNLIT_TRANSLUCENT;
}

LINKED_LAST(draw_tree_like_ninja)
void draw_tree_like_ninja(NJS_OBJECT *object)
{
    if (!in_ninja_forms()) {
        njDrawObject(object);
        return;
    }
    sa1_live_forms(TRUE);
    draw_tree(object);
    sa1_live_forms(FALSE);
}

LINKED_LAST(draw_action_like_ninja)
void draw_action_like_ninja(NJS_ACTION *action, Float frame)
{
    if (!in_ninja_forms()) {
        njAction(action, frame);
        return;
    }
    sa1_live_forms(TRUE);
    draw_action(action, frame);
    sa1_live_forms(FALSE);
}

/* the walk keeps no point light queue or list, so there is nothing to take */
void draw_take_point_lights(void)
{
}
#endif
