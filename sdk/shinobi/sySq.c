/* the SH-4's store queues */
#include <sg_sq.h>
#include <sg_xpt.h>

/* the MMU control register, and its bit that enables address translation */
#define MMUCR (*(Uint32 *)0xff000010)
#define MMUCR_AT 1
/* the store queue area registers. with translation off, each holds bits 28
 * to 26 of its queue's target address, TARGET_AREA, in its field QACR_AREA.
 * AREA_BITS are those three bits shifted down */
#define QACR0 (*(Uint32 *)0xff000038)
#define QACR1 (*(Uint32 *)0xff00003c)
#define QACR_AREA 0x1c
#define QACR_AREA_SHIFT 2
#define TARGET_AREA 0x1c000000
#define TARGET_AREA_SHIFT 26
#define AREA_BITS 7
/* UTLB entry 57, which sySqStart takes for the queues' page, in the address
 * and data arrays */
#define UTLB_ADDRESS (*(Uint32 *)0xf6003900)
#define UTLB_DATA (*(Uint32 *)0xf7003900)

/* a queue's address: the target address, aligned to 32 bytes, in the store
 * queue area at 0xe0000000 */
#define SQ_BASE 0xe0000000
#define SQ_MASK 0x1fffffe0
/* the entry maps a 1 MB page: valid and dirty in the address array, and
 * valid, 1 MB and shared in the data array */
#define PAGE_MASK 0xfffffc00
#define PAGE_SIZE 0x100000
#define UTLB_ADDRESS_FLAGS 0x300
#define UTLB_DATA_FLAGS 0x192
/* the data array's physical page number, bits 28 to 10 */
#define UTLB_PPN 0x1ffffc00
/* *size with translation off, where no page bounds the queues: the largest
 * 29-bit physical address */
#define UNBOUNDED_SIZE 0x1fffffff

const char *sygSqBuild = "\r\nsySq Ver 1.00 Build:Sep 16 1998 14:38:15\r\n";
const Uint32 sygSqVer = 1;

void sySqInit(void)
{
}

void sySqFinish(void)
{
}

void *sySqStart(Uint32 *size, void *dstArray)
{
    Uint32 queue = ((Uint32)dstArray & SQ_MASK) | SQ_BASE;
    Sint32 area;

    if ((MMUCR & MMUCR_AT) == MMUCR_AT) {
        UTLB_ADDRESS = (queue & PAGE_MASK) | UTLB_ADDRESS_FLAGS;
        UTLB_DATA = ((Uint32)dstArray & UTLB_PPN) | UTLB_DATA_FLAGS;
        *size = PAGE_SIZE - (queue & (PAGE_SIZE - 1));
    } else {
        /* @hack spelling: area and the first store in one statement, which
         * loads each register's address from the pool (MOV.L at 0x8c65f738
         * and 0x8c65f73c, and at 0x8ce0ce0c and 0x8ce0ce10 in LOADER.BIN). as
         * two statements, QACR1's address is QACR0's plus 4, the function is
         * 8 bytes shorter and 34 of 50 words differ */
        area = ((((Sint32)dstArray & TARGET_AREA) >> TARGET_AREA_SHIFT) &
                AREA_BITS)
               << QACR_AREA_SHIFT,
        QACR0 = (QACR0 & ~QACR_AREA) | area;
        QACR1 = (QACR1 & ~QACR_AREA) | area;
        *size = UNBOUNDED_SIZE;
    }
    return (void *)queue;
}
