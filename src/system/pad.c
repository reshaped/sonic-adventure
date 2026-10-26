/* controller sampling and game input */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "camera/camera.h"
#include "player/player.h"
#include "progress/recorder.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* a stick pushed further than this from center, in a pad's x1 or y1,
 * presses that direction */
#define STICK_PRESS 78
/* a held direction repeats in pad_press_repeat once it has been held for
 * more than REPEAT_DELAY scans, then every REPEAT_RATE scans */
#define REPEAT_DELAY 30
#define REPEAT_RATE 6
/* the buttons a pad holds together to quit, and how many scans in a row
 * must find them held */
#define QUIT_BUTTONS                                                           \
    (PDD_DGT_TA | PDD_DGT_TB | PDD_DGT_TX | PDD_DGT_TY | PDD_DGT_ST)
#define QUIT_SCANS 1
/* the players whose sticks are read: those of the first two pads */
#define STICK_PLAYERS 2

static void pad_count_quit(void);
static void pad_read_buttons(int slot);
static void pad_read_sticks(void);

Uint32 pad_action_buttons = PDD_DGT_TB | PDD_DGT_TX;
Uint32 pad_jump_buttons = PDD_DGT_TA;
Uint32 pad_camera_buttons = 0;
Uint32 pad_choose_buttons = PDD_DGT_TA | PDD_DGT_ST;
Uint32 pad_cancel_buttons = PDD_DGT_TB;
Uint32 pad_grab_buttons = PDD_DGT_TB | PDD_DGT_TX | PDD_DGT_TY;
Uint32 pad_debug_buttons = 0;
static Uint8 pads_on = 1;
static Uint8 player_pads_on[2] = {1, 1};
static PDS_PERIPHERAL pad_none = {0};
static Sint8 quit_pressed = 0;
static Sint16 quit_held_scans = 0;

PDS_PERIPHERAL pad_records[PLAYER_COUNT];
PDS_PERIPHERAL *per_info[PAD_PORTS];
Uint8 dat_8c75d910[20];
struct pad_stick pad_sticks[PLAYER_COUNT];
struct pad_stick pad_sticks_read[PLAYER_COUNT];
Uint32 pad_held[PAD_PORTS];
Uint32 pad_press_repeat[PAD_PORTS];
Uint32 pad_held_live[PAD_PORTS];
Uint32 pad_press_live[PAD_PORTS];
Bool pad_port_used[PAD_PORTS];
int pad_ports[PAD_PORTS];
static int pad_repeat_scans[PAD_PORTS];

void pad_enable(void)
{
    pads_on = 1;
}

void pad_disable(void)
{
    pads_on = 0;
}

Bool pad_is_enabled(void)
{
    return pads_on;
}

void pad_enable_player(Uint8 player)
{
    if (player != 0 && player != 1) {
        player_pads_on[0] = 1;
        player_pads_on[1] = 1;
        return;
    }
    player_pads_on[player] = 1;
}

void pad_disable_player(Uint8 player)
{
    if (player != 0 && player != 1) {
        player_pads_on[0] = 0;
        player_pads_on[1] = 0;
        return;
    }
    player_pads_on[player] = 0;
}

Bool pad_player_is_enabled(Uint8 player)
{
    return player_pads_on[player];
}

Bool pad_quit_pressed(int unused)
{
    return quit_pressed;
}

/* set quit_pressed, for one scan, once QUIT_SCANS scans in a row have found
 * a pad holding all of QUIT_BUTTONS, one of them just pressed */
static void pad_count_quit(void)
{
    Sint8 held;

    held = 0;
    if ((per_info[0]->on & QUIT_BUTTONS) == QUIT_BUTTONS &&
        (per_info[0]->press & QUIT_BUTTONS)) {
        held = 1;
    }
    if ((per_info[1]->on & QUIT_BUTTONS) == QUIT_BUTTONS &&
        (per_info[1]->press & QUIT_BUTTONS)) {
        held = 1;
    }
    if ((per_info[2]->on & QUIT_BUTTONS) == QUIT_BUTTONS &&
        (per_info[2]->press & QUIT_BUTTONS)) {
        held = 1;
    }
    if ((per_info[3]->on & QUIT_BUTTONS) == QUIT_BUTTONS &&
        (per_info[3]->press & QUIT_BUTTONS)) {
        held = 1;
    }
    if (held == 1) {
        quit_held_scans++;
    } else {
        quit_held_scans = 0;
    }
    if (quit_held_scans >= QUIT_SCANS) {
        quit_pressed = 1;
        quit_held_scans = 0;
    } else {
        quit_pressed = 0;
    }
}

static void pad_read_buttons(int slot)
{
    int sticks;

    sticks = 0;
    if (per_info[slot] == NULL) {
        return;
    }
    if (per_info[slot]->y1 > STICK_PRESS) {
        sticks = PDD_DGT_KD;
    }
    if (per_info[slot]->y1 < -STICK_PRESS) {
        sticks |= PDD_DGT_KU;
    }
    if (per_info[slot]->x1 > STICK_PRESS) {
        sticks |= PDD_DGT_KR;
    }
    if (per_info[slot]->x1 < -STICK_PRESS) {
        sticks |= PDD_DGT_KL;
    }
    pad_press_repeat[slot] = ~pad_held[slot] & sticks;
    pad_held[slot] = sticks;
    pad_held[slot] |= per_info[slot]->on;
    pad_press_repeat[slot] |= per_info[slot]->press;
    if (pad_press_repeat[slot] & PAD_DIRECTIONS) {
        pad_repeat_scans[slot] = 0;
    } else if (pad_held[slot] & PAD_DIRECTIONS) {
        if (++pad_repeat_scans[slot] > REPEAT_DELAY) {
            pad_repeat_scans[slot] -= REPEAT_RATE;
            pad_press_repeat[slot] |= pad_held[slot] & PAD_DIRECTIONS;
        }
    } else {
        pad_repeat_scans[slot] = 0;
    }
    if (pads_on == 0) {
        pad_held_live[slot] = 0;
        pad_press_live[slot] = 0;
    } else {
        pad_held_live[slot] = pad_held[slot];
        pad_press_live[slot] = pad_press_repeat[slot];
    }
}

void pad_scan(void)
{
    int slot;
    int port;
    Uint8 player;
    Uint8 i;
    PDS_PERIPHERAL *records;
    PDS_PERIPHERAL *record;

    per_info[0] = per_info[1] = per_info[2] = per_info[3] = &pad_none;
    /* @hack spelling: the stores from pad_ports[3] down, as the image makes
     * them at 0x8c053e6e to 0x8c053e74; in port order, 4 of 646 words
     * differ */
    pad_ports[3] = -1;
    pad_ports[2] = -1;
    pad_ports[1] = -1;
    pad_ports[0] = -1;
    slot = 0;
    port = 0;
    per_info[slot] = (PDS_PERIPHERAL *)njGetPeripheral(NJD_PORT_A0);
    if ((per_info[slot]->support & PDD_DEV_CONTROLLER) == PDD_DEV_CONTROLLER) {
        pad_port_used[port] = TRUE;
        pad_ports[slot] = port;
        slot++;
    } else {
        pad_port_used[port] = FALSE;
    }
    port = 1;
    per_info[slot] = (PDS_PERIPHERAL *)njGetPeripheral(NJD_PORT_B0);
    if ((per_info[slot]->support & PDD_DEV_CONTROLLER) == PDD_DEV_CONTROLLER) {
        pad_port_used[port] = TRUE;
        pad_ports[slot] = port;
        slot++;
    } else {
        pad_port_used[port] = FALSE;
    }
    port = 2;
    per_info[slot] = (PDS_PERIPHERAL *)njGetPeripheral(NJD_PORT_C0);
    if ((per_info[slot]->support & PDD_DEV_CONTROLLER) == PDD_DEV_CONTROLLER) {
        pad_port_used[port] = TRUE;
        pad_ports[slot] = port;
        slot++;
    } else {
        pad_port_used[port] = FALSE;
    }
    port = 3;
    per_info[slot] = (PDS_PERIPHERAL *)njGetPeripheral(NJD_PORT_D0);
    if ((per_info[slot]->support & PDD_DEV_CONTROLLER) == PDD_DEV_CONTROLLER) {
        pad_port_used[port] = TRUE;
        pad_ports[slot] = port;
        slot++;
    } else {
        pad_port_used[port] = FALSE;
    }
    if (slot == 0) {
        per_info[0] = &pad_none;
    }
    recording_update();
    /* a player whose input the pads don't drive holds no button, and its old
     * keeps what its on held */
    records = pad_records;
    if (pads_on == 1) {
        for (player = 0; player < PLAYER_COUNT; player++) {
            if (player == 0 && player_pads_on[0] == 1) {
                pad_records[0] = *per_info[0];
            } else if (player == 1 && player_pads_on[1] == 1) {
                pad_records[1] = *per_info[1];
            } else {
                pad_records[player].old = pad_records[player].on;
                pad_records[player].on = 0;
                pad_records[player].off = -1;
                /* @hack spelling: release and press, and y1 and x1, stored in
                 * chains, which keep pad_scan's MOV R9,R11 at 0x8c053fca; as
                 * four single stores, it goes, and 149 of 646 words differ */
                pad_records[player].release = pad_records[player].press = 0;
                pad_records[player].y1 = pad_records[player].x1 = 0;
                pad_records[player].l = 0x8000;
                pad_records[player].r = 0x8000;
                pad_records[player].x2 = -32768;
                pad_records[player].y2 = -32768;
            }
        }
    } else {
        record = records;
        for (i = 0; i < PLAYER_COUNT; i++) {
            record->old = record->on;
            record->on = 0;
            record->off = -1;
            /* @hack spelling: release and press, and y1 and x1, stored in
             * chains, which keep pad_scan's MOV R9,R11 at 0x8c053fca; as four
             * single stores, it goes, and 149 of 646 words differ */
            record->release = record->press = 0;
            record->y1 = record->x1 = 0;
            record->l = 0x8000;
            record->r = 0x8000;
            record->x2 = -32768;
            record->y2 = -32768;
            record++;
        }
    }
    pad_count_quit();
    pad_read_sticks();
    for (slot = 0; slot < PAD_PORTS; slot++) {
        pad_read_buttons(slot);
    }
}

/* fill pad_sticks_read with each player's stick. pad_sticks gets the same
 * while the pads drive the game and the player's own pad drives it, and
 * zeros otherwise. players past STICK_PLAYERS read their sticks as
 * centered */
static void pad_read_sticks(void)
{
    int i;
    Float x;
    Float y;
    Float squared;
    Angle ang;
    Float stroke;

    for (i = 0; i < PLAYER_COUNT; i++) {
        if (i < STICK_PLAYERS) {
            x = (Float)(per_info[i]->x1 << 8);
            y = (Float)(per_info[i]->y1 << 8);
        } else {
            x = y = 0.0F;
        }
        if (x > PAD_DEAD_ZONE || x < -PAD_DEAD_ZONE || y > PAD_DEAD_ZONE ||
            y < -PAD_DEAD_ZONE) {
            if (x >= 0.0F) {
                x -= PAD_DEAD_ZONE;
            } else {
                x += PAD_DEAD_ZONE;
            }
            if (y >= 0.0F) {
                y -= PAD_DEAD_ZONE;
            } else {
                y += PAD_DEAD_ZONE;
            }
            if (camera_work != NULL) {
                ang = (Angle)(atan2f(y, x) * ANGLE_PER_RADIAN) -
                      camera_work->ang.y;
            } else {
                ang = (Angle)(atan2f(y, x) * ANGLE_PER_RADIAN);
            }
            squared = (x * x) + (y * y);
            stroke =
                squared * njSqrt(squared) *
                (1.0F / (PAD_STICK_REACH * PAD_STICK_REACH * PAD_STICK_REACH));
        } else {
            ang = 0;
            stroke = 0.0F;
        }
        /* the low 16 bits, an angle from 0 to 0xffff */
        ang = (Uint16)ang;
        pad_sticks_read[i].ang = ang;
        pad_sticks_read[i].stroke = stroke;
        /* @bug memory: for players 2 to 7, the test reads
         * player_pads_on[i] past its two bytes, by the MOV.B @(R0,R14),R3 at
         * 0x8c054548, 0x8c0546a8, 0x8c0547fc and 0x8c05495e. their sticks
         * are zero either way */
        if (pads_on == 1 && player_pads_on[i] == 1) {
            pad_sticks[i].ang = ang;
            pad_sticks[i].stroke = stroke;
        } else {
            pad_sticks[i].ang = 0;
            pad_sticks[i].stroke = 0.0F;
        }
    }
}
