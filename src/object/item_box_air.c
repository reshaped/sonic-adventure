/* the floating item box, O ITEMBOXAIR or O ITEMBOX AIR in the object lists,
 * which a player's or an attack's touch bursts. this file also keeps two
 * things both kinds of box share. one is the row of items just collected,
 * at the foot of the screen. the other is the blast an explosion item sends
 * out */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/helpers.h"
#include "draw/model.h"
#include "effect/snow.h"
#include "main.h"
#include "object/item_box.h"
#include "object/item_box_air.h"
#include "object/models.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texlists.h"
#include "system/texture.h"
#include "system/util.h"

/* the box's work->mode */
enum box_mode {
    BOX_INIT, /* starts the box */
    BOX_WAIT, /* grows in, then waits for a player's or an attack's touch */
    BOX_BURST, /* bursts, swelling to BURST_SCALE */
    BOX_DRAW, /* draws, and nothing more */
    BOX_KILL /* kills the task */
};

/* a waiting box's scale grows GROW_STEP a step, to 1. a bursting box's
 * scale swells BURST_STEP a step, and the box goes once it reaches
 * BURST_SCALE */
#define GROW_STEP 0.05F
#define BURST_STEP 0.2F
#define BURST_SCALE 5.0F

/* the state of the row of collected items on show */
enum shown_state {
    SHOWN_NONE, /* none on show */
    SHOWN_ENTER, /* they slide in from the left to their places */
    SHOWN_HOLD, /* they hold */
    SHOWN_LEAVE /* they shrink away */
};

/* the items the row has room for */
#define SHOWN_COUNT 20
/* a holding row counts its steps in shown.timer. the step that takes the
 * count past SHOWN_STEPS starts the row shrinking away, over SHOWN_STEPS
 * steps */
#define SHOWN_STEPS 60
/* the row's items stand SHOWN_SPACING apart */
#define SHOWN_SPACING 80.0F
/* an item comes on show at SHOWN_START_X, off the screen's left, and
 * slides SHOWN_SLIDE a step to its place. it comes at scale SHOWN_SCALE,
 * and an item that has shrunk grows back SHOWN_GROW a step to it */
#define SHOWN_START_X (-60.0F)
#define SHOWN_SLIDE 30.0F
#define SHOWN_SCALE 2.0F
#define SHOWN_GROW 0.2F

/* a blast's radius starts at BLAST_START and grows BLAST_GROWTH a step.
 * past BLAST_MOST, the blast ends, and the radius is NO_BLAST */
#define BLAST_START 10.0F
#define BLAST_GROWTH 10.0F
#define BLAST_MOST 500.0F
#define NO_BLAST (-1.0F)

/* the box's record. its task's extra points to it */
struct item_box_air {
    Uint8 unk_00[4];
    enum item_kind kind; /* the item, which the step copies from scl.x */
    Uint8 unk_08[12];
    Angle panel_ang; /* the turn of the item's panel about y */
    Float burst; /* the box's scale as it bursts, from 1 to BURST_SCALE */
    Float grow; /* its scale as it grows in, from 0 to 1 */
};

/* the box's task's extra: its record, or NULL */
struct item_box_air_extra {
    struct item_box_air *box;
};

#define EXTRA(task) ((struct item_box_air_extra *)(task)->extra)

/* a collected item on show */
struct shown_item {
    enum item_kind kind;
    Float x; /* its x on the screen */
    Float scale;
    Sint32 rings; /* item_rings_given as the item was collected */
};

/* the row of collected items on show */
struct shown_items {
    enum shown_state state;
    Sint32 count;
    /* while the row holds, the steps it has held. while it shrinks, the
     * steps left */
    Uint32 timer;
    struct shown_item items[SHOWN_COUNT];
};

static void item_box_air_destroy(struct task *task);
static void item_box_air_draw(struct task *task);
static struct item_box_air *item_box_air_alloc(void);
static void item_box_air_wait(struct task *task);
static void item_box_air_burst(struct task *task);
static void shown_step(struct task *task);
static void shown_enter(void);
static void shown_hold(void);
static void shown_leave(void);
static void shown_draw(struct task *task);
static void shown_draw_item(enum item_kind kind, Float x, int rings,
                            Float scale);
static void shown_destroy(struct task *task);
static void item_explosion_step(struct task *task);
static void item_box_air_label(struct task *task);
static void item_box_air_lower_draw(NJS_OBJECT *object);
static void item_box_air_panel_draw(NJS_OBJECT *object);

static const NJS_VECTOR item_box_air_puff_speed = {0.0F, 1.0F, 0.0F};

/* clang-format off */
static struct collision_shape item_box_air_shape = {
    SHAPE_SPHERE | LOCKON_NO_NUMBER, 0,
    PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OTHERS), {0.0F, 5.0F, 0.0F},
    10.0F, 0.0F, 0.0F, {0}, 0, NULL};
/* clang-format on */

/* draw_object draws item_box_air_panel by item_box_air_panel_draw. while
 * the box bursts, it also draws item_box_air_lower by
 * item_box_air_lower_draw */
static struct draw_override item_box_air_overrides[] = {
    {&item_box_air_panel, item_box_air_panel_draw},
    {NULL, NULL},
};
static struct draw_override item_box_air_burst_overrides[] = {
    {&item_box_air_panel, item_box_air_panel_draw},
    {&item_box_air_lower, item_box_air_lower_draw},
    {NULL, NULL},
};
#ifdef GCC
/* clang-format off */
static const struct collision_shape item_box_air_burst_shape = {
    SHAPE_SPHERE | SHAPE_OFF, 0, 0, {0.0F, 0.0F, 0.0F}, 0.0F, 0.0F, 0.0F, {0},
    0, NULL};
/* clang-format on */
#endif

/* the sprite of a collected item's face, whose texture shown_draw_item
 * sets */
static NJS_TEXANIM shown_face = {
#include "object/item_box_air/shown_face.inc"
};
/* the sprite of the ring count a random ring item gave, drawn over its face.
 * its texture is item_ringnum, whose four quarters hold 40, 20, 15 and 1.
 * set_rings_uv picks the quarter */
static NJS_TEXANIM shown_rings = {
#include "object/item_box_air/shown_rings.inc"
};

/* the name the set editor shows for each kind of item, by enum item_kind */
static char *item_names[ITEM_COUNT] = {
    "HAYAASI", "MUTEKI", "5RING",         "10RING",       "RANDOMRING",
    "BARIA",   "1UP",    "EKUSUPUROSYON", "ITEM_TH_BARIA"};

Sint32 item_rings_given;
/* the record of the box whose step or draw runs. the draw overrides read
 * it, as draw_object hands them no box */
static struct item_box_air *item_box_air_current;
/* where an explosion item's blast went off, and its radius, or NO_BLAST
 * while there is no blast */
static NJS_POINT3 item_explosion_pos;
static Float item_explosion_radius;
static struct shown_items shown;

void item_box_air_create(NJS_POINT3 *pos, enum item_kind kind)
{
    struct task *task =
        task_create(TASK_MOTION | TASK_WORK | TASK_FORCE | TASK_EXTRA, 3,
                    item_box_air_step);
    struct task_work *work = task->work;

    work->pos = *pos;
    work->scl.x = (Float)kind;
}

void item_box_air_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    item_box_air_current = EXTRA(task)->box;
    if (item_box_air_current != NULL) {
        if ((int)work->scl.x < 0) {
            work->scl.x = ITEM_COUNT - 1;
        } else if ((int)work->scl.x >= ITEM_COUNT) {
            work->scl.x = 0.0F;
        }
        if (work->mode != BOX_INIT) {
            /* @hack type: the kind converted through a Uint32, which keeps
             * the image's test against 2147483648.0F ahead of the FTRC
             * (FCMP/GT at 0x8c060cb4, FTRC at 0x8c060cbe). as an enum
             * item_kind, the test goes, the object's code runs 64 bytes
             * short, and 174 of the step's 211 words differ */
            item_box_air_current->kind = (Uint32)work->scl.x;
        }
    }
    if (placement_editing(task)) {
        if (work->mode == BOX_INIT) {
            work->mode++;
            EXTRA(task)->box = item_box_air_alloc();
            task->destructor = item_box_air_destroy;
            task->draw = item_box_air_draw;
        }
        item_box_air_current = EXTRA(task)->box;
        item_box_air_draw(task);
        item_box_air_label(task);
    } else {
        switch (work->mode) {
        case BOX_INIT:
            work->mode = BOX_WAIT;
            EXTRA(task)->box = item_box_air_alloc();
            /* a lockon_target for E-102 to lock on to, in place of the
             * task's motion record */
            if (task->motion != NULL) {
                heap_free(task->motion);
            }
            task->motion = heap_calloc(1, sizeof(struct lockon_target));
            item_box_air_current = EXTRA(task)->box;
            task->destructor = item_box_air_destroy;
            task->draw = item_box_air_draw;
            collision_init(task, &item_box_air_shape, COLLISION_ITEM, 1, 1);
            item_box_air_current->burst = 1.0F;
            if (!IN_STAGE(STAGE_CART)) {
                /* @hack redundant: an empty else, which has BOX_WAIT's panel
                 * turn take the record in R2 and the turn in R3 (0x8c060df6,
                 * 0x8c060df8). without it, the two trade and 5 of 211 words
                 * differ */
                if (task->placement != NULL) {
                    placement_keep(task);
                } else {
                }
            }
            return;
        case BOX_WAIT:
            item_box_air_draw(task);
            item_box_air_wait(task);
            item_box_air_current->panel_ang += NJM_DEG_ANG(3);
            return;
        case BOX_BURST:
            item_box_air_draw(task);
            item_box_air_burst(task);
            return;
        case BOX_DRAW:
            item_box_air_draw(task);
            return;
        /* @hack redundant: a case label nothing needs, which keeps the image's
         * CMP/EQ #4,R0 and its branch (0x8c060d1a). left to the default,
         * the compare goes, the object's code runs 32 bytes short, and 126
         * of 211 words differ */
        case BOX_KILL:
            break;
        default:
            break;
        }
        if (task->placement != NULL) {
            placement_kill(task);
        } else {
            task_kill(task);
        }
    }
}

static void item_box_air_destroy(struct task *task)
{
    if (EXTRA(task)->box != NULL) {
        heap_free(EXTRA(task)->box);
        EXTRA(task)->box = NULL;
    }
}

/* draw the box in view: at its grow scale while it starts and waits, and
 * otherwise at its burst scale, fading */
static void item_box_air_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    item_box_air_current = EXTRA(task)->box;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    texture_set_regular();
    if (work->mode == BOX_WAIT || work->mode == BOX_INIT) {
        njScale(NULL, item_box_air_current->grow, item_box_air_current->grow,
                item_box_air_current->grow);
        draw_object(&item_box_air_object, item_box_air_overrides);
    } else {
        njScale(NULL, item_box_air_current->burst, item_box_air_current->burst,
                item_box_air_current->burst);
        draw_attr_save();
        draw_attr_set(0, NJD_FLAG_USE_ALPHA);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        draw_set_material((1.0F - item_box_air_current->burst) * 0.25F, 0.0F,
                          0.0F, 0.0F);
        draw_object(&item_box_air_object, item_box_air_burst_overrides);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
    }
    njPopMatrix(1);
}

/* a record from the heap, zeroed, or NULL when the heap has no room */
static struct item_box_air *item_box_air_alloc(void)
{
    struct item_box_air *box;
    int i;

    /* @hack redundant: the new record tested to no effect, which keeps the
     * image's TST R0,R0 at 0x8c061060. without it, the function runs 2
     * bytes short and 17 of its 23 words differ */
    if ((box = heap_alloc(sizeof(*box))) != NULL) {
    }
    /* NOLINTNEXTLINE(clang-diagnostic-sign-compare) */
    for (i = 0; i < sizeof(*box); i++) {
        ((Uint8 *)box)[i] = 0;
    }
    return box;
}

/* draw the box's lower part, darkened as the box bursts */
static void item_box_air_lower_draw(NJS_OBJECT *object)
{
    NJS_ARGB color;

    draw_control_save();
    njControl3D(NJD_CONTROL_3D_OFFSET_MATERIAL);
    color.r = color.g = color.b = (1.0F - item_box_air_current->burst) * 0.25F;
    njSetConstantMaterial(&color);
    njSimpleDrawObject(object);
    draw_control_restore();
}

/* draw the panel with the face of the box's item, turned panel_ang about
 * y */
static void item_box_air_panel_draw(NJS_OBJECT *object)
{
    item_kinds[ITEM_1UP].texture = item_1up_textures[get_character()];
    object->model->mats->attr_texId =
        item_kinds[item_box_air_current->kind].texture;
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    TURN(njRotateZ, object->ang[2]);
    TURN(njRotateX, object->ang[0]);
    TURN(njRotateY, item_box_air_current->panel_ang);
    draw_model(object->model);
}

/* grow the box in. once it has grown, a player's or an attack's touch
 * bursts it. the item is given when player 0 burst the box. in E-102's
 * game, or with Tails as player 0 and player 1 not Sonic, it is given
 * whatever burst the box */
static void item_box_air_wait(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;

    item_box_air_current->grow += GROW_STEP;
    if (item_box_air_current->grow > 1.0F) {
        item_box_air_current->grow = 1.0F;
        if ((work->collision->flags & COLLISION_HIT) &&
            (work->collision->hit->kind == COLLISION_PLAYER ||
             work->collision->hit->kind == COLLISION_ATTACK)) {
            work->mode = BOX_BURST;
            pos = work->pos;
            pos.y += 15.0F;
            snow_create(&pos, &item_box_air_puff_speed, 1.2F);
            if (item_kinds[item_box_air_current->kind].give != NULL &&
                (work->collision->hit->owner == player_get_task(0) ||
                 get_character() == CHARACTER_E102 ||
                 (player_get_character(0) == CHARACTER_TAILS &&
                  player_get_character(1) != CHARACTER_SONIC))) {
                item_kinds[item_box_air_current->kind].give(work);
                item_show(item_box_air_current->kind);
            }
            lockon_release(task);
            collision_free(work);
            /* @bug memory: the burst's shape reads 36 bytes past
             * item_box_air_overrides' end. collision_init takes that table of
             * draw overrides as the burst's one shape (MOV.L at 0x8c061298),
             * and the shape's 52 bytes are the table's 16 and the 36 after
             * it */
#ifdef GCC
            /* repairs the read the tag names: GCC's burst takes a shape that
             * touches nothing. the image's touches nothing either: its kind
             * is the low four bits of item_box_air_panel's address, 0xc,
             * which no check takes */
            collision_init(task, &item_box_air_burst_shape, COLLISION_OBJECT, 1,
                           1);
#else
            collision_init(task,
                           (struct collision_shape *)item_box_air_overrides,
                           COLLISION_OBJECT, 1, 1);
#endif
        } else {
            collision_register(work);
        }
    }
}

static void item_box_air_burst(struct task *task)
{
    item_box_air_current->burst += BURST_STEP;
    if (item_box_air_current->burst >= BURST_SCALE) {
        item_box_air_current->burst = BURST_SCALE;
        task->work->mode = BOX_KILL;
    }
}

void item_reset(void)
{
    struct task *task = task_create(TASK_WORK, 3, shown_step);

    task->draw = shown_draw;
    task->destructor = shown_destroy;
    shown.count = 0;
    shown.state = SHOWN_NONE;
    item_explosion_radius = NO_BLAST;
}

void item_show(enum item_kind kind)
{
    /* @bug memory: shown.count goes untested against SHOWN_COUNT
     * (MOV.L R4,@R3 at 0x8c06132a). a 21st item on show writes past
     * shown.items, into the bss after it */
#ifdef GCC
    /* repairs the stores the tag names, which in a stage with panels
     * overwrite their live tasks. GCC's row takes no item past its last, but
     * enters again, as the image's does on each item */
    if (shown.count >= SHOWN_COUNT) {
        shown.state = SHOWN_ENTER;
        return;
    }
#endif
    shown.items[shown.count].kind = kind;
    shown.items[shown.count].x = SHOWN_START_X;
    shown.items[shown.count].scale = SHOWN_SCALE;
    shown.items[shown.count].rings = item_rings_given;
    shown.count++;
    shown.state = SHOWN_ENTER;
}

/* the step of item_reset's task: move the row on show and draw it, and grow
 * the blast. the row also draws as the task's draw */
static void shown_step(struct task *task)
{
    switch (shown.state) {
    case SHOWN_NONE:
        break;
    case SHOWN_ENTER:
        shown_enter();
        shown_draw(task);
        break;
    case SHOWN_HOLD:
        shown_hold();
        shown_draw(task);
        break;
    case SHOWN_LEAVE:
        shown_leave();
        shown_draw(task);
        break;
    }
    item_explosion_step(task);
}

/* slide each item in from the left to its place in the row, which is
 * centered on the screen. the row holds once every item is in place */
static void shown_enter(void)
{
    int i;
    int done = 0;
    Float place;

    for (i = 0; i < shown.count; i++) {
        shown.items[i].x += SHOWN_SLIDE;
        place = ((shown.count - 1) * (SHOWN_SPACING / 2)) -
                (i * SHOWN_SPACING) + SCREEN_MIDDLE_X;
        if (shown.items[i].x >= place) {
            shown.items[i].x = place;
            done++;
        }
        shown.items[i].scale += SHOWN_GROW;
        if (shown.items[i].scale > SHOWN_SCALE) {
            shown.items[i].scale = SHOWN_SCALE;
        }
    }
    if (done == shown.count) {
        shown.state = SHOWN_HOLD;
        shown.timer = 0;
    }
}

static void shown_hold(void)
{
    int i;

    for (i = 0; i < shown.count; i++) {
        shown.items[i].scale += SHOWN_GROW;
        if (shown.items[i].scale > SHOWN_SCALE) {
            shown.items[i].scale = SHOWN_SCALE;
        }
    }
    shown.timer++;
    if (shown.timer > SHOWN_STEPS) {
        shown.timer = SHOWN_STEPS;
        shown.state = SHOWN_LEAVE;
    }
}

static void shown_leave(void)
{
    int i;

    for (i = 0; i < shown.count; i++) {
        shown.items[i].scale = shown.timer * (SHOWN_SCALE / SHOWN_STEPS);
    }
    shown.timer--;
    if (shown.timer == 0) {
        shown.count = 0;
        shown.state = SHOWN_NONE;
    }
}

static void shown_draw(struct task *task)
{
    int i;

    for (i = 0; i < shown.count; i++) {
        struct shown_item *item = &shown.items[i];

        shown_draw_item(item->kind, item->x, item->rings, item->scale);
    }
}

/* @hack spelling: the corners set through a helper SHC expands at each of
 * its four calls (0x8c06171e, 0x8c0617a0, 0x8c0617b2, 0x8c0617c4); called,
 * item_box_air.c's code runs 96 bytes short and 175 of shown_draw_item's 175
 * words differ */
#pragma inline(set_rings_uv)
/* set shown_rings's corners in item_ringnum to the quarter that holds the
 * count */
static void set_rings_uv(int u1, int v1, int u2, int v2)
{
    /* @hack type: each corner stored through a byte offset into
     * shown_rings, for which SHC loads u1's, v1's and u2's addresses from
     * the pool and steps from u1's to v2's; as fields, it stores each from
     * R0 at an offset from &shown_rings, shown_draw_item runs 56 bytes
     * short and 137 of its 175 words differ */
    *(Sint16 *)((char *)&shown_rings + (4 * sizeof(Sint16))) = u1;
    *(Sint16 *)((char *)&shown_rings + (5 * sizeof(Sint16))) = v1;
    *(Sint16 *)((char *)&shown_rings + (6 * sizeof(Sint16))) = u2;
    *(Sint16 *)((char *)&shown_rings + (7 * sizeof(Sint16))) = v2;
}

/* draw the item's face at x, at scale, and on a random ring item the count
 * it gave. a count of 5 or 10 shows as ITEM_5RING's or ITEM_10RING's face,
 * with no count over it. a count other than 40, 20 or 15 shows as 1 */
static void shown_draw_item(enum item_kind kind, Float x, int rings,
                            Float scale)
{
    NJS_SPRITE sprite;
    NJS_SPRITE number;

    if (kind == ITEM_RANDOMRING) {
        switch (rings) {
        case 5:
            kind = ITEM_5RING;
            break;
        case 10:
            kind = ITEM_10RING;
            break;
        }
    }
    item_kinds[ITEM_1UP].texture = item_1up_textures[get_character()];
    shown_face.texid = item_kinds[kind].texture;
    sprite.tlist = &obj_regular_texlist;
    sprite.tanim = &shown_face;
    sprite.sx = scale;
    sprite.sy = scale;
    sprite.ang = 0;
    sprite.p.x = x;
    sprite.p.y = 370.0F;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    njDrawSprite2D(&sprite, 0, -10.0F, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    if (kind == ITEM_RANDOMRING) {
        number.p.x = x - 1.0F;
        number.p.y = 372.0F;
        number.sx = scale;
        number.sy = scale;
        number.ang = 0;
        number.tlist = &obj_regular_texlist;
        number.tanim = &shown_rings;
        /* @hack spelling: a draw at the end of each case, which keeps
         * njDrawSprite2D's address in R12 (MOV.L at 0x8c0616c6) and the
         * stack frame at 100 bytes. with one draw after the switch, the
         * stack frame is 96 bytes and 111 of 175 words differ. the cases
         * stand in the order the image lays out their code */
        switch (rings) {
        case 40:
            set_rings_uv(0, 0, 127, 127);
            njDrawSprite2D(&number, 0, -9.0F,
                           NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
            break;
        case 20:
            set_rings_uv(0, 128, 127, 255);
            njDrawSprite2D(&number, 0, -9.0F,
                           NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
            break;
        case 15:
            set_rings_uv(128, 0, 255, 127);
            njDrawSprite2D(&number, 0, -9.0F,
                           NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
            break;
        default:
            set_rings_uv(128, 128, 255, 255);
            njDrawSprite2D(&number, 0, -9.0F,
                           NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
            break;
        }
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

static void shown_destroy(struct task *task)
{
}

void item_explosion_start(NJS_POINT3 *pos)
{
    item_explosion_pos = *pos;
    item_explosion_radius = BLAST_START;
}

static void item_explosion_step(struct task *task)
{
    if (item_explosion_radius >= 0.0F) {
        item_explosion_radius += BLAST_GROWTH;
        if (item_explosion_radius > BLAST_MOST) {
            item_explosion_radius = NO_BLAST;
        }
    }
}

Bool item_explosion_reaches(NJS_POINT3 *pos)
{
    NJS_VECTOR away;

    if (item_explosion_radius >= 0.0F) {
        away.x = pos->x - item_explosion_pos.x;
        away.y = pos->y - item_explosion_pos.y;
        away.z = pos->z - item_explosion_pos.z;
        if (item_explosion_radius > njScalor(&away)) {
            return TRUE;
        }
    }
    return FALSE;
}

static void item_box_air_label(struct task *task)
{
    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> ITEM NUMBER ");
    njPrintC(NJM_LOCATION(20, 20), item_names[(int)task->work->scl.x]);
}
