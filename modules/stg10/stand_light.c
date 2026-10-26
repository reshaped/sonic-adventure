/* O Stand Light lamp on a stand that shines a beam */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "effect/beam.h"
#include "object/common.h"
#include "stg10/stand_light.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the beam's length is scale y, and its width at its end scale z. a scale
 * below BEAM_MIN takes BEAM_LENGTH or BEAM_WIDTH instead */
#define BEAM_MIN 0.1F
#define BEAM_LENGTH 28.0F
#define BEAM_WIDTH 3.0F
/* the beam tilts about x by the arm's tip less BEAM_TIP. the draw adds
 * BEAM_TIP less the tip, since its half turn about y reverses the tilt */
#define BEAM_TIP NJM_DEG_ANG(5)

/* the light's view of its task_work */
struct stand_light_work {
    Sint8 mode;
    Uint8 unk_01;
    Uint8 id;
    Uint8 unk_03;
    Sint16 flags;
    Uint8 unk_06[2];
    Angle tip; /* the arm's tilt about x: scale x, in degrees */
    /* a copy of the arm's model less its last meshset, or NULL when the heap
     * had no room. the light draws the whole arm then */
    NJS_MODEL *arm;
    Uint8 unk_10[4];
    struct angles ang;
    NJS_POINT3 pos;
    NJS_POINT3 scl;
    struct collision_work *collision;
    struct event_work *event;
};

static void stand_light_run(struct task *task);
static void stand_light_draw(struct task *task);
static void stand_light_destroy(struct task *task);

static const struct beam_texture stand_light_beam_texture = {
#include "stg10/stand_light/stand_light_beam_texture.inc"
};

static NJS_MATERIAL stand_light_arm_materials[5] = {
#include "stg10/stand_light/stand_light_arm_materials.inc"
};

static Sint16 stand_light_arm_meshes0[7] = {
#include "stg10/stand_light/stand_light_arm_meshes0.inc"
};

static Sint16 stand_light_arm_meshes1[30] = {
#include "stg10/stand_light/stand_light_arm_meshes1.inc"
};

static Sint16 stand_light_arm_meshes2[30] = {
#include "stg10/stand_light/stand_light_arm_meshes2.inc"
};

static Sint16 stand_light_arm_meshes3[7] = {
#include "stg10/stand_light/stand_light_arm_meshes3.inc"
};

static Sint16 stand_light_arm_meshes4[10] = {
#include "stg10/stand_light/stand_light_arm_meshes4.inc"
};

static NJS_COLOR stand_light_arm_uvs0[6] = {
#include "stg10/stand_light/stand_light_arm_uvs0.inc"
};

static NJS_COLOR stand_light_arm_uvs1[24] = {
#include "stg10/stand_light/stand_light_arm_uvs1.inc"
};

static NJS_COLOR stand_light_arm_uvs2[24] = {
#include "stg10/stand_light/stand_light_arm_uvs2.inc"
};

static NJS_COLOR stand_light_arm_uvs3[6] = {
#include "stg10/stand_light/stand_light_arm_uvs3.inc"
};

static NJS_COLOR stand_light_arm_uvs4[8] = {
#include "stg10/stand_light/stand_light_arm_uvs4.inc"
};

static NJS_MESHSET stand_light_arm_meshsets[5] = {
#include "stg10/stand_light/stand_light_arm_meshsets.inc"
};

static NJS_POINT3 stand_light_arm_points[26] = {
#include "stg10/stand_light/stand_light_arm_points.inc"
};

static NJS_VECTOR stand_light_arm_normals[26] = {
#include "stg10/stand_light/stand_light_arm_normals.inc"
};

static NJS_MODEL stand_light_arm_model = {
#include "stg10/stand_light/stand_light_arm_model.inc"
};

static NJS_OBJECT stand_light_arm_object = {
#include "stg10/stand_light/stand_light_arm_object.inc"
};

static NJS_MATERIAL stand_light_materials[6] = {
#include "stg10/stand_light/stand_light_materials.inc"
};

static Sint16 stand_light_meshes0[27] = {
#include "stg10/stand_light/stand_light_meshes0.inc"
};

static Sint16 stand_light_meshes1[15] = {
#include "stg10/stand_light/stand_light_meshes1.inc"
};

static Sint16 stand_light_meshes2[12] = {
#include "stg10/stand_light/stand_light_meshes2.inc"
};

static Sint16 stand_light_meshes3[25] = {
#include "stg10/stand_light/stand_light_meshes3.inc"
};

static Sint16 stand_light_meshes4[9] = {
#include "stg10/stand_light/stand_light_meshes4.inc"
};

static Sint16 stand_light_meshes5[10] = {
#include "stg10/stand_light/stand_light_meshes5.inc"
};

static NJS_COLOR stand_light_uvs0[24] = {
#include "stg10/stand_light/stand_light_uvs0.inc"
};

static NJS_COLOR stand_light_uvs1[12] = {
#include "stg10/stand_light/stand_light_uvs1.inc"
};

static NJS_COLOR stand_light_uvs2[10] = {
#include "stg10/stand_light/stand_light_uvs2.inc"
};

static NJS_COLOR stand_light_uvs3[20] = {
#include "stg10/stand_light/stand_light_uvs3.inc"
};

static NJS_COLOR stand_light_uvs4[8] = {
#include "stg10/stand_light/stand_light_uvs4.inc"
};

static NJS_COLOR stand_light_uvs5[8] = {
#include "stg10/stand_light/stand_light_uvs5.inc"
};

static NJS_MESHSET stand_light_meshsets[6] = {
#include "stg10/stand_light/stand_light_meshsets.inc"
};

static NJS_POINT3 stand_light_points[37] = {
#include "stg10/stand_light/stand_light_points.inc"
};

static NJS_VECTOR stand_light_normals[37] = {
#include "stg10/stand_light/stand_light_normals.inc"
};

static NJS_MODEL stand_light_model = {
#include "stg10/stand_light/stand_light_model.inc"
};

/* the stand, its child the arm */
/* @hack type: stand_light_object as an array of one. SHC then pools its
 * address and reads its child at @(44,R3) (0x0c9151e6). as one NJS_OBJECT,
 * SHC pools the child's address, 26 of stand_light_step's 121 words differ,
 * and 9 of stand_light_draw's 136 */
static NJS_OBJECT stand_light_object[1] = {
#include "stg10/stand_light/stand_light_object.inc"
};

/* the stand, a cylinder 4 units across and 12 high */
static struct collision_shape stand_light_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 6.0F, 0.0F},
    2.0F,
    6.0F,
    0.0F,
    {0},
    0,
    NULL,
};

/* the beam's width at its start and at its end. the draw sets the end's from
 * scale z */
static Float stand_light_beam_widths[2] = {0.15F, BEAM_WIDTH};

void stand_light_step(struct task *task)
{
    struct stand_light_work *work = (struct stand_light_work *)task->work;

    /* the work must keep task_work's own fields where task_work does */
    /* @hack redundant: pos is tested twice, as the image's first and fifth
     * compares both test its offset (0x0c915172, 0x0c9151a4); tested once,
     * 90 of 121 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (!WORK_SAME_PLACE(struct stand_light_work, pos) ||
        !WORK_SAME_PLACE(struct stand_light_work, id) ||
        !WORK_SAME_PLACE(struct stand_light_work, collision) ||
        !WORK_SAME_PLACE(struct stand_light_work, event) ||
        !WORK_SAME_PLACE(struct stand_light_work, pos) ||
        !WORK_SAME_PLACE(struct stand_light_work, flags)) {
        return;
    }
    /* NOLINTEND(misc-redundant-expression) */
    work->tip = (Angle)(work->scl.x * ANGLE_PER_DEGREE);
    collision_init(task, &stand_light_shape, COLLISION_OBJECT, 1, 1);
    /* @hack spelling: the copy assigned in the test. SHC tests it in R0 (TST
     * R0,R0 at 0x0c9151dc), and stores it in the branch's delay slot.
     * assigned first, it is copied to R3 to be tested, the function runs 2
     * bytes long and 65 of 121 words differ */
    if ((work->arm = heap_alloc(sizeof(NJS_MODEL))) != NULL) {
        *work->arm = *stand_light_object->child->model;
        work->arm->nbMeshset--;
    }
    if (work->scl.y < BEAM_MIN) {
        work->scl.y = BEAM_LENGTH;
    }
    if (work->scl.z < BEAM_MIN) {
        work->scl.z = BEAM_WIDTH;
    }
    task->step = stand_light_run;
    task->draw = stand_light_draw;
    task->destructor = stand_light_destroy;
}

static void stand_light_run(struct task *task)
{
    struct stand_light_work *work = (struct stand_light_work *)task->work;

    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<-MODEL Ang.(DEG)");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<-Beam length");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<-Beam width");
        work->tip = (Angle)(work->scl.x * ANGLE_PER_DEGREE);
    }
    collision_register((struct task_work *)work);
    stand_light_draw(task);
}

/* where the beam starts, from the light's position. the draw adds it to the
 * position without turning it, so it doesn't follow the light's angle y */
static const NJS_POINT3 stand_light_beam_start = {0.5F, 11.15F, 0.0F};

/* the beam adds to the color behind it */
static void stand_light_draw(struct task *task)
{
    struct stand_light_work *work = (struct stand_light_work *)task->work;
    NJS_OBJECT *object = stand_light_object;
    NJS_POINT3 pos;
    struct angles ang;

    if (frame_step != 0) {
        return;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_model(object->model);
    object = object->child;
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    njRotateXYZ(NULL, work->tip + object->ang[0], object->ang[1],
                object->ang[2]);
    if (work->arm != NULL) {
        draw_model(work->arm);
    } else {
        draw_model(object->model);
    }
    njPopMatrix(1);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    pos = stand_light_beam_start;
    njAddVector(&pos, &work->pos);
    ang = work->ang;
    /* beam_draw_ray_textured shines along -z, and the arm reaches along +z */
    ang.y += 0x8000;
    ang.x += BEAM_TIP - work->tip;
    stand_light_beam_widths[1] = work->scl.z;
    /* the texture is const, though beam_draw_ray_textured takes a plain
     * pointer */
    beam_draw_ray_textured(&pos, &ang, stand_light_beam_widths,
                           (struct beam_texture *)&stand_light_beam_texture,
                           work->scl.y);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
}

static void stand_light_destroy(struct task *task)
{
    struct stand_light_work *work = (struct stand_light_work *)task->work;

    if (work->arm != NULL) {
        heap_free(work->arm);
    }
}
