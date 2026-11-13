/* Ninja's drawers. their source runs here as it stands, but for the bodies
 * only SHC compiles. those bodies project a model's points and write its
 * vertices. here they give only what Ninja's C branches on. they count the
 * points behind the eye, set the clip flags and give where a point lies. the
 * drawers here record each draw they pass to Ninja's own. a clip test scales
 * x by port_draw_aspect, as the non-matching drawers do for 16:9 */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>

#include <string.h>

#include "ninja/njkamui.h"
#include "native.h"
#include "play_math.h"
#include "rules.h"
#include "sa1.h"
#include "sa_engine.h"

void njInitGlobal(void);

/* njModelE.c's struct calc_work through normals, which easy_calc_body reads */
struct easy_work {
    NJS_VECTOR light_direction;
    float negative_diffuse;
    int32_t point_count;
    void *queue_points;
    NJS_POINT3 *points;
    NJS_VECTOR *normals;
};

void ninja_easy_draw_model(NJS_MODEL *model);
void ninja_simple_draw_model(NJS_MODEL *model);
void __real_njDrawTriangle3D(NJS_POINT3COL *p, Int n, Uint32 attr);
void __real_njDrawPolygon2D(NJS_POINT2COL *p, Int n, Float pri, Uint32 attr);
void __real_njDrawLine3D(NJS_POINT3COL *p, Int n, Uint32 attr);
void __real_njDrawTexture(NJS_TEXTURE_VTX *polygon, Int count, Int tex,
                          Int flag);
void __real_njDrawPolygon(NJS_POLYGON_VTX *polygon, Int count, Int trans);
int32_t easy_calc_body(struct easy_work *work);
void SimpleCalcModelMatrix(NJS_POINT3 *out, NJS_POINT3 *from);
static int easy_out(const NJS_MODEL *model);
static int simple_out(const NJS_MODEL *model);
static int all_clipped(const NJS_MODEL *model, float px, float py, int full);

/* SHC's intrinsics that Ninja's C reaches. a PREF has no store queue to
 * send here. st_ext stores XMTRX */
void _builtin_prefetch(void *address)
{
    (void)address;
}

void _builtin_st_ext(float (*matrix)[4])
{
    memcpy(matrix, native_xmtrx(), 16 * sizeof(float));
}

char *_builtin_strcpy(char *to, const char *from)
{
    return strcpy(to, from);
}

/* the lists' write pointers, which reach no hardware here. njInitSystem keeps
 * only what the 640 by 480 picture needs of Ninja's own */
static PKMDWORD list_pointers[8];

void native_lists_over(void)
{
    memset(list_pointers, 0, sizeof list_pointers);
}

void njInitSystem(Int mode, Int frame, Int count)
{
    (void)mode, (void)frame, (void)count;
    _nj_vertex_buffer_.pCurrentPtr = list_pointers;
    _nj_screen_.w = 640.0F;
    _nj_screen_.h = 480.0F;
    njInitGlobal();
}

/* njSetBackColor's plane, whose colors a picture takes */
uint32_t sa1_back[3] = {0xff000000U, 0xff000000U, 0xff000000U};

KMSTATUS kmSetBackGroundPlane(PVOID pVertex[3], KMVERTEXTYPE type, KMINT32 size)
{
    (void)type, (void)size;
    for (int i = 0; i < 3; i++) {
        sa1_back[i] = (uint32_t)((KMVERTEX0 *)pVertex[i])->uBaseRGB.dwPacked;
    }
    return KMSTATUS_SUCCESS;
}

/* the attributes one of Ninja's own drawers takes. forms is TRUE where the
 * control flags hold NINJA_FORMS_CONTROL and the attributes it ors in hold
 * UNLIT_TRANSLUCENT */
static struct sa1_attrs attrs_now(void)
{
    struct sa1_attrs attrs = {
        (_nj_control_3d_flag_ & NJD_CONTROL_3D_CONSTANT_ATTR) != 0,
        _nj_control_3d_flag_, _nj_constant_attr_and_, _nj_constant_attr_or_,
        (_nj_control_3d_flag_ & NINJA_FORMS_CONTROL) == NINJA_FORMS_CONTROL &&
            (_nj_constant_attr_or_ & UNLIT_TRANSLUCENT) == UNLIT_TRANSLUCENT};

    return attrs;
}

void _njEasyDrawModel(NJS_MODEL *model)
{
    struct sa1_attrs now = attrs_now();

    sa1_record_model(model, SA_SHADE_RAMP, SA1_RUN, &now,
                     easy_out(model) ? SA_DRAW_OUT : 0);
    ninja_easy_draw_model(model);
}

void _njSimpleDrawModelSub(NJS_MODEL *model)
{
    struct sa1_attrs now = attrs_now();

    sa1_record_model(model, SA_SHADE_LINEAR, SA1_RUN, &now,
                     simple_out(model) ? SA_DRAW_OUT : 0);
    ninja_simple_draw_model(model);
}

/* the slice doesn't build Ninja's full drawer, njModel.c. here its entries
 * the game reaches record each model with Ninja's lights, and draw nothing.
 * full_out flags SA_DRAW_OUT a model the drawer sends nothing for, by
 * _njDrawModelSub's and _njCalcModel's tests. it flags a model of more points
 * than _nj_vertex_num_. under NJD_CONTROL_3D_MODEL_CLIP, it flags one whose
 * sphere lies outside the clips. without NJD_CONTROL_3D_NO_CLIP_CHECK, it
 * flags one whose points all hold one clip flag. the game sets no
 * NJD_CONTROL_3D_VERTEX_OFFSET, under which the drawer first moves each point
 * along its normal */
static uint16_t full_out(const NJS_MODEL *model)
{
    const float *m = native_xmtrx();
    float x = model->center.x * m[0] + model->center.y * m[4] +
              model->center.z * m[8] + m[12];
    float y = model->center.x * m[1] + model->center.y * m[5] +
              model->center.z * m[9] + m[13];
    float z = model->center.x * m[2] + model->center.y * m[6] +
              model->center.z * m[10] + m[14];
    float r = model->r;
    float px = _nj_screen_.dist * port_draw_aspect;
    float py = _nj_screen_.dist * _nj_scr_aspect_y_;
    float inv;

    if (model->nbPoint > _nj_vertex_num_) {
        return SA_DRAW_OUT;
    }
    if (_nj_control_3d_flag_ & NJD_CONTROL_3D_MODEL_CLIP) {
        if (!(z + r > _nj_near_clip_) || z - r > _nj_far_clip_) {
            return SA_DRAW_OUT;
        }
        inv = 1.0F / (z + r);
        if (_nj_2d_clip_[0].x > px * (x + r) * inv + _nj_screen_.cx ||
            px * (x - r) * inv + _nj_screen_.cx > _nj_2d_clip_[1].x ||
            py * (y + r) * inv + _nj_screen_.cy < _nj_2d_clip_[0].y ||
            py * (y - r) * inv + _nj_screen_.cy > _nj_2d_clip_[1].y) {
            return SA_DRAW_OUT;
        }
    }
    if (_nj_control_3d_flag_ & NJD_CONTROL_3D_NO_CLIP_CHECK) {
        return 0;
    }
    return all_clipped(model, px, py, TRUE) ? SA_DRAW_OUT : 0;
}

static void full_draw(NJS_MODEL *model)
{
    struct sa1_attrs now = attrs_now();

    sa1_record_model(model, SA_SHADE_LIGHTS, SA1_RUN, &now, full_out(model));
}

/* _njDrawObjectSub's walk. njModel.c spells it as a case for each set of
 * NJD_EVAL_UNIT_ bits */
static void full_walk(NJS_OBJECT *object)
{
    const Uint32 unit =
        NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL;

    do {
        Uint32 flags = object->evalflags;

        if ((flags & unit) != unit) {
            njPushMatrix(NULL);
        }
        if (!(flags & NJD_EVAL_UNIT_POS)) {
            njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
        }
        if (!(flags & NJD_EVAL_UNIT_ANG)) {
            if (flags & NJD_EVAL_ZXY_ANG) {
                njRotateZXY(NULL, object->ang[0], object->ang[1],
                            object->ang[2]);
            } else {
                njRotateXYZ(NULL, object->ang[0], object->ang[1],
                            object->ang[2]);
            }
        }
        if (!(flags & NJD_EVAL_UNIT_SCL)) {
            njScale(NULL, object->scl[0], object->scl[1], object->scl[2]);
        }
        if (!(flags & NJD_EVAL_HIDE)) {
            full_draw(object->model);
        }
        if (!(flags & NJD_EVAL_BREAK)) {
            full_walk(object->child);
        }
        if ((flags & unit) != unit) {
            njPopMatrix(1);
        }
        object = object->sibling;
    } while (object);
}

void njDrawObject(NJS_OBJECT *object)
{
    njGetkmPointer();
    full_walk(object);
    njSetkmPointer();
}

void njDrawModel(NJS_MODEL *model)
{
    njGetkmPointer();
    full_draw(model);
    njSetkmPointer();
}

/* what njMotion.c's walks call of the full drawer. each record takes Ninja's
 * lights itself, so the queue they set up holds nothing here */
void njDrawModelSub(NJS_MODEL *model)
{
    full_draw(model);
}

void njSetQueue(void)
{
}

void njSetLightMaterials(void)
{
}

void njCalcLightMatrices(void)
{
}

void njResetQueue(void)
{
}

void __wrap_njDrawTriangle3D(NJS_POINT3COL *p, Int n, Uint32 attr)
{
    sa1_record_triangles(p, n, attr);
    __real_njDrawTriangle3D(p, n, attr);
}

/* the screen's polygons, as fades draw them. the record follows the call,
 * since it reads the header words the call leaves in Kamui's context. each
 * 2D draw asks present_record first, as the Dreamcast's layer does */
void __wrap_njDrawPolygon2D(NJS_POINT2COL *p, Int n, Float pri, Uint32 attr)
{
    if (!present_record(PRESENT_POLYGON_2D, p, n, 0, pri, attr)) {
        return;
    }
    __real_njDrawPolygon2D(p, n, pri, attr);
    sa1_record_polygon_2d(p, n, pri, attr);
}

/* njDrawLine3D's lines, as sparks and Chaos 0's rain draw them. this record
 * too reads what the call leaves in Kamui's context */
void __wrap_njDrawLine3D(NJS_POINT3COL *p, Int n, Uint32 attr)
{
    __real_njDrawLine3D(p, n, attr);
    sa1_record_lines(p, n, attr);
}

/* and the screen's strips, as a title card and a panel draw them */
void __wrap_njDrawTexture(NJS_TEXTURE_VTX *polygon, Int count, Int tex,
                          Int flag)
{
    if (!present_record(PRESENT_TEXTURE, polygon, count, tex, 0.0F,
                        (Uint32)flag)) {
        return;
    }
    sa1_record_screen_strip(polygon, count, 1, (uint32_t)tex, flag != 0);
    __real_njDrawTexture(polygon, count, tex, flag);
}

void __wrap_njDrawPolygon(NJS_POLYGON_VTX *polygon, Int count, Int trans)
{
    if (!present_record(PRESENT_POLYGON, polygon, count, 0, 0.0F,
                        (Uint32)trans)) {
        return;
    }
    sa1_record_screen_strip(polygon, count, 0, 0, trans != 0);
    __real_njDrawPolygon(polygon, count, trans);
}

/* njEasyCalcModel's body: the points whose z is 0 or less, which decide
 * whether the model draws at all */
int32_t easy_calc_body(struct easy_work *work)
{
    int32_t behind = 0;

    for (int32_t i = 0; i < work->point_count; i++) {
        const NJS_POINT3 *p = &work->points[i];
        float in[4] = {p->x, p->y, p->z, 1.0F};
        float at[4];

        sa_ftrv(native_xmtrx(), in, at);
        behind += !(at[2] > 0.0F);
    }
    return behind;
}

/* njModelS.c's struct calc_work, field for field, which its loops take */
struct simple_work {
    int32_t count;
    uint8_t *out;
    NJS_POINT3 *points;
    NJS_VECTOR *normals;
    uint32_t clip_and;
    uint32_t clip_or;
    float scale_x, scale_y;
    float clip_left, clip_right, clip_top, clip_bottom;
};

/* a point's clip flags, as njModelS.c names them. the four sides' flags lie
 * above CLIP_NEAR and CLIP_FAR, and CLIP_ALL holds all six */
#define CLIP_NEAR 1U
#define CLIP_FAR 2U
#define CLIP_ALL 0x3fU

/* the clip flags a drawer's point loops set, by their float operations. a
 * point nearer than the near clip takes no side flag. the simple drawer's
 * loops 3 and 4 take 1/|z| as FSRRA of z * z. where full is TRUE, 1/z is the
 * quotient the full drawer's loops 8, 9 and A take */
static void clip_flags(struct simple_work *work, int full)
{
    uint32_t all = ~0U;
    uint32_t any = 0;

    for (int32_t i = 0; i < work->count; i++) {
        const NJS_POINT3 *p = &work->points[i];
        float in[4] = {p->x, p->y, p->z, 1.0F};
        float at[4];
        uint32_t flags = 0;

        sa_ftrv(native_xmtrx(), in, at);
        if (!(_nj_near_clip_ > at[2])) {
            float inverse = full ? 1.0F / at[2] : sa_fsrra(at[2] * at[2]);
            float x = at[0] * work->scale_x * inverse + _nj_screen_.cx;
            float y = at[1] * work->scale_y * inverse + _nj_screen_.cy;

            flags = (uint32_t)(y > work->clip_bottom) << 3 |
                    (uint32_t)(work->clip_top > y) << 2 |
                    (uint32_t)(x > work->clip_right) << 1 |
                    (uint32_t)(work->clip_left > x);
        }
        flags = flags << 2 | (at[2] > _nj_far_clip_ ? CLIP_FAR : 0) |
                (_nj_near_clip_ > at[2] ? CLIP_NEAR : 0);
        all &= flags;
        any |= flags;
    }
    work->clip_and &= all;
    work->clip_or |= any;
}

/* whether _njEasyDrawModel sends nothing for model, by its own tests. it
 * sends nothing for more points than _nj_vertex_num_, or a sphere outside the
 * clips under NJD_CONTROL_3D_MODEL_CLIP. it sends nothing where every point
 * lies at or behind the eye */
static int easy_out(const NJS_MODEL *model)
{
    struct easy_work work = {0};

    if (model->nbPoint > _nj_vertex_num_) {
        return 1;
    }
    if (_nj_control_3d_flag_ & NJD_CONTROL_3D_MODEL_CLIP) {
        NJS_POINT3 center;
        float r = model->r;
        float px = _nj_screen_.dist * port_draw_aspect;
        float py = _nj_screen_.dist * _nj_scr_aspect_y_;
        float inv;

        njCalcPoint(NULL, (NJS_POINT3 *)&model->center, &center);
        if (!(center.z + r > _nj_near_clip_) || center.z - r > _nj_far_clip_) {
            return 1;
        }
        inv = 1.0F / (center.z + r);
        if (px * (center.x - r) * inv + _nj_screen_.cx > _nj_2d_clip_[1].x ||
            px * (center.x + r) * inv + _nj_screen_.cx < _nj_2d_clip_[0].x ||
            py * (center.y - r) * inv + _nj_screen_.cy > _nj_2d_clip_[1].y ||
            py * (center.y + r) * inv + _nj_screen_.cy < _nj_2d_clip_[0].y) {
            return 1;
        }
    }
    work.point_count = model->nbPoint;
    work.points = model->points;
    return easy_calc_body(&work) == model->nbPoint;
}

/* the same for the simple drawer, whose first two tests are the easy one's.
 * its last is a clip flag every point holds, which
 * NJD_CONTROL_3D_NO_CLIP_CHECK skips */
static int simple_out(const NJS_MODEL *model)
{
    float px = _nj_screen_.dist * port_draw_aspect;
    float py = _nj_screen_.dist * _nj_scr_aspect_y_;

    if (model->nbPoint > _nj_vertex_num_) {
        return 1;
    }
    if (_nj_control_3d_flag_ & NJD_CONTROL_3D_MODEL_CLIP) {
        NJS_POINT3 center;
        float r = model->r;
        float inv;

        SimpleCalcModelMatrix(&center, (NJS_POINT3 *)&model->center);
        if (center.z + r <= _nj_near_clip_ || center.z - r > _nj_far_clip_) {
            return 1;
        }
        inv = 1.0F / (center.z + r);
        if (_nj_2d_clip_[0].x > (center.x + r) * px * inv + _nj_screen_.cx ||
            px * (center.x - r) * inv + _nj_screen_.cx > _nj_2d_clip_[1].x ||
            _nj_2d_clip_[0].y > (center.y + r) * py * inv + _nj_screen_.cy ||
            py * (center.y - r) * inv + _nj_screen_.cy > _nj_2d_clip_[1].y) {
            return 1;
        }
    }
    if (_nj_control_3d_flag_ & NJD_CONTROL_3D_NO_CLIP_CHECK) {
        return 0;
    }
    return all_clipped(model, px, py, FALSE);
}

/* whether model's points all hold one clip flag, as clip_flags sets them
 * under the scales px and py. a model of no points does */
static int all_clipped(const NJS_MODEL *model, float px, float py, int full)
{
    struct simple_work work = {0};

    work.count = model->nbPoint;
    work.points = model->points;
    work.clip_and = ~0U;
    work.scale_x = px;
    work.scale_y = py;
    work.clip_left = _nj_2d_clip_[0].x;
    work.clip_right = _nj_2d_clip_[1].x;
    work.clip_top = _nj_2d_clip_[0].y;
    work.clip_bottom = _nj_2d_clip_[1].y;
    clip_flags(&work, full);
    return (work.clip_and & CLIP_ALL) != 0;
}

void loop3_body(struct simple_work *work, NJS_MATRIX *m)
{
    (void)m;
    clip_flags(work, FALSE);
}

void loop4_body(struct simple_work *work, NJS_MATRIX *m)
{
    (void)m;
    clip_flags(work, FALSE);
}

void loop1_body(struct simple_work *work, NJS_MATRIX *m)
{
    (void)work, (void)m;
}

void loop2_body(struct simple_work *work, NJS_MATRIX *m)
{
    (void)work, (void)m;
}

void SimpleCalcModelMatrix(NJS_POINT3 *out, NJS_POINT3 *from)
{
    float in[4] = {from->x, from->y, from->z, 1.0F};
    float at[4];

    sa_ftrv(native_xmtrx(), in, at);
    out->x = at[0];
    out->y = at[1];
    out->z = at[2];
}

/* the light in model space, which only a vertex's color reads */
void SimpleCalcModelLight(NJS_VECTOR *out, NJS_VECTOR *from, float scale)
{
    (void)out, (void)from, (void)scale;
}

void simple_draw_noop(void)
{
}

void save_queue_cursor(float **slot)
{
    (void)slot;
}

void restore_queue_cursor(float **slot)
{
    (void)slot;
}

/* the strip helpers. on the console each writes a vertex and steps the
 * cursor, but here no vertex goes anywhere */
#define NO_VERTEX(name)                                                        \
    void name(void)                                                            \
    {                                                                          \
    }
NO_VERTEX(NJM_SET_POLY_STRIP)
NO_VERTEX(NJM_SET_POLY_X_STRIP)
NO_VERTEX(_NJM_SET_TEX_STRIP)
NO_VERTEX(_NJM_SET_TEX_X_STRIP)
NO_VERTEX(NJM_SET_ENV_STRIP)
NO_VERTEX(NJM_SET_ENV_X_STRIP)
NO_VERTEX(NJM_SET_POLY_STRIP_START)
NO_VERTEX(NJM_SET_POLY_X_STRIP_START)
NO_VERTEX(NJM_SET_TEX_STRIP_START)
NO_VERTEX(NJM_SET_TEX_X_STRIP_START)
NO_VERTEX(NJM_SET_ENV_STRIP_START)
NO_VERTEX(NJM_SET_ENV_X_STRIP_START)
NO_VERTEX(NJM_SET_POLY_STRIP_END)
NO_VERTEX(NJM_SET_POLY_X_STRIP_END)
NO_VERTEX(NJM_SET_TEX_STRIP_END)
NO_VERTEX(NJM_SET_TEX_X_STRIP_END)
NO_VERTEX(NJM_SET_ENV_STRIP_END)
NO_VERTEX(NJM_SET_ENV_X_STRIP_END)
