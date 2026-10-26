/* the drive screen, which picks the memory card the game saves to */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "advertise/back.h"
#include "advertise/buttons.h"
#include "advertise/card.h"
#include "advertise/dialog.h"
#include "advertise/draw.h"
#include "advertise/drive_select.h"
#include "advertise/menus.h"
#include "advertise/screen.h"
#include "advertise/sound.h"
#include "advertise/textures.h"
#include "advertise/trial.h"
#include "draw/screen.h"
#include "hud/menu.h"
#include "object/common.h"
#include "save/card.h"
#include "system/pad.h"
#include "system/task.h"

#define TEXT_1_LENGTH 40
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 38
#define TEXT_4_LENGTH 26
#define TEXT_5_LENGTH 38
#define TEXT_6_LENGTH 24
#include "advertise/drive_select/text.inc"

#define SAVE_BLOCKS 10 /* the free blocks a new save needs */
/* drive_select_poll looks in the drives from its POLL_FROM'th call after
 * drive_select_open */
#define POLL_FROM 16
/* the polls in a row with no card ready that DRIVE_CHOOSING allows before
 * DRIVE_EMPTY */
#define EMPTY_POLLS 90

enum drive_state {
    DRIVE_CHOOSING,
    DRIVE_CHOSEN,
    /* read the chosen card's save, make and write a new one, or warn that the
     * card has no room for one. with no card ready, make a new save and write
     * it nowhere */
    DRIVE_LOAD,
    /* the card has fewer than SAVE_BLOCKS blocks free: go on without the
     * card, or choose again */
    DRIVE_WARNING,
    DRIVE_WARNING_CLOSE,
    /* no card ready for more than EMPTY_POLLS polls: wait for one, or go on
     * without the card on A or Start */
    DRIVE_EMPTY,
    DRIVE_WITHOUT_CARD,
    DRIVE_DONE
};

enum drive_direction { DRIVE_UP, DRIVE_DOWN, DRIVE_LEFT, DRIVE_RIGHT };

struct drive_select {
    enum screen_mode mode;
    int back;
    int next;
    Float z;
    Float fade;
    Uint32 tint;
    struct task *dialog;
    enum drive_state state;
    /* what the last poll found in each drive, by slot: port * 2 + unit */
    enum card_find drive[CARD_DRIVES];
    Sint8 any_ready; /* 1 when the last poll found a card ready */
    int empty_polls; /* polls in a row with no card ready, up to 65535 */
    Sint16 timer;
    Sint16 poll_calls;
    Uint32 phase; /* the highlight's angle, unsigned to wrap */
    /* the drive the cursor is on */
    Uint8 port;
    Uint8 unit;
    Sint8 answer;
};

/* ava_vmssel's textures */
enum {
    VMSSEL_COLUMN_TOP,
    VMSSEL_COLUMN,
    VMSSEL_BUTTON, /* the oval under a column, which holds the port's name */
    VMSSEL_NUMBER_1,
    VMSSEL_NUMBER_2,
    VMSSEL_NUMBER_3,
    VMSSEL_NUMBER_4,
    VMSSEL_NUMBER_5,
    VMSSEL_NUMBER_6,
    VMSSEL_NUMBER_7,
    VMSSEL_NUMBER_8,
    VMSSEL_PORT_A,
    VMSSEL_PORT_B,
    VMSSEL_PORT_C,
    VMSSEL_PORT_D,
    VMSSEL_SELECT, /* the frame round the cursor's drive */
    VMSSEL_TITLE,
    VMSSEL_CARD,
    VMSSEL_CARD_SHADOW
};

static const Uint8 port_textures[4] = {VMSSEL_PORT_A, VMSSEL_PORT_B,
                                       VMSSEL_PORT_C, VMSSEL_PORT_D};
static const Uint8 unit_textures[8] = {
    VMSSEL_NUMBER_1, VMSSEL_NUMBER_2, VMSSEL_NUMBER_3, VMSSEL_NUMBER_4,
    VMSSEL_NUMBER_5, VMSSEL_NUMBER_6, VMSSEL_NUMBER_7, VMSSEL_NUMBER_8};
static char *drive_warning_lines[6];
static const struct dialog_spec space_warning = {
    drive_warning_lines, 6, 1, 320.0F, 200.0F, 10.0F, -1, -16};
static char *drive_warning_lines[6] = {
    TEXT_1, TEXT_2, TEXT_3, TEXT_4, TEXT_5, TEXT_6,
};
static void previous_slot(struct drive_select *work);
static void next_slot(struct drive_select *work);
static void previous_port(struct drive_select *work);
static void next_port(struct drive_select *work);
typedef void drive_select_move(struct drive_select *work);
static drive_select_move *directions[4] = {previous_slot, next_slot,
                                           previous_port, next_port};

static struct task *drive_select_task;

static Uint8 drive_slot(struct drive_select *work)
{
    return (work->port * 2) + work->unit;
}

static void select_slot(struct drive_select *work, Uint8 slot)
{
    work->port = slot / 2;
    work->unit = slot % 2;
}

static void previous_slot(struct drive_select *work)
{
    Uint8 slot = drive_slot(work);
    slot = slot == 0 ? 7 : slot - 1;
    select_slot(work, slot);
}

static void next_slot(struct drive_select *work)
{
    Uint8 slot = drive_slot(work);
    slot = slot >= 7 ? 0 : slot + 1;
    select_slot(work, slot);
}

static void previous_port(struct drive_select *work)
{
    if (work->port == 0) {
        work->port = 3;
        work->unit = 1 - work->unit;
    } else {
        work->port--;
    }
}

static void next_port(struct drive_select *work)
{
    if (work->port == 3) {
        work->port = 0;
        work->unit = 1 - work->unit;
    } else {
        work->port++;
    }
}

/* move the cursor in direction until it reaches a drive with a card ready,
 * trying each drive once. with none ready, it ends where it started */
static void move_selection(struct drive_select *work,
                           enum drive_direction direction)
{
    int i;

    for (i = 0; i < CARD_DRIVES; i++) {
        directions[direction](work);
        if (work->drive[drive_slot(work)] != CARD_NOT_READY) {
            break;
        }
    }
}

static void drive_select_clear(struct drive_select *work)
{
    int i;

    for (i = 0; i < CARD_DRIVES; i++) {
        work->drive[i] = CARD_NOT_READY;
    }
    work->any_ready = 0;
    work->empty_polls = 0;
}

static void drive_select_open(struct drive_select *work)
{
    textures_load_screen(SCREEN_DRIVE);
    advertise_music_play(TUNE_OPTION);
    advertise_voice_summary(LINE_DRIVE);
    /* @hack spelling: copy, a copy of work. SHC then copies work to R4,
     * drive_select_clear's argument, before the two stores, and stores
     * through R4 (MOV R14,R4 at 0x8c90a300). without it, SHC stores through
     * R14 and copies work to R4 in the BSR's slot, the function runs 2 bytes
     * short, drive_select_poll moves, and 14 of 28 words differ */
    {
        struct drive_select *copy = work;
        copy->state = DRIVE_CHOOSING;
        copy->empty_polls = 0;
    }
    save_disabled = FALSE;
    drive_select_clear(work);
    work->poll_calls = 0;
}

/* count the call in poll_calls, and from the POLL_FROM'th call after
 * drive_select_open on, look in every drive */
static void drive_select_poll(struct drive_select *work)
{
    int i;
    Sint8 previous;

    /* @bug arithmetic: poll_calls is a Sint16, which the 32768th call after
     * drive_select_open wraps to -32768 (EXTS.W at 0x8c90a336 for the
     * compare). the drives then go unread for 32784 calls, until the count is
     * back at POLL_FROM */
    if (++work->poll_calls >= POLL_FROM) {
        previous = work->any_ready;
        work->any_ready = 0;
        for (i = 0; i < CARD_DRIVES; i++) {
            work->drive[i] = card_find_save(i);
            if (work->drive[i] != CARD_NOT_READY) {
                work->any_ready = 1;
            }
        }
        if (work->any_ready != previous) {
            work->empty_polls = 0;
        }
        if (work->any_ready == 0 && work->empty_polls < 65535) {
            work->empty_polls++;
        }
    }
}

static void drive_select_run(struct drive_select *work)
{
    Uint8 old_slot = drive_slot(work);

    work->phase += NJM_DEG_ANG(4);
    switch (work->state) {
    case DRIVE_CHOOSING:
        if (pad_press_repeat[0] & PDD_DGT_KL) {
            move_selection(work, DRIVE_LEFT);
        } else if (pad_press_repeat[0] & PDD_DGT_KR) {
            move_selection(work, DRIVE_RIGHT);
        } else if (pad_press_repeat[0] & PDD_DGT_KU) {
            move_selection(work, DRIVE_UP);
        } else if (pad_press_repeat[0] & PDD_DGT_KD) {
            move_selection(work, DRIVE_DOWN);
        } else if (pad_choose_pressed() && textures_ready == TRUE &&
                   work->any_ready) {
            work->timer = 8;
            se_choose();
            advertise_sound_stop();
            work->state = DRIVE_CHOSEN;
        } else if (pad_back_pressed() && textures_ready == TRUE &&
                   work->back != SCREEN_OPTIONS) {
            screen_go(work->back);
            se_cancel();
            work->state = DRIVE_DONE;
        } else if (work->empty_polls > EMPTY_POLLS) {
            work->state = DRIVE_EMPTY;
        }
        if (old_slot != drive_slot(work)) {
            se_move();
        }
        if (work->drive[drive_slot(work)] == CARD_NOT_READY) {
            move_selection(work, DRIVE_RIGHT);
        }
        break;
    case DRIVE_CHOSEN:
        if (--work->timer <= 0) {
            work->state = DRIVE_LOAD;
        }
        break;
    case DRIVE_LOAD:
        save_drive = drive_slot(work);
        if (work->any_ready) {
            if (advertise_save_file_exists()) {
                advertise_save_read();
                screen_go(work->next);
                work->state = DRIVE_DONE;
            } else if (card_free_blocks() < SAVE_BLOCKS) {
                work->dialog = dialog_create(3, &space_warning);
                advertise_menu_popup_default(DRIVE_MENU);
                work->state = DRIVE_WARNING;
            } else {
                advertise_save_reset_write();
                screen_go(work->next);
                work->state = DRIVE_DONE;
            }
        } else {
            advertise_save_reset();
            screen_go(work->next);
            work->state = DRIVE_DONE;
        }
        break;
    case DRIVE_WARNING:
        if (dialog_mode(work->dialog) == DIALOG_SHOWN) {
            /* @hack spelling: the answer assigned in the test. SHC then tests
             * the byte it stores, in R0 (EXTS.B R0,R0 at 0x8c90a6d4).
             * assigned first, SHC reads the answer back from the work, the
             * function runs longer, the functions after it move, and 101 of
             * 326 words differ */
            if ((work->answer = menu_result()) >= 0) {
                dialog_close(work->dialog);
                work->timer = 16;
                work->state = DRIVE_WARNING_CLOSE;
            }
        }
        break;
    case DRIVE_WARNING_CLOSE:
        if (work->timer == 4) {
            advertise_music_play(TUNE_OPTION);
        }
        if (--work->timer <= 0) {
            switch (work->answer) {
            case 0:
                work->state = DRIVE_WITHOUT_CARD;
                break;
            default:
                work->state = DRIVE_CHOOSING;
                break;
            }
        }
        break;
    case DRIVE_EMPTY:
        if (work->any_ready == 1) {
            /* @hack spelling: copy, a copy of work. SHC then copies work to
             * R4 and stores through R4 (MOV R14,R4 at 0x8c90a722). without
             * it, SHC stores through R14, the function runs 2 bytes short,
             * and 53 of 326 words differ */
            struct drive_select *copy = work;
            copy->state = DRIVE_CHOOSING;
            copy->empty_polls = 0;
        } else if (pad_choose_pressed() && textures_ready == TRUE) {
            work->state = DRIVE_WITHOUT_CARD;
        }
        break;
    case DRIVE_WITHOUT_CARD:
        save_disabled = TRUE;
        save_drive = 0;
        save_slot = 0;
        /* @hack spelling: mode_visits through a pointer to menu_state. SHC then
         * pools &menu_state, and reads the field at a displacement (MOV.B
         * @(12,R4),R0 at 0x8c90a752). through menu_state, the pool holds the
         * field's address, and 8 of 326 words differ */
        {
            struct menu_state *state = &menu_state;
            state->mode_visits++;
        }
        screen_go(work->back);
        work->state = DRIVE_DONE;
        break;
    case DRIVE_DONE:
        break;
    }
}

static void draw_drive(Uint32 tint, Float x, Float y, Float z)
{
    screen_quad_color(tint);
    screen_quad_draw_scaled(QUAD_CENTERED | VMSSEL_CARD_SHADOW, x, y, z,
                            1.273F * 0.8F, 1.273F * 0.8F);
    screen_quad_draw_scaled(QUAD_CENTERED | VMSSEL_CARD, x, y, z - 0.3F, 0.8F,
                            0.8F);
}

static void draw_selected_drive(Uint32 tint, Uint32 highlight, Float x, Float y,
                                Float z)
{
    screen_quad_color(tint);
    screen_quad_draw_scaled(QUAD_CENTERED | VMSSEL_CARD_SHADOW, x, y, z - 0.6F,
                            1.273F, 1.273F);
    screen_quad_draw_scaled(QUAD_CENTERED | VMSSEL_CARD, x, y, z - 0.9F, 1.0F,
                            1.0F);
    screen_quad_color(highlight);
    screen_quad_draw_scaled(QUAD_CENTERED | VMSSEL_SELECT, x, y - 8.0F,
                            z - 1.2F, 1.0F, 1.0F);
}

/* draw each drive with a card ready, translucent where the card has no save,
 * with the cursor's drive highlighted */
static void drive_select_draw_drives(struct drive_select *work)
{
    int port;
    int unit;
    Uint32 highlight;
    Uint32 tint;
    Uint8 slot;
    Float x;
    Float y;

    njSetTexture(&ava_vmssel_texlist);
    highlight = menu_blend_color(0xffffeb00, 0xffe29300,
                                 (njSin(work->phase) + 1.0F) * 0.5F);
    for (port = 0; port < PAD_PORTS; port++) {
        for (unit = 0; unit < 2; unit++) {
            /* @hack spelling: port + port, which SHC adds (ADD R12,R5 at
             * 0x8c90a91a). as port * 2, SHC shifts, x and y trade FR14 and
             * FR15, and 13 of 151 words differ */
            slot = port + port + unit;
            x = (144.0F * port) + 104.0F;
            y = (157.0F * unit) + 168.0F;
            if (work->drive[slot] != CARD_NOT_READY) {
                tint =
                    work->drive[slot] == CARD_NO_SAVE ? 0x90ffffff : 0xffffffff;
                if (drive_slot(work) == slot) {
                    draw_selected_drive(tint, highlight, x, y, work->z - 10.0F);
                } else {
                    draw_drive(tint, x, y, work->z - 10.0F);
                }
                screen_quad_color(0xffffffff);
                screen_quad_draw(QUAD_CENTERED | unit_textures[unit], x,
                                 y - 8.0F, work->z - 14.0F);
            }
        }
    }
}

/* draw a column for each port, behind its two drives */
static void drive_select_draw_ports(struct drive_select *work)
{
    int i;
    Float left;
    Float x;

    njSetTexture(&ava_vmssel_texlist);
    for (i = 0; i < PAD_PORTS; i++) {
        screen_quad_colors(
            0xff004fff,
            menu_blend_color(0xff004fff, 0xff00e0f7, 12.0F / 255.0F),
            0xff004fff,
            menu_blend_color(0xff004fff, 0xff00e0f7, 12.0F / 255.0F));
        left = 47.0F + (i * 144.0F);
        screen_quad_draw(VMSSEL_COLUMN_TOP, left, 84.0F, work->z - 8.0F);
        screen_quad_colors(
            menu_blend_color(0xff004fff, 0xff00e0f7, 12.0F / 255.0F),
            0xff00e0f7,
            menu_blend_color(0xff004fff, 0xff00e0f7, 12.0F / 255.0F),
            0xff00e0f7);
        screen_quad_draw_sized(VMSSEL_COLUMN, left, 100.0F, work->z - 8.0F,
                               128.0F, 324.0F);
        x = 104.0F + (i * 144.0F);
        screen_quad_color(0xffffffff);
        screen_quad_draw_scaled(QUAD_CENTERED | VMSSEL_BUTTON, x, 424.0F,
                                work->z - 10.0F, 1.281F, 1.281F);
        screen_quad_draw(QUAD_CENTERED | port_textures[i], x, 424.0F,
                         work->z - 12.0F);
    }
    /* @hack redundant: a call of screen_quad_draw that never runs. it keeps
     * the first column's i * 144.0F, left and x at sp+36, sp+16 and sp+4
     * (0x8c90aac6, 0x8c90aad2, 0x8c90ab32). without it, the columns take
     * other slots, one of them sp+0, the function runs 2 bytes short, the
     * functions after it move, and 281 of 485 words differ */
    if (0) {
        screen_quad_draw(0, 0.0F, 0.0F, 0.0F);
    }
    /* @hack redundant: a call of screen_quad_colors that never runs. it keeps
     * screen_quad_colors' address in R9 and screen_quad_draw's in R8
     * (0x8c90aabc, 0x8c90aae6). without it, the two trade, the function runs
     * 2 bytes short, the functions after it move, and 284 of 485 words
     * differ */
    if (0) {
        screen_quad_colors(256, 256, 256, 256);
    }
    /* @hack redundant: calls that never run, which settle the columns'
     * frame slots. without them, the slots at sp+16 and sp+24 trade, as do
     * those at sp+32 and sp+44 (0x8c90aad2, 0x8c90ac7a), and 8 of 485 words
     * differ */
    if (0) {
        screen_go(610);
        screen_go(610);
        screen_go(624);
        screen_go(624);
        screen_go(569);
        screen_go(569);
        screen_go(547);
        screen_go(547);
        screen_go(547);
        screen_go(547);
        advertise_music_play(771);
        advertise_music_play(771);
        advertise_music_play(771);
        advertise_music_play(771);
        advertise_music_play(891);
        advertise_music_play(891);
        advertise_music_play(891);
        advertise_music_play(891);
        advertise_music_play(891);
        advertise_music_play(891);
        advertise_music_play(891);
        advertise_music_play(891);
        advertise_music_play(891);
        advertise_music_play(891);
        advertise_music_play(891);
        textures_load_screen(1082);
        textures_load_screen(1082);
        textures_load_screen(1097);
        textures_load_screen(1097);
        textures_load_screen(1097);
        textures_load_screen(1104);
        textures_load_screen(1104);
        textures_load_screen(1104);
        textures_load_screen(1118);
        textures_load_screen(1118);
    }
}

/* draw the title, and the columns the drives sit in */
static void drive_select_draw_title(struct drive_select *work)
{
    screen_quad_color(0xffffffff);
    njSetTexture(&ava_vmssel_texlist);
    screen_quad_draw(VMSSEL_TITLE, 42.0F, 45.0F, work->z - 14.0F);
    screen_band_draw(0.0F, 38.0F, work->z - 8.0F, 564.0F, 41.0F);
    drive_select_draw_ports(work);
}

static void drive_select_init(struct task *task, struct screen_links *links)
{
    struct drive_select *work = task->extra;

    work->back = links->back;
    work->next = links->next;
    work->z = 57000.0F;
    work->mode = SCREEN_HIDDEN;
    work->port = 0;
    work->unit = 0;
    work->state = DRIVE_CHOOSING;
    work->empty_polls = 0;
    drive_select_clear(work);
}

static void drive_select_step(struct task *task)
{
    struct drive_select *work = task->extra;

    if (ADVERTISE_WORK()->screen == SCREEN_DRIVE &&
        work->mode == SCREEN_HIDDEN) {
        drive_select_open(work);
        work->fade = 0.0F;
        work->mode = SCREEN_OPENING;
    }
    if (ADVERTISE_WORK()->last_screen == SCREEN_DRIVE &&
        work->mode == SCREEN_SHOWN) {
        work->fade = 0.0F;
        work->mode = SCREEN_CLOSING;
    }
    if (work->mode > SCREEN_HIDDEN && work->mode < SCREEN_CLOSING) {
        drive_select_poll(work);
    }
    switch (work->mode) {
    case SCREEN_HIDDEN:
        work->tint = 0;
        break;
    case SCREEN_OPENING:
        work->tint = color_fade_in(work->fade);
        work->fade += 0.1F;
        if (work->fade >= 1.0F) {
            work->mode = SCREEN_SHOWN;
        }
        break;
    case SCREEN_SHOWN:
        work->tint = 0xffffffff;
        ADVERTISE_WORK()->shown = SCREEN_DRIVE;
        drive_select_run(work);
        break;
    case SCREEN_CLOSING:
        work->tint = color_fade_out(work->fade);
        work->fade += 0.1F;
        if (work->fade >= 1.0F) {
            textures_switch(SCREEN_DRIVE);
            ADVERTISE_WORK()->closed = 1;
            work->mode = SCREEN_HIDDEN;
        }
        break;
    }
}

static void drive_select_draw(struct task *task)
{
    struct drive_select *work = task->extra;

    if (work->tint != 0) {
        screen_blend_alpha();
        screen_quad_tint(work->tint);
        drive_select_draw_title(work);
        drive_select_draw_drives(work);
        back_draw(work->z - 4.0F);
    }
}

struct task *drive_select_create(struct screen_links *links)
{
    struct task *task;
    struct drive_select *work;

    /* @hack spelling: the task assigned in the test. SHC then tests it in R0
     * (TST R0,R0 at 0x8c90b0e4), and copies it to R14 after. assigned first,
     * it is copied to R14 and tested there, and 2 of 32 words differ */
    if ((drive_select_task = task = task_create(0, 3, drive_select_step)) !=
        NULL) {
        work = heap_calloc(1, sizeof(struct drive_select));
        task->extra = work;
        drive_select_init(task, links);
        task->step = drive_select_step;
        task->draw = drive_select_draw;
    }
    return task;
}

void drive_select_destroy(void)
{
    if (drive_select_task) {
        task_destroy(drive_select_task);
    }
}

void drive_select_back_options(void)
{
    ((struct drive_select *)drive_select_task->extra)->back = SCREEN_OPTIONS;
}
