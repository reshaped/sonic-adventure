/* the card menu. on it the player picks a Chao or an egg on a memory card
 * to take, or a card to put the held one on */
#ifndef AL_MAIN_CARD_MENU_H
#define AL_MAIN_CARD_MENU_H
#include <sg_xpt.h>

struct chao;

enum card_menu_mode { CARD_MENU_TAKE, CARD_MENU_PUT };

struct card_menu {
    Sint32 mode; /* an enum card_menu_mode */
    /* the panel's message, by its number in card_menu.c's message tables */
    Sint32 message;
    /* set while the menu takes input. the buttons and the panel fade in
     * once it is set, and out once it clears. so does the cursor, when
     * taking */
    Bool open;
    /* set as the menu closes, which ends the drives and the previews of
     * their Chao */
    Bool closing;
    /* the slot under the cursor. the slots stand in four rows and four
     * columns, slot n at row n % 4 and column n / 4. a drive's Chao is slot
     * 2 * drive, and its egg the slot below. putting moves the cursor by 2
     * or 4, so it stays on a Chao's slot */
    Sint32 cursor;
    Bool chosen; /* taking: a slot is chosen */
    Bool placed; /* putting: the drive at cursor / 2 is chosen */
    /* putting: TRUE when held is a Chao's, FALSE for an egg's. the caller
     * sets it for its own use, and the menu never reads it */
    Bool held_chao;
    /* TRUE once the menu's first step finds card_drives_start returning 0,
     * which it never does. the creates clear it */
    Bool failed;
    struct chao *held; /* putting: the record of the Chao or egg to put */
    struct chao *record; /* taking: the chosen slot's, in card_menu_drives */
};

extern struct card_menu card_menu;

/* card_menu_put_create and card_menu_take_create clear failed, set mode and
 * message, and create the menu's task. its first step clears open,
 * closing, cursor, chosen, placed and record. until then they hold the last
 * menu's values.
 *
 * that first step also takes the garden over. it restarts the backup
 * library with card_drives_start, sets garden_camera_fixed and alife_paused,
 * and disables player 0's pad. nothing in the menu undoes these, its end
 * included. so once the menu is done, the caller must call card_drives_end,
 * clear garden_camera_fixed and alife_paused, and enable player 0's pad, as
 * vms_step does. AL_MAIN.PRS must stay loaded until card_drives_end has run.
 *
 * each step of the menu polls the cards with card_drives_take_changes, which
 * can write to a card. a B pressed in the step in which the player chooses ends
 * the menu without setting closing. a caller that waits on closing then waits
 * for good */

/* open the menu for putting card_menu.held on a card. held must stay valid
 * while the menu runs. placed is set once the player picks a drive whose
 * card has held's slot empty. an egg's slot is the egg slot. any other
 * type's is the Chao slot, but no drive takes CHAO_PARTNER */
void card_menu_put_create(void);
/* open the menu for taking a Chao or an egg from a card. chosen is set,
 * with record the slot's record, once the player picks a slot that isn't
 * empty */
void card_menu_take_create(void);
/* show message on the panel in a task of its own, which fades in, holds for
 * 152 steps and fades out. with the task pool or the heap spent, it writes
 * the message and its place through a null task's work */
void card_menu_notice_create(Sint16 message);

#endif
