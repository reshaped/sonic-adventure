/* Cast one sphere against the mesh whose owner published this yaw. */
#ifndef COLLISION_YAW_MESH_H
#define COLLISION_YAW_MESH_H
#include "collision/polygon.h"
#include "collision/bounds.h"
#include "collision/yaw.h"

static Bool yaw_mesh_cast(const struct collision_mesh *mesh, shz_vec3_t pivot,
                          struct yaw_path *path, Float start, Float end,
                          struct yaw_hits *best)
{
    Bool changed = FALSE;
    yaw_prepare(path, end);
    if (path->speed_bound > 0) {
        Float duration = path->tolerance / path->speed_bound;
        if (duration < best->time_tolerance) {
            best->time_tolerance = duration;
        }
    }
    struct yaw_sample a = yaw_at(path, start),
                      b = yaw_at(path, best->contact.time);
    struct sweep_bounds box;
    NJS_POINT3 from;
    NJS_VECTOR chord;
    zam_store_vec3(&from, shz_vec3_add(pivot, a.center));
    zam_store_vec3(&chord, shz_vec3_sub(b.center, a.center));
    Float span = best->contact.time - start;
    sweep_extents(&box, from, chord,
                  path->radius + path->acceleration * span * span * 0.125F +
                      path->tolerance);
    const struct polygon *stop = mesh->last->next;
    for (const struct polygon *tri = mesh->polygons; tri != stop;
         tri = tri->next) {
        struct impact_triangle face;
        if (sweep_box_misses(&box, tri->vertex) ||
            !impact_triangle_init(&face, tri->vertex)) {
            continue;
        }
        struct yaw_contact found;
        enum yaw_result kind = yaw_interval(
            path, &face, shz_vec3_sub(zam_load_vec3(tri->vertex), pivot),
            tri->attr & NJD_FLAG_DOUBLE_SIDE, a, b, &found);
        if (kind == YAW_UNCERTAIN && found.safe < best->unresolved) {
            best->unresolved = found.safe;
        } else if (kind == YAW_TOUCH &&
                   (!best->face || found.time < best->contact.time)) {
            best->contact = found;
            best->face = tri;
            b = yaw_at(path, found.time);
            changed = TRUE;
        }
    }
    return changed;
}

#endif
