/* the buyon, which the set editor names E BUYON. buyon_step moves it and
 * keeps its collision. five child tasks, its parts, draw it. parts 0 and 4
 * always draw, and parts 1 to 3 draw while it grows, chases and shrinks */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "animal/kinds.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "effect/explosion.h"
#include "enemy/enemy.h"
#include "hud/hud.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg03/buyon.h"
#include "stg03/buyon_shapes.h"
#include "stg03/buyon_textures.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* the work's mode, for the buyon and for each of its parts. a part's mode
 * follows its parent's, but a part never turns or bursts */
enum buyon_mode {
    BUYON_INIT, /* the task's first step sets it up */
    /* it hops ahead, and turns when it meets a wall or an attack touches it */
    BUYON_WANDER,
    BUYON_TURN, /* it hops in place, turning toward a target near home */
    BUYON_GROW, /* it stands and swells */
    BUYON_CHASE, /* it follows player 0 */
    BUYON_SHRINK, /* it stands and shrinks back */
    BUYON_BURST, /* its next step bursts it */
    /* the buyon and its parts do nothing in it. no step sets it */
    BUYON_INACTIVE
};

/* a part's vars: the action it plays, and its frame in it. the parent's vars
 * hold copies of two parts' frames, and the chase's footsteps */
struct buyon_vars {
    union {
        NJS_ACTION *action; /* a part's */
        Float part_frame; /* the parent's copy of part 0's frame, unread */
    } animation;
    /* a part's frame. the parent's is a copy of part 4's, which the parent
     * moves and collides by */
    Float frame;
    /* the parent's footsteps sounded since its chase action last started
     * over, 0 to 2 */
    int footstep;
};

#define VARS(work) ((struct buyon_vars *)&(work)->vars)
/* the buyon's speed ahead, a Float kept in the word of
 * record->motion.record. a player's task keeps its player record there */
#define SPEED(record) (*(Float *)&(record)->motion.record)
/* the rate the parts play their actions at, in the buyon's scl.x */
#define RATE(work) ((work)->scl.x)

/* the buyon destroys itself once neither the camera nor player 0 is within
 * 1000 units */
#define BUYON_RANGE (1000.0F * 1000.0F)

/* the record's sight and near distances, 100 and 50, squared as the record
 * keeps them */
#define SIGHT_RANGE (100.0F * 100.0F)
#define NEAR_RANGE (50.0F * 50.0F)
/* player 0 is near the buyon only within NEAR_HEIGHT of its height */
#define NEAR_HEIGHT 30.0F

/* the buyon counts as facing aim_y within FACING of it */
#define FACING NJM_DEG_ANG(30)

/* a target lies within half TARGET_SPREAD of home along x and z. the spread
 * is 0x419fffff, the float just below 20 */
#define TARGET_SPREAD 19.999999F

/* as the buyon wanders or turns, it hops up at HOP_LIFT when its frame
 * reaches HOP_FRAME. a wandering hop goes ahead at HOP_SPEED too. from
 * HOP_FRAME to HOP_END, a wandering buyon slows by HOP_DRAG a step. a
 * turning one turns toward aim_y by its record's turn_speed */
#define HOP_FRAME 14.0F
#define HOP_END 30.0F
#define HOP_LIFT 2.0F
#define HOP_SPEED 1.6F
#define HOP_DRAG 0.1F

/* as the buyon chases, a footstep sounds when its frame passes FOOTSTEP_1
 * and FOOTSTEP_2. from FOOTSTEP_1 to FOOTSTEP_1_END, and from FOOTSTEP_2 to
 * FOOTSTEP_2_END, the buyon brakes by CHASE_BRAKE a step. the buyon brakes
 * the same way while it doesn't face player 0. otherwise the buyon gains
 * CHASE_ACCEL a step, up to CHASE_SPEED */
#define FOOTSTEP_1 9.0F
#define FOOTSTEP_1_END 15.0F
#define FOOTSTEP_2 39.0F
#define FOOTSTEP_2_END 45.0F
#define CHASE_BRAKE 0.2F
#define CHASE_ACCEL 0.02F
/* 0x3e570a3c, the float just below 0.21 */
#define CHASE_SPEED 0.20999999F

/* the speed buyon_move adds to the buyon's fall each step */
#define GRAVITY 0.3F

/* growing, chasing and shrinking each end on the step the timer passes its
 * count */
#define GROW_STEPS 50
#define CHASE_STEPS 600
#define SHRINK_STEPS 30

/* the record's radius once the buyon has grown */
#define GROWN_RADIUS 10.61F

/* player 0 dents part 1's model only within GROWN_RADIUS of the buyon
 * across. player 0's pos must also lie from DENT_BELOW below the buyon's to
 * DENT_ABOVE above it */
#define DENT_BELOW 1.0F
#define DENT_ABOVE 32.8415F
/* a point within DENT_GAP of player 0's middle moves in to DENT_GAP short of
 * it. it lands on the line from shape 2's center to that middle. each of the
 * point's neighbours moves in to DENT_PULL of its distance from shape 2's
 * center */
#define DENT_GAP 5.0F
#define DENT_PULL 0.85F
/* a point that hangs from part 0 drops HANG_DROP, then goes where part 0's
 * pose puts it. row 0 of buyon_vertices hangs on every frame. rows 1 and 2
 * hang all through the chase. in the growth, they hang from frames
 * HANG_1_FROM and HANG_2_FROM on. in the shrink, they hang up to frames
 * HANG_1_UNTIL and HANG_2_UNTIL */
#define HANG_DROP 27.0F
#define HANG_1_FROM 40.0F
#define HANG_2_FROM 50.0F
#define HANG_1_UNTIL 25.0F
#define HANG_2_UNTIL 20.0F

#define SE_GROW 78
#define SE_SHRINK 79
#define SE_FOOTSTEP 80
#define SE_BOUNCE 81

/* in work->tick: a bounce of player 0 is due. buyon_push_player sets and
 * clears the bit in one call */
#define BOUNCE_DUE 1

static const NJS_VECTOR buyon_zero_velocity = {0.0F, 0.0F, 0.0F};

/* scratch. a function that reads one of these has written it first. none
 * keeps a value from one call to the next */
static NJS_MATRIX buyon_inverse_matrix;
static NJS_MATRIX buyon_world_matrix;
static NJS_MATRIX buyon_pose_matrix;

extern NJS_ACTION buyon_part_0_wander;
extern NJS_ACTION buyon_part_3_wander;
extern NJS_ACTION buyon_part_2_wander;
extern NJS_ACTION buyon_part_0_chase;
extern NJS_ACTION buyon_part_3_chase;
extern NJS_ACTION buyon_part_2_chase;
extern NJS_ACTION buyon_part_0_grow;
extern NJS_ACTION buyon_part_3_grow;
extern NJS_ACTION buyon_part_2_grow;

extern NJS_ACTION buyon_part_4_wander;
extern NJS_ACTION buyon_part_1_grow;
extern NJS_ACTION buyon_part_1_chase;
extern NJS_ACTION buyon_part_0_shrink;
extern NJS_ACTION buyon_part_1_shrink;
extern NJS_ACTION buyon_part_2_shrink;
extern NJS_ACTION buyon_part_3_shrink;
extern NJS_ACTION buyon_part_4_grow;
extern NJS_ACTION buyon_part_4_chase;
extern NJS_ACTION buyon_part_4_shrink;

/* the points of part 1's model that buyon_deformed_draw bends, by index into
 * its points. they lie in 6 rows of 6. each row closes, so column 5
 * neighbours column 0 */
extern Uint32 buyon_vertices[6][6];

extern struct task_spec buyon_parts[];

/* the parts' destructor, which does nothing */
static void buyon_part_destroy(struct task *task)
{
    /* @hack redundant: an empty test of placement_editing's result, whose TST
     * R0,R0 fills the return's delay slot (0x0c90874c). as a call alone, the
     * call becomes a jump, and 7 of 7 words differ */
    if (placement_editing(task)) {
    }
}

static void buyon_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = (struct lockon_target *)task->motion;
    task_destroy_children(task);
    enemy_end(task, work, record);
}

/* draw the part's action at its frame, where its work places and turns it.
 * it draws only in a display frame's first step, while frame_step is 0 */
static void buyon_part_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle angle;
    if (!frame_step) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, angle);
        njSetTexture(&buyon_texlist);
        draw_action(VARS(work)->animation.action, VARS(work)->frame);
        njPopMatrix(1);
    }
}

/* draw the part, then advance its frame at its parent's RATE. the frame goes
 * back to 0 once it reaches the motion's nbFrame */
static void buyon_part_advance(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    buyon_part_draw(task);
    VARS(work)->frame += RATE(parent);
    if (VARS(work)->frame >= VARS(work)->animation.action->motion->nbFrame) {
        VARS(work)->frame = 0.0F;
    }
}

/* pick a target at random near home, at the buyon's height. set aim_y to the
 * heading toward it */
static void buyon_choose_target(struct task_work *work,
                                struct lockon_target *record)
{
    record->target.x =
        record->home.x + (((rand() * RAND_SCALE) - 0.5F) * TARGET_SPREAD);
    record->target.y = work->pos.y;
    record->target.z =
        record->home.z + (((rand() * RAND_SCALE) - 0.5F) * TARGET_SPREAD);
    record->aim_y = (Angle)(atan2f(record->target.x - work->pos.x,
                                   record->target.z - work->pos.z) *
                            ANGLE_PER_RADIAN);
}

/* set the buyon's speed by its mode, then move it ahead along ang.y and let
 * it fall */
static void buyon_move(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = (struct lockon_target *)task->motion;

    switch (work->mode) {
    case BUYON_WANDER:
        if (njAbs(VARS(work)->frame - HOP_FRAME) < 1.0F) {
            SPEED(record) = HOP_SPEED;
            record->motion.speed.y = HOP_LIFT;
        }
        if (VARS(work)->frame >= HOP_FRAME && VARS(work)->frame <= HOP_END) {
            if (njAbs(SPEED(record)) < HOP_DRAG) {
                SPEED(record) = 0.0F;
            } else if (SPEED(record) < 0.0F) {
                SPEED(record) += HOP_DRAG;
            } else {
                SPEED(record) -= HOP_DRAG;
            }
        }
        break;
    case BUYON_TURN:
        if (njAbs(VARS(work)->frame - HOP_FRAME) < 1.0F) {
            record->motion.speed.y = HOP_LIFT;
        }
        if (VARS(work)->frame >= HOP_FRAME && VARS(work)->frame <= HOP_END) {
            work->ang.y =
                angle_approach(work->ang.y, record->aim_y, record->turn_speed);
        }
        if (angle_distance(work->ang.y, record->aim_y) < FACING) {
            work->mode = BUYON_WANDER;
            SPEED(record) = 0.0F;
        }
        break;
    case BUYON_CHASE:
        record->target = player_work[0]->pos;
        record->aim_y = (Angle)(atan2f(record->target.x - work->pos.x,
                                       record->target.z - work->pos.z) *
                                ANGLE_PER_RADIAN);
        work->ang.y =
            angle_approach(work->ang.y, record->aim_y, record->turn_speed);
        if (angle_distance(work->ang.y, record->aim_y) < FACING) {
            if (VARS(work)->frame < FOOTSTEP_1 ||
                (VARS(work)->frame > FOOTSTEP_1_END &&
                 VARS(work)->frame < FOOTSTEP_2) ||
                VARS(work)->frame > FOOTSTEP_2_END) {
                SPEED(record) += CHASE_ACCEL;
                if (SPEED(record) > CHASE_SPEED) {
                    SPEED(record) = CHASE_SPEED;
                }
            } else {
                SPEED(record) += -CHASE_BRAKE;
                if (SPEED(record) < 0.0F) {
                    SPEED(record) = 0.0F;
                }
            }
        } else {
            SPEED(record) += -CHASE_BRAKE;
            if (SPEED(record) < 0.0F) {
                SPEED(record) = 0.0F;
            }
        }
        switch (VARS(work)->footstep) {
        case 0:
            if (VARS(work)->frame > FOOTSTEP_1) {
                VARS(work)->footstep++;
                sound_play_at(SE_FOOTSTEP, 0, 0, 0, work->pos.x, work->pos.y,
                              work->pos.z);
            }
            break;
        case 1:
            if (VARS(work)->frame > FOOTSTEP_2) {
                VARS(work)->footstep++;
                sound_play_at(SE_FOOTSTEP, 0, 0, 0, work->pos.x, work->pos.y,
                              work->pos.z);
            }
            break;
        case 2:
            if (VARS(work)->frame < FOOTSTEP_2) {
                VARS(work)->footstep = 0;
            }
            break;
        }
        break;
    }
    record->motion.speed.x = SPEED(record) * njSin(-work->ang.y + 0x8000);
    work->pos.x += record->motion.speed.x;
    record->motion.speed.z = SPEED(record) * njCos(-work->ang.y + 0x8000);
    work->pos.z -= record->motion.speed.z;
    record->motion.speed.y += -GRAVITY;
    work->pos.y += record->motion.speed.y;
}

/* move the buyon out of the world's surfaces. a wandering buyon that an
 * attack touches, or that meets a wall, goes back to last_pos. it then turns
 * toward a new target */
static void buyon_resolve_contact(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = (struct lockon_target *)task->motion;
    enemy_collide(work, record);
    if (work->mode == BUYON_WANDER) {
        if (((work->collision->flags & COLLISION_HIT) &&
             work->collision->hit->kind == COLLISION_ATTACK) ||
            (record->flags & ENEMY_ON_WALL)) {
            work->mode = BUYON_TURN;
            buyon_choose_target(work, record);
            work->pos = record->last_pos;
            SPEED(record) = 0.0F;
        }
    }
}

/* shapes 1, 2 and 3 follow parts 0, 3 and 2. each center in buyon_shapes is
 * posed by node 0 of its part's action, at the buyon's frame. a shrinking
 * buyon, like a wandering or turning one, poses the wander actions */
static void buyon_update_collision(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ACTION *first;
    NJS_ACTION *second;
    NJS_ACTION *third;
    NJS_POINT3 transformed;
    NJS_POINT3 local;

    switch (work->mode) {
    case BUYON_GROW:
        first = &buyon_part_0_grow;
        second = &buyon_part_3_grow;
        third = &buyon_part_2_grow;
        break;
    case BUYON_CHASE:
        first = &buyon_part_0_chase;
        second = &buyon_part_3_chase;
        third = &buyon_part_2_chase;
        break;
    default:
        first = &buyon_part_0_wander;
        second = &buyon_part_3_wander;
        third = &buyon_part_2_wander;
        break;
    }
    if (pose_action(&_unit_matrix_, first, VARS(work)->frame, 0,
                    &buyon_world_matrix)) {
        local = buyon_shapes[1].center;
        njCalcPoint(&buyon_world_matrix, &local, &transformed);
        work->collision->shapes[1].center = transformed;
    }
    if (pose_action(&_unit_matrix_, second, VARS(work)->frame, 0,
                    &buyon_world_matrix)) {
        local = buyon_shapes[2].center;
        njCalcPoint(&buyon_world_matrix, &local, &transformed);
        work->collision->shapes[2].center = transformed;
    }
    if (pose_action(&_unit_matrix_, third, VARS(work)->frame, 0,
                    &buyon_world_matrix)) {
        local = buyon_shapes[3].center;
        njCalcPoint(&buyon_world_matrix, &local, &transformed);
        work->collision->shapes[3].center = transformed;
    }
}

/* bounce player 0 off the buyon. a player that hurts the buyon starts its
 * burst, and player 0 hops up at 2. while it grows, chases or shrinks, a
 * player's touch on shape 2 or 3 throws player 0 off. the throw points from
 * the shape's center to player 0, 2 long. the buyon's speed along x and z
 * adds to it. a downward throw turns up, and the whole then doubles */
static void buyon_push_player(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision = work->collision;
    struct collision_work *contact = collision->hit;
    struct lockon_target *record = (struct lockon_target *)task->motion;
    NJS_VECTOR velocity;
    velocity = buyon_zero_velocity;
    /* @hack spelling: the test of contact's kind nested inside the test of
     * COLLISION_DAMAGED, which gives BUYON_BURST's 6 R2 (MOV #6,R2 at
     * 0x0c908eda); as one test with &&, R3 takes it and 6 of 190 words
     * differ */
    if (work->flags & COLLISION_DAMAGED) {
        if (contact->kind == COLLISION_PLAYER) {
            velocity.y = 2.0F;
            work->tick |= BOUNCE_DUE;
            work->submode = work->mode;
            work->mode = BUYON_BURST;
        }
    }
    switch (work->mode) {
    case BUYON_GROW:
    case BUYON_CHASE:
    case BUYON_SHRINK:
        /* hit_shape is -1 for no touch */
        if (collision->shapes[2].hit_shape > -1 &&
            contact->kind == COLLISION_PLAYER) {
            velocity = collision->shapes[2].center;
        } else if (collision->shapes[3].hit_shape > -1 &&
                   contact->kind == COLLISION_PLAYER) {
            velocity = collision->shapes[3].center;
        } else {
            break;
        }
        njAddVector(&velocity, &work->pos);
        njSubVector(&velocity, &player_work[0]->pos);
        njUnitVector(&velocity);
        velocity.x *= -2.0F;
        velocity.y *= -2.0F;
        velocity.z *= -2.0F;
        velocity.x += record->motion.speed.x;
        velocity.z -= record->motion.speed.z;
        if (velocity.y < 0.0F) {
            velocity.y *= -1.0F;
        }
        velocity.x *= 2.0F;
        velocity.y *= 2.0F;
        velocity.z *= 2.0F;
        collision->shapes[2].hit_shape = -1;
        work->tick |= BOUNCE_DUE;
        break;
    }
    if (work->tick & BOUNCE_DUE) {
        player_set_speed(0, velocity.x, velocity.y, velocity.z);
        work->tick &= ~BOUNCE_DUE;
        sound_play(SE_BOUNCE, 0, 0, 0);
        vibration_start(0, 4, 15, 0);
    }
}

/* 1 while player 0 is near the buyon, else 0 */
static int buyon_player_near(struct task *task)
{
    struct task_work *work = task->work;
    int near = 0;
    Float distance;
    if (njAbs(work->pos.y - player_work[0]->pos.y) < NEAR_HEIGHT) {
        distance = njDistanceP2P(&work->pos, &player_work[0]->pos);
        /* @bug logic: the tests hold the distance, not its square, to the
         * squared SIGHT_RANGE and NEAR_RANGE (FCMP/GT at 0x0c9090b2 and
         * 0x0c9090ba). so player 0 counts as near out to 10000, and the
         * second test passes nothing the first fails */
        if (distance < SIGHT_RANGE || distance < NEAR_RANGE) {
            near = 1;
        }
    }
    return near;
}

/* pull a point of part 1's model in toward shape 2's center, to DENT_PULL of
 * its distance. row and column pick the point from buyon_vertices. the
 * part's action poses both */
static void buyon_contract_vertex(struct task *task, int row, int column)
{
    struct task_work *work = task->work;
    NJS_POINT3 vertex;
    NJS_POINT3 center;
    NJS_POINT3 local;
    NJS_POINT3 transformed;
    Float length;
    Angle angle;
    njUnitMatrix(&buyon_world_matrix);
    TURN_MATRIX_VIA(njRotateY, &buyon_world_matrix, work->ang.y, angle);
    if (pose_action(&_unit_matrix_, VARS(work)->animation.action,
                    VARS(work)->frame, 0, &buyon_pose_matrix)) {
        pose_get_matrix(0, &buyon_pose_matrix);
        local = VARS(work)
                    ->animation.action->object->model
                    ->points[buyon_vertices[row][column]];
        njCalcPoint(&buyon_pose_matrix, &local, &transformed);
        njCalcPoint(&buyon_world_matrix, &transformed, &vertex);
        njAddVector(&vertex, &work->pos);
        local = buyon_shapes[2].center;
        njCalcPoint(&buyon_pose_matrix, &local, &transformed);
        njCalcPoint(&buyon_world_matrix, &transformed, &center);
        njAddVector(&center, &work->pos);
        local = vertex;
        njSubVector(&local, &center);
        length = njScalor(&local) * DENT_PULL;
        njUnitVector(&local);
        local.x *= length;
        local.y *= length;
        local.z *= length;
        njAddVector(&local, &center);
        njSubVector(&local, &work->pos);
        njInvertMatrix(&buyon_pose_matrix);
        njInvertMatrix(&buyon_world_matrix);
        njCalcPoint(&buyon_world_matrix, &local, &transformed);
        njCalcPoint(&buyon_pose_matrix, &transformed, &vertex);
        VARS(work)
            ->animation.action->object->model
            ->points[buyon_vertices[row][column]] = vertex;
    }
}

/* dent part 1's model near player 0's middle. a point in rows 1 to 4 of
 * buyon_vertices within DENT_GAP of it moves in. so do the point's neighbours
 * along its row and column, but for those in rows 0 and 5 */
static void buyon_push_vertices(struct task *task)
{
    int row;
    int column;
    int side;
    int neighbour;
    struct task_work *work = task->work;
    NJS_POINT3 vertex;
    NJS_POINT3 middle;
    NJS_POINT3 local;
    NJS_POINT3 transformed;
    NJS_POINT3 center;
    Float length;
    Float distance;
    Angle angle;
    struct player *player;
    for (row = 1; row < 5; row++) {
        int previous_row;
        int next_row;
        /* @hack spelling: column zeroed ahead of the row indices, which sets
         * previous_row before the address of buyon_vertices[row] (ADD #-1,R3
         * at 0x0c909366, ADD R0,R1 at 0x0c909368); zeroed in the for, the two
         * trade places, and 2 of 375 words differ */
        column = 0;
        previous_row = row - 1;
        next_row = row + 1;
        for (; column < 6; column++) {
            njUnitMatrix(&buyon_world_matrix);
            TURN_MATRIX_VIA(njRotateY, &buyon_world_matrix, work->ang.y, angle);
            if (pose_action(&_unit_matrix_, VARS(work)->animation.action,
                            VARS(work)->frame, 0, &buyon_pose_matrix)) {
                pose_get_matrix(0, &buyon_pose_matrix);
                local = VARS(work)
                            ->animation.action->object->model
                            ->points[buyon_vertices[row][column]];
                njCalcPoint(&buyon_pose_matrix, &local, &transformed);
                njCalcPoint(&buyon_world_matrix, &transformed, &vertex);
                njAddVector(&vertex, &work->pos);
                middle = player_work[0]->pos;
                player = player_get(0);
                middle.y += player->consts.center_y;
                distance = njDistanceP2P(&vertex, &middle);
                if (distance < DENT_GAP) {
                    local = buyon_shapes[2].center;
                    njCalcPoint(&buyon_pose_matrix, &local, &transformed);
                    njCalcPoint(&buyon_world_matrix, &transformed, &center);
                    njAddVector(&center, &work->pos);
                    local = middle;
                    njSubVector(&local, &center);
                    length = njScalor(&local) - DENT_GAP;
                    njUnitVector(&local);
                    local.x *= length;
                    local.y *= length;
                    local.z *= length;
                    njInvertMatrix(&buyon_world_matrix);
                    njInvertMatrix(&buyon_pose_matrix);
                    njAddVector(&local, &center);
                    njSubVector(&local, &work->pos);
                    njCalcPoint(&buyon_world_matrix, &local, &transformed);
                    njCalcPoint(&buyon_pose_matrix, &transformed, &vertex);
                    VARS(work)
                        ->animation.action->object->model
                        ->points[buyon_vertices[row][column]] = vertex;

                    /* @hack redundant: a store of column to previous_row
                     * that never runs. it gives previous_row, next_row and
                     * the address of buyon_vertices[row] the frame slots
                     * sp+12, sp+8 and sp+4 (0x0c90936a to 0x0c90936e). it
                     * gives DENT_GAP and its negation FR14 and FR13
                     * (0x0c909430, 0x0c909506). without it, 11 of 375 words
                     * differ */
                    if (0) {
                        previous_row = column;
                    }
                    for (side = 0; side < 4; side++) {
                        switch (side) {
                        case 0:
                            if (previous_row > 0) {
                                buyon_contract_vertex(task, previous_row,
                                                      column);
                            }
                            break;
                        case 1:
                            if (next_row < 5) {
                                buyon_contract_vertex(task, next_row, column);
                            }
                            break;
                        case 2:
                            neighbour = column - 1;
                            if (neighbour < 0) {
                                neighbour = 5;
                            }
                            buyon_contract_vertex(task, row, neighbour);
                            break;
                        case 3:
                            neighbour = column + 1;
                            if (neighbour > 5) {
                                neighbour = 0;
                            }
                            buyon_contract_vertex(task, row, neighbour);
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }
    }
}

/* part 1's draw. it bends part 1's model, draws it, then puts the model's
 * points back. every buyon draws from that one model. it draws only while
 * the buyon grows, chases or shrinks. rows 0 to 2 of buyon_vertices hang
 * from part 0's pose. rows 1 and 2 don't early in the growth or late in the
 * shrink. player 0 near the buyon dents the model */
static void buyon_deformed_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ACTION *action;
    NJS_POINT3 saved[6][6];
    NJS_POINT3 transformed;
    NJS_POINT3 point;
    Float height[6][6];
    Float dx;
    Float dy;
    Float dz;
    int row;
    int column;
    if (frame_step) {
        return;
    }
    switch (work->mode) {
    case BUYON_GROW:
        action = &buyon_part_0_grow;
        break;
    case BUYON_CHASE:
        action = &buyon_part_0_chase;
        break;
    case BUYON_SHRINK:
        action = &buyon_part_0_shrink;
        break;
    default:
        return;
    }

    /* @hack redundant: a test of action that never runs. it puts
     * buyon_pose_matrix's address at sp+12 and buyon_inverse_matrix's at
     * sp+16 (0x0c90975e, 0x0c909760). it puts dx in FR5 and dy in FR4
     * (0x0c909908, 0x0c90990e). without it, each pair trades places, and 26
     * of 439 words differ */
    if (0) {
        if (action) {
        }
    }
    if (pose_action(NULL, VARS(work)->animation.action, VARS(work)->frame, 0,
                    &buyon_world_matrix)) {
        njSetMatrix(&buyon_inverse_matrix, &buyon_world_matrix);
        njInvertMatrix(&buyon_inverse_matrix);
        if (pose_action(NULL, action, VARS(work)->frame, 0,
                        &buyon_pose_matrix)) {
            pose_get_matrix(0, &buyon_pose_matrix);
            for (row = 0; row < 6; row++) {
                for (column = 0; column < 6; column++) {
                    saved[row][column] =
                        VARS(work)
                            ->animation.action->object->model
                            ->points[buyon_vertices[row][column]];
                    height[row][column] = saved[row][column].y;
                    switch (row) {
                    case 2: {
                        /* @hack spelling: the mode read into an int, which
                         * keeps it in R4 for the second test (MOV R0,R4 at
                         * 0x0c9097f0, MOV R4,R0 at 0x0c909802); as
                         * work->mode in both tests, 81 of 439 words differ */
                        int mode = (int)work->mode;
                        if (mode == BUYON_GROW &&
                            VARS(work)->frame < HANG_2_FROM) {
                            break;
                        }
                        if (mode == BUYON_SHRINK &&
                            VARS(work)->frame > HANG_2_UNTIL) {
                            break;
                        }
                        /* fall through */
                    }
                    case 1: {
                        /* @hack spelling: the mode read into an int, which
                         * keeps it in R4 for the second test (MOV R0,R4 at
                         * 0x0c90981a, MOV R4,R0 at 0x0c909828); as
                         * work->mode in both tests, 59 of 439 words differ */
                        int mode = (int)work->mode;
                        if (mode == BUYON_GROW &&
                            VARS(work)->frame < HANG_1_FROM) {
                            break;
                        }
                        if (mode == BUYON_SHRINK &&
                            VARS(work)->frame > HANG_1_UNTIL) {
                            break;
                        }
                        /* fall through */
                    }
                    case 0:
                        saved[row][column].y -= HANG_DROP;
                        njCalcPoint(&buyon_pose_matrix, &saved[row][column],
                                    &transformed);
                        njCalcPoint(&buyon_inverse_matrix, &transformed,
                                    &point);
                        VARS(work)
                            ->animation.action->object->model
                            ->points[buyon_vertices[row][column]] = point;
                        break;
                    }
                }
            }
            dx = player_work[0]->pos.x - work->pos.x;
            dy = player_work[0]->pos.y - work->pos.y;
            dz = player_work[0]->pos.z - work->pos.z;
            if (dy > -DENT_BELOW && dy < DENT_ABOVE) {
                Float squared = (dx * dx) + (dz * dz);
                if (njSqrt(squared) < GROWN_RADIUS) {
                    buyon_push_vertices(task);
                }
            }
            buyon_part_draw(task);
            for (row = 0; row < 6; row++) {
                /* @hack spelling: each row of saved reached through saved_row,
                 * with column zeroed ahead of it. it keeps the image's
                 * registers in the loop that saves the points and in this one
                 * (0x0c909794 to 0x0c9097d6, 0x0c909a46 to 0x0c909a88). as
                 * saved[row][column] in a plain for, 28 of 439 words differ */
                NJS_POINT3(*saved_row)[6];
                column = 0;
                saved_row = &saved[row];
                for (; column < 6; column++) {
                    (*saved_row)[column].y = height[row][column];
                    VARS(work)
                        ->animation.action->object->model
                        ->points[buyon_vertices[row][column]] =
                        (*saved_row)[column];
                }
            }
        } else {
            buyon_part_draw(task);
        }
    } else {
        buyon_part_draw(task);
    }
}

/* buyon_part_advance for part 1, which draws it bent */
static void buyon_deformed_advance(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    buyon_deformed_draw(task);
    VARS(work)->frame += RATE(parent);
    if (VARS(work)->frame >= VARS(work)->animation.action->motion->nbFrame) {
        VARS(work)->frame = 0.0F;
    }
}

void buyon_part_0_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    work->pos = parent->pos;
    work->ang = parent->ang;
    switch (work->mode) {
    case BUYON_INIT:
        work->mode = BUYON_WANDER;
        work->scl.x = work->scl.y = work->scl.z = 1.0F;
        VARS(work)->animation.action = &buyon_part_0_wander;
        task->destructor = buyon_part_destroy;
        task->draw = buyon_part_draw;
        break;
    case BUYON_WANDER:
        buyon_part_advance(task);
        if (parent->mode == BUYON_GROW) {
            work->mode = BUYON_GROW;
            VARS(work)->animation.action = &buyon_part_0_grow;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_TURN:
        break;
        /* @hack redundant: a second break, which never runs. it makes the
         * BUYON_GROW case load buyon_part_advance's address into R2
         * (0x0c909bcc). without it, R3 takes the address, and 2 of 117 words
         * differ */
        break;
    case BUYON_GROW:
        buyon_part_advance(task);
        if (parent->mode == BUYON_CHASE) {
            work->mode = BUYON_CHASE;
            VARS(work)->animation.action = &buyon_part_0_chase;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_CHASE:
        buyon_part_advance(task);
        if (parent->mode == BUYON_SHRINK) {
            work->mode = BUYON_SHRINK;
            VARS(work)->animation.action = &buyon_part_0_shrink;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_SHRINK:
        buyon_part_advance(task);
        if (parent->mode == BUYON_WANDER || parent->mode == BUYON_TURN) {
            work->mode = BUYON_WANDER;
            VARS(work)->animation.action = &buyon_part_0_wander;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_INACTIVE:
        break;
    }
    VARS(parent)->animation.part_frame = VARS(work)->frame;
}

void buyon_part_1_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    NJS_ACTION *initial = &buyon_part_1_grow;
    work->pos = parent->pos;
    work->ang = parent->ang;
    switch (work->mode) {
    case BUYON_INIT:
        work->mode = BUYON_WANDER;
        VARS(work)->animation.action = initial;
        task->destructor = buyon_part_destroy;
        work->scl.x = work->scl.y = work->scl.z = 0.0F;
        break;
    case BUYON_WANDER:
        if (parent->mode == BUYON_GROW) {
            work->mode = BUYON_GROW;
            VARS(work)->animation.action = initial;
            task->draw = buyon_deformed_draw;
            VARS(work)->frame = 0.0F;
            work->scl.y = 0.0F;
        }
        break;
    case BUYON_TURN:
        break;
    case BUYON_GROW:
        buyon_deformed_advance(task);
        if (parent->mode == BUYON_CHASE) {
            work->mode = BUYON_CHASE;
            VARS(work)->animation.action = &buyon_part_1_chase;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_CHASE:
        buyon_deformed_advance(task);
        if (parent->mode == BUYON_SHRINK) {
            work->mode = BUYON_SHRINK;
            VARS(work)->animation.action = &buyon_part_1_shrink;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_SHRINK:
        buyon_deformed_advance(task);
        if (parent->mode == BUYON_WANDER || parent->mode == BUYON_TURN) {
            work->mode = BUYON_WANDER;
            task->draw = NULL;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_INACTIVE:
        break;
    }
}

void buyon_part_2_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    NJS_ACTION *initial = &buyon_part_2_grow;
    work->pos = parent->pos;
    work->ang = parent->ang;
    switch (work->mode) {
    case BUYON_INIT:
        work->mode = BUYON_WANDER;
        VARS(work)->animation.action = initial;
        task->destructor = buyon_part_destroy;
        break;
    case BUYON_WANDER:
        if (parent->mode == BUYON_GROW) {
            work->mode = BUYON_GROW;
            VARS(work)->animation.action = initial;
            task->draw = buyon_part_draw;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_TURN:
        break;
        /* @hack redundant: a second break, which never runs. it makes the
         * BUYON_GROW case load buyon_part_advance's address into R2
         * (0x0c909dd2). without it, R3 takes the address, and 2 of 105 words
         * differ */
        break;
    case BUYON_GROW:
        buyon_part_advance(task);
        if (parent->mode == BUYON_CHASE) {
            work->mode = BUYON_CHASE;
            VARS(work)->animation.action = &buyon_part_2_chase;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_CHASE:
        buyon_part_advance(task);
        if (parent->mode == BUYON_SHRINK) {
            work->mode = BUYON_SHRINK;
            VARS(work)->animation.action = &buyon_part_2_shrink;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_SHRINK:
        buyon_part_advance(task);
        if (parent->mode == BUYON_WANDER || parent->mode == BUYON_TURN) {
            work->mode = BUYON_WANDER;
            task->draw = NULL;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_INACTIVE:
        break;
    }
}

void buyon_part_3_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    work->pos = parent->pos;
    work->ang = parent->ang;
    switch (work->mode) {
    case BUYON_INIT:
        work->mode = BUYON_WANDER;
        work->scl.x = work->scl.y = work->scl.z = 1.0F;
        task->destructor = buyon_part_destroy;
        break;
    case BUYON_WANDER:
        if (parent->mode == BUYON_GROW) {
            work->mode = BUYON_GROW;
            VARS(work)->animation.action = &buyon_part_3_grow;
            task->draw = buyon_part_draw;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_TURN:
        break;
        /* @hack redundant: a second break, which never runs. it makes the
         * BUYON_GROW case load buyon_part_advance's address into R2
         * (0x0c909ef0). without it, R3 takes the address, and 2 of 113 words
         * differ */
        break;
    case BUYON_GROW:
        buyon_part_advance(task);
        if (parent->mode == BUYON_CHASE) {
            work->mode = BUYON_CHASE;
            VARS(work)->animation.action = &buyon_part_3_chase;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_CHASE:
        buyon_part_advance(task);
        if (parent->mode == BUYON_SHRINK) {
            work->mode = BUYON_SHRINK;
            VARS(work)->animation.action = &buyon_part_3_shrink;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_SHRINK:
        buyon_part_advance(task);
        if (parent->mode == BUYON_WANDER || parent->mode == BUYON_TURN) {
            work->mode = BUYON_WANDER;
            task->draw = NULL;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_INACTIVE:
        break;
    }
}

void buyon_part_4_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    work->pos = parent->pos;
    work->ang = parent->ang;
    switch (work->mode) {
    case BUYON_INIT:
        work->mode = BUYON_WANDER;
        work->scl.x = work->scl.y = work->scl.z = 1.0F;
        VARS(work)->animation.action = &buyon_part_4_wander;
        task->destructor = buyon_part_destroy;
        task->draw = buyon_part_draw;
        break;
    case BUYON_WANDER:
        buyon_part_advance(task);
        if (parent->mode == BUYON_GROW) {
            work->mode = BUYON_GROW;
            VARS(work)->animation.action = &buyon_part_4_grow;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_TURN:
        break;
        /* @hack redundant: a second break, which never runs. it makes the
         * BUYON_GROW case load buyon_part_advance's address into R2
         * (0x0c909ffa). without it, R3 takes the address, and 2 of 118 words
         * differ */
        break;
    case BUYON_GROW:
        buyon_part_advance(task);
        if (parent->mode == BUYON_CHASE) {
            work->mode = BUYON_CHASE;
            VARS(work)->animation.action = &buyon_part_4_chase;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_CHASE:
        buyon_part_advance(task);
        if (parent->mode == BUYON_SHRINK) {
            work->mode = BUYON_SHRINK;
            VARS(work)->animation.action = &buyon_part_4_shrink;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_SHRINK:
        buyon_part_advance(task);
        if (parent->mode == BUYON_WANDER || parent->mode == BUYON_TURN) {
            work->mode = BUYON_WANDER;
            VARS(work)->animation.action = &buyon_part_4_wander;
            VARS(work)->frame = 0.0F;
        }
        break;
    case BUYON_INACTIVE:
        break;
    }
    VARS(parent)->frame = VARS(work)->frame;
}

void buyon_step(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = (struct lockon_target *)task->motion;
    Angle angle;
    if (placement_try_leave_by(task, BUYON_RANGE)) {
        return;
    }
    if (edit_mode) {
        /* in the set editor, the buyon draws parts 4 and 0 unposed, and does
         * nothing more */
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, angle);
        njSetTexture(&buyon_texlist);
        /* @hack spelling: part 4's object read as
         * (&buyon_part_4_wander)->object, which loads the action's address into
         * R4, draw_tree's argument (0x0c90a0c6). as buyon_part_4_wander.object,
         * R2 takes the address, and 2 of 605 words differ */
        draw_tree((&buyon_part_4_wander)->object);
        njTranslate(NULL, 0.0F, 4.7656F, 0.0F);
        /* @hack spelling: part 0's object read the same way, which loads
         * draw_tree's address into R3 and the action's into R4 (0x0c90a0d8,
         * 0x0c90a0da). as buyon_part_0_wander.object, these and njPopMatrix's
         * address take other registers, and 6 of 605 words differ */
        draw_tree((&buyon_part_0_wander)->object);
        njPopMatrix(1);
        return;
    }
    /* @hack redundant: the else after the return, which loads the address the
     * INIT, TURN, GROW, CHASE and SHRINK cases each call first into R1
     * (0x0c90a1e8, 0x0c90a400, 0x0c90a47c, 0x0c90a520, 0x0c90a5a2); without
     * it, R2 takes them and 10 of 605 words differ */
    else {
        /* each case below that calls buyon_push_player may then set a mode
         * of its own. that mode takes the place of the burst the push
         * started */
        switch (work->mode) {
        case BUYON_INIT:
            work->mode = BUYON_WANDER;
            RATE(work) = 1.0F;
            collision_init(task, buyon_shapes, COLLISION_ENEMY, 1,
                           BUYON_SHAPES);
            work->collision->radius = 30.0F;
            work->collision->shapes[0].flags |= SHAPE_OFF;
            work->collision->shapes[2].flags |= SHAPE_OFF;
            work->collision->shapes[3].flags |= SHAPE_OFF;
            task->destructor = buyon_destroy;
            task_create_children(buyon_parts, task);
            record = enemy_alloc_record(task, work);
            record->sight_angle = NJM_DEG_ANG(30);
            record->sight_distance = SIGHT_RANGE;
            record->near_distance = NEAR_RANGE;
            record->home = work->pos;
            record->last_pos = work->pos;
            record->turn_speed = NJM_DEG_ANG(1);
            SPEED(record) = 0.0F;
            record->flags |= ENEMY_SHADOW;
            record->probe_offset.x = 0.0F;
            record->probe_offset.y = 4.7656F;
            record->probe_offset.z = 0.0F;
            record->top = 10.0F;
            record->radius = 5.0F;
            record->bottom = -4.7656F;
            record->step_down = 31.8415F;
            record->wall_bounce = 0.5F;
            record->floor_bounce = 0.8F;
            record->unk_c4 = 0.0F;
            record->wall_turn = NJM_DEG_ANG(1);
            record->shadow_size = 1.0F;
            record->shadow_ratio = 0.8F;
            buyon_choose_target(work, record);
            work->ang.y = record->aim_y;
            break;
        case BUYON_WANDER:
            buyon_move(task);
            buyon_resolve_contact(task);
            buyon_update_collision(task);
            buyon_push_player(task);
            if (buyon_player_near(task)) {
                work->mode = BUYON_GROW;
                SPEED(record) = 0.0F;
                TASK_TIMER(work) = 0;
                work->collision->shapes[2].flags &= ~SHAPE_OFF;
                work->collision->shapes[3].flags &= ~SHAPE_OFF;
                work->collision->radius = 50.0F;
                record->probe_offset.y = 7.57F;
                record->top = 37.0F;
                record->radius = GROWN_RADIUS;
                record->bottom = -7.57F;
                record->shadow_size = 1.0F;
            }
            task_step_children(task);
            collision_register(work);
            break;
        case BUYON_TURN:
            buyon_move(task);
            buyon_resolve_contact(task);
            buyon_update_collision(task);
            buyon_push_player(task);
            if (buyon_player_near(task)) {
                work->mode = BUYON_GROW;
                SPEED(record) = 0.0F;
                TASK_TIMER(work) = 0;
                work->collision->shapes[2].flags &= ~SHAPE_OFF;
                work->collision->shapes[3].flags &= ~SHAPE_OFF;
                work->collision->radius = 50.0F;
                record->probe_offset.y = 7.57F;
                record->top = 37.0F;
                record->radius = GROWN_RADIUS;
                record->bottom = -7.57F;
                record->shadow_size = 1.0F;
            }
            task_step_children(task);
            collision_register(work);
            break;
        case BUYON_GROW:
            buyon_resolve_contact(task);
            buyon_update_collision(task);
            buyon_push_player(task);
            if (++TASK_TIMER(work) > GROW_STEPS) {
                work->mode = BUYON_CHASE;
                TASK_TIMER(work) = 0;
                RATE(work) = 1.5F;
                VARS(work)->footstep = 0;
            }
            task_step_children(task);
            collision_register(work);

            sound_play_at_for(SE_GROW, (int)work, 1, 0, work->pos.x,
                              work->pos.y, work->pos.z, 2);
            break;
        case BUYON_CHASE:
            buyon_move(task);
            buyon_resolve_contact(task);
            buyon_update_collision(task);
            buyon_push_player(task);
            if (++TASK_TIMER(work) > CHASE_STEPS) {
                work->mode = BUYON_SHRINK;
                TASK_TIMER(work) = 0;
                RATE(work) = 1.0F;
                record->motion.speed.x = 0.0F;
                work->collision->shapes[2].flags |= SHAPE_OFF;
                work->collision->shapes[3].flags |= SHAPE_OFF;
                work->collision->radius = 30.0F;
                record->probe_offset.y = 4.7656F;
                record->top = 10.0F;
                record->radius = 5.0F;
                record->bottom = -4.7656F;
                record->shadow_size = 1.0F;
            }
            task_step_children(task);
            collision_register(work);
            break;
        case BUYON_SHRINK:
            buyon_resolve_contact(task);
            buyon_update_collision(task);
            buyon_push_player(task);
            if (++TASK_TIMER(work) > SHRINK_STEPS) {
                work->mode = BUYON_WANDER;
                TASK_TIMER(work) = 0;
                buyon_choose_target(work, record);
            }
            task_step_children(task);
            collision_register(task->work);
            sound_play_at_for(SE_SHRINK, (int)work, 1, 0, work->pos.x,
                              work->pos.y, work->pos.z, 2);
            break;
        case BUYON_BURST:
            if (work->submode == BUYON_CHASE) {
                explosion_create_at(work->pos.x, work->pos.y + 30.0F,
                                    work->pos.z, 1.4F);
                animal_create_random(15, work->pos.x, work->pos.y + 30.0F,
                                     work->pos.z);
            } else {
                animal_create_random(15, work->pos.x, work->pos.y + 15.0F,
                                     work->pos.z);
            }
            explosion_create_at(work->pos.x, work->pos.y + 5.0F, work->pos.z,
                                1.4F);
            enemy_scatter_debris(work, DEBRIS_COUNT);
            score_add_bonus(100);
            buyon_destroy(task);
            placement_kill(task);
            break;
        case BUYON_INACTIVE:
            break;
        }
    }
    record->last_pos = work->pos;
}
