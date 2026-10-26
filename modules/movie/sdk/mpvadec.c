/* read MPEG video's headers, from the sequence header to the slice header, and
 * decode the macroblocks of I, P and B pictures */
#include <sg_xpt.h>
#include <sofdec.h>

#include "movie/sdk/mpv.h"

/* the error codes for an invalid handle: MPV_DecodePicAtr's and
 * MPV_GetPicAtr's, then MPV_GetBitRate's */
#define MPV_ERR_PICATR_HN ((Sint32)0xff03020c)
#define MPV_ERR_GETBITRATE_HN ((Sint32)0xff03020d)

/* mpvdec_GoNextLayer's result when the data ends before a start code, or the
 * start code is of a kind its mask leaves out */
#define NO_LAYER (-1)
/* the increment the macroblock decoders take for bits that are no address
 * increment code, and for an increment past the picture's last macroblock */
#define NO_MB (-2)

/* the rows of a table indexed by a picture's coding type: an unused row 0,
 * then MPV_PTYPE_I to MPV_PTYPE_D */
#define PTYPE_ROWS (MPV_PTYPE_D + 1)

/* the index in mpv->make of the maker for an MPV_MB_* type's directions.
 * MPV_MB_BACKWARD and MPV_MB_FORWARD are bits 2 and 3, so the index is 1 for
 * backward, 2 for forward and 3 for both */
#define MAKER_INDEX(type) (((type) & (MPV_MB_BACKWARD | MPV_MB_FORWARD)) >> 2)

/* a start code opens with 23 zero bits. MPEG video keeps a run that long out
 * of a slice's coded data. a peek of 23 bits that finds them all zero
 * therefore means the slice has ended */
#define START_CODE_ZEROS 23

/* the value mpvdec_ResetDc gives each DC prediction: a pixel value of 128, at
 * the DC coefficient's scale of 8 */
#define DC_PRED_RESET (128 * 8)

/* the mpvdec_ functions declared below that take data, size and used read the
 * stream at *data. *size counts the bytes from *data on, and *used the bytes
 * stepped past so far. each steps *data past what it reads, takes as much from
 * *size and adds it to *used */
Sint32 mpvdec_DecShc(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                     Sint32 *used);
void mpvdec_InitIqm(struct mpv_obj *mpv);
void mpvdec_InitNqm(struct mpv_obj *mpv);
void mpvdec_InitMbAdr2Pos(struct mpv_obj *mpv);
Sint32 mpvdec_DecGsc(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                     Sint32 *used);
Sint32 mpvdec_DecPsc(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                     Sint32 *used);
Sint32 mpvdec_GoNextLayer(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                          Sint32 *used, Sint32 mask);
Sint32 mpvdec_GoNextDelim(Uint8 **data, Sint32 *size, Sint32 *used);
Sint32 mpvdec_Recover(struct mpv_obj *mpv, Sint32 mask, Uint8 **data,
                      Sint32 *size, Sint32 *used);
Sint32 mpvdec_DecSlice(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                       Sint32 *used);
Sint32 mpvdec_DecIpicMb(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                        Sint32 *used);
Sint32 mpvdec_DecPpicMb(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                        Sint32 *used);
void mpvdec_ResetMv(struct mpv_motion *motion);
void mpvdec_ResetDc(struct mpv_obj *mpv);
Sint32 mpvdec_MotionSub(struct mpv_obj *mpv, struct mpv_motion *motion,
                        Sint32 *vector, Sint32 *predict);
Sint32 mpvdec_DecBpicMb(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                        Sint32 *used);
Sint32 mpvdec_DecDpicMb(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                        Sint32 *used);
void mpvdec_WaitCnt(Sint32 count);

/* the handlers mpvdec_DecPsc sets for a picture. mpvdec_mb_decoders is indexed
 * by the picture's coding type. mpvdec_intra_blocks and mpvdec_nintra_blocks
 * are indexed by MPV_COND_DECODE's value. the other tables take that value
 * first and the coding type second. mpvdec_intra_makers takes
 * MPV_COND_INTRA_TA's value ahead of both */
static Sint32 (*const mpvdec_mb_decoders[PTYPE_ROWS])(struct mpv_obj *mpv,
                                                      Uint8 **data,
                                                      Sint32 *size,
                                                      Sint32 *used) = {
    NULL,
    mpvdec_DecIpicMb,
    mpvdec_DecPpicMb,
    mpvdec_DecBpicMb,
    mpvdec_DecDpicMb,
};
static void (*const mpvdec_skippers[2][PTYPE_ROWS])(struct mpv_obj *mpv,
                                                    Sint32 count) = {
    {NULL, NULL, MPVMMC_PpicSkipped, MPVMMC_BpicSkipped, NULL},
    {NULL, NULL, MPVCMC_PpicSkipped, MPVCMC_BpicSkipped, NULL},
};
static Sint32 (*const mpvdec_intra_blocks[2])(struct mpv_obj *mpv) = {
    MPVMDEC_IntraBlocks,
    MPVCDEC_IntraBlocks,
};
static Sint32 (*const mpvdec_nintra_blocks[2])(struct mpv_obj *mpv) = {
    MPVMDEC_NintraBlocks,
    MPVCDEC_NintraBlocks,
};
static void (*const mpvdec_intra_makers[2][2][PTYPE_ROWS])(
    struct mpv_obj *mpv) = {
    {
        {NULL, NULL, NULL, NULL, NULL},
        {NULL, MPVCMC_IntraRt, MPVCMC_IntraRt, MPVCMC_IntraTa, NULL},
    },
    {
        {NULL, NULL, NULL, NULL, NULL},
        {NULL, MPVCMC_IntraTa, MPVCMC_IntraTa, MPVCMC_IntraTa, NULL},
    },
};
static void (*const mpvdec_forward_makers[2][PTYPE_ROWS])(
    struct mpv_obj *mpv) = {
    {NULL, NULL, NULL, NULL, NULL},
    {NULL, NULL, MPVCMC_ForwardRt, MPVCMC_ForwardTa, NULL},
};
static void (*const mpvdec_backward_makers[2][PTYPE_ROWS])(
    struct mpv_obj *mpv) = {
    {NULL, NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, MPVCMC_Backward, NULL},
};
static void (*const mpvdec_bidirect_makers[2][PTYPE_ROWS])(
    struct mpv_obj *mpv) = {
    {NULL, NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, MPVCMC_BiDirect, NULL},
};

/* the macros below read the stream through four locals of their caller:
 * word, next, taken and cursor. the four hold what struct mpv_bits' word,
 * next, used and cursor hold, with cursor as a byte pointer. GET_READER loads
 * them from mpv->bits, and PUT_READER stores them back. word is the word at
 * cursor - 8 shifted left by taken, and next is the word at cursor - 4. the
 * first byte with no bit taken is then at cursor - 8 + (taken + 7) / 8 */

/* load the big-endian word at cursor into dst, and step cursor past it */
/* @hack spelling: cursor = cursor + 1, which SHC compiles differently from
 * cursor++: as cursor++, 87 of mpvdec_DecIpicMb's 234 words differ and 29 of
 * mpvdec_MotionSub's 165 */
/* @hack type: the first byte read as an Sint8, whose sign bits the shifts
 * push out of dst. SHC then loads it with MOV.B and shifts it with no EXTU.B,
 * as at 0x8ced5e00 to 0x8ced5e06. read as a Uint8, the readers run long, the
 * PSG by 70 bytes and PSGSFD02 by 16, and 570 of mpvdec_DecShc's 607 words
 * differ */
#define LOAD_WORD(dst)                                                         \
    {                                                                          \
        (dst) = (Sint8)cursor[0]; /* NOLINT(bugprone-signed-char-misuse) */    \
        cursor = cursor + 1;                                                   \
        (dst) <<= 8;                                                           \
        (dst) |= *cursor;                                                      \
        cursor = cursor + 1;                                                   \
        (dst) <<= 8;                                                           \
        (dst) |= *cursor;                                                      \
        cursor = cursor + 1;                                                   \
        (dst) <<= 8;                                                           \
        (dst) |= *cursor;                                                      \
        cursor = cursor + 1;                                                   \
    }

/* set the reader to the first bit after the 4-byte start code at *stream.
 * word takes the aligned word that holds that bit, shifted past the bits
 * before it, and next the word after it */
/* @hack type: *stream read a second time, as an Sint32, which C89 leaves
 * undefined. SHC then loads the pointer twice, MOV.L @R9 at 0x8ced5df4 and
 * 0x8ced5df8. read once, mpvdec_DecGsc and mpvdec_DecPsc run 4 bytes long
 * each, and 391 of mpvdec_DecShc's 607 words differ, 154 of mpvdec_DecGsc's
 * 167 and 429 of mpvdec_DecPsc's 505 */
#define OPEN_READER(stream)                                                    \
    {                                                                          \
        cursor = (Uint8 *)(((Uint32)(stream)[0] + 4) & ~3UL);                  \
        taken = *(Sint32 *)(stream) + 4;                                       \
        taken -= (Sint32)cursor;                                               \
        taken *= 8;                                                            \
        LOAD_WORD(word);                                                       \
        word <<= taken;                                                        \
        LOAD_WORD(next);                                                       \
    }

/* take the next width bits, 1 to 32, into destination */
#define TAKE_FIELD(width, destination)                                         \
    {                                                                          \
        if (taken >= 32 - (width)) {                                           \
            taken -= 32 - (width);                                             \
            if (taken != 0) {                                                  \
                word |= next >> ((width) - taken);                             \
                (destination) = word >> (32 - (width));                        \
                word = next << taken;                                          \
            } else {                                                           \
                (destination) = word >> (32 - (width));                        \
                word = next;                                                   \
            }                                                                  \
            LOAD_WORD(next);                                                   \
        } else {                                                               \
            taken += (width);                                                  \
            (destination) = word >> (32 - (width));                            \
            word <<= (width);                                                  \
        }                                                                      \
    }

/* copy the next width bits, 1 to 32, into destination without taking them */
#define PEEK_FIELD(width, destination)                                         \
    {                                                                          \
        (destination) = word >> (32 - (width));                                \
        if (taken > 32 - (width)) {                                            \
            (destination) |= next >> (64 - (width) - taken);                   \
        }                                                                      \
    }

/* take the next width bits and drop them */
#define SKIP_BITS(width)                                                       \
    {                                                                          \
        taken += (width);                                                      \
        if (taken >= 32) {                                                     \
            taken -= 32;                                                       \
            word = next << taken;                                              \
            LOAD_WORD(next);                                                   \
        } else {                                                               \
            word <<= (width);                                                  \
        }                                                                      \
    }

/* take the next bit into destination, as 1 or 0 */
/* @hack spelling: word = word << 1, which SHC compiles differently from
 * word <<= 1: as word <<= 1, 57 of mpvdec_DecGsc's 167 words differ */
#define TAKE_FLAG(destination)                                                 \
    {                                                                          \
        (destination) = (Sint32)word < 0;                                      \
        if (taken == 31) {                                                     \
            taken = 0;                                                         \
            word = next;                                                       \
            LOAD_WORD(next);                                                   \
        } else {                                                               \
            taken++;                                                           \
            word = word << 1;                                                  \
        }                                                                      \
    }

#define GET_READER(mpv)                                                        \
    {                                                                          \
        word = (mpv)->bits.word;                                               \
        next = (mpv)->bits.next;                                               \
        taken = (mpv)->bits.used;                                              \
        cursor = (Uint8 *)(mpv)->bits.cursor;                                  \
    }

#define PUT_READER(mpv)                                                        \
    {                                                                          \
        (mpv)->bits.word = word;                                               \
        (mpv)->bits.next = next;                                               \
        (mpv)->bits.used = taken;                                              \
        (mpv)->bits.cursor = (const Uint32 *)cursor;                           \
    }

Sint32 MPV_DecodePicAtr(struct mpv_obj *mpv, struct mpv_stream *stm,
                        Sint32 *used)
{
    /* @hack spelling: hn, a copy of mpv, which SHC sets in the delay slot of
     * the call to MPVLIB_CheckHn, MOV R4,R13 at 0x8ced5d20; through mpv, 85 of
     * the 93 words differ */
    struct mpv_obj *hn = mpv;
    Uint8 *data;
    Sint32 size;
    Sint32 code;
    Sint32 kind;

    if (MPVLIB_CheckHn(hn) != 0) {
        return MPVERR_SetCode(NULL, MPV_ERR_PICATR_HN);
    }
    data = stm->data;
    size = stm->size;
    *used = 0;
    for (;;) {
        code = mpvdec_Recover(
            hn, MPV_DELIM_SEQUENCE | MPV_DELIM_GOP | MPV_DELIM_PICTURE, &data,
            &size, used);
        if (code != 0) {
            return MPVERR_SetCode(hn, code);
        }
        kind = MPV_CheckDelim(data);
        /* each reader steps data on to the next start code, so a case that
         * continues starts the for loop over at that start code */
        switch (kind) {
        case MPV_DELIM_SEQUENCE:
            code = mpvdec_DecShc(hn, &data, &size, used);
            if (code == NO_LAYER) {
                continue;
            }
            if (code != 0) {
                goto done;
            }
            /* fall through */
        case MPV_DELIM_GOP:
            code = mpvdec_DecGsc(hn, &data, &size, used);
            if (code != 0) {
                continue;
            }
            /* fall through */
        case MPV_DELIM_PICTURE:
            code = mpvdec_DecPsc(hn, &data, &size, used);
            if (code != 0) {
                continue;
            }
            goto done;
        }
    }
done:
    return code;
}

/* read the sequence header at *data into mpv and mpv->picture. a quantizer
 * matrix the header leaves out takes its default. then set the picture's size
 * in macroblocks, mpv->last_index and mbpos, and step to the next start code
 * as mpvdec_GoNextLayer does. returns 0 at a group's header, else NO_LAYER */
Sint32 mpvdec_DecShc(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                     Sint32 *used)
{
    struct mpv_picture *picture;
    /* @hack redundant: register on cursor, taken, next, word, flag and value.
     * SHC then keeps data in R9, MOV R5,R9 at 0x8ced5de6, and 12 bytes of
     * locals, ADD #-12,R15 at 0x8ced5dea. without it, data goes to the frame,
     * the function runs 4 bytes long and 598 of its 607 words differ */
    register Uint8 *cursor;
    register Sint32 taken;
    register Uint32 next;
    register Uint32 word;
    register Sint32 flag;
    Sint32 i;
    register Uint32 value;
    Sint32 end;
    Sint32 bytes;

    picture = &mpv->picture;
    OPEN_READER(data);
    TAKE_FIELD(12, picture->width);
    TAKE_FIELD(12, picture->height);
    TAKE_FIELD(4, mpv->aspect_ratio);
    TAKE_FIELD(4, picture->rate);
    TAKE_FIELD(18, mpv->bit_rate);
    /* the marker bit, which is always 1 */
    SKIP_BITS(1);
    TAKE_FIELD(10, mpv->vbv_buffer_size);
    TAKE_FLAG(mpv->constrained);
    TAKE_FLAG(flag);
    if (flag) {
        for (i = 0; i < 64; i++) {
            TAKE_FIELD(8, value);
            mpv->intra_qmat[mpvbdec_zigzag[i]] = value;
        }
    } else {
        mpvdec_InitIqm(mpv);
    }
    TAKE_FLAG(flag);
    if (flag) {
        for (i = 0; i < 64; i++) {
            TAKE_FIELD(8, value);
            mpv->inter_qmat[mpvbdec_zigzag[i]] = value;
        }
    } else {
        mpvdec_InitNqm(mpv);
    }
    picture->mb_width = (picture->width + 15) >> 4;
    picture->mb_height = (picture->height + 15) >> 4;
    mpv->last_index = (picture->mb_height * picture->mb_width) - 1;
    mpvdec_InitMbAdr2Pos(mpv);
    /* @hack type: end as an Sint32, which holds the byte count and then the
     * address. SHC then copies taken, MOV R13,R5 at 0x8ced6262, ahead of
     * cursor's step back, ADD #-8,R14 at 0x8ced6266. as a Uint8 *, the two
     * trade places and 2 of the 607 words differ */
    end = (taken + 7) >> 3;
    cursor -= 8;
    end += (Sint32)cursor;
    bytes = end - (Sint32)*data;
    *used += bytes;
    *size -= bytes;
    *data = (Uint8 *)end;
    return mpvdec_GoNextLayer(mpv, data, size, used, MPV_DELIM_GOP);
}

/* copy the default intra quantizer matrix, mpvbdec_dfl_iqm, into
 * mpv->intra_qmat. SHC expands the call in mpvdec_DecShc */
void mpvdec_InitIqm(struct mpv_obj *mpv)
{
    /* @hack type: the destination cast to Uint32 *, with which mpvdec_DecShc's
     * refill after its second flag loads its bytes into R3, R2 and R3, at
     * 0x8ced61a4 to 0x8ced61ac. without it, R2 and R3 trade places there and
     * in the loop after, and 32 of mpvdec_DecShc's 607 words differ */
    UTY_MemcpyDword((Uint32 *)mpv->intra_qmat, mpvbdec_dfl_iqm, 16);
    return; /* NOLINT(readability-redundant-control-flow) */
}

/* fill mpv->inter_qmat with the default non-intra quantizer matrix, every
 * weight 16. SHC expands the call in mpvdec_DecShc */
void mpvdec_InitNqm(struct mpv_obj *mpv)
{
    UTY_MemsetDword(mpv->inter_qmat, 0x10101010, 16);
    /* @hack redundant: a return where the function returns anyway. with it,
     * mpvdec_DecShc computes the width in macroblocks in R2 and then the
     * height in R1, at 0x8ced623c to 0x8ced6250. without it, the two trade
     * places, mpvdec_DecShc loses MOV R1,R2 at 0x8ced624e and runs 2 bytes
     * short, and 50 of its 607 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

/* fill mbpos with each macroblock's row and column, at the picture's width in
 * macroblocks. every entry is filled, past the picture's last macroblock
 * too */
void mpvdec_InitMbAdr2Pos(struct mpv_obj *mpv)
{
    Sint32 row;
    Sint32 column;
    Sint32 width;
    Sint8 *pos;
    Sint8 *end;

    width = mpv->picture.mb_width;
    row = 0;
    column = 0;
    pos = (Sint8 *)&mpv->mbpos;
    end = pos + sizeof(mpv->mbpos);
    for (; pos < end; pos += 2, column++) {
        if (column >= width) {
            row++;
            column = 0;
        }
        pos[0] = row;
        pos[1] = column;
    }
}

/* read the group header at *data: its time code into mpv->picture, and its
 * closed_gop and broken_link flags into mpv. then step to the next start code
 * as mpvdec_GoNextLayer does. returns 0 at a picture's header, else
 * NO_LAYER */
Sint32 mpvdec_DecGsc(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                     Sint32 *used)
{
    struct mpv_picture *picture;
    /* @hack redundant: register on cursor, taken, next, word, code and end.
     * SHC then gives taken R11 and next R12, MOV.L @R5,R11 at 0x8ced632c and
     * MOV.B @R14+,R12 at 0x8ced6354. without it, the two trade places and 41
     * of the 167 words differ */
    register Uint8 *cursor;
    register Sint32 taken;
    register Uint32 next;
    register Uint32 word;
    register Uint32 code;
    register Uint8 *end;
    Sint32 bytes;

    picture = &mpv->picture;
    OPEN_READER(data);
    TAKE_FIELD(25, code);
    picture->frame = code & 63;
    code >>= 6;
    picture->second = code & 63;
    /* past the seconds and the marker bit above them */
    code >>= 7;
    picture->minute = code & 63;
    code >>= 6;
    picture->hour = code & 31;
    code >>= 5;
    picture->drop = code;
    TAKE_FLAG(mpv->closed_gop);
    TAKE_FLAG(mpv->broken_link);
    /* @hack spelling: 7 added to taken in place, before cursor steps back. SHC
     * then puts ADD #7,R11 at 0x8ced6442 ahead of ADD #-8,R14 at 0x8ced6446.
     * with the 7 added in the shift, the two trade places and 2 of the 167
     * words differ */
    taken += 7;
    cursor -= 8;
    end = cursor + (taken >> 3);
    bytes = end - *data;
    *used += bytes;
    *size -= bytes;
    *data = end;
    return mpvdec_GoNextLayer(mpv, data, size, used, MPV_DELIM_PICTURE);
}

/* read the picture header at *data: its temporal reference, coding type and
 * VBV delay. a P picture's header also gives the forward full_pel flag and
 * f_code, and a B picture's gives both directions'. then set mpv's handlers
 * for the picture, and step to the next start code as mpvdec_GoNextLayer
 * does. returns 0 at the picture's first slice, else NO_LAYER. where the
 * header's extra information runs past the data, it returns MPV_ERR_END with
 * *data at the data's end. the coding type must be 1 to 4: the handler tables
 * are indexed by it unchecked. types 5 to 7 read past their ends, and type 0
 * sets NULL handlers, which mpvdec_DecSlice then calls */
Sint32 mpvdec_DecPsc(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                     Sint32 *used)
{
    struct mpv_picture *picture;
    struct mpv_motion *motion;
    /* @hack redundant: register on cursor, taken, next, word, code and bytes.
     * SHC then leaves type and motion in the frame, MOV.L R3,@(4,R15) at
     * 0x8ced65dc and MOV.L R2,@R15 at 0x8ced65fc. without it, the function
     * runs 4 bytes short and 217 of its 505 words differ */
    register Uint8 *cursor;
    register Sint32 taken;
    register Uint32 next;
    register Uint32 word;
    Sint32 type;
    register Sint32 code;
    Sint32 decode;
    Sint32 flag;
    Sint32 end;
    register Sint32 bytes;

    picture = &mpv->picture;
    OPEN_READER(data);
    TAKE_FIELD(10, picture->temporal_ref);
    TAKE_FIELD(3, picture->type);
    TAKE_FIELD(16, mpv->vbv_delay);
    type = picture->type;
    if (type == MPV_PTYPE_P || type == MPV_PTYPE_B) {
        motion = &mpv->forward;
        TAKE_FLAG(motion->full_pel);
        TAKE_FIELD(3, code);
        motion->r_size = --code;
        motion->shift = 27 - code;
        motion->f = 1 << code;
        if (type == MPV_PTYPE_B) {
            motion = &mpv->backward;
            TAKE_FLAG(motion->full_pel);
            /* @hack spelling: TAKE_FIELD(3, code) written out, with word >> 29
             * taken ahead of the test, at 0x8ced66da to 0x8ced66de. as
             * TAKE_FIELD, the function runs 8 bytes short and 327 of its 505
             * words differ */
            {
                Uint32 peek = word >> 29;
                if (taken >= 29) {
                    taken -= 29;
                    if (taken != 0) {
                        word |= next >> (3 - taken);
                        code = word >> 29;
                        word = next << taken;
                    } else {
                        code = peek;
                        word = next;
                    }
                    LOAD_WORD(next);
                } else {
                    taken += 3;
                    code = peek;
                    word <<= 3;
                }
            }
            motion->r_size = --code;
            motion->shift = 27 - code;
            motion->f = 1 << code;
        }
    }
    decode = mpv->cond[MPV_COND_DECODE];
    mpv->decode_mb = mpvdec_mb_decoders[type];
    mpv->skipped = mpvdec_skippers[decode][type];
    mpv->intra_blocks = mpvdec_intra_blocks[decode];
    mpv->nintra_blocks = mpvdec_nintra_blocks[decode];
    /* @bug memory: with MPV_COND_DECODE 0, the four makers set here are NULL,
     * and the decoder calls one at the first macroblock it decodes: an I
     * picture's intra maker, by JSR @R3 at 0x8ceb4b6c. each table's row for 0
     * holds NULL (mpvdec_intra_makers' at 0x8cedcedc and 0x8cedcf04,
     * mpvdec_forward_makers' at 0x8cedcf2c, mpvdec_backward_makers' at
     * 0x8cedcf54 and mpvdec_bidirect_makers' at 0x8cedcf7c). MPVMMC_Intra,
     * MPVMMC_Forward, MPVMMC_Backward and MPVMMC_BiDirect, the makers that do
     * nothing, go unused: no word of MOVIE.BIN holds their addresses */
    mpv->make_intra =
        mpvdec_intra_makers[mpv->cond[MPV_COND_INTRA_TA]][decode][type];
    mpv->make[MAKER_INDEX(MPV_MB_BACKWARD)] =
        mpvdec_backward_makers[decode][type];
    mpv->make[MAKER_INDEX(MPV_MB_FORWARD)] =
        mpvdec_forward_makers[decode][type];
    mpv->make[MAKER_INDEX(MPV_MB_BACKWARD | MPV_MB_FORWARD)] =
        mpvdec_bidirect_makers[decode][type];
    /* skip the extra information: a byte after each set flag bit, up to a
     * clear one */
    for (;;) {
        flag = (Sint32)word < 0;
        /* @hack redundant: the else after the break. with it, the refill loads
         * its bytes into R3, R2 and R3, at 0x8ced67e8 to 0x8ced67f4. without
         * it, R2 and R3 trade places, and 9 of the 505 words differ */
        if (flag == 0) {
            SKIP_BITS(1);
            break;
        } else {
            SKIP_BITS(9);
            /* @hack type: the count formed as Sint32s. SHC then adds
             * cursor - 8, ADD R3,R10 at 0x8ced6816, before it subtracts *data,
             * SUB R2,R10 at 0x8ced681a. as a pointer difference, 58 of the
             * 505 words differ, or 18 as
             * ((taken + 7) >> 3) + (cursor - 8 - *data) */
            bytes = ((taken + 7) >> 3) + (Sint32)(cursor - 8);
            bytes -= (Sint32)*data;
            if (*size <= bytes) {
                *used += *size;
                *data += *size;
                *size = 0;
                return MPV_ERR_END;
            }
        }
    }
    /* @hack type: end as an Sint32, which holds the byte count and then the
     * address. SHC then copies taken, MOV R12,R13 at 0x8ced6836, ahead of
     * cursor's step back, ADD #-8,R14 at 0x8ced683a. as a Uint8 *, the two
     * trade places and 2 of the 505 words differ */
    end = (taken + 7) >> 3;
    cursor -= 8;
    end += (Sint32)cursor;
    bytes = end - (Sint32)*data;
    *used += bytes;
    *size -= bytes;
    *data = (Uint8 *)end;
    return mpvdec_GoNextLayer(mpv, data, size, used, MPV_DELIM_FIRST_SLICE);
}

/* step to the next start code. where that is an extension's, step past it to
 * the start code after, and then do the same for a user data's. returns 0 if
 * the start code it stops at is of a kind in mask, and NO_LAYER if not or if
 * the data ends first */
Sint32 mpvdec_GoNextLayer(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                          Sint32 *used, Sint32 mask)
{
    Sint32 kind;

    kind = mpvdec_GoNextDelim(data, size, used);
    if (kind == 0) {
        return NO_LAYER;
    }
    if (kind == MPV_DELIM_EXTENSION) {
        *data += 4;
        *size -= 4;
        *used += 4;
        kind = mpvdec_GoNextDelim(data, size, used);
        if (kind == 0) {
            return NO_LAYER;
        }
    }
    if (kind == MPV_DELIM_USER_DATA) {
        *data += 4;
        *size -= 4;
        *used += 4;
        kind = mpvdec_GoNextDelim(data, size, used);
        if (kind == 0) {
            return NO_LAYER;
        }
    }
    if ((kind & mask) == 0) {
        return NO_LAYER;
    }
    return 0;
}

/* step to the first start code at or after *data, or to the data's end where
 * there is none. returns the start code's MPV_DELIM_* kind, or 0 for none */
Sint32 mpvdec_GoNextDelim(Uint8 **data, Sint32 *size, Sint32 *used)
{
    Uint8 *found;
    Sint32 kind;
    Sint32 skip;

    found = MPV_SearchDelim(*data, *size, MPV_DELIM_ANY);
    if (found == NULL) {
        kind = 0;
        skip = *size;
    } else {
        kind = MPV_CheckDelim(found);
        skip = found - *data;
    }
    *data += skip;
    *size -= skip;
    *used += skip;
    return kind;
}

Sint32 MPV_GetPicAtr(struct mpv_obj *mpv, struct mpv_picture *picture)
{
    /* @hack spelling: hn, a copy of mpv, which SHC keeps in R14 across
     * MPVLIB_CheckHn, MOV R4,R14 at 0x8ced698e; through mpv, which then goes
     * to the stack, 19 of the 48 words differ */
    struct mpv_obj *hn = mpv;

    if (MPVLIB_CheckHn(hn) != 0) {
        return MPVERR_SetCode(NULL, MPV_ERR_PICATR_HN);
    }
    *picture = hn->picture;
    return 0;
}

Sint32 MPV_GetBitRate(struct mpv_obj *mpv, Sint32 *bitrate)
{
    /* @hack spelling: hn, a copy of mpv, which SHC keeps in R14 across
     * MPVLIB_CheckHn, MOV R4,R14 at 0x8ced6a04; through mpv, which then goes
     * to the stack, the function runs 2 bytes short and the PSG 4, and 22 of
     * its 25 words differ */
    struct mpv_obj *hn = mpv;

    if (MPVLIB_CheckHn(hn) != 0) {
        return MPVERR_SetCode(NULL, MPV_ERR_GETBITRATE_HN);
    }
    *bitrate = hn->bit_rate;
    return 0;
}

Sint32 MPVDEC_DecPicture(struct mpv_obj *mpv, Uint8 *data, Sint32 size,
                         Sint32 *used)
{
    Sint32 code;
    Sint32 kind;

    *used = 0;
    for (;;) {
        code = mpvdec_Recover(mpv, MPV_DELIM_ANY, &data, &size, used);
        if (code != 0) {
            return MPVERR_SetCode(mpv, code);
        }
        kind = MPV_CheckDelim(data);
        if ((kind & MPV_DELIM_SLICE) == 0) {
            break;
        }
        mpvdec_DecSlice(mpv, &data, &size, used);
    }
    return 0;
}

/* return 0 at once where *data is at a start code of a kind in mask. else
 * count a miss in mpv->err. with MPV_COND_RECOVER clear, return MPV_ERR_DELIM.
 * with it set, step to the next start code of a kind in mask, count a recovery
 * and return 0. where none follows, it returns MPV_ERR_END with *data at the
 * data's end */
Sint32 mpvdec_Recover(struct mpv_obj *mpv, Sint32 mask, Uint8 **data,
                      Sint32 *size, Sint32 *used)
{
    struct mpv_err_info *err;
    Uint8 *found;

    if ((MPV_CheckDelim(*data) & mask) == 0) {
        err = &mpv->err;
        err->misses++;
        /* @hack redundant: the else after the return. with it, SHC calls
         * MPV_SearchDelim through R2, at 0x8ced6ad2. without it, SHC calls it
         * through R3, and 13 of the 73 words differ */
        if (mpv->cond[MPV_COND_RECOVER] == 0) {
            return MPV_ERR_DELIM;
        } else {
            found = MPV_SearchDelim(*data, *size, mask);
            /* @hack redundant: the else after the return. with it, used's
             * address takes R2, at 0x8ced6af8. without it, the address takes
             * R3, and 10 of the 73 words differ */
            if (found == NULL) {
                *used += *size;
                *data += *size;
                *size = 0;
                return MPV_ERR_END;
            } else {
                *used += found - *data;
                *size -= found - *data;
                *data = found;
                err->recoveries++;
            }
        }
    }
    return 0;
}

/* read the slice header at *data: its row and its quantizer scale. the row
 * sets mpv->index to the macroblock before the row's first, which the first
 * address increment counts from. then reset the motion and DC predictions,
 * and decode the slice's macroblocks with mpv->decode_mb. returns its result.
 * where the header's extra information runs past the data, it returns
 * MPV_ERR_END with *data at the data's end */
Sint32 mpvdec_DecSlice(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                       Sint32 *used)
{
    /* @hack redundant: register on cursor, next, word and flag. SHC then
     * keeps size in R9, MOV R6,R9 at 0x8ced6b56, and cursor in R12. without
     * it, size takes R8 and cursor R14, and 128 of the 250 words differ */
    register Uint8 *cursor;
    Sint32 taken;
    register Uint32 next;
    register Uint32 word;
    Uint32 code;
    register Sint32 flag;
    Sint32 bytes;

    cursor = (Uint8 *)((Uint32)*data & ~3UL);
    /* @hack type: *data read a second time, as an Sint32, which C89 leaves
     * undefined, as in OPEN_READER. SHC then loads the pointer twice,
     * MOV.L @R12,R12 at 0x8ced6b62 and MOV.L @R11,R11 at 0x8ced6b68. read
     * once, the PSG runs 30 bytes long and 241 of the 250 words differ */
    taken = *(Sint32 *)data - (Sint32)cursor;
    taken *= 8;
    LOAD_WORD(word);
    word <<= taken;
    LOAD_WORD(next);
    /* the next 32 bits are the slice's start code, which code takes whole. its
     * last byte is the slice's row, counted from 1 */
    if (taken != 0) {
        word |= next >> (32 - taken);
        code = word;
        word = next << taken;
    } else {
        code = word;
        word = next;
    }
    LOAD_WORD(next);
    mpv->index = (((code & 0xff) - 1) * mpv->picture.mb_width) - 1;
    TAKE_FIELD(5, mpv->qscale);
    mpvdec_ResetMv(&mpv->forward);
    mpvdec_ResetMv(&mpv->backward);
    mpvdec_ResetDc(mpv);
    /* skip the extra information: a byte after each set flag bit, up to a
     * clear one */
    for (;;) {
        flag = (Sint32)word < 0;
        /* @hack redundant: the else after the break. with it, the refill loads
         * its bytes into R3, R2 and R3, at 0x8ced6cbc to 0x8ced6cc4. without
         * it, R2 and R3 trade places, and 9 of the 250 words differ */
        if (flag == 0) {
            SKIP_BITS(1);
            break;
        } else {
            SKIP_BITS(9);
            /* @hack type: the count formed as Sint32s. SHC then adds
             * cursor - 8, ADD R3,R4 at 0x8ced6ce8, before it subtracts *data,
             * SUB R3,R4 at 0x8ced6cee. as a pointer difference, the function
             * runs 2 bytes short and 55 of its 250 words differ, or 12 of them
             * as bytes += cursor - 8 - *data */
            bytes = (taken + 7) >> 3;
            bytes += (Sint32)(cursor - 8);
            bytes -= (Sint32)*data;
            if (*size <= bytes) {
                *used += *size;
                *data += *size;
                *size = 0;
                return MPV_ERR_END;
            }
        }
    }
    PUT_READER(mpv);
    return mpv->decode_mb(mpv, data, size, used);
}

/* the macroblock decoders go in PSGSFD02. the three reservations below bring
 * it to the 0x6000 bytes MPVASEC_CheckSectionSiz wants of it. each reservation
 * ends in an empty assembler comment. its ; ends the text clang-format reads
 * as a C statement */
#pragma section SFD02

/* clang-format off */
#ifndef __clang__
/* PSGSFD02's first 7680 bytes, reserved and left unwritten, from 0x8ceb2be0 to
 * mpvdec_DecIpicMb at 0x8ceb49e0. this #pragma asm block opens PSGSFD02 with
 * its own .SECTION line. SHC writes the block ahead of the section line it
 * writes for mpvdec_DecIpicMb. without the block's own line, the 7680 bytes go
 * in the PSG, between mpvdec_DecSlice and mpvdec_DecDpicMb */
/* @asm open: no inspect rule reads the bytes its .RES.B reserves */
#pragma asm
        .SECTION    PSGSFD02,CODE,ALIGN=4
        .RES.B      7680
        ;
#pragma endasm
#endif
/* clang-format on */

/* decode an I picture's macroblocks from the reader mpvdec_DecSlice leaves in
 * mpv->bits. decoding stops at the next start code, or at an increment past
 * the picture's last macroblock. it also stops at bits that are no address
 * increment code. *data then steps to the next start code, and the function
 * returns 0 */
Sint32 mpvdec_DecIpicMb(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                        Sint32 *used)
{
    Uint8 *cursor;
    Sint32 taken;
    Uint32 word;
    Uint32 next;
    Uint32 code;
    Uint32 entry;
    Sint32 length;
    Sint32 value;
    Sint32 start;
    Sint32 increment;
    Uint8 *end;
    Sint32 bytes;
    Sint32 invalid = NO_MB;

    /* @hack redundant: a never-run store to invalid. SHC then keeps NO_MB in
     * R8 from MOV #-2,R8 at 0x8ceb49f2 to its use at 0x8ceb4ad6. without it,
     * 18 of the 234 words differ */
    if (0) {
        invalid = 0;
    }
    GET_READER(mpv);
    /* @hack redundant: a never-run read of used. SHC then gives used the
     * frame's first slot and size the second, at 0x8ceb49fa and 0x8ceb49f8.
     * without it, the function runs 8 bytes short and 56 of its 234 words
     * differ */
    if (0) {
        if (used) {
        }
    }
    for (;;) {
        PEEK_FIELD(START_CODE_ZEROS, code);
        if (code == 0) {
            break;
        }
        start = mpv->index;
        for (;;) {
            /* @hack redundant: three never-run reads of start. SHC then keeps
             * start in R5 from its load at 0x8ceb4a26 to its use at
             * 0x8ceb4ae8. without them, 58 of the 234 words differ */
            if (0) {
                if (start) {
                }
                if (start) {
                }
                if (start) {
                }
            }
            /* @hack redundant: never-run reads: next 4 times, taken 8,
             * word 10 and data 18. SHC then gives word, next, taken and cursor
             * R11, R13, R12 and R14, and data R10, ahead of mpv in R9, at
             * 0x8ceb49ea to 0x8ceb4a02. without them, 192 of the 234 words
             * differ */
            if (0) {
                if (next) {
                }
                if (next) {
                }
                if (next) {
                }
                if (next) {
                }
                if (taken) {
                }
                if (taken) {
                }
                if (taken) {
                }
                if (taken) {
                }
                if (taken) {
                }
                if (taken) {
                }
                if (taken) {
                }
                if (taken) {
                }
                if (word) {
                }
                if (word) {
                }
                if (word) {
                }
                if (word) {
                }
                if (word) {
                }
                if (word) {
                }
                if (word) {
                }
                if (word) {
                }
                if (word) {
                }
                if (word) {
                }
                if (data) {
                }
                if (data) {
                }
                if (data) {
                }
                if (data) {
                }
                if (data) {
                }
                if (data) {
                }
                if (data) {
                }
                if (data) {
                }
                if (data) {
                }
                if (data) {
                }
                if (data) {
                }
                if (data) {
                }
                if (data) {
                }
                if (data) {
                }
                if (data) {
                }
                if (data) {
                }
                if (data) {
                }
                if (data) {
                }
            }
            PEEK_FIELD(12, code);
            if ((code >> 8) == 0) {
                entry = mpvvlc_mbai_i_0[code];
            } else {
                entry = mpvvlc_mbai_i_1[code >> 6];
            }
            length = entry & 15;
            SKIP_BITS(length);
            /* @hack type: (Uint8)(entry >> 2) >> 2 for (entry >> 4) & 63.
             * SHC forms it with SHLR2, EXTU.B and SHLR2 at 0x8ceb4a94 to
             * 0x8ceb4a98. as (entry >> 4) & 63, 49 of the 234 words differ */
            value = (Uint8)(entry >> 2) >> 2;
            if (value == MBAI_STUFFING) {
                continue;
            }
            if (value == MBAI_ESCAPE) {
                mpv->index += MBAI_ESCAPE_STEP;
                continue;
            }
            if (value == MBAI_NO_CODE) {
                increment = invalid;
            } else {
                mpv->index += value;
                mpv->mb_type = entry >> 10;
                if (mpv->index > mpv->last_index) {
                    increment = invalid;
                } else {
                    increment = mpv->index - start;
                }
            }
            if (increment == NO_MB) {
                goto finish;
            }
            break;
        }
        if (mpv->mb_type & MPV_MB_QUANT) {
            TAKE_FIELD(5, mpv->qscale);
        }
        PUT_READER(mpv);
        mpv->intra_blocks(mpv);
        mpv->make_intra(mpv);
        GET_READER(mpv);
    }
finish:
    /* @hack spelling: the bytes counted before cursor steps back, which puts
     * MOV R12,R4 at 0x8ceb4b7a ahead of ADD #-8,R14 at 0x8ceb4b7e; with cursor
     * first, 2 of the 234 words differ */
    {
        Sint32 byte_offset = (taken + 7) >> 3;
        cursor -= 8;
        end = cursor + byte_offset;
    }
    bytes = end - *data;
    *used += bytes;
    *size -= bytes;
    *data = end;
    mpvdec_GoNextDelim(data, size, used);
    return 0;
}

/* clang-format off */
#ifndef __clang__
/* 7280 bytes reserved and left unwritten, from mpvdec_DecIpicMb's end at
 * 0x8ceb4bd0 to mpvdec_DecPpicMb at 0x8ceb6840 */
/* @asm open: no inspect rule reads the bytes its .RES.B reserves */
#pragma asm
        .RES.B      7280
        ;
#pragma endasm
#endif
/* clang-format on */

/* decode a P picture's macroblocks from the reader mpvdec_DecSlice leaves in
 * mpv->bits. decoding stops at the next start code, or at an increment past
 * the picture's last macroblock. it also stops at bits that are no address
 * increment code or no motion code. *data then steps to the next start code,
 * and the function returns 0 */
Sint32 mpvdec_DecPpicMb(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                        Sint32 *used)
{
    /* @hack redundant: register on cursor, taken, word, next, entry, length,
     * value, start and first. SHC then gives word, taken, next and cursor R10
     * to R13 and entry R14, ahead of mpv in R8 and data in R9, at 0x8ceb684e
     * to 0x8ceb6862. without it, 121 of the 415 words differ */
    register Uint8 *cursor;
    register Sint32 taken;
    register Uint32 word;
    register Uint32 next;
    Uint32 code;
    Uint32 packed;
    register Uint32 entry;
    register Sint32 length;
    register Sint32 value;
    register Sint32 start;
    register Sint32 first;
    Uint8 *end;
    Sint32 bytes;
    Sint32 bits;

    first = 1;
    GET_READER(mpv);
next_macroblock:
    PEEK_FIELD(START_CODE_ZEROS, entry);
    if (entry == 0) {
        goto picture_done;
    }
    start = mpv->index;
read_address_increment:
    PEEK_FIELD(11, code);
    if ((code >> 7) == 0) {
        entry = mpvvlc_mbai_p_0[code];
    } else {
        entry = mpvvlc_mbai_p_1[code >> 6];
    }
    length = entry & 15;
    SKIP_BITS(length);
    /* @hack redundant: a never-run jump. SHC then loads the table's entry
     * into R14, MOV.W @(R0,R14),R14 at 0x8ceb68c4, the register that later
     * holds the increment. without it, the entry takes R5, and 12 of the 415
     * words differ */
    if (0) {
        goto picture_done;
    }
    /* @hack type: (Uint8)(entry >> 2) >> 2 for (entry >> 4) & 63. SHC forms
     * it with SHLR2, EXTU.B and SHLR2 at 0x8ceb68f4 to 0x8ceb68f8. as
     * (entry >> 4) & 63, 49 of the 415 words differ */
    value = (Uint8)(entry >> 2) >> 2;
    if (value == MBAI_STUFFING) {
        goto read_address_increment;
    }
    if (value == MBAI_ESCAPE) {
        mpv->index += MBAI_ESCAPE_STEP;
        goto read_address_increment;
    }
    /* @hack spelling: the gotos, with which bits that are no code and an
     * increment past the last macroblock share the store of NO_MB at
     * 0x8ceb6936, ahead of a literal pool; as if and else, the function runs 8
     * bytes long and 323 of its 415 words differ */
    if (value == MBAI_NO_CODE) {
        goto invalid_increment;
    }
    mpv->index += value;
    entry >>= 10;
    mpv->mb_type = entry;
    if (mpv->index <= mpv->last_index) {
        goto valid_increment;
    }
invalid_increment:
    entry = NO_MB;
    goto address_increment_done;
valid_increment:
    entry = mpv->index - start;
address_increment_done:
    if (entry == (Uint32)NO_MB) {
        goto picture_done;
    }
    /* past the slice's first macroblock, an increment over 1 skips the
     * macroblocks between. mpv->skipped makes the skipped macroblocks, and the
     * decoder then resets the forward and DC predictions */
    if (first == 0 && entry > 1) {
        mpv->skipped(mpv, entry);
        mpvdec_ResetMv(&mpv->forward);
        mpvdec_ResetDc(mpv);
    }
    if ((mpv->mb_type & MPV_MB_TYPED) == 0) {
        PEEK_FIELD(5, code);
        packed = mpvvlc_p_mbtype[code];
        mpv->mb_type = packed >> 8;
        length = packed & 0xff;
        SKIP_BITS(length);
    }
    if (mpv->mb_type & MPV_MB_QUANT) {
        TAKE_FIELD(5, mpv->qscale);
    }
    if (mpv->mb_type & MPV_MB_FORWARD) {
        PUT_READER(mpv);
        entry = mpvdec_MotionSub(mpv, &mpv->forward, &mpv->forward.x,
                                 &mpv->forward.predict_x);
        code = mpvdec_MotionSub(mpv, &mpv->forward, &mpv->forward.y,
                                &mpv->forward.predict_y);
        entry |= code;
        GET_READER(mpv);
        if (entry != 0) {
            goto picture_done;
        }
    } else {
        mpvdec_ResetMv(&mpv->forward);
    }
    if (mpv->mb_type & MPV_MB_PATTERN) {
        PEEK_FIELD(9, code);
        packed = mpvvlc_cbp[code];
        mpv->cbp = (packed & ~15) << 16;
        length = packed & 0xff;
        SKIP_BITS(length);
    } else {
        mpv->cbp = 0;
    }
    PUT_READER(mpv);
    if (mpv->mb_type & MPV_MB_INTRA) {
        mpv->intra_blocks(mpv);
        mpv->make_intra(mpv);
    } else {
        if (mpv->cbp != 0) {
            mpv->nintra_blocks(mpv);
        }
        /* a P picture predicts each non-intra macroblock forward, by the zero
         * vector where the macroblock codes none */
        mpv->make[MAKER_INDEX(MPV_MB_FORWARD)](mpv);
        mpvdec_ResetDc(mpv);
    }
    GET_READER(mpv);
    first = 0;
    goto next_macroblock;
picture_done:
    /* @hack spelling: taken copied before cursor steps back. SHC then puts
     * MOV R11,R5 at 0x8ceb6b86 ahead of ADD #-8,R13 at 0x8ceb6b8a. without
     * the copy, the two trade places and 2 of the 415 words differ */
    bits = taken;
    cursor -= 8;
    end = cursor + ((bits + 7) >> 3);
    /* @hack spelling: the address difference in two statements. SHC then keeps
     * end in R5 and the count in R4, MOV R5,R4 at 0x8ceb6b96. as
     * end - *data, 11 of the 415 words differ */
    bytes = (Sint32)end;
    bytes -= (Sint32)*data;
    *used += bytes;
    *size -= bytes;
    *data = end;
    mpvdec_GoNextDelim(data, size, used);
    /* @hack redundant: a never-run return of entry. SHC then keeps entry in
     * R14 for the start code's peek at 0x8ceb6864 and the motion results at
     * 0x8ceb6a76. without it, the peek takes R4, and 38 of the 415 words
     * differ */
    if (0) {
        return entry;
    }
    return 0;
}

/* clang-format off */
#ifndef __clang__
/* 6856 bytes reserved and left unwritten, from mpvdec_DecPpicMb's end at
 * 0x8ceb6bd0 to mpvdec_ResetMv at 0x8ceb8698. they put mpvdec_ResetMv,
 * mpvdec_ResetDc and mpvdec_MotionSub out of mpvdec_DecPpicMb's BSR reach.
 * mpvdec_DecPpicMb calls the three with JSR through pool words, at
 * 0x8ceb696e, 0x8ceb6974 and 0x8ceb6a70. mpvdec_DecBpicMb, after them, calls
 * mpvdec_ResetDc and mpvdec_MotionSub with BSR, at 0x8ceb8932 and
 * 0x8ceb8a26 */
/* @asm open: no inspect rule reads the bytes its .RES.B reserves */
#pragma asm
        .RES.B      6856
        ;
#pragma endasm
#endif
/* clang-format on */

/* zero motion's vector and its prediction */
void mpvdec_ResetMv(struct mpv_motion *motion)
{
    motion->predict_x = 0;
    motion->predict_y = 0;
    motion->x = 0;
    motion->y = 0;
}

void mpvdec_ResetDc(struct mpv_obj *mpv)
{
    mpv->dc_pred.y = DC_PRED_RESET;
    mpv->dc_pred.cr = DC_PRED_RESET;
    mpv->dc_pred.cb = DC_PRED_RESET;
}

/* read one component of a motion vector from mpv->bits: its motion code,
 * then a residual of motion->r_size bits if the code and r_size aren't 0. the
 * code and the residual give a delta, and the component is *predict plus that
 * delta, wrapped into the vector's range. *predict takes the component, and
 * *vector takes it, doubled where motion->full_pel is set. returns 0, or -1
 * for bits that are no motion code, which leave the reader, *vector and
 * *predict as they were */
Sint32 mpvdec_MotionSub(struct mpv_obj *mpv, struct mpv_motion *motion,
                        Sint32 *vector, Sint32 *predict)
{
    Uint8 *cursor;
    Sint32 taken;
    Uint32 word;
    Uint32 next;
    Uint32 code;
    Uint32 entry;
    Sint32 delta;
    Sint32 r_size;
    Sint32 shift;
    Uint32 residual;
    Sint32 error;

    GET_READER(mpv);
    error = 0;
    r_size = motion->r_size;
    shift = motion->shift;
    PEEK_FIELD(11, code);
    if ((code >> 7) == 0) {
        entry = mpvvlc_motion_0[code];
    } else {
        entry = mpvvlc_motion_1[code >> 6];
    }
    /* @hack redundant: a never-run sum of 23 reads of entry. with it, the
     * function saves and uses R8 to R13, at 0x8ceb86b4 to 0x8ceb86c2. without
     * it, the function uses R9 to R14, and 89 of its 165 words differ */
    if (0) {
        error = entry + entry + entry + entry + entry + entry + entry + entry +
                entry + entry + entry + entry + entry + entry + entry + entry +
                entry + entry + entry + entry + entry + entry + entry;
    }
    /* an entry holds the motion code in its low byte, signed, and the code's
     * bit count in the byte above */
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    delta = (Sint8)entry;
    if (delta == MOTION_NO_CODE) {
        error = -1;
    } else {
        entry = (entry >> 8) & 0xff;
        SKIP_BITS(entry);
        if (delta == 0) {
            *vector = *predict;
        } else {
            if (r_size != 0) {
                TAKE_FIELD(r_size, residual);
                delta <<= r_size;
                residual = motion->f - 1 - residual;
                if (delta > 0) {
                    delta -= residual;
                } else {
                    delta += residual;
                }
            }
            delta += *predict;
            /* *vector takes the sum wrapped into the vector's range: its low
             * 5 + r_size bits, sign-extended, since SHC shifts an Sint32 right
             * arithmetically */
            *vector = (delta << shift) >> shift;
            *predict = *vector;
        }
        if (motion->full_pel) {
            *vector <<= 1;
        }
    }
    PUT_READER(mpv);
    return error;
}

/* decode a B picture's macroblocks from the reader mpvdec_DecSlice leaves in
 * mpv->bits. decoding stops at the next start code, or at an increment past
 * the picture's last macroblock. it also stops at bits that are no address
 * increment code or no motion code. *data then steps to the next start code,
 * and the function returns 0 */
Sint32 mpvdec_DecBpicMb(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                        Sint32 *used)
{
    /* @hack redundant: register on cursor, taken, word, next, code, entry,
     * length, start and first. SHC then gives word, taken, next and cursor R10
     * to R13 and entry R14, ahead of mpv in R8 and data in R9, at 0x8ceb8816
     * to 0x8ceb882c. without it, 124 of the 453 words differ */
    register Uint8 *cursor;
    register Sint32 taken;
    register Uint32 word;
    register Uint32 next;
    register Uint32 code;
    Uint32 packed;
    register Uint32 entry;
    register Sint32 length;
    Sint32 value;
    register Sint32 start;
    register Sint32 first;
    Uint8 *end;
    Sint32 bytes;
    Sint32 bits;

    first = 1;
    GET_READER(mpv);
next_macroblock:
    PEEK_FIELD(START_CODE_ZEROS, entry);
    if (entry == 0) {
        goto picture_done;
    }
    start = mpv->index;
read_address_increment:
    PEEK_FIELD(11, code);
    if ((code >> 7) == 0) {
        entry = mpvvlc_mbai_b_0[code];
    } else {
        entry = mpvvlc_mbai_b_1[code >> 6];
    }
    length = entry & 15;
    SKIP_BITS(length);
    /* @hack redundant: a never-run jump. SHC then loads the table's entry
     * into R14, MOV.W @(R0,R14),R14 at 0x8ceb888c, the register that later
     * holds the increment. without it, 18 of the 453 words differ */
    if (0) {
        goto picture_done;
    }
    /* @hack type: (Uint8)(entry >> 2) >> 2 for (entry >> 4) & 63. SHC forms
     * it with SHLR2, EXTU.B and SHLR2 at 0x8ceb88be to 0x8ceb88c2. as
     * (entry >> 4) & 63, the function runs 4 bytes long and 378 of its 453
     * words differ */
    value = (Uint8)(entry >> 2) >> 2;
    if (value == MBAI_STUFFING) {
        goto read_address_increment;
    }
    if (value == MBAI_ESCAPE) {
        mpv->index += MBAI_ESCAPE_STEP;
        goto read_address_increment;
    }
    /* @hack spelling: the gotos, with which bits that are no code and an
     * increment past the last macroblock share the store of NO_MB at
     * 0x8ceb8900, ahead of a literal pool; as if and else, the function runs
     * 12 bytes long and 352 of its 453 words differ */
    if (value == MBAI_NO_CODE) {
        goto invalid_increment;
    }
    mpv->index += value;
    entry >>= 10;
    mpv->mb_type = entry;
    if (mpv->index <= mpv->last_index) {
        goto valid_increment;
    }
invalid_increment:
    entry = NO_MB;
    goto address_increment_done;
valid_increment:
    entry = mpv->index - start;
address_increment_done:
    if (entry == (Uint32)NO_MB) {
        goto picture_done;
    }
    /* past the slice's first macroblock, an increment over 1 skips the
     * macroblocks between. mpv->skipped makes the skipped macroblocks, and the
     * decoder then resets the DC predictions */
    if (first == 0 && entry > 1) {
        mpv->skipped(mpv, entry);
        mpvdec_ResetDc(mpv);
    }
    if ((mpv->mb_type & MPV_MB_TYPED) == 0) {
        PEEK_FIELD(6, code);
        packed = mpvvlc_b_mbtype[code];
        mpv->mb_type = packed >> 8;
        length = packed & 0xff;
        SKIP_BITS(length);
    }
    if (mpv->mb_type & MPV_MB_QUANT) {
        TAKE_FIELD(5, mpv->qscale);
    }
    if (mpv->mb_type & MPV_MB_FORWARD) {
        PUT_READER(mpv);
        entry = mpvdec_MotionSub(mpv, &mpv->forward, &mpv->forward.x,
                                 &mpv->forward.predict_x);
        code = mpvdec_MotionSub(mpv, &mpv->forward, &mpv->forward.y,
                                &mpv->forward.predict_y);
        entry |= code;
        GET_READER(mpv);
        if (entry != 0) {
            goto picture_done;
        }
    }
    if (mpv->mb_type & MPV_MB_BACKWARD) {
        PUT_READER(mpv);
        entry = mpvdec_MotionSub(mpv, &mpv->backward, &mpv->backward.x,
                                 &mpv->backward.predict_x);
        code = mpvdec_MotionSub(mpv, &mpv->backward, &mpv->backward.y,
                                &mpv->backward.predict_y);
        entry |= code;
        GET_READER(mpv);
        if (entry != 0) {
            goto picture_done;
        }
    }
    if (mpv->mb_type & MPV_MB_PATTERN) {
        PEEK_FIELD(9, code);
        packed = mpvvlc_cbp[code];
        mpv->cbp = (packed & ~15) << 16;
        length = packed & 0xff;
        SKIP_BITS(length);
    } else {
        mpv->cbp = 0;
    }
    PUT_READER(mpv);
    if (mpv->mb_type & MPV_MB_INTRA) {
        mpv->intra_blocks(mpv);
        mpv->make_intra(mpv);
        mpvdec_ResetMv(&mpv->forward);
        mpvdec_ResetMv(&mpv->backward);
    } else {
        if (mpv->cbp != 0) {
            mpv->nintra_blocks(mpv);
        }
        /* make[0], beside the three makers MAKER_INDEX indexes, keeps the
         * maker for this macroblock's directions. mpv->skipped repeats it for
         * the macroblocks skipped before the next one coded */
        mpv->make[0] = mpv->make[MAKER_INDEX(mpv->mb_type)];
        mpv->make[0](mpv);
        mpvdec_ResetDc(mpv);
    }
    GET_READER(mpv);
    first = 0;
    goto next_macroblock;
picture_done:
    /* @hack spelling: taken copied before cursor steps back. SHC then puts
     * MOV R11,R5 at 0x8ceb8b86 ahead of ADD #-8,R13 at 0x8ceb8b8a. without
     * the copy, the two trade places, size and used trade frame slots at
     * 0x8ceb8822 and 0x8ceb8824, and 8 of the 453 words differ */
    bits = taken;
    cursor -= 8;
    end = cursor + ((bits + 7) >> 3);
    /* @hack spelling: the address difference in two statements. SHC then keeps
     * end in R5 and the count in R4, MOV R5,R4 at 0x8ceb8b98. as
     * end - *data, 17 of the 453 words differ */
    bytes = (Sint32)end;
    bytes -= (Sint32)*data;
    *used += bytes;
    *size -= bytes;
    *data = end;
    mpvdec_GoNextDelim(data, size, used);
    /* @hack redundant: a never-run return of entry. SHC then keeps entry in
     * R14 for the start code's peek at 0x8ceb882e and the motion results at
     * 0x8ceb8a2a and 0x8ceb8a72. without it, the function runs 108 bytes long
     * and 442 of its 453 words differ */
    if (0) {
        return entry;
    }
    return 0;
}

#pragma section

/* decode none of a D picture's macroblocks, and return 0 */
Sint32 mpvdec_DecDpicMb(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                        Sint32 *used)
{
    /* @bug logic: MPVDEC_DecPicture loops on a D picture's first slice. this
     * leaves *data at the slice's start code (RTS at 0x8ced6d4c), so
     * MPVDEC_DecPicture finds the same slice again and decodes it again (BSR
     * to mpvdec_DecSlice at 0x8ced6a7c). only a slice header whose extra
     * information runs past the data moves *data on, to the data's end */
    return 0;
}

/* spin through an empty loop count times, or not at all for a count below 1.
 * nothing in MOVIE.BIN calls it */
void mpvdec_WaitCnt(Sint32 count)
{
    if (count > 0) {
        do {
        } while (--count);
    }
}
