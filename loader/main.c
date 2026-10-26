/* LOADER.BIN's main: the hand-off to Dream Passport */
#include <ninja.h>
#include <sg_ov.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "loader/main.h"
#include "loader/sbinit.h"
#include "loader/sdk/gdfs.h"

/* the overlay library's handle and the GD file system's, in the first two
 * entries of the OVS_LIBTBL at OVD_DEF_LIBTBL_ADR. each entry takes 8
 * bytes, after the table's 8-byte head */
/* @hack type: volatile on both. it keeps the load of entry[1]'s
 * address (0x8ce00332) after the read of entry[0], and the loads of
 * sbInitSystem's frame and count (0x8ce00344, 0x8ce00346) after the two
 * stores; without it, those loads move earlier and 23 of 78 words differ */
#define OVERLAY_LIBRARY (*(OVLIB volatile *)(OVD_DEF_LIBTBL_ADR + 8))
#define GD_LIBRARY (*(OVLIB volatile *)(OVD_DEF_LIBTBL_ADR + 16))

/* where main reads sp.bin and jumps to it: 1ST_READ.BIN's load address */
#define PASSPORT_AREA 0x8c010000

void main(void)
{
    /* @hack spelling: file declared ahead of name, so that its MOV #0,R14
     * (0x8ce0031e) falls inside the copy of name's initializer; declared
     * after name, the MOV comes earlier and 5 of 78 words differ */
    GDFS file = NULL;
    char name[20] = "sp.bin";
    OVLIB overlay;
    OVLIB gd;
    Sint32 size;
    Sint32 sectors;

    /* the loader's own two libraries hold those entries from before the
     * system starts until after it ends. the handles they displace go back
     * before the jump */
    overlay = OVERLAY_LIBRARY;
    gd = GD_LIBRARY;
    OVERLAY_LIBRARY = &ovg_libhdl;
    GD_LIBRARY = &gdg_fs_libhdl;
    sbInitSystem(NJD_RESOLUTION_640x480_NTSCI, NJD_FRAMEBUFFER_MODE_RGB565, 1);
    /* on a disc without sp.bin, this loop never ends */
    while (file == NULL) {
        file = gdFsOpen(name, NULL);
    }
    gdFsGetFileSize(file, &size);
    sectors = (size + GDD_FS_SCTSIZE - 1) >> 11;
    gdFsRead(file, sectors, (void *)PASSPORT_AREA);
    gdFsClose(file);
    sbExitSystem();
    OVERLAY_LIBRARY = overlay;
    GD_LIBRARY = gd;
    ((void (*)(void))PASSPORT_AREA)();
}
