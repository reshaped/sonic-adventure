/* the game's sound below its own queue. the adapter gives the calls the game
 * makes of Shinobi's sound library and of CRI's ADX streams. each goes to
 * native_sound_engine as the ABI's audio commands, banks and streams, or
 * nowhere while that is NULL. a stream's state and time, which the game
 * reads back, follow the vsyncs and not the device. so play takes the same
 * steps with sound or without */
#include <shinobi.h>
#include <sofdec.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native.h"
#include "sa1.h"
#include "sa_engine.h"

struct sa_engine *native_sound_engine;

/* the Dreamcast's vsync, in ns */
#define VSYNC_NS 16683350ULL
#define BANKS 128 /* the sound library's banks of each kind */
#define PORTS SDD_MIDI_PORT_MAX
#define BLOCKS 8 /* the memory blocks the adapter keeps at once */
#define FX_OUT_MAX 15 /* the sound library's last fx output */
/* an ADX file's header, as adx_dcd.c reads it, big-endian: its mark, and
 * where its fields lie. the data starts ADX_DTOFS_BASE past the offset at
 * ADX_DATA. ADX_LOOPS is the count of loops, ADX_LOOP_START the loop's first
 * sample and ADX_LOOP_END the sample it ends before. ADX_HEAD is the bytes of
 * a header's fields, which a stream's file holds at least. ADX_LOOP_HEAD is
 * the bytes of a header that holds a loop, at the least. the data is blocks
 * of ADX_BLOCK bytes, each one channel's next ADX_BLOCK_SAMPLES samples */
#define ADX_MARK 0x8000
#define ADX_DATA 2
#define ADX_DTOFS_BASE 4
#define ADX_CHANNELS 7
#define ADX_RATE 8
#define ADX_SAMPLES 12
#define ADX_LOOPS 0x16
#define ADX_LOOP_START 0x1c
#define ADX_LOOP_END 0x24
#define ADX_HEAD 0x2c
#define ADX_LOOP_HEAD 0x30
#define ADX_BLOCK 18
#define ADX_BLOCK_SAMPLES 32
/* the streams the adapter keeps at once. the game makes its music's and its
 * voice's, and movie.c makes one for a movie's sound */
#define STREAMS 4
#define AFS_FILES 4096
/* the places sdQsndSetPos sends */
#define QSOUND_PLACES 7
/* how late a file's stream is heard after its start: sofdec reads ahead
 * before it plays */
#define READ_AHEAD_MS 136

static void submit(const sa_audio_command *command)
{
    uint32_t game;

    if (!native_sound_engine) {
        return;
    }
    game = native_fpu_host();
    sa_audio_submit(native_sound_engine, command, 1);
    native_fpu_game(game);
}

static void command(uint16_t kind, uint32_t port, int32_t sound, int32_t value,
                    int32_t priority, int32_t updates)
{
    sa_audio_command c;

    memset(&c, 0, sizeof c);
    c.head.kind = SA_REC_AUDIO;
    c.head.size = sizeof c;
    c.kind = kind;
    c.sound = sound;
    c.key.space = SA_SPACE_VALUE;
    c.key.id = port;
    c.priority = priority;
    c.value = value;
    c.updates = updates;
    submit(&c);
}

/* the library's memory blocks and ports: a block is the data and size its
 * last sdMemBlkSetPrm gave */
struct block {
    const uint8_t *data;
    int32_t size;
    int used;
};
static struct block blocks[BLOCKS];
static SDHANDLE ports[PORTS];
static int ports_open;
/* where each bank lies in sound memory, as sdSndMemAllocateBank put it */
struct place {
    int32_t address, size;
};
static struct place program_banks[BANKS], sequence_banks[BANKS],
    other_banks[BANKS];

static uint32_t little32(const uint8_t *at);

static int port_of(SDMIDI handle)
{
    return handle ? (int)(handle - ports) : 0;
}

SDE_ERR sdLibInit(void *wrk_ptr, Sint32 mem_blk_handle_max,
                  Sint32 second_host_cmd_max)
{
    (void)wrk_ptr, (void)mem_blk_handle_max, (void)second_host_cmd_max;
    return SDE_ERR_NOTHING;
}

SDE_ERR sdMemBlkSetTransferMode(SDE_MEMBLK_TRANSFER_MODE transfer_mode)
{
    (void)transfer_mode;
    return SDE_ERR_NOTHING;
}

SDE_ERR sdMemBlkCreate(SDMEMBLK *handle)
{
    for (int i = 0; i < BLOCKS; i++) {
        if (!blocks[i].used) {
            blocks[i].used = 1;
            blocks[i].data = NULL;
            blocks[i].size = 0;
            *handle = (SDMEMBLK)&blocks[i];
            return SDE_ERR_NOTHING;
        }
    }
    return SDE_ERR_HANDLE_NO_ENOUGH;
}

SDE_ERR sdMemBlkDestroy(SDMEMBLK handle)
{
    if (handle) {
        ((struct block *)handle)->used = 0;
    }
    return SDE_ERR_NOTHING;
}

SDE_ERR sdMemBlkSetPrm(SDMEMBLK handle, const void *src_blk_ptr,
                       const Sint32 src_blk_sz,
                       const SD_MEMBLK_CALLBACK_FUNC callback_func,
                       const void *callback_first_arg)
{
    struct block *block = (struct block *)handle;

    (void)callback_func, (void)callback_first_arg;
    if (!block) {
        return SDE_ERR_HANDLE_NULL;
    }
    block->data = src_blk_ptr;
    block->size = src_blk_sz;
    return SDE_ERR_NOTHING;
}

/* the backend runs its own driver, but reads the driver file's tables in
 * sound memory. so the file's program goes to sound memory's start, as
 * Shinobi's sdDrvInit puts it. the program is the bytes after the file's
 * 0x20-byte header. the word at offset 8 counts them */
SDE_ERR sdDrvInit(SDMEMBLK handle)
{
    const struct block *block = (const struct block *)handle;
    uint32_t game;
    sa_audio_bank driver;

    if (!block || !block->data) {
        return SDE_ERR_HANDLE_NULL;
    }
    if (memcmp(block->data, "SDRV", 4) != 0) {
        return SDE_ERR_DATA_ILLEGAL_TYPE;
    }
    driver.kind = SA_BANK_OTHER;
    driver.number = 0;
    driver.address = 0;
    driver.bytes = little32(block->data + 8);
    driver.data = block->data + 0x20;
    if (native_sound_engine) {
        game = native_fpu_host();
        sa_audio_load(native_sound_engine, &driver);
        native_fpu_game(game);
    }
    return SDE_ERR_NOTHING;
}

SDE_ERR sdSysServer(void)
{
    return SDE_ERR_NOTHING;
}

SDE_ERR sdMidiOpenPort(SDMIDI *handle)
{
    if (ports_open >= PORTS) {
        return SDE_ERR_HANDLE_NO_ENOUGH;
    }
    *handle = &ports[ports_open++];
    return SDE_ERR_NOTHING;
}

/* the bytes as the game gives them: a status and its data. the SDK's
 * declaration puts a Uint8 before its ..., where C's va_start promises
 * nothing. clang's va_start finds the data bytes past it on each host the
 * native slice builds for */
#pragma clang diagnostic ignored "-Wvarargs"
SDE_ERR sdMidiSetMes(SDMIDI handle, SDS_MIDI_MES *midi_mes_ptr,
                     const Uint8 midi_mes, ...)
{
    va_list data;
    int status = midi_mes & 0xf0;

    (void)handle;
    memset(midi_mes_ptr, 0, sizeof *midi_mes_ptr);
    if (status < 0x80 || status == 0xf0) {
        return SDE_ERR_NOTHING;
    }
    va_start(data, midi_mes);
    midi_mes_ptr->m_Member[0] = midi_mes;
    midi_mes_ptr->m_Member[1] = (Uint8)va_arg(data, int);
    if (status != SDD_MIDI_PRG_CHG && status != SDD_MIDI_MONO_PRES) {
        midi_mes_ptr->m_Member[2] = (Uint8)va_arg(data, int);
    }
    va_end(data);
    return SDE_ERR_NOTHING;
}

SDE_ERR sdMidiSendMes(SDMIDI handle, const SDS_MIDI_MES *midi_mes_ptr,
                      const Sint8 priority)
{
    const Uint8 *m = midi_mes_ptr->m_Member;

    (void)priority;
    if (m[0] >= 0x80) {
        command(SA_AUDIO_MIDI, port_of(handle), 0,
                m[0] | (m[1] << 8) | (m[2] << 16), 0, 0);
    }
    return SDE_ERR_NOTHING;
}

SDE_ERR sdMidiPlay(SDMIDI handle, const Sint8 bank_num, const Sint8 data_num,
                   const Sint8 priority)
{
    command(SA_AUDIO_SEQUENCE, port_of(handle),
            ((bank_num & 0xff) << 8) | (data_num & 0xff), 0, priority, 0);
    return SDE_ERR_NOTHING;
}

SDE_ERR sdMidiStop(SDMIDI handle)
{
    command(SA_AUDIO_SEQUENCE_STOP, port_of(handle), 0, 0, 0, 0);
    return SDE_ERR_NOTHING;
}

SDE_ERR sdMidiSetVol(SDMIDI handle, const Sint8 vol, const Sint32 fade_time)
{
    command(SA_AUDIO_SEQUENCE_VOLUME, port_of(handle), 0, vol, 0, fade_time);
    return SDE_ERR_NOTHING;
}

/* the space, stereo or mono, as the console's setting gives it. the fx
 * program, its outputs and qsound's places go to the backend's DSP as the
 * driver takes them. a place goes as its byte, which carries the game's
 * QSOUND_CHANGED mark */
SDE_ERR sdSndSetSpace(const SDE_SPACE space)
{
    command(SA_AUDIO_SPACE, 0, 0, space == SDE_SPACE_MONO, 0, 0);
    return SDE_ERR_NOTHING;
}

SDE_ERR sdSndSetFxPrg(const Sint8 new_fx_prg_num, const Sint8 new_fx_out_num)
{
    if (new_fx_prg_num < 0 || new_fx_out_num < 0) {
        return SDE_ERR_PRM_OVER_RANGE;
    }
    command(SA_AUDIO_FX_PROGRAM, 0, new_fx_prg_num, new_fx_out_num, 0, 0);
    return SDE_ERR_NOTHING;
}

SDE_ERR sdSndSetFxOutPrm(const Sint8 out_num, const Sint8 out_pan,
                         const Sint8 out_lev)
{
    if (out_num < 0 || out_num > FX_OUT_MAX) {
        return SDE_ERR_PRM_OVER_RANGE;
    }
    command(SA_AUDIO_FX_OUTPUT, (uint32_t)out_num, out_pan, out_lev, 0, 0);
    return SDE_ERR_NOTHING;
}

SDE_ERR sdQsndSetPos(const Sint8 *pos)
{
    for (int i = 0; i < QSOUND_PLACES; i++) {
        command(SA_AUDIO_QSOUND, (uint32_t)i, 0, pos[i] & 0xff, 0, 0);
    }
    return SDE_ERR_NOTHING;
}

static struct place *places(SDE_DATA_TYPE type, uint32_t *kind)
{
    switch (type) {
    case SDE_DATA_TYPE_MIDI_PRG_BANK:
        *kind = SA_BANK_PROGRAMS;
        return program_banks;
    case SDE_DATA_TYPE_MIDI_SEQ_BANK:
        *kind = SA_BANK_SEQUENCES;
        return sequence_banks;
    default:
        *kind = SA_BANK_OTHER;
        return other_banks;
    }
}

SDE_ERR sdSndMemAllocateBank(const SDE_DATA_TYPE data_type, int bank_num,
                             Sint32 allocate_ptr, Sint32 allocate_size)
{
    uint32_t kind;
    struct place *place = places(data_type, &kind);

    if (bank_num < 0 || bank_num >= BANKS) {
        return SDE_ERR_PRM_OVER_RANGE;
    }
    place[bank_num].address = allocate_ptr;
    place[bank_num].size = allocate_size;
    return SDE_ERR_NOTHING;
}

SDE_ERR sdBankDownload(SDMEMBLK handle, const SDE_DATA_TYPE bank_type,
                       const Sint8 bank_num)
{
    const struct block *block = (const struct block *)handle;
    uint32_t game;
    sa_audio_bank bank;
    struct place *place = places(bank_type, &bank.kind);

    if (!block || bank_num < 0 || place[(int)bank_num].size <= 0) {
        return SDE_ERR_UNKNOW_NUM;
    }
    bank.number = (uint32_t)bank_num;
    bank.address = (uint32_t)place[(int)bank_num].address;
    bank.bytes = (uint32_t)block->size;
    bank.data = block->data;
    if (native_sound_engine) {
        game = native_fpu_host();
        sa_audio_load(native_sound_engine, &bank);
        native_fpu_game(game);
    }
    return SDE_ERR_NOTHING;
}

/* a multi unit's header and a bank's record, by sd_multi_unit_download.c's
 * names. their words are little-endian */
struct multi_unit {
    char type[4]; /* "SMLT" */
    int8_t version;
    int8_t unk_05[3];
    int32_t bank_num;
    int32_t unk_0c;
    int32_t unk_10;
    int32_t unk_14[3];
};
struct multi_unit_bank {
    int32_t type; /* an SDE_DATA_TYPE */
    int32_t num;
    int32_t address; /* an offset in sound memory */
    int32_t size; /* negative when the bank takes no allocation */
    int32_t data_offset; /* from the start of the unit */
    int32_t data_size;
    int32_t unk_18[2];
};
_Static_assert(sizeof(struct multi_unit) == 0x20 &&
                   sizeof(struct multi_unit_bank) == 0x20,
               "a multi unit's header and each bank's record take 0x20 bytes");

/* a file of banks, as sd_multi_unit_download.c reads one: its header, each
 * bank's record, and the banks' bytes. a bank takes its place unless its
 * size is negative, and its bytes where it has any. with no block free, no
 * bank takes its bytes, since the calls that follow refuse a null block */
SDE_ERR sdMultiUnitDownload(SDMEMBLK handle)
{
    const struct block *block = (const struct block *)handle;
    const uint8_t *unit;
    struct multi_unit head;
    SDMEMBLK each = NULL;

    if (!block || !(unit = block->data)) {
        return SDE_ERR_HANDLE_NULL;
    }
    if (memcmp(unit, "SMLT", sizeof head.type) != 0) {
        return SDE_ERR_DATA_ILLEGAL_TYPE;
    }
    memcpy(&head, unit, sizeof head);
    if (head.version < 1) {
        return SDE_ERR_DATA_ILLEGAL_TYPE;
    }
    sdMemBlkCreate(&each);
    for (int32_t i = 0; i < head.bank_num; i++) {
        struct multi_unit_bank bank;
        SDE_DATA_TYPE type;

        memcpy(&bank, unit + sizeof head + sizeof bank * (size_t)i,
               sizeof bank);
        type = (SDE_DATA_TYPE)bank.type;
        if (bank.size >= 0) {
            sdSndMemAllocateBank(type, bank.num, bank.address, bank.size);
        }
        if (bank.data_size > 0) {
            sdMemBlkSetPrm(each, unit + bank.data_offset, bank.data_size,
                           SDD_MEMBLK_NO_FUNC, NULL);
            sdBankDownload(each, type, (Sint8)bank.num);
        }
    }
    sdMemBlkDestroy(each);
    return SDE_ERR_NOTHING;
}

/* the streams. a stream's bytes are its file's, read whole as it starts */
struct talk {
    int used;
    int stream;
    Sint32 stat;
    unsigned long started; /* the vsync it started at */
    uint32_t rate, samples; /* the file's, from its header */
    int loops;
};
static struct talk talks[STREAMS];
static uint8_t *bytes;
static size_t bytes_room;
/* the voices' archive's files, as ADXF_LoadPartition read its table */
static char afs_name[256];
static uint32_t afs_offset[AFS_FILES], afs_size[AFS_FILES];
static int afs_count;

static uint32_t big16(const uint8_t *at)
{
    return (uint32_t)at[0] << 8 | at[1];
}

static uint32_t big32(const uint8_t *at)
{
    return (uint32_t)at[0] << 24 | (uint32_t)at[1] << 16 |
           (uint32_t)at[2] << 8 | at[3];
}

static uint32_t little32(const uint8_t *at)
{
    return (uint32_t)at[3] << 24 | (uint32_t)at[2] << 16 |
           (uint32_t)at[1] << 8 | at[0];
}

/* read size bytes from offset in the disc's file name into bytes. a size of
 * 0 reads the whole file. it returns the count read */
static size_t read_bytes(const char *name, long offset, size_t size)
{
    FILE *file = native_disc_file(name);
    size_t got = 0;

    if (!file) {
        return 0;
    }
    if (size == 0 && fseek(file, 0, SEEK_END) == 0) {
        size = (size_t)ftell(file);
    }
    if (size > bytes_room) {
        uint8_t *more = realloc(bytes, size);

        if (more) {
            bytes = more;
            bytes_room = size;
        }
    }
    if (size <= bytes_room && fseek(file, offset, SEEK_SET) == 0) {
        got = fread(bytes, 1, size, file);
    }
    fclose(file);
    return got;
}

/* it returns 1 where the size bytes read hold a loop, as sa_audio_stream says
 * a file does, and else 0. the engine loops its stream by the same words, so
 * the game reads a stream as ended only where the engine plays it out */
static int loops(size_t size)
{
    uint32_t first;
    uint32_t end;

    if (size < ADX_LOOP_HEAD ||
        big16(bytes + ADX_DATA) + ADX_DTOFS_BASE < ADX_LOOP_HEAD ||
        big16(bytes + ADX_LOOPS) != 1) {
        return 0;
    }
    first = big32(bytes + ADX_LOOP_START);
    end = big32(bytes + ADX_LOOP_END);
    return first < end && end <= big32(bytes + ADX_SAMPLES);
}

/* start a stream of the size bytes read, heard wait_ms after. bytes that
 * aren't an ADX file start nothing, and what the stream played before
 * stops */
static void start(struct talk *talk, size_t size, int32_t wait_ms)
{
    uint32_t game;

    if (size < ADX_HEAD || (bytes[0] << 8 | bytes[1]) != ADX_MARK) {
        ADXT_Stop((ADXT)talk);
        return;
    }
    talk->rate = big32(bytes + ADX_RATE);
    talk->samples = big32(bytes + ADX_SAMPLES);
    talk->loops = loops(size);
    talk->started = native_vsyncs;
    talk->stat = ADXT_STAT_PLAYING;
    if (native_sound_engine) {
        if (wait_ms) {
            command(SA_AUDIO_STREAM_WAIT, (uint32_t)talk->stream, 0, wait_ms, 0,
                    0);
        }
        game = native_fpu_host();
        sa_audio_stream(native_sound_engine, (uint32_t)talk->stream, bytes,
                        (uint32_t)size);
        native_fpu_game(game);
    }
}

/* the samples a stream has played by the vsyncs since it started */
static uint32_t played(const struct talk *talk)
{
    return (uint32_t)((unsigned long long)(native_vsyncs - talk->started) *
                      talk->rate * VSYNC_NS / 1000000000ULL);
}

void ADXT_Init(void)
{
}

void ADXT_ConfigVsyncSvr(Sint32 flag)
{
    (void)flag;
}

void ADXT_ExecServer(void)
{
}

void ADXT_EntryErrFunc(void (*func)(void *obj, char *msg), void *obj)
{
    (void)func, (void)obj;
}

ADXT ADXT_Create(Sint32 maxnch, void *work, Sint32 worksize)
{
    (void)work, (void)worksize;
    for (int i = 0; i < STREAMS; i++) {
        if (!talks[i].used) {
            memset(&talks[i], 0, sizeof talks[i]);
            talks[i].used = 1;
            /* the music's has two channels, and the voice's one */
            talks[i].stream = maxnch >= 2 ? 0 : 1;
            return (ADXT)&talks[i];
        }
    }
    return NULL;
}

void ADXT_Destroy(ADXT adxt)
{
    struct talk *talk = (struct talk *)adxt;

    if (talk) {
        ADXT_Stop(adxt);
        talk->used = 0;
    }
}

void ADXT_StartFname(ADXT adxt, char *fname)
{
    if (adxt) {
        start((struct talk *)adxt, read_bytes(fname, 0, 0), READ_AHEAD_MS);
    }
}

/* the bytes of the ADX file that starts at adx, by its header: its data
 * takes a block for each channel and each ADX_BLOCK_SAMPLES samples. it reads
 * the header through its count of samples. it returns 0 for bytes that don't
 * start an ADX file */
static size_t adx_size(const uint8_t *adx)
{
    if ((adx[0] << 8 | adx[1]) != ADX_MARK) {
        return 0;
    }
    return (size_t)(adx[ADX_DATA] << 8 | adx[ADX_DATA + 1]) + ADX_DTOFS_BASE +
           (big32(adx + ADX_SAMPLES) + ADX_BLOCK_SAMPLES - 1) /
               ADX_BLOCK_SAMPLES * ADX_BLOCK * adx[ADX_CHANNELS];
}

int sa1_adx_whole(const void *adx, size_t held)
{
    size_t size = held >= ADX_SAMPLES + 4 ? adx_size(adx) : 0;

    return size != 0 && size <= held;
}

/* an ADX file in memory, as movie.c hands over a movie's sound. it reads the
 * file to the length adx_size gives. the caller's bytes hold that many, as
 * sa1_adx_whole tests. bytes that aren't an ADX file start nothing, and what
 * the stream played before stops */
void ADXT_StartMem(ADXT adxt, void *adxdat)
{
    size_t size;

    if (!adxt) {
        return;
    }
    size = adx_size(adxdat);
    if (size == 0) {
        ADXT_Stop(adxt);
        return;
    }
    if (size > bytes_room) {
        uint8_t *more = realloc(bytes, size);

        if (!more) {
            ADXT_Stop(adxt);
            return;
        }
        bytes = more;
        bytes_room = size;
    }
    memcpy(bytes, adxdat, size);
    start((struct talk *)adxt, size, 0);
}

void ADXT_StartAfs(ADXT adxt, Sint32 patid, Sint32 fid)
{
    size_t size = 0;

    (void)patid;
    if (adxt && fid >= 0 && fid < afs_count) {
        size = read_bytes(afs_name, afs_offset[fid], afs_size[fid]);
    }
    if (adxt) {
        start((struct talk *)adxt, size, 0);
    }
}

void ADXT_Stop(ADXT adxt)
{
    struct talk *talk = (struct talk *)adxt;

    if (talk && talk->stat != ADXT_STAT_STOP) {
        talk->stat = ADXT_STAT_STOP;
        command(SA_AUDIO_STREAM_STOP, (uint32_t)talk->stream, 0, 0, 0, 0);
    }
}

Sint32 ADXT_GetStat(ADXT adxt)
{
    struct talk *talk = (struct talk *)adxt;

    if (talk && talk->stat == ADXT_STAT_PLAYING && !talk->loops &&
        played(talk) >= talk->samples) {
        talk->stat = ADXT_STAT_PLAYEND;
    }
    return talk ? talk->stat : ADXT_STAT_STOP;
}

void ADXT_GetTime(ADXT adxt, Sint32 *ncount, Sint32 *tscale)
{
    struct talk *talk = (struct talk *)adxt;
    Sint32 stat = ADXT_GetStat(adxt);

    *ncount = 0;
    *tscale = 1;
    if (stat == ADXT_STAT_PLAYING) {
        *ncount = (Sint32)played(talk);
        *tscale = (Sint32)talk->rate;
    } else if (stat == ADXT_STAT_PLAYEND) {
        *ncount = (Sint32)talk->samples;
        *tscale = (Sint32)talk->rate;
    }
}

void ADXT_SetOutVol(ADXT adxt, Sint32 vol)
{
    struct talk *talk = (struct talk *)adxt;

    if (talk) {
        command(SA_AUDIO_STREAM_VOLUME, (uint32_t)talk->stream, 0, vol, 0, 0);
    }
}

void ADXT_SetAutoRcvr(ADXT adxt, Sint32 rmode)
{
    (void)adxt, (void)rmode;
}

void ADXT_SetSvrFreq(ADXT adxt, Sint32 freq)
{
    (void)adxt, (void)freq;
}

/* an AFS archive: its mark, its count of files, and each one's offset and
 * size, little-endian */
Sint32 ADXF_LoadPartition(Sint32 ptid, char *fname, void *ptinfo, Sint32 nmax)
{
    uint8_t head[8];
    FILE *file;

    (void)ptid, (void)ptinfo;
    afs_count = 0;
    snprintf(afs_name, sizeof afs_name, "%s", fname);
    if (!(file = native_disc_file(afs_name))) {
        return -1;
    }
    if (fread(head, 1, 8, file) == 8 && !memcmp(head, "AFS", 3)) {
        int count = (int)little32(head + 4);

        count = count < nmax ? count : nmax;
        count = count < AFS_FILES ? count : AFS_FILES;
        for (int i = 0; i < count && fread(head, 1, 8, file) == 8; i++) {
            afs_offset[i] = little32(head);
            afs_size[i] = little32(head + 4);
            afs_count = i + 1;
        }
    }
    fclose(file);
    return 0;
}
