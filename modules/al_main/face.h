/* a Chao's face: its eyes and mouth, where it looks, and its blink */
#ifndef AL_MAIN_FACE_H
#define AL_MAIN_FACE_H
#include <ninja.h>

#include "al_main/chao.h"
#include "system/task.h"

/* show eyes 0 to 7 or mouth 0 to 6 in place of the Chao's own. they show
 * for the next steps + 1 calls of alife_face_step, and for none where steps
 * is below 0. new eyes also clear FACE_FLAG_2 */
void alife_face_set_eyes(struct alife_chao *chao, int eyes, int steps);
void alife_face_set_mouth(struct alife_chao *chao, int mouth, int steps);
/* while another mouth shows, turn the Chao's eyes toward the angles x and z. x
 * is their turn about the x axis, and z their slant about the z axis. steps
 * sets look_steps, which counts down but ends nothing */
void alife_face_look(struct alife_chao *chao, Angle x, Angle z, int steps);
/* set the Chao's own eyes, mouth and eye angles from its kind, agres and
 * curio. any other eyes, mouth or look then ends */
void alife_face_init(struct task_work *work, struct alife_chao *chao);
/* step the face, and put it on the Chao's eye and mouth nodes. a Chaos
 * Chao keeps one face, whatever the setters above ask */
void alife_face_step(struct task_work *work, struct alife_chao *chao);

#endif
