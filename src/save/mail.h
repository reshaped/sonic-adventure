/* mail: pages written to the memory card for the browser to post, each
 * with a Chao or a record */
#ifndef SAVE_MAIL_H
#define SAVE_MAIL_H
#include <sg_xpt.h>

struct chao;

/* the boot ROM's console record, at the address kbCfgGetId returns: the
 * console's 8-byte id, and its area as one character */
struct console_record {
    Uint8 id[8];
    Uint8 unk_08[2];
    char area;
};

/* TRUE if the flash memory holds a set of the browser's internet settings.
 * a flash call that fails gives FALSE */
Bool save_isp_found(void);
/* TRUE for an ASCII letter or digit */
Bool mail_is_alnum(int ch);

/* each writer below builds a page at WORK_AREA, over what is there. it
 * writes the page with save_write_file to save_drive's card, then plays
 * SE_RING. it returns TRUE once the write is tried, whatever its result.
 * without internet settings, or if a flash call fails, it returns FALSE,
 * with the page partly built and nothing written. the first argument is
 * ignored */

/* write SONICADV_H00, a page that carries *chao. chao must address a
 * readable struct chao */
Bool save_write_chao_mail(int unused, struct chao *chao);
/* write SONICADV_H01, a page that carries the save the game plays,
 * saves[save_slot] */
Bool save_write_player_mail(int unused);
/* write SONICADV_H02, a page that carries a race. race holds its act,
 * character, minutes and seconds, a byte each from the low byte up. the
 * mail's time is minutes * 3600 + seconds * 60 + frames' low byte */
Bool save_write_cart_mail(int unused, int race, int frames);
/* write SONICADV_H03, a page that carries a downloaded mission's id and
 * time, and the character playing. it is a SAVE_FILE_EVENT, whose write may
 * still run as the writer returns. until the write ends, the file's image 1
 * MB into WORK_AREA must stay as it is */
Bool save_write_mission_mail(int unused, int mission, int time);

#endif
