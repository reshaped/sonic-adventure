/* the shots the sweeping turrets fire, in one list that one task steps. one
 * hurt sphere follows the shot nearest the players, and a second follows
 * another shot */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "effect/bomb.h"
#include "effect/explosion.h"
#include "player/player.h"
#include "stg06/shots.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 11
#include "stg06/shots/text.inc"

/* the speed a shot in flight gains downward each step */
#define SHOT_GRAVITY 0.01F
/* a shot's sprite turns SHOT_SPIN a step. the parity of its angle picks the way
 * it turns. an odd angle turns one way, and an even angle the other. SHOT_SPIN
 * is even, so each shot keeps the way its first angle's parity gives it */
#define SHOT_SPIN 9000
/* a shot's first angle is random, from 0 up to FIRST_ANGLE_MAX, two turns */
#define FIRST_ANGLE_MAX 0x1ffff
/* the steps a shot lies after it lands, before it's freed */
#define SHOT_LIE 50.0F
/* a hurt sphere's radius on a shot in flight, and on a landed shot. a
 * landed shot's sphere sits LANDED_RISE above it */
#define SHOT_RADIUS 2.0F
#define LANDED_RADIUS 5.0F
#define LANDED_RISE 2.5F
#define SHOT_SHAPES 2
/* a hurt sphere's attack level */
#define SHOT_ATTACK 2

/* a shot in the list shots heads. flight counts down a step at a time. the
 * shot moves while flight is above 0. it is freed once flight is below
 * -SHOT_LIE. angle is its sprite's */
struct shot {
    NJS_POINT3 pos;
    NJS_VECTOR velocity;
    Float flight;
    Angle angle;
    struct shot *prev;
    struct shot *next;
};

/* the hurt spheres, which shots_step moves onto two of the shots */
static struct collision_shape shots_shapes[SHOT_SHAPES] = {
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHOT_ATTACK | GUARD(3) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     SHOT_RADIUS,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHOT_ATTACK | GUARD(3) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     SHOT_RADIUS,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};
static NJS_TEXNAME shots_textures[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST shots_texlist = {
#include "stg06/shots/shots_texlist.inc"
};
static NJS_TEXANIM shots_texanim[1] = {
#include "stg06/shots/shots_texanim.inc"
};
/* the task every shot shares, or NULL, and the list of shots, newest
 * first */
static struct task *shots_task = NULL;
static struct shot *shots = NULL;

static void shots_init(struct task *task);
static void shot_free(struct shot *shot);
static void shots_destroy(struct task *task);
static void shots_draw(struct task *task);
static void shots_step(struct task *task);

/* the first step of the shots' task. unless the set editor holds the task,
 * it gives the task the hurt spheres */
void shots_init(struct task *task)
{
    struct task_work *work = task->work;
    if (!placement_editing(task)) {
        collision_init(task, shots_shapes, COLLISION_OBJECT, 1, SHOT_SHAPES);
    }
    task->draw = shots_draw;
    task->step = shots_step;
    task->destructor = shots_destroy;
    work->timer = 0;
}

/* unlink a shot from the list and free it */
void shot_free(struct shot *shot)
{
    if (!shot->prev) {
        shots = shot->next;
    } else {
        shot->prev->next = shot->next;
    }
    if (shot->next) {
        shot->next->prev = shot->prev;
    }
    heap_free(shot);
}

/* free every shot, and forget the task if it is still shots_task */
void shots_destroy(struct task *task)
{
    struct shot *shot = shots;
    struct shot *next;
    while (shot) {
        next = shot->next;
        shot_free(shot);
        shot = next;
    }
    if (shots_task == task) {
        shots_task = NULL;
    }
}

/* draw every shot in flight as an additive sprite, whichever task the draw
 * is for. it draws on a frame's first step only */
void shots_draw(struct task *task)
{
    struct shot *shot = shots;
    NJS_SPRITE sprite;
    if (!frame_step) {
        sprite.tlist = &shots_texlist;
        sprite.tanim = shots_texanim;
        sprite.sx = 5.0F * 0.3F;
        sprite.sy = 3.5F * 0.3F;
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        while (shot) {
            if (shot->flight > 0.0F) {
                sprite.ang = shot->angle;
                sprite.p = shot->pos;
                njDrawSprite3D(&sprite, 0, NJD_SPRITE_ANGLE | NJD_SPRITE_ALPHA);
            }
            shot = shot->next;
        }
    }
}

/* draw the shots, then spin and move them. a shot in flight falls. on the step
 * it lands, it makes a white puff half the time. the task ends once a step
 * starts with no shot. unless the set editor holds the task, the step then
 * places the hurt spheres. a touch in the last check sets off a puff and an
 * explosion at the first sphere. as the step walks the list, nearest holds the
 * shot nearest player 0 or 1 so far. the first sphere goes onto the last shot
 * nearest holds. the second goes onto previous, the shot nearest held before
 * that one, if any */
void shots_step(struct task *task)
{
    struct task_work *work = task->work;
    struct shot *shot = shots;
    struct shot *nearest = NULL;
    struct shot *previous = NULL;
    struct shot *next;
    struct collision_work *collision;
    Float nearest_squared = 0.0F;
    char *position;
    char *velocity;
    Float *player_position;
    struct task_work *player;
    Angle angle;
    Float x;
    Float y;
    Float z;
    Float vx;
    Float vy;
    Float vz;
    Float dx;
    Float dy;
    Float dz;
    Float squared;
    Float squared_1;

    if (!shot) {
        task_kill(task);
    }
    shots_draw(task);
    while (shot) {
        next = shot->next;
        angle = shot->angle;
        if (angle & 1) {
            angle += SHOT_SPIN;
        } else {
            angle -= SHOT_SPIN;
        }
        shot->angle = angle;
        /* @hack spelling: pos and velocity read a Float at a time through byte
         * cursors and pos stored back in reverse, as the image's increments and
         * decrements walk them; through the members, the function runs 32 bytes
         * long and 248 of 299 words differ. with only velocity.y's add through
         * its member, 220 differ */
        position = (char *)&shot->pos;
        velocity = (char *)&shot->velocity;
        x = *(Float *)position;
        position += sizeof(Float);
        y = *(Float *)position;
        position += sizeof(Float);
        z = *(Float *)position;
        vx = *(Float *)velocity;
        velocity += sizeof(Float);
        vy = *(Float *)velocity;
        velocity += sizeof(Float);
        vz = *(Float *)velocity;
        if (shot->flight > 0.0F) {
            x += vx;
            y += vy;
            z += vz;
            *(Float *)(velocity - sizeof(Float)) += -SHOT_GRAVITY;
            *(Float *)position = z;
            position -= sizeof(Float);
            *(Float *)position = y;
            position -= sizeof(Float);
            *(Float *)position = x;
        }
        player = player_work[0];
        /* @hack spelling: the player's pos walked as Floats through
         * VECTOR_FLOATS, for the image's postincrement loads; through the
         * members, 209 of 299 words differ */
        player_position = VECTOR_FLOATS(&player->pos);
        dx = *player_position++;
        dy = *player_position++;
        dz = *player_position;
        dx -= x;
        dy -= y;
        dz -= z;
        squared = (dx * dx) + (dy * dy) + (dz * dz);
        if (player_work[1]) {
            player = player_work[1];
            /* @hack spelling: the second player's pos walked the same way;
             * through the members, 190 of 299 words differ */
            player_position = VECTOR_FLOATS(&player->pos);
            dx = *player_position++;
            dy = *player_position++;
            dz = *player_position;
            dz -= z;
            dx -= x;
            dy -= y;
            squared_1 = (dx * dx) + (dy * dy) + (dz * dz);
            if (squared > squared_1) {
                squared = squared_1;
            }
        }
        if (shot->flight < 1.0F) {
            if (shot->flight >= 0.0F) {
                if ((Float)rand() * RAND_SCALE < 0.5F) {
                    bomb_create(&shot->pos, 0.3F);
                }
            } else if (shot->flight < -SHOT_LIE) {
                shot_free(shot);
                shot = NULL;
            }
        }
        if (shot) {
            shot->flight -= 1.0F;
            if (nearest_squared > squared || !nearest) {
                nearest_squared = squared;
                previous = nearest;
                nearest = shot;
            }
        }
        shot = next;
    }
    if (placement_editing(task)) {
        return;
    }
    /* @hack redundant: a never-run store of previous, which SHC counts: it
     * keeps work in R9 and previous in R10 (0x0c91f140, 0x0c91f144), and the
     * second player's dy in FR9 and dz in FR7 (0x0c91f228, 0x0c91f22e);
     * without it, each pair trades and 22 of 299 words differ */
    if (0) {
        previous = NULL;
    }
    collision = work->collision;
    if (collision->flags & COLLISION_HIT) {
        bomb_create(&collision->shapes->center, 0.3F);
        explosion_create_at(collision->shapes->center.x,
                            collision->shapes->center.y,
                            collision->shapes->center.z, 0.5F);
    }
    if (nearest) {
        collision->shapes->center = nearest->pos;
        work->pos = nearest->pos;
        if (nearest->flight <= 0.0F) {
            collision->shapes->center.y += LANDED_RISE;
            collision->shapes->radius = LANDED_RADIUS;
        } else {
            collision->shapes->radius = SHOT_RADIUS;
        }
    }
    if (previous) {
        collision->shapes[1].center = previous->pos;
        /* @bug logic: the second sphere's size tests nearest's flight, not
         * previous's own (FMOV.S @(R0,R13),FR2 at 0x0c91f34c, R13 holding
         * nearest). previous's sphere takes the landed size while nearest
         * has landed, and the size in flight while nearest flies */
        if (nearest->flight <= 0.0F) {
            collision->shapes[1].center.y += LANDED_RISE;
            collision->shapes[1].radius = LANDED_RADIUS;
        } else {
            collision->shapes[1].radius = SHOT_RADIUS;
        }
    }
    work->timer++;
    collision_register(work);
}

int sky_deck_shot_fire(NJS_POINT3 *from, NJS_POINT3 *to, Float scale)
{
    struct shot *shot;
    NJS_VECTOR velocity;
    Float time;
    Float reciprocal;
    if (!shots_task) {
        shots_task = task_create(TASK_WORK, 3, shots_init);
        if (!shots_task) {
            return 0;
        }
        shots_task->work->pos = *from;
    }
    shot = heap_alloc(sizeof(struct shot));
    if (shot) {
        time = njSqrt(-((to->y - from->y) * (2.0F / SHOT_GRAVITY)));
        velocity.x = to->x - from->x;
        velocity.y = 0.0F;
        velocity.z = to->z - from->z;
        reciprocal = 1.0F / time;
        velocity.x *= reciprocal;
        velocity.z *= reciprocal;
        shot->flight = time;
        shot->pos = *from;
        shot->velocity = velocity;
        shot->angle = (int)((Float)rand() * (FIRST_ANGLE_MAX * RAND_SCALE));
        if (!shots) {
            shots = shot;
            shot->prev = NULL;
            shot->next = NULL;
        } else {
            shot->prev = NULL;
            shot->next = shots;
            shots->prev = shot;
            shots = shot;
        }
        return 1;
    }
    return 0;
}
