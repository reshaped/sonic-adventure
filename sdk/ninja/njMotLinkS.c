/* the motion link draws, which draw their models with njSimpleDrawModel. a
 * link blends two motions, its slots 0 and 1, to pose the tree, and a shape
 * link blends two shape motions into the models' points and normals.
 * njSimpleActionLinkOld, njSimpleDrawMotionLinkOld and
 * njSimpleDrawShapeMotionLinkOld draw as the forms without Old do: unlike
 * njMotion.c's Old forms, they take no angle the short way round */
#include <ninja.h>
#include <sg_xpt.h>

/* set up slot's shape motion at its frame in the link, and choose the slot's
 * shape steps by the motion's type and interpolation. a shape motion sets
 * _nj_mot_shape_spline_cnt_, which njMakeShapeLink blends by, to its
 * interpolation. with a null one, that global keeps its value, and the slot
 * uses each model's own points and normals. returns 1, with nothing set up,
 * when the frame is not before the motion's nbFrame, else 0 */
static Uint32 link_start_shape(NJS_MOTION_LINK *link, Int slot)
{
    struct nj_motlink_data_info *info = &_nj_shapelink_data_info_[slot];
    NJS_MOTION *motion = link->motion[slot];
    Uint16 parts = 0;
    Uint16 type = 0;
    Uint16 spline = _nj_mot_shape_spline_cnt_;

    if (motion != NULL) {
        if (!(link->frame[slot] < (Sint32)motion->nbFrame)) {
            return 1;
        }
        info->frame = link->frame[slot];
        info->mdata = motion->mdata;
        info->nbFrame = motion->nbFrame;
        type = motion->type;
        spline = _nj_mot_shape_spline_cnt_ =
            (motion->inp_fn & NJD_MTYPE_MASK) > 0;
    }
    info->factor = 0;
    info->unk_18 = 0;
    if (type & NJD_MTYPE_VERT_4) {
        parts = SHAPE_POINTS;
    }
    if (type & NJD_MTYPE_NORM_5) {
        parts += SHAPE_NORMALS;
    }
    _nj_shapelink_data_func_[slot] = _nj_shapelink_data_func_table_[parts];
    _nj_motion_shape_data_[slot] = _nj_shapelink_int_func_table[spline];
    _nj_motlink_shape_data_cnt_[slot] = parts;
    _nj_motlink_shape_data_type_[slot] = type;
    return 0;
}

/* set up slot's motion at its frame in the link, with its factor count and
 * type. a motion sets _nj_mot_spline_cnt_, which link_select_steps chooses
 * both slots' interpolations and blends by, to its interpolation. with a null
 * one, that global keeps its value, and the slot poses each node by its own
 * pos, ang and scl. returns 1, with nothing set up, when the frame is not
 * before the motion's nbFrame, else 0 */
static Uint32 link_start_motion(NJS_MOTION_LINK *link, Int slot)
{
    struct nj_motlink_data_info *info = &_nj_motlink_data_info_[slot];
    NJS_MOTION *motion = link->motion[slot];
    Uint16 type = 0;
    Uint16 factors = 0;

    if (motion != NULL) {
        if (!(link->frame[slot] < (Sint32)motion->nbFrame)) {
            return 1;
        }
        factors = motion->inp_fn & ~NJD_MTYPE_MASK;
        _nj_mot_spline_cnt_ = (motion->inp_fn & NJD_MTYPE_MASK) > 0;
        type = motion->type;
        info->mdata = motion->mdata;
        info->nbFrame = motion->nbFrame;
        info->frame = link->frame[slot];
    }
    info->factor = 0;
    info->unk_18 = 0;
    _nj_motlink_data_cnt_[slot] = factors;
    _nj_motlink_data_type_[slot] = type;
    return 0;
}

/* choose slot's node step by its factor count. each of its position, angle and
 * scale steps reads keys where the slot's type has that factor, and the node's
 * own value otherwise. also set what both slots share: the rotation order by
 * object's NJD_EVAL_ZXY_ANG, and the interpolations and blends by
 * _nj_mot_spline_cnt_ */
static void link_select_steps(NJS_OBJECT *object, Int slot)
{
    Uint32 type;
    void (*rotate)(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz);
    void (*step)(NJS_OBJECT *object);

    _nj_motlink_data_func_[slot] =
        _nj_motlink_data_func_table_[_nj_motlink_data_cnt_[slot]];
    rotate = njRotateXYZ;
    if (object->evalflags & NJD_EVAL_ZXY_ANG) {
        rotate = njRotateZXY;
    }
    _nj_rotate_func_ = rotate;
    type = _nj_motlink_data_type_[slot];
    step = njMotionObjTranslateData;
    if (type & NJD_MTYPE_POS_0) {
        step = njMotionIntTranslateData;
    }
    _nj_motion_translate_data_[slot] = step;
    step = njMotionObjRotateData;
    if (type & NJD_MTYPE_ANG_1) {
        step = njMotionIntRotateData;
    }
    _nj_motion_rotate_data_[slot] = step;
    step = njMotionObjScaleData;
    if (type & NJD_MTYPE_SCL_2) {
        step = njMotionIntScaleData;
    }
    _nj_motion_scale_data_[slot] = step;
    _nj_motion_int_f_func_ = _nj_motlink_int_f_func_table_[_nj_mot_spline_cnt_];
    _nj_motion_int_a_func_ = _nj_motlink_int_a_func_table_[_nj_mot_spline_cnt_];
    _nj_int_f_func_ = _nj_int_f_func_table_[_nj_mot_spline_cnt_];
    _nj_int_a_func_ = _nj_int_a_func_table_[_nj_mot_spline_cnt_];
}

/* draw object and its siblings, each with its children in its own matrix.
 * njMakeMotionLink moves a node's matrix by the blend of the slots. under
 * NJD_EVAL_SKIP, the node's own pos, ang and scl move it, in its own rotation
 * order */
static void link_draw_motion_tree(NJS_OBJECT *object)
{
    Uint32 flags;
    void (*rotate)(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz);
    Float *vector;
    Float x;
    Float y;
    Float z;
    Angle *angles;
    Angle ax;
    Angle ay;
    Angle az;
    NJS_MATRIX *matrix;

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
            njMakeMotionLink();
        } else {
            /* @hack spelling: the null matrix through a local, which SHC keeps
             * in R11 for njTranslate, the rotation and njScale (MOV #0,R11 at
             * 0x8c6176fe; MOV R11,R4 at 0x8c61770c, 0x8c617724 and
             * 0x8c617734); passed as 0 or NULL, each call takes MOV #0,R4 in
             * its slot and 51 of 112 words differ */
            /* @hack type: the local set to a plain 0: set to NULL, it builds as
             * the null passed directly does, and 51 of 112 words differ */
            matrix = 0;
            /* @hack spelling: pos and scl read through a stepping pointer, and
             * ang through angles into locals, which SHC loads by @R4+
             * (0x8c617704 and 0x8c61772c) and from @R4 up (0x8c61771c to
             * 0x8c617720); indexed in the calls, the function is 6 bytes longer
             * and 101 of 112 words differ */
            vector = object->pos;
            x = *vector++;
            y = *vector++;
            z = *vector;
            njTranslate(matrix, x, y, z);
            rotate = njRotateXYZ;
            if (flags & NJD_EVAL_ZXY_ANG) {
                rotate = njRotateZXY;
            }
            angles = object->ang;
            ax = angles[0];
            ay = angles[1];
            az = angles[2];
            rotate(matrix, ax, ay, az);
            vector = object->scl;
            x = *vector++;
            y = *vector++;
            z = *vector;
            njScale(matrix, x, y, z);
        }
        if (_nj_motion_callback_) {
            _nj_motion_callback_(object);
        }
        if (object->model && !(flags & NJD_EVAL_HIDE)) {
            njSimpleDrawModel(object->model);
        }
        if (object->child) {
            link_draw_motion_tree(object->child);
        }
        njPopMatrix(1);
        object = object->sibling;
    } while (object);
}

/* link_draw_motion_tree, but the two shape slots' steps run on each node,
 * and njMakeShapeLink then draws its model. under NJD_EVAL_SHAPE_SKIP the
 * model is drawn as link_draw_motion_tree draws it */
static void link_draw_shape_tree(NJS_OBJECT *object)
{
    Uint32 flags;
    void (*rotate)(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz);
    Float *vector;
    Float x;
    Float y;
    Float z;
    Angle *angles;
    Angle ax;
    Angle ay;
    Angle az;
    NJS_MATRIX *matrix;

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
            njMakeMotionLink();
        } else {
            /* @hack spelling: the null matrix through a local, which SHC keeps
             * in R9 for njTranslate, the rotation and njScale (MOV #0,R9 at
             * 0x8c61787e; MOV R9,R4 at 0x8c61788c, 0x8c6178a4 and
             * 0x8c6178b4); passed as 0 or NULL, each call takes MOV #0,R4 in
             * its slot, the function is 2 bytes longer, and 126 of 140 words
             * differ */
            /* @hack type: the local set to a plain 0: set to NULL, it builds as
             * the null passed directly does, and 126 of 140 words differ */
            matrix = 0;
            /* @hack spelling: pos and scl read through a stepping pointer, and
             * ang through angles into locals, which SHC loads by @R4+
             * (0x8c617884 and 0x8c6178ac) and from @R4 up (0x8c61789c to
             * 0x8c6178a0); indexed in the calls, the function is 12 bytes
             * longer and 97 of 140 words differ */
            vector = object->pos;
            x = *vector++;
            y = *vector++;
            z = *vector;
            njTranslate(matrix, x, y, z);
            rotate = njRotateXYZ;
            if (flags & NJD_EVAL_ZXY_ANG) {
                rotate = njRotateZXY;
            }
            angles = object->ang;
            ax = angles[0];
            ay = angles[1];
            az = angles[2];
            rotate(matrix, ax, ay, az);
            vector = object->scl;
            x = *vector++;
            y = *vector++;
            z = *vector;
            njScale(matrix, x, y, z);
        }
        if (_nj_motion_callback_) {
            _nj_motion_callback_(object);
        }
        if (!(flags & NJD_EVAL_SHAPE_SKIP)) {
            _nj_pnt_int_flag_ = 0;
            _nj_nml_int_flag_ = 0;
            _nj_mot_current_data_ = 0;
            _nj_shapelink_data_func_[0](object);
            _nj_mot_current_data_ = 1;
            _nj_shapelink_data_func_[1](object);
            njMakeShapeLink(object);
        } else if (object->model && !(flags & NJD_EVAL_HIDE)) {
            njSimpleDrawModel(object->model);
        }
        if (object->child) {
            link_draw_shape_tree(object->child);
        }
        njPopMatrix(1);
        object = object->sibling;
    } while (object);
}

void njSimpleActionLink(NJS_ACTION_LINK *actionlink, Float rate)
{
    NJS_OBJECT *object;
    NJS_MOTION_LINK *motionlink;

    if (rate > 1.0F) {
        return;
    }
    motionlink = actionlink->motionlink;
    object = actionlink->object;
    _nj_mot_current_data_ = 0;
    _nj_mot_current_frame_ = rate;
    if (link_start_motion(motionlink, 0)) {
        return;
    }
    if (link_start_motion(motionlink, 1)) {
        return;
    }
    link_select_steps(object, 0);
    link_select_steps(object, 1);
    njGetkmPointer();
    link_draw_motion_tree(object);
    njSetkmPointer();
}

void njSimpleDrawMotionLink(NJS_OBJECT *object, NJS_MOTION_LINK *motionlink,
                            Float rate)
{
    if (rate > 1.0F) {
        return;
    }
    _nj_mot_current_data_ = 0;
    _nj_mot_current_frame_ = rate;
    if (link_start_motion(motionlink, 0)) {
        return;
    }
    if (link_start_motion(motionlink, 1)) {
        return;
    }
    link_select_steps(object, 0);
    link_select_steps(object, 1);
    njGetkmPointer();
    link_draw_motion_tree(object);
    njSetkmPointer();
}

void njSimpleDrawShapeMotionLink(NJS_OBJECT *object,
                                 NJS_MOTION_LINK *motionlink,
                                 NJS_MOTION_LINK *shapelink, Float rate)
{
    if (rate > 1.0F) {
        return;
    }
    _nj_mot_current_data_ = 0;
    _nj_mot_current_frame_ = rate;
    if (link_start_motion(motionlink, 0)) {
        return;
    }
    if (link_start_motion(motionlink, 1)) {
        return;
    }
    if (link_start_shape(shapelink, 0)) {
        return;
    }
    if (link_start_shape(shapelink, 1)) {
        return;
    }
    link_select_steps(object, 0);
    link_select_steps(object, 1);
    njGetkmPointer();
    _nj_draw_model_sub_ = njSimpleDrawModel;
    link_draw_shape_tree(object);
    njSetkmPointer();
}

void njSimpleActionLinkOld(NJS_ACTION_LINK *actionlink, Float rate)
{
    NJS_OBJECT *object;
    NJS_MOTION_LINK *motionlink;

    if (rate > 1.0F) {
        return;
    }
    motionlink = actionlink->motionlink;
    object = actionlink->object;
    _nj_mot_current_data_ = 0;
    _nj_mot_current_frame_ = rate;
    if (link_start_motion(motionlink, 0)) {
        return;
    }
    if (link_start_motion(motionlink, 1)) {
        return;
    }
    link_select_steps(object, 0);
    link_select_steps(object, 1);
    njGetkmPointer();
    link_draw_motion_tree(object);
    njSetkmPointer();
}

void njSimpleDrawMotionLinkOld(NJS_OBJECT *object, NJS_MOTION_LINK *motionlink,
                               Float rate)
{
    if (rate > 1.0F) {
        return;
    }
    _nj_mot_current_data_ = 0;
    _nj_mot_current_frame_ = rate;
    if (link_start_motion(motionlink, 0)) {
        return;
    }
    if (link_start_motion(motionlink, 1)) {
        return;
    }
    link_select_steps(object, 0);
    link_select_steps(object, 1);
    njGetkmPointer();
    link_draw_motion_tree(object);
    njSetkmPointer();
}

void njSimpleDrawShapeMotionLinkOld(NJS_OBJECT *object,
                                    NJS_MOTION_LINK *motionlink,
                                    NJS_MOTION_LINK *shapelink, Float rate)
{
    if (rate > 1.0F) {
        return;
    }
    _nj_mot_current_data_ = 0;
    _nj_mot_current_frame_ = rate;
    if (link_start_motion(motionlink, 0)) {
        return;
    }
    if (link_start_motion(motionlink, 1)) {
        return;
    }
    if (link_start_shape(shapelink, 0)) {
        return;
    }
    if (link_start_shape(shapelink, 1)) {
        return;
    }
    link_select_steps(object, 0);
    link_select_steps(object, 1);
    njGetkmPointer();
    _nj_draw_model_sub_ = njSimpleDrawModel;
    link_draw_shape_tree(object);
    njSetkmPointer();
}
