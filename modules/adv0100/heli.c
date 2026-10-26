/* O HELI helicopter, which kills its placement on its first step and so is
 * never drawn */
#include <ninja.h>

#include "adv0100/heli.h"
#include "draw/model.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* its main rotor is its child, and the tail rotor that child's sibling */
extern NJS_OBJECT heli_object;

void egg_carrier_heli_step(struct task *task)
{
    placement_kill(task);
}

void egg_carrier_heli_draw(struct task_work *work, struct task_motion *motion)
{
    NJS_OBJECT *body = &heli_object;
    NJS_OBJECT *rotor;
    Angle ang;

    njPushMatrix(NULL);
    texture_set_stage();
    njTranslateV(NULL, &work->pos);
    njRotateZXY(NULL, work->ang.x, work->ang.y, work->ang.z);
    draw_model(body->model);
    rotor = body->child;
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)rotor->pos);
    TURN_VIA(njRotateY, motion->aim.x, ang);
    draw_model(rotor->model);
    njPopMatrix(1);
    rotor = rotor->sibling;
    njTranslateV(NULL, (NJS_VECTOR *)rotor->pos);
    TURN_VIA(njRotateY, motion->aim.z, ang);
    draw_model(rotor->model);
    njPopMatrix(1);
}
