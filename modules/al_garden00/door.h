/* O AR_DOOR, the garden's folding door, and the copy of the garden's flags */
#ifndef AL_GARDEN00_DOOR_H
#define AL_GARDEN00_DOOR_H

#include "system/task.h"

/* the step O AR_DOOR names */
void garden_door_init(struct task *task);
/* the garden's flags are event_flags[0] to [9]. chao_get_file_data's bytes
 * keep a copy of them, which alife_save writes into the a-life file.
 * garden_flags_clear clears the flags and the copy, with no caller in this
 * image. garden_flags_load sets the flags from the copy, and
 * garden_flags_save sets the copy from the flags */
void garden_flags_clear(void);
void garden_flags_load(void);
void garden_flags_save(void);

#endif
