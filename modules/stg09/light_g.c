/* O LHTG light, which goes on and off for random times */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/helpers.h"
#include "stg09/light.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT light_g_object;
extern NJS_MATERIAL light_g_materials[];

/* destroyed beyond about 600 units of the camera and player 0 */
#define RANGE ((600.0F * 600.0F) + 10.0F)

#define SE_LIT 275
/* the radius of the sphere about a light that draw_in_view tests */
#define VIEW_RADIUS 105.0F

#define LIT(work) ((work)->mode) /* TRUE while the light is on */
/* the most steps a light stays on, and the most it stays off */
#define MOST_ON_STEPS(work) ((work)->scl.x)
#define MOST_OFF_STEPS(work) ((work)->scl.y)

/* the lamp's texture while it's on, then while it's off */
static struct texture_swap light_g_lamp[2] = {
    {&light_g_materials[4], 16, 0},
    {&light_g_materials[4], 15, 0},
};

static void light_g_destroy(struct task *task)
{
    texture_swap_release(light_g_lamp, 2);
}

static void light_g_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, VIEW_RADIUS)) {
        texture_swap_use(&light_g_lamp[LIT(work) ? 0 : 1], 1);
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&light_g_object);
        njPopMatrix(1);
        texture_swap_restore(light_g_lamp, 2);
    }
}

static void light_g_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    light_g_draw(task);
    /* @hack spelling: the test for 0, written <= 0 for the image's CMP/PL
     * (0x0c91e100). TASK_TIMER is a Uint16, so <= 0 holds only at 0. as == 0,
     * the test is TST, and 2 of light_g_run's 98 words differ */
    if (TASK_TIMER(work)-- <= 0) {
        if (LIT(work)) {
            TASK_TIMER(work) =
                _builtin_fabsf(rand() * MOST_OFF_STEPS(work) * RAND_SCALE);
            LIT(work) = FALSE;
        } else {
            TASK_TIMER(work) =
                _builtin_fabsf(rand() * MOST_ON_STEPS(work) * RAND_SCALE);
            LIT(work) = TRUE;
        }
    }
    if (LIT(work)) {
        sound_play_at_for(SE_LIT, (int)work, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 3);
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> LightON time max");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "-> LightOFF time max");
    }
}

void light_g_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        work->timer = 0;
        LIT(work) = TRUE;
        task->draw = light_g_draw;
        task->step = light_g_run;
        task->destructor = light_g_destroy;
        texture_swap_save(light_g_lamp, 2);
    }
}
