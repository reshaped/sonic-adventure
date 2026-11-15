/* the matrix stack's room and depth, in matrices: njGlobal.c's, which no
 * header of the SDK declares, as njInitMatrix, njPushMatrix and
 * njPopMatrix keep them */
#ifndef XBOX_NJSTACK_H
#define XBOX_NJSTACK_H
#include <ninja.h>

extern Sint32 _max_stack_size_;
extern Sint32 _stack_size_;

#endif
