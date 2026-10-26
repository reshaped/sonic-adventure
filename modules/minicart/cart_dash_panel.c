/* O Dash Cart, the dash panel for carts. Twinkle Park's object list names it
 * O DASH PANEL 4 CART */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "minicart/cart_dash_panel.h"
#include "minicart/e_cart.h"
#include "minicart/shareobj_textures.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT cart_dash_panel_object;

/* a panel pushes at scl.x, or at DASH_SPEED where scl.x is below 0.1. its
 * arrows scroll DASH_SCROLL_PER_SPEED in v a step for each unit of speed */
#define DASH_SPEED 10.0F
#define DASH_SCROLL_PER_SPEED 4.0F

/* the model's meshset of the arrows, and its uvs, which the scroll moves */
#define ARROW_MESHSET 2
#define ARROW_UVS 4
/* the scroll wraps past SCROLL_WRAP, a texture's span in v */
#define SCROLL_WRAP 256.0F

/* half a turn, in angle units */
#define HALF_TURN 0x8000

/* a panel's record, which the first word of its task's extra points to */
struct arrow_scroll {
    Uint8 unk_00[4];
    Float offset; /* the arrows' scroll in v */
};

#define SCROLL(task) (*(struct arrow_scroll **)(task)->extra)

/* a panel's work->mode. this file never sets DASH_PANEL_STILL or
 * DASH_PANEL_GONE */
enum dash_panel_mode {
    DASH_PANEL_INIT,
    DASH_PANEL_PUSHING,
    DASH_PANEL_STILL,
    DASH_PANEL_GONE
};

static void cart_dash_panel_destroy(struct task *task);
static void cart_dash_panel_draw(struct task *task);
static struct arrow_scroll *cart_dash_panel_alloc(void);
static void cart_dash_panel_scroll(struct task *task);
static void cart_dash_panel_push(struct task *task);

static struct collision_shape cart_dash_panel_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OTHERS),
    {0.0F, 0.0F, 0.0F},
    17.0F,
    3.0F,
    0.0F,
    {0},
    0,
    NULL};
/* each arrow uv's v at a scroll of 0 */
static Sint16 cart_dash_panel_v[ARROW_UVS] = {
#include "minicart/cart_dash_panel/cart_dash_panel_v.inc"
};

/* the record of the panel whose step runs, which the scroll reads */
static struct arrow_scroll *cart_dash_panel_arrows;

void cart_dash_panel_step(struct task *task)
{
    struct task_work *work = task->work;

    cart_dash_panel_arrows = SCROLL(task);
    if (placement_editing(task)) {
        if (work->mode == DASH_PANEL_INIT) {
            work->mode++;
            SCROLL(task) = cart_dash_panel_alloc();
            task->destructor = cart_dash_panel_destroy;
            task->draw = cart_dash_panel_draw;
        }
        cart_dash_panel_arrows = SCROLL(task);
        cart_dash_panel_draw(task);
        cart_dash_panel_scroll(task);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> SPEED (default 10.f)");
    } else {
        switch (work->mode) {
        case DASH_PANEL_INIT:
            work->mode = DASH_PANEL_PUSHING;
            SCROLL(task) = cart_dash_panel_alloc();
            cart_dash_panel_arrows = SCROLL(task);
            task->destructor = cart_dash_panel_destroy;
            task->draw = cart_dash_panel_draw;
            collision_init(task, &cart_dash_panel_shape, COLLISION_OBJECT, 1,
                           1);
            if (work->scl.x < 0.1F) {
                work->scl.x = DASH_SPEED;
            }
            break;
        case DASH_PANEL_PUSHING:
            cart_dash_panel_draw(task);
            cart_dash_panel_scroll(task);
            cart_dash_panel_push(task);
            break;
        case DASH_PANEL_STILL:
            cart_dash_panel_draw(task);
            break;
        case DASH_PANEL_GONE:
        default:
            placement_kill(task);
            break;
        }
    }
}

static void cart_dash_panel_destroy(struct task *task)
{
    if (SCROLL(task) != NULL) {
        heap_free(SCROLL(task));
        SCROLL(task) = NULL;
    }
}

static void cart_dash_panel_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njSetTexture(&obj_shareobj_texlist);
    draw_tree(&cart_dash_panel_object);
    njPopMatrix(1);
}

/* a panel's record from the heap, cleared byte by byte */
static struct arrow_scroll *cart_dash_panel_alloc(void)
{
    struct arrow_scroll *scroll;
    /* @hack type: i, an int against sizeof's unsigned, which the NOLINT
     * below admits. the loop's cursor then starts as 0 plus the record, in R4
     * (0x0c9144d2, 0x0c9144d6); as a Uint32, R4 takes the record itself and
     * 16 of 23 words differ */
    int i;

    /* @hack redundant: the empty test, which writes the image's TST R0,R0
     * (0x0c9144da); without it, 17 of 23 words differ */
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
static void cart_dash_panel_scroll(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: the model read through the object's address, which pools
     * the object's address and reads the model 4 past it (0x0c9144f6,
     * 0x0c914504); as cart_dash_panel_object.model, SHC pools the address of
     * the object's model field, 4 past the object's, and 11 of 76 words and
     * 11 pool words differ */
    NJS_COLOR *uv =
        (&cart_dash_panel_object)->model->meshsets[ARROW_MESHSET].vertuv;
    int i;

    for (i = 0; i < ARROW_UVS; i++) {
        uv->tex.v = cart_dash_panel_v[i] + cart_dash_panel_arrows->offset;
        /* @hack spelling: uv = uv + 1, which keeps uv in R6 and
         * cart_dash_panel_arrows' address in R7 (0x0c914524, 0x0c914500); as
         * uv++, the two trade and 14 of 76 words differ */
        uv = uv + 1;
    }
    if (work->scl.x < 0.1F) {
        cart_dash_panel_arrows->offset += DASH_SPEED * DASH_SCROLL_PER_SPEED;
    } else {
        /* @hack spelling: the speed read through task->work again, not work,
         * which keeps work in the frame (MOV.L R3,@R15 at 0x0c914502) and
         * loads task->work anew here (0x0c91455e); through work, 76 of 76
         * words differ */
        cart_dash_panel_arrows->offset +=
            task->work->scl.x * DASH_SCROLL_PER_SPEED;
    }
    if (cart_dash_panel_arrows->offset > SCROLL_WRAP) {
        cart_dash_panel_arrows->offset -= SCROLL_WRAP;
    }
}

/* push player 0, or a cart, that hits the panel. the push is scl.x a step
 * toward the panel's -z */
static void cart_dash_panel_push(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    /* @bug memory: a panel that started while the set editor held it has no
     * collision record. this then reads the record's flags through null
     * (0x0c91460a) */
    if (work->collision->flags & COLLISION_HIT) {
        NJS_VECTOR speed = {0.0F, 0.0F, -DASH_SPEED};

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
