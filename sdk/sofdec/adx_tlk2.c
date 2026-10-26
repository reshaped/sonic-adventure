/* ADX talk starts from a disc file, an AFS archive's file, or ADX data in
 * memory, alone or in an ACX archive */
#include <sg_xpt.h>
#include <sj.h>
#include <sofdec.h>

/* a memory joint's size, larger than any ADX data */
#define ADXT_MAX_DATASIZE 0x40000000

/* x with its four bytes reversed, which reads a big-endian word */
#define ADX_SWAP32(x)                                                          \
    ((((x) >> 24) & 0xff) | (((x) >> 8) & 0xff00) | (((x) << 8) & 0xff0000) |  \
     ((x) << 24))

/* an ACX archive's header, in big-endian words: the file count at ACX_NFILE,
 * and file no's offset from the archive at ACX_OFST(no) */
#define ACX_NFILE 1
#define ACX_OFST(no) (((no) + 1) * 2)

/* start file fid of partition patid, an AFS archive's file table */
void ADXT_StartAfs(ADXT adxt, Sint32 patid, Sint32 fid)
{
    Sint32 ofst;
    Sint32 nsct;

    ADXT_Stop(adxt);
    ADXCRS_Lock();
    ADXF_GetFileRange(patid, fid, &ofst, &nsct);
    /* @hack spelling: stm set in the test, which tests ADXSTM_OpenRange's R0
     * itself (TST R0,R0 at 0x8c65bdac). set before the test, SHC tests a copy
     * in R2, the function is 2 bytes longer and 30 of 50 words differ */
    if ((adxt->stm = ADXSTM_OpenRange(ofst, nsct, adxt->sjf)) == NULL) {
        /* @bug logic: a failed ADXSTM_OpenRange unlocks twice, here and after
         * the if (JSR at 0x8c65bdb4 and 0x8c65bdd4), so the lock depth ends
         * one below where the start found it. inside one level of a caller's
         * lock, the second unlock restores the interrupt mask before the
         * caller unlocks */
        ADXCRS_Unlock();
        ADXERR_CallErrFunc("E8101202: can't open %d, %d (ADXT_StartAfs)\n",
                           patid, fid);
    } else {
        adxt_start_stm(adxt);
    }
    ADXCRS_Unlock();
}

void ADXT_StartFname(ADXT adxt, char *fname)
{
    Sint32 fad;
    Sint32 fsize;
    Sint32 ret;

    ADXT_Stop(adxt);
    ADXCRS_Lock();
    ret = ADXSTM_GetFad(fname, &fad);
    if (ret < 0) {
        ADXCRS_Unlock();
        ADXERR_CallErrFunc("E8101203: can't open '%s' (ADXT_StartFname)",
                           fname);
        return;
    }
    ret = ADXSTM_GetFsizeSct(fname, &fsize);
    if (ret < 0) {
        ADXCRS_Unlock();
        ADXERR_CallErrFunc("E8101204: can't open '%s' (ADXT_StartFname)",
                           fname);
        return;
    }
    /* @hack spelling: stm set in the test, which tests ADXSTM_OpenRange's R0
     * itself (TST R0,R0 at 0x8c65be44). set before the test, SHC tests a copy
     * in R2, adxt and fname trade R13 and R14, the function is 2 bytes longer
     * and 42 of 73 words differ */
    if ((adxt->stm = ADXSTM_OpenRange(fad, fsize, adxt->sjf)) == NULL) {
        /* @bug logic: a failed ADXSTM_OpenRange unlocks twice, here and after
         * the if (JSR @R12 at 0x8c65be4a and 0x8c65be60), as in
         * ADXT_StartAfs */
        ADXCRS_Unlock();
        ADXERR_CallErrFunc("E8101205: can't open '%s' (ADXT_StartFname)",
                           fname);
    } else {
        adxt_start_stm(adxt);
    }
    ADXCRS_Unlock();
}

void ADXT_StartMem(ADXT adxt, void *adxdat)
{
    SJ sj;

    ADXT_Stop(adxt);
    ADXCRS_Lock();
    sj = SJMEM_Create(adxdat, ADXT_MAX_DATASIZE);
    if (sj == NULL) {
        /* @bug logic: a failed SJMEM_Create unlocks twice, here and after the
         * if (JSR at 0x8c65be98, JMP at 0x8c65beb8), as ADXT_StartAfs does
         * after a failed ADXSTM_OpenRange */
        ADXCRS_Unlock();
        ADXERR_CallErrFunc("E8101207: can't create sj (ADXT_StartMem)\n");
    } else {
        adxt_start_sj(adxt, sj);
        adxt->pmode = ADXT_PMODE_MEM;
    }
    ADXCRS_Unlock();
}

/* start file no of the ACX archive at acx. a no outside the archive's files
 * stops the stream and starts nothing */
void ADXT_StartMemIdx(ADXT adxt, void *acx, Sint32 no)
{
    Sint32 *idx = acx;
    Sint32 ofst;
    char *adr;
    SJ sj;

    ADXT_Stop(adxt);
    if (no >= ADX_SWAP32(idx[ACX_NFILE]) || no < 0) {
        return;
    }
    ADXCRS_Lock();
    ofst = ADX_SWAP32(idx[ACX_OFST(no)]);
    adr = (char *)acx + ofst;
    sj = SJMEM_Create(adr, ADXT_MAX_DATASIZE);
    if (sj == NULL) {
        /* @bug logic: a failed SJMEM_Create unlocks twice, here and after the
         * if (JSR at 0x8c65bf54, JMP at 0x8c65bf78), as ADXT_StartAfs does
         * after a failed ADXSTM_OpenRange */
        ADXCRS_Unlock();
        ADXERR_CallErrFunc("E8101207: can't create sj (ADXT_StartMemIdx)");
    } else {
        adxt_start_sj(adxt, sj);
        adxt->pmode = ADXT_PMODE_MEM;
    }
    ADXCRS_Unlock();
}

char *adxt_build = "\nADXT Ver.5.34 Build:Dec 03 1998 17:58:47\n";
