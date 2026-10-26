/* the race course's tables: each kind's start positions and distance, the
 * course's sections, and the points the racers and the camera head for.
 * race_origin follows them */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_race/course.h"

/* each kind's start position for each lane */
static const NJS_POINT3 course_starts[5][8] = {
#include "al_race/course/course_starts.inc"
};
/* each kind's distance to the finish */
static const Float course_distances[5] = {
#include "al_race/course/course_distances.inc"
};
static const struct race_segment course_segments[17] = {
#include "al_race/course/course_segments.inc"
};
static const NJS_POINT3 course_targets[20] = {
#include "al_race/course/course_targets.inc"
};
const struct race_course race_course = {
#include "al_race/course/race_course.inc"
};
const NJS_POINT3 race_origin = {
#include "al_race/course/race_origin.inc"
};
/* 36 bytes after race_origin, whose address no word in this image,
 * AL_MAIN.BIN or 1ST_READ.BIN holds. as Floats, they are a 3 by 3 identity
 * matrix */
/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const Uint8 dat_0cb9a6e4[36] = {
#include "al_race/course/dat_0cb9a6e4.inc"
};
