/* the uni, a sea urchin circled by four child tasks, in three kinds */
#ifndef ENEMY_UNI_H
#define ENEMY_UNI_H
#include <ninja.h>

struct task;

/* UNI_A_UNIBODY, the PVM of E UNI A's and E UNI B's bodies */
extern NJS_TEXLIST uni_a_unibody_texlist;
/* UNI_C_UNIBODY, the PVM of E UNI C's body and fireballs */
extern NJS_TEXLIST uni_c_unibody_texlist;

/* the first steps the object lists name E UNI A, E UNI B and E UNI C. the
 * low byte of ang.z is the master emerald piece the uni carries, which
 * Knuckles takes as it bursts. a uni farther than scl.x + 50 from where it
 * was placed turns toward that place.
 *
 * E UNI A and E UNI B start 12 and 9 above the floor under them. E UNI C
 * starts 8 above it, unless scl.z is 1000 or more. below that scl.z, a uni
 * that isn't waiting rises or sinks toward 8 above the floor under it. it
 * moves back to where it was when that floor lies more than 50 below its
 * last height */
void uni_a_start(struct task *task);
void uni_b_start(struct task *task);
void uni_c_start(struct task *task);

#endif
