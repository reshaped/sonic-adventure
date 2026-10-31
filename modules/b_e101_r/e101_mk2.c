/* E-101 mkII, the boss E-102 fights in B_E101_R. it circles near player 0,
 * then attacks. it leaps, warps or dashes, or fires missiles or its beam. its
 * health picks the row of attack_rows its attacks come from. a hit hurts it
 * only after a dash, as it slows. at other times a hovering boss swats
 * E-102's shot away, or leaps from it */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "b_e101_r/beam.h"
#include "b_e101_r/camera.h"
#include "b_e101_r/e101_mk2.h"
#include "b_e101_r/effects.h"
#include "b_e101_r/files.h"
#include "b_e101_r/glow.h"
#include "b_e101_r/missile.h"
#include "b_e101_r/trail.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "draw/draw.h"
#include "draw/motion.h"
#include "draw/pose.h"
#include "enemy/enemy.h"
#include "event/script.h"
#include "hud/boss_name.h"
#include "hud/hud.h"
#include "hud/life_gauge.h"
#include "object/tikal.h"
#include "player/e102_hit.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/math.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 26
#include "b_e101_r/e101_mk2/text.inc"

#define E101_MK2(task) ((struct e101_mk2 *)(task)->extra)

/* the submode that ends a mode. a mode's step sets it once the mode is
 * done. a new mode can cut a mode short. e101_mk2_step then sets it, and
 * runs the mode's step once more. a mode's step that ends with it set runs
 * the mode's ending, if the mode has one */
#define MODE_DONE (-1)

/* change to mode next at the end of this step, unless it is the mode now.
 * last_mode keeps the mode it leaves. a later call in the same step
 * replaces an earlier one */
#define SET_MODE(task, next)                                                   \
    do {                                                                       \
        if ((next) != (task)->work->mode) {                                    \
            E101_MK2(task)->next_mode = (next);                                \
            E101_MK2(task)->last_mode = (Sint32)(task)->work->mode;            \
        }                                                                      \
    } while (0)

/* ask the boss's motion for entry, unless entry plays now */
#define PLAY(boss, entry)                                                      \
    do {                                                                       \
        if ((boss)->motion.current != (entry)) {                               \
            (boss)->motion.wanted = (entry);                                   \
        }                                                                      \
    } while (0)

/* the entries of e101_mk2_actions, by the actions they play */
enum e101_mk2_action {
    ACTION_HOVER,
    ACTION_DASH,
    ACTION_DASH_END,
    ACTION_BEAM,
    ACTION_SWAT_ARM1,
    ACTION_SWAT_ARM0,
    ACTION_HURT,
    ACTION_HURT_DASHING,
    ACTION_DEFEAT
};

/* the nodes of the boss's object tree, and those whose matrices
 * e101_mk2_pose reads */
#define FIGURE_NODES 65
#define NODE_BODY 8
#define NODE_BACK 14
#define NODE_ARM0_HAND 40
#define NODE_ARM1_HAND 51
#define NODE_JET0 24
#define NODE_JET1 28
/* e101_mk2_pose gets this node's matrix, and nothing reads it */
#define NODE_18 18

/* the boss's shapes. shape 0 sits at its position. SHAPE_ON_BODY and the two
 * after it sit on the nodes of its body and its hands */
#define SHAPES 4
#define SHAPE_ON_BODY 1
#define SHAPE_ON_ARM0_HAND 2
#define SHAPE_ON_ARM1_HAND 3

/* shape 0's guard while E-102 can lock on to the boss. E-102's aim passes
 * over a target whose first shape has the full guard, SHAPE_GUARD */
#define LOCKABLE_GUARD GUARD(2)

/* hover_y, the height the boss hovers at, is HOVER_HEIGHT above the floor
 * its shadow finds. it starts at MIN_HOVER_Y, 20 above the floor's middle,
 * and never drops below it */
#define HOVER_HEIGHT 30.0F
#define MIN_HOVER_Y 772.09F
/* player 0 at this height or below falls out of the stage */
#define FALL_Y 393.47635F

/* the most its y angle turns in a step. turn_smoothly's spring turns it up
 * to TURN_SPRING_MOST. it turns up to TURN_FAST as the boss leaps or winds
 * up a dash. it turns up to TURN_SLOW as the boss takes aim for one. in the
 * dash itself, it turns up to TURN_DASH */
#define TURN_SPRING_MOST NJM_DEG_ANG(6)
#define TURN_FAST NJM_DEG_ANG(8)
#define TURN_SLOW NJM_DEG_ANG(2)
#define TURN_DASH NJM_DEG_ANG(0.5)
/* the missiles and the dash wait for the camera to come within READY_TURN
 * of its aim */
#define READY_TURN NJM_DEG_ANG(30)
/* each wobble phase turns WOBBLE_STEP a step. the pitch's turns
 * WOBBLE_CHARGE while the beam charges */
#define WOBBLE_STEP NJM_DEG_ANG(2)
#define WOBBLE_CHARGE NJM_DEG_ANG(100)

#define SE_BOSS_DASH 0x1fd
#define SE_MISSILE 0x1fe
#define SE_HURT 0x200
#define SE_CRACKLE 0x201
#define SE_CHARGE 0x204
#define SE_LEAP 0x206
#define SE_WIND_UP 0x207
#define SE_SWAT 0x208
#define SE_EXPLOSION 26

/* Tikal's voice for the hint */
#define HINT_VOICE 0x336
/* the timer's reading as one count. a minute counts 6000, and a second 100.
 * its frames, from 0 to 59, count 1 each */
#define TIMER_COUNT(minutes, seconds, frames)                                  \
    (((minutes) * 6000) + ((seconds) * 100) + (frames))
/* Tikal's hint comes once the timer's count has changed by more than this
 * since the fight began */
#define HINT_TIME TIMER_COUNT(1, 0, 0)

/* the field of view the intro sets for the fight */
#define BOSS_PERSPECTIVE NJM_DEG_ANG(55)

/* a hover's steps before the boss picks its next attack */
#define ATTACK_WAIT 200
/* a hovering boss steers toward a target ORBIT_RADIUS from player 0. it
 * stops steering within TARGET_REACH of the target */
#define ORBIT_RADIUS 100.0F
#define TARGET_REACH 2.0F
/* reaches in x and z. a hovering boss leaps from a shot within DODGE_REACH.
 * a swat swings at SWING_REACH, and knocks the shot away at SWAT_REACH. a
 * dash swats a shot at DASH_SWAT_REACH */
#define DODGE_REACH 30.0F
#define SWING_REACH 100.0F
#define SWAT_REACH 6.0F
#define DASH_SWAT_REACH 35.0F
/* a leap lands LEAP_REACH past the floor's middle from player 0. a warp
 * lands WARP_BACK from the floor's middle, back along the camera's heading,
 * and WARP_UP above it */
#define LEAP_REACH 140.0F
#define WARP_BACK 380.0F
#define WARP_UP 180.0F
/* the steps a dash takes to reach aim */
#define DASH_STEPS 34.0F
/* the steps between missiles */
#define MISSILE_GAP 10
/* the boss tilts TILT_MOST at a speed of TILT_SPEED or more */
#define TILT_MOST NJM_DEG_ANG(18)
#define TILT_SPEED 3.0F
/* a bob rising through 0 slower than this a step speeds up to it */
#define BOB_LEAST_RISE 0.1F

/* what each hit the boss has taken adds to damage */
#define DAMAGE_PER_HIT 30.0F
/* E101_MK2_BEAMS's shots. each shot's blast is BLAST_SIZE, but the last
 * shot's is LAST_BLAST_SIZE plus damage */
#define BEAMS_SHOTS 7
#define BLAST_SIZE 60.0F
#define LAST_BLAST_SIZE 40.0F

/* with 1 or less health, the boss crackles every CRACKLE_INTERVAL steps, at
 * a chance of CRACKLE_CHANCE */
#define CRACKLE_INTERVAL 90
#define CRACKLE_CHANCE 0.6F

/* the middle of the floor the fight is on, and the boss's start */
static NJS_POINT3 floor_middle = {0.0F, 752.09F, -385.89F};
static NJS_POINT3 start_pos = {0.0F, 752.09F, -385.89F};

/* TRUE once Tikal's hint has been given, or once a hit has made it too
 * late */
static Bool hint_done = FALSE;

static struct collision_shape e101_mk2_shapes[SHAPES] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE | LOCKON_NO_NUMBER,
     SHAPE_HURTS_OBJECTS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_PLAYERS |
         LOCKABLE_GUARD,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     12.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_OBJECTS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_PLAYERS |
         LOCKABLE_GUARD,
     PUSH_PLAYERS,
     {0.0F, 0.0F, 0.0F},
     10.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_OBJECTS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_PLAYERS |
         LOCKABLE_GUARD,
     PUSH_PLAYERS,
     {0.0F, 0.0F, 0.0F},
     15.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_OBJECTS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_PLAYERS |
         LOCKABLE_GUARD,
     PUSH_PLAYERS,
     {0.0F, 0.0F, 0.0F},
     15.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

static struct motion_entry e101_mk2_actions[] = {
    {&e101_mk2_hover_action, FIGURE_NODES, MOTION_LOOP, ACTION_HOVER, 0.125F,
     1.0F},
    {&e101_mk2_dash_action, FIGURE_NODES, MOTION_HOLD, ACTION_HOVER, 0.0625F,
     1.0F},
    {&e101_mk2_dash_end_action, FIGURE_NODES, MOTION_ONCE, ACTION_HOVER, 1.0F,
     1.0F},
    {&e101_mk2_beam_action, FIGURE_NODES, MOTION_ONCE, ACTION_HOVER, 0.0625F,
     0.5F},
    {&e101_mk2_swat1_action, FIGURE_NODES, MOTION_ONCE, ACTION_HOVER, 1.0F,
     1.0F},
    {&e101_mk2_swat0_action, FIGURE_NODES, MOTION_ONCE, ACTION_HOVER, 1.0F,
     1.0F},
    {&e101_mk2_hurt_action, FIGURE_NODES, MOTION_HOLD, ACTION_HOVER, 1.0F,
     0.5F},
    {&e101_mk2_hurt_dashing_action, FIGURE_NODES, MOTION_HOLD, ACTION_HOVER,
     1.0F, 0.5F},
    {&e101_mk2_defeat_action, FIGURE_NODES, MOTION_LOOP, ACTION_DEFEAT, 0.0625F,
     0.5F},
};

/* the attacks the boss makes in turn, from a row for each health.
 * e101_mk2_next_attack sets attack_index back to 0 as it picks
 * E101_MK2_HOVER, so the row starts again */
static int attack_rows[E101_MK2_HEALTH + 1][8] = {
    {E101_MK2_HOVER, E101_MK2_HOVER, E101_MK2_HOVER, E101_MK2_HOVER,
     E101_MK2_HOVER, E101_MK2_HOVER, E101_MK2_HOVER, E101_MK2_HOVER},
    {E101_MK2_LEAP, E101_MK2_WARP, E101_MK2_BEAMS, E101_MK2_LEAP,
     E101_MK2_MISSILES, E101_MK2_LEAP, E101_MK2_DASH, E101_MK2_HOVER},
    {E101_MK2_LEAP, E101_MK2_MISSILES, E101_MK2_WARP, E101_MK2_BEAM,
     E101_MK2_LEAP, E101_MK2_DASH, E101_MK2_HOVER, E101_MK2_HOVER},
    {E101_MK2_LEAP, E101_MK2_MISSILES, E101_MK2_DASH, E101_MK2_HOVER,
     E101_MK2_HOVER, E101_MK2_HOVER, E101_MK2_HOVER, E101_MK2_HOVER},
    {E101_MK2_LEAP, E101_MK2_DASH, E101_MK2_HOVER, E101_MK2_HOVER,
     E101_MK2_HOVER, E101_MK2_HOVER, E101_MK2_HOVER, E101_MK2_HOVER},
};

/* a row of missile_rows: the chance that a hover's next attack is missiles,
 * and how many it fires */
struct missile_row {
    Float chance;
    Float count;
};

/* a row for each health, from 0 to E101_MK2_HEALTH */
static struct missile_row missile_rows[E101_MK2_HEALTH + 1] = {
    {1.0F, 8.0F}, {0.8F, 8.0F}, {0.6F, 6.0F}, {0.5F, 4.0F}, {0.0F, 0.0F},
};

/* a point off each hand: the beam charges halfway between the two */
static NJS_POINT3 cannon_offset = {0.0F, -15.0F, 0.0F};

static void e101_mk2_intro(struct task *task);
static void e101_mk2_hover(struct task *task);
static void e101_mk2_leap(struct task *task);
static void e101_mk2_warp(struct task *task);
static void e101_mk2_swat(struct task *task);
static void e101_mk2_fire_missiles(struct task *task);
static void e101_mk2_dash(struct task *task);
static void e101_mk2_fire_beam(struct task *task);
static void e101_mk2_hurt(struct task *task);
static void e101_mk2_defeat(struct task *task);
static void e101_mk2_pause(struct task *task);

/* each mode's step, by enum e101_mk2_mode */
static void (*e101_mk2_modes[])(struct task *task) = {
    e101_mk2_intro, e101_mk2_hover,     e101_mk2_leap,
    e101_mk2_warp,  e101_mk2_swat,      e101_mk2_fire_missiles,
    e101_mk2_dash,  e101_mk2_fire_beam, e101_mk2_fire_beam,
    e101_mk2_hurt,  e101_mk2_defeat,    e101_mk2_pause,
};

/* the boss's record, which e101_mk2_sway_node reads */
static struct e101_mk2 *e101_mk2_record;
/* the timer's count as the fight began, by TIMER_COUNT */
static int start_time;

/* E-101 mkII's name: E101 and a {, which the boss name's font draws as mkII */
static const char boss_name[6] = {
#include "b_e101_r/e101_mk2/boss_name.inc"
};

static const struct script_page hint_pages[2] = {
    {TEXT_1, 100},
    {NULL, 0},
};

/* where the flames start off each jet's node, the same for both. jet_dir is
 * the way they go */
static const NJS_POINT3 jet_offsets[2] = {{1.3F, 0.0F, 0.0F},
                                          {1.3F, 0.0F, 0.0F}};
static const NJS_VECTOR jet_dir = {1.0F, 0.0F, 0.0F};
/* where the thrust starts off its back's node, and the way it goes */
static const NJS_POINT3 thrust_offset = {0.0F, -2.0F, 0.0F};
static const NJS_VECTOR thrust_dir = {0.0F, 1.0F, 0.0F};

static void e101_mk2_step(struct task *task);
static void e101_mk2_draw(struct task *task);
static void e101_mk2_destroy(struct task *task);

void e101_mk2_create(void)
{
    struct task *task;
    struct task_work *work;
    struct e101_mk2 *boss;

    if ((task = task_create(TASK_WORK, 2, e101_mk2_step)) != NULL) {
        work = task->work;
        collision_init(task, e101_mk2_shapes, COLLISION_ITEM, 1, SHAPES);
        enemy_alloc_record(task, work);
        boss = (struct e101_mk2 *)motion_create(work, sizeof(struct e101_mk2));
        boss->motion.wanted = ACTION_HOVER;
        boss->motion.current = ACTION_HOVER;
        boss->motion.actions = e101_mk2_actions;
        e101_mk2_record = task->extra = boss;
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        boss->last_mode = boss->next_mode = work->mode = E101_MK2_INTRO;
        motion_step(work, &boss->motion);
        boss_name_create(boss_name, -1, 240, 120);
        task->step = e101_mk2_step;
        task->draw = e101_mk2_draw;
        task->destructor = e101_mk2_destroy;
        e101_mk2_swat_sparks_init();
        e101_mk2_glow_init();
        e101_mk2_beam_init();
        e101_mk2_beam_sparks_init();
    }
}

static void e101_mk2_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;

    e101_mk2_swat_sparks_clear();
    e101_mk2_glow_clear();
    e101_mk2_beam_clear();
    e101_mk2_beam_sparks_clear();
    e101_mk2_jets_destroy();
    boss->motion.state = MOTION_FREE;
    motion_step(work, &boss->motion);
    tikal_hint_end();
}

/* take a hit, unless the boss is swatting. an unguarded hit takes 1 from its
 * health and sends it to E101_MK2_HURT, or at 0 to E101_MK2_DEFEAT. a
 * guarded hit lets the shot fly on, except in a dash, which swats shots
 * itself */
static void e101_mk2_take_hit(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *target = LOCKON_TARGET(task);
    struct e101_mk2 *boss = task->extra;
    struct task *shot;
    NJS_POINT3 pos;

    if ((work->flags & COLLISION_DAMAGED) && work->mode != E101_MK2_SWAT) {
        if (boss->flags & E101_MK2_GUARDED) {
            if (work->mode == E101_MK2_DASH) {
                return;
            } else {
                lockon_release_hit(task);
            }
        } else {
            if ((shot = lockon_get_shot(target->lockon)) != NULL) {
                VECTOR_COPY(&pos, &shot->work->pos);
            } else {
                VECTOR_COPY(&pos, &work->pos);
            }
            lockon_release(task);
            e102_hit_create(&pos, -15.0F);
            if (work->mode != E101_MK2_HURT) {
                boss->flags |= E101_MK2_GUARDED;
                boss->health -= 1.0F;
                boss->health_units = (int)boss->health;
                if (boss->health <= 0.0F) {
                    boss->health = 0.0F;
                    SET_MODE(task, E101_MK2_DEFEAT);
                    return;
                } else {
                    SET_MODE(task, E101_MK2_HURT);
                }
            }
        }
    }
}

/* give Tikal's hint once the timer's count has changed by more than
 * HINT_TIME since the fight began. E-102's timer counts down, so the change
 * counts either way. the hint needs the stage to have counted a miss, a
 * retry or a restart since it loaded. it never comes once the boss has been
 * hit */
static void e101_mk2_hint(struct e101_mk2 *boss)
{
    Uint8 minutes;
    Uint8 seconds;
    Uint8 frames;
    int time;

    if (!hint_done) {
        if (boss->health != E101_MK2_HEALTH) {
            hint_done = TRUE;
        } else if (stage_get_retries() > 0) {
            time_get(&minutes, &seconds, &frames);
            time = TIMER_COUNT(minutes, seconds, frames);
            if ((time - start_time < 0 ? start_time - time
                                       : time - start_time) > HINT_TIME) {
                tikal_hint_create((struct script_page *)hint_pages, HINT_VOICE);
                hint_done = TRUE;
            }
        }
    }
}

/* ask for the boss's next attack, the next in its row of attack_rows. after
 * a dodge, ask for E101_MK2_HOVER instead. either way, set damage for the
 * beam */
static void e101_mk2_next_attack(struct task *task)
{
    struct e101_mk2 *boss = task->extra;
    struct task_work *work = task->work;
    int mode;

    if (boss->flags & E101_MK2_DODGED) {
        mode = E101_MK2_HOVER;
        boss->attack_index = 0;
    } else {
        mode = attack_rows[boss->health_units][boss->attack_index++];
    }
    if (mode == E101_MK2_HOVER) {
        boss->attack_index = 0;
    }
    /* @hack redundant: the test of the mode ahead of SET_MODE's own, CMP/EQ
     * R6,R3 at 0x0c9005c2; without it, 55 of 59 words differ */
    if (work->mode != mode) {
        SET_MODE(task, mode);
    }
    boss->damage = (E101_MK2_HEALTH - boss->health) * DAMAGE_PER_HIT;
}

static void e101_mk2_set_missiles(struct task *task)
{
    struct e101_mk2 *boss = task->extra;

    boss->missile_chance = missile_rows[boss->health_units].chance;
    boss->missiles = (int)missile_rows[boss->health_units].count;
}

/* turn *ang toward aim on a spring. *speed gains accel times the turn left,
 * then is scaled by damping and held within most. *ang turns by *speed. within
 * half a degree of aim, a *speed under a tenth of a degree stops *ang there */
static void turn_spring(Angle aim, Angle *ang, Angle *speed, Angle most,
                        Float accel, Float damping)
{
    Angle change = (Angle)(angle_difference(*ang, aim) * accel);

    *speed += change;
    *speed *= damping;
    *speed = CLAMP(*speed, -most, most);
    *ang += *speed;
    if ((*ang - aim < 0 ? aim - *ang : *ang - aim) < NJM_DEG_ANG(0.5)) {
        /* @hack type: *speed read through a volatile pointer, which
         * loads it again for the test and for each arm of the ?:, MOV.L @R14
         * at 0x0c9006dc, 0x0c9006e4 and 0x0c9006ea; without it, 35 of 82
         * words differ */
        if ((*(volatile Angle *)speed < 0
                 ? -*(volatile Angle *)speed
                 : *(volatile Angle *)speed) < NJM_DEG_ANG(0.1)) {
            *speed = 0;
            *ang = aim;
        }
    }
}

/* the heading of pos seen from origin, in x and z */
static Angle angle_from(NJS_POINT3 *pos, NJS_POINT3 *origin)
{
    Float dx = pos->x - origin->x;
    Float dz = pos->z - origin->z;

    return angle_arc_tan2(dz, dx);
}

/* turn its y angle toward aim_y by no more than most, unless
 * E101_MK2_STILL holds it */
static void turn_toward(struct task *task, Angle most)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;
    Angle diff;

    if (!(boss->flags & E101_MK2_STILL)) {
        diff = angle_difference(work->ang.y, boss->aim_y);
        work->ang.y += CLAMP(diff, -most, most);
    }
}

/* turn its y angle toward aim_y on turn_spring's spring, unless
 * E101_MK2_STILL holds it */
static void turn_smoothly(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;

    if (!(boss->flags & E101_MK2_STILL)) {
        turn_spring(boss->aim_y, &work->ang.y, &boss->turn_speed,
                    TURN_SPRING_MOST, 0.06F, 0.85F);
    }
}

static void shape_on_node(struct task_work *work, NJS_MATRIX *matrix, int shape)
{
    njSetMatrix(NULL, &_unit_matrix_);
    njMultiMatrix(NULL, matrix);
    njGetTranslation(NULL, &work->collision->shapes[shape].center);
}

/* each shapes_ function below changes the three shapes on the boss's
 * nodes, from SHAPE_ON_BODY on. shape 0 stays as it is */

static void shapes_on(struct task_work *work)
{
    unsigned int i;

    for (i = SHAPE_ON_BODY; i < SHAPES; ++i) {
        work->collision->shapes[i].flags &= ~SHAPE_OFF;
    }
}

static void shapes_off(struct task_work *work)
{
    unsigned int i;

    for (i = SHAPE_ON_BODY; i < SHAPES; ++i) {
        work->collision->shapes[i].flags |= SHAPE_OFF;
    }
}

static void shapes_attack(struct task_work *work)
{
    unsigned int i;

    for (i = SHAPE_ON_BODY; i < SHAPES; ++i) {
        work->collision->shapes[i].levels =
            (work->collision->shapes[i].levels & ~SHAPE_ATTACK) | SHAPE_ATTACK;
    }
}

static void shapes_harmless(struct task_work *work)
{
    unsigned int i;

    for (i = SHAPE_ON_BODY; i < SHAPES; ++i) {
        work->collision->shapes[i].levels =
            work->collision->shapes[i].levels & ~SHAPE_ATTACK;
    }
}

static void shapes_push(struct task_work *work)
{
    unsigned int i;

    for (i = SHAPE_ON_BODY; i < SHAPES; ++i) {
        work->collision->shapes[i].push |= PUSH_PLAYERS;
        work->collision->shapes[i].push &= ~PUSH_FIRM(PUSH_PLAYERS);
    }
}

static void shapes_firm(struct task_work *work)
{
    unsigned int i;

    for (i = SHAPE_ON_BODY; i < SHAPES; ++i) {
        work->collision->shapes[i].push &= ~PUSH_PLAYERS;
        work->collision->shapes[i].push |= PUSH_FIRM(PUSH_PLAYERS);
    }
}

/* the shot flying at the boss's lock-on, or NULL for none */
static struct task *incoming_shot(struct task *task)
{
    struct lockon_target *target = LOCKON_TARGET(task);

    if (target->lockon == NULL) {
        return NULL;
    }
    return lockon_get_shot(target->lockon);
}

/* TRUE while a shot flies at the boss's lock-on, and FALSE for none. *pos
 * gets its position unless pos is NULL */
static Bool find_shot(struct task *task, NJS_POINT3 *pos)
{
    struct task *shot = incoming_shot(task);
    struct task_work *work;

    if (shot == NULL) {
        return FALSE;
    }
    if (pos != NULL) {
        work = shot->work;
        VECTOR_COPY(pos, &work->pos);
    }
    return TRUE;
}

/* side_of's results */
#define SIDE_ARM1 1
#define SIDE_ARM0 2

/* the arm on pos's side of the boss, in x and z */
static int side_of(struct task *task, NJS_POINT3 *pos)
{
    struct task_work *work = task->work;
    Float sine;
    Float cosine;
    NJS_VECTOR side; /* a quarter turn from its y angle */
    NJS_VECTOR off;

    _builtin_fsca(work->ang.y, &sine, &cosine);
    side.x = -sine;
    side.z = cosine;
    /* @hack spelling: off through its address, which copies R15 to R3 and adds
     * 8 for each store, at 0x0c900a72 and 0x0c900a90; without it, 44 of 53
     * words differ */
    (&off)->x = work->pos.x - pos->x;
    (&off)->z = work->pos.z - pos->z;
    if (vector_inner_product_across(&side, &off) > 0.0F) {
        return SIDE_ARM0;
    }
    return SIDE_ARM1;
}

/* knock the shot flying at the boss away from it, at 8 a step and never
 * downward */
static void swat_shot(struct task *task)
{
    struct task *shot = incoming_shot(task);
    struct task_work *work;
    struct task_motion *motion;
    NJS_VECTOR speed;

    if (shot != NULL) {
        work = shot->work;
        motion = shot->motion;
        e101_mk2_swat_sparks(&work->pos);
        VECTOR_DIFF(&speed, &work->pos, &task->work->pos);
        vector_unit(&speed, &speed);
        VECTOR_SCALE(&speed, &speed, 8.0F);
        if (speed.y < 0.0F) {
            speed.y = -speed.y;
        }
        VECTOR_COPY(&motion->speed, &speed);
        sound_play(SE_SWAT, 0, 0, 0);
        lockon_release_hit(task);
    }
}

/* fire the beam the way the boss faces, back along its y angle. it aims at
 * player 0's height. its aim is as far from the cannon in x and z as player 0
 * is. the blast it sets off has size */
static void shoot_beam(struct task_work *work, struct e101_mk2 *boss,
                       Float size)
{
    NJS_POINT3 *player = &player_get_task(0)->work->pos;
    NJS_VECTOR dir;
    Float reach;

    _builtin_fsca(work->ang.y, &dir.z, &dir.x);
    reach = point_distance_across(player, &boss->cannon);
    /* @hack spelling: dir through its address, which copies R15 to R3 or R2 and
     * adds 8 for each read and store, as at 0x0c900c36 and 0x0c900c3c;
     * without it, 45 of 75 words differ */
    (&dir)->x = -((&dir)->x * reach);
    (&dir)->z = -((&dir)->z * reach);
    dir.y = player->y - boss->cannon.y;
    vector_unit(&dir, &dir);
    e101_mk2_beam_aim(boss->beam, &dir, size);
    e101_mk2_beam_set_state(boss->beam, BEAM_FIRE);
}

/* pose the boss's action to find its nodes. it places the shapes and the
 * jets. it places the cannon in a beam mode, and the thrust in a dash. it also
 * keeps the pose for the copies, and each hand's matrix for its trail */
static void e101_mk2_pose(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;
    NJS_ACTION *action;
    NJS_MATRIX node_18;
    NJS_MATRIX hand0;
    NJS_MATRIX hand1;
    NJS_MATRIX body;
    NJS_MATRIX jets[2];
    NJS_MATRIX back;
    NJS_POINT3 cannon0;
    NJS_POINT3 cannon1;
    int i;
    Angle ang;

    njPushMatrix(&_unit_matrix_);
    if (boss->motion.state != MOTION_BLENDING) {
        action = boss->motion.actions[boss->motion.current].action;
    } else {
        action = boss->motion.blend;
    }
    njTranslate(NULL, work->pos.x, work->pos.y + boss->bob, work->pos.z);
    TURN_VIA(njRotateZ, boss->roll, ang);
    TURN_VIA(njRotateX, boss->pitch, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateY, -work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    njScale(NULL, 1.0F, 1.0F, 1.0F);
    pose_action(NULL, action, boss->motion.frame, NODE_ARM1_HAND, &hand1);
    pose_get_matrix(NODE_ARM0_HAND, &hand0);
    pose_get_matrix(NODE_18, &node_18);
    pose_get_matrix(NODE_BODY, &body);
    pose_get_matrix(NODE_JET0, &jets[0]);
    pose_get_matrix(NODE_JET1, &jets[1]);
    pose_get_matrix(NODE_BACK, &back);
    if (boss->flags & E101_MK2_WARPING) {
        e101_mk2_pose_add_warping(task);
    } else {
        e101_mk2_pose_add(task);
    }
    e101_mk2_trail_add(boss, &hand1, 1);
    e101_mk2_trail_add(boss, &hand0, 0);
    shape_on_node(work, &body, SHAPE_ON_BODY);
    shape_on_node(work, &hand1, SHAPE_ON_ARM1_HAND);
    shape_on_node(work, &hand0, SHAPE_ON_ARM0_HAND);
    if (work->mode == E101_MK2_BEAM || work->mode == E101_MK2_BEAMS) {
        njSetMatrix(NULL, &_unit_matrix_);
        njMultiMatrix(NULL, &hand0);
        njCalcPoint(NULL, &cannon_offset, &cannon0);
        njSetMatrix(NULL, &_unit_matrix_);
        njMultiMatrix(NULL, &hand1);
        njCalcPoint(NULL, &cannon_offset, &cannon1);
        boss->cannon.x = (cannon0.x + cannon1.x) * 0.5F;
        boss->cannon.y = (cannon0.y + cannon1.y) * 0.5F;
        boss->cannon.z = (cannon0.z + cannon1.z) * 0.5F;
    }
    for (i = 0; i < 2; i++) {
        njSetMatrix(NULL, &_unit_matrix_);
        njMultiMatrix(NULL, &jets[i]);
        njCalcPoint(NULL, (NJS_POINT3 *)&jet_offsets[i], &boss->jets[i]);
        njCalcVector(NULL, (NJS_VECTOR *)&jet_dir, &boss->jet_dirs[i]);
    }
    if (work->mode == E101_MK2_DASH) {
        njSetMatrix(NULL, &_unit_matrix_);
        njMultiMatrix(NULL, &back);
        njCalcPoint(NULL, (NJS_POINT3 *)&thrust_offset, &boss->thrust_pos);
        njCalcVector(NULL, (NJS_VECTOR *)&thrust_dir, &boss->thrust_dir);
    }
    njPopMatrix(1);
}

/* set aim, aim_y, orbit_aim, player_speed and target_distance2. pass the
 * camera aim_y and player 0's speed */
static void track_player(struct task *task)
{
    Angle aim;
    Float speed;
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;
    struct task_work *player = player_get_task(0)->work;
    NJS_VECTOR *player_speed = &player_motion[0]->speed;

    VECTOR_COPY(&boss->aim, &player_get_task(0)->work->pos);
    boss->aim.x += player_speed->x * boss->lead;
    boss->aim.z += player_speed->z * boss->lead;
    if (boss->flags & E101_MK2_LEADS) {
        boss->aim_y = angle_from(&work->pos, &boss->aim);
    } else {
        boss->aim_y = angle_from(&work->pos, &player->pos);
    }
    boss->orbit_aim = angle_from(&floor_middle, &player->pos);
    boss->player_speed = vector_magnitude_across(&player_motion[0]->speed);
    /* @hack spelling: aim and speed, which make SHC read aim_y and player_speed
     * back from the record for the call, at 0x0c90110e and 0x0c901114; with the
     * call reading the two fields itself, 19 of 104 words differ */
    aim = boss->aim_y;
    speed = boss->player_speed;
    e101_r_camera_set_aim(aim, speed);
    boss->target_distance2 =
        point_distance_squared_across(&work->pos, &boss->target);
}

/* start orbit at the boss's heading seen from player 0. orbit_turn turns it
 * a tenth of a degree a step, either way at random */
static void orbit_start(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;
    struct task_work *player = player_get_task(0)->work;

    boss->orbit = angle_from(&work->pos, &player->pos);
    boss->orbit_turn = (rand() & 1) ? NJM_DEG_ANG(0.1) : -NJM_DEG_ANG(0.1);
}

/* while E101_MK2_SWAYS holds, lean its upper body out of the turn left to
 * aim_y. otherwise let it come back. grow or shrink the copies and each arm's
 * trail by their flags */
static void sway_step(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;
    Angle diff;
    Float lean;

    if (boss->flags & E101_MK2_SWAYS) {
        diff = angle_difference(work->ang.y, boss->aim_y);
        diff = CLAMP(diff, -NJM_DEG_ANG(30), NJM_DEG_ANG(30));
        lean = -(diff * (0.0833F * (360.0F / 65536.0F)));
        boss->sway += CLAMP(lean - boss->sway, -0.1F, 0.1F);
    } else {
        boss->sway *= 0.95F;
    }
    e101_mk2_trail_grow(boss, E101_MK2_BLUR | E101_MK2_WARPING,
                        &boss->blur_copies);
    e101_mk2_trail_grow(boss, E101_MK2_TRAIL_ARM0, &boss->trail_copies[0]);
    e101_mk2_trail_grow(boss, E101_MK2_TRAIL_ARM1, &boss->trail_copies[1]);
}

/* set the warp's destination WARP_BACK from the floor's middle, back along
 * the camera's heading, and WARP_UP above it */
static void set_warp_destination(struct task *task)
{
    struct e101_mk2 *boss = task->extra;
    Float sine;
    Float cosine;

    _builtin_fsca(e101_r_camera_get_heading(), &sine, &cosine);
    boss->destination.x = floor_middle.x - (cosine * WARP_BACK);
    boss->destination.z = floor_middle.z - (sine * WARP_BACK);
    boss->destination.y = floor_middle.y + WARP_UP;
}

/* set the leap's destination LEAP_REACH past the floor's middle from
 * player 0, at start_pos's height */
static void set_leap_destination(struct task *task)
{
    struct task_work *player = player_work[0];
    struct e101_mk2 *boss = task->extra;
    NJS_VECTOR dir;

    /* @hack spelling: dir through its address, which copies R15 to R3 for each
     * store, as at 0x0c90132c and 0x0c90133e; without it, 74 of 80 words
     * differ */
    /* @hack spelling: floor_middle through its address, which SHC loads into R2
     * for each statement, at 0x0c90132a and 0x0c90138e, rather than once into
     * R14; without it, 34 of 80 words differ */
    (&dir)->x = (&floor_middle)->x - player->pos.x;
    (&dir)->z = (&floor_middle)->z - player->pos.z;
    dir.y = 0.0F;
    if (dir.x == 0.0F && dir.z == 0.0F) {
        dir.x = 1.0F;
    } else {
        vector_unit(&dir, &dir);
    }
    VECTOR_SCALE(&dir, &dir, LEAP_REACH);
    boss->destination.x = (&dir)->x + (&floor_middle)->x;
    boss->destination.z = (&dir)->z + (&floor_middle)->z;
    boss->destination.y = start_pos.y;
}

/* move target round player 0, ORBIT_RADIUS from it, by orbit_turn. once
 * orbit is more than 45 degrees from orbit_aim, orbit_turn turns it back.
 * the turn grows with player 0's speed, and is at least a tenth of a degree
 * a step */
static void orbit_step(struct task *task)
{
    struct e101_mk2 *boss = task->extra;
    struct task_work *player = player_get_task(0)->work;
    Angle diff = angle_difference(boss->orbit, boss->orbit_aim);
    int step;
    Float sine;
    Float cosine;

    if ((diff < 0 ? -diff : diff) > NJM_DEG_ANG(45)) {
        step = (int)(boss->player_speed * 113.75F);
        if (step < NJM_DEG_ANG(0.1)) {
            step = NJM_DEG_ANG(0.1);
        }
        boss->orbit_turn = diff >= 0 ? step : -step;
    }
    boss->orbit += boss->orbit_turn;
    _builtin_fsca(boss->orbit, &sine, &cosine);
    boss->target.x = player->pos.x + (cosine * ORBIT_RADIUS);
    boss->target.z = player->pos.z + (sine * ORBIT_RADIUS);
}

/* bob about its height on a spring, losing bob_damping of bob_speed each
 * step. rising through 0 slower than BOB_LEAST_RISE a step, it speeds up to
 * BOB_LEAST_RISE, and bobs on undamped */
static void bob_step(struct task *task)
{
    struct e101_mk2 *boss = task->extra;
    int above;
    /* @hack spelling: no and yes, which keep 0 in R7 and 1 in R6, MOV #0,R7 at
     * 0x0c901494 and MOV #1,R6 at 0x0c9014ba; as 0 and 1 in their place, 25 of
     * 85 words differ */
    int no = 0;
    int yes = 1;

    boss->bob_speed += -(boss->bob * 0.01F);
    above = boss->bob >= 0.0F ? yes : no;
    /* @hack redundant: a never-run store of above to the flags, which keeps 0
     * and 1 in the image's registers (0x0c901494 to 0x0c9014be); without it,
     * R5, R6 and R7 rotate and 9 of 85 words differ */
    if (0) {
        boss->flags = above;
    }
    boss->bob += boss->bob_speed;
    if (boss->bob >= 0.0F) {
        if (above) {
            goto damp;
        }
        above = no;
    } else {
        if (above) {
            goto damp;
        }
        above = yes;
    }
    if (!above) {
        if ((boss->bob_speed < 0.0F ? -boss->bob_speed : boss->bob_speed) <
            BOB_LEAST_RISE) {
            boss->bob_speed =
                boss->bob_speed > 0.0F ? BOB_LEAST_RISE : -BOB_LEAST_RISE;
            boss->bob_damping = 0.0F;
        }
    }
damp:
    boss->bob_speed -= boss->bob_speed * boss->bob_damping;
}

/* while E101_MK2_TILTS holds, ease pitch and roll toward a tilt by its
 * velocity, plus its wobble. its velocity in z pitches it, and in x rolls it.
 * otherwise they come back to level. either way, it then bobs */
static void tilt_step(struct task *task)
{
    struct e101_mk2 *boss = task->extra;
    Angle pitch;
    Angle roll;
    Float sine;
    Float cosine;
    Float speed;
    Float most;
    Float least;
    Float rate;

    if (boss->flags & E101_MK2_TILTS) {
        speed = boss->velocity.x;
        rate = TILT_MOST * (1.0F / TILT_SPEED);
        least = -TILT_SPEED;
        most = TILT_SPEED;
        /* @hack redundant: a never-run store of most + speed to bob, which
         * keeps the image's loads and registers for most and least
         * (0x0c9015a0 to 0x0c9015ce); without it, FR5, FR6 and FR7 rotate and
         * 9 of 157 words differ */
        if (0) {
            boss->bob = most + speed;
        }
        /* NOLINTBEGIN(readability-avoid-nested-conditional-operator) */
        roll = speed >= most    ? -TILT_MOST
               : speed <= least ? TILT_MOST
                                : (Angle)(-(speed * rate));
        /* NOLINTEND(readability-avoid-nested-conditional-operator) */
        speed = boss->velocity.z;
        /* @hack redundant: a never-run store of speed to bob, which keeps the
         * image's FR6 and FR7 for most and least (0x0c9015a2 to 0x0c9015c0);
         * without it, the two trade and 6 of 157 words differ */
        if (0) {
            boss->bob = speed;
        }
        /* NOLINTBEGIN(readability-avoid-nested-conditional-operator) */
        pitch = speed >= most    ? TILT_MOST
                : speed <= least ? -TILT_MOST
                                 : (Angle)(speed * rate);
        /* NOLINTEND(readability-avoid-nested-conditional-operator) */
        boss->wobble_phase.x += boss->wobble_step.x;
        boss->wobble_phase.z += boss->wobble_step.z;
        _builtin_fsca(boss->wobble_phase.x, &sine, &cosine);
        pitch += (Angle)(boss->wobble * sine * ANGLE_PER_DEGREE);
        _builtin_fsca(boss->wobble_phase.z, &sine, &cosine);
        roll += (Angle)(boss->wobble * sine * ANGLE_PER_DEGREE);
        boss->pitch += (pitch - boss->pitch) * 0.1F;
        boss->roll += (roll - boss->roll) * 0.1F;
    } else {
        boss->pitch = (Angle)(boss->pitch * 0.95F);
        boss->roll = (Angle)(boss->roll * 0.95F);
    }
    bob_step(task);
}

/* bring its y to hover_y. from below, it rises there at once. from above,
 * it falls faster each step, unless E101_MK2_FLOATS holds it where it is */
static void fall(struct task_work *work, struct e101_mk2 *boss)
{
    Float rise = boss->hover_y - work->pos.y;
    Float speed = boss->velocity.y;

    if (rise > 0.0F && rise > speed) {
        speed = rise;
    }
    if (!(boss->flags & E101_MK2_FLOATS)) {
        speed -= 0.05F;
    } else {
        speed = 0.0F;
    }
    boss->velocity.y = speed;
    work->pos.y += speed;
    if (work->pos.y <= boss->hover_y) {
        work->pos.y = boss->hover_y;
        boss->velocity.y = 0.0F;
    }
}

/* save last_pos, move by velocity, slow it in x and z, and fall */
static void move(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;

    VECTOR_COPY(&boss->last_pos, &work->pos);
    /* @hack spelling: the sums spelled out, which read the velocity by
     * FMOV.S @(R0,R5),FR3 at 0x0c9017c0 and 0x0c9017cc; with +=, SHC forms
     * its address in R1 first, the function runs 4 bytes long and 32 of 40
     * words differ */
    work->pos.x = work->pos.x + boss->velocity.x;
    work->pos.z = work->pos.z + boss->velocity.z;
    boss->velocity.x *= 0.96F;
    boss->velocity.z *= 0.96F;
    fall(work, boss);
}

/* move by velocity without slowing it, and fall. last_pos stays */
static void glide(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;

    /* @hack spelling: the sums spelled out, as in move, FMOV.S @(R0,R5),FR3
     * at 0x0c9017f8 and 0x0c901806; with +=, the function runs 4 bytes long
     * and 16 of 18 words differ */
    work->pos.x = work->pos.x + boss->velocity.x;
    work->pos.z = work->pos.z + boss->velocity.z;
    fall(work, boss);
}

/* ease its velocity in x and z toward target, until the boss is within
 * TARGET_REACH of it */
static void steer_to_target(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;
    Float want;

    if (boss->target_distance2 > SQUARE(TARGET_REACH)) {
        want = (boss->target.x - work->pos.x) * 0.06F;
        boss->velocity.x += (want - boss->velocity.x) * 0.04F;
        want = (boss->target.z - work->pos.z) * 0.06F;
        boss->velocity.z += (want - boss->velocity.z) * 0.04F;
    }
}

static void steer_to_destination(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;

    boss->velocity.x = (boss->destination.x - work->pos.x) * 0.06F;
    boss->velocity.z = (boss->destination.z - work->pos.z) * 0.06F;
}

/* set its velocity in x and z to move_speed along its y angle */
static void drive(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;
    Float sine;
    Float cosine;

    _builtin_fsca(work->ang.y, &sine, &cosine);
    boss->velocity.x = cosine * boss->move_speed;
    boss->velocity.z = sine * boss->move_speed;
}

/* set the fight up, then hover still while the camera swings round the
 * boss. the timer and the pad stay stopped until the swing is done. then
 * start the fight: the life gauge, the fight camera and the hover */
static void e101_mk2_intro(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;
    Uint8 minutes;
    Uint8 seconds;
    Uint8 frames;

    camera_set_perspective(BOSS_PERSPECTIVE);
    time_stop();
    pad_disable();
    switch (work->submode) {
    case 0:
        boss->health = E101_MK2_HEALTH;
        boss->health_units = E101_MK2_HEALTH;
        boss->flags = 0;
        boss->steps = 0;
        boss->hover_steps = 0;
        e101_mk2_set_missiles(task);
        e101_mk2_missiles_reset();
        work->pos = start_pos;
        work->ang.x = work->ang.y = work->ang.z = 0;
        boss->target = start_pos;
        boss->destination = start_pos;
        boss->velocity.x = boss->velocity.y = boss->velocity.z = 0.0F;
        boss->pitch = boss->roll = 0;
        boss->last_hover_y = boss->hover_y = MIN_HOVER_Y;
        boss->blur_copies = 0;
        e101_mk2_poses_reset(task);
        e101_mk2_trails_reset(task);
        orbit_start(task);
        boss->bob = 0.0F;
        boss->bob_speed = 0.1F;
        boss->bob_damping = 0.0F;
        boss->wobble = 4.0F;
        boss->wobble_phase.x = 0;
        boss->wobble_phase.z = 0;
        boss->wobble_step.x = WOBBLE_STEP;
        boss->wobble_step.z = WOBBLE_STEP;
        shapes_on(work);
        shapes_harmless(work);
        time_get(&minutes, &seconds, &frames);
        start_time = TIMER_COUNT(minutes, seconds, frames);
        boss->flags |= E101_MK2_STILL;
        camera_start_swing(work, 30.0F, 40.0F, 240);
        camera_override_mode(CAMERA_MODE_CHAOS_STINIT, CAMERA_ADJUST_NONE);
        PLAY(boss, ACTION_HOVER);
        e101_mk2_jets_create(boss->jets, boss->jet_dirs);
        work->submode = 1;
        break;
    case 1:
        if (camera_swing_done) {
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        boss->flags &= ~E101_MK2_STILL;
        time_start();
        pad_enable();
        life_gauge_create(0, 0, (int)boss->health);
        e101_r_camera_reset();
        e101_r_camera_start(&work->pos);
        camera_override_mode(CAMERA_MODE_E101R, CAMERA_ADJUST_NONE);
        e101_r_camera_set_mode(E101_R_CAMERA_FOLLOW);
        SET_MODE(task, E101_MK2_HOVER);
    }
    move(task);
}

/* circle player 0, open to E-102's lock-on. every ATTACK_WAIT steps, pick
 * the next attack: missiles, by missile_chance, or the next in its row. a
 * shot flying at it asks for E101_MK2_SWAT, or half the time raises the
 * full guard. under the guard, it leaps once the shot is within
 * DODGE_REACH, or gone */
static void e101_mk2_hover(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;
    NJS_POINT3 shot;
    Float distance;

    move(task);
    switch (work->submode) {
    case 0:
        boss->flags |= E101_MK2_TILTS | E101_MK2_SWAYS;
        boss->flags &= ~E101_MK2_FLOATS;
        PLAY(boss, ACTION_HOVER);
        work->collision->shapes->levels =
            (work->collision->shapes->levels & ~SHAPE_GUARD) | LOCKABLE_GUARD;
        boss->flags |= E101_MK2_GUARDED;
        shapes_on(work);
        shapes_harmless(work);
        if (boss->last_mode != E101_MK2_SWAT &&
            boss->last_mode != E101_MK2_LEAP) {
            boss->hover_steps = 0;
        }
        work->submode = 1;
        break;
    case 1:
        if (boss->hover_steps >= ATTACK_WAIT) {
            boss->hover_steps = 0;
            if (rand() * RAND_SCALE <= boss->missile_chance) {
                boss->missile_chance *= 0.8F;
                SET_MODE(task, E101_MK2_MISSILES);
                break;
            }
            if (boss->flags & E101_MK2_DODGED) {
                boss->flags &= ~E101_MK2_DODGED;
                boss->attack_index = 0;
            }
            e101_mk2_next_attack(task);
            if (boss->next_mode != E101_MK2_HOVER) {
                work->submode = MODE_DONE;
            }
        }
        /* @hack spelling: find_shot's result compared with TRUE, which SHC
         * tests with CMP/EQ #1 at 0x0c901cd0. find_shot returns only TRUE or
         * FALSE, so a plain test of it is the same; as one, 2 of 225 words
         * differ */
        if (find_shot(task, NULL) == TRUE) {
            if (rand() * RAND_SCALE < 0.5F) {
                work->collision->shapes->levels =
                    (work->collision->shapes->levels & ~SHAPE_GUARD) |
                    SHAPE_GUARD;
                work->submode = 2;
                break;
            }
            SET_MODE(task, E101_MK2_SWAT);
            work->submode = MODE_DONE;
        }
        steer_to_target(task);
        turn_smoothly(task);
        break;
    case 2:
        if (!find_shot(task, &shot)) {
            distance = 0.0F;
        } else {
            distance = point_distance_squared_across(&work->pos, &shot);
        }
        if (distance <= SQUARE(DODGE_REACH)) {
            SET_MODE(task, E101_MK2_LEAP);
            boss->flags |= E101_MK2_DODGED;
            work->submode = MODE_DONE;
        }
        steer_to_target(task);
        turn_smoothly(task);
        break;
    }
    /* @hack redundant: a test of the submode at the end, which loads it at
     * 0x0c901da2 and compares it at 0x0c901da4 to no effect; without it, 11
     * of 225 words differ */
    if (work->submode == MODE_DONE) {
        return;
    }
}

/* under the full guard, leap to LEAP_REACH past the floor's middle from
 * player 0. bounce as it lands. each step, it lets go of any lock-on, and the
 * shot flies on */
static void e101_mk2_leap(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;
    Float distance;

    lockon_release_hit(task);
    move(task);
    steer_to_destination(task);
    turn_toward(task, TURN_FAST);
    switch (work->submode) {
    case 0:
        boss->flags &= ~E101_MK2_FLOATS;
        boss->flags |= E101_MK2_BLUR;
        work->collision->shapes->levels =
            (work->collision->shapes->levels & ~SHAPE_GUARD) | SHAPE_GUARD;
        boss->flags |= E101_MK2_GUARDED;
        shapes_off(work);
        set_leap_destination(task);
        sound_play_at(SE_LEAP, 0, 0, 0, work->pos.x, work->pos.y, work->pos.z);
        ++work->submode;
        break;
    case 1:
        distance =
            point_distance_squared_across(&work->pos, &boss->destination);
        boss->bob = 0.0F;
        if (distance <= 100.0F && work->pos.y == boss->hover_y) {
            e101_mk2_jets_set_state(JETS_FLARE);
            boss->bob_speed = work->pos.y - boss->last_pos.y;
            boss->bob_damping = 0.115F;
            boss->flags &= ~E101_MK2_BLUR;
            ++work->submode;
        }
        break;
    case 2:
        if (boss->blur_copies == 0) {
            e101_mk2_next_attack(task);
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        orbit_start(task);
        e101_mk2_jets_set_state(JETS_SETTLE);
        boss->flags &= ~E101_MK2_BLUR;
        shapes_on(work);
    }
}

/* under the full guard, blur where it stands. then leave an afterimage, and
 * reappear floating high on the camera's side. each step, it lets go of any
 * lock-on, and the shot flies on */
static void e101_mk2_warp(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;
    NJS_ACTION *action;

    lockon_release_hit(task);
    switch (work->submode) {
    case 0:
        boss->flags |= E101_MK2_BLUR | E101_MK2_WARPING;
        work->collision->shapes->levels =
            (work->collision->shapes->levels & ~SHAPE_GUARD) | SHAPE_GUARD;
        boss->flags |= E101_MK2_GUARDED;
        shapes_off(work);
        set_warp_destination(task);
        boss->timer = 0;
        sound_play_at(SE_LEAP, 0, 0, 0, work->pos.x, work->pos.y, work->pos.z);
        ++work->submode;
        break;
    case 1:
        move(task);
        turn_smoothly(task);
        if (++boss->timer >= 8) {
            if (boss->motion.state != MOTION_BLENDING) {
                action = boss->motion.actions[boss->motion.current].action;
            } else {
                action = boss->motion.blend;
            }
            e101_mk2_afterimage_create(boss, action);
            boss->flags &= ~(E101_MK2_BLUR | E101_MK2_WARPING);
            boss->flags |= E101_MK2_FLOATS;
            VECTOR_COPY(&work->pos, &boss->destination);
            e101_r_camera_set_mode(E101_R_CAMERA_COAST);
            boss->timer = 0;
            ++work->submode;
        }
        break;
    case 2:
        if (++boss->timer >= 40) {
            e101_mk2_next_attack(task);
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        boss->flags &= ~(E101_MK2_BLUR | E101_MK2_WARPING);
        shapes_on(work);
    }
}

/* once the shot is within SWING_REACH, swing the arm on its side. knock the
 * shot away once it is within SWAT_REACH. the arm trails light */
static void e101_mk2_swat(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;
    NJS_POINT3 shot;
    Float distance;

    move(task);
    steer_to_target(task);
    turn_smoothly(task);
#ifdef GCC
    /* GCC can't see that distance is set wherever it is read: only cases 1
     * and 3 read it, and the test below sets it for both */
    distance = 0.0F;
#endif
    if (work->submode <= 3 && work->submode != MODE_DONE) {
        if (!find_shot(task, &shot)) {
            distance = 0.0F;
        } else {
            distance = point_distance_squared_across(&work->pos, &shot);
        }
    }
    switch (work->submode) {
    case 0:
        work->collision->shapes->levels =
            (work->collision->shapes->levels & ~SHAPE_GUARD) | SHAPE_GUARD;
        boss->flags |= E101_MK2_GUARDED;
        ++work->submode;
        break;
    case 1:
        if (distance <= SQUARE(SWING_REACH)) {
            boss->trail_color = 2;
            if (side_of(task, &shot) == SIDE_ARM0) {
                PLAY(boss, ACTION_SWAT_ARM0);
                boss->flags |= E101_MK2_TRAIL_ARM0;
            } else {
                PLAY(boss, ACTION_SWAT_ARM1);
                boss->flags |= E101_MK2_TRAIL_ARM1;
            }
            ++work->submode;
        }
        break;
    case 2:
        motion_flags &= ~MOTION_HOLDING;
        if (boss->motion.current == ACTION_SWAT_ARM1 ||
            boss->motion.current == ACTION_SWAT_ARM0) {
            ++work->submode;
        }
        break;
    case 3:
        if (distance <= SQUARE(SWAT_REACH)) {
            swat_shot(task);
            motion_flags &= ~MOTION_ENDED;
            ++work->submode;
        }
        break;
    case 4:
        if (motion_flags & MOTION_ENDED) {
            orbit_start(task);
            SET_MODE(task, E101_MK2_HOVER);
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        boss->flags &= ~(E101_MK2_TRAIL_ARM0 | E101_MK2_TRAIL_ARM1);
    }
}

/* once the camera has turned toward it, fire its missiles one every
 * MISSILE_GAP steps. they rise in a fan over it from one side to the other. a
 * shot flying at it sends it back to its hover */
static void e101_mk2_fire_missiles(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;
    Float sine;
    Float cosine;
    Float heading_sine;
    Float heading_cosine;
    int steps;
    int fired;
    NJS_VECTOR speed;

    move(task);
    switch (work->submode) {
    case 0:
        work->collision->shapes->levels =
            (work->collision->shapes->levels & ~SHAPE_GUARD) | LOCKABLE_GUARD;
        boss->flags |= E101_MK2_GUARDED;
        shapes_on(work);
        shapes_harmless(work);
        e101_r_camera_set_mode(E101_R_CAMERA_TURN);
        boss->timer = 0;
        ++work->submode;
        break;
    case 1:
        if (e101_r_camera_get_turn() <= READY_TURN) {
            e101_r_camera_set_target_rise(10.0F);
            ++work->submode;
        }
        break;
    case 2:
        steps = boss->missiles * MISSILE_GAP;
        /* @hack spelling: find_shot's result compared with TRUE, which SHC
         * tests with CMP/EQ #1 at 0x0c902376. find_shot returns only TRUE or
         * FALSE, so a plain test of it is the same; as one, 2 of 210 words
         * differ */
        if (find_shot(task, NULL) == TRUE) {
            SET_MODE(task, E101_MK2_HOVER);
        }
        if (boss->timer % MISSILE_GAP == 0) {
            fired = boss->timer / MISSILE_GAP;
            _builtin_fsca(
                (Angle)((((120.0F / boss->missiles) * fired) + 30.0F) *
                        ANGLE_PER_DEGREE),
                &sine, &cosine);
            _builtin_fsca(-work->ang.y, &heading_sine, &heading_cosine);
            speed.x = cosine * heading_sine * 3.0F;
            speed.z = cosine * heading_cosine * 3.0F;
            speed.y = sine * 3.0F;
            e101_mk2_missile_create(&work->pos, &speed, work->ang.y);
            sound_play_at(SE_MISSILE, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        ++boss->timer;
        if (boss->timer >= steps) {
            orbit_start(task);
            e101_mk2_next_attack(task);
            work->submode = MODE_DONE;
        }
        turn_smoothly(task);
        break;
    }
    if (work->submode == MODE_DONE) {
        e101_r_camera_set_mode(E101_R_CAMERA_FOLLOW);
        e101_r_camera_set_target_rise(0.0F);
    }
}

/* wind up and glow, then dash at where player 0 will be, its back
 * thrusting. while guarded, it swats a shot within DASH_SWAT_REACH. it drops
 * its guard as the dash ends, and takes hits until it hovers again */
static void e101_mk2_dash(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;
    NJS_POINT3 shot;
    Float distance;
    Float per_step;

    glide(task);
    switch (work->submode) {
    case 0:
        e101_r_camera_set_mode(E101_R_CAMERA_TURN);
        boss->flags &= ~(E101_MK2_TILTS | E101_MK2_SWAYS);
        work->collision->shapes->levels =
            (work->collision->shapes->levels & ~SHAPE_GUARD) | LOCKABLE_GUARD;
        boss->flags |= E101_MK2_GUARDED;
        shapes_on(work);
        shapes_attack(work);
        e101_mk2_glow_show(TRUE);
        sound_play(SE_WIND_UP, 0, 0, 0);
        ++work->submode;
        /* fall through: it slows from this step */
    case 1:
        boss->velocity.x *= 0.9F;
        boss->velocity.z *= 0.9F;
        turn_toward(task, TURN_FAST);
        if (e101_r_camera_get_turn() <= READY_TURN) {
            PLAY(boss, ACTION_DASH);
            boss->trail_color = 1;
            boss->flags |= E101_MK2_TRAIL_ARM0 | E101_MK2_TRAIL_ARM1;
            ++work->submode;
        }
        break;
    case 2:
        boss->velocity.x *= 0.9F;
        boss->velocity.z *= 0.9F;
        turn_toward(task, TURN_FAST);
        motion_flags &= ~MOTION_HOLDING;
        if (boss->motion.current == ACTION_DASH) {
            boss->lead = 5.0F;
            boss->flags |= E101_MK2_LEADS;
            boss->timer = 0;
            shapes_firm(work);
            ++work->submode;
        }
        break;
    case 3:
        boss->velocity.x *= 0.95F;
        boss->velocity.z *= 0.95F;
        turn_toward(task, TURN_SLOW);
        if (boss->motion.frame >= 75.0F) {
            e101_r_camera_set_mode(E101_R_CAMERA_TRACK);
            e101_mk2_jets_set_state(JETS_FLARE);
            boss->lead = 25.0F;
            /* cover the way to aim in DASH_STEPS steps, back along its y
             * angle */
            per_step = 1.0F / DASH_STEPS;
            boss->move_speed =
                -(point_distance_across(&boss->aim, &work->pos) * per_step);
            ++work->submode;
        }
        break;
    case 4:
        drive(task);
        turn_toward(task, TURN_DASH);
        sound_play_for(SE_BOSS_DASH, (int)task, 1, 0, 4);
        if (boss->motion.frame >= 80.0F) {
            boss->flags |= E101_MK2_THRUST;
        }
        if (boss->motion.frame >= 99.0F) {
            boss->timer = 0;
            boss->flags &= ~E101_MK2_GUARDED;
            e101_mk2_glow_show(FALSE);
            e101_r_camera_set_mode(E101_R_CAMERA_BACK_OFF);
            ++work->submode;
        }
        break;
    case 5:
        drive(task);
        boss->move_speed *= 0.95F;
        sound_play_for(SE_BOSS_DASH, (int)task, 1, 0, 4);
        if (++boss->timer >= 60) {
            boss->flags &= ~E101_MK2_THRUST;
            e101_mk2_jets_set_state(JETS_SETTLE);
            shapes_harmless(work);
            shapes_push(work);
            PLAY(boss, ACTION_DASH_END);
            ++work->submode;
        }
        break;
    case 6:
        drive(task);
        boss->move_speed *= 0.95F;
        motion_flags &= ~MOTION_HOLDING;
        if (boss->motion.current == ACTION_DASH_END) {
            motion_flags &= ~MOTION_ENDED;
            ++work->submode;
        }
        break;
    case 7:
        drive(task);
        boss->move_speed *= 0.95F;
        if (boss->motion.frame >= 45.0F) {
            boss->velocity.x = 0.0F;
            boss->velocity.z = 0.0F;
            ++work->submode;
        }
        break;
    case 8:
        turn_smoothly(task);
        if (motion_flags & MOTION_ENDED) {
            orbit_start(task);
            e101_mk2_next_attack(task);
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        boss->flags &= ~E101_MK2_THRUST;
        e101_mk2_jets_set_state(JETS_SETTLE);
        boss->flags &=
            ~(E101_MK2_TRAIL_ARM0 | E101_MK2_TRAIL_ARM1 | E101_MK2_LEADS);
        boss->flags |= E101_MK2_TILTS | E101_MK2_SWAYS;
        e101_r_camera_set_mode(E101_R_CAMERA_FOLLOW);
        e101_mk2_glow_show(FALSE);
        shapes_harmless(work);
        shapes_push(work);
        boss->flags |= E101_MK2_GUARDED;
    } else if (boss->flags & E101_MK2_GUARDED) {
        /* @hack spelling: find_shot's result compared with TRUE, which SHC
         * tests with CMP/EQ #1 at 0x0c9028ea. find_shot returns only TRUE or
         * FALSE, so a plain test of it is the same; as one, 2 of 429 words
         * differ */
        if (find_shot(task, &shot) == TRUE) {
            distance = point_distance_squared_across(&work->pos, &shot);
            if (distance <= SQUARE(DASH_SWAT_REACH)) {
                swat_shot(task);
            }
        }
    }
}

/* charge the beam between its hands and fire it, then rest. it fires once in
 * E101_MK2_BEAM, and BEAMS_SHOTS times in E101_MK2_BEAMS. the last shot holds
 * damage steps before it fires */
static void e101_mk2_fire_beam(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;

    move(task);
    turn_smoothly(task);
    switch (work->submode) {
    case 0:
        e101_r_camera_set_mode(E101_R_CAMERA_BEHIND);
        work->collision->shapes->levels =
            (work->collision->shapes->levels & ~SHAPE_GUARD) | SHAPE_GUARD;
        boss->flags |= E101_MK2_GUARDED;
        if (work->mode == E101_MK2_BEAMS) {
            boss->beams_left = BEAMS_SHOTS;
        } else {
            boss->beams_left = 1;
        }
        boss->timer = 0;
        ++work->submode;
        /* fall through: the wait starts this step */
    case 1:
        if (++boss->timer >= 40) {
            PLAY(boss, ACTION_BEAM);
            ++work->submode;
        }
        break;
    case 2:
        motion_flags &= ~MOTION_HOLDING;
        if (boss->motion.current == ACTION_BEAM) {
            boss->beam = e101_mk2_beam_create(&boss->cannon);
            ++work->submode;
        }
        break;
    case 3:
        if (boss->motion.frame >= 23.0F) {
            e101_mk2_beam_set_state(boss->beam, BEAM_CHARGE);
            boss->wobble = 5.0F;
            /* @bug logic: stores the pitch's wobble step twice and the roll's
             * never, so the roll's wobble keeps WOBBLE_STEP while the beam
             * charges. the second store, the MOV.L R4,@(R0,R14) at 0x0c902ac4
             * after the one at 0x0c902ac2, writes wobble_step.x where
             * wobble_step.z is meant, as e101_mk2_intro's pair sets both. the
             * resets in submode 6 and at MODE_DONE double their store to x
             * alike, which changes nothing there: the roll's step is still
             * WOBBLE_STEP */
            boss->wobble_step.x = WOBBLE_CHARGE;
            boss->wobble_step.x = WOBBLE_CHARGE;
            ++work->submode;
        }
        break;
    case 4:
        e101_mk2_beam_sparks(&boss->cannon);
        if (boss->motion.frame >= 35.0F) {
            boss->lead = 10.0F;
            boss->flags |= E101_MK2_FROZEN | E101_MK2_LEADS;
            if (--boss->beams_left > 0) {
                boss->timer = 0;
            } else {
                boss->timer = (int)boss->damage;
            }
            ++work->submode;
        }
        break;
    case 5:
        e101_mk2_beam_sparks(&boss->cannon);
        if (--boss->timer <= 0) {
            boss->flags &= ~E101_MK2_FROZEN;
            ++work->submode;
        }
        break;
    case 6:
        e101_mk2_beam_sparks(&boss->cannon);
        if (boss->motion.frame >= 42.0F) {
            Float size;

            if (boss->beams_left > 0) {
                size = BLAST_SIZE;
            } else {
                size = boss->damage + LAST_BLAST_SIZE;
                boss->flags |= E101_MK2_STILL;
                boss->flags &= ~E101_MK2_LEADS;
            }
            shoot_beam(work, boss, size);
            boss->wobble = 4.0F;
            /* @hack redundant: a first store of the pitch's wobble step,
             * which the second replaces, MOV.L R11,@(R0,R14) at 0x0c902bb2;
             * without it, 13 of 316 words differ */
            boss->wobble_step.x = WOBBLE_STEP;
            boss->wobble_step.x = WOBBLE_STEP;
            ++work->submode;
        }
        break;
    case 7:
        if (boss->motion.frame >= 60.0F) {
            if (boss->beams_left > 0) {
                boss->motion.frame = 23.0F;
                work->submode = 2;
            } else {
                ++work->submode;
            }
        }
        break;
    case 8:
        if (boss->motion.current == ACTION_HOVER) {
            boss->timer = 0;
            ++work->submode;
        }
        break;
    case 9:
        if (++boss->timer >= 120) {
            orbit_start(task);
            e101_r_camera_set_mode(E101_R_CAMERA_FOLLOW);
            e101_mk2_next_attack(task);
            work->submode = MODE_DONE;
        }
        break;
    }
    switch (work->submode) {
    case 4:
    case 5:
        sound_play_at_for(SE_CHARGE, (int)task, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 4);
        break;
    }
    if (work->submode == MODE_DONE) {
        boss->flags &= ~(E101_MK2_FROZEN | E101_MK2_LEADS | E101_MK2_STILL);
        boss->wobble = 4.0F;
        /* @hack redundant: a first store of the pitch's wobble step, which
         * the second replaces, MOV.L R11,@(R0,R14) at 0x0c902cb6; without it,
         * 26 of 316 words differ */
        boss->wobble_step.x = WOBBLE_STEP;
        boss->wobble_step.x = WOBBLE_STEP;
        e101_r_camera_set_mode(E101_R_CAMERA_FOLLOW);
    }
}

/* reel from a hit, crackling, then hover again */
static void e101_mk2_hurt(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;

    move(task);
    switch (work->submode) {
    case 0:
        work->collision->shapes->levels =
            (work->collision->shapes->levels & ~SHAPE_GUARD) | SHAPE_GUARD;
        boss->flags |= E101_MK2_GUARDED;
        shapes_on(work);
        shapes_harmless(work);
        boss->flags |= E101_MK2_STILL;
        boss->flags &= ~E101_MK2_SWAYS;
        e101_mk2_set_missiles(task);
        /* @bug logic: the test stores ACTION_DASH as the entry playing,
         * MOV.W R0,@(10,R14) at 0x0c902d5a, and always holds, so the boss
         * always reels as from a dash */
        if ((boss->motion.current = ACTION_DASH)) {
            PLAY(boss, ACTION_HURT_DASHING);
        } else {
            PLAY(boss, ACTION_HURT);
        }
        sound_play(SE_HURT, 0, 0, 0);
        sound_play(SE_CRACKLE, 0, 0, 0);
        e101_mk2_lightning_create(&work->pos);
        ++work->submode;
        break;
    case 1:
        if (boss->motion.frame >= 64.0F) {
            PLAY(boss, ACTION_HOVER);
            SET_MODE(task, E101_MK2_HOVER);
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        boss->attack_index = 0;
        boss->flags &= ~E101_MK2_STILL;
        boss->flags |= E101_MK2_SWAYS;
    }
}

/* drift and break up in lightning and blasts as the camera circles it,
 * then clear the stage */
static void e101_mk2_defeat(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;
    NJS_POINT3 *pos = &work->pos;

    move(task);
    switch (work->submode) {
    case 0:
        e101_r_camera_set_mode(E101_R_CAMERA_TURN);
        boss->flags |= E101_MK2_BLUR;
        boss->flags &= ~(E101_MK2_TILTS | E101_MK2_SWAYS);
        PLAY(boss, ACTION_DEFEAT);
        boss->timer = 200;
        e101_mk2_missiles_explode();
        time_stop();
        ++work->submode;
        break;
    case 1:
        if (--boss->timer <= 0) {
            boss->timer = 200;
            ++work->submode;
        }
        break;
    case 2:
        if (boss->steps % 4 == 0) {
            e101_mk2_lightning_near(pos, 10.0F);
        }
        /* a blast goes off when timer divides evenly by timer / 5: about
         * one step in 5 */
        /* @bug arithmetic: under a timer of 5, the divisor is 0. __modls,
         * called at 0x0c902f4e, then sets errno and returns 0, so a blast goes
         * off each step */
#ifdef GCC
        /* keeps the value the tag names, 0: under a timer of 5 the test
         * holds. GCC's own timer % 0 gives timer, which fails it */
        if (boss->timer * 40 / 200 == 0 ||
            boss->timer % (boss->timer * 40 / 200) == 0) {
#else
        if (boss->timer % (boss->timer * 40 / 200) == 0) {
#endif
            e101_mk2_light_burst(pos, 80.0F);
            e101_mk2_bomb_near(pos, 20.0F);
            if (rand() * RAND_SCALE <= 0.5F) {
                sound_play_at(SE_EXPLOSION, 0, 0, 0, pos->x, pos->y, pos->z);
            }
        }
        if (boss->timer % 20 == 0) {
            boss->velocity.x = (rand() * RAND_SCALE) - 0.5F;
            boss->velocity.z = (rand() * RAND_SCALE) - 0.5F;
        }
        if (--boss->timer <= 0) {
            e101_r_camera_set_mode(E101_R_CAMERA_ORBIT);
            pad_disable();
            e101_mk2_jets_destroy();
            boss->timer = 0;
            ++work->submode;
        }
        break;
    case 3:
        e101_mk2_lightning_near(pos, 10.0F);
        /* a blast goes off when timer divides evenly by timer / 6: about
         * one step in 6 */
        /* @bug arithmetic: under a timer of 6, the divisor is 0 here too.
         * __modls, called at 0x0c90305a, then returns 0, so a blast goes off
         * each step */
#ifdef GCC
        /* keeps the value the tag names, 0: under a timer of 6 the test
         * holds. GCC's own timer % 0 gives timer, which fails it from 1 to
         * 5 */
        if (boss->timer * 30 / 180 == 0 ||
            boss->timer % (boss->timer * 30 / 180) == 0) {
#else
        if (boss->timer % (boss->timer * 30 / 180) == 0) {
#endif
            e101_mk2_light_burst(pos, 80.0F);
            e101_mk2_bomb_near(pos, 20.0F);
            if (rand() * RAND_SCALE <= 0.5F) {
                sound_play_at(SE_EXPLOSION, 0, 0, 0, pos->x, pos->y, pos->z);
            }
        }
        if (++boss->timer >= 180) {
            boss->flags &= ~E101_MK2_BLUR;
            e101_r_camera_set_mode(E101_R_CAMERA_ORBIT_END);
            boss->timer = 0;
            ++work->submode;
        }
        break;
    case 4:
        e101_mk2_lightning_near(&work->pos, 10.0F);
        if (++boss->timer >= 200) {
            ++work->submode;
        }
        break;
    case 5:
        stage_clear();
        work->submode = MODE_DONE;
        break;
    }
    switch (work->submode) {
    case 2:
    case 3:
    case 4:
        if (boss->timer % 60 == 0) {
            sound_play_at(SE_CRACKLE, 0, 0, -16, pos->x, pos->y, pos->z);
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        pad_enable();
        boss->flags &= ~E101_MK2_BLUR;
    }
}

static void e101_mk2_pause(struct task *task)
{
    if (per_info[0]->press & (PDD_DGT_TA | PDD_DGT_TB)) {
        SET_MODE(task, E101_MK2_HOVER);
        camera_override_mode(CAMERA_MODE_E101R, CAMERA_ADJUST_NONE);
    }
}

static void e101_mk2_step(struct task *task)
{
    struct task_work *work = task->work;
    struct e101_mk2 *boss = task->extra;
    NJS_POINT3 *pos = &player_work[0]->pos;

    if (pos->y <= FALL_Y) {
        player_fall(0);
    }
    boss->steps++;
    boss->hover_steps++;
    sway_step(task);
    e101_mk2_pose(task);
    orbit_step(task);
    e101_mk2_modes[work->mode](task);
    tilt_step(task);
    e101_mk2_take_hit(task);
    track_player(task);
    e101_mk2_glow_step();
    e101_r_camera_set_wide(e101_mk2_missiles_flying > 0);
    if (boss->flags & E101_MK2_THRUST) {
        e101_mk2_thrust_puff(&boss->thrust_pos, &boss->thrust_dir);
        e101_mk2_thrust_puff(&boss->thrust_pos, &boss->thrust_dir);
    }
    if (boss->health <= 1.0F) {
        if (boss->steps % CRACKLE_INTERVAL == 0) {
            if (rand() * RAND_SCALE < CRACKLE_CHANCE) {
                e101_mk2_lightning_create(&work->pos);
                sound_play_at(SE_CRACKLE, 0, 0, 0, work->pos.x, work->pos.y,
                              work->pos.z);
            }
        }
    }
    e101_mk2_hint(boss);
    if (work->mode != boss->next_mode) {
        if (work->submode != MODE_DONE) {
            work->submode = MODE_DONE;
            e101_mk2_modes[work->mode](task);
        }
        work->mode = boss->next_mode;
        work->submode = 0;
    }
    life_gauge_health = boss->health;
    if (!(boss->flags & E101_MK2_FROZEN)) {
        motion_step(work, &boss->motion);
    }
    collision_register(work);
    e101_mk2_draw(task);
}

/* the motion callback: move SWAY_OBJECT along its z by sway, leaning the
 * upper body */
static void e101_mk2_sway_node(NJS_OBJECT *object)
{
    if (object == (NJS_OBJECT *)(e101_r_models + SWAY_OBJECT)) {
        njTranslate(NULL, 0.0F, 0.0F, e101_mk2_record->sway);
    }
}

/* draw the boss and its glow, then its copies or its arms' trails. while the
 * boss is drawn, it also sets hover_y, which fall reads */
static void e101_mk2_draw(struct task *task)
{
    struct e101_mk2 *boss = task->extra;
    struct task_work *work = task->work;
    NJS_ACTION *action;
    Angle ang;

    if (boss->motion.state != MOTION_BLENDING) {
        action = boss->motion.actions[boss->motion.current].action;
    } else {
        action = boss->motion.blend;
    }
    if (!(boss->flags & E101_MK2_HIDDEN)) {
        /* hover_y follows the floor its shadow finds. a floor that would put
         * it at MIN_HOVER_Y or below leaves it as it was */
        Float hover_y = shadow_draw(work, 4.0F) + HOVER_HEIGHT;

        if (hover_y <= MIN_HOVER_Y) {
            /* @hack redundant: hover_y compared with MIN_HOVER_Y, which picks
             * MIN_HOVER_Y only where hover_y holds it already, FCMP/EQ FR5,FR2
             * at 0x0c903486; as a plain copy of hover_y, 13 of 211 words
             * differ */
            if (boss->hover_y == MIN_HOVER_Y) {
                hover_y = MIN_HOVER_Y;
            } else {
                hover_y = boss->hover_y;
            }
        }
        if (boss->last_hover_y == MIN_HOVER_Y) {
            boss->last_hover_y = hover_y;
        } else {
            boss->last_hover_y = boss->hover_y;
        }
        boss->hover_y = hover_y;
        boss->height = work->pos.y - hover_y;
    }
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + boss->bob, work->pos.z);
    TURN_VIA(njRotateZ, boss->roll, ang);
    TURN_VIA(njRotateX, boss->pitch, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateY, -work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    njScale(NULL, 1.0F, 1.0F, 1.0F);
    draw_control_save();
    draw_attr_save();
    e101_mk2_glow_draw();
    draw_attr_restore();
    draw_control_restore();
    njSetTexture(e101_r_texlist);
    if (!(boss->flags & E101_MK2_HIDDEN)) {
        njSetMotionCallback(e101_mk2_sway_node);
        draw_action(action, boss->motion.frame);
        njSetMotionCallback(NULL);
    }
    njPopMatrix(1);
    draw_control_save();
    draw_attr_save();
    if (boss->blur_copies > 0) {
        e101_mk2_blur_draw(boss, action);
    } else {
        if (boss->trail_copies[0] > 0) {
            e101_mk2_trail_draw(boss, 0, boss->trail_color);
        }
        if (boss->trail_copies[1] > 0) {
            e101_mk2_trail_draw(boss, 1, boss->trail_color);
        }
    }
    draw_attr_restore();
    draw_control_restore();
}
