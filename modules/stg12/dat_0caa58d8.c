/* 8 bytes of B, between cargo_train.c's and camera_sweep.c's: a word nothing
 * in this image or 1ST_READ.BIN names, then e105_drawing_task, which e105.c
 * reads and sets through its extern. which object holds them is unresolved:
 * e105.c's own B, which it doesn't place, cargo_train.c's, were its B to run
 * on past cargo_train_speed, that of one of the 15 other objects linked
 * between with no B, cargo_ride.c to switch.c, camera_sweep.c's, were its B
 * to start 8 bytes early, or one of bss alone, as this build places them */
#include <sg_xpt.h>

#include "system/task.h"

static Uint8 dat_0caa58d8[4];
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
struct task *e105_drawing_task;
