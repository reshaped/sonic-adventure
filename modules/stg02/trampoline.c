/* O POLINE: a trampoline. it throws a player higher on each bounce in a row, up
 * to the third */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg02/trampoline.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_OBJECT windy_trampoline_object;
extern NJS_OBJECT windy_trampoline_spring_object;
extern NJS_OBJECT windy_trampoline_stretched_object;

/* a player lands on the trampoline within LAND_REACH of its center in x and
 * z. it must also be between LAND_LOW and LAND_HIGH above it. a landed player
 * coming down leaves the trampoline once it strays STAY_REACH or farther */
#define LAND_REACH 31.0F
#define STAY_REACH 29.0F
#define LAND_LOW 19.0F
#define LAND_HIGH 95.0F

/* the trampoline squashes the player for SQUASH_STEPS steps. its spring shakes
 * until step SHAKE_STEPS of the bounce */
#define SQUASH_STEPS 6
#define SHAKE_STEPS 40

/* by bounce, the height above the trampoline at which a player coming down
 * starts the squash */
#define SQUASH_FIRST 19.0F
#define SQUASH_SECOND 22.0F
#define SQUASH_LATER 25.0F
/* by bounce, the height above the trampoline the squash first holds the
 * player at */
#define HOLD_FIRST 19.0F
#define HOLD_SECOND 20.0F
#define HOLD_LATER 21.0F
/* the squash then sinks the player this far a step, on its first bounce and
 * on the rest */
#define SINK_FIRST 1.2F
#define SINK_LATER 1.4F
/* the squash draws the player CENTER_PULL a step toward the center, while it
 * is more than CENTER_NEAR away */
#define CENTER_NEAR 3.0F
#define CENTER_PULL 1.5F
/* by bounce, the speed upward the trampoline throws the player at. on the
 * first bounce, it lifts the player FIRST_LIFT a step before the throw */
#define THROW_FIRST 3.0F
#define THROW_SECOND 5.0F
#define THROW_LATER 7.0F
#define FIRST_LIFT 2.0F
/* the spring's y scale grows SPRING_GROW a step during the squash */
#define SPRING_GROW 0.1F
/* a player in the air lands for its next bounce at this height above the
 * trampoline */
#define RETURN_HEIGHT 39.0F

#define SE_TRAMPOLINE 55

/* the trampoline's work->mode. in TRAMPOLINE_GONE the trampoline destroys
 * itself. no step here sets that mode */
enum trampoline_mode {
    TRAMPOLINE_START = 0,
    TRAMPOLINE_WAIT = 1,
    TRAMPOLINE_LANDED = 3,
    TRAMPOLINE_BOUNCE = 4, /* the trampoline squashes the player, then
                            * throws it */
    TRAMPOLINE_AIR = 5,
    TRAMPOLINE_GONE = 6
};

/* the trampoline's work->vars. work->scl is the spring's scale, and work->ang.z
 * the phase of its shake */
struct trampoline_vars {
    Uint16 steps; /* since the squash began */
    Uint16 bounce; /* bounces in a row so far */
    Uint8 unk_04[4];
    Float player_y; /* the player's height as the spring squashes */
};

#define VARS(work) ((struct trampoline_vars *)&(work)->vars)
/* @hack type: steps and bounce cleared as one word, vars.l[0], which SHC stores
 * with one MOV.L, as at 0x0c90a17c; without it, 40 of 216 words differ */
#define CLEAR_COUNTS(work) ((work)->vars.l[0] = 0)
/* the player the trampoline holds, 0 or 1 */
#define PLAYER(work) ((work)->submode)

static struct collision_shape windy_trampoline_shape = {SHAPE_CYLINDER,
                                                        0,
                                                        PUSH_ALL |
                                                            PUSH_FIRM(PUSH_ALL),
                                                        {0.0F, 0.0F, 0.0F},
                                                        20.0F,
                                                        10.0F,
                                                        0.0F,
                                                        {0, 0, 0},
                                                        0,
                                                        NULL};

static void windy_trampoline_reset(struct task_work *work);
static void windy_trampoline_wait(struct task *task);
static void windy_trampoline_land(struct task_work *work);
static void windy_trampoline_bounce(struct task_work *work);
static void windy_trampoline_air(struct task_work *work);
static void windy_trampoline_draw(struct task *task);

/* clear the trampoline's counts and return it to waiting once player 0 comes
 * near. player 0 must be within LAND_REACH in x and z, and above LAND_LOW.
 * the step calls it while player 1 holds the trampoline */
static void windy_trampoline_player_0(struct task *task)
{
    struct task_work *work = task->work;
    struct task *player = player_get_task(0);
    struct task_work *on;
    Float dx;
    Float dz;
    Float distance;

    if (player != NULL) {
        on = player->work;
        dx = on->pos.x - work->pos.x;
        dz = on->pos.z - work->pos.z;
        distance = njSqrt((dx * dx) + (dz * dz));
        if (distance < LAND_REACH && on->pos.y > work->pos.y + LAND_LOW) {
            work->mode = TRAMPOLINE_WAIT;
            CLEAR_COUNTS(work);
        }
    }
}

void windy_trampoline_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case TRAMPOLINE_START:
        windy_trampoline_reset(work);
        collision_init(task, &windy_trampoline_shape, COLLISION_OBJECT, 1, 1);
        task->draw = windy_trampoline_draw;
        break;
    case TRAMPOLINE_WAIT:
        windy_trampoline_wait(task);
        collision_register(work);
        break;
    case TRAMPOLINE_LANDED:
        if (PLAYER(work) == 1) {
            windy_trampoline_player_0(task);
        }
        windy_trampoline_land(work);
        break;
    case TRAMPOLINE_BOUNCE:
        if (PLAYER(work) == 1) {
            windy_trampoline_player_0(task);
        }
        windy_trampoline_bounce(work);
        break;
    case TRAMPOLINE_AIR:
        if (PLAYER(work) == 1) {
            windy_trampoline_player_0(task);
        }
        windy_trampoline_air(work);
        break;
    case TRAMPOLINE_GONE:
        task_destroy(task);
        return;
    default:
        work->mode = TRAMPOLINE_START;
        break;
    }
    windy_trampoline_draw(task);
}

static void windy_trampoline_reset(struct task_work *work)
{
    PLAYER(work) = 0;
    work->ang.z = 0;
    work->scl.x = 1.0F;
    work->scl.y = 0.0F;
    work->scl.z = 1.0F;
    CLEAR_COUNTS(work);
    work->mode = TRAMPOLINE_WAIT;
}

/* outside the set editor, take a player that lands on the trampoline. when both
 * players land, player 1 takes it: the loop tests it last */
static void windy_trampoline_wait(struct task *task)
{
    struct task_work *work = task->work;
    struct task *player;
    struct task_work *on;
    int i;
    Float dx;
    Float dz;
    Float distance;

    if (!placement_editing(task)) {
        for (i = 0; i < 2; i++) {
            player = player_get_task(i);
            if (player == NULL) {
                continue;
            }
            on = player->work;
            dx = on->pos.x - work->pos.x;
            dz = on->pos.z - work->pos.z;
            distance = njSqrt((dx * dx) + (dz * dz));
            if (distance < LAND_REACH) {
                if (on->pos.y > work->pos.y + LAND_LOW &&
                    on->pos.y < work->pos.y + LAND_HIGH) {
                    work->mode = TRAMPOLINE_LANDED;
                    PLAYER(work) = i;
                }
            }
        }
    }
}

/* start the squash once the player comes down to its bounce's squash
 * height. hold the player at its bounce's hold height. once the player
 * strays STAY_REACH away in x and z, the trampoline clears its counts and
 * waits again */
static void windy_trampoline_land(struct task_work *work)
{
    struct task *player = player_get_task(PLAYER(work));
    struct task_work *on = player->work;
    Float dx = on->pos.x - work->pos.x;
    Float dz = on->pos.z - work->pos.z;
    Float distance = njSqrt((dx * dx) + (dz * dz));

    if (distance >= STAY_REACH) {
        work->mode = TRAMPOLINE_WAIT;
        CLEAR_COUNTS(work);
    } else if (VARS(work)->bounce == 0) {
        if (on->pos.y <= work->pos.y + SQUASH_FIRST) {
            work->mode = TRAMPOLINE_BOUNCE;
            player_squash(PLAYER(work), 1);
            on->pos.y = VARS(work)->player_y = work->pos.y + HOLD_FIRST;
        }
    } else if (VARS(work)->bounce == 1) {
        if (on->pos.y <= work->pos.y + SQUASH_SECOND) {
            work->mode = TRAMPOLINE_BOUNCE;
            player_squash(PLAYER(work), 2);
            on->pos.y = VARS(work)->player_y = work->pos.y + HOLD_SECOND;
        }
    } else {
        if (on->pos.y <= work->pos.y + SQUASH_LATER) {
            work->mode = TRAMPOLINE_BOUNCE;
            player_squash(PLAYER(work), 3);
            on->pos.y = VARS(work)->player_y = work->pos.y + HOLD_LATER;
        }
    }
}

/* squash the player for SQUASH_STEPS steps, then throw it up by its bounce's
 * throw speed. the spring grows during the squash, and swings after it until
 * step SHAKE_STEPS. the trampoline then waits for the player in the air */
static void windy_trampoline_bounce(struct task_work *work)
{
    struct task_work *on = player_work[PLAYER(work)];
    Float dx;
    Float dz;
    Float distance;

    VARS(work)->steps++;
    if (VARS(work)->steps <= SQUASH_STEPS) {
        if (VARS(work)->bounce == 0) {
            VARS(work)->player_y -= SINK_FIRST;
        } else {
            VARS(work)->player_y -= SINK_LATER;
        }
        on->pos.y = VARS(work)->player_y;
        dx = work->pos.x - on->pos.x;
        dz = work->pos.z - on->pos.z;
        distance = njSqrt((dx * dx) + (dz * dz));
        if (distance > CENTER_NEAR) {
            on->pos.x += (dx / distance) * CENTER_PULL;
            on->pos.z += (dz / distance) * CENTER_PULL;
        }
    } else if (VARS(work)->steps == SQUASH_STEPS + 1) {
        if (VARS(work)->bounce == 0) {
            VARS(work)->player_y += FIRST_LIFT;
            on->pos.y = VARS(work)->player_y;
        } else if (VARS(work)->bounce == 1) {
            sound_play(SE_TRAMPOLINE, 0, 0, 0);
            player_bounce_higher(PLAYER(work), 2, 0.0F, THROW_SECOND, 0.0F);
        } else {
            sound_play(SE_TRAMPOLINE, 0, 0, 0);
            player_bounce_higher(PLAYER(work), 3, 0.0F, THROW_LATER, 0.0F);
        }
    } else if (VARS(work)->steps == SQUASH_STEPS + 2) {
        if (VARS(work)->bounce == 0) {
            sound_play(SE_TRAMPOLINE, 0, 0, 0);
            player_bounce_higher(PLAYER(work), 1, 0.0F, THROW_FIRST, 0.0F);
        }
    }
    if (VARS(work)->steps <= SQUASH_STEPS) {
        work->scl.y += SPRING_GROW;
    } else if (VARS(work)->steps <= SHAKE_STEPS) {
        work->ang.z += 0x2000;
        work->scl.y = (60 - VARS(work)->steps) * njCos(work->ang.z) * 0.02F;
    } else {
        work->scl.y = 0.0F;
        work->ang.z = 0;
        work->mode = TRAMPOLINE_AIR;
        VARS(work)->steps = 0;
    }
}

/* count the next bounce once the player comes back down to RETURN_HEIGHT.
 * a player that lands on the ground or an object first ends the run of
 * bounces. the trampoline then clears its counts and waits again */
static void windy_trampoline_air(struct task_work *work)
{
    struct task_work *on = player_work[PLAYER(work)];

    if (on->flags & PLAYER_ON_GROUND) {
        CLEAR_COUNTS(work);
        work->mode = TRAMPOLINE_WAIT;
        return;
    }
    if (on->pos.y <= work->pos.y + RETURN_HEIGHT) {
        work->mode = TRAMPOLINE_LANDED;
        VARS(work)->bounce++;
    }
}

static void windy_trampoline_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *base = &windy_trampoline_object;
    NJS_OBJECT *spring = &windy_trampoline_spring_object;
    NJS_OBJECT *stretched = &windy_trampoline_stretched_object;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njPushMatrix(NULL);
    if (work->scl.y >= 0.0F) {
        njTranslate(NULL, spring->pos[0], spring->pos[1], spring->pos[2]);
        njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
        draw_model(spring->model);
    } else {
        njTranslate(NULL, spring->pos[0], spring->pos[1], spring->pos[2]);
        njScale(NULL, -work->scl.x, -work->scl.y, -work->scl.z);
        njScale(NULL, 1.0F, 1.8F, 1.0F);
        draw_model(stretched->model);
    }
    njPopMatrix(1);
    draw_model(base->model);
    njPopMatrix(1);
}
