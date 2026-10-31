/* helpers Casinopolis's objects share, and its props */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "player/knuckles.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/helpers.h"
#include "system/frame.h"
#include "system/loader.h"
#include "system/task.h"
#include "system/util.h"

/* destroyed beyond about 600 units of the camera and player 0 */
#define RANGE ((600.0F * 600.0F) + 10.0F)

/* the margin casino_world_add, casino_world_set_model and
 * casino_world_test_near add to the radius they're given */
#define NEAR_MARGIN 16.0F
/* what scroll_prop_draw, shift_prop_draw and swap_prop_draw add to the
 * model's radius for their view test */
#define VIEW_MARGIN 16.0F

/* a work's first two vars: casino_world_add's copy in the collision world,
 * and model_prop_start's model */
struct world_vars {
    NJS_OBJECT *copy;
    NJS_MODEL *model;
};

#define WORLD(work) ((struct world_vars *)&(work)->vars)

/* MODEL_REACH is the radius of a prop's copy. it is the model's radius, plus
 * its center's distance from the origin and REACH_MARGIN */
#define REACH_MARGIN 20.0F
#define MODEL_REACH(model)                                                     \
    ((model)->r + njScalor(&(model)->center) + REACH_MARGIN)
/* MODEL_REACH as world_prop_start, shift_world_prop_start and
 * shift_motion_prop_start keep it in the work's timer, rounded down to a
 * whole number */
#define REACH(work) ((Uint16)(work)->timer)

/* a work's vars for a prop of one tree */
struct prop_vars {
    NJS_OBJECT *copy; /* casino_world_add's, for world_prop_start's prop */
    NJS_OBJECT *object;
    Float range; /* squared, for the ranged props */
};

#define PROP(work) ((struct prop_vars *)&(work)->vars)

struct motion_vars {
    Uint8 unk_08[4];
    NJS_ACTION *action;
};

#define MOTION(work) ((struct motion_vars *)&(work)->vars)

struct spin_vars {
    /* turn[0] is the prop's turn about y so far. each step adds turn[1],
     * the step spin_prop_start is given, and scl.x degrees to it */
    /* @hack type: an array, not two members. spin_prop_run forms its address
     * once (ADD #8 at 0x0c90d904) to read both. as two members, SHC reads each
     * at its offset in the work, the step runs 2 bytes shorter, and 29 of
     * its 43 words differ */
    Uint16 turn[2];
    NJS_OBJECT *object;
};

#define SPIN(work) ((struct spin_vars *)&(work)->vars)

/* a pushed prop's action plays once player 0 touches the prop. player 0 must
 * have come more than PUSH_NEARER nearer over its last PUSH_STEPS steps */
#define PUSH_NEARER 0.2F
#define PUSH_STEPS 2

struct pushed_vars {
    NJS_ACTION *action;
    NJS_OBJECT *object; /* drawn while the action is at rest */
    Float frame; /* the action's, 0 at rest */
};

#define PUSHED(work) ((struct pushed_vars *)&(work)->vars)

struct scroll_vars {
    Sint32 se;
    NJS_OBJECT *object;
    struct uv_scroll *rows;
};

#define SCROLL(work) ((struct scroll_vars *)&(work)->vars)

struct shift_vars {
    NJS_OBJECT *copy;
    NJS_OBJECT *object;
    struct uv_shift *rows;
};

#define SHIFT(work) ((struct shift_vars *)&(work)->vars)

struct shift_motion_vars {
    NJS_OBJECT *copy;
    NJS_ACTION *action;
    struct uv_shift *rows;
};

#define SHIFT_MOTION(work) ((struct shift_motion_vars *)&(work)->vars)

struct swap_vars {
    NJS_OBJECT *copy;
    NJS_OBJECT *object;
    struct texture_swap *rows;
};

#define SWAP(work) ((struct swap_vars *)&(work)->vars)

/* @hack type: casino_carry_turn's and casino_dust_toward_camera's vectors as
 * this union, whose coords the vector helpers take. as NJS_VECTOR locals,
 * 167 of the 367 words of the two functions differ */
union vector_coords {
    NJS_VECTOR vector;
    Float coords[3];
};

/* a vector's coordinates, as the vector helpers take them */
#define COORDS(v) (&(v)->x)

/* what casino_carry_turn's views cast a union's address to */
typedef Float coords_array[3];

Bool casino_any_below(Float y)
{
    if (player_work[0] && player_work[0]->pos.y <= y) {
        return TRUE;
    }
    if (player_work[1] && player_work[1]->pos.y <= y) {
        return TRUE;
    }
    if (camera_work->pos.y <= y) {
        return TRUE;
    }
    return FALSE;
}

Bool casino_any_above(Float y)
{
    if (player_work[0] && player_work[0]->pos.y >= y) {
        return TRUE;
    }
    if (player_work[1] && player_work[1]->pos.y >= y) {
        return TRUE;
    }
    if (camera_work->pos.y >= y) {
        return TRUE;
    }
    return FALSE;
}

Bool casino_any_in_cylinder(NJS_POINT3 *center, Float radius2, Float height)
{
    struct task_work *work;
    Float top;
    Float bottom;
    Float dx;
    Float dz;
    Float distance2;

    top = center->y + height;
    bottom = center->y - height;
    work = player_work[0];
    if (work && bottom <= work->pos.y && work->pos.y <= top) {
        dx = center->x - work->pos.x;
        dz = center->z - work->pos.z;
        distance2 = (dx * dx) + (dz * dz);
        if (distance2 <= radius2) {
            return TRUE;
        }
    }
    work = player_work[1];
    if (work && bottom <= work->pos.y && work->pos.y <= top) {
        dx = center->x - work->pos.x;
        dz = center->z - work->pos.z;
        distance2 = (dx * dx) + (dz * dz);
        if (distance2 <= radius2) {
            return TRUE;
        }
    }
    work = camera_work;
    if (bottom <= work->pos.y && work->pos.y <= top) {
        dx = center->x - work->pos.x;
        dz = center->z - work->pos.z;
        distance2 = (dx * dx) + (dz * dz);
        if (distance2 <= radius2) {
            return TRUE;
        }
    }
    return FALSE;
}

void casino_world_add(struct task *task, NJS_OBJECT *object, Float radius,
                      Sint32 flags)
{
    struct task_work *work = task->work;
    NJS_OBJECT *copy;
    Float reach;

    WORLD(work)->copy = copy = world_object_alloc();
    copy->evalflags = 0;
    copy->scl[0] = 1.0F;
    copy->scl[1] = 1.0F;
    copy->scl[2] = 1.0F;
    copy->ang[0] = 0;
    copy->ang[1] = work->ang.y;
    copy->ang[2] = 0;
    copy->pos[0] = work->pos.x;
    copy->pos[1] = work->pos.y;
    copy->pos[2] = work->pos.z;
    copy->model = object->model;
    copy->child = object->child;
    copy->sibling = NULL;
    work->flags &= ~WORLD_TESTED;
    flags |= WORLD_SOLID;
    reach = radius + NEAR_MARGIN;
    if (reach <= WORLD_SMALL_REACH) {
        flags |= WORLD_SMALL;
    } else if (reach <= WORLD_MEDIUM_REACH) {
        flags |= WORLD_MEDIUM;
    } else if (reach <= WORLD_LARGE_REACH) {
        flags |= WORLD_LARGE;
    }
    world_add(flags, task, copy);
}

void casino_world_remove(struct task *task)
{
    struct task_work *work = task->work;

    if (WORLD(work)->copy) {
        world_remove(task, WORLD(work)->copy);
        world_object_free(WORLD(work)->copy);
    }
}

void casino_world_set_model(struct task *task, NJS_MODEL *model, Float radius)
{
    struct task_work *work = task->work;
    NJS_OBJECT *copy = WORLD(work)->copy;
    /* @hack redundant: a point nothing reads, which gives the image's 28-byte
     * stack frame (ADD #-28 at 0x0c90c02a). without it, 2 of its 86 words
     * differ */
    NJS_POINT3 unused;
    NJS_POINT3 center;
    Angle ang;

    if (model) {
        if (copy) {
            copy->model = model;
            /* @hack spelling: the center read through copy, not from model.
             * work then takes R13 and copy R14 (0x0c90c030, 0x0c90c034). from
             * model, the two trade, and 64 of its 86 words differ */
            center = copy->model->center;
            ang = work->ang.y;
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);
            TURN(njRotateY, ang);
            njCalcPoint(NULL, &center, &center);
            njPopMatrix(1);
            /* @hack type: copy->pos, a Float[3], stored in place as an
             * NJS_POINT3, which SHC moves as words (MOV.L at 0x0c90c080
             * to 0x0c90c090). without it, as three element stores, the function
             * runs 8 bytes shorter, and 47 of 86 words differ */
            *(NJS_POINT3 *)copy->pos = work->pos;
            copy->ang[1] = ang;
            if (player_in_range(&center, radius + NEAR_MARGIN)) {
                work->flags |= WORLD_TESTED;
            } else {
                work->flags &= ~WORLD_TESTED;
            }
            return;
        }
    } else if (copy) {
        copy->model = model;
    }
    work->flags &= ~WORLD_TESTED;
}

void casino_world_test_near(struct task *task, Float radius)
{
    struct task_work *work = task->work;
    NJS_OBJECT *copy = WORLD(work)->copy;
    /* @hack redundant: a point nothing reads, which gives the image's 28-byte
     * stack frame (ADD #-28 at 0x0c90c12c). without it, 2 of its 79 words
     * differ */
    NJS_POINT3 unused;
    NJS_POINT3 center;
    Angle ang;

    if (copy) {
        center = copy->model->center;
        ang = work->ang.y;
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, ang);
        njCalcPoint(NULL, &center, &center);
        njPopMatrix(1);
        /* @hack type: copy->pos, a Float[3], stored in place as an NJS_POINT3,
         * which SHC moves as words (MOV.L at 0x0c90c17c to 0x0c90c18c). without
         * it, as three element stores, 20 of 79 words differ */
        *(NJS_POINT3 *)copy->pos = work->pos;
        if (player_in_range(&center, radius + NEAR_MARGIN)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

/* @hack spelling: the vector helpers, these and vector_sum, walk coordinates
 * through Float pointers, which SHC loads with FMOV.S @Rm+, as at 0x0c90c244.
 * as field arithmetic, 391 of the 416 words of casino_turn_to and
 * casino_carry_turn differ */
#pragma inline(vector_difference, vector_subtract, vector_add, vector_clear)

/* to less from, into out */
static void vector_difference(const Float *from, const Float *to, Float *out)
{
    *out++ = *to++ - *from++;
    *out++ = *to++ - *from++;
    *out = *to - *from;
}

static void vector_subtract(const Float *by, Float *to)
{
    *to++ -= *by++;
    *to++ -= *by++;
    *to -= *by;
}

static void vector_add(const Float *by, Float *to)
{
    *to++ += *by++;
    *to++ += *by++;
    *to += *by;
}

static void vector_clear(Float *to)
{
    *to++ = 0.0F;
    *to++ = 0.0F;
    *to = 0.0F;
}

void casino_turn_to(struct task *task, NJS_VECTOR *move, Angle ang)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct task_force *force = task->force;
    Angle turn = ang - work->ang.y;
    NJS_VECTOR offset;
    NJS_VECTOR before;

    if (player) {
        vector_difference(COORDS(&work->pos), COORDS(&player->pos),
                          COORDS(&offset));
        before = offset;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, turn);
        njCalcVector(NULL, &offset, &offset);
        njPopMatrix(1);
        vector_subtract(COORDS(&before), COORDS(&offset));
    } else {
        vector_clear(COORDS(&offset));
    }
    /* @hack redundant: a goto to the next statement, which SHC deletes but
     * which has it load move into R2 for the test at 0x0c90c310. without it,
     * move takes R3, and 2 of its 130 words differ */
    goto moved;
moved:
    if (move) {
        vector_add(COORDS(move), COORDS(&offset));
    }
    force->speed = offset;
    force->turn.y = -turn;
    work->ang.y = ang;
}

#pragma inline(vector_sum)

/* at plus by, into out */
static void vector_sum(const Float *at, const Float *by, Float *out)
{
    *out++ = *by++ + *at++;
    *out++ = *by++ + *at++;
    *out = *by + *at;
}

/* casino_carry_turn moves a climbing Knuckles only while he's more than
 * CLIMB_AXIS across from the work's position. he must also be within
 * CLIMB_SLACK of radius */
#define CLIMB_AXIS 0.1F
#define CLIMB_SLACK 10.0F

void casino_carry_turn(struct task *task, NJS_VECTOR *move, Angle turn,
                       Float radius)
{
    struct task_work *player = player_work[0];
    struct task_work *work = task->work;
    struct player *record;
    struct task_force *force;
    Float distance;
    Float scale;
    union vector_coords offset;
    union vector_coords before;

    if (player && knuckles_get_activity(0) == KNUCKLES_CLIMBING) {
        record = player_get(0);
        if (record && record->ground_object == WORLD(work)->copy) {
            vector_difference(COORDS(&work->pos), COORDS(&player->pos),
                              offset.coords);
            njPushMatrix(&_unit_matrix_);
            TURN(njRotateY, turn);
            njCalcVector(NULL, &offset.vector, &offset.vector);
            njPopMatrix(1);
            distance = njSqrt((offset.vector.x * offset.vector.x) +
                              (offset.vector.z * offset.vector.z));
            if (distance > CLIMB_AXIS &&
                _builtin_fabsf(radius - distance) < CLIMB_SLACK) {
                scale = radius / distance;
                offset.vector.x *= scale;
                offset.vector.z *= scale;
                if (move) {
                    vector_add(COORDS(move), offset.coords);
                }
                vector_sum(COORDS(&work->pos), offset.coords,
                           COORDS(&player->pos));
                return;
            }
        }
    }
    force = task->force;
    force->turn.y = 0;
    if (player && (player->flags & PLAYER_ON_LAND)) {
        vector_difference(COORDS(&work->pos), COORDS(&player->pos),
                          offset.coords);
        before = offset;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, turn);
        njCalcVector(NULL, &offset.vector, &offset.vector);
        njPopMatrix(1);
        /* @hack type: before as an array through a cast of its address. force
         * then takes R10 and move R11 (0x0c90c4a2, 0x0c90c5b0). as
         * before.coords, the two trade, and 12 of its 286 words differ */
        vector_subtract(*(coords_array *)&before, offset.coords);
        force->turn.y = -turn;
    } else {
        /* @hack type: offset as an array through a cast of its address, which
         * SHC forms from R15 at each use, as at 0x0c90c5a0. as offset.coords,
         * SHC keeps the address in R14, and 275 of its 286 words differ */
        vector_clear(*(coords_array *)&offset);
    }
    if (move) {
        vector_add(COORDS(move), offset.coords);
    }
    force->speed = offset.vector;
}

void casino_draw_sphere(NJS_POINT3 *pos, Float radius)
{
    Float scale = radius * MARKER_FIT;

    njPushMatrix(NULL);
    njTranslateV(NULL, pos);
    njScale(NULL, scale, scale, scale);
    draw_tree(&marker_object);
    njPopMatrix(1);
}

void casino_draw_cylinder(NJS_POINT3 *pos, Float radius, Float height)
{
    Float scale = radius * MARKER_FIT;
    Float scale_y = height * MARKER_FIT;

    njPushMatrix(NULL);
    njTranslate(NULL, pos->x, pos->y, pos->z);
    njScale(NULL, scale, scale_y, scale);
    draw_tree(&marker_cylinder_object);
    njPopMatrix(1);
}

static void model_prop_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        draw_model(WORLD(work)->model);
        njPopMatrix(1);
    }
}

static void model_prop_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        draw_model(WORLD(work)->model);
        njPopMatrix(1);
        if (!placement_editing(task)) {
            collision_register(work);
        }
    }
}

void model_prop_start(struct task *task, NJS_MODEL *model,
                      struct collision_shape *shapes, int count)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            work->ang.x = 0;
            work->ang.z = 0;
            collision_init(task, shapes, COLLISION_OBJECT, 1, count);
        }
        task->draw = model_prop_draw;
        task->step = model_prop_run;
        WORLD(work)->model = model;
    }
}

/* a UV move is taken modulo 4096, and uv_scroll_move's phase modulo 1024 */
#define MOVE_MASK 0xfff
#define PHASE_MASK 0x3ff

/* copy count UVs of from into to, each moved by move_u and move_v */
static void uv_copy_moved(NJS_TEX *to, NJS_TEX *from, int move_u, int move_v,
                          int count)
{
    /* @hack spelling: byte cursors, stepped by a Sint16 within each NJS_TEX and
     * then on to the next. SHC reads u and v with @R1+ and adds 2 to out
     * between its stores (0x0c90c7f0 to 0x0c90c7fe). through the fields, SHC
     * reads v at @(2,Rn) and adds 4 to each pointer, and 17 of its 23 words
     * differ */
    unsigned char *out = (unsigned char *)to;
    const unsigned char *in = (const unsigned char *)from;

    move_u &= MOVE_MASK;
    move_v &= MOVE_MASK;
    while (count--) {
        /* @hack redundant: register, which gives the sums R4 and R5, and out
         * R14. without it, the sums take R5 and R0, out takes R1, and 23 of
         * its 23 words differ */
        register int sum_u;
        register int sum_v;

        sum_u = *(const Sint16 *)in;
        in += sizeof(Sint16);
        sum_v = *(const Sint16 *)in + move_v;
        sum_u += move_u;
        *(Sint16 *)out = sum_u;
        out += sizeof(Sint16);
        *(Sint16 *)out = sum_v;
        in -= sizeof(Sint16);
        in = (const unsigned char *)((const NJS_TEX *)in + 1);
        out -= sizeof(Sint16);
        out = (unsigned char *)((NJS_TEX *)out + 1);
    }
}

void uv_shift_make(struct uv_shift *rows, int count)
{
    int i;

    for (i = 0; i < count; i++, rows++) {
        if (rows->users++ == 0) {
            rows->own = (NJS_TEX *)rows->meshset->vertuv;
            rows->shifted = heap_alloc((rows->count + 1) * sizeof(NJS_TEX));
            while (!rows->shifted) {
            }
            uv_copy_moved(rows->shifted, rows->own, rows->shift_u,
                          rows->shift_v, rows->count);
        }
    }
}

void uv_shift_use(struct uv_shift *rows, int count)
{
    while (count--) {
        rows->meshset->vertuv = (NJS_COLOR *)rows->shifted;
        rows++;
    }
}

void uv_shift_restore(struct uv_shift *rows, int count)
{
    while (count--) {
        rows->meshset->vertuv = (NJS_COLOR *)rows->own;
        rows++;
    }
}

void uv_shift_free(struct uv_shift *rows, int count)
{
    int i;

    for (i = 0; i < count; i++, rows++) {
        if (--rows->users == 0) {
            heap_free(rows->shifted);
            rows->shifted = NULL;
        }
    }
}

void uv_scroll_save(struct uv_scroll *rows, int count)
{
    int i;

    for (i = 0; i < count; i++, rows++) {
        rows->own = (NJS_TEX *)rows->meshset->vertuv;
    }
}

void uv_scroll_move(struct uv_scroll *rows, int count, NJS_TEX *room, int phase)
{
    Sint16 scroll = phase;

    if (frame_step <= 0) {
        scroll &= PHASE_MASK;
        while (count--) {
            int move_u = scroll * rows->step_u;
            int move_v = scroll * rows->step_v;

            uv_copy_moved(room, rows->own, move_u, move_v, rows->count);
            rows->meshset->vertuv = (NJS_COLOR *)room;
            room += rows->count;
            rows++;
        }
    }
}

void uv_scroll_restore(struct uv_scroll *rows, int count)
{
    while (count--) {
        rows->meshset->vertuv = (NJS_COLOR *)rows->own;
        rows++;
    }
}

void uv_scroll_release(struct uv_scroll *rows, int count)
{
}

void casino_blend_palette(Uint32 *from, Uint32 *to, Uint32 *out, Float blend)
{
    Float from_share;
    /* @hack spelling: copies of from and to, which SHC steps in R7 and R14 with
     * the count in R0 (0x0c90ca04 to 0x0c90ca0a). stepping the parameters, it
     * counts in R13, and 23 of its 116 words differ */
    Uint32 *in_from = from;
    Uint32 *in_to = to;
    int count = DRAW_PALETTE_ENTRIES * 2;
    /* @hack type: the words taken signed, which SHC shifts arithmetically (SHAD
     * at 0x0c90ca48, EXTS.W at 0x0c90ca78). as Uint32, the function runs
     * shorter, and 109 of its 116 words differ */
    Sint32 base;
    Sint32 toward;
    Sint32 color;

    /* @hack spelling: from_share set in two statements. base and toward then
     * load into R4 and R5 (0x0c90ca40, 0x0c90ca44). as 1.0F - blend, the two
     * trade, and 10 of its 116 words differ */
    from_share = 1.0F;
    from_share -= blend;
    while (count--) {
        base = *in_from++;
        toward = *in_to++;
        color = ((Uint32)(((Uint8)(base >> 8) * from_share) +
                          ((Uint8)(toward >> 8) * blend))
                 << 8) +
                ((Uint32)(((Uint8)(base >> 16) * from_share) +
                          ((Uint8)(toward >> 16) * blend))
                 << 16) +
                ((Uint32)(((Uint8)(base >> 24) * from_share) +
                          ((Uint8)(toward >> 24) * blend))
                 << 24) +
                (Uint32)(((Uint8)base * from_share) + ((Uint8)toward * blend));
        *out++ = color;
    }
}

void casino_blend_set_areas(Float blend)
{
    casino_blend_palette((Uint32 *)SET_AREA_4, (Uint32 *)SET_AREA_5,
                         draw_palettes[DRAW_PLAYER_PALETTE], blend);
}

void texture_swap_save(struct texture_swap *rows, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        rows->own = rows->material->attr_texId;
        rows++;
    }
}

void texture_swap_use(struct texture_swap *rows, int count)
{
    while (count--) {
        rows->material->attr_texId = rows->texture;
        rows++;
    }
}

void texture_swap_restore(struct texture_swap *rows, int count)
{
    while (count--) {
        rows->material->attr_texId = rows->own;
        rows++;
    }
}

void texture_swap_release(struct texture_swap *rows, int count)
{
}

void casino_dust_toward_camera(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale,
                               Float inset)
{
    union vector_coords at;
    Float distance;

    /* @hack spelling: byte cursors over pos and the camera's position. the last
     * walk then keeps at's in R5 and the camera's in R4 (0x0c90cc08
     * to 0x0c90cc30). stepped as Float pointers, the two trade, and 17 of
     * its 81 words differ */
    {
        Float *out = at.coords;
        const char *from = (const char *)pos;
        const char *eye = (const char *)&camera_work->pos;

        *out++ = *(const Float *)from - *(const Float *)eye;
        from += sizeof(Float);
        eye += sizeof(Float);
        *out++ = *(const Float *)from - *(const Float *)eye;
        from += sizeof(Float);
        eye += sizeof(Float);
        *out = *(const Float *)from - *(const Float *)eye;
    }
    /* @hack type: at's address as a cast here, which SHC forms from R15 at each
     * use (0x0c90cbd0, 0x0c90cc2c). as &at.vector here too, SHC keeps it in
     * R14, and 69 of its 81 words differ */
    distance = njScalor((NJS_VECTOR *)&at);
    if (distance > inset) {
        Float *out = at.coords;

        distance = (distance - inset) / distance;
        *out++ *= distance;
        *out++ *= distance;
        *out *= distance;
    }
    {
        const char *eye = (const char *)&camera_work->pos;
        Float *out = at.coords;

        *out++ += *(const Float *)eye;
        eye += sizeof(Float);
        *out++ += *(const Float *)eye;
        eye += sizeof(Float);
        *out += *(const Float *)eye;
    }
    dust_create(&at.vector, speed, scale);
}

/* TRUE past a frame's first step, or while a sphere of radius about pos is
 * out of view */
static Bool out_of_view(NJS_POINT3 *pos, Float radius)
{
    if (frame_step > 0) {
        return TRUE;
    }
    return !draw_in_view(pos->x, pos->y, pos->z, radius);
}

static void scaled_prop_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE) && !frame_step) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        njScale(NULL, work->scl.x + 1.0F, work->scl.y + 1.0F,
                work->scl.z + 1.0F);
        draw_tree(PROP(work)->object);
        njPopMatrix(1);
    }
}

static void ranged_scaled_prop_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, PROP(work)->range) && !frame_step) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        njScale(NULL, work->scl.x + 1.0F, work->scl.y + 1.0F,
                work->scl.z + 1.0F);
        draw_tree(PROP(work)->object);
        njPopMatrix(1);
    }
}

static void scaled_zxy_prop_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE) && !frame_step) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        njScale(NULL, work->scl.x + 1.0F, work->scl.y + 1.0F,
                work->scl.z + 1.0F);
        draw_tree(PROP(work)->object);
        njPopMatrix(1);
    }
}

void scaled_zxy_prop_start(struct task *task, NJS_OBJECT *object)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        task->draw = scaled_zxy_prop_draw;
        task->step = scaled_zxy_prop_draw;
        PROP(work)->object = object;
    }
}

void scaled_prop_start(struct task *task, NJS_OBJECT *object)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        task->draw = scaled_prop_draw;
        task->step = scaled_prop_draw;
        PROP(work)->object = object;
    }
}

static void scaled_prop_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        scaled_prop_draw(task);
        if (!placement_editing(task)) {
            collision_register(work);
        }
    }
}

static void ranged_scaled_prop_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, PROP(work)->range)) {
        ranged_scaled_prop_draw(task);
        if (!placement_editing(task)) {
            collision_register(work);
        }
    }
}

/* scale the work's shapes by scl + 1, as its draw scales its tree. each axis
 * of scl below -1 is first raised to -1, so that no scale is negative. the
 * record's radius is then set again */
static void scale_shapes(struct task_work *work)
{
    Uint16 count = work->collision->count;
    Uint16 i;
    struct collision_shape *shape;

    for (i = 0; i < count; i++) {
        if (work->scl.x <= -1.0F) {
            work->scl.x = -1.0F;
        }
        if (work->scl.y <= -1.0F) {
            work->scl.y = -1.0F;
        }
        if (work->scl.z <= -1.0F) {
            work->scl.z = -1.0F;
        }
        /* @hack spelling: shape set in two statements, which read the shapes
         * from the work on each pass (0x0c90cfb0, 0x0c90cfb6). as &shapes[i],
         * SHC hoists the read out of the loop, and 79 of its 83 words differ */
        shape = work->collision->shapes;
        shape += i;
        shape->radius = shape->radius * (work->scl.x + 1.0F);
        shape->size_y = shape->size_y * (work->scl.y + 1.0F);
        shape->size_z = shape->size_z * (work->scl.z + 1.0F);
        shape->center.x = shape->center.x * (work->scl.x + 1.0F);
        shape->center.y = shape->center.y * (work->scl.y + 1.0F);
        shape->center.z = shape->center.z * (work->scl.z + 1.0F);
    }
    collision_set_radius(work->collision);
}

void scaled_solid_prop_start(struct task *task, NJS_OBJECT *object,
                             struct collision_shape *shapes, int count)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            work->ang.x = 0;
            work->ang.z = 0;
            collision_init(task, shapes, COLLISION_OBJECT, 1, count);
            scale_shapes(work);
        }
        task->draw = scaled_prop_draw;
        task->step = scaled_prop_run;
        PROP(work)->object = object;
    }
}

void ranged_scaled_solid_prop_start(struct task *task, Float range,
                                    NJS_OBJECT *object,
                                    struct collision_shape *shapes, int count)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, range)) {
        if (!placement_editing(task)) {
            work->ang.x = 0;
            work->ang.z = 0;
            collision_init(task, shapes, COLLISION_OBJECT, 1, count);
            scale_shapes(work);
        }
        task->draw = ranged_scaled_prop_draw;
        task->step = ranged_scaled_prop_run;
        PROP(work)->range = range;
        PROP(work)->object = object;
    }
}

/* an unlit prop's model must have at most MOST_MATERIALS materials.
 * unlit_prop_draw saves each one's attrflags on its stack, where an
 * eleventh's would overwrite the draw's saved return address */
#define MOST_MATERIALS 10

static void unlit_prop_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    NJS_MATERIAL *material;
    Uint32 attrflags[MOST_MATERIALS];
    int count;

    if (!placement_try_leave_by(task, PROP(work)->range) && !frame_step) {
        object = PROP(work)->object;
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        count = object->model->nbMat;
        material = object->model->mats;
        while (count--) {
            attrflags[count] = material->attrflags;
            if (work->submode) {
                material->attrflags = attrflags[count] & ~NJD_FLAG_IGNORE_LIGHT;
            } else {
                material->attrflags = attrflags[count] | NJD_FLAG_IGNORE_LIGHT;
            }
            material++;
        }
        draw_tree(object);
        count = object->model->nbMat;
        material = object->model->mats;
        while (count--) {
            material->attrflags = attrflags[count];
            material++;
        }
        njPopMatrix(1);
    }
}

static void unlit_prop_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, PROP(work)->range)) {
        unlit_prop_draw(task);
        if (placement_editing(task)) {
            work->submode = (int)work->scl.x & 1;
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> Light Ignore");
            return;
        }
        collision_register(work);
    }
}

void unlit_prop_start(struct task *task, Float range, NJS_OBJECT *object,
                      struct collision_shape *shapes, int count)
{
    struct task_work *work = task->work;

    range += 1.0F;
    if (!placement_try_leave_by(task, range)) {
        if (!placement_editing(task)) {
            work->ang.x = 0;
            work->ang.z = 0;
            collision_init(task, shapes, COLLISION_OBJECT, 1, count);
        }
        task->draw = unlit_prop_draw;
        task->step = unlit_prop_run;
        PROP(work)->object = object;
        PROP(work)->range = range;
        work->submode = (int)work->scl.x & 1;
    }
}

static void ranged_prop_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, PROP(work)->range) && !frame_step) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        draw_tree(PROP(work)->object);
        njPopMatrix(1);
    }
}

static void ranged_prop_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, PROP(work)->range)) {
        ranged_prop_draw(task);
        if (!placement_editing(task)) {
            collision_register(work);
        }
    }
}

void ranged_prop_start(Float range, struct task *task, NJS_OBJECT *object,
                       struct collision_shape *shapes, int count)
{
    struct task_work *work = task->work;

    range += 1.0F;
    if (!placement_try_leave_by(task, range)) {
        if (!placement_editing(task)) {
            work->ang.x = 0;
            work->ang.z = 0;
            collision_init(task, shapes, COLLISION_OBJECT, 1, count);
        }
        task->draw = ranged_prop_draw;
        task->step = ranged_prop_run;
        PROP(work)->object = object;
        PROP(work)->range = range;
    }
}

static void prop_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE) && !frame_step) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        draw_tree(PROP(work)->object);
        njPopMatrix(1);
    }
}

static void solid_prop_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        prop_draw(task);
        if (!placement_editing(task)) {
            collision_register(work);
        }
    }
}

void solid_prop_start(struct task *task, NJS_OBJECT *object,
                      struct collision_shape *shapes, int count)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            work->ang.x = 0;
            work->ang.z = 0;
            collision_init(task, shapes, COLLISION_OBJECT, 1, count);
        }
        task->draw = prop_draw;
        task->step = solid_prop_run;
        PROP(work)->object = object;
    }
}

static void world_prop_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        prop_draw(task);
        if (!placement_editing(task)) {
            casino_world_test_near(task, REACH(work));
        }
    }
}

static void world_prop_destroy(struct task *task)
{
    casino_world_remove(task);
}

void world_prop_start(struct task *task, NJS_OBJECT *object)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        work->timer = (Uint16)MODEL_REACH(object->model);
        task->draw = prop_draw;
        task->step = world_prop_run;
        task->destructor = world_prop_destroy;
        PROP(work)->object = object;
        if (!placement_editing(task)) {
            casino_world_add(task, object, REACH(work), 0);
        }
    }
}

static void motion_prop_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE) && !frame_step) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        draw_action(MOTION(work)->action, work->scl.x);
        njPopMatrix(1);
    }
}

static void motion_prop_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (work->scl.x >= MOTION(work)->action->motion->nbFrame - 1) {
            work->scl.x = 0.0F;
        }
        motion_prop_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "Mot Pitch");
            return;
        }
        collision_register(work);
        work->scl.x += work->scl.y;
    }
}

void motion_prop_start(struct task *task, NJS_ACTION *action,
                       struct collision_shape *shapes, int count)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            collision_init(task, shapes, COLLISION_OBJECT, 1, count);
        }
        task->draw = motion_prop_draw;
        task->step = motion_prop_run;
        MOTION(work)->action = action;
    }
}

static void spin_prop_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE) && !frame_step) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y + SPIN(work)->turn[0]);
        draw_tree(SPIN(work)->object);
        njPopMatrix(1);
    }
}

static void spin_prop_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        spin_prop_draw(task);
        SPIN(work)->turn[0] +=
            SPIN(work)->turn[1] + (int)(work->scl.x * ANGLE_PER_DEGREE);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "RotSpd");
        }
    }
}

void spin_prop_start(struct task *task, NJS_OBJECT *object, int step)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        task->draw = spin_prop_draw;
        task->step = spin_prop_run;
        SPIN(work)->object = object;
        SPIN(work)->turn[0] = 0;
        SPIN(work)->turn[1] = step;
    }
}

static void pushed_prop_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE) && !frame_step) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        if (PUSHED(work)->frame != 0.0F) {
            draw_action(PUSHED(work)->action, PUSHED(work)->frame);
        } else {
            draw_tree(PUSHED(work)->object);
        }
        njPopMatrix(1);
    }
}

static void pushed_prop_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task *other;
    NJS_POINT3 pos;
    NJS_POINT3 before;
    struct angles ang_before;
    Float speed;
    Float last;
    Float distance;
    Float distance_before;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    if (PUSHED(work)->frame != 0.0F) {
        draw_action(PUSHED(work)->action, PUSHED(work)->frame);
    } else {
        draw_tree(PUSHED(work)->object);
    }
    njPopMatrix(1);
    speed = _builtin_fabsf(work->scl.x);
    last = (Float)(PUSHED(work)->action->motion->nbFrame - 1);
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "AnimSpd");
        PUSHED(work)->frame += speed;
        if (PUSHED(work)->frame >= last) {
            PUSHED(work)->frame = 0.0F;
        }
    } else {
        if (PUSHED(work)->frame == 0.0F) {
            if (work->collision->flags & COLLISION_HIT) {
                other = work->collision->hit->owner;
                if (other->work == player_work[0] &&
                    ((player_work[0]->flags & PLAYER_ON_LAND) ||
                     !(player_work[0]->flags & PLAYER_ON_TASK))) {
                    player_get_history(0, PUSH_STEPS, &before, &ang_before);
                    pos = other->work->pos;
                    distance = njDistanceP2P(&pos, &work->pos);
                    distance_before = njDistanceP2P(&before, &work->pos);
                    if (distance_before - distance > PUSH_NEARER) {
                        PUSHED(work)->frame += speed;
                    }
                }
            }
        } else {
            PUSHED(work)->frame += speed;
        }
        if (PUSHED(work)->frame >= last) {
            PUSHED(work)->frame = 0.0F;
        }
        collision_register(work);
    }
}

void pushed_prop_start(struct task *task, NJS_OBJECT *object,
                       struct collision_shape *shapes, NJS_ACTION *action)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            work->ang.x = 0;
            work->ang.z = 0;
            collision_init(task, shapes, COLLISION_OBJECT, 1, 1);
        }
        task->draw = pushed_prop_draw;
        task->step = pushed_prop_run;
        PUSHED(work)->object = object;
        PUSHED(work)->action = action;
        PUSHED(work)->frame = 0.0F;
    }
}

/* the most UVs a scroll prop's rows can hold, which its draw copies to its
 * stack */
#define MOST_SCROLLED_UVS 200

static void scroll_prop_draw(struct task *task)
{
#ifdef GCC
    /* uv_scroll_move points the meshsets' vertuv here, and model.c's
     * clipped strips read each u and v pair as one longword, which faults
     * on an address off 4. GCC aligns this array only to its Sint16s */
    NJS_TEX room[MOST_SCROLLED_UVS] __attribute__((aligned(4)));
#else
    NJS_TEX room[MOST_SCROLLED_UVS];
#endif
    struct task_work *work = task->work;
    struct uv_scroll *rows = SCROLL(work)->rows;
    Sint16 phase = work->timer;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    Sint16 count = work->submode;

    if (!placement_try_leave_by(task, RANGE) && !frame_step) {
        if (!out_of_view(&work->pos,
                         SCROLL(work)->object->model->r + VIEW_MARGIN)) {
            uv_scroll_move(rows, count, room, phase);
            casino_set_texture(CASINO_TEXTURES_ACTS_12);
            njPushMatrix(NULL);
            njTranslateV(NULL, &work->pos);
            TURN(njRotateY, work->ang.y);
            draw_tree(SCROLL(work)->object);
            njPopMatrix(1);
            uv_scroll_restore(rows, count);
        }
    }
}

static void scroll_prop_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        scroll_prop_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "Speed");
        }
        work->timer += (int)work->scl.z;
    }
}

static void scroll_sound_prop_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        scroll_prop_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "Speed");
        }
        work->timer += (int)work->scl.z;
        if (!frame_step) {
            sound_play_at_for(SCROLL(work)->se, (int)work, 1, 0, work->pos.x,
                              work->pos.y, work->pos.z, 10);
        }
    }
}

static void scroll_prop_destroy(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: a read of work->collision that nothing uses. SHC drops
     * it, but keeps the load of task->work, into R4 in the RTS's delay slot at
     * 0x0c90de90. without it, 1 of its 2 words differs */
    struct collision_work *collision = work->collision;
}

void scroll_prop_start(struct task *task, NJS_OBJECT *object,
                       struct uv_scroll *rows, Sint8 count)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        SCROLL(work)->rows = rows;
        work->timer = 0;
        work->submode = count;
        SCROLL(work)->object = object;
        task->draw = scroll_prop_draw;
        task->step = scroll_prop_run;
        task->destructor = scroll_prop_destroy;
        uv_scroll_save(rows, count);
    }
}

void scroll_sound_prop_start(struct task *task, NJS_OBJECT *object,
                             struct uv_scroll *rows, Sint8 count, int se)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        SCROLL(work)->rows = rows;
        work->timer = 0;
        work->submode = count;
        SCROLL(work)->object = object;
        SCROLL(work)->se = se;
        task->draw = scroll_prop_draw;
        task->step = scroll_sound_prop_run;
        task->destructor = scroll_prop_destroy;
        uv_scroll_save(rows, count);
    }
}

static void shift_prop_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct uv_shift *rows = SHIFT(work)->rows;
    Sint8 count = work->tick;
    Sint8 phase = work->mode;

    if (placement_try_leave_by(task, RANGE) || frame_step) {
        return;
    }
    if (out_of_view(&work->pos, SHIFT(work)->object->model->r + VIEW_MARGIN)) {
        return;
    }
    if (phase) {
        uv_shift_use(rows + ((phase - 1) * count), count);
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_tree(SHIFT(work)->object);
    njPopMatrix(1);
    if (phase) {
        uv_shift_restore(rows + ((phase - 1) * count), count);
    }
}

static void shift_prop_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        shift_prop_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "Pitch");
        }
        if ((Uint16)work->timer++ > (Sint16)work->scl.z) {
            ++work->mode;
            if (work->mode > work->submode) {
                work->mode = 0;
            }
            work->timer = 0;
        }
    }
}

static void shift_prop_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct uv_shift *rows = SHIFT(work)->rows;

    uv_shift_free(rows, work->tick * work->submode);
}

void shift_prop_start(struct task *task, NJS_OBJECT *object,
                      struct uv_shift *rows, Sint8 count, Sint8 phases)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        SHIFT(work)->rows = rows;
        work->tick = count;
        work->timer = 0;
        work->mode = 0;
        work->submode = phases;
        SHIFT(work)->object = object;
        task->draw = shift_prop_draw;
        task->step = shift_prop_run;
        task->destructor = shift_prop_destroy;
        uv_shift_make(rows, count * phases);
    }
}

/* shift_prop_run, but the x angle counts the phase's steps, since the
 * work's timer holds REACH */
static void shift_world_prop_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        shift_prop_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "Pitch");
        }
        if (work->ang.x++ > (int)work->scl.z) {
            ++work->mode;
            if (work->mode > work->submode) {
                work->mode = 0;
            }
            work->ang.x = 0;
        }
        casino_world_test_near(task, REACH(work));
    }
}

static void shift_world_prop_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct uv_shift *rows = SHIFT(work)->rows;

    uv_shift_free(rows, work->tick * work->submode);
    casino_world_remove(task);
}

void shift_world_prop_start(struct task *task, NJS_OBJECT *object,
                            struct uv_shift *rows, Sint8 count, Sint8 phases)
{
    struct task_work *work = task->work;
    NJS_OBJECT root;

    if (!placement_try_leave_by(task, RANGE)) {
        work->ang.x = 0;
        SHIFT(work)->rows = rows;
        work->tick = count;
        work->timer = MODEL_REACH(object->model);
        work->mode = 0;
        work->submode = phases;
        SHIFT(work)->object = object;
        task->draw = shift_prop_draw;
        task->step = shift_world_prop_run;
        task->destructor = shift_world_prop_destroy;
        uv_shift_make(rows, count * phases);
        root = *object;
        root.child = NULL;
        root.sibling = NULL;
        casino_world_add(task, &root, REACH(work), 0);
    }
}

static void shift_motion_prop_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct uv_shift *rows = SHIFT_MOTION(work)->rows;
    Sint8 count = work->tick;
    Sint8 phase = work->mode;

    if (placement_try_leave_by(task, RANGE) || frame_step) {
        return;
    }
    if (phase) {
        uv_shift_use(rows + ((phase - 1) * count), count);
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_action(SHIFT_MOTION(work)->action, work->scl.x);
    njPopMatrix(1);
    if (phase) {
        uv_shift_restore(rows + ((phase - 1) * count), count);
    }
}

static void shift_motion_prop_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (work->scl.x >= SHIFT_MOTION(work)->action->motion->nbFrame - 1) {
            work->scl.x = 0.0F;
        }
        shift_motion_prop_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "Mot Pitch");
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "UV Pitch");
        }
        work->scl.x += work->scl.y;
        if (work->ang.x++ > (int)work->scl.z) {
            ++work->mode;
            if (work->mode > work->submode) {
                work->mode = 0;
            }
            work->ang.x = 0;
        }
        casino_world_test_near(task, REACH(work));
    }
}

static void shift_motion_prop_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct uv_shift *rows = SHIFT_MOTION(work)->rows;

    uv_shift_free(rows, work->tick * work->submode);
    casino_world_remove(task);
}

void shift_motion_prop_start(struct task *task, NJS_ACTION *action,
                             struct uv_shift *rows, Sint8 count, Sint8 phases)
{
    struct task_work *work = task->work;
    NJS_OBJECT root;

    if (!placement_try_leave_by(task, RANGE)) {
        work->ang.x = 0;
        SHIFT_MOTION(work)->rows = rows;
        work->tick = count;
        work->timer = MODEL_REACH(action->object->model);
        work->mode = 0;
        work->submode = phases;
        SHIFT_MOTION(work)->action = action;
        work->scl.x = 0.0F;
        task->draw = shift_motion_prop_draw;
        task->step = shift_motion_prop_run;
        task->destructor = shift_motion_prop_destroy;
        uv_shift_make(rows, count * phases);
        root = *action->object;
        root.child = NULL;
        root.sibling = NULL;
        casino_world_add(task, &root, REACH(work), 0);
    }
}

static void swap_prop_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct texture_swap *rows = SWAP(work)->rows;
    Sint8 count = work->tick;
    Sint8 phase = work->mode;

    if (placement_try_leave_by(task, RANGE) || frame_step) {
        return;
    }
    if (out_of_view(&work->pos, SWAP(work)->object->model->r + VIEW_MARGIN)) {
        return;
    }
    texture_swap_use(rows + (phase * count), count);
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_tree(SWAP(work)->object);
    njPopMatrix(1);
    texture_swap_restore(rows + (phase * count), count);
}

static void swap_solid_prop_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        swap_prop_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "Pitch");
        }
        if ((Uint16)work->timer++ > (Sint16)work->scl.z) {
            ++work->mode;
            if (work->mode >= work->submode) {
                work->mode = 0;
            }
            work->timer = 0;
        }
        collision_register(work);
    }
}

static void swap_solid_prop_destroy(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: a read of work->collision that nothing uses. SHC drops
     * it, but keeps the load of task->work, into R4 in the RTS's delay slot at
     * 0x0c90e7b2. without it, 1 of its 2 words differs */
    struct collision_work *collision = work->collision;
}

void swap_solid_prop_start(struct task *task, NJS_OBJECT *object,
                           struct collision_shape *shapes, int shape_count,
                           struct texture_swap *rows, Sint8 phases, Sint8 count)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        SWAP(work)->rows = rows;
        work->timer = 0;
        work->tick = count;
        work->mode = 0;
        work->submode = phases;
        SWAP(work)->object = object;
        task->draw = swap_prop_draw;
        task->step = swap_solid_prop_run;
        task->destructor = swap_solid_prop_destroy;
        texture_swap_save(rows, phases * count);
        if (!placement_editing(task)) {
            collision_init(task, shapes, COLLISION_OBJECT, 1, shape_count);
        }
    }
}

static void swap_turn_prop_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (!placement_try_leave_by(task, RANGE)) {
        swap_prop_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "RotSpd");
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "Pitch");
        }
        if ((Uint16)work->timer++ > (Sint16)work->scl.z) {
            ++work->mode;
            if (work->mode >= work->submode) {
                work->mode = 0;
            }
            work->timer = 0;
        }
        object = SWAP(work)->object;
        casino_turn_to(task, NULL,
                       work->ang.y + (Angle)(work->scl.y * ANGLE_PER_DEGREE));
        casino_world_test_near(task, MODEL_REACH(object->model));
    }
}

static void swap_turn_prop_destroy(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: a read of work->collision that nothing uses. SHC drops
     * it, but keeps the load of task->work, into R5 in the jump's delay slot at
     * 0x0c90e946. without it, 1 of its 3 words differs */
    struct collision_work *collision = work->collision;

    casino_world_remove(task);
}

void swap_turn_prop_start(struct task *task, NJS_OBJECT *object,
                          struct texture_swap *rows, Sint8 phases, Sint8 count)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        SWAP(work)->rows = rows;
        work->timer = 0;
        work->tick = count;
        work->mode = 0;
        work->submode = phases;
        SWAP(work)->object = object;
        task->draw = swap_prop_draw;
        task->step = swap_turn_prop_run;
        task->destructor = swap_turn_prop_destroy;
        texture_swap_save(rows, phases * count);
        casino_world_add(task, object, MODEL_REACH(object->model), 0);
    }
}

static void swap_world_prop_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (!placement_try_leave_by(task, RANGE)) {
        swap_prop_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "Pitch");
        }
        if ((Uint16)work->timer++ > (Sint16)work->scl.z) {
            ++work->mode;
            if (work->mode >= work->submode) {
                work->mode = 0;
            }
            work->timer = 0;
        }
        object = SWAP(work)->object;
        casino_world_test_near(task, MODEL_REACH(object->model));
    }
}

static void swap_world_prop_destroy(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: a read of work->collision that nothing uses. SHC drops
     * it, but keeps the load of task->work, into R5 in the jump's delay slot at
     * 0x0c90eafc. without it, 1 of its 3 words differs */
    struct collision_work *collision = work->collision;

    casino_world_remove(task);
}

void swap_world_prop_start(struct task *task, NJS_OBJECT *object,
                           struct texture_swap *rows, Sint8 phases, Sint8 count)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        SWAP(work)->rows = rows;
        work->timer = 0;
        work->tick = count;
        work->mode = 0;
        work->submode = phases;
        SWAP(work)->object = object;
        task->draw = swap_prop_draw;
        task->step = swap_world_prop_run;
        task->destructor = swap_world_prop_destroy;
        texture_swap_save(rows, phases * count);
        casino_world_add(task, object, MODEL_REACH(object->model), 0);
    }
}

static void shift_solid_prop_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        shift_prop_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "Pitch");
        }
        if ((Uint16)work->timer++ > (Sint16)work->scl.z) {
            ++work->mode;
            if (work->mode > work->submode) {
                work->mode = 0;
            }
            work->timer = 0;
        }
        collision_register(work);
    }
}

static void shift_solid_prop_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct uv_shift *rows = SHIFT(work)->rows;

    uv_shift_free(rows, work->tick * work->submode);
}

void shift_solid_prop_start(struct task *task, NJS_OBJECT *object,
                            struct collision_shape *shapes, int shape_count,
                            struct uv_shift *rows, Sint8 count, Sint8 phases)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        SHIFT(work)->rows = rows;
        work->tick = count;
        work->timer = 0;
        work->mode = 0;
        work->submode = phases;
        SHIFT(work)->object = object;
        task->draw = shift_prop_draw;
        task->step = shift_solid_prop_run;
        task->destructor = shift_solid_prop_destroy;
        uv_shift_make(rows, count * phases);
        if (!placement_editing(task)) {
            collision_init(task, shapes, COLLISION_OBJECT, 1, shape_count);
        }
    }
}
