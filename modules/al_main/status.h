/* how time, fruit and animals change what a Chao's record keeps: its likes
 * and feelings, growth and type, parts and skills. two debug pages print
 * records. a record's minimal is a set of animal kinds, kind k at bit 14 - k.
 * alife_take_animal adds to it the kind of each animal a Chao takes */
#ifndef AL_MAIN_STATUS_H
#define AL_MAIN_STATUS_H
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "chao/chao.h"
#include "system/task.h"

/* for each record in GARDEN_EGG_CARRIER's list, print LIKE at the start of
 * row 15, and the record's like for Sonic along the row. the likes stand
 * five columns apart from column 5. nothing calls it */
void status_likes_print(void);
/* the status page, which shows one Chao of the entries' Chao list, or none.
 * each press of pad 0's Y moves it to the next Chao, newest first, and past
 * the last to none. for the Chao it shows, it draws a line from player 0 and
 * a turning pointer above the Chao. it prints the Chao's record and the
 * timers too. pad 0's up raises the record's v_pos by 0.05 a press, and down
 * lowers it. right raises its h_pos by 0.05 a press, and left lowers it.
 * each stays within -2 to 2. the Chao's task must keep a struct alife_chao
 * at its motion, and player 0's work must exist */
void status_page_step(void);
/* does nothing */
void fun_0c90d634(struct task_work *work, struct alife_chao *chao);
/* add amount to the record's like for alife_character and to its
 * user_like. each sum wraps to a signed byte, and is then clamped to -100
 * to 100. the like's raised count rises by 1, to 200, and like_changes by
 * 1, to 9999 */
void status_raise_like(struct task_work *work, struct alife_chao *chao,
                       int amount);
/* take amount from the record's like for alife_character and from its
 * user_like, each wrapped and clamped as status_raise_like does. a life above 0
 * falls by 1, like_changes rises by 1, to 9999, and pleasure becomes 0. the
 * raised count stays as it was */
void status_hurt(struct task_work *work, struct alife_chao *chao, int amount);
/* step the timers status_step reads. a timer whose count is above its
 * limit restarts at 0 and ticks this step. any other count rises by 1,
 * without a tick */
void status_timers_step(void);
/* step the timers, then each record in the three gardens' lists but eggs and
 * empty records, by the timers that tick. "away" below means in a garden other
 * than alife_garden.
 * - life: an adult other than chaos loses old / 200 + 1 life, to no less than
 *   0, and its old rises by 1. where the loss would take it below 0 and the
 *   Chao is away, a Chao with a user_like above 80 is reborn, with a life of 0
 *   and an old of 1. chao_list_remove takes any other off its list, and it
 *   becomes CHAO_NOTHING
 * - emotion: each like's raised count falls by 1, to no less than 10. a count
 *   of 0 wraps to 255 instead. pleasure and unk_6b fall by 5, anger by
 *   5 - (agres + 100) / 40, sorrow by 5 - (100 - curio) / 40, and fear by
 *   5 - (100 - agres) / 40. each of the five stays at 0 or more
 * - growth: flag 0x1 of the record's flags is set. away, growth rises by 0.008
 *   for a child and 0.004 otherwise, to 1.5. a child whose growth then passes 1
 *   takes its adult type, as status_evolve_record chooses it
 * - age: age rises by 1
 * - breed: an adult other than chaos gains (400 - unk_65) * 0.015 breed,
 *   truncated, to 200. where the gain takes it past 200 and the Chao is away,
 *   an egg comes at a chance of 1 in 10, if chao_count gives the Chao's garden
 *   2 to 7 Chao
 * - sleep: sleep rises by (unk_64 + 200) * 0.025, truncated, to 200
 * - hunger and tedious: in alife_garden, each rises by 1, to 200
 *
 * the egg is the first empty record, if any, which chao_init_egg resets and
 * chao_list_add puts in the parent's garden's list. neither sets the egg's
 * garden, which stays as the empty record held it */
void status_step(void);
/* choose the record's adult type from its h_pos and v_pos. with both
 * between -0.3 and 0.3, the type is standard. it is chaos instead for a
 * record whose minimal holds every animal kind, with a user_like above 90
 * and a generation of 2 or more. otherwise h_pos + v_pos and h_pos - v_pos
 * choose it:
 * - swim: a sum below 0, and a difference of 0 or more
 * - fly: a sum of 0 or more, and a difference below 0
 * - run: both 0 or more
 * - power: both below 0
 * then the record's growth, h_pos and v_pos shrink to a tenth, and the
 * Chao's take the same values */
void status_evolve(struct task_work *work, struct alife_chao *chao);
/* choose the record's adult type and shrink its growth, h_pos and v_pos as
 * status_evolve does, with no Chao to copy them to */
void status_evolve_record(struct chao *record);
/* clamp the Chao's h_pos and v_pos to -1.5 to 1.5 and its growth to 0 to
 * 1.5. the record's life, skills and hp_max stay at most 9999, and its hp
 * at most hp_max */
void status_clamp(struct task_work *work, struct alife_chao *chao);
/* give the Chao parts from an animal of kind, below ANIMAL_KINDS, slot by
 * slot, from child_part_objects for a child and adult_part_objects
 * otherwise. the first three slots' nodes in the Chao's shape take a model,
 * and the other slots' nodes take a child node.
 * - a slot the table gives a tree takes it at animal_part_chances' chance
 *   for kind and the slot's part. its node takes the tree's model or child,
 *   and the record's parts takes kind for the part. the diffuse colors of
 *   the model's materials, or of the child's model's, turn white and keep
 *   their alpha. those materials are the table's, which every Chao with the
 *   part shares
 * - a slot the table gives no tree is emptied at a chance of 4 in 10. its
 *   node loses its model, or takes the node after it as its child, and the
 *   record's part becomes -1
 * the second slot of a pair takes the first's outcome, with its own tree,
 * which must exist wherever the first's does */
void status_give_parts(struct task_work *work, struct alife_chao *chao,
                       Uint8 kind);
/* add to the record's skills what an animal of kind gives. below ANIMAL_MOGU,
 * each skill rises by the kind's bonus for it, doubled where the record's type
 * is the skill's. ANIMAL_ZOU adds nothing. from ANIMAL_MOGU on, one skill at
 * random rises by 100 */
void status_give_skills(struct task_work *work, struct alife_chao *chao,
                        Uint8 kind);
/* add kind, which must be below ANIMAL_KINDS, to the record's minimal */
void status_add_minimal(struct task_work *work, struct alife_chao *chao,
                        Uint8 kind);
/* the Chao eats a fruit of kind. status_raise_like adds 3 to its like and
 * user_like. its hunger falls by 50, to 0, and its sleep and pleasure rise
 * by 50 and 40, each to 200. the Chao's growth rises by 0.03 for a child
 * and 0.016 otherwise, and the record's life by 5. then kind adds more:
 * - 1: hp_max rises by 120, hp fills, and breed becomes 200
 * - 2 to 5: power, swim, run or fly rises by 20, hp by 120 and hp_max by 40
 * - 6: every skill rises by 20 and hp_max by 80, and hp fills
 * - 7: the Chao's unk_0e8 becomes 3600
 * - 8: agres rises by 20, to 100, and anger becomes 200
 * - 9: sleep rises to 150 at least, and fear and sorrow become 200
 * - 10: hp and hp_max rise by 40
 * any other kind adds nothing. status_clamp and alife_shape_blend follow */
void status_eat_fruit(struct task_work *work, struct alife_chao *chao,
                      Sint8 kind);
/* lean the Chao toward the skill of an animal of kind, read as a Uint8. the
 * kinds fall in five groups of three, in enum order. the first four lean toward
 * swim, fly, run and power. swim lowers v_pos and fly raises it; run raises
 * h_pos and power lowers it. a child leans by 0.03. an adult leans by 0.0225
 * where its type is the group's skill, and by 0.015 otherwise. either lean
 * doubles while the record's skill is below its points. the fifth group leans
 * one of the four ways at random, by 0.15 for a child and 0.075 for an adult.
 * the Chao's growth rises by 0.015 for a child and 0.008 for an adult, doubled
 * where the lean doubles. status_clamp and alife_shape_blend follow */
void status_lean(struct task_work *work, struct alife_chao *chao, int kind);
/* TRUE when the record's minimal holds kind, which must be below
 * ANIMAL_KINDS. otherwise FALSE, and the Chao's work returns to MODE_IDLE
 * with submode and timer 0, and the record's tedious rises by 20, to 200 */
Bool status_require_minimal(struct alife_chao *chao, int kind);
/* TRUE when the record's minimal holds kind, which must be below
 * ANIMAL_KINDS. it changes nothing, and nothing calls it */
Bool status_in_minimal(struct alife_chao *chao, int kind);

#endif
