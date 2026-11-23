/* Sofdec, CRI's movie player, on the port's side: the mwPly calls
 * modules/movie/movie.c makes, over pl_mpeg's demuxer and libmpeg2's
 * decoder, which make pc links into 1ST_READ.BIN in place of the movie
 * image's modules/movie/sdk/. the movie image's own code, its fades, skips
 * and timing, is the console's.
 *
 * a player streams its SFD, an MPEG-1 system stream, from the disc through
 * the host's file calls. pl_mpeg demuxes it, and libmpeg2
 * (xbox/third_party/libmpeg2/) decodes its video, 320 pixels across for a
 * picture the console shows 640 across, with its MMX and MMXEXT IDCT and
 * motion compensation, the Pentium III's: XBMC played DVDs on the Xbox with
 * it. pl_mpeg's own decoder, all C, ran the opening at some 27 frames a
 * second of its 30 on a console
 *
 * the SFD's audio is an ADX stream, in the MPEG audio packets of stream
 * 0xc0, which the console's ADX library plays, as Sofdec's SFADXT does: a
 * second demuxer over the same file takes the packets, mwPlyExecServer
 * copies their data into a ring joint, and an ADX stream started on the
 * joint decodes it, its server running from the blank. the ADX stream is
 * the movie's clock while it plays, as it is Sofdec's: before it starts,
 * the clock holds, and once it ends, or where a movie has no sound, the
 * clock counts the blanks on from there.
 *
 * mwPlyStartFrame, which the movie's loop calls each frame, sets the
 * movie's clock, and mwPlyExecServer decodes the pictures due by then. the
 * latest goes to the host as its planes (host_api's video_movie_frame), a
 * texture which xbox/port/render.c draws under each frame's lists, at the
 * player's brightness, as Sofdec's renderer draws it.
 *
 * pl_mpeg's and libmpeg2's memory is the room host_api's movie_memory
 * gives, which each player starts empty, and gives back when it ends */
#include <kamui.h>
#include <shinobi.h>
#include <sg_ov.h>
#include <sg_xpt.h>
#include <sofdec.h>
#include <string.h>

#include "xbox/port/host.h"
#include "xbox/port/port.h"

/* the overlay library's handles, which movie_main enters in its table */
OVS_LIB_OBJ mwg_gsc_libhdl;
OVS_LIB_OBJ mwg_sj_rbf_libhdl;

/* pl_mpeg's allocator: a bump of PORT_MOVIE, emptied per player. a block
 * keeps its size ahead of it, for realloc */
static Uint8 *movie_next;
static Uint8 *movie_room; /* PORT_MOVIE_SIZE bytes, while a movie plays */

static void *movie_alloc(Uint32 size)
{
    Uint32 *block;

    size = (size + 15) & ~15u;
    if (movie_next == NULL || movie_next + size + 16 > movie_room + PORT_MOVIE_SIZE) {
        port_log("movie: out of memory");
        return NULL;
    }
    block = (Uint32 *)movie_next;
    block[0] = size;
    movie_next += size + 16;
    return (Uint8 *)block + 16;
}

static void *movie_realloc(void *p, Uint32 size)
{
    void *to;

    if (p == NULL) {
        return movie_alloc(size);
    }
    to = movie_alloc(size);
    if (to != NULL) {
        Uint32 old = *(Uint32 *)((Uint8 *)p - 16);

        memcpy(to, p, old < size ? old : size);
    }
    return to;
}

static void movie_free(void *p)
{
    (void)p;
}

#define PLM_NO_STDIO
/* the SDK's stdlib.h has no abs */
#define abs(x) __builtin_abs(x)
#define PLM_MALLOC(size) movie_alloc((Uint32)(size))
#define PLM_REALLOC(p, size) movie_realloc((p), (Uint32)(size))
#define PLM_FREE(p) movie_free(p)
#define PL_MPEG_IMPLEMENTATION
#include "xbox/third_party/pl_mpeg.h"

#include "xbox/third_party/libmpeg2/mpeg2.h"

/* libmpeg2's memory, from the same room, at the 64 bytes its blocks want.
 * the room is emptied per player, so a free does nothing */
static void *mpeg2_room(unsigned size, mpeg2_alloc_t reason)
{
    Uint8 *p = movie_alloc((Uint32)size + 64);

    (void)reason;
    return p != NULL ? (void *)(((Uint32)p + 63) & ~63u) : NULL;
}

static int mpeg2_room_free(void *p)
{
    (void)p;
    return 1;
}

/* the movie's two textures' ids, past the renderer's texture cache's: a
 * frame goes to the one the last frame didn't, so that its upload never
 * waits for the NV2A to finish drawing the last */
#define MOVIE_TEXTURE 2000
/* the video's packets, MPEG's video stream 0 */
#define VIDEO_PACKETS 0xe0
/* the pictures the decoder may fall behind the clock before it skips the
 * B-pictures, as Sofdec drops frames: some 0.2 s at 30 a second */
#define CATCH_UP 6
/* 27 MHz, the unit of a sequence's frame period, by a blank of 1/60 s */
#define PERIOD_PER_BLANK 450000u

/* the SFD's audio: its packets' stream, and the ADX stream's memory. the
 * ring holds the packets' data, about 1.5 s of a stereo stream at 44.1 kHz,
 * and its extra bytes mirror its start for a chunk past its end. the ADX
 * stream's work has, by sofdec.h, 16448 bytes of output for each of its two
 * channels and an input ring of the rest */
#define AUDIO_PACKETS 0xc0
#define AUDIO_RING (128 * 1024)
#define AUDIO_RING_EXTRA (32 * 1024)
#define AUDIO_WORK (2 * 16448 + 64 + 64 * 1024)
#define AUDIO_CHANNELS 2
/* the most a feed copies at once, which the ring's extra bytes take */
#define AUDIO_CHUNK AUDIO_RING_EXTRA
/* the blanks the clock waits for the ADX stream to start before it counts
 * without it */
#define AUDIO_START_BLANKS 60

/* a demuxer's reads of the player's file, each at its own offset */
struct movie_source {
    struct movie_player *player;
    Uint32 offset;
};

struct movie_player {
    MW_PLY_OBJ object;
    /* the video: its demuxer, libmpeg2's decoder and its state, the frame
     * period of 27 MHz, the pictures decoded and whether the stream ended */
    plm_demux_t *video_demux;
    mpeg2dec_t *mpeg2;
    const mpeg2_info_t *info;
    Uint32 frame_period;
    Uint32 pictures;
    int video_ended;
    void *file;
    Sint32 size;
    struct movie_source video, audio_source;
    /* the audio: its demuxer, the packet's data not yet in the ring, the
     * ring joint and the ADX stream. audio is null without sound */
    plm_demux_t *audio_demux;
    const Uint8 *pending;
    Sint32 pending_len;
    int audio_ended; /* its packets all in the ring */
    SJ sj;
    ADXT audio;
    Sint32 volume;
    /* the clock: the blanks it had at a blank, from which it counts on
     * while the ADX stream isn't its source */
    Uint32 clock_base;
    Uint32 clock_base_blank;
    int audio_clock; /* whether the ADX stream may yet drive the clock */
    MWE_PLY_STAT stat;
    Sint32 bright;
    Uint32 start_blank; /* port_blanks at its start */
    Uint32 blanks; /* the blanks the movie has played */
    int width, height;
    int has_frame;
    int texture; /* 0 or 1: the latest frame's of the two textures */
};

void *port_gdfs_take_file(GDFS gdfs);
extern volatile Uint32 port_blanks;

static struct movie_player player;

/* what render.c draws under the lists: the movie's texture, its size and
 * its brightness, while a player has a frame */
int port_movie_frame(unsigned *texture, int *width, int *height, int *bright)
{
    if (!player.has_frame || player.stat == MWE_PLY_STAT_STOP) {
        return 0;
    }
    *texture = MOVIE_TEXTURE + (unsigned)player.texture;
    *width = player.width;
    *height = player.height;
    *bright = player.bright;
    return 1;
}

/* the buffer's file callbacks, as pl_mpeg's own for a FILE, over the host's
 * reads at a source's offset */
static void load(plm_buffer_t *buffer, void *user)
{
    struct movie_source *source = user;
    struct movie_player *self = source->player;
    Sint32 room;
    int got;

    if (buffer->discard_read_bytes) {
        plm_buffer_discard_read_bytes(buffer);
    }
    room = (Sint32)(buffer->capacity - buffer->length);
    if (room <= 0) {
        /* a buffer full of bytes not yet read is no end */
        return;
    }
    got = 0;
    if (self->file != NULL && (Sint32)source->offset < self->size) {
        got = HOST->file_read(self->file, source->offset, buffer->bytes + buffer->length, room);
    }
    if (got <= 0) {
        buffer->has_ended = TRUE;
        return;
    }
    source->offset += (Uint32)got;
    buffer->length += (size_t)got;
}

static void seek(plm_buffer_t *buffer, size_t offset, void *user)
{
    (void)buffer;
    ((struct movie_source *)user)->offset = (Uint32)offset;
}

static size_t tell(plm_buffer_t *buffer, void *user)
{
    (void)buffer;
    return ((struct movie_source *)user)->offset;
}

/* the next packet of the video, or null for none yet, or at the end */
static plm_packet_t *video_packet(struct movie_player *self)
{
    for (;;) {
        plm_packet_t *packet = plm_demux_decode(self->video_demux);

        if (packet == NULL) {
            self->video_ended = plm_buffer_has_ended(self->video_demux->buffer);
            return NULL;
        }
        if (packet->type == VIDEO_PACKETS) {
            return packet;
        }
    }
}

/* decode up to the due picture, giving libmpeg2 the packets it asks for.
 * the last picture shown, if the last decoded shows one, goes to the host
 * once, from its planes, into the texture the last frame didn't take */
static void video_decode(struct movie_player *self, Uint32 due)
{
    const mpeg2_fbuf_t *shown = NULL;

    mpeg2_skip(self->mpeg2, (Sint32)(due - self->pictures) > CATCH_UP);
    while (self->pictures < due && !self->video_ended) {
        plm_packet_t *packet;

        switch (mpeg2_parse(self->mpeg2)) {
        case STATE_BUFFER:
            packet = video_packet(self);
            if (packet == NULL) {
                goto done;
            }
            mpeg2_buffer(self->mpeg2, packet->data, packet->data + packet->length);
            break;
        case STATE_SEQUENCE:
            if (self->info->sequence->frame_period != 0) {
                self->frame_period = self->info->sequence->frame_period;
            }
            break;
        case STATE_SLICE:
        case STATE_END:
        case STATE_INVALID_END:
            self->pictures++;
            shown = self->info->display_fbuf;
            break;
        default:
            break;
        }
    }
done:
    if (shown != NULL && self->info->sequence != NULL) {
        const mpeg2_sequence_t *sequence = self->info->sequence;

        self->width = (int)sequence->picture_width;
        self->height = (int)sequence->picture_height;
        self->texture = self->has_frame ? self->texture ^ 1 : 0;
        HOST->video_movie_frame(MOVIE_TEXTURE + (unsigned)self->texture, self->width, self->height, shown->buf[0],
                                shown->buf[1], shown->buf[2], (int)sequence->width, (int)sequence->chroma_width);
        if (!self->has_frame) {
            static char line[64];
            int sprintf(char *, const char *, ...);

            sprintf(line, "movie: %dx%d", self->width, self->height);
            port_log(line);
        }
        self->has_frame = 1;
    }
}

/* the ADX stream's end, and the ring's */
static void audio_destroy(struct movie_player *self)
{
    if (self->audio != NULL) {
        ADXT_Stop(self->audio);
        ADXT_Destroy(self->audio);
        self->audio = NULL;
    }
    if (self->sj != NULL) {
        SJ_Destroy(self->sj);
        self->sj = NULL;
    }
    self->audio_demux = NULL;
    self->pending = NULL;
    self->pending_len = 0;
}

/* copy the audio packets' data into the ring while it has room */
static void audio_feed(struct movie_player *self)
{
    while (self->sj != NULL && !self->audio_ended) {
        SJCK chunk;
        Sint32 count;

        if (self->pending_len == 0) {
            plm_packet_t *packet = plm_demux_decode(self->audio_demux);

            if (packet == NULL) {
                self->audio_ended = plm_buffer_has_ended(self->audio_demux->buffer);
                return;
            }
            if (packet->type != AUDIO_PACKETS) {
                continue;
            }
            self->pending = packet->data;
            self->pending_len = (Sint32)packet->length;
            continue;
        }
        count = self->pending_len < AUDIO_CHUNK ? self->pending_len : AUDIO_CHUNK;
        self->sj->vtbl->GetChunk(self->sj, SJ_LIN_FREE, count, &chunk);
        if (chunk.len <= 0) {
            self->sj->vtbl->UngetChunk(self->sj, SJ_LIN_FREE, &chunk);
            return;
        }
        memcpy(chunk.data, self->pending, (size_t)chunk.len);
        self->pending += chunk.len;
        self->pending_len -= chunk.len;
        self->sj->vtbl->PutChunk(self->sj, SJ_LIN_DATA, &chunk);
        if (chunk.len < count) {
            /* the ring is full */
            return;
        }
    }
}

/* the ADX stream over a second demuxer of the file, from its start. a
 * movie whose memory, joint or stream can't be had plays silent */
static void audio_start(struct movie_player *self)
{
    plm_buffer_t *buffer;
    char *ring;
    void *work;

    self->audio_ended = 0;
    self->audio_clock = 0;
    self->audio_source.offset = 0;
    buffer = plm_buffer_create_with_callbacks(load, seek, tell, (size_t)self->size, &self->audio_source);
    ring = movie_alloc(AUDIO_RING + AUDIO_RING_EXTRA + 32);
    work = movie_alloc(AUDIO_WORK);
    if (buffer == NULL || ring == NULL || work == NULL) {
        return;
    }
    ring = (char *)(((Uint32)ring + 31) & ~31u);
    self->audio_demux = plm_demux_create(buffer, 1);
    if (self->audio_demux == NULL) {
        return;
    }
    self->sj = SJRBF_Create(ring, AUDIO_RING, AUDIO_RING_EXTRA);
    if (self->sj == NULL) {
        port_log("movie: no ring joint for the sound");
        return;
    }
    self->audio = ADXT_Create(AUDIO_CHANNELS, work, AUDIO_WORK);
    if (self->audio == NULL) {
        port_log("movie: no ADX stream for the sound");
        audio_destroy(self);
        return;
    }
    ADXT_SetOutVol(self->audio, self->volume);
    audio_feed(self);
    if (self->sj->vtbl->GetNumData(self->sj, SJ_LIN_DATA) == 0) {
        /* no sound in the file */
        audio_destroy(self);
        return;
    }
    ADXT_StartSj(self->audio, self->sj);
    self->audio_clock = 1;
}

static void player_destroy(MWPLY ply)
{
    struct movie_player *self = (struct movie_player *)ply;

    audio_destroy(self);
    if (self->file != NULL) {
        HOST->file_close(self->file);
        self->file = NULL;
    }
    if (self->mpeg2 != NULL) {
        mpeg2_close(self->mpeg2);
        self->mpeg2 = NULL;
    }
    self->video_demux = NULL;
    self->stat = MWE_PLY_STAT_STOP;
    self->has_frame = 0;
    if (movie_room != NULL) {
        HOST->movie_memory(0);
        movie_room = NULL;
        movie_next = NULL;
    }
}

/* where set, as HOST_POKE sets it, each movie ends as it starts: for a
 * machine too slow for them, as xemu's emulated CPU is, where a test means
 * to reach the menus */
int port_movie_skip;

/* fname names an SFD in the current directory, as gdFsOpen opens it */
static void player_start_fname(MWPLY ply, Sint8 *fname)
{
    struct movie_player *self = (struct movie_player *)ply;
    GDFS handle;
    plm_buffer_t *buffer;

    if (port_movie_skip) {
        self->stat = MWE_PLY_STAT_PLAYEND;
        return;
    }
    handle = gdFsOpen((const char *)fname, NULL);

    self->stat = MWE_PLY_STAT_ERROR;
    if (handle == NULL) {
        port_log("movie: no such file");
        return;
    }
    /* the host's file behind the handle, read directly by offset */
    gdFsGetFileSize(handle, &self->size);
    self->file = port_gdfs_take_file(handle);
    self->video.player = self;
    self->video.offset = 0;
    self->audio_source.player = self;
    if (movie_room == NULL) {
        movie_room = HOST->movie_memory(PORT_MOVIE_SIZE);
    }
    if (movie_room == NULL) {
        port_log("movie: no memory");
        return;
    }
    movie_next = movie_room;
    buffer = plm_buffer_create_with_callbacks(load, seek, tell, (size_t)self->size, &self->video);
    if (buffer == NULL) {
        return;
    }
    self->video_demux = plm_demux_create(buffer, 1);
    /* libmpeg2's MMX and MMXEXT code, the Pentium III's, which it doesn't
     * detect here (config.h), and its memory from the room */
    mpeg2_accel(MPEG2_ACCEL_X86_MMX | MPEG2_ACCEL_X86_MMXEXT);
    mpeg2_malloc_hooks(mpeg2_room, mpeg2_room_free);
    self->mpeg2 = self->video_demux != NULL ? mpeg2_init() : NULL;
    if (self->mpeg2 == NULL) {
        return;
    }
    self->info = mpeg2_info(self->mpeg2);
    self->frame_period = 900900; /* 29.97 frames a second, until the sequence says */
    self->pictures = 0;
    self->video_ended = 0;
    audio_start(self);
    self->start_blank = port_blanks;
    self->blanks = 0;
    self->clock_base = 0;
    self->clock_base_blank = port_blanks;
    self->has_frame = 0;
    self->stat = MWE_PLY_STAT_PLAYING;
}

static void player_stop(MWPLY ply)
{
    struct movie_player *self = (struct movie_player *)ply;

    if (self->stat == MWE_PLY_STAT_PLAYING) {
        self->stat = MWE_PLY_STAT_STOP;
    }
    if (self->audio != NULL) {
        ADXT_Stop(self->audio);
    }
}

static MWE_PLY_STAT player_get_stat(MWPLY ply)
{
    return ((struct movie_player *)ply)->stat;
}

/* the time played, in blanks of 1/60 second */
static void player_get_time(MWPLY ply, Sint32 *ncount, Sint32 *tscale)
{
    *ncount = (Sint32)((struct movie_player *)ply)->blanks;
    *tscale = 60;
}

/* vol in tenths of a decibel, 0 the loudest, as ADXT_SetOutVol takes it */
static void player_set_out_vol(MWPLY ply, Sint32 vol)
{
    struct movie_player *self = (struct movie_player *)ply;

    self->volume = vol;
    if (self->audio != NULL) {
        ADXT_SetOutVol(self->audio, vol);
    }
}

static MWD_IF player_calls = {
    NULL, NULL, NULL, NULL, NULL, player_destroy, player_start_fname, player_stop, player_get_stat,
    player_get_time, NULL, player_set_out_vol, NULL, NULL, NULL,
};

/* Sofdec's pace: its vertical blank handler gives the movie's loop back
 * from the mwg_nvsync_pic-th blank since mwPlyStartFrame, 2 for a movie of
 * 29.97 frames a second, through Kamui's wait-vsync callback, which the
 * flip runs. so the loop, and the frame counts its fades take, run at 30 a
 * second */
#define NVSYNC_PIC 2
static Uint32 frame_start;

static void wait_vsync(void *arg)
{
    (void)arg;
    while (port_blanks - frame_start < NVSYNC_PIC) {
        HOST->wait_vblank();
    }
}

void mwPlyInitSofdec(MWS_PLY_INIT_SFD *iprm)
{
    (void)iprm;
    frame_start = port_blanks;
    kmSetWaitVsyncCallback(wait_vsync, NULL);
}

void mwPlyFinishSofdec(void)
{
    kmSetWaitVsyncCallback(NULL, NULL);
}

MWPLY mwPlyCreateSofdec(MWS_PLY_CPRM_SFD *cprm)
{
    (void)cprm;
    memset(&player, 0, sizeof player);
    player.object.vtbl = &player_calls;
    player.bright = 255;
    player.stat = MWE_PLY_STAT_STOP;
    return &player.object;
}

void mwPlySetFastHalfpel(MWPLY mwply, Sint32 sw)
{
    (void)mwply;
    (void)sw;
}

void mwPlySetBright(MWPLY mwply, Sint32 val)
{
    ((struct movie_player *)mwply)->bright = val;
}

/* the movie's clock, in blanks: the ADX stream's time while it plays, and
 * otherwise the blanks since the clock's base. it holds while the stream
 * starts, for AUDIO_START_BLANKS at most */
static void clock_update(struct movie_player *self)
{
    Sint32 stat = self->audio != NULL ? ADXT_GetStat(self->audio) : ADXT_STAT_STOP;

    if (self->audio_clock && (stat == ADXT_STAT_PLAYING || stat == ADXT_STAT_DECEND)) {
        Sint32 ncount;
        Sint32 tscale;

        ADXT_GetTime(self->audio, &ncount, &tscale);
        if (tscale > 0) {
            self->blanks = (Uint32)((long long)ncount * 60 / tscale);
        }
        self->clock_base = self->blanks;
        self->clock_base_blank = port_blanks;
        return;
    }
    if (self->audio_clock && (stat == ADXT_STAT_DECINFO || stat == ADXT_STAT_PREP) && self->clock_base == 0 &&
        port_blanks - self->clock_base_blank < AUDIO_START_BLANKS) {
        return;
    }
    self->audio_clock = self->audio_clock && stat != ADXT_STAT_PLAYEND && stat != ADXT_STAT_STOP;
    self->blanks = self->clock_base + (port_blanks - self->clock_base_blank);
}

/* the movie's clock, however many blanks pass between the movie loop's
 * calls, two at Ninja's 30 frames a second */
void mwPlyStartFrame(void)
{
    frame_start = port_blanks;
    if (player.stat == MWE_PLY_STAT_PLAYING) {
        clock_update(&player);
    }
}

/* how far the pictures may stray from the clock before the player follows
 * it: within DRIFT of it either way, each call shows one picture */
#define DRIFT 2

/* one picture a call, and end the movie at its video's end. the movie's
 * loop calls this once a frame period, every NVSYNC_PIC blanks, so one
 * picture a call shows each for two blanks, as the console's Sofdec does.
 * the clock, the ADX stream's samples played, steps as its server copies
 * them to sound RAM: followed picture by picture, it showed pictures for
 * one blank and three by turns, a judder. so it only corrects drift: some
 * DRIFT pictures behind it, the player decodes up to it (skipping
 * B-pictures while it is CATCH_UP behind, video_decode), and DRIFT ahead of
 * it, as while the sound starts, it holds the picture shown */
void mwPlyExecServer(void)
{
    Uint32 clock_due;
    Uint32 next;
    Sint32 off;

    if (player.stat != MWE_PLY_STAT_PLAYING || player.mpeg2 == NULL) {
        return;
    }
    audio_feed(&player);
    clock_due = (Uint32)((unsigned long long)player.blanks * PERIOD_PER_BLANK / player.frame_period) + 1;
    next = player.pictures + 1;
    off = (Sint32)(clock_due - next);
    if (off >= DRIFT) {
        video_decode(&player, clock_due);
    } else if (off > -DRIFT) {
        video_decode(&player, next);
    }
    if (player.video_ended) {
        player.stat = MWE_PLY_STAT_PLAYEND;
    }
}
