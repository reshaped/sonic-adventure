/* the spina, a flier with a spinning blade on top, in three kinds. SPINA A
 * flies about where it was placed, and turns toward player 0 when it sees
 * a player. SPINA B and SPINA C swing to and fro, and SPINA C throws
 * sparks now and then */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "animal/kinds.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/explosion.h"
#include "enemy/enemy.h"
#include "enemy/spina.h"
#include "hud/hud.h"
#include "object/placement.h"
#include "player/knuckles.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 13
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 8
#define TEXT_10_LENGTH 8
#define TEXT_11_LENGTH 10
#define TEXT_12_LENGTH 8
#define TEXT_13_LENGTH 8
#define TEXT_14_LENGTH 7
#include "enemy/spina/text.inc"

enum spina_mode {
    SPINA_INIT = 0,
    SPINA_FLY = 1,
    SPINA_SPRING = 2,
    SPINA_EXPLODE = 4,
    SPINA_GONE = 5
};

/* SPINA C's submode as it flies */
enum spina_sparks { SPINA_SPARKING, SPINA_RESTING };

/* the phases the spina keeps in record->angles, for its tilts, its bob and
 * its hover */
enum spina_angle {
    SPINA_TILT_Z = 0,
    SPINA_TILT_X = 1,
    SPINA_BOB = 2,
    SPINA_HOVER = 5
};

#define SPINA_FAR 0x100 /* in flags: past its range */
#define SPINA_SEES 0x200 /* in flags: it sees a player */

/* a spina's range from home is scl.x plus this. past it, spina_turn turns
 * the spina toward home. SPINA A's set editor names scl.x SEARCH RANGE */
#define MOVE_RANGE 60.0F

#define SE_BLADE 813
#define SE_SPARKS 814
/* the squared distances from player 0 within which each sound plays */
#define BLADE_RANGE (240.0F * 240.0F)
#define SPARKS_RANGE (120.0F * 120.0F)

#define BLADE_TILT NJM_DEG_ANG(8)

/* the body's tilt at phase, up to 8 degrees either way */
#define TILT(phase) ((Angle)(njSin(phase) * (8.0F * ANGLE_PER_DEGREE)))

#define PIECE(record) ((record)->unk_4a) /* its master emerald piece */
#define HEIGHT(record) ((record)->unk_78) /* SPINA A hovers about it */
#define SWING(record) ((record)->heading) /* SPINA C's swing phase */

/* the sparks' shape in spina_shapes, which starts with SHAPE_OFF. SPINA C
 * clears SHAPE_OFF only as a rest ends, so the shape stays off through
 * SPINA C's first sparking */
#define SPARKS_SHAPE 2

static void spina_draw(struct task *task);

static NJS_TEXNAME supi_supi_textures[14] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0},
};
NJS_TEXLIST supi_supi_texlist = {
#include "enemy/spina/supi_supi_texlist.inc"
};

static NJS_MATERIAL spina_blade_materials[3] = {
#include "enemy/spina/spina_blade_materials.inc"
};

static Sint16 spina_blade_meshes0[28] = {
#include "enemy/spina/spina_blade_meshes0.inc"
};

static Sint16 spina_blade_meshes1[20] = {
#include "enemy/spina/spina_blade_meshes1.inc"
};

static Sint16 spina_blade_meshes2[27] = {
#include "enemy/spina/spina_blade_meshes2.inc"
};

static NJS_COLOR spina_blade_uvs0[24] = {
#include "enemy/spina/spina_blade_uvs0.inc"
};

static NJS_COLOR spina_blade_uvs1[16] = {
#include "enemy/spina/spina_blade_uvs1.inc"
};

static NJS_COLOR spina_blade_uvs2[22] = {
#include "enemy/spina/spina_blade_uvs2.inc"
};

static NJS_MESHSET spina_blade_meshsets[3] = {
#include "enemy/spina/spina_blade_meshsets.inc"
};

static NJS_POINT3 spina_blade_points[32] = {
#include "enemy/spina/spina_blade_points.inc"
};

static NJS_VECTOR spina_blade_normals[32] = {
#include "enemy/spina/spina_blade_normals.inc"
};

static NJS_MODEL spina_blade_model = {
#include "enemy/spina/spina_blade_model.inc"
};

static NJS_OBJECT spina_blade_object = {
#include "enemy/spina/spina_blade_object.inc"
};

static NJS_MATERIAL spina_body_materials[12] = {
#include "enemy/spina/spina_body_materials.inc"
};

static Sint16 spina_body_meshes0[16] = {
#include "enemy/spina/spina_body_meshes0.inc"
};

static Sint16 spina_body_meshes1[8] = {
#include "enemy/spina/spina_body_meshes1.inc"
};

static Sint16 spina_body_meshes2[24] = {
#include "enemy/spina/spina_body_meshes2.inc"
};

static Sint16 spina_body_meshes3[21] = {
#include "enemy/spina/spina_body_meshes3.inc"
};

static Sint16 spina_body_meshes4[21] = {
#include "enemy/spina/spina_body_meshes4.inc"
};

static Sint16 spina_body_meshes5[30] = {
#include "enemy/spina/spina_body_meshes5.inc"
};

static Sint16 spina_body_meshes6[30] = {
#include "enemy/spina/spina_body_meshes6.inc"
};

static Sint16 spina_body_meshes7[5] = {
#include "enemy/spina/spina_body_meshes7.inc"
};

static Sint16 spina_body_meshes8[54] = {
#include "enemy/spina/spina_body_meshes8.inc"
};

static Sint16 spina_body_meshes9[12] = {
#include "enemy/spina/spina_body_meshes9.inc"
};

static Sint16 spina_body_meshes10[30] = {
#include "enemy/spina/spina_body_meshes10.inc"
};

static Sint16 spina_body_meshes11[39] = {
#include "enemy/spina/spina_body_meshes11.inc"
};

static NJS_COLOR spina_body_uvs0[12] = {
#include "enemy/spina/spina_body_uvs0.inc"
};

static NJS_COLOR spina_body_uvs1[6] = {
#include "enemy/spina/spina_body_uvs1.inc"
};

static NJS_COLOR spina_body_uvs2[20] = {
#include "enemy/spina/spina_body_uvs2.inc"
};

static NJS_COLOR spina_body_uvs3[18] = {
#include "enemy/spina/spina_body_uvs3.inc"
};

static NJS_COLOR spina_body_uvs4[18] = {
#include "enemy/spina/spina_body_uvs4.inc"
};

static NJS_COLOR spina_body_uvs5[26] = {
#include "enemy/spina/spina_body_uvs5.inc"
};

static NJS_COLOR spina_body_uvs6[26] = {
#include "enemy/spina/spina_body_uvs6.inc"
};

static NJS_COLOR spina_body_uvs7[4] = {
#include "enemy/spina/spina_body_uvs7.inc"
};

static NJS_COLOR spina_body_uvs8[48] = {
#include "enemy/spina/spina_body_uvs8.inc"
};

static NJS_COLOR spina_body_uvs9[10] = {
#include "enemy/spina/spina_body_uvs9.inc"
};

static NJS_COLOR spina_body_uvs10[24] = {
#include "enemy/spina/spina_body_uvs10.inc"
};

static NJS_COLOR spina_body_uvs11[34] = {
#include "enemy/spina/spina_body_uvs11.inc"
};

static NJS_MESHSET spina_body_meshsets[12] = {
#include "enemy/spina/spina_body_meshsets.inc"
};

static NJS_POINT3 spina_body_points[110] = {
#include "enemy/spina/spina_body_points.inc"
};

static NJS_VECTOR spina_body_normals[110] = {
#include "enemy/spina/spina_body_normals.inc"
};

static NJS_MODEL spina_body_model = {
#include "enemy/spina/spina_body_model.inc"
};

static NJS_OBJECT spina_object = {
#include "enemy/spina/spina_object.inc"
};

/* clang-format off */
static struct collision_shape spina_shapes[3] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_UNTURNED,
     ATTACK(1) | SHAPE_HURTS_PLAYERS, 0, {0.0F, 5.5F, 0.0F}, 11.2F, 0.7F, 0.0F,
     {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_UNTURNED,
     ATTACK(1) | SHAPE_HURTS_PLAYERS, PUSH_ALL, {0.0F, 7.5F, 0.0F}, 6.0F, 6.0F,
     0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_OFF | SHAPE_HURTABLE | SHAPE_UNTURNED,
     ATTACK(2) | GUARD(2) | SHAPE_HURTS_PLAYERS, 0, {0.0F, 5.0F, 0.0F}, 20.0F,
     0.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static Float dat_8c5dd404[15] = {0.45F, 0.3F, 3.5F, 0.0F, 0.0F,
                                 0.0F,  0.0F, 0.0F, 0.0F, 0.5F,
                                 0.0F,  1.5F, 0.3F, 0.3F, 1.0F};

static void spina_init(struct task *task, struct task_work *work)
{
    int time = play_time_get();
    struct lockon_target *record;

    collision_init(task, spina_shapes, COLLISION_ITEM, 1, 3);
    record = enemy_alloc_record(task, work);
    work->submode = 0;
    work->timer = 0;
    PIECE(record) = work->ang.z;
    work->ang.z = (work->ang.z & ~0xff) >> 8;
    record->sight_distance = 80.0F * 80.0F;
    record->sight_angle = NJM_DEG_ANG(160);
    record->near_distance = 20.0F * 20.0F;
    record->motion.speed.x = 0.4F;
    record->turn_speed = 0x180;
    record->unk_80 = 1.0F;
    record->unk_84 = 0.0F;
    record->phase = (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
    SWING(record) = (time * work->ang.z) + work->ang.x;
    HEIGHT(record) = work->pos.y;
    enemy_set_home(work, record);
    work->mode = SPINA_FLY;
    knuckles_find_piece(PIECE(record), &work->pos);
    task->draw = spina_draw;
}

static void spina_turn(struct task_work *work, struct lockon_target *record)
{
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    int turned;

    if (work->flags & SPINA_FAR) {
        enemy_turn_home(work, record);
    } else if (work->flags & SPINA_SEES) {
        /* @hack redundant: turned set and never read, which keeps
         * enemy_turn_to_player a JSR (0x8c10f888); without it, the call
         * becomes a tail jump, spina_turn runs 18 bytes long and 17 of 17
         * words differ */
        turned = enemy_turn_to_player(work, record, 0);
    }
}

static void spina_mark_far(struct task_work *work, struct lockon_target *record)
{
    Float distance = enemy_home_distance2(work, record);
    Float range = work->scl.x + MOVE_RANGE;

    if (distance > range * range) {
        work->flags |= SPINA_FAR;
    } else {
        work->flags &= ~SPINA_FAR;
    }
}

static void spina_explode(struct task_work *work)
{
    explosion_create_at(work->pos.x, work->pos.y + 5.0F, work->pos.z, 1.4F);
    enemy_scatter_debris(work, DEBRIS_COUNT);
    work->mode = SPINA_GONE;
}

static void spina_take_hit(struct task_work *work, struct lockon_target *record)
{
    /* @hack redundant: player_work handed to enemy_ignore, which keeps the
     * load of its address into R4 (0x8c10f94a); without it, spina_take_hit
     * runs 4 bytes short and 30 of 33 words differ */
    enemy_ignore((void *)player_work);
    if (enemy_take_hit(work, record)) {
        ENEMY_SET_MODE(work, record, SPINA_EXPLODE);
        record->unk_84 = 0.5F;
        enemy_set_home(work, record);
    }
}

static void spina_mark_sight(struct task_work *work,
                             struct lockon_target *record)
{
    if (enemy_sees_player(work, record)) {
        work->flags |= SPINA_SEES;
    } else {
        work->flags &= ~SPINA_SEES;
    }
}

static void spina_a_move(struct task_work *work, struct lockon_target *record)
{
    Float height = player_work[0]->pos.y + 6.0F;

    record->motion.speed.x += record->motion.accel.x;
    record->motion.speed.z += record->motion.accel.z;
    if (record->motion.speed.x > 0.4F) {
        record->motion.speed.x = 0.4F;
    } else if (record->motion.speed.x < 0.1F) {
        record->motion.speed.x = 0.1F;
    }
    if (height > HEIGHT(record)) {
        HEIGHT(record) += 0.2F;
    } else {
        HEIGHT(record) -= 0.2F;
    }
    if (HEIGHT(record) > record->home.y + 20.0F) {
        HEIGHT(record) = record->home.y + 20.0F;
    } else if (HEIGHT(record) < record->home.y - 20.0F) {
        HEIGHT(record) = record->home.y - 20.0F;
    }
    work->pos.x += (record->motion.speed.x * njCos(work->ang.y)) +
                   (record->motion.speed.z * njSin(work->ang.y));
    work->pos.y = HEIGHT(record) + (njSin(record->angles[SPINA_HOVER]) * 1.6F);
    work->pos.z += -(record->motion.speed.x * njSin(work->ang.y)) +
                   (record->motion.speed.z * njCos(work->ang.y));
    record->angles[SPINA_HOVER] += 0x280;
}

/* SPINA B's swing follows the play time, so every SPINA B of one speed and
 * phase swings in step. this advances SWING too, which only SPINA C
 * reads */
static void spina_b_swing(struct task_work *work, struct lockon_target *record)
{
    Float swing = njSin((play_time_get() * work->ang.z) + work->ang.x);

    work->pos.x = record->home.x + (swing * work->scl.x);
    work->pos.y = record->home.y + (swing * work->scl.y);
    work->pos.z = record->home.z + (swing * work->scl.z);
    SWING(record) += work->ang.z;
}

static void spina_c_swing(struct task_work *work, struct lockon_target *record)
{
    Float swing = njSin(SWING(record));

    work->pos.x = record->home.x + (swing * work->scl.x);
    work->pos.y = record->home.y + (swing * work->scl.y);
    work->pos.z = record->home.z + (swing * work->scl.z);
    SWING(record) += work->ang.z;
}

/* draw the spina and advance its tilts, bob and blade. it does both only
 * when draw_in_view passes, in view on a frame's first step */
static void spina_draw_spinning(struct task_work *work,
                                struct lockon_target *record)
{
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    njSetTexture(&supi_supi_texlist);
    record->angles[SPINA_TILT_Z] += 0x340;
    record->angles[SPINA_TILT_X] += 0x240;
    record->angles[SPINA_BOB] += 0x300;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    /* the model faces a quarter turn from its heading */
    TURN(njRotateY, work->ang.y + 0x4000);
    /* the body bobs, and tilts about a point 5 up */
    njTranslate(NULL, 0.0F, (njSin(record->angles[SPINA_BOB]) * 2.0F) + 5.0F,
                0.0F);
    TURN(njRotateZ, TILT(record->angles[SPINA_TILT_Z]));
    TURN(njRotateX, TILT(record->angles[SPINA_TILT_X]));
    njTranslate(NULL, 0.0F, -5.0F, 0.0F);
    njScale(NULL, 1.5F, 1.5F, 1.5F);
    draw_model(&spina_body_model);
    njTranslate(NULL, 0.0F, 5.0F, 0.0F);
    njRotateX(NULL, BLADE_TILT);
    TURN(njRotateY, record->phase);
    draw_model(&spina_blade_model);
    njPopMatrix(1);
    record->phase += 0x3000;
}

/* the spina as it stands, which task_draw_all draws while the game is
 * paused. it advances nothing */
static void spina_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    njSetTexture(&supi_supi_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y + 0x4000);
    njTranslate(NULL, 0.0F, (njSin(record->angles[SPINA_BOB]) * 2.0F) + 5.0F,
                0.0F);
    TURN(njRotateZ, TILT(record->angles[SPINA_TILT_Z]));
    TURN(njRotateX, TILT(record->angles[SPINA_TILT_X]));
    njTranslate(NULL, 0.0F, -5.0F, 0.0F);
    njScale(NULL, 1.5F, 1.5F, 1.5F);
    draw_model(&spina_body_model);
    njTranslate(NULL, 0.0F, 5.0F, 0.0F);
    njRotateX(NULL, BLADE_TILT);
    TURN(njRotateY, record->phase);
    draw_model(&spina_blade_model);
    njPopMatrix(1);
}

void spina_keep_low(struct task_work *work)
{
    Float ground = shadow_draw(work, 1.2F);

    if (work->pos.y > ground + 3.0F) {
        work->pos.y = ground + 3.0F;
    }
}

static void spina_edit_draw_body(struct task_work *work, Float x, Float y,
                                 Float z)
{
    njSetTexture(&supi_supi_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, x, y, z);
    TURN(njRotateY, work->ang.y);
    draw_tree(&spina_object);
    njPopMatrix(1);
}

static void spina_a_edit_draw(struct task *task, struct task_work *work)
{
    if (placement_editing(task)) {
        if (work->tick > 25) {
            njPrintC(NJM_LOCATION(12, 8), "<< SPINNA A >>");
        } else {
            njPrintC(NJM_LOCATION(12, 8), "<<          >>");
        }
        if (work->tick++ > 50) {
            work->tick = 0;
        }
        njPrintC(NJM_LOCATION(17, 16), "->              ");
        njPrintC(NJM_LOCATION(17, 17), "->  OBJECT ANG  ");
        njPrintC(NJM_LOCATION(17, 18), "->  EMERALD     ");
        njPrintC(NJM_LOCATION(17, 19), "->  SEARCH RANGE");
        njPrintC(NJM_LOCATION(17, 20), "->              ");
        njPrintC(NJM_LOCATION(17, 21), "->              ");
    }
    spina_edit_draw_body(work, work->pos.x, work->pos.y, work->pos.z);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    draw_set_material(0.2F, 0.0F, 1.0F, 0.0F);
    /* marker_cylinder_object is 20 across, so this flattens it into a disc
     * of radius scl.x + 70 */
    njScale(NULL, (work->scl.x + 70.0F) * 0.1F, 0.02F,
            (work->scl.x + 70.0F) * 0.1F);
    draw_tree(&marker_cylinder_object);
    njPopMatrix(1);
}

static void spina_b_edit_draw(struct task *task, struct task_work *work)
{
    int time = play_time_get();
    Float x = work->pos.x;
    Float y = work->pos.y;
    Float z = work->pos.z;
    Float swing;

    if (placement_editing(task)) {
        if (work->tick > 25) {
            njPrintC(NJM_LOCATION(4, 8), "< SPINNA B (BIRIBIRI NASHI) >");
        } else {
            njPrintC(NJM_LOCATION(4, 8), "<                           >");
        }
        if (work->tick++ > 50) {
            work->tick = 0;
        }
        njPrintC(NJM_LOCATION(17, 16), "-> PHASE       ");
        njPrintC(NJM_LOCATION(17, 17), "-> OBJECT ANG  ");
        njPrintC(NJM_LOCATION(17, 18), "-> SPEED&EME   ");
        njPrintC(NJM_LOCATION(17, 19), "-> X WIDTH     ");
        njPrintC(NJM_LOCATION(17, 20), "-> Y WIDTH     ");
        njPrintC(NJM_LOCATION(17, 21), "-> Z WIDTH     ");
    }
    spina_edit_draw_body(work, x + (work->scl.x * njSin(work->ang.x)),
                         y + (work->scl.y * njSin(work->ang.x)),
                         z + (work->scl.z * njSin(work->ang.x)));
    if (TASK_TIMER(work)++ > 2) {
        work->timer = 0;
    } else {
        swing = njSin((time * work->ang.z) + work->ang.x);
        spina_edit_draw_body(work, x + (swing * work->scl.x),
                             y + (swing * work->scl.y),
                             z + (swing * work->scl.z));
    }
    draw_line(x - work->scl.x, y - work->scl.y, z - work->scl.z,
              x + work->scl.x, y + work->scl.y, z + work->scl.z);
}

static void spina_c_edit_draw(struct task *task, struct task_work *work)
{
    int time = play_time_get();
    Float x = work->pos.x;
    Float y = work->pos.y;
    Float z = work->pos.z;
    Float swing;

    if (placement_editing(task)) {
        if (work->tick > 4) {
            njPrintC(NJM_LOCATION(4, 8), "< SPINNA C (BIRIBIRI ARI) >");
        } else {
            njPrintC(NJM_LOCATION(4, 8), "<                        >");
        }
        if (work->tick++ > 8) {
            work->tick = 0;
        }
        njPrintC(NJM_LOCATION(17, 16), "-> PHASE       ");
        njPrintC(NJM_LOCATION(17, 17), "-> OBJECT ANG  ");
        njPrintC(NJM_LOCATION(17, 18), "-> SPEED&EME   ");
        njPrintC(NJM_LOCATION(17, 19), "-> X WIDTH     ");
        njPrintC(NJM_LOCATION(17, 20), "-> Y WIDTH     ");
        njPrintC(NJM_LOCATION(17, 21), "-> Z WIDTH     ");
    }
    spina_edit_draw_body(work, x + (work->scl.x * njSin(work->ang.x)),
                         y + (work->scl.y * njSin(work->ang.x)),
                         z + (work->scl.z * njSin(work->ang.x)));
    if (TASK_TIMER(work)++ > 2) {
        work->timer = 0;
    } else {
        swing = njSin((time * work->ang.z) + work->ang.x);
        spina_edit_draw_body(work, x + (swing * work->scl.x),
                             y + (swing * work->scl.y),
                             z + (swing * work->scl.z));
    }
    draw_line(x - work->scl.x, y - work->scl.y, z - work->scl.z,
              x + work->scl.x, y + work->scl.y, z + work->scl.z);
}

static void spina_die(struct task *task, struct task_work *work,
                      struct lockon_target *record)
{
    animal_create_random(5, work->pos.x, work->pos.y, work->pos.z);
    enemy_end(task, work, record);
    placement_kill(task);
}

static void spina_a_fly(struct task_work *work, struct lockon_target *record)
{
    spina_mark_sight(work, record);
    spina_mark_far(work, record);
    spina_turn(work, record);
    spina_a_move(work, record);
    spina_take_hit(work, record);
    spina_draw_spinning(work, record);
    if (enemy_player_distance2(work, 0) < BLADE_RANGE) {
        sound_play_at_for(SE_BLADE, (int)work, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 1);
    }
}

static void spina_b_fly(struct task_work *work, struct lockon_target *record)
{
    spina_mark_sight(work, record);
    spina_mark_far(work, record);
    spina_turn(work, record);
    spina_b_swing(work, record);
    spina_take_hit(work, record);
    spina_draw_spinning(work, record);
    if (enemy_player_distance2(work, 0) < BLADE_RANGE) {
        sound_play_at_for(SE_BLADE, (int)work, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 1);
    }
}

/* SPINA C sparks and rests by turns, timed by work->timer, which
 * spina_c_step advances each step */
static void spina_c_fly(struct task_work *work, struct lockon_target *record)
{
    struct collision_work *collision = work->collision;
    NJS_VECTOR segment;
    NJS_POINT3 from;
    NJS_POINT3 to;
    Angle pitch;
    Angle yaw;
    Float length;
    int i;

    spina_mark_sight(work, record);
    spina_mark_far(work, record);
    spina_turn(work, record);
    spina_c_swing(work, record);
    spina_take_hit(work, record);
    spina_draw_spinning(work, record);
    if (enemy_player_distance2(work, 0) < BLADE_RANGE) {
        sound_play_at_for(SE_BLADE, (int)work, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 1);
    }
    switch (work->submode) {
    case SPINA_SPARKING:
        if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 30.0F)) {
            for (i = 0; i < 16; i++) {
                pitch = (Angle)((0.5F - (rand() * RAND_SCALE)) * 180.0F *
                                ANGLE_PER_DEGREE);
                yaw = (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
                length = ((rand() * RAND_SCALE) + 0.5F) * 10.0F;
                segment.x = length * njCos(pitch) * njCos(yaw);
                segment.y = length * njSin(pitch);
                segment.z = length * njCos(pitch) * njSin(yaw);
                to = work->pos;
                from = work->pos;
                njAddVector(&to, &segment);
                from.y += 5.0F;
                to.y += 5.0F;
                draw_line_v(&from, &to);
                from = to;

                pitch += (Angle)((0.5F - (rand() * RAND_SCALE)) * 120.0F *
                                 ANGLE_PER_DEGREE);
                yaw += (Angle)((0.5F - (rand() * RAND_SCALE)) * 80.0F *
                               ANGLE_PER_DEGREE);
                length = ((rand() * RAND_SCALE) + 1.0F) * 4.0F;
                segment.x = length * njCos(pitch) * njCos(yaw);
                segment.y = length * njSin(pitch);
                segment.z = length * njCos(pitch) * njSin(yaw);
                to = from;
                njAddVector(&to, &segment);
                draw_line_v(&from, &to);
                from = to;

                pitch += (Angle)((0.5F - (rand() * RAND_SCALE)) * 120.0F *
                                 ANGLE_PER_DEGREE);
                yaw += (Angle)((0.5F - (rand() * RAND_SCALE)) * 80.0F *
                               ANGLE_PER_DEGREE);
                length = ((rand() * RAND_SCALE) + 1.0F) * 2.0F;
                segment.x = length * njCos(pitch) * njCos(yaw);
                segment.y = length * njSin(pitch);
                segment.z = length * njCos(pitch) * njSin(yaw);
                to = from;
                njAddVector(&to, &segment);
                draw_line_v(&from, &to);
            }
        }
        if (enemy_player_distance2(work, 0) < SPARKS_RANGE) {
            sound_play_at_for(SE_SPARKS, (int)work, 1, 0, work->pos.x,
                              work->pos.y, work->pos.z, 1);
        }
        if (TASK_TIMER(work) > 120) {
            work->timer = 0;
            collision->shapes[SPARKS_SHAPE].flags |= SHAPE_OFF;
            work->submode = SPINA_RESTING;
        }
        break;
    case SPINA_RESTING:
        if (TASK_TIMER(work) > 180) {
            work->timer = 0;
            collision->shapes[SPARKS_SHAPE].flags &= ~SHAPE_OFF;
            work->submode = SPINA_SPARKING;
        }
        break;
    }
}

static void spina_spring(struct task_work *work, struct lockon_target *record)
{
    NJS_VECTOR pull;

    pull.x = record->home.x - work->pos.x;
    pull.y = record->home.y - work->pos.y;
    pull.z = record->home.z - work->pos.z;
    pull.x *= 0.06F;
    pull.y *= 0.1F;
    pull.z *= 0.06F;
    record->motion.speed.x += pull.x;
    record->motion.speed.y += pull.y;
    record->motion.speed.z += pull.z;
    work->pos.x += record->motion.speed.x;
    work->pos.y += record->motion.speed.y;
    work->pos.z += record->motion.speed.z;
    if (TASK_TIMER(work)++ > 25) {
        work->mode = SPINA_EXPLODE;
    }
}

static void spina_leave(struct task *task, struct task_work *work,
                        struct lockon_target *record)
{
    enemy_end(task, work, record);
    /* @hack redundant: a return at the end, which in spina_a_step loads
     * collision_register's address into R2 (0x8c11142e); without it, R3,
     * and summed over spina_a_step, spina_b_step and spina_c_step, which
     * expand it, 2 of 326 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

void spina_a_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct lockon_target *record = LOCKON_TARGET(task);

    /* @hack redundant: player 0's work read through player and dropped, which
     * keeps its load into R4 (0x8c111326); without it, spina_a_step runs 32
     * bytes short and 105 of 107 words differ */
    (void)player->mode;
    if (edit_mode) {
        spina_a_edit_draw(task, work);
    } else {
        switch (work->mode) {
        case SPINA_INIT:
            spina_init(task, work);
            return;
        case SPINA_FLY:
            spina_a_fly(work, record);
            break;
        case SPINA_SPRING:
            spina_spring(work, record);
            spina_draw_spinning(work, record);
            break;
            /* @hack redundant: a second break, which loads spina_explode's
             * address into R1 (0x8c111384); without it, R3, and 2 of 107
             * words differ */
            break;
        case SPINA_EXPLODE:
            spina_explode(work);
            /* fall through */
        case SPINA_GONE:
            knuckles_take_piece(PIECE(record), &work->pos);
            spina_die(task, work, record);
            return;
        }
        knuckles_find_piece(PIECE(record), &work->pos);
        enemy_save_pos(work, record);
        if (placement_try_leave(task)) {
            spina_leave(task, work, record);
        } else {
            collision_register(work);
        }
    }
}

void spina_b_step(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    if (edit_mode) {
        spina_b_edit_draw(task, work);
    } else {
        switch (work->mode) {
        case SPINA_INIT:
            spina_init(task, work);
            return;
        case SPINA_FLY:
            spina_b_fly(work, record);
            break;
        case SPINA_SPRING:
            spina_spring(work, record);
            spina_draw_spinning(work, record);
            break;
            /* @hack redundant: a second break, which loads spina_explode's
             * address into R1 (0x8c1114b0); without it, R3, and 2 of 109
             * words differ */
            break;
        case SPINA_EXPLODE:
            spina_explode(work);
            /* fall through */
        case SPINA_GONE:
            knuckles_take_piece(PIECE(record), &work->pos);
            spina_die(task, work, record);
            return;
        }
        knuckles_find_piece(PIECE(record), &work->pos);
        enemy_save_pos(work, record);
        if (placement_try_leave(task)) {
            spina_leave(task, work, record);
        } else if (work->mode == SPINA_FLY) {
            collision_register(work);
        }
    }
}

void spina_c_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct lockon_target *record = LOCKON_TARGET(task);

    /* @hack redundant: player 0's work read through player and dropped, which
     * keeps its load into R4 (0x8c11157c); without it, 73 of 110 words
     * differ */
    (void)player->mode;
    if (edit_mode) {
        spina_c_edit_draw(task, work);
    } else {
        switch (work->mode) {
        case SPINA_INIT:
            spina_init(task, work);
            return;
        case SPINA_FLY:
            spina_c_fly(work, record);
            break;
        case SPINA_SPRING:
            spina_spring(work, record);
            spina_draw_spinning(work, record);
            break;
            /* @hack redundant: a second break, which loads spina_explode's
             * address into R1 (0x8c1115da); without it, R3, and 2 of 110
             * words differ */
            break;
        case SPINA_EXPLODE:
            spina_explode(work);
            /* fall through */
        case SPINA_GONE:
            knuckles_take_piece(PIECE(record), &work->pos);
            spina_die(task, work, record);
            return;
        }
        knuckles_find_piece(PIECE(record), &work->pos);
        enemy_save_pos(work, record);
        if (placement_try_leave(task)) {
            spina_leave(task, work, record);
        } else if (work->mode == SPINA_FLY) {
            collision_register(work);
        }
        work->timer++;
    }
}
