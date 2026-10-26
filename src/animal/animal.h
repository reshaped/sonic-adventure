/* the small animals, in a stage and in the Chao gardens */
#ifndef ANIMAL_ANIMAL_H
#define ANIMAL_ANIMAL_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the kinds, each named for its texture files */
enum animal_kind {
    ANIMAL_GOMA = 0, /* the seal */
    ANIMAL_PEN = 1, /* the penguin */
    ANIMAL_RAKO = 2, /* the otter */
    ANIMAL_KUJA = 3, /* the peacock */
    ANIMAL_TUBA = 4, /* the swallow */
    ANIMAL_OUM = 5, /* the parrot */
    ANIMAL_BANB = 6, /* the deer */
    ANIMAL_USA = 7, /* the rabbit */
    ANIMAL_WARA = 8, /* the wallaby */
    ANIMAL_GORI = 9, /* the gorilla */
    ANIMAL_LION = 10, /* the lion */
    ANIMAL_ZOU = 11, /* the elephant */
    ANIMAL_MOGU = 12, /* the mole */
    ANIMAL_KOAR = 13, /* the koala */
    ANIMAL_SUKA = 14, /* the skunk */
    /* the number of kinds, and so the rows of each table by kind */
    ANIMAL_KINDS = 15,
    /* animal_create_custom's kind, which no table by kind has a row for. it
     * draws with its model's action and texlist */
    ANIMAL_CUSTOM = ANIMAL_KINDS
};

/* each kind's action */
extern NJS_ACTION goma_action;
extern NJS_ACTION pen_action;
extern NJS_ACTION rako_action;
extern NJS_ACTION kuja_action;
extern NJS_ACTION tuba_action;
extern NJS_ACTION oum_action;
extern NJS_ACTION banb_action;
extern NJS_ACTION usa_action;
extern NJS_ACTION wara_action;
extern NJS_ACTION gori_action;
extern NJS_ACTION lion_action;
extern NJS_ACTION zou_action;
extern NJS_ACTION mogu_action;
extern NJS_ACTION koar_action;
extern NJS_ACTION suka_action;

/* what animal_create_custom makes an animal from */
struct animal_model {
    NJS_POINT3 pos;
    Float speed; /* motion frames a step */
    NJS_ACTION *action;
    NJS_TEXLIST *texlist;
};

/* make an animal at x, y, z whose first step is start, one of the
 * animal_start_ steps. it faces a random way, and player 0 looks at it for
 * 300 steps. returns the task's work, or null for a null start or a failed
 * allocation. a task that can't be allocated leaks the animal's record */
struct task_work *animal_create(Float x, Float y, Float z,
                                void (*start)(struct task *task));
/* as animal_create, but the animal circles where it's made, in a barrier
 * that doesn't break with age */
struct task_work *animal_create_staying(Float x, Float y, Float z,
                                        void (*start)(struct task *task));
/* each kind's first step, for animal_create's start. it destroys the task
 * when player 0's slot is empty */
void animal_start_goma(struct task *task);
void animal_start_pen(struct task *task);
void animal_start_tuba(struct task *task);
void animal_start_kuja(struct task *task);
void animal_start_oum(struct task *task);
void animal_start_koar(struct task *task);
void animal_start_wara(struct task *task);
void animal_start_banb(struct task *task);
void animal_start_usa(struct task *task);
void animal_start_mogu(struct task *task);
void animal_start_zou(struct task *task);
void animal_start_lion(struct task *task);
void animal_start_gori(struct task *task);
void animal_start_suka(struct task *task);
void animal_start_rako(struct task *task);
/* make an animal from model as animal_create_staying does, one that flies
 * and draws with model's action and texlist. it copies model's position and
 * speed, so model needn't outlive the call, but the action and texlist must
 * outlive the animal. destructor, if not null, gets the task as the animal
 * is destroyed. returns the task's work, or null for a null model or a
 * failed allocation. in a Chao garden the animal's first step destroys it.
 *
 * a null return can leave the task running. when the animal's spec alone
 * can't be allocated, the task runs on with model's action and a null spec,
 * which its steps read through. destructor is never called for it */
struct task_work *animal_create_custom(struct animal_model *model,
                                       void (*destructor)(struct task *task));

#endif
