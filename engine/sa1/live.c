/* SA1's model walk on a host, behind sa_present_live. live.h says why it is
 * out of line. at each draw it makes the draw's record from the state at the
 * call. it then changes Ninja's context as model.src does, for the next draw
 * to inherit. tests of the view and the near clip decide the change. what
 * only the hardware reads stays out: the points' projection and colors, and
 * the strips.
 *
 * the tests decide state a later draw reads, so they run in the game's
 * arithmetic. each takes its float operations in model.src's order, under
 * the rounding engine/native/runner.c sets for the game. FTRV is
 * play_math.h's sa_ftrv */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <string.h>

#include "native.h"
#include "ninja/njkamui.h"

/* rules.h's set_header reads and leaves Ninja's words by the first three
 * names, which header.h declares on the Dreamcast. a texture name's texaddr
 * holds a 32-bit reference, which TEXTURE_RECORD widens to a host pointer */
#define sa1_live_header _nj_parameterkm_
#define sa1_live_texture _nj_current_texture
#define sa1_live_texlist _nj_current_texlist
#define TEXTURE_RECORD(texaddr) native_widen(texaddr)

#include "live.h"
#include "play_math.h"
#include "sa1.h"

extern float _nj_far_clip_;

struct sa1_screen sa1_screen;
/* the constant attributes the last run or draw in view took. a run's land
 * draws read them */
static struct sa1_attrs taken;
static float view_dist, view_aspect_y;

void sa1_live_screen(Float aspect_x, void *scratch)
{
    (void)scratch;
    sa1_screen.cx = _nj_screen_.cx;
    sa1_screen.cy = _nj_screen_.cy;
    view_dist = _nj_screen_.dist;
    view_aspect_y = _nj_scr_aspect_y_;
    sa1_screen.near_clip = _nj_near_clip_;
    sa1_screen.scale_x = aspect_x * view_dist;
    sa1_screen.scale_y = view_aspect_y * view_dist;
}

/* whether the layer has model.h's Ninja forms on. on the Dreamcast they
 * apply to a nested draw alone, so only its record takes them */
static Bool forms;

void sa1_live_forms(Bool on)
{
    forms = on;
}

void sa1_live_forget(void)
{
}

struct sa1_attrs sa1_attrs_now(void)
{
    struct sa1_attrs attrs = {
        (_nj_control_3d_flag_ & NJD_CONTROL_3D_CONSTANT_ATTR) != 0,
        _nj_control_3d_flag_, _nj_constant_attr_and_, _nj_constant_attr_or_,
        FALSE};

    return attrs;
}

void sa1_live_run(void)
{
    taken = sa1_attrs_now();
}

static inline uint32_t drawn_flags(const NJS_MATERIAL *material)
{
    uint32_t flags = material->attrflags;

    return taken.constant ? (flags & taken.and_bits) | taken.or_bits : flags;
}

/* the header words as a model's meshsets leave them, in order */
static void meshes_state(const NJS_MODEL *model)
{
    const NJS_MESHSET *meshset = model->meshsets;

    for (int left = model->nbMeshset; left != 0; left--, meshset++) {
        const NJS_MATERIAL *material =
            &model->mats[meshset->type_matId & MATERIAL_ID];
        uint32_t drawn = drawn_flags(material);

        set_header(material, drawn, drawn_mode(drawn));
    }
}

/* where a point lies in view space, by the game's FTRV */
static inline void view_point(const NJS_POINT3 *p, float out[4])
{
    float in[4] = {p->x, p->y, p->z, 1.0F};

    sa_ftrv(native_xmtrx(), in, out);
}

/* model.src's cull_model, with its bugs. the top test scales y by
 * aspect_y times cx. the bottom test scales y by dist, and compares it with
 * the clip's right edge */
static Bool model_in_view(const NJS_MODEL *model)
{
    float at[4];
    float r = model->r;
    float near_clip = sa1_screen.near_clip;
    float side = sa1_screen.scale_x;
    float bottom = view_dist;
    float top = view_aspect_y * _nj_screen_.cx;
    float inverse;

    view_point(&model->center, at);
    if (!(at[2] > near_clip - r) || at[2] > _nj_far_clip_ + r) {
        return FALSE;
    }
    inverse = 1.0F / (at[2] + r);
    side *= inverse;
    bottom *= inverse;
    top *= inverse;
    if (!((at[0] + r) * side + _nj_screen_.cx > _nj_2d_clip_[0].x) ||
        (at[0] - r) * side + _nj_screen_.cx > _nj_2d_clip_[1].x) {
        return FALSE;
    }
    return (at[1] + r) * top + _nj_screen_.cy > _nj_2d_clip_[0].y &&
           !((at[1] - r) * bottom + _nj_screen_.cy > _nj_2d_clip_[1].x);
}

/* whether any of model's points lies farther than the near clip. it tests
 * nbPoint points, where model.src's project_points tests one more for an
 * odd count */
static Bool any_past_near(const NJS_MODEL *model)
{
    for (int i = 0; i < model->nbPoint; i++) {
        float at[4];

        view_point(&model->points[i], at);
        if (at[2] > sa1_screen.near_clip) {
            return TRUE;
        }
    }
    return FALSE;
}

static inline void reset_shading(void)
{
    _nj_parameterkm_.TSPPARAMBUFFER |= MODULATE_ALPHA;
}

static void live_model(const NJS_MODEL *model)
{
    struct sa1_attrs now = sa1_attrs_now();
    Bool in = model_in_view(model);

    sa1_record_model(model, SA_SHADE_PALETTE, SA1_CONTEXT, &now,
                     in ? 0 : SA_DRAW_OUT);
    if (in) {
        taken = now;
        if (any_past_near(model)) {
            njGetkmPointer();
            meshes_state(model);
            reset_shading();
            njSetkmPointer();
        }
    }
}

/* SA1_LIVE_NESTED, in a context its caller opened. as in model.src, the
 * shading goes back to MODULATE_ALPHA for any model in view. taken takes
 * the attributes without the forms, since a land draw that reads them is in
 * none */
static void live_nested(const NJS_MODEL *model)
{
    struct sa1_attrs now = sa1_attrs_now();
    struct sa1_attrs recorded = now;
    Bool in = model_in_view(model);

    recorded.forms = forms;
    sa1_record_model(model, SA_SHADE_PALETTE, SA1_RUN, &recorded,
                     in ? 0 : SA_DRAW_OUT);
    if (in) {
        taken = now;
        if (any_past_near(model)) {
            meshes_state(model);
        }
        reset_shading();
    }
}

/* SA1_LIVE_LAND: a model of a run, with no view test, under the attributes
 * in taken */
static void live_land(const NJS_MODEL *model)
{
    sa1_record_model(model, SA_SHADE_PALETTE, SA1_RUN, &taken, 0);
    if (any_past_near(model)) {
        meshes_state(model);
    }
}

sa_result sa_present_live(sa_engine *engine, const sa_draw_live *draw)
{
    const NJS_MODEL *model = draw->geometry;
    uint32_t how;

    (void)engine;
    if (draw->format != SA_SA1_BASIC) {
        return SA_ERR_UNSUPPORTED;
    }
    how = ((const struct sa1_live_state *)draw->state)->how;
    if (how == SA1_LIVE_LAND) {
        live_land(model);
    } else if (how == SA1_LIVE_MODEL) {
        live_model(model);
    } else {
        live_nested(model);
    }
    return SA_OK;
}
