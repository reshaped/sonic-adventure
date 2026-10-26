/* a Chao in the world: the surfaces around it and how they push it, its
 * speed and its turn toward its target, and the garden of the stage */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "al_main/physics.h"
#include "camera/camera.h"
#include "chao/chao.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "object/models.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the way up */
static const NJS_VECTOR toward_up = {0.0F, 1.0F, 0.0F};
/* the lift chao_collide_floor figures a slope's pull from */
static const NJS_VECTOR slope_lift = {0.0F, 0.05F, 0.0F};
/* the way to each level side */
static const NJS_VECTOR toward_minus_x = {-1.0F, 0.0F, 0.0F};
static const NJS_VECTOR toward_plus_x = {1.0F, 0.0F, 0.0F};
static const NJS_VECTOR toward_minus_z = {0.0F, 0.0F, -1.0F};
static const NJS_VECTOR toward_plus_z = {0.0F, 0.0F, 1.0F};

void alife_terrain_init(struct task_work *work, struct alife_chao *chao)
{
    struct chao_terrain *terrain = &chao->terrain;

    terrain->unk_04 = 0.0F;
    terrain->query_height = 5.0F;
    terrain->unk_0c = 0.0F;
    terrain->head_height = 15.0F;
    terrain->unk_14 = 15.0F;
    terrain->unk_18 = -3.0F;
    terrain->wall_bounce = 0.23F;
    terrain->unk_34 = 1.6F;
    terrain->unk_38 = 1.0F;
}

int alife_find_sides(struct chao_terrain *terrain)
{
    struct sides_query *result;
    NJS_VECTOR away;
    struct sides_query query;
    NJS_POINT3 *pos;
    struct world_object *objects;
    NJS_OBJECT *object;
    Float distance;
    Float size;
    Float was;
    Float now;
    int side;
    int flags = 0;
    Uint16 i;

    result = &terrain->query;
    pos = &result->pos;
    query.pos = result->pos;
    /* @hack spelling: side = side + 1, which keeps the cursors that read
     * objects[i].flags after a find in the frame words at sp+8 and sp+12
     * (0x0c90a3fe, 0x0c90a554); as side++, the two trade and 8 of 575 words
     * differ */
    for (side = 0; side < SIDES; side = side + 1) {
        result->sides[side].found = FALSE;
    }
    terrain->flags &=
        ~(TERRAIN_WATER_ABOVE | TERRAIN_WATER_BELOW | TERRAIN_DEEP);
    terrain->flags &= ~(TERRAIN_CEILING | TERRAIN_FLOOR | 0x200);
    terrain->flags &= ~(TERRAIN_WALL | TERRAIN_FACING_WALL | 0x100 | 0x2000);
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
            away.x = object->pos[0] - pos->x;
            away.y = object->pos[1] - pos->y;
            away.z = object->pos[2] - pos->z;
            distance = njScalor(&away);
            size = object->scl[0];
            if (object->scl[1] > size) {
                size = object->scl[1];
            }
            if (object->scl[2] > size) {
                size = object->scl[2];
            }
            if (distance > size * object->model->r) {
                continue;
            }
        }
        if (objects[i].flags & WORLD_WATER) {
            if (polygon_find_sides(&query, objects[i].object,
                                   objects[i].flags)) {
                flags = objects[i].flags;
                if (query.sides[SIDE_BELOW].found) {
                    was = njAbs(pos->y - result->sides[SIDE_BELOW].level);
                    now = njAbs(pos->y - query.sides[SIDE_BELOW].level);
                    /* @hack spelling: now < was, which keeps was in FR5 and now
                     * in FR4 for the FCMP/GT at 0x0c90a428; as was > now, the
                     * two trade and 3 of 575 words differ */
                    if (now < was) {
                        result->sides[SIDE_BELOW] = query.sides[SIDE_BELOW];
                        terrain->flags |= TERRAIN_WATER_BELOW;
                        terrain->flags |= TERRAIN_DEEP;
                    }
                }
                if (query.sides[SIDE_ABOVE].found) {
                    was = njAbs(result->sides[SIDE_ABOVE].level - pos->y);
                    now = njAbs(query.sides[SIDE_ABOVE].level - pos->y);
                    if (was > now) {
                        result->sides[SIDE_ABOVE] = query.sides[SIDE_ABOVE];
                        if (now < 5.0F) {
                            terrain->flags |= TERRAIN_WATER_ABOVE;
                            terrain->flags |= TERRAIN_DEEP;
                        }
                    }
                }
            }
        } else if (polygon_find_sides(&query, objects[i].object,
                                      objects[i].flags)) {
            flags = objects[i].flags;
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
                    terrain->flags &= ~TERRAIN_WATER_BELOW;
                }
            }
            if (query.sides[SIDE_ABOVE].found) {
                was = njAbs(result->sides[SIDE_ABOVE].level - pos->y);
                now = njAbs(query.sides[SIDE_ABOVE].level - pos->y);
                if (was > now) {
                    result->sides[SIDE_ABOVE] = query.sides[SIDE_ABOVE];
                    terrain->flags &= ~TERRAIN_WATER_ABOVE;
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
    return flags;
}

/* find the surfaces around the query's point as alife_find_sides does, but
 * leave water out of the sides and the TERRAIN_ flags. water_level becomes the
 * highest level of any water surface found below or above the point, capped at
 * 0. with no water it is -GROUND_FAR.
 *
 * on a frame's first step, shadow_object is drawn on each floor found below the
 * point but water. the shadow's size and tilt follow the nearest floor found so
 * far, in list order. drawing leaves texture_set_regular's texlist current and
 * the constant material at 0 */
static int terrain_find_sides_and_draw_shadow(struct chao_terrain *terrain)
{
    struct sides_query *result;
    NJS_VECTOR away;
    struct sides_query query;
    NJS_POINT3 *pos;
    struct world_object *objects;
    NJS_OBJECT *object;
    Float distance;
    Float size;
    Float was;
    Float now;
    int side;
    int flags = 0;
    Uint16 i;

    result = &terrain->query;
    pos = &result->pos;
    query.pos = result->pos;
    for (side = 0; side < SIDES; side++) {
        result->sides[side].found = FALSE;
    }
    terrain->flags &=
        ~(TERRAIN_WATER_ABOVE | TERRAIN_WATER_BELOW | TERRAIN_DEEP);
    terrain->flags &= ~(TERRAIN_CEILING | TERRAIN_FLOOR | 0x200);
    terrain->flags &= ~(TERRAIN_WALL | TERRAIN_FACING_WALL | 0x100 | 0x2000);
    result->sides[SIDE_MINUS_X].level = -GROUND_FAR;
    result->sides[SIDE_PLUS_X].level = GROUND_FAR;
    result->sides[SIDE_BELOW].level = -GROUND_FAR;
    result->sides[SIDE_ABOVE].level = GROUND_FAR;
    result->sides[SIDE_MINUS_Z].level = -GROUND_FAR;
    result->sides[SIDE_PLUS_Z].level = GROUND_FAR;
    terrain->water_level = -GROUND_FAR;
    objects = world_objects;
    world_list_near(pos->x, pos->y, pos->z, 1.0F);
    for (i = 0; i < world_count; i++) {
        /* @hack spelling: entry for the object, the owner, the water test and
         * water's find, and objects[i] for the rest. SHC figures entry from i
         * on each pass, into R8 from 0x0c90a900, and steps cursors on the
         * stack for objects[i]; with objects[i] throughout, 507 of 586 words
         * differ */
        struct world_object *entry = &objects[i];

        object = entry->object;
        if (entry->owner != NULL) {
            away.x = object->pos[0] - pos->x;
            away.y = object->pos[1] - pos->y;
            away.z = object->pos[2] - pos->z;
            distance = njScalor(&away);
            size = object->scl[0];
            if (object->scl[1] > size) {
                size = object->scl[1];
            }
            if (object->scl[2] > size) {
                size = object->scl[2];
            }
            if (distance > size * object->model->r) {
                continue;
            }
        }
        if (entry->flags & WORLD_WATER) {
            if (polygon_find_sides(&query, entry->object, entry->flags)) {
                flags = objects[i].flags;
                if (query.sides[SIDE_BELOW].found) {
                    if (query.sides[SIDE_BELOW].level > terrain->water_level) {
                        terrain->water_level = query.sides[SIDE_BELOW].level;
                    }
                }
                if (query.sides[SIDE_ABOVE].found) {
                    if (query.sides[SIDE_ABOVE].level > terrain->water_level) {
                        terrain->water_level = query.sides[SIDE_ABOVE].level;
                    }
                }
            }
            continue;
        }
        if (polygon_find_sides(&query, objects[i].object, objects[i].flags)) {
            flags = objects[i].flags;
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
                /* @hack spelling: now < was, which keeps was in FR4 and now in
                 * FR5 for the FCMP/GT at 0x0c90aa82; as was > now, the two
                 * trade and 3 of 586 words differ */
                if (now < was) {
                    result->sides[SIDE_PLUS_X] = query.sides[SIDE_PLUS_X];
                }
            }
            if (query.sides[SIDE_BELOW].found) {
                was = njAbs(pos->y - result->sides[SIDE_BELOW].level);
                now = njAbs(pos->y - query.sides[SIDE_BELOW].level);
                if (was > now) {
                    result->sides[SIDE_BELOW] = query.sides[SIDE_BELOW];
                }
                if (!frame_step) {
                    Float scale;
                    Float alpha;
                    Float height;

                    scale = 0.3F /
                            (1.0F +
                             (njAbs(pos->y - result->sides[SIDE_BELOW].level) *
                              0.03F));
                    texture_set_regular();
                    height = query.pos.y - query.sides[SIDE_BELOW].level;
                    alpha = -(height * 0.02F);
                    if (alpha > 0.0F) {
                        alpha = 0.0F;
                    }
                    draw_set_material(alpha - 0.2F, 0.0F, 0.0F, 0.0F);
                    njPushMatrix(NULL);
                    njTranslate(NULL, pos->x,
                                query.sides[SIDE_BELOW].level + 0.01F, pos->z);
                    njRotateXYZ(NULL, result->sides[SIDE_BELOW].ang_x, 0,
                                result->sides[SIDE_BELOW].ang_z);
                    njScale(NULL, scale, scale, scale);
                    njSimpleDrawObject(shadow_object);
                    njPopMatrix(1);
                    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
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
                /* @hack spelling: now < was, which keeps was in FR4 and now in
                 * FR5 for the FCMP/GT at 0x0c90ac78; as was > now, the two
                 * trade and 3 of 586 words differ */
                if (now < was) {
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
    if (terrain->water_level > 0.0F) {
        terrain->water_level = 0.0F;
    }
    return flags;
}

/* the Chao's head is head_height above the query's point. where it is over
 * the surface above, the Chao stops on y and drops by a tenth of the
 * overlap. TERRAIN_CEILING says whether it did */
static void chao_collide_ceiling(struct task_work *work,
                                 struct alife_chao *chao)
{
    struct chao_terrain *terrain = &chao->terrain;
    struct sides_query *query = &terrain->query;
    Float top = query->pos.y + terrain->head_height;
    struct surface *ceiling = &query->sides[SIDE_ABOVE];

    terrain->flags &= ~TERRAIN_CEILING;
    if (top > ceiling->level) {
        chao->speed.y = 0.0F;
        work->pos.y -= (top - ceiling->level) * 0.1F;
        terrain->flags |= TERRAIN_CEILING;
    }
}

/* the Chao's floor is the surface below. where that surface faces up and the
 * surface above is less than 8 over the Chao's pos, the floor is the surface
 * above instead.
 *
 * a Chao below its floor rises 0.6 of the way to it. a fall then bounces back
 * at 0.4 of its speed, and a rise under 0.01 stops. CHAO_ON_GROUND and
 * TERRAIN_FLOOR set. a slope then pulls a Chao above water_level while 0x80 in
 * chao->flags is set. the push then clears 0x80, and the walk, the stop and a
 * throw set it again each step. a slope is a surface below whose normal's y is
 * under 0.98. it pulls the Chao's speed down the slope by 20 times slope_lift's
 * share along it.
 *
 * last, wherever the Chao is, a pos.y below -50 becomes -50 */
static void chao_collide_floor(struct task_work *work, struct alife_chao *chao)
{
    struct chao_terrain *terrain = &chao->terrain;
    struct sides_query *query = &terrain->query;
    NJS_VECTOR up;
    Float floor;
    Float ceiling;

    up = toward_up;
    ceiling = query->sides[SIDE_ABOVE].level;
    floor = query->sides[SIDE_BELOW].level;
    if (njInnerProduct(&query->sides[SIDE_BELOW].normal, &up) > 0.0F) {
        if (work->pos.y + 8.0F > ceiling) {
            floor = ceiling;
        }
    }
    if (work->pos.y < floor) {
        /* @hack spelling: speed.y read and written through a pointer, which SHC
         * keeps in R4, set at 0x0c90ae3e; by name, through @(R0,R14), 92 of
         * 209 words differ */
        Float *speed_y = &chao->speed.y;

        work->pos.y += (floor - work->pos.y) * 0.6F;
        if (*speed_y < 0.0F) {
            *speed_y *= -0.4F;
        } else if (*speed_y < 0.01F) {
            *speed_y = 0.0F;
        }
        work->flags |= CHAO_ON_GROUND;
        terrain->flags |= TERRAIN_FLOOR;
        /* @hack spelling: water_level read through chao, not terrain, which
         * keeps chao in R14 and work in R13 (0x0c90ade0); through terrain,
         * the two trade and 31 of 209 words differ */
        if (work->pos.y > chao->terrain.water_level && (chao->flags & 0x80)) {
            if (njInnerProduct(&query->sides[SIDE_BELOW].normal, &up) < 0.98F) {
                NJS_VECTOR lift;
                NJS_VECTOR across;
                NJS_VECTOR slope;
                Float pull;
                NJS_VECTOR *normal = &query->sides[SIDE_BELOW].normal;

                lift = slope_lift;
                njOuterProduct(&lift, normal, &across);
                njOuterProduct(normal, &across, &slope);
                if (njScalor(&slope) == 0.0F) {
                    slope.x = 0.0F;
                    slope.y = 0.0F;
                    slope.z = 0.0F;
                } else {
                    njUnitVector(&slope);
                }
                pull = njInnerProduct(&lift, &slope) * 20.0F;
                chao->speed.x -= slope.x * pull;
                chao->speed.y -= slope.y * pull;
                chao->speed.z -= slope.z * pull;
            }
        }
        chao->flags &= ~0x80;
    }
    if (work->pos.y < -50.0F) {
        work->pos.y = -50.0F;
    }
}

/* the plane of the wall, through at_x, pos.y and at_z */
#define WALL_PLANE(at_x, at_z)                                                 \
    do {                                                                       \
        plane.px = (at_x);                                                     \
        plane.py = pos.y;                                                      \
        plane.pz = (at_z);                                                     \
        plane.vx = wall->normal.x;                                             \
        plane.vy = wall->normal.y;                                             \
        plane.vz = wall->normal.z;                                             \
    } while (0)

/* where overlap is above 0, push the Chao 0.35 of it, level, along the
 * wall's normal. TERRAIN_WALL sets, and so does TERRAIN_FACING_WALL where
 * the Chao's facing and the normal have a dot product under -0.35. a speed
 * into the wall loses twice wall_bounce of its part along the normal. the
 * yaw then turns by wall_turn, one way or the other by the side of the
 * normal the new speed lies on */
#define WALL_PUSH(overlap)                                                     \
    do {                                                                       \
        depth = (overlap);                                                     \
        if (depth > 0.0F) {                                                    \
            push.x = wall->normal.x;                                           \
            push.y = 0.0F;                                                     \
            push.z = wall->normal.z;                                           \
            push.x *= depth * 0.35F;                                           \
            push.y *= depth * 0.35F;                                           \
            push.z *= depth * 0.35F;                                           \
            njAddVector(&work->pos, &push);                                    \
            terrain->flags |= TERRAIN_WALL;                                    \
            if (njInnerProduct(&forward, &wall->normal) < -0.35F) {            \
                terrain->flags |= TERRAIN_FACING_WALL;                         \
            }                                                                  \
            if (njInnerProduct(&chao->speed, &wall->normal) < 0.0F) {          \
                normal = wall->normal;                                         \
                if (njScalor(&normal) == 0.0F) {                               \
                    normal.x = 0.0F;                                           \
                    normal.y = 0.1F;                                           \
                    normal.z = 0.0F;                                           \
                } else {                                                       \
                    njUnitVector(&normal);                                     \
                }                                                              \
                share = -(njInnerProduct(&chao->speed, &normal) *              \
                          terrain->wall_bounce * 2.0F);                        \
                normal.x *= share;                                             \
                normal.y *= share;                                             \
                normal.z *= share;                                             \
                njAddVector(&chao->speed, &normal);                            \
                njOuterProduct(&normal, &chao->speed, &axis);                  \
                if (axis.y > 0.0F) {                                           \
                    work->ang.y -= terrain->wall_turn;                         \
                } else {                                                       \
                    work->ang.y += terrain->wall_turn;                         \
                }                                                              \
            }                                                                  \
        }                                                                      \
    } while (0)

/* the push of the wall found on one level side. the wall pushes where its
 * normal's dot product with the way to that side is under -0.5, facing the
 * Chao, or over 0.5, facing away. its overlap is radius less the point's
 * distance to its plane, or radius plus it for a wall facing away. at_x and
 * at_z may read wall, which is set first. the other names are
 * chao_collide_walls' locals */
#define WALL(side, toward, at_x, at_z)                                         \
    do {                                                                       \
        if (query->sides[side].found) {                                        \
            NJS_VECTOR out;                                                    \
            NJS_POINT3 nearest;                                                \
            NJS_VECTOR push;                                                   \
            NJS_VECTOR axis;                                                   \
            NJS_VECTOR normal;                                                 \
                                                                               \
            out = (toward);                                                    \
            wall = &query->sides[side];                                        \
            if (njInnerProduct(&out, &wall->normal) < -0.5F) {                 \
                WALL_PLANE(at_x, at_z);                                        \
                WALL_PUSH(radius - njDistanceP2PL(&pos, &plane, &nearest));    \
            } else if (njInnerProduct(&out, &wall->normal) > 0.5F) {           \
                WALL_PLANE(at_x, at_z);                                        \
                WALL_PUSH(radius + njDistanceP2PL(&pos, &plane, &nearest));    \
            }                                                                  \
        }                                                                      \
    } while (0)

/* each wall found on a level side of the query's point pushes the Chao, as
 * WALL says. the push is 0.35 of the way toward 2.5 in front of the wall's
 * plane. TERRAIN_WALL says whether a wall pushed the Chao, and
 * TERRAIN_FACING_WALL whether it faced one that did */
static void chao_collide_walls(struct task_work *work, struct alife_chao *chao)
{
    NJS_POINT3 pos;
    NJS_PLANE plane;
    struct chao_terrain *terrain = &chao->terrain;
    struct sides_query *query = &terrain->query;
    struct surface *wall;
    NJS_VECTOR forward;
    Float radius = 2.5F;
    Float depth;
    Float share;

    forward.x = njCos(work->ang.y);
    forward.y = 0.0F;
    forward.z = -njSin(work->ang.y);
    pos = query->pos;
    terrain->flags &= ~TERRAIN_WALL;
    WALL(SIDE_MINUS_X, toward_minus_x, wall->level, pos.z);
    WALL(SIDE_PLUS_X, toward_plus_x, wall->level, pos.z);
    WALL(SIDE_MINUS_Z, toward_minus_z, pos.x, wall->level);
    if (query->sides[SIDE_PLUS_Z].found) {
        NJS_VECTOR out;
        NJS_POINT3 nearest;
        NJS_VECTOR push;
        NJS_VECTOR axis;
        NJS_VECTOR normal;

        out = toward_plus_z;
        wall = &query->sides[SIDE_PLUS_Z];
        if (njInnerProduct(&out, &wall->normal) < -0.5F) {
            WALL_PLANE(pos.x, wall->level);
            WALL_PUSH(radius - njDistanceP2PL(&pos, &plane, &nearest));
            /* @hack redundant: a return where control returns anyway, which
             * stores the yaw at 0x0c90bbfe ahead of the branch to the end, not
             * in its delay slot; without it, 201 of 1560 words differ */
            return;
        } else if (njInnerProduct(&out, &wall->normal) > 0.5F) {
            WALL_PLANE(pos.x, wall->level);
            WALL_PUSH(radius + njDistanceP2PL(&pos, &plane, &nearest));
        }
    }
}

static void terrain_mark_deep(struct task_work *work, struct alife_chao *chao)
{
    if (chao->terrain.water_level > work->pos.y + 1.0F) {
        chao->terrain.flags |= TERRAIN_DEEP;
    }
}

void alife_collide_terrain(struct task_work *work, struct alife_chao *chao)
{
    struct sides_query *query = &chao->terrain.query;

    work->flags &= ~CHAO_ON_GROUND;
    query->pos = work->pos;
    query->pos.y += chao->terrain.query_height;
    terrain_find_sides_and_draw_shadow(&chao->terrain);
    chao_collide_ceiling(work, chao);
    chao_collide_walls(work, chao);
    chao_collide_floor(work, chao);
    terrain_mark_deep(work, chao);
}

void alife_set_target(struct task_work *work, struct alife_chao *chao,
                      NJS_POINT3 *target)
{
    chao->target = *target;
}

void alife_set_home(struct task_work *work, struct alife_chao *chao)
{
    chao->home = work->pos;
}

void alife_save_pos(struct task_work *work, struct alife_chao *chao)
{
    chao->last_pos = work->pos;
}

void alife_restore_pos(struct task_work *work, struct alife_chao *chao)
{
    work->pos = chao->last_pos;
}

Float alife_target_distance(struct task_work *work, struct alife_chao *chao)
{
    Float distance2 = SQUARE(chao->target.x - work->pos.x) +
                      SQUARE(chao->target.y - work->pos.y) +
                      SQUARE(chao->target.z - work->pos.z);

    return njSqrt(distance2);
}

Float alife_target_distance2(struct task_work *work, struct alife_chao *chao)
{
    Float distance2 = SQUARE(chao->target.x - work->pos.x) +
                      SQUARE(chao->target.y - work->pos.y) +
                      SQUARE(chao->target.z - work->pos.z);

    return distance2;
}

/* set aim to the yaw that faces the target. the result is how far the
 * Chao's yaw is from aim, from 0 to 0x8000 */
static Angle aim_at_target(struct task_work *work, struct alife_chao *chao)
{
    Angle yaw = work->ang.y;

    chao->aim = TARGET_HEADING(work, chao);
    /* @hack type: the cast to Uint16, which is the EXTU.W at 0x0c90bf84 and
     * passes angle_distance's 0 to 0x8000 unchanged; without it, 11 of 40
     * words differ */
    return (Uint16)angle_distance(chao->aim, yaw);
}

Angle alife_turn_to_target(struct task_work *work, struct alife_chao *chao)
{
    Angle turn = aim_at_target(work, chao);

    work->ang.y = angle_approach(work->ang.y, chao->aim, chao->turn_speed);
    return turn;
}

Float alife_camera_distance_squared(Float *pos)
{
    /* @hack spelling: point steps from the caller's x into y and z as a Float
     * cursor, for the post-increment loads at 0x0c90bfbe and 0x0c90bfc2; as a
     * byte cursor over the point, 14 of 27 words differ */
    Float *point = pos;
    Float x;
    Float y;
    Float z;
    Float dx;
    Float dy;
    Float dz;

    /* @hack spelling: pos, reused as the cursor over the camera's floats, holds
     * it in R4 and point in R5, copied at 0x0c90bfbc, for the post-increment
     * loads at 0x0c90bfc6 and 0x0c90bfca; with a cursor of its own, 26 of 27
     * words differ */
    pos = VECTOR_FLOATS(&camera_work->pos);
    x = *point++;
    y = *point++;
    z = *point;
    /* @hack spelling: dx, dy and dz loaded with the camera's floats ahead of
     * the subtractions, which moves its y and z through FR3 for their FSUBs
     * (0x0c90bfce, 0x0c90bfd6); subtracted where they load, 21 of 27 words
     * differ */
    dx = *pos++;
    dy = *pos++;
    dz = *pos;
    /* @hack spelling: the squares and their sum in a statement to each step.
     * dx = dx * dx copies each square's operand to FR3 (0x0c90bfdc to
     * 0x0c90bfe4), and the two += keep the sum in FR4, moved to FR0 in the
     * RTS's delay slot (0x0c90bfee); as *=, or one sum of the squares, 18 of
     * 27 words differ */
    dx = x - dx;
    dy = y - dy;
    dz = z - dz;
    dx = dx * dx;
    dy = dy * dy;
    dz = dz * dz;
    dx += dy;
    dx += dz;
    return dx;
}

void alife_apply_speed(struct task_work *work, struct alife_chao *chao)
{
    chao->speed.x += chao->accel.x;
    if (!(work->collision->flags & COLLISION_HIT_BOTTOM)) {
        chao->speed.y += chao->accel.y - 0.05F;
    }
    chao->speed.z += chao->accel.z;
    work->pos.x += chao->speed.x;
    work->pos.y += chao->speed.y;
    work->pos.z += chao->speed.z;
    chao->accel.x = 0.0F;
    chao->accel.y = 0.0F;
    chao->accel.z = 0.0F;
}

void alife_drag(struct task_work *work, struct alife_chao *chao, Float drag)
{
    chao->accel.x = -(drag * chao->speed.x);
    chao->accel.z = -(drag * chao->speed.z);
}

int alife_get_garden(void)
{
    /* @hack type: stage a Uint16, for the EXTU.W at 0x0c90c0c0 after the
     * EXTU.B; as an int, 1 of 29 words differ */
    Uint16 stage = (Uint8)(stage_get_stage_act() >> 8);

    switch (stage) {
    case STAGE_ALIFE_SS:
        return GARDEN_STATION_SQUARE;
    case STAGE_ALIFE_EG:
        return GARDEN_EGG_CARRIER;
    case STAGE_ALIFE_MR:
        return GARDEN_MYSTIC_RUINS;
    }
    return -1;
}
