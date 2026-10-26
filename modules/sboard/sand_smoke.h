/* sand smoke, which pillars and sand worms raise */
#ifndef SBOARD_SAND_SMOKE_H
#define SBOARD_SAND_SMOKE_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"
#include "system/task_desc.h"

/* where the smoke's sprites stand, and the scale they draw at. the points
 * lie in the frame the smoke's pos and ang place */
struct sand_smoke_shape {
    const NJS_POINT3 *points;
    int count;
    Float size;
};

/* what sand_smoke_desc's init takes as arg. the smoke copies *pos and
 * *ang, and keeps shape and done, which must outlive it */
struct sand_smoke_arg {
    const NJS_POINT3 *pos;
    const struct angles *ang;
    const struct sand_smoke_shape *shape;
    Sint8 *done; /* the maker sets it TRUE to fade the smoke out */
};

/* the smoke's kind, which its maker creates as a child. the smoke fades
 * in, fades out once *done is TRUE, and then destroys itself */
extern const struct task_desc sand_smoke_desc;

#endif
