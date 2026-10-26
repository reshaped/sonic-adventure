/* the cart's intro: the camera flies a curve through the act's keys, then
 * the race's lights start */
#ifndef MINICART_INTRO_H
#define MINICART_INTRO_H

/* sample the act's intro path into intro_path, and have the camera play
 * it. once the camera is done, a task starts the race's lights */
void intro_camera_start(void);

#endif
