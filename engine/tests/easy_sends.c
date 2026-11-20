/* the easy walk's sends, engine/dc/sa1/easy.h's, on a host. a case holds
 * the vertices a meshset leaves in the test's store queue to those it
 * expects. the cases send strips, triangles and quads, whole and cut at
 * z = 1. an untextured meshset's vertuv is null, as a model's may be. the
 * build's sanitizer traps where a send takes an offset from it. the test
 * needs no disc */
#include <kamui.h>
#include <ninja.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* the walk's batch.h, as a host can read it */
#include "host/easy_batch.h"
#include "sa1/easy.h"

#define CHECK(test)                                                            \
    do {                                                                       \
        if (!(test)) {                                                         \
            fprintf(stderr, "%s:%d: %s: %s\n", __FILE__, __LINE__, running,    \
                    #test);                                                    \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

/* the word a queue holds where no send wrote */
#define UNWRITTEN 0x5a5a5a5aU
/* the vertices the queue holds, more than any meshset here sends */
#define QUEUE 32
/* the point behind the eye */
#define BEHIND 4

/* four points at z = 2 and one at z = -2, as the walk's projection leaves
 * them. an edge between the depths crosses z = 1 a quarter of the way from
 * its front. a crossing's floats are then exact */
static const struct sa1_easy_point points[] = {
    {0.0F, 0.0F, 0.5F, 0.25F},  {16.0F, 0.0F, 0.5F, 0.5F},
    {0.0F, 16.0F, 0.5F, 0.75F}, {16.0F, 16.0F, 0.5F, 1.0F},
    {12.0F, 4.0F, -0.5F, 0.0F},
};
/* a coordinate pair for each point a meshset here names */
static NJS_COLOR pairs[9];
/* the screen a cut takes a point from and back to. its center lies off the
 * origin, and its scales are distinct powers of 2. so a crossing's x and y
 * are exact, and a cut that mixes the axes fails */
static const struct sa1_live_easy state = {
    .screen = {.cx = 4.0F, .cy = 8.0F, .scale_x = 2.0F, .scale_y = 4.0F},
};

static struct sa1_easy_vertex queue[QUEUE];
/* the vertices sent so far, each the queue's next */
static int sends;
/* the case under test, for a failure's line */
static const char *running;

/* machine.h's prefetch, a PREF that sends a store queue on the SH-4 */
void _builtin_prefetch(void *p)
{
    CHECK(p == &queue[sends]);
    sends++;
}

/* the sanitizer's trap, at a send that ran C the standard leaves undefined.
 * the build's sanitizer runs an instruction the host refuses, in place of a
 * call to its runtime. the host raises SIGILL or SIGTRAP */
static void trapped(int number)
{
    (void)number;
    fprintf(stderr, "%s: %s: a send runs C the standard leaves undefined\n",
            __FILE__, running);
    _Exit(1);
}

/* a vertex a case expects. a crossing lies on the edge from point to
 * to_point, whose ends take pair and to_pair. to_point is -1 for a vertex
 * that is no crossing */
struct sent {
    Uint32 control;
    int point, pair;
    int to_point, to_pair;
};
#define POINT(control, point, pair)                                            \
    {KM_VERTEXPARAM_##control, point, pair, -1, -1}
#define CROSSING(control, point, pair, to_point, to_pair)                      \
    {KM_VERTEXPARAM_##control, point, pair, to_point, to_pair}

static Uint32 word(Float value)
{
    Uint32 bits;

    memcpy(&bits, &value, sizeof bits);
    return bits;
}

/* a point's value is a. a crossing's is the value at z = 1 between a and b */
static Float crossed(const struct sent *want, Float a, Float b)
{
    Float t = want->point == BEHIND ? 0.75F : 0.25F;

    return want->to_point < 0 ? a : a + ((b - a) * t);
}

/* where want's edge crosses z = 1 on the screen, along the axis of center
 * and scale. a and b are its ends' places on that axis. the crossing is
 * found in view space, as easy_clip finds it */
static Float crossed_on(const struct sent *want, Float a, Float a_reciprocal_z,
                        Float b, Float b_reciprocal_z, Float center,
                        Float scale)
{
    Float view_a = (a - center) / (a_reciprocal_z * scale);
    Float view_b = (b - center) / (b_reciprocal_z * scale);

    return (crossed(want, view_a, view_b) * scale) + center;
}

/* send meshes as a meshset of type through the walk, near and textured as
 * asked. hold the queue to want's count vertices and nothing past them */
static void sends_of(const char *name, Uint16 type, const Sint16 *meshes,
                     Uint16 polygons, Bool near, Bool textured,
                     const struct sent *want, int count)
{
    NJS_MESHSET meshset = {.type_matId = type,
                           .nbMesh = polygons,
                           .meshes = (Sint16 *)meshes,
                           .vertuv = textured ? pairs : NULL};
    struct sa1_easy_vertex *end;
    int i;

    running = name;
    sends = 0;
    memset(queue, (int)(UNWRITTEN & 0xff), sizeof queue);
    if (textured) {
        end = near ? easy_textured_near(queue, &meshset, points, &state)
                   : easy_textured(queue, &meshset, points, &state);
    } else {
        end = near ? easy_untextured_near(queue, &meshset, points, &state)
                   : easy_untextured(queue, &meshset, points, &state);
    }
    CHECK(end == &queue[count]);
    CHECK(sends == count);
    for (i = 0; i < count; i++) {
        const struct sa1_easy_vertex *got = &queue[i];
        const struct sa1_easy_point *a = &points[want[i].point];
        const struct sa1_easy_point *b =
            &points[want[i].to_point < 0 ? want[i].point : want[i].to_point];
        const NJS_TEX *pair_a = &pairs[want[i].pair].tex;
        const NJS_TEX *pair_b =
            &pairs[want[i].to_pair < 0 ? want[i].pair : want[i].to_pair].tex;

        CHECK(got->control == want[i].control);
        if (want[i].to_point < 0) {
            CHECK(got->x == a->x && got->y == a->y);
            CHECK(got->reciprocal_z == a->reciprocal_z);
        } else {
            CHECK(got->x == crossed_on(&want[i], a->x, a->reciprocal_z, b->x,
                                       b->reciprocal_z, state.screen.cx,
                                       state.screen.scale_x));
            CHECK(got->y == crossed_on(&want[i], a->y, a->reciprocal_z, b->y,
                                       b->reciprocal_z, state.screen.cy,
                                       state.screen.scale_y));
            CHECK(got->reciprocal_z == 1.0F);
        }
        CHECK(got->intensity == crossed(&want[i], a->intensity, b->intensity));
        if (textured) {
            CHECK(got->u ==
                  crossed(&want[i], pair_a->u, pair_b->u) * EASY_SCALE);
            CHECK(got->v ==
                  crossed(&want[i], pair_a->v, pair_b->v) * EASY_SCALE);
        } else {
            CHECK(word(got->u) == UNWRITTEN && word(got->v) == UNWRITTEN);
        }
        CHECK(got->unwritten == UNWRITTEN);
    }
    CHECK(queue[count].control == UNWRITTEN);
}

/* a case with and without texture coordinates */
#define SENDS(name, type, meshes, polygons, near, ...)                         \
    do {                                                                       \
        static const struct sent want[] = {__VA_ARGS__};                       \
        int count = (int)(sizeof want / sizeof want[0]);                       \
                                                                               \
        sends_of(name ", untextured", type, meshes, polygons, near, FALSE,     \
                 want, count);                                                 \
        sends_of(name ", textured", type, meshes, polygons, near, TRUE, want,  \
                 count);                                                       \
    } while (0)

int main(void)
{
    /* the second strip's count carries NJD_TRIMESH_END. triangles and quads
     * each hold a polygon the walk culls, then one it sends. each near case
     * puts the point behind the eye in its first strip or polygon */
    static const Sint16 strips[] = {3, 0, 1, 2, (Sint16)(NJD_TRIMESH_END | 4),
                                    2, 1, 3, 0};
    static const Sint16 triangles[] = {0, 1, 2, 2, 1, 0};
    static const Sint16 quads[] = {0, 1, 2, 3, 2, 1, 0, 3};
    static const Sint16 near_strips[] = {5, 0, 1, 2, BEHIND, 3, 4, 2, 1, 0, 3};
    static const Sint16 near_triangles[] = {1, 2, BEHIND, 2, 1, 0};
    static const Sint16 near_quads[] = {0, 1, 2, BEHIND, 2, 1, 0, 3};
    int i;

    signal(SIGILL, trapped);
    signal(SIGTRAP, trapped);
    for (i = 0; i < (int)(sizeof pairs / sizeof pairs[0]); i++) {
        pairs[i].tex.u = (Sint16)(16 * i);
        pairs[i].tex.v = (Sint16)(256 - (32 * i));
    }
    /* each strip whole, each point with the pair at its place in the
     * meshset */
    SENDS("strips", NJD_MESHSET_TRIMESH, strips, 2, FALSE, POINT(NORMAL, 0, 0),
          POINT(NORMAL, 1, 1), POINT(ENDOFSTRIP, 2, 2), POINT(NORMAL, 2, 3),
          POINT(NORMAL, 1, 4), POINT(NORMAL, 3, 5), POINT(ENDOFSTRIP, 0, 6));
    /* the polygon whose first three points' area is negative, with its own
     * pairs, past the culled one's */
    SENDS("triangles", NJD_MESHSET_3, triangles, 2, FALSE, POINT(NORMAL, 2, 3),
          POINT(NORMAL, 1, 4), POINT(ENDOFSTRIP, 0, 5));
    SENDS("quads", NJD_MESHSET_4, quads, 2, FALSE, POINT(NORMAL, 2, 4),
          POINT(NORMAL, 1, 5), POINT(NORMAL, 0, 6), POINT(ENDOFSTRIP, 3, 7));
    /* the first strip sends its whole triangle, then cuts its two with the
     * point behind. each cut sends four corners, the fourth before the
     * third. the second strip goes whole, with the pairs past the first's */
    SENDS("near strips", NJD_MESHSET_TRIMESH, near_strips, 2, TRUE,
          POINT(NORMAL, 0, 0), POINT(NORMAL, 1, 1), POINT(ENDOFSTRIP, 2, 2),
          CROSSING(NORMAL, BEHIND, 3, 1, 1), POINT(NORMAL, 1, 1),
          CROSSING(NORMAL, 2, 2, BEHIND, 3), POINT(ENDOFSTRIP, 2, 2),
          POINT(NORMAL, 2, 2), CROSSING(NORMAL, 2, 2, BEHIND, 3),
          POINT(NORMAL, 3, 4), CROSSING(ENDOFSTRIP, BEHIND, 3, 3, 4),
          POINT(NORMAL, 2, 5), POINT(NORMAL, 1, 6), POINT(NORMAL, 0, 7),
          POINT(ENDOFSTRIP, 3, 8));
    /* a triangle with the point behind, cut, then one whole */
    SENDS("near triangles", NJD_MESHSET_3, near_triangles, 2, TRUE,
          CROSSING(NORMAL, BEHIND, 2, 1, 0), POINT(NORMAL, 1, 0),
          CROSSING(NORMAL, 2, 1, BEHIND, 2), POINT(ENDOFSTRIP, 2, 1),
          POINT(NORMAL, 2, 3), POINT(NORMAL, 1, 4), POINT(ENDOFSTRIP, 0, 5));
    /* a quad with the point behind, then one whole. the clip sends the
     * first's points 0 to 2 whole and unculled, and 1 to 3 cut */
    SENDS("near quads", NJD_MESHSET_4, near_quads, 2, TRUE, POINT(NORMAL, 0, 0),
          POINT(NORMAL, 1, 1), POINT(ENDOFSTRIP, 2, 2),
          CROSSING(NORMAL, BEHIND, 3, 1, 1), POINT(NORMAL, 1, 1),
          CROSSING(NORMAL, 2, 2, BEHIND, 3), POINT(ENDOFSTRIP, 2, 2),
          POINT(NORMAL, 2, 4), POINT(NORMAL, 1, 5), POINT(NORMAL, 0, 6),
          POINT(ENDOFSTRIP, 3, 7));
    return 0;
}
