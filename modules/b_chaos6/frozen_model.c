/* Chaos 6's frozen overlay, which boss.c draws over him while he's frozen:
 * chaos6_frozen_object and its model */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos6/frozen_model.h"

static NJS_MATERIAL dat_0c9097d8[1] = {
#include "b_chaos6/frozen_model/dat_0c9097d8.inc"
};
static Sint16 dat_0c9097ec[2882] = {
#include "b_chaos6/frozen_model/dat_0c9097ec.inc"
};
static NJS_COLOR dat_0c90ae70[2259] = {
#include "b_chaos6/frozen_model/dat_0c90ae70.inc"
};
static NJS_MESHSET dat_0c90d1bc[1] = {
#include "b_chaos6/frozen_model/dat_0c90d1bc.inc"
};
static NJS_POINT3 dat_0c90d1d4[524] = {
#include "b_chaos6/frozen_model/dat_0c90d1d4.inc"
};
static NJS_VECTOR dat_0c90ea64[524] = {
#include "b_chaos6/frozen_model/dat_0c90ea64.inc"
};
static NJS_MODEL dat_0c9102f4 = {
#include "b_chaos6/frozen_model/dat_0c9102f4.inc"
};
NJS_OBJECT chaos6_frozen_object = {
#include "b_chaos6/frozen_model/chaos6_frozen_object.inc"
};
