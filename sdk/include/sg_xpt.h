/* SDK scalar types and Bool */
#ifndef SG_XPT_H
#define SG_XPT_H

typedef unsigned char Uint8;
typedef signed char Sint8;
typedef unsigned short Uint16;
typedef short Sint16;
typedef unsigned long Uint32;
typedef long Sint32;
typedef int Int;
typedef float Float;
enum bool { false, true };
typedef enum bool Bool;
#define FALSE (0)
#define TRUE (1)

#define NULL ((void *)0)

#endif
