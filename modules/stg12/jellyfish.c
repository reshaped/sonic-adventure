/* O KURAGE: jellyfish that drift about a tank and flee player 0 */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg12/jellyfish.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 4
#define TEXT_2_LENGTH 14
#define TEXT_3_LENGTH 14
#define TEXT_4_LENGTH 14
#define TEXT_5_LENGTH 10
#include "stg12/jellyfish/text.inc"

#define JELLYFISH_FRAMES 5

/* a jellyfish's flags. JELLYFISH_FRAME masks the frame its sprite shows.
 * run sets JELLYFISH_FAR when a moving jellyfish decides its course outside
 * CAMERA_NEAR of the camera. run clears it once the jellyfish is inside */
#define JELLYFISH_FRAME 15
#define JELLYFISH_FAR 16

/* a jellyfish. pos is its offset from the tank's center. pos and speed are
 * in the tank's axes, which ang.y turns */
struct jellyfish {
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    Sint16 yaw;
    Sint16 target_yaw;
    Sint16 pitch;
    Sint16 target_pitch;
    Sint16 flags;
    Sint16 timer; /* the steps it moves before it can decide its course */
};

struct jellyfish_vars {
    Uint16 count;
    Uint16 active; /* how many, from the first, move and draw */
    Float extent; /* the largest of scl.x, scl.y and scl.z */
    struct jellyfish *jellyfish;
};

#define VARS(work) ((struct jellyfish_vars *)&(work)->vars)

/* every jellyfish is active while the camera is within extent + ALL_WITHIN
 * of the tank's center. farther off, the share active falls linearly, to
 * none at extent + NONE_FROM */
#define ALL_WITHIN 85.0F
#define NONE_FROM 300.0F

/* destroyed beyond about extent + 300 units of the camera and player 0. many
 * of the game's squared ranges add 10, which widens this one by less than
 * 0.02 */
#define LEAVE_RANGE(work) (SQUARE(VARS(work)->extent + 300.0F) + 10.0F)

/* squared distances. a jellyfish flees player 0 within FLEE_RANGE of it,
 * and is far only outside CAMERA_NEAR of the camera */
#define FLEE_RANGE (30.0F * 30.0F)
#define CAMERA_NEAR (80.0F * 80.0F)

/* a jellyfish is still at STILL_SPEED a step or slower */
#define STILL_SPEED 0.005F

/* the share of the way to its target pitch and yaw a jellyfish turns in a
 * step */
#define TURN_SHARE 0.05F

static NJS_TEXNAME shelter_jellyfish_textures[1] = {{TEXT_5, 0, 0}};
NJS_TEXLIST shelter_jellyfish_texlist = {
#include "stg12/jellyfish/shelter_jellyfish_texlist.inc"
};
static NJS_TEXANIM shelter_jellyfish_frames[JELLYFISH_FRAMES] = {
#include "stg12/jellyfish/shelter_jellyfish_frames.inc"
};

static void shelter_jellyfish_destroy(struct task *task);
static void shelter_jellyfish_run(struct task *task);
static void shelter_jellyfish_draw(struct task *task);

/* draw the tank for the set editor: the marker cube, tinted green and
 * scaled to the tank. half is the tank's half-size along each axis */
static void shelter_jellyfish_draw_tank(NJS_POINT3 *half)
{
    Float x = half->x * 0.1F;
    Float y = half->y * 0.1F;
    Float z = half->z * 0.1F;

    njPushMatrix(NULL);
    njScale(NULL, x * 2.0F, y * 2.0F, z * 2.0F);
    draw_set_material(0.03F, 0.0F, 1.0F, 0.0F);
    draw_tree(marker_cube_object);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* place each jellyfish at random in the tank, still and unturned. each takes
 * a random frame and a timer below 200 */
static void shelter_jellyfish_scatter(struct task *task)
{
    struct task_work *work = task->work;
    int count = VARS(work)->count;
    struct jellyfish *jellyfish = VARS(work)->jellyfish;
    Float x = work->scl.x;
    Float y = work->scl.y;
    Float z = work->scl.z;
    int i;

    for (i = 0; i < count; i++, jellyfish++) {
        /* @hack spelling: the position stored a Float at a time through a
         * cursor, as the image steps its stores (0x0c9099a2 to 0x0c9099d6);
         * through the members, 120 of 128 words differ and
         * shelter_jellyfish_scatter is 12 bytes shorter */
        Float *at = VECTOR_FLOATS(&jellyfish->pos);
        Float *speed;

        *at++ = x * ((rand() * RAND_SCALE) - 0.5F) * 2.0F;
        *at++ = y * ((rand() * RAND_SCALE) - 0.5F) * 2.0F;
        *at = z * ((rand() * RAND_SCALE) - 0.5F) * 2.0F;
        /* @hack spelling: the speed stored through a cursor too (0x0c9099d8 to
         * 0x0c9099e2); through the members, 56 of 128 words differ and
         * shelter_jellyfish_scatter is 2 bytes shorter */
        /* @hack spelling: the speed's cursor set after the position's stores,
         * which builds it in R4 (0x0c9099c4, 0x0c9099c8); set at its
         * declaration, 109 of 128 words differ and shelter_jellyfish_scatter
         * is 2 bytes shorter */
        speed = VECTOR_FLOATS(&jellyfish->speed);
        *speed++ = 0.0F;
        *speed++ = 0.0F;
        *speed = 0.0F;
        jellyfish->flags = rand() % JELLYFISH_FRAMES;
        jellyfish->timer = (int)(rand() * (200.0F * RAND_SCALE));
        jellyfish->pitch = 0;
        jellyfish->target_pitch = 0;
        jellyfish->yaw = 0;
        jellyfish->target_yaw = 0;
    }
}

void shelter_jellyfish_step(struct task *task)
{
    struct task_work *work = task->work;
    int count = (Uint8)work->ang.z;
    Float extent;
    Float *size;

    if (placement_editing(task)) {
        count = 1;
    }
    /* @hack spelling: scl read through a cursor, for the image's post-increment
     * loads (0x0c909a54, 0x0c909a56); through the members, 50 of 74 words
     * differ and shelter_jellyfish_step is 2 bytes shorter */
    size = VECTOR_FLOATS(&work->scl);
    extent = *size++;
    if (*size++ > extent) {
        extent = size[-1];
    }
    if (*size > extent) {
        extent = *size;
    }
    VARS(work)->extent = extent;
    if (placement_try_leave_by(task, LEAVE_RANGE(work))) {
        return;
    }
    task->draw = shelter_jellyfish_draw;
    task->step = shelter_jellyfish_run;
    task->destructor = shelter_jellyfish_destroy;
    VARS(work)->count = count;
    VARS(work)->active = 0;
    VARS(work)->jellyfish = heap_alloc(count * sizeof(struct jellyfish));
    shelter_jellyfish_scatter(task);
    TASK_TIMER(work) = 0;
}

static void shelter_jellyfish_destroy(struct task *task)
{
    struct task_work *work = task->work;

    heap_free(VARS(work)->jellyfish);
}

/* the jellyfish move only on steps when frame_step is 0 and the tank is in
 * view. TASK_TIMER(work) counts those steps. a jellyfish's schedule is that
 * count plus the number of active jellyfish after it. a jellyfish whose
 * schedule is a multiple of 8 aims its target pitch and yaw along its
 * speed. one whose speed along z is 0 keeps its old targets. a jellyfish
 * whose schedule is a multiple of 16 decides its course once its timer has
 * run out */
static void shelter_jellyfish_run(struct task *task)
{
    struct task_work *work = task->work;
    Float distance;
    Float density;
    Float extent;
    /* @hack redundant: 12 bytes of frame at sp+68 that no instruction reads or
     * writes, of a type unknown, declared as a Uint8[12]. they fill out the
     * frame's 128 bytes, with 48 of saved registers (0x0c909b40 to 0x0c909b56)
     * and 68 of the values the code keeps at sp+0 to sp+67; without them, 3 of
     * 644 words differ */
    Uint8 unused[12];
    int count;
    struct jellyfish *jellyfish;
    Float player_x;
    Float player_y;
    Float player_z;
    Float camera_x;
    Float camera_y;
    Float camera_z;
    Float limit_x;
    Float limit_y;
    Float limit_z;
    Float x;
    Float y;
    Float z;
    Float vx;
    Float vy;
    Float vz;
    Float squared;
    Float factor;
    Float abs_z;
    Float delta_x;
    Float delta_y;
    Float delta_z;
    int schedule;
    Sint16 timer;
    Sint16 pitch_delta;
    Sint16 yaw_delta;
    int pitch_step;
    int yaw_step;
    struct jellyfish_vars *vars;

    if (placement_try_leave_by(task, LEAVE_RANGE(work))) {
        return;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_Z), TEXT_1);
        njPrintD(NJM_LOCATION(24, EDITOR_ROW_ANG_Z), (Uint8)work->ang.z, 4);
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), TEXT_2);
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), TEXT_3);
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), TEXT_4);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        shelter_jellyfish_draw_tank(&work->scl);
        njPopMatrix(1);
        return;
    }
    if (frame_step) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                      njScalor(&work->scl) + 16.0F)) {
        return;
    }
    distance = njDistanceP2P(&camera_work->pos, &work->pos);
    if (distance > VARS(work)->extent + ALL_WITHIN) {
        extent = VARS(work)->extent;
        density =
            (extent - distance + NONE_FROM) * (1.0F / (NONE_FROM - ALL_WITHIN));
        if (density < 0.0F) {
            density = 0.0F;
        }
        /* @hack spelling: vars set here and in the else, which puts work + 8 in
         * R4 for the count and active (0x0c909c70 to 0x0c909c90). through
         * VARS(work), 280 of 644 words differ. set once before the if, 327 to
         * 349 differ, and set at its declaration, 440.
         * shelter_jellyfish_run is 6 bytes shorter in each */
        vars = VARS(work);
        /* @hack spelling: the product held in distance, which gives it
         * distance's FR4 and moves density to FR2 (0x0c909c76 to
         * 0x0c909c84); as (int)(vars->count * density), 340 of 644 words
         * differ and shelter_jellyfish_run is 2 bytes shorter */
        distance = vars->count * density;
        vars->active = (int)distance;
    } else {
        vars = VARS(work);
        vars->active = vars->count;
    }
    shelter_jellyfish_draw(task);
    if (VARS(work)->active > 0) {
        count = VARS(work)->active;
        /* @bug logic: player 0's and the camera's offsets from the tank's
         * center are in the world's axes. the tests below compare these with
         * a jellyfish's pos, in the tank's axes, unturned (the FSUBs at
         * 0x0c909cb4 to 0x0c909cf0). the two sets of axes agree only while
         * ang.y is 0 */
        player_x = player_work[0]->pos.x - work->pos.x;
        player_y = player_work[0]->pos.y - work->pos.y;
        player_z = player_work[0]->pos.z - work->pos.z;
        camera_x = camera_work->pos.x - work->pos.x;
        camera_y = camera_work->pos.y - work->pos.y;
        camera_z = camera_work->pos.z - work->pos.z;
        limit_x = work->scl.x;
        limit_y = work->scl.y;
        limit_z = work->scl.z;
        /* @hack redundant: a never-run test of atan2f's address, which gives
         * it R9 and the count R8 (0x0c909cee, 0x0c909ca8), and camera_x
         * sp+20 and camera_y sp+16 (0x0c909cda, 0x0c909ce6); without it, 28
         * of 644 words differ */
        if (0) {
            if (&atan2f) {
            }
        }
        /* @hack redundant: a never-run store to extent, which keeps extent at
         * sp+56, player_x at sp+48 and player_z at sp+52 (0x0c909c5a,
         * 0x0c909cb6, 0x0c909cce); without it, 212 of 644 words differ and
         * shelter_jellyfish_run is 2 bytes shorter */
        if (0) {
            extent = 1.0F;
        }
        /* @hack redundant: two never-run stores to player_z, each in a block
         * of its own, which keep player_x to player_z at sp+48, sp+44 and
         * sp+52 and limit_x to limit_z at sp+36, sp+32 and sp+40 (0x0c909cb6
         * to 0x0c909d0a); with one, or both in one block, 52 of 644 words
         * differ, and with neither, 224, with shelter_jellyfish_run 6 bytes
         * shorter */
        if (0) {
            player_z = 1.0F;
        }
        /* @hack redundant: the same store again, in a block of its own, for the
         * frame slots above; without it, 52 of 644 words differ */
        if (0) {
            player_z = 1.0F;
        }
        jellyfish = VARS(work)->jellyfish;
        for (; count--; jellyfish++) {
            Float *position;
            Float *velocity;

            schedule = TASK_TIMER(work) + count;
            /* @hack spelling: the timer loaded, then decremented, which keeps
             * extent at sp+56 and player_z at sp+52 (0x0c909c5a, 0x0c909cce)
             * and y, vy and z in FR8, FR6 and FR7 (0x0c909dc4 to
             * 0x0c909dd6); in one statement, 196 of 644 words differ and
             * shelter_jellyfish_run is 2 bytes shorter */
            timer = jellyfish->timer;
            /* @hack spelling: the decrement spelled timer = timer - 1; as
             * timer -= 1, 26 of 644 words differ */
            timer = timer - 1;
            if (timer <= 0) {
                timer = 0;
            }
            /* @hack spelling: the position and the speed read and written
             * through cursors, here and below, as the image walks the floats
             * (0x0c909db4 to 0x0c909dcc, 0x0c909e12 to 0x0c909e1c,
             * 0x0c909e2c to 0x0c909e3c, 0x0c909e78 to 0x0c909e8c, 0x0c90a066
             * to 0x0c90a080); through the members, 167 to 453 of 644 words
             * differ at each */
            position = VECTOR_FLOATS(&jellyfish->pos);
            velocity = VECTOR_FLOATS(&jellyfish->speed);
            x = *position++;
            y = *position++;
            z = *position;
            vx = *velocity++;
            vy = *velocity++;
            vz = *velocity;
            /* @hack spelling: y, then x, then z, which keeps the speed cursor
             * in R5 and the position cursor in R4 (0x0c909db4 to 0x0c909dbc);
             * in x, y, z order, 21 of 644 words differ */
            y += vy;
            x += vx;
            z += vz;
            /* @hack redundant: a never-run test of vy, which keeps y, vy and
             * z in FR8, FR6 and FR7 (0x0c909dc4 to 0x0c909dd6), and delta_y
             * at sp+4 and delta_z at sp+8 (0x0c909eae, 0x0c909eb8); without
             * it, 22 of 644 words differ */
            if (0) {
                if (vy) {
                }
            }
            /* at a wall of the tank, a jellyfish steps back and reverses
             * along that axis */
            /* @hack spelling: y = y - vy and z = z - vz beside x -= vx, which
             * keep the speed cursor in R5 and x, y and z in FR9, FR8 and FR7
             * (0x0c909db4 to 0x0c909dd6); as y -= vy, 124 of 644 words and a
             * pool word differ, as z -= vz, 120, and as x = x - vx, 128 */
            if (_builtin_fabsf(x) >= limit_x) {
                x -= vx;
                vx = -vx;
            }
            if (_builtin_fabsf(y) >= limit_y) {
                y = y - vy;
                vy = -vy;
            }
            if (_builtin_fabsf(z) >= limit_z) {
                z = z - vz;
                vz = -vz;
            }
            vy = (vy + 0.00001F) * 0.91F;
            vz *= 0.985F;
            vx *= 0.985F;
            *velocity = vz;
            *--velocity = vy;
            *--velocity = vx;
            *position = z;
            *--position = y;
            *--position = x;
            /* a far jellyfish stops being far once it is inside CAMERA_NEAR of
             * the camera. until then, it skips the rest of its step when its
             * schedule is a multiple of 8. on its other steps, the tests below
             * read its schedule shifted right by 3 */
            if (jellyfish->flags & JELLYFISH_FAR) {
                Float dx;
                Float dy;
                Float dz;

                position = VECTOR_FLOATS(&jellyfish->pos);
                dx = camera_x - *position++;
                /* @hack spelling: dy set, then reduced, which gives dy FR4 and
                 * dz FR6 (0x0c909e2e, 0x0c909e36); in one statement, the two
                 * trade registers and 8 of 644 words differ */
                dy = camera_y;
                dy -= *position++;
                dz = camera_z - *position;
                if ((dx * dx) + (dy * dy) + (dz * dz) < CAMERA_NEAR) {
                    jellyfish->flags &= ~JELLYFISH_FAR;
                } else {
                    if (!(schedule & 7)) {
                        jellyfish->timer = timer;
                        continue;
                    }
                    schedule >>= 3;
                }
            }
            if (!(schedule & 15)) {
                position = VECTOR_FLOATS(&jellyfish->pos);
                x = *position++;
                y = *position++;
                z = *position;
                velocity = VECTOR_FLOATS(&jellyfish->speed);
                vx = *velocity++;
                vy = *velocity++;
                vz = *velocity;
                if (!timer) {
                    delta_x = x - player_x;
                    delta_y = y - player_y;
                    delta_z = z - player_z;
                    squared = (delta_x * delta_x) + (delta_y * delta_y) +
                              (delta_z * delta_z);
                    /* @hack redundant: a never-run store to delta_x, which
                     * keeps extent at sp+56 and player_z at sp+52
                     * (0x0c909c5a, 0x0c909cce), and camera_x at sp+20 and
                     * camera_y at sp+16 (0x0c909cda, 0x0c909ce6); without
                     * it, 202 of 644 words and a pool word differ, and
                     * shelter_jellyfish_run is 2 bytes shorter */
                    if (0) {
                        delta_x = 1.0F;
                    }
                    if (squared < FLEE_RANGE) {
                        timer = (int)(rand() * (100.0F * RAND_SCALE));
                        factor = 0.3F / njSqrt(squared);
                        delta_x *= factor;
                        delta_y *= factor;
                        delta_z *= factor;
                        vy = delta_y;
                        vx = delta_x;
                        vz = delta_z;
                    } else {
                        Float norm = (vx * vx) + (vy * vy) + (vz * vz);

                        /* a still jellyfish pushes off in a random
                         * direction. a moving one keeps its course, and
                         * becomes far outside CAMERA_NEAR of the camera */
                        if (norm <= STILL_SPEED * STILL_SPEED) {
                            timer = (int)(rand() * (200.0F * RAND_SCALE));
                            vx += ((rand() * RAND_SCALE) - 0.5F) * 0.6F;
                            vy += ((rand() * RAND_SCALE) - 0.5F) * 0.6F;
                            vz += ((rand() * RAND_SCALE) - 0.5F) * 0.6F;
                        } else {
                            Float dx;
                            Float dy;
                            Float dz;

                            dx = x - camera_x;
                            dy = y - camera_y;
                            dz = z - camera_z;
                            norm = (dx * dx) + (dy * dy) + (dz * dz);
                            if (norm > CAMERA_NEAR) {
                                jellyfish->flags |= JELLYFISH_FAR;
                            }
                        }
                    }
                    /* one decision in twelve also nudges its speed */
                    if (rand() * RAND_SCALE < 1.0F / 12.0F) {
                        vx += ((rand() * RAND_SCALE) - 0.5F) * 0.04F;
                        vy += ((rand() * RAND_SCALE) - 0.5F) * 0.02F;
                        vz += ((rand() * RAND_SCALE) - 0.5F) * 0.04F;
                    }
                }
                velocity = VECTOR_FLOATS(&jellyfish->speed);
                *velocity++ = vx;
                *velocity++ = vy;
                *velocity = vz;
            }
            velocity = VECTOR_FLOATS(&jellyfish->speed);
            vx = *velocity++;
            vy = *velocity++;
            vz = *velocity;
            abs_z = _builtin_fabsf(vz);
            /* @hack redundant: a never-run test of vz, which keeps vx in FR12
             * and vz in FR13 (0x0c90a07a, 0x0c90a080) and the far
             * jellyfish's dx and dz in FR5 and FR6 (0x0c909e28, 0x0c909e36);
             * without it, 13 of 644 words differ */
            if (0) {
                if (vz) {
                }
            }
            /* @hack redundant: a never-run test of vx, which keeps the
             * decision's dx and dy in FR7 and FR4 (0x0c909fe2 to
             * 0x0c909ff0); without it, 168 of 644 words differ and
             * shelter_jellyfish_run is 2 bytes shorter. without both tests,
             * 11 differ: the far jellyfish's
             * dx and dz trade FR5 and FR6, and extent and player_z sp+56 and
             * sp+52 (0x0c909c5a, 0x0c909cce) */
            if (0) {
                if (vx) {
                }
            }
            if (abs_z > 0.0F && !(schedule & 7)) {
                jellyfish->target_pitch =
                    -(int)(atan2f(vy, abs_z) * ANGLE_PER_RADIAN);
                jellyfish->target_yaw =
                    (int)(atan2f(vx, vz) * ANGLE_PER_RADIAN);
            }
            /* @hack spelling: each turn's step worked out in factor, a
             * statement at a time. the pitch's keeps the decision's dx and dy
             * in FR7 and FR4 (0x0c909fe2 to 0x0c909ff0), and the yaw's keeps
             * camera_x at sp+20 and camera_y at sp+16 (0x0c909cda,
             * 0x0c909ce6). with factor = delta * TURN_SHARE, 123 of 644 words
             * differ for the pitch. for the yaw, 54 differ and
             * shelter_jellyfish_run is 2 bytes longer */
            pitch_delta = jellyfish->target_pitch - jellyfish->pitch;
            yaw_delta = jellyfish->target_yaw - jellyfish->yaw;
            factor = pitch_delta;
            factor *= TURN_SHARE;
            pitch_step = (int)factor;
            jellyfish->pitch += pitch_step;
            factor = yaw_delta;
            factor *= TURN_SHARE;
            yaw_step = (int)factor;
            /* @hack redundant: a never-run test of factor, which keeps
             * camera_x at sp+20 and camera_y at sp+16 (0x0c909cda,
             * 0x0c909ce6) and the far jellyfish's dx and dz in FR5 and FR6
             * (0x0c909e28, 0x0c909e36); without it, 14 of 644 words differ */
            if (0) {
                if (factor) {
                }
            }
            /* @hack redundant: a never-run test of pitch_step, which keeps it
             * in R4 and yaw_step in R5 (0x0c90a0de, 0x0c90a0ee), and the
             * decision's dx and dy in FR7 and FR4 (0x0c909fe2 to
             * 0x0c909ff0); without it, 168 of 644 words differ and
             * shelter_jellyfish_run is 2 bytes shorter */
            if (0) {
                if (pitch_step) {
                }
            }
            jellyfish->yaw += yaw_step;
            jellyfish->timer = timer;
        }
    }
    /* @hack redundant: a never-run read of extent, which keeps its store at
     * sp+56 (0x0c909c5a), a slot nothing loads; without it, the frame is 4
     * bytes smaller, 342 of 644 words differ, and shelter_jellyfish_run is 2
     * bytes shorter */
    if (0) {
        njPrintD(0, (int)extent, 0);
    }
    TASK_TIMER(work)++;
}

/* draw the active jellyfish, each a sprite of its frame turned by its pitch,
 * blended additively */
static void shelter_jellyfish_draw(struct task *task)
{
    struct task_work *work;
    NJS_SPRITE sprite;
    int count;
    struct jellyfish *jellyfish;

    /* @hack spelling: work set inside the test with a comma, which keeps task
     * at sp+0 for the call (0x0c90a138, 0x0c90a154); set before the if, 114
     * of 122 words differ */
    if (placement_try_leave_by(task, (work = task->work, LEAVE_RANGE(work)))) {
        return;
    }
    if (frame_step > 0) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                      njScalor(&work->scl) + 10.0F)) {
        return;
    }
    sprite.tlist = &shelter_jellyfish_texlist;
    sprite.tanim = shelter_jellyfish_frames;
    sprite.sx = 1.0F;
    sprite.sy = 1.0F;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetTexture(&shelter_jellyfish_texlist);
    count = VARS(work)->active;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    jellyfish = VARS(work)->jellyfish;
    while (count--) {
        sprite.ang = jellyfish->pitch;
        sprite.p = jellyfish->pos;
        njDrawSprite3D(&sprite, jellyfish->flags & JELLYFISH_FRAME,
                       NJD_SPRITE_ANGLE | NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
        jellyfish++;
    }
    njPopMatrix(1);
}
