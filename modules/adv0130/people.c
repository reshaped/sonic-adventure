/* the people of the Egg Carrier inside, whom player 0 can talk to. only act
 * 3 has one: E-102, whose script passes for Amy alone */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0130/people.h"
#include "adv0130/scripts.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "event/play.h"
#include "event/script.h"
#include "object/common.h"
#include "object/models.h"
#include "player/player.h"
#include "player/talk.h"
#include "player/textures.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION e102_stand_action;

/* the most people egg_carrier_inside_people_place places at once */
#define PEOPLE_MAX 3

/* a person's talk reaches TALK_REACH past its shape's radius, and talk_add
 * adds player 0's radius to that */
#define TALK_REACH 20.0F

/* the people task's record, at task->extra */
struct people_watch {
    Bool placed; /* TRUE from the task's placing until the act changes */
    int act; /* the act it placed them in */
};

/* a kind of person: how it looks, its shape, and where player 0 looks during
 * a talk */
struct person_kind {
    int character; /* whose textures it loads, or below 0 for none */
    NJS_TEXLIST *texlist;
    NJS_ACTION *action;
    Float lift; /* how far above pos the model is drawn */
    Float eye; /* how far above pos player 0 looks during a talk */
    struct collision_shape shape;
};

/* a person's task work, which the task holds in place of a task_work. each
 * member the two share lies at the same offset in both */
struct person_work {
    Sint8 mode;
    Sint8 submode;
    Uint8 id;
    /* 1 while the person's message shows, as message_set_shown sets it */
    Uint8 shown;
    Sint16 flags;
    Sint16 kind;
    Float frame; /* the action's frame */
    /* TRUE once player 0 starts talking to the person. person_step clears it
     * as it starts the person's message */
    Bool talking;
    Uint8 unk_10[4];
    struct angles ang;
    NJS_POINT3 pos;
    Uint8 unk_2c[4];
    struct script_pages *pick; /* what the person's row last picked */
    int row; /* in egg_carrier_inside_script_table */
    struct collision_work *collision;
    struct event_work *event;
};

#define PERSON(task) ((struct person_work *)(task)->work)

/* a person kind's shape: a capsule or a sphere, centered y above the person.
 * the person's angles don't turn it. it pushes players and objects, and
 * players can't push it. a player it pushes back doesn't start to push
 * against it */
/* clang-format off */
#define PERSON_SHAPE(shape_kind, y, radius, size_y)                            \
    {(shape_kind) | SHAPE_NO_PUSH | SHAPE_UNTURNED, 0,                         \
     PUSH_PLAYERS | PUSH_OBJECTS | PUSH_FIRM(PUSH_PLAYERS),                    \
     {0.0F, (y), 0.0F}, (radius), (size_y), 0.0F, {0}, 0, NULL}
/* clang-format on */

static void people_come_out(struct people_watch *watch);
static void people_watch_act(struct people_watch *watch);
static void people_step(struct task *task);
static void person_init(struct task *task);
static void person_start(struct task *task);
static void person_animate(struct task *task);
static void person_listen(struct task *task);
static void person_step(struct task *task);
static void person_draw(struct task *task);
static void person_destroy(struct task *task);
static void person_shadow(struct task *task, Float size);

/* the people placed, people_count of them, in the order of placement */
static int people_count = 0;
static struct task *people[PEOPLE_MAX];

void egg_carrier_inside_people_init(void)
{
    struct task *task;
    struct people_watch *watch;

    script_set_table((struct script_row *)egg_carrier_inside_script_table);
    /* @hack spelling: task assigned in the test, which tests task_create's R0
     * with the copy to R14 in the BT/S's slot (0x0c92b796, 0x0c92b79a);
     * assigned apart, SHC tests R14 and 3 of 34 words differ */
    if ((task = task_create(0, 1, people_step)) != NULL) {
        watch = heap_calloc(1, sizeof(struct people_watch));
        if (watch != NULL) {
            task->extra = watch;
            people_step(task);
        } else {
            task_destroy(task);
        }
    }
}

static void people_come_out(struct people_watch *watch)
{
    int act = (Uint8)stage_get_stage_act();

    egg_carrier_inside_people_place();
    watch->act = act;
    watch->placed = TRUE;
}

/* destroy the people once the act isn't the one they were placed in */
static void people_watch_act(struct people_watch *watch)
{
    if (watch->act != (Uint8)stage_get_stage_act()) {
        egg_carrier_inside_people_remove();
        watch->placed = FALSE;
    }
}

static void people_step(struct task *task)
{
    struct people_watch *watch = task->extra;

    if (!watch->placed) {
        people_come_out(watch);
    } else {
        people_watch_act(watch);
    }
}

void egg_carrier_inside_people_place(void)
{
    const struct person_list *list;
    const struct person_spot *spot;
    struct script_pages *pick;
    struct task *task;
    struct person_work *work;
    int left;
    int row;

    people_count = 0;
    list = &egg_carrier_inside_people_lists[(Uint8)stage_get_stage_act()];
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

void egg_carrier_inside_people_remove(void)
{
    while (people_count != 0) {
        people_count--;
        if (people[people_count] != NULL) {
            task_destroy(people[people_count]);
            people[people_count] = NULL;
        }
    }
}

static const struct person_kind people_kinds[1] = {
    {CHARACTER_E102, &e102_texlist, &e102_stand_action, 14.0F, 20.0F,
     PERSON_SHAPE(SHAPE_CAPSULE, 10.5F, 8.0F, 6.5F)},
};

/* start the person, and give it its step, draw and destructor. the
 * WORK_SAME_PLACE tests always pass */
static void person_init(struct task *task)
{
    /* @hack redundant: pos tested a second time, CMP/EQ R2,R3 at 0x0c92b9bc
     * after ADD #32 to each; tested once, the function runs 12 bytes shorter
     * and 24 of 51 words differ */
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

    /* @hack redundant: the empty else, which keeps talk_add's address in R2 and
     * TALK_LEVEL in R3 (0x0c92ba16, 0x0c92ba18); without it, the two trade
     * and 4 of 57 words differ */
    if (kind->character >= 0) {
        texture_load_character(kind->character);
    } else {
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
     * sum in FR2 as it stores it (FCMP/GT FR1,FR2 at 0x0c92ba92); advanced
     * before it, SHC loads the frame again, the function runs 2 bytes longer
     * and 15 of 38 words differ */
    if ((work->frame += 1.0F) > kind->action->motion->nbFrame - 1.0F) {
        work->frame = 0.0F;
    }
    person_draw(task);
}

/* animate the person. once its message has ended, end the talk, clear player
 * 0's look and go back to person_step */
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
 * it. once player 0 starts talking to it, run its row again and show the
 * pick. when the heap has no room or the script fails, show the last pick
 * instead. player 0 then looks at the person's eye height until
 * person_listen ends the talk */
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

/* on a frame's first step, while no event has control, draw the person and
 * its shadow. draw_in_view measures it by the larger of its shape's radius
 * and size_y */
static void person_draw(struct task *task)
{
    struct person_work *work = PERSON(task);
    const struct person_kind *kind = &people_kinds[work->kind];
    Float lift;
    NJS_TEXLIST *texlist;
    NJS_ACTION *action;

    if (!frame_step &&
        draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                     work->collision->shapes->radius >
                             work->collision->shapes->size_y
                         ? work->collision->shapes->radius
                         : work->collision->shapes->size_y) &&
        !event_in_control()) {
        person_shadow(task, kind->shape.radius);
        njPushMatrix(NULL);
        lift = kind->lift;
        texlist = kind->texlist;
        action = kind->action;
        njTranslateV(NULL, &work->pos);
        njTranslate(NULL, 0.0F, lift, 0.0F);
        njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
        draw_set_palette(DRAW_PLAYER_PALETTE);
        njSetTexture(texlist);
        draw_action(action, work->frame);
        draw_set_palette(0);
        njPopMatrix(1);
    }
}

/* release the kind's textures and the pick, and take the talk off the list.
 * a kind whose character is above 0 releases that character's textures, and
 * any other its texlist. so for a character of 0, the texlist goes, though
 * person_start loaded CHARACTER_SONIC's textures. the one kind in
 * people_kinds has the character CHARACTER_E102 */
static void person_destroy(struct task *task)
{
    struct person_work *work = PERSON(task);
    const struct person_kind *kind = &people_kinds[work->kind];

    if (kind->character > 0) {
        texture_release_character(kind->character);
    } else {
        njReleaseTexture(kind->texlist);
    }
    heap_free(work->pick);
    talk_remove(task);
}

/* draw shadow_model unturned on the ground below the person, scaled by a
 * quarter of size. it ignores the ground's slope */
static void person_shadow(struct task *task, Float size)
{
    struct person_work *work = PERSON(task);
    struct angles slope;
    Float y;
    Float scale;

    y = ground_below(work->pos.x, work->pos.y, work->pos.z, &slope) + 0.6F;
    texture_set_regular();
    njPushMatrix(NULL);
    scale = size * 0.25F;
    njTranslate(NULL, work->pos.x, y, work->pos.z);
    njScale(NULL, scale, scale, scale * 0.8F);
    draw_model(&shadow_model);
    njPopMatrix(1);
}
