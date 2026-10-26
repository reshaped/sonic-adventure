/* O KINKA solid gold coin that spins on its edge */
#ifndef STG09_GOLD_COIN_H
#define STG09_GOLD_COIN_H
#include <ninja.h>

#include "system/task.h"

/* coin_spray draws its model for each piece that has bounced */
extern NJS_OBJECT gold_coin_object;

/* the object list's step */
void gold_coin_step(struct task *task);

#endif
