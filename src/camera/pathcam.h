/* the paths the PATHCAM mode plays, by stage and act */
#ifndef CAMERA_PATHCAM_H
#define CAMERA_PATHCAM_H
#include <ninja.h>
#include <sg_xpt.h>

/* a key of a PATHCAM path: the camera's angles and position */
struct pathcam_key {
    Sint16 ang_y;
    Sint16 ang_x;
    Sint16 ang_z;
    Uint8 unk_06[2];
    NJS_POINT3 pos;
};

struct pathcam_path {
    Sint32 unk_00;
    /* how far the camera plays the keys, in keys. once its place reaches
     * length, it starts again at keys[0]. keys holds length + 1 keys, so a
     * place short of length has a key after it */
    Float length;
    struct pathcam_key *keys;
};

/* the stage_act that ends pathcam_routes */
#define PATHCAM_END 0xffff

/* the PATHCAM paths of one stage and act */
struct pathcam_route {
    /* the stage and act, as stage_get_stage_act gives them, or PATHCAM_END */
    Uint16 stage_act;
    Uint16 count;
    struct pathcam_path **paths;
};

/* a route for each stage and act with PATHCAM paths, then one whose
 * stage_act is PATHCAM_END */
extern struct pathcam_route pathcam_routes[];

#endif
