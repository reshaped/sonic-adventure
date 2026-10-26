/* Shinobi overlay library, as far as the sources use it */
#ifndef SG_OV_H
#define SG_OV_H
#include <sg_xpt.h>

#ifndef UUID_DEFINED
#define UUID_DEFINED
typedef struct _UUID {
    Uint32 Data1;
    Uint16 Data2;
    Uint16 Data3;
    Uint8 Data4[8];
} UUID;
#endif

/* the overlay library's own ID, version and revision */
#define OVD_LID                                                                \
    {0xba7e0a00,                                                               \
     0xbda9,                                                                   \
     0x11d1,                                                                   \
     {0x8a, 0xeb, 0x00, 0x60, 0x08, 0x95, 0x62, 0x5f}}
#define OVD_VERSION_NO 0x00
#define OVD_REVISION_NO 0x41

typedef enum {
    OVE_ERR_OK = 0,
    OVE_ERR_INVALID_FORMAT = -1,
    OVE_ERR_INVALID_LIB_ID = -2,
    OVE_ERR_INVALID_LIB_HDL = -3
} OVE_ERR;

typedef struct {
    struct _ov_lif *lif;
    Uint8 ver;
    Uint8 revi;
    Sint16 rsv1;
    Sint32 rsv2;
    Sint32 rsv3;
    UUID lid;
} OVS_LIB_OBJ;
typedef OVS_LIB_OBJ *OVLIB;
typedef struct _ov_lif OVS_LIB_IF;
typedef OVS_LIB_IF *OVLIB_IF;

/* a module. ovMdlEntryTbl and ovMdlDeleteTbl enter or delete each library
 * between its start and end records. they know the records by their IDs,
 * "Lib Handle Start" and "Lib Handle End  ". they return
 * OVE_ERR_INVALID_FORMAT for a module whose first record isn't the start
 * record. neither's search for the end record has a bound */
typedef struct {
    Sint32 rsv[8];
    OVS_LIB_OBJ libobj[1]; /* start record, library handles, end record */
} OVS_MDL_OBJ;
typedef OVS_MDL_OBJ *OVMDL;

/* a critical section's saved interrupt mask */
typedef Sint32 OV_CS;

/* the library table, which every module finds through two words at fixed
 * addresses: the table's address and its capacity */
#define OVD_LIBTBL_ADR_PTR 0x8c00fff0
#define OVD_LIBTBL_ADR_MAX 0x8c00fff4
/* the default table's address and capacity. ovGetLibTbl passes them to ovInit
 * when no table is set */
#define OVD_DEF_LIBTBL_ADR 0x8c00fc00
#define OVD_DEF_LIBTBL_MAX 120
typedef struct {
    Sint32 nlib;
    Sint32 rsv1;
    struct {
        OVLIB lib;
        Sint32 ref;
    } entry[1];
} OVS_LIBTBL;

/* the overlay library's own handle */
extern OVS_LIB_OBJ ovg_libhdl;

/* the library with ID lid, its reference count raised, or NULL when the table
 * holds none */
OVLIB ovLibOpen(UUID *lid);
/* lower the reference count of the library with ID lid */
void ovLibClose(UUID *lid);
OVE_ERR ovMdlEntryTbl(OVMDL ovmdl);
OVE_ERR ovMdlDeleteTbl(OVMDL ovmdl);
/* run the module at ovmdl through P2, uncached, and return what it returns.
 * the module starts with ovmdl in R4 */
Sint32 ovMdlExec(OVMDL ovmdl);
OVLIB_IF ovLibGetLibIf(OVLIB lib);
void ovLibGetVersion(OVLIB lib, Uint8 *ver, Uint8 *revi);
UUID *ovLibGetUuid(OVLIB lib);
void ovInit(void *ovltbl, Sint32 nlib);
void ovFinish(void);
/* add lib, with no references, unless the table holds a library with its ID.
 * the result is always OVE_ERR_OK, and nothing checks the table's capacity */
OVE_ERR ovLibEntryTbl(OVLIB lib);
/* remove lib from the table and close its gap, or return
 * OVE_ERR_INVALID_LIB_ID when the table doesn't hold it */
OVE_ERR ovLibDeleteTbl(OVLIB lib);

#endif
