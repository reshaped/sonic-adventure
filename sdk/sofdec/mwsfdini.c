/* Sofdec's setup before syHwInit, which moves exceptions and interrupts to a
 * stack of their own */
#include <sg_xpt.h>
#include <shinobi.h>
#include <sofdec.h>

/* Shinobi's interrupt mode and stack, which syHwInit passes to syIntInit.
 * syhw.c defines sygIntMode const, which puts it in the constant section. this
 * declaration drops the const, and mwPlyPreInitSofdec writes it there: the
 * section is in RAM, so the store takes effect */
extern Uint32 sygIntMode;
extern void *sygIntStack;

Uint32 mwsfdg_intStack[MWSFD_INT_STACK_WORDS];

void mwPlyPreInitSofdec(void)
{
    sygIntMode = SYD_INT_MODE_INTSTACK;
    sygIntStack = mwsfdg_intStack + MWSFD_INT_STACK_WORDS;
}
