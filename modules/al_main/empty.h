/* three functions that do nothing. music.c's behaviors call all three,
 * and alife_shape_free calls fun_0c90ca28 */
#ifndef AL_MAIN_EMPTY_H
#define AL_MAIN_EMPTY_H

#include "al_main/chao.h"
#include "system/task.h"

void fun_0c90ca20(struct task_work *work, struct alife_chao *chao, int unk);
void fun_0c90ca24(struct task_work *work, struct alife_chao *chao, int unk);
void fun_0c90ca28(struct task_work *work, struct alife_chao *chao);

#endif
