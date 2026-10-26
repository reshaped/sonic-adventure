/* kmiIntStatisticData's declaration, apart from kamui.h. SHC, the compiler,
 * lays zeroed globals out in the order of their first declarations. SDK 1.30J's
 * interrupt member defines a zeroed static ahead of kmiIntStatisticData, and
 * 1ST_READ.BIN holds them in that order. were this header included there, its
 * declaration would come first, and SHC would lay kmiIntStatisticData out
 * first. so neither interrupt member includes it */
#ifndef KMISTAT_H
#define KMISTAT_H

#include <kamui.h>

extern struct kmi_interrupt_statistics kmiIntStatisticData;

#endif
