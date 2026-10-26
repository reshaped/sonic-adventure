/* the motion steps and the shape steps, their interpolations, linear and along
 * a Catmull-Rom spline, and their tables, with njInitShape and
 * njSetMotionCallback */
#include <ninja.h>
#include <sg_xpt.h>

/* an Angle's full turn, and half of it */
enum { TURN = 65536, HALF_TURN = TURN / 2 };

/* set *index to the last of count keys whose keyframe is at most the motion's
 * frame. the frame is truncated and compared unsigned. *index is 0 where no
 * keyframe after the first is at most the frame. the keys must be in frame
 * order, and count at least 1. keys holds NJS_MKEY_F or NJS_MKEY_A, both 16
 * bytes with the keyframe first */
static void find_motion_key(void *keys, Uint32 count, Uint32 *index)
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

/* find_motion_key for shape keys, at the shape motion's frame */
static void find_shape_motion_key(NJS_MKEY_P *keys, Uint32 count, Uint32 *index)
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
static void linear_vector(NJS_MKEY_F *keys, Uint32 index, Uint32 count,
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
    t = _nj_motion_data_info_.frame - (Float)(Sint32)key->keyframe;
    span = (Sint32)next->keyframe - (Sint32)key->keyframe;
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

/* linear_vector for angles, each blended as a Float and truncated */
static void linear_angles(NJS_MKEY_A *keys, Uint32 index, Uint32 count,
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
    t = _nj_motion_data_info_.frame - (Float)(Sint32)key->keyframe;
    span = (Sint32)next->keyframe - (Sint32)key->keyframe;
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
 * and each sum is truncated back. size=104 is the body's length, for the reason
 * weighted_sum gives */
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
 * the spline runs through the key before key index, the key and the two after
 * it, and t is as linear_vector forms it. the key before the first is the last,
 * and each key past the last is the first */
static void cubic_vector(NJS_MKEY_F *keys, Uint32 index, Uint32 count,
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
    Float wa;
    Float wb;
    Float wc;
    Float wd;
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
    next = keys;
    after = keys;
    if (index + 1 < count) {
        next = key + 1;
    }
    if (index + 2 < count) {
        after = next + 1;
    }
    t = _nj_motion_data_info_.frame - (Float)(Sint32)key->keyframe;
    span = (Sint32)next->keyframe - (Sint32)key->keyframe;
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
    /* @hack spelling: the four keys' values passed through pa to pd; passed
     * directly, SHC saves R11 as well, the function runs 4 bytes long and 108
     * of 118 words differ */
    pa = before->key;
    pb = key->key;
    pc = next->key;
    pd = after->key;
    weighted_sum(pa, pb, pc, pd, wa, wb, wc, wd, out);
}

/* cubic_vector for angles, each blended as a Float and truncated */
static void cubic_angles(NJS_MKEY_A *keys, Uint32 index, Uint32 count,
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
    t = _nj_motion_data_info_.frame - (Float)(Sint32)key->keyframe;
    span = (Sint32)next->keyframe - (Sint32)key->keyframe;
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
    /* @hack spelling: the four keys' values passed through pa to pd, as in
     * cubic_vector; passed directly, SHC saves R11 as well, the function runs 4
     * bytes long and 138 of 148 words differ */
    pa = before->key;
    pb = key->key;
    pc = next->key;
    pd = after->key;
    weighted_sum_angles(pa, pb, pc, pd, wa, wb, wc, wd, out);
}

void njMotionIntTranslate(NJS_OBJECT *obj)
{
    struct nj_motion_info *info = &_nj_motion_data_info_;
    Uint32 factor = info->factor;
    NJS_MKEY_F *keys = info->p[factor];
    Float *pos = obj->pos;
    Uint32 index;
    Uint32 count;
    Float value[3];
    Float x;
    Float y;
    Float z;

    if (keys) {
        count = info->nb[factor];
        find_motion_key(keys, count, &index);
        _nj_motion_int_f_func_(keys, index, count, value);
        pos = value;
    }
    x = *pos++;
    y = *pos++;
    z = *pos;
    njTranslate(info->m, x, y, z);
    info->factor = factor + 1;
}

void njMotionIntScale(NJS_OBJECT *obj)
{
    struct nj_motion_info *info = &_nj_motion_data_info_;
    Uint32 factor = info->factor;
    NJS_MKEY_F *keys = info->p[factor];
    Float *scl = obj->scl;
    Uint32 index;
    Uint32 count;
    Float value[3];
    Float x;
    Float y;
    Float z;

    if (keys) {
        count = info->nb[factor];
        find_motion_key(keys, count, &index);
        _nj_motion_int_f_func_(keys, index, count, value);
        scl = value;
    }
    x = *scl++;
    y = *scl++;
    z = *scl;
    njScale(info->m, x, y, z);
    info->factor = factor + 1;
}

void njMotionIntRotateXYZ(NJS_OBJECT *obj)
{
    struct nj_motion_info *info = &_nj_motion_data_info_;
    Uint32 factor = info->factor;
    NJS_MKEY_A *keys = info->p[factor];
    Angle *ang = obj->ang;
    Uint32 index;
    Uint32 count;
    Angle value[3];
    Angle x;
    Angle y;
    Angle z;

    if (keys) {
        count = info->nb[factor];
        find_motion_key(keys, count, &index);
        _nj_motion_int_a_func_(keys, index, count, value);
        ang = value;
    }
    x = ang[0];
    y = ang[1];
    z = ang[2];
    njRotateXYZ(info->m, x, y, z);
    info->factor = factor + 1;
}

void njMotionIntRotateZXY(NJS_OBJECT *obj)
{
    struct nj_motion_info *info = &_nj_motion_data_info_;
    Uint32 factor = info->factor;
    NJS_MKEY_A *keys = info->p[factor];
    Angle *ang = obj->ang;
    Uint32 index;
    Uint32 count;
    Angle value[3];
    Angle x;
    Angle y;
    Angle z;

    if (keys) {
        count = info->nb[factor];
        find_motion_key(keys, count, &index);
        _nj_motion_int_a_func_(keys, index, count, value);
        ang = value;
    }
    x = ang[0];
    y = ang[1];
    z = ang[2];
    njRotateZXY(info->m, x, y, z);
    info->factor = factor + 1;
}

/* set four[0] to four[3] to the points or normals of four keys: the one before
 * key index, the key and the two after it. the key before the first is the
 * last, and each key past the last is the first. returns t, the weight of the
 * next key, as linear_vector forms it at the shape motion's frame */
static Float sample_shape_cubic(NJS_MKEY_P *keys, Uint32 index, Uint32 count,
                                NJS_POINT3 **four)
{
    NJS_MKEY_P *before;
    NJS_MKEY_P *key;
    NJS_MKEY_P *next;
    NJS_MKEY_P *after;
    Float t;
    Float inverse;
    Sint32 span;
    NJS_POINT3 *data;

    key = &keys[index];
    before = &keys[count - 1];
    if (index > 0) {
        before = key - 1;
    }
    next = keys;
    if (index + 1 < count) {
        next = key + 1;
    }
    after = keys;
    if (index + 2 < count) {
        after = next + 1;
    }
    t = _nj_shape_data_info_.frame - (Float)(Sint32)key->keyframe;
    span = (Sint32)next->keyframe - (Sint32)key->keyframe;
    if (span > 0) {
        inverse = (Float)span;
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        t *= inverse;
    }
    /* @hack spelling: each key's data stored through data, which SHC loads
     * into R4 and stores from there (0x8c65627c to 0x8c65628c); stored
     * directly, it takes R3 and R2, and 8 of 56 words differ */
    four += 4;
    data = after->key;
    *--four = data;
    data = next->key;
    *--four = data;
    data = key->key;
    *--four = data;
    data = before->key;
    *--four = data;
    return t;
}

/* set pair[0] and pair[1] to the points or normals of key index and the next
 * key. the key after the last is the first. returns t, the weight of the next
 * key, as linear_vector forms it at the shape motion's frame */
static Float sample_shape_linear(NJS_MKEY_P *keys, Uint32 index, Uint32 count,
                                 NJS_POINT3 **pair)
{
    NJS_MKEY_P *key = &keys[index];
    NJS_MKEY_P *next = keys;
    Float t;
    Float inverse;
    Sint32 span;
    NJS_POINT3 *data;

    if (index + 1 < count) {
        next = key + 1;
    }
    t = _nj_shape_data_info_.frame - (Float)(Sint32)key->keyframe;
    span = (Sint32)next->keyframe - (Sint32)key->keyframe;
    if (span > 0) {
        inverse = (Float)span;
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        t *= inverse;
    }
    /* @hack spelling: each key's data stored through data, into R4 as in
     * sample_shape_cubic (0x8c6562c6 to 0x8c6562cc); stored directly, it takes
     * R3 and R2, and 4 of 31 words differ */
    pair += 2;
    data = next->key;
    *--pair = data;
    data = key->key;
    *--pair = data;
    return t;
}

/* NOLINTNEXTLINE(clang-diagnostic-undefined-internal) */
static Float squared_length(Float x, Float y, Float z, Float w);

/* clang-format off */
#ifndef __clang__
/* returns x * x + y * y + z * z + w * w, by one FIPR. SHC expands the body at
 * each call. size=4 is its length in bytes, for the reason weighted_sum
 * gives */
/* @asm operand: fipr-registers, each FIPR FV4,FV4, at 0x8c656306 in
 * linear_normal and 0x8c656408 in cubic_normal, squares the blended normal in
 * the registers it is in */
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
static void linear_normal(Float *a, Float *b, Float *out, Float t)
{
    Float rest;
    Float x;
    Float y;
    Float z;
    Float term;
    Float length;

    rest = 1.0F - t;
    /* @hack spelling: Float pointers walk the a, b and out points' x, y and z,
     * which gives the FMOV.S @R4+ and @R5+ loads (0x8c6562e2 to 0x8c6562fa) and
     * the FMOV.S @-R14 stores (0x8c656326 to 0x8c65632a); with NJS_POINT3
     * parameters read and stored by member, 42 of 50 words differ and the
     * section is 8 bytes longer */
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
static void linear_point(Float *a, Float *b, Float *out, Float t)
{
    Float rest;
    Float x;
    Float y;
    Float z;
    Float term;

    /* @hack spelling: Float pointers walk the a, b and out points' x, y and z,
     * which gives the FMOV.S @R4+ and @R5+ loads (0x8c656336 to 0x8c656354) and
     * the FMOV.S @-R6 stores (0x8c65635a to 0x8c656360); with NJS_POINT3
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

/* set out to the Catmull-Rom spline through a, b, c and d at t, which runs from
 * b at 0 to c at 1. out is scaled to unit length as linear_normal scales it */
static void cubic_normal(Float *a, Float *b, Float *c, Float *d, Float *out,
                         Float t)
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
    Float *sum;

    square = t * t;
    cube = square * t;
    wa = ((2.0F * square) - t - cube) * 0.5F;
    wb = (1.5F * cube) - (2.5F * square) + 1.0F;
    wc = (0.5F * t) - (1.5F * cube) + (2.0F * square);
    wd = (cube - square) * 0.5F;
    weighted_sum(a, b, c, d, wa, wb, wc, wd, out);
    /* @hack spelling: a Float pointer walks the out point's x, y and z up, and
     * out steps back down from past z, which gives the FMOV.S @R4+ loads
     * (0x8c6563fa to 0x8c656402) and the FMOV.S @-R14 stores (0x8c656428 to
     * 0x8c65642c); with an NJS_POINT3 out read and stored by member, 68 of 108
     * words differ and the section is 8 bytes longer */
    sum = out;
    x = *sum++;
    y = *sum++;
    z = *sum;
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

/* set out to the Catmull-Rom spline through a, b, c and d at t, which runs from
 * b at 0 to c at 1 */
static void cubic_point(Float *a, Float *b, Float *c, Float *d, Float *out,
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

/* where the shape motion's current factor has keys, fill four and *t as
 * sample_shape_cubic does, and add SHAPE_POINTS to *found */
static void get_cubic_points(Sint32 *found, Float *t, NJS_POINT3 **four)
{
    Uint32 index;
    Uint32 factor = _nj_shape_data_info_.factor;
    NJS_MKEY_P *keys = _nj_shape_data_info_.p[factor];
    /* @hack redundant: register on count, which keeps count in R14 and keys
     * in R13 (MOV.L @(R0,R6),R14 at 0x8c6564ec); without it, the two trade
     * registers, found and four trade stack slots, and 11 of 41 words differ */
    register Uint32 count;

    if (keys) {
        count = _nj_shape_data_info_.nb[factor];
        find_shape_motion_key(keys, count, &index);
        *t = sample_shape_cubic(keys, index, count, four);
        *found += SHAPE_POINTS;
    }
}

/* get_cubic_points for normals, which adds SHAPE_NORMALS to *found */
static void get_cubic_normals(Sint32 *found, Float *t, NJS_POINT3 **four)
{
    Uint32 index;
    Uint32 factor = _nj_shape_data_info_.factor;
    NJS_MKEY_P *keys = _nj_shape_data_info_.p[factor];
    /* @hack redundant: register on count, as in get_cubic_points; without
     * it, 11 of 39 words differ */
    register Uint32 count;

    if (keys) {
        count = _nj_shape_data_info_.nb[factor];
        find_shape_motion_key(keys, count, &index);
        *t = sample_shape_cubic(keys, index, count, four);
        *found += SHAPE_NORMALS;
    }
}

/* set the first count points of _nj_shape_buf_ by cubic_point, each from the
 * same point of four[0] to four[3] at t, and set model->points to them. count
 * must be at least 1 */
static void cubic_morph_points(NJS_MODEL *model, Sint32 count, Float t,
                               NJS_POINT3 **four)
{
    NJS_POINT3 *out = (NJS_POINT3 *)_nj_shape_buf_;
    NJS_POINT3 *buffer = out;
    NJS_POINT3 *a = four[0];
    NJS_POINT3 *b = four[1];
    NJS_POINT3 *c = four[2];
    NJS_POINT3 *d = four[3];
    Sint32 left = count;

    do {
        cubic_point(&a->x, &b->x, &c->x, &d->x, &out->x, t);
        a++;
        b++;
        c++;
        d++;
        out++;
    } while (--left != 0);
    model->points = buffer;
}

/* cubic_morph_points for normals, each set by cubic_normal, with
 * model->normals set to them */
static void cubic_morph_normals(NJS_MODEL *model, Sint32 count, Float t,
                                NJS_POINT3 **four)
{
    NJS_POINT3 *out = (NJS_POINT3 *)_nj_shape_buf_;
    NJS_POINT3 *buffer = out;
    NJS_POINT3 *a = four[0];
    NJS_POINT3 *b = four[1];
    NJS_POINT3 *c = four[2];
    NJS_POINT3 *d = four[3];
    Sint32 left = count;

    do {
        cubic_normal(&a->x, &b->x, &c->x, &d->x, &out->x, t);
        a++;
        b++;
        c++;
        d++;
        out++;
    } while (--left != 0);
    model->normals = buffer;
}

/* cubic_morph_normals, but into the count points of _nj_shape_buf_ that
 * follow those cubic_morph_points sets */
static void cubic_morph_normals_after_points(NJS_MODEL *model, Sint32 count,
                                             Float t, NJS_POINT3 **four)
{
    /* @hack spelling: the float index walks the buffer's NJS_POINT3 members to
     * the normals after count points, which loads the buffer pointer into R0
     * (MOV.L
     * @R2,R0 at 0x8c656660); as (NJS_POINT3 *)_nj_shape_buf_ + count, 3 of 52
     * words differ */
    NJS_POINT3 *out = (NJS_POINT3 *)&_nj_shape_buf_[count * 3];
    NJS_POINT3 *buffer = out;
    NJS_POINT3 *a = four[0];
    NJS_POINT3 *b = four[1];
    NJS_POINT3 *c = four[2];
    NJS_POINT3 *d = four[3];
    Sint32 left = count;

    do {
        cubic_normal(&a->x, &b->x, &c->x, &d->x, &out->x, t);
        a++;
        b++;
        c++;
        d++;
        out++;
    } while (--left != 0);
    model->normals = buffer;
}

/* the cubic shape step, which draws the node's model with _nj_draw_model_sub_.
 * where the shape motion has keys for the model's points, normals or both, it
 * morphs them into _nj_shape_buf_ for the draw, the normals after the points
 * where both are morphed. it then restores the model's own points and normals.
 * a null model, or one under NJD_EVAL_HIDE, is left alone */
static void draw_cubic_shape(NJS_OBJECT *obj)
{
    NJS_MODEL *model = obj->model;
    NJS_POINT3 *points;
    NJS_POINT3 *normals;
    Sint32 count;
    Sint32 found;
    Float point_t;
    Float normal_t;
    NJS_POINT3 *point_four[4];
    NJS_POINT3 *normal_four[4];

    if (model && !(obj->evalflags & NJD_EVAL_HIDE)) {
        found = 0;
        points = model->points;
        normals = model->normals;
        count = model->nbPoint;
        switch (_nj_mot_shape_data_cnt_) {
        case SHAPE_POINTS:
            get_cubic_points(&found, &point_t, point_four);
            break;
        case SHAPE_NORMALS:
            get_cubic_normals(&found, &normal_t, normal_four);
            break;
        case SHAPE_BOTH:
            get_cubic_points(&found, &point_t, point_four);
            ++_nj_shape_data_info_.factor;
            get_cubic_normals(&found, &normal_t, normal_four);
            break;
        }
        switch (found) {
        case SHAPE_POINTS:
            cubic_morph_points(model, count, point_t, point_four);
            break;
        case SHAPE_NORMALS:
            cubic_morph_normals(model, count, normal_t, normal_four);
            break;
        case SHAPE_BOTH:
            cubic_morph_points(model, count, point_t, point_four);
            cubic_morph_normals_after_points(model, count, normal_t,
                                             normal_four);
            break;
        }
        _nj_draw_model_sub_(model);
        model->points = points;
        model->normals = normals;
    }
}

/* get_cubic_points for a linear shape motion: it fills pair and *t as
 * sample_shape_linear does */
static void get_linear_points(Sint32 *found, Float *t, NJS_POINT3 **pair)
{
    Uint32 index;
    Uint32 factor = _nj_shape_data_info_.factor;
    NJS_MKEY_P *keys = _nj_shape_data_info_.p[factor];
    /* @hack redundant: register on count, as in get_cubic_points; without
     * it, 11 of 39 words differ */
    register Uint32 count;

    if (keys) {
        count = _nj_shape_data_info_.nb[factor];
        find_shape_motion_key(keys, count, &index);
        *t = sample_shape_linear(keys, index, count, pair);
        *found += SHAPE_POINTS;
    }
}

/* get_linear_points for normals, which adds SHAPE_NORMALS to *found */
static void get_linear_normals(Sint32 *found, Float *t, NJS_POINT3 **pair)
{
    Uint32 index;
    Uint32 factor = _nj_shape_data_info_.factor;
    NJS_MKEY_P *keys = _nj_shape_data_info_.p[factor];
    /* @hack redundant: register on count, as in get_cubic_points; without
     * it, 11 of 39 words differ */
    register Uint32 count;

    if (keys) {
        count = _nj_shape_data_info_.nb[factor];
        find_shape_motion_key(keys, count, &index);
        *t = sample_shape_linear(keys, index, count, pair);
        *found += SHAPE_NORMALS;
    }
}

/* set the first count points of _nj_shape_buf_ by linear_point, each from the
 * same point of pair[0] and pair[1] at t, and set model->points to them. count
 * must be at least 1 */
static void linear_morph_points(NJS_MODEL *model, Sint32 count, Float t,
                                NJS_POINT3 **pair)
{
    NJS_POINT3 *out = (NJS_POINT3 *)_nj_shape_buf_;
    NJS_POINT3 *buffer = out;
    NJS_POINT3 *a = pair[0];
    NJS_POINT3 *b = pair[1];
    Sint32 left = count;

    do {
        linear_point(&a->x, &b->x, &out->x, t);
        a++;
        b++;
        out++;
    } while (--left != 0);
    model->points = buffer;
}

/* linear_morph_points for normals, each set by linear_normal, with
 * model->normals set to them */
static void linear_morph_normals(NJS_MODEL *model, Sint32 count, Float t,
                                 NJS_POINT3 **pair)
{
    NJS_POINT3 *out = (NJS_POINT3 *)_nj_shape_buf_;
    NJS_POINT3 *buffer = out;
    NJS_POINT3 *a = pair[0];
    NJS_POINT3 *b = pair[1];
    Sint32 left = count;

    do {
        linear_normal(&a->x, &b->x, &out->x, t);
        a++;
        b++;
        out++;
    } while (--left != 0);
    model->normals = buffer;
}

/* linear_morph_normals, but into the count points of _nj_shape_buf_ that
 * follow those linear_morph_points sets */
static void linear_morph_normals_after_points(NJS_MODEL *model, Sint32 count,
                                              Float t, NJS_POINT3 **pair)
{
    /* @hack spelling: the float index walks the buffer's NJS_POINT3 members to
     * the normals after count points, which loads the buffer pointer into R0
     * (MOV.L
     * @R2,R0 at 0x8c6568e6); as (NJS_POINT3 *)_nj_shape_buf_ + count, 3 of 40
     * words differ */
    NJS_POINT3 *out = (NJS_POINT3 *)&_nj_shape_buf_[count * 3];
    NJS_POINT3 *buffer = out;
    NJS_POINT3 *a = pair[0];
    NJS_POINT3 *b = pair[1];
    Sint32 left = count;

    do {
        linear_normal(&a->x, &b->x, &out->x, t);
        a++;
        b++;
        out++;
    } while (--left != 0);
    model->normals = buffer;
}

/* the linear shape step: draw_cubic_shape, but blending each point and normal
 * linearly from the key's to the next key's */
static void draw_linear_shape(NJS_OBJECT *obj)
{
    NJS_MODEL *model = obj->model;
    NJS_POINT3 *points;
    NJS_POINT3 *normals;
    Sint32 count;
    Sint32 found;
    Float point_t;
    Float normal_t;
    NJS_POINT3 *point_pair[2];
    NJS_POINT3 *normal_pair[2];

    if (model && !(obj->evalflags & NJD_EVAL_HIDE)) {
        found = 0;
        points = model->points;
        normals = model->normals;
        count = model->nbPoint;
        switch (_nj_mot_shape_data_cnt_) {
        case SHAPE_POINTS:
            get_linear_points(&found, &point_t, point_pair);
            break;
        case SHAPE_NORMALS:
            get_linear_normals(&found, &normal_t, normal_pair);
            break;
        case SHAPE_BOTH:
            get_linear_points(&found, &point_t, point_pair);
            ++_nj_shape_data_info_.factor;
            get_linear_normals(&found, &normal_t, normal_pair);
            break;
        }
        switch (found) {
        case SHAPE_POINTS:
            linear_morph_points(model, count, point_t, point_pair);
            break;
        case SHAPE_NORMALS:
            linear_morph_normals(model, count, normal_t, normal_pair);
            break;
        case SHAPE_BOTH:
            linear_morph_points(model, count, point_t, point_pair);
            linear_morph_normals_after_points(model, count, normal_t,
                                              normal_pair);
            break;
        }
        _nj_draw_model_sub_(model);
        model->points = points;
        model->normals = normals;
    }
}

void njMotionObjTranslate(NJS_OBJECT *obj)
{
    Float *pos = obj->pos;
    Float x = *pos++;
    Float y = *pos++;
    Float z = *pos;

    njTranslate(_nj_motion_data_info_.m, x, y, z);
}

void njMotionObjRotateXYZ(NJS_OBJECT *obj)
{
    Angle *ang = obj->ang;
    Angle x = ang[0];
    Angle y = ang[1];
    Angle z = ang[2];

    njRotateXYZ(_nj_motion_data_info_.m, x, y, z);
}

void njMotionObjRotateZXY(NJS_OBJECT *obj)
{
    Angle *ang = obj->ang;
    Angle x = ang[0];
    Angle y = ang[1];
    Angle z = ang[2];

    njRotateZXY(_nj_motion_data_info_.m, x, y, z);
}

void njMotionObjScale(NJS_OBJECT *obj)
{
    Float *scl = obj->scl;
    Float x = *scl++;
    Float y = *scl++;
    Float z = *scl;

    njScale(_nj_motion_data_info_.m, x, y, z);
}

/* motion_node_step_0 to motion_node_step_4, the node steps, by the motion's
 * factor count. each runs the position, angle and scale steps on the node.
 * with a count above 0, a step first points the motion at the node's record,
 * an NJS_MDATA1 to NJS_MDATA4 by the count, from its first factor, and after
 * the three steps moves the motion's mdata to the next node's record */
static void motion_node_step_0(NJS_OBJECT *obj)
{
    _nj_motion_translate_(obj);
    _nj_motion_rotate_(obj);
    _nj_motion_scale_(obj);
}

static void motion_node_step_1(NJS_OBJECT *obj)
{
    NJS_MDATA1 *record = _nj_motion_data_info_.mdata;

    _nj_motion_data_info_.p = record->p;
    _nj_motion_data_info_.nb = record->nb;
    _nj_motion_data_info_.factor = 0;
    _nj_motion_translate_(obj);
    _nj_motion_rotate_(obj);
    _nj_motion_scale_(obj);
    _nj_motion_data_info_.mdata = record + 1;
}

static void motion_node_step_2(NJS_OBJECT *obj)
{
    NJS_MDATA2 *record = _nj_motion_data_info_.mdata;

    _nj_motion_data_info_.p = record->p;
    _nj_motion_data_info_.nb = record->nb;
    _nj_motion_data_info_.factor = 0;
    _nj_motion_translate_(obj);
    _nj_motion_rotate_(obj);
    _nj_motion_scale_(obj);
    _nj_motion_data_info_.mdata = record + 1;
}

static void motion_node_step_3(NJS_OBJECT *obj)
{
    NJS_MDATA3 *record = _nj_motion_data_info_.mdata;

    _nj_motion_data_info_.p = record->p;
    _nj_motion_data_info_.nb = record->nb;
    _nj_motion_data_info_.factor = 0;
    _nj_motion_translate_(obj);
    _nj_motion_rotate_(obj);
    _nj_motion_scale_(obj);
    _nj_motion_data_info_.mdata = record + 1;
}

static void motion_node_step_4(NJS_OBJECT *obj)
{
    NJS_MDATA4 *record = _nj_motion_data_info_.mdata;

    _nj_motion_data_info_.p = record->p;
    _nj_motion_data_info_.nb = record->nb;
    _nj_motion_data_info_.factor = 0;
    _nj_motion_translate_(obj);
    _nj_motion_rotate_(obj);
    _nj_motion_scale_(obj);
    _nj_motion_data_info_.mdata = record + 1;
}

/* shape_node_step_0 to shape_node_step_2, the shape node steps, by what the
 * shape motion moves. with neither points nor normals, the step draws the
 * node's model as it is, with _nj_draw_model_sub_, unless the model is null or
 * under NJD_EVAL_HIDE. otherwise a step points the shape motion at the node's
 * record, an NJS_MDATA1 for one part or an NJS_MDATA2 for both, from its first
 * factor. it runs the shape step, then moves the shape motion's mdata to the
 * next node's record */
static void shape_node_step_0(NJS_OBJECT *obj)
{
    if (obj->model && !(obj->evalflags & NJD_EVAL_HIDE)) {
        _nj_draw_model_sub_(obj->model);
    }
}

static void shape_node_step_1(NJS_OBJECT *obj)
{
    NJS_MDATA1 *record = _nj_shape_data_info_.mdata;

    _nj_shape_data_info_.p = record->p;
    _nj_shape_data_info_.nb = record->nb;
    _nj_shape_data_info_.factor = 0;
    _nj_motion_shape_(obj);
    _nj_shape_data_info_.mdata = record + 1;
}

static void shape_node_step_2(NJS_OBJECT *obj)
{
    NJS_MDATA2 *record = _nj_shape_data_info_.mdata;

    _nj_shape_data_info_.p = record->p;
    _nj_shape_data_info_.nb = record->nb;
    _nj_shape_data_info_.factor = 0;
    _nj_motion_shape_(obj);
    _nj_shape_data_info_.mdata = record + 1;
}

void njInitShape(Float *buf)
{
    _nj_shape_buf_ = buf;
}

void njSetMotionCallback(void (*func)(NJS_OBJECT *obj))
{
    _nj_motion_callback_ = func;
}

/* linear_angles, but each angle of the next key is first moved a turn toward
 * key index's where the two are more than half a turn apart, so the blend takes
 * the short way round */
static void linear_angles_short(NJS_MKEY_A *keys, Uint32 index, Uint32 count,
                                Angle *out)
{
    NJS_MKEY_A *key = &keys[index];
    NJS_MKEY_A *next = keys;
    Float t;
    Float rest;
    Float inverse;
    Sint32 span;
    Sint32 x;
    Sint32 y;
    Sint32 z;

    if (index + 1 < count) {
        next = key + 1;
    }
    t = _nj_motion_data_info_.frame - (Float)(Sint32)key->keyframe;
    span = (Sint32)next->keyframe - (Sint32)key->keyframe;
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

void (*_nj_motion_data_func_table_[5])(NJS_OBJECT *obj) = {
    motion_node_step_0, motion_node_step_1, motion_node_step_2,
    motion_node_step_3, motion_node_step_4};
void (*_nj_shape_data_func_table_[4])(NJS_OBJECT *obj) = {
    shape_node_step_0, shape_node_step_1, shape_node_step_1, shape_node_step_2};
void (*_nj_motion_int_f_func_table_[2])(NJS_MKEY_F *keys, Uint32 index,
                                        Uint32 nb, Float *out) = {linear_vector,
                                                                  cubic_vector};
void (*_nj_motion_int_a_func_table_[2])(NJS_MKEY_A *keys, Uint32 index,
                                        Uint32 nb, Angle *out) = {linear_angles,
                                                                  cubic_angles};
void (*_nj_shape_int_func_table_[2])(NJS_OBJECT *obj) = {draw_linear_shape,
                                                         draw_cubic_shape};
void (*_nj_motion_int_a_func_table_old_[2])(NJS_MKEY_A *keys, Uint32 index,
                                            Uint32 nb, Angle *out) = {
    linear_angles_short, cubic_angles};
