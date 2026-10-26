/* the Chao race hall's sign, whose face shows each of two textures in
 * turn */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_race/scenery.h"
#include "al_race/sign.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"

/* the sign's object and its face's material */
extern NJS_OBJECT dat_0cbc8994;
extern NJS_MATERIAL dat_0cbc8570;

/* a face of the sign: its material, and its texture for each side */
struct sign_face {
    NJS_MATERIAL *material;
    Uint16 textures[2];
};

/* what sign_start takes: the sign's object, and its count faces */
struct sign_kind {
    NJS_OBJECT *object;
    const struct sign_face *faces;
    Sint16 count;
};

/* the sign's state, in its task's motion. object, faces and count are the
 * sign_kind's */
struct sign {
    Sint16 side; /* the side shown, 0 or 1, which picks each face's texture */
    Sint16 timer; /* steps until the other side shows */
    NJS_OBJECT *object;
    const struct sign_face *faces;
    Sint16 count;
};

#define SIGN(task) ((struct sign *)(task)->motion)

/* the steps each side of the sign shows */
#define SIDE_STEPS 36

static void sign_start(struct task *task, void *arg);
static void sign_step(struct task *task);
static void sign_draw(struct task *task);
static void sign_show(struct sign *sign);

static const struct task_desc sign_desc = {
    sizeof(struct sign), TASK_WORK, sign_start, sign_step, sign_draw, NULL};
static const struct sign_face sign_faces[1] = {
    {&dat_0cbc8570, {34, 35}},
};
static const struct sign_kind sign_kind = {&dat_0cbc8994, sign_faces, 1};

void sign_create(void)
{
    scenery_add(&sign_desc, (void *)&sign_kind);
}

static void sign_start(struct task *task, void *arg)
{
    struct task_work *work = task->work;
    struct sign *sign = SIGN(task);
    const struct sign_kind *kind = arg;
    NJS_OBJECT *object = kind->object;

    scenery_place(work, object);
    sign->side = 0;
    sign->timer = SIDE_STEPS;
    sign->object = object;
    sign->faces = kind->faces;
    sign->count = kind->count;
    sign_show(sign);
}

static void sign_step(struct task *task)
{
    struct sign *sign;

    sign_draw(task);
    sign = SIGN(task);
    sign->timer--;
    if (sign->timer <= 0) {
        sign->timer = SIDE_STEPS;
        sign->side++;
        if (sign->side >= 2) {
            sign->side = 0;
        }
        sign_show(sign);
    }
}

static void sign_draw(struct task *task)
{
    struct sign *sign;

    if (frame_step == 0 && !placement_far_from_camera(task)) {
        sign = SIGN(task);
        texture_set_stage();
        draw_tree(sign->object);
    }
}

static void sign_show(struct sign *sign)
{
    int count = sign->count;
    const struct sign_face *face;
    int i;

    for (i = 0, face = sign->faces; i < count; i++, face++) {
        face->material->attr_texId = face->textures[sign->side];
    }
}
