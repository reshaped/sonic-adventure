#include <string.h>
#ifdef GCC
#include <zam.h>
#include <sh4zam/shz_mem.h>
#include <stdbool.h>
#include <stdint.h>

/* system RAM is area 3. an address holds 3 in bits 26 to 28 and 0 in bit
 * 30 exactly when it reaches system RAM through P0 below 0x40000000, P1 or
 * P2. the test of bit 30 refuses P4, and with it system RAM through P3 */
#define SYSTEM_RAM_BITS 0x5c000000U
#define SYSTEM_RAM 0x0c000000U
/* the fewest bytes memset sets by set_pairs. from that many on, set_pairs
 * runs fewer instructions than memset's loop, wherever s starts */
#define SET_PAIRS_FROM 12
/* the bytes of a pair of longs, which shz_memset8 sets whole, from an
 * address a multiple of PAIR_BYTES */
#define PAIR_BYTES 8
_Static_assert(SET_PAIRS_FROM >= PAIR_BYTES - 1,
               "set_pairs needs n of PAIR_BYTES - 1 or more");

static inline bool in_system_ram(uintptr_t address)
{
    return (address & SYSTEM_RAM_BITS) == SYSTEM_RAM;
}

/* shz_memset8's 64-bit value as two equal longs. GCC builds this union with
 * one MOV, and (uint64_t)word << 32 | word with a DMULU */
union pair {
    uint64_t value;
    uint32_t longs[2];
};

/* set n bytes from s, n PAIR_BYTES - 1 or more. shz_memset8 sets the whole
 * pairs. byte, half and long stores set the head and the tail, the bytes
 * before the first pair and after the last. each of those stores lies on a
 * boundary of its size. for such an n, the head and the tail take n bytes at
 * most, so they never overlap. they are set before the call, so only s lives
 * across it. noinline keeps memset from saving registers for a short set */
static __attribute__((noinline)) void *set_pairs(void *s, int c, size_t n)
{
    unsigned char *to = s;
    unsigned char *end = to + n;
    unsigned char byte = (unsigned char)c;
    uint32_t word = byte * 0x01010101U;
    union pair pair = {.longs = {word, word}};
    size_t head = -(uintptr_t)to % PAIR_BYTES;
    size_t tail = (uintptr_t)end % PAIR_BYTES;

    if (head & 1) {
        *to++ = byte;
    }
    if (head & 2) {
        *(uint16_t *)to = (uint16_t)word;
        to += 2;
    }
    if (head & 4) {
        *(uint32_t *)to = word;
        to += 4;
    }
    if (tail & 1) {
        *--end = byte;
    }
    if (tail & 2) {
        end -= 2;
        *(uint16_t *)end = (uint16_t)word;
    }
    if (tail & 4) {
        end -= 4;
        *(uint32_t *)end = word;
    }
    shz_memset8(to, pair.value, end - to);
    return s;
}
#endif

void *memset(void *s, int c, size_t n)
{
    size_t i;
    unsigned char *to = s;

#ifdef GCC
    /* only a block in system RAM is set by pairs. shz_memset8's stores are
     * 64-bit, and a 32-bit register isn't known to take them */
    if (n >= SET_PAIRS_FROM && in_system_ram((uintptr_t)s)) {
        return set_pairs(s, c, n);
    }
#endif
    for (i = 0; i < n; ++i) {
        *to++ = (unsigned char)c;
    }
    return s;
}
