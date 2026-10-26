/* lock-ons: the two cubes that turn round each target E-102 has locked on
 * to, and its number */
#ifndef PLAYER_LOCKON_H
#define PLAYER_LOCKON_H
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/polygon.h"
#include "system/task.h"

struct e102;

/* a lockon_target flag, set once the task is hit. E-102 locks on to no
 * task with it. a shot from his laser at such a task flies on through it
 * once lockon_release lets its lock-on go. enemy/enemy.h has an enemy's
 * other flags */
#define LOCKON_HIT 0x1000

/* the record, in place of task_motion, of a task E-102 can lock on to.
 * enemy_alloc_record makes an enemy's, with defaults. lockon_step makes a
 * zeroed one for a target that has none */
struct lockon_target {
    struct task_motion motion;
    struct task *lockon; /* the lock-on round it, or null */
    /* the task's own destructor, kept here while e102_aim.c's destructor
     * takes its place. that destructor releases the lock-on, then calls
     * this one */
    void (*destructor)(struct task *task);
    Uint8 unk_48;
    Sint8 last_mode; /* an enemy's work mode before its last change */
    /* unk_4a, unk_4c, unk_74 to unk_8c and unk_1e4 have no one meaning.
     * each kind of target names them through macros of its own */
    Sint8 unk_4a;
    Uint8 unk_4b;
    Sint16 unk_4c;
    Uint16 flags;
    NJS_POINT3 home; /* where it was placed */
    NJS_POINT3 target; /* where it heads */
    NJS_POINT3 last_pos; /* where it was the step before */
    Float unk_74;
    Float unk_78;
    Float unk_7c;
    Float unk_80;
    Float unk_84;
    Float unk_88;
    Float unk_8c;
    Uint8 unk_90[0xc];
    /* it meets the world at probe_offset from its pos, turned with it, and
     * reaches top above, bottom below and radius across */
    NJS_VECTOR probe_offset;
    Float top;
    Float radius;
    Float bottom;
    Float step_down; /* the deepest ledge it steps down */
    Float wall_bounce; /* the part of its speed a wall sends back */
    Float floor_bounce; /* the part of its speed the ground sends back */
    Float friction; /* the part of its speed it keeps on the ground */
    /* enemy_alloc_record sets it to 0.1, and some kinds of target set their
     * own. no C reads it */
    Float unk_c4;
    Angle wall_turn; /* how far a wall turns it */
    Float shadow_size;
    Float shadow_ratio; /* its shadow's length to width */
    struct sides_query ground; /* the surfaces around it */
    Float scale_speed; /* how fast its scale springs back toward 1 */
    /* the heading saru, the monkey enemy, chooses. other kinds of enemy use
     * it their own way */
    Angle heading;
    Angle turn_speed; /* how far it turns toward aim_y in a step */
    Angle phase; /* an angle each kind of enemy advances its own way */
    Angle angles[8]; /* each kind of enemy indexes these its own way */
    Angle aim_y; /* the heading it turns toward */
    /* it sees a player within sight_distance and within sight_angle of
     * the way it faces, or within near_distance any way. both distances
     * are kept squared */
    Angle sight_angle;
    Float sight_distance;
    Float near_distance;
    Float frame; /* its frame in action */
    Float unk_1e4;
    Float next_frame; /* its frame in next_action, which it blends into */
    Float blend; /* how far it has blended into next_action, 0 to 1 */
    Float blend_speed; /* added to blend each step */
    NJS_ACTION *action;
    NJS_ACTION *next_action;
    NJS_MATRIX matrix; /* as it was last drawn */
};

#define LOCKON_TARGET(task) ((struct lockon_target *)(task)->motion)

/* a shape flag: a target whose first shape has it shows no number, and
 * counts toward no bonus */
#define LOCKON_NO_NUMBER 0x100000

/* a lock-on's extra, which E-102's aim fills as it makes the lock-on */
struct lockon_extra {
    Sint8 marked; /* TRUE once E-102 marks it for the volley he fires */
    Sint8 number; /* the number it shows */
    Sint8 unk_02;
    Uint8 unk_03;
    Sint16 upgrades; /* E-102's, as he locked on */
    Uint8 unk_06[2];
    struct task *target; /* null once the target lets it go */
    struct task_work *player; /* E-102's work */
    struct e102 *e102;
    struct task *shot; /* the shot flying at it, or null */
    Uint8 unk_18[4];
    /* the radius of the target's first shape, or 10 for a target with no
     * collision */
    Float radius;
    struct angles ang; /* the cubes turn by y and z */
    struct task *bonus; /* the bonus its shot counts toward */
    void (*step)(struct task *task); /* the target's step as it was locked on */
};

/* E102TIME's digits, colon, e102_getsecbg and e102_lockon0, eight pixels
 * square, for numbers drawn in the world */
extern NJS_SPRITE lockon_sprite;

/* a lock-on's step. its maker fills its extra first: target, number, e102
 * and upgrades */
void lockon_step(struct task *task);
/* the shot flying at a lock-on, and the lock-on a shot flies at. each
 * returns null for a null task or for none */
struct task *lockon_get_shot(struct task *lockon);
struct task *lockon_of_shot(struct task *shot);
/* target lets its lock-on go: the lock-on ends on its next step, and the
 * two forget each other. with E-102's laser, a shot at a target with
 * LOCKON_HIT then flies on. a null target, or one with no lock-on, is
 * left as it is */
void lockon_release(struct task *target);
/* lockon_release, after which the shot at the lock-on flies on once the
 * lock-on ends. lockon_release_hit's lock-on ends on its next step, and
 * lockon_release_hit_fading's fades for 60 steps first */
void lockon_release_hit(struct task *target);
void lockon_release_hit_fading(struct task *target);

#endif
