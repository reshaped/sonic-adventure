/* Sonic Adventure's part of the engine's C ABI, beside sa_engine.h's core,
 * which knows no game. it takes the numbers the core leaves to a game. C17
 * and C++17 compile it alike. its records hold one layout on every target,
 * as the core's do */
#ifndef SA_SA1_H
#define SA_SA1_H

#include "sa_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { SA_GAME_SA1 = 1 };

/* the form of a live draw's geometry, sa_draw_live's format. a strip's z is
 * a 1/z, as njDrawPolygon and njDrawTexture take it */
enum {
    SA_SA1_BASIC = 1, /* a Ninja basic model, NJS_MODEL */
    SA_SA1_SPRITE = 2, /* NJS_SPRITE, at the frame the draw's state names */
    SA_SA1_POLYGON = 3, /* a strip on the screen of NJS_POLYGON_VTX */
    SA_SA1_TEXTURE = 4, /* a strip on the screen of NJS_TEXTURE_VTX */
    SA_SA1_POLYGON_2D = 5, /* njDrawPolygon2D's NJS_POINT2COL */
    SA_SA1_LINE_2D = 6, /* njDrawLine2D's NJS_POINT2COL */
    /* a line's two ends in view space, NJS_POINT3COL, as njDrawLine3D clips
     * them */
    SA_SA1_LINE_3D = 7,
    SA_SA1_CHARACTER = 8, /* a character of Ninja's print */
    /* a basic model lit by one light, one intensity a point, as
     * njEasyDrawModel takes it */
    SA_SA1_EASY = 9
};

/* SA1's records: a task's birth and death, and the palettes a picture's
 * draws shade from */
enum {
    SA_REC_TASK_CREATE = 3,
    SA_REC_TASK_DESTROY = 4,
    /* 5 to 10 and 24 are reserved */
    SA_REC_PALETTES = 23
};

/* a child's level, which it doesn't have */
#define SA_TASK_CHILD 0xffU
/* the records task_create gave a task */
enum {
    SA_TASK_WORK = 0x1,
    SA_TASK_MOTION = 0x2,
    SA_TASK_FORCE = 0x4,
    SA_TASK_EXTRA = 0x8
};
/* a task as a capture first meets it, ahead of the first draw that names
 * it. its serial counts the tasks the capture has met, from 1, and is the
 * instance its draws carry */
typedef struct sa_task_create {
    sa_record_head head;
    uint32_t task; /* its serial */
    /* the parent's serial, or 0 for a root and for a parent not yet met */
    uint32_t parent;
    uint32_t creator; /* 0 */
    uint32_t step; /* its step's name id */
    uint8_t level; /* 0 to 7, or SA_TASK_CHILD */
    uint8_t records; /* SA_TASK_WORK and the others */
    uint16_t reserved; /* 0 */
} sa_task_create;
/* task_destroy took a task the capture has met, ahead of its destructor and
 * its children */
typedef struct sa_task_destroy {
    sa_record_head head;
    uint32_t task;
    uint32_t destructor; /* the name id of the destructor it runs, or 0 */
} sa_task_destroy;

/* SA1's shade, the mode the core leaves to a game. a point takes its base
 * and offset colors from the palette sa_shade.palette names, as sa_palettes
 * says. under SA_MAT_SPECULAR it takes its offset from the palette after. an
 * unlit point of a blended material takes white, and an unlit textured
 * point adds its unlit offset. a meshset whose material is under
 * SA_MAT_WHITE takes white for its base color. each meshset after it does
 * too, whatever its own material. model.src's unlit blended meshset whitens
 * every point. the model's later meshsets draw from those points */
enum { SA_SHADE_PALETTE = 0 };
/* the palettes a picture's draws shade from. dot is a point's normal dotted
 * with the draw's light. the point takes entry (int)(dot * 127.5 + 127.5),
 * held to 0 to 255. an entry is a base color, then an offset color, each
 * ARGB. after the palettes come the unlit offsets, by point index, as many
 * as src/draw/model.h's DRAW_MOST_POINTS */
#define SA_PALETTES 8
#define SA_PALETTE_ENTRIES 256
#define SA_UNLIT_OFFSETS 2048
typedef struct sa_palettes {
    sa_record_head head;
    uint32_t words[SA_PALETTES * SA_PALETTE_ENTRIES * 2 + SA_UNLIT_OFFSETS];
} sa_palettes;
/* the palettes the draws after it shade from */
sa_result sa_present_palettes(sa_engine *engine, const sa_palettes *palettes);

/* SA1's scene, sa_scene_begin's: an act's land, as the land's task holds it.
 * its name is the stage in the high byte and the act in the low. a part's
 * number, under SA_INSTANCE_SCENE, is a land entry's place in the land's
 * table, from 0. the land's moving parts' models count on from the entries,
 * in the order the task draws them. a part comes under
 * SA_SHADE_PALETTE, and shades from the palettes of the picture that shows
 * it */

/* SA1's views, sa_present_view's. its camera sets Ninja's view at each step
 * of a frame. a draw names Ninja's view as it is made. a draw ahead of the
 * camera's first step finds the view the frame before ended under.
 * sa_picture.view is the view most of the picture's draws in the world were
 * made under. of two that tie, it is the earlier. where the picture holds no
 * draw in the world, it is the view its first draw found */

/* SA1's scene's lights, sa_scene_lighting's: the act's light and its
 * sources. they are as the game's variables hold them when the land's task
 * takes the land. light_load_act fills them from the act's sources file, on
 * the card or the disc. a change of act in the Mystic Ruins or the Past
 * loads none. there they are what the stage's own code last read. the light
 * points straight down, turned by the file's two angles as njRotateY and
 * then njRotateZ turn it. its color and three strengths are the file's. the
 * sources are those of the file's 30 that it marks used, in its order. the
 * game reads a source's rate, from 0 to 1, in its light editor alone. a
 * source's second position isn't carried, since only
 * light_place_point_and_test, which nothing calls, measures from it. no draw
 * of SA1's reads a source. a model's points take their light from the light
 * as play turns it and from the act's palettes */

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#define SA_SA1_LAYOUT(type, bytes)                                             \
    static_assert(sizeof(type) == (bytes), #type "'s layout")
#else
#define SA_SA1_LAYOUT(type, bytes)                                             \
    _Static_assert(sizeof(type) == (bytes), #type "'s layout")
#endif
SA_SA1_LAYOUT(sa_task_create, 24);
SA_SA1_LAYOUT(sa_task_destroy, 12);
SA_SA1_LAYOUT(sa_palettes, 24580);
#undef SA_SA1_LAYOUT

#endif
