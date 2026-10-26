/* E GACHA robots that walk toward player 0 and burst, and COLLI GACHA walls
 * that stop them. gacha_robot_list_step ranks the robots and stops them at
 * the walls */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/sprite.h"
#include "draw/tree.h"
#include "effect/blast_dome.h"
#include "effect/dust.h"
#include "effect/explosion.h"
#include "effect/hit.h"
#include "object/models.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg10/gacha_robot.h"
#include "stg10/gacha_robot_walk.h"
#include "stg10/puff.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the robot in less detail, as drawn from MID_DISTANCE and FAR_DISTANCE
 * away */
extern NJS_OBJECT gacha_robot_mid_object;
extern NJS_OBJECT gacha_robot_far_object;
/* the models of gacha_robot_mid_object's body and two sides, which a burst
 * throws as pieces */
extern NJS_MODEL gacha_robot_body_model;
extern NJS_MODEL gacha_robot_side_a_model;
extern NJS_MODEL gacha_robot_side_b_model;

/* the robots play SE_ROBOT_WALK each step of ROBOT_WAIT and ROBOT_CHASE, all
 * under the one id WALK_ID. sound_play_for then keeps one slot playing it
 * while any robot walks */
#define SE_ROBOT_WALK 981
#define WALK_ID 119
#define SE_ROBOT_FUSE 295
#define SE_ROBOT_BURST 25

/* the most robots and walls the lists hold. any past the most go unlisted
 * that step */
#define ROBOTS 100
#define WALLS 16

/* the frames of a stride, and of gacha_robot_walk_motion, which a robot plays
 * as two strides */
#define STRIDE_FRAMES 8.0F
#define WALK_FRAMES 16.0F

/* the share of its speed a robot loses each step it moves. in ROBOT_WAIT and
 * ROBOT_CHASE, it loses WALK_DRAG of its x and z speed as well */
#define DRAG 0.025F
#define WALK_DRAG 0.25F

/* a waiting robot rests this many steps between its slow strides */
#define WAIT_STEPS 30

/* within PACE_NEAR of player 0, a robot paces its strides by its rank. the
 * robots past the PACE_RANKS nearest stride slowest */
#define PACE_NEAR 100.0F
#define PACE_RANKS 10

/* player 0 is ahead of a robot while the robot's heading lies within SIGHT
 * of player 0's direction. a robot chases player 0 ahead of it within
 * CHASE_NEAR */
#define SIGHT NJM_DEG_ANG(45)
#define CHASE_NEAR 200.0F

/* a robot among the FUSE_RANKS nearest counts the steps player 0 is ahead of
 * it and within FUSE_NEAR. work->tick holds the count. a step with player 0
 * within FUSE_NEAR but not ahead clears the count. the robot lights its fuse
 * once the count passes FUSE_DELAY. the fuse then burns FUSE_STEPS steps */
#define FUSE_RANKS 2
#define FUSE_NEAR 30.0F
#define FUSE_DELAY 15
#define FUSE_STEPS 40

/* a burst's ball sets off a robot within CHAIN_NEAR of the ball's center.
 * that robot bursts CHAIN_STEPS steps later. the ball throws a robot that is
 * farther, but within THROW_NEAR */
#define CHAIN_NEAR 20.0F
#define CHAIN_STEPS 15
#define THROW_NEAR 30.0F

/* the robot loses detail from these distances to the camera */
#define MID_DISTANCE 130.0F
#define FAR_DISTANCE 260.0F

/* the half dome a burst blows. its BANDS bands span DOME_ARC from its top
 * down to its rim, and each has SIDES sides */
#define BANDS 4
#define SIDES 8
#define DOME_ARC NJM_DEG_ANG(90)
#define BAND_ANGLE (DOME_ARC / BANDS)
#define SIDE_ANGLE (NJM_DEG_ANG(360) / SIDES)
/* the points in a band's strip: a top and a bottom one for each side. the
 * first two then repeat to close the strip */
#define STRIP_LENGTH ((SIDES + 1) * 2)

/* a burst lasts BLAST_STEPS steps, and its dome fades over the last
 * FADE_STEPS */
#define BLAST_STEPS 60
#define FADE_STEPS 30

/* the units of a wall's size for each unit of its scale. its grid's squares
 * are this wide */
#define WALL_UNIT 10.0F
/* the editor's colors for a wall's grid and its arrow, blue and green */
#define GRID_COLOR 0xff0000ff
#define ARROW_COLOR 0xff00ff00

/* in a robot's work->flags */
#define ROBOT_GROUNDED 0x1 /* it stands on its floor */
#define ROBOT_STRIDING 0x200 /* it is partway through a stride */
/* the last wall gacha_robot_stop_at_walls tested stopped the robot. this file
 * sets the flag and never reads it */
#define ROBOT_AT_WALL 0x1000

enum gacha_robot_mode {
    ROBOT_INIT = 0,
    /* it rests WAIT_STEPS steps between slow strides toward player 0 */
    ROBOT_WAIT = 1,
    ROBOT_CHASE = 2, /* its rank among the robots paces its strides */
    ROBOT_THROWN = 4, /* a burst threw it, and it hasn't landed yet */
    ROBOT_FALL = 5, /* it falls to its floor */
    ROBOT_LAND = 6, /* it landed, and wobbles a while */
    ROBOT_HIT = 7, /* a player hit it. it wobbles, then explodes */
    ROBOT_FUSE = 8, /* its fuse is lit, and it sparks */
    ROBOT_BURST = 9, /* it bursts once its countdown runs out */
    ROBOT_GONE = 11
};

enum gacha_blast_mode { BLAST_INIT, BLAST_GROW, BLAST_DONE };

enum gacha_list_mode { LIST_INIT, LIST_ON };

enum gacha_wall_mode { WALL_INIT, WALL_ON };

enum gacha_piece_mode { PIECE_INIT, PIECE_BOUNCE, PIECE_REST, PIECE_DONE };

/* a robot's motion record, which gacha_robot_init allocates at task->motion
 * in place of a struct task_motion */
struct gacha_robot_motion {
    NJS_VECTOR speed;
    NJS_VECTOR accel; /* gacha_robot_move adds it to speed, then clears it */
    /* the angles it turns toward: its heading in y, and 0 in x and z, which
     * levels it */
    struct angles aim;
    struct angles turn; /* the most each angle turns toward aim in a step */
    Uint8 unk_30[4];
    Float ground; /* the height of its floor */
    Uint8 unk_38[4];
    Float frame; /* the frame of gacha_robot_walk_motion the robot shows */
    Float frame_speed; /* the walk's frames each step of a stride plays */
    Float thrust; /* its push along its heading this step, 0 between strides */
    Sint32 countdown; /* the steps left in ROBOT_FUSE or ROBOT_BURST */
    Angle player_ang; /* the heading toward player 0 */
    Angle player_off; /* the size of the turn from its heading to player_ang */
    /* its distance from player 0, and its rank by that distance, 0 for the
     * nearest. gacha_robot_list_step sets both */
    Float player_distance;
    Sint32 rank;
};

struct gacha_robot_vars {
    Uint8 unk_00[4];
    Sint32 wait; /* the steps before its next stride */
};

#define ROBOT(work) ((struct gacha_robot_vars *)&(work)->vars)
/* the speed at which a wobbling robot's height scale springs toward 1 */
#define WOBBLE(work) ((work)->scl.z)

struct gacha_blast_vars {
    Uint8 unk_00[4];
    Sint32 scroll; /* how far the dome's texture has scrolled */
    Float growth; /* the dome's growth this step */
};

#define BLAST(work) ((struct gacha_blast_vars *)&(work)->vars)
/* the dome's opacity, from 1 down, which also sets how much of each band
 * shows */
#define FADE(work) ((work)->scl.z)

/* a wall, as gacha_wall_step makes it at its first step. it keeps that
 * place */
struct gacha_wall {
    Angle ang;
    Float half_width;
    Float height;
    NJS_POINT3 pos;
    /* the corners of a box round the wall, a unit past it each way */
    NJS_POINT3 min;
    NJS_POINT3 max;
};

struct gacha_wall_vars {
    Uint8 unk_00[8];
    struct gacha_wall *wall;
};

#define WALL(work) ((struct gacha_wall_vars *)&(work)->vars)

/* a piece of a burst robot. work->tick counts its bounces */
struct gacha_piece_vars {
    Uint8 unk_00[8];
    NJS_MODEL *model;
};

#define PIECE(work) ((struct gacha_piece_vars *)&(work)->vars)

/* a piece's struct task_motion, laid out as the piece uses it */
struct gacha_piece_motion {
    Uint8 unk_00[4];
    NJS_VECTOR speed;
    NJS_VECTOR accel; /* gacha_piece_step adds it to speed, then clears it */
    Uint8 unk_1c[12];
    Angle spin_x; /* its turn about x each step */
    Uint8 unk_2c[4];
    Angle spin_z; /* about z */
    Uint8 unk_34[4];
    Float ground; /* the height it started at, where it bounces */
};

/* a robot in the list, and its distance from player 0 */
struct gacha_robot_entry {
    struct task *task;
    Float distance;
};

static void gacha_robot_decide(struct task *task, struct task_work *work,
                               struct gacha_robot_motion *motion);
static void gacha_robot_sense(struct task_work *work,
                              struct gacha_robot_motion *motion);
static Bool gacha_robot_start_fall(struct task_work *work,
                                   struct gacha_robot_motion *motion);
static void gacha_robot_animate(struct task_work *work,
                                struct gacha_robot_motion *motion);
static void gacha_robot_drive(struct task_work *work,
                              struct gacha_robot_motion *motion);
static void gacha_robot_level(struct task_work *work,
                              struct gacha_robot_motion *motion);
static void gacha_robot_move(struct task_work *work,
                             struct gacha_robot_motion *motion);
static void gacha_robot_burst(struct task_work *work,
                              struct gacha_robot_motion *motion);
static void gacha_robot_wobble(struct task_work *work,
                               struct gacha_robot_motion *motion);
static Angle gacha_robot_player_ang(struct task_work *work,
                                    struct gacha_robot_motion *motion);
static void gacha_robot_turn(struct task_work *work,
                             struct gacha_robot_motion *motion);
static Bool gacha_robot_take_hit(struct task_work *work,
                                 struct gacha_robot_motion *motion);
static Bool gacha_robot_take_blast(struct task_work *work,
                                   struct gacha_robot_motion *motion);
static Bool gacha_robot_light_fuse(struct task_work *work,
                                   struct gacha_robot_motion *motion);
static Bool gacha_robot_start_chase(struct task_work *work,
                                    struct gacha_robot_motion *motion);
static void gacha_robot_pace(struct task_work *work,
                             struct gacha_robot_motion *motion);
static void gacha_robot_draw_spark(struct task_work *work,
                                   struct gacha_robot_motion *motion);
static void gacha_robot_puff(struct task_work *work,
                             struct gacha_robot_motion *motion);
static void gacha_robot_destroy(struct task *task);
static void gacha_robot_draw(struct task *task);
static void gacha_robot_draw_at(struct task_work *work,
                                struct gacha_robot_motion *motion);
static void gacha_dome_init(void);
static void gacha_dome_scroll(int scroll);
static void gacha_dome_blend(Float blend);
static void gacha_blast_draw(struct task *task);
static void gacha_blast_step(struct task *task);
static void gacha_robot_flash(struct task_work *work,
                              struct gacha_robot_motion *motion);
static void gacha_robot_enlist(struct task *task);
static void gacha_wall_enlist(struct task *task);
static void gacha_robot_rank(void);
static void gacha_robot_stop_at_walls(void);
static void gacha_wall_destroy(struct task *task);
static int gacha_wall_stop(struct task *robot, struct task *wall);
static void gacha_wall_draw_grid(struct task_work *work);
static void gacha_piece_draw(struct task *task);
static void gacha_piece_step(struct task *task);
static void gacha_robot_break(struct task_work *work);

/* the cos and sin of each side's angle */
static Float gacha_dome_sides[SIDES * 2];
/* each band's points, a top and a bottom one for each side */
static NJS_POINT3 gacha_dome_points[BANDS * SIDES * 2];
/* the points' normals, which stay zero */
static NJS_VECTOR gacha_dome_normals[BANDS * SIDES * 2];
/* each band's strip: its length, then its points' indexes. gacha_dome_uvs
 * holds each strip point's place in the texture */
static Sint16 gacha_dome_strips[BANDS * (1 + STRIP_LENGTH)];
static NJS_COLOR gacha_dome_uvs[BANDS * STRIP_LENGTH];
/* the walls, and the robots with their distances, enlisted since
 * gacha_robot_list_step last ran */
static struct task *gacha_walls[WALLS];
static struct gacha_robot_entry gacha_robots[ROBOTS];
/* the two ends of the line gacha_wall_draw_grid draws */
static NJS_POINT3 gacha_wall_line_points[2];

/* a robot's two shapes. the first, a cylinder, pushes other enemies, and
 * players and attacks pass through it. a burst's ball hurts the cylinder. the
 * second, a sphere, pushes players, and they can't push it. players hurt the
 * sphere. at attack 0, neither shape hurts what it touches */
static struct collision_shape gacha_robot_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_UNTURNED | SHAPE_IGNORE_PLAYERS,
     SHAPE_HURTS_PLAYERS,
     PUSH_ENEMIES,
     {0.0F, 5.0F, 0.0F},
     10.0F,
     5.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_UNTURNED | SHAPE_IGNORE_ENEMIES,
     SHAPE_HURTS_PLAYERS,
     PUSH_PLAYERS | PUSH_FIRM(PUSH_PLAYERS),
     {0.0F, 5.0F, 0.0F},
     10.0F,
     5.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* the four puffs a robot raises as it lands from a fall */
static struct dust_params gacha_robot_puff_params = {
    2.0F,
    0.038F,
    0.19F,
    0.06F,
    0.005F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.5F, 1.0F, 1.0F, 1.0F},
};

/* the spark a robot shows in ROBOT_FUSE. gacha_robot_draw_spark moves it to
 * the robot before each draw */
static NJS_SPRITE gacha_robot_spark = {
#include "stg10/gacha_robot/gacha_robot_spark.inc"
};

/* the burst's ball, which grows with the dome. it pushes no shape, and no
 * shape pushes it */
static struct collision_shape gacha_blast_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_UNTURNED,
    ATTACK(2) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    10.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static NJS_MESHSET gacha_dome_meshset = {
#include "stg10/gacha_robot/gacha_dome_meshset.inc"
};
static NJS_MATERIAL gacha_dome_material = {
#include "stg10/gacha_robot/gacha_dome_material.inc"
};
static NJS_MODEL gacha_dome_model = {
#include "stg10/gacha_robot/gacha_dome_model.inc"
};

static Bool gacha_dome_made = FALSE;

/* a burst's large flashes, and its small ones, which fly faster.
 * hit_create_from keeps a pointer to the record each flash is made from */
static struct dust_params gacha_robot_flash_large = {
    1.0F,
    0.1F,
    0.5F,
    0.065F,
    0.0F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};
static struct dust_params gacha_robot_flash_small = {
    0.2F,
    0.01F,
    0.2F,
    0.065F,
    0.0F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

static int gacha_robot_count = 0;
static int gacha_wall_count = 0;

/* the colors of the line's two ends. gacha_wall_draw_grid sets both to
 * GRID_COLOR for the grid, then to ARROW_COLOR for the arrow */
static NJS_COLOR gacha_wall_line_colors[2] = {
#include "stg10/gacha_robot/gacha_wall_line_colors.inc"
};
static NJS_POINT3COL gacha_wall_line = {
#include "stg10/gacha_robot/gacha_wall_line.inc"
};

/* what a burst robot breaks into */
static NJS_MODEL *gacha_robot_pieces[3] = {&gacha_robot_side_a_model,
                                           &gacha_robot_side_b_model,
                                           &gacha_robot_body_model};

/* allocate the robot's motion record and set the robot up, unless the task
 * already has the record. a robot the set editor holds gets no collision
 * record */
static void gacha_robot_init(struct task *task)
{
    struct task_work *work = task->work;
    struct gacha_robot_motion *motion;

    if (!task->motion) {
        motion = heap_calloc(1, sizeof(struct gacha_robot_motion));
        task->motion = (struct task_motion *)motion;
        work->tick = 0;
        work->timer = 0;
        work->scl.x = 1.0F;
        work->scl.y = 1.0F;
        WOBBLE(work) = 0.0F;
        motion->speed.x = 0.0F;
        motion->speed.y = 0.0F;
        motion->speed.z = 0.0F;
        motion->accel.x = 0.0F;
        motion->accel.y = 0.0F;
        motion->accel.z = 0.0F;
        motion->turn.x = 16;
        motion->turn.z = 16;
        motion->turn.y = 768;
        motion->aim.y = work->ang.y;
        motion->frame = 0.0F;
        motion->frame_speed = 0.0F;
        motion->player_ang = 0;
        motion->ground = shadow_draw(work, 1.0F);
        task->destructor = gacha_robot_destroy;
        task->draw = gacha_robot_draw;
        if (!placement_editing(task)) {
            collision_init(task, gacha_robot_shapes, COLLISION_ENEMY, 1, 2);
            work->collision->radius = 15.0F;
        }
    }
}

/* add a step's gravity to the robot's accel */
/* @hack spelling: the add in a helper SHC expands at each call, which keeps
 * ROBOT_THROWN's code apart from ROBOT_FALL's, as the image has them
 * (0x0c90ceac, 0x0c90cec6). with the add written out in both, SHC merges
 * them: the object's code runs 32 bytes short and 117 of gacha_robot_step's
 * 214 words differ */
static void gacha_robot_gravity(struct gacha_robot_motion *motion)
{
    motion->accel.y += -0.08F;
}

/* in the set editor, a robot past its first step only draws */
void gacha_robot_step(struct task *task)
{
    struct task_work *work = task->work;
    struct gacha_robot_motion *motion =
        (struct gacha_robot_motion *)task->motion;

    if (task->placement && placement_try_leave(task)) {
        return;
    }
    if (edit_mode && work->mode) {
        gacha_robot_draw_at(work, motion);
        return;
    }
    gacha_robot_decide(task, work, motion);
    switch (work->mode) {
    case ROBOT_INIT:
        gacha_robot_init(task);
        motion = (struct gacha_robot_motion *)task->motion;
        work->mode = ROBOT_CHASE;
        break;
    case ROBOT_WAIT:
        if (work->flags & ROBOT_STRIDING) {
            gacha_robot_turn(work, motion);
        } else {
            /* @hack spelling: copy, a copy of motion, which SHC keeps in R4
             * for the store through it (MOV R13,R4 at 0x0c90ce3c); without
             * it, 23 of 214 words differ */
            struct gacha_robot_motion *copy = motion;

            copy->aim.y = copy->player_ang;
        }
        gacha_robot_animate(work, motion);
        gacha_robot_drive(work, motion);
        gacha_robot_move(work, motion);
        sound_play_for(SE_ROBOT_WALK, WALK_ID, 1, 0, 2);
        break;
    case ROBOT_CHASE:
        if (work->flags & ROBOT_STRIDING) {
            gacha_robot_turn(work, motion);
        } else {
            /* @hack spelling: copy, a copy of motion, which SHC keeps in R4
             * for the store through it (MOV R13,R4 at 0x0c90ce82); without
             * it, 54 of 214 words differ */
            struct gacha_robot_motion *copy = motion;

            copy->aim.y = copy->player_ang;
        }
        gacha_robot_animate(work, motion);
        gacha_robot_drive(work, motion);
        gacha_robot_move(work, motion);
        sound_play_for(SE_ROBOT_WALK, WALK_ID, 1, 0, 2);
        break;
    case ROBOT_THROWN:
        gacha_robot_level(work, motion);
        gacha_robot_gravity(motion);
        gacha_robot_move(work, motion);
        break;
    case ROBOT_FALL:
        gacha_robot_level(work, motion);
        gacha_robot_gravity(motion);
        gacha_robot_move(work, motion);
        break;
    case ROBOT_LAND:
        gacha_robot_wobble(work, motion);
        gacha_robot_move(work, motion);
        break;
    case ROBOT_FUSE:
        gacha_robot_animate(work, motion);
        gacha_robot_draw_spark(work, motion);
        gacha_robot_move(work, motion);
        break;
    case ROBOT_BURST:
        gacha_robot_move(work, motion);
        break;
    case ROBOT_HIT:
        gacha_robot_wobble(work, motion);
        gacha_robot_move(work, motion);
        break;
    case ROBOT_GONE:
        if (task->placement) {
            placement_kill(task);
        } else {
            task_kill(task);
        }
        return;
    }
    gacha_robot_sense(work, motion);
    gacha_robot_draw_at(work, motion);
    gacha_robot_enlist(task);
    if (!placement_editing(task)) {
        if (work->mode != ROBOT_GONE && work->mode != ROBOT_HIT) {
            collision_register(work);
        }
    }
}

/* change the robot's mode by what hit it, its footing and where player 0
 * is */
static void gacha_robot_decide(struct task *task, struct task_work *work,
                               struct gacha_robot_motion *motion)
{
    NJS_POINT3 between;

    if (gacha_robot_take_hit(work, motion)) {
        /* the midpoint of the robot and player 0, for the flash. hit_create
         * ignores pos, so the flash isn't placed there */
        between = work->pos;
        njAddVector(&between, &player_work[0]->pos);
        between.x *= 0.5F;
        between.y *= 0.5F;
        between.z *= 0.5F;
        hit_create(&between, 0.5F);
        WOBBLE(work) = 0.35F;
        player_set_speed(0, 0.0F, 1.2F, 0.0F);
    }
    switch (work->mode) {
    case ROBOT_WAIT:
        /* @hack spelling: the goto past the wait, which loads the wait into R3
         * (MOV.L @(12,R14),R3 at 0x0c90d106); as an if round the wait, it
         * takes R2 and 2 of 317 words differ */
        if (work->flags & ROBOT_STRIDING) {
            goto wait_checks;
        }
        if (ROBOT(work)->wait == 0) {
            ROBOT(work)->wait = WAIT_STEPS;
            motion->frame_speed = 0.5F;
            motion->turn.y = 256;
            work->flags |= ROBOT_STRIDING;
        } else {
            ROBOT(work)->wait--;
        }
    wait_checks:
        if (gacha_robot_take_blast(work, motion)) {
            break;
        }
        if (gacha_robot_start_fall(work, motion)) {
            break;
        }
        if (gacha_robot_light_fuse(work, motion)) {
            break;
        }
        if (gacha_robot_start_chase(work, motion)) {
            break;
        }
        break;
    case ROBOT_CHASE:
        /* @hack spelling: the goto past the wait, which loads the wait into R3
         * (MOV.L @(12,R14),R3 at 0x0c90d166); as an if round the wait, it
         * takes R2 and 2 of 317 words differ */
        if (work->flags & ROBOT_STRIDING) {
            goto chase_checks;
        }
        if (ROBOT(work)->wait == 0) {
            gacha_robot_pace(work, motion);
            work->flags |= ROBOT_STRIDING;
        } else {
            ROBOT(work)->wait--;
        }
    chase_checks:
        if (gacha_robot_take_blast(work, motion)) {
            break;
        }
        if (gacha_robot_start_fall(work, motion)) {
            break;
        }
        if (gacha_robot_light_fuse(work, motion)) {
            break;
        }
        if (gacha_robot_start_chase(work, motion)) {
            break;
        }
        work->mode = ROBOT_WAIT;
        break;
    case ROBOT_THROWN:
        if (work->flags & ROBOT_GROUNDED) {
            work->mode = ROBOT_LAND;
            work->timer = 30;
            WOBBLE(work) = 0.18F;
            work->ang.x = 0;
            work->ang.z = 0;
            motion->aim.x = 0;
            motion->aim.z = 0;
        }
        break;
    case ROBOT_FALL:
        if (work->flags & ROBOT_GROUNDED) {
            work->mode = ROBOT_LAND;
            work->timer = 15;
            WOBBLE(work) = 0.18F;
            work->ang.x = 0;
            work->ang.z = 0;
            motion->aim.x = 0;
            motion->aim.z = 0;
            gacha_robot_puff(work, motion);
        }
        break;
    case ROBOT_LAND:
        if (gacha_robot_take_blast(work, motion)) {
            break;
        }
        if (gacha_robot_start_fall(work, motion)) {
            break;
        }
        if (work->timer == 0) {
            work->mode = ROBOT_WAIT;
        } else {
            work->timer--;
        }
        break;
    case ROBOT_FUSE:
        if (motion->countdown == 0) {
            work->mode = ROBOT_BURST;
        } else {
            motion->countdown--;
            if (gacha_robot_take_blast(work, motion)) {
                break;
            }
        }
        break;
    case ROBOT_BURST:
        if (motion->countdown == 0) {
            if (task->placement) {
                placement_stop(task);
            }
            gacha_robot_burst(work, motion);
            work->mode = ROBOT_GONE;
        } else {
            motion->countdown--;
        }
        break;
    case ROBOT_HIT:
        if (work->timer == 0) {
            motion->countdown = 0;
            explosion_create_at(work->pos.x, work->pos.y + 5.0F, work->pos.z,
                                1.4F);
            work->mode = ROBOT_GONE;
        } else {
            work->timer--;
        }
        break;
    }
}

/* land the robot on its floor, and find the heading toward player 0 */
static void gacha_robot_sense(struct task_work *work,
                              struct gacha_robot_motion *motion)
{
    work->flags &= ~ROBOT_GROUNDED;
    if (work->pos.y < motion->ground + 0.1F) {
        work->pos.y = motion->ground;
        work->flags |= ROBOT_GROUNDED;
    }
    motion->player_ang = gacha_robot_player_ang(work, motion);
    motion->player_off = angle_distance(motion->player_ang, work->ang.y);
}

/* enter ROBOT_FALL, and return TRUE, when the robot isn't on its floor */
static Bool gacha_robot_start_fall(struct task_work *work,
                                   struct gacha_robot_motion *motion)
{
    if (!(work->flags & ROBOT_GROUNDED)) {
        work->mode = ROBOT_FALL;
        return TRUE;
    }
    return FALSE;
}

/* advance the walk while the robot strides. one stride ends at
 * STRIDE_FRAMES, and the next at WALK_FRAMES, which wraps to 0 */
static void gacha_robot_animate(struct task_work *work,
                                struct gacha_robot_motion *motion)
{
    Float frame = motion->frame;
    Float speed = motion->frame_speed;

    if (work->flags & ROBOT_STRIDING) {
        if (frame < STRIDE_FRAMES) {
            frame += speed;
            if (frame >= STRIDE_FRAMES) {
                work->flags &= ~ROBOT_STRIDING;
            }
        } else {
            frame += speed;
            if (frame >= WALK_FRAMES) {
                frame -= WALK_FRAMES;
                work->flags &= ~ROBOT_STRIDING;
            }
        }
    }
    motion->frame = frame;
}

/* add to the robot's accel its thrust along its heading, which is 0 between
 * strides. each call also takes WALK_DRAG of the robot's x and z speed from
 * the accel */
static void gacha_robot_drive(struct task_work *work,
                              struct gacha_robot_motion *motion)
{
    Angle ang;
    Float *speed;
    Float *accel;
    Float x;
    Float z;
    Float ax;
    Float ay;
    Float az;
    Float thrust;

    motion->thrust = 0.0F;
    if (work->flags & ROBOT_STRIDING) {
        motion->thrust = motion->frame_speed * 0.2F;
    }
    ang = work->ang.y;
    /* @hack spelling: the speed read, and the accel read and written, through
     * Float cursors, which take the accel with @R14+ and store it with @-R14
     * (0x0c90d442, 0x0c90d474); through the fields, the function runs long
     * and 61 of 70 words differ */
    /* @hack spelling: each step of accel a statement of its own, two of them
     * written as accel = accel + 1. they put the speed's x at R15+4, its z at
     * R15 and the accel's y at R15+8 (0x0c90d438 to 0x0c90d446); folded into
     * the reads and stores, the slots change and 11 of 70 words differ */
    speed = &motion->speed.x;
    x = *speed++;
    accel = &motion->accel.x;
    speed++;
    z = *speed++;
    ax = *accel;
    accel = accel + 1;
    ay = *accel;
    accel = accel + 1;
    az = *accel;
    accel++;
    thrust = motion->thrust;
    ax += thrust * njCos(ang);
    az += -(thrust * njSin(ang));
    ax -= x * WALK_DRAG;
    /* @hack spelling: az's sum written out, which keeps ax in FR14 and az in
     * FR15 (FMAC FR0,FR13,FR14 at 0x0c90d452); as az -= z * WALK_DRAG, the
     * two trade and 18 of 70 words differ */
    az = az - (z * WALK_DRAG);
    accel--;
    *accel = az;
    accel--;
    *accel = ay;
    accel--;
    *accel = ax;
}

/* turn the robot's x and z angles toward aim's, which level it */
static void gacha_robot_level(struct task_work *work,
                              struct gacha_robot_motion *motion)
{
    work->ang.x = angle_approach(work->ang.x, motion->aim.x, motion->turn.x);
    work->ang.z = angle_approach(work->ang.z, motion->aim.z, motion->turn.z);
}

/* add the accel to the robot's speed, less DRAG of the speed, and clear the
 * accel. then move the robot by its speed */
static void gacha_robot_move(struct task_work *work,
                             struct gacha_robot_motion *motion)
{
    Float *accel = &motion->accel.x;
    Float *speed = &motion->speed.x;
    Float *pos;
    Float x;
    Float y;
    Float z;
    Float ax;
    Float ay;
    Float az;
    Float px;
    Float py;
    Float pz;

    /* @hack spelling: the speed, the accel and the position read and written
     * through Float cursors, which take the speed with @R5+ and the accel
     * with @R6+ (0x0c90d4e2, 0x0c90d4f2); through the fields, the function
     * runs long and 40 of 40 words differ */
    /* @hack spelling: each step of the cursors a statement of its own, which
     * loads DRAG into FR7 (FMOV.S @R0,FR7 at 0x0c90d4e6); folded into the
     * reads and stores, DRAG takes FR9 and 12 of 40 words differ */
    x = *speed;
    speed++;
    ax = *accel;
    accel++;
    y = *speed;
    speed++;
    ay = *accel;
    accel++;
    az = *accel;
    accel++;
    z = *speed;
    speed++;
    ax -= x * DRAG;
    x += ax;
    ay -= y * DRAG;
    y += ay;
    az -= z * DRAG;
    z += az;
    pos = &work->pos.x;
    speed--;
    *speed = z;
    speed--;
    *speed = y;
    speed--;
    *speed = x;
    accel--;
    *accel = 0.0F;
    accel--;
    *accel = 0.0F;
    accel--;
    *accel = 0.0F;
    px = *pos++;
    py = *pos++;
    pz = *pos++;
    px += x;
    py += y;
    pz += z;
    pos--;
    *pos = pz;
    pos--;
    *pos = py;
    pos--;
    *pos = px;
}

/* blow a dome at the robot, with flashes, and throw its pieces */
static void gacha_robot_burst(struct task_work *work,
                              struct gacha_robot_motion *motion)
{
    struct task *task = task_create(TASK_WORK, 3, gacha_blast_step);

    if (task) {
        struct task_work *blast = task->work;

        blast->pos = work->pos;
    }
    gacha_robot_flash(work, motion);
    gacha_robot_break(work);
    sound_play_at(SE_ROBOT_BURST, 0, 0, 0, work->pos.x, work->pos.y,
                  work->pos.z);
}

/* spring the robot's height scale, scl.y, toward 1 at the speed WOBBLE(work).
 * its width scale, scl.x, takes 2 less the height */
static void gacha_robot_wobble(struct task_work *work,
                               struct gacha_robot_motion *motion)
{
    Float *scl = &work->scl.y;
    Float y;
    Float z;
    Float x;

    /* @hack spelling: the scales read and written through scl, a Float cursor,
     * which loads them with @R4+ and stores them with @-R4 (0x0c90d586,
     * 0x0c90d5aa); through the fields, 24 of 25 words differ */
    y = *scl++;
    z = *scl++;
    z += -((y - 1.0F) * 0.32F) - (z * 0.12F);
    y += z;
    x = 2.0F - y;
    *--scl = z;
    *--scl = y;
    *--scl = x;
}

static Angle gacha_robot_player_ang(struct task_work *work,
                                    struct gacha_robot_motion *motion)
{
    struct task_work *player = player_work[0];

    return (Angle)(atan2f(player->pos.x - work->pos.x,
                          player->pos.z - work->pos.z) *
                   ANGLE_PER_RADIAN) -
           NJM_DEG_ANG(90);
}

static void gacha_robot_turn(struct task_work *work,
                             struct gacha_robot_motion *motion)
{
    work->ang.y = angle_approach(work->ang.y, motion->aim.y, motion->turn.y);
}

/* enter ROBOT_HIT, and return TRUE, when the robot is hurt and its ball
 * touched a player. a robot already hit or gone stays as it is */
static Bool gacha_robot_take_hit(struct task_work *work,
                                 struct gacha_robot_motion *motion)
{
    struct collision_work *hit;

    if ((work->flags & COLLISION_DAMAGED) && work->mode != ROBOT_GONE &&
        work->mode != ROBOT_HIT) {
        hit = work->collision->shapes[1].hit;
        if (hit && hit->kind == COLLISION_PLAYER) {
            work->mode = ROBOT_HIT;
            work->timer = 30;
            return TRUE;
        }
    }
    return FALSE;
}

/* set the robot off or throw it when it is hurt and its cylinder touched a
 * burst's ball. the rule beside CHAIN_NEAR picks which, and a thrown robot
 * flies away from the ball's center. return TRUE if either happens */
static Bool gacha_robot_take_blast(struct task_work *work,
                                   struct gacha_robot_motion *motion)
{
    struct collision_work *hit;
    struct task_work *blast;
    Float distance;
    Float push;
    NJS_VECTOR away;

    if (work->flags & COLLISION_DAMAGED) {
        hit = work->collision->shapes->hit;
        if (hit && hit->kind == COLLISION_OBJECT) {
            blast = hit->owner->work;
            distance = njDistanceP2P(&blast->pos, &work->pos);
            if (distance < CHAIN_NEAR) {
                motion->countdown = CHAIN_STEPS;
                work->mode = ROBOT_BURST;
                return TRUE;
            } else if (distance < THROW_NEAR) {
                away.x = work->pos.x - blast->pos.x;
                away.y = 0.0F;
                away.z = work->pos.z - blast->pos.z;
                njUnitVector(&away);
                push = RANDOM(1.0F, 2.0F);
                motion->accel.x += away.x * push;
                motion->accel.y += 1.2F + (rand() * RAND_SCALE);
                motion->accel.z += away.z * push;
                work->ang.z = (Angle)(rand() * 0.75F) - 0x3000;
                work->mode = ROBOT_THROWN;
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* count toward lighting the robot's fuse by the rule beside FUSE_DELAY, and
 * return TRUE once it lights. a lit robot stops where it stands */
static Bool gacha_robot_light_fuse(struct task_work *work,
                                   struct gacha_robot_motion *motion)
{
    if (motion->player_distance < FUSE_NEAR && motion->rank < FUSE_RANKS) {
        if (motion->player_off < SIGHT) {
            if (++work->tick > FUSE_DELAY) {
                motion->speed.x = 0.0F;
                motion->speed.z = 0.0F;
                motion->countdown = FUSE_STEPS;
                work->mode = ROBOT_FUSE;
                sound_play(SE_ROBOT_FUSE, 0, 0, 0);
                return TRUE;
            }
        } else {
            work->tick = 0;
        }
    }
    return FALSE;
}

/* enter ROBOT_CHASE, aimed at player 0, and return TRUE, while player 0 is
 * ahead and within CHASE_NEAR */
static Bool gacha_robot_start_chase(struct task_work *work,
                                    struct gacha_robot_motion *motion)
{
    if (motion->player_distance < CHASE_NEAR) {
        if (motion->player_off < SIGHT) {
            motion->aim.y = motion->player_ang;
            work->mode = ROBOT_CHASE;
            return TRUE;
        }
    }
    return FALSE;
}

/* set the next stride's wait, speed and turn by the robot's rank. the three
 * nearest robots stride fastest, with no wait, and those past the PACE_RANKS
 * nearest slowest. a robot farther than PACE_NEAR from player 0 ignores its
 * rank */
static void gacha_robot_pace(struct task_work *work,
                             struct gacha_robot_motion *motion)
{
    int wait;
    int rank = motion->rank;
    int turn;
    Float speed = 1.0F;

    if (motion->player_distance > PACE_NEAR) {
        wait = 5;
        turn = 512;
    } else if (rank < PACE_RANKS) {
        wait = 0;
        switch (rank) {
        case 0:
            speed = 2.4F;
            turn = 512;
            break;
        case 1:
            speed = 2.0F;
            turn = 592;
            break;
        case 2:
            speed = 1.8F;
            turn = 1024;
            break;
        default:
            wait = 5;
            turn = 1024;
            break;
        }
    } else {
        wait = 10;
        speed = 0.4F;
        turn = 512;
    }
    /* @hack redundant: two tests of wait that never run, which keep wait in R6
     * and turn in R7 (MOV.L R6,@(12,R4) at 0x0c90d944, MOV.L R7,@(40,R5) at
     * 0x0c90d94e); without them, wait and turn trade registers and 13 of 53
     * words differ */
    if (0) {
        if (wait) {
        }
        if (wait) {
        }
    }
    ROBOT(work)->wait = wait;
    motion->frame_speed = speed * 0.5F;
    motion->turn.y = turn;
}

static void gacha_robot_draw_spark(struct task_work *work,
                                   struct gacha_robot_motion *motion)
{
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    gacha_robot_spark.p.x = work->pos.x;
    /* @hack redundant: the stores of p.y and p.z inside a select on TRUE.
     * SHC then loads each field's own address from the pool (0x0c90d9b2,
     * 0x0c90d9be), as for p.x. as plain stores, they share one temporary of
     * the sprite's address, the function runs 8 bytes short, 13 of 64 words
     * differ, and 8 of its 18 pool words */
    TRUE ? (void)(gacha_robot_spark.p.y = work->pos.y + 5.0F,
                  gacha_robot_spark.p.z = work->pos.z)
         : (void)0;
    sprite_set_depth(-32.0F);
    sprite_draw_3d(&gacha_robot_spark, (motion->countdown >> 1) & 7,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR |
                       NJD_SPRITE_ANGLE);
    sprite_set_depth(0.0F);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* raise four puffs from the robot, a quarter turn apart from a random
 * angle */
static void gacha_robot_puff(struct task_work *work,
                             struct gacha_robot_motion *motion)
{
    int i;
    Angle ang = RANDOM_ANGLE();

    for (i = 0; i < 4; i++, ang += NJM_DEG_ANG(90)) {
        gacha_robot_puff_params.pos.x = work->pos.x;
        gacha_robot_puff_params.pos.y = work->pos.y + 5.0F;
        gacha_robot_puff_params.pos.z = work->pos.z;
        gacha_robot_puff_params.speed.x = njCos(ang) * 1.5F;
        gacha_robot_puff_params.speed.y = 0.0F;
        gacha_robot_puff_params.speed.z = njSin(ang) * 1.5F;
        final_puff_create(&gacha_robot_puff_params);
    }
}

/* free the robot's motion, and clear it so that task_destroy doesn't free it
 * again */
static void gacha_robot_destroy(struct task *task)
{
    if (task->motion) {
        heap_free(task->motion);
        task->motion = NULL;
    }
}

static void gacha_robot_draw(struct task *task)
{
    gacha_robot_draw_at(task->work, (struct gacha_robot_motion *)task->motion);
}

/* draw the robot, then its shadow on its floor. the robot loses detail from
 * MID_DISTANCE of the camera, and more from FAR_DISTANCE */
static void gacha_robot_draw_at(struct task_work *work,
                                struct gacha_robot_motion *motion)
{
    Float distance;
    Angle ang;
    NJS_OBJECT *object;

    if (frame_step == 0) {
        njSetTexture(&gacha_robot_texlist);
        njPushMatrix(NULL);
        distance = njDistanceP2P(&camera_work->pos, &work->pos);
        njTranslate(NULL, work->pos.x, work->pos.y + 3.0F, work->pos.z);
        TURN_VIA(njRotateY, work->ang.y + NJM_DEG_ANG(90), ang);
        TURN(njRotateZ, work->ang.z);
        njScale(NULL, work->scl.x * 1.5F, work->scl.y * 1.5F,
                work->scl.x * 1.5F);
        if (distance < MID_DISTANCE) {
            object = gacha_robot_object;
        } else if (distance < FAR_DISTANCE) {
            object = &gacha_robot_mid_object;
        } else {
            object = &gacha_robot_far_object;
        }
        draw_motion(object, &gacha_robot_walk_motion, motion->frame);
        njPopMatrix(1);
        texture_set_regular();
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, motion->ground + 0.5F, work->pos.z);
        njScale(NULL, 3.0F, 3.0F, 3.0F);
        njEasyDrawObject(shadow_object);
        njPopMatrix(1);
    }
}

/* make the dome's model, once */
static void gacha_dome_init(void)
{
    Float *side;
    Float *point;
    Sint16 *strip;
    Angle ang;
    Angle around;
    int band;
    int i;
    int first;
    int next;

    if (gacha_dome_made) {
        return;
    }
    gacha_dome_made = TRUE;
    side = gacha_dome_sides;
    for (ang = 0; ang < NJM_DEG_ANG(360); ang += SIDE_ANGLE) {
        *side++ = njCos(ang);
        *side++ = njSin(ang);
    }
    /* @hack spelling: the points stored through point, a Float cursor, each
     * with FMOV.S FRm,@R14 and ADD #4,R14 (0x0c90ddfa on); through NJS_POINT3's
     * fields, the function runs long and 125 of 183 words differ */
    point = &gacha_dome_points[0].x;
    for (ang = 0; ang < DOME_ARC; ang += BAND_ANGLE) {
        Float top_sin;
        Float top_cos;
        Float bottom_sin;
        Float bottom_cos;

        side = gacha_dome_sides;
        top_sin = njSin(ang);
        top_cos = njCos(ang);
        bottom_sin = njSin(ang + BAND_ANGLE);
        bottom_cos = njCos(ang + BAND_ANGLE);
        for (around = 0; around < NJM_DEG_ANG(360); around += SIDE_ANGLE) {
            Float x = *side++;
            Float z = *side++;

            *point++ = top_sin * x;
            *point++ = top_cos;
            *point++ = top_sin * z;
            *point++ = bottom_sin * x;
            *point++ = bottom_cos;
            *point++ = bottom_sin * z;
        }
    }
    strip = gacha_dome_strips;
    next = 0;
    for (band = 0; band < BANDS; band++) {
        first = next;
        *strip++ = STRIP_LENGTH;
        for (i = 0; i < SIDES; i++) {
            *strip++ = next++;
            *strip++ = next++;
        }
        *strip++ = first++;
        *strip++ = first;
    }
    gacha_dome_scroll(0);
}

/* set the dome's uvs for its texture scrolled down by scroll. the scroll
 * counts from 0, not from the last call's */
static void gacha_dome_scroll(int scroll)
{
    Sint16 *uv = &gacha_dome_uvs[0].tex.u;
    int band;
    int i;
    int left;
    int top = scroll;

    for (band = 0; band < BANDS; band++) {
        left = 0;
        for (i = 0; i < SIDES + 1; i++) {
            *uv++ = left;
            *uv++ = top;
            *uv++ = left;
            *uv++ = top + 63;
            left += 64;
            top += 64;
        }
    }
}

/* move each band's top ring toward its bottom ring, leaving blend of the
 * band's width. at 1 the band is whole */
static void gacha_dome_blend(Float blend)
{
    Float *side;
    Float *point;
    Angle ang;
    Angle around;

    /* @hack spelling: the top points stored through point, a Float cursor,
     * which steps past each bottom point with ADD #16,R14 (0x0c90e016); through
     * NJS_POINT3's fields, 17 of 91 words differ */
    point = &gacha_dome_points[0].x;
    for (ang = 0; ang < DOME_ARC; ang += BAND_ANGLE) {
        Float top_sin;
        Float top_cos;
        Float bottom_sin;
        Float bottom_cos;

        side = gacha_dome_sides;
        top_sin = njSin(ang);
        top_cos = njCos(ang);
        bottom_sin = njSin(ang + BAND_ANGLE);
        bottom_cos = njCos(ang + BAND_ANGLE);
        top_sin = bottom_sin + (blend * (top_sin - bottom_sin));
        top_cos = bottom_cos + (blend * (top_cos - bottom_cos));
        for (around = 0; around < NJM_DEG_ANG(360); around += SIDE_ANGLE) {
            Float x = *side++;
            Float z = *side++;

            *point++ = top_sin * x;
            *point++ = top_cos;
            *point++ = top_sin * z;
            point += 3;
        }
    }
}

/* draw the dome at the size scl.x, faded to FADE(work), which also thins its
 * bands */
static void gacha_blast_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float scale;
    Float alpha;
    NJS_MODEL *model;

    njSetTexture(&blast_dome_texlist);
    gacha_dome_blend(FADE(work));
    gacha_dome_scroll(BLAST(work)->scroll);
    njPushMatrix(NULL);
    model = &gacha_dome_model;
    scale = work->scl.x;
    alpha = FADE(work);
    model->mats->diffuse.argb.a = (int)(alpha * 255.0F);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njScale(NULL, scale, scale, scale);
    draw_model(model);
    njPopMatrix(1);
}

/* grow the dome. its growth each step slows toward 0.4. its ball's radius
 * stays 0.8 of the dome's size. once FADE_STEPS steps are left, the dome
 * fades and its ball is off */
static void gacha_blast_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case BLAST_INIT:
        work->timer = BLAST_STEPS;
        BLAST(work)->scroll = 0;
        BLAST(work)->growth = 5.0F;
        work->scl.x = 2.0F;
        FADE(work) = 1.0F;
        gacha_dome_init();
        collision_init(task, &gacha_blast_shape, COLLISION_OBJECT, 1, 1);
        task->draw = gacha_blast_draw;
        work->mode = BLAST_GROW;
        break;
    case BLAST_GROW:
        if (--TASK_TIMER(work) == 0) {
            work->mode = BLAST_DONE;
            break;
        }
        if (TASK_TIMER(work) == FADE_STEPS) {
            work->collision->shapes->flags |= SHAPE_OFF;
        }
        if (TASK_TIMER(work) < FADE_STEPS) {
            FADE(work) -= 0.033F;
        }
        BLAST(work)->growth += 0.1F;
        BLAST(work)->growth *= 0.8F;
        work->scl.x += BLAST(work)->growth;
        work->collision->shapes->radius = work->scl.x * 0.8F;
        BLAST(work)->scroll += 10;
        break;
    case BLAST_DONE:
        task_kill(task);
        return;
    }
    gacha_blast_draw(task);
    collision_register(work);
}

/* @hack spelling: gacha_set_speed, a helper the pragma has SHC expand at
 * each call, as the image does. without it, the object's code runs 32 bytes
 * short and 270 of gacha_robot_flash's 275 words differ */
#pragma inline(gacha_set_speed)
/* set the floats at speed to x, y and z */
/* @hack implicit: the parameters in z, y, x order. C leaves the order of a
 * call's arguments unspecified, and SHC evaluates them right to left, so
 * each call draws x's rand first. with the parameters in x, y, z order and
 * the rands drawn into x, y and z before each call, the object's code runs
 * 32 bytes long and 222 of gacha_robot_flash's 275 words differ */
/* @hack spelling: the floats stored through speed, a Float cursor stepped down
 * from past z; through an NJS_VECTOR's fields, gacha_robot_flash runs long
 * and 219 of its 275 words differ */
static void gacha_set_speed(Float *speed, Float z, Float y, Float x)
{
    speed += 3;
    *--speed = z;
    *--speed = y;
    *--speed = x;
}

static void gacha_robot_flash(struct task_work *work,
                              struct gacha_robot_motion *motion)
{
    int i;

    gacha_robot_flash_large.pos.x = work->pos.x;
    gacha_robot_flash_large.pos.y = work->pos.y + 5.0F;
    gacha_robot_flash_large.pos.z = work->pos.z;
    gacha_robot_flash_small.pos.x = work->pos.x;
    gacha_robot_flash_small.pos.y = work->pos.y + 5.0F;
    gacha_robot_flash_small.pos.z = work->pos.z;
    for (i = 0; i < 3; i++) {
        gacha_set_speed(&gacha_robot_flash_large.speed.x,
                        ((rand() * RAND_SCALE) - 0.5F) * 2.0F,
                        ((rand() * RAND_SCALE) - 0.5F) * 1.0F,
                        ((rand() * RAND_SCALE) - 0.5F) * 2.0F);
        hit_create_from(&gacha_robot_flash_large);
    }
    for (i = 0; i < 8; i++) {
        gacha_set_speed(&gacha_robot_flash_small.speed.x,
                        ((rand() * RAND_SCALE) - 0.5F) * 6.0F,
                        (((rand() * RAND_SCALE) - 0.5F) * 2.0F) + 0.4F,
                        ((rand() * RAND_SCALE) - 0.5F) * 6.0F);
        hit_create_from(&gacha_robot_flash_small);
    }
}

void gacha_robot_list_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case LIST_INIT:
        gacha_robot_count = 0;
        gacha_wall_count = 0;
        work->mode = LIST_ON;
        break;
    case LIST_ON:
        if (!gacha_robot_count) {
            break;
        }
        gacha_robot_stop_at_walls();
        gacha_robot_rank();
        break;
    }
    gacha_robot_count = 0;
    gacha_wall_count = 0;
}

/* enlist the robot for gacha_robot_list_step's next run, unless the list is
 * full */
static void gacha_robot_enlist(struct task *task)
{
    if (gacha_robot_count < ROBOTS) {
        gacha_robots[gacha_robot_count++].task = task;
    }
}

static void gacha_wall_enlist(struct task *task)
{
    if (gacha_wall_count < WALLS) {
        gacha_walls[gacha_wall_count++] = task;
        /* @hack redundant: a return where control returns anyway, which keeps
         * gacha_wall_enlist a function of its own; without it, SHC expands
         * the call inline and omits the function, and 15 of 15 words
         * differ */
        return;
    }
}

static void gacha_robot_swap(struct gacha_robot_entry *left,
                             struct gacha_robot_entry *right)
{
    struct task *task = left->task;
    Float distance = left->distance;

    left->distance = right->distance;
    left->task = right->task;
    right->distance = distance;
    right->task = task;
}

/* partition first to last about last's distance, nearer entries first.
 * return where last's entry ends up */
static struct gacha_robot_entry *
gacha_robot_partition(struct gacha_robot_entry *first,
                      struct gacha_robot_entry *last)
{
    struct gacha_robot_entry *left = first - 1;
    struct gacha_robot_entry *right = last;
    Float pivot = last->distance;

    for (;;) {
        do {
            left++;
        } while (left->distance < pivot);
        do {
            right--;
        } while (left < right && right->distance > pivot);
        if (left >= right) {
            break;
        }
        gacha_robot_swap(left, right);
    }
    gacha_robot_swap(left, last);
    return left;
}

/* sort first to last by distance, nearest first */
static void gacha_robot_sort(struct gacha_robot_entry *first,
                             struct gacha_robot_entry *last)
{
    if (first < last) {
        struct gacha_robot_entry *pivot = gacha_robot_partition(first, last);

        gacha_robot_sort(first, pivot - 1);
        gacha_robot_sort(pivot + 1, last);
    }
}

/* give each robot in the list its distance from player 0, and its rank */
static void gacha_robot_rank(void)
{
    int i;
    struct task *task;
    struct gacha_robot_entry *entry = gacha_robots;
    struct task_work *player = player_work[0];
    struct gacha_robot_motion *motion;
    struct gacha_robot_entry *first;
    struct gacha_robot_entry *last;

    for (i = 0; i < gacha_robot_count; i++) {
        task = entry->task;
        entry->distance = njDistanceP2P(&player->pos, &task->work->pos);
        entry++;
    }
    /* @hack spelling: the sort's ends held in first and last, which keeps
     * gacha_robots in R10 (MOV.L @(0x0c90e6e4),R10 at 0x0c90e612); passed
     * straight to the sort, it takes R11 and 10 of 65 words differ */
    first = gacha_robots;
    last = gacha_robots + (gacha_robot_count - 1);
    gacha_robot_sort(first, last);
    entry = gacha_robots;
    for (i = 0; i < gacha_robot_count; i++) {
        motion = (struct gacha_robot_motion *)entry->task->motion;
        motion->rank = i;
        motion->player_distance = entry->distance;
        entry++;
    }
}

/* test each listed robot against each listed wall. a wall that stops a robot
 * sets ROBOT_AT_WALL, and PUSH_FIRM(PUSH_ALL) in its cylinder's push. no
 * group's shapes can then push the cylinder. each wall's test overwrites the
 * flag and those bits, so the last wall's result holds */
static void gacha_robot_stop_at_walls(void)
{
    int i;
    int j;
    int index; /* NOLINT(clang-diagnostic-unused-but-set-variable) */
    struct gacha_robot_entry *entry;
    struct task *task;
    struct collision_shape *shape;
    struct task **wall;

    for (i = 0; i < gacha_wall_count; i++) {
        /* @hack redundant: index, set to i and never read, which has SHC
         * work wall out from i each pass (SHLL2 R3 at 0x0c90e6a6); without
         * it, 49 of 72 words differ */
        index = i;
        entry = gacha_robots;
        wall = gacha_walls + i;
        /* @hack spelling: entry stepped as entry = entry + 1, which keeps
         * entry in R13 (MOV.L @(0x0c90e6e4),R13 at 0x0c90e6a8); as entry++,
         * it takes R12 and 9 of 72 words differ */
        for (j = 0; j < gacha_robot_count; j++, entry = entry + 1) {
            task = entry->task;
            shape = task->work->collision->shapes;
            if (gacha_wall_stop(task, *wall) == TRUE) {
                task->work->flags |= ROBOT_AT_WALL;
                shape->push |= PUSH_FIRM(PUSH_ALL);
            } else {
                task->work->flags &= ~ROBOT_AT_WALL;
                shape->push &= PUSH_ALL;
            }
        }
    }
    /* @hack redundant: a store of 0 to gacha_wall_count that never runs, which
     * keeps the image's 4-byte frame and 0 in R8 (0x0c90e692, 0x0c90e696), and
     * gacha_wall_step, after it, in place; without it, the frame goes, the
     * function runs 2 bytes short and 51 of 72 words differ */
    if (0) {
        gacha_wall_count = 0;
    }
}

/* a wall reaching WALL_UNIT times scl.x each way along x, and WALL_UNIT
 * times scl.y up, turned by ang.y. its front faces +z. in the set editor, it
 * draws as a grid and stops no robot */
void gacha_wall_step(struct task *task)
{
    struct task_work *work = task->work;
    struct gacha_wall *wall;
    Float dx;
    Float dz;
    Float x;
    Float y;
    Float z;
    Float *pos;

    if (placement_try_leave(task)) {
        return;
    }
    if (edit_mode) {
        gacha_wall_draw_grid(work);
        return;
    }
    switch (work->mode) {
    case WALL_INIT:
        wall = heap_calloc(1, sizeof(struct gacha_wall));
        WALL(work)->wall = wall;
        wall->ang = work->ang.y;
        wall->half_width = work->scl.x * WALL_UNIT;
        wall->height = work->scl.y * WALL_UNIT;
        wall->pos = work->pos;
        dx = njAbs(njCos(wall->ang) * wall->half_width) + 1.0F;
        dz = njAbs(njSin(wall->ang) * wall->half_width) + 1.0F;
        /* @hack spelling: the position read through pos, a Float cursor, with
         * @R4+ (0x0c90e7f6); through the fields, the function runs long and
         * 57 of 129 words differ */
        pos = &wall->pos.x;
        x = *pos++;
        y = *pos++;
        z = *pos++;
        wall->min.x = x - dx;
        wall->min.y = y - 1.0F;
        wall->min.z = z - dz;
        wall->max.x = x + dx;
        wall->max.y = y + wall->height + 1.0F;
        wall->max.z = z + dz;
        task->destructor = gacha_wall_destroy;
        work->mode++;
        break;
    case WALL_ON:
        break;
    }
    gacha_wall_enlist(task);
}

static void gacha_wall_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (WALL(work)->wall) {
        heap_free(WALL(work)->wall);
    }
}

/* TRUE when the wall stops the robot, which then stands 1.25 in front of it
 * with no speed. the wall stops a robot within a unit of its face, on either
 * side. a robot that moves along the wall's +z, its front, by over 0.01 a
 * step goes free */
static int gacha_wall_stop(struct task *robot, struct task *wall_task)
{
    struct task_work *work = robot->work;
    struct gacha_robot_motion *motion =
        (struct gacha_robot_motion *)robot->motion;
    struct gacha_wall *wall = WALL(wall_task->work)->wall;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    Float *walk;
    Float x;
    Float y;
    Float z;
    Float bx;
    Float by;
    Float bz;
    Float vz;
    Float zero;

    pos = work->pos;
    speed = motion->speed;
    /* @hack spelling: the points read through walk, a Float cursor, as with
     * @R4+ at 0x0c90e8ac; through the fields, 94 of 167 words differ */
    walk = &pos.x;
    x = *walk++;
    y = *walk++;
    z = *walk;
    walk = &wall->min.x;
    bx = *walk++;
    by = *walk++;
    bz = *walk;
    if (x < bx || y < by || z < bz) {
        return FALSE;
    }
    walk = &wall->max.x;
    bx = *walk++;
    by = *walk++;
    bz = *walk;
    if (x > bx || y > by || z > bz) {
        return FALSE;
    }
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &wall->pos);
    TURN(njRotateY, wall->ang);
    njInvertMatrix(NULL);
    njCalcPoint(NULL, &pos, &pos);
    njCalcVector(NULL, &speed, &speed);
    njPopMatrix(1);
    walk = &pos.x;
    x = *walk++;
    walk++;
    z = *walk;
    walk = &speed.x;
    /* @hack spelling: walk = walk + 2, which keeps pos's x, y and z in FR4,
     * FR6 and FR5 (FMOV.S @R4+,FR4 at 0x0c90e8ac); as walk += 2, they take
     * FR6, FR5 and FR4, and 16 of 167 words differ */
    walk = walk + 2;
    vz = *walk;
    if (x > wall->half_width || x < -wall->half_width || z > 1.0F ||
        z < -1.0F) {
        return FALSE;
    }
    if (vz > 0.01F) {
        return FALSE;
    }
    pos.z = 1.25F;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &wall->pos);
    TURN(njRotateY, wall->ang);
    njCalcPoint(NULL, &pos, &work->pos);
    njPopMatrix(1);
    /* @hack spelling: a zero of its own, which puts the cursor's ADD #12,R4
     * ahead of the FLDI0 (0x0c90ea04); with 0.0F in each store, the two swap
     * and 4 of 167 words differ */
    zero = 0.0F;
    /* @hack spelling: the speed cleared through walk, a Float cursor stepped
     * down from past z (FMOV.S FR4,@-R4 at 0x0c90ea0a); through the fields, 34
     * of 167 words differ */
    walk = &motion->speed.x + 3;
    *--walk = zero;
    *--walk = zero;
    *--walk = zero;
    return TRUE;
}

/* draw the wall as a grid of WALL_UNIT squares, scl.x and scl.y rounded, in
 * GRID_COLOR. an arrowhead in ARROW_COLOR points to its front */
static void gacha_wall_draw_grid(struct task_work *work)
{
    Float min_x;
    Float max_x;
    Float min_y;
    Float max_y;
    Float at;
    NJS_POINT3 *first;
    NJS_POINT3 *second;
    NJS_POINT3COL *line;

    njPushMatrix(NULL);
    min_x = njRoundOff(-work->scl.x);
    max_x = njRoundOff(work->scl.x);
    min_y = 0.0F;
    max_y = njRoundOff(work->scl.y);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njScale(NULL, WALL_UNIT, WALL_UNIT, WALL_UNIT);
    /* @hack spelling: the chained stores, here and in each loop, which keep the
     * colors' address in R11 (MOV.L @(0x0c90ebe8),R11 at 0x0c90ea8e); as
     * stores of their own, 27 of 165 words differ */
    gacha_wall_line_colors[0].color = gacha_wall_line_colors[1].color =
        GRID_COLOR;
    /* @hack spelling: first and second set inside their first stores here, and
     * second again inside its first store after the first loop. they keep
     * second in R9 (MOV R10,R9 at 0x0c90ea92); set by statements of their
     * own, 21 of 165 words differ */
    (first = &gacha_wall_line_points[0])->z = 0.0F;
    line = &gacha_wall_line;
    (second = &gacha_wall_line_points[1])->z = 0.0F;
    first->y = min_y;
    second->y = max_y;
    /* NOLINTNEXTLINE(bugprone-float-loop-counter) */
    for (at = min_x; at <= max_x; at += 1.0F) {
        first->x = second->x = at;
        njDrawLine3D(line, 1, NJD_DRAW_NORMAL);
    }
    first = gacha_wall_line_points;
    first->x = min_x;
    (second = &gacha_wall_line_points[1])->x = max_x;
    /* NOLINTNEXTLINE(bugprone-float-loop-counter) */
    for (at = min_y; at <= max_y; at += 1.0F) {
        first->y = second->y = at;
        njDrawLine3D(line, 1, NJD_DRAW_NORMAL);
    }
    gacha_wall_line_colors[0].color = gacha_wall_line_colors[1].color =
        ARROW_COLOR;
    gacha_wall_line_points[0].x = 0.0F;
    gacha_wall_line_points[0].y = 0.1F;
    gacha_wall_line_points[0].z = 1.0F;
    gacha_wall_line_points[1].x = min_x;
    gacha_wall_line_points[1].y = 0.1F;
    gacha_wall_line_points[1].z = 0.0F;
    njDrawLine3D(line, 1, NJD_DRAW_NORMAL);
    gacha_wall_line_points[0].x = 0.0F;
    gacha_wall_line_points[0].y = 0.1F;
    gacha_wall_line_points[0].z = 1.0F;
    gacha_wall_line_points[1].x = max_x;
    gacha_wall_line_points[1].y = 0.1F;
    gacha_wall_line_points[1].z = 0.0F;
    njDrawLine3D(line, 1, NJD_DRAW_NORMAL);
    njPopMatrix(1);
}

void gacha_robot_launch(struct task *machine, struct gacha_launch *launch)
{
    struct task *robot =
        task_create_child(TASK_WORK, gacha_robot_step, machine);
    struct task_work *work;
    struct gacha_robot_motion *motion;

    if (robot) {
        gacha_robot_init(robot);
        work = robot->work;
        motion = (struct gacha_robot_motion *)robot->motion;
        switch (launch->kind) {
        case LAUNCH_DROP:
            work->mode = ROBOT_FALL;
            work->pos = launch->pos;
            work->ang.y = launch->ang_y;
            motion->ground = launch->ground;
            break;
        case LAUNCH_WITH_SPEED:
            work->mode = ROBOT_FALL;
            work->pos = launch->pos;
            work->ang.y = launch->ang_y;
            motion->accel = launch->speed;
            motion->ground = launch->ground;
            break;
        }
    }
}

/* draw the piece, then its shadow on its floor */
static void gacha_piece_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct gacha_piece_motion *motion =
        (struct gacha_piece_motion *)task->motion;

    njSetTexture(&gacha_robot_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njRotateZXY(NULL, work->ang.x, work->ang.y, work->ang.z);
    draw_model(PIECE(work)->model);
    njPopMatrix(1);
    texture_set_regular();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, motion->ground + 0.5F, work->pos.z);
    njScale(NULL, 1.0F, 1.0F, 1.0F);
    draw_tree(shadow_object);
    njPopMatrix(1);
}

/* a piece falls, and bounces four times off the height it started at, then
 * rests. it kills itself on its 102nd step */
static void gacha_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct gacha_piece_motion *motion =
        (struct gacha_piece_motion *)task->motion;
    Float x;
    Float y;
    Float z;
    Float *walk;
    Float *accel;
    Float ax;
    Float ay;
    Float az;
    Float px;
    Float py;
    Float pz;

    if (TASK_TIMER(work)++ > 100) {
        work->mode = PIECE_DONE;
    }
    switch (work->mode) {
    case PIECE_INIT:
        motion->ground = work->pos.y;
        task->draw = gacha_piece_draw;
        work->mode = PIECE_BOUNCE;
        break;
    case PIECE_BOUNCE:
        motion->accel.y += -0.1F;
        if (work->pos.y < motion->ground) {
            if (work->tick > 3) {
                work->mode = PIECE_REST;
                /* @hack spelling: the chained clear, which loads its 0 into FR6
                 * (FLDI0 FR6 at 0x0c90ecf6); as two stores, the 0 takes FR4
                 * and 28 of 158 words differ */
                motion->speed.y = motion->speed.x = 0.0F;
                motion->speed.z = 0.0F;
                motion->spin_x = 0;
                motion->spin_z = 0;
                break;
            }
            work->pos.y = motion->ground + 1.0F;
            motion->speed.x *= 0.8F;
            motion->speed.y *= -0.8F;
            motion->speed.z *= 0.8F;
            motion->spin_x *= 0.6F;
            /* @hack spelling: spin_z's product written out, which leaves it
             * in R3 (STS FPUL,R3 at 0x0c90edaa); as spin_z *= 0.6F, it takes
             * R2 and 2 of 158 words differ */
            motion->spin_z = motion->spin_z * 0.6F;
            work->tick++;
        }
        break;
    case PIECE_REST:
        break;
    case PIECE_DONE:
        task_kill(task);
        return;
    }
    /* @hack spelling: the speed, the accel and the position read and written
     * through Float cursors, as with @R7+ at 0x0c90ee30; through the fields,
     * the function runs long and 49 of 158 words differ */
    /* @hack spelling: each step of the speed's read a statement of its own,
     * which reads x into FR4 and y into FR5 (0x0c90ee30, 0x0c90ee34); folded
     * into the reads, the two trade and 10 of 158 words differ */
    walk = &motion->speed.x;
    x = *walk;
    walk++;
    y = *walk;
    walk++;
    z = *walk;
    walk++;
    accel = &motion->accel.x;
    ax = *accel++;
    ay = *accel++;
    az = *accel++;
    /* @hack spelling: x's and y's sums written out, which load the clear's 0
     * into FR6 (FLDI0 FR6 at 0x0c90ecf6); as +=, the 0 takes FR4 and 30 of
     * 158 words differ */
    x = x + ax;
    y = y + ay;
    z += az;
    *--accel = 0.0F;
    *--accel = 0.0F;
    *--accel = 0.0F;
    x *= 0.95F;
    y *= 0.95F;
    z *= 0.95F;
    *--walk = z;
    *--walk = y;
    *--walk = x;
    walk = &work->pos.x;
    px = *walk++;
    py = *walk++;
    pz = *walk++;
    px += x;
    py += y;
    pz += z;
    /* @hack redundant: a test of pz that never runs, a use SHC counts, which
     * keeps pz in FR6 and py in FR8 (0x0c90ee60, 0x0c90ee5c); without it, pz
     * and py trade registers and 6 of 158 words differ */
    if (0) {
        if (pz) {
        }
    }
    *--walk = pz;
    *--walk = py;
    *--walk = px;
    work->ang.x += motion->spin_x;
    work->ang.z += motion->spin_z;
    gacha_piece_draw(task);
}

/* throw the robot's three pieces up and out at random */
static void gacha_robot_break(struct task_work *work)
{
    int i;
    struct task *task;
    struct task_work *piece;
    struct gacha_piece_motion *motion;
    Angle ang;
    Float speed;

    for (i = 0; i < 3; i++) {
        task = task_create(TASK_WORK | TASK_MOTION, 3, gacha_piece_step);
        if (task) {
            motion = (struct gacha_piece_motion *)task->motion;
            piece = task->work;
            piece->pos = work->pos;
            ang = RANDOM_ANGLE();
            piece->ang.y = ang;
            speed = RANDOM(3.0F, 5.0F);
            motion->speed.x = speed * njCos(ang);
            motion->speed.y = RANDOM(2.0F, 4.0F);
            motion->speed.z = speed * njSin(ang);
            motion->spin_x = (Angle)(rand() * 0.125F) - 0x800;
            motion->spin_z = (Angle)(rand() * 0.125F) - 0x800;
            PIECE(piece)->model = gacha_robot_pieces[i];
        }
    }
}
