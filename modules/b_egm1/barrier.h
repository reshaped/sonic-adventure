/* the Egg Hornet's barrier: a shell round the boss that fades in as it
 * dives, and out again */
#ifndef B_EGM1_BARRIER_H
#define B_EGM1_BARRIER_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* the barrier's texlist. the stage's pvms list names it, so the stage loads
 * egm1barrier's textures into it */
extern NJS_TEXLIST egm1barrier_texlist;

/* what egg_hornet_barrier_request asks of the barrier. a step takes up a
 * show or a hide, which stays asked until its fade ends */
enum barrier_request { BARRIER_KEEP, BARRIER_SHOW, BARRIER_HIDE };
/* how far the barrier's fade has got */
enum barrier_state {
    BARRIER_OFF,
    BARRIER_FADING_IN,
    BARRIER_ON,
    BARRIER_FADING_OUT
};

/* the barrier, in the boss's own record */
struct barrier {
    struct task *owner; /* the task the barrier draws at */
    /* what the draw adds to the model's alpha: fade - 1 as it fades. at -1
     * the barrier is hidden, and at 0 it shows at the model's own alpha */
    Float alpha_offset;
    Float unk_08; /* a show clears it, and nothing reads it */
    Float fade; /* how far it has faded in, from 0 hidden to 1 shown */
    Sint32 timer; /* the steps since its texture changed */
    Sint32 interval; /* the steps each texture shows */
    Uint32 texture; /* the index of the texture it shows */
    Uint8 state; /* enum barrier_state */
    Uint8 request; /* enum barrier_request */
};

/* an empty function, which eggmobile1_start and viewer_boss_init call */
void fun_0c900bc0(void);
/* set the barrier hidden, with nothing asked. set its model to add its
 * color to what's behind it. owner stays unset */
void egg_hornet_barrier_init(struct barrier *barrier);
/* take up the barrier's request, and step its fade, by 1/40 a step. turn its
 * model's texture, a new one every 2 steps */
void egg_hornet_barrier_step(struct barrier *barrier);
/* draw the barrier's model twice, at 2 and 2.4 times its size, unless the
 * barrier is off. it draws 10 below the owner, turned as the owner is. it
 * draws on a frame's first step only, and leaves the constant material
 * white */
void egg_hornet_barrier_draw(struct barrier *barrier);
/* ask the barrier to show or hide, or BARRIER_KEEP to go on as it is. it
 * returns TRUE */
Bool egg_hornet_barrier_request(struct barrier *barrier, int request);
/* set the task the barrier draws at. the draw reads it unchecked, so set it
 * after init and before the barrier first shows */
void egg_hornet_barrier_set_owner(struct barrier *barrier, struct task *owner);

#endif
