/* Kamui's frame against the submission owner's, engine/dc/owner.c, on a
 * model of the chip. engine/mk/owner.mk builds this file twice. one build
 * links Kamui's interupt_kamui_lib.c and kmsurfac_kamui_lib.c as they stand.
 * the other links owner.c, with the names it defines renamed out of them.
 * each build drives its frame through the same seeded events and prints a
 * digest a seed. the digest takes every register access in order and the
 * frame's calls out. it takes the device and statistics records after each
 * render, flip, reset and wake. it ends with the texture memory the
 * background packets land in. owner.mk requires the two outputs equal.
 *
 * registers keep their addresses, so the frame's code runs unedited. their
 * pages are mapped with no access, and a fault single steps each access.
 * KMDWORD is 32 bits: owner.mk compiles the frame's sources under -Dlong=int.
 * this file too defines long as int around Kamui's headers. the frame's
 * sources cast pointers to KMDWORD. so the image links low, and the lists lie
 * in a fixed arena below 4 GB. the chip wakes only after a register access,
 * between the frame's calls and in the wait callback. so no interrupt comes
 * between two plain stores to memory. this needs x86-64 Linux.
 *
 * owner_diff SEEDS FRAMES prints the digests and what the seeds reached.
 * owner_diff -t SEED FRAMES prints one seed's trace, to compare by hand.
 * owner_diff -6 SEEDS FRAMES runs the owner's frame alone at 60 fps. each
 * odd frame flips by owner_flip_on. owner_diff returns 1 where a flip times
 * out */
#define _GNU_SOURCE
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <unistd.h>

#define long int
#include <kamui.h>
#include <kmistat.h>
#undef long

/* vertex_buffer_model's bit that a NOBUF model sets */
#define NOBUF 0x20
#define LISTS 5
#define SLOTS 4

#define INT_STATUS 0xa05f6900U
#define INT_STATUS_ERROR 0xa05f6908U
#define SYNC_STATUS 0xa05f810cU
/* SYNC_STATUS's bit for the field the display shows */
#define SYNC_FIELD 0x400U
#define START_RENDER 0xa05f8014U
#define TA_ISP_CURRENT 0xa05f8138U
#define PVR_REGISTERS 0xa05f8000U
#define TCNT0 0xffd8000cU
/* the model's timer: TCNT0 starts each seed at TIMER_START and counts down
 * TIMER_STEP at each read. so the times the frame takes between two reads
 * are other than 0 */
#define TIMER_START 0xfffffff0U
#define TIMER_STEP 7

/* INT_STATUS's bits */
#define INT_RENDER_END 0x07U
#define INT_VBLANK_IN 0x08U
#define INT_HBLANK_IN 0x20U
#define INT_YUV_END 0x40U
#define INT_DMA2_END 0x80000U
/* INT_STATUS_ERROR's bits */
#define INT_STRIP_OVERRUN 0x02U
#define INT_ISP_OVERFLOW 0x04U
#define INT_OL_OVERFLOW 0x08U
/* each list's end bit in INT_STATUS, by ListType */
static const uint32_t list_ends[LISTS] = {0x80, 0x100, 0x200, 0x400, 0x200000};

/* the pages of the registers the frame's code touches, and each page's
 * words as the chip holds them */
#define PAGE_BYTES 4096U
#define PAGE_WORDS (PAGE_BYTES / 4)
#define PAGE_OF(address) ((address) & ~(PAGE_BYTES - 1))
static const uint32_t pages[] = {0xa05f6000, 0xa05f8000, 0xff000000, 0xffa00000,
                                 0xffd80000};
#define PAGES (sizeof(pages) / sizeof(pages[0]))
static uint32_t chip[PAGES][PAGE_WORDS];

/* texture memory and the arena that holds the lists, at fixed addresses
 * below 4 GB. both lie past the heap, which starts up to 1 GB above the
 * image */
#define TEXTURE_MEMORY 0xa5000000U
#define TEXTURE_BYTES 0x1000000U
/* bank 0's ISP parameters start BANK_BYTES into texture memory, and bank
 * 1's twice that. a bank's background packet lands where they end, from
 * OL_STEP below their start to ISP_REACH above it. so every packet lies in
 * the first PACKETS_BYTES */
#define BANK_BYTES 0x100000U
#define ISP_REACH 0x80000U
#define PACKETS_BYTES (3 * BANK_BYTES)
#define ARENA 0x60000000U
#define ARENA_BYTES 0x1000000U
#define LIST_BYTES 0x40000U
/* owner.c's grow moves a bank's limits by these steps. it keeps ol_limit
 * more than OL_FLOOR above isp_base */
#define ISP_STEP 0x2000U
#define OL_STEP 0x800U
#define OL_FLOOR 0x140U
/* the background packet's words: a bank's two limits stand its bytes apart */
#define PACKET_WORDS 48U
#define LIMIT_GAP (PACKET_WORDS * 4U)
#define MODIFIER_WORDS 8
#define MODIFIER (ARENA + ARENA_BYTES - MODIFIER_WORDS * 4)
#define SQ_BASE 0xe0000000U

/* what one seed makes likely, each in percent */
static struct {
    uint32_t vblank, render, list, isp, ol, strip, hblank, dma, yuv, full,
        overlong, between;
} odds;

/* the seed's state: the event stream's generator, the digest and whether the
 * trace prints */
static uint32_t random_state;
static uint64_t digest;
static int tracing;
static int rendering;
/* the owner's calls at 60 fps, which Kamui's build leaves null */
void owner_sixty(int on) __attribute__((weak));
KMSTATUS owner_flip_on(void) __attribute__((weak));
static int sixty;
/* set while the handler runs. the chip's level 6 interrupt doesn't nest, so
 * on_step wakes no chip then */
static int in_handler;

/* what the seeds reached, by the events the frame's code answers */
static struct {
    uint32_t frames, wakes, between, isp, ol, strip, renders, resets, fatal[8],
        timeouts_render, timeouts_ta, full;
} reached;

static uint32_t next(void)
{
    uint32_t x = random_state;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return random_state = x;
}

static int chance(uint32_t percent)
{
    return next() % 100 < percent;
}

/* fold a record's three words into the digest: 64-bit FNV-1a over words, not
 * bytes */
static void record(int kind, uint32_t a, uint32_t b)
{
    const uint32_t words[3] = {(uint32_t)kind, a, b};
    int i;

    for (i = 0; i < 3; i++) {
        digest = (digest ^ words[i]) * 0x100000001b3ULL;
    }
    if (tracing) {
        printf("%c %08x %08x\n", kind, a, b);
    }
}

/* a record whose last word is the bytes' 32-bit FNV-1a */
static void record_bytes(int kind, uint32_t a, const void *bytes, uint32_t size)
{
    const unsigned char *p = bytes;
    uint32_t hash = 2166136261U;
    uint32_t i;

    for (i = 0; i < size; i++) {
        hash = (hash ^ p[i]) * 16777619U;
    }
    record(kind, a, hash);
}

/* the two records as the frame's code left them. the callbacks' functions
 * are this image's addresses, so they stay out */
static void record_state(void)
{
    static struct kmi_device_extension device;

    memcpy(&device, (const void *)&kmiDeviceExtension, sizeof(device));
    memset(device.callback, 0, sizeof(device.callback));
    record_bytes('S', 0, &device, sizeof(device));
    record_bytes('T', 0, &kmiIntStatisticData, sizeof(kmiIntStatisticData));
}

static uint32_t *chip_word(uint32_t address)
{
    size_t i;

    for (i = 0; i < PAGES; i++) {
        if (PAGE_OF(address) == pages[i]) {
            return &chip[i][(address % PAGE_BYTES) / 4];
        }
    }
    return NULL;
}

/* a write as the chip takes it: the status registers clear the bits written */
static void chip_write(uint32_t address, uint32_t value)
{
    uint32_t *word = chip_word(address);

    if (address == INT_STATUS || address == INT_STATUS_ERROR) {
        *word &= ~value;
        return;
    }
    if (address == START_RENDER) {
        rendering = 1;
        reached.renders++;
    }
    *word = value;
}

static struct {
    uint32_t address;
    int write;
} access_now;

#define TRAP_FLAG 0x100

static void chip_wake(void);

/* an access to a register's page. load the word and step the instruction */
static void on_fault(int signal_number, siginfo_t *info, void *context)
{
    ucontext_t *uc = context;
    uintptr_t address = (uintptr_t)info->si_addr;
    uint32_t *word = address >> 32 ? NULL : chip_word((uint32_t)address);

    (void)signal_number;
    if (word == NULL) {
        fprintf(stderr, "owner_diff: stray access at %#zx\n", (size_t)address);
        _exit(2);
    }
    access_now.address = (uint32_t)address;
    access_now.write = (uc->uc_mcontext.gregs[REG_ERR] & 2) != 0;
    mprotect((void *)(uintptr_t)PAGE_OF(access_now.address), PAGE_BYTES,
             PROT_READ | PROT_WRITE);
    *(volatile uint32_t *)address = *word;
    uc->uc_mcontext.gregs[REG_EFL] |= TRAP_FLAG;
}

/* the instruction ran. take what it wrote and close the page */
static void on_step(int signal_number, siginfo_t *info, void *context)
{
    ucontext_t *uc = context;
    uint32_t address = access_now.address;
    uint32_t value = *(volatile uint32_t *)(uintptr_t)address;

    (void)signal_number;
    (void)info;
    uc->uc_mcontext.gregs[REG_EFL] &= ~TRAP_FLAG;
    mprotect((void *)(uintptr_t)PAGE_OF(address), PAGE_BYTES, PROT_NONE);
    if (access_now.write) {
        record('W', address, value);
        chip_write(address, value);
    } else {
        record('R', address, value);
        if (address == TCNT0) {
            *chip_word(address) -= TIMER_STEP;
        }
    }
    if (!in_handler && chance(odds.between)) {
        reached.between++;
        chip_wake();
    }
}

/* the frame's calls out, which both builds make to these functions */

static KMINT32 (*handler)(void);
static PKMDWORD bases[2][LISTS];
static PKMDWORD writes[2][LISTS];
PKMDWORD *kmiVertexBuff;
PKMDWORD *kmiCurrentVertexBuff;
volatile struct kmi_device_extension kmiDeviceExtension;

void owner_diff_prefetch(void *address)
{
    record('Q', (uint32_t)(uintptr_t)address, 0);
}

void kmiRegisterPhIntHandler(KMINT32 (*function)(void))
{
    handler = function;
}

KMINT32 kmiWriteRegister(KMDWORD reg, KMDWORD value)
{
    *(volatile KMDWORD *)(uintptr_t)(PVR_REGISTERS + reg) = value;
    return 0;
}

KMDWORD kmiReadRegister(KMDWORD reg)
{
    return *(volatile KMDWORD *)(uintptr_t)(PVR_REGISTERS + reg);
}

static void copy(int kind, void *destination, const void *source, uint32_t size)
{
    uint32_t address = (uint32_t)(uintptr_t)destination;

    record_bytes(kind, address, source, size);
    if (address < SQ_BASE) {
        memcpy(destination, source, size);
    }
}

void kmiMemCopy4(void *destination, void *source, KMDWORD size)
{
    copy('c', destination, source, size);
}

PKMDWORD kmiMemCopy8(PKMDWORD destination, PKMDWORD source, KMDWORD size)
{
    copy('C', destination, source, size);
    return destination;
}

KMSTATUS kmiDMAtoTARequest(KMDWORD target, const KMDWORD *source, KMDWORD size,
                           KMDWORD flags, KMDWORD **handle)
{
    (void)flags;
    (void)handle;
    if (chance(odds.full)) {
        reached.full++;
        record('d', target, (uint32_t)(uintptr_t)source);
        return DMA_QUEUE_FULL;
    }
    record('D', (uint32_t)(uintptr_t)source, size);
    return 0;
}

KMBOOLEAN kmiExecuteDMAChain(void)
{
    record('E', 0, 0);
    return 0;
}

KMINT32 kmiDMAHazardReset(void)
{
    record('H', 0, 0);
    return 0;
}

void kmiCreateRegionArray(KMDWORD width, KMDWORD height, KMDWORD index,
                          KMINT32 unused, KMINT32 bank, KMDWORD order)
{
    (void)unused;
    record('A', width << 16 | height,
           (uint32_t)index << 16 | (uint32_t)bank << 8 | order);
}

void kmiFlipVertexBuffer(KMDWORD bank)
{
    int list;

    record('V', bank, 0);
    kmiVertexBuff = bases[bank];
    kmiCurrentVertexBuff = writes[bank];
    for (list = 0; list < LISTS; list++) {
        writes[bank][list] = bases[bank][list];
    }
}

void kmiFatalErrorEntry(KMDWORD code)
{
    reached.fatal[code & 7]++;
    record('X', code, 0);
}

void kmiExCallBackFunc(PKMCALLBACKFUNC function, void *argument)
{
    (void)function;
    record('B', (uint32_t)(uintptr_t)argument, 0);
}

/* one wake of the chip, raising together what happened since the last. a
 * render ends at the seed's odds where one runs, and a list where it was
 * sent. either ends one time in a hundred where none was. one wake in five
 * puts TA_ISP_CURRENT within a word of the ol_limit - OL_STEP grow tests. the
 * rest put it in the bank's first ISP_REACH bytes */
static void chip_wake(void)
{
    uint32_t status = 0;
    uint32_t error = 0;
    uint32_t bank = (uint32_t)kmiDeviceExtension.ta_slot & 1;
    int list;

    reached.wakes++;
    if (chance(odds.vblank)) {
        status |= INT_VBLANK_IN;
    }
    if (chance(rendering ? odds.render : 1)) {
        status |= INT_RENDER_END;
        rendering = 0;
    }
    for (list = 0; list < LISTS; list++) {
        if (chance(kmiDeviceExtension.lists_sent >> list & 1 ? odds.list : 1)) {
            status |= list_ends[list];
        }
    }
    status |= chance(odds.hblank) ? INT_HBLANK_IN : 0;
    status |= chance(odds.dma) ? INT_DMA2_END : 0;
    status |= chance(odds.yuv) ? INT_YUV_END : 0;
    error |= chance(odds.isp) ? INT_ISP_OVERFLOW : 0;
    error |= chance(odds.ol) ? INT_OL_OVERFLOW : 0;
    error |= chance(odds.strip) ? INT_STRIP_OVERRUN : 0;
    reached.isp += (error & INT_ISP_OVERFLOW) != 0;
    reached.ol += (error & INT_OL_OVERFLOW) != 0;
    reached.strip += (error & INT_STRIP_OVERRUN) != 0;
    *chip_word(INT_STATUS) = status;
    *chip_word(INT_STATUS_ERROR) = error;
    *chip_word(SYNC_STATUS) = chance(50) ? SYNC_FIELD : 0;
    *chip_word(TA_ISP_CURRENT) =
        chance(20) ? kmiDeviceExtension.ta_bank[bank].ol_limit - OL_STEP +
                         (next() % 3 - 1) * 4
                   : kmiDeviceExtension.ta_bank[bank].isp_base +
                         (next() & (ISP_REACH - 4));
    record('I', status, error);
    in_handler = 1;
    handler();
    in_handler = 0;
    record_state();
}

static void wait_callback(void *argument)
{
    (void)argument;
    chip_wake();
}

/* the draws of a frame. each list in use grows by whole parameters, and may
 * pass its limit at the seed's odds. the direct list stays empty one time in
 * four */
static void draw(void)
{
    uint32_t direct = kmiDeviceExtension.vertex_buffer_list;
    int list;

    for (list = 0; list < LISTS; list++) {
        uint32_t room = kmiDeviceExtension.list_limit[list] / 4;

        if (!(kmiDeviceExtension.lists_used & (1U << list)) ||
            ((uint32_t)list == direct && chance(25))) {
            continue;
        }
        kmiCurrentVertexBuff[list] +=
            (next() % (chance(odds.overlong) ? room * 2 : room)) & ~7U;
    }
}

/* a seed's device and odds. about a third of the seeds run clean: the chip
 * keeps up and the timeouts are far off. the stuck seeds hold a render past
 * the second wait of owner.c's flip_wait */
static void set_up(void)
{
    static const uint32_t timeouts[] = {0, 1, 3, 8};
    static const uint32_t modes[] = {
        0,
        KM_DSPMODE_INTERLACE,
        KM_DSPMODE_HEIGHT_480,
        KM_DSPMODE_HEIGHT_480 | KM_DSPMODE_INTERLACE,
        KM_DSPMODE_HEIGHT_480 | KM_DSPMODE_WIDTH_640,
        KM_DSPMODE_HEIGHT_480 | KM_DSPMODE_PSEUDONONINTERLACE,
        KM_DSPMODE_HEIGHT_480 | KM_VGA};
    static const uint32_t capabilities[] = {
        KMI_CAP_ISP_CURRENT, KMI_CAP_SYNC_STATUS, KMI_CAP_RESET_BEFORE_RENDER,
        KMI_CAP_DUMMY_OPAQUE};
    volatile struct kmi_device_extension *d = &kmiDeviceExtension;
    uint32_t bank;
    uint32_t i;
    int clean;
    int list;
    int stuck;

    memset((void *)d, 0, sizeof(*d));
    memset(&kmiIntStatisticData, 0, sizeof(kmiIntStatisticData));
    memset(chip, 0, sizeof(chip));
    memset((void *)(uintptr_t)TEXTURE_MEMORY, 0, PACKETS_BYTES);
    *chip_word(TCNT0) = TIMER_START;
    rendering = 0;

    odds.vblank = 20 + next() % 70;
    odds.render = next() % 70;
    odds.list = 5 + next() % 60;
    odds.isp = chance(60) ? next() % 25 : 0;
    odds.ol = chance(60) ? next() % 35 : 0;
    odds.strip = chance(50) ? next() % 25 : 0;
    odds.hblank = 10;
    odds.dma = 20;
    odds.yuv = 3;
    odds.full = chance(50) ? next() % 25 : 0;
    odds.overlong = chance(30) ? next() % 40 : 0;
    odds.between = chance(50) ? next() % 15 : 0;
    stuck = chance(8);
    clean = !stuck && chance(35);
    if (stuck) {
        odds.render = 0;
        odds.list = 80;
    }
    if (clean) {
        odds.vblank = 50;
        odds.render = 70;
        odds.list = 70;
        odds.isp = odds.strip = odds.overlong = 0;
        odds.ol = chance(50) ? 4 : 0;
    }

    d->latency_model = KM_LATENCYMODEL_2V;
    /* the frame's code reads only the model's NOBUF bit, so its low bits
     * needn't follow vertex_buffer_list */
    d->vertex_buffer_model = NOBUF;
    d->vertex_buffer_list = chance(90) ? KM_OPAQUE_POLYGON : KM_TRANS_POLYGON;
    d->lists_used = (next() & 31) | 1U << d->vertex_buffer_list;
    for (i = 0; i < sizeof(capabilities) / sizeof(capabilities[0]); i++) {
        d->capabilities |= chance(50) ? capabilities[i] : 0;
    }
    d->opaque_opb = chance(70);
    d->translucent_opb = chance(70);
    d->presort = chance(50);
    d->opb_downward = chance(85);
    d->display_mode =
        (KMINT32)modes[next() % (sizeof(modes) / sizeof(modes[0]))];
    d->strip_buffer = chance(10);
    d->vsync_wait = stuck ? 2 : (KMINT32)(next() % 3);
    d->vsync_remaining = d->vsync_wait;
    d->render_timeout = stuck ? 64 : clean ? 8 : timeouts[next() % 4];
    d->vertex_timeout = clean ? 8 : timeouts[next() % 4];
    d->tiles_x = 20;
    d->tiles_y = 15;
    d->ta_options = next() & 1;
    d->regions_stale = (KMINT32)(next() & 3);
    d->display_changed = chance(30);
    d->video_control = next();
    d->fb_read_control = next();
    d->fb_write_control = next();
    d->fb_write_stride = next();
    d->fb_clip_x = next();
    d->fb_clip_y = next();
    d->scaler_control = next() & 0xffff;
    d->background_skip = next();
    d->background_depth = next();
    d->fb_start[0] = 0x200000;
    d->fb_start[1] = 0x400000;
    d->field_offset[0] = 0;
    d->field_offset[1] = 1280;
    d->common_modifier = chance(30);
    d->common_modifier_data = (PKMDWORD)(uintptr_t)MODIFIER;
    d->common_modifier_size = MODIFIER_WORDS * 4;
    for (i = 0; i < PACKET_WORDS; i++) {
        d->background_packet[i] = next();
    }
    for (bank = 0; bank < 2; bank++) {
        volatile struct kmi_ta_bank *b = &d->ta_bank[bank];

        b->region_base = 0x700000 + bank * 0x10000;
        b->isp_base = BANK_BYTES * (bank + 1);
        b->isp_limit = b->isp_base + 0x400 + (next() & 0x3ffc);
        b->ol_limit = b->isp_limit + LIMIT_GAP;
        b->ol_base = b->ol_limit + (next() & 0x7ffc);
        /* each limit may lie a few steps from its bound in grow, within a
         * word either way */
        if (chance(40)) {
            b->ol_base = b->isp_limit + ISP_STEP * (1 + next() % 3) +
                         LIMIT_GAP + (next() % 3 - 1) * 4;
        }
        if (chance(40)) {
            b->ol_limit = b->isp_base + OL_FLOOR + OL_STEP * (1 + next() % 3) +
                          (next() % 3 - 1) * 4;
            b->isp_limit = b->ol_limit - LIMIT_GAP;
        }
        b->next_opb_init = b->ol_base + 0x1000;
        for (list = 0; list < LISTS; list++) {
            bases[bank][list] =
                (PKMDWORD)(uintptr_t)(ARENA +
                                      (bank * LISTS + list) * LIST_BYTES);
            writes[bank][list] = bases[bank][list];
        }
    }
    for (list = 0; list < LISTS; list++) {
        d->list_limit[list] = 0x400 + (next() & 0xfe0);
    }
    for (i = 0; i < CALLBACKS; i++) {
        d->callback[i].argument = (void *)(uintptr_t)(0x100 + i);
    }
    d->callback[WAIT_VSYNC_CALLBACK].function = wait_callback;
    for (i = 0; i < SLOTS; i++) {
        d->slot_state[i] = SLOT_FREE;
    }
    d->slot_state[0] = SLOT_FILLING;
    kmiVertexBuff = bases[0];
    kmiCurrentVertexBuff = writes[0];
    kmiInitInterrupt();
}

static void run_seed(uint32_t seed, uint32_t frames)
{
    uint32_t frame;
    uint32_t wakes;

    random_state = seed * 2654435761U + 1;
    digest = 14695981039346656037ULL;
    set_up();
    if (sixty) {
        owner_sixty(1);
    }
    for (frame = 0; frame < frames; frame++) {
        reached.frames++;
        draw();
        if (chance(3)) {
            kmResetPointer();
            reached.resets++;
            record_state();
            continue;
        }
        record('r', kmRender(), 0);
        record_state();
        for (wakes = next() % 3; wakes > 0; wakes--) {
            chip_wake();
        }
        record('f', sixty && frame & 1 ? owner_flip_on() : kmFlipFrameBuffer(),
               0);
        record_state();
        for (wakes = next() % 3; wakes > 0; wakes--) {
            chip_wake();
        }
    }
    reached.timeouts_render += kmiIntStatisticData.render_timeouts;
    reached.timeouts_ta += kmiIntStatisticData.ta_timeouts;
    /* each background packet, whose stores to texture memory no fault
     * records */
    record_bytes('Z', 0, (const void *)(uintptr_t)TEXTURE_MEMORY,
                 PACKETS_BYTES);
}

int main(int argc, char **argv)
{
    struct sigaction action;
    size_t i;
    uint32_t seed;
    uint32_t seeds;
    uint32_t frames;
    int one;

    one = argc == 4 && strcmp(argv[1], "-t") == 0;
    sixty = argc == 4 && strcmp(argv[1], "-6") == 0;
    if ((argc != 3 && !one && !sixty) || (sixty && owner_sixty == NULL)) {
        fprintf(stderr, "usage: owner_diff SEEDS FRAMES | owner_diff -t SEED "
                        "FRAMES | owner_diff -6 SEEDS FRAMES (the owner's "
                        "build alone)\n");
        return 2;
    }
    seeds = (uint32_t)strtoul(argv[1 + one + sixty], NULL, 0);
    frames = (uint32_t)strtoul(argv[2 + one + sixty], NULL, 0);
    for (i = 0; i < PAGES; i++) {
        if (mmap((void *)(uintptr_t)pages[i], PAGE_BYTES, PROT_NONE,
                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1,
                 0) == MAP_FAILED) {
            perror("owner_diff: a register's page");
            return 2;
        }
    }
    if (mmap((void *)(uintptr_t)TEXTURE_MEMORY, TEXTURE_BYTES,
             PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1,
             0) == MAP_FAILED ||
        mmap((void *)(uintptr_t)ARENA, ARENA_BYTES, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1,
             0) == MAP_FAILED) {
        perror("owner_diff: the arena");
        return 2;
    }
    memset(&action, 0, sizeof(action));
    action.sa_flags = SA_SIGINFO | SA_NODEFER;
    action.sa_sigaction = on_fault;
    sigaction(SIGSEGV, &action, NULL);
    action.sa_sigaction = on_step;
    sigaction(SIGTRAP, &action, NULL);
    if (one) {
        tracing = 1;
        run_seed(seeds, frames);
        return 0;
    }
    for (seed = 1; seed <= seeds; seed++) {
        run_seed(seed, frames);
        printf("seed %u %016llx\n", seed, (unsigned long long)digest);
    }
    printf("reached: %u frames, %u wakes, %u of them between accesses, %u "
           "renders, %u resets; %u ISP overflows, "
           "%u object list overflows, %u strip overruns; %u render and %u TA "
           "timeouts; %u full DMA queues; fatal",
           reached.frames, reached.wakes, reached.between, reached.renders,
           reached.resets, reached.isp, reached.ol, reached.strip,
           reached.timeouts_render, reached.timeouts_ta, reached.full);
    for (i = 0; i < 8; i++) {
        printf(" %u", reached.fatal[i]);
    }
    printf("\n");
    return sixty && reached.fatal[KMI_FATAL_FLIP_TIMEOUT] != 0;
}
