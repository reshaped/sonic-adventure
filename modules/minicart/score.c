/* the race's end: the score screen, the race's mail, then the game over's
 * choice */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "camera/camera.h"
#include "draw/fade.h"
#include "hud/hud.h"
#include "main.h"
#include "minicart/e_cart.h"
#include "minicart/score.h"
#include "save/mail.h"
#include "save/minigame.h"
#include "save/save.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texlists.h"
#include "system/texture.h"

#define QUAD_TEXTURED                                                          \
    (NJD_USE_TEXTURE | NJD_DRAW_CONNECTED | NJD_FILL | NJD_TRANSPARENT)

/* the task starts the score screen once its timer passes WAIT_STEPS */
#define WAIT_STEPS 180

enum score_mode {
    SCORE_WAIT,
    SCORE_START,
    SCORE_SHOW, /* the score screen, until cart_score_done */
    SCORE_LOADED, /* 2 steps once the game over's textures load */
    SCORE_CHOOSE,
    SCORE_CHOSEN
};

/* the game over's choices, from left to right */
enum game_over_choice { CHOICE_CONTINUE, CHOICE_QUIT };

/* the task's work->vars */
struct score_vars {
    Uint8 unk_00[4];
    Sint32 loaded; /* the steps since the game over's textures loaded */
    Uint8 unk_08[4];
};

#define VARS(work) ((struct score_vars *)&(work)->vars)

/* save_write_cart_mail's race and frames arguments, as words. bytes holds
 * the act, the character, the minutes and the seconds in words[0], from its
 * low byte up. it holds the frames in words[1]'s low byte */
union cart_race_mail {
    struct {
        Uint8 act;
        Uint8 character;
        struct save_time time;
    } bytes;
    int words[2];
};

static void cart_score_step(struct task *task);
static void cart_draw_game_over(int choice);

/* TRUE once cart_score_done marks the score screen as ended */
static Bool cart_score_over;
/* the score screen's task, as SCORE_START made it */
static struct task *cart_score_task;
/* the game over's choice, a game_over_choice */
static int cart_choice;
/* the glow's phase. each draw adds 4, wrapping through the multiples of 4 a
 * Sint8 holds. the glow is the phase's absolute value over 128, from 0 to
 * 1 */
static Sint8 cart_game_over_phase;

void cart_score_start(void)
{
    camera_override_high(camera_step_none, CAMERA_ADJUST_NONE, CAMERA_BY_STEP);
    cart_race_over = TRUE;
    task_create(TASK_WORK, 3, cart_score_step);
}

static void cart_score_step(struct task *task)
{
    struct task_work *work = task->work;
    union cart_race_mail mail;
    struct save_cart_times times;
    int act;

    switch (work->mode) {
    case SCORE_WAIT:
        ++TASK_TIMER(work);
        if (TASK_TIMER(work) > WAIT_STEPS) {
            work->mode = SCORE_START;
        }
        break;
    case SCORE_START:
        cart_score_task = task_create(TASK_WORK, 1, score_step);
        work->mode = SCORE_SHOW;
        cart_score_over = FALSE;
        break;
    case SCORE_SHOW:
        if (cart_score_over == TRUE) {
            cart_choice = CHOICE_CONTINUE;
            stream_destroy();
            texture_load_pvm("GAMEOVER", &game_over_texlist);
            njSetBackColor(0xff000000, 0xff000000, 0xff000000);
            work->mode = SCORE_LOADED;
            VARS(work)->loaded = 0;
            act = (Uint8)stage_get_stage_act();
            if (act != 0) {
                cart_copy_times(&times);
                mail.bytes.act = act;
                mail.bytes.character = get_character();
                mail.bytes.time.minutes = times.time[0].minutes;
                mail.bytes.time.seconds = times.time[0].seconds;
                mail.bytes.time.frames = times.time[0].frames;
                /* @bug uninitialized: the stores end with frames at sp+24
                 * (MOV.B at 0x0c90a932), and words[1] reads sp+24 to sp+27
                 * (MOV.L at 0x0c90a938). words[1]'s upper three bytes are
                 * what the stack held, and save_write_cart_mail reads only
                 * its low byte */
                save_write_cart_mail(0, mail.words[0], mail.words[1]);
            }
        }
        break;
    case SCORE_LOADED:
        ++VARS(work)->loaded;
        if (VARS(work)->loaded > 1) {
            work->mode = SCORE_CHOOSE;
            stream_reset();
            stream_request_interlude(MUSIC_CONTINUE);
        }
        break;
    case SCORE_CHOOSE:
        if ((pad_press_live[0] & PDD_DGT_KL) &&
            cart_choice != CHOICE_CONTINUE) {
            cart_choice = CHOICE_CONTINUE;
            sound_play(SE_MOVE, 0, 0, 0);
        } else if ((pad_press_live[0] & PDD_DGT_KR) &&
                   cart_choice != CHOICE_QUIT) {
            cart_choice = CHOICE_QUIT;
            sound_play(SE_MOVE, 0, 0, 0);
        }
        if (pad_press_live[0] & PDD_DGT_TA) {
            if (cart_choice == CHOICE_CONTINUE) {
                stage_restart();
            } else {
                stage_end(STAGE_END_CLEAR);
            }
            sound_play(SE_CHOOSE, 0, 0, 0);
            game_over_end();
            work->mode = SCORE_CHOSEN;
        } else {
            cart_draw_game_over(cart_choice);
        }
        break;
    case SCORE_CHOSEN:
        break;
    }
    pause_disable();
}

void cart_score_done(void)
{
    cart_score_over = TRUE;
}

/* draw both choices' words over their backs, the glow behind the chosen
 * one's words, and each choice's frame. the chosen frame pulses from red to
 * blue as the glow grows, and the other is blue. the function also sets up a
 * black quad over the screen and the banner's quad, and draws neither. the
 * banner's colors and uvs serve the choices' words */
static void cart_draw_game_over(int choice)
{
    NJS_POINT2COL quad;
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];
    NJS_COLOR shade;
    NJS_COLOR uv[4];
    Float glow;
    Float edge;
    Float spacing;
    Float left;
    Float right;
    int offset;
    /* @hack type: blue and chosen volatile, which keeps them at sp+0
     * and sp+4 and reads them there at each use (MOV.L R4,@(4,R15) at
     * 0x0c90aaba); without it, 438 of 539 words and 30 pool words differ */
    volatile Float blue;
    volatile int chosen = choice;
    NJS_COLOR *color;

    glow = cart_game_over_phase;
    cart_game_over_phase = (cart_game_over_phase + 4) & 0xfc;
    if (glow < 0.0F) {
        glow = -glow;
    }
    glow *= 1.0F / 128.0F;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    quad.p = points;
    /* @hack redundant: never-run tests of points and colors, which give points'
     * address R10 and uv's R8, which otherwise trade; without it, 12 of 539
     * words differ */
    if (0) {
        if (points[0].x) {
        }
        if (points[0].x) {
        }
        if (colors[0].color) {
        }
        if (colors[0].color) {
        }
    }
    quad.col = colors;
    quad.tex = NULL;
    quad.num = 4;
    color = &shade;
    color->argb.r = 0;
    color->argb.g = 0;
    color->argb.b = 0;
    color->argb.a = 255;
    quad.p[0].x = 0.0F;
    quad.p[0].y = 0.0F;
    quad.col[0].color = color->color;
    quad.p[1].x = SCREEN_WIDTH;
    quad.p[1].y = 0.0F;
    quad.col[1].color = color->color;
    quad.p[2].x = SCREEN_WIDTH;
    quad.p[2].y = SCREEN_HEIGHT;
    quad.col[2].color = color->color;
    quad.p[3].x = 0.0F;
    quad.p[3].y = SCREEN_HEIGHT;
    quad.col[3].color = color->color;

    color->argb.b = color->argb.g = color->argb.r = 255;
    quad.tex = uv;
    njSetTexture(&game_over_texlist);
    njSetTextureNum(GAME_OVER_BANNER);
    /* @hack spelling: spacing and edge set in the banner's corner, which loads
     * them into FR13 and FR14 here for the choices (0x0c90abac, 0x0c90abb2);
     * set where they are declared, the loads move to the function's entry,
     * and 140 of 539 words and 22 pool words differ */
    points[0].x = spacing = 192.0F;
    edge = 160.0F;
    points[0].y = edge;
    colors[0].color = color->color;
    points[1].x = 448.0F;
    points[1].y = edge;
    colors[1].color = color->color;
    points[2].x = 448.0F;
    points[2].y = 208.0F;
    colors[2].color = color->color;
    points[3].x = spacing;
    points[3].y = 208.0F;
    colors[3].color = color->color;
    uv[0].tex.u = 0;
    uv[0].tex.v = 0;
    uv[1].tex.u = 255;
    uv[1].tex.v = 0;
    uv[2].tex.u = 255;
    uv[2].tex.v = 255;
    uv[3].tex.u = 0;
    uv[3].tex.v = 255;

    /* each choice's words, then its back behind them */
    quad.p[0].x = edge;
    quad.p[0].y = 368.0F;
    quad.p[1].x = 288.0F;
    quad.p[1].y = 368.0F;
    quad.p[2].x = 288.0F;
    quad.p[2].y = 400.0F;
    quad.p[3].x = edge;
    quad.p[3].y = 400.0F;
    njSetTextureNum(GAME_OVER_CONTINUE);
    njDrawPolygon2D(&quad, 4, -1.0F, QUAD_TEXTURED);
    njSetTextureNum(GAME_OVER_BACK);
    njDrawPolygon2D(&quad, 4, -1.02F, QUAD_TEXTURED);
    quad.p[0].x = 352.0F;
    quad.p[1].x = 480.0F;
    quad.p[2].x = 480.0F;
    quad.p[3].x = 352.0F;
    njSetTextureNum(GAME_OVER_QUIT);
    njDrawPolygon2D(&quad, 4, -1.0F, QUAD_TEXTURED);
    njSetTextureNum(GAME_OVER_BACK);
    njDrawPolygon2D(&quad, 4, -1.02F, QUAD_TEXTURED);

    /* the glow behind the chosen one's words, from orange at glow 0 to
     * yellow at 1 */
    njSetTextureNum(GAME_OVER_CURSOR);
    color->argb.b = 0;
    color->argb.g = 152.0F + (glow * 83.0F);
    color->argb.r = 236.0F + (glow * 19.0F);
    /* @hack spelling: offset set inside its first use, which loads 160 into R1
     * ahead of the glow's colors (MOV.W at 0x0c90ad30); set in a statement
     * of its own, the load follows offset's multiply, and 26 of 539 words
     * differ */
    quad.p[0].x = left = 160 + (offset = 192 * chosen);
    quad.p[0].y = 352.0F;
    quad.col[0].color = color->color;
    quad.p[1].x = right = 288 + offset;
    quad.p[1].y = 352.0F;
    quad.col[1].color = color->color;
    quad.p[2].x = right;
    quad.p[2].y = 416.0F;
    quad.col[2].color = color->color;
    quad.p[3].x = left;
    quad.p[3].y = 416.0F;
    quad.col[3].color = color->color;
    njDrawPolygon2D(&quad, 4, -1.01F, QUAD_TEXTURED);

    /* each choice's frame */
    blue = glow * 255.0F;
    if (chosen == CHOICE_CONTINUE) {
        /* @hack redundant: 0.0F added to blue before it converts, here and in
         * the second frame (FADD at 0x0c90ae60 and 0x0c90aee2); without it,
         * 291 of 539 words and 14 pool words differ */
        color->argb.b = 0.0F + blue;
        color->argb.g = 0;
        color->argb.r = 255.0F - blue;
    } else {
        color->argb.b = 255;
        color->argb.g = 0;
        color->argb.r = 0;
    }
    njSetTextureNum(GAME_OVER_FRAME);
    quad.p[0].x = edge;
    quad.col[0].color = color->color;
    quad.p[1].x = 288.0F;
    quad.col[1].color = color->color;
    quad.p[2].x = 288.0F;
    quad.col[2].color = color->color;
    quad.p[3].x = edge;
    quad.col[3].color = color->color;
    njDrawPolygon2D(&quad, 4, -1.02F, QUAD_TEXTURED);
    if (chosen == CHOICE_QUIT) {
        color->argb.b = 0.0F + blue;
        color->argb.g = 0;
        color->argb.r = 255.0F - blue;
    } else {
        color->argb.b = 255;
        color->argb.g = 0;
        color->argb.r = 0;
    }
    quad.p[0].x += spacing;
    quad.col[0].color = color->color;
    quad.p[1].x += spacing;
    quad.col[1].color = color->color;
    quad.p[2].x += spacing;
    quad.col[2].color = color->color;
    quad.p[3].x += spacing;
    quad.col[3].color = color->color;
    njDrawPolygon2D(&quad, 4, -1.02F, QUAD_TEXTURED);
}
