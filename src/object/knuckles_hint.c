/* Knuckles' hint, O KN HINT: Tikal's light, which glows brighter as
 * Knuckles nears it. once he reaches it, it calls him and leads him toward
 * the nearest piece of the master emerald */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "event/script.h"
#include "object/common.h"
#include "object/knuckles_hint.h"
#include "object/placement.h"
#include "object/tikal_light.h"
#include "player/knuckles.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/math.h"
#include "system/task.h"

#define TEXT_1_LENGTH 11
#include "object/knuckles_hint/text.inc"

/* squared distances from the light to the point 5 above player 0. within
 * GLOW_RANGE the light brightens, and beyond it the light dims. within
 * CALL_RANGE it calls him */
#define GLOW_RANGE (100.0F * 100.0F)
#define CALL_RANGE (8.0F * 8.0F)

/* the steps the light circles once it has called, and then the steps it
 * leads */
#define CALL_STEPS 102
#define LEAD_STEPS 180

/* each step, the light gains or loses ALPHA_STEP of alpha. as it circles it
 * turns CIRCLE_TURN and flies CIRCLE_SPEED, and as it leads it flies
 * LEAD_SPEED */
#define ALPHA_STEP 0.03F
#define CIRCLE_TURN NJM_DEG_ANG(22.5)
#define CIRCLE_SPEED 2.0F
#define LEAD_SPEED 3.2F

/* the sound the light plays as it starts to lead, and the voice of its call */
#define SE_LEAD 747
#define VOICE_CALL 840

/* the hint's view of its work. it keeps the light's alpha and scale where
 * task_work keeps scl.x and scl.y */
struct knuckles_hint_work {
    Sint8 mode;
    Sint8 submode;
    Uint8 id;
    Uint8 tick;
    Sint16 flags;
    Sint16 timer;
    struct task *light;
    Angle phase; /* the light's heading round its circle */
    Uint8 unk_10[4];
    struct angles ang;
    NJS_POINT3 pos;
    Float alpha;
    Float scale;
    Uint8 unk_34[4];
    struct collision_work *collision;
    struct event_work *event;
};

#define HINT(task) ((struct knuckles_hint_work *)(task)->work)

enum knuckles_hint_mode {
    KNUCKLES_HINT_WAIT, /* until player 0 comes within CALL_RANGE */
    KNUCKLES_HINT_CALL, /* for CALL_STEPS */
    KNUCKLES_HINT_LEAD /* for LEAD_STEPS, or until no piece is left */
};

static void knuckles_hint_step(struct task *task);
static void knuckles_hint_destroy(struct task *task);

/* const places the call among the constants, where the image has it.
 * struct script_pages and script_show take no const, so each use casts it
 * away */

/* the call: one page, which shows for 60 steps */
static const struct script_page knuckles_hint_pages[] = {
    {TEXT_1, 60},
    {NULL, 0},
};

/* the call in VOICE_CALL, with no flag to set or clear */
static const struct script_pages knuckles_hint_pick = {
    (struct script_page *)knuckles_hint_pages, VOICE_CALL, -1};

void knuckles_hint_init(struct task *task)
{
    struct knuckles_hint_work *work = HINT(task);

    /* the hint starts only while knuckles_hint_work keeps task_work's fields
     * where task_work does */
    /* @hack redundant: pos tested twice, as the image tests it (ADD #32 at
     * 0x8c064e2e and 0x8c064e62); tested once, knuckles_hint.c's code runs
     * 32 bytes short and 59 of 89 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (!WORK_SAME_PLACE(struct knuckles_hint_work, pos) ||
        !WORK_SAME_PLACE(struct knuckles_hint_work, id) ||
        !WORK_SAME_PLACE(struct knuckles_hint_work, collision) ||
        !WORK_SAME_PLACE(struct knuckles_hint_work, event) ||
        !WORK_SAME_PLACE(struct knuckles_hint_work, pos) ||
        !WORK_SAME_PLACE(struct knuckles_hint_work, flags)) {
        return;
    }
    /* NOLINTEND(misc-redundant-expression) */
    /* @bug logic: the kill is undone, and the hint runs on for any
     * character. placement_kill's JSR at 0x8c064e8c sets task_destroy as
     * the step, and the store of knuckles_hint_step below replaces it
     * (MOV.L R2,@(16,R12) at 0x8c064ec2) */
    if (!placement_editing(task) && get_character() != CHARACTER_KNUCKLES) {
        placement_kill(task);
    }
    work->light = tikal_light_create(task, &work->light, &work->pos);
    work->mode = KNUCKLES_HINT_WAIT;
    work->alpha = 0.0F;
    work->scale = 4.0F;
    tikal_light_set(work->light, work->scale, work->alpha);
    task->step = knuckles_hint_step;
    task->draw = NULL;
    task->destructor = knuckles_hint_destroy;
}

/* the light brightens while player 0 is within GLOW_RANGE, up to an alpha
 * of 1, and dims beyond it, down to 0. once player 0 comes within
 * CALL_RANGE outside the set editor, the hint adds one to hints_used, shows
 * the call and circles. out of range, the hint leaves */
static void knuckles_hint_wait(struct task *task)
{
    struct knuckles_hint_work *work = HINT(task);
    NJS_POINT3 player;
    Float distance;

    player_get_history(0, 0, &player, NULL);
    player.y += 5.0F;
    distance = point_distance_squared(&player, &work->pos);
    if (distance > GLOW_RANGE) {
        work->alpha -= ALPHA_STEP;
        if (work->alpha < 0.0F) {
            work->alpha = 0.0F;
        }
    } else {
        work->alpha += ALPHA_STEP;
        if (work->alpha > 1.0F) {
            work->alpha = 1.0F;
        }
    }
    if (!placement_editing(task) && distance < CALL_RANGE) {
        hints_used++;
        script_show((struct script_pages *)&knuckles_hint_pick);
        work->timer = CALL_STEPS;
        work->mode++;
    }
    tikal_light_set(work->light, work->scale, work->alpha);
    /* @hack redundant: a return where the function ends, which keeps the
     * image's TST R0,R0 of the result (0x8c064f76); without it, 7 of 89 words
     * differ */
    if (placement_try_leave(task)) {
        return;
    }
}

/* circle for CALL_STEPS, then play SE_LEAD, turn player 0 to look at the
 * hint, and lead */
static void knuckles_hint_call(struct task *task)
{
    struct knuckles_hint_work *work = HINT(task);
    Float sine;
    Float cosine;

    work->phase += CIRCLE_TURN;
    _builtin_fsca(work->phase, &sine, &cosine);
    work->pos.x += sine * CIRCLE_SPEED;
    work->pos.z += cosine * CIRCLE_SPEED;
    if (--work->timer == 0) {
        work->mode++;
        work->timer = LEAD_STEPS;
        sound_play(SE_LEAD, 0, 0, 0);
        player_set_look_task(0, task, NULL);
    }
}

/* fly toward the nearest piece found and not taken, leaving a flash each
 * step, for LEAD_STEPS. then clear player 0's look and end. with no such
 * piece left, end at once, and leave player 0 looking at the hint */
static void knuckles_hint_lead(struct task *task)
{
    struct knuckles_hint_work *work = HINT(task);
    NJS_POINT3 piece;
    Float distance;

    tikal_light_flash(work->light);
    if (knuckles_nearest_piece(&work->pos, &piece, &distance)) {
        tikal_light_move_toward(&work->pos, &piece, LEAD_SPEED);
        if (--work->timer == 0) {
            player_clear_look(0);
            placement_kill(task);
        }
    } else {
        placement_kill(task);
    }
}

static void (*const knuckles_hint_modes[])(struct task *task) = {
    knuckles_hint_wait,
    knuckles_hint_call,
    knuckles_hint_lead,
};

static void knuckles_hint_step(struct task *task)
{
    struct knuckles_hint_work *work = HINT(task);

    task_step_children(task);
    knuckles_hint_modes[work->mode](task);
}

static void knuckles_hint_destroy(struct task *task)
{
}
