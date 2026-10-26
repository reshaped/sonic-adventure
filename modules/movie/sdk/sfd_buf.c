/* the Sofdec buffers between the stages: rings of bytes, queues of pictures
 * and rings of samples, laid over a handle's work area */
#include <sg_xpt.h>

#include "movie/sdk/sfd.h"

/* the stage in wtrno or rtrno of a buffer that no stage writes or reads */
#define SFBUF_NOTR SFD_TR_MAX

/* the bytes kept past the end of the video, audio and user rings. the video
 * and audio decoder stages copy the data that wraps to the ring's top into
 * this margin, as far as it holds. so each reads the data across the end in
 * one run */
#define SFBUF_VIDEO_MARGIN 0x28000
#define SFBUF_AUDIO_MARGIN 0x800
#define SFBUF_USER_MARGIN 0x800

/* the words of a ring's delimiter table, which follows its margin.
 * SFBUF_InitHn makes every ring without one */
#define SFBUF_DLM_NWORD 0x100

/* the mark sfbuf_InitRingBuf stores in each ring's tag. nothing in the player
 * reads it */
#define SFBUF_RING_TAG 0x57e4

/* the errors the buffer functions record through SFLIB_SetErr */
#define SFBUF_ERR_NOAREA ((Sint32)0xff000401) /* the buffer has no area */
/* SFBUF_RingAddWrite's written isn't the ring's count of bytes written */
#define SFBUF_ERR_WRITTEN ((Sint32)0xff000402)
#define SFBUF_ERR_CLOSED ((Sint32)0xff000403) /* the ring is closed */
/* a write past the unread data, and a read past the data written: in a ring
 * of bytes, then in a ring of samples */
#define SFBUF_ERR_RINGOVER ((Sint32)0xff000404)
#define SFBUF_ERR_RINGUNDER ((Sint32)0xff000405)
#define SFBUF_ERR_ARINGOVER ((Sint32)0xff000406)
#define SFBUF_ERR_ARINGUNDER ((Sint32)0xff000407)

/* the address of sample pos in a plane. a sample of up to 8 bits takes a byte,
 * and a wider one a halfword */
#define SFBUF_SMPL(nbit, plane, pos)                                           \
    ((nbit) <= 8 ? (char *)(plane) + (pos)                                     \
                 : (char *)((Sint16 *)(plane) + (pos)))

/* NULL, as the address of a run of no samples */
/* @hack redundant: NULL either way, through a select on nbit whose test SHC
 * keeps as a CMP/GT that no branch reads (CMP/GT R7,R5 at 0x8ced7a4a,
 * CMP/GT R7,R4 at 0x8ced7b9a). as NULL, the tests go, the object runs 16
 * bytes short, and 236 of the 240 words of SFBUF_AringGetWrite and
 * SFBUF_AringGetRead differ */
#define SFBUF_NOSMPL(nbit) ((nbit) <= 8 ? NULL : NULL)

static void sfbuf_MakeBufPtr(char **area, const Sint32 *size, char *work);
static void sfbuf_InitRingBuf(struct sfbuf *buf, char **area,
                              const Sint32 *size, Sint32 bufno, Sint32 margin,
                              Bool has_dlm_tbl);
static void sfbuf_InitVfrmBuf(struct sfbuf *buf, char **area,
                              const Sint32 *size, Sint32 bufno);
static void sfbuf_InitAringBuf(struct sfbuf *buf, char **area,
                               const Sint32 *size, Sint32 bufno);

void SFBUF_Init(struct sfbuf_work *work)
{
}

void SFBUF_Finish(struct sfbuf_work *work)
{
}

/* lay buftbl's buffers over prm->work: buffer i takes the next
 * prm->bufsize[i] bytes, in SFD_BUF_* order. the input ring's margin is what
 * its size leaves past whole units of prm->unit bytes, so the ring holds whole
 * units */
void SFBUF_InitHn(struct sfd_handle *hn, struct sfbuf_tbl *buftbl,
                  struct sfd_create_param *prm)
{
    struct sfbuf *buf;
    const Sint32 *size;
    Sint32 margin;
    char *area[SFD_BUF_MAX];

    buf = buftbl->buf;
    size = prm->bufsize;
    sfbuf_MakeBufPtr(area, size, prm->work);
    margin = size[SFD_BUF_INPUT] % prm->unit;
    sfbuf_InitRingBuf(buf, area, size, SFD_BUF_INPUT, margin, FALSE);
    sfbuf_InitRingBuf(buf, area, size, SFD_BUF_VIDEO, SFBUF_VIDEO_MARGIN,
                      FALSE);
    sfbuf_InitRingBuf(buf, area, size, SFD_BUF_AUDIO, SFBUF_AUDIO_MARGIN,
                      FALSE);
    sfbuf_InitVfrmBuf(buf, area, size, SFD_BUF_FRAME);
    sfbuf_InitAringBuf(buf, area, size, SFD_BUF_PCM);
    sfbuf_InitVfrmBuf(buf, area, size, SFD_BUF_FRAME_POST);
    sfbuf_InitAringBuf(buf, area, size, SFD_BUF_PCM_POST);
    sfbuf_InitRingBuf(buf, area, size, SFD_BUF_USER, SFBUF_USER_MARGIN, FALSE);
}

static void sfbuf_MakeBufPtr(char **area, const Sint32 *size, char *work)
{
    Sint32 i;
    char *ptr;

    ptr = work;
    for (i = 0; i < SFD_BUF_MAX; i++) {
        area[i] = ptr;
        ptr += size[i];
    }
}

/* make buffer bufno an empty ring of bytes over area[bufno]. the ring takes
 * size[bufno] bytes less margin, and less a delimiter table if has_dlm_tbl is
 * TRUE. a buffer of size 0 gets no area, margin or table */
static void sfbuf_InitRingBuf(struct sfbuf *buf, char **area,
                              const Sint32 *size, Sint32 bufno, Sint32 margin,
                              Bool has_dlm_tbl)
{
    struct sfbuf_ring *ring;
    Bool has_area;
    Sint32 nword;
    Sint32 nbyte;

    /* @hack spelling: buf, area and size stepped on to bufno, and the buffer
     * and its area and size reached through them. indexed by bufno instead, 71
     * of 79 words differ */
    buf += bufno;
    area += bufno;
    size += bufno;
    ring = &buf->u.ring;
    if (*size == 0) {
        margin = 0;
        has_dlm_tbl = FALSE;
        has_area = FALSE;
    } else {
        has_area = TRUE;
    }
    if (!has_dlm_tbl) {
        nword = 0;
    } else {
        nword = SFBUF_DLM_NWORD;
    }
    nbyte = nword * sizeof(Sint32);
    buf->type = SFBUF_TYPE_RING;
    buf->has_area = has_area;
    buf->prep = FALSE;
    buf->term = FALSE;
    buf->wtrno = SFBUF_NOTR;
    buf->rtrno = SFBUF_NOTR;
    ring->top = *area;
    ring->size = *size - margin - nbyte;
    ring->end = ring->top + ring->size;
    ring->margin = margin;
    ring->wp = ring->top;
    ring->rp = ring->top;
    ring->dlm = NULL;
    ring->dlm_end = NULL;
    ring->dlm_len = 0;
    ring->written = 0;
    ring->read = 0;
    ring->closed = FALSE;
    ring->dlm_tbl = ring->end + ring->margin;
    ring->dlm_nword = nword;
    ring->dlm_used = 0;
    ring->tag = SFBUF_RING_TAG;
}

/* make buffer bufno a queue of pictures over area[bufno], with no slot handed
 * out */
static void sfbuf_InitVfrmBuf(struct sfbuf *buf, char **area,
                              const Sint32 *size, Sint32 bufno)
{
    struct sfbuf_vfrm *vfrm;
    Bool has_area;
    Sint32 i;

    /* @hack spelling: buf, area and size stepped on to bufno, and the buffer
     * and its area and size reached through them. indexed by bufno instead, the
     * object runs 8 bytes short and 55 of 56 words differ */
    buf += bufno;
    area += bufno;
    size += bufno;
    if (*size == 0) {
        has_area = FALSE;
    } else {
        has_area = TRUE;
    }
    vfrm = &buf->u.vfrm;
    buf->type = SFBUF_TYPE_VFRM;
    buf->has_area = has_area;
    buf->prep = FALSE;
    buf->term = FALSE;
    buf->wtrno = SFBUF_NOTR;
    buf->rtrno = SFBUF_NOTR;
    vfrm->top = *area;
    vfrm->size = *size;
    vfrm->unk_08 = 0;
    vfrm->unk_0c = 0;
    for (i = 0; i < SFBUF_NQUE; i++) {
        vfrm->que[i].stat = 0;
    }
}

/* make buffer bufno a ring of samples over area[bufno]. it holds no samples
 * until SFBUF_FixAringBuf sets their format */
static void sfbuf_InitAringBuf(struct sfbuf *buf, char **area,
                               const Sint32 *size, Sint32 bufno)
{
    struct sfbuf_aring *aring;
    Bool has_area;

    /* @hack spelling: buf, area and size stepped on to bufno, and the buffer
     * and its area and size reached through them. indexed by bufno instead, the
     * object runs 4 bytes short and 46 of 46 words differ */
    buf += bufno;
    area += bufno;
    size += bufno;
    if (*size == 0) {
        has_area = FALSE;
    } else {
        has_area = TRUE;
    }
    aring = &buf->u.aring;
    buf->type = SFBUF_TYPE_ARING;
    buf->has_area = has_area;
    buf->prep = FALSE;
    buf->term = FALSE;
    buf->wtrno = SFBUF_NOTR;
    buf->rtrno = SFBUF_NOTR;
    aring->top = *area;
    aring->size = *size;
    aring->sfreq = 0;
    aring->nbit = 0;
    aring->nch = 0;
    aring->plane0 = NULL;
    aring->plane1 = NULL;
    aring->nsmpl = 0;
    aring->wpos = 0;
    aring->rpos = 0;
    aring->written = 0;
    aring->read = 0;
}

/* give buffer bufno, a ring of samples, the stream's format: nbit bits a
 * sample, nch channels and sfreq samples a second. the area splits into nch
 * planes of equal size, and plane0 and plane1 address the first two. with one
 * channel, plane1 is the area's end. the ring's length, nsmpl, is a plane's
 * bytes over the bytes a sample takes */
void SFBUF_FixAringBuf(struct sfd_handle *hn, Sint32 bufno, Sint32 nbit,
                       Sint32 nch, Sint32 sfreq)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;
    struct sfbuf_aring *aring;
    char *top;
    Sint32 plane;
    Sint32 nbyte;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    aring = &buf->u.aring;
    top = aring->top;
    plane = aring->size;
    if (nch > 1) {
        plane /= nch;
    }
    nbyte = (nbit + 7) / 8;
    aring->nbit = nbit;
    aring->nch = nch;
    aring->sfreq = sfreq;
    aring->plane0 = top;
    aring->plane1 = top + plane;
    aring->nsmpl = plane / nbyte;
}

/* the room to write in ring bufno, in *inf. it runs from wp up to rp, or else
 * up to end and then from top up to rp. a full ring has none */
Sint32 SFBUF_RingGetWrite(struct sfd_handle *hn, Sint32 bufno,
                          struct sfbuf_ring_inf *inf)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;
    struct sfbuf_ring *ring;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    if (!buf->has_area) {
        return SFLIB_SetErr(hn, SFBUF_ERR_NOAREA);
    }
    ring = &buf->u.ring;
    if (ring->written >= ring->read + ring->size) {
        inf->area = ring->wp;
        inf->len = 0;
        inf->wrap = ring->wp;
        inf->wraplen = 0;
    } else if (ring->wp < ring->rp) {
        inf->area = ring->wp;
        inf->len = ring->rp - ring->wp;
        inf->wrap = ring->wp;
        inf->wraplen = 0;
    } else {
        inf->area = ring->wp;
        inf->len = ring->end - ring->wp;
        inf->wrap = ring->top;
        inf->wraplen = ring->rp - ring->top;
    }
    inf->closed = ring->closed;
    inf->written = ring->written;
    inf->read = ring->read;
    return 0;
}

/* count len bytes written at wp. written must be the count SFBUF_RingGetWrite
 * gave in inf->written, and the ring must be open, or nothing changes. a write
 * of any bytes clears dlm. a write past the unread data still counts, and
 * returns SFBUF_ERR_RINGOVER */
Sint32 SFBUF_RingAddWrite(struct sfd_handle *hn, Sint32 bufno, Sint32 len,
                          Sint32 written)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;
    struct sfbuf_ring *ring;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    if (!buf->has_area) {
        return SFLIB_SetErr(hn, SFBUF_ERR_NOAREA);
    }
    ring = &buf->u.ring;
    if (written != ring->written) {
        return SFLIB_SetErr(hn, SFBUF_ERR_WRITTEN);
    }
    if (ring->closed) {
        return SFLIB_SetErr(hn, SFBUF_ERR_CLOSED);
    }
    ring->wp += len;
    if (ring->wp >= ring->end) {
        ring->wp -= ring->size;
    }
    if (len > 0) {
        ring->dlm = NULL;
    }
    ring->written += len;
    if (ring->written > ring->read + ring->size) {
        return SFLIB_SetErr(hn, SFBUF_ERR_RINGOVER);
    }
    return 0;
}

/* the data to read in ring bufno, in *inf. it runs from rp up to wp, or else
 * up to end and then from top up to wp. an empty ring has none */
Sint32 SFBUF_RingGetRead(struct sfd_handle *hn, Sint32 bufno,
                         struct sfbuf_ring_inf *inf)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;
    struct sfbuf_ring *ring;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    if (!buf->has_area) {
        return SFLIB_SetErr(hn, SFBUF_ERR_NOAREA);
    }
    ring = &buf->u.ring;
    if (ring->written <= ring->read) {
        inf->area = ring->rp;
        inf->len = 0;
        inf->wrap = ring->rp;
        inf->wraplen = 0;
    } else if (ring->rp < ring->wp) {
        inf->area = ring->rp;
        inf->len = ring->wp - ring->rp;
        inf->wrap = ring->rp;
        inf->wraplen = 0;
    } else {
        inf->area = ring->rp;
        inf->len = ring->end - ring->rp;
        inf->wrap = ring->top;
        inf->wraplen = ring->wp - ring->top;
    }
    inf->closed = ring->closed;
    inf->written = ring->written;
    inf->read = ring->read;
    return 0;
}

/* count len bytes read at rp. a read that reaches end goes on from top, and
 * clears dlm_end and dlm_len. a read past the data written still counts, and
 * returns SFBUF_ERR_RINGUNDER */
Sint32 SFBUF_RingAddRead(struct sfd_handle *hn, Sint32 bufno, Sint32 len)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;
    struct sfbuf_ring *ring;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    if (!buf->has_area) {
        return SFLIB_SetErr(hn, SFBUF_ERR_NOAREA);
    }
    ring = &buf->u.ring;
    ring->rp += len;
    if (ring->rp >= ring->end) {
        ring->rp -= ring->size;
        ring->dlm_end = NULL;
        ring->dlm_len = 0;
    }
    /* clear dlm once rp reaches it. compared as addresses, a NULL dlm is
     * always reached */
    if ((Uint32)ring->rp >= (Uint32)ring->dlm) {
        ring->dlm = NULL;
    }
    ring->read += len;
    if (ring->read > ring->written) {
        return SFLIB_SetErr(hn, SFBUF_ERR_RINGUNDER);
    }
    return 0;
}

void SFBUF_RingGetDlm(struct sfd_handle *hn, Sint32 bufno, char **dlm,
                      Sint32 *dlm_len, char **dlm_end, Sint32 *margin)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;
    struct sfbuf_ring *ring;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    ring = &buf->u.ring;
    *dlm = ring->dlm;
    *dlm_end = ring->dlm_end;
    *dlm_len = ring->dlm_len;
    *margin = ring->margin;
}

void SFBUF_RingSetDlm(struct sfd_handle *hn, Sint32 bufno, char *dlm,
                      Sint32 dlm_len, char *dlm_end)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;
    struct sfbuf_ring *ring;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    ring = &buf->u.ring;
    ring->dlm = dlm;
    ring->dlm_end = dlm_end;
    ring->dlm_len = dlm_len;
}

/* the room to write in buffer bufno, a ring of samples, as SFBUF_RingGetWrite
 * gives it for bytes. inf->area and inf->wrap give each run's start in the
 * first two planes. a full ring gives NULL for both, and room from wpos up to
 * rpos gives NULL for the wrap. inf also gets the ring's format */
Sint32 SFBUF_AringGetWrite(struct sfd_handle *hn, Sint32 bufno,
                           struct sfbuf_aring_inf *inf)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;
    struct sfbuf_aring *aring;
    Sint32 sfreq;
    Sint32 nbit;
    Sint32 nch;
    char *plane0;
    char *plane1;
    Sint32 nsmpl;
    Sint32 wpos;
    Sint32 rpos;
    Sint32 written;
    Sint32 read;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    if (!buf->has_area) {
        return SFLIB_SetErr(hn, SFBUF_ERR_NOAREA);
    }
    aring = &buf->u.aring;
    sfreq = aring->sfreq;
    nbit = aring->nbit;
    nch = aring->nch;
    plane0 = aring->plane0;
    plane1 = aring->plane1;
    nsmpl = aring->nsmpl;
    wpos = aring->wpos;
    rpos = aring->rpos;
    written = aring->written;
    read = aring->read;
    inf->sfreq = sfreq;
    inf->nbit = nbit;
    inf->nch = nch;
    inf->written = written;
    inf->read = read;
    if (written >= read + nsmpl) {
        inf->len = 0;
        inf->area[0] = SFBUF_NOSMPL(nbit);
        inf->area[1] = SFBUF_NOSMPL(nbit);
        /* @hack spelling: a goto into the next arm, which has a full ring share
         * that arm's stores of an empty wrap (BRA 0x8ced7a82 at 0x8ced7a52).
         * with the three stores in the full ring's arm in place of the goto,
         * the object runs 12 bytes long, and 96 of 121 words differ */
        goto nowrap;
    } else if (wpos < rpos) {
        inf->len = rpos - wpos;
        inf->area[0] = SFBUF_SMPL(nbit, plane0, wpos);
        inf->area[1] = SFBUF_SMPL(nbit, plane1, wpos);
    nowrap:
        inf->wraplen = 0;
        inf->wrap[0] = SFBUF_NOSMPL(nbit);
        inf->wrap[1] = SFBUF_NOSMPL(nbit);
    } else {
        inf->len = nsmpl - wpos;
        inf->area[0] = SFBUF_SMPL(nbit, plane0, wpos);
        inf->area[1] = SFBUF_SMPL(nbit, plane1, wpos);
        inf->wraplen = rpos;
        inf->wrap[0] = SFBUF_SMPL(nbit, plane0, 0);
        inf->wrap[1] = SFBUF_SMPL(nbit, plane1, 0);
    }
    return 0;
}

/* count nsmpl samples written at wpos. a write past the unread samples still
 * counts, and returns SFBUF_ERR_ARINGOVER */
Sint32 SFBUF_AringAddWrite(struct sfd_handle *hn, Sint32 bufno, Sint32 nsmpl)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;
    struct sfbuf_aring *aring;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    if (!buf->has_area) {
        return SFLIB_SetErr(hn, SFBUF_ERR_NOAREA);
    }
    aring = &buf->u.aring;
    aring->wpos += nsmpl;
    if (aring->wpos >= aring->nsmpl) {
        aring->wpos -= aring->nsmpl;
    }
    aring->written += nsmpl;
    if (aring->written > aring->read + aring->nsmpl) {
        return SFLIB_SetErr(hn, SFBUF_ERR_ARINGOVER);
    }
    return 0;
}

/* the samples to read in buffer bufno, a ring of samples, as
 * SFBUF_RingGetRead gives bytes. inf gets them in the form SFBUF_AringGetWrite
 * gives its room */
Sint32 SFBUF_AringGetRead(struct sfd_handle *hn, Sint32 bufno,
                          struct sfbuf_aring_inf *inf)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;
    struct sfbuf_aring *aring;
    Sint32 sfreq;
    Sint32 nbit;
    Sint32 nch;
    char *plane0;
    char *plane1;
    Sint32 nsmpl;
    Sint32 wpos;
    Sint32 rpos;
    Sint32 written;
    Sint32 read;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    if (!buf->has_area) {
        return SFLIB_SetErr(hn, SFBUF_ERR_NOAREA);
    }
    aring = &buf->u.aring;
    sfreq = aring->sfreq;
    nbit = aring->nbit;
    nch = aring->nch;
    plane0 = aring->plane0;
    plane1 = aring->plane1;
    nsmpl = aring->nsmpl;
    wpos = aring->wpos;
    rpos = aring->rpos;
    written = aring->written;
    read = aring->read;
    inf->sfreq = sfreq;
    inf->nbit = nbit;
    inf->nch = nch;
    inf->written = written;
    inf->read = read;
    if (written <= read) {
        inf->len = 0;
        inf->area[0] = SFBUF_NOSMPL(nbit);
        inf->area[1] = SFBUF_NOSMPL(nbit);
        /* @hack spelling: a goto into the next arm, which has an empty ring
         * share that arm's stores of an empty wrap (BRA 0x8ced7bd2 at
         * 0x8ced7ba2). with the three stores in the empty ring's arm in place
         * of the goto, the object runs 12 bytes long, and 98 of 119 words
         * differ */
        goto nowrap;
    } else if (rpos < wpos) {
        inf->len = wpos - rpos;
        inf->area[0] = SFBUF_SMPL(nbit, plane0, rpos);
        inf->area[1] = SFBUF_SMPL(nbit, plane1, rpos);
    nowrap:
        inf->wraplen = 0;
        inf->wrap[0] = SFBUF_NOSMPL(nbit);
        inf->wrap[1] = SFBUF_NOSMPL(nbit);
    } else {
        inf->len = nsmpl - rpos;
        inf->area[0] = SFBUF_SMPL(nbit, plane0, rpos);
        inf->area[1] = SFBUF_SMPL(nbit, plane1, rpos);
        inf->wraplen = wpos;
        inf->wrap[0] = SFBUF_SMPL(nbit, plane0, 0);
        inf->wrap[1] = SFBUF_SMPL(nbit, plane1, 0);
    }
    return 0;
}

/* count nsmpl samples read at rpos. a read past the samples written still
 * counts, and returns SFBUF_ERR_ARINGUNDER */
Sint32 SFBUF_AringAddRead(struct sfd_handle *hn, Sint32 bufno, Sint32 nsmpl)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;
    struct sfbuf_aring *aring;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    if (!buf->has_area) {
        return SFLIB_SetErr(hn, SFBUF_ERR_NOAREA);
    }
    aring = &buf->u.aring;
    aring->rpos += nsmpl;
    if (aring->rpos >= aring->nsmpl) {
        aring->rpos -= aring->nsmpl;
    }
    aring->read += nsmpl;
    if (aring->read > aring->written) {
        return SFLIB_SetErr(hn, SFBUF_ERR_ARINGUNDER);
    }
    return 0;
}

/* a queue of pictures takes no writes. SFBUF_VfrmGetWrite and
 * SFBUF_VfrmAddWrite change nothing, and return 0 */
Sint32 SFBUF_VfrmGetWrite(struct sfd_handle *hn, Sint32 bufno, Sint32 arg)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    /* @hack redundant: an empty test of has_area, which SHC keeps as its load
     * and a TST R1,R1 that no branch reads (0x8ced7c90, 0x8ced7c92). without
     * it, SHC keeps nothing but the return, the object runs 12 bytes short, and
     * 11 of 11 words differ */
    if (!buf->has_area) {
    }
    return 0;
}

Sint32 SFBUF_VfrmAddWrite(struct sfd_handle *hn, Sint32 bufno, Sint32 arg)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    /* @hack redundant: an empty test of has_area, which SHC keeps as its load
     * and a TST R1,R1 that no branch reads (0x8ced7ca6, 0x8ced7ca8). without
     * it, SHC keeps nothing but the return, the object runs 12 bytes short, and
     * 10 of 11 words differ */
    if (!buf->has_area) {
    }
    return 0;
}

/* a queue with no area is read through the stage that writes it.
 * SFBUF_VfrmGetRead and SFBUF_VfrmAddRead call that stage's slot of the same
 * name with arg, and return its result. a read of a queue with an area changes
 * nothing, and returns 0 */
Sint32 SFBUF_VfrmGetRead(struct sfd_handle *hn, Sint32 bufno, Sint32 arg)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    if (!buf->has_area) {
        return SFTRN_CallTrtTrif(hn, buf->wtrno, SFD_SLOT_GET_READ, arg, 0);
    }
    return 0;
}

Sint32 SFBUF_VfrmAddRead(struct sfd_handle *hn, Sint32 bufno, Sint32 arg)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    if (!buf->has_area) {
        return SFTRN_CallTrtTrif(hn, buf->wtrno, SFD_SLOT_ADD_READ, arg, 0);
    }
    return 0;
}

/* hand buffer bufno's area back to the caller. *area gets it, and *size its
 * whole size, a ring's margin and delimiter table included. the buffer is left
 * with no area */
void SFBUF_TakeBuf(struct sfd_handle *hn, Sint32 bufno, char **area,
                   Sint32 *size)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;
    struct sfbuf_ring *ring;
    struct sfbuf_vfrm *vfrm;
    struct sfbuf_aring *aring;
    Sint32 type;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    buf->has_area = FALSE;
    type = buf->type;
    switch (type) {
    case SFBUF_TYPE_RING:
        ring = &buf->u.ring;
        *area = ring->top;
        *size = ring->size + ring->margin + (ring->dlm_nword * sizeof(Sint32));
        break;
    case SFBUF_TYPE_VFRM:
        vfrm = &buf->u.vfrm;
        *area = vfrm->top;
        *size = vfrm->size;
        break;
    case SFBUF_TYPE_ARING:
        aring = &buf->u.aring;
        *area = aring->top;
        *size = aring->size;
        break;
    }
    /* @hack redundant: a return where control returns anyway, which gives the
     * default arm and the queue's arm their own RTS (0x8ced7d54, 0x8ced7d76),
     * and puts the pool after SFBUF_SetPrepFlg. without it, both arms branch
     * to one RTS, the pool follows this function, the object runs 2 bytes
     * long, and 4 of 43 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

void SFBUF_SetPrepFlg(struct sfd_handle *hn, Sint32 bufno, Bool flag)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    buf->prep = flag;
}

Bool SFBUF_GetPrepFlg(struct sfd_handle *hn, Sint32 bufno)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    return buf->prep;
}

void SFBUF_SetTermFlg(struct sfd_handle *hn, Sint32 bufno, Bool flag)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    buf->term = flag;
}

Bool SFBUF_GetTermFlg(struct sfd_handle *hn, Sint32 bufno)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    return buf->term;
}
