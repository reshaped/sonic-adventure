/* the PSP build's names, as psplink.py writes them and symbols.c and loads.c
 * read them: names.bin holds the program's own, and each file of programs/
 * its program's after its image. a table is its head, its rows, then the
 * names' text, each ending in a NUL. an address counts from the program's
 * base, where the system loaded it: console_ram's row gives that */
#ifndef PSP_NAMES_H
#define PSP_NAMES_H
#include <stdint.h>
#include <stdio.h>

#define NAMES_MAGIC 0x4d4e4153U /* SANM */
#define PROGRAM_MAGIC 0x52504153U /* SAPR */

struct names_head {
    uint32_t magic, count, text_bytes;
    /* console_ram's address, by which a reader finds the base */
    uint32_t console;
};

struct names_row {
    uint32_t address, size, code, name;
};

/* a file of programs/: its head, image_bytes of its image, which load at
 * origin, bss_bytes of zeros after them, then its table of names. the link
 * placed the image for a program loaded at base */
struct program_head {
    uint32_t magic, base, origin, image_bytes, bss_bytes;
};

/* the program's base, or 0 before native_read_symbols has read names.bin */
uint32_t psp_base(void);
/* a table's rows, read from file, each address moved by base. it returns 0
 * where it reads them all. kept is nonzero for the program's own, which
 * each later load adds again: a program's rows last until the next load */
int psp_names_read(FILE *file, uint32_t base, int kept);
/* forget the loaded programs' rows that lie in the bytes from start to end,
 * and make the list ready again */
void psp_names_forget(uint32_t start, uint32_t end);

#endif
