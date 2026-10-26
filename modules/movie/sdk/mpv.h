/* the Sofdec MPEG video decoder's records and interfaces
 *
 * each MPV_ call that takes a decoder and returns an Sint32 checks it first,
 * as MPVLIB_CheckHn does. a null decoder fails the check, as does one whose
 * stat doesn't mark it handed out, as after MPV_Destroy. the call then returns
 * a negative error code and records it as the library's */
#ifndef MOVIE_SDK_MPV_H
#define MOVIE_SDK_MPV_H
#include <sg_xpt.h>

/* the 32-bit words in table, the unit UTY_MemcpyDword counts in. make lint
 * fails a sizeof quotient it takes for a miscounted entry count, and the
 * divisor's parentheses mark this one as meant */
#define MPV_NWORD(table) (sizeof(table) / (sizeof(Uint32)))

/* the start code kinds MPV_CheckDelim returns, one bit each. slice start code
 * 0x101, the first row's, sets MPV_DELIM_SLICE too. as a mask, MPV_DELIM_ANY
 * matches every kind */
#define MPV_DELIM_SLICE 0x01
#define MPV_DELIM_FIRST_SLICE 0x02
#define MPV_DELIM_PICTURE 0x04
#define MPV_DELIM_GOP 0x08
#define MPV_DELIM_EXTENSION 0x10
#define MPV_DELIM_USER_DATA 0x20
#define MPV_DELIM_SEQUENCE 0x40
#define MPV_DELIM_SEQUENCE_END 0x80
#define MPV_DELIM_ANY (-1)

/* the length of a decoder's cond, which MPV_SetCond indexes by the conditions
 * below */
#define MPV_COND_MAX 8
/* 1 by default: where the data is not at a start code of a kind the decoder
 * reads, search on to the next one. 0: return MPV_ERR_DELIM there */
#define MPV_COND_RECOVER 1
/* 1 by default: decode each macroblock's blocks, and make the macroblock. 0:
 * decode no blocks and make no skipped macroblock, but leave every other maker
 * null, which the decoder calls at the first macroblock it decodes */
#define MPV_COND_DECODE 2
/* 1: average two samples, not four, for a vector at half pels both across
 * and down. 0 by default */
#define MPV_COND_HALF_PEL_2 3
/* 1: write an intra macroblock of an I or P picture to the output only, not
 * into the second frame as well. 0 by default */
#define MPV_COND_INTRA_TA 4

/* a macroblock's type, as bits. its type code gives them, unless the code of
 * its address increment gave them already, which then sets MPV_MB_TYPED too */
#define MPV_MB_INTRA 0x01
#define MPV_MB_PATTERN 0x02
#define MPV_MB_BACKWARD 0x04
#define MPV_MB_FORWARD 0x08
#define MPV_MB_QUANT 0x10
#define MPV_MB_TYPED 0x20

/* a macroblock's 8 by 8 blocks, and its bytes in the output. the output holds
 * Cb, Cr and then the four Y blocks, a byte a pixel */
#define MPV_BLOCKS 6
#define MPV_MB_BYTES 384

/* size bytes of the stream at data */
struct mpv_stream {
    Uint8 *data;
    Sint32 size;
};

/* an error record, a decoder's or the library's own */
struct mpv_err_info {
    void (*func)(void *obj, Sint32 code); /* NULL for none */
    void *obj; /* func's first argument */
    Sint32 code; /* the last code recorded */
    /* the times the data was not at a start code of a kind the decoder reads,
     * and the times it searched on and found one */
    Sint32 misses;
    Sint32 recoveries;
};

/* the block decoders' parameters for one block: where its coefficients go, and
 * how they are dequantized */
struct mpv_block_param {
    Uint8 unk_00[16];
    /* the first coded coefficient's place in the inverse DCT's order */
    Sint32 first;
    Uint8 unk_14[8];
    Float *coef;
    Uint8 *qmat;
    Sint32 qscale;
    Sint32 *dc_pred; /* the DC predictor of the block's plane */
    Sint8 *dcsiz; /* the plane's table of DC size codes */
    /* 0 in an intra macroblock, 1 in a predicted one */
    Sint32 nonintra;
};

/* the inverse DCT's parameters, which DCT_FzInitPa sets up */
struct dct_fz_param {
    /* coded[i] is nonzero where block i holds more than its DC coefficient.
     * the drivers read the first MPV_BLOCKS */
    Uint8 coded[8];
    /* sparse transforms a block that holds only its DC coefficient. dense is
     * DCTAFZ_DenseCbp, which the drivers call by name, not through dense */
    void (*sparse)(const Float *coef, Sint16 *block);
    void (*dense)(struct dct_fz_param *param);
    /* the first block's bit in cbp, and the mask that clears it */
    Uint32 first_bit;
    Uint32 first_keep;
    /* the two counts MPV_GetDctCnt reports. DCT_FzInitPa zeroes them, and
     * nothing writes them after, so both stay 0 */
    Sint32 count[2];
    /* added to each result before it is truncated */
    Float round_bias;
    /* the bytes from one block's coefficients to the next's, in coef */
    Sint32 stride;
    /* the blocks left to transform, block i in bit 31 - i */
    /* @hack type: cbp signed, though it holds bits, so that DCT_FzInitPa
     * stores the null coef, blocks and cursor from R5, which holds cbp's 0
     * (0x8ced9748 to 0x8ced974c), and forms unk_3c's 128 as 256 - 128
     * (ADD #-128 at 0x8ced974e). as Uint32, the section runs 4 bytes long,
     * which moves the pool DCT_FzInit shares with DCT_FzInitPa, and 37 of 255
     * words differ in DCT_FzInit and the field's four users, DCT_FzInitPa,
     * DCT_FzTrans6Blk, DCT_FzTransCbp and MPVCDEC_NintraBlocks */
    Sint32 cbp;
    Float *coef;
    Sint16 **blocks; /* the six blocks' outputs */
    /* DCTAFZ_DenseCbp's next output. it leaves cursor past all six */
    Sint16 **cursor;
    /* taken from each result after it is truncated */
    Sint32 output_bias;
    Sint32 unk_3c; /* 128, which no driver or kernel reads */
    /* the 4 by 4 matrices a pass takes a line's even and odd coefficients
     * through */
    Float *even;
    Float *odd;
    Uint8 *scratch; /* DCTAFZ_DenseCbp's, 256 bytes a block */
    /* DCTAFZ_DenseCbp's state in its last pass: the blocks left, and its place
     * in scratch */
    Sint32 left;
    Uint8 *scratch_at;
};

/* the motion kernels' parameters: one 8 by 8 block's source, and where its
 * copy goes */
struct mpv_mc {
    /* the copy kernels MPVAMC_OneRef1 picks by first & 3 */
    void (*ref1[4])(Uint8 *block, const Uint8 *src, Sint32 pitch);
    Uint8 unk_10[4];
    Uint8 *write; /* where MPVACMC_PpicSkipBlk copies the block too */
    Uint8 *block; /* the kernels' output, 64 bytes */
    /* the bytes from one row of first to the next, as MPVACMC_PpicSkipBlk
     * reads them. it writes its copy to write in rows 8 bytes apart */
    Sint32 skip_pitch;
    /* the bytes from one row of first, or of second, to the next, as the
     * motion kernels read them */
    Sint32 read_pitch;
    /* the source block, at its first row, and the one MPVAMC_OneRefV2 and
     * MPVAMC_OneRef4 average with it */
    Uint8 *first;
    Uint8 *second;
    Uint8 unk_2c[8];
};

/* the macroblock writers' parameters */
struct mpv_mb_param {
    const Uint8 *clip; /* the clip table's entry for 0 */
    Sint16 *blocks; /* the decoded blocks, in the output's order */
    /* the prediction. a bidirectional macroblock's forward one is in pred, and
     * its backward one in pred_b */
    Uint8 *pred;
    Uint8 *pred_b;
};

/* where MPVACMC_OutputIntra6blk writes a block: its first row, and the bytes
 * from one row to the next */
struct mpv_out_block {
    Uint8 *start;
    Sint32 pitch;
};

/* a frame: its Cb, Cr and Y planes, and their pitches */
struct mpv_frame {
    Uint8 *cb;
    Uint8 *cr;
    Uint8 *y;
    Sint16 c_pitch;
    Sint16 y_pitch;
};

/* a macroblock's offsets in a frame's chroma and luma planes, in bytes */
struct mpv_mb_offset {
    Sint32 c;
    Sint32 y;
};

/* the picture coding types */
#define MPV_PTYPE_I 1
#define MPV_PTYPE_P 2
#define MPV_PTYPE_B 3
#define MPV_PTYPE_D 4

/* a picture's information, from its headers: its size, its group's time code
 * and its coding type */
struct mpv_picture {
    Sint32 width;
    Sint32 height;
    Sint32 mb_width; /* the width in macroblocks */
    Sint32 mb_height; /* the height in macroblocks */
    Sint32 rate; /* the frame rate code */
    Sint32 temporal_ref; /* its frames past its group's time code */
    Sint32 type; /* an MPV_PTYPE_* */
    Sint32 drop; /* whether the time code drops frames */
    Sint32 hour;
    Sint32 minute;
    Sint32 second;
    Sint32 frame;
};

/* MPV_DecodeFrm's request. a P or B picture predicts forward from first, and a
 * B picture backward from second. an I or P picture's macroblocks also go into
 * second, the intra ones only while MPV_COND_INTRA_TA is 0. output takes the
 * picture's macroblocks in address order, MPV_MB_BYTES each */
struct mpv_request {
    struct mpv_frame first;
    struct mpv_frame second;
    Uint8 *output;
    struct mpv_picture *picture;
    Sint32 misses;
    Sint32 recoveries;
};

/* the DC predictors of the Y, Cb and Cr blocks */
struct mpv_dc_pred {
    Sint32 y;
    Sint32 cb;
    Sint32 cr;
};

/* a motion vector, and how its picture codes it */
struct mpv_motion {
    Sint32 full_pel; /* nonzero for a vector in whole pels */
    Sint32 r_size; /* the f_code less 1: a residual's bits */
    /* 27 - r_size. a vector shifted left by it, then arithmetically right by
     * it, keeps its low 5 + r_size bits, which wraps it into its range */
    Sint32 shift;
    Sint32 f; /* 1 << r_size */
    /* the last vector decoded, before full_pel doubles it */
    Sint32 predict_x;
    Sint32 predict_y;
    /* the vector, in half pels */
    Sint32 x;
    Sint32 y;
    Uint8 unk_20[4];
};

/* the tables every decoder shares, which MPV_Init places in its work after the
 * decoders. the library calls this record its ixa */
struct mpv_ixa {
    Uint16 masks[16];
    Sint8 zigzag[64];
    Uint8 dct[0x90];
    Uint8 vlc[0x5b0];
    Uint8 clip[0x400];
};

/* the stream reader, which the block decoders keep in registers. the next 32
 * bits are word's top 32 - used bits, then next's top used bits. once word's
 * bits are all taken, next replaces it, and the big-endian word at cursor
 * replaces next */
struct mpv_bits {
    Uint32 word; /* the current word, shifted past the bits taken from it */
    Uint32 next; /* the word after it */
    Sint32 used; /* the bits taken from the current word, 0 to 31 */
    const Uint32 *cursor; /* the next word to load */
};

/* a decoder, 8 KB, which MPV_Create hands out */
struct mpv_obj {
    struct mpv_bits bits;
    /* the block decoders' run and level tables */
    const Uint32 *run_level_8;
    const Uint16 *run_level_4;
    const Uint16 *run_level_2;
    const Uint16 *run_level_1;
    const Uint16 *run_level_0a;
    const Uint16 *run_level_0b;
    const Uint16 *run_level_0c;
    Sint8 *zigzag; /* ixa's zigzag */
    struct mpv_ixa *ixa;
    Uint8 *dct; /* ixa's dct */
    const Uint8 *clip; /* the clip table's entry for 0 */
    struct mpv_block_param block;
    struct dct_fz_param dct_param;
    struct mpv_mc mc;
    struct mpv_mb_param mb;
    /* where MPVACMC_OutputIntra6blk puts an intra macroblock's blocks, besides
     * the output. rt holds their places in the second frame. ta puts every
     * block through mb_work's first 64 bytes, rows 8 apart */
    struct mpv_out_block rt[MPV_BLOCKS];
    struct mpv_out_block ta[MPV_BLOCKS];
    Sint32 stat; /* free, or handed out */
    Sint32 unk_16c;
    Sint32 cond[MPV_COND_MAX]; /* by MPV_COND_* */
    struct mpv_picture picture;
    struct mpv_err_info err;
    Uint8 unk_1d4[12];
    struct mpv_request request; /* a copy of MPV_DecodeFrm's */
    /* the sequence header's aspect ratio code, bit rate, VBV buffer size and
     * constrained parameters flag */
    Sint32 aspect_ratio;
    Sint32 bit_rate;
    Sint32 vbv_buffer_size;
    Sint32 constrained;
    /* the group's two flags, and the picture's VBV delay */
    Sint32 closed_gop;
    Sint32 broken_link;
    Sint32 vbv_delay;
    /* the picture's handlers, which MPV_DecodePicAtr picks by its coding type
     * and the conditions. decode_mb decodes a slice's macroblocks */
    Sint32 (*decode_mb)(struct mpv_obj *mpv, Uint8 **data, Sint32 *size,
                        Sint32 *used);
    /* make the count - 1 macroblocks the slice skips before the current one */
    void (*skipped)(struct mpv_obj *mpv, Sint32 count);
    /* decode an intra or a predicted macroblock's blocks */
    Sint32 (*intra_blocks)(struct mpv_obj *mpv);
    Sint32 (*nintra_blocks)(struct mpv_obj *mpv);
    void (*make_intra)(struct mpv_obj *mpv); /* make an intra macroblock */
    /* the predicted macroblock makers, by a macroblock's MPV_MB_BACKWARD and
     * MPV_MB_FORWARD bits shifted down 2. a B picture's predicted macroblock
     * copies its maker to make[0], which the skipped macroblocks after it
     * repeat */
    void (*make[4])(struct mpv_obj *mpv);
    Sint32 qscale;
    struct mpv_motion forward;
    struct mpv_motion backward;
    /* the current macroblock's index in mbpos, and the picture's last */
    Sint32 index;
    Sint32 last_index;
    Sint32 mb_type; /* the current macroblock's MPV_MB_* bits */
    /* the current macroblock's coded block pattern, a bit a block. bits 29 to
     * 24 mark the blocks in the stream's order, Y first. its CBP table entry
     * repeats Cb's and Cr's bits in bits 31 and 30. so bits 31 to 26 mark the
     * blocks in the output's order, Cb first */
    Uint32 cbp;
    struct mpv_dc_pred dc_pred;
    /* blocks, in the order the inverse DCT writes them */
    Sint16 *block_ptr[MPV_BLOCKS];
    Uint8 unk_2d0[16];
    /* the decoded blocks, in the output's order: Cb, Cr, then the four Y
     * blocks */
    Sint16 blocks[MPV_BLOCKS][64];
    /* the coefficients, in the stream's order: the four Y blocks, then Cb and
     * Cr */
    Float coef[MPV_BLOCKS][64];
    /* the intra and non-intra quantizer matrices, in the inverse DCT's order */
    Uint8 intra_qmat[64];
    Uint8 inter_qmat[64];
    /* the two predictions, a macroblock each. the inverse DCT's scratch
     * overwrites them */
    Uint8 mb_work[4 * MPV_MB_BYTES];
    /* each macroblock's row and column, for up to 1200 macroblocks */
    Sint8 mbpos[1200][2];
    Uint8 dmy1[0x440];
};

/* mpv_deli.c: start codes, each 00 00 01 and then its kind's byte. a picture
 * start code's kind byte is 00, which can also start the next code. a search
 * that rejects the first of two such codes it meets misses the second */
/* the MPV_DELIM_* bits of the start code in the 4 bytes at buf, or 0 */
Sint32 MPV_CheckDelim(Uint8 *buf);
/* the last start code of a kind in mask that lies whole in the len bytes
 * before buf, or NULL */
Uint8 *MPV_BsearchDelim(Uint8 *buf, Sint32 len, Sint32 mask);
/* the first start code of a kind in mask that lies whole in the len bytes at
 * buf, or NULL */
Uint8 *MPV_SearchDelim(Uint8 *buf, Sint32 len, Sint32 mask);
/* copy the len bytes at src to dst, stopping after the first start code of a
 * kind in mask. returns src past the last byte copied */
Uint8 *MPV_CopyStm(Uint8 *dst, Uint8 *src, Sint32 len, Sint32 mask);

/* mpv_err.c: error records */
/* clear the library's own record */
void MPVERR_Init(void);
/* clear err: func and obj NULL, and code and both counts 0 */
void MPVERR_InitErrInf(struct mpv_err_info *err);
/* send mpv's errors to func, with obj. returns 0 */
Sint32 MPV_SetErrFunc(struct mpv_obj *mpv, void (*func)(void *obj, Sint32 code),
                      void *obj);
/* copy mpv's error record to *err. returns 0 */
Sint32 MPV_GetErrInf(struct mpv_obj *mpv, struct mpv_err_info *err);
/* record code as mpv's last, or the library's when mpv is NULL, and pass a
 * nonzero code to the record's func. returns code */
Sint32 MPVERR_SetCode(struct mpv_obj *mpv, Sint32 code);

/* mpv_frm.c: pictures */
/* decode the slices at the start of stm's data, the picture MPV_DecodePicAtr
 * set mpv up for, as request asks. *used gets the bytes read. it fills
 * *request->picture with the picture's information, and request->misses and
 * request->recoveries with this call's counts. FR12 to FR15 and the second bank
 * come back as they were. returns MPVDEC_DecPicture's result */
Sint32 MPV_DecodeFrm(struct mpv_obj *mpv, struct mpv_stream *stm, Sint32 *used,
                     struct mpv_request *request);
/* skip the slices at the start of stm's data. *used gets the bytes up to the
 * next start code of a picture, group, sequence or sequence end. returns 0, or
 * where none follows, an error code, with *used all of the data. the result is
 * also recorded as mpv's last code */
Sint32 MPV_SkipFrm(struct mpv_obj *mpv, struct mpv_stream *stm, Sint32 *used);
void MPVFRM_Init(void);

/* mpv_lib.c: the library and its decoders */
/* the one error MPV_Init returns: the decoder's code sections out of order */
#define MPV_ERR_SECTION_SEQ ((Sint32)0xff03ff05)
/* lay the library out in work for nobj decoders, all free. work must be
 * 4-byte aligned and hold (nobj + 2) * 8 KB, which MPV_Init clears. the
 * decoders start at its first 8 KB boundary, and the tables they share follow
 * them. while the operand cache runs in index mode, the library uses work as
 * MPVLIB_ConvOix gives it. returns 0, or MPV_ERR_SECTION_SEQ before it touches
 * work. on any other layout error it never returns */
Sint32 MPV_Init(Sint32 nobj, void *work);
/* does nothing */
void MPV_Finish(void);
/* hand out a free decoder, its conditions at their defaults. returns NULL when
 * none is free */
struct mpv_obj *MPV_Create(void);
/* free mpv. returns 0 */
Sint32 MPV_Destroy(struct mpv_obj *mpv);
/* set mpv's condition id, an MPV_COND_*, to value. id must be below
 * MPV_COND_MAX. returns 0 */
Sint32 MPV_SetCond(struct mpv_obj *mpv, Sint32 id, Sint32 value);
/* copy dct_param's two counts to *count0 and *count1 */
void MPV_GetDctCnt(struct mpv_obj *mpv, Sint32 *count0, Sint32 *count1);
/* addr, with address bit 25 set if the instruction cache ran in index mode
 * at MPV_Init. in index mode, that bit picks the cache's half */
Uint32 MPVLIB_ConvIix(Uint32 addr);
/* addr, with address bit 25 set if the operand cache ran in index mode at
 * MPV_Init */
Uint32 MPVLIB_ConvOix(Uint32 addr);
/* 0 if mpv isn't null and its stat marks it handed out, which a freed
 * decoder's doesn't, and else -1 */
Sint32 MPVLIB_CheckHn(struct mpv_obj *mpv);
/* fill ixa's inverse DCT matrices, as MPV_Init does */
void mpvlib_InitUseLib(struct mpv_ixa *ixa);

/* mpv_bdec.c: the block decoders' tables */
/* mpvbdec_zigzag[i] is the place in the inverse DCT's order of coefficient i
 * of the zigzag scan. mpvbdec_dfl_iqm is the default intra quantizer matrix,
 * in that order */
extern Sint8 mpvbdec_zigzag[64];
extern Sint8 mpvbdec_dfl_iqm[64];
/* fill mpvbdec_zigzag and mpvbdec_dfl_iqm, and copy the zigzag scan and the
 * bit masks to ixa */
void MPVBDEC_Init(struct mpv_ixa *ixa);

/* mpv_cdec.c: a macroblock's blocks, decoded from mpv's stream into coef and
 * transformed into blocks. both return 0 */
/* all six blocks of an intra macroblock, each plane's DC from its own
 * predictor */
Sint32 MPVCDEC_IntraBlocks(struct mpv_obj *mpv);
/* the blocks mpv->cbp marks in a predicted macroblock */
Sint32 MPVCDEC_NintraBlocks(struct mpv_obj *mpv);

/* mpv_cmc.c: the macroblock makers, which write the request's next
 * macroblocks */
/* the clip table's entry for 0. the table maps -384 to 639 into 0 to 255 */
extern const Uint8 *mpv_clip_0_255_base;
/* build the clip table. with clip not NULL, copy the table there, 1024 bytes,
 * and use the copy */
void MPVCMC_Init(Uint8 *clip);
/* set up mpv's motion kernels and its ta destinations */
void MPVCMC_InitObj(struct mpv_obj *mpv);
/* give mpv's rt destinations the pitches of its request's second frame */
void MPVCMC_InitMcOiRt(struct mpv_obj *mpv);
/* an intra macroblock, from mpv->blocks. MPVCMC_IntraRt also writes it to its
 * place in the second frame */
void MPVCMC_IntraRt(struct mpv_obj *mpv);
void MPVCMC_IntraTa(struct mpv_obj *mpv);
/* a predicted macroblock: its prediction plus its coded blocks. it predicts
 * forward from the first frame, backward from the second, or both ways from
 * both. MPVCMC_ForwardRt also writes it to its place in the second frame */
void MPVCMC_ForwardRt(struct mpv_obj *mpv);
void MPVCMC_ForwardTa(struct mpv_obj *mpv);
void MPVCMC_Backward(struct mpv_obj *mpv);
void MPVCMC_BiDirect(struct mpv_obj *mpv);
/* the count - 1 macroblocks a P picture skips before the current one. each is
 * copied from its place in the first frame to the output, and to its place in
 * the second frame */
void MPVCMC_PpicSkipped(struct mpv_obj *mpv, Sint32 count);
/* the count - 1 macroblocks a B picture skips before the current one. make[0]
 * makes each, with no coded blocks */
void MPVCMC_BpicSkipped(struct mpv_obj *mpv, Sint32 count);

/* mpv_mdec.c and mpv_mmc.c: block decoders and macroblock makers that do
 * nothing. MPV_COND_DECODE 0 selects the block decoders and the makers of
 * skipped macroblocks */
Sint32 MPVMDEC_IntraBlocks(struct mpv_obj *mpv);
Sint32 MPVMDEC_NintraBlocks(struct mpv_obj *mpv);
Sint32 mpvmdec_AwayIntraYblock(void);
Sint32 mpvmdec_AwayIntraCblock(void);
Sint32 mpvmdec_AwayNintraBlock(void);
void MPVMMC_Intra(struct mpv_obj *mpv);
void MPVMMC_Forward(struct mpv_obj *mpv);
void MPVMMC_Backward(struct mpv_obj *mpv);
void MPVMMC_BiDirect(struct mpv_obj *mpv);
void MPVMMC_PpicSkipped(struct mpv_obj *mpv, Sint32 count);
void MPVMMC_BpicSkipped(struct mpv_obj *mpv, Sint32 count);

/* mpv_mc.c: the motion kernels' setup */
/* give mc the copy kernels MPVAMC_OneRef1 picks by its source's alignment */
void MPVMC_Init(struct mpv_mc *mc);

/* mpvadec.c: headers and slices */
/* the data is not at a start code of a kind the decoder reads, and
 * MPV_COND_RECOVER is 0 */
#define MPV_ERR_DELIM (-2)
/* the data ends before a start code of a kind the decoder reads, or inside a
 * header */
#define MPV_ERR_END (-3)
/* read the headers from the start of stm's data to a picture's first slice,
 * and set mpv up to decode that picture. the data may start at a sequence, a
 * group or a picture header. the picture's coding type must be 1 to 4. *used
 * gets the bytes read. returns 0 or an error code */
Sint32 MPV_DecodePicAtr(struct mpv_obj *mpv, struct mpv_stream *stm,
                        Sint32 *used);
/* copy the picture information the last MPV_DecodePicAtr read to *picture.
 * returns 0 */
Sint32 MPV_GetPicAtr(struct mpv_obj *mpv, struct mpv_picture *picture);
/* copy the sequence header's bit rate field to *bitrate. returns 0 */
Sint32 MPV_GetBitRate(struct mpv_obj *mpv, Sint32 *bitrate);
/* decode the slices of the picture MPV_DecodePicAtr set mpv up for, from the
 * size bytes at data. it stops at the first start code that isn't a slice's.
 * *used gets the bytes read. returns 0 or an error code. at a D picture's first
 * slice it reads that slice again and again, and never returns */
Sint32 MPVDEC_DecPicture(struct mpv_obj *mpv, Uint8 *data, Sint32 size,
                         Sint32 *used);

/* the values the mpvvlc_mbai_ tables give in place of an address increment,
 * past the highest increment, 33. MBAI_STUFFING is stuffing, which the
 * decoders skip. MBAI_ESCAPE adds MBAI_ESCAPE_STEP to the macroblock index
 * ahead of the increment that follows. MBAI_NO_CODE marks bits that are no
 * address increment code */
#define MBAI_STUFFING 34
#define MBAI_ESCAPE 35
#define MBAI_NO_CODE 36
#define MBAI_ESCAPE_STEP 33
/* the motion code the mpvvlc_motion_ tables give for bits that are no motion
 * code */
#define MOTION_NO_CODE 127

/* mpv_vlc.c: the tables the decoder reads, declared in the order mpv_vlc.c
 * defines them, which its literal pools follow */
extern Sint16 *mpvvlc_mbai_i_0;
extern Sint16 *mpvvlc_mbai_i_1;
extern Sint16 *mpvvlc_mbai_p_0;
extern Sint16 *mpvvlc_mbai_p_1;
extern Sint16 *mpvvlc_mbai_b_0;
extern Sint16 *mpvvlc_mbai_b_1;
extern Sint16 *mpvvlc_p_mbtype;
extern Sint16 *mpvvlc_b_mbtype;
extern Sint16 *mpvvlc_motion_0;
extern Sint16 *mpvvlc_motion_1;
extern Sint16 *mpvvlc_cbp;
extern Sint8 *mpvvlc_y_dcsiz;
extern Sint8 *mpvvlc_c_dcsiz;
extern const Uint16 *mpvvlc_run_level_0c;
extern const Uint16 *mpvvlc_run_level_0b;
extern const Uint16 *mpvvlc_run_level_0a;
extern const Uint16 *mpvvlc_run_level_1;
extern const Uint16 *mpvvlc_run_level_2;
extern const Uint16 *mpvvlc_run_level_4;
extern const Uint32 *mpvvlc_run_level_8;
/* nonzero if the tables are the wrong sizes, which mpvlib_ChkFatal reports
 * as MPV_ERR_VLC_SIZE. their sizes are fixed, so it returns 0 */
Sint32 MPVVLC_IsVlcSizErr(void);
/* build the tables, and point the mpvvlc_ pointers at them. vlc is NULL, or
 * an ixa's vlc, which then takes copies of every table but the address
 * increment and coded block pattern ones. the copies fill vlc, and their
 * pointers point at them */
void MPVVLC_Init(Uint8 *vlc);
/* build tables as MPVVLC_Init does, without pointing the mpvvlc_ pointers at
 * them */
void mpvvlc_InitMbai(void); /* the address increment tables */
void mpvvlc_InitMbType(void); /* the macroblock type tables */
void mpvvlc_InitDcSiz(void); /* the DC size tables */
/* the run and level table of the codes with 1 to 5 leading zeros */
void mpvvlc_InitRunLevel(void);

/* the decoder's assembly: the section checks, the block decoder, the motion
 * compensation, the macroblock writers and the inverse DCT's kernels */
Sint32 MPVASEC_CheckSectionSiz(void);
Sint32 MPVASEC_CheckSectionSeq(void);
Sint32 MPVABDEC_IntraBlock(struct mpv_obj *mpv, struct mpv_block_param *block);
Sint32 MPVABDEC_NintraBlock(struct mpv_obj *mpv, struct mpv_block_param *block);
void MPVABDEC_EraseBlock6(Float *coef);
void MPVAMC_OneRef1(struct mpv_mc *mc);
void MPVAMC_OneRefH2(struct mpv_mc *mc);
void MPVAMC_OneRefV2(struct mpv_mc *mc);
void MPVAMC_OneRef4(struct mpv_mc *mc);
void MPVAMC_OneRef1Byte(Uint8 *block, const Uint8 *src, Sint32 pitch);
void MPVAMC_OneRef1Word(Uint8 *block, const Uint8 *src, Sint32 pitch);
void MPVAMC_OneRef1Long(Uint8 *block, const Uint8 *src, Sint32 pitch);
void MPVACMC_OutputIntra6blk(Sint16 *blocks, Uint8 *write,
                             struct mpv_out_block *out, const Uint8 *clip);
void MPVACMC_OneMakeMbTaSub(Uint8 *write, struct mpv_mb_param *mb, Uint32 cbp);
void MPVACMC_BiMakeMbTaSub(Uint8 *write, struct mpv_mb_param *mb, Uint32 cbp);
void MPVACMC_OutputMbRfb(Uint8 *pred, struct mpv_mb_offset *ofs,
                         struct mpv_frame *frame);
Uint8 *MPVACMC_PpicSkipBlk(struct mpv_mc *mc, Uint8 *block);
void DCTAFZ_SparseDc(const Float *coef, Sint16 *block);
void DCTAFZ_DenseCbp(struct dct_fz_param *param);

/* dct_fz.c: the inverse DCT's setup and drivers */
/* fill the inverse DCT's two matrices into dct's first 128 bytes, which must be
 * 8-byte aligned. with dct null, it fills storage of its own. each param that
 * DCT_FzInitPa sets up afterwards points at these matrices, so they must stay
 * valid while such a param is in use */
void DCT_FzInit(Uint8 *dct);
/* set param up for the kernels, and for the matrices DCT_FzInit filled last.
 * sparse and dense get the kernels, and even and odd the matrices. it also
 * sets the stride, the biases, first_bit and first_keep, and 128 in unk_3c.
 * every other field is 0 or null */
void DCT_FzInitPa(struct dct_fz_param *param);
/* transform the six blocks at param->coef, stride bytes apart, into the shorts
 * at param->blocks[0] to [5]. block i goes through param->sparse where
 * coded[i] is 0. the rest, if any, go through DCTAFZ_DenseCbp, with cbp set to
 * them and cursor to blocks. DCTAFZ_DenseCbp overwrites those blocks' first 32
 * floats and their scratch. param must come from DCT_FzInitPa after
 * DCT_FzInit, with coef, blocks and scratch set since, and meet the kernels'
 * terms */
void DCT_FzTrans6Blk(struct dct_fz_param *param);
/* transform the blocks in param->cbp as DCT_FzTrans6Blk does, on the same
 * terms, clearing from cbp each that goes through param->sparse */
void DCT_FzTransCbp(struct dct_fz_param *param);

#endif
