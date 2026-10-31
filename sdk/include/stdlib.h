/* C runtime general utilities, as far as the sources use them */
#ifndef STDLIB_H
#define STDLIB_H

#ifdef GCC
/* stddef.h's size_t, unsigned int, which GCC's built-in functions and
 * SH4ZAM's headers declare with. the unsigned long below would conflict */
#include <stddef.h>
#define SIZE_T_DEFINED
#endif
#ifndef SIZE_T_DEFINED
#define SIZE_T_DEFINED
typedef unsigned long size_t;
#endif

/* the largest result rand gives */
#define RAND_MAX 32767

int rand(void);
void srand(unsigned int seed);
void *calloc(size_t nmemb, size_t size);
void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);

#endif
