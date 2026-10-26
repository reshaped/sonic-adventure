/* the step that runs a Chao's behavior by its mode, the idle and newborn
 * behaviors, and the choices of the next mode */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/alife.h"
#include "al_main/behavior.h"
#include "al_main/chao.h"
#include "al_main/court.h"
#include "al_main/entry.h"
#include "al_main/express.h"
#include "al_main/face.h"
#include "al_main/follow.h"
#include "al_main/gesture.h"
#include "al_main/hold.h"
#include "al_main/hurt.h"
#include "al_main/icon.h"
#include "al_main/life.h"
#include "al_main/mood.h"
#include "al_main/move.h"
#include "al_main/music.h"
#include "al_main/physics.h"
#include "al_main/sleep.h"
#include "al_main/take_animal.h"
#include "chao/chao.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

static void alife_idle(struct task_work *work, struct alife_chao *chao);
static void alife_newborn(struct task_work *work, struct alife_chao *chao);

/* each behavior, by the Chao's work->mode, from 0 to 42 */
static void (*alife_behaviors[])(struct task_work *work,
                                 struct alife_chao *chao) = {
    alife_idle,         alife_go,      alife_newborn,
    alife_sleep,        alife_hurt,    alife_take_animal,
    alife_hold,         alife_evolve,  alife_reincarnate,
    alife_go_to_player, fun_0c917140,  fun_0c914380,
    fun_0c9145a0,       fun_0c911680,  alife_hungry,
    fun_0c911ac0,       fun_0c911c38,  fun_0c911d5e,
    fun_0c911e14,       fun_0c911f40,  fun_0c912040,
    fun_0c912120,       fun_0c91221a,  alife_sleepy,
    fun_0c9124c4,       alife_trumpet, fun_0c912600,
    alife_drum,         alife_sing,    fun_0c913280,
    fun_0c9170c0,       fun_0c917240,  alife_court,
    alife_partner,      alife_breed,   alife_join_court,
    fun_0c9134c0,       fun_0c9136e0,  alife_mark_favorite,
    fun_0c912ba0,       fun_0c912ca8,  fun_0c912daa,
    fun_0c9130c6,
};

/* the modes choose_play picks from. like_modes is for a Chao that likes
 * player 0's character, and dislike_modes, angry_modes and afraid_modes for
 * one that dislikes it. sad_modes and glad_modes follow its sorrow and
 * pleasure, and play_modes is for the rest. choose_play reads no count: it
 * scales rand() to just under a list's count, as 7.99F for like_modes' 8 */
/* clang-format off */
static int like_modes[8] = {16, 13, 15, 24, MODE_SING, 17, 37,
                            MODE_GO_TO_PLAYER};
static int like_count = 8;
static int dislike_modes[1] = {20};
static int dislike_count = 1;
static int angry_modes[2] = {29, 36};
static int angry_count = 2;
static int afraid_modes[2] = {22, 10};
static int afraid_count = 2;
static int sad_modes[3] = {21, 22, MODE_HUNGRY};
static int sad_count = 3;
static int glad_modes[4] = {24, MODE_SING, 13, 15};
static int glad_count = 4;
static int play_modes[13] = {26, MODE_DRUM, 11, 12, MODE_TRUMPET, 39, 41,
                             MODE_MARK_FAVORITE, 42, 36, 24, MODE_SING, 17};
static int play_count = 13;
/* clang-format on */

/* a place among the four nearest the Chao, and its distance */
struct near_place {
    Float distance;
    struct alife_entry *entry;
};

static struct near_place near_places[4];

#define SE_HATCH 0x265
#define SE_BABY 0x367
#define SE_VOICE 0x36a
#define SE_VOICE_2 0x348

/* the motion a hit Chao gets up by, in hurt.c */
#define MOTION_GET_UP 2

/* the behavior of a Chao just hatched. 25 steps in, it shows other eyes,
 * and a mouth by how aggressive its record says it is, each for 200 steps.
 * past 200 steps it gets up, ends the other eyes and sets both eye models'
 * texture. once MOTION_GET_UP ends, it goes idle */
static void alife_newborn(struct task_work *work, struct alife_chao *chao)
{
    struct chao_motion *motion = &chao->motion;

    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 1;
        work->submode++;
        break;
    case 1:
        if (TASK_TIMER(work) == 25) {
            Sint8 agres = chao->record->agres;

            alife_face_set_eyes(chao, 1, 200);
            if (agres > 50) {
                alife_face_set_mouth(chao, 3, 200);
            } else if (agres > 0) {
                alife_face_set_mouth(chao, 2, 200);
            } else if (agres > -50) {
                alife_face_set_mouth(chao, 5, 200);
            } else {
                alife_face_set_mouth(chao, 6, 200);
            }
            sound_play(SE_HATCH, 0, 0, 0);
            chao->scale_speed = 0.16F;
        }
        if (TASK_TIMER(work) > 200) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = MOTION_GET_UP;
            alife_face_set_eyes(chao, 0, 0);
            chao->shape.objects[NODE_EYE_0]->model->mats[0].attr_texId = 2;
            chao->shape.objects[NODE_EYE_1]->model->mats[0].attr_texId = 2;
            work->submode++;
        }
        break;
    case 2:
        if (motion->flags & MOTION_ENDED) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            chao->last_mode = work->mode;
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
            chao->mode_timer = 0;
        }
        break;
    }
    chao->accel.x = -chao->speed.x * 0.2F;
    chao->accel.z = -chao->speed.z * 0.2F;
}

/* TRUE while player 0 is within 30 of the Chao, and FALSE otherwise */
static Bool player_near(struct task_work *work, struct alife_chao *chao)
{
    NJS_POINT3 *pos = &player_work[0]->pos;

    if (njDistanceP2P(pos, &work->pos) < 30.0F) {
        return TRUE;
    }
    return FALSE;
}

void alife_go_to_place(struct task_work *work, struct alife_chao *chao)
{
    struct alife_entry *entry = alife_get_place_list();
    struct alife_entry *chosen;
    int i;
    int farthest;
    int kind;
    int index;
    Float distance;
    /* @hack spelling: ICON_GLAD_1 through a local, which SHC loads into R11
     * once (MOV #1,R11 at 0x0c901024); spelled in place, the function runs 2
     * bytes long and 128 of 351 words differ */
    int glad_icon = ICON_GLAD_1;

    for (i = 0; i < 4; i++) {
        near_places[i].distance = 1.0e8F;
        near_places[i].entry = NULL;
    }
    while (entry) {
        farthest = 0;
        for (i = 1; i < 4; i++) {
            if (near_places[farthest].distance < near_places[i].distance) {
                farthest = i;
            }
        }
        distance = njDistanceP2P(&entry->task->work->pos, &work->pos);
        if (distance < near_places[farthest].distance) {
            near_places[farthest].distance = distance;
            near_places[farthest].entry = entry;
        }
        entry = entry->next;
    }
    index = (int)(rand() * (3.99F * RAND_SCALE));
    chosen = near_places[index].entry;
    for (i = 0; i < 4; i++) {
        if (chao->record->hunger > 150) {
            /* @bug memory: with fewer than four places in the garden, an empty
             * slot's entry is null, and this reads the kind through it (MOV.W
             * @(2,R4),R0 at 0x0c9010b6, 0x0c901106, 0x0c9011aa and
             * 0x0c9011fa) */
            kind = near_places[i].entry->kind;
            if (kind == ALIFE_TREE || kind == ALIFE_FRUIT) {
                if (rand() * RAND_SCALE < 0.4F) {
                    chosen = near_places[i].entry;
                    chao->icon.kind = glad_icon;
                    chao->icon.symbol.state = 0;
                    chao->icon.ball.state = 0;
                    chao->icon.timer = 60;
                }
            }
        }
    }
    if (chosen) {
        int ang = (int)(rand() * 1.9999999F);
        Float radius = chosen->radius;

        chao->target = chosen->task->work->pos;
        chao->target.x += njCos(ang) * radius;
        chao->target.z += njSin(ang) * radius;
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        chao->last_mode = work->mode;
        work->mode = MODE_GO;
        work->submode = 0;
        work->timer = 0;
    } else {
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        chao->last_mode = work->mode;
        work->mode = MODE_IDLE;
        work->submode = 0;
        work->timer = 0;
    }
    /* @hack redundant: a never-run store of 1 to the chosen place's kind, which
     * keeps the image's R10, R11 and R12 (0x0c900ffe, 0x0c901024, 0x0c9010b4);
     * without it, the three rotate and 34 of 351 words differ */
    if (0) {
        chosen->kind = 1;
    }
    chao->mode_timer = 0;
}

/* switch the Chao to mode to, at submode 0 with its timer and mode_timer
 * at 0. last_mode keeps the mode it leaves */
/* NOLINTBEGIN(bugprone-signed-char-misuse) */
#define SET_MODE(work, chao, to)                                               \
    ((chao)->last_mode = (work)->mode, (work)->mode = (to),                    \
     (work)->submode = 0, (work)->timer = 0, (chao)->mode_timer = 0)
/* NOLINTEND(bugprone-signed-char-misuse) */

/* the mode an idle Chao with no need takes. the higher record->tedious,
 * the likelier it plays, and a play takes 20 from it, down to 0. with
 * player 0 near, a strong like or dislike for the player's character can
 * pick the play, and otherwise its sorrow and pleasure pick it. a Chao that
 * doesn't play goes to a place, the likelier the higher
 * record->skills.run, or takes MODE_NO_PLAY, whose behavior is fun_0c912ca8 */
static void choose_play(struct task_work *work, struct alife_chao *chao)
{
    struct chao *record;
    int index;
    int character = alife_character;
    Sint8 like = chao->record->likes[character].like;

    record = chao->record;
    if (rand() * RAND_SCALE < (record->tedious * 0.005F) - 0.1F) {
        if (record->tedious > 20) {
            record->tedious -= 20;
        } else {
            record->tedious = 0;
        }
        /* @hack redundant: a never-run call of rand, which keeps rand's address
         * in R11 (0x0c901338), as the image has it; without it, R10 and R11
         * trade and 24 of 330 words differ */
        if (0) {
            index = rand();
        }
        if (player_near(work, chao) && rand() * RAND_SCALE < 0.5F) {
            if (like > 40 && rand() * RAND_SCALE < 0.5F) {
                if (rand() * RAND_SCALE < 0.6F) {
                    index = (int)(rand() * (7.99F * RAND_SCALE));
                    SET_MODE(work, chao, like_modes[index]);
                    return;
                } else {
                    SET_MODE(work, chao, MODE_GO_TO_PLAYER);
                    return;
                }
            }
            if (rand() * RAND_SCALE > 0.5F && like < -40) {
                if (record->anger > 50) {
                    index = (int)(rand() * (1.9F * RAND_SCALE));
                    SET_MODE(work, chao, angry_modes[index]);
                    return;
                }
                if (record->fear > 50) {
                    index = (int)(rand() * (1.9F * RAND_SCALE));
                    SET_MODE(work, chao, afraid_modes[index]);
                    return;
                } else {
                    index = (int)(rand() * (0.9F * RAND_SCALE));
                    SET_MODE(work, chao, dislike_modes[index]);
                    return;
                }
            }
        }
        if (record->sorrow > 50) {
            index = (int)(rand() * (2.9F * RAND_SCALE));
            SET_MODE(work, chao, sad_modes[index]);
            return;
        }
        if (record->pleasure > 50) {
            index = (int)(rand() * (3.9F * RAND_SCALE));
            SET_MODE(work, chao, glad_modes[index]);
            return;
        } else {
            index = (int)(rand() * (12.9F * RAND_SCALE));
            SET_MODE(work, chao, play_modes[index]);
            return;
        }
    } else if (rand() * RAND_SCALE < (record->skills.run * 0.000025F) + 0.6F) {
        alife_go_to_place(work, chao);
    } else {
        SET_MODE(work, chao, MODE_NO_PLAY);
    }
}

/* the idle behavior. its first step plays an idle motion, then it waits
 * until its timer passes 120. any Chao but a child below growth 0.18 skips
 * both 7 times in 10. then it tests its needs in turn: hunger, breeding, a
 * courting Chao it touches, sleepiness, sleep, growth and the end of its
 * life. each need whose test holds sets the mode, and the last one wins.
 * each but MODE_JOIN_COURT is set by SET_MODE, which keeps the mode it
 * replaces in last_mode. so of those, the second-last need's mode ends in
 * last_mode, or MODE_IDLE when only one holds. MODE_JOIN_COURT leaves
 * last_mode and mode_timer as they were. with no need, choose_play sets the
 * mode */
static void alife_idle(struct task_work *work, struct alife_chao *chao)
{
    struct chao *record = chao->record;
    Float chance = rand() * RAND_SCALE;
    int chosen = 0;
    struct alife_entry *entry;

    switch (work->submode) {
    case 0:
        if (record->type == CHAO_CHILD && record->growth < 0.18F) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 84;
            work->submode++;
            sound_play_at(SE_BABY, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
            alife_face_set_mouth(chao, 0, 2);
        } else if (chance < 0.1F) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = 4;
            work->submode++;
            break;
        } else {
            if (chance < 0.3F) {
                chance = rand() * RAND_SCALE;
                chao->motion.flags |= MOTION_NEW;
                chao->motion.id = 115;
                chao->icon.kind = ICON_SYMBOL_3;
                chao->icon.symbol.state = 0;
                chao->icon.ball.state = 0;
                chao->icon.timer = 60;
                if (chance < 0.3F) {
                    sound_play_at(SE_VOICE, 0, 0, -20, work->pos.x, work->pos.y,
                                  work->pos.z);
                } else {
                    sound_play_at(SE_VOICE_2, 0, 0, -20, work->pos.x,
                                  work->pos.y, work->pos.z);
                }
            } else {
                work->submode++;
            }
            work->submode++;
        }
        break;
    case 1:
        if (TASK_TIMER(work) > 120) {
            work->submode++;
        }
        break;
    case 2:
        if (record->hunger > 180) {
            if (rand() * RAND_SCALE < (record->hunger - 180) * 0.025F) {
                SET_MODE(work, chao, MODE_HUNGRY);
                chosen = 1;
            }
        }
        if (record->type != CHAO_CHILD && record->type != CHAO_CHAOS) {
            if (record->breed > 195 && (work->flags & CHAO_ON_GROUND)) {
                if (alife_get_chao_count() < 8) {
                    SET_MODE(work, chao, MODE_COURT);
                    chosen = 1;
                }
            }
        }
        if (record->type != CHAO_CHILD && record->type != CHAO_CHAOS) {
            if ((entry = alife_entry_touching(work, chao, ALIFE_COURTING)) !=
                NULL) {
                chao->target = entry->task->work->pos;
                work->mode = MODE_JOIN_COURT;
                work->submode = 0;
                work->timer = 0;
                chosen = 1;
            }
        }
        if (record->sleep > 170 && (work->flags & CHAO_ON_GROUND)) {
            if (rand() * RAND_SCALE > ((200 - record->sleep) * 0.025F) + 0.3F) {
                SET_MODE(work, chao, MODE_SLEEPY);
                chosen = 1;
            }
        }
        if (record->sleep > 195 && (work->flags & CHAO_ON_GROUND)) {
            chosen = 1;
            SET_MODE(work, chao, MODE_SLEEP);
        }
        if (record->type == CHAO_CHILD && record->growth > 1.0F &&
            (work->flags & CHAO_ON_GROUND)) {
            SET_MODE(work, chao, MODE_GROW_UP);
            chosen = 1;
        }
        if (record->hp == 0 ||
            (record->life == 0 && (work->flags & CHAO_ON_GROUND))) {
            SET_MODE(work, chao, MODE_LIFE_OVER);
            chosen = 1;
        }
        if (!chosen) {
            choose_play(work, chao);
        }
        break;
    }
    if (work->flags & CHAO_ON_GROUND) {
        chao->accel.x = -chao->speed.x * 0.8F;
        chao->accel.z = -chao->speed.z * 0.8F;
        chao->accel.y = 0;
    }
}

/* empty, with a behavior's parameters, and no caller in this image. it
 * isn't static, since SHC drops an uncalled static */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
void fun_0c901a0a(struct task_work *work, struct alife_chao *chao)
{
}

void alife_behave(struct task_work *work, struct alife_chao *chao)
{
    /* @hack spelling: both positions walked as Floats from their x, for the
     * image's @Rn+ loads; as byte cursors, 8 of 53 words differ */
    Float *pos = (Float *)&work->pos;
    Float *player_pos = (Float *)&player_work[0]->pos;
    Uint32 *flags = &chao->player_flags;
    Float x = *pos++ - *player_pos++;
    Float y = *pos++ - *player_pos++;
    Float z = *pos - *player_pos;

    if ((x * x) + (y * y) + (z * z) < 10000.0F) {
        *flags |= CHAO_NEAR_PLAYER;
        /* @hack redundant: a never-run store of z through player_pos, which
         * keeps the image's FR4 and FR6 in the sum of squares (0x0c901a56,
         * 0x0c901a64); without it, the two trade and 8 of 53 words differ */
        if (0) {
            *player_pos = z;
        }
    } else {
        *flags &= ~CHAO_NEAR_PLAYER;
    }
    alife_behaviors[work->mode](work, chao);
    alife_apply_speed(work, chao);
}

Bool alife_interrupt(struct task_work *work, struct alife_chao *chao)
{
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int mode = work->mode;

    if (mode == MODE_GROW_UP || mode == MODE_LIFE_OVER) {
        return FALSE;
    }
    if (fun_0c915b00(work, chao) || alife_take_hit(work, chao) ||
        alife_try_take_animal(work, chao) || alife_try_hold(work, chao)) {
        return TRUE;
    }
    return FALSE;
}
