/* the uni, a sea urchin circled by four child tasks, in three kinds. E UNI
 * A and E UNI B wait until they see a player, while E UNI C moves from the
 * start. then each floats ahead, and turns toward player 0 while it sees a
 * player. E UNI A throws its spikes ahead one at a time once it faces
 * player 0. E UNI B's spikes spiral away from it as it stops waiting. E UNI
 * C's fireballs swing out and back while it sees a player */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "animal/kinds.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/explosion.h"
#include "effect/flame.h"
#include "enemy/enemy.h"
#include "enemy/saru.h"
#include "enemy/uni.h"
#include "object/placement.h"
#include "player/knuckles.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 10
#include "enemy/uni/text.inc"

/* a uni's work->mode. E UNI C stays in UNI_WAIT until it is hit, and
 * moves in that mode */
enum uni_mode {
    UNI_WAIT = 0,
    UNI_A_APPROACH = 2, /* until it faces player 0 */
    /* each spike flies off as its angle round its circle passes between
     * 0xc000 and 0xc800 */
    UNI_A_THROW = 3,
    UNI_B_APPROACH = 6, /* its spikes spiral away */
    UNI_HIT = 9 /* an attack has hit it: it bursts on its next step */
};

enum spike_mode { SPIKE_ORBIT, SPIKE_FLY, SPIKE_IDLE, SPIKE_BURST };

enum fireball_mode {
    FIREBALL_ORBIT,
    FIREBALL_SPREAD,
    FIREBALL_HOLD,
    FIREBALL_RETURN,
    FIREBALL_FALL,
    FIREBALL_BURST
};

/* the angles a uni and its children keep in record->angles. UNI_PHASE is
 * the phase of a body's bob, and a child's angle around its circle. a
 * child's circle tilts by UNI_TILT_X and UNI_TILT_Z */
enum uni_angle { UNI_PHASE = 0, UNI_TILT_X = 3, UNI_TILT_Z = 4 };

/* where a child starts: its angle around its circle, and the circle's
 * tilts */
struct spike_place {
    Angle phase;
    Angle tilt_x;
    Angle tilt_z;
};

/* in a uni's work->flags */
#define UNI_SOLID 0x100 /* it can be touched */
#define UNI_FAR 0x200 /* it is past its range from home */
#define UNI_SEES 0x400 /* it sees a player */

/* a uni's range from home is scl.x plus MOVE_RANGE. the set editor names
 * scl.x MOVE RANGE */
#define MOVE_RANGE 50.0F

/* with scl.z at NO_GROUND or more, a uni keeps its height instead of
 * hovering over the floor. E UNI C's set editor names scl.z NOCOLLI */
#define NO_GROUND 1000.0F

/* a uni that isn't waiting rises or sinks toward HOVER_HEIGHT above the
 * floor under it. it moves back to where it was when that floor lies more
 * than MAX_DROP below its last height */
#define HOVER_HEIGHT 8.0F
#define MAX_DROP 50.0F

/* the model faces half a turn from the uni's heading */
#define MODEL_TURN 0x8000

/* the range, squared, within which the camera or player 0 keeps the uni
 * placed. placement_try_leave_by takes it */
#define UNI_LEAVE_RANGE (700.0F * 700.0F)

/* the share of its speed a uni loses a step */
#define DRAG 0.03F

#define SE_BURNING 812

/* a child's circle, kept in its record's motion: the circle's radius, and
 * how much the radius grows each step */
#define ORBIT_RADIUS(record) ((record)->motion.slip)
#define ORBIT_SPREAD(record) ((record)->motion.gravity)

/* @hack spelling: the two calls in one expression, which keeps task_kill a
 * JSR ahead of the return (0x8c10c9fc, 0x8c10d12a, 0x8c10dd00); as two
 * statements, it becomes a tail jump, and summed over the three child
 * steps, 80 of 231 words differ */
#define SPIKE_KILL(task, work, record)                                         \
    (enemy_end((task), (work), (record)), task_kill(task))

static NJS_TEXNAME uni_a_texnames[3] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
};
NJS_TEXLIST uni_a_unibody_texlist = {
#include "enemy/uni/uni_a_unibody_texlist.inc"
};

static NJS_MATERIAL uni_a_materials[4] = {
#include "enemy/uni/uni_a_materials.inc"
};

static Sint16 uni_a_meshes0[179] = {
#include "enemy/uni/uni_a_meshes0.inc"
};

static Sint16 uni_a_meshes1[30] = {
#include "enemy/uni/uni_a_meshes1.inc"
};

static Sint16 uni_a_meshes2[12] = {
#include "enemy/uni/uni_a_meshes2.inc"
};

static Sint16 uni_a_meshes3[72] = {
#include "enemy/uni/uni_a_meshes3.inc"
};

static NJS_COLOR uni_a_uvs0[148] = {
#include "enemy/uni/uni_a_uvs0.inc"
};

static NJS_COLOR uni_a_uvs1[28] = {
#include "enemy/uni/uni_a_uvs1.inc"
};

static NJS_COLOR uni_a_uvs2[10] = {
#include "enemy/uni/uni_a_uvs2.inc"
};

static NJS_COLOR uni_a_uvs3[58] = {
#include "enemy/uni/uni_a_uvs3.inc"
};

static NJS_MESHSET uni_a_meshsets[4] = {
#include "enemy/uni/uni_a_meshsets.inc"
};

static NJS_POINT3 uni_a_points[101] = {
#include "enemy/uni/uni_a_points.inc"
};

static NJS_VECTOR uni_a_normals[101] = {
#include "enemy/uni/uni_a_normals.inc"
};

static NJS_MODEL uni_a_model = {
#include "enemy/uni/uni_a_model.inc"
};

static NJS_OBJECT uni_a_object = {
#include "enemy/uni/uni_a_object.inc"
};

static NJS_TEXNAME uni_c_texnames[3] = {
    {TEXT_4, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
};
NJS_TEXLIST uni_c_unibody_texlist = {
#include "enemy/uni/uni_c_unibody_texlist.inc"
};

static NJS_MATERIAL uni_c_materials[4] = {
#include "enemy/uni/uni_c_materials.inc"
};

static Sint16 uni_c_meshes0[179] = {
#include "enemy/uni/uni_c_meshes0.inc"
};

static Sint16 uni_c_meshes1[30] = {
#include "enemy/uni/uni_c_meshes1.inc"
};

static Sint16 uni_c_meshes2[12] = {
#include "enemy/uni/uni_c_meshes2.inc"
};

static Sint16 uni_c_meshes3[72] = {
#include "enemy/uni/uni_c_meshes3.inc"
};

static NJS_COLOR uni_c_uvs0[148] = {
#include "enemy/uni/uni_c_uvs0.inc"
};

static NJS_COLOR uni_c_uvs1[28] = {
#include "enemy/uni/uni_c_uvs1.inc"
};

static NJS_COLOR uni_c_uvs2[10] = {
#include "enemy/uni/uni_c_uvs2.inc"
};

static NJS_COLOR uni_c_uvs3[58] = {
#include "enemy/uni/uni_c_uvs3.inc"
};

static NJS_MESHSET uni_c_meshsets[4] = {
#include "enemy/uni/uni_c_meshsets.inc"
};

static NJS_POINT3 uni_c_points[101] = {
#include "enemy/uni/uni_c_points.inc"
};

static NJS_VECTOR uni_c_normals[101] = {
#include "enemy/uni/uni_c_normals.inc"
};

static NJS_MODEL uni_c_model = {
#include "enemy/uni/uni_c_model.inc"
};

static NJS_OBJECT uni_c_object = {
#include "enemy/uni/uni_c_object.inc"
};

static NJS_MATERIAL uni_c_ball_materials[2] = {
#include "enemy/uni/uni_c_ball_materials.inc"
};

static Sint16 uni_c_ball_meshes0[152] = {
#include "enemy/uni/uni_c_ball_meshes0.inc"
};

static Sint16 uni_c_ball_meshes1[30] = {
#include "enemy/uni/uni_c_ball_meshes1.inc"
};

static NJS_COLOR uni_c_ball_uvs0[114] = {
#include "enemy/uni/uni_c_ball_uvs0.inc"
};

static NJS_MESHSET uni_c_ball_meshsets[2] = {
#include "enemy/uni/uni_c_ball_meshsets.inc"
};

static NJS_POINT3 uni_c_ball_points[33] = {
#include "enemy/uni/uni_c_ball_points.inc"
};

static NJS_VECTOR uni_c_ball_normals[33] = {
#include "enemy/uni/uni_c_ball_normals.inc"
};

static NJS_MODEL uni_c_ball_model = {
#include "enemy/uni/uni_c_ball_model.inc"
};

/* @hack type: an array of one, whose model the draws read 4 past the
 * object's address (MOV.L @(4,R4),R4 at 0x8c10d394, 0x8c10d57c); as a plain
 * object, the pool holds the model member's address, and summed over
 * uni_c_fireball_draw_burning and uni_c_fireball_draw, 4 of 270 words
 * differ */
static NJS_OBJECT uni_c_ball_object[1] = {
#include "enemy/uni/uni_c_ball_object.inc"
};

/* clang-format off */
static struct collision_shape uni_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS,
    PUSH_FIRM(PUSH_PLAYERS), {0.0F, 0.4F, 0.0F}, 4.5F, 0.0F, 0.0F, {0}, 0,
    NULL};
static struct collision_shape uni_spike_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(2) | GUARD(1) | SHAPE_HURTS_PLAYERS,
    PUSH_FIRM(PUSH_PLAYERS), {0.0F, 0.0F, 0.0F}, 3.5F, 0.0F, 0.0F, {0}, 0,
    NULL};
/* clang-format on */

static struct spike_place uni_a_spike_places[4] = {
    {0, 0x2000, 0},
    {0x8000, 0x2000, 0},
    {0x4000, 0x6000, 0},
    {0xc000, 0x6000, 0},
};

/* where E UNI B's spikes and E UNI C's fireballs start. the four children
 * take submode 0 to 3, so nothing reads the last four entries */
static struct spike_place uni_b_spike_places[8] = {
#include "enemy/uni/uni_b_spike_places.inc"
};

static void uni_spike_drift(struct task_work *work,
                            struct lockon_target *record)
{
    record->motion.speed.y += record->motion.accel.y - 0.0004F;
    work->pos.x += record->motion.speed.x;
    work->pos.y += record->motion.speed.y;
    work->pos.z += record->motion.speed.z;
    record->motion.accel.x = 0.0F;
    record->motion.accel.y = 0.0F;
    record->motion.accel.z = 0.0F;
}

static void uni_c_fireball_move(struct task_work *work,
                                struct lockon_target *record)
{
    /* @hack spelling: the drag's product negated and added, FNEG and FADD at
     * 0x8c10c524 and 0x8c10c526; with -=, SHC subtracts, the function runs 4
     * bytes short and 64 of 74 words differ. the speed meets one product,
     * 0x3c449ba4, as SHC folds DRAG * 0.4F */
    record->motion.accel.x += -(record->motion.speed.x * (DRAG * 0.4F));
    record->motion.accel.z += -(record->motion.speed.z * (DRAG * 0.4F));
    record->motion.speed.x += record->motion.accel.x;
    record->motion.speed.y += record->motion.accel.y - 0.3F;
    record->motion.speed.z += record->motion.accel.z;
    work->pos.x += record->motion.speed.x;
    work->pos.y += record->motion.speed.y;
    work->pos.z += record->motion.speed.z;
    record->motion.accel.x = 0.0F;
    record->motion.accel.y = 0.0F;
    record->motion.accel.z = 0.0F;
}

static void uni_spike_burst(struct task_work *work,
                            struct lockon_target *record)
{
    explosion_create_at(work->pos.x, work->pos.y + 2.0F, work->pos.z, 1.0F);
}

/* draw the spike and advance its spin. it does both only when draw_in_view
 * passes, in view on a frame's first step */
static void uni_a_spike_draw_spinning(struct task_work *work,
                                      struct lockon_target *record)
{
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    njSetTexture(&togeball_togeball_texlist);
    work->ang.x += 0x200;
    work->ang.z += 0x300;
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    draw_tree(&togeball_object);
    njPopMatrix(1);
}

/* the spike as it stands, which task_draw_all draws while the game is
 * paused. it advances nothing */
static void uni_a_spike_draw(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(&togeball_togeball_texlist);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    draw_tree(&togeball_object);
    njPopMatrix(1);
}

static void uni_a_spike_orbit(struct task *task, struct lockon_target *record)
{
    struct task_work *body = task->parent->work;
    struct task_work *work = task->work;
    NJS_POINT3 *center = &body->pos;
    Float radius = ORBIT_RADIUS(record);
    Angle yaw = body->ang.y;
    int phase = (Uint16)record->angles[UNI_PHASE];
    NJS_VECTOR offset;

    /* @hack redundant: work->mode read and dropped, which puts record in R13
     * and work in R14 (MOV R5,R13 at 0x8c10c7b6, MOV.L @(32,R10),R14 at
     * 0x8c10c7ba); without it, the two trade and 20 of 155 words differ */
    (void)work->mode;
    offset.x = radius * njCos(phase);
    offset.y = 0.0F;
    offset.z = radius * njSin(phase);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, center);
    TURN(njRotateY, yaw);
    TURN(njRotateZ, record->angles[UNI_TILT_Z]);
    TURN(njRotateX, record->angles[UNI_TILT_X]);
    njCalcPoint(NULL, &offset, &work->pos);
    njPopMatrix(1);
    uni_a_spike_draw_spinning(work, record);
    record->angles[UNI_PHASE] += 0x400;
    /* @hack spelling: the body's mode read through task->parent->work in both
     * tests, not through body, which reloads both pointers at each
     * (0x8c10c84e, 0x8c10c850); through body, SHC keeps it in a register
     * and 147 of 155 words differ */
    if (task->parent->work->mode == UNI_A_THROW && phase > 0xc000 &&
        phase < 0xc800) {
        NJS_VECTOR speed = {1.45F, -0.1F, 0.0F};

        ENEMY_SET_MODE(work, record, SPIKE_FLY);
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, yaw);
        njCalcVector(NULL, &speed, &record->motion.speed);
        njPopMatrix(1);
    }
    if (task->parent->work->mode == UNI_HIT) {
        ENEMY_SET_MODE(work, record, SPIKE_BURST);
    }
}

static void uni_a_spike_fly(struct task *task, struct lockon_target *record)
{
    /* @hack type: volatile on body, which keeps the image's read of
     * the body's pos.x, MOV.L @R3,R3 at 0x8c10c934; without it, SHC drops
     * the read, the function runs 12 bytes short and 47 of 49 words
     * differ */
    volatile struct task_work *body = task->parent->work;
    struct task_work *work = task->work;

    (void)body->pos.x;
    uni_spike_drift(work, record);
    enemy_collide(work, record);
    if (record->flags & (ENEMY_ON_GROUND | ENEMY_ON_WALL)) {
        ENEMY_SET_MODE(work, record, SPIKE_BURST);
    }
    uni_a_spike_draw_spinning(work, record);
    record->angles[UNI_PHASE] += 0x200;
    ORBIT_RADIUS(record) += 0.3F;
}

static void uni_a_spike_step(struct task *task)
{
    /* @hack redundant: register on work and record, which puts task in R12
     * and record in R13 (MOV R4,R12 at 0x8c10c988, MOV.L @(36,R12),R13 at
     * 0x8c10c99a); as plain locals, the two trade and 15 of 66 words
     * differ */
    register struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    register struct lockon_target *record = LOCKON_TARGET(task);

    /* @hack redundant: player 0's work read through player and dropped, which
     * keeps its load into R4 (0x8c10c994); without it, 27 of 66 words
     * differ */
    (void)player->mode;
    /* @hack spelling: an empty arm for edit_mode, which loads enemy_end's
     * address into R3 and task_kill's and enemy_save_pos's into R2
     * (0x8c10c9f0, 0x8c10c9fa, 0x8c10ca04); as if (!edit_mode), the three
     * load into R2, R3 and R3, and 6 of 66 words differ */
    if (edit_mode) {
    } else {
        switch (work->mode) {
        case SPIKE_ORBIT:
            uni_a_spike_orbit(task, record);
            break;
        case SPIKE_FLY:
            uni_a_spike_fly(task, record);
            break;
        case SPIKE_IDLE:
        case SPIKE_BURST:
            uni_spike_burst(work, record);
            SPIKE_KILL(task, work, record);
            return;
        /* @hack redundant: an empty default, which loads enemy_save_pos's
         * address into R2 (0x8c10ca04); without it, R3, and 2 of 66 words
         * differ */
        default:
            break;
        }
        enemy_save_pos(work, record);
        work->timer++;
        collision_register(work);
    }
}

static void uni_spike_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    enemy_end(task, work, record);
}

static void uni_a_spike_start(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;

    collision_init(task, &uni_spike_shape, COLLISION_OBJECT, 1, 1);
    record = enemy_alloc_record(task, work);
    record->flags |= ENEMY_KEEP_ON_LEDGE | ENEMY_SHADOW;
    work->tick = 0;
    work->timer = 0;
    /* @hack redundant: the radius set to 0, which the 10.0F below replaces
     * before any read. it keeps the image's store of FR15 (0x8c10caa2);
     * without it, SHC no longer keeps 0 in FR15, the function runs 8 bytes
     * short and 141 of 148 words differ */
    ORBIT_RADIUS(record) = 0.0F;
    record->turn_speed = 0xe0;
    work->ang.x = (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
    work->ang.z = (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
    record->aim_y = work->ang.y;
    record->frame = 0.0F;
    record->probe_offset.x = 0.0F;
    record->probe_offset.y = 0.0F;
    record->probe_offset.z = 0.0F;
    record->top = 3.0F;
    record->radius = 15.0F;
    record->bottom = -3.0F;
    record->wall_bounce = 0.7F;
    record->floor_bounce = 0.7F;
    record->unk_c4 = 0.7F;
    record->shadow_size = 1.6F;
    record->shadow_ratio = 1.0F;
    ORBIT_RADIUS(record) = 10.0F;
    enemy_set_home(work, record);
    task->destructor = uni_spike_destroy;
    task->step = uni_a_spike_step;
    task->draw = uni_a_spike_draw;
    work->flags |= UNI_SOLID;
    record->angles[UNI_PHASE] = uni_a_spike_places[work->submode].phase;
    record->angles[UNI_TILT_X] = uni_a_spike_places[work->submode].tilt_x;
    record->angles[UNI_TILT_Z] = uni_a_spike_places[work->submode].tilt_z;
}

/* E UNI B's spikes draw with these, the same code as
 * uni_a_spike_draw_spinning and uni_a_spike_draw */
static void uni_b_spike_draw_spinning(struct task_work *work,
                                      struct lockon_target *record)
{
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    njSetTexture(&togeball_togeball_texlist);
    work->ang.x += 0x200;
    work->ang.z += 0x300;
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    draw_tree(&togeball_object);
    njPopMatrix(1);
}

static void uni_b_spike_draw(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(&togeball_togeball_texlist);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    draw_tree(&togeball_object);
    njPopMatrix(1);
}

static void uni_b_spike_orbit(struct task *task, struct lockon_target *record)
{
    struct task_work *body = task->parent->work;
    struct task_work *work = task->work;
    NJS_POINT3 *center = &body->pos;
    Float radius = ORBIT_RADIUS(record);
    Angle yaw = body->ang.y;
    int phase = (Uint16)record->angles[UNI_PHASE];
    NJS_VECTOR offset;

    /* @hack redundant: work->mode read and dropped, which puts record in R13
     * and work in R14 (MOV R5,R13 at 0x8c10cdb6, MOV.L @(32,R11),R14 at
     * 0x8c10cdba); without it, the two trade and 20 of 149 words differ */
    (void)work->mode;
    offset.x = radius * njCos(phase);
    offset.y = 0.0F;
    offset.z = radius * njSin(phase);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, center);
    TURN(njRotateY, yaw);
    TURN(njRotateZ, record->angles[UNI_TILT_Z]);
    TURN(njRotateX, record->angles[UNI_TILT_X]);
    njCalcPoint(NULL, &offset, &work->pos);
    njPopMatrix(1);
    uni_b_spike_draw_spinning(work, record);
    record->angles[UNI_PHASE] += 0x200;
    /* @hack spelling: the body's mode read through task->parent->work in both
     * tests, not through body, which reloads both pointers at each
     * (0x8c10ce50, 0x8c10ce52); through body, SHC keeps it in a register and
     * 141 of 149 words differ */
    if (task->parent->work->mode == UNI_B_APPROACH) {
        NJS_VECTOR speed = {1.0F, 0.0F, 0.0F};

        ENEMY_SET_MODE(work, record, SPIKE_FLY);
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, yaw);
        njCalcVector(NULL, &speed, &record->motion.speed);
        njPopMatrix(1);
    }
    if (task->parent->work->mode == UNI_HIT) {
        ENEMY_SET_MODE(work, record, SPIKE_BURST);
    }
}

static void uni_b_spike_fly(struct task *task, struct lockon_target *record)
{
    struct task_work *body = task->parent->work;
    struct task_work *work = task->work;
    NJS_POINT3 *center = &body->pos;
    Float radius = ORBIT_RADIUS(record);
    Angle yaw = body->ang.y;
    int phase = (Uint16)record->angles[UNI_PHASE];
    NJS_VECTOR offset;

    /* @hack redundant: work->mode read and dropped, which puts phase in R11 and
     * yaw in R12 (0x8c10cf44, 0x8c10cf46); without it, the two trade and 7
     * of 149 words differ */
    (void)work->mode;
    offset.x = radius * njCos(phase);
    offset.y = 0.0F;
    offset.z = radius * njSin(phase);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, center);
    TURN(njRotateY, yaw);
    TURN(njRotateZ, record->angles[UNI_TILT_Z]);
    TURN(njRotateX, record->angles[UNI_TILT_X]);
    njCalcPoint(NULL, &offset, &work->pos);
    njPopMatrix(1);
    enemy_collide(work, record);
    if (record->flags & (ENEMY_ON_GROUND | ENEMY_ON_WALL)) {
        ENEMY_SET_MODE(work, record, SPIKE_BURST);
    }
    if (work->collision->flags & COLLISION_HIT) {
        ENEMY_SET_MODE(work, record, SPIKE_BURST);
    }
    uni_b_spike_draw_spinning(work, record);
    if (record->turn_speed < 0x400) {
        record->turn_speed += 4;
    }
    if (ORBIT_SPREAD(record) < 2.5F) {
        ORBIT_SPREAD(record) += 0.003F;
    }
    record->angles[UNI_PHASE] += record->turn_speed;
    ORBIT_RADIUS(record) += ORBIT_SPREAD(record);
}

static void uni_b_spike_burst_with_body(struct task *task,
                                        struct lockon_target *record)
{
    struct task_work *work = task->work;
    struct task_work *body = task->parent->work;

    if (body->mode == UNI_HIT) {
        ENEMY_SET_MODE(work, record, SPIKE_BURST);
    }
}

static void uni_b_spike_step(struct task *task)
{
    /* @hack redundant: register on work and record, which puts task in R12,
     * work in R13 and record in R14 (MOV R4,R12 at 0x8c10d0c4, MOV.L
     * @(32,R12),R13 at 0x8c10d0ce, MOV.L @(36,R12),R14 at 0x8c10d0d6); as
     * plain locals, task takes R14, work R12 and record R13, and 29 of 77
     * words differ */
    register struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    register struct lockon_target *record = LOCKON_TARGET(task);

    /* @hack redundant: player 0's work read through player and dropped, which
     * keeps its load into R4 (0x8c10d0d0); without it, 75 of 77 words
     * differ */
    (void)player->mode;
    /* @hack spelling: an empty arm for edit_mode, which keeps SPIKE_IDLE's own
     * call of uni_b_spike_burst_with_body (BSR at 0x8c10d110); as if
     * (!edit_mode), that case runs into SPIKE_FLY's call, the function runs
     * 10 bytes short and 38 of 77 words differ */
    if (edit_mode) {
    } else {
        switch (work->mode) {
        case SPIKE_ORBIT:
            uni_b_spike_orbit(task, record);
            uni_b_spike_burst_with_body(task, record);
            break;
        case SPIKE_FLY:
            uni_b_spike_fly(task, record);
            uni_b_spike_burst_with_body(task, record);
            break;
        case SPIKE_IDLE:
            uni_b_spike_burst_with_body(task, record);
            break;
        case SPIKE_BURST:
            uni_spike_burst(work, record);
            SPIKE_KILL(task, work, record);
            return;
        /* @hack redundant: an empty default, which loads enemy_save_pos's
         * address into R2 (0x8c10d132); without it, R3, and 2 of 77 words
         * differ */
        default:
            break;
        }
        enemy_save_pos(work, record);
        work->timer++;
        collision_register(work);
    }
}

static void uni_b_spike_start(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;

    collision_init(task, &uni_spike_shape, COLLISION_OBJECT, 1, 1);
    record = enemy_alloc_record(task, work);
    record->flags |= ENEMY_KEEP_ON_LEDGE | ENEMY_SHADOW;
    work->tick = 0;
    work->timer = 0;
    /* @hack redundant: the radius set to 0, which the 10.0F below replaces
     * before any read. it keeps the image's store of FR15 (0x8c10d1c2);
     * without it, SHC no longer keeps 0 in FR15, the function runs 8 bytes
     * short and 138 of 144 words differ */
    ORBIT_RADIUS(record) = 0.0F;
    record->turn_speed = 0xe0;
    work->ang.x = (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
    work->ang.z = (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
    record->aim_y = work->ang.y;
    record->frame = 0.0F;
    record->probe_offset.x = 0.0F;
    record->probe_offset.y = 0.0F;
    record->probe_offset.z = 0.0F;
    record->top = 3.0F;
    record->radius = 3.0F;
    record->bottom = -3.0F;
    record->wall_bounce = 1.0F;
    record->floor_bounce = 1.0F;
    record->unk_c4 = 1.0F;
    record->shadow_size = 1.6F;
    record->shadow_ratio = 1.0F;
    ORBIT_RADIUS(record) = 10.0F;
    enemy_set_home(work, record);
    task->destructor = uni_spike_destroy;
    task->step = uni_b_spike_step;
    task->draw = uni_b_spike_draw;
    work->flags |= UNI_SOLID;
    record->angles[UNI_PHASE] = uni_b_spike_places[work->submode].phase;
    record->angles[UNI_TILT_X] = uni_b_spike_places[work->submode].tilt_x;
    record->angles[UNI_TILT_Z] = uni_b_spike_places[work->submode].tilt_z;
}

/* draw the fireball, advance its spin, and make a flame 4 toward the
 * camera from it. it does all three only when draw_in_view passes, in view
 * on a frame's first step. the flame comes every third such step while the
 * circle's radius is under 20, and every one from 20 */
static void uni_c_fireball_draw_burning(struct task_work *work,
                                        struct lockon_target *record)
{
    NJS_VECTOR toward;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    njSetTexture(&uni_c_unibody_texlist);
    work->ang.x += 0x200;
    work->ang.z += 0x300;
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    njScale(NULL, 0.6F, 0.6F, 0.6F);
    draw_model(uni_c_ball_object[0].model);
    njPopMatrix(1);
    toward.x = camera_work->pos.x - work->pos.x;
    toward.y = camera_work->pos.y - work->pos.y;
    toward.z = camera_work->pos.z - work->pos.z;
    njUnitVector(&toward);
    toward.x *= 4.0F;
    toward.y *= 4.0F;
    toward.z *= 4.0F;
    if (ORBIT_RADIUS(record) < 20.0F) {
        if (work->tick++ > 1) {
            NJS_VECTOR speed = {0.0F, 0.1F, 0.0F};
            NJS_POINT3 pos;

            pos = work->pos;
            njAddVector(&pos, &toward);
            flame_create(&pos, &speed, 0.9F);
            work->tick = 0;
        }
    } else {
        NJS_VECTOR speed = {0.0F, 0.1F, 0.0F};
        NJS_POINT3 pos;

        pos = work->pos;
        njAddVector(&pos, &toward);
        flame_create(&pos, &speed, 1.0F);
        work->tick = 0;
    }
}

static void uni_c_fireball_draw(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(&uni_c_unibody_texlist);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    njScale(NULL, 0.6F, 0.6F, 0.6F);
    draw_model(uni_c_ball_object[0].model);
    njPopMatrix(1);
}

static void uni_c_fireball_fall_with_body(struct task *task,
                                          struct lockon_target *record)
{
    struct task_work *work = task->work;
    struct task_work *body = task->parent->work;

    if (body->mode == UNI_HIT) {
        NJS_VECTOR speed = {1.0F, 0.0F, 0.0F};

        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, body->ang.y - record->angles[UNI_PHASE] + 0xc000);
        njCalcVector(NULL, &speed, &record->motion.speed);
        njPopMatrix(1);
        ENEMY_SET_MODE(work, record, FIREBALL_FALL);
    }
}

static void uni_c_fireball_orbit(struct task *task,
                                 struct lockon_target *record)
{
    struct task_work *body = task->parent->work;
    struct task_work *work = task->work;
    NJS_POINT3 *center = &body->pos;
    Float radius = ORBIT_RADIUS(record);
    Angle yaw = body->ang.y;
    int phase = (Uint16)record->angles[UNI_PHASE];
    NJS_VECTOR offset;

    offset.x = radius * njCos(phase);
    offset.y = 0.0F;
    offset.z = radius * njSin(phase);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, center);
    TURN(njRotateY, yaw);
    TURN(njRotateZ, record->angles[UNI_TILT_Z]);
    TURN(njRotateX, record->angles[UNI_TILT_X]);
    njCalcPoint(NULL, &offset, &work->pos);
    njPopMatrix(1);
    uni_c_fireball_draw_burning(work, record);
    record->angles[UNI_PHASE] += 0x360;
    record->angles[UNI_TILT_X] += 0x1a0;
    /* @hack spelling: the body's flags read through task->parent->work, not
     * through body, which reloads both pointers (0x8c10d742, 0x8c10d744);
     * through body, SHC keeps it in a register and 143 of 154 words differ */
    if (TASK_TIMER(work) > 60 && (task->parent->work->flags & UNI_SEES)) {
        NJS_VECTOR speed = {1.0F, 0.0F, 0.0F};

        ENEMY_SET_MODE(work, record, FIREBALL_SPREAD);
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, yaw);
        njCalcVector(NULL, &speed, &record->motion.speed);
        njPopMatrix(1);
        record->turn_speed = 0x360;
    }
    uni_c_fireball_fall_with_body(task, record);
}

static void uni_c_fireball_spread(struct task *task,
                                  struct lockon_target *record)
{
    struct task_work *body = task->parent->work;
    struct task_work *work = task->work;
    NJS_POINT3 *center = &body->pos;
    Angle yaw = body->ang.y;
    Float radius = ORBIT_RADIUS(record);
    int phase = (Uint16)record->angles[UNI_PHASE];
    NJS_VECTOR offset;

    /* @hack redundant: work->mode read and dropped, which puts phase in R10 and
     * yaw in R11 (0x8c10d836, 0x8c10d82e); without it, the two trade, and so
     * do two slots of the frame, and 13 of 154 words differ */
    (void)work->mode;
    record->angles[UNI_TILT_X] =
        angle_approach(record->angles[UNI_TILT_X], 0, 0x200);
    offset.x = radius * njCos(phase);
    offset.y = 0.0F;
    offset.z = radius * njSin(phase);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, center);
    TURN(njRotateY, yaw);
    TURN(njRotateZ, record->angles[UNI_TILT_Z]);
    TURN(njRotateX, record->angles[UNI_TILT_X]);
    njCalcPoint(NULL, &offset, &work->pos);
    njPopMatrix(1);
    uni_c_fireball_draw_burning(work, record);
    if (record->turn_speed < 0x500) {
        record->turn_speed += 2;
    }
    if (ORBIT_SPREAD(record) < 2.5F) {
        ORBIT_SPREAD(record) += 0.015F;
    }
    record->angles[UNI_PHASE] += record->turn_speed;
    ORBIT_RADIUS(record) += ORBIT_SPREAD(record);
    if (ORBIT_RADIUS(record) > 50.0F) {
        ENEMY_SET_MODE(work, record, FIREBALL_HOLD);
    }
    uni_c_fireball_fall_with_body(task, record);
}

static void uni_c_fireball_hold(struct task *task, struct lockon_target *record)
{
    struct task_work *body = task->parent->work;
    struct task_work *work = task->work;
    NJS_POINT3 *center = &body->pos;
    Float radius = ORBIT_RADIUS(record);
    Angle yaw = body->ang.y;
    int phase = (Uint16)record->angles[UNI_PHASE];
    NJS_VECTOR offset;

    /* @hack redundant: work->mode read and dropped, which puts phase in R10 and
     * yaw in R11 (0x8c10d9c8, 0x8c10d9ca); without it, the two trade and 7
     * of 115 words differ */
    (void)work->mode;
    offset.x = radius * njCos(phase);
    offset.y = 0.0F;
    offset.z = radius * njSin(phase);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, center);
    TURN(njRotateY, yaw);
    TURN(njRotateZ, record->angles[UNI_TILT_Z]);
    TURN(njRotateX, record->angles[UNI_TILT_X]);
    njCalcPoint(NULL, &offset, &work->pos);
    njPopMatrix(1);
    uni_c_fireball_draw_burning(work, record);
    record->angles[UNI_PHASE] += record->turn_speed;
    if (TASK_TIMER(work) > 60) {
        ENEMY_SET_MODE(work, record, FIREBALL_RETURN);
        ORBIT_SPREAD(record) = 0.0F;
    }
    uni_c_fireball_fall_with_body(task, record);
}

static void uni_c_fireball_return(struct task *task,
                                  struct lockon_target *record)
{
    struct task_work *body = task->parent->work;
    struct task_work *work = task->work;
    NJS_POINT3 *center = &body->pos;
    Float radius = ORBIT_RADIUS(record);
    Angle yaw = body->ang.y;
    int phase = (Uint16)record->angles[UNI_PHASE];
    NJS_VECTOR offset;

    offset.x = radius * njCos(phase);
    offset.y = 0.0F;
    offset.z = radius * njSin(phase);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, center);
    TURN(njRotateY, yaw);
    TURN(njRotateZ, record->angles[UNI_TILT_Z]);
    TURN(njRotateX, record->angles[UNI_TILT_X]);
    njCalcPoint(NULL, &offset, &work->pos);
    njPopMatrix(1);
    uni_c_fireball_draw_burning(work, record);
    if (record->turn_speed > 0x360) {
        record->turn_speed -= 4;
    }
    if (ORBIT_SPREAD(record) > -2.0F) {
        ORBIT_SPREAD(record) -= 0.002F;
    }
    record->angles[UNI_PHASE] += record->turn_speed;
    ORBIT_RADIUS(record) += ORBIT_SPREAD(record);
    if (ORBIT_RADIUS(record) < 7.0F) {
        ENEMY_SET_MODE(work, record, FIREBALL_ORBIT);
        ORBIT_SPREAD(record) = 0.0F;
    }
    uni_c_fireball_fall_with_body(task, record);
}

static void uni_c_fireball_fall(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;

    uni_c_fireball_move(work, record);
    enemy_collide(work, record);
    uni_c_fireball_draw_burning(work, record);
    if (record->flags & ENEMY_ON_GROUND) {
        ENEMY_SET_MODE(work, record, FIREBALL_BURST);
    }
}

static void uni_c_fireball_step(struct task *task)
{
    /* @hack redundant: register on work and record, which puts task in R12,
     * work in R13 and record in R14 (MOV R4,R12 at 0x8c10dc84, MOV.L
     * @(32,R12),R13 at 0x8c10dc8e, MOV.L @(36,R12),R14 at 0x8c10dc96); as
     * plain locals, task takes R14, work R12 and record R13, and 29 of 88
     * words differ */
    register struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    register struct lockon_target *record = LOCKON_TARGET(task);

    /* @hack redundant: player 0's work read through player and dropped, which
     * keeps its load into R4 (0x8c10dc90); without it, the function runs 64
     * bytes short and 87 of 88 words differ */
    (void)player->mode;
    /* @hack spelling: an empty arm for edit_mode, which loads the addresses of
     * uni_spike_burst, enemy_end and task_kill into R1, R3 and R2, and
     * enemy_save_pos's into R2 (0x8c10dcec to 0x8c10dd08); as if
     * (!edit_mode), they load into R3, R2, R3 and R3, and 8 of 88 words
     * differ */
    if (edit_mode) {
    } else {
        switch (work->mode) {
        case FIREBALL_ORBIT:
            uni_c_fireball_orbit(task, record);
            break;
        case FIREBALL_SPREAD:
            uni_c_fireball_spread(task, record);
            break;
        case FIREBALL_HOLD:
            uni_c_fireball_hold(task, record);
            break;
        case FIREBALL_RETURN:
            uni_c_fireball_return(task, record);
            break;
        case FIREBALL_FALL:
            uni_c_fireball_fall(task, record);
            break;
        case FIREBALL_BURST:
            uni_spike_burst(work, record);
            SPIKE_KILL(task, work, record);
            return;
        /* @hack redundant: an empty default, which loads enemy_save_pos's
         * address into R2 (0x8c10dd08); without it, R3, and 2 of 88 words
         * differ */
        default:
            break;
        }
        enemy_save_pos(work, record);
        work->timer++;
        collision_register(work);
    }
}

static void uni_c_fireball_start(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;

    collision_init(task, &uni_spike_shape, COLLISION_OBJECT, 1, 1);
    record = enemy_alloc_record(task, work);
    record->flags |= ENEMY_KEEP_ON_LEDGE | ENEMY_SHADOW;
    work->tick = 0;
    work->timer = 0;
    /* @hack redundant: the radius set to 0, which the 7.0F below replaces
     * before any read. it keeps the image's store of FR15 (0x8c10ddcc);
     * without it, SHC no longer keeps 0 in FR15, the function runs 8 bytes
     * short and 141 of 148 words differ */
    ORBIT_RADIUS(record) = 0.0F;
    record->turn_speed = 0xe0;
    work->ang.x = (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
    work->ang.z = (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
    record->aim_y = work->ang.y;
    record->frame = 0.0F;
    record->probe_offset.x = 0.0F;
    record->probe_offset.y = 0.0F;
    record->probe_offset.z = 0.0F;
    record->top = 3.0F;
    record->radius = 3.0F;
    record->bottom = -3.0F;
    record->wall_bounce = 0.7F;
    record->floor_bounce = 0.9F;
    record->unk_c4 = 0.7F;
    record->shadow_size = 1.6F;
    record->shadow_ratio = 1.0F;
    ORBIT_RADIUS(record) = 7.0F;
    enemy_set_home(work, record);
    task->destructor = uni_spike_destroy;
    task->step = uni_c_fireball_step;
    task->draw = uni_c_fireball_draw;
    work->flags |= UNI_SOLID;
    record->angles[UNI_PHASE] = uni_b_spike_places[work->submode].phase;
    record->angles[UNI_TILT_X] = uni_b_spike_places[work->submode].tilt_x;
    record->angles[UNI_TILT_Z] = uni_b_spike_places[work->submode].tilt_z;
}

/* when something hits the uni and enemy_take_hit returns TRUE, set the
 * uni to burst on its next step */
static void uni_take_hit(struct task_work *work, struct lockon_target *record)
{
    struct collision_work *collision;

    /* @hack redundant: player_work handed to enemy_ignore, which keeps its
     * load into R4 (0x8c10df26); without it, the function runs 2 bytes short
     * and 28 of 31 words differ */
    enemy_ignore((void *)player_work);
    /* @hack redundant: hit_shape read and dropped, which keeps its MOV.W
     * (0x8c10df30); without it, the function runs 6 bytes short and 28 of
     * 31 words differ */
    (void)work->collision->hit_shape;
    collision = work->collision;
    /* @hack redundant: the hit handed to enemy_ignore, which keeps its load
     * into R4 in the delay slot of the COLLISION_HIT test's branch
     * (0x8c10df38); without it, the function runs 6 bytes long and 30 of 31
     * words differ */
    enemy_ignore(collision->hit);
    if (!(collision->flags & COLLISION_HIT)) {
        return;
    }
    if (!enemy_take_hit(work, record)) {
        return;
    }
    work->mode = UNI_HIT;
    /* this overwrites bytes 1 and 2 of vars.l[0], which holds E UNI C's copy
     * of ang.z. knuckles_take_piece reads only byte 0, the piece, which
     * stays as it was */
    ENEMY_MARK_BEATEN(work);
    work->scl.z = 0.35F;
}

static void uni_move(struct task_work *work, struct lockon_target *record)
{
    /* @hack spelling: the drag's product negated and added, FNEG and FADD at
     * 0x8c10df72 and 0x8c10df74; with -=, SHC subtracts, the function runs 6
     * bytes short and 72 of 79 words differ */
    record->motion.accel.x += -(record->motion.speed.x * DRAG);
    record->motion.accel.y += -(record->motion.speed.y * DRAG);
    record->motion.accel.z += -(record->motion.speed.z * DRAG);
    record->motion.speed.x += record->motion.accel.x;
    record->motion.speed.y += record->motion.accel.y;
    record->motion.speed.z += record->motion.accel.z;
    work->pos.x += record->motion.speed.x;
    work->pos.y += record->motion.speed.y;
    work->pos.z += record->motion.speed.z;
    record->motion.accel.x = 0.0F;
    record->motion.accel.y = 0.0F;
    record->motion.accel.z = 0.0F;
}

static void uni_a_draw_body(struct task_work *work,
                            struct lockon_target *record)
{
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    njSetTexture(&uni_a_unibody_texlist);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x,
                work->pos.y + (njSin(record->angles[UNI_PHASE]) * 0.4F),
                work->pos.z);
    TURN(njRotateY, work->ang.y);
    njRotateY(NULL, MODEL_TURN);
    draw_tree(&uni_a_object);
    njPopMatrix(1);
}

static void uni_a_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    uni_a_draw_body(work, record);
}

static void uni_c_draw_body(struct task_work *work,
                            struct lockon_target *record)
{
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    njSetTexture(&uni_c_unibody_texlist);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x,
                work->pos.y + (njSin(record->angles[UNI_PHASE]) * 0.4F),
                work->pos.z);
    TURN(njRotateY, work->ang.y);
    njRotateY(NULL, MODEL_TURN);
    draw_tree(&uni_c_object);
    njPopMatrix(1);
}

static void uni_c_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    uni_c_draw_body(work, record);
}

static void uni_a_edit_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 edge;
    NJS_POINT3 from;
    NJS_POINT3 to;
    int i;

    if (placement_editing(task)) {
        if (work->tick++ > 50) {
            work->tick = 0;
        }
        njPrintC(NJM_LOCATION(17, 16), "->             ");
        njPrintC(NJM_LOCATION(17, 17), "-> ROT Y       ");
        njPrintC(NJM_LOCATION(17, 18), "-> EMERALD     ");
        njPrintC(NJM_LOCATION(17, 19), "-> MOVE RANGE");
        njPrintC(NJM_LOCATION(17, 20), "->             ");
        njPrintC(NJM_LOCATION(17, 21), "->             ");
    }
    edge.x = work->scl.x + MOVE_RANGE;
    edge.y = 0.0F;
    edge.z = 0.0F;
    for (i = 0; i < ENEMY_RANGE_SEGMENTS; i++) {
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, i * ENEMY_RANGE_TURN);
        njCalcPoint(NULL, &edge, &from);
        njPopMatrix(1);
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, (i + 1) * ENEMY_RANGE_TURN);
        njCalcPoint(NULL, &edge, &to);
        njPopMatrix(1);
        draw_line_v(&from, &to);
    }
    njSetTexture(&uni_a_unibody_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njRotateY(NULL, MODEL_TURN);
    draw_tree(&uni_a_object);
    njPopMatrix(1);
}

static void uni_c_edit_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 edge;
    NJS_POINT3 from;
    NJS_POINT3 to;
    int i;

    if (placement_editing(task)) {
        if (work->tick > 25) {
            njPrintC(NJM_LOCATION(8, 8), "< BURNING UNI >");
        } else {
            njPrintC(NJM_LOCATION(8, 8), "<             >");
        }
        if (work->tick++ > 50) {
            work->tick = 0;
        }
        njPrintC(NJM_LOCATION(17, 16), "->             ");
        njPrintC(NJM_LOCATION(17, 17), "-> ROT Y       ");
        njPrintC(NJM_LOCATION(17, 18), "-> EMERALD     ");
        njPrintC(NJM_LOCATION(17, 19), "-> MOVE RANGE");
        njPrintC(NJM_LOCATION(17, 20), "->             ");
        njPrintC(NJM_LOCATION(17, 21), "->1000<sclz NOCOLLI");
    }
    edge.x = work->scl.x + MOVE_RANGE;
    edge.y = 0.0F;
    edge.z = 0.0F;
    for (i = 0; i < ENEMY_RANGE_SEGMENTS; i++) {
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, i * ENEMY_RANGE_TURN);
        njCalcPoint(NULL, &edge, &from);
        njPopMatrix(1);
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, (i + 1) * ENEMY_RANGE_TURN);
        njCalcPoint(NULL, &edge, &to);
        njPopMatrix(1);
        draw_line_v(&from, &to);
    }
    njSetTexture(&uni_c_unibody_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njRotateY(NULL, MODEL_TURN);
    draw_tree(&uni_c_object);
    njPopMatrix(1);
}

static void uni_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    enemy_end(task, work, record);
}

static void uni_die(struct task *task, struct task_work *work,
                    struct lockon_target *record)
{
    enemy_end(task, work, record);
    placement_kill(task);
    task->step = task_destroy_children;
    lockon_release(task);
}

static void uni_burst(struct task_work *work, struct lockon_target *record)
{
    explosion_create_at(work->pos.x, work->pos.y + 5.0F, work->pos.z, 1.4F);
    enemy_scatter_debris(work, DEBRIS_COUNT);
    animal_create_random(0, work->pos.x, work->pos.y + 15.0F, work->pos.z);
}

/* turn the uni toward home while it is past its range, or else toward
 * player 0 while it sees a player. it returns TRUE when it turns toward
 * player 0 and its heading was already within 0x100 of player 0. it
 * returns FALSE otherwise */
static Bool uni_turn(struct task_work *work, struct lockon_target *record)
{
    if (work->flags & UNI_FAR) {
        enemy_turn_home(work, record);
    } else if (work->flags & UNI_SEES) {
        if (enemy_turn_to_player(work, record, 0) < 0x100) {
            return TRUE;
        }
    }
    return FALSE;
}

static void uni_mark_far(struct task_work *work, struct lockon_target *record)
{
    Float distance = enemy_home_distance2(work, record);
    Float range = work->scl.x + MOVE_RANGE;

    if (distance > range * range) {
        work->flags |= UNI_FAR;
    } else {
        work->flags &= ~UNI_FAR;
    }
}

static void uni_mark_sight(struct task_work *work, struct lockon_target *record)
{
    if (enemy_sees_player(work, record)) {
        work->flags |= UNI_SEES;
    } else {
        work->flags &= ~UNI_SEES;
    }
}

static void uni_hover(struct task_work *work, struct lockon_target *record)
{
    Float ground;

    if (work->scl.z < NO_GROUND) {
        ground = shadow_draw(work, 1.2F);
        if (record->last_pos.y - MAX_DROP > ground) {
            work->pos = record->last_pos;
            return;
        }
        if (work->pos.y > ground + HOVER_HEIGHT) {
            work->pos.y -= 0.1F;
        } else {
            work->pos.y += 0.1F;
        }
    }
}

static void uni_a_wait(struct task_work *work, struct lockon_target *record)
{
    uni_take_hit(work, record);
    uni_move(work, record);
    uni_a_draw_body(work, record);
    if (enemy_sees_player(work, record)) {
        ENEMY_SET_MODE(work, record, UNI_A_APPROACH);
    }
}

static void uni_a_approach(struct task_work *work, struct lockon_target *record)
{
    uni_mark_sight(work, record);
    uni_mark_far(work, record);
    if (uni_turn(work, record)) {
        ENEMY_SET_MODE(work, record, UNI_A_THROW);
    }
    uni_take_hit(work, record);
    record->motion.accel.x = njCos(work->ang.y) * 0.02F;
    record->motion.accel.z = njSin(-work->ang.y) * 0.02F;
    uni_move(work, record);
    uni_hover(work, record);
    uni_a_draw_body(work, record);
}

static void uni_a_throw(struct task_work *work, struct lockon_target *record)
{
    /* @hack redundant: work->mode read and dropped, which puts work in R14 and
     * record in R13 (MOV R4,R14 at 0x8c10eb4c, MOV R5,R13 at 0x8c10eb54);
     * without it, the two trade and 19 of 44 words differ */
    (void)work->mode;
    uni_mark_sight(work, record);
    uni_mark_far(work, record);
    uni_turn(work, record);
    uni_take_hit(work, record);
    record->motion.accel.x = njCos(work->ang.y) * 0.02F;
    record->motion.accel.z = njSin(-work->ang.y) * 0.02F;
    uni_move(work, record);
    uni_hover(work, record);
    uni_a_draw_body(work, record);
}

static void uni_a_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct lockon_target *record = LOCKON_TARGET(task);

    /* @hack redundant: player 0's work read through player and dropped, which
     * keeps its load into R4 (0x8c10ebb4); without it, 18 of 104 words
     * differ */
    (void)player->mode;
    if (edit_mode) {
        uni_a_edit_draw(task);
    } else {
        if (placement_try_leave_by(task, UNI_LEAVE_RANGE)) {
            return;
        }
        switch (work->mode) {
        case UNI_WAIT:
            uni_a_wait(work, record);
            break;
        case UNI_A_APPROACH:
            uni_a_approach(work, record);
            break;
        case UNI_A_THROW:
            uni_a_throw(work, record);
            break;
        case UNI_HIT:
            knuckles_take_piece(work->ang.z, &work->pos);
            uni_burst(work, record);
            uni_die(task, work, record);
            return;
        }
        knuckles_find_piece(work->ang.z, &work->pos);
        enemy_save_pos(work, record);
        work->timer++;
        record->angles[UNI_PHASE] += 0x300;
        if (work->flags & UNI_SOLID) {
            collision_register(work);
        }
        task_step_children(task);
    }
}

void uni_a_start(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;
    /* @hack redundant: register on i, which puts work in R11 and
     * task_create_child's address in R10 (MOV.L @(32,R12),R11 at 0x8c10ed3a,
     * MOV.L @(0x8c10eedc),R10 at 0x8c10ee0a); without it, the two trade and
     * 17 of 198 words differ */
    register int i;
    /* @hack redundant: an array never used, whose 12 bytes give the image's
     * ADD #-12,R15 (0x8c10ed30); without it, the function runs 4 bytes
     * short and 192 of 198 words differ */
    Uint32 unused[3];

    collision_init(task, &uni_shape, COLLISION_ENEMY, 1, 1);
    record = enemy_alloc_record(task, work);
    record->flags |= ENEMY_KEEP_ON_LEDGE | ENEMY_SHADOW;
    work->tick = 0;
    work->timer = 0;
    record->motion.slip = 0.0F;
    record->turn_speed = 0x2b0;
    record->sight_distance = 300.0F * 300.0F;
    record->sight_angle = 0x8000;
    record->near_distance = 200.0F * 200.0F;
    record->aim_y = work->ang.y;
    record->frame = 0.0F;
    record->probe_offset.x = 0.0F;
    record->probe_offset.y = 0.0F;
    record->probe_offset.z = 0.0F;
    record->top = 5.0F;
    record->radius = 5.0F;
    record->bottom = -5.0F;
    record->wall_bounce = 1.0F;
    record->floor_bounce = 1.0F;
    record->unk_c4 = 1.0F;
    record->shadow_size = 1.6F;
    record->shadow_ratio = 1.0F;
    record->angles[UNI_PHASE] =
        (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
    record->motion.gravity = 0.1F;
    work->pos.y = shadow_draw(work, 0.0F) + 12.0F;
    enemy_set_home(work, record);
    task->destructor = uni_destroy;
    task->step = uni_a_step;
    task->draw = uni_a_draw;
    work->flags |= UNI_SOLID;
    for (i = 0; i < 4; i++) {
        struct task *child =
            task_create_child(TASK_WORK, uni_a_spike_start, task);

        if (child != NULL) {
            struct task_work *child_work = child->work;

            child_work->submode = i;
            child_work->timer = 0;
        }
    }
    knuckles_find_piece(work->ang.z, &work->pos);
}

static void uni_b_wait(struct task_work *work, struct lockon_target *record)
{
    uni_take_hit(work, record);
    uni_move(work, record);
    uni_a_draw_body(work, record);
    if (enemy_sees_player(work, record)) {
        ENEMY_SET_MODE(work, record, UNI_B_APPROACH);
    }
}

static void uni_b_approach(struct task_work *work, struct lockon_target *record)
{
    /* @hack redundant: work->mode read and dropped, which puts work in R14 and
     * record in R13 (MOV R4,R14 at 0x8c10ef82, MOV R5,R13 at 0x8c10ef8a);
     * without it, the two trade and 19 of 46 words differ */
    (void)work->mode;
    uni_mark_sight(work, record);
    uni_mark_far(work, record);
    uni_turn(work, record);
    uni_take_hit(work, record);
    record->motion.accel.x = njCos(work->ang.y) * 0.02F;
    record->motion.accel.z = njSin(-work->ang.y) * 0.02F;
    uni_move(work, record);
    uni_hover(work, record);
    uni_a_draw_body(work, record);
}

static void uni_b_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct lockon_target *record = LOCKON_TARGET(task);

    /* @hack redundant: player 0's work read through player and dropped, which
     * keeps its load into R4 (0x8c10efee); without it, 46 of 99 words
     * differ */
    (void)player->mode;
    if (edit_mode) {
        uni_a_edit_draw(task);
    } else {
        if (placement_try_leave_by(task, UNI_LEAVE_RANGE)) {
            return;
        }
        switch (work->mode) {
        case UNI_WAIT:
            uni_b_wait(work, record);
            break;
        case UNI_B_APPROACH:
            uni_b_approach(work, record);
            break;
        case UNI_HIT:
            knuckles_take_piece(work->ang.z, &work->pos);
            uni_burst(work, record);
            uni_die(task, work, record);
            return;
        }
        knuckles_find_piece(work->ang.z, &work->pos);
        enemy_save_pos(work, record);
        work->timer++;
        if (work->flags & UNI_SOLID) {
            collision_register(work);
        }
        record->angles[UNI_PHASE] += 0x300;
        task_step_children(task);
    }
}

void uni_b_start(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;
    /* @hack redundant: register on i, which puts work in R12 and
     * task_create_child's address in R11 (MOV.L @(32,R13),R12 at 0x8c10f138,
     * MOV.L @(0x8c10f2dc),R11 at 0x8c10f208); without it, the two trade and
     * 20 of 186 words differ */
    register int i;
    /* @hack redundant: an array never used, whose 12 bytes give the image's
     * ADD #-12,R15 (0x8c10f12e); without it, the function runs 4 bytes
     * short and 180 of 186 words differ */
    Uint32 unused[3];

    collision_init(task, &uni_shape, COLLISION_ENEMY, 1, 1);
    record = enemy_alloc_record(task, work);
    record->flags |= ENEMY_KEEP_ON_LEDGE | ENEMY_SHADOW;
    work->tick = 0;
    work->timer = 0;
    record->motion.slip = 0.0F;
    record->turn_speed = 0x2b0;
    record->sight_distance = 300.0F * 300.0F;
    record->sight_angle = 0x8000;
    record->near_distance = 200.0F * 200.0F;
    record->aim_y = work->ang.y;
    record->frame = 0.0F;
    record->probe_offset.x = 0.0F;
    record->probe_offset.y = 0.0F;
    record->probe_offset.z = 0.0F;
    record->top = 5.0F;
    record->radius = 5.0F;
    record->bottom = -5.0F;
    record->wall_bounce = 1.0F;
    record->floor_bounce = 1.0F;
    record->unk_c4 = 1.0F;
    record->shadow_size = 1.6F;
    record->shadow_ratio = 1.0F;
    record->angles[UNI_PHASE] =
        (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
    record->motion.gravity = 0.1F;
    work->pos.y = shadow_draw(work, 0.0F) + 9.0F;
    enemy_set_home(work, record);
    task->destructor = uni_destroy;
    task->step = uni_b_step;
    task->draw = uni_a_draw;
    work->flags |= UNI_SOLID;
    for (i = 0; i < 4; i++) {
        struct task *child =
            task_create_child(TASK_WORK, uni_b_spike_start, task);

        if (child != NULL) {
            struct task_work *child_work = child->work;

            child_work->submode = i;
        }
    }
    knuckles_find_piece(work->ang.z, &work->pos);
}

static void uni_c_swim(struct task_work *work, struct lockon_target *record)
{
    uni_mark_sight(work, record);
    uni_mark_far(work, record);
    uni_turn(work, record);
    uni_take_hit(work, record);
    record->motion.accel.x = njCos(work->ang.y) * 0.008F;
    record->motion.accel.z = njSin(-work->ang.y) * 0.008F;
    uni_move(work, record);
    uni_hover(work, record);
    uni_c_draw_body(work, record);
    sound_play_at_for(SE_BURNING, (int)work, 1, 0, work->pos.x, work->pos.y,
                      work->pos.z, 1);
    record->angles[UNI_PHASE] += 0x300;
}

static void uni_c_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct lockon_target *record = LOCKON_TARGET(task);

    work->ang.z = work->vars.l[0];
    /* @hack redundant: player 0's work read through player and dropped, which
     * keeps its load into R4 (0x8c10f410); without it, 53 of 87 words
     * differ */
    (void)player->mode;
    if (edit_mode) {
        uni_c_edit_draw(task);
    } else {
        if (placement_try_leave_by(task, UNI_LEAVE_RANGE)) {
            return;
        }
        switch (work->mode) {
        case UNI_WAIT:
            uni_c_swim(work, record);
            break;
        case UNI_HIT:
            knuckles_take_piece(work->ang.z, &work->pos);
            uni_burst(work, record);
            uni_die(task, work, record);
            return;
        }
        knuckles_find_piece(work->ang.z, &work->pos);
        enemy_save_pos(work, record);
        work->timer++;
        if (work->flags & UNI_SOLID) {
            collision_register(work);
        }
        task_step_children(task);
    }
}

void uni_c_start(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;
    int i;
    /* @hack redundant: an array never used, whose 12 bytes give the image's
     * ADD #-12,R15 (0x8c10f50e); without it, the function runs 2 bytes
     * short and 176 of 194 words differ */
    Uint32 unused[3];

    /* @hack redundant: work->mode read and dropped, which puts task in R12 and
     * work in R13 (MOV R4,R12 at 0x8c10f510, MOV.L @(32,R12),R13 at
     * 0x8c10f512); without it, the two trade and 25 of 194 words differ */
    (void)work->mode;
    /* a copy of ang.z, which holds the piece. uni_c_step restores ang.z
     * from it at each step, which undoes the set editor's writes to ang.z
     * from its cursor. no other writer of the uni's ang.z is known */
    work->vars.l[0] = work->ang.z;
    collision_init(task, &uni_shape, COLLISION_ENEMY, 1, 1);
    record = enemy_alloc_record(task, work);
    record->flags |= ENEMY_KEEP_ON_LEDGE | ENEMY_SHADOW;
    work->tick = 0;
    work->timer = 0;
    record->motion.slip = 0.0F;
    record->turn_speed = 0x100;
    record->sight_distance = 400.0F * 400.0F;
    record->sight_angle = 0x8000;
    record->near_distance = 280.0F * 280.0F;
    record->aim_y = work->ang.y;
    record->frame = 0.0F;
    record->probe_offset.x = 0.0F;
    record->probe_offset.y = 0.0F;
    record->probe_offset.z = 0.0F;
    record->top = 5.0F;
    record->radius = 5.0F;
    record->bottom = -5.0F;
    record->wall_bounce = 1.0F;
    record->floor_bounce = 1.0F;
    record->unk_c4 = 1.0F;
    record->shadow_size = 1.6F;
    record->shadow_ratio = 1.0F;
    record->angles[UNI_PHASE] =
        (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
    record->motion.gravity = 0.1F;
    if (work->scl.z < NO_GROUND) {
        work->pos.y = shadow_draw(work, 0.0F) + HOVER_HEIGHT;
    }
    enemy_set_home(work, record);
    task->destructor = uni_destroy;
    task->step = uni_c_step;
    task->draw = uni_c_draw;
    work->flags |= UNI_SOLID;
    for (i = 0; i < 4; i++) {
        struct task *child =
            task_create_child(TASK_WORK, uni_c_fireball_start, task);

        if (child != NULL) {
            struct task_work *child_work = child->work;

            child_work->submode = i;
        }
    }
    knuckles_find_piece(work->ang.z, &work->pos);
}
