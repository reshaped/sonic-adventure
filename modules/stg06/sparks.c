/* the sparks a blast throws, in batches that one task moves and draws */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "stg06/sparks.h"
#include "system/task.h"
#include "system/util.h"

#define SPARK_LIFE 250

/* each step scales a spark's speed by SPARK_DRAG, then adds SPARK_GRAVITY to
 * its y */
#define SPARK_DRAG 0.97F
#define SPARK_GRAVITY (-0.4F)

/* a spark that crosses its batch's floor with a y speed over SPARK_BOUNCE
 * either way bounces there. the bounce reverses that speed and keeps
 * SPARK_BOUNCE_KEEP of it */
#define SPARK_BOUNCE 4.0F
#define SPARK_BOUNCE_KEEP 0.6F

/* each step sets a spark's head SPARK_LENGTH ahead of it, along its speed.
 * at a speed of SPARK_SLOW or less, the head lies the speed squared ahead
 * instead */
#define SPARK_LENGTH 1.3F
#define SPARK_SLOW 0.1F

/* the bytes a batch's arrays allot each spark. a spark uses 24 of
 * POINTS_ROOM, 12 of SPEEDS_ROOM and 8 of COLORS_ROOM */
#define POINTS_ROOM 32
#define SPEEDS_ROOM 16
#define COLORS_ROOM 16

/* a batch, in the list spark_batches heads. the batch and its three arrays
 * are heap blocks, which spark_batch_free gives back */
struct spark_batch {
    /* two to a spark: where it is, then a point along its speed */
    NJS_POINT3 *points;
    NJS_COLOR *colors; /* a color for each point */
    NJS_VECTOR *speeds; /* each spark's speed, in units a step */
    Float floor; /* the height its sparks bounce at */
    int count; /* how many sparks */
    /* the steps left. the step that finds it 0 frees the batch */
    int life;
    struct spark_batch *prev;
    struct spark_batch *next;
};

/* the one task that moves and draws every batch, or null */
static struct task *sparks_task = NULL;
static struct spark_batch *spark_batches = NULL; /* newest first */

static void sparks_destroy(struct task *task);
static void sparks_step(struct task *task);

static void sparks_start(struct task *task)
{
    task->step = sparks_step;
    task->destructor = sparks_destroy;
}

static void spark_batch_free(struct spark_batch *batch)
{
    if (batch->prev == NULL) {
        spark_batches = batch->next;
    } else {
        batch->prev->next = batch->next;
    }
    if (batch->next != NULL) {
        batch->next->prev = batch->prev;
    }
    heap_free(batch->colors);
    heap_free(batch->points);
    heap_free(batch->speeds);
    heap_free(batch);
}

static void sparks_destroy(struct task *task)
{
    struct spark_batch *batch = spark_batches;
    struct spark_batch *next;

    while (batch) {
        next = batch->next;
        spark_batch_free(batch);
        batch = next;
    }
    if (sparks_task == task) {
        sparks_task = NULL;
    }
}

/* draw each batch, then move its sparks a step. the task kills itself on a
 * step that finds no batch */
static void sparks_step(struct task *task)
{
    struct spark_batch *batch;
    struct spark_batch *next;
    NJS_POINT3COL lines;
    NJS_POINT3 *point;
    Float *walk;
    char *bytes;
    NJS_VECTOR *speed;
    NJS_VECTOR *out_speed;
    int batches = 0;
    int i;
    int count;
    Float x;
    Float y;
    Float z;
    Float dx;
    Float dy;
    Float dz;
    Float length;
    Float end_x;
    Float end_y;
    Float end_z;
    Float floor;

    /* @hack spelling: batch loaded before tex is cleared, which puts the clear
     * in the delay slot of the BF/S at 0x0c915a42; with tex cleared first, 4
     * of 222 words differ */
    batch = spark_batches;
    lines.tex = NULL;
    for (; batch; batch = next) {
        count = batch->count;
        floor = batch->floor;
        batches++;
        next = batch->next;
        lines.p = batch->points;
        lines.col = batch->colors;
        lines.num = count + count;
        njDrawLine3D(&lines, count, 0);
        point = batch->points;
        out_speed = batch->speeds;
        /* @hack spelling: i cleared before speed is loaded, which puts speed's
         * MOV R4,R11 in the delay slot of the BT/S at 0x0c915a80; with
         * i = 0 in the for, 2 of 222 words differ */
        i = 0;
        speed = batch->speeds;
        for (; i < count; i++) {
            /* @hack spelling: the point read as an array of Floats, through
             * VECTOR_FLOATS, which keeps the spilled coordinates in the
             * image's frame slots; as a byte cursor over the point, 14 of 222
             * words differ, and through the members, 56 */
            walk = VECTOR_FLOATS(&batch->points[i * 2]);
            x = *walk++;
            y = *walk++;
            z = *walk;
            /* @hack spelling: speed read a Float at a time through a byte
             * cursor stepping by sums, as the image reads it; through the
             * members, 72 of 222 words differ */
            bytes = (char *)speed;
            dx = *(Float *)bytes;
            bytes = bytes + sizeof(Float);
            dy = *(Float *)bytes;
            bytes = bytes + sizeof(Float);
            dz = *(Float *)bytes;
            dx *= SPARK_DRAG;
            dy *= SPARK_DRAG;
            dz *= SPARK_DRAG;
            dy += SPARK_GRAVITY;
            if (((y + dy >= floor && y <= floor) ||
                 (y + dy <= floor && y >= floor)) &&
                njAbs(dy) > SPARK_BOUNCE) {
                dy = -(dy * SPARK_BOUNCE_KEEP);
                x = x + dx;
                /* just past floor, on the side the bounce heads for */
                y = floor + (dy * 0.0001F);
                z = z + dz;
            } else {
                x = x + dx;
                y = y + dy;
                z = z + dz;
            }
            length = njSqrt((dx * dx) + (dy * dy) + (dz * dz));
            if (length > SPARK_SLOW) {
                length = SPARK_LENGTH / length;
            }
            end_x = x + (dx * length);
            end_y = y + (dy * length);
            end_z = z + (dz * length);
            /* @hack spelling: the line's two ends and the speed stored a Float
             * at a time through a byte cursor, as the image writes them;
             * through the members, 73 of 222 words differ */
            bytes = (char *)point;
            *(Float *)bytes = x;
            bytes += sizeof(Float);
            *(Float *)bytes = y;
            bytes += sizeof(Float);
            *(Float *)bytes = z;
            bytes += sizeof(Float);
            *(Float *)bytes = end_x;
            bytes += sizeof(Float);
            *(Float *)bytes = end_y;
            bytes += sizeof(Float);
            *(Float *)bytes = end_z;
            bytes = (char *)out_speed;
            *(Float *)bytes = dx;
            bytes += sizeof(Float);
            *(Float *)bytes = dy;
            bytes += sizeof(Float);
            *(Float *)bytes = dz;
            /* @hack spelling: the steps written out as sums, which give point
             * R10, out_speed R12 and speed R11 (0x0c915bde to 0x0c915be4); as
             * point += 2, out_speed++ and speed++, 13 of 222 words differ */
            point = point + 2;
            out_speed = out_speed + 1;
            speed = speed + 1;
        }
        if (batch->life-- == 0) {
            spark_batch_free(batch);
        }
    }
    if (!batches) {
        task_kill(task);
    }
}

Bool sky_deck_sparks_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float spread,
                            Float floor, int count)
{
    struct spark_batch *batch;
    NJS_VECTOR spark_speed;
    int i;
    int at;
    int ahead;

    if (sparks_task == NULL) {
        sparks_task = task_create(0, 3, sparks_start);
        if (sparks_task == NULL) {
            return FALSE;
        }
    }
    batch = heap_alloc(sizeof(*batch));
    if (batch != NULL) {
        batch->points = heap_alloc(count * POINTS_ROOM);
        batch->speeds = heap_alloc(count * SPEEDS_ROOM);
        batch->colors = heap_alloc(count * COLORS_ROOM);
        batch->floor = floor;
        batch->count = count;
        batch->life = SPARK_LIFE;
        for (i = 0; i < count; i++) {
            /* @hack spelling: at, the tail's index, written i + i, which keeps
             * the 28-byte frame (ADD #-28,R15 at 0x0c915c78); as i * 2, the
             * frame takes 40 bytes and 226 of 266 words differ */
            at = i + i;
            ahead = at + 1;
            spark_speed.x =
                speed->x + (spread * ((rand() * RAND_SCALE) - 0.5F) * 2.0F);
            spark_speed.y =
                speed->y + (spread * ((rand() * RAND_SCALE) - 0.5F) * 2.0F);
            spark_speed.z =
                speed->z + (spread * ((rand() * RAND_SCALE) - 0.5F) * 2.0F);
            batch->points[at].x = pos->x;
            batch->points[at].y = pos->y;
            batch->points[at].z = pos->z;
            batch->points[ahead].x = pos->x + spark_speed.x;
            batch->points[ahead].y = pos->y + spark_speed.y;
            batch->points[ahead].z = pos->z + spark_speed.z;
            batch->speeds[i] = spark_speed;
            batch->colors[at].argb.a = 255;
            batch->colors[at].argb.r = 239;
            batch->colors[at].argb.g = 223;
            batch->colors[at].argb.b = 223;
            batch->colors[ahead].argb.a = 255;
            batch->colors[ahead].argb.r = 223;
            batch->colors[ahead].argb.g = 223;
            batch->colors[ahead].argb.b = 223;
        }
        if (spark_batches == NULL) {
            spark_batches = batch;
            batch->prev = NULL;
            batch->next = NULL;
        } else {
            batch->prev = NULL;
            batch->next = spark_batches;
            spark_batches->prev = batch;
            spark_batches = batch;
        }
        return TRUE;
    }
    return FALSE;
}
