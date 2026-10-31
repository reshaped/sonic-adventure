/* C runtime memory and string functions, as far as the sources use them */
#ifndef STRING_H
#define STRING_H

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

#ifndef NULL
#define NULL ((void *)0)
#endif

void *memcpy(void *s1, const void *s2, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memset(void *s, int c, size_t n);
char *strcpy(char *s1, const char *s2);
char *strncpy(char *s1, const char *s2, size_t n);
int strcmp(const char *s1, const char *s2);
char *strcat(char *s1, const char *s2);
char *strchr(const char *s, int c);
size_t strlen(const char *s);
#ifdef GCC
/* GCC's memcpy and SH4ZAM's shz_memmove call it for blocks that overlap,
 * and GCC's code may call it where the C doesn't. sdk/runtime/memcpy.c
 * defines it from the shipped memcpy */
void *memmove(void *s1, const void *s2, size_t n);
#endif

#endif
