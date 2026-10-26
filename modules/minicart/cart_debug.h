/* the intro's camera path, and how it is sampled from the keys
 * stg03/cart_routes.h declares. the camera editor on the cart's debug pages
 * edits and plays them. stg03/cart_debug.h declares the pages */
#ifndef MINICART_CART_DEBUG_H
#define MINICART_CART_DEBUG_H

#include "camera/camera.h"
/* stg03/cart_debug.h declares cart_debug_page, which has to be declared
 * before intro_path. SHC lays out zeroed globals in the order of their first
 * declarations. the image holds cart_debug_page at 0x0ca5df78 and intro_path
 * at 0x0ca5df7c */
#include "stg03/cart_debug.h"

/* the records intro_path holds */
#define INTRO_RECORDS 420
/* the records intro_path_make samples for each key, 1 / INTRO_STEPS of a
 * span apart */
#define INTRO_STEPS 20

/* the records intro_path_make last sampled, which the intro's camera and
 * the camera editor play */
extern struct camera_key intro_path[INTRO_RECORDS];

/* sample the closed uniform cubic b-spline of keys' first count keys into
 * intro_path. it takes INTRO_STEPS records to a key, and returns how many it
 * wrote. count must be 0, or 3 to 21: intro_path holds 21 keys' records */
int intro_path_make(struct camera_key *keys, int count);

#endif
