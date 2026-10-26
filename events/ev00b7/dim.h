/* EV00B7.PRS's dimming of the models until the spotlights find Eggman */
#ifndef EV00B7_DIM_H
#define EV00B7_DIM_H

/* start the dimming's task. the task's first step saves draw_palettes[0]
 * and [1], halves their base colors and turns their offset colors black.
 * that step also makes a light with njSetLightIntensity's diffuse at 0 and
 * its ambient at -3. destroying the task puts the saved palettes back and
 * deletes the light. call it only while no dimming runs: a second task
 * saves the dimmed palettes as the ones to put back, and the first keeps
 * running */
void ev00b7_dim_create(void);
/* end the dimming, if it's on */
void ev00b7_dim_kill(void);

#endif
