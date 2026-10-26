/* a Chao in the world: the surfaces around it and how they push it, its
 * speed and its turn toward its target, and the garden of the stage */
#ifndef AL_MAIN_PHYSICS_H
#define AL_MAIN_PHYSICS_H
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "system/task.h"
#include "system/util.h"

/* the yaw that faces the Chao's target from its work's pos. a yaw of 0
 * faces along x */
#define TARGET_HEADING(work, chao)                                             \
    ((Angle)(atan2f((chao)->target.x - (work)->pos.x,                          \
                    (chao)->target.z - (work)->pos.z) *                        \
             ANGLE_PER_RADIAN) -                                               \
     0x4000)

/* set the Chao's terrain's query_height to 5, head_height to 15 and
 * wall_bounce to 0.23. unk_04 and unk_0c take 0, unk_14 15, unk_18 -3,
 * unk_34 1.6 and unk_38 1. the rest of the terrain keeps its values, and
 * work is unused */
void alife_terrain_init(struct task_work *work, struct alife_chao *chao);

/* fill terrain->query with the nearest surface on each side of its pos. the
 * result is the world flags of the last object with a surface, or 0.
 *
 * the surfaces are those of the objects world_list_near lists within 1 of the
 * point, which stay listed. a task's object counts only where the point lies
 * within its model's radius, times its largest scale, of the object's pos. a
 * side with no surface keeps found FALSE. its level is then -GROUND_FAR below
 * and toward -x and -z, and GROUND_FAR on the other three sides. water counts
 * only below and above the point.
 *
 * TERRAIN_WATER_BELOW says water is the nearest surface below.
 * TERRAIN_WATER_ABOVE says it is the nearest above, and less than 5 over the
 * point. TERRAIN_DEEP sets with either, and a nearer surface found later clears
 * only the other flag. the other TERRAIN_ flags clear, as do bits 0x100, 0x200
 * and 0x2000 of the flags */
int alife_find_sides(struct chao_terrain *terrain);

/* clear CHAO_ON_GROUND, and find the surfaces around the point
 * query_height above the Chao's pos, as alife_find_sides does. water there
 * sets only water_level. on a frame's first step, the Chao's shadow is
 * drawn on each floor below. then the ceiling, the walls and the floor push
 * the Chao, in that order. a floor that pushes it sets CHAO_ON_GROUND again,
 * and clears 0x80 in chao->flags. the TERRAIN_ flags say what pushed it,
 * and TERRAIN_DEEP is set where water_level is more than 1 over its pos */
void alife_collide_terrain(struct task_work *work, struct alife_chao *chao);

/* copy *target to chao->target */
void alife_set_target(struct task_work *work, struct alife_chao *chao,
                      NJS_POINT3 *target);
/* keep work->pos in chao->home */
void alife_set_home(struct task_work *work, struct alife_chao *chao);
/* keep work->pos in chao->last_pos */
void alife_save_pos(struct task_work *work, struct alife_chao *chao);
/* put chao->last_pos back in work->pos */
void alife_restore_pos(struct task_work *work, struct alife_chao *chao);

/* the distance from the Chao's work's pos to its target */
Float alife_target_distance(struct task_work *work, struct alife_chao *chao);
/* the square of that distance */
Float alife_target_distance2(struct task_work *work, struct alife_chao *chao);
/* turn the Chao's yaw toward its target, by at most turn_speed. aim takes
 * the yaw that faces the target. the result is how far the yaw was from aim
 * before the turn, from 0 to 0x8000 */
Angle alife_turn_to_target(struct task_work *work, struct alife_chao *chao);
/* the square of the camera's distance to a point. pos must address the
 * point's x, y and z, in that order. camera_work must be set */
Float alife_camera_distance_squared(Float *pos);

/* step the Chao by its speed. first the speed takes accel on x and z, and
 * accel.y less 0.05 on y, a pull down. COLLISION_HIT_BOTTOM in
 * work->collision's flags leaves speed.y as it was. then work->pos takes
 * the speed, and accel clears. work->collision must be set */
void alife_apply_speed(struct task_work *work, struct alife_chao *chao);
/* slow the Chao on x and z: accel is -drag times speed */
void alife_drag(struct task_work *work, struct alife_chao *chao, Float drag);

/* the garden of the stage, an enum garden, or -1 outside the gardens */
int alife_get_garden(void);

#endif
