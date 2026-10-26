/* a memory block's download into the sound memory allocated to a bank */
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdBankDownload(SDMEMBLK handle, const SDE_DATA_TYPE bank_type,
                       const Sint8 bank_num)
{
    SDE_ERR err;
    Sint32 address;
    Sint32 size;

    err = sdSndMemGetBankStat(bank_type, bank_num, &address, &size);
    if (err != SDE_ERR_NOTHING) {
        goto error;
    }
    /* no sound memory is allocated to the bank */
    if (address == 0 || size <= 0) {
        err = SDE_ERR_UNKNOW_NUM;
        goto error;
    }
    err = sdMemBlkTransfer(handle, address);
    /* @hack redundant: a goto to the next label, whose test SHC keeps: TST
     * R14,R14 at 0x8c6187e0, ahead of the label's at 0x8c6187e2. without it,
     * 13 of 44 words differ */
    if (err != SDE_ERR_NOTHING) {
        goto error;
    }

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdBankDownload, err);
    }
    return err;
}
