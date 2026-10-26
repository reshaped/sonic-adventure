/* threads that take turns, each on a stack of its own. a thread runs until
 * it yields the turn. the root is the flow that calls thread_init, on the
 * stack it already has */
#ifndef SYSTEM_THREAD_H
#define SYSTEM_THREAD_H

struct thread;

/* make the root the only thread, and the running one. threads left from
 * before are dropped, not freed */
void thread_init(void);
/* destroy every thread but the root, with their children. call it from the
 * root: called from another thread, it never returns once it destroys that
 * one, and the threads it hadn't reached live on */
void thread_destroy_all(void);

/* a new thread at the end of the root's ring, set to start at entry(arg)
 * on its first turn. its memory is size bytes of the heap, or 4096 for 0.
 * the thread's record comes first, then thread_get_data's bytes, and the
 * stack grows down from the end. nothing stops the stack at the data.
 * null when the heap has no room. arg must stay valid while entry uses it.
 * entry's return destroys the thread.
 *
 * thread_create_child puts the thread at the end of parent's children
 * instead, or the running thread's for a null parent */
struct thread *thread_create(void *entry, void *arg, int size);
struct thread *thread_create_child(struct thread *parent, void *entry,
                                   void *arg, int size);
/* start the thread over at entry(arg), from the top of its stack. a null
 * thread, or the running one, starts over at once, and the call doesn't
 * return. any other thread starts over on its next turn. thread must not be
 * the root, whose record holds no stack top */
void thread_start(struct thread *thread, void *entry, void *arg);
/* destroy the thread and its children. thread must not be the root, which
 * isn't on the heap. when the running thread is among them, the next one
 * runs, and the call doesn't return */
void thread_destroy(struct thread *thread);
/* pass the turn to the next thread, and return when it comes back. each
 * thread but the root has one turn between two of the root's. the turns go
 * depth first: a thread's children come after it and before the rest of its
 * ring. the stage yields from the root once a step */
void thread_yield(void);

/* a thread like thread_create's, just before this one in its ring, under
 * the same parent */
struct thread *thread_insert_before(struct thread *thread, void *entry,
                                    void *arg, int size);

/* these five take a null thread as the running one. thread_get_data gives
 * the bytes after the thread's record, below its stack, which last until
 * the thread is destroyed. its thread must not be the root, which has no
 * data. status and unk_1c hold what the thread's owner stores, and a new
 * thread's are 0 */
void *thread_get_data(struct thread *thread);
void thread_set_status(struct thread *thread, int status);
int thread_get_status(struct thread *thread);
void thread_set_unk_1c(struct thread *thread, int value);
int thread_get_unk_1c(struct thread *thread);
/* the destructor runs as the thread is destroyed, after its children are.
 * it runs on the destroyer's stack, with thread_current set to the thread.
 * a null thread is the running one */
void thread_set_destructor(struct thread *thread, void (*destructor)(void));

/* thread.c's inline assembly reaches these three by symbol. thread_current
 * is the running thread, and thread_next moves it to the thread whose turn
 * is next. an entry returns into thread_exit, which destroys the running
 * thread */
extern struct thread *thread_current;
void thread_next(void);
void thread_exit(void);

#endif
