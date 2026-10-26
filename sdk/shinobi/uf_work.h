/* the play history library's shared flags */
#ifndef UF_WORK_H
#define UF_WORK_H
#include <sg_xpt.h>

/* the flags every title shares, in packet 0 of the history partition.
 * ufSaveAction runs its action on a title's record only while auto_save and
 * data are set. ufReadAllHead reads a record's packets only while data is
 * set */
struct uf_user_flag {
    Uint8 auto_save;
    Uint8 version;
    Uint8 netuse;
    Uint8 data;
    Uint8 unk_04[55];
    /* a parity bit for each of the first eight bytes, which ufSaveUserFlag
     * sets */
    Uint8 parity;
};
extern struct uf_user_flag ufgUserFlag;

#endif
