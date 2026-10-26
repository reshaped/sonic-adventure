/* controller sampling and game input */
#ifndef SYSTEM_PAD_H
#define SYSTEM_PAD_H
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#define PAD_PORTS 4

/* a stick as the game reads it. ang is its direction less the camera's
 * turn about y, from 0 to 0xffff. stroke is how far it's pushed: r cubed
 * over PAD_STICK_REACH cubed, r the length of its x and y, each less the
 * dead zone. within the dead zone on both axes, both are 0 */
struct pad_stick {
    Angle ang;
    Float stroke;
};

/* the buttons each action takes, as PDD_DGT_ masks. by default action
 * takes B or X; jump A; choose A or Start; cancel B; and grab B, X or Y */
extern Uint32 pad_action_buttons;
extern Uint32 pad_jump_buttons;
extern Uint32 pad_choose_buttons;
extern Uint32 pad_cancel_buttons;
extern Uint32 pad_grab_buttons;
/* the buttons that pick the next camera mode and that work the debug
 * controls. both start as 0, no button */
extern Uint32 pad_camera_buttons;
extern Uint32 pad_debug_buttons;
/* the buttons a player mashes to break out of ice: each PDD_DGT_ bit but
 * C's and Z's */
#define PAD_MASH_BUTTONS                                                       \
    (PDD_DGT_TL | PDD_DGT_TR | PDD_DGT_TX | PDD_DGT_TY | PDD_DGT_KR |          \
     PDD_DGT_KL | PDD_DGT_KD | PDD_DGT_KU | PDD_DGT_ST | PDD_DGT_TA |          \
     PDD_DGT_TB)
#define PAD_DIRECTIONS (PDD_DGT_KU | PDD_DGT_KD | PDD_DGT_KL | PDD_DGT_KR)
/* a stick's dead zone: 12 in a pad's x1 or y1, shifted up 8 bits as
 * pad_read_sticks reads them */
#define PAD_DEAD_ZONE 0xc00
/* how far a stick reaches past the dead zone */
#define PAD_STICK_REACH (32768.0F - PAD_DEAD_ZONE)
/* the bits of buttons that mask holds, as a Sint32 */
/* @hack type: both sides cast to Sint32, which keeps select_step's 6 for
 * SELECT_FADE_OUT in R2 at 0x8c094c56, and its calls' addresses after it
 * in R2, R3 and R2; as Uint32s, summed over the 52 functions that use it,
 * 632 of 40759 words differ */
#define PAD_BUTTONS(buttons, mask) ((Sint32)(buttons) & (Sint32)(mask))

/* SHC lays out bss in the order of each name's first declaration. the
 * declarations from pad_records to pad_ports below thus set the order of
 * pad.c's bss. pad_repeat_scans, the one static in it, comes after them */

/* each player's input. while the pads drive the game, players 0 and 1 copy
 * per_info[0] and per_info[1] if their own pads drive them. every other
 * record holds no button, and its old keeps what its on held */
extern PDS_PERIPHERAL pad_records[8];
/* the connected pads' records, in port order, as pointers recording_update
 * may write through. when port D holds no pad, the slot after the last pad
 * points to D's record, unless no pad is connected. every other slot
 * points to a record of zeros */
extern PDS_PERIPHERAL *per_info[PAD_PORTS];
extern Uint8 dat_8c75d910[20];
/* each player's stick, or zeros while the pads don't drive the game or the
 * player's own pad doesn't drive it. pad_stick_b is pad_sticks[1] under a
 * name of its own */
extern struct pad_stick pad_sticks[8];
extern struct pad_stick pad_stick_b;
/* each player's stick as read, whether its pad drives the player or not */
extern struct pad_stick pad_sticks_read[8];
/* each pad's buttons, by its slot in per_info. pad_held holds those held,
 * with the stick's directions as the d-pad's. pad_press_repeat holds those
 * just pressed, and each held direction again as it repeats. the _live
 * masks copy them while the pads drive the game, and are 0 otherwise */
extern Uint32 pad_held[PAD_PORTS];
extern Uint32 pad_press_repeat[PAD_PORTS];
extern Uint32 pad_held_live[PAD_PORTS];
extern Uint32 pad_press_live[PAD_PORTS];
/* TRUE for each port a pad is in */
extern Bool pad_port_used[PAD_PORTS];
/* the port of each slot's pad, or -1 past the last pad */
extern int pad_ports[PAD_PORTS];

/* let the pads drive the game, or stop them. while they don't, pad_records
 * and pad_sticks are idle and the _live masks are 0. pad_is_enabled returns
 * TRUE while they do */
void pad_enable(void);
void pad_disable(void);
Bool pad_is_enabled(void);
/* let player 0's or 1's pad drive the player, or stop it. any other number,
 * such as PAD_BOTH_PLAYERS, means both. pad_player_is_enabled returns TRUE
 * while the player's pad drives it, and player must be 0 or 1 */
#define PAD_BOTH_PLAYERS 0xff
void pad_enable_player(Uint8 player);
void pad_disable_player(Uint8 player);
Bool pad_player_is_enabled(Uint8 player);
/* TRUE after the pad_scan in which a pad comes to hold A, B, X, Y and Start
 * together. unused is ignored */
Bool pad_quit_pressed(int unused);
/* read the pads into per_info, in port order, then into pad_records, the
 * sticks and the button masks. in between, while recording_on,
 * recording_update records pad 0 or plays a recording back over it */
void pad_scan(void);

#endif
