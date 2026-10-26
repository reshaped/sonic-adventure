/* the stage's land: its table and entries, and the name of the pvm its
 * textures load from */
#include <ninja.h>

#include "b_egm1/land.h"
/* only the full build's table points to eggmobile1_land_texlist */
#include "b_egm1/land_textures.h" /* NOLINT(misc-include-cleaner) */
#include "collision/world.h"

extern struct land_motion eggmobile1_land_motions[];

/* the objects the land's entries name */
extern NJS_OBJECT dat_0c921078;
extern NJS_OBJECT dat_0c921454;
extern NJS_OBJECT dat_0c9217a8;
extern NJS_OBJECT dat_0c921a08;
extern NJS_OBJECT dat_0c9228ac;
extern NJS_OBJECT dat_0c9253d8;
extern NJS_OBJECT dat_0c926b30;
extern NJS_OBJECT dat_0c9285b0;
extern NJS_OBJECT dat_0c928968;
extern NJS_OBJECT dat_0c9297a4;
extern NJS_OBJECT dat_0c929c88;
extern NJS_OBJECT dat_0c929f90;
extern NJS_OBJECT dat_0c92e444;
extern NJS_OBJECT dat_0c931aac;
extern NJS_OBJECT dat_0c93220c;
extern NJS_OBJECT dat_0c9323e4;
extern NJS_OBJECT dat_0c9326e4;
extern NJS_OBJECT dat_0c932ae0;
extern NJS_OBJECT dat_0c933100;
extern NJS_OBJECT dat_0c9339f0;
extern NJS_OBJECT dat_0c933c00;
extern NJS_OBJECT dat_0c933e10;
extern NJS_OBJECT dat_0c9371c8;
extern NJS_OBJECT dat_0c93bd08;
extern NJS_OBJECT dat_0c93e41c;
extern NJS_OBJECT dat_0c93eee0;
extern NJS_OBJECT dat_0c93f44c;
extern NJS_OBJECT dat_0c93fa20;
extern NJS_OBJECT dat_0c93fbe4;
extern NJS_OBJECT dat_0c9403cc;
extern NJS_OBJECT dat_0c940f68;
extern NJS_OBJECT dat_0c94179c;
extern NJS_OBJECT dat_0c941924;
extern NJS_OBJECT dat_0c942b98;
extern NJS_OBJECT dat_0c943818;
extern NJS_OBJECT dat_0c9448bc;
extern NJS_OBJECT dat_0c9450ec;
extern NJS_OBJECT dat_0c9457b4;
extern NJS_OBJECT dat_0c9459c4;
extern NJS_OBJECT dat_0c946414;
extern NJS_OBJECT dat_0c9468bc;
extern NJS_OBJECT dat_0c947250;
extern NJS_OBJECT dat_0c94753c;
extern NJS_OBJECT dat_0c947930;
extern NJS_OBJECT dat_0c947bb8;
extern NJS_OBJECT dat_0c947e88;
extern NJS_OBJECT dat_0c948618;
extern NJS_OBJECT dat_0c948e58;
extern NJS_OBJECT dat_0c949358;
extern NJS_OBJECT dat_0c94972c;
extern NJS_OBJECT dat_0c949f30;
extern NJS_OBJECT dat_0c94a734;
extern NJS_OBJECT dat_0c94ad64;
extern NJS_OBJECT dat_0c94b828;
extern NJS_OBJECT dat_0c94c02c;
extern NJS_OBJECT dat_0c94ca30;
extern NJS_OBJECT dat_0c94d234;
extern NJS_OBJECT dat_0c94d97c;
extern NJS_OBJECT dat_0c94e180;
extern NJS_OBJECT dat_0c94e700;
extern NJS_OBJECT dat_0c94ec80;
extern NJS_OBJECT dat_0c94ef28;
extern NJS_OBJECT dat_0c94f1d0;
extern NJS_OBJECT dat_0c950014;
extern NJS_OBJECT dat_0c950ce8;
extern NJS_OBJECT dat_0c951b94;
extern NJS_OBJECT dat_0c9524e0;
extern NJS_OBJECT dat_0c95379c;
extern NJS_OBJECT dat_0c9542cc;
extern NJS_OBJECT dat_0c954718;
extern NJS_OBJECT dat_0c954b54;
extern NJS_OBJECT dat_0c954fac;
extern NJS_OBJECT dat_0c955460;
extern NJS_OBJECT dat_0c956278;
extern NJS_OBJECT dat_0c956714;
extern NJS_OBJECT dat_0c956bc0;
extern NJS_OBJECT dat_0c957200;
extern NJS_OBJECT dat_0c957500;
extern NJS_OBJECT dat_0c957b10;
extern NJS_OBJECT dat_0c95828c;
extern NJS_OBJECT dat_0c958390;
extern NJS_OBJECT dat_0c95904c;
extern NJS_OBJECT dat_0c959b2c;
extern NJS_OBJECT dat_0c95a75c;
extern NJS_OBJECT dat_0c95b5d8;
extern NJS_OBJECT dat_0c95bbbc;
extern NJS_OBJECT dat_0c95c7e4;
extern NJS_OBJECT dat_0c95ca84;
extern NJS_OBJECT dat_0c95df04;
extern NJS_OBJECT dat_0c95e6a0;
extern NJS_OBJECT dat_0c95ecc4;
extern NJS_OBJECT dat_0c95ee98;
extern NJS_OBJECT dat_0c95fea8;
extern NJS_OBJECT dat_0c9603bc;
extern NJS_OBJECT dat_0c9603f0;
extern NJS_OBJECT dat_0c960424;
extern NJS_OBJECT dat_0c960458;
extern NJS_OBJECT dat_0c96048c;
extern NJS_OBJECT dat_0c9604c0;
extern NJS_OBJECT dat_0c9604f4;
extern NJS_OBJECT dat_0c960528;
extern NJS_OBJECT dat_0c96055c;
extern NJS_OBJECT dat_0c960590;
extern NJS_OBJECT dat_0c9605c4;
extern NJS_OBJECT dat_0c9605f8;
extern NJS_OBJECT dat_0c96062c;
extern NJS_OBJECT dat_0c960660;
extern NJS_OBJECT dat_0c960694;
extern NJS_OBJECT dat_0c9606c8;
extern NJS_OBJECT dat_0c9606fc;
extern NJS_OBJECT dat_0c960730;
extern NJS_OBJECT dat_0c960764;
extern NJS_OBJECT dat_0c960798;
extern NJS_OBJECT dat_0c9607cc;
extern NJS_OBJECT dat_0c960800;
extern NJS_OBJECT dat_0c960834;
extern NJS_OBJECT dat_0c960868;
extern NJS_OBJECT dat_0c96089c;
extern NJS_OBJECT dat_0c9608d0;
extern NJS_OBJECT dat_0c960904;
extern NJS_OBJECT dat_0c960938;
extern NJS_OBJECT dat_0c96096c;
extern NJS_OBJECT dat_0c9609a0;
extern NJS_OBJECT dat_0c9609d4;
extern NJS_OBJECT dat_0c960a08;
extern NJS_OBJECT dat_0c960a3c;
extern NJS_OBJECT dat_0c960a70;
extern NJS_OBJECT dat_0c960aa4;
extern NJS_OBJECT dat_0c960ad8;
extern NJS_OBJECT dat_0c960b0c;
extern NJS_OBJECT dat_0c960b40;
extern NJS_OBJECT dat_0c960b74;

static struct land_entry eggmobile1_land_entries[132] = {
#include "b_egm1/land/eggmobile1_land_entries.inc"
};

#pragma section HEAD
/* the pvm's name is an array, since the table's generated body points only
 * at symbols. SHC aligns the array to 1 and a string literal to 4. as the
 * object's only CHEAD constant, the array starts where the literal would */
static const char eggmobile1_land_pvm[9] = {
#include "b_egm1/land/eggmobile1_land_pvm.inc"
};
struct land_table eggmobile1_land = {
#include "b_egm1/land/eggmobile1_land.inc"
};
#pragma section
