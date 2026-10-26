/* the Chao race's tuner, which steps Float values from the second pad */
#ifndef AL_RACE_TUNE_H
#define AL_RACE_TUNE_H
#include <ninja.h>

struct tune_entry {
    const char *label;
    Float *value;
    /* what each tune_step adds to *value while up is held, or takes while
     * down is */
    Float step;
};

/* clear every entry. tune_step needs it or tune_add to have run first:
 * before either, tune_step takes the first slot's null as an entry */
void tune_reset(void);
/* add entry in the first free slot, unless entry is null or no slot is free */
void tune_add(struct tune_entry *entry);
/* run the tuner while it holds an entry, and else do nothing. left and right
 * on the second pad choose the entry shown, and holding up or down steps its
 * value. the entry's label and value print on each frame's first step */
void tune_step(void);

#endif
