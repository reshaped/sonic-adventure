/* O HAGURUMA: a gear that turns about y, carrying player 0 round */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/gear.h"
#include "stg09/helpers.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT gear_object;

/* the task destroys itself beyond about 250 of the camera and player 0.
 * placement_try_leave_by takes the range squared */
#define RANGE ((250.0F * 250.0F) + 10.0F)
/* the radius casino_world_add and casino_world_test_near take for the gear's
 * copy in the collision world */
#define COPY_RADIUS 97.0F

#define SE_TURN 274

Angle casino_gear_turn = 27;
Float casino_gear_scale = 2.7070063F;

static void gear_init(struct task *task);
static void gear_destroy(struct task *task);
static void gear_run(struct task *task);
static void gear_draw(struct task *task);

void gear_step(struct task *task)
{
    /* @hack redundant: work, read and never used, which keeps task in R14
     * across the range check (MOV R4,R14 at 0x0c90bb68). without it, 12 of
     * 16 words differ */
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        gear_init(task);
    }
}

static void gear_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        casino_world_add(task, &gear_object, COPY_RADIUS, WORLD_MOVES);
    }
    task->draw = gear_draw;
    task->step = gear_run;
    task->destructor = gear_destroy;
    work->timer = 0;
    work->mode = 0;
}

static void gear_destroy(struct task *task)
{
    casino_world_remove(task);
}

/* the gear's y angle is its turn each step times the steps it has run. the
 * timer counts them, outside the set editor */
static void gear_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        gear_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X),
                     TURNS_RIGHT(work) ? "RotDir:Migi" : "RotDir:Hidari");
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "RotSpd:");
            njPrintD(EDITOR_NUMBER(EDITOR_ROW_SCL_Y),
                     (int)njRoundOff(work->scl.y), 4);
        } else {
            Float time = TASK_TIMER(work);
            Angle ang = casino_gear_turn;
            Float scale = casino_gear_scale;
            Float turns;
            Sint16 *low;
            Float scaled;

            if (TURNS_RIGHT(work)) {
                turns = scale * njRoundOff(work->scl.y) * time;
            } else {
                turns = -(scale * njRoundOff(work->scl.y) * time);
            }
            /* @hack type: ang's low halfword, the only half scaled, read and
             * written through a Sint16 pointer, for the image's MOV.W @R4,R3
             * at 0x0c90bcd0. as union angle_halves, the function runs 34
             * bytes long and 122 of 125 words differ */
            low = (Sint16 *)&ang;
            scaled = *low;
            scaled *= turns;
            *low = scaled;
            sound_play_for(SE_TURN, (int)work, 1, 0, 10);
            casino_turn_to(task, NULL, ang);
            casino_world_test_near(task, COPY_RADIUS);
            work->timer++;
        }
    }
}

static void gear_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &gear_object;
    Angle ang;

    if (!placement_try_leave_by(task, RANGE)) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        draw_tree(object);
        njPopMatrix(1);
    }
}
