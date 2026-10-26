#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdSndMemGetBankStat(const SDE_DATA_TYPE data_type, const Sint8 bank_num,
                            Sint32 *allocate_ptr, Sint32 *allocate_size)
{
    struct sd_work *work = sdgWrkPtr;
    SDE_ERR err = SDE_ERR_NOTHING;
    struct sd_bank *bank;

    /* @hack spelling: the second test, of the two pointers, joins its
     * comparisons with |, where || would do. SHC sets each with MOVT and joins
     * them with OR (0x8c65e04c to 0x8c65e052). with ||, the function runs 8
     * bytes short and 99 of 117 words differ */
    if (work == NULL) {
        err = SDE_ERR_NO_INIT;
    } else if ((allocate_ptr == NULL) | (allocate_size == NULL)) {
        err = SDE_ERR_PRM_OVER_RANGE;
    } else if (bank_num < 0) {
        err = SDE_ERR_BANK_ILLEGAL_NUM;
        goto error;
    } else {
        switch (data_type) {
        case SDE_DATA_TYPE_MIDI_SEQ_BANK:
            if (bank_num >= SD_BANK_MAX) {
                err = SDE_ERR_BANK_ILLEGAL_NUM;
                goto error;
            }
            bank = &work->banks.midi_seq[bank_num];
            break;
        case SDE_DATA_TYPE_MIDI_PRG_BANK:
            if (bank_num >= SD_BANK_MAX) {
                err = SDE_ERR_BANK_ILLEGAL_NUM;
                goto error;
            }
            bank = &work->banks.midi_prg[bank_num];
            break;
        case SDE_DATA_TYPE_SHOT_BANK:
            if (bank_num >= SD_BANK_MAX) {
                err = SDE_ERR_BANK_ILLEGAL_NUM;
                goto error;
            }
            bank = &work->banks.shot[bank_num];
            break;
        case SDE_DATA_TYPE_PSTM_RING_BUF:
            if (bank_num >= SD_BANK_MAX) {
                err = SDE_ERR_BANK_ILLEGAL_NUM;
                goto error;
            }
            bank = &work->banks.pstm_ring[bank_num];
            break;
        case SDE_DATA_TYPE_FX_PRG_BANK:
            if (bank_num != 0) {
                err = SDE_ERR_BANK_ILLEGAL_NUM;
                goto error;
            }
            bank = &work->banks.fx_prg;
            break;
        case SDE_DATA_TYPE_FX_OUT_BANK:
            if (bank_num != 0) {
                err = SDE_ERR_BANK_ILLEGAL_NUM;
                goto error;
            }
            bank = &work->banks.fx_out;
            break;
        case SDE_DATA_TYPE_FX_PRG_WRK:
            if (bank_num != 0) {
                err = SDE_ERR_BANK_ILLEGAL_NUM;
                goto error;
            }
            bank = &work->banks.fx_wrk;
            break;
        default:
            bank = NULL;
            err = SDE_ERR_DATA_ILLEGAL_TYPE;
        }
        /* @hack redundant: a goto to the next label, which has SHC copy the
         * bank's offset through R3 and its size through R2 (MOV.L @R4,R3 at
         * 0x8c65e126). without it, the two trade and 6 of 117 words differ */
        goto found;
    found:
        if (bank != NULL) {
            *allocate_ptr = bank->address;
            *allocate_size = bank->size;
        } else {
            if (allocate_ptr != NULL) {
                *allocate_ptr = 0;
            }
            if (allocate_size != NULL) {
                *allocate_size = -1;
            }
        }
    }

error:
    return err;
}
