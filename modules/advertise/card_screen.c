/* the card screen, the menus' first from the top: the game's title, after a
 * notice when no memory card is ready. Start goes on, and the title's music
 * ending starts the demo */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <sofdec.h>

#include "advertise/buttons.h"
#include "advertise/card.h"
#include "advertise/card_screen.h"
#include "advertise/dialog.h"
#include "advertise/draw.h"
#include "advertise/mode_select.h"
#include "advertise/screen.h"
#include "advertise/sound.h"
#include "advertise/textures.h"
#include "draw/screen.h"
#include "main.h"
#include "object/common.h"
#include "save/card.h"
#include "sound/stream.h"
#include "system/pad.h"
#include "system/task.h"

#define TEXT_1_LENGTH 38
#define TEXT_2_LENGTH 42
#define TEXT_3_LENGTH 22
#define TEXT_4_LENGTH 40
#define TEXT_5_LENGTH 40
#include "advertise/card_screen/text.inc"

/* ava_gtitle0's textures: the title's eight pieces, which lie as mode select
 * lays ava_title's out */
enum {
    GTITLE0_PIECE_1,
    GTITLE0_PIECE_2,
    GTITLE0_PIECE_3,
    GTITLE0_PIECE_4,
    GTITLE0_PIECE_5,
    GTITLE0_PIECE_6,
    GTITLE0_PIECE_7,
    GTITLE0_PIECE_8
};

enum card_screen_state {
    CARD_SCREEN_CHECK,
    CARD_SCREEN_NOTICE,
    CARD_SCREEN_TITLE,
    CARD_SCREEN_DONE
};

struct card_screen {
    enum screen_mode mode;
    int back;
    int next;
    Float z;
    Float fade;
    Uint32 tint;
    enum card_screen_state state;
    struct task *notice;
    Sint8 has_card;
};

static char *card_screen_notice_lines[5] = {
    TEXT_1, TEXT_2, TEXT_3, TEXT_4, TEXT_5,
};
static const struct dialog_spec card_screen_notice = {
    card_screen_notice_lines, 5, 1, 320.0F, 240.0F, 10.0F, -1, -16,
};

static struct task *card_screen_task;

static Bool card_screen_any_card(void)
{
    int drive;

    for (drive = 0; drive < CARD_DRIVES; drive++) {
        if (card_find_save(drive)) {
            return TRUE;
        }
    }
    return FALSE;
}

static void card_screen_open(struct card_screen *work)
{
    textures_load_screen(SCREEN_CARD);
    card_pick_summary();
    if (menu_state.mode_visits || menu_state.card_noticed == 1) {
        advertise_music_play(TUNE_TITLE2);
        /* @hack spelling: next set through card_screen_task, whose extra is
         * work. SHC then reads card_screen_task and stores through its extra
         * (MOV.L R0,@(8,R1) at 0x8c9086e0). through work, it stores through
         * R14, the function runs 6 bytes short, card_screen_run moves, and 26
         * of 46 words differ */
        ((struct card_screen *)card_screen_task->extra)->next = SCREEN_MODE;
        mode_select_back_card();
        work->state = CARD_SCREEN_TITLE;
    } else {
        /* @hack spelling: has_card assigned in the test. SHC then tests the
         * result in R0 (EXTS.B R0,R0 at 0x8c9086f2). assigned first, SHC
         * reads has_card back from the work, and 15 of 46 words differ */
        if ((work->has_card = card_screen_any_card())) {
            advertise_music_play(TUNE_TITLE2);
        }
        work->state = CARD_SCREEN_CHECK;
    }
}

static void card_screen_run(struct card_screen *work)
{
    switch (work->state) {
    case CARD_SCREEN_CHECK:
        if (work->has_card != 0) {
            work->state = CARD_SCREEN_TITLE;
        } else {
            menu_state.card_noticed = 1;
            work->notice = dialog_create(3, &card_screen_notice);
            work->state = CARD_SCREEN_NOTICE;
        }
        break;
    case CARD_SCREEN_NOTICE:
        if (pad_choose_pressed() && dialog_mode(work->notice) == DIALOG_SHOWN) {
            dialog_close(work->notice);
            if (!card_screen_any_card()) {
                /* @hack spelling: next set through card_screen_task, as in
                 * card_screen_open (MOV.L R0,@(8,R1) at 0x8c9087ce). through
                 * work, the function runs 6 bytes short, the functions after
                 * it move, and 72 of 116 words differ */
                ((struct card_screen *)card_screen_task->extra)->next =
                    SCREEN_MODE;
                mode_select_back_card();
                save_disabled = TRUE;
                save_drive = 0;
                save_slot = 0;
            }
            advertise_music_play(TUNE_TITLE2);
            work->state = CARD_SCREEN_TITLE;
        }
        break;
    case CARD_SCREEN_TITLE:
        if ((pad_press_repeat[0] & PDD_DGT_ST) && textures_ready == TRUE) {
            screen_go(work->next);
            se_choose();
            work->state = CARD_SCREEN_DONE;
            break;
        }
        if (pad_back_pressed() && textures_ready == TRUE) {
            break;
        }
        if (advertise_music_state() == ADXT_STAT_PLAYEND) {
            ADVERTISE_WORK()->choice = MENU_DEMO;
            screen_go(SCREEN_NONE);
            work->state = CARD_SCREEN_DONE;
        }
        break;
    case CARD_SCREEN_DONE:
        break;
    }
}

static void card_screen_draw_title(Float z)
{
    njSetTexture(&ava_gtitle0_texlist);
    screen_quad_color(0xffffffff);
    screen_quad_draw(GTITLE0_PIECE_1, 0.0F, 0.0F, z - 4.0F);
    screen_quad_draw(GTITLE0_PIECE_2, 0.0F, 256.0F, z - 4.0F);
    screen_quad_draw(GTITLE0_PIECE_3, 256.0F, 0.0F, z - 4.0F);
    screen_quad_draw(GTITLE0_PIECE_4, 256.0F, 256.0F, z - 4.0F);
    screen_quad_draw(GTITLE0_PIECE_5, 512.0F, 0.0F, z - 4.0F);
    screen_quad_draw(GTITLE0_PIECE_6, 512.0F, 128.0F, z - 4.0F);
    screen_quad_draw(GTITLE0_PIECE_7, 512.0F, 256.0F, z - 4.0F);
    screen_quad_draw(GTITLE0_PIECE_8, 512.0F, 384.0F, z - 4.0F);
}

static void card_screen_init(struct task *task, struct screen_links *links)
{
    struct card_screen *work = task->extra;

    work->back = links->back;
    work->next = links->next;
    work->z = 54000.0F;
    work->mode = SCREEN_HIDDEN;
    work->state = CARD_SCREEN_CHECK;
}

static void card_screen_step(struct task *task)
{
    struct card_screen *work = task->extra;

    if (ADVERTISE_WORK()->screen == SCREEN_CARD &&
        work->mode == SCREEN_HIDDEN) {
        card_screen_open(work);
        work->fade = 0.0F;
        work->mode = SCREEN_OPENING;
    }
    if (ADVERTISE_WORK()->last_screen == SCREEN_CARD &&
        work->mode == SCREEN_SHOWN) {
        textures_load_screen(SCREEN_CARD);
        work->fade = 0.0F;
        work->mode = SCREEN_CLOSING;
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
        ADVERTISE_WORK()->shown = SCREEN_CARD;
        card_screen_run(work);
        break;
    case SCREEN_CLOSING:
        work->tint = color_fade_out(work->fade);
        work->fade += 0.1F;
        if (work->fade >= 1.0F) {
            textures_switch(SCREEN_CARD);
            ADVERTISE_WORK()->closed = 1;
            work->mode = SCREEN_HIDDEN;
        }
        break;
    }
}

static void card_screen_draw(struct task *task)
{
    struct card_screen *work = task->extra;

    if (work->tint && work->state >= CARD_SCREEN_TITLE) {
        /* @hack spelling: the three calls as one expression. SHC then calls the
         * title's draw and returns (BSR at 0x8c908a7e). as three statements,
         * the draw becomes a tail jump, the function runs 4 bytes longer, and
         * 7 of 22 words differ */
        screen_blend_alpha(), screen_quad_tint(work->tint),
            card_screen_draw_title(work->z);
    }
}

struct task *card_screen_create(struct screen_links *links)
{
    struct task *task;
    struct card_screen *work;

    /* @hack spelling: the task assigned in the test. SHC then tests it in R0
     * (TST R0,R0 at 0x8c908ad4), and copies it to R14 after. assigned first,
     * it is copied to R14 and tested there, and 2 of 32 words differ */
    if ((card_screen_task = task =
             task_create(TASK_EXTRA, 3, card_screen_step))) {
        work = heap_calloc(1, sizeof(struct card_screen));
        task->extra = work;
        card_screen_init(task, links);
        task->step = card_screen_step;
        task->draw = card_screen_draw;
    }
    return task;
}

void card_screen_destroy(void)
{
    if (card_screen_task) {
        task_destroy(card_screen_task);
    }
}

void card_screen_next_mode(void)
{
    ((struct card_screen *)card_screen_task->extra)->next = SCREEN_MODE;
}
