/* Continuous sphere contacts on the native port's terrain geometry. */
#include <ninja.h>
#include <math.h>
#include <stdio.h>

#include "play_math.h"

/* The host test links outside the Dreamcast kernel's section layout. */
#undef LINKED_LAST
#define LINKED_LAST(name)
#include "collision/support.h"

Float njSqrt(Float value)
{
    return sa_fsqrt(value);
}

/* These three perpendicular axes have length three. Integer inputs rotate
 * into an oblique plane without rounding, and a radius of five becomes 15. */
static shz_vec3_t cast_axes(NJS_VECTOR v)
{
    return (shz_vec3_t){.x = v.x + 2 * v.y - 2 * v.z,
                        .y = 2 * v.x + v.y + 2 * v.z,
                        .z = 2 * v.x - 2 * v.y - v.z};
}

static int crossings(void)
{
    const NJS_POINT3 points[] = {{0, 0, 0}, {16, 0, 0}, {0, 12, 0}};
    /* Each outward vector is perpendicular to a supporting plane at its
     * feature. Moving the sphere backward increases its gap from that plane. */
    const struct {
        NJS_POINT3 point;
        NJS_VECTOR outward, move;
    } hits[] = {{{4, 3, 0}, {0, 0, 5}, {4, -2, -9}}, /* face */
                {{8, 0, 0}, {0, -4, 3}, {2, 8, -6}}, /* edge */
                {{0, 0, 0}, {-4, -3, 0}, {9, 7, 2}}}; /* vertex */
    const Float times[] = {1.0F / 256, 0.5F, 255.0F / 256};
    for (int order = 0; order < 6; order++) {
        NJS_POINT3 v[3];
        for (int i = 0; i < 3; i++) {
            int index = (order + (order < 3 ? i : 2 * i)) % 3;
            zam_store_vec3(v + i, cast_axes(points[index]));
        }
        struct impact_triangle face;
        if (!impact_triangle_init(&face, v)) {
            return 1;
        }
        for (int which = 0; which < 3; which++) {
            shz_vec3_t at =
                shz_vec3_sub(cast_axes(hits[which].point), zam_load_vec3(v));
            at = shz_vec3_add(at, cast_axes(hits[which].outward));
            shz_vec3_t move = cast_axes(hits[which].move);
            for (int i = 0; i < 3; i++) {
                Float t, expected = times[i];
                shz_vec3_t start =
                    shz_vec3_sub(at, shz_vec3_scale(move, expected));
                if (!impact_time(&face, start, move, 15, 0.0001F, TRUE, 1,
                                 &t) ||
                    njAbs(t - expected) > 0.00001F ||
                    impact_time(&face, start, move, 15, 0.0001F, TRUE,
                                expected - 1.0F / 512, &t)) {
                    fprintf(stderr,
                            "wrong contact for feature %d, order %d, time %g\n",
                            which, order, (double)expected);
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* At this sharp tip the closest point is the origin for every vertex order. */
static int tip(const NJS_POINT3 vertex[3])
{
    struct impact_triangle face;
    if (!impact_triangle_init(&face, vertex)) {
        return 1;
    }
    for (int hit = 0; hit <= 1; hit++) {
        Float xy = hit ? -0.75F : -1.25F;
        NJS_VECTOR start = {xy, xy, 1.5F};
        shz_vec3_t p =
            shz_vec3_sub(zam_load_vec3(&start), zam_load_vec3(vertex));
        struct contact_plane contact;
        if ((impact_contact(&face, p, 2, 0, &contact) != IMPACT_NONE) != hit) {
            return 2;
        }
        start.z = 3;
        p = shz_vec3_sub(zam_load_vec3(&start), zam_load_vec3(vertex));
        shz_vec3_t down = {.z = -1.5F};
        Float time;
        if (impact_time(&face, p, down, 2, 0, FALSE, 1, &time) != hit) {
            return 3;
        }
        if (hit &&
            njAbs(time - (3 - njSqrt(4 - 2 * xy * xy)) / 1.5F) > 0.0001F) {
            return 4;
        }
    }
    return 0;
}

/* In the tilted pair, moving either way along X increases the minimum
 * Y required by the walls. With the third plane the nearest point is
 * (0,1,1). Every wall starts with a positive penetration. */
static int overlap_planes(void)
{
    const Float side = 65535.0F / 65537, rise = 512.0F / 65537;
    const Float tolerance = 5.0F / 65536;
    const struct contact_plane skew[] = {{{.x = -side, .y = rise}, rise},
                                         {{.y = -0.6F, .z = 0.8F}, 0.2F},
                                         {{.x = side, .y = rise}, rise}};
    for (int first = 0; first < 3; first++) {
        for (int step = 1; step <= 2; step++) {
            struct contact_plane plane[3];
            for (int i = 0; i < 3; i++) {
                plane[i] = skew[(first + step * i) % 3];
            }
            shz_vec3_t result;
            if (!planes_project(plane, 3, (shz_vec3_t){0}, tolerance,
                                &result) ||
                njAbs(result.x) > 0.0005F || njAbs(result.y - 1) > 0.0005F ||
                njAbs(result.z - 1) > 0.0005F) {
                return 1;
            }
        }
    }
    /* These three faces intersect at (2,-1,0). Twice the first raw normal
     * plus the other two is four times that point, so it is also the
     * shortest translation from the origin. */
    const shz_vec3_t normals[] = {{.x = -8190, .y = -16385, .z = -16384},
                                  {.x = 8192, .y = 16379, .z = 16389},
                                  {.x = 8196, .y = 16387, .z = 16379}};
    struct contact_plane throat[3];
    for (int i = 0; i < 3; i++) {
        shz_vec3_t n = normals[i];
        n = shz_vec3_scale(n, 1.0F / sqrtf(plane_dot(n, n)));
        throat[i] = (struct contact_plane){n, 2 * n.x - n.y};
    }
    shz_vec3_t push;
    if (!planes_project(throat, 3, (shz_vec3_t){0}, tolerance, &push) ||
        njAbs(push.x - 2) > 0.005F || njAbs(push.y + 1) > 0.005F ||
        njAbs(push.z) > 0.005F) {
        return 4;
    }
    for (int first = 0; first < 2; first++) {
        struct contact_plane slab[2];
        slab[first] = (struct contact_plane){{.x = 1}, 1};
        slab[1 - first] = (struct contact_plane){{.x = -1}, -3};
        shz_vec3_t result;
        if (!planes_project(slab, 2, (shz_vec3_t){.x = 2, .y = 3}, tolerance,
                            &result) ||
            result.x != 2 || result.y != 3 || result.z != 0 ||
            !planes_project(slab, 2, (shz_vec3_t){.x = 4, .y = 3}, tolerance,
                            &result) ||
            result.x != 3 || result.y != 3 || result.z != 0) {
            return 2;
        }
        slab[1 - first].offset = -1;
        if (!planes_project(slab, 2, (shz_vec3_t){.y = 3}, tolerance,
                            &result) ||
            result.x != 1 || result.y != 3 || result.z != 0) {
            return 2;
        }
        slab[1 - first].offset = -0.5F;
        if (planes_project(slab, 2, (shz_vec3_t){0}, tolerance, &result) ||
            result.x != 1 || result.y != 3 || result.z != 0) {
            return 3;
        }
    }
    return 0;
}

/* In the triangle (0, 8a, 8b), the line u*a + 2b lies inside for
 * 0 <= u <= 6. Turning the sheet through vertical preserves that interval. */
static int support_intervals(void)
{
    static const NJS_VECTOR basis[][2] = {{{1, 0, 0}, {0, 0, 1}},
                                          {{0, 1, 0}, {0, 0, 1}},
                                          {{0, 1, 0}, {1, 0, 0}},
                                          {{1, 8, 0}, {0, 0, 1}},
                                          {{0, 8, 1}, {1, 0, 0}}};
    for (unsigned i = 0; i < sizeof basis / sizeof *basis; i++) {
        shz_vec3_t a = zam_load_vec3(basis[i]), b = zam_load_vec3(basis[i] + 1);
        shz_vec3_t start = shz_vec3_scale(shz_vec3_sub(b, a), 2);
        shz_vec3_t move = shz_vec3_scale(a, 10);
        for (int reverse = 0; reverse < 2; reverse++) {
            NJS_POINT3 v[3] = {0};
            zam_store_vec3(v + 1 + reverse, shz_vec3_scale(a, 8));
            zam_store_vec3(v + 2 - reverse, shz_vec3_scale(b, 8));
            struct impact_triangle face;
            Float lo, hi;
            if (!impact_triangle_init(&face, v) ||
                !ground_interval(&face, start, move, 0, &lo, &hi) ||
                njAbs(lo - 0.2F) > 0.000001F || njAbs(hi - 0.8F) > 0.000001F) {
                fprintf(stderr,
                        "support interval failed on sheet %u, winding %d\n", i,
                        reverse);
                return 1;
            }
        }
    }
    /* On a steep slope the airborne foot still casts vertically. */
    const NJS_POINT3 slope[] = {{0, 0, 0}, {4, 8, 0}, {0, 0, 8}};
    struct impact_triangle face;
    Float lo, hi;
    return !impact_triangle_init(&face, slope) ||
           !ground_footprint(&face, (shz_vec3_t){.x = -2, .y = 2, .z = 1},
                             (shz_vec3_t){.x = 8}, 0, &lo, &hi) ||
           lo != 0.25F || hi != 0.6875F;
}

int main(void)
{
    int projection = overlap_planes();
    if (projection) {
        fprintf(stderr, "overlap plane check %d failed\n", projection);
        return 1;
    }
    if (crossings() || support_intervals()) {
        return 1;
    }
    const NJS_POINT3 points[] = {
        {0, 0, 0}, {1024, 1024, 0}, {1024 - 1.0F / 128, 1024 + 1.0F / 128, 0}};
    const NJS_POINT3 thin[] = {{0, 0, 0},
                               {1024, 1023.875F, 1023.75F},
                               {1023.875F, 1023.75F, 1023.625F}};
    for (int i = 0; i < 3; i++) {
        NJS_POINT3 v[3] = {points[i], points[(i + 1) % 3], points[(i + 2) % 3]};
        int failure = tip(v);
        if (failure) {
            fprintf(stderr, "tip check %d failed at vertex %d\n", failure, i);
            return 1;
        }
        for (int j = 0; j < 3; j++) {
            v[j] = thin[(i + j) % 3];
        }
        struct impact_triangle face;
        if (!impact_triangle_init(&face, v) || face.cross.x != -1.0F / 64 ||
            face.cross.y != 2.0F / 64 || face.cross.z != -1.0F / 64) {
            fprintf(stderr, "thin-face normal failed at vertex %d\n", i);
            return 1;
        }
    }
    return 0;
}
