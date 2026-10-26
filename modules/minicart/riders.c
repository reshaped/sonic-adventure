/* the motions and actions of Amy, Big, E-102, Knuckles and Tails riding
 * the cart */
#include <ninja.h>

#include "minicart/riders.h"
#include "player/big_models.h" /* NOLINT(misc-include-cleaner) */

extern NJS_OBJECT amy_object;
extern NJS_OBJECT e102_object;
extern NJS_OBJECT knuckles_object;
extern NJS_OBJECT tails_object;

#pragma section HEAD
static NJS_MKEY_A amy_cart_ang0[2] = {
#include "minicart/riders/amy_cart_ang0.inc"
};
static NJS_MKEY_A amy_cart_ang1[2] = {
#include "minicart/riders/amy_cart_ang1.inc"
};
static NJS_MKEY_A amy_cart_ang2[2] = {
#include "minicart/riders/amy_cart_ang2.inc"
};
static NJS_MKEY_A amy_cart_ang3[2] = {
#include "minicart/riders/amy_cart_ang3.inc"
};
static NJS_MKEY_A amy_cart_ang4[2] = {
#include "minicart/riders/amy_cart_ang4.inc"
};
static NJS_MKEY_A amy_cart_ang5[2] = {
#include "minicart/riders/amy_cart_ang5.inc"
};
static NJS_MKEY_A amy_cart_ang6[2] = {
#include "minicart/riders/amy_cart_ang6.inc"
};
static NJS_MKEY_A amy_cart_ang7[2] = {
#include "minicart/riders/amy_cart_ang7.inc"
};
static NJS_MKEY_A amy_cart_ang8[2] = {
#include "minicart/riders/amy_cart_ang8.inc"
};
static NJS_MKEY_A amy_cart_ang9[2] = {
#include "minicart/riders/amy_cart_ang9.inc"
};
static NJS_MKEY_A amy_cart_ang10[2] = {
#include "minicart/riders/amy_cart_ang10.inc"
};
static NJS_MKEY_A amy_cart_ang11[2] = {
#include "minicart/riders/amy_cart_ang11.inc"
};
static NJS_MKEY_A amy_cart_ang12[2] = {
#include "minicart/riders/amy_cart_ang12.inc"
};
static NJS_MKEY_A amy_cart_ang13[2] = {
#include "minicart/riders/amy_cart_ang13.inc"
};
static NJS_MKEY_A amy_cart_ang14[2] = {
#include "minicart/riders/amy_cart_ang14.inc"
};
static NJS_MKEY_A amy_cart_ang15[2] = {
#include "minicart/riders/amy_cart_ang15.inc"
};
static NJS_MKEY_A amy_cart_ang16[2] = {
#include "minicart/riders/amy_cart_ang16.inc"
};
static NJS_MKEY_A amy_cart_ang17[2] = {
#include "minicart/riders/amy_cart_ang17.inc"
};
static NJS_MKEY_A amy_cart_ang18[2] = {
#include "minicart/riders/amy_cart_ang18.inc"
};
static NJS_MKEY_A amy_cart_ang19[2] = {
#include "minicart/riders/amy_cart_ang19.inc"
};
static NJS_MKEY_A amy_cart_ang20[2] = {
#include "minicart/riders/amy_cart_ang20.inc"
};
static NJS_MKEY_A amy_cart_ang21[2] = {
#include "minicart/riders/amy_cart_ang21.inc"
};
static NJS_MKEY_A amy_cart_ang22[2] = {
#include "minicart/riders/amy_cart_ang22.inc"
};
static NJS_MKEY_A amy_cart_ang23[2] = {
#include "minicart/riders/amy_cart_ang23.inc"
};
static NJS_MKEY_A amy_cart_ang24[2] = {
#include "minicart/riders/amy_cart_ang24.inc"
};
static NJS_MKEY_A amy_cart_ang25[2] = {
#include "minicart/riders/amy_cart_ang25.inc"
};
static NJS_MKEY_A amy_cart_ang26[2] = {
#include "minicart/riders/amy_cart_ang26.inc"
};
static NJS_MKEY_A amy_cart_ang27[2] = {
#include "minicart/riders/amy_cart_ang27.inc"
};
static NJS_MKEY_A amy_cart_ang28[2] = {
#include "minicart/riders/amy_cart_ang28.inc"
};
static NJS_MDATA2 amy_cart_mdata[78] = {
#include "minicart/riders/amy_cart_mdata.inc"
};
static NJS_MOTION amy_cart_motion = {
#include "minicart/riders/amy_cart_motion.inc"
};
NJS_ACTION amy_cart_action = {
#include "minicart/riders/amy_cart_action.inc"
};
static NJS_MKEY_F big_cart_pos0[2] = {
#include "minicart/riders/big_cart_pos0.inc"
};
static NJS_MKEY_A big_cart_ang0[2] = {
#include "minicart/riders/big_cart_ang0.inc"
};
static NJS_MKEY_A big_cart_ang1[2] = {
#include "minicart/riders/big_cart_ang1.inc"
};
static NJS_MKEY_A big_cart_ang2[2] = {
#include "minicart/riders/big_cart_ang2.inc"
};
static NJS_MKEY_A big_cart_ang3[2] = {
#include "minicart/riders/big_cart_ang3.inc"
};
static NJS_MKEY_A big_cart_ang4[2] = {
#include "minicart/riders/big_cart_ang4.inc"
};
static NJS_MKEY_A big_cart_ang5[2] = {
#include "minicart/riders/big_cart_ang5.inc"
};
static NJS_MKEY_A big_cart_ang6[2] = {
#include "minicart/riders/big_cart_ang6.inc"
};
static NJS_MKEY_A big_cart_ang7[2] = {
#include "minicart/riders/big_cart_ang7.inc"
};
static NJS_MKEY_A big_cart_ang8[2] = {
#include "minicart/riders/big_cart_ang8.inc"
};
static NJS_MKEY_A big_cart_ang9[2] = {
#include "minicart/riders/big_cart_ang9.inc"
};
static NJS_MKEY_A big_cart_ang10[2] = {
#include "minicart/riders/big_cart_ang10.inc"
};
static NJS_MKEY_A big_cart_ang11[2] = {
#include "minicart/riders/big_cart_ang11.inc"
};
static NJS_MKEY_A big_cart_ang12[2] = {
#include "minicart/riders/big_cart_ang12.inc"
};
static NJS_MKEY_A big_cart_ang13[2] = {
#include "minicart/riders/big_cart_ang13.inc"
};
static NJS_MKEY_A big_cart_ang14[2] = {
#include "minicart/riders/big_cart_ang14.inc"
};
static NJS_MKEY_A big_cart_ang15[2] = {
#include "minicart/riders/big_cart_ang15.inc"
};
static NJS_MKEY_A big_cart_ang16[2] = {
#include "minicart/riders/big_cart_ang16.inc"
};
static NJS_MKEY_A big_cart_ang17[2] = {
#include "minicart/riders/big_cart_ang17.inc"
};
static NJS_MKEY_A big_cart_ang18[2] = {
#include "minicart/riders/big_cart_ang18.inc"
};
static NJS_MKEY_A big_cart_ang19[2] = {
#include "minicart/riders/big_cart_ang19.inc"
};
static NJS_MKEY_A big_cart_ang20[2] = {
#include "minicart/riders/big_cart_ang20.inc"
};
static NJS_MKEY_A big_cart_ang21[2] = {
#include "minicart/riders/big_cart_ang21.inc"
};
static NJS_MKEY_A big_cart_ang22[2] = {
#include "minicart/riders/big_cart_ang22.inc"
};
static NJS_MKEY_A big_cart_ang23[2] = {
#include "minicart/riders/big_cart_ang23.inc"
};
static NJS_MKEY_A big_cart_ang24[2] = {
#include "minicart/riders/big_cart_ang24.inc"
};
static NJS_MKEY_A big_cart_ang25[2] = {
#include "minicart/riders/big_cart_ang25.inc"
};
static NJS_MKEY_A big_cart_ang26[2] = {
#include "minicart/riders/big_cart_ang26.inc"
};
static NJS_MKEY_A big_cart_ang27[2] = {
#include "minicart/riders/big_cart_ang27.inc"
};
static NJS_MDATA2 big_cart_mdata[73] = {
#include "minicart/riders/big_cart_mdata.inc"
};
static NJS_MOTION big_cart_motion = {
#include "minicart/riders/big_cart_motion.inc"
};
NJS_ACTION big_cart_action = {
#include "minicart/riders/big_cart_action.inc"
};
static NJS_MKEY_A e102_cart_ang0[2] = {
#include "minicart/riders/e102_cart_ang0.inc"
};
static NJS_MKEY_A e102_cart_ang1[2] = {
#include "minicart/riders/e102_cart_ang1.inc"
};
static NJS_MKEY_A e102_cart_ang2[2] = {
#include "minicart/riders/e102_cart_ang2.inc"
};
static NJS_MKEY_A e102_cart_ang3[2] = {
#include "minicart/riders/e102_cart_ang3.inc"
};
static NJS_MKEY_A e102_cart_ang4[2] = {
#include "minicart/riders/e102_cart_ang4.inc"
};
static NJS_MKEY_A e102_cart_ang5[2] = {
#include "minicart/riders/e102_cart_ang5.inc"
};
static NJS_MKEY_A e102_cart_ang6[2] = {
#include "minicart/riders/e102_cart_ang6.inc"
};
static NJS_MKEY_A e102_cart_ang7[2] = {
#include "minicart/riders/e102_cart_ang7.inc"
};
static NJS_MKEY_A e102_cart_ang8[2] = {
#include "minicart/riders/e102_cart_ang8.inc"
};
static NJS_MKEY_A e102_cart_ang9[2] = {
#include "minicart/riders/e102_cart_ang9.inc"
};
static NJS_MKEY_A e102_cart_ang10[2] = {
#include "minicart/riders/e102_cart_ang10.inc"
};
static NJS_MKEY_A e102_cart_ang11[2] = {
#include "minicart/riders/e102_cart_ang11.inc"
};
static NJS_MKEY_A e102_cart_ang12[2] = {
#include "minicart/riders/e102_cart_ang12.inc"
};
static NJS_MKEY_A e102_cart_ang13[2] = {
#include "minicart/riders/e102_cart_ang13.inc"
};
static NJS_MKEY_A e102_cart_ang14[2] = {
#include "minicart/riders/e102_cart_ang14.inc"
};
static NJS_MKEY_A e102_cart_ang15[2] = {
#include "minicart/riders/e102_cart_ang15.inc"
};
static NJS_MKEY_A e102_cart_ang16[2] = {
#include "minicart/riders/e102_cart_ang16.inc"
};
static NJS_MKEY_A e102_cart_ang17[2] = {
#include "minicart/riders/e102_cart_ang17.inc"
};
static NJS_MKEY_A e102_cart_ang18[2] = {
#include "minicart/riders/e102_cart_ang18.inc"
};
static NJS_MKEY_A e102_cart_ang19[2] = {
#include "minicart/riders/e102_cart_ang19.inc"
};
static NJS_MKEY_A e102_cart_ang20[2] = {
#include "minicart/riders/e102_cart_ang20.inc"
};
static NJS_MDATA2 e102_cart_mdata[77] = {
#include "minicart/riders/e102_cart_mdata.inc"
};
static NJS_MOTION e102_cart_motion = {
#include "minicart/riders/e102_cart_motion.inc"
};
NJS_ACTION e102_cart_action = {
#include "minicart/riders/e102_cart_action.inc"
};
static NJS_MKEY_A knuckles_cart_ang0[2] = {
#include "minicart/riders/knuckles_cart_ang0.inc"
};
static NJS_MKEY_A knuckles_cart_ang1[2] = {
#include "minicart/riders/knuckles_cart_ang1.inc"
};
static NJS_MKEY_A knuckles_cart_ang2[2] = {
#include "minicart/riders/knuckles_cart_ang2.inc"
};
static NJS_MKEY_A knuckles_cart_ang3[2] = {
#include "minicart/riders/knuckles_cart_ang3.inc"
};
static NJS_MKEY_A knuckles_cart_ang4[2] = {
#include "minicart/riders/knuckles_cart_ang4.inc"
};
static NJS_MKEY_A knuckles_cart_ang5[2] = {
#include "minicart/riders/knuckles_cart_ang5.inc"
};
static NJS_MKEY_A knuckles_cart_ang6[2] = {
#include "minicart/riders/knuckles_cart_ang6.inc"
};
static NJS_MKEY_A knuckles_cart_ang7[2] = {
#include "minicart/riders/knuckles_cart_ang7.inc"
};
static NJS_MKEY_A knuckles_cart_ang8[2] = {
#include "minicart/riders/knuckles_cart_ang8.inc"
};
static NJS_MKEY_A knuckles_cart_ang9[2] = {
#include "minicart/riders/knuckles_cart_ang9.inc"
};
static NJS_MKEY_A knuckles_cart_ang10[2] = {
#include "minicart/riders/knuckles_cart_ang10.inc"
};
static NJS_MKEY_A knuckles_cart_ang11[2] = {
#include "minicart/riders/knuckles_cart_ang11.inc"
};
static NJS_MKEY_A knuckles_cart_ang12[2] = {
#include "minicart/riders/knuckles_cart_ang12.inc"
};
static NJS_MKEY_A knuckles_cart_ang13[2] = {
#include "minicart/riders/knuckles_cart_ang13.inc"
};
static NJS_MKEY_A knuckles_cart_ang14[2] = {
#include "minicart/riders/knuckles_cart_ang14.inc"
};
static NJS_MKEY_A knuckles_cart_ang15[2] = {
#include "minicart/riders/knuckles_cart_ang15.inc"
};
static NJS_MKEY_A knuckles_cart_ang16[2] = {
#include "minicart/riders/knuckles_cart_ang16.inc"
};
static NJS_MKEY_A knuckles_cart_ang17[2] = {
#include "minicart/riders/knuckles_cart_ang17.inc"
};
static NJS_MKEY_A knuckles_cart_ang18[2] = {
#include "minicart/riders/knuckles_cart_ang18.inc"
};
static NJS_MKEY_A knuckles_cart_ang19[2] = {
#include "minicart/riders/knuckles_cart_ang19.inc"
};
static NJS_MKEY_A knuckles_cart_ang20[2] = {
#include "minicart/riders/knuckles_cart_ang20.inc"
};
static NJS_MKEY_A knuckles_cart_ang21[2] = {
#include "minicart/riders/knuckles_cart_ang21.inc"
};
static NJS_MKEY_A knuckles_cart_ang22[2] = {
#include "minicart/riders/knuckles_cart_ang22.inc"
};
static NJS_MKEY_A knuckles_cart_ang23[2] = {
#include "minicart/riders/knuckles_cart_ang23.inc"
};
static NJS_MKEY_A knuckles_cart_ang24[2] = {
#include "minicart/riders/knuckles_cart_ang24.inc"
};
static NJS_MKEY_A knuckles_cart_ang25[2] = {
#include "minicart/riders/knuckles_cart_ang25.inc"
};
static NJS_MKEY_A knuckles_cart_ang26[2] = {
#include "minicart/riders/knuckles_cart_ang26.inc"
};
static NJS_MDATA2 knuckles_cart_mdata[73] = {
#include "minicart/riders/knuckles_cart_mdata.inc"
};
static NJS_MOTION knuckles_cart_motion = {
#include "minicart/riders/knuckles_cart_motion.inc"
};
NJS_ACTION knuckles_cart_action = {
#include "minicart/riders/knuckles_cart_action.inc"
};
static NJS_MKEY_A tails_cart_ang0[2] = {
#include "minicart/riders/tails_cart_ang0.inc"
};
static NJS_MKEY_A tails_cart_ang1[2] = {
#include "minicart/riders/tails_cart_ang1.inc"
};
static NJS_MKEY_A tails_cart_ang2[2] = {
#include "minicart/riders/tails_cart_ang2.inc"
};
static NJS_MKEY_A tails_cart_ang3[2] = {
#include "minicart/riders/tails_cart_ang3.inc"
};
static NJS_MKEY_A tails_cart_ang4[2] = {
#include "minicart/riders/tails_cart_ang4.inc"
};
static NJS_MKEY_A tails_cart_ang5[2] = {
#include "minicart/riders/tails_cart_ang5.inc"
};
static NJS_MKEY_A tails_cart_ang6[2] = {
#include "minicart/riders/tails_cart_ang6.inc"
};
static NJS_MKEY_A tails_cart_ang7[2] = {
#include "minicart/riders/tails_cart_ang7.inc"
};
static NJS_MKEY_A tails_cart_ang8[2] = {
#include "minicart/riders/tails_cart_ang8.inc"
};
static NJS_MKEY_A tails_cart_ang9[2] = {
#include "minicart/riders/tails_cart_ang9.inc"
};
static NJS_MKEY_A tails_cart_ang10[2] = {
#include "minicart/riders/tails_cart_ang10.inc"
};
static NJS_MKEY_A tails_cart_ang11[2] = {
#include "minicart/riders/tails_cart_ang11.inc"
};
static NJS_MKEY_A tails_cart_ang12[2] = {
#include "minicart/riders/tails_cart_ang12.inc"
};
static NJS_MKEY_A tails_cart_ang13[2] = {
#include "minicart/riders/tails_cart_ang13.inc"
};
static NJS_MKEY_A tails_cart_ang14[2] = {
#include "minicart/riders/tails_cart_ang14.inc"
};
static NJS_MKEY_A tails_cart_ang15[2] = {
#include "minicart/riders/tails_cart_ang15.inc"
};
static NJS_MKEY_A tails_cart_ang16[2] = {
#include "minicart/riders/tails_cart_ang16.inc"
};
static NJS_MKEY_A tails_cart_ang17[2] = {
#include "minicart/riders/tails_cart_ang17.inc"
};
static NJS_MKEY_A tails_cart_ang18[2] = {
#include "minicart/riders/tails_cart_ang18.inc"
};
static NJS_MKEY_A tails_cart_ang19[2] = {
#include "minicart/riders/tails_cart_ang19.inc"
};
static NJS_MKEY_A tails_cart_ang20[2] = {
#include "minicart/riders/tails_cart_ang20.inc"
};
static NJS_MKEY_A tails_cart_ang21[2] = {
#include "minicart/riders/tails_cart_ang21.inc"
};
static NJS_MKEY_A tails_cart_ang22[2] = {
#include "minicart/riders/tails_cart_ang22.inc"
};
static NJS_MKEY_A tails_cart_ang23[2] = {
#include "minicart/riders/tails_cart_ang23.inc"
};
static NJS_MKEY_A tails_cart_ang24[2] = {
#include "minicart/riders/tails_cart_ang24.inc"
};
static NJS_MDATA2 tails_cart_mdata[68] = {
#include "minicart/riders/tails_cart_mdata.inc"
};
static NJS_MOTION tails_cart_motion = {
#include "minicart/riders/tails_cart_motion.inc"
};
NJS_ACTION tails_cart_action = {
#include "minicart/riders/tails_cart_action.inc"
};
#pragma section
