/* the native slice's runner. play DISC FRAMES [STAGE ACT] runs the game's
 * main for FRAMES vsyncs, or with 0 until it ends. it reports where play got
 * to. DISC is the directory that holds the disc's files. given STAGE and
 * ACT, the run begins that act as the opening movie ends, else at the menus.
 * --capture FILE writes the pictures of that stage. --keys FILE plays a
 * script of keys, and --card FILE names the memory card's image.
 * --window ROOT draws in a window at the game's rate, ROOT holding the
 * build's images. --headless ROOT draws without one, and play.c names its
 * other options. a hook may take an option ahead of these.
 *
 * given no DISC and FRAMES, the runner is the front door. it plays the menus
 * until the game ends, in play's window on the build's images. its disc is
 * the one the settings name, else the build's. its card is the player's,
 * which card.c makes where none lies. the settings are --settings FILE's, or
 * at the front door the player's own (settings.c). an option overrides the
 * settings' line for it.
 *
 * the game reaches some memory by constant address. that memory is the work
 * area, the module and set areas, and RAM past the image. the runner maps a
 * domain for those, with the console's 16 MB of main RAM at 0x0c000000. the
 * same pages lie again at 0x8c000000 and 0xac000000, the SH-4's P1 and P2
 * mirrors. the game's C reaches some registers by address too, as timer.c
 * reaches TMU's. the domain maps the console's register and video areas as
 * zeros that take writes. so a read of a timer finds no time gone.
 * shinobi.h's syCacheICI calls syCacheICI_P2_ at its address ORed with
 * 0xa0000000. the link places syCacheICI_P2_ at P2_CALL, an address the OR
 * leaves as it is. the domain holds a return there.
 *
 * a task's records hold 32-bit pointers (marks.py). whatever they point at
 * lies in the domain's 4 GB, at NATIVE_DOMAIN. the image links there, and
 * the domain gives the game a stack and a heap. Windows maps the domain by
 * fixed address as Linux does, and catches a fault by a vectored handler */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#include <sys/mman.h>
#include <sys/personality.h>
#include <unistd.h>
#endif

#include "native.h"

#define RAM_SIZE 0x01000000
static const unsigned long ram_mirrors[] = {0x0c000000, 0x8c000000, 0xac000000};
#define P2_CALL 0xb0000000
/* the bytes mapped there, a page, for the return */
#define P2_CALL_SIZE 4096
#ifdef _WIN32
/* lld takes no address for a name, so the runner places it */
#define SPELLED(x) #x
#define SPELLED_OUT(x) SPELLED(x)
__asm__(".globl syCacheICI_P2_\n.set syCacheICI_P2_, " SPELLED_OUT(P2_CALL));
#endif
#define GAME_STACK 0x40000000UL
#define GAME_STACK_SIZE 0x01000000UL
/* the system bus, PVR, AICA and flash; video RAM; the TA's FIFO; the
 * operand cache as RAM; the store queues; the SH-4's on-chip registers */
static const unsigned long devices[][2] = {
    {0xa0000000, 0x01000000}, {0xa4000000, 0x02000000},
    {0x10000000, 0x02000000}, {0x7c000000, 0x00010000},
    {0xe0000000, 0x04000000}, {0xff000000, 0x01000000},
};

const char *native_disc;
int native_target_stage = -1, native_target_act;
struct sa_engine *native_engine;
/* the disc the settings name */
static const char *settings_disc;
static unsigned long frames_wanted;
unsigned long native_vsyncs;
static void (*vsync_function)(void);

/* the host's maps, each 1 where it holds. zeros maps only where nothing
 * lies, and every ram_view maps the same pages of main RAM */
#ifdef _WIN32
static int zeros(void *at, size_t size)
{
    return VirtualAlloc(at, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE) ==
           at;
}

static int ram_view(void *at)
{
    static HANDLE ram;

    if (!ram) {
        ram = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                                 RAM_SIZE, NULL);
    }
    return ram &&
           MapViewOfFileEx(ram, FILE_MAP_ALL_ACCESS, 0, 0, RAM_SIZE, at) == at;
}

static int code(void *at, size_t size)
{
    DWORD was;

    return VirtualProtect(at, size, PAGE_EXECUTE_READ, &was) != 0;
}
#else
static int zeros(void *at, size_t size)
{
    return mmap(at, size, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0) == at;
}

static int ram_view(void *at)
{
    static int ram = -1;

    if (ram < 0 && ((ram = memfd_create("sa1-ram", 0)) < 0 ||
                    ftruncate(ram, RAM_SIZE) != 0)) {
        return 0;
    }
    return mmap(at, RAM_SIZE, PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_FIXED_NOREPLACE, ram, 0) == at;
}

static int code(void *at, size_t size)
{
    return mprotect(at, size, PROT_READ | PROT_EXEC) == 0;
}
#endif

static void unmapped(const char *what, void *at)
{
    fprintf(stderr, "sa1: can't map %s at %p\n", what, at);
    exit(2);
}

void native_map(void *at, size_t size, const char *what)
{
    if (!zeros(at, size)) {
        unmapped(what, at);
    }
}

static void map_domain(void)
{
    void *ret = (void *)(NATIVE_DOMAIN + P2_CALL);

    for (size_t i = 0; i < sizeof ram_mirrors / sizeof ram_mirrors[0]; i++) {
        void *at = (void *)(NATIVE_DOMAIN + ram_mirrors[i]);

        if (!ram_view(at)) {
            unmapped("main RAM", at);
        }
    }
    for (size_t i = 0; i < sizeof devices / sizeof devices[0]; i++) {
        native_map((void *)(NATIVE_DOMAIN + devices[i][0]), devices[i][1],
                   "the registers");
    }
    /* the game's stack and its threads', before a driver's 32-bit maps can
     * take their place */
    native_map((void *)(NATIVE_DOMAIN + GAME_STACK), GAME_STACK_SIZE,
               "the game's stack");
    native_threads_reserve();
    /* x86's RET, which the P2 calls reach */
    native_map(ret, P2_CALL_SIZE, "the P2 calls");
    *(unsigned char *)ret = 0xc3;
    if (!code(ret, P2_CALL_SIZE)) {
        unmapped("the P2 calls", ret);
    }
}

/* MXCSR's RC rounds toward zero with both its bits set. FTZ flushes a
 * denormal result to zero, and DAZ reads a denormal operand as zero. the x87
 * control word's PC rounds at a float's 24 bits with both its bits clear.
 * its RC is as MXCSR's */
#define MXCSR_RC_ZERO (3U << 13)
#define MXCSR_FTZ (1U << 15)
#define MXCSR_DAZ (1U << 6)

/* the game's FPSCR rounds toward zero and flushes a denormal result, as
 * MXCSR's RC and FTZ do. DAZ stays clear, so a denormal operand keeps its
 * value, as math.c's default build reads it */
static void set_fpu(void)
{
    unsigned int csr = __builtin_ia32_stmxcsr();
    csr |= MXCSR_RC_ZERO | MXCSR_FTZ;
    csr &= ~MXCSR_DAZ;
    __builtin_ia32_ldmxcsr(csr);
}

void native_report(const char *why)
{
    fprintf(stderr, "sa1: %s after %lu vsyncs: ", why, native_vsyncs);
    native_report_state(stderr);
    fflush(stderr);
}

void (*native_pace)(void);

static void end_run(const char *why, int status) __attribute__((noreturn));
void syBtExit(void) __attribute__((noreturn));

/* the run's end. it reports where play got to. the hooks hear of the end,
 * and play.c closes what it opened. then the process ends with status, on
 * the stack of whichever of the game's threads called. _exit runs none of
 * exit's handlers. so nothing of the game's runs past the call, and play.c
 * closes once */
static void end_run(const char *why, int status)
{
    native_report(why);
    native_hook_run_end();
    if (native_play_close) {
        native_play_close();
    }
    _exit(status);
}

/* the game's flag of an open tray or a changed disc, which loader_note_error
 * sets (src/system/loader.c) */
extern int loader_error_flag;

/* Shinobi's exit to the console's boot ROM, which never returns to its
 * caller. the game calls it at the quit chord and at the menus' exit
 * (src/progress/demo.c, src/main.c). the adapter calls it at that chord in
 * the opening movie (engine/sa1/movie.c). loader_poll_drive calls it under
 * loader_error_flag (src/system/loader.c). sbInitSystem calls it where
 * gdFsInit fails, which gdfs.c's never does (src/system/sbinit.c).
 *
 * here the call is the run's end. its caller runs on one of the game's
 * threads (threads.c), and control goes back to none of them, nor to the
 * runner's main: end_run ends the process inside the call. an exit the
 * player asks for ends with 0. one under loader_error_flag ends with 2, as
 * a run that can't go on does */
void syBtExit(void)
{
    if (loader_error_flag) {
        end_run("the game left at a disc error", 2);
    }
    end_run("the game left", 0);
}

/* frame.c's waits call it. native_vsyncs counts the calls, not the host's
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
    if (frames_wanted && native_vsyncs >= frames_wanted) {
        end_run("done", 0);
    }
}

void njSetVSyncFunction(void (*function)(void))
{
    vsync_function = function;
}

#ifdef _WIN32
/* a fault in the image's code ends the run, as a signal does on Linux.
 * another module's fault goes on to its handlers: a driver may catch its
 * own */
static LONG CALLBACK fault(EXCEPTION_POINTERS *pointers)
{
    EXCEPTION_RECORD *record = pointers->ExceptionRecord;
    void *at = record->ExceptionAddress;
    HMODULE module;

    switch (record->ExceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_IN_PAGE_ERROR:
        at = (void *)record->ExceptionInformation[1];
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
    case EXCEPTION_PRIV_INSTRUCTION:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_OVERFLOW:
    case EXCEPTION_DATATYPE_MISALIGNMENT:
    case EXCEPTION_STACK_OVERFLOW:
        break;
    default:
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            record->ExceptionAddress, &module) ||
        module != GetModuleHandleA(NULL)) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    fprintf(stderr, "sa1: exception %#lx at %p\n", record->ExceptionCode, at);
    native_report("stopped");
    _exit(3);
}

static void catch_faults(void)
{
    AddVectoredExceptionHandler(1, fault);
}

/* the link fixes the image's base and turns off its dynamic base, so Windows
 * loads it there */
static void fix_addresses(char **argv)
{
    (void)argv;
}
#else
static void fault(int signal, siginfo_t *info, void *context)
{
    (void)context;
    fprintf(stderr, "sa1: signal %d at %p\n", signal, info->si_addr);
    native_report("stopped");
    _exit(3);
}

/* the handler runs on a stack of its own. a thread that overflows its stack
 * leaves no room there for the handler's frame */
static void catch_faults(void)
{
    static char own[0x10000];
    stack_t stack = {.ss_sp = own, .ss_size = sizeof own};
    struct sigaction action = {.sa_sigaction = fault,
                               .sa_flags = SA_SIGINFO | SA_ONSTACK};

    sigaltstack(&stack, NULL);
    sigemptyset(&action.sa_mask);
    sigaction(SIGSEGV, &action, NULL);
    sigaction(SIGBUS, &action, NULL);
    sigaction(SIGILL, &action, NULL);
    sigaction(SIGFPE, &action, NULL);
}

/* the kernel can place a library on the domain's fixed addresses. so the
 * runner turns randomizing off and runs itself again */
static void fix_addresses(char **argv)
{
    int persona = personality(0xffffffff);

    if (persona != -1 && !(persona & ADDR_NO_RANDOMIZE) &&
        personality((unsigned long)persona | ADDR_NO_RANDOMIZE) != -1) {
        execv("/proc/self/exe", argv);
    }
}
#endif

static int take_disc(const char *name, const char *value)
{
    if (strcmp(name, "disc") != 0) {
        return 0;
    }
    settings_disc = native_memory(strdup(value), "the settings' disc");
    return value[0] ? 1 : -1;
}

/* whether this run opens the engine: the front door, or a run given one of
 * play's options */
static int engine_opens;

/* words a player must read, and the run's end. a box waits for the player,
 * so only a run that opens the engine stands one */
static void refuse(const char *words)
{
    fprintf(stderr, "sa1: %s\n", words);
    if (engine_opens && native_play_alert) {
        native_play_alert(words);
    }
    exit(2);
}

/* the adapter finds some of the game's statics by name in this program's own
 * symbol table. a program that can't read its file, or lacks the table or a
 * name, starts no game */
static void find_statics(void)
{
    static const char keep[] = "the program must keep the symbol table it "
                               "was linked with, which strip removes";
    char words[320];
    const char *lacking;
    int unread = native_read_symbols();

    if (unread < 0) {
        refuse("can't read this program's own file, /proc/self/exe, where "
               "it finds the game's statics by name in its symbol table");
    }
    if (unread) {
        snprintf(words, sizeof words,
                 "this program's own file holds no symbol table, where it "
                 "finds the game's statics by name: %s",
                 keep);
        refuse(words);
    }
    if ((lacking = sa1_statics_find())) {
        snprintf(words, sizeof words,
                 "this program's symbol table holds no object named %s, a "
                 "static of the game's it finds by name: %s",
                 lacking, keep);
        refuse(words);
    }
}

/* refs.py --check's narrowing: a pointer outside the domain can't be a
 * reference */
uint32_t native_narrow(void *pointer)
{
    uintptr_t address = (uintptr_t)pointer;

    if (address && (uint64_t)address >> 32 != (uint64_t)NATIVE_DOMAIN >> 32) {
        fprintf(stderr, "sa1: a word took %p, outside the domain\n", pointer);
        abort();
    }
    return (uint32_t)address;
}

int main(int argc, char **argv)
{
    const char *capture = NULL;
    const char *card = NULL;
    const char *settings_path = NULL;
    char place[4096] = "", card_place[4096] = "", words[4096 + 256];
    char *settings = NULL;
    /* play's options, taken after the settings file so they override its
     * lines */
    const char **later =
        native_memory(calloc((size_t)argc, sizeof *later), "the run's options");
    int later_count = 0;
    int front;
    int recorded = 0;
    int taken;
    FILE *first;

    fix_addresses(argv);
    while (argc > 2 && !strncmp(argv[1], "--", 2)) {
        if ((taken = native_hook_option(argc, argv))) {
            argc -= taken;
            argv += taken;
            continue;
        }
        if (!native_play_option &&
            (!strcmp(argv[1], "--window") || !strcmp(argv[1], "--headless"))) {
            fprintf(stderr, "sa1: this build has no pictures\n");
            return 2;
        }
        if (!strcmp(argv[1], "--capture")) {
            capture = argv[2];
        } else if (!strcmp(argv[1], "--keys")) {
            native_keys_open(argv[2]);
            recorded = 1;
        } else if (!strcmp(argv[1], "--card")) {
            card = argv[2];
        } else if (!strcmp(argv[1], "--settings")) {
            settings_path = argv[2];
        } else if (native_play_option) {
            later[later_count++] = argv[1];
            later[later_count++] = argv[2];
        } else {
            break;
        }
        argc -= 2;
        argv += 2;
    }
    front = argc == 1;
    if (!front && argc != 3 && argc != 5) {
        fprintf(stderr,
                "usage: play %s[--capture FILE] [--card FILE] [--keys FILE] "
                "[--settings FILE] [DISC FRAMES [STAGE ACT]]\n",
                native_hook_usage);
        return 2;
    }
    if (settings_path && !(settings = native_settings_read(settings_path))) {
        fprintf(stderr, "sa1: can't read %s\n", settings_path);
        return 2;
    }
    if (!settings_path && front &&
        (settings_path =
             native_settings_place(place, sizeof place, 0, "settings.txt"))) {
        settings = native_settings_read(settings_path);
    }
    if (settings) {
        native_settings_take(settings, settings_path, take_disc);
        if (native_play_settings) {
            native_play_settings(settings, settings_path);
        }
    }
    for (int i = 0; i < later_count; i += 2) {
        if (!native_play_option(later[i], later[i + 1])) {
            fprintf(stderr, "sa1: no option %s\n", later[i]);
            return 2;
        }
    }
    engine_opens = later_count || front;
    if (native_hook_run_start()) {
        recorded = 1;
    }
    if (capture) {
        native_capture_open(capture);
    }
    native_disc =
        front ? settings_disc ? settings_disc : NATIVE_BUILD_DISC : argv[1];
    if (!(first = native_disc_file("1ST_READ.BIN"))) {
        snprintf(words, sizeof words,
                 "no disc at %s: %s%s names the directory that holds the "
                 "disc's files, 1ST_READ.BIN among them",
                 native_disc, front ? "a line \"disc DIR\" in " : "DISC",
                 front ? settings_path ? settings_path : "the settings" : "");
        refuse(words);
    }
    fclose(first);
    if (native_disc_differs(words, sizeof words)) {
        refuse(words);
    }
    /* the player's card, which card.c makes where none lies */
    if (!card && front &&
        native_settings_place(card_place, sizeof card_place, 1, "card.bin")) {
        card = card_place;
    }
    if (card) {
        native_card_open(card);
    }
    if (front) {
        /* a file of the player's that has no place has no path to name */
        fprintf(stderr, "sa1: the disc at %s; %s%s; %s%s\n", native_disc,
                card ? "the card at " : "no place for a card", card ? card : "",
                settings        ? "settings from "
                : settings_path ? "no settings at "
                                : "no place for settings",
                settings_path ? settings_path : "");
    } else {
        frames_wanted = strtoul(argv[2], NULL, 0);
    }
    if (argc == 5) {
        native_target_stage = atoi(argv[3]);
        native_target_act = atoi(argv[4]);
    }
    catch_faults();
    map_domain();
    /* after map_domain, so the read's map of the program's whole file lies
     * clear of the domain */
    find_statics();
    if (engine_opens && (later_count || native_play_open)) {
        native_play_open(native_disc, recorded);
    }
    set_fpu();
    native_asm_init();
    native_programs_keep();
    native_run_main((void *)(NATIVE_DOMAIN + GAME_STACK), GAME_STACK_SIZE);
    native_report("main returned");
    return 1;
}
