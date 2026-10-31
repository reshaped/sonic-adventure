/* the motion draws that draw their models with njSimpleDrawModel.
 * njSimpleActionOld, njSimpleDrawMotionOld and njSimpleDrawShapeMotionOld take
 * each angle the short way round where they interpolate linearly, and
 * njSimpleDrawShapeMotionOld leaves _nj_draw_model_sub_, which the shape steps
 * draw with, as it stands */
#include <ninja.h>
#include <sg_xpt.h>

/* set up the shape motion at frame. choose its shape node step by what it
 * moves, and its shape step by its interpolation. with a null shape motion,
 * each node's model is drawn as it is. returns 1, with nothing set up, when
 * frame is not below the shape motion's nbFrame, else 0 */
static int start_simple_shape(NJS_MOTION *shape, Float frame)
{
    struct nj_motion_info *info = &_nj_shape_data_info_;
    Uint16 parts = 0;
    Uint16 spline = 0;
    Uint16 type = 0;

    if (shape) {
        if (!(frame < (Float)(Sint32)shape->nbFrame)) {
            return 1;
        }
        info->mdata = shape->mdata;
        info->nbFrame = shape->nbFrame;
        type = shape->type;
        spline = (shape->inp_fn & NJD_MTYPE_MASK) > 0;
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
    _nj_shape_data_func_ = _nj_shape_data_func_table_[parts];
    _nj_motion_shape_ = _nj_shape_int_func_table_[spline];
    _nj_mot_shape_data_type_ = type;
    _nj_mot_shape_spline_cnt_ = spline;
    _nj_mot_shape_data_cnt_ = parts;
    return 0;
}

/* set up the motion at frame, with its factor count, interpolation and type.
 * a null motion has no factors and no type, so each node is posed by its own
 * pos, ang and scl. returns 1, with nothing set up, when frame is not below
 * the motion's nbFrame, else 0 */
static int start_simple_motion(NJS_MOTION *motion, Float frame)
{
    struct nj_motion_info *info = &_nj_motion_data_info_;
    Uint16 type = 0;
    Uint16 spline = 0;
    Uint16 count = 0;

    if (motion) {
        if (!(frame < (Float)(Sint32)motion->nbFrame)) {
            return 1;
        }
        info->mdata = motion->mdata;
        info->nbFrame = motion->nbFrame;
        count = motion->inp_fn & ~NJD_MTYPE_MASK;
        spline = (motion->inp_fn & NJD_MTYPE_MASK) > 0;
        type = motion->type;
    }
    info->frame = frame;
    info->factor = 0;
    /* @hack type: m set to a plain 0: NULL's cast to void * gives the zero
     * locals other registers, masks with AND #192,R0, not the MOV.W of 0xc0 at
     * 0x8c656dd4, and moves this function's pool past select_simple_steps;
     * with NULL, the function is 82 bytes, not 144, the object 4 bytes
     * shorter, and 29 of 44 words differ */
    info->m = 0;
    _nj_mot_data_cnt_ = count;
    _nj_mot_spline_cnt_ = spline;
    _nj_mot_data_type_ = type;
    return 0;
}

/* choose the node step by the motion's factor count. each of the position,
 * angle and scale steps reads keys where the motion's type has that factor,
 * and the node's own pos, ang or scl otherwise. the angle step turns every
 * node in the order object's NJD_EVAL_ZXY_ANG gives. the interpolations
 * follow _nj_mot_spline_cnt_ */
static void select_simple_steps(NJS_OBJECT *object)
{
    int type = _nj_mot_data_type_;
    void (*translate)(NJS_OBJECT *obj);
    void (*rotate)(NJS_OBJECT *obj);
    void (*scale)(NJS_OBJECT *obj);
    Uint32 zxy;

    _nj_motion_data_func_ = _nj_motion_data_func_table_[_nj_mot_data_cnt_];
    translate = njMotionObjTranslate;
    if (type & NJD_MTYPE_POS_0) {
        translate = njMotionIntTranslate;
    }
    _nj_motion_translate_ = translate;
    zxy = object->evalflags & NJD_EVAL_ZXY_ANG;
    if (type & NJD_MTYPE_ANG_1) {
        rotate = njMotionIntRotateXYZ;
        if (zxy) {
            rotate = njMotionIntRotateZXY;
        }
    } else {
        rotate = njMotionObjRotateXYZ;
        if (zxy) {
            rotate = njMotionObjRotateZXY;
        }
    }
    _nj_motion_rotate_ = rotate;
    scale = njMotionObjScale;
    if (type & NJD_MTYPE_SCL_2) {
        scale = njMotionIntScale;
    }
    _nj_motion_scale_ = scale;
    _nj_motion_int_f_func_ = _nj_motion_int_f_func_table_[_nj_mot_spline_cnt_];
    _nj_motion_int_a_func_ = _nj_motion_int_a_func_table_[_nj_mot_spline_cnt_];
}

static void draw_simple_motion_tree(NJS_OBJECT *object)
{
    do {
        Uint32 flags = object->evalflags;

        njPushMatrix(NULL);
        if (!(flags & NJD_EVAL_SKIP)) {
            _nj_motion_data_func_(object);
        } else {
            Angle *angles;
            NJS_MATRIX *m = _nj_motion_data_info_.m;
            void (*rotate)(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz);
            Float x;
            Float y;
            Float z;
            Angle angx;
            Angle angy;
            Angle angz;
            Float *values = object->pos;

            x = *values++;
            y = *values++;
            z = *values;
            njTranslate(m, x, y, z);
            rotate = njRotateXYZ;
            if (flags & NJD_EVAL_ZXY_ANG) {
                rotate = njRotateZXY;
            }
            /* @bug memory: nothing sets angles, so a node under
             * NJD_EVAL_SKIP turns by the three words at the stack pointer, read
             * at 0x8c656ef8, 0x8c656efc and 0x8c656efe: angx, angy and
             * angz take the PR, R8 and R9 the function saved on entry */
#ifdef GCC
            /* repairs the unset angles the tag above names: under GCC, the
             * node turns by object->ang, as in njMotion.c's draw_motion_tree */
            angles = object->ang;
#endif
            /* NOLINTBEGIN(clang-diagnostic-uninitialized) */
            angx = angles[0];
            angy = angles[1];
            angz = angles[2];
            /* NOLINTEND(clang-diagnostic-uninitialized) */
            rotate(m, angx, angy, angz);
            values = object->scl;
            x = *values++;
            y = *values++;
            z = *values;
            njScale(m, x, y, z);
        }
        if (_nj_motion_callback_) {
            _nj_motion_callback_(object);
        }
        if (object->model && !(flags & NJD_EVAL_HIDE)) {
            njSimpleDrawModel(object->model);
        }
        if (object->child) {
            draw_simple_motion_tree(object->child);
        }
        njPopMatrix(1);
        object = object->sibling;
    } while (object);
}

static void draw_simple_shape_tree(NJS_OBJECT *object)
{
    do {
        Uint32 flags = object->evalflags;

        njPushMatrix(NULL);
        if (!(flags & NJD_EVAL_SKIP)) {
            _nj_motion_data_func_(object);
        } else {
            Angle *angles;
            NJS_MATRIX *m = _nj_motion_data_info_.m;
            void (*rotate)(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz);
            Float x;
            Float y;
            Float z;
            Angle angx;
            Angle angy;
            Angle angz;
            Float *values = object->pos;

            x = *values++;
            y = *values++;
            z = *values;
            njTranslate(m, x, y, z);
            rotate = njRotateXYZ;
            if (flags & NJD_EVAL_ZXY_ANG) {
                rotate = njRotateZXY;
            }
            /* @bug memory: nothing sets angles, so a node under
             * NJD_EVAL_SKIP turns by the three words at the stack pointer, read
             * at 0x8c657022, 0x8c657026 and 0x8c657028: angx, angy and
             * angz take the PR, R8 and R9 the function saved on entry */
#ifdef GCC
            /* repairs the unset angles the tag above names: under GCC, the
             * node turns by object->ang, as in njMotion.c's draw_shape_tree */
            angles = object->ang;
#endif
            /* NOLINTBEGIN(clang-diagnostic-uninitialized) */
            angx = angles[0];
            angy = angles[1];
            angz = angles[2];
            /* NOLINTEND(clang-diagnostic-uninitialized) */
            rotate(m, angx, angy, angz);
            values = object->scl;
            x = *values++;
            y = *values++;
            z = *values;
            njScale(m, x, y, z);
        }
        if (_nj_motion_callback_) {
            _nj_motion_callback_(object);
        }
        if (!(flags & NJD_EVAL_SHAPE_SKIP)) {
            _nj_shape_data_func_(object);
        } else if (object->model && !(flags & NJD_EVAL_HIDE)) {
            njSimpleDrawModel(object->model);
        }
        if (object->child) {
            draw_simple_shape_tree(object->child);
        }
        njPopMatrix(1);
        object = object->sibling;
    } while (object);
}

void njSimpleAction(NJS_ACTION *action, Float frame)
{
    NJS_MOTION *motion = action->motion;
    NJS_OBJECT *object = action->object;

    if (start_simple_motion(motion, frame) == 0) {
        select_simple_steps(object);
        njGetkmPointer();
        draw_simple_motion_tree(object);
        njSetkmPointer();
    }
}

void njSimpleDrawMotion(NJS_OBJECT *obj, NJS_MOTION *motion, Float frame)
{
    if (start_simple_motion(motion, frame) == 0) {
        select_simple_steps(obj);
        njGetkmPointer();
        draw_simple_motion_tree(obj);
        njSetkmPointer();
    }
}

void njSimpleDrawShapeMotion(NJS_OBJECT *obj, NJS_MOTION *motion,
                             NJS_MOTION *shape, Float frame)
{
    if (start_simple_motion(motion, frame) == 0 &&
        start_simple_shape(shape, frame) == 0) {
        select_simple_steps(obj);
        _nj_draw_model_sub_ = njSimpleDrawModel;
        njGetkmPointer();
        draw_simple_shape_tree(obj);
        njSetkmPointer();
    }
}

/* select_simple_steps, with the angle interpolations of
 * _nj_motion_int_a_func_table_old_ */
static void select_simple_steps_old(NJS_OBJECT *object)
{
    int type = _nj_mot_data_type_;
    void (*translate)(NJS_OBJECT *obj);
    void (*rotate)(NJS_OBJECT *obj);
    void (*scale)(NJS_OBJECT *obj);
    Uint32 zxy;

    _nj_motion_data_func_ = _nj_motion_data_func_table_[_nj_mot_data_cnt_];
    translate = njMotionObjTranslate;
    if (type & NJD_MTYPE_POS_0) {
        translate = njMotionIntTranslate;
    }
    _nj_motion_translate_ = translate;
    zxy = object->evalflags & NJD_EVAL_ZXY_ANG;
    /* @hack spelling: the store in each arm, which SHC merges into one at
     * 0x8c6571c4, puts #4 in R2 and _nj_motion_rotate_'s address in R3 at
     * 0x8c6571be and 0x8c6571c0; stored once after the if, as
     * select_simple_steps stores it, R2 and R3 trade roles from there to the
     * end, the object is 4 bytes longer, and 29 of 56 words differ */
    if (type & NJD_MTYPE_ANG_1) {
        rotate = njMotionIntRotateXYZ;
        if (zxy) {
            rotate = njMotionIntRotateZXY;
        }
        _nj_motion_rotate_ = rotate;
    } else {
        rotate = njMotionObjRotateXYZ;
        if (zxy) {
            rotate = njMotionObjRotateZXY;
        }
        _nj_motion_rotate_ = rotate;
    }
    scale = njMotionObjScale;
    if (type & NJD_MTYPE_SCL_2) {
        scale = njMotionIntScale;
    }
    _nj_motion_scale_ = scale;
    _nj_motion_int_f_func_ = _nj_motion_int_f_func_table_[_nj_mot_spline_cnt_];
    _nj_motion_int_a_func_ =
        _nj_motion_int_a_func_table_old_[_nj_mot_spline_cnt_];
}

void njSimpleActionOld(NJS_ACTION *action, Float frame)
{
    NJS_MOTION *motion = action->motion;
    NJS_OBJECT *object = action->object;

    if (start_simple_motion(motion, frame) == 0) {
        select_simple_steps_old(object);
        njGetkmPointer();
        draw_simple_motion_tree(object);
        njSetkmPointer();
    }
}

void njSimpleDrawMotionOld(NJS_OBJECT *obj, NJS_MOTION *motion, Float frame)
{
    if (start_simple_motion(motion, frame) == 0) {
        select_simple_steps_old(obj);
        njGetkmPointer();
        draw_simple_motion_tree(obj);
        njSetkmPointer();
    }
}

void njSimpleDrawShapeMotionOld(NJS_OBJECT *obj, NJS_MOTION *motion,
                                NJS_MOTION *shape, Float frame)
{
    if (start_simple_motion(motion, frame) == 0 &&
        start_simple_shape(shape, frame) == 0) {
        select_simple_steps_old(obj);
        njGetkmPointer();
        draw_simple_shape_tree(obj);
        njSetkmPointer();
    }
}
