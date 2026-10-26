/* the rivals the Chao race makes, and the task for each Chao in the race */
#ifndef AL_RACE_RACE_RIVAL_H
#define AL_RACE_RACE_RIVAL_H
#include <sg_xpt.h>

struct chao;
struct task;

/* create and return a task at level 3, with work, that runs step. the task
 * holds a struct alife_chao for record at its motion, as alife_chao_alloc
 * gives it. it borrows record, which must stay valid until the task is
 * destroyed. null when the pool or the heap runs out. when the heap runs out,
 * alife_chao_alloc destroys the task, and race_create_chao still writes its
 * step */
struct task *race_create_chao(void (*step)(struct task *task),
                              struct chao *record);

/* the modes race_rival_init sets a rival up in. RIVAL_BLACK_EGG to
 * RIVAL_GOLD_EGG are the egg modes. to vary a value is to move it at random
 * by up to 1% either way, then keep it from 1 to 9999 */
enum race_rival_mode {
    RIVAL_VARIED, /* each skill source's, varied */
    /* each skill at random from 1600 to 2399, or from 2000 to 2799 in a race
     * of RACE_KIND_LAST, and hp and hp_max one random value below 10000.
     * RIVAL_RANDOM_2 sets a rival up as RIVAL_RANDOM does */
    RIVAL_RANDOM,
    RIVAL_RANDOM_2,
    RIVAL_BLACK_EGG, /* each skill 9999, CHAO_BEGG, and one fixed name */
    /* each skill 9999 and CHAO_GROWTH_DUE, with CHAO_PAL and a silver egg's
     * rival's name, or with CHAO_GOLD and a gold egg's */
    RIVAL_SILVER_EGG,
    RIVAL_GOLD_EGG
};

/* set rival up in mode, an enum race_rival_mode. RIVAL_VARIED and the egg
 * modes read source, and a null source makes any mode but RIVAL_RANDOM_2
 * RIVAL_RANDOM.
 *
 * outside RIVAL_RANDOM and RIVAL_RANDOM_2, hp and hp_max are both source's
 * hp_max, varied. outside the egg modes, CHAO_GROWTH_DUE is set at random, the
 * likelier the lower the skills' average, and race_rival_name_by_type names
 * rival by its type.
 *
 * the skills set the type in RIVAL_RANDOM and RIVAL_RANDOM_2. in the others
 * they set it where source's type & 0x7f isn't CHAO_CHILD. below an average of
 * 200 that type is CHAO_CHILD. from 200 up, it is the highest skill's type
 * where that skill is more than 280 over the average, else CHAO_STANDARD. of
 * equal skills, the first of fly, swim, run and power counts as the highest.
 * where the skills don't set the type, it is CHAO_STANDARD in RIVAL_SILVER_EGG
 * and RIVAL_GOLD_EGG and CHAO_CHILD in the others.
 *
 * the other flags and race_wins are cleared. h_pos, v_pos, growth, life,
 * old, kind, agres, curio, charm, breed, sleep, hunger, tedious, unk_64 and
 * unk_65 are random. kind, agres and curio are 0 in RIVAL_BLACK_EGG instead,
 * and sleep, unk_64 and unk_65 are 198 in RIVAL_SILVER_EGG. each of parts is a
 * random animal or none, and none in the egg modes. outside RIVAL_BLACK_EGG,
 * index picks the name, and must not be negative. the rest of rival stays as
 * it was */
void race_rival_init(struct chao *rival, struct chao *source, int mode,
                     int index);

/* copy into name, with its null, entry index % 8 of the eight rival names
 * for type. CHAO_CHILD has a table of its own, and every other type shares
 * another. index must not be negative */
void race_rival_name_by_type(Uint8 *name, int type, int index);
/* the same, from the gold egg's rivals' table where gold is nonzero, and
 * from the silver egg's where it is 0 */
void race_rival_name_by_egg(Uint8 *name, int gold, int index);

#endif
