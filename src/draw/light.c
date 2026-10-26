/* the stage's directional light, and the lights tasks bring */
#include <ninja.h>

#include "draw/light.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* the modes of light_stage_step's task and of light_create's. LIGHT_END
 * kills the task, and light_create's reaches it once the stage or act
 * changes */
enum light_mode { LIGHT_INIT, LIGHT_KEEP, LIGHT_END };

/* what light_create's task keeps in work->vars */
struct light_vars {
    NJS_LIGHT *light;
    struct light_spec *spec;
};

#define VARS(work) ((struct light_vars *)&(work)->vars)

static void light_stage_destroy(struct task *task);
static void light_destroy(struct task *task);
static void light_step(struct task *task);

NJS_LIGHT dir_light;

/* make dir_light in the first step. each step after copies it to ninja's
 * easy light, with its direction turned by the current matrix and its
 * ambient doubled */
void light_stage_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR direction;

    switch (work->mode) {
    case LIGHT_INIT:
        njCreateLight(&dir_light, NJD_PHONG_DIR);
        njSetLightIntensity(&dir_light, 0.8F, 1.0F, 0.25F);
        njSetLightColor(&dir_light, 1.0F, 1.0F, 1.0F);
        njSetLightAngle(&dir_light, NJM_DEG_ANG(15), NJM_DEG_ANG(70));
        njSetLightDirection(&dir_light, 0.866025F, -0.6F, 0.0F);
        task->destructor = light_stage_destroy;
        work->mode++;
        break;
    case LIGHT_KEEP:
        njPushMatrix(NULL);
        njCalcVector(NULL, &dir_light.vctr, &direction);
        njPopMatrix(1);
        njSetEasyLight(direction.x, direction.y, direction.z);
        njSetEasyLightIntensity(dir_light.attr.idif,
                                dir_light.attr.iamb * 2.0F);
        njSetEasyLightColor(dir_light.attr.argb.r, dir_light.attr.argb.g,
                            dir_light.attr.argb.b);
        break;
    case LIGHT_END:
        task_kill(task);
        break;
    }
}

static void light_stage_destroy(struct task *task)
{
    njDeleteLight(&dir_light);
}

static void light_destroy(struct task *task)
{
    struct task_work *work = task->work;
    NJS_LIGHT *light = VARS(work)->light;

    njDeleteLight(light);
}

/* make the light from its spec in the first step, and end the task once
 * the stage or act differs from the first step's. the work's timer holds
 * that stage and act */
static void light_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_LIGHT *light;
    struct light_spec *spec;

    switch (work->mode) {
    case LIGHT_INIT:
        light = VARS(work)->light;
        spec = VARS(work)->spec;
        TASK_TIMER(work) = stage_get_stage_act();
        njCreateLight(light, spec->type);
        njSetLightIntensity(light, spec->specular, spec->diffuse,
                            spec->ambient);
        njSetLightColor(light, spec->color.r, spec->color.g, spec->color.b);
        if (spec->type & (NJD_DIR_LIGHT | NJD_SPOT_LIGHT)) {
            njSetLightDirection(light, spec->direction.x, spec->direction.y,
                                spec->direction.z);
        }
        if (spec->type & (NJD_POINT_LIGHT | NJD_SPOT_LIGHT)) {
            njSetLightRange(light, spec->near, spec->far);
        }
        if (spec->type & NJD_SPOT_LIGHT) {
            njSetLightAngle(light, spec->inner, spec->outer);
        }
        task->destructor = light_destroy;
        work->mode++;
        break;
    case LIGHT_KEEP:
        if (TASK_TIMER(work) != stage_get_stage_act()) {
            work->mode = LIGHT_END;
        }
        break;
    case LIGHT_END:
        task_kill(task);
        break;
    }
}

struct task *light_create(struct task *parent, NJS_LIGHT *light,
                          struct light_spec *spec)
{
    struct task_work *from = parent->work;
    struct task *task;
    struct task_work *work;
    NJS_POINT3 offset;
    NJS_VECTOR turned;

    /* @hack spelling: task assigned in the test, which tests
     * task_create_child's R0 with the copy to R12 in the BT/S's slot
     * (0x8c05659e, 0x8c0565a2); assigned apart, SHC copies it first and
     * tests R12, and 3 of 106 words differ */
    if ((task = task_create_child(TASK_WORK, light_step, parent)) != NULL) {
        work = task->work;
        if (work != NULL) {
            if (from != NULL) {
                offset = spec->pos;
                njPushMatrix(&_unit_matrix_);
                njTranslateV(NULL, &from->pos);
                TURN(njRotateZ, from->ang.z);
                TURN(njRotateX, from->ang.x);
                TURN(njRotateY, from->ang.y);
                /* @bug logic: the task stands at the parent's position plus
                 * offset unturned, and the turned offset goes unread.
                 * njCalcVector's JSR at 0x8c05660a writes turned at R15+12,
                 * and the sums read offset from R15+24 (0x8c056614 to
                 * 0x8c056636) */
                njCalcVector(NULL, &offset, &turned);
                njPopMatrix(1);
                work->pos.x = from->pos.x + offset.x;
                work->pos.y = from->pos.y + offset.y;
                work->pos.z = from->pos.z + offset.z;
            }
            VARS(work)->light = light;
            VARS(work)->spec = spec;
        }
    } else {
        /* @hack redundant: a return of NULL where return task returns the same,
         * which gives the null path its own MOV #0,R0 (0x8c056642,
         * 0x8c056644); without it, 13 of 106 words differ */
        return NULL;
    }
    return task;
}
