/* Sofdec, CRI's movie player, on the port's side: the mwPly calls
 * modules/movie/movie.c makes, over pl_mpeg, which make pc links into
 * 1ST_READ.BIN in place of the movie image's modules/movie/sdk/. the movie
 * image's own code, its fades, skips and timing, is the console's.
 *
 * a player streams its SFD, an MPEG-1 system stream, from the disc through
 * the host's file calls, and pl_mpeg demuxes it and decodes its video, 320
 * pixels across for a picture the console shows 640 across.
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
 * movie's clock, and mwPlyExecServer decodes the frames due by then. the latest frame is a
 * texture, which xbox/port/render.c draws under each frame's lists, at the
 * player's brightness, as Sofdec's renderer draws it.
 *
 * pl_mpeg's memory is the room host_api's movie_memory gives, which each
 * player starts empty, and gives back when it ends */
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

/* pl_mpeg's allocator: a bump of PORT_MOVIE, emptied per player, below the
 * room the latest frame's ARGB takes at its top. a block keeps its size
 * ahead of it, for realloc */
#define FRAME_BYTES (640 * 480 * 4)
static Uint8 *movie_next;
static Uint8 *movie_room; /* PORT_MOVIE_SIZE bytes, while a movie plays */

static void *movie_alloc(Uint32 size)
{
    Uint32 *block;

    size = (size + 15) & ~15u;
    if (movie_next == NULL || movie_next + size + 16 > movie_room + PORT_MOVIE_SIZE - FRAME_BYTES) {
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

/* the movie texture's id, past the renderer's texture cache's */
#define MOVIE_TEXTURE 2000
/* how much of a stream the probe reads for its streams */
#define PROBE_BYTES (256 * 1024)

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
    plm_t *plm;
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
    Uint32 decoded; /* the blanks decoded to */
    int width, height;
    int has_frame;
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
    *texture = MOVIE_TEXTURE;
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

static void on_video(plm_t *plm, plm_frame_t *frame, void *user)
{
    struct movie_player *self = user;
    Uint32 *texels = (Uint32 *)(movie_room + PORT_MOVIE_SIZE) - frame->width * frame->height;
    unsigned k;

    (void)plm;
    plm_frame_to_bgra(frame, (uint8_t *)texels, (int)frame->width * 4);
    for (k = 0; k < frame->width * frame->height; k++) {
        texels[k] |= 0xff000000u;
    }
    self->width = (int)frame->width;
    self->height = (int)frame->height;
    HOST->video_texture(MOVIE_TEXTURE, self->width, self->height, (const unsigned *)texels);
    if (!self->has_frame) {
        static char line[64];
        int sprintf(char *, const char *, ...);

        sprintf(line, "movie: %dx%d", self->width, self->height);
        port_log(line);
    }
    self->has_frame = 1;
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
    self->plm = NULL;
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
    self->plm = plm_create_with_buffer(buffer, 1);
    if (self->plm == NULL) {
        return;
    }
    /* an SFD's system header gives a video bound of 0, so the streams come
     * from a probe of the packets. its audio is ADX, which pl_mpeg would
     * take for MP2 and search to the end for a header of: the decoders are
     * made for the video alone, then the stream starts again */
    plm_demux_probe(self->plm->demux, PROBE_BYTES);
    self->plm->demux->num_audio_streams = 0;
    self->plm->has_decoders = FALSE;
    self->plm->video_packet_type = 0;
    self->plm->audio_packet_type = 0;
    if (!plm_init_decoders(self->plm)) {
        return;
    }
    plm_rewind(self->plm);
    plm_set_audio_enabled(self->plm, 0);
    plm_set_loop(self->plm, 0);
    plm_set_video_decode_callback(self->plm, on_video, self);
    audio_start(self);
    self->start_blank = port_blanks;
    self->blanks = 0;
    self->decoded = 0;
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

/* the most the player decodes to catch up, in seconds, before it skips */
#define CATCH_UP 0.2

/* decode the frames due by the clock, and end the movie at its stream's
 * end */
void mwPlyExecServer(void)
{
    if (player.stat != MWE_PLY_STAT_PLAYING || player.plm == NULL) {
        return;
    }
    audio_feed(&player);
    if (player.blanks > player.decoded) {
        double owed = (player.blanks - player.decoded) / 60.0;

        /* a machine too slow to decode every frame in its time, as xemu's
         * emulated CPU is, drops them, as Sofdec does: past CATCH_UP behind,
         * the player seeks to the clock's time, which decodes the keyframe
         * before it, and else decoding all it owes would only fall further
         * behind */
        if (owed > CATCH_UP) {
            plm_seek(player.plm, plm_get_time(player.plm) + owed, 0);
        } else {
            plm_decode(player.plm, owed);
        }
        player.decoded = player.blanks;
    }
    if (plm_has_ended(player.plm)) {
        player.stat = MWE_PLY_STAT_PLAYEND;
    }
}
