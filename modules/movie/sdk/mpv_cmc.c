/* the macroblock makers: each macroblock made from its blocks and the
 * reference frames, and written to the output */
#include <machine.h>
#include <sg_xpt.h>
#include <sofdec.h>

#include "movie/sdk/mpv.h"

/* the store queues' address control registers. in each, the QACR_AREA bits
 * are address bits 28 to 26 of its queue's writes, which pick the 64 MB area
 * the writes go to */
#define QACR0 (*(volatile Uint32 *)0xff000038)
#define QACR1 (*(volatile Uint32 *)0xff00003c)
#define QACR_AREA 0x1c
/* the store queue area, and the address bits within a 64 MB area, which a
 * store queue's address keeps */
#define SQ_AREA 0xe0000000
#define SQ_OFFSET 0x03ffffff

/* point both store queues at the 64 MB area that holds write, and turn write
 * into its address in the store queue area. the makers mask interrupts while
 * they write through it */
#define SQ_REDIRECT(write)                                                     \
    do {                                                                       \
        Uint32 area = ((Uint32)(write) >> 24) & QACR_AREA;                     \
        QACR0 = area;                                                          \
        QACR1 = area;                                                          \
        (write) = (Uint8 *)(((Uint32)(write) & SQ_OFFSET) | SQ_AREA);          \
    } while (0)

/* the parts of a split motion vector: whole pels across and down, then the half
 * pels */
enum { MV_X, MV_Y, MV_HALF_X, MV_HALF_Y, MV_MAX };

/* how many entries the clip table holds below 0, and as many above 255 */
#define CLIP_MARGIN 384

const Uint8 *mpv_clip_0_255_base;
static Uint8 mpv_clip_0_255_tbl[CLIP_MARGIN + 256 + CLIP_MARGIN];

/* the prediction kernels, as [half2][half_x + 2 * half_y]. half2 is the
 * decoder's MPV_COND_HALF_PEL_2 condition, and half_x and half_y are a
 * vector's half pels across and down. with the condition set, a vector at half
 * pels both ways averages two samples, not four */
static void (*const mpvcmc_ref_kernels[2][4])(struct mpv_mc *mc) = {
    {MPVAMC_OneRef1, MPVAMC_OneRefH2, MPVAMC_OneRefV2, MPVAMC_OneRef4},
    {MPVAMC_OneRef1, MPVAMC_OneRefH2, MPVAMC_OneRefV2, MPVAMC_OneRefV2},
};

static void mpvcmc_InitClip0255(void);
static void mpvcmc_InitMcOiTa(struct mpv_obj *mpv);
static void mpvcmc_OneReadMb(struct mpv_obj *mpv, Uint8 *block,
                             struct mpv_mb_offset *ofs, struct mpv_frame *frame,
                             struct mpv_motion *motion);
static void mpvcmc_OneMakeMbTa(struct mpv_obj *mpv, struct mpv_mb_param *mb);
static void mpvcmc_BiMakeMbTa(struct mpv_obj *mpv, struct mpv_mb_param *mb);
static void mpvcmc_CalcOfs(struct mpv_obj *mpv, Sint32 index,
                           struct mpv_mb_offset *ofs, struct mpv_frame *frame);
static void mpvcmc_CalcMv(Sint32 x, Sint32 y, Sint32 *mv);

void MPVCMC_Init(Uint8 *clip)
{
    mpvcmc_InitClip0255();
    if (clip != NULL) {
        UTY_MemcpyDword(clip, mpv_clip_0_255_tbl,
                        MPV_NWORD(mpv_clip_0_255_tbl));
        mpv_clip_0_255_base = clip + CLIP_MARGIN;
    }
}

static void mpvcmc_InitClip0255(void)
{
    Uint8 *p = mpv_clip_0_255_tbl;
    Sint32 i;

    for (i = 0; i < CLIP_MARGIN; ++i) {
        *p++ = 0;
    }
    for (i = 0; i < 256; ++i) {
        *p++ = i;
    }
    for (i = 0; i < CLIP_MARGIN; ++i) {
        *p++ = 255;
    }
    mpv_clip_0_255_base = mpv_clip_0_255_tbl + CLIP_MARGIN;
}

void MPVCMC_InitObj(struct mpv_obj *mpv)
{
    MPVMC_Init(&mpv->mc);
    mpvcmc_InitMcOiTa(mpv);
}

/* aim every ta destination at mb_work's first 64 bytes, rows 8 apart */
static void mpvcmc_InitMcOiTa(struct mpv_obj *mpv)
{
    struct mpv_out_block *out = mpv->ta;
    Uint8 *pred = mpv->mb_work;

    out[0].start = pred;
    out[1].start = pred;
    out[2].start = pred;
    out[3].start = pred;
    out[4].start = pred;
    out[5].start = pred;
    out[0].pitch = 8;
    out[1].pitch = 8;
    out[2].pitch = 8;
    out[3].pitch = 8;
    out[4].pitch = 8;
    out[5].pitch = 8;
}

void MPVCMC_InitMcOiRt(struct mpv_obj *mpv)
{
    struct mpv_out_block *out = mpv->rt;
    struct mpv_request *request = &mpv->request;
    struct mpv_frame *frame = &request->second;
    Sint32 pitch;

    pitch = frame->c_pitch;
    out[0].pitch = pitch;
    out[1].pitch = pitch;
    pitch = frame->y_pitch;
    out[2].pitch = pitch;
    out[3].pitch = pitch;
    out[4].pitch = pitch;
    out[5].pitch = pitch;
}

/* the code goes in section PSGSFD00, beside the assembly it calls */
#pragma section SFD00

void MPVCMC_IntraRt(struct mpv_obj *mpv)
{
    Sint32 imask = get_imask();
    Uint8 *write;
    struct mpv_request *request;
    struct mpv_frame *frame;
    struct mpv_mb_offset ofs;
    struct mpv_out_block *out;
    Sint32 y_pitch;

    /* @hack redundant: write's address taken, which keeps write in the frame,
     * MOV.L R2,@R15 at 0x8ceb0e80; without it, 90 of 107 words differ */
    (void)&write;
    set_imask(IMASK_ALL);
    request = &mpv->request;
    out = mpv->rt;
    frame = &request->second;
    write = request->output;
    request->output = write + MPV_MB_BYTES;
    SQ_REDIRECT(write);
    mpvcmc_CalcOfs(mpv, mpv->index, &ofs, frame);
    y_pitch = frame->y_pitch;
    /* @hack spelling: ofs read through its address, which SHC forms again at
     * each use, MOV R15,R3 and ADD #8,R3 at 0x8ceb0eb6 and 0x8ceb0ebe; read as
     * ofs.c and ofs.y, 62 of 107 words differ */
    out[0].start = frame->cb + (&ofs)->c;
    out[1].start = frame->cr + (&ofs)->c;
    out[2].start = frame->y + (&ofs)->y;
    out[3].start = out[2].start + 8;
    out[4].start = out[2].start + (y_pitch * 8);
    out[5].start = out[4].start + 8;
    MPVACMC_OutputIntra6blk(mpv->blocks[0], write, out, mpv->clip);
    set_imask(imask);
}

void MPVCMC_IntraTa(struct mpv_obj *mpv)
{
    Sint32 imask = get_imask();
    Uint8 *write;
    struct mpv_request *request;

    /* @hack redundant: write's address taken, which keeps write in the frame,
     * MOV.L R2,@R15 at 0x8ceb0f4a; without it, 46 of 58 words differ */
    (void)&write;
    set_imask(IMASK_ALL);
    request = &mpv->request;
    write = request->output;
    request->output = write + MPV_MB_BYTES;
    SQ_REDIRECT(write);
    MPVACMC_OutputIntra6blk(mpv->blocks[0], write, mpv->ta, mpv->clip);
    set_imask(imask);
}

void MPVCMC_ForwardRt(struct mpv_obj *mpv)
{
    struct mpv_mb_offset ofs;
    struct mpv_mb_param *mb = &mpv->mb;
    struct mpv_request *request = &mpv->request;

    mpvcmc_OneReadMb(mpv, mb->pred, &ofs, &request->first, &mpv->forward);
    mpvcmc_OneMakeMbTa(mpv, mb);
    MPVACMC_OutputMbRfb(mb->pred, &ofs, &request->second);
}

void MPVCMC_ForwardTa(struct mpv_obj *mpv)
{
    struct mpv_mb_offset ofs;
    struct mpv_mb_param *mb = &mpv->mb;
    struct mpv_request *request = &mpv->request;

    mpvcmc_OneReadMb(mpv, mb->pred, &ofs, &request->first, &mpv->forward);
    mpvcmc_OneMakeMbTa(mpv, mb);
}

void MPVCMC_Backward(struct mpv_obj *mpv)
{
    struct mpv_mb_offset ofs;
    struct mpv_mb_param *mb = &mpv->mb;
    struct mpv_request *request = &mpv->request;

    mpvcmc_OneReadMb(mpv, mb->pred, &ofs, &request->second, &mpv->backward);
    mpvcmc_OneMakeMbTa(mpv, mb);
}

void MPVCMC_BiDirect(struct mpv_obj *mpv)
{
    struct mpv_mb_offset ofs;
    struct mpv_mb_param *mb = &mpv->mb;
    struct mpv_request *request = &mpv->request;

    mpvcmc_OneReadMb(mpv, mb->pred, &ofs, &request->first, &mpv->forward);
    mpvcmc_OneReadMb(mpv, mb->pred_b, &ofs, &request->second, &mpv->backward);
    mpvcmc_BiMakeMbTa(mpv, mb);
}

/* predict the current macroblock from frame, displaced by motion, into block.
 * block takes Cb, Cr and then the four Y blocks, 64 bytes each. *ofs gets the
 * macroblock's offsets */
static void mpvcmc_OneReadMb(struct mpv_obj *mpv, Uint8 *block,
                             struct mpv_mb_offset *ofs, struct mpv_frame *frame,
                             struct mpv_motion *motion)
{
    Sint32 half2 = mpv->cond[MPV_COND_HALF_PEL_2];
    Sint32 y_pitch = frame->y_pitch;
    Sint32 c_pitch = frame->c_pitch;
    Sint32 x;
    Sint32 y;
    Sint32 c_ofs;
    Sint32 y_ofs;
    Sint32 c_mv[MV_MAX];
    Sint32 y_mv[MV_MAX];
    void (*c_ref)(struct mpv_mc *mc);
    void (*y_ref)(struct mpv_mc *mc);
    struct mpv_mc *mc;
    Uint8 *src;

    mpvcmc_CalcOfs(mpv, mpv->index, ofs, frame);
    x = motion->x;
    y = motion->y;
    /* Cb and Cr move half as far as Y */
    mpvcmc_CalcMv(x / 2, y / 2, c_mv);
    c_ofs = ofs->c + c_mv[MV_X] + (c_mv[MV_Y] * c_pitch);
    mpvcmc_CalcMv(x, y, y_mv);
    y_ofs = ofs->y + y_mv[MV_X] + (y_mv[MV_Y] * y_pitch);
    c_ref = mpvcmc_ref_kernels[half2][c_mv[MV_HALF_X] + (c_mv[MV_HALF_Y] * 2)];
    y_ref = mpvcmc_ref_kernels[half2][y_mv[MV_HALF_X] + (y_mv[MV_HALF_Y] * 2)];
    mc = &mpv->mc;
    /* mc->second is a row below mc->first. where MPV_COND_HALF_PEL_2 is set
     * and the vector lies at a half pel across, mc->second is also a pixel to
     * the right */
    mc->read_pitch = c_pitch;
    mc->block = block;
    src = frame->cb + c_ofs;
    mc->first = src;
    src += (half2 ? c_mv[MV_HALF_X] : 0) + c_pitch;
    mc->second = src;
    c_ref(mc);
    mc->block = block + 64;
    src = frame->cr + c_ofs;
    mc->first = src;
    src += (half2 ? c_mv[MV_HALF_X] : 0) + c_pitch;
    mc->second = src;
    c_ref(mc);
    mc->read_pitch = y_pitch;
    mc->block = block + 128;
    src = frame->y + y_ofs;
    mc->first = src;
    src += (half2 ? y_mv[MV_HALF_X] : 0) + y_pitch;
    mc->second = src;
    y_ref(mc);
    mc->block = block + 192;
    mc->first += 8;
    mc->second += 8;
    y_ref(mc);
    mc->block = block + 256;
    mc->first += (y_pitch - 1) * 8;
    mc->second += (y_pitch - 1) * 8;
    y_ref(mc);
    mc->block = block + 320;
    mc->first += 8;
    mc->second += 8;
    y_ref(mc);
}

/* add the coded blocks to the prediction, and write the macroblock as the
 * request's next */
static void mpvcmc_OneMakeMbTa(struct mpv_obj *mpv, struct mpv_mb_param *mb)
{
    Sint32 imask = get_imask();
    Uint8 *write;
    struct mpv_request *request;

    /* @hack redundant: write's address taken, which keeps write in the frame,
     * MOV.L R2,@R15 at 0x8ceb1292; without it, 46 of 56 words differ */
    (void)&write;
    set_imask(IMASK_ALL);
    request = &mpv->request;
    write = request->output;
    request->output = write + MPV_MB_BYTES;
    SQ_REDIRECT(write);
    MPVACMC_OneMakeMbTaSub(write, mb, mpv->cbp);
    set_imask(imask);
}

/* average the two predictions, add the coded blocks, and write the macroblock
 * as the request's next */
static void mpvcmc_BiMakeMbTa(struct mpv_obj *mpv, struct mpv_mb_param *mb)
{
    Sint32 imask = get_imask();
    Uint8 *write;
    struct mpv_request *request;

    /* @hack redundant: write's address taken, which keeps write in the frame,
     * MOV.L R2,@R15 at 0x8ceb1302; without it, 46 of 56 words differ */
    (void)&write;
    set_imask(IMASK_ALL);
    request = &mpv->request;
    write = request->output;
    request->output = write + MPV_MB_BYTES;
    SQ_REDIRECT(write);
    MPVACMC_BiMakeMbTaSub(write, mb, mpv->cbp);
    set_imask(imask);
}

void MPVCMC_PpicSkipped(struct mpv_obj *mpv, Sint32 count)
{
    Uint8 *block = mpv->mb_work;
    struct mpv_request *request = &mpv->request;
    struct mpv_frame *frame = &request->first;
    struct mpv_frame *out = &request->second;
    struct mpv_mc *mc = &mpv->mc;
    Sint32 y_pitch = frame->y_pitch;
    Sint32 c_pitch = frame->c_pitch;
    Uint8 *write = request->output;
    struct mpv_mb_offset ofs;

    request->output = write + (count * MPV_MB_BYTES) - MPV_MB_BYTES;
    while (--count) {
        mpvcmc_CalcOfs(mpv, mpv->index - count, &ofs, frame);
        mc->skip_pitch = c_pitch;
        mc->write = write;
        /* @hack spelling: ofs read through its address, which SHC forms again
         * at each use, MOV R15,R3 and ADD #16,R3 at 0x8ceb13e2 and 0x8ceb13ea;
         * read as ofs.c and ofs.y, 93 of 140 words differ */
        mc->first = frame->cb + (&ofs)->c;
        write = MPVACMC_PpicSkipBlk(mc, block);
        block += 64;
        mc->write = write;
        mc->first = frame->cr + (&ofs)->c;
        write = MPVACMC_PpicSkipBlk(mc, block);
        block += 64;
        mc->skip_pitch = y_pitch;
        mc->write = write;
        mc->first = frame->y + (&ofs)->y;
        write = MPVACMC_PpicSkipBlk(mc, block);
        block += 64;
        mc->write = write;
        mc->first += 8;
        write = MPVACMC_PpicSkipBlk(mc, block);
        block += 64;
        mc->write = write;
        mc->first += (y_pitch - 1) * 8;
        write = MPVACMC_PpicSkipBlk(mc, block);
        block += 64;
        mc->write = write;
        mc->first += 8;
        write = MPVACMC_PpicSkipBlk(mc, block);
        block = mpv->mb_work;
        MPVACMC_OutputMbRfb(block, &ofs, out);
    }
}

void MPVCMC_BpicSkipped(struct mpv_obj *mpv, Sint32 count)
{
    Sint32 end = mpv->index;
    Sint32 index;
    void (*make)(struct mpv_obj *mpv);

    mpv->cbp = 0;
    make = mpv->make[0];
    index = end - count + 1;
    while (index < end) {
        mpv->index = index;
        make(mpv);
        ++index;
    }
    mpv->index = end;
}

/* *ofs gets macroblock index's offsets in frame's planes */
static void mpvcmc_CalcOfs(struct mpv_obj *mpv, Sint32 index,
                           struct mpv_mb_offset *ofs, struct mpv_frame *frame)
{
    Sint32 row;
    Sint32 column;
    Sint8 *pos = mpv->mbpos[index];
    Sint32 c_pitch = frame->c_pitch;
    Sint32 y_pitch = frame->y_pitch;

    row = pos[0] * 8;
    column = pos[1] * 8;
    ofs->c = (row * c_pitch) + column;
    row <<= 1;
    column <<= 1;
    ofs->y = (row * y_pitch) + column;
}

/* split a motion vector in half pels into whole pels and half pels */
static void mpvcmc_CalcMv(Sint32 x, Sint32 y, Sint32 *mv)
{
    mv[MV_X] = x >> 1;
    mv[MV_Y] = y >> 1;
    mv[MV_HALF_X] = x & 1;
    mv[MV_HALF_Y] = y & 1;
}

#pragma section
