/* O KEYBLOCK stand and key block, which Big carries to its stand to open the
 * hotel's pool */
#ifndef ADV00_KEY_BLOCK_H
#define ADV00_KEY_BLOCK_H

#include "system/task.h"

/* the step the object list names for O KEYBLOCK. for every character but
 * Big, and for Big once FLAG_BIG_SS_HOTEL_POOL is set, the block starts on its
 * stand. otherwise the stand is empty and its block lies apart from it, for
 * Big to carry there. once the blocks of two stands have settled on them,
 * FLAG_BIG_SS_HOTEL_POOL is set */
void key_block_init(struct task *task);

#endif
