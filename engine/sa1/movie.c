/* MOVIE.BIN on a host. it plays the opening and the stories' movies as
 * modules/movie/movie.c does. pl_mpeg takes sofdec's part. it splits an SFD
 * into its MPEG-1 pictures and its ADX sound, which sound.c plays. the
 * sound's time picks the picture to show. each is drawn as sofdec's renderer
 * draws it, as a YUV422 texture on a strip over the screen. the slice doesn't
 * link MOVIE.BIN's cast.c, so list 7 draws the cast's black alone */
#include <kamui.h>
#include <ninja.h>
#include <shinobi.h>
#include <sofdec.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native.h"
#include "ninja/njkamui.h"
#include "sa1.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"
#pragma clang diagnostic pop

/* a movie of the lists: its file and its fades, as MOVIE.BIN's movies */
struct movie {
    const char *file;
    int fade; /* FADE_OUT_ONLY, FADE_FULL or FADE_HALF */
    int fade_frame; /* the frame its fade out starts at */
};

/* each fades out over FADE_OUT_FRAMES from its fade frame. all but
 * FADE_OUT_ONLY first fade in from dark over FADE_IN_TIME hundredths of a
 * second. FADE_HALF's fade out starts from half brightness. the fade after
 * Start skips takes SKIP_FRAMES, and lowers the volume VOLUME_STEP a frame
 * to VOLUME_LEAST */
#define FADE_OUT_ONLY 0
#define FADE_FULL 1
#define FADE_HALF 2
#define FADE_IN_TIME 100
#define FADE_OUT_FRAMES 30
#define SKIP_FRAMES 15
#define VOLUME_STEP 3
#define VOLUME_LEAST (-999)
/* the brightness a fade in reaches, and a movie's own before its fades */
#define BRIGHT 192
/* the opening's list, and the list the cast's black covers. cast_fade holds
 * the black's alpha, or CAST_SOLID for opaque */
#define LIST_OPENING 0
#define LIST_CAST 7
#define LIST_END (-1)
#define CAST_SOLID (-1)

/* where a movie is in its fades, as MOVIE.BIN's */
enum fade_state {
    FADE_IN, /* from dark, over FADE_IN_TIME */
    FADE_PLAY, /* until the movie's fade frame */
    FADE_OUT, /* to dark, over FADE_OUT_FRAMES */
    FADE_DONE, /* dark for the rest of the movie */
    FADE_SKIP, /* Start: the skip's fade starts this frame */
    FADE_SKIP_OUT /* to dark, over SKIP_FRAMES */
};

static const struct movie movies[10] = {
    {"SLH448_1.SFD", FADE_FULL, 170},    {"SAH448_6.SFD", FADE_FULL, 3180},
    {"SA1_600.SFD", FADE_HALF, 2870},    {"SA2_600.SFD", FADE_FULL, 378},
    {"SA3_600.SFD", FADE_FULL, 860},     {"SA4_600.SFD", FADE_FULL, 550},
    {"SA5_600.SFD", FADE_OUT_ONLY, 390}, {"SA6_600.SFD", FADE_FULL, 630},
    {"SA7_300.SFD", FADE_FULL, 300},     {"SA8_600.SFD", FADE_FULL, 540},
};
static const int lists[9][3] = {
    {0, 1, LIST_END}, {2, LIST_END}, {3, LIST_END},
    {4, LIST_END},    {5, LIST_END}, {6, LIST_END},
    {7, LIST_END},    {8, LIST_END}, {9, LIST_END},
};

/* a movie's sound starts START_FRAMES frames after the movie. a brightness
 * shows SHOWN_FRAMES frames after the frame that sets it. each picture shows
 * at once */
#define START_FRAMES 14
#define SHOWN_FRAMES 2

/* TEXFMT_YUV422 is the PowerVR's YUV422, which ninja.h names no format for.
 * MOVIE_INDEX is the movie's global index, which no PVM file's texture may
 * share */
#define TEXFMT_YUV422 0x03
#define MOVIE_INDEX 0x7fff0000U

/* a movie as it plays: its pictures, by pl_mpeg, and its sound's file */
struct player {
    plm_video_t *video;
    uint8_t *adx;
    int pictures, decoded;
    double rate; /* pictures a second */
    ADXT talk;
    /* the picture the texture holds, -1 for none */
    int shown;
    int bright;
    int volume;
    /* the brightness and the cast's black of the last SHOWN_FRAMES frames */
    int brights[SHOWN_FRAMES], covers[SHOWN_FRAMES];
};

/* the texture each picture takes in turn, in its texels */
static uint16_t *texels;
static NJS_TEXINFO movie_info;
static NJS_TEXNAME movie_name;
static NJS_TEXLIST movie_texlist = {&movie_name, 1};

/* the strip's corners: bottom left, top left, bottom right, top right */
static NJS_TEXTURE_VTX strip[4];
/* the cast's black, a quad over the screen at its alpha */
static NJS_POINT2 cover_points[4] = {
    {0.0F, 0.0F}, {0.0F, 480.0F}, {640.0F, 480.0F}, {640.0F, 0.0F}};
static NJS_COLOR cover_colors[4];
static NJS_POINT2COL cast_cover = {cover_points, cover_colors, NULL, 4};

/* these are MOVIE.BIN's. movie_skipped marks Start in a fade in.
 * movie_result is 1 while the list plays on, and -1 after the pads' quit. it
 * is 0 once a movie stops the list or Start skips one */
static int movie_frame;
static enum fade_state movie_fade_state;
static int movie_fade_frame;
static int movie_skipped;
static int movie_result;
static int cast_fade;

extern PDS_PERIPHERAL *per_info[4];
void pad_enable(void);
void pad_scan(void);
Bool pad_quit_pressed(int unused);
void sbExitSystem(void);
void stage_set_stage_act(uint8_t stage, uint8_t act);

/* read name's file whole, and split it into the pictures and the ADX file.
 * p->adx is null where the sound's bytes hold no ADX file whole, since
 * ADXT_StartMem reads one to the length its header gives. it returns 0 where
 * it can't read the file, or finds no pictures or frame rate */
static int movie_open(struct player *p, const char *name)
{
    FILE *file = native_disc_file(name);
    uint8_t *bytes;
    long size;
    size_t adx_bytes = 0, adx_room = 1 << 20;
    plm_buffer_t *in, *video;
    plm_demux_t *demux;
    plm_packet_t *packet;

    if (!file || fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) <= 0 ||
        fseek(file, 0, SEEK_SET) != 0 || !(bytes = malloc((size_t)size))) {
        if (file) {
            fclose(file);
        }
        return 0;
    }
    if (fread(bytes, 1, (size_t)size, file) != (size_t)size) {
        size = 0;
    }
    fclose(file);
    in = plm_buffer_create_with_memory(bytes, (size_t)size, 0);
    demux = plm_demux_create(in, 1);
    video = plm_buffer_create_for_appending((size_t)size);
    p->adx = malloc(adx_room);
    while ((packet = plm_demux_decode(demux))) {
        if (packet->type == PLM_DEMUX_PACKET_VIDEO_1) {
            plm_buffer_write(video, packet->data, packet->length);
        } else if (packet->type == PLM_DEMUX_PACKET_AUDIO_1) {
            if (adx_bytes + packet->length > adx_room) {
                adx_room = 2 * (adx_bytes + packet->length);
                p->adx = realloc(p->adx, adx_room);
            }
            memcpy(p->adx + adx_bytes, packet->data, packet->length);
            adx_bytes += packet->length;
        }
    }
    plm_demux_destroy(demux);
    free(bytes);
    plm_buffer_signal_end(video);
    /* a picture's start code, 0 0 1 0, begins each picture */
    p->pictures = 0;
    for (size_t i = 0; i + 3 < video->length; i++) {
        p->pictures += video->bytes[i] == 0 && video->bytes[i + 1] == 0 &&
                       video->bytes[i + 2] == 1 && video->bytes[i + 3] == 0;
    }
    p->video = plm_video_create_with_buffer(video, 1);
    p->rate = plm_video_get_framerate(p->video);
    if (!sa1_adx_whole(p->adx, adx_bytes)) {
        free(p->adx);
        p->adx = NULL;
    }
    p->decoded = 0;
    p->shown = -1;
    return p->rate > 0 && p->pictures > 0;
}

static void movie_close(struct player *p)
{
    if (p->video) {
        plm_video_destroy(p->video);
    }
    free(p->adx);
    memset(p, 0, sizeof *p);
}

/* the sound's time in seconds, 0 until it starts */
static double movie_time(const struct player *p, Sint32 *ncount, Sint32 *tscale)
{
    *ncount = 0;
    *tscale = 1;
    if (p->talk) {
        ADXT_GetTime(p->talk, ncount, tscale);
    }
    return (double)*ncount / *tscale;
}

/* decode up to picture number, and write it to the texels in YUV422. each U
 * and V serves two rows, as the PowerVR's converter turns YUV420 to YUV422 */
static void movie_picture(struct player *p, int number)
{
    plm_frame_t *frame = NULL;
    int w, h;

    while (p->decoded <= number) {
        if (!(frame = plm_video_decode(p->video))) {
            return;
        }
        p->decoded++;
    }
    if (!frame) {
        return;
    }
    w = (int)frame->width;
    h = (int)frame->height;
    texels = realloc(texels, (size_t)w * (size_t)h * 2);
    for (int y = 0; y < h; y++) {
        const uint8_t *luma = frame->y.data + y * frame->y.width;
        const uint8_t *u = frame->cb.data + (y / 2) * frame->cb.width;
        const uint8_t *v = frame->cr.data + (y / 2) * frame->cr.width;
        uint16_t *out = texels + y * w;

        for (int x = 0; x < w; x += 2) {
            out[x] = (uint16_t)(u[x / 2] | luma[x] << 8);
            out[x + 1] = (uint16_t)(v[x / 2] | luma[x + 1] << 8);
        }
    }
    if (p->shown >= 0) {
        njReleaseTexture(&movie_texlist);
    } else {
        float tu, tv, su, sv, x, y, sx, sy;

        /* the strip places a picture as rnv_disp.c's mwRnvCalcDef places one
         * up to 320 by 240. a taller one shows at its own height.
         * mwRnvCalcDef does so for heights of 241 to 320 and 361 to 480 */
        tu = 0.25F / (float)w;
        su = (float)(w - 1) / (float)w;
        x = (640.0F - 2.0F * (float)w) / 2.0F;
        sx = 2.0F * (float)(w - 1);
        if (h <= 240) {
            tv = 0.25F / (float)h;
            sv = (float)(h - 1) / (float)h;
            y = (480.0F - 2.0F * (float)h) / 2.0F;
            sy = 2.0F * (float)(h - 1);
        } else {
            tv = 0.0F;
            sv = 1.0F;
            y = (480.0F - (float)h) / 2.0F;
            sy = (float)h;
        }
        strip[0] =
            (NJS_TEXTURE_VTX){x, y + sy, 1.0F / 65536.0F, tu, tv + sv, 0};
        strip[1] = (NJS_TEXTURE_VTX){x, y, 1.0F / 65536.0F, tu, tv, 0};
        strip[2] = (NJS_TEXTURE_VTX){x + sx,  y + sy,  1.0F / 65536.0F,
                                     tu + su, tv + sv, 0};
        strip[3] =
            (NJS_TEXTURE_VTX){x + sx, y, 1.0F / 65536.0F, tu + su, tv, 0};
    }
    njSetTextureInfo(&movie_info, texels, NJD_TEXFMT_RECTANGLE | TEXFMT_YUV422,
                     w, h);
    njSetTextureName(&movie_name, &movie_info, MOVIE_INDEX,
                     NJD_TEXATTR_TYPE_MEMORY | NJD_TEXATTR_GLOBALINDEX);
    njLoadTexture(&movie_texlist);
    p->shown = number;
}

/* draw the picture nearest the sound's time. its brightness and the cast's
 * black are those set SHOWN_FRAMES frames before. the strip takes sofdec's
 * context: opaque, filtered bilinearly, clamped and without fog */
static void movie_draw(struct player *p, int list, double time)
{
    int number = (int)(time * p->rate + 0.5);
    int bright = p->brights[movie_frame % SHOWN_FRAMES];
    int cover = p->covers[movie_frame % SHOWN_FRAMES];
    uint32_t color;

    p->brights[movie_frame % SHOWN_FRAMES] = p->bright;
    p->covers[movie_frame % SHOWN_FRAMES] = cast_fade;
    if (!native_picture_sink) {
        return;
    }
    color = 0xff000000U |
            (uint32_t)(movie_frame < START_FRAMES ? 0 : bright) * 0x010101U;
    if (number >= p->pictures) {
        number = p->pictures - 1;
    }
    if (number != p->shown) {
        movie_picture(p, number);
    }
    if (p->shown >= 0) {
        for (int i = 0; i < 4; i++) {
            strip[i].col = color;
        }
        njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
        njTextureClampMode(NJD_TEXTURECLAMP_CLAMP_UV);
        njFogDisable();
        njDrawTexture(strip, 4, (Int)MOVIE_INDEX, 0);
    }
    if (list == LIST_CAST && cover != 0) {
        int solid = cover == CAST_SOLID;
        uint32_t black = solid ? 0xff000000U : (uint32_t)cover << 24;

        for (int i = 0; i < 4; i++) {
            cover_colors[i].color = black;
        }
        njDrawPolygon2D(&cast_cover, 4, -1.0F,
                        solid ? NJD_FILL : NJD_FILL | NJD_TRANSPARENT);
    }
    sa1_picture_end();
}

/* a frame of the movies: two vsyncs, as MOVIE.BIN sets on a VGA cable, the
 * cable the slice's syCblCheckCable names */
static void movie_wait(void)
{
    native_vsync();
    native_vsync();
}

static void movie_set_bright(struct player *p, int level)
{
    p->bright = level & 0xff;
}

static void movie_fade_start(struct player *p, int current)
{
    movie_skipped = cast_fade = 0;
    if (movies[current].fade != FADE_OUT_ONLY) {
        movie_set_bright(p, 0);
        movie_fade_state = FADE_IN;
    } else {
        movie_fade_state = FADE_PLAY;
    }
}

/* step MOVIE.BIN's fades a frame, at time in hundredths of a second. it
 * returns 1 once the skip's fade out reaches dark, and else 0 */
static int movie_fade_step(struct player *p, int list, int current, int time)
{
    int level;
    int vol;

    switch (movie_fade_state) {
    case FADE_IN:
        level = time * BRIGHT / FADE_IN_TIME;
        if (level >= BRIGHT) {
            movie_fade_state++;
            level = BRIGHT;
            if (movie_skipped) {
                movie_fade_state = FADE_SKIP;
            }
        }
        movie_set_bright(p, level);
        break;
    case FADE_PLAY:
        if (movie_frame >= movies[current].fade_frame) {
            movie_fade_frame = movie_frame;
            movie_fade_state++;
        }
        break;
    case FADE_OUT:
        if (list != LIST_CAST) {
            level = (movie_fade_frame - movie_frame + FADE_OUT_FRAMES) *
                    BRIGHT /
                    (movies[current].fade == FADE_HALF ? 2 * FADE_OUT_FRAMES
                                                       : FADE_OUT_FRAMES);
            if (level < 0) {
                movie_fade_state++;
                level = 0;
            }
            movie_set_bright(p, level);
        } else {
            cast_fade =
                (movie_frame - movie_fade_frame) * 255 / FADE_OUT_FRAMES;
            if (cast_fade >= 255) {
                cast_fade = CAST_SOLID;
                movie_fade_state++;
            }
        }
        break;
    case FADE_SKIP:
        movie_fade_frame = movie_frame;
        movie_fade_state++;
        /* fall through: the skip's fade starts this frame */
    case FADE_SKIP_OUT:
        vol = (movie_fade_frame - movie_frame) * VOLUME_STEP;
        if (vol < VOLUME_LEAST) {
            vol = VOLUME_LEAST;
        }
        p->volume = vol;
        if (p->talk) {
            ADXT_SetOutVol(p->talk, vol);
        }
        if (list != LIST_CAST) {
            level = (movie_fade_frame - movie_frame + SKIP_FRAMES) * BRIGHT /
                    SKIP_FRAMES;
            if (level < 0) {
                level = 0;
                movie_fade_state = FADE_DONE;
            }
            movie_set_bright(p, level);
            if (level == 0) {
                return 1;
            }
        } else {
            cast_fade = (movie_frame - movie_fade_frame) * 255 / SKIP_FRAMES;
            if (cast_fade >= 255) {
                cast_fade = CAST_SOLID;
                movie_fade_state = FADE_DONE;
            }
            if (cast_fade < 0) {
                return 1;
            }
        }
        break;
    case FADE_DONE:
        break;
    }
    return 0;
}

/* play one movie of list to its end, a skip or the pads' quit */
static void movie_run(int list, int current)
{
    struct player p = {0};
    Sint32 ncount, tscale;

    if (!movie_open(&p, movies[current].file)) {
        movie_close(&p);
        movie_result = 0;
        return;
    }
    fprintf(stderr, "sa1: movie %s, %d pictures, at vsync %lu\n",
            movies[current].file, p.pictures, native_vsyncs);
    p.bright = BRIGHT;
    movie_fade_start(&p, current);
    for (int i = 0; i < SHOWN_FRAMES; i++) {
        p.brights[i] = p.bright;
        p.covers[i] = 0;
    }
    movie_frame = -1;
    for (;;) {
        double time;

        movie_wait();
        movie_frame++;
        if (movie_frame == START_FRAMES) {
            /* a movie with no whole ADX file, or no ADXT for it, stops the
             * list. so does one whose file ADXT_StartMem starts no sound
             * from. in MOVIE.BIN, an ADX that fails to decode does. p.volume
             * is 0 unless a skip has lowered it */
            if (!p.adx || !(p.talk = ADXT_Create(2, NULL, 0))) {
                movie_result = 0;
                break;
            }
            ADXT_SetOutVol(p.talk, p.volume);
            ADXT_StartMem(p.talk, p.adx);
            if (ADXT_GetStat(p.talk) == ADXT_STAT_STOP) {
                movie_result = 0;
                break;
            }
        }
        pad_scan();
        if (pad_quit_pressed(0)) {
            if (list == LIST_OPENING) {
                /* the console goes back to its boot ROM. the slice's
                 * syBtExit ends the run, and never returns here */
                sbExitSystem();
                syBtExit();
            } else {
                movie_result = -1;
                break;
            }
        }
        /* Start skips any movie, as MOVIE.BIN's NONMATCHING side does */
        if (per_info[0]->press & PDD_DGT_ST) {
            movie_result = 0;
            if (movie_fade_state == FADE_IN) {
                movie_skipped = 1;
            } else {
                if (movie_fade_state == FADE_DONE) {
                    break;
                }
                if (movie_fade_state == FADE_PLAY) {
                    movie_fade_state = FADE_SKIP;
                }
            }
        }
        time = movie_time(&p, &ncount, &tscale);
        if (movie_fade_step(&p, list, current, ncount * 100 / tscale)) {
            break;
        }
        /* the movie ends with its sound */
        if (p.talk && ADXT_GetStat(p.talk) != ADXT_STAT_PLAYING) {
            break;
        }
        movie_draw(&p, list, time);
    }
    if (p.talk) {
        ADXT_Stop(p.talk);
        ADXT_Destroy(p.talk);
    }
    if (p.shown >= 0) {
        njReleaseTexture(&movie_texlist);
    }
    movie_close(&p);
    movie_wait();
}

/* MOVIE.BIN's entry. it returns -1 after the pads' quit. otherwise it
 * returns 1 after the opening, list 0, and 0 after any other. it gives back
 * Ninja's context as the list found it, where MOVIE.BIN remakes the display.
 * where a run has a target stage and native_hook_opening answers nonzero, the
 * opening returns 0. main.c then begins the target's act, as after a story's
 * movie */
int movie_entry(int list)
{
    KMVERTEXCONTEXT context = *_nj_current_ctx_;

    if (list == LIST_OPENING && native_target_stage >= 0 &&
        native_hook_opening()) {
        stage_set_stage_act((uint8_t)native_target_stage,
                            (uint8_t)native_target_act);
        return 0;
    }
    movie_result = 1;
    pad_enable();
    pad_scan();
    for (int i = 0; lists[list][i] != LIST_END && movie_result > 0; i++) {
        movie_run(list, lists[list][i]);
    }
    *_nj_current_ctx_ = context;
    kmProcessVertexRenderState(_nj_current_ctx_);
    movie_wait();
    if (movie_result >= 0) {
        movie_result = list == LIST_OPENING;
    }
    return movie_result;
}
