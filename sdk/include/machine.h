/* SHC intrinsic functions, as far as the sources use them */
#ifndef MACHINE_H
#define MACHINE_H

#ifdef GCC
/* under -m4-single-only, the FPU model config/gcc.mk's GCC_TARGET gives,
 * GCC's code takes FPSCR's PR and SZ clear. FR is clear as the CPU's reset
 * leaves it. GCC's side of set_fpscr and st_ext below relies on all three */
#ifndef __SH4_SINGLE_ONLY__
#error "GCC's side of the code takes -m4-single-only"
#endif
#endif

/* set_imask and get_imask write and read SR's interrupt mask. IMASK_ALL masks
 * every interrupt, and 0 masks none. get_vbr returns VBR, the base address of
 * the exception and interrupt handlers */
#ifdef __clang__
void _builtin_set_imask(int mask);
int _builtin_get_imask(void);
void *_builtin_get_vbr(void);
#endif
#ifdef GCC
/* SHC writes the three in place, and GCC always does: code that runs
 * uncached through P2, as mwsfd.c's switches of the cache do, can't call out
 * to P1. set_imask's LDC writes all of SR, T among it. no load or store moves
 * across the LDC, which opens or closes what the mask guards */
__attribute__((always_inline)) static inline void _builtin_set_imask(int mask)
{
    int sr;

    __asm__ volatile("stc sr,%0" : "=r"(sr));
    sr = (sr & ~0xf0) | ((mask & 15) << 4);
    __asm__ volatile("ldc %0,sr" : : "r"(sr) : "t", "memory");
}

__attribute__((always_inline)) static inline int _builtin_get_imask(void)
{
    int sr;

    __asm__ volatile("stc sr,%0" : "=r"(sr));
    return (sr >> 4) & 15;
}

__attribute__((always_inline)) static inline void *_builtin_get_vbr(void)
{
    void *vbr;

    __asm__ volatile("stc vbr,%0" : "=r"(vbr));
    return vbr;
}
#endif
#define set_imask(mask) _builtin_set_imask(mask)
#define get_imask() _builtin_get_imask()
#define get_vbr() _builtin_get_vbr()
#define IMASK_ALL 15

/* set_fpscr and get_fpscr write and read FPSCR, the FPU mode and status
 * register */
#ifdef __clang__
void _builtin_set_fpscr(int cr);
int _builtin_get_fpscr(void);
#endif
#ifdef GCC
/* GCC's set_fpscr keeps FPSCR's PR, SZ and FR as they are, where SHC's
 * writes them from cr. the two agree, since each cr the sources pass holds
 * the three clear. rfiber's FIBER_FPSCR clears them, and every other cr is
 * FPSCR as get_fpscr read it, _fmtout's less PR */
#define _builtin_set_fpscr __builtin_sh_set_fpscr
#define _builtin_get_fpscr __builtin_sh_get_fpscr
#endif
#define set_fpscr(cr) _builtin_set_fpscr(cr)
#define get_fpscr() _builtin_get_fpscr()

/* TAS.B: in one indivisible operation, set bit 7 of the byte at addr and
 * return whether the byte was 0 */
#ifdef __clang__
int _builtin_tas(char *addr);
#endif
#ifdef GCC
static inline int _builtin_tas(char *addr)
{
    int was_zero;

    __asm__ volatile("tas.b @%1\n\tmovt %0"
                     : "=r"(was_zero)
                     : "r"(addr)
                     : "t", "memory");
    return was_zero;
}
#endif
#define tas(addr) _builtin_tas(addr)

/* PREF: prefetch the cache line at p. at a store queue address, write that
 * store queue to memory */
#ifdef __clang__
void _builtin_prefetch(void *p);
#endif
#ifdef GCC
/* PREF at a store queue address writes out the stores before it, so no load
 * or store moves across it. GCC's __builtin_prefetch lets them move */
static inline void _builtin_prefetch(void *p)
{
    __asm__ volatile("pref @%0" : : "r"(p) : "memory");
}
#endif
#define prefetch(p) _builtin_prefetch(p)

/* store XMTRX at m: XF0 to XF15 in its 16 floats, in order, as eight pair
 * stores between two FSCHGs */
#ifdef __clang__
void _builtin_st_ext(float (*m)[4]);
#endif
#ifdef GCC
/* the pair stores run from m's end down, XD14 first, as SHC writes them */
static inline void _builtin_st_ext(float (*m)[4])
{
    float *end = (float *)(m + 4);

    __asm__ volatile("fschg\n\t"
                     "fmov xd14,@-%0\n\t"
                     "fmov xd12,@-%0\n\t"
                     "fmov xd10,@-%0\n\t"
                     "fmov xd8,@-%0\n\t"
                     "fmov xd6,@-%0\n\t"
                     "fmov xd4,@-%0\n\t"
                     "fmov xd2,@-%0\n\t"
                     "fmov xd0,@-%0\n\t"
                     "fschg"
                     : "+r"(end), "=m"(*(float (*)[16])m));
}
#endif
#define st_ext(m) _builtin_st_ext(m)

/* TRAPA #trap_no: trap unconditionally, with trap_no << 2 in TRA */
#ifdef __clang__
void _builtin_trapa(int trap_no);
#endif
#ifdef GCC
#define _builtin_trapa(trap_no)                                                \
    __asm__ volatile("trapa %0" : : "i"(trap_no) : "memory")
#endif
#define trapa(trap_no) _builtin_trapa(trap_no)

/* the string intrinsics, which SHC compiles to calls to the C runtime's
 * _slow_strcpy and _slow_strcmp1 */
#ifdef __clang__
char *_builtin_strcpy(char *s1, const char *s2);
int _builtin_strcmp(const char *s1, const char *s2);
#endif
#ifdef GCC
/* _slow_strcpy takes s1 in R0 and s2 in R1, outside C's calling convention.
 * it returns s1 in R0 and changes R1 and T alone. the asm moves s1 and s2
 * into R0 and R1 itself. a register variable bound to R0 would hold s1 there
 * up to the asm, and in sg_fhref.c that leaves GCC no R0 to spill with.
 * sdk/runtime/strcpy.c's strcpy calls _builtin_strcpy, so a call of strcpy
 * here would recurse */
void _slow_strcpy(void);
int _slow_strcmp1(const char *s1, const char *s2);

static inline char *_builtin_strcpy(char *s1, const char *s2)
{
    char *to;

    __asm__ volatile("mov %2,r1\n\tjsr @%3\n\tmov %1,r0\n\tmov r0,%0"
                     : "=r"(to)
                     : "r"(s1), "r"(s2), "r"(_slow_strcpy)
                     : "r0", "r1", "pr", "t", "memory");
    return to;
}

#define _builtin_strcmp _slow_strcmp1
#endif

#endif
