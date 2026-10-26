/* the wheel's character preview: the character of the story at the top of the
 * wheel, which poses once the story is picked */
#include <ninja.h>
#include <sg_xpt.h>

#include "advertise/last_story.h"
#include "advertise/preview.h"
#include "advertise/textures.h"
#include "advertise/trial.h"
#include "advertise/wheel.h"
#include "draw/light.h"
#include "draw/model.h"
#include "draw/screen.h"
#include "draw/tree.h"
#include "event/play.h"
#include "object/common.h"
#include "player/amy.h"
#include "player/big.h"
#include "player/big_models.h"
#include "player/e102.h"
#include "player/knuckles.h"
#include "player/tails.h"
#include "player/player.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the speed of every character's actions */
#define PREVIEW_SPEED 0.25F
/* the preview keeps every character off screen for its first HIDDEN_STEPS
 * steps */
#define HIDDEN_STEPS 2

/* a story's character: its player step, its textures, and its actions with
 * their blends. the first action loops until the story is picked. the rest then
 * play in turn, and the last of them loops */
struct preview_character {
    void (*step)(struct task *task);
    NJS_TEXLIST *texlist;
    NJS_ACTION **actions; /* ended by NULL */
    const int *blends; /* each action's blend from the one before */
};

struct preview {
    struct task *characters[STORIES];
    struct task *camera;
    Uint8 unk_20[12];
    Sint8 steps; /* its steps so far, up to HIDDEN_STEPS */
    /* the index of each character's next action to queue: 0 before its loop,
     * and 1 while it loops. once the character is picked and the rest are
     * queued, it is the index of the NULL that ends its actions */
    Sint8 actions[STORIES];
    /* nonzero from preview_set_chosen until the top story's character plays
     * its other actions */
    Uint8 chosen;
};

extern NJS_ACTION sonic_run_action;
extern NJS_ACTION tails_fly_action;
extern NJS_ACTION tails_clear_action;
extern NJS_ACTION knuckles_preview_action;
extern NJS_ACTION amy_walk_action;
extern NJS_ACTION amy_clear_action;
extern NJS_ACTION e102_slide_stand_action;
extern NJS_ACTION sonic_chosen_action;
extern NJS_ACTION sonic_chosen_loop_action;
extern NJS_ACTION tails_chosen_loop_action;
extern NJS_ACTION knuckles_chosen_action;
extern NJS_ACTION knuckles_chosen_loop_action;
extern NJS_ACTION amy_chosen_loop_action;
extern NJS_ACTION big_chosen_action;
extern NJS_ACTION big_chosen_action_2;
extern NJS_ACTION big_chosen_loop_action;
extern NJS_ACTION e102_chosen_action;
extern NJS_ACTION e102_chosen_loop_action;

static void last_story_step(struct task *task);

static const int sonic_blends[] = {0, 0, 8};
static const int tails_blends[] = {0, 0, 0};
static const int knuckles_blends[] = {0, 0, 8};
static const int amy_blends[] = {0, 0, 0};
static const int big_blends[] = {0, 0, 8, 8};
static const int e102_blends[] = {0, 0, 8};

/* the aim preview_stand_by gives Sonic, which his step turns him toward, so
 * that he faces the camera as the others do */
static Angle sonic_aim = NJM_DEG_ANG(90);
static NJS_ACTION *preview_sonic_actions[] = {
    &sonic_run_action,
    &sonic_chosen_action,
    &sonic_chosen_loop_action,
    NULL,
};
static NJS_ACTION *preview_tails_actions[] = {
    &tails_fly_action,
    &tails_clear_action,
    &tails_chosen_loop_action,
    NULL,
};
static NJS_ACTION *preview_knuckles_actions[] = {
    &knuckles_preview_action,
    &knuckles_chosen_action,
    &knuckles_chosen_loop_action,
    NULL,
};
static NJS_ACTION *preview_amy_actions[] = {
    &amy_walk_action,
    &amy_clear_action,
    &amy_chosen_loop_action,
    NULL,
};
static NJS_ACTION *preview_big_actions[] = {
    &big_run_action,
    &big_chosen_action,
    &big_chosen_action_2,
    &big_chosen_loop_action,
    NULL,
};
static NJS_ACTION *preview_e102_actions[] = {
    &e102_slide_stand_action,
    &e102_chosen_action,
    &e102_chosen_loop_action,
    NULL,
};
static const struct preview_character preview_characters[STORIES] = {
    {sonic_step, &sonic_texlist, preview_sonic_actions, sonic_blends},
    {tails_step, &tails_texlist, preview_tails_actions, tails_blends},
    {knuckles_step, &knuckles_texlist, preview_knuckles_actions,
     knuckles_blends},
    {amy_step, &amy_texlist, preview_amy_actions, amy_blends},
    {big_step, &big_texlist, preview_big_actions, big_blends},
    {e102_step, &e102_texlist, preview_e102_actions, e102_blends},
    {last_story_step, NULL, NULL, NULL},
};
/* where each story's character stands while the story is at the top of the
 * wheel */
static const NJS_POINT3 preview_places[STORIES] = {
#include "advertise/preview/preview_places.inc"
};

static struct texture_pvm preview_sonic_pvms[] = {{"SONIC", &sonic_texlist},
                                                  {0}};
static struct texture_pvm preview_tails_pvms[] = {{"MILES", &tails_texlist},
                                                  {0}};
static struct texture_pvm preview_knuckles_pvms[] = {
    {"KNUCKLES", &knuckles_texlist},
    {0},
};
static struct texture_pvm preview_amy_pvms[] = {{"AMY", &amy_texlist}, {0}};
static struct texture_pvm preview_big_pvms[] = {{"BIG", &big_texlist}, {0}};
static struct texture_pvm preview_e102_pvms[] = {{"E102", &e102_texlist}, {0}};
/* each character's PVMs, indexed by the character's number, not its story.
 * Eggman and Tikal have none */
static struct texture_pvm *preview_character_pvms[] = {
    preview_sonic_pvms,
    NULL,
    preview_tails_pvms,
    preview_knuckles_pvms,
    NULL,
    preview_amy_pvms,
    preview_e102_pvms,
    preview_big_pvms,
};

static struct task *preview_task;
/* preview_places, as preview_create copies them */
static NJS_POINT3 preview_spots[STORIES];

/* load the character's PVMs if nothing uses them yet, and count one more user
 * in character_users */
static void preview_load_textures(int character)
{
    struct texture_pvm *pvm = preview_character_pvms[character];

    if (pvm == NULL) {
        return;
    }
    if (character_users[character] == 0) {
        for (; pvm->texlist; pvm++) {
            if (pvm->name == NULL) {
                njLoadTexture(pvm->texlist);
            } else {
                texture_load_pvm(pvm->name, pvm->texlist);
            }
        }
    }
    character_users[character]++;
}

/* count one user fewer of the character's PVMs, and release them if it was the
 * last */
static void preview_release_textures(int character)
{
    struct texture_pvm *pvm = preview_character_pvms[character];

    if (pvm == NULL) {
        return;
    }
    if (character_users[character] == 1) {
        for (; pvm->texlist; pvm++) {
            njReleaseTexture(pvm->texlist);
        }
    }
    if (character_users[character] > 0) {
        character_users[character]--;
    }
}

/* the last story's step, which turns last_story_object and draws it */
static void last_story_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    work->ang.y += NJM_DEG_ANG(4);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, -work->ang.y, ang);
    njSetTexture(&ava_chsel_texlist);
    draw_tree(&last_story_object);
    njPopMatrix(1);
}

/* put every character off screen, facing the camera */
static void preview_stand_by(struct preview *preview)
{
    int i;
    struct task *task;
    struct task_work *work;

    for (i = 0; i < STORIES; i++) {
        task = preview->characters[i];
        work = task->work;
        work->pos.x = 400.0F;
        work->pos.y = 0.0F;
        work->pos.z = -20.0F;
        work->ang.x = 0;
        work->ang.z = 0;
        if (i != STORY_LAST) {
            work->ang.y = NJM_DEG_ANG(90);
        }
        if (i == STORY_SONIC) {
            task->motion->aim.y = sonic_aim;
        }
    }
}

static void preview_show(struct preview *preview)
{
    struct task *task;
    struct task_work *work;

    preview_stand_by(preview);
    task = preview->characters[menu_state.story];
    work = task->work;
    work->pos.x = preview_spots[menu_state.story].x;
    work->pos.y = preview_spots[menu_state.story].y;
    work->pos.z = preview_spots[menu_state.story].z;
}

static void preview_create_characters(struct preview *preview)
{
    int i;
    struct task *task;
    struct task_work *work;

    for (i = 0; i < STORIES; i++) {
        preview->characters[i] = NULL;
        preview->actions[i] = 0;
        preview_spots[i].x = preview_places[i].x;
        preview_spots[i].y = preview_places[i].y;
        preview_spots[i].z = preview_places[i].z;
        if (i != STORY_LAST) {
            preview_load_textures(story_characters[i]);
        }
        task = preview->characters[i] =
            task_create(TASK_MOTION | TASK_WORK | TASK_FORCE, 1,
                        preview_characters[i].step);
        if (task != NULL) {
            work = task->work;
            work->ang.y = NJM_DEG_ANG(90);
            if (i == STORY_E102) {
                work->id = 2;
            }
            /* @hack spelling: the player's variables reached through
             * task->work, read again for each store (MOV.L @(32,R4),R3 at
             * 0x8c9028be and 0x8c9028c4). through work, SHC allocates the
             * function's registers otherwise from its first instruction on, and
             * 339 of 396 words differ */
            PLAYER_VARS(task->work)->character = story_characters[i];
            PLAYER_VARS(task->work)->player = i;
        }
    }
    preview_stand_by(preview);
    njSetPerspective(NJM_DEG_ANG(70));
    draw_model_init();
}

static void preview_animate(struct preview *preview)
{
    int i;
    struct task *task;
    const struct preview_character *character;
    Float speed;
    Sint8 action;

    /* @hack spelling: the loop as a do-while. SHC then enters its first pass
     * directly, at 0x8c902bb4. as a for loop, SHC branches to the test there
     * first, the function runs 2 bytes longer, and 64 of 145 words differ */
    i = 0;
    do {
        task = preview->characters[i];
        if (task == NULL) {
            continue;
        }
        if (i == STORY_LAST) {
            continue;
        }
        character = &preview_characters[i];
        player_set_mode(i, PLAYER_MODE_STILL);
        if (i == STORY_TAILS) {
            player_set_mode(i, PLAYER_MODE_HOVER);
        }
        /* @hack redundant: the speed chosen by a select whose arms are both
         * PREVIEW_SPEED. SHC still tests for Amy and E-102 (CMP/EQ #3,R0 at
         * 0x8c902bf4, CMP/EQ #5,R0 at 0x8c902bfa), and loads the speed into
         * FR15 in each pass (0x8c902c0a). with PREVIEW_SPEED alone, SHC loads
         * it once ahead of the loop, and 92 of 145 words differ */
        /* NOLINTBEGIN(misc-redundant-expression) */
        speed =
            (i == STORY_AMY || i == STORY_E102) ? PREVIEW_SPEED : PREVIEW_SPEED;
        /* NOLINTEND(misc-redundant-expression) */
        if (preview->actions[i] == 0) {
            event_play_action(task, character->actions[0], character->texlist,
                              speed, 1, 0);
            preview->actions[i] = 1;
        } else if (preview->actions[i] == 1 && preview->chosen != 0 &&
                   menu_state.story == i) {
            while (character->actions[action = preview->actions[i]] != NULL) {
                if (character->actions[++preview->actions[i]] != NULL) {
                    event_play_action(task, character->actions[action],
                                      character->texlist, speed, 0,
                                      character->blends[action]);
                } else {
                    event_play_action(task, character->actions[action],
                                      character->texlist, speed, 1,
                                      character->blends[action]);
                }
            }
            preview->chosen = 0;
        }
    } while (++i < STORIES);
}

static void preview_release(struct preview *preview)
{
    int i;

    for (i = 0; i < STORIES; i++) {
        if (preview->characters[i] != NULL) {
            task_destroy(preview->characters[i]);
        }
        if (i != STORY_LAST) {
            preview_release_textures(story_characters[i]);
        }
    }
}

static void preview_init(struct task *task)
{
    struct preview *preview = task->extra;
    struct task_work *work;

    screen_palettes_grays();
    light_read_disc_sources(STAGE_SKY_DECK, 0);
    light_read_disc_palettes(STAGE_SKY_DECK, 0);
    draw_set_palette(DRAW_PLAYER_PALETTE);
    draw_set_light(0.577F, 0.577F, 0.577F);
    /* @hack spelling: the camera assigned in the test. SHC then tests it in R0
     * (TST R0,R0 at 0x8c902e7e), and stores it in the slot of the BT/S.
     * assigned first, it is copied to R3 and tested there, the function runs
     * 2 bytes longer, and 30 of 57 words differ */
    if ((preview->camera =
             task_create(TASK_WORK | TASK_FORCE, 0, screen_view_from_task))) {
        work = preview->camera->work;
        work->ang.x = work->ang.y = work->ang.z = 0;
        work->pos.x = work->pos.y = work->pos.z = 0.0F;
    }
    preview_create_characters(preview);
    preview->steps = 0;
    preview->chosen = 0;
}

static void preview_step(struct task *task)
{
    struct preview *preview = task->extra;

    if (preview->steps < HIDDEN_STEPS) {
        preview_stand_by(preview);
        preview->steps++;
    } else {
        preview_show(preview);
    }
}

static void preview_draw(struct task *task)
{
    struct preview *preview = task->extra;

    njSetPerspective(NJM_DEG_ANG(70));
    draw_model_init();
    if (preview->steps >= HIDDEN_STEPS) {
        preview_animate(preview);
    }
}

static void preview_destroy(struct task *task)
{
    struct preview *preview = task->extra;

    if (preview->camera != NULL) {
        task_destroy(preview->camera);
    }
    preview_release(preview);
}

struct task *preview_create(void)
{
    struct task *task;
    struct preview *preview;

    /* @hack spelling: the task assigned in the test. SHC then tests it in R0
     * (TST R0,R0 at 0x8c902f56), and copies it to R14 after. assigned first,
     * it is copied to R14 and tested there, and 2 of 30 words differ */
    if ((preview_task = task = task_create(0, 3, preview_step)) != NULL) {
        preview = heap_calloc(1, sizeof(struct preview));
        task->extra = preview;
        preview_init(task);
        task->step = preview_step;
        task->draw = preview_draw;
        task->destructor = preview_destroy;
    }
    return task;
}

void preview_kill(void)
{
    if (preview_task != NULL) {
        task_destroy(preview_task);
    }
}

void preview_set_chosen(int chosen)
{
    ((struct preview *)preview_task->extra)->chosen = chosen;
}

void preview_reset_pose(void)
{
    struct preview *preview = preview_task->extra;

    preview->chosen = 0;
    preview->actions[menu_state.story] = 0;
}
