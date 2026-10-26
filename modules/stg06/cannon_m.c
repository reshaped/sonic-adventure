/* CANNON_M, the cannon that shells the nearest player. it opens once the
 * floor under that player comes near. the cannon aims at a mark, the point
 * its shell will land on. the mark starts ahead of the cannon, then moves
 * toward that floor. the cannon kicks once it has aimed long enough, if its
 * barrel can point at the mark. it also kicks once the player gets away. as
 * the barrel stops moving back, the cannon fires at the mark if the barrel
 * can point there. the barrel then turns back to rest, and the cannon
 * closes. the cannon also closes if the player gets away before the mark
 * moves. a hit breaks it, and so does the land's collapse */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "effect/explosion.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg06/cannon_m.h"
#include "stg06/collapse.h"
#include "stg06/debris.h"
#include "stg06/models.h"
#include "stg06/shell.h"
#include "stg06/sky_deck.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT cannon_m_object;
extern NJS_MOTION cannon_m_motion;

/* it opens once the floor under the nearest player comes within
 * CANNON_NEAR. it holds while that floor stays within CANNON_FAR. it aims
 * while the floor also stays within CANNON_FAR_Y above or below it */
#define CANNON_NEAR 280.0F
#define CANNON_FAR (CANNON_NEAR * 1.2F)
#define CANNON_FAR_Y (25.0F * 1.2F)
/* it aims from AIM_HEIGHT above its position, and the barrel pivots
 * PIVOT_HEIGHT above it */
#define AIM_HEIGHT 34.0F
#define PIVOT_HEIGHT 50.0F
/* each step it aims, the mark moves MARK_SHARE of the way to the player's
 * floor. it moves at most MARK_STEP */
#define MARK_SHARE 0.4F
#define MARK_STEP 2.6F
/* it leaves the first mark once its timer passes HOLD_STEPS, and kicks
 * once the timer passes AIM_STEPS */
#define HOLD_STEPS 20
#define AIM_STEPS 100
/* the barrel's pitch grows as the barrel points lower. PITCH_LEVEL is the pitch
 * cannon_m_aim gives a shot that leaves level. it points the barrel 5 degrees
 * above level. PITCH_LOWEST, 20 degrees past it, is as low as the barrel
 * points */
#define PITCH_LEVEL NJM_DEG_ANG(85)
#define PITCH_LOWEST NJM_DEG_ANG(105)
/* once the barrel's speed is within SETTLED of 0, the barrel turns back to
 * rest. it turns RETURN_SPEED a step, in yaw and in pitch */
#define RETURN_SPEED NJM_DEG_ANG(3)
#define SETTLED 0.5F
/* as it kicks, the barrel's speed becomes RECOIL_KICK, sending it back.
 * while the barrel is out, a spring takes RECOIL_OUT of its offset from its
 * speed each step. while it is back, the spring takes RECOIL_BACK. the speed
 * then keeps RECOIL_DAMP of itself */
#define RECOIL_KICK (-8.0F)
#define RECOIL_OUT 0.3F
#define RECOIL_BACK 0.7F
#define RECOIL_DAMP 0.91F
/* the frames its motion advances a step */
#define MOTION_RATE 0.5F
/* the explosion's scale as it breaks, and its height above the cannon */
#define BREAK_BLAST 18.0F
#define BREAK_UP 15.0F

#define CANNON_VIEW 70.0F

/* the spec's gravity, as the stage's other cannons give theirs */
#define SHELL_GRAVITY 0.1F

/* where the barrel points: its yaw in the world, and its pitch */
struct cannon_m_aim {
    Uint16 yaw;
    Uint16 pitch;
};

/* the cannon's own layout of task_work */
struct cannon_m_work {
    Sint8 mode;
    Sint8 unk_01;
    Uint8 unk_02;
    Uint8 unk_03;
    Sint16 flags;
    Uint16 timer;
    struct cannon_m_aim aim;
    Float recoil_speed;
    Float recoil; /* the barrel's offset from rest, out toward the muzzle */
    struct angles ang;
    NJS_POINT3 pos;
    NJS_POINT3 mark; /* the point its shell will land on */
    struct collision_work *collision;
    struct event_work *event;
};

enum cannon_m_mode {
    CANNON_WAIT, /* closed, until the floor under the nearest player is near */
    CANNON_OPEN, /* its motion plays, then it aims at the first mark */
    CANNON_HOLD, /* aimed at the first mark, while that floor stays near */
    CANNON_AIM, /* the mark closes on that floor, and the barrel follows */
    CANNON_FIRE, /* it fires once the barrel stops moving back */
    CANNON_BROKEN,
    CANNON_RETURN, /* the barrel turns back to rest */
    CANNON_CLOSE /* its motion plays backward */
};

/* the muzzle from the barrel's pivot, and the way the barrel points, both
 * in the barrel's own frame */
static const NJS_POINT3 cannon_m_muzzle = {0.0F, 33.0F, 0.0F};
static const NJS_POINT3 cannon_m_forward = {0.0F, 1.0F, 0.0F};
/* the flight cannon_m_aim plans. a shell crosses cannon_m_pace a step. its
 * speed along y gains cannon_m_gravity, which is negative, each step */
static Float cannon_m_gravity = -0.27F;
static Float cannon_m_pace = 3.9F;
/* each shot sets the spec's gravity to cannon_m_gravity before it fires, in
 * place of the -SHELL_GRAVITY here */
static struct shell_spec cannon_m_shell = {SHELL_SPOT, 0, -SHELL_GRAVITY, 0.5F,
                                           NULL};
static struct debris_spec cannon_m_debris = {&cannon_m_object,
                                             {0.0F, 0.0F, 0.0F},
                                             {0, 0, 0},
                                             {0.0F, 0.7F, 0.0F},
                                             {0.0F, 0.0F, 0.0F},
                                             500.0F,
                                             {1.0F, 1.0F, 1.0F},
                                             1.0F,
                                             50.0F,
                                             2.0F,
                                             0.98F,
                                             0,
                                             &obj_skydeck_texlist,
                                             0.0F};
static struct collision_shape cannon_m_shape = {SHAPE_CAPSULE | SHAPE_HURTABLE,
                                                0,
                                                PUSH_ALL | PUSH_FIRM(PUSH_ALL),
                                                {0.0F, 17.5F, 0.0F},
                                                38.0F,
                                                17.5F,
                                                0.0F,
                                                {0, 0, 0},
                                                0,
                                                NULL};
/* the speed and acceleration of its pieces as a hit breaks it, and as the
 * collapse does */
static const NJS_VECTOR cannon_m_break_speed = {0.0F, 1.0F, 0.0F};
static const NJS_VECTOR cannon_m_break_accel = {0.0F, -0.1F, 0.0F};
static const NJS_VECTOR cannon_m_collapse_speed = {0.0F, 1.0F, 0.0F};
static const NJS_VECTOR cannon_m_collapse_accel = {0.0F, -0.1F, 0.0F};

static void cannon_m_init(struct task *task);
static void cannon_m_destroy(struct task *task);
static void cannon_m_run(struct task *task);
static void cannon_m_draw(struct task *task);

/* set target to the nearest player's x and z, at the height of the floor
 * under that player */
static void cannon_m_find_target(NJS_POINT3 *target, struct cannon_m_work *work)
{
    int nearest;
    struct player *player = player_get(nearest = player_nearest(&work->pos));
    struct task_work *player_task = player_work[nearest];

    target->x = player_task->pos.x;
    target->y = player->shadow.below;
    target->z = player_task->pos.z;
}

/* set point to the muzzle, with the barrel at pitch, and at yaw in the
 * world. cannon_m_gravity is then subtracted from point's y, which raises
 * point 0.27 above the muzzle */
static void cannon_m_find_muzzle(const Angle *pitch, const Angle *yaw,
                                 NJS_POINT3 *point, struct cannon_m_work *work)
{
    NJS_POINT3 muzzle;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    NJS_POINT3 forward;
    Float *at;
    Float x;
    Float y;
    Float z;

    muzzle = cannon_m_muzzle;
    /* @hack redundant: forward, copied from cannon_m_forward and never read
     * (0x0c90f604 to 0x0c90f614); without it, 72 of 87 words differ and the
     * object ends 32 bytes early */
    forward = cannon_m_forward;
    /* @hack spelling: the position read through a cursor, which steps R4 by
     * post-increment (0x0c90f5f0 to 0x0c90f622); read by member, 78 of 87
     * words differ */
    at = VECTOR_FLOATS(&work->pos);
    x = *at++;
    y = *at++;
    z = *at;
    njPushMatrix(&_unit_matrix_);
    njTranslate(NULL, x, y + PIVOT_HEIGHT, z);
    TURN(njRotateY, *yaw);
    TURN(njRotateX, *pitch);
    njCalcPoint(NULL, &muzzle, point);
    njPopMatrix(1);
    point->y -= cannon_m_gravity;
}

/* set speed to carry a shell from start to end at cannon_m_pace across a
 * step. set yaw to face end, and pitch to point at the top of the arc. a
 * shell that starts falling takes pitch along speed instead. returns 1.
 * where the barrel would point lower than PITCH_LOWEST, it sets pitch to
 * PITCH_LOWEST and returns 0 */
static int cannon_m_aim(NJS_POINT3 *start, NJS_POINT3 *end, NJS_VECTOR *speed,
                        Angle *pitch, Angle *yaw)
{
    Float *from;
    Float *to;
    Float *out;
    Float steps;
    Float x;
    Float y;
    Float z;
    Float height;
    Float distance;
    Float dx;
    Float dy;
    Float dz;
    /* @hack type: pitch_low, kept at sp+4, reads back the low half of the
     * pitch, which is stored whole (0x0c90f79c, 0x0c90f7b6); tested as
     * (Sint16)*pitch, 112 of 122 words differ and the object ends 32 bytes
     * early */
    Sint16 *pitch_low = (Sint16 *)pitch;

    /* @hack spelling: start and end read a Float at a time through cursors, as
     * the image steps R6 and R4 (0x0c90f6fc to 0x0c90f710); read through the
     * members, 106 of 122 words differ */
    /* @hack spelling: each step a statement of its own, and start's x read into
     * dx first, which keeps pitch_low at sp+4 (0x0c90f6fa), y in FR4 and
     * steps in FR5 (0x0c90f720 to 0x0c90f782); stepped inside the
     * subtractions, 26 of 122 words differ, and with dx in one subtraction,
     * 22 */
    from = VECTOR_FLOATS(start);
    to = VECTOR_FLOATS(end);
    dx = *from;
    from++;
    dx = *to - dx;
    to++;
    dy = *to - *from;
    to++;
    from++;
    dz = *to - *from;
    /* @hack redundant: four never-run stores of dz, which keep dz in FR15
     * and dx in FR14 (0x0c90f6fe, 0x0c90f70e), pitch_low at sp+4
     * (0x0c90f6fa), y in FR4 and steps in FR5 (0x0c90f720 to 0x0c90f782);
     * without them, 84 of 122 words differ */
    if (0) {
        *to = dz;
        *to = dz;
        *to = dz;
        *to = dz;
    }
    steps = njSqrt((dx * dx) + (dz * dz));
    steps /= cannon_m_pace;
    x = dx / steps;
    y = (dy - (cannon_m_gravity * steps * steps * 0.5F)) / steps;
    z = dz / steps;
    /* @hack spelling: speed written a Float at a time through a cursor, as ADD
     * #4 steps R4 (0x0c90f740 to 0x0c90f750); written through the members,
     * 115 of 122 words differ and the object ends 32 bytes late */
    /* @hack spelling: each step a statement of its own, which keeps y in FR4
     * and steps in FR5 (0x0c90f720 to 0x0c90f782); as *out++, the two trade
     * and 22 of 122 words differ */
    out = VECTOR_FLOATS(speed);
    *out = x;
    out++;
    *out = y;
    out++;
    *out = z;
    /* @hack redundant: a never-run store of y, which keeps y in FR4 and steps
     * in FR5 on both paths (0x0c90f720 to 0x0c90f782); without it, the two
     * trade and 22 of 122 words differ */
    if (0) {
        *out = y;
    }
    if (y >= 0.0F) {
        steps = -y / cannon_m_gravity;
        x *= steps;
        z *= steps;
        y *= steps;
        height = y + (cannon_m_gravity * steps * steps * 0.5F);
        distance = njSqrt((x * x) + (z * z));
    } else {
        height = y;
        distance = cannon_m_pace;
    }
    *pitch =
        -(Angle)(atan2f(height, distance) * ANGLE_PER_RADIAN) + PITCH_LEVEL;
    *yaw = (Angle)(atan2f(x, z) * ANGLE_PER_RADIAN);
    if (*pitch_low > PITCH_LOWEST) {
        *pitch = PITCH_LOWEST;
        return 0;
    }
    return 1;
}

/* @hack redundant: register on task, which keeps it in R14 across the range
 * check (0x0c90f81a); without it, task is kept at sp+0, 11 of 14 words
 * differ and cannon_m_init starts 2 bytes late */
void cannon_m_step(register struct task *task)
{
    if (!placement_try_leave(task)) {
        cannon_m_init(task);
    }
}

static void cannon_m_init(struct task *task)
{
    struct cannon_m_work *work = (struct cannon_m_work *)task->work;

    if (!placement_editing(task)) {
        collision_init(task, &cannon_m_shape, COLLISION_ITEM, 1, 1);
    }
    task->draw = cannon_m_draw;
    task->step = cannon_m_run;
    task->destructor = cannon_m_destroy;
    work->timer = 0;
    work->mode = CANNON_WAIT;
    work->ang.x = 0;
    work->ang.z = 0;
}

static void cannon_m_destroy(struct task *task)
{
}

static void cannon_m_run(struct task *task)
{
    struct cannon_m_work *work = (struct cannon_m_work *)task->work;
    NJS_POINT3 point;
    NJS_VECTOR vector;
    NJS_POINT3 place;

    if (placement_try_leave(task)) {
        return;
    }
    if (work->mode == CANNON_BROKEN) {
        return;
    }
    cannon_m_draw(task);
    if (placement_editing(task)) {
        return;
    }
    work->timer++;
    switch (work->mode) {
    case CANNON_WAIT:
        /* a look every fourth step */
        if (!(work->timer & 3)) {
            Float steps;
            NJS_POINT3 *from;
            Float *at;

            cannon_m_find_target(&point, work);
            if (njDistanceP2P(&point, &work->pos) < CANNON_NEAR) {
                /* the first mark lies ahead of the cannon, at the cannon's
                 * height. its distance from the cannon is cannon_m_pace times
                 * the steps of a fall from rest. the fall runs from AIM_HEIGHT
                 * above the cannon to the player's floor. njSqrt of a negative
                 * n returns -sqrt(-n). so for a floor above that height, the
                 * steps come out negative. the mark then lies as far behind */
                /* @hack spelling: vector stored through a cursor, as ADD #4
                 * steps R4 (0x0c90f96c to 0x0c90f986); stored by member, 91 of
                 * 853 words differ */
                at = VECTOR_FLOATS(&vector);
                *at++ = 0.0F;
                *at++ = 0.0F;
                *at = cannon_m_pace;
                /* @hack spelling: place reached through from, which the image
                 * keeps in R9 (0x0c90f97a to 0x0c90f99a); by name, 86 of 853
                 * words differ */
                from = &place;
                *from = work->pos;
                from->y += AIM_HEIGHT;
                steps = njSqrt((point.y - from->y) / cannon_m_gravity * 2.0F);
                vector.z *= steps;
                njPushMatrix(&_unit_matrix_);
                njTranslateV(NULL, &work->pos);
                TURN(njRotateY, work->ang.y);
                njCalcPoint(NULL, &vector, &vector);
                njPopMatrix(1);
                work->mark = vector;
                work->mode = CANNON_OPEN;
                work->timer = 0;
            }
        }
        break;
    case CANNON_OPEN: {
        Float frame = work->timer * MOTION_RATE;

        sound_play_at_for(SE_TURRET, (int)task, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 2);
        if (frame >= cannon_m_motion.nbFrame) {
            Angle pitch;
            Angle yaw;

            point = work->pos;
            point.y += AIM_HEIGHT;
            cannon_m_aim(&point, &work->mark, &vector, &pitch, &yaw);
            work->aim.pitch = pitch;
            work->aim.yaw = yaw;
            work->mode = CANNON_HOLD;
            work->timer = 0;
        }
    } break;
    case CANNON_CLOSE: {
        Float frame = work->timer * MOTION_RATE;

        if (frame >= cannon_m_motion.nbFrame) {
            work->mode = CANNON_WAIT;
            work->timer = 0;
        }
    } break;
    case CANNON_RETURN:
        if (_builtin_fabsf(work->recoil_speed) <= SETTLED) {
            /* @hack spelling: the halves read through aim, from R4 at work + 8
             * (0x0c90fb6e to 0x0c90fb76), and stored through work; read
             * through work, 96 of 853 words differ */
            struct cannon_m_aim *aim = &work->aim;
            Sint32 tilt;
            Sint32 turn;

            /* @hack spelling: the pitch read, then -PITCH_LEVEL added, which
             * copies the half to R5 and adds -15473 from R3 (0x0c90fb72 to
             * 0x0c90fb7c); in one statement, SHC adds the half to the
             * constant and 79 of 853 words differ, and with -=, it subtracts
             * 15473 and 1 differs */
            tilt = (Sint16)aim->pitch;
            tilt += -PITCH_LEVEL;
            turn = (Sint16)aim->yaw - work->ang.y;
            if ((Sint16)tilt >= RETURN_SPEED) {
                work->aim.pitch -= RETURN_SPEED;
            }
            if ((Sint16)tilt <= -RETURN_SPEED) {
                work->aim.pitch += RETURN_SPEED;
            }
            if ((Sint16)turn >= RETURN_SPEED) {
                work->aim.yaw -= RETURN_SPEED;
            }
            if ((Sint16)turn <= -RETURN_SPEED) {
                work->aim.yaw += RETURN_SPEED;
            }
            if ((Sint16)tilt > -RETURN_SPEED && (Sint16)tilt < RETURN_SPEED &&
                (Sint16)turn > -RETURN_SPEED && (Sint16)turn < RETURN_SPEED) {
                work->mode = CANNON_CLOSE;
                work->timer = 0;
            }
        }
        break;
    case CANNON_HOLD:
        cannon_m_find_target(&point, work);
        if (njDistanceP2P(&point, &work->pos) < CANNON_FAR) {
            if (work->timer > HOLD_STEPS) {
                work->mode = CANNON_AIM;
                work->timer = 0;
            }
        } else {
            work->mode = CANNON_RETURN;
            work->timer = 0;
        }
        break;
    case CANNON_AIM:
        cannon_m_find_target(&point, work);
        if (_builtin_fabsf(work->pos.y - point.y) < CANNON_FAR_Y &&
            njDistanceP2P(&point, &work->pos) < CANNON_FAR) {
            NJS_POINT3 origin;
            NJS_VECTOR speed;
            Float length;
            Angle pitch;
            Angle yaw;
            int ready;
            Float *by;
            Float *to;
            Float *at;

            place = work->mark;
            /* @hack spelling: the step taken through cursors, which load by
             * post-increment and store as ADD #4 steps R6 (0x0c90fca0 to
             * 0x0c90fcba); by member, 188 of 853 words differ */
            /* @hack spelling: by set last, after to and at in either order,
             * which sets place's cursor up in R4 before point's in R5
             * (0x0c90fc82 to 0x0c90fc8a) and loads place's y before point's
             * (0x0c90fcaa, 0x0c90fcac); with by set earlier, 4 or 6 of 853
             * words differ */
            to = VECTOR_FLOATS(&vector);
            at = VECTOR_FLOATS(&point);
            by = VECTOR_FLOATS(&place);
            *to++ = *at++ - *by++;
            *to++ = *at++ - *by++;
            *to = *at - *by;
            vector.x *= MARK_SHARE;
            vector.y *= MARK_SHARE;
            vector.z *= MARK_SHARE;
            length = njScalor(&vector);
            if (length > MARK_STEP) {
                Float scale = MARK_STEP / length;

                vector.x *= scale;
                vector.y *= scale;
                vector.z *= scale;
            }
            /* @hack spelling: the step added through cursors (0x0c90fd12 to
             * 0x0c90fd3e); by member, 426 of 853 words differ and the object
             * ends 32 bytes late */
            /* @hack spelling: the first sum written out, and the middle one
             * post-incremented, for which SHC copies the cursor to R3
             * (0x0c90fd28); with += for the first, the walks here and above
             * take other registers of R4 to R6 (0x0c90fc82 to 0x0c90fd3e) and
             * 24 of 853 words differ, and post-incremented throughout, 26 */
            to = VECTOR_FLOATS(&place);
            by = VECTOR_FLOATS(&vector);
            *to = *to + *by;
            by++;
            to++;
            *to++ += *by;
            by++;
            *to += *by;
            work->mark = place;
            origin = work->pos;
            origin.y += AIM_HEIGHT;
            ready = cannon_m_aim(&origin, &work->mark, &speed, &pitch, &yaw);
            work->aim.pitch = pitch;
            work->aim.yaw = yaw;
            cannon_m_find_muzzle(&pitch, &yaw, &origin, work);
            if (ready && work->timer > AIM_STEPS) {
                sound_play_at(SE_CANNON_FIRE, 0, 0, 64, work->pos.x,
                              work->pos.y, work->pos.z);
                work->recoil_speed = RECOIL_KICK;
                work->mode = CANNON_FIRE;
                work->timer = 0;
            }
        } else {
            sound_play_at(SE_CANNON_FIRE, 0, 0, 64, work->pos.x, work->pos.y,
                          work->pos.z);
            work->mode = CANNON_FIRE;
            work->timer = 0;
            work->recoil_speed = RECOIL_KICK;
        }
        break;
    case CANNON_FIRE:
        if (work->recoil_speed >= 0.0F) {
            Angle pitch;
            Angle yaw;

            point = work->pos;
            point.y += AIM_HEIGHT;
            /* where the barrel can point at the mark, it aims again from
             * the muzzle and fires from there */
            if (cannon_m_aim(&point, &work->mark, &place, &pitch, &yaw)) {
                cannon_m_find_muzzle(&pitch, &yaw, &vector, work);
                cannon_m_aim(&vector, &work->mark, &place, &pitch, &yaw);
                cannon_m_shell.gravity = cannon_m_gravity;
                sky_deck_shell_fire(&vector, &work->mark, &place,
                                    &cannon_m_shell);
            }
            work->mode = CANNON_RETURN;
            work->timer = 0;
        }
        break;
    }
    work->recoil_speed += work->recoil > 0.0F ? -(work->recoil * RECOIL_OUT)
                                              : -(work->recoil * RECOIL_BACK);
    work->recoil_speed *= RECOIL_DAMP;
    work->recoil += work->recoil_speed;
    if ((work->flags & COLLISION_DAMAGED) && work->mode != CANNON_BROKEN) {
        point = cannon_m_break_speed;
        vector = cannon_m_break_accel;
        sky_deck_debris_create(&work->pos, &work->ang, &point, &vector,
                               work->pos.y, &cannon_m_debris);
        explosion_create_at(work->pos.x, work->pos.y + BREAK_UP, work->pos.z,
                            BREAK_BLAST);
        sound_play_at(SE_CANNON_BREAK, 0, 0, 32, work->pos.x, work->pos.y,
                      work->pos.z);
        work->mode = CANNON_BROKEN;
        work->timer = 0;
        sky_deck_hop((struct task_work *)work);
        placement_kill(task);
    } else if (collapse_reached(work->pos.x, work->pos.y, work->pos.z)) {
        point = cannon_m_collapse_speed;
        vector = cannon_m_collapse_accel;
        sky_deck_debris_create(&work->pos, &work->ang, &point, &vector,
                               work->pos.y, &cannon_m_debris);
        explosion_create_at(work->pos.x, work->pos.y + BREAK_UP, work->pos.z,
                            BREAK_BLAST);
        sound_play_at(SE_CANNON_BREAK, 0, 0, 32, work->pos.x, work->pos.y,
                      work->pos.z);
        work->mode = CANNON_BROKEN;
        work->timer = 0;
        placement_kill(task);
    } else if (work->mode != CANNON_BROKEN) {
        collision_register((struct task_work *)work);
    }
}

/* while it aims, the spot its shell will land on shows at the mark. while
 * it waits, opens or closes, it draws its motion. otherwise it draws the
 * tree's first three nodes without their own moves. the barrel turns to the
 * aim and moves out by its recoil. its two siblings turn with its yaw */
static void cannon_m_draw(struct task *task)
{
    struct cannon_m_work *work = (struct cannon_m_work *)task->work;
    NJS_OBJECT *object = &cannon_m_object;
    NJS_MOTION *motion = &cannon_m_motion;
    Angle ang;
    Angle yaw;
    NJS_OBJECT *part;
    Float frame;
    Float last;

    /* @hack redundant: an if (0.0F) block that never runs, whose store
     * through motion gives motion R13, the object R12 (0x0c910136,
     * 0x0c910138) and njPopMatrix's address R11 (0x0c9101ac); without it,
     * the three take R12, R11 and R13, and 27 of 232 words differ. tested as
     * 0, it puts placement_try_leave's address in R2, not R3 (0x0c910134),
     * and 2 differ */
    if (0.0F) {
        motion->nbFrame = 0;
    }
    if (placement_try_leave(task)) {
        return;
    }
    if (work->mode == CANNON_BROKEN) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, CANNON_VIEW)) {
        return;
    }
    if (work->mode == CANNON_AIM) {
        sky_deck_spot_draw(&work->mark, 1.0F, 1.0F, 1.0F);
    }
    njSetTexture(&obj_skydeck_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    switch (work->mode) {
    case CANNON_WAIT:
        draw_motion(object, motion, 0.0F);
        break;
    case CANNON_CLOSE:
        frame = work->timer * MOTION_RATE;
        frame = frame > motion->nbFrame - 1.0F ? 0.0F
                                               : motion->nbFrame - frame - 1.0F;
        draw_motion(object, motion, frame);
        break;
    case CANNON_OPEN:
        frame = work->timer * MOTION_RATE;
        last = motion->nbFrame - 1.0F;
        if (frame > last) {
            frame = last;
        }
        draw_motion(object, motion, frame);
        break;
    default:
        yaw = work->aim.yaw - work->ang.y;
        draw_model(object->model);
        object = object->child;
        draw_model(object->model);
        object = object->child;
        draw_model(object->model);
        object = object->child;
        njPushMatrix(NULL);
        if (yaw) {
            njRotateY(NULL, yaw);
        }
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, PIVOT_HEIGHT, 0.0F);
        /* @hack spelling: the pitch through ang, which widens the half once,
         * before the test, and stores it whole (0x0c9102be to 0x0c9102c6);
         * through TURN, SHC stores the half and widens it again after the
         * reload, and 60 of 232 words differ */
        TURN_VIA(njRotateX, work->aim.pitch, ang);
        njTranslate(NULL, 0.0F, work->recoil, 0.0F);
        draw_model(object->model);
        njPopMatrix(1);
        part = object->sibling;
        njPushMatrix(NULL);
        njTranslateV(NULL, (NJS_VECTOR *)part->pos);
        draw_model(part->model);
        njPopMatrix(1);
        part = part->sibling;
        njPushMatrix(NULL);
        njTranslateV(NULL, (NJS_VECTOR *)part->pos);
        draw_model(part->model);
        njPopMatrix(1);
        njPopMatrix(1);
        break;
    }
    njPopMatrix(1);
}
