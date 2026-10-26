/* Emergency: an invisible launcher on the tables. its sphere sends player 0
 * flying once hit for long enough */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "player/player.h"
#include "stg09/emergency.h"
#include "system/task.h"
#include "system/util.h"

struct emergency_vars {
    Float touched; /* the steps in a row the sphere has been hit */
};

#define VARS(work) ((struct emergency_vars *)&(work)->vars)

/* the length and color of the line the set editor draws from the sphere */
#define EMERGENCY_LINE 50.0F
#define EMERGENCY_LINE_COLOR 0xffff8000

static struct collision_shape emergency_shape = {
    SHAPE_SPHERE,
    ATTACK(3),
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    0.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static void emergency_destroy(struct task *task)
{
}

static void emergency_run(struct task *task)
{
    struct task_work *work = task->work;

    if (work->collision->flags & COLLISION_HIT) {
        if (VARS(work)->touched >= work->scl.z) {
            NJS_VECTOR speed;
            Angle ang[3];

            speed.x = work->scl.y;
            speed.y = 0.0F;
            speed.z = 0.0F;
            ang[0] = 0;
            ang[1] = work->ang.y;
            /* @hack spelling: the last store and the call as one expression,
             * under which ang[2] and the call's zero arguments share R3
             * (0x0c90a5d6 to 0x0c90a5f0). as two statements, 22 of 148 words
             * differ */
            /* @hack type: the angles passed as (const Angle *)&ang, under
             * which SHC stores them through R15 (MOV.L R3,@(24,R15) at
             * 0x0c90a5ea). as ang, it stores them through their address in
             * R13, and 42 of 148 words differ */
            ang[2] = 0, player_send_flying(0, &speed, (const Angle *)&ang, 0);
            VARS(work)->touched = 0.0F;
        } else {
            VARS(work)->touched += 1.0F;
        }
    } else {
        VARS(work)->touched = 0.0F;
    }
    collision_register(work);
    object_clear_touch(work, NULL);
    if (placement_editing(task)) {
        NJS_POINT3 points[2];
        NJS_COLOR color;
        NJS_POINT3COL line;
        Float scale;
        Angle ang;

        njPrintC(NJM_LOCATION(20, EDITOR_ROW_SCL_X), "<- Size");
        njPrintC(NJM_LOCATION(20, EDITOR_ROW_SCL_Y), "<- Power");
        njPrintC(NJM_LOCATION(20, EDITOR_ROW_SCL_Z), "<- Int");
        njPushMatrix(NULL);
        scale = work->scl.x * 0.1F;
        points[0].x = points[0].y = points[0].z = 0.0F;
        points[1].x = points[1].y = 0.0F;
        points[1].z = EMERGENCY_LINE;
        color.color = EMERGENCY_LINE_COLOR;
        line.p = points;
        line.col = &color;
        line.tex = NULL;
        line.num = 2;
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScale(NULL, scale, scale, scale);
        njSimpleDrawObject(&marker_object);
        njDrawLine3D(&line, 1, 0);
        njPopMatrix(1);
    }
}

void emergency_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &emergency_shape, COLLISION_OBJECT, 1, 1);
    work->collision->shapes->radius = work->scl.x;
    task->step = emergency_run;
    task->destructor = emergency_destroy;
}
