/* the easy motion draws, which draw each model with njEasyDrawModel. their
 * steps are this file's own. they interpolate the motion and the shape motion
 * linearly, whatever interpolation each names, and blend each angle straight,
 * not the short way round */
#include <ninja.h>
#include <sg_xpt.h>

static void translate_keys(NJS_OBJECT *object);
static void scale_keys(NJS_OBJECT *object);
static void rotate_keys_xyz(NJS_OBJECT *object);
static void rotate_keys_zxy(NJS_OBJECT *object);
static void translate_object(NJS_OBJECT *object);
static void rotate_object_xyz(NJS_OBJECT *object);
static void rotate_object_zxy(NJS_OBJECT *object);
static void scale_object(NJS_OBJECT *object);
static void motion_node_0(NJS_OBJECT *object);
static void motion_node_1(NJS_OBJECT *object);
static void motion_node_2(NJS_OBJECT *object);
static void motion_node_3(NJS_OBJECT *object);
static void motion_node_4(NJS_OBJECT *object);
static void shape_node_0(NJS_OBJECT *object);
static void shape_node_1(NJS_OBJECT *object);
static void shape_node_2(NJS_OBJECT *object);

/* the node steps, by the motion's factor count, and the shape node steps, by
 * what the shape motion moves */
static void (*motion_steps[])(NJS_OBJECT *obj) = {
    motion_node_0, motion_node_1, motion_node_2, motion_node_3, motion_node_4};
static void (*shape_steps[])(NJS_OBJECT *obj) = {shape_node_0, shape_node_1,
                                                 shape_node_1, shape_node_2};
/* the motion's factor count */
static Uint16 motion_count;
/* the motion's type, whose NJD_MTYPE_ bits say which factors it has */
static Uint16 motion_type;
/* what the shape motion moves: SHAPE_POINTS, SHAPE_NORMALS or SHAPE_BOTH */
static Uint16 shape_parts;
/* the shape motion's type, which this file stores and never reads */
static Uint16 shape_type;

/* set up the shape motion at frame, and choose its shape node step by what it
 * moves. with a null shape motion, each node's model is drawn as it is.
 * returns 1, with nothing set up, when frame is not below the shape motion's
 * nbFrame, else 0 */
static int start_easy_shape(NJS_MOTION *shape, Float frame)
{
    struct nj_motion_info *info = &_nj_shape_data_info_;
    Uint16 parts = 0;
    Uint16 type = 0;

    if (shape) {
        if (!(frame < (Float)(Sint32)shape->nbFrame)) {
            return 1;
        }
        info->mdata = shape->mdata;
        info->nbFrame = shape->nbFrame;
        type = shape->type;
    }
    info->frame = frame;
    info->factor = 0;
    info->m = NULL;
    if (type & NJD_MTYPE_VERT_4) {
        parts = SHAPE_POINTS;
    }
    if (type & NJD_MTYPE_NORM_5) {
        parts += SHAPE_NORMALS;
    }
    _nj_shape_data_func_ = shape_steps[parts];
    shape_type = type;
    shape_parts = parts;
    return 0;
}

/* set up the motion at frame, with its factor count and type. a null motion
 * has no factors and no type, so each node is posed by its own pos, ang and
 * scl. returns 1, with nothing set up, when frame is not below the motion's
 * nbFrame, else 0 */
static int start_easy_motion(NJS_MOTION *motion, Float frame)
{
    Uint16 type = 0;
    Uint16 count = 0;

    if (motion) {
        if (!(frame < (Float)(Sint32)motion->nbFrame)) {
            return 1;
        }
        _nj_motion_data_info_.mdata = motion->mdata;
        _nj_motion_data_info_.nbFrame = motion->nbFrame;
        count = motion->inp_fn & ~NJD_MTYPE_MASK;
        type = motion->type;
    }
    _nj_motion_data_info_.frame = frame;
    _nj_motion_data_info_.factor = 0;
    _nj_motion_data_info_.m = NULL;
    motion_count = count;
    motion_type = type;
    return 0;
}

/* choose the node step by the motion's factor count. each of the position,
 * angle and scale steps reads keys where the motion's type has that factor,
 * and the node's own pos, ang or scl otherwise. the angle step turns every
 * node in the order object's NJD_EVAL_ZXY_ANG gives */
static void select_easy_steps(NJS_OBJECT *object)
{
    Uint16 type = motion_type;
    Uint32 zxy;
    void (*step)(NJS_OBJECT *obj);

    _nj_motion_data_func_ = motion_steps[motion_count];
    step = translate_object;
    if (type & NJD_MTYPE_POS_0) {
        step = translate_keys;
    }
    _nj_motion_translate_ = step;
    zxy = object->evalflags & NJD_EVAL_ZXY_ANG;
    if (type & NJD_MTYPE_ANG_1) {
        step = rotate_keys_xyz;
        if (zxy) {
            step = rotate_keys_zxy;
        }
    } else {
        step = rotate_object_xyz;
        if (zxy) {
            step = rotate_object_zxy;
        }
    }
    _nj_motion_rotate_ = step;
    step = scale_object;
    if (type & NJD_MTYPE_SCL_2) {
        step = scale_keys;
    }
    _nj_motion_scale_ = step;
}

/* set *index to the last of count keys whose keyframe is at most the motion's
 * frame. the frame is truncated and compared unsigned. *index is 0 where no
 * keyframe after the first is at most the frame. the keys must be in frame
 * order, and count at least 1. keys holds NJS_MKEY_F or NJS_MKEY_A, both 16
 * bytes with the keyframe first */
static void find_vector_key(void *keys, Uint32 count, Uint32 *index)
{
    Uint32 low = 0;
    Uint32 frame = (Sint32)_nj_motion_data_info_.frame;
    Uint32 middle;
    Uint32 key;

    while (count - low - 1 > 0) {
        middle = (count + low) >> 1;
        key = ((const NJS_MKEY_F *)keys)[middle].keyframe;
        if (frame >= key) {
            low = middle;
        } else {
            count = middle;
        }
    }
    *index = low;
}

/* find_vector_key for shape keys, at the shape motion's frame */
static void find_shape_key(NJS_MKEY_P *keys, Uint32 count, Uint32 *index)
{
    Uint32 low = 0;
    Uint32 frame = (Sint32)_nj_shape_data_info_.frame;
    Uint32 middle;
    Uint32 key;

    while (count - low - 1 > 0) {
        middle = (count + low) >> 1;
        key = keys[middle].keyframe;
        if (frame >= key) {
            low = middle;
        } else {
            count = middle;
        }
    }
    *index = low;
}

/* interpolate key index's three values linearly toward the next key's, into
 * out. the key after the last is the first. the weight t is the motion's frame
 * less the key's keyframe. where span, the next keyframe less the key's, is
 * above 0, t is then multiplied by an approximate 1 / span: FSRRA's
 * 1 / sqrt(span), squared */
static void sample_vector(NJS_MKEY_F *keys, Uint32 index, Uint32 count,
                          Float *out)
{
    NJS_MKEY_F *key = &keys[index];
    NJS_MKEY_F *next = keys;
    Float *a;
    Float *b;
    Float t;
    Float rest;
    Float inverse;
    Float x;
    Float y;
    Float z;
    Float term;
    Sint32 span;

    if (index + 1 < count) {
        next = key + 1;
    }
    /* @hack type: the keyframe read as a Sint32, not converted to one, so the
     * span subtracts straight into R5, SUB R3,R5 at 0x8c61593c; converted, as
     * (Float)(Sint32)key->keyframe, it subtracts into R2 and copies it to R5,
     * the function is 2 bytes longer, and 34 of 49 words differ */
    t = _nj_motion_data_info_.frame - (Float)(*(const Sint32 *)&key->keyframe);
    span = next->keyframe - key->keyframe;
    if (span > 0) {
        inverse = (Float)span;
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        t *= inverse;
    }
    rest = 1.0F;
    rest -= t;
    a = key->key;
    b = next->key;
    out = out + 3;
    x = *a++;
    term = *b++;
    x *= rest;
    term *= t;
    x += term;
    y = *a++;
    term = *b++;
    y *= rest;
    term *= t;
    y += term;
    z = *a;
    z *= rest;
    term = *b;
    term *= t;
    z += term;
    *--out = z;
    *--out = y;
    *--out = x;
}

/* sample_vector for angles, each blended as a Float and truncated */
static void sample_angles(NJS_MKEY_A *keys, Uint32 index, Uint32 count,
                          Angle *out)
{
    NJS_MKEY_A *key = &keys[index];
    NJS_MKEY_A *next = keys;
    Float t;
    Float rest;
    Float inverse;
    Sint32 span;

    if (index + 1 < count) {
        next = key + 1;
    }
    /* @hack type: the keyframe read as a Sint32, not converted to one, so the
     * span subtracts straight into R5, SUB R3,R5 at 0x8c61599e; converted, as
     * (Float)(Sint32)key->keyframe, it subtracts into R2 and copies it to R5,
     * the code fills the pad before the pool, and 49 of 65 words differ */
    t = _nj_motion_data_info_.frame - (Float)(*(const Sint32 *)&key->keyframe);
    span = next->keyframe - key->keyframe;
    if (span > 0) {
        inverse = (Float)span;
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        t *= inverse;
    }
    rest = 1.0F - t;
    out[0] = (Angle)(((Float)key->key[0] * rest) + ((Float)next->key[0] * t));
    out[1] = (Angle)(((Float)key->key[1] * rest) + ((Float)next->key[1] * t));
    out[2] = (Angle)(((Float)key->key[2] * rest) + ((Float)next->key[2] * t));
}

/* translate_keys, scale_keys, rotate_keys_xyz and rotate_keys_zxy, the keyed
 * steps. each reads the motion's next factor for the node: it moves the
 * current matrix by the factor's keys at the motion's frame, or by the node's
 * own pos, ang or scl where the factor has no keys. it then moves
 * _nj_motion_data_info_.factor past the factor */
static void translate_keys(NJS_OBJECT *object)
{
    struct nj_motion_info *info = &_nj_motion_data_info_;
    Uint32 factor = info->factor;
    NJS_MKEY_F *keys = info->p[factor];
    Float *pos = object->pos;
    Uint32 index;
    Float value[3];
    Float x;
    Float y;
    Float z;

    if (keys) {
        Uint32 count = info->nb[factor];
        find_vector_key(keys, count, &index);
        sample_vector(keys, index, count, value);
        pos = value;
    }
    x = *pos++;
    y = *pos++;
    z = *pos;
    njTranslate(NULL, x, y, z);
    info->factor = factor + 1;
}

static void scale_keys(NJS_OBJECT *object)
{
    struct nj_motion_info *info = &_nj_motion_data_info_;
    Uint32 factor = info->factor;
    NJS_MKEY_F *keys = info->p[factor];
    Float *scl = object->scl;
    Uint32 index;
    Float value[3];
    Float x;
    Float y;
    Float z;

    if (keys) {
        Uint32 count = info->nb[factor];
        find_vector_key(keys, count, &index);
        sample_vector(keys, index, count, value);
        scl = value;
    }
    x = *scl++;
    y = *scl++;
    z = *scl;
    njScale(NULL, x, y, z);
    info->factor = factor + 1;
}

static void rotate_keys_xyz(NJS_OBJECT *object)
{
    struct nj_motion_info *info = &_nj_motion_data_info_;
    Uint32 factor = info->factor;
    NJS_MKEY_A *keys = info->p[factor];
    Angle *ang = object->ang;
    Uint32 index;
    Angle value[3];
    Angle x;
    Angle y;
    Angle z;

    if (keys) {
        Uint32 count = info->nb[factor];
        find_vector_key(keys, count, &index);
        sample_angles(keys, index, count, value);
        ang = value;
    }
    x = ang[0];
    y = ang[1];
    z = ang[2];
    njRotateXYZ(NULL, x, y, z);
    info->factor = factor + 1;
}

static void rotate_keys_zxy(NJS_OBJECT *object)
{
    struct nj_motion_info *info = &_nj_motion_data_info_;
    Uint32 factor = info->factor;
    NJS_MKEY_A *keys = info->p[factor];
    Angle *ang = object->ang;
    Uint32 index;
    Angle value[3];
    Angle x;
    Angle y;
    Angle z;

    if (keys) {
        Uint32 count = info->nb[factor];
        find_vector_key(keys, count, &index);
        sample_angles(keys, index, count, value);
        ang = value;
    }
    x = ang[0];
    y = ang[1];
    z = ang[2];
    njRotateZXY(NULL, x, y, z);
    info->factor = factor + 1;
}

/* set pair[0] and pair[1] to the points or normals of key index and the next
 * key. the key after the last is the first. returns t, the weight of the next
 * key, as sample_vector forms it at the shape motion's frame */
static Float sample_shape(NJS_MKEY_P *keys, Uint32 index, Uint32 count,
                          NJS_POINT3 **pair)
{
    NJS_MKEY_P *key = &keys[index];
    NJS_MKEY_P *next = keys;
    Float t;
    Float inverse;
    Sint32 span;

    if (index + 1 < count) {
        next = key + 1;
    }
    /* @hack type: the keyframe read as a Sint32, not converted to one, so the
     * span subtracts straight into R5, SUB R3,R5 at 0x8c615be2; converted, as
     * (Float)(Sint32)key->keyframe, it subtracts into R2 and copies it to R5,
     * the function is 2 bytes longer, and 20 of 31 words differ */
    t = _nj_shape_data_info_.frame - (Float)(*(const Sint32 *)&key->keyframe);
    span = next->keyframe - key->keyframe;
    if (span > 0) {
        inverse = (Float)span;
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        t *= inverse;
    }
    pair += 2;
    *--pair = next->key;
    *--pair = key->key;
    return t;
}

/* NOLINTNEXTLINE(clang-diagnostic-undefined-internal) */
static Float squared_length(Float x, Float y, Float z, Float w);

/* clang-format off */
#ifndef __clang__
/* returns x * x + y * y + z * z + w * w, by one FIPR. SHC expands the body at
 * each call. size=4 is its length in bytes. without it, SHC can write a
 * literal pool ahead of the expansion, behind a BRA that skips the pool, and
 * the use-assembler-pools patch refuses a pool so guarded */
/* @asm operand: fipr-registers, FIPR FV4,FV4 at 0x8c615c36 squares the blended
 * normal in the registers it is in */
#pragma inline_asm(squared_length(size=4))
static Float squared_length(Float x, Float y, Float z, Float w)
{
        FIPR        FV4,FV4
        FMOV.S      FR7,FR0
}
#endif
/* clang-format on */

/* blend a and b, t of the way to b, into out. out is scaled to unit length by
 * FSRRA's approximate 1 / sqrt. a blend whose squared length is not above
 * 0.00001 becomes (0, 0, 1) */
static void blend_normal(Float *a, Float *b, Float *out, Float t)
{
    Float rest;
    Float x;
    Float y;
    Float z;
    Float term;
    Float length;

    rest = 1.0F - t;
    /* @hack spelling: Float pointers walk the a, b and out points' x, y and z,
     * which gives the FMOV.S @R4+ and @R5+ loads (0x8c615c12 to 0x8c615c2a) and
     * the FMOV.S @-R14 stores (0x8c615c56 to 0x8c615c5a); with NJS_POINT3
     * parameters read and stored by member, 42 of 50 words differ and the
     * section is 4 bytes longer */
    x = *a++;
    x *= rest;
    term = *b++;
    term *= t;
    x += term;
    y = *a++;
    y *= rest;
    term = *b++;
    term *= t;
    y += term;
    z = *a;
    z *= rest;
    term = *b;
    term *= t;
    z += term;
    length = squared_length(x, y, z, 0.0F);
    if (length > 0.00001F) {
        length = _builtin_fsrra(length);
        x *= length;
        y *= length;
        z *= length;
    } else {
        x = 0.0F;
        y = 0.0F;
        z = 1.0F;
    }
    out += 3;
    *--out = z;
    *--out = y;
    *--out = x;
}

/* blend a and b, t of the way to b, into out */
static void blend_point(Float *a, Float *b, Float *out, Float t)
{
    Float rest;
    Float x;
    Float y;
    Float z;
    Float term;

    /* @hack spelling: Float pointers walk the a, b and out points' x, y and z,
     * which gives the FMOV.S @R4+ and @R5+ loads (0x8c615c66 to 0x8c615c84) and
     * the FMOV.S @-R6 stores (0x8c615c8a to 0x8c615c90); with NJS_POINT3
     * parameters read and stored by member, 16 of 22 words differ and the
     * section is 4 bytes longer */
    out += 3;
    rest = 1.0F - t;
    x = *a++;
    x *= rest;
    term = *b++;
    term *= t;
    x += term;
    y = *a++;
    y *= rest;
    term = *b++;
    term *= t;
    y += term;
    z = *a;
    z *= rest;
    term = *b;
    term *= t;
    z += term;
    *--out = z;
    *--out = y;
    *--out = x;
}

/* where the shape motion's current factor has keys, fill pair and *t as
 * sample_shape does, and add SHAPE_POINTS to *found */
static void get_shape_points(Sint32 *found, Float *t, NJS_POINT3 **pair)
{
    struct nj_motion_info *info = &_nj_shape_data_info_;
    Uint32 factor = info->factor;
    NJS_MKEY_P *keys = info->p[factor];
    Uint32 index;

    if (keys) {
        Uint32 count = info->nb[factor];
        find_shape_key(keys, count, &index);
        *t = sample_shape(keys, index, count, pair);
        *found += SHAPE_POINTS;
    }
}

/* get_shape_points for normals, which adds SHAPE_NORMALS to *found */
static void get_shape_normals(Sint32 *found, Float *t, NJS_POINT3 **pair)
{
    struct nj_motion_info *info = &_nj_shape_data_info_;
    Uint32 factor = info->factor;
    NJS_MKEY_P *keys = info->p[factor];
    Uint32 index;

    if (keys) {
        Uint32 count = info->nb[factor];
        find_shape_key(keys, count, &index);
        *t = sample_shape(keys, index, count, pair);
        *found += SHAPE_NORMALS;
    }
}

/* set the first count points of _nj_shape_buf_ by blend_point, each from the
 * same point of pair[0] and pair[1] at t, and set model->points to them. count
 * must be at least 1 */
static void morph_points(NJS_MODEL *model, Sint32 count, NJS_POINT3 **pair,
                         Float t)
{
    NJS_POINT3 *out = (NJS_POINT3 *)_nj_shape_buf_;
    NJS_POINT3 *buffer = out;
    NJS_POINT3 *a = pair[0];
    NJS_POINT3 *b = pair[1];
    Sint32 left = count;

    do {
        blend_point(&a->x, &b->x, &out->x, t);
        a++;
        b++;
        out++;
    } while (--left != 0);
    model->points = buffer;
}

/* morph_points for normals, each set by blend_normal, with model->normals set
 * to them */
static void morph_normals(NJS_MODEL *model, Sint32 count, NJS_POINT3 **pair,
                          Float t)
{
    NJS_POINT3 *out = (NJS_POINT3 *)_nj_shape_buf_;
    NJS_POINT3 *buffer = out;
    NJS_POINT3 *a = pair[0];
    NJS_POINT3 *b = pair[1];
    Sint32 left = count;

    do {
        blend_normal(&a->x, &b->x, &out->x, t);
        a++;
        b++;
        out++;
    } while (--left != 0);
    model->normals = buffer;
}

/* morph_normals, but into the count points of _nj_shape_buf_ that follow those
 * morph_points sets */
static void morph_normals_after_points(NJS_MODEL *model, Sint32 count,
                                       NJS_POINT3 **pair, Float t)
{
    /* @hack spelling: the float index walks the buffer's NJS_POINT3 members to
     * the normals after count points, which loads the buffer pointer into R0
     * (MOV.L
     * @R2,R0 at 0x8c615dfa); as (NJS_POINT3 *)_nj_shape_buf_ + count, 3 of 40
     * words differ */
    NJS_POINT3 *out = (NJS_POINT3 *)&_nj_shape_buf_[count * 3];
    NJS_POINT3 *buffer = out;
    NJS_POINT3 *a = pair[0];
    NJS_POINT3 *b = pair[1];
    Sint32 left = count;

    do {
        blend_normal(&a->x, &b->x, &out->x, t);
        a++;
        b++;
        out++;
    } while (--left != 0);
    model->normals = buffer;
}

/* the shape step, which draws the node's model with njEasyDrawModel. where the
 * shape motion has keys for the model's points, normals or both, it morphs them
 * into _nj_shape_buf_ for the draw, the normals after the points where both
 * are morphed. it then restores the model's own points and normals. a null
 * model, or one under NJD_EVAL_HIDE, is left alone */
static void draw_morphed_model(NJS_OBJECT *object)
{
    NJS_MODEL *model = object->model;
    NJS_POINT3 *point_pair[2];
    NJS_POINT3 *normal_pair[2];
    NJS_POINT3 *points;
    NJS_POINT3 *normals;
    Float point_t;
    Float normal_t;
    Sint32 found;
    Sint32 count;

    if (!model || (object->evalflags & NJD_EVAL_HIDE)) {
        return;
    }
    points = model->points;
    normals = model->normals;
    count = model->nbPoint;
    found = 0;
    switch (shape_parts) {
    case SHAPE_POINTS:
        get_shape_points(&found, &point_t, point_pair);
        break;
    case SHAPE_NORMALS:
        get_shape_normals(&found, &normal_t, normal_pair);
        break;
    case SHAPE_BOTH:
        get_shape_points(&found, &point_t, point_pair);
        _nj_shape_data_info_.factor++;
        get_shape_normals(&found, &normal_t, normal_pair);
        break;
    }
    switch (found) {
    case SHAPE_POINTS:
        morph_points(model, count, point_pair, point_t);
        break;
    case SHAPE_NORMALS:
        morph_normals(model, count, normal_pair, normal_t);
        break;
    case SHAPE_BOTH:
        morph_points(model, count, point_pair, point_t);
        morph_normals_after_points(model, count, normal_pair, normal_t);
        break;
    }
    njEasyDrawModel(model);
    model->points = points;
    model->normals = normals;
}

/* translate_object, rotate_object_xyz, rotate_object_zxy and scale_object,
 * the steps that move the current matrix by the node's own pos, ang or scl,
 * for a factor the motion's type lacks */
static void translate_object(NJS_OBJECT *object)
{
    Float *pos = object->pos;
    Float x;
    Float y;
    Float z;

    x = *pos++;
    y = *pos++;
    z = *pos;
    njTranslate(NULL, x, y, z);
}

static void rotate_object_xyz(NJS_OBJECT *object)
{
    Angle *ang = object->ang;
    Angle x;
    Angle y;
    Angle z;

    x = ang[0];
    y = ang[1];
    z = ang[2];
    njRotateXYZ(NULL, x, y, z);
}

static void rotate_object_zxy(NJS_OBJECT *object)
{
    Angle *ang = object->ang;
    Angle x;
    Angle y;
    Angle z;

    x = ang[0];
    y = ang[1];
    z = ang[2];
    njRotateZXY(NULL, x, y, z);
}

static void scale_object(NJS_OBJECT *object)
{
    Float *scl = object->scl;
    Float x;
    Float y;
    Float z;

    x = *scl++;
    y = *scl++;
    z = *scl;
    njScale(NULL, x, y, z);
}

/* motion_node_0 to motion_node_4, the node steps, by the motion's factor
 * count. each runs the position, angle and scale steps on the node. with a
 * count above 0, a step first points the motion at the node's record, an
 * NJS_MDATA1 to NJS_MDATA4 by the count, from its first factor, and after the
 * three steps moves the motion's mdata to the next node's record */
static void motion_node_0(NJS_OBJECT *object)
{
    _nj_motion_translate_(object);
    _nj_motion_rotate_(object);
    _nj_motion_scale_(object);
}

static void motion_node_1(NJS_OBJECT *object)
{
    struct nj_motion_info *info = &_nj_motion_data_info_;
    NJS_MDATA1 *record = info->mdata;

    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    _nj_motion_translate_(object);
    _nj_motion_rotate_(object);
    _nj_motion_scale_(object);
    info->mdata = record + 1;
}

static void motion_node_2(NJS_OBJECT *object)
{
    struct nj_motion_info *info = &_nj_motion_data_info_;
    NJS_MDATA2 *record = info->mdata;

    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    _nj_motion_translate_(object);
    _nj_motion_rotate_(object);
    _nj_motion_scale_(object);
    info->mdata = record + 1;
}

static void motion_node_3(NJS_OBJECT *object)
{
    struct nj_motion_info *info = &_nj_motion_data_info_;
    NJS_MDATA3 *record = info->mdata;

    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    _nj_motion_translate_(object);
    _nj_motion_rotate_(object);
    _nj_motion_scale_(object);
    info->mdata = record + 1;
}

static void motion_node_4(NJS_OBJECT *object)
{
    struct nj_motion_info *info = &_nj_motion_data_info_;
    NJS_MDATA4 *record = info->mdata;

    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    _nj_motion_translate_(object);
    _nj_motion_rotate_(object);
    _nj_motion_scale_(object);
    info->mdata = record + 1;
}

/* shape_node_0 to shape_node_2, the shape node steps, by what the shape
 * motion moves. with neither points nor normals, the step draws the node's
 * model as it is, with njEasyDrawModel, unless the model is null or under
 * NJD_EVAL_HIDE. otherwise a step points the shape motion at the node's
 * record, an NJS_MDATA1 for one part or an NJS_MDATA2 for both, from its first
 * factor. it runs draw_morphed_model, then moves the shape motion's mdata to
 * the next node's record */
static void shape_node_0(NJS_OBJECT *object)
{
    if (object->model && !(object->evalflags & NJD_EVAL_HIDE)) {
        njEasyDrawModel(object->model);
    }
}

static void shape_node_1(NJS_OBJECT *object)
{
    struct nj_motion_info *info = &_nj_shape_data_info_;
    NJS_MDATA1 *record = info->mdata;

    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    draw_morphed_model(object);
    info->mdata = record + 1;
}

static void shape_node_2(NJS_OBJECT *object)
{
    struct nj_motion_info *info = &_nj_shape_data_info_;
    NJS_MDATA2 *record = info->mdata;

    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    draw_morphed_model(object);
    info->mdata = record + 1;
}

static void draw_easy_motion_tree(NJS_OBJECT *object)
{
    do {
        Uint32 flags = object->evalflags;

        njPushMatrix(NULL);
        if (!(flags & NJD_EVAL_SKIP)) {
            _nj_motion_data_func_(object);
        } else {
            void (*rotate)(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz);
            Float *pos = object->pos;
            Float *scl;
            Angle *angles;
            Float x;
            Float y;
            Float z;
            Angle angx;
            Angle angy;
            Angle angz;

            x = *pos++;
            y = *pos++;
            z = *pos;
            njTranslate(NULL, x, y, z);
            rotate = njRotateXYZ;
            if (flags & NJD_EVAL_ZXY_ANG) {
                rotate = njRotateZXY;
            }
            /* @bug memory: nothing sets angles, so a node under
             * NJD_EVAL_SKIP turns by the three words at the stack pointer, read
             * at 0x8c61614c, 0x8c61614e and 0x8c616150: angx, angy and angz
             * take the PR, R8 and R9 the function saved on entry */
            /* NOLINTBEGIN(clang-diagnostic-uninitialized) */
            angx = angles[0];
            angy = angles[1];
            angz = angles[2];
            /* NOLINTEND(clang-diagnostic-uninitialized) */
            rotate(NULL, angx, angy, angz);
            scl = object->scl;
            x = *scl++;
            y = *scl++;
            z = *scl;
            njScale(NULL, x, y, z);
        }
        if (_nj_motion_callback_) {
            _nj_motion_callback_(object);
        }
        if (object->model && !(flags & NJD_EVAL_HIDE)) {
            njEasyDrawModel(object->model);
        }
        if (object->child) {
            draw_easy_motion_tree(object->child);
        }
        njPopMatrix(1);
        object = object->sibling;
    } while (object);
}

static void draw_easy_shape_tree(NJS_OBJECT *object)
{
    do {
        Uint32 flags = object->evalflags;

        njPushMatrix(NULL);
        if (!(flags & NJD_EVAL_SKIP)) {
            _nj_motion_data_func_(object);
        } else {
            void (*rotate)(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz);
            Float *pos = object->pos;
            Float *scl;
            Angle *angles;
            Float x;
            Float y;
            Float z;
            Angle angx;
            Angle angy;
            Angle angz;

            x = *pos++;
            y = *pos++;
            z = *pos;
            njTranslate(NULL, x, y, z);
            rotate = njRotateXYZ;
            if (flags & NJD_EVAL_ZXY_ANG) {
                rotate = njRotateZXY;
            }
            angles = object->ang;
            angx = angles[0];
            angy = angles[1];
            angz = angles[2];
            rotate(NULL, angx, angy, angz);
            scl = object->scl;
            x = *scl++;
            y = *scl++;
            z = *scl;
            njScale(NULL, x, y, z);
        }
        if (_nj_motion_callback_) {
            _nj_motion_callback_(object);
        }
        if (!(flags & NJD_EVAL_SHAPE_SKIP)) {
            _nj_shape_data_func_(object);
        } else if (object->model && !(flags & NJD_EVAL_HIDE)) {
            njEasyDrawModel(object->model);
        }
        if (object->child) {
            draw_easy_shape_tree(object->child);
        }
        njPopMatrix(1);
        object = object->sibling;
    } while (object);
}

void njEasyAction(NJS_ACTION *action, Float frame)
{
    NJS_MOTION *motion = action->motion;
    NJS_OBJECT *object = action->object;

    if (start_easy_motion(motion, frame) == 0) {
        select_easy_steps(object);
        njGetkmPointer();
        draw_easy_motion_tree(object);
        njSetkmPointer();
    }
}

void njEasyDrawMotion(NJS_OBJECT *object, NJS_MOTION *motion, Float frame)
{
    if (start_easy_motion(motion, frame) == 0) {
        select_easy_steps(object);
        njGetkmPointer();
        draw_easy_motion_tree(object);
        njSetkmPointer();
    }
}

void njEasyDrawShapeMotion(NJS_OBJECT *object, NJS_MOTION *motion,
                           NJS_MOTION *shape, Float frame)
{
    if (start_easy_motion(motion, frame) == 0 &&
        start_easy_shape(shape, frame) == 0) {
        select_easy_steps(object);
        njGetkmPointer();
        draw_easy_shape_tree(object);
        njSetkmPointer();
    }
}
