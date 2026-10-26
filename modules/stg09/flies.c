/* O HAE: flies that wander about a box and flee player 0 and the camera */
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
#include "stg09/casino.h"
#include "stg09/flies.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 4
#define TEXT_2_LENGTH 14
#define TEXT_3_LENGTH 14
#define TEXT_4_LENGTH 14
#include "stg09/flies/text.inc"

/* the task destroys itself beyond about 250 of the camera and player 0.
 * placement_try_leave_by takes the range squared */
#define RANGE ((250.0F * 250.0F) + 10.0F)

#define FLY_FRAMES 2

/* in a fly's flags. a fly that decides to keep its course beyond 85 of the
 * camera becomes far. it stays far until it comes within 85 */
#define FLY_FAR 4

/* squared distances. a deciding fly flees player 0 within FLEE_PLAYER, or
 * else the camera within FLEE_CAMERA. a fly within CAMERA_NEAR of the camera
 * isn't far */
#define FLEE_PLAYER (20.0F * 20.0F)
#define FLEE_CAMERA (40.0F * 40.0F)
#define CAMERA_NEAR (85.0F * 85.0F)

/* every fly is active while the camera is within extent + ALL_ACTIVE of the
 * box's center. none is from extent + NONE_ACTIVE. the active share falls
 * evenly between the two */
#define ALL_ACTIVE 100.0F
#define NONE_ACTIVE (400.0F * 400.0F)

/* each move, a fly keeps FLY_KEEP of its speed along x and z. along y it
 * gains FLY_LIFT, then keeps FLY_KEEP_Y */
#define FLY_KEEP 0.98F
#define FLY_KEEP_Y 0.94F
#define FLY_LIFT 0.00001F

/* a deciding fly no faster than this gains speed at random */
#define WANDER_SPEED 0.2F

/* a fly. pos is its offset from the box's center, in the box's turned
 * axes */
struct fly {
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    /* the sprite's angle. each turn takes it half of the way to
     * target_heading */
    Sint16 heading;
    Sint16 target_heading;
    Sint16 flags;
    Sint16 timer; /* the steps it moves before it can decide again */
};

struct flies_vars {
    Uint16 count;
    Uint16 active; /* the count of flies, from the first, that move and draw */
    Float extent; /* the largest of scl's three */
    struct fly *flies;
};

#define VARS(work) ((struct flies_vars *)&(work)->vars)

/* the scale of a fly's sprite across and up */
static Float flies_width = 0.2F;
static Float flies_height = 0.2F;
static NJS_TEXANIM flies_frames[FLY_FRAMES] = {
#include "stg09/flies/flies_frames.inc"
};

static void flies_destroy(struct task *task);
static void flies_run(struct task *task);
static void flies_draw(struct task *task);

/* draw the box for the set editor, in faint green. half is half its size on
 * each axis */
static void flies_draw_box(NJS_POINT3 *half)
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

/* put each fly at random in the box, still, with a timer below 40 */
static void flies_scatter(struct task *task)
{
    struct task_work *work = task->work;
    int count = VARS(work)->count;
    struct fly *fly = VARS(work)->flies;
    Float x = work->scl.x;
    Float y = work->scl.y;
    Float z = work->scl.z;
    int i;

    for (i = 0; i < count; ++i, ++fly) {
        /* @hack spelling: the position stored through a cursor, which SHC steps
         * along it in R8 and R11 (0x0c91e97e to 0x0c91e9b2). through the
         * fields, 110 of 116 words differ */
        Float *at = VECTOR_FLOATS(&fly->pos);
        Float *speed;

        *at++ = x * ((rand() * RAND_SCALE) - 0.5F) * 2.0F;
        *at++ = y * ((rand() * RAND_SCALE) - 0.5F) * 2.0F;
        *at = z * ((rand() * RAND_SCALE) - 0.5F) * 2.0F;
        /* @hack spelling: the speed's cursor set after the position's stores,
         * where SHC builds it in R4 (0x0c91e99e, 0x0c91e9a2). set at its
         * declaration, 79 of 116 words differ */
        /* @hack spelling: the speed cleared through a cursor, which SHC steps
         * along it in R4 (0x0c91e9b4 to 0x0c91e9bc). through the fields, 47
         * of 116 words differ */
        speed = VECTOR_FLOATS(&fly->speed);
        *speed++ = 0.0F;
        *speed++ = 0.0F;
        *speed = 0.0F;
        fly->flags = 0;
        fly->timer = (Sint16)(rand() * (40.0F * RAND_SCALE));
        fly->heading = 0;
        fly->target_heading = 0;
    }
}

void flies_step(struct task *task)
{
    struct task_work *work = task->work;
    int count = (Uint8)work->ang.z;
    Float extent;
    Float *size;

    if (placement_editing(task)) {
        count = 1;
    }
    /* @hack spelling: scl read through a cursor, for the image's post-increment
     * loads at 0x0c91ea1a and 0x0c91ea1c. through the fields, 44 of 61 words
     * differ */
    size = VECTOR_FLOATS(&work->scl);
    extent = *size++;
    if (*size++ > extent) {
        extent = size[-1];
    }
    if (*size > extent) {
        extent = *size;
    }
    VARS(work)->extent = extent;
    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    task->draw = flies_draw;
    task->step = flies_run;
    task->destructor = flies_destroy;
    VARS(work)->count = count;
    VARS(work)->active = 0;
    VARS(work)->flies = heap_alloc(count * sizeof(struct fly));
    flies_scatter(task);
    TASK_TIMER(work) = 0;
}

static void flies_destroy(struct task *task)
{
    struct task_work *work = task->work;

    heap_free(VARS(work)->flies);
}

/* the flies move only on a frame's first step while the box is in view.
 * TASK_TIMER counts those steps. a fly's schedule is that count plus the
 * number of active flies after it. a fly whose timer is out decides on its
 * speed when its schedule is a multiple of 8. a moving fly aims its heading
 * along its speed when its schedule is a multiple of 4 */
static void flies_run(struct task *task)
{
    struct task_work *work = task->work;
    Float x;
    Float y;
    Float z;
    Float vx;
    Float vy;
    Float vz;
    Float dx;
    Float dy;
    Float dz;
    Float *position;
    Float *velocity;
    Float px;
    Float py;
    Float pz;
    Float player_squared;
    Float camera_squared;
    Float origin_squared;
    Float factor;
    Float speed_squared;
    Float far_squared;
    Float heading_squared;
    struct flies_vars *vars;
    Float distance;
    struct task_work *player;
    Float extent;
    Float share;
    /* @hack redundant: 12 bytes of frame at sp+116 that no code reads, of a
     * type unknown, declared as a Uint8[12]. the image's ADD #-128,R15 reserves
     * them (0x0c91eb18); without them, flies_run runs 64 bytes short and 772 of
     * 799 words differ */
    Uint8 unused[12];
    int count;
    struct fly *fly;
    Float player_x;
    Float player_y;
    Float player_z;
    Float camera_x;
    Float camera_y;
    Float camera_z;
    Float wall_x;
    Float wall_y;
    Float wall_z;

    if (placement_try_leave_by(task, RANGE)) {
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
        flies_draw_box(&work->scl);
        njPopMatrix(1);
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                      njScalor(&work->scl) + 16.0F)) {
        return;
    }
    if (frame_step) {
        return;
    }
    distance = njDistanceP2P(&camera_work->pos, &work->pos);
    if (distance > VARS(work)->extent + ALL_ACTIVE) {
        /* @bug logic: NONE_ACTIVE is 400 squared, but distance, from
         * njDistanceP2P (0x0c91ec5e), isn't squared (FADD at 0x0c91ec84).
         * the share then falls only 1 / 159900 a unit */
        /* @hack spelling: extent assigned inside the sum, which stores it at
         * sp+112 (0x0c91ec78). assigned in a statement of its own, 80 of 799
         * words differ */
        share = ((extent = VARS(work)->extent) - distance + NONE_ACTIVE) *
                (1.0F / (NONE_ACTIVE - ALL_ACTIVE));
        if (share < 0.0F) {
            share = 0.0F;
        }
        /* @hack spelling: the vars through a local pointer, set here and in the
         * else, which puts work + 8 in R4 for the count and active
         * (0x0c91ec8e to 0x0c91ecae). through VARS(work), 317 of 799 words
         * differ */
        vars = VARS(work);
        /* the active count is the share times the count, rounded down. so
         * just past ALL_ACTIVE, where the share is below 1, one fly drops
         * out */
        share = vars->count * share;
        vars->active = (int)share;
    } else {
        vars = VARS(work);
        vars->active = vars->count;
    }
    flies_draw(task);
    if (VARS(work)->active > 0) {
        count = VARS(work)->active;
        player = player_work[0];
        /* @hack redundant: two tests of player that never run. they keep
         * njSqrt's address in R9 and the schedule in R10 (0x0c91ed08,
         * 0x0c91ed3c), and px, py and pz at sp+36, sp+28 and sp+24
         * (0x0c91eeb0 to 0x0c91eecc). with one test, 36 of 799 words differ.
         * with neither, camera_work's address takes R8, which holds the mask
         * 7 (0x0c91ec54, 0x0c91ed06), and 271 of 799 words and 11 pool
         * words differ */
        if (0) {
            if (player) {
                if (player) {
                }
            }
        }
        /* @bug logic: player 0's and the camera's offsets from the box's
         * center take no turn (the FSUBs at 0x0c91ecd4 to 0x0c91ed12),
         * though a fly's position is in the box's turned axes. in a box that
         * ang.y turns, the flies take each offset as turned with the box. a
         * fly flees player 0 and the camera, and measures the camera's
         * distance, from where each would stand if it turned with the box */
        player_x = player->pos.x - work->pos.x;
        player_y = player->pos.y - work->pos.y;
        player_z = player->pos.z - work->pos.z;
        camera_x = camera_work->pos.x - work->pos.x;
        camera_y = camera_work->pos.y - work->pos.y;
        camera_z = camera_work->pos.z - work->pos.z;
        /* @hack redundant: camera_z copied to itself. it keeps camera_x,
         * camera_y and camera_z at sp+68, sp+80 and sp+64 (0x0c91ecfa to
         * 0x0c91ed14), and the decision's vx in FR12 and vz in FR13
         * (0x0c91ee96, 0x0c91ee9e). without it, 34 of 799 words differ */
        /* NOLINTNEXTLINE(clang-diagnostic-self-assign) */
        camera_z = camera_z;
        wall_x = work->scl.x;
        wall_y = work->scl.y;
        wall_z = work->scl.z;
        /* @hack redundant: a test of wall_x that never runs. it keeps
         * player_x to player_z at sp+104, sp+100 and sp+96 (0x0c91ecd6 to
         * 0x0c91ecee), and wall_x to wall_z at sp+84, sp+88 and sp+92
         * (0x0c91ed1c to 0x0c91ed2c). without it, 53 of 799 words differ */
        if (0) {
            if (wall_x) {
            }
        }
        fly = VARS(work)->flies;
        for (; count--; ++fly) {
            int schedule = TASK_TIMER(work) + count;
            Sint16 timer = fly->timer - 1;

            if (timer <= 0) {
                timer = 0;
            }
            /* @hack spelling: x loaded ahead of the speed, which keeps the
             * position's cursor in R4 and the speed's in R5 (0x0c91ed4e to
             * 0x0c91ed54). with the speed first, the two trade and 15 of 799
             * words differ */
            position = VECTOR_FLOATS(&fly->pos);
            velocity = VECTOR_FLOATS(&fly->speed);
            x = *position++;
            vx = *velocity++;
            vy = *velocity++;
            vz = *velocity;
            x += vx;
            y = *position++ + vy;
            z = *position + vz;
            /* @hack redundant: a test of z that never runs. it keeps x, y and
             * z in FR8, FR9 and FR7 (0x0c91ed5a to 0x0c91ed64). without it,
             * camera_work's address takes R8, which holds the mask 7
             * (0x0c91ec54, 0x0c91ed06), and 264 of 799 words and 11 pool
             * words differ */
            if (0) {
                if (z) {
                }
            }
            /* a fly at or past a wall steps back, and its speed across the
             * wall reverses */
            if (!(wall_x > _builtin_fabsf(x))) {
                x -= vx;
                vx = -vx;
            }
            if (!(wall_y > _builtin_fabsf(y))) {
                y -= vy;
                vy = -vy;
            }
            if (!(wall_z > _builtin_fabsf(z))) {
                z -= vz;
                vz = -vz;
            }
            vy = (vy + FLY_LIFT) * FLY_KEEP_Y;
            vx *= FLY_KEEP;
            vz *= FLY_KEEP;
            *velocity = vz;
            *--velocity = vy;
            *--velocity = vx;
            *position = z;
            *--position = y;
            *--position = x;
            /* a far fly skips its decision, aim and turn when its schedule is
             * a multiple of 8. otherwise the tests below take its schedule
             * shifted right 3 */
            if (fly->flags & FLY_FAR) {
                position = VECTOR_FLOATS(&fly->pos);
                dx = camera_x - *position++;
                dy = camera_y - *position++;
                dz = camera_z - *position;
                if ((dx * dx) + (dy * dy) + (dz * dz) < CAMERA_NEAR) {
                    fly->flags &= ~FLY_FAR;
                } else {
                    if (!(schedule & 7)) {
                        fly->timer = timer;
                        continue;
                    }
                    schedule >>= 3;
                }
            }
            if (!(schedule & 7)) {
                position = VECTOR_FLOATS(&fly->pos);
                x = *position++;
                y = *position++;
                z = *position;
                velocity = VECTOR_FLOATS(&fly->speed);
                vx = *velocity++;
                vy = *velocity++;
                vz = *velocity;
                if (!timer) {
                    px = x - player_x;
                    py = y - player_y;
                    pz = z - player_z;
                    player_squared = (px * px) + (py * py) + (pz * pz);
                    /* @hack redundant: a store to z that never runs. it keeps
                     * y at sp+8 and z at sp+12, and vx in FR12, vy at sp+0
                     * and vz in FR13 (0x0c91ee8a to 0x0c91ee9e). without it,
                     * 76 of 799 words differ */
                    if (0) {
                        z = vz;
                    }
                    if (player_squared < FLEE_PLAYER) {
                        timer = (Sint16)(rand() * (20.0F * RAND_SCALE));
                        factor = 1.0F / njSqrt(player_squared);
                        px *= factor;
                        py *= factor;
                        pz *= factor;
                        vx = px;
                        vy = py;
                        vz = pz;
                    } else {
                        Float nx;
                        Float ny;
                        Float nz;
                        Float cx;
                        Float cy;
                        Float cz;

                        nx = x - camera_x;
                        ny = y - camera_y;
                        nz = z - camera_z;
                        camera_squared = (nx * nx) + (ny * ny) + (nz * nz);
                        /* @hack spelling: cy set ahead of the camera's test,
                         * though only the far test reads it. it keeps cx at
                         * sp+48 and the origin's factor at sp+52 (0x0c91ef3c,
                         * 0x0c91f042). set beside cx and cz, the two trade,
                         * and 5 of 799 words differ */
                        cy = y - camera_y;
                        if (camera_squared < FLEE_CAMERA) {
                            timer = (Sint16)(rand() * (20.0F * RAND_SCALE));
                            factor = 1.0F / njSqrt(camera_squared);
                            nx *= factor;
                            ny *= factor;
                            nz *= factor;
                            vx = nx;
                            vy = ny;
                            vz = nz;
                        } else {
                            speed_squared = (vx * vx) + (vy * vy) + (vz * vz);
                            /* four times in five, a fly more than 1 from
                             * the box's center gains speed away from it.
                             * otherwise it gains speed in any direction */
                            if (!(speed_squared >
                                  WANDER_SPEED * WANDER_SPEED)) {
                                /* @hack redundant: a read of vx into a local
                                 * of its own that never runs. it keeps vx in
                                 * FR12 and vy at sp+0 (0x0c91ee96,
                                 * 0x0c91ee9a). without it, the two trade, and
                                 * 139 of 799 words differ */
                                if (0) {
                                    Float probe = vx;

                                    if (probe) {
                                    }
                                }
                                timer = (Sint16)(rand() * (40.0F * RAND_SCALE));
                                /* @hack spelling: origin_squared assigned
                                 * inside the test, which stores it at sp+108
                                 * (0x0c91f01e). assigned before the if,
                                 * player_x to player_z and wall_x to wall_z
                                 * each sit 4 bytes higher, and 18 of 799
                                 * words differ */
                                if ((origin_squared =
                                         (x * x) + (y * y) + (z * z)) > 1.0F &&
                                    !(rand() * RAND_SCALE > 0.8F)) {
                                    factor = 1.0F / njSqrt(origin_squared);
                                    vx += x * factor * rand() * RAND_SCALE;
                                    vy += y * factor * rand() * RAND_SCALE;
                                    vz += z * factor * rand() * RAND_SCALE;
                                } else {
                                    vx += ((rand() * RAND_SCALE) - 0.5F) * 2.0F;
                                    vy += ((rand() * RAND_SCALE) - 0.5F) * 2.0F;
                                    vz += ((rand() * RAND_SCALE) - 0.5F) * 2.0F;
                                }
                            } else {
                                cx = x - camera_x;
                                cz = z - camera_z;
                                far_squared = (cx * cx) + (cy * cy) + (cz * cz);
                                if (far_squared > CAMERA_NEAR) {
                                    fly->flags |= FLY_FAR;
                                }
                            }
                        }
                    }
                    /* one decision in 13 also nudges the speed at random */
                    if (rand() * RAND_SCALE < 1.0F / 13.0F) {
                        vx += ((rand() * RAND_SCALE) - 0.5F) * 0.16F;
                        vy += ((rand() * RAND_SCALE) - 0.5F) * 0.08F;
                        vz += ((rand() * RAND_SCALE) - 0.5F) * 0.16F;
                    }
                }
                velocity = VECTOR_FLOATS(&fly->speed);
                *velocity++ = vx;
                *velocity++ = vy;
                *velocity = vz;
            }
            velocity = VECTOR_FLOATS(&fly->speed);
            vx = *velocity++;
            vy = *velocity++;
            vz = *velocity;
            heading_squared = (vx * vx) + (vy * vy) + (vz * vz);
            /* @hack redundant: a test of vx that never runs. it keeps vx in
             * FR13, vy in FR12 and vz at sp+56 (0x0c91f1aa to 0x0c91f1b4),
             * and njSqrt's address in R9 and the schedule in R10 (0x0c91ed08,
             * 0x0c91ed3c). without it, 82 of 799 words differ */
            if (0) {
                if (vx) {
                }
            }
            if (heading_squared > 0.0F && !(schedule & 3)) {
                fly->target_heading =
                    (Angle)(atan2f(vy, (vx * njCos(camera_work->ang.y)) +
                                           (vz * njSin(camera_work->ang.y))) *
                            ANGLE_PER_RADIAN) +
                    0x4000;
            }
            /* turn the heading half of the way to its target, the short way
             * round */
            /* @hack spelling: the turn in three statements. as one expression,
             * camera_work's address takes R8, which holds the mask 7
             * (0x0c91ec54, 0x0c91ed06), and 288 of 799 words differ */
            {
                Sint16 delta = fly->target_heading - fly->heading;
                Float step = delta;
                int turn;

                step *= 0.5F;
                turn = (int)step;
                fly->heading += turn;
            }
            fly->timer = timer;
        }
        /* @hack redundant: a test of wall_y that never runs. it keeps
         * player_x to player_z at sp+104, sp+100 and sp+96 (0x0c91ecd6 to
         * 0x0c91ecee), and wall_y and wall_z at sp+88 and sp+92 (0x0c91ed24,
         * 0x0c91ed2c). without it, 32 of 799 words differ */
        if (0) {
            if (wall_y) {
            }
        }
    }
    TASK_TIMER(work)++;
    /* @hack redundant: a test of extent that never runs. it keeps extent's
     * store at sp+112 (0x0c91ec78), a slot nothing loads. without it, the
     * frame is 4 bytes smaller, and 772 of 799 words differ */
    if (0) {
        if (extent) {
        }
    }
}

/* draw the active flies on a frame's first step, each a sprite turned by
 * its heading. the sprites take flies_frames' two entries in turn, by
 * TASK_TIMER's low bit */
static void flies_draw(struct task *task)
{
    struct task_work *work;
    NJS_SPRITE sprite;
    struct fly *fly;
    int count;
    int frame;

    /* @hack spelling: work set inside the test with a comma, which keeps task's
     * store at sp+0 (0x0c91f2d0), though nothing reads it. set before the if,
     * 110 of 116 words differ */
    if (work = task->work, placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (frame_step > 0) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                      njScalor(&work->scl) + 20.0F)) {
        return;
    }
    /* @hack redundant: the address of camera_work's pos, taken and dropped.
     * with the store below that never runs, it loads camera_work's address
     * into R4 and its value into R3 (0x0c91f312, 0x0c91f316), though nothing
     * reads them. without it, 80 of 116 words differ */
    (void)&camera_work->pos;
    sprite.tlist = &obj_casino9_texlist;
    sprite.tanim = flies_frames;
    /* @hack redundant: a store to camera_work that never runs. without it,
     * the read above loads nothing, and 80 of 116 words differ */
    if (0) {
        camera_work = NULL;
    }
    /* @hack redundant: the scales times 1, for the image's FLDI1 FR4 and its
     * two FMULs (0x0c91f30a, 0x0c91f324, 0x0c91f32c). without it, 80 of 116
     * words differ */
    sprite.sx = flies_width * 1.0F;
    sprite.sy = flies_height * 1.0F;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    count = VARS(work)->active;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    fly = VARS(work)->flies;
    frame = TASK_TIMER(work) & 1;
    while (count--) {
        sprite.ang = fly->heading;
        sprite.p = fly->pos;
        njDrawSprite3D(&sprite, frame,
                       NJD_SPRITE_ANGLE | NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
        ++fly;
    }
    njPopMatrix(1);
}
