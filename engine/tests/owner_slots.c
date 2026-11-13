/* the submission owner's slots, engine/dc/owner.c, on a model of the chip
 * that any host runs. a picture owns its slot's bank and the background
 * plane it finished with. the plane is the packet owner.c copies past the
 * picture's ISP parameters, with its tag and depth. each scenario queues
 * pictures, each with a plane of its own. at each render's start, the
 * renderer's registers and the packet they find must be its picture's. no
 * bank may open until the picture it held is past its render.
 *
 * engine/mk/owner.mk compiles owner.c as it stands, with a 32-bit KMDWORD
 * under -Dlong=int. owner.c's WORD takes the machine's words from this file,
 * since a host may map nothing at their addresses. a store to a word is
 * plain. so the model sets the interrupt status before each call of the
 * handler, which reads it once.
 *
 * the events are those the chip raises. a picture's lists end once, a render
 * ends where one runs, and a vblank comes at any time. one interrupt can
 * carry more than one of them.
 *
 * each flip in a scenario takes a script of events, one a callback of its
 * wait. the events in brackets come in one interrupt. a flip fails where it
 * goes on ahead of its script's end or waits past it. so each script holds
 * its flip to the event that lets it go on.
 *
 * owner_slots [SEEDS FRAMES] runs the scenarios, then SEEDS seeded runs of
 * FRAMES frames, 64 of 256 by default. it prints the first four failures of
 * each scenario or seed, and returns 1 where any fails */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define long int
#include <kamui.h>
#include <kmistat.h>
#undef long

#include "owner.h"

#define NOBUF 0x20
#define LISTS 5
#define SLOTS 4
#define PACKET_WORDS 48
#define LIST_WORDS 1024

#define INT_STATUS 0xa05f6900U
#define INT_STATUS_ERROR 0xa05f6908U
#define PVR_REGISTERS 0xa05f8000U
#define START_RENDER 0xa05f8014U
#define PARAM_BASE 0xa05f8020U
#define REGION_BASE 0xa05f802cU
#define FB_WRITE_START 0xa05f8060U
#define BACKGROUND_DEPTH 0xa05f8088U
#define BACKGROUND_TAG 0xa05f808cU
#define TA_ISP_BASE 0xa05f8128U
#define TA_ISP_CURRENT 0xa05f8138U

/* INT_STATUS's bits. INT_ISP_OVERFLOW is INT_STATUS_ERROR's */
#define INT_RENDER_END 0x07U
#define INT_VBLANK_IN 0x08U
#define INT_ISP_OVERFLOW 0x04U
static const uint32_t list_ends[LISTS] = {0x80, 0x100, 0x200, 0x400, 0x200000};

/* BACKGROUND_TAG's fields: the plane's skip, and its packet's offset in the
 * ISP parameters, in words from bit 3 */
#define TAG_SKIP 0xff000000U
#define TAG_OFFSET 0x00fffff8U
#define TAG_BYTES(tag) (((tag) & TAG_OFFSET) >> 1)

/* the pages of the registers the owner touches, and the texture memory its
 * two banks lie in. set_up puts the banks BANK_BYTES apart */
#define PAGE_BYTES 4096U
#define PAGE_OF(address) ((address) & ~(PAGE_BYTES - 1))
static const uint32_t pages[] = {0xa05f6000, 0xa05f8000, 0xff000000, 0xffa00000,
                                 0xffd80000};
#define PAGES (sizeof(pages) / sizeof(pages[0]))
static uint32_t chip[PAGES][PAGE_BYTES / 4];
#define TEXTURE_MEMORY 0xa5000000U
#define BANK_BYTES 0x10000U
#define ISP_BYTES 0x4000U
static uint32_t texture[3 * BANK_BYTES / 4];

/* a picture: its number in its scenario, its ISP parameters' end and the
 * plane the game set */
struct picture {
    uint32_t number;
    uint32_t end;
    uint32_t skip;
    uint32_t depth;
    uint32_t packet[PACKET_WORDS];
};

/* pictures counts every scenario's draws, and first holds its value as this
 * one began. the count gives each picture its plane. owner.c's slots keep
 * their planes across scenarios, so a plane left there is no later
 * picture's. rendered holds the last eight pictures rendered */
static struct picture held[SLOTS];
static struct picture drawn;
static uint32_t pictures;
static uint32_t first;
static uint32_t rendered[8];
static uint32_t renders;
/* TRUE from a render's start to its end, and from a flip to its lists' end.
 * each age counts the vblanks so far */
static int rendering;
static int taking;
static uint32_t render_age;
static uint32_t taking_age;

/* the scenario that runs, the failures it has printed, and every scenario's
 * failures */
static const char *scenario;
static unsigned printed;
static unsigned failures;
/* a flip's script, in the letters interrupt reads, and the callbacks a
 * seeded flip's wait has run */
static const char *script;
static uint32_t random_state;
static uint32_t waited;
/* finished counts the lists' ends that come while a finished slot waits for
 * its render. reset counts the resets, and left the ends of 60 fps, that
 * come then. together counts the seeds' interrupts of more than one event */
static struct {
    uint32_t finished, reset, left, together;
} reached;

static KMINT32 (*handler)(void);
static KMDWORD lists[2][LISTS][LIST_WORDS];
static PKMDWORD bases[2][LISTS];
static PKMDWORD writes[2][LISTS];
PKMDWORD *kmiVertexBuff;
PKMDWORD *kmiCurrentVertexBuff;
volatile struct kmi_device_extension kmiDeviceExtension;
struct kmi_interrupt_statistics kmiIntStatisticData;

#define DEVICE kmiDeviceExtension

static void fail(const char *format, ...)
{
    va_list arguments;

    failures++;
    if (printed++ < 4) {
        printf("%s: ", scenario);
        va_start(arguments, format);
        vprintf(format, arguments);
        va_end(arguments);
        printf("\n");
    }
}

/* TRUE where a slot the tile accelerator finished waits for its render */
static int slot_waits(void)
{
    int slot;

    for (slot = 0; slot < SLOTS; slot++) {
        if (DEVICE.slot_state[slot] == SLOT_TA_DONE) {
            return 1;
        }
    }
    return 0;
}

/* the model's word at address, or null where it keeps none there */
static uint32_t *word_at(uint32_t address)
{
    size_t i;

    for (i = 0; i < PAGES; i++) {
        if (PAGE_OF(address) == pages[i]) {
            return &chip[i][(address % PAGE_BYTES) / 4];
        }
    }
    if (address >= TEXTURE_MEMORY &&
        address - TEXTURE_MEMORY < sizeof(texture)) {
        return &texture[(address - TEXTURE_MEMORY) / 4];
    }
    return NULL;
}

static uint32_t *word(uint32_t address)
{
    uint32_t *found = word_at(address);

    if (found == NULL) {
        printf("%s: a stray access at %#x\n", scenario, address);
        exit(1);
    }
    return found;
}

/* owner.c's WORD */
volatile KMDWORD *owner_slots_word(KMDWORD address)
{
    return word(address);
}

/* the owner's calls out. a write of TA_ISP_BASE opens a bank, and the tile
 * accelerator writes its lists over from there. so no slot but the write slot
 * may hold a picture in that bank not past its render */

void kmiRegisterPhIntHandler(KMINT32 (*function)(void))
{
    handler = function;
}

KMINT32 kmiWriteRegister(KMDWORD reg, KMDWORD value)
{
    int slot;

    if (PVR_REGISTERS + reg == TA_ISP_BASE) {
        for (slot = 0; slot < SLOTS; slot++) {
            KMINT32 state = DEVICE.slot_state[slot];

            if (DEVICE.ta_bank[slot & 1].isp_base == value &&
                slot != DEVICE.write_slot &&
                (state == SLOT_IN_TA || state == SLOT_TA_DONE ||
                 state == SLOT_RENDERING)) {
                fail("bank %d opens under picture %u, which slot %d holds "
                     "in state %d",
                     slot & 1, held[slot].number, slot, state);
            }
        }
    }
    *word(PVR_REGISTERS + reg) = value;
    return 0;
}

KMDWORD kmiReadRegister(KMDWORD reg)
{
    return *word(PVR_REGISTERS + reg);
}

/* a copy into a list. any other target is a store queue's, which no host
 * holds */
void kmiMemCopy4(void *destination, void *source, KMDWORD size)
{
    uintptr_t at = (uintptr_t)destination;

    if (at >= (uintptr_t)lists && at - (uintptr_t)lists < sizeof(lists)) {
        memcpy(destination, source, size);
    }
}

KMSTATUS kmiDMAtoTARequest(KMDWORD target, const KMDWORD *source, KMDWORD size,
                           KMDWORD flags, KMDWORD **handle)
{
    (void)target;
    (void)source;
    (void)size;
    (void)flags;
    (void)handle;
    return 0;
}

KMBOOLEAN kmiExecuteDMAChain(void)
{
    return 0;
}

KMINT32 kmiDMAHazardReset(void)
{
    return 0;
}

void kmiCreateRegionArray(KMDWORD width, KMDWORD height, KMDWORD index,
                          KMINT32 unused, KMINT32 bank, KMDWORD order)
{
    (void)width;
    (void)height;
    (void)index;
    (void)unused;
    (void)bank;
    (void)order;
}

void kmiSetOpaqueDummy(void)
{
}

void kmiFlipVertexBuffer(KMDWORD bank)
{
    int list;

    kmiVertexBuff = bases[bank];
    kmiCurrentVertexBuff = writes[bank];
    for (list = 0; list < LISTS; list++) {
        writes[bank][list] = bases[bank][list];
    }
}

/* no run overfills a list or a bank, or holds a flip to its timeout */
void kmiFatalErrorEntry(KMDWORD code)
{
    fail("the owner stops on fatal error %u", code);
}

void kmiExCallBackFunc(PKMCALLBACKFUNC function, void *argument)
{
    (void)function;
    (void)argument;
}

/* a render starts. the renderer takes the lists at PARAM_BASE and the plane's
 * packet at the tag's offset from there. fail where what it finds isn't the
 * picture slot holds */
static void render_started(int slot)
{
    const struct picture *picture = &held[slot];
    volatile struct kmi_ta_bank *bank = &DEVICE.ta_bank[slot & 1];
    uint32_t base = *word(PARAM_BASE);
    uint32_t tag = *word(BACKGROUND_TAG);
    uint32_t own = (picture->skip & TAG_SKIP) |
                   ((picture->end - bank->isp_base) << 1 & TAG_OFFSET);
    const uint32_t *packet = word_at(TEXTURE_MEMORY + base + TAG_BYTES(tag));

    if (renders != 0 && picture->number <= rendered[(renders - 1) % 8]) {
        fail("picture %u in slot %d renders after picture %u", picture->number,
             slot, rendered[(renders - 1) % 8]);
    }
    rendering = 1;
    render_age = 0;
    rendered[renders++ % 8] = picture->number;
    if (base != bank->isp_base || *word(REGION_BASE) != bank->region_base ||
        *word(FB_WRITE_START) != DEVICE.fb_start[slot & 1]) {
        fail("picture %u in slot %d renders from lists at %#x, a region "
             "array at %#x and into a frame buffer at %#x, and its bank's "
             "are %#x, %#x and %#x",
             picture->number, slot, base, *word(REGION_BASE),
             *word(FB_WRITE_START), bank->isp_base, bank->region_base,
             DEVICE.fb_start[slot & 1]);
    }
    if (tag != own) {
        fail("picture %u in slot %d renders with tag %#010x, and its own is "
             "%#010x",
             picture->number, slot, tag, own);
    }
    if (*word(BACKGROUND_DEPTH) != (picture->depth & ~15U)) {
        fail("picture %u in slot %d renders with depth %#010x, and its own "
             "is %#010x",
             picture->number, slot, *word(BACKGROUND_DEPTH),
             picture->depth & ~15U);
    }
    if (packet == NULL ||
        word_at(TEXTURE_MEMORY + base + TAG_BYTES(tag) + PACKET_WORDS * 4) ==
            NULL ||
        memcmp(packet, picture->packet, sizeof(picture->packet)) != 0) {
        fail("picture %u in slot %d renders with the packet at %#x, which "
             "isn't its own",
             picture->number, slot, base + TAG_BYTES(tag));
    }
}

/* raise one interrupt of these events and run the handler. l ends the lists
 * of the tile accelerator's slot. v is a vblank, r a render's end and o an
 * ISP overflow */
static void interrupt(const char *events)
{
    uint32_t status = 0;
    uint32_t error = 0;
    int list;

    for (; *events != '\0'; events++) {
        switch (*events) {
        case 'l':
            reached.finished += (uint32_t)slot_waits();
            *word(TA_ISP_CURRENT) = held[DEVICE.ta_slot].end;
            for (list = 0; list < LISTS; list++) {
                status |= DEVICE.lists_sent >> list & 1 ? list_ends[list] : 0;
            }
            taking = 0;
            break;
        case 'v':
            status |= INT_VBLANK_IN;
            render_age += (uint32_t)rendering;
            taking_age += (uint32_t)taking;
            break;
        case 'r':
            status |= INT_RENDER_END;
            rendering = 0;
            break;
        case 'o':
            error |= INT_ISP_OVERFLOW;
            break;
        }
    }
    *word(INT_STATUS) = status;
    *word(INT_STATUS_ERROR) = error;
    *word(START_RENDER) = 0;
    handler();
    if (*word(START_RENDER) != 0) {
        render_started(DEVICE.render_slot);
    }
}

/* raise one event alone */
static void event(int kind)
{
    char events[2] = {(char)kind, '\0'};

    interrupt(events);
}

static uint32_t next(void)
{
    uint32_t x = random_state;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return random_state = x;
}

/* a seeded interrupt. each event the chip can raise now comes one time in
 * three, or a vblank where none does. renders and lists end within two
 * vblanks, inside the owner's timeouts */
static void event_seeded(void)
{
    uint32_t pick = next();
    char events[4];
    int count = 0;

    if (rendering && (render_age >= 2 || pick % 3 == 0)) {
        events[count++] = 'r';
    }
    if (taking && (taking_age >= 2 || (pick >> 8) % 3 == 0)) {
        events[count++] = 'l';
    }
    if (count == 0 || (pick >> 16) % 3 == 0) {
        events[count++] = 'v';
    }
    events[count] = '\0';
    reached.together += (uint32_t)(count > 1);
    interrupt(events);
}

/* a flip's wait raises the script's next interrupt, or with no script a
 * seeded one */
static void wait_callback(void *argument)
{
    char events[4];
    size_t count;

    (void)argument;
    if (script == NULL) {
        if (++waited > 64) {
            printf("%s: a flip waits past 64 events\n", scenario);
            exit(1);
        }
        event_seeded();
        return;
    }
    if (*script == '\0') {
        printf("%s: a flip waits past its events\n", scenario);
        exit(1);
    }
    if (*script != '(') {
        event(*script++);
        return;
    }
    count = strcspn(++script, ")");
    if (count >= sizeof(events) || script[count] == '\0') {
        printf("%s: a script's brackets hold %s\n", scenario, script);
        exit(1);
    }
    memcpy(events, script, count);
    events[count] = '\0';
    script += count + 1;
    interrupt(events);
}

/* the device in the configuration the game sets, on VGA and a chip that
 * reports TA_ISP_CURRENT. both banks have room to grow. the owner's frame
 * runs at the rate asked */
static void set_up(const char *name, int sixty)
{
    volatile struct kmi_device_extension *d = &kmiDeviceExtension;
    uint32_t bank;
    int list;
    int i;

    scenario = name;
    printed = 0;
    memset((void *)d, 0, sizeof(*d));
    memset(&kmiIntStatisticData, 0, sizeof(kmiIntStatisticData));
    memset(chip, 0, sizeof(chip));
    memset(texture, 0, sizeof(texture));
    memset(held, 0, sizeof(held));
    first = pictures;
    renders = 0;
    rendering = 0;
    taking = 0;
    script = "";
    d->latency_model = KM_LATENCYMODEL_2V;
    d->vertex_buffer_model = NOBUF;
    d->vertex_buffer_list = KM_OPAQUE_POLYGON;
    d->lists_used = 1U << KM_OPAQUE_POLYGON | 1U << KM_TRANS_POLYGON;
    d->capabilities = KMI_CAP_ISP_CURRENT | KMI_CAP_SYNC_STATUS;
    d->opb_downward = 1;
    d->display_mode = KM_VGA;
    d->render_timeout = 8;
    d->vertex_timeout = 8;
    d->tiles_x = 20;
    d->tiles_y = 15;
    for (bank = 0; bank < 2; bank++) {
        volatile struct kmi_ta_bank *b = &d->ta_bank[bank];

        b->isp_base = BANK_BYTES * (bank + 1);
        b->isp_limit = b->isp_base + ISP_BYTES;
        b->ol_limit = b->isp_limit + PACKET_WORDS * 4;
        b->ol_base = b->isp_base + BANK_BYTES - 0x1000;
        b->next_opb_init = b->ol_base;
        b->region_base = b->ol_base + 0x800;
        d->fb_start[bank] = 0x200000 * (bank + 1);
        for (list = 0; list < LISTS; list++) {
            bases[bank][list] = lists[bank][list];
            writes[bank][list] = lists[bank][list];
        }
    }
    for (list = 0; list < LISTS; list++) {
        d->list_limit[list] = LIST_WORDS * 4 - 64;
    }
    d->callback[WAIT_VSYNC_CALLBACK].function = wait_callback;
    for (i = 0; i < SLOTS; i++) {
        d->slot_state[i] = SLOT_FREE;
    }
    d->slot_state[0] = SLOT_FILLING;
    kmiVertexBuff = bases[0];
    kmiCurrentVertexBuff = writes[0];
    kmiInitInterrupt();
    /* the frame is Kamui's from a vblank at which no slot waits */
    owner_sixty(0);
    event('v');
    owner_sixty(sixty);
}

/* the game's draws of the next picture: its lists and its background plane,
 * as kmSetBackGroundPlane leaves it. every picture's plane is its own. its
 * ISP parameters end where no picture near it ends in either bank */
static void draw(void)
{
    volatile struct kmi_device_extension *d = &kmiDeviceExtension;
    uint32_t n = pictures++;
    int i;

    drawn.number = n - first;
    drawn.end =
        d->ta_bank[d->write_slot & 1].isp_base + 0x400 + n * 0x2c0 % 0x3000;
    drawn.skip = (n % 7 + 1) << 24;
    drawn.depth = 0x3f000005 + n * 0x100;
    for (i = 0; i < PACKET_WORDS; i++) {
        drawn.packet[i] = n * 0x01000193U + (uint32_t)i;
        d->background_packet[i] = drawn.packet[i];
    }
    d->background_skip = drawn.skip;
    d->background_depth = drawn.depth;
    kmiCurrentVertexBuff[KM_OPAQUE_POLYGON] += 8;
    kmiCurrentVertexBuff[KM_TRANS_POLYGON] += 8 * (n % 5 + 1);
}

/* draw a picture and flip it, by owner_flip_on where on is TRUE. its wait
 * raises the script's events, or seeded ones where events is null */
static void flip(int on, const char *events)
{
    int slot = DEVICE.write_slot;
    KMSTATUS status;

    draw();
    if (kmRender() != KMSTATUS_SUCCESS) {
        fail("kmRender refuses picture %u", drawn.number);
    }
    held[slot] = drawn;
    script = events;
    waited = 0;
    taking = 1;
    taking_age = 0;
    status = on ? owner_flip_on() : kmFlipFrameBuffer();
    if (status != KMSTATUS_SUCCESS) {
        fail("the flip refuses picture %u", drawn.number);
    }
    if (events != NULL && *script != '\0') {
        fail("picture %u's flip goes on ahead of its events %s", drawn.number,
             script);
    }
}

/* fail unless the scenario's renders were of these pictures, in this order */
static void expect(const char *order)
{
    char found[2 * 8 + 1];
    uint32_t i;

    for (i = 0; i < renders && i < 8; i++) {
        found[2 * i] = (char)('0' + rendered[i]);
        found[2 * i + 1] = ' ';
    }
    found[i == 0 ? 0 : 2 * i - 1] = '\0';
    if (renders > 8 || strcmp(found, order) != 0) {
        fail("%u renders, of pictures %s, and the scenario's are %s", renders,
             found, order);
    }
}

/* one picture a frame. each renders from the vblank its flip waits for, by
 * which the one before has rendered. six take every slot, and two again */
static void one_a_frame(const char *name, int sixty)
{
    int i;

    set_up(name, sixty);
    flip(0, "lv");
    for (i = 1; i < 6; i++) {
        flip(0, "lrv");
    }
    expect("0 1 2 3 4 5");
}

/* split frames, frames of play that show two pictures. the tile accelerator
 * finishes both before a vblank. the first waits for its render as the
 * second's lists end. after picture 0, the pairs take slots 1 and 2, then 3
 * and 0 */
static void two_finished(void)
{
    int i;

    set_up("two finished before a vblank", 1);
    flip(0, "lv");
    event('r');
    for (i = 0; i < 3; i++) {
        flip(1, "l");
        flip(0, "lvr");
        event('v');
        event('r');
    }
    expect("0 1 2 3 4 5 6");
}

/* a second picture still waits for its render when its flip ends, as a late
 * frame leaves it. the next frame's first picture's lists end ahead of that
 * render */
static void late_second(void)
{
    int i;

    set_up("a second picture waits as the next frame's first finishes", 1);
    flip(1, "l");
    event('v');
    flip(0, "vlr");
    for (i = 0; i < 2; i++) {
        flip(1, "lvr");
        event('v');
        flip(0, "vlr");
    }
    event('v');
    expect("0 1 2 3 4 5");
}

/* kmResetPointer throws a picture away while the one before it waits. the
 * picture drawn in its place has its lists end ahead of that render */
static void reset(void)
{
    set_up("a reset while a picture waits", 1);
    flip(1, "l");
    draw();
    kmResetPointer();
    flip(0, "lvr");
    event('v');
    expect("0 2");
}

/* two ISP overflows while a picture waits, each growing both banks. one
 * comes in the next picture's direct list. the other comes in its flip's
 * wait, ahead of its lists' end and the first's render */
static void overflow(void)
{
    set_up("an overflow while a picture waits", 1);
    flip(1, "l");
    event('o');
    flip(0, "olvr");
    event('v');
    expect("0 1");
    if (kmiIntStatisticData.isp_overflows != 2) {
        fail("%u ISP overflows counted of 2",
             kmiIntStatisticData.isp_overflows);
    }
}

/* 60 fps ends with a picture finished and waiting for its render. the next
 * flip goes on at the vblank that starts its render, not at the first
 * render's end. the frame is Kamui's from that vblank. the flip after it has
 * a vblank ahead of its lists' end. it still waits for the render before it
 * and then a vblank. at 60 fps it would go on at that render's end */
static void leaving(void)
{
    set_up("60 fps ends as a picture waits", 1);
    flip(0, "vl");
    owner_sixty(0);
    flip(0, "lvrv");
    flip(0, "vlrv");
    expect("0 1 2");
}

/* 60 fps ends with no picture waiting, one picture a frame. the next flip
 * goes on at the vblank that starts its render and ends the 60 fps frame. it
 * doesn't wait that render out */
static void leaving_idle(void)
{
    set_up("60 fps ends with no picture waiting", 1);
    flip(0, "lv");
    flip(0, "lrv");
    owner_sixty(0);
    flip(0, "lrv");
    flip(0, "vlrv");
    expect("0 1 2 3");
}

/* 60 fps ends ahead of a split frame's last flip, while its first picture
 * renders. the game asks the rate there too. the last flip goes on at the
 * vblank that starts its own render */
static void leaving_split(void)
{
    set_up("60 fps ends ahead of a split frame's last flip", 1);
    flip(0, "lv");
    event('r');
    flip(1, "l");
    event('v');
    owner_sixty(0);
    flip(0, "lrv");
    flip(0, "vlrv");
    expect("0 1 2 3");
}

/* 60 fps starts with a picture finished and waiting. its lists overflow,
 * then end in one interrupt with the vblank that ends its flip. Kamui's frame
 * leaves it so. it renders at the next vblank over its own plane, which its
 * lists' end left at 30 fps. the device holds the next picture's plane by
 * then */
static void entering(void)
{
    set_up("60 fps starts as a picture waits", 0);
    flip(0, "o(vl)");
    owner_sixty(1);
    flip(0, "vrl");
    event('v');
    expect("0 1");
}

/* the rate a seeded run asks from here: the other, one time in sixteen */
static int rate_seeded(int sixty)
{
    if (next() % 16 != 0) {
        return sixty;
    }
    reached.left += (uint32_t)(sixty && slot_waits());
    owner_sixty(!sixty);
    return !sixty;
}

/* a seed's frames. each shows one picture, or at 60 fps may split into two.
 * one frame in eight throws its last picture away. the chip's interrupts
 * come between the calls as within the flips. the slots move each vblank, or
 * one in two or three. the rate changes between frames. it also changes
 * where the game asks it, ahead of a frame's last flip or reset. every
 * picture flipped renders once */
static void seeded(uint32_t seed, uint32_t frames)
{
    static char name[32];
    uint32_t flipped = 0;
    uint32_t frame;
    uint32_t events;
    int sixty = 1;

    snprintf(name, sizeof(name), "seed %u", seed);
    random_state = seed * 2654435761U + 1;
    set_up(name, sixty);
    DEVICE.vsync_wait = (KMINT32)(seed % 3);
    for (frame = 0; frame < frames; frame++) {
        sixty = rate_seeded(sixty);
        if (sixty && next() % 2 == 0) {
            flip(1, NULL);
            flipped++;
            for (events = next() % 3; events > 0; events--) {
                event_seeded();
            }
        }
        sixty = rate_seeded(sixty);
        if (next() % 8 == 0) {
            reached.reset += (uint32_t)slot_waits();
            draw();
            kmResetPointer();
        } else {
            flip(0, NULL);
            flipped++;
        }
        for (events = next() % 3; events > 0; events--) {
            event_seeded();
        }
    }
    for (events = 0; events < 16; events++) {
        event(rendering ? 'r' : 'v');
    }
    if (renders != flipped) {
        fail("%u renders of %u pictures flipped", renders, flipped);
    }
}

int main(int argc, char **argv)
{
    uint32_t seeds = argc == 3 ? (uint32_t)strtoul(argv[1], NULL, 0) : 64;
    uint32_t frames = argc == 3 ? (uint32_t)strtoul(argv[2], NULL, 0) : 256;
    uint32_t seed;

    if (argc != 1 && argc != 3) {
        fprintf(stderr, "usage: owner_slots [SEEDS FRAMES]\n");
        return 2;
    }
    one_a_frame("one picture a frame at 30 fps", 0);
    one_a_frame("one picture a frame at 60 fps", 1);
    two_finished();
    late_second();
    reset();
    overflow();
    leaving();
    leaving_idle();
    leaving_split();
    entering();
    for (seed = 1; seed <= seeds; seed++) {
        seeded(seed, frames);
    }
    if (seeds != 0 && (reached.finished == 0 || reached.reset == 0 ||
                       reached.left == 0 || reached.together == 0)) {
        fail("the seeds reach %u ends of lists, %u resets and %u ends of 60 "
             "fps while a slot waits, and %u interrupts of more than one "
             "event",
             reached.finished, reached.reset, reached.left, reached.together);
    }
    if (failures != 0) {
        printf("%u failures\n", failures);
    }
    return failures != 0;
}
