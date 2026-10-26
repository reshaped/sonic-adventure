/* lights: the stage's directional light, the lights tasks bring, and the
 * act's lighting */
#ifndef DRAW_LIGHT_H
#define DRAW_LIGHT_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* a light for light_create's task to make. type is njCreateLight's, and its
 * bits say which of the fields after pos apply */
struct light_spec {
    Sint32 type;
    Float specular, diffuse, ambient;
    NJS_ARGB color; /* its alpha goes unread */
    /* the task's offset from its parent's position, which the parent's
     * angles don't turn. the light takes no location from it */
    NJS_POINT3 pos;
    NJS_VECTOR direction; /* with NJD_DIR_LIGHT or NJD_SPOT_LIGHT */
    Float near, far; /* with NJD_POINT_LIGHT or NJD_SPOT_LIGHT */
    Angle inner, outer; /* with NJD_SPOT_LIGHT */
};

/* the stage's directional light */
extern NJS_LIGHT dir_light;

/* the step of a task that makes dir_light, then copies it to ninja's easy
 * light each step after. the task's end deletes dir_light */
void light_stage_step(struct task *task);
/* create a child of parent that, in its first step, makes the ninja light
 * at light from spec. the task's end deletes it. the task ends itself once
 * the stage or act differs from its first step's. parent must not be null.
 * light must last until the task ends, and spec until its first step. null
 * if the child can't be created */
struct task *light_create(struct task *parent, NJS_LIGHT *light,
                          struct light_spec *spec);

/* the act's lighting, which the light editor edits: its light, its light
 * sources, and the first six of draw_palettes. a sources file holds the
 * sources and then the light's settings. a palettes file on the card fills
 * the first six palettes, and one on the disc fills draw_palettes from its
 * start. the disc's PL_Q1B.bin, PL_Y1B.bin and PL_Y2B.bin are 16768 bytes,
 * longer than draw_palettes. read in whole sectors, each runs on over all of
 * draw_unlit_offsets, which follows draw_palettes */

/* write the palettes or the sources to the card. the file's name is
 * SONICADV_P or SONICADV_S, then stage << 3 | act as two base-32 digits,
 * each 0 to 9 or A to V. -1 if the header or the request fails. otherwise
 * wait for the drive and return 0, without checking the write's result */
int light_write_palettes(int stage, int act);
int light_write_sources(int stage, int act);
/* load the current act's lighting, and set the lights from it. the sources
 * and palettes come from the card when both files are there. otherwise both
 * start from defaults, and the disc's files are read over them. it turns
 * the light editor off */
void light_load_act(void);
/* read the disc file named name into dest, in whole 2048-byte sectors,
 * which dest must have room for. TRUE after the read, or FALSE if the file
 * doesn't open. the read's own result goes unchecked */
Bool light_read_disc(const char *name, void *dest);
/* read the sources or the palettes for stage and act from the disc, as
 * light_read_disc does, over the act's lighting. without the act's file,
 * each reads act 0's. TRUE if the act's file or act 0's opens. a file's
 * name starts SL_ for sources or PL_ for palettes. the stage's and the act's
 * digits and B.bin follow, as in PL_60B.bin for stage 6's act 0. the stage's
 * digit is 0 to 9 or A to Z. from stage 36 on, the _ turns to 1 and the
 * digit stands for the stage less 26 */
Bool light_read_disc_sources(Uint16 stage, int act);
Bool light_read_disc_palettes(Uint16 stage, int act);
/* set the lights from the act's light. the models' light and ninja's easy
 * light take its direction turned by the current matrix, and the act's
 * ninja light takes it unturned. then it takes the queued point lights with
 * draw_take_point_lights */
void light_apply(void);
/* 1 if the first listed light source reaches the model placed by the
 * current matrix, else 0. a source reaches the model when its range plus
 * the model's radius exceeds the distance between them. it moves the light
 * editor's work point to the model's center */
int light_place_point_and_test(NJS_MODEL *model);

#endif
