/* the port's host on the Xbox: an XBE, built with nxdk, that maps the port's
 * RAM and code areas at the console's addresses, loads 1ST_READ.BIN's
 * image, fills the port layer's struct host_api and runs the game's main,
 * as xbox/host/pc/host.c does on a PC. everything above struct host_api,
 * the images and the port layer, is the PC build's own: tools/pclink.py's
 * i386 images, run as they are.
 *
 * the disc, D:, which nxdk mounts at the XBE's folder, holds
 *   default.xbe       this host;
 *   modules.pak       the build's own: tools/pcdisc.py's x86 image files,
 *                     images.txt, code_areas.txt and pclink.py's images' code;
 *   disc.pak, music.pak, movies.pak   the Dreamcast disc's files, the
 *                     texture archives cut down to their keys (pack.py);
 *   textures.pak      the textures, as the NV2A takes them (xboxtex.c);
 *   host.txt          the settings the PC host takes from its environment,
 *                     a KEY=VALUE a line: HOST_PADS, HOST_POKE, HOST_CLOCK,
 *                     HOST_WATCH, HOST_QUIT (XBOX.md has them).
 * memory cards are E:\SONICADV\vmu_A1.bin and the like.
 *
 * memory. the console's RAM and the port's memory, area 3, are virtual
 * memory at 0x0c000000. the console's other views of it, 0x8c000000,
 * 0xac000000 and the store queues' 0xe0000000, are page tables of the
 * host's, whose entries are copies of area 3's: a title runs in ring 0, and
 * the kernel's page tables map themselves at 0xc0000000, its directory at
 * 0xc0300000. none of those views is the kernel's (the probe in probe/
 * found their directory entries empty). the register pages the kept SDK code
 * writes, 0xa05f0000, 0xa0700000 and sound RAM's 0xa0800000, are scratch
 * memory mapped the same way. the CPU's control page, 0xff000000, is the
 * Xbox's flash ROM, and the port never touches it.
 *
 * the vertical blank. the kernel has no call to read or set another
 * thread's registers, so the host chains the kernel's clock interrupt,
 * vector 0x30, 1000 times a second. at a blank, if the game's thread is the
 * one interrupted, in the game's code, where the port's may_interrupt lets
 * it, the host makes the interrupt return to blank_trampoline in place of
 * the game's eip: the trampoline moves to the port's interrupt stack, runs
 * the host's own chores for the blank, and enters the port's interrupt
 * entry with every register as the interrupt found it, as the PC host's
 * signal handler does. a blank during a host call runs as the call returns.
 * a blank that finds the game's thread elsewhere, in the kernel or another
 * thread, waits for the next tick.
 *
 * the game's stacks. a title runs in ring 0, where an interrupt pushes its
 * frames on the stack it interrupts, and the kernel runs its DPCs and its
 * thread switches there as the interrupt ends: on the game's fibers' stacks,
 * some of them 4 KB, as the backup library's, which that overflowed. so the
 * game's thread runs at DISPATCH_LEVEL, where the kernel defers both, and
 * drops to PASSIVE_LEVEL only in a host call, on the host's 1 MB stack,
 * where they then run. the clock's interrupt, at a level above, still comes.
 *
 * the log goes to a buffer in memory, after the line "HOST LOG", which
 * run.sh finds in a dump of the guest's memory, and a CPU fault in the game
 * is logged and halts. this host has no video or sound yet: those calls do
 * nothing, and the pads are HOST_PADS's alone */
#include <hal/video.h>
#include <nxdk/mount.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <xboxkrnl/xboxkrnl.h>

#include <SDL.h>

#include "../../port/host.h"
#include "../../port/port.h"
#include "video.h"
#include "aica_apu.h"

#ifdef HOST_WITH_SOUND
void xbox_sound_report(void);
int xbox_sound_open(void *ram);
void xbox_sound_blanks(unsigned blanks);
unsigned xbox_sound_read(unsigned offset, int size);
void xbox_sound_write(unsigned offset, unsigned value, int size);
#else
/* built without the AICA (the Makefile's SOUND): its registers read 0 */
static int xbox_sound_open(void *ram)
{
    (void)ram;
    return -1;
}
static void xbox_sound_blanks(unsigned blanks)
{
    (void)blanks;
}
static unsigned xbox_sound_read(unsigned offset, int size)
{
    (void)offset, (void)size;
    return 0;
}
static void xbox_sound_write(unsigned offset, unsigned value, int size)
{
    (void)offset, (void)value, (void)size;
}
static void aica_apu_report(void)
{
}
static void aica_apu_preload(const char *name)
{
    (void)name;
}
static void xbox_sound_report(void)
{
}
#endif
static int have_sound;
static UCHAR *sound_ram_kernel; /* sound RAM by its own address, for the APU */

#define RAM 0x0c000000u
/* to the end of what the renderer's room uses (PORT_RENDER_USED): the rest
 * of that room is only reserved, and the movie player's room is mapped from
 * video's as a movie plays (host_api's movie_memory) */
#define AREA_SIZE (PORT_RENDER + PORT_RENDER_USED - RAM)
#define CODE 0x10000000u
#define PAGE 0x1000u

/* the log */

/* run.sh finds the log in a dump of physical memory, so it lives in
 * contiguous memory, which log_text takes at its first line: a buffer in the
 * XBE's data is contiguous only by its virtual pages, and the log read from
 * the dump stopped, or ran into garbage, at its first page's end */
#define LOG_SIZE (1u << 18)
static char *log_buffer;
static unsigned log_length;
/* a report's start in the log, while one is made (report_begin): the
 * trimming below moves it with the lines */
static unsigned report_from;
static volatile unsigned log_written;
/* the log, once full, keeps its first LOG_KEEP bytes, the start's lines,
 * and drops the oldest of the rest, a quarter of it at a time, so that a
 * long run still logs its last minutes, a report or a fault among them */
#define LOG_KEEP (1u << 16)
#define LOG_MARK "host: [older lines dropped here: the log was full]\n"

static void log_trim(void)
{
    unsigned drop = LOG_SIZE / 4;
    unsigned mark = sizeof LOG_MARK - 1;
    unsigned from = LOG_KEEP + drop;

    /* the cut at a line's start */
    while (from < log_length && log_buffer[from - 1] != '\n') {
        from++;
    }
    memcpy(log_buffer + LOG_KEEP, LOG_MARK, mark);
    memmove(log_buffer + LOG_KEEP + mark, log_buffer + from, log_length - from);
    log_length -= from - LOG_KEEP - mark;
    if (report_from >= from) {
        report_from -= from - LOG_KEEP - mark;
    } else if (report_from > LOG_KEEP) {
        report_from = LOG_KEEP;
    }
    log_written = ~0u;
}

static void log_text(const char *text)
{
    if (log_buffer == NULL) {
        log_buffer = MmAllocateContiguousMemoryEx(LOG_SIZE, 0, 0x03ffffff, 0x1000, PAGE_READWRITE);
        if (log_buffer == NULL) {
            return;
        }
        memset(log_buffer, 0, LOG_SIZE);
        memcpy(log_buffer, "HOST LOG\n", sizeof "HOST LOG\n");
        log_length = sizeof "HOST LOG\n" - 1;
    }
    while (*text != '\0') {
        if (log_length >= LOG_SIZE - 64) {
            log_trim();
        }
        log_buffer[log_length++] = *text++;
    }
    log_buffer[log_length] = '\0';
}

static void logf_(const char *format, ...)
{
    char line[512];
    va_list args;

    va_start(args, format);
    vsnprintf(line, sizeof line, format, args);
    va_end(args);
    log_text(line);
}

/* the log's line, for video.c */
void host_log_line(const char *format, ...)
{
    char line[512];
    va_list args;

    va_start(args, format);
    vsnprintf(line, sizeof line, format, args);
    va_end(args);
    log_text(line);
}

/* stop: the log ends, and the machine waits for run.sh's dump */
/* the log on the hard disk, E:\SONICADV\log.txt, for a real Xbox, whose
 * memory run.sh can't read: written whole by a thread of its own, which
 * runs while the game waits for a blank: every 10 seconds under
 * HOST_LOG_FILE=1, with each report (the pad's Back, HOST_REPORT), after a
 * stall, and at a halt, a fault's among them, under HOST_CRASH_LOG */
/* HOST_LOG_FILE=1: the log goes to the disk every 10 seconds. off else, as
 * each write of it held up the game on a real Xbox */
static int log_file;
/* HOST_REPORT (on unless =0): the pad's Back makes a report. HOST_CRASH_LOG
 * (on unless =0): the log goes to the disk at a fault, a halt or a stall */
static int report_on = 1, crash_log = 1;

/* the last write that failed: its bytes written, of how many, and the
 * kernel's error, for the next report (report_host_part) */
static unsigned write_failures, write_failed_done, write_failed_length;
static DWORD write_failed_error;

static int write_file(const char *path, const char *bytes, unsigned length)
{
    HANDLE file;
    DWORD done = 0;

    CreateDirectoryA("E:\\SONICADV", NULL);
    file = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        write_failures++;
        write_failed_done = 0;
        write_failed_length = length;
        write_failed_error = GetLastError();
        return -1;
    }
    {
        /* in pieces of 4 KB: because larger writes came out empty where
         * free memory was low (the kernel's cache takes memory for each) */
        unsigned at = 0;

        while (at < length) {
            DWORD piece = length - at > 0x1000 ? 0x1000 : length - at, wrote = 0;

            if (!WriteFile(file, bytes + at, piece, &wrote, NULL) || wrote == 0) {
                write_failures++;
                write_failed_done = at;
                write_failed_length = length;
                write_failed_error = GetLastError();
                break;
            }
            at += wrote;
        }
        done = at;
    }
    {
        /* to the disk now: because the kernel's cache writes lazily, a halt
         * or the dashboard's reset right after left an empty log.txt */
        IO_STATUS_BLOCK status;

        NtFlushBuffersFile(file, &status);
    }
    CloseHandle(file);
    return done == length ? 0 : -1;
}

/* the log as it is now, if it has grown since it was last written */
static void log_write(void)
{
    unsigned length = log_length;

    if (log_buffer == NULL || length == log_written) {
        return;
    }
    /* to log.new, then in log.txt's place: a write that fails leaves the
     * last log.txt whole, not empty */
    if (write_file("E:\\SONICADV\\log.new", log_buffer, length) == 0) {
        DeleteFileA("E:\\SONICADV\\log.txt");
        if (MoveFileA("E:\\SONICADV\\log.new", "E:\\SONICADV\\log.txt")) {
            log_written = length;
        } else {
            write_failures++;
            write_failed_done = length;
            write_failed_length = length;
            write_failed_error = GetLastError();
        }
    }
}

/* HOST_LOG_FILE's write */
static void log_to_disk(void)
{
    if (log_file) {
        log_write();
    }
}

static void log_thread_tick(void);

static DWORD __stdcall log_thread(LPVOID unused)
{
    unsigned ticks = 0;

    (void)unused;
    for (;;) {
        Sleep(250);
        log_thread_tick();
        if (++ticks % 40 == 0) {
            log_to_disk();
        }
    }
    return 0;
}

static void halt(const char *why)
{
    logf_("host: %s\nHOST END\n", why);
    /* the log once more, where the halt allows the disk: a fault may come
     * at any level, so this is the best it can do */
    KfLowerIrql(PASSIVE_LEVEL);
    if (log_file || crash_log) {
        log_write();
    }
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

/* settings: host.txt's lines, as the PC host's environment */
static char settings[16384];

const char *setting(const char *key)
{
    size_t length = strlen(key);
    char *line = settings;

    while (line != NULL && *line != '\0') {
        if (strncmp(line, key, length) == 0 && line[length] == '=') {
            return line + length + 1;
        }
        line = strchr(line, '\0') + 1;
    }
    return NULL;
}

static void read_settings(void)
{
    FILE *file = fopen("D:\\host.txt", "rb");
    size_t size;
    size_t k;

    if (file == NULL) {
        return;
    }
    size = fread(settings, 1, sizeof settings - 2, file);
    fclose(file);
    /* a UTF-8 byte order mark, as Windows' Notepad may write, would hide the
     * first line's key */
    if (size >= 3 && memcmp(settings, "\xef\xbb\xbf", 3) == 0) {
        memmove(settings, settings + 3, size - 3);
        size -= 3;
    }
    settings[size] = settings[size + 1] = '\0';
    for (k = 0; k < size; k++) {
        if (settings[k] == '\n' || settings[k] == '\r') {
            settings[k] = '\0';
        }
    }
    /* empty lines end the walk early otherwise: close them up */
    for (k = 0; k + 1 < size; k++) {
        if (settings[k] == '\0' && settings[k + 1] == '\0') {
            memmove(settings + k, settings + k + 1, size - k);
            size--;
            k--;
        }
    }
}

/* memory */

#define PDE(va) (*(volatile ULONG *)(0xc0300000u + ((ULONG)(va) >> 22) * 4))
#define PTE(va) (*(volatile ULONG *)(0xc0000000u + ((ULONG)(va) >> 12) * 4))

static void *allocate(ULONG base, ULONG size, ULONG protect)
{
    PVOID at = (PVOID)base;
    SIZE_T length = size;
    NTSTATUS status = NtAllocateVirtualMemory(&at, 0, &length, MEM_RESERVE | MEM_COMMIT, protect);
    ULONG k;

    if (!NT_SUCCESS(status) || (base != 0 && (ULONG)at != base)) {
        logf_("host: can't allocate %lu bytes at 0x%08lx: status 0x%08lx\n", size, base, (ULONG)status);
        return NULL;
    }
    /* every page present, for its entry to be copied */
    for (k = 0; k < size; k += PAGE) {
        ((volatile UCHAR *)at)[k] = 0;
    }
    /* and locked: the kernel moves an unlocked page to make room for a
     * contiguous allocation, and updates its own entry for it, but not the
     * copies alias makes. the views then came apart: with area 3 mapped
     * 6 MB smaller, the texture pool's allocation moved sound RAM's pages,
     * the driver the game wrote at 0xa0800000 never reached the AICA's
     * view, and the ADX library read a position the driver never wrote
     * (XBOX.md, the ADX hang) */
    MmLockUnlockBufferPages(at, size, FALSE);
    return at;
}

/* memory for size bytes at base, in a range reserved before */
static void *commit(ULONG base, ULONG size, int lock)
{
    PVOID at = (PVOID)base;
    SIZE_T length = size;
    NTSTATUS status = NtAllocateVirtualMemory(&at, 0, &length, MEM_COMMIT, PAGE_READWRITE);
    ULONG k;

    if (!NT_SUCCESS(status) || (ULONG)at != base) {
        logf_("host: can't commit %lu bytes at 0x%08lx: status 0x%08lx\n", size, base, (ULONG)status);
        return NULL;
    }
    for (k = 0; k < size; k += PAGE) {
        ((volatile UCHAR *)at)[k] = 0;
    }
    /* locked where views alias it, as allocate says */
    if (lock) {
        MmLockUnlockBufferPages(at, size, FALSE);
    }
    return at;
}

/* video memory, PORT_VRAM, is reserved and backed as the port writes it
 * (host_api's vram_commit), 64 KB at a time: the textures of the pack never
 * reach it, and what it doesn't take is the textures' pool's. nothing
 * reads it through the console's other views, so it has no aliases */
static unsigned vram_committed;

static int have_video; /* video_open succeeded (main) */

static int raw_vram_commit(unsigned offset, unsigned size)
{
    if (offset >= PORT_VRAM_SIZE || size > PORT_VRAM_SIZE - offset) {
        return 0;
    }
    if (commit(PORT_VRAM + offset, size, 0) == NULL) {
        /* the kernel has no pages left: the textures' pool gives some
         * back, once, and the commit is tried again */
        if (!have_video || !video_give_memory(size) || commit(PORT_VRAM + offset, size, 0) == NULL) {
            return 0;
        }
    }
    vram_committed += size;
    return 1;
}

/* map size bytes at view onto the pages at from, by copies of their page
 * table entries, in page tables of the host's. pages not there, as video
 * memory's, stay unmapped in the view */
static int alias(ULONG view, ULONG from, ULONG size)
{
    ULONG k;

    for (k = 0; k < size; k += PAGE) {
        ULONG va = view + k;

        if (!(PDE(from + k) & 1) || !(PTE(from + k) & 1)) {
            continue;
        }

        if (!(PDE(va) & 1)) {
            ULONG *table = MmAllocateContiguousMemoryEx(PAGE, 0, 0x03ffffff, PAGE, PAGE_READWRITE);

            if (table == NULL) {
                logf_("host: no page table for 0x%08lx\n", va);
                return -1;
            }
            memset(table, 0, PAGE);
            PDE(va) = MmGetPhysicalAddress(table) | (PDE(from + k) & 0xfff);
        } else if (PDE(va) & 0x80) {
            logf_("host: 0x%08lx is the kernel's\n", va);
            return -1;
        }
        PTE(va) = PTE(from + k);
    }
    /* flush the TLB */
    __asm__ volatile("movl %%cr3, %%eax; movl %%eax, %%cr3" : : : "eax", "memory");
    return 0;
}

/* map size bytes at view onto physical memory from physical, or unmap them
 * where physical is ~0, with the flags of RAM's own entries, in page tables
 * of the host's where the view has none */
static int map_physical(ULONG view, ULONG physical, ULONG size)
{
    ULONG k;

    for (k = 0; k < size; k += PAGE) {
        ULONG va = view + k;

        if (!(PDE(va) & 1)) {
            ULONG *table = MmAllocateContiguousMemoryEx(PAGE, 0, 0x03ffffff, PAGE, PAGE_READWRITE);

            if (table == NULL) {
                logf_("host: no page table for 0x%08lx\n", va);
                return -1;
            }
            memset(table, 0, PAGE);
            PDE(va) = MmGetPhysicalAddress(table) | (PDE(RAM) & 0xfff);
        } else if (PDE(va) & 0x80) {
            logf_("host: 0x%08lx is the kernel's\n", va);
            return -1;
        }
        PTE(va) = physical == ~0ul ? 0 : (physical + k) | (PTE(RAM) & 0xfff);
    }
    __asm__ volatile("movl %%cr3, %%eax; movl %%eax, %%cr3" : : : "eax", "memory");
    return 0;
}

/* the movie player's room, PORT_MOVIE in area 3 and its views, as the game
 * addresses it: its DMA takes the console's views of its buffers. its pages
 * are video's lent ones (video_movie_memory), contiguous, mapped at each
 * view while a movie plays */
static void *movie_room(unsigned size)
{
    static const ULONG views[] = {0, 0x8c000000u - RAM, 0xac000000u - RAM, 0xe0000000u - RAM};
    void *room;
    int k;

    if (size == 0) {
        for (k = 0; k < 4; k++) {
            map_physical(PORT_MOVIE + views[k], ~0ul, PORT_MOVIE_SIZE);
        }
        video_movie_memory(0);
        return NULL;
    }
    if (size > PORT_MOVIE_SIZE || (room = video_movie_memory(PORT_MOVIE_SIZE)) == NULL) {
        return NULL;
    }
    for (k = 0; k < 4; k++) {
        if (map_physical(PORT_MOVIE + views[k], MmGetPhysicalAddress(room), PORT_MOVIE_SIZE) != 0) {
            video_movie_memory(0);
            return NULL;
        }
    }
    return (void *)PORT_MOVIE;
}

static int map_memory(ULONG code_end)
{
    UCHAR *scratch;

    {
        /* area 3's range, then its memory but video memory's */
        PVOID at = (PVOID)RAM;
        SIZE_T length = AREA_SIZE;

        if (!NT_SUCCESS(NtAllocateVirtualMemory(&at, 0, &length, MEM_RESERVE, PAGE_READWRITE)) || (ULONG)at != RAM) {
            logf_("host: can't reserve area 3\n");
            return -1;
        }
    }
    if (commit(RAM, PORT_VRAM - RAM, 1) == NULL || commit(PORT_RENDER, PORT_RENDER_USED, 1) == NULL ||
        allocate(CODE, code_end - CODE, PAGE_EXECUTE_READWRITE) == NULL) {
        return -1;
    }
    if (alias(0x8c000000u, RAM, AREA_SIZE) || alias(0xac000000u, RAM, AREA_SIZE) ||
        alias(0xe0000000u, RAM, AREA_SIZE)) {
        return -1;
    }
    {
        /* the renderer's unused room and the movie's, reserved, the movie's
         * for movie_room's mappings */
        PVOID at = (PVOID)(PORT_RENDER + PORT_RENDER_USED);
        SIZE_T length = PORT_MOVIE + PORT_MOVIE_SIZE - (PORT_RENDER + PORT_RENDER_USED);

        if (!NT_SUCCESS(NtAllocateVirtualMemory(&at, 0, &length, MEM_RESERVE, PAGE_READWRITE))) {
            logf_("host: can't reserve the movie's room\n");
        }
    }
    /* the PowerVR's and the AICA's register pages, 64 KB each, and the
     * AICA's 2 MB of sound RAM */
    scratch = allocate(0, 0x220000, PAGE_READWRITE);
    if (scratch == NULL || alias(0xa05f0000u, (ULONG)scratch, 0x10000) ||
        alias(0xa0700000u, (ULONG)scratch + 0x10000, 0x10000) ||
        alias(0xa0800000u, (ULONG)scratch + 0x20000, 0x200000)) {
        return -1;
    }
    sound_ram_kernel = scratch + 0x20000;
    logf_("host: the PowerVR's, the AICA's registers and sound RAM at 0x%08lx\n", (ULONG)scratch);
    /* each view is the same memory */
    *(volatile ULONG *)RAM = 0x5ea6a000u;
    if (*(volatile ULONG *)0x8c000000u != 0x5ea6a000u || *(volatile ULONG *)0xac000000u != 0x5ea6a000u ||
        *(volatile ULONG *)0xe0000000u != 0x5ea6a000u) {
        logf_("host: the views of RAM differ\n");
        return -1;
    }
    *(volatile ULONG *)RAM = 0;
    return 0;
}

/* images: ELF, as tools/pclink.py writes them */

typedef struct {
    UCHAR ident[16];
    USHORT type, machine;
    ULONG version, entry, phoff, shoff, flags;
    USHORT ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
} elf_header;
typedef struct {
    ULONG type, offset, vaddr, paddr, filesz, memsz, flags, align;
} elf_segment;
typedef struct {
    ULONG name, type, flags, addr, offset, size, link, info, addralign, entsize;
} elf_section;
typedef struct {
    ULONG name, value, size;
    UCHAR info, other;
    USHORT shndx;
} elf_symbol;

struct symbols {
    char name[32];
    elf_symbol *table;
    int count;
    char *strings;
};
static struct symbols main_symbols;
static char loaded_image[32];

/* the disc's packs (pack.py): D:\modules.pak, which a build writes whole
 * (the x86 images' data files, images.txt, code_areas.txt and each image's
 * code, an ELF, as code/IMAGE), and the Dreamcast disc's files in
 * D:\disc.pak, D:\music.pak and D:\movies.pak. a file is its pack's bytes
 * from an offset; a name is found in the packs in that order */
#define FILEPACK_MAGIC 0x4b504153 /* "SAPK" */
struct filepack_header {
    ULONG magic;
    ULONG count;
    ULONG size; /* the whole file's: a transfer cut short has less */
    ULONG build;
};
struct filepack_entry {
    char name[48];
    ULONG offset; /* from the file's start */
    ULONG size;
};
#define PACKS 4
#define PACK_MOVIES 3
static const char *const pack_names[PACKS] = {"modules", "disc", "music", "movies"};
static struct {
    HANDLE file;
    struct filepack_entry *entries;
    ULONG count;
} packs[PACKS];

static void halt(const char *why);

static int pack_read(int pack, ULONG offset, void *to, ULONG size)
{
    IO_STATUS_BLOCK status;
    LARGE_INTEGER at;

    at.QuadPart = offset;
    status.Information = 0;
    if (!NT_SUCCESS(NtReadFile(packs[pack].file, NULL, NULL, NULL, &status, to, size, &at))) {
        return 0;
    }
    return (int)status.Information;
}

static void packs_open(void)
{
    static char why[96];
    int k;

    for (k = 0; k < PACKS; k++) {
        struct filepack_header header;
        char path[32];
        ULONG bytes;

        snprintf(path, sizeof path, "D:\\%s.pak", pack_names[k]);
        packs[k].file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (packs[k].file == INVALID_HANDLE_VALUE) {
            logf_("host: no %s\n", path);
            continue;
        }
        bytes = 0;
        if (pack_read(k, 0, &header, sizeof header) != sizeof header || header.magic != FILEPACK_MAGIC ||
            header.count > 0x10000 || header.size != GetFileSize(packs[k].file, NULL) ||
            (packs[k].entries = malloc(bytes = header.count * sizeof *packs[k].entries)) == NULL ||
            pack_read(k, sizeof header, packs[k].entries, bytes) != (int)bytes) {
            snprintf(why, sizeof why, "%s isn't whole: copy it to the console again", path);
            halt(why);
        }
        packs[k].count = header.count;
        logf_("host: %s: %lu files, build %08lx\n", path, header.count, header.build);
    }
    if (packs[0].count == 0) {
        halt("no D:\\modules.pak");
    }
}

/* name's entry and its pack, or null */
static const struct filepack_entry *pack_find(const char *name, int *pack)
{
    int k;

    for (k = 0; k < PACKS; k++) {
        ULONG low = 0;
        ULONG high = packs[k].count;

        while (low < high) {
            ULONG mid = (low + high) / 2;
            int order = strcmp(packs[k].entries[mid].name, name);

            if (order == 0) {
                *pack = k;
                return &packs[k].entries[mid];
            }
            if (order < 0) {
                low = mid + 1;
            } else {
                high = mid;
            }
        }
    }
    return NULL;
}

/* a text file of the packs, whole, with a 0 after it; the caller frees it */
static char *pack_text(const char *name)
{
    int pack;
    const struct filepack_entry *entry = pack_find(name, &pack);
    char *text;

    if (entry == NULL || (text = malloc(entry->size + 1)) == NULL) {
        return NULL;
    }
    text[pack_read(pack, entry->offset, text, entry->size)] = '\0';
    return text;
}

/* load an image's segments, all or only its code's, and keep its symbols
 * where into isn't null */
static int load_image(const char *image, int code_only, struct symbols *into)
{
    char name[48];
    elf_header header;
    const struct filepack_entry *entry;
    int pack;
    int k;
#define READ_AT(at, to, bytes) (pack_read(pack, entry->offset + (at), to, bytes) == (int)(bytes) ? 0 : -1)

    snprintf(name, sizeof name, "code/%s", image);
    entry = pack_find(name, &pack);
    if (entry == NULL || READ_AT(0, &header, sizeof header) != 0) {
        logf_("host: no %s in the packs\n", name);
        return -1;
    }
    for (k = 0; k < header.phnum; k++) {
        elf_segment segment;

        if (READ_AT(header.phoff + (ULONG)k * header.phentsize, &segment, sizeof segment) != 0) {
            break;
        }
        if (segment.type != 1 || segment.memsz == 0 || (code_only && segment.vaddr < CODE)) {
            continue;
        }
        if (READ_AT(segment.offset, (void *)segment.vaddr, segment.filesz) != 0) {
            logf_("host: %s: a short segment\n", name);
        }
        memset((void *)(segment.vaddr + segment.filesz), 0, segment.memsz - segment.filesz);
    }
    for (k = 0; into != NULL && k < header.shnum; k++) {
        elf_section section;
        elf_section strings;

        if (READ_AT(header.shoff + (ULONG)k * header.shentsize, &section, sizeof section) != 0 || section.type != 2) {
            continue;
        }
        if (READ_AT(header.shoff + section.link * header.shentsize, &strings, sizeof strings) != 0) {
            break;
        }
        into->table = malloc(section.size);
        into->strings = malloc(strings.size);
        if (into->table == NULL || into->strings == NULL || READ_AT(section.offset, into->table, section.size) ||
            READ_AT(strings.offset, into->strings, strings.size)) {
            logf_("host: %s: no room for its symbols\n", name);
            break;
        }
        into->count = (int)(section.size / sizeof(elf_symbol));
        snprintf(into->name, sizeof into->name, "%s", image);
    }
#undef READ_AT
    return 0;
}

static ULONG symbol_address(struct symbols *in, const char *name)
{
    int k;

    for (k = 0; k < in->count; k++) {
        if (in->table[k].shndx != 0 && strcmp(in->strings + in->table[k].name, name) == 0) {
            return in->table[k].value;
        }
    }
    return 0;
}


static const char *nearest(struct symbols *in, ULONG address, ULONG *past)
{
    const char *best = NULL;
    ULONG at = 0;
    int k;

    for (k = 0; k < in->count; k++) {
        elf_symbol *s = &in->table[k];
        int type = s->info & 15;

        if (s->shndx == 0 || s->name == 0 || type == 3 || type == 4) {
            continue;
        }
        if (s->value <= address && s->value >= at) {
            at = s->value;
            best = in->strings + s->name;
        }
    }
    *past = address - at;
    return best;
}

static void report(const char *what, ULONG address)
{
    ULONG past = 0;
    const char *name = main_symbols.count ? nearest(&main_symbols, address, &past) : NULL;

    if (name != NULL && past < 0x10000) {
        logf_("host: %s 0x%08lx 1st_read:%s+0x%lx\n", what, address, name, past);
    } else {
        logf_("host: %s 0x%08lx (last image loaded: %s)\n", what, address, loaded_image);
    }
}

/* the images' files, from images.txt: a disc path and its image */
#define IMAGE_FILES 512
static struct {
    char path[64];
    char image[32];
} image_files[IMAGE_FILES];
static int image_file_count;

static void read_images(void)
{
    char *text = pack_text("images.txt");
    char *line;

    if (text == NULL) {
        logf_("host: no images.txt in the packs\n");
        return;
    }
    for (line = text; *line != '\0' && image_file_count < IMAGE_FILES;) {
        if (sscanf(line, "%63s %31s", image_files[image_file_count].path, image_files[image_file_count].image) == 2) {
            image_file_count++;
        }
        line += strcspn(line, "\n");
        line += *line == '\n';
    }
    free(text);
}

/* the host_api calls */

/* a blank during a host call waits for it to return: the call may hold the
 * C library's state, and the blank's handlers may switch fibers */
volatile int host_busy;
/* the longest of the game's outermost host calls since the last report,
 * which holds the blanks back while it runs (blank_owed): its kind, its
 * length in timestamp counts, and the blank it ended at */
static unsigned long long call_started, call_most;
static unsigned call_kind, call_most_kind, call_most_blank;
/* the frame's host calls by kind, outermost ones, in timestamp counts and
 * calls: a frame whose calls took more than 14 ms logs its three costliest
 * kinds as the next frame begins (video_begin), and a call of more than
 * 4 ms logs itself, so that a hitch names what it was */
static unsigned long long frame_kind_tsc[32];
static unsigned frame_kind_calls[32];
static unsigned host_names_frame(void);
static unsigned tsc_per_us(void);
static const char *const host_call_names[32] = {
    "log", "exit", "file_open", "file_size", "file_read", "file_close", "dir_exists", "time_us",
    "wait_vblank", "pad_read", "set_interrupt", "get_date", "card_exists", "card_read", "card_write",
    "video_begin", "video_texture", "video_draw", "video_present", "video_fog", "aica_read", "aica_write",
    "video_textures_lost", "movie_memory", "video_vertices", "video_texture_packed", "video_texture_known",
    "vram_commit", "video_store", "video_draw_items", "stats", "video_arena",
};

static unsigned long long host_irql_tsc; /* the IRQL's changes, for the profile */
static volatile int blank_owed;
static const struct host_interrupt *interrupt;
static volatile unsigned blanks;
static void blank_chores(void);
/* the performance counter's ticks the sound's emulation has taken, for
 * host_api's stats */
static ULONGLONG sound_total_ticks;
static ULONGLONG sound_most_ticks; /* the most one blank's chores took, since the last report */


extern unsigned long long video_swap_wait_tsc; /* video.c's wait_off_screen, in this video_begin */

static void leave_host(void)
{
    if (host_busy == 1) {
        unsigned long long took = __builtin_ia32_rdtsc() - call_started;

        if (took > call_most) {
            call_most = took;
            call_most_kind = call_kind;
            call_most_blank = blanks;
        }
        if (call_kind == 15) {
            /* video_begin: the frame before is done. its own time (the wait
             * for the NV2A) is the next frame's */
            unsigned long long total = 0;
            unsigned per_us = tsc_per_us();
            int k;

            for (k = 0; k < 32; k++) {
                total += k == 8 ? 0 : frame_kind_tsc[k];
            }
            /* the GPU profile's frames are all slow by design: not logged */
            if (total > 14000ull * per_us && !video_gpu_profiling()) {
                int best[3] = {-1, -1, -1}, j;
                char line[256];
                int at = 0;

                for (k = 0; k < 32; k++) {
                    for (j = 0; j < 3; j++) {
                        if (best[j] < 0 || frame_kind_tsc[k] > frame_kind_tsc[best[j]]) {
                            int m;

                            for (m = 2; m > j; m--) {
                                best[m] = best[m - 1];
                            }
                            best[j] = k;
                            break;
                        }
                    }
                }
                for (j = 0; j < 3; j++) {
                    if (best[j] >= 0 && frame_kind_tsc[best[j]] != 0) {
                        at += snprintf(line + at, sizeof line - (size_t)at, "%s %s %lu us (%u)", j ? "," : "",
                                       host_call_names[best[j]], (ULONG)(frame_kind_tsc[best[j]] / per_us),
                                       frame_kind_calls[best[j]]);
                    }
                }
                logf_("host: frame %u's host calls took %lu us:%s\n", host_names_frame(), (ULONG)(total / per_us),
                      line);
            }
            memset(frame_kind_tsc, 0, sizeof frame_kind_tsc);
            memset(frame_kind_calls, 0, sizeof frame_kind_calls);
        }
        if (call_kind == 15) {
            /* video_begin's wait for the last frame's swap is a wait by
             * design, as the blank's is: at 720p it is every frame's, and
             * its lines filled the log. the longest call above keeps it */
            took -= video_swap_wait_tsc < took ? video_swap_wait_tsc : took;
        }
        frame_kind_tsc[call_kind] += took;
        frame_kind_calls[call_kind]++;
        /* the blank's wait (wait_vblank) is a wait by design, and the NV2A's
         * at a frame's begin common: because they filled the log, not logged */
        if (call_kind != 8 && took > (call_kind == 15 ? 12000ull : 4000ull) * tsc_per_us() && !video_gpu_profiling()) {
            logf_("host: a %s of %lu us, at blank %u, frame %u\n", host_call_names[call_kind],
                  (ULONG)(took / tsc_per_us()), blanks, host_names_frame());
        }
    }
    while (host_busy == 1 && blank_owed) {
        blank_owed = 0;
        blank_chores();
        if (interrupt != NULL) {
            interrupt->run_now();
        }
    }
    /* back to the game: DISPATCH_LEVEL, as the outermost call returns, if
     * the call lowered it */
    if (host_busy == 1 && KeGetCurrentIrql() < DISPATCH_LEVEL) {
        unsigned long long t0 = __builtin_ia32_rdtsc();

        KeRaiseIrqlToDpcLevel();
        host_irql_tsc += __builtin_ia32_rdtsc() - t0;
    }
    host_busy--;
}

/* into a host call: PASSIVE_LEVEL, where the kernel may wait */
/* the calls that run at DISPATCH_LEVEL, as the game does, by their index
 * in host_api after its version: those that only write memory, push to the
 * NV2A or spin, of which the game makes thousands a frame. lowering the IRQL
 * to PASSIVE_LEVEL, and raising it after, delivers the DPCs pending and lets
 * other threads run, which the rest need: the files, the cards, the blank's
 * wait, the pads (SDL's locks), the present and the movie's memory */
static const unsigned char host_fast[32] = {
    [0] = 1,  /* log */
    [7] = 1,  /* time_us */
    [15] = 1, /* video_begin */
    [16] = 1, /* video_texture */
    [17] = 1, /* video_draw */
    [19] = 1, /* video_fog */
    [20] = 1, /* aica_read */
    [21] = 1, /* aica_write */
    [22] = 1, /* video_textures_lost */
    [24] = 1, /* video_vertices */
    /* not video_texture_known, 26: it reads the pack's texels as the game
     * loads a texture (video.c's pack_load), which takes PASSIVE_LEVEL */
    [28] = 1, /* video_store */
    [29] = 1, /* video_draw_items */
    [30] = 1, /* stats */
    [31] = 1, /* video_arena */
};

/* the calls by kind, and the timestamp counts lowering and raising the IRQL
 * took, for the profile */
static unsigned host_calls[32];

void host_lower_irql(unsigned k)
{
    host_calls[k & 31]++;
    if (host_busy == 1) {
        call_started = __builtin_ia32_rdtsc();
        call_kind = k & 31;
    }
    if (!host_fast[k & 31] && KeGetCurrentIrql() > PASSIVE_LEVEL) {
        unsigned long long t0 = __builtin_ia32_rdtsc();

        KfLowerIrql(PASSIVE_LEVEL);
        host_irql_tsc += __builtin_ia32_rdtsc() - t0;
    }
}

void host_thunk_leave(void)
{
    leave_host();
}

static void raw_log(const char *text)
{
    logf_("game: %s\n", text);
}

static void raw_exit(int status)
{
    char why[32];

    snprintf(why, sizeof why, "the game exited with %d", status);
    halt(why);
}

/* a disc path as images.txt and the disc hold it: no leading slash,
 * capitals, slashes */
static void normalize(const char *path, char *out, size_t room)
{
    size_t k = 0;

    while (*path == '/' || *path == '\\') {
        path++;
    }
    for (; *path != '\0' && k + 1 < room; path++) {
        char c = *path == '\\' ? '/' : *path;

        out[k++] = c >= 'a' && c <= 'z' ? (char)(c - 'a' + 'A') : c;
    }
    out[k] = '\0';
}

struct host_file {
    int pack;
    ULONG offset;
    ULONG size;
};

static void *raw_file_open(const char *path)
{
    char name[256];
    struct host_file *handle;
    const struct filepack_entry *entry;
    int pack;
    int k;

    normalize(path, name, sizeof name);
    entry = pack_find(name, &pack);
    if (entry == NULL) {
        return NULL;
    }
    for (k = 0; k < image_file_count; k++) {
        if (strcmp(image_files[k].path, name) == 0) {
            load_image(image_files[k].image, 1, NULL);
            snprintf(loaded_image, sizeof loaded_image, "%s", image_files[k].image);
            logf_("host: loaded %s's code\n", image_files[k].image);
            break;
        }
    }
    {
        /* a sound bank: its samples converted ahead, from the sound pack
         * (aica_apu.c), read here where the game is loading anyway */
        size_t length = strlen(name);
        const char *base = strrchr(name, '/');

        if (have_sound && length > 4 && strcmp(name + length - 4, ".MLT") == 0) {
            aica_apu_preload(base != NULL ? base + 1 : name);
        }
    }
    handle = malloc(sizeof *handle);
    handle->pack = pack;
    handle->offset = entry->offset;
    handle->size = entry->size;
    return handle;
}

static int raw_file_size(void *file)
{
    return (int)((struct host_file *)file)->size;
}

static int raw_file_read(void *file, unsigned offset, void *to, int count)
{
    struct host_file *handle = file;

    if (offset >= handle->size || count <= 0) {
        return 0;
    }
    if ((ULONG)count > handle->size - offset) {
        count = (int)(handle->size - offset);
    }
    return pack_read(handle->pack, handle->offset + offset, to, (ULONG)count);
}

static void raw_file_close(void *file)
{
    free(file);
}

/* a directory is there where some file's name starts with it */
static int raw_dir_exists(const char *path)
{
    char name[256];
    size_t length;
    ULONG e;
    int k;

    normalize(path, name, sizeof name);
    length = strlen(name);
    if (length == 0) {
        return 1;
    }
    for (k = 0; k < PACKS; k++) {
        for (e = 0; e < packs[k].count; e++) {
            if (strncmp(packs[k].entries[e].name, name, length) == 0 && packs[k].entries[e].name[length] == '/') {
                return 1;
            }
        }
    }
    return 0;
}

static ULONGLONG started;
static ULONGLONG frequency;
static unsigned long long host_start_tsc; /* the timestamp counter at started, for its rate */
/* the timestamp counter's counts a microsecond, by the performance
 * counter's since the start, measured again each second at most */
static unsigned tsc_per_us(void)
{
    static unsigned rate = 733;
    static unsigned long long measured_at;
    unsigned long long tsc = __builtin_ia32_rdtsc();

    if (tsc - measured_at > 1000000000ull && frequency != 0) {
        ULONGLONG qpc = KeQueryPerformanceCounter();

        if (qpc > started + frequency / 10) {
            rate = (unsigned)((tsc - host_start_tsc) * frequency / (qpc - started) / 1000000);
        }
        measured_at = tsc;
    }
    return rate ? rate : 733;
}
#ifdef HOST_WITH_SOUND
extern unsigned long long xbox_sound_ticks_arm, xbox_sound_ticks_apu;
#else
static unsigned long long xbox_sound_ticks_arm, xbox_sound_ticks_apu;
#endif

/* the performance counter, for sound_xbox.cpp's profile */
unsigned long long host_ticks(void)
{
    return KeQueryPerformanceCounter();
}

unsigned long long host_tick_rate(void)
{
    return frequency;
}

/* microseconds by a multiply: the game asks the time often (the ADX server
 * five times a pass), and a 64-bit division a call, __aulldiv, was 3% of a
 * real Xbox's time in a stage. us_scale is 2^32 microseconds a tick */
static ULONGLONG us_scale;

static unsigned raw_time_us(void)
{
    ULONGLONG ticks = KeQueryPerformanceCounter() - started;

    if (us_scale == 0) {
        us_scale = (1000000ULL << 32) / frequency;
    }
    return (unsigned)((ticks >> 32) * us_scale + (((ticks & 0xffffffffu) * us_scale) >> 32));
}

static void raw_wait_vblank(void)
{
    unsigned was = blanks;

    while (blanks == was) {
        Sleep(1);
    }
}

/* HOST_CLOCK=frames: HOST_PADS, HOST_POKE and HOST_QUIT count the frames the
 * game has rendered, Ninja's _nj_vsync_count_, as on the PC */
static volatile ULONG *frame_clock;
static volatile ULONG *frames_rendered; /* Ninja's _nj_vsync_count_, for the profile */

/* the frame the game has rendered, for the hitches' log lines */
static unsigned host_names_frame(void)
{
    return frames_rendered != NULL ? (unsigned)*frames_rendered : 0;
}
static int profiling; /* HOST_PROFILE=1: the profile every 600 blanks */

static unsigned script_now(void)
{
    return frame_clock != NULL ? *frame_clock : blanks;
}

unsigned host_script_now(void)
{
    return script_now();
}

/* HOST_PADS=AT:BUTTONS[/X/Y],...: port A's buttons, PDD_DGT_ bits in hex */
static const char *pad_script;

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

/* the pads: port A is the first game controller SDL finds, opened as it
 * comes and closed as it goes, read once a blank, its buttons as the PC
 * host's (xbox/host/pc/video.c): A, B, X, Y and Start as the console's, the
 * d-pad, the left stick, the triggers as L and R, and the right stick's
 * left and right as L and R too, the camera's turn. HOST_PADS' scripted
 * buttons add to them */
static int have_pads;
static SDL_GameController *pad;
static volatile int report_wanted; /* the pad's Back, for blank_chores */

static void pads_open(void)
{
    have_pads = SDL_Init(SDL_INIT_GAMECONTROLLER) == 0;
    if (!have_pads) {
        logf_("host: no game controllers: %s\n", SDL_GetError());
        return;
    }
    /* the host reads the pads' state (pad_buttons), never SDL's events:
     * because each move of a stick was queued and never taken, leaking
     * memory, those events are ignored */
    SDL_GameControllerEventState(SDL_IGNORE);
    SDL_JoystickEventState(SDL_IGNORE);
}

/* back to the dashboard, by the machine's reset. the kernel's quick reboot
 * keeps the machine as the host leaves it, its clock interrupt aimed at the
 * host's code and its page tables with the console's views, and a real
 * Xbox went to a black screen on it. the clock's vector goes back to the
 * kernel's first, for the kernel's own way out */
extern void *clock_original;

static void to_dashboard(void)
{
    struct {
        USHORT limit;
        ULONG base;
    } __attribute__((packed)) idtr;

    if (clock_original != NULL) {
        USHORT *gate;

        __asm__ volatile("cli");
        __asm__ volatile("sidt %0" : "=m"(idtr));
        gate = (USHORT *)(idtr.base + 0x30 * 8);
        gate[0] = (USHORT)(ULONG)clock_original;
        gate[3] = (USHORT)((ULONG)clock_original >> 16);
        __asm__ volatile("sti");
    }
    HalReturnToFirmware(HalRebootRoutine);
}

/* the right stick's dead zone, percent of its throw across: wider than the
 * left stick's (HOST_STICK_DEAD), so that a thumb resting on it turns the
 * camera less */
#define RIGHT_STICK_DEAD 20

static unsigned pad_buttons(int *x, int *y, int *l, int *r)
{
    static const struct {
        SDL_GameControllerButton button;
        unsigned bit;
    } map[] = {
        {SDL_CONTROLLER_BUTTON_A, 1u << 2},         {SDL_CONTROLLER_BUTTON_B, 1u << 1},
        {SDL_CONTROLLER_BUTTON_X, 1u << 10},        {SDL_CONTROLLER_BUTTON_Y, 1u << 9},
        {SDL_CONTROLLER_BUTTON_START, 1u << 3},     {SDL_CONTROLLER_BUTTON_DPAD_UP, 1u << 4},
        {SDL_CONTROLLER_BUTTON_DPAD_DOWN, 1u << 5}, {SDL_CONTROLLER_BUTTON_DPAD_LEFT, 1u << 6},
        {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, 1u << 7},
    };
    unsigned buttons = 0;
    size_t k;

    if (!have_pads) {
        return 0;
    }
    SDL_GameControllerUpdate();
    /* and whatever events came anyway (a controller's arrival among them),
     * dropped as they come */
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    if (pad != NULL && !SDL_GameControllerGetAttached(pad)) {
        SDL_GameControllerClose(pad);
        pad = NULL;
    }
    for (k = 0; pad == NULL && (int)k < SDL_NumJoysticks(); k++) {
        if (SDL_IsGameController((int)k)) {
            pad = SDL_GameControllerOpen((int)k);
            if (pad != NULL) {
                logf_("host: pad A is %s\n", SDL_GameControllerName(pad));
            }
        }
    }
    if (pad == NULL) {
        return 0;
    }
    for (k = 0; k < sizeof map / sizeof map[0]; k++) {
        if (SDL_GameControllerGetButton(pad, map[k].button)) {
            buttons |= map[k].bit;
        }
    }
    {
        /* Back, which the console's pad hasn't: a report (blank_chores),
         * under HOST_REPORT */
        static int back_was;
        int back = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_BACK);

        /* both triggers, Start and Back together: back to the dashboard,
         * the log written first. the triggers count from halfway in */
        if (back && SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_START) &&
            SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 16384 &&
            SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 16384) {
            logf_("host: both triggers, Start and Back: to the dashboard\nHOST END\n");
            if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
                KfLowerIrql(PASSIVE_LEVEL);
            }
            log_write();
            to_dashboard();
        }
        if (back && !back_was && SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT) <= 16384 &&
            SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) <= 16384) {
            report_wanted = report_on;
        }
        back_was = back;
    }
    /* SDL's -32768 to 32767 as the console's 0 to 255, 128 at rest, past a
     * dead zone (host_stick; HOST_DEADZONE=PERCENT of the throw, 0 for
     * none), and its triggers' 0 to 32767 as 0 to 255 */
    {
        static int dead = -1;

        if (dead < 0) {
            unsigned long percent =
                setting("HOST_DEADZONE") != NULL ? strtoul(setting("HOST_DEADZONE"), NULL, 10) : HOST_STICK_DEAD;

            dead = (int)((percent > 90 ? 90 : percent) * 32767 / 100);
        }
        host_stick(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX),
                   SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY), dead, x, y);
    }
    *l = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 128;
    *r = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 128;
    {
        /* the right stick, which the console's pad hasn't, turns the camera
         * as the triggers do: its left as L, its right as R, by how far
         * past the dead zone, the deeper of stick and trigger taken */
        int across = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_RIGHTX);
        int dead = 32767 * RIGHT_STICK_DEAD / 100;
        int depth = across < 0 ? -across : across;

        if (depth > dead) {
            int value = (depth - dead) * 255 / (32767 - dead);

            value = value > 255 ? 255 : value;
            if (across < 0 && value > *l) {
                *l = value;
            } else if (across > 0 && value > *r) {
                *r = value;
            }
        }
    }
    return buttons;
}

static unsigned raw_pad_read(int port, int *x, int *y, int *l, int *r)
{
    unsigned buttons;

    *x = *y = 128;
    *l = *r = 0;
    if (port != 0) {
        return ~0u;
    }
    buttons = pad_buttons(x, y, l, r);
    return buttons | scripted_buttons(x, y);
}

static void raw_get_date(int fields[7])
{
    TIME_FIELDS t;
    LARGE_INTEGER now;

    KeQuerySystemTime(&now);
    RtlTimeToTimeFields(&now, &t);
    fields[0] = t.Year;
    fields[1] = t.Month;
    fields[2] = t.Day;
    fields[3] = t.Hour;
    fields[4] = t.Minute;
    fields[5] = t.Second;
    fields[6] = t.Weekday;
}

/* memory cards: E:\SONICADV\vmu_A1.bin and the like */
static void card_path(int drive, char *out, size_t room)
{
    snprintf(out, room, "E:\\SONICADV\\vmu_%c%d.bin", 'A' + drive / 2, drive % 2 + 1);
}

static int raw_card_exists(int drive)
{
    char path[64];

    card_path(drive, path, sizeof path);
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

/* a card's file, opened for its block at block, or INVALID_HANDLE_VALUE:
 * the Win32 calls, which nxdk makes on the kernel's, since pdclib's fopen
 * takes no update mode on the Xbox */
static HANDLE card_open(int drive, unsigned block, DWORD access, DWORD disposition)
{
    char path[64];
    HANDLE file;

    card_path(drive, path, sizeof path);
    file = CreateFileA(path, access, 0, NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file != INVALID_HANDLE_VALUE && SetFilePointer(file, (LONG)(block * 512), NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        CloseHandle(file);
        return INVALID_HANDLE_VALUE;
    }
    return file;
}

static int raw_card_read(int drive, unsigned block, void *to)
{
    HANDLE file = card_open(drive, block, GENERIC_READ, OPEN_EXISTING);
    DWORD got = 0;
    BOOL ok;

    if (file == INVALID_HANDLE_VALUE) {
        return -1;
    }
    ok = ReadFile(file, to, 512, &got, NULL) && got == 512;
    CloseHandle(file);
    return ok ? 0 : -1;
}

static int raw_card_write(int drive, unsigned block, const void *from)
{
    HANDLE file;
    DWORD put = 0;
    BOOL ok;

    CreateDirectoryA("E:\\SONICADV", NULL);
    file = card_open(drive, block, GENERIC_READ | GENERIC_WRITE, OPEN_ALWAYS);
    if (file == INVALID_HANDLE_VALUE) {
        return -1;
    }
    ok = WriteFile(file, from, 512, &put, NULL) && put == 512;
    ok = CloseHandle(file) && ok;
    return ok ? 0 : -1;
}

/* video: video.c's, where it opened */
static int have_video;

static void raw_video_begin(unsigned clear)
{
    if (have_video) {
        video_begin(clear);
    }
}

static void raw_video_texture(unsigned id, int width, int height, const unsigned *argb)
{
    if (have_video) {
        video_texture(id, width, height, argb);
    }
}

static void raw_video_draw(const struct host_draw *draw, const unsigned short *indices, int count)
{
    if (have_video) {
        video_draw(draw, indices, count);
    }
}

static int raw_video_texture_packed(unsigned id, unsigned key_lo, unsigned key_hi)
{
    return have_video ? video_texture_packed(id, key_lo, key_hi) : 0;
}

static struct host_vertex *raw_video_store(int *vertices)
{
    if (!have_video) {
        *vertices = 0;
        return NULL;
    }
    return video_store(vertices);
}

static void raw_video_draw_items(const struct host_item *items, int count)
{
    if (have_video) {
        video_draw_items(items, count);
    }
}

static struct host_vertex *raw_video_arena(int *vertices)
{
    if (!have_video) {
        *vertices = 0;
        return NULL;
    }
    return video_arena(vertices);
}

/* since the last call, in microseconds: the waits for the NV2A and for the
 * display's blank (video.c), and the sound's emulation */
static void raw_stats(unsigned out[4])
{
    extern ULONGLONG video_gpu_wait_ticks, video_blank_wait_ticks;
    static ULONGLONG last[3];
    ULONGLONG now[3];
    int k;

    now[0] = video_gpu_wait_ticks;
    now[1] = video_blank_wait_ticks;
    now[2] = sound_total_ticks;
    for (k = 0; k < 3; k++) {
        out[k] = frequency != 0 ? (unsigned)((now[k] - last[k]) * 1000000ULL / frequency) : 0;
        last[k] = now[k];
    }
    out[3] = 0;
}

static int raw_video_texture_known(unsigned key_lo, unsigned key_hi)
{
    return have_video ? video_texture_known(key_lo, key_hi) : 0;
}

static void raw_video_vertices(const struct host_vertex *vertices, int count)
{
    if (have_video) {
        video_vertices(vertices, count);
    }
}

static void raw_video_present(void)
{
    if (have_video) {
        video_present();
    }
}

static void raw_video_fog(unsigned table_color, unsigned vertex_color, float density, const unsigned short table[128])
{
    if (have_video) {
        video_fog(table_color, vertex_color, density, table);
    }
}

static int raw_video_textures_lost(void)
{
    return have_video ? video_textures_lost() : 0;
}

/* the movie player's room: video lends it from its textures' (video.c),
 * and movie_room maps it where the port's memory has it */
static void *movie_room(unsigned size);
static void *raw_movie_memory(unsigned size)
{
    return have_video ? movie_room(size) : NULL;
}

/* the AICA's registers: sound_xbox.cpp's AICA, where sound runs */
static unsigned raw_aica_read(unsigned offset, int size)
{
    return have_sound ? xbox_sound_read(offset, size) : 0;
}

static void raw_aica_write(unsigned offset, unsigned value, int size)
{
    if (have_sound) {
        xbox_sound_write(offset, value, size);
    }
}

/* the vertical blank */

static void raw_set_interrupt(const struct host_interrupt *with);

static struct host_api api = {
    HOST_API_VERSION, raw_log, raw_exit, raw_file_open, raw_file_size, raw_file_read, raw_file_close,
    raw_dir_exists, raw_time_us, raw_wait_vblank, raw_pad_read, raw_set_interrupt, raw_get_date,
    raw_card_exists, raw_card_read, raw_card_write,
    raw_video_begin, raw_video_texture, raw_video_draw, raw_video_present, raw_video_fog,
    raw_aica_read, raw_aica_write, raw_video_textures_lost, raw_movie_memory, raw_video_vertices,
    raw_video_texture_packed, raw_video_texture_known, raw_vram_commit, raw_video_store, raw_video_draw_items,
    raw_stats, raw_video_arena,
};

/* every call runs on the host's stack, through a thunk, as on the PC: the
 * game's stacks are as small as the console's */
#define THUNKS 32
#define HOST_STACK_SIZE 0x100000
static char host_stack[HOST_STACK_SIZE] __attribute__((aligned(16)));
char *host_stack_low = host_stack;
char *host_stack_top = host_stack + HOST_STACK_SIZE;
void *host_targets[THUNKS];

#define THUNK(k)                                                                                                     \
    __asm__(".text\n.globl _host_thunk_" #k "\n_host_thunk_" #k ":\n    movl $" #k ", %eax\n    jmp host_thunk_common\n")
THUNK(0); THUNK(1); THUNK(2); THUNK(3); THUNK(4); THUNK(5); THUNK(6); THUNK(7);
THUNK(8); THUNK(9); THUNK(10); THUNK(11); THUNK(12); THUNK(13); THUNK(14); THUNK(15);
THUNK(16); THUNK(17); THUNK(18); THUNK(19); THUNK(20); THUNK(21); THUNK(22); THUNK(23);
THUNK(24); THUNK(25); THUNK(26); THUNK(27); THUNK(28); THUNK(29); THUNK(30); THUNK(31);
__asm__(".text\n"
        "host_thunk_common:\n"
        "    lock incl _host_busy\n"
        "    pushl %ebp\n"
        "    movl %esp, %ebp\n"
        "    pushl %esi\n"
        "    pushl %edi\n"
        "    pushl %ebx\n"
        "    movl %eax, %ebx\n"
        "    movl %esp, %eax\n"
        "    subl _host_stack_low, %eax\n"
        "    cmpl $0x100000, %eax\n"
        "    jb 1f\n"
        /* off the host's stack: onto its slice for this depth of calls. a
         * call from a fiber that a held blank's handlers run, inside another
         * host call, mustn't land on that call's frames */
        "    movl _host_busy, %eax\n"
        "    decl %eax\n"
        "    andl $3, %eax\n"
        "    shll $18, %eax\n"
        "    negl %eax\n"
        "    addl _host_stack_top, %eax\n"
        "    movl %eax, %esp\n"
        "1:\n"
        "    subl $48, %esp\n"
        "    andl $-16, %esp\n"
        "    pushl %ebx\n"
        "    call _host_lower_irql\n"
        "    addl $4, %esp\n"
        "    leal 8(%ebp), %esi\n"
        "    movl %esp, %edi\n"
        "    movl $8, %ecx\n"
        "    cld\n"
        "    rep movsl\n"
        "    call *_host_targets(,%ebx,4)\n"
        "    movl %eax, %esi\n"
        "    movl %edx, %edi\n"
        "    call _host_thunk_leave\n"
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
    host_thunk_30(void), host_thunk_31(void);
static void (*const thunks[THUNKS])(void) = {
    host_thunk_0,  host_thunk_1,  host_thunk_2,  host_thunk_3,  host_thunk_4,  host_thunk_5,  host_thunk_6,
    host_thunk_7,  host_thunk_8,  host_thunk_9,  host_thunk_10, host_thunk_11, host_thunk_12, host_thunk_13,
    host_thunk_14, host_thunk_15, host_thunk_16, host_thunk_17, host_thunk_18, host_thunk_19, host_thunk_20,
    host_thunk_21, host_thunk_22, host_thunk_23, host_thunk_24, host_thunk_25, host_thunk_26, host_thunk_27,
    host_thunk_28, host_thunk_29, host_thunk_30, host_thunk_31,
};

/* the blank's interrupt. clock_hook runs first of the kernel's clock
 * interrupt, on whatever stack it interrupts, with the interrupt's frame,
 * eip, cs and eflags, above the registers it saves */
void *clock_original;
void *blank_stack_top;
void *blank_entry;
static ULONG code_end;
static PKTHREAD game_thread;
static ULONGLONG blank_ticks;
static ULONGLONG next_blank;
static volatile int blank_due;
/* the blanks are the display's own (video_blank_locked): the game's frames
 * then keep step with the frames shown. else, and where the display gives
 * none for two periods, they are the clock's */
static int blank_from_display;

__asm__(".text\n"
        ".globl _clock_hook\n"
        "_clock_hook:\n"
        "    pushal\n"
        "    cld\n"
        "    leal 32(%esp), %eax\n"
        "    pushl %eax\n"
        "    call _clock_check\n"
        "    addl $4, %esp\n"
        "    popal\n"
        "    jmp *_clock_original\n"
        /* where a redirected blank resumes the game's thread: on the port's
         * interrupt stack, the host's chores, then the port's entry, every
         * register and the flags as the interrupt found them.
         *
         * the x87 and SSE state too: the game may be anywhere, with values
         * live in its XMM and x87 registers, and the chores' sound
         * emulation computes in them. the port's entry saves that state
         * only after the chores, so without the FXSAVE here they ran over
         * it, and the game resumed on their values: a view test or a
         * matrix gone wrong for a frame, the stage's objects and the land
         * dropped. the chores run as from a host call: an empty x87 stack,
         * the game's control word and MXCSR */
        ".globl _blank_trampoline\n"
        "_blank_trampoline:\n"
        "    movl _blank_stack_top, %esp\n"
        "    pushfl\n"
        "    pushal\n"
        "    cld\n"
        "    movl %esp, %ebx\n"
        "    subl $512, %esp\n"
        "    andl $-16, %esp\n"
        "    fxsave (%esp)\n"
        "    fninit\n"
        "    fldcw (%esp)\n"
        "    call _blank_chores_c\n"
        "    fxrstor (%esp)\n"
        "    movl %ebx, %esp\n"
        "    popal\n"
        "    popfl\n"
        "    jmp *_blank_entry\n"
        ".globl _blank_trampoline_end\n"
        "_blank_trampoline_end:\n");
void clock_hook(void);
void blank_trampoline(void);
void blank_trampoline_end(void);

/* a profile: the game thread's eip at each clock tick, which the chores
 * report every ten seconds by function */
#define SAMPLES 1024
static ULONG samples[SAMPLES];
static volatile unsigned sample_count;
/* the longest gap between two clock interrupts since the last report, and
 * the blank it ended at: some 1 ms where the clock ticks as it should. a
 * long one means nothing ran, the game's thread nor the blank's chores:
 * the machine itself stopped (an emulator's pause), or interrupts were off */
static ULONGLONG clock_last, clock_gap_most;
/* the pictures shown (clock_check): how many blanks each stayed on the
 * screen, counted by how many, and the last DISPLAY_RUNS in order */
#define DISPLAY_RUNS 120
static ULONG display_shown;
static unsigned display_blanks, display_held[10], display_run_count;
static unsigned char display_runs[DISPLAY_RUNS];
static unsigned clock_gap_blank;

void clock_check(ULONG *frame)
{
    ULONG eip = frame[0];

    if (interrupt == NULL) {
        return;
    }
    {
        ULONGLONG now = KeQueryPerformanceCounter();

        if (clock_last != 0 && now - clock_last > clock_gap_most) {
            clock_gap_most = now - clock_last;
            clock_gap_blank = blanks;
        }
        clock_last = now;
    }
    if (KeGetCurrentThread() == game_thread) {
        samples[sample_count++ % SAMPLES] = eip;
    }
    if (!blank_due) {
        ULONGLONG now = KeQueryPerformanceCounter();

        if (blank_from_display) {
            /* the blank: the NV2A's flag as it rises, or pbkit's count as
             * it moves where its DPC took the flag before this tick saw it,
             * but not both for one blank: a sign within half a blank of the
             * last is that one's. none for a blank and a half, and the
             * clock gives one. next_blank is the last blank's time and a
             * blank's length */
            static unsigned seen;
            static int was_flagged;
            unsigned count = video_blank_count();
            int flagged = video_blank_flagged();
            int rose = flagged && !was_flagged;

            was_flagged = flagged;
            if (!rose && count == seen && now < next_blank + blank_ticks / 2) {
                return;
            }
            if (!rose && count != seen && now + blank_ticks / 2 < next_blank) {
                seen = count;
                return;
            }
            seen = count;
            next_blank = now + blank_ticks;
        } else {
            if (now < next_blank) {
                return;
            }
            next_blank += blank_ticks;
            if (next_blank < now) {
                next_blank = now + blank_ticks;
            }
        }
        blanks++;
        blank_due = 1;
        {
            /* the picture on the screen: the display's start, which pbkit
             * moves at a blank to a frame finished. how many blanks each
             * stayed, for the report: an even 30 a second is a 2 each */
            ULONG shown = *(volatile ULONG *)(0xfd000000u + 0x00600800u);

            if (shown != display_shown) {
                if (display_shown != 0) {
                    unsigned held = display_blanks > 9 ? 9 : display_blanks;

                    display_held[held]++;
                    display_runs[display_run_count++ % DISPLAY_RUNS] = (unsigned char)held;
                }
                display_shown = shown;
                display_blanks = 0;
            }
            display_blanks++;
        }
    }
    if (host_busy) {
        blank_owed = 1;
        blank_due = 0;
        return;
    }
    /* the game's thread, in the game's code: else the next tick tries */
    if (KeGetCurrentThread() != game_thread || eip < CODE || eip >= code_end) {
        return;
    }
    blank_due = 0;
    if (!interrupt->may_interrupt(eip)) {
        /* the port runs it as its mask drops */
        return;
    }
    *interrupt->saved_eip = eip;
    *interrupt->saved_esp = (unsigned long)(frame + 3);
    frame[0] = (ULONG)blank_trampoline;
}

/* the host's own work at each blank: the pokes, the watched word, the
 * mode's log and the quit, as the PC host's blank does */
static const char *pokes;
static const char *watch;
static volatile int *watched_mode;
static int last_mode = -1;
static unsigned quit_at;
static unsigned chores_done;

static void check_pokes(void)
{
    char symbol[128];

    while (pokes != NULL && *pokes != '\0') {
        char *end;
        unsigned at = (unsigned)strtoul(pokes, &end, 10);
        const char *name;
        size_t length;
        unsigned offset = 0;
        unsigned long value;
        int size = 4;
        ULONG address;

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
                *(volatile UCHAR *)address = (UCHAR)value;
            } else if (size == 2) {
                *(volatile USHORT *)address = (USHORT)value;
            } else {
                *(volatile ULONG *)address = (ULONG)value;
            }
        }
        pokes = *end == ',' ? end + 1 : end;
    }
}

/* HOST_WATCH=SYMBOL: a 1ST_READ.BIN word, logged as it changes */
static volatile ULONG *watched;
static ULONG watched_value;
static int watched_first = 1;

/* the functions the samples fall in, the commonest first */
static void profile(void)
{
    static const char *names[SAMPLES];
    static int counts[SAMPLES];
    static ULONG copy[SAMPLES];
    int n = sample_count < SAMPLES ? (int)sample_count : SAMPLES;
    int kinds = 0;
    int k;
    int j;

    memcpy(copy, samples, sizeof copy);
    sample_count = 0;
    for (k = 0; k < n; k++) {
        ULONG past;
        const char *name = copy[k] >= CODE && copy[k] < code_end ? nearest(&main_symbols, copy[k], &past) : NULL;

        if (name == NULL) {
            name = copy[k] < CODE ? "(the host)" : copy[k] >= 0x80000000u ? "(the kernel)" : "(an overlay or the host)";
        }
        for (j = 0; j < kinds && names[j] != name; j++) {
        }
        if (j == kinds) {
            names[kinds] = name;
            counts[kinds++] = 0;
        }
        counts[j]++;
    }
    for (k = 0; k < 28 && kinds > 0; k++) {
        int best = 0;

        for (j = 1; j < kinds; j++) {
            if (counts[j] > counts[best]) {
                best = j;
            }
        }
        if (counts[best] == 0) {
            break;
        }
        logf_("host:   %3d of %d samples in %s\n", counts[best], n, names[best]);
        counts[best] = 0;
    }
    /* the host's own, by 256 bytes of its code, which have no names here:
     * symbolize.py names them from main.map */
    kinds = 0;
    for (k = 0; k < n; k++) {
        ULONG at = copy[k] & ~0xffu;

        if (copy[k] >= CODE) {
            continue;
        }
        for (j = 0; j < kinds && copy[j] != at; j++) {
        }
        if (j == kinds) {
            copy[kinds] = at; /* below k: its sample is already counted */
            counts[kinds++] = 0;
        }
        counts[j]++;
    }
    for (k = 0; k < 20 && kinds > 0; k++) {
        int best = 0;

        for (j = 1; j < kinds; j++) {
            if (counts[j] > counts[best]) {
                best = j;
            }
        }
        if (counts[best] == 0) {
            break;
        }
        logf_("host:   %3d of %d samples in host 0x%08lx\n", counts[best], n, copy[best]);
        counts[best] = 0;
    }
}

/* the report. Back asks for it (report_wanted); the chores mark its start
 * in the log and set the port's port_report, the port logs its part at its
 * next render and clears it, the chores add the host's part, and the log's
 * thread writes the report alone to E:\SONICADV\reportNN.txt, the first
 * number not on the disk, and the whole log to log.txt */
static volatile int *port_report_flag;
static const struct port_where *port_where_at;
static const struct port_frame *port_frames_at;
static const volatile unsigned *port_frame_count_at;
static volatile int report_stage; /* 0 none, 1 the port's part asked for, 2 to be written */
static unsigned report_asked, report_to, report_number;
static volatile unsigned free_kb_now, free_kb_lowest = ~0u;
/* the kernel's file cache, pool and stacks at the log thread's last look */
static volatile unsigned cache_kb_now, kpool_kb_now, stack_kb_now;

static void log_where(void)
{
    const struct port_where *w = port_where_at;

    if (w == NULL) {
        return;
    }
    /* nxdk's printf prints no floats: the places in whole units */
    logf_("host: as of the last frame: game_mode %d, stage %d act %d, character %d\n", w->game_mode, w->stage, w->act,
          w->character);
    if (w->player) {
        logf_("host:   player 0 at %ld %ld %ld (bits %08lx %08lx %08lx), angles %x %x %x\n", (long)w->player_pos[0],
              (long)w->player_pos[1], (long)w->player_pos[2], *(const ULONG *)&w->player_pos[0],
              *(const ULONG *)&w->player_pos[1], *(const ULONG *)&w->player_pos[2], w->player_ang[0] & 0xffff,
              w->player_ang[1] & 0xffff, w->player_ang[2] & 0xffff);
    }
    if (w->camera) {
        logf_("host:   the camera at %ld %ld %ld, angles %x %x %x\n", (long)w->camera_pos[0], (long)w->camera_pos[1],
              (long)w->camera_pos[2], w->camera_ang[0] & 0xffff, w->camera_ang[1] & 0xffff, w->camera_ang[2] & 0xffff);
    }
}

/* the port's last frames, from its own record, for a fault or a stall,
 * where the port can't log them itself */
static void log_last_frames(int count)
{
    unsigned have, k;
    int j;

    if (port_frames_at == NULL || port_frame_count_at == NULL) {
        return;
    }
    have = *port_frame_count_at;
    logf_("host: the port's last frames, in us: frame busy logic drawer render lists draws sort gpuwait blankwait "
          "sound idle; strips vertices models gpu-vertices\n");
    for (k = have > (unsigned)count ? have - (unsigned)count : 0; k < have; k++) {
        const struct port_frame *f = &port_frames_at[k % PORT_FRAMES];
        char line[200];
        int at = snprintf(line, sizeof line, "host:   %u:", f->number);

        for (j = 0; j < PHASES; j++) {
            at += snprintf(line + at, sizeof line - (size_t)at, " %u", f->us[j]);
        }
        logf_("%s; %u %u %u %u; steps %u of %u\n", line, f->strips, f->vertices, f->models, f->gpu_vertices,
              f->steps, f->steps_meant);
    }
}

static void report_host_part(void)
{
    if (write_failures != 0) {
        logf_("report: the disk: %u writes failed so far, the last at %u of %u bytes, the kernel's error %lu\n",
              write_failures, write_failed_done, write_failed_length, (unsigned long)write_failed_error);
    }
    logf_("report: memory: %u KB free now, the least %u KB (the log's thread looks 4 times a second); the kernel's file "
          "cache %u KB, its pool %u KB, stacks %u KB\n",
          free_kb_now, free_kb_lowest == ~0u ? 0 : free_kb_lowest, cache_kb_now, kpool_kb_now, stack_kb_now);
    if (have_video) {
        video_report_memory();
    }
    logf_("report: video memory backed: %u KB of %u\n", vram_committed >> 10, (unsigned)(PORT_VRAM_SIZE >> 10));
    if (have_sound) {
        aica_apu_report_memory();
        logf_("report: sound: the most one blank's chores took since the last report %lu us\n",
              (ULONG)(sound_most_ticks * 1000000 / frequency));
    }
    sound_most_ticks = 0;
    logf_("report: the log: %u of %u KB\n", log_length >> 10, LOG_SIZE >> 10);
    logf_("report: the clock's longest gap between interrupts since the last report: %lu us, at blank %u (some 1000 "
          "is a clock that ran)\n",
          (ULONG)(clock_gap_most * 1000000 / frequency), clock_gap_blank);
    clock_gap_most = 0;
    {
        char runs[DISPLAY_RUNS + 1];
        unsigned k, n = display_run_count < DISPLAY_RUNS ? display_run_count : DISPLAY_RUNS;

        for (k = 0; k < n; k++) {
            runs[k] = (char)('0' + display_runs[(display_run_count - n + k) % DISPLAY_RUNS]);
        }
        runs[n] = '\0';
        logf_("report: pictures on the screen since the last report, by the blanks each stayed: 1 %u, 2 %u, 3 %u, "
              "4 %u, more %u (an even 30 a second is all 2s)\n",
              display_held[1], display_held[2], display_held[3], display_held[4],
              display_held[5] + display_held[6] + display_held[7] + display_held[8] + display_held[9]);
        logf_("report: the last %u, oldest first: %s\n", n, runs);
        memset(display_held, 0, sizeof display_held);
    }
    {
        /* the timestamp counter's rate, by the performance counter's since
         * the start */
        static const char *const names[32] = {
            "log", "exit", "file_open", "file_size", "file_read", "file_close", "dir_exists", "time_us",
            "wait_vblank", "pad_read", "set_interrupt", "get_date", "card_exists", "card_read", "card_write",
            "video_begin", "video_texture", "video_draw", "video_present", "video_fog", "aica_read", "aica_write",
            "video_textures_lost", "movie_memory", "video_vertices", "video_texture_packed", "video_texture_known",
            "vram_commit", "video_store", "video_draw_items", "stats", "video_arena",
        };
        static unsigned long long tsc0, qpc0;
        unsigned long long tsc = __builtin_ia32_rdtsc(), qpc = KeQueryPerformanceCounter();
        unsigned long long per_us;

        if (tsc0 == 0) {
            tsc0 = host_start_tsc;
            qpc0 = started;
        }
        per_us = qpc > qpc0 ? (tsc - tsc0) * frequency / (qpc - qpc0) / 1000000 : 733;
        logf_("report: the longest host call since the last report: %s, %lu us, at blank %u (it holds the blanks "
              "back while it runs)\n",
              names[call_most_kind], (ULONG)(call_most / (per_us ? per_us : 733)), call_most_blank);
        call_most = 0;
    }
    logf_("report: where the game's thread was, the last %u clock ticks:\n",
          sample_count < SAMPLES ? sample_count : SAMPLES);
    profile();
}

static void report_step(void)
{
    /* HOST_REPORT_AT=FRAME: a report there, as the pad's Back makes one,
     * for a scripted run */
    {
        static int checked;
        static unsigned at;

        if (!checked) {
            checked = 1;
            at = setting("HOST_REPORT_AT") != NULL ? (unsigned)strtoul(setting("HOST_REPORT_AT"), NULL, 10) : 0;
        }
        if (at != 0 && script_now() >= at) {
            at = 0;
            report_wanted = 1;
        }
    }
    if (report_stage == 0 && report_wanted) {
        report_wanted = 0;
        report_from = log_length;
        /* the run's own count: the file's number, the first not on the
         * disk, is the log's thread's to find, and its line says it */
        static unsigned reports_made;

        logf_("HOST REPORT %u of this run, at blank %u, frame %u\n", ++reports_made, blanks,
              frames_rendered != NULL ? (unsigned)*frames_rendered : 0);
        report_asked = blanks;
        report_stage = 1;
        if (port_report_flag != NULL) {
            *port_report_flag = 1;
        }
        return;
    }
    if (report_stage == 1 && (port_report_flag == NULL || *port_report_flag == 0 || blanks - report_asked > 120)) {
        if (port_report_flag == NULL || *port_report_flag != 0) {
            logf_("report: the game rendered no frame in two seconds: the port's part is missing\n");
            if (port_report_flag != NULL) {
                *port_report_flag = 0;
            }
            log_where();
            log_last_frames(30);
        }
        report_host_part();
        logf_("HOST REPORT END\n");
        report_to = log_length;
        report_stage = 2;
    }
}

/* the log's thread, 4 times a second: the free memory, a report to write,
 * and the stall: under HOST_CRASH_LOG, a game that renders no frame for 10
 * seconds after its first is logged once, where it is and where its thread
 * has been, and the log written. a thread that hangs at DISPATCH_LEVEL keeps
 * this one from running, and goes unseen */
static void log_thread_tick(void)
{
    static unsigned last_frame, still_ticks;
    static int stalled;
    MM_STATISTICS stats;

    memset(&stats, 0, sizeof stats);
    stats.Length = sizeof stats;
    MmQueryStatistics(&stats);
    free_kb_now = (unsigned)stats.AvailablePages * 4;
    cache_kb_now = (unsigned)stats.CachePagesCommitted * 4;
    kpool_kb_now = (unsigned)stats.PoolPagesCommitted * 4;
    stack_kb_now = (unsigned)stats.StackPagesCommitted * 4;
    if (free_kb_now < free_kb_lowest) {
        free_kb_lowest = free_kb_now;
        /* each new low under 2 MB, where the game was: because free memory
         * can fall near nothing unseen, its lows are logged */
        if (free_kb_now < 2048) {
            const struct port_where *w = port_where_at;
            unsigned used = 0, size = 0;

            if (have_video) {
                video_pool_kb(&used, &size);
            }
            logf_("host: memory: a new low, %u KB free, at blank %u, frame %u; game_mode %d, stage %d act %d; "
                  "textures' pool %u of %u KB; the kernel's file cache %lu KB, pool %lu KB, stacks %lu KB, virtual "
                  "committed %lu KB\n",
                  free_kb_now, blanks, frame_clock != NULL ? (unsigned)*frame_clock : 0, w != NULL ? w->game_mode : -1,
                  w != NULL ? w->stage : -1, w != NULL ? w->act : -1, used, size,
                  (unsigned long)stats.CachePagesCommitted * 4, (unsigned long)stats.PoolPagesCommitted * 4, (unsigned long)stats.StackPagesCommitted * 4,
                  (unsigned long)stats.VirtualMemoryBytesCommitted >> 10);
        }
    }
    if (report_stage == 2) {
        char path[64];
        unsigned from = report_from, to = report_to;

        if (report_number == 0) {
            for (report_number = 1; report_number < 99; report_number++) {
                snprintf(path, sizeof path, "E:\\SONICADV\\report%02u.txt", report_number);
                if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) {
                    break;
                }
            }
        } else if (report_number < 99) {
            report_number++;
        }
        snprintf(path, sizeof path, "E:\\SONICADV\\report%02u.txt", report_number);
        if (to > from && to <= log_length) {
            int failed = write_file(path, log_buffer + from, to - from);
            KIRQL was = KeRaiseIrqlToDpcLevel();

            logf_("host: report %u %s %s\n", report_number, failed ? "couldn't be written to" : "written to", path);
            KfLowerIrql(was);
        }
        log_write();
        report_stage = 0;
    }
    if (crash_log && frames_rendered != NULL) {
        unsigned frame = (unsigned)*frames_rendered;

        if (frame != last_frame || frame == 0) {
            if (stalled) {
                KIRQL was = KeRaiseIrqlToDpcLevel();

                logf_("host: frames again after the stall, at blank %u, frame %u\n", blanks, frame);
                KfLowerIrql(was);
            }
            last_frame = frame;
            still_ticks = 0;
            stalled = 0;
        } else if (!stalled && ++still_ticks >= 40) {
            KIRQL was = KeRaiseIrqlToDpcLevel();

            stalled = 1;
            logf_("host: STALL: no frame rendered for 10 seconds, at blank %u, frame %u\n", blanks, frame);
            log_where();
            log_last_frames(16);
            logf_("host: where the game's thread was, the last %u clock ticks:\n",
                  sample_count < SAMPLES ? sample_count : SAMPLES);
            profile();
            KfLowerIrql(was);
            log_write();
        }
    }
}

static void blank_chores(void)
{
    static unsigned sound_blanks;

    /* pbkit's DPC, which a blank queues, runs as the IRQL drops: it counts
     * the blank, frees the buffer the display has left and turns the NV2A's
     * interrupt on again, without which the next blank swaps no frame in.
     * the game's thread keeps DISPATCH_LEVEL for long stretches, and the
     * frames came to the screen a blank late here and there: so once a
     * blank, here, on the interrupt's or the host's stack */
    if (have_video && KeGetCurrentIrql() > PASSIVE_LEVEL) {
        KIRQL was = KeGetCurrentIrql();

        KfLowerIrql(PASSIVE_LEVEL);
        KfRaiseIrql(was);
    }

    /* the sound: the ARM7 and the channels' model for the blanks since the
     * last chores, the APU's voices after them */
    if (have_video) {
        video_idle_present();
    }
    if (have_sound && blanks != sound_blanks) {
        ULONGLONG before = KeQueryPerformanceCounter();

        xbox_sound_blanks(blanks - sound_blanks);
        before = KeQueryPerformanceCounter() - before;
        sound_total_ticks += before;
        if (before > sound_most_ticks) {
            sound_most_ticks = before;
        }
        sound_blanks = blanks;
    }
    chores_done = blanks;
    check_pokes();
    if (watched != NULL && (watched_first || *watched != watched_value)) {
        watched_first = 0;
        watched_value = *watched;
        logf_("host: %s = %ld at blank %u, frame %u\n", watch, (long)watched_value, blanks,
              frame_clock != NULL ? (unsigned)*frame_clock : 0);
    }
    if (watched_mode != NULL) {
        /* the bloom in a stage or a field alone: main.h's GAME_MODE_ACTION,
         * GAME_MODE_FIELD and GAME_MODE_TRIAL */
        int mode = *watched_mode;

        video_bloom_on = mode == 4 || mode == 5 || mode == 9;
    }
    if (watched_mode != NULL && *watched_mode != last_mode) {
        last_mode = *watched_mode;
        logf_("host: game_mode %d at blank %u, frame %u; %u KB free (the log's thread's last look)\n", last_mode,
              blanks, frame_clock != NULL ? (unsigned)*frame_clock : 0, free_kb_now);
    }
    report_step();
    /* HOST_PROFILE=1: every 600 blanks, where the time went */
    if (blanks % 600 == 0 && profiling) {
        {
            /* the frames rendered, and their rate over the 600 blanks */
            static unsigned last_frames;
            unsigned frames = frames_rendered != NULL ? (unsigned)*frames_rendered : 0;

            logf_("host: blank %u, frame %u, %u.%u frames a second\n", blanks, frames, (frames - last_frames) / 10,
                  (frames - last_frames) % 10);
            last_frames = frames;
        }
        profile();
        {
            /* the host calls a second, the busiest kinds, and the IRQL's
             * lowering and raising, in ms a second, by the timestamp
             * counter's rate over these blanks */
            static unsigned long long last_tsc, last_qpc;
            unsigned long long tsc = __builtin_ia32_rdtsc(), qpc = KeQueryPerformanceCounter();
            unsigned total = 0, k, best[3] = {0, 0, 0};

            for (k = 0; k < 32; k++) {
                total += host_calls[k];
                if (host_calls[k] > host_calls[best[2]]) {
                    best[2] = k;
                    if (host_calls[best[2]] > host_calls[best[1]]) {
                        unsigned t = best[1];
                        best[1] = best[2];
                        best[2] = t;
                    }
                    if (host_calls[best[1]] > host_calls[best[0]]) {
                        unsigned t = best[0];
                        best[0] = best[1];
                        best[1] = t;
                    }
                }
            }
            if (last_qpc != 0 && qpc > last_qpc) {
                unsigned long long tsc_hz = (tsc - last_tsc) * frequency / (qpc - last_qpc);

                logf_("host: calls: %u a second, most of kinds %u, %u and %u (%u, %u, %u); the IRQL %lu ms a "
                      "second; the CPU at %lu MHz\n",
                      total / 10, best[0], best[1], best[2], host_calls[best[0]] / 10, host_calls[best[1]] / 10,
                      host_calls[best[2]] / 10, (ULONG)(host_irql_tsc * 100 / (tsc_hz ? tsc_hz : 1)),
                      (ULONG)(tsc_hz / 1000000));
            }
            last_tsc = tsc;
            last_qpc = qpc;
            memset(host_calls, 0, sizeof host_calls);
            host_irql_tsc = 0;
        }
        if (have_video) {
            video_report();
            logf_("host: video memory: %u KB backed of %u\n", vram_committed >> 10, (unsigned)(PORT_VRAM_SIZE >> 10));
        }
        if (have_sound) {
            /* the 600 blanks' sound, in ms a second */
            logf_("host: sound: arm7 and channels %lu.%lu ms a second, voices %lu.%lu\n",
                  (ULONG)(xbox_sound_ticks_arm * 100 / frequency), (ULONG)(xbox_sound_ticks_arm * 1000 / frequency % 10),
                  (ULONG)(xbox_sound_ticks_apu * 100 / frequency), (ULONG)(xbox_sound_ticks_apu * 1000 / frequency % 10));
            xbox_sound_ticks_arm = xbox_sound_ticks_apu = 0;
            xbox_sound_report();
            aica_apu_report();
        }
    }
    if (quit_at != 0 && script_now() >= quit_at) {
        char why[64];

        snprintf(why, sizeof why, "quit at blank %u, frame %u", blanks, script_now());
        halt(why);
    }
}

void blank_chores_c(void)
{
    blank_chores();
}

static void raw_set_interrupt(const struct host_interrupt *with)
{
    struct {
        USHORT limit;
        ULONG base;
    } __attribute__((packed)) idtr;
    USHORT *gate;
    ULONG handler;

    blank_stack_top = with->stack_top;
    blank_entry = (void *)with->entry;
    interrupt = with;
    __asm__ volatile("sidt %0" : "=m"(idtr));
    gate = (USHORT *)(idtr.base + 0x30 * 8);
    handler = gate[0] | (ULONG)gate[3] << 16;
    logf_("host: the clock interrupt's handler is 0x%08lx\n", handler);
    clock_original = (void *)handler;
    /* the console's 59.94 blanks a second: 16683333 ns */
    blank_ticks = frequency * 16683333ULL / 1000000000ULL;
    __asm__ volatile("cli");
    next_blank = KeQueryPerformanceCounter() + blank_ticks;
    gate[0] = (USHORT)(ULONG)clock_hook;
    gate[3] = (USHORT)((ULONG)clock_hook >> 16);
    __asm__ volatile("sti");
}

/* faults: the CPU's divide error, invalid opcode, stack fault, general
 * protection, page fault, x87 error, alignment check and SIMD error, logged
 * with where, the game's place and its last frames, then a halt, which
 * writes the log under HOST_CRASH_LOG */
#define FAULT(n, code) \
    __asm__(".text\n.globl _fault_" #n "\n_fault_" #n ":\n" code "    pushl $" #n "\n    jmp fault_common\n")
FAULT(0, "    pushl $0\n");
FAULT(6, "    pushl $0\n");
FAULT(12, "");
FAULT(13, "");
FAULT(14, "");
FAULT(16, "    pushl $0\n");
FAULT(17, "");
FAULT(19, "    pushl $0\n");
__asm__(".text\n"
        "fault_common:\n"
        "    pushal\n"
        "    cld\n"
        "    pushl %esp\n"
        "    call _fault_report\n");
void fault_0(void), fault_6(void), fault_12(void), fault_13(void), fault_14(void), fault_16(void), fault_17(void),
    fault_19(void);

void fault_report(ULONG *frame)
{
    /* pushal's edi, esi, ebp, esp, ebx, edx, ecx, eax, then the vector, the
     * error code, eip, cs and eflags */
    ULONG cr2;
    ULONG *ebp = (ULONG *)frame[2];
    int depth;

    __asm__ volatile("movl %%cr2, %0" : "=r"(cr2));
    logf_("host: fault %lu, error 0x%lx, address 0x%08lx\n", frame[8], frame[9], cr2);
    report("eip", frame[10]);
    logf_("host: eax %08lx ebx %08lx ecx %08lx edx %08lx esi %08lx edi %08lx ebp %08lx esp %08lx\n", frame[7],
          frame[4], frame[6], frame[5], frame[1], frame[0], frame[2], (ULONG)(frame + 13));
    {
        ULONG *sp = frame + 13;
        int k;

        /* 64 words, whose code addresses symbolize.py's caller can name
         * against each image: a leaf function keeps no frame to walk */
        for (k = 0; k < 64; k += 4) {
            logf_("host:   [esp+%02x] %08lx %08lx %08lx %08lx\n", k * 4, sp[k], sp[k + 1], sp[k + 2], sp[k + 3]);
        }
    }
    {
        /* Ninja's current texture list, for a fault on a texture that never
         * loaded (njSetkmTextureNum on a real Xbox, entering Emerald Coast):
         * the list, its count, and its first entries' name and address */
        ULONG list = symbol_address(&main_symbols, "_nj_current_texlist");

        if (list != 0 && *(ULONG *)list >= RAM && *(ULONG *)list < RAM + AREA_SIZE) {
            const ULONG *texlist = (const ULONG *)*(ULONG *)list;
            ULONG count = texlist[1];
            ULONG k;

            logf_("host: _nj_current_texlist 0x%08lx: textures 0x%08lx, %lu\n", (ULONG)texlist, texlist[0], count);
            for (k = 0; k < count && k < 8 && texlist[0] >= RAM && texlist[0] < RAM + AREA_SIZE; k++) {
                const ULONG *entry = (const ULONG *)(texlist[0] + k * 12);

                logf_("host:   texture %lu: filename 0x%08lx attr 0x%08lx texaddr 0x%08lx\n", k, entry[0], entry[1],
                      entry[2]);
            }
        }
    }
    {
        /* the faulting function's first four arguments, where it keeps a
         * frame, and what each that points into RAM points at: by RAM's own
         * address and by the console's cached view, which are one memory
         * unless a page has moved under the views (allocate), with each
         * page's entry */
        ULONG *args = (ULONG *)frame[2];
        int k;

        if ((ULONG)args > (ULONG)(frame + 13) && (ULONG)args < (ULONG)(frame + 13) + 0x100000) {
            logf_("host: arguments %08lx %08lx %08lx %08lx\n", args[2], args[3], args[4], args[5]);
            for (k = 2; k < 6; k++) {
                ULONG at = args[k] & ~3ul;

                if (at >= RAM && at + 32 <= PORT_VRAM) {
                    const ULONG *own = (const ULONG *)at;
                    const ULONG *view = (const ULONG *)(at - RAM + 0x8c000000u);

                    logf_("host:   at 0x%08lx: %08lx %08lx %08lx %08lx %08lx %08lx, entry %08lx\n", at, own[0], own[1],
                          own[2], own[3], own[4], own[5], PTE(at));
                    logf_("host:   its view:      %08lx %08lx %08lx %08lx %08lx %08lx, entry %08lx\n", view[0], view[1],
                          view[2], view[3], view[4], view[5], PTE(view));
                }
            }
        }
    }
    {
        /* RAM's pages whose entries are no longer what the views copied */
        ULONG at;
        unsigned moved = 0;

        for (at = RAM; at < PORT_VRAM; at += PAGE) {
            if ((PTE(at) ^ PTE(at - RAM + 0x8c000000u)) & ~0xffful) {
                if (moved < 8) {
                    logf_("host: the page at 0x%08lx has entry %08lx, its view %08lx\n", at, PTE(at),
                          PTE(at - RAM + 0x8c000000u));
                }
                moved++;
            }
        }
        logf_("host: %u of RAM's pages differ from their views\n", moved);
    }
    /* the frames, while ebp stays above the stack pointer on its stack,
     * which on a real Xbox lies at 0xd0000000 and up */
    for (depth = 0; depth < 12 && (ULONG)ebp > (ULONG)(frame + 13) && (ULONG)ebp < (ULONG)(frame + 13) + 0x100000;
         depth++) {
        report("from", ebp[1]);
        ebp = (ULONG *)ebp[0];
    }
    {
        /* the 16 words below the stack pointer: a return address popped just
         * before a jump to nothing is still there */
        ULONG *sp = frame + 13;
        int k;

        for (k = -16; k < 0; k += 4) {
            logf_("host:   [esp-%02x] %08lx %08lx %08lx %08lx\n", -k * 4, sp[k], sp[k + 1], sp[k + 2], sp[k + 3]);
        }
    }
    {
        /* the task whose step was running: the GCC port's present.c keeps it
         * around each step (present_run), and task.c the root's step. a
         * task's step, draw and destructor are its fifth to seventh words
         * (src/system/task.h's struct task), its parent the third */
        ULONG at = symbol_address(&main_symbols, "present_task");
        ULONG root = symbol_address(&main_symbols, "task_current_root_step");

        if (root != 0 && *(ULONG *)root != 0) {
            report("the root task's step", *(ULONG *)root);
        }
        if (at != 0 && *(ULONG *)at >= RAM && *(ULONG *)at < RAM + AREA_SIZE) {
            const ULONG *task = (const ULONG *)*(ULONG *)at;

            logf_("host: the task stepping 0x%08lx, its work 0x%08lx\n", (ULONG)task, task[8]);
            report("  its step", task[4]);
            report("  its draw", task[5]);
            report("  its destructor", task[6]);
            if (task[2] >= RAM && task[2] < RAM + AREA_SIZE) {
                report("  its parent's step", ((const ULONG *)task[2])[4]);
            }
        }
    }
    log_where();
    log_last_frames(16);
    halt("stopped at the fault");
}

static void set_gate(int vector, void (*handler)(void))
{
    struct {
        USHORT limit;
        ULONG base;
    } __attribute__((packed)) idtr;
    USHORT *gate;

    __asm__ volatile("sidt %0" : "=m"(idtr));
    gate = (USHORT *)(idtr.base + (ULONG)vector * 8);
    __asm__ volatile("cli");
    gate[0] = (USHORT)(ULONG)handler;
    gate[3] = (USHORT)((ULONG)handler >> 16);
    __asm__ volatile("sti");
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

static void (*game_main)(void);

static DWORD __stdcall run_game(LPVOID unused)
{
    (void)unused;
    game_thread = KeGetCurrentThread();
    profiling = setting("HOST_PROFILE") != NULL && strtoul(setting("HOST_PROFILE"), NULL, 10) != 0;
    /* the movies are off for now, as each ends as it starts (movie.c's
     * port_movie_skip): HOST_MOVIES=1 plays them */
    {
        /* the port's counts with the profile, its log lines every 60
         * frames too (port.h's port_profile, bits 0 to 2), and the times
         * alone for the reports */
        ULONG counts = symbol_address(&main_symbols, "port_profile");

        if (counts != 0 && (profiling || report_on || crash_log)) {
            *(volatile int *)counts = profiling ? 7 : 1;
        }
        logf_("host: reports on Back %s, the log written at a fault or a stall %s, every 10 seconds %s\n",
              report_on ? "on" : "off", crash_log ? "on" : "off", log_file ? "on" : "off");
    }
    /* the port's figures on the screen (port.h's port_overlay) */
    /* on unless HOST_OVERLAY=0, so that a host.txt without the line, or
     * none, still shows them */
    {
        ULONG overlay = symbol_address(&main_symbols, "port_overlay");
        int on = setting("HOST_OVERLAY") == NULL || strtoul(setting("HOST_OVERLAY"), NULL, 10) != 0;

        if (overlay != 0) {
            *(volatile int *)overlay = on;
        }
        logf_("host: the figures on the screen are %s%s\n", on ? "on" : "off",
              overlay == 0 ? ", but the game's images have no port_overlay: they are older than this host" : "");
    }
    /* the settings as read, for a host.txt that isn't the one meant */
    {
        const char *line = settings;

        if (*line == '\0') {
            logf_("host: D:\\host.txt is empty or missing\n");
        }
        while (*line != '\0') {
            logf_("host: setting %s\n", line);
            line = strchr(line, '\0') + 1;
        }
    }
    /* HOST_60FPS=1: the experimental 60 frames a second (src/system/port.h's
     * port_toggles[TOGGLE_FAST], which L, R and the d-pad's down flip in
     * play): a stage's frame split into two pictures, in play only and where
     * src/system/pace.c allows it. port_chosen[TOGGLE_FAST] marks it chosen,
     * as the chord does: else a save's load (src/save/save.c's) puts back the
     * save's own state, and the split went off with the first save loaded */
    if (setting("HOST_60FPS") != NULL && strtoul(setting("HOST_60FPS"), NULL, 10) != 0) {
        ULONG toggles = symbol_address(&main_symbols, "port_toggles");
        ULONG chosen = symbol_address(&main_symbols, "port_chosen");

        if (toggles != 0 && chosen != 0) {
            ((volatile int *)toggles)[2] = 1;
            ((volatile int *)chosen)[2] = 1;
        }
        logf_("host: 60 frames a second %s\n",
              toggles != 0 && chosen != 0 ? "on" : "asked for, but the images have no port_toggles or port_chosen");
    }
    if (setting("HOST_MOVIES") == NULL || packs[PACK_MOVIES].count == 0) {
        ULONG skip = symbol_address(&main_symbols, "port_movie_skip");

        if (skip != 0) {
            *(volatile int *)skip = 1;
        }
    }
    logf_("host: running main at 0x%08lx, MXCSR 0x%04x\n", (ULONG)game_main, fpu_as_console());
    KeRaiseIrqlToDpcLevel();
    game_main();
    halt("the game's main returned");
    return 0;
}

int main(void)
{
    char *line;
    char *areas;

    frequency = KeQueryPerformanceFrequency();
    started = KeQueryPerformanceCounter();
    host_start_tsc = __builtin_ia32_rdtsc();
    logf_("host: the Xbox host, performance counter at %lu Hz\n", (ULONG)frequency);
    logf_("host: its log at 0x%08lx, its stack 0x%08lx to 0x%08lx, its call table at 0x%08lx\n", (ULONG)log_buffer,
          (ULONG)host_stack_low, (ULONG)host_stack_top, (ULONG)host_targets);
    set_gate(0, fault_0);
    set_gate(6, fault_6);
    set_gate(12, fault_12);
    set_gate(13, fault_13);
    set_gate(14, fault_14);
    set_gate(16, fault_16);
    set_gate(17, fault_17);
    set_gate(19, fault_19);
    nxMountDrive('E', "\\Device\\Harddisk0\\Partition1\\");
    read_settings();
    log_file = setting("HOST_LOG_FILE") != NULL && strtoul(setting("HOST_LOG_FILE"), NULL, 10) != 0;
    report_on = setting("HOST_REPORT") == NULL || strtoul(setting("HOST_REPORT"), NULL, 10) != 0;
    crash_log = setting("HOST_CRASH_LOG") == NULL || strtoul(setting("HOST_CRASH_LOG"), NULL, 10) != 0;
    {
        /* the log's thread: the 10 seconds' writes, the reports, the
         * stalls and the free memory */
        HANDLE writer = CreateThread(NULL, 0x4000, log_thread, NULL, 0, NULL);

        if (writer != NULL) {
            SetThreadPriority(writer, THREAD_PRIORITY_LOWEST);
        }
    }
    packs_open();
    areas = pack_text("code_areas.txt");
    if (areas == NULL) {
        halt("no code_areas.txt in D:\\modules.pak");
    }
    for (line = areas; *line != '\0';) {
        sscanf(line, "end 0x%lx", &code_end);
        line += strcspn(line, "\n");
        line += *line == '\n';
    }
    free(areas);
    if (code_end <= CODE || map_memory(code_end) != 0) {
        halt("can't map the port's memory");
    }
    {
        MM_STATISTICS stats;

        memset(&stats, 0, sizeof stats);
        stats.Length = sizeof stats;
        MmQueryStatistics(&stats);
        logf_("host: memory mapped: area 3 %lu MB, code %lu KB; %lu MB free\n", AREA_SIZE >> 20,
              (code_end - CODE) >> 10, stats.AvailablePages >> 8);
    }
    if (load_image("1st_read", 0, &main_symbols) != 0) {
        halt("can't load 1st_read");
    }
    read_images();
    pads_open();
    /* the AICA, on the APU. the sound opens first: video takes what memory
     * is left for its textures */
    have_sound = xbox_sound_open(sound_ram_kernel) == 0;
    have_video = setting("HOST_HEADLESS") == NULL && video_open() == 0;
    if (have_video && video_hd()) {
        /* 720p's picture is 16:9: the port's own (port_toggles[TOGGLE_WIDE],
         * which the overlay's switch still flips), drawn into the game's 640
         * by 480 as for a TV that stretches it, which the host's scale to
         * 1280 by 720 does */
        ULONG toggles = symbol_address(&main_symbols, "port_toggles");

        if (toggles != 0) {
            ((volatile int *)toggles)[1] = 1;
        }
        logf_("host: 720p: 16:9 %s\n", toggles != 0 ? "on" : "asked for, but the images have no port_toggles");
    }
    blank_from_display = have_video && video_blank_locked();
    logf_("host: the blanks are %s\n", blank_from_display ? "the display's" : "the clock's");
    pad_script = setting("HOST_PADS");
    pokes = setting("HOST_POKE");
    watch = setting("HOST_WATCH");
    if (setting("HOST_QUIT") != NULL) {
        quit_at = (unsigned)strtoul(setting("HOST_QUIT"), NULL, 10);
    }
    {
        /* the port's table: api's version, then a thunk for each call */
        static struct host_api thunked;
        void **from = (void **)&api;
        void **to = (void **)&thunked;
        int calls = (int)(sizeof api / sizeof(void *)) - 1;
        int k;

        if (calls > THUNKS) {
            halt("host_api has more calls than thunks");
        }
        to[0] = from[0];
        for (k = 0; k < calls; k++) {
            host_targets[k] = from[k + 1];
            to[k + 1] = (void *)thunks[k];
        }
        *(struct host_api **)HOST_API_SLOT = &thunked;
    }
    watched_mode = (volatile int *)symbol_address(&main_symbols, "game_mode");
    if (watch != NULL) {
        watched = (volatile ULONG *)symbol_address(&main_symbols, watch);
    }
    frames_rendered = (volatile ULONG *)symbol_address(&main_symbols, "_nj_vsync_count_");
    /* the report's, in the port (xbox/port/report.c) */
    port_report_flag = (volatile int *)symbol_address(&main_symbols, "port_report");
    port_where_at = (const struct port_where *)symbol_address(&main_symbols, "port_where");
    port_frames_at = (const struct port_frame *)symbol_address(&main_symbols, "port_frames");
    port_frame_count_at = (const volatile unsigned *)symbol_address(&main_symbols, "port_frame_count");
    if (setting("HOST_CLOCK") != NULL && strcmp(setting("HOST_CLOCK"), "frames") == 0) {
        frame_clock = frames_rendered;
    }
    game_main = (void (*)(void))symbol_address(&main_symbols, "main");
    if (game_main == NULL) {
        halt("1st_read has no main");
    }
    if (CreateThread(NULL, 0x100000, run_game, NULL, 0, NULL) == NULL) {
        halt("can't start the game's thread");
    }
    for (;;) {
        Sleep(1000);
    }
    return 0;
}
