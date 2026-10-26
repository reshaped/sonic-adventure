#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdSndMemAllocateBank(const SDE_DATA_TYPE data_type, int bank_num,
                             Sint32 allocate_ptr, Sint32 allocate_size)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    struct sd_work *work = sdgWrkPtr;
    /* the bank's entry in the driver's data map, as a sound memory offset that
     * is never dereferenced */
    struct sd_bank *driver_bank;
    struct sd_bank *bank;

    if (work == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    /* @hack type: bank_num narrowed to a Sint8 at each use, which
     * sign-extends it from R5 each time (EXTS.B R5,R7 at 0x8c65de74, EXTS.B
     * R5,R11 at 0x8c65de82); through a Sint8 local, SHC copies it to R14 first
     * and 87 of 159 words differ */
    if ((Sint8)bank_num < 0) {
        err = SDE_ERR_BANK_ILLEGAL_NUM;
        goto error;
    }
    switch (data_type) {
    case SDE_DATA_TYPE_MIDI_SEQ_BANK:
        if ((Sint8)bank_num >= SD_BANK_MAX) {
            err = SDE_ERR_BANK_ILLEGAL_NUM;
            goto error;
        }
        driver_bank =
            &((struct sd_banks *)work->data_map)->midi_seq[(Sint8)bank_num];
        bank = &work->banks.midi_seq[(Sint8)bank_num];
        break;
    case SDE_DATA_TYPE_MIDI_PRG_BANK:
        if ((Sint8)bank_num >= SD_BANK_MAX) {
            err = SDE_ERR_BANK_ILLEGAL_NUM;
            goto error;
        }
        driver_bank =
            &((struct sd_banks *)work->data_map)->midi_prg[(Sint8)bank_num];
        bank = &work->banks.midi_prg[(Sint8)bank_num];
        break;
    case SDE_DATA_TYPE_SHOT_BANK:
        if ((Sint8)bank_num >= SD_BANK_MAX) {
            err = SDE_ERR_BANK_ILLEGAL_NUM;
            goto error;
        }
        driver_bank =
            &((struct sd_banks *)work->data_map)->shot[(Sint8)bank_num];
        bank = &work->banks.shot[(Sint8)bank_num];
        break;
    case SDE_DATA_TYPE_PSTM_RING_BUF:
        if ((Sint8)bank_num >= SD_BANK_MAX) {
            err = SDE_ERR_BANK_ILLEGAL_NUM;
            goto error;
        }
        driver_bank =
            &((struct sd_banks *)work->data_map)->pstm_ring[(Sint8)bank_num];
        bank = &work->banks.pstm_ring[(Sint8)bank_num];
        break;
    case SDE_DATA_TYPE_FX_PRG_BANK:
        if ((Sint8)bank_num >= 1) {
            err = SDE_ERR_BANK_ILLEGAL_NUM;
            goto error;
        }
        driver_bank = &((struct sd_banks *)work->data_map)->fx_prg;
        bank = &work->banks.fx_prg;
        break;
    case SDE_DATA_TYPE_FX_OUT_BANK:
        if ((Sint8)bank_num >= 1) {
            err = SDE_ERR_BANK_ILLEGAL_NUM;
            goto error;
        }
        driver_bank = &((struct sd_banks *)work->data_map)->fx_out;
        bank = &work->banks.fx_out;
        break;
    case SDE_DATA_TYPE_FX_PRG_WRK:
        if ((Sint8)bank_num >= 1) {
            err = SDE_ERR_BANK_ILLEGAL_NUM;
            goto error;
        }
        driver_bank = &((struct sd_banks *)work->data_map)->fx_wrk;
        bank = &work->banks.fx_wrk;
        break;
    case SDE_DATA_TYPE_SND_DRV:
    case SDE_DATA_TYPE_MULTI_UNIT:
    case SDE_DATA_TYPE_MIDI_SEQ_DATA:
    case SDE_DATA_TYPE_SHOT_DATA:
    case SDE_DATA_TYPE_PSTM_DATA:
    default:
        err = SDE_ERR_DATA_ILLEGAL_TYPE;
        goto error;
    }
    sdSndMemWriteSint32((Sint32)&driver_bank->address, allocate_ptr);
    sdSndMemWriteSint32((Sint32)&driver_bank->size, allocate_size);
    bank->address = allocate_ptr;
    bank->size = allocate_size;

error:
    /* @hack redundant: a store of SDE_ERR_NOTHING to err that never runs;
     * without it, err and the bank stride swap R12 and R11 and 13 of 159 words
     * differ */
    if (0) {
        err = SDE_ERR_NOTHING;
    }
    return err;
}
