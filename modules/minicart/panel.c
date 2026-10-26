/* ARROW, Twinkle Park's O PANEL: a sign whose glow fades in and out */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "minicart/panel.h"
#include "minicart/shareobj_textures.h"
#include "object/placement.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT twinkle_panel_object;

/* the panel's range, squared, as placement_try_leave_by takes it */
#define PANEL_RANGE (3010.0F * 3010.0F)

/* a 256th of a turn: the glow's phase advances by it each step */
#define PANEL_PULSE 0x100
/* the glow, the phase's sine, never falls below PANEL_DIMMEST */
#define PANEL_DIMMEST (-0.8F)

/* what the panel keeps in its work's vars: the tree it draws, and the
 * glow's phase */
struct panel_vars {
    NJS_OBJECT *object;
    Uint8 unk_04[4];
    Angle phase;
};

#define VARS(work) ((struct panel_vars *)&(work)->vars)
/* the glow, from PANEL_DIMMEST to 1, in scl.x. the draw sets it as the
 * constant material's alpha */
#define PANEL_GLOW(work) ((work)->scl.x)

enum panel_mode { PANEL_INIT, PANEL_GLOWING };

static void twinkle_panel_destroy(struct task *task)
{
    task_kill(task);
}

static void twinkle_panel_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&obj_shareobj_texlist);
        draw_set_material(PANEL_GLOW(work), 1.0F, 1.0F, 1.0F);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(VARS(work)->object);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
}

void twinkle_panel_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (placement_try_leave_by(task, PANEL_RANGE)) {
        return;
    }
    if (stage_get_stage_act() == STAGE_ACT(STAGE_TWINKLE, 0) ||
        IN_STAGE(STAGE_CART)) {
        object = &twinkle_panel_object;
        /* the set editor shows it unlit */
        if (edit_mode != 0) {
            VARS(work)->object = object;
            PANEL_GLOW(work) = 0.0F;
            twinkle_panel_draw(task);
        } else {
            switch (work->mode) {
            case PANEL_INIT:
                work->mode = PANEL_GLOWING;
                VARS(work)->object = object;
                task->destructor = twinkle_panel_destroy;
                task->draw = twinkle_panel_draw;
                break;
            case PANEL_GLOWING:
                VARS(work)->phase += PANEL_PULSE;
                PANEL_GLOW(work) = njSin(VARS(work)->phase);
                if (PANEL_GLOW(work) < PANEL_DIMMEST) {
                    PANEL_GLOW(work) = PANEL_DIMMEST;
                }
                twinkle_panel_draw(task);
                break;
            }
        }
    }
}
