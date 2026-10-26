/* the inset: a box on the screen that draws the stage again, looking down
 * on player 1 */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/inset.h"
#include "main.h"
#include "player/player.h"
#include "system/task.h"

/* the size of the tiles njUserClipping clips by, in pixels */
#define TILE 32

/* how far above player 1 the inset's view stands */
#define EYE_HEIGHT 100.0F

/* inset_flags' bits. inset_show, inset_hide and inset_toggle change
 * INSET_ASKED, and inset_update copies it to INSET_UP, which inset_clip and
 * inset_draw read */
#define INSET_ASKED 0x1
#define INSET_UP 0x2

/* the color inset_clip fills the inset's tiles with, as ARGB */
#define WHITE 0xffffffff

static Uint32 inset_flags = 0;
static NJS_SCREEN inset_full_screen = {500.0F, SCREEN_WIDTH, SCREEN_HEIGHT,
                                       SCREEN_MIDDLE_X, SCREEN_MIDDLE_Y};
static NJS_SCREEN inset_screen = {500.0F, SCREEN_WIDTH / 4, SCREEN_HEIGHT / 4,
                                  SCREEN_WIDTH * 3 / 4, SCREEN_HEIGHT * 3 / 4};
static struct task *inset_task;
static NJS_VIEW inset_view;

static void look_down(struct task_work *work, struct task_motion *motion);

void inset_clip(enum inset_clipping clip)
{
    NJS_POINT2 tiles[2];
    NJS_POINT2COL quad;
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];
    NJS_SCREEN *full = &inset_full_screen;
    NJS_SCREEN *inset = &inset_screen;
    Float left;
    Float right;
    Float top;
    Float bottom;
    Float full_left;
    Float full_right;
    Float full_top;
    Float full_bottom;
    int i;

    /* full and inset point at this file's screens, so the test never holds.
     * the image tests both all the same (0x8c09da12, 0x8c09da16) */
    if (full == NULL && inset == NULL) {
        return;
    }
    if (!(inset_flags & INSET_UP)) {
        clip = INSET_CLIP_NONE;
    }
    /* the inset's edges, held within the full screen's */
    left = inset->cx - (inset->w * 0.5F);
    top = inset->cy - (inset->h * 0.5F);
    right = inset->cx + (inset->w * 0.5F);
    bottom = inset->cy + (inset->h * 0.5F);
    full_left = full->cx - (full->w * 0.5F);
    full_top = full->cy - (full->h * 0.5F);
    full_right = full->cx + (full->w * 0.5F);
    full_bottom = full->cy + (full->h * 0.5F);
    if (left < full_left) {
        left = full_left;
    }
    if (right > full_right) {
        right = full_right;
    }
    if (top < full_top) {
        top = full_top;
    }
    if (bottom > full_bottom) {
        bottom = full_bottom;
    }
    /* NOLINTBEGIN(bugprone-integer-division) */
    tiles[0].x = (int)left / TILE;
    tiles[0].y = (int)top / TILE;
    tiles[1].x = (int)right / TILE;
    tiles[1].y = (int)bottom / TILE;
    /* NOLINTEND(bugprone-integer-division) */
    switch (clip) {
    case INSET_CLIP_NONE:
        njUserClipping(NJD_CLIP_DISABLE, NULL);
        _nj_screen_.cx = full->cx;
        _nj_screen_.cy = full->cy;
        _nj_screen_.w = full->w;
        _nj_screen_.h = full->h;
        break;
    case INSET_CLIP_AROUND:
        njUserClipping(NJD_CLIP_OUTSIDE, tiles);
        _nj_screen_.cx = full->cx;
        _nj_screen_.cy = full->cy;
        _nj_screen_.w = full->w;
        _nj_screen_.h = full->h;
        break;
    case INSET_CLIP_INSIDE:
        njUserClipping(NJD_CLIP_INSIDE, tiles);
        _nj_screen_.cx = inset->cx;
        _nj_screen_.cy = inset->cy;
        _nj_screen_.w = inset->w;
        _nj_screen_.h = inset->h;
        quad.p = points;
        quad.col = colors;
        quad.num = 4;
        points[0].x = points[1].x = tiles[0].x * TILE;
        points[2].x = points[3].x = ((tiles[1].x + 1.0F) * TILE) - 1.0F;
        points[0].y = points[3].y = tiles[0].y * TILE;
        points[1].y = points[2].y = ((tiles[1].y + 1.0F) * TILE) - 1.0F;
        for (i = 0; i < 4; i++) {
            colors[i].color = WHITE;
        }
        njDrawPolygon2D(&quad, 4, 0.1F, NJD_DRAW_NORMAL);
        break;
    }
}

void inset_show(void)
{
    inset_flags |= INSET_ASKED;
}

void inset_hide(void)
{
    inset_flags &= ~INSET_ASKED;
}

void inset_toggle(void)
{
    inset_flags ^= INSET_ASKED;
}

Bool inset_update(void)
{
    if (inset_flags & INSET_ASKED) {
        inset_flags |= INSET_UP;
        return TRUE;
    }
    inset_flags &= ~INSET_UP;
    return FALSE;
}

void inset_set_size(enum inset_clipping clip, Float width, Float height)
{
    switch (clip) {
    case INSET_CLIP_NONE:
    case INSET_CLIP_AROUND:
        inset_full_screen.w = width;
        inset_full_screen.h = height;
        break;
    case INSET_CLIP_INSIDE:
        inset_screen.w = width;
        inset_screen.h = height;
        break;
    }
}

void inset_resize(enum inset_clipping clip, Float width, Float height)
{
    switch (clip) {
    case INSET_CLIP_NONE:
    case INSET_CLIP_AROUND:
        inset_full_screen.w = width;
        inset_full_screen.h = height;
        break;
    case INSET_CLIP_INSIDE:
        inset_screen.w = width;
        inset_screen.h = height;
        break;
    }
}

void inset_create_task(void (*step)(struct task *task))
{
    inset_task = task_create(TASK_WORK, 0, step);
}

void inset_step_task(void)
{
    struct task *task = inset_task;

    if (task != NULL && task->step != NULL) {
        task->step(task);
    }
}

void inset_draw(void)
{
    Float dist;
    int level;

    if (!(inset_flags & INSET_UP)) {
        return;
    }
    dist = _nj_screen_.dist;
    njSetPerspective(NJM_DEG_ANG(30));
    njPushMatrix(NULL);
    look_down(player_work[1], player_motion[1]);
    for (level = 1; level <= TASK_LEVELS - 1; level++) {
        task_draw_level(level);
    }
    njPopMatrix(1);
    _nj_screen_.dist = dist;
}

/* set ninja's view to inset_view, looking straight down on work from
 * EYE_HEIGHT above it. motion goes unread */
static void look_down(struct task_work *work, struct task_motion *motion)
{
    Float x;
    Float y;
    Float z;
    Angle ang_x;
    Angle ang_y;
    Angle ang_z;
    /* @hack redundant: 24 bytes of the frame that the function never reads.
     * their type is unknown. the image's ADD #-40,R15 at 0x8c09ddd8 reserves
     * them; without them, 2 of 47 words differ */
    Uint8 unused[24];

    x = work->pos.x;
    y = work->pos.y + EYE_HEIGHT;
    z = work->pos.z;
    ang_x = NJM_DEG_ANG(-90);
    ang_y = 0;
    ang_z = 0;
    njInitView(&inset_view);
    njRotateViewZRelative(&inset_view, ang_z);
    njRotateViewXRelative(&inset_view, ang_x);
    njRotateViewYRelative(&inset_view, ang_y);
    njTranslateViewRelative(&inset_view, x, y, z);
    njSetView(&inset_view);
}
