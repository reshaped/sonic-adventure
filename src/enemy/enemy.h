/* what the enemies share: their records, collision with the world,
 * turning, breaking apart and being hit */
#ifndef ENEMY_ENEMY_H
#define ENEMY_ENEMY_H
#include <ninja.h>
#include <sg_xpt.h>

#include "player/lockon.h"
#include "system/task.h"

/* an enemy's bits in its lockon_target's flags, beside LOCKON_HIT */
/* enemy_collide holds the enemy at a ledge. it puts the enemy back at
 * last_pos, takes a further 0.4 of speed.x and speed.z off pos.x and pos.z,
 * and zeroes the speed. a ledge is a floor more than step_down below the
 * enemy's bottom, or no floor at all */
#define ENEMY_KEEP_ON_LEDGE 0x1
#define ENEMY_SHADOW 0x2 /* enemy_collide draws its shadow on the floor */
/* on the floor, enemy_collide tilts the enemy toward the floor's slope */
#define ENEMY_TILT 0x4
/* enemy_collide sets or clears these three on each call */
#define ENEMY_UNDER_CEILING 0x8 /* its top reached the ceiling */
#define ENEMY_ON_GROUND 0x10 /* it reached the floor, or a ledge held it */
#define ENEMY_ON_WALL 0x40 /* a wall within its radius pushed it out */
/* while this is set, enemy_collide doesn't hold the enemy at a ledge.
 * enemy_collide sets it at each hold, and clears it as the enemy reaches the
 * floor. enemy_alloc_record sets it too */
#define ENEMY_NO_LEDGE 0x80
#define ENEMY_BLENDING 0x200 /* it blends from action into next_action */
/* enemy_alloc_record sets it. enemy_take_hit clears it on a hit by a
 * player's or an attack's collision record that doesn't throw the enemy */
#define ENEMY_UNK_400 0x400
/* enemy_take_hit sets it as a hit throws the enemy up */
#define ENEMY_THROWN 0x800
/* with ENEMY_WATER, enemy_collide finds liquids as well as the ground. it
 * then sets ENEMY_ON_WATER to what ground_find_sides_water returns. without
 * ENEMY_WATER, it leaves ENEMY_ON_WATER as it is */
#define ENEMY_WATER 0x2000
#define ENEMY_ON_WATER 0x4000

/* the set editor draws an enemy's range as ENEMY_RANGE_SEGMENTS lines
 * round a circle, each spanning ENEMY_RANGE_TURN */
#define ENEMY_RANGE_SEGMENTS 32
#define ENEMY_RANGE_TURN (0x10000 / ENEMY_RANGE_SEGMENTS)

#define ENEMY_SET_MODE(work, record, to)                                       \
    ((record)->last_mode = (work)->mode, (work)->mode = (to), (work)->timer = 0)

/* an enemy calls it as a hit beats it. bytes 1 and 2 mean what the enemy's
 * own vars make of them */
#define ENEMY_MARK_BEATEN(work) ((work)->vars.b[1] = 0, (work)->vars.b[2] = 1)

/* do nothing with value. a call keeps the caller's load of value, which
 * the image makes and never uses. enemies' hit checks pass it player_work,
 * player 0's work or the collision record that hit them */
#pragma inline(enemy_ignore)
/* a source that includes this header and never calls it leaves it unused */
/* NOLINTNEXTLINE(clang-diagnostic-unused-function) */
static void enemy_ignore(void *value)
{
    /* @hack redundant: a store through value that never runs. it keeps each
     * caller's load of value, as saru_take_hit's of player_work at
     * 0x8c10a2a4. without it, SHC drops the loads, and summed over the 12
     * functions that call it, 440 of 618 words differ */
    if (0) {
        *(void **)value = value;
    }
}

/* allocate task's record, for E-102 to lock on to, and point task->motion
 * at it. the record's fields are 0 but for the defaults and home, which
 * takes work->pos. if the heap has no room, it kills task and returns NULL.
 * most of the game's callers leave the result unchecked */
struct lockon_target *enemy_alloc_record(struct task *task,
                                         struct task_work *work);
/* does nothing. an enemy calls it as it dies, leaves or is destroyed */
void enemy_end(struct task *task, struct task_work *work,
               struct lockon_target *record);
/* 1 while the enemy sees player 0 or 1, by its record's sight_angle,
 * sight_distance and near_distance, else 0 */
Sint8 enemy_sees_player(struct task_work *work, struct lockon_target *record);
/* move the enemy out of the ceiling, the floor and the walls that the
 * record's probe finds. as the record's flags ask, it also holds the enemy
 * at ledges and draws its shadow */
void enemy_collide(struct task_work *work, struct lockon_target *record);
void enemy_set_home(struct task_work *work, struct lockon_target *record);
void enemy_save_pos(struct task_work *work, struct lockon_target *record);
void enemy_restore_pos(struct task_work *work, struct lockon_target *record);
/* enemy_aim_at_target, enemy_aim_at_home and enemy_aim_at_player set aim_y
 * to the heading toward the record's target, its home or a player. each
 * returns how far work->ang.y is from aim_y, 0 to 0x8000. each turn below
 * calls the matching aim and turns ang.y toward aim_y by turn_speed. it
 * returns what the aim returned */
int enemy_aim_at_target(struct task_work *work, struct lockon_target *record);
int enemy_turn_to_target(struct task_work *work, struct lockon_target *record);
int enemy_aim_at_home(struct task_work *work, struct lockon_target *record);
int enemy_turn_home(struct task_work *work, struct lockon_target *record);
int enemy_aim_at_player(struct task_work *work, struct lockon_target *record,
                        Uint8 player);
int enemy_turn_to_player(struct task_work *work, struct lockon_target *record,
                         Uint8 player);
/* aim at the player, then set aim_y to the opposite heading and turn ang.y
 * toward it by turn_speed. it returns what enemy_aim_at_player returned */
int enemy_turn_from_player(struct task_work *work, struct lockon_target *record,
                           Uint8 player);
Float enemy_target_distance(struct task_work *work,
                            struct lockon_target *record);
Float enemy_target_distance2(struct task_work *work,
                             struct lockon_target *record);
Float enemy_home_distance(struct task_work *work, struct lockon_target *record);
Float enemy_home_distance2(struct task_work *work,
                           struct lockon_target *record);
Float enemy_player_distance2(struct task_work *work, int player);
/* 1 while njProjectScreen puts pos within 0 to SCREEN_WIDTH across and 0 to
 * 400 down, else 0 */
Sint8 enemy_on_screen(NJS_POINT3 *pos);
/* set the record's target to one of four points near player, spot 0 to 3.
 * the points turn with the camera's heading */
void enemy_target_near_player(struct task_work *work,
                              struct lockon_target *record, int player,
                              int spot);
/* break the enemy into a piece for each model in the NULL-ended models,
 * drawn with texlist. each piece falls and bounces for 61 steps. it holds
 * its model and texlist until it goes, so both must last as long */
void enemy_break(struct task_work *work, NJS_MODEL **models,
                 NJS_TEXLIST *texlist);
/* the bits of debris an enemy throws as it blows up */
#define DEBRIS_COUNT 8
/* throw count bits of debris out from the enemy. each falls for 120 steps */
void enemy_scatter_debris(struct task_work *work, Uint8 count);
/* spring scl.y back toward 1 at scale_speed. each call damps scale_speed
 * by 0.2 of itself and pulls it by 0.32 of 1 - scl.y. scl.y stays within
 * 0.3 to 1.7, and scl.x becomes 2 - scl.y */
void enemy_spring_scale(struct task_work *work, struct lockon_target *record);
/* set player's speed to 2 straight up and shake its pad. it does so only
 * for Sonic, Tails or Knuckles curled up in the air */
void enemy_bounce_player(Sint8 player);
/* TRUE if a hit damaged the enemy this step or an explosion item's blast
 * reaches it, else FALSE. damage sets scale_speed to 0.35 for
 * enemy_spring_scale. damage by a player's or an attack's collision record
 * also sets LOCKON_HIT and adds 10 bonus points. if the hitting shape has
 * SHAPE_THROWS_ENEMIES, the hit throws the enemy up and sets ENEMY_THROWN.
 * otherwise the hit clears ENEMY_UNK_400 and calls enemy_bounce_player: for
 * player 0 if the hitting collision record is player 0's, else for player 1 */
Bool enemy_take_hit(struct task_work *work, struct lockon_target *record);

#endif
