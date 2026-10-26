/* threads that take turns, each on a stack of its own */
#include <sg_xpt.h>
#include <string.h>

#include "system/task.h"
#include "system/thread.h"

/* the bytes a thread takes when its creator asks for 0: its record, its
 * data and its stack */
#define THREAD_SIZE 0x1000

/* the base of P1, the SH-4's cached view of memory without translation */
#define P1 0x80000000

/* a thread's record, at the start of its memory. thread_get_data's bytes
 * follow it, and the stack grows down from the memory's end. the threads
 * under one parent form a ring through next and prev, and those without a
 * parent share the root's */
struct thread {
    struct thread *next;
    struct thread *prev;
    struct thread *parent;
    struct thread *child; /* the first of its children, or null */
    /* while another thread runs, the thread's stack pointer, at the
     * registers thread_yield saved or prepare laid out. from sp up, the
     * frame holds FR12 to FR15, R4 and R8 to R14. above them lie MACL, MACH
     * and GBR, the address the thread goes on from, and PR */
    void *sp;
    void *stack; /* the top of the stack, in P1, or null for the root */
    void (*destructor)(void);
    int unk_1c;
    int status;
};

struct thread *thread_current;
/* the root's record, outside the heap. the root runs on the stack it had
 * before thread_init */
static struct thread thread_root;

/* resume, run and prepare are inline assembly, which Clang doesn't see. to
 * Clang they are declared and never defined */
/* NOLINTBEGIN(clang-diagnostic-undefined-internal) */
static void resume(void);
static void run(void *entry, void *arg);
static void prepare(struct thread *thread, void *entry, void *arg);
/* NOLINTEND(clang-diagnostic-undefined-internal) */

static struct thread *thread_alloc(void *entry, void *arg, int size);
static struct thread *thread_insert_after(struct thread *thread, void *entry,
                                          void *arg, int size);

void thread_init(void)
{
    memset(&thread_root, 0, sizeof(thread_root));
    thread_current = thread_root.next = thread_root.prev = &thread_root;
}

/* the next turn goes to the running thread's first child, or else to the
 * next thread in its ring. a ring that comes back to its first thread is
 * done, and the turn goes on past its parent */
void thread_next(void)
{
    struct thread *thread;

    thread = thread_current;
    if (thread->child != NULL) {
        thread = thread->child;
    } else {
        thread = thread->next;
        while (thread->parent != NULL && thread->parent->child == thread) {
            thread = thread->parent->next;
        }
    }
    thread_current = thread;
}

/* clang-format off */
#ifndef __clang__
/* switch threads. the running thread's registers go on its stack, in the
 * frame struct thread's sp describes, and thread_next picks the next
 * thread. that thread's registers come off its own stack, and it goes on
 * where it left off */
/* @asm abi: stack-written, MOV.L @(16,R1),R15 at 0x8c09f27c loads the saved
 * stack pointer of the thread that thread_next chose */
#pragma inline_asm(thread_yield)
void thread_yield(void)
{
        STS.L       PR,@-R15
        STS.L       PR,@-R15
        STC.L       GBR,@-R15
        STS.L       MACH,@-R15
        STS.L       MACL,@-R15
        MOV.L       R14,@-R15
        MOV.L       R13,@-R15
        MOV.L       R12,@-R15
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       R4,@-R15
        FMOV.S      FR15,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR12,@-R15
        MOV.L       #_thread_current,R8
        MOV.L       @R8,R1
        BSR         _thread_next
        MOV.L       R15,@(16,R1)            ; sp
        MOV.L       @R8,R1
        MOV.L       @(16,R1),R15
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        FMOV.S      @R15+,FR15
        MOV.L       @R15+,R4
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
        MOV.L       @R15+,R12
        MOV.L       @R15+,R13
        MOV.L       @R15+,R14
        LDS.L       @R15+,MACL
        LDS.L       @R15+,MACH
        LDC.L       @R15+,GBR
        MOV.L       @R15+,R0
        JMP         @R0
        LDS.L       @R15+,PR
}
#endif
/* clang-format on */

void thread_exit(void)
{
    thread_destroy(thread_current);
}

/* free the thread's children, then the thread. each runs its destructor
 * and leaves its ring before its memory goes back to the heap. a thread
 * that was running passes the turn on, by thread_next, before it leaves */
static void thread_free(struct thread *thread)
{
    struct thread *current;
    struct thread *prev;
    struct thread *next;
    void (*destructor)(void);

    while (thread->child != NULL) {
        thread_free(thread->child);
    }
    destructor = thread->destructor;
    if (destructor != NULL) {
        current = thread_current;
        thread_current = thread;
        thread->destructor = NULL;
        destructor();
        thread_current = current;
    }
    if (thread_current == thread) {
        thread_next();
    }
    if (thread->parent != NULL && thread->parent->child == thread) {
        if (thread->next == thread) {
            thread->parent->child = NULL;
        } else {
            thread->parent->child = thread->next;
        }
    }
    prev = thread->prev;
    next = thread->next;
    prev->next = next;
    next->prev = prev;
    heap_free(thread);
}

/* clang-format off */
#ifndef __clang__
/* take the running thread's registers off its stack, as thread_yield left
 * them, and go on where it left off */
/* @asm abi: stack-written, MOV.L @(16,R1),R15 at 0x8c09f364, in thread_destroy,
 * loads the running thread's saved stack pointer */
#pragma inline_asm(resume)
static void resume(void)
{
        MOV.L       #_thread_current,R8
        MOV.L       @R8,R1
        MOV.L       @(16,R1),R15            ; sp
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        FMOV.S      @R15+,FR15
        MOV.L       @R15+,R4
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
        MOV.L       @R15+,R12
        MOV.L       @R15+,R13
        MOV.L       @R15+,R14
        LDS.L       @R15+,MACL
        LDS.L       @R15+,MACH
        LDC.L       @R15+,GBR
        MOV.L       @R15+,R0
        JMP         @R0
        LDS.L       @R15+,PR
}
#endif
/* clang-format on */

void thread_destroy(struct thread *thread)
{
    struct thread *current;

    current = thread_current;
    thread_free(thread);
    if (current != thread_current) {
        resume();
    }
}

/* each pass reads thread->next after thread_destroy has freed the thread.
 * the read still finds the following thread: heap_free writes only block
 * headers, and next is the thread's first word */
void thread_destroy_all(void)
{
    struct thread *thread;

    for (thread = thread_root.next; thread != &thread_root;
         thread = thread->next) {
        thread_destroy(thread);
    }
}

/* clang-format off */
#ifndef __clang__
/* start the running thread over: entry(arg) from the top of its stack,
 * returning into thread_exit */
/* @asm abi: stack-written, MOV.L @(20,R1),R15 at 0x8c09f3f0, in thread_start,
 * loads the top of the running thread's stack */
#pragma inline_asm(run)
static void run(void *entry, void *arg)
{
        MOV.L       #_thread_current,R8
        MOV.L       @R8,R1
        MOV.L       @(20,R1),R15            ; stack
        MOV.L       #_thread_exit,R0
        LDS         R0,PR
        MOV         R4,R0
        JMP         @R0
        MOV         R5,R4
}

/* lay out the thread's stack from its top as thread_yield leaves a
 * switched-out thread's. the thread's next turn then starts entry(arg),
 * which returns into thread_exit. the thread starts with the GBR, MACH,
 * MACL and FR12 to FR15 that thread_start's caller left. its R8 to R14
 * start as whatever the stack's memory held */
/* expanded into thread_start, it stores MACH and MACL as the caller left
 * them, at 0x8c09f422 and 0x8c09f424 */
/* @asm open: no inspect rule reads the MACH and MACL it stores as its caller
 * left them */
#pragma inline_asm(prepare)
static void prepare(struct thread *thread, void *entry, void *arg)
{
        MOV.L       @(20,R4),R1             ; stack
        MOV.L       #_thread_exit,R0
        MOV.L       R0,@-R1
        MOV.L       R5,@-R1
        STC.L       GBR,@-R1
        STS.L       MACH,@-R1
        STS.L       MACL,@-R1
        ADD         #-28,R1                 ; R14 to R8, as they come
        MOV.L       R6,@-R1
        FMOV.S      FR15,@-R1
        FMOV.S      FR14,@-R1
        FMOV.S      FR13,@-R1
        FMOV.S      FR12,@-R1
        MOV.L       R1,@(16,R4)             ; sp
}
#endif
/* clang-format on */

void thread_start(struct thread *thread, void *entry, void *arg)
{
    if (thread == NULL || thread == thread_current) {
        run(entry, arg);
    } else {
        prepare(thread, entry, arg);
    }
}

/* allocate a thread of size bytes, or THREAD_SIZE for 0, set to start at
 * entry(arg) on its first turn. the caller links it into a ring. null when
 * the heap has no room */
static struct thread *thread_alloc(void *entry, void *arg, int size)
{
    struct thread *thread;

    if (size == 0) {
        size = THREAD_SIZE;
    }
    thread = heap_alloc(size);
    if (thread != NULL) {
        thread->stack = (void *)(((Uint32)thread + size) | P1);
        thread->parent = thread->child = NULL;
        thread->destructor = NULL;
        thread->unk_1c = thread->status = 0;
        thread_start(thread, entry, arg);
    }
    return thread;
}

struct thread *thread_create(void *entry, void *arg, int size)
{
    return thread_insert_after(thread_root.prev, entry, arg, size);
}

/* SHC expands it into thread_create_child, its one caller. the image also keeps
 * this copy, at 0x8c09f49c, which nothing calls. it isn't static, since SHC
 * would expand a static at its calls and write no copy of its own */
struct thread *thread_insert_before(struct thread *thread, void *entry,
                                    void *arg, int size)
{
    return thread_insert_after(thread->prev, entry, arg, size);
}

static struct thread *thread_insert_after(struct thread *thread, void *entry,
                                          void *arg, int size)
{
    struct thread *made;
    struct thread *next;

    made = thread_alloc(entry, arg, size);
    if (made != NULL) {
        made->parent = thread->parent;
        next = thread->next;
        thread->next = made;
        made->prev = thread;
        made->next = next;
        next->prev = made;
    }
    return made;
}

struct thread *thread_create_child(struct thread *parent, void *entry,
                                   void *arg, int size)
{
    struct thread *thread;

    if (parent == NULL) {
        parent = thread_current;
    }
    if (parent->child != NULL) {
        return thread_insert_before(parent->child, entry, arg, size);
    }
    thread = thread_alloc(entry, arg, size);
    if (thread != NULL) {
        parent->child = thread;
        thread->parent = parent;
        thread->prev = thread;
        thread->next = thread;
    }
    return thread;
}

void *thread_get_data(struct thread *thread)
{
    if (thread == NULL) {
        thread = thread_current;
    }
    return thread + 1;
}

void thread_set_status(struct thread *thread, int status)
{
    if (thread == NULL) {
        thread = thread_current;
    }
    thread->status = status;
}

int thread_get_status(struct thread *thread)
{
    if (thread == NULL) {
        thread = thread_current;
    }
    return thread->status;
}

void thread_set_unk_1c(struct thread *thread, int value)
{
    if (thread == NULL) {
        thread = thread_current;
    }
    thread->unk_1c = value;
}

int thread_get_unk_1c(struct thread *thread)
{
    if (thread == NULL) {
        thread = thread_current;
    }
    return thread->unk_1c;
}

void thread_set_destructor(struct thread *thread, void (*destructor)(void))
{
    if (thread == NULL) {
        thread = thread_current;
    }
    thread->destructor = destructor;
}
