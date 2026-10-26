/* O KSONICZO: the Sonic statue, which bursts into pieces when an attack
 * hurts it. O HAHEN1 to O HAHEN6: six shards, drawn as a burst ends.
 * O SONZCOL: a collision sphere, registered as a burst ends */
#ifndef STG09_BURST_STATUE_H
#define STG09_BURST_STATUE_H

#include "system/task.h"

/* the step for O KSONICZO, and for O SONZ while player 0 is Knuckles.
 * task needs TASK_WORK's record. the statue starts only within about 600 of
 * the camera or player 0. while player 0 is Sonic, the step runs
 * sonic_statue_step, and the statue is O SONZ's. otherwise the statue stands
 * until an attack hurts it. the hit bursts it only while player 0 and the
 * owner of its hit record are on land. camera_at_event_level and
 * camera_at_comp_level must also give FALSE.
 *
 * as the statue bursts, its pieces fly out and land at the floor below
 * player 0. camera_override gives the camera camera_step_pose, at an eye and
 * a target set in the statue's coordinates. player_set_speed sends player 0
 * off at (0, 2, 1), turned by the statue's y angle. the statue's 151st step
 * after the hit draws the pieces for the last time. it then gives the camera
 * back, and puts player 0 at scl, unless scl is 0.
 *
 * the start needs room on the heap, and outside the set editor in the
 * collision world's pool. with either out, it writes through null. player 0
 * must exist as an attack hurts the statue and as it bursts. camera_work
 * must exist as it bursts.
 *
 * outside the set editor, the standing statue lends the collision world a
 * copy of its object. the burst gives it back to the pool.
 *
 * the first statue started outside the set editor while none leads is the
 * leader. the leader's mode is 2 from the step its burst ends to the next */
void sonic_statue_knuckles_step(struct task *task);
/* the steps for O HAHEN1 to O HAHEN6. task needs TASK_WORK's record. a shard
 * destroys itself beyond about 600 of the camera and player 0. it draws
 * while the leader's mode is 2, and in the set editor. it draws its tree
 * scaled by scl.x + 1 and turned by its z, x and y angles. it draws only
 * while a sphere about it as large as the scaled model is in view */
void statue_shard_1_step(struct task *task);
void statue_shard_2_step(struct task *task);
void statue_shard_3_step(struct task *task);
void statue_shard_4_step(struct task *task);
void statue_shard_5_step(struct task *task);
void statue_shard_6_step(struct task *task);
/* the step for O SONZCOL. task needs TASK_WORK's record. it destroys itself
 * beyond about 250 of the camera and player 0. while the leader's mode is 2,
 * it registers a sphere of radius scl.x. in the set editor, it draws the
 * sphere as a marker instead */
void statue_collision_step(struct task *task);

#endif
