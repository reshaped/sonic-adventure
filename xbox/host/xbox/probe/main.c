/* the memory map's probe, for stage 5: which of the console's addresses the
 * port can have on the Xbox, where the PC host maps them (xbox/host/pc/host.c).
 *
 * it reports over the serial port, COM1 on the SuperIO chip at 0x3f8, which
 * xemu writes to a file under -serial file:PATH, on the screen, and into
 * the_report, after the line "PROBE REPORT", which a dump of the guest's
 * memory (QMP's pmemsave) finds:
 *
 * - the physical memory, all and free;
 * - each address's page directory entry, and its page table entry where the
 *   entry points at a table: the kernel's page tables map themselves at
 *   0xc0000000, the directory at 0xc0300000;
 * - whether NtAllocateVirtualMemory gives the console's RAM at 0x0c000000 and
 *   the code areas at 0x10000000;
 * - the layout the Xbox host would take: 16 MB of contiguous memory, the
 *   console's RAM, mapped by 4 MB pages written into the directory by hand
 *   at each of the console's views, 0x0c000000, 0x8c000000, 0xac000000 and
 *   the store queues' 0xe0000000, each checked by a word written through
 *   the block and read through the view, at each 4 MB page's two ends. a
 *   view whose directory entry the kernel uses is left alone */
#include <hal/debug.h>
#include <hal/video.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <xboxkrnl/xboxkrnl.h>

static void outb(unsigned short port, unsigned char value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* the SuperIO chip's first serial port, at 0x3f8 */
static void serial_init(void)
{
    outb(0x2e, 0x55); /* configuration mode */
    outb(0x2e, 0x07);
    outb(0x2f, 0x04); /* logical device 4, serial port 1 */
    outb(0x2e, 0x60);
    outb(0x2f, 0x03);
    outb(0x2e, 0x61);
    outb(0x2f, 0xf8);
    outb(0x2e, 0x30);
    outb(0x2f, 0x01); /* active */
    outb(0x2e, 0xaa);
}

static char the_report[16384] = "PROBE REPORT\n";
static unsigned report_length = sizeof "PROBE REPORT\n" - 1;

static void report(const char *format, ...)
{
    char line[256];
    va_list args;
    const char *c;

    va_start(args, format);
    vsnprintf(line, sizeof line, format, args);
    va_end(args);
    for (c = line; *c != '\0'; c++) {
        if (*c == '\n') {
            outb(0x3f8, '\r');
        }
        outb(0x3f8, (unsigned char)*c);
    }
    debugPrint("%s", line);
    for (c = line; *c != '\0' && report_length < sizeof the_report - 32; c++) {
        the_report[report_length++] = *c;
    }
    the_report[report_length] = '\0';
}

#define PDE(va) (*(volatile ULONG *)(0xc0300000u + ((ULONG)(va) >> 22) * 4))
#define PTE(va) (*(volatile ULONG *)(0xc0000000u + ((ULONG)(va) >> 12) * 4))

static void show_entry(ULONG va)
{
    ULONG pde = PDE(va);

    if (!(pde & 1)) {
        report("  0x%08lx: directory entry empty (%08lx)\n", va, pde);
    } else if (pde & 0x80) {
        report("  0x%08lx: 4 MB page, physical 0x%08lx, flags %03lx\n", va, pde & 0xffc00000u, pde & 0xfff);
    } else {
        ULONG pte = PTE(va);

        report("  0x%08lx: table at 0x%08lx; page %s, physical 0x%08lx, flags %03lx\n", va, pde & 0xfffff000u,
               (pte & 1) ? "present" : "absent", pte & 0xfffff000u, pte & 0xfff);
    }
}

static void try_allocate(ULONG base, ULONG size)
{
    PVOID at = (PVOID)base;
    SIZE_T length = size;
    NTSTATUS status = NtAllocateVirtualMemory(&at, 0, &length, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);

    report("  NtAllocateVirtualMemory 0x%08lx, %lu MB: status 0x%08lx, got 0x%08lx, %lu bytes\n", base,
           size >> 20, (ULONG)status, (ULONG)at, (ULONG)length);
}

/* map size bytes at view, by 4 MB pages, to the contiguous block at block,
 * physical address physical, and check each page through the view */
static void try_view(ULONG view, volatile ULONG *block, ULONG physical, ULONG size)
{
    ULONG at;
    int good = 1;

    for (at = 0; at < size; at += 0x400000) {
        if (PDE(view + at) & 1) {
            report("  0x%08lx: in use, left alone\n", view + at);
            return;
        }
    }
    for (at = 0; at < size; at += 0x400000) {
        PDE(view + at) = (physical + at) | 0x83; /* present, writable, 4 MB */
        __asm__ volatile("invlpg (%0)" : : "r"(view + at) : "memory");
    }
    for (at = 0; at < size; at += 0x400000) {
        block[at / 4] = 0x5ea6a000u ^ (view + at);
        block[(at + 0x3ffffc) / 4] = 0x0dc0ffeeu ^ (view + at);
        good &= *(volatile ULONG *)(view + at) == (0x5ea6a000u ^ (view + at)) &&
                *(volatile ULONG *)(view + at + 0x3ffffc) == (0x0dc0ffeeu ^ (view + at));
        /* and the other way: written through the view, read through the block */
        *(volatile ULONG *)(view + at + 0x200000) = 0x600df00du ^ at;
        good &= block[(at + 0x200000) / 4] == (0x600df00du ^ at);
    }
    report("  0x%08lx: %lu MB at physical 0x%08lx: %s\n", view, size >> 20, physical,
           good ? "the same memory" : "NOT the same memory");
}

int main(void)
{
    static const ULONG views[] = {0x0c000000u, 0x0d000000u, 0x10000000u, 0x80000000u, 0x8c000000u, 0xa0000000u,
                                  0xa0400000u, 0xa0800000u, 0xac000000u, 0xe0000000u, 0xff000000u};
    MM_STATISTICS stats;
    volatile ULONG *block;
    unsigned k;

    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    serial_init();
    report("probe: the console's addresses on the Xbox\n");
    memset(&stats, 0, sizeof stats);
    stats.Length = sizeof stats;
    MmQueryStatistics(&stats);
    report("memory: %lu pages, %lu free (%lu MB free)\n", stats.TotalPhysicalPages, stats.AvailablePages,
           stats.AvailablePages >> 8);
    report("page directory entries:\n");
    for (k = 0; k < sizeof views / sizeof views[0]; k++) {
        show_entry(views[k]);
    }
    report("allocations at fixed addresses:\n");
    try_allocate(0x10000000u, 0x00400000u);
    report("the console's RAM, 16 MB contiguous, at each view:\n");
    block = MmAllocateContiguousMemoryEx(0x1000000, 0, 0x03ffffff, 0x400000, PAGE_READWRITE);
    if (block == NULL) {
        report("  no 16 MB block aligned on 4 MB\n");
    } else {
        ULONG physical = MmGetPhysicalAddress((PVOID)block);

        report("  block at 0x%08lx, physical 0x%08lx\n", (ULONG)block, physical);
        try_view(0x0c000000u, block, physical, 0x1000000);
        try_view(0x8c000000u, block, physical, 0x1000000);
        try_view(0xac000000u, block, physical, 0x1000000);
        try_view(0xe0000000u, block, physical, 0x1000000);
    }
    MmQueryStatistics(&stats);
    report("memory after: %lu MB free\n", stats.AvailablePages >> 8);
    report("probe: done\nPROBE END\n");
    for (;;) {
        Sleep(1000);
    }
    return 0;
}
