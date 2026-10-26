/* the motion draws that light their models as njDrawModel does.
 * njActionOld, njDrawMotionOld and njDrawShapeMotionOld differ from the forms
 * without Old only in their linear angle interpolation, which takes each angle
 * the short way round */
#include <ninja.h>
#include <sg_xpt.h>

static void draw_motion_tree(NJS_OBJECT *object);
static void draw_shape_tree(NJS_OBJECT *object);

/* set up the shape motion at frame. choose its shape node step by what it
 * moves, and its shape step by its interpolation. with a null shape motion,
 * each node's model is drawn as it is. returns 1, with nothing set up, when
 * frame is not below the shape motion's nbFrame, else 0 */
static int start_shape(NJS_MOTION *shape, Float frame)
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
static int start_motion(NJS_MOTION *motion, Float frame)
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
     * 0x8c615270, and moves this function's pool past select_steps; with NULL,
     * the function is 82 bytes, not 144, the object 4 bytes shorter, and 29
     * of 44 words differ */
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
static void select_steps(NJS_OBJECT *object)
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

/* draw object's tree by draw_motion_tree, in the light setup njDrawModel makes
 * for one model, made here once for the whole tree */
static void draw_lit_motion(NJS_OBJECT *object)
{
    njSetQueue();
    njSetLightMaterials();
    njCalcLightMatrices();
    njGetkmPointer();
    draw_motion_tree(object);
    njSetkmPointer();
    njResetQueue();
}

/* draw_lit_motion for draw_shape_tree. it first sets _nj_draw_model_sub_ to
 * njDrawModelSub, which the shape steps draw each model with */
static void draw_lit_shape(NJS_OBJECT *object)
{
    _nj_draw_model_sub_ = njDrawModelSub;
    njSetQueue();
    njSetLightMaterials();
    njCalcLightMatrices();
    njGetkmPointer();
    draw_shape_tree(object);
    njSetkmPointer();
    njResetQueue();
}

static void draw_motion_tree(NJS_OBJECT *object)
{
    do {
        njPushMatrix(NULL);
        if (!(object->evalflags & NJD_EVAL_SKIP)) {
            _nj_motion_data_func_(object);
        } else {
            NJS_MATRIX *m = _nj_motion_data_info_.m;
            void (*rotate)(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz);
            Float x;
            Float y;
            Float z;
            Float *values;
            Angle angx;
            Angle angy;
            Angle angz;

            values = object->pos;
            x = *values++;
            y = *values++;
            z = *values;
            njTranslate(m, x, y, z);
            rotate = njRotateXYZ;
            if (object->evalflags & NJD_EVAL_ZXY_ANG) {
                rotate = njRotateZXY;
            }
            angx = object->ang[0];
            angy = object->ang[1];
            angz = object->ang[2];
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
        if (object->model && !(object->evalflags & NJD_EVAL_HIDE)) {
            njDrawModelSub(object->model);
        }
        if (object->child) {
            draw_motion_tree(object->child);
        }
        njPopMatrix(1);
        object = object->sibling;
    } while (object);
}

static void draw_shape_tree(NJS_OBJECT *object)
{
    do {
        njPushMatrix(NULL);
        if (!(object->evalflags & NJD_EVAL_SKIP)) {
            _nj_motion_data_func_(object);
        } else {
            NJS_MATRIX *m = _nj_motion_data_info_.m;
            void (*rotate)(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz);
            Float x;
            Float y;
            Float z;
            Float *values;
            Angle angx;
            Angle angy;
            Angle angz;

            values = object->pos;
            x = *values++;
            y = *values++;
            z = *values;
            njTranslate(m, x, y, z);
            rotate = njRotateXYZ;
            if (object->evalflags & NJD_EVAL_ZXY_ANG) {
                rotate = njRotateZXY;
            }
            angx = object->ang[0];
            angy = object->ang[1];
            angz = object->ang[2];
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
        if (!(object->evalflags & NJD_EVAL_SHAPE_SKIP)) {
            _nj_shape_data_func_(object);
        } else if (object->model && !(object->evalflags & NJD_EVAL_HIDE)) {
            njDrawModelSub(object->model);
        }
        if (object->child) {
            draw_shape_tree(object->child);
        }
        njPopMatrix(1);
        object = object->sibling;
    } while (object);
}

void njAction(NJS_ACTION *action, Float frame)
{
    NJS_MOTION *motion = action->motion;
    NJS_OBJECT *object = action->object;

    if (start_motion(motion, frame) == 0) {
        select_steps(object);
        draw_lit_motion(object);
    }
}

void njDrawMotion(NJS_OBJECT *object, NJS_MOTION *motion, Float frame)
{
    if (start_motion(motion, frame) == 0) {
        select_steps(object);
        draw_lit_motion(object);
    }
}

void njDrawShapeMotion(NJS_OBJECT *object, NJS_MOTION *motion,
                       NJS_MOTION *shape, Float frame)
{
    if (start_motion(motion, frame) == 0 && start_shape(shape, frame) == 0) {
        select_steps(object);
        draw_lit_shape(object);
    }
}

/* select_steps, with the angle interpolations of
 * _nj_motion_int_a_func_table_old_ */
static void select_steps_old(NJS_OBJECT *object)
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
     * 0x8c6156b8, puts #4 in R2 and _nj_motion_rotate_'s address in R3 at
     * 0x8c6156b2 and 0x8c6156b4; stored once after the if, as select_steps
     * stores it, R2 and R3 trade roles from there to the end and 12 of 56
     * words differ */
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

void njActionOld(NJS_ACTION *action, Float frame)
{
    NJS_MOTION *motion = action->motion;
    NJS_OBJECT *object = action->object;

    if (start_motion(motion, frame) == 0) {
        select_steps_old(object);
        draw_lit_motion(object);
    }
}

void njDrawMotionOld(NJS_OBJECT *object, NJS_MOTION *motion, Float frame)
{
    if (start_motion(motion, frame) == 0) {
        select_steps_old(object);
        draw_lit_motion(object);
    }
}

void njDrawShapeMotionOld(NJS_OBJECT *object, NJS_MOTION *motion,
                          NJS_MOTION *shape, Float frame)
{
    if (start_motion(motion, frame) == 0 && start_shape(shape, frame) == 0) {
        select_steps_old(object);
        draw_lit_shape(object);
    }
}
