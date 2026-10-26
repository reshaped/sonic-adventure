/* how far each story has been played, in percent, as the character wheel
 * shows it */
#ifndef ADVERTISE_STORY_PROGRESS_H
#define ADVERTISE_STORY_PROGRESS_H

/* the percent of story played, where story is an enum wheel_story. it is the
 * percent of the last of the story's marks that is reached, or 0 if none is.
 * a mark is reached once its event flag is set, or once its event has
 * played */
int story_progress(int story);

#endif
