/* the cart, which the set editor names E CART. player 0 boards an empty
 * cart and drives it. a monkey drives each other cart along the act's
 * routes. outside STAGE_CART, a monkey's cart also chases player 0 once
 * player 0 comes near */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "camera/cart.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/helpers.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/explosion.h"
#include "effect/snow.h"
#include "hud/hud.h"
#include "object/helpers.h"
#include "object/placement.h"
#include "object/ring.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg03/cart.h"
#include "stg03/cart_debug.h"
#include "stg03/cart_pieces.h"
#include "stg03/cart_routes.h"
#include "stg03/shareobj_textures.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* the pieces a wrecked cart breaks into, each drawn with a model of
 * cart_piece_models */
#define CART_PIECES 10
/* the id cart_step gives a cart's work. cart_push pushes a task with this
 * id as a cart */
#define CART_ID 10
#define SE_EXPLOSION 25

/* a texid points at a material's attr_texId, whose low 14 bits hold the
 * texture's number. a draw that sets a cart's color keeps the 2 bits above
 * the number. it clears attr_texId's other bits, then adds the color */
#define TEXTURE_NUMBER 0x3fff
#define TEXTURE_KEEP 0xc000
/* a look's models come with these textures on the materials the draws set.
 * TEXTURE_COLOR is the color's, cart_colors' first. TEXTURE_TENS and
 * TEXTURE_ONES are the ring count's digits, showing 0 and 1 */
#define TEXTURE_COLOR 68
#define TEXTURE_TENS 95
#define TEXTURE_ONES 55
/* the colors a cart can take, and room for a look's color texids.
 * cart_find_texids doesn't check the room */
#define COLORS 7
#define COLOR_TEXIDS 10
/* the texture both digits show while the cart doesn't count rings */
#define TEXTURE_NO_DIGIT 89

/* pad 0's stick's x, shifted up a byte, as a Sint16 */
#define STICK_X() ((Sint16)(pad_records[0].x1 * 256))
/* a drift starts only with the cart faster than DRIFT_SPEED, and the stick
 * past DRIFT_STICK. it moves on from DRIFT_READY, and from DRIFT_TURN, once
 * drift_timer passes DRIFT_WAIT: after three steps. DRIFT_BOOST ends as the
 * stick eases inside DRIFT_EASE */
#define DRIFT_SPEED 4.0F
#define DRIFT_STICK 20000
#define DRIFT_WAIT 2
#define DRIFT_EASE 10000
/* the boost DRIFT_BOOST adds each step. the image holds it as 0x3c23d709
 * (0x0c90fd44), the float one below the speed limit's 0.01F (0x0c90fe44). as
 * 0.01F, 265 of 552 words and 30 pool words differ */
#define BOOST_GAIN 0.009999999F
/* the boost cart_steer takes away each step, down to 0 */
#define BOOST_FADE 0.009F

/* player 0 can board an empty cart within twice WAIT_RADIUS of it */
#define WAIT_RADIUS 20.0F
/* player 0's leap aboard takes BOARD_STEPS steps to reach the cart across x
 * and z. it rises for BOARD_PEAK steps, as BOARD_FALL a step slows it. past
 * the peak, it lands once lower than BOARD_HEIGHT above the cart */
#define BOARD_STEPS 30
#define BOARD_PEAK 15
#define BOARD_FALL 0.3F
#define BOARD_HEIGHT 5.0F
/* a leap that lands farther than BOARD_REACH along x or z misses the cart */
#define BOARD_REACH 4.0F
/* release_timer's start as player 0 leaves a cart */
#define RELEASE_STEPS 30

#define AIRBORNE_HEIGHT 3.0F
/* cart_move counts a bump when the cart moves farther than BUMP_MOVE but
 * less than BUMP_SHARE of its speed */
#define BUMP_MOVE 2.0F
#define BUMP_SHARE 0.9F
/* the speed a wreck's piece gains downward each step */
#define PIECE_GRAVITY 0.02F
/* a piece that lands slower than PIECE_STILL along each axis stops */
#define PIECE_STILL 0.5F
/* half of RAND_SCALE. a piece's random push upward reaches half of
 * explosion_speed. its push along x or z reaches as far either way */
#define RAND_HALF_SCALE (1.0F / 65536.0F)
/* a cart player 0 drives leans LIFT_TILT about z as cart_push sends it up
 * or down */
#define LIFT_TILT (-NJM_DEG_ANG(30))
/* in the air, the tilt.z of a cart player 0 drives grows AIR_TILT a step,
 * up to AIR_TILT_MOST. on the floor, the cart's pitch and roll turn
 * SLOPE_TURN a step toward the slope's. a drift leans tilt.x DRIFT_LEAN a
 * step, and keeps DRIFT_TILT_KEEP of tilt.z. with no drift, the tilt keeps
 * TILT_KEEP of each */
#define AIR_TILT NJM_DEG_ANG(0.5)
#define AIR_TILT_MOST NJM_DEG_ANG(45)
#define SLOPE_TURN NJM_DEG_ANG(0.5)
#define DRIFT_LEAN NJM_DEG_ANG(0.2)
#define DRIFT_TILT_KEEP 0.5F
#define TILT_KEEP 0.7F
/* cart_move keeps a move's x and z at least MOVE_SHARE of the speed's, or
 * CIRCUIT_MOVE_SHARE on STAGE_CART. it keeps the move's y at most
 * MOVE_Y_SHARE of the speed's */
#define MOVE_SHARE 0.98F
#define CIRCUIT_MOVE_SHARE 0.9F
#define MOVE_Y_SHARE 1.5F
/* the share of its offset a bob keeps each step the cart doesn't bob */
#define BOB_FADE 0.98F
/* each wheel drawn turns wheel_phase by half of an angle. the angle is
 * WHEEL_TURN degrees, plus WHEEL_TURN_PER_SPEED degrees for each unit of
 * speed */
#define WHEEL_TURN 3.0F
#define WHEEL_TURN_PER_SPEED 30.0F
/* a cart Big drives stops dead when faster than BIG_SPEED */
#define BIG_SPEED 30.0F
/* a cart smokes while its vitality is at most its motion's smoke_vitality. it
 * puffs whenever play time is a multiple of its vitality plus SMOKE_BASE */
#define SMOKE_BASE 8

/* a cart turns toward its route's point by ROUTE_TURN a step, and toward
 * player 0 by CHASE_TURN. it accelerates while it heads within AIMED of
 * either, and brakes otherwise */
#define ROUTE_TURN NJM_DEG_ANG(2)
#define CHASE_TURN NJM_DEG_ANG(1)
#define AIMED NJM_DEG_ANG(10)
/* on STAGE_CART, a cart faster than ROUTE_SPEED brakes */
#define ROUTE_SPEED 7.0F
/* a cart reaches its route's point within twice ROUTE_RADIUS */
#define ROUTE_RADIUS 20.0F
/* cart_find_route's points a call, and its spheres' radius */
#define SEARCH_POINTS 3
#define SEARCH_RADIUS 100.0F
/* a cart player 0 drives takes the rails no more than RAIL_DROP below their
 * first point */
#define RAIL_DROP 10.0F
/* on STAGE_CART, an empty cart counts its steps in start_steps. it rests at its
 * act's pose while the count is past POSE_AFTER and short of POSE_BEFORE.
 * player 0 boards it once the count reaches BOARD_AT */
#define POSE_AFTER 4
#define POSE_BEFORE 7
#define BOARD_AT 10

/* player 0's aim counts as ahead within RIDER_AHEAD of the cart's heading */
#define RIDER_AHEAD NJM_DEG_ANG(45)
/* the most rider_aim turns player 0 toward its target in a step */
#define AIM_TURN 0x1000
/* at least SPRAY_WAIT steps pass between two of a drift's sprays. as many
 * pass between two of a bump's puffs */
#define SPRAY_WAIT 4
/* near_radius_step holds the world's near radius at WIDE_RADIUS. it ends once
 * its count of steps passes WIDE_LAST, on its 12th step */
#define WIDE_RADIUS 2000.0F
#define WIDE_LAST 10
/* each afterimage starts AFTERIMAGE_GAP weaker than the last, and fades by
 * AFTERIMAGE_FADE a step */
#define AFTERIMAGE_GAP 0.01F
#define AFTERIMAGE_FADE 0.03F

/* Big's and E-102's models, and each look's wheels */
extern NJS_OBJECT cart_big_object;
extern NJS_OBJECT cart_e102_object;
extern NJS_OBJECT cart_wheel_1_object;
extern NJS_OBJECT cart_wheel_2_object;
extern NJS_OBJECT cart_big_wheel_1_object;
extern NJS_OBJECT cart_big_wheel_2_object;
extern NJS_OBJECT cart_e102_wheel_1_object;
extern NJS_OBJECT cart_e102_wheel_2_object;
/* the monkey a CART_MONKEY cart draws with */
extern NJS_OBJECT cart_monkey_object;

/* the model a cart draws as: cart_object, cart_big_object or
 * cart_e102_object */
enum cart_look { LOOK_PLAIN, LOOK_BIG, LOOK_E102, LOOKS };

/* the route point cart_find_route tests next: a route of the act's, and a
 * point of that route */
struct cart_search {
    Uint8 route;
    Uint8 point;
};

/* a cart's record, which cart_alloc takes from the heap and clears */
struct cart {
    int flags; /* CART_MONKEY and the rest below */
    /* cart_settings.ring_loss_time, set as player 0 boards. nothing reads
     * it */
    int ring_loss_timer;
    /* the radius player 0's first shape had before it boarded */
    Float rider_radius;
    Angle wheel_phase; /* how far the wheels have turned */
    Sint16 color; /* the texture its color texids take */
    /* the steps cart_take_hits passes over after player 0 is hurt aboard. in a
     * wreck, the timer is 0 until the pieces fly, and then 1. cart_wreck then
     * adds 1 for each step that all the pieces lie still */
    Sint16 damage_timer;
    Sint16 board_timer; /* steps into boarding */
    /* steps until cart_count_release registers the cart's collision again */
    Sint16 release_timer;
    Sint16 drift_state; /* its drift's state, of enum cart_drift */
    Sint16 drift_timer; /* steps in the drift's state */
    Angle drift_turn; /* what a drift takes from ang.y each step */
    /* max_vitality, or 0, which wrecks the cart. nothing here sets another
     * value */
    Sint8 vitality;
    /* a lean in x and z, which cart_draw turns by after ang */
    struct angles tilt;
    /* the rails' point the cart moves to next. it stays at their count after
     * the ride, so a cart rides them once */
    int rail_point;
    Float bob_offset; /* the bob's height above the hover height */
    Float bob_phase; /* the bob's phase, in degrees */
    int puff_wait; /* steps until a bump can puff dust again */
    int spray_wait; /* steps until a drift can spray snow again */
    Float ground_height; /* the floor's height under the cart */
    NJS_VECTOR speed;
    struct angles slope; /* the floor's slope under the cart */
    /* faster than this, the cart takes no drive from A or B. it rises from
     * min_speed toward max_speed by a hundredth of the gap for each ring, up
     * to 99 */
    Float speed_limit;
    /* this step's drive, turned by the cart's angles. cart_move adds its x
     * and z to the speed */
    NJS_VECTOR drive;
    /* gravity in y. x and z hold the slope's pull as the cart last found it
     * on the floor */
    NJS_VECTOR pull;
    Float boost; /* added to the acceleration A gives */
    int rest_timer; /* steps before a monkey's cart chases again */
    Uint8 route_point; /* the point of its route it heads for */
    Uint8 route; /* the route it follows plus 1, or 0 for none */
    struct cart_search search;
    /* its position as cart_follow_rider last saw it */
    NJS_POINT3 last_pos;
    Sint8 look; /* its look, of enum cart_look */
    Sint8 start_steps; /* the steps cart_circuit_start has counted */
    /* player 0's work flags as cart_wait_rider saw them a step ago */
    Sint16 player_flags;
    Uint8 piece_stopped[CART_PIECES];
    NJS_POINT3 piece_pos[CART_PIECES];
    NJS_VECTOR piece_speed[CART_PIECES];
    struct angles piece_ang[CART_PIECES];
    struct angles piece_spin[CART_PIECES]; /* added to piece_ang each step */
    NJS_VECTOR board_speed; /* player 0's speed each step of its leap aboard */
    struct angles board_spin; /* player 0's turn each step of the leap */
};

/* the record, in the first word of the task's extra */
#define CART(task) (*(struct cart **)(task)->extra)

/* a quarter, a half and a whole turn. a cart's model faces a quarter turn past
 * the cart's ang.y. player 0 aboard faces QUARTER_TURN less the cart's ang.y */
#define QUARTER_TURN 0x4000
#define HALF_TURN 0x8000
#define WHOLE_TURN 0x10000

/* in a cart's flags */
#define CART_MONKEY 0x1 /* a monkey drives it, and draws with it */
#define CART_DRIFTING 0x2 /* it drifts, from DRIFT_TURN on */
/* the sign of the stick's x as the drift readied */
#define CART_DRIFT_PLUS 0x4
#define CART_DRIFT_MINUS 0x8
/* it is higher than AIRBORNE_HEIGHT over its hover height */
#define CART_AIRBORNE 0x10
/* set whenever cart_count_release registers the collision. nothing reads it */
#define CART_REGISTERED 0x20
/* cart_move counted a bump, and cart_dust has yet to puff dust for it */
#define CART_BUMPED 0x40

/* in a cart's work flags, the bit that is PLAYER_ON_LAND in a player's.
 * cart_move clears it, and nothing here sets it */
#define CART_ON_LAND 0x1

/* a cart's mode, in its work's mode */
enum cart_mode {
    CART_INIT, /* the first step makes its record */
    CART_MONKEY_DRIVEN, /* a monkey drives it */
    CART_EMPTY, /* it waits for player 0 to board */
    CART_BOARDING, /* player 0 leaps aboard */
    CART_RIDDEN, /* player 0 drives it */
    /* it runs along the rails, with player 0 aboard */
    CART_RAILS,
    /* it drives as a monkey's cart does, with player 0 aboard */
    CART_AUTOPILOT,
    CART_WRECKED, /* its pieces fly and fall */
    /* cart_hold holds it, with player 0 aboard. it moves only by cart_place and
     * cart_push */
    CART_HELD,
    /* cart_release let it go. its collision never registers again */
    CART_RELEASED,
    /* nothing sets it. it drives as in CART_AUTOPILOT, but last_pos stays as
     * it was */
    CART_MODE_10,
    CART_END /* placement_kill ends the task */
};

/* the drift of a cart player 0 drives, in drift_state. a drift in any state
 * ends as the cart slows below DRIFT_SPEED */
enum cart_drift {
    DRIFT_NONE,
    /* A and L or R held start it, with the stick past DRIFT_STICK. the stick
     * must stay there */
    DRIFT_READY,
    DRIFT_TURN, /* the cart turns harder each step */
    /* it turns harder still and gains boost, until the stick eases */
    DRIFT_BOOST
};

/* the ridden cart's afterimages, in cart_afterimages. AFTERIMAGES_NONE holds
 * while none run. AFTERIMAGES_START holds from cart_afterimage_start until
 * afterimages_step creates the first, and AFTERIMAGES_ON after */
enum cart_afterimages { AFTERIMAGES_NONE, AFTERIMAGES_ON, AFTERIMAGES_START };

/* one of the cart's sounds: its number in Twinkle Park, and in other
 * stages */
struct cart_sound {
    int twinkle;
    int other;
};

/* the strength in a work's vars. afterimages_step's is the next afterimage's,
 * and an afterimage's is its own */
struct afterimage_vars {
    Float strength;
};

#define AFTERIMAGE(work) ((struct afterimage_vars *)&(work)->vars)

/* where cart_circuit_start puts an empty cart on STAGE_CART */
struct cart_pose {
    NJS_POINT3 pos;
    struct angles ang;
};

static void player_touch_step(struct task *task);
static void cart_free(struct task *task);
static void cart_draw(struct task *task);
static void cart_pieces_draw(struct task *task);
static struct cart *cart_alloc(void);
static void cart_find_texids(NJS_MODEL *model, int look);
static void cart_find_tree_texids(NJS_OBJECT *object, int look);
static void cart_find_all_texids(void);
static void cart_wreck(struct task *task, struct task_work *work);
static void cart_find_ground(struct task *task, struct task_work *work);
static void cart_steer(struct task *task, struct task_work *work);
static void cart_move(struct task *task, struct task_work *work);
static void cart_take_hits(struct task *task, struct task_work *work);
static void cart_fall(struct task *task, struct task_work *work);
static void cart_board(struct task *task, struct task_work *work);
static void cart_carry_rider(struct task *task, struct task_work *work);
static void cart_count_release(struct task *task, struct task_work *work);
static void cart_wait_rider(struct task *task, struct task_work *work);
static void cart_pilot(struct task *task, struct task_work *work);
static void cart_follow_route(struct task *task, struct task_work *work);
static void cart_chase(struct task *task, struct task_work *work);
static void cart_follow_rider(struct task *task, struct task_work *work);
static int rider_faces_ahead(struct task_work *work);
static void cart_ride_start(struct task *task);
static void cart_ride_end(struct task *task);
static void cart_find_route(struct task *task, int act);
static void cart_turn_vector(struct angles *ang, NJS_VECTOR *vector);
static void cart_slow(NJS_VECTOR *speed, Float amount);
static void cart_wheel_draw(NJS_OBJECT *object);
static void cart_bob(void);
static void cart_hover(struct task_work *work);
static void cart_ride_rails(struct task *task, struct task_work *work);
static void cart_face(struct task_work *work, NJS_VECTOR *way);
static void cart_enter_rails(struct task *task, struct task_work *work);
static void cart_sound_play(struct task *task, int row);
static void cart_sound_keep(struct task *task, int row);
static void cart_circuit_start(struct task *task);
static void cart_dust(struct task *task, struct task_work *work);
static void rider_aim(struct task_work *work, struct task_motion *motion,
                      struct player *record);
static void cart_start_autopilot(struct task *task, struct task_work *work);
static void near_radius_step(struct task *task);
static void cart_afterimage_start(Float strength);
static void afterimages_step(struct task *task);
static void afterimage_step(struct task *task);
static void afterimage_draw(struct task *task);

/* each character's cart's shape, by get_character's number. Big's and
 * E-102's are wider */
/* clang-format off */
static struct collision_shape cart_shapes[CHARACTER_COUNT] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS, PUSH_ALL,
     {0.0F, 0.0F, 0.0F}, 10.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS, PUSH_ALL,
     {0.0F, 0.0F, 0.0F}, 10.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS, PUSH_ALL,
     {0.0F, 0.0F, 0.0F}, 10.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS, PUSH_ALL,
     {0.0F, 0.0F, 0.0F}, 10.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS, PUSH_ALL,
     {0.0F, 0.0F, 0.0F}, 10.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS, PUSH_ALL,
     {0.0F, 0.0F, 0.0F}, 10.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS, PUSH_ALL,
     {0.0F, 0.0F, 0.0F}, 16.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS, PUSH_ALL,
     {0.0F, 0.0F, 0.0F}, 16.0F, 0.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */
/* the mark cart_texids_found holds once cart_find_all_texids has found every
 * look's texids. cart_texids_found is 0 until then */
#define TEXIDS_FOUND 0x309
static int cart_texids_found = 0;
/* each character's cart's color on STAGE_CART, by get_character's number,
 * as an index into cart_colors */
static Sint16 cart_character_colors[CHARACTER_COUNT] = {1, 0, 4, 6, 0, 5, 0, 2};
/* how far above its task's position each look's model draws */
static Float cart_heights[LOOKS] = {0.0F, 8.2F, 13.5F};
/* rows of cart_sounds: the engine's, and a bump's, of SOUND_ROWS */
#define SOUND_ENGINE 0
#define SOUND_BUMP 1
#define SOUND_ROWS 6
/* each row's sound in Twinkle Park, and elsewhere. only SOUND_ENGINE's and
 * SOUND_BUMP's rows play */
static struct cart_sound cart_sounds[SOUND_ROWS] = {
    {73, 665}, {74, 661}, {25, 25}, {7, 662}, {7, 663}, {7, 664},
};
/* the snow a drifting cart sprays on the floor. these are snow_create's
 * params, but at an alpha of 0.08 */
static struct dust_params cart_drift_snow = {
    1.0F,
    0.038F,
    0.19F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.08F, 1.0F, 1.0F, 1.0F},
};
/* cart_heights again, for afterimage_draw */
static Float afterimage_heights[LOOKS] = {0.0F, 8.2F, 13.5F};

/* 1 puts the cart player 0 drives on autopilot. cart_setup sets it to 0,
 * and nothing sets it to 1 */
static int cart_autopilot;
/* the record of the cart this file last stepped, drew, pushed or let go */
static struct cart *cart_current;
/* the task of the cart player 0 rides, or null */
static struct task *cart_ridden;
/* the character whose shape and motion the carts take: get_character's
 * result as the last cart started */
static Sint32 cart_character;
/* the afterimages' state, of enum cart_afterimages */
static int cart_afterimages;
/* each look's texids. every cart draws from the looks' shared models, so
 * each draw sets their textures first. a look's color texids take the
 * cart's color, and cart_color_texid_counts counts them. its digit texids
 * hold the tens, the ones and an unused third */
static int cart_color_texid_counts[LOOKS];
static Uint32 *cart_color_texids[LOOKS][COLOR_TEXIDS];
static Uint32 *cart_digit_texids[LOOKS][3];
/* 1 when player 0's collision had a hit at player_touch_step's last
 * step, else 0 */
static int player_touched;

/* each digit's texture, 0 to 9, then TEXTURE_NO_DIGIT */
static const Sint16 cart_digits[11] = {
    95, 55, 56, 57, 58, 59, 60, 61, 62, 63, TEXTURE_NO_DIGIT,
};
/* the colors a cart can take, as textures. a placement's scl.x picks one */
static const Sint16 cart_colors[COLORS] = {68, 70, 80, 85, 90, 92, 93};
/* cart_object's wheels, which cart_wheel_draw turns */
static const struct draw_override cart_wheels[3] = {
    {&cart_wheel_1_object, cart_wheel_draw},
    {&cart_wheel_2_object, cart_wheel_draw},
    {NULL, NULL},
};
/* @bug memory: no null row ends the table. for each node of Big's
 * or E-102's tree that has a model and that no row names, draw_object reads
 * on past the table's end (0x8c05a4ba, 0x8c05a4c8). the word there is
 * cart_smoke_speed's x, whose 0.0F ends the scan as a null object would */
#ifdef GCC
/* keeps the scan's end the tag names: GCC's table ends with a null row */
static const struct draw_override cart_large_wheels[5] = {
    {&cart_big_wheel_1_object, cart_wheel_draw},
    {&cart_big_wheel_2_object, cart_wheel_draw},
    {&cart_e102_wheel_1_object, cart_wheel_draw},
    {&cart_e102_wheel_2_object, cart_wheel_draw},
    {NULL, NULL},
};
#else
static const struct draw_override cart_large_wheels[4] = {
    {&cart_big_wheel_1_object, cart_wheel_draw},
    {&cart_big_wheel_2_object, cart_wheel_draw},
    {&cart_e102_wheel_1_object, cart_wheel_draw},
    {&cart_e102_wheel_2_object, cart_wheel_draw},
};
#endif
/* the speed of a damaged cart's smoke */
static const NJS_VECTOR cart_smoke_speed = {0.0F, 1.0F, 0.0F};
/* a zero vector, which cart_fall starts the slope's pull from */
static const NJS_VECTOR cart_zero = {0.0F, 0.0F, 0.0F};
/* the speed of a bump's dust */
static const NJS_VECTOR cart_puff_speed;

void cart_place(struct task *task, NJS_POINT3 *pos, struct angles *ang)
{
    struct task_work *work = task->work;

    if (work->mode == CART_HELD || work->mode == CART_RELEASED) {
        work->pos = *pos;
        cart_ridden->work->ang = *ang;
    }
}

struct task *cart_hold(void)
{
    struct task *task = cart_ridden;

    if (task) {
        task->work->mode = CART_HELD;
    }
    return task;
}

void cart_release(struct task *task)
{
    struct task_work *work = task->work;
    /* @bug memory: a cart that started while the set editor held it has no
     * collision record, yet player 0 can ride it. this then reads the
     * record's shapes through null (0x0c90ca5a), and writes through what it
     * reads (0x0c90ca66) */
    struct collision_shape *shape = work->collision->shapes;

    cart_current = CART(task);
    shape->flags &= ~SHAPE_OFF;
    player_ride_off(0, 0.0F, 2.0F, 2.0F);
    cart_current->release_timer = RELEASE_STEPS;
    cart_ride_end(task);
    work->mode = CART_RELEASED;
}

void cart_push(struct task *task, NJS_VECTOR *speed)
{
    struct task_work *work = task->work;

    if (task == player_get_task(0)) {
        if (cart_ridden) {
            struct task_work *cart_work = cart_ridden->work;

            cart_current = CART(cart_ridden);
            cart_face(cart_work, speed);
            cart_work->pos.x = cart_current->last_pos.x + speed->x;
            cart_work->pos.y = cart_current->last_pos.y + speed->y;
            cart_work->pos.z = cart_current->last_pos.z + speed->z;
            player_work[0]->pos = cart_work->pos;
            cart_afterimage_start(0.5F);
        } else {
            player_set_speed(0, speed->x, speed->y, speed->z);
        }
        vibration_start(0, 6, 30, 0);
    } else if (work->id == CART_ID) {
        /* @hack spelling: the cart's work read through task->work again, not
         * work, which keeps work in the frame for the id test (0x0c90cad4) and
         * loads task->work anew into R11 (0x0c90cbce); through work, 140 of 142
         * words differ */
        struct task_work *cart_work = task->work;

        cart_current = CART(task);
        cart_face(cart_work, speed);
        cart_work->pos.x = cart_current->last_pos.x + speed->x;
        cart_work->pos.y = cart_current->last_pos.y + speed->y;
        cart_work->pos.z = cart_current->last_pos.z + speed->z;
        cart_current->speed.x = speed->x;
        cart_current->speed.y = speed->y;
        cart_current->speed.z = speed->z;
    }
}

void cart_set_boost(Float boost)
{
    cart_current->boost = boost;
}

void cart_setup(void)
{
    int act;
    int i;

    task_create(TASK_WORK, 0, player_touch_step);
    player_touched = 0;
    cart_ridden = NULL;
    camera_cart_action = 0;
    if (!IN_STAGE(STAGE_TWINKLE)) {
        /* @hack spelling: the cast in a statement of its own, which keeps the
         * EXTU.W of the act's word ahead of its EXTU.B (0x0c90cc96,
         * 0x0c90cc9a); cast in one statement, the EXTU.W goes and 63 of 92
         * words differ */
        act = stage_get_stage_act();
        act = (Uint8)act;
        cart_routes[act][0] = cart_start_routes[act];
        for (i = 1; i < ROUTES; i++) {
            cart_routes[act][i] = cart_no_route;
        }
    }
    cart_autopilot = 0;
    task_create(TASK_WORK, 0, near_radius_step);
    if (IN_STAGE(STAGE_TWINKLE)) {
        cart_motions[CHARACTER_SONIC].hover_height = 5.0F;
    } else {
        cart_motions[CHARACTER_SONIC].hover_height = 9.0F;
    }
    cart_afterimages = AFTERIMAGES_NONE;
}

/* note in player_touched whether player 0's collision has a hit this step */
static void player_touch_step(struct task *task)
{
    if (player_work[0]->collision->flags & COLLISION_HIT) {
        player_touched = 1;
    } else {
        player_touched = 0;
    }
}

/* a placement's scl.x picks the cart's color from cart_colors, and its scl.z
 * the cart's look. scl.y 1 leaves the cart empty, and any other value gives it
 * a monkey. on STAGE_CART, every cart takes the color cart_character_colors
 * gives the character playing. when Big or E-102 plays, every cart there takes
 * that character's look */
void cart_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_far_from_camera(task)) {
        task->draw = NULL;
        return;
    }
    if (work->mode == CART_WRECKED) {
        task->draw = cart_pieces_draw;
    } else {
        task->draw = cart_draw;
    }
    cart_current = CART(task);
    /* while the set editor holds the cart, it only draws, and takes its
     * placement's values anew each step. a cart whose first step comes here
     * gets no collision record */
    if (placement_editing(task)) {
        if (work->mode == CART_INIT) {
            work->mode = CART_MONKEY_DRIVEN;
            CART(task) = cart_alloc();
            cart_current = CART(task);
            cart_current->flags |= CART_MONKEY;
            task->destructor = cart_free;
            task->draw = cart_draw;
            cart_find_all_texids();
            cart_debug_page = PAGE_START;
            cart_character = get_character();
        }
        cart_current = CART(task);
        if ((Sint16)work->scl.x >= COLORS) {
            work->scl.x = 0.0F;
        } else if ((Sint16)work->scl.x < 0) {
            work->scl.x = COLORS - 1;
        }
        cart_current->color = cart_colors[(Sint16)work->scl.x];
        if ((Sint16)work->scl.y == 1) {
            work->mode = CART_EMPTY;
            cart_current->flags &= ~CART_MONKEY;
        } else {
            work->mode = CART_MONKEY_DRIVEN;
            cart_current->flags |= CART_MONKEY;
        }
        cart_current->vitality = cart_motions[cart_character].max_vitality;
        cart_current->look = (Sint8)work->scl.z;
        if (cart_current->look < 0 || cart_current->look >= LOOKS) {
            cart_current->look = LOOK_PLAIN;
        }
        work->id = CART_ID;
        cart_draw(task);
        return;
    }
    switch (work->mode) {
    case CART_INIT:
        work->mode = CART_MONKEY_DRIVEN;
        /* @hack redundant: a jump to the next statement, which puts the task's
         * extra in R3 to be spilled (0x0c90cf96, 0x0c90cf9a); without it, R2
         * takes it and 2 of 636 words differ */
        goto allocate;
    allocate:
        CART(task) = cart_alloc();
        cart_current = CART(task);
        task->destructor = cart_free;
        cart_character = get_character();
        collision_init(task, &cart_shapes[cart_character], COLLISION_ENEMY, 1,
                       1);
        cart_find_all_texids();
        cart_current->color = cart_colors[(Sint16)work->scl.x];
        if ((Sint16)work->scl.y == 1) {
            work->mode = CART_EMPTY;
        }
        cart_current->vitality = cart_motions[cart_character].max_vitality;
        cart_current->look = (Sint8)work->scl.z;
        work->id = CART_ID;
        if (cart_current->look < 0 || cart_current->look >= LOOKS) {
            cart_current->look = LOOK_PLAIN;
        }
        if (IN_STAGE(STAGE_CART)) {
            if (cart_character == CHARACTER_BIG) {
                cart_current->look = LOOK_BIG;
            } else if (cart_character == CHARACTER_E102) {
                cart_current->look = LOOK_E102;
            }
            cart_current->color =
                cart_colors[cart_character_colors[cart_character]];
        }
        cart_current->last_pos = work->pos;
        cart_debug_page = PAGE_START;
        break;
    case CART_MONKEY_DRIVEN:
        cart_current->flags |= CART_MONKEY;
        cart_follow_rider(task, work);
        cart_find_ground(task, work);
        cart_draw(task);
        cart_fall(task, work);
        cart_pilot(task, work);
        cart_take_hits(task, work);
        cart_move(task, work);
        cart_sound_keep(task, SOUND_ENGINE);
        break;
    case CART_EMPTY:
        cart_circuit_start(task);
        cart_current->flags &= ~CART_MONKEY;
        cart_follow_rider(task, work);
        cart_wait_rider(task, work);
        cart_find_ground(task, work);
        cart_draw(task);
        cart_fall(task, work);
        cart_take_hits(task, work);
        cart_move(task, work);
        break;
    case CART_BOARDING:
        cart_current->flags &= ~CART_MONKEY;
        cart_follow_rider(task, work);
        cart_find_ground(task, work);
        cart_draw(task);
        cart_fall(task, work);
        cart_take_hits(task, work);
        cart_move(task, work);
        cart_board(task, work);
        break;
    case CART_RIDDEN:
        cart_current->flags &= ~CART_MONKEY;
        cart_follow_rider(task, work);
        cart_find_ground(task, work);
        cart_draw(task);
        cart_fall(task, work);
        cart_steer(task, work);
        cart_take_hits(task, work);
        cart_move(task, work);
        cart_carry_rider(task, work);
        cart_enter_rails(task, work);
        cart_start_autopilot(task, work);
        cart_sound_keep(task, SOUND_ENGINE);
        break;
    case CART_RAILS:
        cart_draw(task);
        cart_ride_rails(task, work);
        cart_carry_rider(task, work);
        cart_sound_keep(task, SOUND_ENGINE);
        break;
    case CART_AUTOPILOT:
        cart_current->flags &= ~CART_MONKEY;
        cart_follow_rider(task, work);
        cart_find_ground(task, work);
        cart_draw(task);
        cart_fall(task, work);
        cart_pilot(task, work);
        cart_take_hits(task, work);
        cart_move(task, work);
        cart_carry_rider(task, work);
        break;
    case CART_WRECKED:
        cart_current->flags &= ~CART_MONKEY;
        task->draw = cart_pieces_draw;
        cart_pieces_draw(task);
        cart_wreck(task, work);
        break;
    case CART_HELD:
        cart_current->flags &= ~CART_MONKEY;
        cart_find_ground(task, work);
        cart_draw(task);
        cart_take_hits(task, work);
        cart_carry_rider(task, work);
        break;
    case CART_RELEASED:
        cart_current->flags &= ~CART_MONKEY;
        cart_find_ground(task, work);
        cart_draw(task);
        cart_take_hits(task, work);
        break;
    case CART_MODE_10:
        cart_current->flags &= ~CART_MONKEY;
        cart_find_ground(task, work);
        cart_draw(task);
        cart_fall(task, work);
        cart_pilot(task, work);
        cart_take_hits(task, work);
        cart_move(task, work);
        cart_carry_rider(task, work);
        break;
    case CART_END:
    default:
        placement_kill(task);
        break;
    }
}

/* the task's destructor: free the cart's record, if it has one */
static void cart_free(struct task *task)
{
    if (CART(task)) {
        heap_free(CART(task));
        CART(task) = NULL;
    }
}

/* draw the cart in its look and color, with its monkey and its shadow, and puff
 * its smoke. it draws nothing in Twinkle Park's acts 1 and 2, or out of view.
 * its digits show player 0's rings, up to 99, while it is CART_RIDDEN or
 * CART_RAILS */
static void cart_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 shadow;
    Angle ang;
    int i;
    int rings;

    cart_current = CART(task);
    if (stage_get_stage_act() == STAGE_ACT(STAGE_TWINKLE, 1) ||
        stage_get_stage_act() == STAGE_ACT(STAGE_TWINKLE, 2)) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    for (i = 0; i < cart_color_texid_counts[cart_current->look]; i++) {
        *cart_color_texids[cart_current->look][i] =
            (*cart_color_texids[cart_current->look][i] & TEXTURE_KEEP) +
            cart_current->color;
    }
    /* @hack spelling: the mode read through task->work again, not work, which
     * loads task->work anew (0x0c90d502) while work stays in R13; through work,
     * 106 of 342 words differ */
    if (task->work->mode == CART_RIDDEN || task->work->mode == CART_RAILS) {
        rings = (Sint16)ring_count();
        if (rings < 0) {
            rings = 0;
        } else if (rings > 99) {
            rings = 99;
        }
        *cart_digit_texids[cart_current->look][0] = cart_digits[rings / 10];
        *cart_digit_texids[cart_current->look][1] = cart_digits[rings % 10];
    } else {
        *cart_digit_texids[cart_current->look][0] = TEXTURE_NO_DIGIT;
        *cart_digit_texids[cart_current->look][1] = TEXTURE_NO_DIGIT;
    }
    njSetTexture(&obj_shareobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njTranslate(NULL, 0.0F, cart_heights[cart_current->look], 0.0F);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y + QUARTER_TURN, ang);
    TURN_VIA(njRotateZ, cart_current->tilt.z, ang);
    TURN_VIA(njRotateX, cart_current->tilt.x, ang);
    /* @hack type: each call casts the table's address, and E-102's adds 2
     * rows to it. SHC then loads each from a pool word of its own (0x0c90d722,
     * 0x0c90d72a). passed as cart_large_wheels and &cart_large_wheels[2], the
     * table loads once into R12 and E-102's adds 16 to it, and 73 of 342
     * words and 15 pool words differ */
    switch (cart_current->look) {
    case LOOK_BIG:
        draw_object(&cart_big_object,
                    (struct draw_override *)&cart_large_wheels);
        break;
    case LOOK_E102:
        draw_object(&cart_e102_object,
                    (struct draw_override *)&cart_large_wheels + 2);
        break;
    default:
        draw_object(&cart_object, (struct draw_override *)cart_wheels);
        break;
    }
    if (cart_current->flags & CART_MONKEY) {
        draw_tree(&cart_monkey_object);
    }
    njPopMatrix(1);
    shadow.x = work->pos.x;
    shadow.y = cart_current->ground_height + 0.2F;
    shadow.z = work->pos.z;
    switch (cart_current->look) {
    case LOOK_BIG:
    case LOOK_E102:
        shadow_draw_at(&cart_current->slope, &shadow, 1.5F);
        break;
    default:
        shadow_draw_at(&cart_current->slope, &shadow, 1.2F);
        break;
    }
    if (cart_current->vitality <= cart_motions[cart_character].smoke_vitality) {
        if ((unsigned int)play_time_get() %
                (cart_current->vitality + SMOKE_BASE) ==
            0) {
            dust_create(&work->pos, (NJS_VECTOR *)&cart_smoke_speed, 1.0F);
        }
    }
}

/* draw the wrecked cart's pieces that still move. a piece vanishes once it
 * stops. every look breaks into cart_object's pieces: the loop reads
 * cart_piece_models from its start (pool word at 0x0c90daf8). Big's pieces
 * follow at 0x0c9f70d4, and E-102's at 0x0c9f7100 */
static void cart_pieces_draw(struct task *task)
{
    Angle ang;
    int i;
    int rings;

    cart_current = CART(task);
    for (i = 0; i < cart_color_texid_counts[cart_current->look]; i++) {
        *cart_color_texids[cart_current->look][i] =
            (*cart_color_texids[cart_current->look][i] & TEXTURE_KEEP) +
            cart_current->color;
    }
    if (task->work->mode == CART_RIDDEN || task->work->mode == CART_RAILS) {
        rings = (Sint16)ring_count();
        if (rings < 0) {
            rings = 0;
        } else if (rings > 99) {
            rings = 99;
        }
        *cart_digit_texids[cart_current->look][0] = cart_digits[rings / 10];
        *cart_digit_texids[cart_current->look][1] = cart_digits[rings % 10];
    } else {
        *cart_digit_texids[cart_current->look][0] = TEXTURE_NO_DIGIT;
        *cart_digit_texids[cart_current->look][1] = TEXTURE_NO_DIGIT;
    }
    njSetTexture(&obj_shareobj_texlist);
    njPushMatrix(NULL);
    for (i = 0; i < CART_PIECES; i++) {
        if (cart_current->piece_stopped[i] == 0) {
            njPushMatrix(NULL);
            njTranslateV(NULL, &cart_current->piece_pos[i]);
            TURN_VIA(njRotateZ, cart_current->piece_ang[i].z, ang);
            TURN_VIA(njRotateX, cart_current->piece_ang[i].x, ang);
            TURN_VIA(njRotateY, cart_current->piece_ang[i].y + QUARTER_TURN,
                     ang);
            draw_model(cart_piece_models[i]);
            njPopMatrix(1);
        }
    }
    njPopMatrix(1);
}

/* a cart's record from the heap, cleared byte by byte */
static struct cart *cart_alloc(void)
{
    struct cart *cart;
    char *bytes;
    /* @hack type: i, an int against sizeof's unsigned, which the NOLINT
     * below admits. the loop's cursor then starts as 0 plus the record, in R4
     * (0x0c90dd50, 0x0c90dd54); as a Uint32, R4 takes the record itself, the
     * function is 2 bytes shorter, and 17 of 25 words differ */
    int i;

    /* @hack redundant: the empty test, which writes the image's TST R0,R0
     * (0x0c90dd56); without it, 17 of 25 words differ */
    if (!(cart = heap_alloc(sizeof(struct cart)))) {
    }
    bytes = (char *)cart;
    /* NOLINTNEXTLINE(clang-diagnostic-sign-compare) */
    for (i = 0; i < sizeof(struct cart); i++) {
        bytes[i] = 0;
    }
    return cart;
}

/* add model's texids to those of look, of enum cart_look. a material with
 * TEXTURE_COLOR joins the color texids. one with TEXTURE_TENS or TEXTURE_ONES
 * becomes the tens' or the ones' texid */
static void cart_find_texids(NJS_MODEL *model, int look)
{
    int i;

    for (i = 0; i < model->nbMat; i++) {
        switch (model->mats[i].attr_texId & TEXTURE_NUMBER) {
        case TEXTURE_COLOR:
            cart_color_texids[look][cart_color_texid_counts[look]++] =
                &model->mats[i].attr_texId;
            break;
        case TEXTURE_TENS:
            cart_digit_texids[look][0] = &model->mats[i].attr_texId;
            break;
        case TEXTURE_ONES:
            cart_digit_texids[look][1] = &model->mats[i].attr_texId;
            break;
        }
    }
}

/* cart_find_texids for each model of object's tree */
static void cart_find_tree_texids(NJS_OBJECT *object, int look)
{
    if (object->model) {
        cart_find_texids(object->model, look);
    }
    if (object->child) {
        cart_find_tree_texids(object->child, look);
    }
    if (object->sibling) {
        cart_find_tree_texids(object->sibling, look);
    }
}

/* find every look's texids, once for all carts */
static void cart_find_all_texids(void)
{
    if (cart_texids_found == TEXIDS_FOUND) {
        return;
    }
    cart_color_texid_counts[LOOK_PLAIN] = 0;
    cart_find_tree_texids(&cart_object, LOOK_PLAIN);
    cart_color_texid_counts[LOOK_BIG] = 0;
    cart_find_tree_texids(&cart_big_object, LOOK_BIG);
    cart_color_texid_counts[LOOK_E102] = 0;
    cart_find_tree_texids(&cart_e102_object, LOOK_E102);
    cart_texids_found = TEXIDS_FOUND;
}

/* step a wrecked cart's pieces. the first step throws them from the cart,
 * each with a puff of dust. a piece takes half the cart's speed, and a
 * random push and spin from cart_settings. a moving piece falls, and puffs
 * dust where it lands. it stops if slower than PIECE_STILL along each axis,
 * and otherwise bounces with half its speed and spin. once all have
 * stopped, damage_timer counts the steps. at dead_wait_time the cart ends */
static void cart_wreck(struct task *task, struct task_work *work)
{
    int i;
    int stopped;
    Float ground;

    if (cart_current->damage_timer == 0) {
        NJS_VECTOR up;

        cart_current->damage_timer++;
        for (i = 0; i < CART_PIECES; i++) {
            up.x = 0.0F;
            up.y = 1.0F;
            up.z = 0.0F;
            dust_create(&work->pos, &up, 1.0F);
            cart_current->piece_pos[i].x = work->pos.x;
            cart_current->piece_pos[i].y = work->pos.y;
            cart_current->piece_pos[i].z = work->pos.z;
            cart_current->piece_speed[i].x =
                (cart_current->speed.x * 0.5F) +
                (rand() * cart_settings.explosion_speed * RAND_SCALE) -
                (cart_settings.explosion_speed * 0.5F);
            cart_current->piece_speed[i].y =
                (cart_current->speed.y * 0.5F) +
                (rand() * cart_settings.explosion_speed * RAND_HALF_SCALE);
            cart_current->piece_speed[i].z =
                (cart_current->speed.z * 0.5F) +
                (rand() * cart_settings.explosion_speed * RAND_SCALE) -
                (cart_settings.explosion_speed * 0.5F);
            cart_current->piece_ang[i].x = work->ang.x;
            cart_current->piece_ang[i].y = work->ang.y;
            cart_current->piece_ang[i].z = work->ang.z;
            /* a piece spins up to half explosion_spin degrees either way. the
             * half is an int's, which drops the half degree of an odd spin */
            /* NOLINTBEGIN(bugprone-integer-division) */
            cart_current->piece_spin[i].x =
                ((rand() * (Float)cart_settings.explosion_spin * RAND_SCALE) -
                 (Float)(cart_settings.explosion_spin / 2)) *
                ANGLE_PER_DEGREE;
            cart_current->piece_spin[i].y =
                ((rand() * (Float)cart_settings.explosion_spin * RAND_SCALE) -
                 (Float)(cart_settings.explosion_spin / 2)) *
                ANGLE_PER_DEGREE;
            cart_current->piece_spin[i].z =
                ((rand() * (Float)cart_settings.explosion_spin * RAND_SCALE) -
                 (Float)(cart_settings.explosion_spin / 2)) *
                ANGLE_PER_DEGREE;
            /* NOLINTEND(bugprone-integer-division) */
        }
    } else {
        stopped = 0;
        for (i = 0; i < CART_PIECES; i++) {
            if (cart_current->piece_stopped[i] == 0) {
                cart_current->piece_pos[i].x += cart_current->piece_speed[i].x;
                cart_current->piece_pos[i].y += cart_current->piece_speed[i].y;
                cart_current->piece_pos[i].z += cart_current->piece_speed[i].z;
                cart_current->piece_ang[i].x += cart_current->piece_spin[i].x;
                cart_current->piece_ang[i].y += cart_current->piece_spin[i].y;
                cart_current->piece_ang[i].z += cart_current->piece_spin[i].z;
                cart_current->piece_speed[i].y -= PIECE_GRAVITY;
                {
                    struct angles slope;

                    ground = ground_below(
                        cart_current->piece_pos[i].x,
                        cart_current->piece_pos[i].y +
                            cart_motions[cart_character].hover_height,
                        cart_current->piece_pos[i].z, &slope);
                }
                if (cart_current->piece_pos[i].y < ground) {
                    NJS_VECTOR up;

                    up.x = 0.0F;
                    up.y = 1.0F;
                    up.z = 0.0F;
                    dust_create(&cart_current->piece_pos[i], &up, 1.0F);
                    if (_builtin_fabsf(cart_current->piece_speed[i].x) <
                            PIECE_STILL &&
                        _builtin_fabsf(cart_current->piece_speed[i].y) <
                            PIECE_STILL &&
                        _builtin_fabsf(cart_current->piece_speed[i].z) <
                            PIECE_STILL) {
                        cart_current->piece_stopped[i] = 1;
                    } else {
                        cart_current->piece_speed[i].x *= 0.5F;
                        cart_current->piece_speed[i].y *= -0.5F;
                        cart_current->piece_speed[i].z *= 0.5F;
                        cart_current->piece_spin[i].x /= 2;
                        cart_current->piece_spin[i].y /= 2;
                        cart_current->piece_spin[i].z /= 2;
                    }
                    cart_current->piece_pos[i].y = ground;
                }
            } else {
                stopped++;
            }
        }
        if (stopped == CART_PIECES) {
            cart_current->damage_timer++;
            if (cart_current->damage_timer >= cart_settings.dead_wait_time) {
                work->mode = CART_END;
            }
        }
    }
}

/* find the floor's height and slope under the cart. the search starts at the
 * cart's hover height above it */
static void cart_find_ground(struct task *task, struct task_work *work)
{
    cart_current->ground_height = ground_below(
        work->pos.x, work->pos.y + cart_motions[cart_character].hover_height,
        work->pos.z, &cart_current->slope);
}

/* the turn the stick asks of the current cart. a STICK_X() of 32000 asks
 * for the motion's steering, in degrees. the turn scales by a half plus
 * speed.y over twice max_speed */
#define STICK_TURN()                                                           \
    ((((cart_current->speed.y /                                                \
        (cart_motions[cart_character].max_speed * 2.0F)) +                     \
       0.5F) *                                                                 \
      (Angle)(STICK_X() * cart_motions[cart_character].steering *              \
              (ANGLE_PER_DEGREE * (1.0F / 32000.0F)))))

/* drive the cart by pad 0. with no drift, A accelerates by the motion's
 * acceleration plus boost, and B brakes. the stick steers, twice as hard while
 * neither is held. faster than speed_limit, the cart takes no drive. boost then
 * fades by BOOST_FADE */
static void cart_steer(struct task *task, struct task_work *work)
{
    NJS_VECTOR drive;
    int rings;

    drive.x = 0.0F;
    drive.y = 0.0F;
    drive.z = 0.0F;
    switch (cart_current->drift_state) {
    case DRIFT_NONE:
        if (pad_records[0].on & PDD_DGT_TA) {
            drive.z =
                cart_motions[cart_character].acceleration + cart_current->boost;
            work->ang.y -= STICK_TURN();
        } else if (pad_records[0].on & PDD_DGT_TB) {
            drive.z = -cart_motions[cart_character].braking;
            work->ang.y -= STICK_TURN();
        } else {
            work->ang.y -= STICK_TURN() * 2.0F;
        }
        cart_current->flags &=
            ~(CART_DRIFTING | CART_DRIFT_PLUS | CART_DRIFT_MINUS);
        if (((pad_records[0].on & PDD_DGT_TL) ||
             (pad_records[0].on & PDD_DGT_TR)) &&
            (pad_records[0].on & PDD_DGT_TA) &&
            njScalor(&cart_current->speed) > DRIFT_SPEED &&
            _builtin_fabsf((Float)STICK_X()) > DRIFT_STICK) {
            if (STICK_X() > 0) {
                cart_current->flags |= CART_DRIFT_PLUS;
            } else {
                cart_current->flags |= CART_DRIFT_MINUS;
            }
            cart_current->drift_timer = 0;
            cart_current->drift_state = DRIFT_READY;
        }
        break;
    case DRIFT_READY:
        work->ang.y -= STICK_TURN();
        if (njScalor(&cart_current->speed) > DRIFT_SPEED) {
            if (cart_current->flags & CART_DRIFT_PLUS) {
                if (STICK_X() > DRIFT_STICK) {
                    cart_current->drift_timer++;
                } else {
                    cart_current->drift_timer = 0;
                }
            } else {
                if (STICK_X() < -DRIFT_STICK) {
                    cart_current->drift_timer++;
                } else {
                    cart_current->drift_timer = 0;
                }
            }
        } else {
            cart_current->drift_timer = 0;
        }
        if (cart_current->drift_timer == 0) {
            cart_current->drift_state = DRIFT_NONE;
        } else if (cart_current->drift_timer > DRIFT_WAIT) {
            cart_current->flags |= CART_DRIFTING;
            cart_current->drift_state = DRIFT_TURN;
            cart_current->drift_timer = 0;
            cart_current->drift_turn = STICK_TURN();
        }
        break;
    case DRIFT_TURN:
        work->ang.y -= cart_current->drift_turn;
        if (cart_current->flags & CART_DRIFT_PLUS) {
            cart_current->drift_turn++;
        } else {
            cart_current->drift_turn--;
        }
        if (njScalor(&cart_current->speed) < DRIFT_SPEED) {
            cart_current->drift_state = DRIFT_NONE;
        } else {
            cart_current->drift_timer++;
            if (cart_current->drift_timer > DRIFT_WAIT) {
                cart_current->drift_state = DRIFT_BOOST;
            }
        }
        break;
    case DRIFT_BOOST:
        cart_current->boost += BOOST_GAIN;
        work->ang.y -= cart_current->drift_turn;
        if (cart_current->flags & CART_DRIFT_PLUS) {
            cart_current->drift_turn += 3;
            if ((pad_records[0].x1 * 256) < DRIFT_EASE) {
                cart_current->drift_state = DRIFT_NONE;
            }
        } else {
            cart_current->drift_turn -= 3;
            if ((pad_records[0].x1 * 256) > -DRIFT_EASE) {
                cart_current->drift_state = DRIFT_NONE;
            }
        }
        if (njScalor(&cart_current->speed) < DRIFT_SPEED) {
            cart_current->drift_state = DRIFT_NONE;
        } else if (pad_records[0].on & PDD_DGT_TB) {
            drive.z = -cart_motions[cart_character].braking;
        }
        break;
    }
    rings = (Sint16)ring_count();
    if (rings > 99) {
        rings = 99;
    }
    cart_current->speed_limit = ((cart_motions[cart_character].max_speed -
                                  cart_motions[cart_character].min_speed) *
                                 rings * 0.01F) +
                                cart_motions[cart_character].min_speed;
    if (njScalor(&cart_current->speed) > cart_current->speed_limit) {
        drive.z = 0.0F;
    }
    cart_turn_vector(&work->ang, &drive);
    cart_current->drive = drive;
    cart_current->boost -= BOOST_FADE;
    if (cart_current->boost < 0.0F) {
        cart_current->boost = 0.0F;
    }
}

/* move the cart a step. its speed takes drive and pull, and loses friction and
 * the grip across its heading. on the floor, its pitch and roll turn toward the
 * slope's, and a drift leans it. in the air, a cart player 0 drives tilts
 * instead.
 *
 * player_collide and player_take_speed move a cart player 0 drives as they move
 * player 0. any other cart moves by its speed. it keeps its shape's radius from
 * the walls and floor near it. the move then keeps to the shares of the speed
 * that MOVE_SHARE, CIRCUIT_MOVE_SHARE and MOVE_Y_SHARE give. the speed becomes
 * the move, unless the move is faster than the speed */
static void cart_move(struct task *task, struct task_work *work)
{
    NJS_POINT3 previous;
    Float share;
    Angle ang;

    cart_current->speed.x += cart_current->drive.x + cart_current->pull.x;
    cart_current->speed.y += cart_current->pull.y;
    cart_current->speed.z += cart_current->drive.z + cart_current->pull.z;
    cart_slow(&cart_current->speed, cart_motions[cart_character].friction);
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, -work->ang.y, ang);
    njCalcVector(NULL, &cart_current->speed, &cart_current->speed);
    njPopMatrix(1);
    if (cart_current->speed.x > cart_motions[cart_character].grip) {
        cart_current->speed.x -= cart_motions[cart_character].grip;
    } else if (cart_current->speed.x < -cart_motions[cart_character].grip) {
        cart_current->speed.x += cart_motions[cart_character].grip;
    } else {
        cart_current->speed.x = 0.0F;
    }
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcVector(NULL, &cart_current->speed, &cart_current->speed);
    njPopMatrix(1);
    cart_dust(task, work);
    if (cart_current->flags & CART_AIRBORNE) {
        if (work->mode == CART_RIDDEN) {
            cart_current->tilt.z += AIR_TILT;
            if (cart_current->tilt.z > AIR_TILT_MOST) {
                cart_current->tilt.z = AIR_TILT_MOST;
            }
        }
    } else {
        /* @hack spelling: angle_approach called through the local approach,
         * whose address loads once into R14 (0x0c91001e) for both calls
         * (0x0c9100ca, 0x0c9100d8); called by name, the function is 32 bytes
         * shorter and 1113 of 1302 words and 133 pool words differ */
        Angle (*approach)(Angle, Angle, Angle) = angle_approach;

        if (cart_current->flags & CART_DRIFTING) {
            if (cart_current->flags & CART_DRIFT_PLUS) {
                cart_current->tilt.x -= DRIFT_LEAN;
            } else {
                cart_current->tilt.x += DRIFT_LEAN;
            }
            cart_current->tilt.z *= DRIFT_TILT_KEEP;
        } else {
            cart_current->tilt.x *= TILT_KEEP;
            cart_current->tilt.z *= TILT_KEEP;
        }
        work->ang.x = approach(work->ang.x, cart_current->slope.x, SLOPE_TURN);
        work->ang.z = approach(work->ang.z, cart_current->slope.z, SLOPE_TURN);
    }
    /* @hack redundant: discarded reads of work's position, which keep
     * cart_current's address in R11 and work in R12 (0x0c90fe78,
     * 0x0c90fe7e); without them, R12 and R10 take them and 96 of 1302 words
     * differ */
    (void)work->pos.x;
    (void)work->pos.y;
    (void)work->pos.z;
    previous = work->pos;
    if (work->mode == CART_RIDDEN) {
        struct task *player;
        struct task_motion *motion;

        /* @hack redundant: a jump to the next statement, which puts
         * player_get_task's address in R3 and the position's copy through R3
         * (0x0c9100f4 to 0x0c91010c); without it, R2 and R3 trade and 9 of 1302
         * words differ */
        goto carry;
    carry:
        player = player_get_task(0);
        motion = player->motion;
        /* @hack redundant: discarded reads of motion's speed, which keep
         * player_work's address in R13 and motion in R14 (0x0c9100fa,
         * 0x0c910102); without them, the two trade and 17 of 1302 words
         * differ */
        (void)motion->speed.x;
        (void)motion->speed.y;
        player_work[0]->pos = work->pos;
        player_work[0]->ang.x = work->ang.x;
        player_work[0]->ang.y = QUARTER_TURN - work->ang.y;
        player_work[0]->ang.z = work->ang.z;
        motion->speed = cart_current->speed;
        /* @hack spelling: the record read as player->motion->record, though
         * motion holds player->motion, for this call and the next. SHC then
         * keeps player in R10 (0x0c9100fc, 0x0c910102). read through motion,
         * R4 takes player and 50 of 1302 words differ */
        player_collide(player_work[0], motion, player->motion->record);
        player_take_speed(player_work[0], motion, player->motion->record);
        work->pos = player_work[0]->pos;
        motion->speed.x = 0.0F;
        motion->speed.y = 0.0F;
        motion->speed.z = 0.0F;
    } else {
        struct world_object *object;
        int remaining;
        int i;
        int replace;
        struct sides_query nearest;
        struct sides_query query;

        njAddVector(&work->pos, &cart_current->speed);
        world_list_near(work->pos.x, work->pos.y, work->pos.z,
                        cart_shapes[cart_character].radius);
        for (i = 0; i < SIDES; i++) {
            nearest.sides[i].found = 0;
        }
        query.pos = work->pos;
        remaining = world_count;
        object = world_objects;
        while (remaining > 0) {
            if (polygon_find_sides(&query, object->object, object->flags) ==
                1) {
                for (i = 0; i < SIDES; i++) {
                    if (query.sides[i].found == 1) {
                        if (!nearest.sides[i].found) {
                            nearest.sides[i] = query.sides[i];
                        } else {
                            replace = 0;
                            if (i & 1) {
                                if (query.sides[i].level <
                                    nearest.sides[i].level) {
                                    replace = 1;
                                }
                            } else {
                                if (query.sides[i].level >
                                    nearest.sides[i].level) {
                                    replace = 1;
                                }
                            }
                            if (replace == 1) {
                                nearest.sides[i] = query.sides[i];
                            }
                        }
                    }
                }
            }
            --remaining;
            ++object;
        }
        if (nearest.sides[SIDE_MINUS_X].found == 1 &&
            work->pos.x - nearest.sides[SIDE_MINUS_X].level <
                cart_shapes[cart_character].radius) {
            work->pos.x = nearest.sides[SIDE_MINUS_X].level +
                          cart_shapes[cart_character].radius;
        }
        if (nearest.sides[SIDE_PLUS_X].found == 1 &&
            nearest.sides[SIDE_PLUS_X].level - work->pos.x <
                cart_shapes[cart_character].radius) {
            work->pos.x = nearest.sides[SIDE_PLUS_X].level -
                          cart_shapes[cart_character].radius;
        }
        if (nearest.sides[SIDE_MINUS_Z].found == 1 &&
            work->pos.z - nearest.sides[SIDE_MINUS_Z].level <
                cart_shapes[cart_character].radius) {
            work->pos.z = nearest.sides[SIDE_MINUS_Z].level +
                          cart_shapes[cart_character].radius;
        }
        if (nearest.sides[SIDE_PLUS_Z].found == 1 &&
            nearest.sides[SIDE_PLUS_Z].level - work->pos.z <
                cart_shapes[cart_character].radius) {
            work->pos.z = nearest.sides[SIDE_PLUS_Z].level -
                          cart_shapes[cart_character].radius;
        }
        work->flags = work->flags & ~CART_ON_LAND;
        if (nearest.sides[SIDE_BELOW].found == 1 &&
            work->pos.y - nearest.sides[SIDE_BELOW].level <
                cart_shapes[cart_character].radius) {
            work->pos.y = nearest.sides[SIDE_BELOW].level +
                          cart_shapes[cart_character].radius;
        }
    }
    /* @hack redundant: a scope for moved, which gives the frame's locals the
     * image's slots, whose offsets the pools hold (0x0c90ff80, 0x0c910980 to
     * 0x0c91098a); without it, 0 of 1302 words differ, but 8 pool words do, and
     * so do 4 of the offsets it loads from the pool SHC puts in cart_take_hits
     * at 0x0c910a80 */
    {
        NJS_VECTOR moved;

        cart_hover(work);
        if (IN_STAGE(STAGE_CART)) {
            share = CIRCUIT_MOVE_SHARE;
        } else {
            share = MOVE_SHARE;
        }
        moved.x = work->pos.x - previous.x;
        moved.y = work->pos.y - previous.y;
        moved.z = work->pos.z - previous.z;
        if (njScalor(&moved) > BUMP_MOVE &&
            njScalor(&moved) < njScalor(&cart_current->speed) * BUMP_SHARE) {
            cart_sound_play(task, SOUND_BUMP);
            if (work->mode == CART_RIDDEN) {
                vibration_start(0, 6, 30, 0);
            }
            cart_current->flags |= CART_BUMPED;
        }
        if (_builtin_fabsf(moved.x) <
            _builtin_fabsf(cart_current->speed.x * share)) {
            moved.x = cart_current->speed.x * share;
        } else if (_builtin_fabsf(moved.x) >
                   _builtin_fabsf(cart_current->speed.x)) {
            moved.x = cart_current->speed.x;
        }
        if (_builtin_fabsf(moved.z) <
            _builtin_fabsf(cart_current->speed.z * share)) {
            moved.z = cart_current->speed.z * share;
        } else if (_builtin_fabsf(moved.z) >
                   _builtin_fabsf(cart_current->speed.z)) {
            moved.z = cart_current->speed.z;
        }
        if (_builtin_fabsf(moved.y) >
            _builtin_fabsf(cart_current->speed.y * MOVE_Y_SHARE)) {
            moved.y = cart_current->speed.y * MOVE_Y_SHARE;
        }
        if (njScalor(&moved) <= njScalor(&cart_current->speed)) {
            cart_current->speed = moved;
        }
    }
}

/* act on the cart's hits, by its mode. each mode sets the attack and guard
 * levels of the shape that meets the hits. that shape is the cart's own, or
 * player 0's first while player 0 boards or drives. while damage_timer runs,
 * this only counts it down.
 *
 * a monkey's cart rests for rest_time once it hits player 0. a monkey's cart
 * hurt while player 0 is in the air on foot sets off an explosion. player 0
 * then bounces up, and the cart waits empty. a monkey's cart hurt while
 * player 0 rides is wrecked.
 *
 * while player 0 drives, its first shape attacks only while player 0 faces
 * ahead. if player 0 is hurt while driving, the cart loses half its speed and
 * player 0 a ring. with no ring to lose, player 0 goes down and the cart is
 * wrecked */
static void cart_take_hits(struct task *task, struct task_work *work)
{
    /* @bug memory: a cart that started while the set editor held it has no
     * collision record. once the editor lets it go, this reads the record's
     * shapes through null (0x0c910a5c). it writes through what it reads for
     * a monkey's cart (0x0c910b82, 0x0c910b8a), an empty one (0x0c910ba6,
     * 0x0c910bb0) and one wrecked while player 0 drives it (0x0c910c9a). an
     * empty cart also writes the record's flags through null (0x0c910b9e) */
    struct collision_shape *shape = work->collision->shapes;

    if (cart_current->damage_timer > 0) {
        --cart_current->damage_timer;
        return;
    }
    switch (work->mode) {
    case CART_MONKEY_DRIVEN:
        if ((work->collision->flags & COLLISION_HIT) &&
            work->collision->hit == player_work[0]->collision) {
            cart_current->rest_timer = cart_settings.rest_time;
        }
        if (player_work[0]->submode != PLAYER_MODE_RIDE) {
            if ((work->flags & COLLISION_DAMAGED) &&
                !(player_work[0]->flags & PLAYER_ON_LAND)) {
                explosion_create(work, 1.0F);
                player_set_speed(0, 0.0F, 2.0F, 0.0F);
                vibration_start(0, 6, 30, 0);
                work->mode = CART_EMPTY;
            }
        } else if (work->flags & COLLISION_DAMAGED) {
            cart_current->vitality = 0;
            vibration_start(0, 6, 30, 0);
        }
        shape->levels = (shape->levels & ~SHAPE_ATTACK) | ATTACK(1);
        /* @hack spelling: the guard's clear written out, which reads the byte
         * by MOV #4,R0 and MOV.B @(R0,R13),R0 and keeps this case's tail of its
         * own (0x0c910b84 to 0x0c910b92); as &=, it reads by MOV.B @(4,R13),R0
         * and branches into CART_EMPTY's tail, the function ends 32 bytes
         * early, and 176 of 275 words and 46 pool words differ */
        shape->levels = shape->levels & ~SHAPE_GUARD;
        cart_count_release(task, work);
        break;
    case CART_EMPTY:
        work->collision->flags &= ~COLLISION_TARGET;
        /* @hack spelling: the attack's clear written out, which reads the byte
         * through R0 (0x0c910ba0, 0x0c910ba2); as &=, 134 of 275 words
         * differ */
        shape->levels = shape->levels & ~SHAPE_ATTACK;
        shape->levels = (shape->levels & ~SHAPE_GUARD) | GUARD(1);
        cart_count_release(task, work);
        break;
    case CART_BOARDING:
        /* @hack spelling: the attack's clear written out, which reads the byte
         * through R0 (0x0c910bbc, 0x0c910bc2); as &=, 121 of 275 words
         * differ */
        player_work[0]->collision->shapes->levels =
            player_work[0]->collision->shapes->levels & ~SHAPE_ATTACK;
        player_work[0]->collision->shapes->levels =
            (player_work[0]->collision->shapes->levels & ~SHAPE_GUARD) |
            GUARD(3);
        break;
    case CART_RIDDEN:
        if (player_work[0]->timer) {
            cart_current->speed.x *= 0.5F;
            cart_current->speed.y *= 0.5F;
            cart_current->speed.z *= 0.5F;
            if ((Sint16)ring_count() != 0) {
                ring_lose(0);
            } else {
                player_down(0);
                cart_current->vitality = 0;
            }
            cart_current->damage_timer = cart_settings.rest_time;
        } else if (rider_faces_ahead(work)) {
            player_work[0]->collision->shapes->levels =
                (player_work[0]->collision->shapes->levels & ~SHAPE_ATTACK) |
                ATTACK(2);
            player_work[0]->collision->shapes->levels =
                (player_work[0]->collision->shapes->levels & ~SHAPE_GUARD) |
                GUARD(1);
        } else {
            /* @hack spelling: both clears written out, which read the byte
             * through R0 (0x0c910c60, 0x0c910c6c); as &=, the attack's makes 23
             * of 275 words differ, and the guard's 17 */
            player_work[0]->collision->shapes->levels =
                player_work[0]->collision->shapes->levels & ~SHAPE_ATTACK;
            player_work[0]->collision->shapes->levels =
                player_work[0]->collision->shapes->levels & ~SHAPE_GUARD;
        }
        break;
    }
    if (cart_current->vitality <= 0) {
        cart_current->damage_timer = 0;
        if (work->mode == CART_RIDDEN) {
            work->mode = CART_WRECKED;
            shape->flags &= ~SHAPE_OFF;
            cart_ride_end(task);
        } else {
            work->mode = CART_WRECKED;
        }
        /* @hack redundant: a jump to the next statement, which puts
         * sound_play's address in R3 for the closing jump (0x0c910d04,
         * 0x0c910d1a); without it, R2 takes it and 2 of 275 words differ */
        goto wrecked;
    wrecked:
        sound_play(SE_EXPLOSION, 0, 0, 0);
    }
}

/* set pull.y to gravity. on the floor, pull's x and z take the slope's pull:
 * the motion's slide, turned by the slope. in the air, they keep the last pull,
 * which cart_move goes on adding to the speed. CART_AIRBORNE follows the cart's
 * height above the floor */
static void cart_fall(struct task *task, struct task_work *work)
{
    Float height;

    cart_current->pull.y = -cart_motions[cart_character].gravity;
    height = work->pos.y - cart_current->ground_height;
    if (!(height >
          cart_motions[cart_character].hover_height + AIRBORNE_HEIGHT)) {
        cart_current->flags &= ~CART_AIRBORNE;
    } else {
        cart_current->flags |= CART_AIRBORNE;
    }
    if (!(cart_current->flags & CART_AIRBORNE)) {
        NJS_VECTOR slide;

        slide = cart_zero;
        slide.y = cart_motions[cart_character].slide;
        cart_turn_vector(&cart_current->slope, &slide);
        cart_current->pull.x = slide.x;
        cart_current->pull.z = slide.z;
    }
}

/* step player 0's leap aboard. player 0 turns to the cart's angles as it
 * moves to the cart. landing within BOARD_REACH, it rides the cart.
 * otherwise it rides off, and the cart waits empty again */
static void cart_board(struct task *task, struct task_work *work)
{
    int distance;
    int state;

    if (!cart_current->board_timer) {
        if ((distance = angle_forward_distance(player_work[0]->ang.x,
                                               work->ang.x)) > HALF_TURN) {
            cart_current->board_spin.x =
                -((WHOLE_TURN - distance) / BOARD_STEPS);
        } else {
            cart_current->board_spin.x = distance / BOARD_STEPS;
        }
        if ((distance = angle_forward_distance(player_work[0]->ang.y,
                                               QUARTER_TURN - work->ang.y)) >
            HALF_TURN) {
            cart_current->board_spin.y =
                -((WHOLE_TURN - distance) / BOARD_STEPS);
        } else {
            cart_current->board_spin.y = distance / BOARD_STEPS;
        }
        if ((distance = angle_forward_distance(player_work[0]->ang.z,
                                               work->ang.z)) > HALF_TURN) {
            cart_current->board_spin.z =
                -((WHOLE_TURN - distance) / BOARD_STEPS);
        } else {
            cart_current->board_spin.z = distance / BOARD_STEPS;
        }
        cart_current->board_speed.x =
            (work->pos.x - player_work[0]->pos.x) * (1.0F / BOARD_STEPS);
        cart_current->board_speed.y = BOARD_FALL * BOARD_PEAK;
        cart_current->board_speed.z =
            (work->pos.z - player_work[0]->pos.z) * (1.0F / BOARD_STEPS);
    }
    if (cart_current->board_timer > BOARD_PEAK &&
        player_work[0]->pos.y < work->pos.y + BOARD_HEIGHT) {
        if (_builtin_fabsf(player_work[0]->pos.x - work->pos.x) > BOARD_REACH ||
            _builtin_fabsf(player_work[0]->pos.z - work->pos.z) > BOARD_REACH) {
            state = -1;
        } else {
            state = 1;
        }
    } else {
        state = 0;
    }
    switch (state) {
    case -1: {
        /* @bug memory: a cart that started while the set editor held it has
         * no collision record. a leap that misses it reads the record's
         * shapes through null (0x0c911008), and writes through what it reads
         * (0x0c911018) */
        struct collision_shape *shape = work->collision->shapes;

        cart_current->board_timer = 0;
        shape->flags &= ~SHAPE_OFF;
        player_ride_off(0, 0.0F, 0.0F, 0.0F);
        cart_current->release_timer = RELEASE_STEPS;
        work->mode = CART_EMPTY;
        break;
    }
    case 1:
        cart_current->board_timer = 0;
        player_work[0]->ang.x = work->ang.x;
        player_work[0]->ang.y = QUARTER_TURN - work->ang.y;
        player_work[0]->ang.z = work->ang.z;
        player_work[0]->pos.x = work->pos.x;
        player_work[0]->pos.y = work->pos.y + 1.0F;
        player_work[0]->pos.z = work->pos.z;
        work->mode = CART_RIDDEN;
        cart_current->ring_loss_timer = cart_settings.ring_loss_time;
        cart_ride_start(task);
        break;
    default:
        cart_current->board_timer++;
        player_work[0]->ang.x += cart_current->board_spin.x;
        player_work[0]->ang.y += cart_current->board_spin.y;
        player_work[0]->ang.z += cart_current->board_spin.z;
        player_work[0]->pos.x += cart_current->board_speed.x;
        player_work[0]->pos.y += cart_current->board_speed.y;
        player_work[0]->pos.z += cart_current->board_speed.z;
        /* @hack type: board_speed reached as an array of Floats, which
         * loads BOARD_FALL by MOVA (0x0c911128) before R0 takes the offset;
         * as board_speed.y, the load goes through FPUL, the function is 2
         * bytes longer and 21 of 307 words differ */
        VECTOR_FLOATS(&cart_current->board_speed)[1] -= BOARD_FALL;
    }
}

/* put player 0 in the cart, at its position and angles, and aim player 0
 * by rider_aim. camera_place follows the cart */
static void cart_carry_rider(struct task *task, struct task_work *work)
{
    struct task *player = player_get_task(0);
    struct task_motion *motion = player->motion;

    player_work[0]->pos = work->pos;
    player_work[0]->ang.x = work->ang.x;
    player_work[0]->ang.y = QUARTER_TURN - work->ang.y;
    player_work[0]->ang.z = work->ang.z;
    /* @hack spelling: the record read as player->motion->record, though motion
     * holds player->motion, which keeps player in R12 (0x0c91115e,
     * 0x0c911164); read through motion, R4 takes player and 37 of 67 words
     * differ */
    rider_aim(player_work[0], motion, player->motion->record);
    camera_place.pos = work->pos;
    camera_place.ang = work->ang;
    /* CAMERA_MODE_CART's view 1 rises while camera_place.target.x isn't 0.
     * target.x takes CART_AIRBORNE's bit as a Float, 16 in the air and 0 on the
     * floor. starting afterimages hold it at 0 until afterimages_step creates
     * the first */
    if (cart_afterimages == AFTERIMAGES_START) {
        camera_place.target.x = 0.0F;
    } else {
        camera_place.target.x = (Float)(cart_current->flags & CART_AIRBORNE);
    }
}

/* count release_timer down. once it has run out, register the cart's
 * collision for this step's checks, while the cart has vitality */
static void cart_count_release(struct task *task, struct task_work *work)
{
    cart_current->release_timer--;
    if (cart_current->release_timer <= 0 && cart_current->vitality > 0) {
        cart_current->release_timer = 0;
        cart_current->flags |= CART_REGISTERED;
        collision_register(work);
    }
}

/* start boarding as A is pressed with player 0 within twice WAIT_RADIUS of
 * the cart. player 0 must not ride, and must have been on land a step ago. a
 * waiting cart takes no drive */
static void cart_wait_rider(struct task *task, struct task_work *work)
{
    NJS_SPHERE player;
    NJS_SPHERE cart;

    if ((pad_records[0].press & PDD_DGT_TA) &&
        player_work[0]->submode != PLAYER_MODE_RIDE &&
        (cart_current->player_flags & PLAYER_ON_LAND)) {
        cart.c = work->pos;
        cart.r = WAIT_RADIUS;
        player.c = player_work[0]->pos;
        player.r = WAIT_RADIUS;
        if (njCollisionCheckSS(&player, &cart)) {
            work->mode = CART_BOARDING;
            cart_current->board_timer = 0;
            player_set_mode(0, PLAYER_MODE_RIDE);
        }
    }
    cart_current->drive.x = 0.0F;
    cart_current->drive.y = 0.0F;
    cart_current->drive.z = 0.0F;
    cart_current->player_flags = player_work[0]->flags;
}

/* drive a monkey's cart, or one on autopilot. on STAGE_CART,
 * cart_follow_route drives it, and elsewhere cart_chase */
static void cart_pilot(struct task *task, struct task_work *work)
{
    if (IN_STAGE(STAGE_CART)) {
        cart_follow_route(task, work);
    } else {
        cart_chase(task, work);
    }
}

/* drive along the cart's route, heading for its point by ROUTE_TURN a step.
 * as the cart reaches a point, it heads for the next, and from the last for
 * the first. it brakes when faster than ROUTE_SPEED. with no route, it looks
 * for one, driving at search_acceleration */
static void cart_follow_route(struct task *task, struct task_work *work)
{
    Float difference[3];
    /* @hack spelling: the squared distance worked through pointers into
     * difference, by way of a copied pointer to it, which forms its address
     * in R14 (0x0c9113be, 0x0c9113c0). as plain stores to difference, 71 of
     * 286 words and 1 pool word differ. without the copy, 61 of 286 words
     * differ */
    Float(*base)[3] = &difference;
    Float(*root)[3] = base;
    Float *dy;
    Float *dz;
    Float *dx = &(*root)[0];
    NJS_SPHERE point;
    NJS_SPHERE cart;
    NJS_VECTOR drive;
    int stage;
    int act;
    Angle ang;

    /* @hack redundant: a never-run reset of dx, which keeps the frame at 60
     * bytes (0x0c9113b6); without it, the frame takes 64 and 84 of 286 words
     * and 1 pool word differ */
    if (0) {
        dx = &(*base)[0];
    }
    /* @hack redundant: the six statements below leave player 0's squared
     * distance from the cart in difference[0]. nothing reads it (0x0c9113b8
     * to 0x0c911406). without them, the function runs 64 bytes short, and
     * 278 of 286 words differ */
    *dx = player_work[0]->pos.x - work->pos.x;
    dy = dx + 1;
    *dy = player_work[0]->pos.y - work->pos.y;
    dz = dx + 2;
    *dz = player_work[0]->pos.z - work->pos.z;
    *dx = (*dx * *dx) + (*dy * *dy) + (*dz * *dz);
    /* @hack spelling: the act taken from the word in a statement of its own,
     * which keeps the word's EXTU.W ahead of the act's EXTU.B (0x0c91140a,
     * 0x0c91140e); taken in one statement, the EXTU.W goes and 20 of 286 words
     * differ */
    stage = stage_get_stage_act();
    act = (Uint8)stage;
    if (!cart_current->route) {
        cart_find_route(task, act);
        drive.x = 0.0F;
        drive.y = 0.0F;
        drive.z = cart_settings.search_acceleration;
        /* @hack redundant: a never-run test of search_acceleration, which
         * loads the squares' terms into FR6 and FR4 (0x0c9113f0, 0x0c9113f2);
         * without it, 8 of 286 words differ */
        if (0) {
            if (cart_settings.search_acceleration) {
            }
        }
        /* @hack redundant: a never-run switch, which keeps the store ahead of
         * the BRA, and a NOP in its delay slot (0x0c91142e to 0x0c911432);
         * without it, the store fills the slot and 3 of 286 words differ */
        if (0) {
            switch (0) {
            default:
                break;
            }
        }
    } else {
        cart.c.x = work->pos.x;
        cart.c.y = work->pos.y;
        cart.c.z = work->pos.z;
        cart.r = ROUTE_RADIUS;
        point.c.x = cart_routes[act][cart_current->route - 1]
                        .points[cart_current->route_point]
                        .x;
        point.c.y = cart_routes[act][cart_current->route - 1]
                        .points[cart_current->route_point]
                        .y;
        point.c.z = cart_routes[act][cart_current->route - 1]
                        .points[cart_current->route_point]
                        .z;
        point.r = ROUTE_RADIUS;
        if (njCollisionCheckSS(&point, &cart)) {
            cart_current->route_point++;
            if (cart_current->route_point >=
                cart_routes[act][cart_current->route - 1].count) {
                cart_current->route_point = 0;
            }
        }
        ang = (Angle)(atan2f(point.c.x - work->pos.x, point.c.z - work->pos.z) *
                      ANGLE_PER_RADIAN);
        work->ang.y = angle_turn_toward(work->ang.y, ang, ROUTE_TURN);
        drive.x = 0.0F;
        drive.y = 0.0F;
        if (njScalor(&cart_current->speed) > ROUTE_SPEED) {
            drive.z = -cart_settings.enemy_braking;
        } else if (angle_shortest_distance(work->ang.y, ang) < AIMED) {
            drive.z = cart_settings.enemy_acceleration;
        } else {
            drive.z = -cart_settings.enemy_braking;
        }
    }
    cart_turn_vector(&work->ang, &drive);
    cart_current->drive = drive;
    --cart_current->rest_timer;
    if (cart_current->rest_timer < 0) {
        cart_current->rest_timer = 0;
    }
}

/* drive at player 0, turning CHASE_TURN a step, once rested and within
 * chase_distance. the cart leaves its route to chase. otherwise it drives as
 * cart_follow_route does, but at any speed */
static void cart_chase(struct task *task, struct task_work *work)
{
    /* @hack spelling: cart_settings through a local, which gives the squared
     * distance's y and z FR6 and FR4 (0x0c9116e8, 0x0c9116fc); read directly,
     * they trade and 5 of 377 words differ */
    struct cart_settings *settings;
    Float distance[3];
    /* @hack spelling: the squared distance worked through pointers into
     * distance, by way of a copied pointer to it, which forms its address in
     * R13 (0x0c9116b6, 0x0c9116ba). as plain stores to distance, 31 of 377
     * words differ. without the copy, 117 of 377 words differ */
    Float(*base)[3] = &distance;
    Float(*root)[3] = base;
    Float *dy;
    Float *dz;
    Float *dx = &(*root)[0];
    NJS_SPHERE point;
    NJS_SPHERE cart;
    int stage;
    int act;
    Angle ang;
    /* @hack type: player 0's work reached through player_work's address,
     * cast, which loads the x offset into FR5 (0x0c9116ea); as player_work
     * uncast, FR4 takes it and 5 of 377 words differ */
    struct task_work **players = (struct task_work **)&player_work;

    /* @hack redundant: never-run tests of root, which keep the frame at 60
     * bytes (0x0c9116ae); without them, the frame takes 64 and 146 of 377
     * words and 2 pool words differ */
    if (0) {
        if (root) {
        }
        if (root) {
        }
    }
    dy = dx + 1;
    dz = dx + 2;
    /* @hack redundant: never-run tests of players, which load the squares'
     * terms into FR6 and FR5 (0x0c9116e8, 0x0c9116ea); without them, 5 of
     * 377 words differ */
    if (0) {
        if (players) {
        }
        if (players) {
        }
    }
    *dx = (*players)->pos.x - work->pos.x;
    *dy = (*players)->pos.y - work->pos.y;
    *dz = (*players)->pos.z - work->pos.z;
    {
        /* @hack redundant: the x offset read into a local, which puts the y and
         * z offsets' addresses in R6 and R7 (0x0c9116c2, 0x0c9116ca);
         * without it, the two trade and 14 of 377 words differ */
        Float unused = *dx;

        /* @hack redundant: a never-run test of unused, which loads the
         * squares' terms into FR6 and FR5 (0x0c9116e8, 0x0c9116ea); without
         * it, 8 of 377 words differ */
        if (0) {
            if (unused) {
            }
        }
        *dx = (*dx * *dx) + (*dy * *dy) + (*dz * *dz);
    }
    settings = &cart_settings;
    /* chase_distance is a squared distance, as *dx is */
    if (!cart_current->rest_timer && *dx < cart_settings.chase_distance) {
        NJS_VECTOR drive;

        cart_current->route = 0;
        cart.c.x = work->pos.x;
        cart.c.y = work->pos.y;
        cart.c.z = work->pos.z;
        point.c.x = player_work[0]->pos.x;
        point.c.y = player_work[0]->pos.y;
        point.c.z = player_work[0]->pos.z;
        ang = (Angle)(atan2f(point.c.x - work->pos.x, point.c.z - work->pos.z) *
                      ANGLE_PER_RADIAN);
        work->ang.y = angle_turn_toward(work->ang.y, ang, CHASE_TURN);
        drive.x = 0.0F;
        drive.y = 0.0F;
        if (angle_shortest_distance(work->ang.y, ang) < AIMED) {
            drive.z = settings->enemy_acceleration;
        } else {
            drive.z = -settings->enemy_braking;
        }
        cart_turn_vector(&work->ang, &drive);
        cart_current->drive = drive;
        /* @hack redundant: a jump to the arm's end, which keeps its turn and
         * its copy of the drive (0x0c911806 to 0x0c911828) apart from the other
         * arm's; without it, SHC shares one, 310 of 377 words and 79 pool words
         * differ and the function ends 32 bytes early */
        goto chased;
    chased:;
    } else {
        NJS_VECTOR drive;

        /* @hack spelling: the act taken from the word in a statement of its
         * own, which gives dy and dz R6 and R7 (0x0c9116c2 to 0x0c9116ce);
         * taken in one statement, they trade and 26 of 377 words differ */
        stage = stage_get_stage_act();
        act = (Uint8)stage;
        if (!cart_current->route) {
            cart_find_route(task, act);
            drive.x = 0.0F;
            drive.y = 0.0F;
            drive.z = settings->search_acceleration;
            /* @hack redundant: a never-run switch, which keeps the store ahead
             * of the BRA, and a NOP in its delay slot (0x0c911850 to
             * 0x0c911854); without it, the store fills the slot and 3 of 377
             * words differ */
            if (0) {
                switch (0) {
                default:
                    break;
                }
            }
        } else {
            cart.c.x = work->pos.x;
            cart.c.y = work->pos.y;
            cart.c.z = work->pos.z;
            cart.r = ROUTE_RADIUS;
            point.c.x = cart_routes[act][cart_current->route - 1]
                            .points[cart_current->route_point]
                            .x;
            point.c.y = cart_routes[act][cart_current->route - 1]
                            .points[cart_current->route_point]
                            .y;
            point.c.z = cart_routes[act][cart_current->route - 1]
                            .points[cart_current->route_point]
                            .z;
            point.r = ROUTE_RADIUS;
            if (njCollisionCheckSS(&point, &cart)) {
                cart_current->route_point++;
                if (cart_current->route_point >=
                    cart_routes[act][cart_current->route - 1].count) {
                    cart_current->route_point = 0;
                }
            }
            ang = (Angle)(atan2f(point.c.x - work->pos.x,
                                 point.c.z - work->pos.z) *
                          ANGLE_PER_RADIAN);
            work->ang.y = angle_turn_toward(work->ang.y, ang, ROUTE_TURN);
            drive.x = 0.0F;
            drive.y = 0.0F;
            if (angle_shortest_distance(work->ang.y, ang) < AIMED) {
                drive.z = settings->enemy_acceleration;
            } else {
                drive.z = -settings->enemy_braking;
            }
        }
        cart_turn_vector(&work->ang, &drive);
        cart_current->drive = drive;
    }
    --cart_current->rest_timer;
    if (cart_current->rest_timer < 0) {
        cart_current->rest_timer = 0;
    }
}

/* move a cart player 0 drives to player 0. while player 0's collision has a
 * hit, the cart's speed becomes what it moved. a cart Big drives then stops
 * dead when faster than BIG_SPEED. every cart notes its position in
 * last_pos */
static void cart_follow_rider(struct task *task, struct task_work *work)
{
    if (work->mode == CART_RIDDEN) {
        work->pos = player_work[0]->pos;
        if (player_touched) {
            cart_current->speed.x = work->pos.x - cart_current->last_pos.x;
            cart_current->speed.y = work->pos.y - cart_current->last_pos.y;
            cart_current->speed.z = work->pos.z - cart_current->last_pos.z;
        }
        if (get_character() == CHARACTER_BIG) {
            if (njScalor(&cart_current->speed) > BIG_SPEED) {
                if (njScalor(&cart_current->speed) != 0.0F) {
                    /* njUnitVector makes the speed a unit vector, and the three
                     * stores below make it 0 */
                    njUnitVector(&cart_current->speed);
                    cart_current->speed.x = 0.0F;
                    cart_current->speed.y = 0.0F;
                    cart_current->speed.z = 0.0F;
                }
            }
        }
    }
    cart_current->last_pos = work->pos;
}

/* 1 when player 0's aim lies within RIDER_AHEAD of the cart's heading, else
 * 0 */
static int rider_faces_ahead(struct task_work *work)
{
    struct task *player = player_get_task(0);
    Angle aim = player->motion->aim.y;

    if (angle_shortest_distance(QUARTER_TURN - aim, work->ang.y) <
        RIDER_AHEAD) {
        return 1;
    }
    return 0;
}

/* make task the ridden cart. player 0's first shape takes the cart's shape's
 * radius, and keeps its own in rider_radius. the camera turns to
 * CAMERA_MODE_CART */
static void cart_ride_start(struct task *task)
{
    struct collision_shape *shape = player_work[0]->collision->shapes;

    cart_current->rider_radius = shape->radius;
    shape->radius = cart_shapes[cart_character].radius;
    cart_ridden = task;
    camera_override_mode(CAMERA_MODE_CART, CAMERA_ADJUST_TIME);
}

/* end the ride. player 0's first shape takes rider_radius back, and no cart
 * is ridden. in Twinkle Park, camera_restore turns the camera back */
static void cart_ride_end(struct task *task)
{
    struct collision_shape *shape = player_work[0]->collision->shapes;

    shape->radius = cart_current->rider_radius;
    cart_ridden = NULL;
    if (IN_STAGE(STAGE_TWINKLE)) {
        camera_restore();
    }
}

/* test SEARCH_POINTS more of the act's route points, from where the search
 * left off. the first within twice SEARCH_RADIUS of the cart becomes the
 * route and point it follows. the search runs through each route, and from
 * the last with points back to the first */
static void cart_find_route(struct task *task, int act)
{
    struct task_work *work = task->work;
    struct cart_route *routes;
    struct cart_search *search;
    int i;

    /* @hack spelling: i set ahead of routes, and not by the for, which copies
     * it into R11 and keeps work in R10 (0x0c911cc8, 0x0c911cce); set by
     * the for, 7 of 115 words differ */
    i = 0;
    routes = cart_routes[act];
    /* @hack redundant: a never-run store under an unsigned 0, which keeps
     * work in R10 and SEARCH_POINTS in R9 (0x0c911cce, 0x0c911cd0). without
     * it, 8 of 115 words differ. under a plain 0, 99 of 115 words and 3 pool
     * words differ */
    if (0U) {
        work->mode = CART_BOARDING;
    }
    for (; i < SEARCH_POINTS; ++i) {
        search = &cart_current->search;
        if (spheres_overlap(&routes[search->route].points[search->point],
                            &work->pos, SEARCH_RADIUS, SEARCH_RADIUS)) {
            cart_current->route = cart_current->search.route + 1;
            cart_current->route_point = cart_current->search.point;
            return;
        }
        cart_current->search.point++;
        search = &cart_current->search;
        if (search->point >= routes[search->route].count) {
            cart_current->search.point = 0;
            cart_current->search.route++;
            if (!routes[cart_current->search.route].points) {
                cart_current->search.route = 0;
            }
        }
    }
    /* @hack redundant: never-run tests of i, work and routes, which keep
     * the same registers (0x0c911cce, 0x0c911cd0); without them, 8 of 115
     * words differ */
    if (0) {
        /* NOLINTNEXTLINE(misc-redundant-expression) */
        if (i < SEARCH_POINTS && i && work && routes && work) {
        }
    }
}

/* turn vector by ang: by z, then x, then y. a zero vector stays as it is */
static void cart_turn_vector(struct angles *ang, NJS_VECTOR *vector)
{
    Angle turn;

    if (njScalor(vector) != 0.0F) {
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateZ, ang->z, turn);
        TURN_VIA(njRotateX, ang->x, turn);
        TURN_VIA(njRotateY, ang->y, turn);
        njCalcVector(NULL, vector, vector);
        njPopMatrix(1);
    }
}

/* take amount from speed's x toward 0, and from its z too unless the
 * current cart drifts */
static void cart_slow(NJS_VECTOR *speed, Float amount)
{
    if (speed->x > 0.0F) {
        speed->x -= amount;
        if (speed->x < 0.0F) {
            speed->x = 0.0F;
        }
    } else if (speed->x < 0.0F) {
        speed->x += amount;
        if (speed->x > 0.0F) {
            speed->x = 0.0F;
        }
    }
    if (!(cart_current->flags & CART_DRIFTING)) {
        if (speed->z > 0.0F) {
            speed->z -= amount;
            if (speed->z < 0.0F) {
                speed->z = 0.0F;
            }
        } else if (speed->z < 0.0F) {
            speed->z += amount;
            if (speed->z > 0.0F) {
                speed->z = 0.0F;
            }
        }
    }
}

/* draw a wheel of the current cart at its node's place.
 * cart_wheel_2_object turns about x by wheel_phase, and every other wheel by
 * -wheel_phase. each wheel drawn first turns wheel_phase, so a draw of the
 * cart turns it once for each wheel */
static void cart_wheel_draw(NJS_OBJECT *object)
{
    Angle ang;

    cart_current->wheel_phase +=
        (Angle)((WHEEL_TURN +
                 (njScalor(&cart_current->speed) * WHEEL_TURN_PER_SPEED)) *
                ANGLE_PER_DEGREE) /
        2;
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    TURN_VIA(njRotateZ, object->ang[2], ang);
    if (object == &cart_wheel_2_object) {
        TURN_VIA(njRotateX, cart_current->wheel_phase, ang);
    } else {
        TURN_VIA(njRotateX, -cart_current->wheel_phase, ang);
    }
    TURN_VIA(njRotateY, object->ang[1], ang);
    draw_model(object->model);
}

/* advance the bob a degree. bob_offset follows a sine about 0.5, from -0.5
 * to 1.5 */
static void cart_bob(void)
{
    cart_current->bob_phase += 1.0F;
    cart_current->bob_offset =
        0.5F +
        (1.0F * njSin((Angle)(cart_current->bob_phase * ANGLE_PER_DEGREE)));
}

/* a cart player 0 drives bobs while |speed.x| + |speed.z| is under 1.
 * otherwise, and for any other cart, the bob's phase goes back to 0. the bob's
 * offset then fades by BOB_FADE a step. every cart then stays at least its
 * hover height, plus the bob, above the floor */
static void cart_hover(struct task_work *work)
{
    struct angles slope;
    Float clearance;
    Float floor;
    Float minimum;

    if (work->mode == CART_RIDDEN) {
        if (_builtin_fabsf(cart_current->speed.x) +
                _builtin_fabsf(cart_current->speed.z) <
            1.0F) {
            cart_bob();
        } else {
            cart_current->bob_phase = 0.0F;
            /* @hack redundant: a never-run store of 0, which keeps the 0 in
             * FR4 and BOB_FADE in FR5 (0x0c911fea, 0x0c911fe2); without it,
             * the two trade and 6 of 82 words differ */
            if (0) {
                work->scl.y = 0.0F;
            }
            cart_current->bob_offset *= BOB_FADE;
        }
    } else {
        cart_current->bob_phase = 0.0F;
        cart_current->bob_offset *= BOB_FADE;
    }
    clearance =
        cart_motions[cart_character].hover_height + cart_current->bob_offset;
    /* @hack spelling: floor set inside the sum, which leaves its copy to FR5
     * in the branch's delay slot (0x0c912068); set before it, 7 of 82 words
     * differ */
    minimum = (floor = ground_below(work->pos.x, work->pos.y + clearance,
                                    work->pos.z, &slope)) +
              clearance;
    if (work->pos.y < minimum) {
        work->pos.y = minimum;
    }
    /* @hack redundant: never-run stores of floor and minimum, which keep
     * floor's copy in FR5 (0x0c912068); without them, 9 of 82 words
     * differ */
    if (0) {
        work->pos.y = floor + minimum;
        work->pos.x = minimum;
    }
}

/* ride the rails a step. the cart moves to the midpoint of the rails'
 * points at rail_point. it goes all the way across, and half way up or
 * down. it pitches the way it moved, and its heading, roll and lean go to 0.
 * past the rails' last point, player 0 drives it again, with its last move
 * as its speed */
static void cart_ride_rails(struct task *task, struct task_work *work)
{
    NJS_POINT3 previous;
    NJS_VECTOR moved;
    NJS_POINT3 points[3];
    struct cart_route *left;

    previous = work->pos;
    /* @hack spelling: left set inside the first point's read, which keeps the
     * frame at 64 bytes and R11 unsaved (0x0c9120c6, 0x0c9120c8); set before
     * it, R11 is saved, the frame takes 60 and 156 of 170 words and 2 pool
     * words differ */
    points[0] =
        (left = &cart_routes[0][RAILS])->points[cart_current->rail_point];
    points[1] = cart_routes[0][RAILS + 1].points[cart_current->rail_point];
    points[2].x = (points[0].x + points[1].x) * 0.5F;
    points[2].y = (points[0].y + points[1].y) * 0.5F;
    points[2].z = (points[0].z + points[1].z) * 0.5F;
    /* @hack redundant: the * 1.0F in x's and z's steps, which SHC writes as
     * an FMAC by 1.0F, as it writes y's half step by 0.5F (0x0c912162,
     * 0x0c912188). without it on x, 97 of 170 words differ; without it on
     * z, 98 of 170 words differ */
    work->pos.x = work->pos.x + ((points[2].x - work->pos.x) * 1.0F);
    work->pos.y = work->pos.y + ((points[2].y - work->pos.y) * 0.5F);
    work->pos.z = work->pos.z + ((points[2].z - work->pos.z) * 1.0F);
    moved.x = work->pos.x - previous.x;
    moved.y = work->pos.y - previous.y;
    moved.z = work->pos.z - previous.z;
    vector_aim_z(&moved, &work->ang.x, &work->ang.y);
    work->ang.y = 0;
    work->ang.z = 0;
    cart_current->tilt.x = 0;
    cart_current->tilt.z = 0;
    cart_current->rail_point++;
    if (cart_current->rail_point == left->count) {
        work->mode = CART_RIDDEN;
        cart_current->speed = moved;
        camera_override_mode(CAMERA_MODE_CART, CAMERA_ADJUST_TIME);
    }
}

/* turn the cart to head along way, with no pitch or roll. a cart player 0
 * drives leans LIFT_TILT while way.y isn't 0, and none otherwise */
static void cart_face(struct task_work *work, NJS_VECTOR *way)
{
    vector_aim_x(way, &work->ang.y, &work->ang.z);
    work->ang.x = 0;
    work->ang.y += QUARTER_TURN;
    work->ang.z = 0;
    cart_current->tilt.x = 0;
    if (work->mode == CART_RIDDEN) {
        if (way->y == 0.0F) {
            cart_current->tilt.z = 0;
        } else {
            cart_current->tilt.z = LIFT_TILT;
        }
    }
}

/* put a cart player 0 drives on the rails in Twinkle Park's act 0. it takes
 * them once it passes their first point along z. it must be no more than
 * RAIL_DROP below that point, and must not have ridden them yet */
static void cart_enter_rails(struct task *task, struct task_work *work)
{
    if (stage_get_stage_act() == STAGE_ACT(STAGE_TWINKLE, 0)) {
        if (cart_current->rail_point == 0 &&
            work->pos.z > cart_routes[0][RAILS].points->z &&
            work->pos.y > cart_routes[0][RAILS].points->y - RAIL_DROP) {
            work->mode = CART_RAILS;
            camera_restore();
        }
    }
}

/* play cart_sounds' row once. rows 0 to 2 play at the cart, and the rest
 * unplaced */
static void cart_sound_play(struct task *task, int row)
{
    struct task_work *work = task->work;

    switch (row) {
    case 0:
    case 1:
    case 2:
        if (IN_STAGE(STAGE_TWINKLE)) {
            sound_play_at(cart_sounds[row].twinkle, 0, 0, 0, work->pos.x,
                          work->pos.y, work->pos.z);
        } else {
            sound_play_at(cart_sounds[row].other, 0, 0, 0, work->pos.x,
                          work->pos.y, work->pos.z);
        }
        break;
    default:
        if (IN_STAGE(STAGE_TWINKLE)) {
            sound_play(cart_sounds[row].twinkle, 0, 0, 0);
        } else {
            sound_play(cart_sounds[row].other, 0, 0, 0);
        }
        break;
    }
}

/* keep cart_sounds' row playing at the cart, keyed by its task, for 3 more
 * updates */
static void cart_sound_keep(struct task *task, int row)
{
    struct task_work *work = task->work;

    if (IN_STAGE(STAGE_TWINKLE)) {
        sound_play_at_for(cart_sounds[row].twinkle, (int)task, 1, 0,
                          work->pos.x, work->pos.y, work->pos.z, 2);
    } else {
        sound_play_at_for(cart_sounds[row].other, (int)task, 1, 0, work->pos.x,
                          work->pos.y, work->pos.z, 2);
    }
}

/* count an empty cart's steps on STAGE_CART. the cart rests at its act's pose
 * for two of them, and player 0 then boards it */
static void cart_circuit_start(struct task *task)
{
    struct task_work *work = task->work;
    struct cart_pose start_poses[6] = {
#include "stg03/cart/start_poses.inc"
    };

    if (IN_STAGE(STAGE_CART)) {
        if (cart_current->start_steps > POSE_AFTER &&
            cart_current->start_steps < POSE_BEFORE) {
            work->pos = start_poses[(Uint8)stage_get_stage_act()].pos;
            work->ang = start_poses[(Uint8)stage_get_stage_act()].ang;
            cart_current->speed.x = 0.0F;
            cart_current->speed.y = 0.0F;
            cart_current->speed.z = 0.0F;
            cart_current->last_pos = work->pos;
        }
        if (cart_current->start_steps++ < BOARD_AT) {
            return;
        }
        work->mode = CART_BOARDING;
        player_set_mode(0, PLAYER_MODE_RIDE);
    }
}

/* a drifting cart sprays snow on the floor under it. a bump puffs dust
 * there */
static void cart_dust(struct task *task, struct task_work *work)
{
    NJS_POINT3 pos;

    pos.x = work->pos.x;
    pos.y = cart_current->ground_height;
    pos.z = work->pos.z;
    if ((cart_current->flags & CART_DRIFTING) &&
        cart_current->spray_wait == 0) {
        cart_drift_snow.pos = pos;
        snow_create_from(&cart_drift_snow);
        cart_current->spray_wait = SPRAY_WAIT;
    } else {
        --cart_current->spray_wait;
        if (cart_current->spray_wait < 0) {
            cart_current->spray_wait = 0;
        }
    }
    if (cart_current->puff_wait == 0) {
        if (cart_current->flags & CART_BUMPED) {
            dust_create(&pos, (NJS_VECTOR *)&cart_puff_speed, 1.0F);
            cart_current->flags &= ~CART_BUMPED;
            cart_current->puff_wait = SPRAY_WAIT;
        }
    } else {
        --cart_current->puff_wait;
    }
}

/* aim the player at the nearest target in its near_tasks list. turn it
 * toward that by AIM_TURN at most. with none listed, player_turn_to_stick
 * turns it */
static void rider_aim(struct task_work *work, struct task_motion *motion,
                      struct player *record)
{
    struct near_task *near;
    struct task_work *nearest;
    Float distance;
    struct collision_shape *own;
    struct collision_shape *other;
    NJS_VECTOR vector;
    Angle ang;

    if (PLAYER_VARS(work)->player == 0) {
        near = near_tasks;
    } else {
        near = near_tasks_player1;
    }
    if (near->work == NULL) {
        player_turn_to_stick(work, motion, record);
        return;
    }
    nearest = near->work;
    distance = near->distance;
    for (; near->work; ++near) {
        if (distance > near->distance) {
            distance = near->distance;
            nearest = near->work;
        }
    }
    own = work->collision->shapes;
    /* @bug logic: a shape's address is a multiple of 4, so this test of it
     * always holds (__modls at 0x0c9126c4, TST R0,R0 at 0x0c9126c8), and the
     * else's player_turn_to_stick never runs */
    if ((int)own % 4 == 0) {
        other = nearest->collision->shapes;
        vector.x = nearest->pos.x + other->center.x - own->center.x;
        vector.y = nearest->pos.y + other->center.y - own->center.y;
        vector.z = nearest->pos.z + other->center.z - own->center.z;
        ang = (Angle)(atan2f(vector.z, vector.x) * ANGLE_PER_RADIAN) & 0xffff;
        motion->aim.y = ang;
        work->ang.y = angle_approach(work->ang.y, ang, AIM_TURN);
    } else {
        player_turn_to_stick(work, motion, record);
    }
}

/* while cart_autopilot is 1, put the cart player 0 drives on autopilot. it
 * follows the act's first route from its second point */
static void cart_start_autopilot(struct task *task, struct task_work *work)
{
    if (cart_autopilot == 1) {
        work->mode = CART_AUTOPILOT;
        cart_current->route = 1;
        cart_current->route_point = 1;
    }
}

/* hold the world's near radius at WIDE_RADIUS, counting the steps in the work's
 * mode. the radius stays as it is once the task ends */
static void near_radius_step(struct task *task)
{
    world_set_near_radius(WIDE_RADIUS);
    if (task->work->mode++ > WIDE_LAST) {
        task_kill(task);
    }
}

/* start the ridden cart's afterimages, the first at strength, unless they
 * run */
static void cart_afterimage_start(Float strength)
{
    struct task *afterimages;

    if (cart_afterimages == AFTERIMAGES_NONE) {
        afterimages = task_create(TASK_WORK, 3, afterimages_step);
        AFTERIMAGE(afterimages->work)->strength = strength;
        cart_afterimages = AFTERIMAGES_START;
    }
}

/* each step player 0 rides a cart, create an afterimage of the cart at the
 * strength. then lower the strength by AFTERIMAGE_GAP. the afterimages end
 * at 0 */
static void afterimages_step(struct task *task)
{
    struct task *image;
    struct task_work *work;

    if (cart_ridden) {
        image = task_create(TASK_WORK, 3, afterimage_step);
        work = image->work;
        work->pos = cart_ridden->work->pos;
        work->ang = cart_ridden->work->ang;
        AFTERIMAGE(work)->strength = AFTERIMAGE(task->work)->strength;
        image->draw = afterimage_draw;
        AFTERIMAGE(task->work)->strength -= AFTERIMAGE_GAP;
        if (!(AFTERIMAGE(task->work)->strength > 0.0F)) {
            AFTERIMAGE(task->work)->strength = 0.0F;
            task_kill(task);
            cart_afterimages = AFTERIMAGES_NONE;
        } else {
            cart_afterimages = AFTERIMAGES_ON;
        }
    }
}

/* draw the afterimage, and fade it by AFTERIMAGE_FADE. it ends at 0 */
static void afterimage_step(struct task *task)
{
    struct task_work *work = task->work;

    afterimage_draw(task);
    AFTERIMAGE(work)->strength -= AFTERIMAGE_FADE;
    if (!(AFTERIMAGE(work)->strength > 0.0F)) {
        AFTERIMAGE(work)->strength = 0.0F;
        task_kill(task);
    }
}

/* draw the ridden cart's look, color and lean at the afterimage's place. the
 * afterimage adds its light onto what lies behind it. its constant material's
 * alpha is the strength less 1. its digits show player 0's rings. with no cart
 * ridden, it draws nothing */
static void afterimage_draw(struct task *task)
{
    struct task_work *work = task->work;
    int i;
    int rings;
    Angle ang;

    if (!cart_ridden) {
        return;
    }
    cart_current = CART(cart_ridden);
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    for (i = 0; i < cart_color_texid_counts[cart_current->look]; ++i) {
        Uint32 *field = cart_color_texids[cart_current->look][i];

        *field = (*field & TEXTURE_KEEP) + cart_current->color;
    }
    rings = (Sint16)ring_count();
    if (rings < 0) {
        rings = 0;
    } else if (rings > 99) {
        rings = 99;
    }
    *cart_digit_texids[cart_current->look][0] = cart_digits[rings / 10];
    *cart_digit_texids[cart_current->look][1] = cart_digits[rings % 10];
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    draw_set_material(AFTERIMAGE(work)->strength - 1.0F, 1.0F, 1.0F, 1.0F);
    njSetTexture(&obj_shareobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njTranslate(NULL, 0.0F, afterimage_heights[cart_current->look], 0.0F);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y + QUARTER_TURN, ang);
    TURN_VIA(njRotateZ, cart_current->tilt.z, ang);
    TURN_VIA(njRotateX, cart_current->tilt.x, ang);
    /* @hack type: each call casts the table's address, and E-102's adds 2
     * rows to it. SHC then loads each from a pool word of its own
     * (0x0c912ba2, 0x0c912baa), and leaves R11 unsaved (0x0c912946). passed
     * as cart_large_wheels and &cart_large_wheels[2], the table stays in
     * R11, which is saved, and 117 of 262 words and 13 pool words differ */
    switch (cart_current->look) {
    case LOOK_BIG:
        draw_object(&cart_big_object,
                    (struct draw_override *)&cart_large_wheels);
        break;
    case LOOK_E102:
        draw_object(&cart_e102_object,
                    (struct draw_override *)&cart_large_wheels + 2);
        break;
    default:
        draw_object(&cart_object, (struct draw_override *)cart_wheels);
        break;
    }
    njPopMatrix(1);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    draw_attr_restore();
}

/* @hack spelling: defined at the file's end, after cart_circuit_start, whose
 * poses' initializer SHC writes among the constants where that function
 * stands. the image holds cart_puff_speed after that initializer, at
 * 0x0c9f908c. defined at its declaration, it comes before the initializer: 2
 * of the code's pool words differ, and 61 of the constants' 136 words
 * differ */
static const NJS_VECTOR cart_puff_speed = {0.0F, 1.0F, 0.0F};
