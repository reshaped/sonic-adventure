/* the gardens' camera mode, which trails player 0, and the globals its
 * step keeps */
#ifndef AL_MAIN_CAMERA_H
#define AL_MAIN_CAMERA_H
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"

/* SHC lays out bss in the order of each name's first declaration. so this
 * header declares, in the image's order, every bss global camera.c
 * defines, those no other file names too. camera.c's five statics follow
 * them */

/* TRUE holds the camera: its target stops easing, and garden_camera_pos
 * stops taking its speed. the mine cart sets it as player 0 boards, and the
 * camera's first step clears it */
extern Bool garden_camera_still;
/* the zoom, which up and down on the pad pick: 0 for the narrowest field
 * of view to 2, the widest, where the camera starts */
extern Uint16 garden_camera_zoom;
/* the point the camera aims at, ahead of player 0. camera_state's target
 * eases toward it */
extern NJS_POINT3 garden_camera_aim;
/* the aim as the last step left it. the camera follows the aim's move
 * since then */
extern NJS_POINT3 garden_camera_last_aim;
/* where the camera is headed. camera_state's pos eases toward it. the
 * surfaces of the world's objects round it push it clear, and so do
 * cam_colli.c's keep-outs */
extern NJS_POINT3 garden_camera_pos;
/* garden_camera_pos's move each step */
extern NJS_VECTOR garden_camera_speed;
/* the step's push, which the speed takes and which is then zeroed */
extern NJS_VECTOR garden_camera_accel;
/* 48 bytes no code reads or writes */
extern NJS_VECTOR dat_0cb1e4fc[4];
/* the level distance from garden_camera_pos to the aim, as each step
 * measures it. L and R swing the camera faster the shorter it is */
extern Float garden_camera_trail;
/* the camera's first step zeroes these, and no code reads them */
extern Sint32 dat_0cb1e530;
extern Sint32 dat_0cb1e534;
extern Sint32 dat_0cb1e538;
/* TRUE turns the camera to its fixed view, at garden_camera_fixed_pos
 * looking at garden_camera_fixed_target, and FALSE turns it back */
extern Bool garden_camera_fixed;
/* how far the camera has turned to the fixed view: none at 0, and all of
 * the way at 0x8000, half a turn. at angle a, the fixed view's share of
 * the camera's pos and target is (1 - cos a) / 2. the angle turns 0x300 a
 * step toward the end garden_camera_fixed picks */
extern Angle garden_camera_fixed_angle;
extern NJS_POINT3 garden_camera_fixed_pos;
extern NJS_POINT3 garden_camera_fixed_target;

/* the step each garden's stage task gives camera_override_high. it moves
 * camera_state's pos and target, and turns fog off. while garden_camera_fixed
 * is FALSE, it sets the field of view by the zoom. in the view's first step,
 * while view->steps is 0, it first starts the camera beside player 0 */
void garden_camera_step(struct camera_view *view);

#endif
