/* the carts' routes, and the camera editor's keys */
#include <ninja.h>

#include "camera/camera.h"
#include "stg03/cart_debug.h"
#include "stg03/cart_routes.h"

/* the routes' points, one array for each route. act 0's six routes come first,
 * then its rails' two sides, then each act's start route */
static NJS_POINT3 dat_0c9f41d0[16] = {
#include "stg03/cart_routes/dat_0c9f41d0.inc"
};
static NJS_POINT3 dat_0c9f4290[44] = {
#include "stg03/cart_routes/dat_0c9f4290.inc"
};
static NJS_POINT3 dat_0c9f44a0[33] = {
#include "stg03/cart_routes/dat_0c9f44a0.inc"
};
static NJS_POINT3 dat_0c9f462c[31] = {
#include "stg03/cart_routes/dat_0c9f462c.inc"
};
static NJS_POINT3 dat_0c9f47a0[50] = {
#include "stg03/cart_routes/dat_0c9f47a0.inc"
};
static NJS_POINT3 dat_0c9f49f8[34] = {
#include "stg03/cart_routes/dat_0c9f49f8.inc"
};
static NJS_POINT3 dat_0c9f4b90[200] = {
#include "stg03/cart_routes/dat_0c9f4b90.inc"
};
static NJS_POINT3 dat_0c9f54f0[200] = {
#include "stg03/cart_routes/dat_0c9f54f0.inc"
};
static NJS_POINT3 dat_0c9f5e50[17] = {
#include "stg03/cart_routes/dat_0c9f5e50.inc"
};
static NJS_POINT3 dat_0c9f5f1c[17] = {
#include "stg03/cart_routes/dat_0c9f5f1c.inc"
};
static NJS_POINT3 dat_0c9f5fe8[8] = {
#include "stg03/cart_routes/dat_0c9f5fe8.inc"
};
static NJS_POINT3 dat_0c9f6048[12] = {
#include "stg03/cart_routes/dat_0c9f6048.inc"
};
static NJS_POINT3 dat_0c9f60d8[12] = {
#include "stg03/cart_routes/dat_0c9f60d8.inc"
};
static NJS_POINT3 dat_0c9f6168[8] = {
#include "stg03/cart_routes/dat_0c9f6168.inc"
};
struct cart_route cart_start_routes[6] = {
#include "stg03/cart_routes/cart_start_routes.inc"
};
struct cart_route cart_no_route = {
#include "stg03/cart_routes/cart_no_route.inc"
};
struct cart_route cart_routes[6][ROUTES] = {
#include "stg03/cart_routes/cart_routes.inc"
};
static struct camera_key dat_0c9f6560[6][DEMO_KEYS] = {
#include "stg03/cart_routes/dat_0c9f6560.inc"
};
struct camera_key (*camera_demo_keys)[DEMO_KEYS] = dat_0c9f6560;
