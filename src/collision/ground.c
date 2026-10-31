/* the surfaces around a point, and shadows on the floor below a task */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/ground.h"
#include "collision/mesh.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "object/models.h"
#include "player/player.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the size of a shadow on a floor height below its caster. it is scale
 * when height is 0, and smaller the farther the floor is */
#define SHADOW_SCALE(scale, height) ((scale) / ((njAbs(height) * 0.03F) + 1.0F))
/* how far a shadow sits above a height shadow_draw_at_height is given */
#define GIVEN_LIFT 0.2F
/* a shadow's length along z, as a share of its width */
#define SHADOW_LENGTH 0.8F

/* the first of a task's force records, player 0's */
#define FORCE(task) ((struct task_force *)(task)->force)

Float ground_below(Float x, Float y, Float z, struct angles *slope)
{
    struct height_query query;
    NJS_VECTOR away;
    Angle ang_x;
    Angle ang_z;
    struct world_object *objects;
    NJS_OBJECT *object;
    struct task_force *force;
    Uint16 i;
    Float best;

    query.pos.x = x;
    query.pos.y = y;
    query.pos.z = z;
    objects = world_objects;
    world_list_near(x, y, z, 10.0F);
    best = -GROUND_FAR;
#ifdef GCC
    /* repairs the unset ang_x and ang_z the tag at the end names: with no
     * floor, GCC writes a flat slope */
    ang_x = 0;
    ang_z = 0;
#endif
    for (i = 0; i < world_count; i++) {
        object = objects[i].object;
        /* a task's object counts only when the point is within its reach
         * across x and z: its model's radius times its largest scale */
        if (objects[i].owner != NULL) {
            Float distance;
            Float size;
            Float reach;

            away.x = object->pos[0] - x;
            away.y = 0.0F;
            away.z = object->pos[2] - z;
            distance = njScalor(&away);
            size = object->scl[0];
            if (size < object->scl[1]) {
                size = object->scl[1];
            }
            if (size < object->scl[2]) {
                size = object->scl[2];
            }
            reach = size * object->model->r;
            if (distance > reach) {
                continue;
            }
        }
        if (!(objects[i].flags & WORLD_LIQUID)) {
            /* @hack spelling: the test handed objects[i].object, not object,
             * which it reads through the pointer in R13 (MOV.L @(4,R13),R5
             * at 0x8c096efe); handed object, SHC's three pointers into
             * objects rotate through R10, R11 and R13 and 10 of 170 words
             * differ */
            if (polygon_find_heights(&query, objects[i].object,
                                     objects[i].flags)) {
                /* a moving object's surfaces move by its y speed */
                if (objects[i].owner != NULL) {
                    /* @hack spelling: force set in its test, which keeps i in
                     * R12 (ADD #1,R12 at 0x8c096f5c); set ahead of the test,
                     * i and two pointers into objects rotate through R10 to
                     * R12 and 10 of 170 words differ */
                    if ((force = objects[i].owner->force) != NULL) {
                        query.above.level += force->speed.y;
                        query.below.level += force->speed.y;
                    }
                }
                if (query.below.found == TRUE && y >= query.below.level &&
                    query.below.level > best) {
                    best = query.below.level;
                    ang_x = query.below.ang_x;
                    ang_z = query.below.ang_z;
                }
            }
        }
    }
    /* @bug uninitialized: with no floor, nothing sets ang_x and ang_z, and
     * slope takes the frame words at R15+8 and R15+12 (0x8c096f6a,
     * 0x8c096f72) */
    slope->x = ang_x;
    slope->z = ang_z;
    return best;
}

Float ground_nearest(Float x, Float y, Float z, struct angles *slope)
{
    struct floor_query query;
    NJS_VECTOR away;
    Angle ang_x;
    Angle ang_z;
    struct world_object *objects;
    NJS_OBJECT *object;
    Uint16 i;
    Float best;
    Float now;
    Float was;

    query.pos.x = x;
    query.pos.y = y;
    query.pos.z = z;
    objects = world_objects;
    world_list_near(x, y, z, 5.0F);
    best = -GROUND_FAR;
#ifdef GCC
    /* repairs the unset ang_x and ang_z the tag at the end names: with no
     * floor, GCC writes a flat slope */
    ang_x = 0;
    ang_z = 0;
#endif
    for (i = 0; i < world_count; i++) {
        object = objects[i].object;
        if (objects[i].owner != NULL) {
            Float distance;
            Float size;
            Float reach;

            away.x = object->pos[0] - x;
            away.y = 0.0F;
            away.z = object->pos[2] - z;
            distance = njScalor(&away);
            size = object->scl[0];
            if (size < object->scl[1]) {
                size = object->scl[1];
            }
            if (size < object->scl[2]) {
                size = object->scl[2];
            }
            reach = size * object->model->r;
            if (distance > reach) {
                continue;
            }
        }
        /* @hack spelling: the test handed objects[i].object, not object, which
         * it reads through the pointer in R13 (MOV.L @(4,R13),R5 at
         * 0x8c0970a4); handed object, two pointers into objects trade R10
         * and R12, was and now trade FR4 and FR5, and 11 of 155 words
         * differ */
        if (polygon_find_floor(&query, objects[i].object, objects[i].flags)) {
            was = njAbs(y - best);
            now = njAbs(y - query.floor.level);
            if (was > now) {
                best = query.floor.level;
                if (objects[i].owner != NULL &&
                    objects[i].owner->force != NULL) {
                    best += FORCE(objects[i].owner)->speed.y;
                }
                ang_x = query.floor.ang_x;
                ang_z = query.floor.ang_z;
            }
        }
    }
    /* @bug uninitialized: with no floor, nothing sets ang_x and ang_z.
     * slope takes ang_x's register, R8, as the caller left it, and the frame
     * word at R15+8 (0x8c0970fe, 0x8c097100) */
    slope->x = ang_x;
    slope->z = ang_z;
    return best;
}

Float ground_nearest_land(Float x, Float y, Float z, struct angles *slope)
{
    struct floor_query query;
    Angle ang_x;
    Angle ang_z;
    struct world_object *objects;
    /* @hack redundant: object, set and never read, whose load SHC keeps in
     * the owner test's delay slot (0x8c097176); without it, SHC hoists
     * world_count's address into R8 and the mask into R9, 105 of 110 words
     * differ and ground.c's code runs 32 bytes long */
    NJS_OBJECT *object; /* NOLINT(clang-diagnostic-unused-but-set-variable) */
    Uint16 i;
    Float best;
    Float now;
    Float was;

    query.pos.x = x;
    query.pos.y = y;
    query.pos.z = z;
    objects = world_objects;
    world_list_near(x, y, z, 5.0F);
    best = -GROUND_FAR;
#ifdef GCC
    /* repairs the unset ang_x and ang_z the tag at the end names: with no
     * floor, GCC writes a flat slope */
    ang_x = 0;
    ang_z = 0;
#endif
    for (i = 0; i < world_count; i++) {
        object = objects[i].object;
        if (objects[i].owner != NULL) {
            continue;
        }
        if (!(objects[i].flags & WORLD_LIQUID)) {
            if (polygon_find_floor(&query, objects[i].object,
                                   objects[i].flags)) {
                was = njAbs(y - best);
                now = njAbs(y - query.floor.level);
                if (was > now) {
                    best = query.floor.level;
                    /* never true: the loop skips every task's object */
                    if (objects[i].owner != NULL &&
                        objects[i].owner->force != NULL) {
                        best += FORCE(objects[i].owner)->speed.y;
                    }
                    ang_x = query.floor.ang_x;
                    ang_z = query.floor.ang_z;
                }
            }
        }
    }
    /* @bug uninitialized: with no floor, nothing sets ang_x and ang_z, and
     * slope takes the frame words at R15+4 and R15+8 (0x8c0971da,
     * 0x8c0971e2) */
    slope->x = ang_x;
    slope->z = ang_z;
    return best;
}

Float ground_find_first_floor(struct task_work *work, struct angles *slope)
{
    /* @hack type: unset as an array of three, which puts unset[1] at R15+68
     * and has SHC keep its address in R9 (ADD #68,R9 at 0x8c09727a); as one
     * Float, 111 of 146 words differ and ground.c's code runs 32 bytes
     * short */
    Float unset[3];
    struct floor_query query;
    NJS_VECTOR away;
    struct world_object *objects;
    NJS_OBJECT *object;
    Uint16 i;

    query.pos = work->pos;
    objects = world_objects;
    world_list_near(work->pos.x, work->pos.y, work->pos.z, 20.0F);
    for (i = 0; i < world_count; i++) {
        object = objects[i].object;
        if (objects[i].owner == NULL) {
            Float distance;
            Float size;
            Float reach;

            away.x = object->pos[0] - work->pos.x;
            away.y = 0.0F;
            away.z = object->pos[2] - work->pos.z;
            distance = njScalor(&away);
            size = object->scl[0];
            if (size < object->scl[1]) {
                size = object->scl[1];
            }
            if (size < object->scl[2]) {
                size = object->scl[2];
            }
            reach = size * object->model->r;
            if (distance > reach) {
                query.floor.level = 4000.0F;
                continue;
            }
        }
        /* @hack spelling: the test handed objects[i].object, not object, which
         * it reads through the pointer in R12 (MOV.L @(4,R12),R5 at
         * 0x8c0972e6); handed object, work and that pointer trade R11 and
         * R12 and 13 of 146 words differ */
        if (polygon_find_floor(&query, objects[i].object, objects[i].flags)) {
            /* @bug uninitialized: nothing writes unset, and the test reads
             * unset[1], the frame word at R15+68 (FMOV.S @R9,FR2 at
             * 0x8c0972f4) */
            if (unset[1] <= query.floor.level + 1.5F) {
                query.floor.level = 3000.0F;
            }
            break;
        } else {
            query.floor.level = 4000.0F;
        }
    }
    /* @bug uninitialized: with no floor found, nothing writes query.floor's
     * angles, and slope takes its frame words at R15+40 and R15+44. with no
     * object near, nothing writes its level either, and the result takes the
     * word at R15+48 (0x8c097318 to 0x8c097326) */
    slope->x = query.floor.ang_x;
    slope->z = query.floor.ang_z;
    return query.floor.level;
}

#ifdef GCC
/* hand port.h's present_shadow the shadow at x, y and z, turned about z, x
 * and y, of width scale, drawn as way says */
static void shadow_cast(Float x, Float y, Float z, Angle ang_z, Angle ang_x,
                        Angle ang_y, Float scale, enum shadow_way way)
{
    struct shadow_place place = {{x, y, z}, ang_z, ang_x, ang_y, scale, way};

    present_shadow(&place);
}

void shadow_draw_placed(const struct shadow_place *place)
{
    texture_set_regular();
    njPushMatrix(NULL);
    njTranslate(NULL, place->pos.x, place->pos.y, place->pos.z);
    TURN(njRotateZ, place->ang_z);
    TURN(njRotateX, place->ang_x);
    TURN(njRotateY, place->ang_y);
    if (place->way == SHADOW_TWICE) {
        njScale(NULL, place->scale, place->scale, place->scale);
    }
    njScale(NULL, place->scale, place->scale, place->scale * SHADOW_LENGTH);
    if (place->way == SHADOW_MODEL) {
        njEasyDrawModel(shadow_object->model);
    } else {
        njEasyDrawObject(shadow_object);
    }
    njPopMatrix(1);
}
#endif

void shadow_draw_at_height(struct task_work *work, Float height)
{
    Float scale;

#ifdef GCC
    scale = SHADOW_SCALE(1.0F, work->pos.y - height);
    shadow_cast(work->pos.x, height + GIVEN_LIFT, work->pos.z, work->ang.z,
                work->ang.x, 0, scale, SHADOW_MODEL);
#else
    if (frame_step == 0) {
        scale = SHADOW_SCALE(1.0F, work->pos.y - height);
        texture_set_regular();
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, height + GIVEN_LIFT, work->pos.z);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        njScale(NULL, scale, scale, scale * SHADOW_LENGTH);
        njEasyDrawModel(shadow_object->model);
        njPopMatrix(1);
    }
#endif
}

Float shadow_draw(struct task_work *work, Float scale)
{
    struct angles slope;
    Float floor;

    floor = ground_below(work->pos.x, work->pos.y + 4.0F, work->pos.z, &slope);
#ifdef GCC
    scale = SHADOW_SCALE(scale, work->pos.y - floor);
    shadow_cast(work->pos.x, floor + SHADOW_LIFT, work->pos.z, slope.z, slope.x,
                -work->ang.y, scale, SHADOW_OBJECT);
#else
    if (frame_step == 0) {
        scale = SHADOW_SCALE(scale, work->pos.y - floor);
        texture_set_regular();
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, floor + SHADOW_LIFT, work->pos.z);
        TURN(njRotateZ, slope.z);
        TURN(njRotateX, slope.x);
        TURN(njRotateY, -work->ang.y);
        njScale(NULL, scale, scale, scale * SHADOW_LENGTH);
        njEasyDrawObject(shadow_object);
        njPopMatrix(1);
    }
#endif
    return floor;
}

void shadow_draw_at(struct angles *ang, NJS_POINT3 *pos, Float scale)
{
#ifdef GCC
    shadow_cast(pos->x, pos->y, pos->z, ang->z, ang->x, 0, scale, SHADOW_TWICE);
#else
    if (frame_step == 0) {
        texture_set_regular();
        njPushMatrix(NULL);
        njTranslateV(NULL, pos);
        TURN(njRotateZ, ang->z);
        TURN(njRotateX, ang->x);
        njScale(NULL, scale, scale, scale);
        njScale(NULL, scale, scale, scale * SHADOW_LENGTH);
        njEasyDrawObject(shadow_object);
        njPopMatrix(1);
    }
#endif
}

Float shadow_draw_slope(struct task_work *work, Float scale,
                        struct angles *slope)
{
    Float floor;

    floor = ground_below(work->pos.x, work->pos.y, work->pos.z, slope);
#ifdef GCC
    scale = SHADOW_SCALE(scale, work->pos.y - floor);
    shadow_cast(work->pos.x, floor + SHADOW_LIFT, work->pos.z, slope->z,
                slope->x, work->ang.y, scale, SHADOW_OBJECT);
#else
    if (frame_step == 0) {
        scale = SHADOW_SCALE(scale, work->pos.y - floor);
        texture_set_regular();
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, floor + SHADOW_LIFT, work->pos.z);
        TURN(njRotateZ, slope->z);
        TURN(njRotateX, slope->x);
        TURN(njRotateY, work->ang.y);
        njScale(NULL, scale, scale, scale * SHADOW_LENGTH);
        njEasyDrawObject(shadow_object);
        njPopMatrix(1);
    }
#endif
    return floor;
}

void shadow_update(struct task_work *work, struct shadow *shadow)
{
    struct height_query query;
    /* @hack spelling: pos, the ripple's place and then each object's offset,
     * which keeps the frame at 108 bytes (ADD #-108,R15 at 0x8c0976d0); as
     * a point for each, the frame takes 120 and 148 of 375 words differ */
    NJS_POINT3 pos;
    Angle ang[3];
    struct world_object *objects;
    struct world_object *entry;
    NJS_OBJECT *object;
    Uint16 i;
    Float scale;

    if (shadow->above_flags & WORLD_LIQUID) {
        if (!(shadow->last_above_flags & WORLD_LIQUID)) {
            player_make_splash(work);
        }
        pos.x = work->pos.x;
        pos.y = shadow->above;
        pos.z = work->pos.z;
        ang[0] = shadow->ang_x;
        ang[1] = work->ang.y;
        ang[2] = shadow->ang_z;
        player_make_ripple(PLAYER_VARS(work)->player, &pos, ang);
    }
    shadow->last_above_flags = shadow->above_flags;
    shadow->last_below_flags = shadow->below_flags;
    shadow->above_flags = 0;
    shadow->below_flags = 0;
    query.pos = work->pos;
    query.pos.y += 1.0F;
    objects = world_objects;
    world_list_near(work->pos.x, work->pos.y, work->pos.z, 200.0F);
    shadow->above = GROUND_FAR;
    shadow->below = -GROUND_FAR;
    for (i = 0; i < world_count; i++) {
        entry = &objects[i];
        object = entry->object;
        /* @hack spelling: entry->object handed to both tests, not object,
         * which keeps work in R13 (MOV R4,R13 at 0x8c0976dc); handed object,
         * work moves to R12 and 88 of 375 words differ */
        if (entry->flags & WORLD_LIQUID) {
            if (polygon_find_model_heights(&query, entry->object) &&
                query.above.found == TRUE && query.pos.y < query.above.level &&
                query.above.level < shadow->above) {
                shadow->above = query.above.level;
                shadow->above_flags = entry->flags;
            }
        } else if (entry->owner != NULL) {
            Float distance;
            Float size;

            pos.x = object->pos[0] - work->pos.x;
            pos.y = 0.0F;
            pos.z = object->pos[2] - work->pos.z;
            distance = njScalor(&pos);
            size = object->scl[0];
            if (size < object->scl[1]) {
                size = object->scl[1];
            }
            if (size < object->scl[2]) {
                size = object->scl[2];
            }
            if (distance > size * object->model->r) {
                continue;
            }
        }
        if (polygon_find_heights(&query, entry->object, entry->flags)) {
            if (entry->owner != NULL) {
                /* @hack spelling: objects[i], not entry, for the force here and
                 * the flags kept below, which keeps work in R13 and the
                 * frame at 108 bytes (0x8c0976dc, 0x8c0976d0); with entry,
                 * work moves to R12, the frame takes 104, 327 of 375 words
                 * differ and ground.c's code runs 64 bytes short */
                struct task_force *force = objects[i].owner->force;

                if (force != NULL) {
                    query.above.level += force->speed.y;
                    query.below.level += force->speed.y;
                }
            }
            if (query.above.found == TRUE && query.pos.y < query.above.level &&
                query.above.level < shadow->above) {
                shadow->above = query.above.level;
                shadow->above_flags = objects[i].flags;
            }
            if (query.below.found == TRUE && query.pos.y >= query.below.level &&
                query.below.level > shadow->below) {
                shadow->below = query.below.level;
                shadow->below_flags = objects[i].flags;
                shadow->ang_x = query.below.ang_x;
                shadow->ang_z = query.below.ang_z;
            }
        }
    }
#ifdef GCC
    if (shadow->below > query.pos.y - 20000.0F) {
        scale = SHADOW_SCALE(shadow->scale, query.pos.y - shadow->below);
        shadow_cast(work->pos.x, shadow->below + SHADOW_LIFT, work->pos.z,
                    shadow->ang_z, shadow->ang_x, -work->ang.y, scale,
                    SHADOW_OBJECT);
    }
#else
    if (frame_step == 0 && shadow->below > query.pos.y - 20000.0F) {
        scale = SHADOW_SCALE(shadow->scale, query.pos.y - shadow->below);
        texture_set_regular();
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, shadow->below + SHADOW_LIFT,
                    work->pos.z);
        TURN(njRotateZ, shadow->ang_z);
        TURN(njRotateX, shadow->ang_x);
        TURN(njRotateY, -work->ang.y);
        njScale(NULL, scale, scale, scale * SHADOW_LENGTH);
        njEasyDrawObject(shadow_object);
        njPopMatrix(1);
    }
#endif
}

Bool ground_find_sides(struct sides_query *result)
{
    NJS_VECTOR away;
    struct sides_query query;
    NJS_POINT3 *pos;
    struct world_object *objects;
    NJS_OBJECT *object;
    Bool found;
    int side;
    Uint16 i;
    Float was;
    Float now;

    found = FALSE;
    pos = &result->pos;
    query.pos = result->pos;
    for (side = 0; side < SIDES; side++) {
        result->sides[side].found = FALSE;
    }
    result->sides[SIDE_MINUS_X].level = -GROUND_FAR;
    result->sides[SIDE_PLUS_X].level = GROUND_FAR;
    result->sides[SIDE_BELOW].level = -GROUND_FAR;
    result->sides[SIDE_ABOVE].level = GROUND_FAR;
    result->sides[SIDE_MINUS_Z].level = -GROUND_FAR;
    result->sides[SIDE_PLUS_Z].level = GROUND_FAR;
    objects = world_objects;
    world_list_near(pos->x, pos->y, pos->z, 1.0F);
    for (i = 0; i < world_count; i++) {
        object = objects[i].object;
        if (objects[i].owner != NULL) {
            Float distance;
            Float size;

            away.x = object->pos[0] - pos->x;
            away.y = object->pos[1] - pos->y;
            away.z = object->pos[2] - pos->z;
            distance = njScalor(&away);
            size = object->scl[0];
            if (size < object->scl[1]) {
                size = object->scl[1];
            }
            if (size < object->scl[2]) {
                size = object->scl[2];
            }
            if (distance > size * object->model->r) {
                continue;
            }
        }
        if (objects[i].flags & WORLD_LIQUID) {
            continue;
        }
        /* @hack spelling: the test handed objects[i].object, not object, which
         * it reads through the pointer in R9 (MOV.L @(4,R9),R5 at
         * 0x8c097bd8); handed object, the pointers into objects in R8, R9
         * and the frame word at R15 trade places and 16 of 414 words
         * differ */
        if (polygon_find_sides(&query, objects[i].object, objects[i].flags)) {
            found = TRUE;
            /* @hack spelling: now < was for the first three sides, and was >
             * now for the rest, which keep the image's choice of FR4 and FR5
             * for was and now on each side, as at 0x8c097c00 and 0x8c097c08;
             * as was > now for all six, the two trade on the first three
             * sides and 9 of 414 words differ */
            if (query.sides[SIDE_MINUS_X].found) {
                was = njAbs(pos->x - result->sides[SIDE_MINUS_X].level);
                now = njAbs(pos->x - query.sides[SIDE_MINUS_X].level);
                if (now < was) {
                    result->sides[SIDE_MINUS_X] = query.sides[SIDE_MINUS_X];
                }
            }
            if (query.sides[SIDE_PLUS_X].found) {
                was = njAbs(result->sides[SIDE_PLUS_X].level - pos->x);
                now = njAbs(query.sides[SIDE_PLUS_X].level - pos->x);
                if (now < was) {
                    result->sides[SIDE_PLUS_X] = query.sides[SIDE_PLUS_X];
                }
            }
            if (query.sides[SIDE_BELOW].found) {
                was = njAbs(pos->y - result->sides[SIDE_BELOW].level);
                now = njAbs(pos->y - query.sides[SIDE_BELOW].level);
                if (now < was) {
                    result->sides[SIDE_BELOW] = query.sides[SIDE_BELOW];
                }
            }
            if (query.sides[SIDE_ABOVE].found) {
                was = njAbs(result->sides[SIDE_ABOVE].level - pos->y);
                now = njAbs(query.sides[SIDE_ABOVE].level - pos->y);
                if (was > now) {
                    result->sides[SIDE_ABOVE] = query.sides[SIDE_ABOVE];
                }
            }
            if (query.sides[SIDE_MINUS_Z].found) {
                was = njAbs(pos->z - result->sides[SIDE_MINUS_Z].level);
                now = njAbs(pos->z - query.sides[SIDE_MINUS_Z].level);
                if (was > now) {
                    result->sides[SIDE_MINUS_Z] = query.sides[SIDE_MINUS_Z];
                }
            }
            if (query.sides[SIDE_PLUS_Z].found) {
                was = njAbs(result->sides[SIDE_PLUS_Z].level - pos->z);
                now = njAbs(query.sides[SIDE_PLUS_Z].level - pos->z);
                if (was > now) {
                    result->sides[SIDE_PLUS_Z] = query.sides[SIDE_PLUS_Z];
                }
            }
        }
    }
    return found;
}

Bool ground_find_sides_water(struct sides_query *result)
{
    NJS_VECTOR away;
    struct sides_query query;
    NJS_POINT3 *pos;
    struct world_object *objects;
    /* @hack redundant: object's null, which SHC deletes but which takes 4
     * bytes of the frame, 232 in all (MOV.W at 0x8c097e70), and puts query
     * at R15+16 (0x8c097e78); without it, 43 of 411 words differ */
    NJS_OBJECT *object = NULL;
    Bool found;
    int side;
    Uint16 i;
    Float was;
    Float now;

    pos = &result->pos;
    found = FALSE;
    query.pos = result->pos;
    for (side = 0; side < SIDES; side++) {
        result->sides[side].found = FALSE;
    }
    result->sides[SIDE_MINUS_X].level = -GROUND_FAR;
    result->sides[SIDE_PLUS_X].level = GROUND_FAR;
    result->sides[SIDE_BELOW].level = -GROUND_FAR;
    result->sides[SIDE_ABOVE].level = GROUND_FAR;
    result->sides[SIDE_MINUS_Z].level = -GROUND_FAR;
    result->sides[SIDE_PLUS_Z].level = GROUND_FAR;
    objects = world_objects;
    world_list_near(pos->x, pos->y, pos->z, 1.0F);
    for (i = 0; i < world_count; i++) {
        object = objects[i].object;
        if (objects[i].owner != NULL) {
            Float distance;
            Float size;
            Float reach;

            away.x = object->pos[0] - pos->x;
            away.y = object->pos[1] - pos->y;
            away.z = object->pos[2] - pos->z;
            distance = njScalor(&away);
            size = object->scl[0];
            if (size < object->scl[1]) {
                size = object->scl[1];
            }
            if (size < object->scl[2]) {
                size = object->scl[2];
            }
            reach = size * object->model->r;
            if (distance > reach) {
                continue;
            }
        }
        /* @hack spelling: the test handed objects[i].object, not object, which
         * it reads through the pointer in R8 (MOV.L @(4,R8),R5 at
         * 0x8c097f8a); handed object, that pointer moves to the frame word
         * at R15+4 and 132 of 411 words differ */
        if (polygon_find_sides(&query, objects[i].object, objects[i].flags)) {
            if (query.sides[SIDE_MINUS_X].found) {
                was = njAbs(pos->x - result->sides[SIDE_MINUS_X].level);
                now = njAbs(pos->x - query.sides[SIDE_MINUS_X].level);
                if (was > now) {
                    result->sides[SIDE_MINUS_X] = query.sides[SIDE_MINUS_X];
                }
            }
            if (query.sides[SIDE_PLUS_X].found) {
                was = njAbs(result->sides[SIDE_PLUS_X].level - pos->x);
                now = njAbs(query.sides[SIDE_PLUS_X].level - pos->x);
                if (was > now) {
                    result->sides[SIDE_PLUS_X] = query.sides[SIDE_PLUS_X];
                }
            }
            if (query.sides[SIDE_BELOW].found) {
                was = njAbs(pos->y - result->sides[SIDE_BELOW].level);
                now = njAbs(pos->y - query.sides[SIDE_BELOW].level);
                if (was > now) {
                    result->sides[SIDE_BELOW] = query.sides[SIDE_BELOW];
                    if (objects[i].flags & WORLD_LIQUID) {
                        found = TRUE;
                    }
                }
            }
            if (query.sides[SIDE_ABOVE].found) {
                was = njAbs(result->sides[SIDE_ABOVE].level - pos->y);
                now = njAbs(query.sides[SIDE_ABOVE].level - pos->y);
                if (was > now) {
                    result->sides[SIDE_ABOVE] = query.sides[SIDE_ABOVE];
                }
            }
            if (query.sides[SIDE_MINUS_Z].found) {
                was = njAbs(pos->z - result->sides[SIDE_MINUS_Z].level);
                now = njAbs(pos->z - query.sides[SIDE_MINUS_Z].level);
                if (was > now) {
                    result->sides[SIDE_MINUS_Z] = query.sides[SIDE_MINUS_Z];
                }
            }
            if (query.sides[SIDE_PLUS_Z].found) {
                was = njAbs(result->sides[SIDE_PLUS_Z].level - pos->z);
                now = njAbs(query.sides[SIDE_PLUS_Z].level - pos->z);
                if (was > now) {
                    result->sides[SIDE_PLUS_Z] = query.sides[SIDE_PLUS_Z];
                }
            }
        }
    }
    return found;
}
