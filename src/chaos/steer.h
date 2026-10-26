/* each Chaos form's parameters, and steering a Chaos */
#ifndef CHAOS_STEER_H
#define CHAOS_STEER_H
#include <ninja.h>
#include <sg_xpt.h>

struct task_work;

/* the forms, by the boss stage each fights in, as indexes of chaos_forms.
 * CHAOS_FORMS counts its rows: a sixth, past CHAOS_7, repeats Chaos 0's */
#define CHAOS_0 0
#define CHAOS_2 1
#define CHAOS_4 2
#define CHAOS_6 3
#define CHAOS_7 4
#define CHAOS_FORMS 6

struct chaos_form {
    Sint8 hits; /* the health it starts with */
    Float height; /* how far it melts down */
    Float model_y; /* how high its model sits above its position */
    Float speed; /* how far chaos_steer moves it a step */
    /* the most chaos_steer and chaos_face_player turn it a call */
    Angle turn;
    NJS_POINT3 start; /* where chaos_face_off puts it */
    Float unk_20;
    /* how far out its points spread as it melts, times the square of the
     * deformer's scale */
    Float melt_spread;
    /* a melt's stagger. let s be the deformer's scale, and j a point's
     * index in its model. the point sinks at most
     * s * height * (1 + (j & 7) / melt_grain). no point sinks lower than
     * model_y below the model's origin. a point already lower rises to that
     * level */
    Float melt_grain;
    Float unk_2c;
    Float unk_30;
    Float stretch_speed; /* how fast Chaos 2's arm stretches out and back */
    Sint16 unk_38;
    Sint16 alpha; /* the alpha of its translucent models, in percent */
    Float unk_3c;
    Float unk_40;
    NJS_POINT3 center; /* the arena's center */
    Float full_scale; /* its puddle's scale at full size */
    /* the attack_timer Chaos 2, 4 and 6 start with. Chaos 2 resets it to
     * this plus a random count below attack_wait_random */
    Sint16 attack_wait;
    Sint16 attack_wait_random;
    Float orbit_radius; /* the widest its emeralds circle */
    Float orbit_height; /* how far its emeralds rise to circle */
};

/* each form's row, by CHAOS_0 to CHAOS_7 */
extern struct chaos_form chaos_forms[];

/* turn work toward record's target by at most the form's turn, then move it
 * ahead by the form's speed. the speed is added to chaos_travel too. record
 * is the boss's struct chaos, and unused is unread */
void chaos_steer(struct task_work *work, void *unused, void *record);
/* turn work by at most the form's turn toward the heading to player 0, less
 * record's facing. record is the boss's struct chaos */
void chaos_face_player(struct task_work *work, void *record);
/* the heading from work's position to the player's */
Angle chaos_angle_to_player(struct task_work *work, int player);

#endif
