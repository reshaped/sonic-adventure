/* O HOSIN, an unseen launcher that flings player 0 out along its aim */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg09/helpers.h"
#include "stg09/launcher.h"
#include "system/task.h"
#include "system/util.h"

/* destroyed beyond about 250 units of the camera and player 0 */
#define RANGE ((250.0F * 250.0F) + 10.0F)

#define RADIUS(work) ((work)->scl.x + 5.0F) /* the shape's radius */
/* added to the aim's x before the aim becomes a unit vector, which leans
 * the aim */
#define LEAN(work) ((work)->scl.y)
#define SPEED(work) ((work)->scl.z) /* added to launcher_speed */

/* the steps a fling leaves player 0 beyond its control */
#define LAUNCH_LOCK 60

/* the aim, before LEAN and the launcher's turn */
static NJS_VECTOR launcher_aim = {0.0F, 10.0F, 0.0F};
static Float launcher_speed = 1.5F;
/* the dust a launch raises: how fast it rises, and its scale */
static Float launcher_dust_rise = 2.5F;
static Float launcher_dust_scale = 5.0F;
/* @hack type: an array of one, not the single shape it is. SHC keeps its
 * address in R12 through launcher_run (MOV.L @(disp,PC),R12 at
 * 0x0c9243d6). as a single shape, the object builds 32 bytes shorter, and 291
 * of launcher_run's 299 words differ */
static struct collision_shape launcher_shape[1] = {
    {
        SHAPE_CYLINDER2 | SHAPE_NO_PUSH,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 0.0F, 0.0F},
        1.0F,
        10.0F,
        0.0F,
        {0},
        0,
        NULL,
    },
};

static void launcher_run(struct task *task);

/* draw shape at pos as marker_cylinder_object, turned as the collision checks
 * turn a SHAPE_CYLINDER2 */
static void launcher_draw_shape(struct task_work *work,
                                struct collision_shape *shape, NJS_POINT3 *pos)
{
    Angle ang;
    Float radius = shape->radius * MARKER_FIT;
    Float height = shape->size_y * MARKER_FIT;

    njPushMatrix(NULL);
    njTranslate(NULL, pos->x, pos->y, pos->z);
    if (shape->flags & SHAPE_TURN_YZX) {
        if (!(shape->flags & SHAPE_IN_WORLD) &&
            !(shape->flags & SHAPE_UNTURNED)) {
            TURN_VIA(njRotateY, work->ang.y, ang);
            TURN_VIA(njRotateZ, work->ang.z, ang);
            TURN_VIA(njRotateX, work->ang.x, ang);
        }
        TURN_VIA(njRotateY, shape->ang.y, ang);
        TURN_VIA(njRotateZ, shape->ang.z, ang);
        TURN_VIA(njRotateX, shape->ang.x, ang);
    } else {
        if (!(shape->flags & SHAPE_IN_WORLD) &&
            !(shape->flags & SHAPE_UNTURNED)) {
            TURN_VIA(njRotateZ, work->ang.z, ang);
            TURN_VIA(njRotateX, work->ang.x, ang);
            TURN_VIA(njRotateY, work->ang.y, ang);
        }
        TURN_VIA(njRotateZ, shape->ang.z, ang);
        TURN_VIA(njRotateX, shape->ang.x, ang);
        TURN_VIA(njRotateY, shape->ang.y, ang);
    }
    njScale(NULL, radius, height, radius);
    draw_tree(&marker_cylinder_object);
    njPopMatrix(1);
}

void launcher_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        task->step = launcher_run;
        if (!placement_editing(task)) {
            collision_init(task, launcher_shape, COLLISION_OBJECT, 1, 1);
            work->collision->shapes->radius = RADIUS(work);
            collision_set_radius(work->collision);
        }
        work->timer = 0;
    }
}

static void launcher_run(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (placement_editing(task)) {
        NJS_POINT3 tip;
        Float *at;
        struct collision_shape *shape;

        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_X), "-> angX ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_Y), "-> angY ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_Z), "-> angZ ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> rad");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "-> utiage angle");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "-> speed");
        launcher_shape->radius = RADIUS(work);
        launcher_draw_shape(work, launcher_shape, &work->pos);
        /* mark the aim with a ball, size_y + 4 along it from the launcher */
        tip = launcher_aim;
        tip.x += LEAN(work);
        njUnitVector(&tip);
        /* @hack spelling: the shape through a local, which SHC copies to R5
         * for the three reads of size_y (MOV R12,R5 at 0x0c924458). through
         * launcher_shape, 71 of launcher_run's 299 words differ */
        shape = launcher_shape;
        /* @hack spelling: tip scaled through a cursor over its floats, which
         * SHC steps with ADD #4,R4 (0x0c92446e). through tip's fields, 69 of
         * launcher_run's 299 words differ */
        at = &tip.x;
        *at *= shape->size_y + 4.0F;
        at++;
        *at *= shape->size_y + 4.0F;
        at++;
        *at *= shape->size_y + 4.0F;
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njCalcPoint(NULL, &tip, &tip);
        njPopMatrix(1);
        casino_draw_sphere(&tip, 3.0F);
    } else {
        if (work->collision->flags & COLLISION_HIT) {
            struct player *player = player_get(0);
            NJS_POINT3 pos;
            NJS_VECTOR speed;
            NJS_VECTOR line;
            Float *at;

            /* @hack spelling: speed and pos set through a cursor over their
             * floats, which SHC steps with ADD #4,R4 (0x0c9245c2 to
             * 0x0c9245de). through the fields, the object builds 32 bytes
             * shorter, and 142 of launcher_run's 299 words differ */
            at = &speed.x;
            *at++ = 0.0F;
            *at++ = launcher_dust_rise;
            *at = 0.0F;
            at = &pos.x;
            *at++ = 0.0F;
            *at++ = launcher_shape->size_y + 5.0F;
            *at = 0.0F;
            line = launcher_aim;
            line.x += LEAN(work);
            njUnitVector(&line);
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);
            TURN_VIA(njRotateZ, work->ang.z, ang);
            TURN_VIA(njRotateX, work->ang.x, ang);
            TURN_VIA(njRotateY, work->ang.y, ang);
            njCalcVector(NULL, &line, &line);
            njCalcVector(NULL, &speed, &speed);
            njCalcPoint(NULL, &pos, &pos);
            njPopMatrix(1);
            player_fling(0, launcher_speed + SPEED(work), &line);
            dust_create(&pos, &speed, launcher_dust_scale);
            player->launch_steps = LAUNCH_LOCK;
        } else {
            work->timer = 0;
        }
        collision_register(work);
    }
}
