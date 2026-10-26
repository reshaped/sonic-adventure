/* the Perfect Chaos fight: its places, its runs, the boss's attacks and the
 * screen's fades */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "b_chaos7/boss.h"
#include "b_chaos7/camera.h"
#include "b_chaos7/chaos7.h"
#include "b_chaos7/damage.h"
#include "b_chaos7/death.h"
#include "b_chaos7/fight.h"
#include "b_chaos7/hint.h"
#include "camera/camera.h"
#include "chaos/chaos.h"
#include "draw/light.h"
#include "draw/motion.h"
#include "hud/boss_name.h"
#include "hud/hud.h"
#include "hud/life_gauge.h"
#include "main.h"
#include "player/player.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 13
#include "b_chaos7/fight/text.inc"

#define FIGHT_PERSPECTIVE NJM_DEG_ANG(100) /* the fight's field of view */

#define SE_RUN_START 0x1c0
#define SE_RUN_FALL 0x1c3

/* player 0 starts the fight with START_RINGS rings. while the fight is on, he
 * loses one each DRAIN_STEPS steps */
#define START_RINGS 50
#define DRAIN_STEPS 60

/* the radius round the boss, across the ground, that keep_inside holds
 * player 0 within */
#define PLACE_REACH 900.0F

/* the boss attacks only from BOSS_READY, on steps when player 0 is within
 * ATTACK_ARC of his heading. each such step counts his attack_timer down. he
 * attacks on the step it falls to 0 or below. an attack sets the timer to
 * ATTACK_WAIT in the first part, and to 0 in the second. there a knock-back
 * raises a timer below 30 to 60. his attack depends on whether player 0 is
 * nearer than ATTACK_NEAR or ATTACK_FAR. a near attack can be tornadoes only
 * while the boss's health is below TORNADO_HEALTH */
#define ATTACK_ARC NJM_DEG_ANG(30)
#define ATTACK_WAIT 60
#define ATTACK_NEAR 3000.0F
#define ATTACK_FAR 4000.0F
#define TORNADO_HEALTH 4.0F

/* a run that carries player 0 pulls him RUN_PULL a step toward the boss's
 * head, node RUN_NODE. it hits once he is within RUN_HIT of the head, and takes
 * RUN_DAMAGE of the boss's health. a run that falls short drops him into the
 * sea. the fight moves on once he is below RUN_SEA */
#define RUN_DAMAGE 2.0F
#define RUN_NODE 21
#define RUN_PULL 15.0F
#define RUN_HIT 50.0F
#define RUN_SEA 20.0F

/* in FIGHT_OPENING_SHOT, the boss takes BOSS_MODE_13 on the step that finds
 * the wait at MODE_13_WAIT */
#define MODE_13_WAIT 120
/* the fight waits on a shot until the shot ends or the wait passes
 * SHOT_STEPS. it then counts the wait again, and gives player 0 his pad once
 * the wait passes PAD_WAIT */
#define SHOT_STEPS 900
#define PAD_WAIT 10
/* the second part holds its shot until the steps since the fade in from black
 * pass BLACK_STEPS. the boss then forms, and the shot plays in
 * FIGHT_OPENING_SHOT */
#define BLACK_STEPS 180

/* the fight's fades last FADE_STEPS. the fight moves on once a fade has
 * FADE_LEFT steps or fewer left */
#define FADE_STEPS 120
#define FADE_LEFT 1
/* the grays and alphas the fight's fades take */
#define BLACK 0
#define WHITE 255
#define CLEAR 0
#define OPAQUE 255
/* njDrawPolygon2D's attributes for the fade's quad: see-through */
#define SEE_THROUGH (NJD_DRAW_CONNECTED | NJD_FILL | NJD_TRANSPARENT)

/* evaluates x twice */
#define ABS(x) ((x) < 0 ? -(x) : (x))

struct fight_vars {
    Uint8 unk_00[4];
    /* black's low half is wait, and its high half drain */
    union {
        struct {
            Sint16 wait; /* steps through a shot's wait, then the pad's */
            Sint16 drain; /* steps toward the next ring drained */
        } steps;
        Sint32 black; /* steps since the fade in from black began */
    } clock;
    Uint8 unk_08[4];
};

#define VARS(work) ((struct fight_vars *)&(work)->vars)

enum fight_mode {
    FIGHT_START = 0,
    FIGHT_OPENING = 1,
    FIGHT_PLACE = 2,
    FIGHT_PLACE_SHOT = 3,
    FIGHT_OPENING_SHOT = 4,
    FIGHT_SHOT = 5,
    FIGHT_PAD_WAIT = 6,
    FIGHT_ON = 7,
    FIGHT_RUN = 8,
    FIGHT_PULL = 9,
    FIGHT_FALL = 10,
    FIGHT_MISSED = 11,
    FIGHT_HIT = 12,
    FIGHT_HIT_SHOT = 13,
    FIGHT_HIT_END = 14,
    FIGHT_WON = 16,
    FIGHT_WHITE = 17,
    FIGHT_CLEAR = 18,
    FIGHT_BLACK = 20,
    FIGHT_BLACK_HOLD = 21,
    FIGHT_PART_2 = 22
};

#pragma section HEAD
/* a run that starts at fight_launch_speed or faster carries player 0 to the
 * boss */
static Float fight_launch_speed = 15.0F;
/* how far the boss camera's target is above what it follows */
static Float chaos7_camera_rise = 400.0F;
Float chaos7_camera_height = 15.0F;
Float chaos7_camera_back = 30.0F;
#pragma section

/* the boss camera's offsets, as camera_set_boss_offsets takes them */
static NJS_POINT3 camera_target_offset = {0.0F, 400.0F, 0.0F};
static NJS_POINT3 camera_offset = {0.0F, 15.0F, 30.0F};
/* nothing reads these two */
static struct angles dat_0ca5ecf0 = {NJM_DEG_ANG(1), NJM_DEG_ANG(1),
                                     NJM_DEG_ANG(1)};
static struct angles dat_0ca5ecfc = {NJM_DEG_ANG(4), NJM_DEG_ANG(4),
                                     NJM_DEG_ANG(4)};
/* the most the boss camera moves a step, as camera_set_boss_limits takes it */
static NJS_VECTOR camera_move_limit = {30.0F, 20.0F, 0.0F};
/* the fight's three places. at each, boss_places is where the boss stands,
 * player_places where player 0 starts, and boss_headings the boss's heading */
static NJS_POINT3 boss_places[3] = {
    {94.51F, 0.0F, -9194.29F},
    {4714.13F, 0.0F, -3487.91F},
    {49.12F, 0.0F, 0.85F},
};
static NJS_POINT3 player_places[3] = {
    {-16.0F, 444.0F, -667.0F},
    {837.0F, 444.0F, -8733.0F},
    {4786.0F, 437.0F, -3427.0F},
};
static Angle boss_headings[3] = {NJM_DEG_ANG(90), -NJM_DEG_ANG(90), 0};
/* clang-format off */
/* the shot that shows each place */
static struct chaos7_shot place_shots[3] = {
#include "b_chaos7/fight/place_shots.inc"
};
static struct chaos7_shot opening_shot = {
    {-244.0F, 435.0F, -8213.0F}, {289.0F, 585.0F, -8363.0F},
    205.0F, 350.0F, 480.0F,
};
/* the two shots of a hit at each place */
static struct chaos7_shot hit_shots[6] = {
#include "b_chaos7/fight/hit_shots.inc"
};
/* clang-format on */
/* where the camera watches a run from, at each place */
static NJS_POINT3 run_views[3] = {
    {405.0F, 30.0F, -8828.0F},
    {5005.0F, 30.0F, -3650.0F},
    {330.0F, 480.0F, 250.0F},
};
static struct task *fight_task = NULL;
/* the boss attacks while boss_attacks is set. pad 1's debug controls toggle
 * it */
static Sint8 boss_attacks = 1;
static Sint16 fade_steps_left = 0;
static Sint16 fade_delay = 0;
/* player 0's speed as a run that falls short drops him, at each place */
static NJS_VECTOR fall_speeds[3] = {
    {0.0F, 0.0F, 7.0F},
    {0.0F, 0.0F, -7.0F},
    {7.0F, 0.0F, 0.0F},
};

Sint8 chaos7_part_2;
Sint8 chaos7_won;
/* the place the fight is at, 0 to 2 */
static Sint8 chaos7_place;
/* set once player 0 has come within PLACE_REACH of the boss */
static Sint8 fight_inside;
static struct task *chaos7_boss;
/* player 0's speed as a run took him */
static Float fight_run_speed;
/* the heading the boss's place gives him, which the boss reads. SHC lays out
 * this file's bss in the order the names are first declared. declared in
 * fight.h, chaos7_boss_heading would come ahead of chaos7_place, so fight.h
 * leaves it out */
Angle chaos7_boss_heading; /* NOLINT(misc-use-internal-linkage) */
static Uint8 fade_gray;
static Float fade_alpha;
static Float fade_rate;
static struct task *death_effects[3];

static Float distance_ahead(void);
static void choose_attack(void);
static void drain_rings(struct task_work *work);
static void keep_inside(void);
static void fight_destroy(struct task *task);
static void fight_fade_draw(void);
static void place_boss(void);
static void place_player(void);
static void load_part_lights(int part);

/* the origin in the head's node. SHC lays out the file's constants in the
 * order it meets them. it meets the boss's name, TEXT_1, where
 * chaos7_fight_step uses it. head_origin is defined at the file's end, after
 * that use, so that it follows the name */
static const NJS_POINT3 head_origin;

/* player 0's distance from the boss across the ground. -1 unless the boss is
 * in BOSS_READY and faces player 0 within ATTACK_ARC */
static Float distance_ahead(void)
{
    struct task_work *boss = chaos7_boss->work;
    struct task_work *player = player_work[0];
    NJS_VECTOR to;
    Angle ang;
    /* @hack redundant: register keeps heading in R14 and ang in R13 (MOV.L
     * @R3,R14 at 0x0c916f2e); without it, the two trade, and 27 of 83 words
     * differ */
    register Angle heading;

    if (boss->mode != BOSS_READY) {
        return -1.0F;
    }
    to = player->pos;
    njSubVector(&to, &boss->pos);
    ang = (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN);
    heading = chaos7_boss_heading;
    if (ABS(angle_difference(ang, heading)) > ATTACK_ARC) {
        return -1.0F;
    }
    return njSqrt((to.x * to.x) + (to.z * to.z));
}

/* once the boss has faced player 0 for his wait, pick his attack by player
 * 0's distance and the boss's health */
static void choose_attack(void)
{
    /* @hack spelling: chaos7_boss reached through its address, which SHC loads
     * into R4 (MOV.L @(0x0c917064),R4 at 0x0c916fe2); without it, the load
     * takes R3, and 2 of 82 words differ */
    struct task **boss = &chaos7_boss;
    struct chaos *chaos = (*boss)->extra;
    Float distance = distance_ahead();
    Float chance;
    int attack;

    if (distance < 0.0F) {
        return;
    }
    if (--chaos->attack_timer > 0) {
        return;
    }
    chaos->attack_timer = chaos7_part_2 ? 0 : ATTACK_WAIT;
    chance = (Float)rand() * RAND_SCALE;
    if (distance < ATTACK_NEAR) {
        if (chaos->health < TORNADO_HEALTH) {
            attack = chance >= 0.6F ? BOSS_RAY : BOSS_TORNADOES;
        } else {
            attack = BOSS_RAY;
            /* @hack redundant: a goto to the call, which lets this branch share
             * the far BOSS_RAY's BRA at 0x0c917092 (BF 0x0c917092 at
             * 0x0c917038); without it, SHC writes a BRA of its own, the
             * function runs 4 bytes long, and 35 of 82 words differ */
            goto chosen;
        }
    } else if (distance < ATTACK_FAR) {
        if (chance >= 0.8F) {
            attack = BOSS_RAY;
        } else if (chance >= 0.4F) {
            attack = BOSS_TORNADOES;
        } else {
            attack = BOSS_BREATH;
        }
    } else {
        attack = BOSS_BREATH;
    }
chosen:
    chaos7_set_mode(attack);
}

int fun_0c9170ba(void)
{
    return 0;
}

/* drain a ring each DRAIN_STEPS steps. with none left, end player 0's super
 * form */
static void drain_rings(struct task_work *work)
{
    /* @hack type: the cast to Sint16, which changes no value ring_count
     * returns, since rings is a Sint16, but keeps the EXTS.W R0,R0 at
     * 0x0c9170c8; without it, 30 of 34 words differ */
    if ((Sint16)ring_count() > 0) {
        VARS(work)->clock.steps.drain++;
        if (VARS(work)->clock.steps.drain >= DRAIN_STEPS) {
            VARS(work)->clock.steps.drain = 0;
            ring_add(-1);
        }
    } else {
        ring_reset();
        player_set_mode(0, PLAYER_MODE_SUPER_END);
    }
}

/* set fight_inside once player 0 comes within PLACE_REACH of the boss. from
 * then on, put him back on that circle each step he is outside it */
static void keep_inside(void)
{
    struct task_work *player = player_work[0];
    Float distance = chaos_distance_across(&boss_work->pos, &player->pos);
    Angle ang;

    if (fight_inside) {
        if (distance > PLACE_REACH) {
            ang = (Angle)(atan2f(player->pos.z - boss_work->pos.z,
                                 player->pos.x - boss_work->pos.x) *
                          ANGLE_PER_RADIAN);
            player->pos.x = boss_work->pos.x + (njCos(ang) * PLACE_REACH);
            player->pos.z = boss_work->pos.z + (njSin(ang) * PLACE_REACH);
        }
    } else if (distance < PLACE_REACH) {
        fight_inside = 1;
    }
}

static void fight_destroy(struct task *task)
{
    task_kill(chaos7_boss);
    fight_task = NULL;
}

/* while a fade runs, cover the screen in its gray at its alpha */
static void fight_fade_draw(void)
{
    NJS_POINT2COL quad;
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];
    NJS_COLOR color;

    if (frame_step) {
        return;
    }
    if (!fade_steps_left) {
        return;
    }
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    quad.p = points;
    quad.col = colors;
    quad.tex = NULL;
    quad.num = 4;
    color.argb.b = color.argb.g = color.argb.r = fade_gray;
    color.argb.a = (Uint8)fade_alpha;
    quad.p[0].x = 0.0F;
    quad.p[0].y = 0.0F;
    quad.col[0] = color;
    quad.p[1].x = SCREEN_WIDTH;
    quad.p[1].y = 0.0F;
    quad.col[1] = color;
    quad.p[2].x = SCREEN_WIDTH;
    quad.p[2].y = SCREEN_HEIGHT;
    quad.col[2] = color;
    quad.p[3].x = 0.0F;
    quad.p[3].y = SCREEN_HEIGHT;
    quad.col[3] = color;
    njDrawPolygon2D(&quad, 4, -0.1F, SEE_THROUGH);
}

void chaos7_fade_start(int from, int to, int gray, int steps, int delay)
{
    if (fade_delay > 0) {
        return;
    }
    fade_gray = gray;
    fade_alpha = (Float)from;
    fade_rate = (Float)(to - from) / (Float)steps;
    fade_steps_left = steps;
    fade_delay = delay;
}

void chaos7_fight_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 head;
    NJS_POINT3 origin;
    NJS_VECTOR pull;
    Sint8 place;
    struct task_work *player = player_work[0];
    struct chaos *chaos;
    Sint8 shot_done;
    int fading;
    Float distance;
    Float scale;

    /* chaos7_boss is made in FIGHT_START, so the step reads its extra only in
     * later modes */
    /* @hack redundant: the mask, which changes nothing in the test, but loads
     * the boss's task into R0 and its extra into R1 (MOV.L @R2,R0 at
     * 0x0c917386); without it, the two trade, and 3 of 799 words differ */
    if (work->mode & 0xff) {
        chaos = chaos7_boss->extra;
    }
    camera_set_perspective(FIGHT_PERSPECTIVE);
    /* @hack spelling: the three copies in the switch's expression, which load
     * chaos7_shot_done and fade_steps_left ahead of the mode (MOV.B @R3,R5 at
     * 0x0c9173a2); without it, as statements above the switch, they load
     * after the mode, and 10 of 799 words and 6 pool words differ */
    switch (place = chaos7_place, shot_done = chaos7_shot_done,
            fading = fade_steps_left, work->mode) {
    case FIGHT_START:
        /* fight_fade_draw takes no task, since the step's own call passes
         * none. as the draw, it leaves its argument unread */
        /* NOLINTNEXTLINE(clang-diagnostic-cast-function-type-*) */
        task->draw = (void (*)(struct task *))fight_fade_draw;
        task->destructor = fight_destroy;
        fight_task = task;
        boss_attacks = 1;
        chaos7_mist_on = 1;
        chaos7_won = 0;
        chaos7_boss =
            task_create_child(TASK_WORK | TASK_MOTION, chaos7_boss_step, task);
        chaos7_place = 0;
        stage_place_player(player);
        place_boss();
        camera_target_offset.y = chaos7_camera_rise;
        camera_offset.y = chaos7_camera_height;
        camera_offset.z = chaos7_camera_back;
        camera_set_boss_offsets(&camera_target_offset, &camera_offset);
        camera_set_boss_limits(NULL, NULL, &camera_move_limit);
        camera_set_perspective(FIGHT_PERSPECTIVE);
        chaos7_part_2 = 0;
        ring_reset();
        ring_add(START_RINGS);
        work->mode = FIGHT_OPENING;
        break;
    case FIGHT_OPENING:
        boss_name_create(TEXT_1, -1, 240, 120);
        time_stop();
        pad_disable_player(0);
        chaos7_shot_start(&opening_shot);
        work->mode = FIGHT_OPENING_SHOT;
        VARS(work)->clock.steps.wait = 0;
        VARS(work)->clock.steps.drain = 0;
        task_create_child(TASK_WORK, chaos7_hint_step, task);
        fight_inside = 0;
        break;
    case FIGHT_PLACE:
        time_stop();
        pad_disable_player(0);
        place_boss();
        chaos7_shot_start(&place_shots[chaos7_place]);
        work->mode = FIGHT_PLACE_SHOT;
        fight_inside = 0;
        VARS(work)->clock.steps.wait = 0;
        break;
    case FIGHT_PLACE_SHOT:
        chaos7_set_mode(BOSS_FORM);
        work->mode = FIGHT_SHOT;
        break;
    case FIGHT_OPENING_SHOT:
        if (VARS(work)->clock.steps.wait == MODE_13_WAIT) {
            chaos7_set_mode(BOSS_MODE_13);
        }
        /* fall through */
    case FIGHT_SHOT:
        VARS(work)->clock.steps.wait++;
        if (chaos7_shot_done || VARS(work)->clock.steps.wait > SHOT_STEPS) {
            place_player();
            camera_restore_high();
            camera_set_mode(CAMERA_MODE_CHAOS_W, CAMERA_ADJUST_RELATIVE3);
            time_start();
            if (work->mode == FIGHT_OPENING_SHOT) {
                life_gauge_create_child(0, 0, BOSS_HEALTH, task);
            }
            VARS(work)->clock.steps.wait = 0;
            work->mode = FIGHT_PAD_WAIT;
        }
        break;
    case FIGHT_PAD_WAIT:
        VARS(work)->clock.steps.wait++;
        if (VARS(work)->clock.steps.wait > PAD_WAIT) {
            pad_enable_player(0);
            work->mode = FIGHT_ON;
        }
        break;
    case FIGHT_ON:
        /* with debug_unlocked, pad 1's x held as y is pressed toggles the
         * boss's attacks. x held as b is pressed toggles chaos7_part_2, and
         * loads that part's lights */
        if (debug_unlocked) {
            if ((pad_records[1].on & PDD_DGT_TX) &&
                (pad_records[1].press & PDD_DGT_TY)) {
                boss_attacks ^= 1;
            }
        }
        if (debug_unlocked) {
            if ((pad_records[1].on & PDD_DGT_TX) &&
                (pad_records[1].press & PDD_DGT_TB)) {
                chaos7_part_2 ^= 1;
                load_part_lights(chaos7_part_2);
            }
        }
        if (boss_attacks) {
            choose_attack();
        }
        keep_inside();
        if (chaos->health > 0.0F) {
            drain_rings(work);
        }
        break;
    case FIGHT_RUN:
        fight_inside = 0;
        vibration_start(0, 2, 15, 0);
        break;
    case FIGHT_PULL:
        vibration_start(0, 2, 15, 0);
        origin = head_origin;
        njPushMatrix(&chaos7_node_matrices[RUN_NODE]);
        njCalcPoint(NULL, &origin, &head);
        njPopMatrix(1);
        pull = head;
        njSubVector(&pull, &player->pos);
        distance = njScalor(&pull);
        scale = RUN_PULL / distance;
        /* @hack redundant: a never-run pair of tests of scale, which leave no
         * code, but settle a register tie in the debug controls as the image
         * has it: debug_unlocked's address in R6 (0x0c91765e); without it, R6
         * and R7 trade and 7 of 799 words differ */
        if (0) {
            if (scale) {
            }
            if (scale) {
            }
        }
        pull.x *= scale;
        pull.y *= scale;
        pull.z *= scale;
        njAddVector(&player->pos, &pull);
        if (distance < RUN_HIT) {
            player_set_mode(0, PLAYER_MODE_RESET);
            work->mode = FIGHT_HIT;
        }
        break;
    case FIGHT_FALL:
        if (player->pos.y < RUN_SEA) {
            chaos7_shot_start(&place_shots[place]);
            chaos7_set_mode(BOSS_MELT);
            time_stop();
            pad_disable_player(0);
            work->mode = FIGHT_MISSED;
        }
        break;
    case FIGHT_MISSED:
        if (chaos7_boss->work->mode == BOSS_MELTED) {
            chaos7_place++;
            if (chaos7_place > 2) {
                chaos7_place = 0;
            }
            work->mode = FIGHT_PLACE;
        }
        break;
    case FIGHT_HIT:
        chaos->health -= RUN_DAMAGE;
        chaos7_set_mode(BOSS_HIT);
        chaos7_shot_start(&hit_shots[chaos7_place * 2]);
        death_effects[0] = death_effects[1] = death_effects[2] = NULL;
        time_stop();
        pad_disable_player(0);
        work->mode = FIGHT_HIT_SHOT;
        break;
    case FIGHT_HIT_SHOT:
        if (shot_done) {
            chaos7_shot_start(&hit_shots[(place * 2) + 1]);
            chaos7_set_mode(BOSS_HIT);
            work->mode = FIGHT_HIT_END;
            if (chaos->health <= 0.0F) {
                if (chaos7_part_2) {
                    death_effects[0] = task_create_child(
                        TASK_WORK, chaos7_death_1_start, task);
                    death_effects[0]->work->pos = boss_work->pos;
                    death_effects[1] = task_create_child(
                        TASK_WORK, chaos7_death_2_start, task);
                    death_effects[1]->work->pos = boss_work->pos;
                    death_effects[2] = task_create_child(
                        TASK_WORK, chaos7_death_3_start, task);
                    death_effects[2]->work->pos = boss_work->pos;
                }
            }
        }
        break;
    case FIGHT_HIT_END:
        if (shot_done) {
            if (death_effects[0]) {
                task_kill(death_effects[0]);
            }
            if (death_effects[1]) {
                task_kill(death_effects[1]);
            }
            if (chaos->health > 0.0F) {
                chaos7_set_mode(BOSS_FORM);
                chaos7_place++;
                if (chaos7_place > 2) {
                    chaos7_place = 0;
                }
                work->mode = FIGHT_PLACE;
            } else if (chaos7_part_2) {
                work->mode = FIGHT_WON;
            } else {
                work->mode = FIGHT_BLACK;
                VARS(work)->clock.black = 0;
                chaos7_fade_start(CLEAR, OPAQUE, BLACK, FADE_STEPS, 0);
            }
        }
        break;
    case FIGHT_WON:
        chaos7_fade_start(CLEAR, OPAQUE, WHITE, FADE_STEPS, 0);
        work->mode = FIGHT_WHITE;
        break;
    case FIGHT_WHITE:
        if (fading <= FADE_LEFT) {
            chaos7_part_2_end();
            camera_restore_high();
            chaos7_fade_start(OPAQUE, CLEAR, WHITE, FADE_STEPS, 0);
            work->mode = FIGHT_CLEAR;
        }
        break;
    case FIGHT_CLEAR:
        if (fading <= FADE_LEFT) {
            stage_clear();
            chaos7_won = 1;
            task_kill(task);
            return;
        }
        break;
    case FIGHT_BLACK:
        if (fading <= FADE_LEFT) {
            chaos7_part_2 = 1;
            chaos7_place++;
            if (chaos7_place > 2) {
                chaos7_place = 0;
            }
            place_boss();
            place_player();
            chaos7_part_2_start();
            chaos7_shot_start(&place_shots[chaos7_place]);
            chaos7_shot_held = 1;
            chaos7_fade_start(OPAQUE, OPAQUE, BLACK, FADE_STEPS, 0);
            work->mode = FIGHT_BLACK_HOLD;
        }
        break;
    case FIGHT_BLACK_HOLD:
        place_player();
        if (fade_steps_left <= FADE_LEFT) {
            chaos7_fade_start(OPAQUE, CLEAR, BLACK, FADE_STEPS, 0);
            work->mode = FIGHT_PART_2;
        }
        break;
    case FIGHT_PART_2:
        place_player();
        VARS(work)->clock.black++;
        if (VARS(work)->clock.black > BLACK_STEPS) {
            chaos->health = BOSS_HEALTH;
            life_gauge_create_child(0, 0, BOSS_HEALTH, task);
            chaos7_set_mode(BOSS_FORM);
            chaos7_shot_held = 0;
            time_stop();
            pad_disable_player(0);
            fight_inside = 0;
            /* @bug logic: the boss never takes BOSS_MODE_13 in the second
             * part's shot, and the shot's wait passes SHOT_STEPS 181 steps
             * sooner. this moves to FIGHT_OPENING_SHOT without clearing
             * clock.steps.wait, as FIGHT_OPENING does. wait is black's low
             * half, which this leaves at BLACK_STEPS + 1, past MODE_13_WAIT
             * (black's store at 0x0c917b8e, the mode's at 0x0c917bc4) */
            work->mode = FIGHT_OPENING_SHOT;
        }
        break;
    }
    task_step_children(task);
    if (fade_delay > 0) {
        fade_delay--;
    }
    if (fade_steps_left > 0) {
        fade_steps_left--;
        fade_alpha += fade_rate;
        fight_fade_draw();
    }
}

static void place_boss(void)
{
    chaos7_boss->work->pos = boss_places[chaos7_place];
    chaos7_boss_heading = chaos7_boss->work->ang.y =
        boss_headings[chaos7_place];
}

static void place_player(void)
{
    struct task_work *player = player_work[0];

    player->pos = player_places[chaos7_place];
}

void chaos7_place_boss(struct task *task, int place)
{
    task->work->pos = boss_places[place];
    chaos7_boss_heading = task->work->ang.y = boss_headings[place];
}

/* watch player 0 from the place's run view, take his pad, and put the boss in
 * BOSS_RUN */
void chaos7_run_start(void)
{
    struct player *record = player_motion[0]->record;
    struct task_work *work;
    NJS_POINT3 *view;

    if (fight_task == NULL) {
        return;
    }
    work = fight_task->work;
    camera_override_high(chaos7_camera_watch_step, 1, CAMERA_LOOK_AT);
    view = &run_views[chaos7_place];
    camera_override_view.pos.x = view->x;
    camera_override_view.pos.y = view->y;
    camera_override_view.pos.z = view->z;
    pad_disable_player(0);
    chaos7_set_mode(BOSS_RUN);
    fight_run_speed = record->speed.x;
    sound_play_held(SE_RUN, (int)work, 1, chaos7_volume);
    sound_play(SE_RUN_START, 0, 0, 127);
    work->mode = FIGHT_RUN;
}

/* when fight_run_speed is fight_launch_speed or more, curl player 0 up and
 * pull him to the head. otherwise drop him into the sea with a ripple */
void chaos7_run_end(void)
{
    struct task_work *work;
    struct task_work *player;
    struct task *ripple;

    if (fight_task == NULL) {
        return;
    }
    work = fight_task->work;
    sound_stop(SE_RUN);
    if (work->mode != FIGHT_RUN) {
        return;
    }
    if (fight_run_speed >= fight_launch_speed) {
        player_set_mode(0, PLAYER_MODE_BALL);
        work->mode = FIGHT_PULL;
    } else {
        player = player_work[0];
        work->mode = FIGHT_FALL;
        ripple = task_create_child(TASK_WORK, chaos7_damage_ripple_start,
                                   fight_task);
        if (ripple) {
            ripple->work->pos = player->pos;
        }
        player->ang.y = boss_work->ang.y;
        player_set_speed(0, fall_speeds[chaos7_place].x, 0.0F,
                         fall_speeds[chaos7_place].z);
        sound_play(SE_RUN_FALL, 0, 0, chaos7_volume);
    }
}

Bool chaos7_run_open(int place)
{
    if (fight_task == NULL) {
        return FALSE;
    }
    if (place != chaos7_place) {
        return FALSE;
    }
    if (fight_task->work->mode != FIGHT_ON &&
        fight_task->work->mode != FIGHT_RUN) {
        return FALSE;
    }
    if (chaos7_boss->work->mode == BOSS_FORM ||
        chaos7_boss->work->mode == BOSS_MELT ||
        chaos7_boss->work->mode == BOSS_MELTED) {
        return FALSE;
    }
    return TRUE;
}

/* read the lights of the fight's first part, 0, or its second, 1 */
static void load_part_lights(int part)
{
    stream_destroy();
    switch (part) {
    case 0:
        light_read_disc_sources(STAGE_CHAOS7, 0);
        light_read_disc_palettes(STAGE_CHAOS7, 0);
        break;
    case 1:
        light_read_disc_sources(STAGE_CHAOS7, 1);
        light_read_disc_palettes(STAGE_CHAOS7, 1);
        break;
    }
}

static const NJS_POINT3 head_origin = {0.0F, 0.0F, 0.0F};
