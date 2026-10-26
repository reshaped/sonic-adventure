/* the routes cart_setup gives the acts, and the camera editor's keys */
#ifndef STG03_CART_ROUTES_H
#define STG03_CART_ROUTES_H

#include "camera/camera.h"
#include "stg03/cart_debug.h"

/* each act's first route, which cart_setup gives it outside Twinkle Park */
extern struct cart_route cart_start_routes[];
/* a route with no points, which cart_setup gives each route past the first */
extern struct cart_route cart_no_route;
/* the keys the camera editor edits, DEMO_KEYS for each act */
extern struct camera_key (*camera_demo_keys)[DEMO_KEYS];

#endif
