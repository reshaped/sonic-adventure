/* the options screen, which goes on to the sound test and the file screen,
 * and sets the events' messages and the speakers */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "advertise/back.h"
#include "advertise/buttons.h"
#include "advertise/card_screen.h"
#include "advertise/draw.h"
#include "advertise/drive_select.h"
#include "advertise/file_select.h"
#include "advertise/menus.h"
#include "advertise/mode_select.h"
#include "advertise/options.h"
#include "advertise/records.h"
#include "advertise/screen.h"
#include "advertise/sound.h"
#include "advertise/sound_config.h"
#include "advertise/textures.h"
#include "advertise/trial.h"
#include "draw/screen.h"
#include "hud/menu.h"
#include "system/pad.h"
#include "system/task.h"

enum option {
    OPTION_ITEM_SOUND_TEST,
    OPTION_ITEM_MESSAGES,
    OPTION_ITEM_SPEAKERS,
    OPTION_ITEM_FILE,
    OPTION_ITEM_BACK,
    OPTION_ITEMS
};
/* MESSAGES_MENU's items, and SPEAKERS_MENU's */
enum messages_item { MESSAGES_VOICE_TEXT, MESSAGES_VOICE_ONLY };
enum speakers_item { SPEAKERS_STEREO, SPEAKERS_MONO };

/* the sound test and file items wait CHOICE_STEPS steps after the choice,
 * and call screen_go for their screens at the step after those */
#define CHOICE_STEPS 10

/* the pictures' animations, in steps. the sound test picture's CD shows each of
 * its CD_TEXTURES textures for CD_STEPS. the messages picture's Sonic shows
 * each of his SONIC_TEXTURES for SONIC_STEPS. "SONIC!" shows one more of its
 * LETTERS letters every LETTER_STEPS, and none for LETTER_STEPS once all have
 * shown */
#define CD_STEPS 4
#define SONIC_STEPS 32
#define LETTER_STEPS 32
#define CD_TEXTURES 2
#define SONIC_TEXTURES 2
#define LETTERS 6

enum options_state {
    OPTIONS_CHOOSING,
    OPTIONS_SOUND_TEST,
    OPTIONS_ASK_MESSAGES,
    OPTIONS_ASK_SPEAKERS,
    OPTIONS_FILE,
    OPTIONS_BACK,
    OPTIONS_DONE
};

struct options {
    enum screen_mode mode;
    int back;
    int next;
    Float z;
    Float fade;
    Uint32 tint;
    enum options_state state;
    int cursor;
    Sint16 timer;
    Angle spin; /* the speakers' scale goes from 1 to 1.25 with its sine */
    Uint8 cd_timer;
    Uint8 cd; /* the CD texture shown */
    Uint8 sonic_timer;
    Uint8 sonic; /* Sonic's texture shown */
    Uint8 letter_timer;
    /* the last letter of "SONIC!" shown, or LETTERS while none is */
    Uint8 letters;
    Uint32 cursor_color;
    Float pulse;
    Angle blink;
    Uint8 alpha;
};

struct option_item {
    Float x, y; /* the picture's top left */
    Float label_x, label_y; /* the label's middle, and the cursor's */
    /* the scale of the label's plate, and of the cursor at a pulse of 1 */
    Float scale_x, scale_y;
    Uint8 label; /* the label's texture in ava_option */
    void (*draw)(struct options *work); /* draws the picture */
    /* the item each direction moves the cursor to, or OPTION_ITEMS where it
     * stays */
    int up, down, left, right;
};

static void options_draw_sound_test(struct options *work);
static void options_draw_messages(struct options *work);
static void options_draw_speakers(struct options *work);
static void options_draw_file(struct options *work);
static void options_draw_back(struct options *work);

static const struct option_item option_items[OPTION_ITEMS] = {
    {168.0F, 96.0F, 216.0F, 212.0F, 0.563F, 0.375F, OPTION_SOUND_TEST,
     options_draw_sound_test, OPTION_ITEMS, OPTION_ITEM_SPEAKERS, OPTION_ITEMS,
     OPTION_ITEM_MESSAGES},
    {376.0F, 96.0F, 424.0F, 212.0F, 0.563F, 0.375F, OPTION_MESSAGES,
     options_draw_messages, OPTION_ITEMS, OPTION_ITEM_FILE,
     OPTION_ITEM_SOUND_TEST, OPTION_ITEMS},
    {168.0F, 248.0F, 216.0F, 364.0F, 0.563F, 0.375F, OPTION_SPEAKERS,
     options_draw_speakers, OPTION_ITEM_SOUND_TEST, OPTION_ITEM_BACK,
     OPTION_ITEMS, OPTION_ITEM_FILE},
    {376.0F, 248.0F, 424.0F, 364.0F, 0.563F, 0.375F, OPTION_FILE,
     options_draw_file, OPTION_ITEM_MESSAGES, OPTION_ITEM_BACK,
     OPTION_ITEM_SPEAKERS, OPTION_ITEMS},
    {0.0F, 0.0F, 320.0F, 416.0F, 0.813F, 0.375F, OPTION_BACK, options_draw_back,
     OPTION_ITEM_SPEAKERS, OPTION_ITEMS, OPTION_ITEMS, OPTION_ITEMS},
};
static const Uint8 cd_textures[CD_TEXTURES] = {OPTION_SOUND_TEST_CD1,
                                               OPTION_SOUND_TEST_CD2};
static const Uint8 sonic_textures[SONIC_TEXTURES] = {OPTION_MESSAGES_SONIC1,
                                                     OPTION_MESSAGES_SONIC2};
static const Float letter_xs[LETTERS] = {19.0F, 29.0F, 42.0F,
                                         54.0F, 59.0F, 71.0F};
static const Uint8 letter_textures[LETTERS] = {
    OPTION_MESSAGES_S, OPTION_MESSAGES_O, OPTION_MESSAGES_N,
    OPTION_MESSAGES_I, OPTION_MESSAGES_C, OPTION_MESSAGES_BANG,
};

static struct task *options_task;

static void options_reset(struct options *work)
{
    work->state = OPTIONS_CHOOSING;
    work->blink = 0;
    work->pulse = 1.0F;
    work->alpha = 255;
    work->spin = 0;
    work->cd_timer = 0;
    work->cd = 0;
    work->sonic_timer = 0;
    work->sonic = 0;
    work->letter_timer = 0;
    work->letters = 0;
}

static void options_open(struct options *work)
{
    textures_load_screen(SCREEN_OPTIONS);
    advertise_music_play(TUNE_OPTION);
    advertise_voice_summary(LINE_OPTIONS);
    options_reset(work);
    if (ADVERTISE_WORK()->last_screen == SCREEN_MODE) {
        work->cursor = OPTION_ITEM_SOUND_TEST;
    }
}

static void options_animate(struct options *work)
{
    if (++work->cd_timer >= CD_STEPS) {
        work->cd_timer = 0;
        if (++work->cd >= CD_TEXTURES) {
            work->cd = 0;
        }
    }
    if (++work->sonic_timer >= SONIC_STEPS) {
        work->sonic_timer = 0;
        if (++work->sonic >= SONIC_TEXTURES) {
            work->sonic = 0;
        }
    }
    if (++work->letter_timer >= LETTER_STEPS) {
        work->letter_timer = 0;
        if (++work->letters >= LETTERS + 1) {
            work->letters = 0;
        }
    }
    work->spin += NJM_DEG_ANG(8);
}

static void options_run(struct options *work)
{
    const struct option_item *item = &option_items[work->cursor];
    int result;

    menu_cursor_step(&work->alpha, &work->blink, &work->pulse);
    work->cursor_color = 0xffff0000;
    switch (work->state) {
    case OPTIONS_CHOOSING:
        work->cursor_color = menu_blend_color(
            0xff12b4ff, 0xffff0000, (njCos(work->blink) + 1.0F) * 0.5F);
        if (pad_choose_pressed() && textures_ready == TRUE) {
            work->timer = 0;
            switch (work->cursor) {
            case OPTION_ITEM_SOUND_TEST:
                work->state = OPTIONS_SOUND_TEST;
                break;
            case OPTION_ITEM_MESSAGES: {
                int voice_only = record_voice_only();

                /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
                advertise_menu_popup(MESSAGES_MENU, voice_only, NULL);
                work->state = OPTIONS_ASK_MESSAGES;
                break;
            }
            case OPTION_ITEM_SPEAKERS: {
                int speakers;

                sound_config_open();
                speakers = sound_config_get() == SYD_CFG_STEREO
                               ? SPEAKERS_STEREO
                               : SPEAKERS_MONO;
                /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
                advertise_menu_popup(SPEAKERS_MENU, speakers, NULL);
                work->state = OPTIONS_ASK_SPEAKERS;
                break;
            }
            case OPTION_ITEM_FILE:
                work->state = OPTIONS_FILE;
                se_choose();
                break;
            case OPTION_ITEM_BACK:
                work->state = OPTIONS_BACK;
                break;
            }
        } else if (pad_back_pressed() && textures_ready == TRUE) {
            work->state = OPTIONS_BACK;
        } else {
            if ((pad_press_repeat[0] & PDD_DGT_KU) &&
                item->up != OPTION_ITEMS) {
                work->pulse = 1.5F;
                work->cursor = item->up;
                se_move();
            } else if ((pad_press_repeat[0] & PDD_DGT_KD) &&
                       item->down != OPTION_ITEMS) {
                work->pulse = 1.5F;
                work->cursor = item->down;
                se_move();
            } else if ((pad_press_repeat[0] & PDD_DGT_KL) &&
                       item->left != OPTION_ITEMS) {
                work->pulse = 1.5F;
                work->cursor = item->left;
                se_move();
            } else if ((pad_press_repeat[0] & PDD_DGT_KR) &&
                       item->right != OPTION_ITEMS) {
                work->pulse = 1.5F;
                work->cursor = item->right;
                se_move();
            }
        }
        break;
    case OPTIONS_SOUND_TEST:
        if (++work->timer > CHOICE_STEPS) {
            screen_go(SCREEN_SOUND_TEST);
            work->state = OPTIONS_DONE;
        }
        break;
    case OPTIONS_ASK_MESSAGES:
        /* @hack type: the result cast to Sint8 in the test and the switch,
         * which SHC sign-extends from its low byte (EXTS.B at 0x8c906aac and
         * 0x8c906ab6); without the casts, 21 of 321 words differ */
        if ((Sint8)(result = menu_result()) >= 0) {
            switch ((Sint8)result) {
            case MESSAGES_VOICE_TEXT:
                record_set_voice_only(0);
                advertise_save_write();
                break;
            case MESSAGES_VOICE_ONLY:
                record_set_voice_only(1);
                advertise_save_write();
                break;
            }
            options_reset(work);
        }
        break;
    case OPTIONS_ASK_SPEAKERS:
        /* @hack type: the result cast to Sint8 in the test and the switch,
         * which SHC sign-extends from its low byte (EXTS.B at 0x8c906b22 and
         * 0x8c906b28); without the casts, 77 of 321 words differ */
        if ((Sint8)(result = menu_result()) >= 0) {
            switch ((Sint8)result) {
            case SPEAKERS_STEREO:
                sound_config_set(SYD_CFG_STEREO);
                break;
            case SPEAKERS_MONO:
                sound_config_set(SYD_CFG_MONO);
                break;
            }
            sound_config_close();
            options_reset(work);
        }
        break;
    case OPTIONS_FILE:
        if (++work->timer > CHOICE_STEPS) {
            card_screen_next_mode();
            mode_select_back_card();
            drive_select_back_options();
            file_select_next_options();
            screen_go(SCREEN_DRIVE);
            work->state = OPTIONS_DONE;
        }
        break;
    case OPTIONS_BACK:
        screen_go(work->back);
        se_cancel();
        work->state = OPTIONS_DONE;
        break;
    case OPTIONS_DONE:
        break;
    }
}

static void options_draw_sound_test(struct options *work)
{
    const struct option_item *item = &option_items[OPTION_ITEM_SOUND_TEST];

    screen_quad_color(0xffffffff);
    njSetTexture(&ava_option_texlist);
    screen_quad_draw(OPTION_SOUND_TEST_BACKING, item->x, item->y,
                     work->z - 8.0F);
    screen_quad_draw(QUAD_CENTERED | OPTION_SOUND_TEST_NOTE, item->x + 47.0F,
                     item->y + 40.0F, work->z - 12.0F);
    screen_quad_draw(cd_textures[work->cd], item->x + 8.0F, item->y + 51.0F,
                     work->z - 10.0F);
}

static void options_draw_messages(struct options *work)
{
    const struct option_item *item = &option_items[OPTION_ITEM_MESSAGES];
    int i;

    screen_quad_color(0xffffffff);
    njSetTexture(&ava_option_texlist);
    screen_quad_draw(OPTION_MESSAGES_BACKING, item->x, item->y, work->z - 6.0F);
    screen_quad_draw(sonic_textures[work->sonic], item->x + 39.0F,
                     item->y + 42.0F, work->z - 8.0F);
    if (work->letters < LETTERS) {
        for (i = 0; i <= work->letters; i++) {
            screen_quad_draw(letter_textures[i], item->x + letter_xs[i],
                             item->y + 17.0F, work->z - 8.0F);
        }
    }
}

static void options_draw_speakers(struct options *work)
{
    const struct option_item *item = &option_items[OPTION_ITEM_SPEAKERS];
    Float scale;

    screen_quad_color(0xffffffff);
    njSetTexture(&ava_option_texlist);
    screen_quad_draw(OPTION_SPEAKERS_BACKING, item->x, item->y, work->z - 6.0F);
    scale = 1.0F + ((njSin(work->spin) + 1.0F) * 0.125F);
    /* @hack redundant: the first speaker's scale times 1.0F, on each axis. SHC
     * then keeps 1 in FR14 (FLDI1 FR14 at 0x8c906d82) for the add above and the
     * products. passed scale itself, SHC loads 1 into FR3 for the add alone and
     * saves no FR14, and 111 of 117 words differ */
    screen_quad_draw_scaled(QUAD_CENTERED | OPTION_SPEAKER, item->x + 44.0F,
                            item->y + 33.0F, work->z - 8.0F, scale * 1.0F,
                            scale * 1.0F);
    screen_quad_draw_scaled(QUAD_CENTERED | OPTION_SPEAKER, item->x + 51.0F,
                            item->y + 69.0F, work->z - 10.0F, scale * 0.6F,
                            scale * 0.66F);
    screen_quad_draw_scaled(QUAD_CENTERED | OPTION_SPEAKER, item->x + 34.0F,
                            item->y + 59.0F, work->z - 12.0F, scale * 0.4F,
                            scale * 0.4F);
}

static void options_draw_file(struct options *work)
{
    const struct option_item *item = &option_items[OPTION_ITEM_FILE];

    screen_quad_color(0xffffffff);
    njSetTexture(&ava_option_texlist);
    screen_quad_draw(OPTION_FILE_BACKING, item->x, item->y, work->z - 6.0F);
    screen_quad_draw(QUAD_CENTERED | OPTION_FILE_CARD, item->x + 27.0F,
                     item->y + 32.0F, work->z - 8.0F);
    screen_quad_draw(QUAD_CENTERED | OPTION_FILE_CARD, item->x + 70.0F,
                     item->y + 64.0F, work->z - 8.0F);
    screen_quad_colors(0x78ff9100, 0x78ff0000, 0x78ff9100, 0x78ff0000);
    screen_quad_draw(QUAD_CENTERED | OPTION_FILE_ARROW, item->x + 65.0F,
                     item->y + 20.0F, work->z - 8.0F);
    screen_quad_draw(QUAD_FLIP_U | QUAD_FLIP_V | QUAD_CENTERED |
                         OPTION_FILE_ARROW,
                     item->x + 30.0F, item->y + 74.0F, work->z - 8.0F);
}

static void options_draw_back(struct options *work)
{
}

static void options_draw_items(struct options *work)
{
    int i;
    Uint32 color;

    for (i = 0; i < OPTION_ITEMS; i++) {
        const struct option_item *item = &option_items[i];

        screen_quad_color(0xffffffff);
        njSetTexture(&ava_option_texlist);
        screen_quad_draw(item->label | QUAD_CENTERED, item->label_x,
                         item->label_y, work->z - 10.0F);
        if (work->cursor == i) {
            /* @hack type: alpha passed as a Sint8. SHC then passes the byte
             * as MOV.B loads it, in the JSR's slot (0x8c907014). as the
             * Uint8, SHC adds an EXTU.B, the function runs 2 bytes longer,
             * and 241 of 373 words differ */
            /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
            menu_draw_column_cursor((Sint8)work->alpha, item->label_x,
                                    item->label_y, work->z - 12.0F,
                                    work->pulse * item->scale_x,
                                    work->pulse * item->scale_y);
            color = work->cursor_color;
        } else {
            color = 0xff12b4ff;
        }
        menu_draw_column_item(color, item->label_x, item->label_y,
                              work->z - 8.0F, item->scale_x, item->scale_y);
    }
}

static void options_draw_frame(struct options *work)
{
    int i;

    screen_quad_color(0xffffffff);
    njSetTexture(&ava_option_texlist);
    screen_quad_draw(OPTION_TITLE, 40.0F, 42.0F, work->z - 14.0F);
    screen_band_draw(0.0F, 40.0F, work->z - 8.0F, 565.0F, 42.0F);
    screen_quad_colors(0x78002e67, 0x78117bff, 0x78002e67, 0x78117bff);
    screen_box_draw(104.0F, 88.0F, work->z - 6.0F, 444.0F, 304.0F);
    for (i = 0; i < OPTION_ITEMS; i++) {
        option_items[i].draw(work);
    }
}

static void options_init(struct task *task, struct screen_links *links)
{
    struct options *work = task->extra;

    work->back = links->back;
    work->next = links->next;
    work->z = 55000.0F;
    work->mode = SCREEN_HIDDEN;
    work->cursor = OPTION_ITEM_SOUND_TEST;
    options_reset(work);
}

static void options_step(struct task *task)
{
    struct options *work = task->extra;

    if (ADVERTISE_WORK()->screen == SCREEN_OPTIONS &&
        work->mode == SCREEN_HIDDEN) {
        options_open(work);
        work->fade = 0.0F;
        work->mode = SCREEN_OPENING;
    }
    if (ADVERTISE_WORK()->last_screen == SCREEN_OPTIONS &&
        work->mode == SCREEN_SHOWN) {
        work->fade = 0.0F;
        work->mode = SCREEN_CLOSING;
    }
    options_animate(work);
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
        ADVERTISE_WORK()->shown = SCREEN_OPTIONS;
        options_run(work);
        break;
    case SCREEN_CLOSING:
        work->tint = color_fade_out(work->fade);
        work->fade += 0.1F;
        if (work->fade >= 1.0F) {
            textures_switch(SCREEN_OPTIONS);
            ADVERTISE_WORK()->closed = 1;
            work->mode = SCREEN_HIDDEN;
        }
        break;
    }
}

static void options_draw(struct task *task)
{
    struct options *work = task->extra;

    if (work->tint) {
        screen_blend_alpha();
        screen_quad_tint(work->tint);
        options_draw_frame(work);
        options_draw_items(work);
        back_draw(work->z - 4.0F);
    }
}

struct task *options_create(struct screen_links *links)
{
    struct task *task;
    struct options *work;

    /* @hack spelling: the task assigned in the test. SHC then tests it in R0
     * (TST R0,R0 at 0x8c907588), and copies it to R14 after. assigned first,
     * it is copied to R14 and tested there, and 2 of 32 words differ */
    if ((options_task = task = task_create(0, 3, options_step))) {
        work = heap_calloc(1, sizeof(struct options));
        task->extra = work;
        options_init(task, links);
        task->step = options_step;
        task->draw = options_draw;
    }
    return task;
}

void options_destroy(void)
{
    if (options_task) {
        task_destroy(options_task);
    }
}
