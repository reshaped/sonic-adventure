/* Twinkle Park's act 2 mirrors, and the tasks that draw reflections in them */
#ifndef STG03_MIRROR_H
#define STG03_MIRROR_H

#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* a mirror. its plane passes through corner0, square to normal. its box has
 * corner0 and corner1 at opposite corners. a task reflects in it from the
 * side normal points to */
struct twinkle_mirror {
    NJS_POINT3 corner0;
    NJS_POINT3 corner1;
    NJS_VECTOR normal;
    /* the offset in the list from this mirror to the one across, or 0 for
     * none */
    int pair;
};

/* empty the mirror list, or copy count mirrors onto its end. each returns
 * TRUE, but twinkle_mirrors_add returns FALSE and copies none when the list
 * would hold more than five */
Bool twinkle_mirrors_clear(void);
Bool twinkle_mirrors_add(int count, struct twinkle_mirror *mirrors);
/* twinkle_reflect creates a child of task that draws task in the mirrors.
 * twinkle_reflect_player ignores task, and creates a task at level 1 that
 * draws player 0 in them. each returns the new task, or null when none can be
 * created. their steps draw only in act 2, STAGE_TWINKLE_MIRRORS, and nothing
 * draws in the set editor */
struct task *twinkle_reflect(struct task *task);
struct task *twinkle_reflect_player(struct task *task);

#endif
