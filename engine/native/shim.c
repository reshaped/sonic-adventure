/* the native slice builds the game for a host. this shim gives it, in C,
 * Shinobi, Ninja and Kamui calls the game makes. it also gives some of
 * the game's assembly, beside what asm2c.py writes. each definition takes
 * the SDK's declaration in the SH-4's widths: int32_t for Sint32, uint32_t
 * for Uint32. a name no object defines links to a stub from stubs.py */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "native.h"
#include "play_math.h"

#define NJD_TEXATTR_TYPE_MEMORY 0x40000000
#define NJD_TEXATTR_GLOBALINDEX 0x00800000

/* the game's state the reports read */
extern int game_mode;
extern int16_t stage_current, act_current;
/* the renders and resets of Kamui's lists. frame_wait makes one a frame,
 * and present.c renders once more where a frame shows two pictures */
static unsigned long frames;

void native_stub(const char *name)
{
    static const char *seen[4096];
    static unsigned int count;

    for (unsigned int i = 0; i < count; i++) {
        if (seen[i] == name) {
            return;
        }
    }
    if (count < sizeof seen / sizeof seen[0]) {
        seen[count++] = name;
    }
    fprintf(stderr, "sa1: stub %s, at vsync %lu\n", name, native_vsyncs);
}

int get_character(void);

static void textures_report(FILE *stream);

void native_report_state(FILE *stream)
{
    fprintf(stream, "%lu frames, game_mode %d, stage %d act %d, character %d\n",
            frames, game_mode, stage_current, act_current, get_character());
    textures_report(stream);
}

int32_t kmRender(void)
{
    frames++;
    native_lists_over();
    return 0;
}

int32_t kmResetPointer(void)
{
    frames++;
    native_lists_over();
    return 0;
}

/* the domain is the host's 4 GB that hold the game's addresses. the PVR's
 * registers lie at their console address in it, where records.c reads a
 * picture's fog */
#define PVR_REGISTERS (NATIVE_DOMAIN + 0xa05f8000UL)

int32_t kmiWriteRegister(uint32_t reg, uint32_t value)
{
    *(volatile uint32_t *)(PVR_REGISTERS + reg) = value;
    return 0;
}

/* SHC's FSCA and FSRRA intrinsics, as play math gives them */
void _builtin_fsca(int32_t angle, float *sine, float *cosine)
{
    float out[2];

    sa_fsca((uint32_t)angle, out);
    *sine = out[0];
    *cosine = out[1];
}

float _builtin_fsrra(float n)
{
    return sa_fsrra(n);
}

/* the _atan2 mesh.src calls, which takes floats. a host's atan2 takes
 * doubles, so this calls the runtime's float body */
extern float _in_atan2s(float y, float x);

float native_atan2(float y, float x)
{
    return _in_atan2s(y, x);
}

/* Ninja's unit matrix, which njGlobal.c's assembly holds */
const float _unit_matrix_[16] = {1, 0, 0, 0, 0, 1, 0, 0,
                                 0, 0, 1, 0, 0, 0, 0, 1};

#define HEAP (NATIVE_DOMAIN + 0x50000000UL)
#define HEAP_SIZE 0x10000000UL

/* Shinobi's heap: HEAP_SIZE bytes of the domain, which the first call maps.
 * a block takes the next of them, to a multiple of 32 bytes, and syFree gives
 * none back. a size the bytes left can't hold gets null, as it does on the
 * console where no block fits. of what the slice compiles, Ninja's
 * njInitVertexBuffer and njExitSystem alone name the two calls, and the slice
 * calls neither (stubs.txt). src/collision/polygon.c names them for its
 * index's arena on the console's side of a guard, which marks.py's CONSOLE
 * leaves out of the slice */
void *syMalloc(uint32_t size)
{
    static char *next;
    uint64_t bytes = ((uint64_t)size + 31) & ~(uint64_t)31;
    void *block;

    if (!next) {
        native_map((void *)HEAP, HEAP_SIZE, "the heap");
        next = (char *)HEAP;
    }
    if (bytes > HEAP + HEAP_SIZE - (uintptr_t)next) {
        return NULL;
    }
    block = next;
    next += bytes;
    return block;
}

void syFree(void *block)
{
    (void)block;
}

/* a vsync in the count's units, as engine/dc/dreamcast.h gives the game's C:
 * 16.68 ms at Shinobi's 1.28 us a count */
#define VSYNC_COUNTS 13034

/* the count moves only at the slice's vsyncs, by a vsync's counts at each */
uint32_t syTmrGetCount(void)
{
    return (uint32_t)(native_vsyncs * VSYNC_COUNTS);
}

/* Shinobi's SYS_RTC_DATE. the clock counts seconds from the start of 1950 in
 * local time, as the console's does. every fourth year is a leap year */
struct rtc_date {
    uint16_t year;
    uint8_t month, day, hour, minute, second, dayofweek, ageofmoon;
};
#define DAY_SECONDS 86400U
/* a new moon, and the lunar month, as syRtcMakeAgeOfMoon takes them */
#define NEW_MOON 0x5cb417d1U
#define LUNAR_MONTH 0x26ee7fU

/* the host's local time, as the clock counts it */
static uint32_t host_count(void)
{
    time_t now = time(NULL);
    const struct tm *local = localtime(&now);
    uint32_t years = (uint32_t)local->tm_year - 50;
    uint32_t days = years * 365 + (years + 1) / 4 + (uint32_t)local->tm_yday;

    return ((days * 24 + (uint32_t)local->tm_hour) * 60 +
            (uint32_t)local->tm_min) *
               60 +
           (uint32_t)local->tm_sec;
}

/* *date takes the host's date, or the count a hook gives in its place. the
 * result is the server mode, 0, as syRtcInit leaves it. a hook hears each
 * count read */
int32_t syRtcGetDate(struct rtc_date *date)
{
    static const uint8_t months[12] = {31, 28, 31, 30, 31, 30,
                                       31, 31, 30, 31, 30, 31};
    uint32_t count, days, length;

    if (!native_hook_clock(&count)) {
        count = host_count();
    }
    native_hook_date(count);
    days = count / DAY_SECONDS;
    date->second = (uint8_t)(count % 60);
    date->minute = (uint8_t)(count / 60 % 60);
    date->hour = (uint8_t)(count / 3600 % 24);
    /* 1950's first day was a Sunday, day 0 */
    date->dayofweek = (uint8_t)(days % 7);
    for (date->year = 1950; days >= (length = date->year % 4 ? 365U : 366U);
         date->year++) {
        days -= length;
    }
    for (date->month = 1;
         days >= (length = months[date->month - 1] +
                           (date->month == 2 && date->year % 4 == 0));
         date->month++) {
        days -= length;
    }
    date->day = (uint8_t)(days + 1);
    date->ageofmoon =
        (uint8_t)((count >= NEW_MOON
                       ? (count - NEW_MOON) % LUNAR_MONTH
                       : LUNAR_MONTH - (NEW_MOON - count) % LUNAR_MONTH) /
                  DAY_SECONDS);
    return 0;
}

/* the console's sound setting, which its flash keeps. it holds
 * SYD_CFG_STEREO until the options set SYD_CFG_MONO. each call returns
 * SYD_CFG_OK */
static int32_t sound_mode;

int32_t syCfgInit(void *work)
{
    (void)work;
    return 0;
}

int32_t syCfgExit(void)
{
    return 0;
}

int32_t syCfgSetSoundMode(int32_t mode)
{
    sound_mode = mode;
    return 0;
}

int32_t syCfgGetSoundMode(int32_t *mode)
{
    *mode = sound_mode;
    return 0;
}

/* the pads: a controller in port A, whose record pads.c fills, and nothing
 * in the other ports. Shinobi numbers six sockets to a port, so PDD_PORT_A0
 * is 0 and PDD_PORT_B0 6. each socket answers its port's record, and a
 * number past the last port's answers the last's. a record lays out as
 * PDS_PERIPHERAL does on the host */
#define PDD_DEV_CONTROLLER 0x000f06fe
#define PORTS 4
#define PORT_NUMBERS 6

struct peripheral {
    uint32_t id, support, on, off, press, release;
    uint16_t r, l;
    int16_t x1, y1, x2, y2;
    char *name;
    void *extend;
    uint32_t old;
    void *info;
};
static struct peripheral pads[PORTS] = {{.support = PDD_DEV_CONTROLLER}};

const struct peripheral *njGetPeripheral(uint32_t port)
{
    native_pad_record(&pads[0]);
    return &pads[port / PORT_NUMBERS < PORTS ? port / PORT_NUMBERS : PORTS - 1];
}

/* textures. a load gives each texture a record of texture memory with no
 * pixels. a record holds the texture's global index, and the size and pixel
 * format that screen.c's quads read. NJS_TEXNAME keeps a record's address in
 * 32 bits. so the records are static data of the executable, which links
 * into the domain.
 *
 * a record is held until its last user releases it, as Ninja's is. each
 * load of a global index adds a user to that index's one record. so a list
 * two tasks load stays whole when one of them releases it. a texture of
 * index 0 takes a record of its own at each load. the adapter (engine/sa1)
 * names a texture by its record. so it hears of a release only where the
 * record ends.
 *
 * a record stands for its first load's texels, as Ninja's keeps them when
 * another file loads its index. so it keeps that file's name id for
 * native_texture_file.
 *
 * record 0 is no texture's and stays zeros. a load takes it where no record
 * is free. a search gives it for an index no record holds. the game so
 * reads zeros there, never another texture's record */
#define PVM_NAME_SIZE 28
#define TEXTURE_RECORDS 16384
/* a chain averages four records where every record is held */
#define TEXTURE_CHAINS 4096

struct texname {
    void *filename;
    uint32_t attr;
    uint32_t texaddr;
};
struct texlist {
    struct texname *textures;
    uint32_t count;
};
/* NJS_TEXMEMLIST in the host's layout. the shim sets only globalIndex and
 * the surface's PixelFormat, nWidth and nHeight */
static struct {
    uint32_t index, words[4];
    void *pixels;
    uint32_t type, depth, format, width, height, rest[2];
    uint32_t *pointers[3];
    uint16_t count, dummy;
} texmem[TEXTURE_RECORDS];
/* each record's users, 0 where it is free. next is the record after it in
 * its chain, or among the freed where it is free. a next of 0 ends either.
 * file is the name id (native_disc_file_id) of the file its size came from,
 * or 0 */
static struct {
    uint32_t users;
    uint16_t next;
    uint32_t file;
} texmem_owners[TEXTURE_RECORDS];
_Static_assert(TEXTURE_RECORDS - 1 <= UINT16_MAX,
               "a record's number fits its owner's link");
/* the held records, each in the chain its index picks, the newest first */
static uint16_t texmem_chains[TEXTURE_CHAINS];
/* the free records: those past texmem_fresh, which no load has taken, and
 * the freed, oldest first. a load takes a fresh record first, then the
 * oldest freed. a name kept past its release so reads zeros until every
 * record free at that release is taken */
static uint32_t texmem_fresh;
static uint16_t texmem_freed, texmem_freed_last;
/* the report's counts. texmem_late counts releases that found their record
 * free, as a name's second release or one after njReleaseTextureAll. such a
 * release ends nothing. once a load takes the record again, the same release
 * ends a user of that load's texture */
static uint32_t texmem_held, texmem_most;
static unsigned long texmem_loads, texmem_late;
extern void *_nj_texmemlist;

static void textures_report(FILE *stream)
{
    fprintf(stream,
            "sa1: texture memory: at most %u records held of %u, %u now, %lu "
            "loads in all, %lu releases of a free record\n",
            texmem_most, TEXTURE_RECORDS - 1, texmem_held, texmem_loads,
            texmem_late);
}

/* the held record of index, the newest where index is 0, or 0 for none */
static uint32_t texture_find(uint32_t index)
{
    uint32_t slot = texmem_chains[index % TEXTURE_CHAINS];

    while (slot != 0 && texmem[slot].index != index) {
        slot = texmem_owners[slot].next;
    }
    return slot;
}

/* the number of the record a name's texaddr addresses, or 0 where it
 * addresses none of them */
static uint32_t texture_slot(uint32_t texaddr)
{
    uint32_t offset = texaddr - (uint32_t)(uintptr_t)texmem;

    return offset % sizeof texmem[0] == 0 &&
                   offset / sizeof texmem[0] < TEXTURE_RECORDS
               ? (uint32_t)(offset / sizeof texmem[0])
               : 0;
}

/* a free record's number, or 0 where none is free, which the run says once */
static uint32_t texture_take(void)
{
    static int said;
    uint32_t slot = 0;

    if (texmem_fresh < TEXTURE_RECORDS - 1) {
        slot = ++texmem_fresh;
    } else if (texmem_freed != 0) {
        slot = texmem_freed;
        texmem_freed = texmem_owners[slot].next;
    } else if (!said) {
        fprintf(stderr, "sa1: none of texture memory's %u records is free\n",
                TEXTURE_RECORDS - 1);
        said = 1;
    }
    return slot;
}

/* a texture file is chunks. each is a tag of 4 bytes, the size of what
 * follows the size, and that much. a GBIX chunk holds a global index, in
 * GBIX_BYTES in all. a PVRT chunk holds a texture. its first PVRT_HEAD bytes
 * hold its pixel format's byte, and its width and height as 16-bit words */
#define CHUNK_SIZE 4
#define CHUNK_BODY 8
#define GBIX_BYTES 16
#define PVRT_FORMAT 8
#define PVRT_WIDTH 12
#define PVRT_HEIGHT 14
#define PVRT_HEAD 16

static uint32_t read16(const uint8_t *at)
{
    uint16_t word;

    memcpy(&word, at, 2);
    return word;
}

static uint32_t read32(const uint8_t *at)
{
    uint32_t word;

    memcpy(&word, at, 4);
    return word;
}

/* load a texture of index, whose PVRT chunk is pvrt, or null where none was
 * read. Kamui keeps the chunk's pixel format in bits 27 to 29. file is the
 * chunk's file's name id, or 0 for a texture made in memory. the load adds a
 * user to the record that holds index, or takes a free one. a record keeps
 * the first size and file a load gives it. where no record is free the load
 * gets record 0 and is no user of it. the result is the record's address in
 * 32 bits */
static uint32_t texture_record(uint32_t index, const uint8_t *pvrt,
                               uint32_t file)
{
    uint32_t slot = index ? texture_find(index) : 0;

    _nj_texmemlist = texmem;
    if (slot == 0) {
        uint16_t *chain = &texmem_chains[index % TEXTURE_CHAINS];

        if ((slot = texture_take()) == 0) {
            return (uint32_t)(uintptr_t)&texmem[0];
        }
        texmem[slot].index = index;
        texmem_owners[slot].next = *chain;
        *chain = (uint16_t)slot;
        if (++texmem_held > texmem_most) {
            texmem_most = texmem_held;
        }
    }
    if (pvrt && texmem[slot].width == 0) {
        texmem[slot].format = (uint32_t)pvrt[PVRT_FORMAT] << 27;
        texmem[slot].width = read16(pvrt + PVRT_WIDTH);
        texmem[slot].height = read16(pvrt + PVRT_HEIGHT);
        texmem_owners[slot].file = file;
    }
    texmem_owners[slot].users++;
    texmem_loads++;
    return (uint32_t)(uintptr_t)&texmem[slot];
}

/* end one user of the held record at slot, or every user where all is set.
 * the last user's end frees the record. the adapter hears first, while the
 * record still holds its index. the record then leaves its chain, turns to
 * zeros and goes behind every other freed record */
static void texture_release(uint32_t slot, int all)
{
    uint16_t *link = &texmem_chains[texmem[slot].index % TEXTURE_CHAINS];
    struct texname name = {NULL, 0, (uint32_t)(uintptr_t)&texmem[slot]};
    struct texlist ended = {&name, 1};

    if (!all && --texmem_owners[slot].users != 0) {
        return;
    }
    sa1_textures_released(&ended);
    while (*link != slot) {
        link = &texmem_owners[*link].next;
    }
    *link = texmem_owners[slot].next;
    memset(&texmem[slot], 0, sizeof texmem[slot]);
    texmem_owners[slot].users = 0;
    texmem_owners[slot].next = 0;
    texmem_owners[slot].file = 0;
    if (texmem_freed != 0) {
        texmem_owners[texmem_freed_last].next = (uint16_t)slot;
    } else {
        texmem_freed = (uint16_t)slot;
    }
    texmem_freed_last = (uint16_t)slot;
    texmem_held--;
}

uint32_t native_texture_file(uint32_t texaddr)
{
    return texmem_owners[texture_slot(texaddr)].file;
}

/* the held record of index, or record 0 where none holds it. Ninja's own
 * returns -1 there, which njSetkmTextureNumG takes unchecked as a record's
 * number in _nj_texmemlist. a search ahead of any load so sets the list too */
int32_t njSearchTexList(void *list, uint32_t size, uint32_t index)
{
    (void)list, (void)size;
    _nj_texmemlist = texmem;
    return (int32_t)texture_find(index);
}

/* the global index a GBIX chunk gives, or 0 for any other chunk */
static uint32_t chunk_index(const uint8_t *chunk)
{
    return !memcmp(chunk, "GBIX", 4) ? read32(chunk + CHUNK_BODY) : 0;
}

/* the chunk after chunk */
static const uint8_t *chunk_end(const uint8_t *chunk)
{
    return chunk + CHUNK_BODY + read32(chunk + CHUNK_SIZE);
}

/* load the texture file name, with .PVR added where name lacks it, as
 * njCheckFileName does. the record holds index, or where index is 0 the
 * file's GBIX index, if any. a file the disc lacks still takes index's
 * record. the file lies in the game's current directory */
static uint32_t file_record(const char *name, uint32_t index)
{
    char file[128];
    uint8_t head[GBIX_BYTES + PVRT_HEAD] = {0};
    const uint8_t *pvrt = NULL;
    size_t length = strlen(name);
    FILE *in;

    snprintf(file, sizeof file,
             length >= 4 && !strcasecmp(name + length - 4, ".PVR") ? "%s"
                                                                   : "%s.PVR",
             name);
    if (!(in = native_disc_file(file))) {
        return texture_record(index, NULL, 0);
    }
    if (fread(head, 1, sizeof head, in) != sizeof head) {
        head[0] = 0;
    }
    fclose(in);
    /* a GBIX chunk, or none, comes first */
    if (!memcmp(head, "PVRT", 4)) {
        pvrt = head;
    } else if (!memcmp(head + GBIX_BYTES, "PVRT", 4)) {
        pvrt = head + GBIX_BYTES;
    }
    return texture_record(index ? index : chunk_index(head), pvrt,
                          native_disc_file_id(file));
}

/* the hook hears of the release first, while each record holds its index.
 * a name that addresses no held record releases nothing. record 0 has no
 * user, and a free record has none left */
int32_t njReleaseTexture(void *texlist)
{
    const struct texlist *list = texlist;

    native_hook_textures_released(texlist);
    for (uint32_t i = 0; i < list->count; i++) {
        uint32_t slot = texture_slot(list->textures[i].texaddr);

        if (texmem_owners[slot].users != 0) {
            texture_release(slot, 0);
        } else if (slot != 0) {
            texmem_late++;
        }
    }
    return 0;
}

/* every held record goes, whatever its users */
void njReleaseTextureAll(void)
{
    for (uint32_t slot = 1; slot <= texmem_fresh; slot++) {
        if (texmem_owners[slot].users != 0) {
            texture_release(slot, 1);
        }
    }
}

/* the head of NJS_TEXINFO in the host's layout. njSetTextureInfo puts its
 * format in Type's high half, whose low byte is a PVRT header's byte 8 */
struct texinfo {
    void *pixels;
    uint32_t type, depth, format, width, height;
};

/* a name under NJD_TEXATTR_GLOBALINDEX holds its global index in texaddr,
 * as Ninja's loaders read it. one under NJD_TEXATTR_TYPE_MEMORY names its
 * pixels' NJS_TEXINFO in place of a file. the result is 1, or -1 where a
 * name found no record free */
int32_t njLoadTexture(struct texlist *texlist)
{
    int32_t result = 1;

    for (uint32_t i = 0; i < texlist->count; i++) {
        struct texname *texture = &texlist->textures[i];
        uint32_t index =
            texture->attr & NJD_TEXATTR_GLOBALINDEX ? texture->texaddr : 0;

        if (texture->attr & NJD_TEXATTR_TYPE_MEMORY) {
            const struct texinfo *info = texture->filename;
            /* the bytes of a PVRT's header that texture_record reads */
            uint8_t pvrt[PVRT_HEAD] = {'P', 'V', 'R', 'T'};

            pvrt[PVRT_FORMAT] = (uint8_t)(info->type >> 16);
            memcpy(pvrt + PVRT_WIDTH, &info->width, 2);
            memcpy(pvrt + PVRT_HEIGHT, &info->height, 2);
            texture->texaddr = texture_record(index, pvrt, 0);
        } else {
            texture->texaddr = file_record(texture->filename, index);
        }
        if (texture_slot(texture->texaddr) == 0) {
            result = -1;
        }
    }
    sa1_textures_loaded(texlist, texlist->count, NULL);
    return result;
}

/* where the last PRS decode wrote, and the name id of the last file opened,
 * which it decoded */
static const void *decoded_at;
static uint32_t decoded_file;

/* a PVM begins with a PVMH chunk, which holds its flags and its count of
 * entries, 16-bit words, and then the entries. the textures' chunks follow
 * it. the flags say what each entry holds after its first 2 bytes, in this
 * order. a name comes under PVM_NAME, and 2 bytes under each of PVM_CATEGORY
 * and PVM_INFO. a global index comes under PVM_INDEX. where an entry gives
 * none, a GBIX ahead of its texture's PVRT gives it. the game's loaders
 * decode a file to addr and load it from there. so the PVM's file is the
 * last decode's where that decode wrote at addr, and otherwise none.
 * the result is 1, or -1 for what isn't a PVM or where no record was free */
#define PVM_FLAGS 8
#define PVM_COUNT 10
#define PVM_ENTRIES 12
#define PVM_NAME 0x1
#define PVM_CATEGORY 0x2
#define PVM_INFO 0x4
#define PVM_INDEX 0x8

int32_t njLoadTexturePvmMemory(uint8_t *addr, struct texlist *texlist)
{
    uint32_t flags = read16(addr + PVM_FLAGS);
    uint32_t count = read16(addr + PVM_COUNT);
    const uint8_t *entry = addr + PVM_ENTRIES;
    const uint8_t *chunk = chunk_end(addr);
    uint32_t file = addr == decoded_at ? decoded_file : 0;
    /* the PVM's names in texlist's order, which the adapter takes in place
     * of the list's own */
    static const char *names[TEXTURE_RECORDS];
    uint32_t loaded = 0;
    int32_t result = 1;

    if (memcmp(addr, "PVMH", 4) != 0) {
        return -1;
    }
    for (uint32_t i = 0; i < count && i < texlist->count; i++) {
        uint32_t index = 0;

        if (i < TEXTURE_RECORDS) {
            names[i] = flags & PVM_NAME ? (const char *)entry + 2 : NULL;
        }
        entry += 2 + (flags & PVM_NAME ? PVM_NAME_SIZE : 0) +
                 (flags & PVM_CATEGORY ? 2 : 0) + (flags & PVM_INFO ? 2 : 0);
        if (flags & PVM_INDEX) {
            index = read32(entry);
            entry += 4;
        }
        /* a PVRT ends the texture's chunks */
        while (memcmp(chunk, "PVRT", 4) != 0) {
            index = index ? index : chunk_index(chunk);
            chunk = chunk_end(chunk);
        }
        texlist->textures[i].texaddr = texture_record(index, chunk, file);
        if (texture_slot(texlist->textures[i].texaddr) == 0) {
            result = -1;
        }
        chunk = chunk_end(chunk);
        loaded++;
    }
    sa1_textures_loaded(texlist, loaded,
                        loaded <= TEXTURE_RECORDS ? names : NULL);
    return result;
}

/* a texture list over size names of PVM_NAME_SIZE bytes each */
void njSetPvmTextureList(struct texlist *texlist, struct texname *texname,
                         char *filename, int32_t size)
{
    texlist->textures = texname;
    texlist->count = (uint32_t)size;
    for (int32_t i = 0; i < size; i++) {
        texname[i] = (struct texname){filename + i * PVM_NAME_SIZE, 0, 0};
    }
}

/* prs.src's decoder. the flag bits come a byte at a time, LSB first. a set
 * flag copies a byte. a clear flag then a set one read a 16-bit word. that
 * word copies from up to 8 KB back, or ends the stream where it is 0. two
 * clear flags copy 2 to 5 bytes from up to 256 back */
static unsigned int prs_bit(const uint8_t **from, unsigned int *bits,
                            unsigned int *count)
{
    unsigned int bit;

    if (*count == 0) {
        *bits = *(*from)++;
        *count = 8;
    }
    bit = *bits & 1;
    *bits >>= 1;
    (*count)--;
    return bit;
}

static int32_t prs_decode(const uint8_t *from, uint8_t *dest)
{
    uint8_t *to = dest;
    unsigned int bits = 0, count = 0;

    for (;;) {
        int32_t offset;
        uint32_t size;

        if (prs_bit(&from, &bits, &count)) {
            *to++ = *from++;
            continue;
        }
        if (prs_bit(&from, &bits, &count)) {
            uint32_t word = read16(from);

            from += 2;
            if (word == 0) {
                break;
            }
            offset = (int32_t)(word >> 3 | 0xffffe000U);
            size = word & 7;
            size = size ? size + 2 : (uint32_t)*from++ + 1;
        } else {
            size = prs_bit(&from, &bits, &count) << 1;
            size = (size | prs_bit(&from, &bits, &count)) + 2;
            offset = (int32_t)(*from++ | 0xffffff00U);
        }
        while (size--) {
            *to = to[offset];
            to++;
        }
    }
    return (int32_t)(to - dest);
}

/* ADVERTISE.PRS, the menus, and SUMMARY.PRS each begin with a table of
 * entries in SH-4 addresses. main.c reads it at MODULE_AREA. once one
 * decodes there, the shim writes over its table the slice's own,
 * advertise_entries or summary_entries */
#define MODULE_AREA 0x0c900000U
/* the bits an address shares with its mirrors: all but its top three, which
 * name the SH-4's region */
#define MIRROR_MASK 0x1fffffffU

struct advertise {
    void *start, *end, *step;
};
extern struct advertise advertise_entries __attribute__((weak));
struct summary {
    void *entries[9];
};
extern struct summary summary_entries __attribute__((weak));

/* an event's file begins with its entry and its texture list, which
 * src/event/play.c reads at EVENT_MODULE. once an event decodes there, the
 * shim writes over them the slice's own, as ev0001_head. Sky Chase's paths
 * load there too, and keep their bytes */
#define EVENT_MODULE 0x0cb80000U
#define EVENT_HEAD_SIZE (2 * sizeof(void *))

/* the four events the disc stores as another's bytes (tools/images.py's
 * EVENT_ALIASES). each runs the program of the event it repeats */
static const char *const event_aliases[][2] = {
    {"EV0054.PRS", "EV0029.PRS"},
    {"EV0111.PRS", "EV0101.PRS"},
    {"EV0177.PRS", "EV0176.PRS"},
    {"EV017B.PRS", "EV017A.PRS"},
};

/* the program a file's bytes are */
static const char *program_of(const char *file)
{
    for (size_t i = 0; i < sizeof event_aliases / sizeof event_aliases[0];
         i++) {
        if (!strcasecmp(file, event_aliases[i][0])) {
            return event_aliases[i][1];
        }
    }
    return file;
}

static void write_event_head(void *dest, const char *file)
{
    char name[32];
    const char *head;

    snprintf(name, sizeof name, "%.6s_head", file);
    for (char *at = name; *at; at++) {
        *at = (char)tolower((unsigned char)*at);
    }
    head = native_symbol(name);
    if (head) {
        memcpy(dest, head, EVENT_HEAD_SIZE);
    } else {
        fprintf(stderr, "sa1: no event image for %s\n", file);
    }
}

int32_t prs_decompress(void *source, void *dest)
{
    int32_t size = prs_decode(source, dest);
    const char *file = program_of(native_last_opened);

    decoded_at = dest;
    decoded_file = native_disc_file_id(native_last_opened);
    native_program_read(file);
    if (((uintptr_t)dest & MIRROR_MASK) == MODULE_AREA &&
        !strcasecmp(native_last_opened, "ADVERTISE.PRS") &&
        &advertise_entries) {
        memcpy(dest, &advertise_entries, sizeof advertise_entries);
    }
    if (((uintptr_t)dest & MIRROR_MASK) == MODULE_AREA &&
        !strcasecmp(native_last_opened, "SUMMARY.PRS") && &summary_entries) {
        memcpy(dest, &summary_entries, sizeof summary_entries);
    }
    if (((uintptr_t)dest & MIRROR_MASK) == EVENT_MODULE &&
        !strncasecmp(file, "EV", 2)) {
        write_event_head(dest, file);
    }
    return size;
}

/* the menus' trial.c calls these two of 1ST_READ.BIN's functions as the
 * menus_ names below, with no prototype. each argument so comes as an int */
void set_character_number(int16_t number);
void stage_set_stage_act(uint8_t stage, uint8_t act);

void menus_set_character_number(int number)
{
    set_character_number((int16_t)number);
}

void menus_set_stage_act(int stage, int act)
{
    stage_set_stage_act((uint8_t)stage, (uint8_t)act);
}

/* math.src's fill: count bytes at mem set to value's low byte */
void fill_bytes(void *mem, int value, int count)
{
    memset(mem, value, (uint32_t)count);
}
