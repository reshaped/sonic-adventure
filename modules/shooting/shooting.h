/* the sky chase's shared state: the course's path and scenes, the tornado's
 * flight, and the stage task's mode */
#ifndef SHOOTING_SHOOTING_H
#define SHOOTING_SHOOTING_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the sound effects more than one source plays */
#define SE_ZAKO_NEAR 670 /* a scene's zako or renzako nearing player 0 */
#define SE_HIT 672 /* a shot hitting a foe, a shell or the carrier */
#define SE_HODAI 675 /* a hodai firing */
#define SE_SHOT_DOWN 682 /* a kirai, an arm or the carrier shot down */

/* how the tornado flies. tornado.c sets it. the camera reads its offset
 * and ang, and a shot its ang */
struct shooting_flight {
    NJS_VECTOR offset; /* from pos, along the axes of the path's turn */
    NJS_POINT3 pos; /* the path's point it's offset from */
    Float speed; /* the length of the point's last move, set by tornado_fly */
    Float alpha; /* of the constant material it draws with */
    struct angles ang; /* on top of the path's own turn */
    Uint8 unk_2c[8];
    int bullets; /* left of the burst */
    int phase; /* steps round 0 to 3: a bullet goes at 0 */
    /* steps a fire button has been held, as tornado_count_charge counts them */
    int charge;
};

/* a scene of the course. its actor flies along path while the tornado's
 * point is from *begin up to *end. path[i] is the actor's place while the
 * tornado's point is *begin + i */
struct shooting_scene {
    int kind; /* SCENE_ZAKO or SCENE_RENZAKO. any other makes a zako */
    NJS_POINT3 *path;
    int *begin;
    int *end;
};

#define SCENE_ZAKO 1
#define SCENE_RENZAKO 2

/* a scene's actor, and whether the tornado's point is in its scene */
struct shooting_actor {
    struct task *task;
    Sint32 active;
};

/* shooting_actors' length: 511 actors, then the null task that ends the
 * list */
#define SHOOTING_ACTORS 512

/* the work->vars of a scene's actor and of its gun. as an actor falls, its
 * work->scl is its velocity */
struct scene_vars {
    /* with spin_y and spin_z, what it adds to its angles each step as it
     * falls */
    Uint16 spin_x;
    Uint16 spin_y;
    Sint16 timer; /* steps left to fall, or a gun's steps to its next shot */
    Sint16 spin_z;
    Uint8 unk_08;
    Uint8 sounded; /* set once SE_ZAKO_NEAR has played */
    Sint8 scene; /* its index in shooting_scenes and shooting_actors */
    Sint8 frame; /* the frame its action draws at */
};

#define SCENE_VARS(work) ((struct scene_vars *)&(work)->vars)

/* a scene actor's modes */
enum scene_mode {
    SCENE_INIT = 0,
    SCENE_FLY = 1, /* the tornado's point is in its scene */
    SCENE_WAIT = 2, /* the point is outside its scene */
    SCENE_FALL = 3, /* a shot hit it */
    SCENE_GONE = 5 /* the point passed its scene, or its fall ended */
};

/* the stage task's modes. shooting_stage_mode takes the mode at the end of
 * each step */
enum shooting_mode {
    SHOOTING_INIT,
    SHOOTING_FLY,
    SHOOTING_DOWN, /* the tornado is shot down */
    SHOOTING_CLEAR, /* the course is flown, and the foes clear out */
    SHOOTING_SCORE,
    SHOOTING_EVENT1, /* act 1's event at the course's end */
    SHOOTING_EVENT2, /* act 2's event half way */
    SHOOTING_AFTER, /* an event is over */
    SHOOTING_SECOND, /* act 2, after its event */
    /* shot down in act 1. the tornado's point moves on until SHOOTING_DOWN */
    SHOOTING_FALL,
    SHOOTING_END
};

/* SHC lays out bss in the order of each name's first declaration. so this
 * header declares every global of shooting.c in the image's order, even
 * those no other source names */
extern struct shooting_flight shooting_flight;
/* the index in shooting_path of the tornado's point */
extern int shooting_path_index;
/* while set, the stage task moves shooting_path_index on by one each step
 * of SHOOTING_FLY, SHOOTING_FALL and SHOOTING_SECOND. the tornado also
 * keeps within its reach of the path while it is set. nothing in this image
 * clears it */
extern Bool shooting_path_running;
/* an entry for each scene's actor, made as the act starts. nothing in
 * this image reads the list */
extern struct shooting_actor shooting_actors[SHOOTING_ACTORS];
extern int shooting_actor_count;
extern NJS_POINT3 *shooting_path;
extern int shooting_path_count;
extern int shooting_act; /* 0 for the first act, 1 for the second */
/* 3 in act 1, and 4 in act 2. nothing in this image reads it */
extern int dat_0cad53d8;
/* the tornado's lives. a hit takes half of one */
extern Float shooting_lives;
extern int shooting_score;
/* 0 in act 1. 1 and 2 in act 2, before and after its event */
extern int shooting_course_part;
/* nothing in this image reads or writes these */
extern int dat_0cad53e8;
extern int dat_0cad53ec;
/* a request to the stage task, which nothing in this image sets. in act 1's
 * SHOOTING_FLY, REQUEST_EVENT1 starts the event and REQUEST_CLEAR enters
 * SHOOTING_CLEAR. in act 2's SHOOTING_FLY or SHOOTING_SECOND, REQUEST_EVENT2
 * starts the event. in those modes, the act sets shooting_request back to 0
 * whatever it held */
enum { REQUEST_EVENT1 = 1, REQUEST_EVENT2 = 2, REQUEST_CLEAR = 3 };
extern int shooting_request;
extern int shooting_stage_mode;
/* the course's scenes, ending at a null path */
extern struct shooting_scene *shooting_scenes;
/* each act starts dat_0cad53fc at -1 and dat_0cad5400 at 0xff07. nothing
 * in this image reads either */
extern int dat_0cad53fc;
extern int dat_0cad5400;

/* the stage task's step for act 1, and for act 2 */
void shooting_act1_step(struct task *task);
void shooting_act2_step(struct task *task);
/* shooting_actors_clear empties shooting_actors. shooting_actors_add puts
 * task at shooting_actor_count, and a null task after it. the count holds
 * only these adds, and a full list takes no more. nothing in this image
 * calls either */
void shooting_actors_clear(void);
void shooting_actors_add(struct task *task);
/* TRUE while the stage task is in SHOOTING_EVENT1, SHOOTING_EVENT2 or
 * SHOOTING_SCORE, else FALSE */
Bool shooting_event_playing(void);

#endif
