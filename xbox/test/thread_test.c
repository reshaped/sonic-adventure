/* src/system/thread.c's threads, make gcc's object on the SH-4 against
 * make pc's C with its x86 switch: a log of each thread's turns as they
 * create, yield, restart and destroy threads, which both runs must write
 * alike. heap_alloc and heap_free are stand-ins: blocks from a pool, never
 * reused, so that a freed thread's record still reads as it was */
#include "harness.h"

struct thread;
void thread_init(void);
struct thread *thread_create(void *entry, void *arg, int size);
struct thread *thread_create_child(struct thread *parent, void *entry, void *arg,
                                   int size);
void thread_start(struct thread *thread, void *entry, void *arg);
void thread_destroy(struct thread *thread);
void thread_destroy_all(void);
void thread_yield(void);
void *thread_get_data(struct thread *thread);
void thread_set_status(struct thread *thread, int status);
int thread_get_status(struct thread *thread);

static unsigned char pool[64 * 4096] __attribute__((aligned(32)));
static unsigned used;

void *heap_alloc(int size)
{
    void *at;

    size = (size + 31) & ~31;
    if (used + (unsigned)size > sizeof pool) {
        return 0;
    }
    at = pool + used;
    used += (unsigned)size;
    return at;
}

void heap_free(void *p)
{
    (void)p;
}

void *memset(void *s, int c, unsigned long n)
{
    unsigned char *at = s;

    while (n-- > 0) {
        *at++ = (unsigned char)c;
    }
    return s;
}

static unsigned log_index;

static void note(unsigned who, unsigned what)
{
    harness_begin("turn", log_index++);
    harness_hex(who);
    harness_hex(what);
    harness_end();
}

static struct thread *threads[8];

/* a thread that takes n turns, noting each, then returns */
static void counter(void *arg)
{
    unsigned who = (unsigned)(unsigned long)arg;
    unsigned k;
    volatile unsigned keep = who * 1000;

    for (k = 0; k < who % 4 + 2; k++) {
        note(who, k + keep);
        thread_yield();
        keep++;
    }
    note(who, 0xdead);
}

/* a thread that makes a child, then restarts a sibling, then destroys a
 * thread, then itself */
static void boss(void *arg)
{
    unsigned who = (unsigned)(unsigned long)arg;

    note(who, 1);
    threads[5] = thread_create_child(0, counter, (void *)6ul, 2048);
    thread_yield();
    note(who, 2);
    thread_start(threads[1], counter, (void *)9ul);
    thread_yield();
    note(who, 3);
    thread_destroy(threads[2]);
    thread_yield();
    note(who, 4);
    thread_destroy(0 == 0 ? threads[0] : 0);
    note(who, 0xbad);
}

int main(void)
{
    unsigned step;

    harness_fpu();
    thread_init();
    threads[0] = thread_create(boss, (void *)1ul, 4096);
    threads[1] = thread_create(counter, (void *)2ul, 0);
    threads[2] = thread_create(counter, (void *)3ul, 2048);
    threads[3] = thread_create_child(threads[1], counter, (void *)4ul, 2048);
    threads[4] = thread_create(counter, (void *)5ul, 2048);
    thread_set_status(threads[4], 77);
    for (step = 0; step < 12; step++) {
        note(0, step);
        thread_yield();
    }
    harness_begin("status", 0);
    harness_hex((unsigned)thread_get_status(threads[4]));
    harness_end();
    thread_destroy_all();
    note(0, 0x5afe);
    return 0;
}
