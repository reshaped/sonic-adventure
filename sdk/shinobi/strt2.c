/* the startup that strt1.src jumps to. _INIT fills the stack, sets up the
 * data and bss sections and adds the program's libraries to the library
 * table. it then runs main, and removes the libraries when main returns */
#include <sg_ov.h>
#include <sg_xpt.h>

/* the stack's lowest byte and the byte past its top, and the word _INIT
 * fills it with: "SEGA" in memory order */
#define STACK_BOTTOM 0x8c00c000
#define STACK_TOP 0x8c00f400
#define STACK_FILL 0x41474553

extern const char *__0ShinobiVersion;
/* the program's module, at DSGLH's start, as strt1.src's table gives it */
extern OVMDL _DSGLH_BGN;
extern Uint8 *_B_BGN, *_B_END;
extern Uint8 *_D_BGN, *_D_END, *_D_ROM;
extern Uint8 *_BSG_BGN, *_BSG_END;
extern Uint8 *_DSG_BGN, *_DSG_END, *_DSG_ROM;
extern void main(void);

void _INITLIBH(void);
void _FINISHLIBH(void);
void _INITSCT(void);

char *sygStartBuild = "\nsyStart Ver 1.02 Build:Oct 26 1998 19:51:11\n";

void _INIT(void)
{
    Uint32 *stack_word;

    /* reference the version string so that it links */
    /* @bug memory: the read takes the string's first four bytes as the address
     * 0x2d2d2d0a, and reads the byte there (MOV.L @R2,R3 and MOV.B @R3,R1 at
     * 0x8c0100b0 and 0x8c0100b2, 0x8ce000b0 and 0x8ce000b2 in LOADER.BIN). the
     * extern above declares __0ShinobiVersion a pointer, though _0sbver.c
     * defines an array */
    (void)*(volatile const char *)__0ShinobiVersion;
    for (stack_word = (Uint32 *)STACK_BOTTOM; stack_word < (Uint32 *)STACK_TOP;
         ++stack_word) {
        *stack_word = STACK_FILL;
    }
    _INITSCT();
    _INITLIBH();
    main();
    _FINISHLIBH();
}

void _INITLIBH(void)
{
    ovMdlEntryTbl(_DSGLH_BGN);
}

void _FINISHLIBH(void)
{
    ovMdlDeleteTbl(_DSGLH_BGN);
}

/* zero B through B32 and BSG through BSG32. copy the initial data into R
 * through R32 from D on, and into RSG through RSG32 from DSG on. strt1.src's
 * tables give the bounds */
void _INITSCT(void)
{
    Uint8 *destination;
    Uint8 *initial;

    for (destination = _B_BGN; destination < _B_END; ++destination) {
        *destination = 0;
    }
    for (destination = _D_BGN, initial = _D_ROM; destination < _D_END;
         ++destination, ++initial) {
        *destination = *initial;
    }
    for (destination = _BSG_BGN; destination < _BSG_END; ++destination) {
        *destination = 0;
    }
    for (destination = _DSG_BGN, initial = _DSG_ROM; destination < _DSG_END;
         ++destination, ++initial) {
        *destination = *initial;
    }
}
