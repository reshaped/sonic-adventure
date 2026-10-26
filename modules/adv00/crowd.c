/* the people the object list names O PEOPLE, who stand where the set places
 * them, and the walkers on the routes of the office, casino and station acts */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv00/crowd.h"
#include "adv00/people.h"
#include "adv00/shadow.h"
#include "adv00/texlists.h"
#include "collision/collision.h"
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

/* the walkers' models and motions */
extern NJS_OBJECT dat_0caee8e4;
extern NJS_OBJECT dat_0caf12b8;
extern NJS_OBJECT dat_0caf3924;
extern NJS_OBJECT dat_0caf6850;
extern NJS_OBJECT dat_0cb06cd8;
extern NJS_OBJECT dat_0cb09ce4;
extern NJS_OBJECT dat_0cae0360;
extern NJS_OBJECT dat_0cae3e38;
extern NJS_OBJECT dat_0cae679c;
extern NJS_OBJECT dat_0cae9ee0;
extern NJS_OBJECT dat_0cafa58c;
extern NJS_OBJECT dat_0cafc128;
extern NJS_OBJECT dat_0cafdfc0;
extern NJS_OBJECT dat_0cb029cc;
extern NJS_OBJECT dat_0cad0ffc;
extern NJS_OBJECT dat_0cad4494;
extern NJS_OBJECT dat_0cad7e9c;
extern NJS_OBJECT dat_0cadb7dc;
extern NJS_OBJECT mannequin_object;
extern NJS_OBJECT dat_0cac6330;
extern NJS_OBJECT dat_0cac8a7c;
extern NJS_OBJECT dat_0cacc504;
extern NJS_OBJECT dat_0cabaec8;
extern NJS_OBJECT dat_0cabd234;
extern NJS_OBJECT dat_0cabf974;
extern NJS_OBJECT dat_0cac17ec;
extern NJS_MOTION dat_0cb0c214;
extern NJS_MOTION dat_0cb0d3a0;
extern NJS_MOTION dat_0cb0dbfc;
extern NJS_MOTION dat_0cb0b2c8;
extern NJS_MOTION dat_0cb0f878;
extern NJS_MOTION dat_0cb11eb0;
extern NJS_MOTION dat_0cb12f8c;
extern NJS_MOTION dat_0cb13c58;
extern NJS_MOTION dat_0cb14934;
extern NJS_MOTION dat_0cb10e24;
extern NJS_MOTION dat_0cb16430;
extern NJS_MOTION dat_0cb18618;
extern NJS_MOTION dat_0cb19464;
extern NJS_MOTION dat_0cb1a1f0;
extern NJS_MOTION dat_0cb1ae4c;
extern NJS_MOTION dat_0cb1ba28;
extern NJS_MOTION dat_0cb179bc;
extern NJS_MOTION dat_0cb1d6e4;
extern NJS_MOTION dat_0cb1eb50;
extern NJS_MOTION dat_0cb22d14;
extern NJS_MOTION mannequin_motion;
extern NJS_MOTION dat_0cb25a4c;
extern NJS_MOTION dat_0cb218c8;
extern NJS_MOTION dat_0cb245c0;
extern NJS_MOTION dat_0cb27d34;
extern NJS_MOTION dat_0cb2b778;
extern NJS_MOTION dat_0cb29110;
extern NJS_MOTION dat_0cb2c5b4;
extern NJS_MOTION dat_0cb270f8;
extern NJS_MOTION dat_0cb2acbc;

/* the message file station_square_messages_load reads, at 0x0cb48000 */
extern struct field_messages field_messages;

#define WALKER_MOVES 7
#define WALKER_MODELS 26

/* on each frame's first step, a walking walker covers its kind's speed times
 * WALK_STRIDE */
#define WALK_STRIDE 1.9F

/* the most a walker turns toward its target each time it moves, just under
 * 15 degrees */
#define TURN_LIMIT 0xaaa

/* the lanes across the pavement, from lane 0 at its right edge. each lies
 * LANE_WIDTH of the pavement's width left of the one before */
#define LANES 5
#define LANE_WIDTH (1.0F / LANES)

/* away from the route's ends by END_MARGIN, a walker turns back when CROWD
 * walkers or more are within CROWD_RANGE ahead of it */
#define END_MARGIN 5.0F
#define CROWD_RANGE 10.0F
#define CROWD 4

/* a walker whose lane is less than CLOSE_LANES from the lane of the walker
 * ahead moves a lane away. it moves left when the ahead walker's lane is right
 * of MIDDLE_LANE, and right otherwise. NO_LANE stands in for the ahead
 * walker's lane when none is ahead, and lies beyond CLOSE_LANES of every
 * lane */
#define CLOSE_LANES 2
#define MIDDLE_LANE (LANES / 2)
#define NO_LANE (-5)

/* a walking route walker stops once player 0 comes within player 0's radius
 * plus STOP_GAP of it. it walks on once player 0 is beyond that radius plus
 * GO_GAP */
#define STOP_GAP 10.0F
#define GO_GAP 20.0F

/* a walker's talk reaches TALK_REACH past its shape's radius, and talk_add
 * adds player 0's radius to that */
#define TALK_REACH 20.0F

enum walker_move_id {
    MOVE_STAND_A,
    MOVE_STAND_B,
    MOVE_STAND_C,
    MOVE_TALK_A,
    MOVE_TALK_B,
    MOVE_WALK,
    /* played when a player with PLAYER_GUARD set hits the walker */
    MOVE_KNOCKED
};

/* one of the three standing moves, at random */
#define STAND_AT_RANDOM() ((int)RANDOM(MOVE_STAND_A, MOVE_TALK_A))

/* the chance that a standing move, as it ends, gives way to one picked by
 * STAND_AT_RANDOM. otherwise it plays again */
#define STAND_CHANGE 0.1F

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

/* a route point, as an act's ss_path file stores it */
struct walk_point {
    int along; /* distance along the route */
    /* the pavement's edges there, each as x and z */
    NJS_POINT2 left;
    NJS_POINT2 right;
};

struct walker_spec {
    Sint8 model;
    /* the walker's row of the field's table of lines, less
     * field_messages.people */
    Sint16 row;
};

struct walk_route_spec {
    const struct walker_spec *walkers;
    char *file;
    int count; /* the points in the ss_path file */
    Sint16 fill_count; /* how many walkers route_fill creates */
};

struct walker_start {
    Sint16 along;
    Sint8 model;
    Sint8 lane;
    Uint8 row; /* its row of the field's table of lines */
    Sint8 way; /* 1 forward along the route, -1 back */
};

/* a walker's place on its route: a node of the route's list, in order of
 * along */
struct walker_place {
    NJS_POINT2 target; /* x and z of the position the walker heads for */
    Float along;
    Float advance; /* along gained on each frame's first step, times pace */
    Float pace; /* always 1: only walker_create sets it */
    struct walker_place *prev;
    struct walker_place *next;
    Float switch_end; /* the along where a change of lane or direction ends */
    Bool switching;
    Sint16 point;
    Sint8 way; /* 1 forward along the route, -1 back */
    Sint8 lane; /* 0 to LANES - 1, from the pavement's right edge */
};

/* the record a route task keeps in its extra */
struct walk_route {
    struct walk_point *points;
    struct walker_place *first; /* the walkers, in order along the route */
    struct walker_place *last;
    int count; /* the points in the ss_path file */
    Uint32 unk_10; /* cleared and never read */
    int walkers; /* the walkers alive on the route, which nothing reads */
    int fill_count; /* how many walkers route_fill creates */
};

/* the record the crowd task keeps in its extra */
struct crowd {
    Bool started; /* TRUE once the act's people and route start */
    struct task *route;
    int act;
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
    Uint16 count; /* number of points on the walker's route */
    Float frame; /* the move's frame, or the blend's progress into the next */
    Bool talking; /* set when player 0 talks to the walker */
    struct walk_point *points;
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

/* distance from player 0 within which a walker shows, when the set entry gives
 * no reach of its own. a squared reach below NO_REACH2 counts as none */
#define WALKER_REACH 410.0F
#define NO_REACH2 0.01F

/* a standing walker within this distance of player 0 draws its motion, and
 * beyond it the plain model */
#define WALKER_NEAR 150.0F

/* a walker draws only while draw_in_view finds the sphere of VIEW_RADIUS
 * about its position in view */
#define VIEW_RADIUS 15.0F

static Bool walker_init(struct task *task, int move);
static void walker_draw(struct task *task);
static void walker_place_at(struct walker_work *work, NJS_POINT2 *out,
                            int point, int way, int lane);
static Bool walker_walk(struct task *task);
static void walker_react(struct walker_work *work, struct walker *walker);
static Bool walker_update(struct task *task);
static Bool walker_stand(struct task *task);
static Bool walker_talk(struct task *task);
static Bool walker_knocked(struct task *task);
static int walker_find_point(struct walker_work *work, NJS_POINT2 *out, int way,
                             int lane, Float along);
static void walker_draw_plain(struct task *task);
static void walker_print(struct task *task);
static void walker_head_for(struct walker_work *work,
                            struct walker_place *place, struct walker *walker,
                            Float along);
static void walker_set_model(struct task *task, struct walker *walker,
                             int index);
static struct walker_place *walker_create(struct task *task,
                                          struct walk_point *points, int count,
                                          struct walker_start *start);
static void walker_step(struct task *task);
static void walker_run(struct task *task);
static struct task *crowd_route_create(int act);
static void crowd_start(struct crowd *crowd);
static void crowd_leave_act(struct crowd *crowd);
static void crowd_step(struct task *task);
static void route_insert(struct walk_route *route, struct walker_place *place);
static void route_swap(struct walk_route *route, struct walker_place *first,
                       struct walker_place *second);
static void route_sort(struct walk_route *route, struct walker_place *place);
static void route_remove(struct walk_route *route, struct walker_place *place);
static void route_add_walker(struct task *task, int along, int index);
static void route_fill(struct task *task);
static void route_wait(struct task *task);
static void route_step(struct task *task);
static void route_destroy(struct task *task);
static void walk_point_at_lane(NJS_POINT2 *out, struct walk_point *point,
                               int lane);
static int walk_next_point(int point, int way, int count);
static void walker_aim(struct walker_place *place, struct walk_point *point,
                       NJS_POINT3 *pos, Float along, Float speed);
static void walker_go(NJS_POINT3 *pos, struct walker_place *place, Angle *ang,
                      Float speed);
static Angle walker_turn(NJS_POINT3 *pos, struct walker_place *place,
                         Angle ang);
static int walker_count_near(struct walker_place *place, int way, Float range);
static Bool crowd_person_update(struct task *task);
static Bool walker_play_move(struct task *task);
static void walker_wait(struct task *task);
static void walker_destroy(struct task *task);
static void walker_route_destroy(struct task *task);

static struct walker_kind walker_kinds[6] = {
    {{{&dat_0cb0c214, 0.5F},
      {&dat_0cb0d3a0, 0.5F},
      {&dat_0cb0d3a0, 0.5F},
      {&dat_0cb0dbfc, 0.5F},
      {&dat_0cb0dbfc, 0.5F},
      {&dat_0cb0b2c8, 1.0F},
      {&dat_0cb0f878, 1.0F}},
     9.8067F,
     16.4F,
     0.34F,
     WALKER_SHAPE(8.5F, 3.0F, 7.0F)},
    {{{&dat_0cb11eb0, 0.5F},
      {&dat_0cb11eb0, 0.5F},
      {&dat_0cb12f8c, 0.5F},
      {&dat_0cb13c58, 0.5F},
      {&dat_0cb14934, 0.5F},
      {&dat_0cb10e24, 1.0F},
      {&dat_0cb16430, 1.0F}},
     10.8472F,
     15.6F,
     0.28F,
     WALKER_SHAPE(7.15F, 2.3F, 5.9999996F)},
    {{{&dat_0cb11eb0, 0.5F},
      {&dat_0cb11eb0, 0.5F},
      {&dat_0cb12f8c, 0.5F},
      {&dat_0cb13c58, 0.5F},
      {&dat_0cb14934, 0.5F},
      {&dat_0cb10e24, 1.0F},
      {&dat_0cb16430, 1.0F}},
     10.18F,
     15.6F,
     0.28F,
     WALKER_SHAPE(7.15F, 2.3F, 5.9999996F)},
    {{{&dat_0cb18618, 0.5F},
      {&dat_0cb19464, 0.5F},
      {&dat_0cb1a1f0, 0.5F},
      {&dat_0cb1ae4c, 0.5F},
      {&dat_0cb1ba28, 0.5F},
      {&dat_0cb179bc, 1.0F},
      {&dat_0cb1d6e4, 1.0F}},
     5.7F,
     9.6F,
     0.19F,
     WALKER_SHAPE(5.0F, 2.0F, 4.0F)},
    {{{&dat_0cb1eb50, 0.5F},
      {&dat_0cb22d14, 0.5F},
      {&dat_0cb22d14, 0.5F},
      {&mannequin_motion, 0.5F},
      {&dat_0cb25a4c, 0.5F},
      {&dat_0cb218c8, 1.0F},
      {&dat_0cb245c0, 1.0F}},
     8.0F,
     13.8F,
     0.35F,
     WALKER_SHAPE(7.0F, 4.0F, 5.0F)},
    {{{&dat_0cb27d34, 0.5F},
      {&dat_0cb2b778, 0.33333332F},
      {&dat_0cb2b778, 0.33333332F},
      {&dat_0cb29110, 0.5F},
      {&dat_0cb2c5b4, 0.5F},
      {&dat_0cb270f8, 1.0F},
      {&dat_0cb2acbc, 1.0F}},
     5.66F,
     9.8F,
     0.33F,
     WALKER_SHAPE(5.0F, 2.0F, 4.0F)},
};

/* each character's message file, by character number. Eggman and Tikal have
 * none */
static char *message_files[CHARACTER_COUNT] = {
    "ss_mes_s.bin", NULL,           "ss_mes_m.bin", "ss_mes_k.bin",
    NULL,           "ss_mes_a.bin", "ss_mes_e.bin", "ss_mes_b.bin",
};

/* a route walker's shapes: its kind's capsule, copied in by
 * walker_start_walking, and a flat cylinder of the same radius */
/* clang-format off */
static struct collision_shape walker_shapes[2] = {
    {0},
    {SHAPE_CYLINDER | SHAPE_NO_PUSH | SHAPE_UNTURNED, 0,
     PUSH_OBJECTS | PUSH_FIRM(PUSH_PLAYERS), {0.0F, 0.0F, 0.0F}, 5.0F, 0.5F,
     0.0F, {0}, 0, NULL},
};
/* clang-format on */

static const struct walker_model walker_models[WALKER_MODELS] = {
    {&dat_0caee8e4, &ss_people_texlist, 0},
    {&dat_0caf12b8, &ss_people_texlist, 0},
    {&dat_0caf3924, &ss_people_texlist, 0},
    {&dat_0caf6850, &ss_people_texlist, 0},
    {&dat_0cb06cd8, &dat_0cb2c994, 0},
    {&dat_0cb09ce4, &dat_0cb2cac4, 0},
    {&dat_0cae0360, &ss_people_texlist, 1},
    {&dat_0cae3e38, &ss_people_texlist, 1},
    {&dat_0cae679c, &ss_people_texlist, 1},
    {&dat_0cae9ee0, &ss_people_texlist, 1},
    {&dat_0cafa58c, &dat_0cb2c9f0, 2},
    {&dat_0cafc128, &dat_0cb2c9f0, 2},
    {&dat_0cafdfc0, &dat_0cb2c9f0, 2},
    {&dat_0cb029cc, &dat_0cb2cac4, 1},
    {&dat_0cad0ffc, &ss_people_texlist, 3},
    {&dat_0cad4494, &ss_people_texlist, 3},
    {&dat_0cad7e9c, &ss_people_texlist, 3},
    {&dat_0cadb7dc, &ss_people_texlist, 3},
    {&mannequin_object, &ss_people_texlist, 4},
    {&dat_0cac6330, &ss_people_texlist, 4},
    {&dat_0cac8a7c, &ss_people_texlist, 4},
    {&dat_0cacc504, &ss_people_texlist, 4},
    {&dat_0cabaec8, &ss_people_texlist, 5},
    {&dat_0cabd234, &ss_people_texlist, 5},
    {&dat_0cabf974, &ss_people_texlist, 5},
    {&dat_0cac17ec, &ss_people_texlist, 5},
};

static const struct walker_spec office_walkers[15] = {
#include "adv00/crowd/office_walkers.inc"
};
static const struct walker_spec casino_walkers[15] = {
    {19, 2}, {19, 7}, {19, 12}, {0, 13}, {19, 19}, {19, 31},
};
static const struct walker_spec station_walkers[15] = {
#include "adv00/crowd/station_walkers.inc"
};

/* each act's route, by act. the sewer, hotel and Twinkle Park acts have
 * none */
static const struct walk_route_spec walk_routes[6] = {
    {office_walkers, "ss_path_o.bin", 59, 15},
    {casino_walkers, "ss_path_c.bin", 31, 6},
    {NULL, NULL, 0, 0},
    {station_walkers, "ss_path_s.bin", 66, 15},
    {NULL, NULL, 0, 0},
    {NULL, NULL, 0, 0},
};

static char *message_file;

void station_square_messages_load(void)
{
    struct task *task;
    struct crowd *crowd;

    message_file = event_flags[FLAG_LAST_STORY]
                       ? "ss_mes_l.bin"
                       : message_files[get_character()];
    if (loader_read_file(message_file, &field_messages) != -1) {
        script_set_lines_table(field_messages.table);
    } else {
        script_set_lines_table(NULL);
    }
    script_set_table((struct script_row *)station_square_script_table);
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
    /* @hack redundant: the image tests pos twice (ADD #32,R2 at 0x0c90fe30
     * and 0x0c90fe70); tested once, 71 of 112 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (WORK_SAME_PLACE(struct walker_work, pos) &&
        WORK_SAME_PLACE(struct walker_work, id) &&
        WORK_SAME_PLACE(struct walker_work, collision) &&
        WORK_SAME_PLACE(struct walker_work, event) &&
        WORK_SAME_PLACE(struct walker_work, pos) &&
        WORK_SAME_PLACE(struct walker_work, flags)) {
        if (work->row < field_messages.people &&
            walker_init(task, STAND_AT_RANDOM())) {
            walker = task->extra;
            collision_init(task, &walker->kind->shape, COLLISION_OBJECT, 1, 1);
            walker->update = crowd_person_update;
            task->destructor = walker_destroy;
        } else if (edit_mode) {
            walker_draw_plain(task);
        }
    }
    /* NOLINTEND(misc-redundant-expression) */
}

/* create the act's route task, read the route's points from the act's ss_path
 * file, and fill the route with walkers. it returns the task, or NULL when
 * the act has no route, or when the points' or the task's allocation or the
 * file's open fails */
static struct task *crowd_route_create(int act)
{
    char *file;
    unsigned int size;
    int alloc;
    struct walk_point *points;
    struct walk_route *route;
    struct task *task;

    if ((file = walk_routes[act].file) != NULL) {
        size = walk_routes[act].count * sizeof(struct walk_point);
        /* the points take whole sectors, as loader_read_file reads them */
        alloc = size / SECTOR_SIZE;
        /* @hack redundant: a jump to the next line, which has SHC load
         * SECTOR_SIZE into R2 ahead of size's multiply (MOV.W at 0x0c90ff86);
         * without it, SHC loads it into R3 partway through the multiply, and 8
         * of 100 words differ */
        goto round_partial;
    round_partial:
        alloc += size % SECTOR_SIZE ? 1 : 0;
        /* @hack redundant: a never-run test of size, which keeps the image's
         * operand order in the test of size % SECTOR_SIZE and heap_alloc's
         * address in R2 (0x0c90ff98, 0x0c90ffce); without it, 3 of 100 words
         * differ */
        if (0) {
            if (size) {
            }
        }
        alloc *= SECTOR_SIZE;
        if (alloc > 0) {
            /* @hack redundant: a jump to the next line, which keeps
             * heap_alloc's address in R2 (0x0c90ffce); without it, the address
             * takes R3, and 2 of 100 words differ */
            goto allocate_points;
        allocate_points:
            points = heap_alloc(alloc);
            /* @hack redundant: a never-run test of alloc, which keeps the
             * route's size in R4 and alloc in R14 (0x0c90ff80, 0x0c90ff90), as
             * the image has them; without it, the size takes R5 and alloc R4,
             * and 29 of 100 words differ */
            if (0) {
                if (alloc) {
                }
            }
            /* @hack redundant: a jump to the next line, which loads
             * heap_calloc's and task_create's addresses into R3 (0x0c90ffd4,
             * 0x0c90ffe4); without it, they take R2, and 11 of 100 words differ
             */
            goto allocate_route;
        allocate_route:
            route = heap_calloc(1, sizeof(struct walk_route));
            if (points) {
                task = task_create(0, 3, route_step);
                if (task) {
                    task->extra = route;
                    route->points = points;
                    route->count = walk_routes[act].count;
                    route->fill_count = walk_routes[act].fill_count;
                    route->unk_10 = 0;
                    route->walkers = 0;
                    task->destructor = route_destroy;
                    if (loader_read_file(file, points) != -1) {
                        route_fill(task);
                        return task;
                    } else {
                        task_destroy(task);
                    }
                }
            }
            /* @bug memory: frees points and route a second time when the
             * file's open fails, by the JSRs at 0x0c91002a and 0x0c910030.
             * task_destroy has already freed route as the task's extra, and
             * points through route_destroy */
            heap_free(points);
            heap_free(route);
        }
    }
    return NULL;
}

static void crowd_start(struct crowd *crowd)
{
    int act = (Uint8)stage_get_stage_act();

    station_square_people_place();
    crowd->route = crowd_route_create(act);
    crowd->act = act;
    crowd->started = TRUE;
}

/* when the act changes, remove the people and the route, so that the next step
 * starts the new act's */
static void crowd_leave_act(struct crowd *crowd)
{
    if (crowd->act != (Uint8)stage_get_stage_act()) {
        station_square_people_remove();
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

/* insert a walker into the route's list, ordered by along */
static void route_insert(struct walk_route *route, struct walker_place *place)
{
    struct walker_place *other;

    if (!route->first) {
        route->first = place;
        route->last = place;
        place->prev = NULL;
    } else {
        other = route->first;
        do {
            if (other->along > place->along) {
                if (other->prev) {
                    other->prev->next = place;
                } else {
                    route->first = place;
                }
                place->next = other;
                place->prev = other->prev;
                other->prev = place;
                return;
            }
            other = other->next;
        } while (other);
        route->last->next = place;
        place->prev = route->last;
        route->last = place;
    }
    place->next = NULL;
}

/* swap first and second, which are adjacent in the list in that order */
static void route_swap(struct walk_route *route, struct walker_place *first,
                       struct walker_place *second)
{
    struct walker_place *other = first->prev;

    if (other) {
        other->next = second;
    } else {
        route->first = second;
    }
    second->prev = other;
    first->prev = second;
    other = second->next;
    if (other) {
        other->prev = first;
    } else {
        route->last = first;
    }
    first->next = other;
    second->next = first;
}

/* swap a walker with a neighbor it has passed, keeping the list ordered by
 * along */
static void route_sort(struct walk_route *route, struct walker_place *place)
{
    struct walker_place *other = place->prev;

    if (other && other->along > place->along) {
        route_swap(route, other, place);
        return;
    }
    other = place->next;
    if (other && other->along < place->along) {
        route_swap(route, place, other);
        /* @hack redundant: a return where control returns anyway, which sends
         * the first swap through this one's tail jump to route_swap (BRA at
         * 0x0c9102da); without it, 21 of 30 words differ */
        return;
    }
}

static void route_remove(struct walk_route *route, struct walker_place *place)
{
    struct walker_place *next;

    if (route->first == place) {
        next = place->next;
        route->first = next;
        if (next) {
            next->prev = NULL;
        } else {
            route->last = NULL;
            return;
        }
    } else {
        place->prev->next = place->next;
    }
    if (route->last == place) {
        place = place->prev;
        route->last = place;
        place->next = NULL;
    } else {
        place->next->prev = place->prev;
    }
}

/* create a walker from the act's walker spec index, at along in a random
 * lane, and add it to the route. it walks toward the farther end of the
 * route */
static void route_add_walker(struct task *task, int along, int index)
{
    struct walk_route *route = task->extra;
    struct task *child;
    const struct walker_spec *spec;
    struct walker_start start;
    struct walker_place *place;

    child = task_create_child(TASK_WORK, walker_step, task);
    if (child) {
        spec = &walk_routes[(Uint8)stage_get_stage_act()].walkers[index];
        start.model = spec->model;
        start.row = spec->row + field_messages.people;
        start.lane = rand() % LANES;
        start.along = along;
        start.way =
            start.along < route->points[route->count - 1].along * 0.5F ? 1 : -1;
        if ((place = walker_create(child, route->points, route->count,
                                   &start)) != NULL) {
            route_insert(route, place);
            ++route->walkers;
        }
    }
}

/* create the route's walkers from the last to the first, each at a random
 * position within its own equal stretch of the route */
static void route_fill(struct task *task)
{
    struct walk_route *route = task->extra;
    int count = route->fill_count;
    /* the division is of ints, so the stretches stop short of the route's end
     * by its remainder */
    /* NOLINTNEXTLINE(bugprone-integer-division) */
    Float stretch = route->points[route->count - 1].along / count;

    do {
        route_add_walker(
            task, (int)(((rand() * RAND_SCALE) + count - 1.0F) * stretch),
            count - 1);
    } while (--count);
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

    heap_free(route->points);
}

/* store in out the position of lane at the route point */
static void walk_point_at_lane(NJS_POINT2 *out, struct walk_point *point,
                               int lane)
{
    Float share = lane * LANE_WIDTH;
    Float right_x = point->right.x;
    Float right_z = point->right.y;

    out->x = right_x + ((point->left.x - right_x) * share);
    out->y = right_z + ((point->left.y - right_z) * share);
}

/* store the position of lane at the route point in out. with out NULL, move
 * the walker there instead, facing the next point in direction way */
static void walker_place_at(struct walker_work *work, NJS_POINT2 *out,
                            int point, int way, int lane)
{
    NJS_POINT2 here;
    NJS_POINT2 next;

    if (out) {
        walk_point_at_lane(out, &work->points[point], lane);
    } else {
        walk_point_at_lane(&here, &work->points[point], lane);
        walk_point_at_lane(&next, &work->points[point + way], lane);
        /* @hack spelling: here read through its address, which SHC forms in R3
         * for each read (MOV R15,R3 and ADD #20,R3 at 0x0c9105a8 and
         * 0x0c9105b0); without it, 32 of 76 words differ */
        work->pos.x = (&here)->x;
        work->pos.z = (&here)->y;
        work->ang_y = angle_arc_tan2(next.y - here.y, -next.x + here.x);
    }
}

/* store the position of lane at along in out. with out NULL, move the walker
 * there instead. an along at or before point 1's gives point 1's position,
 * and one past point count - 2's gives that point's, where count is
 * work->count. at a point the walker faces the next point in direction way.
 * between two points it faces off its way, as the @bug below says. it
 * returns the index of the point the walker heads for next */
static int walker_find_point(struct walker_work *work, NJS_POINT2 *out, int way,
                             int lane, Float along)
{
    int count = work->count;
    int index;
    struct walk_point *before;
    struct walk_point *after;
    NJS_POINT2 to;
    NJS_POINT2 from;
    Float share;
    Float x;
    Float z;
    Float before_x;
    Float before_z;
    Float after_x;
    Float after_z;

    for (index = 0; index < count && along > work->points[index].along;
         ++index) {
    }
    /* @hack redundant: a never-run test of lane, which keeps count in R9 and
     * lane in R10 (0x0c910640, 0x0c910646), as the image has them; without it,
     * count and lane swap registers, and 17 of 173 words differ */
    if (0) {
        if (lane) {
        }
    }
    if (index <= 1) {
        walker_place_at(work, out, 1, way, lane);
        return way + 1;
    }
    if (index >= count - 1) {
        walker_place_at(work, out, count - 2, way, lane);
        return way + count - 2;
    }
    if (along == work->points[index].along) {
        walker_place_at(work, out, index, way, lane);
        return way + index;
    }
    after = &work->points[index];
    before = after - 1;
    walk_point_at_lane(&from, before, lane);
    walk_point_at_lane(&to, after, lane);
    share = (Float)after->along;
    along -= share;
    share = along / ((Float)before->along - share);
    before_x = from.x;
    after_x = to.x;
    after_z = to.y;
    before_z = from.y;
    x = after_x + ((before_x - after_x) * share);
    z = after_z;
    z += (before_z - after_z) * share;
    /* @hack redundant: a never-run test of before_x, which keeps before_x in
     * FR14 and before_z in FR13 (0x0c9106fa, 0x0c910720), as the image has
     * them; without it, before_x and before_z swap registers, and 6 of 173
     * words differ */
    if (0) {
        if (before_x) {
        }
    }
    /* @hack redundant: a never-run test of after_z, which keeps after_z in FR5
     * and z in FR6 (0x0c910714, 0x0c910722), as the image has them; without it,
     * after_z and z swap registers, and 13 of 173 words differ */
    if (0) {
        if (after_z) {
        }
    }
    /* @hack redundant: a second never-run test of after_z. without it, after_z
     * and z swap FR5 and FR6 (0x0c910714, 0x0c910722), and 13 of 173 words
     * differ */
    if (0) {
        if (after_z) {
        }
    }
    /* @hack redundant: a never-run test of x, which keeps x in FR4, after_z in
     * FR5 and z in FR6 (0x0c910714 to 0x0c910726), as the image has them;
     * without it, x takes FR6, after_z FR4 and z FR5, and 16 of 173 words
     * differ */
    if (0) {
        if (x) {
        }
    }
    /* @hack redundant: a second never-run test of x. without it, x takes FR6,
     * after_z FR4 and z FR5 (0x0c910714 to 0x0c910726), and 16 of 173 words
     * differ */
    if (0) {
        if (x) {
        }
    }
    if (out) {
        out->x = x;
        out->y = z;
    } else {
        work->pos.x = x;
        work->pos.z = z;
        /* @bug logic: a walker that starts between two points faces
         * off its way, until its walk turns it toward its target. the facing
         * takes before_z less after_x, where after_z belongs (FSUB FR15,FR4
         * at 0x0c91074a). with after_z, the angle would face before, the way
         * -1 heads. the multiply by way (MUL.L at 0x0c91074c) keeps that
         * facing for way 1, and negates it for way -1, which mirrors it */
        work->ang_y =
            angle_arc_tan2(before_z - after_x, after_x - before_x) * way;
    }
    return way == 1 ? index : index - 1;
}

/* aim the walker at points[point] at its kind's walking speed, make
 * walker_update its update, and give it walker_shapes */
static void walker_start_walking(struct task *task, struct walker_place *place,
                                 struct walk_point *points, int point)
{
    struct walker *walker = task->extra;
    struct walker_work *work = WALKER(task);

    walker_aim(place, &points[point], &work->pos, place->along,
               walker->kind->speed * WALK_STRIDE);
    walker->place = place;
    walker->update = walker_update;
    njMemCopy4(walker_shapes, &walker->kind->shape,
               sizeof(struct collision_shape) / 4);
    walker_shapes[1].radius = walker->kind->shape.radius;
    collision_init(task, walker_shapes, COLLISION_OBJECT_MUTUAL, 1, 2);
}

/* start the route walker task as start gives it, and return its place on the
 * route. when an allocation fails or the walker's row picks no lines, it
 * destroys the task and returns NULL */
static struct walker_place *walker_create(struct task *task,
                                          struct walk_point *points, int count,
                                          struct walker_start *start)
{
    struct walker_work *work = WALKER(task);
    struct walker_place *place = heap_calloc(1, sizeof(struct walker_place));
    int point;
    int way;

    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    work->model = start->model;
    work->row = start->row;
    if (place && walker_init(task, MOVE_WALK)) {
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        way = start->way;
        place->way = way;
        place->lane = start->lane;
        place->along = start->along;
        place->pace = 1.0F;
        work->points = points;
        work->count = count;
        point = walker_find_point(work, NULL, way, start->lane, start->along);
        place->point = point;
        walker_start_walking(task, place, points, point);
        work->pos.y = 0.0F;
        task->destructor = walker_route_destroy;
        return place;
    } else {
        heap_free(place);
        task_destroy(task);
        return NULL;
    }
}

/* walk toward each route point in turn, reversing at either end, on a frame's
 * first step only. on a frame's later steps, and on the step it reverses at
 * an end, it returns TRUE. on any other, it returns FALSE once player 0 is
 * beyond the walker's reach, and TRUE otherwise */
static Bool walker_walk(struct task *task)
{
    struct walker_work *work = WALKER(task);
    struct walker *walker = task->extra;
    struct walker_place *place;
    struct walker_kind *kind;
    struct walker_move *move;
    struct walk_point *points;
    int point;
    int next;
    Float speed;
    Float pace;
    Float distance2;
    NJS_POINT3 pos;

    if (frame_step) {
        return TRUE;
    }
    place = walker->place;
    /* @hack spelling: kind set through its address, which keeps it in a stack
     * slot (MOV.L R3,@(4,R15) at 0x0c9108f6); without it, 170 of 190 words
     * differ */
    speed = (*&kind = walker->kind)->speed * WALK_STRIDE;
    points = work->points;
    point = place->point;
    move = &kind->moves[walker->move];
    if ((place->way == 1 && !(points[point].along > place->along)) ||
        (place->way != 1 && !(place->along > points[point].along))) {
        if ((next = walk_next_point(point, place->way, work->count)) < 0) {
            place->way = -place->way;
            walker_head_for(work, place, walker, place->along + place->way);
            return TRUE;
        }
        walker_aim(place, &points[next], &work->pos, place->along, speed);
        place->point = next;
    }
    pace = place->pace;
    walker_go(&work->pos, place, &work->ang_y, speed * pace);
    if ((work->frame += (pace * move->speed) * MOVE_TIME) >
        (Float)move->motion->nbFrame - 1.0F) {
        work->frame = 0.0F;
    }
    place->along += pace * place->advance;
    player_get_history(0, 0, &pos, NULL);
    /* @hack spelling: distance2 set through its address, which stores it to the
     * stack and reads it back (FMOV.S FR0,@R8 and FMOV.S @R15,FR2 at
     * 0x0c910a76 and 0x0c910a78); without it, 184 of 190 words differ */
    *&distance2 = point_distance_squared_across(&pos, &work->pos);
    if (distance2 > work->reach2) {
        return FALSE;
    }
    return TRUE;
}

/* the next point index in direction way, or -1 past either end of the route */
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

/* aim the walker at the point's position in its lane, with advance set so that
 * along reaches the point's along as the walker arrives */
static void walker_aim(struct walker_place *place, struct walk_point *point,
                       NJS_POINT3 *pos, Float along, Float speed)
{
    Float left = point->along - along;
    Float share = place->lane * LANE_WIDTH;
    Float x = point->right.x + ((point->left.x - point->right.x) * share);
    Float right_z = point->right.y;
    Float z = right_z + ((point->left.y - right_z) * share);

    place->advance = left * speed *
                     _builtin_fsrra(((x - pos->x) * (x - pos->x)) +
                                    ((z - pos->z) * (z - pos->z)));
    place->target.x = x;
    place->target.y = z;
}

/* turn the walker toward its target, and move it speed units ahead */
static void walker_go(NJS_POINT3 *pos, struct walker_place *place, Angle *ang,
                      Float speed)
{
    Angle turned = *ang;
    Float sine;
    Float cosine;

    turned += walker_turn(pos, place, turned);
    _builtin_fsca(turned, &sine, &cosine);
    pos->x -= speed * cosine;
    pos->z += speed * sine;
    *ang = turned;
}

/* the turn toward the target, at most TURN_LIMIT either way */
static Angle walker_turn(NJS_POINT3 *pos, struct walker_place *place, Angle ang)
{
    Sint16 turn = (Sint16)(angle_arc_tan2(place->target.y - pos->z,
                                          -place->target.x + pos->x) -
                           ang);

    if ((turn >= 0 ? turn : -turn) < TURN_LIMIT) {
        return turn;
    } else {
        return turn >= 0 ? TURN_LIMIT : -TURN_LIMIT;
    }
}

/* the number of consecutive walkers ahead in direction way within range of
 * the walker */
static int walker_count_near(struct walker_place *place, int way, Float range)
{
    Float along = place->along;
    int count = 0;
    Float distance = 0.0F;

    while ((place = way == 1 ? place->next : place->prev) &&
           range > _builtin_fabsf(distance = place->along - along)) {
        ++count;
    }
    return count;
}

/* aim the walker at walker_find_point's position of its lane at along,
 * starting a switch that ends once the walker passes along */
static void walker_head_for(struct walker_work *work,
                            struct walker_place *place, struct walker *walker,
                            Float along)
{
    Float advance;

    place->point =
        walker_find_point(work, &place->target, place->way, place->lane, along);
    place->switch_end = along;
    place->switching = TRUE;
    advance = along - place->along;
    advance *= walker->kind->speed;
    place->advance = advance *
                     _builtin_fsrra(((place->target.x - work->pos.x) *
                                     (place->target.x - work->pos.x)) +
                                    ((place->target.y - work->pos.z) *
                                     (place->target.y - work->pos.z))) *
                     WALK_STRIDE;
}

/* keep the route's list in order of along, and the walker apart from the
 * walkers ahead. it turns back from a crowd, and moves a lane away from a
 * walker in a close lane. a switch ends once the walker passes switch_end */
static void walker_keep_apart(struct task *task)
{
    struct walker *walker = task->extra;
    struct walker_place *place = walker->place;
    struct walker_work *work = WALKER(task);
    struct walker_place *ahead;
    Float along;
    int count;
    int way;
    int lane;
    int other_lane;

    route_sort(task->parent->extra, place);
    along = place->along;
    if (along > END_MARGIN &&
        work->points[work->count - 1].along - END_MARGIN > along) {
        count = walker_count_near(place, place->way, CROWD_RANGE);
        if (count >= CROWD) {
            way = -place->way;
            place->way = way;
            walker_head_for(work, place, walker, way + along);
        }
    }
    ahead = place->way == 1 ? place->next : place->prev;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    lane = place->lane;
    other_lane = ahead ? ahead->lane : NO_LANE;
    if ((lane - other_lane >= 0 ? lane - other_lane : other_lane - lane) <
        CLOSE_LANES) {
        lane += other_lane < MIDDLE_LANE ? 1 : -1;
        place->lane = lane;
        walker_head_for(work, place, walker, place->along + place->way);
    }
    if (place->switching &&
        (place->switch_end - place->along) * place->way < 0.0F) {
        walker_aim(place, &work->points[place->point], &work->pos, place->along,
                   walker->kind->speed * WALK_STRIDE);
        place->switching = FALSE;
    }
}

/* a route walker's update. while the next move is MOVE_KNOCKED or a talking
 * move, or the walker is beyond its reach, it returns FALSE. otherwise it
 * keeps the walker apart from the walkers ahead, and stops it or walks it on
 * as STOP_GAP and GO_GAP say. it returns FALSE on a step that switches the
 * walker between walking and standing. on any other it returns TRUE, so that
 * walker_react can start a knock or a talk */
static Bool walker_update(struct task *task)
{
    struct walker *walker = task->extra;
    struct walker_work *work = WALKER(task);
    struct player *player;
    NJS_POINT3 pos;
    Float radius;
    Float reach;
    int next = walker->next_move;

    if (next == MOVE_KNOCKED) {
        return FALSE;
    }
    if (next == MOVE_TALK_A) {
        return FALSE;
    }
    if (next == MOVE_TALK_B) {
        return FALSE;
    }
    if (work->distance2 > work->reach2) {
        return FALSE;
    }
    walker_keep_apart(task);
    player = player_get(0);
    radius = player ? player->consts.radius : 0.0F;
    player_get_history(0, 0, &pos, NULL);
    if (walker->next_move == MOVE_WALK) {
        reach = radius + STOP_GAP;
        if (reach * reach > work->distance2) {
            walker->next_move = STAND_AT_RANDOM();
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

/* allocate the walker's record and its lines, start it in move, and register
 * it as a talk for player 0. it returns TRUE once started. when an allocation
 * fails or the walker's row picks no lines, it frees what it allocated and
 * returns FALSE */
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
 * from walker_models[0] for an index out of range. a walker with no reach of
 * its own takes WALKER_REACH */
static void walker_set_model(struct task *task, struct walker *walker,
                             int index)
{
    const struct walker_model *model;
    struct walker_work *work;
    struct walker_kind *kind;
    int number;

    if (index < 0 || index >= WALKER_MODELS) {
        index = 0;
    }
    model = &walker_models[index];
    number = model->kind;
    walker->object = model->object;
    walker->texlist = model->texlist;
    kind = &walker_kinds[number];
    walker->kind = kind;
    work = WALKER(task);
    if (work->reach2 < NO_REACH2) {
        work->reach2 = WALKER_REACH * WALKER_REACH;
    }
}

/* play a standing move. as it ends, another may follow, as STAND_CHANGE says.
 * outside the set editor, it returns FALSE once player 0 is beyond the
 * walker's reach. it returns TRUE otherwise. outside the editor, the walker
 * draws its plain model while player 0 is beyond WALKER_NEAR of it */
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
    move = walker->move + kind->moves;
    if ((work->frame += MOVE_TIME * move->speed) >
        (Float)move->motion->nbFrame - 1.0F) {
        if (STAND_CHANGE > rand() * RAND_SCALE) {
            walker->next_move = STAND_AT_RANDOM();
        }
        walker->last_frame = work->frame - move->speed;
        walker->blend = BLEND_MOVE;
        work->frame = 0.0F;
    }
    return TRUE;
}

/* play a talking move until the lines stop showing. then end the talk, stop
 * player 0 looking at the walker, and stand */
static Bool walker_talk(struct task *task)
{
    struct walker *walker = task->extra;
    struct walker_work *work = WALKER(task);
    struct walker_move *move;
    int next;

    if (((next = walker->next_move) == MOVE_TALK_A || next == MOVE_TALK_B) &&
        !work->shown) {
        talk_end();
        player_clear_look(0);
        walker->next_move = STAND_AT_RANDOM();
        walker->last_frame = work->frame;
        walker->blend = BLEND_MOVE;
        work->frame = 0.0F;
    }
    /* @hack redundant: a never-run test of work, which keeps the walker in R13
     * and work in R14 (0x0c9111ae, 0x0c9111b0), as the image has them; without
     * it, the walker and work swap registers, and 14 of 82 words differ */
    if (0) {
        if (work) {
        }
    }
    move = &walker->kind->moves[walker->move];
    /* @hack spelling: the wrap as a ?: with a (void)0 arm, which has SHC load
     * 0.0F for its store (FLDI0 FR3 at 0x0c91123a); as an if, SHC keeps 0.0F
     * in FR15 for the function's two stores of it, and 77 of 82 words
     * differ */
    ((work->frame += MOVE_TIME * move->speed) >
     (Float)move->motion->nbFrame - 1.0F)
        ? (void)(work->frame = 0.0F)
        : (void)0;
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
 * when player 0 talks to it, start a talking move and show its lines. the row
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
        walker->next_move = (rand() & 1) + MOVE_TALK_A;
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
    walker_stand, walker_stand, walker_stand,   walker_talk,
    walker_talk,  walker_walk,  walker_knocked,
};

static Bool walker_play_move(struct task *task)
{
    struct walker *walker = task->extra;
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
    NJS_POINT3 pos;
    Bool (*update)(struct task *task);
    Float frame;

    player_get_history(0, 0, &pos, NULL);
    /* @hack spelling: work->distance2 set through its address, which SHC forms
     * in R8 before the call and stores through after it (ADD #28,R2 and MOV
     * R2,R8 at 0x0c911464 and 0x0c911468, FMOV.S FR0,@R8 at 0x0c911470);
     * without it, 70 of 73 words differ */
    *&work->distance2 = point_distance_squared_across(&pos, &work->pos);
    /* @hack redundant: a jump to the next line, which loads walker->update into
     * R2 (0x0c911472); without it, walker->update takes R3, and 3 of 73 words
     * differ */
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
            collision_init(task, &walker->kind->shape, COLLISION_OBJECT, 1, 1);
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
     * (MOV.L @(32,R12),R12 at 0x0c91166a); through work, 98 of 145 words
     * differ */
    drawn = WALKER(task);
    njTranslateV(NULL, &drawn->pos);
    shadow_draw_person(kind->shape.radius);
    njTranslate(NULL, 0.0F, kind->lift, 0.0F);
    TURN_VIA(njRotateY, drawn->ang_y, ang);
    draw_set_palette(DRAW_PLAYER_PALETTE);
    njSetTexture(walker->texlist);
    object = walker->object;
    if (walker->animate) {
        frame = drawn->frame;
        if (walker->move == walker->next_move) {
            draw_motion(object, kind->moves[walker->move].motion, frame);
        } else {
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
     * walker_route_destroy branch here (BRA at 0x0c9117b6); without it, SHC
     * expands this function there, the object is 32 bytes longer, and 22 of
     * the 36 words of the two functions differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

static void walker_route_destroy(struct task *task)
{
    struct walk_route *route = task->parent->extra;
    struct walker *walker = task->extra;

    --route->walkers;
    route_remove(route, walker->place);
    heap_free(walker->place);
    walker_destroy(task);
}
