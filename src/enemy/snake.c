/* the snake: a line of eleven child tasks, its segments. the lead, at the
 * line's front, draws nothing and circles where the snake was placed. the
 * ten behind it follow as its head, body and tail. now and then the
 * burners down its body fire, or a roll runs down it. once a segment is
 * hit, every segment bursts in turn, outward from that one */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "animal/kinds.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/explosion.h"
#include "effect/flame.h"
#include "enemy/enemy.h"
#include "enemy/snake.h"
#include "object/placement.h"
#include "player/knuckles.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 3
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 6
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 8
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 8
#include "enemy/snake/text.inc"

enum snake_mode {
    SNAKE_INIT,
    SNAKE_CIRCLE,
    SNAKE_BREAK_UP, /* from a hit until every segment has burst */
    SNAKE_END
};

enum segment_mode {
    SEGMENT_INIT,
    SEGMENT_MOVE,
    SEGMENT_WAIT, /* for its turn to burst */
    SEGMENT_GONE,
    SEGMENT_END
};

/* every segment, the lead included */
#define SEGMENT_COUNT 11

#define SNAKE_HIT 0x4 /* in the snake's flags: a segment is hit */

struct snake_vars {
    Uint8 unk_00[9];
    Sint8 hit_place;
};

struct segment_vars {
    Uint8 unk_00[8];
    /* @hack type: BURNING, ROLLING and BURN_STEPS as the array state,
     * which SHC reaches through an address it forms (MOV R13,R2 and ADD
     * #16,R2 at 0x8c0ff5dc, 0x8c0ff5e2); as three fields, both of
     * snake_segment_init's stores take @(R0,R13), and summed over the five
     * functions that use state, 183 of 440 words differ */
    Sint8 state[3];
};
/* the indexes into state */
enum segment_state {
    BURNING, /* 1 while the burners wait their turn or fire */
    ROLLING, /* 1 while the segment rolls a turn */
    BURN_STEPS /* the steps the burners have fired */
};

#define VARS(work) ((struct snake_vars *)&(work)->vars)
#define SEGMENT(work) ((struct segment_vars *)&(work)->vars)

/* a segment's place in the line */
#define PLACE(work) ((work)->submode)
/* the lead draws nothing and has no shape */
#define LEAD_PLACE 0
#define HEAD_PLACE 1
#define TAIL_PLACE (SEGMENT_COUNT - 1)
/* the snake's count of segments burst */
#define BURSTS(work) ((work)->submode)

/* the snake's burn timing, and the radius of the lead's circle round home.
 * snake_create_segments copies the radius into each segment */
#define SINCE_BURN(work) ((work)->scl.x) /* steps since the last burn */
#define WAIT(work) ((work)->scl.y) /* steps before a roll or the next burn */
#define RADIUS(work) ((work)->scl.z)

/* the radius, held to these */
#define RADIUS_MIN 60.0F
#define RADIUS_MAX 150.0F

/* in the snake's record: its master emerald piece, and the phase of the
 * lead's rise and fall. the phase is in degrees */
#define PIECE(record) ((record)->motion.phase)
#define BOB(record) ((record)->frame)

#define SE_BURNERS 824
/* the place whose burners play SE_BURNERS for them all */
#define SOUND_PLACE 3
/* the place that frees an animal as it bursts */
#define ANIMAL_PLACE 5

/* steps between one place's burn and the next's, from the head back */
#define BURN_GAP 6
/* the steps each place away from the segment hit adds to a burst's wait */
#define BURST_GAP 3
/* the chance that a step past the wait rolls the lead rather than burns */
#define ROLL_CHANCE 0.15F
/* how far the segment ahead rolls before the next starts its roll */
#define ROLL_LAG 0x1000
/* how far the lead moves along its circle a step */
#define CIRCLE_SPEED 0.7F

struct snake_part {
    NJS_OBJECT *object;
    NJS_TEXLIST *texlist;
    Float spacing; /* how far behind the segment ahead */
};

static void snake_destroy(struct task *task);

/* declared ahead for snake_parts, which the image places before them */
static NJS_OBJECT snake_head_object;
static NJS_OBJECT snake_neck_object;
static NJS_OBJECT snake_body_object;
static NJS_OBJECT snake_tail_object;

/* clang-format off */
static struct collision_shape snake_head_shapes[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_TURN_YZX,
     ATTACK(1) | SHAPE_HURTS_PLAYERS, 0, {11.5F, -3.0F, 0.0F}, 5.5F, 0.0F, 0.0F,
     {0}, 0, NULL},
};
static struct collision_shape snake_neck_shapes[2] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_TURN_YZX,
     ATTACK(1) | SHAPE_HURTS_PLAYERS, 0, {7.0F, -1.0F, -4.0F}, 5.0F, 0.0F, 0.0F,
     {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_TURN_YZX,
     ATTACK(1) | SHAPE_HURTS_PLAYERS, 0, {7.0F, -1.0F, 4.0F}, 5.0F, 0.0F, 0.0F,
     {0}, 0, NULL},
};
static struct collision_shape dat_8c5b3c4c[2] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_TURN_YZX,
     ATTACK(1) | SHAPE_HURTS_PLAYERS, 0, {3.0F, -1.0F, -7.0F}, 5.0F, 0.0F, 0.0F,
     {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_TURN_YZX,
     ATTACK(1) | SHAPE_HURTS_PLAYERS, 0, {3.0F, -1.0F, 7.0F}, 5.0F, 0.0F, 0.0F,
     {0}, 0, NULL},
};
static struct collision_shape snake_body_shapes[2] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_TURN_YZX,
     ATTACK(1) | SHAPE_HURTS_PLAYERS, 0, {4.0F, 0.0F, -5.5F}, 7.0F, 0.0F, 0.0F,
     {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_TURN_YZX,
     ATTACK(1) | SHAPE_HURTS_PLAYERS, 0, {4.0F, 0.0F, 5.5F}, 7.0F, 0.0F, 0.0F,
     {0}, 0, NULL},
};
static struct collision_shape dat_8c5b3d1c = {
    SHAPE_CAPSULE | SHAPE_HURTABLE | SHAPE_TURN_YZX,
    ATTACK(2) | GUARD(3) | SHAPE_HURTS_PLAYERS, PUSH_FIRM(PUSH_ALL),
    {4.0F, 0.0F, 0.0F}, 6.0F, 8.0F, 0.0F, {0x4000, 0, 0}, 0, NULL};
static struct collision_shape snake_tail_shapes[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_TURN_YZX,
     ATTACK(1) | SHAPE_HURTS_PLAYERS, 0, {4.0F, 0.0F, 0.0F}, 6.0F, 0.0F, 0.0F,
     {0}, 0, NULL},
};
/* clang-format on */

/* each place's part. the lead, at LEAD_PLACE, has none */
static struct snake_part snake_parts[SEGMENT_COUNT] = {
    {NULL, NULL, 0.0F},
    {&snake_head_object, &e_snake_texlist, 1.0F},
    {&snake_neck_object, &e_snake_texlist, 5.0F},
    {&snake_body_object, &e_snake_texlist, 6.0F},
    {&snake_body_object, &e_snake_texlist, 11.0F},
    {&snake_body_object, &e_snake_texlist, 11.0F},
    {&snake_body_object, &e_snake_texlist, 11.0F},
    {&snake_body_object, &e_snake_texlist, 11.0F},
    {&snake_body_object, &e_snake_texlist, 11.0F},
    {&snake_neck_object, &e_snake_texlist, 15.5F},
    {&snake_tail_object, &e_snake_texlist, 7.8F},
};

static NJS_TEXNAME e_snake_textures[9] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0}, {TEXT_6, 0, 0},
    {TEXT_7, 0, 0}, {TEXT_8, 0, 0}, {TEXT_9, 0, 0},
};
NJS_TEXLIST e_snake_texlist = {
#include "enemy/snake/e_snake_texlist.inc"
};

static NJS_MATERIAL snake_head_materials[6] = {
#include "enemy/snake/snake_head_materials.inc"
};

static Sint16 snake_head_meshes0[18] = {
#include "enemy/snake/snake_head_meshes0.inc"
};

static Sint16 snake_head_meshes1[132] = {
#include "enemy/snake/snake_head_meshes1.inc"
};

static Sint16 snake_head_meshes2[36] = {
#include "enemy/snake/snake_head_meshes2.inc"
};

static Sint16 snake_head_meshes3[6] = {
#include "enemy/snake/snake_head_meshes3.inc"
};

static NJS_COLOR snake_head_uvs0[18] = {
#include "enemy/snake/snake_head_uvs0.inc"
};

static NJS_COLOR snake_head_uvs1[132] = {
#include "enemy/snake/snake_head_uvs1.inc"
};

static NJS_COLOR snake_head_uvs2[36] = {
#include "enemy/snake/snake_head_uvs2.inc"
};

static NJS_COLOR snake_head_uvs3[6] = {
#include "enemy/snake/snake_head_uvs3.inc"
};

static NJS_MESHSET snake_head_meshsets[4] = {
#include "enemy/snake/snake_head_meshsets.inc"
};

static NJS_POINT3 snake_head_points[42] = {
#include "enemy/snake/snake_head_points.inc"
};

static NJS_VECTOR snake_head_normals[42] = {
#include "enemy/snake/snake_head_normals.inc"
};

static NJS_MODEL snake_head_model = {
#include "enemy/snake/snake_head_model.inc"
};

static NJS_OBJECT snake_head_object = {
#include "enemy/snake/snake_head_object.inc"
};

static NJS_MATERIAL snake_neck_burner_minus_z_materials[2] = {
#include "enemy/snake/snake_neck_burner_minus_z_materials.inc"
};

static Sint16 snake_neck_burner_minus_z_meshes0[36] = {
#include "enemy/snake/snake_neck_burner_minus_z_meshes0.inc"
};

static Sint16 snake_neck_burner_minus_z_meshes1[18] = {
#include "enemy/snake/snake_neck_burner_minus_z_meshes1.inc"
};

static NJS_COLOR snake_neck_burner_minus_z_uvs0[36] = {
#include "enemy/snake/snake_neck_burner_minus_z_uvs0.inc"
};

static NJS_COLOR snake_neck_burner_minus_z_uvs1[18] = {
#include "enemy/snake/snake_neck_burner_minus_z_uvs1.inc"
};

static NJS_MESHSET snake_neck_burner_minus_z_meshsets[2] = {
#include "enemy/snake/snake_neck_burner_minus_z_meshsets.inc"
};

static NJS_POINT3 snake_neck_burner_minus_z_points[13] = {
#include "enemy/snake/snake_neck_burner_minus_z_points.inc"
};

static NJS_VECTOR snake_neck_burner_minus_z_normals[13] = {
#include "enemy/snake/snake_neck_burner_minus_z_normals.inc"
};

static NJS_MODEL snake_neck_burner_minus_z_model = {
#include "enemy/snake/snake_neck_burner_minus_z_model.inc"
};

static NJS_OBJECT snake_neck_burner_minus_z_object = {
#include "enemy/snake/snake_neck_burner_minus_z_object.inc"
};

static NJS_MATERIAL snake_neck_burner_plus_z_materials[2] = {
#include "enemy/snake/snake_neck_burner_plus_z_materials.inc"
};

static Sint16 snake_neck_burner_plus_z_meshes0[36] = {
#include "enemy/snake/snake_neck_burner_plus_z_meshes0.inc"
};

static Sint16 snake_neck_burner_plus_z_meshes1[18] = {
#include "enemy/snake/snake_neck_burner_plus_z_meshes1.inc"
};

static NJS_COLOR snake_neck_burner_plus_z_uvs0[36] = {
#include "enemy/snake/snake_neck_burner_plus_z_uvs0.inc"
};

static NJS_COLOR snake_neck_burner_plus_z_uvs1[18] = {
#include "enemy/snake/snake_neck_burner_plus_z_uvs1.inc"
};

static NJS_MESHSET snake_neck_burner_plus_z_meshsets[2] = {
#include "enemy/snake/snake_neck_burner_plus_z_meshsets.inc"
};

static NJS_POINT3 snake_neck_burner_plus_z_points[13] = {
#include "enemy/snake/snake_neck_burner_plus_z_points.inc"
};

static NJS_VECTOR snake_neck_burner_plus_z_normals[13] = {
#include "enemy/snake/snake_neck_burner_plus_z_normals.inc"
};

static NJS_MODEL snake_neck_burner_plus_z_model = {
#include "enemy/snake/snake_neck_burner_plus_z_model.inc"
};

static NJS_OBJECT snake_neck_burner_plus_z_object = {
#include "enemy/snake/snake_neck_burner_plus_z_object.inc"
};

static NJS_MATERIAL snake_neck_materials[7] = {
#include "enemy/snake/snake_neck_materials.inc"
};

static Sint16 snake_neck_meshes0[12] = {
#include "enemy/snake/snake_neck_meshes0.inc"
};

static Sint16 snake_neck_meshes1[24] = {
#include "enemy/snake/snake_neck_meshes1.inc"
};

static Sint16 snake_neck_meshes2[12] = {
#include "enemy/snake/snake_neck_meshes2.inc"
};

static Sint16 snake_neck_meshes3[12] = {
#include "enemy/snake/snake_neck_meshes3.inc"
};

static Sint16 snake_neck_meshes4[30] = {
#include "enemy/snake/snake_neck_meshes4.inc"
};

static NJS_COLOR snake_neck_uvs0[12] = {
#include "enemy/snake/snake_neck_uvs0.inc"
};

static NJS_COLOR snake_neck_uvs2[12] = {
#include "enemy/snake/snake_neck_uvs2.inc"
};

static NJS_COLOR snake_neck_uvs3[12] = {
#include "enemy/snake/snake_neck_uvs3.inc"
};

static NJS_COLOR snake_neck_uvs4[30] = {
#include "enemy/snake/snake_neck_uvs4.inc"
};

static NJS_MESHSET snake_neck_meshsets[5] = {
#include "enemy/snake/snake_neck_meshsets.inc"
};

static NJS_POINT3 snake_neck_points[17] = {
#include "enemy/snake/snake_neck_points.inc"
};

static NJS_VECTOR snake_neck_normals[17] = {
#include "enemy/snake/snake_neck_normals.inc"
};

static NJS_MODEL snake_neck_model = {
#include "enemy/snake/snake_neck_model.inc"
};

static NJS_OBJECT snake_neck_object = {
#include "enemy/snake/snake_neck_object.inc"
};

static NJS_MATERIAL snake_body_burner_minus_z_materials[2] = {
#include "enemy/snake/snake_body_burner_minus_z_materials.inc"
};

static Sint16 snake_body_burner_minus_z_meshes0[36] = {
#include "enemy/snake/snake_body_burner_minus_z_meshes0.inc"
};

static Sint16 snake_body_burner_minus_z_meshes1[18] = {
#include "enemy/snake/snake_body_burner_minus_z_meshes1.inc"
};

static NJS_COLOR snake_body_burner_minus_z_uvs0[36] = {
#include "enemy/snake/snake_body_burner_minus_z_uvs0.inc"
};

static NJS_COLOR snake_body_burner_minus_z_uvs1[18] = {
#include "enemy/snake/snake_body_burner_minus_z_uvs1.inc"
};

static NJS_MESHSET snake_body_burner_minus_z_meshsets[2] = {
#include "enemy/snake/snake_body_burner_minus_z_meshsets.inc"
};

static NJS_POINT3 snake_body_burner_minus_z_points[13] = {
#include "enemy/snake/snake_body_burner_minus_z_points.inc"
};

static NJS_VECTOR snake_body_burner_minus_z_normals[13] = {
#include "enemy/snake/snake_body_burner_minus_z_normals.inc"
};

static NJS_MODEL snake_body_burner_minus_z_model = {
#include "enemy/snake/snake_body_burner_minus_z_model.inc"
};

static NJS_OBJECT snake_body_burner_minus_z_object = {
#include "enemy/snake/snake_body_burner_minus_z_object.inc"
};

static NJS_MATERIAL snake_body_burner_plus_z_materials[2] = {
#include "enemy/snake/snake_body_burner_plus_z_materials.inc"
};

static Sint16 snake_body_burner_plus_z_meshes0[36] = {
#include "enemy/snake/snake_body_burner_plus_z_meshes0.inc"
};

static Sint16 snake_body_burner_plus_z_meshes1[18] = {
#include "enemy/snake/snake_body_burner_plus_z_meshes1.inc"
};

static NJS_COLOR snake_body_burner_plus_z_uvs0[36] = {
#include "enemy/snake/snake_body_burner_plus_z_uvs0.inc"
};

static NJS_COLOR snake_body_burner_plus_z_uvs1[18] = {
#include "enemy/snake/snake_body_burner_plus_z_uvs1.inc"
};

static NJS_MESHSET snake_body_burner_plus_z_meshsets[2] = {
#include "enemy/snake/snake_body_burner_plus_z_meshsets.inc"
};

static NJS_POINT3 snake_body_burner_plus_z_points[13] = {
#include "enemy/snake/snake_body_burner_plus_z_points.inc"
};

static NJS_VECTOR snake_body_burner_plus_z_normals[13] = {
#include "enemy/snake/snake_body_burner_plus_z_normals.inc"
};

static NJS_MODEL snake_body_burner_plus_z_model = {
#include "enemy/snake/snake_body_burner_plus_z_model.inc"
};

static NJS_OBJECT snake_body_burner_plus_z_object = {
#include "enemy/snake/snake_body_burner_plus_z_object.inc"
};

static NJS_MATERIAL snake_body_materials[6] = {
#include "enemy/snake/snake_body_materials.inc"
};

static Sint16 snake_body_meshes0[24] = {
#include "enemy/snake/snake_body_meshes0.inc"
};

static Sint16 snake_body_meshes1[12] = {
#include "enemy/snake/snake_body_meshes1.inc"
};

static Sint16 snake_body_meshes2[12] = {
#include "enemy/snake/snake_body_meshes2.inc"
};

static Sint16 snake_body_meshes3[30] = {
#include "enemy/snake/snake_body_meshes3.inc"
};

static Sint16 snake_body_meshes4[12] = {
#include "enemy/snake/snake_body_meshes4.inc"
};

static NJS_COLOR snake_body_uvs1[12] = {
#include "enemy/snake/snake_body_uvs1.inc"
};

static NJS_COLOR snake_body_uvs2[12] = {
#include "enemy/snake/snake_body_uvs2.inc"
};

static NJS_COLOR snake_body_uvs3[30] = {
#include "enemy/snake/snake_body_uvs3.inc"
};

static NJS_COLOR snake_body_uvs4[12] = {
#include "enemy/snake/snake_body_uvs4.inc"
};

static NJS_MESHSET snake_body_meshsets[5] = {
#include "enemy/snake/snake_body_meshsets.inc"
};

static NJS_POINT3 snake_body_points[17] = {
#include "enemy/snake/snake_body_points.inc"
};

static NJS_VECTOR snake_body_normals[17] = {
#include "enemy/snake/snake_body_normals.inc"
};

static NJS_MODEL snake_body_model = {
#include "enemy/snake/snake_body_model.inc"
};

static NJS_OBJECT snake_body_object = {
#include "enemy/snake/snake_body_object.inc"
};

static NJS_MATERIAL snake_tail_materials[3] = {
#include "enemy/snake/snake_tail_materials.inc"
};

static Sint16 snake_tail_meshes0[18] = {
#include "enemy/snake/snake_tail_meshes0.inc"
};

static Sint16 snake_tail_meshes1[24] = {
#include "enemy/snake/snake_tail_meshes1.inc"
};

static NJS_COLOR snake_tail_uvs0[18] = {
#include "enemy/snake/snake_tail_uvs0.inc"
};

static NJS_COLOR snake_tail_uvs1[24] = {
#include "enemy/snake/snake_tail_uvs1.inc"
};

static NJS_MESHSET snake_tail_meshsets[2] = {
#include "enemy/snake/snake_tail_meshsets.inc"
};

static NJS_POINT3 snake_tail_points[9] = {
#include "enemy/snake/snake_tail_points.inc"
};

static NJS_VECTOR snake_tail_normals[9] = {
#include "enemy/snake/snake_tail_normals.inc"
};

static NJS_MODEL snake_tail_model = {
#include "enemy/snake/snake_tail_model.inc"
};

static NJS_OBJECT snake_tail_object = {
#include "enemy/snake/snake_tail_object.inc"
};

static void snake_segment_burst(struct task *task)
{
    struct task_work *work;
    struct task_work *snake;
    Float scale;

    /* @hack redundant: the segment's place read and dropped, which keeps its
     * MOV.B (0x8c0ff48e); without it, snake_segment_burst runs 6 bytes
     * short and 45 of 48 words differ */
    (void)PLACE(task->work);
    work = task->work;
    snake = task->parent->work;

    if (PLACE(work) % 2 == 0) {
        scale = 1.2F;
    } else {
        scale = 0.4F;
    }
    explosion_create(work, scale);
    if (PLACE(work) == ANIMAL_PLACE) {
        animal_create_random(3, work->pos.x, work->pos.y, work->pos.z);
    }
    BURSTS(snake)++;
    work->mode = SEGMENT_GONE;
}

static void snake_segment_take_hit(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *hit = work->collision->hit;
    struct task *snake = task->parent;

    if (work->flags & COLLISION_DAMAGED) {
        if (hit->kind == COLLISION_PLAYER || hit->kind == COLLISION_ATTACK) {
            LOCKON_TARGET(snake)->flags |= LOCKON_HIT;
            if (player_work[0] == hit->owner->work) {
                enemy_bounce_player(0);
            } else {
                enemy_bounce_player(1);
            }
        }
        work->mode = SEGMENT_WAIT;
        snake->work->flags |= SNAKE_HIT;
        VARS(snake->work)->hit_place = PLACE(work);
    }
}

static void snake_segment_init(struct task *task)
{
    struct task_work *work = task->work;

    if (enemy_alloc_record(task, work) == NULL) {
        /* @bug memory: the snake's task goes to heap_free (JMP at
         * 0x8c0ff586), but tasks come from the task pool, one heap block,
         * not each from heap_alloc. heap_free takes the 16 bytes before the
         * task for its block's header, and writes the heap's links into
         * them or through them */
        heap_free(task->parent);
        return;
    }
    switch (PLACE(work)) {
    case LEAD_PLACE:
        break;
    case HEAD_PLACE:
        collision_init(task, snake_head_shapes, COLLISION_ITEM, 1, 1);
        break;
    case HEAD_PLACE + 1:
    case TAIL_PLACE - 1:
        collision_init(task, snake_neck_shapes, COLLISION_ITEM, 1, 2);
        break;
    default:
        collision_init(task, snake_body_shapes, COLLISION_ITEM, 1, 2);
        break;
    case TAIL_PLACE:
        collision_init(task, snake_tail_shapes, COLLISION_ITEM, 1, 1);
        break;
    }
    SEGMENT(work)->state[BURNING] = 0;
    SEGMENT(work)->state[BURN_STEPS] = 0;
    work->mode = SEGMENT_MOVE;
}

static void snake_segment_draw_part(struct task_work *work)
{
    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        njSetTexture(snake_parts[PLACE(work)].texlist);
        if (snake_parts[PLACE(work)].object != NULL) {
            njPushMatrix(NULL);
            njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
            TURN(njRotateY, work->ang.y);
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
            njTranslate(NULL, 8.0F, 0.0F, 0.0F);
            njRotateY(NULL, 0x8000);
            draw_tree(snake_parts[PLACE(work)].object);
            njPopMatrix(1);
        }
    }
}

/* the segment as it stands, which task_draw_all draws while the game is
 * paused */
static void snake_segment_draw(struct task *task)
{
    struct task_work *work = task->work;

    snake_segment_draw_part(work);
}

static void snake_segment_burn(struct task_work *work)
{
    NJS_POINT3 from;
    NJS_POINT3 pos;
    NJS_VECTOR along;
    NJS_VECTOR speed;

    njPushMatrix(&_unit_matrix_);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    if (SEGMENT(work)->state[BURNING] == 1 && work->tick++ > TASK_TIMER(work)) {
        from.x = 4.0F;
        from.y = -5.0F;
        from.z = 12.0F;
        along.x = 0.35F;
        along.y = 0.0F;
        along.z = 1.15F;
        njCalcPoint(NULL, &from, &pos);
        njCalcVector(NULL, &along, &speed);
        flame_create(&pos, &speed, 1.0F);
        from.x = 4.0F;
        from.y = -5.0F;
        from.z = -12.0F;
        along.x = 0.35F;
        along.y = 0.0F;
        along.z = -1.15F;
        njCalcPoint(NULL, &from, &pos);
        njCalcVector(NULL, &along, &speed);
        flame_create(&pos, &speed, 1.0F);
        if (SEGMENT(work)->state[BURN_STEPS]++ > 16) {
            SEGMENT(work)->state[BURNING] = 0;
            SEGMENT(work)->state[BURN_STEPS] = 0;
        }
        if (PLACE(work) == SOUND_PLACE) {
            sound_play_at(SE_BURNERS, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
    }
    njPopMatrix(1);
}

static void snake_segment_roll(struct task_work *work)
{
    if (SEGMENT(work)->state[ROLLING] == 1) {
        if (work->ang.x < 0x10000) {
            work->ang.x += 0x98;
            return;
        }
        work->ang.x = 0;
        SEGMENT(work)->state[ROLLING] = 0;
    }
}

/* while the segment waits to burst, its part is drawn twice on a frame's
 * first step */
static void snake_segment_wait(struct task *task)
{
    struct task_work *work = task->work;

    if (work->tick++ < TASK_TIMER(work)) {
        snake_segment_draw_part(work);
    } else {
        snake_segment_burst(task);
    }
    snake_segment_draw_part(work);
}

static void snake_segment_end(struct task *task, struct task_work *work)
{
    struct lockon_target *record = LOCKON_TARGET(task);

    task_destroy(task);
    enemy_end(task, work, record);
}

static void snake_segment_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SEGMENT_INIT:
        snake_segment_init(task);
        task->draw = snake_segment_draw;
        task->destructor = snake_destroy;
        break;
    case SEGMENT_MOVE:
        snake_segment_burn(work);
        snake_segment_roll(work);
        snake_segment_draw_part(work);
        if (PLACE(work) != LEAD_PLACE) {
            snake_segment_take_hit(task);
            collision_register(work);
        }
        break;
    case SEGMENT_WAIT:
        snake_segment_wait(task);
        break;
    case SEGMENT_GONE:
        break;
    case SEGMENT_END:
        snake_segment_end(task, work);
        break;
    }
}

static void snake_create_segments(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    struct task_work *segment;
    Float distance;
    int i;

    if (RADIUS(work) < RADIUS_MIN) {
        RADIUS(work) = RADIUS_MIN;
    } else if (RADIUS(work) > RADIUS_MAX) {
        RADIUS(work) = RADIUS_MAX;
    }
#ifdef GCC
    /* repairs the unset distance the tag in the loop names: GCC's starts at
     * 0. the lead's spacing is 0, so the lead starts where the snake stands.
     * each other segment starts its spacing behind the one ahead */
    distance = 0.0F;
#endif
    for (i = 0; i < SEGMENT_COUNT; i++) {
        child = task_create_child(TASK_WORK, snake_segment_step, task);
        if (child != NULL) {
            segment = child->work;
            /* @bug uninitialized: no path sets distance, which SHC keeps
             * in FR15, so the first spacing adds to whatever the caller
             * left there (FADD FR3,FR15 at 0x8c0ffaa4) */
            /* NOLINTNEXTLINE(clang-diagnostic-uninitialized) */
            distance += snake_parts[i].spacing;
            segment->pos.x = work->pos.x - (njCos(work->ang.y) * distance);
            segment->pos.y =
                work->pos.y +
                (njSin((Angle)(i * ((2.0F * NJD_PI / 8) * ANGLE_PER_RADIAN))) *
                 12.0F);
            segment->pos.z = work->pos.z - (njSin(work->ang.y) * distance);
            segment->ang.x = work->ang.x;
            segment->ang.y = work->ang.y;
            segment->ang.z = work->ang.z;
            segment->tick = 0;
            segment->timer = 0;
            RADIUS(segment) = RADIUS(work);
            PLACE(segment) = i;
        }
    }
}

static void snake_init(struct task *task, struct task_work *work)
{
    struct lockon_target *record = enemy_alloc_record(task, work);

    work->mode = SNAKE_CIRCLE;
    BURSTS(work) = 0;
    SINCE_BURN(work) = 0.0F;
    WAIT(work) = RANDOM(90.0F, 290.0F);
    BOB(record) = rand() * (180.0F * RAND_SCALE);
    record->home.x = work->pos.x;
    record->home.y = work->pos.y;
    record->home.z = work->pos.z;
    snake_create_segments(task);
    /* @hack redundant: a return at the end, with which SHC saves task in a
     * frame slot it opens with ADD #-4,R15 (0x8c0ffd46, 0x8c0ffd4a); without
     * it, SHC pushes task with MOV.L R4,@-R15, snake_init runs 2 bytes short
     * and 51 of 54 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

/* past the wait, each step either starts the lead's roll or starts a burn
 * and the next wait. a burn fires the burners from the head back to the
 * place before the tail, BURN_GAP steps apart */
static void snake_burn_or_roll(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child = task->child;
    struct task_work *segment;
    int i;

    if (SINCE_BURN(work) > WAIT(work)) {
        while (child->next != NULL) {
            child = child->next;
        }
        if (rand() * RAND_SCALE < ROLL_CHANCE) {
            segment = child->work;
            SEGMENT(segment)->state[ROLLING] = 1;
        } else {
            i = 0;
            for (child = child->prev; child->prev != NULL;
                 child = child->prev) {
                segment = child->work;
                segment->tick = 0;
                segment->timer = i++ * BURN_GAP;
                SEGMENT(segment)->state[BURNING] = 1;
            }
            SINCE_BURN(work) = 0.0F;
            WAIT(work) = RANDOM(60.0F, 260.0F);
        }
    }
    SINCE_BURN(work) += 1.0F;
}

static void snake_take_hit(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child = task->child;
    struct task_work *segment;
    struct lockon_target *record;
    Sint8 distance;
    int i;

    if (work->flags & SNAKE_HIT) {
        for (i = 0; i < SEGMENT_COUNT; i++) {
            segment = child->work;
            distance = PLACE(segment) - VARS(work)->hit_place;
            distance = (distance < 0) ? -distance : distance;
            segment->tick = 0;
            segment->timer = distance * BURST_GAP;
            record = LOCKON_TARGET(child);
            if (record != NULL && record->lockon != NULL) {
                if (LOCKON_TARGET(task)->flags & LOCKON_HIT) {
                    record->flags |= LOCKON_HIT;
                }
            }
            segment->mode = SEGMENT_WAIT;
            child = child->next;
        }
        work->mode = SNAKE_BREAK_UP;
    }
}

static void snake_end_when_gone(struct task_work *work)
{
    if (BURSTS(work) == SEGMENT_COUNT) {
        work->mode = SNAKE_END;
    }
}

static void snake_face_along(struct task_work *work, NJS_VECTOR *along)
{
    Float y;
    Float length;

    work->ang.y = (Angle)(atan2f(along->z, -along->x) * ANGLE_PER_RADIAN);
    /* @hack spelling: y read into a local before the length, which keeps it in
     * FR15 across njSqrt (0x8c0fff5a, 0x8c0fff82); read in place, 59 of 63
     * words differ */
    y = along->y;
    length = njSqrt(VECTOR_SQUARED(along));
    work->ang.z = (Angle)(acosf(y / length) * ANGLE_PER_RADIAN) - 0x4000;
}

static void snake_circle(struct task_work *work, struct lockon_target *record)
{
    work->ang.y -= (Angle)((CIRCLE_SPEED * 360.0F) /
                           (RADIUS(work) * (2.0F * NJD_PI)) * ANGLE_PER_DEGREE);
    work->pos.x = record->home.x + (njCos(work->ang.y) * RADIUS(work));
    work->pos.y = record->home.y +
                  (njSin((Angle)(BOB(record) * ANGLE_PER_DEGREE)) * 17.0F);
    work->pos.z = record->home.z + (njSin(work->ang.y) * RADIUS(work));
    BOB(record) += 2.0F;
}

/* set a segment its spacing behind the one ahead, on the line between them.
 * it starts its roll once the one ahead has rolled ROLL_LAG further */
static void snake_follow(struct task_work *ahead, struct task_work *work)
{
    NJS_VECTOR back;
    Float spacing = snake_parts[PLACE(work)].spacing;

    back.x = work->pos.x - ahead->pos.x;
    back.y = work->pos.y - ahead->pos.y;
    back.z = work->pos.z - ahead->pos.z;
    snake_face_along(work, &back);
    njUnitVector(&back);
    back.x *= spacing;
    back.y *= spacing;
    back.z *= spacing;
    work->pos.x = ahead->pos.x + back.x;
    work->pos.y = ahead->pos.y + back.y;
    work->pos.z = ahead->pos.z + back.z;
    if (ahead->ang.x - work->ang.x > ROLL_LAG) {
        SEGMENT(work)->state[ROLLING] = 1;
    }
}

/* move the lead round its circle, and each other segment up behind the one
 * ahead of it. task_create_child puts each new child first in the list, so
 * the lead, made first, is the last child. each child's prev holds the next
 * place back */
static void snake_move(struct task *task, struct lockon_target *record)
{
    struct task *child = task->child;
    int i;

    while (child->next != NULL) {
        child = child->next;
    }
    snake_circle(child->work, record);
    for (i = 0; i < SEGMENT_COUNT - 1; i++) {
        snake_follow(child->work, child->prev->work);
        child = child->prev;
    }
}

static void snake_edit_draw(struct task_work *work)
{
    Float radius = RADIUS(work);

    if (radius < RADIUS_MIN) {
        radius = RADIUS_MIN;
    } else if (radius > RADIUS_MAX) {
        radius = RADIUS_MAX;
    }
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateY, work->ang.y);
    njRotateY(NULL, 0xc000);
    njSetTexture(&e_snake_texlist);
    draw_tree(&snake_head_object);
    /* no draw follows these scales */
    njScale(NULL, radius, 1.0F, radius);
    njScale(NULL, 0.1F, 0.5F, 0.1F);
    njPopMatrix(1);
}

void snake_step(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;

    /* @hack redundant: player 0's work handed to enemy_ignore, which keeps its
     * load into R4 (0x8c100260); without it, 55 of 100 words differ */
    enemy_ignore(player_work[0]);
    record = LOCKON_TARGET(task);
    if (edit_mode) {
        snake_edit_draw(work);
    } else {
        switch (work->mode) {
        case SNAKE_INIT:
            snake_init(task, work);
            record = LOCKON_TARGET(task);
            PIECE(record) = work->ang.z;
            knuckles_find_piece(PIECE(record), &work->pos);
            task->destructor = snake_destroy;
            return;
        case SNAKE_CIRCLE:
            snake_move(task, record);
            snake_burn_or_roll(task);
            snake_take_hit(task);
            break;
        case SNAKE_BREAK_UP:
            snake_move(task, record);
            snake_end_when_gone(work);
            break;
        case SNAKE_END:
            knuckles_take_piece(PIECE(record), &work->pos);
            /* @hack spelling: the two calls in one statement, which keeps
             * placement_kill a JSR ahead of the return (0x8c100352); as
             * two, it becomes a tail jump, snake_step runs 4 bytes long
             * and 35 of 100 words differ */
            task_destroy_children(task), placement_kill(task);
            return;
        default:
            /* @hack redundant: an empty default, which loads
             * knuckles_find_piece's address into R2 (0x8c10035a); without
             * it, the four calls from there trade R2 and R3, and 8 of 100
             * words differ */
            break;
        }
        knuckles_find_piece(PIECE(record), &work->pos);
        if (placement_try_leave(task)) {
            task_destroy_children(task);
            /* @hack redundant: record cleared and never read again,
             * which keeps task_destroy_children a JSR ahead of the return
             * (0x8c100370); without it, the call becomes a tail jump,
             * snake_step runs 4 bytes long and 19 of 100 words differ */
            record = NULL;
            return;
        }
        task_step_children(task);
    }
}

static void snake_destroy(struct task *task)
{
}
