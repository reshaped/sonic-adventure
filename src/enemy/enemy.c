/* what the enemies share: their records, collision with the world,
 * turning, breaking apart and being hit */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/polygon.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "effect/particle.h"
#include "enemy/enemy.h"
#include "hud/hud.h"
#include "main.h"
#include "object/item_box_air.h"
#include "object/models.h"
#include "player/lockon.h"
#include "player/player.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

extern NJS_MODEL enemy_debris_model;

/* the vars of a piece of a broken enemy, or of a bit of debris */
struct enemy_piece_vars {
    NJS_MODEL *model;
    Angle spin; /* a piece adds it to ang.x and to ang.y each step */
    /* the list a piece draws with. debris draws with the regular slot's list */
    NJS_TEXLIST *texlist;
};

#define VARS(work) ((struct enemy_piece_vars *)&(work)->vars)
#define SPIN_Z(work) ((work)->flags)
#define SPEED(work) ((work)->scl)

enum enemy_piece_mode { PIECE_START, PIECE_FALL, PIECE_END };

/* a piece or a bit of debris goes once its timer reaches PIECE_TIMER_END.
 * a piece's timer counts 2 a step, and debris's 1 */
#define PIECE_TIMER_END 120

/* the points enemy_target_near_player picks from, before it turns them by
 * the camera's heading */
static NJS_POINT3 enemy_player_spots[4] = {
    {40.0F, 0.0F, -20.0F},
    {20.0F, 0.0F, -40.0F},
    {-20.0F, 0.0F, -40.0F},
    {-40.0F, 0.0F, -20.0F},
};

struct lockon_target *enemy_alloc_record(struct task *task,
                                         struct task_work *work)
{
    struct lockon_target *record;

    record = heap_calloc(1, sizeof(*record));
    if (record == NULL) {
        task_kill(task);
        return NULL;
    }
    task->motion = &record->motion;
    record->home = work->pos;
    record->radius = 15.0F;
    record->probe_offset.y = 5.0F;
    record->top = 10.0F;
    record->bottom = -1.0F;
    record->step_down = 50.0F;
    record->floor_bounce = 0.9F;
    record->friction = 0.8F;
    record->unk_c4 = 0.1F;
    record->wall_bounce = 0.7F;
    record->wall_turn = 0x400;
    record->sight_angle = NJM_DEG_ANG(80);
    record->sight_distance = 150.0F * 150.0F;
    record->near_distance = 35.0F * 35.0F;
    record->turn_speed = 0x100;
    record->flags |= ENEMY_NO_LEDGE | ENEMY_UNK_400;
    record->flags &= ~(LOCKON_HIT | ENEMY_WATER);
    return record;
}

void enemy_end(struct task *task, struct task_work *work,
               struct lockon_target *record)
{
}

Sint8 enemy_sees_player(struct task_work *work, struct lockon_target *record)
{
    Sint8 i;

    for (i = 0; i < 2; i++) {
        struct task_work *player = player_work[i];

        if (player != NULL) {
            Float distance2 = SQUARE(player->pos.x - work->pos.x) +
                              SQUARE(player->pos.y - work->pos.y) +
                              SQUARE(player->pos.z - work->pos.z);

            if (distance2 < record->sight_distance) {
                Angle heading = work->ang.y;
                Angle aim = (Angle)(atan2f(player->pos.x - work->pos.x,
                                           player->pos.z - work->pos.z) *
                                    ANGLE_PER_RADIAN) -
                            NJM_DEG_ANG(90);
                int away = (Uint16)angle_distance(aim, heading);

                if (away < record->sight_angle ||
                    distance2 < record->near_distance) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

static void enemy_place_probe(struct task_work *work,
                              struct lockon_target *record)
{
    struct sides_query *ground = &record->ground;
    /* @hack redundant: a zero vector the image copies into the frame from
     * 0x8c12a0b8 and never reads (0x8c049fea to 0x8c049ffa); without it, 105
     * of 119 words differ */
    NJS_VECTOR unused = {0.0F, 0.0F, 0.0F};
    NJS_VECTOR turned;
    NJS_VECTOR offset;

    offset = record->probe_offset;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njCalcPoint(NULL, &offset, &turned);
    njPopMatrix(1);
    ground->pos.x = work->pos.x + turned.x;
    ground->pos.y = work->pos.y + turned.y;
    ground->pos.z = work->pos.z + turned.z;
    if (record->flags & ENEMY_WATER) {
        if (ground_find_sides_water(ground)) {
            /* @hack spelling: the flag set by flags = flags | ENEMY_ON_WATER,
             * which keeps record in R14 and ground in R12 (MOV R5,R14 at
             * 0x8c049ff4); with |=, the two trade and 13 of 119 words differ */
            record->flags = record->flags | ENEMY_ON_WATER;
        } else {
            record->flags &= ~ENEMY_ON_WATER;
        }
    } else {
        ground_find_sides(ground);
    }
}

static void enemy_collide_ceiling(struct task_work *work,
                                  struct lockon_target *record)
{
    Float top = record->ground.pos.y + record->top;
    struct surface *ceiling = &record->ground.sides[SIDE_ABOVE];

    record->flags &= ~ENEMY_UNDER_CEILING;
    if (top > ceiling->level) {
        record->motion.speed.y = 0.0F;
        work->pos.y -= (top - ceiling->level) * 0.3F;
        record->flags |= ENEMY_UNDER_CEILING;
    }
}

static void enemy_collide_ground(struct task_work *work,
                                 struct lockon_target *record)
{
    Float bottom = record->ground.pos.y + record->bottom;
    struct surface *floor = &record->ground.sides[SIDE_BELOW];
    Float width;
    Float length;

    if ((record->flags & ENEMY_KEEP_ON_LEDGE) &&
        !(record->flags & ENEMY_NO_LEDGE) &&
        (bottom - record->step_down > floor->level || !floor->found)) {
        enemy_restore_pos(work, record);
        record->flags |= ENEMY_ON_GROUND | ENEMY_NO_LEDGE;
        work->pos.x -= record->motion.speed.x * 0.4F;
        work->pos.z -= record->motion.speed.z * 0.4F;
        record->motion.speed.x = 0.0F;
        record->motion.speed.y = 0.0F;
        record->motion.speed.z = 0.0F;
    } else {
        record->flags &= ~ENEMY_ON_GROUND;
        if (floor->level > bottom) {
            record->motion.speed.y *= -record->floor_bounce;
            record->motion.speed.x *= record->friction;
            record->motion.speed.z *= record->friction;
            work->pos.y += (floor->level - bottom) * 0.3F;
            if (record->flags & ENEMY_TILT) {
                work->ang.x = angle_approach(work->ang.x, floor->ang_x, 0x80);
                work->ang.z = angle_approach(work->ang.z, floor->ang_z, 0x80);
            }
            record->flags |= ENEMY_ON_GROUND;
            record->flags &= ~ENEMY_NO_LEDGE;
        }
    }
    if (!(record->flags & ENEMY_SHADOW)) {
        return;
    }
    width = record->shadow_size /
            (1.0F + (0.03F * _builtin_fabsf(bottom - floor->level)));
    length = record->shadow_size * record->shadow_ratio;
    texture_set_regular();
    draw_set_material(-0.05F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, floor->level + 0.2F, work->pos.z);
    TURN(njRotateZ, floor->ang_z);
    TURN(njRotateX, floor->ang_x);
    TURN(njRotateY, work->ang.y);
    njScale(NULL, width, width, length);
    njSimpleDrawObject(shadow_object);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* push the enemy out of the wall the probe found on side. (facing_x,
 * facing_z) points toward that side, and the wall counts only if its normal
 * lies within 60 degrees of the opposite way. the wall's plane passes
 * through (plane_x, pos.y, plane_z), and plane_x and plane_z may read wall.
 * an enemy within radius of the plane moves out along the normal by 0.35 of
 * its depth. if it moves into the wall, it also bounces off by wall_bounce
 * and turns by wall_turn. the macro names enemy_collide_walls' arguments and
 * locals */
#define WALL_PUSH(side, facing_x, facing_z, plane_x, plane_z)                  \
    do {                                                                       \
        if (ground->sides[side].found) {                                       \
            NJS_VECTOR facing = {facing_x, 0.0F, facing_z};                    \
            NJS_POINT3 nearest;                                                \
            NJS_VECTOR push;                                                   \
            NJS_VECTOR axis;                                                   \
            NJS_VECTOR normal;                                                 \
                                                                               \
            wall = &ground->sides[side];                                       \
            if (njInnerProduct(&facing, &wall->normal) < -0.5F) {              \
                plane.px = plane_x;                                            \
                plane.py = pos.y;                                              \
                plane.pz = plane_z;                                            \
                plane.vx = wall->normal.x;                                     \
                plane.vy = wall->normal.y;                                     \
                plane.vz = wall->normal.z;                                     \
                depth = radius - njDistanceP2PL(&pos, &plane, &nearest);       \
                if (depth > 0.0F) {                                            \
                    push.x = wall->normal.x;                                   \
                    push.y = 0.0F;                                             \
                    push.z = wall->normal.z;                                   \
                    push.x *= depth * 0.35F;                                   \
                    push.y *= depth * 0.35F;                                   \
                    push.z *= depth * 0.35F;                                   \
                    njAddVector(&work->pos, &push);                            \
                    record->flags |= ENEMY_ON_WALL;                            \
                    if (njInnerProduct(&record->motion.speed, &wall->normal) < \
                        0.0F) {                                                \
                        normal = wall->normal;                                 \
                        if (njUnitVector(&normal) == 0.0F) {                   \
                            normal.x = 1.0F;                                   \
                            normal.y = 0.0F;                                   \
                            normal.z = 0.0F;                                   \
                        }                                                      \
                        bounce =                                               \
                            -(njInnerProduct(&record->motion.speed, &normal) * \
                              record->wall_bounce * 2.0F);                     \
                        normal.x *= bounce;                                    \
                        normal.y *= bounce;                                    \
                        normal.z *= bounce;                                    \
                        njAddVector(&record->motion.speed, &normal);           \
                        njOuterProduct(&normal, &record->motion.speed, &axis); \
                        if (axis.y > 0.0F) {                                   \
                            work->ang.y -= record->wall_turn;                  \
                        } else {                                               \
                            work->ang.y += record->wall_turn;                  \
                        }                                                      \
                    }                                                          \
                }                                                              \
            }                                                                  \
        }                                                                      \
    } while (0)

static void enemy_collide_walls(struct task_work *work,
                                struct lockon_target *record)
{
    NJS_POINT3 pos;
    NJS_PLANE plane;
    struct sides_query *ground = &record->ground;
    struct surface *wall;
    Float radius = record->radius;
    Float depth;
    Float bounce;

    pos = ground->pos;
    record->flags &= ~ENEMY_ON_WALL;
    WALL_PUSH(SIDE_MINUS_X, -1.0F, 0.0F, wall->level, pos.z);
    WALL_PUSH(SIDE_PLUS_X, 1.0F, 0.0F, wall->level, pos.z);
    WALL_PUSH(SIDE_MINUS_Z, 0.0F, -1.0F, pos.x, wall->level);
    WALL_PUSH(SIDE_PLUS_Z, 0.0F, 1.0F, pos.x, wall->level);
}

void enemy_collide(struct task_work *work, struct lockon_target *record)
{
    enemy_place_probe(work, record);
    enemy_collide_ceiling(work, record);
    enemy_collide_ground(work, record);
    enemy_collide_walls(work, record);
}

void enemy_set_home(struct task_work *work, struct lockon_target *record)
{
    record->home = work->pos;
}

void enemy_save_pos(struct task_work *work, struct lockon_target *record)
{
    record->last_pos = work->pos;
}

void enemy_restore_pos(struct task_work *work, struct lockon_target *record)
{
    work->pos = record->last_pos;
}

Float enemy_target_distance(struct task_work *work,
                            struct lockon_target *record)
{
    Float distance2 = SQUARE(record->target.x - work->pos.x) +
                      SQUARE(record->target.y - work->pos.y) +
                      SQUARE(record->target.z - work->pos.z);

    return njSqrt(distance2);
}

Float enemy_target_distance2(struct task_work *work,
                             struct lockon_target *record)
{
    Float distance2 = SQUARE(record->target.x - work->pos.x) +
                      SQUARE(record->target.y - work->pos.y) +
                      SQUARE(record->target.z - work->pos.z);

    return distance2;
}

int enemy_aim_at_target(struct task_work *work, struct lockon_target *record)
{
    Angle heading = work->ang.y;

    record->aim_y = (Angle)(atan2f(record->target.x - work->pos.x,
                                   record->target.z - work->pos.z) *
                            ANGLE_PER_RADIAN) -
                    NJM_DEG_ANG(90);
    return (Uint16)angle_distance(record->aim_y, heading);
}

int enemy_turn_to_target(struct task_work *work, struct lockon_target *record)
{
    int away = enemy_aim_at_target(work, record);

    work->ang.y =
        angle_approach(work->ang.y, record->aim_y, record->turn_speed);
    return away;
}

Float enemy_home_distance(struct task_work *work, struct lockon_target *record)
{
    Float distance2 = SQUARE(record->home.x - work->pos.x) +
                      SQUARE(record->home.y - work->pos.y) +
                      SQUARE(record->home.z - work->pos.z);

    return njSqrt(distance2);
}

Float enemy_home_distance2(struct task_work *work, struct lockon_target *record)
{
    Float distance2 = SQUARE(record->home.x - work->pos.x) +
                      SQUARE(record->home.y - work->pos.y) +
                      SQUARE(record->home.z - work->pos.z);

    return distance2;
}

int enemy_aim_at_home(struct task_work *work, struct lockon_target *record)
{
    Angle heading = work->ang.y;

    record->aim_y = (Angle)(atan2f(record->home.x - work->pos.x,
                                   record->home.z - work->pos.z) *
                            ANGLE_PER_RADIAN) -
                    NJM_DEG_ANG(90);
    return (Uint16)angle_distance(record->aim_y, heading);
}

int enemy_turn_home(struct task_work *work, struct lockon_target *record)
{
    int away = enemy_aim_at_home(work, record);

    work->ang.y =
        angle_approach(work->ang.y, record->aim_y, record->turn_speed);
    return away;
}

Float enemy_player_distance2(struct task_work *work, int player)
{
    struct task_work *other = player_work[player];
    Float distance2 = SQUARE(other->pos.x - work->pos.x) +
                      SQUARE(other->pos.y - work->pos.y) +
                      SQUARE(other->pos.z - work->pos.z);

    return distance2;
}

int enemy_aim_at_player(struct task_work *work, struct lockon_target *record,
                        Uint8 player)
{
    struct task_work *other = player_work[player];
    Angle heading = work->ang.y;
    /* @hack redundant: pos, never read, which puts work in R14 and other in R13
     * (MOV R4,R14 at 0x8c04ae90); without it, the two trade and 9 of 47
     * words differ */
    NJS_POINT3 *pos = &work->pos;

    record->aim_y =
        (Angle)(atan2f(other->pos.x - work->pos.x, other->pos.z - work->pos.z) *
                ANGLE_PER_RADIAN) -
        NJM_DEG_ANG(90);
    return (Uint16)angle_distance(record->aim_y, heading);
}

int enemy_turn_to_player(struct task_work *work, struct lockon_target *record,
                         Uint8 player)
{
    int away = enemy_aim_at_player(work, record, player);

    work->ang.y =
        angle_approach(work->ang.y, record->aim_y, record->turn_speed);
    return away;
}

int enemy_turn_from_player(struct task_work *work, struct lockon_target *record,
                           Uint8 player)
{
    int away = enemy_aim_at_player(work, record, player);

    record->aim_y += NJM_DEG_ANG(180);
    work->ang.y =
        angle_approach(work->ang.y, record->aim_y, record->turn_speed);
    return away;
}

Sint8 enemy_on_screen(NJS_POINT3 *pos)
{
    NJS_POINT2 projected;

    njProjectScreen(NULL, pos, &projected);
    if (projected.x >= 0.0F && projected.x <= SCREEN_WIDTH &&
        projected.y >= 0.0F && projected.y <= 400.0F) {
        return 1;
    }
    return 0;
}

void enemy_target_near_player(struct task_work *work,
                              struct lockon_target *record, int player,
                              int spot)
{
    struct task_work *other = player_work[player];
    NJS_POINT3 offset;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    TURN(njRotateY, camera_work->ang.y);
    njCalcPoint(NULL, &enemy_player_spots[spot], &offset);
    njPopMatrix(1);
    record->target.x = other->pos.x + offset.x;
    record->target.y = other->pos.y + offset.y;
    record->target.z = other->pos.z + offset.z;
}

static void enemy_piece_move(struct task_work *work)
{
    struct angles slope;
    Float height;

    SPEED(work).y -= 0.06F;
    if (work->tick++ > 9) {
        work->tick = 0;
        height =
            ground_below(work->pos.x, work->pos.y + 4.0F, work->pos.z, &slope);
        height = _builtin_fabsf(height - work->pos.y);
        if (SPEED(work).y < 0.0F && height < 1.0F) {
            SPEED(work).y = -(SPEED(work).y * 0.8F);
        }
    }
    VECTOR_ADD(&work->pos, &SPEED(work));
    work->ang.x += VARS(work)->spin;
    work->ang.y += VARS(work)->spin;
    work->ang.z += SPIN_Z(work);
}

/* debris spins at fixed rates. the spins enemy_scatter_debris gives each
 * bit go unread */
static void enemy_debris_move(struct task_work *work)
{
    SPEED(work).y -= 0.09F;
    VECTOR_ADD(&work->pos, &SPEED(work));
    work->ang.x += 0x200;
    work->ang.y += 0x1000;
    work->ang.z += 0x700;
}

static void enemy_piece_draw(struct task_work *work)
{
    NJS_MODEL *model = VARS(work)->model;
    NJS_POINT3 *center = &model->center;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        njSetTexture(VARS(work)->texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        njTranslate(NULL, -center->x, -center->y, -center->z);
        draw_model(model);
        njPopMatrix(1);
    }
}

static void enemy_debris_draw(struct task_work *work)
{
    NJS_MODEL *model = VARS(work)->model;
    /* @hack redundant: center, never read, which keeps model in R13 across the
     * calls (MOV.L @(8,R14),R13 at 0x8c04b2a8); without it, model goes to
     * the frame, the function runs 4 bytes short and 29 of 87 words differ */
    NJS_POINT3 *center = &model->center;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        texture_set_regular();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        njScale(NULL, 0.5F, 1.5F, 0.5F);
        njScale(NULL, 0.6F, 0.6F, 0.6F);
        draw_model(model);
        njPopMatrix(1);
    }
}

static void enemy_piece_dust(struct task_work *work)
{
    struct particle *particle;

    particle = particle_alloc(dust_pool.pool, dust_rise_step);
    if (particle != NULL) {
        particle->pos = work->pos;
        particle->scale = 0.1F * DUST_SIZE;
        particle->time = rand() * RAND_SCALE * 6.0F;
        particle->ang = rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE;
    }
}

/* a piece leaves dust every third step */
static void enemy_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    Float y;

    switch (work->mode) {
    case PIECE_START:
        work->mode = PIECE_FALL;
        break;
    case PIECE_FALL:
        if (TASK_TIMER(work)++ >= PIECE_TIMER_END) {
            TASK_TIMER(work) = 0xff;
            work->mode = PIECE_END;
        }
        enemy_piece_move(work);
        enemy_piece_draw(work);
        /* @hack redundant: stores of y that never run. SHC gives y a
         * register, FR15, then deletes the stores, so the image saves FR15
         * and never uses it (0x8c04b402). without them, the function runs 2
         * bytes short and 60 of 62 words differ */
        if (0) {
            SPEED(work).x = y;
            SPEED(work).y = y;
        }
        if (TASK_TIMER(work)++ % 3 == 0) {
            enemy_piece_dust(work);
        }
        break;
    case PIECE_END:
        TASK_TIMER(work) = 0xff;
        VARS(work)->model = NULL;
        VARS(work)->texlist = NULL;
        task_kill(task);
        break;
    }
}

static void enemy_debris_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case PIECE_START:
        work->mode = PIECE_FALL;
        VARS(work)->model = &enemy_debris_model;
        break;
    case PIECE_FALL:
        if (TASK_TIMER(work)++ >= PIECE_TIMER_END) {
            VARS(work)->model = NULL;
            VARS(work)->texlist = NULL;
            task_kill(task);
        } else {
            enemy_debris_move(work);
            enemy_debris_draw(work);
        }
        break;
    }
}

void enemy_break(struct task_work *work, NJS_MODEL **models,
                 NJS_TEXLIST *texlist)
{
    int i;
    struct task *task;
    struct task_work *piece;

    for (i = 0; models[i] != NULL; i++) {
        task = task_create(TASK_WORK, 2, enemy_piece_step);
        if (task != NULL) {
            piece = task->work;
            piece->pos = work->pos;
            SPEED(piece).x = ((rand() * RAND_SCALE) - 0.5F) * 2.0F;
            SPEED(piece).y = (rand() * RAND_SCALE * 2.0F) + 0.9F;
            SPEED(piece).z = ((rand() * RAND_SCALE) - 0.5F) * 2.0F;
            piece->ang = work->ang;
            piece->tick = i % 10;
            TASK_TIMER(piece) = 0;
            VARS(piece)->model = models[i];
            VARS(piece)->spin =
                (0.5F - (rand() * RAND_SCALE)) * 20.0F * ANGLE_PER_DEGREE;
            SPIN_Z(piece) =
                (0.5F - (rand() * RAND_SCALE)) * 20.0F * ANGLE_PER_DEGREE;
            VARS(piece)->texlist = texlist;
        }
    }
}

void enemy_scatter_debris(struct task_work *work, Uint8 count)
{
    int i;
    struct task *task;
    struct task_work *debris;

    for (i = 0; i < count; i++) {
        task = task_create(TASK_WORK, 3, enemy_debris_step);
        if (task != NULL) {
            debris = task->work;
            debris->pos = work->pos;
            SPEED(debris).x = ((rand() * RAND_SCALE) - 0.5F) * 3.0F;
            SPEED(debris).y = (rand() * RAND_SCALE * 3.0F) + 0.1F;
            SPEED(debris).z = ((rand() * RAND_SCALE) - 0.5F) * 3.0F;
            debris->ang = work->ang;
            TASK_TIMER(debris) = 0;
            VARS(debris)->spin =
                ((rand() * RAND_SCALE) + 1.0F) * 50.0F * ANGLE_PER_DEGREE;
            SPIN_Z(debris) =
                ((rand() * RAND_SCALE) + 1.0F) * 50.0F * ANGLE_PER_DEGREE;
        }
    }
}

void enemy_spring_scale(struct task_work *work, struct lockon_target *record)
{
    Float stretch = work->scl.y - 1.0F;

    record->scale_speed += -(record->scale_speed * 0.2F) - (stretch * 0.32F);
    work->scl.y += record->scale_speed;
    if (work->scl.y > 1.7F) {
        work->scl.y = 1.7F;
    } else if (work->scl.y < 0.3F) {
        work->scl.y = 0.3F;
    }
    work->scl.x = 2.0F - work->scl.y;
}

void enemy_bounce_player(Sint8 player)
{
    struct task_work *work = player_work[player];
    int character;

    if (work == NULL) {
        return;
    }
    if ((work->flags & PLAYER_BALL) && !(work->flags & PLAYER_ON_GROUND)) {
        character = PLAYER_VARS(work)->character;
        if (character == CHARACTER_SONIC || character == CHARACTER_TAILS ||
            character == CHARACTER_KNUCKLES) {
            player_set_speed(player, 0.0F, 2.0F, 0.0F);
            vibration_start(player, 6, 30, 0);
        }
    }
}

Bool enemy_take_hit(struct task_work *work, struct lockon_target *record)
{
    struct collision_work *hit = work->collision->hit;

    if (work->flags & COLLISION_DAMAGED) {
        record->scale_speed = 0.35F;
        if (hit->kind == COLLISION_PLAYER || hit->kind == COLLISION_ATTACK) {
            record->flags |= LOCKON_HIT;
            score_add_bonus(10);
            if (hit->shapes[work->collision->hit_shape].flags &
                SHAPE_THROWS_ENEMIES) {
                record->motion.speed.y = 0.7F;
                record->flags |= ENEMY_THROWN;
            } else {
                if (player_work[0] == hit->owner->work) {
                    enemy_bounce_player(0);
                } else {
                    enemy_bounce_player(1);
                }
                record->flags &= ~ENEMY_UNK_400;
            }
        }
        return TRUE;
    }
    if (item_explosion_reaches(&work->pos)) {
        return TRUE;
    }
    return FALSE;
}
