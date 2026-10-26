/* the Egg Hornet's wreck: it smokes, shudders, bursts apart and bobs */
#ifndef B_EGM1_WRECK_H
#define B_EGM1_WRECK_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;
struct task_work;

enum smoke_mode { SMOKE_ON, SMOKE_OFF };

/* the wreck's state, in struct egg_hornet's state */
enum wreck_state {
    WRECK_START,
    WRECK_SHUDDER,
    WRECK_DELAY,
    WRECK_BURST,
    WRECK_BOB,
    WRECK_DONE /* set 180 steps after the burst's step. the boss bobs on */
};

/* start puffs of smoke about *pos, which must outlive them. they go on
 * until wreck_smoke_set_mode(SMOKE_OFF). with no task free, it makes none.
 * the allocation of its record isn't checked */
void wreck_smoke_create(NJS_POINT3 *pos);
/* set the mode of the smoke made last, which must exist and not have
 * ended */
void wreck_smoke_set_mode(enum smoke_mode mode);
/* set the wreck to WRECK_START. task's extra must be a struct egg_hornet */
void wreck_reset(struct task *task);
/* run the boss's wreck a step: it shudders, bursts apart and bobs. work is
 * task's work. the burst ends the smoke made last, which must not have
 * ended */
void wreck_step(struct task *task, struct task_work *work);

#endif
