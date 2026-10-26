/* Zero's banks, read into the memory past the module's end: EROBO.PB's
 * textures, and EROBO.NB's models and motions. each arm and action here
 * holds offsets into the model bank, which erobo_load turns into
 * pointers */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_robo/banks.h"
#include "system/binary.h"
#include "system/loader.h"

/* the end of the module's bss, from the table bounds.src makes */
extern Uint8 *eggmanrobo_bss_end;

/* the banks' alignment in memory */
#define BANK_ALIGN 32

/* each action's object and motion, as offsets into the model bank */
static struct njb_action beam_offsets = {
#include "b_robo/banks/beam_offsets.inc"
};
static struct njb_action wind_up_offsets = {
#include "b_robo/banks/wind_up_offsets.inc"
};
static struct njb_action throw_offsets = {
#include "b_robo/banks/throw_offsets.inc"
};
static struct njb_action throw_end_offsets = {
#include "b_robo/banks/throw_end_offsets.inc"
};
static struct njb_action hand_out_offsets = {
#include "b_robo/banks/hand_out_offsets.inc"
};
static struct njb_action hand_home_offsets = {
#include "b_robo/banks/hand_home_offsets.inc"
};
static struct njb_action hit_front_offsets = {
#include "b_robo/banks/hit_front_offsets.inc"
};
static struct njb_action hit_front_land_offsets = {
#include "b_robo/banks/hit_front_land_offsets.inc"
};
static struct njb_action hit_behind_offsets = {
#include "b_robo/banks/hit_behind_offsets.inc"
};
static struct njb_action hit_behind_land_offsets = {
#include "b_robo/banks/hit_behind_land_offsets.inc"
};
static struct njb_action turn_around_offsets = {
#include "b_robo/banks/turn_around_offsets.inc"
};
static struct njb_action leap_offsets = {
#include "b_robo/banks/leap_offsets.inc"
};
static struct njb_action leap_fall_offsets = {
#include "b_robo/banks/leap_fall_offsets.inc"
};
static struct njb_action walk_offsets = {
#include "b_robo/banks/walk_offsets.inc"
};
static struct njb_action stand_offsets = {
#include "b_robo/banks/stand_offsets.inc"
};
static struct njb_action shocked_offsets = {
#include "b_robo/banks/shocked_offsets.inc"
};
static struct njb_action knocked_down_offsets = {
#include "b_robo/banks/knocked_down_offsets.inc"
};
static struct njb_action hurt_offsets = {
#include "b_robo/banks/hurt_offsets.inc"
};
static struct njb_action get_up_offsets = {
#include "b_robo/banks/get_up_offsets.inc"
};
static struct njb_action spin_offsets = {
#include "b_robo/banks/spin_offsets.inc"
};

/* the arms' objects, their models and children as offsets into the model
 * bank */
static NJS_OBJECT arm0_template = {
#include "b_robo/banks/arm0_template.inc"
};
static NJS_OBJECT arm1_template = {
#include "b_robo/banks/arm1_template.inc"
};

Uint8 *erobo_models = NULL;
NJS_TEXLIST *erobo_textures = NULL;
NJS_OBJECT erobo_arm0_object;
NJS_OBJECT erobo_arm1_object;
NJS_ACTION erobo_beam_action;
NJS_ACTION erobo_wind_up_action;
NJS_ACTION erobo_throw_action;
NJS_ACTION erobo_throw_end_action;
NJS_ACTION erobo_hand_out_action;
NJS_ACTION erobo_hand_home_action;
NJS_ACTION erobo_hit_front_action;
NJS_ACTION erobo_hit_front_land_action;
NJS_ACTION erobo_hit_behind_action;
NJS_ACTION erobo_hit_behind_land_action;
NJS_ACTION erobo_turn_around_action;
NJS_ACTION erobo_leap_action;
NJS_ACTION erobo_leap_fall_action;
NJS_ACTION erobo_walk_action;
NJS_ACTION erobo_stand_action;
NJS_ACTION erobo_shocked_action;
NJS_ACTION erobo_knocked_down_action;
NJS_ACTION erobo_hurt_action;
NJS_ACTION erobo_get_up_action;
NJS_ACTION erobo_spin_action;

/* point action at the object and the motion that offsets gives in the
 * model bank */
static void rebase_action(struct njb_action *offsets, NJS_ACTION *action)
{
    action->object = (NJS_OBJECT *)(erobo_models + offsets->object);
    action->motion = (NJS_MOTION *)(erobo_models + offsets->motion);
}

void erobo_load(void)
{
    Uint8 *memory = (Uint8 *)(((Uint32)eggmanrobo_bss_end + BANK_ALIGN - 1) &
                              ~(BANK_ALIGN - 1));

    /* pvb_load loads the textures out of the texture bank, and the model
     * bank is then read over it */
    loader_read_file("EROBO.PB", memory);
    erobo_textures = (NJS_TEXLIST *)pvb_load(memory);
    erobo_models = memory;
    loader_read_file("EROBO.NB", memory);
    njb_relocate(erobo_models);
    rebase_action(&beam_offsets, &erobo_beam_action);
    rebase_action(&wind_up_offsets, &erobo_wind_up_action);
    rebase_action(&throw_offsets, &erobo_throw_action);
    rebase_action(&throw_end_offsets, &erobo_throw_end_action);
    rebase_action(&hand_out_offsets, &erobo_hand_out_action);
    rebase_action(&hand_home_offsets, &erobo_hand_home_action);
    rebase_action(&hit_front_offsets, &erobo_hit_front_action);
    rebase_action(&hit_front_land_offsets, &erobo_hit_front_land_action);
    rebase_action(&hit_behind_offsets, &erobo_hit_behind_action);
    rebase_action(&hit_behind_land_offsets, &erobo_hit_behind_land_action);
    rebase_action(&turn_around_offsets, &erobo_turn_around_action);
    rebase_action(&leap_offsets, &erobo_leap_action);
    rebase_action(&leap_fall_offsets, &erobo_leap_fall_action);
    rebase_action(&walk_offsets, &erobo_walk_action);
    rebase_action(&stand_offsets, &erobo_stand_action);
    rebase_action(&shocked_offsets, &erobo_shocked_action);
    rebase_action(&knocked_down_offsets, &erobo_knocked_down_action);
    rebase_action(&hurt_offsets, &erobo_hurt_action);
    rebase_action(&get_up_offsets, &erobo_get_up_action);
    rebase_action(&spin_offsets, &erobo_spin_action);
    erobo_arm0_object = arm0_template;
    erobo_arm0_object.model =
        (NJS_MODEL *)(erobo_models + (Uint32)arm0_template.model);
    erobo_arm0_object.child =
        (NJS_OBJECT *)(erobo_models + (Uint32)arm0_template.child);
    erobo_arm1_object = arm1_template;
    erobo_arm1_object.model =
        (NJS_MODEL *)(erobo_models + (Uint32)arm1_template.model);
    erobo_arm1_object.child =
        (NJS_OBJECT *)(erobo_models + (Uint32)arm1_template.child);
}

void erobo_release(void)
{
    pvb_release((struct pvb_texlist *)erobo_textures);
    erobo_textures = NULL;
}
