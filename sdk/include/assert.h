/* C's assert and static_assert, for GCC alone: SH4ZAM's headers, its library
 * and ninja_zam.h include this header. each inclusion defines assert anew by
 * NDEBUG as it then stands, as C requires, so the header has no include
 * guard. under NDEBUG, assert checks nothing. otherwise a false assertion
 * runs TRAPA #0xc3, GDB's breakpoint on SH, and runs it again each time the
 * trap's handler returns */
#undef assert
#ifdef NDEBUG
#define assert(expression) ((void)0)
#else
#define assert(expression)                                                     \
    ((expression) ? (void)0 : ({                                               \
        for (;;) {                                                             \
            __asm__ volatile("trapa #0xc3");                                   \
        }                                                                      \
    }))
#endif

/* C11's name for _Static_assert, which SH4ZAM's SHZ_CONVERT uses */
#define static_assert _Static_assert
