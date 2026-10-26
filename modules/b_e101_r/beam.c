/* E-101 mkII's beam. a ball of light charges at its cannon, and the beam
 * shoots from it. where the beam's tip strikes the floor, a blast goes off */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "b_e101_r/beam.h"
#include "b_e101_r/blast.h"
#include "b_e101_r/files.h"
#include "b_egm2/blast.h"
#include "collision/collision.h"
#include "effect/beam.h"
#include "effect/sprite.h"
#include "enemy/enemy.h"
#include "hud/hud.h"
#include "object/ring.h"
#include "sound/sound.h"
#include "system/math.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

#define SE_BEAM 517 /* played as the beam fires, and as its blast goes off */

/* the beam's texture in e101_r_texlist */
#define BEAM_TEXTURE 35

/* the floor the fight is on: its height, and its squared reach in x and z
 * from floor_center. the tip strikes the floor within that reach */
#define FLOOR_Y 752.09F
#define FLOOR_REACH_SQUARED 53786.883F

/* the most steps the tip flies before the beam fades */
#define FLIGHT_STEPS 120
/* the distance the tip flies each step, times the length of
 * e101_mk2_beam_aim's dir */
#define TIP_SPEED 10.0F

#define PULSE_STEP 0x5555 /* the turn of the ball's pulse each step */
/* the most the pulse adds to the ball's size, either way */
#define PULSE_SIZE 0.05F
/* charging, the ball grows by this factor each step, up to a size of 1 */
#define CHARGE_GROWTH 1.08F
/* the beam's width while its fade is 1 */
#define BEAM_WIDTH 4.0F
/* fading, the ball's size and the fade keep this share of themselves each
 * step. below FADED, the beam stops drawing. at FADED or less, it ends */
#define FADE_SHRINK 0.8F
#define FADED 0.01F

/* the chance that a strike scatters rings in place of its blast, and the
 * rings it scatters */
#define STRIKE_RING_CHANCE 0.1F
#define STRIKE_RINGS 40

/* in a beam's flags: set once the beam begins to fade */
#define BEAM_FADING 0x1

/* a beam's record, in task->extra */
struct beam {
    int state; /* an enum beam_state */
    struct sprite ball; /* the ball of light, drawn at the foot or the tip */
    NJS_POINT3 *origin; /* the cannon, which the foot follows until it fires */
    NJS_POINT3 foot;
    NJS_POINT3 tip;
    NJS_VECTOR tip_speed; /* the tip's move each step */
    Float ball_size;
    /* 1, falling toward 0 as the beam fades. the beam's width scales with
     * it */
    Float fade;
    Float blast_size; /* the size of the blast the tip sets off */
    Angle pulse_phase;
    int flight_steps; /* the steps the tip has flown */
    Uint32 flags;
};

/* the middle of the floor the tip strikes */
static NJS_POINT3 floor_center = {0.0F, FLOOR_Y, -385.89F};

static struct sprite_frame ball_frames[8] = {
#include "b_e101_r/beam/ball_frames.inc"
};

/* the beam's texture, colors and corners. e101_mk2_beam_init fills in the
 * texture's global index */
static struct beam_texture beam_texture = {
#include "b_e101_r/beam/beam_texture.inc"
};

static struct collision_shape beam_shape = {
    SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
    ATTACK(2) | GUARD(3) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES |
        SHAPE_HURTS_OBJECTS,
    PUSH_PLAYERS,
    {0.0F, 0.0F, 0.0F},
    10.0F,
    0.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL};

static void beam_draw(struct task *task);

void e101_mk2_beam_init(void)
{
    sprite_bind(e101_r_texlist, ball_frames, 0);
    beam_texture.texture =
        ((NJS_TEXMEMLIST *)e101_r_texlist->textures[BEAM_TEXTURE].texaddr)
            ->globalIndex;
}

void e101_mk2_beam_clear(void)
{
}

static void beam_line(NJS_POINT3 *from, NJS_POINT3 *to, Float width)
{
    Float widths[2];

    widths[0] = width;
    widths[1] = width;
    beam_draw_line_textured(from, to, widths, &beam_texture);
}

/* set off a blast of size size at pos, which the blast copies. a strike can
 * scatter STRIKE_RINGS there instead, at a chance of STRIKE_RING_CHANCE. it
 * can only while the ring count is 0 or less. B or X must also be held on
 * the first connected pad */
static void beam_strike(NJS_POINT3 *pos, Float size)
{
    struct blast_params params;

    /* @hack type: the ring count cast to Sint16, which SHC sign-extends with
     * EXTS.W at 0x0c906f6e. ring_count returns rings, a Sint16, so the cast
     * keeps its value; without it, the code after it moves up 2 bytes, and 73
     * of 80 words differ */
    if ((Sint16)ring_count() <= 0 && rand() * RAND_SCALE < STRIKE_RING_CHANCE &&
        (per_info[0]->on & (PDD_DGT_TX | PDD_DGT_TB))) {
        ring_scatter(pos->x, pos->y, pos->z, STRIKE_RINGS);
        sound_play(SE_RINGS_LOST, 0, 0, 0);
    } else {
        params.pos = pos;
        params.flags = BLAST_COPY_POS | BLAST_HURTS;
        params.beam_start = 0;
        params.beam_end = 180;
        params.beam_length = size + 60.0F;
        params.beam_speed = 91;
        params.grow_start = 0;
        params.grow_end = 80;
        params.fade_start = 180;
        params.end = 220;
        params.size = size;
        e101_mk2_blast_create(&params);
        sound_play(SE_BEAM, 0, 0, 0);
    }
}

void e101_mk2_beam_set_state(struct task *task, int state)
{
    struct beam *beam = task->extra;

    beam->state = state;
}

void e101_mk2_beam_aim(struct task *task, NJS_VECTOR *dir, Float size)
{
    struct beam *beam = task->extra;

    beam->tip_speed.x = dir->x * TIP_SPEED;
    beam->tip_speed.y = dir->y * TIP_SPEED;
    beam->tip_speed.z = dir->z * TIP_SPEED;
    beam->blast_size = size;
}

static void beam_draw(struct task *task)
{
    struct beam *beam = task->extra;
    Float sine;
    Float cosine;
    Float size;

    _builtin_fsca(beam->pulse_phase, &sine, &cosine);
    size = beam->ball_size + (sine * PULSE_SIZE);
    switch (beam->state) {
    case BEAM_IDLE:
        break;
    case BEAM_CHARGE:
        beam->ball.body.scale = size;
        sprite_draw_at(&beam->ball, &beam->foot);
        break;
    case BEAM_FIRE:
    case BEAM_FLY:
        beam->ball.body.scale = 1.0F;
        sprite_draw_at(&beam->ball, &beam->tip);
        /* fall through: the ball at the foot and the beam draw as they do
         * fading */
    case BEAM_FADE:
        if (beam->fade < FADED) {
            break;
        }
        beam->ball.body.scale = size;
        sprite_draw_at(&beam->ball, &beam->foot);
        beam_line(&beam->foot, &beam->tip, beam->fade * BEAM_WIDTH);
        break;
    }
}

static void beam_step(struct task *task)
{
    struct beam *beam = task->extra;

    sprite_play_step(&beam->ball.play, beam->ball.frames);
    switch (beam->state) {
    case BEAM_IDLE:
        break;
    case BEAM_CHARGE:
        VECTOR_COPY(&beam->foot, beam->origin);
        beam->pulse_phase += PULSE_STEP;
        beam->ball_size *= CHARGE_GROWTH;
        if (!(beam->ball_size < 1.0F)) {
            beam->ball_size = 1.0F;
        }
        break;
    case BEAM_FIRE:
        VECTOR_COPY(&beam->foot, beam->origin);
        VECTOR_COPY(&beam->tip, beam->origin);
        beam->ball_size = 0.5F;
        sound_play_at_for(SE_BEAM, (int)task, 1, 0, beam->foot.x, beam->foot.y,
                          beam->foot.z, 45);
        beam->state++;
        break;
    case BEAM_FLY:
        VECTOR_SUM(&beam->tip, &beam->tip, &beam->tip_speed);
        if (beam->tip.y <= FLOOR_Y) {
            if (point_distance_squared_across(&beam->tip, &floor_center) <=
                FLOOR_REACH_SQUARED) {
                beam_strike(&beam->tip, beam->blast_size);
                beam->flags |= BEAM_FADING;
                beam->state++;
                break;
            }
        }
        if (++beam->flight_steps >= FLIGHT_STEPS) {
            beam->flags |= BEAM_FADING;
            beam->state++;
            break;
        }
        break;
    case BEAM_FADE:
        if (beam->foot.y <= FLOOR_Y) {
            if (point_distance_squared_across(&beam->foot, &floor_center) <=
                FLOOR_REACH_SQUARED) {
                task_kill(task);
                return;
            }
        }
        if (beam->fade <= FADED) {
            task_kill(task);
            return;
        }
        break;
    }
    if (beam->flags & BEAM_FADING) {
        beam->ball_size *= FADE_SHRINK;
        beam->fade *= FADE_SHRINK;
    }
    beam_draw(task);
}

struct task *e101_mk2_beam_create(NJS_POINT3 *origin)
{
    struct task *task;
    struct beam *beam;
    struct sprite_desc desc;

    if ((task = task_create(TASK_WORK, 4, beam_step)) != NULL) {
        beam = heap_alloc(sizeof(struct beam));
        if (beam != NULL) {
            collision_init(task, &beam_shape, COLLISION_ENEMY, 1, 1);
            task->work->collision->radius = 80.0F;
            enemy_alloc_record(task, task->work);
            task->extra = beam;
            beam->state = BEAM_IDLE;
            beam->origin = origin;
            beam->pulse_phase = 0;
            beam->ball_size = 0.1F;
            beam->fade = 1.0F;
            beam->flight_steps = 0;
            beam->flags = 0;
            desc.step = NULL;
            desc.speed.x = desc.speed.y = desc.speed.z = 0.0F;
            desc.scale = 0.0F;
            desc.depth = -15.0F;
            desc.drag = 0.0F;
            desc.gravity = 0.0F;
            desc.frames = ball_frames;
            desc.rate = 1.0F;
            sprite_set(&beam->ball, origin, &desc, 0);
            task->draw = beam_draw;
        } else {
            task_destroy(task);
            task = NULL;
        }
    }
    return task;
}
