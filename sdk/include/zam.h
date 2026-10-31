/* what the tree's GCC code takes with SH4ZAM's headers: sg_xpt.h ahead of
 * them, checks of GCC's flags, and the macros its helpers share. each of the
 * tree's headers that reads SH4ZAM's includes this one first.
 *
 * XMTRX, XF0 to XF15 as a matrix, is state GCC doesn't see. GCC keeps
 * volatile asms in order among themselves, as it keeps volatile accesses,
 * and the tree's C relies on that order for XMTRX. so an asm that reads or
 * writes XMTRX must be volatile. each of SH4ZAM's XMTRX entries that the
 * tree calls runs its asm volatile */
#ifndef ZAM_H
#define ZAM_H
/* sg_xpt.h's enum bool doesn't parse after stdbool.h, which SH4ZAM's headers
 * include, so sg_xpt.h comes first */
#include <sg_xpt.h>
#include <sh4zam/shz_cdefs.h>
#if SHZ_BACKEND != SHZ_SH4
#error "the helpers run SH4ZAM's SH-4 back end, which -m4-single-only selects"
#endif
#ifdef __FAST_MATH__
#error "under -ffast-math, SH4ZAM's FSCA entries call sinf and cosf, not FSCA"
#endif
/* a helper that inlines at every call. without always_inline, GCC's inliner
 * weighs each call at -O2 or -Os, and may leave one out of line. GCC inlines
 * only always_inline functions at -O0, which it takes where SHC takes
 * -optimize=0. a helper inlined there still calls out of line each SH4ZAM
 * function that is SHZ_INLINE, inline without always_inline */
#define ALWAYS_INLINE static inline __attribute__((always_inline))
/* fail the compile unless zam_##name's type is compatible with name's. a
 * function-like macro of name calls the helper zam_##name in name's place,
 * so the two must agree. name must be declared above */
#define ZAM_ASSERT_COMPATIBLE(name)                                            \
    _Static_assert(__builtin_types_compatible_p(__typeof__(name),              \
                                                __typeof__(zam_##name)))
#endif
