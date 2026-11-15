/* the port's stand-in for Flycast's core/oslib/virtmem.h: a block of
 * memory the DSP's recompiler writes its code to and runs. on Linux, a
 * mapping made writable and executable at once; on the Xbox, where a title
 * runs in ring 0 and its memory is all executable, the static buffer */
#pragma once
#include <cstddef>
#ifdef __linux__
#include <sys/mman.h>
#endif

#define DECLARE_CODE_CACHE(Name, Size) alignas(4096) static u8 Name[Size];

namespace virtmem
{
inline bool prepare_jit_block(void *code_area, size_t size, void **code_area_rwx)
{
#ifdef __linux__
    void *at = mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (at == MAP_FAILED) {
        return false;
    }
    *code_area_rwx = at;
#else
    *code_area_rwx = code_area;
#endif
    return true;
}

inline void release_jit_block(void *code_area, size_t size)
{
#ifdef __linux__
    munmap(code_area, size);
#else
    (void)code_area;
    (void)size;
#endif
}
}
