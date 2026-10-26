/* Zero's arms. a throw sends hand 0 out on its chain, and a spread swings
 * out both hands */
#ifndef B_ROBO_ARMS_H
#define B_ROBO_ARMS_H

/* the modes erobo_arms_set_mode takes. erobo_arms_step moves on to modes of
 * its own, numbered past EROBO_ARMS_RETURN, and erobo_arms_get_mode returns
 * those too */
enum erobo_arms_mode {
    EROBO_ARMS_IDLE,
    EROBO_ARMS_THROW, /* hand 0 flies at the marker's lock and back */
    EROBO_ARMS_SPREAD, /* both hands swing out to the sides */
    /* both hands hold the spread 30 steps more, then fly home */
    EROBO_ARMS_RETURN
};

void erobo_arms_set_mode(enum erobo_arms_mode mode);
/* the mode the arms are in. it is EROBO_ARMS_IDLE again once the hands are
 * home */
int erobo_arms_get_mode(void);
/* set the arms idle and create their two tasks. the first holds a sphere
 * for each node of both chains. the spheres hurt the player from
 * EROBO_ARMS_SPREAD until the hands start home. the second draws the light
 * along the chains and its sparks, with the textures erobo_effects_start
 * binds. the module must stay loaded until both tasks are destroyed */
void erobo_arms_start(void);
/* do nothing. the tasks erobo_arms_start created live on */
void erobo_arms_end(void);
/* run the arms' mode a step. every mode but EROBO_ARMS_IDLE and
 * EROBO_ARMS_RETURN needs Zero's task. while a hand is off its arm, the
 * step hides its objects in the model bank */
void erobo_arms_step(void);
/* on a frame's first step, draw each hand that is off its arm, and its
 * chain. it needs both banks erobo_load read */
void erobo_arms_draw(void);

#endif
