/* the game's own motion link draws, which draw each model with draw_model.
 * their steps are this file's own, and interpolate linearly whatever
 * interpolation a motion names */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/model.h"

static void action_link_translate_keys(NJS_OBJECT *object);
static void action_link_scale_keys(NJS_OBJECT *object);
static void action_link_rotate_keys(NJS_OBJECT *object);
static void action_link_translate_object(NJS_OBJECT *object);
static void action_link_rotate_object(NJS_OBJECT *object);
static void action_link_scale_object(NJS_OBJECT *object);
static void action_link_motion_node_0(NJS_OBJECT *object);
static void action_link_motion_node_1(NJS_OBJECT *object);
static void action_link_motion_node_2(NJS_OBJECT *object);
static void action_link_motion_node_3(NJS_OBJECT *object);
static void action_link_motion_node_4(NJS_OBJECT *object);
static void action_link_shape_node_0(NJS_OBJECT *object);
static void action_link_shape_node_1(NJS_OBJECT *object);
static void action_link_shape_node_2(NJS_OBJECT *object);
static void action_link_transform(void);
static void action_link_draw_morphed_model(NJS_OBJECT *object);

/* a slot's node steps, by its motion's factor count */
static void (*action_link_motion_steps[])(NJS_OBJECT *obj) = {
    action_link_motion_node_0, action_link_motion_node_1,
    action_link_motion_node_2, action_link_motion_node_3,
    action_link_motion_node_4};
/* a slot's shape node steps, by what its shape motion morphs: nothing,
 * points, normals or both */
static void (*action_link_shape_steps[])(NJS_OBJECT *obj) = {
    action_link_shape_node_0, action_link_shape_node_1,
    action_link_shape_node_1, action_link_shape_node_2};

/* set *index to the last of the count keys whose keyframe is at most the
 * current slot's frame, or to 0 where none is. the keys must be in frame
 * order, and count at least 1. the frame is truncated to an integer and
 * compared unsigned. keys are NJS_MKEY_F or NJS_MKEY_A, which both lead with
 * the keyframe and are 16 bytes long */
static void action_link_find_key(void *keys, Uint32 count, Uint32 *index)
{
    Uint32 frame;
    Uint32 low;
    Uint32 middle;
    Uint32 key;

    low = 0;
    frame = (Sint32)_nj_motlink_data_info_[_nj_mot_current_data_].frame;
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

/* action_link_find_key for shape keys, at the frame of the current slot's
 * shape motion */
static void action_link_find_shape_key(NJS_MKEY_P *keys, Uint32 count,
                                       Uint32 *index)
{
    Uint32 frame;
    Uint32 low;
    Uint32 middle;
    Uint32 key;

    low = 0;
    frame = (Sint32)_nj_shapelink_data_info_[_nj_mot_current_data_].frame;
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

/* start slot's shape motion at link->frame[slot]: set the slot's
 * _nj_shapelink_data_info_, its shape node step and what it morphs. it
 * returns 1, setting nothing, when that frame is at or above the motion's
 * nbFrame, and 0 otherwise. a shape motion also sets
 * _nj_mot_shape_spline_cnt_ to its interpolation, which no step here reads.
 * a null shape motion takes the step that keeps each model's own points and
 * normals */
static Sint32 action_link_start_shape(NJS_MOTION_LINK *link, Sint32 slot)
{
    struct nj_motlink_data_info *info;
    NJS_MOTION *motion;
    Uint16 morphs;
    Uint16 type;

    info = &_nj_shapelink_data_info_[slot];
    motion = link->motion[slot];
    morphs = 0;
    type = 0;
    if (motion != NULL) {
        if (link->frame[slot] >= (Float)(Sint32)motion->nbFrame) {
            return 1;
        }
        info->frame = link->frame[slot];
        info->mdata = motion->mdata;
        info->nbFrame = motion->nbFrame;
        type = motion->type;
        _nj_mot_shape_spline_cnt_ = (motion->inp_fn & NJD_MTYPE_MASK) > 0;
    }
    info->factor = 0;
    info->unk_18 = 0;
    if (type & NJD_MTYPE_VERT_4) {
        morphs = SHAPE_POINTS;
    }
    if (type & NJD_MTYPE_NORM_5) {
        morphs += SHAPE_NORMALS;
    }
    _nj_shapelink_data_func_[slot] = action_link_shape_steps[morphs];
    /* @hack redundant: the type stored as 0 before it is stored as type, the
     * two stores the image makes (MOV.W at 0x8c041b02 and 0x8c041b08);
     * without it, the function runs 4 bytes short and 28 of 78 words
     * differ */
    _nj_motlink_shape_data_type_[slot] = 0;
    _nj_motlink_shape_data_cnt_[slot] = morphs;
    _nj_motlink_shape_data_type_[slot] = type;
    return 0;
}

/* start slot's motion at link->frame[slot]: set the slot's
 * _nj_motlink_data_info_, factor count and type. it returns 1, setting
 * nothing, when that frame is at or above the motion's nbFrame, and 0
 * otherwise. a null motion has no factors and no type, so the slot poses each
 * node by its own pos, ang and scl */
static Sint32 action_link_start_motion(NJS_MOTION_LINK *link, Sint32 slot)
{
    struct nj_motlink_data_info *info;
    NJS_MOTION *motion;
    Uint16 type;
    Uint16 count;

    info = &_nj_motlink_data_info_[slot];
    motion = link->motion[slot];
    type = 0;
    count = 0;
    if (motion != NULL) {
        if (link->frame[slot] >= (Float)(Sint32)motion->nbFrame) {
            return 1;
        }
        info->frame = link->frame[slot];
        info->mdata = motion->mdata;
        info->nbFrame = motion->nbFrame;
        count = motion->inp_fn & ~NJD_MTYPE_MASK;
        type = motion->type;
    }
    /* @hack spelling: the zero the stores and the result take, held in link,
     * which is spent by then. SHC returns it with MOV R4,R0 and the NOP that
     * -extra=a=400 writes after it (0x8c041b7a, 0x8c041b7c); as a plain 0
     * the result takes MOV #0,R0, the function runs 2 bytes short and 6 of 57
     * words differ */
    link = 0;
    info->factor = (Uint32)link;
    info->unk_18 = (Uint32)link;
    _nj_motlink_data_cnt_[slot] = count;
    _nj_motlink_data_type_[slot] = type;
    return (Sint32)link;
}

/* set slot's steps for its motion, once started. the node step goes by the
 * slot's factor count. position, angle and scale each take the keyed step
 * where the slot's type has that factor, and the node's own value where it
 * hasn't. object's NJD_EVAL_ZXY_ANG picks the rotation order for every node,
 * in _nj_rotate_func_, which the two slots share */
static void action_link_select_steps(NJS_OBJECT *object, Sint32 slot)
{
    Uint32 type;
    void (*rotate)(NJS_MATRIX *matrix, Angle angx, Angle angy, Angle angz);
    void (*step)(NJS_OBJECT *obj);

    _nj_motlink_data_func_[slot] =
        action_link_motion_steps[_nj_motlink_data_cnt_[slot]];
    rotate = njRotateXYZ;
    if (object->evalflags & NJD_EVAL_ZXY_ANG) {
        rotate = njRotateZXY;
    }
    _nj_rotate_func_ = rotate;
    type = _nj_motlink_data_type_[slot];
    step = action_link_translate_object;
    if (type & NJD_MTYPE_POS_0) {
        step = action_link_translate_keys;
    }
    _nj_motion_translate_data_[slot] = step;
    step = action_link_rotate_object;
    if (type & NJD_MTYPE_ANG_1) {
        step = action_link_rotate_keys;
    }
    _nj_motion_rotate_data_[slot] = step;
    step = action_link_scale_object;
    if (type & NJD_MTYPE_SCL_2) {
        step = action_link_scale_keys;
    }
    _nj_motion_scale_data_[slot] = step;
}

/* blend key index's three values toward the next key's, into out. the key
 * after the last is the first. the weight is the current slot's frame less
 * key index's keyframe, divided by the span to the next keyframe where that
 * span is positive */
static void action_link_sample_vector(NJS_MKEY_F *keys, Uint32 index,
                                      Uint32 count, Float *out)
{
    NJS_MKEY_F *key;
    Float *from;
    Float *to;
    Float weight;
    Float rest;
    Float inverse;
    Float ax;
    Float ay;
    Float az;
    Float bx;
    Float by;
    Float bz;
    Sint32 span;

    key = &keys[index];
    if (index + 1 < count) {
        keys = key + 1;
    }
    weight = _nj_motlink_data_info_[_nj_mot_current_data_].frame -
             (Float)(Sint32)key->keyframe;
    span = (Sint32)keys->keyframe - (Sint32)key->keyframe;
    if (span > 0) {
        /* inverse becomes 1 / span, to FSRRA's precision, as the square of
         * its 1 / sqrt(span) */
        inverse = (Float)span;
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        weight *= inverse;
    }
    /* @hack spelling: rest set to 1 before weight is taken from it, which gives
     * rest FR5 and ax FR8 (FLDI1 FR5 at 0x8c041c1c); set as 1.0F - weight,
     * they trade and 11 of 114 words differ */
    rest = 1.0F;
    rest -= weight;
    from = key->key;
    to = keys->key;
    ax = *from++;
    ax *= rest;
    bx = *to++;
    bx *= weight;
    ax += bx;
    ay = *from++;
    ay *= rest;
    by = *to++;
    by *= weight;
    ay += by;
    az = *from;
    az *= rest;
    bz = *to;
    bz *= weight;
    az += bz;
    out += 3;
    *--out = az;
    *--out = ay;
    *--out = ax;
}

/* action_link_sample_vector for angles, each blended as a Float */
static void action_link_sample_angles(NJS_MKEY_A *keys, Uint32 index,
                                      Uint32 count, Angle *out)
{
    NJS_MKEY_A *key;
    Float weight;
    Float rest;
    Float inverse;
    Sint32 span;

    key = &keys[index];
    if (index + 1 < count) {
        keys = key + 1;
    }
    weight = _nj_motlink_data_info_[_nj_mot_current_data_].frame -
             (Float)(Sint32)key->keyframe;
    span = (Sint32)keys->keyframe - (Sint32)key->keyframe;
    if (span > 0) {
        inverse = (Float)span;
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        weight *= inverse;
    }
    rest = 1.0F - weight;
    out[0] =
        (Angle)(((Float)key->key[0] * rest) + ((Float)keys->key[0] * weight));
    out[1] =
        (Angle)(((Float)key->key[1] * rest) + ((Float)keys->key[1] * weight));
    out[2] =
        (Angle)(((Float)key->key[2] * rest) + ((Float)keys->key[2] * weight));
}

/* the keyed steps. each writes the current slot's position, scale or angles
 * for the node, sampled from the node's current factor at the slot's frame. a
 * factor without keys gives the node's own value instead. a sampled value
 * sets the slot's bit in _nj_pos_int_flag_, _nj_scl_int_flag_ or
 * _nj_ang_int_flag_. each step then advances the factor */
static void action_link_translate_keys(NJS_OBJECT *object)
{
    struct nj_motlink_data_info *info;
    Float *out;
    Float *from;
    NJS_MKEY_F *keys;
    Uint32 factor;
    Uint32 count;
    Uint32 index;
    Float x;
    Float y;
    Float z;

    info = &_nj_motlink_data_info_[_nj_mot_current_data_];
    out = &_nj_motion_link_pos_[_nj_mot_current_data_ * 3];
    factor = info->factor;
    keys = info->p[factor];
    if (keys != NULL) {
        count = info->nb[factor];
        action_link_find_key(keys, count, &index);
        action_link_sample_vector(keys, index, count, out);
        _nj_pos_int_flag_ |= _nj_mot_current_data_ + 1;
    } else {
        from = object->pos;
        x = *from++;
        y = *from++;
        z = *from;
        out += 3;
        *--out = z;
        *--out = y;
        *--out = x;
    }
    info->factor = factor + 1;
}

static void action_link_scale_keys(NJS_OBJECT *object)
{
    struct nj_motlink_data_info *info;
    Float *out;
    Float *from;
    NJS_MKEY_F *keys;
    Uint32 factor;
    Uint32 count;
    Uint32 index;
    Float x;
    Float y;
    Float z;

    info = &_nj_motlink_data_info_[_nj_mot_current_data_];
    out = &_nj_motion_link_scl_[_nj_mot_current_data_ * 3];
    factor = info->factor;
    keys = info->p[factor];
    if (keys != NULL) {
        count = info->nb[factor];
        action_link_find_key(keys, count, &index);
        action_link_sample_vector(keys, index, count, out);
        _nj_scl_int_flag_ |= _nj_mot_current_data_ + 1;
    } else {
        from = object->scl;
        x = *from++;
        y = *from++;
        z = *from;
        out += 3;
        *--out = z;
        *--out = y;
        *--out = x;
    }
    info->factor = factor + 1;
}

static void action_link_rotate_keys(NJS_OBJECT *object)
{
    struct nj_motlink_data_info *info;
    Angle *out;
    Angle *from;
    NJS_MKEY_A *keys;
    Uint32 factor;
    Uint32 count;
    Uint32 index;

    info = &_nj_motlink_data_info_[_nj_mot_current_data_];
    out = &_nj_motion_link_ang_[_nj_mot_current_data_ * 3];
    factor = info->factor;
    keys = info->p[factor];
    if (keys != NULL) {
        count = info->nb[factor];
        action_link_find_key(keys, count, &index);
        action_link_sample_angles(keys, index, count, out);
        _nj_ang_int_flag_ |= _nj_mot_current_data_ + 1;
    } else {
        from = object->ang;
        out[0] = from[0];
        out[1] = from[1];
        out[2] = from[2];
    }
    info->factor = factor + 1;
}

/* put the data of four keys into data: the key before key index, the key,
 * and the two after it. the key before the first is the last, and any key
 * past the last is the first. it returns the weight of the key after key
 * index at the frame of the current slot's shape motion, as
 * action_link_sample_vector weighs it */
static Float action_link_sample_shape(NJS_MKEY_P *keys, Uint32 index,
                                      Uint32 count, NJS_POINT3 **data)
{
    NJS_MKEY_P *key;
    NJS_MKEY_P *before;
    NJS_MKEY_P *next;
    Float weight;
    Float inverse;
    Sint32 span;

    key = &keys[index];
    before = &keys[count - 1];
    if (index > 0) {
        before = key - 1;
    }
    next = keys;
    if (index + 1 < count) {
        next = key + 1;
    }
    if (index + 2 < count) {
        keys = next + 1;
    }
    weight = _nj_shapelink_data_info_[_nj_mot_current_data_].frame -
             (Float)(Sint32)key->keyframe;
    span = (Sint32)next->keyframe - (Sint32)key->keyframe;
    if (span > 0) {
        inverse = (Float)span;
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        weight *= inverse;
    }
    data[0] = before->key;
    data[1] = key->key;
    data[2] = next->key;
    data[3] = keys->key;
    return weight;
}

/* NOLINTNEXTLINE(clang-diagnostic-undefined-internal) */
static Float squared_length(Float x, Float y, Float z, Float fourth);

#ifdef GCC
ALWAYS_INLINE Float squared_length(Float x, Float y, Float z, Float fourth)
{
    return zam_fipr_self(x, y, z, fourth);
}
#else
/* clang-format off */
#ifndef __clang__
/* the squared length of (x, y, z, fourth), in FR0. SHC expands the body at
 * the call. size=4 gives the body's length in bytes. without it, SHC 5.0
 * R10 can place a literal pool ahead of the expansion, with a BRA over the
 * pool. shckit's use-assembler-pools patch refuses such a pool */
/* @asm operand: fipr-registers, FIPR FV4,FV4 at 0x8c041fbc squares the
 * blended normal in the registers it is in */
#pragma inline_asm(squared_length(size=4))
static Float squared_length(Float x, Float y, Float z, Float fourth)
{
        FIPR        FV4,FV4
        FMOV.S      FR7,FR0
}
#endif
/* clang-format on */
#endif

/* blend first toward second by weight into out, and scale the blend to unit
 * length, to FSRRA's precision. a blend whose squared length is not above
 * 0.00001 becomes (0, 0, 1) */
static void action_link_blend_normal(Float *first, Float *second, Float *out,
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
static void action_link_blend_point(Float *first, Float *second, Float *out,
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

/* where the current slot's shape factor has keys, put the data of the four
 * keys around the slot's shape frame into data. put data[2]'s weight into
 * *weight. buffer then becomes the slot's points, and the slot's bit is set
 * in _nj_pnt_int_flag_. *keyed takes what the slot's shape motion morphs.
 * the normals' version sets the slot's normals and _nj_nml_int_flag_ alike */
static void action_link_get_shape_points(Uint32 *keyed, Float *weight,
                                         NJS_POINT3 **data, NJS_POINT3 *buffer)
{
    struct nj_motlink_data_info *info;
    NJS_MKEY_P *keys;
    Uint32 factor;
    Uint32 count;
    Uint32 index;

    info = &_nj_shapelink_data_info_[_nj_mot_current_data_];
    factor = info->factor;
    keys = info->p[factor];
    if (keys != NULL) {
        count = info->nb[factor];
        action_link_find_shape_key(keys, count, &index);
        *weight = action_link_sample_shape(keys, index, count, data);
        _nj_shape_link_point_[_nj_mot_current_data_] = buffer;
        _nj_pnt_int_flag_ |= _nj_mot_current_data_ + 1;
        *keyed = _nj_motlink_shape_data_cnt_[_nj_mot_current_data_];
    }
}

static void action_link_get_shape_normals(Uint32 *keyed, Float *weight,
                                          NJS_POINT3 **data, NJS_POINT3 *buffer)
{
    struct nj_motlink_data_info *info;
    NJS_MKEY_P *keys;
    Uint32 factor;
    Uint32 count;
    Uint32 index;

    info = &_nj_shapelink_data_info_[_nj_mot_current_data_];
    factor = info->factor;
    keys = info->p[factor];
    if (keys != NULL) {
        count = info->nb[factor];
        action_link_find_shape_key(keys, count, &index);
        *weight = action_link_sample_shape(keys, index, count, data);
        _nj_shape_link_normal_[_nj_mot_current_data_] = buffer;
        _nj_nml_int_flag_ |= _nj_mot_current_data_ + 1;
        *keyed = _nj_motlink_shape_data_cnt_[_nj_mot_current_data_];
    }
}

/* the same, for a shape motion that morphs both, but adding SHAPE_POINTS to
 * *keyed, or for normals SHAPE_NORMALS */
static void action_link_get_both_points(Uint32 *keyed, Float *weight,
                                        NJS_POINT3 **data, NJS_POINT3 *buffer)
{
    struct nj_motlink_data_info *info;
    NJS_MKEY_P *keys;
    Uint32 factor;
    Uint32 count;
    Uint32 index;

    info = &_nj_shapelink_data_info_[_nj_mot_current_data_];
    factor = info->factor;
    keys = info->p[factor];
    if (keys != NULL) {
        count = info->nb[factor];
        action_link_find_shape_key(keys, count, &index);
        *weight = action_link_sample_shape(keys, index, count, data);
        _nj_shape_link_point_[_nj_mot_current_data_] = buffer;
        _nj_pnt_int_flag_ |= _nj_mot_current_data_ + 1;
        *keyed += SHAPE_POINTS;
    }
}

static void action_link_get_both_normals(Uint32 *keyed, Float *weight,
                                         NJS_POINT3 **data, NJS_POINT3 *buffer)
{
    struct nj_motlink_data_info *info;
    NJS_MKEY_P *keys;
    Uint32 factor;
    Uint32 count;
    Uint32 index;

    info = &_nj_shapelink_data_info_[_nj_mot_current_data_];
    factor = info->factor;
    keys = info->p[factor];
    if (keys != NULL) {
        count = info->nb[factor];
        action_link_find_shape_key(keys, count, &index);
        *weight = action_link_sample_shape(keys, index, count, data);
        _nj_shape_link_normal_[_nj_mot_current_data_] = buffer;
        _nj_nml_int_flag_ |= _nj_mot_current_data_ + 1;
        *keyed += SHAPE_NORMALS;
    }
}

/* blend count points of data[1] toward data[2]'s by weight, into out. the
 * normals' version blends normals alike. count must be at least 1 */
static void action_link_morph_points(NJS_POINT3 **data, NJS_POINT3 *out,
                                     Float weight, Sint32 count)
{
    NJS_POINT3 *first;
    NJS_POINT3 *second;
    Sint32 left;

    first = data[1];
    second = data[2];
    left = count;
    do {
        action_link_blend_point(&first->x, &second->x, &out->x, weight);
        first++;
        second++;
        out++;
    } while (--left != 0);
}

static void action_link_morph_normals(NJS_POINT3 **data, NJS_POINT3 *out,
                                      Float weight, Sint32 count)
{
    NJS_POINT3 *first;
    NJS_POINT3 *second;
    Sint32 left;

    first = data[1];
    second = data[2];
    left = count;
    do {
        action_link_blend_normal(&first->x, &second->x, &out->x, weight);
        first++;
        second++;
        out++;
    } while (--left != 0);
}

/* the current slot's shape step for the node's model. the slot's points and
 * normals start as the model's own. those the slot's shape factors have keys
 * for are morphed into the slot's part of _nj_shape_buf_. for a model of n
 * points, that part holds n points and then n normals, from slot * 6n Floats
 * in. a null model, or one under NJD_EVAL_HIDE, is left alone */
static void action_link_morph_model(NJS_OBJECT *object)
{
    NJS_POINT3 *point_data[4];
    NJS_POINT3 *normal_data[4];
    Float point_weight;
    Float normal_weight;
    Uint32 keyed;
    NJS_MODEL *model;
    NJS_POINT3 *points;
    NJS_POINT3 *normals;
    Sint32 count;
    Sint32 offset;

    model = object->model;
    if (model == NULL) {
        return;
    }
    if (object->evalflags & NJD_EVAL_HIDE) {
        return;
    }
    keyed = 0;
    count = object->model->nbPoint;
    offset = _nj_mot_current_data_ * count * 6;
    points = (NJS_POINT3 *)&_nj_shape_buf_[offset];
    normals = (NJS_POINT3 *)&_nj_shape_buf_[(count * 3) + offset];
    _nj_shape_link_point_[_nj_mot_current_data_] = model->points;
    _nj_shape_link_normal_[_nj_mot_current_data_] = model->normals;
    switch (_nj_motlink_shape_data_cnt_[_nj_mot_current_data_]) {
    case SHAPE_POINTS:
        action_link_get_shape_points(&keyed, &point_weight, point_data, points);
        break;
    case SHAPE_NORMALS:
        action_link_get_shape_normals(&keyed, &normal_weight, normal_data,
                                      normals);
        break;
    case SHAPE_BOTH:
        action_link_get_both_points(&keyed, &point_weight, point_data, points);
        _nj_shapelink_data_info_[_nj_mot_current_data_].factor++;
        action_link_get_both_normals(&keyed, &normal_weight, normal_data,
                                     normals);
        break;
    }
    switch (keyed) {
    case SHAPE_POINTS:
        action_link_morph_points(point_data, points, point_weight, count);
        break;
    case SHAPE_NORMALS:
        action_link_morph_normals(normal_data, normals, normal_weight, count);
        break;
    case SHAPE_BOTH:
        action_link_morph_points(point_data, points, point_weight, count);
        action_link_morph_normals(normal_data, normals, normal_weight, count);
        break;
    }
}

/* the steps that give the current slot the node's own position, angles or
 * scale */
static void action_link_translate_object(NJS_OBJECT *object)
{
    Float *from;
    Float *out;
    Float x;
    Float y;
    Float z;

    from = object->pos;
    out = &_nj_motion_link_pos_[_nj_mot_current_data_ * 3];
    x = *from++;
    y = *from++;
    z = *from;
    out += 3;
    *--out = z;
    *--out = y;
    *--out = x;
}

static void action_link_rotate_object(NJS_OBJECT *object)
{
    Angle *from;
    Angle *out;
    Angle x;
    Angle y;
    Angle z;

    from = object->ang;
    out = &_nj_motion_link_ang_[_nj_mot_current_data_ * 3];
    x = from[0];
    y = from[1];
    z = from[2];
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

static void action_link_scale_object(NJS_OBJECT *object)
{
    Float *from;
    Float *out;
    Float x;
    Float y;
    Float z;

    from = object->scl;
    out = &_nj_motion_link_scl_[_nj_mot_current_data_ * 3];
    x = *from++;
    y = *from++;
    z = *from;
    out += 3;
    *--out = z;
    *--out = y;
    *--out = x;
}

/* the node steps, by factor count. each but action_link_motion_node_0 takes
 * the node's record from the current slot's mdata and moves mdata past it.
 * each runs the slot's position, angle and scale steps */
static void action_link_motion_node_0(NJS_OBJECT *object)
{
    _nj_motion_translate_data_[_nj_mot_current_data_](object);
    _nj_motion_rotate_data_[_nj_mot_current_data_](object);
    _nj_motion_scale_data_[_nj_mot_current_data_](object);
}

static void action_link_motion_node_1(NJS_OBJECT *object)
{
    struct nj_motlink_data_info *info;
    NJS_MDATA1 *record;

    info = &_nj_motlink_data_info_[_nj_mot_current_data_];
    record = info->mdata;
    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    _nj_motion_translate_data_[_nj_mot_current_data_](object);
    _nj_motion_rotate_data_[_nj_mot_current_data_](object);
    _nj_motion_scale_data_[_nj_mot_current_data_](object);
    info->mdata = record + 1;
}

static void action_link_motion_node_2(NJS_OBJECT *object)
{
    struct nj_motlink_data_info *info;
    NJS_MDATA2 *record;

    info = &_nj_motlink_data_info_[_nj_mot_current_data_];
    record = info->mdata;
    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    _nj_motion_translate_data_[_nj_mot_current_data_](object);
    _nj_motion_rotate_data_[_nj_mot_current_data_](object);
    _nj_motion_scale_data_[_nj_mot_current_data_](object);
    info->mdata = record + 1;
}

static void action_link_motion_node_3(NJS_OBJECT *object)
{
    struct nj_motlink_data_info *info;
    NJS_MDATA3 *record;

    info = &_nj_motlink_data_info_[_nj_mot_current_data_];
    record = info->mdata;
    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    _nj_motion_translate_data_[_nj_mot_current_data_](object);
    _nj_motion_rotate_data_[_nj_mot_current_data_](object);
    _nj_motion_scale_data_[_nj_mot_current_data_](object);
    info->mdata = record + 1;
}

static void action_link_motion_node_4(NJS_OBJECT *object)
{
    struct nj_motlink_data_info *info;
    NJS_MDATA4 *record;

    info = &_nj_motlink_data_info_[_nj_mot_current_data_];
    record = info->mdata;
    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    _nj_motion_translate_data_[_nj_mot_current_data_](object);
    _nj_motion_rotate_data_[_nj_mot_current_data_](object);
    _nj_motion_scale_data_[_nj_mot_current_data_](object);
    info->mdata = record + 1;
}

/* the shape node steps, by what the current slot's shape motion morphs.
 * with nothing, the slot takes the model's own points and normals, unless the
 * model is null or under NJD_EVAL_HIDE. the others take the node's record
 * from the slot's mdata, run action_link_morph_model and move mdata past the
 * record */
static void action_link_shape_node_0(NJS_OBJECT *object)
{
    NJS_MODEL *model;

    model = object->model;
    if (model != NULL && !(object->evalflags & NJD_EVAL_HIDE)) {
        _nj_shape_link_point_[_nj_mot_current_data_] = model->points;
        _nj_shape_link_normal_[_nj_mot_current_data_] = model->normals;
    }
}

static void action_link_shape_node_1(NJS_OBJECT *object)
{
    struct nj_motlink_data_info *info;
    NJS_MDATA1 *record;

    info = &_nj_shapelink_data_info_[_nj_mot_current_data_];
    record = info->mdata;
    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    action_link_morph_model(object);
    info->mdata = record + 1;
}

static void action_link_shape_node_2(NJS_OBJECT *object)
{
    struct nj_motlink_data_info *info;
    NJS_MDATA2 *record;

    info = &_nj_shapelink_data_info_[_nj_mot_current_data_];
    record = info->mdata;
    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    action_link_morph_model(object);
    info->mdata = record + 1;
}

/* move the current matrix by the node's position, angles and scale. each is
 * the two slots' values blended by _nj_mot_current_frame_ from slot 0's
 * toward slot 1's, where either slot's keys gave it, and slot 0's otherwise.
 * a scale from keys blends the slots' positions instead */
static void action_link_transform(void)
{
    Float position[3];
    Float scale[3];
    Angle angle[3];
    Float *from;
    Float *to;
    Float *out;
    Angle *angle_from;
    Angle *angle_to;
    Float *base;
    Float ax;
    Float ay;
    Float az;
    Float bx;
    Float by;
    Float bz;
    /* @hack spelling: weight declared after the blend floats, so SHC writes
     * each b * weight into b's register, as FMUL FR4,FR6 at 0x8c04270c does;
     * declared ahead of them, it multiplies into a copy of weight, the
     * function runs 4 bytes long and 151 of 168 words differ */
    Float weight;
    Float rest;
    Float one;
    Angle x;
    Angle y;
    Angle z;

    one = 1.0F;
    from = base = _nj_motion_link_pos_;
    if (_nj_pos_int_flag_ > 0) {
        /* @hack spelling: x's load ahead of weight's, which keeps base in R7
         * and one in FR9 (0x8c0426e4, FLDI1 FR9 at 0x8c0426e2); loaded with
         * the rest of x's blend, base takes R13, which the function saves, it
         * runs 2 bytes long and 116 of 168 words differ */
        ax = *from++;
        weight = _nj_mot_current_frame_;
        rest = one - weight;
        to = base + 3;
        ax *= rest;
        bx = *to++;
        ax += bx * weight;
        ay = *from++;
        ay *= rest;
        by = *to++;
        ay += by * weight;
        az = *from;
        az *= rest;
        bz = *to;
        az += bz * weight;
    } else {
        ax = *from++;
        ay = *from++;
        az = *from;
    }
    out = position + 3;
    *--out = az;
    *--out = ay;
    *--out = ax;
    angle_to = _nj_motion_link_ang_;
    if (_nj_ang_int_flag_ > 0) {
        weight = _nj_mot_current_frame_;
        rest = one - weight;
        angle_from = angle_to;
        angle_to += 3;
        x = (Angle)(((Float)angle_from[0] * rest) +
                    ((Float)angle_to[0] * weight));
        y = (Angle)(((Float)angle_from[1] * rest) +
                    ((Float)angle_to[1] * weight));
        z = (Angle)(((Float)angle_from[2] * rest) +
                    ((Float)angle_to[2] * weight));
    } else {
        x = *angle_to++;
        y = *angle_to++;
        z = *angle_to;
    }
    angle[0] = x;
    angle[1] = y;
    angle[2] = z;
    if (_nj_scl_int_flag_ > 0) {
        weight = _nj_mot_current_frame_;
        rest = one - weight;
        /* @bug logic: the scale blends the two slots' positions, not their
         * scales. base holds _nj_motion_link_pos_, and from and to copy it
         * from R7 (MOV R7,R5 and MOV R7,R4 at 0x8c0427be and 0x8c0427c0) */
        from = to = base;
        to += 3;
        /* @hack spelling: x's and y's blends spelled unlike z's. each
         * multiplies by weight in a statement of its own, and x's also steps
         * from in one and adds as ax = bx + ax. they keep base in R7,
         * &_nj_mot_current_frame_ in R12 and one in FR9 (0x8c0426e4,
         * 0x8c0426ec, 0x8c0426e2); spelled as the position blend's, 51 of 168
         * words differ */
        ax = *from;
        from++;
        ax *= rest;
        bx = *to++;
        bx *= weight;
        ax = bx + ax;
        ay = *from++;
        ay *= rest;
        by = *to++;
        by *= weight;
        ay += by;
        az = *from;
        az *= rest;
        bz = *to;
        az += bz * weight;
    } else {
        from = _nj_motion_link_scl_;
        ax = *from++;
        ay = *from++;
        az = *from;
    }
    out = scale + 3;
    *--out = az;
    *--out = ay;
    *--out = ax;
    from = position;
    ax = *from++;
    ay = *from++;
    az = *from;
    njTranslate(NULL, ax, ay, az);
    angle_from = angle;
    x = angle_from[0];
    y = angle_from[1];
    z = angle_from[2];
    _nj_rotate_func_(NULL, x, y, z);
    from = scale;
    ax = *from++;
    ay = *from++;
    az = *from;
    njScale(NULL, ax, ay, az);
}

/* draw the node's model with draw_model, then put its own points and normals
 * back. a null model, or one under NJD_EVAL_HIDE, isn't drawn. where either
 * slot's keys gave points and either slot's gave normals, the two slots' are
 * first blended by _nj_mot_current_frame_ into _nj_shape_buf_. the blends
 * follow both slots' parts: for a model of n points, n points from 12n
 * Floats in, then n normals */
static void action_link_draw_morphed_model(NJS_OBJECT *object)
{
    NJS_MODEL *model;
    NJS_POINT3 *points;
    NJS_VECTOR *normals;
    NJS_POINT3 *first;
    NJS_POINT3 *second;
    NJS_POINT3 *out;
    NJS_POINT3 *out_normals;
    Sint32 count;
    Sint32 left;

    model = object->model;
    if (model == NULL) {
        return;
    }
    if (object->evalflags & NJD_EVAL_HIDE) {
        return;
    }
    points = model->points;
    normals = model->normals;
    if (_nj_pnt_int_flag_ > 0 && _nj_nml_int_flag_ > 0) {
        first = _nj_shape_link_point_[0];
        second = _nj_shape_link_point_[1];
        count = model->nbPoint;
        out = (NJS_POINT3 *)&_nj_shape_buf_[count * 12];
        left = count;
        do {
            action_link_blend_point(&first->x, &second->x, &out->x,
                                    _nj_mot_current_frame_);
            first++;
            second++;
            out++;
        } while (--left != 0);
        first = _nj_shape_link_normal_[0];
        second = _nj_shape_link_normal_[1];
        count = model->nbPoint;
        out_normals = (NJS_POINT3 *)&_nj_shape_buf_[count * 15];
        left = count;
        do {
            action_link_blend_normal(&first->x, &second->x, &out_normals->x,
                                     _nj_mot_current_frame_);
            first++;
            second++;
            out_normals++;
        } while (--left != 0);
        /* @bug memory: out and out_normals have run past their blends
         * when the model takes them (MOV.L at 0x8c042908 and 0x8c04290a), so
         * it is drawn with the blended normals for its points, and for its
         * normals the nbPoint vectors after them, past the blends */
        model->points = out;
        model->normals = out_normals;
    }
    draw_model(model);
    model->points = points;
    model->normals = normals;
}

static void action_link_draw_motion_tree(NJS_OBJECT *object)
{
    Uint32 flags;
    NJS_MATRIX *matrix;
    void (*rotate)(NJS_MATRIX *matrix, Angle angx, Angle angy, Angle angz);
    Float *from;
    Angle *angles;
    Float x;
    Float y;
    Float z;
    Angle angx;
    Angle angy;
    Angle angz;

    do {
        flags = object->evalflags;
        njPushMatrix(NULL);
        if (!(flags & NJD_EVAL_SKIP)) {
            _nj_pos_int_flag_ = 0;
            _nj_ang_int_flag_ = 0;
            _nj_scl_int_flag_ = 0;
            _nj_mot_current_data_ = 0;
            _nj_motlink_data_func_[0](object);
            _nj_mot_current_data_ = 1;
            _nj_motlink_data_func_[1](object);
            action_link_transform();
        } else {
            matrix = _nj_motion_data_info_.m;
            from = object->pos;
            x = *from++;
            y = *from++;
            z = *from;
            njTranslate(matrix, x, y, z);
            rotate = njRotateXYZ;
            if (flags & NJD_EVAL_ZXY_ANG) {
                rotate = njRotateZXY;
            }
            angles = object->ang;
            angx = angles[0];
            angy = angles[1];
            angz = angles[2];
            rotate(matrix, angx, angy, angz);
            from = object->scl;
            x = *from++;
            y = *from++;
            z = *from;
            njScale(matrix, x, y, z);
        }
        if (_nj_motion_callback_ != NULL) {
            _nj_motion_callback_(object);
        }
        if (object->model != NULL && !(flags & NJD_EVAL_HIDE)) {
            draw_model(object->model);
        }
        if (object->child != NULL) {
            action_link_draw_motion_tree(object->child);
        }
        njPopMatrix(1);
        object = object->sibling;
    } while (object != NULL);
}

static void action_link_draw_shape_tree(NJS_OBJECT *object)
{
    Uint32 flags;
    NJS_MATRIX *matrix;
    void (*rotate)(NJS_MATRIX *matrix, Angle angx, Angle angy, Angle angz);
    Float *from;
    Angle *angles;
    Float x;
    Float y;
    Float z;
    Angle angx;
    Angle angy;
    Angle angz;

    do {
        flags = object->evalflags;
        njPushMatrix(NULL);
        if (!(flags & NJD_EVAL_SKIP)) {
            _nj_pos_int_flag_ = 0;
            _nj_ang_int_flag_ = 0;
            _nj_scl_int_flag_ = 0;
            _nj_mot_current_data_ = 0;
            _nj_motlink_data_func_[0](object);
            _nj_mot_current_data_ = 1;
            _nj_motlink_data_func_[1](object);
            action_link_transform();
        } else {
            matrix = _nj_motion_data_info_.m;
            from = object->pos;
            x = *from++;
            y = *from++;
            z = *from;
            njTranslate(matrix, x, y, z);
            rotate = njRotateXYZ;
            if (flags & NJD_EVAL_ZXY_ANG) {
                rotate = njRotateZXY;
            }
            angles = object->ang;
            angx = angles[0];
            angy = angles[1];
            angz = angles[2];
            rotate(matrix, angx, angy, angz);
            from = object->scl;
            x = *from++;
            y = *from++;
            z = *from;
            njScale(matrix, x, y, z);
        }
        if (_nj_motion_callback_ != NULL) {
            _nj_motion_callback_(object);
        }
        if (!(flags & NJD_EVAL_SHAPE_SKIP)) {
            _nj_pnt_int_flag_ = 0;
            _nj_nml_int_flag_ = 0;
            _nj_mot_current_data_ = 0;
            _nj_shapelink_data_func_[0](object);
            _nj_mot_current_data_ = 1;
            _nj_shapelink_data_func_[1](object);
            action_link_draw_morphed_model(object);
        } else if (object->model != NULL && !(flags & NJD_EVAL_HIDE)) {
            draw_model(object->model);
        }
        if (object->child != NULL) {
            action_link_draw_shape_tree(object->child);
        }
        njPopMatrix(1);
        object = object->sibling;
    } while (object != NULL);
}

void draw_action_link(NJS_ACTION_LINK *link, Float ratio)
{
    NJS_OBJECT *object;
    NJS_MOTION_LINK *motions;

    if (ratio > 1.0F) {
        return;
    }
    motions = link->motionlink;
    object = link->object;
    _nj_mot_current_data_ = 0;
    _nj_mot_current_frame_ = ratio;
    if (action_link_start_motion(motions, 0)) {
        return;
    }
    if (action_link_start_motion(motions, 1)) {
        return;
    }
    action_link_select_steps(object, 0);
    action_link_select_steps(object, 1);
    njGetkmPointer();
    action_link_draw_motion_tree(object);
    njSetkmPointer();
}

void draw_motion_link(NJS_OBJECT *object, NJS_MOTION_LINK *link, Float ratio)
{
    if (ratio > 1.0F) {
        return;
    }
    _nj_mot_current_data_ = 0;
    _nj_mot_current_frame_ = ratio;
    if (action_link_start_motion(link, 0)) {
        return;
    }
    if (action_link_start_motion(link, 1)) {
        return;
    }
    action_link_select_steps(object, 0);
    action_link_select_steps(object, 1);
    njGetkmPointer();
    action_link_draw_motion_tree(object);
    njSetkmPointer();
}

void draw_shape_motion_link(NJS_OBJECT *object, NJS_MOTION_LINK *link,
                            NJS_MOTION_LINK *shape, Float ratio)
{
    if (ratio > 1.0F) {
        return;
    }
    _nj_mot_current_data_ = 0;
    _nj_mot_current_frame_ = ratio;
    if (action_link_start_motion(link, 0)) {
        return;
    }
    if (action_link_start_motion(link, 1)) {
        return;
    }
    if (action_link_start_shape(shape, 0)) {
        return;
    }
    if (action_link_start_shape(shape, 1)) {
        return;
    }
    action_link_select_steps(object, 0);
    action_link_select_steps(object, 1);
    njGetkmPointer();
    action_link_draw_shape_tree(object);
    njSetkmPointer();
}
