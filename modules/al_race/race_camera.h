/* the race's camera, which places the view each step as its mode says */
#ifndef AL_RACE_RACE_CAMERA_H
#define AL_RACE_RACE_CAMERA_H
#include <ninja.h>
#include <sg_xpt.h>

struct camera_view;
struct race_game;

/* in a race camera's flags */
/* the mode called camera_override, which race_camera_release undoes */
#define RACE_CAMERA_OVERRIDE 0x1
/* on its last call, race_camera_avoid_walls stopped the point the follow
 * modes ease toward short of a wall. while the flag is set, they turn toward
 * 180 degrees round the racer, behind it */
#define RACE_CAMERA_WALLED 0x2
/* the follow modes turn toward the camera's heading on the next
 * race_camera_step, which clears the flag */
#define RACE_CAMERA_HEADING 0x4

/* RACE_CAMERA_HALL_FIXED's view, which the chao window's board stands in:
 * where the camera stands, and its turns about x and y */
#define HALL_VIEW_X 2088.0F
#define HALL_VIEW_Y 38.0F
#define HALL_VIEW_Z 0.0F
#define HALL_VIEW_ANG_X (-2730)
#define HALL_VIEW_ANG_Y 15834

/* a race camera's modes. on its first step, at phase 0, each mode that
 * places the camera calls camera_override and sets phase to 1. the mode puts
 * the camera in camera_place. the step the mode gives camera_override reads
 * camera_place. the racer is the one in the game's lane. an angle round the
 * racer counts from the way it faces. RACE_CAMERA_FOLLOW to
 * RACE_CAMERA_BEHIND are the follow modes */
enum race_camera_mode {
    RACE_CAMERA_NONE, /* places nothing */
    /* round player 0 in the hall, turned by the triggers. with no player 0,
     * the step releases the camera and sets RACE_CAMERA_NONE */
    RACE_CAMERA_HALL,
    RACE_CAMERA_HALL_FIXED, /* the hall, from the HALL_VIEW place */
    /* 20 ahead of the racer, looking back at it. it starts once the racer
     * has its Chao */
    RACE_CAMERA_AHEAD,
    /* the course's fixed view: one for races of kind 0, 1 and 4, one for
     * kind 2, and one for the rest */
    RACE_CAMERA_COURSE_FIXED,
    /* 20 behind the racer, but 135 degrees round it on racer_path_index 0 and
     * 225 on 1 */
    RACE_CAMERA_FOLLOW,
    RACE_CAMERA_FRONT, /* 20 in front of the racer */
    RACE_CAMERA_FRONT_2, /* as RACE_CAMERA_FRONT */
    RACE_CAMERA_BEHIND, /* 20 behind the racer */
    /* at a point of the course, looking at the racer. the point is
     * race_course's target 18 for a racer on racer_path_index 0 whose y is
     * below 20, and target 19 for one on 1. for any other racer, it is the
     * camera point of the racer's segment. with no Chao, the camera looks at
     * the racer's distance along the course's path */
    RACE_CAMERA_WATCH,
    /* from race_course's target 16, then along to 17 and round a circle
     * through it, looking at the winner, the racer in the standings' first
     * place. it starts once the winner has its Chao */
    RACE_CAMERA_FINISH,
    RACE_CAMERA_NONE_2, /* as RACE_CAMERA_NONE */
    RACE_CAMERA_FREE, /* where pad 0 flies it */
    RACE_CAMERA_EDITOR /* the set editor's, which places nothing */
};

/* RACE_CAMERA_FINISH's phases, in its state's finish.phase. the first two
 * hold at race_course's target 16, the third moves to 17, and the fourth
 * circles */
enum race_camera_finish_phase {
    FINISH_HOLD,
    FINISH_HOLD_2,
    FINISH_MOVE,
    FINISH_CIRCLE
};

/* what a race camera's mode keeps from one step to the next */
union race_camera_state {
    struct {
        /* 1 when the last step to test the hall's bounds moved the aim back
         * inside them, else 0. the aim is the point the camera eases toward.
         * while it is 1, the triggers don't turn the camera */
        Uint8 at_edge;
        Uint8 unk_01;
        Uint8 unk_02[10];
    } hall;
    struct {
        /* the camera's angle round the racer, which turns a degree a step
         * toward the one the mode asks for */
        Angle heading;
        Uint8 unk_04[4];
        Uint16 segment; /* the followed racer's on the last step */
        /* the lane followed on the last step. the image zero-extends it
         * before it compares the game's lane (0x0cb89182) */
        Uint16 lane;
    } follow;
    struct {
        /* the angle round the point race_camera_orbit circles, which turns
         * by the orbit's step each step */
        Angle heading;
        Sint32 segment; /* the followed racer's on the last step */
        Uint8 unk_08[4];
    } orbit;
    struct {
        /* steps in the two holds, and in FINISH_MOVE the share of the way
         * from race_course's target 16 to 17 */
        Float timer;
        Angle heading; /* the angle round FINISH_CIRCLE's circle */
        Sint32 phase; /* an enum race_camera_finish_phase */
    } finish;
};

/* the race's camera, which race_camera_step runs */
struct race_camera {
    Uint8 mode; /* an enum race_camera_mode */
    /* 0 until the mode's step calls camera_override, then 1. set it to 0
     * with each change of mode */
    Uint8 phase;
    Uint8 previous; /* the mode the last race_camera_step left */
    Uint8 unk_03;
    Uint16 flags;
    Uint8 unk_06[2];
    /* the follow modes' angle above the racer, or RACE_CAMERA_HALL's angle
     * round player 0 */
    Angle angle;
    Sint32 heading; /* the angle race_camera_set_heading asks for */
    NJS_VECTOR target_speed; /* how far camera_place.target eases a step */
    NJS_VECTOR speed; /* how far camera_place.pos eases a step */
    Uint8 unk_28[12];
    /* RACE_CAMERA_HALL's pull toward its aim, which each step adds to speed
     * and clears */
    NJS_VECTOR push;
    union race_camera_state state;
};

/* a step for camera_override that copies camera_place's target, angles and
 * distance, as CAMERA_FROM_TARGET reads them */
void fun_0cb889fa(struct camera_view *view);
/* set mode and previous to RACE_CAMERA_NONE, without releasing the camera */
void race_camera_reset(struct race_camera *camera);
/* run the camera's mode a step, then set previous to the mode the step leaves,
 * and clear RACE_CAMERA_HEADING. while the set editor is open, the step first
 * releases the camera and sets RACE_CAMERA_EDITOR at phase 0. the mode stays
 * RACE_CAMERA_EDITOR after the editor closes. the racer's lane, and for
 * RACE_CAMERA_FINISH the winner's, must be below RACE_LANES. the follow modes
 * and RACE_CAMERA_WATCH need that racer's work */
void race_camera_step(struct race_camera *camera, struct race_game *game);
/* the camera's mode, an enum race_camera_mode */
int race_camera_get_mode(struct race_camera *camera);
/* while RACE_CAMERA_OVERRIDE is set, call camera_restore and clear the
 * flag. the mode stays as it is */
void race_camera_release(struct race_camera *camera);
/* release the camera, then set RACE_CAMERA_WATCH at phase 0 */
void fun_0cb88d14(struct race_camera *camera);
/* the same as race_camera_release */
void fun_0cb88d2a(struct race_camera *camera);
/* copy pos to camera_place.target */
void fun_0cb89620(NJS_POINT3 *pos);
/* have the follow modes turn toward heading, an angle round the racer, on
 * the next race_camera_step. it sets RACE_CAMERA_HEADING, which that step
 * clears */
void race_camera_set_heading(struct race_camera *camera, int heading);
/* store in out where point, given in camera_work's frame, lies in the
 * world, and return 1. the frame turns by camera_work's x and y angles, not
 * by its z. with no camera_work, return 0 and leave out as it is */
int race_camera_to_world(NJS_POINT3 *out, NJS_POINT3 *point);

#endif
