/* a garden's Chao egg. left alone, it hatches. player 0 can hold, shake and
 * throw it. the VMS, the gardens' memory card station, can take it in. a
 * hatch leaves the shell's top and bottom, which fall and fade */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/alife.h"
#include "al_main/behavior.h"
#include "al_main/chao.h"
#include "al_main/egg.h"
#include "al_main/entry.h"
#include "al_main/models.h"
#include "al_main/physics.h"
#include "al_main/shape.h"
#include "al_main/status.h"
#include "chao/chao.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/common.h"
#include "player/player.h"
#include "player/target.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* a shell piece's work->tick: the kind of egg it is from, which picks its
 * model */
enum shell_kind { SHELL_PLAIN, SHELL_GOLD, SHELL_PAL, SHELL_BEGG };

/* the shells' tops and bottoms, by enum shell_kind */
static NJS_OBJECT *shell_tops[] = {
    &alife_egg_top_object,
    &alife_gold_egg_top_object,
    &alife_pal_egg_top_object,
    &alife_begg_egg_top_object,
};
static NJS_OBJECT *shell_bottoms[] = {
    &alife_egg_bottom_object,
    &alife_gold_egg_bottom_object,
    &alife_pal_egg_bottom_object,
    &alife_begg_egg_bottom_object,
};

struct egg_vars {
    Uint8 shakes; /* the times player 0 has shaken it */
    Uint8 unk_01[3];
    /* counted down as the egg waits, from a random start of 14400 to 21599.
     * below 100, the egg stirs */
    Sint32 wait;
};

#define VARS(work) ((struct egg_vars *)&(work)->vars)

/* the egg's bits in its work's flags */
#define EGG_SHAKEN 0x2000 /* shaken enough: a waiting egg stirs at once */
#define EGG_THROWN 0x4000 /* a waiting egg hatches once it lands */
#define EGG_HATCHED 0x8000 /* egg_show draws the plain shell's bottom */

/* a free egg's work->mode */
enum egg_mode {
    EGG_WAIT,
    EGG_STIR,
    EGG_HOP_1,
    EGG_HOP_2,
    EGG_HOP_3,
    EGG_HATCH,
    EGG_LAND,
    EGG_CRACK,
    EGG_GONE
};

#define SE_HOP 0x258

/* the top of a hatched egg's shell. it flies up, then falls and bounces on
 * the floor as it fades out */
static void shell_top_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    Float floor_height;
    Float red;
    Float green;
    Float blue;

    switch (work->mode) {
    case 0:
        work->scl.z = 1.0F; /* its opacity */
        work->mode++;
        /* fall through */
    case 1:
        work->scl.z -= 0.01F;
        if (work->scl.z < 0.0F) {
            task_kill(task);
        }
        motion->speed.y -= 0.05F;
        VECTOR_ADD(&work->pos, &motion->speed);
        floor_height = shadow_draw(work, 0.5F);
        if (floor_height > work->pos.y - 2.0F) {
            work->pos.y = floor_height + 2.0F;
            motion->speed.x *= 0.7F;
            motion->speed.z *= 0.7F;
            if (motion->speed.y < 0.0F) {
                motion->speed.y *= -0.6F;
            }
        }
        if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
            return;
        }
        draw_control_clear(NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL);
        draw_attr_save();
        draw_attr_set(0, NJD_FLAG_USE_ALPHA);
        njSetTexture(&alife_chao_texlist);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        njTranslate(NULL, 0.0F, -3.5F, 0.0F);
        njScale(NULL, 0.85F, 0.85F, 0.85F);
        /* @hack spelling: the three 1.5s passed through locals, which loads
         * 1.5 into FR4 and copies it to FR5, FR6 and FR7 before the alpha
         * takes FR4 (0x0c907422 to 0x0c907430); as three constants, the 1.5
         * loads into FR14, and 5 of 217 words differ */
        red = 1.5F;
        green = 1.5F;
        blue = 1.5F;
        draw_set_material(work->scl.z - 1.0F, red, green, blue);
        njSimpleDrawModel(shell_tops[work->tick]->model);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPopMatrix(1);
        draw_control_set(NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL);
        draw_attr_restore();
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        break;
    }
}

/* the bottom of a hatched egg's shell. it sinks to the floor and slides
 * along z, slower on the floor, as it fades out */
static void shell_bottom_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    Float floor_height;
    Float red;
    Float green;
    Float blue;

    switch (work->mode) {
    case 0:
        work->scl.z = 1.0F; /* its opacity */
        work->mode++;
        /* fall through */
    case 1:
        work->scl.z -= 0.01F;
        if (work->scl.z < 0.0F) {
            task_kill(task);
        }
        work->pos.x += motion->speed.x;
        work->pos.y += motion->speed.y - 0.05F;
        work->pos.z += motion->speed.z;
        floor_height = shadow_draw(work, 0.5F);
        if (floor_height > work->pos.y - 2.0F) {
            work->pos.y = floor_height + 2.0F;
            motion->speed.x *= 0.7F;
            motion->speed.z *= 0.7F;
            if (motion->speed.y < 0.0F) {
                motion->speed.y *= -0.6F;
            }
        }
        if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
            return;
        }
        draw_control_clear(NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL);
        draw_attr_save();
        draw_attr_set(0, NJD_FLAG_USE_ALPHA);
        njSetTexture(&alife_chao_texlist);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        njTranslate(NULL, 0.0F, -2.0F, 0.0F);
        njScale(NULL, 0.85F, 0.85F, 0.85F);
        /* @hack spelling: the three 1.5s passed through locals, which loads
         * 1.5 into FR4 and copies it to FR6, FR5 and FR7 before the alpha
         * takes FR4 (0x0c9075f0 to 0x0c907688); as three constants, the 1.5
         * loads into FR14, and 4 of 214 words differ */
        red = 1.5F;
        green = 1.5F;
        blue = 1.5F;
        draw_set_material(work->scl.z - 1.0F, red, green, blue);
        njSimpleDrawModel(shell_bottoms[work->tick]->model);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPopMatrix(1);
        draw_control_set(NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL);
        draw_attr_restore();
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        break;
    }
}

static void egg_init(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;

    collision_init(task, chao_collisions, COLLISION_ENEMY, 1, 2);
    work->collision->flags &= ~COLLISION_TARGET;
    /* guard 3: an attack, at most 3, never exceeds it, so no shape hurts
     * these */
    work->collision->shapes[0].levels =
        (work->collision->shapes[0].levels & ~SHAPE_GUARD) | GUARD(3);
    work->collision->shapes[1].levels =
        (work->collision->shapes[1].levels & ~SHAPE_GUARD) | GUARD(3);
    work->ang.x = 0;
    work->ang.z = 0;
    chao->spin = 0;
    chao->unk_080 = 0;
    chao->v_pos = chao->record->v_pos;
    chao->h_pos = chao->record->h_pos;
    chao->growth = chao->record->growth;
    VARS(work)->wait = (Uint32)(rand() * (7200 * RAND_SCALE)) + 14400;
    work->tick = 0;
    work->timer = 0;
    VARS(work)->shakes = 0;
    alife_terrain_init(work, chao);
    fun_0c90d634(work, chao);
    alife_set_home(work, chao);
    carry_set_kind(work, task->motion, CARRY_KIND_SMALL);
    alife_entry_add(task, ALIFE_CHAO, ALIFE_EGG, 2.5F);
    work->flags &= ~(EGG_SHAKEN | EGG_THROWN | EGG_HATCHED);
    if (chao->record == chao_get_carried()) {
        player_hand_task(0, task);
        chao_clear_carried();
    }
}

static void egg_destroy(struct task *task);
static void egg_step(struct task *task);
static void egg_draw(struct task *task);

void alife_egg_start(struct task *task)
{
    stage_get_stage_act();
    task->destructor = egg_destroy;
    egg_init(task);
    task->step = egg_step;
    task->draw = egg_draw;
}

static void egg_destroy(struct task *task)
{
    struct alife_chao *chao = (struct alife_chao *)task->motion;
    struct alife_entry *entry = task->extra;

    /* a hatch adds a node for the record at the head of its garden's list.
     * while that node is the head, chao_list_remove finds it first, keeps
     * it, and drops every node after it */
    chao_list_remove(chao->record);
    alife_entry_remove(entry);
}

static void egg_show(struct task_work *work, struct alife_chao *chao)
{
    struct chao *record = chao->record;
    Float width = work->scl.x * 0.85F;
    Float height = work->scl.y * 0.85F;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        return;
    }
    draw_control_clear(NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL);
    njSetTexture(&alife_chao_texlist);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njScale(NULL, width, height, width);
    draw_set_material(0.0F, 1.5F, 1.5F, 1.5F);
    if (work->flags & EGG_HATCHED) {
        /* @hack spelling: the model read through the object's address, which
         * pools the object and loads the model from 4 past it (0x0c9079cc);
         * as alife_egg_bottom_object.model, the pool holds the member's
         * address, and 2 of 158 words differ */
        njSimpleDrawModel((&alife_egg_bottom_object)->model);
    } else if (record->flags & CHAO_GOLD) {
        draw_tree(&alife_gold_egg_object);
    } else if (record->flags & CHAO_PAL) {
        draw_tree(&alife_pal_egg_object);
    } else if (record->flags & CHAO_BEGG) {
        draw_tree(&alife_begg_egg_object);
    } else {
        draw_tree(&alife_egg_object);
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPopMatrix(1);
    draw_control_set(NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL);
}

static void egg_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;

    egg_show(work, chao);
}

/* left alone, the egg waits, stirs, hops three times and hatches into a
 * Chao whose like for player 0's character is 50. thrown as it waits, it
 * hatches once it lands, into one whose like is -50. while edit_mode is
 * set, a free egg does nothing */
static void egg_free(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;
    struct chao *record;
    struct task *born;
    struct task_work *child_work;
    struct alife_chao *born_chao;
    struct task *piece;
    struct task_work *piece_work;

    if (!edit_mode) {
        switch (work->mode) {
        case EGG_WAIT:
            VARS(work)->wait--;
            if (VARS(work)->wait < 100 || (work->flags & EGG_SHAKEN)) {
                work->mode = EGG_STIR;
                work->submode = 0;
                work->timer = 0;
            }
            if (work->flags & EGG_THROWN) {
                work->mode = EGG_LAND;
                work->submode = 0;
                work->timer = 0;
            }
            break;
        case EGG_STIR:
            if (TASK_TIMER(work) > 120 && (work->flags & CHAO_ON_GROUND)) {
                work->mode = EGG_HOP_1;
                work->submode = 0;
                work->timer = 0;
                sound_play_at(SE_HOP, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
                chao->speed.y = 0.5F;
                chao->scale_speed = 0.16F;
            }
            break;
        case EGG_HOP_1:
            if (TASK_TIMER(work) > 60 && (work->flags & CHAO_ON_GROUND)) {
                work->mode = EGG_HOP_2;
                work->submode = 0;
                work->timer = 0;
                sound_play_at(SE_HOP, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
                chao->speed.y = 0.5F;
                chao->scale_speed = 0.16F;
            }
            break;
        case EGG_HOP_2:
            if (TASK_TIMER(work) > 30 && (work->flags & CHAO_ON_GROUND)) {
                work->mode = EGG_HOP_3;
                work->submode = 0;
                work->timer = 0;
                sound_play_at(SE_HOP, 0, 0, -20, work->pos.x, work->pos.y,
                              work->pos.z);
                chao->speed.y = 0.5F;
                chao->scale_speed = 0.16F;
            }
            break;
        case EGG_HOP_3:
            if (TASK_TIMER(work) > 40 && (work->flags & CHAO_ON_GROUND)) {
                work->mode = EGG_HATCH;
                work->submode = 0;
                work->timer = 0;
            }
            break;
        case EGG_HATCH:
            if (TASK_TIMER(work) > 30) {
                record = chao->record;
                record->type = CHAO_CHILD;
                chao->record->user_like = 50;
                chao->record->likes[alife_character].like = 50;
                born = alife_chao_create(chao->record);
                child_work = born->work;
                born_chao = (struct alife_chao *)born->motion;
                born_chao->record->pos = work->pos;
                child_work->pos = work->pos;
                /* 13 is fun_0c911680's mode, one of those choose_play
                 * gives a glad Chao */
                child_work->mode = 13;
                sound_play_at(SE_CHAO_SQUEAK, 0, 0, -20, work->pos.x,
                              work->pos.y, work->pos.z);
                piece = task_create(TASK_MOTION | TASK_WORK, 3, shell_top_step);
                piece->work->pos = work->pos;
                piece->motion->speed.x = ((rand() * RAND_SCALE) - 0.5F) * 0.6F;
                piece->motion->speed.y = 0.8F;
                piece->motion->speed.z = ((rand() * RAND_SCALE) - 0.5F) * 0.6F;
                chao_list_add(alife_garden, chao->record);
                chao->record->garden = alife_garden;
                piece_work = piece->work;
                if (record->flags & CHAO_GOLD) {
                    piece_work->tick = SHELL_GOLD;
                } else if (record->flags & CHAO_PAL) {
                    piece_work->tick = SHELL_PAL;
                } else if (record->flags & CHAO_BEGG) {
                    piece_work->tick = SHELL_BEGG;
                } else {
                    piece_work->tick = SHELL_PLAIN;
                }
                piece =
                    task_create(TASK_MOTION | TASK_WORK, 3, shell_bottom_step);
                piece->work->pos = work->pos;
                piece->motion->speed.x = 0.0F;
                piece->motion->speed.y = 0.0F;
                piece->motion->speed.z = 0.6F;
                piece_work = piece->work;
                if (record->flags & CHAO_GOLD) {
                    piece_work->tick = SHELL_GOLD;
                } else if (record->flags & CHAO_PAL) {
                    piece_work->tick = SHELL_PAL;
                } else if (record->flags & CHAO_BEGG) {
                    piece_work->tick = SHELL_BEGG;
                } else {
                    piece_work->tick = SHELL_PLAIN;
                }
                work->flags |= EGG_HATCHED;
                work->mode = EGG_GONE;
                work->submode = 0;
                work->timer = 0;
            }
            break;
        case EGG_LAND:
            if (work->flags & CHAO_ON_GROUND) {
                work->mode = EGG_CRACK;
                work->submode = 0;
                work->timer = 0;
            }
            break;
        case EGG_CRACK:
            record = chao->record;
            record->type = CHAO_CHILD;
            chao->record->user_like = -50;
            chao->record->likes[alife_character].like = -50;
            born = alife_chao_create(chao->record);
            child_work = born->work;
            born_chao = (struct alife_chao *)born->motion;
            born_chao->record->pos = work->pos;
            child_work->pos = work->pos;
            born_chao->speed = chao->speed;
            child_work->mode = MODE_NEWBORN;
            chao_list_add(alife_garden, chao->record);
            chao->record->garden = alife_garden;
            piece = task_create(TASK_MOTION | TASK_WORK, 3, shell_top_step);
            piece->work->pos = work->pos;
            piece->motion->speed.x = ((rand() * RAND_SCALE) - 0.5F) * 0.6F;
            piece->motion->speed.y = 0.8F;
            piece->motion->speed.z = ((rand() * RAND_SCALE) - 0.5F) * 0.6F;
            /* @hack redundant: a never-run store of EGG_GONE, 8, which SHC
             * weighs, and so loads 600 into R8 and 8 into R9 (0x0c907b0a,
             * 0x0c907b24), as the image does; without it, the two trade and 23
             * of 701 words differ */
            if (0) {
                work->mode = EGG_GONE;
            }
            piece_work = piece->work;
            if (record->flags & CHAO_GOLD) {
                piece_work->tick = SHELL_GOLD;
            } else if (record->flags & CHAO_PAL) {
                piece_work->tick = SHELL_PAL;
            } else if (record->flags & CHAO_BEGG) {
                piece_work->tick = SHELL_BEGG;
            } else {
                piece_work->tick = SHELL_PLAIN;
            }
            piece = task_create(TASK_MOTION | TASK_WORK, 3, shell_bottom_step);
            piece->work->pos = work->pos;
            piece->motion->speed.x = 0.0F;
            piece->motion->speed.y = 0.0F;
            piece->motion->speed.z = 0.6F;
            piece_work = piece->work;
            if (record->flags & CHAO_GOLD) {
                piece_work->tick = SHELL_GOLD;
            } else if (record->flags & CHAO_PAL) {
                piece_work->tick = SHELL_PAL;
            } else if (record->flags & CHAO_BEGG) {
                piece_work->tick = SHELL_BEGG;
            } else {
                piece_work->tick = SHELL_PLAIN;
            }
            work->mode = EGG_GONE;
            work->submode = 0;
            work->timer = 0;
            break;
        case EGG_GONE:
            task_kill(task);
            break;
        }
        if (work->flags & CHAO_ON_GROUND) {
            chao->accel.x = -(chao->speed.x * 0.8F);
            chao->accel.z = -(chao->speed.z * 0.8F);
            chao->accel.y = 0.0F;
            if (chao->speed.y < -0.3F) {
                chao->scale_speed = 0.16F;
            }
        }
        alife_apply_speed(work, chao);
        alife_collide_terrain(work, chao);
        if (work->flags & CARRY_HELD) {
            chao->handling = HANDLING_HELD;
            work->submode = 0;
            work->timer = 0;
            status_raise_like(work, chao, 1);
        } else if (!(chao->flags & 0x40)) {
            collision_register(work);
        }
    }
}

/* in player 0's hands. past 10 shakes, it stirs as soon as it is free.
 * dropped, it is thrown if it moves fast enough, sits on the VMS if that is
 * the first entry it touches, or goes free. while a warp has set
 * chao_get_carried_to's garden, the egg becomes the carried Chao and moves
 * to that garden */
static void egg_held(struct task *task)
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
        work->submode++;
        /* fall through */
    case 1:
        work->ang.y = -player_work[0]->ang.y;
        magnitude = njScalor(&chao->accel);
        if (magnitude > 0.01F && TASK_TIMER(work)++ > 20) {
            VARS(work)->shakes++;
            work->timer = 0;
            if (VARS(work)->shakes > 10) {
                work->flags |= EGG_SHAKEN;
            }
        }
        if (!(work->flags & CARRY_HELD)) {
            magnitude = VECTOR_SQUARED(&chao->speed);
            if (magnitude > 1.0F) {
                chao->handling = HANDLING_THROWN;
                work->timer = 0;
                chao->speed.x *= 0.6F;
                chao->speed.y *= 1.4F;
                chao->speed.z *= 0.6F;
                if (njScalor(&chao->speed) > 2.0F) {
                    njUnitVector(&chao->speed);
                    chao->speed.x *= 2.0F;
                    chao->speed.y *= 2.0F;
                    chao->speed.z *= 2.0F;
                }
                status_hurt(work, chao, 3);
                work->flags |= EGG_THROWN;
            } else if ((entry->flags & ENTRY_TOUCHING) &&
                       entry->hits[0]->kind == ALIFE_VMS) {
                chao->handling = HANDLING_VMS;
            } else {
                chao->handling = HANDLING_FREE;
            }
        } else if (!(player_work[0]->flags & PLAYER_HOLDING)) {
            chao->handling = HANDLING_FREE;
        } else {
            garden = chao_get_carried_to();
            if (garden != -1) {
                chao_set_carried(chao->record);
                chao_move(chao->record, garden);
            }
        }
        break;
    }
}

/* thrown. it falls, and goes free once it has landed and its timer, reset
 * at the throw, passes 100. picked up, it is held again */
static void egg_thrown(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;

    chao->speed.y -= 0.05F;
    VECTOR_ADD(&work->pos, &chao->speed);
    work->ang.x = 0;
    work->ang.z = 0;
    switch (work->submode) {
    case 0:
        work->submode++;
        break;
    case 1:
        if (work->flags & CHAO_ON_GROUND) {
            work->submode++;
        }
        break;
    case 2:
        if (TASK_TIMER(work) > 100) {
            chao->handling = HANDLING_FREE;
        }
        break;
    }
    alife_collide_terrain(work, chao);
    if (work->flags & CHAO_ON_GROUND) {
        /* @hack spelling: each product added negated, by an FNEG and an
         * FADD (0x0c9084f0 to 0x0c908500); as -=, each is one FSUB, the
         * function runs 4 bytes short, and 48 of 130 words differ */
        chao->accel.x += -(chao->speed.x * 0.5F);
        chao->accel.z += -(chao->speed.z * 0.5F);
        if (chao->speed.y < -0.2F) {
            chao->scale_speed = 0.16F;
        }
    }
    if (work->flags & CARRY_HELD) {
        chao->handling = HANDLING_HELD;
        work->submode = 0;
        work->timer = 0;
        status_raise_like(work, chao, 1);
    } else {
        collision_register(work);
    }
}

/* on the VMS. the egg goes free once its timer passes 300. one whose entry
 * is asked to let go before then sinks instead, turning, to a y of -100 */
static void egg_on_vms(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;
    struct alife_entry *entry = task->extra;

    switch (work->submode) {
    case 0:
        work->submode++;
        /* fall through */
    case 1:
        if (entry->flags & ENTRY_LET_GO) {
            work->submode++;
        }
        if (TASK_TIMER(work) > 300) {
            chao->handling = HANDLING_FREE;
        }
        /* @hack spelling: a return where control returns anyway, which ends the
         * case on an RTS with the store of handling in its slot
         * (0x0c9085c0); as a break, 1 of 50 words differ */
        return;
    case 2:
        work->pos.y -= 0.16F;
        work->ang.y += 0x1200;
        if (work->pos.y < -100.0F) {
            work->pos.y = -100.0F;
        }
        break;
    default:
        /* @hack redundant: a default that returns, which ends the submode tests
         * on an RTS (0x0c908562); without it, 1 of 50 words differ */
        return;
    }
}

/* HANDLING_REBORN's egg keeps still until its submode is set to 1. it then goes
 * free once its timer passes 60 */
static void egg_still(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;

    switch (work->submode) {
    case 0:
        break;
    case 1:
        if (TASK_TIMER(work) > 60) {
            chao->handling = HANDLING_FREE;
            work->submode = 0;
        }
        break;
    }
}

static void egg_step(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;

    if (!alife_paused) {
        switch (chao->handling) {
        case HANDLING_FREE:
            egg_free(task);
            break;
        case HANDLING_HELD:
            egg_held(task);
            break;
        case HANDLING_THROWN:
            egg_thrown(task);
            break;
        case HANDLING_VMS:
            egg_on_vms(task);
            break;
        case HANDLING_REBORN:
            egg_still(task);
            break;
        }
        alife_squash_step(work, chao);
        egg_show(work, chao);
        alife_save_pos(work, chao);
        work->tick++;
        work->timer++;
    } else {
        if (chao->handling == HANDLING_VMS) {
            egg_on_vms(task);
        }
        egg_show(work, chao);
    }
}
