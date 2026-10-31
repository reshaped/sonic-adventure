/* Sonic's actions in the stage's generated data */
#ifndef STG08_SONIC_ACTIONS_H
#define STG08_SONIC_ACTIONS_H
#include <ninja.h>

/* the actions 1ST_READ.BIN's sonic_actions names, valid only while the stage
 * is loaded. each one's comment gives the rows of sonic_actions that name it:
 * their names in enum sonic_action, or their indexes where the enum has none */
extern NJS_ACTION dat_0caa6454; /* ACTION_DANGLE_LET_GO */
extern NJS_ACTION dat_0caa7d08; /* rows 97 and 98 */
extern NJS_ACTION dat_0caaaaac; /* ACTION_DANGLE_LEFT */
extern NJS_ACTION dat_0caad820; /* ACTION_DANGLE_RIGHT, ACTION_DANGLE_CLIMB */
extern NJS_ACTION dat_0caaee64; /* ACTION_DANGLE */
extern NJS_ACTION dat_0cab0ae8; /* ACTION_DANGLE_SLIP */

#endif
