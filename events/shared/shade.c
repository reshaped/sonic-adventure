/* an event's shade: a filled polygon of one color over the screen. its
 * alpha eases toward a target each step */
#include <ninja.h>
#include <sg_xpt.h>

#include "shared/shade.h"
#include "system/task.h"

/* the shade's record, which lies in its task's work from vars on */
struct shade {
    NJS_POINT2 *points;
    Sint32 count;
    Float depth;
    NJS_COLOR color;
    Sint32 ease; /* while positive, alpha closes 1/ease of its gap a step */
    Uint8 alpha; /* the alpha it eases toward */
};

/* the shade's task_work, as the shade lays it out. the shade never uses
 * the fields before vars */
struct shade_work {
    Uint8 unk_00[8];
    struct shade shade;
};

#define SHADE(task) (&((struct shade_work *)(task)->work)->shade)

static void event_shade_destroy(struct task *task)
{
}

/* draw the polygon with every point in the shade's color. the list of
 * colors comes from the heap for the draw */
static void event_shade_draw(struct task *task)
{
    /* @hack redundant: register on shade, which keeps it in R14 and count
     * in R13 (MOV.L @(32,R4),R14 at 0x0cb891ae in EV0008); without it, the
     * two trade and 10 of 46 words differ */
    register struct shade *shade = SHADE(task);
    NJS_COLOR color;
    NJS_POINT2COL polygon;
    NJS_COLOR *colors;
    int count = shade->count;
    int i;

    /* @hack redundant: a goto to the next statement, which loads the color into
     * R2, not R3, and stores the points ahead of heap_alloc's address
     * (MOV.L @(12,R14),R2 at 0x0cb891b6 in EV0008); without it, 5 of 46
     * words differ */
    goto copy;
copy:
    color = shade->color;
    polygon.p = shade->points;
    polygon.num = count;
    polygon.col = heap_alloc(count * sizeof(NJS_COLOR));
    colors = polygon.col;
    for (i = count; i != 0; i--) {
        *colors++ = color;
    }
    njDrawPolygon2D(&polygon, count, shade->depth, NJD_FILL | NJD_TRANSPARENT);
    heap_free(polygon.col);
}

/* while ease is positive, close 1/ease of the alpha's gap to the target.
 * then draw */
static void event_shade_step(struct task *task)
{
    /* @hack spelling: the task through a copy, self, which SHC makes in R5
     * (MOV R4,R5 at 0x0cb89200 in EV0008); without it, the function runs 2
     * bytes short and 25 of 25 words differ */
    struct task *self = task;
    struct shade *shade = SHADE(self);
    int ease;

    /* @hack redundant: a never-run draw, which keeps the record in R14, a
     * saved register (MOV.L R14,@-R15 and MOV.L @(32,R5),R14 at 0x0cb89202
     * and 0x0cb89206 in EV0008); without it, the function saves no
     * register, runs 2 bytes short and 14 of 25 words differ */
    if (0) {
        event_shade_draw(task);
        task = NULL;
    }
    ease = shade->ease;
    if (ease > 0) {
        shade->color.argb.a =
            (((ease - 1) * shade->color.argb.a) + shade->alpha) / ease;
    }
    event_shade_draw(task);
}

struct task *event_shade_create(Uint32 color, NJS_POINT2 *points, int count)
{
    struct task *task = task_create(TASK_WORK, 5, NULL);
    struct shade *shade;
    /* @hack spelling: the record through a copy, from, which SHC keeps in
     * R1 (MOV.L @(32,R14),R1 at 0x0cb8925c in EV0008); without it, R4
     * holds it and 8 of 42 words differ */
    struct shade *from;
    /* @hack spelling: the task through a copy, made, which SHC keeps in
     * R14, a saved register (MOV R0,R14 at 0x0cb89248 in EV0008); without
     * it, the task stays in R4, the function runs 4 bytes short and 42 of
     * 42 words differ */
    struct task *made = task;

    if (task != NULL) {
        task->step = event_shade_step;
        task->draw = event_shade_draw;
        task->destructor = event_shade_destroy;
        from = SHADE(task);
        shade = from;
        /* @hack redundant: a goto to the next statement, which copies the
         * points through R2 and loads ease's 0 before the count (MOV.L
         * @(8,R15),R2 at 0x0cb8925e in EV0008); without it, 5 of 42 words
         * differ */
        goto set;
    set:
        shade->points = points;
        shade->count = count;
        shade->depth = EVENT_SHADE_DEPTH;
        shade->color.color = color;
        shade->ease = 0;
        /* the target takes color's low byte, its blue. ease is 0, so no
         * step eases toward it before event_shade_fade replaces it */
        shade->alpha = color;
    }
    return made;
}

void event_shade_fade(struct task *task, int alpha, int ease)
{
    /* @hack spelling: the record through a copy, from, which SHC keeps in
     * R1 (MOV.L @(32,R4),R1 at 0x0cb89286 in EV0008); without it, R4 holds
     * it and 5 of 11 words differ */
    struct shade *from = SHADE(task);
    struct shade *shade = from;

    shade->alpha = alpha;
    shade->ease = ease;
    if (ease <= 0) {
        shade->color.argb.a = alpha;
    }
}
