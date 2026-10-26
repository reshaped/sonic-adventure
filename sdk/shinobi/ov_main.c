/* overlay library: the shared library table, by ID, and module registration */
#include <machine.h>
#include <sg_ov.h>
#include <sg_xpt.h>
#include <string.h>

#define LIBTBL (*(OVS_LIBTBL **)OVD_LIBTBL_ADR_PTR)
#define LIBTBL_MAX (*(Sint32 *)OVD_LIBTBL_ADR_MAX)

/* the base of the SH-4's P2 area, through which memory is reached uncached */
#define P2 0xa0000000

/* the IDs of the records that open and close a module's library list. each
 * is a UUID's sixteen bytes, with no terminator */
/* NOLINTBEGIN(clang-diagnostic-unterminated-string-initialization) */
const char ovg_mdl_start_lidstr[16] = "Lib Handle Start";
const char ovg_mdl_end_lidstr[16] = "Lib Handle End  ";
/* NOLINTEND(clang-diagnostic-unterminated-string-initialization) */

OVS_LIB_OBJ ovg_libhdl = {NULL, OVD_VERSION_NO, OVD_REVISION_NO, 0, 0,
                          0,    OVD_LID};

void ovLockCs(OV_CS *cs);
void ovUnlockCs(const OV_CS *cs);
OVS_LIBTBL *ovGetLibTbl(Sint32 *max);
Sint32 ovSearchLib(OVLIB lib);
Sint32 ovSearchLibByUuid(UUID *lid);

/* ovLockCs masks every interrupt and saves the old mask in cs, which
 * ovUnlockCs restores */
void ovLockCs(OV_CS *cs)
{
    *cs = get_imask();
    set_imask(IMASK_ALL);
}

void ovUnlockCs(const OV_CS *cs)
{
    set_imask(*cs);
}

/* return the library table, and its capacity in *max. with no table set,
 * ovInit sets up the default one first */
OVS_LIBTBL *ovGetLibTbl(Sint32 *max)
{
    OVS_LIBTBL *table = LIBTBL;

    if (table == NULL) {
        ovInit((void *)OVD_DEF_LIBTBL_ADR, OVD_DEF_LIBTBL_MAX);
    }
    table = LIBTBL;
    *max = LIBTBL_MAX;
    return table;
}

Sint32 ovSearchLib(OVLIB lib)
{
    Sint32 max;
    int i;
    OVS_LIBTBL *table = ovGetLibTbl(&max);

    for (i = 0; i < table->nlib; i++) {
        if (lib == table->entry[i].lib) {
            return i;
        }
    }
    return OVE_ERR_INVALID_LIB_HDL;
}

Sint32 ovSearchLibByUuid(UUID *lid)
{
    Sint32 max;
    int i;
    OVS_LIBTBL *table = ovGetLibTbl(&max);
    OVLIB lib;

    for (i = 0; i < table->nlib; i++) {
        lib = table->entry[i].lib;
        if (memcmp(lid, &lib->lid, sizeof(UUID)) == 0) {
            return i;
        }
    }
    return OVE_ERR_INVALID_LIB_ID;
}

void ovInit(void *ovltbl, Sint32 nlib)
{
    LIBTBL = ovltbl;
    LIBTBL_MAX = nlib;
}

void ovFinish(void)
{
}

OVLIB ovLibOpen(UUID *lid)
{
    OV_CS cs;
    Sint32 max;
    int i;
    OVS_LIBTBL *table;
    OVLIB lib;

    ovLockCs(&cs);
    table = ovGetLibTbl(&max);
    i = ovSearchLibByUuid(lid);
    if (i >= 0) {
        lib = table->entry[i].lib;
        table->entry[i].ref++;
    } else {
        lib = NULL;
    }
    ovUnlockCs(&cs);
    return lib;
}

void ovLibClose(UUID *lid)
{
    OV_CS cs;
    Sint32 max;
    int i;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    OVE_ERR err;
    OVS_LIBTBL *table;

    ovLockCs(&cs);
    table = ovGetLibTbl(&max);
    i = ovSearchLibByUuid(lid);
    /* @hack redundant: err, set in each branch and never read, which has the
     * unlock take SR into R2 and its mask into R3 (0x8c61aec2, 0x8c61aec4);
     * without the stores, the two trade and 4 of 45 words differ */
    if (i >= 0) {
        err = OVE_ERR_OK;
        table->entry[i].ref--;
    } else {
        err = OVE_ERR_INVALID_LIB_ID;
    }
    ovUnlockCs(&cs);
}

OVE_ERR ovLibEntryTbl(OVLIB lib)
{
    OV_CS cs;
    Sint32 max;
    int i;
    OVS_LIBTBL *table;

    ovLockCs(&cs);
    table = ovGetLibTbl(&max);
    i = ovSearchLibByUuid(&lib->lid);
    if (i < 0) {
        /* @hack spelling: last, a copy of the count to index by, which SHC
         * loads into R4 (MOV.L @R14,R4 at 0x8c61af32 in 1ST_READ.BIN); with
         * entry set from table->nlib, the count goes to R5, and 7 of 57 words
         * differ in each image */
        int last = table->nlib;
        /* @hack type: entry, an unsigned copy of last, whose scaled value
         * SHC keeps in R5 for both stores (0x8c61af3a to 0x8c61af48 in
         * 1ST_READ.BIN). last, a count of libraries, is never negative, so
         * entry indexes as last does. indexed by last, 7 of 57 words differ in
         * each image */
        unsigned int entry = last;

        table->entry[entry].lib = lib;
        table->entry[entry].ref = 0;
        table->nlib++;
    }
    ovUnlockCs(&cs);
    return OVE_ERR_OK;
}

OVE_ERR ovLibDeleteTbl(OVLIB lib)
{
    OV_CS cs;
    Sint32 max;
    int i;
    int found;
    OVE_ERR err;
    OVS_LIBTBL *table;

    ovLockCs(&cs);
    table = ovGetLibTbl(&max);
    found = ovSearchLib(lib);
    if (found >= 0) {
        table->nlib--;
        for (i = found; i < table->nlib; i++) {
            /* @hack spelling: entry[next], which SHC scales apart from entry[i]
             * (ADD #1,R5 at 0x8c61afb2 in 1ST_READ.BIN); as entry[i + 1], 39
             * of 87 words differ in each image */
            int next;

            table->entry[i].lib = table->entry[next = i + 1].lib;
            table->entry[i].ref = table->entry[next].ref;
        }
        table->entry[i].lib = NULL;
        table->entry[i].ref = 0;
        err = OVE_ERR_OK;
    } else {
        err = OVE_ERR_INVALID_LIB_ID;
    }
    ovUnlockCs(&cs);
    return err;
}

OVLIB_IF ovLibGetLibIf(OVLIB lib)
{
    return lib->lif;
}

void ovLibGetVersion(OVLIB lib, Uint8 *ver, Uint8 *revi)
{
    *ver = lib->ver;
    *revi = lib->revi;
}

UUID *ovLibGetUuid(OVLIB lib)
{
    return &lib->lid;
}

OVE_ERR ovMdlEntryTbl(OVMDL ovmdl)
{
    /* @hack spelling: start and end, pointers to the records' IDs, which SHC
     * loads before the first compare (0x8c61b068, 0x8c61b06c in
     * 1ST_READ.BIN); with the IDs named in the compares, 38 of 44 words
     * differ in each image */
    const char *start = ovg_mdl_start_lidstr;
    const char *end = ovg_mdl_end_lidstr;
    OVLIB lib;
    int i;

    if (memcmp(&ovmdl->libobj[0].lid, start, sizeof(UUID)) != 0) {
        return OVE_ERR_INVALID_FORMAT;
    }
    for (i = 1;; i++) {
        lib = &ovmdl->libobj[i];
        if (memcmp(&lib->lid, end, sizeof(UUID)) == 0) {
            break;
        }
        ovLibEntryTbl(lib);
    }
    return OVE_ERR_OK;
}

OVE_ERR ovMdlDeleteTbl(OVMDL ovmdl)
{
    /* @hack spelling: start and end, pointers to the records' IDs, which SHC
     * loads before the first compare (0x8c61b0e6, 0x8c61b0ea in
     * 1ST_READ.BIN); with the IDs named in the compares, 17 of 44 words
     * differ in each image */
    const char *start = ovg_mdl_start_lidstr;
    const char *end = ovg_mdl_end_lidstr;
    OVLIB lib;
    int i;

    if (memcmp(&ovmdl->libobj[0].lid, start, sizeof(UUID)) != 0) {
        return OVE_ERR_INVALID_FORMAT;
    }
    for (i = 1;; i++) {
        lib = &ovmdl->libobj[i];
        if (memcmp(&lib->lid, end, sizeof(UUID)) == 0) {
            break;
        }
        ovLibDeleteTbl(lib);
    }
    return OVE_ERR_OK;
}

Sint32 ovMdlExec(OVMDL ovmdl)
{
    /* @hack implicit: the tail jump (JMP @R2 at 0x8c61b138 in 1ST_READ.BIN,
     * 0x8ce07178 in LOADER.BIN) leaves the module's own R0 as the result;
     * returned explicitly, the call builds the address in R0, not R2, and adds
     * a NOP, and 7 of 7 words differ in each image */
    ((Sint32 (*)(void))((Uint32)ovmdl | P2))();
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}
