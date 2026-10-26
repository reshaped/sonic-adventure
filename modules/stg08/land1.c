/* act 1's land: its entries, and the table that points to them */
#include <ninja.h>

#include "collision/world.h"
#include "stg08/icecap02_textures.h" /* NOLINT(misc-include-cleaner) */
#include "stg08/land1.h"

/* the objects the land's entries name */
extern NJS_OBJECT dat_0c95e9c8;
extern NJS_OBJECT dat_0c9604f0;
extern NJS_OBJECT dat_0c9615f8;
extern NJS_OBJECT dat_0c962940;
extern NJS_OBJECT dat_0c96370c;
extern NJS_OBJECT dat_0c9639b4;
extern NJS_OBJECT dat_0c963b2c;
extern NJS_OBJECT dat_0c963d60;
extern NJS_OBJECT dat_0c9640ac;
extern NJS_OBJECT dat_0c9643dc;
extern NJS_OBJECT dat_0c9646e8;
extern NJS_OBJECT dat_0c9649dc;
extern NJS_OBJECT dat_0c964e38;
extern NJS_OBJECT dat_0c965374;
extern NJS_OBJECT dat_0c965bb8;
extern NJS_OBJECT dat_0c965eb0;
extern NJS_OBJECT dat_0c9661d0;
extern NJS_OBJECT dat_0c966adc;
extern NJS_OBJECT dat_0c966e54;
extern NJS_OBJECT dat_0c966f24;
extern NJS_OBJECT dat_0c967178;
extern NJS_OBJECT dat_0c967248;
extern NJS_OBJECT dat_0c96742c;
extern NJS_OBJECT dat_0c967578;
extern NJS_OBJECT dat_0c9676a4;
extern NJS_OBJECT dat_0c9677b0;
extern NJS_OBJECT dat_0c967e58;
extern NJS_OBJECT dat_0c9680c4;
extern NJS_OBJECT dat_0c96836c;
extern NJS_OBJECT dat_0c96871c;
extern NJS_OBJECT dat_0c9689d4;
extern NJS_OBJECT dat_0c968bc8;
extern NJS_OBJECT dat_0c969498;
extern NJS_OBJECT dat_0c969b98;
extern NJS_OBJECT dat_0c96aa98;
extern NJS_OBJECT dat_0c96b654;
extern NJS_OBJECT dat_0c96b9f8;
extern NJS_OBJECT dat_0c96c3e0;
extern NJS_OBJECT dat_0c96d40c;
extern NJS_OBJECT dat_0c96de4c;
extern NJS_OBJECT dat_0c96e624;
extern NJS_OBJECT dat_0c96f6d0;
extern NJS_OBJECT dat_0c9708ac;
extern NJS_OBJECT dat_0c972950;
extern NJS_OBJECT dat_0c9737bc;
extern NJS_OBJECT dat_0c974454;
extern NJS_OBJECT dat_0c977d78;
extern NJS_OBJECT dat_0c9781f4;
extern NJS_OBJECT dat_0c97837c;
extern NJS_OBJECT dat_0c97c1b4;
extern NJS_OBJECT dat_0c97d9b4;
extern NJS_OBJECT dat_0c97e5f8;
extern NJS_OBJECT dat_0c97f434;
extern NJS_OBJECT dat_0c97fd50;
extern NJS_OBJECT dat_0c9806a8;
extern NJS_OBJECT dat_0c982244;
extern NJS_OBJECT dat_0c983094;
extern NJS_OBJECT dat_0c983300;
extern NJS_OBJECT dat_0c983c3c;
extern NJS_OBJECT dat_0c984518;
extern NJS_OBJECT dat_0c984764;
extern NJS_OBJECT dat_0c984868;
extern NJS_OBJECT dat_0c984d90;
extern NJS_OBJECT dat_0c98505c;
extern NJS_OBJECT dat_0c985404;
extern NJS_OBJECT dat_0c985824;
extern NJS_OBJECT dat_0c985fe4;
extern NJS_OBJECT dat_0c9886a0;
extern NJS_OBJECT dat_0c988abc;
extern NJS_OBJECT dat_0c98a224;
extern NJS_OBJECT dat_0c98b594;
extern NJS_OBJECT dat_0c98cd54;
extern NJS_OBJECT dat_0c98e050;
extern NJS_OBJECT dat_0c98f614;
extern NJS_OBJECT dat_0c98f890;
extern NJS_OBJECT dat_0c98f994;
extern NJS_OBJECT dat_0c98fc10;
extern NJS_OBJECT dat_0c9b1e84;
extern NJS_OBJECT dat_0c9c5ff4;
extern NJS_OBJECT dat_0c9c6028;
extern NJS_OBJECT dat_0c9c605c;
extern NJS_OBJECT dat_0c9c6090;
extern NJS_OBJECT dat_0c9c60c4;

static struct land_entry icecap_act1_land_entries[83] = {
#include "stg08/land1/icecap_act1_land_entries.inc"
};
struct land_table icecap_act1_land = {
#include "stg08/land1/icecap_act1_land.inc"
};
