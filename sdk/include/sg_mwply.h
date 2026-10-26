/* middleware player, as far as the sources use it */
#ifndef SG_MWPLY_H
#define SG_MWPLY_H
#include <sg_xpt.h>

/* the vertical blank chain priority of the manager's handler and the GSC's */
#define MWD_PLY_PRI_VBLNK (0x70)

/* a player's status, as GetStat returns it */
typedef enum _mwe_ply_stat {
    MWE_PLY_STAT_STOP = 0,
    MWE_PLY_STAT_PREP = 1,
    MWE_PLY_STAT_PLAYING = 2,
    MWE_PLY_STAT_PLAYEND = 3,
    MWE_PLY_STAT_ERROR = 4
} MWE_PLY_STAT;

/* a player handle. each kind of player creates its own, whose first member
 * points at its calls */
typedef struct {
    struct _mwply_if *vtbl;
} MW_PLY_OBJ;
typedef MW_PLY_OBJ *MWPLY;

/* ecode is an error message */
typedef void (*MW_PLY_ERRFN)(void *obj, char *ecode);

/* a player's calls. the first three slots are COM's: the SFD player leaves them
 * null, and nothing calls them */
typedef struct _mwply_if {
    void (*QueryInterface)();
    void (*AddRef)();
    void (*Release)();
    void (*VsyncHndl)(MWPLY ply);
    void (*ExecSvrHndl)(MWPLY ply);
    void (*Destroy)(MWPLY ply);
    void (*StartFname)(MWPLY ply, Sint8 *fname);
    void (*Stop)(MWPLY ply);
    MWE_PLY_STAT (*GetStat)(MWPLY ply);
    /* the time played: *ncount counts at *tscale a second */
    void (*GetTime)(MWPLY ply, Sint32 *ncount, Sint32 *tscale);
    void (*Pause)(MWPLY ply, Sint32 sw);
    /* set the output volume from vol: 0 is the loudest, and lower values are
     * quieter. the SFD player passes vol through ADXT_SetOutVol to
     * mwRnaSetOutVol, which clamps it to -999 through 0 */
    void (*SetOutVol)(MWPLY ply, Sint32 vol);
    Sint32 (*GetOutVol)(MWPLY ply);
    void (*SetOutPan)(MWPLY ply, Sint32 chno, Sint32 pan);
    Sint32 (*GetOutPan)(MWPLY ply, Sint32 chno);
} MWD_IF;

#define mwPlyDestroy(ply) (*(ply)->vtbl->Destroy)((ply))
#define mwPlyStartFname(ply, fname) (*(ply)->vtbl->StartFname)((ply), (fname))
#define mwPlyStop(ply) (*(ply)->vtbl->Stop)((ply))
#define mwPlyGetStat(ply) (*(ply)->vtbl->GetStat)((ply))
#define mwPlyGetTime(ply, ncount, tscale)                                      \
    (*(ply)->vtbl->GetTime)((ply), (ncount), (tscale))
#define mwPlySetOutVol(ply, vol) (*(ply)->vtbl->SetOutVol)((ply), (vol))

/* each passes its call on to the manager's: mwMngStartFrame, mwMngEntryErrFn,
 * mwMngExecServer, mwMngInit and mwMngFinish */
void mwPlyStartFrame(void);
void mwPlyEntryErrFunc(MW_PLY_ERRFN errfn, void *obj);
void mwPlyExecServer(void);
void mwPlyInitSystem(void);
void mwPlyExitSystem(void);

void mwPlySh4CpyMem(void *destination, const void *source, Sint32 size);

#endif
