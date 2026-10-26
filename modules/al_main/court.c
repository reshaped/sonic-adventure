/* courtship: the flowers a courting Chao scatters, the call that brings a
 * partner to it, and the egg the two make */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/alife.h"
#include "al_main/behavior.h"
#include "al_main/chao.h"
#include "al_main/court.h"
#include "al_main/entry.h"
#include "al_main/face.h"
#include "al_main/models.h"
#include "chao/chao.h"
#include "collision/ground.h"
#include "draw/tree.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the steps a flower stays, the call lasts and a courting Chao waits for a
 * partner. each ends once the timer of its own task's work passes this */
#define COURT_STEPS 3600

/* a flower's work->tick: the model it draws */
enum flower_model { FLOWER_PLAIN, FLOWER_WHITE };

struct court_vars {
    Uint8 unk_00[4];
    Sint32 flowers; /* the flowers scattered so far */
};

#define VARS(work) ((struct court_vars *)&(work)->vars)

/* a flower grows to full size, stays while its Chao courts or breeds, for at
 * most COURT_STEPS, and then shrinks away. it draws on a frame's first step
 * only */
static void flower_step(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack type: the parent's mode read as a Uint16, which SHC widens by
     * EXTU.W before each test (0x0c915470, 0x0c915476). MODE_COURT and
     * MODE_BREED are below 128, so the tests hold as for the field's own Sint8;
     * as a Sint8, 2 of 126 words differ */
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    Uint16 mode = task->parent->work->mode;
    Float scale;
    Angle ang;

    switch (work->submode) {
    case 0:
        work->scl.x = 0.0F;
        work->submode++;
        break;
    case 1:
        work->scl.x += 0.05F;
        if (work->scl.x > 1.0F) {
            work->scl.x = 1.0F;
            work->submode++;
        }
        break;
    case 2:
        if (TASK_TIMER(work)++ > COURT_STEPS ||
            (mode != MODE_COURT && mode != MODE_BREED)) {
            work->submode++;
        }
        break;
    case 3:
        work->scl.x -= 0.02F;
        if (work->scl.x < 0.0F) {
            task_kill(task);
        }
        break;
    }
    if (frame_step != 0) {
        return;
    }
    scale = work->scl.x;
    njSetTexture(&alife_chao_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, scale, scale, scale);
    if (work->tick == FLOWER_PLAIN) {
        draw_tree(&alife_flower_object);
    } else {
        draw_tree(&alife_white_flower_object);
    }
    njPopMatrix(1);
}

/* a flower on the ground near the Chao, up to 10 off on x and on z. it is
 * white at even odds */
static void flower_create(struct task_work *work, struct alife_chao *chao)
{
    struct task *task;
    struct task_work *flower;
    struct angles slope;
    NJS_POINT3 pos;

    task = task_create_child(TASK_WORK, flower_step, chao->task);
    if (task != NULL) {
        flower = task->work;
        if (rand() * RAND_SCALE > 0.5F) {
            flower->tick = FLOWER_PLAIN;
        } else {
            flower->tick = FLOWER_WHITE;
        }
        flower->pos.x = work->pos.x + (((rand() * RAND_SCALE) - 0.5F) * 20.0F);
        flower->pos.z = work->pos.z + (((rand() * RAND_SCALE) - 0.5F) * 20.0F);
        flower->pos.y = work->pos.y + 10.0F;
        /* @hack spelling: the point copied field by field into pos, which
         * SHC builds at sp+4 and reads back for the call (0x0c91562a to
         * 0x0c91566c); without it, 105 of 111 words differ */
        pos.x = flower->pos.x;
        pos.y = flower->pos.y;
        pos.z = flower->pos.z;
        flower->pos.y = ground_below(pos.x, pos.y, pos.z, &slope);
        flower->mode = work->mode;
        /* @bug logic: the yaw takes a random 0 to 360 degrees and
         * multiplies it by 360 / 65536, which turns an Angle into degrees,
         * not degrees into an Angle (FMUL at 0x0c915680). the yaw is then 0
         * or 1, so every flower faces one way */
        flower->ang.y =
            (Angle)(rand() * ((360.0F * RAND_SCALE) * (360.0F / 65536.0F)));
    }
}

static void calling_step(struct task *task)
{
    struct task_work *work = task->work;

    if (task->parent->work->mode != MODE_COURT) {
        task_kill(task);
    }
    if (TASK_TIMER(work)++ > COURT_STEPS) {
        task_kill(task);
    }
}

static void calling_destroy(struct task *task)
{
    alife_entry_remove(task->extra);
    /* @hack redundant: a return where control returns anyway, which has SHC
     * make the frame by ADD #-4,R15 (0x0c915700); without it, SHC pushes task
     * instead, and 6 of 6 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

static void calling_init(struct task *task)
{
    alife_entry_add(task, ALIFE_CALLS, ALIFE_COURTING, 30.0F);
    task->step = calling_step;
    task->destructor = calling_destroy;
}

/* the call, an entry where the Chao is, which lasts until the Chao stops
 * courting or COURT_STEPS pass */
static void calling_create(struct task_work *work, struct alife_chao *chao)
{
    struct task *task;
    struct task_work *calling;

    task = task_create_child(TASK_WORK, calling_init, chao->task);
    if (task != NULL) {
        calling = task->work;
        calling->pos = work->pos;
    }
}

void alife_court(struct task_work *work, struct alife_chao *chao)
{
    struct alife_entry *entry = chao->task->extra;
    /* @hack spelling: the motion read through a pointer of its own, which SHC
     * keeps in the frame at sp (0x0c9157a0); without it, 131 of 142 words
     * differ */
    struct chao_motion *motion = &chao->motion;

    alife_face_set_eyes(chao, 2, 1);
    alife_face_set_mouth(chao, 1, 1);
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 5;
        work->submode++;
        VARS(work)->flowers = 0;
        break;
    case 1:
        if (TASK_TIMER(work) > 25) {
            flower_create(work, chao);
            sound_play_at(SE_CHAO_SQUEAK, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
            VARS(work)->flowers++;
            work->timer = 0;
        }
        if (VARS(work)->flowers > 8) {
            calling_create(work, chao);
            work->submode++;
            entry->held = NULL;
            /* @hack spelling: a return where control returns anyway, which
             * branches to the end with the store of entry->held in its slot
             * (BRA at 0x0c915868); as a break, 60 of 142 words differ */
            return;
        }
        break;
    case 2:
        if (TASK_TIMER(work) > COURT_STEPS) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 6;
            chao->record->breed = 0;
            work->submode++;
        }
        if (alife_entry_touching(work, chao, ALIFE_HATCHED) != NULL) {
            work->mode = MODE_BREED;
            work->submode = 0;
            work->timer = 0;
        }
        break;
    case 3:
        if (motion->flags & MOTION_ENDED) {
            chao->record->breed = 0;
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
        }
        break;
    }
}

void alife_partner(struct task_work *work, struct alife_chao *chao)
{
    work->mode = MODE_IDLE;
    work->submode = 0;
    work->timer = 0;
}

void alife_breed(struct task_work *work, struct alife_chao *chao)
{
    struct alife_entry *entry = chao->task->extra;
    struct alife_chao *partner = chao;
    struct chao *other;
    struct chao *child;
    struct task *egg;
    struct task_work *egg_work;

    if (entry->held != NULL) {
        partner = (struct alife_chao *)entry->held->task->motion;
    }
    other = partner->record;
    if (other->type == CHAO_CHILD || other->type == CHAO_CHAOS) {
        entry->held = NULL;
        work->mode = MODE_IDLE;
        work->submode = 0;
        work->timer = 0;
    }
    if (alife_get_chao_count() < ALIFE_CHAO_FULL) {
        if (other->charm * 0.005F > rand() * RAND_SCALE) {
            if (((other->skills.swim + other->skills.fly + other->skills.run +
                  other->skills.power) *
                 0.000025F) +
                    0.5F >
                rand() * RAND_SCALE) {
                chao->record->breed = 0;
                if (chao->record->life < 50) {
                    chao->record->life = 0;
                } else {
                    chao->record->life -= 50;
                }
                child = chao_find_empty();
                if (child != NULL) {
                    chao_breed(chao->record, other, child);
                    child->type = CHAO_EGG;
                    egg = alife_chao_create(child);
                    egg_work = egg->work;
                    egg_work->pos.x = work->pos.x + (njCos(work->ang.y) * 3.0F);
                    egg_work->pos.y = work->pos.y;
                    egg_work->pos.z = work->pos.z - (njSin(work->ang.y) * 3.0F);
                    child->pos = work->pos;
                    chao_set_id(child, 0);
                    chao_list_add(alife_garden, child);
                    child->garden = alife_garden;
                }
            }
        }
    }
    work->mode = MODE_IDLE;
    work->submode = 0;
    work->timer = 0;
}
