/* the port's host on a PC: a 32-bit Linux program that maps the port's RAM
 * and code areas, loads 1ST_READ.BIN's image, fills the port layer's
 * struct host_api and runs the game's main.
 *
 *   host [LINK [IMAGES [DISC]]]
 *
 * LINK is tools/pclink.py's output, build/pc/link by default: each image's
 * final.elf, and code_areas.txt, whose last line gives the code areas' end.
 * IMAGES is tools/pcdisc.py's, build/pc/disc: the x86 images' files, which
 * stand in for the disc's, and images.txt, which names each file's image.
 * DISC is the disc's files, build/disc. A file opens from IMAGES where it is
 * there, and from DISC otherwise, its path in capitals, as the disc holds
 * them. Opening an image's file loads the image's code into its area.
 *
 * RAM is one shared memory object, mapped in each of the console's views,
 * at 0x0c000000, 0x8c000000 and 0xac000000, so that code that reaches RAM
 * by another view, as the uncached P2 or a P1 constant, reaches the same
 * bytes. the console's register areas the kept SDK code writes, the
 * PowerVR's, the AICA's and the store queues, are
 * scratch memory, which takes their writes and reads back what they wrote,
 * all but the store queues, which are the RAM object's view below:
 * what reads them for the hardware's state, as a wait for a sync, takes a
 * port side of its own. the CPU's control page isn't mapped: the only
 * registers the kept code writes there, the store queues' QACR0 and QACR1,
 * are xbox/port/system.c's port_qacr under XBOX.
 *
 * the window is SDL2's with OpenGL, in video.c, and the keyboard is port
 * A's pad. HOST_HEADLESS runs without it.
 *
 * memory cards are files under build/pc/saves/, or HOST_SAVES.
 *
 * the vertical blank is SIGALRM, 60 times a second, on the game's thread.
 * its handler runs on a stack of its own and resumes the thread at the
 * port's interrupt entry, as the console's interrupt preempts it.
 *
 * a fault prints its address and the nearest name of 1ST_READ.BIN's image
 * or of the image last loaded, then stops */
#define _GNU_SOURCE
#include <ctype.h>
#include <elf.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <ucontext.h>
#include <sys/syscall.h>
#include <linux/hw_breakpoint.h>
#include <linux/perf_event.h>
#include <fcntl.h>
#include <unistd.h>

#include "../../port/host.h"
#include "../../port/port.h"
#include "sound.h"
#include "video.h"

#define RAM 0x0c000000u
/* the shared memory object holds area 3, 64 MB: the console's 16 MB of RAM,
 * then the port layer's own memory, PORT_MEMORY in xbox/port/port.h. besides
 * the console's views of RAM, it is mapped at the store queues, 0xe0000000,
 * so that a store queue write, to 0xe0000000 and the low 26 bits of the
 * address it is for, reaches that address, as the console's PREF sends it */
#define AREA_SIZE 0x04000000u
#define RAM_SIZE_KNOWN 0x01000000u
#define CODE 0x10000000u

static const char *link_dir = "build/pc/link";
static const char *images_dir = "build/pc/disc";
static const char *disc_dir = "build/disc";

/* an image's symbols, for the fault report */
struct symbols {
    char name[32];
    Elf32_Sym *table;
    int count;
    char *strings;
};
static struct symbols main_symbols, loaded_symbols;

static void *read_whole(const char *path, long *size)
{
    FILE *file = fopen(path, "rb");
    void *bytes;

    if (file == NULL) {
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    rewind(file);
    bytes = malloc((size_t)*size);
    if (fread(bytes, 1, (size_t)*size, file) != (size_t)*size) {
        fclose(file);
        free(bytes);
        return NULL;
    }
    fclose(file);
    return bytes;
}

/* load an image's segments, all of them or only its code's, and keep its
 * symbols in into */
static int load_image(const char *image, int code_only, struct symbols *into)
{
    char path[512];
    long size;
    unsigned char *elf;
    Elf32_Ehdr *header;
    Elf32_Shdr *sections;
    int k;

    snprintf(path, sizeof path, "%s/%s/final.elf", link_dir, image);
    elf = read_whole(path, &size);
    if (elf == NULL) {
        fprintf(stderr, "host: no %s\n", path);
        return -1;
    }
    header = (Elf32_Ehdr *)elf;
    for (k = 0; k < header->e_phnum; k++) {
        Elf32_Phdr *segment = (Elf32_Phdr *)(elf + header->e_phoff + k * header->e_phentsize);

        if (segment->p_type != PT_LOAD || segment->p_memsz == 0) {
            continue;
        }
        if (code_only && segment->p_vaddr < CODE) {
            continue;
        }
        memcpy((void *)(uintptr_t)segment->p_vaddr, elf + segment->p_offset, segment->p_filesz);
        memset((void *)(uintptr_t)(segment->p_vaddr + segment->p_filesz), 0, segment->p_memsz - segment->p_filesz);
    }
    sections = (Elf32_Shdr *)(elf + header->e_shoff);
    for (k = 0; k < header->e_shnum; k++) {
        if (sections[k].sh_type == SHT_SYMTAB) {
            snprintf(into->name, sizeof into->name, "%s", image);
            into->table = (Elf32_Sym *)(elf + sections[k].sh_offset);
            into->count = (int)(sections[k].sh_size / sizeof(Elf32_Sym));
            into->strings = (char *)(elf + sections[sections[k].sh_link].sh_offset);
        }
    }
    return 0;
}

static uint32_t symbol_address(struct symbols *in, const char *name)
{
    int k;

    for (k = 0; k < in->count; k++) {
        if (strcmp(in->strings + in->table[k].st_name, name) == 0 && in->table[k].st_shndx != SHN_UNDEF) {
            return in->table[k].st_value;
        }
    }
    return 0;
}

/* the name at or below address, and how far past it address is */
static const char *nearest(struct symbols *in, uint32_t address, uint32_t *past)
{
    const char *best = NULL;
    uint32_t at = 0;
    int k;

    for (k = 0; k < in->count; k++) {
        Elf32_Sym *s = &in->table[k];

        if (s->st_shndx == SHN_UNDEF || s->st_name == 0 || ELF32_ST_TYPE(s->st_info) == STT_SECTION ||
            ELF32_ST_TYPE(s->st_info) == STT_FILE) {
            continue;
        }
        if (s->st_value <= address && s->st_value >= at) {
            at = s->st_value;
            best = in->strings + s->st_name;
        }
    }
    *past = address - at;
    return best;
}

static void report(const char *what, uint32_t address)
{
    struct symbols *sets[2] = {&main_symbols, &loaded_symbols};
    int k;

    fprintf(stderr, "host: %s 0x%08x", what, address);
    for (k = 0; k < 2; k++) {
        uint32_t past;
        const char *name = sets[k]->count ? nearest(sets[k], address, &past) : NULL;

        if (name != NULL && past < 0x10000) {
            fprintf(stderr, " %s:%s+0x%x", sets[k]->name, name, past);
        }
    }
    fprintf(stderr, "\n");
}

static void on_fault(int signal, siginfo_t *info, void *context)
{
    ucontext_t *uc = context;

    fprintf(stderr, "host: signal %d at address 0x%08lx\n", signal, (unsigned long)(uintptr_t)info->si_addr);
    report("eip", (uint32_t)uc->uc_mcontext.gregs[REG_EIP]);
    fprintf(stderr, "host: eax %08x ebx %08x ecx %08x edx %08x esi %08x edi %08x ebp %08x esp %08x\n",
            (unsigned)uc->uc_mcontext.gregs[REG_EAX], (unsigned)uc->uc_mcontext.gregs[REG_EBX],
            (unsigned)uc->uc_mcontext.gregs[REG_ECX], (unsigned)uc->uc_mcontext.gregs[REG_EDX],
            (unsigned)uc->uc_mcontext.gregs[REG_ESI], (unsigned)uc->uc_mcontext.gregs[REG_EDI],
            (unsigned)uc->uc_mcontext.gregs[REG_EBP], (unsigned)uc->uc_mcontext.gregs[REG_ESP]);
    /* a fault on the code's first byte, as a call through a null or wild
     * pointer makes, leaves the caller's return address at the stack's top */
    if ((uint32_t)uc->uc_mcontext.gregs[REG_EIP] == (uint32_t)(uintptr_t)info->si_addr) {
        report("called from", *(uint32_t *)(uintptr_t)uc->uc_mcontext.gregs[REG_ESP]);
    }
    {
        uint32_t *frame = (uint32_t *)(uintptr_t)uc->uc_mcontext.gregs[REG_EBP];
        int depth;

        for (depth = 0; depth < 16 && frame != NULL && (uintptr_t)frame > 0x1000; depth++) {
            report("from", frame[1]);
            frame = (uint32_t *)(uintptr_t)frame[0];
        }
    }
    _exit(128 + signal);
}

/* the host_api calls */

static void raw_host_log(const char *text)
{
    fprintf(stderr, "game: %s\n", text);
}

static void host_exit(int status)
{
    exit(status);
}

struct host_file {
    FILE *file;
    long size;
};

/* the image whose file path names, from images.txt, into image */
static int image_of(const char *path, char *image, size_t room)
{
    char list[512];
    char line[512];
    FILE *file;
    int found = 0;

    snprintf(list, sizeof list, "%s/images.txt", images_dir);
    file = fopen(list, "r");
    if (file == NULL) {
        return 0;
    }
    while (fgets(line, sizeof line, file) != NULL) {
        char name[256];
        char which[64];

        if (sscanf(line, "%255s %63s", name, which) == 2 && strcmp(name, path) == 0) {
            snprintf(image, room, "%s", which);
            found = 1;
            break;
        }
    }
    fclose(file);
    return found;
}

static void normalize(const char *path, char *out, size_t room)
{
    size_t k = 0;

    while (*path == '/' || *path == '\\') {
        path++;
    }
    for (; *path != '\0' && k + 1 < room; path++) {
        out[k++] = *path == '\\' ? '/' : (char)toupper((unsigned char)*path);
    }
    out[k] = '\0';
}

static void *raw_host_file_open(const char *path)
{
    char name[256];
    char full[768];
    char image[64];
    struct host_file *handle;
    FILE *file;

    normalize(path, name, sizeof name);
    snprintf(full, sizeof full, "%s/%s", images_dir, name);
    file = fopen(full, "rb");
    if (file == NULL) {
        snprintf(full, sizeof full, "%s/%s", disc_dir, name);
        file = fopen(full, "rb");
    }
    if (file == NULL) {
        return NULL;
    }
    if (image_of(name, image, sizeof image)) {
        load_image(image, 1, &loaded_symbols);
        fprintf(stderr, "host: loaded %s's code\n", image);
    }
    handle = malloc(sizeof *handle);
    handle->file = file;
    fseek(file, 0, SEEK_END);
    handle->size = ftell(file);
    return handle;
}

static int raw_host_file_size(void *file)
{
    return (int)((struct host_file *)file)->size;
}

static int raw_host_file_read(void *file, unsigned offset, void *to, int count)
{
    struct host_file *handle = file;

    fseek(handle->file, (long)offset, SEEK_SET);
    return (int)fread(to, 1, (size_t)count, handle->file);
}

static void raw_host_file_close(void *file)
{
    struct host_file *handle = file;

    fclose(handle->file);
    free(handle);
}

static int raw_host_dir_exists(const char *path)
{
    char name[256];
    char full[768];
    struct stat info;

    normalize(path, name, sizeof name);
    snprintf(full, sizeof full, "%s/%s", disc_dir, name);
    return stat(full, &info) == 0 && S_ISDIR(info.st_mode);
}

static struct timespec started;

/* HOST_SLOW=N: the console's time runs N times slower than the PC's, its
 * blanks and its timers alike, so a PC too slow to draw a frame in its two
 * blanks, as Mesa's software GL may be, still drops none. the game then runs
 * frame for frame as the console's does, for tools/flycast/compare.sh */
static unsigned slow_factor(void)
{
    static unsigned slow;

    if (slow == 0) {
        slow = getenv("HOST_SLOW") != NULL && atoi(getenv("HOST_SLOW")) > 1 ? (unsigned)atoi(getenv("HOST_SLOW")) : 1;
    }
    return slow;
}

static unsigned host_time_us(void)
{
    unsigned slow = slow_factor();
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);
    return (unsigned)(((now.tv_sec - started.tv_sec) * 1000000ULL + (now.tv_nsec - started.tv_nsec) / 1000) / slow);
}

/* memory cards: HOST_SAVES/vmu_A1.bin and the like, by drive: A1, A2, B1
 * and on */
static const char *saves_dir = "build/pc/saves";

static void card_path(int drive, char *out, size_t room)
{
    snprintf(out, room, "%s/vmu_%c%d.bin", saves_dir, 'A' + drive / 2, drive % 2 + 1);
}

static int raw_host_card_exists(int drive)
{
    char path[512];
    struct stat st;

    card_path(drive, path, sizeof path);
    return stat(path, &st) == 0;
}


static int raw_host_card_read(int drive, unsigned block, void *to)
{
    char path[512];
    FILE *file;
    int ok;

    card_path(drive, path, sizeof path);
    file = fopen(path, "rb");
    if (file == NULL) {
        return -1;
    }
    ok = fseek(file, (long)block * 512, SEEK_SET) == 0 && fread(to, 512, 1, file) == 1;
    fclose(file);
    return ok ? 0 : -1;
}

static int raw_host_card_write(int drive, unsigned block, const void *from)
{
    char path[512];
    FILE *file;
    int ok;

    mkdir(saves_dir, 0777);
    card_path(drive, path, sizeof path);
    file = fopen(path, "r+b");
    if (file == NULL) {
        file = fopen(path, "w+b");
    }
    if (file == NULL) {
        return -1;
    }
    ok = fseek(file, (long)block * 512, SEEK_SET) == 0 && fwrite(from, 512, 1, file) == 1;
    ok = fclose(file) == 0 && ok;
    return ok ? 0 : -1;
}

/* a vertical blank that comes during a host call, which may hold libc's
 * locks, waits until the call returns: the blank's handlers can switch to
 * another of the game's fibers, which could make host calls of its own */
static volatile int host_busy;
static volatile int blank_owed;
static const struct host_interrupt *interrupt;

static void leave_host(void)
{
    /* the blanks a call held back run as it ends, with the host still busy,
     * so that one coming meanwhile waits too */
    while (host_busy == 1 && blank_owed) {
        blank_owed = 0;
        if (interrupt != NULL) {
            interrupt->run_now();
        }
    }
    host_busy--;
}

/* wait for the next vertical blank */
static volatile unsigned blanks;

static void host_wait_vblank(void)
{
    unsigned was = blanks;

    while (blanks == was) {
        usleep(500);
    }
}

/* whether the window opened: without one the host runs headless */
static int have_video;

/* HOST_CLOCK=frames: HOST_PADS and HOST_SHOT count the frames the game has
 * rendered, Ninja's _nj_vsync_count_, in place of blanks, and so do
 * HOST_POKE's. A disc load stops
 * that count, so a script keeps its place in an emulator whose loads take
 * longer: tools/flycast/compare.lua reads the same two strings */
static volatile uint32_t *frame_clock;

static unsigned script_now(void)
{
    return frame_clock != NULL ? *frame_clock : blanks;
}

/* HOST_QUIT=N: quit at N, a blank or under HOST_CLOCK=frames a frame, once
 * the sound's thread has ended the WAV file: tools/flycast/sound.sh's runs
 * end at a frame, as compare.lua's do */
static unsigned quit_at;
static int quit_sent;

/* HOST_PADS=BLANK:BUTTONS,...: port A's buttons, PDD_DGT_ bits in hex, and
 * its stick,
 * from each BLANK on, for a run without a window: 600:8,606:0 presses Start
 * for six blanks at blank 600 */
static const char *pad_script;

/* an entry is BLANK:BUTTONS, or BLANK:BUTTONS/X/Y with the stick's x and y,
 * 0 to 255 about 128 at rest: 2400:0/128/0 pushes the stick up */
static unsigned scripted_buttons(int *x, int *y)
{
    const char *at = pad_script;
    unsigned buttons = 0;

    while (at != NULL && *at != '\0') {
        char *end;
        unsigned from = (unsigned)strtoul(at, &end, 10);

        if (*end != ':' || from > script_now()) {
            break;
        }
        buttons = (unsigned)strtoul(end + 1, &end, 16);
        *x = *y = 128;
        if (*end == '/') {
            *x = (int)strtol(end + 1, &end, 10);
            if (*end == '/') {
                *y = (int)strtol(end + 1, &end, 10);
            }
        }
        at = *end == ',' ? end + 1 : NULL;
    }
    return buttons;
}

static unsigned host_pad_read(int port, int *x, int *y, int *l, int *r)
{
    unsigned buttons;

    *x = *y = 128;
    *l = *r = 0;
    if (port != 0) {
        return ~0u;
    }
    buttons = scripted_buttons(x, y);
    if (have_video) {
        buttons |= video_keys();
        video_stick(x, y, l, r);
    }
    return buttons;
}

/* the game's mode, watched for the log: game_mode's address in 1ST_READ.BIN,
 * and its last value */
static volatile int *watched_mode;
static int last_mode = -1;

/* HOST_WATCH=IMAGE:SYMBOL@OFFSET@...: a word the log reports as it changes,
 * while IMAGE is loaded. from SYMBOL's address, each @OFFSET reads the
 * pointer there and adds OFFSET to it, and the word at the end is the one
 * watched: advertise:advertise_task@44@4 is the ADVERTISE menus' screen */
static const char *watch;
static int watch_last = -1;
static int watch_valid;

static int in_ram(uint32_t address)
{
    return address >= RAM && address + 4 <= RAM + AREA_SIZE;
}

static void check_watch(void)
{
    char image[32];
    char name[96];
    const char *at = strchr(watch, ':');
    const char *step;
    struct symbols *in;
    uint32_t address;
    int value;

    if (at == NULL || at - watch >= (int)sizeof image) {
        return;
    }
    snprintf(image, sizeof image, "%.*s", (int)(at - watch), watch);
    in = strcmp(image, main_symbols.name) == 0 ? &main_symbols
         : strcmp(image, loaded_symbols.name) == 0 ? &loaded_symbols : NULL;
    step = strchr(at + 1, '@');
    snprintf(name, sizeof name, "%.*s", step != NULL ? (int)(step - at - 1) : (int)strlen(at + 1), at + 1);
    address = in != NULL ? symbol_address(in, name) : 0;
    for (; address != 0 && step != NULL; step = strchr(step + 1, '@')) {
        uint32_t pointer;

        if (!in_ram(address)) {
            address = 0;
            break;
        }
        pointer = *(uint32_t *)(uintptr_t)address & 0x1fffffffu;
        address = pointer == 0 ? 0 : pointer + (uint32_t)strtol(step + 1, NULL, 0);
    }
    if (address == 0 || !in_ram(address)) {
        if (watch_valid) {
            fprintf(stderr, "host: %s gone at blank %u\n", watch, blanks);
            watch_valid = 0;
        }
        return;
    }
    value = *(int *)(uintptr_t)address;
    if (!watch_valid || value != watch_last) {
        fprintf(stderr, "host: %s = %d at blank %u, frame %u\n", watch, value, blanks,
                frame_clock != NULL ? *frame_clock : 0);
        watch_last = value;
        watch_valid = 1;
    }
}

/* HOST_POKE=BLANK:SYMBOL[+OFFSET]:VALUE[:SIZE],...: at BLANK, write VALUE,
 * SIZE bytes of it, 1, 2 or 4 and 4 if left out, at 1ST_READ.BIN's SYMBOL
 * plus OFFSET: menu_state's all_open, say, which opens every story and
 * stage on the menus */
static const char *pokes;

static void check_pokes(void)
{
    while (pokes != NULL && *pokes != '\0') {
        char symbol[96];
        char *end;
        unsigned at = (unsigned)strtoul(pokes, &end, 10);
        const char *name;
        size_t length;
        unsigned offset = 0;
        unsigned long value;
        int size = 4;
        uint32_t address;

        if (at > script_now() || *end != ':') {
            return;
        }
        name = end + 1;
        length = strcspn(name, ":+");
        snprintf(symbol, sizeof symbol, "%.*s", (int)length, name);
        end = (char *)name + length;
        if (*end == '+') {
            offset = (unsigned)strtoul(end + 1, &end, 0);
        }
        value = strtoul(end + 1, &end, 0);
        if (*end == ':') {
            size = (int)strtol(end + 1, &end, 10);
        }
        address = symbol_address(&main_symbols, symbol);
        if (address != 0) {
            address += offset;
            if (size == 1) {
                *(volatile uint8_t *)(uintptr_t)address = (uint8_t)value;
            } else if (size == 2) {
                *(volatile uint16_t *)(uintptr_t)address = (uint16_t)value;
            } else {
                *(volatile uint32_t *)(uintptr_t)address = (uint32_t)value;
            }
        }
        pokes = *end == ',' ? end + 1 : end;
    }
}

static void on_alarm(int signal, siginfo_t *info, void *context)
{
    (void)signal;
    (void)info;
    blanks++;
    if (watch != NULL) {
        check_watch();
    }
    check_pokes();
    if (quit_at != 0 && !quit_sent && script_now() >= quit_at) {
        quit_sent = 1;
        fprintf(stderr, "host: quit at blank %u, frame %u\n", blanks, frame_clock != NULL ? *frame_clock : 0);
        sound_quit();
    }
    sound_blank();
    if (watched_mode != NULL && *watched_mode != last_mode) {
        last_mode = *watched_mode;
        fprintf(stderr, "host: game_mode %d at blank %u, frame %u\n", last_mode, blanks,
                frame_clock != NULL ? *frame_clock : 0);
    }
    if (host_busy) {
        blank_owed = 1;
    } else if (interrupt != NULL) {
        ucontext_t *uc = context;
        unsigned long eip = (unsigned long)uc->uc_mcontext.gregs[REG_EIP];

        /* the game's thread resumes at the port's interrupt entry, on the
         * interrupt stack, its other registers as they were */
        if (interrupt->may_interrupt(eip)) {
            *interrupt->saved_eip = eip;
            *interrupt->saved_esp = (unsigned long)uc->uc_mcontext.gregs[REG_ESP];
            uc->uc_mcontext.gregs[REG_EIP] = (greg_t)(uintptr_t)interrupt->entry;
            uc->uc_mcontext.gregs[REG_ESP] = (greg_t)(uintptr_t)interrupt->stack_top;
        }
    }
}

static void host_set_interrupt(const struct host_interrupt *with)
{
    static char alternate[0x10000];
    stack_t stack;
    struct sigaction alarm;
    struct sigevent event;
    struct itimerspec every = {{0, 16683333}, {0, 16683333}};

    if (slow_factor() > 1) {
        long long period = 16683333LL * slow_factor();

        every.it_interval.tv_sec = every.it_value.tv_sec = (time_t)(period / 1000000000);
        every.it_interval.tv_nsec = every.it_value.tv_nsec = (long)(period % 1000000000);
    }
    timer_t timer;

    interrupt = with;
    /* the signal's frame goes on a stack of the host's, never on the game's,
     * which the console sized for its own interrupts */
    stack.ss_sp = alternate;
    stack.ss_size = sizeof alternate;
    stack.ss_flags = 0;
    sigaltstack(&stack, NULL);
    memset(&alarm, 0, sizeof alarm);
    alarm.sa_sigaction = on_alarm;
    alarm.sa_flags = SA_RESTART | SA_ONSTACK | SA_SIGINFO;
    sigaction(SIGALRM, &alarm, NULL);
    /* the blank goes to the game's thread alone, never to one of SDL's or
     * the GL driver's */
    memset(&event, 0, sizeof event);
    event.sigev_notify = SIGEV_THREAD_ID;
    event.sigev_signo = SIGALRM;
    event._sigev_un._tid = (int)syscall(SYS_gettid);
    if (timer_create(CLOCK_MONOTONIC, &event, &timer) == 0) {
        timer_settime(timer, 0, &every, NULL);
    }
}

static void raw_host_get_date(int fields[7])
{
    time_t now = time(NULL);
    struct tm local;

    localtime_r(&now, &local);
    fields[0] = local.tm_year + 1900;
    fields[1] = local.tm_mon + 1;
    fields[2] = local.tm_mday;
    fields[3] = local.tm_hour;
    fields[4] = local.tm_min;
    fields[5] = local.tm_sec;
    fields[6] = local.tm_wday;
}


static void host_log(const char *text)
{
    host_busy++;
    raw_host_log(text);
    leave_host();
}

static void *host_file_open(const char *path)
{
    void *result;

    host_busy++;
    result = raw_host_file_open(path);
    leave_host();
    return result;
}

static int host_file_size(void *file)
{
    int result;

    host_busy++;
    result = raw_host_file_size(file);
    leave_host();
    return result;
}

static int host_file_read(void *file, unsigned offset, void *to, int count)
{
    int result;

    host_busy++;
    result = raw_host_file_read(file, offset, to, count);
    leave_host();
    return result;
}

static void host_file_close(void *file)
{
    host_busy++;
    raw_host_file_close(file);
    leave_host();
}

static int host_dir_exists(const char *path)
{
    int result;

    host_busy++;
    result = raw_host_dir_exists(path);
    leave_host();
    return result;
}

static void host_get_date(int fields[7])
{
    host_busy++;
    raw_host_get_date(fields);
    leave_host();
}

static int host_card_exists(int drive)
{
    int result;

    host_busy++;
    result = raw_host_card_exists(drive);
    leave_host();
    return result;
}

static int host_card_read(int drive, unsigned block, void *to)
{
    int result;

    host_busy++;
    result = raw_host_card_read(drive, block, to);
    leave_host();
    return result;
}

static int host_card_write(int drive, unsigned block, const void *from)
{
    int result;

    host_busy++;
    result = raw_host_card_write(drive, block, from);
    leave_host();
    return result;
}

static void host_video_begin(unsigned clear)
{
    host_busy++;
    if (have_video) {
        video_begin(clear);
    }
    leave_host();
}

static void host_video_texture(unsigned id, int width, int height, const unsigned *argb)
{
    host_busy++;
    if (have_video) {
        video_texture(id, width, height, argb);
    }
    leave_host();
}

static void host_video_draw(const struct host_draw *draw, const unsigned short *indices, int count)
{
    host_busy++;
    if (have_video) {
        video_draw(draw, indices, count);
    }
    leave_host();
}

/* the Xbox's pack, where HOST_PACK names one (video.c) */
static int host_video_texture_packed(unsigned id, unsigned key_lo, unsigned key_hi)
{
    int found;

    host_busy++;
    found = video_texture_packed(id, key_lo, key_hi, have_video);
    leave_host();
    return found;
}

static int host_video_texture_known(unsigned key_lo, unsigned key_hi)
{
    int known;

    host_busy++;
    known = video_texture_known(key_lo, key_hi);
    leave_host();
    return known;
}

static struct host_vertex *host_video_store(int *vertices)
{
    struct host_vertex *video_store(int *vertices);
    struct host_vertex *room;

    host_busy++;
    room = video_store(vertices);
    leave_host();
    return room;
}

static void host_video_draw_items(const struct host_item *items, int count)
{
    host_busy++;
    if (have_video) {
        video_draw_items(items, count);
    }
    leave_host();
}

/* the PC measures the presents alone, as the wait for a frame's show: its
 * sound runs on a thread of its own */
static void host_stats(unsigned out[4])
{
    extern unsigned video_present_us;
    static unsigned last;

    out[0] = 0;
    out[1] = have_video ? video_present_us - last : 0;
    out[2] = 0;
    out[3] = 0;
    last = video_present_us;
}

static struct host_vertex *host_video_arena(int *vertices)
{
    struct host_vertex *video_arena(int *vertices);
    struct host_vertex *room;

    host_busy++;
    room = video_arena(vertices);
    leave_host();
    return room;
}

/* all of video memory is mapped with the rest of the port's */
static int host_vram_commit(unsigned offset, unsigned size)
{
    (void)offset;
    (void)size;
    return 1;
}

static void host_video_vertices(const struct host_vertex *vertices, int count)
{
    host_busy++;
    if (have_video) {
        video_vertices(vertices, count);
    }
    leave_host();
}

/* HOST_SHOT=BLANK,...: the frame that shows at each BLANK, as
 * build/pc/shot_BLANK.ppm */
static const char *shots;

static void host_video_present(void)
{
    host_busy++;
    if (have_video) {
        while (shots != NULL && *shots != '\0') {
            char *end;
            unsigned at = (unsigned)strtoul(shots, &end, 10);
            char path[64];

            if (at > script_now()) {
                break;
            }
            snprintf(path, sizeof path, "build/pc/shot_%s%u.ppm", frame_clock != NULL ? "f" : "", at);
            video_shot(path);
            fprintf(stderr, "host: %s at blank %u, frame %u\n", path, blanks, frame_clock != NULL ? *frame_clock : 0);
            shots = *end == ',' ? end + 1 : end;
        }
        video_present();
        if (video_quit()) {
            fprintf(stderr, "host: the window closed\n");
            exit(0);
        }
    }
    leave_host();
}

/* the port calls the host through thunks, one for each of host_api's calls,
 * which run it on a stack of the host's: the game's stacks, an event's
 * thread's among them, are as small as the console's SDK needed, and the
 * host's calls take kilobytes. a thunk marks the host busy, moves to the
 * host's stack unless it is on it, copies 8 words of arguments, calls the
 * call, ends the busy time as leave_host does, and returns its result */
#define HOST_STACK_SIZE 0x100000
static char host_stack[HOST_STACK_SIZE] __attribute__((aligned(16)));
char *host_stack_low = host_stack;
char *host_stack_top = host_stack + HOST_STACK_SIZE;
void *host_targets[64];
#define THUNKS 33

void host_thunk_leave(void)
{
    leave_host();
}

#define THUNK(k)                                                                                                     \
    __asm__(".text\n.globl host_thunk_" #k "\nhost_thunk_" #k ":\n    movl $" #k ", %eax\n    jmp host_thunk_common\n")
THUNK(0); THUNK(1); THUNK(2); THUNK(3); THUNK(4); THUNK(5); THUNK(6); THUNK(7);
THUNK(8); THUNK(9); THUNK(10); THUNK(11); THUNK(12); THUNK(13); THUNK(14); THUNK(15);
THUNK(16); THUNK(17); THUNK(18); THUNK(19); THUNK(20); THUNK(21); THUNK(22); THUNK(23);
THUNK(24); THUNK(25); THUNK(26); THUNK(27); THUNK(28); THUNK(29); THUNK(30); THUNK(31);
THUNK(32);
__asm__(".text\n"
        "host_thunk_common:\n"
        "    lock incl host_busy\n"
        "    pushl %ebp\n"
        "    movl %esp, %ebp\n"
        "    pushl %esi\n"
        "    pushl %edi\n"
        "    pushl %ebx\n"
        "    movl %eax, %ebx\n"
        "    movl %esp, %eax\n"
        "    subl host_stack_low, %eax\n"
        "    cmpl $0x100000, %eax\n"
        "    jb 1f\n"
        /* off the host's stack: onto its slice for this depth of calls. a
         * call from a fiber that a held blank's handlers run, inside another
         * host call, mustn't land on that call's frames */
        "    movl host_busy, %eax\n"
        "    decl %eax\n"
        "    andl $3, %eax\n"
        "    shll $18, %eax\n"
        "    negl %eax\n"
        "    addl host_stack_top, %eax\n"
        "    movl %eax, %esp\n"
        "1:\n"
        "    subl $48, %esp\n"
        "    andl $-16, %esp\n"
        "    leal 8(%ebp), %esi\n"
        "    movl %esp, %edi\n"
        "    movl $8, %ecx\n"
        "    cld\n"
        "    rep movsl\n"
        "    call *host_targets(,%ebx,4)\n"
        "    movl %eax, %esi\n"
        "    movl %edx, %edi\n"
        "    call host_thunk_leave\n"
        "    movl %esi, %eax\n"
        "    movl %edi, %edx\n"
        "    leal -12(%ebp), %esp\n"
        "    popl %ebx\n"
        "    popl %edi\n"
        "    popl %esi\n"
        "    popl %ebp\n"
        "    ret\n");
void host_thunk_0(void), host_thunk_1(void), host_thunk_2(void), host_thunk_3(void), host_thunk_4(void),
    host_thunk_5(void), host_thunk_6(void), host_thunk_7(void), host_thunk_8(void), host_thunk_9(void),
    host_thunk_10(void), host_thunk_11(void), host_thunk_12(void), host_thunk_13(void), host_thunk_14(void),
    host_thunk_15(void), host_thunk_16(void), host_thunk_17(void), host_thunk_18(void), host_thunk_19(void),
    host_thunk_20(void), host_thunk_21(void), host_thunk_22(void), host_thunk_23(void), host_thunk_24(void),
    host_thunk_25(void), host_thunk_26(void), host_thunk_27(void), host_thunk_28(void), host_thunk_29(void),
    host_thunk_30(void), host_thunk_31(void), host_thunk_32(void);
static void (*const thunks[THUNKS])(void) = {
    host_thunk_0,  host_thunk_1,  host_thunk_2,  host_thunk_3,  host_thunk_4,  host_thunk_5,  host_thunk_6,
    host_thunk_7,  host_thunk_8,  host_thunk_9,  host_thunk_10, host_thunk_11, host_thunk_12, host_thunk_13,
    host_thunk_14, host_thunk_15, host_thunk_16, host_thunk_17, host_thunk_18, host_thunk_19, host_thunk_20,
    host_thunk_21, host_thunk_22, host_thunk_23, host_thunk_24, host_thunk_25, host_thunk_26, host_thunk_27,
    host_thunk_28, host_thunk_29, host_thunk_30, host_thunk_31, host_thunk_32,
};

/* the AICA's registers, which sound.cpp's AICA serves */
static unsigned host_aica_read(unsigned offset, int size)
{
    unsigned value;

    host_busy++;
    value = sound_reg_read(offset, size);
    leave_host();
    return value;
}

static void host_aica_write(unsigned offset, unsigned value, int size)
{
    host_busy++;
    sound_reg_write(offset, value, size);
    leave_host();
}

static void host_video_fog(unsigned table_color, unsigned vertex_color, float density, const unsigned short table[128])
{
    host_busy++;
    if (have_video) {
        video_fog(table_color, vertex_color, density, table);
    }
    leave_host();
}

/* the PC keeps every texture it's given */
static int host_video_textures_lost(void)
{
    return 0;
}

/* the PC maps the movie player's room with the rest of the port's memory */
static void *host_movie_memory(unsigned size)
{
    return size != 0 && size <= PORT_MOVIE_SIZE ? (void *)(uintptr_t)PORT_MOVIE : NULL;
}

/* a movie's frame from its planes: on the PC, converted to ARGB as
 * pl_mpeg's plm_frame_to_bgra does, and given as video_texture's */
static void host_video_movie_frame(unsigned id, int width, int height, const unsigned char *y_plane,
                                   const unsigned char *cb_plane, const unsigned char *cr_plane, int y_stride,
                                   int c_stride)
{
    static unsigned argb[640 * 480];
    int row, col;

    if (width < 2 || height < 2 || width * height > 640 * 480) {
        return;
    }
    host_busy++;
    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            int cr = cr_plane[(row >> 1) * c_stride + (col >> 1)] - 128;
            int cb = cb_plane[(row >> 1) * c_stride + (col >> 1)] - 128;
            int y = ((y_plane[row * y_stride + col] - 16) * 76309) >> 16;
            int r = y + ((cr * 104597) >> 16);
            int g = y - ((cb * 25674 + cr * 53278) >> 16);
            int b = y + ((cb * 132201) >> 16);

            r = r < 0 ? 0 : r > 255 ? 255 : r;
            g = g < 0 ? 0 : g > 255 ? 255 : g;
            b = b < 0 ? 0 : b > 255 ? 255 : b;
            argb[row * width + col] = 0xff000000u | (unsigned)r << 16 | (unsigned)g << 8 | (unsigned)b;
        }
    }
    if (have_video) {
        video_texture(id, width, height, argb);
    }
    leave_host();
}

static struct host_api api = {
    HOST_API_VERSION, host_log, host_exit, host_file_open, host_file_size, host_file_read, host_file_close,
    host_dir_exists, host_time_us, host_wait_vblank, host_pad_read, host_set_interrupt, host_get_date,
    host_card_exists, host_card_read, host_card_write,
    host_video_begin, host_video_texture, host_video_draw, host_video_present, host_video_fog,
    host_aica_read, host_aica_write, host_video_textures_lost, host_movie_memory, host_video_vertices,
    host_video_texture_packed, host_video_texture_known, host_vram_commit, host_video_store, host_video_draw_items,
    host_stats, host_video_arena, host_video_movie_frame,
};

/* map size bytes at address, of memory, or of the shared RAM object where
 * fd isn't -1 */
static int map_at(uint32_t address, uint32_t size, int fd)
{
    void *at = mmap((void *)(uintptr_t)address, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                    (fd < 0 ? MAP_PRIVATE | MAP_ANONYMOUS : MAP_SHARED) | MAP_FIXED_NOREPLACE, fd, 0);

    if (at != (void *)(uintptr_t)address) {
        fprintf(stderr, "host: can't map 0x%08x: %s\n", address, strerror(errno));
        return -1;
    }
    return 0;
}

/* the SH-4's FPU mode for the game's code, as its FPSCR holds it
 * (0x00040001: rounding toward zero, denormals as zero): SSE's rounding
 * toward zero and flush-to-zero, and denormals-are-zero where the CPU has
 * it (MXCSR_MASK, by FXSAVE: early Pentium IIIs, the Xbox's among them, may
 * not, and setting a bit outside the mask faults). because x86 starts each
 * thread rounding to nearest, which rounds every sum otherwise than the
 * console, the mode is set here; the x87, which the doubles take, the same,
 * at 53 bits (control word 0x0e7f) */
static unsigned fpu_as_console(void)
{
    static unsigned char area[512] __attribute__((aligned(16)));
    unsigned mask, csr;

    __asm__ volatile("fxsave %0" : "=m"(area));
    mask = *(unsigned *)(area + 28);
    if (mask == 0) {
        mask = 0xffbfu; /* the default: all but denormals-are-zero */
    }
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    csr |= 0x6000u | 0x8000u | (0x0040u & mask);
    csr &= mask;
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    {
        /* the x87's too, for the doubles FIPR's and FTRV's sums take
         * (xbox/include/sse/sh4_sse.h), as Flycast's SSE2 takes them under the
         * FPSCR's mode: rounding toward zero, a double's 53 bits */
        unsigned short word = 0x0e7f;

        __asm__ volatile("fldcw %0" : : "m"(word));
    }
    return csr;
}

int main(int argc, char **argv)
{
    char line[256];
    unsigned code_end = 0;
    FILE *areas;
    struct sigaction fault;
    void (*game_main)(void);

    if (argc > 1) {
        link_dir = argv[1];
    }
    if (argc > 2) {
        images_dir = argv[2];
    }
    if (argc > 3) {
        disc_dir = argv[3];
    }
    clock_gettime(CLOCK_MONOTONIC, &started);
    snprintf(line, sizeof line, "%s/code_areas.txt", link_dir);
    areas = fopen(line, "r");
    if (areas == NULL) {
        fprintf(stderr, "host: no %s\n", line);
        return 1;
    }
    while (fgets(line, sizeof line, areas) != NULL) {
        sscanf(line, "end 0x%x", &code_end);
    }
    fclose(areas);
    {
        int ram = memfd_create("ram", 0);

        if (ram < 0 || ftruncate(ram, AREA_SIZE) != 0 || map_at(RAM, AREA_SIZE, ram) ||
            map_at(0x8c000000u, AREA_SIZE, ram) || map_at(0xac000000u, AREA_SIZE, ram) ||
            map_at(0xe0000000u, AREA_SIZE, ram) || map_at(CODE, code_end - CODE, -1) ||
            map_at(0xa05f0000u, 0x10000, -1) || map_at(0xa0700000u, 0x10000, -1) ||
            map_at(0xa0800000u, 0x200000, -1)) {
            return 1;
        }
    }
    /* the AICA, over the sound RAM mapped at 0xa0800000 */
    sound_open((void *)(uintptr_t)0xa0800000u);
    memset(&fault, 0, sizeof fault);
    fault.sa_sigaction = on_fault;
    fault.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &fault, NULL);
    sigaction(SIGBUS, &fault, NULL);
    sigaction(SIGILL, &fault, NULL);
    sigaction(SIGFPE, &fault, NULL);
    if (load_image("1st_read", 0, &main_symbols) != 0) {
        return 1;
    }
    shots = getenv("HOST_SHOT");
    if (getenv("HOST_QUIT") != NULL) {
        quit_at = (unsigned)strtoul(getenv("HOST_QUIT"), NULL, 10);
    }
    have_video = getenv("HOST_HEADLESS") == NULL && video_open() == 0;
    if (!have_video) {
        fprintf(stderr, "host: no window: running headless\n");
    }
    watch = getenv("HOST_WATCH");
    pokes = getenv("HOST_POKE");
    pad_script = getenv("HOST_PADS");
    if (getenv("HOST_SAVES") != NULL) {
        saves_dir = getenv("HOST_SAVES");
    }
    {
        /* the port's table: api's version, then a thunk for each call */
        static struct host_api thunked;
        void **from = (void **)&api;
        void **to = (void **)&thunked;
        int calls = (int)(sizeof api / sizeof(void *)) - 1;
        int k;

        if (calls > THUNKS) {
            fprintf(stderr, "host: host_api has more calls than thunks\n");
            return 1;
        }
        to[0] = from[0];
        for (k = 0; k < calls; k++) {
            host_targets[k] = from[k + 1];
            to[k + 1] = (void *)thunks[k];
        }
        *(struct host_api **)HOST_API_SLOT = &thunked;
    }
    watched_mode = (volatile int *)(uintptr_t)symbol_address(&main_symbols, "game_mode");
    if (getenv("HOST_CLOCK") != NULL && strcmp(getenv("HOST_CLOCK"), "frames") == 0) {
        frame_clock = (volatile uint32_t *)(uintptr_t)symbol_address(&main_symbols, "_nj_vsync_count_");
        if (frame_clock == NULL) {
            fprintf(stderr, "host: HOST_CLOCK=frames, but 1st_read has no _nj_vsync_count_\n");
            return 1;
        }
    }
    game_main = (void (*)(void))(uintptr_t)symbol_address(&main_symbols, "main");
    if (game_main == NULL) {
        fprintf(stderr, "host: 1st_read has no main\n");
        return 1;
    }
    fprintf(stderr, "host: running main at %p, MXCSR 0x%04x\n", (void *)game_main, fpu_as_console());
    game_main();
    return 0;
}
