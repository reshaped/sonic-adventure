/* the Chao race course's streams, with running textures and the sound of
 * water */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_race/scenery.h"
#include "al_race/stream.h"
#include "draw/state.h"
#include "draw/tree.h"
#include "object/simple.h"
#include "sound/cue.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"

/* the streams' objects */
extern NJS_OBJECT dat_0cbc6a90;
extern NJS_OBJECT dat_0cbc6e30;

/* what race_stream_start takes for a stream */
struct stream_kind {
    NJS_OBJECT *object;
    Sint16 step; /* how far its texture runs each step, in v units */
};

/* a stream's state, in its task's motion */
struct stream {
    NJS_OBJECT *object;
    NJS_COLOR *uvs; /* the first meshset's, which race_stream_step runs */
    Uint16 count; /* the number of UVs */
    Sint16 offset; /* how far the UVs have run, within UV_WRAP */
    Sint16 step;
};

#define STREAM(task) ((struct stream *)(task)->motion)

#define STREAM_COUNT 2

/* a texture's height in v units */
#define UV_WRAP 256

#define SE_STREAM 1102

static void race_stream_start(struct task *task, void *arg);
static void race_stream_step(struct task *task);
static void race_stream_draw(struct task *task);

static const struct sound_falloff stream_falloff = {127, -128, 100.0F, 700.0F};
static const struct task_desc stream_desc = {
    sizeof(struct stream), TASK_WORK,        race_stream_start,
    race_stream_step,      race_stream_draw, NULL};
static const struct stream_kind stream_kinds[STREAM_COUNT] = {
#include "al_race/stream/stream_kinds.inc"
};

void streams_create(void)
{
    const struct stream_kind *kind;
    /* @hack spelling: STREAM_COUNT through a local, which the loop tests in
     * R12 (MOV #2,R12 at 0x0cb993e8, CMP/GE R12,R13 at 0x0cb993fa); as
     * i < STREAM_COUNT, SHC unrolls the loop to two calls a pass, the
     * function runs 8 bytes long and 15 of 22 words differ */
    int count = STREAM_COUNT;
    int i;

    for (i = 0, kind = stream_kinds; i < count; i++, kind++) {
        scenery_add(&stream_desc, (void *)kind);
    }
}

static void race_stream_start(struct task *task, void *arg)
{
    NJS_OBJECT *object = ((struct stream_kind *)arg)->object;
    NJS_MESHSET *meshset = object->model->meshsets;
    struct task_work *work = task->work;
    struct stream *stream = STREAM(task);

    scenery_place(work, object);
    /* @hack spelling: the kind's object read again through arg once
     * scenery_place returns (MOV.L @R13,R2 at 0x0cb9942e); through object,
     * 25 of 36 words differ */
    stream->object = ((struct stream_kind *)arg)->object;
    stream->uvs = meshset->vertuv;
    stream->count = meshset_count_points(meshset);
    stream->offset = 0;
    stream->step = ((struct stream_kind *)arg)->step;
}

static void race_stream_step(struct task *task)
{
    struct stream *stream = STREAM(task);
    int count = stream->count;
    int step = stream->step;
    int offset = stream->offset + step;
    int i;
    NJS_COLOR *uv;

    if (offset < 0) {
        step += UV_WRAP;
    } else if (offset >= UV_WRAP) {
        step -= UV_WRAP;
    }
    stream->offset += step;
    for (i = 0, uv = stream->uvs; i < count; i++, uv++) {
        uv->tex.v += step;
    }
    sound_keep_near(SE_STREAM, task, &task->work->pos, &stream_falloff);
    race_stream_draw(task);
}

static void race_stream_draw(struct task *task)
{
    struct stream *stream;

    if (frame_step) {
        return;
    }
    stream = STREAM(task);
    draw_state_save();
    draw_constant_attr(NJD_FLAG_CLAMP_V, 0);
    texture_set_stage();
    draw_tree(stream->object);
    draw_state_restore();
}
