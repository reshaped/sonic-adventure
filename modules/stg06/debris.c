/* the debris, a model tree broken into its nodes. the nodes fly apart, tumble
 * and trail dust. a node that lands vanishes, and a flame starts where it lies.
 * the whirl pulls the nearest player round a turning point. sky_deck_hop lifts
 * player 0 */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "effect/flame.h"
#include "player/player.h"
#include "stg06/debris.h"
#include "stg06/models.h"
#include "stg06/wind.h"
#include "system/task.h"
#include "system/util.h"

/* a node of the tree, flying on its own */
struct debris_piece {
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    NJS_MODEL *model;
    struct angles ang;
    struct angles spin; /* added to ang each step it flies */
    Uint32 flags;
};

/* the bits of a piece's flags */
#define PIECE_GONE 0x1
#define PIECE_LANDED 0x2 /* on the floor, sliding along it */
/* the steps a landed piece has lain on the floor, in spin.x, which only a
 * flying piece reads. the piece is gone once that passes PIECE_REST */
#define LANDED_STEPS(piece) ((piece)->spin.x)
#define PIECE_REST 40

/* the heap bytes the create gives each piece, 4 more than it uses */
#define PIECE_ROOM 60

struct debris {
    int count; /* how many pieces there are */
    struct debris_piece *pieces;
    struct debris_spec spec; /* a copy, with the create's arguments added */
};

/* the dust the flying pieces trail, white. debris_piece_smoke sets its scale,
 * pos and speed for each puff */
static struct dust_params debris_dust = {1.0F,
                                         0.038F,
                                         0.19F,
                                         0.0F,
                                         0.01F,
                                         {0.0F, 0.0F, 0.0F},
                                         {0.0F, 0.0F, 0.0F},
                                         {1.0F, 1.0F, 1.0F, 1.0F}};
/* the factor on the wind the create adds to accel, and the wind's least push
 * along x */
static Float debris_wind_scale = 1.7F;
static Float debris_wind_least = 0.01F;
/* the wind's push up */
#define WIND_LIFT 0.01F

/* range and strength size the pulls, as sky_deck_whirl_create says. radius is
 * the turning point's distance from pos */
struct whirl_vars {
    Float range;
    Float strength;
    Float radius;
};

#define WHIRL(work) ((struct whirl_vars *)&(work)->vars)
/* the number of the player the whirl pulls */
#define WHIRL_PLAYER(work) ((work)->tick)
/* the turn the whirl's point takes each step */
#define WHIRL_TURN(work) ((work)->ang.x)

/* the whirl rises WHIRL_RISE each step the player is within WHIRL_NEAR of its
 * height */
#define WHIRL_NEAR 10.0F
#define WHIRL_RISE 10.0F

static void debris_destroy(struct task *task);
static void debris_draw(struct task *task);
static void debris_step(struct task *task);

/* set wind to the push at pos, and return the wind's strength there. the push
 * is that strength's absolute value plus debris_wind_least along x, and
 * WIND_LIFT up */
static Float debris_wind(NJS_POINT3 *pos, NJS_VECTOR *wind)
{
    Float strength = sky_deck_wind_strength(pos->x, pos->z);

    wind->x = njAbs(strength) + debris_wind_least;
    wind->z = 0.0F;
    wind->y = WIND_LIFT;
    return strength;
}

void sky_deck_hop(struct task_work *work)
{
    struct task *toucher = work->collision->hit->owner;
    struct task_work *other = toucher->work;
    NJS_VECTOR to;
    NJS_VECTOR speed;
    struct angles ang;

    /* @hack redundant: ang, the bearing from work to the task that hit it,
     * which nothing reads, from atan2f at 0x0c91e13c; without it, 68 of 69
     * words differ and the function ends 68 bytes early */
    to.x = other->pos.x - work->pos.x;
    to.y = 0.0F;
    to.z = other->pos.z - work->pos.z;
    ang.x = 0;
    ang.y = (Angle)(atan2f(to.x, to.z) * ANGLE_PER_RADIAN);
    ang.z = 0;
    speed.x = 0.0F;
    speed.y = 2.0F;
    speed.z = 0.0F;
    player_set_speed(0, speed.x, speed.y, speed.z);
    other->pos.y += 1.0F;
}

/* count object and the nodes below it, but none below a node with
 * NJD_EVAL_BREAK */
static int debris_count(NJS_OBJECT *object)
{
    NJS_OBJECT *child;
    int count = 1;

    child = (object->evalflags & NJD_EVAL_BREAK) ? NULL : object->child;
    while (child) {
        count += debris_count(child);
        child = child->sibling;
    }
    return count;
}

/* make the piece at index for object, and one for each node below it. the
 * current matrix places them. parent is the piece of object's parent, whose ang
 * adds to object's, or null for a root. return the index after the last piece
 * made */
static int debris_break(NJS_OBJECT *object, struct debris_piece *parent,
                        struct debris *debris, int index)
{
    NJS_OBJECT *child =
        (object->evalflags & NJD_EVAL_BREAK) ? NULL : object->child;
    struct debris_piece *piece = &debris->pieces[index++];
    NJS_VECTOR out;
    Float scatter;
    Float reach;
    Float scale;
    Float flight;
    Float length;
    Angle ang;

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
        piece->ang.x = object->ang[0] + debris->spec.ang.x;
        piece->ang.y = object->ang[1] + debris->spec.ang.y;
        piece->ang.z = object->ang[2] + debris->spec.ang.z;
    }
    /* @bug logic: a piece less than 0.1 above the floor is put at a height of
     * 0.1, not 0.1 above the floor: the test reads the floor (0x0c91e2c4 to
     * 0x0c91e2d4), and the store at 0x0c91e2dc doesn't. each cannon whose
     * spec sets unk_38[1] above 0 passes its own height for the floor, and
     * SET0602S.BIN places five such cannons at -200. a piece that starts less
     * than 0.1 above one's base is put about 200 above it */
    if (debris->spec.unk_38[1] > 0.0F &&
        piece->pos.y - debris->spec.floor - 0.1F < 0.0F) {
        piece->pos.y = 0.1F;
    }
    flight = debris->spec.burst_speed * 2.0F;
    /* @hack spelling: scatter doubled in a statement of its own; doubled as
     * it's loaded, 26 of 326 words differ */
    scatter = debris->spec.scatter;
    scatter *= 2.0F;
    /* @hack spelling: a Float at a time through byte cursors, as the image
     * reads them, origin stepping by a sum, which keeps it in the image's
     * register; through the members, the function runs 2 bytes long and 210
     * of 326 words differ */
    {
        char *to = (char *)&out;
        char *from = (char *)&piece->pos;
        char *origin = (char *)&debris->spec.pos;

        *(Float *)to = *(Float *)from - *(Float *)origin;
        from += sizeof(Float);
        origin = origin + sizeof(Float);
        to += sizeof(Float);
        *(Float *)to = *(Float *)from - *(Float *)origin;
        from += sizeof(Float);
        origin = origin + sizeof(Float);
        to += sizeof(Float);
        *(Float *)to = *(Float *)from - *(Float *)origin;
    }
    reach = debris->spec.burst_radius;
    if (reach != 0.0F) {
        scale = flight / reach;
    } else {
        reach = 1.0F;
        scale = 0.0F;
    }
    length = njScalor(&out);
    out.x *= scale;
    out.y *= scale;
    out.z *= scale;
    reach = length / reach;
    piece->spin.x = (Angle)(((rand() * RAND_SCALE) - 0.5F) *
                            ((reach * 0.2F) + (rand() * (0.5F * RAND_SCALE))) *
                            (ANGLE_PER_DEGREE * 10.0F));
    piece->spin.y = (Angle)(((rand() * RAND_SCALE) - 0.5F) *
                            ((reach * 0.2F) + (rand() * (0.5F * RAND_SCALE))) *
                            (ANGLE_PER_DEGREE * 10.0F));
    piece->spin.z = (Angle)(((rand() * RAND_SCALE) - 0.5F) *
                            ((reach * 0.2F) + (rand() * (0.5F * RAND_SCALE))) *
                            (ANGLE_PER_DEGREE * 10.0F));
    piece->speed.x = out.x + (scatter * ((rand() * RAND_SCALE) - 0.5F)) +
                     debris->spec.speed.x;
    piece->speed.y = out.y + (scatter * ((rand() * RAND_SCALE) - 0.5F)) +
                     debris->spec.speed.y;
    piece->speed.z = out.z + (scatter * ((rand() * RAND_SCALE) - 0.5F)) +
                     debris->spec.speed.z;
    piece->flags = 0;
    while (child) {
        index = debris_break(child, piece, debris, index);
        child = child->sibling;
    }
    njPopMatrix(1);
    return index;
}

/* draw the piece while it flies. return 0 once it's gone */
static int debris_piece_draw(struct debris_piece *piece,
                             struct debris_spec *spec)
{
    Angle ang;

    if (!(piece->flags & PIECE_GONE)) {
        if (!(piece->flags & PIECE_LANDED)) {
            njPushMatrix(NULL);
            njTranslateV(NULL, &piece->pos);
            TURN_VIA(njRotateZ, piece->ang.z, ang);
            TURN_VIA(njRotateY, piece->ang.y, ang);
            TURN_VIA(njRotateX, piece->ang.x, ang);
            draw_model(piece->model);
            njPopMatrix(1);
        }
        return 1;
    }
    return 0;
}

/* start a flame the step after the piece lands. while it flies, puff dust from
 * its middle on a fifth of its steps, at random. return 0 once it's gone */
static int debris_piece_smoke(struct debris_piece *piece,
                              struct debris_spec *spec)
{
    if (!(piece->flags & PIECE_GONE)) {
        if (piece->flags & PIECE_LANDED) {
            if (LANDED_STEPS(piece) == 0) {
                NJS_VECTOR speed;

                speed.x = piece->speed.x * 0.2F;
                speed.y = 1.0F;
                speed.z = piece->speed.z * 0.2F;
                flame_create(&piece->pos, &speed,
                             1.0F + (piece->model->r * 0.8F));
            }
        } else if (rand() * RAND_SCALE < 0.2F) {
            NJS_POINT3 pos;
            NJS_VECTOR speed;

            pos.x = piece->pos.x + piece->model->center.x;
            pos.y = piece->pos.y + piece->model->center.y;
            pos.z = piece->pos.z + piece->model->center.z;
            speed.x = 1.2F + (piece->speed.x * 0.2F);
            speed.y = piece->speed.y * 0.2F;
            speed.z = piece->speed.z * 0.2F;
            debris_dust.scale =
                (piece->model->r * 0.2F) + (rand() * (0.3F * RAND_SCALE));
            debris_dust.speed = speed;
            debris_dust.pos = pos;
            dust_create_from(&debris_dust);
        }
        return 1;
    }
    return 0;
}

/* move the piece a step, and return 0 once it's gone. a flying piece moves,
 * takes accel, slows by drag and turns by its spin. it is gone once farther
 * than range from the tree's pos. a piece whose move would cross the floor
 * lands instead. a landed piece moves along x and z alone. it is gone once it
 * has lain there over PIECE_REST steps */
static int debris_piece_move(struct debris_piece *piece,
                             struct debris_spec *spec)
{
    if (!(piece->flags & PIECE_GONE)) {
        if ((piece->flags & PIECE_LANDED) ||
            ((piece->pos.y >= spec->floor &&
              piece->pos.y + piece->speed.y <= spec->floor) ||
             (piece->pos.y <= spec->floor &&
              piece->pos.y + piece->speed.y >= spec->floor))) {
            piece->pos.x += piece->speed.x;
            piece->pos.z += piece->speed.z;
            piece->speed.x += spec->accel.x;
            piece->speed.z += spec->accel.z;
            piece->speed.x *= spec->drag;
            piece->speed.z *= spec->drag;
            if (piece->flags & PIECE_LANDED) {
                LANDED_STEPS(piece)++;
                if (LANDED_STEPS(piece) > PIECE_REST) {
                    piece->flags |= PIECE_GONE;
                }
            } else {
                piece->flags |= PIECE_LANDED;
                LANDED_STEPS(piece) = 0;
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
            if (njDistanceP2P(&piece->pos, &spec->pos) > spec->range) {
                piece->flags |= PIECE_GONE;
            }
        }
        return 1;
    }
    return 0;
}

/* a record holding the pieces of spec's tree and a copy of spec. it adds to the
 * copy the wind at pos and its own pos, ang, speed, accel and floor */
static struct debris *debris_create(NJS_POINT3 *pos, struct angles *ang,
                                    NJS_VECTOR *speed, NJS_VECTOR *accel,
                                    Float floor, struct debris_spec *spec)
{
    /* @hack spelling: floor copied to a local, which the image keeps in its
     * frame; read as floor, 8 of 186 words differ */
    Float start_floor = floor;
    struct debris *debris;
    NJS_OBJECT *object;
    int count;
    int index;
    NJS_VECTOR wind;
    Angle turn;

    for (count = 0, object = spec->object; object; object = object->sibling) {
        count += debris_count(object);
    }
    debris = heap_alloc(sizeof(struct debris));
    /* @bug memory: the record's NULL is tested (TST R14,R14 at 0x0c91e8ac) only
     * after the count and the pieces are stored through it: the count at
     * 0x0c91e8a6, and the pieces in the branch's slot at 0x0c91e8b0 */
    debris->count = count;
    debris->pieces = heap_alloc(count * PIECE_ROOM);
    if (!debris) {
        return NULL;
    }
    debris->spec = *spec;
    /* @hack redundant: the flags tested to no effect (TST R1,R1 at 0x0c91e8f4);
     * without it, 147 of 186 words differ and the function ends 6 bytes
     * early */
    if (debris->spec.flags) {
    }
    debris_wind(pos, &wind);
    debris->spec.floor += start_floor;
    debris->spec.pos.x += pos->x;
    debris->spec.pos.y += pos->y;
    debris->spec.pos.z += pos->z;
    debris->spec.speed.x += speed->x;
    debris->spec.speed.y += speed->y;
    debris->spec.speed.z += speed->z;
    debris->spec.accel.x += accel->x + (wind.x * debris_wind_scale);
    debris->spec.accel.y += accel->y + (wind.y * debris_wind_scale);
    debris->spec.accel.z += accel->z + (wind.z * debris_wind_scale);
    debris->spec.ang.x += ang->x;
    debris->spec.ang.y += ang->y;
    debris->spec.ang.z += ang->z;
    njPushMatrix(&_unit_matrix_);
    /* @hack type: a bare 0 for the matrix; as NULL, 8 of 186 words differ */
    njTranslateV(0, &debris->spec.pos);
    TURN_VIA(njRotateY, debris->spec.ang.y, turn);
    object = debris->spec.object;
    index = 0;
    while (object) {
        index = debris_break(object, NULL, debris, index);
        object = object->sibling;
    }
    njPopMatrix(1);
    return debris;
}

/* draw each piece that flies, and return 0 once every piece is gone. with any
 * texlist in the spec, set obj_skydeck_texlist first. the spec's own list is
 * never read */
static int debris_draw_pieces(struct debris *debris)
{
    int live = 0;
    int count = debris->count;
    struct debris_piece *piece = debris->pieces;
    struct debris_spec *spec = &debris->spec;
    int i;

    if (spec->texlist) {
        njSetTexture(&obj_skydeck_texlist);
    }
    for (i = 0; i < count; i++) {
        if (debris_piece_draw(piece, spec)) {
            live++;
        }
        piece++;
    }
    if (!live) {
        return 0;
    }
    return 1;
}

static int debris_smoke_pieces(struct debris *debris)
{
    int live = 0;
    int count = debris->count;
    struct debris_piece *piece = debris->pieces;
    struct debris_spec *spec = &debris->spec;
    int i;

    for (i = 0; i < count; i++) {
        if (debris_piece_smoke(piece, spec)) {
            live++;
        }
        piece++;
    }
    if (!live) {
        return 0;
    }
    return 1;
}

static int debris_move_pieces(struct debris *debris)
{
    int live = 0;
    int count = debris->count;
    struct debris_piece *piece = debris->pieces;
    struct debris_spec *spec = &debris->spec;
    int i;

    for (i = 0; i < count; i++) {
        if (debris_piece_move(piece, spec)) {
            live++;
        }
        piece++;
    }
    if (!live) {
        return 0;
    }
    return 1;
}

static void debris_free(struct debris *debris)
{
    /* @hack redundant: the flags tested to no effect, which keeps debris_free
     * too big to expand; without it, SHC expands it into debris_destroy, its
     * one caller, and omits it, and 14 of 14 words differ */
    if (debris->spec.flags) {
    }
    heap_free(debris->pieces);
    heap_free(debris);
}

struct debris_vars {
    struct debris *debris;
};

#define VARS(work) ((struct debris_vars *)&(work)->vars)

static void debris_start(struct task *task)
{
    task->step = debris_step;
    task->draw = debris_draw;
    task->destructor = debris_destroy;
}

static void debris_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->debris) {
        debris_free(VARS(work)->debris);
    }
}

static void debris_draw(struct task *task)
{
    struct task_work *work = task->work;

    debris_draw_pieces(VARS(work)->debris);
}

/* draw, smoke and move the pieces. the task ends once the smoke or the move
 * finds every piece gone */
static void debris_step(struct task *task)
{
    struct task_work *work = task->work;

    debris_draw_pieces(VARS(work)->debris);
    if (!debris_smoke_pieces(VARS(work)->debris) ||
        !debris_move_pieces(VARS(work)->debris)) {
        task_kill(task);
    }
}

struct task *sky_deck_debris_create(NJS_POINT3 *pos, struct angles *ang,
                                    NJS_VECTOR *speed, NJS_VECTOR *accel,
                                    Float floor, struct debris_spec *spec)
{
    struct task *task = task_create(TASK_WORK, 3, debris_start);

    if (task) {
        struct task_work *work = task->work;

        work->pos = *pos;
        VARS(work)->debris = debris_create(pos, ang, speed, accel, floor, spec);
        return task;
    }
    return NULL;
}

static void whirl_step(struct task *task);
static void whirl_destroy(struct task *task);

static void whirl_start(struct task *task)
{
    task->step = whirl_step;
    task->destructor = whirl_destroy;
}

/* pull the player toward the turning point and toward the whirl's pos, as
 * sky_deck_whirl_create says. swing the player's facing to and fro.
 * TASK_TIMER(work) counts the steps left */
static void whirl_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[WHIRL_PLAYER(work)];
    NJS_POINT3 point;
    NJS_VECTOR to_point;
    NJS_VECTOR to_middle;
    Float point_distance;
    Float middle_distance;
    Float scale;
    Angle ang;

    point.x = 0.0F;
    point.y = 0.0F;
    point.z = WHIRL(work)->radius;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcPoint(NULL, &point, &point);
    njPopMatrix(1);
    /* @hack spelling: to_point worked a Float at a time through cursors; by
     * member, 196 of 247 words differ */
    {
        Float *to = &to_point.x;
        Float *from = &point.x;
        Float *at = &player->pos.x;

        *to++ = *from - *at;
        from++;
        at++;
        *to++ = *from - *at;
        from++;
        at++;
        *to = *from - *at;
    }
    /* @hack spelling: to_middle worked a Float at a time through cursors; by
     * member, 193 of 247 words differ */
    {
        Float *to = &to_middle.x;
        Float *at = &player->pos.x;
        Float *from = &work->pos.x;

        *to = *from - *at;
        to++;
        from++;
        at++;
        *to = *from - *at;
        to++;
        from++;
        at++;
        *to = *from - *at;
    }
    point_distance = njScalor(&to_point);
    middle_distance = njScalor(&to_middle);
    if (njAbs(to_middle.y) < WHIRL_NEAR) {
        work->pos.y += WHIRL_RISE;
    }
    if (point_distance < WHIRL(work)->range &&
        point_distance > WHIRL(work)->strength * 2.0F) {
        scale = WHIRL(work)->strength / point_distance;
        /* @hack spelling: to_point scaled through a cursor; by member, 127 of
         * 247 words differ */
        {
            Float *walk = &to_point.x;

            *walk++ *= scale;
            *walk++ *= scale;
            *walk *= scale;
        }
    }
    if (middle_distance < WHIRL(work)->range &&
        middle_distance > WHIRL(work)->strength * 2.0F) {
        scale = WHIRL(work)->strength / middle_distance * 0.5F;
        /* @hack spelling: to_middle scaled through a cursor; by member, 98 of
         * 247 words differ */
        {
            Float *walk = &to_middle.x;

            *walk++ *= scale;
            *walk++ *= scale;
            *walk *= scale;
        }
    }
    /* @hack spelling: to_middle added to the player's accel a Float at a time
     * through cursors; by member, 111 of 247 words differ and the function ends
     * 24 bytes late */
    {
        Float *to = &player_motion[WHIRL_PLAYER(work)]->accel.x;
        Float *from = &to_middle.x;

        *to += *from;
        to++;
        from++;
        *to += *from;
        to++;
        from++;
        *to += *from;
    }
    /* @hack spelling: to_point added to the player's accel a Float at a time
     * through cursors; by member, 89 of 247 words differ and the function ends
     * 24 bytes late */
    {
        Float *to = &player_motion[WHIRL_PLAYER(work)]->accel.x;
        Float *from = &to_point.x;

        *to += *from;
        to++;
        from++;
        *to += *from;
        to++;
        from++;
        *to += *from;
    }
    player->ang.y +=
        (Angle)(njSin((Angle)(TASK_TIMER(work) * (ANGLE_PER_DEGREE * 5.0F))) *
                (ANGLE_PER_DEGREE * 20.0F));
    work->ang.y += WHIRL_TURN(work);
    if (TASK_TIMER(work)-- == 0) {
        task_kill(task);
    }
}

static void whirl_destroy(struct task *task)
{
}

struct task *sky_deck_whirl_create(NJS_POINT3 *pos, Float radius, Float range,
                                   Angle turn, Sint16 steps, Float strength)
{
    struct task *task;
    struct task_work *work;
    struct task_work *player;
    NJS_VECTOR to;

    task = task_create(TASK_WORK, 3, whirl_start);
    if (!task) {
        return task;
    }
    work = task->work;
    WHIRL_PLAYER(work) = player_nearest(pos);
    work->pos = *pos;
    WHIRL(work)->range = range;
    WHIRL(work)->radius = radius;
    player = player_work[WHIRL_PLAYER(work)];
    /* @hack spelling: to worked a Float at a time through cursors, the first
     * converted from its address; by member, 84 of 92 words differ */
    {
        Float *out = (Float *)&to;
        Float *from = &player->pos.x;
        Float *at = &work->pos.x;

        *out = *from - *at;
        out++;
        from++;
        at++;
        *out = *from - *at;
        out++;
        from++;
        at++;
        *out = *from - *at;
    }
    work->ang.y = (Angle)(atan2f(to.x, to.z) * ANGLE_PER_RADIAN);
    WHIRL_TURN(work) = turn;
    work->timer = steps;
    WHIRL(work)->strength = strength;
    return task;
}
