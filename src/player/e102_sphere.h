/* the sphere one of E-102's shots leaves where it ends, once his laser has
 * pierced its target. it swells and fades */
#ifndef PLAYER_E102_SPHERE_H
#define PLAYER_E102_SPHERE_H

struct task;

/* the sphere's step, which draws it too. the sphere swells for 59 steps
 * after its first, and fades over the last 29 of them. its task ends on its
 * 62nd step */
void e102_sphere_step(struct task *task);

#endif
