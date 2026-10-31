/* Sonic's actions in the stage's generated data */
#ifndef STG06_SONIC_ACTIONS_H
#define STG06_SONIC_ACTIONS_H
#include <ninja.h>

/* the actions 1ST_READ.BIN's sonic_actions names, valid only while the stage
 * is loaded. each one's comment gives the rows of sonic_actions that name it:
 * their names in enum sonic_action, or their indexes where the enum has
 * none. row ACTION_CIRCLE_LAND's action lies in dat_0cadb8c8.c instead */
extern NJS_ACTION dat_0cad1584; /* ACTION_CIRCLE_RIGHT */
extern NJS_ACTION dat_0cad5248; /* ACTION_CIRCLE_LEFT */
extern NJS_ACTION dat_0cad6dac; /* ACTION_CIRCLE */
extern NJS_ACTION dat_0cad8f60; /* ACTION_CIRCLE_DOWN */

#endif
