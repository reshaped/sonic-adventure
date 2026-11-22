/* Kamui, the PowerVR's driver, on the port's side: the calls Ninja and the
 * game make of it, but those of kmvertex_kamui_lib.c, kmglobal_kamui_lib.c,
 * bgplane_kamui_lib.c and kmvtxlst_kamui_lib.c, which make pc keeps, since they build the
 * polygon headers' words and set the renderer's state as the console's
 * Kamui does. their state, kmiDeviceExtension, the current list's, and the
 * video registers kmiWriteRegister writes, lives here.
 *
 * each of the five lists, by ListType, is a buffer of LIST_SIZE(k) bytes in
 * the port's memory, the opaque list too, which the console's 2V latency
 * sends straight to the tile accelerator: Ninja writes the lists through the
 * store queues, which the host maps on that memory. kmRender hands the
 * lists to the renderer, then starts them again, and reports the render's
 * end and the vertices' as the console's interrupts do.
 *
 * video memory is PORT_VRAM, 8 MB of the port's memory, by its 64-bit
 * view's offsets, which the texture control words hold: kmtex_kamui_lib.c
 * and fballoc_kamui_lib.c, which make pc keeps, allocate textures in it and
 * load them, through kmiCopyToFrameBuffer here. a write raises
 * the generation of each 64 KB page it touches, and a palette's write the
 * palettes', which xbox/port/render.c's texture cache checks.
 *
 * most textures never reach it. kmLoadTexture asks port_texture_load first:
 * a texture the host's pack holds, by the key of its bytes (texhash.h), is
 * noted by its place, and its bytes aren't copied; the renderer asks
 * port_texture_at before it reads video memory. so the host backs video
 * memory only where something is written (port_vram_commit, 64 KB at a
 * time), and the Xbox keeps the rest for its textures. the 32-bit view's
 * writes, the frame buffers' clears and the tile accelerator's region
 * arrays, which nothing of the port reads, go nowhere */
#include <kamui.h>
#include <kmistat.h>
#include <sg_xpt.h>

#include "xbox/port/host.h"
#include "xbox/port/port.h"
#include "xbox/port/texhash.h"

Uint32 syTmrGetCount(void); /* shinobi.h's, xbox/port/system.c's */

#define LISTS 5
static const Uint32 list_sizes[LISTS] = PORT_LIST_SIZES;
static Uint32 list_offset(int k)
{
    Uint32 offset = 0;
    int j;

    for (j = 0; j < k; j++) {
        offset += list_sizes[j];
    }
    return offset;
}
#define LIST_SIZE(k) (list_sizes[k])
#define LIST_BASE(k) ((PKMDWORD)(PORT_LISTS + list_offset(k)))
_Static_assert(0x00040000u + 0x00004000u + 0x00030000u + 0x00004000u + 0x00008000u == PORT_LISTS_SIZE,
               "PORT_LISTS_SIZE is the lists' sizes' sum");

volatile struct kmi_device_extension kmiDeviceExtension;
extern struct kmi_global_parameter kmiGlobalWork;
KMCURRENTLISTSTATE kmiCurrentListState;
KMDWORD kmiCurrentListType;
PKMDWORD *kmiCurrentVertexBuff;
PKMDWORD *kmiDstVertexBuffer;
PKMDWORD kmi_write_pointers[LISTS];
PKMDWORD kmi_list_bases[2][LISTS];
PKMDWORD *kmi_banks[2] = {kmi_list_bases[0], kmi_list_bases[1]};
PKMDWORD kmi_list_starts[LISTS];

/* the PowerVR's registers by their offsets, which kmiWriteRegister keeps
 * for the renderer: the fog table, its colors, the border's, the palettes */
#define REGISTERS 0x2000
static KMDWORD registers[REGISTERS / 4];

static PKMVERTEXBUFFDESC buffer_desc;
static PKMCALLBACKFUNC eor_callback;
static void *eor_argument;
static PKMCALLBACKFUNC eov_callback;
static void *eov_argument;
static PKMCALLBACKFUNC vsync_callback;
static void *vsync_argument;

volatile struct kmi_device_extension *volatile kmiHardwareResource = &kmiDeviceExtension;

Uint32 port_vram_pages[PORT_VRAM_SIZE / PORT_VRAM_PAGE];
Uint32 port_palette_generation = 1;
static Uint32 write_generation = 1;

/* a register's value, as the renderer reads it */
KMDWORD port_register(KMDWORD reg)
{
    return kmiReadRegister(reg);
}

/* video memory's 64 KB pages the host has backed */
static Uint8 vram_backed[PORT_VRAM_SIZE / PORT_VRAM_PAGE];

int port_vram_commit(unsigned offset, unsigned size)
{
    unsigned page;
    int all = 1;

    if (size == 0) {
        return 1;
    }
    for (page = offset / PORT_VRAM_PAGE; page <= (offset + size - 1) / PORT_VRAM_PAGE; page++) {
        if (page >= PORT_VRAM_SIZE / PORT_VRAM_PAGE) {
            break;
        }
        if (!vram_backed[page]) {
            if (HOST->vram_commit(page * PORT_VRAM_PAGE, PORT_VRAM_PAGE)) {
                vram_backed[page] = 1;
            } else {
                static int logged;

                if (!logged) {
                    logged = 1;
                    port_log("kamui: no memory for video memory: a texture's bytes dropped");
                }
                all = 0;
            }
        }
    }
    return all;
}

/* the textures the pack holds, by their place in video memory: a hash
 * table on the place, whose freed entries keep the probes going */
#define PLACES 4096
#define PLACE_FREE 0xffffffffu
#define PLACE_GONE 0xfffffffeu
static struct place {
    Uint32 base; /* or PLACE_FREE, or PLACE_GONE */
    Uint32 bytes;
    Uint32 serial;
    Uint32 kind; /* format, vq, raster and the sizes, as texhash_start takes them */
    struct texhash key;
} places[PLACES];
static int places_ready;
static Uint32 place_serial;

static Uint32 place_kind(unsigned format, int vq, int raster, unsigned width, unsigned height)
{
    return format | (vq ? 8u : 0) | (raster ? 16u : 0) | width << 8 | height << 20;
}

static void places_start(void)
{
    int k;

    if (!places_ready) {
        for (k = 0; k < PLACES; k++) {
            places[k].base = PLACE_FREE;
        }
        places_ready = 1;
    }
}

/* the entries a write of size bytes at offset overlaps go: the texture
 * there is another now. but one the write's last tail bytes alone reach
 * stays: kmtex's load of a mipmapped 16-bit texture writes 4 bytes past
 * its surface (its @bug memory), into the next texture's first bytes,
 * which are before that one's smallest level where it is mipmapped too */
static void places_drop(Uint32 offset, Uint32 size, Uint32 tail)
{
    int k;

    if (!places_ready) {
        return;
    }
    for (k = 0; k < PLACES; k++) {
        struct place *p = &places[k];

        if (p->base < PLACE_GONE && p->base < offset + size && offset < p->base + p->bytes &&
            !(size >= tail && p->base >= offset + size - tail)) {
            p->base = PLACE_GONE;
        }
    }
}

int port_texture_at(unsigned base, unsigned format, int vq, int raster, unsigned width, unsigned height,
                    struct texhash *key, unsigned *serial)
{
    Uint32 slot = (base >> 5) % PLACES;
    int k;

    if (!places_ready) {
        return 0;
    }
    for (k = 0; k < PLACES; k++, slot = (slot + 1) % PLACES) {
        const struct place *p = &places[slot];

        if (p->base == PLACE_FREE) {
            return 0;
        }
        if (p->base == base) {
            if (p->kind != place_kind(format, vq, raster, width, height)) {
                return 0;
            }
            *key = p->key;
            *serial = p->serial;
            return 1;
        }
    }
    return 0;
}

/* the entry into the table, at its place's first free slot: 0 where the
 * table is full */
static int place_insert(const struct place *placed)
{
    Uint32 slot = (placed->base >> 5) % PLACES;
    int k;

    for (k = 0; k < PLACES; k++, slot = (slot + 1) % PLACES) {
        if (places[slot].base >= PLACE_GONE) {
            places[slot] = *placed;
            return 1;
        }
    }
    return 0;
}

static void mark_vram(KMDWORD offset, KMDWORD size);

/* fballoc's fb_move, as it compacts video memory: size bytes from src to
 * dst, which may overlap. a texture the pack holds has no bytes there, and
 * its entry moves; anything else moves as bytes */
int port_vram_move(KMDWORD dst, KMDWORD src, KMDWORD size)
{
    struct place moved[16];
    int count = 0;
    int whole = 0;
    int k;

    dst &= PORT_VRAM_SIZE - 1;
    src &= PORT_VRAM_SIZE - 1;
    if (size == 0 || dst == src || src + size > PORT_VRAM_SIZE || dst + size > PORT_VRAM_SIZE) {
        return 1;
    }
    for (k = 0; places_ready && k < PLACES; k++) {
        struct place *p = &places[k];

        if (p->base < PLACE_GONE && p->base >= src && p->base < src + size) {
            if (count < 16) {
                moved[count++] = *p;
            }
            whole |= p->base == src;
            p->base = PLACE_GONE;
        }
    }
    if (count == 1 && whole) {
        /* the block is that texture: no bytes to move, and what the table
         * held at its new place is gone */
        places_drop(dst, size, 0);
    } else {
        Uint8 *vram = (Uint8 *)PORT_VRAM;

        if (port_vram_commit(src, size) && port_vram_commit(dst, size)) {
            __builtin_memmove(vram + dst, vram + src, size);
        }
        mark_vram(dst, size);
    }
    for (k = 0; k < count; k++) {
        moved[k].base = moved[k].base - src + dst;
        place_insert(&moved[k]);
    }
    return 1;
}

/* kmLoadTexture's first step: whether the host's pack holds the texture,
 * by its bytes at texture, as the file has them, which are video memory's
 * but that a mipmapped 16-bit one's start four bytes on there. it is then
 * noted at its place, and kmLoadTexture copies nothing */
int port_texture_load(const KMSURFACEDESC *surface, const void *texture)
{
    static const Uint32 mip_offsets[11] = {0x00006, 0x00008, 0x00010, 0x00030, 0x000b0, 0x002b0,
                                           0x00ab0, 0x02ab0, 0x0aab0, 0x2aab0, 0xaaab0};
    static const Uint32 vq_mip_offsets[11] = {0x00000, 0x00001, 0x00002, 0x00006, 0x00016, 0x00056,
                                              0x00156, 0x00556, 0x01556, 0x05556, 0x15556};
    const Uint8 *bytes = texture;
    Uint32 flags = surface->fSurfaceFlags;
    Uint32 format = surface->PixelFormat >> 27;
    Uint32 width = surface->u0.USize;
    Uint32 height = surface->u1.VSize;
    Uint32 base = (Uint32)surface->pSurface & (PORT_VRAM_SIZE - 1);
    /* a small VQ texture: its codebook, shorter by its size, goes to the
     * surface's start and its indices 2048 bytes in, as a VQ texture's
     * (kmtex's load_small_vq); the file has the indices right after it */
    static const Uint32 small_codebook[7] = {0, 0, 0, 128, 128, 256, 1024};
    static const Uint32 small_codebook_mipmap[7] = {0, 0, 0, 128, 128, 512, 2048};
    int small = (surface->SurfaceType & 0xffff) == KM_SURFACETYPE_SMALLVQ_TEXTURE;
    int vq = small || (flags & KM_SURFACEFLAGS_VQ) != 0;
    int mip = (flags & KM_SURFACEFLAGS_MIPMAPED) != 0;
    int raster = !vq && !(flags & KM_SURFACEFLAGS_TWIDDLED);
    Uint32 level = 0;
    Uint32 codebook = 2048;
    Uint32 start;
    Uint32 top;
    Uint32 span;
    struct texhash key;

    if (((surface->SurfaceType & 0xffff) != KM_SURFACETYPE_TEXTURE && !small) || format > 2 ||
        (flags & (KM_SURFACEFLAGS_STRIDE | KM_SURFACEFLAGS_PALETTIZED)) || width < 8 || height < 8 || width > 1024 ||
        height > 1024 || (width & (width - 1)) || (height & (height - 1)) || (mip && (width != height || raster))) {
        port_counts.copied_loads++;
        return 0;
    }
    while ((1u << level) < width) {
        level++;
    }
    if (small) {
        if (width != height || level > 6 || small_codebook[level] == 0) {
            port_counts.copied_loads++;
            return 0;
        }
        codebook = mip ? small_codebook_mipmap[level] : small_codebook[level];
    }
    start = vq ? codebook + (mip ? vq_mip_offsets[level] : 0) : mip ? mip_offsets[level] - 4 : 0;
    top = vq ? width * height / 4 : width * height * 2;
    if (__builtin_memcmp(bytes, TEXSTUB_MAGIC, 12) == 0 || __builtin_memcmp(bytes, TEXSTUB_MAGIC + 4, 8) == 0) {
        /* the Xbox disc's file has the key in the texels' place */
        const Uint8 *at = bytes + (__builtin_memcmp(bytes, TEXSTUB_MAGIC, 12) == 0 ? 12 : 8);

        __builtin_memcpy(&key.lo, at, 4);
        __builtin_memcpy(&key.hi, at + 4, 4);
        if (!HOST->video_texture_known(key.lo, key.hi)) {
            /* nothing to copy either: the texture stays as video memory has it */
            port_log("kamui: a texture's key isn't in the pack");
            return 0;
        }
    } else {
        texhash_start(&key, format, vq, raster || small, width, height);
        if (vq) {
            texhash_bytes(&key, bytes, codebook);
        }
        texhash_bytes(&key, bytes + start, top);
        if (!HOST->video_texture_known(key.lo, key.hi)) {
            port_counts.copied_loads++;
            return 0;
        }
    }
    places_start();
    /* its bytes in video memory, by its own sizes: the levels to the
     * largest one's end */
    span = (vq ? 2048 + (mip ? vq_mip_offsets[level] : 0) : mip ? mip_offsets[level] : 0) + top;
    if (small) {
        /* a small VQ texture's bytes are two runs, its short codebook and,
         * 2048 bytes on, its indices: Kamui's allocator for them
         * (kmiVQalloc) packs other small ones' codebooks between the two,
         * and a place that spanned both would drop those neighbors', which
         * then drew from video memory's zeros, black. its place is the
         * codebook's run alone, where any load over it starts */
        places_drop(base + 2048, span - 2048, 0);
        span = codebook;
    }
    places_drop(base, span, 0);
    {
        struct place placed;

        placed.base = base;
        placed.bytes = span;
        placed.serial = ++place_serial;
        placed.kind = place_kind(format, vq, raster, width, height);
        placed.key = key;
        if (!place_insert(&placed)) {
            port_counts.copied_loads++;
            return 0;
        }
    }
    port_counts.packed_loads++;
    return 1;
}

static void mark_vram(KMDWORD offset, KMDWORD size)
{
    KMDWORD page;

    places_drop(offset, size, 4);
    write_generation++;
    for (page = offset / PORT_VRAM_PAGE; page <= (offset + size - 1) / PORT_VRAM_PAGE && page < PORT_VRAM_SIZE / PORT_VRAM_PAGE;
         page++) {
        port_vram_pages[page] = write_generation;
    }
}

/* size bytes from src to video memory at offset in the 64-bit view, which
 * the port keeps as one array */
KMINT32 kmiCopyToFrameBuffer(KMDWORD offset, KMDWORD *src, KMDWORD size)
{
    Uint8 *to = (Uint8 *)PORT_VRAM + (offset & (PORT_VRAM_SIZE - 1));
    const Uint8 *from = (const Uint8 *)src;
    KMDWORD k;

    size &= ~3u;
    if ((offset & (PORT_VRAM_SIZE - 1)) + size > PORT_VRAM_SIZE) {
        size = PORT_VRAM_SIZE - (offset & (PORT_VRAM_SIZE - 1));
    }
    if (!port_vram_commit(offset & (PORT_VRAM_SIZE - 1), size)) {
        return 0;
    }
    for (k = 0; k < size; k++) {
        to[k] = from[k];
    }
    if (size != 0) {
        mark_vram(offset & (PORT_VRAM_SIZE - 1), size);
    }
    return 0;
}

KMINT32 kmiCopyFromFrameBuffer(KMDWORD *dst, KMDWORD offset, KMDWORD size)
{
    const Uint8 *from = (const Uint8 *)PORT_VRAM + (offset & (PORT_VRAM_SIZE - 1));
    Uint8 *to = (Uint8 *)dst;
    KMDWORD k;

    if ((offset & (PORT_VRAM_SIZE - 1)) + size > PORT_VRAM_SIZE) {
        size = PORT_VRAM_SIZE - (offset & (PORT_VRAM_SIZE - 1));
    }
    if (!port_vram_commit(offset & (PORT_VRAM_SIZE - 1), size)) {
        for (k = 0; k < (size & ~3u); k++) {
            to[k] = 0;
        }
        return 0;
    }
    for (k = 0; k < (size & ~3u); k++) {
        to[k] = from[k];
    }
    return 0;
}

/* the 32-bit view, by which Kamui clears the frame buffers and writes the
 * tile accelerator's region arrays: the port has neither in video memory,
 * and its textures are in the 64-bit view, so a write goes nowhere and a
 * read gives zeros. (the writes were kept once, and each marked all of
 * video memory as changed) */
KMINT32 kmiCopyToPVRMemory(void *dst, void *src, KMDWORD size)
{
    (void)dst;
    (void)src;
    (void)size;
    return 0;
}

KMINT32 kmiCopyFromPVRMemory(void *dst, void *src, KMDWORD size)
{
    KMDWORD *to = dst;
    KMDWORD k;

    (void)src;
    for (k = 0; k < size / 4; k++) {
        to[k] = 0;
    }
    return 0;
}

void kmiPurgeCache(void *addr)
{
    (void)addr;
}

KMBOOLEAN kmiWaitForFinishDMA(KMDWORD handle)
{
    (void)handle;
    return TRUE;
}

/* a transfer to the tile accelerator: the YUV converter's, for the movie
 * player, which the port has none of yet */
KMSTATUS kmiDMAtoTARequest(KMDWORD target, const KMDWORD *source, KMDWORD length, KMDWORD flags, KMDWORD **handle)
{
    static KMDWORD done;

    (void)target;
    (void)source;
    (void)flags;
    if (length == 0) {
        return DMA_EMPTY;
    }
    port_unimplemented("kmiDMAtoTARequest");
    *handle = &done;
    return DMA_QUEUED;
}

void kmiDefaultInterruptHandler(KMINT32 event)
{
    (void)event;
    port_log("kamui: texture memory is full");
}

/* xbox/port/render.c's: draw the lists, each from its start to its end */
void port_render(PKMDWORD starts[LISTS], PKMDWORD ends[LISTS]);

KMINT32 kmiWriteRegister(KMDWORD reg, KMDWORD value)
{
    KMDWORD offset = reg & 0xffff;

    if (offset < REGISTERS) {
        if ((offset >= 0x1000 || offset == 0x108) && registers[offset / 4] != value) {
            port_palette_generation++;
        }
        registers[offset / 4] = value;
    }
    return 0;
}

KMDWORD kmiReadRegister(KMDWORD reg)
{
    return (reg & 0xffff) < REGISTERS ? registers[(reg & 0xffff) / 4] : 0;
}

/* Size bytes, a multiple of 4, from SrcAdr to dstPtr */
PKMDWORD kmiMemCopy8(PKMDWORD dstPtr, PKMDWORD SrcAdr, KMDWORD Size)
{
    KMDWORD k;

    for (k = 0; k < Size / 4; k++) {
        dstPtr[k] = SrcAdr[k];
    }
    return dstPtr + Size / 4;
}

/* every transfer has ended, as the port makes none */
KMBOOLEAN kmiQueryFinishDMA(KMDWORD handle)
{
    (void)handle;
    return TRUE;
}

void kmiDiscardRenderPipeline(void)
{
}

void kmiResetBoard(void)
{
}

/* each list starts with a header that opens it, its ListType in the
 * control word, as the console's lists do: kmSetUserClipping tests for a
 * list with nothing past it */
void port_runs_reset(void);

static void start_lists(void)
{
    int k;

    /* the model runs the lists named go with them (xbox/port/render.c) */
    port_runs_reset();
    for (k = 0; k < LISTS; k++) {
        PKMDWORD base = LIST_BASE(k);
        unsigned w;

        base[0] = 0x80000000u | ((KMDWORD)k << 24);
        for (w = 1; w < LIST_HEADER_WORDS; w++) {
            base[w] = 0;
        }
        kmi_write_pointers[k] = base + LIST_HEADER_WORDS;
        kmi_list_starts[k] = base;
    }
}

KMDWORD kmGetCurrentVertexOffset(KMINT32 ListType)
{
    return kmiCurrentVertexBuff[ListType] - kmi_list_starts[ListType];
}

KMSTATUS kmInitDevice(KMVIDEOMODE nVideoMode)
{
    (void)nVideoMode;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetDisplayMode(KMDISPLAYMODE nDisplayMode, KMBPPMODE nBpp, KMBOOLEAN fDither, KMBOOLEAN fAntiAlias)
{
    (void)nDisplayMode;
    (void)nBpp;
    (void)fDither;
    (void)fAntiAlias;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetSystemConfiguration(PKMSYSTEMCONFIGSTRUCT pSystemConfigStruct)
{
    int k;

    for (k = 0; k < LISTS; k++) {
        kmi_list_bases[0][k] = kmi_list_bases[1][k] = LIST_BASE(k);
    }
    start_lists();
    kmiFBinit();
    kmiCurrentVertexBuff = kmi_write_pointers;
    buffer_desc = pSystemConfigStruct->pBufferDesc;
    if (buffer_desc != NULL) {
        buffer_desc->pCurrentPtr = kmi_write_pointers;
        buffer_desc->pGlobalParam = (PKMDWORD)&kmiGlobalWork;
        buffer_desc->pCurrentListState = &kmiCurrentListState;
        buffer_desc->ppBuffer = kmi_banks;
        buffer_desc->fActiveList = (1 << LISTS) - 1;
    }
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmDiscardVertexBuffer(PKMVERTEXBUFFDESC pBufferDesc)
{
    (void)pBufferDesc;
    return KMSTATUS_SUCCESS;
}

void kmResetPointer(void)
{
    start_lists();
}

KMSTATUS kmRender(void)
{
    PKMDWORD ends[LISTS];
    int k;

    /* the port's figures, printed into the translucent list before the
     * lists' ends are taken */
    if (port_overlay) {
        port_overlay_draw();
    }
    for (k = 0; k < LISTS; k++) {
        static int overflowed;

        ends[k] = kmi_write_pointers[k];
        /* a list past its room has run into the next: its PORT_LIST_SIZES entry needs
         * raising */
        if ((Uint8 *)ends[k] > (Uint8 *)LIST_BASE(k) + LIST_SIZE(k) && !(overflowed & (1 << k))) {
            overflowed |= 1 << k;
            port_log("kamui: a list outgrew its size in PORT_LIST_SIZES");
        }
    }
    /* the render's length, as the console's Kamui gives its transfer's to
     * the tile accelerator (kmiIntStatisticData's ta_start and ta_time), on
     * syTmrGetCount's clock: src/system/present.c's split takes ta_time as a
     * picture's intake, which each half of a split frame must leave room
     * for. here the render is the CPU's, the lists read and the draws handed
     * to the host, with the host's wait for its graphics chip: some 5 ms a
     * picture, which a split's pacing counted as none (kmiIntStatisticData
     * stayed 0), so that frames it judged to fit ran late, and the split
     * ended and engaged by turns. the PowerVR's render, render_time, has no
     * like here: the host's chip draws after, and its wait is in this one */
    {
        Uint32 started = syTmrGetCount();

        port_render(kmi_list_starts, ends);
        kmiIntStatisticData.ta_start = started;
        kmiIntStatisticData.ta_time = syTmrGetCount() - started;
    }
    start_lists();
    if (eov_callback != NULL) {
        eov_callback(eov_argument);
    }
    if (eor_callback != NULL) {
        eor_callback(eor_argument);
    }
    return KMSTATUS_SUCCESS;
}

/* the flip shows the frame at a vertical blank, as the console's Kamui
 * does: it waits for the next blank where none has come since the last
 * flip, so that a loop of njWaitVSync, as the movie player's, runs a frame
 * a blank. while it waits, it runs the wait-vsync callback */
extern volatile Uint32 port_blanks;

KMSTATUS kmFlipFrameBuffer(void)
{
    static Uint32 last_flip;

    if (port_blanks == last_flip) {
        HOST->wait_vblank();
    }
    last_flip = port_blanks;
    HOST->video_present();
    if (vsync_callback != NULL) {
        vsync_callback(vsync_argument);
    }
    return KMSTATUS_SUCCESS;
}

/* engine/dc/owner.h's call, which engine/dc/wide.c's sa_present_mode
 * makes once a frame. make dc's frame owner, owner.c, which the port leaves
 * out, shows each picture when rendered at 60 fps. the port's flip waits for
 * a blank where none has come since the last, at either rate, so a split
 * frame's two pictures take a blank each, as at 60 */
void owner_sixty(int on)
{
    (void)on;
}

KMSTATUS kmStopDisplayFrameBuffer(void)
{
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmAdjustDisplayCenter(long nXAdjust, long nYAdjust)
{
    (void)nXAdjust;
    (void)nYAdjust;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetEORCallback(PKMCALLBACKFUNC callback, void *argument)
{
    eor_callback = callback;
    eor_argument = argument;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetEndOfVertexCallback(PKMCALLBACKFUNC callback, void *argument)
{
    eov_callback = callback;
    eov_argument = argument;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetWaitVsyncCallback(PKMCALLBACKFUNC callback, void *argument)
{
    vsync_callback = callback;
    vsync_argument = argument;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmfSetFatalErrorCallback(PKMCALLBACKFUNC callback)
{
    (void)callback;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmfSetTimeOutCount(KMDWORD nVertexTimeOut, KMDWORD nRenderTimeOut)
{
    (void)nVertexTimeOut;
    (void)nRenderTimeOut;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmfGetInternalSystemMetrics(PKMISYSTEMMETRICS pSystemMetrics)
{
    char *bytes = (char *)pSystemMetrics;
    unsigned k;

    for (k = 0; k < sizeof *pSystemMetrics; k++) {
        bytes[k] = 0;
    }
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmRenderTexture(PKMSURFACEDESC pTextureSurface)
{
    (void)pTextureSurface;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmuConvertFBtoBMP(PKMDWORD pOutputData, PKMDWORD pInputData, KMINT32 nWidth, KMINT32 nHeight,
                           KMBPPMODE nBpp)
{
    (void)pOutputData;
    (void)pInputData;
    (void)nWidth;
    (void)nHeight;
    (void)nBpp;
    return KMSTATUS_SUCCESS;
}
