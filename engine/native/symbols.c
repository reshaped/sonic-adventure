/* read the executable's own symbols into names.h's list */
#include <elf.h>
#include <fcntl.h>
#include <link.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "names.h"
#include "native.h"

/* a symbol's type sits in the same bits in either class */
#define ST_TYPE(info) ((info) & 0xf)

/* whether bytes from offset on lie in a file of size bytes */
static int within(uint64_t offset, uint64_t bytes, uint64_t size)
{
    return offset <= size && bytes <= size - offset;
}

/* section i, or null unless it is a symbol table that lies in the image
 * with its strings. the image must hold its section headers. a string table
 * must end in a NUL, or a name could run past the image */
static const ElfW(Shdr) * symbol_table(const uint8_t *image, size_t size, int i)
{
    const ElfW(Ehdr) *elf = (const ElfW(Ehdr) *)image;
    const ElfW(Shdr) *sections = (const ElfW(Shdr) *)(image + elf->e_shoff);
    const ElfW(Shdr) * names;

    if (sections[i].sh_type != SHT_SYMTAB ||
        sections[i].sh_link >= elf->e_shnum) {
        return NULL;
    }
    names = &sections[sections[i].sh_link];
    return within(sections[i].sh_offset, sections[i].sh_size, size) &&
                   within(names->sh_offset, names->sh_size, size) &&
                   names->sh_size &&
                   !image[names->sh_offset + names->sh_size - 1]
               ? &sections[i]
               : NULL;
}

/* add the symbols of the ELF file at path. the result is 0 where it adds
 * them. a file that can't be read gives -1, and one with no symbol table 1.
 * neither adds a name. names_symbol answers a name's first object added,
 * and more than one source has a task_levels. so the first pass adds the
 * objects that follow task.c's STT_FILE, and the second the rest */
static int read_file(const char *path)
{
    int fd = open(path, O_RDONLY);
    struct stat st;
    int sized = fd >= 0 && fstat(fd, &st) == 0;
    size_t size = sized ? (size_t)st.st_size : 0;
    const uint8_t *image = MAP_FAILED;
    const ElfW(Ehdr) * elf;
    const ElfW(Shdr) * sections;
    int tables = 0;

    /* mmap refuses a length of 0. an empty file reads as one with no table */
    if (size) {
        image = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    }
    if (fd >= 0) {
        close(fd);
    }
    if (!sized || (size && image == MAP_FAILED)) {
        return -1;
    }
    elf = (const ElfW(Ehdr) *)image;
    if (size >= sizeof *elf &&
        within(elf->e_shoff, (uint64_t)elf->e_shnum * sizeof *sections, size)) {
        for (int i = 0; i < elf->e_shnum; i++) {
            tables += symbol_table(image, size, i) != NULL;
        }
    }
    if (!tables) {
        if (size) {
            munmap((void *)image, size);
        }
        return 1;
    }
    /* each name stays in the image, which stays mapped */
    sections = (const ElfW(Shdr) *)(image + elf->e_shoff);
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < elf->e_shnum; i++) {
            const ElfW(Shdr) *table = symbol_table(image, size, i);
            const ElfW(Shdr) * strings;
            const ElfW(Sym) * symbols;
            const char *names;
            size_t count;
            int in_task_c = 0;

            if (!table) {
                continue;
            }
            strings = &sections[table->sh_link];
            symbols = (const ElfW(Sym) *)(image + table->sh_offset);
            names = (const char *)(image + strings->sh_offset);
            count = table->sh_size / sizeof(ElfW(Sym));
            for (size_t j = 0; j < count; j++) {
                const char *name = names + symbols[j].st_name;
                int type = ST_TYPE(symbols[j].st_info);
                int task_static = in_task_c && type == STT_OBJECT;

                if (symbols[j].st_name >= strings->sh_size) {
                    continue;
                }
                if (type == STT_FILE) {
                    in_task_c = !strcmp(name, "task.c");
                } else if ((type == STT_FUNC || type == STT_OBJECT) &&
                           symbols[j].st_value && task_static == !pass) {
                    names_symbol_add((uint32_t)symbols[j].st_value,
                                     (uint32_t)symbols[j].st_size,
                                     type == STT_FUNC, name);
                }
            }
        }
    }
    names_symbols_ready();
    return 0;
}

/* a read that fails leaves the symbols unread, so the next call reads
 * again */
int native_read_symbols(void)
{
    static int done;
    int failed = done ? 0 : read_file("/proc/self/exe");

    done = !failed;
    return failed;
}

uint32_t native_name_id(void *function)
{
    return names_name_id((uint32_t)(uintptr_t)function);
}

char *native_symbol(const char *name)
{
    native_read_symbols();
    return native_widen(names_symbol(name));
}
