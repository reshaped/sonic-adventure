/* the PSP slice's runner. it starts the game's main beside the program's own
 * file, EBOOT.PBP, and runs it until the game leaves or the player ends it
 * from the PSP's own menu. the folder holds what a run reads:
 *
 *   disc/        the disc's files, 1ST_READ.BIN among them
 *   programs/    the build's programs, which loads.c reads
 *   names.bin    the build's names, which symbols.c reads
 *   settings.txt the run's settings, where the player keeps any
 *   card.bin     the memory card's image, which card.c makes where none lies
 *   log.txt      what the run reports
 *
 * the settings are lines NAME VALUE. stage N and act N begin a run in that
 * act as the opening ends, quit N quits the stage after that many vsyncs as
 * the pause menu does, keys FILE plays FILE's script of keys in the pad's
 * place (engine/native/pads.c), dirty 1 fills the heap's free memory with
 * bytes that are no zero before the game takes any, crash N stops the run
 * at an exception after N vsyncs, to try crash.prx, trace 1 has the log
 * name each file the game opens and each program it loads, vsyncs N ends the
 * run after that many, with its report, and play.c names the rest.
 *
 * the game reaches some memory by constant address: the loader's areas and
 * the buffers the game fixes in the console's RAM past its image. console_ram
 * holds those bytes (psp.h), and the link defines it, with the names of the
 * programs that load there (engine/tools/psplink.py). the game's stack and
 * its threads' are the program's own bytes too, and its heap is the game's
 * own arena */
#include <malloc.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psppower.h>
#include <pspsdk.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "draw.h"
#include "native.h"
#include "names.h"
#include "psp.h"

PSP_MODULE_INFO("SonicAdventure", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);
/* the main thread's stack holds the game's stack and its threads' stacks
 * too (main), under MAIN_STACK_KB of its own */
#define MAIN_STACK_KB 64
#define GAME_STACK_SIZE 0x10000
#define WITHIN_BYTES (GAME_STACK_SIZE + NATIVE_THREADS_BYTES)
PSP_MAIN_THREAD_STACK_SIZE_KB(MAIN_STACK_KB + (WITHIN_BYTES + 1023) / 1024);
/* the C library's heap takes the program's memory but this much, which the
 * system's own threads and blocks take */
PSP_HEAP_SIZE_KB(-1);
PSP_HEAP_THRESHOLD_SIZE_KB(192);

#define DISC "disc"
#define SETTINGS "settings.txt"
#define CARD "card.bin"
#define LOG "log.txt"
#define LOG_BYTES (1 << 20)

const char *native_disc = DISC;
int native_target_stage = -1, native_target_act;
struct sa_engine *native_engine;
unsigned long native_vsyncs;
int native_trace;
static unsigned long last_vsync, quit_vsync;

/* the crash module's record and entry, and the settings that try it */
int main(int argc, char **argv);
static int crash_loaded;
static unsigned long crash_vsync, crash_test_vsync;
PspDebugRegBlock psp_crash_regs;
static char crash_stack[0x4000] __attribute__((aligned(16), used));
void psp_crash_entry(void);
void psp_crash_report(void) __attribute__((used));
void stage_end(short end);
static void (*vsync_function)(void);
void (*native_pace)(void);

/* the FPU's FCR31 for the game: its low two bits round toward zero, and FS
 * flushes a denormal result to zero, as the SH-4's FPSCR has the game. no
 * exception's enable bit is set, so a division by zero gives an infinity */
#define FCR31_TOWARD_ZERO 1U
#define FCR31_FLUSH (1U << 24)

static void set_fpu(void)
{
    native_fpu_game(FCR31_TOWARD_ZERO | FCR31_FLUSH);
}

/* the shim's heap for Shinobi maps its room, and the slice calls none of
 * what asks for it (shim.c's syMalloc). the PSP maps nothing */
void native_map(void *at, size_t size, const char *what)
{
    (void)size;
    fprintf(stderr, "sa1: the PSP maps no memory for %s at %p\n", what, at);
    exit(2);
}

void native_report(const char *why)
{
    fprintf(stderr,
            "sa1: %s after %lu vsyncs, %u bytes of the heap in use: ", why,
            native_vsyncs, (unsigned)mallinfo().uordblks);
    native_report_state(stderr);
    psp_frame_report();
    fflush(stderr);
}

static void end_run(const char *why) __attribute__((noreturn));
void syBtExit(void) __attribute__((noreturn));

/* the run's end. it reports where play got to, the hooks hear of the end,
 * play.c closes what it opened, and the PSP takes the screen back */
static void end_run(const char *why)
{
    native_fpu_host();
    native_report(why);
    native_hook_run_end();
    if (native_play_close) {
        native_play_close();
    }
    psp_draw_report();
    psp_scene_report();
    psp_sound_report();
    fflush(stderr);
    sceKernelExitGame();
    for (;;) {
        sceKernelSleepThread();
    }
}

/* Shinobi's exit to the console's boot ROM, which never returns to its
 * caller. here the call is the run's end, as runner.c's is */
void syBtExit(void)
{
    end_run("the game left");
}

/* words on the PSP's own text screen, which stay until the player leaves
 * from the PSP's menu */
static void stand(const char *words) __attribute__((noreturn));

static void stand(const char *words)
{
    native_fpu_host();
    fprintf(stderr, "sa1: %s\n", words);
    fflush(stderr);
    pspDebugScreenInit();
    pspDebugScreenPrintf("Sonic Adventure can't go on:\n\n%s\n", words);
    for (;;) {
        sceKernelSleepThread();
    }
}

/* the game's stop at a failure it can't go on from, as dreamcast.h's
 * port_stop says. here the words stand on the screen */
void port_stop(const char *what, const char *name, uint32_t number)
{
    char words[256];

    snprintf(words, sizeof words,
             "the game stopped: %s %s 0x%x after %lu vsyncs", what,
             name ? name : "", (unsigned)number, native_vsyncs);
    psp_draw_report();
    psp_sound_report();
    stand(words);
}

/* frame.c's waits call it. native_vsyncs counts the calls, not the PSP's
 * time, so a run decides at the same steps each time */
void native_vsync(void)
{
    native_vsyncs++;
    if (native_pace) {
        native_pace();
    }
    if (vsync_function) {
        vsync_function();
    }
    if (native_vsyncs % 600 == 0) {
        native_report("at ten seconds");
    }
    /* a stop at an exception, made to try the crash module: a store at an
     * address the program has no memory at. crashtest makes the report
     * alone, of a record that says so */
    if (crash_vsync && native_vsyncs == crash_vsync) {
        fprintf(stderr, "sa1: a store at no memory, by the crash setting\n");
        /* through a variable, so the compiler knows no address of it */
        static volatile uintptr_t nowhere = 0x00000010U;

        *(volatile uint32_t *)nowhere = 0x5a1;
    }
    if (crash_test_vsync && native_vsyncs == crash_test_vsync) {
        psp_crash_regs.cause = 7 << 2;
        psp_crash_regs.epc = (uint32_t)(uintptr_t)psp_crash_report;
        psp_crash_regs.r[31] = (uint32_t)(uintptr_t)main;
        psp_crash_regs.r[29] = (uint32_t)(uintptr_t)crash_stack;
        psp_crash_regs.badvaddr = 0x00000010U;
        psp_crash_entry();
    }
    /* the trace's word that the game still runs, each two seconds of its
     * own time */
    if (native_trace && native_vsyncs % 120 == 0) {
        fprintf(stderr, "sa1: alive at vsync %lu\n", native_vsyncs);
    }
    /* the pause menu's quit, stage.h's STAGE_END_QUIT */
    if (quit_vsync && native_vsyncs == quit_vsync) {
        stage_end(6);
    }
    if (native_vsyncs == last_vsync) {
        end_run("done");
    }
}

void njSetVSyncFunction(void (*function)(void))
{
    vsync_function = function;
}

/* the PSP's menu asks the game to end */
static int left(int arg1, int arg2, void *common)
{
    (void)arg1;
    (void)arg2;
    (void)common;
    sceKernelExitGame();
    return 0;
}

static int wait_to_leave(SceSize args, void *argp)
{
    (void)args;
    (void)argp;
    sceKernelRegisterExitCallback(sceKernelCreateCallback("leave", left, NULL));
    sceKernelSleepThreadCB();
    return 0;
}

/* a thread above the game's, which says in the log where the game stands
 * when its vsyncs stop coming: after ten seconds without one, and again
 * after forty. a load from the card can take the first */
static volatile int watching;
/* the game's stack, for the watch's words of it */
static const uint32_t *watch_stack, *watch_stack_end;
/* the program's code and constants lie from its base up to its first
 * written data, which this is among */
static char watch_data[8] = "data";

/* the words of the game's stack that are addresses of code, the program's
 * or a loaded program's, from the stack's top down: the calls the game
 * stands in, the outermost first, and past them what earlier calls left */
static void watch_calls(void)
{
    char line[160];
    int said = 0, at = 0;

    for (const uint32_t *word = watch_stack_end - 1;
         word >= watch_stack && said < 96; word--) {
        uintptr_t value = *word;

        if (value % 4 == 0 &&
            ((value >= psp_base() && value < (uintptr_t)watch_data) ||
             (value >= (uintptr_t)console_ram &&
              value < (uintptr_t)console_ram + CONSOLE_RAM_SIZE))) {
            at += snprintf(line + at, sizeof line - (size_t)at, " %08x",
                           (unsigned)value);
            if (++said % 8 == 0) {
                fprintf(stderr, "sa1: calls:%s\n", line);
                at = 0;
            }
        }
    }
    if (at) {
        fprintf(stderr, "sa1: calls:%s\n", line);
    }
}

static int watch(SceSize args, void *argp)
{
    unsigned long seen = 0;
    int stood = 0;

    (void)args;
    (void)argp;
    for (;;) {
        sceKernelDelayThread(1000000);
        if (!watching || native_vsyncs != seen) {
            seen = native_vsyncs;
            stood = 0;
            continue;
        }
        stood++;
        if (stood == 10 || stood == 40) {
            void *at = sa1_running;

            fprintf(stderr,
                    "sa1: no vsync for %d seconds after %lu: the game is in "
                    "%s at %p, the program's %08x, and the last file opened "
                    "was %s\n",
                    stood, seen,
                    !at                ? "no task"
                    : sa1_running_draw ? "a task's draw"
                                       : "a task's step",
                    at, (unsigned)((uintptr_t)at - psp_base()),
                    native_last_opened);
            if (stood == 10) {
                watch_calls();
            }
        }
    }
    return 0;
}

/* the log reaches the stick a line at a time. a file's length is recorded
 * as the file closes, so a log held open through a stop reads back empty:
 * each write opens the log, adds to it and closes it. the log goes by the
 * system's own calls and its whole path, which main makes of the program's,
 * so a thread whose folder is another's writes the same file */
static char log_path[256] = LOG;
/* the folder's length in log_path, where the log's name goes */
static size_t log_folder;
static int log_at, log_made;

/* a line goes into the log's file where the last ended: the file is made
 * whole as the run begins (log_make), so a line changes none of the card's
 * record of the file, only bytes of it. a run that stops in the middle of a
 * line then leaves a file that still reads. past LOG_BYTES the lines begin
 * again at the file's start */
static int log_write(void *cookie, const char *bytes, int count)
{
    SceUID file;

    (void)cookie;
    if (!log_made || count <= 0) {
        return count;
    }
    if (log_at + count > LOG_BYTES) {
        log_at = 0;
    }
    if ((file = sceIoOpen(log_path, PSP_O_WRONLY, 0777)) >= 0) {
        sceIoLseek32(file, log_at, PSP_SEEK_SET);
        if (sceIoWrite(file, bytes, (SceSize)count) == count) {
            log_at += count;
        }
        sceIoClose(file);
    }
    return count;
}

/* make the log's file: LOG_BYTES of empty lines. a name the card refuses,
 * as one a stopped run left damaged, gives way to the next, log1.txt and
 * on */
static void log_make(void)
{
    static char blank[4096];

    memset(blank, '\n', sizeof blank);
    for (int tried = 0; tried < 4 && !log_made; tried++) {
        SceUID file;
        int at = 0;

        if (tried) {
            snprintf(log_path + log_folder, sizeof log_path - log_folder,
                     "log%d.txt", tried);
        }
        sceIoRemove(log_path);
        file =
            sceIoOpen(log_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
        if (file < 0) {
            continue;
        }
        while (at < LOG_BYTES &&
               sceIoWrite(file, blank, sizeof blank) == (int)sizeof blank) {
            at += (int)sizeof blank;
        }
        sceIoClose(file);
        log_made = at >= LOG_BYTES;
    }
}

/* a stop at an exception. crash.prx, a kernel module beside the program,
 * takes the exceptions no other handler does: its handler, the SDK's own,
 * saves the processor's registers in psp_crash_regs and returns from the
 * exception into psp_crash_entry, both by the addresses its link gave it,
 * which are this program's. the entry takes a stack of its own, since the
 * stopped thread's may be the fault, and the report goes to the log and
 * the screen. without the module the system ends a run at an exception
 * with no word */
__asm__(".text\n"
        ".set push\n"
        ".set noreorder\n"
        ".globl psp_crash_entry\n"
        ".ent psp_crash_entry\n"
        "psp_crash_entry:\n"
        "    lui $sp, %hi(crash_stack + 0x3fe0)\n"
        "    addiu $sp, $sp, %lo(crash_stack + 0x3fe0)\n"
        "    j psp_crash_report\n"
        "    nop\n"
        ".end psp_crash_entry\n"
        ".set pop\n");

static void crash_say(const char *format, ...)
{
    char line[200];
    va_list args;
    int count;

    va_start(args, format);
    count = vsnprintf(line, sizeof line, format, args);
    va_end(args);
    if (count > (int)sizeof line - 1) {
        count = (int)sizeof line - 1;
    }
    log_write(NULL, line, count);
    pspDebugScreenPrintf("%s", line);
}

void psp_crash_report(void)
{
    /* the processor's names for the cause's code */
    static const char *const causes[32] = {
        [0] = "an interrupt",
        [4] = "an address error at a load or a fetch",
        [5] = "an address error at a store",
        [6] = "a bus error at a fetch",
        [7] = "a bus error at a load or a store",
        [8] = "a system call",
        [9] = "a breakpoint, as a divide by zero is",
        [10] = "a reserved instruction",
        [11] = "a coprocessor unusable",
        [12] = "an overflow",
        [13] = "a trap",
        [15] = "a floating point exception"};
    static const char names[32][3] = {
        "zr", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2",
        "t3", "t4", "t5", "t6", "t7", "s0", "s1", "s2", "s3", "s4", "s5",
        "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"};
    static int reported;
    const PspDebugRegBlock *r = &psp_crash_regs;
    uint32_t host = NATIVE_FCR31_HOST;
    uint32_t base = psp_base();
    uint32_t code = r->cause >> 2 & 31;
    uint32_t sp = r->r[29];

    __asm__ volatile("ctc1 %0, $31" : : "r"(host));
    if (reported++) {
        for (;;) {
            sceKernelSleepThread();
        }
    }
    pspDebugScreenInit();
    crash_say("sa1: the program stopped at %s\n",
              causes[code] ? causes[code] : "an exception");
    crash_say("sa1: epc %08x, the program's %08x; ra %08x, the program's "
              "%08x\n",
              (unsigned)r->epc, (unsigned)(r->epc - base), (unsigned)r->r[31],
              (unsigned)(r->r[31] - base));
    crash_say("sa1: badvaddr %08x cause %08x status %08x fcr31 %08x\n",
              (unsigned)r->badvaddr, (unsigned)r->cause, (unsigned)r->status,
              (unsigned)r->fsr);
    for (int i = 0; i < 32; i += 4) {
        crash_say("sa1: %s %08x %s %08x %s %08x %s %08x\n", names[i],
                  (unsigned)r->r[i], names[i + 1], (unsigned)r->r[i + 1],
                  names[i + 2], (unsigned)r->r[i + 2], names[i + 3],
                  (unsigned)r->r[i + 3]);
    }
    /* the stack's words, where its pointer is a word's address in the
     * program's memory */
    if (sp % 4 == 0 && sp >= 0x08800000U && sp < 0x0c000000U - 96) {
        const uint32_t *word = (const uint32_t *)(uintptr_t)sp;

        for (int i = 0; i < 24; i += 4) {
            crash_say("sa1: stack %02x: %08x %08x %08x %08x\n", i * 4,
                      (unsigned)word[i], (unsigned)word[i + 1],
                      (unsigned)word[i + 2], (unsigned)word[i + 3]);
        }
    }
    crash_say("sa1: after %lu vsyncs, and the last file opened was %s\n",
              native_vsyncs, native_last_opened);
    for (;;) {
        sceKernelSleepThread();
    }
}

/* load the crash module, where it lies beside the program */
static void crash_module(void)
{
    char path[sizeof log_path];
    FILE *there;
    SceUID module;

    snprintf(path, sizeof path, "%.*scrash.prx", (int)log_folder, log_path);
    if (!(there = fopen(path, "rb"))) {
        return;
    }
    fclose(there);
    module = pspSdkLoadStartModule(path, PSP_MEMORY_PARTITION_KERNEL);
    crash_loaded = module >= 0;
    /* the module leaves its mark in the record's first word, and what the
     * system said of its handler in the second: 0 where it took it */
    fprintf(stderr,
            "sa1: the crash module answers %08x, its mark %08x, and the "
            "system said %08x of its handler\n",
            (unsigned)module, (unsigned)psp_crash_regs.frame[0],
            (unsigned)psp_crash_regs.frame[1]);
    psp_crash_regs.frame[0] = psp_crash_regs.frame[1] = 0;
}

/* fill the heap's free memory with bytes that are no pointer, no count and
 * no small number, as memory the system hands out may hold */
static void dirty_heap(void)
{
    enum { PIECE = 64 << 10, MOST = 1024 };
    static void *pieces[MOST];
    int count = 0;

    while (count < MOST && (pieces[count] = malloc(PIECE))) {
        memset(pieces[count++], 0xa5, PIECE);
    }
    fprintf(stderr, "sa1: %d KB of the heap dirtied\n", count * (PIECE >> 10));
    while (count) {
        free(pieces[--count]);
    }
}

/* the program's first step, ahead of every constructor's: the FPU takes the
 * host's word, under which no exception is enabled (native.h) */
static void __attribute__((constructor(101))) first_fpu(void)
{
    (void)native_fpu_host();
}

static int take_target(const char *name, const char *value)
{
    if (!strcmp(name, "stage")) {
        native_target_stage = atoi(value);
        return 1;
    }
    if (!strcmp(name, "act")) {
        native_target_act = atoi(value);
        return 1;
    }
    if (!strcmp(name, "vsyncs")) {
        last_vsync = strtoul(value, NULL, 10);
        return 1;
    }
    if (!strcmp(name, "quit")) {
        quit_vsync = strtoul(value, NULL, 10);
        return 1;
    }
    if (!strcmp(name, "keys")) {
        native_keys_open(value);
        return 1;
    }
    if (!strcmp(name, "crash")) {
        crash_vsync = strtoul(value, NULL, 10);
        return 1;
    }
    if (!strcmp(name, "crashtest")) {
        crash_test_vsync = strtoul(value, NULL, 10);
        return 1;
    }
    if (!strcmp(name, "trace")) {
        native_trace = atoi(value);
        return 1;
    }
    if (!strcmp(name, "dirty")) {
        if (atoi(value)) {
            dirty_heap();
        }
        return 1;
    }
    return 0;
}

/* the adapter finds some of the game's statics by name, in the names the
 * build wrote beside the program. a program without them starts no game */
static void find_statics(void)
{
    char words[160];
    const char *lacking;

    if (native_read_symbols()) {
        stand("can't read names.bin, the build's names, beside the program");
    }
    if ((lacking = sa1_statics_find())) {
        snprintf(words, sizeof words,
                 "names.bin holds no object named %s, a static of the game's "
                 "the program finds by name",
                 lacking);
        stand(words);
    }
}

int main(int argc, char **argv)
{
    /* the game's stack, and its threads' stacks over it. they lie in this
     * frame, inside the stack the system made for the main thread: the
     * system knows the thread by that stack, and the stack pointer never
     * leaves it */
    char within[WITHIN_BYTES + 16];
    char *game_stack = within + (-(uintptr_t)within & 15);
    char *settings;
    FILE *first;
    FILE *log;
    SceUID leaver =
        sceKernelCreateThread("leave", wait_to_leave, 0x11, 0x1000, 0, NULL);

    if (leaver >= 0) {
        sceKernelStartThread(leaver, 0, NULL);
    }
    (void)native_fpu_host();
    scePowerSetClockFrequency(333, 333, 166);
    /* the log lies beside the program, whose path the system gives */
    if (argc > 0 && argv[0] && strrchr(argv[0], '/') &&
        (size_t)(strrchr(argv[0], '/') - argv[0]) + sizeof "/" LOG <=
            sizeof log_path) {
        size_t folder = (size_t)(strrchr(argv[0], '/') - argv[0]) + 1;

        memcpy(log_path, argv[0], folder);
        strcpy(log_path + folder, LOG);
        log_folder = folder;
    }
    log_make();
    if ((log = funopen(NULL, NULL, log_write, NULL, NULL))) {
        setvbuf(log, NULL, _IOLBF, 0);
        stderr = log;
    }
    fprintf(stderr, "sa1: the program runs, with its log at %s\n", log_path);
    crash_module();
    if ((settings = native_settings_read(SETTINGS))) {
        native_settings_take(settings, SETTINGS, take_target);
        if (native_play_settings) {
            native_play_settings(settings, SETTINGS);
        }
    }
    if (!(first = native_disc_file("1ST_READ.BIN"))) {
        stand("no disc: the folder disc, beside the program, holds the "
              "disc's files, 1ST_READ.BIN among them");
    }
    fclose(first);
    fprintf(stderr, "sa1: the settings and the disc are read\n");
    native_card_open(CARD);
    find_statics();
    fprintf(stderr,
            "sa1: %u bytes free outside the heap before the engine opens, "
            "the most together %u, and %u of the heap in use\n",
            (unsigned)sceKernelTotalFreeMemSize(),
            (unsigned)sceKernelMaxFreeMemSize(), (unsigned)mallinfo().uordblks);
    if (native_play_open) {
        native_play_open(native_disc, native_scripted);
    }
    fprintf(stderr,
            "sa1: the engine open, %u bytes of the C library's heap in use\n",
            (unsigned)mallinfo().uordblks);
    set_fpu();
    native_asm_init();
    memset(within, 0, sizeof within);
    native_threads_place(game_stack + GAME_STACK_SIZE);
    native_threads_reserve();
    {
        SceKernelThreadInfo info = {.size = sizeof info};

        if (sceKernelReferThreadStatus(sceKernelGetThreadId(), &info) >= 0) {
            fprintf(stderr,
                    "sa1: the main thread's stack is %d bytes at %p, and "
                    "the game's stacks are %u bytes at %p\n",
                    info.stackSize, info.stack, (unsigned)WITHIN_BYTES,
                    (void *)game_stack);
        }
    }
    {
        SceUID watcher =
            sceKernelCreateThread("watch", watch, 0x12, 0x2000, 0, NULL);

        if (watcher >= 0) {
            sceKernelStartThread(watcher, 0, NULL);
        }
        watch_stack = (const uint32_t *)(uintptr_t)game_stack;
        watch_stack_end =
            (const uint32_t *)(uintptr_t)(game_stack + GAME_STACK_SIZE);
        watching = 1;
    }
    fprintf(stderr, "sa1: the game starts\n");
    native_run_main(game_stack, GAME_STACK_SIZE);
    end_run("main returned");
}
