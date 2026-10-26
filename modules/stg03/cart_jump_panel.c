/* the jump panel for carts, which the set editor names O JUMP PANEL 4 CART */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg03/cart.h"
#include "stg03/cart_jump_panel.h"
#include "stg03/shareobj_textures.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT cart_jump_panel_object;

/* a panel throws at scl.x, or at JUMP_SPEED where scl.x is below 0.1. it
 * throws up at scl.y times that, or at JUMP_RATE times it where scl.y is
 * below 0.1. its arrows scroll JUMP_SCROLL_PER_SPEED in v a step for each
 * unit of speed */
#define JUMP_SPEED 10.0F
#define JUMP_RATE 0.7F
#define JUMP_SCROLL_PER_SPEED 4.0F

/* the model's meshset of the arrows, and its uvs, which the scroll moves */
#define ARROW_MESHSET 3
#define ARROW_UVS 4
/* the scroll wraps past SCROLL_WRAP, a texture's span in v */
#define SCROLL_WRAP 256.0F

/* half a turn, in angle units */
#define HALF_TURN 0x8000

/* the panel's shapes. a hit on shapes 0 to JUMP_LAST_RAMP_SHAPE, the ramp's,
 * throws. a hit on the others doesn't */
#define JUMP_SHAPES 4
#define JUMP_LAST_RAMP_SHAPE 1

/* a panel's record, which the first word of its task's extra points to */
struct arrow_scroll {
    Uint8 unk_00[4];
    Float offset; /* the arrows' scroll in v */
};

#define SCROLL(task) (*(struct arrow_scroll **)(task)->extra)

/* a panel's work->mode. this file never sets JUMP_PANEL_STILL or
 * JUMP_PANEL_GONE */
enum jump_panel_mode {
    JUMP_PANEL_INIT,
    JUMP_PANEL_THROWING,
    JUMP_PANEL_STILL,
    JUMP_PANEL_GONE
};

static void cart_jump_panel_destroy(struct task *task);
static void cart_jump_panel_draw(struct task *task);
static struct arrow_scroll *cart_jump_panel_alloc(void);
static void cart_jump_panel_scroll(struct task *task);
static void cart_jump_panel_throw(struct task *task);

#define JUMP_BOX(y, z, size_x, size_y, size_z)                                 \
    {SHAPE_RECTANGLE,                                                          \
     0,                                                                        \
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),                                           \
     {0.0F, (y), (z)},                                                         \
     (size_x),                                                                 \
     (size_y),                                                                 \
     (size_z),                                                                 \
     {0},                                                                      \
     0,                                                                        \
     NULL}
static struct collision_shape cart_jump_panel_shapes[JUMP_SHAPES] = {
    JUMP_BOX(9.4F, 2.0F, 35.0F, 5.7F, 16.0F),
    JUMP_BOX(19.6F, -9.6F, 35.0F, 6.9F, 8.4F),
    JUMP_BOX(14.3F, -16.2F, 38.0F, 8.9F, 3.6F),
    JUMP_BOX(7.2F, -2.6F, 38.0F, 3.3F, 17.2F),
};
/* each arrow uv's v at a scroll of 0 */
static Sint16 cart_jump_panel_v[ARROW_UVS] = {
#include "stg03/cart_jump_panel/cart_jump_panel_v.inc"
};

/* the record of the panel whose step runs, which the scroll reads */
static struct arrow_scroll *cart_jump_panel_arrows;

void cart_jump_panel_step(struct task *task)
{
    struct task_work *work = task->work;

    cart_jump_panel_arrows = SCROLL(task);
    if (placement_editing(task)) {
        if (work->mode == JUMP_PANEL_INIT) {
            work->mode++;
            SCROLL(task) = cart_jump_panel_alloc();
            task->destructor = cart_jump_panel_destroy;
            task->draw = cart_jump_panel_draw;
        }
        cart_jump_panel_arrows = SCROLL(task);
        cart_jump_panel_draw(task);
        cart_jump_panel_scroll(task);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "->SPEED (default 10.f)");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "->JUMP RATE(default 0.7f)");
    } else {
        switch (work->mode) {
        case JUMP_PANEL_INIT:
            work->mode = JUMP_PANEL_THROWING;
            SCROLL(task) = cart_jump_panel_alloc();
            cart_jump_panel_arrows = SCROLL(task);
            task->destructor = cart_jump_panel_destroy;
            task->draw = cart_jump_panel_draw;
            collision_init(task, cart_jump_panel_shapes, COLLISION_OBJECT, 1,
                           JUMP_SHAPES);
            if (work->scl.x < 0.1F) {
                work->scl.x = JUMP_SPEED;
            }
            if (work->scl.y < 0.1F) {
                work->scl.y = JUMP_RATE;
            }
            break;
        case JUMP_PANEL_THROWING:
            cart_jump_panel_draw(task);
            cart_jump_panel_scroll(task);
            cart_jump_panel_throw(task);
            break;
        case JUMP_PANEL_STILL:
            cart_jump_panel_draw(task);
            break;
        case JUMP_PANEL_GONE:
        default:
            placement_kill(task);
            break;
        }
    }
}

static void cart_jump_panel_destroy(struct task *task)
{
    if (SCROLL(task) != NULL) {
        heap_free(SCROLL(task));
        SCROLL(task) = NULL;
    }
}

static void cart_jump_panel_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njSetTexture(&obj_shareobj_texlist);
    draw_tree(&cart_jump_panel_object);
    njPopMatrix(1);
}

/* a panel's record from the heap, cleared byte by byte */
static struct arrow_scroll *cart_jump_panel_alloc(void)
{
    struct arrow_scroll *scroll;
    /* @hack type: i, an int against sizeof's unsigned, which the NOLINT
     * below admits. the loop's cursor then starts as 0 plus the record, in R4
     * (0x0c913234, 0x0c913238); as a Uint32, R4 takes the record itself and
     * 16 of 23 words differ */
    int i;

    /* @hack redundant: the empty test, which writes the image's TST R0,R0
     * (0x0c91323c); without it, 17 of 23 words differ */
    if ((scroll = heap_alloc(sizeof(*scroll))) == NULL) {
    }
    /* NOLINTNEXTLINE(clang-diagnostic-sign-compare) */
    for (i = 0; i < sizeof(*scroll); i++) {
        ((Uint8 *)scroll)[i] = 0;
    }
    return scroll;
}

/* set the v of each of ARROW_MESHSET's uvs to its own plus the scroll.
 * then move the scroll on by the panel's speed */
static void cart_jump_panel_scroll(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: the model read through the object's address, which pools
     * the object's address and reads the model 4 past it (0x0c913258,
     * 0x0c913266); as cart_jump_panel_object.model, SHC pools the address of
     * the object's model field, 4 past the object's, and 11 of 76 words and
     * 11 pool words differ */
    NJS_COLOR *uv =
        (&cart_jump_panel_object)->model->meshsets[ARROW_MESHSET].vertuv;
    int i;

    for (i = 0; i < ARROW_UVS; i++) {
        uv->tex.v = cart_jump_panel_v[i] + cart_jump_panel_arrows->offset;
        /* @hack spelling: uv = uv + 1, which keeps uv in R6 and
         * cart_jump_panel_arrows' address in R7 (0x0c913286, 0x0c913262); as
         * uv++, the two trade and 14 of 76 words differ */
        uv = uv + 1;
    }
    if (work->scl.x < 0.1F) {
        cart_jump_panel_arrows->offset += JUMP_SPEED * JUMP_SCROLL_PER_SPEED;
    } else {
        /* @hack spelling: the speed read through task->work again, not work,
         * which keeps work in the frame (MOV.L R3,@R15 at 0x0c913264) and
         * loads task->work anew here (0x0c9132c0); through work, 76 of 76
         * words differ */
        cart_jump_panel_arrows->offset +=
            task->work->scl.x * JUMP_SCROLL_PER_SPEED;
    }
    if (cart_jump_panel_arrows->offset > SCROLL_WRAP) {
        cart_jump_panel_arrows->offset -= SCROLL_WRAP;
    }
}

/* throw player 0, or a cart, that hits the ramp. the throw is scl.x a step
 * toward the panel's -z, and scl.y times that along its y */
static void cart_jump_panel_throw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR speed;
    Angle ang;

    /* @bug memory: a panel that started while the set editor held it has no
     * collision record. this then reads the record's flags through null
     * (0x0c91336a) */
    if ((work->collision->flags & COLLISION_HIT) &&
        work->collision->own_shape <= JUMP_LAST_RAMP_SHAPE) {
        speed.x = 0.0F;
        speed.y = work->scl.y * work->scl.x;
        speed.z = work->scl.x;
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y + HALF_TURN, ang);
        njCalcVector(NULL, &speed, &speed);
        njPopMatrix(1);
        cart_push(work->collision->hit->owner, &speed);
        sound_play_at(SE_DASH, 0, 0, 0, work->pos.x, work->pos.y, work->pos.z);
    }
    collision_register(work);
}
