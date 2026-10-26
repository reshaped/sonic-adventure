/* Perfect Chaos, the boss */
#ifndef B_CHAOS7_BOSS_H
#define B_CHAOS7_BOSS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* his modes, as his work's mode holds them and chaos7_set_mode asks for
 * them */
enum chaos7_boss_mode {
    /* the mode he starts in: his first step makes his record and sets him
     * up */
    BOSS_SETUP = 0,
    BOSS_READY = 1, /* the fight picks his attacks only in this mode */
    BOSS_MODE_2 = 2,
    BOSS_RAY = 3, /* he turns toward player 0 and fires a ray along node 15 */
    /* he breathes a beam at player 0, and six at the sea between them */
    BOSS_BREATH = 4,
    BOSS_TORNADOES = 5, /* three tornadoes spiral out from him */
    BOSS_MODE_6 = 6,
    BOSS_RUN = 7, /* a run path carries player 0 in at him */
    BOSS_HIT = 8, /* he breaks up, a node at a time */
    BOSS_BROKEN = 9, /* he draws nothing, after BOSS_HIT */
    /* his forming figure plays. then his body shows, and parts of him fade
     * in */
    BOSS_FORM = 10,
    /* parts of him fade, then his forming figure plays backward */
    BOSS_MELT = 11,
    BOSS_MELTED = 12, /* he draws nothing, after BOSS_MELT */
    BOSS_MODE_13 = 13,
    BOSS_MODE_14 = 14,
    BOSS_EVENT_RAY = 16 /* a ray as long as an event asks */
};

/* his health as he sets up and as the fight's second part starts, in units
 * of the life gauge */
#define BOSS_HEALTH 6

/* the volume the fight passes with some of its sounds. sound_play and
 * sound_play_held, which take it, ignore their volume */
extern Sint32 chaos7_volume;
/* the matrices of his beams' nodes, from each node into the world in his
 * last pose. his step sets them. his breath's seven beams start at the
 * first seven, nodes 16, 10, 9, 12, 11, 4 and 6. his ray starts at
 * chaos7_beam_matrices[CHAOS7_RAY_BEAM], node 15's */
#define CHAOS7_RAY_BEAM 7
extern NJS_MATRIX chaos7_beam_matrices[];
/* the matrices of his body's nodes, from each node into the world in his
 * last pose. his step sets them. SHC lays out bss by each name's first
 * declaration, so this declaration follows chaos7_beam_matrices' */
extern NJS_MATRIX chaos7_node_matrices[];

/* his step, for a task with a work. his first step allocates his record
 * and keeps it in the task's extra. it sets boss_work and chaos_record, and
 * gives the task his draw and his destructor. while the record can't be
 * allocated, each step tries again. player 0 must exist while it steps */
void chaos7_boss_step(struct task *task);
/* unless he's in mode already, set boss_next_mode to mode and
 * boss_last_mode to the mode he's in */
void chaos7_set_mode(int mode);
/* knock player 0 back, and flash the screen white through
 * chaos7_fade_start. in the fight's second part, an attack fewer than 30
 * steps away is put back to 60 steps away */
void chaos7_knock_player_back(void);
/* end the mode he's in, as his step ends one, and put him in BOSS_HIT at
 * its start. task is his task. boss_next_mode stays as it is: unless it is
 * BOSS_HIT, his next step ends BOSS_HIT again. nothing in the image calls
 * it */
void fun_0c916df8(struct task *task);

#endif
