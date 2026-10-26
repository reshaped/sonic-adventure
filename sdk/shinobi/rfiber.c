/* fiber servers: cooperative fibers on their own stacks, switched by setjmp
 * and longjmp */
#include <machine.h>
#include <rfiber.h>
#include <setjmp.h>
#include <sg_xpt.h>
#include <string.h>

#define SERVERS 8
/* a server's fiber slots. nothing checks rfCreateServer's fibers, the
 * server's maximum, against this */
#define FIBERS 32

/* fiber states: free, ready, killed and not yet ended, or ended */
enum { FIBER_FREE, FIBER_READY, FIBER_KILLED, FIBER_ENDED };

/* jmp_buf word indices of the stack pointer and the return address */
#define JMP_SP 0
#define JMP_PC 2

/* the wake_tick of a fiber that sleeps until rfWakeupFiber */
#define NEVER 0x7fffffff

/* the FPSCR the fibers run under: the CPU's value at reset */
#define FIBER_FPSCR 0x40001

struct rf_fiber {
    void *stack;
    Uint32 stack_words;
    Sint32 unk_08;
    Sint32 state;
    /* rfKillFiberLevel kills the fibers at or below a level */
    Sint32 level;
    /* the server tick from which the fiber may run again */
    Uint32 wake_tick;
    jmp_buf context;
    void (*on_kill)(void);
    Sint32 unk_b4;
    Sint32 unk_b8;
};

struct rf_server {
    Sint32 used;
    /* the context in rfExecFiberServer that a sleeping fiber returns to */
    jmp_buf scheduler;
    /* a context saved as the server is created. rfCreateFiber copies it into
     * a new fiber's context, then overwrites all of it */
    jmp_buf initial;
    Uint32 tick;
    Sint32 fiber_max;
    Sint32 fiber_count;
    struct rf_fiber fibers[FIBERS];
    struct rf_fiber *running;
};

static struct rf_server fiber_servers[SERVERS];
/* the server whose pass runs now */
static struct rf_server *current_server;
/* current_server's value when the latest pass began, which that pass restores
 * as it ends. only one value is kept, so after a nested pass has ended, the
 * pass around it ends with current_server still at its own server */
static struct rf_server *previous_server;
static int fiber_server_count;

void rfInitSystem(Sint32 arg)
{
    memset(fiber_servers, 0, sizeof(fiber_servers));
    current_server = NULL;
    previous_server = NULL;
    fiber_server_count = 0;
}

struct rf_server *rfCreateServer(Sint32 fibers)
{
    int mask;
    int i;
    struct rf_server *server;

    mask = get_imask();
    set_imask(IMASK_ALL);
    for (i = 0, server = fiber_servers; i < SERVERS; i++, server++) {
        if (!server->used) {
            break;
        }
    }
    if (i == SERVERS) {
        server = NULL;
    } else {
        server->fiber_max = fibers;
        server->fiber_count = 0;
        server->tick = 0;
        memset(server->fibers, 0, sizeof(server->fibers));
        memset(server->initial, 0, sizeof(server->initial));
        setjmp(server->initial); /* NOLINT(bugprone-unused-return-value) */
        server->used = true;
        fiber_server_count++;
    }
    set_imask(mask);
    return server;
}

Sint32 rfCreateFiber(struct rf_server *server, void (*func)(void), void *stack,
                     Uint32 size)
{
    jmp_buf here;
    struct rf_fiber *fiber;
    int i;

    memset(here, 0, sizeof(here));
    if (server->fiber_count == server->fiber_max) {
        return -1;
    }
    for (i = 0, fiber = server->fibers; fiber->state; i++, fiber++) {
    }
    fiber->unk_08 = 0;
    /* @hack spelling: the size divided here, which puts its SHLR2 R11 between
     * the stores of unk_08 and state (0x8c11d9f2 in 1ST_READ.BIN); divided
     * just before stack_words' store, 25 of 90 words differ */
    size /= 4;
    fiber->state = FIBER_READY;
    fiber->wake_tick = server->tick;
    fiber->stack = stack;
    fiber->stack_words = size;
    fiber->on_kill = NULL;
    fiber->level = 0;
    fiber->unk_b4 = 0;
    memcpy(fiber->context, server->initial, sizeof(fiber->context));
    setjmp(here); /* NOLINT(bugprone-unused-return-value) */
    memcpy(fiber->context, here, sizeof(fiber->context));
    /* the fiber's first turn starts in func, with the stack pointer at the
     * last word of its stack */
    fiber->context[JMP_SP] = (int)&((int *)stack)[size - 1];
    fiber->context[JMP_PC] = (int)func;
    return i;
}

static void fiber_free(struct rf_server *server, struct rf_fiber *fiber)
{
    server->fiber_count--;
    fiber->state = FIBER_FREE;
    if (fiber->on_kill) {
        fiber->on_kill();
    }
}

void rfExitSystem(void)
{
    int i;
    struct rf_server *server;

    for (i = 0, server = fiber_servers; i < SERVERS; i++, server++) {
        if (server->used) {
            rfDestroyServer(server);
        }
    }
}

void rfDestroyServer(struct rf_server *server)
{
    int i;
    struct rf_fiber *fiber;

    for (i = 0, fiber = server->fibers; i < server->fiber_max; i++, fiber++) {
        if (fiber->state) {
            fiber_free(server, fiber);
        }
    }
    memset(server, 0, sizeof(*server));
}

/* end the running fiber, which the server frees when its next pass reaches
 * it */
void rfEndFiber(void)
{
    current_server->running->state = FIBER_ENDED;
    rfSleepFiber(0);
}

/* kill a ready fiber, which the server ends on its next pass, and return 0.
 * returns -1 for a fiber in any other state */
Sint32 rfKillFiber(struct rf_server *server, Sint32 fiber)
{
    struct rf_fiber *killed;

    killed = &server->fibers[fiber];
    if (killed->state == FIBER_READY) {
        rfWakeupFiber(server, fiber);
        killed->state = FIBER_KILLED;
        return 0;
    }
    return -1;
}

void rfKillFiberLevel(struct rf_server *server, Sint32 level)
{
    int i;
    struct rf_fiber *fiber;

    for (i = 0, fiber = server->fibers; i < server->fiber_max; i++, fiber++) {
        /* @hack spelling: a continue for a free fiber, which SHC tests on its
         * own, before the level (TST R2,R2 at 0x8c11db5c in 1ST_READ.BIN);
         * joined to the level test with &&, 3 of 34 words differ */
        if (!fiber->state) {
            continue;
        }
        if (fiber->level <= level) {
            rfKillFiber(server, i);
        }
    }
}

void rfWakeupFiber(struct rf_server *server, Sint32 fiber)
{
    struct rf_fiber *woken;

    woken = &server->fibers[fiber];
    if (woken->state) {
        woken->wake_tick = server->tick;
    }
}

void rfSleepFiber(Sint32 ticks)
{
    current_server->running->wake_tick =
        ticks >= 0 ? current_server->tick + ticks : NEVER;
    if (setjmp(current_server->running->context) == 0) {
        longjmp(current_server->scheduler, 1);
    }
}

void rfSleepVFiber(void)
{
    rfSleepFiber(1);
}

void rfExecFiberServer(struct rf_server *server)
{
    int fpscr;
    int i;
    struct rf_fiber *fiber;

    previous_server = current_server;
    current_server = server;
    fiber = server->fibers;
    i = 0;
    fpscr = get_fpscr();
    set_fpscr(FIBER_FPSCR);
    /* NOLINTNEXTLINE(bugprone-unused-return-value) */
    setjmp(server->scheduler);
    /* @hack implicit: i and fiber, which a sleeping fiber's longjmp finds
     * changed since the setjmp, so C leaves them indeterminate. SHC keeps them
     * in R12 and R14, which hold 0 and the first fiber (0x8c11dbf2,
     * 0x8c11dc0a) when the setjmp at 0x8c11dc16 saves them. each longjmp
     * restores those values, so it restarts the pass at the first fiber. set
     * in the loop's head after the setjmp, 22 of 83 words differ */
    for (; i < server->fiber_max; i++, fiber++) {
        server->running = fiber;
        switch (fiber->state) {
        case FIBER_FREE:
            break;
        case FIBER_ENDED:
            fiber_free(server, fiber);
            break;
        case FIBER_READY:
            if (fiber->wake_tick > server->tick) {
                break;
            }
            longjmp(fiber->context, 1);
            break;
        case FIBER_KILLED:
            /* a later pass frees an ended fiber without running it, so the
             * context's new return address, rfEndFiber, is never taken */
            fiber->state = FIBER_ENDED;
            fiber->context[JMP_PC] = (int)rfEndFiber;
            break;
        }
    }
    server->tick++;
    current_server = previous_server;
    set_fpscr(fpscr);
}
