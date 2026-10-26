/* O BIGROCK, a big rock that topples once the orca's head comes near it. it
 * breaks into pieces as it falls */
#ifndef STG01_BIG_ROCK_H
#define STG01_BIG_ROCK_H

#include "system/task.h"

/* the rocks' objects, whole and in pieces. arch_rock.c's parts lists name
 * them too */
extern NJS_OBJECT beach_big_rock_a_object;
extern NJS_OBJECT beach_big_rock_a_piece_1_object;
extern NJS_OBJECT beach_big_rock_a_piece_2_object;
extern NJS_OBJECT beach_big_rock_a_piece_3_object;
extern NJS_OBJECT beach_big_rock_a_piece_4_object;
extern NJS_OBJECT beach_big_rock_a_piece_5_object;
extern NJS_OBJECT beach_big_rock_a_piece_6_object;
extern NJS_OBJECT beach_big_rock_a_piece_7_object;
extern NJS_OBJECT beach_big_rock_a_piece_8_object;
extern NJS_OBJECT beach_big_rock_a_piece_9_object;
extern NJS_OBJECT beach_big_rock_b_object;
extern NJS_OBJECT beach_big_rock_b_piece_1_object;
extern NJS_OBJECT beach_big_rock_b_piece_2_object;
extern NJS_OBJECT beach_big_rock_b_piece_3_object;
extern NJS_OBJECT beach_big_rock_b_piece_4_object;
extern NJS_OBJECT beach_big_rock_b_piece_5_object;
extern NJS_OBJECT beach_big_rock_b_piece_6_object;
extern NJS_OBJECT beach_big_rock_b_piece_7_object;
extern NJS_OBJECT beach_big_rock_b_piece_8_object;
extern NJS_OBJECT beach_big_rock_b_piece_9_object;
extern NJS_OBJECT beach_big_rock_b_piece_10_object;

/* both inits need a task with a work and a motion. work->scl.y must lie
 * within int's range: its integer part, modulo 2, is the rock's kind, a or
 * b. the work's mode stays 0 until the rock starts to topple */

/* the step the object list names for O BIGROCK. outside the set editor, it
 * gives the rock its kind's capsule */
void beach_big_rock_init(struct task *task);
/* start a big rock as another task's child. unlike beach_big_rock_init, it
 * keeps the work's ang.x and ang.z, which task_create_child set. it gives
 * the rock its capsule in the set editor too. its step has no range test,
 * and runs the rock even in the set editor */
void beach_big_rock_child_init(struct task *task);

#endif
