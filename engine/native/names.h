/* a build's names: each function and object, from its own symbols. the
 * adapter and the shim find the game's statics by name here. the adapter
 * also names a task's functions by them */
#ifndef NAMES_H
#define NAMES_H
#include <stdint.h>

/* id is the name's sa_name_id, and added counts the names added before it.
 * code is nonzero for a function */
struct symbol {
    uint32_t address, size, id, added;
    int code;
    const char *name;
};

/* add a name. call names_symbols_ready after the last, before a search by
 * address. it sorts the names, and stretches each of size 0 to the next
 * address */
void names_symbol_add(uint32_t address, uint32_t size, int code,
                      const char *name);
/* forget every name, for a list made again */
void names_symbols_clear(void);
void names_symbols_ready(void);
/* an object's address by its name, or 0: the first such object added. a
 * function that shares the name holds none of the game's data */
uint32_t names_symbol(const char *name);
/* the sa_name_id of the name whose bytes hold function. it returns 0 where
 * function is 0 or no name holds it */
uint32_t names_name_id(uint32_t function);
/* the name whose bytes hold address, or null */
const struct symbol *names_symbol_at(uint32_t address);

#endif
