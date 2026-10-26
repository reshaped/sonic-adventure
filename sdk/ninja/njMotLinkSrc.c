/* the motion links' steps, their interpolations, the blends of the two slots
 * that njMakeMotionLink and njMakeShapeLink make, and their tables */
#include <ninja.h>
#include <sg_xpt.h>

/* an Angle's full turn, and half of it */
enum { TURN = 65536, HALF_TURN = TURN / 2 };
/* both slots' bits in an int flag such as _nj_pos_int_flag_, where slot 0's
 * bit is 1 and slot 1's is 2 */
enum { BOTH_SLOTS = 3 };

/* set *index to the last of count keys whose keyframe is at most the frame of
 * the current slot. the frame is truncated and compared unsigned. *index is 0
 * where no keyframe after the first is at most the frame. the keys must be in
 * frame order, and count at least 1. keys holds NJS_MKEY_F or NJS_MKEY_A, both
 * 16 bytes with the keyframe first */
static void link_find_motion_key(void *keys, Uint32 count, Uint32 *index)
{
    Uint32 low = 0;
    Uint32 middle;
    Uint32 key;
    Uint32 frame = (Sint32)_nj_motlink_data_info_[_nj_mot_current_data_].frame;

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

/* link_find_motion_key for shape keys, at the frame of the current slot's
 * shape motion */
static void link_find_shape_motion_key(NJS_MKEY_P *keys, Uint32 count,
                                       Uint32 *index)
{
    Uint32 low = 0;
    Uint32 middle;
    Uint32 key;
    Uint32 frame =
        (Sint32)_nj_shapelink_data_info_[_nj_mot_current_data_].frame;

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
 * out. the key after the last is the first. the weight t is the slot's frame
 * less the key's keyframe. where span, the next keyframe less the key's, is
 * above 0, t is then multiplied by an approximate 1 / span: FSRRA's
 * 1 / sqrt(span), squared */
static void link_linear_vector(NJS_MKEY_F *keys, Uint32 index, Uint32 count,
                               Float *out)
{
    NJS_MKEY_F *key = &keys[index];
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
        keys = key + 1;
    }
    /* @hack type: the keyframe read through a Sint32 pointer, which SHC loads
     * after the next key's (MOV.L @R14,R2 at 0x8c65406c); without it, as
     * (Float)(Sint32)key->keyframe, it loads it first and 10 of 58 words
     * differ */
    t = _nj_motlink_data_info_[_nj_mot_current_data_].frame -
        (Float)(*(const Sint32 *)&key->keyframe);
    span = keys->keyframe - key->keyframe;
    if (span > 0) {
        inverse = (Float)span;
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        t *= inverse;
    }
    rest = 1.0F;
    rest -= t;
    a = key->key;
    b = keys->key;
    x = *a++ * rest;
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
    out += 3;
    *--out = z;
    *--out = y;
    *--out = x;
}

/* link_linear_vector for angles, each blended as a Float and truncated */
static void link_linear_angles(NJS_MKEY_A *keys, Uint32 index, Uint32 count,
                               Angle *out)
{
    NJS_MKEY_A *key = &keys[index];
    Float t;
    Float rest;
    Float inverse;
    Sint32 span;

    if (index + 1 < count) {
        keys = key + 1;
    }
    /* @hack type: the keyframe read as in link_linear_vector (MOV.L @R14,R2
     * at 0x8c6540e0); without it, 10 of 72 words differ */
    t = _nj_motlink_data_info_[_nj_mot_current_data_].frame -
        (Float)(*(const Sint32 *)&key->keyframe);
    span = keys->keyframe - key->keyframe;
    if (span > 0) {
        inverse = (Float)span;
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        t *= inverse;
    }
    rest = 1.0F - t;
    out[0] = (Angle)(((Float)key->key[0] * rest) + ((Float)keys->key[0] * t));
    out[1] = (Angle)(((Float)key->key[1] * rest) + ((Float)keys->key[1] * t));
    out[2] = (Angle)(((Float)key->key[2] * rest) + ((Float)keys->key[2] * t));
}

/* NOLINTBEGIN(clang-diagnostic-undefined-internal) */
static void weighted_sum(Float *a, Float *b, Float *c, Float *d, Float wa,
                         Float wb, Float wc, Float wd, Float *out);
static void weighted_sum_angles(Angle *a, Angle *b, Angle *c, Angle *d,
                                Float wa, Float wb, Float wc, Float wd,
                                Angle *out);
/* NOLINTEND(clang-diagnostic-undefined-internal) */

/* clang-format off */
#ifndef __clang__
/* set out to wa * a + wb * b + wc * c + wd * d, where a to d are three Floats
 * each. the weights arrive in FV4. each component of out is one FIPR of them
 * with that component of a to d. SHC expands the body at each call. size=44
 * is its length in bytes. without it, SHC can write a literal pool ahead of
 * the expansion, behind a BRA that skips the pool, and the use-assembler-pools
 * patch refuses a pool so guarded */
/* @asm open: no inspect rule reads FV4 kept across its three FIPR FV4,FV0 */
#pragma inline_asm(weighted_sum(size=44))
static void weighted_sum(Float *a, Float *b, Float *c, Float *d, Float wa,
                         Float wb, Float wc, Float wd, Float *out)
{
        MOV.L       @R15,R1
        FMOV.S      @R4+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R6+,FR2
        FMOV.S      @R7+,FR3
        FIPR        FV4,FV0
        FMOV.S      FR3,FR8
        FMOV.S      @R4+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R6+,FR2
        FMOV.S      @R7+,FR3
        FIPR        FV4,FV0
        FMOV.S      FR3,FR9
        FMOV.S      @R4+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R6+,FR2
        FMOV.S      @R7+,FR3
        FIPR        FV4,FV0
        ADD         #12,R1
        FMOV.S      FR3,@-R1
        FMOV.S      FR9,@-R1
        FMOV.S      FR8,@-R1
}

/* weighted_sum for angles. each angle is converted to a Float for its FIPR,
 * and each sum is truncated back. size=104 is the body's length, for the
 * reason weighted_sum gives */
/* @asm open: no inspect rule reads FV4 kept across its three FIPR FV4,FV0 */
#pragma inline_asm(weighted_sum_angles(size=104))
static void weighted_sum_angles(Angle *a, Angle *b, Angle *c, Angle *d,
                                Float wa, Float wb, Float wc, Float wd,
                                Angle *out)
{
        MOV.L       @R15,R1
        MOV.L       @R4+,R0
        LDS         R0,FPUL
        FLOAT       FPUL,FR0
        MOV.L       @R5+,R0
        LDS         R0,FPUL
        FLOAT       FPUL,FR1
        MOV.L       @R6+,R0
        LDS         R0,FPUL
        FLOAT       FPUL,FR2
        MOV.L       @R7+,R0
        LDS         R0,FPUL
        FLOAT       FPUL,FR3
        FIPR        FV4,FV0
        FMOV.S      FR3,FR8
        MOV.L       @R4+,R0
        LDS         R0,FPUL
        FLOAT       FPUL,FR0
        MOV.L       @R5+,R0
        LDS         R0,FPUL
        FLOAT       FPUL,FR1
        MOV.L       @R6+,R0
        LDS         R0,FPUL
        FLOAT       FPUL,FR2
        MOV.L       @R7+,R0
        LDS         R0,FPUL
        FLOAT       FPUL,FR3
        FIPR        FV4,FV0
        FMOV.S      FR3,FR9
        MOV.L       @R4,R0
        LDS         R0,FPUL
        FLOAT       FPUL,FR0
        MOV.L       @R5,R0
        LDS         R0,FPUL
        FLOAT       FPUL,FR1
        MOV.L       @R6,R0
        LDS         R0,FPUL
        FLOAT       FPUL,FR2
        MOV.L       @R7,R0
        LDS         R0,FPUL
        FLOAT       FPUL,FR3
        FIPR        FV4,FV0
        ADD         #12,R1
        FTRC        FR3,FPUL
        STS         FPUL,R0
        MOV.L       R0,@-R1
        FTRC        FR9,FPUL
        STS         FPUL,R0
        MOV.L       R0,@-R1
        FTRC        FR8,FPUL
        STS         FPUL,R0
        MOV.L       R0,@-R1
}
#endif
/* clang-format on */

/* interpolate key index's three values along a Catmull-Rom spline, into out.
 * the spline runs through the key before it, the key and the two after it, and
 * t is as link_linear_vector forms it. out[6] to out[8] get the tangent
 * link_blend_cubic_vector reads: the spline's tangents at the key and at the
 * next, blended linearly by t. the tangent at a key is half the step from the
 * key before it to the key after it. the key before the first is the last, and
 * each key past the last is the first */
static void link_cubic_vector(NJS_MKEY_F *keys, Uint32 index, Uint32 count,
                              Float *out)
{
    NJS_MKEY_F *key;
    NJS_MKEY_F *before;
    NJS_MKEY_F *next;
    NJS_MKEY_F *after;
    Float t;
    Float square;
    Float cube;
    Float inverse;
    Float rest;
    Float wa;
    Float wb;
    Float wc;
    Float wd;
    Float x;
    Float y;
    Float z;
    Float term;
    Float *pa;
    Float *pb;
    Float *pc;
    Float *pd;
    Sint32 span;

    key = &keys[index];
    before = &keys[count - 1];
    if (index > 0) {
        before = key - 1;
    }
    /* @hack redundant: a step of keys and out that never runs; it keeps after
     * in keys' register, R4 (MOV R14,R4 at 0x8c654190); without it, after
     * takes R12, the function runs 4 bytes short and 136 of 186 words
     * differ */
    if (0) {
        keys++;
        out++;
    }
    next = keys;
    after = keys;
    if (index + 1 < count) {
        next = key + 1;
    }
    if (index + 2 < count) {
        after = next + 1;
    }
    /* @hack type: the keyframe read through a Sint32 pointer, which gives
     * before R13 (MOV R6,R13 at 0x8c65415a); without it, as
     * (Float)(Sint32)key->keyframe, before takes R14 and 134 of 186 words
     * differ */
    t = _nj_motlink_data_info_[_nj_mot_current_data_].frame -
        (Float)(*(const Sint32 *)&key->keyframe);
    /* @hack spelling: the span in two statements, which puts pa in R11 and pc
     * in R12 (MOV R13,R11 at 0x8c654202, MOV R14,R12 at 0x8c654212); in one, as
     * link_cubic_angles has it, the two trade and 12 of 186 words differ */
    span = next->keyframe;
    span -= key->keyframe;
    if (span > 0) {
        inverse = (Float)span;
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        t *= inverse;
    }
    square = t * t;
    cube = square * t;
    wa = ((2.0F * square) - t - cube) * 0.5F;
    wb = (1.5F * cube) - (2.5F * square) + 1.0F;
    wc = (0.5F * t) - (1.5F * cube) + (2.0F * square);
    wd = (cube - square) * 0.5F;
    pa = before->key;
    pb = key->key;
    pc = next->key;
    pd = after->key;
    weighted_sum(pa, pb, pc, pd, wa, wb, wc, wd, out);
    t *= 0.5F;
    rest = 0.5F - t;
    x = *pc;
    x -= *pa;
    x *= rest;
    term = *pd;
    term -= *pb;
    term *= t;
    x += term;
    pa++;
    pb++;
    pc++;
    pd++;
    y = *pc;
    y -= *pa;
    y *= rest;
    term = *pd;
    term -= *pb;
    term *= t;
    y += term;
    pa++;
    pb++;
    pc++;
    pd++;
    z = *pc;
    z -= *pa;
    z *= rest;
    term = *pd;
    term -= *pb;
    term *= t;
    z += term;
    out += 9;
    *--out = z;
    *--out = y;
    *--out = x;
}

/* link_cubic_vector for angles, each blended as a Float and truncated */
static void link_cubic_angles(NJS_MKEY_A *keys, Uint32 index, Uint32 count,
                              Angle *out)
{
    NJS_MKEY_A *key;
    NJS_MKEY_A *before;
    NJS_MKEY_A *next;
    NJS_MKEY_A *after;
    Float t;
    Float square;
    Float cube;
    Float inverse;
    Float rest;
    Float wa;
    Float wb;
    Float wc;
    Float wd;
    Angle *pa;
    Angle *pb;
    Angle *pc;
    Angle *pd;
    Sint32 span;

    key = &keys[index];
    before = &keys[count - 1];
    if (index > 0) {
        before = key - 1;
    }
    next = keys;
    after = keys;
    if (index + 1 < count) {
        next = key + 1;
    }
    if (index + 2 < count) {
        after = next + 1;
    }
    /* @hack type: the keyframe read through a Sint32 pointer, which keeps key
     * in R7 (MOV R5,R7 at 0x8c6542d6); without it, as
     * (Float)(Sint32)key->keyframe, key takes R14 and 32 of 207 words
     * differ */
    t = _nj_motlink_data_info_[_nj_mot_current_data_].frame -
        (Float)(*(const Sint32 *)&key->keyframe);
    span = next->keyframe - key->keyframe;
    if (span > 0) {
        inverse = (Float)span;
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        t *= inverse;
    }
    square = t * t;
    cube = square * t;
    wa = ((2.0F * square) - t - cube) * 0.5F;
    wb = (1.5F * cube) - (2.5F * square) + 1.0F;
    wc = (0.5F * t) - (1.5F * cube) + (2.0F * square);
    wd = (cube - square) * 0.5F;
    t *= 0.5F;
    rest = 0.5F - t;
    out[6] = (Angle)(((Float)(next->key[0] - before->key[0]) * rest) +
                     ((Float)(after->key[0] - key->key[0]) * t));
    out[7] = (Angle)(((Float)(next->key[1] - before->key[1]) * rest) +
                     ((Float)(after->key[1] - key->key[1]) * t));
    out[8] = (Angle)(((Float)(next->key[2] - before->key[2]) * rest) +
                     ((Float)(after->key[2] - key->key[2]) * t));
    /* @hack spelling: the four keys' values passed through pa to pd; passed
     * directly, SHC saves R10 as well, the function runs 6 bytes long and 187
     * of 207 words differ */
    pa = before->key;
    pb = key->key;
    pc = next->key;
    pd = after->key;
    weighted_sum_angles(pa, pb, pc, pd, wa, wb, wc, wd, out);
}

/* blend the two slots' values into out, rate of the way to slot 1's. slot 0's
 * are values[0] to [2], and slot 1's values[3] to [5] */
static void link_blend_linear_vector(Float *values, Float *out, Float rate)
{
    Float *a;
    Float *b;
    Float rest;
    Float x;
    Float y;
    Float z;
    Float term;

    rest = 1.0F - rate;
    a = values;
    b = values + 3;
    x = *a++;
    x *= rest;
    term = *b++;
    term *= rate;
    x += term;
    y = *a++;
    y *= rest;
    term = *b++;
    term *= rate;
    y += term;
    z = *a;
    z *= rest;
    term = *b;
    term *= rate;
    z += term;
    out += 3;
    *--out = z;
    *--out = y;
    *--out = x;
}

/* link_blend_linear_vector for angles, each blended as a Float and
 * truncated */
/* NOLINTNEXTLINE(readability-non-const-parameter) */
static void link_blend_linear_angles(Angle *values, Angle *out, Float rate)
{
    Float weight = rate;

    /* @hack spelling: slot 1's weight held in weight, a copy of rate, and slot
     * 0's, 1 - rate, in rate itself, which SHC forms in FR4 after copying rate
     * to FR5 and FR3 (FMOV.S FR4,FR5 at 0x8c654494). with rate as slot 1's
     * weight and a local set to 1.0F - rate as slot 0's, the function runs 4
     * bytes short and 40 of 41 words differ */
    rate = 1.0F - rate;
    out[0] = (Angle)(((Float)values[0] * rate) + ((Float)values[3] * weight));
    out[1] = (Angle)(((Float)values[1] * rate) + ((Float)values[4] * weight));
    out[2] = (Angle)(((Float)values[2] * rate) + ((Float)values[5] * weight));
}

/* set out to the cubic Hermite curve between the slots at rate. slot 0's
 * values are values[0] to [2], with the tangent values[6] to [8]. slot 1's are
 * values[3] to [5], with the tangent values[9] to [11] */
static void link_blend_cubic_vector(Float *values, Float *out, Float rate)
{
    Float square;
    Float cube;
    Float w0;
    Float w1;
    Float w2;
    Float w3;

    square = rate * rate;
    cube = square * rate;
    w0 = (2.0F * cube) - (3.0F * square) + 1.0F;
    w1 = (3.0F * square) - (2.0F * cube);
    w2 = rate - (2.0F * square) + cube;
    w3 = cube - square;
    weighted_sum(values, values + 3, values + 6, values + 9, w0, w1, w2, w3,
                 out);
}

/* link_blend_cubic_vector for angles, each blended as a Float and
 * truncated */
static void link_blend_cubic_angles(Angle *values, Angle *out, Float rate)
{
    Float square;
    Float cube;
    Float w0;
    Float w1;
    Float w2;
    Float w3;

    square = rate * rate;
    cube = square * rate;
    w0 = (2.0F * cube) - (3.0F * square) + 1.0F;
    w1 = (3.0F * square) - (2.0F * cube);
    w2 = rate - (2.0F * square) + cube;
    w3 = cube - square;
    weighted_sum_angles(values, values + 3, values + 6, values + 9, w0, w1, w2,
                        w3, out);
}

void njMotionIntTranslateData(NJS_OBJECT *object)
{
    struct nj_motlink_data_info *info =
        &_nj_motlink_data_info_[_nj_mot_current_data_];
    Float *pos = &_nj_motion_link_pos_[_nj_mot_current_data_ * 3];
    Uint32 factor = info->factor;
    NJS_MKEY_F *keys = info->p[factor];
    Uint32 index;

    if (keys != NULL) {
        Uint32 count = info->nb[factor];
        link_find_motion_key(keys, count, &index);
        _nj_motion_int_f_func_(keys, index, count, pos);
        _nj_pos_int_flag_ |= _nj_mot_current_data_ + 1;
    } else {
        Float *from = object->pos;
        Float *to = pos + 3;
        Float x;
        Float y;
        Float z;
        x = *from++;
        y = *from++;
        z = *from;
        *--to = z;
        *--to = y;
        *--to = x;
    }
    info->factor = factor + 1;
}

void njMotionIntScaleData(NJS_OBJECT *object)
{
    struct nj_motlink_data_info *info =
        &_nj_motlink_data_info_[_nj_mot_current_data_];
    Float *scl = &_nj_motion_link_scl_[_nj_mot_current_data_ * 3];
    Uint32 factor = info->factor;
    NJS_MKEY_F *keys = info->p[factor];
    Uint32 index;

    if (keys != NULL) {
        Uint32 count = info->nb[factor];
        link_find_motion_key(keys, count, &index);
        _nj_motion_int_f_func_(keys, index, count, scl);
        _nj_scl_int_flag_ |= _nj_mot_current_data_ + 1;
    } else {
        Float *from = object->scl;
        Float *to = scl + 3;
        Float x;
        Float y;
        Float z;
        x = *from++;
        y = *from++;
        z = *from;
        *--to = z;
        *--to = y;
        *--to = x;
    }
    info->factor = factor + 1;
}

void njMotionIntRotateData(NJS_OBJECT *object)
{
    struct nj_motlink_data_info *info =
        &_nj_motlink_data_info_[_nj_mot_current_data_];
    Angle *ang = &_nj_motion_link_ang_[_nj_mot_current_data_ * 3];
    Uint32 factor = info->factor;
    NJS_MKEY_A *keys = info->p[factor];
    Uint32 index;

    if (keys != NULL) {
        Uint32 count = info->nb[factor];
        link_find_motion_key(keys, count, &index);
        _nj_motion_int_a_func_(keys, index, count, ang);
        _nj_ang_int_flag_ |= _nj_mot_current_data_ + 1;
    } else {
        Angle *from = object->ang;
        ang[0] = from[0];
        ang[1] = from[1];
        ang[2] = from[2];
    }
    /* @hack redundant: a step of info that never runs; it adds a use of info,
     * so factor takes R11 (MOV.L @(20,R14),R11 at 0x8c654788) and count R10;
     * without it, the two trade and 7 of 67 words differ */
    if (0) {
        info++;
    }
    info->factor = factor + 1;
}

/* set data[0] to data[3] to the points or normals of four keys: the one before
 * key index, the key and the two after it. the key before the first is the
 * last, and each key past the last is the first. returns t, the weight of the
 * next key, as link_linear_vector forms it at the frame of the current slot's
 * shape motion */
static Float link_sample_shape(NJS_MKEY_P *keys, Uint32 index, Uint32 count,
                               NJS_POINT3 **data)
{
    NJS_MKEY_P *key;
    NJS_MKEY_P *before;
    NJS_MKEY_P *next;
    Float t;
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
    /* @hack type: the keyframe read through a Sint32 pointer, which gives key
     * R14 (MOV R5,R14 at 0x8c6547ee); without it, as
     * (Float)(Sint32)key->keyframe, key takes R13 and 23 of 64 words differ */
    t = _nj_shapelink_data_info_[_nj_mot_current_data_].frame -
        (Float)(*(const Sint32 *)&key->keyframe);
    span = next->keyframe - key->keyframe;
    if (span > 0) {
        inverse = (Float)span;
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        t *= inverse;
    }
    data[0] = before->key;
    data[1] = key->key;
    data[2] = next->key;
    data[3] = keys->key;
    return t;
}

/* NOLINTNEXTLINE(clang-diagnostic-undefined-internal) */
static Float squared_length(Float x, Float y, Float z, Float w);

/* clang-format off */
#ifndef __clang__
/* returns x * x + y * y + z * z + w * w, by one FIPR. SHC expands the body at
 * each call. size=4 is its length in bytes, for the reason weighted_sum
 * gives */
/* @asm operand: fipr-registers, each FIPR FV4,FV4, at 0x8c65489e in
 * link_linear_normal, 0x8c6549d4 in link_cubic_normal and 0x8c654b1a in
 * link_bezier_normal, squares the blended normal in the registers it is in */
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
static void link_linear_normal(Float *a, Float *b, Float *out, Float t)
{
    Float rest;
    Float x;
    Float y;
    Float z;
    Float term;
    Float length;

    rest = 1.0F - t;
    /* @hack spelling: Float pointers walk the a, b and out points' x, y and z,
     * which gives the FMOV.S @R4+ and @R5+ loads (0x8c65487a to 0x8c654892) and
     * the FMOV.S @-R14 stores (0x8c6548be to 0x8c6548c2); with NJS_POINT3
     * parameters read and stored by member, 40 of 50 words differ and the
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
static void link_linear_point(Float *a, Float *b, Float *out, Float t)
{
    Float rest;
    Float x;
    Float y;
    Float z;
    Float term;

    /* @hack spelling: Float pointers walk the a, b and out points' x, y and z,
     * which gives the FMOV.S @R4+ and @R5+ loads (0x8c6548ce to 0x8c6548ec) and
     * the FMOV.S @-R6 stores (0x8c6548f2 to 0x8c6548f8); with NJS_POINT3
     * parameters read and stored by member, 18 of 25 words differ and the
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

/* set out to the Catmull-Rom spline through a, b, c and d at t, which runs
 * from b at 0 to c at 1. out is scaled to unit length as link_linear_normal
 * scales it */
static void link_cubic_normal(Float *a, Float *b, Float *c, Float *d,
                              Float *out, Float t)
{
    Float square;
    Float cube;
    Float wa;
    Float wb;
    Float wc;
    Float wd;
    Float x;
    Float y;
    Float z;
    Float length;

    square = t * t;
    cube = square * t;
    wa = ((2.0F * square) - t - cube) * 0.5F;
    wb = (1.5F * cube) - (2.5F * square) + 1.0F;
    wc = (0.5F * t) - (1.5F * cube) + (2.0F * square);
    wd = (cube - square) * 0.5F;
    weighted_sum(a, b, c, d, wa, wb, wc, wd, out);
    /* @hack spelling: a Float pointer walks the out point's x, y and z up, and
     * out steps back down from past z, which gives the FMOV.S @R14+ loads
     * (0x8c6549c8 to 0x8c6549cc) and the FMOV.S @-R14 stores (0x8c6549f2 to
     * 0x8c6549f6); with an NJS_POINT3 out read and stored by member, 66 of 106
     * words differ and the section is 8 bytes longer */
    x = *out++;
    y = *out++;
    z = *out++;
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
    *--out = z;
    *--out = y;
    *--out = x;
}

/* set out to the Catmull-Rom spline through a, b, c and d at t, which runs
 * from b at 0 to c at 1 */
static void link_cubic_point(Float *a, Float *b, Float *c, Float *d, Float *out,
                             Float t)
{
    Float square;
    Float cube;
    Float wa;
    Float wb;
    Float wc;
    Float wd;

    square = t * t;
    cube = square * t;
    wa = ((2.0F * square) - t - cube) * 0.5F;
    wb = (1.5F * cube) - (2.5F * square) + 1.0F;
    wc = (0.5F * t) - (1.5F * cube) + (2.0F * square);
    wd = (cube - square) * 0.5F;
    weighted_sum(a, b, c, d, wa, wb, wc, wd, out);
}

/* set out to the cubic Bezier curve from a to b at t. c is the control point
 * after a, and d the one before b. out is scaled to unit length as
 * link_linear_normal scales it */
static void link_bezier_normal(Float *a, Float *b, Float *c, Float *d,
                               Float *out, Float t)
{
    Float rest;
    Float w0;
    Float w1;
    Float w2;
    Float w3;
    Float x;
    Float y;
    Float z;
    Float length;

    rest = 1.0F - t;
    w0 = rest * rest * rest;
    w2 = t * rest * rest * 3.0F;
    w3 = t * t * rest * 3.0F;
    w1 = t * t * t;
    weighted_sum(a, b, c, d, w0, w1, w2, w3, out);
    /* @hack spelling: a Float pointer walks the out point's x, y and z up, and
     * out steps back down from past z, which gives the FMOV.S @R14+ loads
     * (0x8c654b0e to 0x8c654b12) and the FMOV.S @-R14 stores (0x8c654b38 to
     * 0x8c654b3c); with an NJS_POINT3 out read and stored by member, 60 of 91
     * words differ and the section is 12 bytes longer */
    x = *out++;
    y = *out++;
    z = *out++;
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
    *--out = z;
    *--out = y;
    *--out = x;
}

/* set out to the cubic Bezier curve from a to b at t. c is the control point
 * after a, and d the one before b */
static void link_bezier_point(Float *a, Float *b, Float *c, Float *d,
                              Float *out, Float t)
{
    Float rest;
    Float w0;
    Float w1;
    Float w2;
    Float w3;

    rest = 1.0F - t;
    w0 = rest * rest * rest;
    w2 = t * rest * rest * 3.0F;
    w3 = t * t * rest * 3.0F;
    w1 = t * t * t;
    weighted_sum(a, b, c, d, w0, w1, w2, w3, out);
}

/* where the current slot's shape factor has keys, fill data and *t as
 * link_sample_shape does. set the slot's bit in _nj_pnt_int_flag_, and *found
 * to its _nj_motlink_shape_data_cnt_. point _nj_shape_link_point_[slot] at
 * buffer, which the morph then fills. point _nj_shape_link_point_[slot + 2],
 * the slot's Bezier control point for njMakeShapeLink, at the next key's
 * points for slot 0 and at the key's own for slot 1 */
static void link_get_cubic_points(NJS_POINT3 *buffer, Uint32 *found, Float *t,
                                  NJS_POINT3 **data)
{
    struct nj_motlink_data_info *info =
        &_nj_shapelink_data_info_[_nj_mot_current_data_];
    /* @hack spelling: factor declared before link, so factor takes R6 and its
     * offset R5 (MOV.L @(20,R4),R6 at 0x8c654bea); without it, with link
     * first, the two trade and 7 of 100 words differ */
    Uint32 factor = info->factor;
    NJS_POINT3 **link = &_nj_shape_link_point_[_nj_mot_current_data_];
    NJS_MKEY_P *keys = info->p[factor];
    Uint32 index;

    if (keys != NULL) {
        Uint32 count = info->nb[factor];
        link_find_shape_motion_key(keys, count, &index);
        *t = link_sample_shape(keys, index, count, data);
        link[0] = buffer;
        link[2] = data[2 - _nj_mot_current_data_];
        *found = _nj_motlink_shape_data_cnt_[_nj_mot_current_data_];
        _nj_pnt_int_flag_ |= _nj_mot_current_data_ + 1;
    }
}

/* link_get_cubic_points for normals */
static void link_get_cubic_normals(NJS_POINT3 *buffer, Uint32 *found, Float *t,
                                   NJS_POINT3 **data)
{
    struct nj_motlink_data_info *info =
        &_nj_shapelink_data_info_[_nj_mot_current_data_];
    /* @hack spelling: factor declared before link, as in link_get_cubic_points;
     * with link first, 7 of 75 words differ */
    Uint32 factor = info->factor;
    NJS_VECTOR **link = &_nj_shape_link_normal_[_nj_mot_current_data_];
    NJS_MKEY_P *keys = info->p[factor];
    Uint32 index;

    if (keys != NULL) {
        Uint32 count = info->nb[factor];
        link_find_shape_motion_key(keys, count, &index);
        *t = link_sample_shape(keys, index, count, data);
        link[0] = buffer;
        link[2] = data[2 - _nj_mot_current_data_];
        *found = _nj_motlink_shape_data_cnt_[_nj_mot_current_data_];
        _nj_nml_int_flag_ |= _nj_mot_current_data_ + 1;
    }
}

/* link_get_cubic_points for a shape motion of SHAPE_BOTH, which adds
 * SHAPE_POINTS to *found rather than setting it */
static void link_get_cubic_both_points(NJS_POINT3 *buffer, Uint32 *found,
                                       Float *t, NJS_POINT3 **data)
{
    struct nj_motlink_data_info *info =
        &_nj_shapelink_data_info_[_nj_mot_current_data_];
    /* @hack spelling: factor declared before link, as in link_get_cubic_points;
     * with link first, 7 of 71 words differ */
    Uint32 factor = info->factor;
    NJS_POINT3 **link = &_nj_shape_link_point_[_nj_mot_current_data_];
    NJS_MKEY_P *keys = info->p[factor];
    Uint32 index;

    if (keys != NULL) {
        Uint32 count = info->nb[factor];
        link_find_shape_motion_key(keys, count, &index);
        *t = link_sample_shape(keys, index, count, data);
        link[0] = buffer;
        link[2] = data[2 - _nj_mot_current_data_];
        _nj_pnt_int_flag_ |= _nj_mot_current_data_ + 1;
        *found += SHAPE_POINTS;
    }
}

/* link_get_cubic_normals for a shape motion of SHAPE_BOTH, which adds
 * SHAPE_NORMALS to *found rather than setting it */
static void link_get_cubic_both_normals(NJS_POINT3 *buffer, Uint32 *found,
                                        Float *t, NJS_POINT3 **data)
{
    struct nj_motlink_data_info *info =
        &_nj_shapelink_data_info_[_nj_mot_current_data_];
    /* @hack spelling: factor declared before link, as in link_get_cubic_points;
     * with link first, 7 of 71 words differ */
    Uint32 factor = info->factor;
    NJS_VECTOR **link = &_nj_shape_link_normal_[_nj_mot_current_data_];
    NJS_MKEY_P *keys = info->p[factor];
    Uint32 index;

    if (keys != NULL) {
        Uint32 count = info->nb[factor];
        link_find_shape_motion_key(keys, count, &index);
        *t = link_sample_shape(keys, index, count, data);
        link[0] = buffer;
        link[2] = data[2 - _nj_mot_current_data_];
        _nj_nml_int_flag_ |= _nj_mot_current_data_ + 1;
        *found += SHAPE_NORMALS;
    }
}

/* set count points of out by link_cubic_point, each from the same point of
 * data[0] to data[3], at t. count must be at least 1 */
static void link_cubic_morph_points(NJS_POINT3 **data, Sint32 count, Float t,
                                    NJS_POINT3 *out)
{
    NJS_POINT3 *a = data[0];
    NJS_POINT3 *b = data[1];
    NJS_POINT3 *c = data[2];
    NJS_POINT3 *d = data[3];
    Sint32 left = count;

    do {
        link_cubic_point(&a->x, &b->x, &c->x, &d->x, &out->x, t);
        a++;
        b++;
        c++;
        d++;
        out++;
    } while (--left != 0);
}

/* link_cubic_morph_points for normals, each set by link_cubic_normal */
static void link_cubic_morph_normals(NJS_POINT3 **data, Sint32 count, Float t,
                                     NJS_POINT3 *out)
{
    NJS_POINT3 *a = data[0];
    NJS_POINT3 *b = data[1];
    NJS_POINT3 *c = data[2];
    NJS_POINT3 *d = data[3];
    Sint32 left = count;

    do {
        link_cubic_normal(&a->x, &b->x, &c->x, &d->x, &out->x, t);
        a++;
        b++;
        c++;
        d++;
        out++;
    } while (--left != 0);
}

/* the current slot's cubic shape step. it points the slot at the points and
 * normals of the node's model. where the slot's shape motion has keys for the
 * points, the normals or both, it morphs them into the slot's part of
 * _nj_shape_buf_ instead. for a model of n points, that part holds n points
 * and then n normals, from slot * 6n Floats in. njMakeShapeLink writes its
 * blend from 12n on. a null model, or one under NJD_EVAL_HIDE, is left alone */
static void link_cubic_shape(NJS_OBJECT *object)
{
    NJS_MODEL *model = object->model;
    NJS_POINT3 *points;
    NJS_POINT3 *normals;
    Sint32 count;
    Sint32 offset;
    Uint32 found;
    Float point_t;
    Float normal_t;
    NJS_POINT3 *point_data[4];
    NJS_POINT3 *normal_data[4];

    if (model != NULL && !(object->evalflags & NJD_EVAL_HIDE)) {
        found = 0;
        count = model->nbPoint;
        /* @hack spelling: the buffer indexed as floats across its NJS_POINT3
         * members, which scales the offset by 6 and then by a float's 4 bytes
         * (0x8c654f0c to 0x8c654f18); as NJS_POINT3 indexes, 128 of 152 words
         * differ and the section is 4 bytes shorter */
        offset = _nj_mot_current_data_ * count * 6;
        points = (NJS_POINT3 *)&_nj_shape_buf_[offset];
        normals = (NJS_POINT3 *)&_nj_shape_buf_[(count * 3) + offset];
        _nj_shape_link_point_[_nj_mot_current_data_] = model->points;
        _nj_shape_link_normal_[_nj_mot_current_data_] = model->normals;
        switch (_nj_motlink_shape_data_cnt_[_nj_mot_current_data_]) {
        case SHAPE_POINTS:
            link_get_cubic_points(points, &found, &point_t, point_data);
            break;
        case SHAPE_NORMALS:
            link_get_cubic_normals(normals, &found, &normal_t, normal_data);
            break;
        case SHAPE_BOTH:
            link_get_cubic_both_points(points, &found, &point_t, point_data);
            _nj_shapelink_data_info_[_nj_mot_current_data_].factor++;
            link_get_cubic_both_normals(normals, &found, &normal_t,
                                        normal_data);
            break;
        }
        switch (found) {
        case SHAPE_POINTS:
            link_cubic_morph_points(point_data, count, point_t, points);
            break;
        case SHAPE_BOTH:
            link_cubic_morph_points(point_data, count, point_t, points);
            link_cubic_morph_normals(normal_data, count, normal_t, normals);
            break;
        case SHAPE_NORMALS:
            link_cubic_morph_normals(normal_data, count, normal_t, normals);
            break;
        }
    }
}

/* link_get_cubic_points for a linear shape motion, which needs no control
 * point */
static void link_get_linear_points(Uint32 *found, Float *t, NJS_POINT3 **data,
                                   NJS_POINT3 *buffer)
{
    struct nj_motlink_data_info *info =
        &_nj_shapelink_data_info_[_nj_mot_current_data_];
    Uint32 factor = info->factor;
    NJS_MKEY_P *keys = info->p[factor];
    Uint32 index;

    if (keys != NULL) {
        Uint32 count = info->nb[factor];
        link_find_shape_motion_key(keys, count, &index);
        *t = link_sample_shape(keys, index, count, data);
        _nj_shape_link_point_[_nj_mot_current_data_] = buffer;
        _nj_pnt_int_flag_ |= _nj_mot_current_data_ + 1;
        *found = _nj_motlink_shape_data_cnt_[_nj_mot_current_data_];
    }
}

/* link_get_linear_points for normals */
static void link_get_linear_normals(Uint32 *found, Float *t, NJS_POINT3 **data,
                                    NJS_POINT3 *buffer)
{
    struct nj_motlink_data_info *info =
        &_nj_shapelink_data_info_[_nj_mot_current_data_];
    Uint32 factor = info->factor;
    NJS_MKEY_P *keys = info->p[factor];
    Uint32 index;

    if (keys != NULL) {
        Uint32 count = info->nb[factor];
        link_find_shape_motion_key(keys, count, &index);
        *t = link_sample_shape(keys, index, count, data);
        _nj_shape_link_normal_[_nj_mot_current_data_] = buffer;
        _nj_nml_int_flag_ |= _nj_mot_current_data_ + 1;
        *found = _nj_motlink_shape_data_cnt_[_nj_mot_current_data_];
    }
}

/* link_get_linear_points for a shape motion of SHAPE_BOTH, which adds
 * SHAPE_POINTS to *found rather than setting it */
static void link_get_linear_both_points(Uint32 *found, Float *t,
                                        NJS_POINT3 **data, NJS_POINT3 *buffer)
{
    struct nj_motlink_data_info *info =
        &_nj_shapelink_data_info_[_nj_mot_current_data_];
    Uint32 factor = info->factor;
    NJS_MKEY_P *keys = info->p[factor];
    Uint32 index;

    if (keys != NULL) {
        Uint32 count = info->nb[factor];
        link_find_shape_motion_key(keys, count, &index);
        *t = link_sample_shape(keys, index, count, data);
        _nj_shape_link_point_[_nj_mot_current_data_] = buffer;
        _nj_pnt_int_flag_ |= _nj_mot_current_data_ + 1;
        *found += SHAPE_POINTS;
    }
}

/* link_get_linear_normals for a shape motion of SHAPE_BOTH, which adds
 * SHAPE_NORMALS to *found rather than setting it */
static void link_get_linear_both_normals(Uint32 *found, Float *t,
                                         NJS_POINT3 **data, NJS_POINT3 *buffer)
{
    struct nj_motlink_data_info *info =
        &_nj_shapelink_data_info_[_nj_mot_current_data_];
    Uint32 factor = info->factor;
    NJS_MKEY_P *keys = info->p[factor];
    Uint32 index;

    if (keys != NULL) {
        Uint32 count = info->nb[factor];
        link_find_shape_motion_key(keys, count, &index);
        *t = link_sample_shape(keys, index, count, data);
        _nj_shape_link_normal_[_nj_mot_current_data_] = buffer;
        _nj_nml_int_flag_ |= _nj_mot_current_data_ + 1;
        *found += SHAPE_NORMALS;
    }
}

/* set count points of out by link_linear_point, each from the same point of
 * data[1] and data[2], at t. count must be at least 1 */
static void link_linear_morph_points(Sint32 count, Float t, NJS_POINT3 *out,
                                     NJS_POINT3 **data)
{
    NJS_POINT3 *a = data[1];
    NJS_POINT3 *b = data[2];
    Sint32 left = count;

    do {
        link_linear_point(&a->x, &b->x, &out->x, t);
        a++;
        b++;
        out++;
    } while (--left != 0);
}

/* link_linear_morph_points for normals, each set by link_linear_normal */
static void link_linear_morph_normals(Sint32 count, Float t, NJS_POINT3 *out,
                                      NJS_POINT3 **data)
{
    NJS_POINT3 *a = data[1];
    NJS_POINT3 *b = data[2];
    Sint32 left = count;

    do {
        link_linear_normal(&a->x, &b->x, &out->x, t);
        a++;
        b++;
        out++;
    } while (--left != 0);
}

/* the current slot's linear shape step: link_cubic_shape, but blending each
 * point and normal linearly from the key's to the next key's */
static void link_linear_shape(NJS_OBJECT *object)
{
    NJS_MODEL *model = object->model;
    NJS_POINT3 *points;
    NJS_POINT3 *normals;
    Sint32 count;
    Sint32 offset;
    Uint32 found;
    Float point_t;
    Float normal_t;
    NJS_POINT3 *point_data[4];
    NJS_POINT3 *normal_data[4];

    if (model != NULL && !(object->evalflags & NJD_EVAL_HIDE)) {
        found = 0;
        count = model->nbPoint;
        /* @hack spelling: the buffer indexed as floats across its NJS_POINT3
         * members, which scales the offset by 6 and then by a float's 4 bytes
         * (0x8c6552f6 to 0x8c655300); as NJS_POINT3 indexes, 135 of 150 words
         * differ and the section is 8 bytes shorter */
        offset = _nj_mot_current_data_ * count * 6;
        points = (NJS_POINT3 *)&_nj_shape_buf_[offset];
        normals = (NJS_POINT3 *)&_nj_shape_buf_[(count * 3) + offset];
        _nj_shape_link_point_[_nj_mot_current_data_] = model->points;
        _nj_shape_link_normal_[_nj_mot_current_data_] = model->normals;
        switch (_nj_motlink_shape_data_cnt_[_nj_mot_current_data_]) {
        case SHAPE_POINTS:
            link_get_linear_points(&found, &point_t, point_data, points);
            break;
        case SHAPE_NORMALS:
            link_get_linear_normals(&found, &normal_t, normal_data, normals);
            break;
        case SHAPE_BOTH:
            link_get_linear_both_points(&found, &point_t, point_data, points);
            _nj_shapelink_data_info_[_nj_mot_current_data_].factor++;
            link_get_linear_both_normals(&found, &normal_t, normal_data,
                                         normals);
            break;
        }
        switch (found) {
        case SHAPE_POINTS:
            link_linear_morph_points(count, point_t, points, point_data);
            break;
        case SHAPE_BOTH:
            link_linear_morph_points(count, point_t, points, point_data);
            link_linear_morph_normals(count, normal_t, normals, normal_data);
            break;
        case SHAPE_NORMALS:
            link_linear_morph_normals(count, normal_t, normals, normal_data);
            break;
        }
    }
}

void njMotionObjTranslateData(NJS_OBJECT *object)
{
    Float *pos = &_nj_motion_link_pos_[_nj_mot_current_data_ * 3] + 3;
    Float *from = object->pos;
    Float x;
    Float y;
    Float z;

    x = *from++;
    y = *from++;
    z = *from;
    *--pos = z;
    *--pos = y;
    *--pos = x;
}

void njMotionObjRotateData(NJS_OBJECT *object)
{
    Angle *ang = &_nj_motion_link_ang_[_nj_mot_current_data_ * 3];
    Angle *from = object->ang;
    Angle x;
    Angle y;
    Angle z;

    x = from[0];
    y = from[1];
    z = from[2];
    ang[0] = x;
    ang[1] = y;
    ang[2] = z;
}

void njMotionObjScaleData(NJS_OBJECT *object)
{
    Float *scl = &_nj_motion_link_scl_[_nj_mot_current_data_ * 3] + 3;
    Float *from = object->scl;
    Float x;
    Float y;
    Float z;

    x = *from++;
    y = *from++;
    z = *from;
    *--scl = z;
    *--scl = y;
    *--scl = x;
}

/* link_motion_node_step_0 to link_motion_node_step_4, the node steps, by the
 * slot's factor count. each runs the current slot's position, angle and scale
 * steps on the node. with factors, a step first points the slot at the node's
 * record, an NJS_MDATA1 to NJS_MDATA4 by the count, and after the three steps
 * moves the slot's mdata to the next node's record */
static void link_motion_node_step_0(NJS_OBJECT *object)
{
    _nj_motion_translate_data_[_nj_mot_current_data_](object);
    _nj_motion_rotate_data_[_nj_mot_current_data_](object);
    _nj_motion_scale_data_[_nj_mot_current_data_](object);
}

static void link_motion_node_step_1(NJS_OBJECT *object)
{
    struct nj_motlink_data_info *info =
        &_nj_motlink_data_info_[_nj_mot_current_data_];
    NJS_MDATA1 *record = info->mdata;

    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    _nj_motion_translate_data_[_nj_mot_current_data_](object);
    _nj_motion_rotate_data_[_nj_mot_current_data_](object);
    _nj_motion_scale_data_[_nj_mot_current_data_](object);
    info->mdata = record + 1;
}

static void link_motion_node_step_2(NJS_OBJECT *object)
{
    struct nj_motlink_data_info *info =
        &_nj_motlink_data_info_[_nj_mot_current_data_];
    NJS_MDATA2 *record = info->mdata;

    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    _nj_motion_translate_data_[_nj_mot_current_data_](object);
    _nj_motion_rotate_data_[_nj_mot_current_data_](object);
    _nj_motion_scale_data_[_nj_mot_current_data_](object);
    info->mdata = record + 1;
}

static void link_motion_node_step_3(NJS_OBJECT *object)
{
    struct nj_motlink_data_info *info =
        &_nj_motlink_data_info_[_nj_mot_current_data_];
    NJS_MDATA3 *record = info->mdata;

    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    _nj_motion_translate_data_[_nj_mot_current_data_](object);
    _nj_motion_rotate_data_[_nj_mot_current_data_](object);
    _nj_motion_scale_data_[_nj_mot_current_data_](object);
    info->mdata = record + 1;
}

static void link_motion_node_step_4(NJS_OBJECT *object)
{
    struct nj_motlink_data_info *info =
        &_nj_motlink_data_info_[_nj_mot_current_data_];
    NJS_MDATA4 *record = info->mdata;

    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    _nj_motion_translate_data_[_nj_mot_current_data_](object);
    _nj_motion_rotate_data_[_nj_mot_current_data_](object);
    _nj_motion_scale_data_[_nj_mot_current_data_](object);
    info->mdata = record + 1;
}

/* link_shape_node_step_0 to link_shape_node_step_2, the shape node steps, by
 * what the slot's shape motion moves. with neither points nor normals, the
 * current slot uses those of the node's model, unless the model is null or
 * under NJD_EVAL_HIDE. otherwise a step points the slot at the node's record,
 * an NJS_MDATA1 for one part or an NJS_MDATA2 for both. it runs the slot's
 * shape step, then moves the slot's mdata to the next node's record */
static void link_shape_node_step_0(NJS_OBJECT *object)
{
    NJS_MODEL *model = object->model;

    if (model != NULL && !(object->evalflags & NJD_EVAL_HIDE)) {
        _nj_shape_link_point_[_nj_mot_current_data_] = model->points;
        _nj_shape_link_normal_[_nj_mot_current_data_] = model->normals;
    }
}

static void link_shape_node_step_1(NJS_OBJECT *object)
{
    struct nj_motlink_data_info *info =
        &_nj_shapelink_data_info_[_nj_mot_current_data_];
    NJS_MDATA1 *record = info->mdata;

    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    _nj_motion_shape_data_[_nj_mot_current_data_](object);
    info->mdata = record + 1;
}

static void link_shape_node_step_2(NJS_OBJECT *object)
{
    struct nj_motlink_data_info *info =
        &_nj_shapelink_data_info_[_nj_mot_current_data_];
    NJS_MDATA2 *record = info->mdata;

    info->p = record->p;
    info->nb = record->nb;
    info->factor = 0;
    _nj_motion_shape_data_[_nj_mot_current_data_](object);
    info->mdata = record + 1;
}

void njMakeMotionLink(void)
{
    Float pos[3];
    Float scl[3];
    Angle ang[3];
    Float half;
    /* BOTH_SLOTS in a cubic link and 0 in a linear one, so a flag above 0 and
     * below it is one slot's alone, in a cubic link */
    Uint32 limit = _nj_mot_spline_cnt_ * BOTH_SLOTS;
    Uint32 flag;
    Float *src;
    Float *b;
    Float *dst;
    Angle *asrc;
    Angle *ab;
    Angle *adst;
    Float x;
    Float y;
    Float z;
    Angle ax;
    Angle ay;
    Angle az;

    flag = _nj_pos_int_flag_;
    /* @hack spelling: half and src set in the test, so half's load, FMOV.S
     * @R0,FR15, fills the BF/S slot at 0x8c655734; without it, as two
     * statements in that order between flag's load and the test, ADD #24,R11
     * fills it and 10 of 274 words differ */
    if ((half = 0.5F, src = _nj_motion_link_pos_, flag > 0)) {
        /* in a cubic link where one slot alone has keys, the other slot's
         * tangent is half the step from slot 0's values to slot 1's */
        if (flag < limit) {
            flag = flag - 1;
            flag = flag ^ 1;
            dst = &src[(flag * 3) + 6] + 3;
            b = src + 3;
            x = *b++;
            x -= *src++;
            x *= half;
            y = *b++;
            /* @hack spelling: y's difference spelled y = y - *src++, which
             * reads y into FR4 and z into FR5 (FMOV.S @R7+,FR4 at 0x8c65575c);
             * as y -= *src++, the two trade and 8 of 274 words differ */
            y = y - *src++;
            y *= half;
            z = *b;
            z -= *src;
            z *= half;
            *--dst = z;
            *--dst = y;
            *--dst = x;
            _nj_pos_int_flag_ = flag;
        }
        _nj_int_f_func_(_nj_motion_link_pos_, pos, _nj_mot_current_frame_);
    } else {
        dst = pos + 3;
        b = src;
        x = *b++;
        y = *b++;
        z = *b;
        *--dst = z;
        *--dst = y;
        *--dst = x;
    }
    flag = _nj_ang_int_flag_;
    asrc = _nj_motion_link_ang_;
    if (flag > 0) {
        if (flag < limit) {
            flag = flag - 1;
            flag = flag ^ 1;
            adst = &asrc[(flag * 3) + 6];
            ab = asrc + 3;
            x = (Float)ab[0];
            x -= (Float)asrc[0];
            x *= half;
            adst[0] = (Angle)x;
            x = (Float)ab[1];
            x -= (Float)asrc[1];
            x *= half;
            adst[1] = (Angle)x;
            x = (Float)ab[2];
            x -= (Float)asrc[2];
            x *= half;
            adst[2] = (Angle)x;
            _nj_ang_int_flag_ = flag;
        }
        _nj_int_a_func_(_nj_motion_link_ang_, ang, _nj_mot_current_frame_);
    } else {
        adst = asrc;
        ang[0] = adst[0];
        ang[1] = adst[1];
        ang[2] = adst[2];
        /* @hack redundant: a step of adst and a store through it that never
         * run; the second definition keeps adst apart from asrc, so the
         * copies read through R4 (MOV R5,R4 at 0x8c655864); without it, they
         * read through asrc in R14, the function runs 2 bytes short and 146 of
         * 274 words differ */
        if (0) {
            adst++;
            *adst = 0;
        }
    }
    src = _nj_motion_link_scl_;
    flag = _nj_scl_int_flag_;
    if (flag > 0) {
        if (flag < limit) {
            flag = flag - 1;
            flag = flag ^ 1;
            dst = &src[(flag * 3) + 6] + 3;
            b = src + 3;
            x = *b++;
            x -= *src++;
            x *= half;
            y = *b++;
            y -= *src++;
            y *= half;
            z = *b;
            z -= *src;
            z *= half;
            *--dst = z;
            *--dst = y;
            *--dst = x;
            _nj_scl_int_flag_ = flag;
        }
        _nj_int_f_func_(_nj_motion_link_scl_, scl, _nj_mot_current_frame_);
    } else {
        x = *src++;
        y = *src++;
        z = *src;
        /* @hack redundant: a step of y that never runs; it adds a use of y,
         * so y takes FR4 and z FR5 (FMOV.S @R5+,FR4 at 0x8c6558da); without
         * it, they trade and 4 of 274 words differ */
        if (0) {
            y++;
        }
        dst = scl + 3;
        *--dst = z;
        *--dst = y;
        *--dst = x;
    }
    src = pos;
    /* @hack redundant: a copy of pos that never runs; it adds a use of pos,
     * so pos takes R11 and &_nj_mot_current_frame_ R10 (0x8c655718 and
     * 0x8c65570e); without it, pos takes R9 and 91 of 274 words differ */
    if (0) {
        src = pos;
    }
    x = *src++;
    /* @hack spelling: src stepped past y as src = src + 1, a statement of its
     * own, which puts b in R7 and dst in R6 in the position's tangent step
     * (MOV R5,R7 at 0x8c65574a); as y = *src++, the two trade and 11 of 274
     * words differ */
    y = *src;
    src = src + 1;
    z = *src;
    njTranslate(NULL, x, y, z);
    ax = ang[0];
    ay = ang[1];
    az = ang[2];
    _nj_rotate_func_(NULL, ax, ay, az);
    src = scl;
    x = *src++;
    y = *src++;
    z = *src;
    njScale(NULL, x, y, z);
}

void njMakeShapeLink(NJS_OBJECT *object)
{
    NJS_MODEL *model = object->model;
    NJS_POINT3 *points;
    NJS_VECTOR *normals;
    Float *buffer;
    NJS_POINT3 *p;
    NJS_POINT3 *n;
    NJS_POINT3 *a;
    NJS_POINT3 *b;
    NJS_POINT3 *c;
    NJS_POINT3 *d;
    Sint32 count;
    Sint32 limit;
    Sint32 other;
    Sint32 i;

    if (model == NULL || (object->evalflags & NJD_EVAL_HIDE)) {
        return;
    }
    points = model->points;
    normals = model->normals;
    if ((Sint32)_nj_pnt_int_flag_ > 0 && (Sint32)_nj_nml_int_flag_ > 0) {
        /* BOTH_SLOTS in a cubic link and 0 in a linear one */
        limit = _nj_mot_shape_spline_cnt_ * BOTH_SLOTS;
        /* in a cubic link where one slot alone has keys for the points, or
         * for the normals, the other slot's control point becomes the keyed
         * slot's */
        if ((Sint32)_nj_pnt_int_flag_ < limit) {
            _nj_pnt_int_flag_--;
            other = 1 - _nj_pnt_int_flag_;
            _nj_shape_link_point_[other + 2] =
                _nj_shape_link_point_[_nj_pnt_int_flag_ + 2];
        }
        if ((Sint32)_nj_nml_int_flag_ < limit) {
            _nj_nml_int_flag_--;
            other = 1 - _nj_nml_int_flag_;
            _nj_shape_link_normal_[other + 2] =
                _nj_shape_link_normal_[_nj_nml_int_flag_ + 2];
        }
        /* @hack spelling: the buffer read in the test, so its load, MOV.L
         * @R3,R6, fills the BF/S slot at 0x8c6559d4; without it, as a
         * statement before the test, MOV R7,R4 fills it and 9 of 265 words
         * differ */
        if ((buffer = _nj_shape_buf_, limit > 0)) {
            a = _nj_shape_link_point_[0];
            b = _nj_shape_link_point_[1];
            c = _nj_shape_link_point_[2];
            d = _nj_shape_link_point_[3];
            count = model->nbPoint;
            /* @hack spelling: the buffer indexed as floats across its
             * NJS_POINT3 members, here and in the three offsets below, which
             * scales count by 12 and by 15 floats (0x8c6559e8 to 0x8c6559ee,
             * 0x8c655a32 to 0x8c655a38); as NJS_POINT3 indexes, 130 of 233
             * words differ and the section is 4 bytes longer */
            p = (NJS_POINT3 *)&buffer[count * 12];
            i = count;
            do {
                link_bezier_point(&a->x, &b->x, &c->x, &d->x, &p->x,
                                  _nj_mot_current_frame_);
                a++;
                b++;
                c++;
                d++;
                p++;
            } while (--i != 0);
            a = _nj_shape_link_normal_[0];
            b = _nj_shape_link_normal_[1];
            c = _nj_shape_link_normal_[2];
            d = _nj_shape_link_normal_[3];
            count = model->nbPoint;
            n = (NJS_POINT3 *)&_nj_shape_buf_[count * 15];
            i = count;
            do {
                link_bezier_normal(&a->x, &b->x, &c->x, &d->x, &n->x,
                                   _nj_mot_current_frame_);
                a++;
                b++;
                c++;
                d++;
                n++;
            } while (--i != 0);
        } else {
            a = _nj_shape_link_point_[0];
            b = _nj_shape_link_point_[1];
            count = model->nbPoint;
            p = (NJS_POINT3 *)&buffer[count * 12];
            i = count;
            do {
                link_linear_point(&a->x, &b->x, &p->x, _nj_mot_current_frame_);
                a++;
                b++;
                p++;
            } while (--i != 0);
            a = _nj_shape_link_normal_[0];
            b = _nj_shape_link_normal_[1];
            count = model->nbPoint;
            n = (NJS_POINT3 *)&_nj_shape_buf_[count * 15];
            i = count;
            do {
                link_linear_normal(&a->x, &b->x, &n->x, _nj_mot_current_frame_);
                a++;
                b++;
                n++;
            } while (--i != 0);
        }
        /* @bug memory: the model takes p and n after the loops have
         * stepped each past its blend (MOV.L at 0x8c655ac8 and 0x8c655aca).
         * p then points at the blended normals, which the model is drawn with
         * as its points, and n at the nbPoint vectors past both blends, which
         * it is drawn with as its normals */
        model->points = p;
        model->normals = n;
    }
    _nj_draw_model_sub_(model);
    model->points = points;
    model->normals = normals;
}

/* link_linear_angles, but each angle of the next key is first moved a turn
 * toward key index's where the two are more than half a turn apart, so the
 * blend takes the short way round */
static void link_linear_angles_short(NJS_MKEY_A *keys, Uint32 index,
                                     Uint32 count, Angle *out)
{
    NJS_MKEY_A *key = &keys[index];
    NJS_MKEY_A *next = keys;
    Sint32 span;
    Sint32 x;
    Sint32 y;
    Sint32 z;
    Float t;
    Float rest;
    Float inverse;

    if (++index < count) {
        next = key + 1;
    }
    /* @hack type: the keyframe read as in link_linear_vector; without it, as
     * (Float)(Sint32)key->keyframe, SHC saves R11 as well, the function runs
     * 4 bytes long and 108 of 122 words differ */
    t = _nj_motlink_data_info_[_nj_mot_current_data_].frame -
        (Float)(*(const Sint32 *)&key->keyframe);
    span = next->keyframe - key->keyframe;
    if (span > 0) {
        inverse = (Float)span;
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        t *= inverse;
    }
    rest = 1.0F - t;
    x = next->key[0] - key->key[0];
    if (x > HALF_TURN) {
        x -= TURN;
    } else if (x < -HALF_TURN) {
        x += TURN;
    }
    x += key->key[0];
    y = next->key[1] - key->key[1];
    if (y > HALF_TURN) {
        y -= TURN;
    } else if (y < -HALF_TURN) {
        y += TURN;
    }
    y += key->key[1];
    z = next->key[2] - key->key[2];
    if (z > HALF_TURN) {
        z -= TURN;
    } else if (z < -HALF_TURN) {
        z += TURN;
    }
    z += key->key[2];
    out[0] = (Angle)(((Float)key->key[0] * rest) + ((Float)x * t));
    out[1] = (Angle)(((Float)key->key[1] * rest) + ((Float)y * t));
    out[2] = (Angle)(((Float)key->key[2] * rest) + ((Float)z * t));
}

/* link_blend_linear_angles, with slot 1's angles taken the short way round
 * from slot 0's, as link_linear_angles_short takes the next key's */
/* NOLINTNEXTLINE(readability-non-const-parameter) */
static void link_blend_linear_angles_short(Angle *values, Angle *out,
                                           Float rate)
{
    Sint32 x;
    Sint32 y;
    Sint32 z;
    Float weight = rate;

    /* @hack spelling: slot 1's weight held in weight, a copy of rate, and slot
     * 0's, 1 - rate, in rate itself, as in link_blend_linear_angles (FMOV.S
     * FR4,FR5 at 0x8c655c2a). with rate as slot 1's weight and a local set to
     * 1.0F - rate as slot 0's, the function runs 4 bytes short and 94 of 96
     * words differ */
    rate = 1.0F - rate;

    x = values[3] - values[0];
    if (x > HALF_TURN) {
        x -= TURN;
    } else if (x < -HALF_TURN) {
        x += TURN;
    }
    x += values[0];
    y = values[4] - values[1];
    if (y > HALF_TURN) {
        y -= TURN;
    } else if (y < -HALF_TURN) {
        y += TURN;
    }
    y += values[1];
    z = values[5] - values[2];
    if (z > HALF_TURN) {
        z -= TURN;
    } else if (z < -HALF_TURN) {
        z += TURN;
    }
    z += values[2];
    out[0] = (Angle)(((Float)values[0] * rate) + ((Float)x * weight));
    out[1] = (Angle)(((Float)values[1] * rate) + ((Float)y * weight));
    out[2] = (Angle)(((Float)values[2] * rate) + ((Float)z * weight));
    /* @hack redundant: a read of values[1] that never runs; it adds a use of
     * values + 1, so 32768 takes R10 and -32768 R11 (0x8c655c3c and
     * 0x8c655c44); without it, they trade and 8 of 96 words differ */
    if (0) {
        out[0] = values[1];
    }
}

void (*_nj_motlink_data_func_table_[5])(NJS_OBJECT *object) = {
    link_motion_node_step_0, link_motion_node_step_1, link_motion_node_step_2,
    link_motion_node_step_3, link_motion_node_step_4};
void (*_nj_shapelink_data_func_table_[4])(NJS_OBJECT *object) = {
    link_shape_node_step_0, link_shape_node_step_1, link_shape_node_step_1,
    link_shape_node_step_2};
void (*_nj_motlink_int_f_func_table_[2])(NJS_MKEY_F *keys, Uint32 index,
                                         Uint32 nb, Float *out) = {
    link_linear_vector, link_cubic_vector};
void (*_nj_motlink_int_a_func_table_[2])(NJS_MKEY_A *keys, Uint32 index,
                                         Uint32 nb, Angle *out) = {
    link_linear_angles, link_cubic_angles};
void (*_nj_int_f_func_table_[2])(Float *values, Float *out, Float rate) = {
    link_blend_linear_vector, link_blend_cubic_vector};
void (*_nj_int_a_func_table_[2])(Angle *values, Angle *out, Float rate) = {
    link_blend_linear_angles, link_blend_cubic_angles};
void (*_nj_shapelink_int_func_table[2])(NJS_OBJECT *object) = {
    link_linear_shape, link_cubic_shape};
void (*_nj_motlink_int_a_func_table_old_[2])(NJS_MKEY_A *keys, Uint32 index,
                                             Uint32 nb, Angle *out) = {
    link_linear_angles_short, link_cubic_angles};
void (*_nj_int_a_func_table_old_[2])(Angle *values, Angle *out, Float rate) = {
    link_blend_linear_angles_short, link_blend_cubic_angles};
