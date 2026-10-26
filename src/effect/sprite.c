/* sprites: textured rectangles that face the screen and play through a list
 * of frames as they move. sprite_create runs one on a task of its own */
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/sprite.h"
#include "effect/sprite_pool.h"
#include "main.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/task.h"

/* a channel from 0 to 1 as a byte of a packed ARGB color */
/* @hack type: the channel converted through int, which SHC does with FTRC
 * and STS alone (0x8c053866, 0x8c05386c); cast straight to Uint32,
 * sprite_render runs 160 bytes long, count unmeasured */
#define COLOR_BYTE(part) ((Uint32)(int)((part) * 255.0F))

void sprite_play_seek(struct sprite_play *play,
                      const struct sprite_frame *frames, Float age)
{
    const struct sprite_frame *frame;
    const struct sprite_frame *last;
    int index;

    last = frames;
    frame = frames + 1;
    index = 0;
    play->age = age;
    for (;;) {
        if (frame->texture < 0) {
            index = 0;
            age -= last->age;
            last = frames;
            frame = frames;
        } else {
            if (frame->age > age) {
                break;
            }
            index++;
            last = frame;
        }
        frame++;
    }
    play->frame = index;
}

/* the texture of frame, as texlist has it loaded */
#define TEXTURE(texlist, frame)                                                \
    ((NJS_TEXMEMLIST *)(texlist)->textures[(frame)->texture].texaddr)

void sprite_bind(NJS_TEXLIST *texlist, const struct sprite_frame *frames,
                 int flags)
{
    struct sprite_frame *frame;

    frame = (struct sprite_frame *)frames;
    if (flags & SPRITE_LOAD) {
        njLoadTexture(texlist);
    }
    while (frame->texture >= 0) {
        if (texlist->textures[frame->texture].texaddr != 0) {
            frame->index = TEXTURE(texlist, frame)->globalIndex;
            if (frame->flags & SPRITE_TEXTURE_SIZE) {
                frame->width =
                    (Float)TEXTURE(texlist, frame)->texinfo.texsurface.nWidth;
                frame->height =
                    (Float)TEXTURE(texlist, frame)->texinfo.texsurface.nHeight;
            }
            if (frame->flags & SPRITE_CENTERED) {
                frame->center_x = frame->width * 0.5F;
                frame->center_y = frame->height * 0.5F;
            }
        }
        frame++;
    }
}

void sprite_play_init(struct sprite_play *play)
{
    play->age = 0.0F;
    play->rate = 1.0F;
    play->frame = 0;
    play->loops = 0;
}

void sprite_set(struct sprite *sprite, NJS_POINT3 *pos,
                struct sprite_desc *desc, int flags)
{
    sprite->step = desc->step;
    sprite_body_init(&sprite->body, pos, &desc->speed);
    if (flags & SPRITE_MOVES) {
        sprite->pos = &sprite->body.pos;
    } else {
        sprite->pos = pos;
    }
    sprite->body.scale = desc->scale;
    sprite->body.depth = desc->depth;
    sprite->motion.drag = desc->drag;
    sprite->motion.gravity = desc->gravity;
    sprite->frames = desc->frames;
    sprite_play_init(&sprite->play);
    sprite->play.rate = desc->rate;
}

int sprite_play_step(struct sprite_play *play,
                     const struct sprite_frame *frames)
{
    const struct sprite_frame *frame;
    const struct sprite_frame *next;
    /* @hack redundant: the loop's count stored to times[0] in the test and
     * never read, which the image stores on the stack (MOV.L R3,@R15 at
     * 0x8c053710); without it, the function runs 2 bytes short and 33 of 59
     * words differ */
    Uint32 times[1];

    next = &frames[play->frame + 1];
    if (next->texture >= 0) {
        play->age += play->rate;
        if (play->age >= next->age) {
            frame = next++;
            switch (next->texture) {
            case SPRITE_LOOP:
                play->loops++;
                if (play->loops < (times[0] = next->index)) {
                    play->age -= frame->age;
                    play->frame = 0;
                    return SPRITE_LOOP;
                }
                /* fall through */
            case SPRITE_END:
                play->age = frame->age;
                return SPRITE_END;
            default:
                break;
            }
            play->frame++;
        }
        return 0;
    }
    return SPRITE_END;
}

int sprite_step(struct sprite *sprite)
{
    int result;

    if (sprite->step != NULL) {
        result = sprite->step(sprite);
    } else {
        sprite_body_move(&sprite->body, &sprite->motion);
        result = sprite_play_step(&sprite->play, sprite->frames);
    }
    return result;
}

void sprite_render(NJS_POINT3 *pos, struct sprite_body *body,
                   struct sprite_play *play, const struct sprite_frame *frames)
{
    const struct sprite_frame *frame;
    const struct sprite_frame *next;
    NJS_POINT3 view;
    NJS_POINT3 screen;
    NJS_POINT3 size;
    NJS_POINT3 center;
    NJS_TEXTURE_VTX quad[4];
    NJS_POINT3 screen_size;
    NJS_POINT3 screen_center;
    Uint32 color;
    Float scale;
    Float blend;
    Float alpha;
    Float red;
    Float green;
    Float blue;

    frame = frames + play->frame;
    next = frame + 1;
    if (!frame_step) {
        njCalcPoint(NULL, pos, &view);
        if (_nj_near_clip_ > view.z + body->depth) {
            return;
        }
        point_project_centered(&view, &screen);
        if (frame->flags & SPRITE_ADDITIVE) {
            njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
            njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        } else {
            njColorBlendingMode(NJD_SOURCE_COLOR,
                                NJD_COLOR_BLENDING_BOTHSRCALPHA);
        }
        /* the frame draws unblended at its own age, or when the next frame
         * has another texture */
        if (play->age == frame->age || frame->texture != next->texture) {
            scale = body->scale;
            size.x = frame->width * scale;
            size.y = frame->height * scale;
            center.x = frame->center_x * scale;
            center.y = frame->center_y * scale;
            color = COLOR_BYTE(frame->color.a) << 24 |
                    COLOR_BYTE(frame->color.r) << 16 |
                    COLOR_BYTE(frame->color.g) << 8 |
                    COLOR_BYTE(frame->color.b);
        } else {
            scale = body->scale;
            blend = (play->age - frame->age) / (next->age - frame->age);
            size.x = frame->width + (blend * (next->width - frame->width));
            size.y = frame->height + (blend * (next->height - frame->height));
            center.x =
                frame->center_x + (blend * (next->center_x - frame->center_x));
            center.y =
                frame->center_y + (blend * (next->center_y - frame->center_y));
            size.x *= scale;
            size.y *= scale;
            center.x *= scale;
            center.y *= scale;
            alpha = frame->color.a + (blend * (next->color.a - frame->color.a));
            red = frame->color.r + (blend * (next->color.r - frame->color.r));
            green = frame->color.g + (blend * (next->color.g - frame->color.g));
            blue = frame->color.b + (blend * (next->color.b - frame->color.b));
            color = COLOR_BYTE(alpha) << 24 | COLOR_BYTE(red) << 16 |
                    COLOR_BYTE(green) << 8 | COLOR_BYTE(blue);
        }
        /* the size and the center project as lengths at pos's view z,
         * without the body's depth */
        size.z = screen.z;
        center.z = screen.z;
        point_project_centered(&size, &screen_size);
        point_project_centered(&center, &screen_center);
        quad[0].u = quad[1].u = 0.0F;
        quad[2].u = quad[3].u = 1.0F;
        quad[0].v = quad[2].v = 0.0F;
        quad[1].v = quad[3].v = 1.0F;
        quad[0].col = quad[1].col = quad[2].col = quad[3].col = color;
        quad[0].z = quad[1].z = quad[2].z = quad[3].z =
            1.0F / (screen.z + body->depth);
        quad[0].x = quad[1].x = screen.x - screen_center.x + SCREEN_MIDDLE_X;
        quad[2].x = quad[3].x =
            screen.x - screen_center.x + screen_size.x + SCREEN_MIDDLE_X;
        quad[0].y = quad[2].y = screen.y - screen_center.y + SCREEN_MIDDLE_Y;
        quad[1].y = quad[3].y =
            screen.y - screen_center.y + screen_size.y + SCREEN_MIDDLE_Y;
        njDrawTexture(quad, 4, frame->index, TRUE);
    }
}

static void sprite_draw(struct sprite *sprite)
{
    sprite_render(sprite->pos, &sprite->body, &sprite->play, sprite->frames);
}

void sprite_draw_at(struct sprite *sprite, NJS_POINT3 *pos)
{
    sprite_render(pos, &sprite->body, &sprite->play, sprite->frames);
}

static void sprite_task_draw(struct task *task)
{
    struct sprite *sprite;

    sprite = task->extra;
    sprite_draw(sprite);
}

static void sprite_task_step(struct task *task)
{
    struct sprite *sprite;

    sprite = task->extra;
    sprite_task_draw(task);
    if (sprite_step(sprite) == SPRITE_END) {
        task_kill(task);
    }
}

static void sprite_task_step_own(struct task *task)
{
    struct sprite *sprite;

    sprite = task->extra;
    sprite_task_draw(task);
    if (sprite->step(sprite) == SPRITE_END) {
        task_kill(task);
    }
}

struct task *sprite_create(NJS_POINT3 *pos, struct sprite_desc *desc, int flags)
{
    struct task *task;
    struct sprite *sprite;

    if ((task = task_create(TASK_WORK, 4, sprite_task_step)) != NULL) {
        sprite = heap_alloc(sizeof(*sprite));
        if (sprite != NULL) {
            task->extra = sprite;
            sprite_set(sprite, pos, desc, flags);
            if (sprite->step != NULL) {
                task->step = sprite_task_step_own;
            }
            task->draw = sprite_task_draw;
        } else {
            task_destroy(task);
            task = NULL;
        }
    }
    return task;
}
