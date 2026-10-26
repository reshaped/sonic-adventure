/* the fiber table and the current fiber */
#include <machine.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

/* sygFbrCurrent while no fiber runs */
#define NO_FIBER ((struct sy_fiber *)-1)
/* the sp of the entry syFbrConvertThreadToFiber takes for the calling
 * thread. it marks the entry taken until the thread's first switch away
 * saves its stack pointer there */
#define UNSAVED_SP ((void *)-1)

static Sint32 find_free_slot(void);
static void lock_fibers(Uint32 *mask);
static void unlock_fibers(const Uint32 *mask);

char *sygFbrBuild = "\nsyFbr Ver 1.00 Build:Sep 29 1998 15:10:08\n";
Sint32 sygFbrMaxFiber = 0;
struct sy_fiber *sygFbrCurrent = NO_FIBER;
void *sygFbrISRstack = NULL;
struct sy_fiber *sygFbrTblPtr;

Sint32 syFbrInit(struct sy_fiber *table, Sint32 count)
{
    if (!table || count < 2) {
        return -1;
    }
    sygFbrTblPtr = table;
    sygFbrMaxFiber = count;
    memset(syFbrGetTblPtr(0), 0, syFbrGetMaxFiber() * sizeof(struct sy_fiber));
    sygFbrCurrent = NO_FIBER;
    return 0;
}

struct sy_fiber *syFbrCreateFiber(void *stack, Uint32 size,
                                  void (*func)(void *), void *data)
{
    struct sy_fiber *fiber = NULL;
    Uint32 mask;
    /* @hack type: volatile on sp, which keeps the size, then the stack
     * pointer, at @R15 (MOV.L R5,@R15 at 0x8c617d52, MOV.L R0,@R15 at
     * 0x8c617d78); without it, 44 of 49 words differ */
    volatile Sint32 sp = size;
    /* @hack type: volatile on user, which keeps data at @(8,R15)
     * (0x8c617d54); without it, 48 of 49 words differ */
    void *volatile user = data;
    Sint32 slot;

    if (!sp || !stack || !func) {
        return NULL;
    }
    sp = (Sint32)syFbrCreateFiberMain(stack, sp, func, user);
    /* @hack redundant: a store of 0 to mask that never runs, which puts mask's
     * frame slot before user's, as in the image; without it, 5 of 49 words
     * differ */
    if (0) {
        mask = 0;
    }
    lock_fibers(&mask);
    slot = find_free_slot();
    if (slot >= 0) {
        fiber = syFbrGetTblPtr(slot);
        syFbrSetSp(fiber, (void *)sp);
        syFbrSetData(fiber, user);
    }
    unlock_fibers(&mask);
    return fiber;
}

struct sy_fiber *syFbrConvertThreadToFiber(void *data)
{
    struct sy_fiber *fiber = NULL;
    Uint32 mask;
    Sint32 slot;

    if (sygFbrCurrent != NO_FIBER) {
        return NULL;
    }
    lock_fibers(&mask);
    slot = find_free_slot();
    if (slot >= 0) {
        fiber = syFbrGetTblPtr(slot);
        syFbrSetSp(fiber, UNSAVED_SP);
        syFbrSetData(fiber, data);
        sygFbrCurrent = fiber;
    }
    unlock_fibers(&mask);
    return fiber;
}

/* the index of the first entry whose sp is NULL, or -1 */
static Sint32 find_free_slot(void)
{
    Sint32 count = syFbrGetMaxFiber();
    Sint32 slot;

    for (slot = 0; slot < count; ++slot) {
        if (!syFbrGet_fib_sp(slot)) {
            return slot;
        }
    }
    return -1;
}

Sint32 syFbrDeleteFiber(struct sy_fiber *fiber)
{
    Uint32 mask;

    if (fiber && fiber != sygFbrCurrent) {
        lock_fibers(&mask);
        syFbrSetSp(fiber, NULL);
        syFbrSetData(fiber, NULL);
        unlock_fibers(&mask);
        return 0;
    }
    return -1;
}

struct sy_fiber *syFbrGetCurrentFiber(void)
{
    return sygFbrCurrent;
}

void *syFbrGetFiberData(void)
{
    return syFbrGetData(sygFbrCurrent);
}

Sint32 syFbrSetIntStack(void *stack)
{
    if (!stack) {
        return -1;
    }
    sygFbrISRstack = stack;
    return 0;
}

struct sy_fiber *syFbrGetTblPtr(int index)
{
    return &sygFbrTblPtr[index];
}

Sint32 syFbrGetMaxFiber(void)
{
    return sygFbrMaxFiber;
}

void *syFbrGetSp(struct sy_fiber *fiber)
{
    return fiber->sp;
}

void *syFbrGetData(struct sy_fiber *fiber)
{
    return fiber->data;
}

void syFbrSetSp(struct sy_fiber *fiber, void *sp)
{
    fiber->sp = sp;
}

void syFbrSetData(struct sy_fiber *fiber, void *data)
{
    fiber->data = data;
}

void *syFbrGet_fib_sp(int index)
{
    return syFbrGetTblPtr(index)->sp;
}

/* keep the interrupt mask in *mask, and mask every interrupt */
static void lock_fibers(Uint32 *mask)
{
    *mask = get_imask();
    set_imask(IMASK_ALL);
}

static void unlock_fibers(const Uint32 *mask)
{
    set_imask(*mask);
}

void syFbrDumpTable(void)
{
}
