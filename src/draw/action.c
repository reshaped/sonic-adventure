/* the game's own copy of ninja's easy motion draws, each model drawn with
 * draw_model_nested */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/model.h"

static void action_translate_keys(NJS_OBJECT *object);
static void action_scale_keys(NJS_OBJECT *object);
static void action_rotate_keys_xyz(NJS_OBJECT *object);
static void action_rotate_keys_zxy(NJS_OBJECT *object);
static void action_translate_object(NJS_OBJECT *object);
static void action_rotate_object_xyz(NJS_OBJECT *object);
static void action_rotate_object_zxy(NJS_OBJECT *object);
static void action_scale_object(NJS_OBJECT *object);
static void action_motion_node_0(NJS_OBJECT *object);
static void action_motion_node_1(NJS_OBJECT *object);
static void action_motion_node_2(NJS_OBJECT *object);
static void action_motion_node_3(NJS_OBJECT *object);
static void action_motion_node_4(NJS_OBJECT *object);
static void action_shape_node_0(NJS_OBJECT *object);
static void action_shape_node_1(NJS_OBJECT *object);
static void action_shape_node_2(NJS_OBJECT *object);

/* the motion's node steps, by its factor count */
static void (*action_motion_steps[])(NJS_OBJECT *obj) = {
    action_motion_node_0, action_motion_node_1, action_motion_node_2,
    action_motion_node_3, action_motion_node_4};
/* the shape motion's node steps, by what it morphs: nothing, points, normals
 * or both */
static void (*action_shape_steps[])(NJS_OBJECT *obj) = {
    action_shape_node_0, action_shape_node_1, action_shape_node_1,
    action_shape_node_2};
/* the motion's factor count and type */
static Uint16 action_factor_count;
static Uint16 action_motion_type;
/* what the shape motion morphs, as SHAPE_POINTS and SHAPE_NORMALS, and its
 * type */
static Uint16 action_shape_morphs;
static Uint16 action_shape_type;

/* start the shape motion at frame: set _nj_shape_data_info_, the node step
 * and what it morphs. it returns 1, setting nothing, when frame is not below
 * the shape's nbFrame, and 0 otherwise. a null shape takes the step that
 * draws each model as it is */
static int action_start_shape(NJS_MOTION *shape, Float frame)
{
    struct nj_motion_info *info = &_nj_shape_data_info_;
    Uint16 morphs = 0;
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
        morphs = SHAPE_POINTS;
    }
    if (type & NJD_MTYPE_NORM_5) {
        morphs += SHAPE_NORMALS;
    }
    _nj_shape_data_func_ = action_shape_steps[morphs];
    action_shape_type = type;
    action_shape_morphs = morphs;
    return 0;
}

/* start the motion at frame: set _nj_motion_data_info_, the factor count and
 * the type. it returns 1, setting nothing, when frame is not below the
 * motion's nbFrame, and 0 otherwise. a null motion has no factors and no type,
 * so each node keeps its own pose */
static int action_start_motion(NJS_MOTION *motion, Float frame)
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
    action_factor_count = count;
    action_motion_type = type;
    return 0;
}

/* set ninja's motion steps for the motion just started. the node step goes by
 * the factor count. position, angle and scale each take the keyed step where
 * the type has that factor, and the node's own value where it hasn't.
 * object's NJD_EVAL_ZXY_ANG picks the rotation order for every node */
static void action_select_steps(NJS_OBJECT *object)
{
    Uint16 type = action_motion_type;
    Uint32 zxy;
    void (*step)(NJS_OBJECT *obj);

    _nj_motion_data_func_ = action_motion_steps[action_factor_count];
    step = action_translate_object;
    if (type & NJD_MTYPE_POS_0) {
        step = action_translate_keys;
    }
    _nj_motion_translate_ = step;
    zxy = object->evalflags & NJD_EVAL_ZXY_ANG;
    if (type & NJD_MTYPE_ANG_1) {
        step = action_rotate_keys_xyz;
        if (zxy) {
            step = action_rotate_keys_zxy;
        }
    } else {
        step = action_rotate_object_xyz;
        if (zxy) {
            step = action_rotate_object_zxy;
        }
    }
    _nj_motion_rotate_ = step;
    step = action_scale_object;
    if (type & NJD_MTYPE_SCL_2) {
        step = action_scale_keys;
    }
    _nj_motion_scale_ = step;
}

/* set *index to the last of the count keys whose keyframe is at most the
 * motion's frame, or to 0 where none is. the keys must be in frame order, and
 * count at least 1. the frame is truncated to an integer and compared
 * unsigned. keys are NJS_MKEY_F or NJS_MKEY_A, which both lead with the
 * keyframe and are 16 bytes long */
static void action_find_key(void *keys, Uint32 count, Uint32 *index)
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

/* action_find_key for shape keys, at the shape motion's frame */
static void action_find_shape_key(NJS_MKEY_P *keys, Uint32 count, Uint32 *index)
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

/* blend key index's three values toward the next key's, into out. the key
 * after the last is the first. the weight is the motion's frame less key
 * index's keyframe, divided by the span to the next keyframe where that span
 * is positive */
static void action_sample_vector(NJS_MKEY_F *keys, Uint32 index, Uint32 count,
                                 Float *out)
{
    NJS_MKEY_F *key = &keys[index];
    NJS_MKEY_F *next = keys;
    Float *from;
    Float *to;
    Float weight;
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
     * span subtracts straight into R5, SUB R3,R5 at 0x8c040f6c; converted, as
     * (Float)(Sint32)key->keyframe, it subtracts into R2 and copies it to R5,
     * 34 of 50 words differ and the function is 2 bytes longer */
    weight =
        _nj_motion_data_info_.frame - (Float)(*(const Sint32 *)&key->keyframe);
    span = next->keyframe - key->keyframe;
    if (span > 0) {
        /* inverse becomes 1 / span, to FSRRA's precision, as the square of
         * its 1 / sqrt(span) */
        inverse = (Float)span;
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        weight *= inverse;
    }
    /* @hack spelling: rest set to 1 before weight is taken from it, which gives
     * x, y and z FR8, FR6 and FR7 (0x8c040f88, 0x8c040f96, 0x8c040f9e); set
     * as 1.0F - weight, they take FR6, FR7 and FR8, and 16 of 50 words
     * differ */
    rest = 1.0F;
    rest -= weight;
    from = key->key;
    to = next->key;
    out = out + 3;
    x = *from++;
    term = *to++;
    x *= rest;
    term *= weight;
    x += term;
    y = *from++;
    term = *to++;
    y *= rest;
    term *= weight;
    y += term;
    z = *from;
    z *= rest;
    term = *to;
    term *= weight;
    z += term;
    *--out = z;
    *--out = y;
    *--out = x;
}

/* action_sample_vector for angles, each blended as a Float */
static void action_sample_angles(NJS_MKEY_A *keys, Uint32 index, Uint32 count,
                                 Angle *out)
{
    NJS_MKEY_A *key = &keys[index];
    NJS_MKEY_A *next = keys;
    Float weight;
    Float rest;
    Float inverse;
    Sint32 span;

    if (index + 1 < count) {
        next = key + 1;
    }
    /* @hack type: the keyframe read as a Sint32, not converted to one, so the
     * span subtracts straight into R5, SUB R3,R5 at 0x8c040fd0; converted, as
     * (Float)(Sint32)key->keyframe, it subtracts into R2 and copies it to R5,
     * 90 of 114 words differ and the function is 4 bytes longer */
    weight =
        _nj_motion_data_info_.frame - (Float)(*(const Sint32 *)&key->keyframe);
    span = next->keyframe - key->keyframe;
    if (span > 0) {
        inverse = (Float)span;
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        weight *= inverse;
    }
    rest = 1.0F - weight;
    out[0] =
        (Angle)(((Float)key->key[0] * rest) + ((Float)next->key[0] * weight));
    out[1] =
        (Angle)(((Float)key->key[1] * rest) + ((Float)next->key[1] * weight));
    out[2] =
        (Angle)(((Float)key->key[2] * rest) + ((Float)next->key[2] * weight));
}

/* the keyed steps. each moves the current matrix by the node's current
 * factor, sampled at the motion's frame, then advances the factor. a factor
 * without keys moves it by the node's own value */
static void action_translate_keys(NJS_OBJECT *object)
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
        action_find_key(keys, count, &index);
        action_sample_vector(keys, index, count, value);
        pos = value;
    }
    x = *pos++;
    y = *pos++;
    z = *pos;
    njTranslate(NULL, x, y, z);
    info->factor = factor + 1;
}

static void action_scale_keys(NJS_OBJECT *object)
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
        action_find_key(keys, count, &index);
        action_sample_vector(keys, index, count, value);
        scl = value;
    }
    x = *scl++;
    y = *scl++;
    z = *scl;
    njScale(NULL, x, y, z);
    info->factor = factor + 1;
}

static void action_rotate_keys_xyz(NJS_OBJECT *object)
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
        action_find_key(keys, count, &index);
        action_sample_angles(keys, index, count, value);
        ang = value;
    }
    x = ang[0];
    y = ang[1];
    z = ang[2];
    njRotateXYZ(NULL, x, y, z);
    info->factor = factor + 1;
}

static void action_rotate_keys_zxy(NJS_OBJECT *object)
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
        action_find_key(keys, count, &index);
        action_sample_angles(keys, index, count, value);
        ang = value;
    }
    x = ang[0];
    y = ang[1];
    z = ang[2];
    njRotateZXY(NULL, x, y, z);
    info->factor = factor + 1;
}

/* put key index's data and the next key's into pair, taking the first key
 * after the last. it returns the weight of the next at the shape motion's
 * frame, as action_sample_vector weighs it */
static Float action_sample_shape(NJS_MKEY_P *keys, Uint32 index, Uint32 count,
                                 NJS_POINT3 **pair)
{
    NJS_MKEY_P *key = &keys[index];
    NJS_MKEY_P *next = keys;
    Float weight;
    Float inverse;
    Sint32 span;

    if (index + 1 < count) {
        next = key + 1;
    }
    /* @hack type: the keyframe read as a Sint32, not converted to one, so the
     * span subtracts straight into R5, SUB R3,R5 at 0x8c041214; converted, as
     * (Float)(Sint32)key->keyframe, it subtracts into R2 and copies it to R5,
     * 20 of 32 words differ and the function is 2 bytes longer */
    weight =
        _nj_shape_data_info_.frame - (Float)(*(const Sint32 *)&key->keyframe);
    span = next->keyframe - key->keyframe;
    if (span > 0) {
        inverse = (Float)span;
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        weight *= inverse;
    }
    pair += 2;
    *--pair = next->key;
    *--pair = key->key;
    return weight;
}

/* NOLINTNEXTLINE(clang-diagnostic-undefined-internal) */
static Float squared_length(Float x, Float y, Float z, Float fourth);

/* clang-format off */
#ifndef __clang__
/* the squared length of (x, y, z, fourth), in FR0. SHC expands the body at
 * the call. size=4 gives the body's length in bytes. without it, SHC 5.0
 * R10 can place a literal pool ahead of the expansion, with a BRA over the
 * pool. shckit's use-assembler-pools patch refuses such a pool */
/* @asm operand: fipr-registers, FIPR FV4,FV4 at 0x8c041268 squares the blended
 * normal in the registers it is in */
#pragma inline_asm(squared_length(size=4))
static Float squared_length(Float x, Float y, Float z, Float fourth)
{
        FIPR        FV4,FV4
        FMOV.S      FR7,FR0
}
#endif
/* clang-format on */

/* blend first toward second by weight into out, and scale the blend to unit
 * length, to FSRRA's precision. a blend whose squared length is not above
 * 0.00001 becomes (0, 0, 1) */
static void action_blend_normal(Float *first, Float *second, Float *out,
                                Float weight)
{
    Float rest;
    Float x;
    Float y;
    Float z;
    Float term;
    Float length;

    rest = 1.0F - weight;
    x = *first++;
    x *= rest;
    term = *second++;
    term *= weight;
    x += term;
    y = *first++;
    y *= rest;
    term = *second++;
    term *= weight;
    y += term;
    z = *first;
    z *= rest;
    term = *second;
    term *= weight;
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

/* blend first toward second by weight into out */
static void action_blend_point(Float *first, Float *second, Float *out,
                               Float weight)
{
    Float rest;
    Float x;
    Float y;
    Float z;
    Float term;

    out += 3;
    rest = 1.0F - weight;
    x = *first++;
    x *= rest;
    term = *second++;
    term *= weight;
    x += term;
    y = *first++;
    y *= rest;
    term = *second++;
    term *= weight;
    y += term;
    z = *first;
    z *= rest;
    term = *second;
    term *= weight;
    z += term;
    *--out = z;
    *--out = y;
    *--out = x;
}

/* where the node's current shape factor has keys, put the data of the two
 * keys around the shape motion's frame into pair. put the second's weight
 * into *weight, then add SHAPE_POINTS to *keyed, or for normals,
 * SHAPE_NORMALS */
static void action_get_shape_points(Sint32 *keyed, Float *weight,
                                    NJS_POINT3 **pair)
{
    struct nj_motion_info *info = &_nj_shape_data_info_;
    Uint32 factor = info->factor;
    NJS_MKEY_P *keys = info->p[factor];
    Uint32 index;

    if (keys) {
        Uint32 count = info->nb[factor];
        action_find_shape_key(keys, count, &index);
        *weight = action_sample_shape(keys, index, count, pair);
        *keyed += SHAPE_POINTS;
    }
}

static void action_get_shape_normals(Sint32 *keyed, Float *weight,
                                     NJS_POINT3 **pair)
{
    struct nj_motion_info *info = &_nj_shape_data_info_;
    Uint32 factor = info->factor;
    NJS_MKEY_P *keys = info->p[factor];
    Uint32 index;

    if (keys) {
        Uint32 count = info->nb[factor];
        action_find_shape_key(keys, count, &index);
        *weight = action_sample_shape(keys, index, count, pair);
        *keyed += SHAPE_NORMALS;
    }
}

/* blend count points of the pair into _nj_shape_buf_, and set model->points
 * to them. the normals' versions blend normals into the buffer's start, or
 * past count points, and set model->normals. count must be at least 1 */
static void action_morph_points(NJS_MODEL *model, Sint32 count,
                                NJS_POINT3 **pair, Float weight)
{
    NJS_POINT3 *out = (NJS_POINT3 *)_nj_shape_buf_;
    NJS_POINT3 *buffer = out;
    NJS_POINT3 *first = pair[0];
    NJS_POINT3 *second = pair[1];
    Sint32 left = count;

    do {
        action_blend_point(&first->x, &second->x, &out->x, weight);
        first++;
        second++;
        out++;
    } while (--left != 0);
    model->points = buffer;
}

static void action_morph_normals(NJS_MODEL *model, Sint32 count,
                                 NJS_POINT3 **pair, Float weight)
{
    NJS_POINT3 *out = (NJS_POINT3 *)_nj_shape_buf_;
    NJS_POINT3 *buffer = out;
    NJS_POINT3 *first = pair[0];
    NJS_POINT3 *second = pair[1];
    Sint32 left = count;

    do {
        action_blend_normal(&first->x, &second->x, &out->x, weight);
        first++;
        second++;
        out++;
    } while (--left != 0);
    model->normals = buffer;
}

static void action_morph_normals_after_points(NJS_MODEL *model, Sint32 count,
                                              NJS_POINT3 **pair, Float weight)
{
    NJS_POINT3 *out = (NJS_POINT3 *)&_nj_shape_buf_[count * 3];
    NJS_POINT3 *buffer = out;
    NJS_POINT3 *first = pair[0];
    NJS_POINT3 *second = pair[1];
    Sint32 left = count;

    do {
        action_blend_normal(&first->x, &second->x, &out->x, weight);
        first++;
        second++;
        out++;
    } while (--left != 0);
    model->normals = buffer;
}

/* draw the node's model with its points, normals or both morphed by the shape
 * motion, then put its own back. a null model, or one under NJD_EVAL_HIDE,
 * isn't drawn */
static void action_draw_morphed_model(NJS_OBJECT *object)
{
    NJS_MODEL *model = object->model;
    NJS_POINT3 *point_pair[2];
    NJS_POINT3 *normal_pair[2];
    NJS_POINT3 *points;
    NJS_POINT3 *normals;
    Float point_weight;
    Float normal_weight;
    Sint32 keyed;
    Sint32 count;

    if (!model || (object->evalflags & NJD_EVAL_HIDE)) {
        return;
    }
    points = model->points;
    normals = model->normals;
    count = model->nbPoint;
    keyed = 0;
    switch (action_shape_morphs) {
    case SHAPE_POINTS:
        action_get_shape_points(&keyed, &point_weight, point_pair);
        break;
    case SHAPE_NORMALS:
        action_get_shape_normals(&keyed, &normal_weight, normal_pair);
        break;
    case SHAPE_BOTH:
        action_get_shape_points(&keyed, &point_weight, point_pair);
        _nj_shape_data_info_.factor++;
        action_get_shape_normals(&keyed, &normal_weight, normal_pair);
        break;
    }
    switch (keyed) {
    case SHAPE_POINTS:
        action_morph_points(model, count, point_pair, point_weight);
        break;
    case SHAPE_NORMALS:
        action_morph_normals(model, count, normal_pair, normal_weight);
        break;
    case SHAPE_BOTH:
        action_morph_points(model, count, point_pair, point_weight);
        action_morph_normals_after_points(model, count, normal_pair,
                                          normal_weight);
        break;
    }
    draw_model_nested(model);
    model->points = points;
    model->normals = normals;
}

/* the steps that move the current matrix by the node's own value */
static void action_translate_object(NJS_OBJECT *object)
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

static void action_rotate_object_xyz(NJS_OBJECT *object)
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

static void action_rotate_object_zxy(NJS_OBJECT *object)
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

static void action_scale_object(NJS_OBJECT *object)
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

/* the node steps, by factor count. each but action_motion_node_0 takes the
 * node's record from mdata and moves mdata past it. each calls the position,
 * angle and scale steps */
static void action_motion_node_0(NJS_OBJECT *object)
{
    _nj_motion_translate_(object);
    _nj_motion_rotate_(object);
    _nj_motion_scale_(object);
}

static void action_motion_node_1(NJS_OBJECT *object)
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

static void action_motion_node_2(NJS_OBJECT *object)
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

static void action_motion_node_3(NJS_OBJECT *object)
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

static void action_motion_node_4(NJS_OBJECT *object)
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

/* the shape node steps, by what the shape motion morphs. with nothing, the
 * node's model draws as it is, unless null or under NJD_EVAL_HIDE. the others
 * take the node's record from mdata, draw the model morphed and move mdata
 * past the record */
static void action_shape_node_0(NJS_OBJECT *object)
{
    if (object->model && !(object->evalflags & NJD_EVAL_HIDE)) {
        draw_model_nested(object->model);
    }
}

static void action_shape_node_1(NJS_OBJECT *object)
{
    struct nj_motion_info *info = &_nj_shape_data_info_;
    NJS_MDATA1 *record = info->mdata;

    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    action_draw_morphed_model(object);
    info->mdata = record + 1;
}

static void action_shape_node_2(NJS_OBJECT *object)
{
    struct nj_motion_info *info = &_nj_shape_data_info_;
    NJS_MDATA2 *record = info->mdata;

    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    action_draw_morphed_model(object);
    info->mdata = record + 1;
}

static void action_draw_motion_tree(NJS_OBJECT *object)
{
    do {
        Uint32 flags = object->evalflags;

        njPushMatrix(NULL);
        if (!(flags & NJD_EVAL_SKIP)) {
            _nj_motion_data_func_(object);
        } else {
            void (*rotate)(NJS_MATRIX *matrix, Angle angx, Angle angy,
                           Angle angz);
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
             * NJD_EVAL_SKIP turns by the three words at the stack pointer, the
             * PR, R8 and R9 the function saved on entry, read at 0x8c04177c,
             * 0x8c04177e and 0x8c041780 */
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
            draw_model_nested(object->model);
        }
        if (object->child) {
            action_draw_motion_tree(object->child);
        }
        njPopMatrix(1);
        object = object->sibling;
    } while (object);
}

static void action_draw_shape_tree(NJS_OBJECT *object)
{
    do {
        Uint32 flags = object->evalflags;

        njPushMatrix(NULL);
        if (!(flags & NJD_EVAL_SKIP)) {
            _nj_motion_data_func_(object);
        } else {
            void (*rotate)(NJS_MATRIX *matrix, Angle angx, Angle angy,
                           Angle angz);
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
            draw_model_nested(object->model);
        }
        if (object->child) {
            action_draw_shape_tree(object->child);
        }
        njPopMatrix(1);
        object = object->sibling;
    } while (object);
}

void draw_action(NJS_ACTION *action, Float frame)
{
    NJS_MOTION *motion = action->motion;
    NJS_OBJECT *object = action->object;

    if (action_start_motion(motion, frame) == 0) {
        action_select_steps(object);
        njGetkmPointer();
        action_draw_motion_tree(object);
        njSetkmPointer();
    }
}

void draw_motion(NJS_OBJECT *object, NJS_MOTION *motion, Float frame)
{
    if (action_start_motion(motion, frame) == 0) {
        action_select_steps(object);
        njGetkmPointer();
        action_draw_motion_tree(object);
        njSetkmPointer();
    }
}

void draw_shape_motion(NJS_OBJECT *object, NJS_MOTION *motion,
                       NJS_MOTION *shape, Float frame)
{
    if (action_start_motion(motion, frame) == 0 &&
        action_start_shape(shape, frame) == 0) {
        action_select_steps(object);
        njGetkmPointer();
        action_draw_shape_tree(object);
        njSetkmPointer();
    }
}
