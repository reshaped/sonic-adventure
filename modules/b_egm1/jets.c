/* the Egg Hornet's jets: flames at its three nozzles that grow, shrink and
 * stretch as it boosts. the tail's flame is a ring, a cone and a core, and
 * each wing's is one model. each nozzle has a glow, which shows while the
 * flames are big enough */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm1/effect_textures.h"
#include "b_egm1/egg_hornet.h"
#include "b_egm1/ground.h"
#include "b_egm1/jets.h"
#include "draw/tree.h"
#include "system/task.h"

extern NJS_OBJECT jet_core_long_object;

/* the flame models and the glows. TEXTURE_FRAMES counts the textures in
 * each of the two texlists, which every flame and glow turns through */
#define FLAMES 5
#define GLOWS 3
#define TEXTURE_FRAMES 3

/* what scale and stretch change by in a step */
#define JETS_RATE 0.05F

/* the scale the flames must pass to draw their glows, and the glow
 * sprite's scale on each axis */
#define GLOW_THRESHOLD 0.3F
#define GLOW_SIZE 0.3F

/* the jets draw from the matrix of the boss's node 0. the tail's flame is on
 * its center line, and each wing's WING_X to a side */
#define JETS_NODE 0
#define TAIL_Y 9.0F
#define TAIL_Z (-44.0F)
#define WING_X 14.0F
#define WING_Y 2.0F
#define WING_Z (-29.0F)

/* the attribute bits of a material's attr_texId, above its texture number */
#define TEXID_ATTR 0xe0000000
#define GLOW_ATTR (NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA)

/* the jets' record, which their task's extra points to */
struct jets {
    /* the steps since each flame's texture last changed */
    int texture_timer[FLAMES];
    /* the steps between each flame's textures, which jets_init leaves 0 */
    int texture_interval[FLAMES];
    Uint32 texture[FLAMES]; /* each flame's texture in egm1jet_texlist */
    int glow_timer[GLOWS];
    int glow_interval[GLOWS];
    Sint16 glow_frame[GLOWS]; /* each glow's frame of glow_anims */
    /* each flame's points and its long shape's, copied to the heap */
    NJS_POINT3 *short_points[FLAMES];
    NJS_POINT3 *long_points[FLAMES];
    /* each flame's point count, which its long shape shares */
    Uint32 counts[FLAMES];
    struct task *hornet; /* the boss's task, whose node places the jets */
    Float scale; /* from nothing at 0 to full size at 1 */
    Float stretch; /* from the short shapes at 0 to the long ones at 1 */
    /* the steps since scale and stretch came to where the mode takes them */
    int done_steps;
    enum jets_mode mode;
};

/* nothing reads it. it starts at 0x0c905a40, the 8-byte boundary where
 * flight.c's constants end, so it may be flight.c's */
static const Float dat_0c905a40[16] = {
    0.05F, 0.5F,        0.1F, 0.625F,      0.15F, 0.8333334F, 0.7F, 1.0F,
    0.85F, 0.55555553F, 0.9F, 0.45454547F, 1.0F,  0.4F,       0.0F, 1.0F,
};

/* the glow's frames, one texture of egm1jetb_texlist each */
static const NJS_TEXANIM glow_anims[TEXTURE_FRAMES] = {
#include "b_egm1/jets/glow_anims.inc"
};

/* each flame's first timer, interval and texture, and each glow's first
 * timer, interval and frame. jets_init copies them into the record, all but
 * texture_interval_starts, which nothing reads */
static const int texture_timer_starts[FLAMES] = {
#include "b_egm1/jets/texture_timer_starts.inc"
};
static const int texture_interval_starts[FLAMES] = {
#include "b_egm1/jets/texture_interval_starts.inc"
};
static const Uint32 texture_starts[FLAMES] = {
#include "b_egm1/jets/texture_starts.inc"
};
static const int glow_timer_starts[GLOWS] = {
#include "b_egm1/jets/glow_timer_starts.inc"
};
static const int glow_interval_starts[GLOWS] = {
#include "b_egm1/jets/glow_interval_starts.inc"
};
static const Sint16 glow_frame_starts[GLOWS] = {
#include "b_egm1/jets/glow_frame_starts.inc"
};

/* each flame model, and the long shape jets_stretch moves it toward */
static NJS_OBJECT *flame_objects[FLAMES] = {
    &jet_ring_object,    &jet_cone_object, &jet_plus_x_object,
    &jet_minus_x_object, &jet_core_object,
};
static NJS_OBJECT *long_objects[FLAMES] = {
    &jet_ring_long_object,    &jet_cone_long_object, &jet_plus_x_long_object,
    &jet_minus_x_long_object, &jet_core_long_object,
};

/* the jets egg_hornet_jets_create made last. egg_hornet_jets_kill leaves
 * it set */
static struct task *jets_task;
/* egm1jet_texlist's and egm1jetb_texlist's textures, which
 * eggmobile1_pvms loads */
static NJS_TEXNAME egm1jet_textures[TEXTURE_FRAMES];
static NJS_TEXNAME egm1jetb_textures[TEXTURE_FRAMES];
NJS_TEXLIST egm1jet_texlist = {
#include "b_egm1/jets/egm1jet_texlist.inc"
};
NJS_TEXLIST egm1jetb_texlist = {
#include "b_egm1/jets/egm1jetb_texlist.inc"
};
/* the sprite every glow draws */
static NJS_SPRITE glow_sprite;

/* copy each flame's points and its long shape's to blocks from the heap.
 * no allocation is checked */
static void jets_copy_points(struct jets *jets)
{
    int i;
    Uint32 count;
    NJS_POINT3 *from;
    NJS_POINT3 *to;

    for (i = 0; i < FLAMES; i++) {
        count = flame_objects[i]->model->nbPoint;
        jets->counts[i] = count;
        from = flame_objects[i]->model->points;
        to = jets->short_points[i] = heap_alloc(count * sizeof(NJS_POINT3));
        while (count > 0) {
            to->x = from->x;
            to->y = from->y;
            to->z = from->z;
            from++;
            to++;
            count--;
        }
        count = long_objects[i]->model->nbPoint;
        from = long_objects[i]->model->points;
        to = jets->long_points[i] = heap_alloc(count * sizeof(NJS_POINT3));
        while (count > 0) {
            to->x = from->x;
            to->y = from->y;
            to->z = from->z;
            from++;
            to++;
            count--;
        }
    }
}

/* put each flame's points back as jets_copy_points copied them */
static void jets_restore_points(struct jets *jets)
{
    int i;
    Uint32 count;
    NJS_POINT3 *from;
    NJS_POINT3 *to;

    for (i = 0; i < FLAMES; i++) {
        count = jets->counts[i];
        to = flame_objects[i]->model->points;
        from = jets->short_points[i];
        while (count > 0) {
            to->x = from->x;
            to->y = from->y;
            to->z = from->z;
            from++;
            to++;
            count--;
        }
    }
}

/* move each flame model's first point from its short shape's toward its
 * long shape's, by stretch */
static void jets_stretch(struct jets *jets)
{
    int i;
    Uint32 j;
    Float stretch = jets->stretch;
    NJS_POINT3 *to;
    NJS_POINT3 *from;
    NJS_POINT3 *toward;

    for (i = 0; i < FLAMES; i++) {
        from = jets->short_points[i];
        toward = jets->long_points[i];
        to = flame_objects[i]->model->points;
        /* @bug logic: to, from and toward never advance, so each pass
         * writes the model's first point, and the others keep their short
         * shape. R7, R14 and R5 hold them unchanged from 0x0c903940 to
         * 0x0c90396a */
        for (j = 0; j < jets->counts[i]; j++) {
            to->x = (from->x * (1.0F - stretch)) + (toward->x * stretch);
            to->y = (from->y * (1.0F - stretch)) + (toward->y * stretch);
            to->z = (from->z * (1.0F - stretch)) + (toward->z * stretch);
        }
    }
}

/* count each flame's and each glow's timer up a step. a timer that reaches
 * its interval goes back to 0, and the next texture shows */
static void jets_turn_textures(struct jets *jets)
{
    int i;

    for (i = FLAMES - 1; i >= 0; i--) {
        jets->texture_timer[i]++;
        if (jets->texture_timer[i] >= jets->texture_interval[i]) {
            jets->texture_timer[i] = 0;
            jets->texture[i] = (jets->texture[i] + 1) % TEXTURE_FRAMES;
        }
    }
    for (i = GLOWS - 1; i >= 0; i--) {
        jets->glow_timer[i]++;
        if (jets->glow_timer[i] >= jets->glow_interval[i]) {
            jets->glow_timer[i] = 0;
            jets->glow_frame[i] = (jets->glow_frame[i] + 1) % TEXTURE_FRAMES;
        }
    }
}

static void jets_set_textures(struct jets *jets)
{
    int i;

    for (i = 0; i < FLAMES; i++) {
        NJS_MATERIAL *material = flame_objects[i]->model->mats;

        material->attr_texId =
            (material->attr_texId & TEXID_ATTR) | jets->texture[i];
    }
}

static void jets_init(struct task *task, struct task **hornet)
{
    struct jets *jets = task->extra;
    int i;

    jets->hornet = *hornet;
    /* each flame's timer is stored twice and its interval never.
     * texture_interval keeps the 0 heap_calloc cleared it to, so each
     * flame's texture changes every step. the two stores are at 0x0c903bea
     * and 0x0c903bf4 */
    for (i = 0; i < FLAMES; i++) {
        jets->texture_timer[i] = texture_timer_starts[i];
        jets->texture_timer[i] = texture_timer_starts[i];
        jets->texture[i] = texture_starts[i];
    }
    for (i = 0; i < GLOWS; i++) {
        jets->glow_timer[i] = glow_timer_starts[i];
        jets->glow_interval[i] = glow_interval_starts[i];
        jets->glow_frame[i] = glow_frame_starts[i];
    }
    glow_sprite.p.x = glow_sprite.p.y = glow_sprite.p.z = 0.0F;
    glow_sprite.sx = glow_sprite.sy = GLOW_SIZE;
    glow_sprite.ang = 0;
    glow_sprite.tlist = &egm1jetb_texlist;
    glow_sprite.tanim = (NJS_TEXANIM *)glow_anims;
    jets_copy_points(jets);
    jets->scale = 1.0F;
    jets->stretch = 0.0F;
    jets->done_steps = 0;
    jets->mode = JETS_HOLD;
}

static void jets_draw(struct task *task);

static void jets_step(struct task *task)
{
    struct jets *jets = task->extra;

    if (jets->hornet == NULL) {
        return;
    }
    switch (jets->mode) {
    case JETS_OFF:
        if (jets->stretch > 0.0F) {
            jets->stretch -= JETS_RATE;
        } else {
            jets->stretch = 0.0F;
            if (jets->scale > 0.0F) {
                jets->scale -= JETS_RATE;
            } else {
                jets->scale = 0.0F;
                jets->done_steps++;
            }
        }
        break;
    case JETS_ON:
        if (jets->scale < 1.0F) {
            jets->scale += JETS_RATE;
            jets->stretch = 0.0F;
        } else if (jets->stretch > 0.0F) {
            jets->stretch -= JETS_RATE;
            jets->scale = 1.0F;
        } else {
            jets->done_steps++;
            jets->stretch = 0.0F;
            jets->scale = 1.0F;
        }
        break;
    case JETS_BOOST:
        if (jets->scale < 1.0F) {
            jets->scale += JETS_RATE;
        } else {
            jets->scale = 1.0F;
            if (jets->stretch < 1.0F) {
                jets->stretch += JETS_RATE;
            } else {
                jets->done_steps++;
            }
        }
        break;
    default:
        break;
    }
    jets_stretch(jets);
    jets_turn_textures(jets);
    jets_set_textures(jets);
    jets_draw(task);
}

/* draw the flames and their glows at the boss's tail and wings */
static void jets_draw(struct task *task)
{
    struct jets *jets = task->extra;
    struct task *hornet = jets->hornet;
    /* @hack spelling: njPushMatrix called through push, which SHC keeps in R13
     * (MOV.L at 0x0c903ebc); called by name, 17 of 190 words differ */
    Bool (*push)(NJS_MATRIX *) = njPushMatrix;
    NJS_MATRIX matrix;

    /* @hack type: volatile on a discarded read of the boss's work through
     * hornet, which keeps the read (MOV.L @R3,R3 at 0x0c903ec2); as
     * (void)hornet->work, SHC drops the read, the function runs 6 bytes
     * shorter and 172 of 190 words differ */
    (void)((volatile struct task *)hornet)->work;
    push(NULL);
    egg_hornet_place(jets->hornet);
    egg_hornet_node_matrix(jets->hornet, JETS_NODE, &matrix);
    njSetMatrix(NULL, &matrix);
    if (jets->scale > 0.0F) {
        njSetTexture(&egm1jet_texlist);
        push(NULL);
        njTranslate(NULL, 0.0F, TAIL_Y, TAIL_Z);
        njScale(NULL, jets->scale, jets->scale, jets->scale);
        draw_tree(&jet_ring_object);
        draw_tree(&jet_cone_object);
        draw_tree(&jet_core_object);
        njPopMatrix(1);
        push(NULL);
        njTranslate(NULL, WING_X, WING_Y, WING_Z);
        njScale(NULL, jets->scale, jets->scale, jets->scale);
        draw_tree(&jet_plus_x_object);
        njPopMatrix(1);
        push(NULL);
        njTranslate(NULL, -WING_X, WING_Y, WING_Z);
        njScale(NULL, jets->scale, jets->scale, jets->scale);
        draw_tree(&jet_minus_x_object);
        njPopMatrix(1);
    }
    if (jets->scale > GLOW_THRESHOLD) {
        njSetTexture(&egm1jetb_texlist);
        push(NULL);
        njTranslate(NULL, 0.0F, TAIL_Y, TAIL_Z);
        njDrawSprite3D(&glow_sprite, jets->glow_frame[0], GLOW_ATTR);
        njPopMatrix(1);
        push(NULL);
        njTranslate(NULL, WING_X, WING_Y, WING_Z);
        njDrawSprite3D(&glow_sprite, jets->glow_frame[1], GLOW_ATTR);
        njPopMatrix(1);
        njTranslate(NULL, -WING_X, WING_Y, WING_Z);
        njDrawSprite3D(&glow_sprite, jets->glow_frame[2], GLOW_ATTR);
        njPopMatrix(1);
    } else {
        njPopMatrix(1);
    }
}

static void jets_destroy(struct task *task)
{
    struct jets *jets = task->extra;
    int i;

    jets_restore_points(jets);
    for (i = 0; i < FLAMES; i++) {
        heap_free(jets->short_points[i]);
        heap_free(jets->long_points[i]);
    }
}

void egg_hornet_jets_create(struct task **hornet)
{
    struct task *task;
    struct jets *jets;

    if ((jets_task = task = task_create(0, 3, jets_step)) != NULL) {
        jets = heap_calloc(1, sizeof(struct jets));
        task->extra = jets;
        jets_init(task, hornet);
        task->step = jets_step;
        task->draw = jets_draw;
        task->destructor = jets_destroy;
    }
}

void egg_hornet_jets_kill(void)
{
    task_kill(jets_task);
}

void fun_0c904188(void)
{
}

void egg_hornet_jets_set_mode(enum jets_mode mode)
{
    struct jets *jets = jets_task->extra;

    jets->done_steps = 0;
    jets->mode = mode;
}

Uint32 egg_hornet_jets_done_steps(void)
{
    struct jets *jets = jets_task->extra;

    return jets->done_steps;
}
