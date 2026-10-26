/* common: what the menus chose, the game's calls on the npc, battles and
 * title cards, and the starts and steps the objects share */
#ifndef OBJECT_COMMON_H
#define OBJECT_COMMON_H
#include <ninja.h>
#include <sg_xpt.h>

#include "save/save.h"
#include "system/task.h"

struct battle;

struct model_vars {
    NJS_OBJECT *object;
    Uint8 unk_04[4];
    NJS_TEXLIST *texlist; /* the kind's, or null when the stage has a texlist */
};

struct action_vars {
    NJS_ACTION *action;
    Float frame;
    NJS_TEXLIST *texlist; /* the kind's, or null when the stage has a texlist */
};

struct world_model_vars {
    /* the copy of model that model_start_world puts in the world outside
     * the set editor, posed as the task */
    NJS_OBJECT *world;
    /* the kind's model. with the stage's texlist, model_start_world points
     * it at the copy of world it adds to object_init's tree */
    NJS_OBJECT *model;
    NJS_TEXLIST *texlist; /* the kind's, or null when the stage has a texlist */
};

/* work's vars as each struct above lays them out. a cast in place lets SHC
 * reach a field at one displacement from work, as the image does. through
 * a pointer to the vars, SHC adds their offset first */
#define MODEL(work) ((struct model_vars *)&(work)->vars)
#define ACTION(work) ((struct action_vars *)&(work)->vars)
#define WORLD_MODEL(work) ((struct world_model_vars *)&(work)->vars)

/* 1 when member lies at the same offset in type as in task_work, else 0.
 * type is an object's own struct for its work. such an object tests the
 * members it shares with task_work this way as it starts */
#define WORK_SAME_PLACE(type, member)                                          \
    (&((struct task_work *)0)->member == &((type *)0)->member)

/* what the menus chose */
struct menu_state {
    /* while set, the menus open every character, stage and choice, whatever
     * the save holds */
    /* @hack type: all_open, trial and summary as arrays of one, whose
     * address SHC loads into the register the byte is then read into (R0 in
     * sonic_board_score at 0x8c0ad8e8). as plain bytes, the address takes a
     * register of its own. summed over the 25 functions that use them, 67 of
     * 1774 words differ, and so do 9 of the 17 of card_open_done, which
     * follows one of them */
    Uint8 all_open[1];
    Uint8 trial[1]; /* the kind of trial picked, a MENU_TRIAL_ */
    /* the summary's voice file, 1 to 8, picked by the clock's second from
     * those the memory cards hold, or 0 for none */
    Uint8 summary[1];
    Uint8 card_noticed; /* 1 once the menus have said no card is in */
    Uint8 unk_04[5];
    Uint8 save; /* the save picked, an index into menu_saves */
    Uint8 mode; /* the item picked on the menus' first screen, a MENU_MODE_ */
    Uint8 wheel_story; /* the story the wheel was left at */
    /* while it's nonzero, the card screen skips its check for a card. the
     * menus raise it each time their first screen opens, and each time the
     * drive screen goes on without a card */
    Uint8 mode_visits;
    /* the story picked on the wheel of characters, as enum wheel_story
     * numbers them: 6 is the last story */
    Uint8 story;
    Uint8 stage; /* the stage and act picked from the trial's list */
    Uint8 act;
    Uint8 unk_10[2];
};

/* menu_state.mode: the items on the menus' first screen */
#define MENU_MODE_ADVENTURE 0
#define MENU_MODE_TRIAL 1
#define MENU_MODE_OPTIONS 2
#define MENU_MODE_INTERNET 3
/* menu_state.trial: a trial of action stages, or of mini games such as
 * icecap's board */
#define MENU_TRIAL_STAGE 0
#define MENU_TRIAL_MINI_GAME 1

/* the menus' module, ADVERTISE, writes it, and the game reads it.
 * boot_finish clears it */
extern struct menu_state menu_state;
/* dat_8c793fc4 and menu_saves are declared here only for their place in
 * bss. SHC lays bss out in the order of each name's first declaration.
 * here the two come after menu_state and before npc_task, as in the image.
 * if common.c declared them first, they would come after npc_task and
 * npc_disabled, which this header declares */
extern Sint32 dat_8c793fc4;
extern struct save menu_saves[SAVE_COUNT];
/* the npc's task, which has Tails follow player 0, or null while none runs */
extern struct task *npc_task;
/* TRUE while Tails is kept from following: once out of sight, he stays
 * out */
extern Bool npc_disabled;

/* choose the npc npc_start starts, by the d-pad's left and right on port
 * B, wrapping round. it prints the choice's name at loc */
void npc_choose(Uint32 loc);
/* start the chosen npc: Tails following player, or a battle of player
 * against a rival */
void npc_start(int unused, struct task *player);
/* the story picked on the menus' wheel. character is unused */
int menu_get_story(int character);
/* take Tails away for an event, as npc_remove_tails does. returns TRUE
 * once his task is killed or with no npc, and FALSE when player 1 is
 * missing or isn't Tails */
Bool npc_leave(void);
/* bring Tails back after the event, as npc_create_tails does. returns his
 * new task, or null with no npc or when task_create fails */
struct task *npc_return(void);
/* keep Tails from following, and let him follow again */
void npc_disable(void);
void npc_enable(void);
struct task *battle_begin(struct battle *battle);
struct task *battle_create_rival(struct battle *battle);
void battle_remove_rival(struct battle *battle);
/* how the last battle stands. each rival's task sets BATTLE_RUNNING as it
 * starts, then how the battle ends */
enum battle_outcome {
    BATTLE_RUNNING,
    BATTLE_WON, /* the rival is beaten */
    BATTLE_LOST /* player 0 is gone, or down */
};

void battle_set_outcome(enum battle_outcome outcome);
enum battle_outcome battle_outcome(void);
/* load the stage's title card to show for frames, 180 if 0, as
 * title_card_begin does. frames must be under 255. returns 0, or -1 when
 * there's no card or it won't load */
Sint16 title_card_load(int frames);
/* draw the card, as title_card_step does. returns 1 once it's done and
 * released, else 0 */
Sint16 title_card_draw(int unused);
/* clear menu_state, and forget the npc's task */
void boot_finish(void);
/* menu_state.mode, and the save menu_state.save picks from menu_saves */
int menu_get_mode(void);
struct save *menu_get_save(void);

/* in a carried thing's work flags */
#define CARRY_GROUND 0x1 /* it rests where it landed */
#define CARRY_WALLS 0x2 /* it lands only on ground; a wall ends its move */
#define CARRY_HOLDABLE 0x40 /* a player can take hold of it */
#define CARRY_HELD 0x1000 /* a player holds it */

/* the kinds of carried thing, which carry_set_kind sets as work->id. by
 * player_carry_flags, Big picks up the small, medium and large ones and
 * pushes the medium and large ones. Sonic, Tails, Knuckles, Amy and E-102
 * pick up the small one and push the medium one. each of the six shakes
 * CARRY_KIND_SHAKE */
enum carry_kind {
    CARRY_KIND_SMALL = 4,
    CARRY_KIND_MEDIUM,
    CARRY_KIND_LARGE,
    CARRY_KIND_SHAKE
};

/* how far from a player a carried thing is held, kept in the motion's
 * drive. carry_set_kind sets it by kind */
#define CARRY_REACH(motion) ((motion)->drive)

/* set work's carry flags and id, and motion's reach and gravity, by kind.
 * motion may be another record with Floats where a task_motion keeps drive
 * and gravity. returns TRUE, or FALSE with nothing changed for a null
 * motion or a kind outside carry_kind */
Bool carry_set_kind(struct task_work *work, struct task_motion *motion,
                    enum carry_kind kind);
/* the step of a small carried thing. unless a player holds it, it falls by
 * motion's gravity and slows to a stop on the ground. outside the set
 * editor, it moves as far as a ball of radius 2.5 at its base can sweep
 * through the world. where the ball stops, it lands and takes the surface's
 * slope. in the air it levels out. an unheld thing outside the set editor
 * registers its collision and forgets its touches */
void carry_step_small(struct task *task);
/* the step of a large carried thing: a small one's, but its ball's radius
 * is 5. it drags more in the air, and a wall doesn't slow it. on the ground
 * it sweeps and moves only on a frame's first step. it forgets its touches
 * every step, held or not */
void carry_step_large(struct task *task);
/* the step of a carried thing that doesn't move by itself. it registers
 * its collision and forgets its touches */
void carry_step_still(struct task *task);
/* clear COLLISION_TOUCHED in work's flags, and motion's accel unless
 * motion is null */
void object_clear_touch(struct task_work *work, struct task_motion *motion);
/* turn vector in place from the world's frame into the task's, the
 * inverse of object_turn */
void object_turn_back(struct task_work *work, NJS_VECTOR *vector);
/* turn vector in place by work's angles, from the task's frame into the
 * world's. the matrix rotates about z, then x, then y */
void object_turn(struct task_work *work, NJS_VECTOR *vector);
/* ready the objects for a stage. object_init makes the tree that draws
 * world models' copies with the stage's texlist. a task at level 3 draws
 * the tree, and frees it as the task ends. then object_init resets the
 * placements and the records some kinds keep. without heap room for the
 * tree's root, it makes no tree and resets nothing */
void object_init(void);
/* the starts of the kinds that are only a model or an action. a kind's
 * step calls one with work's vars set as the start's struct lays them out.
 * it returns TRUE when the task is out of range, as placement_try_leave
 * does. otherwise it loads the vars' texlist, or sets it null when the
 * stage has one, and returns FALSE.
 *
 * model_start, model_start_scaled, model_start_solid and
 * model_start_solid_scaled draw the model at once. with the stage's texlist
 * and outside the set editor, each gives the task its own step, draw and
 * destructor. otherwise the task keeps the kind's step, which calls the
 * start again. without the stage's texlist, each call loads the texlist
 * again */

/* a model that only draws, and one drawn scaled by one more than work->scl
 * on each axis. a scale of 0 draws the model at its own size */
Bool model_start(struct task *task);
Bool model_start_scaled(struct task *task);
/* the same, whose own step also registers the collision the kind gave the
 * work */
Bool model_start_solid(struct task *task);
Bool model_start_solid_scaled(struct task *task);
/* a model the players stand on. outside the set editor, it puts a copy of
 * model in the world, posed as the task, as WORLD_SOLID with the
 * WORLD_SIZE its radius picks. world_object_alloc must have an object to
 * lend: with none, this writes through null. it always gives the task its
 * own step and destructor. with the stage's texlist and a copy in the
 * world, object_init's tree draws a copy of that. otherwise, or when the
 * tree can't take one, the task draws the model itself */
Bool model_start_world(struct task *task);
/* an action played over and over, a frame each step. it always gives the
 * task its own step, draw and destructor */
Bool model_start_action(struct task *task);
/* scale each shape of work's collision by one more than work->scl. its
 * center scales on all three axes, its radius by x, size_y by y and size_z
 * by z. any axis of scl below -1 is raised to -1 first, so no factor is
 * negative. work must have a collision */
void object_scale_shapes(struct task_work *work);
/* put model in the world under flags, posed as the task, and return the
 * object that holds it. the object comes from world_object_alloc. it takes
 * model's model, child and sibling, and model's evalflags but for
 * NJD_EVAL_UNIT_POS and NJD_EVAL_UNIT_ANG. its scl stays as its last
 * borrower left it. the pool must have an object to lend: with none, this
 * writes through null */
NJS_OBJECT *object_add_world(NJS_OBJECT *model, struct task *task,
                             Sint32 flags);

#endif
