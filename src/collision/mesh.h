/* collision meshes in assembly: their pools, the buckets that sort a new
 * mesh's polygons by z, and the searches for a mesh's surfaces */
#ifndef COLLISION_MESH_H
#define COLLISION_MESH_H
#include <ninja.h>
#include <sg_xpt.h>

struct collision_mesh;
struct floor_query;
struct height_query;
struct polygon;

/* the meshes in use, newest first. the first one's prev holds this list's
 * address, negated, not a mesh's */
extern struct collision_mesh *collision_mesh_list;

/* a pool's free records lie in a list linked through next. an allocation
 * returns null rather than take the list's last record */

/* make polygons[0..count) the free polygons, writing only their next.
 * count must be at least 2 */
void polygon_pool_init(struct polygon *polygons, int count);
/* make meshes[0..count) the free meshes, writing only their next, and
 * empty collision_mesh_list. count must be at least 2 */
void collision_mesh_pool_init(struct collision_mesh *meshes, int count);
/* take a free mesh and put it first in collision_mesh_list, writing only
 * its next and prev. returns it, or null */
struct collision_mesh *collision_mesh_alloc(void);
/* return the mesh's polygons to their free list, and set its object to
 * null. then take the mesh out of collision_mesh_list, and put it on its
 * free list. its polygons and last keep their values */
void collision_mesh_free(struct collision_mesh *mesh);
/* take a free polygon and put it first in the mesh's polygons, writing
 * only its next and prev. returns it, or null */
struct polygon *collision_mesh_add_polygon(struct collision_mesh *mesh);

/* polygon_alloc sorts a new mesh's polygons into 128 buckets by its
 * argument z. polygon.c gives z as the polygon's center z less that of the
 * mesh's root model. polygon_alloc cuts z toward 0 to a whole number. each
 * bucket takes 16 whole numbers, from -1024 up, and the first and last also
 * take every one past them */

/* empty every bucket */
void collision_mesh_clear_buckets(void);
/* take a free polygon and put it first in the bucket for z, writing only
 * its next and prev. returns it, or null */
struct polygon *polygon_alloc(Float z);
/* join the buckets' lists, lowest z first, as the mesh's polygons, from
 * mesh->polygons to mesh->last. within a bucket, the newest polygon comes
 * first. with every bucket empty, it sets only mesh->last, to null */
void collision_mesh_take_buckets(struct collision_mesh *mesh);

/* polygon_find_floor and polygon_find_heights find the first mesh in
 * collision_mesh_list whose object is object. they test its polygons near
 * the query's point, as mesh.src says. each returns TRUE when it finds a
 * surface, and FALSE when it finds none. neither reads flags.
 *
 * a polygon gives a surface when the point's x and z lie inside its three
 * sides, not on one. its projection on x and z must also have an area over
 * 0.005.
 *
 * the surface's level is the polygon's y at the point's x and z, and its
 * flags are the mesh's world flags. its normal n is (b - a) x (c - a) for
 * the polygon's points a, b and c, made a unit vector. its ang_x is
 * asinf(n.z), and its ang_z -atan2(n.x, n.y), in Ninja angles */

/* query->floor takes the first surface the search finds. a later one
 * replaces it only by a test that reads the normal's y for the point's y.
 * that test and the search's skip to its window are shipped bugs, which
 * mesh.src describes. with no surface, floor.found is FALSE and the rest of
 * query keeps its values */
Bool polygon_find_floor(struct floor_query *query, NJS_OBJECT *object,
                        Uint32 flags);
/* query->below takes the highest surface at or under the point's y, and
 * query->above the lowest over it. a side with no surface has found FALSE,
 * and its level is the point's y less 220 for below, or plus 220 for above.
 * with no mesh for object, both levels keep their values */
Bool polygon_find_heights(struct height_query *query, NJS_OBJECT *object,
                          Uint32 flags);

Float vector_length_squared(Float x, Float y, Float z);
/* the squared distance between the points, and the same over x and z
 * alone */
Float mesh_point_distance2(NJS_POINT3 *from, NJS_POINT3 *to);
Float mesh_point_distance2_across(NJS_POINT3 *from, NJS_POINT3 *to);
/* transform the point in place by the matrix in XMTRX, the FPU's back
 * bank, translation included */
void matrix_calc_point(NJS_POINT3 *point);

#endif
