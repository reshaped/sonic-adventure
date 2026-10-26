/* the parser's reading of pack, system and packet headers, a bit field at a
 * time */
#include <sg_xpt.h>

#include "movie/sdk/mps.h"

Bool mpsdec_DecOneHd(struct mps_handle *mps, Uint8 *buf, Sint32 nbyte,
                     Sint32 *used, Sint32 *dlm);
void mpsdec_DecPackHd(struct mps_pack_hd *pack, const Uint8 *buf, Sint32 nbyte,
                      Sint32 *used);
void mpsdec_DecSysHd(struct mps_sys_hd *sys, const Uint8 *buf, Sint32 nbyte,
                     Sint32 *used);
void mpsdec_DecPketHd(struct mps_pket_hd *pket, const Uint8 *buf, Sint32 nbyte,
                      Sint32 *used);

/* an MPEG-1 pack header's bytes, its start code included */
#define PACK_HD_BYTES 12
/* a packet's start code and length field, the bytes its length doesn't
 * count */
#define PKET_PREFIX_BYTES 6

/* the stream IDs of private stream 1, padding and private stream 2 */
#define STREAM_PRIVATE_1 0xbd
#define STREAM_PADDING 0xbe
#define STREAM_PRIVATE_2 0xbf

/* the bit reader works on four locals of the function that expands it. cur
 * holds the current word's unread bits at its top, and next the word after
 * it. nbit counts the current word's bits already read, and p points past
 * next. the words are big-endian */

/* load the word at p into w, and move p past it */
/* @hack type: the first byte read as an Sint8, whose sign extension the three
 * shifts drop, which spares its EXTU.B (MOV.B @R6+,R13 at 0x8ced9a8c); read as
 * a Uint8, SHC zero-extends it, and 1938 of 2003 words differ in the three
 * functions that read headers */
#define LOAD_WORD(w)                                                           \
    {                                                                          \
        (w) = (Uint32)(Sint8)p[0];                                             \
        p++;                                                                   \
        (w) <<= 8;                                                             \
        (w) |= *p;                                                             \
        p++;                                                                   \
        (w) <<= 8;                                                             \
        (w) |= *p;                                                             \
        p++;                                                                   \
        (w) <<= 8;                                                             \
        (w) |= *p;                                                             \
        p++;                                                                   \
    }

/* read the next n bits into v, loading the next word when cur runs out */
#define GET_BITS_STEP(n, v)                                                    \
    {                                                                          \
        if (nbit >= 32 - (n)) {                                                \
            nbit -= 32 - (n);                                                  \
            if (nbit != 0) {                                                   \
                cur |= next >> ((n) - nbit);                                   \
                (v) = cur >> (32 - (n));                                       \
                cur = next << nbit;                                            \
            } else {                                                           \
                (v) = cur >> (32 - (n));                                       \
                cur = next;                                                    \
            }                                                                  \
            LOAD_WORD(next);                                                   \
        } else {                                                               \
            nbit += (n);                                                       \
            (v) = cur >> (32 - (n));                                           \
            cur <<= (n);                                                       \
        }                                                                      \
    }
#define GET_BITS(n, v)                                                         \
    do                                                                         \
        GET_BITS_STEP(n, v)                                                    \
    while (0)

/* @hack spelling: GET_BITS, with v shifted in a statement of its own, which
 * keeps cur in R13 and next in R14 in mpsdec_DecPackHd, its one user
 * (0x8ced9a8c, 0x8ced9aa0); with GET_BITS for the rate, the two trade, and 151
 * of 362 words differ */
#define GET_BITS_IN(n, v)                                                      \
    do {                                                                       \
        if (nbit >= 32 - (n)) {                                                \
            nbit -= 32 - (n);                                                  \
            if (nbit != 0) {                                                   \
                cur |= next >> ((n) - nbit);                                   \
                (v) = cur;                                                     \
                (v) >>= 32 - (n);                                              \
                cur = next << nbit;                                            \
            } else {                                                           \
                (v) = cur;                                                     \
                (v) >>= 32 - (n);                                              \
                cur = next;                                                    \
            }                                                                  \
            LOAD_WORD(next);                                                   \
        } else {                                                               \
            nbit += (n);                                                       \
            (v) = cur;                                                         \
            (v) >>= 32 - (n);                                                  \
            cur <<= (n);                                                       \
        }                                                                      \
    } while (0)

#define SKIP_BITS(n)                                                           \
    do {                                                                       \
        nbit += (n);                                                           \
        if (nbit >= 32) {                                                      \
            nbit -= 32;                                                        \
            cur = next << nbit;                                                \
            LOAD_WORD(next);                                                   \
        } else {                                                               \
            cur <<= (n);                                                       \
        }                                                                      \
    } while (0)

/* read one bit into v, as TRUE or FALSE */
#define GET_FLAG(v)                                                            \
    do {                                                                       \
        (v) = (Sint32)cur < 0;                                                 \
        if (nbit == 31) {                                                      \
            nbit = 0;                                                          \
            cur = next;                                                        \
            LOAD_WORD(next);                                                   \
        } else {                                                               \
            nbit++;                                                            \
            cur <<= 1;                                                         \
        }                                                                      \
    } while (0)

/* read the next n bits into v without consuming them */
#define PEEK_BITS(n, v)                                                        \
    do {                                                                       \
        (v) = cur >> (32 - (n));                                               \
        if (nbit > 32 - (n)) {                                                 \
            (v) |= next >> (64 - (n) - nbit);                                  \
        }                                                                      \
    } while (0)

/* read a time stamp into v. it skips 4 bits, then reads the stamp's 33 bits in
 * parts of 3, 15 and 15, each followed by a marker bit. v gets the stamp
 * divided by 4, built in the expanding function's hi, mid and lo */
#define GET_TIME(v)                                                            \
    do {                                                                       \
        SKIP_BITS(4);                                                          \
        GET_BITS(3, hi);                                                       \
        SKIP_BITS(1);                                                          \
        GET_BITS(15, mid);                                                     \
        SKIP_BITS(1);                                                          \
        GET_BITS(15, lo);                                                      \
        SKIP_BITS(1);                                                          \
        (v) = (hi << 28) | (mid << 13) | (lo >> 2);                            \
    } while (0)

void MPSDEC_Init(void)
{
}

void MPSDEC_Finish(void)
{
}

Sint32 MPS_DecHd(struct mps_handle *mps, Uint8 *buf, Sint32 nbyte, Sint32 *used,
                 Sint32 *dlm)
{
    Sint32 len;
    Sint32 found;
    Bool more;
    Sint32 kind;

    *used = 0;
    *dlm = 0;
    if (MPSLIB_CheckHn(mps) != 0) {
        return MPSLIB_SetErr(NULL, MPS_ERR_HN);
    }
    while (nbyte >= 4) {
        more = mpsdec_DecOneHd(mps, buf, nbyte, &len, &found);
        *dlm |= found;
        buf += len;
        nbyte -= len;
        *used += len;
        if (!more) {
            break;
        }
    }
    if (*dlm & MPS_DLM_SYS) {
        /* @hack spelling: the system header read through sys, which takes its
         * address in R4 (ADD #28,R4 at 0x8ced9982) and keeps mps in R13 and
         * nbyte in R14; read through mps, the two trade, and 54 of 95 words
         * differ */
        struct mps_sys_hd *sys = &mps->sys;

        if (sys->audio_bound != 0) {
            kind = MPS_SYS_AUDIO;
        } else if (sys->video_bound != 0) {
            kind = MPS_SYS_VIDEO;
        } else {
            kind = MPS_SYS_OTHER;
        }
        mps->sys_by_kind[kind] = mps->sys;
    }
    return 0;
}

/* read the header at buf: *used gets its bytes, and *dlm its MPS_DLM_* kind.
 * return TRUE for a pack or system header, which another may follow, and else
 * FALSE */
Bool mpsdec_DecOneHd(struct mps_handle *mps, Uint8 *buf, Sint32 nbyte,
                     Sint32 *used, Sint32 *dlm)
{
    Bool more = FALSE;

    *used = 0;
    *dlm = MPSDEL_CheckDelim(buf);
    switch (*dlm) {
    case MPS_DLM_PACK:
        mpsdec_DecPackHd(&mps->pack, buf, nbyte, used);
        more = TRUE;
        break;
    case MPS_DLM_SYS:
        mpsdec_DecSysHd(&mps->sys, buf, nbyte, used);
        more = TRUE;
        break;
    case MPS_DLM_PKET:
        mpsdec_DecPketHd(&mps->pket, buf, nbyte, used);
        break;
    /* @hack redundant: the end's case, which shares the default's code, keeps
     * its test (CMP/EQ R1,R0 at 0x8ced9a04); without it, 43 of 66 words differ
     */
    case MPS_DLM_END:
    default:
        break;
    }
    return more;
}

void mpsdec_DecPackHd(struct mps_pack_hd *pack, const Uint8 *buf, Sint32 nbyte,
                      Sint32 *used)
{
    Uint32 mark;
    Uint32 rate;
    Sint32 hi;
    Sint32 mid;
    Sint32 lo;
    /* @hack redundant: register on p, nbit, cur and next, which gives nbit R5
     * and next R14 (ADD #4,R5 at 0x8ced9a7a, MOV.B @R6+,R14 at 0x8ced9aa0);
     * without it, the two trade, and 320 of 362 words differ */
    register Uint8 *p;
    register Sint32 nbit;
    register Uint32 cur;
    register Uint32 next;

    /* the reader starts at the aligned word that holds the byte after the
     * start code, nbit bits into it */
    p = (Uint8 *)((Uint32)(buf + 4) & ~3UL);
    /* @hack spelling: nbit built in four statements, which builds it in buf's
     * register, R5, and p in R6 (ADD #4,R5 at 0x8ced9a7a); as one expression,
     * the two trade, and 330 of 362 words differ */
    nbit = (Sint32)buf;
    nbit += 4;
    nbit -= (Sint32)p;
    nbit *= 8;
    LOAD_WORD(cur);
    LOAD_WORD(next);
    cur <<= nbit;
    /* an MPEG-1 pack starts with 0010, and an MPEG-2 pack with 01. mark takes
     * the pack's first two bits. the current word holds the first of them at
     * least, so where the second lies in next, next shifts right by 1 to
     * follow it */
    /* @hack spelling: mark shifted in statements of its own, as GET_BITS_IN
     * shifts v, which takes the two bits in each branch (ROTL R0 at
     * 0x8ced9ad8, 0x8ced9ae8); shifted where it is assigned, SHC takes them
     * once ahead of the test, and 300 of 362 words differ */
    if (nbit >= 30) {
        nbit -= 30;
        if (nbit != 0) {
            cur |= next >> 1;
            mark = cur;
            mark >>= 30;
            cur = next << 1;
        } else {
            mark = cur;
            mark >>= 30;
            cur = next;
        }
        LOAD_WORD(next);
    } else {
        nbit += 2;
        mark = cur;
        mark >>= 30;
        cur <<= 2;
    }
    SKIP_BITS(2);
    GET_BITS(3, hi);
    SKIP_BITS(1);
    GET_BITS(15, mid);
    SKIP_BITS(1);
    GET_BITS(15, lo);
    SKIP_BITS(1);
    SKIP_BITS(1);
    GET_BITS_IN(15, rate);
    SKIP_BITS(1);
    pack->mpeg1 = mark == 0;
    pack->scr = (hi << 28) | (mid << 13) | (lo >> 2);
    pack->mux_rate = rate;
    *used = PACK_HD_BYTES;
}

void mpsdec_DecSysHd(struct mps_sys_hd *sys, const Uint8 *buf, Sint32 nbyte,
                     Sint32 *used)
{
    Uint8 *end;
    /* @hack redundant: register on p, nbit and next, which keeps a 0 in R1 for
     * each flag's nbit (MOV #0,R1 and MOV R1,R14 at 0x8ced9efc, 0x8ced9efe);
     * without it, SHC sets R14 itself, and 216 of 457 words differ */
    register Uint8 *p;
    register Sint32 nbit;
    Bool more;
    Uint32 cur;
    register Uint32 next;

    p = (Uint8 *)((Uint32)(buf + 4) & ~3UL);
    /* @hack spelling: nbit built in four statements, as in mpsdec_DecPackHd,
     * which builds it in R14 from a copy of buf (MOV R5,R14 and ADD #4,R14 at
     * 0x8ced9d52, 0x8ced9d5a); as one expression, SHC builds it in R7 and
     * copies it, and 412 of 457 words differ */
    nbit = (Sint32)buf;
    nbit += 4;
    nbit -= (Sint32)p;
    nbit *= 8;
    LOAD_WORD(cur);
    LOAD_WORD(next);
    cur <<= nbit;
    GET_BITS(16, sys->len);
    SKIP_BITS(1);
    GET_BITS(22, sys->rate_bound);
    SKIP_BITS(1);
    GET_BITS(6, sys->audio_bound);
    GET_FLAG(sys->fixed);
    GET_FLAG(sys->csps);
    GET_FLAG(sys->audio_lock);
    GET_FLAG(sys->video_lock);
    SKIP_BITS(1);
    GET_BITS(5, sys->video_bound);
    SKIP_BITS(8);
    /* skip the streams' 24-bit entries, which each start with a set bit */
    /* @hack spelling: the bit through more, which SHC builds as 0 or 1 in R4
     * and tests (MOVT R4 at 0x8ceda05e, TST R4,R4 at 0x8ceda064); tested in
     * place, SHC branches on CMP/PZ, and 61 of 457 words differ */
    for (;;) {
        more = (Sint32)cur < 0;
        if (!more) {
            break;
        }
        SKIP_BITS(24);
    }
    /* end is the first byte past the bits read. it lies (nbit + 7) / 8 bytes
     * past the start of cur's word, which is 8 bytes before p */
    /* @hack spelling: end computed in four statements, which steps p back 8
     * before the sum (ADD #-8,R6 at 0x8ceda09a); as one expression, SHC
     * subtracts the 8 after it, and 3 of 457 words differ */
    nbit += 7;
    nbit >>= 3;
    p -= 8;
    end = p + nbit;
    *used = end - buf;
    /* one stray byte before a packet's start code counts as the header's */
    if (MPSDEL_CheckDelim(end) == MPS_DLM_NONE &&
        MPSDEL_CheckDelim(end + 1) == MPS_DLM_PKET) {
        ++*used;
    }
}

void mpsdec_DecPketHd(struct mps_pket_hd *pket, const Uint8 *buf, Sint32 nbyte,
                      Sint32 *used)
{
    Sint32 bits;
    Sint32 id;
    Sint32 kind;
    Sint32 number;
    Bool scale;
    Sint32 size;
    Sint32 hi;
    Sint32 mid;
    Sint32 lo;
    Uint8 *end;
    /* @hack redundant: register on nbit, cur, next and p, which keeps the
     * reader in R14, R13, R12 and R11 (0x8ceda0f4 to 0x8ceda11e); without it,
     * SHC keeps it on the stack, and 1184 of 1184 words differ */
    register Sint32 nbit;
    register Uint32 cur;
    register Uint32 next;
    register Uint8 *p;

    p = (Uint8 *)((Uint32)(buf + 3) & ~3UL);
    nbit = ((Sint32)(buf + 3) - (Sint32)p) * 8;
    LOAD_WORD(cur);
    LOAD_WORD(next);
    cur <<= nbit;
    /* @hack spelling: the stream ID read in GET_BITS_STEP's bare scope, not
     * GET_BITS's loop of one pass, which keeps the test's MOV #24,R2 after
     * next's load (0x8ceda138); with GET_BITS, SHC moves it into the load, and
     * 29 of 1184 words differ */
    GET_BITS_STEP(8, id);
    pket->stream_id = id;
    if (id >= 0xe0 && id <= 0xef) {
        kind = MPS_PKET_VIDEO;
        number = id - 0xe0;
    } else if (id >= 0xc0 && id <= 0xdf) {
        kind = MPS_PKET_AUDIO;
        number = id - 0xc0;
    } else if (id == STREAM_PRIVATE_1) {
        kind = MPS_PKET_PRIVATE;
        number = 1;
    } else if (id == STREAM_PRIVATE_2) {
        kind = MPS_PKET_PRIVATE;
        number = 2;
    } else if (id == STREAM_PADDING) {
        kind = MPS_PKET_PADDING;
        number = 0;
    } else {
        kind = MPS_PKET_OTHER;
        number = 0;
    }
    pket->kind = kind;
    pket->number = number;
    GET_BITS(16, pket->len);
    /* the header of padding and of private stream 2 ends with the length */
    if (id == STREAM_PRIVATE_2 || id == STREAM_PADDING) {
        *used = PKET_PREFIX_BYTES;
        pket->data_len = pket->len;
        return;
    }
    /* skip the stuffing bytes, each 0xff */
    for (;;) {
        PEEK_BITS(8, bits);
        if (bits != 0xff) {
            break;
        }
        SKIP_BITS(8);
    }
    /* 01 starts a buffer size, in units of 128 bytes, or of 1024 when its
     * scale bit is set */
    PEEK_BITS(2, bits);
    if (bits == 1) {
        SKIP_BITS(2);
        GET_FLAG(scale);
        GET_BITS(13, size);
        size <<= 7;
        if (scale) {
            size <<= 3;
        }
        pket->buf_size = size;
    }
    /* 0010 starts a presentation time stamp, and 0011 both stamps. anything
     * else is taken as the 0000 1111 of neither */
    PEEK_BITS(4, bits);
    if (bits == 2) {
        GET_TIME(pket->pts);
        pket->dts = MPS_UNREAD;
    } else if (bits == 3) {
        GET_TIME(pket->pts);
        GET_TIME(pket->dts);
    } else {
        SKIP_BITS(8);
        pket->pts = MPS_UNREAD;
        pket->dts = MPS_UNREAD;
    }
    /* end is the first byte past the bits read. it lies (nbit + 7) / 8 bytes
     * past the start of cur's word, which is 8 bytes before p */
    /* @hack type: p - 8 added to the byte count as an Sint32, which subtracts
     * the 8 from a copy of p (MOV R11,R3 and ADD #-8,R3 at 0x8cedaa0c); as a
     * pointer sum, SHC adds p first, and 22 of 1184 words differ */
    end = (Uint8 *)((Sint32)(p - 8) + ((nbit + 7) >> 3));
    *used = end - buf;
    pket->data_len = pket->len - *used + PKET_PREFIX_BYTES;
}
