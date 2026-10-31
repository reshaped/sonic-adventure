/* the copies E-101 mkII leaves. each arm's trail and the boss's poses are
 * kept in rings. a trail or a blur draws every other kept slot, back from
 * the slot two before the newest */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_e101_r/e101_mk2.h"
#include "b_e101_r/files.h"
#include "b_e101_r/trail.h"
#include "draw/draw.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the matrices each arm's trail keeps, and the poses kept: the last 32 of
 * each, in a ring. RING gives index's slot */
#define KEPT 32
#define RING(index) ((index) & (KEPT - 1))
/* the most copies a trail or a blur grows to */
#define COPIES 4
/* the slots e101_mk2_pose_add_warping fills with the pose it keeps */
#define WARP_POSES 9
/* a trail's first copy is drawn at TRAIL_SCALE, and a blur's at BLUR_SCALE.
 * each copy after it is COPY_GROWTH bigger */
#define TRAIL_SCALE 1.1F
#define BLUR_SCALE 1.0F
#define COPY_GROWTH 0.04F
/* the copies draw unlit, in the constant material. a trail's copies add
 * their light to what's behind them, and a blur's cover it by their alpha */
#define COPY_CONTROL                                                           \
    (NJD_CONTROL_3D_CONSTANT_ATTR | NJD_CONTROL_3D_ENABLE_ALPHA |              \
     NJD_CONTROL_3D_CONSTANT_MATERIAL)
#define TRAIL_ATTR                                                             \
    (NJD_SA_ONE | NJD_DA_ONE | NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_ALPHA)
#define BLUR_ATTR                                                              \
    (NJD_SA_ONE | NJD_DA_INV_SRC | NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_ALPHA)

/* a pose of the boss, as its work and its record held it */
struct pose {
    NJS_POINT3 pos;
    struct angles ang;
    Angle pitch;
    Angle roll;
    Float frame; /* its action's frame */
    Float sway;
};

/* an afterimage's record, its task's extra. pose points into the ring */
struct afterimage {
    struct pose *pose;
    NJS_ACTION *action;
    Float scale;
    Float alpha;
};

/* the brightness of a trail's copies, newest first. a trail of n copies
 * takes the table's last n */
static const Float trail_fades[COPIES] = {0.8F, 0.6F, 0.4F, 0.1F};
/* the colors of the arms' trails, which e101_mk2_trail_draw's color picks */
static const NJS_ARGB trail_colors[3] = {
#include "b_e101_r/trail/trail_colors.inc"
};
/* the alpha of a blur's copies, read as trail_fades is */
static const Float blur_alphas[COPIES] = {0.4F, 0.3F, 0.2F, 0.1F};

static NJS_OBJECT *trail_objects[2] = {
    &e101_mk2_trail_0_object,
    &e101_mk2_trail_1_object,
};

/* the sway of the pose being drawn, for pose_sway_node */
static Float pose_sway;
/* the newest slot of the poses' ring, and of each trail's */
static int pose_head;
static int trail_heads[2];
static struct pose poses[KEPT];
static NJS_MATRIX trail_matrices[2][KEPT];

void e101_mk2_trails_reset(struct task *task)
{
    int i;

    for (i = 0; i < 2; i++) {
        trail_heads[i] = 0;
    }
}

void e101_mk2_trail_add(struct e101_mk2 *boss, NJS_MATRIX *matrix, int arm)
{
    int head = RING(trail_heads[arm] + 1);
    NJS_MATRIX *to;
    /* @hack redundant: two points no code uses, which make room for 24 bytes
     * in the frame (ADD #-28,R15 at 0x0c9040fa); without them, 2 of 23 words
     * differ */
    NJS_POINT3 unused[2];

    trail_heads[arm] = head;
    to = &trail_matrices[arm][head];
    njSetMatrix(to, matrix);
}

void e101_mk2_trail_grow(struct e101_mk2 *boss, Sint32 flag, Sint32 *copies)
{
    int count = *copies;

    if (boss->flags & flag) {
        if (!(boss->steps & 1) && count < COPIES) {
            count++;
        }
    } else {
        if (!(boss->steps & 1) && count > 0) {
            count--;
        }
    }
    *copies = count;
}

/* draw object at matrix, scaled by scale, on a frame's first step only */
static void trail_draw_copy(NJS_MATRIX *matrix, NJS_OBJECT *object, Float scale)
{
    if (!frame_step) {
        njPushMatrix(NULL);
        njMultiMatrix(NULL, matrix);
        njScale(NULL, scale, scale, scale);
        njDrawObject(object);
        njPopMatrix(1);
    }
}

/* each copy is bigger than the one before it, and dimmer */
void e101_mk2_trail_draw(struct e101_mk2 *boss, int arm, int color)
{
    int index = trail_heads[arm] - 2;
    NJS_MATRIX *matrices = trail_matrices[arm];
    int first = COPIES - boss->trail_copies[arm];
    Float scale = TRAIL_SCALE;
    int count;
    Float bright;

    draw_control_save();
    draw_attr_save();
    njControl3D(COPY_CONTROL);
    njSetConstantAttr(~(NJD_SA_MASK | NJD_DA_MASK), TRAIL_ATTR);
    count = boss->trail_copies[arm];
    while (count > 0) {
        bright = trail_fades[first];
        first++;
        draw_set_material(bright, bright * trail_colors[color].r,
                          bright * trail_colors[color].g,
                          bright * trail_colors[color].b);
        index &= KEPT - 1;
        trail_draw_copy(&matrices[index], trail_objects[arm], scale);
        scale += COPY_GROWTH;
        count--;
        index -= 2;
    }
    draw_attr_restore();
    draw_control_restore();
}

void e101_mk2_poses_reset(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;
    int i;

    pose_head = 0;
    for (i = 0; i < KEPT; i++) {
        poses[i].pos = work->pos;
        poses[i].ang = work->ang;
        poses[i].pitch = boss->pitch;
        poses[i].roll = boss->roll;
        poses[i].frame = 0.0F;
        poses[i].sway = 0.0F;
    }
}

void e101_mk2_pose_add(struct task *task)
{
    int head;
    struct task_work *work;
    struct e101_mk2 *boss;

    head = RING(pose_head + 1);
    /* @hack spelling: boss and work set in one statement, which loads work into
     * R6 and boss into R5 (0x0c9043c4, 0x0c9043ca); as two statements, 12 of
     * 45 words differ */
    boss = task->extra, work = task->work;
    pose_head = head;
    poses[head].pos = work->pos;
    poses[head].ang = work->ang;
    poses[head].pitch = boss->pitch;
    poses[head].roll = boss->roll;
    poses[head].frame = boss->motion.frame;
    poses[head].sway = boss->sway;
}

void e101_mk2_pose_add_warping(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;
    /* @hack spelling: head read from pose_head ahead of the RING step, which
     * keeps the old head in the frame (MOV.L R3,@R15 at 0x0c90445c), where
     * nothing reads it; as int head = RING(pose_head + 1), 130 of 134 words
     * differ */
    int head = pose_head;
    int i;

    head = RING(head + 1);
    pose_head = head;
    for (i = 0; i < WARP_POSES; i++) {
        poses[head].pos = work->pos;
        poses[head].ang = work->ang;
        poses[head].pitch = boss->pitch;
        poses[head].roll = boss->roll;
        poses[head].frame = boss->motion.frame;
        poses[head].sway = boss->sway;
        head = RING(head - 1);
    }
}

/* the motion callback: move SWAY_OBJECT along its z by the sway of the pose
 * drawn */
static void pose_sway_node(NJS_OBJECT *object)
{
    if (object == (NJS_OBJECT *)(e101_r_models + SWAY_OBJECT)) {
        njTranslate(NULL, 0.0F, 0.0F, pose_sway);
    }
}

/* draw action at pose, scaled by scale, on a frame's first step only. alpha
 * sets all four of the constant material's channels */
static void pose_draw(struct pose *pose, NJS_ACTION *action, Float scale,
                      Float alpha)
{
    Angle ang;

    pose_sway = pose->sway;
    if (!frame_step) {
        draw_set_material(alpha, alpha, alpha, alpha);
        njPushMatrix(NULL);
        njTranslateV(NULL, &pose->pos);
        TURN_VIA(njRotateZ, pose->roll, ang);
        TURN_VIA(njRotateX, pose->pitch, ang);
        TURN_VIA(njRotateZ, pose->ang.z, ang);
        TURN_VIA(njRotateY, -pose->ang.y, ang);
        TURN_VIA(njRotateX, pose->ang.x, ang);
        njScale(NULL, scale, scale, scale);
        njAction(action, pose->frame);
        njPopMatrix(1);
    }
}

/* each copy is bigger than the one before it, and fainter */
void e101_mk2_blur_draw(struct e101_mk2 *boss, NJS_ACTION *action)
{
    int index = pose_head - 2;
    int first = COPIES - boss->blur_copies;
    Float scale = BLUR_SCALE;
    int count;

    njControl3D(COPY_CONTROL);
    njSetConstantAttr(0xffffffff, BLUR_ATTR);
    njSetMotionCallback(pose_sway_node);
    count = boss->blur_copies;
    while (count > 0) {
        index &= KEPT - 1;
        pose_draw(&poses[index], action, scale, blur_alphas[first]);
        first++;
        scale += COPY_GROWTH;
        count--;
        index -= 2;
    }
    njSetMotionCallback(NULL);
}

static void afterimage_draw(struct task *task)
{
    struct afterimage *image = task->extra;

    pose_sway = image->pose->sway;
    draw_control_save();
    draw_attr_save();
    njControl3D(COPY_CONTROL);
    njSetConstantAttr(0xffffffff, NJD_FLAG_USE_ALPHA);
    njSetTexture(e101_r_texlist);
    njSetMotionCallback(pose_sway_node);
    pose_draw(image->pose, image->action, image->scale, image->alpha);
    njSetMotionCallback(NULL);
    draw_attr_restore();
    draw_control_restore();
}

static void afterimage_step(struct task *task)
{
    struct afterimage *image = task->extra;

    image->scale *= 1.05F;
    image->alpha -= 0.05F;
    if (image->alpha <= 0.0F) {
        task_kill(task);
    } else {
        afterimage_draw(task);
    }
}

void e101_mk2_afterimage_create(struct e101_mk2 *boss, NJS_ACTION *action)
{
    struct task *task;
    struct afterimage *image;

    if ((task = task_create(TASK_WORK, 4, afterimage_step)) != NULL) {
        image = heap_alloc(sizeof(struct afterimage));
        if (image) {
            task->extra = image;
            /* @bug memory: with the newest pose in slot 0, the index
             * is -1, from ADD #-1,R3 at 0x0c90483e. the afterimage then draws
             * the 40 bytes before poses, at 0x0c92e848, as its pose */
#ifdef GCC
            /* repairs the index the tag names: RING takes it to the ring's
             * last slot, which holds the pose before the newest */
            image->pose = &poses[RING(pose_head - 1)];
#else
            image->pose = &poses[pose_head - 1];
#endif
            image->action = action;
            image->scale = 1.0F;
            image->alpha = 0.5F;
        } else {
            task_destroy(task);
        }
    }
}
