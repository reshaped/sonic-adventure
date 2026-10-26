/* the Chao race's menus. slots keep their icons and models, a panel stands
 * behind them, and the menu_draw_ functions draw without a task */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/models.h"
#include "al_race/game.h"
#include "al_race/menu.h"
#include "al_race/race_camera.h"
#include "al_race/texlists.h"
#include "chao/chao.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/sparkle.h"
#include "save/card.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the five courses' models */
extern NJS_OBJECT dat_0cbcd9c4;
extern NJS_OBJECT dat_0cbcb564;
extern NJS_OBJECT dat_0cbcf294;
extern NJS_OBJECT dat_0cbce62c;
extern NJS_OBJECT dat_0cbcce14;

#define MENU_SLOTS 12

/* menu_models' card drive, the first model that takes the a-life window's
 * textures */
#define DRIVE_MODEL 5
/* menu_icons' first icon that takes the a-life window's textures */
#define FIRST_WINDOW_ICON 26

/* the turn about y a model starts at, which LOOK_RED turns it back to */
#define MODEL_TURN 0x4000

/* an icon's depth, and its depth in front of the rest and behind them */
#define ICON_DEPTH (-2.0F)
#define FRONT_DEPTH (-1.9F)
#define BACK_DEPTH (-45.0F)

/* LOOK_POP's stages, which an icon's work keeps in mode */
enum menu_pop { POP_START, POP_OPEN, POP_FADE };
/* the steps a popped icon shows, from the pop's start, before it fades */
#define POP_STEPS 60

/* the panel's full width and height, which it grows to in
 * MENU_PANEL_STEPS */
#define PANEL_WIDTH 3.8F
#define PANEL_HEIGHT 5.2F

/* how far a spinning model bobs above and below its place */
#define SPIN_BOB 1.0F

/* the vars of the menus' tasks. an icon's or a model's work keeps its look
 * in submode, and its index in menu_icons or menu_models in tick. an icon's
 * scl.z is its alpha */
struct menu_vars {
    /* the slot that keeps the task. a task its slot no longer keeps destroys
     * itself on its next step */
    struct task **slot;
    Uint8 unk_04[4];
    NJS_OBJECT *object; /* the model menu_spin_step draws */
};

#define VARS(work) ((struct menu_vars *)&(work)->vars)

/* a sprite menu_draw_sprite draws */
struct menu_sprite {
    Float x;
    Float y;
    Float sx;
    Float sy;
    Float depth;
    int frame;
};

static struct task *menu_icon_create(int icon, Float x, Float y,
                                     struct task **slot);
static void menu_icon_step(struct task *task);
static void menu_icon_settle(struct task_work *work);
static void menu_icon_pulse(struct task_work *work);
static void menu_icon_raise(struct task_work *work);
static void menu_icon_pop(struct task_work *work);
static void menu_icon_draw(struct task_work *work);
static struct task *menu_model_create(int model, const NJS_POINT3 *pos,
                                      struct task **slot);
static void menu_model_step(struct task *task);
static void menu_model_settle(struct task_work *work);
static void menu_model_pulse(struct task_work *work);
static void menu_model_raise(struct task_work *work);
static void menu_model_draw(struct task_work *work);
static struct task *menu_panel_create(struct task **slot);
static void menu_panel_step(struct task *task);
static void menu_panel_grow(struct task_work *work);
static void menu_panel_draw(struct task_work *work);
static int menu_garden_icon(int kind, int name);
static void menu_show_icon(Uint8 icon, Float x, Float y);
static void menu_draw_sprite(struct menu_sprite *at);
static void menu_spin_step(struct task *task);

/* each icon's region of its texture list. the icons from FIRST_WINDOW_ICON
 * on take the a-life window's list, and the rest the race's */
static const NJS_TEXANIM menu_icons[32] = {
#include "al_race/menu/menu_icons.inc"
};
/* the five courses' models, then DRIVE_MODEL */
static NJS_OBJECT *const menu_models[6] = {
    &dat_0cbcd9c4, &dat_0cbcb564, &dat_0cbcf294,
    &dat_0cbce62c, &dat_0cbcce14, &card_drive_object,
};
const NJS_TEXANIM menu_sprites[9] = {
#include "al_race/menu/menu_sprites.inc"
};
/* each menu's icons and models, by slot, and where each stands. an icon's
 * place is a point on the screen, and a model's a point in the camera's
 * frame, which race_camera_to_world places in the world */
static const Uint8 mode_icons[5] = {1, 0, 2, 17, 14};
/* @hack type: the points as pairs of Floats, which SHC reads through a copy
 * of the row's cursor, MOV R8,R11 at 0x0cb93f96; as NJS_POINT2, 35 of
 * menu_show_modes' 40 words differ */
static const Float mode_at[5][2] = {
    {320.0F, 180.0F}, {320.0F, 240.0F}, {320.0F, 300.0F},
    {320.0F, 368.0F}, {320.0F, 88.0F},
};
static const int course_models[5] = {0, 1, 2, 3, 4};
static const NJS_POINT3 course_at[5] = {
    {-5.0F, 1.4F, -13.0F}, {-1.7F, 1.4F, -13.0F}, {1.7F, 1.4F, -13.0F},
    {5.0F, 1.4F, -13.0F},  {0.0F, 0.0F, -13.0F},
};
static const NJS_POINT3 course_wide_at[5] = {
    {-6.0F, 1.4F, -13.0F}, {-3.0F, 1.4F, -13.0F}, {0.0F, 1.4F, -13.0F},
    {3.0F, 1.4F, -13.0F},  {6.0F, 1.4F, -13.0F},
};
static const int course_icons[2] = {17, 15};
/* @hack type: the points as pairs of Floats, which SHC reads both through
 * R2 (0x0cb9408a, 0x0cb9408e); as NJS_POINT2, it copies R2 for x, and 187
 * of menu_show_courses' 232 words differ */
static const Float course_icons_at[2][2] = {
    {320.0F, 368.0F},
    {320.0F, 88.0F},
};
static const Uint8 garden_models[1] = {DRIVE_MODEL};
static const NJS_POINT3 garden_model_at = {-23.0F, -1.0F, -50.0F};
static const Uint8 garden_icons[5] = {8, 9, 10, 17, 16};
/* @hack type: the points as pairs of Floats, which SHC reads both through
 * R2 (0x0cb9426e, 0x0cb94274); as NJS_POINT2, it copies R2 for x, and 136
 * of menu_show_gardens' 178 words differ */
static const Float garden_at[5][2] = {
    {250.0F, 200.0F}, {390.0F, 200.0F}, {540.0F, 200.0F},
    {320.0F, 368.0F}, {320.0F, 88.0F},
};
static const Float menu_unused_at[2] = {320.0F, 335.0F};
static const Uint8 entry_icons[4] = {18, 17, 16, 19};
/* @hack type: the points as pairs of Floats, which SHC reads through a copy
 * of the row's cursor, MOV R10,R8 at 0x0cb943f2; as NJS_POINT2, 44 of
 * menu_show_entry's 50 words differ */
static const Float entry_at[4][2] = {
    {210.0F, 368.0F},
    {420.0F, 368.0F},
    {320.0F, 88.0F},
    {320.0F, 368.0F},
};
static const Uint8 course_names[5] = {3, 4, 5, 6, 7};
static const NJS_POINT2 course_name_at[1] = {{320.0F, 119.0F}};
static const Uint8 message_icons[2] = {24, 25};
/* @hack type: the points as pairs of Floats, which puts the cursor's ADD
 * #8,R12 between y's load and its store (0x0cb94490); as NJS_POINT2, 38 of
 * menu_draw_message's 66 words differ */
static const Float message_at[2][2] = {
    {251.0F, 240.0F},
    {448.0F, 240.0F},
};
static const NJS_POINT3 drive_at[CARD_DRIVES] = {
#include "al_race/menu/drive_at.inc"
};
static const NJS_POINT2 drive_icons_at[6] = {
    {180.0F, 320.0F}, {275.0F, 320.0F}, {365.0F, 320.0F},
    {460.0F, 320.0F}, {80.0F, 180.0F},  {80.0F, 260.0F},
};
static const NJS_POINT2 side_icons_at[2] = {{65.0F, 350.0F}, {575.0F, 350.0F}};
static const NJS_POINT3 side_models_at[2] = {{-24.5F, -14.0F, -45.0F},
                                             {24.5F, -14.0F, -45.0F}};
static const NJS_POINT2 chosen_at[2] = {{320.0F, 320.0F}, {320.0F, 240.0F}};

static struct task **menu_slots = NULL;
static struct task *menu_panel = NULL;

/* menu_slots converts to and from void * in heap_calloc's result,
 * heap_free's argument and each test against NULL. sg_xpt.h defines NULL as
 * ((void *)0) */
/* NOLINTBEGIN(bugprone-multi-level-implicit-pointer-conversion) */
struct task **menu_slots_create(void)
{
    int i;

    menu_slots = heap_calloc(MENU_SLOTS, sizeof(struct task *));
    if (menu_slots != NULL) {
        for (i = 0; i < MENU_SLOTS; i++) {
            menu_slots[i] = NULL;
        }
    }
    return menu_slots;
}

void menu_slots_free(void)
{
    if (menu_slots != NULL) {
        menu_slots_clear();
        heap_free(menu_slots);
    }
    menu_slots = NULL;
}

int menu_slots_empty(void)
{
    int i;

    if (menu_slots == NULL) {
        return 0;
    }
    for (i = 0; i < MENU_SLOTS; i++) {
        if (menu_slots[i] != NULL) {
            return 0;
        }
    }
    return 1;
}

void menu_slots_clear(void)
{
    int i;

    if (menu_slots != NULL) {
        for (i = 0; i < MENU_SLOTS; i++) {
            if (menu_slots[i] != NULL) {
                task_destroy(menu_slots[i]);
                menu_slots[i] = NULL;
            }
        }
    }
}

int menu_slot_look(int slot, int look)
{
    struct task *task;
    struct task_work *work;

    if (menu_slots != NULL) {
        task = menu_slots[slot];
        if (task != NULL) {
            work = task->work;
            work->submode = look;
            return 1;
        }
    }
    return 0;
}

static struct task *menu_icon_create(int icon, Float x, Float y,
                                     struct task **slot)
{
    struct task *task;
    struct task_work *work;

    if (*slot != NULL) {
        task_destroy(*slot);
    }
    if ((task = task_create(TASK_WORK, 6, menu_icon_step)) != NULL) {
        work = task->work;
        work->timer = 0;
        work->submode = LOOK_PLAIN;
        work->tick = icon;
        work->scl.z = 1.0F;
        work->mode = POP_START;
        work->pos.x = x;
        work->pos.y = y;
        work->scl.x = 1.0F;
        work->scl.y = 1.0F;
        VARS(work)->slot = slot;
        *slot = task;
        return task;
    }
    return NULL;
}

static void menu_icon_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task **slot = VARS(work)->slot;

    work->timer++;
    if (*slot != task) {
        work->submode = LOOK_GONE;
    }
    switch (work->submode) {
    case LOOK_PLAIN:
    case LOOK_RED:
    case LOOK_BACK:
    case LOOK_YELLOW:
        menu_icon_settle(work);
        break;
    case LOOK_PULSE:
    case LOOK_PULSE_YELLOW:
        menu_icon_pulse(work);
        break;
    case LOOK_RAISED:
        menu_icon_raise(work);
        break;
    case LOOK_HIDDEN:
        return;
    case LOOK_GONE:
        if (*slot == task) {
            *slot = NULL;
        }
        task_destroy(task);
        return;
    case LOOK_POP:
    case LOOK_POP + 1:
    case LOOK_POP + 2:
        menu_icon_pop(work);
        menu_icon_draw(work);
        return;
    default:
        menu_icon_settle(work);
        break;
    }
    menu_icon_draw(work);
    work->submode = LOOK_PLAIN;
}

static void menu_icon_settle(struct task_work *work)
{
    if (work->scl.x > 1.0F) {
        work->scl.x -= 0.05F;
        if (work->scl.x < 1.0F) {
            work->scl.x = 1.0F;
        }
    }
    if (work->scl.y > 1.0F) {
        work->scl.y -= 0.05F;
        if (work->scl.y < 1.0F) {
            work->scl.y = 1.0F;
        }
    }
}

static void menu_icon_pulse(struct task_work *work)
{
    Float pulse = 0.125F + (0.125F * njSin(work->ang.x));

    work->scl.x = pulse + 1.0F;
    work->scl.y = pulse + 1.0F;
    work->ang.x += NJM_DEG_ANG(4);
}

static void menu_icon_raise(struct task_work *work)
{
    work->scl.x = 1.25F;
    work->scl.y = 1.25F;
}

static void menu_icon_pop(struct task_work *work)
{
    switch (work->mode) {
    case POP_START:
        work->scl.x = 0.0F;
        work->scl.y = 1.0F;
        work->timer = 0;
        work->mode++;
        /* fall through */
    case POP_OPEN:
        work->scl.x += 0.1F;
        if (work->scl.x > 1.0F) {
            work->scl.x = 1.0F;
            if (TASK_TIMER(work) > POP_STEPS) {
                work->mode++;
            }
        }
        break;
    case POP_FADE:
        work->scl.z -= 0.05F;
        if (work->scl.z < -1.0F) {
            work->scl.z = -1.0F;
            work->submode = LOOK_GONE;
        }
        break;
    }
}

static void menu_icon_draw(struct task_work *work)
{
    NJS_SPRITE sprite;
    NJS_TEXLIST *texlist;
    Float depth = ICON_DEPTH;

    if (frame_step != 0) {
        return;
    }
    if (work->submode == LOOK_PULSE || work->submode == LOOK_PULSE_YELLOW) {
        depth = FRONT_DEPTH;
    } else if (work->submode == LOOK_BACK) {
        depth = BACK_DEPTH;
    }
    sprite.p.x = work->pos.x;
    sprite.p.y = work->pos.y;
    sprite.p.z = 0.0F;
    sprite.ang = 0;
    texlist =
        work->tick >= FIRST_WINDOW_ICON ? &alife_window_texlist : &dat_0cbc089c;
    sprite.tlist = texlist;
    sprite.tanim = (NJS_TEXANIM *)menu_icons;
    sprite.sx = work->scl.x;
    sprite.sy = work->scl.y;
    njSetTexture(texlist);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    if (work->submode == LOOK_RED) {
        /* @hack spelling: the red's 1.0 through a local, which SHC holds in
         * FR15 for every tint's 1.0 (FLDI1 FR15 at 0x0cb93a3a); in place, 11
         * of 120 words differ */
        Float full = 1.0F;
        draw_set_material(work->scl.z, full, 0.6F, 0.6F);
    } else if (work->submode == LOOK_PULSE_YELLOW ||
               work->submode == LOOK_YELLOW) {
        draw_set_material(work->scl.z, 1.0F, 1.0F, 0.5F);
    } else {
        draw_set_material(work->scl.z, 1.0F, 1.0F, 1.0F);
    }
    njDrawSprite2D(&sprite, work->tick, depth,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

static struct task *menu_model_create(int model, const NJS_POINT3 *pos,
                                      struct task **slot)
{
    struct task *task;
    struct task_work *work;

    if (*slot != NULL) {
        task_destroy(*slot);
    }
    if ((task = task_create(TASK_WORK, 6, menu_model_step)) != NULL) {
        work = task->work;
        work->submode = LOOK_PLAIN;
        work->tick = model;
        work->scl.x = 1.0F;
        work->scl.y = 1.0F;
        work->scl.z = 1.0F;
        work->ang.y = MODEL_TURN;
        VARS(work)->slot = slot;
        work->pos.x = pos->x;
        work->pos.y = pos->y;
        work->pos.z = pos->z;
        *slot = task;
        return task;
    }
    return NULL;
}

static void menu_model_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task **slot = VARS(work)->slot;

    if (*slot != task) {
        work->submode = LOOK_GONE;
    }
    if (work->submode != LOOK_HIDDEN) {
        switch (work->submode) {
        case LOOK_PLAIN:
            menu_model_settle(work);
            break;
        case LOOK_RED:
            work->ang.y = MODEL_TURN;
            menu_model_settle(work);
            break;
        case LOOK_PULSE:
            menu_model_pulse(work);
            work->ang.y += NJM_DEG_ANG(2);
            break;
        case LOOK_PULSE_YELLOW:
            menu_model_pulse(work);
            break;
        case LOOK_RAISED:
            menu_model_raise(work);
            break;
        case LOOK_GONE:
            if (*slot == task) {
                *slot = NULL;
            }
            task_destroy(task);
            return;
        default:
            menu_model_settle(work);
            break;
        }
        work->submode = LOOK_PLAIN;
        menu_model_draw(work);
    }
}

static void menu_model_settle(struct task_work *work)
{
    if (work->scl.x > 1.0F) {
        work->scl.x -= 0.05F;
        if (work->scl.x < 1.0F) {
            work->scl.x = 1.0F;
        }
    }
    if (work->scl.y > 1.0F) {
        work->scl.y -= 0.05F;
        if (work->scl.y < 1.0F) {
            work->scl.y = 1.0F;
        }
    }
    if (work->scl.z > 1.0F) {
        work->scl.z -= 0.05F;
        if (work->scl.z < 1.0F) {
            work->scl.z = 1.0F;
        }
    }
}

static void menu_model_pulse(struct task_work *work)
{
    Float pulse = 0.125F + (0.125F * njSin(work->ang.x));

    work->scl.x = pulse + 1.0F;
    work->scl.y = pulse + 1.0F;
    work->scl.z = pulse + 1.0F;
    work->ang.x += NJM_DEG_ANG(3);
}

static void menu_model_raise(struct task_work *work)
{
    work->scl.x = 1.25F;
    work->scl.y = 1.25F;
    work->scl.z = 1.25F;
}

static void menu_model_draw(struct task_work *work)
{
    /* @hack spelling: the model's entry taken by its address here, which SHC
     * keeps on the stack (MOV.L R3,@R15 at 0x0cb93d24) until the draw reads
     * it (0x0cb93dd4); read at the draw, 79 of 88 words differ */
    NJS_OBJECT *const *object = &menu_models[work->tick];
    NJS_TEXLIST *texlist;
    NJS_POINT3 pos;
    Angle ang;
    Angle turn;

    if (work->tick >= DRIVE_MODEL) {
        texlist = &alife_window_texlist;
    } else {
        texlist = &dat_0cbc089c;
    }
    /* @hack spelling: pos's members through its address, which SHC takes anew
     * for each store (MOV R15,R3 and ADD #4,R3 from 0x0cb93d64); as members,
     * 65 of 88 words differ */
    (&pos)->x = work->pos.x;
    (&pos)->y = work->pos.y;
    (&pos)->z = work->pos.z;
    race_camera_to_world(&pos, &pos);
    njSetTexture(texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, pos.x, pos.y, pos.z);
    ang = work->ang.y;
    /* @hack redundant: the turn tested before TURN_VIA tests it again (TST
     * R13,R13 at 0x0cb93db0 and 0x0cb93db4); with TURN_VIA alone, 20 of 88
     * words differ */
    if (ang) {
        TURN_VIA(njRotateY, ang, turn);
    }
    njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
    draw_tree(*object);
    njPopMatrix(1);
}

static struct task *menu_panel_create(struct task **slot)
{
    struct task *task;
    struct task_work *work;

    if (*slot != NULL) {
        task_destroy(*slot);
    }
    if ((task = task_create(TASK_WORK, 6, menu_panel_step)) != NULL) {
        work = task->work;
        work->submode = LOOK_PLAIN;
        work->timer = 0;
        work->scl.x = 0.0F;
        work->scl.y = 0.0F;
        /* the panel's height grows by scl.z each step */
        work->scl.z = PANEL_HEIGHT / MENU_PANEL_STEPS;
        VARS(work)->slot = slot;
        *slot = task;
        return task;
    }
    return NULL;
}

static void menu_panel_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 offset;

    if (work->submode != LOOK_HIDDEN) {
        offset.x = 0.0F;
        offset.y = 0.5F;
        offset.z = -56.5F;
        race_camera_to_world(&work->pos, &offset);
        if (work->submode != LOOK_HIDDEN) {
            menu_panel_grow(work);
            menu_panel_draw(work);
        }
    }
}

static void menu_panel_grow(struct task_work *work)
{
    work->scl.x += (PANEL_WIDTH / PANEL_HEIGHT) * work->scl.z;
    work->scl.y += work->scl.z;
    if (work->scl.x > PANEL_WIDTH) {
        work->scl.x = PANEL_WIDTH;
    }
    if (work->scl.y > PANEL_HEIGHT) {
        work->scl.y = PANEL_HEIGHT;
    }
}

static void menu_panel_draw(struct task_work *work)
{
    njSetTexture(&alife_window_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njRotateXYZ(NULL, -2730, 15834, 0);
    njScale(NULL, work->scl.x, work->scl.y, 1.0F);
    draw_tree(&card_panel_object);
    njPopMatrix(1);
}

void menu_show_modes(void)
{
    int i;

    if (menu_slots != NULL) {
        for (i = 0; i < 5; i++) {
            menu_icon_create(mode_icons[i], mode_at[i][0], mode_at[i][1],
                             &menu_slots[i]);
        }
    }
}

void menu_show_courses(int wide)
{
    int i;
    const NJS_POINT3 *at;

    if (menu_slots != NULL) {
        at = wide == 0 ? course_at : course_wide_at;
        for (i = 0; i < 7; i++) {
            if (i <= 4) {
                menu_model_create(course_models[i], &at[i], &menu_slots[i]);
            } else {
                menu_icon_create(course_icons[i - 5], course_icons_at[i - 5][0],
                                 course_icons_at[i - 5][1], &menu_slots[i]);
            }
        }
    }
}

void menu_show_gardens(void)
{
    int i;
    const NJS_POINT3 *at;
    const Uint8 *model;

    if (menu_slots != NULL) {
        /* @hack spelling: the model and its place through locals set before
         * the loop, which SHC keeps in R9 and R10 (0x0cb94246, 0x0cb94248);
         * read in the call, 173 of 178 words differ */
        model = garden_models;
        for (i = 0, at = &garden_model_at; i < 6; i++) {
            if (i == 0) {
                menu_model_create(model[0], at, &menu_slots[0]);
            } else {
                menu_icon_create(garden_icons[i - 1], garden_at[i - 1][0],
                                 garden_at[i - 1][1], &menu_slots[i]);
            }
        }
    }
}

void menu_show_entry(void)
{
    int i;

    if (menu_slots != NULL) {
        for (i = 0; i < 4; i++) {
            menu_icon_create(entry_icons[i], entry_at[i][0], entry_at[i][1],
                             &menu_slots[i]);
        }
    }
}

void menu_draw_course(int course)
{
    struct task_work work;

    if (course >= 5) {
        return;
    }
    work.pos.x = course_name_at[0].x;
    work.pos.y = course_name_at[0].y;
    work.tick = course_names[course];
    work.scl.x = work.scl.y = 1.5F;
    work.scl.z = 1.0F;
    work.submode = LOOK_RED;
    menu_icon_draw(&work);
}

void menu_draw_message(void)
{
    struct task_work work;
    int i;

    for (i = 0; i < 2; i++) {
        work.pos.x = message_at[i][0];
        work.pos.y = message_at[i][1];
        work.tick = message_icons[i];
        work.scl.x = work.scl.y = 1.0F;
        work.scl.z = 1.0F;
        work.submode = LOOK_PLAIN;
        menu_icon_draw(&work);
    }
}

void menu_draw_drives(void)
{
    struct task_work work;
    int i;

    for (i = 0; i < CARD_DRIVES; i++) {
        work.pos = drive_at[i];
        work.tick = DRIVE_MODEL;
        work.ang.y = MODEL_TURN;
        work.scl.x = work.scl.y = work.scl.z = 0.65F;
        work.submode = LOOK_PLAIN;
        menu_model_draw(&work);
    }
    for (i = 0; i < 6; i++) {
        work.pos.x = drive_icons_at[i].x;
        work.pos.y = drive_icons_at[i].y;
        work.tick = i + FIRST_WINDOW_ICON;
        work.scl.x = work.scl.y = 1.0F;
        work.scl.z = 1.0F;
        work.submode = LOOK_PLAIN;
        menu_icon_draw(&work);
    }
}

int menu_garden_before(int kind)
{
    switch (kind) {
    case ENTRY_CARD:
        return GARDEN_EGG_CARRIER;
    case GARDEN_STATION_SQUARE:
        return ENTRY_CARD;
    case GARDEN_MYSTIC_RUINS:
        return GARDEN_STATION_SQUARE;
    case GARDEN_EGG_CARRIER:
    default:
        return GARDEN_MYSTIC_RUINS;
    }
}

int menu_garden_after(int kind)
{
    switch (kind) {
    case ENTRY_CARD:
        return GARDEN_STATION_SQUARE;
    case GARDEN_STATION_SQUARE:
        return GARDEN_MYSTIC_RUINS;
    case GARDEN_MYSTIC_RUINS:
        return GARDEN_EGG_CARRIER;
    case GARDEN_EGG_CARRIER:
    default:
        return ENTRY_CARD;
    }
}

/* the icon of kind's garden's picture, or with name set, of its name.
 * ENTRY_CARD takes icon 5 either way, and any other kind is taken as
 * GARDEN_EGG_CARRIER */
static int menu_garden_icon(int kind, int name)
{
    if (name == 0) {
        switch (kind) {
        case ENTRY_CARD:
            return 5;
        case GARDEN_STATION_SQUARE:
            return 8;
        case GARDEN_MYSTIC_RUINS:
            return 9;
        case GARDEN_EGG_CARRIER:
        default:
            return 10;
        }
    } else {
        switch (kind) {
        case ENTRY_CARD:
            return 5;
        case GARDEN_STATION_SQUARE:
            return 11;
        case GARDEN_MYSTIC_RUINS:
            return 12;
        case GARDEN_EGG_CARRIER:
        default:
            return 13;
        }
    }
}

void menu_draw_gardens(int kind)
{
    int sides[2];
    struct task_work work;
    int i;

    sides[0] = menu_garden_before(kind);
    sides[1] = menu_garden_after(kind);
    for (i = 0; i <= 1; i++) {
        if (sides[i] != ENTRY_CARD) {
            work.pos.x = side_icons_at[i].x;
            work.pos.y = side_icons_at[i].y;
            work.scl.x = work.scl.y = 0.45F;
            work.tick = menu_garden_icon(sides[i], 0);
            work.scl.z = 1.0F;
            work.submode = LOOK_PLAIN;
            menu_icon_draw(&work);
        } else {
            work.pos = side_models_at[i];
            work.tick = DRIVE_MODEL;
            work.ang.y = MODEL_TURN;
            work.scl.x = work.scl.y = 0.58F;
            work.scl.z = 0.1F;
            work.submode = LOOK_PLAIN;
            menu_model_draw(&work);
        }
    }
    if (kind != ENTRY_CARD) {
        const NJS_POINT2 *at;

        /* @hack spelling: chosen_at's address kept in at from the first load,
         * which SHC saves on the stack (MOV.L R4,@R15 at 0x0cb9482e) for the
         * second icon (0x0cb9485c); as chosen_at[0] and chosen_at[1], 109 of
         * 170 words differ */
        work.pos.x = (at = chosen_at)->x;
        work.pos.y = chosen_at->y;
        work.scl.x = work.scl.y = 1.5F;
        work.tick = menu_garden_icon(kind, 1);
        work.scl.z = 1.0F;
        work.submode = LOOK_YELLOW;
        menu_icon_draw(&work);
        work.pos.x = at[1].x;
        work.pos.y = at[1].y;
        work.scl.x = work.scl.y = 1.0F;
        work.tick = menu_garden_icon(kind, 0);
        work.scl.z = 0.25F;
        work.submode = LOOK_BACK;
        menu_icon_draw(&work);
    }
}

int menu_panel_open(void)
{
    if (menu_panel != NULL) {
        return 0;
    }
    menu_panel_create(&menu_panel);
    return 1;
}

void menu_panel_close(void)
{
    if (menu_panel != NULL) {
        task_destroy(menu_panel);
        menu_panel = NULL;
    }
}

static void menu_show_icon(Uint8 icon, Float x, Float y)
{
    struct task **slots;

    slots = menu_slots;
    if (slots == NULL) {
        return;
    }
    menu_icon_create(icon, x, y, slots);
}
/* NOLINTEND(bugprone-multi-level-implicit-pointer-conversion) */

void menu_show_start(void)
{
    menu_show_icon(20, 320.0F, 240.0F);
}

void menu_show_goal(void)
{
    menu_show_icon(21, 320.0F, 240.0F);
}

void menu_show_cool(void)
{
    menu_show_icon(22, 320.0F, 240.0F);
}

void menu_show_get(void)
{
    menu_show_icon(23, 320.0F, 240.0F);
}

static void menu_draw_sprite(struct menu_sprite *at)
{
    NJS_SPRITE sprite;

    if (frame_step != 0) {
        return;
    }
    sprite.p.x = at->x;
    sprite.p.y = at->y;
    sprite.p.z = 0.0F;
    sprite.ang = 0;
    sprite.tlist = &dat_0cbc089c;
    sprite.tanim = (NJS_TEXANIM *)menu_sprites;
    sprite.sx = at->sx;
    sprite.sy = at->sy;
    njSetTexture(&dat_0cbc089c);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    njDrawSprite2D(&sprite, at->frame, at->depth,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

void menu_draw_sprite_at(Float x, Float y, Float scale, int frame)
{
    struct menu_sprite sprite;

    sprite.x = x;
    sprite.y = y;
    sprite.sx = scale;
    sprite.sy = scale;
    sprite.frame = frame;
    sprite.depth = -1.8F;
    menu_draw_sprite(&sprite);
}

void menu_spin_create(int course, NJS_POINT3 *pos)
{
    struct task *task;
    struct task_work *work;
    int model;

    switch (course) {
    case 0:
        model = 0;
        break;
    case 1:
        model = 1;
        break;
    case 2:
        model = 2;
        break;
    case 3:
        model = 3;
        break;
    case 4:
        model = 4;
        break;
    default:
        return;
    }
    if ((task = task_create(TASK_WORK, 6, menu_spin_step)) != NULL) {
        work = task->work;
        work->pos = *pos;
        work->ang.x = 0;
        work->ang.y = 0;
        work->ang.z = 0;
        work->timer = 0;
        VARS(work)->object = menu_models[model];
    }
}

static void menu_spin_step(struct task *task)
{
    struct task_work *work;
    NJS_OBJECT *object;
    Float bob;
    Angle ang;
    Angle turn;
    NJS_POINT3 glint;

    if ((Uint8)stage_get_stage_act() == ACT_HALL) {
        task_destroy(task);
        return;
    }
    if (frame_step != 0) {
        return;
    }
    work = task->work;
    object = VARS(work)->object;
    work->ang.y += NJM_DEG_ANG(2);
    work->ang.z += NJM_DEG_ANG(3);
    bob = SPIN_BOB * njCos(work->ang.z);
    njSetTexture(&dat_0cbc089c);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + bob, work->pos.z);
    ang = work->ang.y;
    /* @hack redundant: the turn tested before TURN_VIA tests it again (TST
     * R13,R13 at 0x0cb94bba and 0x0cb94bbe); with TURN_VIA alone, 21 of 161
     * words differ */
    if (ang) {
        TURN_VIA(njRotateY, ang, turn);
    }
    njScale(NULL, 2.7F, 2.7F, 2.7F);
    draw_tree(object);
    njPopMatrix(1);
    if (TASK_TIMER(work)++ > 3) {
        glint.x = RANDOM(-5.0F, 5.0F);
        glint.z = RANDOM(-5.0F, 5.0F);
        glint.y = RANDOM(-5.0F, 5.0F);
        /* @hack spelling: glint and the place through their addresses, which
         * SHC takes anew for each sum (MOV R15,R3 and ADD #8,R3 from
         * 0x0cb94c28); as members, 46 of 161 words differ */
        (&glint)->x += (&work->pos)->x;
        (&glint)->y += (&work->pos)->y;
        (&glint)->z += (&work->pos)->z;
        sparkle_create_glint(&glint, 0.35F, 0.35F);
        work->timer = 0;
    }
}
