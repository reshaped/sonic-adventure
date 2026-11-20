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

#ifdef NONMATCHING
/* a helper that inlines at every call. without always_inline, GCC's inliner
 * weighs each call at -O2 or -Os, and may leave one out of line. GCC inlines
 * only always_inline functions at -O0, which it takes where SHC takes
 * -optimize=0. a helper inlined there still calls out of line each SH4ZAM
 * function that is SHZ_INLINE, inline without always_inline */
#define ALWAYS_INLINE static inline __attribute__((always_inline))
#endif

#endif
