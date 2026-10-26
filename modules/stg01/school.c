/* O SAKANA8K: a school of fish that swim about a tank and flee player 0 */
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
#include "stg01/school.h"
#include "system/cache.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the models of fish of kinds 1, 2 and 3 */
extern NJS_OBJECT beach_school_fish_1_object;
extern NJS_OBJECT beach_school_fish_2_object;
extern NJS_OBJECT beach_school_fish_3_object;

/* a fish's flags. FISH_KIND masks its kind, 1 to 3. run sets FISH_FAR when
 * a moving fish decides its course outside CAMERA_NEAR of the camera. run
 * clears it once the fish is inside */
#define FISH_KIND 3
#define FISH_FAR 4

/* a fish. pos is its offset from the tank's center. pos and speed are in
 * the tank's axes, which ang.y turns */
struct school_fish {
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    /* the yaw, in the tank's axes, that faces the fish toward the camera.
     * run eases it HEADING_SHARE of the way toward the yaw that
     * beach_school_heading returns */
    Angle heading;
    Sint16 yaw;
    Sint16 target_yaw;
    Sint16 pitch;
    Sint16 target_pitch;
    Sint16 flags;
    Sint16 timer; /* the steps it moves before it can decide its course */
};

struct school_vars {
    Uint16 count;
    Uint16 active; /* how many, from the first, move and draw */
    Float extent; /* the largest of scl.x, scl.y and scl.z */
    struct school_fish *fish;
};

#define VARS(work) ((struct school_vars *)&(work)->vars)

/* every fish is active while the camera is within extent + ALL_WITHIN of
 * the tank's center. farther off, the share active falls linearly, to none
 * at extent + NONE_FROM */
#define ALL_WITHIN 105.0F
#define NONE_FROM 400.0F

/* the task destroys itself once the camera and player 0 both lie beyond about
 * extent + 400. the range is squared, as placement_try_leave_by takes it */
#define LEAVE_RANGE_SQUARED(work) (SQUARE(VARS(work)->extent + 400.0F) + 10.0F)

/* squared distances. a fish flees player 0 within FLEE_RANGE of him, and is
 * far only outside CAMERA_NEAR of the camera */
#define FLEE_RANGE (30.0F * 30.0F)
#define CAMERA_NEAR (100.0F * 100.0F)

/* a fish is still at STILL_SPEED a step or slower */
#define STILL_SPEED 0.01F

/* the share of the way to its target pitch and yaw a fish turns in a step */
#define TURN_SHARE 0.12F
/* a fish's heading turns this share of the way to the yaw that
 * beach_school_heading returns. it turns when the fish's schedule is a
 * multiple of 4 */
#define HEADING_SHARE 0.8F

/* each step, a fish's speed keeps DRAG of itself. its y speed first gains
 * LIFT, and then keeps only DRAG_Y */
#define DRAG 0.98F
#define LIFT 0.00001F
#define DRAG_Y 0.85F

/* a fish's timer starts below DECIDE_WAIT. a fish that flees player 0 moves
 * off at FLEE_SPEED, its timer set below FLEE_WAIT. a still fish's speed gains
 * up to PUSH / 2 either way on each axis. its timer is then set below
 * DECIDE_WAIT */
#define DECIDE_WAIT 200.0F
#define FLEE_SPEED 0.7F
#define FLEE_WAIT 100.0F
#define PUSH 1.4F

static void beach_school_destroy(struct task *task);
static void beach_school_run(struct task *task);
static void beach_school_draw(struct task *task);

/* draw the tank for the set editor: the marker cube, tinted green and
 * scaled to the tank. half is the tank's half-size along each axis */
static void beach_school_draw_tank(NJS_POINT3 *half)
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

/* the yaw, in the tank's axes, that faces a fish at point toward the
 * camera. point is in the tank's axes too */
static int beach_school_heading(NJS_POINT3 *point, struct task_work *work)
{
    NJS_POINT3 world;

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njCalcPoint(NULL, point, &world);
    njPopMatrix(1);
    return (int)(atan2f(camera_work->pos.x - world.x,
                        camera_work->pos.z - world.z) *
                 ANGLE_PER_RADIAN) -
           work->ang.y;
}

/* place each fish at random in the tank, still and unturned. each takes a
 * random kind and timer */
static void beach_school_scatter(struct task *task)
{
    struct task_work *work = task->work;
    int count = VARS(work)->count;
    struct school_fish *fish = VARS(work)->fish;
    Float x = work->scl.x;
    Float y = work->scl.y;
    Float z = work->scl.z;
    int i;
    int kind;

    for (i = 0; i < count; i++, fish++) {
        /* @hack spelling: the position stored a Float at a time through a
         * cursor, as the image steps its stores (0x0c9077f2 to 0x0c907826);
         * through the members, 135 of 142 words differ */
        Float *at = VECTOR_FLOATS(&fish->pos);
        Float *speed;

        *at++ = x * ((rand() * RAND_SCALE) - 0.5F) * 2.0F;
        *at++ = y * ((rand() * RAND_SCALE) - 0.5F) * 2.0F;
        *at = z * ((rand() * RAND_SCALE) - 0.5F) * 2.0F;
        /* @hack spelling: the speed stored through a cursor too (0x0c907828 to
         * 0x0c907832); through the members, 70 of 142 words differ */
        /* @hack spelling: the speed's cursor set after the position's stores,
         * which builds it in R4 (0x0c907814, 0x0c907818); set at its
         * declaration, 124 of 142 words differ and the functions after it
         * move */
        speed = VECTOR_FLOATS(&fish->speed);
        *speed++ = 0.0F;
        *speed++ = 0.0F;
        *speed = 0.0F;
        /* a roll of kind 3 is rolled again. so a fish is kind 3 about one time
         * in nine. kinds 1 and 2 come about four times in nine each */
        kind = (rand() % 3) + 1;
        if (kind == 3) {
            kind = (rand() % 3) + 1;
        }
        fish->flags = kind;
        fish->timer = (int)(rand() * (DECIDE_WAIT * RAND_SCALE));
        fish->pitch = 0;
        fish->target_pitch = 0;
        fish->yaw = 0;
        fish->target_yaw = 0;
    }
}

void beach_school_step(struct task *task)
{
    struct task_work *work = task->work;
    int count = (Uint8)work->ang.z;
    Float *size;
    Float extent;

    if (placement_editing(task)) {
        count = 1;
    }
    /* @hack spelling: scl read through a cursor, for the image's post-increment
     * loads (0x0c907920, 0x0c907922); through the members, 50 of 74 words
     * differ and the functions after it move */
    size = VECTOR_FLOATS(&work->scl);
    extent = *size++;
    if (*size++ > extent) {
        extent = size[-1];
    }
    if (*size > extent) {
        extent = *size;
    }
    VARS(work)->extent = extent;
    if (placement_try_leave_by(task, LEAVE_RANGE_SQUARED(work))) {
        return;
    }
    task->draw = beach_school_draw;
    task->step = beach_school_run;
    task->destructor = beach_school_destroy;
    VARS(work)->count = count;
    VARS(work)->active = 0;
    VARS(work)->fish = heap_alloc(count * sizeof(struct school_fish));
    beach_school_scatter(task);
    TASK_TIMER(work) = 0;
}

static void beach_school_destroy(struct task *task)
{
    struct task_work *work = task->work;

    heap_free(VARS(work)->fish);
}

/* run moves the fish on every step the tank is in view, after it calls the
 * draw. TASK_TIMER(work) counts those steps. a fish's schedule is that count
 * plus the number of active fish after it. a fish whose schedule is a
 * multiple of 4 turns its heading. it also aims its target pitch and yaw
 * along its speed. one whose speed along z is 0 keeps its old targets. a fish
 * whose schedule is a multiple of 8 decides its course once its timer has
 * run out */
static void beach_school_run(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: 12 bytes of frame at sp+68 that no instruction reads or
     * writes, of a type unknown, declared as a Uint8[12]. they fill out the
     * frame's 128 bytes, with 48 of saved registers (0x0c9079a4 to 0x0c9079ba)
     * and 68 of the values the code keeps at sp+0 to sp+67; without them, 3 of
     * 664 words differ */
    Uint8 unused[12];
    Float distance;
    Float density;
    Float extent;
    int count;
    struct school_fish *fish;
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
    Sint16 heading_delta;
    struct school_vars *vars;
    int pitch_step;
    int yaw_step;
    Float origin_x;
    Float origin_y;

    if (placement_try_leave_by(task, LEAVE_RANGE_SQUARED(work))) {
        return;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_Z), "cnt:");
        njPrintD(NJM_LOCATION(24, EDITOR_ROW_ANG_Z), (Uint8)work->ang.z, 4);
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "Suisou Scale.x");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "Suisou Scale.y");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "Suisou Scale.z");
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        beach_school_draw_tank(&work->scl);
        njPopMatrix(1);
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
        vars = VARS(work);
        /* @hack redundant: a never-run test of vars, which gives
         * work->pos.x FR5 and pos.y FR4 (0x0c907b82, 0x0c907b90) and puts
         * camera_x at sp+24, camera_y at sp+20 and camera_z at sp+32
         * (0x0c907bae to 0x0c907bc8); without it, 17 of 664 words
         * differ */
        if (0) {
            if (vars) {
            }
        }
        /* @hack spelling: the product held in distance, which gives it
         * distance's FR4 and moves density to FR2 (0x0c907b48 to
         * 0x0c907b54); as (int)(vars->count * density), 250 of 664 words
         * differ */
        distance = vars->count * density;
        vars->active = (int)distance;
    } else {
        vars = VARS(work);
        vars->active = vars->count;
    }
    beach_school_draw(task);
    if (VARS(work)->active > 0) {
        count = VARS(work)->active;
        /* @bug logic: player 0's and the camera's offsets from the
         * tank's center are in the world's axes (0x0c907b88 to 0x0c907bc6).
         * the tests below compare them with a fish's pos, in the tank's axes,
         * unturned. the two agree only while ang.y is 0 */
        /* @hack spelling: pos.x and pos.y through locals, which gives pos.x,
         * pos.y and pos.z FR5, FR4 and FR6 (0x0c907b82, 0x0c907b90,
         * 0x0c907b9c); read in place, 9 of 664 words differ */
        origin_x = work->pos.x;
        player_x = player_work[0]->pos.x - origin_x;
        origin_y = work->pos.y;
        player_y = player_work[0]->pos.y - origin_y;
        player_z = player_work[0]->pos.z - work->pos.z;
        camera_x = camera_work->pos.x - origin_x;
        camera_y = camera_work->pos.y - origin_y;
        camera_z = camera_work->pos.z - work->pos.z;
        limit_x = work->scl.x;
        limit_y = work->scl.y;
        limit_z = work->scl.z;
        fish = VARS(work)->fish;
        while (count--) {
            Float *position;
            Float *velocity;

            schedule = count + TASK_TIMER(work);
            /* @hack spelling: the timer loaded, then decremented, which keeps
             * pos.x in FR5 and the position cursor in R4 (0x0c907b82,
             * 0x0c907c02); in one statement, 96 of 664 words differ */
            timer = fish->timer;
            timer = timer - 1;
            if (timer <= 0) {
                timer = 0;
            }
            /* @hack spelling: the position and the speed read and written
             * through cursors, here and below, as the image walks the floats
             * (0x0c907c0a to 0x0c907c6c, 0x0c907c7c, 0x0c907d3c,
             * 0x0c907f14); through the members, 47 to 151 of 664 words
             * differ at each */
            position = VECTOR_FLOATS(&fish->pos);
            velocity = VECTOR_FLOATS(&fish->speed);
            x = *position++;
            vx = *velocity++;
            x += vx;
            y = *position++;
            vy = *velocity++;
            y += vy;
            vz = *velocity;
            z = *position;
            z += vz;
            /* @hack redundant: never-run stores of 0.9F, a constant this
             * code never loads, which keep player_y at sp+48, limit_x at
             * sp+44 and limit_z at sp+40 (0x0c907b96, 0x0c907bd0,
             * 0x0c907be0); with 0.5F, 1.0F or 2.0F, 17 of 664 words
             * differ, and without them, 6 */
            if (0) {
                x = 0.9F;
                y = 0.9F;
            }
            /* at a wall of the tank, a fish steps back and reverses along
             * that axis */
            if (_builtin_fabsf(x) >= limit_x) {
                x -= vx;
                vx = -vx;
            }
            if (_builtin_fabsf(y) >= limit_y) {
                y -= vy;
                vy = -vy;
            }
            /* @hack spelling: z = z - vz beside x -= vx and y -= vy, which
             * keeps x, y and z in FR9, FR8 and FR7 (0x0c907c0e to
             * 0x0c907c18); as z -= vz, 20 of 664 words differ */
            if (_builtin_fabsf(z) >= limit_z) {
                z = z - vz;
                vz = -vz;
            }
            vy = (vy + LIFT) * DRAG_Y;
            vz *= DRAG;
            vx *= DRAG;
            *velocity = vz;
            *--velocity = vy;
            *--velocity = vx;
            *position = z;
            *--position = y;
            *--position = x;
            /* a far fish stops being far once it is inside CAMERA_NEAR of
             * the camera. until then, it skips the rest of its step when its
             * schedule is a multiple of 8. on its other steps, the tests
             * below read its schedule shifted right by 3 */
            if (fish->flags & FISH_FAR) {
                Float dx;
                Float dy;
                Float dz;

                position = VECTOR_FLOATS(&fish->pos);
                dx = camera_x - *position++;
                dy = camera_y - *position++;
                dz = camera_z - *position;
                if ((dx * dx) + (dy * dy) + (dz * dz) < CAMERA_NEAR) {
                    fish->flags &= ~FISH_FAR;
                } else {
                    if (!(schedule & 7)) {
                        fish->timer = timer;
                        fish++;
                        continue;
                    }
                    schedule >>= 3;
                }
            }
            if (!(schedule & 7)) {
                position = VECTOR_FLOATS(&fish->pos);
                x = *position++;
                y = *position++;
                z = *position;
                velocity = VECTOR_FLOATS(&fish->speed);
                vx = *velocity++;
                vy = *velocity++;
                vz = *velocity;
                if (!timer) {
                    delta_x = x - player_x;
                    delta_y = y - player_y;
                    delta_z = z - player_z;
                    squared = (delta_x * delta_x) + (delta_y * delta_y) +
                              (delta_z * delta_z);
                    if (squared < FLEE_RANGE) {
                        timer = (int)(rand() * (FLEE_WAIT * RAND_SCALE));
                        factor = FLEE_SPEED / njSqrt(squared);
                        delta_x *= factor;
                        delta_y *= factor;
                        delta_z *= factor;
                        vx = delta_x;
                        vy = delta_y;
                        vz = delta_z;
                    } else {
                        Float norm = (vx * vx) + (vy * vy) + (vz * vz);

                        /* a still fish pushes off in a random direction. a
                         * moving one keeps its course, and becomes far
                         * outside CAMERA_NEAR of the camera */
                        if (norm <= STILL_SPEED * STILL_SPEED) {
                            timer = (int)(rand() * (DECIDE_WAIT * RAND_SCALE));
                            vx += ((rand() * RAND_SCALE) - 0.5F) * PUSH;
                            vy += ((rand() * RAND_SCALE) - 0.5F) * PUSH;
                            vz += ((rand() * RAND_SCALE) - 0.5F) * PUSH;
                        } else {
                            /* @hack spelling: x = x - camera_x beside
                             * y -= camera_y, which keeps the position cursor
                             * in R4 and the speed cursor in R5 (0x0c907c02,
                             * 0x0c907c06); as x -= camera_x, 93 of 664 words
                             * differ */
                            x = x - camera_x;
                            y -= camera_y;
                            z -= camera_z;
                            norm = (x * x) + (y * y) + (z * z);
                            if (norm > CAMERA_NEAR) {
                                fish->flags |= FISH_FAR;
                            }
                            /* @hack redundant: a never-run test of vx,
                             * which keeps the reloaded vx in FR13 and vz
                             * in FR12 (0x0c907d4a, 0x0c907d52), and
                             * delta_x at sp+8 and delta_y at sp+4
                             * (0x0c907d64, 0x0c907d70); without it, 87 of
                             * 664 words differ */
                            if (0) {
                                if (vx) {
                                }
                            }
                        }
                    }
                    /* one decision in fifteen also nudges its speed */
                    if (rand() * RAND_SCALE < 1.0F / 15.0F) {
                        vx += ((rand() * RAND_SCALE) - 0.5F) * 0.04F;
                        vy += ((rand() * RAND_SCALE) - 0.5F) * 0.02F;
                        vz += ((rand() * RAND_SCALE) - 0.5F) * 0.04F;
                    }
                }
                /* @hack spelling: each store, then the cursor's step, which
                 * keeps x, y and z in FR9, FR8 and FR7 (0x0c907c0e to
                 * 0x0c907c18); as *velocity++ stores, 59 of 664 words
                 * differ */
                velocity = VECTOR_FLOATS(&fish->speed);
                *velocity = vx;
                velocity++;
                *velocity = vy;
                velocity++;
                *velocity = vz;
            }
            velocity = VECTOR_FLOATS(&fish->speed);
            vx = *velocity++;
            vy = *velocity++;
            vz = *velocity;
            abs_z = _builtin_fabsf(vz);
            if (abs_z > 0.0F && !(schedule & 3)) {
                fish->target_pitch =
                    -(int)(atan2f(vy, abs_z) * ANGLE_PER_RADIAN);
                fish->target_yaw = (int)(atan2f(vx, vz) * ANGLE_PER_RADIAN);
            }
            /* @hack spelling: each turn's step worked out in factor, a
             * statement at a time, both differences first (0x0c907f5c,
             * 0x0c907f64); as one expression each, 22 of 664 words differ, and
             * for the heading, 6 */
            pitch_delta = fish->target_pitch - fish->pitch;
            yaw_delta = fish->target_yaw - fish->yaw;
            factor = pitch_delta;
            factor *= TURN_SHARE;
            pitch_step = (int)factor;
            factor = yaw_delta;
            factor *= TURN_SHARE;
            yaw_step = (int)factor;
            fish->pitch += pitch_step;
            fish->yaw += yaw_step;
            if (!(schedule & 3)) {
                heading_delta =
                    beach_school_heading(&fish->pos, work) - fish->heading;
                factor = heading_delta;
                factor *= HEADING_SHARE;
                fish->heading += (int)factor;
            }
            fish->timer = timer;
            fish++;
        }
        /* @hack redundant: a never-run test of limit_y, which keeps it at
         * sp+36 and limit_z at sp+40 (0x0c907bd8, 0x0c907be0), and the
         * position cursor in R4 and the speed cursor in R5 (0x0c907c02,
         * 0x0c907c06); without it, 82 of 664 words differ */
        if (0) {
            if (limit_y) {
            }
        }
    }
    /* @hack redundant: a never-run read of extent, which keeps its store
     * at sp+60 (0x0c907b2c), a slot nothing loads; without it, 213 of 664
     * words differ */
    if (0) {
        njPrintD(0, (int)extent, 0);
    }
    TASK_TIMER(work)++;
}

/* draw the active fish on a frame's first step, while the tank is in view.
 * each fish turns from its yaw a fifth of the way toward showing the camera
 * its side. it then turns by its pitch */
static void beach_school_draw(struct task *task)
{
    struct task_work *work;
    int count;
    struct school_fish *fish;
    Angle turn;
    Sint16 delta;
    Sint16 *difference;
    Float blend;

    /* @hack spelling: work set inside the test with a comma, which keeps task
     * at sp+0 for the call (0x0c90803c, 0x0c908058); set before the if, 90
     * of 174 words differ */
    if (placement_try_leave_by(
            task, (work = task->work, LEAVE_RANGE_SQUARED(work)))) {
        return;
    }
    if (frame_step > 0) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                      njScalor(&work->scl) + 20.0F)) {
        return;
    }
    texture_set_stage();
    count = VARS(work)->active;
    njPushMatrix(NULL);
    /* @hack redundant: never-run calls, which keep njTranslateV's address in
     * R11, the kind mask in R10 and the turn in R9 (0x0c9080a8, 0x0c9080d2,
     * 0x0c908158); without them, 12 of 174 words differ */
    if (0) {
        njTranslateV(NULL, &work->pos);
        njTranslateV(NULL, &work->scl);
        njPopMatrix(count);
    }
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    fish = VARS(work)->fish;
    njPushMatrix(NULL);
    while (count--) {
        /* @bug uninitialized: a fish's first draw turns it by a heading the
         * heap left (MOV.L @(24,R14),R2 at 0x0c9080d8), and run eases it on
         * from there (0x0c907f9c). the scatter stores every field but heading
         * (0x0c9077f2 to 0x0c90787e) */
        /* @hack spelling: the difference reached through its address, which
         * stores it as a halfword at sp+4 and loads it back for each use
         * (0x0c9080de to 0x0c908146); as a plain Sint16, 90 of 174 words and
         * 4 pool words differ */
        *(difference = &delta) = fish->heading - fish->yaw;
        if (*difference > 0) {
            *difference -= NJM_DEG_ANG(90);
        } else {
            *difference += NJM_DEG_ANG(90);
        }
        blend = *difference;
        blend *= 0.2F;
        njTranslateV(NULL, &fish->pos);
        TURN_VIA(njRotateY, (int)blend + fish->yaw, turn);
        TURN_VIA(njRotateX, fish->pitch, turn);
        switch (fish->flags & FISH_KIND) {
        case 1:
            draw_model(beach_school_fish_1_object.model);
            break;
        case 2:
            draw_model(beach_school_fish_2_object.model);
            break;
        case 3:
            draw_model(beach_school_fish_3_object.model);
            break;
        /* @hack redundant: a default that only breaks, which puts
         * matrix_load_parent's address in R3 (0x0c9081a2); without it, 2 of
         * 174 words differ */
        default:
            break;
        }
        matrix_load_parent();
        fish++;
    }
    /* @hack redundant: never-run prints, which keep the count in R8 and the
     * kind mask in R10 (0x0c9080a0, 0x0c9080d2); without them, 11 of 174
     * words differ */
    if (0) {
        njPrintD(0, fish->flags & FISH_KIND, 0);
        njPrintD(0, turn, 0);
        njPrintD(1, fish->flags & FISH_KIND, 0);
    }
    /* @hack redundant: a jump to the next line, which puts njPopMatrix's
     * address in R2 (0x0c9081b0); without it, 2 of 174 words differ */
    goto pop;
pop:
    njPopMatrix(2);
}
