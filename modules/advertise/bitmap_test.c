/* a bitmap test: a window of one word of text, in a box shaded in two blues.
 * nothing in ADVERTISE creates one */
#include <sg_xpt.h>

#include "advertise/bitmap_test.h"
#include "draw/screen.h"
#include "system/bitmap.h"
#include "system/task.h"

#define TEXT_1_LENGTH 6
#include "advertise/bitmap_test/text.inc"

static void bitmap_test_step(struct task *task);
static void bitmap_test_destroy(struct task *task);

static Uint32 test_box_colors[4] = {
    0x80117bff,
    0x80002e67,
    0x80117bff,
    0x80002e67,
};

Uint32 sum_unsigned(Float *values)
{
    Uint32 first = *values++;
    Uint32 second = *values++;
    Uint32 third = *values++;
    Uint32 fourth = *values;
    Uint32 sum = first + second + third + fourth;

    return sum;
}

struct task *bitmap_test_create(struct bitmap_test_spec *spec)
{
    struct task *task = task_create(TASK_WORK, 1, bitmap_test_step);
    struct task_work *work;
    struct bitmap *bitmap;

    bitmap = task->extra = heap_calloc(1, sizeof(struct bitmap));
    bitmap_alloc(bitmap, spec->x, spec->y, spec->width, spec->height,
                 spec->index);
    work = task->work;
    work->mode = spec->mode;
    work->vars.l[2] = spec->unk_18;
    task->destructor = bitmap_test_destroy;
    return task;
}

static void bitmap_test_step(struct task *task)
{
    struct task_work *work = task->work;
    struct bitmap *bitmap = task->extra;

    switch (work->mode) {
    case 0:
        bitmap = heap_calloc(1, sizeof(struct bitmap));
        /* @bug memory: the first bitmap leaks, with its pixels, as the second
         * takes its place in task->extra (MOV.L R0,@(44,R13) at 0x8c900314).
         * no code calls bitmap_test_create, so this never runs */
        task->extra = bitmap;
        bitmap_alloc(bitmap, 32, 240, 512, 128, 0x100000);
        task->destructor = bitmap_test_destroy;
        work->mode = 1;
        break;
    case 1:
        bitmap_clear(bitmap);
        bitmap_print(bitmap, TEXT_1);
        bitmap_queue(bitmap);
        bitmap_draw(bitmap);
        screen_quad_colors(test_box_colors[0], test_box_colors[1],
                           test_box_colors[2], test_box_colors[3]);
        screen_box_draw(bitmap->x - 16, bitmap->y - 16, 1.1F,
                        bitmap->width + 32, bitmap->height + 32);
        break;
    }
}

static void bitmap_test_destroy(struct task *task)
{
    struct bitmap *bitmap = task->extra;

    if (bitmap) {
        bitmap_free(bitmap);
        heap_free(bitmap);
    }
}
