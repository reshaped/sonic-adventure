/* how to play. the controller task drives the page task and the back
 * task. A turns the pages to the next page that shows, and B or X to the
 * one before. when a turn crosses between the controls pages and the game
 * pages, the controller turns the back too. it then plays the voice for the
 * back's new page */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "draw/screen.h"
#include "hud/menu.h"
#include "main.h"
#include "object/common.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "summary/tutorial.h"
#include "summary/tutorial_back.h"
#include "summary/tutorial_screen.h"
#include "system/bitmap.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/timer.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 4
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 10
#define TEXT_9_LENGTH 9
#define TEXT_10_LENGTH 5
#include "summary/tutorial_screen/text.inc"

/* the rows of the voice tables, by menu_state.summary: one for no voice
 * file, then one for each of the eight voice files */
#define VOICE_ROWS 9

/* tutorial_screen_step's phases, in tutorial_phase. the screen fades in
 * from black, waits for the controller's answer, fades out to black, and
 * ends */
enum tutorial_phase {
    SCREEN_START,
    SCREEN_FADE_IN,
    SCREEN_WAIT,
    SCREEN_FADE_OUT,
    SCREEN_DONE
};

/* the controller's answer: none yet, the exit menu's start, or back out to
 * the menus */
enum { ANSWER_NONE, ANSWER_START, ANSWER_BACK };

/* the controller task's modes, in work->mode. CONTROLLER_START plays the
 * controls voice. CONTROLLER_READY starts a turn on A, B or X. on the last
 * page that shows, A pops up the exit menu. on the first, B or X answers
 * ANSWER_BACK. CONTROLLER_TURN turns the back with the pages until the page
 * task's turn ends. CONTROLLER_MENU answers by the item the menu closes on.
 * CONTROLLER_UNUSED does nothing */
enum tutorial_controller_mode {
    CONTROLLER_START,
    CONTROLLER_READY,
    CONTROLLER_TURN,
    CONTROLLER_UNUSED,
    CONTROLLER_MENU
};

/* the back's page, a BACK_ page, as the controller keeps it in the back
 * task's work. the back task never reads it: it draws its record's shown
 * page */
#define BACK_PAGE(back_task) ((back_task)->work->vars.w[0])

/* the controller task's record, in task->extra */
struct tutorial_controller {
    struct task *page_tasks[2]; /* the page task, twice. nothing reads them */
    Uint8 story;
};

/* the controller task's work, a struct task_work it lays out its own way */
struct tutorial_controller_work {
    Sint8 mode, submode, id;
    Uint8 next; /* the page a turn forward shows */
    Sint16 answer; /* an ANSWER_, which tutorial_screen_step reads */
    Sint16 timer;
    /* the back's page, as CONTROLLER_READY reads it from BACK_PAGE. each step
     * there first copies back_page[0] into back_page[1], then reads the page
     * into back_page[0]. when the two differ, the controller plays the new
     * page's voice */
    Sint16 back_page[2];
    Uint8 unk_0c[8];
    struct task *page_task;
    struct task *back_task;
};

/* the items of the exit menu, which A pops up on the last page. B and X
 * choose EXIT_CANCEL too */
enum { EXIT_START, EXIT_CANCEL, EXIT_ITEMS };
/* the two labels the menu shows, as tutorial_menu_texlist numbers them */
enum { LABEL_CANCEL = 0, LABEL_START = 2 };

static const struct menu_item tutorial_exit_items[] = {
    {-120.0F, 0.0F, LABEL_START},
    {120.0F, 0.0F, LABEL_CANCEL},
};

static NJS_TEXNAME tutorial_menu_textures[] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0},
};
static NJS_TEXLIST tutorial_menu_texlist = {
#include "summary/tutorial_screen/tutorial_menu_texlist.inc"
};
static const struct menu tutorial_exit_menu = {
    MENU_ROW,
    NULL,
    &tutorial_menu_texlist,
    tutorial_exit_items,
    &menu_default_sounds,
    0x97008740,
    0x97008740,
    320.0F,
    240.0F,
    100.0F,
    480.0F,
    88.0F,
    1.625F,
    0.8F,
    EXIT_ITEMS,
    EXIT_CANCEL,
};

static int tutorial_result = OVERLAY_RUNNING;
static Uint8 tutorial_tunes[] = {
    MUSIC_ADVSONIC, MUSIC_ADVMILES, MUSIC_ADVKNKLS, MUSIC_ADVAMY,
    MUSIC_ADVBIG,   MUSIC_ADVE102,  MUSIC_CHAOS,
};
/* the voice for the back's controls page, by menu_state.summary and story */
static Uint16 tutorial_controls_voices[VOICE_ROWS][TUTORIALS] = {
    {5, 6, 7, 8, 10, 9},
    {41, 42, 43, 44, 46, 45},
    {59, 60, 61, 62, 64, 63},
    {77, 78, 79, 80, 82, 81},
    {95, 96, 97, 98, 100, 99},
    {113, 114, 115, 116, 118, 117},
    {132, 133, 134, 135, 137, 136},
    {150, 151, 153, 154, 156, 155},
    {23, 24, 25, 26, 28, 27},
};
/* the voice for the back's game page, by menu_state.summary */
static Uint16 tutorial_game_voices[VOICE_ROWS] = {
    11, 47, 65, 83, 101, 119, 138, 157, 29,
};

static Float tutorial_fade;
static Float tutorial_fade_step;
static int tutorial_phase;
static NJS_TEXLIST *tutorial_back_texlist;
static struct task *tutorial_controller_task;
static struct task *tutorial_menu_task;

static struct task *tutorial_controller_create(int level, int story);
static void tutorial_music_step(struct task *task);
static void tutorial_play_controls_voice(int story);
static void tutorial_play_game_voice(void);

int tutorial_screen_start(void *unused)
{
    /* @hack redundant: a position never used, whose 12 bytes give the image's
     * 12-byte frame (ADD #-12,R15 at 0x8c900b06); without it, the function
     * takes no frame and 117 of 117 words differ */
    NJS_POINT3 unused_pos;
    int story;

    tutorial_phase = SCREEN_START;
    tutorial_back_texlist = NULL;
    tutorial_menu_task = NULL;
    stream_destroy();
    texture_release_characters();
    njReleaseTextureAll();
    heap_init();
    task_init();
    frame_pacing_init(1, 1);
    bitmap_clear_queue();
    sound_load_file("emeraldcoast.mlt");
    njSetBorderColor(0xff000000);
    njSetBackColor(0xff000000, 0xff000000, 0xff000000);
    njFogDisable();
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
    screen_quad_init();
    screen_cover_set_color(0);
    tutorial_fade = 1.0F;
    tutorial_fade_step = 0.1F;
    story = menu_get_story(get_character());
    texture_load_pvm("adv_window", &screen_box_texlist);
    texture_load_pvm("ava_csr", &menu_row_texlist);
    texture_load_pvm("ava_dlg", &tutorial_menu_texlist);
    texture_load_pvm("tuto_cmn", &tutorial_title_texlist);
    tutorial_back_texlist = tutorial_back_load(story);
    tutorial_load_textures(story);
    stream_reset();
    task_create(0, 4, tutorial_music_step);
    tutorial_controller_task = tutorial_controller_create(1, story);
    tutorial_menu_task = menu_create();
    return 1;
}

int tutorial_screen_end(void *unused)
{
    int story;

    draw_off = TRUE;
    story = menu_get_story(get_character());
    bitmap_clear_queue();
    stream_stop_music();
    stream_stop_voice();
    task_step_all();
    stream_destroy();
    task_destroy_all();
    task_step_all();
    tutorial_release_textures(story);
    njReleaseTexture(&screen_box_texlist);
    njReleaseTexture(&menu_row_texlist);
    njReleaseTexture(&tutorial_menu_texlist);
    njReleaseTexture(&tutorial_title_texlist);
    njReleaseTextureAll();
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
    draw_off = FALSE;
    return 1;
}

int tutorial_screen_step(void *unused)
{
    pad_scan();
    pad_enable();
    task_step_all();
    task_draw_all();
    switch (tutorial_phase) {
    case SCREEN_START:
        screen_cover_show(TRUE);
        tutorial_whiten_back_color(menu_get_story(get_character()));
        tutorial_phase++;
        break;
    case SCREEN_FADE_IN:
        screen_cover_show(TRUE);
        tutorial_fade -= tutorial_fade_step;
        screen_cover_draw(tutorial_fade);
        if (tutorial_fade <= 0.0F) {
            tutorial_fade = 0.0F;
            tutorial_phase = SCREEN_WAIT;
        }
        break;
    case SCREEN_WAIT:
        screen_cover_show(FALSE);
        if (((struct tutorial_controller_work *)tutorial_controller_task->work)
                ->answer == ANSWER_BACK) {
            tutorial_phase = SCREEN_FADE_OUT;
            tutorial_result = OVERLAY_BACK;
        } else if (((struct tutorial_controller_work *)
                        tutorial_controller_task->work)
                       ->answer == ANSWER_START) {
            tutorial_phase = SCREEN_FADE_OUT;
            tutorial_result = OVERLAY_DONE;
        }
        break;
    case SCREEN_FADE_OUT:
        screen_cover_show(TRUE);
        tutorial_fade += tutorial_fade_step;
        if (tutorial_fade >= 1.0F) {
            njSetBackColor(0xff000000, 0xff000000, 0xff000000);
            tutorial_fade = 1.0F;
            tutorial_phase = SCREEN_DONE;
        }
        break;
    case SCREEN_DONE:
        screen_cover_show(TRUE);
        screen_cover_draw(tutorial_fade);
        tutorial_screen_end(NULL);
        return tutorial_result;
    }
    screen_cover_draw(tutorial_fade);
    if (pad_quit_pressed(0)) {
        tutorial_screen_end(NULL);
        return OVERLAY_RESET;
    }
    return OVERLAY_RUNNING;
}

static void tutorial_controller_step(struct task *task)
{
    struct tutorial_controller_work *work =
        (struct tutorial_controller_work *)task->work;
    struct task *page_task = work->page_task;
    struct task *back_task = work->back_task;
    struct tutorial_controller *controller = task->extra;
    /* @hack spelling: the back's record read through work->back_task, not the
     * local back_task. SHC then sets R10 to 1 before it sets R9 to 0
     * (0x8c900ed4 and 0x8c900ed8); without it, R9 is set first, and 2 of 218
     * words differ */
    struct tutorial_back *back = work->back_task->extra;
    int current;
    int next;

    /* @hack redundant: a test of work && work that never runs. its reads, with
     * those of the test of next below, keep the frame at 4 bytes and put 1 in
     * R10 and 0 in R9 (0x8c900ed0 to 0x8c900ed8); without it, the frame takes
     * 8 bytes, the function grows by 10 bytes, and 131 of 218 words differ */
    if (0) {
        /* NOLINTNEXTLINE(misc-redundant-expression) */
        if (work && work) {
        }
    }
    switch (work->mode) {
    case CONTROLLER_START:
        tutorial_play_controls_voice(controller->story);
        work->mode = CONTROLLER_READY;
        break;
    case CONTROLLER_READY:
        work->back_page[1] = work->back_page[0];
        current = tutorial_get_page(page_task);
        if (pad_press_repeat[0] & PDD_DGT_TA) {
            next = tutorial_find_page(page_task, 1);
            if (next > current) {
                work->next = next;
                tutorial_set_mode(page_task, TUTORIAL_TURN);
                tutorial_set_next(page_task, next);
                work->mode = CONTROLLER_TURN;
            }
            sound_play(SE_MENU_OPEN, 0, 0, 0);
        } else if (pad_press_repeat[0] & (PDD_DGT_TX | PDD_DGT_TB)) {
            next = tutorial_find_page(page_task, -1);

            /* @hack redundant: a test of next && next that never runs. its
             * reads, with those of the test of work above, keep the frame at 4
             * bytes and put 1 in R10 and 0 in R9 (0x8c900ed0 to 0x8c900ed8).
             * without both tests, the frame keeps its 4 bytes, but R10 takes 0
             * and R9 takes 1, and 20 of 218 words differ. without it, the
             * frame takes 8 bytes, the function grows by 10 bytes, and 131 of
             * 218 words differ */
            if (0) {
                /* NOLINTNEXTLINE(misc-redundant-expression) */
                if (next && next) {
                }
            }
            if (next < current) {
                tutorial_set_mode(page_task, TUTORIAL_TURN);
                tutorial_set_next(page_task, next);
                work->mode = CONTROLLER_TURN;
            }
            sound_play(SE_MENU_CLOSE, 0, 0, 0);
        }
        work->back_page[0] = BACK_PAGE(back_task);
        if (work->back_page[0] != work->back_page[1]) {
            if (work->back_page[0] == BACK_CONTROLS) {
                tutorial_play_controls_voice(controller->story);
            } else {
                tutorial_play_game_voice();
            }
        }
        switch (tutorial_get_edge(page_task)) {
        case TUTORIAL_LAST:
            work->mode = CONTROLLER_MENU;
            menu_popup_default(&tutorial_exit_menu);
            break;
        case TUTORIAL_FIRST:
            work->answer = ANSWER_BACK;
            break;
        default:
            work->answer = ANSWER_NONE;
            break;
        }
        break;
    case CONTROLLER_TURN:
        current = tutorial_get_page(page_task);
        if (current < tutorial_controls(page_task)) {
            BACK_PAGE(back_task) = BACK_CONTROLS;
            back->shown = BACK_CONTROLS;
            back->before = BACK_GAME;
        } else {
            BACK_PAGE(back_task) = BACK_GAME;
            back->shown = BACK_GAME;
            back->before = BACK_CONTROLS;
        }
        if (page_task->work->flags & TUTORIAL_CROSSED) {
            tutorial_back_turn(back_task);
        }
        if (tutorial_get_mode(page_task) == TUTORIAL_SHOWN) {
            work->mode = CONTROLLER_READY;
        }
        break;
    case CONTROLLER_UNUSED:
        break;
    case CONTROLLER_MENU:
        /* @hack redundant: a jump to the label right below it. SHC writes no
         * code for it, but loads menu_result's address into R2 (0x8c9010b6).
         * without it, R3 takes the address and 2 of 218 words differ */
        goto read_answer;
    read_answer:
        /* @hack type: a cast of menu_result's item, which is -1, 0 or 1 here,
         * to a Sint8. it changes no value, but SHC sign-extends the item for
         * it (EXTS.B R0,R4 at 0x8c9010bc). without it, MOV R0,R4 takes its
         * place and 1 of 218 words differ */
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        next = (Sint8)menu_result();
        if (next >= 0) {
            switch (next) {
            /* @hack redundant: a default label on case EXIT_START, which no
             * item reaches. SHC then sends a value that matches no case to case
             * EXIT_START's code: the BRA at 0x8c9010cc, whose slot loads
             * ANSWER_START for the store at 0x8c9010d2. without it, that
             * value branches past the switch, the function grows by 4 bytes,
             * and 31 of 218 words differ */
            default:
            case EXIT_START:
                work->answer = ANSWER_START;
                break;
            case EXIT_CANCEL:
                work->answer = ANSWER_BACK;
                break;
            }
        }
        break;
    }
}

static void tutorial_controller_destroy(struct task *task)
{
    struct tutorial_controller_work *work =
        (struct tutorial_controller_work *)task->work;
    struct task *page_task = work->page_task;
    struct task *back_task = work->back_task;

    if (page_task) {
        task_kill(page_task);
    }
    if (back_task) {
        task_kill(back_task);
    }
}

static struct task *tutorial_controller_create(int level, int story)
{
    struct task *task;
    struct tutorial_controller *controller;
    struct task *page_task;
    struct task *back_task;

    task = task_create(TASK_WORK, level, tutorial_controller_step);
    if (task) {
        task->extra = heap_calloc(1, sizeof(struct tutorial_controller));
        controller = task->extra;
        page_task = tutorial_create(level + 1, story, NULL);
        ((struct tutorial_controller_work *)task->work)->page_task = page_task;
        back_task = tutorial_back_create(level + 1, story, NULL);
        ((struct tutorial_controller_work *)task->work)->back_task = back_task;
        controller->page_tasks[0] = page_task;
        controller->page_tasks[1] = page_task;
        controller->story = story;
        task->destructor = tutorial_controller_destroy;
    }
    return task;
}

static void tutorial_music_step(struct task *task)
{
    int story = menu_get_story(get_character());

    /* @bug logic: menu_get_story returns menu_state.story, a Uint8, so 0
     * to 255 (EXTU.B at 0x8c05b60c). the CMP/EQ #-1 at 0x8c9011d0 never
     * holds, and the test always passes */
    if (story != -1) {
        stream_request_music(tutorial_tunes[story]);
    }
    task_kill(task);
}

static void tutorial_play_controls_voice(int story)
{
    Uint8 summary = menu_state.summary[0];

    if (summary < VOICE_ROWS && story < TUTORIALS) {
        stream_request_voice(tutorial_controls_voices[summary][story]);
    }
}

static void tutorial_play_game_voice(void)
{
    Uint8 summary = menu_state.summary[0];

    if (summary < VOICE_ROWS) {
        stream_request_voice(tutorial_game_voices[summary]);
    }
}
