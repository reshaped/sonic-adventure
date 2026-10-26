/* helpers for the modules' simple objects: collision records, trees drawn
 * at a work's place, their copies in the collision world, and bounces on
 * the ground */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/world.h"
#include "draw/tree.h"
#include "object/simple.h"
#include "player/player.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* object_bounce looks for the ground from this far above a thing's foot,
 * pos less offset */
#define GROUND_ABOVE 4.0F
/* the speed a falling thing gains each step. a thing no more than
 * FALL_STEP above the ground comes to rest on it, once it rises or falls no
 * faster than FALL_STEP */
#define FALL_STEP 0.027222218F
/* a speed no faster than FALL_STEP, squared, counts as none */
#define REST_SPEED2 (FALL_STEP * FALL_STEP)

static const NJS_VECTOR object_no_speed = {0.0F, 0.0F, 0.0F};
static const NJS_VECTOR object_no_accel = {0.0F, 0.0F, 0.0F};

static void object_turn_x(struct angles *ang)
{
    TURN(njRotateX, ang->x);
}

static void object_turn_y(struct angles *ang)
{
    TURN(njRotateY, ang->y);
}

static void object_turn_z(struct angles *ang)
{
    TURN(njRotateZ, ang->z);
}

void object_collision_init(struct task *task, const struct collision_list *list,
                           const struct collision_kind *kind)
{
    collision_init(task, list->shapes, kind->kind, kind->unused, list->count);
}

/* @hack spelling: the texlist set through an inline function, which keeps
 * texlist in the frame slot above the object's (MOV.L R6,@(8,R15) at
 * 0x8c0a5ac0); written out, texlist and work trade slots, and summed over
 * the three draws, 43 of 150 words differ */
#pragma inline(set_texlist)
/* @hack spelling: njScale through an inline function, which keeps the scale
 * in a frame slot until the call (FMOV.S FR4,@(R0,R15) at 0x8c0a5a58);
 * written out, the scale stays in FR15, and summed over
 * object_draw_tree_scaled and object_draw_tree_zyx, 90 of 108 words
 * differ */
#pragma inline(scale_by)

/* set texlist, or the stage's textures for NULL */
static void set_texlist(NJS_TEXLIST *texlist)
{
    if (texlist == NULL) {
        texture_set_stage();
    } else {
        njSetTexture(texlist);
    }
}

static void scale_by(Float scale)
{
    njScale(NULL, scale, scale, scale);
}

void object_draw_tree_scaled(NJS_OBJECT *object, struct task_work *work,
                             Float scale, NJS_TEXLIST *texlist)
{
    struct angles *ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    ang = &work->ang;
    object_turn_z(ang);
    object_turn_x(ang);
    object_turn_y(ang);
    scale_by(scale);
    set_texlist(texlist);
    draw_tree(object);
    njPopMatrix(1);
}

void object_draw_tree(NJS_OBJECT *object, struct task_work *work,
                      NJS_TEXLIST *texlist)
{
    struct angles *ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    ang = &work->ang;
    object_turn_z(ang);
    object_turn_x(ang);
    object_turn_y(ang);
    set_texlist(texlist);
    draw_tree(object);
    njPopMatrix(1);
}

NJS_OBJECT *object_world_copy(NJS_OBJECT *source)
{
    NJS_OBJECT *object;
    /* @hack spelling: the scale stored through scl, object->scl's address in R5
     * (ADD #32,R5 at 0x8c0a5b6e); stored through object, 21 of 30 words
     * differ */
    Float *scl;

    object = world_object_alloc();
    if (object != NULL) {
        object->evalflags =
            source->evalflags & ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
        object->model = source->model;
        object->child = source->child;
        /* @hack spelling: scl set before sibling is copied, which forms its
         * address ahead of the copies (ADD #32,R5 at 0x8c0a5b6e); set after
         * the copy, 5 of 30 words differ */
        scl = object->scl;
        object->sibling = source->sibling;
        scl[0] = 1.0F;
        scl[1] = 1.0F;
        scl[2] = 1.0F;
    }
    return object;
}

void object_world_place(NJS_OBJECT *object, struct task_work *work, Float scale)
{
    /* @hack spelling: the copies through pointers, which take pos's and ang's
     * addresses (ADD #32 at 0x8c0a5b98, ADD #20 at 0x8c0a5ba0) and read at
     * offsets from them; through object and work, 27 of 28 words differ */
    Float *to;
    NJS_POINT3 *from;
    Angle *ang_to;
    struct angles *ang_from;

    from = &work->pos;
    to = object->pos;
    to[0] = from->x;
    to[1] = from->y;
    to[2] = from->z;
    ang_to = object->ang;
    ang_from = &work->ang;
    ang_to[0] = ang_from->x;
    ang_to[1] = ang_from->y;
    ang_to[2] = ang_from->z;
    to = object->scl;
    to[0] = scale;
    to[1] = scale;
    to[2] = scale;
}

void object_force_clear(struct task_motion *motion)
{
    motion->speed = object_no_speed;
    motion->accel = object_no_accel;
}

/* the cutoffs on the scaled radius are ten times model_start_world's */
void object_world_add_sized(struct task *task, NJS_OBJECT *object, Sint32 flags,
                            Float scale)
{
    Float radius;

    flags |= WORLD_SOLID;
    radius = object->model->r * scale;
    if (radius <= WORLD_LARGE_REACH) {
        if (radius > WORLD_MEDIUM_REACH) {
            flags |= WORLD_LARGE;
        } else if (radius > WORLD_SMALL_REACH) {
            flags |= WORLD_MEDIUM;
        } else {
            flags |= WORLD_SMALL;
        }
    }
    world_add(flags, task, object);
}

Bool object_world_test_radius(struct task_work *work, Float radius)
{
    Bool in_range;

    in_range = player_in_range(&work->pos, radius) != 0;
    if (in_range) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    return in_range;
}

void object_draw_tree_zyx(NJS_OBJECT *object, struct task_work *work,
                          Float scale, NJS_TEXLIST *texlist)
{
    struct angles *ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    ang = &work->ang;
    object_turn_z(ang);
    object_turn_y(ang);
    object_turn_x(ang);
    scale_by(scale);
    set_texlist(texlist);
    draw_tree(object);
    njPopMatrix(1);
}

void object_world_free(struct task *task, NJS_OBJECT *object)
{
    if (object != NULL) {
        world_remove(task, object);
        world_object_free(object);
    }
}

Bool object_bounce(NJS_POINT3 *pos, NJS_VECTOR *speed, NJS_VECTOR *bounce,
                   Float offset)
{
    struct angles slope;
    Float x;
    Float y;
    Float z;
    Float speed_x;
    Float speed_y;
    Float speed_z;
    Float ground;
    /* the part of the step after the landing: 0 in the air, and 1 on the
     * ground. pos moves across at the old speed through the part before
     * and at the scaled speed through after */
    Float after;
    Float below;
    Float before;
    Bool moving;
    Bool result;

    x = pos->x;
    y = pos->y;
    z = pos->z;
    speed_x = speed->x;
    speed_y = speed->y;
    speed_z = speed->z;
    ground = ground_below(x, y - offset + GROUND_ABOVE, z, &slope) + offset;
    result = TRUE;
    moving = SQUARE(speed_x) + SQUARE(speed_z) > REST_SPEED2;
    if (y - ground <= FALL_STEP && SQUARE(speed_y) <= REST_SPEED2) {
        speed_y = 0.0F;
        y = ground;
        after = 1.0F;
        if (!moving) {
            result = FALSE;
        }
    } else {
        speed_y += -FALL_STEP;
        y += speed_y;
        below = y - ground;
        if (below > 0.0F) {
            after = 0.0F;
        } else {
            y = ground - (below * bounce->y);
            after = SQUARE(speed_y) <= REST_SPEED2 ? 1.0F : below / speed_y;
            speed_y *= -bounce->y;
        }
    }
    pos->y = y;
    speed->y = speed_y;
    if (moving) {
        before = 1.0F - after;
        x += before * speed_x;
        z += before * speed_z;
        if (after > 0.0F) {
            speed_x *= bounce->x;
            speed_z *= bounce->z;
            x += after * speed_x;
            z += after * speed_z;
            speed->x = speed_x;
            speed->z = speed_z;
        }
        pos->x = x;
        pos->z = z;
    } else {
        speed->x = 0.0F;
        speed->z = 0.0F;
    }
    return result;
}

int meshset_count_points(NJS_MESHSET *meshset)
{
    int total = 0;
    /* @hack spelling: i zeroed where it's declared, which zeroes total from it
     * ahead of the loads (MOV #0,R5 and MOV R5,R7 at 0x8c0a5efe); zeroed in
     * the for, 3 of 18 words differ */
    int i = 0;
    int count = meshset->nbMesh;
    Sint16 *meshes = meshset->meshes;
    int points;

    for (; i < count; i++) {
        points = (Uint8)*meshes;
        total += points;
        meshes += points + 1;
    }
    return total;
}
