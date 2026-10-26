/* O HELI0, Chaos 0's helicopter. it bobs up and down where it's placed. its
 * searchlight, drawn as a lens flare, shines down ahead of it */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos0/helicopter.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "effect/lens.h"
#include "object/common.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION chaos0_helicopter_action;

/* how far the helicopter rises above and sinks below where it's placed */
#define BOB_HEIGHT 10.0F
/* the bob's angle turns less than BOB_TURN a step, but at least half of it.
 * the turn is chosen at random as the helicopter starts */
#define BOB_TURN 256.0F
/* the helicopter plays chaos0_helicopter_action's first HELICOPTER_FRAMES
 * frames, one a step, and then starts over */
#define HELICOPTER_FRAMES 8.0F
/* the searchlight shines 60 degrees down from the helicopter's -x, and turns
 * with its y angle alone */
#define LIGHT_TILT 0x2aaa

struct helicopter_vars {
    Float frame;
    Uint8 unk_04[4];
    /* @hack type: the bob's angle and its turn a step in an array, whose
     * address helicopter_run takes (ADD #16,R4 at 0x0c903f86) to read both
     * through it. as two Sint16 fields, 65 of the 191 words of
     * helicopter_draw, helicopter_run and chaos0_helicopter_step differ */
    Sint16 bob[2];
};

#define HELICOPTER_VARS(work) ((struct helicopter_vars *)&(work)->vars)

static void helicopter_destroy(struct task *task);
static void helicopter_draw(struct task *task);
static void helicopter_run(struct task *task);

/* the way the searchlight shines in the helicopter's frame before its tilt,
 * and the way it shines in the world, which helicopter_draw sets */
static NJS_VECTOR searchlight_ahead = {-1.0F, 0.0F, 0.0F};
static NJS_VECTOR searchlight_dir;

static void helicopter_destroy(struct task *task)
{
}

static void helicopter_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float bob;
    Angle ang;
    NJS_POINT3 pos;

    bob = njSin(HELICOPTER_VARS(work)->bob[0]) * BOB_HEIGHT;
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njTranslate(NULL, 0.0F, bob, 0.0F);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(&chaos0_helicopter_action, HELICOPTER_VARS(work)->frame);
    njPopMatrix(1);
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njRotateZ(NULL, LIGHT_TILT);
    njCalcVector(NULL, &searchlight_ahead, &searchlight_dir);
    njPopMatrix(1);
    pos = work->pos;
    pos.y += bob;
    lens_draw_facing(&pos, &searchlight_dir);
}

static void helicopter_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        if (!placement_editing(task)) {
            object_clear_touch(work, NULL);
        }
        HELICOPTER_VARS(work)->bob[0] += HELICOPTER_VARS(work)->bob[1];
        HELICOPTER_VARS(work)->frame += 1.0F;
        if (HELICOPTER_VARS(work)->frame >= HELICOPTER_FRAMES) {
            HELICOPTER_VARS(work)->frame -= HELICOPTER_FRAMES;
        }
        helicopter_draw(task);
    }
}

void chaos0_helicopter_step(struct task *task)
{
    struct task_work *work = task->work;

    HELICOPTER_VARS(work)->bob[0] = 0;
    HELICOPTER_VARS(work)->bob[1] = RANDOM(0.5F, 1.0F) * BOB_TURN;
    HELICOPTER_VARS(work)->frame = 0.0F;
    task->destructor = helicopter_destroy;
    task->step = helicopter_run;
    task->draw = helicopter_draw;
}
