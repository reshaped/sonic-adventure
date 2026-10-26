/* Tails' race against Sonic or Eggman, its progress bar and its result
 * banner */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/glare.h"
#include "effect/panlight.h"
#include "main.h"
#include "object/common.h"
#include "object/helpers.h"
#include "player/player.h"
#include "player/race.h"
#include "player/sonic.h"
#include "progress/emblem.h"
#include "save/save.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg06/race_course.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

#define TEXT_1_LENGTH 12
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 15
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 8
#define TEXT_10_LENGTH 8
#define TEXT_11_LENGTH 8
#define TEXT_12_LENGTH 12
#include "player/race/text.inc"

/* @hack type: pad_jump_buttons under a second name, a Sint32 the link
 * places at its address. at GOAL_ARRIVE, SHC then loads the 0 for
 * aim->jumps into R2 before the store to press (MOV #0,R2 at 0x8c08553c);
 * through pad.h's Uint32, or a cast of it, R3 takes the 0 after the store,
 * and 3 of race_sonic_drive's 706 words differ */
extern Sint32 pad_jump_buttons_signed;

enum course {
    COURSE_WINDY, /* Windy Valley's third act, against Sonic */
    COURSE_HIGHWAY, /* Speed Highway's first, against Eggman */
    COURSE_SKY_DECK, /* Sky Deck's first, against Sonic */
    COURSE_ICECAP, /* Ice Cap's third, against Sonic on his board */
    COURSE_CASINO, /* Casinopolis' second, against Sonic */
    COURSE_COUNT
};

/* the kinds of point, in the low byte of a point's attr. a kind says how
 * the race sets Sonic's stick on the way to the point. it also says when
 * he reaches the point. steered means the stick points him at the point,
 * and released means the stick is let go. the stretch is the line from
 * the point before to the point. past means beyond the point along the
 * stretch. near means within 5 of the point, as NEAR measures. near across
 * means within 5 in x and z alone */
#define POINT_KIND(attr) ((Uint8)(attr))
enum point_kind {
    POINT_RUN, /* steered at race_sonic_stroke's stroke, until past or near */
    POINT_RUN_LEVEL, /* as POINT_RUN, with past measured in x and z alone */
    POINT_END, /* the last point, beyond the goal, which he never heads for */
    POINT_COAST_NEAR, /* the stick released, until near */
    POINT_DASH, /* steered with the stick all the way, until past */
    POINT_COAST, /* the stick released, until past */
    POINT_EASE, /* steered less as he nears it, until past or near */
    POINT_LAUNCHED, /* steered less as he nears it, until he's launched */
    POINT_REQUESTED, /* as POINT_LAUNCHED, until his mode is requested */
    POINT_PUSH, /* the stick pushed the point's way, until near across */
    /* steered less as he nears it, with a jump once near across, until he
     * hangs from a ledge */
    POINT_LEDGE,
    POINT_DASH_NEAR, /* steered with the stick all the way, until near */
    POINT_LAND, /* the stick released, until he lands */
    /* set in place of a POINT_RUN or POINT_RUN_LEVEL as he stalls on a
     * slope. the race presses action, then holds it until spin_steps passes
     * SPIN_STEPS */
    POINT_SPIN_UP,
    POINT_PATH, /* the point's way while on a path, else steered, until past */
    /* steered in the air, until past. on the ground, he leaps for it */
    POINT_AIR,
    /* runs up to the point and stands ready there. he reaches it once the
     * player's point passes his last, or once the point's wait runs out. in
     * mission A, he reaches it at once */
    POINT_WAIT,
    /* steered on his board, until past or near. while the player leads, the
     * race pushes him on until his last point is BOARD_PUSH_END. in mission
     * A, it always pushes him. after BOARD_STEPS steps on the way, he jumps
     * each step. if the player leads by BOARD_SKIP_LEAD points or more, the
     * race moves him on instead */
    POINT_BOARD,
    POINT_GOAL, /* jumps there until the race is over, and never passes it */
    /* set in place of the kind as he leaps, until race_sonic_fly ends it */
    POINT_LEAP
};

/* the flags in the bits above a point's kind. with POINT_NEVER_STUCK,
 * race_sonic_stuck never holds on the way to the point */
#define POINT_NEVER_STUCK 0x01000000
/* a stroke is how far the stick is pushed, as struct pad_stick has it. on
 * the way to the point, race_sonic_stroke gives the stroke these flags ask
 * for. with neither, it gives race_sonic_pace */
#define POINT_STROKE_MOST 0x02000000 /* a stroke of 0.8 */
/* a stroke of 1, which wins over POINT_STROKE_MOST */
#define POINT_STROKE_FULL 0x04000000
/* as he reaches the POINT_BOARD before this point, he gets off his board.
 * his own pad stays off */
#define POINT_STAND 0x08000000
/* as he reaches the point, he leaps on to the next one */
#define POINT_LEAP_NEXT 0x10000000
#define POINT_JUMP_OFF 0x20000000 /* as he reaches the point, he jumps */
/* the most a POINT_WAIT waits for the player, in units of WAIT_STEPS, or 0
 * for no limit */
#define POINT_WAIT_SHIFT 20
#define POINT_WAIT_TIME(time) ((time) << POINT_WAIT_SHIFT)
#define POINT_WAIT_MASK POINT_WAIT_TIME(0xf)
#define WAIT_STEPS 120
/* the stick_way a POINT_PUSH or POINT_PATH pushes */
#define POINT_WAY(way) ((Uint32)(way) << 30)
#define POINT_STICK(attr) (((attr) >> 30) & 3)

/* the ways of race_sticks */
enum stick_way { WAY_RIGHT, WAY_LEFT, WAY_UP, WAY_DOWN };

/* distances, squared as njScalor2 measures them */
#define NEAR (5.0F * 5.0F)
#define ASTRAY (1000.0F * 1000.0F)
#define FINISH_RANGE (150.0F * 150.0F)
/* the distance across within which race_stroke eases the stick */
#define EASE_REACH 40.0F
/* how high above a point the race puts Sonic as it moves him on */
#define PUT_HEIGHT 5.0F
/* the dot product of ahead and the stretch at which race_sonic_reach
 * counts a point reached past or near as past, rather than at 0 */
#define PAST_EARLY 0.001F
/* the most points Sonic may fall behind the player before the race moves
 * him on */
#define MOST_BEHIND 40
#define MOST_BEHIND_A 20 /* in mission A */
#define LEAP_STEPS 60 /* a leap's steps */
#define SPIN_STEPS 60 /* a spin up holds action until spin_steps passes it */
#define STUCK_STEPS 120 /* how many steps back race_sonic_stuck looks */
/* the most steps Sonic takes toward a point before the race leaps him for
 * it, by the point's kind, as race_sonic_drive's switch gives them */
#define MOST_STEPS_RUN 420
#define MOST_STEPS_EASE 600
#define MOST_STEPS_PUSH 900
#define MOST_STEPS_COAST 1200
#define BOARD_STEPS 180 /* the steps toward a POINT_BOARD before he jumps */
/* the player's lead, in points, from which the race moves him on instead
 * of jumping */
#define BOARD_SKIP_LEAD 20
#define BOARD_PUSH_END 210 /* his point from which the race stops pushing */

/* the race task's modes. only Eggman's race sets RACE_OVER */
enum race_mode { RACE_START, RACE_RUN, RACE_OVER };

/* Sonic's submodes at a POINT_WAIT. WAIT_ARRIVE steers him until past or
 * near, then sets player 1's mode to PLAYER_MODE_READY. WAIT_READY sets it
 * again whenever its timer passes READY_STEPS. the timer then starts again
 * from 0, so that is every 302 steps */
enum wait_submode { WAIT_ARRIVE, WAIT_READY };
#define READY_STEPS 300

/* Sonic's submodes at the goal, where he jumps until the race is over */
enum goal_submode {
    GOAL_ARRIVE, /* to GOAL_DONE if the race is over, else to a jump */
    GOAL_JUMP, /* steered with jump held, until he lands */
    /* on the ground, until the timer passes GOAL_REST. he then jumps again,
     * but from his fifth landing on, he leaps for GOAL_HEIGHT above the
     * goal. once the race is over, he stays here, and a push moves him off
     * Tails while he's near */
    GOAL_LANDED,
    GOAL_LEAP, /* in the leap, until race_sonic_fly ends it */
    GOAL_FALL, /* falling from the leap, until he lands */
    GOAL_DONE
};
#define GOAL_REST 32
#define GOAL_HEIGHT 45.0F

/* the race task's work vars. index 0 is for the player, Tails, and 1 for
 * the racer, Sonic or Eggman */
struct race_vars {
    Uint16 last[2]; /* the index of each line's last point */
    Sint16 point[2]; /* the player's nearest, and the racer's last reached */
    /* the steps since Sonic reached point[1], or since the race last made
     * him leap for the next */
    Sint16 steps;
    Sint16 spin_steps; /* the steps Sonic has held action to spin up */
};

#define VARS(work) ((struct race_vars *)&(work)->vars)

/* Sonic's way between two points of his line: from one, to the one he
 * heads for */
struct race_aim {
    NJS_POINT3 from;
    NJS_POINT3 to;
    NJS_VECTOR ahead; /* from him to the point he heads for */
    Float distance; /* ahead's length, squared */
    /* the attr of the point he heads for, or POINT_SPIN_UP or POINT_LEAP in
     * its place */
    Sint32 attr;
    Sint32 resume; /* the attr to go back to after a spin up or leap */
    Angle heading; /* the stick's angle as race_sonic_heading last set it */
    Float speed; /* his speed ahead, the step before */
    Sint32 jumps; /* the times he has jumped again on landing at the goal */
};

struct race_eggman {
    NJS_POINT3 from;
    NJS_POINT3 to;
    NJS_VECTOR speed;
    Angle bob_phase;
    Float bob; /* the height the bob adds now */
};

/* a spark of Eggman's jets. it starts at from, at the speed to - from.
 * both points are in his frame */
struct race_spark {
    struct dust_params *params;
    NJS_POINT3 from;
    NJS_POINT3 to;
};

/* the race's sayings. the voice tables give sayings 5 and 6 voices too,
 * but nothing says them */
enum race_saying {
    SAY_RACER_CLOSES = 0, /* the racer closes to one point behind Tails */
    SAY_TAILS_CLOSES = 1, /* Tails closes to one point behind the racer */
    SAY_LEADING = 2, /* Tails takes the lead */
    SAY_TRAILING = 3, /* Tails loses the lead */
    SAY_FAR_BEHIND = 4, /* the lead changes, with Tails over 8 points behind */
    SAY_START = 7 /* the racer's point comes to 1 or more */
};

/* the voices wait until the racer's point is 1 or more, then say
 * SAY_START */
enum race_voices_mode { VOICES_WAIT, VOICES_RACE };

/* the voices' state, in the race task's extra */
struct race_voices {
    enum race_voices_mode mode;
    enum race_saying last;
    /* a bit for each saying, set while its second voice was the last it
     * played */
    Sint32 turns;
    Sint32 wait; /* steps until another saying can play */
};

#define VOICES(task) ((struct race_voices *)(task)->extra)

/* the steps after a saying before another can play */
#define SAYING_GAP 180

/* a texture task's vars: the texlist it loads, and releases as it's
 * destroyed */
struct race_texture_vars {
    Uint8 unk_00[8];
    NJS_TEXLIST *texlist;
};

#define TEXTURE(work) ((struct race_texture_vars *)&(work)->vars)

/* the result banner's vars */
struct race_banner_vars {
    Uint8 unk_00[4];
    Float sprite; /* the index of its first race_sprites entry, as a Float */
    Float scale;
};

#define BANNER(work) ((struct race_banner_vars *)&(work)->vars)

enum race_banner_mode { BANNER_INIT, BANNER_GROW, BANNER_SHRINK, BANNER_SHOW };

/* race_sprites' entries, each a MILESRACE texture of its own. a banner is
 * BANNER_SPRITES entries from its first */
#define RACE_BAR 0
#define RACE_SONIC 1
#define RACE_TAILS 2
#define RACE_YOU_WIN 3
#define RACE_YOU_LOSE 7
#define RACE_EGGMAN 11
#define BANNER_SPRITES 4

/* the egg mobile, its jets and their texlist, which Speed Highway's module
 * holds. race_eggman_texture_step loads MRACE_EGGMOBLE into the texlist */
extern NJS_TEXLIST highway_eggmobile_texlist;
extern NJS_OBJECT highway_eggmobile_object;
extern NJS_OBJECT highway_eggmobile_jets_object;

static void race_start(struct task *task);
static void race_step(struct task *task);
static void race_start_windy(struct task_work *work, struct task_work *sonic,
                             struct task_work *player);
static void race_run(struct task *task, struct task_work *sonic,
                     struct task_work *player);
static void race_start_icecap(struct task_work *work, struct task_work *sonic,
                              struct task_work *player);
static void race_start_sky_deck(struct task_work *work, struct task_work *sonic,
                                struct task_work *player);
static void race_start_casino(struct task_work *work, struct task_work *sonic,
                              struct task_work *player);
static void race_eggman_start(struct task *task);
static void race_eggman_step(struct task *task);
static void race_eggman_init(struct task *task, struct race_eggman *eggman);
static void race_eggman_run(struct task *task, struct race_eggman *eggman);
static void race_aim_at(Sint16 point);
static void race_eggman_place(struct task_work *work, Sint16 point,
                              struct race_eggman *eggman);
static void race_set_lines(void);
static void race_set_lasts(struct task_work *work);
static Sint16 race_player_point(struct task_work *work,
                                struct task_work *player);
static Sint16 race_nearest_point(NJS_POINT3 *pos,
                                 const struct race_point *point, Sint16 count);
static Sint16 race_sonic_reach(struct task_work *work, struct task_work *sonic,
                               struct race_aim *aim);
static void race_sonic_drive(struct task_work *work, struct task_work *sonic,
                             struct race_aim *aim);
static void race_sonic_leap(struct task_work *sonic, NJS_POINT3 *to, int steps);
static Bool race_sonic_fly(struct task_work *sonic);
static Bool race_sonic_near_across(struct race_aim *aim);
static Angle race_sonic_heading(struct task_work *work, struct task_work *sonic,
                                struct race_aim *aim);
static Float race_stroke(NJS_VECTOR *ahead, Float reach, Float most);
static Bool race_sonic_stuck(struct task_work *sonic);
static Float race_sonic_stroke(struct race_aim *aim);
static Sint16 race_sonic_next(struct task_work *work, struct task_work *sonic,
                              struct race_aim *aim, Sint16 point);
static void race_eggman_draw(struct task *task);
static void race_eggman_spark(struct task *task, struct race_spark *spark);
static void race_eggman_glare(struct task *task);
static Sint16 race_eggman_reach(struct task_work *work,
                                struct race_eggman *eggman);
static void race_eggman_steer(struct task_work *work,
                              struct race_eggman *eggman);
static void race_eggman_turn(struct task_work *work,
                             struct race_eggman *eggman);
static void race_eggman_bob(struct task_work *work, struct race_eggman *eggman);
static void race_voice_step(struct task *task, Sint16 player_point,
                            Sint16 racer_point);
static void race_say(struct race_voices *voices, enum race_saying say);
static Bool race_check_mission_a(void);
static void race_texture_destroy(struct task *task);
static void race_draw_bar(Float player, Float player_last, Float racer,
                          Float racer_last);
static void race_banner_draw(struct task *task);

static enum race_outcome race_result;
static enum course race_course;
static struct jump race_leap;
static NJS_SPRITE race_sprite;
/* the course's lines: the player's, and the racer's */
static const struct race_point *race_player_line;
static const struct race_point *race_racer_line;
static Sint32 dat_8c7a142c[3];
static struct race_eggman race_eggman;
static struct race_aim race_aim;
static Sint32 dat_8c7a14a0;
/* Sonic's stroke at a point whose flags ask for none, as
 * race_sonic_stroke gives it. a POINT_LEDGE eases the stick from it */
static Float race_sonic_pace;
/* Eggman's top speed, and the speed he gains toward his point each step */
static Float race_eggman_top_speed;
static Float race_eggman_turn_speed;

/* the courses but Sky Deck's, which its module holds */
static const struct race_point race_windy_points[249] = {
    {POINT_RUN, {1096.0F, -332.0F, -1255.0F}},
    {POINT_WAIT, {1045.0F, -332.0F, -1226.0F}},
    {POINT_RUN | POINT_NEVER_STUCK | POINT_STROKE_FULL,
     {996.0F, -332.0F, -1195.0F}},
    {POINT_RUN, {962.32142F, -338.2938F, -1174.672F}},
    {POINT_RUN, {943.7489F, -338.81177F, -1161.6583F}},
    {POINT_RUN, {899.52723F, -353.98337F, -1133.6471F}},
    {POINT_RUN, {835.77503F, -378.40931F, -1093.8101F}},
    {POINT_RUN, {782.62482F, -398.7733F, -1060.5977F}},
    {POINT_RUN, {751.26642F, -400.53937F, -1041.8672F}},
    {POINT_RUN, {717.3539F, -400.2608F, -1021.7952F}},
    {POINT_RUN, {680.54914F, -400.0F, -1003.02033F}},
    {POINT_RUN, {636.0793F, -400.0F, -1003.1509F}},
    {POINT_RUN, {589.11213F, -400.0F, -1003.28882F}},
    {POINT_RUN, {543.6053F, -400.0F, -1017.49872F}},
    {POINT_RUN, {499.1723F, -400.0F, -1044.3078F}},
    {POINT_RUN, {464.63837F, -400.0F, -1083.2304F}},
    {POINT_RUN, {440.25187F, -400.0F, -1134.453F}},
    {POINT_RUN, {427.3782F, -398.66657F, -1191.7225F}},
    {POINT_RUN, {423.87256F, -381.487F, -1250.4305F}},
    {POINT_RUN, {416.2677F, -393.5428F, -1312.3779F}},
    {POINT_RUN, {417.25071F, -405.1922F, -1373.2123F}},
    {POINT_RUN, {476.11332F, -406.0F, -1400.0F}},
    {POINT_RUN, {546.8131F, -406.0F, -1400.0F}},
    {POINT_RUN, {619.2286F, -406.0099F, -1391.3772F}},
    {POINT_RUN, {686.43873F, -407.15436F, -1368.4732F}},
    {POINT_RUN, {735.0849F, -411.83176F, -1308.9864F}},
    {POINT_RUN, {751.1987F, -430.43012F, -1233.3349F}},
    {POINT_RUN, {775.7258F, -454.31431F, -1159.3062F}},
    {POINT_RUN, {823.0727F, -483.4312F, -1097.1252F}},
    {POINT_RUN, {888.9681F, -519.3898F, -1059.4897F}},
    {POINT_RUN, {963.56293F, -560.6595F, -1056.4461F}},
    {POINT_RUN, {1030.1788F, -600.22162F, -1094.3356F}},
    {POINT_RUN, {1066.0064F, -634.7153F, -1167.0124F}},
    {POINT_RUN, {1053.8095F, -671.6165F, -1248.9394F}},
    {POINT_RUN, {998.0414F, -716.04523F, -1309.072F}},
    {POINT_RUN, {913.9386F, -755.99231F, -1331.2991F}},
    {POINT_RUN, {822.621F, -789.9224F, -1313.8115F}},
    {POINT_RUN, {742.9421F, -833.5504F, -1268.8586F}},
    {POINT_RUN, {681.2427F, -879.0065F, -1199.4045F}},
    {POINT_RUN, {649.4559F, -919.13752F, -1108.9162F}},
    {POINT_RUN, {640.0F, -936.0191F, -1002.96491F}},
    {POINT_RUN, {640.0F, -937.9227F, -893.4198F}},
    {POINT_RUN, {640.0F, -942.99421F, -782.3214F}},
    {POINT_RUN, {640.0F, -960.2164F, -670.46503F}},
    {POINT_RUN, {640.0F, -988.96113F, -559.88282F}},
    {POINT_RUN, {640.0F, -1031.5585F, -451.23041F}},
    {POINT_RUN, {640.0F, -1096.8554F, -353.4534F}},
    {POINT_RUN, {640.0F, -1171.434F, -259.41297F}},
    {POINT_RUN, {640.0F, -1259.121F, -174.83851F}},
    {POINT_RUN, {640.0F, -1350.5403F, -91.923F}},
    {POINT_RUN, {640.0F, -1446.043F, -11.087F}},
    {POINT_RUN, {640.0F, -1544.8681F, 68.237801F}},
    {POINT_RUN, {640.0F, -1645.5994F, 147.74491F}},
    {POINT_RUN, {640.0F, -1748.3503F, 227.2167F}},
    {POINT_RUN, {640.0F, -1852.6211F, 307.2497F}},
    {POINT_RUN, {640.0F, -1958.1008F, 388.2141F}},
    {POINT_RUN, {640.0F, -2064.5606F, 470.3704F}},
    {POINT_RUN, {640.0F, -2170.6475F, 555.37623F}},
    {POINT_RUN, {640.0F, -2275.3711F, 644.23993F}},
    {POINT_RUN, {640.0F, -2377.1688F, 738.69422F}},
    {POINT_RUN, {639.99982F, -2455.7825F, 854.44983F}},
    {POINT_RUN, {639.99982F, -2507.4212F, 983.60322F}},
    {POINT_RUN, {640.602F, -2533.6712F, 1124.1392F}},
    {POINT_RUN, {648.02662F, -2552.3917F, 1266.245F}},
    {POINT_RUN, {706.11451F, -2614.253F, 1383.6736F}},
    {POINT_RUN, {812.2659F, -2678.116F, 1459.5424F}},
    {POINT_RUN, {950.15662F, -2711.118F, 1483.5567F}},
    {POINT_LAUNCHED, {1135.0F, -2709.0F, 1488.0F}},
    {POINT_RUN_LEVEL | POINT_STROKE_FULL, {1158.0F, -2641.0F, 1434.0F}},
    {POINT_RUN, {1188.0F, -2641.0F, 1437.0F}},
    {POINT_WAIT | POINT_WAIT_TIME(10), {1283.7552F, -2655.0F, 1398.4761F}},
    {POINT_REQUESTED | POINT_NEVER_STUCK, {1356.0F, -2653.0F, 1327.7433F}},
    {POINT_RUN | POINT_NEVER_STUCK, {1468.0F, -2634.0F, 1346.0F}},
    {POINT_RUN, {1576.7562F, -2615.3113F, 1337.5864F}},
    {POINT_RUN, {1677.0F, -2550.0F, 1346.0F}},
    {POINT_RUN, {1888.0F, -2530.0F, 1337.0F}},
    {POINT_RUN, {2002.4931F, -2536.4161F, 1202.9207F}},
    {POINT_RUN, {2104.5127F, -2537.054F, 1081.3121F}},
    {POINT_RUN, {2207.1634F, -2539.642F, 958.9779F}},
    {POINT_RUN, {2310.2901F, -2550.8443F, 836.07593F}},
    {POINT_RUN, {2411.2015F, -2589.9454F, 715.815F}},
    {POINT_RUN, {2504.0728F, -2662.389F, 605.13532F}},
    {POINT_RUN, {2587.5208F, -2760.7347F, 505.6855F}},
    {POINT_RUN, {2662.9251F, -2876.1058F, 415.82251F}},
    {POINT_RUN, {2733.3282F, -3000.3543F, 331.9191F}},
    {POINT_RUN, {2801.0542F, -3129.2823F, 251.2059F}},
    {POINT_RUN, {2867.4378F, -3260.9927F, 172.09351F}},
    {POINT_RUN, {2933.5F, -3394.1702F, 93.3638F}},
    {POINT_RUN, {2999.6458F, -3528.2828F, 14.534F}},
    {POINT_RUN, {3066.4427F, -3662.6265F, -65.0712F}},
    {POINT_RUN, {3134.931F, -3795.8553F, -146.69231F}},
    {POINT_RUN, {3207.1546F, -3925.1104F, -232.7652F}},
    {POINT_RUN, {3288.9554F, -4040.9913F, -330.25171F}},
    {POINT_RUN, {3390.944F, -4107.63F, -451.79682F}},
    {POINT_RUN, {3500.5872F, -4132.024F, -582.46503F}},
    {POINT_RUN, {3612.4734F, -4135.423F, -715.2875F}},
    {POINT_RUN, {3714.0464F, -4054.0694F, -815.5411F}},
    {POINT_RUN, {3731.059F, -3918.5901F, -742.48542F}},
    {POINT_RUN, {3717.314F, -3981.1875F, -601.3448F}},
    {POINT_RUN, {3738.6802F, -4153.8711F, -583.07953F}},
    {POINT_RUN, {3804.5601F, -4292.925F, -649.138F}},
    {POINT_RUN, {3912.8748F, -4325.4771F, -786.0187F}},
    {POINT_RUN, {3943.397F, -4399.7496F, -941.26923F}},
    {POINT_RUN, {3894.3296F, -4477.1495F, -1091.2219F}},
    {POINT_RUN | POINT_STROKE_MOST, {3765.2015F, -4492.432F, -1212.3516F}},
    {POINT_RUN | POINT_STROKE_MOST, {3597.9622F, -4477.728F, -1229.472F}},
    {POINT_RUN | POINT_STROKE_MOST, {3585.8421F, -4469.2393F, -1074.8489F}},
    {POINT_RUN | POINT_STROKE_MOST, {3750.1146F, -4438.507F, -1028.4124F}},
    {POINT_RUN | POINT_STROKE_MOST, {3912.7496F, -4396.283F, -1093.8456F}},
    {POINT_RUN | POINT_STROKE_MOST, {4073.556F, -4376.6246F, -1062.0365F}},
    {POINT_RUN | POINT_STROKE_MOST, {4078.783F, -4425.274F, -901.33942F}},
    {POINT_RUN | POINT_STROKE_MOST, {3919.512F, -4465.861F, -848.39051F}},
    {POINT_RUN | POINT_STROKE_MOST, {3820.9935F, -4375.1285F, -955.3441F}},
    {POINT_RUN | POINT_STROKE_MOST, {3812.758F, -4341.835F, -1130.2858F}},
    {POINT_RUN | POINT_STROKE_MOST, {3861.1978F, -4415.4673F, -1286.0031F}},
    {POINT_RUN | POINT_STROKE_MOST, {3983.472F, -4456.5743F, -1414.7181F}},
    {POINT_LAUNCHED, {4065.0F, -4457.0F, -1527.0F}},
    {POINT_RUN_LEVEL | POINT_STROKE_FULL, {4117.0F, -4393.0F, -1499.0F}},
    {POINT_RUN, {4176.0F, -4393.0F, -1518.0F}},
    {POINT_RUN, {4237.672F, -4390.17F, -1571.0853F}},
    {POINT_RUN, {4249.0F, -4393.0F, -1638.0F}},
    {POINT_RUN, {4184.2911F, -4401.585F, -1733.6744F}},
    {POINT_RUN, {4157.0F, -4393.0F, -1770.0F}},
    {POINT_RUN | POINT_STROKE_FULL, {4154.363F, -4395.856F, -1908.6689F}},
    {POINT_RUN | POINT_STROKE_FULL, {4163.637F, -4320.238F, -2016.477F}},
    {POINT_RUN | POINT_STROKE_FULL, {4172.5801F, -4278.885F, -1898.2399F}},
    {POINT_RUN | POINT_STROKE_FULL, {4160.554F, -4302.7081F, -1760.5298F}},
    {POINT_RUN, {4157.0F, -4249.0F, -1691.0F}},
    {POINT_LAUNCHED, {4135.0F, -4114.0F, -1618.0F}},
    {POINT_COAST, {4274.0F, -3864.0F, -1847.0F}},
    {POINT_COAST, {4431.0F, -3465.0F, -2296.0F}},
    {POINT_COAST, {4914.0F, -3878.0F, -1995.0F}},
    {POINT_COAST, {5363.0F, -4248.0F, -1735.0F}},
    {POINT_COAST, {5259.0F, -4009.0F, -1860.0F}},
    {POINT_COAST, {4988.0F, -3373.0F, -2190.0F}},
    {POINT_RUN, {4988.0F, -3665.0F, -2190.0F}},
    {POINT_LAND, {4928.0F, -4049.0F, -2149.0F}},
    {POINT_WAIT | POINT_WAIT_TIME(5), {4967.0F, -4049.0F, -2164.0F}},
    {POINT_RUN | POINT_NEVER_STUCK | POINT_STROKE_FULL,
     {5105.0F, -4049.0F, -2194.9617F}},
    {POINT_RUN, {5193.276F, -4058.1741F, -2162.3614F}},
    {POINT_RUN, {5287.4253F, -4101.816F, -2012.7461F}},
    {POINT_RUN, {5432.318F, -4171.4463F, -1940.7393F}},
    {POINT_RUN, {5557.961F, -4253.7793F, -2021.2099F}},
    {POINT_RUN, {5530.9024F, -4324.059F, -2171.833F}},
    {POINT_RUN, {5383.955F, -4404.9595F, -2210.4532F}},
    {POINT_RUN, {5244.5552F, -4472.2203F, -2120.615F}},
    {POINT_RUN, {5185.486F, -4551.1753F, -1971.6496F}},
    {POINT_RUN, {5209.37F, -4587.073F, -1800.0323F}},
    {POINT_RUN, {5247.4722F, -4533.989F, -1639.9455F}},
    {POINT_RUN, {5197.905F, -4380.8541F, -1628.3752F}},
    {POINT_RUN, {5079.118F, -4406.6441F, -1730.213F}},
    {POINT_RUN, {5041.8292F, -4579.845F, -1733.1708F}},
    {POINT_RUN, {5053.2413F, -4732.9961F, -1656.3511F}},
    {POINT_RUN, {5101.5123F, -4761.2984F, -1487.8303F}},
    {POINT_RUN, {5150.759F, -4764.761F, -1316.0872F}},
    {POINT_RUN, {5199.4585F, -4786.524F, -1146.2518F}},
    {POINT_RUN, {5246.0381F, -4838.6421F, -983.8089F}},
    {POINT_RUN, {5287.8145F, -4927.118F, -838.1185F}},
    {POINT_RUN, {5324.3663F, -5043.16F, -710.64814F}},
    {POINT_RUN, {5357.3643F, -5172.2032F, -595.5697F}},
    {POINT_RUN, {5388.4253F, -5306.684F, -487.24616F}},
    {POINT_RUN, {5418.4371F, -5443.4512F, -382.58497F}},
    {POINT_RUN, {5447.8125F, -5581.245F, -280.1387F}},
    {POINT_RUN, {5476.9185F, -5718.957F, -178.633F}},
    {POINT_RUN, {5505.986F, -5855.8682F, -77.2653F}},
    {POINT_RUN, {5535.474F, -5990.7154F, 25.571698F}},
    {POINT_RUN, {5566.2344F, -6120.7051F, 132.84899F}},
    {POINT_RUN, {5600.26F, -6237.122F, 251.5076F}},
    {POINT_RUN, {5640.29F, -6323.028F, 391.1103F}},
    {POINT_RUN, {5686.0015F, -6355.698F, 550.52222F}},
    {POINT_RUN, {5729.808F, -6361.6021F, 712.678F}},
    {POINT_RUN, {5674.707F, -6370.681F, 854.83832F}},
    {POINT_RUN, {5544.243F, -6413.4141F, 946.3438F}},
    {POINT_RUN, {5481.8814F, -6477.391F, 1079.3106F}},
    {POINT_RUN, {5547.1861F, -6552.757F, 1197.8965F}},
    {POINT_RUN, {5691.026F, -6613.0972F, 1202.5405F}},
    {POINT_RUN, {5759.9073F, -6689.495F, 1081.8681F}},
    {POINT_RUN, {5706.124F, -6745.929F, 945.0886F}},
    {POINT_RUN, {5596.5953F, -6814.979F, 854.27112F}},
    {POINT_RUN, {5456.3555F, -6881.9893F, 828.9211F}},
    {POINT_RUN, {5320.134F, -6844.505F, 862.7167F}},
    {POINT_RUN, {5288.891F, -6692.2134F, 885.50312F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5280.856F, -6763.1104F, 891.7286F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5280.856F, -6918.561F, 891.7286F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5280.856F, -7072.882F, 891.7286F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5280.856F, -7226.0313F, 891.7286F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5280.856F, -7378.003F, 891.7286F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5280.856F, -7528.748F, 891.7286F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5280.856F, -7678.271F, 891.7286F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5280.856F, -7826.5381F, 891.7286F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5280.856F, -7973.5293F, 891.7286F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5280.856F, -8119.226F, 891.7286F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5280.856F, -8263.592F, 891.7286F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5280.856F, -8406.622F, 891.7289F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5280.856F, -8548.28F, 891.72913F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5279.9273F, -8688.523F, 890.4445F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5276.5831F, -8827.267F, 885.8171F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5261.3643F, -8963.7911F, 886.86512F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5277.9083F, -9089.92F, 882.32953F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5317.255F, -9192.49F, 930.2463F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5252.9385F, -9293.111F, 919.7117F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5300.545F, -9392.04F, 890.45832F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5296.008F, -9490.043F, 950.5629F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5256.6871F, -9589.155F, 899.55951F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5275.0416F, -9708.665F, 875.75062F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5275.2081F, -9833.813F, 875.66272F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5280.667F, -9956.979F, 874.0972F}},
    {POINT_PATH | POINT_WAY(WAY_DOWN), {5340.0274F, -10051.43F, 857.076F}},
    {POINT_RUN, {5457.0F, -10068.0F, 821.0F}},
    {POINT_RUN, {5538.0F, -10118.0F, 750.0F}},
    {POINT_RUN, {5586.067F, -10174.198F, 661.376F}},
    {POINT_RUN, {5601.0103F, -10222.856F, 559.28772F}},
    {POINT_RUN, {5579.443F, -10234.948F, 450.97422F}},
    {POINT_RUN | POINT_STROKE_MOST, {5563.34F, -10243.639F, 344.6523F}},
    {POINT_RUN | POINT_STROKE_MOST, {5512.3794F, -10300.255F, 272.147F}},
    {POINT_RUN | POINT_STROKE_MOST, {5443.314F, -10366.695F, 222.9709F}},
    {POINT_RUN | POINT_STROKE_MOST, {5379.934F, -10425.932F, 162.95379F}},
    {POINT_RUN | POINT_STROKE_MOST, {5363.25F, -10441.847F, 69.992592F}},
    {POINT_RUN | POINT_STROKE_MOST, {5421.62F, -10383.202F, 14.8988F}},
    {POINT_RUN | POINT_STROKE_MOST, {5506.785F, -10336.445F, -7.5124F}},
    {POINT_RUN | POINT_STROKE_MOST, {5600.839F, -10310.474F, -16.752701F}},
    {POINT_RUN | POINT_STROKE_MOST, {5695.9742F, -10300.675F, -25.9473F}},
    {POINT_RUN | POINT_STROKE_MOST, {5788.1353F, -10306.689F, -43.5364F}},
    {POINT_RUN | POINT_STROKE_MOST, {5871.495F, -10327.258F, -76.21141F}},
    {POINT_RUN | POINT_STROKE_MOST, {5935.9151F, -10355.7081F, -131.42121F}},
    {POINT_RUN | POINT_STROKE_MOST, {5970.25F, -10382.997F, -207.1007F}},
    {POINT_RUN | POINT_STROKE_MOST, {5953.8702F, -10404.102F, -286.09977F}},
    {POINT_RUN | POINT_STROKE_MOST, {5891.319F, -10419.677F, -338.5099F}},
    {POINT_RUN | POINT_STROKE_MOST, {5813.863F, -10436.434F, -356.20542F}},
    {POINT_RUN | POINT_STROKE_MOST, {5736.6451F, -10454.7081F, -353.35141F}},
    {POINT_RUN | POINT_STROKE_MOST, {5673.6392F, -10476.323F, -317.4111F}},
    {POINT_RUN | POINT_STROKE_MOST, {5638.906F, -10501.571F, -256.23432F}},
    {POINT_RUN | POINT_STROKE_MOST, {5636.264F, -10533.738F, -191.78501F}},
    {POINT_RUN | POINT_STROKE_MOST, {5663.9112F, -10575.668F, -143.1079F}},
    {POINT_RUN | POINT_STROKE_MOST, {5704.158F, -10627.144F, -124.70099F}},
    {POINT_RUN | POINT_STROKE_MOST, {5742.821F, -10680.097F, -130.44381F}},
    {POINT_RUN | POINT_STROKE_MOST, {5770.033F, -10727.97F, -161.18479F}},
    {POINT_RUN | POINT_STROKE_MOST, {5786.6651F, -10757.095F, -212.17779F}},
    {POINT_RUN | POINT_STROKE_MOST, {5799.43F, -10763.595F, -269.2003F}},
    {POINT_RUN | POINT_STROKE_MOST, {5809.936F, -10755.085F, -324.2236F}},
    {POINT_RUN | POINT_STROKE_MOST, {5818.4864F, -10737.158F, -374.58561F}},
    {POINT_RUN | POINT_STROKE_MOST, {5817.429F, -10726.66F, -424.13831F}},
    {POINT_RUN | POINT_STROKE_MOST, {5777.0F, -10749.0F, -536.0F}},
    {POINT_RUN | POINT_STROKE_MOST, {5788.8882F, -10750.0F, -602.4021F}},
    {POINT_WAIT | POINT_WAIT_TIME(5), {5807.3125F, -10749.5831F, -676.34192F}},
    {POINT_RUN | POINT_NEVER_STUCK | POINT_STROKE_FULL,
     {5823.359F, -10759.827F, -740.24103F}},
    {POINT_RUN, {5827.5762F, -10758.383F, -795.1743F}},
    {POINT_GOAL, {5831.0F, -10755.0F, -866.0F}},
    {POINT_END, {5831.0F, -10700.0F, -866.0F}},
};
static const Sint32 race_windy_count = 249;

static const struct race_point race_highway_points[256] = {
    {POINT_RUN, {-512.0F, -1.0F, 5.0F}},
    {POINT_RUN, {-414.01401F, -1.574F, 6.366F}},
    {POINT_RUN, {-283.607F, -1.154F, 7.792F}},
    {POINT_RUN, {-161.195F, 1.339F, 7.735F}},
    {POINT_RUN, {-44.581F, -0.888F, 6.15F}},
    {POINT_RUN, {86.82F, -16.028998F, 2.236F}},
    {POINT_RUN, {239.567F, -52.229F, 8.694F}},
    {POINT_RUN, {394.716F, -96.607F, 8.916F}},
    {POINT_RUN, {527.21192F, -181.64399F, 8.769F}},
    {POINT_RUN, {610.107F, -297.493F, 9.695F}},
    {POINT_RUN, {664.26703F, -397.124F, 7.01F}},
    {POINT_RUN, {708.16193F, -449.47F, 9.443F}},
    {POINT_RUN, {745.18793F, -494.52601F, 31.729F}},
    {POINT_RUN, {777.795F, -517.154F, 64.721001F}},
    {POINT_RUN, {798.806F, -532.353F, 104.774F}},
    {POINT_RUN, {799.46894F, -537.05903F, 153.657F}},
    {POINT_RUN, {798.597F, -540.351F, 207.131F}},
    {POINT_RUN, {803.845F, -539.702F, 254.597F}},
    {POINT_RUN, {811.11902F, -549.075F, 285.826F}},
    {POINT_RUN, {830.289F, -566.25892F, 307.41697F}},
    {POINT_RUN, {881.905F, -576.987F, 317.867F}},
    {POINT_RUN, {933.345F, -602.32892F, 313.624F}},
    {POINT_RUN, {1050.3471F, -658.26594F, 307.38401F}},
    {POINT_RUN, {1171.3901F, -749.66303F, 303.98597F}},
    {POINT_RUN, {1519.8171F, -1101.3471F, 307.0F}},
    {POINT_RUN, {1697.022F, -1234.457F, 304.81296F}},
    {POINT_RUN, {1823.215F, -1324.6619F, 312.42F}},
    {POINT_RUN, {1928.1979F, -1404.3649F, 352.181F}},
    {POINT_RUN, {1991.1429F, -1447.491F, 412.36997F}},
    {POINT_RUN, {2037.2131F, -1452.504F, 478.862F}},
    {POINT_RUN, {2088.991F, -1404.4169F, 567.51093F}},
    {POINT_RUN, {2113.3611F, -1318.2449F, 629.93903F}},
    {POINT_RUN, {2132.1851F, -1298.5209F, 657.077F}},
    {POINT_RUN, {2176.8338F, -1291.277F, 729.70893F}},
    {POINT_RUN, {2261.8458F, -1281.4161F, 763.973F}},
    {POINT_RUN, {2366.4498F, -1266.928F, 774.90693F}},
    {POINT_RUN, {2482.668F, -1240.6821F, 770.0F}},
    {POINT_RUN, {2613.1148F, -1194.447F, 754.118F}},
    {POINT_RUN, {2731.324F, -1149.0489F, 715.69F}},
    {POINT_RUN, {2833.4688F, -1124.5781F, 683.362F}},
    {POINT_RUN, {2957.515F, -1110.496F, 620.699F}},
    {POINT_RUN, {3062.53F, -1134.952F, 502.67701F}},
    {POINT_RUN, {3144.6111F, -1138.0081F, 447.606F}},
    {POINT_RUN, {3244.443F, -1141.0621F, 399.00901F}},
    {POINT_RUN, {3368.001F, -1170.7659F, 342.023F}},
    {POINT_RUN, {3595.378F, -1260.15F, 289.40702F}},
    {POINT_RUN, {3733.626F, -1332.3361F, 272.57F}},
    {POINT_RUN, {3821.6167F, -1351.4849F, 261.35F}},
    {POINT_RUN, {3899.2188F, -1346.0939F, 262.232F}},
    {POINT_RUN, {4006.2017F, -1308.1851F, 276.03501F}},
    {POINT_RUN, {4058.0391F, -1285.801F, 278.56202F}},
    {POINT_RUN, {4110.577F, -1277.9189F, 281.092F}},
    {POINT_RUN, {4177.012F, -1294.996F, 286.273F}},
    {POINT_RUN, {4230.6895F, -1322.6479F, 292.43601F}},
    {POINT_RUN, {4367.7891F, -1390.1541F, 304.252F}},
    {POINT_RUN, {4505.639F, -1459.561F, 361.403F}},
    {POINT_RUN, {4651.738F, -1513.085F, 477.52601F}},
    {POINT_RUN, {4793.8365F, -1572.3129F, 659.331F}},
    {POINT_RUN, {4873.925F, -1607.436F, 798.91602F}},
    {POINT_RUN, {4985.258F, -1638.8071F, 914.075F}},
    {POINT_RUN, {5102.511F, -1675.374F, 1005.46894F}},
    {POINT_RUN, {5185.83F, -1702.124F, 1058.889F}},
    {POINT_RUN, {5227.5293F, -1693.3021F, 1095.6921F}},
    {POINT_RUN, {5261.588F, -1672.7399F, 1126.8541F}},
    {POINT_RUN, {5281.285F, -1645.9301F, 1147.7939F}},
    {POINT_RUN, {5289.6802F, -1627.9611F, 1223.335F}},
    {POINT_RUN, {5288.99F, -1611.3559F, 1273.5109F}},
    {POINT_RUN, {5279.1592F, -1607.066F, 1310.926F}},
    {POINT_RUN, {5254.7461F, -1623.9169F, 1344.694F}},
    {POINT_RUN, {5196.7461F, -1628.0571F, 1381.973F}},
    {POINT_RUN, {5140.3521F, -1612.78F, 1417.6451F}},
    {POINT_RUN, {5080.0F, -1585.691F, 1458.0F}},
    {POINT_RUN, {5011.067F, -1564.905F, 1528.5411F}},
    {POINT_RUN, {4948.007F, -1539.2951F, 1582.2391F}},
    {POINT_RUN, {4894.7935F, -1491.4841F, 1617.1529F}},
    {POINT_RUN, {4832.255F, -1457.025F, 1640.9271F}},
    {POINT_RUN, {4766.0455F, -1436.707F, 1655.074F}},
    {POINT_RUN, {4718.0F, -1416.22F, 1660.3851F}},
    {POINT_RUN, {4690.927F, -1399.2919F, 1662.6309F}},
    {POINT_RUN, {4641.0F, -1314.702F, 1662.0389F}},
    {POINT_RUN, {4597.87F, -1247.7039F, 1668.699F}},
    {POINT_RUN, {4570.287F, -1229.3129F, 1666.8809F}},
    {POINT_RUN, {4522.9532F, -1212.5579F, 1665.061F}},
    {POINT_RUN, {4480.7793F, -1208.5411F, 1664.5781F}},
    {POINT_RUN, {4422.5381F, -1218.5839F, 1664.0959F}},
    {POINT_RUN, {4374.34F, -1274.824F, 1663.1309F}},
    {POINT_RUN, {4343.0762F, -1362.3591F, 1662.3831F}},
    {POINT_RUN, {4263.066F, -1426.9931F, 1664.6801F}},
    {POINT_RUN, {4194.639F, -1436.874F, 1666.34F}},
    {POINT_RUN, {4090.875F, -1440.8149F, 1668.0F}},
    {POINT_RUN, {4025.203F, -1422.4239F, 1655.598F}},
    {POINT_RUN, {3977.3831F, -1375.689F, 1630.4139F}},
    {POINT_RUN, {3958.1041F, -1337.1149F, 1616.561F}},
    {POINT_RUN, {3938.8248F, -1317.8281F, 1602.707F}},
    {POINT_RUN, {3900.189F, -1306.8039F, 1571.8039F}},
    {POINT_RUN, {3755.732F, -1300.936F, 1504.5159F}},
    {POINT_RUN, {3603.1048F, -1282.8591F, 1456.2441F}},
    {POINT_RUN, {3414.33F, -1268.7989F, 1458.3521F}},
    {POINT_RUN, {3210.0F, -1258.0F, 1495.0F}},
    {POINT_RUN, {3037.0171F, -1239.8521F, 1524.2869F}},
    {POINT_RUN, {2808.734F, -1224.3521F, 1604.8879F}},
    {POINT_RUN, {2718.4097F, -1219.309F, 1680.777F}},
    {POINT_RUN, {2652.4617F, -1227.556F, 1770.658F}},
    {POINT_RUN, {2622.9061F, -1242.3941F, 1932.6879F}},
    {POINT_RUN, {2636.0F, -1248.0F, 2206.0F}},
    {POINT_RUN, {2640.4021F, -1254.375F, 2363.349F}},
    {POINT_RUN, {2650.918F, -1277.9821F, 2483.732F}},
    {POINT_RUN, {2682.588F, -1312.467F, 2586.783F}},
    {POINT_RUN, {2758.495F, -1363.387F, 2705.3648F}},
    {POINT_RUN, {2907.2671F, -1410.21F, 2842.5628F}},
    {POINT_RUN, {3074.4068F, -1441.9689F, 2977.5647F}},
    {POINT_RUN, {3192.4131F, -1454.3409F, 3036.318F}},
    {POINT_RUN, {3423.787F, -1459.7861F, 3093.705F}},
    {POINT_RUN, {3533.9588F, -1470.384F, 3125.247F}},
    {POINT_RUN, {3611.38F, -1482.25F, 3170.6141F}},
    {POINT_RUN, {3733.9371F, -1495.4689F, 3310.662F}},
    {POINT_RUN, {3790.2601F, -1479.092F, 3400.443F}},
    {POINT_RUN, {3846.0811F, -1468.371F, 3503.47F}},
    {POINT_RUN, {3876.8021F, -1479.2391F, 3564.2891F}},
    {POINT_RUN, {3891.6458F, -1499.4771F, 3603.0867F}},
    {POINT_RUN, {3900.5501F, -1525.125F, 3629.811F}},
    {POINT_RUN, {3908.6451F, -1574.2299F, 3646.0081F}},
    {POINT_RUN, {3935.451F, -1836.3299F, 3700.7017F}},
    {POINT_RUN, {3961.463F, -1868.686F, 3734.7518F}},
    {POINT_RUN, {3991.049F, -1883.957F, 3759.0247F}},
    {POINT_RUN, {4044.2811F, -1887.8299F, 3793.609F}},
    {POINT_RUN, {4096.5435F, -1904.2889F, 3816.07F}},
    {POINT_RUN, {4148.808F, -1921.717F, 3841.0201F}},
    {POINT_RUN, {4367.0F, -1938.0F, 3961.0F}},
    {POINT_RUN, {4455.684F, -1932.8731F, 4012.3577F}},
    {POINT_RUN, {4548.118F, -1931.9271F, 4054.2188F}},
    {POINT_RUN, {4639.941F, -1912.7761F, 4090.4651F}},
    {POINT_RUN, {4717.3541F, -1889.6409F, 4093.9617F}},
    {POINT_RUN, {4786.42F, -1867.639F, 4082.7391F}},
    {POINT_RUN, {4844.751F, -1841.827F, 4036.432F}},
    {POINT_RUN, {4895.537F, -1833.1999F, 3935.401F}},
    {POINT_RUN, {4913.6441F, -1841.7559F, 3878.0811F}},
    {POINT_RUN, {4933.942F, -1866.3941F, 3826.908F}},
    {POINT_RUN, {4936.853F, -1879.2709F, 3778.1331F}},
    {POINT_RUN, {4933.17F, -1880.8521F, 3745.5647F}},
    {POINT_RUN, {4911.003F, -1881.9421F, 3681.0521F}},
    {POINT_RUN, {4853.927F, -1881.767F, 3634.4251F}},
    {POINT_RUN, {4792.0904F, -1882.861F, 3607.9861F}},
    {POINT_RUN, {4716.486F, -1882.8621F, 3623.6268F}},
    {POINT_RUN, {4633.747F, -1881.9161F, 3687.3858F}},
    {POINT_RUN, {4555.611F, -1880.637F, 3791.7291F}},
    {POINT_RUN, {4504.511F, -1879.639F, 3873.526F}},
    {POINT_RUN, {4395.809F, -1877.6719F, 4044.5318F}},
    {POINT_RUN, {4362.387F, -1873.2341F, 4122.5674F}},
    {POINT_RUN, {4348.5F, -1866.0081F, 4194.3482F}},
    {POINT_RUN, {4349.9732F, -1863.1191F, 4239.946F}},
    {POINT_RUN, {4358.9161F, -1854.449F, 4313.5923F}},
    {POINT_RUN, {4379.0743F, -1847.223F, 4371.3995F}},
    {POINT_RUN, {4431.7442F, -1835.6619F, 4452.295F}},
    {POINT_RUN, {4469.141F, -1834.217F, 4510.0782F}},
    {POINT_RUN, {4506.535F, -1831.3261F, 4564.971F}},
    {POINT_RUN, {4566.2032F, -1831.3261F, 4625.643F}},
    {POINT_RUN, {4625.8731F, -1834.217F, 4693.537F}},
    {POINT_RUN, {4761.963F, -1842.3909F, 4792.99F}},
    {POINT_RUN, {4891.817F, -1854.7321F, 4864.7271F}},
    {POINT_RUN, {4992.054F, -1862.5059F, 4894.936F}},
    {POINT_RUN, {5108.0F, -1875.9581F, 4895.961F}},
    {POINT_RUN, {5274.8921F, -1893.5679F, 4876.1451F}},
    {POINT_RUN, {5465.948F, -1865.2271F, 4853.5F}},
    {POINT_RUN, {5610.989F, -1839.652F, 4896.5206F}},
    {POINT_RUN, {5757.7032F, -1816.2659F, 4987.033F}},
    {POINT_RUN, {5957.423F, -1805.0571F, 5131.061F}},
    {POINT_RUN, {6129.175F, -1800.8521F, 5260.582F}},
    {POINT_RUN, {6174.641F, -1785.9189F, 5327.26F}},
    {POINT_RUN, {6193.448F, -1757.4891F, 5408.571F}},
    {POINT_RUN, {6202.9771F, -1739.6719F, 5480.83F}},
    {POINT_RUN, {6196.3711F, -1720.4531F, 5625.6963F}},
    {POINT_RUN, {6169.7154F, -1715.215F, 5810.2862F}},
    {POINT_RUN, {6120.859F, -1714.7111F, 5978.2222F}},
    {POINT_RUN, {6064.236F, -1724.2221F, 6103.6304F}},
    {POINT_RUN, {5939.4395F, -1732.1041F, 6317.1114F}},
    {POINT_RUN, {5831.3834F, -1749.059F, 6439.861F}},
    {POINT_RUN, {5726.099F, -1739.319F, 6565.4014F}},
    {POINT_RUN, {5591.531F, -1726.65F, 6643.1304F}},
    {POINT_RUN, {5470.6045F, -1703.6719F, 6666.484F}},
    {POINT_RUN, {5376.0F, -1668.0F, 6636.0F}},
    {POINT_RUN, {5306.533F, -1659.2709F, 6570.014F}},
    {POINT_RUN, {5281.8262F, -1638.2811F, 6477.6304F}},
    {POINT_RUN, {5303.0F, -1624.0F, 6370.0F}},
    {POINT_RUN, {5379.0F, -1629.0F, 6271.0F}},
    {POINT_RUN, {5460.0F, -1659.0F, 6230.0F}},
    {POINT_RUN, {5542.0F, -1689.0F, 6232.0F}},
    {POINT_RUN, {5625.274F, -1721.96F, 6273.0264F}},
    {POINT_RUN, {5689.1783F, -1774.6269F, 6358.827F}},
    {POINT_RUN, {5709.984F, -1814.389F, 6469.516F}},
    {POINT_RUN, {5680.533F, -1856.7371F, 6626.531F}},
    {POINT_RUN, {5653.9253F, -1894.6739F, 6702.393F}},
    {POINT_RUN, {5641.8121F, -1949.8641F, 6768.0572F}},
    {POINT_RUN, {5629.031F, -2034.6459F, 6838.9942F}},
    {POINT_RUN, {5609.6456F, -2117.0838F, 6896.1705F}},
    {POINT_RUN, {5589.8482F, -2189.753F, 6948.336F}},
    {POINT_RUN, {5577.2701F, -2223.559F, 6991.1334F}},
    {POINT_RUN, {5567.418F, -2243.3687F, 7032.224F}},
    {POINT_RUN, {5548.8423F, -2254.0938F, 7086.486F}},
    {POINT_RUN, {5491.526F, -2257.441F, 7266.0206F}},
    {POINT_RUN, {5456.359F, -2276.7728F, 7406.7325F}},
    {POINT_RUN, {5435.3335F, -2293.7608F, 7486.5645F}},
    {POINT_RUN, {5422.2291F, -2311.467F, 7567.069F}},
    {POINT_RUN, {5440.9283F, -2324.182F, 7647.6666F}},
    {POINT_RUN, {5471.0835F, -2319.0538F, 7700.1802F}},
    {POINT_RUN, {5503.736F, -2304.0137F, 7739.4073F}},
    {POINT_RUN, {5535.4532F, -2280.9861F, 7767.018F}},
    {POINT_RUN, {5580.762F, -2238.015F, 7791.5435F}},
    {POINT_RUN, {5625.766F, -2156.266F, 7818.783F}},
    {POINT_RUN, {5661.3882F, -2046.491F, 7846.0035F}},
    {POINT_RUN, {5699.132F, -1973.5621F, 7870.0982F}},
    {POINT_RUN, {5759.488F, -1930.7869F, 7901.9581F}},
    {POINT_RUN, {5826.772F, -1900.168F, 7932.684F}},
    {POINT_RUN, {5948.865F, -1877.46F, 7989.9161F}},
    {POINT_RUN, {6136.172F, -1873.1071F, 8065.2481F}},
    {POINT_RUN, {6362.0F, -1875.0F, 8157.0F}},
    {POINT_RUN, {6551.461F, -1885.501F, 8239.749F}},
    {POINT_RUN, {6816.6304F, -1918.551F, 8327.523F}},
    {POINT_RUN, {6936.346F, -1955.733F, 8412.215F}},
    {POINT_RUN, {7039.207F, -1985.684F, 8497.925F}},
    {POINT_RUN, {7122.596F, -2017.2659F, 8622.573F}},
    {POINT_RUN, {7179.3472F, -2052.228F, 8774.818F}},
    {POINT_RUN, {7236.9722F, -2103.1668F, 8995.118F}},
    {POINT_RUN, {7266.251F, -2132.286F, 9167.61F}},
    {POINT_RUN, {7272.3243F, -2140.9097F, 9255.5831F}},
    {POINT_RUN, {7294.7554F, -2155.3121F, 9349.008F}},
    {POINT_RUN, {7327.103F, -2180.051F, 9427.253F}},
    {POINT_RUN, {7381.3125F, -2204.7918F, 9492.555F}},
    {POINT_RUN, {7458.207F, -2234.3121F, 9560.571F}},
    {POINT_RUN, {7559.537F, -2263.836F, 9605.164F}},
    {POINT_RUN, {7656.814F, -2278.1641F, 9621.378F}},
    {POINT_RUN, {7750.0391F, -2292.4961F, 9637.595F}},
    {POINT_RUN, {7916.0F, -2303.0F, 9649.0F}},
    {POINT_RUN, {8013.4981F, -2296.691F, 9649.757F}},
    {POINT_RUN, {8224.266F, -2299.6641F, 9641.649F}},
    {POINT_RUN, {8422.874F, -2306.828F, 9637.595F}},
    {POINT_RUN, {8597.432F, -2302.613F, 9641.646F}},
    {POINT_RUN, {8779.827F, -2278.625F, 9686.24F}},
    {POINT_RUN, {8925.743F, -2266.0621F, 9742.995F}},
    {POINT_RUN, {9079.766F, -2263.016F, 9848.396F}},
    {POINT_RUN, {9193.255F, -2259.9688F, 9945.689F}},
    {POINT_RUN, {9274.321F, -2247.586F, 10030.823F}},
    {POINT_RUN, {9371.597F, -2198.422F, 10099.738F}},
    {POINT_RUN, {9460.374F, -2108.328F, 10127.9581F}},
    {POINT_RUN, {9526.25F, -2064.6451F, 10139.936F}},
    {POINT_RUN, {9595.122F, -2041.746F, 10139.936F}},
    {POINT_RUN, {9660.999F, -2012.504F, 10132.45F}},
    {POINT_RUN, {9734.361F, -1996.5311F, 10118.972F}},
    {POINT_RUN, {9819.702F, -1980.5621F, 10106.993F}},
    {POINT_RUN, {9902.048F, -1978.8909F, 10106.993F}},
    {POINT_RUN, {9963.434F, -1978.0551F, 10105.497F}},
    {POINT_RUN, {10011.344F, -1977.223F, 10106.993F}},
    {POINT_RUN, {10059.255F, -1977.223F, 10112.983F}},
    {POINT_RUN, {10155.077F, -1977.223F, 10127.9581F}},
    {POINT_RUN, {10246.0F, -1969.0F, 10156.0F}},
    {POINT_RUN, {10335.0F, -1974.0F, 10177.0F}},
};
static const Sint32 race_highway_count = 256;

static const struct race_point race_icecap_points[255] = {
    {POINT_BOARD, {101.7993F, 334.58707F, -65.85601F}},
    {POINT_BOARD, {-17.315398F, 234.8987F, -105.7023F}},
    {POINT_BOARD, {-136.43009F, 135.2104F, -145.54848F}},
    {POINT_BOARD, {-255.5448F, 35.521996F, -185.39481F}},
    {POINT_BOARD, {-374.6595F, -64.1663F, -225.2411F}},
    {POINT_BOARD, {-493.77411F, -163.8546F, -265.08732F}},
    {POINT_BOARD, {-612.8888F, -263.54297F, -304.9336F}},
    {POINT_BOARD, {-655.6997F, -413.5921F, -328.66431F}},
    {POINT_BOARD, {-691.39942F, -568.33442F, -350.893F}},
    {POINT_BOARD, {-800.71222F, -662.8121F, -393.8284F}},
    {POINT_BOARD, {-933.2833F, -738.24982F, -443.306F}},
    {POINT_BOARD, {-1065.8531F, -813.6869F, -492.7833F}},
    {POINT_BOARD, {-1198.424F, -889.12452F, -542.2609F}},
    {POINT_BOARD, {-1330.995F, -964.5621F, -591.7385F}},
    {POINT_BOARD, {-1463.5649F, -1039.9993F, -641.2158F}},
    {POINT_BOARD, {-1596.1359F, -1115.437F, -690.6934F}},
    {POINT_BOARD, {-1728.7059F, -1190.8738F, -740.1707F}},
    {POINT_BOARD, {-1861.2768F, -1266.3115F, -789.6483F}},
    {POINT_BOARD, {-1993.8476F, -1341.7493F, -839.126F}},
    {POINT_BOARD, {-2126.4185F, -1417.1868F, -888.6036F}},
    {POINT_BOARD, {-2258.9883F, -1492.6238F, -938.08082F}},
    {POINT_BOARD, {-2391.5594F, -1568.0615F, -987.5585F}},
    {POINT_BOARD, {-2524.1294F, -1643.4985F, -1037.0357F}},
    {POINT_BOARD, {-2656.7002F, -1718.9362F, -1086.5135F}},
    {POINT_BOARD, {-2789.2713F, -1794.3737F, -1135.991F}},
    {POINT_BOARD, {-2921.8414F, -1869.811F, -1185.4683F}},
    {POINT_BOARD, {-3054.4131F, -1945.2493F, -1234.9462F}},
    {POINT_BOARD, {-3186.983F, -2020.6862F, -1284.4236F}},
    {POINT_BOARD, {-3319.553F, -2096.1231F, -1333.9008F}},
    {POINT_BOARD, {-3452.124F, -2171.561F, -1383.3785F}},
    {POINT_BOARD, {-3584.695F, -2246.9983F, -1432.8559F}},
    {POINT_BOARD, {-3717.2657F, -2322.4361F, -1482.3337F}},
    {POINT_BOARD, {-3849.8367F, -2397.8736F, -1531.8112F}},
    {POINT_BOARD, {-3982.4065F, -2473.311F, -1581.2885F}},
    {POINT_BOARD, {-4114.9771F, -2548.7483F, -1630.766F}},
    {POINT_BOARD, {-4247.5484F, -2624.1861F, -1680.2438F}},
    {POINT_BOARD, {-4380.12F, -2699.624F, -1729.7215F}},
    {POINT_BOARD, {-4512.689F, -2775.061F, -1779.1987F}},
    {POINT_BOARD, {-4645.2603F, -2850.4983F, -1828.6763F}},
    {POINT_BOARD, {-4777.83F, -2925.9356F, -1878.1535F}},
    {POINT_BOARD, {-4910.401F, -3001.3731F, -1927.6314F}},
    {POINT_BOARD, {-5042.9722F, -3076.811F, -1977.109F}},
    {POINT_BOARD, {-5175.5416F, -3152.2481F, -2026.5862F}},
    {POINT_BOARD, {-5307.719F, -3233.648F, -2065.0923F}},
    {POINT_BOARD, {-5439.552F, -3320.1978F, -2094.1204F}},
    {POINT_BOARD, {-5547.363F, -3428.5816F, -2138.8887F}},
    {POINT_BOARD, {-5648.91F, -3542.6592F, -2187.7605F}},
    {POINT_BOARD, {-5775.33F, -3623.64F, -2237.9021F}},
    {POINT_BOARD, {-5911.3145F, -3691.8941F, -2288.5318F}},
    {POINT_BOARD, {-6047.2984F, -3760.1475F, -2339.1614F}},
    {POINT_BOARD, {-6183.283F, -3828.4017F, -2389.7916F}},
    {POINT_BOARD, {-6319.2671F, -3896.6561F, -2440.4212F}},
    {POINT_BOARD, {-6456.6573F, -3957.4004F, -2494.3582F}},
    {POINT_BOARD, {-6598.889F, -3992.2627F, -2559.6927F}},
    {POINT_BOARD, {-6736.849F, -4040.1277F, -2622.207F}},
    {POINT_BOARD, {-6864.279F, -4120.058F, -2677.7662F}},
    {POINT_BOARD, {-6991.2481F, -4200.5503F, -2733.1231F}},
    {POINT_BOARD, {-7078.399F, -4329.7242F, -2770.97F}},
    {POINT_BOARD, {-7165.5503F, -4458.9004F, -2808.817F}},
    {POINT_BOARD, {-7252.7012F, -4588.0743F, -2846.6634F}},
    {POINT_BOARD, {-7339.852F, -4717.2481F, -2884.5103F}},
    {POINT_BOARD, {-7427.0015F, -4846.423F, -2922.357F}},
    {POINT_BOARD, {-7514.154F, -4975.599F, -2960.2037F}},
    {POINT_BOARD, {-7601.3052F, -5104.774F, -2998.051F}},
    {POINT_BOARD, {-7688.4551F, -5233.9483F, -3035.8975F}},
    {POINT_BOARD, {-7775.606F, -5363.1231F, -3073.7442F}},
    {POINT_BOARD, {-7862.757F, -5492.297F, -3111.5906F}},
    {POINT_BOARD, {-7949.908F, -5621.4722F, -3149.4378F}},
    {POINT_BOARD, {-8037.059F, -5750.647F, -3187.2845F}},
    {POINT_BOARD, {-8138.8414F, -5860.9444F, -3196.5674F}},
    {POINT_BOARD, {-8273.368F, -5928.9961F, -3141.9271F}},
    {POINT_BOARD, {-8407.894F, -5997.048F, -3087.2864F}},
    {POINT_BOARD, {-8485.753F, -6086.511F, -3000.71F}},
    {POINT_BOARD, {-8513.436F, -6194.934F, -2885.8545F}},
    {POINT_BOARD, {-8541.12F, -6303.357F, -2770.9993F}},
    {POINT_BOARD, {-8568.802F, -6411.7813F, -2656.1426F}},
    {POINT_BOARD, {-8596.485F, -6520.2042F, -2541.2876F}},
    {POINT_BOARD, {-8624.1661F, -6628.6265F, -2426.4322F}},
    {POINT_BOARD, {-8651.849F, -6737.0503F, -2311.5757F}},
    {POINT_BOARD, {-8679.533F, -6845.474F, -2196.7205F}},
    {POINT_BOARD, {-8707.215F, -6953.8975F, -2081.8641F}},
    {POINT_BOARD, {-8734.898F, -7062.321F, -1967.0087F}},
    {POINT_BOARD, {-8762.582F, -7170.745F, -1852.1523F}},
    {POINT_BOARD, {-8797.618F, -7286.0762F, -1747.2975F}},
    {POINT_BOARD, {-8839.573F, -7407.9063F, -1651.845F}},
    {POINT_BOARD, {-8881.526F, -7529.7354F, -1556.3929F}},
    {POINT_BOARD, {-8923.481F, -7651.5674F, -1460.9388F}},
    {POINT_BOARD, {-8965.434F, -7773.3941F, -1365.4882F}},
    {POINT_BOARD, {-9007.389F, -7895.2251F, -1270.0348F}},
    {POINT_BOARD, {-9049.342F, -8017.054F, -1174.5831F}},
    {POINT_BOARD, {-9091.297F, -8138.8834F, -1079.1302F}},
    {POINT_BOARD, {-9117.521F, -8257.463F, -977.76264F}},
    {POINT_BOARD, {-9106.698F, -8368.387F, -862.4705F}},
    {POINT_BOARD, {-9095.876F, -8479.311F, -747.1766F}},
    {POINT_BOARD, {-9085.055F, -8590.236F, -631.88263F}},
    {POINT_BOARD, {-9074.233F, -8701.162F, -516.58863F}},
    {POINT_BOARD, {-9063.412F, -8812.085F, -401.2957F}},
    {POINT_BOARD, {-9052.588F, -8923.007F, -286.0044F}},
    {POINT_BOARD, {-9041.767F, -9033.93F, -170.71131F}},
    {POINT_BOARD, {-8946.34F, -9111.22F, -141.9796F}},
    {POINT_BOARD, {-8798.3331F, -9167.602F, -167.0467F}},
    {POINT_BOARD, {-8650.323F, -9223.986F, -192.11431F}},
    {POINT_BOARD, {-8502.316F, -9280.368F, -217.18119F}},
    {POINT_BOARD, {-8358.782F, -9340.247F, -242.95411F}},
    {POINT_BOARD, {-8333.017F, -9492.18F, -287.30106F}},
    {POINT_BOARD, {-8274.309F, -9608.672F, -345.53077F}},
    {POINT_BOARD, {-8146.62F, -9650.955F, -432.83106F}},
    {POINT_BOARD, {-8018.93F, -9693.238F, -520.13093F}},
    {POINT_BOARD, {-7891.2413F, -9735.519F, -607.4302F}},
    {POINT_BOARD, {-7763.552F, -9777.802F, -694.73011F}},
    {POINT_BOARD, {-7671.7134F, -9835.626F, -810.8999F}},
    {POINT_BOARD, {-7586.577F, -9896.354F, -932.4618F}},
    {POINT_BOARD, {-7501.439F, -9957.082F, -1054.0257F}},
    {POINT_BOARD, {-7416.3023F, -10017.811F, -1175.5886F}},
    {POINT_BOARD, {-7332.51F, -10081.505F, -1296.5458F}},
    {POINT_BOARD, {-7249.3521F, -10146.595F, -1417.2151F}},
    {POINT_BOARD, {-7166.1924F, -10211.688F, -1537.8875F}},
    {POINT_BOARD, {-7083.033F, -10276.78F, -1658.5589F}},
    {POINT_BOARD, {-6998.361F, -10343.146F, -1776.9198F}},
    {POINT_BOARD, {-6892.4615F, -10427.401F, -1862.9449F}},
    {POINT_BOARD, {-6786.5621F, -10511.657F, -1948.9706F}},
    {POINT_BOARD, {-6680.6631F, -10595.91F, -2034.9959F}},
    {POINT_BOARD, {-6574.766F, -10680.165F, -2121.0203F}},
    {POINT_BOARD, {-6464.1304F, -10761.554F, -2203.6507F}},
    {POINT_BOARD, {-6351.0F, -10841.429F, -2284.4883F}},
    {POINT_BOARD, {-6237.8682F, -10921.305F, -2365.3267F}},
    {POINT_BOARD, {-6117.899F, -11011.2F, -2403.7564F}},
    {POINT_BOARD, {-5991.7295F, -11110.175F, -2403.7564F}},
    {POINT_BOARD, {-5865.5645F, -11209.148F, -2403.7564F}},
    {POINT_BOARD, {-5739.271F, -11304.79F, -2386.6436F}},
    {POINT_BOARD, {-5612.846F, -11396.88F, -2351.294F}},
    {POINT_BOARD, {-5486.4195F, -11488.97F, -2315.9444F}},
    {POINT_BOARD, {-5359.995F, -11581.059F, -2280.595F}},
    {POINT_BOARD, {-5239.934F, -11684.793F, -2258.1631F}},
    {POINT_BOARD, {-5120.5416F, -11789.745F, -2237.0853F}},
    {POINT_BOARD, {-5001.149F, -11894.696F, -2216.0074F}},
    {POINT_BOARD, {-4876.936F, -11994.66F, -2204.2864F}},
    {POINT_BOARD, {-4747.2251F, -12088.934F, -2203.2376F}},
    {POINT_BOARD, {-4617.5142F, -12183.2081F, -2202.1885F}},
    {POINT_BOARD, {-4494.677F, -12280.715F, -2188.6583F}},
    {POINT_BOARD, {-4401.9273F, -12392.361F, -2120.4908F}},
    {POINT_BOARD, {-4309.18F, -12504.003F, -2052.3235F}},
    {POINT_BOARD, {-4216.429F, -12615.651F, -1984.1547F}},
    {POINT_BOARD, {-4133.1524F, -12737.597F, -1921.9804F}},
    {POINT_BOARD, {-4051.6104F, -12861.429F, -1860.9025F}},
    {POINT_BOARD, {-3970.0694F, -12985.258F, -1799.8257F}},
    {POINT_BOARD, {-3880.1407F, -13107.512F, -1748.1387F}},
    {POINT_BOARD, {-3789.7276F, -13229.673F, -1696.9938F}},
    {POINT_BOARD, {-3699.3162F, -13351.835F, -1645.8501F}},
    {POINT_BOARD, {-3599.4466F, -13471.264F, -1608.0878F}},
    {POINT_BOARD, {-3497.84F, -13590.191F, -1572.784F}},
    {POINT_BOARD, {-3415.9698F, -13723.599F, -1546.2664F}},
    {POINT_BOARD, {-3348.9993F, -13867.936F, -1526.3819F}},
    {POINT_BOARD, {-3282.0284F, -14012.274F, -1506.497F}},
    {POINT_BOARD, {-3215.059F, -14156.611F, -1486.6128F}},
    {POINT_BOARD, {-3126.283F, -14283.276F, -1455.3484F}},
    {POINT_BOARD, {-3018.2286F, -14394.323F, -1414.0236F}},
    {POINT_BOARD, {-2910.1741F, -14505.368F, -1372.6987F}},
    {POINT_BOARD, {-2775.3257F, -14583.026F, -1343.4954F}},
    {POINT_BOARD, {-2631.7452F, -14649.816F, -1318.2391F}},
    {POINT_BOARD, {-2488.1636F, -14716.606F, -1292.9823F}},
    {POINT_BOARD, {-2344.5855F, -14783.392F, -1267.7261F}},
    {POINT_BOARD, {-2208.0472F, -14858.874F, -1231.6135F}},
    {POINT_BOARD, {-2073.531F, -14936.849F, -1192.3865F}},
    {POINT_BOARD, {-1939.012F, -15014.825F, -1153.159F}},
    {POINT_BOARD, {-1824.2078F, -15096.448F, -1083.6409F}},
    {POINT_BOARD, {-1724.9097F, -15180.943F, -990.28803F}},
    {POINT_BOARD, {-1625.6131F, -15265.436F, -896.9362F}},
    {POINT_BOARD, {-1537.3375F, -15358.566F, -804.03174F}},
    {POINT_BOARD, {-1483.7408F, -15478.858F, -712.5411F}},
    {POINT_BOARD, {-1430.143F, -15599.151F, -621.04914F}},
    {POINT_BOARD, {-1376.5462F, -15719.443F, -529.5586F}},
    {POINT_BOARD, {-1357.6221F, -15816.662F, -413.8749F}},
    {POINT_BOARD, {-1376.4114F, -15888.774F, -271.88932F}},
    {POINT_BOARD, {-1395.2F, -15960.885F, -129.9069F}},
    {POINT_BOARD, {-1413.99F, -16033.001F, 12.0851F}},
    {POINT_BOARD, {-1432.7792F, -16105.115F, 154.0707F}},
    {POINT_BOARD, {-1451.5683F, -16177.227F, 296.05631F}},
    {POINT_BOARD, {-1445.1491F, -16272.014F, 420.75806F}},
    {POINT_BOARD, {-1418.9321F, -16384.61F, 531.8885F}},
    {POINT_BOARD, {-1392.7157F, -16497.202F, 643.01532F}},
    {POINT_BOARD, {-1366.4997F, -16609.793F, 754.14203F}},
    {POINT_BOARD, {-1324.4622F, -16716.29F, 853.75483F}},
    {POINT_BOARD, {-1181.3749F, -16783.83F, 879.8024F}},
    {POINT_BOARD, {-1038.2917F, -16851.364F, 905.8491F}},
    {POINT_BOARD, {-901.23664F, -16904.06F, 870.3296F}},
    {POINT_BOARD, {-768.4836F, -16946.151F, 790.8302F}},
    {POINT_BOARD, {-635.7349F, -16988.243F, 711.33332F}},
    {POINT_BOARD, {-547.50043F, -17103.797F, 658.33112F}},
    {POINT_BOARD, {-468.99396F, -17235.407F, 611.1187F}},
    {POINT_BOARD, {-390.4846F, -17367.024F, 563.9047F}},
    {POINT_BOARD, {-311.97947F, -17498.63F, 516.6931F}},
    {POINT_BOARD, {-233.2332F, -17630.032F, 469.29697F}},
    {POINT_BOARD, {-153.3501F, -17760.422F, 421.0287F}},
    {POINT_BOARD, {-73.4652F, -17890.815F, 372.75937F}},
    {POINT_BOARD, {6.4196F, -18021.211F, 324.4901F}},
    {POINT_BOARD, {86.30269F, -18151.602F, 276.22181F}},
    {POINT_BOARD, {166.1894F, -18281.999F, 227.95141F}},
    {POINT_BOARD, {253.249F, -18403.438F, 172.84821F}},
    {POINT_BOARD, {354.3815F, -18507.33F, 104.3422F}},
    {POINT_BOARD, {455.5128F, -18611.217F, 35.8372F}},
    {POINT_BOARD, {556.64673F, -18715.11F, -32.669701F}},
    {POINT_BOARD, {550.5763F, -18803.86F, -150.5661F}},
    {POINT_BOARD, {514.97492F, -18888.442F, -282.0645F}},
    {POINT_BOARD, {479.3734F, -18973.024F, -413.5629F}},
    {POINT_BOARD, {443.772F, -19057.604F, -545.0612F}},
    {POINT_BOARD, {386.4486F, -19149.122F, -654.57062F}},
    {POINT_BOARD, {284.52216F, -19254.875F, -718.9191F}},
    {POINT_BOARD, {182.59711F, -19360.63F, -783.26673F}},
    {POINT_BOARD, {80.6707F, -19466.387F, -847.61524F}},
    {POINT_BOARD, {6.0744F, -19583.483F, -925.4967F}},
    {POINT_BOARD, {-58.8882F, -19704.565F, -1008.1444F}},
    {POINT_BOARD, {-130.76429F, -19815.7F, -1097.8976F}},
    {POINT_BOARD, {-207.6464F, -19919.61F, -1192.7951F}},
    {POINT_BOARD, {-284.5285F, -20023.52F, -1287.6927F}},
    {POINT_BOARD, {-361.4082F, -20127.43F, -1382.587F}},
    {POINT_BOARD, {-430.45942F, -20242.512F, -1467.5995F}},
    {POINT_BOARD, {-486.4296F, -20376.266F, -1536.1001F}},
    {POINT_BOARD, {-542.3971F, -20510.016F, -1604.5972F}},
    {POINT_BOARD, {-598.3655F, -20643.766F, -1673.0955F}},
    {POINT_BOARD, {-660.9364F, -20766.284F, -1748.8923F}},
    {POINT_BOARD, {-754.4009F, -20836.213F, -1858.8375F}},
    {POINT_BOARD, {-847.86353F, -20906.14F, -1968.7806F}},
    {POINT_BOARD, {-941.33F, -20976.065F, -2078.728F}},
    {POINT_BOARD, {-1034.7946F, -21045.995F, -2188.6734F}},
    {POINT_BOARD, {-1128.2609F, -21115.922F, -2298.621F}},
    {POINT_BOARD, {-1217.7869F, -21191.745F, -2407.911F}},
    {POINT_BOARD, {-1306.8473F, -21268.26F, -2517.1231F}},
    {POINT_BOARD, {-1395.9079F, -21344.776F, -2626.335F}},
    {POINT_BOARD, {-1484.9668F, -21421.29F, -2735.545F}},
    {POINT_BOARD, {-1576.4026F, -21497.932F, -2842.656F}},
    {POINT_BOARD, {-1669.5069F, -21574.66F, -2948.2845F}},
    {POINT_BOARD, {-1762.6149F, -21651.391F, -3053.9173F}},
    {POINT_BOARD, {-1855.6336F, -21728.192F, -3159.568F}},
    {POINT_BOARD, {-1944.3305F, -21808.577F, -3266.272F}},
    {POINT_BOARD, {-2033.0276F, -21888.956F, -3372.976F}},
    {POINT_BOARD, {-2121.7217F, -21969.338F, -3479.6766F}},
    {POINT_BOARD, {-2210.4183F, -22049.72F, -3586.3804F}},
    {POINT_BOARD, {-2296.8084F, -22130.688F, -3694.4971F}},
    {POINT_BOARD, {-2381.6683F, -22212.047F, -3803.5547F}},
    {POINT_BOARD, {-2466.529F, -22293.407F, -3912.6116F}},
    {POINT_BOARD, {-2551.3865F, -22374.766F, -4021.6656F}},
    {POINT_BOARD, {-2640.04F, -22455.206F, -4128.2935F}},
    {POINT_BOARD, {-2731.405F, -22534.985F, -4233.1763F}},
    {POINT_BOARD, {-2822.7725F, -22614.766F, -4338.061F}},
    {POINT_BOARD, {-2913.9595F, -22691.297F, -4445.1895F}},
    {POINT_BOARD, {-3004.3785F, -22754.108F, -4561.7813F}},
    {POINT_BOARD, {-3094.7938F, -22816.911F, -4678.3692F}},
    {POINT_BOARD, {-3185.2142F, -22879.72F, -4794.962F}},
    {POINT_BOARD, {-3275.6316F, -22942.528F, -4911.552F}},
    {POINT_BOARD, {-3444.6277F, -23120.918F, -5116.356F}},
    {POINT_BOARD, {-3533.7007F, -23188.766F, -5223.7344F}},
    {POINT_RUN | POINT_STAND, {-3575.0F, -23310.0F, -5341.0F}},
    {POINT_GOAL, {-3717.0F, -23318.0F, -5510.0F}},
    {POINT_END, {-3717.0F, -23270.0F, -5510.0F}},
};
static const Sint32 race_icecap_count = 255;

static const struct race_point race_casino_points[99] = {
    {POINT_RUN, {19.0F, -1694.0F, 2849.0F}},
    {POINT_RUN, {-13.0F, -1834.0F, 2858.0F}},
    {POINT_WAIT, {-40.0F, -1834.9775F, 2867.3736F}},
    {POINT_RUN | POINT_NEVER_STUCK | POINT_STROKE_FULL,
     {-95.0F, -1834.0F, 2870.0F}},
    {POINT_RUN, {-172.31791F, -1837.1204F, 2870.0F}},
    {POINT_RUN, {-269.0F, -1794.0F, 2870.0F}},
    {POINT_RUN, {-334.4111F, -1754.3107F, 2870.0F}},
    {POINT_RUN, {-418.0F, -1734.0F, 2870.0F}},
    {POINT_RUN, {-515.5197F, -1740.0F, 2870.0F}},
    {POINT_RUN, {-597.0F, -1676.0F, 2870.0F}},
    {POINT_RUN, {-661.569F, -1626.6185F, 2870.0F}},
    {POINT_RUN, {-727.0F, -1560.0F, 2870.0F}},
    {POINT_RUN, {-796.3794F, -1496.5381F, 2870.0F}},
    {POINT_RUN, {-855.0F, -1484.0F, 2870.0F}},
    {POINT_RUN, {-900.0F, -1484.0F, 2870.0F}},
    {POINT_WAIT | POINT_WAIT_TIME(5), {-939.0F, -1484.0F, 2870.0F}},
    {POINT_RUN | POINT_NEVER_STUCK | POINT_STROKE_FULL,
     {-982.5289F, -1484.0F, 2870.0F}},
    {POINT_RUN, {-1020.0F, -1484.0F, 2830.0F}},
    {POINT_RUN, {-1060.0F, -1484.0F, 2840.0F}},
    {POINT_RUN, {-1070.0F, -1484.0F, 2869.0F}},
    {POINT_RUN, {-1170.5021F, -1502.5657F, 2870.1561F}},
    {POINT_AIR, {-1317.0F, -1502.0F, 2870.0F}},
    {POINT_AIR, {-1371.3118F, -1308.1716F, 2870.0F}},
    {POINT_AIR, {-1377.9339F, -1117.4069F, 2870.0F}},
    {POINT_AIR, {-1384.564F, -926.4075F, 2870.0F}},
    {POINT_AIR, {-1391.1968F, -735.32782F, 2870.0F}},
    {POINT_RUN, {-1448.0F, -677.0F, 2870.0F}},
    {POINT_WAIT | POINT_WAIT_TIME(10), {-1527.3925F, -696.74903F, 2870.0F}},
    {POINT_RUN | POINT_NEVER_STUCK | POINT_STROKE_FULL,
     {-1593.0F, -706.0F, 2868.0F}},
    {POINT_RUN, {-1701.6193F, -767.08063F, 2857.695F}},
    {POINT_RUN, {-1740.0F, -800.0F, 2815.0F}},
    {POINT_RUN, {-1755.5792F, -875.29523F, 2721.7562F}},
    {POINT_RUN, {-1756.0F, -969.61243F, 2557.0562F}},
    {POINT_RUN, {-1756.0F, -1052.498F, 2386.972F}},
    {POINT_RUN, {-1756.0208F, -1133.0069F, 2216.6666F}},
    {POINT_RUN, {-1756.0208F, -1172.0F, 2126.0F}},
    {POINT_RUN, {-1779.1229F, -1220.6314F, 2055.2103F}},
    {POINT_RUN, {-1838.0F, -1254.0F, 2024.0F}},
    {POINT_RUN, {-1931.8779F, -1312.6092F, 2020.2302F}},
    {POINT_RUN, {-1980.0F, -1333.0F, 2017.0F}},
    {POINT_RUN, {-2093.9913F, -1401.4708F, 2020.0F}},
    {POINT_RUN, {-2249.1817F, -1496.3937F, 2032.2292F}},
    {POINT_RUN, {-2296.0F, -1551.0F, 2070.0F}},
    {POINT_RUN, {-2305.6912F, -1612.1583F, 2146.9964F}},
    {POINT_RUN, {-2308.0F, -1648.0F, 2216.0F}},
    {POINT_RUN, {-2278.1556F, -1709.7471F, 2291.052F}},
    {POINT_RUN, {-2218.0F, -1748.0F, 2315.0F}},
    {POINT_RUN, {-2138.1653F, -1810.5124F, 2325.7923F}},
    {POINT_RUN, {-2092.0F, -1841.0F, 2345.0F}},
    {POINT_RUN, {-2059.7637F, -1917.8975F, 2423.142F}},
    {POINT_RUN, {-2056.0F, -2017.0F, 2590.0F}},
    {POINT_RUN, {-2056.0F, -2076.1841F, 2723.9813F}},
    {POINT_RUN, {-2055.9932F, -2116.1417F, 2885.1006F}},
    {POINT_AIR, {-2060.0F, -2273.0F, 3004.0F}},
    {POINT_AIR, {-2056.0F, -2300.3274F, 3077.335F}},
    {POINT_AIR, {-2055.4544F, -2193.9583F, 3091.8731F}},
    {POINT_AIR, {-2055.3472F, -2040.5207F, 3123.1055F}},
    {POINT_AIR, {-2056.0F, -1910.0F, 3175.6614F}},
    {POINT_RUN, {-2049.3978F, -1912.6409F, 3325.1922F}},
    {POINT_RUN, {-2018.0F, -1918.0F, 3382.0F}},
    {POINT_WAIT | POINT_WAIT_TIME(10), {-1952.2524F, -1954.1517F, 3402.945F}},
    {POINT_RUN | POINT_NEVER_STUCK | POINT_STROKE_FULL,
     {-1820.7706F, -2008.7884F, 3405.0F}},
    {POINT_RUN, {-1692.0306F, -2059.7032F, 3404.7083F}},
    {POINT_RUN, {-1632.0F, -2077.0F, 3397.0F}},
    {POINT_RUN, {-1583.0079F, -2103.066F, 3358.0074F}},
    {POINT_RUN, {-1567.2641F, -2123.0777F, 3231.4625F}},
    {POINT_RUN | POINT_STROKE_FULL | POINT_JUMP_OFF,
     {-1565.0F, -2130.0F, 3140.7134F}},
    {POINT_RUN, {-1566.0F, -2147.09F, 2986.177F}},
    {POINT_RUN | POINT_STROKE_FULL | POINT_JUMP_OFF,
     {-1566.0F, -2146.0F, 2920.0F}},
    {POINT_RUN, {-1565.9999F, -2159.3475F, 2759.025F}},
    {POINT_RUN, {-1566.0F, -2191.3084F, 2663.2608F}},
    {POINT_WAIT | POINT_WAIT_TIME(15), {-1565.0F, -2200.0F, 2591.0F}},
    {POINT_RUN | POINT_NEVER_STUCK | POINT_STROKE_FULL,
     {-1540.0F, -2202.0F, 2543.0F}},
    {POINT_RUN, {-1521.0F, -2204.0F, 2502.0F}},
    {POINT_RUN | POINT_LEAP_NEXT, {-1521.0F, -2204.0F, 2473.0F}},
    {POINT_RUN, {-1521.0F, -2204.0F, 2433.0F}},
    {POINT_RUN | POINT_STROKE_FULL, {-1566.0F, -2202.0F, 2422.0F}},
    {POINT_RUN, {-1595.0F, -2202.0F, 2408.0F}},
    {POINT_RUN, {-1610.0F, -2204.0F, 2375.0F}},
    {POINT_RUN, {-1610.0F, -2204.0F, 2336.0F}},
    {POINT_RUN, {-1610.0F, -2204.0F, 2315.0F}},
    {POINT_RUN, {-1583.0F, -2202.0F, 2285.0F}},
    {POINT_RUN | POINT_LEAP_NEXT, {-1547.0F, -2202.0F, 2253.0F}},
    {POINT_RUN, {-1523.0F, -2204.0F, 2207.0F}},
    {POINT_RUN, {-1523.0F, -2204.0F, 2168.0F}},
    {POINT_RUN, {-1523.0F, -2204.0F, 2130.0F}},
    {POINT_RUN, {-1523.0F, -2204.0F, 2104.0F}},
    {POINT_RUN | POINT_LEAP_NEXT, {-1523.0F, -2204.0F, 2080.0F}},
    {POINT_RUN, {-1523.0F, -2204.0F, 2040.0F}},
    {POINT_RUN, {-1550.0F, -2202.0F, 2031.0F}},
    {POINT_RUN, {-1583.0F, -2202.0F, 2025.0F}},
    {POINT_RUN, {-1610.0F, -2204.0F, 2013.0F}},
    {POINT_RUN, {-1610.0F, -2204.0F, 1969.0F}},
    {POINT_RUN, {-1610.0F, -2204.0F, 1930.0F}},
    {POINT_RUN, {-1595.0F, -2202.0F, 1903.0F}},
    {POINT_RUN | POINT_LEAP_NEXT, {-1576.0F, -2202.0F, 1870.0F}},
    {POINT_RUN, {-1563.0F, -2200.0F, 1830.0F}},
    {POINT_GOAL, {-1568.0F, -2205.0F, 1788.0F}},
    {POINT_END, {-1568.0F, -2150.0F, 1788.0F}},
};
static const Sint32 race_casino_count = 99;

/* each stick_way's axes for race_set_stick: x in the high half, and y in
 * the low */
#define STICK(x, y) (((x) * 0x10000) | ((y) & 0xffff))
static const Sint32 race_sticks[4] = {
    STICK(128, 0),
    STICK(-128, 0),
    STICK(0, -128),
    STICK(0, 128),
};

/* each saying's two voices, at 2 * say and 2 * say + 1.
 * race_sonic_voices are for the races against Sonic, and
 * race_eggman_voices for Speed Highway's */
static const int race_sonic_voices[16] = {
    210, 210, 211, 211, 212, 213, 214, 215,
    216, 217, 218, 218, 219, 219, 209, 209,
};
static const int race_eggman_voices[16] = {
    221, 221, 222, 222, 223, 224, 225, 226,
    227, 228, 229, 229, 230, 230, 220, 220,
};

static NJS_TEXNAME race_textures[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};

NJS_TEXLIST milesrace_texlist = {
#include "player/race/milesrace_texlist.inc"
};

/* each banner's pieces sit side by side, centered on the point
 * race_banner_draw draws them at */
static NJS_TEXANIM race_sprites[12] = {
#include "player/race/race_sprites.inc"
};

/* each course's two lines, the player's then the racer's. race_counts
 * holds their counts in the same order. every course gives both the same
 * line */
static const struct race_point *race_lines[COURSE_COUNT * 2] = {
    race_windy_points,   race_windy_points,    race_highway_points,
    race_highway_points, sky_deck_race_points, sky_deck_race_points,
    race_icecap_points,  race_icecap_points,   race_casino_points,
    race_casino_points,
};

static const Sint32 *race_counts[COURSE_COUNT * 2] = {
    &race_windy_count,   &race_windy_count,    &race_highway_count,
    &race_highway_count, &sky_deck_race_count, &sky_deck_race_count,
    &race_icecap_count,  &race_icecap_count,   &race_casino_count,
    &race_casino_count,
};

/* the params that the sparks of Eggman's three jets share */
static struct dust_params race_spark_params = {
    0.75F,
    0.4F,
    6.1F,
    0.05F,
    0.0F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.5F, 0.0F},
    {5.0F, 0.3F, 0.3F, 1.0F},
};

static struct race_spark race_sparks[3] = {
    {&race_spark_params, {8.1F, 4.886F, -7.747F}, {8.1F, 4.886F, -9.747F}},
    {&race_spark_params, {0.0F, 10.077F, -13.371F}, {0.0F, 10.077F, -15.371F}},
    {&race_spark_params, {-8.1F, 4.886F, -7.747F}, {-8.1F, 4.886F, -9.747F}},
};

/* where Eggman's jet glares from, and toward, in his frame */
static NJS_POINT3 race_glare_from = {0.0F, -0.6F, 9.714F};
static NJS_POINT3 race_glare_to = {0.0F, -0.6F, 10.714F};

/* @hack type: an array of one, not the single shape it is, so
 * race_eggman_init takes 1 into R5 for its stores of 1 (MOV #1,R5 at
 * 0x8c08482c) and into R2 again for collision_init's last two arguments
 * (0x8c084886); as one shape, SHC keeps 1 in R13 and saves R12, the
 * function runs 2 bytes longer and 63 of its 69 words differ */
/* clang-format off */
static struct collision_shape race_eggman_shape[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(2) | GUARD(3) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F}, 12.0F, 0.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* TRUE while Tails races for mission A, in which Sonic and Eggman go
 * faster */
static Bool race_mission_a = FALSE;

void race_create(Uint16 character, Sint16 player, Sint16 stage)
{
    struct task *task;

    if (character != CHARACTER_TAILS) {
        return;
    }
    if (player != 1) {
        return;
    }
    race_result = RACE_RUNNING;
    race_mission_a = race_check_mission_a();
    switch (stage) {
    case STAGE_WINDY:
        if ((Uint8)stage_get_stage_act() == 2) {
            race_set_course(COURSE_WINDY);
            task_create(TASK_EXTRA | TASK_WORK, 0, race_start);
            task = task_create(TASK_MOTION | TASK_WORK | TASK_FORCE, 1,
                               sonic_step);
            PLAYER_VARS(task->work)->character = CHARACTER_SONIC;
            PLAYER_VARS(task->work)->player = 1;
            task->work->id = RACER_ID;
            pad_disable_player(1);
        }
        break;
    case STAGE_HIGHWAY:
        if ((Uint8)stage_get_stage_act() == 0) {
            race_set_course(COURSE_HIGHWAY);
            task_create(TASK_EXTRA | TASK_WORK, 0, race_eggman_start);
        }
        break;
    case STAGE_SKY_DECK:
        if ((Uint8)stage_get_stage_act() == 0) {
            race_set_course(COURSE_SKY_DECK);
            task_create(TASK_EXTRA | TASK_WORK, 0, race_start);
            task = task_create(TASK_MOTION | TASK_WORK | TASK_FORCE, 1,
                               sonic_step);
            PLAYER_VARS(task->work)->character = CHARACTER_SONIC;
            PLAYER_VARS(task->work)->player = 1;
            task->work->id = RACER_ID;
            pad_disable_player(1);
        }
        break;
    case STAGE_ICECAP:
        if ((Uint8)stage_get_stage_act() == 2) {
            if (game_mode == GAME_MODE_TRIAL &&
                menu_state.trial[0] == MENU_TRIAL_MINI_GAME) {
                return;
            }
            race_set_course(COURSE_ICECAP);
            task_create(TASK_EXTRA | TASK_WORK, 0, race_start);
            task = task_create(TASK_MOTION | TASK_WORK | TASK_FORCE, 1,
                               sonic_step);
            PLAYER_VARS(task->work)->character = CHARACTER_SONIC;
            PLAYER_VARS(task->work)->player = 1;
            task->work->id = RACER_ID;
            pad_disable_player(1);
        }
        break;
    case STAGE_CASINO:
        /* @hack spelling: a break out of the case where the other cases nest
         * the race in an if. the jump skips what the if would, and loads the
         * new task's work into R2 for its first store (MOV.L @(32,R4),R2 at
         * 0x8c0841c0); nested, R3 takes it and 2 of race_create's 200 words
         * differ */
        if ((Uint8)stage_get_stage_act() != 1) {
            break;
        }
        race_set_course(COURSE_CASINO);
        task_create(TASK_EXTRA | TASK_WORK, 0, race_start);
        task = task_create(TASK_MOTION | TASK_WORK | TASK_FORCE, 1, sonic_step);
        PLAYER_VARS(task->work)->character = CHARACTER_SONIC;
        PLAYER_VARS(task->work)->player = 1;
        task->work->id = RACER_ID;
        pad_disable_player(1);
        break;
    }
}

/* the first step of the race task in a race against Sonic. it stores
 * Sonic's player number and character in its work's first two bytes, as
 * his own task has them. race_set_lasts then writes last[0] over both
 * bytes, so nothing reads those stores */
static void race_start(struct task *task)
{
    struct task_work *work = task->work;

    /* @hack spelling: task->work read again for each store, as the image does
     * (MOV.L @(32,R14),R3 at 0x8c0841f2 and 0x8c0841fe); through work, the
     * function runs 4 bytes shorter and 32 of its 36 words differ */
    PLAYER_VARS(task->work)->character = CHARACTER_SONIC;
    PLAYER_VARS(task->work)->player = 1;
    race_set_lines();
    race_set_lasts(work);
    if (race_mission_a == TRUE) {
        race_sonic_pace = 1.0F;
        trial_count = 1;
    } else {
        race_sonic_pace = 0.6F;
    }
    task->step = race_step;
}

static void race_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct task_work *sonic = player_work[1];
    int i;

    if (player == NULL || sonic == NULL) {
        task_kill(task);
        return;
    }
    /* @hack redundant: sonic tested against NULL again, though the test
     * above returned if it was. the image's BT at 0x8c08425a reuses that
     * test's T; without it, 59 of race_step's 181 words differ */
    if (sonic != NULL) {
        /* so that Sonic's attacks never hurt Tails */
        for (i = 0; i < sonic->collision->count; i++) {
            sonic->collision->shapes[i].levels &= ~SHAPE_HURTS_PLAYERS;
        }
    }
    switch (race_course) {
    case COURSE_WINDY:
        switch (work->mode) {
        case RACE_START:
            race_start_windy(work, sonic, player);
            break;
        case RACE_RUN:
            race_run(task, sonic, player);
            break;
        case RACE_OVER:
            break;
        }
        break;
    case COURSE_HIGHWAY:
        break;
    case COURSE_ICECAP:
        switch (work->mode) {
        case RACE_START:
            race_start_icecap(work, sonic, player);
            break;
        case RACE_RUN:
            race_run(task, sonic, player);
            break;
        /* @hack redundant: case 3, a mode nothing sets, where the other
         * courses' switches have RACE_OVER. it keeps the image's CMP/EQ #3 at
         * 0x8c084334; without it, 171 of race_step's 181 words differ */
        case 3:
            break;
        }
        break;
    case COURSE_SKY_DECK:
        switch (work->mode) {
        case RACE_START:
            race_start_sky_deck(work, sonic, player);
            break;
        case RACE_RUN:
            race_run(task, sonic, player);
            break;
        case RACE_OVER:
            break;
        }
        break;
    case COURSE_CASINO:
        switch (work->mode) {
        case RACE_START:
            race_start_casino(work, sonic, player);
            break;
        case RACE_RUN:
            race_run(task, sonic, player);
            break;
        case RACE_OVER:
            break;
        }
        break;
    default:
        task_kill(task);
        return;
    }
    race_draw_bar(VARS(work)->point[0], VARS(work)->last[0],
                  VARS(work)->point[1], VARS(work)->last[1]);
}

/* Sonic starts ten to the player's side, and one up */
static void race_start_windy(struct task_work *work, struct task_work *sonic,
                             struct task_work *player)
{
    work->mode = RACE_RUN;
    sonic->ang = player->ang;
    scratch_vector.x = 0.0F;
    scratch_vector.y = 1.0F;
    scratch_vector.z = 10.0F;
    player_turn_to_world(player, &scratch_vector);
    vector_add(&player->pos, &scratch_vector, &sonic->pos);
    VARS(work)->point[0] =
        race_nearest_point(&player->pos, race_player_line, VARS(work)->last[0]);
    VARS(work)->point[1] =
        race_nearest_point(&sonic->pos, race_racer_line, VARS(work)->last[1]);
    race_aim_at(VARS(work)->point[0]);
}

static void race_run(struct task *task, struct task_work *sonic,
                     struct task_work *player)
{
    struct task_work *work = task->work;
    struct race_aim *aim = &race_aim;
    Sint16 player_point;
    Sint16 racer_point;

    vector_between(&sonic->collision->shapes->center, &aim->to, &aim->ahead);
    aim->distance = njScalor2(&aim->ahead);
    player_point = race_player_point(work, player);
    racer_point = race_sonic_reach(work, sonic, aim);
    race_sonic_drive(work, sonic, aim);
    race_voice_step(task, player_point, racer_point);
    VARS(work)->point[0] = player_point;
    VARS(work)->point[1] = racer_point;
}

/* Sonic starts on his board, ten to the player's side and one up. he is
 * aimed from the course's first point at its second */
static void race_start_icecap(struct task_work *work, struct task_work *sonic,
                              struct task_work *player)
{
    const struct race_point *point = race_racer_line;
    struct race_aim *aim = &race_aim;
    struct task *board;
    /* @hack spelling: the scratch vector through a pointer, which keeps player
     * in R11 (MOV R6,R11 at 0x8c0844fe); through scratch_vector itself,
     * player takes R12 and 27 of race_start_icecap's 92 words differ */
    NJS_VECTOR *offset;
    Sint32 *word;
    int i;

    work->mode = RACE_RUN;
    player_set_mode(1, PLAYER_MODE_BOARD);
    sonic->ang = player->ang;
    offset = &scratch_vector;
    offset->x = 0.0F;
    offset->y = 1.0F;
    offset->z = 10.0F;
    player_turn_to_world(player, offset);
    sonic->pos.x = player->pos.x + offset->x;
    sonic->pos.y = player->pos.y + offset->y;
    sonic->pos.z = player->pos.z + offset->z;
    board = task_create(TASK_WORK | TASK_MOTION, 2, sonic_board_start);
    /* the board's rider: player 1, whose number starts the board's vars */
    board->work->vars.b[0] = 1;
    word = (Sint32 *)aim;
    for (i = sizeof(*aim) / sizeof(*word); i != 0; i--) {
        *word++ = 0;
    }
    aim->from = point->pos;
    point++;
    aim->to = point->pos;
    aim->attr = point->attr;
}

static void race_start_sky_deck(struct task_work *work, struct task_work *sonic,
                                struct task_work *player)
{
    work->mode = RACE_RUN;
    sonic->ang = player->ang;
    scratch_vector.x = 0.0F;
    scratch_vector.y = 1.0F;
    scratch_vector.z = 10.0F;
    player_turn_to_world(player, &scratch_vector);
    vector_add(&player->pos, &scratch_vector, &sonic->pos);
    VARS(work)->point[0] =
        race_nearest_point(&player->pos, race_player_line, VARS(work)->last[0]);
    VARS(work)->point[1] =
        race_nearest_point(&sonic->pos, race_racer_line, VARS(work)->last[1]);
    race_aim_at(VARS(work)->point[0]);
}

static void race_start_casino(struct task_work *work, struct task_work *sonic,
                              struct task_work *player)
{
    work->mode = RACE_RUN;
    sonic->ang = player->ang;
    scratch_vector.x = 0.0F;
    scratch_vector.y = 1.0F;
    scratch_vector.z = 10.0F;
    player_turn_to_world(player, &scratch_vector);
    vector_add(&player->pos, &scratch_vector, &sonic->pos);
    VARS(work)->point[0] =
        race_nearest_point(&player->pos, race_player_line, VARS(work)->last[0]);
    VARS(work)->point[1] =
        race_nearest_point(&sonic->pos, race_racer_line, VARS(work)->last[1]);
    race_aim_at(VARS(work)->point[0]);
}

/* the first step of Speed Highway's race task, which is Eggman. he flies
 * the racer's line, and the player's line measures Tails. it calls
 * race_set_lasts twice, and the second call writes what the first did */
static void race_eggman_start(struct task *task)
{
    struct task_work *work = task->work;

    race_set_lines();
    race_set_lasts(work);
    race_set_lasts(work);
    task->step = race_eggman_step;
    task->draw = race_eggman_draw;
}

static void race_eggman_step(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: eggman declared before player, which puts &race_eggman's
     * pool word at 0x8c084780 and player_work's at 0x8c084784; the other
     * way round, the two words trade and 2 of race_eggman_step's 95 words
     * differ */
    struct race_eggman *eggman = &race_eggman;
    struct task_work *player = player_work[0];

    if (player == NULL) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case RACE_START:
        race_eggman_init(task, eggman);
        /* fall through */
    case RACE_RUN:
        race_eggman_run(task, eggman);
        break;
    case RACE_OVER:
        race_eggman_bob(work, eggman);
        break;
    }
    collision_register(work);
    object_step_draw(task);
    race_eggman_spark(task, &race_sparks[0]);
    race_eggman_spark(task, &race_sparks[1]);
    race_eggman_spark(task, &race_sparks[2]);
    race_eggman_glare(task);
    race_draw_bar(VARS(work)->point[0], VARS(work)->last[0],
                  VARS(work)->point[1], VARS(work)->last[1]);
}

static void race_eggman_init(struct task *task, struct race_eggman *eggman)
{
    struct task_work *work = task->work;
    struct task_work *player = player_get_task(0)->work;

    work->mode = RACE_RUN;
    if (race_mission_a == TRUE) {
        race_eggman_top_speed = 3.5F;
        race_eggman_turn_speed = 2.2F;
        trial_count = 1;
    } else {
        race_eggman_top_speed = 2.0F;
        race_eggman_turn_speed = 0.08F;
    }
    VARS(work)->point[0] =
        race_nearest_point(&player->pos, race_player_line, VARS(work)->last[0]);
    VARS(work)->point[1] =
        race_nearest_point(&player->pos, race_racer_line, VARS(work)->last[1]);
    race_eggman_place(work, VARS(work)->point[1], eggman);
    collision_init(task, race_eggman_shape, COLLISION_OBJECT, 1, 1);
}

/* the race ends once Tails or Eggman comes within FINISH_RANGE of the
 * player's line's last point. the stage then clears. Tails wins if both
 * come within it in one step */
static void race_eggman_run(struct task *task, struct race_eggman *eggman)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    const NJS_POINT3 *end = &race_player_line[VARS(work)->last[0]].pos;
    Sint16 player_point;
    Sint16 eggman_point;
    Float distance;

    vector_between(&player->pos, end, &scratch_vector);
    distance = njScalor2(&scratch_vector);
    if (distance < FINISH_RANGE) {
        work->mode = RACE_OVER;
        race_set_won();
        stage_clear();
        return;
    }
    vector_between(&work->pos, end, &scratch_vector);
    distance = njScalor2(&scratch_vector);
    if (distance < FINISH_RANGE) {
        work->mode = RACE_OVER;
        race_set_lost();
        stage_clear();
        return;
    }
    player_point = race_player_point(work, player);
    eggman_point = race_eggman_reach(work, eggman);
    race_eggman_steer(work, eggman);
    race_eggman_bob(work, eggman);
    race_eggman_turn(work, eggman);
    race_voice_step(task, player_point, eggman_point);
    VARS(work)->point[1] = eggman_point;
    VARS(work)->point[0] = player_point;
}

/* aim Sonic from the point at the next, with the rest of race_aim cleared */
static void race_aim_at(Sint16 point)
{
    const struct race_point *from = &race_racer_line[point];
    struct race_aim *aim = &race_aim;
    Sint32 *word;
    int i;

    word = (Sint32 *)aim;
    for (i = sizeof(*aim) / sizeof(*word); i != 0; i--) {
        *word++ = 0;
    }
    aim->from = from->pos;
    from++;
    aim->to = from->pos;
    aim->attr = from->attr;
}

/* put Eggman at the point, aimed at the next, with the rest of his state
 * cleared */
static void race_eggman_place(struct task_work *work, Sint16 point,
                              struct race_eggman *eggman)
{
    const struct race_point *from = &race_racer_line[point];
    Sint32 *word;
    int i;

    word = (Sint32 *)eggman;
    for (i = sizeof(*eggman) / sizeof(*word); i != 0; i--) {
        *word++ = 0;
    }
    vector_copy(&from->pos.x, &work->pos.x);
    vector_copy(&from->pos.x, &eggman->from.x);
    from++;
    vector_copy(&from->pos.x, &eggman->to.x);
}

static void race_set_lines(void)
{
    const struct race_point **line = &race_lines[race_course * 2];

    race_player_line = *line++;
    race_racer_line = *line;
}

static void race_set_lasts(struct task_work *work)
{
    const Sint32 **count = &race_counts[race_course * 2];

    VARS(work)->last[0] = **count++ - 1;
    VARS(work)->last[1] = **count - 1;
}

/* the player's nearest point, measured at each frame's first step and kept
 * through its others */
static Sint16 race_player_point(struct task_work *work,
                                struct task_work *player)
{
    if (frame_step != 0) {
        return VARS(work)->point[0];
    }
    return race_nearest_point(&player->pos, race_player_line,
                              VARS(work)->last[0]);
}

static Sint16 race_nearest_point(NJS_POINT3 *pos,
                                 const struct race_point *point, Sint16 count)
{
    int i;
    int nearest;
    Float best;
    Float distance;

    best = 100000000.0F;
    /* @hack spelling: nearest cleared in the loop's first clause, after i,
     * which puts its zero in the BF/S's delay slot (MOV R4,R9 at
     * 0x8c084b68); cleared before the loop, i's zero takes the slot and 2
     * of race_nearest_point's 60 words differ */
    for (i = 0, nearest = 0; i < count; i++) {
        scratch_vector.x = pos->x - point->pos.x;
        scratch_vector.y = pos->y - point->pos.y;
        scratch_vector.z = pos->z - point->pos.z;
        distance = njScalor2(&scratch_vector);
        if (distance < best) {
            best = distance;
            nearest = i;
        }
        point++;
    }
    return nearest;
}

/* return the last point Sonic has reached. he reaches the point he heads
 * for as its kind says, and is then aimed at the next. the race also moves
 * him on to the point when he strays farther than ASTRAY from it. it does
 * the same when he falls more than MOST_BEHIND points behind the player.
 * in mission A, the limit is MOST_BEHIND_A. that move puts him PUT_HEIGHT
 * above the point. he stands again, unless he then heads for a POINT_BOARD */
static Sint16 race_sonic_reach(struct task_work *work, struct task_work *sonic,
                               struct race_aim *aim)
{
    Sint16 point = VARS(work)->point[1];
    Sint16 next;
    Float dot;
    Float across;
    int behind;
    Sint32 wait;
    /* @hack type: an int, which takes the count sign-extended before the
     * increment (EXTS.W R0,R4 at 0x8c084ef0); as a Sint16, SHC stores the
     * new count first and takes 1 off it, and 44 of race_sonic_reach's 382
     * words differ */
    int steps;

    switch (POINT_KIND(aim->attr)) {
    case POINT_COAST_NEAR:
    case POINT_DASH_NEAR:
        if (aim->distance < NEAR) {
            point = race_sonic_next(work, sonic, aim, point);
        }
        break;
    case POINT_PUSH:
        across = (aim->ahead.x * aim->ahead.x) + (aim->ahead.z * aim->ahead.z);
        if (across < NEAR) {
            point = race_sonic_next(work, sonic, aim, point);
        }
        break;
    case POINT_DASH:
    case POINT_COAST:
    case POINT_PATH:
    case POINT_AIR:
        vector_between(&aim->from, &aim->to, &scratch_vector);
        dot = njInnerProduct(&aim->ahead, &scratch_vector);
        if (dot <= 0.0F) {
            point = race_sonic_next(work, sonic, aim, point);
        }
        break;
    case POINT_RUN:
    case POINT_EASE:
    case POINT_SPIN_UP:
        vector_between(&aim->from, &aim->to, &scratch_vector);
        dot = njInnerProduct(&aim->ahead, &scratch_vector);
        if (dot <= PAST_EARLY || aim->distance < NEAR) {
            point = race_sonic_next(work, sonic, aim, point);
        }
        break;
    case POINT_RUN_LEVEL:
        vector_between(&aim->from, &aim->to, &scratch_vector);
        scratch_vector.y = 0.0F;
        scratch_vector2 = aim->ahead;
        scratch_vector2.y = 0.0F;
        dot = njInnerProduct(&scratch_vector2, &scratch_vector);
        if (dot <= PAST_EARLY || aim->distance < NEAR) {
            point = race_sonic_next(work, sonic, aim, point);
        }
        break;
    case POINT_REQUESTED:
        if (sonic->flags & PLAYER_REQUEST) {
            point = race_sonic_next(work, sonic, aim, point);
        }
        break;
    case POINT_LAUNCHED:
        if (sonic->submode == PLAYER_MODE_LAUNCH ||
            sonic->submode == PLAYER_MODE_SPRING ||
            sonic->submode == PLAYER_MODE_LAUNCH_SPEED) {
            point = race_sonic_next(work, sonic, aim, point);
        }
        break;
    case POINT_LEDGE:
        if (sonic->submode == PLAYER_MODE_LEDGE) {
            point = race_sonic_next(work, sonic, aim, point);
        }
        break;
    case POINT_LAND:
        if (sonic->flags & PLAYER_ON_GROUND) {
            point = race_sonic_next(work, sonic, aim, point);
        }
        break;
    case POINT_BOARD:
        vector_between(&aim->from, &aim->to, &scratch_vector);
        dot = njInnerProduct(&aim->ahead, &scratch_vector);
        if (dot <= PAST_EARLY || aim->distance < NEAR) {
            point = race_sonic_next(work, sonic, aim, point);
            if (aim->attr & POINT_STAND) {
                player_set_mode(1, PLAYER_MODE_RESET);
                pad_disable_player(1);
            }
        }
        break;
    case POINT_WAIT:
        if (VARS(work)->point[0] > point || race_mission_a == TRUE) {
            point = race_sonic_next(work, sonic, aim, point);
            break;
        }
        wait = aim->attr & POINT_WAIT_MASK;
        if (wait != 0) {
            wait >>= POINT_WAIT_SHIFT;
            if (VARS(work)->steps > wait * WAIT_STEPS) {
                point = race_sonic_next(work, sonic, aim, point);
                break;
            }
        }
        /* fall through */
    /* @hack redundant: an empty last case, which POINT_WAIT falls into, and
     * which puts POINT_GOAL in the switch's table: it holds 19 kinds (MOV
     * #19,R1 at 0x8c084c0c); without it, the table holds 18 and 125 of
     * race_sonic_reach's 382 words differ */
    case POINT_GOAL:
        break;
    }
    if (point == VARS(work)->point[1]) {
        steps = VARS(work)->steps++;
        if (POINT_KIND(aim->attr) == POINT_BOARD && steps > BOARD_STEPS) {
            if (VARS(work)->point[0] - VARS(work)->point[1] < BOARD_SKIP_LEAD) {
                pad_records[1].press |= pad_jump_buttons;
            } else {
                point = race_sonic_next(work, sonic, aim, point);
                sonic->pos = aim->from;
            }
        }
    }
    behind = race_mission_a == TRUE ? MOST_BEHIND_A : MOST_BEHIND;
    if (aim->distance > ASTRAY || VARS(work)->point[0] - point > behind) {
        next = race_sonic_next(work, sonic, aim, point);
        player_set_position(1, aim->from.x, aim->from.y + PUT_HEIGHT,
                            aim->from.z);
        if (POINT_KIND(aim->attr) != POINT_BOARD) {
            player_set_mode(1, PLAYER_MODE_RESET);
        }
        return next;
    }
    return point;
}

/* set the record's stick axes, x1 and y1, from a race_sticks word */
static void race_set_stick(PDS_PERIPHERAL *pad, Sint32 stick)
{
    pad->x1 = stick >> 16;
    pad->y1 = stick;
}

/* drive Sonic's pad as the kind of the point he heads for says */
static void race_sonic_drive(struct task_work *work, struct task_work *sonic,
                             struct race_aim *aim)
{
    NJS_VECTOR *ahead = &aim->ahead;
    struct player *record = player_get(1);
    struct task_work *player;
    Float level;
    Float speed;
    Float dot;
    Float push;
    int lead;
    Bool restart;

    switch (POINT_KIND(aim->attr)) {
    case POINT_RUN:
    case POINT_RUN_LEVEL:
        pad_sticks[1].ang = race_sonic_heading(work, sonic, aim);
        pad_sticks[1].stroke = race_sonic_stroke(aim);
        /* stalled on a slope of 25 to 45 degrees, he spins up */
        level = -njInnerProduct(&world_gravity, &record->ground_normal);
        if (level > njCos(NJM_DEG_ANG(45)) && level < njCos(NJM_DEG_ANG(25))) {
            speed = record->speed.x;
            if (aim->speed > speed && speed < 0.1F &&
                njScalor(&record->speed) < 0.1F) {
                aim->resume = aim->attr;
                aim->attr = POINT_SPIN_UP;
                pad_records[1].press |= pad_action_buttons;
                VARS(work)->spin_steps = 0;
            }
        }
        if (race_sonic_stuck(sonic)) {
            race_sonic_leap(sonic, &aim->to, LEAP_STEPS);
            aim->attr = POINT_LEAP;
        }
        break;
    case POINT_DASH:
    case POINT_DASH_NEAR:
        pad_sticks[1].ang = race_sonic_heading(work, sonic, aim);
        pad_sticks[1].stroke = 1.0F;
        if (race_sonic_stuck(sonic)) {
            race_sonic_leap(sonic, &aim->to, LEAP_STEPS);
            aim->attr = POINT_LEAP;
        }
        break;
    case POINT_EASE:
        pad_sticks[1].ang = race_sonic_heading(work, sonic, aim);
        pad_sticks[1].stroke = race_stroke(ahead, EASE_REACH, 0.6F);
        break;
    case POINT_AIR:
        if (!(sonic->flags & PLAYER_ON_GROUND)) {
            pad_sticks[1].ang = race_sonic_heading(work, sonic, aim);
            pad_sticks[1].stroke = race_stroke(ahead, EASE_REACH, 0.6F);
        }
        break;
    case POINT_LAUNCHED:
    case POINT_REQUESTED:
        pad_sticks[1].ang = race_sonic_heading(work, sonic, aim);
        pad_sticks[1].stroke = race_stroke(ahead, EASE_REACH, 1.0F);
        if (race_sonic_stuck(sonic) && (sonic->flags & PLAYER_ON_GROUND)) {
            race_sonic_leap(sonic, &aim->to, LEAP_STEPS);
            aim->attr = POINT_LEAP;
        }
        break;
    case POINT_PUSH:
        race_set_stick(&pad_records[1], race_sticks[POINT_STICK(aim->attr)]);
        break;
    case POINT_LEDGE:
        pad_sticks[1].ang = race_sonic_heading(work, sonic, aim);
        pad_sticks[1].stroke = race_stroke(ahead, EASE_REACH, race_sonic_pace);
        if (race_sonic_near_across(aim)) {
            if (sonic->flags & PLAYER_ON_GROUND) {
                pad_records[1].press |= pad_jump_buttons;
            }
        }
        break;
    case POINT_SPIN_UP:
        pad_sticks[1].ang = race_sonic_heading(work, sonic, aim);
        pad_sticks[1].stroke = 1.0F;
        if (VARS(work)->spin_steps++ > SPIN_STEPS) {
            aim->attr = aim->resume;
        } else {
            pad_records[1].on |= pad_action_buttons;
        }
        break;
    case POINT_PATH:
        if (sonic->flags & PLAYER_ON_PATH) {
            race_set_stick(&pad_records[1],
                           race_sticks[POINT_STICK(aim->attr)]);
        } else {
            pad_sticks[1].ang = race_sonic_heading(work, sonic, aim);
            pad_sticks[1].stroke = race_sonic_stroke(aim);
        }
        break;
    case POINT_LEAP:
        if (race_sonic_fly(sonic)) {
            aim->attr = aim->resume;
        }
        break;
    case POINT_WAIT:
        switch (work->submode) {
        case WAIT_ARRIVE:
            pad_sticks[1].ang = race_sonic_heading(work, sonic, aim);
            pad_sticks[1].stroke = race_stroke(ahead, EASE_REACH, 0.6F);
            vector_between(&aim->from, &aim->to, &scratch_vector);
            dot = njInnerProduct(&aim->ahead, &scratch_vector);
            if (dot <= 0.0F || aim->distance < NEAR) {
                work->submode = WAIT_READY;
                player_set_mode(1, PLAYER_MODE_READY);
            }
            break;
        case WAIT_READY:
            if (TASK_TIMER(work)++ > READY_STEPS) {
                player_set_mode(1, PLAYER_MODE_READY);
                TASK_TIMER(work) = 0;
            }
            break;
        }
        break;
    case POINT_BOARD:
        pad_sticks[1].ang = race_sonic_heading(work, sonic, aim);
        pad_sticks[1].stroke = race_sonic_stroke(aim);
        lead = VARS(work)->point[0] - VARS(work)->point[1];
        if (!race_mission_a) {
            if (lead > 0 && VARS(work)->point[1] < BOARD_PUSH_END) {
                scratch_vector = aim->ahead;
                scratch_vector.y = 0.0F;
                vector_normalize(&scratch_vector);
                if (lead > 6) {
                    push = 0.2F;
                } else if (lead > 3) {
                    push = 0.1F;
                } else {
                    push = 0.05F;
                }
                player_set_accel(1, scratch_vector.x * push, -push,
                                 scratch_vector.z * push);
            }
        } else {
            scratch_vector = aim->ahead;
            scratch_vector.y = 0.0F;
            vector_normalize(&scratch_vector);
            push = 0.05F;
            player_set_accel(1, scratch_vector.x * push, -push,
                             scratch_vector.z * push);
        }
        break;
    case POINT_GOAL:
        switch (work->submode) {
        case GOAL_ARRIVE:
            if (race_outcome() != RACE_RUNNING) {
                work->submode = GOAL_DONE;
            } else {
                work->submode = GOAL_JUMP;
                pad_records[1].press |= pad_jump_buttons_signed;
                aim->jumps = 0;
            }
            break;
        case GOAL_JUMP:
            pad_sticks[1].ang = race_sonic_heading(work, sonic, aim);
            pad_sticks[1].stroke = race_stroke(ahead, EASE_REACH, 0.6F);
            pad_records[1].on |= pad_jump_buttons_signed;
            if (sonic->flags & PLAYER_ON_GROUND) {
                work->submode = GOAL_LANDED;
                TASK_TIMER(work) = 0;
            }
            break;
        case GOAL_LANDED:
            if (race_outcome() != RACE_RUNNING) {
                player = player_work[0];
                vector_between(&player->pos, &sonic->pos, &scratch_vector);
                if (njScalor2(&scratch_vector) < NEAR) {
                    player_set_accel(1, scratch_vector.x, scratch_vector.y,
                                     scratch_vector.z);
                }
                return;
            }
            if (TASK_TIMER(work)++ > GOAL_REST) {
                work->submode = GOAL_JUMP;
                pad_records[1].press |= pad_jump_buttons;
                if (aim->jumps++ > 3) {
                    work->submode = GOAL_LEAP;
                    scratch_vector = aim->to;
                    scratch_vector.y += GOAL_HEIGHT;
                    race_sonic_leap(sonic, &scratch_vector, LEAP_STEPS);
                }
            }
            break;
        case GOAL_LEAP:
            if (race_sonic_fly(sonic)) {
                work->submode = GOAL_FALL;
            }
            break;
        case GOAL_FALL:
            if (sonic->flags & PLAYER_ON_GROUND) {
                work->submode = GOAL_LANDED;
                TASK_TIMER(work) = 0;
            }
            break;
        case GOAL_DONE:
            break;
        }
        break;
    }
    /* once the steps on the way pass his kind's limit, he leaps for the
     * point */
    restart = FALSE;
    switch (POINT_KIND(aim->attr)) {
    case POINT_RUN:
    case POINT_RUN_LEVEL:
    case POINT_LAUNCHED:
        if (VARS(work)->steps > MOST_STEPS_RUN) {
            restart = TRUE;
        }
        break;
    case POINT_EASE:
    case POINT_LAND:
    case POINT_SPIN_UP:
    case POINT_PATH:
    case POINT_GOAL:
        if (VARS(work)->steps > MOST_STEPS_EASE) {
            restart = TRUE;
        }
        break;
    case POINT_PUSH:
    case POINT_LEDGE:
        if (VARS(work)->steps > MOST_STEPS_PUSH) {
            restart = TRUE;
        }
        break;
    case POINT_COAST_NEAR:
    case POINT_DASH:
    case POINT_COAST:
    case POINT_REQUESTED:
        if (VARS(work)->steps > MOST_STEPS_COAST) {
            restart = TRUE;
        }
        break;
    case POINT_AIR:
        if ((sonic->flags & PLAYER_ON_GROUND) ||
            VARS(work)->steps > MOST_STEPS_COAST) {
            restart = TRUE;
        }
        break;
    case POINT_LEAP:
        break;
    }
    if (restart) {
        VARS(work)->steps = 0;
        race_sonic_leap(sonic, &aim->to, LEAP_STEPS);
        aim->attr = POINT_LEAP;
    }
    aim->speed = record->speed.x;
}

/* start Sonic's leap to the point, steps long, curled up and with his
 * collision off. it saves race_aim's attr in resume */
static void race_sonic_leap(struct task_work *sonic, NJS_POINT3 *to, int steps)
{
    struct jump *leap = &race_leap;
    struct player *record = player_get(1);

    race_aim.resume = race_aim.attr;
    player_set_mode(1, PLAYER_MODE_BALL);
    leap->pos = sonic->pos;
    leap->target = *to;
    leap->gravity = record->consts.gravity;
    leap->steps = steps;
    jump_aim(leap);
    collision_shapes_off(sonic);
}

/* step Sonic's leap, and return TRUE once its steps have run out, else
 * FALSE. his collision comes back on as he starts to fall. at the end he
 * stands again, at the leap's target unless the leap began within 2 of
 * it. jump_landed measures from leap's pos, where it began */
static Bool race_sonic_fly(struct task_work *sonic)
{
    struct jump *leap = &race_leap;
    enum jump_state state;

    player_set_position(1, sonic->pos.x + leap->speed.x,
                        sonic->pos.y + leap->speed.y,
                        sonic->pos.z + leap->speed.z);
    leap->speed.y -= leap->gravity;
    if (leap->speed.y < 0.0F) {
        collision_shapes_on(sonic);
    }
    state = jump_landed(leap);
    if (state == JUMP_FLYING) {
        return FALSE;
    }
    player_set_mode(1, PLAYER_MODE_RESET);
    if (state == JUMP_OFF_TARGET) {
        player_set_position(1, leap->target.x, leap->target.y, leap->target.z);
    }
    return TRUE;
}

static Bool race_sonic_near_across(struct race_aim *aim)
{
    Float across =
        (aim->ahead.x * aim->ahead.x) + (aim->ahead.z * aim->ahead.z);

    return across < NEAR;
}

/* the stick's angle toward the point. at a POINT_RUN on the ground, it
 * can keep his last angle instead. it does while he faces along the
 * stretch with the point behind him. it doesn't once point[1] is less than
 * two short of his line's last */
static Angle race_sonic_heading(struct task_work *work, struct task_work *sonic,
                                struct race_aim *aim)
{
    Angle last = aim->heading;
    Angle ang;
    Float along;
    Float front;
    Sint16 next;

    ang =
        (Angle)(atan2f(aim->ahead.z, aim->ahead.x) * ANGLE_PER_RADIAN) & 0xffff;
    if (POINT_KIND(aim->attr) == POINT_RUN &&
        (sonic->flags & PLAYER_ON_GROUND)) {
        next = VARS(work)->point[1] + 2;
        if (next <= VARS(work)->last[1]) {
            scratch_vector.x = 1.0F;
            scratch_vector.y = 0.0F;
            scratch_vector.z = 0.0F;
            player_turn_to_world(sonic, &scratch_vector);
            vector_between(&aim->from, &aim->to, &scratch_vector2);
            along = njInnerProduct(&scratch_vector, &scratch_vector2);
            scratch_vector3 = aim->ahead;
            scratch_vector3.y = 0.0F;
            front = njInnerProduct(&scratch_vector, &scratch_vector3);
            if (along > 0.0F && front < 0.0F) {
                ang = last;
            }
        }
    }
    aim->heading = ang;
    return ang;
}

/* a stroke by Sonic's distance across from the point. it is most at reach
 * or beyond, and in proportion nearer. it is never under 0.2 */
static Float race_stroke(NJS_VECTOR *ahead, Float reach, Float most)
{
    Float across = njSqrt((ahead->x * ahead->x) + (ahead->z * ahead->z));
    Float stroke;

    if (across > reach) {
        across = reach;
    }
    stroke = most * across / reach;
    if (stroke < 0.2F) {
        stroke = 0.2F;
    }
    return stroke;
}

/* TRUE while Sonic is near where he was STUCK_STEPS steps before, unless
 * his point has POINT_NEVER_STUCK */
static Bool race_sonic_stuck(struct task_work *sonic)
{
    struct race_aim *aim = &race_aim;
    NJS_POINT3 pos;

    if (aim->attr & POINT_NEVER_STUCK) {
        return FALSE;
    }
    player_get_history(1, STUCK_STEPS, &pos, NULL);
    vector_between(&pos, &sonic->pos, &scratch_vector);
    if (njScalor2(&scratch_vector) < NEAR) {
        return TRUE;
    }
    return FALSE;
}

/* the stroke the point's flags ask for, or race_sonic_pace */
static Float race_sonic_stroke(struct race_aim *aim)
{
    if (aim->attr & POINT_STROKE_FULL) {
        return 1.0F;
    } else if (aim->attr & POINT_STROKE_MOST) {
        return 0.8F;
    } else {
        return race_sonic_pace;
    }
}

/* when point + 1 reaches Sonic's line's last, or he heads for a
 * POINT_GOAL, return the last and change nothing. otherwise return
 * point + 1, the point he has reached. he is aimed from it at the point
 * after it, and work's submode, timer and steps go to 0. with
 * POINT_JUMP_OFF on point + 1, he presses jump. with POINT_LEAP_NEXT, he
 * leaps for the next point */
static Sint16 race_sonic_next(struct task_work *work, struct task_work *sonic,
                              struct race_aim *aim_arg, Sint16 point)
{
    Bool leap;
    Sint16 last;
    struct race_aim *aim;
    const struct race_point *next;

    point++;
    last = VARS(work)->last[1];
    /* @hack spelling: aim copied from aim_arg after last is read, which
     * moves it into R14 after that load (MOV R6,R14 at 0x8c085ae4) and
     * keeps last in R7; through the parameter, last takes R6 and 6 of
     * race_sonic_next's 97 words differ */
    aim = aim_arg;
    leap = FALSE;
    if (point >= last || POINT_KIND(aim->attr) == POINT_GOAL) {
        return last;
    }
    work->submode = 0;
    TASK_TIMER(work) = 0;
    VARS(work)->steps = 0;
    if (aim->attr & POINT_JUMP_OFF) {
        pad_records[1].press |= pad_jump_buttons;
    }
    if (aim->attr & POINT_LEAP_NEXT) {
        leap = TRUE;
    }
    next = &race_racer_line[point];
    aim->from = next->pos;
    next++;
    aim->to = next->pos;
    aim->attr = next->attr;
    vector_between(&sonic->pos, &aim->to, &aim->ahead);
    aim->distance = njScalor2(&aim->ahead);
    aim->speed = 0.0F;
    if (leap == TRUE) {
        race_sonic_leap(sonic, &aim->to, LEAP_STEPS);
        aim->attr = POINT_LEAP;
    }
    return point;
}

/* @hack spelling: race_between is vector_between with Float cursors, which
 * it walks across a vector's members as ISO C leaves undefined. SHC
 * expands it in the spark and the glare. it loads through the cursors with
 * post-increment (FMOV.S @R5+,FR2 at 0x8c085cae). through NJS_VECTOR
 * members, it reads y and z through R0, and 25 of the 179 words of
 * race_eggman_spark and race_eggman_glare differ */
#pragma inline(race_between)
static void race_between(Float *from, Float *to, Float *out)
{
    *out++ = *to++ - *from++;
    *out++ = *to++ - *from++;
    *out = *to - *from;
}

static void race_eggman_draw(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(&highway_eggmobile_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, work->ang.z);
    draw_tree(&highway_eggmobile_object);
    draw_tree(&highway_eggmobile_jets_object);
    njPopMatrix(1);
}

static void race_eggman_spark(struct task *task, struct race_spark *spark)
{
    struct task_work *work = task->work;
    NJS_POINT3 to;

    njPushMatrix(&_unit_matrix_);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, work->ang.z);
    njCalcPoint(NULL, &spark->from, &spark->params->pos);
    njCalcPoint(NULL, &spark->to, &to);
    njPopMatrix(1);
    race_between(&spark->params->pos.x, &to.x, &spark->params->speed.x);
    panlight_create(spark->params);
}

/* draw the glare of Eggman's jet */
static void race_eggman_glare(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    NJS_VECTOR dir;
    NJS_ARGB color;
    /* @hack spelling: size, a local set to 0.6F beside color.a, which
     * loads 0.6 once into FR5 for both (FMOV.S @R0,FR5 at 0x8c085d6e, FMOV.S
     * FR5,FR4 at 0x8c085d8a); passed as 0.6F, SHC loads it again, the
     * function runs 2 bytes longer and 19 of race_eggman_glare's 100 words
     * differ */
    Float size;

    njPushMatrix(&_unit_matrix_);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, work->ang.z);
    njCalcPoint(NULL, &race_glare_from, &pos);
    njCalcPoint(NULL, &race_glare_to, &dir);
    njPopMatrix(1);
    race_between(&pos.x, &dir.x, &dir.x);
    vector_normalize(&dir);
    size = 0.6F;
    color.a = 0.6F;
    color.r = 0.5F;
    color.g = 0.7F;
    color.b = 1.0F;
    glare_draw(&pos, &dir, size, &color);
}

/* return the last point Eggman has reached, and aim him at the next while
 * one is left. he reaches the point he heads for once he's past it. a
 * point near the one before counts as reached at once */
static Sint16 race_eggman_reach(struct task_work *work,
                                struct race_eggman *eggman)
{
    Sint16 point = VARS(work)->point[1];
    Float dot;
    const struct race_point *next;

    vector_between(&eggman->from, &eggman->to, &scratch_vector);
    vector_between(&work->pos, &eggman->to, &scratch_vector2);
    dot = njInnerProduct(&scratch_vector, &scratch_vector2);
    if (dot <= 0.0F || njScalor2(&scratch_vector) < NEAR) {
        if (++point < VARS(work)->last[1]) {
            next = &race_racer_line[point];
            vector_copy(&next->pos.x, &eggman->from.x);
            next++;
            vector_copy(&next->pos.x, &eggman->to.x);
        }
    }
    return point;
}

/* add race_eggman_turn_speed toward the point to Eggman's speed, cap it at
 * race_eggman_top_speed, and move him by it */
static void race_eggman_steer(struct task_work *work,
                              struct race_eggman *eggman)
{
    struct angles aim;

    vector_between(&work->pos, &eggman->to, &scratch_vector);
    vector_normalize(&scratch_vector);
    scratch_vector2.x = scratch_vector.x * race_eggman_turn_speed;
    scratch_vector2.y = scratch_vector.y * race_eggman_turn_speed;
    scratch_vector2.z = scratch_vector.z * race_eggman_turn_speed;
    vector_add(&eggman->speed, &scratch_vector2, &scratch_vector3);
    if (njScalor(&scratch_vector3) > race_eggman_top_speed) {
        vector_aim_y(&scratch_vector3, &aim.x, &aim.z);
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, aim.z);
        TURN(njRotateX, aim.x);
        scratch_vector2.x = 0.0F;
        scratch_vector2.y = race_eggman_top_speed;
        scratch_vector2.z = 0.0F;
        njCalcVector(NULL, &scratch_vector2, &scratch_vector3);
        njPopMatrix(1);
    }
    vector_copy(&scratch_vector3.x, &eggman->speed.x);
    vector_add(&work->pos, &scratch_vector3, &work->pos);
}

/* face the way he flies, and lean into the turn toward the point */
static void race_eggman_turn(struct task_work *work, struct race_eggman *eggman)
{
    Float dx;
    Float dz;
    Angle ang;

    work->ang.y =
        (Angle)(atan2f(eggman->speed.x, eggman->speed.z) * ANGLE_PER_RADIAN);
    dx = eggman->to.x - work->pos.x;
    dz = eggman->to.z - work->pos.z;
    ang = (Angle)(atan2f(dx, dz) * ANGLE_PER_RADIAN);
    ang = -ang + work->ang.y;
    work->ang.z = angle_approach(work->ang.z, ang, 0x40);
}

static void race_eggman_bob(struct task_work *work, struct race_eggman *eggman)
{
    Float bob = njSin(eggman->bob_phase) * 3.0F;

    work->pos.y -= eggman->bob;
    work->pos.y += bob;
    eggman->bob_phase += 0x400;
    eggman->bob = bob;
}

/* count down the voices' wait, and say what the change in Tails' lead
 * calls for. his lead is player_point less racer_point. his last lead is
 * work's point[0] less point[1], from the step before */
static void race_voice_step(struct task *task, Sint16 player_point,
                            Sint16 racer_point)
{
    struct task_work *work = task->work;
    struct race_voices *voices = VOICES(task);
    Sint16 player = VARS(work)->point[0];
    /* @hack spelling: the racer's last point read through a pointer to the
     * points, which the image stores to the frame and loads again (MOV.L
     * R4,@R15 at 0x8c0860ba, MOV.L @R15,R3 at 0x8c0860bc); read as
     * VARS(work)->point[1], 38 of race_voice_step's 116 words differ */
    Sint16 *points = VARS(work)->point;
    Sint16 racer = points[1];
    Sint16 last_lead = player - racer;
    Sint16 lead = player_point - racer_point;

    voices->wait--;
    switch (voices->mode) {
    case VOICES_WAIT:
        if (racer_point >= 1) {
            voices->mode = VOICES_RACE;
            race_say(voices, SAY_START);
        }
        break;
    case VOICES_RACE:
        if (lead != last_lead && race_outcome() == RACE_RUNNING && lead != 0) {
            if (lead > 0) {
                if (lead == 1 && last_lead > 1) {
                    race_say(voices, SAY_RACER_CLOSES);
                } else if (last_lead <= 0) {
                    race_say(voices, SAY_LEADING);
                }
            } else {
                if (lead == -1 && last_lead < -1) {
                    race_say(voices, SAY_TAILS_CLOSES);
                } else if (last_lead >= 0) {
                    race_say(voices, SAY_TRAILING);
                } else if (lead < -8) {
                    race_say(voices, SAY_FAR_BEHIND);
                }
            }
        }
        break;
    }
}

/* say the saying in the next of its two voices. nothing plays if it was
 * the last said, or within SAYING_GAP steps of the last */
static void race_say(struct race_voices *voices, enum race_saying say)
{
    int second = 0;
    const int *table;
    /* @hack spelling: the bit flipped through a pointer to turns, which the
     * image takes in R5 (ADD #8,R5 at 0x8c0861fa) and stores through (MOV.L
     * R2,@R5 at 0x8c086204) before it reads turns again; flipped in place,
     * the function runs 4 bytes shorter and 31 of race_say's 53 words
     * differ */
    Sint32 *turns;
    int bit;

    if (voices->last == say) {
        return;
    }
    if (voices->wait > 0) {
        return;
    }
    voices->wait = SAYING_GAP;
    if ((Uint8)(stage_get_stage_act() >> 8) != STAGE_HIGHWAY) {
        table = race_sonic_voices;
    } else {
        table = race_eggman_voices;
    }
    bit = 1 << say;
    turns = &voices->turns;
    *turns ^= bit;
    if (voices->turns & bit) {
        second = 1;
    }
    stream_request_voice(table[(say * 2) + second]);
    voices->last = say;
}

/* TRUE once Tails has won the stage's missions C and B, so he races for A */
static Bool race_check_mission_a(void)
{
    Bool result = FALSE;
    int stage = (Uint8)(stage_get_stage_act() >> 8);

    if (emblem_check_mission(saves, CHARACTER_TAILS, stage, MISSION_C) &&
        emblem_check_mission(saves, CHARACTER_TAILS, stage, MISSION_B)) {
        result = TRUE;
    }
    return result;
}

void race_texture_step(struct task *task)
{
    struct task_work *work = task->work;

    TEXTURE(work)->texlist = &milesrace_texlist;
    texture_load_pvm("MILESRACE", TEXTURE(work)->texlist);
    task->step = object_idle;
    task->destructor = race_texture_destroy;
}

void race_eggman_texture_step(struct task *task)
{
    struct task_work *work = task->work;

    TEXTURE(work)->texlist = &highway_eggmobile_texlist;
    texture_load_pvm("MRACE_EGGMOBLE", TEXTURE(work)->texlist);
    task->step = object_idle;
    task->destructor = race_texture_destroy;
}

static void race_texture_destroy(struct task *task)
{
    struct task_work *work = task->work;

    njReleaseTexture(TEXTURE(work)->texlist);
}

/* draw the bar along the bottom of the screen, with Tails' mark and the
 * racer's on it. each mark sits at its point's share of its line. Tails'
 * mark is on top */
static void race_draw_bar(Float player, Float player_last, Float racer,
                          Float racer_last)
{
    if (hud_hidden < 0 || frame_step == 0) {
        return;
    }
    player = player * (255.0F / player_last);
    racer = racer * (255.0F / racer_last);
    njSetTexture(&milesrace_texlist);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    race_sprite.tlist = &milesrace_texlist;
    race_sprite.tanim = race_sprites;
    race_sprite.sy = 1.0F;
    race_sprite.sx = 16.0F;
    race_sprite.p.x = 184.0F;
    race_sprite.p.y = 424.0F;
    njDrawSprite2D(&race_sprite, RACE_BAR, -2.004F, NJD_SPRITE_ALPHA);
    race_sprite.sx = 1.0F;
    race_sprite.p.x = racer + 184.0F;
    race_sprite.p.y = 416.0F;
    if ((Uint8)(stage_get_stage_act() >> 8) == STAGE_HIGHWAY) {
        njDrawSprite2D(&race_sprite, RACE_EGGMAN, -2.001F, NJD_SPRITE_ALPHA);
    } else {
        njDrawSprite2D(&race_sprite, RACE_SONIC, -2.001F, NJD_SPRITE_ALPHA);
    }
    /* @hack spelling: the 184 added to player in a statement of its own, ahead
     * of the store to p.x. it keeps player in FR15 and racer in FR14
     * (FMOV.S FR4,FR15 at 0x8c08630c, FMOV.S FR6,FR14 at 0x8c086312); added
     * in the store, the two trade and 12 of race_draw_bar's 111 words
     * differ */
    player += 184.0F;
    race_sprite.p.x = player;
    race_sprite.p.y = 416.0F;
    njDrawSprite2D(&race_sprite, RACE_TAILS, -2.0F, NJD_SPRITE_ALPHA);
}

void race_set_course(Sint32 course)
{
    race_course = course;
}

void race_set_won(void)
{
    race_result = RACE_WON;
}

void race_set_lost(void)
{
    race_result = RACE_LOST;
}

enum race_outcome race_outcome(void)
{
    return race_result;
}

/* the result banner's step. it shows YOU WIN if Tails won, else YOU LOSE.
 * it grows from a scale of 0.5 to 4, then shrinks to 2 and stays there */
void race_result_step(struct task *task)
{
    struct task_work *work = task->work;
    Float scale = BANNER(work)->scale;

    switch (work->mode) {
    case BANNER_INIT:
        work->mode = BANNER_GROW;
        if (race_result == RACE_WON) {
            BANNER(work)->sprite = RACE_YOU_WIN;
        } else {
            BANNER(work)->sprite = RACE_YOU_LOSE;
        }
        BANNER(work)->scale = 0.5F;
        task->draw = race_banner_draw;
        break;
    case BANNER_GROW:
        scale += 0.25F;
        if (scale >= 4.0F) {
            work->mode = BANNER_SHRINK;
        }
        BANNER(work)->scale = scale;
        break;
    case BANNER_SHRINK:
        scale -= 0.25F;
        if (scale <= 2.0F) {
            work->mode = BANNER_SHOW;
            scale = 2.0F;
        }
        BANNER(work)->scale = scale;
        break;
    case BANNER_SHOW:
        break;
    }
    object_step_draw(task);
}

static void race_banner_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float scale = BANNER(work)->scale;
    int sprite;
    int i;

    if (frame_step == 0) {
        return;
    }
    njSetTexture(&milesrace_texlist);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    race_sprite.tlist = &milesrace_texlist;
    race_sprite.tanim = race_sprites;
    race_sprite.sx = scale;
    race_sprite.sy = scale;
    race_sprite.p.x = SCREEN_MIDDLE_X;
    race_sprite.p.y = SCREEN_MIDDLE_Y;
    sprite = (int)BANNER(work)->sprite;
    for (i = 0; i < BANNER_SPRITES; i++) {
        njDrawSprite2D(&race_sprite, sprite++, -2.004F, NJD_SPRITE_ALPHA);
    }
}
