/* player effects: bubbles and ice, and Sonic's spin balls, dash, aura and
 * light charge */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/sprite.h"
#include "draw/tree.h"
#include "hud/hud.h"
#include "object/placement.h"
#include "player/effects.h"
#include "player/player.h"
#include "player/sonic_effect_models.h"
#include "player/sonic_ice_piece.h"
#include "system/frame.h"
#include "system/loader.h"
#include "system/task.h"
#include "system/texlists.h"
#include "system/util.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 8
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 8
#define TEXT_10_LENGTH 8
#define TEXT_11_LENGTH 8
#define TEXT_12_LENGTH 8
#define TEXT_13_LENGTH 8
#define TEXT_14_LENGTH 8
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 8
#define TEXT_17_LENGTH 8
#define TEXT_18_LENGTH 8
#define TEXT_19_LENGTH 8
#define TEXT_20_LENGTH 8
#define TEXT_21_LENGTH 8
#define TEXT_22_LENGTH 8
#define TEXT_23_LENGTH 8
#define TEXT_24_LENGTH 8
#define TEXT_25_LENGTH 8
#define TEXT_26_LENGTH 9
#define TEXT_27_LENGTH 9
#define TEXT_28_LENGTH 9
#define TEXT_29_LENGTH 9
#include "player/effects/text.inc"

/* how many textures SON_EFF's texlist has */
#define SON_EFF_TEXTURES 29

/* bubble_sources holds CHARACTER_SOURCES rows for each of five characters */
#define BUBBLE_SOURCES 15
#define CHARACTER_SOURCES 3

/* a bubbles task's work->tick: which of its character's sources it uses.
 * a drowning player has a source for its bursts and one for between them */
enum bubble_use {
    BUBBLES_UNDER_WATER,
    BUBBLES_DROWN_BURST,
    BUBBLES_DROWN_STEADY
};

/* where a source's bubbles start and the way they head. BUBBLES_AT_WELD4
 * starts them out from weld_point4 along weld_axis4, the point and axis
 * the player's WELD_MARK + 4 weld keeps; BUBBLES_AT_WELD5 takes weld 5's.
 * any other kind starts them eye_height above the player's middle, heading
 * along x */
enum bubble_kind { BUBBLES_AT_WELD4, BUBBLES_AT_WELD5 };

/* a bubbles task's work->submode */
enum bubble_burst { BUBBLES_STEADY, BUBBLES_BURSTING };
/* the timer a burst starts with. bubbles_step takes 1 off it on each step of a
 * burst, and adds 1 back on a frame's first step. it falls only on a frame's
 * later steps, so a burst ends only while frames take two steps or more */
#define BURST_TIME 40

/* a bubble starts less than this far below the water's surface, or not at
 * all */
#define BUBBLE_DEPTH 500.0F

/* how far up from the player a spin ball starts, turned as the player
 * faces */
#define BALL_HEIGHT 5.0F
/* the steps a ball draws after its first: BALL_STEPS for a jitter or
 * follow ball, and SHRINK_STEPS for a shrink ball. each fades out over
 * them */
#define BALL_STEPS 5
#define SHRINK_STEPS 10

/* in a dash's work->flags: it draws dash_textures_b, the second set */
#define DASH_SECOND_SET 0x8
/* the steps a dash draws with the first set. with the second, it ends on
 * step SECOND_SET_END, before that step draws */
#define DASH_STEPS 20
#define SECOND_SET_END 19
/* the streak model's length along x */
#define STREAK_LENGTH 8.0F

/* a trail stops growing at a step this short or shorter */
#define TRAIL_STOP 0.05F

/* a charge spark's speed on each axis lies within half this of 0 */
#define SPARK_SPREAD 1.0F

/* the farthest a light of the light charge flies in a step */
#define CHARGE_SPEED 3.0F

#define ICE_PIECES 16

/* the aura's model: an object of Sonic's s_mot, 0x2ad4 bytes into
 * MOTION_AREA, where the file loads */
#define AURA_MODEL ((NJS_OBJECT *)(MOTION_AREA + 0x2ad4))

struct bubble {
    Float surface; /* the height of the water's surface, where it bursts */
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    Sint16 ang;
    Sint16 spin; /* added to ang each frame */
    Float aspect; /* its height over its width */
    Float size;
    Float max_size;
    struct bubble *prev;
    struct bubble *next;
};

struct bubble_start {
    Sint32 character;
    NJS_POINT3 offset; /* from where the weld puts them */
    Float reach; /* how far along the weld's axis they start */
    Float speed; /* how fast they start along the weld's axis */
    Sint32 kind; /* a bubble_kind */
};

struct bubble_motion {
    Float size_max; /* each grows to a random size between this and size_min */
    Float size_min;
    Float squash; /* a bubble's aspect is 1 less up to this, at random */
    Float drag; /* the share of its speed a bubble keeps each frame */
    Float lift; /* the upward speed a bubble gains each frame */
    /* a new bubble's speed gains up to this either way on each axis */
    Float spread;
    Float rate; /* the chance of a bubble on a step that leaves it to chance */
    /* a bubble spins up to half this either way, in degrees a frame */
    Float spin;
    Float growth; /* the size it gains each frame */
    Float wobble_rate; /* the chance each frame it wobbles */
    /* a wobble adds up to half this either way to the speed on x and z */
    Float wobble;
};

struct bubble_source {
    struct bubble_start start;
    struct bubble_motion motion;
};

struct bubbles_vars {
    struct bubble_source *sources; /* its character's rows, or null */
    Uint8 unk_04[4];
    struct bubble *list;
};

struct ball_vars {
    Uint8 player; /* the player's number, which the player's task sets */
    Uint8 unk_01[7];
    NJS_TEXLIST *texlist; /* s_ball_a or b */
};

struct dash_vars {
    Uint8 player; /* the player's number, which the player's task sets */
    Uint8 unk_01[7];
    NJS_OBJECT *object; /* its own copy of the streak */
};

struct trail_vars {
    Uint32 count; /* the points the strip has room for */
    union {
        Uint32 color;
        struct {
            Uint8 b, g, r;
            /* @hack type: a signed, so charge_spark_create's -1 is MOV #-1
             * (0x8c0bc8ea). as a Uint8 set to 255, SHC loads it from a pool:
             * charge_spark_create runs 70 bytes long and 135 of its 151
             * words differ */
            Sint8 a;
        } argb; /* as NJS_BGRA has them */
    } color;
    NJS_POINT3 *follow; /* the point it grows toward */
};

struct spark_vars {
    Uint32 count; /* the steps it has left */
};

/* the vars of one of the light charge's lights */
struct charge_vars {
    Uint8 player; /* the number of the player it flies to */
    Uint8 unk_01[3];
    Sint32 timer; /* the steps it has left */
};

/* the vars of a block of ice, and of each piece it breaks into */
struct ice_vars {
    NJS_OBJECT *object;
    Uint8 unk_04[4];
    NJS_TEXLIST *texlist;
};

#define BUBBLES(work) ((struct bubbles_vars *)&(work)->vars)
#define BALL(work) ((struct ball_vars *)&(work)->vars)
#define DASH(work) ((struct dash_vars *)&(work)->vars)
#define TRAIL(work) ((struct trail_vars *)&(work)->vars)
#define SPARK(work) ((struct spark_vars *)&(work)->vars)
#define CHARGE(work) ((struct charge_vars *)&(work)->vars)
#define ICE(work) ((struct ice_vars *)&(work)->vars)

/* a bubbles task's source, which work->tick picks, and the number of the
 * player whose bubbles they are, which player_init stores in work->scl.z */
#define SOURCE(work) (&BUBBLES(work)->sources[(work)->tick])
#define OWNER(work) ((Sint8)(work)->scl.z)

/* a trail's strip of triangles, which trail_start allocates and keeps in
 * task->motion */
#define STRIP(task) ((NJS_POINT3COL *)(task)->motion)

static void bubbles_destroy(struct task *task);
static void bubbles_step(struct task *task);
static void bubbles_draw(struct task *task);
static void ball_jitter_place(struct task_work *work);
static void ball_follow_place(struct task_work *work);
static void ball_shrink_place(struct task_work *work);
static void ball_jitter_draw(struct task *task);
static void ball_jitter_step(struct task *task);
static void ball_follow_draw(struct task *task);
static void ball_follow_step(struct task *task);
static void ball_shrink_draw(struct task *task);
static void ball_shrink_step(struct task *task);
static Bool dash_alloc(struct task_work *work);
static void dash_draw(struct task *task);
static void dash_step(struct task *task);
static void dash_destroy(struct task *task);

/* each character's CHARACTER_SOURCES rows, in bubble_use order */
static struct bubble_source bubble_sources[BUBBLE_SOURCES] = {
    {{CHARACTER_TAILS, {0.0F, 0.0F, 0.0F}, 1.3F, 0.2F, BUBBLES_AT_WELD4},
     {0.4F, 0.2F, 0.3F, 0.961F, 0.03F, 0.1F, 0.1F, 8.0F, 0.08F, 0.3F, 0.12F}},
    {{CHARACTER_TAILS, {0.0F, 0.0F, 0.0F}, 1.3F, 0.3F, BUBBLES_AT_WELD4},
     {1.0F, 0.3F, 0.3F, 0.961F, 0.03F, 0.3F, 0.9F, 8.0F, 0.08F, 0.3F, 0.12F}},
    {{CHARACTER_TAILS, {0.0F, 0.0F, 0.0F}, 1.3F, 0.3F, BUBBLES_AT_WELD4},
     {0.4F, 0.2F, 0.3F, 0.961F, 0.03F, 0.3F, 0.4F, 8.0F, 0.08F, 0.3F, 0.12F}},
    {{CHARACTER_BIG, {0.0F, 0.0F, 0.0F}, 2.5F, 0.3F, BUBBLES_AT_WELD5},
     {0.8F, 0.3F, 0.3F, 0.961F, 0.03F, 0.1F, 0.1F, 8.0F, 0.1F, 0.3F, 0.15F}},
    {{CHARACTER_BIG, {0.0F, 0.0F, 0.0F}, 2.5F, 0.5F, BUBBLES_AT_WELD5},
     {2.3F, 0.5F, 0.3F, 0.961F, 0.03F, 0.5F, 0.9F, 8.0F, 0.1F, 0.3F, 0.15F}},
    {{CHARACTER_BIG, {0.0F, 0.0F, 0.0F}, 2.5F, 0.5F, BUBBLES_AT_WELD5},
     {0.8F, 0.3F, 0.3F, 0.961F, 0.03F, 0.5F, 0.4F, 8.0F, 0.1F, 0.3F, 0.15F}},
    {{CHARACTER_SONIC, {0.0F, -2.0F, 0.0F}, -0.4F, -0.18F, BUBBLES_AT_WELD4},
     {0.4F, 0.2F, 0.3F, 0.961F, 0.03F, 0.06F, 0.1F, 8.0F, 0.1F, 0.3F, 0.12F}},
    {{CHARACTER_SONIC, {0.0F, -2.0F, 0.0F}, -0.4F, -0.3F, BUBBLES_AT_WELD4},
     {1.0F, 0.3F, 0.3F, 0.961F, 0.03F, 0.3F, 0.9F, 8.0F, 0.1F, 0.3F, 0.12F}},
    {{CHARACTER_SONIC, {0.0F, -2.0F, 0.0F}, -0.4F, -0.3F, BUBBLES_AT_WELD4},
     {0.4F, 0.2F, 0.3F, 0.961F, 0.03F, 0.3F, 0.4F, 8.0F, 0.1F, 0.3F, 0.12F}},
    {{CHARACTER_KNUCKLES, {0.0F, -1.2F, 0.0F}, -1.6F, -0.18F, BUBBLES_AT_WELD4},
     {0.4F, 0.2F, 0.3F, 0.961F, 0.03F, 0.06F, 0.1F, 8.0F, 0.1F, 0.3F, 0.12F}},
    {{CHARACTER_KNUCKLES, {0.0F, -1.2F, 0.0F}, -1.6F, -0.3F, BUBBLES_AT_WELD4},
     {1.0F, 0.3F, 0.3F, 0.961F, 0.03F, 0.3F, 0.9F, 8.0F, 0.1F, 0.3F, 0.12F}},
    {{CHARACTER_KNUCKLES, {0.0F, -1.2F, 0.0F}, -1.6F, -0.3F, BUBBLES_AT_WELD4},
     {0.4F, 0.2F, 0.3F, 0.961F, 0.03F, 0.3F, 0.4F, 8.0F, 0.1F, 0.3F, 0.12F}},
    {{CHARACTER_AMY, {0.0F, -0.9F, 0.0F}, -1.6F, -0.1F, BUBBLES_AT_WELD4},
     {0.3F, 0.15F, 0.3F, 0.961F, 0.03F, 0.1F, 0.1F, 8.0F, 0.1F, 0.3F, 0.12F}},
    {{CHARACTER_AMY, {0.0F, -0.9F, 0.0F}, -1.6F, -0.2F, BUBBLES_AT_WELD4},
     {1.0F, 0.3F, 0.3F, 0.961F, 0.03F, 0.3F, 0.9F, 8.0F, 0.1F, 0.3F, 0.12F}},
    {{CHARACTER_AMY, {0.0F, -0.9F, 0.0F}, -1.6F, -0.2F, BUBBLES_AT_WELD4},
     {0.3F, 0.1F, 0.3F, 0.961F, 0.03F, 0.3F, 0.3F, 8.0F, 0.1F, 0.3F, 0.12F}},
};

/* a bubble's sprite: all of the bubble texture */
static NJS_TEXANIM bubble_texanim = {
#include "player/effects/bubble_texanim.inc"
};

static NJS_TEXNAME son_eff_textures[SON_EFF_TEXTURES] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0},
};

static NJS_TEXLIST ball_textures[2] = {
#include "player/effects/ball_textures.inc"
};

/* a dash's texture for each step it draws: s_dash01, s_dash02, s_dash03 for
 * nine steps, then s_dash04 to s_dash12. the second set has s_dash01,
 * s_dash13, s_dash14 for eight steps, then s_dash15 to s_dash23. a dash
 * with the second set ends before it draws s_dash23 */
static NJS_TEXLIST dash_textures[DASH_STEPS] = {
#include "player/effects/dash_textures.inc"
};
static NJS_TEXLIST dash_textures_b[19] = {
#include "player/effects/dash_textures_b.inc"
};

/* the two looks of the light charge: lcharge01 and 03 for the trails, and
 * lcharge02 and 04 for the lights */
static NJS_TEXLIST trail_textures[2] = {
#include "player/effects/trail_textures.inc"
};
static NJS_TEXLIST charge_textures[2] = {
#include "player/effects/charge_textures.inc"
};

NJS_TEXLIST son_eff_texlist = {
#include "player/effects/son_eff_texlist.inc"
};

/* a light's sprite */
static NJS_TEXANIM charge_texanim = {
#include "player/effects/charge_texanim.inc"
};
static NJS_SPRITE charge_sprites[2] = {
#include "player/effects/charge_sprites.inc"
};

/* clang-format off */
static struct collision_shape charge_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_IGNORE_OTHERS,
    ATTACK(1) | GUARD(3) | SHAPE_HURTS_ENEMIES, PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F}, 2.0F, 0.0F, 0.0F, {0}, 0, NULL};
/* clang-format on */

/* the turn back from the player's middle to where it was a step before, at
 * the streak's scale. dash_step's second step turns the end of the
 * streak's copy at x 0 to it */
static NJS_MATRIX dash_matrix;

/* a new bubble at the head of work's list, or null when the heap is full */
static struct bubble *bubble_alloc(struct task_work *work)
{
    struct bubble *bubble = heap_alloc(sizeof(struct bubble));

    if (bubble != NULL) {
        bubble->prev = NULL;
        bubble->next = BUBBLES(work)->list;
        if (BUBBLES(work)->list != NULL) {
            BUBBLES(work)->list->prev = bubble;
        }
        BUBBLES(work)->list = bubble;
    }
    return bubble;
}

static void bubble_free(struct task_work *work, struct bubble *bubble)
{
    struct bubble *prev = bubble->prev;
    struct bubble *next = bubble->next;

    if (prev != NULL) {
        prev->next = next;
    } else {
        BUBBLES(work)->list = next;
    }
    if (next != NULL) {
        next->prev = prev;
    }
    heap_free(bubble);
}

/* carry pos, speed and, unless it is null, point from the player's own
 * frame into the world's. the player's frame is centered on its middle and
 * turned as it faces */
static void bubble_to_world(int player, NJS_POINT3 *pos, NJS_VECTOR *speed,
                            NJS_POINT3 *point)
{
    struct task_work *work = player_work[player];
    struct collision_shape *shape = work->collision->shapes;

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &shape->center);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, NJM_DEG_ANG(180) - work->ang.y);
    njCalcPoint(NULL, pos, pos);
    njCalcVector(NULL, speed, speed);
    if (point != NULL) {
        njCalcPoint(NULL, point, point);
    }
    njPopMatrix(1);
}

/* a new bubble's start and speed in the world, by work's source, and
 * unless point is null, the point it starts out from */
static void bubble_origin(struct task_work *work, NJS_POINT3 *pos,
                          NJS_VECTOR *speed, NJS_POINT3 *point)
{
    struct player *record = player_get(OWNER(work));
    Float *axis;

    switch (SOURCE(work)->start.kind) {
    case BUBBLES_AT_WELD4:
        *speed = record->weld_axis4;
        /* @hack spelling: the speed scaled through a Float walk, for the
         * image's @Rn+ loads; through the members, 124 of 379 words
         * differ */
        axis = &speed->x;
        *axis++ *= SOURCE(work)->start.speed;
        *axis++ *= SOURCE(work)->start.speed;
        *axis *= SOURCE(work)->start.speed;
        pos->x = SOURCE(work)->start.offset.x +
                 (SOURCE(work)->start.reach * record->weld_axis4.x) +
                 record->weld_point4.x;
        pos->y = SOURCE(work)->start.offset.y +
                 (SOURCE(work)->start.reach * record->weld_axis4.y) +
                 record->weld_point4.y;
        pos->z = SOURCE(work)->start.offset.z +
                 (SOURCE(work)->start.reach * record->weld_axis4.z) +
                 record->weld_point4.z;
        if (point != NULL) {
            *point = record->weld_point4;
        }
        break;
    case BUBBLES_AT_WELD5:
        *speed = record->weld_axis5;
        /* @hack spelling: the same walk; through the members, 126 of 379 words
         * differ */
        axis = &speed->x;
        *axis++ *= SOURCE(work)->start.speed;
        *axis++ *= SOURCE(work)->start.speed;
        *axis *= SOURCE(work)->start.speed;
        pos->x = SOURCE(work)->start.offset.x +
                 (SOURCE(work)->start.reach * record->weld_axis5.x) +
                 record->weld_point5.x;
        pos->y = SOURCE(work)->start.offset.y +
                 (SOURCE(work)->start.reach * record->weld_axis5.y) +
                 record->weld_point5.y;
        pos->z = SOURCE(work)->start.offset.z +
                 (SOURCE(work)->start.reach * record->weld_axis5.z) +
                 record->weld_point5.z;
        if (point != NULL) {
            *point = record->weld_point5;
        }
        break;
    default:
        *pos = SOURCE(work)->start.offset;
        pos->x += SOURCE(work)->start.reach;
        pos->y += record->consts.eye_height;
        speed->x = SOURCE(work)->start.speed;
        speed->y = 0.0F;
        speed->z = 0.0F;
        if (point != NULL) {
            point->x = 0.0F;
            point->y = pos->y;
            point->z = 0.0F;
        }
        break;
    }
    bubble_to_world(OWNER(work), pos, speed, point);
}

/* a new bubble from work's source, or null. it needs water above the
 * player as of the step before, and room on the heap. its start must lie
 * less than BUBBLE_DEPTH below the water's surface */
static struct bubble *bubble_create(struct task_work *work)
{
    Sint8 number = OWNER(work);
    struct player *record = player_get(number);
    struct bubble *bubble = NULL;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    Float size;

    if (!(record->shadow.last_above_flags & WORLD_WATER)) {
        return NULL;
    }
    bubble_origin(work, &pos, &speed, NULL);
    /* @hack spelling: bubble assigned in the test, which keeps pos.y, the
     * surface and bubble in the frame (0x8c0ba7f4, 0x8c0ba800, 0x8c0ba824).
     * in an if of its own, they take registers: the function runs 22 bytes
     * short and 274 of 292 words differ */
    if (pos.y < record->shadow.above &&
        pos.y + BUBBLE_DEPTH > record->shadow.above &&
        (bubble = bubble_alloc(work)) != NULL) {
        size =
            ((SOURCE(work)->motion.size_max - SOURCE(work)->motion.size_min) *
             (rand() * RAND_SCALE)) +
            SOURCE(work)->motion.size_min;
        speed.x +=
            ((rand() * RAND_SCALE) - 0.5F) * SOURCE(work)->motion.spread * 2.0F;
        speed.y +=
            ((rand() * RAND_SCALE) - 0.5F) * SOURCE(work)->motion.spread * 2.0F;
        speed.z +=
            ((rand() * RAND_SCALE) - 0.5F) * SOURCE(work)->motion.spread * 2.0F;
        bubble->pos = pos;
        bubble->speed = speed;
        /* any angle: up to three turns, which an Angle wraps */
        bubble->ang = (Angle)(rand() * RAND_SCALE * 1080.0F * ANGLE_PER_DEGREE);
        bubble->spin = (Angle)(((rand() * RAND_SCALE) - 0.5F) *
                               SOURCE(work)->motion.spin * ANGLE_PER_DEGREE);
        bubble->surface = record->shadow.above;
        bubble->aspect =
            1.0F - (rand() * SOURCE(work)->motion.squash * RAND_SCALE);
        bubble->size = size * 0.1F;
        bubble->max_size = size;
    }
    return bubble;
}

/* find the sources for player 0's character, unless the set editor holds
 * the task */
void bubbles_start(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack type: i unsigned, which SHC tests with CMP/HS (0x8c0baac2); as
     * an int, 1 of 63 words differ */
    Uint32 i;

    task->draw = bubbles_draw;
    task->step = bubbles_step;
    task->destructor = bubbles_destroy;
    BUBBLES(work)->list = NULL;
    work->submode = BUBBLES_STEADY;
    work->timer = 0;
    if (!placement_editing(task)) {
        work->tick = BUBBLES_UNDER_WATER;
        BUBBLES(work)->sources = NULL;
        for (i = 0; i < BUBBLE_SOURCES; i += CHARACTER_SOURCES) {
            if (player_get_character(0) == bubble_sources[i].start.character) {
                BUBBLES(work)->sources = &bubble_sources[i];
                return;
            }
        }
    }
}

static void bubbles_destroy(struct task *task)
{
    struct task_work *work = task->work;

    while (BUBBLES(work)->list != NULL) {
        bubble_free(work, BUBBLES(work)->list);
    }
}

/* make the player's bubbles each step, and move them once a frame. a burst
 * starts once the player's action is at frame 1 or less. under water, the
 * player tries for a bubble by chance on each step of a burst. drowning, it
 * tries on every step of a burst, and by chance between bursts. bubble_create
 * decides each try */
static void bubbles_step(struct task *task)
{
    struct task_work *work = task->work;
    struct bubble_source *source;
    struct bubble *bubble;
    struct bubble *next;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    NJS_POINT3 point;
    NJS_POINT3 center;
    Float *center_axis;
    Float *pos_axis;
    Float *speed_axis;
    char *center_byte;
    char *point_byte;
    char *pos_byte;
    Float ball_radius;
    Float cx;
    Float cy;
    Float cz;
    Float drag;
    Float lift;
    Float growth;
    Float wobble_rate;
    Float wobble;
    Float dx;
    Float dy;
    Float dz;
    Float px;
    Float py;
    Float pz;
    Float vx;
    Float vy;
    Float vz;
    Float distance2;
    Float radius;
    Float radius2;
    Float size;
    Float push;
    Float grown;
    Float room;
    Float y;
    Float limit;
    Float surface;

    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "player");
        return;
    }
    if (player_work[OWNER(work)] == NULL || SOURCE(work) == NULL) {
        task_kill(task);
        return;
    }
    if (player_work[OWNER(work)]->submode == PLAYER_MODE_DROWN) {
        if (work->submode != BUBBLES_STEADY) {
            work->tick = BUBBLES_DROWN_BURST;
        } else {
            work->tick = BUBBLES_DROWN_STEADY;
        }
    } else {
        work->tick = BUBBLES_UNDER_WATER;
    }
    if (work->submode != BUBBLES_STEADY) {
        if (work->tick != BUBBLES_UNDER_WATER ||
            rand() * RAND_SCALE < SOURCE(work)->motion.rate) {
            bubble_create(work);
        }
        if (TASK_TIMER(work)-- <= 0) {
            work->submode = BUBBLES_STEADY;
        }
    } else {
        if (work->tick != BUBBLES_UNDER_WATER &&
            rand() * RAND_SCALE < SOURCE(work)->motion.rate) {
            bubble_create(work);
        }
        if (!(player_get(OWNER(work))->anim.frame > 1.0F)) {
            work->submode = BUBBLES_BURSTING;
            work->timer = BURST_TIME;
        }
    }
    bubbles_draw(task);
    if (frame_step != 0) {
        return;
    }

    /* the ball the bubbles keep out of. its center mirrors their start, pos,
     * through point, so pos lies on its surface */
    bubble_origin(work, &pos, &speed, &point);
    /* @hack spelling: center as point less pos through byte cursors, for the
     * image's @Rn+ loads; through the members, the function runs 2 bytes
     * long and 300 of 482 words differ */
    center_byte = (char *)&center;
    point_byte = (char *)&point;
    pos_byte = (char *)&pos;
    *(Float *)center_byte = *(Float *)point_byte - *(Float *)pos_byte;
    center_byte += sizeof(Float);
    point_byte += sizeof(Float);
    pos_byte += sizeof(Float);
    *(Float *)center_byte = *(Float *)point_byte - *(Float *)pos_byte;
    center_byte += sizeof(Float);
    point_byte += sizeof(Float);
    pos_byte += sizeof(Float);
    *(Float *)center_byte = *(Float *)point_byte - *(Float *)pos_byte;
    /* @hack spelling: center doubled through a Float walk; through the members,
     * 300 of 482 words differ */
    center_axis = &center.x;
    *center_axis++ *= 2.0F;
    *center_axis++ *= 2.0F;
    *center_axis *= 2.0F;
    ball_radius = njScalor(&center);
    /* @hack spelling: pos added to center through byte cursors; through the
     * members, the function runs 6 bytes long and 267 of 482 words
     * differ */
    center_byte = (char *)&center;
    pos_byte = (char *)&pos;
    *(Float *)center_byte += *(Float *)pos_byte;
    center_byte += sizeof(Float);
    pos_byte += sizeof(Float);
    *(Float *)center_byte += *(Float *)pos_byte;
    center_byte += sizeof(Float);
    pos_byte += sizeof(Float);
    *(Float *)center_byte += *(Float *)pos_byte;
    cx = center.x;
    cy = center.y;
    cz = center.z;
    source = SOURCE(work);
    drag = source->motion.drag;
    lift = source->motion.lift;
    growth = source->motion.growth;
    wobble_rate = source->motion.wobble_rate;
    wobble = source->motion.wobble;

    for (bubble = BUBBLES(work)->list; bubble != NULL; bubble = next) {
        next = bubble->next;
        /* @hack spelling: the bubble's pos and speed walked as Floats, out and
         * back, for the image's @Rn+ loads and @-Rn stores; through the
         * members, 197 of 482 words differ */
        pos_axis = &bubble->pos.x;
        px = *pos_axis++;
        py = *pos_axis++;
        pz = *pos_axis;
        speed_axis = &bubble->speed.x;
        vx = *speed_axis++;
        vy = *speed_axis++;
        vz = *speed_axis;
        /* @hack spelling: py stepped first, then pz, then px, which keeps px,
         * py and pz in FR9, FR7 and FR8 (0x8c0bad66 to 0x8c0bad6e); in x, y
         * and z order, px and pz trade registers and 6 of 482 words
         * differ */
        py += vy;
        vy += lift;
        pz += vz;
        px += vx;
        vx *= drag;
        vz *= drag;
        vy *= drag;
        *speed_axis = vz;
        *--speed_axis = vy;
        *--speed_axis = vx;
        *pos_axis = pz;
        *--pos_axis = py;
        *--pos_axis = px;

        /* @hack spelling: size read ahead of the distance, which loads it into
         * FR5 at 0x8c0badce; read where radius takes it, 35 of 482 words
         * differ */
        size = bubble->size;
        /* @hack spelling: pos walked as Floats for the distance; through the
         * members, the function runs 2 bytes long and 174 of 482 words
         * differ */
        pos_axis = &bubble->pos.x;
        /* @hack spelling: cx and cy negated and added, which SHC negates once
         * ahead of the loop and keeps in the frame (FNEG at 0x8c0bad44 and
         * 0x8c0bad52); subtracted, the function runs 20 bytes short and 257
         * of 482 words differ */
        dx = -cx + *pos_axis++;
        dy = -cy + *pos_axis++;
        dz = *pos_axis - cz;
        distance2 = (dx * dx) + (dy * dy) + (dz * dz);
        radius = ball_radius + size;
        /* @hack spelling: radius squared through a local, which SHC computes
         * in FR5 (FMUL FR1,FR5 at 0x8c0badfe); in the test, the function
         * runs 2 bytes short and 159 of 482 words differ */
        radius2 = radius * radius;
        /* the ball pushes a bubble inside it out to its surface, but never
         * down */
        if (radius2 > distance2 && distance2 > 0.01F) {
            push = (radius + 0.1F) / njSqrt(distance2);
            dx *= push;
            dy *= push;
            dz *= push;
            /* @hack spelling: pos written through a Float walk, z through
             * pos_axis[1] past y; through the members, 142 of 482 words
             * differ */
            pos_axis = &bubble->pos.x;
            *pos_axis++ = cx + dx;
            if (dy >= 0.0F) {
                *pos_axis++ = cy + dy;
                *pos_axis = cz + dz;
            } else {
                pos_axis[1] = cz + dz;
            }
        }

        y = bubble->pos.y;
        limit = bubble->max_size;
        grown = bubble->size + growth;
        if (grown > limit) {
            grown = limit;
        }
        /* @hack spelling: the surface through a local, read once
         * (0x8c0bae6a), which keeps px in FR9, py in FR7, and cx and cy in
         * their frame slots. read twice, px and py trade registers, two
         * frame slots trade places, and 12 of 482 words differ */
        surface = bubble->surface;
        room = surface - grown - y;
        if (y >= surface) {
            bubble_free(work, bubble);
        } else {
            if (room < 0.0F) {
                grown += room;
            }
            bubble->size = grown;
        }

        /* @bug memory: a bubble that bursts is freed above (BSR to
         * bubble_free at 0x8c0bae7c), and its angle is still written here
         * (0x8c0baee4). its speed is too when the wobble's roll passes
         * (0x8c0baec2, 0x8c0baeda) */
        if (rand() * RAND_SCALE < wobble_rate) {
            /* @hack spelling: speed stepped to y and indexed on to z through a
             * Float walk; through the members, 203 of 482 words differ */
            speed_axis = &bubble->speed.x;
            *speed_axis++ += ((rand() * RAND_SCALE) - 0.5F) * wobble;
            speed_axis[1] += ((rand() * RAND_SCALE) - 0.5F) * wobble;
        }
        bubble->ang += bubble->spin;
    }
    work->timer++;
}

static void bubbles_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct bubble *bubble;
    NJS_SPRITE sprite;

    if (frame_step > 0) {
        return;
    }
    sprite.tlist = &eff_regular_texlist;
    sprite.tanim = &bubble_texanim;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    for (bubble = BUBBLES(work)->list; bubble != NULL; bubble = bubble->next) {
        sprite.ang = bubble->ang;
        sprite.p = bubble->pos;
        sprite.sx = bubble->size;
        sprite.sy = bubble->size * bubble->aspect;
        sprite_draw_3d(&sprite, 0,
                       NJD_SPRITE_ANGLE | NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
    }
}

void ball_jitter_start(struct task *task)
{
    ball_jitter_place(task->work);
    task->step = ball_jitter_step;
    task->draw = ball_jitter_draw;
    ball_jitter_draw(task);
}

void ball_follow_start(struct task *task)
{
    ball_follow_place(task->work);
    task->step = ball_follow_step;
    task->draw = ball_follow_draw;
    ball_follow_draw(task);
}

void ball_shrink_start(struct task *task)
{
    ball_shrink_place(task->work);
    task->step = ball_shrink_step;
    task->draw = ball_shrink_draw;
    ball_shrink_draw(task);
}

/* place a jitter ball as an offset from the player. it starts BALL_HEIGHT
 * up, turned as the player faces, then 0.2 to 1.2 further in a random
 * direction. TURN reads its angle twice, so each random turn draws one
 * rand for its test and another for its angle */
static void ball_jitter_place(struct task_work *work)
{
    struct task_work *player = player_work[BALL(work)->player];
    NJS_VECTOR vec;
    NJS_VECTOR offset;

    if (player != NULL) {
        vec.x = vec.z = 0.0F;
        vec.y = BALL_HEIGHT;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, player->ang.z);
        TURN(njRotateX, player->ang.x);
        TURN(njRotateY, -player->ang.y);
        njCalcVector(NULL, &vec, &work->pos);
        vec.x = vec.z = 0.0F;
        vec.y = (rand() * RAND_SCALE) + 0.2F;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, (Angle)RANDOM_ANGLE());
        TURN(njRotateX, (Angle)RANDOM_ANGLE());
        TURN(njRotateY, (Angle)RANDOM_ANGLE());
        njCalcVector(NULL, &vec, &offset);
        njPopMatrix(2);
        njAddVector(&work->pos, &offset);
    }
    if (rand() * RAND_SCALE > 0.5F) {
        BALL(work)->texlist = &ball_textures[0];
    } else {
        BALL(work)->texlist = &ball_textures[1];
    }
}

/* place a follow ball as an offset from the player's position, which scl
 * keeps. it starts BALL_HEIGHT up, turned as the player faces, then 1 to 3
 * further in a random direction */
static void ball_follow_place(struct task_work *work)
{
    struct task_work *player = player_work[BALL(work)->player];
    NJS_VECTOR vec;
    NJS_VECTOR offset;

    if (player != NULL) {
        vec.x = vec.z = 0.0F;
        vec.y = BALL_HEIGHT;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, player->ang.z);
        TURN(njRotateX, player->ang.x);
        TURN(njRotateY, -player->ang.y);
        njCalcVector(NULL, &vec, &work->pos);
        vec.x = vec.z = 0.0F;
        vec.y = RANDOM(1.0F, 3.0F);
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, (Angle)RANDOM_ANGLE());
        TURN(njRotateX, (Angle)RANDOM_ANGLE());
        TURN(njRotateY, player->ang.y);
        njCalcVector(NULL, &vec, &offset);
        njPopMatrix(2);
        njAddVector(&work->pos, &offset);
        work->scl = player->pos;
    }
    if (rand() * RAND_SCALE > 0.5F) {
        BALL(work)->texlist = &ball_textures[0];
    } else {
        BALL(work)->texlist = &ball_textures[1];
    }
}

/* place a shrink ball in the world, BALL_HEIGHT up from the player and
 * turned as the player faces, at three times its size */
static void ball_shrink_place(struct task_work *work)
{
    struct task_work *player = player_work[BALL(work)->player];
    NJS_VECTOR vec;

    if (player != NULL) {
        vec.x = vec.z = 0.0F;
        vec.y = BALL_HEIGHT;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, player->ang.z);
        TURN(njRotateX, player->ang.x);
        TURN(njRotateY, -player->ang.y);
        njCalcVector(NULL, &vec, &work->pos);
        njAddVector(&work->pos, &player->pos);
        njPopMatrix(1);
    }
    if (rand() * RAND_SCALE > 0.5F) {
        BALL(work)->texlist = &ball_textures[0];
    } else {
        BALL(work)->texlist = &ball_textures[1];
    }
    work->scl.x = 3.0F;
}

/* the ball with the player, turned as it faces, tipped and squashed, and
 * fading */
static void ball_jitter_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[BALL(work)->player];
    NJS_ARGB color;
    NJS_VECTOR vec;

    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_ALPHA);
    color.r = color.g = color.b = 0.0F;
    color.a = -(TASK_TIMER(work) * (1.0F / BALL_STEPS));
    njSetConstantMaterial(&color);
    njSetTexture(BALL(work)->texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x + player->pos.x, work->pos.y + player->pos.y,
                work->pos.z + player->pos.z);
    TURN(njRotateZ, player->ang.z);
    TURN(njRotateX, player->ang.x);
    TURN(njRotateY, -player->ang.y);
    vec.x = 0.0F;
    vec.y = -1.0F;
    vec.z = 0.0F;
    njTranslateV(NULL, &vec);
    njRotateZ(NULL, NJM_DEG_ANG(-45));
    vec.x = 0.7F;
    vec.y = 1.1F;
    vec.z = 0.8F;
    njScaleV(NULL, &vec);
    draw_tree(&sonic_ball_object);
    njPopMatrix(1);
    color.a = 0.0F;
    njSetConstantMaterial(&color);
    draw_attr_restore();
}

/* draw the ball for BALL_STEPS steps, then end */
static void ball_jitter_step(struct task *task)
{
    struct task_work *work = task->work;

    if (TASK_TIMER(work)++ >= BALL_STEPS) {
        task_kill(task);
    } else {
        ball_jitter_draw(task);
    }
}

/* the ball where the player was, turned as it faced, and fading */
static void ball_follow_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;

    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_ALPHA);
    color.r = color.g = color.b = 0.0F;
    color.a = -(TASK_TIMER(work) * (1.0F / BALL_STEPS));
    njSetConstantMaterial(&color);
    njSetTexture(BALL(work)->texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x + work->scl.x, work->pos.y + work->scl.y,
                work->pos.z + work->scl.z);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, -work->ang.y);
    draw_tree(&sonic_ball_object);
    njPopMatrix(1);
    color.a = 0.0F;
    njSetConstantMaterial(&color);
    draw_attr_restore();
}

/* draw the ball where the player was a step before, for BALL_STEPS steps,
 * then end */
static void ball_follow_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[BALL(work)->player];

    if (TASK_TIMER(work)++ >= BALL_STEPS) {
        task_kill(task);
        return;
    }
    ball_follow_draw(task);
    work->scl = player->pos;
    work->ang = player->ang;
}

/* the ball where it was left, at its size, and fading */
static void ball_shrink_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;

    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_ALPHA);
    color.r = color.g = color.b = 0.0F;
    color.a = -(TASK_TIMER(work) * (1.0F / SHRINK_STEPS));
    njSetConstantMaterial(&color);
    njSetTexture(BALL(work)->texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njScale(NULL, work->scl.x, work->scl.x, work->scl.x);
    draw_tree(&sonic_ball_object);
    njPopMatrix(1);
    color.a = 0.0F;
    njSetConstantMaterial(&color);
    draw_attr_restore();
}

/* draw the ball for SHRINK_STEPS steps, shrinking it after each, then end */
static void ball_shrink_step(struct task *task)
{
    struct task_work *work = task->work;

    if (TASK_TIMER(work)++ >= SHRINK_STEPS) {
        task_kill(task);
        return;
    }
    ball_shrink_draw(task);
    work->scl.x -= 0.4F;
}

/* set ang[1] and ang[2] to the angles about y and z that turn the x axis
 * toward vector, and clear ang[0]. a vector of no length leaves them */
/* @hack type: ang an Angle *, which callers point at a struct angles' x and
 * which reaches its other members by index, as ISO C leaves undefined.
 * dash_step then keeps &aim.x in R12 and reads aim through it (0x8c0bbdde,
 * 0x8c0bbdf2). as a struct angles *, SHC builds the address in R5, and 61
 * of 412 words differ, all of them dash_step's */
static void dash_angles(NJS_VECTOR *vector, Angle *ang)
{
    Float length = njScalor(vector);

    if (length != 0.0F) {
        ang[1] = (Angle)(atan2f(-vector->z, vector->x) * ANGLE_PER_RADIAN);
        ang[2] = (Angle)(asinf(vector->y / length) * ANGLE_PER_RADIAN);
        ang[0] = 0;
    }
}

void dash_start(struct task *task)
{
    if (dash_alloc(task->work) == TRUE) {
        task->step = dash_step;
        task->draw = dash_draw;
        task->destructor = dash_destroy;
    } else {
        task_kill(task);
    }
}

/* copy the streak, its model and its points onto the heap, with the copy's
 * object in the dash's vars. start the dash at the player's middle. FALSE if
 * the heap has no room, with none of the copy left allocated */
static Bool dash_alloc(struct task_work *work)
{
    NJS_OBJECT *object = &sonic_dash_object;
    Uint32 count = object->model->nbPoint;
    NJS_OBJECT *copy;
    NJS_POINT3 *to;
    NJS_POINT3 *from;
    Uint32 i;
    struct task_work *player;
    struct collision_shape *shape;

    copy = heap_alloc(sizeof(NJS_OBJECT));
    /* @hack spelling: an else that returns FALSE, not an early return, which
     * puts the model copy's source in R2 and its destination in R3
     * (0x8c0bb960, 0x8c0bb962); with an early return, the two trade and 22
     * of 144 words differ */
    if (copy != NULL) {
        *copy = *object;
        DASH(work)->object = copy;
    } else {
        return FALSE;
    }
    if ((copy->model = heap_alloc(sizeof(NJS_MODEL))) != NULL) {
        *copy->model = *object->model;
    } else {
        heap_free(copy);
        return FALSE;
    }
    if ((copy->model->points = heap_alloc(count * sizeof(NJS_POINT3))) !=
        NULL) {
        to = copy->model->points;
        from = object->model->points;
        for (i = 0; i < count; i++) {
            VECTOR_COPY(to, from);
            to++;
            from++;
        }
    } else {
        heap_free(copy->model);
        heap_free(copy);
        return FALSE;
    }
    player = player_work[DASH(work)->player];
    if (player != NULL) {
        shape = player->collision->shapes;
        work->pos = shape->center;
    }
    return TRUE;
}

/* the streak, its first step as the model has it, and its copy after */
static void dash_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = DASH(work)->object;

    if (work->flags & DASH_SECOND_SET) {
        njSetTexture(&dash_textures_b[TASK_TIMER(work) - 1]);
    } else {
        njSetTexture(&dash_textures[TASK_TIMER(work) - 1]);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, work->ang.z);
    njScaleV(NULL, &work->scl);
    if (TASK_TIMER(work) == 1) {
        draw_tree(&sonic_dash_object);
    } else {
        draw_tree(object);
    }
    njPopMatrix(1);
}

/* draw the dash for DASH_STEPS steps, or with the second set until step
 * SECOND_SET_END. the first step stretches the streak from the player's
 * middle back to where the dash started. it takes the second set if the
 * record's own.sonic.dashes shows a dash two steps back but none one step
 * back. the second step turns the copy's end at x 0, its odd points, to
 * dash_matrix */
static void dash_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = DASH(work)->object;
    NJS_VECTOR vec;
    NJS_VECTOR offset;
    struct angles aim;

    /* @hack redundant: the second test an else after the first's return, which
     * loads its SECOND_SET_END into R2 (0x8c0bbb88); without the else, R3,
     * and 2 of 370 words differ */
    if (TASK_TIMER(work)++ >= DASH_STEPS) {
        task_kill(task);
        return;
    } else if ((work->flags & DASH_SECOND_SET) &&
               TASK_TIMER(work) >= SECOND_SET_END) {
        task_kill(task);
        return;
    }
    if (TASK_TIMER(work) == 1) {
        struct task_work *player = player_work[DASH(work)->player];
        struct player *record;

        if (player != NULL) {
            vec.x = vec.z = 0.0F;
            vec.y = player_get(DASH(work)->player)->consts.center_y;
            njPushMatrix(&_unit_matrix_);
            TURN(njRotateZ, player->ang.z);
            TURN(njRotateX, player->ang.x);
            TURN(njRotateY, -player->ang.y);
            njCalcVector(NULL, &vec, &offset);
            njPopMatrix(1);
            njAddVector(&offset, &player->pos);
            vec = work->pos;
            work->pos = offset;
            njSubVector(&vec, &offset);
            dash_angles(&vec, &work->ang.x);
            work->scl.x = njScalor(&vec) * (1.0F / STREAK_LENGTH);
            work->scl.y = 1.0F;
            work->scl.z = 1.0F;
            record = player_motion[DASH(work)->player]->record;
            if ((record->own.sonic.dashes[0] & 4) &&
                !(record->own.sonic.dashes[0] & 2)) {
                work->flags |= DASH_SECOND_SET;
            }
        }
    } else if (TASK_TIMER(work) == 2) {
        struct task_work *player = player_work[DASH(work)->player];
        /* @hack spelling: the streak read through original, which loads its
         * address into R4 and its model through it (0x8c0bbd3c, 0x8c0bbd40).
         * read as sonic_dash_object, the pool holds its model field's address,
         * and 6 of 370 words differ */
        NJS_OBJECT *original;
        NJS_POINT3 *point;
        Uint32 count;
        Uint32 i;

        if (player != NULL) {
            original = &sonic_dash_object;
            point = object->model->points;
            count = original->model->nbPoint;
            vec.x = vec.z = 0.0F;
            vec.y = player_get(DASH(work)->player)->consts.center_y;
            njPushMatrix(&_unit_matrix_);
            TURN(njRotateZ, player->ang.z);
            TURN(njRotateX, player->ang.x);
            TURN(njRotateY, -player->ang.y);
            njCalcVector(NULL, &vec, &offset);
            njPopMatrix(1);
            njAddVector(&offset, &player->pos);
            vec = work->pos;
            njSubVector(&vec, &offset);
            if (njScalor(&vec) != 0.0F) {
                dash_angles(&vec, &aim.x);
                njUnitMatrix(&dash_matrix);
                TURN_MATRIX(njRotateY, &dash_matrix, aim.y);
                TURN_MATRIX(njRotateZ, &dash_matrix, aim.z);
                njScaleV(&dash_matrix, &work->scl);
                njPushMatrix(&_unit_matrix_);
                TURN(njRotateY, work->ang.y);
                TURN(njRotateZ, work->ang.z);
                njScaleV(NULL, &work->scl);
                njInvertMatrix(NULL);
                njMultiMatrix(NULL, &dash_matrix);
                for (i = 0, point++; i < count; i += 2, point += 2) {
                    vec = *point;
                    njCalcPoint(NULL, &vec, point);
                }
                njPopMatrix(1);
            }
        }
    }
    dash_draw(task);
}

/* free the dash's copy: its points, its model and its object. the model and the
 * object are each read through before their NULL tests. no test can fail:
 * dash_start sets this destructor only once dash_alloc has the whole copy */
static void dash_destroy(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = DASH(work)->object;

    if (object->model->points != NULL) {
        heap_free(object->model->points);
    }
    if (object->model != NULL) {
        heap_free(object->model);
    }
    if (object != NULL) {
        heap_free(object);
    }
}

/* while the record blends, the aura draws the blend in place of the action. its
 * first color's green falls from 0.28 to 0.1 and rises back over each 128 steps
 * of play time */
void aura_draw(struct task_work *work, struct task_motion *motion,
               struct player *record)
{
    NJS_ACTION action;
    NJS_ARGB color;
    int index;
    Float phase;
    /* @hack spelling: the scale through three locals, which SHC copies
     * from FR15 to FR6 first (0x8c0bc05c, 0x8c0bc08c); passed as constants,
     * FR4 comes first and 4 of 157 words differ */
    Float sx;
    Float sy;
    Float sz;

    if (frame_step != 0) {
        return;
    }
    action.object = AURA_MODEL;
    index = record->anim.action;
    /* @hack redundant: index picked while the record blends, where nothing
     * reads it (0x8c0bbf8e, 0x8c0bbf90); without it, 135 of 157 words differ */
    if (record->anim.state == PLAYER_ANIM_BLENDING) {
        index = record->anim.current;
    }
    njPushMatrix(NULL);
    if (record->anim.state != PLAYER_ANIM_BLENDING) {
        action.motion = record->anim.actions[index].action->motion;
    } else {
        action.motion = record->anim.blend->motion;
    }
    phase = (Float)(play_time_get() & 0x7f);
    if (phase >= 64.0F) {
        phase = 128.0F - phase;
    }
    phase *= 1.0F / 64.0F;
    color.r = 0.0F;
    color.g = 0.28F - (phase * 0.18F);
    color.b = 1.0F;
    color.a = 0.0F;
    njSetConstantMaterial(&color);
    njSimpleAction(&action, record->anim.frame);
    color.r = 0.05F;
    /* @hack spelling: green less 0.15 as -0.15 added, FADD at 0x8c0bc04e; with
     * -=, SHC subtracts, and 1 of 157 words differ */
    color.g += -0.15F;
    njSetConstantMaterial(&color);
    sx = 1.05F;
    sy = 1.05F;
    sz = 1.05F;
    njScale(NULL, sx, sy, sz);
    njSimpleAction(&action, record->anim.frame);
    color.g -= 0.05F;
    color.b = 0.3F;
    njSetConstantMaterial(&color);
    sx = 1.05F;
    sy = 1.05F;
    sz = 1.05F;
    njScale(NULL, sx, sy, sz);
    njSimpleAction(&action, record->anim.frame);
    color.b = color.g = color.r = 0.0F;
    njSetConstantMaterial(&color);
    njPopMatrix(1);
}

/* draw the strip, once it has a triangle, in the look work->tick picks.
 * it draws only on a frame's first step */
static void trail_draw(struct task *task)
{
    /* @hack redundant: work set and never read, which keeps task in R12 for
     * the texture's lookup (0x8c0bc128, 0x8c0bc14a); without it, task waits
     * at @R15, the function runs 4 bytes long and 49 of 50 words differ */
    struct task_work *work = task->work;
    NJS_POINT3COL *strip = STRIP(task);

    if (strip->num >= 3 && frame_step == 0) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njSetTexture(&trail_textures[task->work->tick]);
        njSetTextureNum(0);
        njDrawTriangle3D(strip, strip->num - 2,
                         NJD_USE_TEXTURE | NJD_TRANSPARENT | NJD_FILL |
                             NJD_DRAW_CONNECTED);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_BOTHSRCALPHA);
    }
}

/* add up to steps points toward the target in scl, each a share of the
 * way left. it stops when the strip is full or a step falls to TRAIL_STOP.
 * the points fall on alternate sides of the trail's line, which rolls as
 * it goes */
static void trail_extend(struct task_work *work, NJS_POINT3COL *strip,
                         int steps)
{
    NJS_POINT3 *points;
    NJS_COLOR *colors;
    NJS_COLOR *uvs;
    int i;
    int j;
    int count;
    Float side;
    Float step;
    Angle pitch;
    Angle yaw;
    NJS_VECTOR vec;
    NJS_POINT3 point;

    points = strip->p;
    colors = strip->col;
    uvs = strip->tex;
    i = strip->num;
    count = TRAIL(work)->count;
    if (i == count) {
        return;
    }
    side = (i & 1) ? 0.4F : -0.4F;
    for (j = 0; j < steps; j++) {
        vec.x = work->scl.x - work->pos.x;
        vec.y = work->scl.y - work->pos.y;
        vec.z = work->scl.z - work->pos.z;
        step = njScalor(&vec) / (steps - j);
        if (step <= TRAIL_STOP) {
            return;
        }
        pitch = -(Angle)(asinf(vec.y / step) * ANGLE_PER_RADIAN);
        yaw = (Angle)(atan2f(vec.x, vec.z) * ANGLE_PER_RADIAN);
        work->ang.x = pitch;
        work->ang.y = yaw;
        work->ang.z += 0x180;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, work->ang.y);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateZ, work->ang.z);
        vec.x = 0.0F;
        vec.y = 0.0F;
        vec.z = step;
        njCalcPoint(NULL, &vec, &point);
        work->pos.x += point.x;
        work->pos.y += point.y;
        work->pos.z += point.z;
        vec.x = side;
        vec.y = 0.0F;
        vec.z = step;
        njCalcPoint(NULL, &vec, &point);
        point.x += work->pos.x;
        point.y += work->pos.y;
        point.z += work->pos.z;
        points[i].x = point.x;
        points[i].y = point.y;
        points[i].z = point.z;
        side = -side;
        njPopMatrix(1);
        colors[i].color = TRAIL(work)->color.color;
        /* u runs along the trail: 0 at its first two points and 255 at its last
         * two. between them, u is 112 for a pair of points, then 144 for a
         * pair, in turn. v crosses the trail: 0 at an even point and 255 at an
         * odd one, but the last two take 0 then 255 */
        if (i == 1) {
            uvs[i].tex.u = 0;
            uvs[i].tex.v = 255;
        } else if (i == count - 1) {
            uvs[i].tex.u = 255;
            uvs[i].tex.v = 255;
        } else if (i == count - 2) {
            uvs[i].tex.u = 255;
            uvs[i].tex.v = 0;
        } else if (i % 4 == 2) {
            uvs[i].tex.u = 112;
            uvs[i].tex.v = 0;
        } else if (i % 4 == 3) {
            uvs[i].tex.u = 112;
            uvs[i].tex.v = 255;
        } else if (i % 4 == 0) {
            uvs[i].tex.u = 144;
            uvs[i].tex.v = 0;
        } else {
            uvs[i].tex.u = 144;
            uvs[i].tex.v = 255;
        }
        strip->num = ++i;
        if (i >= count) {
            break;
        }
    }
}

/* free the strip's points, colors and uvs. the arm that frees tex sets col to
 * NULL, not tex, so tex keeps its freed address */
static void trail_destroy(struct task *task)
{
    NJS_POINT3COL *strip;

    strip = STRIP(task);
    if (strip->p != NULL) {
        heap_free(strip->p);
        strip->p = NULL;
    }
    if (strip->col != NULL) {
        heap_free(strip->col);
        strip->col = NULL;
    }
    if (strip->tex != NULL) {
        heap_free(strip->tex);
        strip->col = NULL;
    }
}

/* fade each point until none is left to fade, then end */
static void trail_fade(struct task *task)
{
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    struct task_work *work;
    NJS_POINT3COL *strip;
    NJS_COLOR *color;
    int i;
    Bool fading;

    /* @hack redundant: work set and never read, which leaves fading in R5
     * and the constant 1 in R7 (0x8c0bc512, 0x8c0bc534); without it, the
     * two trade registers and 5 of 40 words differ */
    work = task->work;
    strip = STRIP(task);
    color = strip->col;
    trail_draw(task);
    fading = FALSE;
    for (i = strip->num; i > 0; i--) {
        if (color->argb.a > 8) {
            color->argb.a -= 5;
            fading = TRUE;
        }
        color++;
    }
    if (!fading) {
        task_kill(task);
    }
}

/* add a point toward what the trail follows, and fade once a step adds
 * none or the strip is full */
static void trail_step(struct task *task)
{
    struct task_work *work;
    NJS_POINT3COL *strip;
    Uint32 last;
    /* @hack spelling: the target through a local, which SHC copies through
     * R4 (0x8c0bc562 to 0x8c0bc570); read in place, through R3 and R2, and 9
     * of 38 words differ */
    NJS_POINT3 *follow;

    work = task->work;
    strip = STRIP(task);
    follow = TRAIL(work)->follow;
    work->scl = *follow;
    last = strip->num;
    trail_extend(work, strip, 1);
    trail_draw(task);
    if (strip->num == last || strip->num >= TRAIL(work)->count) {
        task->step = trail_fade;
    }
}

/* make the strip with room for count points: the first at work->pos, and
 * two more toward what the trail follows. the task ends if the heap has no
 * room, or if the strip has fewer than three points or no room for more */
static void trail_start(struct task *task)
{
    struct task_work *work;
    NJS_POINT3COL *strip;
    NJS_POINT3 *points;
    NJS_COLOR *colors;
    NJS_COLOR *uvs;
    int count;
    int size;
    NJS_POINT3 *follow;
    NJS_VECTOR vec;

    /* @hack redundant: count's and size's addresses taken and dropped, which
     * keeps each in a frame slot of its own, count at @R15 and size at
     * @(4,R15) (0x8c0bc5ce, 0x8c0bc5e4); without them, count takes R8, the
     * frame shrinks from 28 bytes to 24 and 45 of 185 words differ */
    (void)&size;
    (void)&count;
    work = task->work;
    if ((task->motion = heap_alloc(sizeof(NJS_POINT3COL))) != NULL) {
        strip = STRIP(task);
        count = TRAIL(work)->count;
        if ((points = heap_alloc(count * sizeof(NJS_POINT3))) == NULL) {
            task_kill(task);
            return;
        }
        size = count * sizeof(NJS_COLOR);
        if ((colors = heap_alloc(size)) == NULL) {
            heap_free(points);
            task_kill(task);
            return;
        }
        if ((uvs = heap_alloc(size)) == NULL) {
            heap_free(colors);
            heap_free(points);
            task_kill(task);
            return;
        }
        strip->p = points;
        strip->col = colors;
        strip->tex = uvs;
        strip->num = 1;
        VECTOR_COPY(&points[0], &work->pos);
        colors[0].color = TRAIL(work)->color.color;
        uvs[0].tex.u = 0;
        uvs[0].tex.v = 0;
        follow = TRAIL(work)->follow;
        work->scl = *follow;
        vec.x = work->scl.x - work->pos.x;
        vec.y = work->scl.y - work->pos.y;
        vec.z = work->scl.z - work->pos.z;
        work->ang.x =
            (Angle)(atan2f(vec.y, njSqrt((vec.x * vec.x) + (vec.z * vec.z))) *
                    ANGLE_PER_RADIAN);
        work->ang.y = (Angle)(atan2f(vec.x, vec.z) * ANGLE_PER_RADIAN);
        trail_extend(work, strip, 2);
        task->step = trail_step;
        task->draw = trail_draw;
        task->destructor = trail_destroy;
        if (strip->num < 3 || strip->num >= TRAIL(work)->count) {
            task_kill(task);
            return;
        }
        trail_draw(task);
    } else {
        task_kill(task);
    }
}

void trail_create(NJS_POINT3 *follow, int count)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 3, trail_start)) != NULL) {
        TRAIL(task->work)->follow = follow;
        TRAIL(task->work)->count = count;
        TRAIL(task->work)->color.color = 0xffffffff;
        task->work->pos = *follow;
    }
}

/* fly on at its speed, which it keeps in scl, until its count runs out */
static void charge_spark_step(struct task *task)
{
    struct task_work *work;

    work = task->work;
    if (SPARK(work)->count-- <= 0) {
        task_kill(task);
        return;
    }
    VECTOR_ADD(&work->pos, &work->scl);
}

void charge_spark_create(struct task_work *work, int count)
{
    struct task *spark;
    struct task *trail;

    if ((spark = task_create(TASK_WORK, 2, charge_spark_step)) != NULL) {
        spark->work->pos = work->pos;
        SPARK(spark->work)->count = count;
        spark->work->scl.x = ((rand() * RAND_SCALE) - 0.5F) * SPARK_SPREAD;
        spark->work->scl.y = ((rand() * RAND_SCALE) - 0.5F) * SPARK_SPREAD;
        spark->work->scl.z = ((rand() * RAND_SCALE) - 0.5F) * SPARK_SPREAD;
    } else {
        /* @hack spelling: an else that returns, not an early return, which
         * loads task_create's address below into R2 (0x8c0bc87c); with an early
         * return, R3, the function runs 2 bytes long and 80 of 151 words
         * differ */
        return;
    }
    if ((trail = task_create(TASK_WORK, 3, trail_start)) != NULL) {
        TRAIL(trail->work)->follow = &spark->work->pos;
        TRAIL(trail->work)->count = count;
        /* a random light color, opaque */
        TRAIL(trail->work)->color.argb.b = RANDOM(127.0F, 254.0F);
        TRAIL(trail->work)->color.argb.g = RANDOM(127.0F, 254.0F);
        TRAIL(trail->work)->color.argb.r = RANDOM(127.0F, 254.0F);
        TRAIL(trail->work)->color.argb.a = -1;
        trail->work->pos = spark->work->pos;
    } else {
        task_kill(spark);
    }
}

/* the light, facing the camera and shrinking as its time runs out */
static void charge_draw(struct task *task)
{
    struct task_work *work;
    Float scale;

    work = task->work;
    if (frame_step == 0) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        scale = CHARGE(work)->timer * (1.0F / 32.0F);
        TURN(njRotateY, camera_work->ang.y);
        TURN(njRotateX, camera_work->ang.x);
        njScale(NULL, scale, scale, scale);
        njFogDisable();
        njDrawSprite3D(&charge_sprites[work->tick], 0, NJD_SPRITE_ALPHA);
        njFogEnable();
        njPopMatrix(1);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
    }
}

/* turn toward the player's middle, which scl keeps, and fly at it until
 * the time runs out. a step flies at most CHARGE_SPEED */
static void charge_step(struct task *task)
{
    struct task_work *work;
    struct task_work *player;
    Angle pitch;
    Angle yaw;
    NJS_VECTOR delta;
    NJS_POINT3 step;
    Float distance;

    work = task->work;
    if (CHARGE(work)->timer-- > 0) {
        player = player_work[CHARGE(work)->player];
        if (player != NULL) {
            work->scl = player->collision->shapes->center;
        }
        delta.x = work->scl.x - work->pos.x;
        delta.y = work->scl.y - work->pos.y;
        delta.z = work->scl.z - work->pos.z;
        distance = njScalor(&delta);
        if (distance == 0.0F) {
            return;
        }
        pitch = -(Angle)(asinf(delta.y / distance) * ANGLE_PER_RADIAN);
        yaw = (Angle)(atan2f(delta.x, delta.z) * ANGLE_PER_RADIAN);
        work->ang.x = angle_approach(work->ang.x, pitch, 0x400);
        work->ang.y = angle_approach(work->ang.y, yaw, 0x700);
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, work->ang.y);
        TURN(njRotateX, work->ang.x);
        if (distance > CHARGE_SPEED) {
            distance = CHARGE_SPEED;
        }
        delta.x = 0.0F;
        delta.y = 0.0F;
        delta.z = distance;
        njCalcPoint(NULL, &delta, &step);
        work->pos.x += step.x;
        work->pos.y += step.y;
        work->pos.z += step.z;
        njPopMatrix(1);
        charge_draw(task);
        collision_register(work);
    } else {
        task_kill(task);
    }
}

/* the light starts at a random heading, pitched between 11.25 degrees below
 * level and about 56 above. the crystal ring picks its look and its trail's */
void charge_create(struct task_work *work, int count)
{
    struct player *record;
    int look;
    struct task *charge;
    struct task *trail;
    struct task_work *light;
    struct task_work *trail_work;
    NJS_POINT3 *center;
    Float distance;
    /* @hack type: pitch and yaw Floats, though each holds an Angle, which
     * SHC converts to a float and back for njSin and njCos (0x8c0bccd8,
     * 0x8c0bccf4, 0x8c0bcd08); as Angles, the function runs 32 bytes short
     * and 132 of 185 words differ */
    Float pitch;
    Float yaw;

    record = player_get(PLAYER_VARS(work)->player);
    if (record != NULL && (record->upgrades & PLAYER_SONIC_CRYSTAL_RING)) {
        look = 1;
    } else {
        look = 0;
    }
    if ((charge = task_create(TASK_WORK, 2, charge_step)) != NULL) {
        center = &work->collision->shapes->center;
        light = charge->work;
        CHARGE(light)->player = PLAYER_VARS(work)->player;
        light->scl = *center;
        distance = RANDOM(25.0F, 40.0F);
        pitch = (Angle)(rand() * 0.375F) - NJM_DEG_ANG(11.25);
        yaw = (Angle)RANDOM_ANGLE();
        light->pos.x = center->x + (distance * njSin(yaw) * njCos(pitch));
        light->pos.z = center->z + (distance * njCos(yaw) * njCos(pitch));
        light->pos.y = center->y + (distance * njSin(pitch));
        light->ang.y = NJM_DEG_ANG(180) - work->ang.y;
        CHARGE(light)->timer = count + 1;
        light->tick = look;
    } else {
        /* @hack spelling: an else that returns, not an early return, which
         * loads task_create's address below into R2 (0x8c0bcd50); with an early
         * return, R3, and 19 of 185 words differ */
        return;
    }
    if ((trail = task_create(TASK_WORK, 3, trail_start)) != NULL) {
        trail_work = trail->work;
        TRAIL(trail_work)->follow = &charge->work->pos;
        TRAIL(trail_work)->count = count;
        TRAIL(trail_work)->color.color = 0xffffffff;
        trail_work->pos = charge->work->pos;
        trail_work->tick = look;
    } else {
        task_kill(charge);
    }
    collision_init(charge, &charge_shape, COLLISION_ATTACK, 1, 1);
    charge->draw = charge_draw;
}

static void ice_draw(struct task *task)
{
    struct task_work *work;

    work = task->work;
    njSetTexture(ICE(work)->texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njScaleV(NULL, &work->scl);
    draw_tree(ICE(work)->object);
    njPopMatrix(1);
}

static void ice_destroy(struct task *task)
{
}

/* fly out along its heading and fall, tumbling. scl.y holds the piece's
 * upward speed, so ice_draw scales the piece by it too */
static void ice_piece_step(struct task *task)
{
    struct task_work *work;

    work = task->work;
    if (placement_try_leave(task)) {
        return;
    }
    work->pos.x += njSin(work->ang.y) * 2.0F;
    work->pos.z += njCos(work->ang.y) * 2.0F;
    work->pos.y += work->scl.y;
    work->scl.y -= 0.1F;
    if (work->scl.y < -6.0F) {
        work->scl.y = -6.0F;
    }
    work->ang.x += 0x100;
    ice_draw(task);
}

/* a piece at a random size and turn */
static void ice_piece_start(struct task *task)
{
    struct task_work *work;
    Float random;

    work = task->work;
    player_get(work->tick);
    random = rand() * RAND_SCALE;
    work->scl.x = random + 0.5F;
    work->ang.x = (Angle)(random * 65536.0F);
    random = rand() * RAND_SCALE;
    work->scl.z = random + 0.5F;
    work->ang.z = (Angle)(random * 65536.0F);
    task->draw = ice_draw;
    task->step = ice_piece_step;
    task->destructor = ice_destroy;
    ICE(work)->object = &sonic_ice_piece_object;
    ICE(work)->texlist = &sonic_ice_texlist;
}

/* the pieces fly off at random headings, rising up to 2 a step */
static void ice_break(struct task_work *work)
{
    struct task *piece;
    int i;

    for (i = 0; i < ICE_PIECES; i++) {
        piece = task_create(TASK_WORK, 6, ice_piece_start);
        piece->work->pos = work->pos;
        piece->work->ang.y = (Angle)RANDOM_ANGLE();
        piece->work->scl.y = rand() * (2.0F * RAND_SCALE);
        piece->work->tick = work->tick;
    }
}

/* stay at the player's middle until its action changes, then break */
static void ice_step(struct task *task)
{
    struct task_work *work;
    Uint8 player;
    struct player *record;

    work = task->work;
    player = work->tick;
    record = player_get(player);
    if (record->anim.action != TASK_TIMER(work)) {
        ice_break(work);
        task_kill(task);
        return;
    }
    work->pos = player_work[player]->collision->shapes->center;
    ice_draw(task);
}

/* the ice's first step. it keeps the player's action in work->timer, and
 * ice_step breaks the ice once the action changes */
static void ice_start(struct task *task)
{
    struct task_work *work;
    struct player *record;

    work = task->work;
    record = player_get(work->tick);
    work->timer = record->anim.action;
    work->scl.x = work->scl.y = work->scl.z = 1.0F;
    task->draw = ice_draw;
    task->step = ice_step;
    task->destructor = ice_destroy;
    ICE(work)->object = &sonic_ice_object;
    ICE(work)->texlist = &sonic_ice_texlist;
}

void ice_create(int player)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 3, ice_start)) != NULL) {
        task->work->tick = player;
    }
}
