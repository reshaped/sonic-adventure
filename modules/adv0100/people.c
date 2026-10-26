/* the Egg Carrier outside's people, whom player 0 can talk to */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/people.h"
#include "adv0100/scripts.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "event/play.h"
#include "event/script.h"
#include "object/common.h"
#include "object/models.h"
#include "player/big_models.h"
#include "player/player.h"
#include "player/talk.h"
#include "player/textures.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION tails_stand_action;
extern NJS_ACTION knuckles_stand_action;
extern NJS_ACTION amy_stand_action;
extern NJS_ACTION e102_stand_action;

/* data of the kinds that this module holds. dat_0caf2f44 is the Eggmobile's
 * action. dat_0cb01818 is the action of the model CHAOS4_COMMON's textures
 * cover. dat_0cb01998 is that model's table of welds, and dat_0cb01b60 is
 * Big's. a weld closes a seam of its model as the model is posed */
extern NJS_TEXLIST ev_eggmoble0_texlist;
extern NJS_ACTION dat_0caf2f44;
extern NJS_TEXLIST chaos4_common_texlist;
extern NJS_ACTION dat_0cb01818;
extern struct weld dat_0cb01998[];
extern struct weld dat_0cb01b60[];

/* the most people egg_carrier_people_place places at once */
#define PEOPLE_MAX 3

/* the characters of the kinds that load a PVM of their own, which
 * people_pvms names */
#define PERSON_CHAOS4_PVM (-10)
#define PERSON_EGGMOBILE_PVM (-9)

/* a person's talk reaches TALK_REACH past its shape's radius, and talk_add
 * adds player 0's radius to that */
#define TALK_REACH 20.0F

/* the matrices welds_pose takes: both welded nodes' matrices, then their
 * inverses */
#define WELD_MATRICES 4

/* the people task's record, at task->extra */
struct people_watch {
    Bool placed; /* TRUE from the task's placing until the act changes */
    int act; /* the act it placed them in */
};

/* a kind of person: how it looks, its shape, and where player 0 looks during
 * a talk */
struct person_kind {
    int character; /* whose textures it loads, or a PERSON_*_PVM */
    NJS_TEXLIST *texlist;
    NJS_ACTION *action;
    Float lift; /* how far above pos the model is drawn */
    Float eye; /* how far above pos player 0 looks during a talk */
    struct collision_shape shape;
    struct weld *welds; /* an array ended by a weld with a null base, or NULL */
    Uint32 last_node; /* the last node welds_pose poses, counted from 0 */
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
    Sint16 kind; /* an enum person_kind_id */
    Float frame; /* the action's frame */
    /* TRUE once player 0 starts talking to the person. person_step clears it
     * as it starts the person's message */
    Bool talking;
    /* WELD_MATRICES matrices for welds_pose, for a kind with welds */
    NJS_MATRIX *weld_matrices;
    struct angles ang;
    NJS_POINT3 pos;
    Uint8 unk_2c[4];
    struct script_pages *pick; /* what the person's row last picked */
    int row; /* in egg_carrier_script_table */
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
static void welds_save(struct weld *weld);
static void welds_pose(NJS_ACTION *action, struct weld *weld,
                       NJS_MATRIX *matrices, Uint32 last_node, Float frame);
static void welds_restore(struct weld *weld);

/* the PVMs of the kinds that load their own, by character less
 * PERSON_CHAOS4_PVM */
static char *people_pvms[2] = {"CHAOS4_COMMON", "ev_eggmoble0"};

/* the people placed, people_count of them, in the order of placement */
static int people_count = 0;
static struct task *people[PEOPLE_MAX];

void egg_carrier_people_init(void)
{
    struct task *task;
    struct people_watch *watch;
    int i;

    for (i = 0; i < PEOPLE_MAX; i++) {
        people[i] = NULL;
    }
    script_set_table((struct script_row *)egg_carrier_script_table);
    /* @hack spelling: task assigned in the test, which tests task_create's R0
     * with the copy to R14 in the BT/S's slot (0x0c92e790, 0x0c92e794);
     * assigned apart, SHC tests R14 and 3 of 47 words differ */
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

    egg_carrier_people_place();
    watch->act = act;
    watch->placed = TRUE;
}

/* destroy the people once the act isn't the one they were placed in */
static void people_watch_act(struct people_watch *watch)
{
    if (watch->act != (Uint8)stage_get_stage_act()) {
        egg_carrier_people_remove();
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

void egg_carrier_people_place(void)
{
    const struct person_list *list;
    const struct person_spot *spot;
    struct script_pages *pick;
    struct task *task;
    struct person_work *work;
    int left;
    int row;

    people_count = 0;
    list = &egg_carrier_people_lists[(Uint8)stage_get_stage_act()];
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

void egg_carrier_people_remove(void)
{
    while (people_count != 0) {
        people_count--;
        if (people[people_count] != NULL) {
            task_destroy(people[people_count]);
            people[people_count] = NULL;
        }
    }
}

/* by enum person_kind_id: the six characters standing, the Eggmobile, and
 * the model CHAOS4_COMMON's textures cover. Big's kind and the last have
 * welds */
static const struct person_kind people_kinds[PERSON_KINDS] = {
    {CHARACTER_SONIC, &sonic_texlist, &sonic_stand_action, 5.4F, 7.0F,
     PERSON_SHAPE(SHAPE_CAPSULE, 5.0F, 4.0F, 3.0F), NULL, 0},
    {CHARACTER_TAILS, &tails_texlist, &tails_stand_action, 4.5F, 6.0F,
     PERSON_SHAPE(SHAPE_CAPSULE, 4.5F, 3.0F, 3.0F), NULL, 0},
    {CHARACTER_KNUCKLES, &knuckles_texlist, &knuckles_stand_action, 5.7F, 9.0F,
     PERSON_SHAPE(SHAPE_CAPSULE, 5.7F, 4.0F, 3.7F), NULL, 0},
    {CHARACTER_AMY, &amy_texlist, &amy_stand_action, 5.0F, 7.0F,
     PERSON_SHAPE(SHAPE_CAPSULE, 5.0F, 3.0F, 3.5F), NULL, 0},
    {CHARACTER_E102, &e102_texlist, &e102_stand_action, 14.0F, 20.0F,
     PERSON_SHAPE(SHAPE_CAPSULE, 10.5F, 8.0F, 6.5F), NULL, 0},
    {CHARACTER_BIG, &big_texlist, &big_stand_action, 8.0F, 15.0F,
     PERSON_SHAPE(SHAPE_CAPSULE, 8.5F, 9.0F, 4.0F), dat_0cb01b60, 73},
    {PERSON_EGGMOBILE_PVM, &ev_eggmoble0_texlist, &dat_0caf2f44, 0.0F, 7.0F,
     PERSON_SHAPE(SHAPE_SPHERE, 12.0F, 10.0F, 0.0F), NULL, 0},
    {PERSON_CHAOS4_PVM, &chaos4_common_texlist, &dat_0cb01818, 55.0F, 7.0F,
     PERSON_SHAPE(SHAPE_SPHERE, 55.0F, 30.0F, 0.0F), dat_0cb01998, 79},
};

/* start the person, and give it its step, draw and destructor. the
 * WORK_SAME_PLACE tests always pass */
static void person_init(struct task *task)
{
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (WORK_SAME_PLACE(struct person_work, pos) &&
        WORK_SAME_PLACE(struct person_work, id) &&
        WORK_SAME_PLACE(struct person_work, collision) &&
        WORK_SAME_PLACE(struct person_work, event) &&
        /* @hack redundant: pos tested a second time, at 0x0c92e992 to
         * 0x0c92e99c; tested once, 24 of 51 words differ */
        WORK_SAME_PLACE(struct person_work, pos) &&
        WORK_SAME_PLACE(struct person_work, flags)) {
        person_start(task);
        task->step = person_step;
        task->draw = person_draw;
        task->destructor = person_destroy;
    }
    /* NOLINTEND(misc-redundant-expression) */
}

/* load the kind's textures, add the person's talk and start its collision.
 * a kind with welds saves their vertices and takes the matrices that pose
 * them from the heap */
static void person_start(struct task *task)
{
    struct person_work *work = PERSON(task);
    const struct person_kind *kind = &people_kinds[work->kind];

    if (kind->character >= 0) {
        texture_load_character(kind->character);
    } else {
        texture_load_pvm(people_pvms[kind->character - PERSON_CHAOS4_PVM],
                         kind->texlist);
    }
    talk_add(task, &work->pos, &work->ang.y, &work->talking,
             kind->shape.radius + TALK_REACH, TALK_LEVEL);
    work->frame = 0.0F;
    collision_init(task, &kind->shape, COLLISION_OBJECT, 1, 1);
    if (kind->welds != NULL) {
        welds_save(kind->welds);
        work->weld_matrices = heap_alloc(WELD_MATRICES * sizeof(NJS_MATRIX));
    }
}

/* advance the action a frame, back to 0 once it passes the last, and draw
 * the person. on a frame's first step, a kind with welds poses them at the
 * new frame first */
static void person_animate(struct task *task)
{
    struct person_work *work = PERSON(task);
    const struct person_kind *kind = &people_kinds[work->kind];

    /* @hack spelling: a select in place of an if, which loads kind->welds
     * into R2 (MOV.L @(R0,R13),R2 at 0x0c92eafa); as an if, SHC loads it
     * into R3 and 2 of 62 words differ */
    ((work->frame = work->frame + 1.0F) > kind->action->motion->nbFrame - 1.0F)
        ? (void)(work->frame = 0.0F)
        : (void)0;
    if (kind->welds != NULL && !frame_step) {
        welds_pose(kind->action, kind->welds, work->weld_matrices,
                   kind->last_node, work->frame);
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

/* restore the welds' vertices, and give back their matrices, the kind's
 * textures, the pick and the talk */
static void person_destroy(struct task *task)
{
    struct person_work *work = PERSON(task);
    const struct person_kind *kind = &people_kinds[work->kind];

    if (kind->welds != NULL) {
        welds_restore(kind->welds);
        heap_free(work->weld_matrices);
    }
    /* @bug logic: the test takes Sonic's kind, character 0, for a kind
     * with a PVM of its own (CMP/PL at 0x0c92eddc), where person_start loads
     * it as a character (CMP/PZ at 0x0c92e9e0). its sonic_texlist is
     * released with njReleaseTexture, and character_users keeps counting
     * its user */
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

/* save each weld's shared vertices, a's point and normal then b's, and find
 * its two nodes in the model. the heap must have room */
static void welds_save(struct weld *weld)
{
    NJS_POINT3 *saved;
    NJS_MODEL *model_a;
    NJS_MODEL *model_b;
    int i;

    for (; weld->base != NULL; weld++) {
        weld->vertices = heap_alloc(4 * sizeof(NJS_POINT3) * weld->count);
        model_a = weld->object_a->model;
        model_b = weld->object_b->model;
        saved = weld->vertices;
        for (i = 0; i < weld->count; i++) {
            *saved++ = model_a->points[weld->indices[i * 2]];
            *saved++ = model_a->normals[weld->indices[i * 2]];
            *saved++ = model_b->points[weld->indices[(i * 2) + 1]];
            *saved++ = model_b->normals[weld->indices[(i * 2) + 1]];
        }
        weld->node_a = tree_node_index(weld->base, weld->object_a);
        weld->node_b = tree_node_index(weld->base, weld->object_b);
    }
}

/* pose the action at frame as far as last_node. in each weld of the action's
 * model, a's shared vertices go back to the copy welds_save made. b's move
 * onto them, through a's matrix and b's inverse */
static void welds_pose(NJS_ACTION *action, struct weld *weld,
                       NJS_MATRIX *matrices, Uint32 last_node, Float frame)
{
    NJS_MATRIX *matrix_a = matrices++;
    NJS_MATRIX *matrix_b = matrices++;
    NJS_MATRIX *inverse_a = matrices++;
    NJS_MATRIX *inverse_b = matrices;
    NJS_POINT3 *points_a;
    NJS_VECTOR *normals_a;
    NJS_POINT3 *points_b;
    NJS_VECTOR *normals_b;
    int i;
    int index_a;
    int index_b;

    pose_action(&_unit_matrix_, action, frame, last_node, NULL);
    for (; weld->base != NULL; weld++) {
        if (weld->base == action->object) {
            pose_get_matrix(weld->node_a, matrix_a);
            pose_get_matrix(weld->node_b, matrix_b);
            njSetMatrix(inverse_a, matrix_a);
            njSetMatrix(inverse_b, matrix_b);
            njInvertMatrix(inverse_a);
            njInvertMatrix(inverse_b);
            njPushMatrix(inverse_b);
            njMultiMatrix(NULL, matrix_a);
            points_a = weld->object_a->model->points;
            normals_a = weld->object_a->model->normals;
            points_b = weld->object_b->model->points;
            normals_b = weld->object_b->model->normals;
            for (i = 0; i < weld->count; i++) {
                index_a = weld->indices[i * 2];
                index_b = weld->indices[(i * 2) + 1];
                points_a[index_a] = weld->vertices[i * 4];
                njCalcPoint(NULL, &points_a[index_a], &points_b[index_b]);
                normals_a[index_a] = weld->vertices[(i * 4) + 1];
                njCalcVector(NULL, &normals_a[index_a], &normals_b[index_b]);
            }
            njPopMatrix(1);
        }
    }
}

/* restore each weld's saved vertices, and free the copy */
static void welds_restore(struct weld *weld)
{
    NJS_MODEL *model_a;
    NJS_MODEL *model_b;
    NJS_POINT3 *saved;
    int i;

    for (; weld->base != NULL; weld++) {
        saved = weld->vertices;
        model_a = weld->object_a->model;
        model_b = weld->object_b->model;
        for (i = 0; i < weld->count; i++) {
            model_a->points[weld->indices[i * 2]] = *saved++;
            model_a->normals[weld->indices[i * 2]] = *saved++;
            model_b->points[weld->indices[(i * 2) + 1]] = *saved++;
            model_b->normals[weld->indices[(i * 2) + 1]] = *saved++;
        }
        heap_free(weld->vertices);
    }
}
