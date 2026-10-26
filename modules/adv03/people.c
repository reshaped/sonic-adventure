/* the past's people: Tikal, Pachacamac, Chao and echidnas. past_people_place
 * places those whose rows' scripts pass, and each plays its action round and
 * round */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv03/people.h"
#include "adv03/shadow.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "event/play.h"
#include "event/script.h"
#include "object/common.h"
#include "player/player.h"
#include "player/talk.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "tikal_prog/tikal.h"

/* Pachacamac's, the Chao's and the echidnas' texture lists, which past_pvms
 * loads with the stage */
extern NJS_TEXLIST k_patya_texlist;
extern NJS_TEXLIST ev_alife_texlist;
extern NJS_TEXLIST past_kn_fam_texlist;
/* the kinds' actions. dat_0cb2d478 lies in TIKAL_PROG.PRS, as does
 * tikal_texlist, which both Tikal kinds draw with. TIKAL_PROG.PRS must stay
 * loaded while a person of either kind draws */
extern NJS_ACTION patya_action;
extern NJS_ACTION dat_0c9e9ab8;
extern NJS_ACTION dat_0c9eaabc;
extern NJS_ACTION dat_0c9eb8a0;
extern NJS_ACTION dat_0cb2d478;
extern NJS_ACTION dat_0ca00ab8;
extern NJS_ACTION past_kn_fam_actions[3];
/* the node of a Chao's model that person_face_camera turns to the camera */
extern NJS_OBJECT dat_0c9ddb2c;

/* the most people past_people_place places at once */
#define PEOPLE_MAX 12

/* a kind's character, whose textures person_start loads and person_destroy
 * releases. a kind whose texture list past_pvms loads with the stage has
 * NO_CHARACTER instead, or NO_CHARACTER_CHAO for a Chao. person_draw turns a
 * node of a Chao's model to the camera */
#define NO_CHARACTER (-1)
#define NO_CHARACTER_CHAO (-2)

/* the indexes of people_kinds. each Chao, Tikal and echidna kind plays an
 * action of its own */
enum person_kind_index {
    PERSON_PACHACAMAC,
    PERSON_CHAO_A,
    PERSON_CHAO_B,
    PERSON_CHAO_C,
    PERSON_TIKAL_A,
    PERSON_TIKAL_B,
    PERSON_ECHIDNA_A,
    PERSON_ECHIDNA_B,
    PERSON_ECHIDNA_C,
    PERSON_KINDS
};

/* a person's talk reaches TALK_REACH past its shape's radius, and talk_add
 * adds player 0's radius to that */
#define TALK_REACH 20.0F

/* the radius draw_in_view tests a person's position by */
#define VIEW_RADIUS 15.0F

/* a kind of person: how it looks, its shape, and where player 0 looks during
 * a talk */
struct person_kind {
    int character;
    NJS_TEXLIST *texlist;
    NJS_ACTION *action;
    Float lift; /* how far above pos the model is drawn */
    Float eye; /* how far above pos player 0 looks during a talk */
    struct collision_shape shape;
};

/* where an act places a person, with the person's row and kind */
struct person_spot {
    NJS_POINT3 pos;
    Sint16 ang_x;
    Sint16 ang_y;
    Sint16 ang_z;
    Sint8 row; /* the person's row of past_script_table */
    Sint8 kind; /* a person_kind_index */
};

struct person_list {
    const struct person_spot *spots;
    int count;
};

/* a person's task work, which the task holds in place of a task_work. each
 * member the two share lies at the same offset in both */
struct person_work {
    Sint8 mode;
    Sint8 submode;
    Uint8 id;
    Uint8 shown; /* 1 while the next message started after a talk shows */
    Sint16 flags;
    Sint16 timer;
    Float frame; /* the action's frame */
    /* TRUE from the start of player 0's talk with the person until
     * person_step answers it */
    Bool talking;
    int kind; /* a person_kind_index */
    struct angles ang;
    NJS_POINT3 pos;
    Uint8 unk_2c[4];
    struct script_pages *pick; /* what the person's row last picked */
    int row; /* in past_script_table */
    struct collision_work *collision;
    struct event_work *event;
};

#define PERSON(task) ((struct person_work *)(task)->work)

/* a person's shape, a capsule whose center lies 7 above the person's
 * position, and a Chao's, a ball. the person's angles turn neither. each
 * pushes players and objects, and players can't push it. a player it pushes
 * back doesn't start to push against it */
/* clang-format off */
#define PERSON_SHAPE                                                           \
    {SHAPE_CAPSULE | SHAPE_NO_PUSH | SHAPE_UNTURNED, 0,                        \
     PUSH_PLAYERS | PUSH_OBJECTS | PUSH_FIRM(PUSH_PLAYERS),                    \
     {0.0F, 7.0F, 0.0F}, 4.0F, 5.0F, 0.0F, {0}, 0, NULL}
#define CHAO_SHAPE                                                             \
    {SHAPE_SPHERE | SHAPE_NO_PUSH | SHAPE_UNTURNED, 0,                         \
     PUSH_PLAYERS | PUSH_OBJECTS | PUSH_FIRM(PUSH_PLAYERS),                    \
     {0.0F, 1.6F, 0.0F}, 2.0F, 0.0F, 0.0F, {0}, 0, NULL}
/* clang-format on */

static void person_init(struct task *task);
static void person_start(struct task *task);
static void person_animate(struct task *task);
static void person_listen(struct task *task);
static void person_step(struct task *task);
static void person_face_camera(NJS_OBJECT *object);
static void person_draw(struct task *task);
static void person_destroy(struct task *task);

/* the kinds, by person_kind_index */
static const struct person_kind people_kinds[PERSON_KINDS] = {
    {NO_CHARACTER, &k_patya_texlist, &patya_action, 7.0F, 7.0F, PERSON_SHAPE},
    {NO_CHARACTER_CHAO, &ev_alife_texlist, &dat_0c9e9ab8, 1.4F, 1.4F,
     CHAO_SHAPE},
    {NO_CHARACTER_CHAO, &ev_alife_texlist, &dat_0c9eaabc, 1.2F, 1.2F,
     CHAO_SHAPE},
    {NO_CHARACTER_CHAO, &ev_alife_texlist, &dat_0c9eb8a0, 0.8F, 0.8F,
     CHAO_SHAPE},
    {CHARACTER_TIKAL, &tikal_texlist, &dat_0cb2d478, 5.4F, 7.0F, PERSON_SHAPE},
    {CHARACTER_TIKAL, &tikal_texlist, &dat_0ca00ab8, 5.4F, 7.0F, PERSON_SHAPE},
    {NO_CHARACTER, &past_kn_fam_texlist, &past_kn_fam_actions[0], 7.0F, 8.0F,
     PERSON_SHAPE},
    {NO_CHARACTER, &past_kn_fam_texlist, &past_kn_fam_actions[1], 7.0F, 9.0F,
     PERSON_SHAPE},
    {NO_CHARACTER, &past_kn_fam_texlist, &past_kn_fam_actions[2], 7.4F, 9.5F,
     PERSON_SHAPE},
};

static const struct person_spot people_act0[3] = {
    {{17.69F, 0.0F, 480.85F}, 0, -30606, 0, 0, PERSON_TIKAL_A},
    {{5.138F, 0.0F, 1212.25F}, 0, -14336, 0, 1, PERSON_TIKAL_A},
    {{-4.862F, 7.0F, 1222.25F}, 0, 14336, 0, 1, PERSON_PACHACAMAC},
};
static const struct person_spot people_act1[8] = {
    {{-80.0F, 0.0F, 369.5F}, 0, -16640, 0, 2, PERSON_TIKAL_A},
    {{0.4F, 90.0F, 48.03F}, 0, 29118, 0, 3, PERSON_TIKAL_A},
    {{-2.74F, 7.51F, 254.65F}, 0, 29118, 0, 4, PERSON_TIKAL_A},
    {{0.0F, 3.0F, 351.0F}, 0, 0, 0, 5, PERSON_CHAO_A},
    {{-14.0F, 1.5F, 360.0F}, 0, -23296, 0, 5, PERSON_CHAO_B},
    {{14.0F, 1.5F, 360.0F}, 0, 25856, 0, 5, PERSON_CHAO_C},
    {{-5.0F, 1.5F, 370.0F}, 0, -32000, 0, 5, PERSON_CHAO_B},
    {{5.0F, 1.5F, 370.0F}, 0, 32000, 0, 5, PERSON_CHAO_C},
};
static const struct person_spot people_act2[13] = {
    {{-15.8F, -4.2F, 434.4F}, 1264, 19456, 0, 6, PERSON_TIKAL_B},
    {{-28.0F, -3.0F, 431.0F}, 0, 12288, 0, 6, PERSON_CHAO_A},
    {{-15.0F, -2.0F, 426.0F}, 0, -8192, 0, 6, PERSON_CHAO_B},
    {{-27.0F, -4.0F, 440.0F}, 0, 29440, 0, 6, PERSON_CHAO_C},
    {{0.0F, 0.0F, 381.0F}, 0, 0, 0, 7, PERSON_TIKAL_A},
    {{0.0F, 0.0F, 401.0F}, 0, -16384, 0, 7, PERSON_PACHACAMAC},
    {{-4.1F, 0.0F, 410.5F}, 0, -15104, 0, 7, PERSON_ECHIDNA_A},
    {{4.8F, -1.0F, 408.74F}, 0, -16128, 0, 7, PERSON_ECHIDNA_B},
    {{-15.8F, -4.2F, 434.4F}, 1264, 19456, 0, 8, PERSON_TIKAL_B},
    {{-28.0F, -3.0F, 431.0F}, 0, 12288, 0, 8, PERSON_CHAO_A},
    {{-15.0F, -2.0F, 426.0F}, 0, -8192, 0, 8, PERSON_CHAO_B},
    {{-27.0F, -4.0F, 440.0F}, 0, 29440, 0, 8, PERSON_CHAO_C},
    {{0.0F, 90.0F, 39.0F}, 0, -32768, 0, 9, PERSON_TIKAL_A},
};

/* clang-format off */
/* row 0: Tikal */
static const Sint16 people_script_0[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_TAILS_RHYTHM_BROOCH,
    SCRIPT_EVENTS, 69,
    SCRIPT_END,
};
/* row 1: Tikal and Pachacamac */
static const Sint16 people_script_1[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_EVENTS, SCRIPT_ON | 131,
    SCRIPT_EVENTS, 133,
    SCRIPT_END,
};
/* rows 2 to 4: Tikal */
static const Sint16 people_script_2[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_EVENTS, SCRIPT_ON | 102,
    SCRIPT_EVENTS, 104,
    SCRIPT_END,
};
static const Sint16 people_script_3[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_EVENTS, SCRIPT_ON | 141,
    SCRIPT_EVENTS, 143,
    SCRIPT_END,
};
static const Sint16 people_script_4[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_EVENTS, SCRIPT_ON | 218,
    SCRIPT_EVENTS, 220,
    SCRIPT_END,
};
/* row 5: five Chao */
static const Sint16 people_script_5[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_EVENTS, SCRIPT_ON | 184,
    SCRIPT_EVENTS, 186,
    SCRIPT_END,
};
/* row 6: Tikal and three Chao */
static const Sint16 people_script_6[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_EVENTS, SCRIPT_ON | 32,
    SCRIPT_EVENTS, 34,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_EVENTS, SCRIPT_ON | 149,
    SCRIPT_EVENTS, 151,
    SCRIPT_END,
};
/* row 7: Tikal, Pachacamac and two echidnas */
static const Sint16 people_script_7[] = {
    SCRIPT_EVENTS, SCRIPT_ON | 245,
    SCRIPT_EVENTS, 247,
    SCRIPT_END,
};
/* row 8: Tikal and three Chao */
static const Sint16 people_script_8[] = {
    SCRIPT_EVENTS, SCRIPT_ON | 247,
    SCRIPT_EVENTS, 251,
    SCRIPT_END,
};
/* row 9: Tikal */
static const Sint16 people_script_9[] = {
    SCRIPT_EVENTS, SCRIPT_ON | 251,
    SCRIPT_EVENTS, 248,
    SCRIPT_END,
};
/* clang-format on */

static const struct person_list people_lists[3] = {
    {people_act0, 3},
    {people_act1, 8},
    {people_act2, 13},
};

/* each row's script decides whether past_people_place places the row's
 * people. the rows hold no runs, so script_pick picks no pages for them, and
 * a talk with one of the people shows no message */
const struct script_row past_script_table[] = {
    {(Sint16 *)people_script_0, NULL}, {(Sint16 *)people_script_1, NULL},
    {(Sint16 *)people_script_2, NULL}, {(Sint16 *)people_script_3, NULL},
    {(Sint16 *)people_script_4, NULL}, {(Sint16 *)people_script_5, NULL},
    {(Sint16 *)people_script_6, NULL}, {(Sint16 *)people_script_7, NULL},
    {(Sint16 *)people_script_8, NULL}, {(Sint16 *)people_script_9, NULL},
};

/* the people placed, people_count of them, in the order of placement */
static int people_count = 0;
static struct task *people[PEOPLE_MAX];

void past_people_place(void)
{
    const struct person_list *list;
    const struct person_spot *spot;
    struct script_pages *pick;
    struct task *task;
    struct person_work *work;
    int left;
    int row;

    people_count = 0;
    list = &people_lists[(Uint8)stage_get_stage_act()];
    spot = list->spots;
    left = list->count;
    if (spot == NULL) {
        return;
    }
    pick = heap_alloc(sizeof(struct script_pages));
    do {
        if (pick != NULL) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            row = spot->row;
            if (script_pick(row, pick)) {
                task = task_create(TASK_WORK, 3, person_init);
                work = PERSON(task);
                work->pick = pick;
                pick = heap_alloc(sizeof(struct script_pages));
                work->row = row;
                work->pos = spot->pos;
                work->ang.x = spot->ang_x;
                work->ang.y = spot->ang_y;
                work->ang.z = spot->ang_z;
                /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
                work->kind = spot->kind;
                people[people_count] = task;
                people_count++;
            }
        }
        spot++;
    } while (--left != 0 && people_count < PEOPLE_MAX);
    heap_free(pick);
}

void past_people_remove(void)
{
    while (people_count != 0) {
        people_count--;
        if (people[people_count] != NULL) {
            task_destroy(people[people_count]);
            people[people_count] = NULL;
        }
    }
}

/* start the person, and give it its step, draw and destructor. the
 * WORK_SAME_PLACE tests always pass */
static void person_init(struct task *task)
{
    /* @hack redundant: pos tested a second time (ADD #32,R2 at 0x0c904d26 and
     * 0x0c904d56); tested once, 24 of 51 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (WORK_SAME_PLACE(struct person_work, pos) &&
        WORK_SAME_PLACE(struct person_work, id) &&
        WORK_SAME_PLACE(struct person_work, collision) &&
        WORK_SAME_PLACE(struct person_work, event) &&
        WORK_SAME_PLACE(struct person_work, pos) &&
        WORK_SAME_PLACE(struct person_work, flags)) {
        person_start(task);
        task->step = person_step;
        task->draw = person_draw;
        task->destructor = person_destroy;
    }
    /* NOLINTEND(misc-redundant-expression) */
}

/* load the textures of the kind's character, if it has one, add the person's
 * talk and start its collision */
static void person_start(struct task *task)
{
    struct person_work *work = PERSON(task);
    const struct person_kind *kind = &people_kinds[work->kind];

    if (kind->character >= 0) {
        texture_load_character(kind->character);
    }
    talk_add(task, &work->pos, &work->ang.y, &work->talking,
             kind->shape.radius + TALK_REACH, TALK_LEVEL);
    work->frame = 0.0F;
    collision_init(task, &kind->shape, COLLISION_OBJECT, 1, 1);
}

/* advance the action a frame, back to 0 once it passes the last, and draw
 * the person */
static void person_animate(struct task *task)
{
    struct person_work *work = PERSON(task);
    const struct person_kind *kind = &people_kinds[work->kind];

    /* @hack spelling: the frame advanced inside the test, which compares the
     * sum in FR2 as it stores it (FCMP/GT FR1,FR2 at 0x0c904e6c); advanced
     * before it, SHC loads the frame again, the function runs 2 bytes longer
     * and 12 of 37 words differ */
    if ((work->frame += 1.0F) > kind->action->motion->nbFrame - 1.0F) {
        work->frame = 0.0F;
    }
    person_draw(task);
}

/* animate the person. once shown is clear, end the talk, clear player 0's
 * look and go back to person_step */
static void person_listen(struct task *task)
{
    struct person_work *work = PERSON(task);

    if (!work->shown) {
        talk_end();
        player_clear_look(0);
        task->step = person_step;
    }
    person_animate(task);
}

/* while no event has control, register the person's collision and animate
 * it. once player 0 starts talking to it, run its row again and hand the
 * pick to script_show. when the heap has no room or the script fails, hand
 * it the last pick instead. past_script_table's rows pick no pages, so the
 * talk shows no message. player 0 then looks at the person's eye height
 * until person_listen ends the talk */
static void person_step(struct task *task)
{
    struct person_work *work = PERSON(task);
    const struct person_kind *kind = &people_kinds[work->kind];
    struct script_pages *pick;
    NJS_POINT3 eye;

    if (event_in_control()) {
        return;
    }
    collision_register((struct task_work *)work);
    if (work->talking) {
        pick = heap_alloc(sizeof(struct script_pages));
        if (pick != NULL && script_pick(work->row, pick)) {
            heap_free(work->pick);
            work->pick = pick;
        } else {
            heap_free(pick);
        }
        message_set_shown(&work->shown);
        script_show(work->pick);
        eye.x = work->pos.x;
        eye.y = work->pos.y + kind->eye;
        eye.z = work->pos.z;
        player_set_look_point(0, &eye);
        work->talking = FALSE;
        task->step = person_listen;
    }
    person_animate(task);
}

static void person_face_camera(NJS_OBJECT *object)
{
    if (object == &dat_0c9ddb2c) {
        TURN(njRotateY, camera_work->ang.y);
        TURN(njRotateX, camera_work->ang.x + 0x8000);
    }
}

/* on a frame's first step, while no event has control and the sphere of
 * VIEW_RADIUS about the person is in view, draw the person and its shadow */
static void person_draw(struct task *task)
{
    struct person_work *work = PERSON(task);
    const struct person_kind *kind;
    Float lift;
    NJS_TEXLIST *texlist;
    NJS_ACTION *action;

    if (!frame_step &&
        draw_in_view(work->pos.x, work->pos.y, work->pos.z, VIEW_RADIUS) &&
        !event_in_control()) {
        njPushMatrix(NULL);
        kind = &people_kinds[work->kind];
        lift = kind->lift;
        texlist = kind->texlist;
        action = kind->action;
        njTranslateV(NULL, &work->pos);
        shadow_draw_person(kind->shape.radius);
        njTranslate(NULL, 0.0F, lift, 0.0F);
        njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
        draw_set_palette(DRAW_PLAYER_PALETTE);
        njSetTexture(texlist);
        if (kind->character == NO_CHARACTER_CHAO) {
            njSetMotionCallback(person_face_camera);
        }
        draw_action(action, work->frame);
        if (kind->character == NO_CHARACTER_CHAO) {
            njSetMotionCallback(NULL);
        }
        draw_set_palette(0);
        njPopMatrix(1);
    }
}

/* give back the kind's character's textures, the pick and the talk */
static void person_destroy(struct task *task)
{
    struct person_work *work = PERSON(task);
    const struct person_kind *kind = &people_kinds[work->kind];

    if (kind->character >= 0) {
        texture_release_character(kind->character);
    }
    heap_free(work->pick);
    talk_remove(task);
}
