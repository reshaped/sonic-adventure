/* terrain probes test the volume swept by a sphere along a segment */
#ifndef COLLISION_CAPSULE_H
#define COLLISION_CAPSULE_H

#include <ninja.h>

static inline NJS_VECTOR capsule_sub(NJS_VECTOR a, NJS_VECTOR b)
{
    return (NJS_VECTOR){a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline NJS_VECTOR capsule_at(NJS_VECTOR p, NJS_VECTOR v, Float t)
{
    return (NJS_VECTOR){p.x + t * v.x, p.y + t * v.y, p.z + t * v.z};
}

static inline NJS_VECTOR capsule_cross(NJS_VECTOR a, NJS_VECTOR b)
{
    return (NJS_VECTOR){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                        a.x * b.y - a.y * b.x};
}

static inline Float capsule_dot(NJS_VECTOR a, NJS_VECTOR b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/* fraction of the nearer point on a segment, including a stationary one */
static inline Float capsule_fraction(Float along, Float length2)
{
    if (along <= 0.0F) {
        return 0.0F;
    }
    return along >= length2 ? 1.0F : along / length2;
}

/* from is the sphere center relative to the first end of the edge */
LINKED_LAST(capsule_edge)
static __attribute__((noinline)) Bool capsule_edge(NJS_VECTOR from,
                                                   NJS_VECTOR speed,
                                                   Float speed2,
                                                   NJS_VECTOR edge,
                                                   Float radius2)
{
    Float length2 = capsule_dot(edge, edge);
    if (speed2 == 0.0F) {
        Float along = capsule_fraction(capsule_dot(from, edge), length2);
        NJS_VECTOR distance = capsule_at(from, edge, -along);
        return capsule_dot(distance, distance) <= radius2;
    }
    NJS_VECTOR offset = capsule_cross(from, edge);
    NJS_VECTOR across = capsule_cross(speed, edge);
    Float t = capsule_fraction(-capsule_dot(offset, across),
                               capsule_dot(across, across));
    NJS_VECTOR at = capsule_at(from, speed, t);
    Float along = capsule_dot(at, edge);
    if (along > 0.0F && along < length2) {
        NJS_VECTOR distance = capsule_cross(at, edge);
        return capsule_dot(distance, distance) <= radius2 * length2;
    }
    /* the point nearest the supporting line lies past an end. that end
     * is the closest feature of the finite edge */
    if (along >= length2) {
        from = capsule_sub(from, edge);
    }
    t = capsule_fraction(-capsule_dot(from, speed), speed2);
    at = capsule_at(from, speed, t);
    return capsule_dot(at, at) <= radius2;
}

/* a probe senses either side. if its point nearest the plane projects
 * outside the face, any contact in the face interior also reaches an edge
 * on the way to that point */
LINKED_LAST(capsule_triangle)
static __attribute__((noinline)) Bool
capsule_triangle(const NJS_POINT3 vertex[3], NJS_POINT3 center,
                 NJS_VECTOR speed, Float speed2, Float radius2)
{
    NJS_VECTOR ab = capsule_sub(vertex[1], vertex[0]);
    NJS_VECTOR ac = capsule_sub(vertex[2], vertex[0]);
    NJS_VECTOR normal = capsule_cross(ab, ac);
    Float area2 = capsule_dot(normal, normal);
    if (!(area2 > 0.0F)) {
        return FALSE;
    }
    NJS_VECTOR from = capsule_sub(center, vertex[0]);
    Float start = capsule_dot(normal, from);
    Float travel = capsule_dot(normal, speed);
    Float end = start + travel;
    Float t = 0.0F;
    Float height = start;
    if ((start < 0.0F && end > 0.0F) || (start > 0.0F && end < 0.0F)) {
        t = -start / travel;
        height = 0.0F;
    } else if (__builtin_fabsf(end) < __builtin_fabsf(start)) {
        t = 1.0F;
        height = end;
    }
    if (height * height > radius2 * area2) {
        return FALSE;
    }
    NJS_VECTOR at = capsule_at(from, speed, t);
    NJS_VECTOR bc = capsule_sub(vertex[2], vertex[1]);
    if (capsule_dot(capsule_cross(ab, at), normal) >= 0.0F &&
        capsule_dot(capsule_cross(at, ac), normal) >= 0.0F &&
        capsule_dot(capsule_cross(bc, capsule_sub(at, ab)), normal) >= 0.0F) {
        return TRUE;
    }
    return capsule_edge(from, speed, speed2, ab, radius2) ||
           capsule_edge(from, speed, speed2, ac, radius2) ||
           capsule_edge(capsule_sub(center, vertex[1]), speed, speed2, bc,
                        radius2);
}
#endif
