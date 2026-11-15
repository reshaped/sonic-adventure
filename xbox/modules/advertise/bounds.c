/* the end of ADVERTISE, where card_open starts the backup library's work
 * area, rounded up to 32 bytes. make pc compiles this in place of
 * modules/advertise/bounds.src, whose DEND gives the module's end. the
 * port links every module into one program, so the work is a buffer of its
 * own: BUM_WORK_SIZE for 1 MB cards in all eight drives, as tools/memory.py
 * sizes the work past ADVERTISE, and room for the rounding */
#include <shinobi.h>
#include <sg_xpt.h>

static Uint8 advertise_backup_work[BUM_WORK_SIZE(BUD_CAPACITY_1MB, 8) + 31];

char *advertise_module_end = (char *)advertise_backup_work;
