/* a bitmap test: a window of one word of text, in a box shaded in two blues.
 * nothing in ADVERTISE creates one */
#ifndef ADVERTISE_BITMAP_TEST_H
#define ADVERTISE_BITMAP_TEST_H

#include <sg_xpt.h>

#include "system/task.h"

struct bitmap_test_spec {
    int x, y;
    int width, height;
    int index; /* the texture's global index */
    int mode;
    int unk_18;
};

/* the sum of values[0] to values[3], each converted to Uint32 first.
 * nothing in ADVERTISE calls it */
Uint32 sum_unsigned(Float *values);
/* start a test window's task at level 1, as spec places and sizes it, and
 * return it. spec gives its texture and mode too. its bitmap and the
 * bitmap's pixels come from the heap. in mode 1 the step draws the window.
 * in mode 0, its first run makes a second bitmap in place of the first,
 * which it leaks, and goes on to mode 1. the second bitmap is 512 by 128,
 * with its top left at x 32 and y 240. nothing in ADVERTISE calls it */
struct task *bitmap_test_create(struct bitmap_test_spec *spec);

#endif
