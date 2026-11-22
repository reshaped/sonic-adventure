/* Finite floor coverage along a straight foot segment. */
#ifndef COLLISION_SUPPORT_H
#define COLLISION_SUPPORT_H
#include "collision/impact.h"
/* Disjoint spans fit in 128 bytes. A full buffer revisits the suffix. */
#define GROUND_SPANS 16

struct ground_spans {
    struct {
        Float lo, hi;
    } span[GROUND_SPANS];
    int count;
    Float omitted; /* earliest start of a discarded interval */
};

/* Each projected edge bounds the part of the segment inside the triangle. */
ALWAYS_INLINE Bool ground_clip_interval(const struct impact_triangle *face,
                                        shz_vec3_t start, shz_vec3_t move,
                                        int u, int v, Float normal,
                                        Float tolerance, Float *lo, Float *hi)
{
    *lo = 0.0F;
    *hi = 1.0F;
    for (int i = 0; i < 3; i++) {
        const shz_vec3_t *a = &face->v[i], *b = &face->v[(i + 1) % 3];
        Float x = b->e[u] - a->e[u], y = b->e[v] - a->e[v];
        Float h = y * (start.e[u] - a->e[u]) - x * (start.e[v] - a->e[v]);
        Float delta = y * move.e[u] - x * move.e[v];
        if (normal < 0.0F) {
            h = -h;
            delta = -delta;
        }
        h += tolerance * (njAbs(x) + njAbs(y));
        Float end = h + delta;
        if (h < 0.0F) {
            if (end < 0.0F) {
                return FALSE;
            }
            Float t = -h / delta;
            if (t > *lo) {
                *lo = t;
            }
        } else if (end < 0.0F) {
            Float t = h / -delta;
            if (t < *hi) {
                *hi = t;
            }
        }
    }
    return *lo <= *hi;
}

/* An airborne foot tests the ground directly below its path. */
LINKED_LAST(ground_footprint)
static __attribute__((noinline, optimize("fp-contract=off"))) Bool
ground_footprint(const struct impact_triangle *face, shz_vec3_t start,
                 shz_vec3_t move, Float tolerance, Float *lo, Float *hi)
{
    return ground_clip_interval(face, start, move, 0, 2, face->normal.y,
                                tolerance, lo, hi);
}

/* Drop the largest normal component so a vertical running surface keeps
 * its full extent along the direction of travel. */
LINKED_LAST(ground_interval)
static __attribute__((noinline, optimize("fp-contract=off"))) Bool
ground_interval(const struct impact_triangle *face, shz_vec3_t start,
                shz_vec3_t move, Float tolerance, Float *lo, Float *hi)
{
    shz_vec3_t n = face->normal;
    if (njAbs(n.y) >= njAbs(n.x) && njAbs(n.y) >= njAbs(n.z)) {
        return ground_footprint(face, start, move, tolerance, lo, hi);
    }
    Bool drop_x = njAbs(n.x) >= njAbs(n.z);
    return ground_clip_interval(face, start, move, drop_x ? 2 : 1,
                                drop_x ? 1 : 0, drop_x ? n.x : n.z, tolerance,
                                lo, hi);
}

/* Keep disjoint intervals sorted. Duplicate faces and triangles on the same
 * continuous sheet coalesce as they arrive. */
static void ground_add_span(struct ground_spans *s, Float lo, Float hi)
{
    int first = 0;
    while (first < s->count && s->span[first].hi < lo) {
        first++;
    }
    int end = first;
    while (end < s->count && s->span[end].lo <= hi) {
        if (s->span[end].lo < lo) {
            lo = s->span[end].lo;
        }
        if (s->span[end].hi > hi) {
            hi = s->span[end].hi;
        }
        end++;
    }
    if (end == first) {
        if (s->count == GROUND_SPANS) {
            /* Keep the nearest intervals. A gap before the earliest dropped
             * one is final, even when more distant spans would overflow. */
            Float dropped = first == s->count ? lo : s->span[s->count - 1].lo;
            if (dropped < s->omitted) {
                s->omitted = dropped;
            }
            if (first == s->count) {
                return;
            }
            s->count--;
        }
        for (int j = s->count; j > first; j--) {
            s->span[j] = s->span[j - 1];
        }
        s->count++;
    } else {
        for (int j = end; j < s->count; j++) {
            s->span[first + 1 + j - end] = s->span[j];
        }
        s->count -= end - first - 1;
    }
    s->span[first].lo = lo;
    s->span[first].hi = hi;
}

static inline Float ground_prefix(const struct ground_spans *spans, Float from)
{
    return spans->count && spans->span[0].lo <= from ? spans->span[0].hi : from;
}

struct ground_edge {
    shz_vec3_t start, direction;
    Float length2;
};

static Bool ground_on_edge_line(const struct ground_edge *edge,
                                const NJS_POINT3 *vertex, Float tolerance)
{
    shz_vec3_t from = shz_vec3_sub(zam_load_vec3(vertex), edge->start);
    shz_vec3_t area = plane_cross(from, edge->direction);
    return plane_dot(area, area) <= tolerance * tolerance * edge->length2;
}

/* Mark the directions along the crease occupied by this triangle. Testing
 * crosswise on the sheet avoids losing a factor of sin(angle) at shallow bends.
 */
static unsigned ground_edge_sides(const struct ground_edge *line,
                                  shz_vec3_t axis, const NJS_POINT3 *vertex,
                                  const shz_vec3_t v[3], shz_vec3_t n,
                                  shz_vec3_t other, Float tolerance)
{
    unsigned sides = 0;
    shz_vec3_t across = plane_cross(n, axis);
    if (plane_dot(across, other) < 0.0F) {
        across = shz_vec3_scale(across, -1.0F);
    }
    for (int i = 0; i < 3; i++) {
        shz_vec3_t a = v[i], b = v[(i + 1) % 3];
        if (plane_dot(across, v[(i + 2) % 3]) <= tolerance ||
            !ground_on_edge_line(line, vertex + i, tolerance) ||
            !ground_on_edge_line(line, vertex + (i + 1) % 3, tolerance)) {
            continue;
        }
        Float x = plane_dot(axis, a), y = plane_dot(axis, b);
        if (x > y) {
            Float t = x;
            x = y;
            y = t;
        }
        if (x > tolerance || y < -tolerance) {
            continue;
        }
        if (x < -tolerance) {
            sides |= 1;
        }
        if (y > tolerance) {
            sides |= 2;
        }
    }
    return sides;
}

#endif
