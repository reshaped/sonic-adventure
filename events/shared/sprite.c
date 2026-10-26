/* an event's sprites: texlists drawn as additive 3d sprites, each a task in
 * a slot. a looping sprite stays where it starts and cycles eight frames. a
 * following sprite keeps to a task as it grows and spins, and moves through
 * every frame */
#include <ninja.h>
#include <sg_xpt.h>

#include "player/player.h"
#include "shared/sprite.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

#define TEXT_1_LENGTH 14
#define TEXT_2_LENGTH 14
#define TEXT_3_LENGTH 14
#define TEXT_4_LENGTH 14
#define TEXT_5_LENGTH 14
#define TEXT_6_LENGTH 14
#define TEXT_7_LENGTH 14
#define TEXT_8_LENGTH 14
#include "shared/sprite/text.inc"

/* the frames a looping sprite cycles, a power of two for the mask that
 * wraps them. its timer runs 1 to 8, and the mask shows 8 as frame 0 */
#define SPRITE_LOOP_FRAMES 8

/* a following sprite's record, from the heap, which the destructor frees */
struct sprite_follow {
    struct task *base; /* the task it keeps to, or null */
    NJS_ARGB color;
    Angle spin; /* added to its angle each step */
    Float grow_x, grow_y; /* added to its scale each step */
    Float speed; /* added to time each step */
    Float time; /* at 1 or more, the next step moves to the next frame */
    NJS_POINT3 offset; /* from base, turned as base is */
};

struct sprite_vars {
    Uint8 player; /* always 0, the player whose angles the first step reads */
    Uint8 unk_01[3];
    struct sprite_follow *follow; /* null for a looping sprite */
    NJS_TEXLIST *texlist;
};

#define SPRITE(work) ((struct sprite_vars *)&(work)->vars)

static struct task *event_sprite_slots[EVENT_SPRITE_SLOTS] = {NULL};

/* EVENT_SPRITE_SPLASH's eight textures, named with .pvr for
 * EVENT_SPRITE_SPLASH_PVR */
static NJS_TEXNAME event_sprite_splash_textures[8] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};

/* indexed by enum event_sprite_texlist. each list is a run of
 * eff_regular_textures, except EVENT_SPRITE_SPLASH_PVR, which is
 * event_sprite_splash_textures */
NJS_TEXLIST event_sprite_texlists[EVENT_SPRITE_TEXLISTS] = {
#include "shared/sprite/event_sprite_texlists.inc"
};

/* frame n is texture n of the texlist */
static NJS_TEXANIM event_sprite_frames[16] = {
#include "shared/sprite/event_sprite_frames.inc"
};

Sint32 event_sprite_load_texture(NJS_TEXLIST *texlist)
{
    return njLoadTexture(texlist);
}

/* draw frames 1 to 7 and then 0, a step each, then nothing for a step. the
 * x scale is EVENT_SPRITE_LOOP_SCALE plus scl.y, and the y scale plus
 * scl.x */
static void event_sprite_loop_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_SPRITE sprite;
    NJS_ARGB color;

    sprite.tlist = SPRITE(work)->texlist;
    sprite.tanim = event_sprite_frames;
    sprite.sy = work->scl.x + EVENT_SPRITE_LOOP_SCALE;
    sprite.sx = work->scl.y + EVENT_SPRITE_LOOP_SCALE;
    sprite.p.x = 0.0F;
    sprite.p.y = 0.0F;
    sprite.p.z = 0.0F;
    color.a = 1.0F;
    color.r = 1.0F;
    color.g = 1.0F;
    color.b = 1.0F;
    if (TASK_TIMER(work) < SPRITE_LOOP_FRAMES) {
        TASK_TIMER(work)++;
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njFogDisable();
        njSetConstantMaterial(&color);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        njDrawSprite3D(&sprite, TASK_TIMER(work) & (SPRITE_LOOP_FRAMES - 1),
                       NJD_SPRITE_COLOR | NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
        njPopMatrix(1);
    } else {
        TASK_TIMER(work) = 0;
    }
}

static void event_sprite_loop_step(struct task *task)
{
    event_sprite_loop_draw(task);
}

static void event_sprite_destroy(struct task *task)
{
    if (SPRITE(task->work)->follow != NULL) {
        heap_free(SPRITE(task->work)->follow);
    }
}

/* grow, keep to the offset from base, and spin. then draw the frame the
 * timer holds in the record's color. past the texlist's last frame, the
 * timer wraps to 0 */
static void event_sprite_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct sprite_follow *follow = SPRITE(work)->follow;
    NJS_SPRITE sprite;
    NJS_MATRIX matrix;
    NJS_VECTOR offset;
    NJS_VECTOR moved;

    sprite.tlist = SPRITE(work)->texlist;
    sprite.tanim = event_sprite_frames;
    sprite.sx = work->scl.x;
    sprite.sy = work->scl.y;
    if (work->scl.x < EVENT_SPRITE_GROWTH_LIMIT &&
        work->scl.y < EVENT_SPRITE_GROWTH_LIMIT && work->scl.x > 0.0F &&
        work->scl.y > 0.0F) {
        work->scl.x += follow->grow_x;
        work->scl.y += follow->grow_y;
    }
    if (follow->base != NULL) {
        offset.x = follow->offset.x;
        offset.y = follow->offset.y;
        offset.z = follow->offset.z;
        njUnitMatrix(&matrix);
        TURN_MATRIX(njRotateZ, &matrix, follow->base->work->ang.z);
        TURN_MATRIX(njRotateX, &matrix, follow->base->work->ang.x);
        TURN_MATRIX(njRotateY, &matrix, -follow->base->work->ang.y);
        njCalcVector(&matrix, &offset, &moved);
        work->pos.x = follow->base->work->pos.x + moved.x;
        work->pos.y = follow->base->work->pos.y + moved.y;
        work->pos.z = follow->base->work->pos.z + moved.z;
    }
    sprite.p.x = 0.0F;
    sprite.p.y = 0.0F;
    sprite.p.z = 0.0F;
    sprite.ang = work->ang.x;
    work->ang.x += follow->spin;
    if (TASK_TIMER(work) < sprite.tlist->nbTexture) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njFogDisable();
        njSetConstantMaterial(&follow->color);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        njDrawSprite3D(&sprite, TASK_TIMER(work),
                       NJD_SPRITE_ANGLE | NJD_SPRITE_COLOR | NJD_SPRITE_SCALE |
                           NJD_SPRITE_ALPHA);
        njPopMatrix(1);
        if (follow->time >= 1.0F) {
            TASK_TIMER(work)++;
            follow->time -= 1.0F;
        }
        follow->time += follow->speed;
    }
    if (TASK_TIMER(work) >= sprite.tlist->nbTexture) {
        TASK_TIMER(work) = 0;
    }
}

static void event_sprite_step(struct task *task)
{
    event_sprite_draw(task);
}

void event_sprite_kill(Sint16 slot)
{
    if (event_sprite_slots[slot] != NULL) {
        task_destroy(event_sprite_slots[slot]);
        event_sprite_slots[slot] = NULL;
    }
}

/* a looping sprite's first step: set its step, draw and destructor, and
 * zero its timer. the offset (8, -10, 8) it turns by the player's angles is
 * never used */
static void event_sprite_loop_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player;
    NJS_MATRIX matrix;
    NJS_VECTOR offset;
    NJS_VECTOR moved;

    if ((player = player_work[SPRITE(work)->player]) != NULL) {
        offset.x = offset.z = 8.0F;
        offset.y = -10.0F;
        njUnitMatrix(&matrix);
        TURN_MATRIX(njRotateZ, &matrix, player->ang.z);
        TURN_MATRIX(njRotateX, &matrix, player->ang.x);
        TURN_MATRIX(njRotateY, &matrix, -player->ang.y);
        njCalcVector(&matrix, &offset, &moved);
    }
    task->step = event_sprite_loop_step;
    task->draw = event_sprite_loop_draw;
    task->destructor = event_sprite_destroy;
    TASK_TIMER(work) = 0;
}

void event_sprite_create_loop(struct task *base, Sint16 slot, Float x, Float y,
                              Float z, NJS_TEXLIST *texlist)
{
    struct task_work *work;
    NJS_MATRIX matrix;
    NJS_VECTOR offset;
    NJS_VECTOR moved;

    event_sprite_slots[slot] =
        task_create(TASK_WORK, 5, event_sprite_loop_start);
    if (event_sprite_slots[slot] == NULL) {
        return;
    }
    work = event_sprite_slots[slot]->work;
    SPRITE(work)->texlist = texlist;
    offset.x = x;
    offset.y = y;
    offset.z = z;
    njUnitMatrix(&matrix);
    if (base != NULL) {
        TURN_MATRIX(njRotateZ, &matrix, base->work->ang.z);
        TURN_MATRIX(njRotateX, &matrix, base->work->ang.x);
        TURN_MATRIX(njRotateY, &matrix, -base->work->ang.y);
        njCalcVector(&matrix, &offset, &moved);
        work->pos.x = base->work->pos.x + moved.x;
        work->pos.y = base->work->pos.y + moved.y;
        work->pos.z = base->work->pos.z + moved.z;
    } else {
        njCalcVector(&matrix, &offset, &moved);
        work->pos.x = moved.x;
        work->pos.y = moved.y;
        work->pos.z = moved.z;
    }
    work->scl.x = 0.0F;
    work->scl.y = 0.0F;
    work->scl.z = 0.0F;
}

/* a following sprite's first step: set its step, draw and destructor, and
 * zero its timer. the offset (8, -10, 8) it turns by the player's angles is
 * never used */
static void event_sprite_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player;
    NJS_MATRIX matrix;
    NJS_VECTOR offset;
    NJS_VECTOR moved;

    if ((player = player_work[SPRITE(work)->player]) != NULL) {
        offset.x = offset.z = 8.0F;
        offset.y = -10.0F;
        njUnitMatrix(&matrix);
        TURN_MATRIX(njRotateZ, &matrix, player->ang.z);
        TURN_MATRIX(njRotateX, &matrix, player->ang.x);
        TURN_MATRIX(njRotateY, &matrix, -player->ang.y);
        njCalcVector(&matrix, &offset, &moved);
    }
    task->step = event_sprite_step;
    task->draw = event_sprite_draw;
    task->destructor = event_sprite_destroy;
    TASK_TIMER(work) = 0;
}

void event_sprite_create(struct task *base, Sint16 slot, Float x, Float y,
                         Float z, NJS_TEXLIST *texlist)
{
    struct sprite_follow *follow;
    struct task_work *work;
    NJS_MATRIX matrix;
    NJS_VECTOR offset;
    NJS_VECTOR moved;

    event_sprite_slots[slot] = task_create(TASK_WORK, 5, event_sprite_start);
    follow = heap_alloc(sizeof(struct sprite_follow));
    /* @bug memory: the record leaks when the task's allocation fails. BRA at
     * 0x0cb80b96 in EV0029 leaves for the exit with it unfreed */
    if (event_sprite_slots[slot] == NULL) {
        return;
    }
    work = event_sprite_slots[slot]->work;
    SPRITE(work)->texlist = texlist;
    SPRITE(work)->follow = follow;
    follow->base = base;
    offset.x = x;
    offset.y = y;
    offset.z = z;
    njUnitMatrix(&matrix);
    if (base != NULL) {
        TURN_MATRIX(njRotateZ, &matrix, base->work->ang.z);
        TURN_MATRIX(njRotateX, &matrix, base->work->ang.x);
        TURN_MATRIX(njRotateY, &matrix, -base->work->ang.y);
        njCalcVector(&matrix, &offset, &moved);
        work->pos.x = base->work->pos.x + moved.x;
        work->pos.y = base->work->pos.y + moved.y;
        work->pos.z = base->work->pos.z + moved.z;
    } else {
        njCalcVector(&matrix, &offset, &moved);
        work->pos.x = moved.x;
        work->pos.y = moved.y;
        work->pos.z = moved.z;
    }
    follow->color.a = 1.0F;
    follow->color.r = 1.0F;
    follow->color.g = 1.0F;
    follow->color.b = 1.0F;
    work->ang.x = 0;
    follow->spin = 0;
    follow->grow_x = 0.0F;
    follow->grow_y = 0.0F;
    work->scl.x = 1.0F;
    work->scl.y = 1.0F;
    work->scl.z = 1.0F;
    follow->offset.x = x;
    follow->offset.y = y;
    follow->offset.z = z;
    follow->speed = 1.0F;
    follow->time = 0.0F;
}

void event_sprite_set_scale(Sint16 slot, Float x, Float y)
{
    if (event_sprite_slots[slot] != NULL) {
        struct task_work *work = event_sprite_slots[slot]->work;

        work->scl.x = x;
        work->scl.y = y;
    }
}

void event_sprite_set_color(Sint16 slot, Float alpha, Float red, Float green,
                            Float blue)
{
    if (event_sprite_slots[slot] != NULL) {
        struct task_work *work = event_sprite_slots[slot]->work;
        struct sprite_follow *follow = SPRITE(work)->follow;

        follow->color.a = alpha;
        follow->color.r = red;
        follow->color.g = green;
        follow->color.b = blue;
    }
}

void event_sprite_turn(Sint16 slot, Angle ang, Angle spin)
{
    if (event_sprite_slots[slot] != NULL) {
        struct task_work *work = event_sprite_slots[slot]->work;
        struct sprite_follow *follow = SPRITE(work)->follow;

        work->ang.x += ang;
        follow->spin = spin;
    }
}

void event_sprite_set_growth(Sint16 slot, Float x, Float y)
{
    if (event_sprite_slots[slot] != NULL) {
        struct task_work *work = event_sprite_slots[slot]->work;
        struct sprite_follow *follow = SPRITE(work)->follow;

        follow->grow_x = x;
        follow->grow_y = y;
    }
}

void event_sprite_nop(int unused)
{
    /* @hack redundant: unused tested for nothing, which puts TST R4,R4 in the
     * RTS's slot (0x0cb80d56 in EV0029); without it, the slot holds a NOP
     * and 1 of 2 words differ */
    if (unused) {
    }
}

void event_sprite_set_speed(Sint16 slot, Float speed)
{
    if (event_sprite_slots[slot] != NULL) {
        struct task_work *work = event_sprite_slots[slot]->work;
        struct sprite_follow *follow = SPRITE(work)->follow;

        follow->speed = speed;
    }
}
