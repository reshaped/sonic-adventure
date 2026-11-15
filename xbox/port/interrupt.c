/* the port's interrupt entry: how the vertical blank preempts the game, as
 * the console's level 6 interrupt does.
 *
 * the host stops the game's thread where it is, a signal on the PC, keeps
 * its eip and esp in port_interrupted_eip and port_interrupted_esp, and
 * resumes it at port_interrupt_entry on the interrupt stack, every other
 * register as it was. the entry saves them all, the flags and the x87 and
 * SSE state with FXSAVE, runs the blank, restores them and returns to the
 * interrupted code, writing only the word below its stack pointer, as a
 * call would. so the game's stacks, which the console sized for its own
 * interrupts, take no signal frame.
 *
 * a delayed fiber switch, which the console makes as the interrupt returns,
 * moves the saved state onto the interrupted fiber's own stack, about 600
 * bytes below its stack pointer, as a frame port_fiber_switch resumes into
 * resume_tail, and returns into the new fiber instead */
#include <sg_xpt.h>
#include <shinobi.h>

#include "xbox/port/host.h"
#include "xbox/port/port.h"

#define STACK_SIZE 0x10000
/* the frame the entry pushes below the interrupt stack's top: eflags and
 * PUSHAL's 8 registers, 12 bytes of padding, then FXSAVE's 512 bytes, which
 * start 16-byte aligned */
#define FRAME_SIZE 560
#define FX_SIZE 512
#define PUSHAL_SIZE 32

Uint32 port_interrupted_eip;
Uint32 port_interrupted_esp;
Uint32 port_resume_sp; /* the interrupted stack pointer less 4, eip there */
static Uint8 interrupt_stack[STACK_SIZE] __attribute__((aligned(16)));
static volatile int in_interrupt;

void port_interrupt_entry(void);
void port_interrupt_entry_end(void);
void port_fiber_resume_tail(void);
void port_fiber_load(void *sp);
void port_interrupt_body(Uint8 *frame);

/* the blank's handlers, in system.c; whether a blank waits, and the delayed
 * fiber switch's target, in fiber.c */
void port_blank_handlers(void);
int port_blank_masked(void);
struct sy_fiber *port_fiber_take_delayed(void);
struct sy_fiber *port_fiber_current(struct sy_fiber *becomes);

__asm__(".text\n"
        ".globl port_interrupt_entry\n"
        "port_interrupt_entry:\n"
        "    pushfl\n"
        "    pushal\n"
        "    subl $524, %esp\n"
        "    fxsave (%esp)\n"
        "    cld\n"
        "    pushl %esp\n"
        "    call port_interrupt_body\n"
        "    addl $4, %esp\n"
        "    fxrstor (%esp)\n"
        "    addl $524, %esp\n"
        "    popal\n"
        "    popfl\n"
        "    movl port_resume_sp, %esp\n"
        "    ret\n"
        ".globl port_interrupt_entry_end\n"
        "port_interrupt_entry_end:\n"
        /* a fiber the delayed switch left: its frame holds the FXSAVE
         * area's address, then the PUSHAL area's, whose eflags and eip
         * follow it */
        ".globl port_fiber_resume_tail\n"
        "port_fiber_resume_tail:\n"
        "    popl %eax\n"
        "    fxrstor (%eax)\n"
        "    popl %eax\n"
        "    movl %eax, %esp\n"
        "    popal\n"
        "    popfl\n"
        "    ret\n"
        /* load a fiber's stack pointer as port_fiber_switch does, saving
         * nothing */
        ".globl port_fiber_load\n"
        "port_fiber_load:\n"
        "    movl 4(%esp), %esp\n"
        "    popl %edi\n"
        "    popl %esi\n"
        "    popl %ebx\n"
        "    popl %ebp\n"
        "    ret\n");

/* the host's question at a blank: whether the game's thread, at eip, may be
 * interrupted now. a blank that may not waits for the mask to drop */
static int may_interrupt(Uint32 eip)
{
    if (in_interrupt || (eip >= (Uint32)port_interrupt_entry && eip < (Uint32)port_interrupt_entry_end) ||
        (eip >= (Uint32)port_fiber_resume_tail && eip < (Uint32)port_fiber_load + 16) || port_blank_masked()) {
        return 0;
    }
    return 1;
}

/* copy the frame onto the interrupted fiber's stack, so that resuming it
 * restores what the blank interrupted, and return its sp for the fiber */
static void *park(const Uint8 *frame)
{
    Uint32 esp = port_interrupted_esp;
    Uint32 *eip_slot = (Uint32 *)(esp - 4);
    Uint8 *pushal = (Uint8 *)(esp - 4 - 4 - PUSHAL_SIZE);
    Uint8 *fx = (Uint8 *)(((Uint32)pushal - FX_SIZE) & ~15u);
    Uint32 *top = (Uint32 *)fx;
    int k;

    *eip_slot = port_interrupted_eip;
    for (k = 0; k < PUSHAL_SIZE + 4; k++) {
        pushal[k] = frame[FRAME_SIZE - PUSHAL_SIZE - 4 + k];
    }
    for (k = 0; k < FX_SIZE; k++) {
        fx[k] = frame[k];
    }
    *--top = (Uint32)pushal;
    *--top = (Uint32)fx;
    *--top = (Uint32)port_fiber_resume_tail;
    *--top = 0; /* ebp */
    *--top = 0; /* ebx */
    *--top = 0; /* esi */
    *--top = 0; /* edi */
    return top;
}

void port_interrupt_body(Uint8 *frame)
{
    struct sy_fiber *to;

    in_interrupt = 1;
    port_blank_handlers();
    to = port_fiber_take_delayed();
    if (to != NULL && to != port_fiber_current(NULL) && to->sp != NULL) {
        struct sy_fiber *from = port_fiber_current(to);

        from->sp = park(frame);
        in_interrupt = 0;
        port_fiber_load(to->sp);
    }
    *(Uint32 *)(port_interrupted_esp - 4) = port_interrupted_eip;
    port_resume_sp = port_interrupted_esp - 4;
    in_interrupt = 0;
}

/* a blank the host held during one of its calls, run as the call returns:
 * as the console's interrupt, it waits while the mask holds it, and runs as
 * the mask drops (port_blank_masked), else a host call from a blank's
 * handler, as the backup library's card I/O, would run the blank's
 * handlers again inside themselves */
static void run_now(void)
{
    if (!port_blank_masked()) {
        port_blank_handlers();
    }
}

static const struct host_interrupt interrupt = {
    port_interrupt_entry, may_interrupt, &port_interrupted_eip, &port_interrupted_esp,
    interrupt_stack + STACK_SIZE, run_now,
};

void port_interrupt_init(void)
{
    HOST->set_interrupt(&interrupt);
}
