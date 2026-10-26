/* Eggman's shudder and the Egg Hornet's actions */
#include <ninja.h>

#include "b_egm1/egg_hornet.h"
#include "draw/motion.h"

extern NJS_ACTION dat_0c91841c;
extern NJS_ACTION dat_0c9185a0;
extern NJS_ACTION dat_0c918724;
extern NJS_ACTION dat_0c919138;
extern NJS_ACTION dat_0c91a3cc;
extern NJS_ACTION dat_0c91b670;
extern NJS_ACTION dat_0c91b774;
extern NJS_ACTION dat_0c91c9f8;
extern NJS_MDATA2 dat_0c91e268[];

static NJS_MOTION dat_0c91e348 = {
#include "b_egm1/actions/dat_0c91e348.inc"
};
NJS_ACTION eggman_shudder_action = {
#include "b_egm1/actions/eggman_shudder_action.inc"
};
struct motion_entry egg_hornet_actions[12] = {
#include "b_egm1/actions/egg_hornet_actions.inc"
};
