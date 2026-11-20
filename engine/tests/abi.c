/* the C ABI's header as a client meets it. engine-test compiles this as C17
 * and as C++17 and runs each. where SH_ELF's GCC is in place, it compiles
 * this for SH-4 too. there the header's asserts hold each record's size on
 * a 32-bit target. main returns 0, or 1 plus the index of the first name
 * whose id isn't FNV-1a's */
#include "sa_engine.h"

static const struct {
    const char *name;
    uint32_t id;
} names[] = {
    /* FNV-1a's own test vectors */
    {"", 0x811c9dc5U},
    {"a", 0xe40c292cU},
    {"foobar", 0xbf9cf968U},
};

int main(void)
{
    int i;

    for (i = 0; i < (int)(sizeof names / sizeof names[0]); i++) {
        if (sa_name_id(names[i].name) != names[i].id) {
            return i + 1;
        }
    }
    return 0;
}
