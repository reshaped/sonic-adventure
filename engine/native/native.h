/* what the native slice's runner and shim share. they are the host's C.
 * they declare the game's names themselves, in the SH-4's widths that
 * prelude.h gives the game */
#ifndef NATIVE_H
#define NATIVE_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* where the game's 4 GB of addresses lie on the host, a multiple of 4 GB. a
 * reference is an address there, and refs.py makes a pointer of one */
#ifndef NATIVE_DOMAIN
#define NATIVE_DOMAIN 0ULL
#endif

/* a reference as the host's pointer, null staying null */
static inline void *native_widen(uint32_t reference)
{
    return reference ? (void *)(NATIVE_DOMAIN | reference) : NULL;
}

/* the disc's directory, as the runner was given it */
extern const char *native_disc;
extern unsigned long native_vsyncs;
/* the stage and act the run begins after the opening. the stage is -1 for
 * the menus */
extern int native_target_stage, native_target_act;

/* sa1_main is src/main.c's main, renamed by native.mk. native_run_main runs
 * it on size bytes of stack at stack until it returns */
void sa1_main(void);
void native_run_main(void *stack, size_t size);
/* map the threads' stacks in the domain, whole, before the game's main
 * runs. a run that can't map them ends */
void native_threads_reserve(void);
/* map size bytes of zeros at at, where the host has placed nothing. a run
 * that can't map them ends, naming what */
void native_map(void *at, size_t size, const char *what);
/* block, which the host's allocator gave the run's start. a start it gave
 * null says in one line what the memory is for, and ends with 2 */
static inline void *native_memory(void *block, const char *what)
{
    if (!block) {
        fprintf(stderr, "sa1: no memory for %s\n", what);
        exit(2);
    }
    return block;
}

/* report where play got to, and what texture memory holds, on stream */
void native_report_state(FILE *stream);
void native_report(const char *why);
/* these are play.c's, each null in a program that links no play.c.
 * native_play_option returns 1 where it takes the option, and 0 where the
 * option isn't play's. recorded is nonzero where keys or a hook give the
 * pads, and play.c then reads no live pad. native_play_close reports play.c's
 * figures */
int native_play_option(const char *name, const char *value)
    __attribute__((weak));
void native_play_open(const char *disc, int recorded) __attribute__((weak));
void native_play_close(void) __attribute__((weak));
/* take play.c's lines out of the settings text read from path, leaving the
 * rest for the backend. a window also shows an alert's words in a box */
void native_play_settings(char *text, const char *path) __attribute__((weak));
void native_play_alert(const char *words) __attribute__((weak));
/* write to out name's path among the player's settings, or among their
 * data where data is nonzero. it returns out, or null where the user has no
 * home or out has no room */
const char *native_settings_place(char *out, size_t size, int data,
                                  const char *name);
/* the file at path's text, or null where it can't be read */
char *native_settings_read(const char *path);
/* give take the name and value of each line of text that holds a name. take
 * returns 1 for its own line, which goes blank so the rest keep their
 * numbers. it returns 0 for another's line, and -1 for a value it refuses. a
 * refusal ends the run, naming path's line */
void native_settings_take(char *text, const char *path,
                          int (*take)(const char *name, const char *value));
/* where a frame's records go and what waits out a vsync, which
 * native_play_open sets. it sets what reads the live pad only in a window
 * without recorded pads. each is null until set */
extern void (*native_picture_sink)(const uint8_t *records, size_t bytes);
extern void (*native_pace)(void);
extern void (*native_pad_read)(void *pad);
/* shake the live pad at strength, from 0 to 65535, for ms. the shaking falls
 * to none over the last fade_ms. it returns nonzero where the pad shakes. a
 * call of 0 ms only answers. native_play_open sets it beside
 * native_pad_read */
extern int (*native_rumble)(uint32_t strength, uint32_t ms, uint32_t fade_ms);
/* name, open to read, in the game's current directory on the disc, or null.
 * its case doesn't count, as for gdFsOpen */
FILE *native_disc_file(const char *name);
/* the sa_name_id of name's path on the disc from that directory, in upper
 * case. the disc need not hold the file */
uint32_t native_disc_file_id(const char *name);
/* each file of the build's disc that SHA256SUMS sums, by its path below the
 * root, and its sum. native.mk writes the table */
struct native_disc_sum {
    const char *path, *sum;
};
extern const struct native_disc_sum native_disc_sums[];
extern const int native_disc_sum_count;
/* nonzero where the disc at native_disc differs from the build's. words
 * then says how, by the first file of the table that differs or can't be
 * read */
int native_disc_differs(char *words, size_t size);
/* the last name gdFsOpen took, which names the program a decode writes */
extern char native_last_opened[64];
/* play.c's engine, or null in a run without one. the game's calls name it
 * where make dc's name SA_DC_ENGINE */
extern struct sa_engine *native_engine;
/* the engine the adapter's sound calls reach, play's, or null in a run
 * without one */
extern struct sa_engine *native_sound_engine;
/* SSE's MXCSR at reset: round to nearest, every exception masked. a call
 * into the engine runs under it. the game rounds toward zero and flushes, as
 * the SH-4 does. native_fpu_host loads it and returns the game's MXCSR, which
 * native_fpu_game loads back as the call returns */
#define DEFAULT_MXCSR 0x1f80U
#ifdef __SSE__
static inline uint32_t native_fpu_host(void)
{
    uint32_t game = __builtin_ia32_stmxcsr();

    __builtin_ia32_ldmxcsr(DEFAULT_MXCSR);
    return game;
}

static inline void native_fpu_game(uint32_t game)
{
    __builtin_ia32_ldmxcsr(game);
}
#endif
/* a frame's lists are over, and njkm.c's write pointers start again */
void native_lists_over(void);
/* the native_disc_file_id of the file that gave its texels to the texture
 * record at texaddr. it returns 0 for a texture made in memory, and where
 * texaddr addresses no record */
uint32_t native_texture_file(uint32_t texaddr);
/* the game loaded the first count textures of texlist, an NJS_TEXLIST.
 * names gives their names, or is null for the list's own. each texture of a
 * list given to sa1_textures_released has a record that ended */
void sa1_textures_loaded(const void *texlist, uint32_t count,
                         const char *const *names);
void sa1_textures_released(const void *texlist);
/* the most each of the adapter's tables held, to stderr */
void sa1_names_report(void);
/* find each of the game's statics that the adapter reads by name. it
 * returns null where it finds them all, or the first name it lacks, and then
 * keeps none. the game's draws read them, so the game starts only after a
 * null */
const char *sa1_statics_find(void);
/* hand port_chord the first pad's record after each sample a live pad or a
 * script gives */
void sa1_chord(void *pad);
/* switch on the 16:9 or 60 fps toggle, as save_load does for a save that
 * kept it on */
void sa1_wide_on(void);
void sa1_fast_on(void);
/* play.c's figures for the readout, the port's on-screen timing display.
 * waited_ns is the time the game's thread waited out vsyncs. device_ns is
 * the device's time for the last picture it timed. late counts the vsyncs
 * the game reached after their time, and lost those it fell behind by. a run
 * without play.c leaves them 0 */
struct sa1_figures {
    uint64_t waited_ns, device_ns, late, lost;
};
extern struct sa1_figures sa1_figures;
/* write the pictures of the run's stage to path, as sa_capture.h's records */
void native_capture_open(const char *path);
/* the head of the game's record of a pad, Shinobi's PDS_PERIPHERAL */
struct native_pad {
    uint32_t id, support, on, off, press, release;
    uint16_t r, l;
    int16_t x1, y1, x2, y2;
};
/* the shim gives pads.c the first pad's record. native_keys_open opens a
 * script of keys to play, and sets native_scripted */
void native_pad_record(void *pad);
void native_keys_open(const char *path);
extern int native_scripted;
/* read the program's own symbols, once. it returns 0 once they are read.
 * else it returns -1 for a file it can't read, and 1 for one with no symbol
 * table. native_symbol gives an object's address by its name, or null.
 * native_name_id gives a function's sa_name_id, or 0 */
int native_read_symbols(void);
char *native_symbol(const char *name);
uint32_t native_name_id(void *function);

/* the hooks: calls made at fixed places in a run. bare.c's bodies do
 * nothing, but its native_hook_opening returns 1. native_hook_option returns
 * how many of argv's words, from argv[1], a hook's option took. it returns 0
 * where argv[1] names none. native_hook_usage holds the hooks' options for
 * the usage line */
int native_hook_option(int argc, char **argv);
extern const char native_hook_usage[];
/* native_hook_run_start runs once options and settings are taken. it
 * returns nonzero where a hook gives the run's pads. native_hook_run_end runs
 * once the run has had its FRAMES vsyncs, or as the game leaves by
 * syBtExit */
int native_hook_run_start(void);
void native_hook_run_end(void);
/* without a live pad, native_hook_pads returns nonzero where a hook gives
 * the pad in place of keys. native_hook_pad_scan runs before the game's
 * pad_scan with the first pad's record, and native_hook_pad_scanned runs
 * after it */
int native_hook_pads(void);
void native_hook_pad_scan(void *pad);
void native_hook_pad_scanned(void);
/* called as the opening starts in a run with a target. it returns nonzero
 * where the opening ends into the target's act, and 0 where its movies play */
int native_hook_opening(void);
/* at a read of the clock, native_hook_clock may write a count of seconds
 * from 1950 to count. it returns 0 where the host's time stands.
 * native_hook_date takes the count the read took */
int native_hook_clock(uint32_t *count);
void native_hook_date(uint32_t count);
/* a texture list's release */
void native_hook_textures_released(void *texlist);
/* the hooks the game's C calls, which prelude.h declares to it and
 * describes. the adapter's C reads both headers, so a hook's two
 * declarations must agree. bare.c's bodies must agree with them too */
void native_hook_stage_start(void);
void native_hook_stage_step(void);
void native_hook_frame_end(void);
void native_hook_frame_waited(void);
void native_hook_task_flags(unsigned int flags);
void native_hook_task_parent(void *parent);
void native_hook_task_alloc(void *task, int level, void *step, void *root_step);
void native_hook_task_destroy(void *task);
void native_hook_heap_free(void *memory);
void native_hook_texture_load(const char *name);
void native_hook_music_request(int music);
void native_hook_voice_request(int voice);

/* XMTRX: XF0 to XF15, as sa_ftrv takes them */
float *native_xmtrx(void);
/* each program the loader reads, by its file's name: its data's and its bss'
 * bounds, in programs.py's table. native_programs_keep copies the data as the
 * run starts. native_program_read puts a program's data back and zeros its
 * bss */
struct native_program {
    const char *file;
    char *data, *data_end, *bss, *bss_end;
};
extern const struct native_program native_programs[];
extern const int native_program_count;
void native_programs_keep(void);
void native_program_read(const char *file);
/* write address over the word of object that holds mark */
void native_address32(void *object, unsigned long size, int mark,
                      void *address);
/* the memory card's image at path, made where no file is there, which a run
 * reads and writes */
void native_card_open(const char *path);
/* the engine whose storage holds the card: play.c's, or null in a run
 * without one. card.c then reads and writes the file itself */
extern struct sa_engine *native_card_engine;
/* memory went back to the heap: forget a thread whose record it held */
void native_thread_freed(void *memory);
/* run one vsync */
void native_vsync(void);
/* fill asm2c.py's tables */
void native_asm_init(void);
/* report name's first call. stubs.py's stubs call it */
void native_stub(const char *name);

#endif
