/* a spray of rings that fall and bounce, and turn into gold coins as they
 * first bounce */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "draw/model.h"
#include "object/models.h"
#include "stg09/casino.h"
#include "stg09/coin_spray.h"
#include "stg09/gold_coin.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

struct coin_piece {
    NJS_POINT3 pos;
    NJS_VECTOR velocity;
    struct angles ang;
    struct angles spin; /* added to ang each step */
    Float ground; /* the height it bounces off */
    Sint16 life; /* the steps it has left, less 1 */
    Sint16 bounces; /* it draws as a coin from the first */
    Float scale;
    struct coin_piece *newer;
    struct coin_piece *older;
};

/* a new piece's life. it lasts one step more than this */
#define COIN_LIFE 180
/* a piece bounces as it crosses its ground at a vertical speed over
 * BOUNCE_SPEED a step. it keeps BOUNCE_KEEP of that speed, turned around, and
 * stands BOUNCE_LIFT times its new speed off the ground */
#define BOUNCE_SPEED 4.0F
#define BOUNCE_KEEP 0.8F
#define BOUNCE_LIFT 0.0001F
/* how much larger than a coin a ring draws, at the same scale */
#define RING_SCALE 1.8F
/* a new piece's angle about each axis is random over this many degrees,
 * centered on 0 */
#define START_SPAN 2160.0F

static Float coin_spray_drag = 0.98F; /* the velocity a piece keeps a step */
static Float coin_spray_gravity = -0.15F;
/* the most a piece's spin turns it each step, either way, in degrees */
static Float coin_spray_yaw_spread = 10.0F; /* about y */
static Float coin_spray_tilt_spread = 5.0F; /* about x and about z */
/* each piece draws at this share of its distance from the camera */
static Float coin_spray_depth = 0.7F;
static struct task *coin_spray_task = NULL;
/* the newest piece, from which the older links run to the oldest */
static struct coin_piece *coin_spray_head = NULL;

static void coin_spray_destroy(struct task *task);
static void coin_spray_draw(struct task *task);
static void coin_spray_run(struct task *task);

static void coin_spray_start(struct task *task)
{
    task->draw = coin_spray_draw;
    task->step = coin_spray_run;
    task->destructor = coin_spray_destroy;
}

static void coin_spray_remove(struct coin_piece *piece)
{
    if (!piece->newer) {
        coin_spray_head = piece->older;
    } else {
        piece->newer->older = piece->older;
    }
    if (piece->older) {
        piece->older->newer = piece->newer;
    }
    heap_free(piece);
}

static void coin_spray_destroy(struct task *task)
{
    struct coin_piece *piece = coin_spray_head;
    struct coin_piece *next;

    while (piece) {
        next = piece->older;
        coin_spray_remove(piece);
        piece = next;
    }
    if (coin_spray_task == task) {
        coin_spray_task = NULL;
    }
}

/* each piece draws as a ring until its first bounce, then as a gold coin */
static void coin_spray_draw(struct task *task)
{
    struct coin_piece *piece = coin_spray_head;
    NJS_POINT3 pos;
    Float scale;
    Angle angle;

    if (frame_step) {
        return;
    }
    draw_control_save();
    draw_attr_save();
    draw_control_set(NJD_CONTROL_3D_CONSTANT_ATTR);
    draw_attr_clear(0, NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_IGNORE_SPECULAR);
    while (piece) {
        scale = piece->scale;
        /* @hack spelling: pos formed through byte cursors, which SHC steps with
         * FMOV.S @R6+ and @R5+ (0x0c913b2a to 0x0c913b48). through the
         * members, 98 of 181 words differ */
        {
            char *to = (char *)&pos;
            char *from = (char *)&piece->pos;
            char *camera = (char *)&camera_work->pos;
            *(Float *)to = *(Float *)from - *(Float *)camera;
            *(Float *)(to += sizeof(Float)) =
                *(Float *)(from += sizeof(Float)) -
                *(Float *)(camera += sizeof(Float));
            to += sizeof(Float);
            from += sizeof(Float);
            camera += sizeof(Float);
            *(Float *)to = *(Float *)from - *(Float *)camera;
        }
        /* @hack spelling: pos scaled through a byte cursor, which ADD #4 steps,
         * with the depth read again for each member (0x0c913b4e to
         * 0x0c913b6a). through the members, 83 of 181 words differ */
        {
            char *to = (char *)&pos;
            *(Float *)to *= coin_spray_depth;
            to += sizeof(Float);
            *(Float *)to *= coin_spray_depth;
            to += sizeof(Float);
            *(Float *)to *= coin_spray_depth;
        }
        /* @hack spelling: the camera's position added through byte cursors,
         * which FMOV.S @R5+ and ADD #4 step (0x0c913b6c to 0x0c913b8c).
         * through the members, 76 of 181 words differ */
        {
            char *to = (char *)&pos;
            char *camera = (char *)&camera_work->pos;
            *(Float *)to += *(Float *)camera;
            to += sizeof(Float);
            camera += sizeof(Float);
            *(Float *)to += *(Float *)camera;
            to += sizeof(Float);
            camera += sizeof(Float);
            *(Float *)to += *(Float *)camera;
        }
        njPushMatrix(NULL);
        njTranslateV(NULL, &pos);
        if (scale != 1.0F) {
            njScale(NULL, scale, scale, scale);
        }
        TURN_VIA(njRotateY, piece->ang.y, angle);
        TURN_VIA(njRotateZ, piece->ang.z, angle);
        TURN_VIA(njRotateX, piece->ang.x, angle);
        if (piece->bounces) {
            casino_set_texture(CASINO_TEXTURES_ACTS_12);
            draw_model(gold_coin_object.model);
        } else {
            texture_set_regular();
            njScale(NULL, RING_SCALE, RING_SCALE, RING_SCALE);
            draw_model(ring_object.model);
        }
        njPopMatrix(1);
        piece = piece->older;
    }
    draw_attr_restore();
    draw_control_restore();
}

/* draw the pieces, then move each a step. a piece ends once its life runs
 * out. the task ends on the step after one that finds no piece */
static void coin_spray_run(struct task *task)
{
    int count = 0;
    struct coin_piece *piece = coin_spray_head;
    struct coin_piece *next;
    Float x;
    Float y;
    Float z;
    Float vx;
    Float vy;
    Float vz;
    Float ground;
    Float next_y;
    Float impulse;
    Float spread;

    coin_spray_draw(task);
    while (piece) {
        /* @hack spelling: pos read through a byte cursor, which FMOV.S @R4+
         * steps (0x0c913ce0 to 0x0c913cee). through the members, 170 of 195
         * words differ */
        {
            char *from = (char *)&piece->pos;
            x = *(Float *)from;
            from += sizeof(Float);
            y = *(Float *)from;
            from += sizeof(Float);
            z = *(Float *)from;
        }
        ground = piece->ground;
        next = piece->older;
        /* @hack spelling: velocity read through a byte cursor, which FMOV.S
         * @R4+ steps (0x0c913cf0 to 0x0c913d04). through the members, 171 of
         * 195 words differ */
        {
            char *from = (char *)&piece->velocity;
            vx = *(Float *)from * coin_spray_drag;
            from += sizeof(Float);
            vy = *(Float *)from * coin_spray_drag;
            from += sizeof(Float);
            vz = *(Float *)from * coin_spray_drag;
        }
        vy += coin_spray_gravity;
        next_y = y + vy;
        ++count;
        if (((next_y >= ground && y <= ground) ||
             (next_y <= ground && y >= ground)) &&
            _builtin_fabsf(vy) > BOUNCE_SPEED) {
            impulse = vy * (1.0F - BOUNCE_KEEP);
            vy = impulse - vy;
            /* @bug logic: the bounce moves the piece by impulse straight out
             * from the world's y axis, which has nothing to do with the spray.
             * it scales x and z by spread (FMACs at 0x0c913d72 and 0x0c913d7c),
             * found from njSqrt of x * x + z * z (0x0c913d58). impulse is the
             * part of vy the bounce takes away. it is negative as the piece
             * falls, so the move is toward the axis */
            if (x == 0.0F && z == 0.0F) {
                spread = 1.0F;
            } else {
                spread = (impulse / njSqrt((x * x) + (z * z))) + 1.0F;
            }
            x = (x * spread) + vx;
            y = ground + (BOUNCE_LIFT * vy);
            z = (z * spread) + vz;
            piece->bounces++;
        } else {
            x += vx;
            y += vy;
            z += vz;
        }
        /* @hack spelling: pos stored through a byte cursor, which ADD #4 steps
         * (0x0c913d98 to 0x0c913da6). through the members, 74 of 195 words
         * differ */
        {
            char *to = (char *)&piece->pos;
            *(Float *)to = x;
            to += sizeof(Float);
            *(Float *)to = y;
            to += sizeof(Float);
            *(Float *)to = z;
        }
        /* @hack spelling: velocity stored through a byte cursor, which ADD #4
         * steps (0x0c913daa to 0x0c913dba). through the members, 73 of 195
         * words differ */
        {
            char *to = (char *)&piece->velocity;
            *(Float *)to = vx;
            to += sizeof(Float);
            *(Float *)to = vy;
            to += sizeof(Float);
            *(Float *)to = vz;
        }
        /* @hack spelling: spin added to ang through byte cursors, which MOV.L
         * @R4+ and ADD #4 step (0x0c913dbc to 0x0c913dda). through the
         * members, 93 of 195 words differ */
        {
            char *to = (char *)&piece->ang;
            char *from = (char *)&piece->spin;

            /* @hack spelling: to stepped by 1 inside each of the first two
             * sums and by the rest of an Angle after it, which puts those
             * sums in R1 (0x0c913dc4, 0x0c913dce). stepped by sizeof(Angle)
             * after each, 8 of 195 words differ */
            *(Angle *)to++ += *(Angle *)from;
            to += sizeof(Angle) - 1;
            from = from + sizeof(Angle);
            *(Angle *)to++ += *(Angle *)from;
            to += sizeof(Angle) - 1;
            from = from + sizeof(Angle);

            /* @hack redundant: a switch on the last sum that does nothing,
             * which keeps the sum in R0 (0x0c913dd4 to 0x0c913dda). as a
             * statement of its own, 4 of 195 words differ */
            switch (*(Angle *)to += *(Angle *)from) {
            default:
                break;
            }
        }
        if (!piece->life--) {
            coin_spray_remove(piece);
        }
        piece = next;
    }
    if (!count) {
        task_kill(task);
    }
}

/* a random Float from -0.5 up to 0.5, never 0.5 itself */
#define JITTER() ((rand() * RAND_SCALE) - 0.5F)

Bool coin_spray_add(NJS_POINT3 *pos, NJS_VECTOR *velocity, Float spread,
                    Float ground, Float scale)
{
    struct coin_piece *piece;

    if (!coin_spray_task) {
        coin_spray_task = task_create(0, 3, coin_spray_start);
        if (!coin_spray_task) {
            return FALSE;
        }
    }
    piece = heap_alloc(sizeof(struct coin_piece));
    if (piece) {
        piece->scale = scale;
        piece->ground = ground;
        piece->life = COIN_LIFE;
        piece->bounces = 0;
        piece->velocity.x = velocity->x + (spread * JITTER() * 2.0F);
        piece->velocity.y = velocity->y + (spread * JITTER() * 2.0F);
        piece->velocity.z = velocity->z + (spread * JITTER() * 2.0F);
        /* @hack spelling: pos formed through byte cursors, which SHC steps with
         * FMOV.S @R5+ and @R6+ (0x0c913f54 to 0x0c913f6e). through the
         * members, 187 of 224 words differ */
        {
            char *to = (char *)&piece->pos;
            char *from = (char *)pos;
            char *step = (char *)&piece->velocity;
            *(Float *)to = *(Float *)from + *(Float *)step;
            *(Float *)(to += sizeof(Float)) =
                *(Float *)(from += sizeof(Float)) +
                *(Float *)(step += sizeof(Float));
            to += sizeof(Float);
            from += sizeof(Float);
            step += sizeof(Float);
            *(Float *)to = *(Float *)from + *(Float *)step;
        }
        piece->spin.x =
            JITTER() * coin_spray_tilt_spread * (2.0F * ANGLE_PER_DEGREE);
        piece->spin.y =
            JITTER() * coin_spray_yaw_spread * (2.0F * ANGLE_PER_DEGREE);
        piece->spin.z =
            JITTER() * coin_spray_tilt_spread * (2.0F * ANGLE_PER_DEGREE);
        piece->ang.x = JITTER() * (START_SPAN * ANGLE_PER_DEGREE);
        piece->ang.y = JITTER() * (START_SPAN * ANGLE_PER_DEGREE);
        piece->ang.z = JITTER() * (START_SPAN * ANGLE_PER_DEGREE);
        if (!coin_spray_head) {
            coin_spray_head = piece;
            piece->newer = NULL;
            piece->older = NULL;
        } else {
            piece->newer = NULL;
            piece->older = coin_spray_head;
            coin_spray_head->newer = piece;
            coin_spray_head = piece;
        }
        return TRUE;
    }
    return FALSE;
}
