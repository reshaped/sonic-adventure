/* the cannons' shells, which fly an arc toward where they're aimed. each
 * lands with a blast or a burst */
#ifndef STG06_SHELL_H
#define STG06_SHELL_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* a deck target, which sky_deck_shell_fire_at hands to its spec's arrive */
struct sky_deck_target;

/* a spec's flags. SHELL_SPOT draws a spot at the target, which grows as the
 * shell nears. SHELL_BURST lands a shell at its target with a burst, not a
 * blast. SHELL_SHADOW casts a shadow on the floor below, and lands the shell
 * on reaching that floor. a SHELL_HARMLESS shell attacks nothing, and flies
 * through whatever it touches */
#define SHELL_SPOT 0x1
#define SHELL_BURST 0x2
#define SHELL_SHADOW 0x4
#define SHELL_HARMLESS 0x8

/* how a cannon's shells fly and land */
struct shell_spec {
    Sint16 flags;
    Sint16 unk_02;
    Float gravity; /* added to a shell's speed.y each step */
    /* the size of the shell and its sphere, trail, spot and shadow, and of
     * its blast or burst */
    Float scale;
    /* called as a shell reaches its target, with the entry
     * sky_deck_shell_fire_at took and the shell's position and speed.
     * sky_deck_shell_fire takes a spec without one */
    void (*arrive)(struct sky_deck_target *target, NJS_POINT3 *pos,
                   NJS_VECTOR *speed);
};

/* fire a shell from pos at speed. the shell's steps are its distance to target
 * over its speed. both are taken in x and z. a shell that hasn't landed sooner
 * is then put at target and lands there. sky_deck_shell_fire takes a spec
 * without an arrive, and sky_deck_shell_fire_at one with an arrive, which it
 * hands entry. return the shell's task, or NULL when task_create fails or the
 * spec's arrive is the wrong kind. the wrong kind still leaves a task without a
 * record. its first step reads the record through NULL. the record's allocation
 * isn't checked */
struct task *sky_deck_shell_fire(NJS_POINT3 *pos, NJS_POINT3 *target,
                                 NJS_VECTOR *speed, struct shell_spec *spec);
struct task *sky_deck_shell_fire_at(NJS_POINT3 *pos, NJS_POINT3 *target,
                                    NJS_VECTOR *speed,
                                    struct sky_deck_target *entry,
                                    struct shell_spec *spec);
/* draw the spot a shell is aimed at, 1 above pos and scaled by sx, sy and
 * sz */
void sky_deck_spot_draw(NJS_POINT3 *pos, Float sx, Float sy, Float sz);

#endif
