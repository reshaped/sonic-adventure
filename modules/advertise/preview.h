/* the wheel's character preview: the character of the story at the top of the
 * wheel, which poses once the story is picked */
#ifndef ADVERTISE_PREVIEW_H
#define ADVERTISE_PREVIEW_H

#include "system/task.h"

/* preview_create starts the preview's task, and returns it, or NULL. it loads
 * the characters' textures, and starts a task for each story's character. for
 * the preview's first two steps, every character stands off screen. from then
 * on, each loops its first action. the character of menu_state.story, the story
 * at the top of the wheel, stands in view. the last story shows
 * last_story_object, turning, in place of a character. preview_kill ends the
 * task preview_create last made, with the characters' tasks, and releases their
 * textures. it doesn't forget the task, so call it at most once for each
 * preview_create.
 *
 * preview_set_chosen and preview_reset_pose need the task live. with chosen
 * nonzero, preview_set_chosen has the character at the top play its other
 * actions in turn, once its loop runs. the last of them loops.
 * preview_reset_pose queues the first action's loop again. nothing calls
 * preview_reset_pose */
struct task *preview_create(void);
void preview_kill(void);
void preview_set_chosen(int chosen);
void preview_reset_pose(void);

#endif
