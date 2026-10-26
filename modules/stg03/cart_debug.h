/* the carts' motions, settings and routes, and the cart's debug pages */
#ifndef STG03_CART_DEBUG_H
#define STG03_CART_DEBUG_H

#include <ninja.h>
#include <sg_xpt.h>

#include "player/player.h"

/* how a character's cart moves, one field to a row of the motion page. only
 * that page reads max_reverse_speed, max_fall_speed and drift_steering */
struct cart_motion {
    Float max_speed;
    Float min_speed;
    Float max_reverse_speed;
    Float slide; /* how hard a slope pushes the cart along it */
    Float gravity;
    Float max_fall_speed;
    Float hover_height; /* the height it aims to hover at */
    Float grip;
    Float acceleration;
    Float braking;
    Float friction;
    int max_vitality;
    int smoke_vitality; /* the cart smokes at this vitality or below */
    Float steering;
    Float drift_steering;
};

/* the carts' settings, one field to a row of the settings page */
struct cart_settings {
    /* a monkey's cart chases player 0 within this squared distance */
    Float chase_distance;
    /* the steps a monkey's cart rests once it hits player 0. cart_take_hits
     * also passes over as many steps once player 0 is hurt aboard */
    int rest_time;
    Float enemy_acceleration;
    Float enemy_braking;
    Float search_acceleration;
    Float explosion_speed;
    int explosion_spin;
    /* a wrecked cart ends once its pieces have all lain still this many
     * steps, less 1 */
    int dead_wait_time;
    /* what a cart's ring_loss_timer starts at as player 0 boards it */
    int ring_loss_time;
};

struct cart_route {
    NJS_POINT3 *points;
    Sint32 count;
};

/* the routes an act has room for */
#define ROUTES 18
/* act 0's routes RAILS and RAILS + 1 are its rails' two sides, point for
 * point */
#define RAILS 16

/* how many keys the camera editor has for each act */
#define DEMO_KEYS 20

/* the debug's pages. pad_records[1]'s Start moves on through PAGE_NONE to
 * PAGE_CAMERA, and from there back to PAGE_NONE */
enum cart_debug_page {
    /* sets up the motion page: the character, row 0 and an amount of 1. it
     * then moves on to PAGE_NONE */
    PAGE_START,
    PAGE_NONE, /* shows nothing, and resets the camera editor */
    PAGE_MOTION,
    PAGE_SETTINGS,
    PAGE_ROUTES,
    PAGE_CAMERA,
    PAGES
};

/* each character's motion, by get_character's number */
extern struct cart_motion cart_motions[CHARACTER_COUNT];
extern struct cart_settings cart_settings;
/* the page cart_debug_run runs, a cart_debug_page */
extern int cart_debug_page;
/* each act's routes. a monkey's cart searches them in order until it
 * reaches one whose points are null. it then goes back to the first */
extern struct cart_route cart_routes[][ROUTES];

/* one call of the debug. per_info[0]'s left and right add and take a ring,
 * and holding per_info[1]'s L and R restarts the act. then the page runs.
 * pad_records[1]'s Start moves on a page while camera_cart_action is 0. it
 * waits until more than 5 calls have passed since the last move. the route
 * page's warps and the camera editor's first key need player 0's work. the
 * camera editor needs camera_work too */
void cart_debug_run(void);

#endif
