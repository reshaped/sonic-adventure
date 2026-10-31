/* E-102's shots at his lock-ons, and the trails, flashes and blasts that go
 * with them */
#ifndef PLAYER_E102_SHOT_H
#define PLAYER_E102_SHOT_H
#include <ninja.h>
#include <sg_xpt.h>

struct e102;
struct player;
struct task;
struct task_work;

/* a shot's work flag. lockon_release sets it on a shot of his laser whose
 * target is LOCKON_HIT, and the shot leaves a sphere where it ends */
#define SHOT_PIERCED 0x100

/* the work as a shot sees it */
struct shot_work {
    Sint8 mode;
    Uint8 unk_01[3];
    Sint16 flags;
    Uint16 upgrades; /* E-102's upgrades when he fired it */
    struct task_work *player; /* E-102's work */
    /* how hard it turns toward its lock-on: 0 on its first step, then 5,
     * rising by 0.5 a step */
    Float pull;
    struct task *lockon; /* the lock-on it flies at, or NULL */
    Sint32 steps; /* since it was fired */
    Uint8 unk_18[8];
    NJS_POINT3 pos;
    NJS_POINT3 scl;
};

#define SHOT(task) ((struct shot_work *)(task)->work)

/* one step of his shooting. till his aim is ready, he faces his nearest
 * lock-on. once it's ready, he marks every lock-on with a target still there
 * and no shot at it, and starts a volley bonus for them. he faces each in turn,
 * nearest first, fires at it and kicks back. each step empties his lists of
 * lock-ons and shots, which their own steps fill again */
void e102_shoot(struct task *task, struct player *record, struct e102 *e102);
/* let the shot go. it flies straight on with no lock-on, at *speed, or at the
 * speed it has if speed is NULL. it ends once it's out of range or its 600
 * steps run out */
void e102_release_shot(struct task *shot, NJS_VECTOR *speed);
#ifdef GCC
/* the work of the E-102 who fired task, or NULL where task isn't one of
 * his shots or hasn't taken its first step */
struct task_work *e102_shot_shooter(struct task *task);
#endif
/* the effects of a shot leaving his cannon, without the shot itself. it
 * makes a flash and a blast at e102->shot_pos, and a puff of smoke at each
 * of e102->points. pad 0 shakes for a player's shot, unless an event has
 * control */
void e102_fire_effects(struct task *task, struct player *record,
                       struct e102 *e102);

#endif
