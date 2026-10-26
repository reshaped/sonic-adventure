/* E-101 mkII's beam. a ball of light charges at its cannon, and the beam
 * shoots from it. where the beam's tip strikes the floor, a blast goes off */
#ifndef B_E101_R_BEAM_H
#define B_E101_R_BEAM_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* a beam starts at BEAM_IDLE, and the boss sets BEAM_CHARGE and BEAM_FIRE.
 * the beam moves itself on through BEAM_FLY and BEAM_FADE, and ends */
enum beam_state { BEAM_IDLE, BEAM_CHARGE, BEAM_FIRE, BEAM_FLY, BEAM_FADE };

/* bind the ball's frames and the beam's texture to e101_r_texlist, which
 * must be loaded */
void e101_mk2_beam_init(void);
/* do nothing */
void e101_mk2_beam_clear(void);
/* state is an enum beam_state */
void e101_mk2_beam_set_state(struct task *task, int state);
/* each step of its flight, the tip moves by dir times 10. size is the size
 * of the blast it sets off */
void e101_mk2_beam_aim(struct task *task, NJS_VECTOR *dir, Float size);
/* a beam at origin, the cannon, whose foot follows the cannon until the
 * beam fires. origin is kept, not copied, and must outlive the beam. NULL
 * when the task or the beam's record can't be allocated. the other
 * allocations go unchecked */
struct task *e101_mk2_beam_create(NJS_POINT3 *origin);

#endif
