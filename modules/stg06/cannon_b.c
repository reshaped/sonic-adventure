/* CANNON_B, a launcher that lobs shells at the deck's platforms as scripts
 * command it. a shell hits the platform it was fired at, and knocks back
 * player 0 standing there. launchers 1, 6, 7 and 8 each queue a script of
 * their own. each does so once, when player 0 stands on a platform in its
 * range */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg06/cannon_b.h"
#include "stg06/knock_back.h"
#include "stg06/models.h"
#include "stg06/shell.h"
#include "stg06/sky_deck.h"
#include "stg06/target.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT cannon_b_object;
extern NJS_MOTION cannon_b_motion;

#define SE_CANNON_B_AIM 163
#define AIM_VOLUME 96

/* the most the barrel turns a step, in pitch and in yaw */
#define TURN_SPEED 0x222
/* the draw turns the barrel's nodes about x by its pitch less BARREL_TILT.
 * at pitch 0 they keep cannon_b_object's own angle */
#define BARREL_TILT 0x3888
/* the launcher aims from AIM_DROP below its position */
#define AIM_DROP 142.0F
/* a shot needs a platform in a box about the point. the box reaches SHOT_REACH
 * along x and along z, and SHOT_HEIGHT along y */
#define SHOT_REACH 8.0F
#define SHOT_HEIGHT 16.0F
/* a shot adds RECOIL_KICK to the recoil. each step, a spring takes
 * RECOIL_PULL of the recoil from its speed. the speed keeps RECOIL_DAMP of
 * itself, and rebounds from 0 at RECOIL_BOUNCE of itself */
#define RECOIL_KICK 15.0F
#define RECOIL_PULL 0.3F
#define RECOIL_DAMP 0.95F
#define RECOIL_BOUNCE (-0.65F)
/* the launcher fires only with the recoil 0 and slower than RECOIL_STILL */
#define RECOIL_STILL 0.5F
#define CANNON_B_VIEW 210.0F
/* the draw and the muzzle place cannon_b_object's nodes by these, in place
 * of the nodes' own. the base node sits BASE_DROP below the body, turned
 * BASE_TURN, which the yaw node below it turns back. two side nodes sit
 * SIDE_DROP below the yaw node, turned half round about z. the mount sits
 * MOUNT_DROP below the yaw node, and one node sits MOUNT_TOP above the
 * mount. the barrels sit BARREL_DROP below the mount, and the one the muzzle
 * is on sits BARREL_BACK behind */
#define BASE_DROP 40.0F
#define BASE_TURN 0x4ccc
#define SIDE_DROP 120.0F
#define HALF_TURN 0x8000
#define MOUNT_DROP 30.0F
#define MOUNT_TOP 30.0F
#define BARREL_DROP 102.0F
#define BARREL_BACK 6.03F

/* a shell flies SHELL_PACE across a step, and falls SHELL_GRAVITY a step
 * faster */
#define SHELL_PACE 4.0F
#define SHELL_GRAVITY 0.1F

/* the barrel, which the launcher allocates from the heap. it is home at
 * pitch 0 and yaw ang.y. nothing draws the recoil, which holds off the
 * next shot until it settles */
struct cannon_b_barrel {
    union angle_halves yaw;
    union angle_halves pitch;
    Float recoil;
    Float recoil_speed;
};

/* the launcher's own layout of task_work */
struct cannon_b_work {
    Sint8 mode;
    Sint8 submode;
    Uint8 unk_02;
    /* the placement's scl.x, by which the scripts find it. 0 for none */
    Sint8 id;
    Sint16 flags;
    Uint16 timer;
    Uint8 unk_08[4];
    struct cannon_b_barrel *barrel;
    struct sky_deck_target *target; /* its own, for an id other than 0 */
    struct angles ang;
    NJS_POINT3 pos;
    /* the placement's scale, until the first fire command makes it the point
     * to aim at */
    NJS_POINT3 scl;
    struct collision_work *collision;
    struct event_work *event;
};

enum cannon_b_mode {
    CANNON_B_CLOSED,
    CANNON_B_OPENING, /* its motion plays */
    CANNON_B_OPEN,
    CANNON_B_AIMING, /* the barrel turns to the point, then fires */
    CANNON_B_CLOSING /* the barrel turns home, then its motion plays back */
};

/* a script's command: open, fire or close the launcher whose id is
 * launcher. a script ends with COMMAND_END */
struct cannon_b_command {
    int op;
    int launcher;
    int platform; /* for COMMAND_FIRE, the id of the platform's target */
};

enum cannon_b_op { COMMAND_END, COMMAND_OPEN, COMMAND_FIRE, COMMAND_CLOSE };

/* the muzzle, from the barrel's pivot in its own frame */
static const NJS_POINT3 cannon_b_muzzle = {0.0F, -40.0F, 0.0F};

static void cannon_b_arrive(struct sky_deck_target *target, NJS_POINT3 *pos,
                            NJS_VECTOR *speed);
static Bool cannon_b_script_step(void *script, int index, Bool *keep);
static void cannon_b_init(struct task *task);
static void cannon_b_destroy(struct task *task);
static void cannon_b_run(struct task *task);
static void cannon_b_draw(struct task *task);

static struct shell_spec cannon_b_shell = {
    SHELL_SPOT | SHELL_BURST | SHELL_HARMLESS, 0, -SHELL_GRAVITY, 0.8F,
    cannon_b_arrive};
/* the scripts that launchers 1, 6, 7 and 8 queue */
static struct cannon_b_command cannon_b_script_1[21] = {
    {COMMAND_OPEN, 1, 0},  {COMMAND_FIRE, 1, 2},  {COMMAND_OPEN, 2, 0},
    {COMMAND_FIRE, 1, 3},  {COMMAND_CLOSE, 1, 0}, {COMMAND_FIRE, 2, 4},
    {COMMAND_OPEN, 3, 0},  {COMMAND_FIRE, 2, 5},  {COMMAND_CLOSE, 2, 0},
    {COMMAND_FIRE, 3, 6},  {COMMAND_OPEN, 4, 0},  {COMMAND_FIRE, 3, 7},
    {COMMAND_CLOSE, 3, 0}, {COMMAND_FIRE, 4, 8},  {COMMAND_OPEN, 5, 0},
    {COMMAND_FIRE, 4, 9},  {COMMAND_CLOSE, 4, 0}, {COMMAND_FIRE, 5, 10},
    {COMMAND_FIRE, 5, 11}, {COMMAND_CLOSE, 5, 0}, {COMMAND_END, 0, 0},
};
static struct cannon_b_command cannon_b_script_6[4] = {
    {COMMAND_OPEN, 6, 0},
    {COMMAND_FIRE, 6, 15},
    {COMMAND_CLOSE, 6, 0},
    {COMMAND_END, 0, 0},
};
static struct cannon_b_command cannon_b_script_7[4] = {
    {COMMAND_OPEN, 7, 0},
    {COMMAND_FIRE, 7, 18},
    {COMMAND_CLOSE, 7, 0},
    {COMMAND_END, 0, 0},
};
static struct cannon_b_command cannon_b_script_8[5] = {
    {COMMAND_OPEN, 8, 0},  {COMMAND_FIRE, 8, 21}, {COMMAND_FIRE, 8, 22},
    {COMMAND_CLOSE, 8, 0}, {COMMAND_END, 0, 0},
};
/* clang-format off */
static struct collision_shape cannon_b_shapes[2] = {
    {SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 24.0F, 0.0F}, 28.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 11.5F, 0.0F}, 23.0F, 23.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* start a closed launcher opening, and return TRUE. FALSE, changing
 * nothing, for any other */
static Bool cannon_b_open(struct cannon_b_work *work)
{
    if (work->mode == CANNON_B_CLOSED) {
        work->mode = CANNON_B_OPENING;
        work->timer = 0;
        return TRUE;
    }
    return FALSE;
}

/* start an open launcher aiming at point, with its sound, and return TRUE.
 * FALSE, changing nothing, unless it is open and its recoil settled */
static Bool cannon_b_fire(NJS_POINT3 *point, struct cannon_b_work *work)
{
    if (work->mode == CANNON_B_OPEN &&
        njAbs(work->barrel->recoil_speed) < RECOIL_STILL &&
        njAbs(work->barrel->recoil) == 0.0F) {
        work->scl = *point;
        work->mode = CANNON_B_AIMING;
        work->timer = 0;
        sound_play_at(SE_CANNON_B_AIM, 0, 0, AIM_VOLUME, work->pos.x,
                      work->pos.y, work->pos.z);
        return TRUE;
    }
    return FALSE;
}

/* start an open launcher closing, and return TRUE. FALSE, changing
 * nothing, for any other */
static Bool cannon_b_close(struct cannon_b_work *work)
{
    if (work->mode == CANNON_B_OPEN) {
        work->mode = CANNON_B_CLOSING;
        work->timer = 0;
        return TRUE;
    }
    return FALSE;
}

/* set point to the muzzle, with the barrel turned to pitch and yaw as the
 * draw places it */
static void cannon_b_find_muzzle(const Angle *pitch, const Angle *yaw,
                                 NJS_POINT3 *point, struct cannon_b_work *work)
{
    NJS_POINT3 muzzle;
    Angle ang;

    muzzle = cannon_b_muzzle;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, *yaw, ang);
    njTranslate(NULL, 0.0F, -(BASE_DROP + MOUNT_DROP), 0.0F);
    njTranslate(NULL, 0.0F, -BARREL_DROP, -BARREL_BACK);
    TURN_VIA(njRotateX, *pitch - BARREL_TILT, ang);
    njCalcPoint(NULL, &muzzle, point);
    njPopMatrix(1);
}

/* set speed to carry a shell from start to end at SHELL_PACE across a
 * step. set yaw to face end, and pitch to point at the top of the arc. a
 * shell that starts falling takes pitch along speed instead. returns 1 */
static int cannon_b_aim(NJS_POINT3 *start, NJS_POINT3 *end, NJS_VECTOR *speed,
                        Angle *pitch, Angle *yaw)
{
    /* @hack spelling: byte cursors step through the whole points a Float at a
     * time, as the image loads and stores them (0x0c9114f8 to 0x0c91150a,
     * 0x0c91153c to 0x0c911548); read through the members, the function
     * runs 2 bytes long and 98 of 107 words differ, and written through
     * them, 2 bytes long and 61 */
    char *from = (char *)start;
    char *to = (char *)end;
    char *out;
    Float dx;
    Float dy;
    Float dz;
    Float steps;
    Float x;
    Float y;
    Float z;
    Float height;
    Float distance;

    dx = *(Float *)to - *(Float *)from;
    from += sizeof(Float);
    to += sizeof(Float);
    dy = *(Float *)to - *(Float *)from;
    to += sizeof(Float);
    from += sizeof(Float);
    dz = *(Float *)to - *(Float *)from;
    /* @hack redundant: three never-run stores of dz, which keep dx in FR14
     * and dz in FR15 (0x0c9114f8, 0x0c91150a). without them, or with one,
     * the function runs 4 bytes short and 101 of 107 words differ. with
     * two, 20 differ */
    if (0) {
        *(Float *)to = dz;
        *(Float *)to = dz;
        *(Float *)to = dz;
    }
    steps = njSqrt((dx * dx) + (dz * dz));
    steps *= 1.0F / SHELL_PACE;
    x = dx / steps;
    y = (dy + (steps * steps * (0.5F * SHELL_GRAVITY))) / steps;
    z = dz / steps;
    out = (char *)speed;
    *(Float *)out = x;
    out += sizeof(Float);
    *(Float *)out = y;
    out += sizeof(Float);
    *(Float *)out = z;
    if (y >= 0.0F) {
        steps = y * (1.0F / SHELL_GRAVITY);
        x *= steps;
        z *= steps;
        y *= steps;
        height = y - (steps * steps * (0.5F * SHELL_GRAVITY));
        distance = njSqrt((x * x) + (z * z));
    } else {
        height = y;
        distance = SHELL_PACE;
    }
    *pitch = -(int)(atan2f(height, distance) * ANGLE_PER_RADIAN);
    *yaw = (int)(atan2f(x, z) * ANGLE_PER_RADIAN);
    return 1;
}

/* @hack redundant: register on task, which keeps it in R14 across the range
 * check (0x0c9115be); without it, task is kept at sp+0, 10 of 14 words
 * differ and cannon_b_init starts 2 bytes late */
void cannon_b_step(register struct task *task)
{
    if (!placement_try_leave(task)) {
        cannon_b_init(task);
    }
}

/* the launcher starts closed, its barrel home. with an id, it adds a
 * TARGET_LAUNCHER target of that id, by which the scripts find it */
static void cannon_b_init(struct task *task)
{
    struct cannon_b_work *work = (struct cannon_b_work *)task->work;

    if (!placement_editing(task)) {
        collision_init(task, cannon_b_shapes, COLLISION_OBJECT, 1, 2);
    }
    task->draw = cannon_b_draw;
    task->step = cannon_b_run;
    task->destructor = cannon_b_destroy;
    work->barrel = heap_alloc(sizeof(struct cannon_b_barrel));
    work->id = (int)work->scl.x;
    work->timer = 0;
    work->mode = CANNON_B_CLOSED;
    work->barrel->recoil = 0.0F;
    work->barrel->recoil_speed = 0.0F;
    work->barrel->pitch.whole = 0;
    work->barrel->yaw.whole = work->ang.y;
    if (work->id == 0) {
        return;
    }
    work->target = sky_deck_target_add((struct task_work *)work);
    work->target->id = work->id;
    work->target->group = TARGET_LAUNCHER;
    work->target->hit = NULL;
}

static void cannon_b_destroy(struct task *task)
{
    struct cannon_b_work *work = (struct cannon_b_work *)task->work;

    /* @hack spelling: a jump past the remove, which calls
     * sky_deck_target_remove through R2 and heap_free through R3
     * (0x0c911666, 0x0c91166e); as an if, the two trade and 4 of 14 words
     * differ */
    if (!work->id) {
        goto free_barrel;
    }
    sky_deck_target_remove(work->target);
free_barrel:
    heap_free(work->barrel);
}

/* closed, launchers 1, 6, 7 and 8 queue their scripts. opening and closing
 * play the motion. aiming turns the barrel toward the point, then fires at
 * the platform there, if any. the recoil springs back every step */
static void cannon_b_run(struct task *task)
{
    struct cannon_b_work *work = (struct cannon_b_work *)task->work;
    struct sky_deck_target *entry;
    Sint8 id;
    int launcher;
    Sint8 stood;
    int pitch_delta;
    int yaw_delta;
    int raw_pitch;
    int raw_yaw;
    NJS_POINT3 origin;
    NJS_POINT3 muzzle;
    NJS_VECTOR speed;
    /* @hack type: each pointer in a record of its own, which puts aim_pitch
     * and aim_yaw at sp+4 and sp+8 and the pointers at sp+12 and sp+16
     * (0x0c911996, 0x0c911998); as volatile pointers alone, the two pairs
     * trade places and 39 of 485 words differ */
    /* @hack type: volatile on the pointers, which reloads them from the
     * frame at each use (0x0c9119ec to 0x0c911a74); without it, 192 of 485
     * words differ and the function ends 2 bytes early */
    struct {
        Sint16 *volatile half;
    } to_pitch, to_yaw; /* at aim_pitch's and aim_yaw's low halves */
    union angle_halves aim_pitch;
    union angle_halves aim_yaw;

    if (placement_try_leave(task)) {
        return;
    }
    cannon_b_draw(task);
    if (placement_editing(task)) {
        return;
    }
    work->timer++;
    /* @hack redundant: a never-run store of CANNON_B_OPEN, which keeps its 2
     * in R9 (0x0c911716) and the barrel's pitch half's address in R8
     * (0x0c9119a2); without it, 333 of 485 words differ and the function
     * ends 2 bytes late */
    if (0) {
        work->mode = CANNON_B_OPEN;
    }
    switch (work->mode) {
    case CANNON_B_CLOSED:
        /* @hack spelling: the id held in id, a Sint8 as work->id is, which
         * launcher and the call below read. read into launcher straight from
         * work, and passed to the call as launcher, the value is the same, but
         * the frame takes 60 bytes, the function ends 2 bytes late and 366 of
         * 485 words differ */
        id = work->id;
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        launcher = id;
        if (launcher && !work->target->queued) {
            if (launcher == 1 || launcher == 6 || launcher == 7 ||
                launcher == 8) {
                /* the first target player 0 stands on, if its group has
                 * TARGET_PLATFORM's bit, as TARGET_TALAP_1's does.
                 * sky_deck_target_find_stood_on ignores its argument, and
                 * the image passes id in R4 (0x0c911782, 0x0c9117b8) */
                entry = sky_deck_target_find_stood_on(id);
                if (entry && (entry->group & TARGET_PLATFORM) &&
                    (stood = entry->id)) {
                    switch (launcher) {
                    case 1:
                        if (stood >= 2 && stood <= 11) {
                            if (sky_deck_sequence_add(cannon_b_script_step,
                                                      cannon_b_script_1)) {
                                work->target->queued |= 1;
                            }
                        }
                        break;
                    case 6:
                        if (stood >= 13) {
                            if (sky_deck_sequence_add(cannon_b_script_step,
                                                      cannon_b_script_6)) {
                                work->target->queued |= 1;
                            }
                        }
                        break;
                    case 7:
                        if (stood >= 16) {
                            if (sky_deck_sequence_add(cannon_b_script_step,
                                                      cannon_b_script_7)) {
                                work->target->queued |= 1;
                            }
                        }
                        break;
                    case 8:
                        if (stood >= 19) {
                            if (sky_deck_sequence_add(cannon_b_script_step,
                                                      cannon_b_script_8)) {
                                work->target->queued |= 1;
                            }
                            /* @hack redundant: a break that repeats the case's
                             * own, which keeps the queued bit's store ahead
                             * of the BRA (0x0c911898); without it, the store
                             * fills the BRA's delay slot and 3 of 485 words
                             * differ */
                            break;
                        }
                        break;
                    }
                }
            }
        }
        break;
    case CANNON_B_OPENING:
        if (work->timer >= cannon_b_motion.nbFrame) {
            work->mode = CANNON_B_OPEN;
            work->timer = 0;
        }
        sound_play_at_for(SE_TURRET, (int)task, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 2);
        break;
    case CANNON_B_CLOSING: {
        Sint16 *at_pitch = work->barrel->pitch.half;
        Sint16 *at_yaw = work->barrel->yaw.half;
        int delta;
        int close_delta;

        if (*at_pitch == 0 && *at_yaw == (Sint16)work->ang.y) {
            if (work->timer >= cannon_b_motion.nbFrame) {
                work->mode = CANNON_B_CLOSED;
                work->timer = 0;
            }
        } else {
            if (*at_pitch > TURN_SPEED) {
                *at_pitch -= TURN_SPEED;
            } else if (*at_pitch < -TURN_SPEED) {
                *at_pitch += TURN_SPEED;
            } else {
                *at_pitch = 0;
            }
            /* @hack type: volatile on the yaw half's read, which loads
             * it (0x0c911962) before ang.y (0x0c911964); without it, the two
             * loads trade and 2 of 485 words differ */
            delta = *(volatile Sint16 *)at_yaw;
            /* @hack spelling: ang.y taken in a statement of its own, which
             * keeps delta in R4 and close_delta in R5 (0x0c911962, 0x0c911968);
             * in one with the read, the two trade and 12 of 485 words differ */
            delta -= work->ang.y;
            close_delta = (Sint16)delta;
            /* @hack redundant: an else that never runs, whose three tests of
             * close_delta keep it in R5 and at_yaw in R6 (0x0c911962,
             * 0x0c911968). without them, the two trade, as do registers
             * before them, and 11 of 485 words differ */
            if (1) {
            } else {
                if (close_delta) {
                }
                if (close_delta) {
                }
                if (close_delta) {
                }
            }
            if (close_delta > TURN_SPEED) {
                delta -= TURN_SPEED;
            } else if (close_delta < -TURN_SPEED) {
                delta += TURN_SPEED;
            } else {
                delta = 0;
            }
            *at_yaw = work->ang.y + delta;
            work->timer = 0;
        }
        break;
    }
    case CANNON_B_AIMING: {
        Sint16 *at_yaw;
        Sint16 *at_pitch;

        /* @hack redundant: a never-run test of origin's address, which sets
         * up to_yaw before to_pitch (0x0c91198a, 0x0c911994); without it,
         * to_pitch comes first and 7 of 485 words differ */
        if (0) {
            /* NOLINTNEXTLINE(clang-diagnostic-pointer-bool-conversion) */
            if (&origin) {
            }
        }
        to_pitch.half = aim_pitch.half;
        /* @hack redundant: a never-run test of work, which puts to_yaw in R2
         * (0x0c911988); without it, R3 takes it and 10 of 485 words differ */
        if (0) {
            if (work) {
            }
        }
        to_yaw.half = aim_yaw.half;
        at_pitch = work->barrel->pitch.half;
        at_yaw = work->barrel->yaw.half;
        origin = work->pos;
        /* @hack redundant: a never-run test of at_yaw, which keeps the task
         * in R10 and CANNON_B_OPEN's 2 in R9 (0x0c9116f4, 0x0c911716);
         * without it, the two trade, the frame takes 60 bytes, 247 of 485
         * words differ and the function ends 4 bytes late */
        if (0) {
            if (at_yaw) {
            }
        }
        /* @hack spelling: AIM_DROP negated and added, as the image adds
         * -142.0 (0x0c9119ba); with -=, 1 of 485 words differ */
        origin.y += -AIM_DROP;
        if (cannon_b_aim(&origin, &work->scl, &speed, &aim_pitch.whole,
                         &aim_yaw.whole)) {
            cannon_b_find_muzzle(&aim_pitch.whole, &aim_yaw.whole, &muzzle,
                                 work);
            raw_pitch = *to_pitch.half - *at_pitch;
            pitch_delta = (Sint16)raw_pitch;
            /* @hack redundant: a never-run test of raw_pitch, which keeps
             * the frame at 56 bytes (0x0c9116f0); without it, the frame takes
             * 60, 236 of 485 words differ and the function ends 2 bytes
             * late */
            if (0) {
                if (raw_pitch) {
                }
            }
            if (pitch_delta > TURN_SPEED) {
                *to_pitch.half = *at_pitch + TURN_SPEED;
            } else if (pitch_delta < -TURN_SPEED) {
                *to_pitch.half = *at_pitch - TURN_SPEED;
            }
            raw_yaw = *to_yaw.half - *at_yaw;
            yaw_delta = (Sint16)raw_yaw;
            /* @hack redundant: a never-run test of raw_yaw, which keeps it in
             * R6 and yaw_delta in R5 (0x0c911a16 to 0x0c911a1a); without it,
             * both take R5 and 4 of 485 words differ */
            if (0) {
                if (raw_yaw) {
                }
            }
            if (yaw_delta > TURN_SPEED) {
                *to_yaw.half = *at_yaw + TURN_SPEED;
            } else if (yaw_delta < -TURN_SPEED) {
                *to_yaw.half = *at_yaw - TURN_SPEED;
            }
            *at_pitch = *to_pitch.half;
            *at_yaw = *to_yaw.half;
            if (!pitch_delta && !yaw_delta) {
                /* the first unbroken platform within reach of the point: its
                 * state has no bit but TARGET_STOOD_ON */
                entry =
                    sky_deck_target_find_masked(TARGET_PLATFORM, &work->scl,
                                                SHOT_REACH, SHOT_HEIGHT, 0, -2);
                if (entry && entry->id > 0) {
                    cannon_b_aim(&muzzle, &work->scl, &speed, &aim_pitch.whole,
                                 &aim_yaw.whole);
                    sky_deck_shell_fire_at(&muzzle, &work->scl, &speed, entry,
                                           &cannon_b_shell);
                    work->barrel->recoil += RECOIL_KICK;
                }
                work->mode = CANNON_B_OPEN;
                work->timer = 0;
            }
        }
        break;
    }
    /* @hack spelling: CANNON_B_OPEN's case last, which keeps the closed case's
     * target in R1 (0x0c91178e); first, R2 takes it and 2 of 485 words
     * differ */
    case CANNON_B_OPEN:
        break;
    }
    /* @hack spelling: the pull negated and added, FNEG and FADD at 0x0c911b00
     * and 0x0c911b02; with -=, SHC subtracts, the function ends 2 bytes early
     * and 51 of 485 words differ */
    work->barrel->recoil_speed += -work->barrel->recoil * RECOIL_PULL;
    work->barrel->recoil_speed *= RECOIL_DAMP;
    if (work->barrel->recoil + work->barrel->recoil_speed <= 0.0F) {
        work->barrel->recoil = 0.0F;
        work->barrel->recoil_speed *= RECOIL_BOUNCE;
    } else {
        work->barrel->recoil += work->barrel->recoil_speed;
    }
    collision_register((struct task_work *)work);
}

/* closed, opening, or closing with its barrel home, the launcher draws as
 * its motion. otherwise it draws node by node, turned to the barrel's yaw
 * and pitch */
static void cannon_b_draw(struct task *task)
{
    struct cannon_b_work *work = (struct cannon_b_work *)task->work;
    NJS_OBJECT *object = &cannon_b_object;
    NJS_MOTION *motion = &cannon_b_motion;
    Angle ang;
    int frame;

    if (placement_try_leave(task) && frame_step) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, CANNON_B_VIEW)) {
        return;
    }
    njSetTexture(&obj_skydeck_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    switch (work->mode) {
    case CANNON_B_CLOSED:
        draw_motion(object, motion, 0.0F);
        break;
    case CANNON_B_OPENING:
        frame =
            work->timer >= motion->nbFrame ? motion->nbFrame - 1 : work->timer;
        draw_motion(object, motion, (Float)frame);
        break;
    case CANNON_B_CLOSING: {
        Sint16 *pitch = work->barrel->pitch.half;
        Sint16 *yaw = work->barrel->yaw.half;

        if (*pitch == 0 && *yaw == work->ang.y) {
            frame = work->timer >= motion->nbFrame
                        ? 0
                        : motion->nbFrame - work->timer - 1;
            draw_motion(object, motion, (Float)frame);
            break;
        }
    }
        /* a closing launcher whose barrel isn't home falls through to the
         * default's draw, node by node */
    default:
        draw_model(object->model);
        object = object->child;
        njTranslate(NULL, 0.0F, -BASE_DROP, 0.0F);
        njRotateY(NULL, BASE_TURN);
        draw_model(object->model);
        object = object->child;
        TURN_VIA(njRotateY, work->barrel->yaw.whole - work->ang.y - BASE_TURN,
                 ang);
        draw_model(object->model);
        object = object->child;
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, -SIDE_DROP, 0.0F);
        njRotateZ(NULL, -HALF_TURN);
        draw_model(object->model);
        njPopMatrix(1);
        object = object->sibling;
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, -SIDE_DROP, 0.0F);
        njRotateZ(NULL, HALF_TURN);
        draw_model(object->model);
        njPopMatrix(1);
        object = object->sibling;
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, -MOUNT_DROP, 0.0F);
        draw_model(object->model);
        object = object->child;
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, -BARREL_DROP, 0.0F);
        TURN_VIA(njRotateX, work->barrel->pitch.whole - BARREL_TILT, ang);
        draw_model(object->model);
        njPopMatrix(1);
        object = object->sibling;
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, MOUNT_TOP, 0.0F);
        draw_model(object->model);
        njPopMatrix(1);
        object = object->sibling;
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, -BARREL_DROP, -BARREL_BACK);
        TURN_VIA(njRotateX, work->barrel->pitch.whole - BARREL_TILT, ang);
        draw_model(object->model);
        njPopMatrix(1);
        njPopMatrix(1);
        break;
    }
    njPopMatrix(1);
}

/* cannon_b_shell's arrive. it hits the target, and knocks back player 0
 * standing on it */
static void cannon_b_arrive(struct sky_deck_target *target, NJS_POINT3 *pos,
                            NJS_VECTOR *speed)
{
    if (target && target->work && target->hit) {
        target->hit(pos, speed, target->work);
        if (target->state & TARGET_STOOD_ON) {
            sky_deck_knock_back(target->work);
        }
    }
}

/* the sequence step that runs a script's command index. TRUE moves on to
 * the next command, and FALSE runs this one again. an open waits for its
 * launcher to be closed, and a close for it to be open. a fire waits for it
 * to be open, its recoil settled. a command whose launcher or platform has
 * no unbroken target by its id is skipped. COMMAND_END clears keep, which
 * frees the script's slot */
static Bool cannon_b_script_step(void *script, int index, Bool *keep)
{
    Bool done = TRUE;
    struct cannon_b_command *command =
        &((struct cannon_b_command *)script)[index];
    struct sky_deck_target *launcher;
    struct sky_deck_target *target;
    struct cannon_b_work *work;

    if (command->op == COMMAND_END) {
        *keep = FALSE;
        return FALSE;
    }
    launcher = sky_deck_target_find_id_masked(TARGET_LAUNCHER, 0, -2,
                                              command->launcher);
    if (launcher) {
        work = (struct cannon_b_work *)launcher->work;
        switch (command->op) {
        case COMMAND_OPEN:
            if (!cannon_b_open(work)) {
                done = FALSE;
            }
            break;
        case COMMAND_CLOSE:
            if (!cannon_b_close(work)) {
                done = FALSE;
            }
            break;
        case COMMAND_FIRE:
            target = sky_deck_target_find_id_masked(TARGET_PLATFORM, 0, -2,
                                                    command->platform);
            if (target && !cannon_b_fire(&target->work->pos, work)) {
                done = FALSE;
            }
            break;
        }
    }
    return done;
}
