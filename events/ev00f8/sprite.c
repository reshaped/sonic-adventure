/* EV00F8's own sprites: additive 3d sprites in ten slots. each shows one
 * texture of a texlist, at an offset from a task or at a fixed place */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev00f8/sprite.h"
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
#include "ev00f8/sprite/text.inc"

#define SPRITE_SLOTS 10

/* a sprite's heap record: ev00f8_sprite_create allocates it, and
 * ev00f8_sprite_destroy frees it */
struct sprite_record {
    struct task *base; /* the task it keeps to, or null */
    NJS_ARGB color;
    Angle spin; /* added to work->ang.x at each draw */
    /* added to work->scl at each draw while both sides are between 0 and
     * 100 */
    Float grow_x, grow_y;
    NJS_POINT3 offset; /* from base, turned as base is */
};

/* a sprite's bytes in work->vars */
struct sprite_vars {
    /* the player whose angles the start reads. the work starts zeroed and
     * nothing writes this, so it is player 0 */
    Uint8 player;
    Uint8 unk_01[3];
    struct sprite_record *record;
    NJS_TEXLIST *texlist;
};

#define SPRITE(work) ((struct sprite_vars *)&(work)->vars)

static struct task *ev00f8_sprite_slots[SPRITE_SLOTS] = {NULL};

/* the texture names of the EVENT_SPRITE_SPLASH_PVR row. they are the
 * EVENT_SPRITE_SPLASH row's eight textures, named by their files */
static NJS_TEXNAME ev00f8_sprite_splash_textures[8] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};

/* indexed by enum event_sprite_texlist, as event_sprite_texlists is */
static NJS_TEXLIST ev00f8_sprite_texlists[EVENT_SPRITE_TEXLISTS] = {
#include "ev00f8/sprite/ev00f8_sprite_texlists.inc"
};

/* frame n is texture n of the texlist */
static NJS_TEXANIM ev00f8_sprite_frames[16] = {
#include "ev00f8/sprite/ev00f8_sprite_frames.inc"
};

Sint32 ev00f8_sprite_load_texture(NJS_TEXLIST *texlist)
{
    return njLoadTexture(texlist);
}

/* the task's draw, which its step calls too, so growth and spin apply at
 * each call. the timer holds the texture to draw. at or past the texlist's
 * length, the draw shows nothing and takes the length off the timer */
static void ev00f8_sprite_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct sprite_record *record = SPRITE(work)->record;
    NJS_SPRITE sprite;
    NJS_MATRIX matrix;
    NJS_VECTOR offset;
    NJS_VECTOR moved;

    sprite.tlist = SPRITE(work)->texlist;
    sprite.tanim = ev00f8_sprite_frames;
    sprite.sx = work->scl.x;
    sprite.sy = work->scl.y;
    if (work->scl.x < 100.0F && work->scl.y < 100.0F && work->scl.x > 0.0F &&
        work->scl.y > 0.0F) {
        work->scl.x += record->grow_x;
        work->scl.y += record->grow_y;
    }
    if (record->base != NULL) {
        offset.x = record->offset.x;
        offset.y = record->offset.y;
        offset.z = record->offset.z;
        njUnitMatrix(&matrix);
        TURN_MATRIX(njRotateZ, &matrix, record->base->work->ang.z);
        TURN_MATRIX(njRotateX, &matrix, record->base->work->ang.x);
        TURN_MATRIX(njRotateY, &matrix, -record->base->work->ang.y);
        njCalcVector(&matrix, &offset, &moved);
        work->pos.x = record->base->work->pos.x + moved.x;
        work->pos.y = record->base->work->pos.y + moved.y;
        work->pos.z = record->base->work->pos.z + moved.z;
    }
    sprite.p.x = 0.0F;
    sprite.p.y = 0.0F;
    sprite.p.z = 0.0F;
    sprite.ang = work->ang.x;
    work->ang.x += record->spin;
    if (TASK_TIMER(work) < sprite.tlist->nbTexture) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njFogDisable();
        njSetConstantMaterial(&record->color);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        njDrawSprite3D(&sprite, TASK_TIMER(work),
                       NJD_SPRITE_ANGLE | NJD_SPRITE_COLOR | NJD_SPRITE_SCALE |
                           NJD_SPRITE_ALPHA);
        njPopMatrix(1);
    } else {
        TASK_TIMER(work) -= sprite.tlist->nbTexture;
    }
}

static void ev00f8_sprite_step(struct task *task)
{
    ev00f8_sprite_draw(task);
}

static void ev00f8_sprite_destroy(struct task *task)
{
    if (SPRITE(task->work)->record != NULL) {
        heap_free(SPRITE(task->work)->record);
    }
}

void ev00f8_sprite_kill(Sint16 slot)
{
    if (ev00f8_sprite_slots[slot] != NULL) {
        task_destroy(ev00f8_sprite_slots[slot]);
        ev00f8_sprite_slots[slot] = NULL;
    }
}

/* the start turns the offset (8, -10, 8) by the player's angles into
 * moved, which nothing reads */
static void ev00f8_sprite_start(struct task *task)
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
    task->step = ev00f8_sprite_step;
    task->draw = ev00f8_sprite_draw;
    task->destructor = ev00f8_sprite_destroy;
}

void ev00f8_sprite_create(struct task *base, Sint16 slot, Float x, Float y,
                          Float z, NJS_TEXLIST *texlist, Sint16 texture)
{
    struct task_work *work;
    struct sprite_record *record;
    NJS_MATRIX matrix;
    NJS_VECTOR offset;
    NJS_VECTOR moved;

    ev00f8_sprite_slots[slot] = task_create(TASK_WORK, 5, ev00f8_sprite_start);
    record = heap_alloc(sizeof(struct sprite_record));
    /* @bug memory: the record leaks when the task's allocation fails. BRA at
     * 0x0cb80bca leaves for the exit with it unfreed */
    if (ev00f8_sprite_slots[slot] != NULL) {
        work = ev00f8_sprite_slots[slot]->work;
        SPRITE(work)->texlist = texlist;
        SPRITE(work)->record = record;
        record->base = base;
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
        record->color.a = 1.0F;
        record->color.r = 1.0F;
        record->color.g = 1.0F;
        record->color.b = 1.0F;
        work->ang.x = 0;
        record->spin = 0;
        record->grow_x = 0.0F;
        record->grow_y = 0.0F;
        work->scl.x = 1.0F;
        work->scl.y = 1.0F;
        work->scl.z = 1.0F;
        record->offset.x = x;
        record->offset.y = y;
        record->offset.z = z;
        work->timer = texture;
    }
}

void ev00f8_sprite_set_scale(Sint16 slot, Float x, Float y)
{
    struct task_work *work;

    if (ev00f8_sprite_slots[slot] != NULL) {
        work = ev00f8_sprite_slots[slot]->work;
        work->scl.x = x;
        work->scl.y = y;
    }
}

void ev00f8_sprite_set_color(Sint16 slot, Float alpha, Float red, Float green,
                             Float blue)
{
    struct sprite_record *record;

    if (ev00f8_sprite_slots[slot] != NULL) {
        record = SPRITE(ev00f8_sprite_slots[slot]->work)->record;
        record->color.a = alpha;
        record->color.r = red;
        record->color.g = green;
        record->color.b = blue;
    }
}

void ev00f8_sprite_turn(Sint16 slot, Angle ang, Angle spin)
{
    struct task_work *work;
    struct sprite_record *record;

    if (ev00f8_sprite_slots[slot] != NULL) {
        work = ev00f8_sprite_slots[slot]->work;
        record = SPRITE(work)->record;
        work->ang.x += ang;
        record->spin = spin;
    }
}

void ev00f8_sprite_set_growth(Sint16 slot, Float x, Float y)
{
    struct sprite_record *record;

    if (ev00f8_sprite_slots[slot] != NULL) {
        record = SPRITE(ev00f8_sprite_slots[slot]->work)->record;
        record->grow_x = x;
        record->grow_y = y;
    }
}
