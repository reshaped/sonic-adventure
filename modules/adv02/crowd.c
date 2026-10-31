/* the people the object list names O PEOPLE, who stand where the set places
 * them, and the walkers on the jungle's paths */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv02/crowd.h"
#include "adv02/mystic_ruins.h"
#include "adv02/people.h"
#include "adv02/scripts.h"
#include "adv02/shadow.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "event/play.h"
#include "event/script.h"
#include "object/common.h"
#include "player/player.h"
#include "player/talk.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/loader.h"
#include "system/math.h"
#include "system/task.h"
#include "system/util.h"

/* the walkers' three models, as walker_models orders them, and the texture
 * list they share */
extern NJS_OBJECT dat_0cb2e33c;
extern NJS_OBJECT dat_0cb26a34;
extern NJS_OBJECT dat_0cb298cc;
extern NJS_TEXLIST dat_0cb396d4;
/* the standing, talking, walking and knocked motions, which both kinds
 * play */
extern NJS_MOTION dat_0cb2fd10;
extern NJS_MOTION dat_0cb36b14;
extern NJS_MOTION dat_0cb324fc;
extern NJS_MOTION dat_0cb34c08;

/* the message file mystic_ruins_messages_load reads, at 0x0cb4a000, past the
 * module's sections */
extern struct field_messages field_messages;

#define WALKER_MOVES 4
#define WALKER_MODELS 3

/* on each frame's first step, a walking walker covers its kind's speed times
 * WALK_STRIDE */
#define WALK_STRIDE 1.0F

/* the most a walker turns toward its target each time it moves, just under 5
 * degrees */
#define TURN_LIMIT 0x38e

/* a walker that moves finds the highest floor at or below the point
 * SETTLE_ABOVE above it, at its new x but its old z. it settles on that floor
 * only when the floor lies less than SETTLE_REACH from its height */
#define SETTLE_ABOVE 5.0F
#define SETTLE_REACH 15.0F

/* a walking path walker stops once player 0 comes within player 0's radius
 * plus STOP_GAP of it. it walks on once player 0 is beyond that radius plus
 * GO_GAP */
#define STOP_GAP 10.0F
#define GO_GAP 20.0F

/* a walker's talk reaches TALK_REACH past its shape's radius, and talk_add
 * adds player 0's radius to that */
#define TALK_REACH 20.0F

/* a walker draws only while draw_in_view finds the sphere of VIEW_RADIUS
 * about its position in view */
#define VIEW_RADIUS 15.0F

enum walker_move_id {
    MOVE_STAND,
    MOVE_TALK,
    MOVE_WALK,
    /* played when a player with PLAYER_GUARD set hits the walker */
    MOVE_KNOCKED
};

/* the chance that the standing move, as it ends, picks MOVE_STAND as the
 * next move. that is the next move already, so the pick changes nothing */
#define STAND_PICK 0.1F

/* each step, a move's motion advances by the move's speed times MOVE_TIME.
 * walking advances it on each frame's first step only */
#define MOVE_TIME 1.0F

/* how far a blend between two moves advances each step, from 0 to 1 */
#define BLEND_STOP (1.0F / 30.0F) /* between walking and standing */
#define BLEND_MOVE 0.08333334F /* between other moves */
#define BLEND_KNOCKED 0.25F /* into MOVE_KNOCKED */

struct walker_move {
    NJS_MOTION *motion;
    Float speed; /* motion frames each step */
};

struct walker_kind {
    struct walker_move moves[WALKER_MOVES];
    Float lift; /* its model's height above its shadow */
    /* the height above its position that player 0 looks at while it talks */
    Float eye;
    Float speed; /* the walking speed, in units of WALK_STRIDE */
    struct collision_shape shape;
};

struct walker_model {
    NJS_OBJECT *object;
    NJS_TEXLIST *texlist;
    int kind; /* an index into walker_kinds */
};

/* a path as path_mr.bin stores it. the file opens with its paths, and a path
 * whose points is null ends them. points is an offset from the file's start
 * until crowd_route_create adds the file's address */
struct walk_path {
    NJS_POINT3 *points;
    int count; /* the points on the path */
};

/* unk_01 and unk_03 are 0 in each of jungle_walkers' entries, and nothing
 * reads them */
struct walker_spec {
    Sint8 model;
    Uint8 unk_01;
    /* the walker's row of the field's table of lines, less
     * field_messages.people */
    Uint8 row;
    Uint8 unk_03;
};

struct walker_start {
    Sint16 along;
    Sint8 model;
    Uint8 row; /* its row of the field's table of lines */
    Sint8 way; /* 1 forward along the path, -1 back */
};

/* a walker's place on its path. along counts in points: points[i] lies at
 * along i */
struct walker_place {
    /* x and z of the position the walker heads for. y is the rise to it from
     * the walker's height when it was aimed. the walker climbs y for each 1
     * that along gains */
    NJS_POINT3 target;
    Float along;
    Float advance; /* along gained on each frame's first step, times pace */
    Float pace; /* always 1: only walker_create sets it */
    Float switch_end; /* the along where a change of direction ends */
    Bool switching;
    Sint16 point;
    Sint8 way; /* 1 forward along the path, -1 back */
};

/* the record the route task keeps in its extra */
struct walk_route {
    struct walk_path *paths; /* path_mr.bin as read, in whole sectors */
};

/* the record the crowd task keeps in its extra */
struct crowd {
    Bool started; /* TRUE once the act's people and route start */
    struct task *route; /* the jungle's route task, or NULL */
    int act; /* the act the people and route started in */
};

/* the record a walker task keeps in its extra. while next_move differs from
 * move, the walker blends from one move into the other. the blend holds move
 * at last_frame and starts next_move at its first frame. each step adds
 * blend to the blend's progress in work->frame, and the blend ends once the
 * progress passes 1 */
struct walker {
    int move;
    int next_move;
    int return_move; /* the move to resume after MOVE_KNOCKED */
    NJS_OBJECT *object;
    NJS_TEXLIST *texlist;
    struct walker_kind *kind;
    Uint8 unk_18[4];
    Float last_frame;
    Float blend;
    struct script_lines *lines;
    Bool (*update)(struct task *task);
    struct walker_place *place;
    Bool animate; /* TRUE to draw the motion, FALSE the plain model */
};

/* a walker's view of its task_work, which keeps task_work's own fields at
 * their offsets. the set entry gives the model in ang.x, the squared reach in
 * scl.x and the message row in scl.z */
struct walker_work {
    Sint8 mode;
    Sint8 submode;
    Uint8 id;
    Uint8 shown; /* set while the walker's lines show */
    Sint16 flags;
    Uint16 count; /* number of points on the walker's path */
    Float frame; /* the move's frame, or the blend's progress into the next */
    Bool talking; /* set when player 0 talks to the walker */
    NJS_POINT3 *points;
    /* the walker_models index to apply, or -1 once applied */
    Sint32 model;
    Angle ang_y;
    Float distance2; /* squared distance in x and z from player 0 */
    NJS_POINT3 pos;
    Float reach2; /* squared distance from player 0 within which it shows */
    Float unk_30;
    Float row; /* its row of the field's table of lines */
    struct collision_work *collision;
    struct event_work *event;
};

#define WALKER(task) ((struct walker_work *)(task)->work)

/* a walker kind's capsule of radius and size_y, centered y above its feet */
/* clang-format off */
#define WALKER_SHAPE(y, radius, size_y)                                        \
    {SHAPE_CAPSULE | SHAPE_NO_PUSH | SHAPE_UNTURNED, 0,                        \
     PUSH_PLAYERS | PUSH_OBJECTS | PUSH_FIRM(PUSH_PLAYERS),                    \
     {0.0F, (y), 0.0F}, (radius), (size_y), 0.0F, {0}, 0, NULL}
/* clang-format on */

/* both kinds' moves, by walker_move_id: each a motion and its speed */
/* clang-format off */
#define WALKER_MOVE_SET                                                        \
    {{&dat_0cb2fd10, 0.5F}, {&dat_0cb36b14, 0.5F}, {&dat_0cb324fc, 1.0F},     \
     {&dat_0cb34c08, 1.0F}}
/* clang-format on */

/* distance from player 0 within which a walker shows, when the set entry gives
 * no reach of its own. a squared reach below NO_REACH2 counts as none */
#define WALKER_REACH 410.0F
#define NO_REACH2 0.01F

/* a standing walker within this distance of player 0 draws its motion, and
 * beyond it the plain model */
#define WALKER_NEAR 150.0F

static struct task *crowd_route_create(int act);
static void crowd_start(struct crowd *crowd);
static void crowd_leave_act(struct crowd *crowd);
static void crowd_step(struct task *task);
static void walker_draw_plain(struct task *task);
static void walker_print(struct task *task);
static void route_add_walker(struct task *task, struct walk_path *path,
                             int index);
static void route_fill(struct task *task);
static void route_wait(struct task *task);
static void route_step(struct task *task);
static void route_destroy(struct task *task);
static void walker_place_at(struct walker_work *work, NJS_POINT3 *out,
                            int point, int way);
static int walker_find_point(struct walker_work *work, NJS_POINT3 *out,
                             Float along, int way);
static void walker_start_walking(struct task *task, struct walker_place *place,
                                 NJS_POINT3 *points, int point);
static struct walker_place *walker_create(struct task *task, NJS_POINT3 *points,
                                          int count,
                                          struct walker_start *start);
static Bool walker_walk(struct task *task);
static int walk_next_point(int point, int way, int count);
static void walker_aim(struct walker_place *place, int index, NJS_POINT3 *point,
                       NJS_POINT3 *pos, Float along, Float speed);
static void walker_go(NJS_POINT3 *pos, NJS_POINT3 *target, Angle *ang,
                      Float speed, Float advance);
static Angle walker_turn(NJS_POINT3 *pos, NJS_POINT3 *target, Angle ang);
static void walker_head_for(struct walker_work *work,
                            struct walker_place *place, struct walker *walker,
                            Float along);
static Bool walker_update(struct task *task);
static Bool crowd_person_update(struct task *task);
static Bool walker_init(struct task *task, int move);
static void walker_set_model(struct task *task, struct walker *walker,
                             int index);
static Bool walker_stand(struct task *task);
static Bool walker_talk(struct task *task);
static Bool walker_knocked(struct task *task);
static void walker_react(struct walker_work *work, struct walker *walker);
static Bool walker_play_move(struct task *task);
static void walker_run(struct task *task);
static void walker_step(struct task *task);
static void walker_wait(struct task *task);
static void walker_draw(struct task *task);
static void walker_destroy(struct task *task);
static void walker_route_destroy(struct task *task);

static const struct walker_model walker_models[WALKER_MODELS] = {
    {&dat_0cb2e33c, &dat_0cb396d4, 0},
    {&dat_0cb26a34, &dat_0cb396d4, 1},
    {&dat_0cb298cc, &dat_0cb396d4, 1},
};

/* the jungle's walkers, by path */
static const struct walker_spec jungle_walkers[5] = {
    {1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 2, 0}, {1, 0, 3, 0}, {2, 0, 4, 0},
};

static struct walker_kind walker_kinds[2] = {
    {WALKER_MOVE_SET, 8.7F, 12.5F, 0.33F, WALKER_SHAPE(7.8F, 3.4F, 6.0999999F)},
    {WALKER_MOVE_SET, 7.15F, 11.8F, 0.3F, WALKER_SHAPE(7.0F, 3.8F, 5.1F)},
};

/* each character's message file, by character number. Eggman and Tikal have
 * none */
static char *message_files[CHARACTER_COUNT] = {
    "mr_mes_s.bin", NULL,           "mr_mes_m.bin", "mr_mes_k.bin",
    NULL,           "mr_mes_a.bin", "mr_mes_e.bin", "mr_mes_b.bin",
};

static char *jungle_path_file = "path_mr.bin";

static char *message_file;

void mystic_ruins_messages_load(void)
{
    struct task *task;
    struct crowd *crowd;

    message_file = event_flags[FLAG_LAST_STORY]
                       ? "mr_mes_l.bin"
                       : message_files[get_character()];
    if (loader_read_file(message_file, &field_messages) != -1) {
        script_set_lines_table(field_messages.table);
    } else {
        script_set_lines_table(NULL);
    }
    script_set_table((struct script_row *)mystic_ruins_script_table);
    if ((task = task_create(0, 1, crowd_step)) != NULL) {
        crowd = heap_calloc(1, sizeof(struct crowd));
        if (crowd) {
            task->extra = crowd;
            crowd_step(task);
        } else {
            task_destroy(task);
        }
    }
}

void crowd_person_init(struct task *task)
{
    struct walker_work *work = WALKER(task);
    struct walker *walker;

    /* walker_work must keep task_work's own fields where task_work does */
    /* @hack redundant: pos tested a second time (ADD #32,R2 at 0x0c91246e and
     * 0x0c9124aa); tested once, 70 of 82 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (WORK_SAME_PLACE(struct walker_work, pos) &&
        WORK_SAME_PLACE(struct walker_work, id) &&
        WORK_SAME_PLACE(struct walker_work, collision) &&
        WORK_SAME_PLACE(struct walker_work, event) &&
        WORK_SAME_PLACE(struct walker_work, pos) &&
        WORK_SAME_PLACE(struct walker_work, flags)) {
        if (work->row < field_messages.people &&
            walker_init(task, MOVE_STAND)) {
            walker = task->extra;
            walker->update = crowd_person_update;
            task->destructor = walker_destroy;
        } else if (edit_mode) {
            walker_draw_plain(task);
        }
    }
    /* NOLINTEND(misc-redundant-expression) */
}

/* create the jungle's route task, read path_mr.bin's paths, and start a
 * walker on each path. it returns NULL in any other act or for an empty file.
 * it returns NULL too when the paths' buffer or the task can't be allocated,
 * or the file can't be opened. a failed allocation of the route goes
 * unchecked */
static struct task *crowd_route_create(int act)
{
    int size;
    int alloc;
    struct walk_path *paths;
    struct walk_path *path;
    struct walk_route *route;
    struct task *task;

    if (act == MYSTIC_RUINS_JUNGLE) {
        size = loader_file_size(jungle_path_file);
        /* @hack redundant: a jump to the next line, which has SHC load
         * __modls's address into R3 and __divls's into R2 (0x0c91257e,
         * 0x0c9125c2); without it, the two trade, and 18 of 92 words differ */
        goto round_size;
    round_size:
        /* the paths take whole sectors, as loader_read_file reads them */
        alloc = size % SECTOR_SIZE ? 1 : 0;
        alloc += size / SECTOR_SIZE;
        alloc *= SECTOR_SIZE;
        /* @hack redundant: the rest in an else after the return, which keeps
         * the code below in the image's registers, as heap_alloc's address
         * in R2 at 0x0c9125d4; without it, R2 and R3 trade from there to
         * 0x0c912614 and 14 of 92 words differ */
        if (alloc <= 0) {
            return NULL;
        } else {
            /* @hack redundant: a select whose arms are both alloc, which keeps
             * alloc in R14 (0x0c912592, 0x0c9125c0); as heap_alloc(alloc), SHC
             * builds it in R4, heap_alloc's argument, and 27 of 92 words
             * differ */
            /* NOLINTNEXTLINE(misc-redundant-expression) */
            paths = heap_alloc(1.0F ? alloc : alloc);
            route = heap_calloc(1, sizeof(struct walk_route));
            if (paths) {
                task = task_create(0, 3, route_step);
                if (task) {
                    task->extra = route;
                    route->paths = paths;
                    task->destructor = route_destroy;
                    /* @hack redundant: the destroy in an else after the return,
                     * which keeps the three calls from there in the image's
                     * registers, as task_destroy's address in R3 at
                     * 0x0c91262e; without it, R2 and R3 trade in each and 6
                     * of 92 words differ */
                    if (loader_read_file(jungle_path_file, paths) != -1) {
                        for (path = paths; path->points; path++) {
                            path->points = (NJS_POINT3 *)((char *)paths +
                                                          (int)path->points);
                        }
                        route_fill(task);
                        return task;
                    } else {
                        task_destroy(task);
                    }
                }
            }
            /* @bug memory: frees paths and route a second time when the
             * file's open fails, by the JSRs at 0x0c912636 and 0x0c91263c.
             * task_destroy has already freed route as the task's extra, and
             * paths through route_destroy */
            heap_free(paths);
            heap_free(route);
        }
    }
    return NULL;
}

static void crowd_start(struct crowd *crowd)
{
    int act = (Uint8)stage_get_stage_act();

    mystic_ruins_people_place();
    crowd->route = crowd_route_create(act);
    crowd->act = act;
    crowd->started = TRUE;
}

/* when the act changes, remove the people and the route, so that the next step
 * starts the new act's */
static void crowd_leave_act(struct crowd *crowd)
{
    if (crowd->act != (Uint8)stage_get_stage_act()) {
        mystic_ruins_people_remove();
        if (crowd->route) {
            task_destroy(crowd->route);
            crowd->route = NULL;
        }
        crowd->started = FALSE;
    }
}

static void crowd_step(struct task *task)
{
    struct crowd *crowd = task->extra;

    if (!crowd->started) {
        crowd_start(crowd);
    } else {
        crowd_leave_act(crowd);
    }
}

/* draw the person's model without motion, for the set editor, and label its
 * rows while the editor edits its set entry */
static void walker_draw_plain(struct task *task)
{
    struct walker_work *work = WALKER(task);
    const struct walker_model *model;
    int index;
    Angle ang;

    njPushMatrix(NULL);
    index = work->model;
    if (index < 0 || index >= WALKER_MODELS) {
        index = 0;
    }
    model = &walker_models[index];
    njTranslateV(NULL, &work->pos);
    njTranslate(NULL, 0.0F, walker_kinds[model->kind].lift, 0.0F);
    TURN_VIA(njRotateY, work->ang_y, ang);
    njSetTexture(model->texlist);
    draw_tree(model->object);
    njPopMatrix(1);
    if (placement_editing(task)) {
        walker_print(task);
    }
}

/* label the set editor's rows for O PEOPLE */
static void walker_print(struct task *task)
{
    struct walker_work *work = WALKER(task);

    njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "<-MODEL INDEX");
    njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "<-ROTATE Y");
    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<-Range Out(SQR)");
    njPrint(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<-Message ID = %d",
            (int)work->row);
}

/* start jungle_walkers[index] on path as a child of the route task, at the
 * path's first point and walking forward */
static void route_add_walker(struct task *task, struct walk_path *path,
                             int index)
{
    struct task *child = task_create_child(TASK_WORK, walker_step, task);
    struct walker_start start;
    const struct walker_spec *spec;

    if (child) {
        spec = &jungle_walkers[index];
        start.model = spec->model;
        start.row = spec->row + field_messages.people;
        start.along = 0;
        start.way = 1;
        walker_create(child, path->points, path->count, &start);
    }
}

/* start a walker on each of the route's paths. the loop tests for the null
 * points that end the paths after each walker, not before the first */
static void route_fill(struct task *task)
{
    int index = 0;
    struct walk_path *path = ((struct walk_route *)task->extra)->paths;

    do {
        route_add_walker(task, path, index);
        path++;
        index++;
    } while (path->points);
}

/* while an event has control, step the walkers only while FLAG_CROWD_IN_EVENT
 * is set. once the event ends, replace them with new walkers */
static void route_wait(struct task *task)
{
    if (!event_in_control()) {
        task_destroy_children(task);
        route_fill(task);
        task->step = route_step;
    } else if (event_flags[FLAG_CROWD_IN_EVENT]) {
        task_step_children(task);
    }
}

static void route_step(struct task *task)
{
    if (event_in_control()) {
        task->step = route_wait;
        return;
    }
    task_step_children(task);
}

static void route_destroy(struct task *task)
{
    struct walk_route *route = task->extra;

    heap_free(route->paths);
}

/* store points[point] in out. with out NULL, it moves the walker to
 * points[point + way] instead and turns it. no call passes NULL */
static void walker_place_at(struct walker_work *work, NJS_POINT3 *out,
                            int point, int way)
{
    NJS_POINT3 here;
    NJS_POINT3 next;

    if (out) {
        *out = work->points[point];
    } else {
        /* @bug uninitialized: ang_y's arc tangent reads next.x and next.z
         * unset, at 0x0c9129ba and 0x0c9129cc. both points are copied into
         * here, the second over the first, and nothing writes next. no call
         * passes NULL, so the read never runs */
        here = work->points[point];
#ifdef GCC
        /* repairs the copy the tag names: next takes the second point, so
         * the walker stands on the first and faces the second */
        next = work->points[point + way];
#else
        here = work->points[point + way];
#endif
        work->pos = here;
        /* NOLINTNEXTLINE(clang-diagnostic-uninitialized) */
        work->ang_y = angle_arc_tan2(next.z - here.z, -next.x + here.x);
    }
}

/* store in out the position for along. along up to 1 gives points[1], along
 * past count - 2 gives points[count - 2], and along on a point between gives
 * that point. it returns the index of the point after that one in direction
 * way. with out NULL, it moves and turns the walker instead, but no call
 * passes NULL. walker_head_for, the only caller, asks only at a path's end,
 * where one of the three cases holds */
static int walker_find_point(struct walker_work *work, NJS_POINT3 *out,
                             Float along, int way)
{
    int count = work->count;
    int index = 0;
    NJS_POINT3 to;
    NJS_POINT3 from;
    Float x;
    Float z;
    Float before_x;
    Float before_z;
    Float after_x;
    Float after_z;
    NJS_POINT3 *after;
    NJS_POINT3 *before;

    while (index < count && along > index) {
        index++;
    }
    if (index <= 1) {
        walker_place_at(work, out, 1, way);
        return way + 1;
    }
    if (index >= count - 1) {
        walker_place_at(work, out, count - 2, way);
        return way + count - 2;
    }
    if (along == index) {
        walker_place_at(work, out, index, way);
        return way + index;
    }
    /* no call reaches the code below. along - index lies between -1 and 0,
     * which puts x and z past points[index], on the side away from
     * points[index - 1] */
    along -= index;
    after = &work->points[index];
    before = after - 1;
    from = *before;
    to = *after;
    before_x = from.x;
    after_x = to.x;
    /* @hack spelling: after_z assigned ahead of before_z, which has SHC load
     * before_z first (FMOV.S @(R0,R6),FR13 at 0x0c912ace); assigned the other
     * way, the two loads trade, and 2 of 155 words differ */
    after_z = to.z;
    before_z = from.z;
    x = after_x + ((before_x - after_x) * along);
    /* @hack spelling: z built in two statements, which keeps before_x in FR14
     * and before_z in FR13 (0x0c912ac6, 0x0c912ace); in one, the two trade,
     * as do R6 and R4, which address from and to, and 26 of 155 words
     * differ */
    z = after_z;
    z += (before_z - after_z) * along;
    /* @hack redundant: a never-run test of after_z, x and z, which gives
     * after_z FR5, x FR6 and z FR7 (0x0c912ad0 to 0x0c912ae2), as the image has
     * them; without it, z takes FR5 and 10 of 155 words differ */
    if (0) {
        if (after_z + after_z + after_z + x + x + x + z + z) {
        }
    }
    if (out) {
        out->x = x;
        /* the rise from the walker to the position would add to.y, which this
         * leaves out (0x0c912ae8 to 0x0c912af8) */
        out->y = ((from.y - to.y) * along) - work->pos.y;
        out->z = z;
    } else {
        work->pos.x = x;
        work->pos.z = z;
        work->pos.y = to.y + ((from.y - to.y) * along);
        /* the angle's first argument is before_z - after_x. a z difference
         * belongs there, before_z - after_z. the angle is then multiplied by
         * way */
        work->ang_y =
            angle_arc_tan2(before_z - after_x, after_x - before_x) * way;
    }
    return way == 1 ? index : index - 1;
}

/* put the walker on points[point], aimed at that point, and make
 * walker_update its update. walker_walk aims it at the next point on its first
 * step */
static void walker_start_walking(struct task *task, struct walker_place *place,
                                 NJS_POINT3 *points, int point)
{
    struct walker_work *work = WALKER(task);
    struct walker *walker = task->extra;

    work->pos = points[point];
    walker_aim(place, point, &points[point], &work->pos, place->along,
               walker->kind->speed * WALK_STRIDE);
    walker->place = place;
    walker->update = walker_update;
}

/* start the path walker task at points[0]'s x and z, at height 0, walking as
 * start gives it, and return its place on the path. when an allocation fails
 * or the walker's row picks no lines, it destroys the task and returns
 * NULL */
static struct walker_place *walker_create(struct task *task, NJS_POINT3 *points,
                                          int count, struct walker_start *start)
{
    struct walker_work *work = WALKER(task);
    struct walker_place *place = heap_calloc(1, sizeof(struct walker_place));

    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    work->model = start->model;
    work->row = start->row;
    /* @hack redundant: the failure in an else after the return, which keeps
     * heap_free's address in R3 at 0x0c912c5c and task_destroy's in R2;
     * without it, the two trade and 4 of 76 words differ */
    if (place && walker_init(task, MOVE_WALK)) {
        /* @hack spelling: way through a local, which has the byte go by R4 to
         * @(R0,R14) (MOV R0,R4 at 0x0c912c22, MOV.B R4,@(R0,R14) at
         * 0x0c912c26); stored directly, 5 of 76 words differ */
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        int way = start->way;

        place->way = way;
        place->along = start->along;
        place->pace = 1.0F;
        work->points = points;
        work->count = count;
        place->point = 0;
        walker_start_walking(task, place, points, 0);
        work->pos.y = 0.0F;
        task->destructor = walker_route_destroy;
        return place;
    } else {
        heap_free(place);
        task_destroy(task);
        return NULL;
    }
}

/* walk toward each path point in turn, reversing at either end, on a frame's
 * first step only. on a frame's later steps, and on the step it reverses at
 * an end, it returns TRUE. on any other, it returns FALSE once player 0 is
 * beyond the walker's reach, and TRUE otherwise */
static Bool walker_walk(struct task *task)
{
    struct walker_work *work = WALKER(task);
    struct walker *walker = task->extra;
    struct walker_kind *kind;
    struct walker_place *place;
    struct walker_move *move;
    NJS_POINT3 *points;
    NJS_POINT3 pos;
    Float speed;
    Float pace;
    Float distance2;
    int point;
    int next;

    if (frame_step) {
        return TRUE;
    }
    place = walker->place;
    kind = walker->kind;
    speed = kind->speed;
    /* @hack spelling: the stride multiplied in a statement of its own, which
     * has the test of along below take FR1 and FR2 (FMOV.S @(R0,R14),FR1 at
     * 0x0c912ccc, FLOAT FPUL,FR2 at 0x0c912cce); as one statement, it takes
     * FR2 and FR3, and 3 of 183 words differ */
    speed *= WALK_STRIDE;
    point = place->point;
    move = &walker->kind->moves[walker->move];
    points = work->points;
    if ((place->way == 1 && place->along >= point) ||
        (place->way != 1 && point >= place->along)) {
        if ((next = walk_next_point(point, place->way, work->count)) < 0) {
            place->way = -place->way;
            walker_head_for(work, place, walker, place->along + place->way);
            return TRUE;
        }
        walker_aim(place, next, &points[next], &work->pos, place->along, speed);
        place->point = next;
    }
    pace = place->pace;
    /* @hack redundant: a never-run test of pace, which gives pace FR15 and
     * speed FR14 (0x0c912d3c, 0x0c912ca8), as the image has them; without it,
     * the two trade and 9 of 183 words differ */
    if (0) {
        if (pace) {
        }
    }
    walker_go(&work->pos, &place->target, &work->ang_y, speed * pace,
              pace * place->advance);
    if ((work->frame += MOVE_TIME * (pace * move->speed)) >
        (Float)move->motion->nbFrame - 1.0F) {
        work->frame = 0.0F;
    }
    place->along += pace * place->advance;
    player_get_history(0, 0, &pos, NULL);
    /* @hack spelling: distance2 set through its address, which puts it at sp+4
     * through R8 (MOV R2,R8 at 0x0c912db2, FMOV.S FR0,@R8 at 0x0c912dbc);
     * plain, the object is 32 bytes shorter, and 178 of 183 words differ */
    *&distance2 = point_distance_squared_across(&pos, &work->pos);
    if (distance2 > work->reach2) {
        return FALSE;
    }
    return TRUE;
}

/* the next point index in direction way, or -1 past either end of the path */
static int walk_next_point(int point, int way, int count)
{
    point += way;
    if (point < 0) {
        return -1;
    }
    if (point >= count) {
        return -1;
    }
    return point;
}

/* aim the walker at point, points[index] of its path, with advance set so
 * that along reaches index as the walker arrives. the target's y is the rise
 * from pos to point */
static void walker_aim(struct walker_place *place, int index, NJS_POINT3 *point,
                       NJS_POINT3 *pos, Float along, Float speed)
{
    Float left = index - along;
    Float x = point->x;
    Float y = point->y;
    Float z = point->z;

    place->advance = left * speed *
                     _builtin_fsrra(((x - pos->x) * (x - pos->x)) +
                                    ((z - pos->z) * (z - pos->z)));
    place->target.x = x;
    place->target.z = z;
    place->target.y = y - pos->y;
}

/* turn the walker toward its target and move it speed units ahead, climbing
 * advance times target->y. between its moves in x and in z, it may settle on
 * a floor, as SETTLE_ABOVE and SETTLE_REACH say */
static void walker_go(NJS_POINT3 *pos, NJS_POINT3 *target, Angle *ang,
                      Float speed, Float advance)
{
    Float sine;
    Float cosine;
    Float ground;
    Angle turned = *ang;
    struct angles slope;

    turned += walker_turn(pos, target, turned);
    _builtin_fsca(turned, &sine, &cosine);
    pos->x -= speed * cosine;
    pos->y += advance * target->y;
    ground = ground_below(pos->x, pos->y + SETTLE_ABOVE, pos->z, &slope);
    if ((ground - pos->y >= 0.0F ? ground - pos->y : pos->y - ground) <
        SETTLE_REACH) {
        pos->y = ground;
    }
    pos->z += speed * sine;
    *ang = turned;
}

/* the turn toward the target, at most TURN_LIMIT either way */
static Angle walker_turn(NJS_POINT3 *pos, NJS_POINT3 *target, Angle ang)
{
    Sint16 turn =
        (Sint16)(angle_arc_tan2(target->z - pos->z, -target->x + pos->x) - ang);

    /* @hack redundant: the rest in an else after the return, which with the
     * select below has the return take TURN_LIMIT from R2 (MOV R2,R0 at
     * 0x0c912fb6); without it, SHC loads the pool word again, and 1 of 50
     * words differ */
    if ((turn >= 0 ? turn : -turn) < TURN_LIMIT) {
        return turn;
    } else {
        if (turn >= 0) {
            /* @hack redundant: a select whose arms are both TURN_LIMIT, which
             * has the return take TURN_LIMIT from R2, loaded for the test
             * above (MOV R2,R0 at 0x0c912fb6); as return TURN_LIMIT, SHC
             * loads the pool word again, and 1 of 50 words differ */
            /* NOLINTNEXTLINE(misc-redundant-expression) */
            return 1.0F ? TURN_LIMIT : TURN_LIMIT;
        }
        return -TURN_LIMIT;
    }
}

/* aim the walker at walker_find_point's position for along, starting a switch
 * that ends once the walker passes along */
static void walker_head_for(struct walker_work *work,
                            struct walker_place *place, struct walker *walker,
                            Float along)
{
    Float advance;

    place->point = walker_find_point(work, &place->target, along, place->way);
    place->switch_end = along;
    place->switching = TRUE;
    advance = along - place->along;
    /* @hack spelling: the speed multiplied in a statement of its own, which
     * keeps advance in FR4 (FMOV FR15,FR4 at 0x0c912fe8); as one statement,
     * advance takes FR5, and 24 of 55 words differ */
    advance *= walker->kind->speed;
    place->advance = advance *
                     _builtin_fsrra(((place->target.x - work->pos.x) *
                                     (place->target.x - work->pos.x)) +
                                    ((place->target.z - work->pos.z) *
                                     (place->target.z - work->pos.z))) *
                     WALK_STRIDE;
}

/* a path walker's update. while the next move is MOVE_KNOCKED or MOVE_TALK,
 * or the walker is beyond its reach, it returns FALSE. otherwise it ends a
 * switch once the walker passes switch_end, and stops the walker or walks it
 * on as STOP_GAP and GO_GAP say. it returns FALSE on a step that switches the
 * walker between walking and standing. on any other it returns TRUE, so that
 * walker_react can start a knock or a talk */
static Bool walker_update(struct task *task)
{
    struct walker_work *work = WALKER(task);
    struct walker *walker = task->extra;
    struct walker_place *place;
    struct player *player;
    NJS_POINT3 pos;
    Float radius;
    Float reach;

    if (walker->next_move == MOVE_KNOCKED || walker->next_move == MOVE_TALK ||
        work->distance2 > work->reach2) {
        return FALSE;
    }
    place = walker->place;
    if (place->switching &&
        (place->switch_end - place->along) * place->way < 0.0F) {
        walker_aim(place, place->point, &work->points[place->point], &work->pos,
                   place->along, walker->kind->speed * WALK_STRIDE);
        place->switching = FALSE;
    }
    player = player_get(0);
    radius = player ? player->consts.radius : 0.0F;
    player_get_history(0, 0, &pos, NULL);
    if (walker->next_move == MOVE_WALK) {
        reach = radius + STOP_GAP;
        if (work->distance2 < reach * reach) {
            walker->next_move = MOVE_STAND;
        } else {
            return TRUE;
        }
    } else {
        reach = radius + GO_GAP;
        if (work->distance2 > reach * reach) {
            walker->next_move = MOVE_WALK;
        } else {
            return TRUE;
        }
    }
    walker->blend = BLEND_STOP;
    walker->last_frame = work->frame;
    work->frame = 0.0F;
    return FALSE;
}

/* the update of one of the set's people, who only stand. it labels the set
 * entry's rows while the editor edits it, and returns TRUE so that
 * walker_react can start a knock or a talk */
static Bool crowd_person_update(struct task *task)
{
    if (placement_editing(task)) {
        walker_print(task);
    }
    return TRUE;
}

/* allocate the walker's record and its lines, and give the walker its model
 * and shape. start it in move, and register it as a talk for player 0. it
 * returns TRUE once started. when an allocation fails or the walker's row
 * picks no lines, it frees what it allocated and returns FALSE */
static Bool walker_init(struct task *task, int move)
{
    struct walker_work *work = WALKER(task);
    struct walker *walker = heap_calloc(1, sizeof(struct walker));
    struct script_lines *lines = heap_alloc(sizeof(struct script_lines));

    if (walker && lines && script_pick_lines((int)work->row, lines)) {
        task->extra = walker;
        walker_set_model(task, walker, work->model);
        work->frame = 0.0F;
        work->model = -1;
        walker->move = move;
        walker->next_move = move;
        walker->lines = lines;
        walker->animate = TRUE;
        talk_add(task, &work->pos, &work->ang_y, &work->talking,
                 walker->kind->shape.radius + TALK_REACH, TALK_LEVEL);
        task->step = walker_step;
        task->draw = walker_draw;
        return TRUE;
    }
    heap_free(walker);
    heap_free(lines);
    return FALSE;
}

/* set the walker's model, texture list and kind from walker_models[index], or
 * from walker_models[0] for an index out of range. it gives the task a
 * collision record of the kind's shape. a walker with no reach of its own
 * takes WALKER_REACH */
static void walker_set_model(struct task *task, struct walker *walker,
                             int index)
{
    const struct walker_model *model;
    struct walker_kind *kind;
    struct walker_work *work;
    int number;

    if (index < 0 || index >= WALKER_MODELS) {
        index = 0;
    }
    model = &walker_models[index];
    /* @hack spelling: the kind's number read ahead of the stores of the model
     * and texture list, which loads it first (MOV.L @(8,R4),R14 at
     * 0x0c9132ea); read where kind is set, 10 of 59 words differ */
    number = model->kind;
    walker->object = model->object;
    walker->texlist = model->texlist;
    kind = &walker_kinds[number];
    walker->kind = kind;
    collision_init(task, &kind->shape, COLLISION_OBJECT, 1, 1);
    /* @hack spelling: the work read after collision_init, from the copy of task
     * at sp+0 (MOV.L @R15,R4 at 0x0c913316); read at the top, the object is
     * 32 bytes shorter, and 59 of 59 words differ */
    work = WALKER(task);
    if (work->reach2 < NO_REACH2) {
        work->reach2 = WALKER_REACH * WALKER_REACH;
    }
}

/* play the standing move over and over. it returns TRUE, but FALSE outside
 * the set editor once player 0 is beyond the walker's reach. outside the
 * editor, the walker draws its plain model while player 0 is beyond
 * WALKER_NEAR of it */
static Bool walker_stand(struct task *task)
{
    struct walker_work *work = WALKER(task);
    struct walker *walker = task->extra;
    struct walker_move *move;
    struct walker_kind *kind;
    Float distance2;

    if (!placement_editing(task)) {
        distance2 = work->distance2;
        if (distance2 > work->reach2) {
            return FALSE;
        }
        walker->animate = distance2 < WALKER_NEAR * WALKER_NEAR;
    }
    kind = walker->kind;
    move = &kind->moves[walker->move];
    if ((work->frame += MOVE_TIME * move->speed) >
        (Float)move->motion->nbFrame - 1.0F) {
        if (rand() * RAND_SCALE < STAND_PICK) {
            walker->next_move = MOVE_STAND;
        }
        walker->last_frame = work->frame - move->speed;
        walker->blend = BLEND_MOVE;
        work->frame = 0.0F;
    }
    return TRUE;
}

/* play the talking move until the lines stop showing. then end the talk,
 * stop player 0 looking at the walker, and stand */
static Bool walker_talk(struct task *task)
{
    struct walker_work *work = WALKER(task);
    struct walker *walker = task->extra;
    struct walker_move *move;

    if (walker->next_move == MOVE_TALK && !work->shown) {
        talk_end();
        player_clear_look(0);
        walker->next_move = MOVE_STAND;
        walker->last_frame = work->frame;
        walker->blend = BLEND_MOVE;
        work->frame = 0.0F;
    }
    move = &walker->kind->moves[walker->move];
    if ((work->frame += MOVE_TIME * move->speed) >
        (Float)move->motion->nbFrame - 1.0F) {
        work->frame = 0.0F;
    }
    return TRUE;
}

/* play MOVE_KNOCKED once, then return to the move before it */
static Bool walker_knocked(struct task *task)
{
    struct walker_work *work = WALKER(task);
    struct walker *walker = task->extra;
    struct walker_move *move = &walker->kind->moves[walker->move];

    if ((work->frame += MOVE_TIME * move->speed) >
        (Float)move->motion->nbFrame - 1.0F) {
        walker->next_move = walker->return_move;
        walker->last_frame = work->frame - move->speed;
        walker->blend = BLEND_MOVE;
        work->frame = 0.0F;
    }
    return TRUE;
}

/* start MOVE_KNOCKED when a player with PLAYER_GUARD set hits the walker.
 * when player 0 talks to it, start MOVE_TALK and show its lines. the row
 * picks new lines, and where it picks none the walker keeps its old ones.
 * player 0 then looks at the point kind->eye above the walker's position */
static void walker_react(struct walker_work *work, struct walker *walker)
{
    struct script_lines *lines;
    NJS_POINT3 pos;

    if (walker->next_move == MOVE_KNOCKED) {
        return;
    }
    if ((work->collision->flags & COLLISION_HIT) &&
        work->collision->hit->kind == COLLISION_PLAYER &&
        (work->collision->hit->owner->work->flags & PLAYER_GUARD)) {
        talk_end();
        walker->last_frame = work->frame;
        work->frame = 0.0F;
        walker->blend = BLEND_KNOCKED;
        walker->return_move = walker->next_move;
        walker->next_move = MOVE_KNOCKED;
    } else if (work->talking) {
        walker->last_frame = work->frame;
        work->frame = 0.0F;
        walker->blend = BLEND_MOVE;
        walker->next_move = MOVE_TALK;
        lines = heap_alloc(sizeof(struct script_lines));
        if (lines && script_pick_lines((int)work->row, lines)) {
            heap_free(walker->lines);
            walker->lines = lines;
        } else {
            heap_free(lines);
        }
        message_set_shown(&work->shown);
        script_show_lines(walker->lines);
        pos.x = work->pos.x;
        pos.y = work->pos.y + walker->kind->eye;
        pos.z = work->pos.z;
        player_set_look_point(0, &pos);
        work->talking = FALSE;
    }
}

static Bool (*const walker_move_steps[WALKER_MOVES])(struct task *task) = {
    walker_stand,
    walker_talk,
    walker_walk,
    walker_knocked,
};

static Bool walker_play_move(struct task *task)
{
    struct walker *walker = task->extra;
    /* @hack spelling: the move through a local, which SHC keeps at sp+0
     * (MOV.L R3,@R15 at 0x0c91363c); without it, sp+0 holds the walker, and
     * 5 of 10 words differ */
    int move = walker->move;

    return walker_move_steps[move](task);
}

/* measure player 0's distance, run the walker's update, and let the walker
 * react unless the update returns FALSE. then play its move, or advance the
 * blend into the next move. a move that returns FALSE ends the step there;
 * otherwise the walker draws and registers its collision */
static void walker_run(struct task *task)
{
    struct walker_work *work = WALKER(task);
    struct walker *walker = task->extra;
    Bool (*update)(struct task *task);
    NJS_POINT3 pos;
    Float frame;

    player_get_history(0, 0, &pos, NULL);
    /* @hack spelling: work->distance2 set through its address, which SHC forms
     * in R8 before the call and stores through after it (ADD #28,R2 and MOV
     * R2,R8 at 0x0c91366a and 0x0c91366e, FMOV.S FR0,@R8 at 0x0c913676);
     * without it, 70 of 73 words differ */
    *&work->distance2 = point_distance_squared_across(&pos, &work->pos);
    /* @hack redundant: a jump to the next line, which loads walker->update into
     * R2 (MOV.L @(40,R14),R2 at 0x0c913678); without it, walker->update takes
     * R3, and 3 of 73 words differ */
    goto update_move;
update_move:
    if (!(update = walker->update) || update(task)) {
        walker_react(work, walker);
    }
    if (walker->move == walker->next_move) {
        if (!walker_play_move(task)) {
            return;
        }
    } else {
        /* @hack spelling: the blend added in a statement of its own, which
         * loads the frame ahead of the blend (FMOV.S @(R0,R13),FR4 at
         * 0x0c9136a6) and gives the test below a delayed branch (BF/S at
         * 0x0c9136b2); in one statement, the blend loads first, and 24 of 73
         * words differ */
        frame = work->frame;
        frame += walker->blend;
        if (frame > 1.0F) {
            walker->move = walker->next_move;
            frame = 0.0F;
        }
        work->frame = frame;
    }
    walker_draw(task);
    collision_register((struct task_work *)work);
}

/* while the set editor edits a person, apply a new model index from its set
 * entry. while an event has control, the walker waits in walker_wait */
static void walker_step(struct task *task)
{
    struct walker_work *work = WALKER(task);
    struct walker *walker = task->extra;

    if (placement_editing(task)) {
        Float row = work->row;

        if (row < field_messages.people && row >= 0.0F && work->model >= 0) {
            collision_free((struct task_work *)work);
            walker_set_model(task, walker, work->model);
            work->frame = 0.0F;
            work->model = -1;
        }
    }
    if (event_in_control()) {
        task->step = walker_wait;
    } else {
        walker_run(task);
    }
}

/* while an event has control, run the walker only while FLAG_CROWD_IN_EVENT
 * is set. after the event, pick new lines, or destroy the walker when its row
 * picks none */
static void walker_wait(struct task *task)
{
    if (!event_in_control()) {
        struct walker_work *work = WALKER(task);
        struct walker *walker = task->extra;

        if (!script_pick_lines((int)work->row, walker->lines)) {
            task_destroy(task);
        } else {
            task->step = walker_step;
        }
    } else if (event_flags[FLAG_CROWD_IN_EVENT]) {
        walker_run(task);
    }
}

/* draw the walker and its shadow, blending between moves, on a frame's first
 * step only. it draws nothing out of view, beyond the walker's reach, or while
 * an event has control and FLAG_CROWD_IN_EVENT is clear */
static void walker_draw(struct task *task)
{
    struct walker_work *work = WALKER(task);
    struct walker *walker;
    struct walker_kind *kind;
    Angle ang;
    NJS_OBJECT *object;
    Float frame;
    NJS_MOTION_LINK link;
    struct walker_work *drawn;

    if (frame_step) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, VIEW_RADIUS)) {
        return;
    }
    if (event_in_control() && !event_flags[FLAG_CROWD_IN_EVENT]) {
        return;
    }
    if (work->distance2 > work->reach2) {
        return;
    }
    njPushMatrix(NULL);
    walker = task->extra;
    kind = walker->kind;
    /* @hack spelling: the work read from task again after the push, as drawn
     * (MOV.L @(32,R12),R12 at 0x0c91389a); through work, 132 of 139 words
     * differ */
    drawn = WALKER(task);
    njTranslateV(NULL, &drawn->pos);
    mystic_ruins_shadow_draw_person(kind->shape.radius);
    njTranslate(NULL, 0.0F, kind->lift, 0.0F);
    TURN_VIA(njRotateY, drawn->ang_y, ang);
    njSetTexture(walker->texlist);
    draw_set_palette(DRAW_PLAYER_PALETTE);
    object = walker->object;
    if (walker->animate) {
        frame = drawn->frame;
        if (walker->move == walker->next_move) {
            draw_motion(object, kind->moves[walker->move].motion, frame);
        } else {
            /* @hack spelling: both motions through locals, which has SHC
             * load both (MOV.L @R5,R5 at 0x0c913916, MOV.L @R6,R6 at
             * 0x0c91391a) before link takes either; assigned directly, 16 of
             * 139 words differ */
            NJS_MOTION *from = kind->moves[walker->move].motion;
            NJS_MOTION *to = kind->moves[walker->next_move].motion;

            link.motion[0] = from;
            link.motion[1] = to;
            link.frame[0] = walker->last_frame;
            link.frame[1] = 0.0F;
            draw_motion_link(object, &link, frame);
        }
    } else {
        draw_tree(object);
    }
    draw_set_palette(0);
    njPopMatrix(1);
}

static void walker_destroy(struct task *task)
{
    struct walker *walker = task->extra;

    heap_free(walker->lines);
    talk_remove(task);
    /* @hack redundant: a return where control returns anyway, which has
     * walker_route_destroy branch here (BRA at 0x0c91398c); without it, SHC
     * expands this function there, the object is 32 bytes longer, and 2 of
     * 15 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

static void walker_route_destroy(struct task *task)
{
    struct walker *walker = task->extra;

    heap_free(walker->place);
    walker_destroy(task);
}
