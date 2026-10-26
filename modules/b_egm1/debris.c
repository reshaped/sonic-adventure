/* debris: a piece for each node of a model. each is flung out from the
 * model's position, and falls leaving a trail */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm1/debris.h"
#include "b_egm1/util.h"
#include "draw/model.h"
#include "system/task.h"
#include "system/util.h"

/* the piece of one node */
struct debris_piece {
    NJS_MATRIX matrix; /* its node's matrix, without the translation */
    /* its node's model, or null for a piece that never draws */
    NJS_MODEL *model;
    NJS_VECTOR speed;
    NJS_VECTOR accel;
    struct angles spin; /* its angles' change each step */
    Uint16 delay; /* nothing reads it */
    NJS_POINT3 pos;
    struct angles ang; /* the draw doesn't read it */
};

/* the debris's record, its task's extra */
struct debris {
    NJS_TEXLIST *texlist;
    struct debris_piece *pieces;
    void (*trail)(NJS_POINT3 *pos, int steps);
    Sint32 steps; /* the steps it has flown */
    Uint16 count; /* its pieces, one for each node */
};

/* each piece's speed up as it is flung. it is data that nothing writes */
static Float egg_hornet_debris_rise = 1.0F;
/* added to each piece's y speed each step */
#define DEBRIS_GRAVITY (-0.04F)

/* add a piece for object's node. then add one for each node of its
 * children's and its later siblings' trees, in order. parent places
 * object's node, and each piece takes its node's place and turn from there.
 * *index counts the pieces. the first piece never draws, even where its node
 * has a model */
static void egg_hornet_debris_add_node(struct debris *debris, Uint16 *index,
                                       NJS_OBJECT *object, NJS_MATRIX *parent)
{
    NJS_MATRIX matrix;
    struct debris_piece *piece = &debris->pieces[*index];

    njSetMatrix(&matrix, parent);
    njTranslate(&matrix, object->pos[0], object->pos[1], object->pos[2]);
    njRotateXYZ(&matrix, object->ang[0], object->ang[1], object->ang[2]);
    /* a node without a model takes a piece that never draws */
    if ((piece->model = object->model) != NULL) {
        njGetTranslation(&matrix, &piece->pos);
        njSetMatrix(&piece->matrix, &matrix);
        matrix_clear_translation(&piece->matrix);
    }
    if (*index == 0) {
        piece->model = NULL;
    }
    ++*index;
    if (object->child != NULL) {
        egg_hornet_debris_add_node(debris, index, object->child, &matrix);
    }
    if (object->sibling != NULL) {
        egg_hornet_debris_add_node(debris, index, object->sibling, parent);
    }
}

/* add a piece for each node of config's object, at config's position and
 * angles, at twice its size */
static void egg_hornet_debris_add_tree(struct debris *debris, Uint16 *index,
                                       struct debris_config *config)
{
    NJS_MATRIX matrix;

    njUnitMatrix(&matrix);
    njTranslateV(&matrix, config->pos);
    matrix_turn_zxy(&matrix, config->ang);
    njScale(&matrix, 2.0F, 2.0F, 2.0F);
    egg_hornet_debris_add_node(debris, index, config->object, &matrix);
}

/* fling each piece out from config's position in x and z, and up, with a
 * random spin. a piece right above or below the position goes out along
 * x */
static void egg_hornet_debris_launch(struct debris *debris,
                                     struct debris_config *config)
{
    Uint16 i;
    NJS_VECTOR dir;
    struct debris_piece *piece;

    for (i = 0; i < debris->count; ++i) {
        piece = &debris->pieces[i];
        piece->accel.x = piece->accel.z = 0.0F;
        piece->accel.y = DEBRIS_GRAVITY;
        piece->spin.x = random_spin(config->spin);
        piece->spin.y = random_spin(config->spin);
        piece->spin.z = random_spin(config->spin);
        dir.x = piece->pos.x - config->pos->x;
        dir.y = 0.0F;
        dir.z = piece->pos.z - config->pos->z;
        if (njScalor(&dir) == 0.0F) {
            dir.x = 0.01F;
        }
        njUnitVector(&dir);
        piece->speed.x = dir.x * config->speed;
        piece->speed.y = egg_hornet_debris_rise;
        piece->speed.z = dir.z * config->speed;
        piece->delay = config->delay * i;
    }
}

static void egg_hornet_debris_init(struct task *task,
                                   struct debris_config *config)
{
    struct debris *debris = task->extra;
    /* @hack redundant: a matrix no code uses, which SHC still makes room for in
     * the frame (ADD #-64,R15 at 0x0c901fe6); without it, 35 of 39 words
     * differ */
    NJS_MATRIX matrix;

    debris->pieces = heap_alloc((Uint16)tree_count_nodes(config->object) *
                                sizeof(struct debris_piece));
    debris->texlist = config->texlist;
    debris->count = 0;
    debris->trail = config->trail;
    debris->steps = 0;
    egg_hornet_debris_add_tree(debris, &debris->count, config);
    egg_hornet_debris_launch(debris, config);
}

static void egg_hornet_debris_draw(struct task *task);

static void egg_hornet_debris_step(struct task *task)
{
    struct debris *debris = task->extra;
    Uint16 i;
    struct debris_piece *piece;
    NJS_POINT3 pos;
    Float top = 0.0F;

    ++debris->steps;
    for (i = 0; i < debris->count; ++i) {
        piece = &debris->pieces[i];
        if (piece->model != NULL) {
            piece->pos.x = piece->pos.x + piece->speed.x;
            piece->pos.y = piece->pos.y + piece->speed.y;
            piece->pos.z = piece->pos.z + piece->speed.z;
            piece->speed.x = piece->speed.x + piece->accel.x;
            piece->speed.y = piece->speed.y + piece->accel.y;
            piece->speed.z = piece->speed.z + piece->accel.z;
            piece->ang.x = piece->ang.x + piece->spin.x;
            piece->ang.y = piece->ang.y + piece->spin.y;
            piece->ang.z = piece->ang.z + piece->spin.z;
            VECTOR_COPY(&pos, &piece->pos);
            if (debris->trail != NULL) {
                debris->trail(&pos, debris->steps);
            }
            if (pos.y > top) {
                top = pos.y;
            }
        }
    }
    if (top <= 0.0F) {
        heap_free(debris->pieces);
        task_kill(task);
    } else {
        egg_hornet_debris_draw(task);
    }
}

static void egg_hornet_debris_draw(struct task *task)
{
    struct debris *debris = task->extra;
    Uint16 i;
    struct debris_piece *piece;

    njSetTexture(debris->texlist);
    for (i = 0; i < debris->count; ++i) {
        piece = &debris->pieces[i];
        if (piece->model != NULL) {
            njPushMatrix(NULL);
            njTranslateV(NULL, &piece->pos);
            njMultiMatrix(NULL, &piece->matrix);
            draw_model(piece->model);
            njPopMatrix(1);
        }
    }
}

void egg_hornet_debris_create(struct debris_config *config)
{
    struct task *task;
    struct debris *debris;

    if ((task = task_create(0, 3, egg_hornet_debris_step)) != NULL) {
        debris = heap_calloc(1, sizeof(struct debris));
        task->extra = debris;
        egg_hornet_debris_init(task, config);
        task->step = egg_hornet_debris_step;
        task->draw = egg_hornet_debris_draw;
    }
}
