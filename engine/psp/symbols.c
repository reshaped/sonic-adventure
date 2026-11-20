/* read the build's names into names.h's list: the program's own from
 * names.bin, and a program's as loads.c reads its file. the list holds the
 * program's own names and those of each program that lies loaded, so a load
 * over a program's bytes makes the list again without that program's */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../native/names.h"
#include "names.h"
#include "native.h"
#include "psp.h"

/* a table read, which the list is made from again */
struct table {
    struct names_row *rows;
    char *text;
    uint32_t count, start, end;
    int kept;
    struct table *next;
};
static struct table *tables;
static uint32_t base;

uint32_t psp_base(void)
{
    return base;
}

static void make_list(void)
{
    names_symbols_clear();
    /* the program's own first, as names_symbol answers the first added */
    for (int kept = 1; kept >= 0; kept--) {
        for (const struct table *t = tables; t; t = t->next) {
            for (uint32_t i = 0; t->kept == kept && i < t->count; i++) {
                names_symbol_add(t->rows[i].address, t->rows[i].size,
                                 (int)t->rows[i].code,
                                 t->text + t->rows[i].name);
            }
        }
    }
    names_symbols_ready();
}

int psp_names_read(FILE *file, uint32_t moved, int kept)
{
    struct names_head head;
    struct table *t;

    if (fread(&head, sizeof head, 1, file) != 1 || head.magic != NAMES_MAGIC ||
        !(t = calloc(1, sizeof *t))) {
        return -1;
    }
    t->rows = malloc((size_t)head.count * sizeof *t->rows + 1);
    t->text = malloc((size_t)head.text_bytes + 1);
    if (!t->rows || !t->text ||
        fread(t->rows, sizeof *t->rows, head.count, file) != head.count ||
        fread(t->text, 1, head.text_bytes, file) != head.text_bytes ||
        (head.text_bytes && t->text[head.text_bytes - 1])) {
        free(t->rows);
        free(t->text);
        free(t);
        return -1;
    }
    if (kept) {
        moved = (uint32_t)(uintptr_t)console_ram - head.console;
        base = moved;
    }
    /* a program none of whose objects is found by name adds nothing */
    if (head.count == 0 && !kept) {
        free(t->rows);
        free(t->text);
        free(t);
        return 0;
    }
    t->count = head.count;
    t->kept = kept;
    t->start = UINT32_MAX;
    for (uint32_t i = 0; i < head.count; i++) {
        if (t->rows[i].name >= head.text_bytes) {
            t->rows[i].name = 0;
        }
        t->rows[i].address += moved;
        t->start =
            t->rows[i].address < t->start ? t->rows[i].address : t->start;
        t->end =
            t->rows[i].address + 1 > t->end ? t->rows[i].address + 1 : t->end;
    }
    t->next = tables;
    tables = t;
    make_list();
    return 0;
}

void psp_names_forget(uint32_t start, uint32_t end)
{
    for (struct table **at = &tables; *at;) {
        struct table *t = *at;

        if (!t->kept && t->start < end && start < t->end) {
            *at = t->next;
            free(t->rows);
            free(t->text);
            free(t);
        } else {
            at = &t->next;
        }
    }
    make_list();
}

/* a read that fails leaves the names unread, so the next call reads again */
int native_read_symbols(void)
{
    static int done;
    FILE *file;

    if (!done && (file = native_buffered(fopen("names.bin", "rb")))) {
        done = psp_names_read(file, 0, 1) == 0;
        fclose(file);
    }
    return done ? 0 : -1;
}

/* the PSP's names hold no function: a function's id is where it lies in the
 * program, which names it for a run */
uint32_t native_name_id(void *function)
{
    return function ? (uint32_t)(uintptr_t)function - base : 0;
}

char *native_symbol(const char *name)
{
    native_read_symbols();
    return native_widen(names_symbol(name));
}
