/* trees of models broken into fragments that fly apart, and a knock-back of
 * player 0 from an object */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "draw/model.h"
#include "effect/dust.h"
#include "effect/flame.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/fragments.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* one node of a broken tree */
struct fragment {
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    NJS_MODEL *model;
    struct angles ang;
    /* the turn each step while it flies. once it lands, spin.x counts its
     * steps on the ground */
    struct angles spin;
    Sint32 flags;
};

/* in a fragment's flags */
#define FRAGMENT_GONE 0x1
#define FRAGMENT_LANDED 0x2

/* a landed fragment is gone once it has slid more steps than this */
#define FRAGMENT_SLIDE 40

struct fragments {
    int count;
    struct fragment *pieces;
    /* a copy of the caller's spec, with fragments_scatter's arguments
     * added */
    struct fragments_spec spec;
};

struct fragments_vars {
    struct fragments *fragments;
};

#define VARS(work) ((struct fragments_vars *)&(work)->vars)

#define TEN_DEGREES (65536.0F / 36.0F)

/* a random Angle to turn a step, either way. its size is less than
 * (range + 0.5) * 5 degrees */
#define SPIN(range)                                                            \
    ((Angle)((((rand() * RAND_SCALE) - 0.5F) *                                 \
              ((range) + (rand() * (0.5F * RAND_SCALE)))) *                    \
             TEN_DEGREES))

/* the heap's room for each fragment, 4 bytes more than struct fragment
 * takes */
#define FRAGMENT_SIZE 60

/* the speed ahead and up of casino_knock_player_back, and of the slow one. a
 * knock-back ignores the stick for KNOCK_STEPS steps */
#define KNOCK_SPEED 1.5F
#define KNOCK_SLOW_SPEED 1.0F
#define KNOCK_STEPS 60

#define SE_BREAK 26

static void fragments_start(struct task *task);
static void fragments_destroy(struct task *task);
static void fragments_task_draw(struct task *task);
static void fragments_run(struct task *task);

void casino_knock_player_back(struct task_work *work)
{
    int player = player_nearest(&work->pos);

    if (player == 0) {
        struct task_work *nearest = player_work[player];
        NJS_VECTOR away;
        NJS_VECTOR speed;
        Angle ang[3];

        away.x = nearest->pos.x - work->pos.x;
        away.y = 0.0F;
        away.z = nearest->pos.z - work->pos.z;
        ang[0] = 0;
        ang[1] = (Angle)(atan2f(away.x, away.z) * ANGLE_PER_RADIAN);
        ang[2] = 0;
        speed.x = KNOCK_SPEED;
        speed.y = KNOCK_SPEED;
        speed.z = 0.0F;
        player_send_flying(0, &speed, ang, KNOCK_STEPS);
        nearest->pos.y += 1.0F;
    }
}

void casino_knock_player_back_slow(struct task_work *work)
{
    int player = player_nearest(&work->pos);

    if (player == 0) {
        struct task_work *nearest = player_work[player];
        NJS_VECTOR away;
        NJS_VECTOR speed;
        Angle ang[3];

        away.x = nearest->pos.x - work->pos.x;
        away.y = 0.0F;
        away.z = nearest->pos.z - work->pos.z;
        ang[0] = 0;
        ang[1] = (Angle)(atan2f(away.x, away.z) * ANGLE_PER_RADIAN);
        ang[2] = 0;
        speed.x = KNOCK_SLOW_SPEED;
        speed.y = KNOCK_SLOW_SPEED;
        speed.z = 0.0F;
        player_send_flying(0, &speed, ang, KNOCK_STEPS);
        nearest->pos.y += 1.0F;
    }
}

/* the count of fragments object's tree breaks into. the fragments are object
 * and the nodes below it, but none below a node with NJD_EVAL_BREAK */
static int fragments_count(NJS_OBJECT *object)
{
    NJS_OBJECT *child;
    int count = 1;

    if (object->evalflags & NJD_EVAL_BREAK) {
        child = NULL;
    } else {
        child = object->child;
    }
    while (child) {
        count += fragments_count(child);
        child = child->sibling;
    }
    return count;
}

/* make the fragments of object's tree from index on, in the current
 * matrix's coordinates. returns the index after them. parent is the
 * fragment of object's parent, or NULL for a tree's root */
static int fragments_make(NJS_OBJECT *object, struct fragment *parent,
                          struct fragments *fragments, int index)
{
    NJS_OBJECT *child;
    struct fragment *piece;
    NJS_VECTOR out;
    Float spread;
    Float radius;
    Float burst;
    Float scale;
    Float distance;
    Angle ang;

    if (object->evalflags & NJD_EVAL_BREAK) {
        child = NULL;
    } else {
        child = object->child;
    }
    piece = &fragments->pieces[index++];
    njPushMatrix(NULL);
    njCalcPoint(NULL, (NJS_POINT3 *)object->pos, &piece->pos);
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    TURN_VIA(njRotateZ, object->ang[2], ang);
    TURN_VIA(njRotateY, object->ang[1], ang);
    TURN_VIA(njRotateX, object->ang[0], ang);
    piece->model = object->model;
    if (parent) {
        piece->ang.x = object->ang[0] + parent->ang.x;
        piece->ang.y = object->ang[1] + parent->ang.y;
        piece->ang.z = object->ang[2] + parent->ang.z;
    } else {
        piece->ang.x = object->ang[0] + fragments->spec.ang.x;
        piece->ang.y = object->ang[1] + fragments->spec.ang.y;
        piece->ang.z = object->ang[2] + fragments->spec.ang.z;
    }
    /* @bug logic: a fragment starting less than 0.1 above the ground moves
     * to a y of 0.1, whatever the ground's height. the test measures from
     * the ground (FSUB at 0x0c922820), but the store of 0.1 doesn't
     * (0x0c922830) */
    if (piece->pos.y - fragments->spec.ground - 0.1F < 0.0F) {
        piece->pos.y = 0.1F;
    }
    burst = fragments->spec.burst_speed * 2.0F;
    spread = fragments->spec.spread * 2.0F;
    {
        /* @hack spelling: the offset from center taken through byte cursors,
         * which SHC steps with FMOV.S @R6+ and @R4+ and ADD #4,R5
         * (0x0c922838 to 0x0c922862). through the members, 210 of 322 words
         * differ */
        Uint8 *to = (Uint8 *)&out;
        Uint8 *at = (Uint8 *)&piece->pos;
        Uint8 *from = (Uint8 *)&fragments->spec.center;

        *(Float *)to = *(Float *)at - *(Float *)from;
        to = to + sizeof(Float);
        at = at + sizeof(Float);
        from = from + sizeof(Float);
        *(Float *)to = *(Float *)at - *(Float *)from;
        to = to + sizeof(Float);
        at = at + sizeof(Float);
        from = from + sizeof(Float);
        *(Float *)to = *(Float *)at - *(Float *)from;
    }
    radius = fragments->spec.burst_radius;
    if (radius != 0.0F) {
        scale = burst / radius;
    } else {
        radius = 1.0F;
        scale = 0.0F;
    }
    distance = njScalor(&out);
    out.x *= scale;
    out.y *= scale;
    out.z *= scale;
    radius = distance / radius;
    piece->spin.x = SPIN(radius * 0.2F);
    piece->spin.y = SPIN(radius * 0.2F);
    piece->spin.z = SPIN(radius * 0.2F);
    piece->speed.x = out.x + (spread * ((rand() * RAND_SCALE) - 0.5F)) +
                     fragments->spec.speed.x;
    piece->speed.y = out.y + (spread * ((rand() * RAND_SCALE) - 0.5F)) +
                     fragments->spec.speed.y;
    piece->speed.z = out.z + (spread * ((rand() * RAND_SCALE) - 0.5F)) +
                     fragments->spec.speed.z;
    piece->flags = 0;
    while (child) {
        index = fragments_make(child, piece, fragments, index);
        child = child->sibling;
    }
    njPopMatrix(1);
    return index;
}

/* fragment_draw, fragment_smoke and fragment_move each return 1 while the
 * fragment isn't gone, and 0 once it is. fragment_draw draws it while it
 * flies */
static int fragment_draw(struct fragment *piece, struct fragments_spec *spec)
{
    Angle ang;

    if (!(piece->flags & FRAGMENT_GONE)) {
        if (!(piece->flags & FRAGMENT_LANDED) && piece->model) {
            njPushMatrix(NULL);
            njTranslateV(NULL, &piece->pos);
            TURN_VIA(njRotateZ, piece->ang.z, ang);
            TURN_VIA(njRotateY, piece->ang.y, ang);
            TURN_VIA(njRotateX, piece->ang.x, ang);
            draw_model(piece->model);
            njPopMatrix(1);
        }
        return 1;
    } else {
        return 0;
    }
}

/* a fragment with a model makes a flame on the step after it lands. as it
 * flies, it makes a puff of dust with a chance of 0.3 each step */
static int fragment_smoke(struct fragment *piece, struct fragments_spec *spec)
{
    if (!(piece->flags & FRAGMENT_GONE)) {
        if (piece->flags & FRAGMENT_LANDED) {
            if (piece->spin.x == 0 && piece->model) {
                NJS_VECTOR speed;

                speed.x = piece->speed.x * 0.2F;
                speed.y = 1.0F;
                speed.z = piece->speed.z * 0.2F;
                flame_create(
                    &piece->pos, &speed,
                    (spec->smoke_size + (spec->smoke_scale * piece->model->r)) *
                        2.0F);
            }
        } else {
            if ((rand() * RAND_SCALE) < 0.3F && piece->model) {
                NJS_POINT3 pos;
                NJS_VECTOR speed;

                pos.x = piece->pos.x + piece->model->center.x;
                pos.y = piece->pos.y + piece->model->center.y;
                pos.z = piece->pos.z + piece->model->center.z;
                speed.x = 0.0F;
                speed.y = 0.0F;
                speed.z = 0.0F;
                dust_create(&pos, &speed,
                            (spec->smoke_size * (Float)rand() * RAND_SCALE) +
                                (spec->smoke_scale * piece->model->r));
            }
        }
        return 1;
    } else {
        return 0;
    }
}

/* move the fragment a step. one whose move would cross the ground lands
 * short of it. from then on, it slides along x and z. a flying fragment
 * farther than reach from center is gone */
static int fragment_move(struct fragment *piece, struct fragments_spec *spec)
{
    if (!(piece->flags & FRAGMENT_GONE)) {
        if ((piece->flags & FRAGMENT_LANDED) ||
            (piece->pos.y >= spec->ground &&
             piece->pos.y + piece->speed.y <= spec->ground) ||
            (piece->pos.y <= spec->ground &&
             piece->pos.y + piece->speed.y >= spec->ground)) {
            piece->pos.x += piece->speed.x;
            piece->pos.z += piece->speed.z;
            piece->speed.x += spec->accel.x;
            piece->speed.z += spec->accel.z;
            piece->speed.x *= spec->drag;
            piece->speed.z *= spec->drag;
            if (piece->flags & FRAGMENT_LANDED) {
                piece->spin.x++;
                if (piece->spin.x > FRAGMENT_SLIDE) {
                    piece->flags |= FRAGMENT_GONE;
                }
            } else {
                piece->flags |= FRAGMENT_LANDED;
                piece->spin.x = 0;
            }
        } else {
            piece->pos.x += piece->speed.x;
            piece->pos.y += piece->speed.y;
            piece->pos.z += piece->speed.z;
            piece->speed.x += spec->accel.x;
            piece->speed.y += spec->accel.y;
            piece->speed.z += spec->accel.z;
            piece->speed.x *= spec->drag;
            piece->speed.y *= spec->drag;
            piece->speed.z *= spec->drag;
            piece->ang.x += piece->spin.x;
            piece->ang.y += piece->spin.y;
            piece->ang.z += piece->spin.z;
            if (njDistanceP2P(&piece->pos, &spec->center) > spec->reach) {
                piece->flags |= FRAGMENT_GONE;
            }
        }
        return 1;
    } else {
        return 0;
    }
}

/* allocate the fragments of spec's trees, with a copy of spec that the
 * arguments move. returns NULL when the heap has no room for the
 * fragments' record */
static struct fragments *fragments_create(NJS_POINT3 *pos, struct angles *ang,
                                          NJS_VECTOR *speed, NJS_VECTOR *accel,
                                          struct fragments_spec *spec,
                                          Float ground)
{
    Angle turn;
    int index;
    struct fragments *fragments;
    NJS_OBJECT *object;
    int count = 0;

    for (object = spec->objects; object; object = object->sibling) {
        count += fragments_count(object);
    }
    fragments = heap_alloc(sizeof(*fragments));
    /* @bug memory: a null fragments is written through before its test
     * takes effect. the count and pieces go to addresses 0 and 4
     * (0x0c922dce, and 0x0c922dd8 in the delay slot of the test's BF/S) */
    fragments->count = count;
    fragments->pieces = heap_alloc(count * FRAGMENT_SIZE);
    if (fragments == NULL) {
        return NULL;
    }
    fragments->spec = *spec;
    if (fragments->spec.own_texlist) {
        njLoadTexture(fragments->spec.own_texlist);
    }
    fragments->spec.ground += ground;
    fragments->spec.center.x += pos->x;
    fragments->spec.center.y += pos->y;
    fragments->spec.center.z += pos->z;
    fragments->spec.speed.x += speed->x;
    fragments->spec.speed.y += speed->y;
    fragments->spec.speed.z += speed->z;
    fragments->spec.accel.x += accel->x;
    fragments->spec.accel.y += accel->y;
    fragments->spec.accel.z += accel->z;
    fragments->spec.ang.x += ang->x;
    fragments->spec.ang.y += ang->y;
    fragments->spec.ang.z += ang->z;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &fragments->spec.center);
    TURN_VIA(njRotateZ, fragments->spec.ang.z, turn);
    TURN_VIA(njRotateY, fragments->spec.ang.y, turn);
    TURN_VIA(njRotateX, fragments->spec.ang.x, turn);
    for (object = fragments->spec.objects, index = 0; object;
         object = object->sibling) {
        index = fragments_make(object, NULL, fragments, index);
    }
    njPopMatrix(1);
    return fragments;
}

/* fragments_draw, fragments_smoke and fragments_move run their fragment_
 * function on each fragment. each returns 1 while any fragment isn't gone,
 * and 0 once all are */
static int fragments_draw(struct fragments *fragments)
{
    int left = 0;
    int count = fragments->count;
    struct fragment *piece = fragments->pieces;
    struct fragments_spec *spec = &fragments->spec;
    int i;

    if (spec->texlist) {
        njSetTexture(spec->texlist);
    }
    for (i = 0; i < count; i++) {
        if (fragment_draw(piece, spec)) {
            left++;
        }
        piece++;
    }
    if (left == 0) {
        return 0;
    } else {
        return 1;
    }
}

static int fragments_smoke(struct fragments *fragments)
{
    int left = 0;
    int count = fragments->count;
    struct fragments_spec *spec = &fragments->spec;
    struct fragment *piece = fragments->pieces;
    int i;

    for (i = 0; i < count; i++) {
        if (fragment_smoke(piece, spec)) {
            left++;
        }
        piece++;
    }
    if (left == 0) {
        return 0;
    } else {
        return 1;
    }
}

static int fragments_move(struct fragments *fragments)
{
    int left = 0;
    int count = fragments->count;
    struct fragments_spec *spec = &fragments->spec;
    struct fragment *piece = fragments->pieces;
    int i;

    for (i = 0; i < count; i++) {
        if (fragment_move(piece, spec)) {
            left++;
        }
        piece++;
    }
    if (left == 0) {
        return 0;
    } else {
        return 1;
    }
}

static void fragments_free(struct fragments *fragments)
{
    if (fragments->spec.own_texlist) {
        njReleaseTexture(fragments->spec.own_texlist);
    }
    heap_free(fragments->pieces);
    heap_free(fragments);
}

static void fragments_start(struct task *task)
{
    task->step = fragments_run;
    task->draw = fragments_task_draw;
    task->destructor = fragments_destroy;
}

static void fragments_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->fragments) {
        fragments_free(VARS(work)->fragments);
    }
}

static void fragments_task_draw(struct task *task)
{
    struct task_work *work = task->work;

    fragments_draw(VARS(work)->fragments);
}

static void fragments_run(struct task *task)
{
    struct task_work *work = task->work;

    fragments_draw(VARS(work)->fragments);
    if (!fragments_smoke(VARS(work)->fragments) ||
        !fragments_move(VARS(work)->fragments)) {
        task_kill(task);
    }
}

struct task *fragments_scatter(NJS_POINT3 *pos, struct angles *ang,
                               NJS_VECTOR *speed, NJS_VECTOR *accel,
                               struct fragments_spec *spec, Float ground)
{
    struct task *task = task_create(TASK_WORK, 3, fragments_start);

    if (task) {
        struct task_work *work = task->work;

        work->pos = *pos;
        VARS(work)->fragments =
            fragments_create(pos, ang, speed, accel, spec, ground);
        vibration_start(0, 6, 30, 0);
        sound_play(SE_BREAK, 0, 0, 0);
        return task;
    } else {
        return NULL;
    }
}
