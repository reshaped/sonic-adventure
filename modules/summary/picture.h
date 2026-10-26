/* the summary's pictures: one for each story, whose eight textures make up
 * the screen */
#ifndef SUMMARY_PICTURE_H
#define SUMMARY_PICTURE_H

#include <ninja.h>
#include <sg_xpt.h>

#include "draw/screen.h"
#include "system/task.h"

/* a struct screen_parts with room for a picture's eight textures */
struct picture_parts {
    Sint16 count;
    struct screen_part parts[8];
};

/* load story's picture, story 0 to 6, as the one picture_draw draws, and
 * return its texlist. where story has no file or texlist, return null and
 * leave none loaded */
NJS_TEXLIST *picture_load(int story);
/* release the loaded picture's textures, if one is loaded */
void picture_release(void);
/* draw the loaded picture in its story's color, with its top left at x and
 * y, at depth z. a picture must be loaded */
void picture_draw(Float x, Float y, Float z);
/* create a task in level that draws story's picture untinted each step, at
 * its work's pos, which starts at (0, 0) at depth 65536. it loads the
 * picture's textures now, and the task releases them as it ends. return the
 * task, or null when task_create fails */
struct task *picture_create(int level, int story);

#endif
