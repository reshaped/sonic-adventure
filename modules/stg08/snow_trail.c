/* Ice Cap's snow effects: a player's trail of snow strips, a board's flakes,
 * and the snow emitter */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "debug/editor.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/snow.h"
#include "hud/hud.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg08/snow_trail.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 16
#define TEXT_2_LENGTH 16
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 10
#include "stg08/snow_trail/text.inc"

/* OBJ_ICECAP2's texture names, of which the trail takes snb01 to snb08 */
extern NJS_TEXNAME obj_icecap2_textures[60];
/* OBJ_ICECAP's texture names, of which the flakes take the last, yuki_moto */
extern NJS_TEXNAME obj_icecap_textures[100];
static NJS_TEXLIST trail_texlist = {
#include "stg08/snow_trail/trail_texlist.inc"
};
/* trail_slot counts the strips that got their copies. each such strip takes
 * the slot in trail_tasks that the count gives, mod TRAIL_SLOTS */
#define TRAIL_SLOTS 8
static Uint8 trail_slot = 0;
static struct task *trail_tasks[TRAIL_SLOTS] = {NULL};
/* the params every emitter's puffs share. snow_create_from keeps a pointer to
 * them, and a puff reads their drag each step. snow_emitter_step sets the
 * drag before each puff it makes. that sets the drag of the puffs already out
 * too */
static struct dust_params emitter_params = {1.0F,
                                            0.008F,
                                            0.05F,
                                            0.06F,
                                            -0.002F,
                                            {0.0F, 0.0F, 0.0F},
                                            {0.0F, 0.0F, 0.0F},
                                            {0.8F, 0.4F, 0.4F, 0.4F}};
static NJS_TEXLIST flake_texlist = {
#include "stg08/snow_trail/flake_texlist.inc"
};
extern NJS_OBJECT trail_object;
extern NJS_OBJECT flake_object;

/* trail_object's model's points lie in TRAIL_PAIRS pairs along the strip */
#define TRAIL_PAIRS 9
/* the ages at which a strip and a flake end. no flake reaches its age, as
 * flake_step's @bug says */
#define TRAIL_LIFE 8
#define FLAKE_LIFE 180

struct trail_vars {
    /* the alpha of the draw's constant material: -0.1 to start, and 1.6
     * times as much each step */
    Float alpha;
    /* the strip's own copy of trail_object, whose points a join moves, or
     * null where an allocation failed */
    NJS_OBJECT *object;
    Float rise; /* added to scl.y each step, and 0.1 less each step */
};
#define TRAIL(work) ((struct trail_vars *)&(work)->vars)
/* a strip's or a flake's steps so far */
#define AGE(work) ((work)->submode)
/* the strip's slot in trail_tasks */
#define TRAIL_SLOT(work) ((work)->tick)
/* the low 16 bits of the play time when the strip was made */
#define MADE_AT(work) TASK_TIMER(work)
/* a COLD emitter's chance of a puff each step, the puffs' scale and their
 * drag */
#define EMIT_CHANCE(work) ((work)->vars.f[0])
#define EMIT_SCALE(work) ((work)->vars.f[1])
#define EMIT_DRAG(work) ((work)->vars.f[2])
/* the fraction a set entry's angle gives an emitter's setting */
#define FROM_SET_ANGLE(ang) ((ang) * (1.0F / 65535.0F))
#define FLAKE_SCALE(work) ((work)->vars.f[2])

static void trail_step(struct task *task);
static void trail_draw(struct task *task);
static void trail_destroy(struct task *task);
static void flake_step(struct task *task);
static void flake_draw(struct task *task);
static void flake_destroy(struct task *task);

/* move this strip's even points onto the next slot's strip's odd points, so
 * that the two meet. it joins only a strip made at the next play time, whose
 * scl.z isn't of the opposite sign. this strip's object must exist */
static void trail_join(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = TRAIL(work)->object;
    struct task *next = trail_tasks[(TRAIL_SLOT(work) + 1) & (TRAIL_SLOTS - 1)];
    struct task_work *next_work;
    NJS_OBJECT *next_object;
    NJS_POINT3 points[TRAIL_PAIRS * 2];
    NJS_POINT3 *source;
    NJS_POINT3 *destination;
    Uint32 source_address;
    Angle angle;
    int i;
    if (!next) {
        return;
    }
    next_work = next->work;
    if (MADE_AT(work) + 1 != MADE_AT(next_work)) {
        return;
    }
    if (work->scl.z * next_work->scl.z < 0.0F) {
        return;
    }
    next_object = TRAIL(next_work)->object;

    if (!next_object) {
        return;
    }
    /* @hack redundant: the test repeated, a second TST and BF at 0x0c9003c0 and
     * 0x0c9003c2; without it, the function is 8 bytes shorter and 158 of 203
     * words differ */
    if (!next_object) {
        return;
    }
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &next_work->pos);
    TURN_VIA(njRotateZ, next_work->ang.z, angle);
    TURN_VIA(njRotateX, next_work->ang.x, angle);
    TURN_VIA(njRotateY, next_work->ang.y, angle);
    njScaleV(NULL, &next_work->scl);
    /* the source steps as an address held in an integer. its last step, to
     * index 19 of the model's 18 points, would be undefined as pointer
     * arithmetic */
    source_address = (Uint32)(next_object->model->points + 1);
    destination = points;

    for (i = 0; i < TRAIL_PAIRS; i++) {
        njCalcPoint(NULL, (NJS_POINT3 *)source_address, destination);
        destination += 2;
        source_address += 2 * sizeof(NJS_POINT3);
    }
    njPopMatrix(1);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, angle);
    TURN_VIA(njRotateX, work->ang.x, angle);
    TURN_VIA(njRotateY, work->ang.y, angle);
    njScaleV(NULL, &work->scl);
    njInvertMatrix(NULL);
    /* @bug memory: a strip whose copies couldn't be allocated reads its
     * object's model through null here. icecap_trail_create leaves such a
     * strip running, and trail_step joins it each step. its TRAIL_SLOT is 0,
     * though it took no slot, so it joins slot 1's strip. it gets here where
     * that strip was made at the next play time, with scl.z not of the
     * opposite sign. MOV.L @(4,R15),R13 at 0x0c900506 loads the null object,
     * and MOV.L @(4,R13),R13 at 0x0c90050c reads through it */
    destination = object->model->points;
    source = points;
    for (i = 0; i < TRAIL_PAIRS; i++) {
        njCalcPoint(NULL, source, destination);
        source += 2;
        destination += 2;
    }
    njPopMatrix(1);
}

void icecap_trail_create(char player)
{
    Uint8 index = player;
    struct task_work *player_workarea = player_work[index];
    struct task_motion *motion = player_motion[index];
    struct player *player_state = player_get(index);
    struct task *task;
    struct task_work *work;
    NJS_OBJECT *object;
    if (!player_workarea) {
        return;
    }
    if (!(player_workarea->flags & PLAYER_ON_GROUND)) {
        return;
    }
    task = task_create(TASK_WORK, 6, trail_step);
    if (!task) {
        return;
    }
    work = task->work;
    task->draw = trail_draw;
    task->destructor = trail_destroy;
    work->pos = player_workarea->pos;
    work->ang = player_workarea->ang;
    work->ang.y = -work->ang.y;
    work->scl.x = -player_state->speed.x * (1.0F / 6.0F);
    work->scl.y = -player_state->speed.y + 0.2F;
    work->scl.z = motion->drive * 0.2F;
    TRAIL(work)->rise = player_state->speed.x * njAbs(motion->drive) * 0.08F;
    TRAIL(work)->alpha = -0.1F;
    if (work->scl.z > 0.0F) {
        work->scl.z += (Float)rand() * (0.1F * RAND_SCALE);
    }
    if (work->scl.z < 0.0F) {
        work->scl.z -= (Float)rand() * (0.1F * RAND_SCALE);
    }
    MADE_AT(work) = play_time_get();
    object = heap_alloc(sizeof(NJS_OBJECT));
    if (!object) {
        return;
    }
    *object = trail_object;
    if ((object->model = heap_alloc(sizeof(NJS_MODEL))) == NULL) {
        heap_free(object);
        return;
    } else {
        *object->model = *trail_object.model;
        if ((object->model->points = heap_alloc(trail_object.model->nbPoint *
                                                sizeof(NJS_POINT3)))) {
            njMemCopy4(object->model->points, trail_object.model->points,
                       object->model->nbPoint * 3);
        } else {
            heap_free(object->model);
            heap_free(object);
            return;
        }
    }
    TRAIL(work)->object = object;
    TRAIL_SLOT(work) = trail_slot & (TRAIL_SLOTS - 1);
    trail_tasks[TRAIL_SLOT(work)] = task;
    trail_slot++;
    trail_join(task);
    trail_draw(task);
}

static void trail_step(struct task *task)
{
    struct task_work *work = task->work;
    AGE(work)++;
    if (AGE(work) >= TRAIL_LIFE) {
        task_kill(task);
        return;
    }
    work->scl.z *= 1.2F;
    work->scl.y += TRAIL(work)->rise;
    TRAIL(work)->rise -= 0.1F;
    TRAIL(work)->alpha *= 1.6F;
    trail_join(task);
    trail_draw(task);
}

static void trail_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB material;
    Angle angle;
    material.r = material.g = material.b = 0.0F;
    material.a = TRAIL(work)->alpha;
    njSetConstantMaterial(&material);

    /* @bug memory: the draw indexes trail_texlist, one texlist, by
     * the strip's age. from age 1, the index runs past it onto trail_slot,
     * trail_tasks and emitter_params, which the draw reads as texlists. MOV.B
     * at 0x0c90088a loads the age, and 0x0c900898 adds it, times 8, to
     * trail_texlist's address for njSetTexture */
#ifdef GCC
    /* repairs the read the tag names: GCC's strip takes the one texlist */
    njSetTexture(&trail_texlist);
#else
    njSetTexture((NJS_TEXLIST *)((Uint32)&trail_texlist +
                                 ((Sint32)AGE(work) * sizeof(NJS_TEXLIST))));
#endif
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, angle);
    TURN_VIA(njRotateX, work->ang.x, angle);
    TURN_VIA(njRotateY, work->ang.y, angle);
    njScaleV(NULL, &work->scl);
    if (TRAIL(work)->object) {
        draw_tree(TRAIL(work)->object);
    } else {
        draw_tree(&trail_object);
    }
    njPopMatrix(1);
    material.a = 0.0F;
    njSetConstantMaterial(&material);
}

static void trail_destroy(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = TRAIL(work)->object;
    /* a strip clears its slot as it ends, even where a newer strip has taken
     * the slot. a strip whose allocation failed took none, and clears slot 0 */
    trail_tasks[TRAIL_SLOT(work)] = NULL;
    if (object) {
        heap_free(object->model->points);
        heap_free(object->model);
        heap_free(object);
        TRAIL(task->work)->object = NULL;
    }
}

static void snow_emitter_step(struct task *task)
{
    struct task_work *work = task->work;
    if (placement_editing(task)) {
        EMIT_CHANCE(work) = FROM_SET_ANGLE(work->ang.x);
        EMIT_SCALE(work) = FROM_SET_ANGLE(work->ang.y);
        EMIT_DRAG(work) = FROM_SET_ANGLE(work->ang.z);
        /* TEXT_1 to TEXT_6 label ang.x, ang.y and ang.z in the set editor,
         * then scl's three. scl gives the puffs' speed */
        njPrintC(NJM_LOCATION(17, 16), TEXT_1);
        njPrintC(NJM_LOCATION(17, 17), TEXT_2);
        njPrintC(NJM_LOCATION(17, 18), TEXT_3);
        njPrintC(NJM_LOCATION(17, 19), TEXT_4);
        njPrintC(NJM_LOCATION(17, 20), TEXT_5);
        njPrintC(NJM_LOCATION(17, 21), TEXT_6);
    }
    if ((Float)rand() * RAND_SCALE < EMIT_CHANCE(work)) {
        emitter_params.pos = work->pos;
        emitter_params.scale =
            EMIT_SCALE(work) + ((Float)rand() * (0.01F * RAND_SCALE));
        emitter_params.speed = work->scl;

        emitter_params.drag = EMIT_DRAG(work);
        snow_create_from(&emitter_params);
    }
}

void icecap_snow_emitter_create(struct task *task)
{
    struct task_work *work = task->work;
    EMIT_CHANCE(work) = FROM_SET_ANGLE(work->ang.x);
    EMIT_SCALE(work) = FROM_SET_ANGLE(work->ang.y);
    task->step = snow_emitter_step;
}

void icecap_board_spray(int player)
{
    Uint8 index = player;
    struct task_work *player_workarea = player_work[index];
    struct task_motion *motion = player_motion[index];
    struct player *player_state = player_get(index);
    struct task *task;
    struct task_work *work;
    int i;
    if (!player_workarea) {
        return;
    }
    if (!(player_workarea->flags & PLAYER_ON_GROUND)) {
        return;
    }
    if (!(player_state->speed.x > 4.0F)) {
        return;
    }
    i = 16;
    do {
        task = task_create(TASK_WORK, 6, flake_step);
        if (task) {
            work = task->work;
            task->draw = flake_draw;
            task->destructor = flake_destroy;
            work->pos = player_workarea->pos;
            work->ang.x = (Float)rand() * 2.0F;
            work->ang.z = (Float)rand() * 2.0F;
            work->ang.y = player_workarea->ang.y;
            FLAKE_SCALE(work) = 0.3F + ((Float)rand() * (0.2F * RAND_SCALE));
            work->scl.x =
                (motion->speed.x * 0.7F) - ((Float)rand() * RAND_SCALE) +
                (motion->drive * njSin(player_workarea->ang.y)) + 0.5F;
            work->scl.y = (motion->speed.y * 0.7F) +
                          (player_state->speed.x *
                           (0.4F + ((Float)rand() * RAND_SCALE)) * 0.15F);
            work->scl.z =
                (motion->speed.z * 0.7F) - ((Float)rand() * RAND_SCALE) +
                (motion->drive * njCos(player_workarea->ang.y)) + 0.5F;
            flake_draw(task);
        }
    } while (i--);

    /* @hack redundant: a call of njSqrt that never runs, which keeps the
     * constant and pointer registers in the image's order: the pointers loaded
     * at 0x0c900b3e and 0x0c900b46 take R11 and R10; without it, the two trade
     * and 21 of 169 words differ */
    if (0) {
        njSqrt(2.0F);
    }
}

static void flake_step(struct task *task)
{
    struct task_work *work = task->work;
    if (placement_try_leave(task)) {
        return;
    }
    AGE(work)++;
    /* @bug logic: the age is a signed byte, which wraps at 127, so the
     * CMP/GE with 180 at 0x0c900cfc never holds */
    /* NOLINTNEXTLINE(clang-diagnostic-tautological-*) */
    if (AGE(work) >= FLAKE_LIFE) {
        task_kill(task);
        return;
    }
    work->ang.x += 1024;
    work->ang.y += 1024;
    work->ang.z += 1024;
    work->scl.y -= 0.04F;
    work->pos.x += work->scl.x;
    work->pos.y += work->scl.y;
    work->pos.z += work->scl.z;
    flake_draw(task);
}

static void flake_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float scale = FLAKE_SCALE(work);
    Angle angle;
    njSetTexture(&flake_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, angle);
    TURN_VIA(njRotateX, work->ang.x, angle);
    TURN_VIA(njRotateY, work->ang.y, angle);
    njScale(NULL, scale, scale, scale);
    draw_tree(&flake_object);
    njPopMatrix(1);
}

/* nothing to free: every flake draws flake_object itself */
static void flake_destroy(struct task *task)
{
}
