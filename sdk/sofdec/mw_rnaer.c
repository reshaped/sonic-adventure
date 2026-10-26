/* where the audio renderer and the sound ports report their errors
 * (mw_rna.h) */
#include <mw_rna.h>
#include <sg_mwply.h>
#include <sg_xpt.h> /* NOLINT(misc-include-cleaner): it gives NULL */

MW_PLY_ERRFN mwg_rna_err_func = NULL;
void *mwg_rna_err_obj = NULL;

void mwRnaErrEntryFunc(MW_PLY_ERRFN errfn, void *obj)
{
    mwg_rna_err_func = errfn;
    mwg_rna_err_obj = obj;
}

void mwRnaErrCallFunc(char *ecode)
{
    if (mwg_rna_err_func != NULL) {
        mwg_rna_err_func(mwg_rna_err_obj, ecode);
    }
}
