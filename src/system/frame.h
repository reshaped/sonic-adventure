/* frame pacing: the vsyncs and steps of a frame, its display, and the vsync
 * callback */
#ifndef SYSTEM_FRAME_H
#define SYSTEM_FRAME_H

/* the vsyncs to a frame, and the vsyncs left in the frame running */
extern int frame_interval;
extern int frame_countdown;
/* the steps to a frame, and the step running, from 0 */
extern int frame_step_count;
extern int frame_step;

/* make each frame last interval vsyncs and run steps steps, and start one
 * now, at step 0. it installs frame_vsync as Ninja's vsync callback and
 * empties the bitmap queue */
void frame_pacing_init(int interval, int steps);
/* make each frame last interval vsyncs, and start one now */
void frame_set_interval(int interval);
/* wait out the frame's last vsyncs, and start the next frame. it then
 * uploads the queued bitmaps, renders what the frame drew and flips to it,
 * or while draw_off is TRUE drops what it drew. last it serves the pads */
void frame_wait(void);
/* wait until left or fewer vsyncs of the frame remain, then serve the pads
 * and send the sound queue. a frame's loop calls it between its steps */
void frame_wait_step(int left);
/* the vsync callback. each vsync it counts frame_countdown down, then
 * serves the memory cards and checks the drive. at a frame's last vsync,
 * before those, it runs the act's hook and serves the sound driver */
void frame_vsync(void);
/* fun_8c054be0 and fun_8c054be4 each return at once */
void fun_8c054be0(void);
void fun_8c054be4(void);

#endif
