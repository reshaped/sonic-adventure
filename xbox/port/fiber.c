/* Shinobi's fibers, syFbr, for x86: the port layer's side of sg_syf2.src.
 * a fiber is a stack and its saved stack pointer, in the table syFbrInit
 * takes: a free entry's sp is NULL, and the current fiber's is stale until
 * it switches away. port_fiber_switch saves the registers the i386 calling
 * convention keeps, and the stack pointer, then loads another fiber's.
 *
 * syFbrDelayedSwitchToFiber, which the console makes from an interrupt and
 * which switches as the interrupt returns, switches as the port's vertical
 * blank returns, at port_fiber_interrupt_return */
#include <shinobi.h>
#include <sg_xpt.h>

#include "xbox/port/port.h"

#define MAX_FIBERS 32

static struct sy_fiber *table;
static Sint32 table_count;
static struct sy_fiber *current;
static struct sy_fiber *delayed;
/* each fiber's function, which its first turn calls with its data */
static void (*funcs[MAX_FIBERS])(void *);

/* save the kept registers and the stack pointer in *save, then load them
 * from load and return on its stack */
void port_fiber_switch(void **save, void *load);
__asm__(".text\n"
        ".globl port_fiber_switch\n"
        "port_fiber_switch:\n"
        "    movl 4(%esp), %eax\n"
        "    movl 8(%esp), %ecx\n"
        "    pushl %ebp\n"
        "    pushl %ebx\n"
        "    pushl %esi\n"
        "    pushl %edi\n"
        "    movl %esp, (%eax)\n"
        "    movl %ecx, %esp\n"
        "    popl %edi\n"
        "    popl %esi\n"
        "    popl %ebx\n"
        "    popl %ebp\n"
        "    ret\n");

/* a fiber's first turn: its function, which never returns */
static void fiber_entry(void)
{
    struct sy_fiber *self = current;

    funcs[self - table](self->data);
    port_unimplemented("a fiber's function returned");
    for (;;) {
    }
}

static struct sy_fiber *free_entry(void)
{
    Sint32 k;

    for (k = 0; table != NULL && k < table_count && k < MAX_FIBERS; k++) {
        if (table[k].sp == NULL) {
            return &table[k];
        }
    }
    return NULL;
}

Sint32 syFbrInit(struct sy_fiber *fibers, Sint32 count)
{
    Sint32 k;

    if (fibers == NULL || count < 2) {
        return -1;
    }
    table = fibers;
    table_count = count;
    for (k = 0; k < count; k++) {
        fibers[k].sp = NULL;
        fibers[k].data = NULL;
    }
    current = NULL;
    delayed = NULL;
    return 0;
}

struct sy_fiber *syFbrConvertThreadToFiber(void *data)
{
    struct sy_fiber *fiber;

    if (current != NULL || (fiber = free_entry()) == NULL) {
        return NULL;
    }
    fiber->sp = (void *)-1;
    fiber->data = data;
    current = fiber;
    return fiber;
}

/* the new stack's top, 16-byte aligned as fiber_entry's caller would leave
 * it: no return address for fiber_entry, which never returns, then
 * fiber_entry as port_fiber_switch's, then the four kept registers */
struct sy_fiber *syFbrCreateFiber(void *stack, Uint32 size, void (*func)(void *), void *data)
{
    struct sy_fiber *fiber = free_entry();
    Uint32 *top;

    if (fiber == NULL || stack == NULL) {
        return NULL;
    }
    top = (Uint32 *)(((Uint32)stack + size) & ~15u);
    *--top = 0; /* fiber_entry's return address */
    *--top = (Uint32)fiber_entry;
    *--top = 0; /* ebp */
    *--top = 0; /* ebx */
    *--top = 0; /* esi */
    *--top = 0; /* edi */
    fiber->sp = top;
    fiber->data = data;
    funcs[fiber - table] = func;
    return fiber;
}

Sint32 syFbrDeleteFiber(struct sy_fiber *fiber)
{
    if (fiber == NULL || fiber == current) {
        return -1;
    }
    fiber->sp = NULL;
    if (delayed == fiber) {
        delayed = NULL;
    }
    return 0;
}

struct sy_fiber *syFbrGetCurrentFiber(void)
{
    return current;
}

void *syFbrGetFiberData(void)
{
    return current != NULL ? current->data : NULL;
}

Sint32 syFbrSetIntStack(void *stack)
{
    return stack == NULL ? -1 : 0;
}

void syFbrSwitchToFiber(struct sy_fiber *fiber)
{
    struct sy_fiber *from = current;

    if (fiber == NULL || fiber == from || fiber->sp == NULL) {
        return;
    }
    current = fiber;
    port_fiber_switch(&from->sp, fiber->sp);
}

void syFbrDelayedSwitchToFiber(struct sy_fiber *fiber)
{
    delayed = fiber;
}

/* the delayed switch's target, taken, for interrupt.c */
struct sy_fiber *port_fiber_take_delayed(void)
{
    struct sy_fiber *to = delayed;

    delayed = NULL;
    return to;
}

/* the current fiber, which becomes becomes where that isn't null */
struct sy_fiber *port_fiber_current(struct sy_fiber *becomes)
{
    struct sy_fiber *was = current;

    if (becomes != NULL) {
        current = becomes;
    }
    return was;
}

/* the vertical blank's return: the delayed switch, if one waits */
void port_fiber_interrupt_return(void)
{
    struct sy_fiber *to = delayed;

    if (to != NULL) {
        delayed = NULL;
        syFbrSwitchToFiber(to);
    }
}

struct sy_fiber *syFbrGetTblPtr(int index)
{
    return table != NULL && index >= 0 && index < table_count ? &table[index] : NULL;
}

Sint32 syFbrGetMaxFiber(void)
{
    return table_count;
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
