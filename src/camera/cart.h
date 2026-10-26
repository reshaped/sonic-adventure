/* the cart camera's action, declared apart from camera.h. modes.c defines
 * it and leaves this header out. SHC lays out zeroed globals in the order of
 * their first declarations, and camera_cart_action follows zeroed statics of
 * modes.c's own */
#ifndef CAMERA_CART_H
#define CAMERA_CART_H

/* what CAMERA_MODE_CART's step does: 0 for its own views, or
 * CAMERA_CART_PLAY or CAMERA_CART_ORBIT */
extern int camera_cart_action;

#endif
