/* an event's clouds: two streams of sprites in a box that wraps about the
 * camera */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "draw/draw.h"
#include "shared/clouds.h"
#include "system/task.h"
#include "system/util.h"

/* each stream's box, which wraps about the camera */
#define CLOUDS_WIDTH 800.0F
#define CLOUDS_HEIGHT 200.0F
#define CLOUDS_DEPTH 800.0F

/* a cloud's alpha by its distance from the camera. it is 0 within
 * CLOUDS_NEAREST, and rises to 1 at CLOUDS_NEAR. it holds 1 out to
 * CLOUDS_FAR, then falls to 0 at CLOUDS_FARTHEST */
#define CLOUDS_NEAREST 40.0F
#define CLOUDS_NEAR 80.0F
#define CLOUDS_FAR 200.0F
#define CLOUDS_FARTHEST 400.0F

/* how far the boxes sway from their place in x, and in y */
#define CLOUDS_SWAY_X 100.0F
#define CLOUDS_SWAY_Y 20.0F

/* what the clouds' step does, by work->mode */
enum clouds_mode {
    CLOUDS_SEED, /* it scatters both streams' clouds, then sets CLOUDS_DRIFT */
    CLOUDS_DRIFT /* it places and draws both streams, then moves their boxes */
};

struct cloud {
    NJS_POINT3 offset; /* its place in the box, -size / 2 to size / 2 */
    NJS_POINT3 pos; /* where it draws */
    Float scale;
    Sint32 frame; /* which of the three textures it shows */
    Angle ang;
    Float alpha;
    Angle twinkle; /* the phase of the twinkle that scales its alpha */
};

struct cloud_stream {
    /* where its box sits. the step sways it in x and y, and drifts it along
     * z */
    NJS_POINT3 origin;
    /* origin less the camera's place, wrapped into the box */
    NJS_POINT3 from_camera;
    NJS_POINT3 size; /* the box's size on each axis */
    struct cloud *clouds;
    Sint32 count;
    Float alpha;
};

/* how far the boxes drift along z each step */
static Float event_clouds_speed = 5.0F;
/* the most a cloud's scale can be, which event_clouds_start sets */
static Float event_clouds_scale = 1.0F;
/* how many clouds each stream has, which event_clouds_start sets */
static Sint32 event_clouds_count = 100;
static struct cloud *event_clouds_0 = NULL;
static struct cloud *event_clouds_1 = NULL;
static struct cloud_stream event_clouds_stream_0 = {
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {CLOUDS_WIDTH, CLOUDS_HEIGHT, CLOUDS_DEPTH},
    NULL,
    0,
    1.0F,
};
static struct cloud_stream event_clouds_stream_1 = {
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {CLOUDS_WIDTH, CLOUDS_HEIGHT, CLOUDS_DEPTH},
    NULL,
    0,
    1.0F,
};
static NJS_TEXANIM event_clouds_anim[3] = {
#include "shared/clouds/event_clouds_anim.inc"
};
static NJS_SPRITE event_clouds_sprite = {
#include "shared/clouds/event_clouds_sprite.inc"
};
/* the clouds' task while they're out */
static struct task *event_clouds = NULL;

static void event_clouds_destroy(struct task *task);
static void event_clouds_draw(struct task *task);
static void event_clouds_seed(struct cloud_stream *stream);
static void event_clouds_wrap(struct cloud_stream *stream);
static void event_clouds_place(struct cloud_stream *stream);
static void event_clouds_draw_stream(struct cloud_stream *stream);

static void event_clouds_step(struct task *task)
{
    struct task_work *work = task->work;
    Float z;
    Float x;

    switch (work->mode) {
    case CLOUDS_SEED:
        task->destructor = event_clouds_destroy;
        task->draw = event_clouds_draw;
        event_clouds_seed(&event_clouds_stream_0);
        event_clouds_seed(&event_clouds_stream_1);
        work->mode = CLOUDS_DRIFT;
        break;
    case CLOUDS_DRIFT:
        event_clouds_wrap(&event_clouds_stream_0);
        event_clouds_place(&event_clouds_stream_0);
        event_clouds_draw_stream(&event_clouds_stream_0);
        event_clouds_wrap(&event_clouds_stream_1);
        event_clouds_place(&event_clouds_stream_1);
        event_clouds_draw_stream(&event_clouds_stream_1);
        z = work->pos.z + event_clouds_speed;
        if (z > CLOUDS_DEPTH) {
            z -= CLOUDS_DEPTH;
        }
        event_clouds_stream_0.origin.z = z;
        event_clouds_stream_1.origin.z = z;
        work->pos.z = z;
        x = njCos(work->ang.y) * CLOUDS_SWAY_X;
        event_clouds_stream_0.origin.x = x;
        event_clouds_stream_1.origin.x = x;
        event_clouds_stream_0.alpha = 1.0F;
        event_clouds_stream_1.alpha = 1.0F;
        work->ang.y += 0x100;
        event_clouds_stream_0.origin.y = njSin(work->ang.x) * CLOUDS_SWAY_Y;
        event_clouds_stream_1.origin.y = njCos(work->ang.x) * CLOUDS_SWAY_Y;
        work->ang.x += 0x200;
        break;
    }
}

static void event_clouds_destroy(struct task *task)
{
}

static void event_clouds_draw(struct task *task)
{
    event_clouds_draw_stream(&event_clouds_stream_0);
    event_clouds_draw_stream(&event_clouds_stream_1);
}

/* scatter the stream's clouds through its box. each takes a random texture
 * and angle, and a scale up to a fifth below event_clouds_scale. a cloud's
 * twinkle starts at its angle */
static void event_clouds_seed(struct cloud_stream *stream)
{
    struct cloud *cloud = stream->clouds;
    int i;

    for (i = 0; i < event_clouds_count; i++, cloud++) {
        cloud->offset.x = ((rand() * RAND_SCALE) - 0.5F) * CLOUDS_WIDTH;
        cloud->offset.y = ((rand() * RAND_SCALE) - 0.5F) * CLOUDS_HEIGHT;
        cloud->offset.z = ((rand() * RAND_SCALE) - 0.5F) * CLOUDS_DEPTH;
        cloud->scale =
            event_clouds_scale * (1.0F - (rand() * (0.2F * RAND_SCALE)));
        cloud->frame = rand() * (3.0F * RAND_SCALE);
        cloud->ang = RANDOM_ANGLE();
        cloud->twinkle = cloud->ang;
    }
}

static void event_clouds_wrap(struct cloud_stream *stream)
{
    Float *walk;
    Float x;
    Float y;
    Float z;
    Float cx;
    Float cy;
    Float cz;

    /* @hack spelling: origin and the camera's place read through walk, a Float
     * cursor (FMOV.S @R4+,FR13 at 0x0cb81568 in EV0026), and from_camera stored
     * back through it (FMOV.S FR15,@-R4 at 0x0cb815c0); without it, the wrap
     * runs 12 bytes long and 60 of its 62 words differ */
    walk = &stream->origin.x;
    x = *walk++;
    y = *walk++;
    z = *walk;
    walk = &camera_work->pos.x;
    cx = *walk++;
    cy = *walk++;
    cz = *walk;
    x -= cx;
    y -= cy;
    z -= cz;
    x += -floorf(x * (1.0F / CLOUDS_WIDTH)) * CLOUDS_WIDTH;
    y += -floorf(y * (1.0F / CLOUDS_HEIGHT)) * CLOUDS_HEIGHT;
    z += -floorf(z * (1.0F / CLOUDS_DEPTH)) * CLOUDS_DEPTH;
    /* @hack spelling: the first step back from past z a statement of its own,
     * which keeps cy in FR5 and cz in FR4 (0x0cb8157a and 0x0cb8157e in
     * EV0026); as *--walk = z, 4 of 62 words differ */
    walk = &stream->from_camera.z + 1;
    walk--;
    *walk = z;
    *--walk = y;
    *--walk = x;
}

/* place each cloud within half a box of the camera on each axis. its place in
 * the box is offset + from_camera, folded in by one size at most. its place
 * from the camera is that less half of CLOUDS_WIDTH, CLOUDS_HEIGHT and
 * CLOUDS_DEPTH. its alpha follows its distance from the camera, times
 * |sin twinkle| */
static void event_clouds_place(struct cloud_stream *stream)
{
    struct cloud *cloud = stream->clouds;
    int i;
    Float *walk;
    Float x;
    Float y;
    Float z;
    Float wx;
    Float wy;
    Float wz;
    Float width;
    Float height;
    Float depth;
    Float cx;
    Float cy;
    Float cz;
    Float alpha;

    for (i = 0; i < stream->count; i++, cloud++) {
        /* @hack spelling: each vector read through walk, a Float cursor, and
         * pos stored back through it from past z (FMOV.S FR8,@-R4 at 0x0cb816be
         * in EV0026); without it, the place runs 12 bytes long and 133 of its
         * 154 words differ */
        /* @hack spelling: offset's first step a statement of its own, which
         * keeps wy in FR9 and wz in FR7 (0x0cb81654 and 0x0cb81658 in
         * EV0026); as x = *walk++, 5 of 154 words differ */
        walk = &cloud->offset.x;
        x = *walk;
        walk++;
        y = *walk++;
        z = *walk;
        walk = &stream->from_camera.x;
        wx = *walk++;
        wy = *walk++;
        wz = *walk;
        x += wx;
        y += wy;
        z += wz;
        walk = &stream->size.x;
        width = *walk++;
        height = *walk++;
        depth = *walk;
        if (x > width) {
            x -= width;
        } else if (x < 0.0F) {
            x += width;
        }
        if (y > height) {
            y -= height;
        } else if (y < 0.0F) {
            y += height;
        }
        if (z > depth) {
            z -= depth;
        } else if (z < 0.0F) {
            z += depth;
        }
        x -= CLOUDS_WIDTH / 2;
        y -= CLOUDS_HEIGHT / 2;
        z -= CLOUDS_DEPTH / 2;
        walk = &camera_work->pos.x;
        cx = *walk++;
        cy = *walk++;
        cz = *walk;
        cx += x;
        cy += y;
        cz += z;
        walk = &cloud->pos.z + 1;
        *--walk = cz;
        *--walk = cy;
        *--walk = cx;
        x *= x;
        y *= y;
        z *= z;
        x += y;
        x += z;
        x = njSqrt(x);
        if (x < CLOUDS_NEAR) {
            if (x < CLOUDS_NEAREST) {
                alpha = 0.0F;
            } else {
                alpha = (x - CLOUDS_NEAREST) *
                        (1.0F / (CLOUDS_NEAR - CLOUDS_NEAREST));
            }
        } else if (x > CLOUDS_FAR) {
            if (x > CLOUDS_FARTHEST) {
                alpha = 0.0F;
            } else {
                alpha = (CLOUDS_FARTHEST - x) *
                        (1.0F / (CLOUDS_FARTHEST - CLOUDS_FAR));
            }
        } else {
            alpha = 1.0F;
        }
        cloud->alpha = alpha * _builtin_fabsf(njSin(cloud->twinkle));
        cloud->twinkle += 0x400;
    }
}

/* draw the stream's clouds as sprites, added to the screen in white. a
 * cloud draws at its alpha times the stream's, and not at all below 0.01 */
static void event_clouds_draw_stream(struct cloud_stream *stream)
{
    NJS_ARGB color = {1.0F, 1.0F, 1.0F, 1.0F};
    struct cloud *cloud;
    Float alpha;
    int i;

    if (stream->alpha < 0.01F) {
        return;
    }
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    for (i = 0, cloud = stream->clouds; i < stream->count; i++, cloud++) {
        alpha = cloud->alpha * stream->alpha;
        if (alpha < 0.01F) {
            continue;
        }
        color.a = alpha;
        njSetConstantMaterial(&color);
        event_clouds_sprite.p = cloud->pos;
        event_clouds_sprite.sx = cloud->scale * 1.2F;
        event_clouds_sprite.sy = cloud->scale * 0.8F;
        event_clouds_sprite.ang = cloud->ang;
        njDrawSprite3D(&event_clouds_sprite, cloud->frame,
                       NJD_SPRITE_ANGLE | NJD_SPRITE_COLOR | NJD_SPRITE_SCALE |
                           NJD_SPRITE_ALPHA);
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

void event_clouds_start(Float scale, int count)
{
    struct cloud_stream *stream;

    if (event_clouds == NULL) {
        event_clouds = task_create(TASK_WORK, 5, event_clouds_step);
        event_clouds_scale = scale;
        event_clouds_count = count;
        event_clouds_0 = heap_alloc(event_clouds_count * sizeof(struct cloud));
        stream = &event_clouds_stream_0;
        stream->clouds = event_clouds_0;
        stream->count = event_clouds_count;
        event_clouds_1 = heap_alloc(event_clouds_count * sizeof(struct cloud));
        stream = &event_clouds_stream_1;
        stream->clouds = event_clouds_1;
        stream->count = event_clouds_count;
    }
}

void event_clouds_stop(void)
{
    if (event_clouds != NULL) {
        task_kill(event_clouds);
        event_clouds = NULL;
        heap_free(event_clouds_0);
        heap_free(event_clouds_1);
    }
}

void event_clouds_set_speed(Float speed)
{
    event_clouds_speed = speed;
}
