/* the beads of light that run along strands in a Chaos's body */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "chaos/chaos.h"
#include "chaos/models.h"
#include "chaos/ribbon.h"
#include "draw/pose.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the ribbon nodes ribbon_hidden holds */
#define RIBBON_NODES 64

/* the steps a bead swells in for and shrinks away for, and the sizes it
 * picks from */
#define GROW_STEPS 15
#define FADE_STEPS 8
#define BEAD_SIZES 16

/* reads x twice, so an x that calls rand draws two numbers */
#define ABS(x) ((x) < 0 ? -(x) : (x))

enum bead_mode {
    BEAD_INIT,
    BEAD_GROW, /* swelling in, for GROW_STEPS steps */
    BEAD_RUN, /* running on, until along reaches end */
    BEAD_FADE /* shrinking for FADE_STEPS steps, then it goes */
};

/* a bead, its task's extra record */
struct ribbon_bead {
    Sint8 mode;
    Uint8 unk_01;
    Sint8 strand; /* its strand's index */
    Sint8 timer; /* the steps left to swell in or to shrink */
    Uint8 unk_04[4];
    Float size; /* one of ribbon_sizes */
    Float swell; /* scales its sway, and with a wobble of 0.3, its size */
    Float sway; /* how far it sways off its path */
    Angle ang; /* the phase of its sway and its wobble */
    Angle spin; /* added to ang each step it runs */
    NJS_POINT3 *path; /* its strand's points */
    /* its place along path, in points. the point it rounds to is the first
     * of the three that place it */
    Float along;
    Float speed; /* added to along each step it swells in or runs */
    Float end; /* the along it runs to */
};

/* a bead's sprite: CHAOS_EFFECT's drop texture, drawn in ribbon_color */
static NJS_TEXANIM ribbon_texanim[1] = {
#include "chaos/ribbon/ribbon_texanim.inc"
};
NJS_ARGB ribbon_color = {
#include "chaos/ribbon/ribbon_color.inc"
};
/* the sizes a bead picks from at random */
static Float ribbon_sizes[BEAD_SIZES] = {
#include "chaos/ribbon/ribbon_sizes.inc"
};

Sint8 ribbon_hidden[RIBBON_NODES];
NJS_SPRITE ribbon_sprite;
static struct ribbon_node *ribbon_nodes;
static struct ribbon_strand *ribbon_strands;
static Sint8 ribbon_node_count;
static Sint8 ribbon_strand_count;
/* each ribbon node's frame, which ribbon_place_nodes and ribbon_update
 * set */
static NJS_MATRIX *ribbon_matrices;
/* the work and the record of the Chaos the ribbon runs in. the work is
 * unread, and no bead draws while the record has CHAOS_NO_BEADS */
static struct task_work *ribbon_work;
static struct chaos *ribbon_owner;
static Float ribbon_scale;
/* ribbon_rate is 6, and ribbon_extra_rate is what the events set. a call
 * starts fewer beads than their sum */
static Float ribbon_rate;
static Float ribbon_extra_rate;
static Sint32 dat_8c7ad3f4;
static NJS_MATRIX ribbon_node_matrix;
/* the phases of ribbon_update's and ribbon_spawn's sines */
static Angle ribbon_update_ang;
static Angle ribbon_spawn_ang;

/* a bead's step. the bead swells in as it starts along its strand, runs on
 * swaying, and shrinks away. it draws on a frame's first step, unless its
 * node is hidden or its Chaos has CHAOS_NO_BEADS */
static void ribbon_bead_step(struct task *task)
{
    struct ribbon_bead *bead = task->extra;
    int node = ribbon_strands[bead->strand].node;
    NJS_POINT3 pos;

    switch (bead->mode) {
    case BEAD_INIT: {
        /* @hack redundant: rand called through random, a register pointer,
         * which keeps rand's address in R14 and the bead in R13 (0x8c0f9a44,
         * 0x8c0f9a10); called by name, rand takes R12 and the bead R14, and
         * 76 of 412 words differ */
        register int (*random)(void) = rand;

        bead->along = -0.5F + (random() * (0.15F * RAND_SCALE));
        /* @hack spelling: the strand's last point as a pointer sum, which
         * folds the - 1 into the load's offset (ADD #-12,R0 at 0x8c0f9a92);
         * as path[count - 1], SHC takes 1 from the count first, ADD #-1,R0,
         * and 154 of 412 words differ */
        bead->speed =
            (0.08F + (random() * (0.18F * RAND_SCALE))) /
            _builtin_fabsf(
                (bead->path + ribbon_strands[bead->strand].count - 1)->y -
                bead->path[0].y +
                (bead->path + ribbon_strands[bead->strand].count - 1)->x -
                bead->path[0].x);
        bead->speed *= ribbon_scale;
        bead->end = (Float)(ribbon_strands[bead->strand].count - 3) + 1.25F;
        bead->timer = GROW_STEPS;
        bead->spin = (Angle)((0.2F + (random() * (0.8F * RAND_SCALE))) *
                             (65536.0F / 12.0F));
        bead->sway = 0.05F + (random() * (0.1F * RAND_SCALE));
        bead->size = ribbon_sizes[(int)(random() * (BEAD_SIZES * RAND_SCALE))];
        bead->swell = 0.1F;
        bead->mode = BEAD_GROW;
    }
        /* fall through */
    case BEAD_GROW:
        bead->along += bead->speed;
        bead->swell += 0.9F / GROW_STEPS;
        bead->timer--;
        if (bead->timer != 0) {
            break;
        }
        bead->mode = BEAD_RUN;
        break;
    case BEAD_RUN:
        bead->along += bead->speed;
        if (bead->along >= bead->end) {
            bead->timer = FADE_STEPS;
            bead->mode = BEAD_FADE;
        }
        bead->ang += bead->spin;
        bead->swell = 0.85F;
        break;
    case BEAD_FADE:
        bead->swell -= 0.9F / FADE_STEPS;
        bead->timer--;
        if (bead->timer != 0) {
            break;
        }
        task_kill(task);
        return;
    /* @hack redundant: the default case, which loads frame_step's address into
     * R2 (0x8c0f9c20); without it, R3, and 3 of 412 words differ */
    default:
        break;
    }

    if (frame_step) {
        return;
    }
    if (ribbon_owner->flags & CHAOS_NO_BEADS) {
        return;
    }
    if (ribbon_hidden[node]) {
        return;
    }
    /* pos: path's three points from the one along rounds to, weighed by
     * the quadratic b-spline's basis at part. part is along less that
     * point's index */
    {
        Float part;
        int index;
        NJS_POINT3 *knot;
        Float w0;
        Float w1;
        Float w2;

        /* @bug logic: index rounds along to the nearest point (FADD of
         * 0.5 and FTRC at 0x8c0f9c58, 0x8c0f9c5a), so part runs from -0.5 to
         * 0.5. the weights are the quadratic b-spline's for a part from 0 to
         * 1, so the bead jumps each time along passes a half, by an eighth of
         * the third difference of the four points there */
        /* @hack spelling: the two stores in one statement, which loads a pool
         * constant into FR7 (MOVA at 0x8c0f9c4a) before bead->along; as two
         * statements, bead->along loads first, and 7 of 412 words differ */
        part = bead->along, index = (int)(part + 0.5F);
        part -= (Float)index;
        w0 = 0.5F * (1.0F - part) * (1.0F - part);
        w1 = 0.5F + (part * (1.0F - part));
        w2 = 0.5F * part * part;
        /* @bug memory: end, the strand's count less 1.75 (ADD #-3
         * and FADD at 0x8c0f9ad0, 0x8c0f9adc), lets along pass count - 2.5.
         * the first knot is then path[count - 2], and the third reads
         * path[count], a point past the strand's last (FMOV.S @R4,FR1 at
         * 0x8c0f9c98) */
        knot = &bead->path[index];
        /* @hack implicit: knot stepped twice and read in one expression, which
         * SHC does left to right; sequenced, through a copy of knot and a
         * partial sum, 132 of 412 words differ */
#ifdef GCC
        {
            /* repairs the read the @bug memory tag above names: where the
             * third knot lies past the strand's last point, GCC takes that
             * last point. in each fight's data, path[count] is the first
             * point of the path that follows, and after the last path, the
             * strands' first 12 bytes */
            const NJS_POINT3 *third =
                index + 2 < ribbon_strands[bead->strand].count
                    ? &knot[2]
                    : &bead->path[ribbon_strands[bead->strand].count - 1];

            pos.x = (knot[0].x * w0) + (knot[1].x * w1) + (third->x * w2);
            pos.y = (knot[0].y * w0) + (knot[1].y * w1) + (third->y * w2);
            pos.z = (knot[0].z * w0) + (knot[1].z * w1) + (third->z * w2);
        }
#else
        /* NOLINTNEXTLINE(clang-diagnostic-unsequenced) */
        pos.x = (knot++->x * w0) + (knot++->x * w1) + (knot->x * w2);
        knot -= 2;
        /* @hack implicit: the same for y; sequenced, 117 of 412 words differ */
        /* NOLINTNEXTLINE(clang-diagnostic-unsequenced) */
        pos.y = (knot++->y * w0) + (knot++->y * w1) + (knot->y * w2);
        knot -= 2;
        /* @hack implicit: the same for z; as knot[0], knot[1] and knot[2] with
         * knot += 2 after, 113 of 412 words differ */
        /* NOLINTNEXTLINE(clang-diagnostic-unsequenced) */
        pos.z = (knot++->z * w0) + (knot++->z * w1) + (knot->z * w2);
#endif
    }
    {
        Float offset = bead->swell * bead->sway;
        Float sine = njSin(bead->ang);
        Float cosine = njCos(bead->ang);
        Float scale;

        pos.x += offset * sine;
        pos.z += offset * cosine;
        scale = ribbon_scale * bead->size;
        ribbon_sprite.sx = scale * (bead->swell + (0.3F * sine));
        ribbon_sprite.sy = scale * (bead->swell + (0.3F * cosine));
    }
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njSetConstantMaterial(&ribbon_color);
    njPushMatrix(&ribbon_matrices[ribbon_strands[bead->strand].node]);
    njTranslateV(NULL, &pos);
    njDrawSprite3D(&ribbon_sprite, 0,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
    njPopMatrix(1);
}

void ribbon_update(void)
{
    int count;
    int i;
    int index;
    struct task *task;
    struct ribbon_bead *bead;

    ribbon_update_ang += NJM_DEG_ANG(1);
    count = ABS((int)((ribbon_rate + ribbon_extra_rate) * rand() *
                      njSin(ribbon_update_ang) * RAND_SCALE));

    while (count > 0) {
        index = (int)(rand() * (Float)ribbon_strand_count * RAND_SCALE);
        task = task_create(0, 4, ribbon_bead_step);
        task->extra = heap_calloc(1, sizeof(struct ribbon_bead));
        if (task->extra == NULL || ribbon_strands[index].path == NULL) {
            task_kill(task);
        } else {
            bead = task->extra;
            bead->mode = BEAD_INIT;
            bead->path = ribbon_strands[index].path;
            bead->strand = index;
        }
        count--;
    }

    for (i = 0; i < ribbon_node_count; i++) {
        njPushMatrix(NULL);
        pose_get_matrix(ribbon_nodes[i].node, &ribbon_node_matrix);
        njMultiMatrix(NULL, &ribbon_node_matrix);
        njTranslateV(NULL, &ribbon_nodes[i].pos);
        TURN(njRotateX, ribbon_nodes[i].ang.x);
        TURN(njRotateY, ribbon_nodes[i].ang.y);
        TURN(njRotateZ, ribbon_nodes[i].ang.z);
        njGetMatrix(&ribbon_matrices[i]);
        njPopMatrix(1);
    }
}

void ribbon_spawn(void)
{
    int count;
    int index;
    struct task *task;
    struct ribbon_bead *bead;

    ribbon_spawn_ang += NJM_DEG_ANG(1);
    count = ABS((int)((ribbon_rate + ribbon_extra_rate) * rand() *
                      njSin(ribbon_spawn_ang) * RAND_SCALE));

    while (count > 0) {
        index = (int)(rand() * (Float)ribbon_strand_count * RAND_SCALE);
        task = task_create(0, 4, ribbon_bead_step);
        task->extra = heap_calloc(1, sizeof(struct ribbon_bead));
        if (task->extra == NULL || ribbon_strands[index].path == NULL) {
            task_kill(task);
        } else {
            bead = task->extra;
            bead->mode = BEAD_INIT;
            bead->path = ribbon_strands[index].path;
            bead->strand = index;
        }
        count--;
    }
}

void ribbon_place_nodes(void)
{
    int i;

    for (i = 0; i < ribbon_node_count; i++) {
        njPushMatrix(NULL);
        pose_get_matrix(ribbon_nodes[i].node, &ribbon_node_matrix);
        njMultiMatrix(NULL, &ribbon_node_matrix);
        njTranslateV(NULL, &ribbon_nodes[i].pos);
        TURN(njRotateX, ribbon_nodes[i].ang.x);
        TURN(njRotateY, ribbon_nodes[i].ang.y);
        TURN(njRotateZ, ribbon_nodes[i].ang.z);
        njGetMatrix(&ribbon_matrices[i]);
        njPopMatrix(1);
    }
}

void ribbon_init(struct task_work *work, struct chaos *owner,
                 struct ribbon *ribbon)
{
    int i;

    ribbon_work = work;
    ribbon_owner = owner;
    ribbon_sprite.tlist = &chaos_effect_texlist;
    ribbon_sprite.tanim = ribbon_texanim;
    ribbon_sprite.ang = 0;
    ribbon_sprite.p.x = 0.0F;
    ribbon_sprite.p.y = 0.0F;
    ribbon_sprite.p.z = 0.0F;
    ribbon_node_count = ribbon->node_count;
    ribbon_strand_count = ribbon->strand_count;
    ribbon_nodes = ribbon->nodes;
    ribbon_strands = ribbon->strands;
    if (ribbon->scale == 0.0F) {
        ribbon_scale = 1.0F;
    } else {
        ribbon_scale = ribbon->scale;
    }
    ribbon_matrices = heap_alloc(ribbon_node_count * sizeof(NJS_MATRIX));
    for (i = 0; i < RIBBON_NODES; i++) {
        ribbon_hidden[i] = 0;
    }
    ribbon_rate = 6.0F;
    ribbon_extra_rate = 0.0F;
}

void ribbon_free(void)
{
    heap_free(ribbon_matrices);
}

void ribbon_set_extra_rate(Float rate)
{
    ribbon_extra_rate = rate;
}
