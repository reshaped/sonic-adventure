/* Station Square people and the field's script table */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/people.h"
#include "adv00/shadow.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "event/play.h"
#include "event/script.h"
#include "object/common.h"
#include "player/big_kaeru_textures.h"
#include "player/big_models.h"
#include "player/player.h"
#include "player/talk.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

#define TEXT_1_LENGTH 40
#define TEXT_2_LENGTH 22
#define TEXT_3_LENGTH 42
#define TEXT_4_LENGTH 53
#define TEXT_5_LENGTH 30
#define TEXT_6_LENGTH 54
#define TEXT_7_LENGTH 30
#define TEXT_8_LENGTH 30
#define TEXT_9_LENGTH 26
#define TEXT_10_LENGTH 73
#define TEXT_11_LENGTH 41
#define TEXT_12_LENGTH 32
#define TEXT_13_LENGTH 59
#define TEXT_14_LENGTH 24
#define TEXT_15_LENGTH 39
#define TEXT_16_LENGTH 39
#define TEXT_17_LENGTH 24
#define TEXT_18_LENGTH 22
#define TEXT_19_LENGTH 16
#define TEXT_20_LENGTH 59
#define TEXT_21_LENGTH 30
#define TEXT_22_LENGTH 16
#define TEXT_23_LENGTH 16
#define TEXT_24_LENGTH 22
#define TEXT_25_LENGTH 43
#define TEXT_26_LENGTH 20
#define TEXT_27_LENGTH 37
#define TEXT_28_LENGTH 28
#define TEXT_29_LENGTH 49
#define TEXT_30_LENGTH 49
#define TEXT_31_LENGTH 41
#define TEXT_32_LENGTH 43
#define TEXT_33_LENGTH 53
#define TEXT_34_LENGTH 38
#define TEXT_35_LENGTH 29
#define TEXT_36_LENGTH 45
#define TEXT_37_LENGTH 50
#define TEXT_38_LENGTH 63
#define TEXT_39_LENGTH 51
#define TEXT_40_LENGTH 43
#define TEXT_41_LENGTH 41
#define TEXT_42_LENGTH 53
#define TEXT_43_LENGTH 49
#define TEXT_44_LENGTH 61
#define TEXT_45_LENGTH 49
#define TEXT_46_LENGTH 61
#include "adv00/people/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION tails_stand_action;
extern NJS_ACTION knuckles_stand_action;
extern NJS_ACTION amy_stand_action;
extern NJS_ACTION e102_stand_action;
extern NJS_ACTION eggman_stand_action;
/* the frog's action */
extern NJS_ACTION dat_8c58332c;
/* Sonic's and Tails' actions low to the ground */
extern NJS_ACTION dat_0cb2decc;
extern NJS_ACTION dat_0cb2e680;
/* the welds of Big's model */
extern struct weld dat_0cb2e6e8[];

/* the most people station_square_people_place places at once */
#define PEOPLE_MAX 3

/* the indexes of people_kinds. the low kinds show Sonic and Tails low to the
 * ground */
enum person_kind_index {
    PERSON_SONIC,
    PERSON_TAILS,
    PERSON_KNUCKLES,
    PERSON_AMY,
    PERSON_E102,
    PERSON_BIG,
    PERSON_FROG, /* the only kind drawn turned about z */
    PERSON_SONIC_LOW,
    PERSON_TAILS_LOW,
    PERSON_EGGMAN
};

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
    /* the character whose textures texture_load_character loads: Big's for
     * the frog, whose texlist loads with his */
    int character;
    NJS_TEXLIST *texlist;
    NJS_ACTION *action;
    Float lift; /* how far above pos the model is drawn */
    Float eye; /* how far above pos player 0 looks during a talk */
    struct collision_shape shape;
    struct weld *welds; /* an array ended by a weld with a null base, or NULL */
    Uint32 last_node; /* the last node welds_pose poses, counted from 0 */
};

/* where an act places a person. the person is placed only when its row's
 * script passes, and it speaks the pages the script picks */
struct person_spot {
    NJS_POINT3 pos;
    Sint16 ang_y;
    Sint8 row; /* in station_square_script_table */
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
    /* 1 while the person's message shows, as message_set_shown sets it */
    Uint8 shown;
    Sint16 flags;
    Sint16 kind; /* a person_kind_index */
    Float frame; /* the action's frame */
    /* TRUE once player 0 starts talking to the person. person_step clears it
     * as it starts the person's message */
    Bool talking;
    /* WELD_MATRICES matrices for welds_pose, for a kind with welds */
    NJS_MATRIX *weld_matrices;
    int slot; /* the person's index in people[] */
    Angle ang_y;
    Angle ang_z; /* the frog's turn about z, left 0 from the cleared work */
    NJS_POINT3 pos;
    Uint8 unk_2c[4];
    struct script_pages *pick; /* what the person's row last picked */
    int row; /* in station_square_script_table */
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

/* the kinds, by person_kind_index */
static const struct person_kind people_kinds[10] = {
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
     PERSON_SHAPE(SHAPE_CAPSULE, 8.5F, 9.0F, 4.0F), dat_0cb2e6e8, 73},
    {CHARACTER_BIG, &big_kaeru_texlist, &dat_8c58332c, 1.6F, 1.6F,
     PERSON_SHAPE(SHAPE_SPHERE, 3.0F, 3.0F, 0.0F), NULL, 0},
    {CHARACTER_SONIC, &sonic_texlist, &dat_0cb2decc, 5.4F, 2.5F,
     PERSON_SHAPE(SHAPE_SPHERE, 2.5F, 4.0F, 0.0F), NULL, 0},
    {CHARACTER_TAILS, &tails_texlist, &dat_0cb2e680, 4.5F, 2.5F,
     PERSON_SHAPE(SHAPE_SPHERE, 2.5F, 3.0F, 0.0F), NULL, 0},
    {CHARACTER_EGGMAN, &eggman_texlist, &eggman_stand_action, 8.6F, 7.0F,
     PERSON_SHAPE(SHAPE_CAPSULE, 7.5F, 5.0F, 5.0F), NULL, 0},
};

/* clang-format off */
/* the station act's people */
static const struct person_spot people_act3[6] = {
    {{-22.23643F, 0.0F, 1828.311F}, -17053, 0, PERSON_BIG},
    {{329.4F, 0.0F, 1617.0F}, 14848, 1, PERSON_SONIC},
    {{-41.47962F, 0.01037F, 1669.0005F}, 26288, 2, PERSON_SONIC},
    {{-41.51701F, 0.0043F, 1689.9186F}, -21554, 3, PERSON_TAILS},
    {{165.60532F, 0.0F, 1238.7776F}, -32768, 4, PERSON_AMY},
    {{36.79F, 1.7F, 1665.08F}, -24775, 5, PERSON_FROG},
};

/* row 0: Big */
static const Sint16 people_script_0[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_EVENTS, SCRIPT_ON | 9,
    SCRIPT_EVENTS, 13,
    SCRIPT_VOICE, 899,
    SCRIPT_FLAG, SCRIPT_ON | FLAG_PLAYABLE_BIG,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_EVENTS, SCRIPT_ON | 53,
    SCRIPT_EVENTS, 59,
    SCRIPT_VOICE, 899,
    SCRIPT_FLAG, SCRIPT_ON | FLAG_PLAYABLE_BIG,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_KNUCKLES_CLEAR_CHAOS2,
    SCRIPT_FLAGS, FLAG_KNUCKLES_CLEAR_RUIN,
    SCRIPT_VOICE, 899,
    SCRIPT_FLAG, SCRIPT_ON | FLAG_PLAYABLE_BIG,
    SCRIPT_END,
};

static const struct script_page people_pages_0[] = {
    {TEXT_1, 300},
    {NULL, 0},
    {TEXT_1, 300},
    {NULL, 0},
    {TEXT_1, 300},
    {NULL, 0},
};

/* row 1: Sonic, who says nothing: the row has no pages */
static const Sint16 people_script_1[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_EVENTS, SCRIPT_ON | 98,
    SCRIPT_EVENTS, 99,
    SCRIPT_END,
};

/* rows 2 to 4: Sonic, Tails and Amy */
static const Sint16 people_script_2[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_FLAGS, FLAG_BIG_CLEAR_TWINKLE_PARK,
    SCRIPT_VOICE, 901,
    SCRIPT_END,
};

static const struct script_page people_pages_2[] = {
    {TEXT_2, 150},
    {NULL, 0},
};

static const Sint16 people_script_3[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_FLAGS, FLAG_BIG_CLEAR_TWINKLE_PARK,
    SCRIPT_VOICE, 902,
    SCRIPT_END,
};

static const struct script_page people_pages_3[] = {
    {TEXT_3, 180},
    {NULL, 0},
};

static const Sint16 people_script_4[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_BIG_CLEAR_TWINKLE_PARK,
    SCRIPT_FLAGS, FLAG_BIG_CLEAR_ICECAP,
    SCRIPT_VOICE, 906,
    SCRIPT_END,
};

static const struct script_page people_pages_4[] = {
    {TEXT_4, 450},
    {NULL, 0},
};

/* row 5: the frog, which says nothing: the row has no pages */
static const Sint16 people_script_5[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_EVENTS, SCRIPT_ON | 211,
    SCRIPT_EVENTS, 227,
    SCRIPT_END,
};

/* the hotel act's people */
static const struct person_spot people_act4[2] = {
    {{-348.3946F, 0.0F, 1715.8912F}, -2544, 6, PERSON_BIG},
    {{-368.3F, 0.0F, 1666.12F}, -15724, 7, PERSON_EGGMAN},
};

/* row 6: Big */
static const Sint16 people_script_6[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_EVENTS, SCRIPT_ON | 21,
    SCRIPT_FLAGS, FLAG_SONIC_CLEAR_MOUNTAIN,
    SCRIPT_VOICE, 899,
    SCRIPT_END,
};

static const struct script_page people_pages_6[] = {
    {TEXT_1, 300},
    {NULL, 0},
};

/* row 7: Eggman, who says nothing: the row has no pages */
static const Sint16 people_script_7[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_KNUCKLES_CLEAR_CASINO,
    SCRIPT_EVENTS, 160,
    SCRIPT_END,
};

/* the casino act's people */
static const struct person_spot people_act1[3] = {
    {{-535.4F, -2.0F, 946.5F}, -1498, 8, PERSON_SONIC_LOW},
    {{-526.0F, -2.0F, 941.0F}, -3584, 9, PERSON_TAILS_LOW},
    {{-56.53389F, 65.99999F, 1342.7798F}, -3072, 10, PERSON_SONIC},
};

/* rows 8 and 9: Sonic and Tails, low to the ground */
static const Sint16 people_script_8[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_KNUCKLES_CLEAR_CASINO,
    SCRIPT_FLAGS, FLAG_KNUCKLES_CLEAR_CHAOS2,
    SCRIPT_VOICE, 900,
    SCRIPT_END,
};

static const struct script_page people_pages_8[] = {
    {TEXT_5, 180},
    {NULL, 0},
};

static const Sint16 people_script_9[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_KNUCKLES_CLEAR_CASINO,
    SCRIPT_FLAGS, FLAG_KNUCKLES_CLEAR_CHAOS2,
    SCRIPT_VOICE, 900,
    SCRIPT_END,
};

static const struct script_page people_pages_9[] = {
    {TEXT_5, 180},
    {NULL, 0},
};

/* row 10: Sonic */
static const Sint16 people_script_10[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_BIG_CLEAR_ICECAP,
    SCRIPT_FLAGS, FLAG_BIG_CLEAR_BEACH,
    SCRIPT_VOICE, 903,
    SCRIPT_END,
};

static const struct script_page people_pages_10[] = {
    {TEXT_6, 220},
    {NULL, 0},
};

/* the office act's people */
static const struct person_spot people_act0[1] = {
    {{-535.4F, -2.0F, 946.5F}, -1498, 11, PERSON_FROG},
};

/* row 11: the frog, which says nothing: the row has no pages */
static const Sint16 people_script_11[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_EVENTS, SCRIPT_ON | 211,
    SCRIPT_EVENTS, 227,
    SCRIPT_END,
};

/* each act's people, by act. the sewers and Twinkle Park have none */
static const struct person_list people_lists[6] = {
    {people_act0, 1},
    {people_act1, 3},
    {NULL, 0},
    {people_act3, 6},
    {people_act4, 2},
    {NULL, 0},
};

/* rows 12 to 19 are the hint boxes'. row 12: the box at the Twinkle Park
 * gate */
static const Sint16 hint_script_12[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_CLEAR_TWINKLE_PARK,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_TAILS_SS_ENTRANCE_CIRCUIT,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_TAILS_SS_CART_PASSPORT,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_KNUCKLES_SS_ENTRANCE_CIRCUIT,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_KNUCKLES_SS_CART_PASSPORT,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_AMY_CLEAR_TWINKLE_PARK,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_BIG_CLEAR_TWINKLE_PARK,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_E102_SS_ENTRANCE_CIRCUIT,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_E102_SS_CART_PASSPORT,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_END,
};

static const struct script_page hint_pages_12[] = {
    {TEXT_7, 0},
    {NULL, 0},
    {TEXT_8, 0},
    {TEXT_9, 0},
    {NULL, 0},
    {TEXT_7, 0},
    {NULL, 0},
    {TEXT_10, 0},
    {NULL, 0},
    {TEXT_11, 0},
    {NULL, 0},
    {TEXT_7, 0},
    {NULL, 0},
    {TEXT_10, 0},
    {NULL, 0},
    {TEXT_11, 0},
    {NULL, 0},
    {TEXT_12, 0},
    {NULL, 0},
    {TEXT_8, 0},
    {TEXT_9, 0},
    {NULL, 0},
    {TEXT_7, 0},
    {NULL, 0},
    {TEXT_11, 0},
    {NULL, 0},
    {TEXT_7, 0},
    {NULL, 0},
    {TEXT_10, 0},
    {NULL, 0},
    {TEXT_11, 0},
    {NULL, 0},
};

/* row 13: the box at the casino area's gate */
static const Sint16 hint_script_13[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | 9,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_SS_EXIT_SEWER,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_FLAGS, SCRIPT_ON | 53,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_TAILS_CLEAR_WINDY,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_KNUCKLES_MR_WEST_ROCK,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_TAILS_CLEAR_HIGHWAY,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_END,
};

static const struct script_page hint_pages_13[] = {
    {TEXT_13, 0},
    {NULL, 0},
    {TEXT_14, 0},
    {NULL, 0},
    {TEXT_15, 0},
    {NULL, 0},
    {TEXT_13, 0},
    {NULL, 0},
    {TEXT_14, 0},
    {NULL, 0},
    {TEXT_15, 0},
    {NULL, 0},
    {TEXT_13, 0},
    {NULL, 0},
    {TEXT_14, 0},
    {NULL, 0},
    {TEXT_15, 0},
    {NULL, 0},
    {TEXT_13, 0},
    {NULL, 0},
    {TEXT_16, 0},
    {NULL, 0},
    {TEXT_16, 0},
    {NULL, 0},
};

/* row 14: the box at the Casinopolis doors */
static const Sint16 hint_script_14[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_SS_ENTRANCE_CASINO,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_TAILS_SS_ENTRANCE_CASINO,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_KNUCKLES_SS_ENTRANCE_CASINO,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_END,
};

static const struct script_page hint_pages_14[] = {
    {TEXT_17, 0},
    {NULL, 0},
    {TEXT_18, 0},
    {TEXT_19, 0},
    {NULL, 0},
    {TEXT_17, 0},
    {NULL, 0},
    {TEXT_18, 0},
    {TEXT_19, 0},
    {NULL, 0},
    {TEXT_17, 0},
    {NULL, 0},
    {TEXT_18, 0},
    {TEXT_19, 0},
    {NULL, 0},
    {TEXT_20, 0},
    {NULL, 0},
    {TEXT_20, 0},
    {NULL, 0},
    {TEXT_20, 0},
    {NULL, 0},
};

/* row 15: the box at the gate to the beach */
static const Sint16 hint_script_15[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_END,
};

static const struct script_page hint_pages_15[] = {
    {TEXT_21, 0},
    {NULL, 0},
    {TEXT_22, 0},
    {NULL, 0},
    {TEXT_22, 0},
    {NULL, 0},
    {TEXT_22, 0},
    {NULL, 0},
    {TEXT_21, 0},
    {NULL, 0},
    {TEXT_23, 0},
    {NULL, 0},
};

/* row 16: the box at the staff passage's card reader */
static const Sint16 hint_script_16[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_SS_ENTRANCE_HIGHWAY,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_TAILS_SS_ENTRANCE_HIGHWAY,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_END,
};

static const struct script_page hint_pages_16[] = {
    {TEXT_24, 0},
    {NULL, 0},
    {TEXT_25, 0},
    {NULL, 0},
    {TEXT_26, 0},
    {NULL, 0},
    {TEXT_27, 0},
    {NULL, 0},
    {TEXT_27, 0},
    {NULL, 0},
    {TEXT_27, 0},
    {NULL, 0},
    {TEXT_27, 0},
    {NULL, 0},
    {TEXT_27, 0},
    {NULL, 0},
};

/* row 17: the box at the Chao Garden elevator. the row has no script, so it
 * shows every character the same page */
static const struct script_page hint_pages_17[] = {
    {TEXT_28, 0},
    {NULL, 0},
};

/* row 18: a box with each character's voiced hints for its story so far */
static const Sint16 hint_script_18[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_CLEAR_BEACH,
    SCRIPT_FLAGS, FLAG_SONIC_CLEAR_EGGMOBILE1,
    SCRIPT_VOICE, 423,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_CLEAR_EGGMOBILE1,
    SCRIPT_FLAGS, FLAG_SONIC_CLEAR_WINDY,
    SCRIPT_VOICE, 756,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_CLEAR_WINDY,
    SCRIPT_FLAGS, FLAG_SONIC_LIGHT_SHOES,
    SCRIPT_VOICE, 414,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_LIGHT_SHOES,
    SCRIPT_FLAGS, FLAG_SONIC_CLEAR_CASINO,
    SCRIPT_VOICE, 415,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_CLEAR_CASINO,
    SCRIPT_FLAGS, FLAG_SONIC_MR_ENTRANCE_ICECAP,
    SCRIPT_VOICE, 416,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_MR_ENTRANCE_ICECAP,
    SCRIPT_FLAGS, FLAG_SONIC_CLEAR_SHOOTING1,
    SCRIPT_VOICE, 756,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_CLEAR_SHOOTING1,
    SCRIPT_EVENTS, 18,
    SCRIPT_VOICE, 808,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_EVENTS, SCRIPT_ON | 18,
    SCRIPT_FLAGS, FLAG_SONIC_CLEAR_HIGHWAY,
    SCRIPT_VOICE, 803,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_CLEAR_HIGHWAY,
    SCRIPT_EVENTS, 21,
    SCRIPT_VOICE, 805,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_EVENTS, SCRIPT_ON | 21,
    SCRIPT_FLAGS, FLAG_SONIC_CLEAR_MOUNTAIN,
    SCRIPT_VOICE, 419,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_CLEAR_CHAOS6,
    SCRIPT_FLAGS, FLAG_SONIC_CLEAR_FINAL_EGG,
    SCRIPT_VOICE, 756,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_EVENTS, SCRIPT_ON | 50,
    SCRIPT_FLAGS, FLAG_TAILS_CLEAR_WINDY,
    SCRIPT_VOICE, 756,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_TAILS_CLEAR_WINDY,
    SCRIPT_FLAGS, FLAG_TAILS_CLEAR_CASINO,
    SCRIPT_VOICE, 415,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_TAILS_CLEAR_CASINO,
    SCRIPT_FLAGS, FLAG_TAILS_MR_ENTRANCE_ICECAP,
    SCRIPT_VOICE, 416,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_TAILS_MR_ENTRANCE_ICECAP,
    SCRIPT_FLAGS, FLAG_TAILS_CLEAR_SHOOTING2,
    SCRIPT_VOICE, 756,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_EVENTS, SCRIPT_ON | 77,
    SCRIPT_FLAGS, FLAG_TAILS_CLEAR_EGGMOBILE2,
    SCRIPT_VOICE, 420,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_EVENTS, SCRIPT_ON | 130,
    SCRIPT_FLAGS, FLAG_KNUCKLES_CLEAR_HIGHWAY,
    SCRIPT_VOICE, 421,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_KNUCKLES_CLEAR_HIGHWAY,
    SCRIPT_FLAGS, FLAG_KNUCKLES_CLEAR_CASINO,
    SCRIPT_VOICE, 415,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_KNUCKLES_CLEAR_CASINO,
    SCRIPT_FLAGS, FLAG_KNUCKLES_CLEAR_CHAOS2,
    SCRIPT_VOICE, 803,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_KNUCKLES_CLEAR_CHAOS2,
    SCRIPT_EVENTS, 146,
    SCRIPT_VOICE, 756,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_EVENTS, SCRIPT_ON | 97,
    SCRIPT_EVENTS, 98,
    SCRIPT_VOICE, 807,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_EVENTS, SCRIPT_ON | 98,
    SCRIPT_FLAGS, FLAG_AMY_CLEAR_TWINKLE_PARK,
    SCRIPT_VOICE, 417,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_EVENTS, SCRIPT_ON | 109,
    SCRIPT_FLAGS, FLAG_AMY_CLEAR_FINAL_EGG,
    SCRIPT_VOICE, 756,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_AMY_CLEAR_FINAL_EGG,
    SCRIPT_FLAGS, FLAG_AMY_CLEAR_EGGMANROBO,
    SCRIPT_VOICE, 757,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_EVENTS, SCRIPT_ON | 208,
    SCRIPT_FLAGS, FLAG_BIG_CLEAR_TWINKLE_PARK,
    SCRIPT_VOICE, 414,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_BIG_CLEAR_TWINKLE_PARK,
    SCRIPT_FLAGS, FLAG_BIG_MR_ENTRANCE_ICECAP,
    SCRIPT_VOICE, 416,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_BIG_CLEAR_ICECAP,
    SCRIPT_EVENTS, 211,
    SCRIPT_VOICE, 756,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_EVENTS, SCRIPT_ON | 211,
    SCRIPT_FLAGS, FLAG_BIG_CLEAR_BEACH,
    SCRIPT_VOICE, 759,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_EVENTS, SCRIPT_ON | 183,
    SCRIPT_EVENTS, 184,
    SCRIPT_VOICE, 759,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_EVENTS, SCRIPT_ON | 195,
    SCRIPT_EVENTS, 197,
    SCRIPT_VOICE, 756,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_EVENTS, SCRIPT_ON | 197,
    SCRIPT_FLAGS, FLAG_E102_CLEAR_E101_R,
    SCRIPT_VOICE, 757,
    SCRIPT_END,
};

static const struct script_page hint_pages_18[] = {
    {TEXT_29, 180},
    {NULL, 0},
    {TEXT_30, 180},
    {NULL, 0},
    {TEXT_31, 180},
    {NULL, 0},
    {TEXT_32, 180},
    {NULL, 0},
    {TEXT_33, 200},
    {NULL, 0},
    {TEXT_30, 180},
    {NULL, 0},
    {TEXT_34, 180},
    {NULL, 0},
    {TEXT_35, 180},
    {NULL, 0},
    {TEXT_36, 180},
    {NULL, 0},
    {TEXT_37, 180},
    {NULL, 0},
    {TEXT_30, 180},
    {NULL, 0},
    {TEXT_30, 180},
    {NULL, 0},
    {TEXT_32, 180},
    {NULL, 0},
    {TEXT_33, 180},
    {NULL, 0},
    {TEXT_30, 180},
    {NULL, 0},
    {TEXT_38, 180},
    {NULL, 0},
    {TEXT_39, 180},
    {NULL, 0},
    {TEXT_40, 180},
    {NULL, 0},
    {TEXT_35, 180},
    {NULL, 0},
    {TEXT_30, 180},
    {NULL, 0},
    {TEXT_41, 180},
    {NULL, 0},
    {TEXT_42, 180},
    {NULL, 0},
    {TEXT_30, 180},
    {NULL, 0},
    {TEXT_43, 180},
    {NULL, 0},
    {TEXT_31, 180},
    {NULL, 0},
    {TEXT_33, 180},
    {NULL, 0},
    {TEXT_30, 180},
    {NULL, 0},
    {TEXT_44, 200},
    {NULL, 0},
    {TEXT_44, 200},
    {NULL, 0},
    {TEXT_30, 180},
    {NULL, 0},
    {TEXT_43, 180},
    {NULL, 0},
};

/* row 19: a box with Sonic's tips on the light dash */
static const Sint16 hint_script_19[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_LIGHT_SHOES,
    SCRIPT_FLAGS, FLAG_SONIC_CLEAR_CASINO,
    SCRIPT_VOICE, 511,
    SCRIPT_END,
};

static const struct script_page hint_pages_19[] = {
    {TEXT_45, 180},
    {TEXT_46, 180},
    {NULL, 0},
};

const struct script_row station_square_script_table[20] = {
    {(Sint16 *)people_script_0, (void *)people_pages_0},
    {(Sint16 *)people_script_1, NULL},
    {(Sint16 *)people_script_2, (void *)people_pages_2},
    {(Sint16 *)people_script_3, (void *)people_pages_3},
    {(Sint16 *)people_script_4, (void *)people_pages_4},
    {(Sint16 *)people_script_5, NULL},
    {(Sint16 *)people_script_6, (void *)people_pages_6},
    {(Sint16 *)people_script_7, NULL},
    {(Sint16 *)people_script_8, (void *)people_pages_8},
    {(Sint16 *)people_script_9, (void *)people_pages_9},
    {(Sint16 *)people_script_10, (void *)people_pages_10},
    {(Sint16 *)people_script_11, NULL},
    {(Sint16 *)hint_script_12, (void *)hint_pages_12},
    {(Sint16 *)hint_script_13, (void *)hint_pages_13},
    {(Sint16 *)hint_script_14, (void *)hint_pages_14},
    {(Sint16 *)hint_script_15, (void *)hint_pages_15},
    {(Sint16 *)hint_script_16, (void *)hint_pages_16},
    {NULL, (void *)hint_pages_17},
    {(Sint16 *)hint_script_18, (void *)hint_pages_18},
    {(Sint16 *)hint_script_19, (void *)hint_pages_19},
};
/* clang-format on */

/* the people placed, people_count of them, in the order of placement */
static int people_count = 0;
static struct task *people[PEOPLE_MAX];

void station_square_people_place(void)
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
                work->ang_y = spot->ang_y;
                /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
                work->kind = spot->kind;
                work->slot = people_count;
                people[people_count] = task;
                people_count++;
            }
        }
        spot++;
    } while (--left != 0 && people_count < PEOPLE_MAX);
    heap_free(pick);
}

/* destroy the people in reverse order of placement */
void station_square_people_remove(void)
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
    /* @hack redundant: pos tested a second time (ADD #32,R2 at 0x0c9119a0 and
     * 0x0c9119d0); tested once, 24 of 51 words differ */
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

/* load the kind's textures, add the person's talk and start its collision.
 * a kind with welds saves their vertices and takes the matrices that pose
 * them from the heap */
static void person_start(struct task *task)
{
    struct person_work *work = PERSON(task);
    const struct person_kind *kind = &people_kinds[work->kind];

    texture_load_character(kind->character);
    talk_add(task, &work->pos, &work->ang_y, &work->talking,
             kind->shape.radius + TALK_REACH, TALK_LEVEL);
    work->frame = 0.0F;
    collision_init(task, &kind->shape, COLLISION_OBJECT, 1, 1);
    if (kind->welds != NULL) {
        welds_save(kind->welds);
        work->weld_matrices = heap_alloc(WELD_MATRICES * sizeof(NJS_MATRIX));
    }
}

/* advance the action a frame, back to 0 once it passes the last, and draw
 * the person. on a frame's first step, a kind with welds then poses them at
 * the new frame */
static void person_animate(struct task *task)
{
    struct person_work *work = PERSON(task);
    const struct person_kind *kind = &people_kinds[work->kind];

    /* @hack spelling: the frame's wrap as one conditional expression, which
     * loads kind->welds into R3 after the draw (MOV.L @(R0,R13),R3 at
     * 0x0c911b30). as an if, R2 takes it and 2 of 63 words differ */
    ((work->frame = work->frame + 1.0F) > kind->action->motion->nbFrame - 1.0F)
        ? (void)(work->frame = 0.0F)
        : (void)0;
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

/* on a frame's first step, while no event has control and the sphere of
 * VIEW_RADIUS about the person is in view, draw the person and its shadow */
static void person_draw(struct task *task)
{
    struct person_work *work = PERSON(task);
    const struct person_kind *kind;
    Float lift;
    NJS_TEXLIST *texlist;
    NJS_ACTION *action;
    Angle ang;

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
        TURN_VIA(njRotateY, work->ang_y, ang);
        if (work->kind == PERSON_FROG) {
            TURN_VIA(njRotateZ, work->ang_z, ang);
        }
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
    texture_release_character(kind->character);
    heap_free(work->pick);
    talk_remove(task);
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
        weld->vertices = heap_alloc(weld->count * (4 * sizeof(NJS_POINT3)));
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
    NJS_POINT3 *saved;
    NJS_MODEL *model_a;
    NJS_MODEL *model_b;
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
