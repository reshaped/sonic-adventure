/* sparks: short streaks of light that fly, slow and fall */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/model.h"
#include "effect/spark.h"
#include "system/task.h"

/* the last of the sixteen steps a spark lives */
#define SPARK_LAST 15
/* the radius of the sphere about a spark that must be in view for the
 * spark to move and draw */
#define SPARK_VIEW_RADIUS 5.0F
/* half the length of the line a spark draws along its speed */
#define SPARK_HALF_LENGTH 0.3F

static void spark_step(struct task *task);

/* the line's colors: orange at its head, then magenta at its tail */
static NJS_COLOR spark_colors[2] = {
#include "effect/spark/spark_colors.inc"
};
static NJS_POINT3 spark_ends[2];
static NJS_POINT3COL spark_line = {
#include "effect/spark/spark_line.inc"
};

void spark_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float unused)
{
    struct task *task;
    struct task_work *work;

    task = task_create(TASK_WORK, 6, spark_step);
    if (task != NULL) {
        work = task->work;
        work->timer = 0;
        work->pos = *pos;
        work->scl = *speed;
    }
}

/* a spark keeps its speed in work->scl. it moves only on a frame's first
 * step, while a sphere of SPARK_VIEW_RADIUS about it is in view. there it
 * keeps 0.975 of its speed, gains 0.085 downward and moves. it then draws
 * as a line along its speed, centered on pos. its timer counts every step,
 * moving or not */
static void spark_step(struct task *task)
{
    struct task_work *work;
    Float *speed;
    Float *pos;
    Float *end;
    Float x;
    Float y;
    Float z;
    Float dx;
    Float dy;
    Float dz;
    Float squared;
    Float scale;

    work = task->work;
    if (TASK_TIMER(work)++ > SPARK_LAST) {
        task_kill(task);
        return;
    }
    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                     SPARK_VIEW_RADIUS)) {
        /* @hack spelling: each vector through a pointer, read forward and
         * written back backward, which keeps the walk as @R4+ loads and @-R4
         * stores (0x8c0495dc on); through the members, SHC indexes them through
         * R0, the function runs 8 bytes long and 93 of 122 words differ */
        speed = &work->scl.x;
        x = *speed++;
        y = *speed++;
        z = *speed++;
        x *= 0.975F;
        y *= 0.975F;
        z *= 0.975F;
        y += -0.085F;
        *--speed = z;
        *--speed = y;
        *--speed = x;
        pos = &work->pos.x;
        /* @hack spelling: each walk of pos in a scope with locals of its own,
         * which keeps py in FR8 and pz in FR7 (0x8c0495fc, 0x8c0495fe);
         * shared, the two trade and 20 of 122 words differ */
        {
            Float px;
            Float py;
            Float pz;

            px = *pos++;
            py = *pos++;
            pz = *pos++;
            px += x;
            py += y;
            pz += z;
            *--pos = pz;
            *--pos = py;
            *--pos = px;
        }
        speed = &work->scl.x;
        dx = *speed++;
        dy = *speed++;
        dz = *speed;
        squared = (dx * dx) + (dy * dy) + (dz * dz);
        scale = _builtin_fsrra(squared);
        dx *= scale;
        dy *= scale;
        dz *= scale;
        dx *= SPARK_HALF_LENGTH;
        dy *= SPARK_HALF_LENGTH;
        dz *= SPARK_HALF_LENGTH;
        pos = &work->pos.x;
        end = &spark_ends[0].z + 1;
        {
            Float px;
            Float py;
            Float pz;

            px = *pos++;
            py = *pos++;
            pz = *pos++;
            px += dx;
            py += dy;
            pz += dz;
            pos -= 3;
            *--end = pz;
            *--end = py;
            *--end = px;
        }
        end = &spark_ends[1].z + 1;
        {
            Float px;
            Float py;
            Float pz;

            px = *pos++;
            py = *pos++;
            pz = *pos;
            pz -= dz;
            py -= dy;
            px -= dx;
            *--end = pz;
            *--end = py;
            *--end = px;
        }
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njDrawLine3D(&spark_line, 1, NJD_DRAW_NORMAL);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
    }
}
