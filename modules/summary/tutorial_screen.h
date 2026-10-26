/* how to play: SUMMARY's tutorial screen, which shows the tutorial of the
 * story picked on the menus a page at a time, over its back */
#ifndef SUMMARY_TUTORIAL_SCREEN_H
#define SUMMARY_TUTORIAL_SCREEN_H

/* start the screen. the story picked on the menus must be a character's,
 * from 0 to 5, unchecked. it destroys the streams, releases every texture,
 * and resets the heap and the tasks. it then loads its textures and the
 * sounds of emeraldcoast.mlt, restarts the streams and creates its tasks.
 * it returns 1 */
int tutorial_screen_start(void *unused);
/* step the screen. it fades in from black, waits for the player's answer,
 * and fades out to black. until then it returns OVERLAY_RUNNING. A on the
 * last page shows a menu of start and cancel. once the screen has faded
 * out, it returns OVERLAY_DONE for start, or OVERLAY_BACK for cancel or for
 * B or X on the first page. it returns OVERLAY_RESET on the pads' quit. it
 * ends the screen itself before it returns other than OVERLAY_RUNNING */
int tutorial_screen_step(void *unused);
/* end the screen. it stops the music and the voice, and destroys the
 * streams and every task. it then releases every texture and sets the
 * texture filter to bilinear. it returns 1 */
int tutorial_screen_end(void *unused);

#endif
