/* Tikal's hints. O TIKAL in the object lists raises a light where it's
 * placed, and the light circles player 0 while Tikal's hint shows. a boss's
 * hint grows the same kind of light beside player 0 */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "event/script.h"
#include "object/common.h"
#include "object/helpers.h"
#include "object/placement.h"
#include "object/tikal.h"
#include "object/tikal_light.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/math.h"
#include "system/task.h"
#include "system/util.h"

/* squared, as point_distance_squared measures: a placed hint's light rises once
 * player 0 is within 200, and speaks once the player is within 15 */
#define SHOW_RANGE (200.0F * 200.0F)
#define SPEAK_RANGE (15.0F * 15.0F)

/* the light circles player 0 this far out, CIRCLE_TURN a step */
#define CIRCLE_RADIUS 15.0F
#define CIRCLE_TURN 0x2b7

/* a placed hint's light rises for RISE_STEPS, by RISE_HEIGHT / RISE_STEPS
 * on each step but the last, and grows to full scale and RISE_ALPHA */
#define RISE_STEPS 60
#define RISE_HEIGHT 20.0F
#define RISE_ALPHA 0.8F
#define HINT_STEPS 23 /* a boss's hint grows beside player 0 */
/* the light spirals off for LEAVE_STEPS, from speed 1 and faster by
 * LEAVE_ACCEL each step. each step it aims LEAVE_CLIMB above its height */
#define LEAVE_STEPS 240
#define LEAVE_ACCEL (0.95F / 12.0F)
#define LEAVE_CLIMB 0.5F
#define REST_STEPS 300 /* a placed hint rests before it starts over */

#define FULL_SCALE 4.0F
#define FULL_ALPHA 1.0F
/* a boss's hint grows or fades by these a step, just over FULL_SCALE / 24
 * and FULL_ALPHA / 24 */
#define HINT_SCALE_STEP 0.16666668F
#define HINT_ALPHA_STEP 0.04166667F

/* the size of the marker the set editor shows for a placed hint with no
 * light */
#define MARKER_SIZE 4.0F

#define SE_SPEAK 6
#define SE_LEAVE 721

/* a hint's submode: placed by O TIKAL, or a boss's */
enum tikal_kind { TIKAL_PLACED, TIKAL_BOSS };

/* a placed hint's modes */
enum tikal_mode {
    TIKAL_RISE,
    TIKAL_WAIT,
    TIKAL_CIRCLE,
    TIKAL_LEAVE,
    TIKAL_REST
};

/* a boss's hint's modes */
enum tikal_hint_mode {
    TIKAL_HINT_SHOW, /* it grows beside player 0, then speaks */
    TIKAL_HINT_CIRCLE,
    TIKAL_HINT_LEAVE,
    TIKAL_HINT_FADE /* it shrinks away unheard, still circling */
};

/* a hint's view of task_work. hint lies over ang, and scale and alpha over
 * scl.x and scl.y. tikal_step labels their rows in the set editor, ang.x
 * to scl.y, "Don't EDIT!" */
struct tikal_work {
    Sint8 mode;
    Sint8 submode; /* TIKAL_PLACED or TIKAL_BOSS */
    Uint8 id;
    Uint8 shown; /* 1 while the hint's message shows */
    Sint16 flags;
    Sint16 timer;
    /* the light's task, or NULL when none was made or once it's destroyed */
    struct task *light;
    /* @hack type: unsigned, so tikal_hint_create stores its 0 and passes it
     * to tikal_place from R13 (0x8c0639d8, 0x8c0639da); as an Angle, SHC
     * makes a new 0 in R2, tikal_hint_create runs 2 bytes long and 42 of
     * 113 words differ */
    Uint32 phase; /* the light's angle round player 0 */
    Float speed; /* the light's speed as it spirals off */
    struct script_pages hint;
    NJS_POINT3 pos;
    Float scale;
    Float alpha;
    Float row; /* scl.z, the hint's row of the stage's script table */
    struct collision_work *collision;
    struct event_work *event;
};

#define TIKAL(task) ((struct tikal_work *)(task)->work)

static void tikal_rise(struct task *task);
static void tikal_wait(struct task *task);
static void tikal_place(NJS_POINT3 *pos, Angle phase);
static void tikal_circle(struct task *task);
static void tikal_leave(struct task *task);
static void tikal_rest(struct task *task);
static void tikal_step(struct task *task);
static void tikal_destroy(struct task *task);
static void tikal_hint_fade(struct task *task);
static void tikal_hint_show(struct task *task);
static void tikal_hint_step(struct task *task);
static void tikal_hint_destroy(struct task *task);

/* the last hint tikal_hint_create made, or NULL once a hint's task is
 * destroyed or a create fails */
static struct task *tikal_hint_task = NULL;

void tikal_init(struct task *task)
{
    struct tikal_work *work = TIKAL(task);
    NJS_POINT3 player;

    /* go no further unless tikal_work keeps these fields where task_work
     * does */
    /* @hack redundant: pos tested twice, as the image tests it (ADD #32,R2 at
     * 0x8c0637d2 and 0x8c06380c); tested once, 124 of 147 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (!WORK_SAME_PLACE(struct tikal_work, pos) ||
        !WORK_SAME_PLACE(struct tikal_work, id) ||
        !WORK_SAME_PLACE(struct tikal_work, collision) ||
        !WORK_SAME_PLACE(struct tikal_work, event) ||
        !WORK_SAME_PLACE(struct tikal_work, pos) ||
        !WORK_SAME_PLACE(struct tikal_work, flags)) {
        return;
    }
    /* NOLINTEND(misc-redundant-expression) */
    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task) || script_pick((int)work->row, &work->hint)) {
        /* keep the hint's place in task->extra, for tikal_rest to start
         * over from */
        if (task->extra == NULL) {
            task->extra = heap_alloc(sizeof(NJS_POINT3));
        }
        if (task->extra != NULL) {
            NJS_POINT3 *home = task->extra;

            *home = work->pos;
        }
        player_get_history(0, 0, &player, NULL);
        if (point_distance_squared(&player, &work->pos) < SHOW_RANGE) {
            work->light = tikal_light_create(task, &work->light, &work->pos);
            work->mode = TIKAL_RISE;
            work->submode = TIKAL_PLACED;
            work->scale = 0.0F;
            work->alpha = 0.0F;
            work->timer = RISE_STEPS;
            tikal_light_set(work->light, work->scale, work->alpha);
            task->step = tikal_step;
            task->draw = NULL;
            task->destructor = tikal_destroy;
            return;
        }
    }
    if (edit_mode) {
        marker_draw(&work->pos, MARKER_SIZE);
    }
}

struct task *tikal_hint_create(struct script_page *pages, Sint32 voice)
{
    struct task *task;
    struct tikal_work *work;

    /* @hack redundant: pos tested twice, as the image tests it (ADD #32,R2 at
     * 0x8c0638f8 and 0x8c06392a); tested once, 82 of 113 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (!WORK_SAME_PLACE(struct tikal_work, pos) ||
        !WORK_SAME_PLACE(struct tikal_work, id) ||
        !WORK_SAME_PLACE(struct tikal_work, collision) ||
        !WORK_SAME_PLACE(struct tikal_work, event) ||
        !WORK_SAME_PLACE(struct tikal_work, pos) ||
        !WORK_SAME_PLACE(struct tikal_work, flags)) {
        return NULL;
    }
    /* NOLINTEND(misc-redundant-expression) */
    /* @hack spelling: task assigned in the test, which tests task_create's R0
     * with the copy to R12 in the BT/S's slot (0x8c0639aa, 0x8c0639ae);
     * assigned apart, SHC tests R12 and 3 of 113 words differ */
    if ((task = task_create(TASK_WORK, 3, tikal_hint_step)) != NULL) {
        work = TIKAL(task);
        work->light = tikal_light_create(task, &work->light, &work->pos);
        work->mode = TIKAL_HINT_SHOW;
        work->submode = TIKAL_BOSS;
        player_get_history(0, 0, &work->pos, NULL);
        work->phase = 0;
        tikal_place(&work->pos, work->phase);
        work->scale = 0.0F;
        work->alpha = 0.0F;
        work->timer = HINT_STEPS;
        tikal_light_set(work->light, work->scale, work->alpha);
        work->hint.pages = pages;
        work->hint.voice = voice;
        work->hint.flag = -1;
        task->destructor = tikal_hint_destroy;
        tikal_hint_task = task;
        return task;
    }
    tikal_hint_task = NULL;
    return NULL;
}

void tikal_hint_end(void)
{
    struct tikal_work *work;

    if (tikal_hint_task != NULL) {
        work = TIKAL(tikal_hint_task);
        switch (work->mode) {
        case TIKAL_HINT_SHOW:
            work->mode = TIKAL_HINT_FADE;
            /* @hack spelling: a return where control returns anyway, which ends
             * the case with an RTS of its own (0x8c063a40); as a break, the
             * case branches to the function's RTS, message_cut's address
             * loads into R2, not R3, and 3 of 20 words differ */
            return;
        case TIKAL_HINT_CIRCLE:
            message_cut();
            break;
        }
    }
}

static void tikal_rise(struct task *task)
{
    struct tikal_work *work = TIKAL(task);

    if (placement_try_leave(task)) {
        return;
    }
    if (--work->timer == 0) {
        work->scale = FULL_SCALE;
        work->alpha = RISE_ALPHA;
        work->mode++;
    } else {
        work->scale += FULL_SCALE / RISE_STEPS;
        work->alpha += RISE_ALPHA / RISE_STEPS;
        work->pos.y += RISE_HEIGHT / RISE_STEPS;
    }
    tikal_light_set(work->light, work->scale, work->alpha);
}

/* once player 0 is within SPEAK_RANGE, stop the player, have it look at the
 * light, and show the hint. the hint waits on while the set editor holds
 * its task */
static void tikal_wait(struct task *task)
{
    struct tikal_work *work;
    NJS_POINT3 player;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        return;
    }
    work = TIKAL(task);
    player_get_history(0, 0, &player, NULL);
    if (point_distance_squared(&player, &work->pos) < SPEAK_RANGE) {
        work->scale = FULL_SCALE;
        work->alpha = FULL_ALPHA;
        tikal_light_set(work->light, work->scale, work->alpha);
        player_clear_speed(player_get_task(0)->motion, player_get(0));
        player_set_look_task(0, task, NULL);
        sound_play(SE_SPEAK, 0, 0, 0);
        message_set_shown(&work->shown);
        script_show(&work->hint);
        work->mode = TIKAL_CIRCLE;
    }
}

/* where the light is at phase round player 0, CIRCLE_RADIUS out. it bobs
 * from 2.5 to 7.5 above the player, four times for every three circles */
static void tikal_place(NJS_POINT3 *pos, Angle phase)
{
    Float sine;
    Float cosine;

    player_get_history(0, 0, pos, NULL);
    _builtin_fsca(phase, &sine, &cosine);
    pos->x += sine * CIRCLE_RADIUS;
    pos->z += cosine * CIRCLE_RADIUS;
    _builtin_fsca((Angle)(phase * (4.0F / 3.0F)), &sine, &cosine);
    pos->y += ((sine * 0.5F) + 1.0F) * 5.0F;
}

/* circle player 0 until the hint's message ends, then leave */
static void tikal_circle(struct task *task)
{
    struct tikal_work *work = TIKAL(task);
    NJS_POINT3 to;

    work->phase += CIRCLE_TURN;
    tikal_place(&to, work->phase);
    tikal_light_move_toward(&work->pos, &to, 1.0F);
    if (!work->shown) {
        player_clear_look(0);
        work->timer = LEAVE_STEPS;
        work->speed = 1.0F;
        work->mode++;
        sound_play_held(SE_LEAVE, (int)task, 1, 0);
    }
}

/* spiral off round player 0, faster and higher each step, with a flash
 * every other step. after LEAVE_STEPS, a placed hint rests and a boss's
 * hint ends */
static void tikal_leave(struct task *task)
{
    struct tikal_work *work = TIKAL(task);
    Float cosine;
    Float sine;
    NJS_POINT3 player;
    NJS_POINT3 to;

    player_get_history(0, 0, &player, NULL);
    to.x = player.x;
    to.z = player.z;
    to.y = work->pos.y;
    work->speed += LEAVE_ACCEL;
    /* the angle that a step of speed covers round a circle of
     * CIRCLE_RADIUS */
    work->phase +=
        (Angle)(360.0F / (2.0F * NJD_PI * CIRCLE_RADIUS / work->speed) *
                ANGLE_PER_DEGREE);
    _builtin_fsca(work->phase, &sine, &cosine);
    to.x += sine * CIRCLE_RADIUS;
    to.z += cosine * CIRCLE_RADIUS;
    to.y += LEAVE_CLIMB;
    tikal_light_move_toward(&work->pos, &to, work->speed);
    if (!(work->timer & 1)) {
        tikal_light_flash(work->light);
    }
    if (--work->timer == 0) {
        if (work->submode == TIKAL_PLACED) {
            sound_stop_id((int)task);
            work->mode++;
            task_destroy_children(task);
            work->timer = REST_STEPS;
        } else {
            task_destroy(task);
        }
    }
}

/* after REST_STEPS, go back to where it was placed and start over */
static void tikal_rest(struct task *task)
{
    struct tikal_work *work = TIKAL(task);

    if (work->timer <= 0) {
        if (task->extra != NULL) {
            work->pos = *(NJS_POINT3 *)task->extra;
        }
        tikal_init(task);
    } else {
        work->timer--;
    }
}

static void (*const tikal_modes[])(struct task *task) = {
    tikal_rise, tikal_wait, tikal_circle, tikal_leave, tikal_rest,
};

static void tikal_step(struct task *task)
{
    struct tikal_work *work = TIKAL(task);

    if (placement_editing(task)) {
        njPrintColor(0xffff0000);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "Don't EDIT!");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "Don't EDIT!");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "Don't EDIT!");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "Don't EDIT!");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "Don't EDIT!");
        njPrintColor(0xffc0c0c0);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<-MessageID");
    }
    task_step_children(task);
    tikal_modes[work->mode](task);
}

static void tikal_destroy(struct task *task)
{
    sound_stop_id((int)task);
}

static void tikal_hint_fade(struct task *task)
{
    struct tikal_work *work = TIKAL(task);
    NJS_POINT3 to;

    work->scale -= HINT_SCALE_STEP;
    work->alpha -= HINT_ALPHA_STEP;
    work->phase += CIRCLE_TURN;
    tikal_place(&to, work->phase);
    tikal_light_move_toward(&work->pos, &to, 1.0F);
    if (work->scale < 0.0F || work->alpha < 0.0F) {
        task_destroy(task);
    } else {
        tikal_light_set(work->light, work->scale, work->alpha);
    }
}

static void tikal_hint_show(struct task *task)
{
    struct tikal_work *work = TIKAL(task);
    NJS_POINT3 to;

    work->phase += CIRCLE_TURN;
    tikal_place(&to, work->phase);
    tikal_light_move_toward(&work->pos, &to, 1.0F);
    if (--work->timer == 0) {
        work->scale = FULL_SCALE;
        work->alpha = FULL_ALPHA;
        message_set_shown(&work->shown);
        script_show(&work->hint);
        work->mode++;
    } else {
        work->scale += HINT_SCALE_STEP;
        work->alpha += HINT_ALPHA_STEP;
    }
    tikal_light_set(work->light, work->scale, work->alpha);
}

static void (*const tikal_hint_modes[])(struct task *task) = {
    tikal_hint_show,
    tikal_circle,
    tikal_leave,
    tikal_hint_fade,
};

static void tikal_hint_step(struct task *task)
{
    struct tikal_work *work = TIKAL(task);

    task_step_children(task);
    tikal_hint_modes[work->mode](task);
}

static void tikal_hint_destroy(struct task *task)
{
    tikal_hint_task = NULL;
    sound_stop_id((int)task);
}
