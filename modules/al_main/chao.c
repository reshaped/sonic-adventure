/* a garden's Chao: its task, which steps it by its handling, and the
 * placing of a garden's Chao as the garden starts */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "al_main/behavior.h"
#include "al_main/chao.h"
#include "al_main/egg.h"
#include "al_main/entry.h"
#include "al_main/face.h"
#include "al_main/icon.h"
#include "al_main/motion.h"
#include "al_main/move.h"
#include "al_main/physics.h"
#include "al_main/shape.h"
#include "al_main/sparkle.h"
#include "al_main/status.h"
#include "chao/chao.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "player/player.h"
#include "player/target.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

#define GARDEN_SPOTS 16

/* where a garden's Chao start, before x and z move by up to 10, or by up
 * to 40 where y is 5 or below */
static NJS_POINT3 garden_spots[GARDEN_COUNT][GARDEN_SPOTS] = {
    {
        {-166.0F, -1.5F, -39.0F},
        {-133.0F, -6.0F, 48.0F},
        {-173.0F, 0.0F, 122.8F},
        {-31.8F, 0.0F, -109.8F},
        {-37.3F, 0.0F, 20.9F},
        {-33.0F, 0.0F, 88.7F},
        {40.5F, 0.0F, 90.0F},
        {26.5F, 0.0F, -1.7F},
        {13.1F, 0.0F, 84.6F},
        {98.9F, 2.9F, 12.0F},
        {92.0F, 3.0F, 57.6F},
        {149.4F, 9.0F, -99.6F},
        {136.9F, 1.0F, 84.7F},
        {-3.3F, 0.0F, -4.6F},
        {12.5F, 0.0F, 41.0F},
        {121.0F, 0.0F, -44.0F},
    },
    {
        {105.0F, 70.9F, -39.0F},
        {-61.0F, 70.9F, -68.7F},
        {75.1F, 2.95F, -244.3F},
        {17.3F, 2.0F, -247.2F},
        {-157.4F, 3.0F, -197.0F},
        {105.0F, 70.9F, -39.0F},
        {-61.0F, 70.9F, -68.7F},
        {75.1F, 2.95F, -244.3F},
        {17.3F, 2.0F, -247.2F},
        {-157.4F, 3.0F, -197.0F},
        {105.0F, 70.9F, -39.0F},
        {-61.0F, 70.9F, -68.7F},
        {75.1F, 2.95F, -244.3F},
        {17.3F, 2.0F, -247.2F},
        {-157.4F, 3.0F, -197.0F},
        {105.0F, 70.9F, -39.0F},
    },
    {
        {162.4F, 15.5F, -108.2F},
        {187.0F, 15.4F, -41.3F},
        {84.9F, 6.45F, -58.7F},
        {150.2F, 3.2F, 38.0F},
        {120.4F, 2.3F, 135.0F},
        {-140.0F, 46.0F, -72.1F},
        {-59.2F, 46.2F, 2.2F},
        {-10.0F, 1.7F, 64.5F},
        {30.4F, 28.4F, 131.5F},
        {-152.0F, 2.1F, 54.2F},
        {-210.3F, 10.7F, 1.6F},
        {-103.0F, 1.7F, 117.0F},
        {-45.9F, -8.0F, -35.6F},
        {-113.0F, 1.8F, 102.0F},
        {-19.8F, 1.7F, 63.9F},
        {75.0F, 1.7F, 87.0F},
    },
};

Uint8 dat_0ca84420[8];
Sint32 alife_paused;
/* 12 bytes after alife_paused, which no code reads */
static Uint8 dat_0ca8442c[12];

static void chao_init(struct task *task);
static void chao_start(struct task *task);
static void chao_destroy(struct task *task);
static void chao_step(struct task *task);

int alife_chao_alloc(struct task *task, struct chao *record)
{
    struct alife_chao *chao = heap_calloc(1, sizeof(*chao));
    if (chao == NULL) {
        task_destroy(task);
        return 0;
    }
    chao->flags |= CHAO_SHAPE_SHOWN;
    chao->record = record;
    chao->scale = 1.0F;
    task->motion = (struct task_motion *)chao;
    chao->task = task;
    return 1;
}

struct task *alife_chao_create(struct chao *record)
{
    struct task *task;
    /* @hack spelling: the type through a local, which reads it into R4 at
     * 0x0c900056 and widens it into R0 for the switch; read in place, 2 of
     * 51 words differ */
    Uint8 type = record->type;
    switch (type) {
    case CHAO_EGG:
        task = task_create(TASK_WORK, 3, alife_egg_start);
        break;
    case CHAO_CHILD:
    case CHAO_STANDARD:
    case CHAO_SWIM:
    case CHAO_FLY:
    case CHAO_RUN:
    case CHAO_POWER:
    case CHAO_CHAOS:
        task = task_create(TASK_WORK, 3, chao_start);
        break;
    default:
        return NULL;
    }
    record->unk_17a = 1;
    if (task && !alife_chao_alloc(task, record)) {
        /* @bug memory: writes the step of a task already freed, by the JSR
         * to task_kill at 0x0c9000c2. alife_chao_alloc destroys the task when
         * it fails, which puts it back on the free list */
        task_kill(task);
        return NULL;
    }
    return task;
}

static void chao_init(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;
    work->mode = 0;
    work->submode = 0;
    /* @hack redundant: the timer, cleared again below before anything
     * reads it (0x0c900122, 0x0c90016a); without this clear, 139 of 158
     * words differ */
    work->timer = 0;
    collision_init(task, chao_collisions, COLLISION_ENEMY, 1, 3);
    work->collision->flags &= ~COLLISION_TARGET;
    work->ang.x = 0;
    work->ang.z = 0;
    chao->spin = 0;
    chao->unk_080 = 0;
    chao->v_pos = chao->record->v_pos;
    chao->h_pos = chao->record->h_pos;
    chao->growth = chao->record->growth;
    work->tick = 0;
    work->timer = 0;
    alife_motion_init(work, chao);
    alife_terrain_init(work, chao);
    fun_0c90d634(work, chao);
    alife_icon_clear(work, chao);
    alife_face_init(work, chao);
    alife_shape_init(work, chao);
    alife_shape_blend(work, chao);
    alife_shape_set_parts(work, chao);
    alife_shape_set_wins(work, chao);
    alife_set_home(work, chao);
    carry_set_kind(work, task->motion, CARRY_KIND_SMALL);
    alife_entry_add(task, ALIFE_CHAO, 0, 3.8F);
    chao->flags &= ~(CHAO_SHOW_NAME | CHAO_IN_COCOON | CHAO_UNK_8);
    chao->record->flags &= ~CHAO_GROWTH_DUE;
    chao->name_alpha = 0;
    /* @hack type: name_frames stored as a Sint16, which stores R12's zero
     * at 0x0c9001f2; as a Uint16, SHC copies the zero to R3 first, and 30
     * of 158 words and 1 pool word differ */
    *(Sint16 *)&chao->name_frames = 0;
    chao->flags &= ~CHAO_NO_FAR_TREES;
    if (chao_get_carried_to() != -1 && chao->record == chao_get_carried()) {
        player_hand_task(0, task);
        chao_clear_carried();
    }
}

void fun_0c9002ac(struct task_work *work, struct alife_chao *chao)
{
    alife_shape_free(work, chao);
}

static void chao_start(struct task *task)
{
    stage_get_stage_act();
    task->destructor = chao_destroy;
    chao_init(task);
    task->step = chao_step;
    task->draw = alife_chao_draw;
}

static void chao_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;
    struct alife_entry *entry = task->extra;
    chao_list_remove(chao->record);

    /* @hack spelling: the comma, which keeps alife_shape_free a call, the JSR
     * at 0x0c9002f4, not a tail jump; as two statements, 7 of 23 words
     * differ */
    alife_entry_remove(entry), alife_shape_free(work, chao);
}

static void chao_free_step(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;
    /* @hack redundant: the else after the return, which loads alife_interrupt's
     * address into R1 at 0x0c900332; without it, 2 of 60 words differ */
    if (edit_mode != 0) {
        return;
    } else {
        /* @hack type: alife_interrupt called through a cast to its own
         * type, which loads the three calls' addresses into R1, R3 and R2
         * (0x0c900332 to 0x0c900342); called plainly, they take R3, R2 and
         * R3, and 6 of 60 words differ */
        ((Bool (*)(struct task_work *, struct alife_chao *))alife_interrupt)(
            work, chao);
        alife_behave(work, chao);
        alife_collide_terrain(work, chao);
        if (work->flags & CARRY_HELD) {
            chao->handling = HANDLING_HELD;
            work->submode = 0;
            work->timer = 0;
            status_raise_like(work, chao, 1);
            if (chao->motion.current != 1) {
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = 17;
            }
            /* @hack redundant: the return ahead of the else, which keeps the
             * store above out of the BRA's delay slot, a NOP at
             * 0x0c900382; without it, 16 of 60 words differ */
            return;
        } else {
            collision_register(work);
        }
    }
}

static void chao_held(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;
    struct alife_entry *entry = task->extra;
    Float magnitude;
    Sint8 garden;

    switch (work->submode) {
    case 0:
        if (entry->held) {
            entry->held->flags &= ~ENTRY_HELD;
            entry->held = NULL;
        }
        chao->flags |= CHAO_SHOW_NAME;
        chao->flags &= ~CHAO_SHAKEN;
        chao->limit = 0;
        chao->speed.x = 0.0F;
        chao->speed.y = 0.0F;
        chao->speed.z = 0.0F;
        work->submode++;
        /* fall through */
    case 1:
        if (TASK_TIMER(work) < 30) {
            work->ang.y =
                angle_approach(work->ang.y, -player_work[0]->ang.y, 0x800);
        } else {
            work->ang.y = -player_work[0]->ang.y;
        }
        if (!(player_work[0]->flags & PLAYER_HOLDING)) {
            magnitude = (chao->speed.x * chao->speed.x) +
                        (chao->speed.y * chao->speed.y) +
                        (chao->speed.z * chao->speed.z);
            chao->flags &= ~CHAO_SHOW_NAME;
            if (magnitude > 1.0F) {
                chao->handling = HANDLING_THROWN;
                work->mode = 0;
                work->submode = 0;
                work->pos = chao->last_pos;
                work->timer = 0;
                chao->speed.x *= 0.4F;
                chao->speed.y *= 1.1F;
                chao->speed.z *= 0.4F;
                if (njScalor(&chao->speed) > 2.0F) {
                    njUnitVector(&chao->speed);
                    chao->speed.x *= 2.0F;
                    chao->speed.y *= 2.0F;
                    chao->speed.z *= 2.0F;
                }
                status_hurt(work, chao, 5);
                if (chao->record->curio < 0) {
                    if (chao->record->agres > 0) {
                        chao->record->anger += 30;
                        if (chao->record->anger > 200) {
                            chao->record->anger = 200;
                        }
                    } else {
                        chao->record->fear += 30;
                        if (chao->record->fear > 200) {
                            chao->record->fear = 200;
                        }
                    }
                }
            } else if ((entry->flags & ENTRY_TOUCHING) &&
                       entry->hits[0]->kind == ALIFE_VMS) {
                chao->handling = HANDLING_VMS;
                work->submode = 0;
            } else {
                chao->handling = HANDLING_FREE;
                work->mode = 0;
                work->submode = 0;
                work->timer = 0;
            }
        } else {
            garden = chao_get_carried_to();
            if (garden != -1) {
                chao_set_carried(chao->record);
                chao_move(chao->record, garden);
            }
            if (!(chao->flags & CHAO_SHAKEN)) {
                magnitude = njScalor(&chao->accel);
                if (magnitude > 0.01F && TASK_TIMER(work)++ > 20) {
                    chao->limit++;
                    work->timer = 0;
                    if (chao->limit > 10) {
                        chao->flags |= CHAO_SHAKEN;
                        status_raise_like(work, chao, 2);
                        /* @bug arithmetic: the sum is stored to the byte
                         * before the clamp reads it (MOV.B at 0x0c9006dc,
                         * CMP/GT at 0x0c9006e6), so a sum past 255 wraps.
                         * a curio of 60 to 99 adds 60, which wraps a
                         * pleasure above 195, and a curio of 100 adds 70,
                         * which wraps one above 185 */
                        chao->record->pleasure +=
                            (((chao->record->curio + 100) / 40) + 2) * 10;
                        if (chao->record->pleasure > 200) {
                            chao->record->pleasure = 200;
                        }
                    }
                }
            }
        }
        break;
    }
}

static void chao_thrown(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;
    /* @hack spelling: the motion through control, taken at entry and kept
     * at R15 (0x0c900758) for case 3; read in place, 96 of 206 words and 12
     * pool words differ */
    struct chao_motion *control = &chao->motion;
    alife_apply_speed(work, chao);
    chao->flags |= 0x80;
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 18;
        chao->icon.kind = ICON_GLAD;
        chao->icon.symbol.state = 0;
        chao->icon.ball.state = 0;
        chao->icon.timer = 200;
        if (chao->record->agres > 0) {
            sound_play(0x391, 0, 0, 0);
        } else {
            sound_play(0x358, 0, 0, 0);
        }
        work->submode++;
        break;
    case 1:
        if (work->flags & CHAO_ON_GROUND) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 19;
            work->submode++;
        }
        break;
    case 2:
        if (TASK_TIMER(work) > 200) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 2;
            work->submode++;
        }
        break;
    case 3:
        if (control->flags & MOTION_ENDED) {
            chao->handling = HANDLING_FREE;
            work->mode = 1;
            work->submode = 0;
            work->timer = 0;
        }
        break;
    }
    if (fun_0c915b00(work, chao)) {
        chao->handling = HANDLING_FREE;
        work->mode = 1;
        work->submode = 0;
        work->timer = 0;
    }
    alife_collide_terrain(work, chao);
    if (work->flags & CHAO_ON_GROUND) {
        chao->accel.x = -(chao->speed.x * 0.3F);
        chao->accel.z = -(chao->speed.z * 0.3F);
        if (chao->speed.y < -0.1F) {
            chao->scale_speed = 0.16F;
        }
    }
    if (work->flags & CARRY_HELD) {
        chao->handling = HANDLING_HELD;
        work->submode = 0;
        work->timer = 0;
        status_raise_like(work, chao, 1);
        if (chao->motion.current != 1) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 17;
        }
        /* @hack redundant: the return ahead of the else, which keeps the store
         * above out of the BRA's delay slot, a NOP at 0x0c900902; without
         * it, 13 of 206 words differ */
        return;
    } else {
        collision_register(work);
    }
}

static void chao_on_vms(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;
    struct alife_entry *entry = task->extra;
    switch (work->submode) {
    case 0:
        sound_play(0x34d, 0, 0, 0);
        work->submode++;
        work->timer = 0;
        /* fall through */
    case 1:
        if (entry->flags & ENTRY_LET_GO) {
            work->submode++;
        }
        if (TASK_TIMER(work) > 150) {
            chao->handling = HANDLING_FREE;
            work->submode = 0;
        }
        break;
    case 2:
        work->pos.y -= 0.16F;
        work->ang.y += 0x1200;
        if (work->pos.y < -100.0F) {
            work->pos.y = -100.0F;
        }
        break;
    }
}

static void chao_step(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;
    struct alife_entry *entry = task->extra;
    NJS_POINT3 position;
    if (alife_paused == 0) {
        switch (chao->handling) {
        case HANDLING_FREE:
            chao_free_step(task);
            break;
        case HANDLING_HELD:
            chao_held(task);
            break;
        case HANDLING_THROWN:
            chao_thrown(task);
            break;
        case HANDLING_VMS:
            chao_on_vms(task);
            break;
        }
        alife_motion_step(work, chao);
        alife_face_step(work, chao);
        alife_grow_step(work, chao);
        alife_chao_show(work, chao);
        if (chao->record->type == CHAO_CHAOS) {
            if (rand() * RAND_SCALE < 0.05F) {
                position.x =
                    work->pos.x + (((rand() * RAND_SCALE) - 0.5F) * 2.0F);
                position.y = work->pos.y + (rand() * RAND_SCALE) + 0.8F;
                position.z =
                    work->pos.z + (((rand() * RAND_SCALE) - 0.5F) * 2.0F);
                if (rand() * RAND_SCALE < 0.8F) {
                    alife_glitter_create_slow(&position);
                } else {
                    alife_glitter_create(&position);
                }
            }
        }
        alife_save_pos(work, chao);
        work->tick++;
        work->timer++;
    } else {
        if (chao->handling == HANDLING_VMS) {
            chao_on_vms(task);
        }
        alife_chao_show(work, chao);
    }
    task_step_children(task);
    entry->flags &= ~ENTRY_LET_GO;
}

void alife_chao_remove(struct task *task)
{
    struct alife_chao *chao = (struct alife_chao *)task->motion;
    struct chao *record = chao->record;
    record->type = CHAO_NOTHING;
    task_kill(task);
}

void alife_chao_create_garden(int garden)
{
    struct chao_node *node = chao_get_list(garden);
    struct task *task;
    int spot;

    /* @hack redundant: date and seed in a bare block, so that date and the
     * loop's pos share a slot: 16 bytes of locals, ADD #-16,R15 at
     * 0x0c900c78; declared at the top, 148 of 167 words differ */
    {
        SYS_RTC_DATE date;
        /* @hack spelling: the seed through a local, which puts 2 *
         * RAND_SCALE and 1.0 in FR15 and FR14, and 10 and 40 in FR13 and
         * FR12; passed in place, each pair trades, and 14 of 167 words and 2
         * pool words differ */
        unsigned int seed;
        syRtcGetDate(&date);
        /* @bug arithmetic: the count, second + 16, passes 31 from second
         * 16 on. SHLD at 0x0c900c98 shifts by the count's low five bits,
         * and drops the bits shifted past 31 */
        seed = date.minute << (date.second + 16);
        srand(seed);
    }
    while (node) {
        NJS_POINT3 pos;
        spot = (int)(rand() * (GARDEN_SPOTS * RAND_SCALE));
        task = alife_chao_create(node->chao);
        pos = garden_spots[garden][spot];
        if (pos.y > 5.0F) {
            pos.x += (1.0F - (rand() * (2.0F * RAND_SCALE))) * 10.0F;
            pos.z += (1.0F - (rand() * (2.0F * RAND_SCALE))) * 10.0F;
        } else {
            pos.x += (1.0F - (rand() * (2.0F * RAND_SCALE))) * 40.0F;
            pos.z += (1.0F - (rand() * (2.0F * RAND_SCALE))) * 40.0F;
        }
        /* @bug memory: alife_chao_create's null, for a record of another
         * type or with the task pool or the heap spent, goes unchecked. the
         * loop reads a work pointer from address 32 (0x0c900d6a) and stores
         * the place and the angle through it */
        task->work->pos = pos;
        task->work->ang.y = (int)(rand() * 1.9999999F);
        node = node->next;
    }
}
