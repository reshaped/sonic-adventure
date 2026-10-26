/* C runtime memory and string functions, as far as the sources use them */
#ifndef STRING_H
#define STRING_H

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

#endif
