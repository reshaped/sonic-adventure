/* the race course's tables, its origin and its paths */
#ifndef AL_RACE_COURSE_H
#define AL_RACE_COURSE_H
#include <ninja.h>

struct path;

/* a section of the course */
struct race_segment {
    Float distance; /* where the section ends, along the course */
    NJS_POINT3 camera; /* the camera's point for a racer in the section */
};

/* the course's tables, which the racers and the camera read */
struct race_course {
    int kind_count; /* the count of kinds in starts and distances, CHAO_RACES */
    NJS_POINT3 (*starts)[8]; /* each kind's start position for each lane */
    Float *distances; /* each kind's distance to the finish */
    struct race_segment *segments; /* the course's 17 sections, in order */
    /* 20 points, by index. points 0 to 7 are each lane's goal, and points 8
     * to 15 each place's spot at the commendation, where the racers face
     * point 16. the camera's finish eases from point 16 to point 17. points
     * 18 and 19 are the camera's for racers on the first two of race_paths */
    NJS_POINT3 *targets;
};

extern const struct race_course race_course;
/* the point (0, 0, 0). race_chao.c moves it by a node's posed matrix to find
 * the node's position */
extern const NJS_POINT3 race_origin;
/* the race's three paths, of which the last is the course's */
extern struct path *race_paths[];

#endif
