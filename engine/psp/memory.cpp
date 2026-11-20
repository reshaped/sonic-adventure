/* the C++ library's operator new, for the PSP: a request the heap can't
 * meet stops the game with the bytes asked, the heap's bytes in use and the
 * caller, where the library's own would end the program with no word of
 * why. the engine's C++ takes no exception, so no caller could go on */
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <new>

extern "C" void port_stop(const char *what, const char *name, uint32_t number)
    __attribute__((noreturn));

namespace {

void *taken(size_t bytes, void *caller)
{
    void *block = malloc(bytes != 0 ? bytes : 1);

    if (block == nullptr) {
        char words[128];
        struct mallinfo heap = mallinfo();

        snprintf(words, sizeof words,
                 "%u BYTES, %u IN USE AND %u FREE OF THE HEAP'S %u, FOR",
                 static_cast<unsigned>(bytes),
                 static_cast<unsigned>(heap.uordblks),
                 static_cast<unsigned>(heap.fordblks),
                 static_cast<unsigned>(heap.arena));
        port_stop("NO MEMORY:", words,
                  static_cast<uint32_t>(reinterpret_cast<uintptr_t>(caller)));
    }
    return block;
}

} // namespace

void *operator new(size_t bytes)
{
    return taken(bytes, __builtin_return_address(0));
}

void *operator new[](size_t bytes)
{
    return taken(bytes, __builtin_return_address(0));
}

void operator delete(void *block) noexcept
{
    free(block);
}

void operator delete[](void *block) noexcept
{
    free(block);
}

void operator delete(void *block, size_t) noexcept
{
    free(block);
}

void operator delete[](void *block, size_t) noexcept
{
    free(block);
}
