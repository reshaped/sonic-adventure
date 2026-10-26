/* GSC's calls through its library interface, mwg_gsc_i */
#include <sg_ov.h>
#include <sg_xpt.h>
#include <sj.h>

#include "movie/sdk/gsc.h"

UUID mwg_gsc_lid = GSC_LID;
OVLIB mwg_gsc_ovl = NULL;
struct gsc_lif *mwg_gsc_i = NULL;

Sint32 mwGscInitOvl(Sint32 nhn, void *work)
{
    /* @hack spelling: Init's result through err, which makes the call a JSR
     * (0x8ced1c98); returned straight, SHC jumps to it and 12 of 40 words
     * differ */
    Sint32 err;

    if (mwg_gsc_ovl != NULL) {
        return 0;
    }
    mwg_gsc_ovl = ovLibOpen(&mwg_gsc_lid);
    if (mwg_gsc_ovl == NULL) {
        return GSC_ERR_NOLIB;
    }
    mwg_gsc_i = (struct gsc_lif *)ovLibGetLibIf(mwg_gsc_ovl);
    err = mwg_gsc_i->Init(nhn, work);
    return err;
}

Sint32 mwGscFinish(void)
{
    Sint32 err;

    if (mwg_gsc_ovl == NULL) {
        return GSC_ERR_NOLIB;
    }
    err = mwg_gsc_i->Finish();
    if (err != 0) {
        return err;
    }
    ovLibClose(&mwg_gsc_lid);
    mwg_gsc_ovl = NULL;
    mwg_gsc_i = NULL;
    return err;
}

void mwGscExecServer(void)
{
    if (mwg_gsc_i != NULL) {
        mwg_gsc_i->ExecServer();
    }
}

struct gsc_stream *mwGscOpenFname(char *fname, SJ sj)
{
    /* @hack spelling: OpenFname's result through hn, which makes the call a
     * JSR (0x8ced1d0c); returned straight, SHC jumps to it and 15 of 16 words
     * differ */
    struct gsc_stream *hn;

    if (mwg_gsc_i == NULL) {
        return NULL;
    }
    hn = mwg_gsc_i->OpenFname(fname, sj);
    return hn;
}

struct gsc_stream *mwGscOpenRange(Sint32 stsct, Sint32 nsct, SJ sj)
{
    /* @hack spelling: OpenRange's result through hn, which makes the call a
     * JSR (0x8ced1d2c); returned straight, SHC jumps to it and 15 of 16 words
     * differ */
    struct gsc_stream *hn;

    if (mwg_gsc_i == NULL) {
        return NULL;
    }
    hn = mwg_gsc_i->OpenRange(stsct, nsct, sj);
    return hn;
}

Sint32 mwGscClose(struct gsc_stream *hn)
{
    /* @hack spelling: Close's result through err, which makes the call a JSR
     * (0x8ced1d4c); returned straight, SHC jumps to it and 15 of 16 words
     * differ */
    Sint32 err;

    if (mwg_gsc_i == NULL) {
        return GSC_ERR_NOLIB;
    }
    err = mwg_gsc_i->Close(hn);
    return err;
}
