/* on the Dreamcast, each program's read writes its data over the last program
 * at its address. it zeros the bss only where the file stores the bss, as some
 * files do. the native slice links every program at once, with its data and
 * bss in sections that native.mk names. native_program_read puts back the data
 * the program started with, and zeros its bss on every read */
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "native.h"

static char **started;

void native_programs_keep(void)
{
    static const char kept[] = "the data the programs started with";

    started = native_memory(
        calloc((size_t)native_program_count, sizeof *started), kept);
    for (int i = 0; i < native_program_count; i++) {
        const struct native_program *p = &native_programs[i];
        size_t size = (size_t)(p->data_end - p->data);

        if (size) {
            started[i] = native_memory(malloc(size), kept);
            memcpy(started[i], p->data, size);
        }
    }
}

void native_program_read(const char *file)
{
    for (int i = 0; started && i < native_program_count; i++) {
        const struct native_program *p = &native_programs[i];

        if (!strcasecmp(file, p->file)) {
            /* a program of no data kept none, and memcpy takes no null */
            if (started[i]) {
                memcpy(p->data, started[i], (size_t)(p->data_end - p->data));
            }
            memset(p->bss, 0, (size_t)(p->bss_end - p->bss));
            return;
        }
    }
}

/* marks.py's constructors call this. on the SH-4, object holds address in a
 * 32-bit word, where marks.py puts mark instead */
void native_address32(void *object, unsigned long size, int mark, void *address)
{
    int32_t *word = NULL;

    for (unsigned long at = 0; at + 4 <= size; at += 4) {
        int32_t *here = (int32_t *)((char *)object + at);

        if (*here == mark) {
            if (word) {
                fprintf(stderr, "sa1: mark %#x twice in a table\n", mark);
                exit(2);
            }
            word = here;
        }
    }
    if (!word) {
        fprintf(stderr, "sa1: mark %#x in no table\n", mark);
        exit(2);
    }
    *word = (int32_t)(uintptr_t)address;
}
