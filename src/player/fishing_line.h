/* the fishing line: a rope of points from the rod's tip to the lure */
#ifndef PLAYER_FISHING_LINE_H
#define PLAYER_FISHING_LINE_H
#include <ninja.h>
#include <sg_xpt.h>

/* the points along the line, its ends among them */
#define FISHING_LINE_POINTS 12

struct fishing_line {
    NJS_POINT3 pos[FISHING_LINE_POINTS];
    NJS_VECTOR speed[FISHING_LINE_POINTS];
    NJS_VECTOR accel[FISHING_LINE_POINTS];
    /* bit i doubles point i's drag. nothing sets a bit, and the lure's line
     * starts zeroed, so no point's drag doubles */
    Uint32 double_drag;
};

/* fishing_line_step's modes, which say how it pulls each point between the
 * ends toward its neighbors. a slack line leaves a point free up to 10 from
 * a neighbor. past that it pulls the point gently, and past 20 hard. a taut
 * line always pulls hard */
enum fishing_line_mode { FISHING_LINE_SLACK, FISHING_LINE_TAUT };

/* call fishing_line_reset from ends[0] to ends[1]. nothing calls it */
void fishing_line_reset_ends(struct fishing_line *line, NJS_POINT3 *ends);
/* put line at rest with every point at from. to has no effect: the step
 * between points it sets comes to 0 */
void fishing_line_reset(struct fishing_line *line, NJS_POINT3 *from,
                        NJS_POINT3 *to);
/* move line a step. its first point goes to from and its last to to. each
 * point between is pulled toward its neighbors as mode says, falls, and
 * moves */
void fishing_line_step(struct fishing_line *line, NJS_POINT3 *from,
                       NJS_POINT3 *to, enum fishing_line_mode mode);
/* draw line as connected white segments, whose points fade with their
 * distance from the camera */
void fishing_line_draw(struct fishing_line *line);

#endif
