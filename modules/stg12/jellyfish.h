/* O KURAGE: jellyfish that drift about a tank and flee player 0 */
#ifndef STG12_JELLYFISH_H
#define STG12_JELLYFISH_H

#include <ninja.h>

#include "system/task.h"

/* the jellyfish's one texture. shelter_pvms names it with no pvm, so
 * texture_load_stage loads it by its own name */
extern NJS_TEXLIST shelter_jellyfish_texlist;

/* the object list's step. task needs TASK_WORK's record. ang.z's low byte
 * is the count of jellyfish, or 1 in the set editor. scl is the tank's
 * half-size along each axis, and ang.y turns the tank about y. the step
 * allocates 36 bytes for each jellyfish from the heap, unchecked.
 * camera_work must be set while the tank is in view, and player 0's work
 * while a jellyfish moves */
void shelter_jellyfish_step(struct task *task);

#endif
