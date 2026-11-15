/* the end of B_ROBO's bss, past which erobo_load reads Zero's banks,
 * rounded up to 32 bytes. make pc compiles this in place of
 * modules/b_robo/bounds.src, whose table gives the module's section
 * bounds. the port links every module into one program, so the banks have
 * a buffer of their own: the longer file, EROBO.PB's 301560 bytes, in whole
 * sectors, as tools/memory.py sizes them, and room for the rounding */
#include <sg_xpt.h>

#include "system/loader.h"

static Uint8 eggmanrobo_banks[SECTORS(301560) * SECTOR_SIZE + 31];

Uint8 *eggmanrobo_bss_end = eggmanrobo_banks;
