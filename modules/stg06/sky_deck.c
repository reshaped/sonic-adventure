/* Sky Deck's stage and sky tasks with the deck's set pieces, grips, paths,
 * trails and flaps. the stage task starts each act. the sky task draws the
 * sky and the clouds, and moves the deck, the draft and the trails */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/mesh.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/light.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#ifdef GCC
#include "object/simple.h"
#endif
#include "object/switch.h"
#include "path/path.h"
#include "path/steps.h"
#include "player/player.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg06/collapse.h"
#include "stg06/floor.h"
#include "stg06/models.h"
#include "stg06/sky_deck.h"
#include "stg06/wind.h"
#include "system/bitmap.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 22
#include "stg06/sky_deck/text.inc"

extern NJS_OBJECT ana_a_object;
extern NJS_OBJECT ana_b_object;
extern NJS_OBJECT cannon_b_object;
extern NJS_OBJECT cannon_h_object;
extern NJS_OBJECT cannon_m_object;
extern NJS_OBJECT cannon_s1_object;
extern NJS_OBJECT cannon_s2_object;
extern NJS_OBJECT cannon_s_object;
extern NJS_OBJECT connect_0_object;
extern NJS_OBJECT connect_0_shadow_object;
extern NJS_OBJECT connect_0_world_object;
extern NJS_OBJECT container_d_object;
extern NJS_OBJECT cyl_0_object;
extern NJS_OBJECT cyl_1_object;
extern NJS_OBJECT cyl_2_object;
extern NJS_OBJECT dat_0ca1534c;
extern NJS_OBJECT dat_0ca17308;
extern NJS_OBJECT dat_0ca18980;
extern NJS_OBJECT dat_0ca1b7b0;
extern NJS_OBJECT dat_0ca20c34;
extern NJS_OBJECT dat_0ca21c28;
extern NJS_OBJECT dat_0ca222b8;
extern NJS_OBJECT dat_0ca22814;
extern NJS_OBJECT dat_0ca23fc8;
extern NJS_OBJECT dat_0ca24ecc;
extern NJS_OBJECT dat_0ca26aa8;
extern NJS_OBJECT dat_0ca29a54;
extern NJS_OBJECT dat_0ca2c098;
extern NJS_OBJECT dat_0ca2e41c;
extern NJS_OBJECT dat_0ca2ffc0;
extern NJS_OBJECT dat_0ca327fc;
extern NJS_OBJECT dat_0ca33270;
extern NJS_OBJECT dat_0ca39318;
extern NJS_OBJECT dat_0ca39ab0;
extern NJS_OBJECT dat_0ca4189c;
extern NJS_OBJECT dat_0ca437b4;
extern NJS_OBJECT dat_0ca44850;
extern NJS_OBJECT dat_0ca466bc;
extern NJS_OBJECT dat_0ca46f3c;
extern NJS_OBJECT dat_0ca47bcc;
extern NJS_OBJECT dat_0ca48758;
extern NJS_OBJECT dat_0ca492f0;
extern NJS_OBJECT dat_0ca4bca4;
extern NJS_OBJECT dat_0ca4dc34;
extern NJS_OBJECT dat_0ca4efe0;
extern NJS_OBJECT dat_0ca4fbc0;
extern NJS_OBJECT dat_0ca50500;
extern NJS_OBJECT dat_0ca51ad8;
extern NJS_OBJECT dat_0ca56494;
extern NJS_OBJECT dat_0ca57428;
extern NJS_OBJECT dat_0ca587f8;
extern NJS_OBJECT dat_0ca59290;
extern NJS_OBJECT dat_0ca5a820;
extern NJS_OBJECT dat_0ca5c48c;
extern NJS_OBJECT dat_0ca5fd24;
extern NJS_OBJECT dat_0ca6100c;
extern NJS_OBJECT dat_0ca622f4;
extern NJS_OBJECT dat_0ca6c750;
extern NJS_OBJECT dat_0ca73464;
extern NJS_OBJECT dat_0ca75890;
extern NJS_OBJECT dat_0ca75b3c;
extern NJS_OBJECT dat_0ca7667c;
extern NJS_OBJECT dat_0ca76f08;
extern NJS_OBJECT dat_0ca77154;
extern NJS_OBJECT dat_0ca773a4;
extern NJS_OBJECT dat_0ca775e4;
extern NJS_OBJECT dat_0ca777dc;
extern NJS_OBJECT dat_0ca779a4;
extern NJS_OBJECT dat_0ca77bf4;
extern NJS_OBJECT dat_0ca77d50;
extern NJS_OBJECT dat_0ca77f90;
extern NJS_OBJECT dat_0ca7810c;
extern NJS_OBJECT dat_0ca78210;
extern NJS_OBJECT dat_0ca784d0;
extern NJS_OBJECT dat_0ca7b25c;
extern NJS_OBJECT dat_0ca7b81c;
extern NJS_OBJECT dat_0ca7bcfc;
extern NJS_OBJECT dat_0ca7cd24;
extern NJS_OBJECT dat_0ca7d20c;
extern NJS_OBJECT dat_0ca7d638;
extern NJS_OBJECT edge_1_object;
extern NJS_OBJECT edge_1_world_object;
extern NJS_OBJECT hatch_lid_0_object;
extern NJS_OBJECT hatch_lid_1_object;
extern NJS_OBJECT hatch_object;
extern NJS_OBJECT hatch_selected_object;
extern NJS_OBJECT lever_object;
extern NJS_OBJECT rane_a_object;
extern NJS_OBJECT rane_b_object;
extern NJS_OBJECT rane_c_object;
extern NJS_OBJECT rane_d_object;
extern NJS_OBJECT rane_e_object;
extern NJS_OBJECT rane_f_object;
extern NJS_OBJECT rane_panel_object;
extern NJS_OBJECT sky_deck_container_object;
extern NJS_OBJECT spot_object;
extern NJS_OBJECT talap_0_object;
extern NJS_OBJECT talap_0_world_object;
extern NJS_OBJECT talap_1_object;
extern NJS_OBJECT talap_1_world_object;
extern NJS_OBJECT dat_0c9ff65c;
extern NJS_OBJECT dat_0ca00104;
extern NJS_OBJECT dat_0ca00a08;
extern NJS_OBJECT dat_0ca0198c;
extern NJS_OBJECT dat_0ca01a90;
extern NJS_OBJECT dat_0ca27350;
extern NJS_MOTION cannon_b_motion;
extern NJS_MOTION cannon_s_motion;
extern NJS_MOTION cannon_h_motion;
extern NJS_MOTION cannon_s1_motion;
extern NJS_MOTION cannon_s2_motion;
extern NJS_MOTION cannon_m_motion;
extern NJS_MOTION dat_0cabc940;
extern NJS_MOTION dat_0cabda7c;
extern NJS_MOTION dat_0cabe468;
extern NJS_MOTION dat_0cabe5dc;
extern NJS_MODEL scroll_00_model;
extern NJS_MODEL scroll_01_model;
extern NJS_MODEL scroll_02_model;
extern NJS_MODEL scroll_03_model;
extern NJS_MODEL scroll_04_model;
extern NJS_MODEL scroll_05_model;
extern NJS_MODEL scroll_06_model;
extern NJS_MODEL scroll_07_model;
extern NJS_MODEL scroll_08_model;
extern NJS_MODEL scroll_09_model;
extern NJS_MODEL scroll_10_model;
extern NJS_MODEL scroll_11_model;

/* the steps sky_deck_music_step counts down before the step on which it
 * asks for the act's music */
#define MUSIC_DELAY 10
/* each act's start plays this, held until something stops it */
#define SE_STAGE 960
/* the sounds as sky_deck_phase flips to sink the sky and to raise it. in
 * the second act, SE_SINK_2 or SE_RISE_2 plays as well while player 0 is
 * past 0 along z */
#define SE_SINK 167
#define SE_RISE 958
#define SE_SINK_2 190
#define SE_RISE_2 189
/* the sound as HANG_A's arm starts to swing down, and the voice with it:
 * Sonic's, or another character's */
#define SE_SWING 164
#define VOICE_SONIC_SWING 1402
#define VOICE_SWING 1360
/* the voice sky_deck_voice_change_act_step plays as it starts: Sonic's, or
 * another character's */
#define VOICE_SONIC_NEXT_ACT 1401
#define VOICE_NEXT_ACT 1359

/* the cylinder within which a player can take hold of a grip */
struct grip_cylinder {
    NJS_POINT3 center; /* its middle, from the task's pos */
    Float radius;
    Float height; /* how far it reaches above and below its middle */
};

/* sky_deck_phase in the first two acts, which the clock flips */
enum sky_phase { SKY_SINKS, SKY_RISES };

/* the sky rises to RISE_MOST and sinks to 0, RISE_SPEED a step. it hangs
 * LIFT_BASE above its rise, and bobs BOB either way of that */
#define RISE_MOST 700.0F
#define RISE_SPEED 1.2F
#define LIFT_BASE 120.0F
#define BOB 40.0F
/* how far the deck rolls either way as the sky rises and sinks. the angle
 * is about 2 degrees */
#define SWAY 384.0F

/* the turns' times count in units of TURN_UNIT steps. the clock goes back
 * to 0 past CLOCK_WRAP */
#define TURN_UNIT 60
#define CLOCK_WRAP 10500

/* sky_deck_phase in the third act, which player 0's place along x moves */
enum tilt_phase {
    TILT_LOW, /* pitched to PITCH_MIN */
    TILT_ROCK_HIGH, /* rocking toward PITCH_MAX */
    TILT_ROCK_LOW, /* rocking back toward PITCH_MIN */
    TILT_LEVEL,
    TILT_ROLLED /* rolled toward ROLL_MIN */
};

/* the deck's pitch and roll limits, about 20 and 89 degrees either way.
 * each _MIN is a negative angle, written unsigned as sky_deck_tilt leaves
 * it */
#define PITCH_MAX 0xe38
#define PITCH_MIN 0xf1c7
#define ROLL_MAX 0x3f49
#define ROLL_MIN 0xc0b6

/* the places along player 0's x that move the third act's tilt_phase. the
 * deck pitches low short of ROCK_FROM, and rocks past it. it levels past
 * LEVEL_FROM, and rocks again short of it. it rolls past ROLL_FROM. once
 * rolled, it levels again only short of ROCK_FROM */
#define ROCK_FROM (-500.0F)
#define LEVEL_FROM (-200.0F)
#define ROLL_FROM 225.0F
/* player 0 falls out of the third act's stage past FALL_FROM along x */
#define FALL_FROM (-1600.0F)
/* while switch 0 is on, player 0 turns it off past SWITCH_OFF_X along x and
 * below SWITCH_OFF_Y */
#define SWITCH_OFF_X 225.0F
#define SWITCH_OFF_Y (-25.0F)

/* the stage task's work, from its first step to sky_deck_end */
static struct task_work *sky_deck_work = NULL;
/* while this is set, the sky leaves draw_palettes as they are. nothing sets
 * it */
static int sky_deck_palettes_held = 0;
/* the angle whose sine rolls the deck and swings the flaps. it turns only
 * in the first two acts */
static Angle sky_deck_swing = 0;
/* the steps the deck has moved since the act started, which bob the sky */
static int sky_deck_bob = 0;
/* a sky_phase in the first two acts, and a tilt_phase in the third */
static int sky_deck_phase = 0;
/* sky_deck_reset zeroes this, and nothing reads it */
static Float dat_0cadbfdc = 0.0F;
/* how far the sky has risen over its phases, from 0 to RISE_MOST */
static Float sky_deck_rise = 0.0F;
/* TRUE once the third act has made player 0 fall out of the stage */
static Bool sky_deck_fell = FALSE;
/* TRUE from the step on which W_WALL's record touches another, until
 * draft_step clears it */
static Bool w_wall_hit = FALSE;
/* each act's sky origin, which sky_deck_sky_lift raises */
static NJS_POINT3 sky_deck_origins[3] = {
    {0.0F, 600.0F, 2500.0F},
    {0.0F, 1050.0F, -2500.0F},
    {0.0F, 750.0F, 0.0F},
};
/* the clock's times at which sky_deck_phase flips, in TURN_UNITs: the
 * second act's, then the first act's. each list ends at -1 */
static int sky_deck_turns_act1[] = {10, 25, 50, 70, 100, 115, 140, 160, -1};
static int sky_deck_turns_act0[] = {20, 30, 60, 80, 110, 120, 150, 170, -1};
struct sky_deck_model sky_deck_models[81] = {
    {&cannon_b_object, &cannon_b_object, NULL, {0, 0, 0, 0}},
    {&cannon_s_object, &cannon_s_object, NULL, {0, 0, 0, 0}},
    {&cannon_h_object, &cannon_h_object, NULL, {0, 0, 0, 0}},
    {&talap_0_object, &talap_0_world_object, &dat_0ca78210, {0, 0, 0, 0}},
    {&dat_0ca1534c, &dat_0ca7d638, &dat_0ca784d0, {0, 0, 0, 0}},
    {&dat_0ca17308, &dat_0ca7d20c, NULL, {0, 0, 0, 0}},
    {&dat_0ca18980, &dat_0ca7cd24, NULL, {0, 0, 0, 0}},
    {&connect_0_object,
     &connect_0_world_object,
     &connect_0_shadow_object,
     {0, 0, 0, 0}},
    {NULL, NULL, NULL, {0, 0, 0, 0}},
    {&dat_0ca1b7b0, &dat_0ca1b7b0, NULL, {0, 0, 0, 0}},
    {&talap_1_object, &talap_1_world_object, &dat_0ca7810c, {0, 0, 0, 0}},
    {&cyl_0_object, &cyl_0_object, &dat_0ca77f90, {0, 0, 0, 0}},
    {&edge_1_object, &edge_1_world_object, &dat_0ca77d50, {0, 0, 0, 0}},
    {&cyl_1_object, &cyl_1_object, &dat_0ca77bf4, {0, 0, 0, 0}},
    {&cyl_2_object, &cyl_2_object, &dat_0ca77bf4, {0, 0, 0, 0}},
    {&dat_0ca20c34, &dat_0ca20c34, NULL, {0, 0, 0, 0}},
    {&dat_0ca21c28, &dat_0ca21c28, NULL, {0, 0, 0, 0}},
    {&dat_0ca222b8, &dat_0ca222b8, &dat_0ca779a4, {0, 0, 0, 0}},
    {&dat_0ca22814, &dat_0ca22814, &dat_0ca77bf4, {0, 0, 0, 0}},
    {&dat_0ca23fc8, &dat_0ca23fc8, NULL, {0, 0, 0, 0}},
    {&dat_0ca24ecc, &dat_0ca24ecc, NULL, {0, 0, 0, 0}},
    {&dat_0ca26aa8, &dat_0ca26aa8, NULL, {0, 0, 0, 0}},
    {&dat_0ca29a54, &dat_0ca7b81c, &dat_0ca775e4, {0, 0, 0, 0}},
    {&dat_0ca2c098, &dat_0ca7b25c, &dat_0ca773a4, {0, 0, 0, 0}},
    {&rane_a_object, &rane_a_object, NULL, {0, 0, 0, 0}},
    {&rane_b_object, &rane_b_object, NULL, {0, 0, 0, 0}},
    {&rane_c_object, &rane_c_object, NULL, {0, 0, 0, 0}},
    {&rane_d_object, &rane_d_object, NULL, {0, 0, 0, 0}},
    {&rane_e_object, &rane_e_object, NULL, {0, 0, 0, 0}},
    {&rane_f_object, &rane_f_object, NULL, {0, 0, 0, 0}},
    {&dat_0ca39318, &dat_0ca39318, NULL, {0, 0, 0, 0}},
    {&rane_panel_object, &rane_panel_object, NULL, {0, 0, 0, 0}},
    {&dat_0ca39ab0, &dat_0ca39ab0, NULL, {0, 0, 0, 0}},
    {&ana_a_object, &ana_a_object, NULL, {0, 0, 0, 0}},
    {&ana_b_object, &ana_b_object, NULL, {0, 0, 0, 0}},
    {&cannon_s1_object, &cannon_s1_object, NULL, {0, 0, 0, 0}},
    {&cannon_s2_object, &cannon_s2_object, NULL, {0, 0, 0, 0}},
    {&cannon_m_object, &cannon_m_object, NULL, {0, 0, 0, 0}},
    {&dat_0ca4bca4, &dat_0ca4bca4, NULL, {0, 0, 0, 0}},
    {&dat_0ca4dc34, &dat_0ca4dc34, NULL, {0, 0, 0, 0}},
    {&dat_0ca4efe0, &dat_0ca4efe0, NULL, {0, 0, 0, 0}},
    {&dat_0ca4fbc0, &dat_0ca4fbc0, NULL, {0, 0, 0, 0}},
    {&dat_0ca50500, &dat_0ca50500, NULL, {0, 0, 0, 0}},
    {&sky_deck_container_object,
     &sky_deck_container_object,
     NULL,
     {0, 0, 0, 0}},
    {&dat_0ca51ad8, &dat_0ca51ad8, NULL, {0, 0, 0, 0}},
    {&dat_0ca44850, &dat_0ca44850, NULL, {0, 0, 0, 0}},
    {&dat_0ca4189c, &dat_0ca4189c, NULL, {0, 0, 0, 0}},
    {&dat_0ca437b4, &dat_0ca437b4, NULL, {0, 0, 0, 0}},
    {&dat_0ca2e41c, &dat_0ca7bcfc, &dat_0ca777dc, {0, 0, 0, 0}},
    {&lever_object, &lever_object, NULL, {0, 0, 0, 0}},
    {&dat_0ca466bc, &dat_0ca466bc, NULL, {0, 0, 0, 0}},
    {&dat_0ca56494, NULL, NULL, {0, 0, 0, 0}},
    {&dat_0ca57428, NULL, NULL, {0, 0, 0, 0}},
    {&dat_0ca587f8, NULL, NULL, {0, 0, 0, 0}},
    {&dat_0ca59290, NULL, NULL, {0, 0, 0, 0}},
    {&dat_0ca5a820, NULL, NULL, {0, 0, 0, 0}},
    {&dat_0ca5c48c, NULL, NULL, {0, 0, 0, 0}},
    {&container_d_object, &container_d_object, NULL, {0, 0, 0, 0}},
    {&dat_0ca6c750, NULL, NULL, {0, 0, 0, 0}},
    {&dat_0ca2ffc0, NULL, NULL, {0, 0, 0, 0}},
    {&hatch_lid_0_object, &hatch_lid_0_object, NULL, {0, 0, 0, 0}},
    {&hatch_lid_1_object, &hatch_lid_1_object, NULL, {0, 0, 0, 0}},
    {&hatch_selected_object, &hatch_selected_object, NULL, {0, 0, 0, 0}},
    {&hatch_selected_object, &hatch_selected_object, NULL, {0, 0, 0, 0}},
    {&hatch_object, &hatch_object, NULL, {0, 0, 0, 0}},
    {&dat_0ca5fd24, &dat_0ca5fd24, NULL, {0, 0, 0, 0}},
    {&dat_0ca6100c, &dat_0ca6100c, NULL, {0, 0, 0, 0}},
    {&dat_0ca622f4, &dat_0ca622f4, NULL, {0, 0, 0, 0}},
    {&spot_object, NULL, NULL, {0, 0, 0, 0}},
    {&dat_0ca327fc, &dat_0ca327fc, NULL, {0, 0, 0, 0}},
    {&dat_0ca33270, &dat_0ca33270, NULL, {0, 0, 0, 0}},
    {&dat_0ca47bcc, NULL, NULL, {0, 0, 0, 0}},
    {&dat_0ca48758, NULL, NULL, {0, 0, 0, 0}},
    {&dat_0ca492f0, &dat_0ca492f0, NULL, {0, 0, 0, 0}},
    {&dat_0ca46f3c, &dat_0ca46f3c, NULL, {0, 0, 0, 0}},
    {&dat_0ca73464, NULL, NULL, {0, 0, 0, 0}},
    {&dat_0ca75890, NULL, NULL, {0, 0, 0, 0}},
    {NULL, NULL, &dat_0ca75b3c, {0, 0, 0, 0}},
    {NULL, NULL, &dat_0ca7667c, {0, 0, 0, 0}},
    {NULL, NULL, &dat_0ca76f08, {0, 0, 0, 0}},
    {NULL, NULL, &dat_0ca77154, {0, 0, 0, 0}},
};
/* the motions a set piece can play, by its motion number, and how many
 * frames each has */
static NJS_MOTION *sky_deck_motions[10] = {
    &cannon_b_motion,  &cannon_s_motion, &cannon_h_motion, &cannon_s1_motion,
    &cannon_s2_motion, &cannon_m_motion, &dat_0cabc940,    &dat_0cabda7c,
    &dat_0cabe468,     &dat_0cabe5dc,
};
static Float sky_deck_motion_frames[10] = {80.0F, 120.0F, 150.0F, 60.0F, 60.0F,
                                           80.0F, 20.0F,  30.0F,  15.0F, 20.0F};
static struct collision_shape next_act_shape = {
    SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    20.0F,
    20.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};
static struct collision_shape grip_shape = {
    SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 50.0F, 0.0F},
    108.0F,
    50.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};
static struct grip_cylinder grip_cylinder = {
    {0.0F, 50.0F, 0.0F}, 108.0F, 50.0F};
static struct collision_shape hangpole_shape = {
    SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    48.0F,
    50.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};
static struct grip_cylinder hangpole_cylinder = {
    {0.0F, 0.0F, 0.0F}, 48.0F, 50.0F};
static struct collision_shape cyl_s_shape = {
    SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 70.0F, 0.0F},
    38.0F,
    65.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};
static struct grip_cylinder cyl_s_cylinder = {
    {0.0F, 70.0F, 0.0F}, 38.0F, 65.0F};
static struct collision_shape tank_h_shape = {
    SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 30.0F, 0.0F},
    32.0F,
    25.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};
static struct grip_cylinder tank_h_cylinder = {
    {0.0F, 30.0F, 0.0F}, 32.0F, 25.0F};
static struct collision_shape top_0_shape = {
    SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    108.0F,
    110.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};
static struct grip_cylinder top_0_cylinder = {
    {0.0F, 0.0F, 0.0F}, 108.0F, 110.0F};
static struct collision_shape duct_shape = {
    SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
    0,
    PUSH_FIRM(PUSH_ALL),
    {5.0F, 4.0F, 0.0F},
    10.0F,
    5.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};
static struct collision_shape fence_l_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    117.0F,
    8.0F,
    3.0F,
    {0, 0, 0},
    0,
    NULL,
};
static struct collision_shape fence_m_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    87.0F,
    8.0F,
    3.0F,
    {0, 0, 0},
    0,
    NULL,
};
static struct collision_shape fence_s_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    50.0F,
    8.0F,
    3.0F,
    {0, 0, 0},
    0,
    NULL,
};
/* TRUE from the step on which Sonic starts HANG_A's arm swinging down. each
 * start of HANG_A or HANG_C outside the set editor clears it, even while the
 * arm is down. while it is set, HANG_A's path has SWUNG_POINTS, and only
 * then does HANG_C's path take players */
static Bool hang_a_down = FALSE;
static NJS_MODEL *scroll_models[] = {
    &scroll_00_model, &scroll_01_model, &scroll_02_model, &scroll_03_model,
    &scroll_04_model, &scroll_05_model, &scroll_06_model, &scroll_07_model,
    &scroll_08_model, &scroll_09_model, &scroll_10_model, &scroll_11_model,
};
static struct collision_shape w_wall_shape = {
    SHAPE_WALL,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    40.0F,
    30.0F,
    10.0F,
    {0, 0, 0},
    0,
    NULL,
};
/* how far the draft has built, from 0 to RAMP_FULL. the lift grows with it */
static int draft_ramp = 0;
static struct collision_shape homing_shape = {
    SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    5.0F,
    5.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};

Sint8 sky_deck_started_act;
Sint8 sky_deck_act;
struct trail_point trail_points[TRAILS][TRAIL_POINTS];
Sint32 trail_life[TRAILS];
struct trail_volume trail_volumes[TRAIL_VOLUMES];
NJS_FOG_TABLE sky_deck_fog_table;
Uint32 sky_deck_fog_color;
Float sky_deck_shade;
Float sky_deck_fog_near;
Float sky_deck_fog_far;
Uint32 sky_deck_palettes_above[DRAW_PALETTES][DRAW_PALETTE_ENTRIES * 2];
Uint32 sky_deck_palettes_below[DRAW_PALETTES][DRAW_PALETTE_ENTRIES * 2];
int sky_deck_sky_mode;
NJS_POINT3 sky_deck_sky_pos;
Float sky_deck_sky_lift;
Float sky_deck_timeline_value;
int sky_deck_cloud_scroll;
int sky_deck_cloud_scroll_low;
Angle sky_deck_roll;
Angle sky_deck_pitch;
NJS_VECTOR dat_0caf373c;
Float sky_deck_timeline_rate;
int sky_deck_timeline_steps;
int sky_deck_clouds;
int sky_deck_cloud_wait;
NJS_COLOR sky_deck_cloud_uvs[CLOUD_UV_ROOM];
NJS_COLOR sky_deck_cloud_uvs_low[CLOUD_UV_ROOM];
int sky_deck_clock;
NJS_POINT3COL trail_line;
NJS_POINT3 trail_line_points[TRAIL_POINTS];
NJS_COLOR trail_line_colors[TRAIL_POINTS];
Float draft_lift;
Float draft_push;

/* the stage task's work->mode */
enum sky_deck_mode { SKY_DECK_INIT, SKY_DECK_RUN, SKY_DECK_END };

static void sky_deck_music_step(struct task *task);
static void sky_deck_act_start(struct task *task);
static void sky_deck_init(struct task *task);
/* NOLINTNEXTLINE(clang-diagnostic-strict-prototypes) */
static void sky_deck_end();
static void sky_deck_reset(Uint16 act);
static void sky_deck_move(Uint16 act);
static void sky_deck_land_blocks(void);
static void sky_deck_fog(void);
static void sky_deck_tilt(void);
static void sky_deck_sky_draw(struct task *task);
static void cloud_step(struct task *task);
static void cloud_draw(struct task *task);
static void cloud_destroy(struct task *task);

/* the step of the stage task's music child. once its timer counts below 0,
 * it resets the streams and asks for its mode's music, then ends */
static void sky_deck_music_step(struct task *task)
{
    struct task_work *work = task->work;

    --work->timer;
    if (work->timer < 0) {
        stream_reset();
        stream_request_music(work->mode);
        task_destroy(task);
    }
}

/* start the act stage_get_stage_act gives. free the trails and still the
 * deck and the draft. then play SE_STAGE, and start a child that asks for
 * the act's music. the first act also starts the wind. the second starts
 * the collapse, and the third turns switch 0 off */
static void sky_deck_act_start(struct task *task)
{
    Uint8 act;
    struct task *made;

    trails_reset();
    /* @hack redundant: a call of task_destroy that never runs. SHC writes no
     * code for it, but it still steers the registers: task takes R14 and
     * task_create_child's address R13, at 0x0c9003b0 and 0x0c9003ca, which
     * otherwise trade; without it, 12 of 142 words differ */
    if (0) {
        task_destroy(task);
    }
    act = stage_get_stage_act();
    sky_deck_reset(act);
    draft_reset();
    switch (act) {
    case 0:
        made = task_create_child(TASK_WORK, sky_deck_music_step, task);
        made->work->mode = MUSIC_SKYDECK1;
        made->work->timer = MUSIC_DELAY;
        sound_play_held(SE_STAGE, -1, 8, 0);
        task_create_child(TASK_WORK, sky_deck_wind_step, task);
        njSetDepthQueue(-1400.0F, -2000.0F);
        break;
    case 1:
        made = task_create_child(TASK_WORK, sky_deck_music_step, task);
        made->work->mode = MUSIC_SKYDECK1;
        made->work->timer = MUSIC_DELAY;
        sound_play_held(SE_STAGE, -1, 8, 0);
        njSetDepthQueue(-1400.0F, -2000.0F);
        collapse_reset();
        task_create_child(TASK_WORK, collapse_step, task);
        break;
    case 2:
        made = task_create_child(TASK_WORK, sky_deck_music_step, task);
        made->work->mode = MUSIC_SKYDECK2;
        made->work->timer = MUSIC_DELAY;
        sound_play_held(SE_STAGE, -1, 8, 0);
        njSetDepthQueue(-1400.0F, -2000.0F);
        switch_set(0, FALSE);
        break;
    default:
        return;
    }
}

void sky_deck_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SKY_DECK_INIT:
        sky_deck_init(task);
        break;
    case SKY_DECK_RUN:
        task_step_children(task);
        break;
    case SKY_DECK_END:
        /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
        sky_deck_end(task);
        /* the case ends without a break. with one, R2 and R3 trade from
         * 0x0c900534, and 5 of 44 words differ */
    }
    sky_deck_act = stage_get_stage_act();
    if (sky_deck_act != sky_deck_started_act) {
        sky_deck_act_start(task);
        sky_deck_started_act = sky_deck_act;
    }
}

/* the stage task's first step. it reads the palettes for above and below
 * the clouds, starts the act and turns the fog on. the fog starts white,
 * from 1250 to 9999 away, until the sky task's first sky_deck_fog */
static void sky_deck_init(struct task *task)
{
    struct task_work *work = task->work;

    sky_deck_act = stage_get_stage_act();
    sky_deck_started_act = sky_deck_act;
    sky_deck_work = work;
    work->mode = SKY_DECK_RUN;
    light_read_disc("PL_60B.BIN", sky_deck_palettes_above);
    light_read_disc("PL_61B.BIN", sky_deck_palettes_below);
#ifdef GCC
    /* repairs the unset bands and gusts: only act 0 starts the wind's task,
     * which clears them. in a later act that a load enters, the image's
     * sky_deck_wind_strength reads the words an earlier module left in
     * STG06's bss (MOV.L @R3,R1 at 0x0c905cca). GCC starts every load with
     * no wind, as act 0's task does */
    sky_deck_wind_clear();
#endif
    sky_deck_act_start(task);
    sky_deck_fog_color = 0xffffffff;
    sky_deck_shade = 0.0F;
    sky_deck_fog_near = -1250.0F;
    sky_deck_fog_far = -9999.0F;
    njGenerateFogTable3(sky_deck_fog_table, sky_deck_fog_near,
                        sky_deck_fog_far);
    njFogEnable();
    njClipZ(-3.0F, -1600.0F);
}

/* forget the stage task's work. sky_deck_step passes the task, in R4 at
 * 0x0c900532, and this reads no argument */
/* @hack type: sky_deck_end declared without a prototype, which keeps SHC
 * from expanding it at its call; declared with the task as its parameter,
 * it is expanded in sky_deck_step and omitted, and 24 of the 44 words of
 * sky_deck_step differ */
/* NOLINTNEXTLINE(clang-diagnostic-strict-prototypes) */
static void sky_deck_end()
{
    sky_deck_work = NULL;
}

/* a row of the second act's timeline. its value and rate hold from the row
 * before's time up to its own */
struct timeline_row {
    Float time; /* in TIMELINE_UNITs of the timeline's steps */
    Float value;
    Float rate;
};

#define TIMELINE_UNIT 30.0F

/* the second act's timeline, which ends at a row with a negative time. the
 * image's constants hold it between sky_deck_init's file names and the
 * welcome's text, so it is defined here */
static const struct timeline_row sky_deck_timeline[] = {
    {0.0F, 0.0F, 0.0F},    {10.0F, 0.0F, 0.0F},   {15.0F, 100.0F, 1.0F},
    {20.0F, 200.0F, 2.0F}, {25.0F, 300.0F, 3.0F}, {28.0F, 400.0F, 4.0F},
    {30.0F, 300.0F, 3.0F}, {38.0F, 200.0F, 2.0F}, {45.0F, 200.0F, 2.0F},
    {50.0F, 0.0F, 0.0F},   {-1.0F, 0.0F, 0.0F},
};

void sky_deck_welcome(void)
{
    struct bitmap bitmap;

    bitmap_alloc_window(&bitmap);
    bitmap_print(&bitmap, TEXT_1);
    bitmap_queue(&bitmap);
    bitmap_fade_in(&bitmap);
    bitmap_wait(&bitmap, 600);
    bitmap_fade_out(&bitmap);
    bitmap_free(&bitmap);
}

enum sky_mode { SKY_START, SKY_RUN };

/* the most clouds alive at once, and the countdown the sky starts as it
 * makes one. it makes the next as the countdown passes 0, 6 steps later */
#define CLOUDS_MOST 32
#define CLOUD_WAIT 5
/* the cloud layers scroll below SCROLL_WRAP in u, over the cloud model's
 * first CLOUD_UVS UVs */
#define SCROLL_WRAP 8192
#define CLOUD_UVS 4

/* the sky task's step. it sets world_land_blocks and the fog, and places
 * the sky over the act's origin. it scrolls the cloud layers and draws the
 * sky. outside the light editor, it also moves the deck and the draft.
 * while fewer than CLOUDS_MOST clouds live, it makes one every 6 steps. a
 * mode other than SKY_START and SKY_RUN destroys the children, and passes
 * the task to heap_free. in every mode, the step then gives player 0 a ball
 * for the trails to blow round. after that, it moves and draws the trails
 * three times */
void sky_deck_sky_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 act = stage_get_stage_act();
    NJS_POINT3 *origin;
    NJS_COLOR *from;
    NJS_COLOR *to;
    Uint32 tex_u;
    int tex_v;
    NJS_OBJECT *object;
    int i;
    struct task_work *player;
    /* @hack redundant: 28 bytes of frame that no code reads, of a type unknown,
     * declared as a Uint8[28]. the image's ADD #-32,R15 reserves them
     * (0x0c9006ca); without them, 2 of 252 words differ */
    Uint8 unk_frame[28];

    /* @hack redundant: the timer read into tex_v, which SHC drops but which
     * still steers the registers: work takes R10 and sky_deck_clouds' address
     * R9, from 0x0c9006d2 and 0x0c9006e0; without it, the two trade and 9 of
     * 252 words differ */
    tex_v = work->timer;
    act = (Uint8)act;
    switch (work->mode) {
    case SKY_START:
        work->mode = SKY_RUN;
        task->draw = sky_deck_sky_draw;
        sky_deck_cloud_scroll = 0;
        sky_deck_cloud_scroll_low = 0;
        sky_deck_sky_lift = LIFT_BASE;
        sky_deck_roll = 0;
        sky_deck_pitch = 0;
        sky_deck_clouds = 0;
#ifdef GCC
        /* repairs the unset wait: the start sets the cloud count but not
         * sky_deck_cloud_wait, so the image's first cloud waits on the word
         * an earlier module left in STG06's bss (MOV.L @R4,R3 at
         * 0x0c9008e0). GCC's first cloud comes on the sky's first run step */
        sky_deck_cloud_wait = 0;
#endif
        dat_0caf373c.x = dat_0caf373c.y = dat_0caf373c.z = 0.0F;
        sky_deck_timeline_rate = 0.0F;
        sky_deck_sky_mode = 0;
        /* fall through */
    case SKY_RUN:
        sky_deck_land_blocks();
        if (light_editor_mode == EDIT_OFF) {
            sky_deck_move(act);
            draft_step(act);
        }
        sky_deck_fog();
        /* @hack spelling: origin set inside the first store, which puts
         * sky_deck_sky_pos's pool word first, at 0x0c9008ac; set apart, the
         * two pool words and their loads trade places, and 3 of 252 words
         * and 4 pool words differ */
        sky_deck_sky_pos.x = (origin = &sky_deck_origins[act])->x;
        sky_deck_sky_pos.y = origin->y + sky_deck_sky_lift;
        sky_deck_sky_pos.z = origin->z;
        sky_deck_cloud_scroll -= 2;
        if (sky_deck_cloud_scroll < 0) {
            sky_deck_cloud_scroll += SCROLL_WRAP;
        }
        /* both layers scroll copies of one model's first CLOUD_UVS UVs. a
         * UV's word holds u in its low half and v in its high half */
        object = &dat_0c9ff65c;
        from = object->model->meshsets->vertuv;
        to = sky_deck_cloud_uvs;
        for (i = 0; i < CLOUD_UVS; i++) {
            tex_u = from->color;
            tex_v = (Sint16)(from->color >> 16);
            from++;
            tex_u += sky_deck_cloud_scroll;
            to->color = (((Uint32)tex_v << 16) & 0xffff0000) | (tex_u & 0xffff);
            to++;
        }
        sky_deck_cloud_scroll_low -= 4;
        if (sky_deck_cloud_scroll_low < 0) {
            sky_deck_cloud_scroll_low += SCROLL_WRAP;
        }
        from = object->model->meshsets->vertuv;
        to = sky_deck_cloud_uvs_low;
        for (i = 0; i < CLOUD_UVS; i++) {
            tex_u = from->color;
            tex_v = (Sint16)(from->color >> 16);
            from++;
            tex_u += sky_deck_cloud_scroll_low;
            to->color = (((Uint32)tex_v << 16) & 0xffff0000) | (tex_u & 0xffff);
            to++;
        }
        if (sky_deck_clouds < CLOUDS_MOST) {
            sky_deck_cloud_wait--;
            if (sky_deck_cloud_wait < 0) {
                sky_deck_clouds++;
                sky_deck_cloud_wait = CLOUD_WAIT;
                task_create_child(TASK_WORK, cloud_step, task);
            }
        }
        sky_deck_sky_draw(task);
        task_step_children(task);
        break;
    default:
        task_destroy_children(task);
        /* @bug memory: heap_free takes the task itself (JSR at 0x0c900916),
         * though tasks come from task_init's pool, not from heap_alloc */
        heap_free(task);
        break;
    }
    player = player_work[0];
    trail_volume_set(work, TRAIL_BALL, TRUE, 5.0F, 0.0F, player->pos.x,
                     player->pos.y, player->pos.z);
    trails_move();
    trails_draw();
    trails_move();
    trails_draw();
    trails_move();
    trails_draw();
}

/* the clouds' level below the sky's point. the fog and the shade blend
 * within FOG_SPAN above and below it */
#define CLOUD_LEVEL 1400.0F
#define FOG_SPAN 150.0F
/* the gray the shade makes of the fog and the back color, at a shade of 1 */
#define SHADE_GRAY 180.0F

/* set the fog and the shade by the camera's height against the clouds'
 * level. at the level, the fog runs from 0 to 300 away and the shade is 1.
 * from FOG_SPAN above or below it, the fog runs from 1250 to 9999 and the
 * shade is 0. both blend linearly between. the fog's color is the shade's
 * gray */
static void sky_deck_fog(void)
{
    Float height;
    Float shade;
    int gray;

    height = (sky_deck_sky_pos.y - camera_work->pos.y - CLOUD_LEVEL) *
             (1.0F / FOG_SPAN);
    if (height < -1.0F) {
        height = -1.0F;
    }
    if (height > 1.0F) {
        height = 1.0F;
    }
    /* @hack redundant: the shade's add of 0 and its products by 1, at
     * 0x0c9009c0 and 0x0c9009e0, and the near's subtraction from -0, at
     * 0x0c9009dc; without them, 47 of 89 words differ */
    if (height < 0.0F) {
        height += 1.0F;
        sky_deck_fog_near = -1250.0F + (height * 1250.0F);
        sky_deck_fog_far = -9999.0F + (height * 9699.0F);
        shade = 0.0F + (height * 1.0F);
        sky_deck_shade = height = shade;
    } else {
        sky_deck_fog_near = -0.0F - (height * 1250.0F);
        sky_deck_fog_far = -300.0F - (height * 9699.0F);
        shade = 1.0F;
        shade -= height * 1.0F;
        sky_deck_shade = height = shade;
    }
    /* @hack redundant: a test of shade that never runs. SHC writes no code
     * for it, but it still steers the registers: shade stays in FR6 in both
     * arms, where the first would take FR5 at 0x0c9009c6; without it, 1 of 89
     * words differ */
    if (0) {
        if (shade) {
        }
    }
    gray = height * SHADE_GRAY;
    sky_deck_fog_color = 0xff000000 | (gray << 16) | (gray << 8) | gray;
    njGenerateFogTable3(sky_deck_fog_table, sky_deck_fog_near,
                        sky_deck_fog_far);
}

/* make the deck's pitch and roll unsigned angles within their limits, and
 * tilt gravity with them */
static void sky_deck_tilt(void)
{
    sky_deck_pitch = (Uint16)sky_deck_pitch;
    sky_deck_roll = (Uint16)sky_deck_roll;
    if (sky_deck_pitch > PITCH_MAX && sky_deck_pitch < 0x8000) {
        sky_deck_pitch = PITCH_MAX;
    } else if (sky_deck_pitch > 0x8000 && sky_deck_pitch < PITCH_MIN) {
        sky_deck_pitch = PITCH_MIN;
    }
    if (sky_deck_roll > ROLL_MAX && sky_deck_roll < 0x8000) {
        sky_deck_roll = ROLL_MAX;
    } else if (sky_deck_roll > 0x8000 && sky_deck_roll < ROLL_MIN) {
        sky_deck_roll = ROLL_MIN;
    }
    world_tilt_gravity(sky_deck_pitch, sky_deck_roll);
}

static void sky_deck_reset(Uint16 act)
{
    sky_deck_swing = 0;
    sky_deck_bob = 0;
    sky_deck_cloud_scroll = 0;
    sky_deck_cloud_scroll_low = 0;
    sky_deck_sky_lift = LIFT_BASE;
    sky_deck_roll = 0;
    sky_deck_pitch = 0;
    sky_deck_clouds = 0;
    dat_0caf373c.x = dat_0caf373c.y = dat_0caf373c.z = 0.0F;
    sky_deck_timeline_rate = 0.0F;
    sky_deck_phase = 0;
    dat_0cadbfdc = 0.0F;
    sky_deck_rise = 0.0F;
    w_wall_hit = FALSE;
    sky_deck_fell = FALSE;
    sky_deck_clock = 0;
}

/* move the deck for act. in the first two acts, the clock flips
 * sky_deck_phase at the act's turns, with a sound. the sky then rises to
 * RISE_MOST or sinks to 0. while the sky moves, the deck rolls to SWAY one
 * way. once the sky stops, the deck sways SWAY either way.
 *
 * in the third act, player 0 falls out of the stage once it passes
 * FALL_FROM along x. for any character but Knuckles, sky_deck_phase
 * follows player 0's place along x. each tilt_phase sets the deck's pitch
 * and roll. while switch 0 is on, the deck levels instead */
static void sky_deck_move(Uint16 act)
{
    int i;
    int played = 0;

    if (act == 1) {
        sky_deck_clock++;
        if (sky_deck_clock > CLOCK_WRAP) {
            sky_deck_clock = 0;
        }
        for (i = 0; sky_deck_turns_act1[i] > 0 && !played; i++) {
            if (sky_deck_turns_act1[i] * TURN_UNIT == sky_deck_clock) {
                sky_deck_phase = (sky_deck_phase + 1) & 1;
                if (sky_deck_phase == SKY_SINKS) {
                    sound_play_for(SE_SINK, 65, 1, 0, 180);
                    if (act == 1 && player_work[0]->pos.z > 0.0F) {
                        sound_play(SE_SINK_2, 0, 0, 0);
                    }
                } else {
                    sound_play_for(SE_RISE, 65, 1, 0, 180);
                    if (act == 1 && player_work[0]->pos.z > 0.0F) {
                        sound_play(SE_RISE_2, 0, 0, 0);
                    }
                }
                played = 1;
            }
        }
    } else if (act == 0) {
        sky_deck_clock++;
        if (sky_deck_clock > CLOCK_WRAP) {
            sky_deck_clock = 0;
        }
        for (i = 0; sky_deck_turns_act0[i] > 0 && !played; i++) {
            if (sky_deck_turns_act0[i] * TURN_UNIT == sky_deck_clock) {
                sky_deck_phase = (sky_deck_phase + 1) & 1;
                if (sky_deck_phase == SKY_SINKS) {
                    sound_play_for(SE_SINK, 65, 1, 0, 180);
                } else {
                    sound_play_for(SE_RISE, 65, 1, 0, 180);
                }
                played = 1;
            }
        }
    }
    if (act == 1) {
        sky_deck_timeline_steps++;
        for (i = 0; sky_deck_timeline[i].time >= 0.0F &&
                    sky_deck_timeline[i].time * TIMELINE_UNIT <
                        sky_deck_timeline_steps;
             i++) {
        }
        if (sky_deck_timeline[i].time < 0.0F) {
            sky_deck_timeline_steps = 0;
        }
        sky_deck_timeline_rate = sky_deck_timeline[i].rate;
        sky_deck_timeline_value = sky_deck_timeline[i].value;
        dat_0caf373c.x =
            float_approach(dat_0caf373c.x, sky_deck_timeline_rate, 0.05F);
    }
    sky_deck_bob++;
    switch (act) {
    case 0:
    case 1:
        switch (sky_deck_phase) {
        case SKY_SINKS:
            sky_deck_rise = float_approach(sky_deck_rise, 0.0F, RISE_SPEED);
            if (sky_deck_rise > 0.0F) {
                sky_deck_swing = (Uint16)sky_deck_swing;
                sky_deck_swing = angle_approach(sky_deck_swing, 0x4000, 128);
            } else {
                sky_deck_swing += 64;
            }
            break;
        case SKY_RISES:
            sky_deck_rise =
                float_approach(sky_deck_rise, RISE_MOST, RISE_SPEED);
            if (sky_deck_rise < RISE_MOST) {
                sky_deck_swing = (Uint16)sky_deck_swing;
                sky_deck_swing = angle_approach(sky_deck_swing, 0xc000, 128);
            } else {
                sky_deck_swing += 64;
            }
            break;
        }
        sky_deck_roll = njSin(sky_deck_swing) * SWAY;
        sky_deck_sky_lift = sky_deck_rise -
                            (njSin((sky_deck_bob * 64) - 8192) * BOB) +
                            LIFT_BASE;
        break;
    case 2:
        /* @hack redundant: a goto to the next statement, which SHC drops but
         * which still gives player_work's address R2 at 0x0c900fc0, and
         * sky_deck_fell's value R1 at 0x0c900fd2; without it, the two trade
         * and 4 of 545 words differ */
        goto player_check;
    player_check:
        if (player_work[0]->pos.x < FALL_FROM && !sky_deck_fell) {
            sky_deck_fell = TRUE;
            player_fall(0);
        }
        if (player_get_character(0) == CHARACTER_KNUCKLES) {
            return;
        }
        if (switch_is_on(0)) {
            sky_deck_roll = angle_approach(sky_deck_roll, 0, 64);
            sky_deck_pitch = angle_approach(sky_deck_pitch, 0, 64);
            if (player_work[0]->pos.x > SWITCH_OFF_X &&
                player_work[0]->pos.y < SWITCH_OFF_Y) {
                switch_set(0, FALSE);
            }
        } else {
            switch (sky_deck_phase) {
            case TILT_LOW:
                if (player_work[0]->pos.x > ROCK_FROM) {
                    sky_deck_phase = TILT_ROCK_HIGH;
                }
                sky_deck_pitch = angle_approach(sky_deck_pitch, PITCH_MIN, 16);
                sky_deck_roll = angle_approach(sky_deck_roll, 0, 16);
                break;
            case TILT_ROCK_HIGH:
                if (player_work[0]->pos.x < ROCK_FROM) {
                    sky_deck_phase = TILT_LOW;
                }
                if (player_work[0]->pos.x > LEVEL_FROM) {
                    sky_deck_phase = TILT_LEVEL;
                }
                if (sky_deck_pitch >= PITCH_MAX && sky_deck_pitch < 0x8000) {
                    sky_deck_phase = TILT_ROCK_LOW;
                }
                sky_deck_pitch = angle_approach(sky_deck_pitch, PITCH_MAX, 16);
                sky_deck_roll = angle_approach(sky_deck_roll, 0, 16);
                break;
            case TILT_ROCK_LOW:
                if (player_work[0]->pos.x < ROCK_FROM) {
                    sky_deck_phase = TILT_LOW;
                }
                if (player_work[0]->pos.x > LEVEL_FROM) {
                    sky_deck_phase = TILT_LEVEL;
                }
                if (sky_deck_pitch > 0x8000 && sky_deck_pitch <= PITCH_MIN) {
                    sky_deck_phase = TILT_ROCK_HIGH;
                }
                sky_deck_pitch = angle_approach(sky_deck_pitch, PITCH_MIN, 16);
                sky_deck_roll = angle_approach(sky_deck_roll, 0, 16);
                break;
            case TILT_LEVEL:
                if (player_work[0]->pos.x < LEVEL_FROM) {
                    sky_deck_phase = TILT_ROCK_HIGH;
                }
                if (player_work[0]->pos.x > ROLL_FROM) {
                    sky_deck_phase = TILT_ROLLED;
                }
                sky_deck_pitch = angle_approach(sky_deck_pitch, 0, 16);
                sky_deck_roll = angle_approach(sky_deck_roll, 0, 16);
                break;
            case TILT_ROLLED:
                if (player_work[0]->pos.x < ROCK_FROM) {
                    sky_deck_phase = TILT_LEVEL;
                }
                sky_deck_pitch = angle_approach(sky_deck_pitch, 0, 16);
                sky_deck_roll = angle_approach(sky_deck_roll, ROLL_MIN, 64);
                break;
            default:
                sky_deck_phase = TILT_LOW;
                break;
            }
        }
        break;
    default:
        return;
    }
    sky_deck_tilt();
}

/* in the first act, draw the land's first block alone. draw none while
 * the camera is past 211 along x and short of 3570 along z. the other acts
 * leave world_land_blocks as it is */
static void sky_deck_land_blocks(void)
{
    Uint16 act = (Uint8)stage_get_stage_act();
    Float x = camera_work->pos.x;
    Float z = camera_work->pos.z;

    switch (act) {
    case 0:
        world_land_blocks = FIRST_BLOCK;
        if (x > 211.0F && z < 3570.0F) {
            world_land_blocks &= ~FIRST_BLOCK;
        }
        break;
    case 1:
    case 2:
        break;
    }
}

/* the heights of the cloud layers, below the sky's point. the camera is
 * under the clouds once it is below LOWER_LAYER */
#define UPPER_LAYER 1350.0F
#define LOWER_LAYER 1450.0F
/* how many draw_palettes the sky sets, from the first */
#define SKY_PALETTES 4

/* draw the sky about its point, rolled and pitched with the deck, while
 * sky_deck_sky_mode is 0. the sky's model is the one for above the clouds'
 * level, or the one for below. the shade grays the back color and the sky
 * model's unlit offsets. the first SKY_PALETTES draw_palettes take the
 * first act's palettes above the clouds' level, and the second's below.
 *
 * the two cloud layers draw over the sky, each faded most as the camera
 * reaches its height. under the clouds, two more layers draw 600 and 700
 * below the lower one. they are fainter by 0.25 in alpha */
static void sky_deck_sky_draw(struct task *task)
{
    int gray;
    int color;
    int bank;
    int i;
    NJS_OBJECT *object;
    NJS_COLOR *uv;
    Float fade;
    NJS_ARGB material;

    draw_control_save();
    draw_control_set(NJD_CONTROL_3D_OFFSET_MATERIAL);
    njFogDisable();
    njClipZ(-3.0F, -40000.0F);
    njPushMatrix(NULL);
    njTranslate(NULL, sky_deck_sky_pos.x, sky_deck_sky_pos.y,
                sky_deck_sky_pos.z);
    TURN(njRotateZ, sky_deck_roll);
    TURN(njRotateX, sky_deck_pitch);
    njScale(NULL, 1.7F, 1.0F, 1.7F);
    switch (sky_deck_sky_mode) {
    case 0:
        gray = sky_deck_shade * SHADE_GRAY;
        color = (gray << 16) | (gray << 8) | gray;
        for (i = 0; i < DRAW_UNLIT_OFFSETS; ++i) {
            draw_unlit_offsets[i] = color;
        }
        njSetBackColor(color, color, color);
        if (!sky_deck_palettes_held) {
            for (bank = 0; bank < SKY_PALETTES; ++bank) {
                if (camera_work->pos.y > sky_deck_sky_pos.y - CLOUD_LEVEL) {
                    for (i = 0; i < DRAW_PALETTE_ENTRIES * 2; ++i) {
                        draw_palettes[bank][i] =
                            sky_deck_palettes_above[bank][i];
                    }
                } else {
                    for (i = 0; i < DRAW_PALETTE_ENTRIES * 2; ++i) {
                        draw_palettes[bank][i] =
                            sky_deck_palettes_below[bank][i];
                    }
                }
            }
        }
        if (camera_work->pos.y < sky_deck_sky_pos.y - CLOUD_LEVEL) {
            object = &dat_0ca0198c;
        } else {
            object = &dat_0ca00104;
        }
        njSetTexture(&obj_skydeck_texlist);
        draw_tree(object);
        for (i = 0; i < DRAW_UNLIT_OFFSETS; ++i) {
            draw_unlit_offsets[i] = 0;
        }
        fade = _builtin_fabsf(camera_work->pos.y - sky_deck_sky_pos.y +
                              UPPER_LAYER);
        fade *= 0.025F;
        fade = 1.0F - fade;
        if (fade < 0.05F) {
            fade = 0.05F;
        }
        if (fade > 0.9F) {
            fade = 0.9F;
        }
        /* @hack redundant: the fade's product by 1, the FMUL at 0x0c9014cc;
         * without it, 96 of 427 words differ */
        material.a = material.r = material.g = material.b = -(fade * 1.0F);
        njSetConstantMaterial(&material);
        if (camera_work->pos.y < sky_deck_sky_pos.y - LOWER_LAYER) {
            object = &dat_0ca01a90;
        } else {
            object = &dat_0c9ff65c;
        }
        uv = object->model->meshsets->vertuv;
        object->model->meshsets->vertuv = sky_deck_cloud_uvs;
        draw_tree(object);
        object->model->meshsets->vertuv = uv;
        fade = _builtin_fabsf(camera_work->pos.y - sky_deck_sky_pos.y +
                              LOWER_LAYER);
        fade *= 0.025F;
        fade = 1.0F - fade;
        if (fade < 0.01F) {
            fade = 0.01F;
        }
        if (fade > 0.9F) {
            fade = 0.9F;
        }
        /* @hack redundant: the fade's product by 1, the FMUL at 0x0c90154e;
         * without it, 29 of 427 words differ */
        material.a = material.r = material.g = material.b = -(fade * 1.0F);
        njSetConstantMaterial(&material);
        njTranslate(NULL, 0.0F, -100.0F, 0.0F);
        if (camera_work->pos.y < sky_deck_sky_pos.y - LOWER_LAYER) {
            object = &dat_0ca01a90;
        } else {
            object = &dat_0c9ff65c;
        }
        uv = object->model->meshsets->vertuv;
        object->model->meshsets->vertuv = sky_deck_cloud_uvs_low;
        draw_tree(object);
        object->model->meshsets->vertuv = uv;
        if (camera_work->pos.y < sky_deck_sky_pos.y - LOWER_LAYER) {
            material.a -= 0.25F;
            njSetConstantMaterial(&material);
            njTranslate(NULL, 0.0F, -600.0F, 0.0F);
            object = &dat_0ca01a90;
            uv = object->model->meshsets->vertuv;
            object->model->meshsets->vertuv = sky_deck_cloud_uvs_low;
            draw_tree(object);
            njTranslate(NULL, 0.0F, -100.0F, 0.0F);
            object->model->meshsets->vertuv = sky_deck_cloud_uvs;
            draw_tree(object);
            object->model->meshsets->vertuv = uv;
        }
        material.a = material.r = material.g = material.b = 0.0F;
        njSetConstantMaterial(&material);
        break;
    }
    njPopMatrix(1);
    njFogEnable();
    draw_control_restore();
    njClipZ(-3.0F, -1600.0F);
}

enum cloud_mode { CLOUD_START, CLOUD_RUN };

struct cloud_vars {
    Uint8 unk_00[8];
    Float fade; /* from 0 to 0.2, at random */
};

#define CLOUD_VARS(work) ((struct cloud_vars *)&(work)->vars)

/* a cloud starts 3000 to 3500 short of the camera along x, within 3000
 * along z. it drifts 30 along x a step with a wind gust round it. it ends
 * 3000 past the camera */
static void cloud_step(struct task *task)
{
    struct task_work *work = task->work;
    Float x;
    Float z;

    switch (work->mode) {
    case CLOUD_START:
        work->mode = CLOUD_RUN;
        task->draw = cloud_draw;
        task->destructor = cloud_destroy;
        x = camera_work->pos.x;
        z = camera_work->pos.z;
        x -= (rand() * (500.0F * RAND_SCALE)) + 3000.0F;
        z += (rand() * (6000.0F * RAND_SCALE)) - 3000.0F;
        work->pos.x = x;
        work->pos.z = z;
        CLOUD_VARS(work)->fade = rand() * (0.2F * RAND_SCALE);
        /* @bug arithmetic: the cast truncates rand() * RAND_SCALE, which is
         * below 1, before the scale to a turn: FTRC at 0x0c9017a2, then
         * SHLL16 at 0x0c9017a6. so ang.x always starts at 0, and every
         * cloud's pulse starts in phase */
        work->ang.x = (int)(rand() * RAND_SCALE) * 0x10000;
        if (rand() * RAND_SCALE < 0.5F) {
            work->ang.y = 0x4000;
        } else {
            work->ang.y = -0x4000;
        }
        /* fall through */
    case CLOUD_RUN:
        work->pos.x += 30.0F;
        if (work->pos.x > camera_work->pos.x + 3000.0F) {
            sky_deck_wind_gust_remove(task);
            task_destroy(task);
            return;
        }
        cloud_draw(task);
        sky_deck_wind_gust_set(task);
        break;
    }
}

/* draw the cloud while it is in view, UPPER_LAYER below the sky's point
 * and rolled with the deck. it is 10 times its model's width and 4.5 times
 * its height. its constant material pulses as ang.x turns. from 2000 to 3000
 * from the camera in x and z, the material blends on to -1 */
static void cloud_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    NJS_ARGB material;
    Float dx;
    Float dz;
    Float length;
    Float distance;
    Float alpha;
    Float blend;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 100.0F)) {
        return;
    }
    draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
    njFogDisable();
    dx = camera_work->pos.x - work->pos.x;
    dz = camera_work->pos.z - work->pos.z;
    length = njSqrt((dx * dx) + (dz * dz));
    work->ang.x += 0x200;
    distance = length - 2000.0F;
    if (distance < 0.0F) {
        distance = 0.0F;
    }
    if (distance > 1000.0F) {
        distance = 1000.0F;
    }
    alpha = 1.0F - ((1.0F - CLOUD_VARS(work)->fade) *
                    ((njSin(work->ang.x) * 0.25F) + 0.75F) * 0.5F);
    /* @hack spelling: the blend in three statements, which multiplies by -1
     * from the pool, at 0x0c901a38; as one expression, SHC folds the add into
     * an FMAC and the -1 into an FNEG, and 66 of 194 words differ */
    blend = (1.0F - alpha) * distance * 0.001F;
    blend += alpha;
    blend *= -1.0F;
    material.r = material.g = material.b = material.a = blend;
    njSetConstantMaterial(&material);
    njSetTexture(&obj_skydeck_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, sky_deck_sky_pos.x, sky_deck_sky_pos.y,
                sky_deck_sky_pos.z);
    TURN(njRotateZ, sky_deck_roll);
    njTranslate(NULL, work->pos.x - sky_deck_sky_pos.x, -UPPER_LAYER,
                work->pos.z - sky_deck_sky_pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, 10.0F, 4.5F, 10.0F);
    draw_tree(&dat_0ca00a08);
    njPopMatrix(1);
    njFogEnable();
    /* @hack spelling: the 0 through blend, which puts it in FR4 at
     * 0x0c901aa6; as a plain 0, it takes FR3 and 5 of 194 words differ */
    blend = 0.0F;
    material.a = material.r = material.g = material.b = blend;
    njSetConstantMaterial(&material);
}

static void cloud_destroy(struct task *task)
{
    --sky_deck_clouds;
}

enum model_mode { MODEL_START, MODEL_RUN };

/* each step once the collapse reaches it, a set piece or the duct falls
 * away with the land. it turns about z, sinks and drifts along x by these */
#define PIECE_TURN 32
#define PIECE_SINK 0.9F
#define PIECE_DRIFT 1.8F

/* the step of a set piece. model is its row of sky_deck_models. motion is
 * the motion it plays, from 0 to 9, or -1 for none. a row outside 0 to 80
 * is row 0, and a motion above 10 is motion 0. outside the set editor, once the
 * collapse reaches the piece, it falls away with the land. leaving range after
 * that ends its placement, so it doesn't come back */
static void sky_deck_model_step(struct task *task, Sint8 model, Sint8 motion)
{
    struct task_work *work = task->work;

    if (model < 0 || model > 80) {
        model = 0;
    }
    if (motion > 10) {
        motion = 0;
    }
    work->submode = model;
    motion++;
    work->tick = motion;
    if (placement_try_leave(task)) {
        if (work->mode == MODEL_RUN &&
            collapse_reached(work->pos.x, work->pos.y, work->pos.z)) {
            placement_kill(task);
        }
        return;
    }
    if (placement_editing(task)) {
        sky_deck_model_draw(task);
    } else {
        switch (work->mode) {
        case MODEL_START:
            task->draw = sky_deck_model_draw;
            task->destructor = sky_deck_model_destroy;
            work->mode = MODEL_RUN;
            work->scl.z = 0.0F;
            MODEL_VARS(work)->world = NULL;
            break;
        case MODEL_RUN:
            if (collapse_reached(work->pos.x, work->pos.y, work->pos.z)) {
                work->ang.z += PIECE_TURN;
                work->pos.y -= PIECE_SINK;
                work->pos.x += PIECE_DRIFT;
            }
            break;
        }
        sky_deck_model_draw(task);
        sky_deck_model_world(task);
    }
}

void sky_deck_model_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = sky_deck_models[work->submode].object;
    NJS_MOTION *motion;
    Float scale;
    Float frame;
    Angle ang;

    if (frame_step) {
        return;
    }
    if (object) {
        njSetTexture(&obj_skydeck_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        if (work->scl.x != 1.0F) {
            scale = work->scl.x;
            njScale(NULL, scale, scale, scale);
            draw_divide_light(work->scl.x);
        }
        if (work->tick) {
            motion = sky_deck_motions[work->tick - 1];
            work->scl.z += 1.0F;
            if (work->scl.z >= sky_deck_motion_frames[work->tick - 1] * 2.0F) {
                work->scl.z = 0.0F;
            }
            frame = work->scl.z;
            if (frame >= sky_deck_motion_frames[work->tick - 1] - 1.0F) {
                frame = sky_deck_motion_frames[work->tick - 1] - 1.0F;
            }
            draw_motion(object, motion, frame);
        } else {
            draw_tree(object);
        }
        if (work->scl.x != 1.0F) {
            draw_reset_light();
        }
        njPopMatrix(1);
    }
    sky_deck_shadow_draw(task, sky_deck_models[work->submode].shadow);
}

/* a set piece casts a shadow while it lies from UPPER_LAYER to SHADOW_TOP
 * below the sky's point. the shadow lies SHADOW_DEPTH below the sky's point,
 * just under the upper cloud layer */
#define SHADOW_TOP 1150.0F
#define SHADOW_DEPTH 1360.0F

void sky_deck_shadow_draw(struct task *task, NJS_OBJECT *object)
{
    struct task_work *work = task->work;
    Float alpha;
    Float jitter;
    Float fade;
    NJS_ARGB material;
    Angle ang;

    if (!object) {
        return;
    }
    if (!(work->pos.y < sky_deck_sky_pos.y - SHADOW_TOP &&
          sky_deck_sky_pos.y - UPPER_LAYER < work->pos.y)) {
        return;
    }
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    /* @hack spelling: the fade in three statements, which adds UPPER_LAYER and
     * then subtracts 100, at 0x0c901e62 and 0x0c901e6c; as one expression,
     * SHC adds 1250, and 140 of 179 words differ */
    fade = work->pos.y - sky_deck_sky_pos.y + UPPER_LAYER;
    fade -= 100.0F;
    fade *= 0.01F;
    alpha = -fade;
    if (alpha > -0.3F) {
        alpha = -0.3F;
    }
    jitter = -2.0F + (rand() * (1.0F / 8192.0F));
    material.a = alpha;
    material.r = material.g = material.b = 0.0F;
    njSetConstantMaterial(&material);
    njSetTexture(&obj_skydeck_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, sky_deck_sky_pos.x, sky_deck_sky_pos.y + jitter,
                sky_deck_sky_pos.z);
    TURN(njRotateZ, sky_deck_roll);
    njTranslate(NULL, work->pos.x - sky_deck_sky_pos.x, -SHADOW_DEPTH,
                work->pos.z - sky_deck_sky_pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(object);
    njPopMatrix(1);
    material.a = material.r = material.g = material.b = 0.0F;
    njSetConstantMaterial(&material);
    draw_attr_restore();
}

void sky_deck_model_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *shape = sky_deck_models[work->submode].world;
    NJS_OBJECT *world;
    union point_floats center;
    Angle ang;

    if (shape) {
        if (!MODEL_VARS(work)->world) {
            MODEL_VARS(work)->world = world = world_object_alloc();
            world->evalflags = NJD_EVAL_BREAK;
            world->scl[0] = 1.0F;
            world->scl[1] = 1.0F;
            world->scl[2] = 1.0F;
            world->ang[0] = work->ang.x;
            world->ang[1] = work->ang.y;
            world->ang[2] = work->ang.z;
            world->pos[0] = work->pos.x;
            world->pos[1] = work->pos.y;
            world->pos[2] = work->pos.z;
            world->model = shape->model;
            world->child = shape->child;
            world->sibling = shape->sibling;
            world_add(WORLD_SOLID, task, world);
        }
        center.vector.x = shape->model->center.x;
        center.vector.y = shape->model->center.y;
        center.vector.z = shape->model->center.z;
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        /* @hack type: the source as the union's own address, which builds
         * center's address from R15 at each use, as at 0x0c9020d6, and gives
         * shape R13; as the member's address, center's address takes R13 and
         * shape R12, and 148 of 163 words differ */
        njCalcVector(NULL, (NJS_VECTOR *)&center, &center.vector);
        njPopMatrix(1);
        center.vector.x += work->pos.x;
        center.vector.y += work->pos.y;
        center.vector.z += work->pos.z;
        if (player_in_range(&center.point, shape->model->r + 50.0F)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
}

void sky_deck_model_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (MODEL_VARS(work)->world) {
        world_remove(task, MODEL_VARS(work)->world);
        world_object_free(MODEL_VARS(work)->world);
        MODEL_VARS(work)->world = NULL;
    }
}

void edge_0_step(struct task *task)
{
    sky_deck_model_step(task, 4, -1);
}

void branch_l_step(struct task *task)
{
    sky_deck_model_step(task, 5, -1);
}

void branch_s_step(struct task *task)
{
    sky_deck_model_step(task, 6, -1);
}

void pole_s_step(struct task *task)
{
    sky_deck_model_step(task, 15, -1);
}

void pole_l_step(struct task *task)
{
    sky_deck_model_step(task, 16, -1);
}

void base_l_step(struct task *task)
{
    sky_deck_model_step(task, 18, -1);
}

void rest_6_step(struct task *task)
{
    sky_deck_model_step(task, 22, -1);
}

void rest_45_step(struct task *task)
{
    sky_deck_model_step(task, 23, -1);
}

void block_0_step(struct task *task)
{
    sky_deck_model_step(task, 24, -1);
}

void block_1_step(struct task *task)
{
    sky_deck_model_step(task, 25, -1);
}

void block_2_step(struct task *task)
{
    sky_deck_model_step(task, 26, -1);
}

void block_3_step(struct task *task)
{
    sky_deck_model_step(task, 27, -1);
}

void block_4_step(struct task *task)
{
    sky_deck_model_step(task, 28, -1);
}

void block_5_step(struct task *task)
{
    sky_deck_model_step(task, 29, -1);
}

void cover_step(struct task *task)
{
    sky_deck_model_step(task, 30, -1);
}

void screw_step(struct task *task)
{
    sky_deck_model_step(task, 31, -1);
}

void launcher_step(struct task *task)
{
    sky_deck_model_step(task, 32, -1);
}

void block_6_step(struct task *task)
{
    sky_deck_model_step(task, 33, -1);
}

void block_7_step(struct task *task)
{
    sky_deck_model_step(task, 34, -1);
}

void rail_l_step(struct task *task)
{
    sky_deck_model_step(task, 41, -1);
}

void rail_s_step(struct task *task)
{
    sky_deck_model_step(task, 42, -1);
}

void fun_0c90226c(struct task *task)
{
    sky_deck_model_step(task, 43, -1);
}

void lantern_step(struct task *task)
{
    sky_deck_model_step(task, 44, -1);
}

void tower_i_step(struct task *task)
{
    sky_deck_model_step(task, 47, -1);
}

void rest_9_step(struct task *task)
{
    sky_deck_model_step(task, 48, -1);
}

void fun_0c902284(struct task *task)
{
    sky_deck_model_step(task, 49, -1);
}

void cold_fan_step(struct task *task)
{
    sky_deck_model_step(task, 50, 7);
}

void fun_0c902290(struct task *task)
{
    sky_deck_model_step(task, 51, 8);
}

void fun_0c902296(struct task *task)
{
    sky_deck_model_step(task, 52, -1);
}

void fun_0c90229c(struct task *task)
{
    sky_deck_model_step(task, 53, -1);
}

void fun_0c9022a2(struct task *task)
{
    sky_deck_model_step(task, 54, -1);
}

void fun_0c9022a8(struct task *task)
{
    sky_deck_model_step(task, 55, -1);
}

void fun_0c9022ae(struct task *task)
{
    sky_deck_model_step(task, 56, -1);
}

void fun_0c9022b4(struct task *task)
{
    sky_deck_model_step(task, 57, -1);
}

void carrier_step(struct task *task)
{
    sky_deck_model_step(task, 58, -1);
}

void carrier2_step(struct task *task)
{
    sky_deck_model_step(task, 75, -1);
}

void carrier3_step(struct task *task)
{
    sky_deck_model_step(task, 76, -1);
}

void fun_0c9022cc(struct task *task)
{
    sky_deck_model_step(task, 60, -1);
}

void fun_0c9022d2(struct task *task)
{
    sky_deck_model_step(task, 61, -1);
}

void fun_0c9022d8(struct task *task)
{
    sky_deck_model_step(task, 62, -1);
}

void fun_0c9022de(struct task *task)
{
    sky_deck_model_step(task, 63, -1);
}

void fun_0c9022e4(struct task *task)
{
    sky_deck_model_step(task, 64, -1);
}

void ctrltower_step(struct task *task)
{
    sky_deck_model_step(task, 65, 9);
}

void scope_step(struct task *task)
{
    sky_deck_model_step(task, 68, -1);
}

void edge_step(struct task *task)
{
    sky_deck_model_step(task, 70, -1);
}

void la_step(struct task *task)
{
    sky_deck_model_step(task, 71, -1);
}

void lb_step(struct task *task)
{
    sky_deck_model_step(task, 72, -1);
}

void lc_step(struct task *task)
{
    sky_deck_model_step(task, 73, -1);
}

void stop_step(struct task *task)
{
    sky_deck_model_step(task, 74, -1);
}

void s_wing_step(struct task *task)
{
    sky_deck_model_step(task, 77, -1);
}

void s_map_step(struct task *task)
{
    sky_deck_model_step(task, 78, -1);
}

void s_obj_step(struct task *task)
{
    sky_deck_model_step(task, 79, -1);
}

void s_eng_step(struct task *task)
{
    sky_deck_model_step(task, 80, -1);
}

/* TRUE when the squared distance from pos to x, y, z is at most
 * range_squared */
static Bool sky_deck_in_range(NJS_POINT3 *pos, Float x, Float y, Float z,
                              Float range_squared)
{
    Float px;
    Float py;
    Float pz;
    Float distance;
    const unsigned char *at;

    px = pos->x;
    /* @hack spelling: y and z read through a byte cursor, which SHC walks with
     * @R4+ at 0x0c90232c and 0x0c902330; read as members, they take more
     * code, and 23 of 23 words differ */
    at = (const unsigned char *)pos + sizeof pos->x;
    py = *(const Float *)at;
    at += sizeof pos->y;
    pz = *(const Float *)at;
    x -= px;
    distance = x * x;
    if (distance <= range_squared) {
        y -= py;
        distance += y * y;
        if (distance <= range_squared) {
            z -= pz;
            distance += z * z;
            if (distance <= range_squared) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static Bool sky_deck_try_leave_by(struct task *task, Float range_squared)
{
    struct placement *placement = task->placement;
    struct task_work *viewer;
    Float x;
    Float y;
    Float z;

    if (placement != NULL && (placement->flags & PLACEMENT_IN_RANGE)) {
        return FALSE;
    }
    if (range_squared == 0.0F) {
        return FALSE;
    }
    x = task->work->pos.x;
    y = task->work->pos.y;
    z = task->work->pos.z;
    /* @hack spelling: each viewer assigned inside its test, which saves the
     * first two to the stack and loads them back, as at 0x0c902396;
     * assigned apart, they stay in R13, and 81 of 85 words differ */
    if ((viewer = camera_work) != NULL &&
        sky_deck_in_range(&viewer->pos, x, y, z, range_squared)) {
        return FALSE;
    }
    if ((viewer = player_work[0]) != NULL &&
        sky_deck_in_range(&viewer->pos, x, y, z, range_squared)) {
        return FALSE;
    }
    if ((viewer = player_work[1]) != NULL &&
        PLAYER_VARS(viewer)->character == CHARACTER_SONIC &&
        sky_deck_in_range(&viewer->pos, x, y, z, 400.0F * 400.0F)) {
        return FALSE;
    }
    task->step = task_destroy;
    return TRUE;
}

Bool sky_deck_try_leave(struct task *task)
{
    if (task->placement) {
        return sky_deck_try_leave_by(task, task->placement->leave.range);
    }
    return sky_deck_try_leave_by(task, 410.0F * 410.0F);
}

enum next_act_mode { NEXT_ACT_START, NEXT_ACT_RUN };

static void next_act_destroy(struct task *task);

void next_act_step(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision;

    switch (work->mode) {
    case NEXT_ACT_START:
        if (!placement_editing(task)) {
            collision_init(task, &next_act_shape, COLLISION_OBJECT, 1, 1);
        }
        task->destructor = next_act_destroy;
        work->mode = NEXT_ACT_RUN;
        break;
    case NEXT_ACT_RUN:
        if (!placement_editing(task)) {
            collision = work->collision;
            if ((collision->flags & COLLISION_HIT) &&
                collision->hit->kind == COLLISION_PLAYER) {
                world_change_act(1);
                placement_change_act(1);
                camera_change_act(1);
                stage_change_act(1);
                stage_place_player(player_work[0]);
            }
            if (player_get(0)->held != task) {
                collision_register(work);
            }
        }
        break;
    }
}

static void next_act_destroy(struct task *task)
{
}

enum change_act_mode { CHANGE_ACT_START, CHANGE_ACT_WAIT };

void sky_deck_voice_change_act_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case CHANGE_ACT_START:
        if (!placement_editing(task)) {
            work->mode = CHANGE_ACT_WAIT;
        }
        work->timer = 60;
        if (get_character() == CHARACTER_SONIC) {
            sound_play(VOICE_SONIC_NEXT_ACT, 0, 0, 0);
        } else {
            sound_play(VOICE_NEXT_ACT, 0, 0, 0);
        }
        break;
    case CHANGE_ACT_WAIT:
        --work->timer;
        if (work->timer == 0) {
            stream_destroy();
            world_change_act(1);
            placement_change_act(1);
            camera_change_act(1);
            stage_change_act(1);
            stage_place_player(player_work[0]);
            task_destroy(task);
        }
        break;
    }
}

void sky_deck_change_act_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case CHANGE_ACT_START:
        if (!placement_editing(task)) {
            work->mode = CHANGE_ACT_WAIT;
        }
        work->timer = 60;
        break;
    case CHANGE_ACT_WAIT:
        --work->timer;
        if (work->timer == 0) {
            stream_destroy();
            world_change_act(1);
            placement_change_act(1);
            camera_change_act(1);
            stage_change_act(1);
            stage_place_player(player_work[0]);
            task_destroy(task);
        }
        break;
    }
}

/* TRUE while player is within the upright cylinder centered on x, y, z. it
 * reaches radius in x and z, and height above and below y, both exclusive */
static Bool grip_reaches(int player, Float x, Float y, Float z, Float radius,
                         Float height)
{
    Float px = player_work[player]->pos.x;
    Float py = player_work[player]->pos.y;
    Float pz = player_work[player]->pos.z;

    if (_builtin_fabsf(py - y) < height) {
        if (vector_length_squared(px - x, 0.0F, pz - z) < radius * radius) {
            return TRUE;
        }
    }
    return FALSE;
}

/* a grip's task->motion */
struct grip_motion {
    struct grip_cylinder *cylinder;
    Uint8 unk_04[48];
    /* the field object/common.h's CARRY_REACH reads: how far from the grip a
     * player going round it is held */
    Float reach;
};

/* a grip's vars. world is MODEL_VARS's, which sky_deck_model_world sets and
 * grip_destroy frees */
struct grip_vars {
    NJS_OBJECT *world;
    Sint16 wait[2]; /* each player's wait to take hold, down to GRIP_READY */
    Uint8 held[2]; /* whether each player holds on */
    Uint8 unk_0a[2];
};

#define GRIP_VARS(work) ((struct grip_vars *)&(work)->vars)

/* a player can take hold once its wait is down to GRIP_READY. one that lets
 * go waits from LET_GO_WAIT, so it can't take hold again for 59 steps */
#define GRIP_READY 1
#define LET_GO_WAIT 60
/* how far past each end of its cylinder a grip reaches */
#define GRIP_SLACK 10.0F
/* the cosine of 60 degrees. a player moving within 60 degrees of straight
 * away from a grip can't take hold */
#define AWAY_COS 0.5F
/* the speed out from the grip and up with which a player that lets go
 * jumps off */
#define JUMP_OUT 1.2F
#define JUMP_UP 2.2F

/* where grip_step finds a player */
enum grip_state {
    GRIP_LEAVING = -1, /* within reach and ready, but moving away */
    GRIP_OUT, /* out of reach, or still waiting */
    GRIP_IN /* within reach and free to take hold */
};

/* TRUE while player is within the grip's cylinder, stretched GRIP_SLACK at
 * each end */
static Bool grip_near(int player, struct task *task)
{
    struct task_work *work = task->work;
    struct grip_motion *motion = (struct grip_motion *)task->motion;
    struct grip_cylinder *cylinder = motion->cylinder;

    return grip_reaches(player, work->pos.x + cylinder->center.x,
                        work->pos.y + cylinder->center.y,
                        work->pos.z + cylinder->center.z, cylinder->radius,
                        cylinder->height + GRIP_SLACK);
}

static void grip_reset(struct task *task)
{
    struct task_work *work = task->work;

    GRIP_VARS(work)->held[0] = 0;
    GRIP_VARS(work)->held[1] = 0;
    GRIP_VARS(work)->wait[0] = GRIP_READY;
    GRIP_VARS(work)->wait[1] = GRIP_READY;
}

/* a player that comes near the grip takes hold and goes round it, unless
 * it is moving away. while carry isn't 0, the grip carries each player that
 * holds on round by its ang.x a step. a player that leaves the grip's reach
 * or stops going round lets go. it then jumps off, out and up */
static void grip_step(struct task *task, int carry)
{
    enum grip_state state[2];
    NJS_VECTOR to_player;
    NJS_VECTOR along;
    struct task_work *work = task->work;
    struct task_work *player;
    Float distance;
    Angle ang;
    Angle heading;
    int i;

    for (i = 0; i < 2; i++) {
        if (player_work[i]) {
            state[i] = GRIP_OUT;
            GRIP_VARS(work)->wait[i]--;
            if (GRIP_VARS(work)->wait[i] == 0) {
                GRIP_VARS(work)->wait[i] = GRIP_READY;
            }
            if (grip_near(i, task) && GRIP_VARS(work)->wait[i] == GRIP_READY) {
                state[i] = GRIP_LEAVING;
                player = player_work[i];
                to_player.x = player->pos.x - work->pos.x;
                to_player.y = 0.0F;
                to_player.z = player->pos.z - work->pos.z;
                njUnitVector(&to_player);
                along.x = player_motion[i]->speed.x;
                along.y = 0.0F;
                along.z = player_motion[i]->speed.z;
                njUnitVector(&along);
                if (njInnerProduct(&along, &to_player) < AWAY_COS) {
                    state[i] = GRIP_IN;
                }
            }
        }
    }
    collision_register(work);
    object_clear_touch(work, NULL);
    for (i = 0; i < 2; i++) {
        if (player_work[i]) {
            if (GRIP_VARS(work)->held[i] && carry) {
                player = player_work[i];
                to_player.x = player->pos.x - work->pos.x;
                to_player.y = 0.0F;
                to_player.z = player->pos.z - work->pos.z;
                distance = njSqrt(vector_length_squared(
                    to_player.x, to_player.y, to_player.z));
                ang = (Angle)(atan2f(-to_player.x, -to_player.z) *
                              ANGLE_PER_RADIAN);
                ang += work->ang.x;
                player_set_angles(i, 0, ang, 0);
                player_set_position(
                    i, (distance * njCos(-0x4000 - ang)) + work->pos.x,
                    player->pos.y,
                    (distance * njSin(-0x4000 - ang)) + work->pos.z);
            }
            if (state[i] == GRIP_OUT || player_get(i)->held != task) {
                if (GRIP_VARS(work)->held[i]) {
                    GRIP_VARS(work)->held[i] = 0;
                    GRIP_VARS(work)->wait[i] = LET_GO_WAIT;
                    player = player_work[i];
                    to_player.x = player->pos.x - work->pos.x;
                    to_player.y = 0.0F;
                    to_player.z = player->pos.z - work->pos.z;
                    njUnitVector(&to_player);
                    heading = (Angle)(atan2f(-to_player.x, -to_player.z) *
                                      ANGLE_PER_RADIAN);
                    player_set_angles(i, 0, heading + 0x8000, 0);
                    /* @hack redundant: a store of i to heading that never
                     * runs. SHC drops it, but it still steers the frame:
                     * heading takes the word at R15+4 and the doubled player
                     * number R15+0, at 0x0c902a5c and 0x0c9029de, which
                     * otherwise trade; without it, 5 of 409 words differ */
                    if (0) {
                        heading = i;
                    }
                    player_set_speed(i, njCos(-0x4000 - heading) * JUMP_OUT,
                                     JUMP_UP,
                                     njSin(-0x4000 - heading) * JUMP_OUT);
                }
            }
            if (state[i] == GRIP_IN && GRIP_VARS(work)->wait[i] == GRIP_READY) {
                if (GRIP_VARS(work)->held[i] == 0) {
                    player_circle(i, task);
                    player = player_work[i];
                    to_player.x = player->pos.x - work->pos.x;
                    to_player.y = 0.0F;
                    to_player.z = player->pos.z - work->pos.z;
                    njUnitVector(&to_player);
                    player_set_angles(
                        i, 0,
                        (Angle)(atan2f(-to_player.x, -to_player.z) *
                                ANGLE_PER_RADIAN),
                        0);
                    GRIP_VARS(work)->held[i] = 1;
                }
            }
        }
    }
}

static void sky_deck_grip_step(struct task *task);
static void grip_destroy(struct task *task);

void sky_deck_grip_start(struct task *task)
{
    struct task_work *work = task->work;
    struct grip_motion *motion = (struct grip_motion *)task->motion;

    collision_init(task, &grip_shape, COLLISION_OBJECT, 1, 1);
    work->collision->radius = 150.0F;
    motion->cylinder = &grip_cylinder;
    task->step = sky_deck_grip_step;
    task->destructor = grip_destroy;
    motion->reach = 105.0F;
    work->ang.x = 0;
    grip_reset(task);
}

static void sky_deck_grip_step(struct task *task)
{
    struct task_work *work = task->work;

    grip_step(task, work->ang.x);
}

static void hangpole_step(struct task *task);

void hangpole_start(struct task *task)
{
    struct grip_motion *motion = (struct grip_motion *)task->motion;

    if (!placement_editing(task)) {
        collision_init(task, &hangpole_shape, COLLISION_OBJECT, 1, 1);
    }
    motion->cylinder = &hangpole_cylinder;
    task->step = hangpole_step;
    task->destructor = grip_destroy;
    motion->reach = 42.0F;
    grip_reset(task);
}

/* @hack redundant: register on task, which keeps it in R14 across
 * sky_deck_try_leave; without it, SHC keeps task in the frame and 10 of 14
 * words differ */
static void hangpole_step(register struct task *task)
{
    if (!sky_deck_try_leave(task)) {
        grip_step(task, 0);
    }
}

static void grip_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (MODEL_VARS(work)->world) {
        world_remove(task, MODEL_VARS(work)->world);
        world_object_free(MODEL_VARS(work)->world);
        MODEL_VARS(work)->world = NULL;
    }
}

static void cyl_s_step(struct task *task);
static void cyl_s_draw(struct task *task);

/* the turn CYL_S takes each step when its set file gives none, about -1
 * degree */
#define CYL_S_TURN (-182)

void cyl_s_start(struct task *task)
{
    struct task_work *work = task->work;
    struct grip_motion *motion = (struct grip_motion *)task->motion;

    if (!placement_editing(task)) {
        collision_init(task, &cyl_s_shape, COLLISION_OBJECT, 1, 1);
    }
    motion->cylinder = &cyl_s_cylinder;
    task->draw = cyl_s_draw;
    task->step = cyl_s_step;
    task->destructor = grip_destroy;
    motion->reach = 32.0F;
    work->submode = 17;
    /* it turns by the set file's ang.z a step, or by CYL_S_TURN when that is
     * 0 */
    if (work->ang.z) {
        work->ang.x = work->ang.z;
        work->ang.z = 0;
    } else {
        work->ang.x = CYL_S_TURN;
    }
    grip_reset(task);
}

static void cyl_s_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!sky_deck_try_leave(task)) {
        grip_step(task, work->ang.x);
        work->ang.y += work->ang.x;
        cyl_s_draw(task);
        sky_deck_model_world(task);
    }
}

static void cyl_s_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = sky_deck_models[work->submode].object;
    Angle ang;

    njSetTexture(&obj_skydeck_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(object);
    njPopMatrix(1);
}

static void tank_h_step(struct task *task);
static void tank_h_draw(struct task *task);

void tank_h_start(struct task *task)
{
    struct task_work *work = task->work;
    struct grip_motion *motion = (struct grip_motion *)task->motion;

    if (!placement_editing(task)) {
        collision_init(task, &tank_h_shape, COLLISION_OBJECT, 1, 1);
    }
    motion->cylinder = &tank_h_cylinder;
    task->draw = tank_h_draw;
    task->step = tank_h_step;
    task->destructor = grip_destroy;
    motion->reach = 26.0F;
    work->submode = 46;
    grip_reset(task);
}

static void tank_h_step(struct task *task)
{
    if (!sky_deck_try_leave(task)) {
        grip_step(task, 0);
        tank_h_draw(task);
        sky_deck_model_world(task);
    }
}

static void tank_h_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = sky_deck_models[work->submode].object;
    NJS_MOTION *motion = sky_deck_motions[6];

    njSetTexture(&obj_skydeck_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    work->scl.z += 1.0F;
    if (work->scl.z >= sky_deck_motion_frames[6]) {
        work->scl.z = 0.0F;
    }
    draw_motion(object, motion, work->scl.z);
    njPopMatrix(1);
}

static void top_0_step(struct task *task);
static void top_0_draw(struct task *task);

void top_0_start(struct task *task)
{
    struct task_work *work = task->work;
    struct grip_motion *motion = (struct grip_motion *)task->motion;

    if (!placement_editing(task)) {
        collision_init(task, &top_0_shape, COLLISION_OBJECT, 1, 1);
    }
    /* @bug memory: in the set editor, collision_init isn't called, so the work
     * has no collision record. this then stores through its null collision,
     * at 0x0c902f54 */
    work->collision->radius = 150.0F;
    motion->cylinder = &top_0_cylinder;
    task->draw = top_0_draw;
    task->step = top_0_step;
    task->destructor = grip_destroy;
    motion->reach = 105.0F;
    work->submode = 9;
    work->ang.x = 0;
    grip_reset(task);
}

static void top_0_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!sky_deck_try_leave(task) && (world_land_blocks & FIRST_BLOCK)) {
        work->ang.y += work->ang.x;
        grip_step(task, work->ang.x);
        top_0_draw(task);
        sky_deck_model_world(task);
    }
}

static void top_0_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = sky_deck_models[work->submode].object;
    Angle ang;

    njSetTexture(&obj_skydeck_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(object);
    njPopMatrix(1);
}

/* the path owners, whose paths players hang from or hold on to. each keeps
 * a path and its points from heap_alloc in PATH_OWNER(work), and its
 * destructor frees both */

static void radder_step(struct task *task);
static void radder_draw(struct task *task);
static void radder_destroy(struct task *task);

/* RADDER's ladder. outside the set editor, its child holds a ledge path up
 * the ladder, 31 points 3 apart */
void radder_start(struct task *task)
{
    NJS_POINT3 out;
    NJS_POINT3 offset;
    struct task_work *work = task->work;
    struct task *child;
    struct path_point *points;
    struct path *path;
    Angle ang;
    int count;
    int i;

    task->step = radder_step;
    task->draw = radder_draw;
    task->destructor = radder_destroy;
    work->submode = 59;
    if (!placement_editing(task)) {
        child = task_create_child(TASK_WORK, path_ledge_step, task);
        PATH_VARS(child->work)->stage_act = stage_get_stage_act();
        child->work->ang.x = 0;
        child->work->ang.y = work->ang.y + 0x4000;
        child->work->ang.z = work->ang.z;
        count = 31;
        offset.x = -5.45F;
        offset.y = -90.05F;
        offset.z = 0.0F;
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        points = heap_alloc(count * sizeof(struct path_point));
        PATH_OWNER(work)->points = points;
        for (i = 0; i < count; i++) {
            offset.y += 3.0F;
            points[i].ang_x = work->ang.x;
            points[i].ang_z = work->ang.z;
            points[i].length = 3.0F;
            njCalcPoint(NULL, &offset, &out);
            points[i].pos.x = work->pos.x + out.x;
            points[i].pos.y = work->pos.y + out.y;
            points[i].pos.z = work->pos.z + out.z;
        }
        njPopMatrix(1);
        path = heap_alloc(sizeof(struct path));
        PATH_OWNER(work)->path = path;
        path->ends = PATH_STOP;
        path->count = count;
        path->length = 90.0F;
        path->points = points;
        path->step = path_ledge_step;
        PATH_VARS(child->work)->path = path;
    }
}

static void radder_step(struct task *task)
{
    if (!sky_deck_try_leave(task)) {
        radder_draw(task);
        task_step_children(task);
    }
}

static void radder_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = sky_deck_models[work->submode].object;
    Angle ang;

    njSetTexture(&obj_skydeck_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(object);
    njPopMatrix(1);
}

static void radder_destroy(struct task *task)
{
    struct task_work *work = task->work;

    task_destroy_children(task);
    heap_free(PATH_OWNER(work)->path);
    heap_free(PATH_OWNER(work)->points);
}

static void duct_step(struct task *task);
static void duct_draw(struct task *task);
static void duct_destroy(struct task *task);

/* DUCT_0's duct and its collision record. outside the set editor, its
 * child holds a ledge path of 2 points 3 apart */
void duct_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    struct path_point *points;
    struct path *path;
    NJS_POINT3 out;
    NJS_POINT3 offset;
    Angle ang;
    int count;
    int i;

    task->step = duct_step;
    task->draw = duct_draw;
    task->destructor = duct_destroy;
    work->submode = 45;
    collision_init(task, &duct_shape, COLLISION_OBJECT, 1, 1);
    if (placement_editing(task)) {
        return;
    }
    child = task_create_child(TASK_WORK, path_ledge_step, task);
    PATH_VARS(child->work)->stage_act = stage_get_stage_act();
    child->work->ang.x = 0;
    child->work->ang.y = work->ang.y;
    child->work->ang.z = work->ang.z + 0x4000;
    count = 2;
    offset.x = 0.0F;
    offset.y = -3.0F;
    offset.z = -8.0F;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    TURN_VIA(njRotateZ, work->ang.z + 0x4000, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    points = heap_alloc(count * sizeof(struct path_point));
    PATH_OWNER(work)->points = points;
    for (i = 0; i < count; i++) {
        points[i].ang_x = work->ang.x;
        points[i].ang_z = work->ang.z;
        points[i].length = 3.0F;
        njCalcPoint(NULL, &offset, &out);
        points[i].pos.x = work->pos.x + out.x;
        points[i].pos.y = work->pos.y + out.y;
        points[i].pos.z = work->pos.z + out.z;
        offset.y += 3.0F;
    }
    njPopMatrix(1);
    path = heap_alloc(sizeof(struct path));
    PATH_OWNER(work)->path = path;
    path->ends = PATH_STOP;
    path->count = count;
    path->length = 3.0F;
    path->points = points;
    path->step = path_ledge_step;
    PATH_VARS(child->work)->path = path;
}

/* once the collapse reaches the duct, it falls away with the land. leaving
 * range after that ends its placement. it is a target players can aim at
 * while player 0 is within 60 */
static void duct_step(struct task *task)
{
    struct task_work *work = task->work;

    if (sky_deck_try_leave(task)) {
        if (collapse_reached(work->pos.x, work->pos.y, work->pos.z)) {
            placement_kill(task);
        }
    } else {
        if (collapse_reached(work->pos.x, work->pos.y, work->pos.z)) {
            work->ang.z += PIECE_TURN;
            work->pos.y -= PIECE_SINK;
            work->pos.x += PIECE_DRIFT;
        }
        if (player_in_range(&work->pos, 60.0F) == 1) {
            work->collision->flags |= COLLISION_TARGET;
        } else {
            work->collision->flags &= ~COLLISION_TARGET;
        }
        duct_draw(task);
        task_step_children(task);
        collision_register(work);
    }
}

static void duct_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = sky_deck_models[work->submode].object;

    njSetTexture(&obj_skydeck_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateY, work->ang.y);
    draw_tree(object);
    njPopMatrix(1);
}

static void duct_destroy(struct task *task)
{
    struct task_work *work = task->work;

    task_destroy_children(task);
    heap_free(PATH_OWNER(work)->path);
    heap_free(PATH_OWNER(work)->points);
}

static void bars_step(struct task *task);
static void bars_draw(struct task *task);
static void bars_destroy(struct task *task);

/* UNTEI's bars. outside the set editor, its child holds a path to hang
 * from, 35 points 6 apart */
void untei_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    struct path_point *points;
    struct path *path;
    NJS_POINT3 out;
    NJS_POINT3 offset;
    int i;
    int count;

    task->step = bars_step;
    task->draw = bars_draw;
    task->destructor = bars_destroy;
    work->submode = 69;
    if (!placement_editing(task)) {
        child = task_create_child(TASK_WORK, path_hang_step, task);
        PATH_VARS(child->work)->stage_act = stage_get_stage_act();
        count = 35;
        offset.x = 0.0F;
        offset.y = 0.0F;
        offset.z = -99.0F;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, work->ang.y);
        points = heap_alloc(count * sizeof(*points));
        PATH_OWNER(work)->points = points;
        for (i = 0; i < count; i++) {
            points[i].ang_x = work->ang.x;
            points[i].ang_z = work->ang.z;
            points[i].length = 6.0F;
            njCalcPoint(NULL, &offset, &out);
            points[i].pos.x = work->pos.x + out.x;
            points[i].pos.y = work->pos.y + out.y;
            points[i].pos.z = work->pos.z + out.z;
            offset.z += 6.0F;
        }
        njPopMatrix(1);
        path = heap_alloc(sizeof(*path));
        PATH_OWNER(work)->path = path;
        path->ends = PATH_STOP;
        path->count = count;
        path->length = 204.0F;
        path->points = points;
        path->step = path_hang_step;
        PATH_VARS(child->work)->path = path;
    }
}

static void bars_step(struct task *task)
{
    if (!sky_deck_try_leave(task)) {
        bars_draw(task);
        task_step_children(task);
    }
}

static void bars_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = sky_deck_models[work->submode].object;

    njSetTexture(&obj_skydeck_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_tree(object);
    if ((Uint8)stage_get_stage_act() == 1) {
        draw_tree(&dat_0ca27350);
    }
    njPopMatrix(1);
}

static void bars_destroy(struct task *task)
{
    struct task_work *work = task->work;

    task_destroy_children(task);
    heap_free(PATH_OWNER(work)->path);
    heap_free(PATH_OWNER(work)->points);
}

/* give a fence a child with a path to hang from along its bars. the path
 * has count points, spacing apart, from x - count * spacing / 2 along x. y
 * and z place it off the fence, and the fence's ang.y turns it */
static void bars_lay(struct task *task, Float x, Float y, Float z, int count,
                     Float spacing)
{
    struct task_work *work = task->work;
    struct task *child;
    struct path_point *points;
    struct path *path;
    NJS_POINT3 out;
    NJS_POINT3 offset;
    int i;

    child = task_create_child(TASK_WORK, path_hang_step, task);
    PATH_VARS(child->work)->stage_act = stage_get_stage_act();
    offset.z = z;
    offset.y = y;
    offset.x = x - (count * spacing * 0.5F);
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateY, work->ang.y);
    points = heap_alloc(count * sizeof(*points));
    PATH_OWNER(work)->points = points;
    for (i = 0; i < count; i++) {
        points[i].ang_x = work->ang.x;
        points[i].ang_z = work->ang.z - 0x4000;
        points[i].length = spacing;
        njCalcPoint(NULL, &offset, &out);
        points[i].pos.x = work->pos.x + out.x;
        points[i].pos.y = work->pos.y + out.y;
        points[i].pos.z = work->pos.z + out.z;
        offset.x += spacing;
    }
    njPopMatrix(1);
    path = heap_alloc(sizeof(*path));
    PATH_OWNER(work)->path = path;
    path->ends = PATH_STOP;
    path->count = count;
    path->length = (count - 1) * spacing;
    path->points = points;
    path->step = path_hang_step;
    PATH_VARS(child->work)->path = path;
}

static void fence_step(struct task *task);

void fence_l_start(struct task *task)
{
    struct task_work *work = task->work;

    task->step = fence_step;
    task->draw = bars_draw;
    task->destructor = bars_destroy;
    work->submode = 38;
    bars_lay(task, 3.0F, 8.0F, 0.0F, 41, 6.0F);
    collision_init(task, &fence_l_shape, COLLISION_OBJECT, 1, 1);
}

void fence_m_start(struct task *task)
{
    struct task_work *work = task->work;

    task->step = fence_step;
    task->draw = bars_draw;
    task->destructor = bars_destroy;
    work->submode = 39;
    bars_lay(task, 3.0F, 8.0F, 0.0F, 31, 6.0F);
    collision_init(task, &fence_m_shape, COLLISION_OBJECT, 1, 1);
}

void fence_s_start(struct task *task)
{
    struct task_work *work = task->work;

    task->step = fence_step;
    task->draw = bars_draw;
    task->destructor = bars_destroy;
    work->submode = 40;
    bars_lay(task, 0.0F, 8.0F, 0.0F, 18, 6.0F);
    collision_init(task, &fence_s_shape, COLLISION_OBJECT, 1, 1);
}

/* how far the deck rolls either way before the fences turn to bars */
#define HANG_ROLL 0x3000

/* while the deck rolls less than HANG_ROLL either way, the fence is solid.
 * past that, it is bars that players hang from */
static void fence_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle roll;

    if (!sky_deck_try_leave(task)) {
        bars_draw(task);
        /* @hack spelling: the roll read through a local, which SHC loads as
         * a word and narrows, with the MOV.L and EXTS.W at 0x0c903bfe; read
         * in place, it loads the low half alone, and 24 of 38 words differ */
        roll = sky_deck_roll;
        /* sky_deck_tilt leaves the roll from 0 to 0xffff, which the casts
         * make signed */
        if ((Sint16)roll < HANG_ROLL && (Sint16)roll > -HANG_ROLL) {
            collision_register(work);
        } else {
            task_step_children(task);
        }
    }
}

/* HANG_A's work->mode: its arm up, or swinging down */
enum hang_a_mode { HANG_UP, HANG_SWING };

/* HANG_A's path has ARM_POINTS, ARM_GAP apart along the arm. it keeps
 * SWUNG_POINTS while hang_a_down is set */
#define ARM_POINTS 49
#define SWUNG_POINTS 39
#define ARM_GAP 6.0F
/* while HANG_A's ang.z is below LEDGE_TILT, its path is a ledge players
 * hold on to. otherwise it is bars they hang from */
#define LEDGE_TILT (-8192)
/* the angle at which the swung arm comes to rest */
#define ARM_REST (-15872)
/* the arm's swing speed, its scl.x, starts at SWING_START. each step it
 * gains SWING_PULL toward ARM_REST. a swing away from rest loses SWING_DRAG
 * more, by tests whose bounds lie ARM_NEAR either way of rest */
#define SWING_START 136.0F
#define SWING_PULL 32.0F
#define SWING_DRAG 16.0F
#define ARM_NEAR 0x800

static void hang_a_lay(struct task *task);
static void hang_a_step(struct task *task);
static void hang_a_draw(struct task *task);
static void hang_a_destroy(struct task *task);

void hang_a_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    struct path_point *points;
    struct path *path;

    task->step = hang_a_step;
    task->draw = hang_a_draw;
    task->destructor = hang_a_destroy;
    work->submode = 69;
    work->mode = HANG_UP;
    if (!placement_editing(task)) {
        child = task_create_child(TASK_WORK, path_hang_step, task);
        PATH_VARS(child->work)->stage_act = stage_get_stage_act();
        child->work->ang.x = 0;
        child->work->ang.y = work->ang.y;
        child->work->ang.z = 0;
        hang_a_down = FALSE;
        points = heap_alloc(ARM_POINTS * sizeof(struct path_point));
        PATH_OWNER(work)->points = points;
        path = heap_alloc(sizeof(struct path));
        PATH_OWNER(work)->path = path;
        PATH_VARS(child->work)->path = path;
        hang_a_lay(task);
    }
}

/* lay HANG_A's path along its arm, tilted by ang.z and turned by ang.y.
 * player 0, if on the path, switches between hanging and holding on as the
 * tilt crosses LEDGE_TILT */
static void hang_a_lay(struct task *task)
{
    struct task_work *work = task->work;
    struct path_point *points = PATH_OWNER(work)->points;
    struct path *path = PATH_OWNER(work)->path;
    struct task_work *player = player_work[0];
    int count;
    NJS_POINT3 out;
    NJS_POINT3 offset;
    int i;

    if (hang_a_down) {
        count = SWUNG_POINTS;
    } else {
        count = ARM_POINTS;
    }
    offset.x = 0.0F;
    offset.y = 0.0F;
    offset.z = 129.0F;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateX, work->ang.z);
    TURN(njRotateY, work->ang.y);
    for (i = 0; i < count; i++) {
        points[i].ang_x = 0;
        points[i].ang_z = 0;
        points[i].length = ARM_GAP;
        njCalcPoint(NULL, &offset, &out);
        points[i].pos.x = work->pos.x + out.x;
        points[i].pos.y = work->pos.y + out.y;
        points[i].pos.z = work->pos.z + out.z;
        offset.z -= ARM_GAP;
    }
    njPopMatrix(1);
    path->ends = PATH_STOP;
    path->count = count;
    path->length = (count - 1) * ARM_GAP;
    path->points = points;
    if (work->ang.z < LEDGE_TILT) {
        path->step = path_ledge_step;
        if (PLAYER_VARS(player)->arg.path == PATH_OWNER(work)->path) {
            player->submode = PLAYER_MODE_LEDGE;
        }
    } else {
        path->step = path_hang_step;
        if (PLAYER_VARS(player)->arg.path == PATH_OWNER(work)->path) {
            player->submode = PLAYER_MODE_HANG;
        }
    }
}

/* while player 0 is less than 200 past HANG_A along z, step the arm and its
 * path. the arm swings down once Sonic is on its path, past 90 short of the
 * arm along z. it then rocks to rest at ARM_REST. the path tilts with the
 * arm. it lies level while player 0 is past the arm along z, no lower than
 * 10 below it */
static void hang_a_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player;

    if (sky_deck_try_leave(task)) {
        return;
    }
    if (player_work[0]->pos.z < work->pos.z + 200.0F) {
        switch (work->mode) {
        case HANG_UP:
            if (get_character() == CHARACTER_SONIC) {
                player = player_work[0];
                if (PLAYER_VARS(player)->arg.path == PATH_OWNER(work)->path &&
                    player->pos.z > work->pos.z - 90.0F) {
                    work->mode = HANG_SWING;
                    work->scl.x = SWING_START;
                    sound_play(SE_SWING, 0, 0, 0);
                    hang_a_down = TRUE;
                    /* only Sonic gets here, so the second voice never
                     * plays */
                    if (get_character() == CHARACTER_SONIC) {
                        sound_play(VOICE_SONIC_SWING, 0, 0, 0);
                    } else {
                        sound_play(VOICE_SWING, 0, 0, 0);
                    }
                }
            }
            break;
        case HANG_SWING:
            work->ang.x += work->scl.x;
            if (work->ang.x < ARM_REST) {
                work->scl.x += SWING_PULL;
                /* @bug logic: below ARM_REST, ang.x is always below this
                 * bound, ARM_REST + ARM_NEAR (-13824, the MOV.W at
                 * 0x0c903fe4), so the test of ang.x always holds. the other
                 * arm's bound is ARM_REST - ARM_NEAR, and the two look
                 * swapped: swapped back, each test would hold only past
                 * ARM_NEAR from rest. as shipped, the arm loses SWING_DRAG
                 * more on every step it swings away from rest */
                if (work->ang.x < ARM_REST + ARM_NEAR && work->scl.x < 0.0F) {
                    work->scl.x += SWING_DRAG;
                }
            } else {
                work->scl.x -= SWING_PULL;
                /* @bug logic: from ARM_REST up, ang.x is always above this
                 * bound, ARM_REST - ARM_NEAR (-17920, the MOV.W at
                 * 0x0c904006), so the test of ang.x always holds, as the one
                 * above does */
                if (work->ang.x > ARM_REST - ARM_NEAR && work->scl.x > 0.0F) {
                    work->scl.x -= SWING_DRAG;
                }
            }
            if (player_work[0]->pos.y >= work->pos.y - 10.0F &&
                work->pos.z <= player_work[0]->pos.z) {
                work->ang.z = 0;
            } else {
                work->ang.z = work->ang.x;
            }
            hang_a_lay(task);
            break;
        }
        task_step_children(task);
    }
    hang_a_draw(task);
}

/* draw HANG_A's models: row 21's, row 19's turned by the swing, and row
 * 20's 108 back along z */
static void hang_a_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    njSetTexture(&obj_skydeck_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    object = sky_deck_models[21].object;
    draw_tree(object);
    njPushMatrix(NULL);
    TURN(njRotateX, work->ang.x);
    object = sky_deck_models[19].object;
    draw_tree(object);
    njPopMatrix(1);
    njTranslate(NULL, 0.0F, 0.0F, -108.0F);
    object = sky_deck_models[20].object;
    draw_tree(object);
    /* @hack redundant: a goto to the next statement, which SHC drops but which
     * still gives njPopMatrix's address R2 at 0x0c904104; without it, the
     * address takes R3 and 2 of 66 words differ */
    goto pop;
pop:
    njPopMatrix(1);
}

static void hang_a_destroy(struct task *task)
{
    struct task_work *work = task->work;

    task_destroy_children(task);
    heap_free(PATH_OWNER(work)->path);
    heap_free(PATH_OWNER(work)->points);
}

static void hang_c_step(struct task *task);
static void hang_c_lay(struct task *task);

void hang_b_start(struct task *task)
{
}

void hang_c_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    struct path_point *points;
    struct path *path;

    task->step = hang_c_step;
    task->destructor = hang_a_destroy;
    if (placement_editing(task)) {
        return;
    }
    child = task_create_child(TASK_WORK, path_hang_step, task);
    PATH_VARS(child->work)->stage_act = stage_get_stage_act();
    child->work->ang.x = 0;
    child->work->ang.y = work->ang.y;
    child->work->ang.z = 0;
    hang_a_down = FALSE;
    points = heap_alloc(8 * sizeof(*points));
    PATH_OWNER(work)->points = points;
    path = heap_alloc(sizeof(*path));
    PATH_OWNER(work)->path = path;
    PATH_VARS(child->work)->path = path;
    hang_c_lay(task);
}

/* @hack redundant: register on task, which keeps it in R14 across
 * sky_deck_try_leave; without it, SHC keeps task in the frame and 10 of 19
 * words differ */
static void hang_c_step(register struct task *task)
{
    if (!sky_deck_try_leave(task) && hang_a_down) {
        task_step_children(task);
    }
}

static void hang_c_lay(struct task *task)
{
    struct task_work *work = task->work;
    struct path_point *points = PATH_OWNER(work)->points;
    struct path *path = PATH_OWNER(work)->path;
    struct path_point *point;
    NJS_POINT3 out;
    NJS_POINT3 offset;
    int i;
    int count;

    count = 8;
    offset.x = 0.0F;
    offset.y = 0.0F;
    offset.z = -117.0F;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateX, work->ang.z);
    TURN(njRotateY, work->ang.y);
    /* @hack spelling: point's step written out, which gives the count,
     * points and i R9, R10 and R8 from 0x0c9042a0; as point++, they take R8,
     * R9 and R10, and 7 of 113 words differ */
    for (i = 0, point = points; i < count; i++, point = point + 1) {
        point->ang_x = 0;
        point->ang_z = 0;
        point->length = 6.0F;
        njCalcPoint(NULL, &offset, &out);
        point->pos.x = work->pos.x + out.x;
        point->pos.y = work->pos.y + out.y;
        point->pos.z = work->pos.z + out.z;
        offset.z -= 6.0F;
    }
    njPopMatrix(1);
    path->ends = PATH_STOP;
    path->count = count;
    path->length = 42.0F;
    path->points = points;
    path->step = path_hang_step;
}

void trails_reset(void)
{
    int i;

    for (i = 0; i < TRAILS; i++) {
        trail_life[i] = -1;
    }
    for (i = 0; i < TRAIL_VOLUMES; i++) {
        trail_volumes[i].kind = -1;
    }
}

/* the vars of TRAIL and of trail_camera_step */
struct trail_vars {
    Uint8 unk_00[4];
    Sint32 wait; /* the countdown to the task's next trail */
    Uint8 unk_08[4];
};

#define TRAIL_VARS(work) ((struct trail_vars *)&(work)->vars)

void trail_start(struct task *task)
{
    struct task_work *work = task->work;

    TRAIL_VARS(work)->wait = 30.0F + (rand() * (30.0F * RAND_SCALE));
    task->step = trail_step;
    task->destructor = trail_destroy;
}

void trail_step(struct task *task)
{
    struct task_work *work = task->work;
    int trail;
    int i;
    struct trail_point *point;
    Float speed;

    if (placement_try_leave(task)) {
        return;
    }
    TRAIL_VARS(work)->wait--;
    if (TRAIL_VARS(work)->wait >= 0) {
        return;
    }
    TRAIL_VARS(work)->wait = 240.0F + (rand() * (30.0F * RAND_SCALE));
    trail = -1;
    for (i = 0; i < TRAILS; i++) {
        if (trail_life[i] < 0) {
            trail = i;
            break;
        }
    }
    if (trail >= 0) {
        trail_life[trail] = TRAIL_LIFE;
        /* @hack spelling: TRAIL_SPEED through a local, which pools the 3 at
         * 0x0c904518, ahead of trail_points' address; written in place, the
         * two pool words trade, and 2 of 92 words and 4 pool words differ */
        speed = TRAIL_SPEED;
        for (i = 0; i < TRAIL_POINTS; i++) {
            point = &trail_points[trail][i];
            point->pos.x = work->pos.x;
            point->pos.y = work->pos.y;
            point->pos.z = work->pos.z;
            point->speed.y = 0.0F;
            point->speed.z = 0.0F;
            point->speed.x = speed;
            point->wait = i * TRAIL_POINT_WAIT;
        }
    }
}

/* trail_camera_step starts a trail UPWIND short of the camera along x. it
 * starts within SPREAD of the camera along y and z */
#define UPWIND 200.0F
#define SPREAD 100.0F

enum trail_camera_mode { TRAIL_CAMERA_START, TRAIL_CAMERA_RUN };

void trail_camera_step(struct task *task)
{
    struct task_work *work = task->work;
    struct trail_point *point;
    int trail;
    int i;
    int found;
    Float x;
    Float y;
    Float z;
    Float speed;

    switch (work->submode) {
    case TRAIL_CAMERA_START:
        TRAIL_VARS(work)->wait = (rand() * (30.0F * RAND_SCALE)) + 30.0F;
        work->submode = TRAIL_CAMERA_RUN;
        break;
    case TRAIL_CAMERA_RUN:
        TRAIL_VARS(work)->wait--;
        if (TRAIL_VARS(work)->wait < 0) {
            TRAIL_VARS(work)->wait = rand() * (20.0F * RAND_SCALE);
            found = -1;
            for (trail = 0; trail < TRAILS; trail++) {
                if (trail_life[trail] < 0) {
                    found = trail;
                    break;
                }
            }
            if (found >= 0) {
                trail_life[found] = TRAIL_LIFE;
                /* @hack spelling: TRAIL_SPEED through a local, which SHC
                 * keeps in FR15 across the rand calls, from 0x0c9045d6;
                 * written in place, 130 of 148 words differ */
                speed = TRAIL_SPEED;
                x = camera_work->pos.x - UPWIND;
                y = camera_work->pos.y +
                    (rand() * (2.0F * SPREAD * RAND_SCALE)) - SPREAD;
                z = camera_work->pos.z +
                    (rand() * (2.0F * SPREAD * RAND_SCALE)) - SPREAD;
                for (i = 0; i < TRAIL_POINTS; i++) {
                    point = &trail_points[found][i];
                    point->pos.x = x;
                    point->pos.y = y;
                    point->pos.z = z;
                    /* @hack spelling: the stores chained, which gives the
                     * search's bound R5 and trail_life's address R6, at
                     * 0x0c90457a; apart, the two trade and 9 of 148 words
                     * differ */
                    point->speed.z = point->speed.y = 0.0F;
                    point->speed.x = speed;
                    point->wait = i * TRAIL_POINT_WAIT;
                }
            }
        }
        break;
    }
}

/* each move, a point's speed along x grows by BLOW, up to BLOW_MOST. it
 * keeps ACROSS_KEEP of its speed along y and z */
#define BLOW 0.2F
#define BLOW_MOST 6.0F
#define ACROSS_KEEP 0.9F
/* a volume turns a point out from its center along an axis. the turn is
 * TURN_MOST less the point's distance from the center along that axis, in
 * radii. the radius times TURN_SCALE scales it */
#define TURN_MOST 1.2F
#define TURN_SCALE 0.02F
/* a volume also slows a point along x. it takes SLOW_SHARE of the point's
 * speed for each radius the point is short of the center */
#define SLOW_SHARE 0.6F

/* a point within a volume's reach turns out from its center. TRAIL_UPRIGHT
 * turns it along z, TRAIL_LYING along y, and TRAIL_BALL along both. a
 * cylinder acts only on points inside it and short of its center along x.
 * a ball speeds up the points past its center, as it slows those short of
 * it */
void trails_move(void)
{
    int trail;
    int i;
    int k;
    struct trail_point *point;
    struct trail_volume *volume;
    Float x;
    Float y;
    Float z;
    Float dx;
    Float dy;
    Float dz;
    Float across;
    Float in;

    for (trail = 0; trail < TRAILS; trail++) {
        if (trail_life[trail] >= 0) {
            trail_life[trail]--;
            for (i = 0; i < TRAIL_POINTS; i++) {
                point = &trail_points[trail][i];
                if (point->wait > 0) {
                    point->wait--;
                } else {
                    point->speed.x += BLOW;
                    if (point->speed.x > BLOW_MOST) {
                        point->speed.x = BLOW_MOST;
                    }
                    point->speed.z *= ACROSS_KEEP;
                    point->speed.y *= ACROSS_KEEP;
                    x = point->pos.x;
                    y = point->pos.y;
                    z = point->pos.z;
                    for (k = 0; k < TRAIL_VOLUMES; k++) {
                        if (trail_volumes[k].kind >= 0) {
                            volume = &trail_volumes[k];
                            dx = x - volume->center.x;
                            dy = y - volume->center.y;
                            dz = z - volume->center.z;
                            in = volume->reach;
                            in -= (dx * dx) + (dy * dy) + (dz * dz);
                            if (in > 0.0F) {
                                in = -dx * SLOW_SHARE;
                                switch (volume->kind) {
                                case TRAIL_UPRIGHT:
                                    across = (dx * dx) + (dz * dz);
                                    if (dx < 0.0F &&
                                        njAbs(dy) < volume->height &&
                                        across <
                                            volume->radius * volume->radius) {
                                        dy = TURN_MOST;
                                        if (dz < 0.0F) {
                                            dy = -TURN_MOST;
                                        }
                                        dy = (dy - (dz / volume->radius)) *
                                             volume->radius * TURN_SCALE;
                                        point->speed.z += dy;
                                        dy = in / volume->radius *
                                             point->speed.x;
                                        point->speed.x -= dy;
                                    }
                                    break;
                                case TRAIL_LYING:
                                    across = (dx * dx) + (dy * dy);
                                    if (dx < 0.0F &&
                                        njAbs(dz) < volume->height &&
                                        across <
                                            volume->radius * volume->radius) {
                                        dz = TURN_MOST;
                                        if (dy < 0.0F) {
                                            dz = -TURN_MOST;
                                        }
                                        dy = (dz - (dy / volume->radius)) *
                                             volume->radius * TURN_SCALE;
                                        point->speed.y += dy;
                                        dy = in / volume->radius *
                                             point->speed.x;
                                        point->speed.x -= dy;
                                    }
                                    break;
                                case TRAIL_BALL:
                                    dx = TURN_MOST;
                                    if (dz < 0.0F) {
                                        dx = -TURN_MOST;
                                    }
                                    dz = (dx - (dz / volume->radius)) *
                                         volume->radius * TURN_SCALE;
                                    point->speed.z += dz;
                                    dz = TURN_MOST;
                                    if (dy < 0.0F) {
                                        dz = -TURN_MOST;
                                    }
                                    dy = (dz - (dy / volume->radius)) *
                                         volume->radius * TURN_SCALE;
                                    point->speed.y += dy;
                                    dy = in / volume->radius * point->speed.x;
                                    point->speed.x -= dy;
                                    break;
                                }
                            }
                        }
                    }
                    point->pos.x += point->speed.x;
                    point->pos.y += point->speed.y;
                    point->pos.z += point->speed.z;
                }
            }
        }
    }
}

void trails_draw(void)
{
    int trail;
    int i;
    struct trail_point *point;

    trail_line.p = trail_line_points;
    trail_line.col = trail_line_colors;
    trail_line.tex = NULL;
    trail_line.num = TRAIL_POINTS;
    for (trail = 0; trail < TRAILS; trail++) {
        if (trail_life[trail] >= 0) {
            for (i = 0; i < TRAIL_POINTS; i++) {
                point = &trail_points[trail][i];
                trail_line_colors[i].argb.a = 0xff;
                trail_line_colors[i].argb.r = 0xff;
                trail_line_colors[i].argb.g = 0xff;
                trail_line_colors[i].argb.b = 0xff;
                trail_line_points[i].x = point->pos.x;
                trail_line_points[i].y = point->pos.y;
                trail_line_points[i].z = point->pos.z;
            }
            njDrawLine3D(&trail_line, TRAIL_POINTS, NJD_DRAW_CONNECTED);
        }
    }
}

void t_colli_start(struct task *task)
{
    task->step = t_colli_step;
    task->draw = t_colli_draw;
    task->destructor = t_colli_destroy;
}

/* what trail_volume_set adds to the radius it is given */
#define VOLUME_MARGIN 10.0F

/* the set file's scale x gives the kind, y the radius and z the height */
void t_colli_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        trail_volume_set(work, (int)work->scl.x, FALSE, work->scl.y,
                         work->scl.z, work->pos.x, work->pos.y, work->pos.z);
        t_colli_draw(task);
    }
}

/* in the set editor, draw the task's volume as a marker */
void t_colli_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct trail_volume *volume;
    int i;
    int found;
    Float radius;
    Float height;

    if (!edit_mode) {
        return;
    }
    found = -1;
    for (i = 0; i < TRAIL_VOLUMES; i++) {
        if (trail_volumes[i].owner == work) {
            found = i;
            break;
        }
    }
    if (found < 0) {
        return;
    }
    volume = &trail_volumes[found];
    radius = volume->radius;
    height = volume->height;
    njPushMatrix(NULL);
    njTranslate(NULL, volume->center.x, volume->center.y, volume->center.z);
    switch (volume->kind) {
    case TRAIL_UPRIGHT:
        njScale(NULL, radius * MARKER_FIT, height * MARKER_FIT,
                radius * MARKER_FIT);
        draw_tree(&marker_cylinder_object);
        break;
    case TRAIL_LYING:
        njRotateX(NULL, 0x4000);
        njScale(NULL, radius * MARKER_FIT, height * MARKER_FIT,
                radius * MARKER_FIT);
        draw_tree(&marker_cylinder_object);
        break;
    case TRAIL_BALL:
        njScale(NULL, radius * MARKER_FIT, radius * MARKER_FIT,
                radius * MARKER_FIT);
        draw_tree(&marker_object);
        break;
    }
    njPopMatrix(1);
}

/* free the volumes the work owns */
void t_colli_destroy(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    for (i = 0; i < TRAIL_VOLUMES; i++) {
        if (trail_volumes[i].owner == work) {
            trail_volumes[i].kind = -1;
        }
    }
}

void trail_volume_set(struct task_work *owner, int kind, Bool force,
                      Float radius, Float height, Float x, Float y, Float z)
{
    struct trail_volume *volume;
    int i;
    int found;

    found = -1;
    for (i = 0; i < TRAIL_VOLUMES; i++) {
        volume = &trail_volumes[i];
        if (volume->kind < 0) {
            found = i;
            break;
        }
        if (volume->owner == owner) {
            found = i;
            break;
        }
    }
    if (found < 0) {
        return;
    }
    volume = &trail_volumes[found];
    if (volume->owner == owner && volume->kind != TRAIL_UPRIGHT && !force) {
        return;
    }
    volume->owner = owner;
    volume->kind = kind;
    volume->center.x = x;
    volume->center.y = y;
    volume->center.z = z;
    volume->radius = radius + VOLUME_MARGIN;
    volume->height = height;
    /* @hack redundant: the radius + 0 in each arm, which keeps the image's
     * FADD ahead of the square; without it, 37 of 89 words differ */
    if (volume->kind == TRAIL_BALL) {
        volume->reach = volume->radius + 0.0F;
        volume->reach *= volume->reach;
    } else {
        volume->reach = volume->radius + 0.0F;
        volume->reach *= volume->reach;
        volume->reach += volume->height * volume->height;
    }
}

/* scroll_draw copies SCROLL_UVS UVs. each call turns the pulse, ang.z, by
 * PULSE_STEP, and takes SCROLL_STEP from the work's timer. the model draws
 * at SCROLL_SCALE times its size */
#define SCROLL_UVS 32
#define PULSE_STEP 364
#define SCROLL_STEP 4
#define SCROLL_SCALE 0.3F

void scroll_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_MODEL *model = scroll_models[work->submode];
    /* @hack type: the image's frame holds 64 UVs, and the copy fills the first
     * SCROLL_UVS. as uv[SCROLL_UVS], the frame takes 128 bytes less, and 0 of
     * 130 words and 2 pool words differ (0x0c904e20, 0x0c904e24) */
    NJS_COLOR uv[64];
    NJS_COLOR *from;
    NJS_COLOR *to = uv;
    NJS_COLOR *kept;
    NJS_ARGB fade;
    int i = 0;
    int tex_u;
    int tex_v;
    int count = SCROLL_UVS;
    Float alpha;

    work->ang.z += PULSE_STEP;
    work->timer -= SCROLL_STEP;
    from = model->meshsets->vertuv;
    /* @bug memory: the copy reads SCROLL_UVS UVs from the model's
     * first meshset (MOV #32,R9 at 0x0c904d20, the CMP/GE at 0x0c904d5a),
     * which holds 22 or 26. it reads the data past them */
#ifdef GCC
    /* repairs the read the tag names: GCC copies the UVs the meshset's
     * strips hold, a UV for each point of each strip */
    count = meshset_count_points(model->meshsets);
#endif
    /* @hack spelling: to, i and count set in their declarations, and the loop
     * a do, which sets the loop up in the image's order; as a for over
     * SCROLL_UVS with to set before it, 128 of 130 words differ */
    do {
        tex_u = (Sint16)from->color / 8;
        tex_v = (Sint16)(from->color >> 16);
        tex_u += work->timer;
        to->color = (tex_u & 0xffff) | (((Uint32)tex_v << 16) & 0xffff0000);
        from++;
        to++;
        i++;
    } while (i < count);
    alpha = njSin(work->ang.z);
    alpha = (alpha * 0.5F) - 0.5F;
    fade.a = alpha;
    fade.r = fade.g = fade.b = 0.0F;
    njSetConstantMaterial(&fade);
    njSetTexture(&obj_skydeck_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njScale(NULL, SCROLL_SCALE, SCROLL_SCALE, SCROLL_SCALE);
    kept = model->meshsets->vertuv;
    model->meshsets->vertuv = uv;
    njSimpleDrawModel(model);
    model->meshsets->vertuv = kept;
    njPopMatrix(1);
    fade.a = fade.r = fade.g = fade.b = 0.0F;
    njSetConstantMaterial(&fade);
}

void trail_destroy(struct task *task)
{
}

/* sky_deck_floor_move's floor heights, and NO_FLOOR for none */
#define LOW_FLOOR (-198.0F)
#define HIGH_FLOOR (-100.0F)
#define NO_FLOOR (-9999.0F)

/* the flaps' rows of sky_deck_models */
#define FLAP01_MODEL 66
#define FLAP02_MODEL 67

#define FLAP_FLAGS (WORLD_TURNED | WORLD_MOVES | WORLD_SOLID)

Bool sky_deck_floor_move(NJS_POINT3 *pos, NJS_VECTOR *speed, struct angles *ang,
                         Float radius)
{
    Float x = pos->x;
    Float y = pos->y;
    Float z = pos->z;
    Bool landed = FALSE;
    Float floor = NO_FLOOR;

    if (x < -550.0F && x > -850.0F && z < 362.0F && z > -454.0F) {
        floor = LOW_FLOOR;
    }
    if (x < 850.0F && x > 0.0F) {
        floor = HIGH_FLOOR;
    }
    x += speed->x;
    y += speed->y;
    z += speed->z;
    if (floor != NO_FLOOR && y - radius < floor) {
        y = radius + floor;
        landed = TRUE;
    }
    pos->x = x;
    pos->y = y;
    pos->z = z;
    return landed;
}

static void flap_add_world(struct task *task);

void flap01_start(struct task *task)
{
    struct task_work *work = task->work;

    task->step = flap_step;
    task->draw = sky_deck_model_draw;
    task->destructor = sky_deck_model_destroy;
    work->submode = FLAP01_MODEL;
    flap_add_world(task);
}

void flap02_start(struct task *task)
{
    struct task_work *work = task->work;

    task->step = flap_step;
    task->draw = sky_deck_model_draw;
    task->destructor = sky_deck_model_destroy;
    work->submode = FLAP02_MODEL;
    flap_add_world(task);
}

/* put the task's row's world shape in the world, where the task stands,
 * with FLAP_FLAGS */
static void flap_add_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *shape = sky_deck_models[work->submode].world;
    NJS_OBJECT *world;

    MODEL_VARS(work)->world = world = world_object_alloc();
    world->evalflags = NJD_EVAL_BREAK;
    world->scl[0] = 1.0F;
    world->scl[1] = 1.0F;
    world->scl[2] = 1.0F;
    world->ang[0] = work->ang.x;
    world->ang[1] = work->ang.y;
    world->ang[2] = work->ang.z;
    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    world->model = shape->model;
    world->child = shape->child;
    world->sibling = shape->sibling;
    world_add(FLAP_FLAGS, task, world);
}

/* a flap swings FLAP_SWING either way, a sixteenth of a turn ahead of the
 * deck's swing */
#define FLAP_SWING 2048.0F
#define FLAP_PHASE 0x1000
/* how far a flap falls each step once the collapse reaches it: down, and
 * along x */
#define FALL_DOWN 1.8F
#define FALL_ACROSS 0.8F
/* the hum each flap plays where it stands */
#define SE_FLAP 166

/* the draft builds by 1 a step while player 0 is low, up to RAMP_FULL. it
 * falls by RAMP_FALL a step while player 0 is high or W_WALL is hit */
#define RAMP_FULL 90
#define RAMP_FALL 4
/* the level is the int ground_below_int gives at a point PROBE_LIFT above
 * player 0. it is 0 instead while that point is less than LEVEL_NEAR above
 * the int. player 0 is low while it is at most LOW_HEIGHT above the level */
#define PROBE_LIFT 2.5F
#define LEVEL_NEAR 200.0F
#define LOW_HEIGHT 40.0F
/* the push is draft_push times (PUSH_LEAST + player 0's height above the
 * level). that height is held from 0 to PUSH_RISE */
#define PUSH_LEAST 100.0F
#define PUSH_RISE 200.0F
/* the draft is strongest while the sky's point is at DRAFT_HEIGHT, and
 * weakens either way of it. at DRAFT_SPAN either way, a third of the lift is
 * left and none of the push. past that, the draft's build starts over on
 * each step */
#define DRAFT_HEIGHT 1400.0F
#define DRAFT_SPAN 200.0F
/* past WEAK_FROM along player 0's z, the lift halves and the push falls to
 * 0.3 of itself */
#define WEAK_FROM (-3100.0F)

enum w_wall_mode { W_WALL_INIT, W_WALL_RUN };

/* a flap's step. it hums, swings with the deck, and falls away with the
 * land once the collapse reaches it. its world shape turns with it */
void flap_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    sound_play_at_for(SE_FLAP, (int)task, 1, 32, work->pos.x, work->pos.y,
                      work->pos.z, 2);
    if (collapse_reached(work->pos.x, work->pos.y, work->pos.z)) {
        work->pos.y -= FALL_DOWN;
        work->pos.x += FALL_ACROSS;
    }
    work->ang.z = njSin(sky_deck_swing + FLAP_PHASE) * FLAP_SWING;
    sky_deck_model_draw(task);
    sky_deck_model_world(task);
    object = MODEL_VARS(work)->world;
    object->ang[2] = work->ang.z;
}

void w_wall_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case W_WALL_INIT:
        /* @hack redundant: the count as 1.0F && 1, which the image loads
         * apart from the mode's 1; as a plain 1, the two share a register and
         * 33 of 41 words differ */
        collision_init(task, &w_wall_shape, COLLISION_OBJECT, 1, 1.0F && 1);
        work->mode = W_WALL_RUN;
        return;
    case W_WALL_RUN:
        if (work->collision->flags & COLLISION_HIT) {
            w_wall_hit = TRUE;
        }
        collision_register(work);
        break;
    default:
        work->mode = W_WALL_INIT;
        return;
    }
}

void draft_reset(void)
{
    draft_ramp = 0;
    draft_lift = 0.0F;
    draft_push = 0.0F;
}

void draft_step(Uint16 act)
{
    Float x = player_work[0]->pos.x;
    Float y = player_work[0]->pos.y + PROBE_LIFT;
    Float z = player_work[0]->pos.z;
    struct angles slope = {0, 0, 0};
    Float strength;
    Float height;
    Float lift;
    Float push;

    if (act != 1) {
        return;
    }
    strength = (sky_deck_sky_pos.y - DRAFT_HEIGHT) * (1.0F / DRAFT_SPAN);
    if (strength < -1.0F) {
        strength = -1.0F;
        draft_ramp = 0;
    }
    if (strength > 1.0F) {
        strength = 1.0F;
        draft_ramp = 0;
    }
    if (strength < 0.0F) {
        strength += 1.0F;
        draft_lift = 0.0005F + (strength * 0.001F);
        /* @hack redundant: the push's add of 0, an FMAC onto the 0 in FR15 at
         * 0x0c9051a4; without it, 135 of 195 words differ */
        draft_push = 0.0F + (strength * 0.005F);
    } else {
        draft_lift = 0.0015F - (strength * 0.001F);
        draft_push = 0.005F - (strength * 0.005F);
    }
    if (player_work[0]->pos.z > WEAK_FROM) {
        draft_lift *= 0.5F;
        draft_push *= 0.3F;
    }
    /* @bug mismatched-call: the draft reads ground_below's height from R0 as an
     * int (LDS R0,FPUL and FLOAT at 0x0c9051e4 and 0x0c9051ea), though
     * ground_below returns it in FR0. the int is whatever R0 holds */
#ifdef GCC
    /* repairs the height the tag names: GCC's draft reads ground_below's
     * Float */
    height = ground_below(x, y, z, &slope);
#else
    height = ground_below_int(x, y, z, &slope);
#endif
    if (y - height < LEVEL_NEAR) {
        height = 0.0F;
    }
    height = player_work[0]->pos.y - height;
    if (height > LOW_HEIGHT || w_wall_hit) {
        draft_ramp -= RAMP_FALL;
        if (draft_ramp < 0) {
            draft_ramp = 0;
        }
    } else {
        draft_ramp++;
        if (draft_ramp > RAMP_FULL) {
            draft_ramp = RAMP_FULL;
        }
    }
    lift = draft_ramp * draft_lift;
    if (height < 0.0F) {
        height = 0.0F;
    }
    if (height > PUSH_RISE) {
        height = PUSH_RISE;
    }
    height += PUSH_LEAST;
    push = 0.0F;
    if (draft_ramp) {
        push = draft_push * height;
    }
    if (player_motion[0]->accel.y < lift) {
        player_motion[0]->accel.y += lift;
    }
    if (player_motion[0]->accel.x < push) {
        player_motion[0]->accel.x += push;
    }
    w_wall_hit = FALSE;
}

void homing_start(struct task *task)
{
    task->step = homing_step;
    task->destructor = homing_destroy;
    collision_init(task, &homing_shape, COLLISION_OBJECT, 1, 1);
}

#define HOMING_NEAR 50.0F

/* HOMING is a target players can aim at while player 0 is within
 * HOMING_NEAR, and only then */
void homing_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (player_in_range(&work->pos, HOMING_NEAR) == 1) {
        work->collision->flags |= COLLISION_TARGET;
    } else {
        work->collision->flags &= ~COLLISION_TARGET;
    }
    collision_register(work);
}

void homing_destroy(struct task *task)
{
}
