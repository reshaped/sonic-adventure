/* the file screen, which picks the save to play from the card's three and
 * shows the records of the one under the cursor */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "advertise/back.h"
#include "advertise/buttons.h"
#include "advertise/draw.h"
#include "advertise/emblems.h"
#include "advertise/file_select.h"
#include "advertise/menus.h"
#include "advertise/records.h"
#include "advertise/screen.h"
#include "advertise/sound.h"
#include "advertise/textures.h"
#include "advertise/trial.h"
#include "advertise/wheel.h"
#include "draw/screen.h"
#include "hud/menu.h"
#include "object/common.h"
#include "save/card.h"
#include "save/mail.h"
#include "save/save.h"
#include "system/pad.h"
#include "system/task.h"

/* the list's rows: a row for each save, then the row that backs out */
#define FILE_ROWS (SAVE_COUNT + 1)
#define BACK_ROW SAVE_COUNT
#define DETAIL_ROWS 4 /* the panel's rows under the story's name */

/* ava_suuji's colon, and ava_emblem's times sign */
#define SUUJI_COLON 0
#define EMBLEM_TIMES 1

/* ava_filesel's textures */
enum {
    FILESEL_BLACK_BAR,
    FILESEL_FILE_BLUE,
    FILESEL_FILE_YELLOW,
    FILESEL_CLEAR,
    FILESEL_EMBLEMS,
    FILESEL_PLAY,
    FILESEL_START,
    FILESEL_ICON_BACKING,
    FILESEL_NAME_BACKING,
    FILESEL_TITLE,
    FILESEL_INNER_CORNER,
    FILESEL_INNER_EDGE,
    FILESEL_INNER_SIDE,
    FILESEL_OUTER_CORNER,
    FILESEL_OUTER_EDGE,
    FILESEL_OUTER_SIDE,
    FILESEL_UPPER_CORNER,
    FILESEL_UPPER_EDGE,
    FILESEL_UPPER_SIDE,
    FILESEL_AMY,
    FILESEL_BIG,
    FILESEL_E102,
    FILESEL_ICON_SHADOW,
    FILESEL_KNUCKLES,
    FILESEL_SONIC,
    FILESEL_SUPER_SONIC,
    FILESEL_TAILS,
    FILESEL_AMY_CLOSED,
    FILESEL_BIG_CLOSED,
    FILESEL_E102_CLOSED,
    FILESEL_KNUCKLES_CLOSED,
    FILESEL_TAILS_CLOSED
};

/* ava_fsdlg's textures */
enum {
    FSDLG_FILE_1,
    FSDLG_FILE_2,
    FSDLG_FILE_3,
    FSDLG_ERASE,
    FSDLG_ERASING,
    FSDLG_DONT_REMOVE
};

/* FILE_MENU's items. FILE_ERASE_MENU has the first two */
enum file_item { ITEM_DECIDE, ITEM_CANCEL, ITEM_ERASE };

enum file_state {
    FILE_CHOOSING,
    FILE_ASKING,
    FILE_ERASING,
    FILE_MAIL_WAIT,
    FILE_MAIL_WRITE,
    FILE_MAIL_FINISH,
    FILE_CANCEL,
    FILE_DONE
};

struct file_select {
    enum screen_mode mode;
    int back, next;
    Float z, fade;
    Uint32 tint;
    Float pulse;
    Uint32 highlight;
    Angle blink;
    enum file_state state;
    int saved_slot;
    Angle spin;
    Sint16 timer;
    Uint8 alpha;
};

/* a layer of the frame round the panel. z is its depth from the screen's, and
 * corner, edge and side are its pieces in ava_filesel */
struct file_frame_layer {
    Float z;
    Uint32 top_color, bottom_color;
    Uint8 corner, edge, side;
    Uint8 pad;
};
/* the labels of the panel's rows, from the top */
static const Uint8 detail_textures[] = {FILESEL_START, FILESEL_CLEAR,
                                        FILESEL_PLAY, FILESEL_EMBLEMS};
/* each story's icon, by enum wheel_story, for a story the save can play and
 * for one it can't. ava_filesel has no closed icon for Sonic or the last
 * story, which take Tails' and E-102's */
static const Uint8 open_character_textures[] = {
    FILESEL_SONIC, FILESEL_TAILS, FILESEL_KNUCKLES,   FILESEL_AMY,
    FILESEL_BIG,   FILESEL_E102,  FILESEL_SUPER_SONIC};
static const Uint8 closed_character_textures[] = {
    FILESEL_TAILS_CLOSED, FILESEL_TAILS_CLOSED, FILESEL_KNUCKLES_CLOSED,
    FILESEL_AMY_CLOSED,   FILESEL_BIG_CLOSED,   FILESEL_E102_CLOSED,
    FILESEL_E102_CLOSED};
static const Uint8 erase_file_textures[] = {FSDLG_FILE_1, FSDLG_FILE_2,
                                            FSDLG_FILE_3};
/* the frame's layers, from front to back */
static const struct file_frame_layer frame_layers[] = {
    {-12.0F, 0xffff9f00, 0xffff9f00, FILESEL_OUTER_CORNER, FILESEL_OUTER_EDGE,
     FILESEL_OUTER_SIDE, 0},
    {-10.0F, 0xffffef00, 0xffff9f00, FILESEL_UPPER_CORNER, FILESEL_UPPER_EDGE,
     FILESEL_UPPER_SIDE, 0},
    {-8.0F, 0xffff9f00, 0xffffeb00, FILESEL_INNER_CORNER, FILESEL_INNER_EDGE,
     FILESEL_INNER_SIDE, 0}};
static struct task *file_select_task;

void file_erase_menu_draw(struct menu_view *view)
{
    Uint32 width;
    Uint32 height;
    Float x;
    Float y;

    njSetTexture(&ava_fsdlg_texlist);
    screen_quad_color(0xffffffff);
    screen_texture_size(&ava_fsdlg_texlist, FSDLG_ERASE, &width, &height);
    /* @hack type: the sizes cast to int, which SHC converts as signed (FLOAT
     * at 0x8c904134 and 0x8c904144). from the Uint32s, SHC adds a test of
     * each sign and a correction, the functions after it move, and 95 of 110
     * words differ */
    x = -113.0F + ((int)width * 0.5F);
    y = -43.0F + ((int)height * 0.5F);
    screen_quad_draw_scaled(QUAD_CENTERED | FSDLG_ERASE,
                            view->x + (x * view->scale_x),
                            view->y + (y * view->scale_y), view->z - 6.0F,
                            view->scale_x, view->scale_y);
    screen_quad_draw_scaled(
        erase_file_textures[menu_state.save] | QUAD_CENTERED,
        (-39.0F * view->scale_x) + view->x, (-35.0F * view->scale_y) + view->y,
        view->z - 6.0F, view->scale_x, view->scale_y);
}

static void file_select_reset(struct file_select *work)
{
    work->state = FILE_CHOOSING;
    menu_state.save = (Uint8)save_slot;
    work->spin = 0;
    work->blink = 0;
    work->highlight = 0xffff0000;
}

static void file_select_open(struct file_select *work)
{
    textures_load_screen(SCREEN_FILE);
    advertise_music_play(TUNE_OPTION);
    advertise_voice_summary(LINE_FILE);
    file_select_reset(work);
    work->saved_slot = save_slot;
    advertise_save_load();
}

static void file_select_run(struct file_select *work)
{
    Uint8 selection;
    int result;
    menu_cursor_step(&work->alpha, &work->blink, &work->pulse);
    work->highlight = 0xffff0000;
    switch (work->state) {
    case FILE_CHOOSING:
        selection = menu_state.save;
        work->highlight = menu_blend_color(0xff0000ff, 0xffff0000,
                                           (njCos(work->blink) + 1.0F) * 0.5F);
        if ((pad_press_repeat[0] & PDD_DGT_KU) && selection > 0) {
            work->pulse = 1.5F;
            selection--;
            se_move();
        } else if ((pad_press_repeat[0] & PDD_DGT_KD) && selection < BACK_ROW) {
            work->pulse = 1.5F;
            selection++;
            se_move();
        } else if (pad_choose_pressed() && textures_ready == TRUE) {
            if (selection == BACK_ROW) {
                work->state = FILE_CANCEL;
            } else if ((pad_held[0] & PDD_DGT_TY) && save_isp_found()) {
                advertise_sound_stop();
                work->timer = 8;
                work->state = FILE_MAIL_WAIT;
            } else {
                advertise_menu_popup_default(FILE_MENU);
                work->state = FILE_ASKING;
            }
        } else if (pad_back_pressed() && textures_ready == TRUE) {
            work->state = FILE_CANCEL;
        }
        if (menu_state.save != selection && selection < SAVE_COUNT) {
            save_slot = selection;
            advertise_save_load();
        }
        menu_state.save = selection;
        save_slot = selection < SAVE_COUNT ? selection : SAVE_COUNT - 1;
        break;
    case FILE_ASKING:
        /* @hack type: the result cast to Sint8 in the test and the switch,
         * which SHC sign-extends from its low byte (EXTS.B at 0x8c90444e and
         * 0x8c904458); without the casts, 83 of 286 words differ */
        if ((Sint8)(result = menu_result()) >= 0) {
            switch ((Sint8)result) {
            case ITEM_DECIDE:
                screen_go(work->next);
                work->state = FILE_DONE;
                break;
            case ITEM_CANCEL:
                file_select_reset(work);
                break;
            case ITEM_ERASE:
                advertise_menu_popup_default(FILE_ERASE_MENU);
                work->state = FILE_ERASING;
                break;
            }
        }
        break;
    case FILE_ERASING:
        /* @hack type: the result cast to Sint8 in the test and the switch,
         * which SHC sign-extends from its low byte (EXTS.B at 0x8c904498 and
         * 0x8c90449e); without the casts, 45 of 286 words differ */
        if ((Sint8)(result = menu_result()) >= 0) {
            switch ((Sint8)result) {
            case ITEM_DECIDE:
                save_reset_write();
                advertise_save_load();
                /* fall through */
            case ITEM_CANCEL:
                file_select_reset(work);
                break;
            }
        }
        break;
    case FILE_MAIL_WAIT:
        if (--work->timer <= 0) {
            work->state = FILE_MAIL_WRITE;
        }
        break;
    case FILE_MAIL_WRITE:
        save_write_player_mail(0);
        work->timer = 8;
        work->state = FILE_MAIL_FINISH;
        return;
    case FILE_MAIL_FINISH:
        if (work->timer == 4) {
            advertise_music_play(TUNE_OPTION);
        }
        if (--work->timer < 0) {
            file_select_reset(work);
        }
        break;
    case FILE_CANCEL:
        save_slot = work->saved_slot;
        screen_go(work->back);
        se_cancel();
        work->state = FILE_DONE;
        break;
    case FILE_DONE:
        break;
    }
}

static void file_select_draw_story_stage(struct file_select *work)
{
    Uint8 sprite;
    Float y = 228.0F;
    njSetTexture(&ava_stnam_texlist);
    screen_quad_color(0xffffffff);
    if ((Uint8)record_stage() != NO_STAGE) {
        sprite =
            file_stage_name_numbers[(Uint8)record_story_stage(record_story())];
        screen_quad_draw(sprite | QUAD_CENTERED, 408.0F, y, work->z - 12.0F);
    }
}

static void file_select_draw_stage(struct file_select *work)
{
    Uint8 sprite;
    Float y = 285.0F;
    njSetTexture(&ava_stnam_texlist);
    screen_quad_color(0xffffffff);
    if ((Uint8)record_stage() != NO_STAGE) {
        sprite = file_stage_name_numbers[(Uint8)record_stage()];
        screen_quad_draw(sprite | QUAD_CENTERED, 408.0F, y, work->z - 12.0F);
    }
}

static void file_select_draw_time(struct file_select *work)
{
    Float y;
    Float z;
    njSetTexture(&ava_suuji_texlist);
    screen_quad_color(0xffffffff);
    y = 342.0F;
    z = work->z - 12.0F;
    screen_quad_draw(QUAD_CENTERED | SUUJI_COLON, 408.0F, y, z);
    suuji_draw(record_play_minutes() / 60, 4, 0, 344.0F, y, z, 1.0F);
    suuji_draw(record_play_minutes() % 60, 2, 1, 424.0F, y, z, 1.0F);
}

static void file_select_draw_emblems(struct file_select *work)
{
    ava_emblem_draw(376.0F, 401.0F, work->z - 12.0F);
    njSetTexture(&ava_emblem_texlist);
    screen_quad_color(0xffffffff);
    screen_quad_draw(QUAD_CENTERED | EMBLEM_TIMES, 417.0F, 401.0F,
                     work->z - 12.0F);
    suuji_draw_all(emblems_won(), 450.0F, 401.0F, work->z - 12.0F);
}

static void file_select_draw_frame(struct file_select *work)
{
    int i;
    const struct file_frame_layer *layer;
    Uint32 upper;
    Uint32 lower;

    njSetTexture(&ava_filesel_texlist);
    for (i = 0; i < 3; i++) {
        layer = &frame_layers[i];
        upper = menu_blend_color(layer->top_color, layer->bottom_color,
                                 32.0F / 360.0F);
        screen_quad_colors(layer->top_color, upper, layer->top_color, upper);
        screen_quad_draw(layer->corner, 238.0F, 78.0F, work->z + layer->z);
        screen_quad_draw(layer->corner | QUAD_TOP_RIGHT | QUAD_FLIP_U, 578.0F,
                         78.0F, work->z + layer->z);
        screen_quad_draw_sized(layer->edge, 270.0F, 78.0F, work->z + layer->z,
                               276.0F, 32.0F);
        lower = menu_blend_color(layer->top_color, layer->bottom_color,
                                 1.0F - (32.0F / 360.0F));
        screen_quad_colors(upper, lower, upper, lower);
        screen_quad_draw_sized(layer->side, 238.0F, 110.0F, work->z + layer->z,
                               32.0F, 296.0F);
        screen_quad_draw_sized(layer->side | QUAD_TOP_RIGHT | QUAD_FLIP_U,
                               578.0F, 110.0F, work->z + layer->z, 32.0F,
                               296.0F);
        screen_quad_colors(lower, layer->bottom_color, lower,
                           layer->bottom_color);
        screen_quad_draw(layer->corner | (QUAD_BOTTOM_LEFT | QUAD_FLIP_V),
                         238.0F, 438.0F, work->z + layer->z);
        screen_quad_draw(layer->corner | QUAD_BOTTOM_RIGHT |
                             (QUAD_FLIP_V | QUAD_FLIP_U),
                         578.0F, 438.0F, work->z + layer->z);
        screen_quad_draw_sized(layer->edge | QUAD_FLIP_V, 270.0F, 406.0F,
                               work->z + layer->z, 276.0F, 32.0F);
    }
    screen_quad_colors(0x7812b4ff, 0x7812b4ff, 0x7812b4ff, 0x7812b4ff);
    screen_box_draw(238.0F, 78.0F, work->z - 4.0F, 340.0F, 360.0F);
}

static void file_select_draw_labels(struct file_select *work)
{
    int row;
    Float y;
    Uint8 word;

    njSetTexture(&ava_filesel_texlist);
    screen_quad_color(0xffffffff);
    screen_quad_draw(FILESEL_TITLE, 32.0F, 40.0F, work->z - 14.0F);
    screen_band_draw(0.0F, 32.0F, work->z - 8.0F, 565.0F, 42.0F);
    for (row = 0; row < DETAIL_ROWS; row++) {
        y = 200.0F + (57.0F * row);
        njSetTexture(&ava_filesel_texlist);
        screen_quad_color(0xffffffff);
        screen_quad_draw(QUAD_CENTERED | FILESEL_BLACK_BAR, 408.0F, y,
                         work->z - 6.0F);
        screen_quad_draw(detail_textures[row] | QUAD_CENTERED, 408.0F, y,
                         work->z - 8.0F);
        menu_draw_column_item(0xff12b4ff, 408.0F, y + 28.0F, work->z - 8.0F,
                              0.969F, 0.5F);
    }
    for (row = 0; row < FILE_ROWS; row++) {
        njSetTexture(&ava_fscmn_texlist);
        if (row < SAVE_COUNT) {
            word = FSCMN_FILE;
        } else {
            word = FSCMN_BACK;
        }
        y = 142.0F + (row * 88.0F);
        screen_quad_color(0xffffffff);
        screen_quad_draw(word, 78.0F, y, work->z - 10.0F);
        y = 129.0F + (row * 88.0F);
        fscmn_draw(row, 168.0F, y, work->z - 14.0F);
    }
    njSetTexture(&ava_filesel_texlist);
    screen_quad_color(0xffffffff);
    screen_quad_draw_scaled(QUAD_CENTERED | FILESEL_NAME_BACKING, 408.0F,
                            122.0F, work->z - 6.0F, 1.883F, 1.0F);
}

static void file_select_draw_story(struct file_select *work)
{
    m_chnam_draw(record_story(), 0xffffffff, 408.0F, 122.0F, work->z - 12.0F,
                 0.7F, 0.7F);
}

static void file_select_draw_characters(struct file_select *work)
{
    Float x;
    Float step;
    Float z;
    int count;
    int i;
    njSetTexture(&ava_filesel_texlist);
    screen_quad_color(0xffffffff);
    if (character_playable(STORY_LAST)) {
        x = 282.0F;
        step = 42.0F;
        count = 7;
    } else {
        x = 298.0F;
        step = 44.0F;
        count = 6;
    }
    z = work->z - 12.0F;
    for (i = 0; i < count; i++) {
        if (character_playable(i)) {
            screen_quad_draw_scaled(open_character_textures[i] | QUAD_CENTERED,
                                    x, 164.0F, z - 0.3F, 0.5F, 0.5F);
            screen_quad_draw(QUAD_CENTERED | FILESEL_ICON_SHADOW, x, 164.0F, z);
        } else {
            screen_quad_draw_scaled(closed_character_textures[i] |
                                        QUAD_CENTERED,
                                    x, 164.0F, z, 0.5F, 0.5F);
        }
        x += step;
    }
}

static void file_select_draw_files(struct file_select *work)
{
    int i;
    Uint8 sprite;
    Uint32 color;
    for (i = 0; i < FILE_ROWS; i++) {
        if (i == menu_state.save) {
            color = work->highlight;
            sprite = FILESEL_FILE_YELLOW;
        } else {
            color = 0xff0000ff;
            sprite = FILESEL_FILE_BLUE;
        }
        njSetTexture(&ava_filesel_texlist);
        screen_quad_color(0xffffffff);
        screen_quad_draw_scaled(sprite, 68.0F, 96.0F + (i * 88.0F),
                                work->z - 6.0F, 1.359F, 1.359F);
        item_draw(color, 168.0F, 129.0F + (i * 88.0F), work->z - 12.0F);
    }
    /* @hack type: alpha passed as a Sint8. SHC then passes the byte as MOV.B
     * loads it (0x8c9051ac). as the Uint8, SHC adds an EXTU.B, the function
     * runs 2 bytes longer, the functions after it move, and 17 of 275 words
     * differ */
    menu_draw_row_cursor((Sint8)work->alpha, 168.0F,
                         129.0F + (menu_state.save * 88.0F), work->z - 14.0F,
                         work->pulse, work->pulse);
}

static void file_select_init(struct task *task, struct screen_links *links)
{
    struct file_select *work = task->extra;
    work->back = links->back;
    work->next = links->next;
    work->z = 59000.0F;
    work->mode = SCREEN_HIDDEN;
    file_select_reset(work);
}

static void file_select_step(struct task *task)
{
    struct file_select *work = task->extra;
    if (ADVERTISE_WORK()->screen == SCREEN_FILE &&
        work->mode == SCREEN_HIDDEN) {
        file_select_open(work);
        work->fade = 0.0F;
        work->mode = SCREEN_OPENING;
    }
    if (ADVERTISE_WORK()->last_screen == SCREEN_FILE &&
        work->mode == SCREEN_SHOWN) {
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
        ADVERTISE_WORK()->shown = SCREEN_FILE;
        file_select_run(work);
        break;
    case SCREEN_CLOSING:
        work->tint = color_fade_out(work->fade);
        work->fade += 0.1F;
        if (work->fade >= 1.0F) {
            textures_switch(SCREEN_FILE);
            ADVERTISE_WORK()->closed = 1;
            work->mode = SCREEN_HIDDEN;
        }
        break;
    }
}

static void file_select_draw(struct task *task)
{
    struct file_select *work = task->extra;
    if (work->tint != 0) {
        screen_blend_alpha();
        screen_quad_tint(work->tint);
        file_select_draw_frame(work);
        file_select_draw_labels(work);
        file_select_draw_story(work);
        file_select_draw_characters(work);
        file_select_draw_files(work);
        file_select_draw_story_stage(work);
        file_select_draw_stage(work);
        file_select_draw_time(work);
        file_select_draw_emblems(work);
        back_draw(work->z - 2.0F);
    }
}

struct task *file_select_create(struct screen_links *links)
{
    struct task *task;
    /* @hack spelling: the task assigned in the test. SHC then tests it in R0
     * (TST R0,R0 at 0x8c9053d4), and copies it to R14 after. assigned first,
     * it is copied to R14 and tested there, and 2 of 32 words differ */
    if ((file_select_task = task = task_create(0, 3, file_select_step)) !=
        NULL) {
        struct file_select *work = heap_calloc(1, sizeof(struct file_select));
        task->extra = work;
        file_select_init(task, links);
        task->step = file_select_step;
        task->draw = file_select_draw;
    }
    return task;
}

void file_select_destroy(void)
{
    if (file_select_task != NULL) {
        task_destroy(file_select_task);
    }
}

void file_select_next_options(void)
{
    ((struct file_select *)file_select_task->extra)->next = SCREEN_OPTIONS;
}
