/* O LIGHTCHANGER box that blends the act's lighting palette when a player
 * comes near */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/light_changer.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/palette.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "system/task.h"
#include "system/util.h"

/* the event flag that keeps the way the light changers last blended:
 * CHANGER_OUT or CHANGER_IN */
#define FLAG_LIGHTS_WAY 5

/* the box's half size on each axis at a scale of 0. each unit of the
 * placement's scale adds as much again */
#define BOX_HALF 5.0F

/* the share the blend gains each step */
#define BLEND_SPEED 0.02F

enum light_changer_mode { CHANGER_WAIT, CHANGER_BLEND };

/* the way a changer blends palette 0: CHANGER_OUT toward palette 4, and
 * CHANGER_IN toward palette 5. WAY keeps it in work->submode */
#define CHANGER_OUT 0
#define CHANGER_IN 1
#define WAY(work) ((work)->submode)

/* SHARE keeps the blend's share, from 0 to 1, in scl.x once blending starts.
 * the marker cube's width reads scl.x too, so the cube follows the share from
 * then on */
#define SHARE(work) ((work)->scl.x)
struct light_changer_vars {
    NJS_VECTOR half; /* the box's half size on each axis */
};

#define VARS(work) ((struct light_changer_vars *)&(work)->vars)

/* the set editor's column where the step prints the way, "OUT" or "IN", on
 * angle x's row */
#define EDITOR_COLUMN_WAY 24

static void light_changer_step(struct task *task);

void light_changer_init(struct task *task)
{
    struct task_work *work = task->work;
    Float x;
    Float y;
    Float z;

    WAY(work) = (Sint8)_builtin_fabsf((Float)work->ang.x) % 2;
    work->mode = CHANGER_WAIT;
    x = (_builtin_fabsf(work->scl.x) + 1.0F) * BOX_HALF;
    y = (_builtin_fabsf(work->scl.y) + 1.0F) * BOX_HALF;
    z = (_builtin_fabsf(work->scl.z) + 1.0F) * BOX_HALF;
    VARS(work)->half.x = x;
    VARS(work)->half.y = y;
    VARS(work)->half.z = z;
    task->step = light_changer_step;
}

/* draw the marker cube on the box each step, editing or not. in the set editor,
 * label the placement's fields and stop. otherwise, once a player comes within
 * the box's half height of its position, start a blend if the changers last
 * blended the other way. a CHANGER_OUT blend moves palette 0 from palette 5 to
 * palette 4 as the share rises from 0 to 1. a CHANGER_IN blend moves it from 4
 * to 5 */
static void light_changer_step(struct task *task)
{
    struct task_work *work = task->work;
    Float x;
    Float y;
    Float z;
    Float share;
    Angle ang;
    /* @hack redundant: way declared register, which puts z at R15+4 and ang
     * at R15 (FMOV.S FR3,@(R0,R15) at 0x0c91225a, MOV.L R3,@R15 at
     * 0x0c912272); without it, the two trade places and 116 of 243 words
     * differ */
    register int way;
    char *text;
    /* @hack spelling: 1.0F and 0.0F held in one and zero, set first, which
     * loads them into FR15 and FR14 before njPushMatrix's call (FLDI1 FR15
     * and FLDI0 FR14 at 0x0c912222 and 0x0c912224); as constants where they
     * are used, 32 of 243 words differ */
    Float one;
    Float zero;

    if (placement_try_leave(task)) {
        return;
    }
    zero = 0.0F;
    one = 1.0F;
    njPushMatrix(NULL);
    x = (_builtin_fabsf(work->scl.x) + one) * BOX_HALF;
    y = (_builtin_fabsf(work->scl.y) + one) * BOX_HALF;
    z = (_builtin_fabsf(work->scl.z) + one) * BOX_HALF;
    njTranslateV(NULL, &work->pos);
    njTranslate(NULL, zero, y, zero);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, x * CUBE_FIT, y * CUBE_FIT, z * CUBE_FIT);
    draw_tree(marker_cube_object);
    njPopMatrix(1);
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "TYPE :");
        way = (int)_builtin_fabsf((Float)work->ang.x) % 2;
        text = way == CHANGER_OUT ? "OUT" : "IN";
        njPrintC(NJM_LOCATION(EDITOR_COLUMN_WAY, EDITOR_ROW_ANG_X), text);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "SCL X");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "SCL Y");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "SCL Z");
        return;
    }
    switch (work->mode) {
    case CHANGER_WAIT:
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        if (event_flags[FLAG_LIGHTS_WAY] == WAY(work)) {
            return;
        }
        if (!player_in_range(&work->pos, VARS(work)->half.y)) {
            return;
        }
        event_flags[FLAG_LIGHTS_WAY] = WAY(work);
        work->mode = CHANGER_BLEND;
        SHARE(work) = zero;
        break;
    case CHANGER_BLEND:
        share = SHARE(work);
        share += BLEND_SPEED;
        if (share > one) {
            share = one;
            work->mode = CHANGER_WAIT;
        } else {
            SHARE(work) = share;
        }
        if (WAY(work) == CHANGER_OUT) {
            palette_mix(palette_get(0, 0), palette_get(4, 0), palette_get(5, 0),
                        share);
            palette_mix(palette_get(0, 1), palette_get(4, 1), palette_get(5, 1),
                        share);
        } else {
            palette_mix(palette_get(0, 0), palette_get(5, 0), palette_get(4, 0),
                        share);
            palette_mix(palette_get(0, 1), palette_get(5, 1), palette_get(4, 1),
                        share);
        }
        break;
    }
}
