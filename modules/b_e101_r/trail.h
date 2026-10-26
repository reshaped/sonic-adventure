/* the copies E-101 mkII leaves: its arms' trails of light, its blur, and its
 * afterimage */
#ifndef B_E101_R_TRAIL_H
#define B_E101_R_TRAIL_H
#include <ninja.h>
#include <sg_xpt.h>

struct e101_mk2;
struct task;

/* make slot 0 each arm's newest. the matrices the trails kept stay, and
 * task is unread */
void e101_mk2_trails_reset(struct task *task);
/* add matrix to the arm's trail, which keeps the last 32. boss is unread */
void e101_mk2_trail_add(struct e101_mk2 *boss, NJS_MATRIX *matrix, int arm);
/* while the boss's flags hold a bit of flag, grow *copies by 1 toward 4.
 * while they hold none, shrink it by 1 toward 0. either changes it only when
 * boss->steps is even */
void e101_mk2_trail_grow(struct e101_mk2 *boss, Sint32 flag, Sint32 *copies);
/* draw boss->trail_copies[arm] copies of the arm's trail, in the colors
 * color picks */
void e101_mk2_trail_draw(struct e101_mk2 *boss, int arm, int color);
/* fill every kept pose with the boss's position, angles and tilt, at frame
 * 0 with no sway. slot 0 is then the newest */
void e101_mk2_poses_reset(struct task *task);
/* keep the boss's pose now in the ring's next slot */
void e101_mk2_pose_add(struct task *task);
/* keep the boss's pose now in the ring's next slot and the eight before it,
 * as it warps */
void e101_mk2_pose_add_warping(struct task *task);
/* draw boss->blur_copies copies of the boss in its kept poses, playing
 * action at each pose's frame */
void e101_mk2_blur_draw(struct e101_mk2 *boss, NJS_ACTION *action);
/* leave a copy of the boss, playing action in the pose before the newest.
 * the copy grows and fades out over 10 steps. it keeps a pointer to that
 * pose's slot, not a copy of the pose */
void e101_mk2_afterimage_create(struct e101_mk2 *boss, NJS_ACTION *action);

#endif
