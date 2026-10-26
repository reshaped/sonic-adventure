/* single-unit vibration calls, over the multi-unit calls */
#include <kaede.h>
#include <sg_pdvib.h>
#include <sg_xpt.h>

/* unit 1 at zero power */
static const PDS_VIBPARAM vib_stop = {1, 0, 0, 0, 0, {0, 0, 0}};

Sint32 pdVibMxIsReady(Uint32 port)
{
    return pdVibIsReady(port);
}

Sint32 pdVibMxSetStopTime(Uint32 port, Uint32 time)
{
    Uint32 word;
    Uint8 *byte;
    Sint32 err;

    if (!pdVibMxIsReady(port)) {
        return PDD_VIBERR_NO_VIBRATOR;
    }
    /* the block pdVibSetStopTime writes for unit 1 alone: the mask's bytes, 0
     * and 2, then the time */
    /* @hack type: a byte cursor through word, which SHC steps in R4 for the
     * three stores (0x8c11b934 to 0x8c11b948 in 1ST_READ.BIN); as word = 2 << 8
     * | (Uint8)time << 16, the function runs 8 bytes short and 34 of 49 words
     * differ */
    word = 0;
    byte = (Uint8 *)&word;
    *byte++ = 0;
    *byte++ = 2;
    *byte++ = time;
    /* @hack spelling: the result through err, which SHC moves to R4 to test it
     * (MOV R0,R4 at 0x8c11b95a in 1ST_READ.BIN); tested directly, the function
     * runs 4 bytes short and 14 of 49 words differ */
    err = kdSetBlockWrite(port, KDD_FUNC_VIBRATION, 0, 0, 0, &word, 1);
    if (err < 0) {
        return PDD_VIBERR_BUSY;
    }
    return PDD_VIBERR_OK;
}

Sint32 pdVibMxStart(Uint32 port, const PDS_VIBPARAM *param)
{
    if (!pdVibMxIsReady(port)) {
        return PDD_VIBERR_NO_VIBRATOR;
    }
    return pdVibStart(port, param, 1);
}

Sint32 pdVibMxStop(Uint32 port)
{
    if (!pdVibMxIsReady(port)) {
        return PDD_VIBERR_NO_VIBRATOR;
    }
    return pdVibStart(port, &vib_stop, 1);
}
