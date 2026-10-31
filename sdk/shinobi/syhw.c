/* hardware initialization and shutdown */
#include <mpdrv.h>
#include <sg_chain.h>
#include <sg_sq.h>
#include <sg_syg2.h>
#include <sg_syhw.h>
#include <sg_xpt.h>
#include <shinobi.h>

struct register_setting {
    Uint32 address;
    Uint32 value;
};

/* a system bus register, through P2 */
#define SB(offset) (0xa05f0000UL + (offset))

/* the CPU to tile accelerator DMA on channel 2, and the sort DMA */
#define CH2_DMA_DESTINATION SB(0x6800)
#define CH2_DMA_LENGTH SB(0x6804)
#define CH2_DMA_START SB(0x6808)
#define SORT_DMA_TABLE SB(0x6810)
#define SORT_DMA_LINK_BASE SB(0x6814)
#define SORT_DMA_LINK_WIDTH SB(0x6818)
#define SORT_DMA_LINK_SHIFT SB(0x681c)
#define SORT_DMA_START SB(0x6820)
#define DMA_REQUEST_MASK SB(0x6840)
#define BUS_WAIT_COUNT SB(0x6844)
#define CH2_DMA_PRIORITY SB(0x6848)
#define CH2_DMA_BURST SB(0x684c)
#define TA_PATH_MODE0 SB(0x6884)
#define TA_PATH_MODE1 SB(0x6888)
#define ROOT_BUS_SPLIT SB(0x68a0)
#define SB_UNK_68A4 SB(0x68a4)
#define SB_UNK_68AC SB(0x68ac)

/* the interrupt registers: status, then the masks of levels 2, 4 and 6, of
 * PVR DMA triggers and of G2 DMA triggers. each has a word for normal and
 * one for external interrupts, and status and the level masks one for error
 * interrupts */
#define INT_STATUS SB(0x6900)
#define INT_LEVEL2 SB(0x6910)
#define INT_LEVEL4 SB(0x6920)
#define INT_LEVEL6 SB(0x6930)
#define INT_PVR_DMA SB(0x6940)
#define INT_G2_DMA SB(0x6950)
#define INT_NORMAL 0x0
#define INT_EXTERNAL 0x4
#define INT_ERROR 0x8

#define MAPLE_DMA_TABLE SB(0x6c04)
#define MAPLE_TRIGGER SB(0x6c10)
#define MAPLE_ENABLE SB(0x6c14)
#define MAPLE_START SB(0x6c18)
#define MAPLE_SYSTEM SB(0x6c80)
#define MAPLE_PROTECTION SB(0x6c8c)
#define MAPLE_MODE SB(0x6ce8)

#define GDROM_DMA_START SB(0x7418)

/* the registers of the G2 bus's four DMA channels, and the bus's timeouts */
#define G2_DMA(channel, reg) SB(0x7800 + (0x20 * (channel)) + (reg))
#define G2_DEVICE 0x00
#define G2_MEMORY 0x04
#define G2_LENGTH 0x08
#define G2_DIRECTION 0x0c
#define G2_TRIGGER 0x10
#define G2_ENABLE 0x14
#define G2_START 0x18
#define G2_SUSPEND 0x1c
#define G2_DS_TIMEOUT SB(0x7890)
#define G2_TR_TIMEOUT SB(0x7894)
#define G2_MODEM_TIMEOUT SB(0x7898)
#define G2_MODEM_WAIT SB(0x789c)
#define G2_UNK_78A0(i) SB(0x78a0 + (4 * (i)))
#define G2_PROTECTION SB(0x78bc)

#define PVR_DMA_DEVICE SB(0x7c00)
#define PVR_DMA_MEMORY SB(0x7c04)
#define PVR_DMA_LENGTH SB(0x7c08)
#define PVR_DMA_DIRECTION SB(0x7c0c)
#define PVR_DMA_TRIGGER SB(0x7c10)
#define PVR_DMA_ENABLE SB(0x7c14)
#define PVR_DMA_START SB(0x7c18)
#define PVR_DMA_PROTECTION SB(0x7c80)

/* the addresses and length syHwInit leaves in each idle DMA. each address
 * starts the last 64 KB of main memory, the tile accelerator path, sound
 * memory or video memory */
#define IDLE_MEMORY 0x0cff0000UL
#define IDLE_TA 0x11ff0000UL
#define IDLE_SOUND 0x009f0000UL
#define IDLE_VIDEO 0x04ff0000UL
#define IDLE_LENGTH 32

/* a DMA protection value: the key, then the first and last megabyte a
 * transfer may reach */
#define PROTECTION(key, first, last)                                           \
    (((Uint32)(key) << 16) | ((first) << 8) | (last))
/* the keys of the Maple, G2 and PVR DMA protection registers, and the
 * highest megabyte number, bits 26 to 20 of an address */
#define MAPLE_KEY 0x6155
#define G2_KEY 0x4659
#define PVR_KEY 0x6702
#define MEGABYTE_MAX 0x7f

#define ROOT_BUS_SPLIT_ON 0x80000000UL
#define G2_TRIGGER_SELECT 5
#define CLEAR_ALL 0xffffffffUL

/* the cache control register, and the operand cache's address array */
#define CCR (*(volatile Uint32 *)0xff00001cUL)
#define CCR_OCE 0x001 /* operand cache enable */
#define CCR_CB 0x004 /* copy-back in P1 */
#define CCR_ICE 0x100 /* instruction cache enable */
#define CCR_ICI 0x800 /* instruction cache invalidate */
#define OC_ADDRESS ((volatile Uint32 *)0xf4000000UL)
#define OC_ENTRIES 512
#define OC_ENTRY_WORDS 8

/* the interrupt controller's control register, and its priority registers
 * A to C */
#define ICR (*(volatile Uint16 *)0xffd00000UL)
#define IPRA (*(volatile Uint16 *)0xffd00004UL)
#define IPRB (*(volatile Uint16 *)0xffd00008UL)
#define IPRC (*(volatile Uint16 *)0xffd0000cUL)

/* code at its P2 address runs uncached */
#define P2(address) ((Uint32)(address) | 0xa0000000UL)

const char sygHwBuild[] = "\nsyHw Ver 1.03 Build:Oct 30 1998 12:20:48\n";
const Uint32 sygCacheForm = CCR_ICE | CCR_CB | CCR_OCE;
#ifdef GCC
/* mwsfdini.c writes sygIntMode, through a declaration without const, before
 * syHwInit reads it. GCC would fold a const definition's value into the
 * read */
Uint32 sygIntMode = SYD_INT_MODE_LOCALSTACK;
#else
const Uint32 sygIntMode = SYD_INT_MODE_LOCALSTACK;
#endif
void *sygIntStack = NULL;
const struct register_setting sygHwRegs[] = {
    {CH2_DMA_DESTINATION, IDLE_TA},
    {CH2_DMA_LENGTH, IDLE_LENGTH},
    {SORT_DMA_TABLE, IDLE_MEMORY},
    {SORT_DMA_LINK_BASE, IDLE_MEMORY},
    {SORT_DMA_LINK_WIDTH, 0},
    {SORT_DMA_LINK_SHIFT, 0},
    {DMA_REQUEST_MASK, 0},
    {BUS_WAIT_COUNT, 0},
    {CH2_DMA_PRIORITY, 0},
    {CH2_DMA_BURST, 0},
    {TA_PATH_MODE0, 0},
    {TA_PATH_MODE1, 0},
    {ROOT_BUS_SPLIT, ROOT_BUS_SPLIT_ON},
    {SB_UNK_68A4, 0},
    {SB_UNK_68AC, 0},
    {INT_LEVEL2 + INT_NORMAL, 0},
    {INT_LEVEL2 + INT_EXTERNAL, 0},
    {INT_LEVEL2 + INT_ERROR, 0},
    {INT_LEVEL4 + INT_NORMAL, 0},
    {INT_LEVEL4 + INT_EXTERNAL, 0},
    {INT_LEVEL4 + INT_ERROR, 0},
    {INT_LEVEL6 + INT_NORMAL, 0},
    {INT_LEVEL6 + INT_EXTERNAL, 0},
    {INT_LEVEL6 + INT_ERROR, 0},
    {INT_PVR_DMA + INT_NORMAL, 0},
    {INT_PVR_DMA + INT_EXTERNAL, 0},
    {INT_G2_DMA + INT_NORMAL, 0},
    {INT_G2_DMA + INT_EXTERNAL, 0},
    {MAPLE_DMA_TABLE, IDLE_MEMORY},
    {MAPLE_TRIGGER, 0},
    {MAPLE_ENABLE, 0},
    {MAPLE_SYSTEM, MAPLE_TIMEOUT(START_TIMEOUT)},
    {MAPLE_PROTECTION, PROTECTION(MAPLE_KEY, MEGABYTE_MAX, 0)},
    {MAPLE_MODE, 1},
    {G2_DMA(0, G2_DEVICE), IDLE_SOUND},
    {G2_DMA(0, G2_MEMORY), IDLE_MEMORY},
    {G2_DMA(0, G2_LENGTH), IDLE_LENGTH},
    {G2_DMA(0, G2_DIRECTION), 0},
    {G2_DMA(0, G2_TRIGGER), G2_TRIGGER_SELECT},
    {G2_DMA(0, G2_ENABLE), 0},
    {G2_DMA(0, G2_SUSPEND), 0},
    {G2_DMA(1, G2_DEVICE), IDLE_SOUND},
    {G2_DMA(1, G2_MEMORY), IDLE_MEMORY},
    {G2_DMA(1, G2_LENGTH), IDLE_LENGTH},
    {G2_DMA(1, G2_DIRECTION), 0},
    {G2_DMA(1, G2_TRIGGER), G2_TRIGGER_SELECT},
    {G2_DMA(1, G2_ENABLE), 0},
    {G2_DMA(1, G2_SUSPEND), 0},
    {G2_DMA(2, G2_DEVICE), IDLE_SOUND},
    {G2_DMA(2, G2_MEMORY), IDLE_MEMORY},
    {G2_DMA(2, G2_LENGTH), IDLE_LENGTH},
    {G2_DMA(2, G2_DIRECTION), 0},
    {G2_DMA(2, G2_TRIGGER), G2_TRIGGER_SELECT},
    {G2_DMA(2, G2_ENABLE), 0},
    {G2_DMA(2, G2_SUSPEND), 0},
    {G2_DS_TIMEOUT, 27},
    {G2_TR_TIMEOUT, 625},
    {G2_MODEM_TIMEOUT, 0},
    {G2_MODEM_WAIT, 1},
    {G2_UNK_78A0(0), 0},
    {G2_UNK_78A0(1), 0},
    {G2_UNK_78A0(2), 0},
    {G2_UNK_78A0(3), 0},
    {G2_UNK_78A0(4), 0},
    {G2_UNK_78A0(5), 0},
    {G2_UNK_78A0(6), 0},
    {G2_PROTECTION, PROTECTION(G2_KEY, MEGABYTE_MAX, 0)},
    {PVR_DMA_DEVICE, IDLE_VIDEO},
    {PVR_DMA_MEMORY, IDLE_MEMORY},
    {PVR_DMA_LENGTH, IDLE_LENGTH},
    {PVR_DMA_DIRECTION, 0},
    {PVR_DMA_TRIGGER, 0},
    {PVR_DMA_ENABLE, 0},
    {PVR_DMA_PROTECTION, PROTECTION(PVR_KEY, MEGABYTE_MAX, 0)},
    /* stop every DMA */
    {CH2_DMA_START, 0},
    {SORT_DMA_START, 0},
    {MAPLE_START, 0},
    {GDROM_DMA_START, 0},
    {G2_DMA(0, G2_START), 0},
    {G2_DMA(1, G2_START), 0},
    {G2_DMA(2, G2_START), 0},
    {PVR_DMA_START, 0},
    /* clear the pending interrupts */
    {INT_STATUS + INT_NORMAL, CLEAR_ALL},
    {INT_STATUS + INT_ERROR, CLEAR_ALL},
    /* the end, an address of 0, then six entries load_registers never
     * reads */
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
};

void syHwCacheFinish(void);
static void disable_caches(void);
static void load_registers(void);

void syHwInit(void)
{
    load_registers();
    syIntInit(sygIntMode, sygIntStack);
    sySqInit();
    ((void (*)(Uint32))P2(syCacheInit_P2_))(sygCacheForm);
    syTmrInit();
}

void syHwInit2(void)
{
    syChainInit();
    syG2SysInit(NULL);
}

void syHwFinish(void)
{
    sySqFinish();
    syHwCacheFinish();
}

/* turn the caches off, running from P2 */
void syHwCacheFinish(void)
{
    /* @hack spelling: func, a copy of disable_caches, which keeps the P2
     * address at @R15 (0x8c11a6ec); with one local, it is not kept there and
     * 7 of 8 words differ */
    void (*func)(void) = disable_caches;
    void (*uncached)(void) = (void (*)(void))P2(func);

    uncached();
}

static void disable_caches(void)
{
    /* @hack redundant: register on i, which gives the count R4, the address
     * R5 and the zero R6 (0x8c11a6f2 to 0x8c11a6fe); without it, 9 of 15 words
     * differ */
    register int i;

    CCR = 0;
    for (i = 0; i < OC_ENTRIES; i++) {
        OC_ADDRESS[i * OC_ENTRY_WORDS] = 0;
    }
    CCR = CCR_ICI;
    CCR = 0;
}

static void load_registers(void)
{
    const struct register_setting *setting;

    ICR = 0;
    IPRA = 0;
    IPRB = 0;
    IPRC = 0;
    /* @hack spelling: the loop's test as a break at the top of its body, where
     * SHC keeps it (TST R3,R3 at 0x8c11a726, and at 0x8ce0046e in
     * LOADER.BIN); as the for condition, it moves to the loop's bottom, the
     * function is 4 bytes longer and 14 of 20 words differ */
    for (setting = sygHwRegs;; setting++) {
        if (setting->address == 0) {
            break;
        }
        *(volatile Uint32 *)setting->address = setting->value;
    }
}
