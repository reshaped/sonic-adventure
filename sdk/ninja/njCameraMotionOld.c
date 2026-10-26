/* SDK 1.00J's camera motion, which differs from njCameraMotion.c in
 * roll_channel alone. the file compiles whole, but only four of its functions
 * reach an image, in njCameraMotion.c's object: its apply_channels,
 * roll_channel, njCameraAction and njCameraMotion, renamed apply_old_channels,
 * old_roll_channel, njCameraActionOld and njCameraMotionOld */
#include <ninja.h>
#include <sg_xpt.h>

/* a key of one angle at keyframe, as the roll and the angle of view hold
 * them */
struct angle_key {
    Uint32 keyframe;
    Angle key;
};

/* a node's motion data of five factors, as NJS_MDATA4 is of four */
struct motion_data5 {
    void *p[5];
    Uint32 nb[5];
};

static void linear_camera_vector(NJS_MKEY_F *keys, Uint32 index, Uint32 count,
                                 Float *out);
static void cubic_camera_vector(NJS_MKEY_F *keys, Uint32 index, Uint32 count,
                                Float *out);
static void position_channel(void);
static void direction_channel(void);
static void interest_channel(void);
static void roll_channel(void);
static void angle_channel(void);

/* where the call's motion stands */
static struct nj_motion_info playback;
/* the motion's factor count */
static Uint16 factor_count;
/* the motion's interpolation: 0 for linear and 1 for any other */
static Uint16 interpolation;
/* the motion's type, whose bits name the factors its records hold */
static Uint16 motion_type;
/* the copy of the camera that the motion moves */
static NJS_CAMERA working_camera;
/* the position's sampler, position_samplers[interpolation] */
static void (*sample_position)(NJS_MKEY_F *keys, Uint32 index, Uint32 count,
                               Float *out);
static void (*position_samplers[])(NJS_MKEY_F *keys, Uint32 index, Uint32 count,
                                   Float *out) = {linear_camera_vector,
                                                  cubic_camera_vector};

/* take motion at frame, and a copy of camera to move, if motion is set and
 * frame is below its nbFrame. otherwise change nothing */
static void prepare_camera(NJS_CAMERA *camera, NJS_MOTION *motion, Float frame)
{
    if (motion && frame < (Float)(Sint32)motion->nbFrame) {
        motion_type = motion->type;
        factor_count = motion->inp_fn & ~NJD_MTYPE_MASK;
        interpolation = (motion->inp_fn & NJD_MTYPE_MASK) > 0;
        playback.mdata = motion->mdata;
        playback.nbFrame = motion->nbFrame;
        playback.frame = frame;
        playback.factor = 0;
        playback.m = 0;
        working_camera = *camera;
        sample_position = position_samplers[interpolation];
    }
}

/* move the copy by the factors bits names, in their record's order, and set
 * the view to it. each _channel function moves the copy by the keys at
 * playback.factor, if they aren't null, and steps playback.factor on. this
 * function steps over the factors no _channel function takes: those of
 * NJD_MTYPE_ANG_1, NJD_MTYPE_SCL_2, 0x200, 0x400 and 0x1800. with both
 * NJD_MTYPE_VEC_3 and NJD_MTYPE_TARGET_3 set, it doesn't step over
 * NJD_MTYPE_VEC_3's factor: interest_channel reads its keys as the point, and
 * each later factor reads the keys of the one before it */
static void apply_channels(Uint32 bits)
{
    if (bits & NJD_MTYPE_POS_0) {
        position_channel();
    }
    {
        /* @hack spelling: the factor through a pointer, which keeps its address
         * in R5 for the load and the store; without it, 57 of 61 words
         * differ */
        Uint32 *slot = &playback.factor;
        Uint32 factor = *slot;

        if (bits & NJD_MTYPE_ANG_1) {
            ++factor;
        }
        if (bits & NJD_MTYPE_SCL_2) {
            ++factor;
        }
        *slot = factor;
    }
    if (bits & NJD_MTYPE_TARGET_3) {
        interest_channel();
    } else if (bits & NJD_MTYPE_VEC_3) {
        direction_channel();
    }
    if (bits & NJD_MTYPE_ROLL_6) {
        roll_channel();
    }
    if (bits & NJD_MTYPE_ANGLE_7) {
        angle_channel();
    }
    {
        /* @hack spelling: as above; without it, 57 of 61 words differ */
        Uint32 *slot = &playback.factor;
        Uint32 factor = *slot;

        if (bits & 0x200) {
            ++factor;
        }
        if (bits & 0x400) {
            ++factor;
        }
        if (bits & 0x1800) {
            ++factor;
        }
        *slot = factor;
    }
    njSetCamera(&working_camera);
}

/* set *index to the last of the count keys whose keyframe is at most the
 * frame, truncated and compared unsigned, or to 0 when none is. count must be
 * at least 1, and the keys in keyframe order */
static void find_camera_key(NJS_MKEY_F *keys, Uint32 count, Uint32 *index)
{
    Uint32 low = 0;
    Uint32 frame = (Sint32)playback.frame;
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

/* find_camera_key for angle keys */
static void find_camera_angle_key(struct angle_key *keys, Uint32 count,
                                  Uint32 *index)
{
    Uint32 low = 0;
    Uint32 frame = (Sint32)playback.frame;
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

/* set *out to key index's angle blended linearly toward the next key's,
 * truncated to an Angle. the key after the last is the first. the blend's t
 * is the frame less key index's keyframe, divided by the span to the next
 * keyframe when that span is above 0 */
static void linear_camera_angle(struct angle_key *keys, Uint32 index,
                                Uint32 count, Angle *out)
{
    struct angle_key *key = &keys[index];
    struct angle_key *next = keys;
    Float t;
    Float rest;
    Float inverse;
    Sint32 span;

    if (index + 1 < count) {
        next = key + 1;
    }
    /* @hack type: the keyframe read through a cast to const Sint32 *, as the
     * frame's conversion takes it. as (Float)(Sint32)key->keyframe, which
     * converts the same value, 27 of 39 words differ */
    t = playback.frame - (Float)(*(const Sint32 *)&key->keyframe);
    span = next->keyframe - key->keyframe;
    if (span > 0) {
        /* 1 / span, as the square of FSRRA's 1 / sqrt(span) */
        inverse = (Float)span;
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        t *= inverse;
    }
    rest = 1.0F - t;
    *out = (Angle)(((Float)key->key * rest) + ((Float)next->key * t));
}

/* linear_camera_angle's blend of key index's three Floats into out[0] to
 * out[2], without truncation */
static void linear_camera_vector(NJS_MKEY_F *keys, Uint32 index, Uint32 count,
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

    /* @hack spelling: keys, reused as the next key, which keeps rest in FR8, x
     * in FR6 and z in FR7; with a next local, as linear_camera_angle has, 16 of
     * 60 words differ */
    if (index + 1 < count) {
        keys = key + 1;
    }
    /* @hack type: the keyframe read through a cast to const Sint32 *, as in
     * linear_camera_angle; as (Float)(Sint32)key->keyframe, 46 of 60 words
     * differ */
    t = playback.frame - (Float)(*(const Sint32 *)&key->keyframe);
    span = keys->keyframe - key->keyframe;
    if (span > 0) {
        inverse = (Float)(keys->keyframe - key->keyframe);
        inverse = _builtin_fsrra(inverse);
        inverse *= inverse;
        t *= inverse;
    }
    rest = 1.0F - t;
    a = key->key;
    out += 3;
    b = keys->key;
    x = *a++;
    term = *b++;
    x *= rest;
    term *= t;
    x += term;
    y = *a++;
    term = *b++;
    y *= rest;
    term *= t;
    /* @hack spelling: y = y + term keeps x in FR6 and y in FR5; as y += term,
     * 10 of 60 words differ */
    y = y + term;
    z = *a;
    z *= rest;
    term = *b;
    term *= t;
    z += term;
    *--out = z;
    *--out = y;
    *--out = x;
}

/* NOLINTBEGIN(clang-diagnostic-undefined-internal) */
static void weighted_sum(Float *a, Float *b, Float *c, Float *d, Float wa,
                         Float wb, Float wc, Float wd, Float *out);
/* NOLINTEND(clang-diagnostic-undefined-internal) */

/* clang-format off */
#ifndef __clang__
/* out[i] = wa * a[i] + wb * b[i] + wc * c[i] + wd * d[i] for i from 0 to 2,
 * each one FIPR of the weights, in FV4. SHC expands the body at its call.
 * size=44, the body's bytes, keeps R10 from putting a pool ahead of the body
 * behind a BRA, which use-assembler-pools refuses */
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
        FMOV.S      @R4,FR0
        FMOV.S      @R5,FR1
        FMOV.S      @R6,FR2
        FMOV.S      @R7,FR3
        FIPR        FV4,FV0
        ADD         #12,R1
        FMOV.S      FR3,@-R1
        FMOV.S      FR9,@-R1
        FMOV.S      FR8,@-R1
}
#endif
/* clang-format on */

/* set out[0] to out[2] to key index's three values on the Catmull-Rom spline
 * through the key before it, it and the two after, at linear_camera_vector's
 * t. every key past the last is the first, and the key before the first is
 * the last */
static void cubic_camera_vector(NJS_MKEY_F *keys, Uint32 index, Uint32 count,
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
    /* @hack type: the keyframe read through a cast to const Sint32 *, as in
     * linear_camera_angle; as (Float)(Sint32)key->keyframe, 92 of 118 words
     * differ */
    t = playback.frame - (Float)(*(const Sint32 *)&key->keyframe);
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
    /* @hack spelling: the four keys' values set right before the body end the
     * key pointers' ranges there, so key keeps R7; passed directly, the four
     * take R11 to R14; without it, 108 of 118 words differ */
    pa = before->key;
    pb = key->key;
    pc = next->key;
    pd = after->key;
    weighted_sum(pa, pb, pc, pd, wa, wb, wc, wd, out);
}

/* set the copy's position, by the motion's interpolation */
static void position_channel(void)
{
    struct nj_motion_info *info = &playback;
    Uint32 factor = info->factor;
    NJS_MKEY_F *keys = info->p[factor];
    Uint32 index;
    Float value[3];
    Float *v;
    Float x;
    Float y;
    Float z;

    if (keys) {
        Uint32 count = info->nb[factor];

        find_camera_key(keys, count, &index);
        sample_position(keys, index, count, value);
        v = value;
        x = *v++;
        y = *v++;
        z = *v;
        v = &working_camera.px + 3;
        *--v = z;
        *--v = y;
        *--v = x;
    }
    info->factor = factor + 1;
}

/* turn the copy to look along a vector from its position */
static void direction_channel(void)
{
    struct nj_motion_info *info = &playback;
    Uint32 factor = info->factor;
    NJS_MKEY_F *keys = info->p[factor];
    Uint32 index;
    Float value[3];
    Float *v;
    Float x;
    Float y;
    Float z;

    if (keys) {
        Uint32 count = info->nb[factor];

        find_camera_key(keys, count, &index);
        linear_camera_vector(keys, index, count, value);
        v = &working_camera.px;
        x = *v++;
        y = *v++;
        z = *v;
        v = value;
        *v += x;
        v++;
        *v += y;
        v++;
        *v += z;
        njPointCameraInterest(&working_camera, value[0], value[1], value[2]);
    }
    info->factor = factor + 1;
}

/* turn the copy to look at a point */
static void interest_channel(void)
{
    struct nj_motion_info *info = &playback;
    Uint32 factor = info->factor;
    NJS_MKEY_F *keys = info->p[factor];
    Uint32 index;
    Float value[3];

    if (keys) {
        Uint32 count = info->nb[factor];

        find_camera_key(keys, count, &index);
        linear_camera_vector(keys, index, count, value);
        njPointCameraInterest(&working_camera, value[0], value[1], value[2]);
    }
    info->factor = factor + 1;
}

/* roll the copy about its direction by an angle, from the roll it has */
static void roll_channel(void)
{
    struct nj_motion_info *info = &playback;
    Uint32 factor = info->factor;
    struct angle_key *keys = info->p[factor];
    Uint32 index;
    Angle value;

    if (keys) {
        Uint32 count = info->nb[factor];

        find_camera_angle_key(keys, count, &index);
        linear_camera_angle(keys, index, count, &value);
        njRollCameraInterest(&working_camera, value);
    }
    info->factor = factor + 1;
}

/* set the copy's angle of view */
static void angle_channel(void)
{
    struct nj_motion_info *info = &playback;
    Uint32 factor = info->factor;
    struct angle_key *keys = info->p[factor];
    Uint32 index;
    Angle value;

    if (keys) {
        Uint32 count = info->nb[factor];

        find_camera_angle_key(keys, count, &index);
        linear_camera_angle(keys, index, count, &value);
        njSetCameraAngle(&working_camera, value);
    }
    info->factor = factor + 1;
}

void njCameraAction(NJS_CACTION *caction, Float frame)
{
    NJS_CAMERA *camera;
    NJS_MOTION *motion;
    struct nj_motion_info *info;
    Uint32 *counts;
    void **keys;
    void *data;

    camera = caction->camera;
    motion = caction->motion;
    prepare_camera(camera, motion, frame);
    info = &playback;
    data = info->mdata;
    switch (factor_count) {
    case 1:
        keys = ((NJS_MDATA1 *)data)->p;
        counts = ((NJS_MDATA1 *)data)->nb;
        break;
    case 2:
        keys = ((NJS_MDATA2 *)data)->p;
        counts = ((NJS_MDATA2 *)data)->nb;
        break;
    case 3:
        keys = ((NJS_MDATA3 *)data)->p;
        counts = ((NJS_MDATA3 *)data)->nb;
        break;
    case 4:
        keys = ((NJS_MDATA4 *)data)->p;
        counts = ((NJS_MDATA4 *)data)->nb;
        break;
    case 5:
        keys = ((struct motion_data5 *)data)->p;
        counts = ((struct motion_data5 *)data)->nb;
        break;
    default:
        return;
    }
    info->p = keys;
    info->nb = counts;
    apply_channels(motion_type);
}

void njCameraMotion(NJS_CAMERA *camera, NJS_MOTION *motion, Float frame)
{
    struct nj_motion_info *info;
    Uint32 *counts;
    void **keys;
    void *data;

    prepare_camera(camera, motion, frame);
    info = &playback;
    data = info->mdata;
    switch (factor_count) {
    case 1:
        keys = ((NJS_MDATA1 *)data)->p;
        counts = ((NJS_MDATA1 *)data)->nb;
        break;
    case 2:
        keys = ((NJS_MDATA2 *)data)->p;
        counts = ((NJS_MDATA2 *)data)->nb;
        break;
    case 3:
        keys = ((NJS_MDATA3 *)data)->p;
        counts = ((NJS_MDATA3 *)data)->nb;
        break;
    case 4:
        keys = ((NJS_MDATA4 *)data)->p;
        counts = ((NJS_MDATA4 *)data)->nb;
        break;
    default:
        return;
    }
    info->p = keys;
    info->nb = counts;
    apply_channels(motion_type);
}
