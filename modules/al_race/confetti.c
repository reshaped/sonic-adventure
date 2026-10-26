/* the confetti the crackers fire: pieces that rise, then drift down to land */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_race/butterfly.h"
#include "al_race/confetti.h"
#include "al_race/cracker.h"
#include "al_race/game.h"
#include "al_race/streamer.h"
#include "al_race/texlists.h"
#include "draw/tree.h"
#include "system/task.h"
#include "system/util.h"

/* the five shapes a piece can take */
extern NJS_OBJECT dat_0cbc7044;
extern NJS_OBJECT dat_0cbc7148;
extern NJS_OBJECT dat_0cbc7a08;
extern NJS_OBJECT dat_0cbc7b0c;
extern NJS_OBJECT dat_0cbc724c;

/* a piece's modes: a step's wait, its rise, and its fall once its rise
 * stops */
enum piece_mode { PIECE_START, PIECE_RISE, PIECE_FALL };

struct piece {
    int mode;
    int unk_04;
    NJS_OBJECT *object;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    NJS_VECTOR drift; /* added to speed each step of the fall */
    struct angles ang;
};

/* the confetti task's record, at its extra */
struct confetti {
    Sint16 count; /* the pieces in flight */
    Sint16 unk_02;
    /* slot numbers. free[count] and those after it are free, and the next
     * piece takes free[count] */
    Sint16 *free;
    Sint16 *flying; /* flying[0] to flying[count - 1], the slots in flight */
    struct piece *pieces; /* CONFETTI_COUNT pieces, by slot */
};

/* confetti_debug_draw would print the pieces in flight and cracker_on at
 * DEBUG_AT, each value DEBUG_VALUE right of its label, while fun_0cb8b7ea
 * returns DEBUG_MODE. fun_0cb8b7ea returns only 0 or -1, so it never
 * prints */
#define DEBUG_MODE 6
#define DEBUG_AT (NJM_LOCATION(60, 15))
#define DEBUG_VALUE (NJM_LOCATION(12, 0))

/* each step of its rise and fall, a piece's y speed gains PIECE_GRAVITY, and
 * the piece keeps PIECE_DRAG of its speed. a falling piece falls no faster than
 * PIECE_FALL_MAX, and lands once its y is at PIECE_GROUND or below */
#define PIECE_GRAVITY (-0.005F)
#define PIECE_DRAG 0.97F
#define PIECE_FALL_MAX (-0.2F)
#define PIECE_GROUND 4.0F

/* a piece's first angle about each axis is up to half PIECE_TILT either
 * way. PIECE_TILT is in Angle units, and half of it is 2.5 degrees. each
 * step the piece turns PIECE_TURN_X, PIECE_TURN_Y and PIECE_TURN_Z about x,
 * y and z: 10, 5 and 1 degrees */
#define PIECE_TILT 910.0F
#define PIECE_TURN_X 0x71c
#define PIECE_TURN_Y 0x38e
#define PIECE_TURN_Z 0xb6

static void confetti_destroy(struct task *task);
static void confetti_piece_draw(struct piece *piece);
static void confetti_draw(struct task *task);
static void confetti_remove(int index);
static Bool confetti_piece_move(struct piece *piece);
static void confetti_step(struct task *task);
static void confetti_debug_draw(void);

static NJS_OBJECT *const piece_objects[5] = {
    &dat_0cbc7044, &dat_0cbc7148, &dat_0cbc7a08, &dat_0cbc7b0c, &dat_0cbc724c,
};
/* the drifts a falling piece draws from. each axis of its drift comes from
 * an entry of its own, at random, and half the entries are zero */
static const NJS_VECTOR piece_drifts[8] = {
    {0.0F, 0.0F, 0.0F},       {0.0F, 0.0F, 0.0F},        {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},       {0.0021F, 0.0F, -0.0021F}, {0.0F, 0.0F, 0.003F},
    {0.0021F, 0.0F, 0.0021F}, {0.0F, 0.0F, -0.003F},
};

static struct task *confetti_task = NULL;

void confetti_create(NJS_POINT3 *pos, NJS_VECTOR *speed, int count)
{
    NJS_VECTOR way;
    Float gain = 0.8F / count; /* what each piece adds to share */
    /* both the chance that a piece is slowed and the part of its speed it
     * keeps then. it starts at 0.2, and the pieces stop once it reaches 1 */
    Float share = 0.2F;

    do {
        way.x = speed->x + (rand() * (0.4F * RAND_SCALE)) - 0.2F;
        way.y = speed->y + (rand() * (0.4F * RAND_SCALE)) - 0.2F;
        way.z = speed->z + (rand() * (0.4F * RAND_SCALE)) - 0.2F;
        if (rand() * RAND_SCALE < share) {
            /* @hack spelling: way's members through &way, whose address SHC
             * computes again for each read and write, R15 plus 8
             * (0x0cb9601e to 0x0cb96046), as the image does. as plain
             * members, the slowing runs 16 bytes short and 48 of 104 words
             * differ */
            (&way)->x = (&way)->x * share;
            (&way)->y = (&way)->y * share;
            (&way)->z = (&way)->z * share;
        }
        confetti_add(pos, &way);
        share += gain;
    } while (share < 1.0F);
}

void confetti_task_create(void)
{
    struct task *task;
    struct confetti *confetti;
    int i;

    if (confetti_task != NULL) {
        return;
    }
    if ((task = task_create(0, 4, confetti_step)) != NULL) {
        confetti = heap_calloc(sizeof(struct confetti), 1);
        if (confetti == NULL) {
            goto fail;
        }
        confetti->count = 0;
        confetti->free = heap_alloc(CONFETTI_COUNT * sizeof(Sint16));
        confetti->flying = heap_alloc(CONFETTI_COUNT * sizeof(Sint16));
        confetti->pieces = heap_alloc(CONFETTI_COUNT * sizeof(struct piece));
        task->extra = confetti;
        if (confetti->free == NULL || confetti->flying == NULL ||
            confetti->pieces == NULL) {
            goto fail;
        }
        for (i = 0; i < CONFETTI_COUNT; i++) {
            confetti->free[i] = i;
        }
        streamers_init();
        butterflies_init();
        task->draw = confetti_draw;
        task->destructor = confetti_destroy;
    }
    confetti_task = task;
    return;
fail:
    if (confetti != NULL) {
        if (confetti->free != NULL) {
            heap_free(confetti->free);
        }
        if (confetti->flying != NULL) {
            heap_free(confetti->flying);
        }
        if (confetti->pieces != NULL) {
            heap_free(confetti->pieces);
        }
    }
    if (task != NULL) {
        task_destroy(task);
    }
}

/* the record itself goes with the task, which frees its extra */
static void confetti_destroy(struct task *task)
{
    struct confetti *confetti;

    if (confetti_task != NULL) {
        confetti = confetti_task->extra;
        heap_free(confetti->free);
        heap_free(confetti->flying);
        heap_free(confetti->pieces);
        confetti_task = NULL;
    }
}

static void confetti_piece_draw(struct piece *piece)
{
    Angle turn;

    njPushMatrix(NULL);
    njTranslateV(NULL, &piece->pos);
    TURN_VIA(njRotateZ, piece->ang.z, turn);
    TURN_VIA(njRotateY, piece->ang.y, turn);
    TURN_VIA(njRotateX, piece->ang.x, turn);
    njSetTexture(&dat_0cbc089c);
    draw_tree(piece->object);
    njPopMatrix(1);
}

static void confetti_draw(struct task *task)
{
    struct confetti *confetti = task->extra;
    int i;

    for (i = 0; i < confetti->count; i++) {
        confetti_piece_draw(&confetti->pieces[confetti->flying[i]]);
    }
    confetti_debug_draw();
}

void confetti_add(NJS_POINT3 *pos, NJS_VECTOR *speed)
{
    struct confetti *confetti;
    struct piece *piece;
    Sint16 slot;
    Sint16 count;

    if (confetti_task == NULL) {
        return;
    }
    confetti = confetti_task->extra;
    count = confetti->count;
    if (count >= CONFETTI_COUNT) {
        return;
    }
    slot = confetti->free[count];
    confetti->flying[count] = slot;
    piece = &confetti->pieces[slot];
    piece->mode = PIECE_START;
    piece->unk_04 = 0;
    piece->object = piece_objects[rand() % 5];
    piece->pos.x = pos->x;
    piece->pos.y = pos->y;
    piece->pos.z = pos->z;
    piece->speed.x = speed->x;
    piece->speed.y = speed->y;
    piece->speed.z = speed->z;
    piece->drift.x = piece_drifts[rand() % 8].x;
    piece->drift.y = piece_drifts[rand() % 8].y;
    piece->drift.z = piece_drifts[rand() % 8].z;
    piece->ang.x = ((rand() * RAND_SCALE) - 0.5F) * PIECE_TILT;
    piece->ang.y = ((rand() * RAND_SCALE) - 0.5F) * PIECE_TILT;
    piece->ang.z = ((rand() * RAND_SCALE) - 0.5F) * PIECE_TILT;
    confetti->count++;
}

/* free the slot flying[index] holds, and move the last of flying into its
 * place */
static void confetti_remove(int index)
{
    struct confetti *confetti;
    Sint16 count;

    if (confetti_task != NULL) {
        confetti = confetti_task->extra;
        if (confetti->count <= 0) {
            return;
        }
        count = --confetti->count;
        confetti->free[count] = confetti->flying[index];
        confetti->flying[index] = confetti->flying[count];
    }
}

/* move and turn the piece a step. FALSE once it has landed, else TRUE */
static Bool confetti_piece_move(struct piece *piece)
{
    switch (piece->mode) {
    case PIECE_START:
        piece->mode++;
        break;
    case PIECE_RISE:
        piece->speed.y += PIECE_GRAVITY;
        piece->pos.x = piece->pos.x + piece->speed.x;
        piece->pos.y = piece->pos.y + piece->speed.y;
        piece->pos.z = piece->pos.z + piece->speed.z;
        piece->speed.x = piece->speed.x * PIECE_DRAG;
        piece->speed.y = piece->speed.y * PIECE_DRAG;
        piece->speed.z = piece->speed.z * PIECE_DRAG;
        if (!(piece->speed.y > 0.0F)) {
            piece->mode++;
        }
        break;
    case PIECE_FALL:
        piece->speed.y += PIECE_GRAVITY;
        piece->speed.x = piece->speed.x + piece->drift.x;
        piece->speed.z = piece->speed.z + piece->drift.z;
        piece->pos.x = piece->pos.x + piece->speed.x;
        piece->pos.y = piece->pos.y + piece->speed.y;
        piece->pos.z = piece->pos.z + piece->speed.z;
        piece->speed.x = piece->speed.x * PIECE_DRAG;
        piece->speed.y = piece->speed.y * PIECE_DRAG;
        piece->speed.z = piece->speed.z * PIECE_DRAG;
        if (!(piece->speed.y > PIECE_FALL_MAX)) {
            piece->speed.y = PIECE_FALL_MAX;
        }
        if (piece->pos.y > PIECE_GROUND) {
            break;
        }
        return FALSE;
    }
    piece->ang.x += PIECE_TURN_X;
    piece->ang.y += PIECE_TURN_Y;
    piece->ang.z += PIECE_TURN_Z;
    return TRUE;
}

static void confetti_step(struct task *task)
{
    struct confetti *confetti = task->extra;
    int i;

    for (i = 0; i < confetti->count; i++) {
        if (!confetti_piece_move(&confetti->pieces[confetti->flying[i]])) {
            confetti_remove(i--);
        }
    }
    confetti_draw(task);
}

static void confetti_debug_draw(void)
{
    struct confetti *confetti;
    Sint32 at;

    if (fun_0cb8b7ea() == DEBUG_MODE) {
        confetti = confetti_task->extra;
        at = DEBUG_AT;
        njPrintSize(8);
        njPrintC(at, "ConfettiNum");
        njPrintD(at++ + DEBUG_VALUE, confetti->count, 4);
        njPrintC(at, "flagCracker");
        njPrintC(at + DEBUG_VALUE, cracker_on() ? "TRUE" : "FALSE");
        njPrintSize(16);
    }
}
