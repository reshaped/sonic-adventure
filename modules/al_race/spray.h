/* the Chao race's sprays of animated drops */
#ifndef AL_RACE_SPRAY_H
#define AL_RACE_SPRAY_H
#include <ninja.h>
#include <sg_xpt.h>

/* the most drops a spray holds */
#define SPRAY_DROPS 5

/* a drop's flags. one outside DROP_KEPT lasts until the end of the next
 * spray_step */
#define DROP_LOOPED 0x1 /* its frames wrapped to the first this step */
#define DROP_KEPT 0xf
#define DROP_SPRAYING 0x10 /* the spray throws drops this step */

/* drop modes. DROP_SHOWN and DROP_FLYING are drawn, and both flying modes
 * animate */
enum drop_mode { DROP_WAIT, DROP_SHOWN, DROP_FLYING, DROP_FLYING_HIDDEN };

struct spray_drop {
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    Uint8 frame;
    Sint8 frame_timer;
    Uint8 mode; /* an enum drop_mode */
    Uint8 flags;
    /* a step count: spray_step adds 1 each step, after the move, which may
     * set it */
    Uint16 timer;
    Uint8 unk_1e[2];
};

/* a drop's move at each spray_step, given the position and velocity the
 * step takes, either of which may be null */
typedef void (*spray_move)(struct spray_drop *drop, NJS_POINT3 *pos,
                           NJS_VECTOR *speed);

struct spray {
    struct spray_drop drops[SPRAY_DROPS];
    NJS_TEXLIST *texlist;
    NJS_TEXANIM *texanim;
    Float scale;
    spray_move move;
    Uint16 frame_steps; /* steps each texture frame shows */
    Uint8 frames; /* the frames in texanim */
    Uint8 count; /* the drops in use */
};

/* what spray_init sets a spray up from */
struct spray_kind {
    NJS_TEXLIST *texlist;
    NJS_TEXANIM *texanim;
    Float scale;
    Uint8 frame_steps;
    Uint8 frames;
    Uint8 count;
};

/* set spray up from kind, with at most SPRAY_DROPS drops, and start each drop
 * with init unless init is null. move moves the drops at each spray_step.
 * kind's frame_steps is unread: spray->frame_steps keeps what it held */
void spray_init(struct spray *spray, struct spray_kind *kind,
                void (*init)(struct spray_drop *drop), spray_move move);
/* step each drop: animate it as drop_mode says, then move it with pos and
 * speed. pos and speed may be null */
void spray_step(struct spray *spray, NJS_POINT3 *pos, NJS_VECTOR *speed);
void spray_draw(struct spray *spray);
void spray_set_move(struct spray *spray, spray_move move);
void spray_set_flags(struct spray *spray, int flags);
/* add gravity to the drop's y velocity, take drag's share of the velocity off
 * it, then move the drop by it */
void spray_fall(struct spray_drop *drop, Float gravity, Float drag);
/* set spray up as three water drops in 1ST_READ.BIN's splash textures. a
 * waiting drop starts to fly from the spray's position only while
 * DROP_SPRAYING is set. the splash is the only kind of spray the race makes */
void spray_init_splash(struct spray *spray);

#endif
