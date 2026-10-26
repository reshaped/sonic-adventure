/* O KAITENKEY: a valve handle that Amy turns. it counts her whole turns each
 * way under its key */
#ifndef STG12_HANDLE_H
#define STG12_HANDLE_H

#include "system/task.h"

/* the number of keys. handles with the same key share its counts */
#define HANDLE_KEYS 8

/* the object list's step. task needs TASK_WORK's record */
void handle_step(struct task *task);

/* set *right and *left to the whole turns each way under key. key must be
 * from 0 to HANDLE_KEYS - 1. a handle of key sets both to 0 as it starts.
 * its set entry gives the most turns it counts each way. one turn more sets
 * that count to 0 */
void handle_get_turns(int key, int *right, int *left);

#endif
