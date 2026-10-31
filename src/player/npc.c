/* the npc: Tails as player 1, following player 0, and the title card */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "camera/battle.h"
#include "collision/collision.h"
#include "draw/screen.h"
#include "event/play.h"
#include "main.h"
#include "object/common.h"
#include "player/tails.h"
#include "player/npc.h"
#include "player/player.h"
#include "progress/progress.h"
#include "save/card.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/timer.h"

/* the npc task's modes. nothing in this file enters NPC_LEAVE, and
 * NPC_STOPPED follows only NPC_LOST_STOP, which npc_watch never returns */
enum npc_mode {
    NPC_START, /* put Tails behind player 0, and follow */
    NPC_WAIT, /* out of sight, until port B presses Start */
    NPC_DROP_IN, /* drop Tails in from above the camera, flying */
    NPC_STOPPED, /* stop Tails and press jump for him, each step */
    NPC_FOLLOW, /* npc_steer drives Tails, until port B's pad is used */
    NPC_PAD, /* port B drives Tails, until its pad rests over IDLE_STEPS */
    NPC_HIDDEN, /* out of sight, until the npc may run again */
    NPC_LEAVE, /* out of sight, then NPC_GONE */
    NPC_GONE /* out of sight, until npc_disabled clears */
};

enum npc_lost { NPC_NOT_LOST, NPC_LOST, NPC_LOST_STOP };

/* how npc_steer drives Tails, in the npc's submode. nothing in this file
 * enters STEER_DESCEND or STEER_CATCH_UP */
enum steer_mode {
    STEER_START, /* wait until both players have records, then run */
    STEER_RUN,
    STEER_JUMP,
    STEER_FLY,
    /* fall, pressing nothing, until less than DESCEND_LEVEL above player 0,
     * then fly */
    STEER_DESCEND,
    STEER_CATCH_UP /* move Tails straight at player 0, a unit each step */
};

/* how far behind player 0 Tails starts, and how high above it he waits out
 * of sight */
#define START_BEHIND 10.0F
#define HIDDEN_HEIGHT 1000000.0F

/* the steps port B's pad rests before npc_steer takes Tails back */
#define IDLE_STEPS 300

/* npc_steer's aim, and its limits for Tails stuck */
#define AIM_DISTANCE 5.0F
#define STUCK_STEPS 60
#define STUCK_DISTANCE 30.0F

/* how far above player 0 Tails stops falling in STEER_DESCEND, and flies */
#define DESCEND_LEVEL 20.0F

/* the distances and steps npc_watch judges Tails lost by */
#define LOST_DISTANCE 1000.0F
#define NEAR_DISTANCE 50.0F
#define UNSEEN_STEPS 300

#define PAD_ANY_BUT_START                                                      \
    (PDD_DGT_TL | PDD_DGT_TR | PDD_DGT_TX | PDD_DGT_TY | PDD_DGT_KR |          \
     PDD_DGT_KL | PDD_DGT_KD | PDD_DGT_KU | PDD_DGT_TA | PDD_DGT_TB)
#define PAD_ANY (PAD_ANY_BUT_START | PDD_DGT_ST)

#define CLASH_FLAGS (PLAYER_GUARD | COLLISION_TOUCHING)

struct npc_vars {
    Uint16 unseen; /* steps in a row Tails has been out of view */
    Uint8 unk_02[2];
    /* the steps port B's pad has rested in NPC_PAD, and the steps NPC_LEAVE
     * has run */
    Sint16 timer;
    Sint16 still; /* steps in a row Tails has moved less than 1 */
};

#define NPC(work) ((struct npc_vars *)&(work)->vars)

struct title_card_name {
    Uint8 stage;
    char *name; /* the texture's file name */
};

struct title_card_list {
    Sint32 count;
    struct title_card_name *names;
};

#define LIST(names) {sizeof(names) / sizeof((names)[0]), (names)}

struct title_card {
    Uint8 unk_00; /* 2 once title_card_begin runs, and read nowhere */
    Uint8 frame; /* the frames it has shown */
    Uint8 stage;
    Uint8 kind;
    Uint16 frames; /* the frames it shows for */
    NJS_TEXLIST *texlist;
};

enum title_card_kind { TITLE_CARD_STAGE, TITLE_CARD_FIELD, TITLE_CARD_OTHER };

static void npc_destroy(struct task *task);
static void npc_step(struct task *task);
static void npc_steer(struct task *task);
static enum npc_lost npc_watch(struct task *task, struct task_work *leader,
                               struct task_work *follower);
static void npc_place_behind(struct task_work *work, NJS_POINT3 *pos,
                             Float distance);
static Bool npc_permit(void);
static Bool npc_story_allows(void);

static struct title_card_name sonic_names[] = {
    {STAGE_BEACH, "s_stage01"},    {STAGE_WINDY, "s_stage02"},
    {STAGE_CASINO, "s_stage03"},   {STAGE_ICECAP, "s_stage04"},
    {STAGE_TWINKLE, "s_stage05"},  {STAGE_HIGHWAY, "s_stage06"},
    {STAGE_MOUNTAIN, "s_stage07"}, {STAGE_SKY_DECK, "s_stage08"},
    {STAGE_RUIN, "s_stage09"},     {STAGE_FINAL, "s_stage10"},
};
static struct title_card_name tails_names[] = {
    {STAGE_WINDY, "m_stage01"},   {STAGE_CASINO, "m_stage02"},
    {STAGE_ICECAP, "m_stage03"},  {STAGE_SKY_DECK, "m_stage04"},
    {STAGE_HIGHWAY, "m_stage05"},
};
static struct title_card_name knuckles_names[] = {
    {STAGE_HIGHWAY, "k_stage01"},  {STAGE_CASINO, "k_stage02"},
    {STAGE_MOUNTAIN, "k_stage03"}, {STAGE_RUIN, "k_stage04"},
    {STAGE_SKY_DECK, "k_stage05"},
};
static struct title_card_name amy_names[] = {
    {STAGE_TWINKLE, "a_stage01"},
    {STAGE_SHELTER, "a_stage02"},
    {STAGE_FINAL, "a_stage03"},
};
static struct title_card_name big_names[] = {
    {STAGE_TWINKLE, "b_stage01"},
    {STAGE_ICECAP, "b_stage02"},
    {STAGE_BEACH, "b_stage03"},
    {STAGE_SHELTER, "b_stage04"},
};
static struct title_card_name e102_names[] = {
    {STAGE_FINAL, "e_stage01"},   {STAGE_BEACH, "e_stage02"},
    {STAGE_WINDY, "e_stage03"},   {STAGE_MOUNTAIN, "e_stage04"},
    {STAGE_SHELTER, "e_stage05"},
};
static struct title_card_list story_lists[] = {
    LIST(sonic_names), LIST(tails_names), LIST(knuckles_names),
    LIST(amy_names),   LIST(big_names),   LIST(e102_names),
};
static struct title_card_name field_names[] = {
    {STAGE_STATION_SQUARE, "t_stationsquare"},
    {STAGE_STATION_SQUARE_2, "t_stationsquare"},
    {STAGE_STATION_SQUARE_3, "t_stationsquare"},
    {STAGE_EGG_CARRIER, "t_eggcarrier"},
    {STAGE_EGG_CARRIER_2, "t_eggcarrier"},
    {STAGE_EGG_CARRIER_3, "t_eggcarrier"},
    {STAGE_EGG_CARRIER_INSIDE, "t_eggcarrier"},
    {STAGE_MYSTIC_RUINS, "t_misticruin"},
    {STAGE_PAST, "t_misticruin"},
};
static struct title_card_list field_list = LIST(field_names);
static struct title_card_name other_names[] = {
    {STAGE_CART, "st_stage03"},      {STAGE_SHOOTING1, "st_stage01"},
    {STAGE_SHOOTING2, "st_stage02"}, {STAGE_SUNABOARD, "st_stage04"},
    {STAGE_PRACTICE, "st_stage05"},
};
static struct title_card_list other_list = LIST(other_names);
static NJS_TEXNAME title_card_texture[1] = {{NULL, 0, 0}};
static NJS_TEXLIST title_card_texlist = {
#include "player/npc/title_card_texlist.inc"
};
/* each character's index into story_lists, or -1 if none */
static Sint8 story_list_of[CHARACTER_COUNT] = {0, -1, 1, 2, -1, 3, 5, 4};
static Sint32 dat_8c1d52bc = 6;

static struct title_card title_card;

struct task *npc_create(struct task *leader)
{
    struct task *task;
    struct task *tails;
    int stage;

    if (npc_task != NULL) {
        return npc_task;
    }
    /* @bug logic: the mask keeps only the stage's low four bits, so
     * STAGE_E101_R and STAGE_ALIFE_MR pass as STAGE_CASINO too (AND #15,R0
     * at 0x8c086620) */
    stage = (stage_get_stage_act() >> 8) & 0xf;
    if (stage != STAGE_CASINO && !npc_permit()) {
        return NULL;
    }
    task = task_create(TASK_WORK, 0, npc_step);
    npc_task = task;
    if (task != NULL) {
        PLAYER_VARS(task->work)->character = CHARACTER_TAILS;
        PLAYER_VARS(task->work)->player = 1;
        task->destructor = npc_destroy;
    } else {
        return NULL;
    }
    tails = task_create(TASK_MOTION | TASK_WORK | TASK_FORCE, 1, tails_step);
    if (tails != NULL) {
        PLAYER_VARS(tails->work)->character = CHARACTER_TAILS;
        PLAYER_VARS(tails->work)->player = 1;
        npc_place_behind(leader->work, &tails->work->pos, START_BEHIND);
    }
    task->work->mode = NPC_START;
    npc_disabled = FALSE;
    return task;
}

static void npc_destroy(struct task *task)
{
    npc_task = NULL;
}

void npc_kill(void)
{
    if (npc_task != NULL) {
        task_kill(npc_task);
        npc_task = NULL;
    }
}

Bool npc_follow(void)
{
    struct task *task = npc_task;

    if (task == NULL) {
        return FALSE;
    }
    task->work->mode = NPC_START;
    return TRUE;
}

Bool npc_wait(void)
{
    struct task *task = npc_task;

    player_get_task(1);
    if (task == NULL) {
        return FALSE;
    }
    player_set_mode(1, PLAYER_MODE_HOVER);
    pad_enable_player(1);
    task->work->mode = NPC_WAIT;
    return TRUE;
}

Bool npc_drop_in(void)
{
    struct task *task = npc_task;

    if (task == NULL) {
        return FALSE;
    }
    player_set_mode(1, PLAYER_MODE_RESET);
    pad_disable_player(1);
    task->work->mode = NPC_DROP_IN;
    return TRUE;
}

Bool npc_remove_tails(void)
{
    struct task *tails;
    struct player_vars *vars;

    if (npc_task == NULL) {
        return TRUE;
    }
    tails = player_get_task(1);
    if (tails != NULL) {
        vars = PLAYER_VARS(tails->work);
        if (vars->character == CHARACTER_TAILS && vars->player == 1) {
            task_kill(tails);
            return TRUE;
        }
    }
    return FALSE;
}

struct task *npc_create_tails(void)
{
    struct task *tails;

    if (npc_task == NULL) {
        return NULL;
    }
    tails = task_create(TASK_MOTION | TASK_WORK | TASK_FORCE, 1, tails_step);
    if (tails != NULL) {
        PLAYER_VARS(tails->work)->character = CHARACTER_TAILS;
        PLAYER_VARS(tails->work)->player = 1;
    }
    return tails;
}

/* halt player in the air: zero its speeds, and set PLAYER_MODE_FLY */
static void npc_halt_and_fly(Uint8 player)
{
    struct task_motion *motion = player_motion[player];
    struct player *record = player_get(player);

    if (motion != NULL && record != NULL) {
        motion->speed.z = 0.0F;
        motion->speed.y = 0.0F;
        motion->speed.x = 0.0F;
        record->speed.z = 0.0F;
        record->speed.y = 0.0F;
        record->speed.x = 0.0F;
        player_set_mode(player, PLAYER_MODE_FLY);
    }
}

/* run the npc's mode. Tails hides where the npc may not run, and his
 * shapes never hurt players */
static void npc_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *leader = player_work[0];
    struct task_work *follower = player_work[1];
    /* @hack redundant: a point never used, whose 12 bytes give the image's
     * 20-byte frame (ADD #-20,R15 at 0x8c0868b0); without it, 5 of 434
     * words differ */
    NJS_POINT3 unused;
    struct task *leader_task = player_get_task(0);
    struct task *follower_task = player_get_task(1);
    int result;
    int i;

    if (leader_task == NULL) {
        work->mode = NPC_START;
    }
    if (follower_task == NULL || leader == NULL || follower == NULL) {
        return;
    }
    if (!npc_permit()) {
        player_set_mode(1, PLAYER_MODE_STILL);
        pad_enable_player(1);
        work->mode = NPC_HIDDEN;
    }
    /* @hack redundant: follower tested for null again, which the return
     * above has ruled out (TST R14,R14 at 0x8c08690c); without it, 160 of
     * 434 words differ */
    if (follower != NULL) {
        for (i = 0; i < follower->collision->count; i++) {
            follower->collision->shapes[i].levels &= ~SHAPE_HURTS_PLAYERS;
        }
    }
    switch (work->mode) {
    case NPC_START:
        pad_disable_player(1);
        NPC(work)->timer = 0;
        npc_place_behind(leader, &follower->pos, START_BEHIND);
        player_set_history(1, 0, &player_work[1]->pos, NULL);
        work->mode = NPC_FOLLOW;
        work->submode = STEER_START;
        break;
    case NPC_WAIT:
        follower->pos = leader->pos;
        follower->pos.y += HIDDEN_HEIGHT;
        player_set_history(1, 0, &follower_task->work->pos, NULL);
        if ((pad_records[1].press & PDD_DGT_ST) && event_in_control() != TRUE &&
            npc_permit()) {
            npc_drop_in();
        }
        break;
    case NPC_DROP_IN:
        pad_disable_player(1);
        camera_get_pos(&follower->pos);
        follower->pos.y += 50.0F;
        player_set_history(1, 0, &follower_task->work->pos, NULL);
        npc_halt_and_fly(1);
        pad_records[1].on |= pad_jump_buttons;
        pad_records[1].press |= pad_jump_buttons;
        work->submode = STEER_FLY;
        work->mode = NPC_FOLLOW;
        break;
    case NPC_STOPPED:
        pad_disable_player(1);
        npc_halt_and_fly(1);
        pad_records[1].on |= pad_jump_buttons;
        pad_records[1].press |= pad_jump_buttons;
        work->submode = STEER_FLY;
        break;
    case NPC_FOLLOW:
        if ((per_info[1]->on & PAD_ANY_BUT_START) ||
            (Float)(per_info[1]->x1 << 8) != 0.0F ||
            (Float)(per_info[1]->y1 << 8) != 0.0F) {
            pad_enable_player(1);
            work->mode = NPC_PAD;
        } else {
            pad_disable_player(1);
        }
        npc_steer(task);
        result = npc_watch(task, leader, follower);
        switch (result) {
        case NPC_LOST:
            npc_drop_in();
            break;
        case NPC_LOST_STOP:
            work->mode = NPC_STOPPED;
        }
        break;
    case NPC_PAD:
        if (!(per_info[1]->on & PAD_ANY) &&
            (Float)(per_info[1]->x1 << 8) == 0.0F &&
            (Float)(per_info[1]->y1 << 8) == 0.0F) {
            NPC(work)->timer++;
            if (NPC(work)->timer > IDLE_STEPS) {
                pad_disable_player(1);
                work->mode = NPC_FOLLOW;
            }
        } else {
            pad_enable_player(1);
            NPC(work)->timer = 0;
        }
        result = npc_watch(task, leader, follower);
        switch (result) {
        case NPC_LOST:
            npc_drop_in();
            break;
        case NPC_LOST_STOP:
            work->mode = NPC_STOPPED;
        }
        break;
    case NPC_HIDDEN:
        follower->pos = leader->pos;
        follower->pos.y += HIDDEN_HEIGHT;
        player_set_history(1, 0, &follower_task->work->pos, NULL);
        if (npc_permit() == TRUE) {
            npc_follow();
        }
        break;
    case NPC_LEAVE:
        follower->pos = leader->pos;
        follower->pos.y += HIDDEN_HEIGHT;
        player_set_history(1, 0, &follower_task->work->pos, NULL);
        if (NPC(work)->timer++ > 3) {
            player_set_mode(1, PLAYER_MODE_STILL);
            work->mode = NPC_GONE;
        }
        break;
    case NPC_GONE:
        follower->pos = leader->pos;
        follower->pos.y += HIDDEN_HEIGHT;
        player_set_history(1, 0, &follower_task->work->pos, NULL);
        if (!npc_disabled) {
            player_set_mode(1, PLAYER_MODE_RESET);
            pad_disable_player(1);
            work->mode = NPC_DROP_IN;
        }
        break;
    }
}

/* steer Tails to the point AIM_DISTANCE from player 0 on his side, by
 * writing port B's buttons and stick for him. he is stuck once he has moved
 * less than 1 a step for over STUCK_STEPS steps in a row. stuck, he jumps
 * if he is more than STUCK_DISTANCE from the point */
static void npc_steer(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *leader = player_work[0];
    struct task_work *follower = player_work[1];
    struct task_motion *leader_motion = player_motion[0];
    struct task_motion *follower_motion = player_motion[1];
    struct player *leader_record = leader_motion->record;
    struct player *follower_record = follower_motion->record;
    NJS_VECTOR dir;
    Float dx;
    Float dz;
    Float distance;
    Float moved;
    Float stroke;
    Angle ang;
    Uint32 on;
    Uint32 press;

    if (leader == NULL) {
        return;
    }
    dir.x = follower->pos.x - leader->pos.x;
    dir.z = follower->pos.z - leader->pos.z;
    dir.y = 0.0F;
    njUnitVector(&dir);
    dir.x *= AIM_DISTANCE;
    dir.z *= AIM_DISTANCE;
    press = pad_records[1].press;
    on = pad_records[1].on;
    dx = leader->pos.x + dir.x - follower->pos.x;
    dz = leader->pos.z + dir.z - follower->pos.z;
    distance = _builtin_sqrtf((dx * dx) + (dz * dz));
    ang = angle_arc_tan2(dz, dx);
    stroke = 0.0F;
    switch (work->submode) {
    case STEER_START:
        if (leader_record != NULL && follower_record != NULL) {
            work->submode = STEER_RUN;
        }
        break;
    case STEER_RUN:
        if (distance < 15.0F) {
            if (leader_record->speed.x < leader_record->consts.trot_speed &&
                follower_record->speed.x >
                    follower_record->consts.running_speed) {
                npc_stick_against(&follower_motion->speed, &stroke, &ang);
            } else if (follower_record->speed.x == 0.0F) {
                if (distance < 0.99999999F) {
                    stroke = 0.0F;
                } else {
                    stroke = 0.005F;
                }
            } else {
                stroke = 0.0F;
            }
        } else if (distance < 40.0F) {
            stroke = ((distance - 20.0F) * 0.025F) + 0.5F;
        } else {
            stroke = 1.0F;
        }
        if (follower->flags & PLAYER_ON_GROUND) {
            /* @hack redundant: leader_record loaded again with the record it
             * holds (MOV.L @R4,R4 at 0x8c086eb8); without it, the function
             * runs 32 bytes short and 424 of 459 words differ */
            leader_record = leader_motion->record;
            if (leader->pos.y - follower->pos.y > 20.0F &&
                leader_record->speed.x < leader_record->consts.running_speed) {
                /* @hack spelling: this OR and the four below written out
                 * as x = x | y, which keeps on in R10 and pad_jump_buttons'
                 * address in R9 (0x8c086d34, 0x8c086d5c); as |=, the two
                 * trade and 20 of 459 words differ */
                press = press | pad_jump_buttons;
                work->submode = STEER_JUMP;
            }
        }
        break;
    case STEER_JUMP:
        stroke = distance * 0.05F;
        if (stroke > 1.0F) {
            stroke = 1.0F;
        } else if (stroke < 0.1F) {
            stroke = 0.0F;
        }
        if (follower->pos.y < leader->pos.y + 3.0F) {
            if (follower_motion->speed.y < 0.0F) {
                press = press | pad_jump_buttons;
                work->submode = STEER_FLY;
            } else {
                on = on | pad_jump_buttons;
            }
        }
        if (follower->flags & PLAYER_ON_GROUND) {
            if (!(follower->flags & PLAYER_BALL)) {
                work->submode = STEER_RUN;
            }
        }
        break;
    case STEER_FLY:
        stroke = distance * 0.05F;
        if (stroke > 1.0F) {
            stroke = 1.0F;
        } else if (stroke < 0.1F) {
            stroke = 0.0F;
        }
        if (follower->flags & PLAYER_ON_GROUND) {
            work->submode = STEER_RUN;
        } else if (distance < 20.0F && leader->pos.y < follower->pos.y) {
            press = press | pad_action_buttons;
        } else {
            on = on | pad_jump_buttons;
        }
        break;
    case STEER_DESCEND:
        stroke = distance * 0.05F;
        if (stroke > 1.0F) {
            stroke = 1.0F;
        } else if (stroke < 0.1F) {
            stroke = 0.0F;
        }
        if (follower->flags & PLAYER_ON_GROUND) {
            work->submode = STEER_RUN;
        } else if (follower->pos.y - leader->pos.y < DESCEND_LEVEL) {
            work->submode = STEER_FLY;
            press |= pad_jump_buttons;
            on |= pad_jump_buttons;
        }
        break;
    case STEER_CATCH_UP:
        /* @hack spelling: dir's members through &dir, here and in the sums
         * below, which forms dir's address afresh for each (MOV R15,R1 and
         * ADD #24,R1 at 0x8c087002, 0x8c087006); as dir.x, the function
         * runs 32 bytes short and 153 of 459 words differ */
        (&dir)->x = leader->pos.x - follower->pos.x;
        (&dir)->y = leader->pos.y - follower->pos.y;
        (&dir)->z = leader->pos.z - follower->pos.z;
        if (vector_magnitude(&dir) < 50.0F || (follower->flags & PLAYER_HURT)) {
            work->submode = STEER_RUN;
            on |= pad_action_buttons;
        } else {
            vector_unit(&dir, &dir);
            /* @hack spelling: the sums written out, which read dir.y and
             * dir.z at an offset in R0 (FMOV.S @(R0,R3),FR3 at 0x8c087070);
             * as +=, SHC reads them through @R1 after ADD R3,R1, and 68 of
             * 459 words differ */
            follower->pos.x = follower->pos.x + (&dir)->x;
            follower->pos.y = follower->pos.y + (&dir)->y;
            follower->pos.z = follower->pos.z + (&dir)->z;
        }
    }
    moved = point_distance(&work->pos, &follower->pos);
    if (moved < 0.99999999F) {
        NPC(work)->still++;
        if (NPC(work)->still > STUCK_STEPS) {
            if (distance > STUCK_DISTANCE) {
                NPC(work)->still = 0;
                press |= pad_jump_buttons;
                work->submode = STEER_JUMP;
            }
        }
    } else {
        NPC(work)->still = 0;
    }
    work->pos = follower->pos;
    work->ang = follower->ang;
    pad_records[1].on = on;
    pad_records[1].press = press;
    pad_sticks[1].ang = ang;
    pad_sticks[1].stroke = stroke;
}

/* NPC_LOST once Tails is over LOST_DISTANCE from player 0, or once he has
 * been out of view over UNSEEN_STEPS steps in a row. NPC_NOT_LOST
 * otherwise. the npc's unseen counts those steps. it goes back to 0 while
 * he is in view or nearer than NEAR_DISTANCE, and whenever this returns
 * NPC_LOST */
static enum npc_lost npc_watch(struct task *task, struct task_work *leader,
                               struct task_work *follower)
{
    NJS_POINT3 view;
    NJS_POINT3 screen;
    NJS_SCREEN *scr = &_nj_screen_;
    Float distance = point_distance(&leader->pos, &follower->pos);

    if (distance > LOST_DISTANCE) {
        NPC(task->work)->unseen = 0;
        return NPC_LOST;
    } else if (distance < NEAR_DISTANCE) {
        NPC(task->work)->unseen = 0;
        return NPC_NOT_LOST;
    }
    njCalcPoint(NULL, &follower->pos, &view);
    point_project_centered(&view, &screen);
#ifdef GCC
    /* x as port.h's window holds it, so that 16:9 widens the test */
    screen.x *= port_view_aspect;
#endif
    if (screen.z < -50.0F || screen.x < -(scr->w * 0.5F) ||
        screen.x > scr->w * 0.5F || screen.y < -(scr->h * 0.5F) ||
        screen.y > scr->h * 0.5F) {
        if (NPC(task->work)->unseen++ > UNSEEN_STEPS) {
            NPC(task->work)->unseen = 0;
            return NPC_LOST;
        }
        return NPC_NOT_LOST;
    }
    NPC(task->work)->unseen = 0;
    return NPC_NOT_LOST;
}

/* set pos to the point distance behind work, as it faces */
static void npc_place_behind(struct task_work *work, NJS_POINT3 *pos,
                             Float distance)
{
    Float sine;
    Float cosine;

    if (work != NULL && pos != NULL) {
        _builtin_fsca(work->ang.y, &sine, &cosine);
        pos->x = work->pos.x - (distance * cosine);
        pos->y = work->pos.y;
        pos->z = work->pos.z - (distance * sine);
    }
}

Angle npc_angle_to(NJS_POINT3 *from, NJS_POINT3 *to, Float *distance,
                   Float *across)
{
    Float dx = to->x - from->x;
    Float dy = to->y - from->y;
    Float dz = to->z - from->z;

    if (distance != NULL) {
        *distance = _builtin_sqrtf((dx * dx) + (dy * dy) + (dz * dz));
    }
    if (across != NULL) {
        *across = _builtin_sqrtf((dx * dx) + (dz * dz));
    }
    return angle_arc_tan2(dz, dx);
}

void npc_stick_against(NJS_VECTOR *speed, Float *stroke, Angle *ang)
{
    if (speed->x == 0.0F && speed->z == 0.0F) {
        if (stroke != NULL) {
            *stroke = 0.0F;
        }
        if (ang != NULL) {
            *ang = 0;
        }
    } else {
        if (stroke != NULL) {
            *stroke = 1.0F;
        }
        if (ang != NULL) {
            *ang = angle_arc_tan2(speed->z, speed->x) + NJM_DEG_ANG(180);
        }
    }
}

Bool npc_players_clash(struct task *task, struct task *other)
{
    /* @bug memory: both works are read before either task is tested, so a
     * null task reads address 0x20 (MOV.L @(32,R4),R14 and MOV.L
     * @(32,R5),R6 at 0x8c087384, 0x8c087388) */
    struct task_work *work = task->work;
    struct task_work *other_work = other->work;

    if (task != NULL && other != NULL) {
        if ((work->flags & CLASH_FLAGS) == CLASH_FLAGS) {
            if (work->collision->hit->owner == other &&
                (other_work->flags & PLAYER_GUARD)) {
                return TRUE;
            }
        } else if ((other_work->flags & CLASH_FLAGS) == CLASH_FLAGS &&
                   other_work->collision->hit->owner == task &&
                   (work->flags & PLAYER_GUARD)) {
            return TRUE;
        }
    }
    return FALSE;
}

/* decide whether Tails may follow Sonic. he never may while npc_disabled
 * is set, for another character, or while an event has control. otherwise
 * the stage and act decide, with npc_story_allows in some stages. where the
 * story lets him into Twinkle Park's first act, it returns TRUE and sets
 * npc_disabled, so every call after it returns FALSE */
static Bool npc_permit(void)
{
    Bool result;
    int stage_act;
    int act;
    int stage;

    if (npc_disabled || get_character() != CHARACTER_SONIC ||
        event_in_control() == TRUE) {
        return FALSE;
    }
    result = npc_story_allows();
    stage_act = stage_get_stage_act();
    act = (Uint8)stage_act;
    stage = (Uint8)(stage_act >> 8);
    switch (stage) {
    case STAGE_PRACTICE:
        result = FALSE;
        break;
    case STAGE_BEACH:
        /* @hack redundant: result set to the value it holds, which SHC
         * tests (TST R14,R14 at 0x8c0874dc); without it, 54 of 137 words
         * differ */
        if (result) {
            result = TRUE;
        } else {
            result = FALSE;
        }
        break;
    case STAGE_WINDY:
        result = TRUE;
        break;
    case STAGE_CASINO:
        if (act == 2 || act == 3) {
            result = FALSE;
        } else {
            result = TRUE;
        }
        break;
    case STAGE_ICECAP:
        if (act == 0 || act == 1) {
            result = TRUE;
        } else {
            result = FALSE;
        }
        break;
    case STAGE_TWINKLE:
        /* @hack redundant: TRUE and FALSE stored where result holds them
         * already, which gives the else a store of its own (MOV R5,R14 at
         * 0x8c087534); without those stores, the function runs 18 bytes
         * short and 78 of 137 words differ */
        if (result) {
            if (act == 0) {
                result = TRUE;
                npc_disable();
            } else if (act == 1) {
                result = TRUE;
            } else {
                result = FALSE;
            }
        } else {
            result = FALSE;
        }
        break;
    case STAGE_HIGHWAY:
        /* @hack redundant: TRUE and FALSE stored where result holds them
         * already, as in STAGE_TWINKLE's arm (MOV R5,R14 at 0x8c08754a);
         * without those stores, the function runs 18 bytes short and 77 of
         * 137 words differ */
        if (result) {
            if (act == 0 || act == 2) {
                result = TRUE;
            } else {
                result = FALSE;
            }
        } else {
            result = FALSE;
        }
        break;
    case STAGE_MOUNTAIN:
        /* @hack redundant: TRUE and FALSE stored where result holds them
         * already, as in STAGE_TWINKLE's arm (MOV R5,R14 at 0x8c087594);
         * without those stores, the function runs 4 bytes short and 71 of
         * 137 words differ */
        if (result) {
            if (act == 0 || act == 1) {
                result = TRUE;
            } else {
                result = FALSE;
            }
        } else {
            result = FALSE;
        }
        break;
    case STAGE_SKY_DECK:
        result = TRUE;
        break;
    case STAGE_RUIN:
        /* @hack redundant: TRUE and FALSE stored where result holds them
         * already, as in STAGE_TWINKLE's arm (MOV R5,R14 at 0x8c0875ae);
         * without those stores, the function runs 4 bytes short and 64 of
         * 137 words differ */
        if (result) {
            if (act == 0 || act == 1) {
                result = TRUE;
            } else {
                result = FALSE;
            }
        } else {
            result = FALSE;
        }
        break;
    case STAGE_FINAL:
        /* @hack redundant: result set to the value it holds, as in
         * STAGE_BEACH's arm (TST R14,R14 at 0x8c0875b0); without it, the
         * function runs 4 bytes short and 64 of 137 words differ */
        if (result) {
            result = TRUE;
        } else {
            result = FALSE;
        }
        break;
    /* @hack redundant: a case label nothing needs, which stretches the jump
     * table to 34 entries (MOV #34,R1 at 0x8c08747a); without it, the table
     * stops at 11, the function runs 64 bytes short and 95 of 137 words
     * differ */
    case STAGE_MYSTIC_RUINS:
    default:
        result = FALSE;
        break;
    }
    return result;
}

/* TRUE in a trial, and once Sonic's story has ended. TRUE too once event
 * EV0100 has played, until EV001A has, and once Sonic has cleared Emerald
 * Coast, until EV000D has played. FALSE otherwise */
static Bool npc_story_allows(void)
{
    if (menu_state.mode == MENU_MODE_TRIAL) {
        return TRUE;
    }
    if (event_flag_check(FLAG_CLEAR_SONIC)) {
        return TRUE;
    }
    if (event_was_played(0x100) && !event_was_played(0x1a)) {
        return TRUE;
    }
    if (event_flag_check(FLAG_SONIC_CLEAR_BEACH) && !event_was_played(0xd)) {
        return TRUE;
    }
    return FALSE;
}

void npc_set_disabled(Bool disabled)
{
    npc_disabled = disabled;
}

/* list's texture name for stage, or null if it has none */
static char *title_card_find(struct title_card_list *list, int stage)
{
    int i;
    struct title_card_name *name;

    name = list->names;
    for (i = 0; i < list->count; i++) {
        if (name->stage == stage) {
            return name->name;
        }
        name++;
    }
    return NULL;
}

Sint16 title_card_begin(int frames)
{
    struct title_card *card = &title_card;
    char *name;
    int index;
    Uint16 stage_act;
    Sint32 result;
    struct title_card_list *list;

    stream_destroy();
    draw_off = FALSE;
    screen_quad_init();
    frame_pacing_init(2, 2);
    screen_blend_alpha();
    njFogDisable();
    njSetBackColor(0xff000000, 0xff000000, 0xff000000);
    card->unk_00 = 2;
    card->frame = 0;
    if (frames == 0) {
        card->frames = 180;
    } else {
        card->frames = frames;
    }
    card->texlist = &title_card_texlist;
    stage_act = stage_get_stage_act();
    card->stage = STAGE_OF(stage_act);
    /* @hack spelling: other_list's address through list, which SHC loads
     * once before the first test (MOV.L @(0x8c087734),R4 at 0x8c0876d2);
     * as &other_list at each use, the function runs 2 bytes long and 49 of
     * 139 words differ */
    list = &other_list;
    if (card->stage == STAGE_PRACTICE) {
        name = title_card_find(list, card->stage);
        card->kind = TITLE_CARD_OTHER;
    } else if (card->stage > STAGE_PRACTICE && card->stage <= STAGE_SHELTER) {
        index = (int)story_list_of[get_character()];
        if (index == -1) {
            return -1;
        }
        name = title_card_find(&story_lists[index], card->stage);
        card->kind = TITLE_CARD_STAGE;
    } else if (card->stage >= STAGE_STATION_SQUARE &&
               card->stage <= STAGE_PAST) {
        name = title_card_find(&field_list, card->stage);
        card->kind = TITLE_CARD_FIELD;
    } else if (card->stage >= STAGE_CART && card->stage <= STAGE_SUNABOARD) {
        name = title_card_find(list, card->stage);
        card->kind = TITLE_CARD_OTHER;
    } else {
        return -1;
    }
    if (name == NULL) {
        return -1;
    }
    card->texlist->textures->filename = name;
    result = njLoadTexture(card->texlist);
    if (result == -1) {
        return -1;
    }
    return 0;
}

Sint16 title_card_step(void)
{
    struct title_card *card = &title_card;
    Bool done = FALSE;

    njSetTexture(card->texlist);
    switch (card->kind) {
    case TITLE_CARD_STAGE:
        screen_quad_draw(QUAD_CENTERED, SCREEN_MIDDLE_X + 32.0F,
                         SCREEN_MIDDLE_Y, 1000.0F);
        break;
    case TITLE_CARD_FIELD:
        screen_quad_draw(QUAD_CENTERED, SCREEN_MIDDLE_X, SCREEN_MIDDLE_Y,
                         1000.0F);
        break;
    case TITLE_CARD_OTHER:
        screen_quad_draw(QUAD_CENTERED, SCREEN_MIDDLE_X + 8.0F, SCREEN_MIDDLE_Y,
                         1000.0F);
        break;
    }
    if (card->frame++ > card->frames && card_saving == 0) {
        njReleaseTexture(card->texlist);
        draw_off = TRUE;
        done = TRUE;
    }
    return done;
}
