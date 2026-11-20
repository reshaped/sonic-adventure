/* the PSP reads a program as the Dreamcast does: the loader's read writes
 * the program's bytes over the last program at its address. the shim's
 * prs_decompress has decoded the disc's file there, the console's program,
 * and calls native_program_read with the file's name. the build's program of
 * that name, in programs/, then takes its place: its image, zeros for its
 * bss, and its names. a file of the disc that is no program has none there,
 * and its decoded bytes stand */
#include <ctype.h>
#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "names.h"
#include "native.h"
#include "psp.h"

void port_stop(const char *what, const char *name, uint32_t number)
    __attribute__((noreturn));

/* the files of the disc that hold addresses of their own bytes, as the
 * console loads them, and that the build makes no program of: the fields'
 * messages, whose rows point at their lines, and the shooting stages'
 * paths, whose scenes point at their points. each is known by a part of
 * its name. no rule tells such a word from a number in a file of pictures
 * or of sound, so no other file is touched */
static int holds_own_addresses(const char *name)
{
    const char *base = strrchr(name, '/');

    base = base ? base + 1 : name;
    if (!strncasecmp(base, "shtpath", 7)) {
        return 1;
    }
    for (; *base; base++) {
        if (!strncasecmp(base, "_mes_", 5)) {
            return 1;
        }
    }
    return 0;
}

/* the console's address of console_ram's byte at at */
static uint32_t console_address(const char *at)
{
    return 0x0c000000U + CONSOLE_RAM_FIRST + (uint32_t)(at - console_ram);
}

void native_file_placed(const char *name, void *at, uint32_t bytes)
{
    char *start = at;
    uint32_t first, moved = 0;

    if (!holds_own_addresses(name) || start < console_ram ||
        bytes > CONSOLE_RAM_SIZE ||
        (size_t)(start - console_ram) > CONSOLE_RAM_SIZE - bytes ||
        (uintptr_t)start % 4 != 0) {
        return;
    }
    /* a word that is the console's address of one of the file's bytes
     * becomes the PSP's address of that byte */
    first = console_address(start);
    for (uint32_t i = 0; i + 4 <= bytes; i += 4) {
        uint32_t word;

        memcpy(&word, start + i, sizeof word);
        if (word - first < bytes) {
            word = (uint32_t)(uintptr_t)(start + (word - first));
            memcpy(start + i, &word, sizeof word);
            moved++;
        }
    }
    if (native_trace) {
        fprintf(stderr, "sa1: %s holds %u addresses of its own bytes\n", name,
                (unsigned)moved);
    }
}

void native_program_read(const char *file)
{
    char path[96] = "programs/";
    struct program_head head;
    size_t at = strlen(path);
    const char *dot = strrchr(file, '.');
    size_t stem = dot ? (size_t)(dot - file) : strlen(file);
    FILE *program;
    char *image;

    for (size_t i = 0; i < stem && at + 5 < sizeof path; i++) {
        path[at++] = (char)toupper((unsigned char)file[i]);
    }
    strcpy(path + at, ".BIN");
    if (!(program = native_buffered(fopen(path, "rb")))) {
        return;
    }
    if (native_trace) {
        fprintf(stderr, "sa1: program %s, at vsync %lu\n", path, native_vsyncs);
    }
    if (fread(&head, sizeof head, 1, program) != 1 ||
        head.magic != PROGRAM_MAGIC) {
        port_stop("PROGRAM", file, 0);
    }
    /* the link placed the program for a base, and its words hold addresses
     * of the program's own beside it */
    if (head.base != psp_base()) {
        port_stop("BASE", file, psp_base());
    }
    image = (char *)(uintptr_t)head.origin;
    if (image < console_ram || head.image_bytes > CONSOLE_RAM_SIZE ||
        head.bss_bytes > CONSOLE_RAM_SIZE - head.image_bytes ||
        (size_t)(image - console_ram) >
            CONSOLE_RAM_SIZE - head.image_bytes - head.bss_bytes) {
        port_stop("PLACE", file, head.origin);
    }
    if (fread(image, 1, head.image_bytes, program) != head.image_bytes) {
        port_stop("READ", file, head.image_bytes);
    }
    memset(image + head.image_bytes, 0, head.bss_bytes);
    /* the image holds code, and the last program's lay at the same
     * addresses: the processor's data cache may still hold some of the new
     * bytes, and its instruction cache the last program's code. the new
     * bytes go to memory, and the instruction cache gives up all it holds */
    sceKernelDcacheWritebackAll();
    sceKernelIcacheInvalidateAll();
    psp_names_forget(head.origin,
                     head.origin + head.image_bytes + head.bss_bytes);
    psp_names_read(program, head.base, 0);
    fclose(program);
}
