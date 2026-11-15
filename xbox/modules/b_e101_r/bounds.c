/* the end of B_E101_R's bss, where e101_r_load reads E101R.PB and E101R.NB,
 * rounded up to 32 bytes. make pc compiles this in place of
 * modules/b_e101_r/bounds.src, whose table gives the module's section
 * bounds. the port links every module into one program, so the banks have
 * a buffer of their own: the longer file, E101R.PB's 308288 bytes, in whole
 * sectors, as tools/memory.py sizes them, and room for the rounding */
#include <sg_xpt.h>

#include "system/loader.h"

static Uint8 e101_r_banks[SECTORS(308288) * SECTOR_SIZE + 31];

Uint8 *e101_r_bss_end = e101_r_banks;
