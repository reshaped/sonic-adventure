#ifndef TESTS_DC_YAW_H
#define TESTS_DC_YAW_H

#include "system/util.h"
#include "collision/yaw.h"

/* Original collision vertices 9, 10, 11, 17, 15, 3 and 4 from the Red
 * Mountain propeller. The last strip belongs to the opposite blade. */
static NJS_POINT3 fan_points[] = {
    {90.726807F, 19.268406F, 4.626649F},
    {96.716141F, 18.347618F, -12.080084F},
    {85.753487F, 17.426834F, -28.786812F},
    {88.240143F, 22.396675F, -12.080081F},
    {-33.658443F, 22.396675F, 82.458237F},
    {-6.665115F, 19.268406F, 1.658278F},
    {-49.370228F, 19.268406F, 76.258369F},
};
static Sint16 fan_strip[] = {3, 1, 2, 3, 3, 3, 0, 1, 3, 4, 5, 6};

/* The sphere rests four units above this blade. Rotation changes its
 * projected position inside the face and preserves the contact height. */
static Bool fan_tangent(Float gap)
{
    const NJS_POINT3 v[] = {{0, 0, -16}, {0, 0, 16}, {48, 0, 0}};
    struct impact_triangle face;
    if (!impact_triangle_init(&face, v)) {
        return FALSE;
    }
    struct yaw_path path = {.origin = {16, 4, 0},
                            .turn = -4096,
                            .radius = 4,
                            .tolerance = impact_tolerance(4, (NJS_VECTOR){0})};
    path.origin.y += gap;
    yaw_prepare(&path, 1);
    struct yaw_sample start = yaw_at(&path, 0), end = yaw_at(&path, 1);
    shz_vec3_t vertex = zam_load_vec3(v);
    if (impact_outside(&face, shz_vec3_sub(start.center, vertex)) ||
        impact_outside(&face, shz_vec3_sub(end.center, vertex))) {
        return FALSE;
    }
    struct yaw_contact contact;
    int visits = 4;
    return yaw_scan(&path, &face, vertex, FALSE, start, end, 0, &visits,
                    &contact) == YAW_CLEAR &&
           visits > 0;
}

/* SET0502K entries 105 and 232 turn 700 angle units and bob by
 * 3*sin(920) in their first moving tick. Each reference time is the
 * first root of point-to-triangle distance minus Knuckles' radius. */
static int fan_geometry(void)
{
    static const struct {
        shz_vec3_t center;
        Float fall, rise, turn, hit;
        Bool both;
    } cases[] = {
        {{101.847245F, 19.479752F, -9.263280F},
         0,
         0.264269F,
         -700,
         0.063756F,
         FALSE},
        {{100.809239F, 25.347618F, -11.583552F},
         -12,
         0.264269F,
         -700,
         0.244336F,
         FALSE},
        {{99.556699F, 13.779752F, -9.054949F}, 0, 0.264269F, -700, -1, FALSE},
        {{99.556699F, 13.779752F, -9.054949F},
         0,
         0.264269F,
         -700,
         0.140686F,
         TRUE},
        {{140, 20, -9}, 0, 0.264269F, -700, -1, TRUE},
        /* A fixed blade clears the stationary body and meets the fall later. */
        {{101.847245F, 19.479752F, -9.263280F}, 0, 0, 0, -1, FALSE},
        {{100.809239F, 25.347618F, -11.583552F}, -12, 0, 0, 0.255355F, FALSE},
        /* The blade tip passes 0.01 inside and outside tangency at mid-turn. */
        {{102.733586F, 18.347618F, -9.343895F}, 0, 0, -700, 0.449849F, FALSE},
        {{102.753504F, 18.347618F, -9.345707F}, 0, 0, -700, -1, FALSE},
    };

    for (unsigned i = 0; i < sizeof cases / sizeof *cases; i++) {
        struct yaw_path path = {
            .origin = cases[i].center,
            .relative = {.y = cases[i].fall - cases[i].rise},
            .travel = {.y = cases[i].rise},
            .turn = cases[i].turn,
            .radius = 5.7F,
            .tolerance =
                impact_tolerance(5.7F, (NJS_VECTOR){0, cases[i].fall, 0}),
        };
        yaw_prepare(&path, 1);
        struct yaw_sample start = yaw_at(&path, 0), end = yaw_at(&path, 1);
        struct yaw_contact first = {.time = 2};
        for (unsigned j = 0; j < sizeof fan_strip / sizeof *fan_strip; j += 4) {
            NJS_POINT3 v[] = {fan_points[fan_strip[j + 1]],
                              fan_points[fan_strip[j + 2]],
                              fan_points[fan_strip[j + 3]]};
            struct impact_triangle face;
            if (!impact_triangle_init(&face, v)) {
                return 1;
            }
            struct yaw_contact contact;
            enum yaw_result result =
                yaw_interval(&path, &face, zam_load_vec3(v), cases[i].both,
                             start, end, &contact);
            if (result == YAW_UNCERTAIN) {
                return 2 + i;
            }
            if (result == YAW_TOUCH && contact.time < first.time) {
                first = contact;
            }
        }
        if (cases[i].hit < 0 ? first.time != 2
                             : njAbs(first.time - cases[i].hit) > 0.001F) {
            return 12 + i;
        }
        if (cases[i].hit >= 0 && (first.safe < 0 || first.safe > first.time ||
                                  first.safe > cases[i].hit + 0.0002F ||
                                  first.time < cases[i].hit - 0.0002F ||
                                  (first.time - first.safe) * path.speed_bound >
                                      path.tolerance * 1.01F)) {
            return 24 + i;
        }
        /* The blade moves toward +Z at the first contact with the resting body.
         */
        if (!i && (njAbs(first.velocity.x - 0.746F) > 0.005F ||
                   njAbs(first.velocity.y - cases[i].rise) > 0.001F ||
                   njAbs(first.velocity.z - 6.481F) > 0.005F ||
                   njAbs(first.normal.z - 0.326F) > 0.005F)) {
            return 22;
        }
    }
    Float tolerance = impact_tolerance(4, (NJS_VECTOR){0});
    return fan_tangent(0) && fan_tangent(tolerance * 0.5F) ? 0 : 23;
}

#endif
