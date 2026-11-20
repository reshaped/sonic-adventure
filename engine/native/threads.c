/* the game's threads on the host. thread.c's C keeps each thread's ring and
 * turns. four of its functions are SH-4 assembly: thread_yield, resume, run
 * and prepare. they keep a thread's registers on its stack in the heap. here
 * a context keeps them, on a stack of its own. a thread's 4 KB in the heap
 * can't hold the host's frames. the stacks lie in the domain, since a task's
 * 32-bit words can point at a thread's locals.
 *
 * Linux switches by ucontext. Windows' fibers take stacks where the system
 * places them, so there a context is a stack pointer. native_switch keeps
 * the registers on that stack, with the stack's bounds from the TEB. Windows'
 * exceptions and Wine's fault handler test a frame against those bounds.
 *
 * the PSP's context is a stack pointer too, and its native_switch keeps
 * what MIPS's EABI keeps across a call. the PSP maps no memory, so its
 * stacks are the program's own bytes, and a guard there is a word each
 * switch holds to its value */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#elif !defined(__PSP__)
#include <sys/mman.h>
#include <ucontext.h>
#endif

#include "native.h"

/* the stacks' place in the domain: a slot to each context, and one more to
 * the restart context. a slot is a guard of GUARD_SIZE bytes that take no
 * access, with a stack over it. a thread that runs off its stack's end
 * faults in the guard, and the stack below stays whole. a frame of
 * GUARD_SIZE or more can pass over the guard without touching it */
#ifdef __PSP__
#define CONTEXTS 8
#define STACK_SIZE 0x4000UL
#define GUARD_SIZE 0x10UL
#define SLOT_SIZE (GUARD_SIZE + STACK_SIZE)
#define STACKS_BYTES ((CONTEXTS + 1) * SLOT_SIZE)
_Static_assert(STACKS_BYTES == NATIVE_THREADS_BYTES,
               "native.h's NATIVE_THREADS_BYTES isn't the stacks' size");
/* where native_threads_place put them */
static char *stacks;
#define STACKS ((uintptr_t)stacks)

void native_threads_place(void *at)
{
    stacks = at;
}
#else
#define CONTEXTS 64
#define STACKS (NATIVE_DOMAIN + 0x42000000UL)
#define STACK_SIZE 0x40000UL
#define GUARD_SIZE 0x10000UL
#define SLOT_SIZE (GUARD_SIZE + STACK_SIZE)
#endif

extern void *thread_current;
void thread_next(void);
void thread_exit(void);

/* the run's end, where a thread can't have its context or its stack */
static void stop(const char *what)
{
    fprintf(stderr, "sa1: can't %s\n", what);
    exit(2);
}

#ifdef _WIN32
/* native_switch(from, to) saves what Win64 keeps across a call, and the
 * TEB's stack bounds, on the running stack. it writes that stack's pointer
 * to *from, and loads the same from the stack at to. the TEB keeps
 * StackBase, StackLimit and DeallocationStack at gs:0x8, gs:0x10 and
 * gs:0x1478. make lays the frame out 8 bytes off a call's alignment, so the
 * xmm moves are unaligned ones */
void native_switch(void **from, void *to);
__asm__(".text\n"
        ".globl native_switch\n"
        "native_switch:\n"
        "    pushq %rbp\n"
        "    pushq %rbx\n"
        "    pushq %rdi\n"
        "    pushq %rsi\n"
        "    pushq %r12\n"
        "    pushq %r13\n"
        "    pushq %r14\n"
        "    pushq %r15\n"
        "    pushq %gs:0x1478\n"
        "    pushq %gs:0x10\n"
        "    pushq %gs:0x8\n"
        "    subq $160, %rsp\n"
        "    movups %xmm6, 0(%rsp)\n"
        "    movups %xmm7, 16(%rsp)\n"
        "    movups %xmm8, 32(%rsp)\n"
        "    movups %xmm9, 48(%rsp)\n"
        "    movups %xmm10, 64(%rsp)\n"
        "    movups %xmm11, 80(%rsp)\n"
        "    movups %xmm12, 96(%rsp)\n"
        "    movups %xmm13, 112(%rsp)\n"
        "    movups %xmm14, 128(%rsp)\n"
        "    movups %xmm15, 144(%rsp)\n"
        "    movq %rsp, (%rcx)\n"
        "    movq %rdx, %rsp\n"
        "    movups 0(%rsp), %xmm6\n"
        "    movups 16(%rsp), %xmm7\n"
        "    movups 32(%rsp), %xmm8\n"
        "    movups 48(%rsp), %xmm9\n"
        "    movups 64(%rsp), %xmm10\n"
        "    movups 80(%rsp), %xmm11\n"
        "    movups 96(%rsp), %xmm12\n"
        "    movups 112(%rsp), %xmm13\n"
        "    movups 128(%rsp), %xmm14\n"
        "    movups 144(%rsp), %xmm15\n"
        "    addq $160, %rsp\n"
        "    popq %gs:0x8\n"
        "    popq %gs:0x10\n"
        "    popq %gs:0x1478\n"
        "    popq %r15\n"
        "    popq %r14\n"
        "    popq %r13\n"
        "    popq %r12\n"
        "    popq %rsi\n"
        "    popq %rdi\n"
        "    popq %rbx\n"
        "    popq %rbp\n"
        "    ret\n");

typedef void *saved;

/* of native_switch's frame: the general registers it pushes, rbp to r15,
 * and the bytes of xmm6 to xmm15, which its subq and addq count too */
#define PUSHED 8
#define XMM_BYTES 160
/* a new stack's bytes from its return word up: that word, the 32 bytes of
 * entry's arguments' home, and 16 spare */
#define ENTRY_BYTES (8 + 32 + 16)

/* a stack whose switch returns into entry as into a call of it. its top
 * holds ENTRY_BYTES, the return word null. that word lies 8 bytes off a
 * multiple of 16, as a call leaves it. below it, native_switch's frame is
 * zeros but entry and the stack's bounds */
static void make(saved *self, char *stack, size_t size, void (*entry)(void))
{
    void **sp = (void **)(stack + size - ENTRY_BYTES);

    *sp = NULL;
    *--sp = (void *)entry;
    for (int i = 0; i < PUSHED; i++) {
        *--sp = NULL;
    }
    *--sp = stack;
    *--sp = stack;
    *--sp = stack + size;
    sp -= XMM_BYTES / sizeof *sp;
    memset(sp, 0, XMM_BYTES);
    *self = sp;
}

static void swap(saved *from, saved *to)
{
    native_switch(from, *to);
}

static void jump(saved *to)
{
    void *left;

    native_switch(&left, *to);
}

/* fresh zeros at a stack, 1 where it holds. a decommitted page stays
 * reserved, and commits again as zeros, as Linux's map over it gives */
static int place(void *stack, size_t size)
{
    VirtualFree(stack, size, MEM_DECOMMIT);
    return VirtualAlloc(stack, size, MEM_COMMIT, PAGE_READWRITE) != NULL;
}

/* no access to the size bytes at at, which stay the threads', 1 where it
 * holds */
static int guard(void *at, size_t size)
{
    return VirtualFree(at, size, MEM_DECOMMIT) != 0;
}
#elif defined(__PSP__)
/* native_switch(from, to) saves what the EABI keeps across a call on the
 * running stack: s0 to s7, fp and ra, f20 to f31, and the FPU's FCR31. it
 * writes that stack's pointer to *from, and loads the same from the stack at
 * to. a frame is SWITCH_WORDS words: the general registers from its first
 * word, then the FPU's, then FCR31, which loads without its cause bits
 * (native.h). the return's delay slot gives the frame
 * back, so the assembler fills no slot of its own: left to, it puts the
 * last load there, and the stack's pointer stays a frame low */
void native_switch(void **from, void *to);
__asm__(".text\n"
        ".set push\n"
        ".set noreorder\n"
        ".globl native_switch\n"
        ".ent native_switch\n"
        "native_switch:\n"
        "    addiu $sp, $sp, -96\n"
        "    sw $s0, 0($sp)\n"
        "    sw $s1, 4($sp)\n"
        "    sw $s2, 8($sp)\n"
        "    sw $s3, 12($sp)\n"
        "    sw $s4, 16($sp)\n"
        "    sw $s5, 20($sp)\n"
        "    sw $s6, 24($sp)\n"
        "    sw $s7, 28($sp)\n"
        "    sw $fp, 32($sp)\n"
        "    sw $ra, 36($sp)\n"
        "    swc1 $f20, 40($sp)\n"
        "    swc1 $f21, 44($sp)\n"
        "    swc1 $f22, 48($sp)\n"
        "    swc1 $f23, 52($sp)\n"
        "    swc1 $f24, 56($sp)\n"
        "    swc1 $f25, 60($sp)\n"
        "    swc1 $f26, 64($sp)\n"
        "    swc1 $f27, 68($sp)\n"
        "    swc1 $f28, 72($sp)\n"
        "    swc1 $f29, 76($sp)\n"
        "    swc1 $f30, 80($sp)\n"
        "    swc1 $f31, 84($sp)\n"
        "    cfc1 $t0, $31\n"
        "    nop\n"
        "    sw $t0, 88($sp)\n"
        "    sw $sp, 0($a0)\n"
        "    move $sp, $a1\n"
        "    lw $t0, 88($sp)\n"
        "    lui $t1, 0xfffc\n"
        "    ori $t1, $t1, 0x0fff\n"
        "    and $t0, $t0, $t1\n"
        "    ctc1 $t0, $31\n"
        "    nop\n"
        "    lw $s0, 0($sp)\n"
        "    lw $s1, 4($sp)\n"
        "    lw $s2, 8($sp)\n"
        "    lw $s3, 12($sp)\n"
        "    lw $s4, 16($sp)\n"
        "    lw $s5, 20($sp)\n"
        "    lw $s6, 24($sp)\n"
        "    lw $s7, 28($sp)\n"
        "    lw $fp, 32($sp)\n"
        "    lw $ra, 36($sp)\n"
        "    lwc1 $f20, 40($sp)\n"
        "    lwc1 $f21, 44($sp)\n"
        "    lwc1 $f22, 48($sp)\n"
        "    lwc1 $f23, 52($sp)\n"
        "    lwc1 $f24, 56($sp)\n"
        "    lwc1 $f25, 60($sp)\n"
        "    lwc1 $f26, 64($sp)\n"
        "    lwc1 $f27, 68($sp)\n"
        "    lwc1 $f28, 72($sp)\n"
        "    lwc1 $f29, 76($sp)\n"
        "    lwc1 $f30, 80($sp)\n"
        "    lwc1 $f31, 84($sp)\n"
        "    jr $ra\n"
        "    addiu $sp, $sp, 96\n"
        ".end native_switch\n"
        ".set pop\n");

typedef void *saved;

#define SWITCH_WORDS 24
#define SWITCH_RA 9
#define SWITCH_FCR31 22
/* the word a stack's guard holds while no frame has run into it */
#define GUARD_WORD 0x5a1a57acU

/* a stack whose switch returns into entry as into a call of it, under the
 * FPU's mode as the caller has it. its frame is zeros but those two words,
 * and 16 bytes stay spare above it */
static void make(saved *self, char *stack, size_t size, void (*entry)(void))
{
    uint32_t *sp = (uint32_t *)(stack + size - 16) - SWITCH_WORDS;
    uint32_t mode;

    __asm__ volatile("cfc1 %0, $31" : "=r"(mode));
    memset(sp, 0, SWITCH_WORDS * sizeof *sp);
    sp[SWITCH_RA] = (uint32_t)entry;
    sp[SWITCH_FCR31] = mode & ~NATIVE_FCR31_CAUSE;
    *self = sp;
}

/* whether the stack that holds the frames at sp has its guard whole. a
 * stack outside the threads' place, as the runner's own is, has none */
static void hold_guard(void *sp)
{
    uintptr_t at = (uintptr_t)sp - STACKS;

    if (at < STACKS_BYTES &&
        *(uint32_t *)(STACKS + at / SLOT_SIZE * SLOT_SIZE) != GUARD_WORD) {
        stop("keep a thread on its stack: its frames ran past the end");
    }
}

static void swap(saved *from, saved *to)
{
    native_switch(from, *to);
    hold_guard(*to);
}

static void jump(saved *to)
{
    void *left;

    native_switch(&left, *to);
}

/* fresh zeros at a stack, 1 where it holds */
static int place(void *stack, size_t size)
{
    memset(stack, 0, size);
    return 1;
}

/* the guard's word in the size bytes at at, 1 where it holds */
static int guard(void *at, size_t size)
{
    (void)size;
    *(uint32_t *)at = GUARD_WORD;
    return 1;
}
#else
typedef ucontext_t saved;

static void make(saved *self, char *stack, size_t size, void (*entry)(void))
{
    if (getcontext(self) != 0) {
        stop("make a thread's context");
    }
    self->uc_stack.ss_sp = stack;
    self->uc_stack.ss_size = size;
    self->uc_link = NULL;
    makecontext(self, entry, 0);
}

static void swap(saved *from, saved *to)
{
    if (swapcontext(from, to) != 0) {
        stop("switch threads");
    }
}

/* setcontext returns only where it fails */
static void jump(saved *to)
{
    setcontext(to);
    stop("switch threads");
}

/* fresh zeros at a stack, 1 where it holds. the map falls on the threads'
 * own pages, which native_threads_reserve took */
static int place(void *stack, size_t size)
{
    return mmap(stack, size, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) == stack;
}

/* no access to the size bytes at at, which stay the threads', 1 where it
 * holds */
static int guard(void *at, size_t size)
{
    return mprotect(at, size, PROT_NONE) == 0;
}
#endif

static struct context {
    void *thread;
    void (*entry)(void *);
    void *arg;
    saved registers;
} contexts[CONTEXTS];
/* what a running thread starts over from, on the last slot's stack */
static saved restart;
/* whether the stacks' place is the threads'. before then, Linux's fixed map
 * of a stack could fall on the host's pages */
static int reserved;

/* slot's stack, over its guard */
static char *stack_at(size_t slot)
{
    return (char *)(STACKS + slot * SLOT_SIZE + GUARD_SIZE);
}

/* whether the caller's frames lie on slot's stack */
static int stands_on(size_t slot)
{
    char here;

    return (uintptr_t)&here - (uintptr_t)stack_at(slot) < STACK_SIZE;
}

void native_threads_reserve(void)
{
#ifndef __PSP__
    native_map((void *)STACKS, (CONTEXTS + 1) * SLOT_SIZE,
               "the threads' stacks");
#endif
    for (size_t slot = 0; slot <= CONTEXTS; slot++) {
        if (!guard(stack_at(slot) - GUARD_SIZE, GUARD_SIZE)) {
            stop("guard a thread's stack");
        }
    }
    reserved = 1;
}

/* thread's context, or a free one made its own. a thread that has ended
 * runs on its free slot's stack until resume */
static struct context *context_of(void *thread)
{
    struct context *free_slot = NULL;

    for (int i = 0; i < CONTEXTS; i++) {
        if (contexts[i].thread == thread) {
            return &contexts[i];
        }
        if (!contexts[i].thread && !free_slot && !stands_on((size_t)i)) {
            free_slot = &contexts[i];
        }
    }
    if (!free_slot) {
        fprintf(stderr, "sa1: more than %d threads\n", CONTEXTS);
        exit(2);
    }
    free_slot->thread = thread;
    return free_slot;
}

static void start(void)
{
    struct context *self = context_of(thread_current);

    self->entry(self->arg);
    thread_exit();
}

/* fresh zeros on self's stack, and a context that starts its entry from the
 * stack's top. the stack's pages go, so the caller runs on another */
static void renew(struct context *self)
{
    size_t slot = (size_t)(self - contexts);
    char *stack = stack_at(slot);

    if (stands_on(slot)) {
        stop("map a stack under the thread that runs on it");
    }
    if (!reserved || !place(stack, STACK_SIZE)) {
        stop("map a thread's stack");
    }
    make(&self->registers, stack, STACK_SIZE, start);
}

void prepare(void *thread, void *entry, void *arg)
{
    struct context *self = context_of(thread);

    self->entry = (void (*)(void *))entry;
    self->arg = arg;
    renew(self);
}

/* the restart context's entry, once the running thread is off its stack */
static void restarted(void)
{
    struct context *self = context_of(thread_current);

    renew(self);
    jump(&self->registers);
}

/* the running thread starts over. its stack can't be renewed under the
 * frames that ask for it. the restart context renews it from a stack of its
 * own */
void run(void *entry, void *arg)
{
    struct context *self = context_of(thread_current);

    self->entry = (void (*)(void *))entry;
    self->arg = arg;
    make(&restart, stack_at(CONTEXTS), STACK_SIZE, restarted);
    jump(&restart);
}

void thread_yield(void)
{
    struct context *self = context_of(thread_current);
    struct context *next;

    thread_next();
    next = context_of(thread_current);
    if (next != self) {
        swap(&self->registers, &next->registers);
    }
}

void resume(void)
{
    jump(&context_of(thread_current)->registers);
}

void native_thread_freed(void *memory)
{
    for (int i = 0; i < CONTEXTS; i++) {
        if (contexts[i].thread == memory) {
            contexts[i].thread = NULL;
        }
    }
}

/* the runner's context, and the game's main's on its stack in the domain */
static saved host, game;

static void game_main(void)
{
#ifdef __PSP__
    fprintf(stderr, "sa1: on the game's stack\n");
#endif
    sa1_main();
    jump(&host);
}

void native_run_main(void *stack, size_t size)
{
    make(&game, stack, size, game_main);
    swap(&host, &game);
}
