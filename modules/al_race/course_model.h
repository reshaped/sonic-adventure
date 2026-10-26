/* the race course's model, a fixture among the scenery's children */
#ifndef AL_RACE_COURSE_MODEL_H
#define AL_RACE_COURSE_MODEL_H

/* add the course's model to the scenery, where its object places it,
 * unturned and at unit scale. its step draws it too. without the scenery
 * task, it adds nothing */
void course_model_create(void);

#endif
