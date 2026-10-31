/* the Egg Viper boss. its modes run in the order its scripts list them,
 * and Eggman rises from it as it opens. two needles unfold from it to
 * strike. thirteen segments trail it, every third a joint and the last its
 * tail. its lasers, energy, ball and pillars of fire are here, with the
 * palette's flashes. so are Tikal's hints and the arena's smoke */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_egm3/blast.h"
#include "b_egm3/camera.h"
#include "b_egm3/effects.h"
#include "b_egm3/egg_viper.h"
#include "b_egm3/eggmobile3.h"
#include "b_egm3/floor.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "enemy/enemy.h"
#include "event/script.h"
#include "hud/boss_name.h"
#include "hud/hud.h"
#include "hud/life_gauge.h"
#include "object/tikal.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 4
#define TEXT_2_LENGTH 4
#define TEXT_3_LENGTH 6
#define TEXT_4_LENGTH 6
#define TEXT_5_LENGTH 6
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 6
#define TEXT_8_LENGTH 4
#define TEXT_9_LENGTH 6
#define TEXT_10_LENGTH 5
#define TEXT_11_LENGTH 5
#define TEXT_12_LENGTH 3
#define TEXT_13_LENGTH 52
#define TEXT_14_LENGTH 36
#define TEXT_15_LENGTH 46
#define TEXT_16_LENGTH 52
#define TEXT_17_LENGTH 32
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 8
#define TEXT_20_LENGTH 9
#define TEXT_21_LENGTH 5
#include "b_egm3/egg_viper/text.inc"

#define RECORD(task) ((struct egg_viper_record *)(task)->extra)

/* the boss's modes, in work->mode. camera.c's phases follow them by
 * number, and viper_mode_names spells them */
enum viper_mode {
    VIPER_INIT,
    VIPER_WAIT, /* it waits for the camera's swing, then flies in */
    /* it runs along the arena, arcing up at player 0 as its joints fire */
    VIPER_LASER1,
    VIPER_LASER2, /* it rises between the rows and fires down at player 0 */
    VIPER_LASER3, /* it flies along player 0's row, firing at him */
    /* it opens its back, Eggman rises, and it gathers energy for a ball */
    VIPER_ENERGY,
    VIPER_BOOMP, /* it rises past the far end of player 0's row */
    /* it opens its front with Eggman up, and creeps along the row as its
     * needles strike */
    VIPER_BOOM,
    VIPER_HAKAID, /* it sweeps the other row, breaking its panels */
    VIPER_HAKAI, /* it bursts up through player 0's row, breaking two panels */
    VIPER_BREAK, /* with its health gone, it dives once and falls */
    /* a test mode: it plays its motions in turn and prints each name. no
     * script lists it */
    VIPER_MAX,
    VIPER_MODES
};

/* bits of the boss's record->flags. VIPER_FIRING is set through each of
 * VIPER_LASER1's arcs. a joint fires a laser as it tops its own arc above
 * y 0 */
#define VIPER_FIRING 0x4
#define VIPER_BACK_OPEN 0x8 /* set while the back is open */
#define VIPER_GATHERING 0x10 /* set while the energy gathers */
#define VIPER_FRONT_OPEN 0x20 /* set while the front is open */
#define VIPER_HIT 0x40 /* Eggman's task sets it at a hit on him */
/* nothing sets it. while it is set, the joints' collisions don't register */
#define VIPER_UNK_80 0x80
/* set from the back's opening until the boss is 0.7 along energy_path. it
 * softens the throw a hit on Eggman gives player 0 once Eggman is up. the soft
 * throw has speed 4 along x the way Eggman rose, and 2 up. the hard one has 7.5
 * the opposite way, and 8 up */
#define VIPER_GENTLE_THROW 0x100
/* a hit on Eggman in the energy mode sets it, as does the start of that mode's
 * withdrawal. the withdrawal's end clears it. while it is set, the collisions
 * of the boss, its needles and its joints don't register */
#define VIPER_HELD_OFF 0x200
/* set from the energy's gathering until the withdrawal. while it is set, a
 * segment that has all but stopped hangs back from the one before it */
#define VIPER_SEGMENTS_HANG 0x400
/* each needle's top sets it while player 0 stands on that top, and clears
 * it while he doesn't */
#define VIPER_RIDDEN 0x800
#define VIPER_BREAKING 0x100000 /* set from VIPER_BREAK's start */
#define VIPER_GONE 0x200000 /* the boss has fallen, and draws no more */

/* bits of a needle's record->flags. the boss sets PART_GO to unfold a
 * folded needle, or to make a holding one strike. the needle clears it once
 * it has unfolded or struck. the boss sets PART_RETURN to fold a holding
 * needle, which clears it as it starts to fold */
#define PART_GO 0x1
#define PART_RETURN 0x2
/* parts_fall_step and segments_fall_step set it in a needle's or a
 * segment's record, and the task then falls */
#define FALL 0x100000

/* a needle's modes, in work->mode */
enum part_mode {
    PART_INIT,
    PART_READY,
    /* in its socket, node NODE_PART for needle 0 and the next for needle 1 */
    PART_FOLDED,
    PART_UNFOLD,
    PART_HOLD,
    PART_STRIKE,
    PART_FOLD,
    PART_FALL
};

/* a segment's modes, in work->mode. SEGMENT_FALL starts the fall, and
 * SEGMENT_FALLING runs it */
enum segment_mode {
    SEGMENT_INIT,
    SEGMENT_FOLLOW,
    SEGMENT_FALLING,
    SEGMENT_FALL
};

/* a segment's tick holds its number from the boss in EGG_VIPER_SEGMENT_NUMBER's
 * bits, and its kind in EGG_VIPER_SEGMENT_KIND's */
#define EGG_VIPER_SEGMENT_NUMBER 0xf
#define EGG_VIPER_SEGMENT_KIND 0x30
#define EGG_VIPER_PLAIN 0x0

/* indexes into egg_viper_objects. OBJECT_PART is a needle, and
 * OBJECT_JOINT_LIGHT the light a joint shows */
#define OBJECT_VIPER 0
#define OBJECT_PART 1
#define OBJECT_SEGMENT 2
#define OBJECT_JOINT 3
#define OBJECT_TAIL 4
#define OBJECT_JOINT_LIGHT 6
/* nodes of the boss's tree. needle 0's socket is node NODE_PART, and
 * needle 1's the next. Eggman rises to NODE_EGGMAN_RISE as the back opens */
#define NODE_PART 10
#define NODE_EGGMAN_RISE 48

/* indexes into egg_viper_effect_objects and egg_viper_effect_texlists.
 * EFFECT_RING is the ring that flashes at a laser's muzzle, and shrinks as
 * a laser charges */
#define EFFECT_LASER 0
#define EFFECT_RING 1
#define EFFECT_PILLAR 4
/* indexes into egg_viper_sprites. SPRITE_HALO is the ball's, and
 * SPRITE_FLARE and SPRITE_SPARK the gathering energy's */
#define SPRITE_IMPACT 0
#define SPRITE_BALL 1
#define SPRITE_HALO 2
#define SPRITE_FLARE 3
#define SPRITE_SPARK 4
/* the scorch's texture, egm3_laserkoge, in egg_viper_effect_texlist */
#define SCORCH_TEXTURE 8
/* the uvs of the arena's smoke model, and of the pillar of fire's */
#define ARENA_SMOKE_UVS 12
#define PILLAR_UVS 58

#define SE_CHARGE 0x1e8
#define SE_LASER 0x1e9
#define SE_IMPACT 0x1ea
#define SE_RUSH 0x1eb /* the boss sets off on a rush */
#define SE_OPEN 0x1ed
#define SE_EGGMAN 0x1ee /* Eggman rises */
#define SE_GATHER 0x1ef
#define SE_BALL 0x1f1
#define SE_LASER_BLAST 0x1f2
#define SE_NEEDLE_STRIKE 0x1f3
#define SE_NEEDLE_SPIN 0x1f4
#define SE_JOINT 0x1f8
#define SE_BURST 0x1f9
#define SE_PILLARS 0x1fa

/* the hits on Eggman the boss takes before it breaks */
#define HEALTH 7
/* how much faster the boss, a needle or a segment falls each step */
#define GRAVITY 0.10888889F

/* Eggman's lines, by index in eggman_voices, named for when he says them */
enum eggman_line {
    LINE_RIDDEN, /* player 0 rides a needle, once a fight */
    LINE_ATTACK, /* VIPER_LASER3's first laser, or VIPER_BOOM's creep */
    LINE_OPEN, /* the back opens */
    LINE_GATHER, /* 210 steps into the gathering */
    LINE_HIT, /* a hit on him leaves the boss health */
    LINE_LEAVE, /* VIPER_LASER2 leaves */
    LINE_ONE_LEFT, /* VIPER_BOOM starts with one hit left, once a fight */
    LINE_SWEEP, /* VIPER_HAKAID's sweep starts */
    LINE_ARC, /* a VIPER_LASER1 arc starts */
    LINE_DEFEAT /* the last hit on him */
};

/* Tikal's hints the boss shows, by index in hint_pages and hint_voices. it
 * shows them as its energy gathers, as its needles strike, and as it breaks */
#define HINT_GATHER 1
#define HINT_STRIKES 3
#define HINT_BREAK 4
/* scripts[4], the first script with VIPER_BOOM */
#define FIRST_BOOM_SCRIPT 4

/* a laser's kinds, in its tick. they are a joint's, the one VIPER_LASER2 aims,
 * and those VIPER_LASER3 fires as it passes */
#define LASER_JOINT 0
#define LASER_AIMED 1
#define LASER_PASSING 2

/* a laser's work->mode once it ends unfired. laser_step's default case
 * kills the task on its next step */
#define LASER_ENDED 100
/* the beam model's scale in z for each unit the laser reaches */
#define LASER_SCALE 0.07302823F
/* how far laser_draw moves the beam model along its z, in the model's own
 * units */
#define LASER_MODEL_Z 18.929505F

/* a laser's record, in task->extra. the beam runs from the task's pos to
 * tip. it first stands at the muzzle while its tip runs out to the target.
 * it then stands at the target while its tip runs in from the muzzle */
struct laser {
    Uint8 unk_00[8];
    Float length; /* the beam's length, as a scale of its model's */
    NJS_VECTOR speed; /* the tip's move each step */
    NJS_POINT3 tip;
    NJS_POINT3 target;
    Uint8 unk_30[40];
    Float time; /* the steps the tip has run, to 15 */
    Uint8 unk_5c[36];
};

/* a scorch's record, in task->extra */
struct scorch {
    Uint8 unk_00[88];
    Float alpha;
    Uint8 unk_5c[36];
};

/* the gathering energy's view of work->vars. fade is how far palette 0 has
 * brightened, from 0 to 1. grown counts the steps the flares have stood
 * full size, and its bit 1 blinks them */
struct gather_vars {
    Float fade;
    Uint8 unk_04[4];
    int grown;
};

/* a palette flash's view of work->vars. level is the value it sets the
 * palette's channel to */
struct flash_vars {
    Uint8 unk_00[8];
    int level;
};

#define GATHER(work) ((struct gather_vars *)&(work)->vars)
#define FLASH(work) ((struct flash_vars *)&(work)->vars)

/* the level a falling flash starts from, 8 above 255. its first step of 8
 * then sets 255 */
#define FLASH_START 0x107

/* VIPER_BOOM's count of its needles' turns. needle NEEDLE_TURNS(work) & 1
 * strikes next. a turn passes as each strike ends, and on each step that
 * starts none */
#define NEEDLE_TURNS(work) ((work)->vars.l[2])
/* the steps a segment or Eggman has left after a hit, in which no other hit
 * registers */
#define HIT_PAUSE(work) ((work)->vars.l[0])
/* a pillar of fire's width */
#define PILLAR_WIDTH(work) ((work)->vars.f[0])
/* the steps a pillar waits before it rises */
#define PILLAR_DELAY(work) ((work)->vars.l[1])
/* how far a pillar has scrolled the uvs' v, below 256 */
#define PILLAR_SCROLL(work) ((work)->vars.l[2])

/* the effects' trees, by EFFECT_ index, the ring's motion, and the
 * texlists the effects draw with */
extern NJS_OBJECT *egg_viper_effect_objects[];
extern NJS_MOTION *egg_viper_effect_motions[];
extern NJS_TEXLIST *egg_viper_effect_texlists[];
/* the joint light's material, whose diffuse green the arena's step pulses */
extern NJS_MATERIAL joint_light_material[];
/* the smoke's model under the floor, and its uvs */
extern NJS_OBJECT arena_smoke_object;
extern NJS_COLOR arena_smoke_uvs[];

static void eggman_say(int line);
static void eggman_say_once(int line);
static void hint_step(struct task *task);
static void hint_destroy(struct task *task);
static void hint_show(int hint);
static void viper_init(struct task *task);
static void viper_wait(struct task *task);
static void viper_laser1(struct task *task);
static void viper_laser2(struct task *task);
static void viper_laser3(struct task *task);
static void viper_energy(struct task *task);
static void viper_boomp(struct task *task);
static void viper_boom(struct task *task);
static void viper_hakaid(struct task *task);
static void viper_hakai(struct task *task);
static void viper_break(struct task *task);
static void viper_max(struct task *task);
static void viper_draw(struct task *task);
static void viper_destroy(struct task *task);
static void part_unfold(struct task *task);
static void part_hold(struct task *task);
static void part_strike(struct task *task);
static void part_fold(struct task *task);
static void part_fall(struct task *task);
static void part_draw(struct task *task);
static void part_destroy(struct task *task);
static void parts_fall_step(struct task *task);
static void part_top_step(struct task *task);
static void part_top_draw(struct task *task);
static void part_top_destroy(struct task *task);
static void segment_draw(struct task *task);
static void segment_destroy(struct task *task);
static void segments_fall_step(struct task *task);
static void laser_create(NJS_POINT3 *pos, int kind);
static void laser_step(struct task *task);
static void laser_draw(struct task *task);
static void laser_destroy(struct task *task);
static void flash_create(NJS_POINT3 *pos, struct angles *ang);
static void flash_step(struct task *task);
static void flash_draw(struct task *task);
static void flash_destroy(struct task *task);
static void charge_create(NJS_POINT3 *pos, struct angles *ang);
static void charge_step(struct task *task);
static void charge_draw(struct task *task);
static void charge_destroy(struct task *task);
static void impact_create(NJS_POINT3 *pos);
static void impact_step(struct task *task);
static void impact_draw(struct task *task);
static void impact_destroy(struct task *task);
static void scorch_create(NJS_POINT3 *pos);
static void scorch_step(struct task *task);
static void scorch_draw(struct task *task);
static void scorch_destroy(struct task *task);
static void ball_create(NJS_POINT3 *pos);
static void ball_step(struct task *task);
static void ball_draw(struct task *task);
static void ball_destroy(struct task *task);
static void gather_create(NJS_POINT3 *pos, struct angles *ang);
static void gather_step(struct task *task);
static void gather_draw(struct task *task);
static void gather_destroy(struct task *task);
static void pillars_create(NJS_POINT3 *pos);
static void pillar_step(struct task *task);
static void pillar_draw(struct task *task);
static void pillar_destroy(struct task *task);
static void pillar_bomb(NJS_POINT3 *pos);
static void eggman_energy_create(void);
static void eggman_energy_step(struct task *task);
static void eggman_energy_draw(struct task *task);
static void eggman_energy_destroy(struct task *task);
static void eggman_boom_create(void);
static void eggman_boom_step(struct task *task);
static void eggman_boom_draw(struct task *task);
static void eggman_boom_destroy(struct task *task);
static void pads_hold(int count);
static void pads_hold_step(struct task *task);
static void pads_hold_destroy(struct task *task);
static void arena_draw(struct task *task);
static void arena_destroy(struct task *task);
static void palette_save(void);
static void palette_raise(int level, int mask, int shift);
static void palette_lower(int level, int mask, int shift);
static void hit_flash_step(struct task *task);
static void palette_restore(struct task *task);
static void defeat_flash_step(struct task *task);
static void blast_flash_step(struct task *task);
static void spline_solve(const Float *knots, const Float *values, Float *dd,
                         Sint32 count);
static Float spline_value(Float along, const Float *knots, const Float *values,
                          const Float *dd, int count);
static int viper_fly_to(struct task_work *work, struct egg_viper_record *record,
                        NJS_POINT3 *target, Angle turn, Float speed);
static int move_to(NJS_POINT3 *pos, NJS_POINT3 *target, Float speed);
static void viper_next_mode(struct task_work *work,
                            struct egg_viper_record *record);
static int motion_play(struct egg_viper_record *record);
static int motion_rewind(struct egg_viper_record *record);
static int sprite_loop(struct egg_viper_record *record);
static int sprite_play(struct egg_viper_record *record);
static void sprite_draw(struct egg_viper_sprite *sprite, NJS_POINT3 *pos,
                        Float frame);
static void sprite_draw_scaled(struct egg_viper_sprite *sprite, NJS_POINT3 *pos,
                               Float scale, Float frame);
static void scorch_quad_draw(NJS_POINT3 *pos, Float alpha);
static void viper_muzzle(struct task_work *work, NJS_POINT3 *pos);
static void viper_ball_point(struct task_work *work, NJS_POINT3 *pos);
static int viper_was_hit(struct task_work *work,
                         struct egg_viper_record *record);
static void node_pos(NJS_MATRIX *matrices, int node, NJS_POINT3 *pos);

Sint32 egg_viper_energy_time;
#ifdef GCC
/* nodes 0 to 50, which the image holds in the three objects below */
NJS_MATRIX egg_viper_matrices[51];
#else
/* nodes 0 to 46. egg_viper_tree_matrices writes nodes 47 to 50 on past its
 * end, and node_pos reads node 48 the same way */
NJS_MATRIX egg_viper_matrices[47];
/* @hack type: node 47's matrix an object of its own, which the segment
 * step loads by one pool word (0x0c9049b8, at 0x0c9048a8 and 0x0c90495a);
 * as &egg_viper_matrices[47] at both, the step's frame grows from 120
 * bytes (ADD #-120,R15 at 0x0c9047ea) to 124, it starts 42 bytes late and
 * 798 of 837 words differ */
NJS_MATRIX egg_viper_anchor_matrix;
/* nodes 48 to 50. nothing reads or writes the 13 matrices after them,
 * which put egg_viper_start_ang where the image has it (0x0c95a48c) */
NJS_MATRIX egg_viper_node_matrices[16];
#endif
Angle egg_viper_start_ang;
Bool egg_viper_camera_next;
Bool egg_viper_camera_ready;
/* the steps left of the 30 a hit sets. VIPER_BOOM waits for them to run out
 * before it closes behind player 0. its Eggman waits for them before he sinks.
 * the arena's step counts them down */
static int hit_wait;
/* the boss and Eggman draw only while it is TRUE. viper_init and the
 * arena's step set it, and nothing clears it */
static Bool viper_shown;
/* TRUE from a hit on Eggman until the arena's step puffs smoke about the
 * boss for it */
static Bool hit_burst;
/* whether the boss runs its next script once this one ends. a hit on
 * Eggman sets it, as does VIPER_HAKAID's end */
static Bool script_next;
/* the energy modes begun since the boss was last hit in one */
static int energy_misses;
static int health;
/* health before the latest hit test. nothing reads it */
static int last_health;
/* bit line is set once Eggman has said line this fight */
static int eggman_said;
static Float entrance_knots[5];
static Float entrance_ddx[5];
static Float entrance_ddy[5];
static Float entrance_ddz[5];
static Float energy_knots[5];
static Float energy_ddx[5];
static Float energy_ddy[5];
static Float energy_ddz[5];
static Float hakaid_knots[6];
static Float hakaid_ddx[6];
static Float hakaid_ddy[6];
static Float hakaid_ddz[6];
/* the sign of Eggman's move in x as he rose in the energy mode, 1 or -1. a
 * soft throw sends player 0 along it, and a hard one against it */
static Float push_way;
/* the pending siblings of egg_viper_tree_matrices' walk */
static NJS_OBJECT *tree_stack[32];
/* the smoke model's uvs as the arena's step found them. each of the
 * smoke's two layers draws them moved by its own scroll */
static NJS_COLOR arena_smoke_saved[ARENA_SMOKE_UVS];
static NJS_COLOR arena_smoke_scroll[2];
/* palette 0's base and offset colors, as viper_init found them */
static Uint32 palette_saved[DRAW_PALETTE_ENTRIES * 2];
/* spline_solve's work. spline_h holds the widths between knots.
 * spline_diag holds each interval's slope, then the diagonal of the
 * eliminated system */
static Float spline_h[16];
static Float spline_diag[16];
/* the scorch quad's corners and uvs, as scorch_quad_draw fills them */
static NJS_POINT3 scorch_points[4];
static NJS_COLOR scorch_uvs[4];

/* where the boss starts */
static NJS_POINT3 start_pos = {0.0F, 100.0F, 0.0F};
/* each script lists modes in the order the boss runs them, then -1 */
static Sint8 script_0[3] = {VIPER_LASER1, VIPER_ENERGY, -1};
static Sint8 script_1[4] = {VIPER_LASER1, VIPER_LASER2, VIPER_ENERGY, -1};
static Sint8 script_2[4] = {VIPER_LASER1, VIPER_LASER3, VIPER_ENERGY, -1};
static Sint8 script_3[4] = {VIPER_LASER1, VIPER_BOOMP, VIPER_BOOM, -1};
static Sint8 script_4[2] = {VIPER_HAKAID, -1};
static Sint8 script_5[3] = {VIPER_HAKAI, VIPER_BOOM, -1};
static Sint8 script_6[4] = {VIPER_LASER1, VIPER_HAKAI, VIPER_BOOM, -1};
/* the boss's scripts, by record->script, then a null. as a script ends, the
 * boss runs the next if script_next is set, and else runs it again. after the
 * last it runs the first */
static Sint8 *scripts[19] = {
    script_0, script_1, script_1, script_2, script_3, script_4, script_5,
    script_5, script_5, script_5, script_5, script_5, script_5, script_5,
    script_5, script_5, script_5, script_5, NULL,
};
/* each mode's name. nothing reads them */
static char *viper_mode_names[VIPER_MODES] = {
    TEXT_1, TEXT_2, TEXT_3, TEXT_4,  TEXT_5,  TEXT_6,
    TEXT_7, TEXT_8, TEXT_9, TEXT_10, TEXT_11, TEXT_12,
};
/* the boss's shapes: its body and its back. the three after them sit
 * behind the back, and stand in for it while the back is open */
/* clang-format off */
static struct collision_shape viper_shapes[5] = {
    {SHAPE_SPHERE | SHAPE_TURN_YZX,
     SHAPE_HURTS_PLAYERS, PUSH_PLAYERS | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F}, 35.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_TURN_YZX,
     SHAPE_HURTS_PLAYERS, PUSH_PLAYERS | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, -60.0F}, 20.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_TURN_YZX,
     SHAPE_HURTS_PLAYERS, PUSH_PLAYERS | PUSH_FIRM(PUSH_ALL),
     {0.0F, 60.0F, -80.0F}, 15.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_TURN_YZX,
     SHAPE_HURTS_PLAYERS, PUSH_PLAYERS | PUSH_FIRM(PUSH_ALL),
     {-50.0F, 10.0F, -80.0F}, 15.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_TURN_YZX,
     SHAPE_HURTS_PLAYERS, PUSH_PLAYERS | PUSH_FIRM(PUSH_ALL),
     {50.0F, 10.0F, -80.0F}, 15.0F, 0.0F, 0.0F, {0}, 0, NULL},
};
static struct collision_shape part_shapes[1] = {
    {SHAPE_CYLINDER2 | SHAPE_TURN_YZX,
     ATTACK(2) | GUARD(2) | SHAPE_HURTS_PLAYERS,
     PUSH_PLAYERS | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F}, 28.0F, 5.0F, 0.0F, {0}, 0, NULL},
};
static struct collision_shape part_top_shape = {
    SHAPE_CYLINDER | SHAPE_TURN_YZX,
    0, PUSH_PLAYERS | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F}, 28.0F, 5.0F, 0.0F, {0}, 0, NULL,
};
static struct collision_shape segment_shape = {
    SHAPE_SPHERE | SHAPE_TURN_YZX,
    ATTACK(2) | GUARD(2) | SHAPE_HURTS_PLAYERS,
    PUSH_PLAYERS | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F}, 10.0F, 0.0F, 0.0F, {0}, 0, NULL,
};
static struct collision_shape joint_shape = {
    SHAPE_SPHERE | SHAPE_TURN_YZX | SHAPE_HURTABLE,
    ATTACK(1) | SHAPE_HURTS_PLAYERS, PUSH_PLAYERS | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F}, 10.0F, 0.0F, 0.0F, {0}, 0, NULL,
};
static struct collision_shape tail_shape = {
    SHAPE_SPHERE | SHAPE_TURN_YZX,
    ATTACK(2) | GUARD(2) | SHAPE_HURTS_PLAYERS,
    PUSH_PLAYERS | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F}, 10.0F, 0.0F, 0.0F, {0}, 0, NULL,
};
static struct collision_shape eggman_shapes[1] = {
    {SHAPE_SPHERE | SHAPE_TURN_YZX | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS, PUSH_PLAYERS | PUSH_FIRM(PUSH_ALL),
     {0.0F, 1.0F, 0.0F}, 15.0F, 0.0F, 0.0F, {0}, 0, NULL},
};
static struct collision_shape impact_shape = {
    SHAPE_CYLINDER | SHAPE_TURN_YZX,
    ATTACK(2) | GUARD(2) | SHAPE_HURTS_PLAYERS, PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F}, 3.0F, 30.0F, 0.0F, {0}, 0, NULL,
};
static struct collision_shape ball_shape = {
    SHAPE_SPHERE | SHAPE_TURN_YZX,
    ATTACK(2) | GUARD(2) | SHAPE_HURTS_PLAYERS, PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F}, 30.0F, 0.0F, 0.0F, {0}, 0, NULL,
};
/* the blast a LASER_AIMED laser makes where it lands. laser_step points its
 * pos at the laser's, which the blast copies */
static struct blast_params laser_blast = {
    NULL, BLAST_COPY_POS | BLAST_HURTS,
    0, 60, 100.0F, 0x5b,
    0, 40, 60, 90, 50.0F,
};
/* clang-format on */
static Uint8 dat_0c938db8[44] = {0};
/* TRUE while a hint's task lives. Eggman says no line while it is */
static Bool hint_showing = FALSE;
/* the voice of each of Eggman's lines, by enum eggman_line */
static Sint32 eggman_voices[10] = {179, 180, 788, 789, 174,
                                   175, 178, 173, 176, 177};
/* Tikal's five hints: each one's voice, and its page of text with the
 * steps it shows. only the three with HINT_ names show */
static Sint32 hint_voices[5] = {860, 864, 865, 862, 863};
static const struct script_page hint_1[2] = {{TEXT_13, 120}, {NULL, 0}};
static const struct script_page hint_2[2] = {{TEXT_14, 120}, {NULL, 0}};
static const struct script_page hint_3[2] = {{TEXT_15, 120}, {NULL, 0}};
static const struct script_page hint_4[2] = {{TEXT_16, 300}, {NULL, 0}};
static const struct script_page hint_5[2] = {{TEXT_17, 120}, {NULL, 0}};
static struct script_page *hint_pages[5] = {
    (struct script_page *)hint_1, (struct script_page *)hint_2,
    (struct script_page *)hint_3, (struct script_page *)hint_4,
    (struct script_page *)hint_5,
};
/* each mode's step, by enum viper_mode */
static void (*viper_modes[VIPER_MODES])(struct task *task) = {
    viper_init,   viper_wait,   viper_laser1, viper_laser2,
    viper_laser3, viper_energy, viper_boomp,  viper_boom,
    viper_hakaid, viper_hakai,  viper_break,  viper_max,
};
/* the path the boss flies in by: up from start_pos, round a loop, and down
 * below the floor */
static Float entrance_x[5] = {0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
static Float entrance_y[5] = {100.0F, 200.0F, 300.0F, 200.0F, -300.0F};
static Float entrance_z[5] = {0.0F, 100.0F, 0.0F, -100.0F, 0.0F};
static struct egg_viper_spline entrance_path = {
    entrance_x,   entrance_y,   entrance_z,   entrance_knots,
    entrance_ddx, entrance_ddy, entrance_ddz, 5,
};
/* VIPER_LASER1's x by side, on player 0's side of x 0 */
static Float laser1_x[2] = {-100.0F, 100.0F};
/* VIPER_LASER2's points by side, each at player 0's z in place of its own.
 * the boss rises from below the floor to above, then leaves out past the
 * row and down */
static NJS_POINT3 laser2_below[2] = {
    {-50.0F, -550.0F, 0.0F},
    {50.0F, -550.0F, 0.0F},
};
static NJS_POINT3 laser2_above[2] = {
    {-60.0F, 80.0F, 0.0F},
    {60.0F, 80.0F, 0.0F},
};
static NJS_POINT3 laser2_away[2] = {
    {-500.0F, 80.0F, 0.0F},
    {500.0F, 80.0F, 0.0F},
};
static NJS_POINT3 laser2_out[2] = {
    {-500.0F, -550.0F, 0.0F},
    {500.0F, -550.0F, 0.0F},
};
/* VIPER_LASER3's points by side. the boss starts below the floor past the end
 * of player 0's row nearer him. it rises to y 70 there. it passes along the row
 * to 120 short of its other end. it then leaves out past the row and down */
static NJS_POINT3 laser3_below[4] = {
    {-260.0F, -300.0F, -680.0F},
    {-260.0F, -300.0F, 680.0F},
    {260.0F, -300.0F, -680.0F},
    {260.0F, -300.0F, 680.0F},
};
static NJS_POINT3 laser3_rise[4] = {
    {-260.0F, 70.0F, -640.0F},
    {-260.0F, 70.0F, 640.0F},
    {260.0F, 70.0F, -640.0F},
    {260.0F, 70.0F, 640.0F},
};
static NJS_POINT3 laser3_pass[4] = {
    {-260.0F, 70.0F, 360.0F},
    {-260.0F, 70.0F, -360.0F},
    {260.0F, 70.0F, 360.0F},
    {260.0F, 70.0F, -360.0F},
};
static NJS_POINT3 laser3_climb[4] = {
    {-560.0F, 200.0F, 360.0F},
    {-560.0F, 200.0F, -360.0F},
    {560.0F, 200.0F, 360.0F},
    {560.0F, 200.0F, -360.0F},
};
static NJS_POINT3 laser3_out[4] = {
    {-660.0F, -400.0F, 360.0F},
    {-660.0F, -400.0F, -360.0F},
    {660.0F, -400.0F, 360.0F},
    {660.0F, -400.0F, -360.0F},
};
/* the path the energy mode flies once the back is open. energy_x_left
 * holds energy_x_right's values negated, its -0.0 among them. viper_energy
 * picks one of them by side, and fills energy_z with the boss's z */
static Float energy_x_right[5] = {100.0F, 80.0F, 0.0F, -20.0F, -90.0F};
static Float energy_x_left[5] = {-100.0F, -80.0F, -0.0F, 20.0F, 90.0F};
static Float energy_y[5] = {40.0F, 40.0F, 50.0F, 60.0F, 90.0F};
static Float energy_z[5] = {0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
static struct egg_viper_spline energy_path = {
    energy_x_right, energy_y,   energy_z,   energy_knots,
    energy_ddx,     energy_ddy, energy_ddz, 5,
};
/* VIPER_BOOMP's points by side. the boss starts below the floor past the far
 * end of player 0's row. it rises to y 80 there, and comes in level to 100 past
 * the row's end */
static NJS_POINT3 boomp_below[4] = {
    {-260.0F, -300.0F, -730.0F},
    {-260.0F, -300.0F, 730.0F},
    {260.0F, -300.0F, -730.0F},
    {260.0F, -300.0F, 730.0F},
};
static NJS_POINT3 boomp_rise[4] = {
    {-260.0F, 80.0F, -680.0F},
    {-260.0F, 80.0F, 680.0F},
    {260.0F, 80.0F, -680.0F},
    {260.0F, 80.0F, 680.0F},
};
static NJS_POINT3 boomp_over[4] = {
    {-260.0F, 80.0F, -580.0F},
    {-260.0F, 80.0F, 580.0F},
    {260.0F, 80.0F, -580.0F},
    {260.0F, 80.0F, 580.0F},
};
/* the two points VIPER_BOOM leaves by, out past the row and down, by side */
static NJS_POINT3 boom_leave[4] = {
    {-560.0F, 200.0F, 480.0F},
    {-560.0F, 200.0F, -480.0F},
    {560.0F, 200.0F, 480.0F},
    {560.0F, 200.0F, -480.0F},
};
static NJS_POINT3 boom_out[4] = {
    {-560.0F, -400.0F, 480.0F},
    {-560.0F, -400.0F, -480.0F},
    {560.0F, -400.0F, 480.0F},
    {560.0F, -400.0F, -480.0F},
};
/* the path VIPER_HAKAID sweeps a row along. it climbs from below the floor past
 * one end, and dives past the other. viper_hakaid fills hakaid_x with the row's
 * x, and picks a z table by the boss's place */
static Float hakaid_x[6] = {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
static Float hakaid_y[6] = {-400.0F, 0.0F, 0.0F, 0.0F, 0.0F, -400.0F};
static Float hakaid_z_down[6] = {
    840.0F, 600.0F, 480.0F, -480.0F, -600.0F, -840.0F,
};
static Float hakaid_z_up[6] = {
    -840.0F, -600.0F, -480.0F, 480.0F, 600.0F, 840.0F,
};
static struct egg_viper_spline hakaid_path = {
    hakaid_x,   hakaid_y,   hakaid_z_down, hakaid_knots,
    hakaid_ddx, hakaid_ddy, hakaid_ddz,    6,
};
static Float dat_0c9391fc[4] = {0.5F, 1.0F, 1.0F, 1.0F};
/* where the gathering energy's three flares stand in the boss's frame,
 * and their sparks 1 behind them */
static NJS_POINT3 gather_points[3] = {
    {0.0F, 60.0F, -100.0F},
    {-50.0F, 10.0F, -100.0F},
    {50.0F, 10.0F, -100.0F},
};
static NJS_POINT3 gather_spark_points[3] = {
    {0.0F, 60.0F, -101.0F},
    {-50.0F, 10.0F, -101.0F},
    {50.0F, 10.0F, -101.0F},
};
/* each pillar of fire's offset from where the boss fell */
static NJS_POINT3 pillar_offsets[3] = {
    {100.0F, 0.0F, 70.0F},
    {30.0F, 50.0F, -10.0F},
    {-60.0F, -40.0F, 120.0F},
};
/* each pillar's width, as a scale of its model's */
static Float pillar_widths[3] = {2.0F, 1.4F, 3.0F};
/* the steps each pillar waits before it rises */
static Sint32 pillar_delays[3] = {0, 30, 40};
static Sint32 dat_0c939290 = 0;
/* how far each layer of the smoke under the floor scrolls a step */
static NJS_TEX arena_smoke_steps[2] = {
#include "b_egm3/egg_viper/arena_smoke_steps.inc"
};
static Sint32 dat_0c93929c = 1;
/* steps since the last puff of smoke as the boss breaks */
static int smoke_wait = 0;
static Uint8 dat_0c9392a4[8] = {0};
/* a puff of smoke's offset from the boss, before a random turn about y and
 * then x */
static NJS_POINT3 smoke_offset = {0.0F, 0.0F, 50.0F};
static NJS_POINT3 dat_0c9392b8 = {0.0F, 1.0F, 0.0F};
/* the change in the joints' light's green each step. its sign flips at 0
 * and at 255 */
static Sint32 joint_light_step = -16;
static Uint8 dat_0c9392c8[12] = {0};
/* TRUE while a palette flash runs. a hit's or a blast's flash that finds
 * it TRUE ends at once. the defeat's flash waits for it to clear */
static Bool palette_busy = FALSE;
/* a scorch quad's corners about its place, and their colors */
static NJS_POINT3 scorch_corners[4] = {
#include "b_egm3/egg_viper/scorch_corners.inc"
};
static NJS_COLOR scorch_colors[4] = {
#include "b_egm3/egg_viper/scorch_colors.inc"
};
static NJS_POINT3COL scorch_quad = {
#include "b_egm3/egg_viper/scorch_quad.inc"
};
/* where the boss's lasers start and its ball forms, in the boss's frame */
static NJS_POINT3 muzzle_offset = {0.0F, -8.27F, 59.4F};
static NJS_POINT3 ball_offset = {0.0F, 2.0F, -70.0F};

/* Eggman says line, and eggman_said records it, unless a hint shows */
static void eggman_say(int line)
{
    if (hint_showing == FALSE) {
        eggman_said |= 1 << line;
        stream_request_voice(eggman_voices[line]);
    }
}

/* the same, but only a line he hasn't said this fight */
static void eggman_say_once(int line)
{
    int bit;

    if (hint_showing == FALSE) {
        bit = 1 << line;
        if (!(eggman_said & bit)) {
            eggman_said |= bit;
            stream_request_voice(eggman_voices[line]);
        }
    }
}

/* a hint's task. it keeps hint_showing TRUE for the steps the hint's page
 * shows */
static void hint_step(struct task *task)
{
    struct task_work *work = task->work;

    /* @hack spelling: a switch of one case, which tests the mode with CMP/EQ #0
     * (0x0c9008a2); as an if, 18 of 24 words differ */
    switch (work->mode) {
    case 0:
        task->destructor = hint_destroy;
        task->draw = NULL;
        hint_showing = TRUE;
        work->mode++;
    }
    if (--work->vars.l[0] == 0) {
        task_kill(task);
    }
}

static void hint_destroy(struct task *task)
{
    hint_showing = FALSE;
}

/* show Tikal's hint, and hold Eggman's lines while its page shows */
static void hint_show(int hint)
{
    struct task *task = task_create(TASK_WORK, 1, hint_step);

    task->work->vars.l[0] = hint_pages[hint]->steps;
    tikal_hint_create(hint_pages[hint], hint_voices[hint]);
}

void egg_viper_step(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    Sint8 mode;
    Angle ang;

    mode = work->mode;
    viper_modes[work->mode](task);
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    egg_viper_tree_matrices(egg_viper_objects[OBJECT_VIPER],
                            egg_viper_matrices);
    njPopMatrix(1);
    last_health = health;
    /* COLLISION_DAMAGED marks a hit on the boss's own shapes, and VIPER_HIT
     * one on Eggman. either cuts egg_viper_energy_time to 10 and sets
     * hit_wait. only a hit on Eggman costs health and sets script_next */
    if (work->flags & COLLISION_DAMAGED) {
        work->flags &= ~COLLISION_DAMAGED;
        egg_viper_energy_time = 10;
        hit_wait = 30;
        life_gauge_health = health;
    } else if (record->flags & VIPER_HIT) {
        record->flags &= ~VIPER_HIT;
        work->flags &= ~COLLISION_DAMAGED;
        egg_viper_energy_time = 10;
        /* @hack spelling: hit_burst's store in hit_wait's statement, which
         * keeps -5, 30, egg_viper_energy_time's address and hit_wait's in
         * R4 to R7 (0x0c9009e4 to 0x0c9009ee); as two statements, 13 of 240
         * words differ */
        hit_wait = 30, hit_burst = TRUE;
        script_next = TRUE;
        health--;
        task_create(TASK_WORK, 2, hit_flash_step);
        eggman_say(health > 0 ? LINE_HIT : LINE_DEFEAT);
        life_gauge_health = health;
    }
    if (task->step != task_destroy && !(record->flags & VIPER_HELD_OFF)) {
        collision_register(work);
    }
    viper_draw(task);
    if (health <= 0) {
        work->mode = VIPER_BREAK;
    }
    if (work->mode != mode) {
        work->submode = 0;
    }
}

/* set up the boss's record, place and shapes, and its health and gauge. start
 * the camera's swing round it, save palette 0 for the flashes, and stop the
 * time */
static void viper_init(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;

    work->submode = 0;
    record->step = 0;
    record->script = 0;
    record->flags = 0;
    record->shape = NULL;
    record->motion = NULL;
    task->destructor = viper_destroy;
    task->draw = viper_draw;
    work->pos = start_pos;
    work->ang.x = 0;
    work->ang.y = egg_viper_start_ang;
    work->ang.z = 0;
    collision_init(task, viper_shapes, COLLISION_OBJECT, 1, 5);
    work->collision->shapes[2].flags |= SHAPE_OFF;
    work->collision->shapes[3].flags |= SHAPE_OFF;
    work->collision->shapes[4].flags |= SHAPE_OFF;
    camera_start_swing(work, 20.0F, 150.0F, 300);
    camera_override_mode(CAMERA_MODE_CHAOS_STINIT, CAMERA_ADJUST_NONE);
    hit_wait = 0;
    viper_shown = TRUE;
    hit_burst = FALSE;
    script_next = FALSE;
    health = last_health = HEALTH;
    life_gauge_create(600, 24, HEALTH);
    egg_viper_energy_time = 240;
    egg_viper_camera_ready = FALSE;
    palette_save();
    time_stop();
    eggman_said = 0;
    energy_misses = 0;
    work->mode = VIPER_WAIT;
}

/* the pads and the time wait for the camera's swing round the boss, which
 * shows its name. the boss then flies in along entrance_path over 240
 * steps. 120 steps later its script starts */
static void viper_wait(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    NJS_POINT3 last;
    struct angles ang;

    switch (work->submode) {
    case 0:
        pad_disable();
        egg_viper_spline_init(&entrance_path);
        record->along = 0.0F;
        work->timer = 0;
        work->submode++;
    case 1:
        boss_name_create(TEXT_18, -1, 240, 120);
        work->submode++;
    case 2:
        pad_disable();
        time_stop();
        if (camera_swing_done) {
            camera_override_mode(CAMERA_MODE_EGM3, CAMERA_ADJUST_NONE);
            egg_viper_camera_reset();
            pad_enable();
            time_start();
            sound_play(SE_RUSH, 0, 0, 0);
            work->submode++;
        }
        break;
    case 3:
        record->along += 1.0F / 240.0F;
        last = work->pos;
        egg_viper_spline_point(&entrance_path, record->along, &work->pos);
        record->moved.x = work->pos.x - last.x;
        record->moved.y = work->pos.y - last.y;
        record->moved.z = work->pos.z - last.z;
        ang.y = (Angle)(atan2f(record->moved.x, record->moved.z) *
                        ANGLE_PER_RADIAN);
        ang.x = (Angle)(atan2f(-record->moved.y,
                               njSqrt((record->moved.x * record->moved.x) +
                                      (record->moved.z * record->moved.z))) *
                        ANGLE_PER_RADIAN);
        work->ang.x = egg_viper_angle_approach(work->ang.x, ang.x, 0x2d8);
        work->ang.y = egg_viper_angle_approach(work->ang.y, ang.y, 0x2d8);
        if (record->along >= 1.0F) {
            work->submode++;
        }
        break;
    case 4:
        if ((Uint16)++work->timer >= 120) {
            viper_next_mode(work, record);
        }
        break;
    }
}

/* once the camera is ready, the boss runs along the arena at 16 a step. it runs
 * at y -150, from the end on player 0's side of z 0. coming within 200 of him,
 * it arcs up half a sine 240 high as its joints fire. past either end it dives
 * and turns back. after its second run it moves on */
static void viper_laser1(struct task *task)
{
    struct task_work *player;
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    NJS_POINT3 last;
    struct angles ang;

    /* @hack spelling: player set after the declarations, which gives it R11
     * and atan2f's address R10 (0x0c900f1a, 0x0c900f28); initialized at its
     * declaration, they trade and 352 of 672 words differ */
    player = player_work[0];
    switch (work->submode) {
    case 0:
        record->motion = record->shape = NULL;
        work->timer = 0;
        work->submode = 1;
    case 1:
        if (egg_viper_camera_ready == FALSE) {
            break;
        }
        egg_viper_camera_ready = FALSE;
        record->side = player->pos.x < 0.0F ? 0 : 1;
        record->target.x = laser1_x[record->side];
        record->target.y = -150.0F;
        record->target.z = player->pos.z > 0.0F ? 550.0F : -550.0F;
        work->submode = 2;
    case 2:
        work->pos.x = record->target.x;
        work->pos.y = record->target.y;
        work->pos.z = record->target.z;
        work->submode = 3;
        break;
    case 3:
        record->moved.z = work->pos.z > 0.0F ? -16.0F : 16.0F;
        work->submode = 4;
    case 4:
        last = work->pos;
        work->pos.z += record->moved.z;
        ang.y = (Angle)(atan2f(0.0F, record->moved.z) * ANGLE_PER_RADIAN);
        ang.x = 0;
        work->ang.x = egg_viper_angle_approach(work->ang.x, ang.x, 0x222);
        work->ang.y = egg_viper_angle_approach(work->ang.y, ang.y, 0x222);
        if (record->moved.z > 0.0F) {
            if (work->pos.z < player->pos.z &&
                work->pos.z >= player->pos.z + -200.0F) {
                work->submode = 5;
            }
        } else {
            if (work->pos.z > player->pos.z &&
                work->pos.z <= player->pos.z + 200.0F) {
                work->submode = 5;
            }
        }
        if ((record->moved.z > 0.0F && work->pos.z >= 550.0F) ||
            (record->moved.z < 0.0F && work->pos.z <= -550.0F)) {
            work->submode = 11;
        }
        break;
    case 5:
        record->phase = 0;
        /* SHC folds 10.0F * (1.0F / 3.0F) to the image's 0x40555554, the
         * word at 0x0c90126c. 10.0F / 3.0F folds to the float above it */
        record->moved.z =
            (record->moved.z > 0.0F ? 1.0F : -1.0F) * (10.0F * (1.0F / 3.0F));
        record->flags |= VIPER_FIRING;
        sound_play(SE_RUSH, 0, 0, 0);
        eggman_say(LINE_ARC);
        work->submode = 6;
    case 6:
        last = work->pos;
        record->phase += 0x111;
        work->pos.y = (njSin(record->phase) * 240.0F) + -150.0F;
        work->pos.z += record->moved.z;
        record->moved.x = work->pos.x - last.x;
        record->moved.y = work->pos.y - last.y;
        record->moved.z = work->pos.z - last.z;
        ang.y = (Angle)(atan2f(record->moved.x, record->moved.z) *
                        ANGLE_PER_RADIAN);
        ang.x = (Angle)(atan2f(-record->moved.y,
                               njSqrt((record->moved.x * record->moved.x) +
                                      (record->moved.z * record->moved.z))) *
                        ANGLE_PER_RADIAN);
        work->ang.x = egg_viper_angle_approach(work->ang.x, ang.x, 0x222);
        work->ang.y = egg_viper_angle_approach(work->ang.y, ang.y, 0x222);
        if (record->phase >= 0x8000) {
            record->flags &= ~VIPER_FIRING;
            record->moved.z = record->moved.z > 0.0F ? 16.0F : -16.0F;
            work->submode = 4;
        }
        break;
    case 11:
        record->moved.y = -16.0F;
        record->moved.z = record->moved.z > 0.0F ? 0.01F : -0.01F;
        work->timer++;
        work->submode = 12;
    case 12:
        work->pos.y += record->moved.y;
        work->pos.z += record->moved.z;
        ang.y = (Angle)(atan2f(0.0F, record->moved.z) * ANGLE_PER_RADIAN);
        ang.x = (Angle)(atan2f(-record->moved.y, record->moved.z) *
                        ANGLE_PER_RADIAN);
        work->ang.x = egg_viper_angle_approach(work->ang.x, ang.x, 0x444);
        work->ang.y = egg_viper_angle_approach(work->ang.y, ang.y, 0x444);
        if (work->pos.y < -550.0F) {
            if ((Uint16)work->timer >= 2) {
                viper_next_mode(work, record);
            } else {
                work->submode = 9;
            }
        }
        break;
    case 9:
        record->moved.y = -record->moved.y;
        record->moved.z = -record->moved.z;
        work->submode = 10;
    case 10:
        work->pos.y += record->moved.y;
        work->pos.z += record->moved.z;
        ang.y = (Angle)(atan2f(0.0F, record->moved.z) * ANGLE_PER_RADIAN);
        ang.x = (Angle)(atan2f(-record->moved.y, record->moved.z) *
                        ANGLE_PER_RADIAN);
        work->ang.x = egg_viper_angle_approach(work->ang.x, ang.x, 0x1c71);
        work->ang.y = egg_viper_angle_approach(work->ang.y, ang.y, 0x1c71);
        if (work->pos.y >= -150.0F) {
            work->submode = 3;
        }
        break;
    }
}

/* the boss waits 120 steps. it then rises at player 0's z, between the rows
 * on his side of x 0. it pitches its nose toward him, and then turns after
 * him for 90 steps. after 30 steps of charge, it fires a LASER_AIMED laser. it
 * leaves 120 steps later */
static void viper_laser2(struct task *task)
{
    struct task_work *player;
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    Float y;
    Float across;
    NJS_POINT3 muzzle;

    /* @hack spelling: player set after the declarations, which loads
     * player_work first (0x0c90144c) and keeps player 0's x at sp+8 and
     * across at sp+0 (0x0c901626, 0x0c901638); initialized at its
     * declaration, those two trade and 20 of 576 words differ */
    player = player_work[0];
    switch (work->submode) {
    case 0:
        record->shape = NULL;
        record->motion = NULL;
        record->side = player->pos.x < 0.0F ? 0 : 1;
        work->submode = 1;
    case 1:
        work->timer = 0;
        work->submode = 2;
    case 2:
        if ((Uint16)++work->timer >= 120) {
            work->submode = 3;
        }
        break;
    case 3:
        work->pos.x = laser2_below[record->side].x;
        work->pos.y = laser2_below[record->side].y;
        work->pos.z = player->pos.z;
        record->target.x = laser2_above[record->side].x;
        record->target.y = laser2_above[record->side].y;
        record->target.z = player->pos.z;
        egg_viper_camera_next = TRUE;
        work->submode = 4;
    case 4:
        work->ang.x = 0xc000;
        work->ang.y = record->side == 0 ? 0xc000 : 0x4000;
        y = work->pos.y;
        work->pos.y += 10.0F;
        work->pos.x += record->side == 0 ? -0.1F : 0.1F;
        if (work->pos.y >= record->target.y) {
            work->submode = 5;
        } else if (work->pos.y >= -100.0F && y < -100.0F) {
            sound_play(SE_RUSH, 0, 0, 0);
        }
        break;
    case 5:
        /* @hack spelling: across set in the ?:'s test, which stores it to sp+0
         * in the branch's delay slot (0x0c901638); set in a statement before
         * it, it takes FR15 and 147 of 576 words differ */
        record->target_ang_x =
            (Angle)(atan2f(-player->pos.y + work->pos.y,
                           (across = player->pos.x - work->pos.x) >= 0.0F
                               ? across
                               : work->pos.x - player->pos.x) *
                    ANGLE_PER_RADIAN) &
            0xffff;
        work->submode = 6;
    case 6:
        work->ang.x =
            egg_viper_angle_approach(work->ang.x, record->target_ang_x, 0x444);
        if (work->ang.x == record->target_ang_x) {
            work->submode = 7;
        }
        break;
    case 7:
        work->timer = 90;
        work->submode = 8;
    case 8:
        record->target.x = player->pos.x;
        record->target.z = player->pos.z;
        record->target_ang_y = (Angle)(atan2f(record->target.x - work->pos.x,
                                              record->target.z - work->pos.z) *
                                       ANGLE_PER_RADIAN) &
                               0xffff;
        work->ang.y =
            egg_viper_angle_approach(work->ang.y, record->target_ang_y, 0x2d8);
        if ((Uint16)--work->timer == 0) {
            work->submode = 9;
        }
        break;
    case 9:
        work->timer = 0;
        viper_muzzle(work, &muzzle);
        charge_create(&muzzle, &work->ang);
        sound_play(SE_CHARGE, 0, 0, 0);
        work->submode = 10;
    case 10:
        if ((Uint16)++work->timer >= 30) {
            work->submode = 11;
        }
        break;
    case 11:
        work->timer = 0;
        viper_muzzle(work, &muzzle);
        laser_create(&muzzle, LASER_AIMED);
        work->submode = 12;
    case 12:
        if ((Uint16)++work->timer >= 120) {
            work->submode = 13;
        }
        break;
    case 13:
        record->target.x = laser2_away[record->side].x;
        record->target.y = laser2_away[record->side].y;
        record->target.z = player->pos.z;
        egg_viper_camera_next = TRUE;
        eggman_say(LINE_LEAVE);
        work->submode = 14;
    case 14:
        if (viper_fly_to(work, record, &record->target, 0x4000, 16.0F)) {
            work->submode = 15;
        }
        break;
    case 15:
        record->target.x = laser2_out[record->side].x;
        record->target.y = laser2_out[record->side].y;
        record->target.z = player->pos.z;
        work->submode = 16;
    case 16:
        if (viper_fly_to(work, record, &record->target, 0x4000, 16.0F)) {
            viper_next_mode(work, record);
        }
        break;
    }
}

/* the boss waits 120 steps, then rises past the end of player 0's row nearer
 * him. it levels out and flies along the row. while he is more than 60 ahead,
 * it fires every 20 steps. once he is behind it, or the pass ends, it climbs
 * away */
static void viper_laser3(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    struct task_work *player = player_work[0];
    NJS_POINT3 muzzle;

    switch (work->submode) {
    case 0:
        record->motion = record->shape = NULL;
        work->submode = 1;
    case 1:
        work->timer = 0;
        work->submode = 2;
    case 2:
        if ((Uint16)++work->timer >= 120) {
            work->submode = 3;
        }
        break;
    case 3:
        record->side =
            (player->pos.x < 0.0F ? 0 : 2) | (player->pos.z < 0.0F ? 0 : 1);
        record->target.x = laser3_below[record->side].x;
        record->target.y = laser3_below[record->side].y;
        record->target.z = laser3_below[record->side].z;
        work->submode = 4;
    case 4:
        work->pos.x = record->target.x;
        work->pos.y = record->target.y;
        work->pos.z = record->target.z;
        egg_viper_camera_next = TRUE;
        work->submode = 5;
        break;
    case 5:
        record->target.x = laser3_rise[record->side].x;
        record->target.y = laser3_rise[record->side].y;
        record->target.z = laser3_rise[record->side].z;
        sound_play(SE_RUSH, 0, 0, 0);
        work->submode = 6;
    case 6:
        if (viper_fly_to(work, record, &record->target, 0x4000, 4.0F)) {
            work->submode = 7;
        }
        break;
    case 7:
        record->target_ang_x = 0;
        work->ang.y = (record->side & 1) == 0 ? 0 : 0x8000;
        work->submode = 8;
    case 8:
        work->ang.x =
            egg_viper_angle_approach(work->ang.x, record->target_ang_x, 0xb6);
        if (work->ang.x == record->target_ang_x) {
            work->submode = 9;
        }
        break;
    case 9:
        record->target.x = laser3_pass[record->side].x;
        record->target.y = laser3_pass[record->side].y;
        record->target.z = laser3_pass[record->side].z;
        work->timer = 0;
        work->submode = 10;
    case 10:
        if (viper_fly_to(work, record, &record->target, 0x5b0, 4.0F)) {
            work->submode = 11;
        }
        if ((record->moved.z > 0.0F && player->pos.z > work->pos.z + 60.0F) ||
            (record->moved.z < 0.0F && player->pos.z < work->pos.z + -60.0F)) {
            if ((Uint16)work->timer % 20 == 0) {
                viper_muzzle(work, &muzzle);
                laser_create(&muzzle, LASER_PASSING);
                if (work->timer == 0) {
                    eggman_say(LINE_ATTACK);
                }
            }
            work->timer++;
        } else if ((record->moved.z > 0.0F && player->pos.z < work->pos.z) ||
                   (record->moved.z < 0.0F && player->pos.z > work->pos.z)) {
            work->submode = 11;
        }
        break;
    case 11:
        record->target.x = laser3_climb[record->side].x;
        record->target.y = laser3_climb[record->side].y;
        record->target.z = laser3_climb[record->side].z;
        work->submode = 12;
    case 12:
        if (viper_fly_to(work, record, &record->target, 0x4000, 8.0F)) {
            work->submode = 13;
        }
        break;
    case 13:
        record->target.x = laser3_out[record->side].x;
        record->target.y = laser3_out[record->side].y;
        record->target.z = laser3_out[record->side].z;
        work->submode = 14;
    case 14:
        if (viper_fly_to(work, record, &record->target, 0x4000, 8.0F)) {
            viper_next_mode(work, record);
        }
        break;
    }
}

/* the boss rises nose up between the rows at player 0's z, on his side of
 * x 0. it levels out and opens its back, and Eggman rises. it holds for
 * egg_viper_energy_time steps, then flies energy_path over 90 steps. it
 * gathers energy for 420 steps, and its wave fires the ball. after a hit
 * while it's open, or after the ball, it closes as it climbs nose up. it
 * then dives below the floor */
static void viper_energy(struct task *task)
{
    int hit = 0;
    struct task_work *player = player_work[0];
    struct egg_viper_record *record = task->extra;
    /* @hack spelling: motions, speed, withdraw and zero, locals for what the
     * image keeps in registers: the motions in R5, 4.0 in FR13, the
     * withdrawal's submode 17 in R8 and 0.0 in FR15 (0x0c901d6a,
     * 0x0c901d64, 0x0c901d6e, 0x0c901d78); each as its literal, with the
     * dead code below that serves it, 46, 14, 61 and 74 of 928 words
     * differ */
    NJS_MOTION **motions = egg_viper_motions;
    Float speed = 4.0F;
    struct task_work *work = task->work;
    Sint8 withdraw = 17;
    Float zero = 0.0F;
    NJS_POINT3 last;
    struct angles ang;
    NJS_POINT3 ball;
    int i;

    switch (work->submode) {
    case 0:
        /* @hack spelling: the shapes read through egg_viper_motions, in R7 from
         * 0x0c901d60 (0x0c901dd8, 0x0c90226a), and the motions through its
         * copy motions in R5; either shape through motions, 1 of 928 words
         * differs, and both, 85 */
        record->motion = motions[EGG_VIPER_BACK_OPEN];
        record->shape = egg_viper_motions[EGG_VIPER_BACK_OPEN_SHAPE];
        record->frame = zero;
        record->speed = 1.0F;
        record->side = player->pos.x < 0.0F ? 0 : 1;
        energy_misses++;
        work->submode = 1;
    case 1:
        record->target.x = record->side == 0 ? -110.0F : 110.0F;
        record->target.y = -200.0F;
        record->target.z = player->pos.z;
        work->submode = 2;
    case 2:
        work->pos.x = record->target.x;
        work->pos.y = record->target.y;
        work->pos.z = record->target.z;
        work->submode = 3;
        break;
    case 3:
        /* @hack redundant: a never-run store of zero, which keeps 0.0 in
         * FR15 (0x0c901d78); without it, 66 of 928 words differ */
        if (0) {
            record->frame = zero;
        }
        /* @hack redundant: a never-run test of motions, which with the
         * second below gives the table R7 and player_work R6 (0x0c901d60,
         * 0x0c901d58), and hit R10 and energy_misses' address R11
         * (0x0c901d66, 0x0c901d70); without it, each pair trades and 24 of
         * 928 words differ */
        if (0) {
            if (motions) {
            }
        }
        /* @hack redundant: a never-run store of withdraw, which keeps 17
         * in R8 (0x0c901d6e); without it, 66 of 928 words differ */
        if (0) {
            work->submode = withdraw;
        }
        /* @hack redundant: a second never-run test of motions, for the same
         * registers, which also gives case 3's test R2 (0x0c901e46); without
         * it, 26 of 928 words differ */
        if (0) {
            if (motions) {
            }
        }
        if (record->side == 0) {
            record->target.x = -100.0F;
            work->ang.y = 0x4000;
        } else {
            record->target.x = 100.0F;
            work->ang.y = 0xc000;
        }
        record->target.y = 40.0F;
        work->ang.x = 0xc000;
        sound_play(SE_RUSH, 0, 0, 0);
        work->submode = 4;
    case 4:
        if (move_to(&work->pos, &record->target, speed)) {
            work->submode = 5;
        }
        break;
    case 5:
        work->submode = 6;
    case 6:
        work->ang.x = (Uint16)(work->ang.x + 0x400);
        if (work->ang.x == 0) {
            work->submode = 7;
        }
        break;
    case 7:
        sound_play(SE_OPEN, 0, 0, 0);
        work->submode = 8;
    case 8:
        if (motion_play(record)) {
            work->submode = 9;
        }
        break;
    case 9:
        eggman_energy_create();
        record->flags |= VIPER_BACK_OPEN;
        record->flags |= VIPER_GENTLE_THROW;
        work->collision->shapes[1].flags |= SHAPE_OFF;
        work->collision->shapes[2].flags &= ~SHAPE_OFF;
        work->collision->shapes[3].flags &= ~SHAPE_OFF;
        work->collision->shapes[4].flags &= ~SHAPE_OFF;
        work->timer = 0;
        eggman_say(LINE_OPEN);
        work->submode = 10;
    case 10:
        if ((Uint16)++work->timer >= egg_viper_energy_time) {
            work->submode = 11;
        }
        if (viper_was_hit(work, record)) {
            pads_hold(60);
            energy_misses = 0;
            work->submode = withdraw;
        }
        break;
    case 11:
        if (record->side == 0) {
            energy_path.x = energy_x_left;
        } else {
            energy_path.x = energy_x_right;
        }
        for (i = 0; i < 5; i++) {
            energy_z[i] = work->pos.z;
        }
        egg_viper_spline_init(&energy_path);
        record->along = zero;
        sound_play(SE_RUSH, 0, 0, 0);
        work->submode = 12;
    case 12:
        record->along += 1.0F / 90.0F;
        last = work->pos;
        egg_viper_spline_point(&energy_path, record->along, &work->pos);
        record->moved.x = work->pos.x - last.x;
        record->moved.y = work->pos.y - last.y;
        record->moved.z = work->pos.z - last.z;
        ang.y = (Angle)(atan2f(record->moved.x, record->moved.z) *
                        ANGLE_PER_RADIAN);
        ang.x = (Angle)(atan2f(-record->moved.y,
                               njSqrt((record->moved.x * record->moved.x) +
                                      (record->moved.z * record->moved.z))) *
                        ANGLE_PER_RADIAN);
        work->ang.x = egg_viper_angle_approach(work->ang.x, ang.x, 0x38e);
        work->ang.y = egg_viper_angle_approach(work->ang.y, ang.y, 0x38e);
        if (record->along >= 1.0F) {
            work->submode = 13;
        } else if (record->along >= 0.7F) {
            record->flags &= ~VIPER_GENTLE_THROW;
        }
        if (viper_was_hit(work, record)) {
            energy_misses = 0;
            hit = 1;
            work->submode = withdraw;
        }
        break;
    case 13:
        gather_create(&work->pos, &work->ang);
        record->flags |= VIPER_GATHERING;
        record->flags |= VIPER_SEGMENTS_HANG;
        work->timer = 0;
        sound_play_held(SE_GATHER, (int)work, 1, 0);
        work->submode = 14;
    case 14:
        if (viper_was_hit(work, record)) {
            energy_misses = 0;
            hit = 1;
            pads_hold(150);
            work->submode = withdraw;
        } else if ((Uint16)++work->timer >= 420) {
            work->submode = 15;
        } else if (stage_get_retries() > 0 && energy_misses >= 3) {
            if ((Uint16)work->timer == 10) {
                hint_show(HINT_GATHER);
            }
        } else if ((Uint16)work->timer == 210) {
            eggman_say(LINE_GATHER);
        }
        break;
    case 15:
        record->motion = motions[EGG_VIPER_WAVE];
        record->shape = egg_viper_motions[EGG_VIPER_WAVE_SHAPE];
        record->frame = zero;
        record->speed = 1.0F;
        work->timer = 0;
        record->flags &= ~VIPER_GATHERING;
        work->submode = 16;
    case 16:
        if (viper_was_hit(work, record)) {
            energy_misses = 0;
            hit = 1;
            pads_hold(150);
            work->submode = withdraw;
        }
        if (motion_play(record)) {
            viper_ball_point(work, &ball);
            ball_create(&ball);
            work->submode = withdraw;
        }
        break;
    case 17:
        record->flags &= ~(VIPER_SEGMENTS_HANG | VIPER_GENTLE_THROW |
                           VIPER_GATHERING | VIPER_BACK_OPEN);
        record->flags |= VIPER_HELD_OFF;
        work->collision->shapes[1].flags &= ~SHAPE_OFF;
        work->collision->shapes[2].flags |= SHAPE_OFF;
        work->collision->shapes[3].flags |= SHAPE_OFF;
        work->collision->shapes[4].flags |= SHAPE_OFF;
        record->target_ang_x = 0xc000;
        sound_stop_id((int)work);
        work->submode = 18;
    case 18:
        motion_rewind(record);
        work->ang.x =
            egg_viper_angle_approach(work->ang.x, record->target_ang_x, 0x2d8);
        work->pos.y += speed;
        if (work->ang.x == record->target_ang_x) {
            work->submode = 19;
        }
        if (viper_was_hit(work, record)) {
            pads_hold(150);
            energy_misses = 0;
            hit = 1;
        }
        break;
    case 19:
        record->target.x = zero;
        record->target.y = -400.0F;
        record->target.z = work->pos.z;
        work->submode = 20;
    case 20:
        if (viper_fly_to(work, record, &record->target, 0x222, speed)) {
            record->flags &= ~VIPER_HELD_OFF;
            viper_next_mode(work, record);
        }
        if (viper_was_hit(work, record)) {
            pads_hold(150);
            energy_misses = 0;
            hit = 1;
        }
        break;
    }
    if (hit) {
        egg_viper_camera_next = TRUE;
    }
}

/* after 30 steps, the boss rises past the far end of player 0's row. it
 * comes in level to 100 past the row's end, where VIPER_BOOM starts */
static void viper_boomp(struct task *task)
{
    struct task_work *player = player_work[0];
    struct egg_viper_record *record = task->extra;
    struct task_work *work = task->work;

    switch (work->submode) {
    case 0:
        record->motion = record->shape = NULL;
        work->flags &= ~COLLISION_DAMAGED;
        work->submode = 1;
    case 1:
        work->timer = 0;
        work->submode = 2;
    case 2:
        if ((Uint16)++work->timer >= 30) {
            work->submode = 3;
        }
        break;
    case 3:
        record->side =
            (player->pos.x < 0.0F ? 0 : 2) | (player->pos.z > 0.0F ? 0 : 1);
        record->target.x = boomp_below[record->side].x;
        record->target.y = boomp_below[record->side].y;
        record->target.z = boomp_below[record->side].z;
        work->submode = 4;
    case 4:
        work->pos.x = record->target.x;
        work->pos.y = record->target.y;
        work->pos.z = record->target.z;
        egg_viper_camera_next = TRUE;
        work->submode = 5;
        break;
    case 5:
        record->target.x = boomp_rise[record->side].x;
        record->target.y = boomp_rise[record->side].y;
        record->target.z = boomp_rise[record->side].z;
        sound_play(SE_RUSH, 0, 0, 0);
        work->submode = 6;
    case 6:
        if (viper_fly_to(work, record, &record->target, 0x4000, 4.0F)) {
            work->submode = 7;
        }
        break;
    case 7:
        record->target.x = boomp_over[record->side].x;
        record->target.y = boomp_over[record->side].y;
        record->target.z = boomp_over[record->side].z;
        work->submode = 8;
    case 8:
        if (viper_fly_to(work, record, &record->target, 0x888, 4.0F)) {
            viper_next_mode(work, record);
        }
        break;
    }
}

/* the boss levels out and unfolds its needles. it opens its front at half
 * speed, and Eggman rises. it creeps along the row toward player 0 at 1 a
 * step. while he is more than 150 ahead, its needles strike in turn. it
 * closes when no panel stands 120 ahead, or when it is hit. it also
 * closes once he is behind it and hit_wait has run out. its needles fold,
 * and 120 steps later it leaves */
static void viper_boom(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    struct task_work *player = player_work[0];

    switch (work->submode) {
    case 0:
        record->motion = record->shape = NULL;
        work->flags &= ~COLLISION_DAMAGED;
        record->side = (player->pos.x < 0.0F ? 0 : 2) |
                       (player->pos.z < work->pos.z ? 0 : 1);
        if (health == 1) {
            eggman_say_once(LINE_ONE_LEFT);
        }
        work->submode = 1;
    case 1:
        work->submode = 2;
    case 2:
        work->ang.x = egg_viper_angle_approach(work->ang.x, 0, 0x16c);
        if (work->ang.x == 0) {
            work->submode = 3;
        }
        break;
    case 3:
        RECORD(egg_viper_needles[0])->flags |= PART_GO;
        RECORD(egg_viper_needles[1])->flags |= PART_GO;
        work->submode = 4;
    case 4:
        if (!(RECORD(egg_viper_needles[0])->flags & PART_GO) &&
            !(RECORD(egg_viper_needles[1])->flags & PART_GO)) {
            work->submode = 5;
        }
        break;
    case 5:
        record->motion = egg_viper_motions[EGG_VIPER_FRONT_OPEN];
        record->shape = NULL;
        record->frame = 0.0F;
        record->speed = 0.5F;
        eggman_boom_create();
        record->flags |= VIPER_FRONT_OPEN;
        sound_play(SE_OPEN, 0, 0, 0);
        work->submode = 6;
    case 6:
        if (motion_play(record)) {
            work->submode = 7;
        }
        break;
    case 7:
        NEEDLE_TURNS(work) = 0;
        record->moved.z = work->pos.z - player->pos.z > 0.0F ? -1.0F : 1.0F;
        eggman_say(LINE_ATTACK);
        work->submode = 8;
    case 8:
        if ((record->moved.z > 0.0F && player->pos.z > work->pos.z + 150.0F) ||
            (record->moved.z < 0.0F && player->pos.z < work->pos.z + -150.0F)) {
            RECORD(egg_viper_needles[NEEDLE_TURNS(work) & 1])->flags |= PART_GO;
        }
        if (stage_get_retries() > 0 && NEEDLE_TURNS(work) == 3 &&
            RECORD(egg_viper_task)->script == FIRST_BOOM_SCRIPT &&
            !(eggman_said & (1 << LINE_RIDDEN))) {
            hint_show(HINT_STRIKES);
        }
        work->submode = 9;
    case 9:
        if (!(RECORD(egg_viper_needles[NEEDLE_TURNS(work) & 1])->flags &
              PART_GO)) {
            NEEDLE_TURNS(work)++;
            work->submode = 8;
        }
        work->pos.z += record->moved.z;
        if ((record->moved.z > 0.0F &&
             floor_standing_at(work->pos.x, work->pos.z + 120.0F) == 0) ||
            (record->moved.z < 0.0F &&
             floor_standing_at(work->pos.x, work->pos.z + -120.0F) == 0) ||
            viper_was_hit(work, record)) {
            work->submode = 12;
        } else if ((record->moved.z > 0.0F && player->pos.z < work->pos.z) ||
                   (record->moved.z < 0.0F && player->pos.z > work->pos.z)) {
            work->submode = 10;
        }
        if (record->flags & VIPER_RIDDEN) {
            eggman_say_once(LINE_RIDDEN);
        }
        break;
    case 10:
        work->submode = 11;
    case 11:
        if (hit_wait <= 0) {
            work->submode = 12;
        }
        break;
    case 12:
        record->motion = egg_viper_motions[EGG_VIPER_FRONT_OPEN];
        record->shape = NULL;
        record->speed = 1.0F;
        RECORD(egg_viper_needles[0])->flags |= PART_RETURN;
        RECORD(egg_viper_needles[1])->flags |= PART_RETURN;
        record->flags &= ~VIPER_FRONT_OPEN;
        sound_play(SE_OPEN, 0, 0, 0);
        work->submode = 13;
    case 13:
        if (motion_rewind(record)) {
            work->submode = 14;
        }
        break;
    case 14:
        work->timer = 0;
        work->submode = 15;
    case 15:
        if ((Uint16)++work->timer >= 120) {
            work->submode = 16;
        }
        break;
    case 16:
        record->target.x = boom_leave[record->side].x;
        record->target.y = boom_leave[record->side].y;
        record->target.z = boom_leave[record->side].z;
        egg_viper_camera_next = TRUE;
        sound_play(SE_RUSH, 0, 0, 0);
        work->submode = 17;
    case 17:
        if (viper_fly_to(work, record, &record->target, 0x888, 4.0F)) {
            work->submode = 18;
        }
        break;
    case 18:
        record->target.x = boom_out[record->side].x;
        record->target.y = boom_out[record->side].y;
        record->target.z = boom_out[record->side].z;
        work->submode = 19;
    case 19:
        work->pos.x = record->target.x;
        work->pos.y = record->target.y;
        work->pos.z = record->target.z;
        viper_next_mode(work, record);
        break;
    }
}

/* the boss waits 60 steps, then sweeps the row across from player 0 along
 * hakaid_path. the sweep runs end to end over 300 steps. it breaks the panel
 * under the boss each step, and sets script_next at the end */
static void viper_hakaid(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    struct task_work *player = player_work[0];
    NJS_POINT3 last;
    struct angles ang;
    int i;

    switch (work->submode) {
    case 0:
        record->motion = NULL;
        record->shape = NULL;
        for (i = 0; i < 6; i++) {
            hakaid_x[i] = player->pos.x > 0.0F ? -FLOOR_ROW_X : FLOOR_ROW_X;
        }
        if (work->pos.z < 0.0F) {
            hakaid_path.z = hakaid_z_up;
        } else {
            hakaid_path.z = hakaid_z_down;
        }
        egg_viper_spline_init(&hakaid_path);
        record->along = 0.0F;
        work->submode = 1;
    case 1:
        work->timer = 0;
        work->submode = 2;
    case 2:
        if ((Uint16)++work->timer >= 60) {
            egg_viper_camera_next = TRUE;
            eggman_say(LINE_SWEEP);
            work->submode = 3;
        }
        break;
    case 3:
        last = work->pos;
        record->along += 1.0F / 300.0F;
        egg_viper_spline_point(&hakaid_path, record->along, &work->pos);
        record->moved.x = work->pos.x - last.x;
        record->moved.y = work->pos.y - last.y;
        record->moved.z = work->pos.z - last.z;
        ang.y = (Angle)(atan2f(record->moved.x, record->moved.z) *
                        ANGLE_PER_RADIAN);
        ang.x = (Angle)(atan2f(-record->moved.y,
                               njSqrt((record->moved.x * record->moved.x) +
                                      (record->moved.z * record->moved.z))) *
                        ANGLE_PER_RADIAN);
        work->ang.x = egg_viper_angle_approach(work->ang.x, ang.x, 0x2d8);
        work->ang.y = egg_viper_angle_approach(work->ang.y, ang.y, 0x2d8);
        if (record->along >= 1.0F) {
            script_next = TRUE;
            viper_next_mode(work, record);
        }
        floor_panel_break(work->pos.x, work->pos.z);
        break;
    }
}

/* the boss searches player 0's row for a standing panel. it starts at the end
 * past the row's middle from him, and searches back toward him. with none, it
 * moves on to its next mode. it waits 90 steps under the first it finds. it
 * then rises through the seam on that panel's side toward him. it breaks the
 * panels either side, and shakes the camera and the pad */
static void viper_hakai(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    struct task_work *player = player_work[0];
    Float middle;
    /* @hack spelling: limit, a local for the 420.0 the image keeps in FR15
     * (0x0c902f7e); as a literal, the function ends 2 bytes early and 85 of
     * 423 words differ */
    Float limit;
    /* @hack spelling: floor_standing_at called through a local, which keeps
     * its address in R11 (0x0c902f78) for both calls (0x0c902f86,
     * 0x0c903026); called by name, 90 of 423 words differ */
    int (*standing)(Float, Float);

    switch (work->submode) {
    case 0:
        record->shape = NULL;
        record->motion = NULL;
        work->flags &= ~COLLISION_DAMAGED;
        work->submode = 1;
    case 1:
        middle =
            floor_row_middle(player->pos.x < 0.0F ? -FLOOR_ROW_X : FLOOR_ROW_X);
        if (middle == FLOOR_NONE_STANDING) {
            goto none;
        }
        record->side =
            (player->pos.x < 0.0F ? 0 : 2) | (middle > player->pos.z ? 0 : 1);
        record->target.x = !(record->side & 2) ? -FLOOR_ROW_X : FLOOR_ROW_X;
        record->target.y = -200.0F;
        record->target.z =
            !(record->side & 1) ? FLOOR_LAST_PANEL_Z : -FLOOR_LAST_PANEL_Z;
        standing = floor_standing_at;
        limit = FLOOR_LAST_PANEL_Z;
        /* @hack redundant: a never-run store of limit, which keeps it in
         * FR15 through the search; without it, 55 of 423 words differ */
        if (0) {
            record->target.z = limit;
        }
        /* @hack spelling: the search written with gotos, so that it leaves
         * through none, the exit a row with no standing panel takes; as a
         * while, 43 of 423 words differ */
        if (standing(record->target.x, record->target.z) != 0) {
            goto found;
        }
    next:
        record->target.z +=
            !(record->side & 1) ? -FLOOR_PANEL_LENGTH : FLOOR_PANEL_LENGTH;
        if (record->target.z > limit ||
            record->target.z < -FLOOR_LAST_PANEL_Z) {
        none:
            viper_next_mode(work, record);
            return;
        }
        if (standing(record->target.x, record->target.z) == 0) {
            goto next;
        }
    found:
        work->submode = 2;
    case 2:
        work->pos.x = record->target.x;
        work->pos.y = record->target.y;
        work->pos.z = record->target.z;
        work->submode = 3;
        break;
    case 3:
        work->timer = 0;
        egg_viper_camera_next = TRUE;
        work->submode = 4;
    case 4:
        if ((Uint16)++work->timer >= 90) {
            work->submode = 5;
        }
        break;
    case 5:
        record->target.x = work->pos.x;
        record->target.y = -100.0F;
        record->target.z = work->pos.z + (!(record->side & 1) ? -60.0F : 60.0F);
        work->ang.x = 0xc000;
        work->ang.y = !(record->side & 1) ? 0x8000 : 0;
        sound_play(SE_RUSH, 0, 0, 0);
        work->submode = 6;
    case 6:
        if (move_to(&work->pos, &record->target, 4.0F)) {
            work->submode = 9;
        }
        break;
    case 9:
        record->target.x = work->pos.x;
        record->target.y = 80.0F;
        record->target.z = work->pos.z;
        floor_panel_break(work->pos.x, work->pos.z + 60.0F);
        egg_viper_camera_shake();
        vibration_start(0, 7, 59, 1);
        work->submode = 10;
        break;
    case 10:
        floor_panel_break(work->pos.x, work->pos.z - 60.0F);
        work->submode = 11;
    case 11:
        if (move_to(&work->pos, &record->target, 4.0F)) {
            viper_next_mode(work, record);
            return;
        }
        break;
    }
}

/* the boss's defeat. its needles fall, its music stops, and palette 0's
 * red pulses. after 60 steps with the pads held, it flies to y 50 over x
 * 0. it sweeps along the arena as its segments fall, turning at either end.
 * after its third turn it leaps onto player 0's row, breaking a panel, and
 * falls into the depths. its pillars of fire rise, and 300 steps later the
 * stage is clear. it rolls all the while */
static void viper_break(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    struct task_work *player = player_work[0];
    /* @hack type: last a point, of which only y is set and read, which
     * makes the frame 28 bytes (ADD #-28,R15 at 0x0c903202); as a Float, it
     * is 20 and 29 of 770 words differ */
    NJS_POINT3 last;

    switch (work->submode) {
    case 0:
        record->shape = NULL;
        record->motion = NULL;
        work->flags &= ~COLLISION_DAMAGED;
        task_create(TASK_WORK, 2, parts_fall_step);
        task_create(TASK_WORK, 2, defeat_flash_step);
        record->flags |= VIPER_BREAKING;
        stream_stop_music();
        work->submode = 1;
    case 1:
        work->timer = 60;
        pad_disable();
        work->submode = 2;
    case 2:
        if ((Uint16)--work->timer == 0) {
            work->submode = 3;
        }
        break;
    case 3:
        record->target.x = 0.0F;
        record->target.y = 50.0F;
        record->target.z = work->pos.z;
        pad_enable();
        work->submode = 4;
    case 4:
        if (viper_fly_to(work, record, &record->target, 0x71c, 4.0F)) {
            work->submode = 5;
        }
        break;
    case 5:
        work->timer = 0;
        record->target.x = work->pos.x;
        record->target.y = work->pos.y;
        record->target.z = work->pos.z;
        record->moved.x = 0.0F;
        record->moved.z = work->pos.z > 0.0F ? -4.0F : 4.0F;
        record->phase = 0;
        task_create(TASK_WORK, 2, segments_fall_step);
        work->submode = 6;
    case 6:
        last.y = work->pos.y;
        record->phase = (Uint16)(record->phase + 0x400);
        work->pos.y = record->target.y + (20.0F * njSin(record->phase));
        work->pos.z += record->moved.z;
        if (work->pos.z >= FLOOR_ROW_END_Z) {
            work->pos.z = FLOOR_ROW_END_Z;
            record->moved.z = -record->moved.z;
            work->timer++;
            if ((Uint16)work->timer == 2) {
                hint_show(HINT_BREAK);
            }
        } else if (work->pos.z <= -FLOOR_ROW_END_Z) {
            work->pos.z = -FLOOR_ROW_END_Z;
            record->moved.z = -record->moved.z;
            work->timer++;
            if ((Uint16)work->timer == 2) {
                hint_show(HINT_BREAK);
            }
        }
        record->moved.y = work->pos.y - last.y;
        work->ang.y = egg_viper_angle_approach(
            work->ang.y,
            (Angle)(atan2f(record->moved.x, record->moved.z) *
                    ANGLE_PER_RADIAN),
            0x5b0);
        work->ang.x = egg_viper_angle_approach(
            work->ang.x,
            (Angle)(atan2f(-record->moved.y,
                           njSqrt((record->moved.x * record->moved.x) +
                                  (record->moved.z * record->moved.z))) *
                    ANGLE_PER_RADIAN),
            0x5b0);
        if ((Uint16)work->timer >= 3) {
            work->submode = 7;
        }
        break;
    case 7:
        record->target.x = player->pos.x > 0.0F ? FLOOR_ROW_X : -FLOOR_ROW_X;
        record->target.y = 15.0F;
        record->target.z =
            -FLOOR_ROW_END_Z + (((int)((player->pos.z + FLOOR_ROW_END_Z) *
                                       (1.0F / FLOOR_PANEL_LENGTH)) +
                                 1) *
                                FLOOR_PANEL_LENGTH);
        /* the leap is an arc over 120 steps. its rise starts GRAVITY times
         * half the steps above the straight line's. so the arc ends about
         * where the line does */
        record->moved.y = ((record->target.y - work->pos.y) * (1.0F / 120.0F)) +
                          (GRAVITY * 60.0F);
        record->moved.x = (record->target.x - work->pos.x) * (1.0F / 120.0F);
        record->moved.z = (record->target.z - work->pos.z) * (1.0F / 120.0F);
        work->timer = 0;
        egg_viper_camera_next = TRUE;
        sound_play(SE_RUSH, 0, 0, 0);
        work->submode = 8;
    case 8:
        record->moved.y += -GRAVITY;
        work->pos.x += record->moved.x;
        work->pos.y += record->moved.y;
        work->pos.z += record->moved.z;
        work->ang.y = egg_viper_angle_approach(
            work->ang.y,
            (Angle)(atan2f(record->moved.x, record->moved.z) *
                    ANGLE_PER_RADIAN),
            0x5b0);
        work->ang.x = egg_viper_angle_approach(
            work->ang.x,
            (Angle)(atan2f(-record->moved.y,
                           njSqrt((record->moved.x * record->moved.x) +
                                  (record->moved.z * record->moved.z))) *
                    ANGLE_PER_RADIAN),
            0x5b0);
        if ((Uint16)++work->timer >= 120) {
            work->pos.x = record->target.x;
            work->pos.y = record->target.y;
            work->pos.z = record->target.z;
            floor_panel_break(work->pos.x, work->pos.z);
            egg_viper_camera_shake();
            vibration_start(0, 7, 59, 1);
            work->submode = 9;
        }
        break;
    case 9:
        record->target.x = 0.0F;
        record->target.y = -400.0F;
        record->target.z = 0.0F;
        /* the fall is an arc too, over 180 steps */
        record->moved.y = ((record->target.y - work->pos.y) * (1.0F / 180.0F)) +
                          (GRAVITY * 90.0F);
        record->moved.x = (record->target.x - work->pos.x) * (1.0F / 180.0F);
        record->moved.z = (record->target.z - work->pos.z) * (1.0F / 180.0F);
        work->timer = 0;
        work->submode = 10;
    case 10:
        record->moved.y += -GRAVITY;
        work->pos.x += record->moved.x;
        work->pos.y += record->moved.y;
        work->pos.z += record->moved.z;
        if ((Uint16)++work->timer >= 180) {
            work->submode = 11;
        }
        break;
    case 11:
        pillars_create(&work->pos);
        record->flags |= VIPER_GONE;
        pad_disable();
        time_stop();
        work->timer = 0;
        work->submode = 12;
    case 12:
        if ((Uint16)++work->timer >= 300) {
            work->submode = 13;
        }
        break;
    case 13:
        pad_enable();
        stage_clear();
        work->submode = 14;
    case 14:
        break;
    }
    work->ang.z = (Uint16)(work->ang.z + 0x800);
}

/* the test mode VIPER_MAX. the boss stands at y 50 over the arena's
 * middle. it plays its three motions in turn and prints each one's name,
 * then starts over */
static void viper_max(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    /* @hack spelling: njPrintC called through a local, which keeps its
     * address in R12 (0x0c903804) for the three calls; called by name, 16
     * of 141 words differ */
    void (*print)(Int, const char *) = njPrintC;
    NJS_MOTION **motions;

    /* @hack spelling: motions set in the switch's selector, which loads the
     * table at 0x0c903802 and copies it in the branch's delay slot
     * (0x0c90380c); set at its declaration, 5 of 141 words differ */
    switch (motions = egg_viper_motions, work->submode) {
    case 0:
        record->shape = NULL;
        record->motion = NULL;
        work->pos.x = 0.0F;
        work->pos.y = 50.0F;
        work->pos.z = 0.0F;
        record->speed = 1.0F;
        work->submode = 1;
    case 1:
        /* @hack spelling: case 1's reads and the shapes go through
         * egg_viper_motions, in R4 from 0x0c903802 (0x0c903882, 0x0c90388a,
         * 0x0c9038dc), and the rest through its copy motions, in R6; with
         * case 1's reads or both shapes through motions, 6 of 141 words
         * differ, with either shape alone 18, and with all three 19 */
        record->motion = egg_viper_motions[EGG_VIPER_BACK_OPEN];
        record->shape = egg_viper_motions[EGG_VIPER_BACK_OPEN_SHAPE];
        record->frame = 0.0F;
        work->submode = 2;
    case 2:
        print(0, TEXT_19);
        if (motion_play(record)) {
            work->submode = 3;
        }
        break;
    case 3:
        record->motion = motions[EGG_VIPER_FRONT_OPEN];
        record->shape = NULL;
        record->frame = 0.0F;
        work->submode = 4;
    case 4:
        print(0, TEXT_20);
        if (motion_play(record)) {
            work->submode = 5;
        }
        break;
    case 5:
        record->motion = motions[EGG_VIPER_WAVE];
        record->shape = egg_viper_motions[EGG_VIPER_WAVE_SHAPE];
        record->frame = 0.0F;
        work->submode = 6;
    case 6:
        print(0, TEXT_21);
        if (motion_play(record)) {
            work->submode = 7;
        }
        break;
    default:
        work->submode = 0;
        break;
    }
}

/* draw the boss's tree in its motion and shape motion, where it has them.
 * it draws nothing once VIPER_GONE is set, or while viper_shown is FALSE */
static void viper_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    Angle ang;

    if ((record->flags & VIPER_GONE) || !viper_shown) {
        return;
    }
    draw_attr_save();
    njSetTexture(&egm3mdl_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    if (!record->motion) {
        if (!frame_step) {
            draw_tree(egg_viper_objects[OBJECT_VIPER]);
        }
    } else if (!record->shape) {
        if (!frame_step) {
            draw_motion(egg_viper_objects[OBJECT_VIPER], record->motion,
                        record->frame);
        }
    } else {
        if (!frame_step) {
            draw_shape_motion(egg_viper_objects[OBJECT_VIPER], record->motion,
                              record->shape, record->frame);
        }
    }
    njPopMatrix(1);
    draw_attr_restore();
}

static void viper_destroy(struct task *task)
{
}

/* a needle's step. the needle sits folded in its socket, spinning, until
 * the boss tells it to unfold. it then holds, strikes and folds back as
 * the boss tells it. it falls as the boss breaks */
void egg_viper_needle_step(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: egg_viper_task's work, whose address the image forms
     * at 0x0c903a8a and 0x0c903a92 without loading it; without it, 62 of
     * 260 words differ */
    struct task_work *boss_work = egg_viper_task->work;
    struct lockon_target *target = LOCKON_TARGET(task);
    struct egg_viper_record *record = task->extra;
    struct egg_viper_record *boss = RECORD(egg_viper_task);
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int mode = work->mode;
    NJS_MATRIX *matrix;

    switch (work->mode) {
    case PART_INIT:
        task->destructor = part_destroy;
        task->draw = part_draw;
        collision_init(task, part_shapes, COLLISION_OBJECT, 1, 1);
        work->mode = PART_READY;
        break;
    case PART_READY:
        work->ang.y = 0;
        if (work->tick == 0) {
            work->ang.z = 0x4000;
            record->spin = -0x200;
        } else {
            work->ang.z = 0xc000;
            record->spin = 0x200;
        }
        record->flags = 0;
        work->mode = PART_FOLDED;
    case PART_FOLDED:
        if (work->tick == 0) {
            matrix = &egg_viper_matrices[NODE_PART];
        } else {
            matrix = &egg_viper_matrices[NODE_PART + 1];
        }
        work->pos.x = (*matrix)[12];
        work->pos.y = (*matrix)[13];
        work->pos.z = (*matrix)[14];
        if (record->flags & PART_GO) {
            work->mode = PART_UNFOLD;
        }
        break;
    case PART_UNFOLD:
        part_unfold(task);
        break;
    case PART_HOLD:
        part_hold(task);
        break;
    case PART_STRIKE:
        part_strike(task);
        break;
    case PART_FOLD:
        part_fold(task);
        break;
    case PART_FALL:
        part_fall(task);
        break;
    }
    work->ang.y += record->spin;
    if (record->flags & FALL) {
        record->flags &= ~FALL;
        work->mode = PART_FALL;
    }
    if (mode != work->mode) {
        work->submode = 0;
    }
    if (task->step != task_destroy && !(boss->flags & VIPER_HELD_OFF) &&
        work->mode != PART_FOLD) {
        collision_register(work);
        enemy_save_pos(work, target);
    }
    part_draw(task);
}

/* the needle spins up to 0x400 a step. it moves 100 out along z and 40 down,
 * and rolls upright. ten steps later its shape lengthens downward, and it makes
 * the top player 0 can ride. it then holds */
static void part_unfold(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: egg_viper_task's work, read at 0x0c903c9c and dropped;
     * without it, 88 of 257 words differ */
    struct task_work *boss_work = egg_viper_task->work;
    struct egg_viper_record *record = task->extra;
    struct egg_viper_record *boss = RECORD(egg_viper_task);
    struct task *child;

    switch (work->submode) {
    case 0:
        if (work->tick == 0) {
            sound_play_held(SE_NEEDLE_SPIN, (int)work, 1, 0);
        }
        work->submode++;
    case 1:
        if (record->spin > 0) {
            record->spin += 32;
            if (record->spin >= 0x400) {
                record->spin = 0x400;
                work->submode++;
            }
        } else {
            record->spin -= 32;
            if (record->spin <= -0x400) {
                record->spin = -0x400;
                work->submode++;
            }
        }
        break;
    case 2:
        record->moved.z = !(boss->side & 1) ? -10.0F : 10.0F;
        work->timer = 0;
        work->submode++;
    case 3:
        work->pos.z += record->moved.z;
        if ((Uint16)++work->timer >= 10) {
            work->submode++;
        }
        break;
    case 4:
        record->moved.y = -4.0F;
        work->timer = 0;
        work->submode++;
    case 5:
        work->pos.y += record->moved.y;
        if ((Uint16)++work->timer >= 10) {
            work->submode++;
        }
        break;
    case 6:
        record->roll = work->tick == 0 ? -0x200 : 0x200;
        work->submode++;
    case 7:
        work->ang.z = (Uint16)(work->ang.z + (Sint16)record->roll);
        if (work->ang.z == 0) {
            work->submode++;
        }
        break;
    case 8:
        work->timer = 0;
        work->submode++;
    case 9:
        if ((Uint16)++work->timer >= 10) {
            record->flags &= ~PART_GO;
            work->collision->shapes->center.y = -10.0F;
            work->collision->shapes->size_y = 10.0F;
            child = task_create_child(TASK_WORK, part_top_step, task);
            child->extra = heap_calloc(1, sizeof(struct egg_viper_record));
            child->work->tick = work->tick;
            work->mode = PART_HOLD;
        }
        break;
    }
}

/* the needle holds 40 below its socket and 100 out along z, and runs its
 * top's step. it strikes or folds when the boss tells it */
static void part_hold(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    /* @hack redundant: egg_viper_task's work, read at 0x0c903e94 and dropped;
     * without it, 81 of 143 words differ */
    struct task_work *boss_work = egg_viper_task->work;
    struct egg_viper_record *boss = RECORD(egg_viper_task);
    NJS_MATRIX *matrices;
    NJS_MATRIX *matrix;

    switch (work->submode) {
    case 0:
        record->target.x = 0.0F;
        record->target.y = -40.0F;
        record->target.z = !(boss->side & 1) ? -100.0F : 100.0F;
        work->submode++;
        break;
    }
    matrices = egg_viper_matrices;
    /* @hack redundant: a never-run test of matrices, which keeps the base
     * in R14 (0x0c903f10); without it, 42 of 143 words differ */
    if (0) {
        if (matrices) {
        }
    }
    matrix = !work->tick ? matrices + NODE_PART : matrices + NODE_PART + 1;
    /* @hack redundant: a never-run test of matrix, which moves it into R14
     * (0x0c903f2a); without it, 77 of 143 words differ */
    if (0) {
        if (matrix) {
        }
    }
    work->pos.x = (*matrix)[12] + record->target.x;
    work->pos.y = (*matrix)[13] + record->target.y;
    work->pos.z = (*matrix)[14] + record->target.z;
    if (record->flags & PART_GO) {
        work->mode = PART_STRIKE;
    }
    if (record->flags & PART_RETURN) {
        record->flags &= ~PART_RETURN;
        work->mode = PART_FOLD;
    } else {
        task_step_children(task);
    }
}

/* the needle draws back 80 along z. it then strikes at 10 a step to where
 * player 0 stood, at y 30. it stays there 60 steps, returns to its hold and
 * clears PART_GO */
static void part_strike(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: egg_viper_task's work, read at 0x0c903fbe and dropped;
     * without it, 110 of 288 words differ */
    struct task_work *boss_work = egg_viper_task->work;
    struct task_work *player = player_work[0];
    struct egg_viper_record *record = task->extra;
    struct egg_viper_record *boss = RECORD(egg_viper_task);
    NJS_MATRIX *matrix;

    switch (work->submode) {
    case 0:
        record->target.x = work->pos.x;
        record->target.y = work->pos.y;
        record->target.z = work->pos.z + (!(boss->side & 1) ? 80.0F : -80.0F);
        work->submode++;
    case 1:
        if (move_to(&work->pos, &record->target, 3.0F)) {
            work->submode++;
        }
        break;
    case 2:
        record->target.x = player->pos.x;
        record->target.y = 30.0F;
        record->target.z = player->pos.z;
        record->unk_18 = work->pos.x;
        record->unk_1c = work->pos.y;
        record->unk_20 = !(boss->side & 1) ? -100.0F : 100.0F;
        sound_play(SE_NEEDLE_STRIKE, 0, 0, 0);
        work->submode++;
    case 3:
        if (move_to(&work->pos, &record->target, 10.0F)) {
            work->submode++;
        }
        break;
    case 4:
        work->timer = 0;
        work->submode++;
    case 5:
        if ((Uint16)++work->timer >= 60) {
            work->submode++;
        }
        break;
    case 6:
        work->submode++;
    case 7:
        /* @hack spelling: matrix set to the base, then moved on, which loads
         * the base by one pool word (0x0c9040fe) and adds the node's offset to
         * it (0x0c904144); as one ?: of the two nodes' addresses, each takes
         * a pool word, the step ends 58 bytes early and 9,076 of the
         * object's 16,560 words differ */
        matrix = egg_viper_matrices;
        matrix = !work->tick ? matrix + NODE_PART : matrix + NODE_PART + 1;
        record->target.x = (*matrix)[12];
        record->target.y = (*matrix)[13] + -40.0F;
        record->target.z =
            (*matrix)[14] + (!(boss->side & 1) ? -100.0F : 100.0F);
        if (move_to(&work->pos, &record->target, 10.0F)) {
            record->flags &= ~PART_GO;
            work->mode = PART_HOLD;
        }
        break;
    }
    task_step_children(task);
}

/* the needle spins down to 0x200 a step, rises 40 and rolls back on its
 * side. it moves 100 back along z into its socket, its shape shrinks back
 * and its top goes. it is then ready to unfold again */
static void part_fold(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: egg_viper_task's work, read at 0x0c9041f4 and dropped;
     * without it, 71 of 245 words differ */
    struct task_work *boss_work = egg_viper_task->work;
    struct egg_viper_record *record = task->extra;
    struct egg_viper_record *boss = RECORD(egg_viper_task);
    int part;

    switch (work->submode) {
    case 0:
        work->submode++;
    case 1:
        if (record->spin > 0) {
            record->spin -= 32;
            if (record->spin <= 0x200) {
                record->spin = 0x200;
                work->submode++;
            }
        } else {
            record->spin += 32;
            if (record->spin >= -0x200) {
                record->spin = -0x200;
                work->submode++;
            }
        }
        break;
    case 2:
        record->moved.y = 4.0F;
        work->timer = 0;
        sound_stop_id((int)work);
        work->submode++;
    case 3:
        work->pos.y += record->moved.y;
        if ((Uint16)++work->timer >= 10) {
            work->submode++;
        }
        break;
    case 4:
        record->roll = work->tick == 0 ? 0x200 : -0x200;
        work->submode++;
    case 5:
        work->ang.z = (Uint16)(work->ang.z + (Sint16)record->roll);
        /* @hack spelling: the tick read once into part, which widens it once
         * (EXTU.B at 0x0c904306) for both tests; as work->tick at each, it
         * widens twice and 19 of 245 words differ */
        part = work->tick;
        if ((!part && work->ang.z == 0x4000) ||
            (part == 1 && work->ang.z == 0xc000)) {
            work->submode++;
        }
        break;
    case 6:
        record->moved.z = !(boss->side & 1) ? 10.0F : -10.0F;
        work->timer = 0;
        work->submode++;
    case 7:
        work->pos.z += record->moved.z;
        if ((Uint16)++work->timer >= 10) {
            work->submode++;
        }
        break;
    default:
        work->collision->shapes->center.y = 0.0F;
        work->collision->shapes->size_y = 5.0F;
        task_destroy_children(task);
        work->mode = PART_READY;
        break;
    }
}

/* the needle hops up in a bomb's puff and falls. it dies below y -200 */
static void part_fall(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;

    switch (work->submode) {
    case 0:
        bomb_create(&work->pos, 1.0F);
        record->moved.y = 3.0F;
        work->submode++;
    case 1:
        record->moved.y += -GRAVITY;
        work->pos.y += record->moved.y;
        if (work->pos.y < -200.0F) {
            task_kill(task);
        }
        break;
    }
}

/* draw the needle turned as the boss is. its own roll about z and spin about y
 * follow */
static void part_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *boss = egg_viper_task->work;
    Angle ang;

    draw_attr_save();
    njSetTexture(&egm3mdl_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, boss->ang.z, ang);
    TURN_VIA(njRotateY, boss->ang.y, ang);
    TURN_VIA(njRotateX, boss->ang.x, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (!frame_step) {
        draw_tree(egg_viper_objects[OBJECT_PART]);
    }
    njPopMatrix(1);
    draw_attr_restore();
}

static void part_destroy(struct task *task)
{
}

/* a task that tells each needle to fall, 30 steps apart, and then ends */
static void parts_fall_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case 0:
        task->destructor = NULL;
        task->draw = NULL;
        work->tick = 0;
        work->timer = 0;
        work->mode++;
    case 1:
        if (!work->timer) {
            work->timer = 30;
            RECORD(egg_viper_needles[work->tick])->flags |= FALL;
            if (++work->tick >= EGG_VIPER_NEEDLES) {
                task_kill(task);
            }
        }
        work->timer--;
        break;
    }
}

/* segment_gap places a starting segment 20 behind the one before it,
 * turned with the boss about y. segment_back points the way a stopped
 * segment hangs back from the one before. the boss's turn about y and the
 * segment's own tilt about x turn it. the segment hangs 20 along it */
/* @hack spelling: defined past viper_max, which lays them out after
 * viper_wait's and viper_max's strings among the constants (0x0c958c34);
 * defined with the data above, they come before those strings
 * (0x0c958c08), and 16 of the constants' 118 words and 6 of the code's
 * differ */
static const NJS_POINT3 segment_gap = {0.0F, 0.0F, -20.0F};
static const NJS_POINT3 segment_back = {0.0F, 0.0F, -1.0F};

/* the step of a needle's top, a child of the needle's task. while player 0
 * stands on it, it carries him round with the needle's spin and sets
 * VIPER_RIDDEN. while he doesn't, it clears VIPER_RIDDEN */
static void part_top_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *part_work = egg_viper_needles[work->tick]->work;
    struct task_work *player = player_work[0];
    struct egg_viper_record *record = task->extra;
    struct egg_viper_record *part = RECORD(egg_viper_needles[work->tick]);
    NJS_POINT3 last;
    NJS_POINT3 relative;
    NJS_POINT3 turned;
    struct collision_shape *shape;
    Angle ang;

    switch (work->mode) {
    case 0:
        task->destructor = part_top_destroy;
        task->draw = part_top_draw;
        collision_init(task, &part_top_shape, COLLISION_OBJECT, 1, 1);
        shape = work->collision->shapes;
        shape->center.y = part_work->collision->shapes->center.y +
                          part_work->collision->shapes->size_y + shape->size_y;
        record->flags = 0;
        work->mode++;
    }
    last = work->pos;
    work->pos.x = part_work->pos.x;
    work->pos.y = part_work->pos.y;
    work->pos.z = part_work->pos.z;
    if (work->flags & COLLISION_UNDER) {
        relative.x = player->pos.x - last.x;
        relative.y = 0.0F;
        relative.z = player->pos.z - last.z;
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        TURN_VIA(njRotateY, part->spin, ang);
        njCalcPoint(NULL, &relative, &turned);
        njPopMatrix(1);
        player_set_position(0, work->pos.x + turned.x,
                            work->pos.y + work->collision->shapes->center.y +
                                work->collision->shapes->size_y - 0.1F,
                            work->pos.z + turned.z);
        player->ang.y = (Uint16)(player->ang.y - (Sint16)part->spin);
        player_motion[0]->aim.y = player->ang.y;
        work->flags &= ~COLLISION_UNDER;
        RECORD(egg_viper_task)->flags |= VIPER_RIDDEN;
    } else {
        RECORD(egg_viper_task)->flags &= ~VIPER_RIDDEN;
    }
    /* @hack redundant: a jump to the next line, which loads the step into R3
     * and task_destroy's address into R0 (0x0c9047b0, 0x0c9047b2); without it,
     * 5 of 231 words differ */
    goto check;
check:
    if (task->step != task_destroy) {
        collision_register(work);
    }
}

static void part_top_draw(struct task *task)
{
}

static void part_top_destroy(struct task *task)
{
}

/* a segment's step. the segment trails 20 behind the one before it, facing it.
 * segment 0 trails the boss's node 47. a hit on a joint throws player 0 up. the
 * joint then lets no hit register for 60 steps. while VIPER_FIRING is set, a
 * joint fires as it tops its arc above y 0. the segment falls once FALL is set
 */
void egg_viper_segment_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *boss_work = egg_viper_task->work;
    struct egg_viper_record *boss = RECORD(egg_viper_task);
    struct lockon_target *target = LOCKON_TARGET(task);
    struct egg_viper_record *record = task->extra;
    int index = work->tick & EGG_VIPER_SEGMENT_NUMBER;
    int kind = work->tick & EGG_VIPER_SEGMENT_KIND;
    NJS_POINT3 anchor;
    struct angles anchor_ang;
    NJS_POINT3 direction;
    /* @hack redundant: unused, 12 bytes of the frame at sp+72 that no
     * instruction reads or writes. the frame's 120 bytes (ADD #-120,R15 at
     * 0x0c9047ea) hold spills from sp+0 to sp+24, then the locals turned at
     * sp+24, local at sp+36, last_moved at sp+48 and last at sp+60. after
     * unused come direction at sp+84, anchor_ang at sp+96 and anchor at
     * sp+108; without unused, 33 of 837 words differ */
    NJS_POINT3 unused;
    NJS_POINT3 last;
    NJS_VECTOR last_moved;
    NJS_POINT3 local;
    NJS_POINT3 turned;
    NJS_POINT3 *point;
    struct angles *angles;
    Angle ang;
    Float direction_x;
    Float direction_y;
    Float moved_x;

    switch (work->mode) {
    case SEGMENT_INIT:
        task->destructor = segment_destroy;
        task->draw = segment_draw;
        switch (kind) {
        case EGG_VIPER_PLAIN:
            collision_init(task, &segment_shape, COLLISION_OBJECT, 1, 1);
            break;
        case EGG_VIPER_JOINT:
            collision_init(task, &joint_shape, COLLISION_ITEM, 1, 1);
            break;
        case EGG_VIPER_TAIL:
            collision_init(task, &tail_shape, COLLISION_OBJECT, 1, 1);
            break;
        }
        if (index == 0) {
            NJS_MATRIX *matrix = &egg_viper_anchor_matrix;
            work->pos.x = (*matrix)[12];
            work->pos.y = (*matrix)[13];
            work->pos.z = (*matrix)[14];
        } else {
            local = segment_gap;
            njPushMatrix(NULL);
            njUnitMatrix(NULL);
            TURN_VIA(njRotateY, boss_work->ang.y, ang);
            njCalcPoint(NULL, &local, &turned);
            njPopMatrix(1);
            work->pos.x = egg_viper_segment_pos[index - 1]->x + turned.x;
            work->pos.y = egg_viper_segment_pos[index - 1]->y + turned.y;
            work->pos.z = egg_viper_segment_pos[index - 1]->z + turned.z;
        }
        HIT_PAUSE(work) = 0;
        work->mode = SEGMENT_FOLLOW;
        break;
    case SEGMENT_FOLLOW:
        last = work->pos;
        if (index == 0) {
            NJS_MATRIX *matrix = &egg_viper_anchor_matrix;
            anchor.x = (*matrix)[12];
            anchor.y = (*matrix)[13];
            anchor.z = (*matrix)[14];
            anchor_ang.x = boss_work->ang.x;
            anchor_ang.y = boss_work->ang.y;
            anchor_ang.z = boss_work->ang.z;
            point = &anchor;
            angles = &anchor_ang;
        } else {
            point = egg_viper_segment_pos[index - 1];
            angles = egg_viper_segment_ang[index - 1];
        }
        direction.x = point->x - work->pos.x;
        direction.y = point->y - work->pos.y;
        direction.z = point->z - work->pos.z;
        /* @hack spelling: direction's x and y read through locals, which take
         * them into FR5 and FR4 for the length test (0x0c904a18,
         * 0x0c904a1c); as direction.x and direction.y, they take FR6 and FR5
         * and 8 of 837 words differ */
        direction_x = direction.x;
        direction_y = direction.y;
        if ((direction_x * direction_x) + (direction_y * direction_y) +
                (direction.z * direction.z) <
            0.001F) {
            direction.x = 0.0F;
            direction.y = 0.0F;
            direction.z = -1.0F;
        }
        work->ang.x =
            (Angle)(atan2f(-direction.y, njSqrt((direction.x * direction.x) +
                                                (direction.z * direction.z))) *
                    ANGLE_PER_RADIAN);
        if (!(direction.x * direction.x > 0.1F) &&
            !(direction.z * direction.z > 0.1F)) {
            work->ang.y = angles->y;
        } else {
            work->ang.y =
                (Angle)(atan2f(direction.x, direction.z) * ANGLE_PER_RADIAN);
        }
        njUnitVector(&direction);
        work->pos.x = point->x - (direction.x * 20.0F);
        work->pos.y = point->y - (direction.y * 20.0F);
        work->pos.z = point->z - (direction.z * 20.0F);
        /* @hack redundant: a never-run store of 4, which keeps 4 out of
         * R12 across the laser's call; without it, the function starts 42
         * bytes late and 799 of 837 words differ */
        if (0) {
            work->mode = 4;
        }
        /* while VIPER_SEGMENTS_HANG is set, a segment that moved less than 0.1
         * this step hangs back. its pitch turns toward 0xf400 */
        if (boss->flags & VIPER_SEGMENTS_HANG) {
            /* @hack spelling: local through delta, which keeps its address in
             * R4 for the three stores (0x0c904b44); by name, 128 of 837
             * words differ */
            NJS_POINT3 *delta = &local;
            delta->x = work->pos.x - last.x;
            delta->y = work->pos.y - last.y;
            delta->z = work->pos.z - last.z;
            /* @hack spelling: delta's x read through moved_x, which keeps the
             * frame at 120 bytes (ADD #-120,R15 at 0x0c9047ea); as delta->x,
             * it is 124, and the step and the code after it end 32 bytes
             * late, past the object's end at 0x0c9089a0, count unmeasured */
            moved_x = delta->x;
            if ((moved_x * moved_x) + (delta->y * delta->y) +
                    (delta->z * delta->z) <
                0.009999998F) {
                turned = segment_back;
                work->ang.x =
                    egg_viper_angle_approach(work->ang.x, 0xf400, 0x5b0);
                njPushMatrix(NULL);
                njUnitMatrix(NULL);
                TURN_VIA(njRotateY, boss_work->ang.y, ang);
                TURN_VIA(njRotateX, work->ang.x, ang);
                njCalcPoint(NULL, &turned, &direction);
                njPopMatrix(1);
                work->pos.x = point->x + (direction.x * 20.0F);
                work->pos.y = point->y + (direction.y * 20.0F);
                work->pos.z = point->z + (direction.z * 20.0F);
            }
        }
        last_moved.y = record->moved.y;
        record->moved.x = work->pos.x - last.x;
        record->moved.y = work->pos.y - last.y;
        record->moved.z = work->pos.z - last.z;
        if (kind == EGG_VIPER_JOINT) {
            if (boss->flags & VIPER_FIRING) {
                if (work->pos.y > 0.0F) {
                    if (last_moved.y > 0.0F) {
                        if (!(record->moved.y > 0.0F)) {
                            laser_create(&work->pos, LASER_JOINT);
                        }
                    }
                }
            }
        }
        if (kind == EGG_VIPER_JOINT) {
            /* @hack spelling: damaged, one local for the hit test's mask,
             * COLLISION_DAMAGED, and for the kind COLLISION_OBJECT a held-off
             * joint takes. both are 4, which the image loads into R4 once
             * (0x0c904cda) for both (0x0c904ce4, 0x0c904d34); as the two
             * names, with the dead code below that serves it, 51 of 837 words
             * differ */
            int damaged = COLLISION_DAMAGED;

            /* @hack redundant: a never-run store of damaged, which keeps
             * it in R4 for both branches; without it, 25 of 837 words
             * differ */
            if (0) {
                damaged = COLLISION_DAMAGED;
            }
            /* @hack redundant: a never-run test of damaged, which gives
             * the hold's test R2 (0x0c904cd4); without it, 2 of 837 words
             * differ */
            if (0) {
                if (damaged) {
                }
            }
            if (HIT_PAUSE(work) == 0) {
                work->collision->kind = COLLISION_ITEM;
                if (work->flags & damaged) {
                    player_set_speed(0, 0.0F, 3.5F, 0.0F);
                    HIT_PAUSE(work) = 60;
                    local.x = work->pos.x;
                    local.y = work->pos.y + 15.0F;
                    local.z = work->pos.z;
                    bomb_create(&local, 1.0F);
                    sound_play(SE_JOINT, 0, 0, 0);
                }
            } else {
                work->collision->kind = damaged;
                if (--HIT_PAUSE(work) == 0) {
                    work->flags &= ~COLLISION_DAMAGED;
                }
            }
        }
        if (record->flags & FALL) {
            work->mode = SEGMENT_FALL;
        }
        break;
    case SEGMENT_FALL:
        record->moved.y = 2.0F;
        bomb_create(&work->pos, 1.5F);
        work->mode = SEGMENT_FALLING;
    case SEGMENT_FALLING:
        record->moved.y += -GRAVITY;
        work->pos.y += record->moved.y;
        if (work->pos.y < -200.0F) {
            task_kill(task);
        }
        break;
    }
    /* @hack redundant: a never-run store to last_moved, which keeps its y
     * in the frame at sp+52 (0x0c904c12, 0x0c904cb6), not its address in
     * R4; without it, 326 of 837 words differ */
    if (0) {
        last_moved.y = 1.0F;
    }
    /* @hack redundant: a never-run store of 1, which keeps 1 out of R12 for
     * the setup's stores; without it, the function starts 42 bytes late
     * and 795 of 837 words differ */
    if (0) {
        work->mode = 1;
    }
    if ((kind == EGG_VIPER_JOINT) && task->step != task_destroy &&
        !(boss->flags & VIPER_UNK_80) && !(boss->flags & VIPER_HELD_OFF) &&
        HIT_PAUSE(work) == 0) {
        collision_register(work);
        enemy_save_pos(work, target);
    }
    segment_draw(task);
}

static void segment_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    draw_attr_save();
    njSetTexture(&egm3mdl_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    switch (work->tick & EGG_VIPER_SEGMENT_KIND) {
    case EGG_VIPER_PLAIN:
        /* @hack redundant: a never-run test of draw_tree, which loads
         * egg_viper_objects into R11 and copies it to R10 (0x0c904eb6,
         * 0x0c904ec4), and with its else gives the frame_step test R3
         * (0x0c904ed2); without it, 6 of 103 words differ, and without its
         * else, 2 */
        if (0) {
            /* NOLINTNEXTLINE(clang-diagnostic-pointer-bool-conversion) */
            if (draw_tree) {
            } else {
            }
        }
        if (!frame_step) {
            draw_tree(egg_viper_objects[OBJECT_SEGMENT]);
        }
        break;
    case EGG_VIPER_JOINT:
        draw_control_save();
        njControl3D(0);
        if (!frame_step) {
            draw_tree(egg_viper_objects[OBJECT_JOINT]);
        }
        if (!frame_step) {
            njDrawObject(egg_viper_objects[OBJECT_JOINT_LIGHT]);
        }
        draw_control_restore();
        break;
    case EGG_VIPER_TAIL:
        if (!frame_step) {
            draw_tree(egg_viper_objects[OBJECT_TAIL]);
        }
        break;
    }
    njPopMatrix(1);
    draw_attr_restore();
}

static void segment_destroy(struct task *task)
{
}

/* a task that tells each segment to fall, from the tail, 20 steps apart,
 * and then ends */
static void segments_fall_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case 0:
        task->destructor = NULL;
        task->draw = NULL;
        work->tick = EGG_VIPER_SEGMENTS - 1;
        work->timer = 0;
        work->mode++;
    case 1:
        if ((Uint16)++work->timer >= 20) {
            work->timer = 0;
            RECORD(egg_viper_segments[work->tick])->flags |= FALL;
            if (work->tick == 0) {
                task_kill(task);
            } else {
                work->tick--;
            }
        }
        break;
    }
}

/* start a laser of kind from pos, which laser_step aims and fires */
static void laser_create(NJS_POINT3 *pos, int kind)
{
    struct task *task = task_create(TASK_WORK, 2, laser_step);

    task->extra = heap_calloc(1, sizeof(struct laser));
    task->work->pos.x = pos->x;
    task->work->pos.y = pos->y;
    task->work->pos.z = pos->z;
    task->work->tick = kind;
}

/* a laser's step. LASER_PASSING aims at player 0, but no more than 300 from
 * pos in z. LASER_JOINT and LASER_AIMED aim at him too. where a panel stands
 * 20 steps of his speed ahead of him, they lead him by 10 steps. a laser fires
 * only at a standing panel, and otherwise ends. its tip runs from the muzzle to
 * the target over 15 steps. the laser then stands at the target. its far end
 * runs in from the muzzle over 15 more steps. LASER_AIMED's lands in a blast.
 * the others flash at pos as they fire, and make an impact where they land */
static void laser_step(struct task *task)
{
    struct task_work *work = task->work;
    struct laser *laser = task->extra;
    NJS_VECTOR delta;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    void (*impact)(NJS_POINT3 *pos);

    /* @hack redundant: a never-run store of impact_create to a local, which
     * puts two spills at sp+20 and sp+24 (0x0c9053e4, 0x0c9053f4); without
     * it, they trade places and 2 of 777 words differ */
    if (0) {
        impact = impact_create;
    }
    switch (work->mode) {
    case 0:
        task->destructor = laser_destroy;
        task->draw = laser_draw;
        laser->tip.x = work->pos.x;
        laser->tip.y = work->pos.y;
        laser->tip.z = work->pos.z;
        if (work->tick == LASER_JOINT) {
            laser->target.x =
                player_work[0]->pos.x + (player_motion[0]->speed.x * 20.0F);
            laser->target.y = 15.0F;
            laser->target.z =
                player_work[0]->pos.z + (player_motion[0]->speed.z * 20.0F);
            if (floor_standing_at(laser->target.x, laser->target.z) != 0) {
                laser->target.x =
                    player_work[0]->pos.x + (player_motion[0]->speed.x * 10.0F);
                laser->target.y = 15.0F;
                laser->target.z =
                    player_work[0]->pos.z + (player_motion[0]->speed.z * 10.0F);
            } else {
                laser->target.x = player_work[0]->pos.x;
                laser->target.y = 15.0F;
                laser->target.z = player_work[0]->pos.z;
            }
        } else if (work->tick == LASER_AIMED) {
            laser->target.x =
                player_work[0]->pos.x + (player_motion[0]->speed.x * 20.0F);
            laser->target.y = 15.0F;
            laser->target.z =
                player_work[0]->pos.z + (player_motion[0]->speed.z * 20.0F);
            if (floor_standing_at(laser->target.x, laser->target.z) != 0) {
                laser->target.x =
                    player_work[0]->pos.x + (player_motion[0]->speed.x * 10.0F);
                laser->target.y = 15.0F;
                laser->target.z =
                    player_work[0]->pos.z + (player_motion[0]->speed.z * 10.0F);
            } else {
                laser->target.x = player_work[0]->pos.x;
                laser->target.y = 15.0F;
                laser->target.z = player_work[0]->pos.z;
            }
        } else {
            laser->target.x = player_work[0]->pos.x;
            laser->target.y = 15.0F;
            laser->target.z = player_work[0]->pos.z;
            if (work->pos.z - laser->target.z > 300.0F) {
                laser->target.z = work->pos.z - 300.0F;
            } else if (work->pos.z - laser->target.z < -300.0F) {
                laser->target.z = work->pos.z + 300.0F;
            }
        }
        if (floor_standing_at(laser->target.x, laser->target.z) == 0) {
            work->mode = LASER_ENDED;
            break;
        }
        laser->speed.x = (laser->target.x - work->pos.x) * (1.0F / 15.0F);
        laser->speed.y = (laser->target.y - work->pos.y) * (1.0F / 15.0F);
        laser->speed.z = (laser->target.z - work->pos.z) * (1.0F / 15.0F);
        delta.x = laser->target.x - work->pos.x;
        delta.y = laser->target.y - work->pos.y;
        delta.z = laser->target.z - work->pos.z;
        work->ang.y = (Angle)(ANGLE_PER_RADIAN * atan2f(delta.x, delta.z));
        work->ang.x = (Angle)(ANGLE_PER_RADIAN *
                              atan2f(-delta.y, njSqrt((delta.x * delta.x) +
                                                      (delta.z * delta.z))));
        laser->time = 0.0F;
        work->timer = 0;
        if (work->tick == LASER_JOINT || work->tick == LASER_PASSING) {
            flash_create(&work->pos, &work->ang);
        }
        sound_play(SE_LASER, 0, 0, 0);
        work->mode++;
    case 1:
        laser->tip.x += laser->speed.x;
        laser->tip.y += laser->speed.y;
        laser->tip.z += laser->speed.z;
        delta.x = laser->tip.x - work->pos.x;
        delta.y = laser->tip.y - work->pos.y;
        delta.z = laser->tip.z - work->pos.z;
        laser->length = njSqrt((delta.x * delta.x) + (delta.y * delta.y) +
                               (delta.z * delta.z)) *
                        LASER_SCALE;
        laser->time += 1.0F;
        if (!(laser->time < 15.0F)) {
            work->mode++;
        }
        break;
    case 2:
        laser->time = 0.0F;
        delta.x = work->pos.x - laser->target.x;
        delta.y = work->pos.y - laser->target.y;
        delta.z = work->pos.z - laser->target.z;
        work->ang.y = (Angle)(ANGLE_PER_RADIAN * atan2f(delta.x, delta.z));
        work->ang.x = (Angle)(ANGLE_PER_RADIAN *
                              atan2f(-delta.y, njSqrt((delta.x * delta.x) +
                                                      (delta.z * delta.z))));
        laser->tip.x = work->pos.x;
        laser->tip.y = work->pos.y;
        laser->tip.z = work->pos.z;
        work->pos.x = laser->target.x;
        work->pos.y = laser->target.y;
        work->pos.z = laser->target.z;
        if (work->tick == LASER_JOINT || work->tick == LASER_PASSING) {
            impact_create(&work->pos);
        } else {
            laser_blast.pos = &work->pos;
            egg_viper_blast_create(&laser_blast);
            vibration_start(0, 5, 59, 1);
            task_create(TASK_WORK, 2, blast_flash_step);
            sound_play(SE_LASER_BLAST, 0, 0, 0);
        }
        work->mode++;
    case 3:
        laser->tip.x += laser->speed.x;
        laser->tip.y += laser->speed.y;
        laser->tip.z += laser->speed.z;
        delta.x = laser->tip.x - work->pos.x;
        delta.y = laser->tip.y - work->pos.y;
        delta.z = laser->tip.z - work->pos.z;
        laser->length = njSqrt((delta.x * delta.x) + (delta.y * delta.y) +
                               (delta.z * delta.z)) *
                        LASER_SCALE;
        laser->time += 1.0F;
        if (!(laser->time < 15.0F)) {
            work->mode++;
        }
        break;
    default:
        task_kill(task);
        break;
    }
    laser_draw(task);
}

static void laser_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct laser *laser = task->extra;
    Angle ang;

    draw_attr_save();
    njSetTexture(egg_viper_effect_texlists[EFFECT_LASER]);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    njScale(NULL, 1.0F, 1.0F, laser->length);
    njTranslate(NULL, 0.0F, 0.0F, LASER_MODEL_Z);
    if (!frame_step) {
        draw_tree(egg_viper_effect_objects[EFFECT_LASER]);
    }
    njPopMatrix(1);
    draw_attr_restore();
}

static void laser_destroy(struct task *task)
{
}

/* a ring at pos, turned by ang, that grows to its size over 10 steps */
static void flash_create(NJS_POINT3 *pos, struct angles *ang)
{
    struct task *task = task_create(TASK_WORK, 2, flash_step);

    task->extra = heap_calloc(1, sizeof(struct egg_viper_record));
    task->work->pos.x = pos->x;
    task->work->pos.y = pos->y;
    task->work->pos.z = pos->z;
    task->work->ang.x = ang->x;
    task->work->ang.y = ang->y;
    task->work->ang.z = ang->z;
}

static void flash_step(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    /* @hack redundant: unused, which keeps the frame's 12 bytes (ADD #-12,R15
     * at 0x0c9057c4) that no instruction reads or writes; without them, the
     * step ends 4 bytes early and 39 of 41 words differ */
    NJS_POINT3 unused;

    switch (work->mode) {
    case 0:
        task->destructor = flash_destroy;
        task->draw = flash_draw;
        record->motion = egg_viper_effect_motions[0];
        record->frame = 0.0F;
        record->speed = 1.0F;
        record->scale = 0.0F;
        work->mode++;
        break;
    default:
        break;
    }
    record->scale += 0.1F;
    if (record->scale > 1.0F) {
        task_kill(task);
    }
    flash_draw(task);
}

static void flash_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    Float scale;
    Angle ang;

    draw_attr_save();
    njSetConstantAttr(0xffffffff, NJD_FLAG_USE_ALPHA);
    njSetTexture(egg_viper_effect_texlists[EFFECT_RING]);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    scale = record->scale;
    njScale(NULL, scale, scale, scale);
    if (!frame_step) {
        draw_tree(egg_viper_effect_objects[EFFECT_RING]);
    }
    njPopMatrix(1);
    draw_attr_restore();
}

static void flash_destroy(struct task *task)
{
}

/* a ring at pos, turned by ang, that shrinks from 20 times its size over
 * 19 steps */
static void charge_create(NJS_POINT3 *pos, struct angles *ang)
{
    struct task *task = task_create(TASK_WORK, 2, charge_step);

    task->extra = heap_calloc(1, sizeof(struct egg_viper_record));
    task->work->pos.x = pos->x;
    task->work->pos.y = pos->y;
    task->work->pos.z = pos->z;
    task->work->ang.x = ang->x;
    task->work->ang.y = ang->y;
    task->work->ang.z = ang->z;
}

static void charge_step(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    /* @hack redundant: unused, which keeps the frame's 12 bytes (ADD #-12,R15
     * at 0x0c9059a4) that no instruction reads or writes; without them, the
     * step ends 4 bytes early and 40 of 42 words differ */
    NJS_POINT3 unused;

    switch (work->mode) {
    case 0:
        task->destructor = charge_destroy;
        task->draw = charge_draw;
        record->motion = egg_viper_effect_motions[0];
        record->frame = 0.0F;
        record->speed = 1.0F;
        record->scale = 20.0F;
        work->mode++;
        break;
    default:
        break;
    }
    record->scale -= 1.0F;
    if (!(record->scale > 1.0F)) {
        task_kill(task);
    }
    charge_draw(task);
}

static void charge_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    Float scale;
    Angle ang;

    draw_attr_save();
    njSetConstantAttr(0xffffffff, NJD_FLAG_USE_ALPHA);
    njSetTexture(egg_viper_effect_texlists[EFFECT_RING]);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    scale = record->scale;
    njScale(NULL, scale, scale, scale);
    if (!frame_step) {
        draw_tree(egg_viper_effect_objects[EFFECT_RING]);
    }
    njPopMatrix(1);
    draw_attr_restore();
}

static void charge_destroy(struct task *task)
{
}

/* a laser's impact at pos: a sprite that plays once and hurts players, and
 * a scorch there */
static void impact_create(NJS_POINT3 *pos)
{
    struct task *task = task_create(TASK_WORK, 2, impact_step);

    task->extra = heap_calloc(1, sizeof(struct egg_viper_record));
    task->work->pos.x = pos->x;
    task->work->pos.y = pos->y;
    task->work->pos.z = pos->z;
}

static void impact_step(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    /* @hack redundant: unused, which keeps the frame's 12 bytes (ADD #-12,R15
     * at 0x0c905b4a) that no instruction reads or writes; without them, 29
     * of 143 words differ */
    NJS_POINT3 unused;

    switch (work->mode) {
    case 0:
        task->destructor = impact_destroy;
        task->draw = impact_draw;
        collision_init(task, &impact_shape, COLLISION_OBJECT, 1, 1);
        record->sprite = egg_viper_sprites[SPRITE_IMPACT];
        record->frame = 0.0F;
        record->speed = 1.0F;
        scorch_create(&work->pos);
        sound_play(SE_IMPACT, 0, 0, 0);
        work->mode++;
    case 1:
        impact_draw(task);
        if (sprite_play(record)) {
            record->frame = 0.0F;
            work->timer = 0;
            work->mode++;
        }
        break;
    default:
        task_kill(task);
        break;
    }
    if (task->step == task_destroy) {
        return;
    }
    collision_register(work);
}

static void impact_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;

    njSetTexture(record->sprite->sprite->tlist);
    draw_attr_save();
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    if (!frame_step) {
        sprite_draw(record->sprite, &work->pos, record->frame);
    }
    draw_attr_restore();
}

static void impact_destroy(struct task *task)
{
}

/* a scorch at pos, that fades over 20 steps */
static void scorch_create(NJS_POINT3 *pos)
{
    struct task *task = task_create(TASK_WORK, 2, scorch_step);

    task->extra = heap_calloc(1, sizeof(struct scorch));
    task->work->pos.x = pos->x;
    task->work->pos.y = pos->y;
    task->work->pos.z = pos->z;
}

static void scorch_step(struct task *task)
{
    struct task_work *work = task->work;
    struct scorch *scorch = task->extra;
    /* @hack redundant: unused, which keeps the frame's 12 bytes (ADD #-12,R15
     * at 0x0c905d44) that no instruction reads or writes; without them, the
     * step ends 4 bytes early and 43 of 45 words differ */
    NJS_POINT3 unused;

    switch (work->mode) {
    case 0:
        task->destructor = scorch_destroy;
        task->draw = scorch_draw;
        scorch->alpha = 1.0F;
        ++work->mode;
    case 1:
        scorch->alpha -= 0.05F;
        if (scorch->alpha <= 0.0F) {
            ++work->mode;
        }
        break;
    default:
        task_kill(task);
        break;
    }
    scorch_draw(task);
}

static void scorch_draw(struct task *task)
{
    if (!frame_step) {
        scorch_quad_draw(&task->work->pos,
                         ((struct scorch *)task->extra)->alpha);
    }
}

static void scorch_destroy(struct task *task)
{
}

/* the boss's ball at pos. it grows over 10 steps, then homes in on player 0.
 * its speed rises 0.1 a step, up to 10. from within 150 of him, it flies
 * straight on, 10% faster each step. it dies 500 from the arena's middle along
 * any axis */
static void ball_create(NJS_POINT3 *pos)
{
    struct task *task = task_create(TASK_WORK, 2, ball_step);

    task->extra = heap_calloc(1, sizeof(struct egg_viper_record));
    task->work->pos.x = pos->x;
    task->work->pos.y = pos->y;
    task->work->pos.z = pos->z;
}

static void ball_step(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    struct task_work *player = player_work[0];
    NJS_VECTOR delta;

    switch (work->mode) {
    case 0:
        task->destructor = ball_destroy;
        task->draw = ball_draw;
        collision_init(task, &ball_shape, COLLISION_OBJECT, 1, 1);
        record->sprite = egg_viper_sprites[SPRITE_BALL];
        record->frame = 0.0F;
        record->speed = 1.0F;
        record->scale = 0.0F;
        record->along = 0.0F;
        work->mode++;
    case 1:
        record->scale += 0.1F;
        if (record->scale >= 1.0F) {
            record->scale = 1.0F;
            sound_play(SE_BALL, 0, 0, 0);
            work->mode++;
        }
        break;
    case 2:
        if (record->along * record->along < 100.0F) {
            record->along += 0.1F;
        }
        viper_fly_to(work, record, &player->pos, 0x222, record->along);
        delta.x = work->pos.x - player->pos.x;
        delta.y = work->pos.y - player->pos.y;
        delta.z = work->pos.z - player->pos.z;
        if ((delta.x * delta.x) + (delta.y * delta.y) + (delta.z * delta.z) <=
            22500.0F) {
            work->mode++;
        }
        break;
    case 3:
        record->moved.x *= 1.1F;
        record->moved.y *= 1.1F;
        record->moved.z *= 1.1F;
        work->pos.x += record->moved.x;
        work->pos.y += record->moved.y;
        work->pos.z += record->moved.z;
        if (work->pos.x < -500.0F || work->pos.x > 500.0F ||
            work->pos.y < -500.0F || work->pos.y > 500.0F ||
            work->pos.z < -500.0F || work->pos.z > 500.0F) {
            work->mode++;
        }
        break;
    default:
        task_kill(task);
        break;
    }
    if (task->step != task_destroy) {
        collision_register(work);
    }
    ball_draw(task);
    sprite_loop(record);
}

/* the ball, and its halo the same size */
static void ball_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    struct egg_viper_sprite *halo;
    NJS_SPRITE *sprite;

    njSetTexture(record->sprite->sprite->tlist);
    draw_attr_save();
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    if (!frame_step) {
        sprite_draw_scaled(record->sprite, &work->pos, record->scale,
                           record->frame);
    }
    halo = egg_viper_sprites[SPRITE_HALO];
    sprite = halo->sprite;
    sprite->p.x = work->pos.x;
    sprite->p.y = work->pos.y;
    sprite->p.z = work->pos.z;
    sprite->sx = record->scale;
    sprite->sy = record->scale;
    if (!frame_step) {
        njDrawSprite3D(sprite, 0, halo->attr);
    }
    draw_attr_restore();
}

static void ball_destroy(struct task *task)
{
}

/* the energy the boss gathers, at pos and turned by ang. three flares and their
 * sparks grow over 357 steps, and then blink. 357 is 0.85 of the 420 steps the
 * gathering takes. palette 0 brightens to white over 420 steps. the task ends
 * once VIPER_GATHERING clears */
static void gather_create(NJS_POINT3 *pos, struct angles *ang)
{
    struct task *task = task_create(TASK_WORK, 2, gather_step);

    task->extra = heap_calloc(1, sizeof(struct egg_viper_record));
    task->work->pos.x = pos->x;
    task->work->pos.y = pos->y;
    task->work->pos.z = pos->z;
    task->work->ang.x = ang->x;
    task->work->ang.y = ang->y;
    task->work->ang.z = ang->z;
}

static void gather_step(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    /* @hack redundant: egg_viper_task's work, read at 0x0c9061fa and dropped;
     * without it, the step ends 6 bytes early and 108 of 116 words
     * differ */
    struct task_work *boss_work = egg_viper_task->work;
    struct egg_viper_record *boss = RECORD(egg_viper_task);

    switch (work->mode) {
    case 0:
        task->destructor = gather_destroy;
        task->draw = gather_draw;
        record->frame = 0.0F;
        record->speed = 1.0F;
        record->along = 0.0F;
        GATHER(work)->fade = 0.0F;
        GATHER(work)->grown = 0;
        work->mode++;
        break;
    }
    record->frame += record->speed;
    if (record->frame >= 8.0F) {
        record->frame -= 8.0F;
    }
    record->along += 1.0F / (420.0F * 0.85F);
    if (record->along > 1.0F) {
        record->along = 1.0F;
        GATHER(work)->grown++;
    }
    GATHER(work)->fade += 1.0F / 420.0F;
    palette_raise((int)(GATHER(work)->fade * 255.0F), 0xff0000, 16);
    palette_raise((int)(GATHER(work)->fade * 255.0F), 0xff00, 8);
    palette_raise((int)(GATHER(work)->fade * 255.0F), 0xff, 0);
    if (!(boss->flags & VIPER_GATHERING)) {
        task_kill(task);
    }
    gather_draw(task);
}

static void gather_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    NJS_POINT3 points[3];
    NJS_POINT3 spark_points[3];
    NJS_MATRIX matrix;
    Angle ang;
    NJS_POINT3 *sparks;
    NJS_POINT3 *point_0;
    NJS_POINT3 *point_1;
    NJS_POINT3 *point_2;
    NJS_POINT3 *spark_0;
    NJS_POINT3 *spark_1;
    NJS_POINT3 *spark_2;
    int *step;
    Float still;
    struct egg_viper_sprite **sprites;

    /* @hack redundant: the addresses of five of the point pointers, which give
     * those five the frame's slots from sp+4 to sp+20. they also give
     * frame_step's address R13 and the sprites' R12 (0x0c906426,
     * 0x0c906428); without them, 22 of 300 words differ */
    (void)&point_0;
    (void)&spark_2;
    (void)&spark_1;
    (void)&spark_0;
    (void)&point_2;
    draw_attr_save();
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    njGetMatrix(&matrix);
    njPopMatrix(1);
    njCalcPoint(&matrix, &gather_points[0], point_0 = &points[0]);
    njCalcPoint(&matrix, &gather_points[1], point_1 = &points[1]);
    njCalcPoint(&matrix, &gather_points[2], point_2 = &points[2]);
    /* @hack spelling: sparks set in the call, which loads the table first
     * (0x0c9063e2) and gives frame_step's address R13 and the sprites' R12
     * (0x0c906426, 0x0c906428); set before it, 20 of 300 words differ */
    njCalcPoint(&matrix, (sparks = gather_spark_points),
                spark_0 = &spark_points[0]);
    njCalcPoint(&matrix, &sparks[1], spark_1 = &spark_points[1]);
    njCalcPoint(&matrix, &sparks[2], spark_2 = &spark_points[2]);
    if (!(GATHER(work)->grown & 2)) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        sprites = egg_viper_sprites;
        step = &frame_step;
        /* @hack redundant: each draw the arm of a ?: on a different true
         * constant, which loads sprite_draw_scaled's address at each call
         * (0x0c90644a); as plain calls, SHC keeps it in R10 and 87 of 300
         * words differ */
        /* @hack redundant: a jump to the next line after each draw, which gives
         * the frame_step tests after them R2 (0x0c906440 on); without the
         * jumps, they take R3 and 12 of 300 words differ */
        /* NOLINTBEGIN(clang-diagnostic-literal-conversion) */
        if ((still = 0.0F, !*step)) {
            (2.0F ? (sprite_draw_scaled(sprites[SPRITE_FLARE], point_0,
                                        record->along, still),
                     0)
                  : 0);
            goto flare_0;
        flare_0:;
            if (!*step) {
                (3.0F ? (sprite_draw_scaled(sprites[SPRITE_FLARE], point_1,
                                            record->along, still),
                         0)
                      : 0);
                goto flare_1;
            flare_1:;
                if (!*step) {
                    (4.0F ? (sprite_draw_scaled(sprites[SPRITE_FLARE], point_2,
                                                record->along, still),
                             0)
                          : 0);
                }
                goto flare_2;
            flare_2:;
            }
        }
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        if (!*step) {
            (5.0F ? (sprite_draw_scaled(sprites[SPRITE_SPARK], spark_0,
                                        record->along, record->frame),
                     0)
                  : 0);
            goto spark_drawn_0;
        spark_drawn_0:;
            if (!*step) {
                (6.0F ? (sprite_draw_scaled(sprites[SPRITE_SPARK], spark_1,
                                            record->along, record->frame),
                         0)
                      : 0);
                goto spark_drawn_1;
            spark_drawn_1:;
                if (!*step) {
                    (7.0F ? (sprite_draw_scaled(sprites[SPRITE_SPARK], spark_2,
                                                record->along, record->frame),
                             0)
                          : 0);
                    goto spark_drawn_2;
                spark_drawn_2:;
                }
            }
        }
        /* NOLINTEND(clang-diagnostic-literal-conversion) */
    }
    draw_attr_restore();
}

static void gather_destroy(struct task *task)
{
    palette_restore(task);
}

/* make three pillars of fire about pos, each to rise after its delay */
static void pillars_create(NJS_POINT3 *pos)
{
    int i;
    struct task *task;

    for (i = 0; i < 3; i++) {
        task = task_create(TASK_WORK, 2, pillar_step);
        task->extra = heap_calloc(1, sizeof(struct egg_viper_record));
        task->work->pos.x = pos->x + pillar_offsets[i].x;
        task->work->pos.y = pos->y + pillar_offsets[i].y;
        task->work->pos.z = pos->z + pillar_offsets[i].z;
        PILLAR_WIDTH(task->work) = pillar_widths[i];
        PILLAR_DELAY(task->work) = pillar_delays[i];
        if (i == 0) {
            sound_play_held(SE_PILLARS, (int)task->work, 1, 0);
        }
    }
}

/* a pillar of fire. after PILLAR_DELAY steps, it rises in 10 sections, 100
 * apart. each section grows to full height over 10 steps, and timer counts
 * the growths finished. from its start, a bomb's puff bursts near it every 8
 * steps. each step adds 15 to the v of each uv of the model the pillars
 * share. it adds 15 to PILLAR_SCROLL too. once PILLAR_SCROLL passes 255, the
 * step takes 256 off it and off the uvs */
static void pillar_step(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    NJS_COLOR *uv;
    int wrap;
    int i;

    switch (work->mode) {
    case 0:
        task->destructor = pillar_destroy;
        task->draw = pillar_draw;
        work->tick = 0;
        work->timer = 0;
        PILLAR_SCROLL(work) = 0;
        record->along = 0.0F;
        work->mode++;
    case 1:
        if (PILLAR_DELAY(work)-- == 0) {
            work->mode++;
        }
        break;
    case 2:
        record->along += 0.1F;
        if (record->along >= 1.0F) {
            work->timer++;
            if ((Uint16)work->timer >= 10) {
                work->mode++;
            } else {
                record->along = 0.0F;
            }
        }
        break;
    case 3:
        break;
    }
    if (++work->tick >= 8) {
        pillar_bomb(&work->pos);
        work->tick = 0;
    }
    uv = egg_viper_effect_objects[EFFECT_PILLAR]->model->meshsets->vertuv;
    PILLAR_SCROLL(work) += 15;
    if (PILLAR_SCROLL(work) >= 256) {
        PILLAR_SCROLL(work) -= 256;
        wrap = -256;
    } else {
        wrap = 0;
    }
    for (i = 0; i < PILLAR_UVS; i++) {
        uv[i].tex.v += 15 + wrap;
    }
    pillar_draw(task);
}

/* draw timer - 1 full sections of the pillar, 100 apart. one more above them
 * takes record->along as its height's scale */
static void pillar_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    int i;
    Float height;

    draw_attr_save();
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetTexture(egg_viper_effect_texlists[EFFECT_PILLAR]);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    /* @hack spelling: height, a local for 1.0F, which puts the spill at
     * 0x0c90678c at sp+8; as the literal, the two spills trade places and 3
     * of 131 words differ */
    height = 1.0F;
    /* @bug logic: i < timer - 1 draws no full section for timer 0 or 1
     * alike (ADD #-1,R0 at 0x0c9067d4), and the growing one lies at 0 for
     * both. so section 0 grows, vanishes as timer reaches 1, and grows
     * again */
    /* @hack spelling: i = i + 1, which gives njTranslate's address R12 and
     * 0 R13 (0x0c9067ae, 0x0c9067c4); as i++, they trade and 9 of 131 words
     * differ */
    for (i = 0; i < (Uint16)work->timer - 1; i = i + 1) {
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, i * 100.0F, 0.0F);
        njScale(NULL, PILLAR_WIDTH(work), height, PILLAR_WIDTH(work));
        if (!frame_step) {
            njDrawObject(egg_viper_effect_objects[EFFECT_PILLAR]);
        }
        njPopMatrix(1);
    }
    /* @hack redundant: never-run calls, which settle the loop's registers
     * and the order of its pool; without them, 67 of 131 words differ */
    if (0) {
        njTranslate(NULL, PILLAR_WIDTH(work), 100.0F, 0.0F);
        njTranslate(NULL, PILLAR_WIDTH(work), 100.0F, 0.0F);
        njTranslate(NULL, 0.0F, 100.0F, 0.0F);
        njPopMatrix(1);
        njPopMatrix(1);
        njDrawObject(NULL);
        njDrawObject(NULL);
        if (frame_step) {
        }
        njPushMatrix(NULL);
    }
    njTranslate(NULL, 0.0F, i * 100.0F, 0.0F);
    njScale(NULL, PILLAR_WIDTH(work), record->along, PILLAR_WIDTH(work));
    if (!frame_step) {
        njDrawObject(egg_viper_effect_objects[EFFECT_PILLAR]);
    }
    njPopMatrix(1);
    draw_attr_restore();
}

static void pillar_destroy(struct task *task)
{
}

/* a bomb's puff within 150 of pos in x and z, and within 100 of y 0 */
static void pillar_bomb(NJS_POINT3 *pos)
{
    NJS_POINT3 at;

    at.x = pos->x + (rand() * (300.0F * RAND_SCALE)) - 150.0F;
    at.y = (rand() * (200.0F * RAND_SCALE)) - 100.0F;
    at.z = pos->z + (rand() * (300.0F * RAND_SCALE)) - 150.0F;
    bomb_create(&at, 3.0F);
}

/* draw the sprite in a task's record at its frame, as impact_draw does. the
 * image holds it at 0x0c9068e0. nothing in the module calls it, and as a static
 * SHC would leave it out */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
void sprite_task_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;

    njSetTexture(record->sprite->sprite->tlist);
    draw_attr_save();
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    if (!frame_step) {
        sprite_draw(record->sprite, &work->pos, record->frame);
    }
    draw_attr_restore();
}

/* nothing in the module calls it, and as a static SHC would leave it out */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
void fun_0c906930(void)
{
}

/* Eggman's task in the energy mode, which raises him from the boss's back */
static void eggman_energy_create(void)
{
    struct task *task = task_create(TASK_WORK, 2, eggman_energy_step);

    enemy_alloc_record(task, task->work);
    task->extra = heap_calloc(1, sizeof(struct egg_viper_record));
}

/* Eggman rises from the boss's middle to node NODE_EGGMAN_RISE over 30 steps,
 * and plays his motion through. he then sways between its frame 35 and its end
 * until the back closes. he sinks back as his motion rewinds. a hit on him
 * throws player 0 off and sets VIPER_HIT and VIPER_HELD_OFF. he then lets no
 * hit register for 60 steps */
static void eggman_energy_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *boss_work = egg_viper_task->work;
    struct egg_viper_record *record = task->extra;
    struct egg_viper_record *boss = RECORD(egg_viper_task);
    /* @hack spelling: lift and push, locals for the 2.0 and 4.0 the image
     * keeps in FR13 and FR14 for the throws (0x0c9069fa, 0x0c9069fe); as
     * literals, 2 of 628 words differ */
    Float lift = 2.0F;
    Float push = 4.0F;
    NJS_POINT3 node;

    switch (work->mode) {
    case 0: {
        /* @hack spelling: HIT_PAUSE's 0 cast from none, the null motion, which
         * makes one 0 in R4 (0x0c906b04) for HIT_PAUSE, the shape and
         * sound_play's arguments; with each 0 stored apart, 21 of 628 words
         * differ */
        NJS_MOTION *none;

        task->destructor = eggman_energy_destroy;
        task->draw = eggman_energy_draw;
        collision_init(task, eggman_shapes, COLLISION_ITEM, 1, 1);
        work->pos.x = boss_work->pos.x;
        work->pos.y = boss_work->pos.y;
        work->pos.z = boss_work->pos.z;
        node_pos(egg_viper_matrices, NODE_EGGMAN_RISE, &node);
        record->moved.x = (node.x - work->pos.x) * (1.0F / 30.0F);
        record->moved.y = (node.y - work->pos.y) * (1.0F / 30.0F);
        record->moved.z = (node.z - work->pos.z) * (1.0F / 30.0F);
        push_way = record->moved.x > 0.0F ? 1.0F : -1.0F;
        record->target.x = 0.0F;
        record->target.y = 0.0F;
        record->target.z = 0.0F;
        none = 0;
        HIT_PAUSE(work) = (int)none;
        record->motion = egg_viper_eggman_motions[0];
        record->shape = none;
        record->frame = 0.0F;
        record->speed = 1.0F;
        sound_play(SE_EGGMAN, 0, 0, 0);
        work->mode = 1;
        break;
    }
    case 1:
        record->target.x += record->moved.x;
        record->target.y += record->moved.y;
        record->target.z += record->moved.z;
        work->pos.x = boss_work->pos.x + record->target.x;
        work->pos.y = boss_work->pos.y + record->target.y;
        work->pos.z = boss_work->pos.z + record->target.z;
        if (HIT_PAUSE(work) == 0) {
            if (work->flags & COLLISION_DAMAGED) {
                player_set_speed(0, push_way * push, lift, 0.0F);
                HIT_PAUSE(work) = 60;
                RECORD(egg_viper_task)->flags |= VIPER_HIT;
                RECORD(egg_viper_task)->flags |= VIPER_HELD_OFF;
            }
        } else {
            work->flags &= ~COLLISION_DAMAGED;
            --HIT_PAUSE(work);
        }
        if ((Uint16)++work->timer >= 30) {
            work->mode = 2;
        }
        break;
    case 2:
        node_pos(egg_viper_matrices, NODE_EGGMAN_RISE, &work->pos);
        if (HIT_PAUSE(work) == 0) {
            if (work->flags & COLLISION_DAMAGED) {
                player_set_speed(0, push_way * push, lift, 0.0F);
                HIT_PAUSE(work) = 60;
                RECORD(egg_viper_task)->flags |= VIPER_HIT;
                RECORD(egg_viper_task)->flags |= VIPER_HELD_OFF;
            }
        } else {
            work->flags &= ~COLLISION_DAMAGED;
            --HIT_PAUSE(work);
        }
        if (motion_play(record)) {
            record->speed = -record->speed;
            work->mode = 3;
        }
        break;
    case 3:
        node_pos(egg_viper_matrices, NODE_EGGMAN_RISE, &work->pos);
        if (HIT_PAUSE(work) == 0) {
            if (work->flags & COLLISION_DAMAGED) {
                if (boss->flags & VIPER_GENTLE_THROW) {
                    player_set_speed(0, push_way * push, lift, 0.0F);
                } else {
                    player_set_speed(0, -(push_way * 7.5F), 8.0F, 0.0F);
                }
                HIT_PAUSE(work) = 60;
                RECORD(egg_viper_task)->flags |= VIPER_HIT;
                RECORD(egg_viper_task)->flags |= VIPER_HELD_OFF;
            }
        } else {
            work->flags &= ~COLLISION_DAMAGED;
            --HIT_PAUSE(work);
        }
        if (record->speed > 0.0F) {
            if (motion_play(record)) {
                record->speed = -record->speed;
            }
        } else {
            motion_play(record);
            if (record->frame <= 35.0F) {
                record->speed = -record->speed;
            }
        }
        if (!(boss->flags & VIPER_BACK_OPEN)) {
            record->speed =
                record->speed > 0.0F ? record->speed : -record->speed;
            work->mode = 4;
        }
        break;
    case 4:
        record->target.x -= record->moved.x;
        record->target.y -= record->moved.y;
        record->target.z -= record->moved.z;
        work->pos.x = boss_work->pos.x + record->target.x;
        work->pos.y = boss_work->pos.y + record->target.y;
        work->pos.z = boss_work->pos.z + record->target.z;
        if (motion_rewind(record)) {
            task_kill(task);
        }
        break;
    }
    /* @hack redundant: never-run stores of 0.0 and push, which keep
     * push_way's address in R9 and the hit test's mask in R8 (0x0c9069ee,
     * 0x0c9069ec); without them, 8 of 628 words differ */
    if (0) {
        record->frame = 0.0F;
        push_way = push;
    }
    work->ang.x = boss_work->ang.x;
    work->ang.y = boss_work->ang.y;
    work->ang.z = boss_work->ang.z;
    if (task->step != task_destroy && HIT_PAUSE(work) == 0) {
        collision_register(work);
        enemy_save_pos(work, LOCKON_TARGET(task));
    }
    eggman_energy_draw(task);
}

/* draw Eggman in his motion, turned half round, while viper_shown is
 * TRUE */
static void eggman_energy_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    Angle ang;

    if (viper_shown) {
        draw_attr_save();
        njSetTexture(&egm3mdl_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        njRotateY(NULL, 0x8000);
        if (frame_step == 0) {
            draw_motion(egg_viper_eggman_objects[0], record->motion,
                        record->frame);
        }
        njPopMatrix(1);
        draw_attr_restore();
    }
}

static void eggman_energy_destroy(struct task *task)
{
}

/* Eggman's task in VIPER_BOOM, which raises him in the boss's cockpit */
static void eggman_boom_create(void)
{
    struct task *task = task_create(TASK_WORK, 2, eggman_boom_step);

    enemy_alloc_record(task, task->work);
    task->extra = heap_calloc(1, sizeof(struct egg_viper_record));
}

/* Eggman rides node EGG_VIPER_EGGMAN_NODE as he rises through his motion.
 * he sways between its frame 35 and its end until the front closes. once
 * hit_wait has run out, he sinks. a hit on him throws player 0 up and sets
 * VIPER_HIT. he then lets no hit register for 60 steps */
static void eggman_boom_step(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    struct task_work *boss_work = egg_viper_task->work;
    struct egg_viper_record *boss = RECORD(egg_viper_task);

    switch (work->mode) {
    case 0: {
        /* @hack spelling: HIT_PAUSE's 0 cast from none, the null motion, which
         * makes one 0 in R4 (0x0c90707e) for HIT_PAUSE, the shape and
         * sound_play's arguments; with each 0 stored apart, 64 of 283 words
         * differ */
        NJS_MOTION *none;

        task->destructor = eggman_boom_destroy;
        task->draw = eggman_boom_draw;
        collision_init(task, eggman_shapes, COLLISION_ITEM, 1, 1);
        none = 0;
        HIT_PAUSE(work) = (int)none;
        record->motion = egg_viper_eggman_motions[0];
        record->shape = none;
        record->frame = 0.0F;
        record->speed = 1.0F;
        sound_play(SE_EGGMAN, 0, 0, 0);
        work->mode = 1;
    }
    /* @hack spelling: case 0's break after its scope, which gives case 1's
     * call to motion_play R1 (0x0c9070a6); inside it, 2 of 283 words
     * differ */
    break;
    case 1:
        if (motion_play(record)) {
            record->speed = -record->speed;
            work->mode = 2;
        }
        break;
    case 2:
        if (HIT_PAUSE(work) == 0) {
            if (work->flags & COLLISION_DAMAGED) {
                player_set_speed(0, 0.0F, 3.0F, 0.0F);
                HIT_PAUSE(work) = 60;
                RECORD(egg_viper_task)->flags |= VIPER_HIT;
            }
        } else if (--HIT_PAUSE(work) == 0) {
            work->flags &= ~COLLISION_DAMAGED;
        }
        if (record->speed > 0.0F) {
            if (motion_play(record)) {
                record->speed = -record->speed;
            }
        } else {
            motion_play(record);
            if (record->frame <= 35.0F) {
                record->speed = -record->speed;
            }
        }
        if (!(boss->flags & VIPER_FRONT_OPEN)) {
            work->mode = 3;
        }
        break;
    case 3:
        if (hit_wait <= 0) {
            record->speed =
                record->speed > 0.0F ? record->speed : -record->speed;
            work->mode = 4;
        }
        break;
    case 4:
        if (motion_rewind(record)) {
            task_kill(task);
        }
        break;
    }
    {
        NJS_MATRIX *matrix =
            &egg_viper_node_matrices[EGG_VIPER_EGGMAN_NODE -
                                     EGG_VIPER_NODE_MATRICES_FIRST];

        work->pos.x = (*matrix)[12];
        work->pos.y = (*matrix)[13];
        work->pos.z = (*matrix)[14];
    }
    work->ang.x = boss_work->ang.x;
    work->ang.y = boss_work->ang.y;
    work->ang.z = boss_work->ang.z;
    if (task->step != task_destroy && HIT_PAUSE(work) == 0) {
        collision_register(work);
        enemy_save_pos(work, LOCKON_TARGET(task));
    }
    eggman_boom_draw(task);
}

static void eggman_boom_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = task->extra;
    Angle ang;

    if (viper_shown == FALSE) {
        return;
    }
    draw_attr_save();
    njSetTexture(&egm3mdl_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    if (!frame_step) {
        draw_motion(egg_viper_eggman_objects[0], record->motion, record->frame);
    }
    njPopMatrix(1);
    draw_attr_restore();
}

static void eggman_boom_destroy(struct task *task)
{
}

/* keep the pads disabled for count steps, from a task that enables them
 * as it ends */
static void pads_hold(int count)
{
    struct task *task = task_create(TASK_WORK, 1, pads_hold_step);

    task->work->vars.l[0] = count;
}

static void pads_hold_step(struct task *task)
{
    struct task_work *work = task->work;

    /* @hack spelling: a switch of one case, which tests the mode with CMP/EQ #0
     * (0x0c90735a); as an if, 11 of 33 words differ */
    switch (work->mode) {
    case 0:
        task->destructor = pads_hold_destroy;
        task->draw = NULL;
        work->mode++;
    }
    pad_disable();
    if (--work->vars.l[0] == 0) {
        task_kill(task);
    }
}

static void pads_hold_destroy(struct task *task)
{
    pad_enable();
}

void egg_viper_tree_matrices(NJS_OBJECT *object, NJS_MATRIX *matrices)
{
    NJS_OBJECT **stack = tree_stack;
    int i = 0;
    NJS_OBJECT **sp = stack;
    /* @hack spelling: the walk on node, a copy of object, which loads
     * njPushMatrix's address into R8 before njPopMatrix's into R13
     * (0x0c9073aa, 0x0c9073ac); walking object, they load the other way
     * round and 3 of 164 words differ */
    NJS_OBJECT *node = object;
    Angle ang;

    while (node != NULL) {
        njPushMatrix(NULL);
        njTranslate(NULL, node->pos[0], node->pos[1], node->pos[2]);
        TURN_VIA(njRotateZ, node->ang[2], ang);
        TURN_VIA(njRotateY, node->ang[1], ang);
        TURN_VIA(njRotateX, node->ang[0], ang);
        njScale(NULL, node->scl[0], node->scl[1], node->scl[2]);
        njGetMatrix(&matrices[i++]);
        if (node->child) {
            *sp++ = node->sibling;
            node = node->child;
        } else if (node->sibling) {
            node = node->sibling;
            njPopMatrix(1);
        } else if (sp == stack) {
            njPopMatrix(1);
            break;
        } else {
            /* @hack spelling: the test against tree_stack by name, beside the
             * test against stack above, which keeps njPushMatrix's address in
             * R8 (0x0c9073aa) and stack in R10 (0x0c9073b0); against stack,
             * njGetMatrix's takes R8, the walk ends 2 bytes early and 2,714
             * of the object's 16,560 words differ */
            do {
                node = *--sp;
                njPopMatrix(1);
            } while (node == NULL && sp != tree_stack);
            njPopMatrix(1);
        }
    }
}

void egg_viper_arena_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *boss = egg_viper_task->work;
    NJS_POINT3 pos;
    NJS_COLOR *src;
    NJS_COLOR *dst;
    int value;
    Angle ang_x;
    Angle ang_y;
    int i;

    switch (work->mode) {
    case 0:
        task->destructor = arena_destroy;
        task->draw = arena_draw;
        src = arena_smoke_uvs;
        dst = arena_smoke_saved;
        for (i = 0; i < ARENA_SMOKE_UVS; i++) {
            /* @hack type: color read through a cast to its own type, which
             * copies it through R2; as (src++)->color, 6 of 523 words
             * differ */
            (dst++)->color = *(const Uint32 *)(src++);
        }
        arena_smoke_scroll[0].tex.u = 0;
        arena_smoke_scroll[0].tex.v = 0;
        work->mode++;
        break;
    case 1:
        work->submode++;
    case 2:
        if (player_work[0]->pos.y < -200.0F) {
            player_down(0);
            work->mode++;
        }
        break;
    }
    if (hit_wait > 0) {
        hit_wait--;
    } else {
        viper_shown = TRUE;
    }
    value = joint_light_material[0].diffuse.argb.g;
    value += joint_light_step;
    if (value >= 255) {
        value = 255;
        joint_light_step = -joint_light_step;
    } else if (value <= 0) {
        value = 0;
        joint_light_step = -joint_light_step;
    }
    joint_light_material[0].diffuse.argb.g = value;
    if (health <= 0 && !(RECORD(egg_viper_task)->flags & VIPER_GONE) &&
        ++smoke_wait >= 6) {
        smoke_wait = 0;
        ang_x = rand() * (65535.0F * RAND_SCALE);
        ang_y = rand() * (65535.0F * RAND_SCALE);
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        TURN(njRotateY, ang_y);
        TURN(njRotateX, ang_x);
        njCalcPoint(NULL, &smoke_offset, &pos);
        njPopMatrix(1);
        pos.x += boss->pos.x;
        pos.y += boss->pos.y;
        pos.z += boss->pos.z;
        bomb_create(&pos, 1.0F);
        sound_play(SE_BURST, 0, 0, 0);
    }
    if (hit_burst != FALSE) {
        hit_burst = FALSE;
        for (i = 0; i < 15; i++) {
            ang_x = rand() * (65535.0F * RAND_SCALE);
            ang_y = rand() * (65535.0F * RAND_SCALE);
            njPushMatrix(NULL);
            njUnitMatrix(NULL);
            TURN(njRotateY, ang_y);
            TURN(njRotateX, ang_x);
            njCalcPoint(NULL, &smoke_offset, &pos);
            njPopMatrix(1);
            pos.x += boss->pos.x;
            pos.y += boss->pos.y;
            pos.z += boss->pos.z;
            bomb_create(&pos, 1.0F);
        }
        sound_play(SE_BURST, 0, 0, 0);
    }
    for (i = 0; i < 2; i++) {
        arena_smoke_scroll[i].tex.u += arena_smoke_steps[i].u;
        /* @bug logic: the else that should wrap u up by 256 tests v
         * (MOV.W @(2,R1),R0 at 0x0c907856), and adds to u (0x0c907862).
         * every step in arena_smoke_steps is 0 or more, so v never falls to
         * -256, and the else never runs */
        if (arena_smoke_scroll[i].tex.u >= 256) {
            arena_smoke_scroll[i].tex.u -= 256;
        } else if (arena_smoke_scroll[i].tex.v <= -256) {
            arena_smoke_scroll[i].tex.u += 256;
        }
        arena_smoke_scroll[i].tex.v += arena_smoke_steps[i].v;
        if (arena_smoke_scroll[i].tex.v >= 256) {
            arena_smoke_scroll[i].tex.v -= 256;
        } else if (arena_smoke_scroll[i].tex.v <= -256) {
            arena_smoke_scroll[i].tex.v += 256;
        }
    }
    arena_draw(task);
}

/* draw the smoke's model twice with the fog off: at y -150, then 10 lower. each
 * layer's uvs are the saved ones plus its scroll. each sum adds 32-bit words,
 * so a carry out of u runs into v */
static void arena_draw(struct task *task)
{
    NJS_COLOR *dst;
    NJS_COLOR *src;
    int i;
    /* @hack redundant: register on k, which gives k R4 and dst R5 in the
     * copy (0x0c907948, 0x0c90794c); without it, they trade and 10 of 101
     * words differ */
    register int k;

    draw_attr_save();
    njFogDisable();
    njSetTexture(&egm3mdl_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, -150.0F, 0.0F);
    for (i = 0; i < 2; i++) {
        src = arena_smoke_saved;
        dst = arena_smoke_uvs;
        for (k = 0; k < ARENA_SMOKE_UVS; k++) {
            dst->color = (src++)->color + arena_smoke_scroll[i].color;
            dst++;
        }
        njTranslate(NULL, 0.0F, -(i * 10.0F), 0.0F);
        if (!frame_step) {
            njDrawObject(&arena_smoke_object);
        }
    }
    njPopMatrix(1);
    njFogEnable();
    draw_attr_restore();
}

static void arena_destroy(struct task *task)
{
    NJS_COLOR *src = arena_smoke_saved;
    NJS_COLOR *dst = arena_smoke_uvs;
    int k;

    for (k = 0; k < ARENA_SMOKE_UVS; k++) {
        /* @hack type: color read through a cast to its own type, which
         * copies it through R2; as (src++)->color, 6 of 65 words differ */
        (dst++)->color = *(const Uint32 *)(src++);
    }
}

/* save palette 0, for the flashes to change and palette_restore to put
 * back */
static void palette_save(void)
{
    Uint32 *src = draw_palettes[0];
    Uint32 *dst = palette_saved;
    int i;

    for (i = 0; i < DRAW_PALETTE_ENTRIES * 2; i++) {
        *dst++ = *src++;
    }
}

/* raise palette 0's base colors in the channel that mask selects and shift
 * places. a color whose channel is below level takes level there. any
 * other goes back to its saved word */
static void palette_raise(int level, int mask, int shift)
{
    Sint32 *pal = (Sint32 *)draw_palettes[0];
    Sint32 *saved = (Sint32 *)palette_saved;
    int i;

    for (i = 0; i < DRAW_PALETTE_ENTRIES * 2; i += 2, pal += 2, saved += 2) {
        if (((*pal & mask) >> shift) < level) {
            *pal &= ~mask;
            *pal |= level << shift;
        } else {
            *pal = *saved;
        }
    }
}

/* lower palette 0's base colors in the channel that mask selects and shift
 * places. a color whose channel exceeds its whole saved word takes level
 * there. any other goes back to its saved word */
static void palette_lower(int level, int mask, int shift)
{
    Sint32 *pal = (Sint32 *)draw_palettes[0];
    Sint32 *saved = (Sint32 *)palette_saved;
    int i;

    for (i = 0; i < DRAW_PALETTE_ENTRIES * 2; i += 2, pal += 2, saved += 2) {
        /* @bug logic: the test compares the channel with its color's
         * whole saved word, signed (CMP/GT R1,R3 at 0x0c907b58), not with one
         * of its channels. a saved word whose alpha has its top bit set is
         * negative, so its color always takes level. a positive saved word
         * above 255 keeps its color as saved */
        if (((*pal & mask) >> shift) > *saved) {
            *pal &= ~mask;
            *pal |= level << shift;
        } else {
            *pal = *saved;
        }
    }
}

/* a hit's flash. palette 0's red falls from 255 by 8 a step to 0, or until the
 * boss breaks. it ends at once if another flash runs */
static void hit_flash_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case 0:
        task->destructor = palette_restore;
        task->draw = NULL;
        if (palette_busy) {
            task->destructor = NULL;
            task_kill(task);
            break;
        }
        FLASH(work)->level = FLASH_START;
        palette_busy = TRUE;
        work->mode++;
        break;
    case 1:
        FLASH(work)->level -= 8;
        if (FLASH(work)->level <= 0) {
            FLASH(work)->level = 0;
        }
        palette_lower(FLASH(work)->level, 0xff0000, 16);
        if (FLASH(work)->level == 0) {
            work->mode++;
        } else if (RECORD(egg_viper_task)->flags & VIPER_BREAKING) {
            work->mode++;
        }
        break;
    default:
        task_kill(task);
        break;
    }
}

/* put palette 0 back as saved, and free it for another flash */
static void palette_restore(struct task *task)
{
    Uint32 *src = palette_saved;
    Uint32 *dst = draw_palettes[0];
    int i;

    for (i = 0; i < DRAW_PALETTE_ENTRIES * 2; i++) {
        *dst++ = *src++;
    }
    palette_busy = FALSE;
}

/* the defeat's flash. once no other flash runs, palette 0's red falls from 255
 * to 191 by 8 a step. it repeats until the boss is gone. it then rises from 196
 * to 255 by 4 a step, and stays */
static void defeat_flash_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case 0:
        task->destructor = palette_restore;
        task->draw = NULL;
        FLASH(work)->level = FLASH_START;
        work->mode++;
        break;
    case 1:
        if (!palette_busy) {
            palette_busy = TRUE;
            work->mode++;
        }
        break;
    case 2:
        FLASH(work)->level -= 8;
        if (FLASH(work)->level <= 0) {
            FLASH(work)->level = 0;
        }
        palette_lower(FLASH(work)->level, 0xff0000, 16);
        if (FLASH(work)->level <= 0xc0) {
            FLASH(work)->level = FLASH_START;
        }
        if (RECORD(egg_viper_task)->flags & VIPER_GONE) {
            FLASH(work)->level = 0xc0;
            work->mode++;
        }
        break;
    case 3:
        FLASH(work)->level += 4;
        if (FLASH(work)->level >= 0x100) {
            FLASH(work)->level = 0xff;
        }
        palette_raise(FLASH(work)->level, 0xff0000, 16);
        if (FLASH(work)->level == 0xff) {
            work->mode++;
        }
        break;
    }
}

/* a blast's flash. palette 0's red rises from 4 to 255 by 4 a step, then falls
 * back to 0. it ends at once if another flash runs */
static void blast_flash_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case 0:
        task->destructor = palette_restore;
        task->draw = NULL;
        if (palette_busy) {
            task->destructor = NULL;
            task_kill(task);
            break;
        }
        FLASH(work)->level = 0;
        work->timer = 0;
        palette_busy = TRUE;
        work->mode++;
        break;
    case 1:
        FLASH(work)->level += 4;
        if (FLASH(work)->level >= 0x100) {
            FLASH(work)->level = 0xff;
        }
        palette_raise(FLASH(work)->level, 0xff0000, 16);
        if (FLASH(work)->level == 0xff) {
            work->mode++;
        }
        break;
    case 2:
        work->mode++;
        break;
    case 3:
        FLASH(work)->level -= 4;
        if (FLASH(work)->level <= 0) {
            FLASH(work)->level = 0;
        }
        palette_lower(FLASH(work)->level, 0xff0000, 16);
        if (FLASH(work)->level == 0) {
            work->mode++;
        }
        break;
    default:
        task_kill(task);
        break;
    }
}

/* fill dd for the natural cubic spline through values at knots. each point's
 * entry is a sixth of its second derivative. the ends' are 0, and the rest
 * solve the spline's tridiagonal system */
static void spline_solve(const Float *knots, const Float *values, Float *dd,
                         Sint32 count)
{
    Sint32 i;
    Float ratio;

    dd[0] = 0.0F;
    dd[count - 1] = 0.0F;
    for (i = 0; i < count - 1; i++) {
        spline_h[i] = knots[i + 1] - knots[i];
        spline_diag[i + 1] = (values[i + 1] - values[i]) / spline_h[i];
    }
    dd[1] = spline_diag[2] - spline_diag[1] - (spline_h[0] * dd[0]);
    spline_diag[1] = 2.0F * (knots[2] - knots[0]);
    for (i = 1; i < count - 2; i++) {
        ratio = spline_h[i] / spline_diag[i];
        dd[i + 1] = spline_diag[i + 2] - spline_diag[i + 1] - (dd[i] * ratio);
        spline_diag[i + 1] =
            (2.0F * (knots[i + 2] - knots[i])) - (spline_h[i] * ratio);
    }
    dd[count - 2] -= spline_h[count - 2] * dd[count - 1];
    for (i = count - 2; i > 0; i--) {
        dd[i] = (dd[i] - (spline_h[i] * dd[i + 1])) / spline_diag[i];
    }
}

void egg_viper_spline_init(struct egg_viper_spline *spline)
{
    Float dx;
    Float dy;
    Float dz;
    int count = spline->count;
    int i;

    spline->knots[0] = 0.0F;
    for (i = 1; i < count; i++) {
        dx = spline->x[i] - spline->x[i - 1];
        dy = spline->y[i] - spline->y[i - 1];
        dz = spline->z[i] - spline->z[i - 1];
        spline->knots[i] =
            spline->knots[i - 1] + njSqrt((dx * dx) + (dy * dy) + (dz * dz));
    }
    for (i = 1; i < count; i++) {
        spline->knots[i] /= spline->knots[count - 1];
    }
    spline_solve(spline->knots, spline->x, spline->ddx, count);
    spline_solve(spline->knots, spline->y, spline->ddy, count);
    spline_solve(spline->knots, spline->z, spline->ddz, count);
}

/* the spline's value at along, in the interval between knots that a
 * binary search finds */
static Float spline_value(Float along, const Float *knots, const Float *values,
                          const Float *dd, int count)
{
    int low = 0;
    int high = count - 1;
    int middle;
    Float width;

    while (low < high) {
        middle = (low + high) / 2;
        if (knots[middle] < along) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    if (low > 0) {
        low--;
    }
    width = knots[low + 1] - knots[low];
    along -= knots[low];
    return ((((((dd[low + 1] - dd[low]) * along / width) + (3.0F * dd[low])) *
              along) -
             (((2.0F * dd[low]) + dd[low + 1]) * width) +
             ((values[low + 1] - values[low]) / width)) *
            along) +
           values[low];
}

void egg_viper_spline_point(struct egg_viper_spline *spline, Float along,
                            NJS_POINT3 *pos)
{
    pos->x = spline_value(along, spline->knots, spline->x, spline->ddx,
                          spline->count);
    pos->y = spline_value(along, spline->knots, spline->y, spline->ddy,
                          spline->count);
    pos->z = spline_value(along, spline->knots, spline->z, spline->ddz,
                          spline->count);
}

Angle egg_viper_angle_approach(Angle from, Angle to, Angle step)
{
    int diff;
    int dist;
    int arc;

    from = (Uint16)from;
    to = (Uint16)to;
    diff = from - to;
    dist = diff >= 0 ? diff : -diff;
    arc = dist < 0x8000 ? dist : 0x10000 - dist;
    if (arc <= step) {
        return to;
    }
    if ((diff >= 0) ^ (dist >= 0x8000)) {
        return (Uint16)(from - step);
    }
    return (Uint16)(from + step);
}

/* turn work toward target by turn at most, about x and y. then move it speed
 * along its new heading. record->moved takes the move. return 1 once work is
 * nearer target than speed, else 0 */
static int viper_fly_to(struct task_work *work, struct egg_viper_record *record,
                        NJS_POINT3 *target, Angle turn, Float speed)
{
    Float dx;
    Float dy;
    Float dz;
    NJS_VECTOR way;
    struct angles ang;
    Angle angle;

    dx = target->x - work->pos.x;
    dy = target->y - work->pos.y;
    dz = target->z - work->pos.z;
    ang.y = (Angle)(atan2f(dx, dz) * ANGLE_PER_RADIAN);
    ang.x =
        (Angle)(atan2f(-dy, njSqrt((dx * dx) + (dz * dz))) * ANGLE_PER_RADIAN);
    work->ang.x = egg_viper_angle_approach(work->ang.x, ang.x, turn);
    work->ang.y = egg_viper_angle_approach(work->ang.y, ang.y, turn);
    way.x = 0.0F;
    way.y = 0.0F;
    way.z = 1.0F;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    TURN_VIA(njRotateY, work->ang.y, angle);
    TURN_VIA(njRotateX, work->ang.x, angle);
    njCalcPoint(NULL, &way, &record->moved);
    njPopMatrix(1);
    record->moved.x *= speed;
    record->moved.y *= speed;
    record->moved.z *= speed;
    work->pos.x += record->moved.x;
    work->pos.y += record->moved.y;
    work->pos.z += record->moved.z;
    way.x = target->x - work->pos.x;
    way.y = target->y - work->pos.y;
    way.z = target->z - work->pos.z;
    return (way.x * way.x) + (way.y * way.y) + (way.z * way.z) < speed * speed;
}

/* move pos speed straight toward target. from within speed, it lands on
 * target and returns 1, and otherwise it returns 0 */
static int move_to(NJS_POINT3 *pos, NJS_POINT3 *target, Float speed)
{
    NJS_VECTOR delta;

    delta.x = target->x - pos->x;
    delta.y = target->y - pos->y;
    delta.z = target->z - pos->z;
    if ((delta.x * delta.x) + (delta.y * delta.y) + (delta.z * delta.z) <=
        speed * speed) {
        pos->x = target->x;
        pos->y = target->y;
        pos->z = target->z;
        return 1;
    } else {
        njUnitVector(&delta);
        pos->x += delta.x * speed;
        pos->y += delta.y * speed;
        pos->z += delta.z * speed;
        return 0;
    }
}

/* set work's mode to the next one the boss's script lists, and step past it. at
 * the script's -1, it starts the script again, or the next script if
 * script_next is set */
static void viper_next_mode(struct task_work *work,
                            struct egg_viper_record *record)
{
    /* a script's modes are signed: -1 ends it */
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int mode = scripts[record->script][record->step];

    /* @bug logic: restarts a script at its first mode but leaves step at 0, not
     * 1, so the next call sets the first mode again (MOV.L R6,@(R0,R5) at
     * 0x0c908510, where only the other path adds 1, at 0x0c90854a).
     * egg_viper_step keeps the submode of a mode that doesn't change, so the
     * first mode's last submode runs a second time */
    if (mode < 0) {
        record->step = 0;
        if (script_next) {
            script_next = FALSE;
            ++record->script;
            if (!scripts[record->script]) {
                record->script = 0;
            }
        }
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        mode = scripts[record->script][record->step];
    } else {
        ++record->step;
    }
    work->mode = mode;
}

/* move the record's frame on by its speed, to the motion's last frame at
 * most. return 1 there, else 0 */
static int motion_play(struct egg_viper_record *record)
{
    record->frame += record->speed;
    if (record->frame >= record->motion->nbFrame - 1) {
        record->frame = record->motion->nbFrame - 1;
        return 1;
    }
    return 0;
}

/* move the record's frame back by its speed, to 0 at least. return 1
 * there, else 0 */
static int motion_rewind(struct egg_viper_record *record)
{
    record->frame -= record->speed;
    if (record->frame <= 0.0F) {
        record->frame = 0.0F;
        return 1;
    }
    return 0;
}

/* move the record's frame on by its speed. past the sprite's last frame, it
 * moves back by frames - 1 and returns 1. otherwise it returns 0 */
static int sprite_loop(struct egg_viper_record *record)
{
    struct egg_viper_sprite *sprite = record->sprite;

    record->frame += record->speed;
    if (record->frame > sprite->frames - 1) {
        record->frame -= sprite->frames - 1;
        return 1;
    }
    return 0;
}

/* move the record's frame on by its speed, to the sprite's last frame at
 * most. return 1 there, else 0 */
static int sprite_play(struct egg_viper_record *record)
{
    struct egg_viper_sprite *sprite = record->sprite;

    record->frame += record->speed;
    if (record->frame >= sprite->frames - 1) {
        record->frame = sprite->frames - 1;
        return 1;
    }
    return 0;
}

static void sprite_draw(struct egg_viper_sprite *sprite, NJS_POINT3 *pos,
                        Float frame)
{
    NJS_SPRITE *njs = sprite->sprite;

    njs->p.x = pos->x;
    njs->p.y = pos->y;
    njs->p.z = pos->z;
    njDrawSprite3D(njs, (Int)frame, sprite->attr);
}

static void sprite_draw_scaled(struct egg_viper_sprite *sprite, NJS_POINT3 *pos,
                               Float scale, Float frame)
{
    NJS_SPRITE *njs = sprite->sprite;

    njs->p.x = pos->x;
    njs->p.y = pos->y;
    njs->p.z = pos->z;
    njs->sx = scale;
    njs->sy = scale;
    njDrawSprite3D(njs, (Int)frame, sprite->attr);
}

/* draw a scorch's quad at pos at alpha, its whole texture spread over it */
static void scorch_quad_draw(NJS_POINT3 *pos, Float alpha)
{
    int i;

    scorch_uvs[0].tex.u = 0;
    scorch_uvs[0].tex.v = 0;
    scorch_uvs[1].tex.u = 0;
    scorch_uvs[1].tex.v = 255;
    scorch_uvs[2].tex.u = 255;
    scorch_uvs[2].tex.v = 255;
    scorch_uvs[3].tex.u = 255;
    scorch_uvs[3].tex.v = 0;
    for (i = 0; i < 4; i++) {
        scorch_colors[i].argb.a = alpha * 255.0F;
    }
    for (i = 0; i < 4; i++) {
        scorch_points[i].x = scorch_corners[i].x + pos->x;
        scorch_points[i].y = scorch_corners[i].y + pos->y;
        scorch_points[i].z = scorch_corners[i].z + pos->z;
    }
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetTexture(&egg_viper_effect_texlist);
    njSetTextureNum(SCORCH_TEXTURE);
    njDrawPolygon3D(&scorch_quad, 4, 0x80000060);
}

/* put where the boss's lasers start in pos, turned with the boss */
static void viper_muzzle(struct task_work *work, NJS_POINT3 *pos)
{
    Angle ang;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    njCalcPoint(NULL, &muzzle_offset, pos);
    njPopMatrix(1);
    pos->x += work->pos.x;
    pos->y += work->pos.y;
    pos->z += work->pos.z;
}

/* the same for where its ball forms */
static void viper_ball_point(struct task_work *work, NJS_POINT3 *pos)
{
    Angle ang;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    njCalcPoint(NULL, &ball_offset, pos);
    njPopMatrix(1);
    pos->x += work->pos.x;
    pos->y += work->pos.y;
    pos->z += work->pos.z;
}

/* 1 if the boss's own shapes or Eggman took a hit, else 0 */
static int viper_was_hit(struct task_work *work,
                         struct egg_viper_record *record)
{
    if ((work->flags & COLLISION_DAMAGED) || (record->flags & VIPER_HIT)) {
        return 1;
    }
    return 0;
}

/* put the place of node's matrix in matrices in pos */
static void node_pos(NJS_MATRIX *matrices, int node, NJS_POINT3 *pos)
{
    NJS_MATRIX *matrix = &matrices[node];

    pos->x = (*matrix)[12];
    pos->y = (*matrix)[13];
    pos->z = (*matrix)[14];
}
