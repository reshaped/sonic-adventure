/* middleware GSC, as far as the sources use it
 *
 * the GSC reads files from the disc into stream joints, a few sectors at a
 * time. a joint's free line is the room the GSC reads into, and its data line
 * the bytes read and not yet taken (sj.h) */
#ifndef SG_MWGSC_H
#define SG_MWGSC_H
#include <sg_ov.h>
#include <sg_xpt.h>
#include <sj.h>

#define MWGSC_SCT_LEN 2048

/* the library handles of the GSC and of the ring buffer joints it reads into,
 * which the movie module enters in the overlay library table */
extern OVS_LIB_OBJ mwg_gsc_libhdl;
extern OVS_LIB_OBJ mwg_sj_rbf_libhdl;

Bool mwGscIsHnNull(void *hn);
/* record code as the last error, and return it. a nonzero code also goes to the
 * error function, if one is set */
Sint32 mwGscSetErrCode(Sint32 code);

/* store in *size the bytes on the joint's free line, or on its data line, and
 * return 0. for a NULL joint or a negative count, it returns a negative error
 * code and leaves *size as it was */
Sint32 mwGscSjGetFreeBsize(SJ sj, Sint32 *size);
Sint32 mwGscSjGetDataBsize(SJ sj, Sint32 *size);
/* the sectors to read next: as many whole sectors as the data line has room
 * for below bufsize bytes, at most limit. when the free line can give them as
 * one chunk, the call stores their count in *sectors and returns 1. otherwise
 * it returns 0, with the whole sectors the free line can give in *sectors. it
 * stores 0 there when no sector fits or the data line can't be measured */
Sint32 mwGscSjIsGetFreeChunk(SJ sj, Sint32 limit, Sint32 bufsize,
                             Sint32 *sectors);
/* take a chunk of sectors * MWGSC_SCT_LEN bytes from the free line into *ck,
 * and return 1. when the line gives a shorter chunk, the call returns 0 and
 * records an error with mwGscSetErrCode. that chunk is neither stored in *ck
 * nor given back to the joint */
Sint32 mwGscSjGetFreeChunk(SJ sj, Sint32 sectors, SJCK *ck);
/* put the first nbyte bytes of *ck on the data line, and leave the rest in
 * *ck */
void mwGscSjPutDataChunk(SJ sj, Sint32 nbyte, SJCK *ck);
/* return a nonempty *ck to the free line, and empty *ck */
void mwGscSjUngetFreeChunk(SJ sj, SJCK *ck);
void mwGscSjReset(SJ sj);

#endif
