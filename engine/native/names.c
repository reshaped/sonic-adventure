/* the list of names a program adds its symbols to (names.h), found by
 * address or by name */
#include <stdlib.h>
#include <string.h>

#include "names.h"
#include "native.h"
#include "sa_sa1.h"

static struct symbol *symbols;
static uint32_t symbol_count, symbol_room;

void names_symbol_add(uint32_t address, uint32_t size, int code,
                      const char *name)
{
    if (symbol_count == symbol_room) {
        symbol_room = symbol_room ? symbol_room * 2 : 1 << 16;
        symbols = native_memory(realloc(symbols, symbol_room * sizeof *symbols),
                                "the program's symbols");
    }
    symbols[symbol_count] = (struct symbol){
        address, size, sa_name_id(name), symbol_count, code, name};
    symbol_count++;
}

void names_symbols_clear(void)
{
    symbol_count = 0;
}

static int by_address(const void *a, const void *b)
{
    const struct symbol *x = a, *y = b;

    /* a sized name ahead of a label at its address */
    if (x->address != y->address) {
        return x->address < y->address ? -1 : 1;
    }
    return x->size > y->size ? -1 : x->size < y->size;
}

void names_symbols_ready(void)
{
    /* a list never added to has no memory, and qsort takes no null */
    if (!symbol_count) {
        return;
    }
    qsort(symbols, symbol_count, sizeof *symbols, by_address);
    for (uint32_t i = 0; i + 1 < symbol_count; i++) {
        uint32_t next = i + 1;

        while (next + 1 < symbol_count &&
               symbols[next].address == symbols[i].address) {
            next++;
        }
        if (!symbols[i].size) {
            symbols[i].size = symbols[next].address - symbols[i].address;
        }
    }
}

uint32_t names_symbol(const char *name)
{
    const struct symbol *first = NULL;

    for (uint32_t i = 0; i < symbol_count; i++) {
        if (!symbols[i].code && !strcmp(symbols[i].name, name) &&
            (!first || symbols[i].added < first->added)) {
            first = &symbols[i];
        }
    }
    return first ? first->address : 0;
}

const struct symbol *names_symbol_at(uint32_t address)
{
    uint32_t low = 0, high = symbol_count;

    while (low < high) {
        uint32_t middle = (low + high) / 2;

        if (symbols[middle].address <= address) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    /* the first of the names at that address, which is the sized one */
    while (low > 1 && symbols[low - 1].address == symbols[low - 2].address) {
        low--;
    }
    if (low && address - symbols[low - 1].address < symbols[low - 1].size) {
        return &symbols[low - 1];
    }
    return NULL;
}

uint32_t names_name_id(uint32_t function)
{
    const struct symbol *found = function ? names_symbol_at(function) : NULL;

    return found ? found->id : 0;
}
