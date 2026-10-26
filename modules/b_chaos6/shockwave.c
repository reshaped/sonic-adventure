/* Chaos 6's shockwave: a ring that spreads over the deck, a band drawn
 * twelve times round, each a twelfth of a turn on. its texture flashes
 * between c6_shock01 and c6_shock02. a ball keeps to the ring's edge on the
 * line to player 0, and hurts him */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos6/shockwave.h"
#include "b_chaos6/textures.h"
#include "collision/collision.h"
#include "draw/tree.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 10
#include "b_chaos6/shockwave/text.inc"

/* the steps the ring's task runs, the last of which kills it, and how far
 * the ring spreads a step. SHOCK_HEIGHT, the band's scale on y, is the same
 * 399 / 71 */
#define SHOCK_STEPS 71
#define SHOCK_SPREAD (399.0F / SHOCK_STEPS)
#define SHOCK_HEIGHT (399.0F / SHOCK_STEPS)
/* the ring's height, 8 above the deck at 744, and how far behind its
 * creator's pos the ring's center starts */
#define SHOCK_Y 752.0F
#define SHOCK_BEHIND 15.0F
#define SE_SHOCK 390

/* c6_shock01's index in chaos6_effect_texlist. c6_shock02 follows it */
#define SHOCK_TEXTURE 18
/* the band draws SHOCK_COPIES times, each SHOCK_TURN, a twelfth of a turn,
 * further round */
#define SHOCK_COPIES 12
#define SHOCK_TURN 0x1555

struct shockwave_vars {
    Uint32 age; /* the steps its task has run, this one included */
};

#define VARS(work) ((struct shockwave_vars *)&(work)->vars)

static void shockwave_run(struct task *task);
static void shockwave_draw(struct task *task);

/* a texlist of c6_shock02 alone, which no code reads. the band draws from
 * chaos6_effect_texlist */
static NJS_TEXNAME shock_textures[1] = {{TEXT_1, 0, 0}};
static NJS_TEXLIST shock_texlist = {
#include "b_chaos6/shockwave/shock_texlist.inc"
};

/* the ring's model: a band of radius 5 and height 10 round the y axis */
static NJS_MATERIAL shock_materials[1] = {
#include "b_chaos6/shockwave/shock_materials.inc"
};

static Sint16 shock_strips[35] = {
#include "b_chaos6/shockwave/shock_strips.inc"
};

static NJS_COLOR shock_uvs[34] = {
#include "b_chaos6/shockwave/shock_uvs.inc"
};

static NJS_MESHSET shock_meshsets[1] = {
#include "b_chaos6/shockwave/shock_meshsets.inc"
};

static NJS_POINT3 shock_points[32] = {
#include "b_chaos6/shockwave/shock_points.inc"
};

static NJS_VECTOR shock_normals[32] = {
#include "b_chaos6/shockwave/shock_normals.inc"
};

static NJS_MODEL shock_model = {
#include "b_chaos6/shockwave/shock_model.inc"
};

static NJS_OBJECT shock_object = {
#include "b_chaos6/shockwave/shock_object.inc"
};

/* a ball of radius SHOCK_SPREAD, placed in world coordinates, that hurts
 * players. each step sets its center on the ring's edge, on the line from
 * the ring's center to player 0 */
static struct collision_shape shock_shape[1] = {
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_GUARD | SHAPE_ATTACK,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     SHOCK_SPREAD,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* a quad the shape of a twelfth of the band, and its colors: four red,
 * then four yellow. no code draws it, though shockwave_draw points it at
 * one set or the other as the ring flashes */
static NJS_POINT3 quad_points[4] = {
#include "b_chaos6/shockwave/quad_points.inc"
};
static NJS_COLOR quad_colors[8] = {
#include "b_chaos6/shockwave/quad_colors.inc"
};
static NJS_POINT3COL quad = {
#include "b_chaos6/shockwave/quad.inc"
};

void chaos6_shockwave_create(struct task_work *from)
{
    struct task *task;
    struct task_work *work;
    Float sine;
    Float cosine;

    if ((task = task_create(TASK_WORK, 6, shockwave_run)) != NULL) {
        work = task->work;
        _builtin_fsca(from->ang.y, &sine, &cosine);
        work->pos.x = from->pos.x - (cosine * SHOCK_BEHIND);
        work->pos.z = from->pos.z - (sine * SHOCK_BEHIND);
        work->pos.y = SHOCK_Y;
        work->scl.x = 1.0F;
        work->scl.y = SHOCK_HEIGHT;
        collision_init(task, shock_shape, COLLISION_ITEM, 1, 1);
        task->work->collision->radius = 1000.0F;
        task->draw = shockwave_draw;
    }
}

/* spread the ring, set its ball on the edge toward player 0, and play
 * SE_SHOCK. the task kills itself on its SHOCK_STEPS'th step */
static void shockwave_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR edge;
    NJS_POINT3 *player;

    if (++VARS(work)->age >= SHOCK_STEPS) {
        task_kill(task);
    } else {
        work->scl.z = work->scl.x += SHOCK_SPREAD;
        player = &player_work[0]->pos;
        edge.x = player->x - work->pos.x;
        edge.y = 0.0F;
        edge.z = player->z - work->pos.z;
        njUnitVector(&edge);
        edge.x *= work->scl.x;
        edge.z *= work->scl.x;
        edge.x += work->pos.x;
        edge.z += work->pos.z;
        edge.y = work->pos.y;
        work->collision->shapes->center = edge;
        collision_register(work);
        sound_play_at_for(SE_SHOCK, (int)work, 1, 127, work->pos.x, work->pos.y,
                          work->pos.z, 2);
        shockwave_draw(task);
    }
}

/* draw the band SHOCK_COPIES times, each a twelfth of a turn further
 * round. it is scaled to the ring's radius, scl.x, across, and by
 * SHOCK_HEIGHT on y. every two steps its texture flashes to the other of
 * c6_shock01 and c6_shock02, and the quad's colors with it. it draws only
 * on a frame's first step */
static void shockwave_draw(struct task *task)
{
    struct task_work *work;
    Angle ang;
    Angle turn;
    Float width;

    if (frame_step) {
        return;
    }
    work = task->work;
    /* @hack spelling: col stored through quad's address, which the pool holds
     * (0x0c90638c), at an offset of 4 (0x0c906396); without it, 17 of 240
     * words differ */
    (&quad)->col = &quad_colors[(VARS(work)->age & 2) * 2];
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    for (turn = 0; turn < SHOCK_COPIES * SHOCK_TURN; turn += SHOCK_TURN) {
        njPushMatrix(NULL);
        width = work->scl.x * 0.2F;
        TURN(njRotateY, turn);
        njScale(NULL, width, work->scl.y, width);
        njSetTexture(&chaos6_effect_texlist);
        shock_materials[0].attr_texId =
            ((VARS(work)->age & 2) >> 1) + SHOCK_TEXTURE;
        /* the model's bounding radius takes the ring's, for clipping */
        /* @hack spelling: r stored through shock_model's address, which the
         * pool holds (0x0c906422), at an offset of 36 (0x0c90642a); without it,
         * 141 of 240 words differ */
        (&shock_model)->r = work->scl.x;
        draw_tree(&shock_object);
        njPopMatrix(1);
    }
    /* @hack redundant: a never-run call of njSetTexture, which makes
     * njSetTexture's address worth a register, the one the image gives it: R8,
     * for both draws (0x0c90640c); without it, njScale's address takes R8,
     * njSetTexture's is loaded at each call, and 24 of 240 words differ */
    if (0) {
        njSetTexture(&chaos6_effect_texlist);
    }
    njPopMatrix(1);
}
