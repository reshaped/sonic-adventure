/* the native slice's prelude, which the slice includes ahead of each of the
 * game's sources. sg_xpt.h types Uint32 and Sint32 as long, 32 bits on the
 * SH-4 and 64 on an LP64 host. that would widen every field, angle and count
 * the game wraps at 32 bits. on an LP64 host the prelude defines SG_XPT_H,
 * sg_xpt.h's guard, and gives its names their SH-4 widths. where long is 32
 * bits, as on Windows, the header stands as it is.
 *
 * kamui.h's KMDWORD stays long. on an LP64 host Kamui's records so lie wider
 * than the SH-4's, alike in each source that reads them, the SDK's, the
 * game's and the adapter's, as each reads kamui.h. a KMDWORD's value leaves
 * them in 32 bits: a register's by the shim's kmiWriteRegister, a header
 * word's by Ninja's Uint32, and a color's or a word's by the adapter's
 * uint32_t */
#ifndef NATIVE_PRELUDE_H
#define NATIVE_PRELUDE_H

#ifdef __FAST_MATH__
#error play's math takes no fast-math: ENGINE.md#numbers
#endif

#ifdef __LP64__
#define SG_XPT_H
typedef unsigned char Uint8;
typedef signed char Sint8;
typedef unsigned short Uint16;
typedef short Sint16;
typedef unsigned int Uint32;
typedef int Sint32;
typedef int Int;
typedef float Float;
enum bool { false, true };
typedef enum bool Bool;
#define FALSE (0)
#define TRUE (1)

#define NULL ((void *)0)
#define ALWAYS_INLINE static inline __attribute__((always_inline))
#endif

#include "intrinsics.h"

/* the engine the game's calls name where make dc's name SA_DC_ENGINE. it is
 * play's, or null in a run without play */
struct sa_engine;
extern struct sa_engine *native_engine;
/* frame.c's waits run the host's vsync. the heap's free gives
 * native_thread_freed each block, which a thread's record may have held */
void native_vsync(void);
void native_thread_freed(void *memory);
/* the adapter runs task.c's steps and draw callbacks, and hears of each
 * task's end and each level drawn */
struct task;
void native_task_step(struct task *task, void (*step)(struct task *task));
void native_task_draw(struct task *task, void (*draw)(struct task *task));
void native_task_destroy(struct task *task);
void native_draw_level(int level);
/* the land's task has loaded its act's land. name is the game's name for
 * the act, and gray is the gray of the land's constant material.
 * native_land_released comes as the land is let go */
struct land_table;
void native_land_loaded(const struct land_table *land, unsigned int name,
                        float gray);
void native_land_released(void);
/* the hooks whose calls marks.py writes into the game's C. native.h declares
 * them to the host's C, which defines them, and bare.c's bodies do nothing.
 * these stand at stage_start's entry, after each stage step, and either side
 * of the main loop's frame_wait */
void native_hook_stage_start(void);
void native_hook_stage_step(void);
void native_hook_frame_end(void);
void native_hook_frame_waited(void);
/* task_create's flags and a child's parent, each ahead of the task's
 * alloc. the free list's next task, null where it is empty, with its level,
 * step and task_current_root_step. a task's destroy, and memory on its way
 * back to the heap */
void native_hook_task_flags(unsigned int flags);
void native_hook_task_parent(void *parent);
void native_hook_task_alloc(void *task, int level, void *step, void *root_step);
void native_hook_task_destroy(void *task);
void native_hook_heap_free(void *memory);
/* a PVM's load by name, and music or a voice the game asks of its streams
 * by number */
void native_hook_texture_load(const char *name);
void native_hook_music_request(int music);
void native_hook_voice_request(int voice);

#endif
