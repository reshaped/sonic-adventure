/* SDK 1.20J GD file system calls through the overlay library interface. unlike
 * 1.30J's, it has no gdFsIsTrayOpen */
#include <sg_ov.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "loader/sdk/gdfs.h"

static const UUID gdfs_lid = GDFS_LID;
static OVLIB gdfs_ovl = NULL;
static const struct gdfs_lif *gdfs_i = NULL;

Sint32 gdFsReadW(Sint32 fad, Sint32 nsct, void *buf)
{
    if (gdfs_i != NULL) {
        return gdfs_i->ReadW(fad, nsct, buf);
    }
    return 0;
}

Sint32 gdFsSetGlobalErr(Sint32 err)
{
    if (gdfs_i != NULL) {
        return gdfs_i->SetGlobalErr(err);
    }
    return 0;
}

Sint32 gdFsGetGlobalErr(void)
{
    if (gdfs_i != NULL) {
        return gdfs_i->GetGlobalErr();
    }
    return 0;
}

Sint32 gdFsInitOvl(Sint32 max_open, void *gdfs_work, Sint32 max_dirent,
                   void *dirbuf)
{
    Sint32 err;

    if (gdfs_ovl == NULL) {
        /* @hack spelling: the open's result is assigned in the test. SHC tests
         * it in R0 (TST R0,R0 at 0x8ce08b8a), and stores it in the branch's
         * delay slot. assigned first, it is copied to R2 to be tested, and 24
         * of 40 words differ */
        if ((gdfs_ovl = ovLibOpen((UUID *)&gdfs_lid)) == NULL) {
            return GDD_ERR_LIBOV;
        }
        gdfs_i = (const struct gdfs_lif *)ovLibGetLibIf(gdfs_ovl);
    }
    /* @hack spelling: Init's result goes through err, which makes the call a
     * JSR (0x8ce08bb0). returned straight, SHC jumps to Init, and 9 of 40
     * words differ */
    err = gdfs_i->Init(max_open, gdfs_work, max_dirent, dirbuf);
    return err;
}

Sint32 gdFsReinit(void)
{
    if (gdfs_i != NULL) {
        return gdfs_i->Reinit();
    }
    return GDD_ERR_INIT;
}

void gdFsFinish(void)
{
    if (gdfs_ovl != NULL) {
        gdfs_i->Finish();
        ovLibClose((UUID *)&gdfs_lid);
        gdfs_ovl = NULL;
        gdfs_i = NULL;
    }
}

GDFS gdFsCreateHn(GDFS gdfs, Sint32 fid, Sint32 fad, Sint32 fsize)
{
    if (gdfs_i != NULL) {
        return gdfs_i->CreateHn(gdfs, fid, fad, fsize);
    }
    return NULL;
}

Sint32 gdFsLoadDir(const char *dirname, GDFS_DIRREC gf_dirrec)
{
    if (gdfs_i != NULL) {
        return gdfs_i->LoadDir(dirname, gf_dirrec);
    }
    return 0;
}

Sint32 gdFsLoadDirFast(const char *dirname, GDFS_DIRREC gf_dirrec)
{
    if (gdfs_i != NULL) {
        return gdfs_i->LoadDirFast(dirname, gf_dirrec);
    }
    return 0;
}

Sint32 gdFsSetDir(GDFS_DIRREC gf_dirrec)
{
    if (gdfs_i != NULL) {
        return gdfs_i->SetDir(gf_dirrec);
    }
    return 0;
}

Sint32 gdFsGetDirInfo(const char *name, GDFS_DIRINFO *dirinfo)
{
    if (gdfs_i != NULL) {
        return gdfs_i->GetDirInfo(name, dirinfo);
    }
    return 0;
}

GDFS gdFsOpen(const char *fname, GDFS_DIRREC gf_dirrec)
{
    if (gdfs_i != NULL) {
        return gdfs_i->Open(fname, gf_dirrec);
    }
    return NULL;
}

GDFS gdFsOpenFast(const char *fname, GDFS_DIRREC gf_dirrec)
{
    if (gdfs_i != NULL) {
        return gdfs_i->OpenFast(fname, gf_dirrec);
    }
    return NULL;
}

GDFS gdFsOpenRange(Sint32 stsct, Sint32 nsct)
{
    if (gdfs_i != NULL) {
        return gdfs_i->OpenRange(stsct, nsct);
    }
    return NULL;
}

GDFS gdFsGetWorkHn(void)
{
    if (gdfs_i != NULL) {
        return gdfs_i->GetWorkHn();
    }
    return NULL;
}

GDFS gdFsGetSysHn(void)
{
    if (gdfs_i != NULL) {
        return gdfs_i->GetSysHn();
    }
    return NULL;
}

void gdFsExecServer(void)
{
    if (gdfs_i != NULL) {
        gdfs_i->ExecServer();
    }
}

void gdFsEntryErrFuncAll(GDFS_ERRFUNC erfunc, void *obj)
{
    if (gdfs_i != NULL) {
        gdfs_i->EntryErrFuncAll(erfunc, obj);
    }
}

Sint32 gdFsGetToc(Sint32 type, void *buf)
{
    if (gdfs_i != NULL) {
        return gdfs_i->GetToc(type, buf);
    }
    return 0;
}

Sint32 gdFsGetDrvStat(void)
{
    if (gdfs_i != NULL) {
        return gdfs_i->GetDrvStat();
    }
    return 0;
}

Sint32 gdFsReqDrvStat(void)
{
    if (gdfs_i != NULL) {
        return gdfs_i->ReqDrvStat();
    }
    return 0;
}

Sint32 gdFsDaPlay(Sint32 st_track, Sint32 end_track, Sint32 reptime)
{
    if (gdfs_i != NULL) {
        return gdfs_i->DaPlay(st_track, end_track, reptime);
    }
    return 0;
}

Sint32 gdFsDaPlaySct(Sint32 st_sct, Sint32 end_sct, Sint32 reptime)
{
    if (gdfs_i != NULL) {
        return gdfs_i->DaPlaySct(st_sct, end_sct, reptime);
    }
    return 0;
}

Sint32 gdFsDaStop(void)
{
    if (gdfs_i != NULL) {
        return gdfs_i->DaStop();
    }
    return 0;
}

Sint32 gdFsDaPause(void)
{
    if (gdfs_i != NULL) {
        return gdfs_i->DaPause();
    }
    return 0;
}

Sint32 gdFsDaRelease(Sint32 reptime)
{
    if (gdfs_i != NULL) {
        return gdfs_i->DaRelease(reptime);
    }
    return 0;
}

Sint32 gdFsDaGetInfo(GDFS_DAINFO *dainfo)
{
    if (gdfs_i != NULL) {
        return gdfs_i->DaGetInfo(dainfo);
    }
    return 0;
}
