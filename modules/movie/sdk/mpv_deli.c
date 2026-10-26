/* MPEG video start code search and classification */
#include <sg_xpt.h>

#include "movie/sdk/mpv.h"

/* the start codes, each as a big-endian word, its 00 00 01 included */
#define PICTURE_START_CODE 0x100
#define SLICE_START_CODE_FIRST 0x101
#define SLICE_START_CODE_LAST 0x1af
#define USER_DATA_START_CODE 0x1b2
#define SEQUENCE_HEADER_CODE 0x1b3
#define EXTENSION_START_CODE 0x1b5
#define SEQUENCE_END_CODE 0x1b7
#define GROUP_START_CODE 0x1b8

Sint32 MPV_CheckDelim(Uint8 *buf)
{
    /* @hack spelling: p, a copy of buf, which SHC moves to R5 to build the
     * code in R4, MOV R4,R5 at 0x8ced5268; through buf, the function runs 2
     * bytes short and 60 of its 61 words differ */
    Uint8 *p = buf;
    Sint32 code;

    code = *p++;
    code <<= 8;
    code |= *p++;
    code <<= 8;
    code |= *p++;
    code <<= 8;
    code |= *p;
    if (code == PICTURE_START_CODE) {
        code = MPV_DELIM_PICTURE;
    } else if (code == SLICE_START_CODE_FIRST) {
        code = MPV_DELIM_FIRST_SLICE | MPV_DELIM_SLICE;
    } else if (code > SLICE_START_CODE_FIRST && code <= SLICE_START_CODE_LAST) {
        code = MPV_DELIM_SLICE;
    } else if (code == USER_DATA_START_CODE) {
        code = MPV_DELIM_USER_DATA;
    } else if (code == SEQUENCE_HEADER_CODE) {
        code = MPV_DELIM_SEQUENCE;
    } else if (code == EXTENSION_START_CODE) {
        code = MPV_DELIM_EXTENSION;
    } else if (code == SEQUENCE_END_CODE) {
        code = MPV_DELIM_SEQUENCE_END;
    } else if (code == GROUP_START_CODE) {
        code = MPV_DELIM_GOP;
    } else {
        code = 0;
    }
    return code;
}

/* state counts the bytes of a start code read so far, from its end back: the
 * kind's byte, then 01, then 00. a fourth, 00, completes the code */
Uint8 *MPV_BsearchDelim(Uint8 *buf, Sint32 len, Sint32 mask)
{
    Uint8 *start;
    /* @hack redundant: register on state, which also gives its values 1 to 3
     * registers of their own, MOV #3,R9, MOV #2,R10 and MOV #1,R11 at
     * 0x8ced52fa to 0x8ced5306; without it, 74 of 77 words differ */
    register Sint32 state;
    char c;

    state = 0;
    start = buf - len;

    while (start < buf) {
        buf--;
        c = *buf;
        switch (state) {
        case 0:
            /* the kind's byte, whatever its value */
            state = 1;
            break;
        case 1:
            if (c == 1) {
                state = 2;
            }
            break;
        case 2:
            if (c == 0) {
                state = 3;
            } else if (c != 1) {
                state = 1;
            }
            break;
        case 3:
            if (c == 0) {
                if (MPV_CheckDelim(buf) & mask) {
                    return buf;
                }
                state = 0;
            } else if (c != 1) {
                state = 1;
            } else {
                state = 2;
            }
            break;
        }
    }
    return NULL;
}

/* state counts the bytes of 00 00 01 read so far */
Uint8 *MPV_SearchDelim(Uint8 *buf, Sint32 len, Sint32 mask)
{
    Uint8 *end;
    Uint8 *code;
    /* @hack redundant: register on state and c, which also gives state's
     * values 1 to 3 registers of their own, MOV #3,R9, MOV #1,R8 and MOV #2,R10
     * at 0x8ced53a6 to 0x8ced53b4; without it, 73 of 77 words differ */
    register Sint32 state;
    register char c;

    state = 0;
    end = buf + len;

    while (buf < end) {
        c = *buf++;
        switch (state) {
        case 0:
            if (c == 0) {
                state = 1;
            }
            break;
        case 1:
            if (c == 0) {
                state = 2;
            } else {
                state = 0;
            }
            break;
        case 2:
            if (c == 1) {
                state = 3;
            } else if (c != 0) {
                state = 0;
            }
            break;
        case 3:
            /* c was the kind's byte, the fourth of the code */
            code = buf - 4;
            if (MPV_CheckDelim(code) & mask) {
                return code;
            }
            state = 0;
            break;
        }
    }
    return NULL;
}

/* state counts the bytes of 00 00 01 copied so far */
Uint8 *MPV_CopyStm(Uint8 *dst, Uint8 *src, Sint32 len, Sint32 mask)
{
    Uint8 *end;
    /* @hack redundant: register on state and c, which also gives state's
     * values 1 to 3 registers of their own, MOV #2,R10, MOV #3,R9 and MOV
     * #1,R11 at 0x8ced543e to 0x8ced544e; without it, 74 of 77 words differ */
    register Sint32 state;
    register char c;

    state = 0;
    end = src + len;

    while (src < end) {
        c = *src++;
        *dst++ = c;
        switch (state) {
        case 0:
            if (c == 0) {
                state = 1;
            }
            break;
        case 1:
            if (c == 0) {
                state = 2;
            } else {
                state = 0;
            }
            break;
        case 2:
            if (c == 1) {
                state = 3;
            } else if (c != 0) {
                state = 0;
            }
            break;
        case 3:
            if (MPV_CheckDelim(src - 4) & mask) {
                return src;
            }
            state = 0;
            break;
        }
    }
    return src;
}
