/* a Chao's face: its eyes and mouth, where it looks, and its blink */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/chao.h"
#include "al_main/face.h"
#include "chao/chao.h"
#include "system/task.h"
#include "system/util.h"

/* a trait below TRAIT_LOW is low, one from TRAIT_HIGH up is high, and one
 * between is middling */
#define TRAIT_LOW (-35)
#define TRAIT_HIGH 35

/* a trait's level, which indexes the face tables */
enum trait_level { LEVEL_LOW, LEVEL_MIDDLING, LEVEL_HIGH };

/* how far the lid turns each step of a blink */
#define LID_SPEED 0xc00
/* the eyes' turn about x while other eyes show, but for their last step, and
 * always a Chaos Chao's */
#define EYE_SHUT 0xc000

/* the most the eyes turn each step about x, and about z */
#define EYE_TURN_X 256
#define EYE_TURN_Z 64

/* a blink's wait is a random count of steps below this */
#define BLINK_WAIT 100.0F

/* the textures of a Chaos Chao's eyes and mouth */
#define CHAOS_EYES 171
#define CHAOS_MOUTH 174

/* the texture of each kind of eyes, and the two of each kind of mouth,
 * one for each of its materials */
static Uint8 eye_textures[8] = {164, 165, 166, 167, 168, 169, 170, 171};
static Uint8 mouth_textures[7][2] = {
    {174, 174}, {174, 175}, {174, 178}, {174, 179},
    {176, 177}, {180, 181}, {174, 182},
};
/* a Chao's own face, by the levels of its agres, kind and curio.
 * face_eyes and face_mouths give its eyes and mouth, face_open_x its eyes'
 * turn about x, and face_slant_z their slant about z */
static Uint8 face_eyes[3][3][3] = {
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 2}, {0, 0, 2}},
    {{0, 0, 0}, {0, 0, 2}, {0, 2, 2}},
};
static Uint8 face_mouths[3][3][3] = {
    {{5, 0, 2}, {5, 5, 2}, {5, 0, 1}},
    {{0, 4, 4}, {0, 0, 0}, {0, 1, 1}},
    {{0, 4, 4}, {0, 2, 2}, {1, 1, 1}},
};
static Angle face_open_x[3][3][3] = {
    {{0, 0xe000, 0xe000}, {0, 0xe000, 0xe000}, {0xe000, 0xc000, 0xc000}},
    {{0xe000, 0xe000, 0xc000}, {0, 0xc000, 0xc000}, {0xe000, 0xc000, 0xc000}},
    {{0xe000, 0xe000, 0xc000}, {0, 0xc000, 0xc000}, {0xe000, 0xc000, 0xc000}},
};
static Angle face_slant_z[3][3][3] = {
    {{0xf000, 0xf800, 0}, {0xf000, 0xf800, 0}, {0xf800, 0, 0}},
    {{0x800, 0x800, 0x800}, {0, 0, 0}, {0, 0, 0}},
    {{0x1000, 0x1000, 0x1000}, {0x800, 0x800, 0x800}, {0, 0, 0}},
};

void alife_face_set_eyes(struct alife_chao *chao, int eyes, int steps)
{
    struct chao_face *face = &chao->face;

    face->other_eyes = eye_textures[eyes];
    face->eyes_steps = steps;
    face->flags &= ~FACE_FLAG_2;
}

void alife_face_set_mouth(struct alife_chao *chao, int mouth, int steps)
{
    struct chao_face *face = &chao->face;

    face->other_mouth_0 = mouth_textures[mouth][0];
    face->other_mouth_1 = mouth_textures[mouth][1];
    face->mouth_steps = steps;
}

static void face_set_rest_eyes(struct alife_chao *chao, int eyes)
{
    struct chao_face *face = &chao->face;

    face->eyes = eye_textures[eyes];
}

static void face_set_rest_mouth(struct alife_chao *chao, int mouth)
{
    struct chao_face *face = &chao->face;

    face->mouth_0 = mouth_textures[mouth][0];
    face->mouth_1 = mouth_textures[mouth][1];
}

void alife_face_look(struct alife_chao *chao, Angle x, Angle z, int steps)
{
    struct chao_face *face = &chao->face;

    face->look_x = x;
    face->look_z = z;
    face->look_steps = steps;
}

static void face_set_rest_look(struct alife_chao *chao, Angle x, Angle z)
{
    struct chao_face *face = &chao->face;

    face->rest_x = x;
    face->rest_z = z;
    /* @hack redundant: a return where control returns anyway, which keeps the
     * function out of line for alife_face_init's BSR at 0x0c9088ca; without
     * it, SHC inlines it there, and 5 of 5 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

void alife_face_init(struct task_work *work, struct alife_chao *chao)
{
    struct chao *record = chao->record;
    struct chao_face *face = &chao->face;
    Sint8 kind;
    Sint8 agres;
    Sint8 curio;
    int eyes;
    int mouth;
    Angle x;
    Angle z;

    if (record->kind < TRAIT_LOW) {
        kind = LEVEL_LOW;
    } else if (record->kind < TRAIT_HIGH) {
        kind = LEVEL_MIDDLING;
    } else {
        kind = LEVEL_HIGH;
    }
    if (record->agres < TRAIT_LOW) {
        agres = LEVEL_LOW;
    } else if (record->agres < TRAIT_HIGH) {
        agres = LEVEL_MIDDLING;
    } else {
        agres = LEVEL_HIGH;
    }
    if (record->curio < TRAIT_LOW) {
        curio = LEVEL_LOW;
    } else if (record->curio < TRAIT_HIGH) {
        curio = LEVEL_MIDDLING;
    } else {
        curio = LEVEL_HIGH;
    }
    eyes = face_eyes[agres][kind][curio];
    mouth = face_mouths[agres][kind][curio];
    z = face_slant_z[agres][kind][curio];
    x = face_open_x[agres][kind][curio];
    face->eye_z = z;
    face->eye_x = x;
    face->open_x = x;
    face_set_rest_eyes(chao, eyes);
    face_set_rest_mouth(chao, mouth);
    face_set_rest_look(chao, x, z);
    face->eyes_steps = -1;
    face->mouth_steps = -1;
    face->look_steps = -1;
}

static void face_show_eyes(struct task_work *work, struct alife_chao *chao)
{
    struct chao_face *face = &chao->face;

    if (face->eyes_steps < 0) {
        chao->shape.objects[NODE_EYE_0]->model->mats[0].attr_texId = face->eyes;
        chao->shape.objects[NODE_EYE_1]->model->mats[0].attr_texId = face->eyes;
        /* @hack redundant: a return where control returns anyway, which ends
         * the branch on an RTS of its own (0x0c908944); without it, 1 of 39
         * words differ */
        return;
    } else {
        face->eyes_steps--;
        chao->shape.objects[NODE_EYE_0]->model->mats[0].attr_texId =
            face->other_eyes;
        chao->shape.objects[NODE_EYE_1]->model->mats[0].attr_texId =
            face->other_eyes;
    }
}

static void face_show_mouth(struct task_work *work, struct alife_chao *chao)
{
    struct chao_face *face = &chao->face;

    if (face->mouth_steps < 0) {
        chao->shape.objects[NODE_MOUTH]->model->mats[0].attr_texId =
            face->mouth_0;
        chao->shape.objects[NODE_MOUTH]->model->mats[1].attr_texId =
            face->mouth_1;
    } else {
        face->mouth_steps--;
        chao->shape.objects[NODE_MOUTH]->model->mats[0].attr_texId =
            face->other_mouth_0;
        chao->shape.objects[NODE_MOUTH]->model->mats[1].attr_texId =
            face->other_mouth_1;
    }
}

/* blink_steps counts up past blink_wait. lid then swings to 0x4000 and
 * back to 0xc000 by LID_SPEED a step, and the wait restarts at a random
 * count below BLINK_WAIT. nothing outside this function reads lid, so the
 * blink turns no node. the eyes' turn about x is open_x, or EYE_SHUT while
 * eyes_steps is above 0 */
static void face_blink(struct task_work *work, struct alife_chao *chao)
{
    struct chao_face *face = &chao->face;

    if (!(face->flags & FACE_OPENING) && !(face->flags & FACE_CLOSING) &&
        face->blink_steps > face->blink_wait) {
        face->flags |= FACE_CLOSING;
    }
    if (face->flags & FACE_CLOSING) {
        face->lid += LID_SPEED;
        if ((Uint16)(face->lid + 0x8000) > 0xc000) {
            face->lid = 0x4000;
            face->flags |= FACE_OPENING;
            face->flags &= ~FACE_CLOSING;
        }
    }
    if (face->flags & FACE_OPENING) {
        face->lid -= LID_SPEED;
        if ((Uint16)(face->lid + 0x8000) < 0x4000) {
            face->lid = 0xc000;
            face->flags &= ~FACE_OPENING;
            face->blink_wait = (Uint8)(rand() * (BLINK_WAIT * RAND_SCALE));
            face->blink_steps = 0;
        }
    }
    face->blink_steps++;
    face->eye_x = face->open_x;
    if (face->eyes_steps > 0) {
        face->eye_x = EYE_SHUT;
    }
}

static void face_turn_eyes(struct task_work *work, struct alife_chao *chao)
{
    struct chao_face *face = &chao->face;

    /* @bug logic: the test reads mouth_steps where look_steps counts
     * the look down (MOV.L @(12,R14),R3 at 0x0c908a9a, and ADD #-1 at
     * 0x0c908ae4). the eyes turn toward the look while another mouth
     * shows, and back to rest while none does */
    if (face->mouth_steps < 0) {
        face->eye_z = angle_approach(face->eye_z, face->rest_z, EYE_TURN_Z);
        face->open_x = angle_approach(face->open_x, face->rest_x, EYE_TURN_X);
    } else {
        face->look_steps--;
        face->eye_z = angle_approach(face->eye_z, face->look_z, EYE_TURN_Z);
        face->open_x = angle_approach(face->open_x, face->look_x, EYE_TURN_X);
    }
}

/* step the eyes' turn and blink, then pose the eye nodes. the two slant
 * opposite ways about z */
static void face_move_eyes(struct task_work *work, struct alife_chao *chao)
{
    struct chao_face *face;
    NJS_OBJECT **objects;

    face_turn_eyes(work, chao);
    face_blink(work, chao);
    face = &chao->face;
    objects = chao->shape.objects;
    objects[NODE_EYE_0_TURN]->ang[0] = face->eye_x;
    objects[NODE_EYE_0_TURN]->ang[2] = face->eye_z;
    objects[NODE_EYE_1_TURN]->ang[0] = face->eye_x;
    objects[NODE_EYE_1_TURN]->ang[2] = -face->eye_z;
}

void alife_face_step(struct task_work *work, struct alife_chao *chao)
{
    struct chao_face *face = &chao->face;

    if (chao->record->type != CHAO_CHAOS) {
        face_move_eyes(work, chao);
        face_show_eyes(work, chao);
        face_show_mouth(work, chao);
    } else {
        /* a Chaos Chao keeps one face. its eyes stay at EYE_SHUT, and each
         * slants as the other does in face_move_eyes */
        chao->shape.objects[NODE_EYE_0]->model->mats[0].attr_texId = CHAOS_EYES;
        chao->shape.objects[NODE_EYE_1]->model->mats[0].attr_texId = CHAOS_EYES;
        chao->shape.objects[NODE_MOUTH]->model->mats[0].attr_texId =
            CHAOS_MOUTH;
        chao->shape.objects[NODE_MOUTH]->model->mats[1].attr_texId =
            CHAOS_MOUTH;
        face->eye_x = EYE_SHUT;
        /* @hack spelling: a bare scope that takes the face's address and
         * the nodes afresh, which SHC adds to chao again (0x0c908bd4 to
         * 0x0c908bda); through face, with the nodes read once, 26 of 74
         * words differ */
        {
            struct chao_face *turn = &chao->face;
            NJS_OBJECT **objects = chao->shape.objects;

            objects[NODE_EYE_0_TURN]->ang[0] = turn->eye_x;
            objects[NODE_EYE_0_TURN]->ang[2] = -turn->eye_z;
            objects[NODE_EYE_1_TURN]->ang[0] = turn->eye_x;
            objects[NODE_EYE_1_TURN]->ang[2] = turn->eye_z;
        }
    }
}
