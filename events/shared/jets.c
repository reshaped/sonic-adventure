/* the Egg Hornet's jets in the events: a flame at its tail and one under
 * each wing. each flame has a glow, and the flames and glows turn through
 * their textures on timers */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/tree.h"
#include "shared/hornet.h"
#include "shared/jets.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 11
#include "shared/jets/text.inc"

/* how many flame models the jets stretch, and how many glows they draw */
#define JET_FLAMES 5
#define JET_GLOWS 3
/* the textures in each of the two texlists, which every flame and glow turns
 * through */
#define JET_TEXTURES 3
/* what scale and stretch change by in a step */
#define JET_RATE 0.05F
/* the scale above which the flames draw their glows */
#define JET_GLOW_SCALE 0.3F
/* the glow sprite's scale on each axis */
#define JET_GLOW_SIZE 0.3F
/* where the flames sit on the hornet. the tail's is on its center line, and
 * each wing's JET_WING_X to a side */
#define JET_TAIL_Y 9.0F
#define JET_TAIL_Z (-44.0F)
#define JET_WING_X 14.0F
#define JET_WING_Y 2.0F
#define JET_WING_Z (-29.0F)
/* the attribute bits of a material's attr_texId, above its texture number */
#define TEXID_ATTR 0xe0000000

/* the jets' record, which their task's extra points to */
struct jets {
    /* the steps since each flame's texture last changed */
    Sint32 texture_timer[JET_FLAMES];
    /* the steps between a flame's textures, which jets_init leaves unset */
    Sint32 texture_interval[JET_FLAMES];
    Uint32 texture[JET_FLAMES]; /* each flame's texture in event_jet_texlist */
    Sint32 glow_timer[JET_GLOWS];
    Sint32 glow_interval[JET_GLOWS];
    Sint16 glow_frame[JET_GLOWS]; /* each glow's frame of jet_glow_anim */
    /* each flame's points and its long shape's, copied to the heap */
    NJS_POINT3 *short_points[JET_FLAMES];
    NJS_POINT3 *long_points[JET_FLAMES];
    /* each flame's point count, which its long shape shares */
    Uint32 counts[JET_FLAMES];
    struct task *hornet; /* the hornet the jets were made for */
    Float scale; /* from nothing at 0 to full size at 1 */
    Float stretch; /* from the short shapes at 0 to the long ones at 1 */
    /* the steps since scale and stretch came to where the mode takes them */
    Sint32 done_steps;
    enum jets_mode mode;
};

/* the glow's frames, one texture of event_jet_glow_texlist each */
static const NJS_TEXANIM jet_glow_anim[JET_TEXTURES] = {
#include "shared/jets/jet_glow_anim.inc"
};

/* each flame's first timer, interval and texture, and each glow's first
 * timer, interval and frame. jets_init copies them into the record, but
 * never reads jet_texture_intervals */
static const Sint32 jet_texture_timers[JET_FLAMES] = {
#include "shared/jets/jet_texture_timers.inc"
};
/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const Sint32 jet_texture_intervals[JET_FLAMES] = {
#include "shared/jets/jet_texture_intervals.inc"
};
static const Uint32 jet_textures[JET_FLAMES] = {
#include "shared/jets/jet_textures.inc"
};
static const Sint32 jet_glow_timers[JET_GLOWS] = {
#include "shared/jets/jet_glow_timers.inc"
};
static const Sint32 jet_glow_intervals[JET_GLOWS] = {
#include "shared/jets/jet_glow_intervals.inc"
};
static const Sint16 jet_glow_frames[JET_GLOWS] = {
#include "shared/jets/jet_glow_frames.inc"
};

/* the flame models, each in a short shape and a long one. the tail's flame
 * is a ring, a cone and a core, and each wing's is one model. each model
 * comes with a texlist of one texture, egm01_jet01, which no code reads */
static NJS_TEXNAME jet_ring_texnames[1] = {{TEXT_1, 0, 0}};

static NJS_TEXLIST jet_ring_texlist = {
#include "shared/jets/jet_ring_texlist.inc"
};

static NJS_MATERIAL jet_ring_materials[1] = {
#include "shared/jets/jet_ring_materials.inc"
};

static Sint16 jet_ring_meshes[40] = {
#include "shared/jets/jet_ring_meshes.inc"
};

static NJS_COLOR jet_ring_uvs[32] = {
#include "shared/jets/jet_ring_uvs.inc"
};

static NJS_MESHSET jet_ring_meshsets[1] = {
#include "shared/jets/jet_ring_meshsets.inc"
};

static NJS_POINT3 jet_ring_points[16] = {
#include "shared/jets/jet_ring_points.inc"
};

static NJS_VECTOR jet_ring_normals[16] = {
#include "shared/jets/jet_ring_normals.inc"
};

static NJS_MODEL jet_ring_model = {
#include "shared/jets/jet_ring_model.inc"
};

static NJS_OBJECT jet_ring_object = {
#include "shared/jets/jet_ring_object.inc"
};

static NJS_TEXNAME jet_cone_texnames[1] = {{TEXT_1, 0, 0}};

static NJS_TEXLIST jet_cone_texlist = {
#include "shared/jets/jet_cone_texlist.inc"
};

static NJS_MATERIAL jet_cone_materials[1] = {
#include "shared/jets/jet_cone_materials.inc"
};

static Sint16 jet_cone_meshes[19] = {
#include "shared/jets/jet_cone_meshes.inc"
};

static NJS_COLOR jet_cone_uvs[18] = {
#include "shared/jets/jet_cone_uvs.inc"
};

static NJS_MESHSET jet_cone_meshsets[1] = {
#include "shared/jets/jet_cone_meshsets.inc"
};

static NJS_POINT3 jet_cone_points[16] = {
#include "shared/jets/jet_cone_points.inc"
};

static NJS_VECTOR jet_cone_normals[16] = {
#include "shared/jets/jet_cone_normals.inc"
};

static NJS_MODEL jet_cone_model = {
#include "shared/jets/jet_cone_model.inc"
};

static NJS_OBJECT jet_cone_object = {
#include "shared/jets/jet_cone_object.inc"
};

static NJS_TEXNAME jet_plus_x_texnames[1] = {{TEXT_1, 0, 0}};

static NJS_TEXLIST jet_plus_x_texlist = {
#include "shared/jets/jet_plus_x_texlist.inc"
};

static NJS_MATERIAL jet_plus_x_materials[1] = {
#include "shared/jets/jet_plus_x_materials.inc"
};

static Sint16 jet_plus_x_meshes[30] = {
#include "shared/jets/jet_plus_x_meshes.inc"
};

static NJS_COLOR jet_plus_x_uvs[24] = {
#include "shared/jets/jet_plus_x_uvs.inc"
};

static NJS_MESHSET jet_plus_x_meshsets[1] = {
#include "shared/jets/jet_plus_x_meshsets.inc"
};

static NJS_POINT3 jet_plus_x_points[12] = {
#include "shared/jets/jet_plus_x_points.inc"
};

static NJS_VECTOR jet_plus_x_normals[12] = {
#include "shared/jets/jet_plus_x_normals.inc"
};

static NJS_MODEL jet_plus_x_model = {
#include "shared/jets/jet_plus_x_model.inc"
};

static NJS_OBJECT jet_plus_x_object = {
#include "shared/jets/jet_plus_x_object.inc"
};

static NJS_TEXNAME jet_minus_x_texnames[1] = {{TEXT_1, 0, 0}};

static NJS_TEXLIST jet_minus_x_texlist = {
#include "shared/jets/jet_minus_x_texlist.inc"
};

static NJS_MATERIAL jet_minus_x_materials[1] = {
#include "shared/jets/jet_minus_x_materials.inc"
};

static Sint16 jet_minus_x_meshes[30] = {
#include "shared/jets/jet_minus_x_meshes.inc"
};

static NJS_COLOR jet_minus_x_uvs[24] = {
#include "shared/jets/jet_minus_x_uvs.inc"
};

static NJS_MESHSET jet_minus_x_meshsets[1] = {
#include "shared/jets/jet_minus_x_meshsets.inc"
};

static NJS_POINT3 jet_minus_x_points[12] = {
#include "shared/jets/jet_minus_x_points.inc"
};

static NJS_VECTOR jet_minus_x_normals[12] = {
#include "shared/jets/jet_minus_x_normals.inc"
};

static NJS_MODEL jet_minus_x_model = {
#include "shared/jets/jet_minus_x_model.inc"
};

static NJS_OBJECT jet_minus_x_object = {
#include "shared/jets/jet_minus_x_object.inc"
};

static NJS_TEXNAME jet_core_texnames[1] = {{TEXT_1, 0, 0}};

static NJS_TEXLIST jet_core_texlist = {
#include "shared/jets/jet_core_texlist.inc"
};

static NJS_MATERIAL jet_core_materials[1] = {
#include "shared/jets/jet_core_materials.inc"
};

static Sint16 jet_core_meshes[19] = {
#include "shared/jets/jet_core_meshes.inc"
};

static NJS_COLOR jet_core_uvs[18] = {
#include "shared/jets/jet_core_uvs.inc"
};

static NJS_MESHSET jet_core_meshsets[1] = {
#include "shared/jets/jet_core_meshsets.inc"
};

static NJS_POINT3 jet_core_points[16] = {
#include "shared/jets/jet_core_points.inc"
};

static NJS_VECTOR jet_core_normals[16] = {
#include "shared/jets/jet_core_normals.inc"
};

static NJS_MODEL jet_core_model = {
#include "shared/jets/jet_core_model.inc"
};

static NJS_OBJECT jet_core_object = {
#include "shared/jets/jet_core_object.inc"
};

static NJS_TEXNAME jet_ring_long_texnames[1] = {{TEXT_1, 0, 0}};

static NJS_TEXLIST jet_ring_long_texlist = {
#include "shared/jets/jet_ring_long_texlist.inc"
};

static NJS_MATERIAL jet_ring_long_materials[1] = {
#include "shared/jets/jet_ring_long_materials.inc"
};

static Sint16 jet_ring_long_meshes[40] = {
#include "shared/jets/jet_ring_long_meshes.inc"
};

static NJS_COLOR jet_ring_long_uvs[32] = {
#include "shared/jets/jet_ring_long_uvs.inc"
};

static NJS_MESHSET jet_ring_long_meshsets[1] = {
#include "shared/jets/jet_ring_long_meshsets.inc"
};

static NJS_POINT3 jet_ring_long_points[16] = {
#include "shared/jets/jet_ring_long_points.inc"
};

static NJS_VECTOR jet_ring_long_normals[16] = {
#include "shared/jets/jet_ring_long_normals.inc"
};

static NJS_MODEL jet_ring_long_model = {
#include "shared/jets/jet_ring_long_model.inc"
};

static NJS_OBJECT jet_ring_long_object = {
#include "shared/jets/jet_ring_long_object.inc"
};

static NJS_TEXNAME jet_cone_long_texnames[1] = {{TEXT_1, 0, 0}};

static NJS_TEXLIST jet_cone_long_texlist = {
#include "shared/jets/jet_cone_long_texlist.inc"
};

static NJS_MATERIAL jet_cone_long_materials[1] = {
#include "shared/jets/jet_cone_long_materials.inc"
};

static Sint16 jet_cone_long_meshes[19] = {
#include "shared/jets/jet_cone_long_meshes.inc"
};

static NJS_COLOR jet_cone_long_uvs[18] = {
#include "shared/jets/jet_cone_long_uvs.inc"
};

static NJS_MESHSET jet_cone_long_meshsets[1] = {
#include "shared/jets/jet_cone_long_meshsets.inc"
};

static NJS_POINT3 jet_cone_long_points[16] = {
#include "shared/jets/jet_cone_long_points.inc"
};

static NJS_VECTOR jet_cone_long_normals[16] = {
#include "shared/jets/jet_cone_long_normals.inc"
};

static NJS_MODEL jet_cone_long_model = {
#include "shared/jets/jet_cone_long_model.inc"
};

static NJS_OBJECT jet_cone_long_object = {
#include "shared/jets/jet_cone_long_object.inc"
};

static NJS_TEXNAME jet_plus_x_long_texnames[1] = {{TEXT_1, 0, 0}};

static NJS_TEXLIST jet_plus_x_long_texlist = {
#include "shared/jets/jet_plus_x_long_texlist.inc"
};

static NJS_MATERIAL jet_plus_x_long_materials[1] = {
#include "shared/jets/jet_plus_x_long_materials.inc"
};

static Sint16 jet_plus_x_long_meshes[30] = {
#include "shared/jets/jet_plus_x_long_meshes.inc"
};

static NJS_COLOR jet_plus_x_long_uvs[24] = {
#include "shared/jets/jet_plus_x_long_uvs.inc"
};

static NJS_MESHSET jet_plus_x_long_meshsets[1] = {
#include "shared/jets/jet_plus_x_long_meshsets.inc"
};

static NJS_POINT3 jet_plus_x_long_points[12] = {
#include "shared/jets/jet_plus_x_long_points.inc"
};

static NJS_VECTOR jet_plus_x_long_normals[12] = {
#include "shared/jets/jet_plus_x_long_normals.inc"
};

static NJS_MODEL jet_plus_x_long_model = {
#include "shared/jets/jet_plus_x_long_model.inc"
};

static NJS_OBJECT jet_plus_x_long_object = {
#include "shared/jets/jet_plus_x_long_object.inc"
};

static NJS_TEXNAME jet_minus_x_long_texnames[1] = {{TEXT_1, 0, 0}};

static NJS_TEXLIST jet_minus_x_long_texlist = {
#include "shared/jets/jet_minus_x_long_texlist.inc"
};

static NJS_MATERIAL jet_minus_x_long_materials[1] = {
#include "shared/jets/jet_minus_x_long_materials.inc"
};

static Sint16 jet_minus_x_long_meshes[30] = {
#include "shared/jets/jet_minus_x_long_meshes.inc"
};

static NJS_COLOR jet_minus_x_long_uvs[24] = {
#include "shared/jets/jet_minus_x_long_uvs.inc"
};

static NJS_MESHSET jet_minus_x_long_meshsets[1] = {
#include "shared/jets/jet_minus_x_long_meshsets.inc"
};

static NJS_POINT3 jet_minus_x_long_points[12] = {
#include "shared/jets/jet_minus_x_long_points.inc"
};

static NJS_VECTOR jet_minus_x_long_normals[12] = {
#include "shared/jets/jet_minus_x_long_normals.inc"
};

static NJS_MODEL jet_minus_x_long_model = {
#include "shared/jets/jet_minus_x_long_model.inc"
};

static NJS_OBJECT jet_minus_x_long_object = {
#include "shared/jets/jet_minus_x_long_object.inc"
};

static NJS_TEXNAME jet_core_long_texnames[1] = {{TEXT_1, 0, 0}};

static NJS_TEXLIST jet_core_long_texlist = {
#include "shared/jets/jet_core_long_texlist.inc"
};

static NJS_MATERIAL jet_core_long_materials[1] = {
#include "shared/jets/jet_core_long_materials.inc"
};

static Sint16 jet_core_long_meshes[19] = {
#include "shared/jets/jet_core_long_meshes.inc"
};

static NJS_COLOR jet_core_long_uvs[18] = {
#include "shared/jets/jet_core_long_uvs.inc"
};

static NJS_MESHSET jet_core_long_meshsets[1] = {
#include "shared/jets/jet_core_long_meshsets.inc"
};

static NJS_POINT3 jet_core_long_points[16] = {
#include "shared/jets/jet_core_long_points.inc"
};

static NJS_VECTOR jet_core_long_normals[16] = {
#include "shared/jets/jet_core_long_normals.inc"
};

static NJS_MODEL jet_core_long_model = {
#include "shared/jets/jet_core_long_model.inc"
};

static NJS_OBJECT jet_core_long_object = {
#include "shared/jets/jet_core_long_object.inc"
};

/* the jets event_jets_create made last. event_jets_kill destroys them and
 * leaves this set */
static struct task *jets_task;

/* event_jet_texlist's and event_jet_glow_texlist's textures, which the
 * event loads */
static NJS_TEXNAME jet_texnames[JET_TEXTURES];
static NJS_TEXNAME jet_glow_texnames[JET_TEXTURES];

/* the sprite every glow draws */
static NJS_SPRITE jet_glow;

/* each flame model, and the long shape jets_stretch moves it toward */
static NJS_OBJECT *jet_objects[JET_FLAMES] = {
    &jet_ring_object, &jet_cone_object, &jet_plus_x_object, &jet_minus_x_object,
    &jet_core_object};
static NJS_OBJECT *jet_long_objects[JET_FLAMES] = {
    &jet_ring_long_object, &jet_cone_long_object, &jet_plus_x_long_object,
    &jet_minus_x_long_object, &jet_core_long_object};

NJS_TEXLIST event_jet_texlist = {
#include "shared/jets/event_jet_texlist.inc"
};
NJS_TEXLIST event_jet_glow_texlist = {
#include "shared/jets/event_jet_glow_texlist.inc"
};

static void jets_draw(struct task *task);

/* copy each flame's points and its long shape's to blocks from the heap.
 * no allocation is checked */
static void jets_copy_points(struct jets *jets)
{
    int i;
    Uint32 count;
    NJS_POINT3 *from;
    NJS_POINT3 *to;

    for (i = 0; i < JET_FLAMES; i++) {
        count = jet_objects[i]->model->nbPoint;
        jets->counts[i] = count;
        from = jet_objects[i]->model->points;
        to = jets->short_points[i] = heap_alloc(count * sizeof(NJS_POINT3));
        while (count > 0) {
            to->x = from->x;
            to->y = from->y;
            to->z = from->z;
            from++;
            to++;
            count--;
        }
        count = jet_long_objects[i]->model->nbPoint;
        from = jet_long_objects[i]->model->points;
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

    for (i = 0; i < JET_FLAMES; i++) {
        count = jets->counts[i];
        to = jet_objects[i]->model->points;
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
 * long shape's, by stretch. the loop means to move every point */
static void jets_stretch(struct jets *jets)
{
    int i;
    Uint32 j;
    NJS_POINT3 *to;
    NJS_POINT3 *from;
    NJS_POINT3 *toward;
    Float stretch = jets->stretch;

    for (i = 0; i < JET_FLAMES; i++) {
        from = jets->short_points[i];
        toward = jets->long_points[i];
        to = jet_objects[i]->model->points;
        /* @bug logic: to, from and toward never move on, so each pass
         * writes the model's first point, and the others keep their short
         * shape. R7, R14 and R5 hold them unchanged from 0x0cb83792 to
         * 0x0cb837bc in EV0007 */
        for (j = 0; j < jets->counts[i]; j++) {
            to->x = (from->x * (1.0F - stretch)) + (toward->x * stretch);
            to->y = (from->y * (1.0F - stretch)) + (toward->y * stretch);
            to->z = (from->z * (1.0F - stretch)) + (toward->z * stretch);
        }
    }
}

/* count each flame's and each glow's timer up a step. a timer that reaches
 * its interval goes back to 0, and the next texture shows */
static void jets_flicker(struct jets *jets)
{
    int i;

    for (i = JET_FLAMES - 1; i >= 0; i--) {
        jets->texture_timer[i]++;
        if (jets->texture_timer[i] >= jets->texture_interval[i]) {
            jets->texture_timer[i] = 0;
            jets->texture[i] = (jets->texture[i] + 1) % JET_TEXTURES;
        }
    }
    for (i = JET_GLOWS - 1; i >= 0; i--) {
        jets->glow_timer[i]++;
        if (jets->glow_timer[i] >= jets->glow_interval[i]) {
            jets->glow_timer[i] = 0;
            jets->glow_frame[i] = (jets->glow_frame[i] + 1) % JET_TEXTURES;
        }
    }
}

static void jets_set_textures(struct jets *jets)
{
    int i;

    for (i = 0; i < JET_FLAMES; i++) {
        NJS_MATERIAL *material = jet_objects[i]->model->mats;

        material->attr_texId =
            (material->attr_texId & TEXID_ATTR) | jets->texture[i];
    }
}

static void jets_init(struct task *task, struct task **hornet)
{
    struct jets *jets = task->extra;
    int i;

    jets->hornet = *hornet;
    /* @bug uninitialized: each flame changes texture at an interval the heap
     * left. this loop stores the flame's timer twice and its interval never
     * (0x0cb839e6 and 0x0cb839f0 in EV0007), and jets_flicker compares the
     * timer with texture_interval as heap_alloc's block held it (MOV.L
     * @R2,R1 at 0x0cb8380a) */
    for (i = 0; i < JET_FLAMES; i++) {
        jets->texture_timer[i] = jet_texture_timers[i];
        jets->texture_timer[i] = jet_texture_timers[i];
        jets->texture[i] = jet_textures[i];
    }
    for (i = 0; i < JET_GLOWS; i++) {
        jets->glow_timer[i] = jet_glow_timers[i];
        jets->glow_interval[i] = jet_glow_intervals[i];
        jets->glow_frame[i] = jet_glow_frames[i];
    }
    jet_glow.p.x = jet_glow.p.y = jet_glow.p.z = 0.0F;
    jet_glow.sx = jet_glow.sy = JET_GLOW_SIZE;
    jet_glow.ang = 0;
    jet_glow.tlist = &event_jet_glow_texlist;
    jet_glow.tanim = (NJS_TEXANIM *)jet_glow_anim;
    jets_copy_points(jets);
    jets->scale = 1.0F;
    jets->stretch = 0.0F;
    jets->done_steps = 0;
    jets->mode = JETS_HOLD;
}

static void jets_step(struct task *task)
{
    struct jets *jets = task->extra;

    if (jets->hornet != NULL) {
        switch (jets->mode) {
        case JETS_OFF:
            if (jets->stretch > 0.0F) {
                jets->stretch -= JET_RATE;
            } else {
                jets->stretch = 0.0F;
                if (jets->scale > 0.0F) {
                    jets->scale -= JET_RATE;
                } else {
                    jets->scale = 0.0F;
                    jets->done_steps++;
                }
            }
            break;
        case JETS_ON:
            if (jets->scale < 1.0F) {
                jets->scale += JET_RATE;
                jets->stretch = 0.0F;
            } else if (jets->stretch > 0.0F) {
                jets->stretch -= JET_RATE;
                jets->scale = 1.0F;
            } else {
                jets->done_steps++;
                jets->stretch = 0.0F;
                jets->scale = 1.0F;
            }
            break;
        case JETS_BOOST:
            if (jets->scale < 1.0F) {
                jets->scale += JET_RATE;
            } else {
                jets->scale = 1.0F;
                if (jets->stretch < 1.0F) {
                    jets->stretch += JET_RATE;
                } else {
                    jets->done_steps++;
                }
            }
            break;
        default:
            break;
        }
        jets_stretch(jets);
        jets_flicker(jets);
        jets_set_textures(jets);
        jets_draw(task);
    }
}

/* draw the flames and their glows at the hornet's tail and wings, as it
 * bobs, heads and banks */
static void jets_draw(struct task *task)
{
    struct task *hornet = event_hornet_task;
    struct jets *jets;
    struct task_work *work;
    struct task_work *body;
    struct hornet_flight *flight;
    Angle ang;
    /* @hack redundant: 64 bytes of frame that no code reads, of a type unknown,
     * declared as a matrix. the image's ADD #-68,R15 reserves them (0x0cb83bfc
     * in EV0007); without them, it reserves 4 and 2 of 219 words differ */
    NJS_MATRIX unused;
    Float wing_y;
    Float wing_z;

    if (hornet == NULL) {
        return;
    }
    jets = task->extra;
    /* the hornet the jets were made for places them, and the flight of
     * event_hornet_task's hornet banks them */
    work = hornet->work;
    flight = HORNET(work)->flight;
    /* @hack spelling: body through task->extra, not jets, which loads the
     * hornet's task into R12 from R14 (MOV.L @(R0,R14),R12 at 0x0cb83c12 in
     * EV0007); without it, 21 of 219 words differ */
    body = ((struct jets *)task->extra)->hornet->work;
    if (flight == NULL) {
        return;
    }
    njPushMatrix(NULL);
    njTranslate(NULL, body->pos.x,
                body->pos.y +
                    njSin((Angle)(HORNET(body)->bob * ANGLE_PER_DEGREE)),
                body->pos.z);
    TURN_VIA(njRotateY, body->ang.y, ang);
    TURN_VIA(njRotateZ, flight->bank, ang);
    /* @hack spelling: the wing flames' y and z through locals, which SHC
     * keeps in FR14 and FR15 across the calls (0x0cb83c70 and 0x0cb83c74 in
     * EV0007); without them, 29 of 219 words differ */
    wing_y = JET_WING_Y;
    wing_z = JET_WING_Z;
    if (jets->scale > 0.0F) {
        njSetTexture(&event_jet_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, JET_TAIL_Y, JET_TAIL_Z);
        njScale(NULL, jets->scale, jets->scale, jets->scale);
        draw_tree(&jet_ring_object);
        draw_tree(&jet_cone_object);
        draw_tree(&jet_core_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, JET_WING_X, wing_y, wing_z);
        njScale(NULL, jets->scale, jets->scale, jets->scale);
        draw_tree(&jet_plus_x_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, -JET_WING_X, wing_y, wing_z);
        njScale(NULL, jets->scale, jets->scale, jets->scale);
        draw_tree(&jet_minus_x_object);
        njPopMatrix(1);
    }
    if (jets->scale > JET_GLOW_SCALE) {
        njSetTexture(&event_jet_glow_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, JET_TAIL_Y, JET_TAIL_Z);
        njDrawSprite3D(&jet_glow, jets->glow_frame[0],
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, JET_WING_X, wing_y, wing_z);
        njDrawSprite3D(&jet_glow, jets->glow_frame[1],
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE);
        njPopMatrix(1);
        njTranslate(NULL, -JET_WING_X, wing_y, wing_z);
        njDrawSprite3D(&jet_glow, jets->glow_frame[2],
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE);
    }
    njPopMatrix(1);
}

static void jets_destroy(struct task *task)
{
    struct jets *jets = task->extra;
    int i;

    jets_restore_points(jets);
    for (i = 0; i < JET_FLAMES; i++) {
        heap_free(jets->short_points[i]);
        heap_free(jets->long_points[i]);
    }
}

void event_jets_create(struct task **hornet)
{
    struct task *task;

    jets_task = task = task_create(TASK_EXTRA, 3, jets_step);
    if (jets_task != NULL) {
        struct jets *jets = heap_alloc(sizeof(struct jets));

        /* @bug memory: leaks the TASK_EXTRA record task_create allocated.
         * the jets' record takes its place in extra (MOV.L R0,@(44,R14) at
         * 0x0cb83e6c in EV0007), and task_destroy frees only the record
         * there */
        task->extra = jets;
        jets_init(task, hornet);
        task->step = jets_step;
        task->draw = jets_draw;
        task->destructor = jets_destroy;
    }
}

void event_jets_kill(void)
{
    if (jets_task != NULL) {
        task_destroy(jets_task);
    }
}

void fun_0cb83e9c(void)
{
}

void event_jets_set_mode(enum jets_mode mode)
{
    struct jets *jets = jets_task->extra;

    jets->done_steps = 0;
    jets->mode = mode;
}

int event_jets_done_steps(void)
{
    struct jets *jets = jets_task->extra;

    return jets->done_steps;
}
