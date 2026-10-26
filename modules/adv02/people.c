/* the people of the Mystic Ruins' acts, whom player 0 can talk to */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/people.h"
#include "adv02/scripts.h"
#include "adv02/shadow.h"
#include "adv02/texlists.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "event/play.h"
#include "event/script.h"
#include "object/common.h"
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
extern NJS_ACTION eggman_stand_action;

/* Knuckles' and Eggman's other actions, the model the Chaos Emerald's action
 * names, and the welds of Big's arms */
extern NJS_ACTION dat_0cb39660;
extern NJS_ACTION dat_0cb3878c;
extern NJS_OBJECT mystic_ruins_emerald_3_object;
extern struct weld mystic_ruins_big_welds[];

/* the last of Big's nodes that his welds need posed */
#define BIG_LAST_WELD_NODE 73

/* the most people mystic_ruins_people_place places at once */
#define PEOPLE_MAX 3

/* the character of a kind that is a thing, not a character. such a kind loads
 * no character's textures, and its action's object is drawn unposed */
#define PERSON_STILL (-2)

/* a person's talk reaches TALK_REACH past its shape's radius, and talk_add
 * adds player 0's radius to that */
#define TALK_REACH 20.0F

/* the matrices welds_pose takes: both welded nodes' matrices, then their
 * inverses */
#define WELD_MATRICES 4

/* the radius draw_in_view tests a person's position by */
#define VIEW_RADIUS 15.0F

/* a kind of person: how it looks, its shape, and where player 0 looks during
 * a talk */
struct person_kind {
    int character; /* a CHARACTER_ value, or PERSON_STILL */
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
    Sint16 kind;
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
    int row; /* in mystic_ruins_script_table */
    struct collision_work *collision;
    struct event_work *event;
};

#define PERSON(task) ((struct person_work *)(task)->work)

#define KIND(work) (&people_kinds[(work)->kind])

/* a person kind's shape: a capsule centered y above the person. the person's
 * angles don't turn it. it pushes players and objects, and players can't push
 * it. a player it pushes back doesn't start to push against it */
/* clang-format off */
#define PERSON_SHAPE(y, radius, size_y)                                        \
    {SHAPE_CAPSULE | SHAPE_NO_PUSH | SHAPE_UNTURNED, 0,                        \
     PUSH_PLAYERS | PUSH_OBJECTS | PUSH_FIRM(PUSH_PLAYERS),                    \
     {0.0F, (y), 0.0F}, (radius), (size_y), 0.0F, {0}, 0, NULL}
/* clang-format on */

static void person_init(struct task *task);
static void person_start(struct task *task);
static void person_animate(struct task *task);
static void person_listen(struct task *task);
static void person_step(struct task *task);
static void person_draw(struct task *task);
static void person_destroy(struct task *task);
static void welds_save(struct weld *weld);
static void welds_pose(NJS_ACTION *action, struct weld *weld,
                       NJS_MATRIX *matrices, Uint32 last_node, Float frame);
static void welds_restore(struct weld *weld);

static NJS_ACTION mystic_ruins_emerald_action = {
#include "adv02/people/mystic_ruins_emerald_action.inc"
};

static int people_count = 0;

/* person kinds: Sonic, Tails, Knuckles, Big and Eggman standing, Knuckles and
 * Eggman in their other actions, and a Chaos Emerald */
static const struct person_kind people_kinds[8] = {
    {CHARACTER_SONIC, &sonic_texlist, &sonic_stand_action, 5.4F, 7.0F,
     PERSON_SHAPE(5.0F, 4.0F, 3.0F), NULL, 0},
    {CHARACTER_TAILS, &tails_texlist, &tails_stand_action, 4.5F, 6.0F,
     PERSON_SHAPE(4.5F, 3.0F, 3.0F), NULL, 0},
    {CHARACTER_KNUCKLES, &knuckles_texlist, &knuckles_stand_action, 5.7F, 9.0F,
     PERSON_SHAPE(5.7F, 4.0F, 3.7F), NULL, 0},
    {CHARACTER_BIG, &big_texlist, &big_stand_action, 8.0F, 15.0F,
     PERSON_SHAPE(8.5F, 9.0F, 4.0F), mystic_ruins_big_welds,
     BIG_LAST_WELD_NODE},
    {CHARACTER_EGGMAN, &eggman_texlist, &eggman_stand_action, 8.6F, 7.0F,
     PERSON_SHAPE(7.5F, 5.0F, 5.0F), NULL, 0},
    {CHARACTER_KNUCKLES, &knuckles_texlist, &dat_0cb39660, 5.7F, 9.0F,
     PERSON_SHAPE(5.7F, 4.0F, 3.7F), NULL, 0},
    {CHARACTER_EGGMAN, &eggman_texlist, &dat_0cb3878c, 8.6F, 7.0F,
     PERSON_SHAPE(7.5F, 5.0F, 5.0F), NULL, 0},
    {PERSON_STILL, &mrobj_texlist, &mystic_ruins_emerald_action, 8.6F, 7.0F,
     PERSON_SHAPE(7.5F, 5.0F, 5.0F), NULL, 0},
};

/* the people placed, people_count of them, in the order of placement */
static struct task *people[PEOPLE_MAX];

void mystic_ruins_people_place(void)
{
    const struct person_list *list;
    const struct person_spot *spot;
    struct script_pages *pick;
    struct task *task;
    struct person_work *work;
    int left;
    int row;

    people_count = 0;
    list = &mystic_ruins_people_lists[(Uint8)stage_get_stage_act()];
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

/* destroy the people in reverse order of placement */
void mystic_ruins_people_remove(void)
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
    /* @hack redundant: pos tested a second time, since SHC writes code for each
     * test (CMP/EQ at 0x0c913b2a and 0x0c913b5a); tested once, 24 of 51 words
     * differ */
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
 * talk and start its collision. a kind with welds saves their vertices and
 * takes the matrices that pose them from the heap */
static void person_start(struct task *task)
{
    struct person_work *work = PERSON(task);
    const struct person_kind *kind;

    /* @hack spelling: kind assigned inside the test, which keeps work in R14
     * and kind in R13 (0x0c913bca, 0x0c913bda); assigned ahead of the test, the
     * two trade and 15 of 66 words differ */
    /* @hack redundant: an empty else, which puts talk_add's address in R3 and
     * TALK_LEVEL in R2 (0x0c913bf0, 0x0c913bf2); without it, the two trade and
     * 4 of 66 words differ */
    if ((kind = KIND(work))->character >= 0) {
        texture_load_character(kind->character);
    } else {
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

/* advance the action a frame, back to 0 once it passes the last, unless the
 * kind is PERSON_STILL. then draw the person. on a frame's first step, a kind
 * with welds then poses them at the new frame */
static void person_animate(struct task *task)
{
    struct person_work *work = PERSON(task);
    const struct person_kind *kind = KIND(work);

    if (kind->character != PERSON_STILL) {
        /* @hack spelling: the wrap as a conditional expression, which keeps
         * work in R13 and kind in R14 (0x0c913c86, 0x0c913c96); as an if, the
         * two trade, the code runs 2 bytes longer, and 45 of 66 words differ */
        ((work->frame = work->frame + 1.0F) >
         kind->action->motion->nbFrame - 1.0F)
            ? (void)(work->frame = 0.0F)
            : (void)0;
    }
    person_draw(task);
    if (kind->welds != NULL && !frame_step) {
        welds_pose(kind->action, kind->welds, work->weld_matrices,
                   kind->last_node, work->frame);
    }
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
    const struct person_kind *kind = KIND(work);
    struct script_pages *pick;
    NJS_POINT3 eye;

    if (!event_in_control()) {
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
        kind = KIND(work);
        lift = kind->lift;
        texlist = kind->texlist;
        action = kind->action;
        njTranslateV(NULL, &work->pos);
        mystic_ruins_shadow_draw_person(kind->shape.radius);
        njTranslate(NULL, 0.0F, lift, 0.0F);
        njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
        njSetTexture(texlist);
        if (kind->character == PERSON_STILL) {
            draw_tree(action->object);
        } else {
            draw_set_palette(DRAW_PLAYER_PALETTE);
            draw_action(action, work->frame);
            draw_set_palette(0);
        }
        njPopMatrix(1);
    }
}

/* restore the welds' vertices, and give back their matrices, the kind's
 * textures, the pick and the talk */
static void person_destroy(struct task *task)
{
    struct person_work *work = PERSON(task);
    const struct person_kind *kind = KIND(work);

    if (kind->welds != NULL) {
        welds_restore(kind->welds);
        heap_free(work->weld_matrices);
    }
    if (kind->character >= 0) {
        texture_release_character(kind->character);
    }
    heap_free(work->pick);
    talk_remove(task);
}

/* save each weld's shared vertices, a's point and normal then b's, and find its
 * two nodes in the model. the heap must have room */
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
 * model, a's shared vertices go back to the copy welds_save made. b's move onto
 * them, through a's matrix and b's inverse */
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
