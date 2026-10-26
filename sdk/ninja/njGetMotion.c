/* the positions, angles and scales a motion gives each node of a tree, written
 * out rather than drawn */
#include <ninja.h>
#include <sg_xpt.h>

/* set up the motion at frame, with its factor count, interpolation and type.
 * a null motion has no factors and no type, so each node gives its own pos,
 * ang and scl. returns 1, with nothing set up, when frame is not below the
 * motion's nbFrame, else 0 */
static int start_motion_data(NJS_MOTION *motion, Float frame)
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
    /* @hack type: m set to a plain 0, not to NULL, whose cast to void * gives
     * the zero locals other registers and masks inp_fn with AND #192,R0 in
     * place of the MOV.W of NJD_MTYPE_MASK at 0x8c6163bc; with NULL, the object
     * is 8 bytes shorter and 27 of 42 words differ */
    info->m = 0;
    _nj_mot_data_cnt_ = count;
    _nj_mot_spline_cnt_ = spline;
    _nj_mot_data_type_ = type;
    return 0;
}

/* choose the node step by the motion's factor count. each of the position,
 * angle and scale steps reads keys where the motion's type has that factor,
 * and the node's own pos, ang or scl otherwise. the interpolations follow
 * _nj_mot_spline_cnt_ */
static void select_data_steps(void)
{
    Uint32 type = _nj_mot_data_type_;
    void (*step)(NJS_OBJECT *obj);

    _nj_motion_data_func_ = _nj_motion_data_func_table_[_nj_mot_data_cnt_];
    step = njGetMotionObjTranslate;
    if (type & NJD_MTYPE_POS_0) {
        step = njGetMotionIntTranslate;
    }
    _nj_motion_translate_ = step;
    step = njGetMotionObjRotate;
    if (type & NJD_MTYPE_ANG_1) {
        step = njGetMotionIntRotate;
    }
    _nj_motion_rotate_ = step;
    step = njGetMotionObjScale;
    if (type & NJD_MTYPE_SCL_2) {
        step = njGetMotionIntScale;
    }
    _nj_motion_scale_ = step;
    _nj_motion_int_f_func_ = _nj_motion_int_f_func_table_[_nj_mot_spline_cnt_];
    _nj_motion_int_a_func_ = _nj_motion_int_a_func_table_[_nj_mot_spline_cnt_];
}

/* set *index to the last of keys[0..end) whose keyframe is at most the
 * motion's frame, truncated and compared unsigned, or to 0 when none is. end
 * must be at least 1, and the keys in keyframe order. keys holds NJS_MKEY_F or
 * NJS_MKEY_A, both 16 bytes with the keyframe first */
static void find_key(void *keys, Uint32 end, Uint32 *index)
{
    Uint32 begin = 0;
    Uint32 middle;
    Uint32 key;
    Uint32 frame = (Sint32)_nj_motion_data_info_.frame;

    while (end - begin - 1 > 0) {
        middle = (begin + end) >> 1;
        key = ((const NJS_MKEY_F *)keys)[middle].keyframe;
        if (frame >= key) {
            begin = middle;
        } else {
            end = middle;
        }
    }
    *index = begin;
}

void njGetMotionIntTranslate(NJS_OBJECT *object)
{
    struct nj_motion_info *info = &_nj_motion_data_info_;
    void *keys;
    Uint32 count;
    Uint32 index;
    Uint32 factor;

    factor = info->factor;
    keys = info->p[factor];
    if (keys) {
        count = info->nb[factor];
        find_key(keys, count, &index);
        _nj_motion_int_f_func_(keys, index, count, _nj_mot_pos_buf_ptr_);
    } else {
        Float *source = object->pos;
        Float x = *source++;
        Float y = *source++;
        Float z = *source;
        Float *destination = _nj_mot_pos_buf_ptr_ + 3;

        *--destination = z;
        *--destination = y;
        *--destination = x;
    }
    _nj_mot_pos_buf_ptr_ += 3;
    info->factor = factor + 1;
}

void njGetMotionIntScale(NJS_OBJECT *object)
{
    struct nj_motion_info *info = &_nj_motion_data_info_;
    void *keys;
    Uint32 count;
    Uint32 index;
    Uint32 factor;

    factor = info->factor;
    keys = info->p[factor];
    if (keys) {
        count = info->nb[factor];
        find_key(keys, count, &index);
        _nj_motion_int_f_func_(keys, index, count, _nj_mot_scl_buf_ptr_);
    } else {
        Float *source = object->scl;
        Float x = *source++;
        Float y = *source++;
        Float z = *source;
        Float *destination = _nj_mot_scl_buf_ptr_ + 3;

        *--destination = z;
        *--destination = y;
        *--destination = x;
    }
    _nj_mot_scl_buf_ptr_ += 3;
    info->factor = factor + 1;
}

void njGetMotionIntRotate(NJS_OBJECT *object)
{
    struct nj_motion_info *info = &_nj_motion_data_info_;
    void *keys;
    Uint32 count;
    Uint32 index;
    Uint32 factor;

    factor = info->factor;
    keys = info->p[factor];
    if (keys) {
        count = info->nb[factor];
        find_key(keys, count, &index);
        _nj_motion_int_a_func_(keys, index, count, _nj_mot_ang_buf_ptr_);
    } else {
        Angle *source = object->ang;
        Angle x = *source++;
        Angle y = *source++;
        Angle z = *source;
        Angle *destination = _nj_mot_ang_buf_ptr_ + 3;

        *--destination = z;
        *--destination = y;
        *--destination = x;
    }
    _nj_mot_ang_buf_ptr_ += 3;
    info->factor = factor + 1;
}

void njGetMotionObjTranslate(NJS_OBJECT *object)
{
    Float *source = object->pos;
    Float x = *source++;
    Float y = *source++;
    Float z = *source;
    Float *destination;

    _nj_mot_pos_buf_ptr_ += 3;
    destination = _nj_mot_pos_buf_ptr_;
    *--destination = z;
    *--destination = y;
    *--destination = x;
}

void njGetMotionObjRotate(NJS_OBJECT *object)
{
    Angle *source = object->ang;
    Angle x = *source++;
    Angle y = *source++;
    Angle z = *source;
    Angle *destination;

    _nj_mot_ang_buf_ptr_ += 3;
    destination = _nj_mot_ang_buf_ptr_;
    *--destination = z;
    *--destination = y;
    *--destination = x;
}

void njGetMotionObjScale(NJS_OBJECT *object)
{
    Float *source = object->scl;
    Float x = *source++;
    Float y = *source++;
    Float z = *source;
    Float *destination;

    _nj_mot_scl_buf_ptr_ += 3;
    destination = _nj_mot_scl_buf_ptr_;
    *--destination = z;
    *--destination = y;
    *--destination = x;
}

/* write the position, angles and scale of each node from object on, each node
 * before its children and they before its siblings. the node step gives them,
 * or under NJD_EVAL_SKIP the node's own pos, ang and scl */
static void gather_motion_data(NJS_OBJECT *object)
{
    do {
        njPushMatrix(NULL);
        if (!(object->evalflags & NJD_EVAL_SKIP)) {
            _nj_motion_data_func_(object);
        } else {
            Float *source = object->pos;
            Float x = *source++;
            Float y = *source++;
            Float z = *source;
            Float *destination;
            Angle *angles;
            Angle angx;
            Angle angy;
            Angle angz;
            Angle *angle_out;

            _nj_mot_pos_buf_ptr_ += 3;
            destination = _nj_mot_pos_buf_ptr_;
            *--destination = z;
            *--destination = y;
            *--destination = x;
            angles = object->ang;
            angx = *angles++;
            angy = *angles++;
            angz = *angles;
            _nj_mot_ang_buf_ptr_ += 3;
            angle_out = _nj_mot_ang_buf_ptr_;
            *--angle_out = angz;
            *--angle_out = angy;
            *--angle_out = angx;
            source = object->scl;
            x = *source++;
            y = *source++;
            z = *source;
            _nj_mot_scl_buf_ptr_ += 3;
            destination = _nj_mot_scl_buf_ptr_;
            *--destination = z;
            *--destination = y;
            *--destination = x;
        }
        if (object->child) {
            gather_motion_data(object->child);
        }
        njPopMatrix(1);
        object = object->sibling;
    } while (object);
}

void njGetAction(NJS_ACTION *action, NJS_MOTION_DATA *data, Float frame)
{
    NJS_MOTION *motion = action->motion;
    NJS_OBJECT *object = action->object;

    _nj_mot_pos_buf_ptr_ = data->pos;
    _nj_mot_ang_buf_ptr_ = data->ang;
    _nj_mot_scl_buf_ptr_ = data->scl;
    if (start_motion_data(motion, frame) == 0) {
        select_data_steps();
        gather_motion_data(object);
    }
}

void njGetDrawMotion(NJS_OBJECT *object, NJS_MOTION *motion,
                     NJS_MOTION_DATA *data, Float frame)
{
    _nj_mot_pos_buf_ptr_ = data->pos;
    _nj_mot_ang_buf_ptr_ = data->ang;
    _nj_mot_scl_buf_ptr_ = data->scl;
    if (start_motion_data(motion, frame) == 0) {
        select_data_steps();
        gather_motion_data(object);
    }
}
