/* the collision meshes of the world's objects, and tests of points, balls,
 * rays and segments against those objects */
#ifndef COLLISION_POLYGON_H
#define COLLISION_POLYGON_H
#include <ninja.h>
#include <sg_xpt.h>

/* a surface a test found for a point, when found is TRUE */
struct surface {
    Bool found;
    Sint32 flags; /* its object's world flags, which only a mesh's tests set */
    /* the angles that turn y to point along normal, as vector_aim_y gives
     * them */
    Angle ang_x;
    Angle ang_z;
    /* its coordinate on the axis the test looks along: for a floor, its
     * height */
    Float level;
    NJS_VECTOR normal; /* a unit vector */
};

/* a point, and the floor a test finds at its x and z, above or below it */
struct floor_query {
    NJS_POINT3 pos;
    struct surface floor;
};

/* a point, and a surface below it and one above it */
struct height_query {
    NJS_POINT3 pos;
    struct surface below;
    struct surface above;
};

/* a point, and the surface of one object nearest it straight above or
 * below */
struct line_query {
    Float radius; /* with speed, widens the reach a triangle is tested in */
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    Uint8 unk_1c[20];
    Float level; /* the surface's height */
    Angle ang_x; /* its angles, as in struct surface */
    Angle ang_z;
    NJS_VECTOR normal; /* a unit vector */
    Uint8 unk_48[60];
};

/* the sides of a point along each axis, in the order sides_query holds
 * them */
enum side {
    SIDE_MINUS_X,
    SIDE_PLUS_X,
    SIDE_BELOW,
    SIDE_ABOVE,
    SIDE_MINUS_Z,
    SIDE_PLUS_Z,
    SIDES
};

/* a point, and the nearest surface on each side of it along each axis */
struct sides_query {
    NJS_POINT3 pos;
    struct surface sides[SIDES];
};

/* a ray, and the nearest surface it meets */
struct ray_query {
    /* polygon_cast_ray makes it 400 when it's over 400 or under 0.01 */
    Float length;
    NJS_POINT3 origin;
    NJS_VECTOR dir; /* a unit vector */
    Float distance; /* from origin to hit */
    NJS_POINT3 hit;
    struct surface surface;
};

/* a segment from pos to pos + dir, and where it crosses the front of a
 * triangle */
struct segment_query {
    NJS_POINT3 pos;
    NJS_VECTOR dir;
    NJS_POINT3 hit;
    /* (b - a) x (c - a) for the triangle's corners a, b and c, as long as
     * twice the triangle's area */
    NJS_VECTOR normal;
    Angle ang_x; /* the angles vector_aim_y gives for normal */
    Angle ang_z;
    Sint32 flags; /* the world flags of the triangle's object */
    Float distance; /* where hit lies on the segment, from 0 at pos to 1 */
};

/* a surface a sweep's ball touched, in the world */
struct polygon_hit {
    Angle ang_x; /* the angles for normal, as in struct surface */
    Angle ang_z;
    NJS_VECTOR front; /* the unit normal of its front */
    /* front, or its reverse for a ball behind a two-sided surface */
    NJS_VECTOR normal;
    NJS_POINT3 point; /* the point of its plane nearest the ball's pos */
    NJS_POINT3 pos; /* point moved the ball's radius along normal */
    NJS_VECTOR speed; /* the part of the ball's speed along the surface */
    NJS_VECTOR normal_speed; /* and the part along front */
};

/* a ball a sweep moves from pos along speed, and the surface it hit */
struct polygon_ball {
    Float radius;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    NJS_VECTOR normal; /* of the ground it touches */
    Uint16 count; /* of the surfaces in polygon_contacts */
    Uint16 hit_index; /* which of polygon_contacts it hit */
    Uint8 unk_2c[8];
    struct polygon_hit hit;
};

/* a surface the last sweep touched */
struct polygon_contact {
    struct polygon_hit hit;
    Float height; /* the ball's height over the surface's plane, along front */
    Float height_after; /* the same, once the ball's speed has moved it */
};

/* the most surfaces a sweep lists */
#define POLYGON_CONTACTS 16
/* the surfaces touched by the last sweep that hit, as many as its ball's
 * count */
/* @hack type: the declaration without its length, which has
 * player_collide_world scale ball->hit_index by MULU.W (0x8c078364); with
 * POLYGON_CONTACTS, it scales by MULS.W and EXTS.W, player.c's code runs 32
 * bytes long, and, summed over the seven functions that change, 683 of 4612
 * words differ */
extern struct polygon_contact polygon_contacts[];

/* a triangle of a collision mesh, in the world. a mesh lists its triangles
 * from the lowest center z up, to within mesh.h's buckets */
struct polygon {
    struct polygon *next;
    struct polygon *prev;
    NJS_POINT3 center; /* its corners' sum times 0.3333 */
    /* the squared distance from center to its farthest corner.
     * collision_mesh_init sets it to 0, which polygon_pool_seek_free takes
     * for a triangle no mesh holds */
    Float radius;
    NJS_POINT3 vertex[3];
    Uint32 attr; /* its material's attrflags, or 0 for a model with none */
};

/* the collision mesh of one of the world's objects: its models' triangles,
 * where the object puts them in the world */
struct collision_mesh {
    struct collision_mesh *next;
    struct collision_mesh *prev;
    NJS_OBJECT *object; /* null when the mesh is free or forgotten */
    Sint32 flags; /* the object's world flags */
    struct polygon *polygons; /* the first and the last */
    struct polygon *last;
    Sint32 unk_18;
    /* where polygon_collect_near's list holds this mesh's triangles, or
     * null */
    struct polygon **near;
    Sint32 unk_20;
    int near_count; /* and how many it holds */
    Float span; /* the longest any of its triangles is along z */
    Float unk_2c;
    NJS_POINT3 center; /* its root model's center, in the world */
    Sint32 unk_3c;
};

/* collision_mesh_init frees every mesh and polygon, as a stage starts.
 * collision_mesh_update runs each step. it frees the meshes of moving
 * objects and of objects no longer near the players. it then makes a mesh
 * for each near object without one, the moving ones included. when 192
 * objects want meshes, it frees nothing first. the pool gives out at most
 * 191 meshes at once, and the update writes a mesh past them through
 * null. both return TRUE */
Bool collision_mesh_init(void);
Bool collision_mesh_update(void);
/* the next update makes meshes for every object of the world, not only
 * those near the players */
void collision_mesh_collect_all(void);
/* free every polygon and mesh, and empty collision_mesh_list, as
 * collision_mesh_init does. the meshes' objects and the polygons' radii
 * keep their values */
void collision_mesh_link_pools(void);
/* move the pool's cursor on to the first polygon from it whose radius is
 * 0, or to null past the pool's end. nothing points the cursor into the
 * pool, so a call reads a radius through null */
void polygon_pool_seek_free(void);
/* set the object of object's mesh to null, as object leaves the world. a
 * later update frees the mesh */
void collision_mesh_forget(NJS_OBJECT *object);

/* list the triangles polygon_sweep tests for the ball, mesh by mesh, and
 * return how many, at most 96. each mesh's near and near_count give its
 * part of the list. once the list is full, the meshes left keep the near
 * and near_count they had.
 *
 * only the meshes of objects world_list_near lists within the ball's
 * radius of pos count. of each, only the triangles whose centers lie
 * within its span plus reach of pos along z count. reach is the ball's
 * radius plus speed's length. a triangle's limit is reach * reach plus its
 * radius, itself a square. it is near when its center's squared distance
 * from pos is at most 1.3 times its limit */
int polygon_collect_near(struct polygon_ball *ball);
/* sweep the ball over the triangles polygon_collect_near listed from
 * object's mesh. the ball's center steps from pos toward pos + speed,
 * radius / 2 at a time, and ends at pos + speed. it touches a triangle
 * when, at one of those points, it lies within radius of the triangle's
 * plane, over the triangle along its normal. flags goes unread.
 *
 * it returns TRUE when the ball touches any, and lists them in
 * polygon_contacts, at most 16. the ball takes their count, and the index
 * of the touch with the lowest height_after. its hit is that touch's, with
 * point and pos averaged over all of them. the 16th touch ends the sweep,
 * and leaves one matrix pushed on the matrix stack.
 *
 * it returns FALSE and changes neither for a ball whose radius isn't over
 * 0, an object with no mesh, or no touch */
Bool polygon_sweep(struct polygon_ball *ball, NJS_OBJECT *object, Uint32 flags);
/* sweep the ball as polygon_sweep does, over the triangles of the models
 * of object's tree, each where the tree puts it in the world. a triangle's
 * reach, squared, is its longest edge's square plus those of speed's
 * length and the radius. it is tested only when each of its corners lies
 * within that reach of pos. flags changes nothing.
 *
 * the sweep starts no new strip of triangles once it has 16 touches. a
 * strip it is in goes on, though, and writes any touch past 16 past
 * polygon_contacts' end. the ball's hit is the touch whose height is
 * nearest 0: the first touch's height before the move, and each other's
 * height_after */
Bool polygon_sweep_model(struct polygon_ball *ball, NJS_OBJECT *object,
                         Uint32 flags);

/* polygon_find_sides to polygon_find_near_levels take a triangle for a
 * point when the point, seen along the test's axis, lies inside the
 * triangle or on an edge. the triangle's projection across that axis must
 * also have an area of at least 0.005. polygon_find_nearest_height, and
 * polygon_find_near_levels through it, take any area over 0. each Bool test
 * returns TRUE when it finds a surface, and FALSE when it finds none */

/* take for each side of the query's point the nearest surface of object's
 * mesh that the line through the point along the side's axis crosses. a
 * crossing at the point counts for the plus side, or above. a triangle
 * counts only when its first corner lies within 220 of the point. a side
 * with no surface has found FALSE and its level 220 past the point the
 * other way, even for an object with no mesh. flags goes unread */
Bool polygon_find_sides(struct sides_query *query, NJS_OBJECT *object,
                        Uint32 flags);
/* take the surface of object's own model nearest the query's point
 * straight above or below it. the query's radius and speed widen a reach
 * test each triangle must pass. a shipped bug has that test measure to
 * stack words, not to the triangle's corners. with no surface, the query
 * keeps its values. flags goes unread */
Bool polygon_find_nearest_height(struct line_query *query, NJS_OBJECT *object,
                                 Uint32 flags);
/* take the highest surface of object's own model under the query's point,
 * and the lowest at or over it. a side with no surface has found FALSE, and
 * the rest of it keeps its values */
Bool polygon_find_model_heights(struct height_query *query, NJS_OBJECT *object);
/* take the sides' surfaces as polygon_find_sides does, from object's own
 * model. in place of polygon_find_sides' limit of 220, each triangle must
 * pass a reach test like polygon_find_nearest_height's. its reach comes
 * from the size in flags, read from past the end of its table for any
 * object smaller than WORLD_HUGE. a side with no surface has found FALSE,
 * and the rest of it keeps its values. no surface's flags change */
Bool polygon_find_model_sides(struct sides_query *query, NJS_OBJECT *object,
                              Uint32 flags);
/* take for each of the world's objects within radius of the query's point
 * its surfaces below and above the point, as polygon_find_model_heights
 * does. a triangle must also pass polygon_find_model_sides' reach test. the
 * query's below takes the lowest of those below, and its above the highest
 * of those above. each takes found, and level where found is TRUE. the
 * rest of each, and a level whose found is FALSE, take stack words no code
 * wrote */
Bool polygon_find_near_heights(struct height_query *query, Float radius);
/* take for each of the world's objects within radius of pos the one
 * surface polygon_find_nearest_height finds. *above gets the lowest of
 * those over pos, and *below the highest of those at or under it. each
 * gets its angles too, where those pointers aren't null. a level with no
 * surface ends on the wrong side of pos: *above at pos->y - 100000, and
 * *below at pos->y + 100000 */
void polygon_find_near_levels(NJS_POINT3 *pos, Float radius, Float *above,
                              Angle *above_x, Angle *above_z, Float *below,
                              Angle *below_x, Angle *below_z);

/* cast the query's ray at the meshes of the world's objects within its
 * length of its origin. it meets a one-sided triangle only from the front.
 * it returns TRUE when it meets a surface within length, and takes the
 * nearest: its distance, hit and surface, but not surface.level. the
 * surface's normal is its front's, from either side. it returns FALSE when
 * it meets none, with surface.found FALSE, and the rest of the query as it
 * was but for length */
Bool polygon_cast_ray(struct ray_query *query);
/* the first object whose mesh the query's segment crosses from the front,
 * or null. the query takes the nearest crossing of that mesh. a first that
 * isn't null is tried before the others, with the query's flags as its
 * world flags. the world's objects within 5 of pos come next, but for
 * first. of those, an object a task owns is tried only when its distance
 * from pos across x and z is at most its reach. that reach is the
 * segment's length across x and z, plus its model's radius at its largest
 * scale, plus 1 */
NJS_OBJECT *polygon_cast_segment(struct segment_query *query,
                                 NJS_OBJECT *first);

/* the triangle's normal (v1 - v0) x (v2 - v0), as long as twice its area.
 * each point is the address of an NJS_POINT3, as a char pointer */
void polygon_normal(const char *v0, const char *v1, const char *v2,
                    NJS_VECTOR *normal);
/* the squared distance between the points */
Float polygon_point_distance2(NJS_POINT3 *from, NJS_POINT3 *to);
/* the same over x and z alone */
Float polygon_point_distance2_across(NJS_POINT3 *from, NJS_POINT3 *to);
/* the squared distance between the three Floats at each pointer */
Float polygon_distance2(Float *from, Float *to);

/* how far point is from the segment from start to end. the segment's
 * nearest point goes to nearest, if it isn't null */
Float collision_segment_distance(NJS_POINT3 *point, NJS_POINT3 *start,
                                 NJS_POINT3 *end, NJS_POINT3 *nearest);
/* TRUE when point lies beside the segment from start to end, not past
 * either end. otherwise FALSE, and it changes nothing. for TRUE, distance2
 * gets the squared distance from point to the segment, and nearest the
 * segment's nearest point. a segment under 0.01 long along each axis counts
 * as start, and leaves nearest as it was */
Bool collision_segment_nearest(NJS_POINT3 *start, NJS_POINT3 *end,
                               NJS_POINT3 *point, NJS_POINT3 *nearest,
                               Float *distance2);

#endif
