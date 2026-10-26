/* helpers Casinopolis's objects share, and its props */
#ifndef STG09_HELPERS_H
#define STG09_HELPERS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

struct collision_shape;

/* casino_any_below returns TRUE when player 0, player 1 or the camera is at
 * or below y. casino_any_above returns TRUE when one is at or above y.
 * casino_any_in_cylinder returns TRUE when one is in a cylinder about center.
 * the cylinder reaches height above and below center, and radius2 is its
 * radius squared. each returns FALSE otherwise. a missing player is passed
 * over, but camera_work must be set */
Bool casino_any_below(Float y);
Bool casino_any_above(Float y);
Bool casino_any_in_cylinder(NJS_POINT3 *center, Float radius2, Float height);
/* put a copy of object in the collision world for task, under flags and
 * WORLD_SOLID. the copy holds object's model and its child nodes, unscaled, at
 * the work's position and y angle. radius + 16 picks its size, from WORLD_SMALL
 * for 30 or less to WORLD_HUGE past 200. the work's first var keeps a pointer
 * to the copy. world_object_alloc lends the copy, and with all 256 of its
 * objects lent, this writes through null */
void casino_world_add(struct task *task, NJS_OBJECT *object, Float radius,
                      Sint32 flags);
/* move the copy to the work's position. set the work's WORLD_TESTED while
 * player 0 or 1 is near, and clear it otherwise. near is within radius + 16
 * of the model's center, placed by the work's position and y angle. the
 * copy keeps its angle, and must have a model. without a copy, this clears
 * WORLD_TESTED */
void casino_world_test_near(struct task *task, Float radius);
/* give the copy model, and test as casino_world_test_near does. this also
 * turns the copy to the work's y angle. a null model leaves the copy without
 * one and clears WORLD_TESTED */
void casino_world_set_model(struct task *task, NJS_MODEL *model, Float radius);
/* take the copy out of the collision world and give it back to
 * world_object_alloc. call it once for each copy: the work keeps its
 * pointer. a second call would give back a copy world_object_alloc may have
 * lent again */
void casino_world_remove(struct task *task);

/* turn the task's y angle to ang. player 0's force record then carries
 * player 0 round the work's position with the turn. it also carries player 0
 * on by move, unless move is null. without player 0, the force's speed is
 * move alone. the task needs TASK_FORCE */
void casino_turn_to(struct task *task, NJS_VECTOR *move, Angle ang);
/* set player 0's force record as casino_turn_to does for a turn of turn,
 * without turning the task. the force carries player 0 round only while
 * player 0 is on land. otherwise its speed is move alone, and its turn 0.
 * Knuckles climbing the copy is moved directly instead, and the force is left
 * as it was. he moves round the work's position by turn, out or in to radius
 * across, and on by move. that holds while he's more than 0.1 across from the
 * work's position, and within 10 of radius. the task needs TASK_FORCE */
void casino_carry_turn(struct task *task, NJS_VECTOR *move, Angle turn,
                       Float radius);
/* 1 for a turn to the right, when the work's truncated scl.x is odd, and 0
 * otherwise. the truncated scl.x must fit an int */
/* @hack type: the Sint8 cast, which gives the image's EXTS.B after the FTRC,
 * as at 0x0c90bbfc in gear_run. without it, 457 of the 814 words of gear_run,
 * kaz_run, kaz_f_run, tube_a_run, tube_b_run and tube_c_run differ */
#define TURNS_RIGHT(work) ((Sint8)(int)(work)->scl.x & 1)

/* casino_draw_sphere draws marker_object at pos, scaled to radius.
 * casino_draw_cylinder draws marker_cylinder_object at pos, scaled to radius
 * across and to reach height above and below pos */
void casino_draw_sphere(NJS_POINT3 *pos, Float radius);
void casino_draw_cylinder(NJS_POINT3 *pos, Float radius, Float height);

struct uv_scroll {
    NJS_MESHSET *meshset;
    Sint16 count; /* the meshset's UVs */
    Sint16 step_u; /* the move for each unit of phase */
    Sint16 step_v;
    Sint16 unk_0a;
    NJS_TEX *own; /* the meshset's own UVs, as uv_scroll_save saves them */
};

/* uv_scroll_save saves each row's meshset's own UVs. uv_scroll_move copies
 * each row's UVs into room, moved by phase times its step. it then points the
 * row's meshset at the copy. it does both only on a frame's first step, while
 * frame_step is 0. room must hold every row's UVs. uv_scroll_move takes phase
 * modulo 1024, and each move modulo 4096. uv_scroll_restore points each
 * meshset at its own UVs again, and uv_scroll_release does nothing */
void uv_scroll_save(struct uv_scroll *rows, int count);
void uv_scroll_move(struct uv_scroll *rows, int count, NJS_TEX *room,
                    int phase);
void uv_scroll_restore(struct uv_scroll *rows, int count);
void uv_scroll_release(struct uv_scroll *rows, int count);

struct uv_shift {
    NJS_MESHSET *meshset;
    Sint16 count; /* the meshset's UVs */
    Sint16 shift_u;
    Sint16 shift_v;
    Sint16 users; /* tasks using the shifted copy */
    NJS_TEX *shifted;
    NJS_TEX *own; /* the meshset's own UVs */
};

/* uv_shift_make counts a user of each row. a row's first user makes its
 * shifted copy on the heap. the game hangs when the heap can't hold it.
 * uv_shift_use points each row's meshset at its shifted copy, and
 * uv_shift_restore at its own UVs. uv_shift_free counts a user off each row,
 * and frees the copy with the last. a shift is taken modulo 4096 */
void uv_shift_make(struct uv_shift *rows, int count);
void uv_shift_use(struct uv_shift *rows, int count);
void uv_shift_restore(struct uv_shift *rows, int count);
void uv_shift_free(struct uv_shift *rows, int count);

struct texture_swap {
    NJS_MATERIAL *material;
    Uint32 texture; /* the attr_texId it swaps in */
    Uint32 own; /* the material's own, as texture_swap_save saves it */
};

/* texture_swap_save saves each row's material's own attr_texId.
 * texture_swap_use gives each material its row's texture, and
 * texture_swap_restore its own again. texture_swap_release does nothing */
void texture_swap_save(struct texture_swap *rows, int count);
void texture_swap_use(struct texture_swap *rows, int count);
void texture_swap_restore(struct texture_swap *rows, int count);
void texture_swap_release(struct texture_swap *rows, int count);

/* blend from toward to by blend, from 0 to 1, into out. each is a palette as
 * draw_palettes holds one: 512 words of base and offset colors. each byte of
 * out is from's times 1 - blend plus to's times blend, truncated */
void casino_blend_palette(Uint32 *from, Uint32 *to, Uint32 *out, Float blend);
/* casino_blend_palette from the palette at SET_AREA_4 toward the one at
 * SET_AREA_5, into draw_palettes[DRAW_PLAYER_PALETTE] */
void casino_blend_set_areas(Float blend);

/* raise dust at pos, by dust_create with speed and scale. a pos farther than
 * inset from the camera first moves inset toward it. camera_work must be
 * set */
void casino_dust_toward_camera(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale,
                               Float inset);

/* the props: objects of one tree, model or action, each made by its start.
 * each rule below holds unless a start's own comment says otherwise.
 *
 * a start out of range does nothing more. the task then destroys itself on
 * its next step, as placement_try_leave_by has it. in range, the start
 * gives the task the prop's draw and step, and both test the range again.
 * the range is the start's own, or else about 600 units of the camera and
 * player 0. a range is squared, as placement_try_leave_by takes it.
 *
 * a prop draws on a frame's first step, with OBJ_CASINO9's textures. it draws
 * at the work's position, turned by its y angle, and its step draws it too.
 *
 * outside the set editor, a start given shapes gives the work a collision
 * record of them. the record is of kind COLLISION_OBJECT, with count shapes,
 * and the step registers it */

/* model, drawn with draw_model. the start clears the work's x and z angles
 * outside the set editor */
void model_prop_start(struct task *task, NJS_MODEL *model,
                      struct collision_shape *shapes, int count);
/* object, scaled by the work's scl + 1 on each axis. scaled_zxy_prop_start's
 * prop turns by the work's z, x and y angles, in that order */
void scaled_prop_start(struct task *task, NJS_OBJECT *object);
void scaled_zxy_prop_start(struct task *task, NJS_OBJECT *object);
/* object, scaled as scaled_prop_start's is, with its shapes scaled to match.
 * outside the set editor, the start clears the work's x and z angles. it also
 * raises each axis of scl below -1 to -1 */
void scaled_solid_prop_start(struct task *task, NJS_OBJECT *object,
                             struct collision_shape *shapes, int count);
void ranged_scaled_solid_prop_start(struct task *task, Float range,
                                    NJS_OBJECT *object,
                                    struct collision_shape *shapes, int count);
/* object, within range + 1, whose root model's materials draw ignoring the
 * light. they draw lit while the truncated scl.x is odd, which the step reads
 * again in the set editor. the model must have at most ten materials: an
 * eleventh's would overwrite the draw's saved return address. the start
 * clears the work's x and z angles outside the set editor */
void unlit_prop_start(struct task *task, Float range, NJS_OBJECT *object,
                      struct collision_shape *shapes, int count);
/* object, within range + 1. the start clears the work's x and z angles
 * outside the set editor */
void ranged_prop_start(Float range, struct task *task, NJS_OBJECT *object,
                       struct collision_shape *shapes, int count);
/* object. the start clears the work's x and z angles outside the set
 * editor */
void solid_prop_start(struct task *task, NJS_OBJECT *object,
                      struct collision_shape *shapes, int count);
/* object, with a copy in the collision world outside the set editor. its
 * radius, rounded down, is the model's radius plus its center's distance from
 * the origin, and 20 more. the work's timer keeps it. outside the set editor,
 * the step calls casino_world_test_near. the destructor removes the copy */
void world_prop_start(struct task *task, NJS_OBJECT *object);
/* action, turned by the work's z, x and y angles in that order, at frame
 * scl.x. outside the set editor, the step moves the frame on by scl.y each
 * step. the frame goes back to 0 once it reaches the motion's last */
void motion_prop_start(struct task *task, NJS_ACTION *action,
                       struct collision_shape *shapes, int count);
/* object, spinning about y. each step turns it on by step, an angle, plus
 * scl.x degrees */
void spin_prop_start(struct task *task, NJS_OBJECT *object, int step);
/* object at rest, and action, played once through at |scl.x| frames a step.
 * play starts on a step whose record holds a hit from player 0's task.
 * player 0 must also be more than 0.2 nearer the prop than player_get_history
 * puts it 2 steps back. it must be on land, or standing on no task. the step
 * draws on every step, and in the set editor plays the action round. the
 * record takes one shape, the first of shapes. the start clears the work's x
 * and z angles outside the set editor */
void pushed_prop_start(struct task *task, NJS_OBJECT *object,
                       struct collision_shape *shapes, NJS_ACTION *action);
/* object, with count rows of UVs scrolling by the work's timer as phase. the
 * step moves the timer on by scl.z, truncated. the rows hold 200 UVs at most,
 * since the draw copies them to its stack. it draws only while a sphere of the
 * model's radius + 16 about the prop is in view. scroll_sound_prop_start's prop
 * also plays the sound effect se on each frame's first step. it plays se at
 * the work's position, with the work for its id */
void scroll_prop_start(struct task *task, NJS_OBJECT *object,
                       struct uv_scroll *rows, Sint8 count);
void scroll_sound_prop_start(struct task *task, NJS_OBJECT *object,
                             struct uv_scroll *rows, Sint8 count, int se);
/* object, with count rows of shifted UVs for each phase from 1 to phases.
 * phase 0 shows the meshsets' own UVs. the phase runs from 0 to phases, then
 * round to 0 again. each phase lasts scl.z + 2 steps, with scl.z truncated,
 * and 1 step at least. the work's timer counts them. it draws only while a
 * sphere of the model's radius + 16 about the prop is in view. the destructor
 * gives up the task's use of the shifted copies */
void shift_prop_start(struct task *task, NJS_OBJECT *object,
                      struct uv_shift *rows, Sint8 count, Sint8 phases);
/* shift_prop_start's prop, with a copy of object's root model in the
 * collision world. the copy is made in the set editor too. it takes
 * world_prop_start's radius, and the step calls casino_world_test_near. the
 * work's x angle counts the phase's steps. the destructor removes the copy
 * too */
void shift_world_prop_start(struct task *task, NJS_OBJECT *object,
                            struct uv_shift *rows, Sint8 count, Sint8 phases);
/* shift_world_prop_start's prop, but of action, whose root model the copy
 * takes. the action plays from frame 0, kept in scl.x, which the step moves on
 * by scl.y. it goes back to 0 once it reaches the motion's last frame. it
 * draws in view or out */
void shift_motion_prop_start(struct task *task, NJS_ACTION *action,
                             struct uv_shift *rows, Sint8 count, Sint8 phases);
/* shift_prop_start's prop, with shape_count shapes */
void shift_solid_prop_start(struct task *task, NJS_OBJECT *object,
                            struct collision_shape *shapes, int shape_count,
                            struct uv_shift *rows, Sint8 count, Sint8 phases);
/* object, with count rows of swapped textures for each phase from 0 to
 * phases - 1. it has shape_count shapes. the phase runs from 0 to phases - 1,
 * then round to 0 again. each phase lasts as long as shift_prop_start's. it
 * draws only in view, as that one does */
void swap_solid_prop_start(struct task *task, NJS_OBJECT *object,
                           struct collision_shape *shapes, int shape_count,
                           struct texture_swap *rows, Sint8 phases,
                           Sint8 count);
/* swap_solid_prop_start's prop, with a copy of object in the collision world
 * in place of shapes. the copy is made in the set editor too. its radius is
 * world_prop_start's, not rounded down, and the step calls
 * casino_world_test_near. the destructor removes the copy.
 * swap_turn_prop_start's prop also turns by scl.y degrees a step, carrying
 * player 0 as casino_turn_to does. its copy keeps the angle it started at,
 * and the task needs TASK_FORCE */
void swap_world_prop_start(struct task *task, NJS_OBJECT *object,
                           struct texture_swap *rows, Sint8 phases,
                           Sint8 count);
void swap_turn_prop_start(struct task *task, NJS_OBJECT *object,
                          struct texture_swap *rows, Sint8 phases, Sint8 count);

#endif
