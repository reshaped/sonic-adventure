/* ADX talk initialization and shutdown */
#include <sg_chain.h>
#include <sg_xpt.h>
#include <sj.h>
#include <sofdec.h>
#include <string.h>

/* the server's priority on the vertical blank chain */
#define SERVER_PRIORITY 0x70

char adxt_obj_mark[] = "MARK:adxt_obj";
ADX_TALK adxt_obj[ADXT_MAX_OBJ] = {0};
/* the server's entry on the vertical blank chain, or 0 for none */
SYCHAIN adxt_chain = 0;
/* whether ADXT_Init adds the server to the vertical blank chain */
Sint32 adxt_vsync_svr_flag = true;
/* how many more times ADXT_Init has run than ADXT_Finish */
Sint32 adxt_init_cnt = 0;

void ADXT_ConfigVsyncSvr(Sint32 flag)
{
    adxt_vsync_svr_flag = flag;
}

void ADXT_Init(void)
{
    ADXCRS_Init();
    ADXRNA_Init();
    ADXCRS_Lock();
    ADXF_Init();
    SJRBF_Init();
    SJMEM_Init();
    ADXSTM_Init();
    ADXSJD_Init();
    ADXERR_Init();
    memset(adxt_obj, 0, sizeof(adxt_obj));
    if (adxt_vsync_svr_flag == true && adxt_chain == 0) {
        /* the image registers ADXT_ExecServer itself, though it takes no
         * argument (0x8c65b574). syChainMasterHandlerPVR_V calls it with the
         * NULL registered here in R4 (0x8c65b576, 0x8c65f26e). the server
         * never reads that argument: its first use of R4 is a write
         * (0x8c65bc10) */
        adxt_chain = syChainAddHandler(SYD_CHAIN_EVENT_IML6_VBLANK,
                                       (void (*)(void *))ADXT_ExecServer,
                                       SERVER_PRIORITY, NULL);
    }
    ADXCRS_Unlock();
    adxt_init_cnt++;
}

void ADXT_Reset(void)
{
    ADXT_Init();
}

void ADXT_Finish(void)
{
    ADXCRS_Lock();
    if (adxt_chain != 0) {
        syChainDeleteHandler(adxt_chain);
        adxt_chain = 0;
    }
    ADXRNA_Finish();
    ADXSJD_Finish();
    ADXSTM_Finish();
    ADXCRS_Unlock();
    adxt_init_cnt--;
}
