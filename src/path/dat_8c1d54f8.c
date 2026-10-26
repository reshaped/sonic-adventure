/* a word of D, between save.c's data and hud.c's, that this build keeps
 * here: path_draw_step's address, which no other word in this image or
 * another holds. nothing in this image or another names the word, and its
 * declaration and owner are unknown */
#include "path/steps.h"
#include "system/task.h"

static void (*dat_8c1d54f8)(struct task *task) = path_draw_step;
