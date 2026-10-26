/* Perfect Chaos, the boss: his modes, his step and his draw */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos7/boss.h"
#include "b_chaos7/breath.h"
#include "b_chaos7/chaos7_textures.h"
#include "b_chaos7/damage.h"
#include "b_chaos7/fight.h"
#include "b_chaos7/ray.h"
#include "b_chaos7/tornado.h"
#include "chaos/chaos.h"
#include "chaos/effect.h"
#include "chaos/steer.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/motion.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "event/play.h"
#include "hud/life_gauge.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* the heading of the place he stands at, which fight.c defines. fight.h
 * doesn't declare it, since SHC lays out bss by each name's first
 * declaration. a declaration there would move it ahead of the bss fight.c
 * declares before it */
extern Angle chaos7_boss_heading;
/* his body's tree, which his actions move, and its welds */
extern NJS_OBJECT chaos7_body_object;
extern struct weld chaos7_welds[];
/* a tree of four nodes, which boss_draw draws with his body while
 * FIGURE_BODY is set: its root, then its other three nodes */
extern NJS_OBJECT dat_0c9e49d0;
extern NJS_OBJECT dat_0c9e3d2c;
extern NJS_OBJECT dat_0c9e2d88;
extern NJS_OBJECT dat_0c9e243c;
/* nodes of his body's tree, which hidden_nodes and fading_nodes list */
extern NJS_OBJECT dat_0c9c9778;
extern NJS_OBJECT dat_0c9c9a18;
extern NJS_OBJECT dat_0c9c9088;
extern NJS_OBJECT dat_0c9c9328;
extern NJS_OBJECT dat_0c9d32c8;
extern NJS_OBJECT dat_0c9d1004;
extern NJS_OBJECT dat_0c9ce914;
extern NJS_OBJECT dat_0c9cae64;
extern NJS_OBJECT dat_0c9c9cec;
extern NJS_OBJECT dat_0c9cd2d0;
extern NJS_OBJECT dat_0c9cd970;
/* his forming figure, which BOSS_FORM and BOSS_MELT play, and its motion.
 * chaos7_textures.h declares its shape motion */
extern NJS_OBJECT form_object;
extern NJS_MOTION form_motion;
/* the actions chaos7_actions plays, in its order */
extern NJS_ACTION dat_0ca327d4;
extern NJS_ACTION dat_0ca3c678;
extern NJS_ACTION dat_0ca24d44;
extern NJS_ACTION dat_0ca19b20;
extern NJS_ACTION dat_0ca275a8;
extern NJS_ACTION dat_0ca2843c;
extern NJS_ACTION dat_0ca2ad30;
extern NJS_ACTION dat_0ca480ec;
extern NJS_ACTION dat_0c9f0160;
extern NJS_ACTION dat_0ca00e74;
extern NJS_ACTION dat_0ca03d18;

/* as his breath starts, and as BOSS_RAY's flash starts */
#define SE_BEAM_START 0x1ae
#define SE_BREAKUP_A 0x1bd
#define SE_REACTION 0x1be /* in BOSS_MODE_13 and BOSS_MODE_14 */
/* as ACTION_RUN shows in BOSS_RUN, and as BOSS_HIT starts */
#define SE_RUN_ACTION 0x1bf
#define SE_BREAKUP_B 0x1c2
#define SE_FORM 0x1c4 /* held while the forming figure plays */
/* SE_NEAR plays while player 0 is within NEAR_RANGE of him, in x and z */
#define SE_NEAR 0x403
#define NEAR_RANGE 400.0F
/* the frames of ACTION_MODE_13 and ACTION_MODE_14 at which SE_REACTION
 * plays */
#define MODE_13_REACTION 40.0F
#define MODE_14_REACTION 16.0F

/* the submode a mode ends in. a mode that cleans up does so at the step it
 * finds MODE_DONE. as he leaves a mode that hasn't reached it, his step runs
 * the mode once more in MODE_DONE first. a mode the cleanup asks for replaces
 * the one he was leaving for */
#define MODE_DONE (-1)

/* bits of his record's figures, which say what boss_draw draws */
#define FIGURE_BODY 0x1 /* his body at its action, and dat_0c9e49d0's tree */
#define FIGURE_FORM 0x8 /* the forming figure, at form_frame */

/* a bit of his work's flags, set once SE_REACTION has played */
#define REACTION_SOUNDED 0x100

/* the entries of chaos7_actions that his modes ask for, named for those
 * modes */
enum chaos7_action {
    ACTION_READY = 0,
    ACTION_MODE_2 = 1,
    ACTION_MODE_13 = 2,
    ACTION_MODE_14 = 3,
    ACTION_BREATH = 4,
    /* it plays on from ACTION_BREATH. his beams fire as it shows */
    ACTION_BREATH_FIRE = 5,
    ACTION_BREATH_END = 6,
    ACTION_TORNADOES = 7,
    ACTION_RAY = 8,
    ACTION_EVENT_RAY = 9,
    ACTION_RUN = 10 /* BOSS_RUN's, and BOSS_HIT's */
};

/* the nodes chaos7_beam_matrices covers, and his body's nodes */
#define BEAM_NODES 8
#define BODY_NODES 42
/* the node his ray starts at */
#define RAY_NODE 15
/* the nodes of dat_0c9e49d0's tree, which spinning_nodes lists */
#define SPINNING_NODES 4

/* his breath's beams: one at player 0, and the rest at the sea */
#define BREATH_BEAMS 7
/* the steps, less one, from his breath's beams to ACTION_BREATH_END */
#define BREATH_HOLD 180
/* where his breath's beams at the sea land. the first lands BREATH_NEAREST
 * of the way from him to player 0, in x and z, and each next one
 * BREATH_FARTHER of the way farther. the first lies BREATH_FIRST_TURN short
 * of the angle from him to player 0, and each next one BREATH_TURN past the
 * one before */
#define BREATH_NEAREST 0.6F
#define BREATH_FARTHER 0.03F
#define BREATH_FIRST_TURN 151
#define BREATH_TURN 303

/* BOSS_TORNADOES starts its tornadoes once ACTION_TORNADOES passes frame
 * TORNADO_START, and ends past frame TORNADO_END */
#define TORNADO_START 31.0F
#define TORNADO_END 81.0F

/* how far BOSS_RAY turns him toward player 0 a step, and the frames of
 * ACTION_RAY at which its ray starts and bursts */
#define RAY_TURN 18
#define RAY_START 36.0F
#define RAY_BURST 86.0F
/* BOSS_RAY's ray stretches to RAY_REACH times the length that takes it
 * down to a height of RAY_FLOOR, along its tilt. each step a ray grows by
 * up to RAY_GROWTH */
#define RAY_REACH 1.05F
#define RAY_FLOOR 100.0F
#define RAY_GROWTH 15.0F
/* the ripples to come after a ray's first, as a ripple's vars.b[4] counts
 * them */
#define RAY_RIPPLES 3
/* BOSS_EVENT_RAY's frames of ACTION_EVENT_RAY. its ripples start at
 * EVENT_RIPPLES, and its ray starts at EVENT_RAY_START and bursts at
 * EVENT_RAY_BURST */
#define EVENT_RIPPLES 102.0F
#define EVENT_RAY_START 110.0F
#define EVENT_RAY_BURST 160.0F

/* the forming figure plays from frame 0 to FORM_END, FORM_RATE frames a
 * step. it fades in over its first FORM_FADE frames */
#define FORM_END 119.0F
#define FORM_RATE 0.5F
#define FORM_FADE 30.0F
/* the steps fading_nodes take to fade in or out */
#define FADE_STEPS 120

/* his water: water_textures holds WATER_ROWS rows of WATER_FRAMES frames,
 * one row for each of water_slots. the frame moves on every WATER_STEPS
 * calls of animate_water, which his step makes once a step */
#define WATER_ROWS 4
#define WATER_FRAMES 4
#define WATER_STEPS 4

/* chaos7_knock_player_back throws player 0 at KNOCK_SPEED in x and z and
 * KNOCK_RISE up. in the fight's second part, it puts an attack fewer than
 * ATTACK_SOON steps away back to ATTACK_LATER */
#define KNOCK_SPEED 2.5F
#define KNOCK_RISE 1.0F
#define ATTACK_SOON 30
#define ATTACK_LATER 60

/* what his modes keep in his work's vars */
struct boss_vars {
    union {
        /* the length BOSS_EVENT_RAY's ray grows to, which
         * chaos7_event_ray sets */
        Float ray_length;
        Sint32 tornadoes; /* how many BOSS_TORNADOES has started */
    } attack;
    /* a count of steps: down in BOSS_BREATH and BOSS_FORM, up in
     * BOSS_MELT */
    Sint32 timer;
    /* the index in breakup_events of the hit's next event, and the steps
     * until it */
    union {
        Sint32 both;
        struct {
            Sint16 event;
            Sint16 wait;
        } each;
    } breakup;
};

#define VARS(work) ((struct boss_vars *)&(work)->vars)

/* an event of the hit's script, which breaks off a node of his body */
struct breakup_event {
    Sint8 node; /* the node it hides. below 0 ends the script */
    Sint8 delay; /* the steps until the next event, or 0 for the same step */
    Sint8 bursts; /* how many bursts it sends from the node */
    Sint8 fragments; /* how many fragments it sends from the node */
    Float spread; /* the farthest a burst lands from the node */
    Float scale; /* the scale it gives the bursts, which no draw reads */
    Sint8 flags;
};

/* bits of an event's flags. BREAKUP_SOUND_A plays SE_BREAKUP_A.
 * BREAKUP_SOUND_B plays SE_BREAKUP_B, unless BREAKUP_SOUND_A is set too.
 * BREAKUP_SHAKE starts a fading shake of pad 0 */
#define BREAKUP_SOUND_A 0x1
#define BREAKUP_SOUND_B 0x2
#define BREAKUP_SHAKE 0x4

static void boss_draw(struct task *task);

#pragma section HEAD
/* how far his figure and his pose stand above his pos, along his turned
 * y */
static Float figure_height = 0.0F;
/* the frames of ACTION_RAY at which BOSS_RAY's ripples and flash start */
static Float ray_ripple_frame = 28.0F;
static Float ray_flash_frame = 12.0F;
/* the frame of ACTION_EVENT_RAY at which BOSS_EVENT_RAY's flash starts */
static Float event_flash_frame = 92.0F;
/* how many mist puffs he makes as he sets up */
static int mist_puff_count = 4;
Sint32 chaos7_volume = 320;
#pragma section

/* a node's origin, where the hit's bursts and fragments start in it, and a
 * fragment's speed before its turn. each fragment replaces the x with a
 * random 2 to 4 */
static const NJS_POINT3 breakup_origin = {0.0F, 0.0F, 0.0F};
static const NJS_VECTOR fragment_speed = {2.5F, 0.0F, 0.0F};

/* the marks his step moves boss_points onto: the origins of the nodes his
 * beams start from, in chaos7_beam_matrices' order, each with its node's x
 * axis */
static struct boss_mark chaos7_marks[] = {
    {16, 0, {0.0F, 0.0F, 0.0F}}, {10, 0, {0.0F, 0.0F, 0.0F}},
    {9, 0, {0.0F, 0.0F, 0.0F}},  {12, 0, {0.0F, 0.0F, 0.0F}},
    {11, 0, {0.0F, 0.0F, 0.0F}}, {4, 0, {0.0F, 0.0F, 0.0F}},
    {6, 0, {0.0F, 0.0F, 0.0F}},  {15, 0, {0.0F, 0.0F, 0.0F}},
    {-1, 0, {0.0F, 0.0F, 0.0F}},
};
/* his motion's table, which enum chaos7_action indexes. its twelfth entry
 * repeats ACTION_RUN's */
/* clang-format off */
static struct motion_entry chaos7_actions[] = {
    {&dat_0ca327d4, 42, MOTION_LOOP, ACTION_READY, 0.0625F, 0.5F},
    {&dat_0ca3c678, 42, MOTION_LOOP, ACTION_MODE_2, 0.0625F, 0.5F},
    {&dat_0ca24d44, 42, MOTION_ONCE, ACTION_READY, 0.0625F, 0.5F},
    {&dat_0ca19b20, 42, MOTION_ONCE, ACTION_READY, 0.0625F, 0.5F},
    {&dat_0ca275a8, 42, MOTION_ONCE, ACTION_BREATH_FIRE, 0.0625F, 0.5F},
    {&dat_0ca2843c, 42, MOTION_LOOP, ACTION_BREATH_FIRE, 1.0F / 14.0F, 0.5F},
    {&dat_0ca2ad30, 42, MOTION_ONCE, ACTION_READY, 0.0625F, 0.5F},
    {&dat_0ca480ec, 42, MOTION_ONCE, ACTION_READY, 0.0625F, 0.5F},
    {&dat_0c9f0160, 42, MOTION_ONCE, ACTION_READY, 0.0625F, 0.5F},
    {&dat_0ca00e74, 42, MOTION_ONCE, ACTION_READY, 0.0625F, 0.5F},
    {&dat_0ca03d18, 42, MOTION_LOOP, ACTION_RUN, 0.0625F, 0.5F},
    {&dat_0ca03d18, 42, MOTION_LOOP, ACTION_RUN, 0.0625F, 0.5F},
};
/* the hit's script, a list of events that a node below 0 ends */
static struct breakup_event breakup_events[] = {
    {15, 0, 1, 6, 0.0F, 1.0F, BREAKUP_SOUND_B},
    {19, 0, 0, 0, 0.0F, 1.0F, 0},
    {17, 8, 0, 0, 0.0F, 1.0F, BREAKUP_SHAKE},
    {18, 0, 0, 0, 0.0F, 1.0F, 0},
    {20, 0, 0, 0, 0.0F, 1.0F, 0},
    {21, 8, 1, 6, 0.0F, 1.0F, BREAKUP_SOUND_A},
    {22, 0, 1, 6, 0.0F, 1.0F, 0},
    {23, 0, 0, 0, 0.0F, 1.0F, 0},
    {24, 8, 1, 6, 0.0F, 1.0F, 0},
    {25, 0, 0, 0, 0.0F, 1.0F, 0},
    {26, 8, 1, 6, 0.0F, 1.0F, 0},
    {27, 0, 0, 0, 0.0F, 1.0F, 0},
    {13, 8, 1, 6, 0.0F, 1.0F, BREAKUP_SOUND_B},
    {14, 0, 0, 0, 0.0F, 1.0F, 0},
    {1, 8, 2, 6, 50.0F, 1.0F, 0},
    {2, 0, 2, 0, 50.0F, 1.0F, 0},
    {16, 0, 1, 6, 0.0F, 1.0F, 0},
    {3, 0, 0, 0, 0.0F, 1.0F, 0},
    {5, 0, 0, 0, 0.0F, 1.0F, 0},
    {4, 8, 1, 0, 0.0F, 1.0F, BREAKUP_SOUND_A},
    {6, 0, 1, 0, 0.0F, 1.0F, 0},
    {9, 0, 1, 0, 0.0F, 1.0F, 0},
    {10, 0, 1, 0, 0.0F, 1.0F, 0},
    {11, 8, 1, 0, 0.0F, 1.0F, 0},
    {12, 0, 1, 0, 0.0F, 1.0F, 0},
    {7, 8, 8, 30, 300.0F, 2.0F, BREAKUP_SOUND_B},
    {8, 0, 0, 0, 0.0F, 2.0F, BREAKUP_SHAKE},
    {36, 8, 1, 0, 0.0F, 0.75F, 0},
    {41, 0, 1, 0, 0.0F, 0.75F, 0},
    {35, 8, 1, 0, 0.0F, 0.75F, 0},
    {40, 0, 1, 0, 0.0F, 0.75F, 0},
    {34, 8, 1, 0, 0.0F, 0.75F, 0},
    {39, 0, 1, 0, 0.0F, 0.75F, 0},
    {33, 8, 1, 0, 0.0F, 0.75F, 0},
    {38, 0, 1, 0, 0.0F, 0.75F, 0},
    {32, 8, 1, 0, 0.0F, 0.75F, BREAKUP_SOUND_B},
    {37, 0, 1, 0, 0.0F, 0.75F, 0},
    {-1, 0, 0, 0, 0.0F, 0.0F, 0},
};
/* clang-format on */
/* lists that a null ends. spinning_nodes are dat_0c9e49d0's four nodes,
 * which turn about y each step */
static NJS_OBJECT *spinning_nodes[] = {
    &dat_0c9e49d0, &dat_0c9e3d2c, &dat_0c9e2d88, &dat_0c9e243c, NULL,
};
/* four nodes of his body, which he keeps hidden */
static NJS_OBJECT *hidden_nodes[] = {
    &dat_0c9c9778, &dat_0c9c9a18, &dat_0c9c9088, &dat_0c9c9328, NULL,
};
/* the nodes BOSS_FORM fades in and BOSS_MELT fades out */
static NJS_OBJECT *fading_nodes[] = {
    &dat_0c9d32c8, &dat_0c9d1004, &dat_0c9ce914, &dat_0c9cae64,
    &dat_0c9c9cec, &dat_0c9cd2d0, &dat_0c9cd970, &dat_0c9e49d0,
    &dat_0c9e3d2c, &dat_0c9e2d88, &dat_0c9e243c, NULL,
};
/* while it is set, boss_draw clears NJD_FLAG_USE_ALPHA in his materials.
 * nothing in the image sets it */
static Sint8 drawn_opaque = 0;
/* a byte of 1, which nothing in the image points at */
static Uint8 dat_0ca5eca9 = 1;
/* the frames of ACTION_TORNADOES at which each tornado starts. the last,
 * 999, lies past the mode's end, so no fourth tornado starts */
static Float tornado_frames[] = {31.0F, 56.0F, 81.0F, 999.0F};
/* the textures of his texlist that his water animates, one for each row of
 * water_textures */
static Sint8 water_slots[] = {3, 2, 9, 10};
/* how far each of spinning_nodes turns about y a step */
static Angle spin_steps[] = {0xccc, 0, 0x666, 0x666};
/* the node each matrix of chaos7_beam_matrices is taken from. his step
 * then takes chaos7_beam_matrices[CHAOS7_RAY_BEAM] again, from RAY_NODE in
 * place of node 0 */
static Sint8 beam_nodes[] = {16, 10, 9, 12, 11, 4, 6, 0};

NJS_MATRIX chaos7_beam_matrices[BEAM_NODES];
NJS_MATRIX chaos7_node_matrices[BODY_NODES];
/* his body's nodes, depth first, as his setup lists them, and the null
 * after them */
static NJS_OBJECT *body_nodes[BODY_NODES + 1];
/* twelve bytes that nothing in the image points at, which keep the bss
 * after them at the image's addresses */
static Uint8 dat_0ca6c204[12];
/* the frame the forming figure plays at */
static Float form_frame;
/* where his breath's beams 1 to 6 aim, which must last while they do. beam
 * 0 aims at player 0 */
static NJS_POINT3 breath_targets[BREATH_BEAMS];
/* BOSS_RAY's ray, and BOSS_EVENT_RAY's, and their works */
static struct task *ray_task;
static struct task_work *ray_work;
static struct task *event_ray_task;
static struct task_work *event_ray_work;
/* the node of the event the hit ran last */
static Sint16 breakup_node;
/* six bytes that nothing in the image points at, which keep the bss after
 * them at the image's addresses */
static Uint8 dat_0ca6c27a[6];
/* a node's matrix in his pose, for his step */
static NJS_MATRIX node_matrix;

/* BOSS_READY: he plays ACTION_READY */
static void boss_ready(struct task_work *work, struct motion *motion)
{
    switch (work->submode) {
    case 0:
        motion->wanted = ACTION_READY;
        work->submode = 1;
        break;
    case 1:
        break;
    }
    /* @hack redundant: a return that ends nothing, for the image's MOV.B and
     * CMP/EQ at 0x0c9154f6 and 0x0c9154fa. without it, the functions after
     * move, and 3 of 14 words differ */
    if (work->submode == MODE_DONE) {
        return;
    }
}

/* BOSS_BREATH. until ACTION_BREATH_FIRE shows, he makes a breath light
 * every fourth step. as it shows, he breathes a beam at player 0, and six
 * at the sea between them. in the fight's second part the mode then ends.
 * otherwise ACTION_BREATH_END starts BREATH_HOLD + 1 steps later, and the
 * mode ends as it shows */
static void boss_breath(struct task *task, struct chaos *chaos)
{
    struct task_work *work = task->work;
    /* @hack spelling: the record copied to a local, which SHC keeps at R15+16
     * to test its steps (stored at 0x0c91551e, read at 0x0c915552). through
     * chaos alone, the stack frame shrinks by 4 bytes and 264 of 317 words
     * differ */
    struct chaos *record = chaos;
    struct task_work *player;
    Float dx;
    Float dz;
    Float distance;
    Float scale;
    Float radius;
    Angle ang;
    Angle heading;
    int i;

    switch (work->submode) {
    case 0:
        chaos->motion.wanted = ACTION_BREATH;
        sound_play(SE_BEAM_START, 0, 0, 0);
        work->submode = 1;
        /* fall through */
    case 1:
        if (!(record->steps & 3)) {
            task_create_child(TASK_MOTION | TASK_WORK,
                              chaos7_breath_light_start, task);
        }
        if (chaos->motion.current == ACTION_BREATH_FIRE) {
            player = player_work[0];
            chaos7_breath_create(&chaos7_beam_matrices[0], &player->pos);
            dx = player->pos.x - work->pos.x;
            dz = player->pos.z - work->pos.z;
            distance = njSqrt((dx * dx) + (dz * dz));
            heading = (Angle)(atan2f(dz, dx) * ANGLE_PER_RADIAN);
            ang = heading - BREATH_FIRST_TURN;
            scale = BREATH_NEAREST;
            for (i = 1; i < BREATH_BEAMS; i++) {
                radius = distance * scale;
                breath_targets[i].x = work->pos.x + (radius * njCos(ang));
                breath_targets[i].y = 0.0F;
                breath_targets[i].z = work->pos.z + (radius * njSin(ang));
                chaos7_breath_create(&chaos7_beam_matrices[i],
                                     &breath_targets[i]);
                ang += BREATH_TURN;
                scale += BREATH_FARTHER;
            }
            if (chaos7_part_2) {
                work->submode = MODE_DONE;
            } else {
                VARS(work)->timer = BREATH_HOLD;
                work->submode = 2;
            }
        }
        break;
    case 2:
        if (!VARS(work)->timer--) {
            chaos->motion.wanted = ACTION_BREATH_END;
            work->submode = 3;
        }
        break;
    case 3:
        if (chaos->motion.current == ACTION_BREATH_END) {
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE && work->mode != BOSS_READY) {
        boss_next_mode = BOSS_READY;
        boss_last_mode = work->mode;
    }
}

/* BOSS_TORNADOES, with the mist hidden. past ACTION_TORNADOES' frame
 * TORNADO_START, a tornado starts as it reaches each of tornado_frames. the
 * second doesn't sway. past frame TORNADO_END the mode ends, and the mist
 * shows again */
static void boss_tornadoes(struct task *task, struct motion *motion)
{
    struct task_work *work = task->work;
    struct task *tornado;

    switch (work->submode) {
    case 0:
        motion->wanted = ACTION_TORNADOES;
        VARS(work)->attack.tornadoes = 0;
        chaos7_mist_on = 0;
        work->submode = 1;
        /* fall through */
    case 1:
        if (motion->current == ACTION_TORNADOES &&
            motion->frame > TORNADO_START) {
            work->submode = 2;
        }
        break;
    case 2:
        if (motion->frame >= tornado_frames[VARS(work)->attack.tornadoes]) {
            tornado = task_create_child(TASK_WORK, chaos7_tornado_start, task);
            if (tornado) {
                tornado->work->pos = work->pos;
                tornado->work->ang.y = work->ang.y;
                tornado->work->mode =
                    VARS(work)->attack.tornadoes != 1 ? TORNADO_SWAYS : 0;
            }
            VARS(work)->attack.tornadoes++;
        }
        if (motion->frame > TORNADO_END) {
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        if (work->mode != BOSS_READY) {
            boss_next_mode = BOSS_READY;
            boss_last_mode = work->mode;
        }
        chaos7_mist_on = 1;
    }
}

/* BOSS_RAY, with the mist hidden. until the ray bursts, he turns toward
 * player 0. ACTION_RAY's frames start the flash, the ripples and the ray,
 * which grows toward its reach and bursts at RAY_BURST. while it grows,
 * each step the ray's collision touches another collision knocks player 0
 * back. a step after
 * the burst the mode ends: he kills the ray and faces his place's heading
 * again */
static void boss_ray(struct task_work *work, struct chaos *chaos)
{
    struct task *ripples;
    Float length;

    if (work->submode <= 5) {
        struct task_work *player = player_work[0];
        Float dx;
        Float dz;
        Angle heading;

        dx = player->pos.x - work->pos.x;
        dz = player->pos.z - work->pos.z;
        heading = (Angle)(atan2f(dz, dx) * ANGLE_PER_RADIAN);
        work->ang.y = angle_approach(work->ang.y, heading, RAY_TURN);
    }
    switch (work->submode) {
    case 0:
        ray_task = NULL;
        chaos->motion.wanted = ACTION_RAY;
        chaos7_mist_on = 0;
        work->submode = 1;
        /* fall through */
    case 1:
        if (chaos->motion.current == ACTION_RAY) {
            work->submode = 2;
        }
        break;
    case 2:
        if (chaos->motion.frame >= ray_flash_frame) {
            task_create(TASK_MOTION | TASK_WORK, 2, chaos7_ray_flash_start);
            sound_play(SE_BEAM_START, 0, 0, 0);
            work->submode = 3;
        }
        break;
    case 3:
        if (chaos->motion.frame >= ray_ripple_frame) {
            ripples = task_create(TASK_MOTION | TASK_WORK, 2,
                                  chaos7_ray_ripple_start);
            if (ripples) {
                ripples->work->vars.b[4] = RAY_RIPPLES;
            }
            work->submode = 4;
        }
        break;
    case 4:
        if (chaos->motion.frame >= RAY_START) {
            if ((ray_task = task_create(TASK_MOTION | TASK_WORK, 2,
                                        chaos7_ray_start)) != NULL) {
                sound_play(SE_RAY, 0, 0, 0);
                work->submode = 5;
            } else {
                work->submode = MODE_DONE;
            }
        }
        break;
    case 5:
        ray_work = ray_task->work;
        length = _builtin_fabsf((ray_work->pos.y - RAY_FLOOR) /
                                njSin(ray_work->ang.z - RAY_SHAPE_TURN));
        length *= RAY_REACH;
        RAY_VARS(ray_work)->length =
            length < RAY_VARS(ray_work)->length + RAY_GROWTH
                ? length
                : RAY_VARS(ray_work)->length + RAY_GROWTH;
        if (ray_work->collision->flags & COLLISION_HIT) {
            chaos7_knock_player_back();
        }
        if (chaos->motion.frame >= RAY_BURST) {
            ray_task->work->mode = RAY_BURSTING;
            work->submode = 6;
        }
        break;
    case 6:
        work->submode = MODE_DONE;
        break;
    }
    if (work->submode == MODE_DONE) {
        if (ray_task) {
            task_kill(ray_task);
        }
        work->ang.y = chaos7_boss_heading;
        if (work->mode != BOSS_READY) {
            boss_next_mode = BOSS_READY;
            boss_last_mode = work->mode;
        }
        chaos7_mist_on = 1;
    }
}

/* BOSS_EVENT_RAY: BOSS_RAY's flash, ripples and ray at ACTION_EVENT_RAY's
 * frames. he doesn't turn, the mist stays, and no touch knocks player 0
 * back. the ray grows toward the length chaos7_event_ray gives, and
 * bursts at EVENT_RAY_BURST. the mode then ends as ACTION_READY shows, and
 * leaves the burst ray to idle */
static void boss_event_ray(struct task_work *work, struct chaos *chaos)
{
    struct task *ripples;

    switch (work->submode) {
    case 0:
        chaos->motion.wanted = ACTION_EVENT_RAY;
        work->submode = 1;
        /* fall through */
    case 1:
        if (chaos->motion.current == ACTION_EVENT_RAY) {
            work->submode = 2;
        }
        break;
    case 2:
        if (chaos->motion.frame >= event_flash_frame) {
            task_create(TASK_MOTION | TASK_WORK, 2, chaos7_ray_flash_start);
            work->submode = 3;
        }
        break;
    case 3:
        if (chaos->motion.frame >= EVENT_RIPPLES) {
            ripples = task_create(TASK_MOTION | TASK_WORK, 2,
                                  chaos7_ray_ripple_start);
            if (ripples) {
                ripples->work->vars.b[4] = RAY_RIPPLES;
            }
            work->submode = 4;
        }
        break;
    case 4:
        if (chaos->motion.frame >= EVENT_RAY_START) {
            if ((event_ray_task = task_create(TASK_MOTION | TASK_WORK, 2,
                                              chaos7_ray_start)) != NULL) {
                sound_play(SE_RAY, 0, 0, 0);
                work->submode = 5;
            } else {
                work->submode = MODE_DONE;
            }
        }
        break;
    case 5:
        event_ray_work = event_ray_task->work;
        RAY_VARS(event_ray_work)->length =
            VARS(work)->attack.ray_length <
                    RAY_VARS(event_ray_work)->length + RAY_GROWTH
                ? VARS(work)->attack.ray_length
                : RAY_VARS(event_ray_work)->length + RAY_GROWTH;
        if (chaos->motion.frame >= EVENT_RAY_BURST) {
            event_ray_task->work->mode = RAY_BURSTING;
            work->submode = 6;
        }
        break;
    case 6:
        if (chaos->motion.current == ACTION_READY) {
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE && work->mode != BOSS_READY) {
        boss_next_mode = BOSS_READY;
        boss_last_mode = work->mode;
    }
}

/* BOSS_RUN: he plays ACTION_RUN, with SE_RUN_ACTION as it shows. each step
 * keeps SE_RUN up for his work */
static void boss_run(struct task_work *work, struct motion *motion)
{
    switch (work->submode) {
    case 0:
        motion->wanted = ACTION_RUN;
        work->submode = 1;
        /* fall through */
    case 1:
        if (motion->current == ACTION_RUN) {
            sound_play(SE_RUN_ACTION, 0, 0, 0);
            work->submode = 2;
        }
        /* fall through */
    case 2:
        sound_play_for(SE_RUN, (int)work, 1, 0, 2);
        break;
    }
    /* @hack redundant: a return that ends nothing, for the image's MOV.B and
     * CMP/EQ at 0x0c915d38 and 0x0c915d3a. without it, 4 of 42 words
     * differ */
    if (work->submode == MODE_DONE) {
        return;
    }
}

/* BOSS_HIT. he shows his body's nodes but hidden_nodes, hides
 * spinning_nodes and plays ACTION_RUN. breakup_events then hides his nodes,
 * an event at a time, with their bursts and fragments. after the last event
 * he goes to BOSS_BROKEN */
static void boss_hit(struct task *task, struct chaos *chaos)
{
    struct task_work *work = task->work;
    struct breakup_event *event;
    struct task *effect;
    struct task_work *bursts;
    NJS_OBJECT *object;
    NJS_POINT3 origin;
    NJS_VECTOR speed;
    int count;

    switch (work->submode) {
    case 0:
        evalflags_clear_all(body_nodes, NJD_EVAL_HIDE);
        evalflags_set_all(hidden_nodes, NJD_EVAL_HIDE);
        evalflags_set_all(spinning_nodes, NJD_EVAL_HIDE);
        chaos->motion.wanted = ACTION_RUN;
        /* @hack type: the event and the wait cleared through the union's
         * Sint32, in one MOV.L (0x0c915db4); without it, as a store to
         * each, 83 of 317 words differ */
        VARS(work)->breakup.both = 0;
        sound_play(SE_RUN_ACTION, 0, 0, 127);
        work->submode = 2;
        /* fall through */
    case 2:
        while (VARS(work)->breakup.each.wait-- <= 0) {
            origin = breakup_origin;
            speed = fragment_speed;
            event = &breakup_events[VARS(work)->breakup.each.event++];
            breakup_node = (Sint16)event->node;
            if (breakup_node < 0) {
                work->submode = MODE_DONE;
                break;
            }
            VARS(work)->breakup.each.wait = (Sint16)event->delay;
            object = body_nodes[breakup_node];
            /* @hack spelling: the hide as a ?: with an empty arm, which keeps
             * work in R11 and breakup_node's address in R12 (0x0c915d7c,
             * 0x0c915dc8). as an if, the two trade and 21 of 317 words
             * differ */
            object ? (object->evalflags |= NJD_EVAL_HIDE) : 0;
            if (event->bursts > 0) {
                effect = task_create(TASK_WORK, 2, chaos7_damage_bursts_step);
                if (effect) {
                    bursts = effect->work;
                    njPushMatrix(&chaos7_node_matrices[breakup_node]);
                    njCalcPoint(NULL, &origin, &bursts->pos);
                    njPopMatrix(1);
                    bursts->vars.b[4] = event->bursts;
                    bursts->vars.f[2] = event->spread;
                    bursts->scl.x = event->scale;
                }
            }
            count = (int)event->fragments;
            while (count > 0) {
                njPushMatrix(&_unit_matrix_);
                effect = task_create(TASK_MOTION | TASK_WORK, 2,
                                     chaos7_damage_drop_start);
                if (effect) {
                    /* TURN reads its angle twice, so each turn draws two
                     * random numbers. the first decides whether it turns at
                     * all */
                    TURN(njRotateZ,
                         (Angle)(RANDOM(30.0F, 75.0F) * ANGLE_PER_DEGREE));
                    TURN(njRotateY, (Angle)(rand() * (360.0F * RAND_SCALE) *
                                            ANGLE_PER_DEGREE));
                    speed.x = RANDOM(2.0F, 4.0F);
                    njCalcVector(NULL, &speed, &effect->motion->speed);
                    njPushMatrix(&chaos7_node_matrices[breakup_node]);
                    njCalcPoint(NULL, &origin, &effect->work->pos);
                    njPopMatrix(1);
                }
                njPopMatrix(1);
                count--;
            }
            if (event->flags & BREAKUP_SOUND_A) {
                sound_play(SE_BREAKUP_A, 0, 0, 127);
            } else if (event->flags & BREAKUP_SOUND_B) {
                sound_play(SE_BREAKUP_B, 0, 0, 127);
            }
            if (event->flags & BREAKUP_SHAKE) {
                vibration_start_fading(0, 7, 59, 6);
            }
        }
        break;
    }
    if (work->submode == MODE_DONE && work->mode != BOSS_BROKEN) {
        boss_next_mode = BOSS_BROKEN;
        boss_last_mode = work->mode;
    }
}

/* BOSS_BROKEN: he draws no figure. as the mode ends, he asks for
 * ACTION_READY, and shows his body's nodes but hidden_nodes */
static void boss_broken(struct task *task, struct chaos *chaos)
{
    struct task_work *work = task->work;
    /* @hack spelling: the record copied to a local, which SHC copies to R6 for
     * the figures' store (MOV R5,R6 at 0x0c916094). through chaos alone, 19
     * of 23 words differ */
    struct chaos *record = chaos;

    record->figures = 0;
    if (work->submode == MODE_DONE) {
        chaos->motion.wanted = ACTION_READY;
        evalflags_clear_all(body_nodes, NJD_EVAL_HIDE);
        evalflags_set_all(hidden_nodes, NJD_EVAL_HIDE);
    }
}

/* BOSS_FORM. the forming figure plays forward to FORM_END while his motion
 * holds. his body then shows at ACTION_READY's start, with spinning_nodes,
 * and fading_nodes fade in. SE_FORM holds until the fade ends, and so does
 * the mode */
static void boss_form(struct task *task, struct chaos *chaos)
{
    struct task_work *work = task->work;
    /* @hack spelling: the record copied to a local. SHC keeps chaos in R10 and
     * the copy, which stores the figures, in R11 (MOV R5,R10 and MOV R5,R11
     * at 0x0c91611e and 0x0c916122). through chaos alone, SHC saves one
     * register fewer, the functions after move, and 84 of 113 words
     * differ */
    struct chaos *record = chaos;

    switch (work->submode) {
    case 0:
        form_frame = 0.0F;
        record->figures = FIGURE_FORM;
        motion_flags |= MOTION_HOLDING;
        sound_play_held(SE_FORM, (int)work, 1, chaos7_volume);
        work->submode = 1;
        /* fall through */
    case 1:
        record->figures = FIGURE_FORM;
        form_frame += FORM_RATE;
        if (form_frame >= FORM_END) {
            motion_flags &= ~MOTION_HOLDING;
            record->figures = FIGURE_BODY;
            chaos->motion.frame = 0.0F;
            chaos->motion.wanted = ACTION_READY;
            evalflags_clear_all(spinning_nodes, NJD_EVAL_HIDE);
            chaos_fade_alpha(task, fading_nodes, 0.0F, 1.0F, FADE_STEPS);
            VARS(work)->timer = FADE_STEPS;
            work->submode = 2;
        }
        break;
    case 2:
        if (!--VARS(work)->timer) {
            sound_stop(SE_FORM);
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE && work->mode != BOSS_READY) {
        boss_next_mode = BOSS_READY;
        boss_last_mode = work->mode;
    }
}

/* BOSS_MELT. fading_nodes fade out. once the fade is past and ACTION_READY
 * shows, the forming figure plays backward from FORM_END while his motion
 * holds, with SE_FORM held. he then shows no figure, and goes to
 * BOSS_MELTED */
static void boss_melt(struct task *task, struct chaos *chaos)
{
    struct task_work *work = task->work;
    /* @hack spelling: the record copied to a local. SHC keeps chaos in R9 and
     * the copy, which stores the figures, in R11 (MOV R5,R11 and MOV R5,R9
     * at 0x0c916248 and 0x0c91624e). through chaos alone, SHC saves one
     * register fewer, and 40 of 102 words differ */
    struct chaos *record = chaos;

    switch (work->submode) {
    case 0:
        chaos->motion.wanted = ACTION_READY;
        VARS(work)->timer = 0;
        chaos_fade_alpha(task, fading_nodes, 1.0F, 0.0F, FADE_STEPS);
        work->submode = 1;
        /* fall through */
    case 1:
        if (VARS(work)->timer++ > FADE_STEPS) {
            if (chaos->motion.current == ACTION_READY) {
                record->figures = FIGURE_FORM;
                motion_flags |= MOTION_HOLDING;
                form_frame = FORM_END;
                sound_play_held(SE_FORM, (int)work, 1, chaos7_volume);
                work->submode = 2;
            }
        }
        break;
    case 2:
        form_frame -= FORM_RATE;
        if (form_frame <= 0.0F) {
            record->figures = 0;
            motion_flags &= ~MOTION_HOLDING;
            sound_stop(SE_FORM);
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        record->figures = 0;
        if (work->mode != BOSS_MELTED) {
            boss_next_mode = BOSS_MELTED;
            boss_last_mode = work->mode;
        }
    }
}

/* BOSS_MODE_2: he shows his body and plays ACTION_MODE_2 */
static void boss_mode_2(struct task_work *work, struct chaos *chaos)
{
    /* @hack spelling: the record copied to a local, which SHC copies to R7 for
     * the figures' store (MOV R5,R7 at 0x0c916386). through chaos alone, the
     * functions after move, and 15 of 17 words differ */
    struct chaos *record = chaos;

    switch (work->submode) {
    case 0:
        record->figures = FIGURE_BODY;
        chaos->motion.wanted = ACTION_MODE_2;
        work->submode = 1;
        break;
    case 1:
        break;
    }
    /* @hack redundant: a return that ends nothing, for the image's MOV.B and
     * CMP/EQ at 0x0c91639c and 0x0c9163a0. without it, the functions after
     * move, and 3 of 17 words differ */
    if (work->submode == MODE_DONE) {
        return;
    }
}

/* BOSS_MODE_13: he plays ACTION_MODE_13 once, with SE_REACTION as it
 * reaches MODE_13_REACTION. the mode ends as ACTION_READY shows */
static void boss_mode_13(struct task_work *work, struct motion *motion)
{
    switch (work->submode) {
    case 0:
        motion->wanted = ACTION_MODE_13;
        work->flags &= ~REACTION_SOUNDED;
        work->submode = 1;
        break;
    case 1:
        if (!(work->flags & REACTION_SOUNDED)) {
            if (motion->frame >= MODE_13_REACTION) {
                sound_play(SE_REACTION, 0, 0, 127);
                work->flags |= REACTION_SOUNDED;
            }
        }
        if (motion->current == ACTION_READY) {
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE && work->mode != BOSS_READY) {
        boss_next_mode = BOSS_READY;
        boss_last_mode = work->mode;
    }
}

/* BOSS_MODE_14: he plays ACTION_MODE_14 once, with SE_REACTION as it
 * reaches MODE_14_REACTION. the mode ends as ACTION_READY shows */
static void boss_mode_14(struct task_work *work, struct motion *motion)
{
    switch (work->submode) {
    case 0:
        work->flags &= ~REACTION_SOUNDED;
        motion->wanted = ACTION_MODE_14;
        work->submode = 1;
        break;
    case 1:
        if (!(work->flags & REACTION_SOUNDED)) {
            if (motion->frame >= MODE_14_REACTION) {
                sound_play(SE_REACTION, 0, 0, 127);
                work->flags |= REACTION_SOUNDED;
            }
        }
        if (motion->current == ACTION_READY) {
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE && work->mode != BOSS_READY) {
        boss_next_mode = BOSS_READY;
        boss_last_mode = work->mode;
    }
}

/* run the mode his work is in for a step */
static void boss_step_mode(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos *chaos = task->extra;

    switch (work->mode) {
    case BOSS_READY:
        boss_ready(work, &chaos->motion);
        break;
    case BOSS_BREATH:
        boss_breath(task, chaos);
        break;
    case BOSS_RAY:
        boss_ray(work, chaos);
        break;
    case BOSS_TORNADOES:
        boss_tornadoes(task, &chaos->motion);
        break;
    case BOSS_MODE_6:
        break;
    case BOSS_RUN:
        boss_run(work, &chaos->motion);
        break;
    case BOSS_EVENT_RAY:
        boss_event_ray(work, chaos);
        break;
    case BOSS_MODE_2:
        boss_mode_2(work, chaos);
        break;
    case BOSS_MODE_13:
        boss_mode_13(work, &chaos->motion);
        break;
    case BOSS_MODE_14:
        boss_mode_14(work, &chaos->motion);
        break;
    case BOSS_HIT:
        boss_hit(task, chaos);
        break;
    case BOSS_FORM:
        boss_form(task, chaos);
        break;
    case BOSS_MELT:
        boss_melt(task, chaos);
        break;
    case BOSS_MELTED:
        chaos->figures = 0;
        break;
    case BOSS_BROKEN:
        boss_broken(task, chaos);
        break;
    }
}

/* his destructor. it ends his mode as his step ends one, and clears the
 * motion callback. it then frees his motion's blend, and writes his welds'
 * copies back and frees them */
static void boss_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct motion *motion = task->extra;

    if (work->submode != MODE_DONE) {
        work->submode = MODE_DONE;
        boss_step_mode(task);
    }
    njSetMotionCallback(NULL);
    motion->state = MOTION_FREE;
    motion_step(work, motion);
    motion->weld_mode = PLAYER_WELD_FREE;
    motion_weld(work, motion);
}

/* his draw's motion callback, which leaves each object as it is */
static void ignore_object(NJS_OBJECT *object)
{
}

/* point each of water_slots in his texlist at its row's frame of
 * water_textures. every WATER_STEPS calls, the frame moves on */
static void animate_water(struct chaos *chaos)
{
    NJS_TEXNAME *texture;
    int i;

    if (++chaos->tick >= WATER_STEPS) {
        chaos->tick = 0;
        if (++chaos->frame >= WATER_FRAMES) {
            chaos->frame = 0;
        }
    }
    for (i = 0; i < WATER_ROWS; i++) {
        texture = &chaos->texlist->textures[water_slots[i]];
        texture->texaddr =
            water_textures[chaos->frame + (i * WATER_FRAMES)].texaddr;
    }
}

/* turn spinning_nodes about y by spin_steps, clearing NJD_EVAL_UNIT_ANG */
static void spin_nodes(void)
{
    /* @hack spelling: steps declared ahead of objects, which puts spin_steps'
     * address ahead of spinning_nodes' in the pool (0x0c91674c,
     * 0x0c916750). declared the other way, the two trade and 2 of 22 words
     * differ */
    Angle *steps = spin_steps;
    NJS_OBJECT **objects = spinning_nodes;
    NJS_OBJECT *object;
    int i;

    /* @hack spelling: steps = steps + 1, which with the never-run steps++
     * below keeps steps in R4 and the object in R5 (0x0c916712, 0x0c91671a).
     * as steps++, the two trade and 9 of 22 words differ */
    for (i = 0; i < SPINNING_NODES; i++, steps = steps + 1) {
        /* @hack redundant: a never-run steps++, which with steps = steps + 1
         * keeps steps in R4 and the object in R5 (0x0c916712, 0x0c91671a);
         * without it, the two trade and 9 of 22 words differ */
        if (0) {
            steps++;
        }
        object = *objects++;
        /* @hack spelling: a continue past a null object. with an if around the
         * turn in its place, SHC unrolls the loop, the functions after move,
         * and 22 of 22 words differ */
        if (!object) {
            continue;
        }
        object->evalflags &= ~NJD_EVAL_UNIT_ANG;
        object->ang[1] += *steps;
    }
}

/* his task's step. the first sets him up, and each later one runs his
 * mode. each step takes the mode boss_next_mode asks for, as MODE_DONE
 * says, and hides the mist while an event is in control. it then sets
 * chaos7_beam_matrices, chaos7_node_matrices and his marks from his pose,
 * and draws him */
void chaos7_boss_step(struct task *task)
{
    struct task_work *work = task->work;
    struct motion *motion = task->extra;
    struct chaos *chaos = task->extra;
    int i;
    NJS_MATRIX *output;
    Sint8 *nodes;
    int *puffs;

    /* @hack redundant: a never-run block of tests of task, which keeps task in
     * R10 (0x0c916774); without it, task goes to the stack frame, which grows
     * from 16 bytes (ADD #-16,R15 at 0x0c916772) to 20, the functions after
     * move, and 377 of 398 words differ */
    if (0) {
        if (task) {
        }
        if (task) {
        }
        if (task) {
        }
    }
    /* @hack redundant: a never-run block of tests of motion, which keeps motion
     * in R8 (0x0c91677e); without it, motion goes to the stack frame, which
     * grows from 16 bytes (ADD #-16,R15 at 0x0c916772) to 20, the functions
     * after move, and 367 of 398 words differ */
    if (0) {
        if (motion) {
        }
        if (motion) {
        }
    }

    switch (work->mode) {
    case BOSS_SETUP:
        motion = motion_create(work, sizeof(struct chaos));
        if (!motion) {
            return;
        }
        motion->current = motion->wanted = ACTION_READY;
        motion->actions = chaos7_actions;
        motion->welds = chaos7_welds;
        boss_next_mode = BOSS_READY;
        work->mode = BOSS_READY;
        task->extra = motion;
        chaos = task->extra;
        boss_work = work;
        chaos_record = chaos;
        chaos->unk_3e = 0;
        chaos->figures = FIGURE_BODY;
        chaos->health = BOSS_HEALTH;
        chaos_current_form = CHAOS_7;
        chaos->colors[0].a = 0.0F;
        chaos->colors[0].r = 0.0F;
        chaos->colors[0].g = 0.0F;
        chaos->colors[0].b = 0.0F;
        boss_next_mode = work->mode;
        motion_step(work, &chaos->motion);
        chaos->texlist = &chaos7_texlist;
        motion_weld(work, &chaos->motion);
        task->draw = boss_draw;
        task->destructor = boss_destroy;
        chaos_list_nodes(&chaos7_body_object, body_nodes);
        /* @hack spelling: mist_puff_count read through a pointer, which SHC
         * keeps in R12 for the loop's test (0x0c916824). read by name, with the
         * tests of puffs below gone, the stack frame grows from 16 bytes
         * (ADD #-16,R15 at 0x0c916772) to 20, the functions after move, and 364
         * of 398 words differ */
        puffs = &mist_puff_count;
        for (i = 0; i < *puffs; i++) {
            task_create_child(TASK_MOTION | TASK_WORK, chaos7_mist_start, task);
        }
        /* @hack redundant: a never-run block of tests of puffs and i, which
         * keeps puffs in R12 and boss_next_mode's address in R11 (0x0c916810,
         * 0x0c91677c); i takes R13 either way. without it, the two trade, as
         * do R15+0 and R15+4 further on (0x0c916a1a), and 34 of 398 words
         * differ */
        if (0) {
            if (puffs) {
            }
            if (puffs) {
            }
            if (i) {
            }
            if (i) {
            }
        }
        /* @hack redundant: a goto to the next line, which loads task_create's
         * address into R2 and evalflags_set_all's into R3 (0x0c91682c,
         * 0x0c91683e). without it, the two registers trade from here to the
         * store after event_in_control (0x0c9168f2), and 18 of 398 words
         * differ */
        goto mist_fade;
    mist_fade:
        task_create(TASK_WORK, 2, chaos7_mist_fade);
        chaos7_mist_on = 0;
        evalflags_set_all(hidden_nodes, NJD_EVAL_HIDE);
        chaos_set_alpha(fading_nodes, 1.0F);
        evalflags_clear_all(body_nodes, NJD_EVAL_HIDE);
        evalflags_set_all(hidden_nodes, NJD_EVAL_HIDE);
        break;
    default:
        boss_step_mode(task);
        break;
    }
    if (event_in_control()) {
        chaos7_mist_on = 0;
    }
    if (work->mode != boss_next_mode) {
        if (work->submode != MODE_DONE) {
            work->submode = MODE_DONE;
            boss_step_mode(task);
        }
        boss_last_mode = work->mode;
        work->mode = boss_next_mode;
        work->submode = 0;
    }
    if (!(motion_flags & MOTION_HOLDING)) {
        motion_step(work, motion);
    }
    motion_weld(work, motion);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, -work->ang.y + 0x8000);
    njTranslate(NULL, 0.0F, figure_height, 0.0F);
    /* @hack spelling: i = 0 ahead of the loop's setup, apart from its for,
     * which loads chaos7_beam_matrices' address into R3 ahead of i's 0 in R2
     * (0x0c916986, 0x0c916988). set in the for, R2 and R3 trade and 9 of 398
     * words differ */
    i = 0;
    output = chaos7_beam_matrices;
    nodes = beam_nodes;
    /* @hack redundant: a never-run test of nodes, which keeps the beam loop's
     * setup as the image has it (0x0c916986 to 0x0c916998); without it, i's 0
     * and node_matrix's address load in another order and register, and 8 of
     * 398 words differ */
    if (0) {
        if (nodes) {
        }
    }
    /* @hack redundant: a goto to the next line, which keeps the beam loop's
     * setup as the image has it (0x0c916986 to 0x0c916998); without it, i's 0
     * and node_matrix's address load in another order and register, and 8 of
     * 398 words differ */
    goto beams;
beams:
    for (; i < BEAM_NODES; i++, output++) {
        njPushMatrix(NULL);
        pose_get_matrix(*nodes, &node_matrix);
        /* @hack spelling: nodes = nodes + 1, which keeps nodes in R13 for the
         * MOV.B @R13+ at 0x0c9169a4. as nodes++, nodes goes to the stack frame,
         * which grows from 16 bytes (ADD #-16,R15 at 0x0c916772) to 20, the
         * functions after move, and 334 of 398 words differ */
        nodes = nodes + 1;
        njMultiMatrix(NULL, &node_matrix);
        njGetMatrix(output);
        njPopMatrix(1);
    }
    njPushMatrix(NULL);
    pose_get_matrix(RAY_NODE, &node_matrix);
    njMultiMatrix(NULL, &node_matrix);
    njGetMatrix(&chaos7_beam_matrices[CHAOS7_RAY_BEAM]);
    njPopMatrix(1);
    /* @hack redundant: a never-run block of calls of njPushMatrix and
     * njPopMatrix, which keeps njPushMatrix's address in R12 and njPopMatrix's
     * in R11 for the loops, and task in R10 (0x0c916774 to 0x0c916810);
     * without it, the registers rotate and 67 of 398 words differ */
    if (0) {
        njPushMatrix(&_unit_matrix_);
        njPopMatrix(1);
        njPopMatrix(1);
    }
    /* @hack redundant: a goto to the next line, which loads node_matrix's
     * address into R2 and its copy into R3 (0x0c916a14 to 0x0c916a1c); without
     * it, the two trade and 4 of 398 words differ */
    goto nodes;
nodes:
    for (i = 0; i < BODY_NODES; i++) {
        njPushMatrix(NULL);
        pose_get_matrix(i, &node_matrix);
        njMultiMatrix(NULL, &node_matrix);
        njGetMatrix(&chaos7_node_matrices[i]);
        njPopMatrix(1);
    }
    njPopMatrix(1);
    chaos_track_points(work, chaos7_marks, figure_height);
    /* @hack spelling: the record passed as motion, cast, which keeps task in
     * R10 and the record's two copies in R8 and R9 (0x0c916774 to 0x0c916782).
     * passed as chaos, they take R9, R13 and R10, and 209 of 398 words
     * differ */
    animate_water((struct chaos *)motion);
    spin_nodes();
    boss_draw(task);
    chaos->steps++;
    life_gauge_health = chaos->health;
    task_step_children(task);
}

/* draw him on a frame's first step, in palette 4, as his figures say. the
 * forming figure fades in over its first FORM_FADE frames: its constant
 * material's alpha, an offset, rises from -1 to 0. the draw also keeps
 * SE_NEAR up for his work */
static void boss_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos *chaos = task->extra;
    int current;
    NJS_ACTION *action;

    if (frame_step) {
        return;
    }
    draw_set_palette(4);
    njSetMotionCallback(ignore_object);
    draw_control_save();
    draw_control_clear(NJD_CONTROL_3D_CONSTANT_ATTR |
                       NJD_CONTROL_3D_CONSTANT_MATERIAL |
                       NJD_CONTROL_3D_OFFSET_MATERIAL);
    draw_attr_save();
    if (drawn_opaque) {
        draw_control_set(NJD_CONTROL_3D_CONSTANT_ATTR);
        njSetConstantAttr(~NJD_FLAG_USE_ALPHA, 0);
    }
    njSetTexture(chaos->texlist);
    current = chaos->motion.current;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    if (chaos->figures & (FIGURE_BODY | FIGURE_FORM)) {
        njPushMatrix(NULL);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, -work->ang.y + 0x8000);
        njTranslate(NULL, 0.0F, figure_height, 0.0F);
        if (chaos->figures & FIGURE_BODY) {
            if (chaos->motion.state != MOTION_BLENDING) {
                action = chaos->motion.actions[current].action;
            } else {
                action = chaos->motion.blend;
            }
            draw_action(action, chaos->motion.frame);
            draw_tree(&dat_0c9e49d0);
        }
        if (chaos->figures & FIGURE_FORM) {
            if (form_frame < FORM_FADE) {
                draw_control_set(NJD_CONTROL_3D_CONSTANT_MATERIAL);
                draw_set_material((form_frame - FORM_FADE) * (1.0F / FORM_FADE),
                                  1.0F, 1.0F, 1.0F);
            }
            draw_shape_motion(&form_object, &form_motion, &form_shape,
                              form_frame);
        }
        njPopMatrix(1);
    }
    njPopMatrix(1);
    draw_control_restore();
    draw_attr_restore();
    njSetMotionCallback(NULL);
    draw_set_palette(0);
    if (chaos_distance_across(&work->pos, &player_work[0]->pos) < NEAR_RANGE) {
        sound_play_for(SE_NEAR, (int)work, 1, 0, 10);
    }
}

void chaos7_set_mode(int mode)
{
    struct task_work *work = boss_work;

    if (mode != work->mode) {
        boss_next_mode = mode;
        boss_last_mode = work->mode;
    }
}

void fun_0c916df8(struct task *task)
{
    struct task_work *work = task->work;

    if (work->submode != MODE_DONE) {
        work->submode = MODE_DONE;
        boss_step_mode(task);
    }
    work->mode = BOSS_HIT;
    work->submode = 0;
}

void chaos7_knock_player_back(void)
{
    NJS_VECTOR speed;
    Angle heading = -boss_work->ang.y + 0x8000;

    speed.x = njCos(heading) * KNOCK_SPEED;
    speed.z = njSin(heading) * KNOCK_SPEED;
    player_knock_back(0, speed.x, KNOCK_RISE, speed.z);
    chaos7_fade_start(180, 0, 255, 60, 90);
    if (chaos7_part_2) {
        if (chaos_record->attack_timer < ATTACK_SOON) {
            chaos_record->attack_timer = ATTACK_LATER;
        }
    }
}
