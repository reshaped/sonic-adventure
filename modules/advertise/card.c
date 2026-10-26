/* the menus' memory card access. the menus run the backup library in a work
 * area of their own. they look on the cards for the game's save and the
 * voice files */
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

#include "advertise/card.h"
#include "object/common.h"
#include "save/card.h"

#define VOICE_FILES 8 /* SONICADV_V01 to SONICADV_V08 */

/* the module's end, past its bss, which bounds.src holds */
extern char *advertise_module_end;

static void card_open_done(void);

static char *voice_files[VOICE_FILES] = {
    "SONICADV_V01", "SONICADV_V02", "SONICADV_V03", "SONICADV_V04",
    "SONICADV_V05", "SONICADV_V06", "SONICADV_V07", "SONICADV_V08",
};
/* a card that holds SONICADV_V09 counts as holding all eight voice files */
static char *all_voices_file[1] = {"SONICADV_V09"};

static struct card_drive advertise_card_drives[CARD_DRIVES];
static Bool card_ready;

void card_open(void)
{
    void *work = (void *)(((Uint32)advertise_module_end + 31) & ~31);

    card_finish();
    memset(advertise_card_drives, 0, sizeof(advertise_card_drives));
    card_ready = FALSE;
    buInit(BUD_CAPACITY_1MB, BUD_USE_DRIVE_ALL, work, card_open_done);
    while (!card_ready) {
    }
}

void card_close(void)
{
    card_init();
}

enum card_find card_find_save(Uint8 drive)
{
    Sint32 result;

    if (buIsReady(drive)) {
        result = buIsExistFile(drive, "SONICADV_SYS");
        if (result == (Sint32)BUD_ERR_FILE_NOT_FOUND) {
            return CARD_NO_SAVE;
        }
        if (result == BUD_ERR_OK) {
            return CARD_SAVE;
        }
    }
    return CARD_NOT_READY;
}

void card_pick_summary(void)
{
    SYS_RTC_DATE date;
    char found[VOICE_FILES];
    int file;
    int drive;
    int i;
    Bool any = FALSE;
    int count;

    for (file = 0; file < VOICE_FILES; file++) {
        found[file] = 0;
        for (drive = 0; drive < CARD_DRIVES; drive++) {
            if (buIsExistFile(drive, voice_files[file]) == BUD_ERR_OK) {
                found[file] = 1;
                any = TRUE;
            }
        }
    }
    for (drive = 0; drive < CARD_DRIVES; drive++) {
        if (buIsExistFile(drive, all_voices_file[0]) == BUD_ERR_OK) {
            for (i = 0; i < VOICE_FILES; i++) {
                found[i] = 1;
                any = TRUE;
            }
        }
    }
    /* @hack redundant: a store of FALSE to any that never runs. SHC then
     * gives the function a 36-byte frame, with found's address in R8, 0 in R11
     * and 8 in R10 (0x8c902328 to 0x8c902338). without it, the frame takes 32
     * bytes, the function runs 2 bytes short, card_open_done moves, and 134 of
     * 144 words differ */
    if (0) {
        any = FALSE;
    }
    if (!any) {
        file = 0;
    } else {
        syRtcGetDate(&date);
        file = 0;
        count = date.second;
        for (;;) {
            file = (file + 1) % VOICE_FILES;
            while (!found[file]) {
                file = (file + 1) % VOICE_FILES;
            }
            if (--count < 0) {
                break;
            }
        }
        file++;
    }
    menu_state.summary[0] = file;
}

static void card_open_done(void)
{
    buDisableServer();
    buSetCompleteCallback(card_complete);
    buSetProgressCallback(card_progress);
    card_ready = TRUE;
}
