/* an event's E-101, which draws the models each event defines */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/ground.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "event/event.h"
#include "shared/e101.h"
#include "shared/e101_textures.h"
#include "system/task.h"
#include "system/util.h"

/* the node of E-101's model the jet rides on, by its place in the walk */
#define E101_JET_NODE 54
#define E101_BOB_HEIGHT 1.0F /* how far the jet bobs above and below */
#define E101_BOB_RATE 0x400 /* how far the bob's phase turns each step */
/* where the jet sits from E101_JET_NODE before its bob, on his still model and
 * on a posed one */
#define E101_JET_STILL_X 5.5F
#define E101_JET_STILL_Y 5.0F
#define E101_JET_POSED_X 5.0F
#define E101_JET_POSED_Y 2.5F

struct e101_vars {
    /* how far the jet bobs from its rest, -E101_BOB_HEIGHT to
     * E101_BOB_HEIGHT */
    Float bob;
    Uint8 unk_04[4];
    Angle phase; /* the bob's phase */
};

#define VARS(work) ((struct e101_vars *)&(work)->vars)

/* his model, which each event that links e101.c defines. the draw draws it
 * while no motion is queued */
extern NJS_OBJECT event_e101_object;

/* E101_JET_NODE's matrix in his model, which each draw sets before it draws
 * the jet */
static NJS_MATRIX event_e101_jet_matrix;

static void event_e101_draw_jet(struct task_work *work);

static void event_e101_destroy(struct task *task)
{
}

static void event_e101_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct event_work *event;
    /* @hack spelling: the jet's matrix through a local, which SHC keeps in R13
     * through the draw (MOV.L at 0x0cb80c4e in EV00B4); without it, the code
     * runs 8 bytes short and 152 of the draw's 154 words differ */
    NJS_MATRIX *jet = &event_e101_jet_matrix;
    Angle ang;
    NJS_ACTION *action;
    Float frame;
    NJS_MATRIX *base;
    /* @hack redundant: 24 bytes of frame that no code reads, of a type unknown,
     * declared as under and slope. the image's ADD #-28,R15 reserves them
     * (0x0cb80c48 in EV00B4); without them, 3 of 154 words differ */
    NJS_POINT3 under;
    struct angles slope;

    event = work->event;
    if (event->motions != NULL) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        event_motions_draw(work);
        /* @hack spelling: the locals action, frame and base, assigned in that
         * order before the pose_action call. SHC then loads the motions before
         * the frame and keeps event in R12 (0x0cb80ca2 and 0x0cb80caa in
         * EV00B4); without them, 16 of 154 words differ */
        action = &event->motions->action;
        frame = event->motion_frame;
        base = &_unit_matrix_;
        pose_action(base, action, frame, E101_JET_NODE, jet);
        njPopMatrix(1);
        event_e101_draw_jet(work);
    } else {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njSetTexture(&event_e101_texlist);
        draw_set_palette(DRAW_PLAYER_PALETTE);
        draw_tree(&event_e101_object);
        njSetMatrix(jet, tree_calc_matrices(&_unit_matrix_, &event_e101_object,
                                            E101_JET_NODE));
        njMultiMatrix(NULL, jet);
        njTranslate(NULL, E101_JET_STILL_X, VARS(work)->bob + E101_JET_STILL_Y,
                    0.0F);
        draw_set_palette(DRAW_PLAYER_PALETTE);
        njSetTexture(&event_e101_jet_texlist);
        draw_tree(&event_e101_jet_object);
        draw_set_palette(0);
        njPopMatrix(1);
    }
}

static void event_e101_step(struct task *task)
{
    struct task_work *work = task->work;
    struct event_work *event = work->event;
    NJS_POINT3 under;
    struct angles slope;
    Float x;
    Float y;
    Float z;

    if (event->paths != NULL) {
        event_paths_step(work);
    }
    x = work->pos.x;
    y = work->pos.y;
    z = work->pos.z;
    under.y = ground_below(x, y, z, &slope);
    if (event->flags & EVENT_GROUND) {
        work->pos.y = under.y;
    }
    if (event->flags & EVENT_SLOPE) {
        work->ang.x = slope.x;
        work->ang.z = slope.z;
    }
    if (event->shadow > 0.0F) {
        under.x = x;
        under.y += SHADOW_LIFT;
        under.z = z;
        /* @hack spelling: work->event read again for the shadow's size (MOV.L
         * @(60,R14),R3 at 0x0cb80e56 in EV00B4), though R13 holds it as event;
         * without it, the step runs 4 bytes short and 36 of 92 words differ */
        shadow_draw_at(&slope, &under, work->event->shadow);
    }
    VARS(work)->bob = njSin(VARS(work)->phase) * E101_BOB_HEIGHT;
    VARS(work)->phase += E101_BOB_RATE;
    event_e101_draw(task);
}

static void event_e101_init(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->bob = 0.0F;
    VARS(work)->phase = 0;
    task->step = event_e101_step;
    task->draw = event_e101_draw;
    task->destructor = event_e101_destroy;
}

struct task *event_e101_create(void)
{
    struct task *task;

    task = task_create(TASK_WORK, 3, event_e101_init);
    if (task != NULL) {
        event_alloc(task->work);
    }
    return task;
}

static void event_e101_draw_jet(struct task_work *work)
{
    Angle ang;

    draw_set_palette(DRAW_PLAYER_PALETTE);
    njSetTexture(&event_e101_jet_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njMultiMatrix(NULL, &event_e101_jet_matrix);
    njTranslate(NULL, E101_JET_POSED_X, VARS(work)->bob + E101_JET_POSED_Y,
                0.0F);
    draw_tree(&event_e101_jet_object);
    njPopMatrix(1);
    draw_set_palette(0);
}
