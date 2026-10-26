/* SHC intrinsic functions, as far as the sources use them */
#ifndef MACHINE_H
#define MACHINE_H

/* set_imask and get_imask write and read SR's interrupt mask. IMASK_ALL masks
 * every interrupt, and 0 masks none. get_vbr returns VBR, the base address of
 * the exception and interrupt handlers */
#ifdef __clang__
void _builtin_set_imask(int mask);
int _builtin_get_imask(void);
void *_builtin_get_vbr(void);
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
#define set_fpscr(cr) _builtin_set_fpscr(cr)
#define get_fpscr() _builtin_get_fpscr()

/* TAS.B: in one indivisible operation, set bit 7 of the byte at addr and
 * return whether the byte was 0 */
#ifdef __clang__
int _builtin_tas(char *addr);
#endif
#define tas(addr) _builtin_tas(addr)

/* PREF: prefetch the cache line at p. at a store queue address, write that
 * store queue to memory */
#ifdef __clang__
void _builtin_prefetch(void *p);
#endif
#define prefetch(p) _builtin_prefetch(p)

/* store XMTRX at m: XF0 to XF15 in its 16 floats, in order, as eight pair
 * stores between two FSCHGs */
#ifdef __clang__
void _builtin_st_ext(float (*m)[4]);
#endif
#define st_ext(m) _builtin_st_ext(m)

/* TRAPA #trap_no: trap unconditionally, with trap_no << 2 in TRA */
#ifdef __clang__
void _builtin_trapa(int trap_no);
#endif
#define trapa(trap_no) _builtin_trapa(trap_no)

/* the string intrinsics, which SHC compiles to calls to the C runtime's
 * _slow_strcpy and _slow_strcmp1 */
#ifdef __clang__
char *_builtin_strcpy(char *s1, const char *s2);
int _builtin_strcmp(const char *s1, const char *s2);
#endif

#endif
