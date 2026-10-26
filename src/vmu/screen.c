/* the vmu screens: what the memory unit in each pad shows */
#include <ninja.h>
#include <sg_lcd.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "event/play.h"
#include "hud/hud.h"
#include "main.h"
#include "player/knuckles.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "vmu/draw.h"
#include "vmu/screen.h"

#define SCREENS 4

#define CHAO_BYTES (VMU_CHAO_SIDE * VMU_CHAO_SIDE / 8)
#define SCREEN_BYTES (VMU_WIDTH * VMU_HEIGHT / 8)
#define CHAO_PICTURES 70
#define SCREEN_PICTURES 16

#define PUZZLES_MAX 50
#define SLIDES_MAX 99

/* updates the puzzle waits for a button before it gives up */
#define PUZZLE_WAIT 18000

#define FADE_UPDATES 64

#define PUZZLE_PICTURES 19

/* puzzles solved between rewards, and to the last reward, after which the
 * round ends */
#define REWARD_EVERY 5
#define REWARD_LAST 15

#define BUTTONS_GO (PDD_DGT_TA | PDD_DGT_ST)
#define BUTTONS_PLAY                                                           \
    (PDD_DGT_KR | PDD_DGT_KL | PDD_DGT_KD | PDD_DGT_KU | PDD_DGT_TA)

enum { PORT_A, PORT_B, PORT_C, PORT_D };

enum screen_mode {
    SCREEN_START, /* clears the screen on its own update */
    SCREEN_CHOOSE,
    SCREEN_LOGO,
    CHAO_START,
    CHAO_GROUND,
    CHAO_AIR,
    CHAO_WATER,
    CHAO_HURT,
    CHAO_CLEAR,
    CHAO_NO_PLAYER,
    CHAO_IDLE,
    CHAO_NOW_AND_THEN,
    CHAO_NEAR,
    CHAO_GUARD,
    CHAO_CLIMB,
    CHAO_GAME_OVER,
    PUZZLE_TITLE,
    PUZZLE_TITLE_WAIT,
    PUZZLE_NEW,
    PUZZLE_PLAY,
    PUZZLE_SOLVED,
    PUZZLE_REWARD,
    PUZZLE_FADE,
    GARDEN_START,
    GARDEN
};

struct vmu_state {
    /* main's game mode, as the last update took it */
    enum game_mode game_mode;
    /* flips on each call while frame_interval is 1. a call that sets it
     * skips the update */
    int skip;
    /* the updates run so far, which numbers the update in progress. its low
     * three bits pick the screen whose own update that is, if any */
    int updates;
    int request; /* from vmu_request, for one update */
    struct vmu_screen screens[SCREENS];
};

static const Uint8 chao_pictures[CHAO_PICTURES][CHAO_BYTES] = {
#include "vmu/screen/chao_pictures.inc"
};

static const Uint8 screen_pictures[SCREEN_PICTURES][SCREEN_BYTES] = {
#include "vmu/screen/screen_pictures.inc"
};

/* each pad's memory unit port: the first sockets, which the screens take,
 * then the second, which nothing reads */
static const Uint32 lcd_ports[] = {
    PDD_PORT_A1, PDD_PORT_B1, PDD_PORT_C1, PDD_PORT_D1,
    PDD_PORT_A2, PDD_PORT_B2, PDD_PORT_C2, PDD_PORT_D2,
};

static const struct vmu_frame anim_ground[5] = {
#include "vmu/screen/anim_ground.inc"
};
static const struct vmu_frame anim_air[4] = {
#include "vmu/screen/anim_air.inc"
};
/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const struct vmu_frame dat_8c1747e8[2] = {
#include "vmu/screen/dat_8c1747e8.inc"
};
static const struct vmu_frame anim_hurt[2] = {
#include "vmu/screen/anim_hurt.inc"
};
static const struct vmu_frame anim_game_over[3] = {
#include "vmu/screen/anim_game_over.inc"
};
static const struct vmu_frame anim_near[4] = {
#include "vmu/screen/anim_near.inc"
};
static const struct vmu_frame anim_idle[5] = {
#include "vmu/screen/anim_idle.inc"
};
static const struct vmu_frame anim_water_2[3] = {
#include "vmu/screen/anim_water_2.inc"
};
static const struct vmu_frame anim_sleep[3] = {
#include "vmu/screen/anim_sleep.inc"
};
static const struct vmu_frame anim_now_and_then[5] = {
#include "vmu/screen/anim_now_and_then.inc"
};
static const struct vmu_frame anim_guard[5] = {
#include "vmu/screen/anim_guard.inc"
};
static const struct vmu_frame anim_guard_2[3] = {
#include "vmu/screen/anim_guard_2.inc"
};
static const struct vmu_frame anim_question[3] = {
#include "vmu/screen/anim_question.inc"
};
static const struct vmu_frame anim_climb[4] = {
#include "vmu/screen/anim_climb.inc"
};
static const struct vmu_frame anim_hurt_2[4] = {
#include "vmu/screen/anim_hurt_2.inc"
};
static const struct vmu_frame dat_8c174958[4] = {
#include "vmu/screen/dat_8c174958.inc"
};
static const struct vmu_frame anim_water[4] = {
#include "vmu/screen/anim_water.inc"
};
static const struct vmu_frame anim_guard_3[4] = {
#include "vmu/screen/anim_guard_3.inc"
};
static const struct vmu_frame dat_8c1749b8[5] = {
#include "vmu/screen/dat_8c1749b8.inc"
};
static const struct vmu_frame anim_climb_2[5] = {
#include "vmu/screen/anim_climb_2.inc"
};
static const struct vmu_frame dat_8c174a08[3] = {
#include "vmu/screen/dat_8c174a08.inc"
};
static const struct vmu_frame anim_clear[2] = {
#include "vmu/screen/anim_clear.inc"
};

/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const struct vmu_frame anim_logo_still[2] = {
#include "vmu/screen/anim_logo_still.inc"
};
static const struct vmu_frame anim_puzzle_title[2] = {
#include "vmu/screen/anim_puzzle_title.inc"
};
static const struct vmu_frame anim_logo[15] = {
#include "vmu/screen/anim_logo.inc"
};
static const struct vmu_frame anim_thank_you[2] = {
#include "vmu/screen/anim_thank_you.inc"
};
static const struct vmu_frame anim_good[2] = {
#include "vmu/screen/anim_good.inc"
};
static const struct vmu_frame anim_wao[2] = {
#include "vmu/screen/anim_wao.inc"
};
static const struct vmu_frame anim_garden[2] = {
#include "vmu/screen/anim_garden.inc"
};
static const struct vmu_frame anim_garden_blink[4] = {
#include "vmu/screen/anim_garden_blink.inc"
};
static const struct vmu_frame anim_garden_left[3] = {
#include "vmu/screen/anim_garden_left.inc"
};
static const struct vmu_frame anim_garden_right[3] = {
#include "vmu/screen/anim_garden_right.inc"
};

static struct vmu_state vmu;

static Uint32 screen_random_value;

static void screen_seed(void)
{
    screen_random_value = play_time_get();
}

/* the C standard's sample rand: 0 to 32767 */
static Uint16 screen_random(void)
{
    screen_random_value = (screen_random_value * 0x41c64e6d) + 12345;
    return (screen_random_value >> 16) & 0x7fff;
}

static void screen_probe(struct vmu_screen *screen)
{
    screen->ready = pdVmsLcdIsReady(screen->lcd);
    screen->direction = pdLcdGetDirection(screen->lcd);
}

static void screens_probe(void)
{
    struct vmu_screen *screen;
    struct vmu_screen *end;

    for (screen = vmu.screens, end = screen + SCREENS; screen < end; screen++) {
        screen_probe(screen);
    }
}

static Bool in_garden(void)
{
    /* @hack spelling: event_in_control compared with TRUE, CMP/EQ #1
     * (0x8c1178d0). it returns a comparison, 0 or 1. as a plain test, SHC
     * writes TST and 2 of 48 words differ */
    if (player_work[0] != NULL && event_in_control() != TRUE &&
        vmu.game_mode == GAME_MODE_FIELD &&
        (IN_STAGE(STAGE_ALIFE_SS) || IN_STAGE(STAGE_ALIFE_EG) ||
         IN_STAGE(STAGE_ALIFE_MR))) {
        return TRUE;
    }
    return FALSE;
}

static Bool garden_enter(struct vmu_screen *screen)
{
    if (in_garden()) {
        screen->mode = GARDEN_START;
        return TRUE;
    }
    return FALSE;
}

static Bool garden_leave(struct vmu_screen *screen)
{
    if (!in_garden()) {
        screen->mode = SCREEN_START;
        return TRUE;
    }
    return FALSE;
}

static void garden_start(struct vmu_screen *screen)
{
    vmu_set_anim(screen, anim_garden);
    screen->look_wait = 60;
    screen->roam_wait = 600;
    screen->pos.x = (screen_random() & 0x1c) - 16;
    screen->pos.y = VMU_HEIGHT;
    screen->target = screen->pos;
}

static void garden_look(struct vmu_screen *screen)
{
    Uint16 roll;

    if (--screen->look_wait <= 0) {
        roll = screen_random();
        switch (roll & 7) {
        case 0:
        case 1:
        case 2:
            vmu_set_anim(screen, anim_garden_blink);
            screen->look_wait = (roll & 63) + 60;
            break;
        case 3:
            vmu_set_anim(screen, anim_garden_left);
            screen->look_wait = (roll & 63) + 60;
            break;
        case 4:
            vmu_set_anim(screen, anim_garden_right);
            screen->look_wait = (roll & 63) + 60;
            break;
        default:
            vmu_set_anim(screen, anim_garden);
            screen->look_wait = (roll & 15) + 60;
            break;
        }
    }
}

static void garden_roam(struct vmu_screen *screen)
{
    Uint16 roll;

    if (--screen->roam_wait <= 0) {
        roll = screen_random();
        screen->roam_wait = (roll & 63) + 120;
        if (roll & 64) {
            switch (roll & 7) {
            case 0:
                screen->target.y = 0;
                break;
            case 1:
                screen->target.y = 4;
                break;
            case 2:
            case 3:
                screen->target.y = 16;
                break;
            default:
                /* down out of sight, where roam_wait gains 45 updates */
                screen->target.y = VMU_HEIGHT;
                screen->roam_wait += 45;
                break;
            }
        } else {
            screen->target.x = (roll & 0x1c) - 16;
        }
    }
    if (vmu_screen_due(screen)) {
        if (screen->pos.x < screen->target.x) {
            screen->pos.x += 2;
        } else if (screen->pos.x > screen->target.x) {
            screen->pos.x -= 2;
        }
        if (screen->pos.y < screen->target.y) {
            screen->pos.y += 2;
        } else if (screen->pos.y > screen->target.y) {
            screen->pos.y -= 2;
        }
    }
}

/* start the animation for the update's request. TRUE for any request, even
 * one with no case below, and FALSE for none */
static Bool take_request(struct vmu_screen *screen)
{
    /* @hack spelling: the request read through a pointer to vmu, which reads it
     * twice (MOV.L @(12,R4) at 0x8c117be2 and 0x8c117bf0); as vmu.request,
     * SHC reads it once and 6 of 44 words differ */
    struct vmu_state *state = &vmu;

    if (state->request == 0) {
        return FALSE;
    }
    switch (state->request) {
    case VMU_REQUEST_CLEAR:
        vmu_set_anim(screen, anim_clear);
        screen->wait = 480;
        screen->mode = CHAO_CLEAR;
        break;
    /* @hack redundant: a case for 2, which no caller asks for, and which keeps
     * CMP/EQ #2 among the switch's tests (0x8c117bf6); without it, 17 of 44
     * words differ */
    case 2:
        break;
    case VMU_REQUEST_GAME_OVER:
        vmu_set_anim(screen, anim_game_over);
        screen->wait = 300;
        screen->mode = CHAO_GAME_OVER;
        break;
    }
    return TRUE;
}

static Bool in_play(void)
{
    /* @hack spelling: event_in_control, which returns 0 or 1, compared with
     * TRUE as in in_garden (0x8c117c6c). as a plain test, 2 of 31 words
     * differ */
    if (player_work[0] != NULL && event_in_control() != TRUE && !in_garden()) {
        switch (vmu.game_mode) {
        case GAME_MODE_ACTION:
        case GAME_MODE_FIELD:
        case GAME_MODE_TRIAL:
            return TRUE;
        default:
            break;
        }
    }
    return FALSE;
}

static Bool chao_leave(struct vmu_screen *screen)
{
    /* @hack spelling: in_play's arm first, which returns FALSE on the
     * fall-through and branches to the mode's store (BT at 0x8c117cca); with
     * the test inverted, as in garden_leave, the store falls through and 10
     * of 19 words differ */
    /* @hack redundant: the else after the return, which stores the mode's 0
     * through R3 (MOV #0,R3 at 0x8c117cd6); without it, the 0 takes R2 and 3
     * of 19 words differ */
    if (in_play()) {
        return FALSE;
    } else {
        screen->mode = SCREEN_START;
        return TRUE;
    }
}

/* set the Chao's mode and animation for what the player does. TRUE when it
 * sets a mode or takes a request. the hurt and guard modes start over on
 * each call while the player is hurt or guards. any other mode starts only
 * when it isn't the one showing */
static Bool chao_react(struct vmu_screen *screen)
{
    int flags;
    Bool near;
    struct near_task *entry;
    struct player *record;
    struct task_work *work;

    if (!in_play()) {
        screen->mode = SCREEN_START;
        return TRUE;
    }
    /* @hack spelling: work assigned inside the test, which keeps it in the
     * frame (MOV.L R2,@R15 at 0x8c117d0c); assigned apart, work takes R12,
     * the frame shrinks from 16 bytes to 12 and 146 of 220 words differ */
    if ((work = player_work[0]) == NULL) {
        /* never runs: in_play has required a player */
        if (vmu.game_mode != GAME_MODE_ACTION) {
            screen->mode = SCREEN_START;
            return TRUE;
        }
        if (screen->mode == CHAO_NO_PLAYER) {
            return FALSE;
        }
        vmu_set_anim(screen, anim_game_over);
        screen->wait = 100;
        screen->mode = CHAO_NO_PLAYER;
    } else {
        if (take_request(screen)) {
            return TRUE;
        }
        flags = work->flags;
        /* @hack spelling: the goto past the hurt arm, which puts a pool just
         * ahead of the epilogue. the last return of TRUE branches over that
         * pool (BRA 0x8c117f62 at 0x8c117f22). with each arm's return of TRUE
         * in place, the function runs 30 bytes shorter, and 200 of 220 words
         * differ */
        if (!(flags & PLAYER_HURT)) {
            goto unhurt;
        }
        screen->wait = 100;
        vmu_set_anim(screen, anim_hurt);
        screen->mode = CHAO_HURT;
    }
    return TRUE;
unhurt:
    if (screen->mode != CHAO_CLIMB && get_character() == CHARACTER_KNUCKLES &&
        knuckles_get_activity(0) == KNUCKLES_CLIMBING) {
        screen->wait = 100;
        if (screen_random() & 1) {
            vmu_set_anim(screen, anim_climb_2);
        } else {
            vmu_set_anim(screen, anim_climb);
        }
        screen->mode = CHAO_CLIMB;
        return TRUE;
    }
    if (screen->mode != CHAO_GROUND && (flags & PLAYER_ON_GROUND)) {
        vmu_set_anim(screen, anim_ground);
        screen->mode = CHAO_GROUND;
        return TRUE;
    }
    if (screen->mode != CHAO_AIR && !(flags & PLAYER_ON_GROUND)) {
        vmu_set_anim(screen, anim_air);
        screen->mode = CHAO_AIR;
        return TRUE;
    }
    if (screen->mode != CHAO_WATER) {
        /* @hack spelling: record assigned inside the test, which tests
         * player_get's R0 and copies it to R4 in the BT/S's slot (0x8c117e46,
         * 0x8c117e4a); assigned apart, SHC tests R4 and 3 of 220 words
         * differ */
        if ((record = player_get(0)) != NULL &&
            record->shadow.above_flags == WORLD_WATER) {
            vmu_set_anim(screen, anim_water);
            screen->mode = CHAO_WATER;
            return TRUE;
        }
    }
    if ((flags & PLAYER_ON_GROUND) && (flags & PLAYER_GUARD)) {
        const struct vmu_frame *anims[3] = {
            anim_guard,
            anim_guard_2,
            anim_guard_3,
        };

        vmu_set_anim(screen, anims[screen_random() % 3]);
        screen->wait = 50;
        screen->mode = CHAO_GUARD;
        return TRUE;
    }
    if (screen->mode == CHAO_NEAR) {
        return FALSE;
    }
    near = FALSE;
    for (entry = near_tasks; entry->work != NULL; entry++) {
        if (entry->distance < 30.0F * 30.0F) {
            near = TRUE;
            break;
        }
    }
    if (near) {
        vmu_set_anim(screen, anim_near);
        screen->wait = 59;
        screen->mode = CHAO_NEAR;
        return TRUE;
    }
    return FALSE;
}

static Bool chao_enter(struct vmu_screen *screen)
{
    if (in_play()) {
        vmu_set_anim(screen, anim_ground);
        screen->mode = CHAO_START;
        return TRUE;
    }
    return FALSE;
}

static Bool logo_show(struct vmu_screen *screen)
{
    /* @hack redundant: an if (1), which sets R0 to 1 ahead of the RTS and
     * leaves a NOP in its slot (0x8c117fb6, 0x8c117fbe); without it, SHC sets
     * R0 in the slot and 5 of 15 words differ */
    if (1) {
        vmu_set_anim(screen, anim_logo);
        screen->mode = SCREEN_LOGO;
        return TRUE;
    }
    return FALSE;
}

static Bool logo_leave(struct vmu_screen *screen)
{
    if (in_garden()) {
        screen->mode = SCREEN_START;
        return TRUE;
    }
    return FALSE;
}

/* the cell that holds the blank tile, or PUZZLE_CELLS for none */
static Sint8 puzzle_find_blank(struct vmu_screen *screen)
{
    Sint8 i = 0;

    while (i < PUZZLE_CELLS && screen->blank != screen->tiles[i]) {
        i++;
    }
    return i;
}

/* slide the tiles between cell and the blank toward the blank, when the two
 * share a row or a column. TRUE when they slid */
static Bool puzzle_slide(struct vmu_screen *screen, Sint8 cell)
{
    Sint8 col;
    Sint8 row;
    Sint8 blank;
    Sint8 blank_col;
    Sint8 blank_row;
    Sint8 i;

    col = cell % PUZZLE_SIDE;
    row = cell / PUZZLE_SIDE;
    blank = puzzle_find_blank(screen);
    blank_col = blank % PUZZLE_SIDE;
    blank_row = blank / PUZZLE_SIDE;
    if (col == blank_col && row == blank_row) {
        return FALSE;
    }
    /* @hack spelling: each arm stores the blank and returns on its own, which
     * makes the frame 8 bytes (ADD #-8,R15 at 0x8c11806a); with one store
     * and return after the arms, it is 4 and 90 of 137 words differ */
    if (col == blank_col) {
        if (row < blank_row) {
            for (i = blank; i > cell; i -= PUZZLE_SIDE) {
                screen->tiles[i] = screen->tiles[i - PUZZLE_SIDE];
            }
            screen->tiles[i] = screen->blank;
            return TRUE;
        } else {
            for (i = blank; i < cell; i += PUZZLE_SIDE) {
                screen->tiles[i] = screen->tiles[i + PUZZLE_SIDE];
            }
            screen->tiles[i] = screen->blank;
            return TRUE;
        }
    } else if (row == blank_row) {
        if (col < blank_col) {
            for (i = blank; i > cell; i--) {
                screen->tiles[i] = screen->tiles[i - 1];
            }
            screen->tiles[i] = screen->blank;
            return TRUE;
        } else {
            for (i = blank; i < cell; i++) {
                screen->tiles[i] = screen->tiles[i + 1];
            }
            screen->tiles[i] = screen->blank;
            return TRUE;
        }
    }
    return FALSE;
}

/* move the cursor by the pad's presses, and slide from its cell on A. any
 * of BUTTONS_PLAY restarts the countdown */
static void puzzle_input(struct vmu_screen *screen)
{
    if (screen->pressed & PDD_DGT_KU) {
        /* @hack spelling: the move assigned inside its test, here and in the
         * move down, which tests the byte it stores, extended (EXTS.B at
         * 0x8c1181f6 and 0x8c118214); as statements, SHC reloads the cursor
         * and 8 of 100 words differ */
        if ((screen->cursor -= PUZZLE_SIDE) < 0) {
            screen->cursor += PUZZLE_SIDE;
        }
    }
    if (screen->pressed & PDD_DGT_KD) {
        if ((screen->cursor += PUZZLE_SIDE) > PUZZLE_CELLS - 1) {
            screen->cursor -= PUZZLE_SIDE;
        }
    }
    if (screen->pressed & PDD_DGT_KR) {
        screen->cursor++;
        if (screen->cursor % PUZZLE_SIDE == 0) {
            screen->cursor--;
        }
    }
    if (screen->pressed & PDD_DGT_KL) {
        screen->cursor--;
        if (screen->cursor < 0 ||
            screen->cursor % PUZZLE_SIDE == PUZZLE_SIDE - 1) {
            screen->cursor++;
        }
    }
    if (screen->pressed & PDD_DGT_TA) {
        if (puzzle_slide(screen, screen->cursor)) {
            if (++screen->slides > SLIDES_MAX) {
                screen->slides = SLIDES_MAX;
            }
        }
    }
    if (screen->pressed & BUTTONS_PLAY) {
        screen->countdown = PUZZLE_WAIT;
    }
}

/* make screen->puzzles slides, each from a cell picked at random in the
 * blank's column or row. the two lines take turns, as along_row says */
static void puzzle_shuffle(struct vmu_screen *screen)
{
    int count;
    Sint8 blank_row;
    Sint8 blank_col;
    Sint8 in_line[4]; /* the other cells in the blank's row and column */
    Sint8 *add;
    Sint8 i;
    Sint8 blank;
    Sint8 line;
    Uint16 pick;
    int tries;
    Sint8 cell;

    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    for (count = screen->puzzles; count > 0; count--) {
        blank = puzzle_find_blank(screen);
        blank_col = blank % PUZZLE_SIDE;
        blank_row = blank / PUZZLE_SIDE;
        for (add = in_line, i = 0; i < PUZZLE_CELLS; i++) {
            if (i == blank) {
                continue;
            }
            if (i % PUZZLE_SIDE == blank_col || i / PUZZLE_SIDE == blank_row) {
                *add = i;
                add++;
            }
        }
        if (screen->along_row == 0) {
            line = blank % PUZZLE_SIDE;
            pick = screen_random() & 3;
            tries = 4;
            while (tries > 0 && line != in_line[pick] % PUZZLE_SIDE) {
                tries--;
                pick = (pick + 1) & 3;
            }
            cell = in_line[pick];
            screen->along_row = 1;
        } else {
            line = blank / PUZZLE_SIDE;
            pick = screen_random() & 3;
            tries = 4;
            while (tries > 0 && line != in_line[pick] / PUZZLE_SIDE) {
                tries--;
                pick = (pick + 1) & 3;
            }
            cell = in_line[pick];
            screen->along_row = 0;
        }
        puzzle_slide(screen, cell);
    }
}

/* TRUE when each cell holds its own tile */
static Bool puzzle_solved(struct vmu_screen *screen)
{
    Sint8 *tile;
    int i;

    for (i = 0, tile = screen->tiles; i < PUZZLE_CELLS; i++, tile++) {
        /* @hack spelling: the continue, which has SHC enter the loop at its
         * test (BRA 0x8c1184e4 at 0x8c1184a6); with the test inverted, SHC
         * enters at the body and 14 of 17 words differ */
        if (i == *tile) {
            continue;
        }
        return FALSE;
    }
    return TRUE;
}

/* start the next puzzle. up to ten picks look for a blank other than the
 * middle tile. up to nine shuffles look for tiles out of order, and up to
 * ten picks for a picture other than the one showing. where all fail, the
 * last stands */
static void puzzle_new(struct vmu_screen *screen)
{
    const struct vmu_frame *picture;
    const struct vmu_frame *pictures[PUZZLE_PICTURES] = {
        anim_ground,  anim_air,     anim_game_over,    anim_near,
        anim_idle,    anim_sleep,   anim_now_and_then, anim_water_2,
        anim_guard,   anim_guard_2, anim_question,     anim_climb,
        anim_hurt_2,  dat_8c174958, anim_water,        anim_guard_3,
        dat_8c1749b8, anim_climb_2, dat_8c174a08,
    };
    int tries;
    int i;

    screen->slides = 0;
    if (++screen->puzzles > PUZZLES_MAX) {
        screen->puzzles = PUZZLES_MAX;
    }
    for (tries = 10; tries > 0; tries--) {
        screen->blank = screen_random() % PUZZLE_CELLS;
        if (screen->blank != PUZZLE_CELLS / 2) {
            break;
        }
    }
    for (i = 0; i < PUZZLE_CELLS; i++) {
        screen->tiles[i] = i;
    }
    for (tries = 9; tries > 0; tries--) {
        puzzle_shuffle(screen);
        if (!puzzle_solved(screen)) {
            break;
        }
    }
    screen->cursor = puzzle_find_blank(screen);
    tries = 10;
    /* @hack spelling: each pick assigned inside its test, which leaves the
     * loops above their tries in R12 (MOV #10,R12 at 0x8c11851c) and keeps
     * the pick in the frame (MOV.L R3,@R15 at 0x8c11859e). assigned apart,
     * tries takes R11, the table's address R12, and 120 of 124 words
     * differ */
    if (screen->anim ==
        (picture = pictures[screen_random() % PUZZLE_PICTURES])) {
        while (--tries > 0 &&
               screen->anim ==
                   (picture = pictures[screen_random() % PUZZLE_PICTURES])) {
        }
    }
    vmu_set_anim(screen, picture);
    screen->countdown = PUZZLE_WAIT;
}

/* count the countdown down. TRUE when it runs out, which sends the screen
 * to SCREEN_START */
static Bool puzzle_timeout(struct vmu_screen *screen)
{
    if (--screen->countdown <= 0) {
        screen->mode = SCREEN_START;
        return TRUE;
    }
    return FALSE;
}

static void screen_read_pad(struct vmu_screen *screen)
{
    int port;
    Sint32 old;
    Sint32 now;

    old = screen->held;
    port = 0;
    while (port < SCREENS && pad_ports[port] != screen->port) {
        port++;
    }
    if (port == SCREENS) {
        now = 0;
    } else {
        now = per_info[port]->on;
    }
    screen->held = now;
    screen->pressed = (old ^ now) & now;
}

/* @hack spelling: puzzle_begin, a helper SHC expands, whose parameter keeps
 * a copy of screen in the frame across screen_seed's call (MOV.L R14,@R15 at
 * 0x8c118a96, MOV.L @R15,R2 at 0x8c118a9a); written in place, 142 of
 * screen_step's 557 words differ, and called, 549 */
#pragma inline(puzzle_begin)
static void puzzle_begin(struct vmu_screen *screen)
{
    screen_seed();
    screen->puzzles = 0;
}

/* run the screen's mode for one update, then write the screen to its lcd.
 * SCREEN_CHOOSE returns before the write */
static void screen_step(struct vmu_screen *screen)
{
    if (!screen->ready) {
        screen->mode = SCREEN_START;
    }
    switch (screen->mode) {
    case SCREEN_START:
        if (vmu_screen_due(screen)) {
            vmu_clear(screen->pixels[0]);
            screen->mode = SCREEN_CHOOSE;
            /* @hack redundant: a break of its own, which sends the arm to a
             * mode store it shares with other arms (BRA 0x8c118baa at
             * 0x8c118762); without it, the arm stores its mode itself and 60 of
             * 557 words differ */
            break;
        }
        break;
    case SCREEN_CHOOSE:
        /* port a's screen shows the game, and c's and d's the puzzle once
         * start is pressed. b's shows nothing */
        switch (screen->port) {
        case PORT_A:
            if (garden_enter(screen)) {
                return;
            }
            if (chao_enter(screen)) {
                return;
            }
            if (logo_show(screen)) {
                return;
            }
            break;
        case PORT_B:
            return;
        case PORT_C:
        case PORT_D:
            /* @hack spelling: a return in each arm, which stores PUZZLE_TITLE
             * through R2 (MOV #16,R2 at 0x8c1187f2); with one return after the
             * test, it goes through R3 and 2 of 557 words differ */
            if (!(screen->pressed & PDD_DGT_ST)) {
                return;
            } else {
                vmu_clear(screen->pixels[0]);
                screen->mode = PUZZLE_TITLE;
                return;
            }
        }
        return;
    case SCREEN_LOGO:
        if (chao_enter(screen)) {
            break;
        }
        if (logo_leave(screen)) {
            break;
        }
        vmu_step_anim(screen);
        vmu_draw_picture(screen);
        break;
    case CHAO_START:
        if (chao_leave(screen)) {
            break;
        }
        vmu_clear(screen->pixels[0]);
        vmu_set_anim(screen, anim_ground);
        screen->mode = CHAO_GROUND;
        break;
    case CHAO_GROUND: {
        struct player *record;

        if (chao_react(screen)) {
            break;
        }
        /* @hack spelling: record assigned inside the test, which leaves
         * vmu_set_anim's address in R12 and vmu_step_anim's in R11 (0x8c1186e8,
         * 0x8c1186ee); assigned apart, the two trade and 17 of 557 words
         * differ */
        if ((record = player_get(0)) != NULL) {
            if (njScalor(&record->speed) < 0.2F) {
                if (++screen->wait > 600) {
                    const struct vmu_frame *idle[2] = {anim_idle, anim_sleep};

                    /* @hack type: the shift's operand cast to Uint32, which
                     * shifts by two SHLR2s (0x8c1188b0, 0x8c1188b2); left an
                     * int, it shifts by SHAD and 162 of 557 words differ */
                    vmu_set_anim(screen,
                                 idle[(Uint32)(play_time_get() & 0x10) >> 4]);
                    screen->mode = CHAO_IDLE;
                    break;
                }
            } else {
                /* the player moves, so wait starts over. while speed.x is
                 * past 2, the timer gains 4 on every update, which hurries
                 * the animation */
                screen->wait = 0;
                if (record->speed.x > 2.0F) {
                    screen->timer += 4;
                }
            }
        }
        if (!(play_time_get() & 0x7ff)) {
            vmu_set_anim(screen, anim_now_and_then);
            screen->wait = 99;
            screen->mode = CHAO_NOW_AND_THEN;
            break;
        }
        vmu_step_anim(screen);
        vmu_draw_chao(screen);
        break;
    }
    case CHAO_AIR:
    case CHAO_WATER:
        if (chao_react(screen)) {
            break;
        }
        vmu_step_anim(screen);
        vmu_draw_chao(screen);
        break;
    case CHAO_HURT:
        if (chao_leave(screen)) {
            break;
        }
        if (take_request(screen)) {
            break;
        }
        if (--screen->wait <= 0) {
            vmu_set_anim(screen, anim_ground);
            screen->mode = CHAO_GROUND;
            break;
        }
        vmu_draw_chao(screen);
        break;
    case CHAO_CLEAR:
    case CHAO_GAME_OVER:
        if (--screen->wait <= 0) {
            screen->mode = SCREEN_START;
            break;
        }
        vmu_step_anim(screen);
        vmu_draw_chao(screen);
        break;
    case CHAO_NO_PLAYER:
    case CHAO_NOW_AND_THEN:
    case CHAO_NEAR:
    case CHAO_GUARD:
    case CHAO_CLIMB:
        if (take_request(screen)) {
            break;
        }
        if (--screen->wait <= 0) {
            vmu_set_anim(screen, anim_ground);
            screen->mode = CHAO_GROUND;
            break;
        }
        vmu_step_anim(screen);
        vmu_draw_chao(screen);
        break;
    case CHAO_IDLE: {
        struct player *record;

        if (chao_leave(screen)) {
            break;
        }
        if (take_request(screen)) {
            break;
        }
        if ((record = player_get(0)) != NULL &&
            njScalor(&record->speed) > 0.2F) {
            vmu_set_anim(screen, anim_ground);
            screen->mode = CHAO_GROUND;
            break;
        }
        vmu_step_anim(screen);
        vmu_draw_chao(screen);
        break;
    }
    case PUZZLE_TITLE:
        vmu_clear(screen->pixels[0]);
        vmu_set_anim(screen, anim_puzzle_title);
        screen->countdown = PUZZLE_WAIT;
        screen->mode = PUZZLE_TITLE_WAIT;
        break;
    case PUZZLE_TITLE_WAIT:
        vmu_draw_picture(screen);
        if (screen->pressed & BUTTONS_GO) {
            vmu_clear(screen->pixels[0]);
            puzzle_begin(screen);
            screen->mode = PUZZLE_NEW;
            break;
        }
        if (puzzle_timeout(screen) || (screen->pressed & PDD_DGT_TB)) {
            screen->mode = SCREEN_START;
            break;
        }
        break;
    case PUZZLE_NEW:
        puzzle_new(screen);
        screen->mode = PUZZLE_PLAY;
        break;
    case PUZZLE_PLAY:
        /* B gives the puzzle up, and takes it off the count */
        if (screen->pressed & PDD_DGT_TB) {
            screen->puzzles--;
            screen->wait = 0;
            screen->blink = 0;
            screen->mode = PUZZLE_FADE;
            break;
        }
        puzzle_input(screen);
        vmu_step_anim(screen);
        vmu_draw_chao(screen);
        vmu_draw_puzzle(screen);
        vmu_draw_tally(screen);
        if (puzzle_solved(screen)) {
            screen->mode = PUZZLE_SOLVED;
            break;
        }
        if (puzzle_timeout(screen)) {
            break;
        }
        screen->blink++;
        break;
    case PUZZLE_SOLVED:
        vmu_clear(screen->pixels[0]);
        vmu_step_anim(screen);
        vmu_draw_chao(screen);
        vmu_draw_tally(screen);
        if (screen->pressed & BUTTONS_GO) {
            screen->wait = 0;
            screen->blink = 1;
            screen->mode = PUZZLE_FADE;
            break;
        }
        screen->blink = 0;
        break;
    case PUZZLE_REWARD:
        vmu_draw_picture(screen);
        if (screen->pressed & BUTTONS_GO) {
            if (screen->puzzles == REWARD_LAST) {
                screen->mode = PUZZLE_TITLE;
                break;
            } else {
                screen->mode = PUZZLE_NEW;
                break;
            }
        }
        if (puzzle_timeout(screen)) {
            break;
        }
        break;
    case PUZZLE_FADE:
        vmu_clear(screen->pixels[0]);
        vmu_draw_chao(screen);
        {
            struct vmu_point size;

            size.x = size.y = screen->wait >> 4;
            vmu_pixelate(screen, &size);
        }
        if (++screen->wait >= FADE_UPDATES) {
            if (screen->blink == 1 && screen->puzzles % REWARD_EVERY == 0) {
                const struct vmu_frame *rewards[3] = {
                    anim_good,
                    anim_wao,
                    anim_thank_you,
                };

                vmu_set_anim(screen,
                             rewards[(screen->puzzles / REWARD_EVERY) - 1]);
                screen->countdown = PUZZLE_WAIT;
                screen->mode = PUZZLE_REWARD;
                break;
            } else {
                screen->mode = PUZZLE_NEW;
                break;
            }
        }
        break;
    case GARDEN_START:
        if (garden_leave(screen)) {
            break;
        }
        vmu_clear(screen->pixels[0]);
        garden_start(screen);
        screen->mode = GARDEN;
        break;
    case GARDEN:
        if (garden_leave(screen)) {
            break;
        }
        garden_look(screen);
        garden_roam(screen);
        if (vmu_screen_due(screen)) {
            vmu_step_anim(screen);
            vmu_draw_picture(screen);
            vmu_shift(screen, &screen->pos);
        }
        break;
    }
    vmu_write(screen);
}

void vmu_request(int request)
{
    vmu.request = request;
}

Bool vmu_screen_due(struct vmu_screen *screen)
{
    if ((vmu.updates & 7) == screen->port) {
        return TRUE;
    }
    return FALSE;
}

void vmu_init(void)
{
    struct vmu_screen *screen;
    struct vmu_screen *end;
    const Uint32 *lcd;
    int port;

    port = 0;
    for (screen = vmu.screens, end = screen + SCREENS, lcd = lcd_ports;
         screen < end; screen++, lcd++, port++) {
        vmu_clear(screen->pixels[0]);
        screen->mode = SCREEN_START;
        screen->lcd = *lcd;
        screen->port = port;
        screen->pressed = screen->held = 0;
    }
    screens_probe();
    vmu.skip = 0;
    vmu.updates = 0;
    vmu.request = 0;
}

void vmu_frame_update(enum game_mode game_mode)
{
    struct vmu_screen *screen;
    struct vmu_screen *end;

    if (frame_interval == 1 && (vmu.skip ^= 1) != 0) {
        return;
    }
    vmu.game_mode = game_mode;
    for (screen = vmu.screens, end = screen + SCREENS; screen < end; screen++) {
        if (vmu_screen_due(screen)) {
            screen_probe(screen);
        }
        screen_read_pad(screen);
        screen_step(screen);
    }
    vmu.updates++;
    vmu.request = 0;
}
