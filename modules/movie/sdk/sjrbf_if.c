/* the ring buffer joint library's interface pointer, and the calls that open,
 * finish and serve the library through the overlay library */
#include <sg_ov.h>
#include <sj.h>

#include "movie/sdk/sjrbf.h"

UUID mwg_sj_rbf_lid = SJRBF_LID;
OVLIB mwg_sj_rbf_ovl = NULL; /* the library mwSjRbfInitOvl opened, or NULL */
SJ_IF *mwg_sj_rbf_i = NULL;

void mwSjRbfInitOvl(void)
{
    if (mwg_sj_rbf_ovl == NULL) {
        mwg_sj_rbf_ovl = ovLibOpen(&mwg_sj_rbf_lid);
        if (mwg_sj_rbf_ovl == NULL) {
            return;
        }
        mwg_sj_rbf_i = (SJ_IF *)ovLibGetLibIf(mwg_sj_rbf_ovl);
        ((struct sj_rbf_lif *)mwg_sj_rbf_i)->Init();
    }
}

void mwSjRbfFinish(void)
{
    if (mwg_sj_rbf_ovl == NULL) {
        return;
    }
    ((struct sj_rbf_lif *)mwg_sj_rbf_i)->Finish();
    ovLibClose(&mwg_sj_rbf_lid);
    mwg_sj_rbf_ovl = NULL;
    mwg_sj_rbf_i = NULL;
}

void mwSjRbfExecServer(void)
{
    if (mwg_sj_rbf_i == NULL) {
        return;
    }
    ((struct sj_rbf_lif *)mwg_sj_rbf_i)->ExecServer();
}
