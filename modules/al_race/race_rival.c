/* the rivals the Chao race makes, and the task for each Chao in the race */
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/chao.h"
#include "al_race/game.h"
#include "al_race/race_rival.h"
#include "chao/chao.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 5
#include "al_race/race_rival/text.inc"

static int vary(int value);

struct task *race_create_chao(void (*step)(struct task *task),
                              struct chao *record)
{
    struct task *task = task_create(TASK_WORK, 3, step);
    if (task && !alife_chao_alloc(task, record)) {
        /* @bug memory: writes the step of a task already freed, by the JSR to
         * task_kill at 0x0cb90688. alife_chao_alloc destroys the task when it
         * fails, which puts it back on the free list */
        task_kill(task);
        return NULL;
    }
    return task;
}

void race_rival_init(struct chao *rival, struct chao *source, int mode,
                     int index)
{
    int average;
    int i;
    int race_kind;
    int low;
    int high;
    Uint16 *best;
    const char *name;
    int type;
    int chance;

    if (!source && mode != RIVAL_RANDOM_2) {
        mode = RIVAL_RANDOM;
    }
    switch (mode) {
    case RIVAL_VARIED:
        rival->skills.swim = vary(source->skills.swim);
        rival->skills.fly = vary(source->skills.fly);
        rival->skills.run = vary(source->skills.run);
        rival->skills.power = vary(source->skills.power);
        break;
    case RIVAL_RANDOM:
    case RIVAL_RANDOM_2:
        /* @hack redundant: a second test of the mode, whose else, 3000 to
         * 7500 at 0x0cb9076e, never runs; without it, the function is 32
         * bytes shorter and 675 of 732 words differ */
        if (mode == RIVAL_RANDOM || mode == RIVAL_RANDOM_2) {
            if (race_get_kind(&race_kind) && race_kind == RACE_KIND_LAST) {
                low = 2000;
                high = 2800;
            } else {
                low = 1600;
                high = 2400;
            }
        } else {
            low = 3000;
            high = 7500;
        }
        rival->skills.swim = RANDOM(low, high);
        rival->skills.fly = RANDOM(low, high);
        rival->skills.run = RANDOM(low, high);
        rival->skills.power = RANDOM(low, high);
        break;
    case RIVAL_BLACK_EGG:
    case RIVAL_SILVER_EGG:
    case RIVAL_GOLD_EGG:
    default:
        rival->skills.swim = 9999;
        rival->skills.fly = 9999;
        rival->skills.run = 9999;
        rival->skills.power = 9999;
        break;
    }
    average = rival->skills.swim + rival->skills.fly + rival->skills.run +
              rival->skills.power;
    average *= 0.25F;
    rival->flags = 0;
    switch (mode) {
    case RIVAL_BLACK_EGG:
        rival->flags |= CHAO_BEGG;
        break;
    case RIVAL_SILVER_EGG:
        rival->flags |= CHAO_PAL;
        rival->flags |= CHAO_GROWTH_DUE;
        break;
    case RIVAL_GOLD_EGG:
        rival->flags |= CHAO_GOLD;
        rival->flags |= CHAO_GROWTH_DUE;
        break;
    default:
        chance = RANDOM(0, 10000);
        if (chance >= average) {
            rival->flags |= CHAO_GROWTH_DUE;
        }
        break;
    }
    if (mode == RIVAL_RANDOM || mode == RIVAL_RANDOM_2 ||
        (source->type & 0x7f) != CHAO_CHILD) {
        rival->type = CHAO_STANDARD;
        if (average < 200) {
            rival->type = CHAO_CHILD;
        } else {
            if (rival->skills.swim > rival->skills.fly) {
                best = &rival->skills.swim;
            } else {
                best = &rival->skills.fly;
            }
            if (rival->skills.run > *best) {
                best = &rival->skills.run;
            }
            if (rival->skills.power > *best) {
                best = &rival->skills.power;
            }
            if (*best - average > 280) {
                if (best == &rival->skills.swim) {
                    rival->type = CHAO_SWIM;
                } else if (best == &rival->skills.fly) {
                    rival->type = CHAO_FLY;
                } else if (best == &rival->skills.run) {
                    rival->type = CHAO_RUN;
                } else {
                    rival->type = CHAO_POWER;
                }
            }
        }
    } else if (mode == RIVAL_SILVER_EGG || mode == RIVAL_GOLD_EGG) {
        rival->type = CHAO_STANDARD;
    } else {
        rival->type = CHAO_CHILD;
    }
    type = rival->type & 0x7f;
    if (mode == RIVAL_BLACK_EGG) {
        name = TEXT_1;
        for (i = 0; *name != 0; i++) {
            /* @hack redundant: a loop of one pass around the copy, which gives
             * the parts loop's three store cursors R11, R10 and R9, and its
             * scale and bias FR14 and FR15, as 0x0cb90bf2 to 0x0cb90c02 set
             * them; without it, the cursors and the two constants trade
             * registers: 43 of 732 words and 2 of 154 other words differ */
            do {
                rival->name[i] = *name++;
            } while (0);
        }
        rival->name[i] = 0;
    } else if (mode == RIVAL_SILVER_EGG) {
        race_rival_name_by_egg(rival->name, 0, index);
    } else if (mode == RIVAL_GOLD_EGG) {
        race_rival_name_by_egg(rival->name, 1, index);
    } else {
        /* @hack type: rival->name cast to its own type, which has the mask make
         * type in R5, the argument's register, at 0x0cb909b8; without it, the
         * mask makes it in R4, and a MOV R4,R5 before the call shifts the code
         * from 0x0cb90a12 to the pool a word on: 26 of 732 words and 1 of 154
         * other words differ */
        race_rival_name_by_type((Uint8 *)rival->name, type, index);
    }
    switch (mode) {
    case RIVAL_RANDOM:
    case RIVAL_RANDOM_2:
        rival->hp_max = rival->hp = RANDOM(0, 10000);
        break;
    default:
        rival->hp_max = rival->hp = vary(source->hp_max);
        break;
    }
    rival->h_pos = RANDOM(-1.0F, 1.0F);
    rival->v_pos = RANDOM(-1.0F, 1.0F);
    rival->growth = RANDOM(0.0F, 1.0F);
    rival->race_wins = 0;
    rival->life = RANDOM(1, 65535);
    rival->old = RANDOM(1, 65535);
    if (mode == RIVAL_BLACK_EGG) {
        rival->kind = 0;
        rival->agres = 0;
        rival->curio = 0;
    } else {
        rival->kind = RANDOM(-100, 100);
        rival->agres = RANDOM(-100, 100);
        rival->curio = RANDOM(-100, 100);
    }
    rival->charm = RANDOM(0, 200);
    rival->breed = RANDOM(0, 200);
    rival->hunger = RANDOM(0, 200);
    rival->tedious = RANDOM(0, 200);
    if (mode != RIVAL_SILVER_EGG) {
        rival->sleep = RANDOM(0, 200);
        rival->unk_64 = RANDOM(0, 200);
        rival->unk_65 = RANDOM(0, 200);
    } else {
        rival->sleep = 198;
        rival->unk_64 = 198;
        rival->unk_65 = 198;
    }
    for (i = 0; i < CHAO_PARTS; i++) {
        if (mode != RIVAL_BLACK_EGG && mode != RIVAL_SILVER_EGG &&
            mode != RIVAL_GOLD_EGG) {
            int animal = RANDOM(-15, 15);
            if (animal < 0) {
                rival->parts[i] = -1;
            } else {
                rival->parts[i] = animal;
            }
        } else {
            rival->parts[i] = -1;
        }
    }
}

/* value moved at random by up to 1% either way, then kept from 1 to 9999 */
static int vary(int value)
{
    int variation = (int)(value * 0.01F);
    value += RANDOM(-variation, variation);
    value = value < 9999 ? value : 9999;
    value = value > 1 ? value : 1;
    return value;
}
