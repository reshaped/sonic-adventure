/* C runtime nonlocal jumps, as far as the sources use them */
#ifndef SETJMP_H
#define SETJMP_H

/* the context setjmp saves, one register to a word. R15, R14 and PR come
 * first, then R13 down to R0. MACH, MACL and GBR follow, then FR0 to FR15,
 * FPUL and FPSCR. FR3 and FR5 take each other's words */
typedef int jmp_buf[38];

/* setjmp saves the context in env and returns 0. longjmp loads R15, PR,
 * FPSCR and each other register env holds but R0. it then returns to that PR
 * with val, or with 1 for a val of 0. the frame at env's R15 must still be
 * live, and FR3 and FR5 come back exchanged. setjmp and longjmp both need
 * FPSCR.SZ 0, and longjmp needs the FPSCR.FR bank setjmp ran under */
int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);

#endif
