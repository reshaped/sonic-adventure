/* the debug editor of an act's camera collisions */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdio.h>

#include "camera/camera.h"
#include "camera/editor.h"
#include "collision/collision.h"
#include "debug/debug.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* the editor task's work->mode. the task's next step opens the editor in
 * OPENING, and closes it in CLOSING */
enum camera_editor_mode {
    CAMERA_EDITOR_INIT,
    CAMERA_EDITOR_CLOSED,
    CAMERA_EDITOR_OPEN,
    CAMERA_EDITOR_WAIT, /* stepped as CLOSED. nothing sets it */
    CAMERA_EDITOR_OPENING, /* camera_editor_request_open asks for it */
    CAMERA_EDITOR_CLOSING /* the system mode asks for it */
};

/* SET MODE on the console, kept in the work's submode: what the pad does */
enum console_mode { CONSOLE_SET, CONSOLE_SYSTEM, CONSOLE_MODES };

/* EDIT ELEMENT on the console: the edited collision's member that the pad
 * edits, named as the member is. ELEMENT_COLANG and ELEMENT_CAMANG each edit
 * an x and a y angle */
enum element {
    ELEMENT_COLPOS,
    ELEMENT_COLSCL,
    ELEMENT_COLANG,
    ELEMENT_CAMPOS,
    ELEMENT_DIRPOS,
    ELEMENT_CAMANG,
    ELEMENT_DISTANCE,
    ELEMENTS
};

/* how much of the console shows: all of it, its first seven rows, or none */
enum page { PAGE_ALL, PAGE_SHORT, PAGE_NONE, PAGES };

/* how many priorities the editor steps a collision through, from 0 */
#define PRIORITIES 16

/* how near the cursor a collision must be for the pad to take it to edit */
#define TAKE_RANGE 200.0F
/* how near the camera a collision must be for its marker to show */
#define MARKER_RANGE 400.0F

static char camera_mode_names[CAMERA_MODES][32] = {
    "FOLLOW",       "A_FOLLOW",    "C_FOLLOW",    "COL_FOLLOW",
    "KNUCKLES",     "A_KNUCKLES",  "C_KNUCKLES",  "COL_KNUCKLES",
    "KNUCKLES2",    "A_KNUCKLES2", "C_KNUCKLES2", "COL_KNUCKLES2",
    "MAGONOTE",     "A_MAGONOTE",  "C_MAGONOTE",  "COL_MAGONOTE",
    "SONIC",        "A_SONIC",     "C_SONIC",     "COL_SONIC",
    "ASHLAND",      "A_ASHLAND",   "C_ASHLAND",   "ASHLAND_I",
    "A_ASHLAND_I",  "C_ASHLAND_I", "FISHING",     "A_FISHING",
    "FIXED",        "A_FIXED",     "C_FIXED",     "KLAMATH",
    "A_KLAMATH",    "C_KLAMATH",   "LINE",        "A_LINE",
    "NEWFOLLOW",    "A_NEWFOLLOW", "C_NEWFOLLOW", "POINT",
    "A_POINT",      "C_POINT",     "SONIC_P",     "A_SONIC_P",
    "C_SONIC_P",    "ADVERTISE",   "BACK",        "BACK2",
    "BUILDING",     "CART",        "CHAOS",       "CHAOS_P",
    "CHAOS_STINIT", "CHAOS_STD",   "CHAOS_W",     "E101R",
    "E103",         "EGM3",        "FOLLOW_G",    "A_FOLLOW_G",
    "LR",           "COLLI",       "RuinWaka1",   "SNOWBOARD",
    "SURVEY",       "TAIHO",       "TORNADE",     "TWO_HARES",
    "LEAVE",        "EDITOR",      "GURIGURI",    "PATHCAM",
};
static char adjust_mode_names[CAMERA_ADJUSTS][32] = {
    "NONE",       "NORMAL",     "NORMAL_S",   "SLOW",       "SLOW_S",
    "TIME",       "THREE1",     "THREE1C",    "THREE2",     "THREE2C",
    "THREE3",     "THREE3C",    "THREE4",     "THREE4C",    "THREE5",
    "THREE5C",    "RELATIVE1",  "RELATIVE1C", "RELATIVE2",  "RELATIVE2C",
    "RELATIVE3",  "RELATIVE3C", "RELATIVE4",  "RELATIVE4C", "RELATIVE5",
    "RELATIVE5C", "RELATIVE6C",
};
/* the shapes by their enum names. nothing reads them */
static char camcol_names[CAMCOL_TYPES][20] = {
    "CAMCOL_SPHERE",
    "CAMCOL_PLANE",
    "CAMCOL_BLOCK",
};
static char console_mode_names[CONSOLE_MODES][10] = {"SET", "SYSTEM"};
static char camera_level_names[CAMERA_LEVELS][16] = {
    "NORMAL  CAMERA", "AREA    CAMERA", "EVENT   CAMERA",
    "COMP    CAMERA", "COLI    CAMERA", "DEBUG   CAMERA",
};
static char element_names[ELEMENTS][20] = {
    "COLLISION POSITION", "COLLISION SCALE",  "COLLISION ANGLE",
    "CAMERA POSITION",    "CAMERA DIRECTION", "CAMERA ANGLE",
    "CAMERA DISTANCE",
};
/* the shapes as the console names them. SHPERE is the game's spelling */
static char camcol_type_names[CAMCOL_TYPES][20] = {
    "SHPERE",
    "PLANE",
    "BLOCK",
};

/* three white lines, 30 long each way along x, y and z, which
 * collisions_draw_axes draws at the act's collisions */
static NJS_POINT3 cam_axis_points[6] = {
#include "camera/editor/cam_axis_points.inc"
};
static NJS_COLOR cam_axis_colors[6] = {
#include "camera/editor/cam_axis_colors.inc"
};
static NJS_POINT3COL cam_axes = {
#include "camera/editor/cam_axes.inc"
};

/* the editor task's work, whose pos and ang are the cursor. it is null
 * until the task first steps, and the task's death leaves it pointing at
 * the freed work */
static struct task_work *cam_work;
/* how far the cursor moves in a step */
static Float cam_speed;
/* TRUE while the act's collisions show */
static Bool cam_shown;
static Sint16 cam_page; /* a page */
/* the edited collision's mode and adjust mode, which the pad picks here and
 * copies into cam_edited */
static Sint16 cam_mode;
static Uint8 cam_adjust;
static Sint16 cam_element; /* the element the pad edits */
/* the level whose modes the pad picks from */
static Sint16 cam_level;
/* what up and down add to a point or the distance in each step they're
 * held: 0.1, 1, 10 or 100 */
static Float cam_float_delta;
/* what up and down add to an angle in each step they're held: 1, 0x10,
 * 0x100 or 0x1000 */
static int cam_hex_delta;
/* the collision the pad edits, which is none of the act's collisions.
 * Start and B add a copy of it to them */
static struct camera_collision cam_edited;
/* blink counters: of the edited collision's shape marker, and of the digit
 * the delta changes in a float and in an angle */
static Sint16 cam_shape_blink;
static Sint16 cam_float_blink;
static Sint16 cam_hex_blink;

/* a blue square twenty across, the marker of a plane */
extern NJS_OBJECT marker_square_object;
/* yellow and red balls four across, which mark where a collision's camera
 * is and where it looks */
extern NJS_OBJECT marker_yellow_ball_object;
extern NJS_OBJECT marker_red_ball_object;

/* marker_object's radius, and the distance from the center of
 * marker_square_object and of marker_box_object to a side */
#define MARKER_SPHERE_RADIUS 10.0F
#define MARKER_SQUARE_HALF_SIZE 10.0F
#define MARKER_BOX_HALF_SIZE 5.0F

static void camera_editor_destroy(struct task *task);
static void collisions_draw_axes(void);
static void collisions_draw_markers(void);
static void camera_editor_open(struct task_work *work);
static void camera_editor_close(struct task_work *work);
static void camera_editor_run(struct task_work *work);
static void camera_editor_set(struct task_work *work);
static void camera_editor_system(struct task_work *work);
static void element_take_camera(struct task_work *work);
static void element_move(struct task_work *work);
static void element_edit(struct task_work *work);
static void edited_draw_markers(struct task_work *work);
static void marker_draw_sphere(struct camera_collision *collision);
static void marker_draw_plane(struct camera_collision *collision);
static void marker_draw_block(struct camera_collision *collision);
static void marker_draw_camera(struct camera_collision *collision);
static void marker_draw_target(struct camera_collision *collision);
static void camera_editor_print(struct task_work *work);
static void cam_format_float(char *buffer, Float value, Sint16 element,
                             Sint16 axis);
static void cam_format_hex(char *buffer, int value, Sint16 element,
                           Sint16 axis);
static void edited_place_at_cursor(struct task_work *work);

static void camera_editor_step(struct task *task)
{
    struct task_work *work = task->work;

    /* @hack spelling: every case but the closed ones returns, which gives the
     * default and CAMERA_EDITOR_INIT returns of their own (0x8c01ce62,
     * 0x8c01ce72); with breaks, they branch to the closed cases', the
     * function runs 6 bytes short and 41 of 54 words differ */
    switch (work->mode) {
    default:
        return;
    case CAMERA_EDITOR_INIT:
        cam_work = work;
        work->mode = CAMERA_EDITOR_CLOSED;
        task->destructor = camera_editor_destroy;
        return;
    case CAMERA_EDITOR_CLOSED:
    case CAMERA_EDITOR_WAIT:
        break;
    case CAMERA_EDITOR_OPENING:
        camera_editor_open(work);
        pad_disable();
        pause_disable();
        return;
    case CAMERA_EDITOR_OPEN:
        camera_editor_run(work);
        return;
    case CAMERA_EDITOR_CLOSING:
        work->mode = CAMERA_EDITOR_CLOSED;
        pad_enable();
        pause_enable();
        camera_editor_close(work);
        return;
    }
}

/* the task's destructor. unless the editor is closed, it calls
 * debug_set_flag(0), as closing does. it leaves the pads, pause and the
 * camera as the editor set them */
static void camera_editor_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (work->mode == CAMERA_EDITOR_OPENING ||
        work->mode == CAMERA_EDITOR_OPEN ||
        work->mode == CAMERA_EDITOR_CLOSING) {
        debug_set_flag(0);
    }
}

/* while the act's collisions show, draw cam_axes at each but the first,
 * turned as the collision is. it draws only in a frame's first step */
static void collisions_draw_axes(void)
{
    struct camera_collision *collision;

    if (frame_step) {
        return;
    }
    if (cam_shown == TRUE) {
        for (collision = &camera_collisions[*camera_collision_count - 1];
             collision > camera_collisions; collision--) {
            njPushMatrix(NULL);
            njTranslate(NULL, collision->col_pos.x, collision->col_pos.y,
                        collision->col_pos.z);
            /* @hack type: the angles cast to Angle, which loads each into R11
             * and extends it once for the test and once for the call
             * (0x8c01cf56, 0x8c01cf68); as plain Uint16s, 37 of 77 words
             * differ */
            TURN(njRotateX, (Angle)collision->col_ang_x);
            TURN(njRotateY, (Angle)collision->col_ang_y);
            njDrawLine3D(&cam_axes, 3, NJD_DRAW_NORMAL);
            njPopMatrix(1);
        }
    }
}

/* while the act's collisions show, draw the marker of each but the first
 * within MARKER_RANGE of the camera. it draws only in a frame's first step */
static void collisions_draw_markers(void)
{
    struct camera_collision *collision;
    NJS_VECTOR to_camera;

    if (frame_step) {
        return;
    }
    if (cam_shown == TRUE) {
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        for (collision = &camera_collisions[*camera_collision_count - 1];
             collision > camera_collisions; collision--) {
            to_camera.x = camera_work->pos.x - collision->col_pos.x;
            to_camera.y = camera_work->pos.y - collision->col_pos.y;
            to_camera.z = camera_work->pos.z - collision->col_pos.z;
            if (MARKER_RANGE > njScalor(&to_camera)) {
                switch (collision->type) {
                case CAMCOL_SPHERE:
                    marker_draw_sphere(collision);
                    break;
                case CAMCOL_PLANE:
                    marker_draw_plane(collision);
                    break;
                case CAMCOL_BLOCK:
                    marker_draw_block(collision);
                }
            }
        }
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
}

/* open the editor in the set mode. the cursor starts at player 0, with the
 * edited collision placed there, and camera_orbit turns the camera about the
 * cursor */
static void camera_editor_open(struct task_work *work)
{
    cam_float_delta = 1.0F;
    cam_hex_delta = 0x10;
    cam_element = ELEMENT_COLPOS;
    cam_speed = 1.0F;
    cam_level = CAMERA_LEVEL_NORMAL;
    cam_shown = TRUE;
    cam_page = PAGE_ALL;
    element_take_camera(work);
    cam_work = work;
    work->mode = CAMERA_EDITOR_OPEN;
    work->submode = CONSOLE_SET;
    work->pos = player_work[0]->pos;
    work->ang = player_work[0]->ang;
    edited_place_at_cursor(work);
    camera_take(work, camera_orbit);
}

static void camera_editor_close(struct task_work *work)
{
    debug_set_flag(0);
    camera_release();
    work->mode = CAMERA_EDITOR_CLOSED;
}

static void camera_editor_run(struct task_work *work)
{
    switch (work->submode) {
    case CONSOLE_SET:
        camera_editor_set(work);
        break;
    case CONSOLE_SYSTEM:
        camera_editor_system(work);
        break;
    }
    collisions_draw_axes();
    collisions_draw_markers();
    camera_editor_print(work);
}

/* the set mode, which edits cam_edited. with Start held, A takes a
 * collision out of the act's collisions to edit, and B adds a copy back. X
 * steps the adjust mode, and Y the shape. with A held, B turns to the system
 * mode, X steps the level, and Y the priority. up and down pick the element,
 * left and right the mode, and L and R slow or speed the cursor. with
 * neither held, element_move and element_edit edit the element */
static void camera_editor_set(struct task_work *work)
{
    struct camera_collision *collision;
    Sint16 i;
    Sint16 best;
    Float nearest;
    Float dx;
    Float dy;
    Float dz;
    Float distance;

    if (per_info[0]->on & PDD_DGT_ST) {
        if (per_info[0]->press & PDD_DGT_TA) {
            /* take the collision nearest the cursor, within TAKE_RANGE, out
             * of the act's collisions. it replaces the edited collision */
            best = *camera_collision_count;
            nearest = TAKE_RANGE;
            collision = &camera_collisions[*camera_collision_count - 1];
            for (i = *camera_collision_count - 1; i >= 0; i--) {
                dx = collision->col_pos.x - work->pos.x;
                dy = collision->col_pos.y - work->pos.y;
                dz = collision->col_pos.z - work->pos.z;
                distance = njSqrt((dx * dx) + (dy * dy) + (dz * dz));
                if (distance < nearest) {
                    best = i;
                    nearest = distance;
                }
                collision--;
            }
            /* @hack type: the count compared as an Sint16, which SHC
             * sign-extends with EXTS.W at 0x8c01d2a8. the two compares agree
             * while the count fits an Sint16, as it does up to
             * CAMERA_COLLISIONS_MAX, the most the editor lets a layout hold;
             * compared whole, 114 of 521 words differ */
            if (best < (Sint16)*camera_collision_count) {
                (*camera_collision_count)--;
                collision = &camera_collisions[best];
                /* NOLINTBEGIN(bugprone-signed-char-misuse) */
                cam_mode = collision->mode;
                cam_adjust = collision->adjust;
                cam_level = camera_modes[cam_mode].level;
                /* NOLINTEND(bugprone-signed-char-misuse) */
                cam_edited = *collision;
                for (i = best; i < (Sint16)*camera_collision_count; i++) {
                    camera_collisions[i] = camera_collisions[i + 1];
                }
            }
            cam_element = ELEMENT_COLPOS;
            element_take_camera(work);
        } else if (per_info[0]->press & PDD_DGT_TB) {
            /* add a copy of the edited collision to the act's collisions,
             * if they're fewer than CAMERA_COLLISIONS_MAX, and place the
             * edited one afresh at the cursor */
            /* @hack type: room compared as a Uint32, which SHC tests with
             * CMP/HS at 0x8c01d3ec. the two compares agree while the count
             * isn't negative, and the editor never takes it below 0; compared
             * signed, CMP/GE takes its place and 1 of 521 words differ */
            /* @hack spelling: the copy through collision, which keeps
             * cam_level's address in R12 and 0 in R11 (0x8c01d204,
             * 0x8c01d212); stored straight, the two trade and 103 of 521
             * words differ */
            if ((Uint32)*camera_collision_count < CAMERA_COLLISIONS_MAX) {
                collision = &camera_collisions[(*camera_collision_count)++];
                *collision = cam_edited;
            }
            edited_place_at_cursor(work);
        } else if (per_info[0]->press & PDD_DGT_TX) {
            if (++cam_adjust >= camera_adjust_count) {
                cam_adjust = 0;
            }
            cam_edited.adjust = cam_adjust;
        } else if (per_info[0]->press & PDD_DGT_TY) {
            cam_edited.type++;
            if (cam_edited.type >= CAMCOL_TYPES) {
                cam_edited.type = CAMCOL_SPHERE;
            }
        }
    } else if (per_info[0]->on & PDD_DGT_TA) {
        if (per_info[0]->press & PDD_DGT_TB) {
            work->submode++;
            if (work->submode >= CONSOLE_MODES) {
                work->submode = CONSOLE_SET;
            }
        } else if (per_info[0]->press & PDD_DGT_TX) {
            /* step the level round NORMAL, AREA, COMP and COLI, and keep any
             * other. then move cam_mode on to a mode at the level */
            switch (cam_level) {
            case CAMERA_LEVEL_NORMAL:
                cam_level = CAMERA_LEVEL_AREA;
                break;
            case CAMERA_LEVEL_AREA:
                cam_level = CAMERA_LEVEL_COMP;
                break;
            case CAMERA_LEVEL_COMP:
                cam_level = CAMERA_LEVEL_COLI;
                break;
            case CAMERA_LEVEL_COLI:
                cam_level = CAMERA_LEVEL_NORMAL;
                break;
            }
            for (;;) {
                if (cam_level == camera_modes[cam_mode].level) {
                    cam_edited.mode = cam_mode;
                    break;
                }
                if (++cam_mode >= camera_mode_count) {
                    cam_mode = 0;
                }
            }
        } else if (per_info[0]->press & PDD_DGT_TY) {
            cam_edited.priority++;
            if (cam_edited.priority >= PRIORITIES) {
                cam_edited.priority = 0;
            }
        } else if (per_info[0]->press & PDD_DGT_KU) {
            cam_element--;
            if (cam_element < ELEMENT_COLPOS) {
                cam_element = ELEMENTS - 1;
            }
            element_take_camera(work);
        } else if (per_info[0]->press & PDD_DGT_KD) {
            cam_element++;
            if (cam_element >= ELEMENTS) {
                cam_element = ELEMENT_COLPOS;
            }
            element_take_camera(work);
        } else if (per_info[0]->press & PDD_DGT_KL) {
            for (;;) {
                if (--cam_mode < 0) {
                    cam_mode = camera_mode_count - 1;
                }
                if (cam_level == camera_modes[cam_mode].level) {
                    break;
                }
            }
            cam_edited.mode = cam_mode;
            element_take_camera(work);
        } else if (per_info[0]->press & PDD_DGT_KR) {
            for (;;) {
                if (++cam_mode >= camera_mode_count) {
                    cam_mode = 0;
                }
                if (cam_level == camera_modes[cam_mode].level) {
                    break;
                }
            }
            cam_edited.mode = cam_mode;
            element_take_camera(work);
        } else if (per_info[0]->on & PDD_DGT_TL) {
            cam_speed -= 0.1F;
            if (cam_speed < 0.0F) {
                cam_speed = 0.0F;
            }
        } else if (per_info[0]->on & PDD_DGT_TR) {
            cam_speed += 0.1F;
        }
    } else {
        element_move(work);
        element_edit(work);
    }
    edited_draw_markers(work);
}

/* the system mode. with A held, B turns back to the set mode, and X closes
 * the editor. otherwise Y turns the console's page, and B shows or hides
 * the act's collisions. a press of X without A does nothing, and Y and B
 * pressed with it are ignored */
static void camera_editor_system(struct task_work *work)
{
    if (per_info[0]->on & PDD_DGT_TA) {
        if (per_info[0]->press & PDD_DGT_TB) {
            work->submode++;
            if (work->submode >= CONSOLE_MODES) {
                work->submode = CONSOLE_SET;
                /* @hack redundant: this return and the close's, which give each
                 * an RTS of its own with the store in its slot (0x8c01d704,
                 * 0x8c01d712); without them, 2 of 61 words differ */
                return;
            }
        } else if (per_info[0]->press & PDD_DGT_TX) {
            work->mode = CAMERA_EDITOR_CLOSING;
            return;
        }
    } else if (per_info[0]->press & PDD_DGT_TX) {
    } else if (per_info[0]->press & PDD_DGT_TY) {
        cam_page++;
        if (cam_page >= PAGES) {
            cam_page = PAGE_ALL;
            /* @hack redundant: this return, where control leaves anyway,
             * which gives the page's wrap an RTS of its own with the store in
             * its slot (0x8c01d73a), as the mode's wrap has; without it, 1 of
             * 61 words differ */
            return;
        }
    } else if (per_info[0]->press & PDD_DGT_TB) {
        cam_shown = cam_shown == TRUE ? FALSE : TRUE;
    }
}

/* take the camera for the element. the collision's elements take
 * camera_orbit, which turns the camera about the cursor. the camera's points
 * and distance take the edited mode's editor step, or camera_orbit where it
 * has none. ELEMENT_CAMANG keeps the camera already taken */
static void element_take_camera(struct task_work *work)
{
    struct camera_mode *mode;

    switch (cam_element) {
    case ELEMENT_COLPOS:
    case ELEMENT_COLSCL:
    case ELEMENT_COLANG:
        camera_take(work, camera_orbit);
        break;
    case ELEMENT_CAMPOS:
    case ELEMENT_DIRPOS:
    case ELEMENT_DISTANCE:
        mode = &camera_modes[cam_mode];
        if (mode->editor == NULL) {
            camera_take(work, camera_orbit);
        } else {
            camera_take(work, mode->editor);
        }
        break;
    }
}

/* move the cursor by cam_speed in each step, and the element's point with
 * it: col_pos for the collision's elements, or cam_pos or dir_pos. the
 * cursor first goes to the point. up and down move it away from the camera
 * and back, left and right across, and L and R up and down. it moves
 * nothing for ELEMENT_CAMANG or ELEMENT_DISTANCE, or with A, B, X or Y held */
static void element_move(struct task_work *work)
{
    NJS_POINT3 *point;
    Float *y;
    Float *z;
    Angle ang;

    ang = camera_work->ang.y;
    if (cam_element == ELEMENT_CAMANG || cam_element == ELEMENT_DISTANCE) {
        return;
    }
    if (per_info[0]->on & (PDD_DGT_TB | PDD_DGT_TA | PDD_DGT_TY | PDD_DGT_TX)) {
        return;
    }
    switch (cam_element) {
    case ELEMENT_COLPOS:
    case ELEMENT_COLSCL:
    case ELEMENT_COLANG:
        point = &cam_edited.col_pos;
        break;
    case ELEMENT_CAMPOS:
        point = &cam_edited.cam_pos;
        break;
    case ELEMENT_DIRPOS:
        point = &cam_edited.dir_pos;
        break;
    default:
        return;
    }
    work->pos.x = point->x;
    /* @hack spelling: y and z taken in the reads, which leaves them in the
     * 8-byte frame (ADD #-8,R15 at 0x8c01d80c); taken apart, the prologue
     * saves R8 and R9 for them instead and 116 of 199 words differ */
    work->pos.y = *(y = &point->y);
    work->pos.z = *(z = &point->z);
    if (per_info[0]->on & (PDD_DGT_KU | PDD_DGT_KD | PDD_DGT_KL | PDD_DGT_KR)) {
        if (per_info[0]->on & PDD_DGT_KU) {
            if (per_info[0]->on & PDD_DGT_KL) {
                ang += 0xa000;
            } else if (per_info[0]->on & PDD_DGT_KR) {
                ang += 0x6000;
            } else {
                ang += 0x8000;
            }
        } else if (per_info[0]->on & PDD_DGT_KD) {
            if (per_info[0]->on & PDD_DGT_KL) {
                ang += 0xe000;
            } else if (per_info[0]->on & PDD_DGT_KR) {
                ang += 0x2000;
            } else {
                /* @hack redundant: down alone adds 0, an arm SHC keeps as a
                 * BT straight to the move (0x8c01d946); without it, 3 of 199
                 * words differ */
                ang += 0x0000;
            }
        } else if (per_info[0]->on & PDD_DGT_KL) {
            ang += 0xc000;
        } else if (per_info[0]->on & PDD_DGT_KR) {
            ang += 0x4000;
        }
        work->pos.z += njCos(ang) * cam_speed;
        work->pos.x += njSin(ang) * cam_speed;
    }
    if (per_info[0]->on & (PDD_DGT_TR | PDD_DGT_TL)) {
        if (per_info[0]->on & PDD_DGT_TL) {
            work->pos.y += cam_speed;
        } else if (per_info[0]->on & PDD_DGT_TR) {
            work->pos.y -= cam_speed;
        }
    }
    point->x = work->pos.x;
    *y = work->pos.y;
    *z = work->pos.z;
}

/* edit the element's value. a press of left makes its delta 10 times
 * bigger, or 16 for an angle, and right as many times smaller. up adds the
 * delta and down takes it away, in each step they're held. they edit x with
 * X held, y with Y and z with B, which for an angle lies past its y angle.
 * the distance takes X alone */
static void element_edit(struct task_work *work)
{
    Uint16 *angle;
    NJS_POINT3 *point;
    Float *y;
    Float *z;

    if (cam_element == ELEMENT_DISTANCE) {
        if (per_info[0]->press & PDD_DGT_KL) {
            cam_float_delta *= 10.0F;
            if (cam_float_delta > 100.0F) {
                cam_float_delta = 100.0F;
            }
        } else if (per_info[0]->press & PDD_DGT_KR) {
            cam_float_delta *= 0.1F;
            if (cam_float_delta < 0.1F) {
                cam_float_delta = 0.1F;
            }
        }
        if (per_info[0]->on & PDD_DGT_TX) {
            if (per_info[0]->on & PDD_DGT_KU) {
                cam_edited.distance += cam_float_delta;
            }
            if (per_info[0]->on & PDD_DGT_KD) {
                cam_edited.distance -= cam_float_delta;
            }
        }
    } else if (cam_element == ELEMENT_COLANG || cam_element == ELEMENT_CAMANG) {
        if (per_info[0]->press & PDD_DGT_KL) {
            cam_hex_delta *= 16;
            if (cam_hex_delta > 0x1000) {
                cam_hex_delta = 0x1000;
            }
        } else if (per_info[0]->press & PDD_DGT_KR) {
            cam_hex_delta /= 16;
            if (cam_hex_delta < 1) {
                cam_hex_delta = 1;
            }
        }
        /* @hack spelling: angle, a Uint16 cursor from col_ang_x or cam_ang_x,
         * reaches the y angle beside it as angle[1]; without it, through a
         * byte cursor from the record's base, 148 of 363 words differ */
        if (cam_element == ELEMENT_COLANG) {
            angle = &cam_edited.col_ang_x;
        } else {
            angle = &cam_edited.cam_ang_x;
        }
        if (per_info[0]->on & PDD_DGT_TX) {
            if (per_info[0]->on & PDD_DGT_KU) {
                /* @hack type: the delta cast to Uint16, here and below,
                 * which loads cam_hex_delta again for each add and subtract,
                 * as MOV.L @R4,R0 at 0x8c01dba0 does; uncast, 122 of 363
                 * words differ */
                angle[0] += (Uint16)cam_hex_delta;
            }
            if (per_info[0]->on & PDD_DGT_KD) {
                angle[0] -= (Uint16)cam_hex_delta;
            }
        } else if (per_info[0]->on & PDD_DGT_TY) {
            if (per_info[0]->on & PDD_DGT_KU) {
                angle[1] += (Uint16)cam_hex_delta;
            }
            if (per_info[0]->on & PDD_DGT_KD) {
                angle[1] -= (Uint16)cam_hex_delta;
            }
        } else if (per_info[0]->on & PDD_DGT_TB) {
            /* @bug memory: angle[2] lies past the two angles, in the
             * low half of col_pos.x at ELEMENT_COLANG and of dir_pos.x at
             * ELEMENT_CAMANG, which B with up or down edits in place of an
             * angle (ADD #4,R6 at 0x8c01dc36, stores at 0x8c01dc3e and
             * 0x8c01dc54) */
            if (per_info[0]->on & PDD_DGT_KU) {
                angle[2] += (Uint16)cam_hex_delta;
            }
            if (per_info[0]->on & PDD_DGT_KD) {
                angle[2] -= (Uint16)cam_hex_delta;
            }
        }
    } else {
        point = NULL;
        if (per_info[0]->press & PDD_DGT_KL) {
            cam_float_delta *= 10.0F;
            if (cam_float_delta > 100.0F) {
                cam_float_delta = 100.0F;
            }
        } else if (per_info[0]->press & PDD_DGT_KR) {
            cam_float_delta *= 0.1F;
            if (cam_float_delta < 0.1F) {
                cam_float_delta = 0.1F;
            }
        }
        switch (cam_element) {
        case ELEMENT_COLPOS:
            point = &cam_edited.col_pos;
            break;
        case ELEMENT_COLSCL:
            point = &cam_edited.col_scl;
            /* a sphere's radius follows its square, which the pad edits as
             * z */
            if (cam_edited.type == CAMCOL_SPHERE) {
                cam_edited.col_scl.x = njSqrt(cam_edited.col_scl.z);
            }
            break;
        case ELEMENT_CAMPOS:
            point = &cam_edited.cam_pos;
            break;
        case ELEMENT_DIRPOS:
            point = &cam_edited.dir_pos;
            break;
        }
        y = &point->y;
        /* @hack spelling: z set inside the test, which loads the delta ahead of
         * it and puts z's ADD #8,R6 in the BT/S's slot (0x8c01dcf0,
         * 0x8c01dcf8); set ahead of the test, 5 of 363 words differ */
        if (z = &point->z, per_info[0]->on & PDD_DGT_TX) {
            if (per_info[0]->on & PDD_DGT_KU) {
                point->x += cam_float_delta;
            }
            if (per_info[0]->on & PDD_DGT_KD) {
                point->x -= cam_float_delta;
            }
        } else if (per_info[0]->on & PDD_DGT_TY) {
            if (per_info[0]->on & PDD_DGT_KU) {
                *y += cam_float_delta;
            }
            if (per_info[0]->on & PDD_DGT_KD) {
                *y -= cam_float_delta;
            }
        } else if (per_info[0]->on & PDD_DGT_TB) {
            if (per_info[0]->on & PDD_DGT_KU) {
                *z += cam_float_delta;
            }
            if (per_info[0]->on & PDD_DGT_KD) {
                *z -= cam_float_delta;
            }
        }
        /* the cursor follows an edited point. a scale isn't a place: the
         * cursor stays, and no axis of the scale goes below 0 */
        if (cam_element != ELEMENT_COLSCL) {
            work->pos.x = point->x;
            work->pos.y = *y;
            work->pos.z = *z;
        } else {
            if (point->x < 0.0F) {
                point->x = 0.0F;
            }
            if (*y < 0.0F) {
                *y = 0.0F;
            }
            if (*z < 0.0F) {
                *z = 0.0F;
            }
        }
    }
}

/* draw the cursor's axes and the edited collision's markers. while the pad
 * edits one of the camera's elements, the shape's marker blinks */
static void edited_draw_markers(struct task_work *work)
{
    debug_draw_axes(work);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    if (cam_element == ELEMENT_COLPOS || cam_element == ELEMENT_COLSCL ||
        cam_element == ELEMENT_COLANG || (cam_shape_blink++ & 8)) {
        switch (cam_edited.type) {
        case CAMCOL_SPHERE:
            marker_draw_sphere(&cam_edited);
            break;
        case CAMCOL_PLANE:
            marker_draw_plane(&cam_edited);
            break;
        case CAMCOL_BLOCK:
            marker_draw_block(&cam_edited);
            break;
        }
    }
    marker_draw_camera(&cam_edited);
    marker_draw_target(&cam_edited);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* draw the marker of a sphere, a plane or a block at the collision's
 * col_pos. a sphere's takes its radius. a plane's or block's takes its half
 * sizes, and turns by its angles */
static void marker_draw_sphere(struct camera_collision *collision)
{
    Float scale = njSqrt(collision->col_scl.z) * (1.0F / MARKER_SPHERE_RADIUS);

    njPushMatrix(NULL);
    njTranslate(NULL, collision->col_pos.x, collision->col_pos.y,
                collision->col_pos.z);
    njScale(NULL, scale, scale, scale);
    draw_model(marker_object.model);
    njPopMatrix(1);
}

static void marker_draw_plane(struct camera_collision *collision)
{
    Float x = collision->col_scl.x * (1.0F / MARKER_SQUARE_HALF_SIZE);
    Float y = collision->col_scl.y * (1.0F / MARKER_SQUARE_HALF_SIZE);

    njPushMatrix(NULL);
    njTranslate(NULL, collision->col_pos.x, collision->col_pos.y,
                collision->col_pos.z);
    /* @hack type: the angles cast to Angle, which extends each once and
     * keeps it in the frame across its test (0x8c01df2a, 0x8c01df3c); as
     * plain Uint16s, 35 of 64 words differ */
    TURN(njRotateX, (Angle)collision->col_ang_x);
    TURN(njRotateY, (Angle)collision->col_ang_y);
    njScale(NULL, x, y, 1.0F);
    draw_model(marker_square_object.model);
    njPopMatrix(1);
}

static void marker_draw_block(struct camera_collision *collision)
{
    Float x = collision->col_scl.x * (1.0F / MARKER_BOX_HALF_SIZE);
    Float y = collision->col_scl.y * (1.0F / MARKER_BOX_HALF_SIZE);
    Float z = collision->col_scl.z * (1.0F / MARKER_BOX_HALF_SIZE);

    njPushMatrix(NULL);
    njTranslate(NULL, collision->col_pos.x, collision->col_pos.y,
                collision->col_pos.z);
    /* @hack type: the angles cast to Angle, as in marker_draw_plane
     * (0x8c01dfb4, 0x8c01dfc6); as plain Uint16s, 36 of 70 words differ */
    TURN(njRotateX, (Angle)collision->col_ang_x);
    TURN(njRotateY, (Angle)collision->col_ang_y);
    njScale(NULL, x, y, z);
    draw_model(marker_box_object.model);
    njPopMatrix(1);
}

/* draw the yellow ball at the collision's cam_pos, and the red one at its
 * dir_pos, where its camera looks */
static void marker_draw_camera(struct camera_collision *collision)
{
    njPushMatrix(NULL);
    njTranslate(NULL, collision->cam_pos.x, collision->cam_pos.y,
                collision->cam_pos.z);
    draw_model(marker_yellow_ball_object.model);
    njPopMatrix(1);
}

static void marker_draw_target(struct camera_collision *collision)
{
    njPushMatrix(NULL);
    njTranslate(NULL, collision->dir_pos.x, collision->dir_pos.y,
                collision->dir_pos.z);
    draw_model(marker_red_ball_object.model);
    njPopMatrix(1);
}

/* print the console's rows that cam_page shows, each a label and a value.
 * the first seven show the editor's own settings and cam_edited's shape. at
 * PAGE_ALL, the rest show cam_edited's fields. it prints only in a frame's
 * first step */
static void camera_editor_print(struct task_work *work)
{
    char buf[32];
    Sint16 row;

    row = 3;
    if (frame_step) {
        return;
    }
    if (cam_page <= PAGE_SHORT) {
        njPrintC(NJM_LOCATION(5, row++), "SET    MODE:");
        njPrintC(NJM_LOCATION(5, row++), "CAMERA MODE:");
        njPrintC(NJM_LOCATION(5, row++), "CAMERA LEVEL:");
        njPrintC(NJM_LOCATION(5, row++), "EDIT ELEMENT:");
        njPrintC(NJM_LOCATION(5, row++), "COLLISION TYPE:");
        njPrintC(NJM_LOCATION(5, row++), "MOVING SPEED:");
        njPrintC(NJM_LOCATION(5, row++), "ADJUST MODE:");
        if (cam_page == PAGE_ALL) {
            njPrintC(NJM_LOCATION(5, row++), "PRIORITY:");
            njPrintC(NJM_LOCATION(5, row++), "X COLPOS:");
            njPrintC(NJM_LOCATION(5, row++), "Y COLPOS:");
            njPrintC(NJM_LOCATION(5, row++), "Z COLPOS:");
            njPrintC(NJM_LOCATION(5, row++), "X COLSCL:");
            njPrintC(NJM_LOCATION(5, row++), "Y COLSCL:");
            njPrintC(NJM_LOCATION(5, row++), "Z COLSCL:");
            njPrintC(NJM_LOCATION(5, row++), "X COLANG:");
            njPrintC(NJM_LOCATION(5, row++), "Y COLANG:");
            njPrintC(NJM_LOCATION(5, row++), "X CAMPOS:");
            njPrintC(NJM_LOCATION(5, row++), "Y CAMPOS:");
            njPrintC(NJM_LOCATION(5, row++), "Z CAMPOS:");
            njPrintC(NJM_LOCATION(5, row++), "X DIRPOS:");
            njPrintC(NJM_LOCATION(5, row++), "Y DIRPOS:");
            njPrintC(NJM_LOCATION(5, row++), "Z DIRPOS:");
            njPrintC(NJM_LOCATION(5, row++), "X CAMANG:");
            njPrintC(NJM_LOCATION(5, row++), "Y CAMANG:");
            njPrintC(NJM_LOCATION(5, row++), "DISTANCE:");
        }
    }
    row = 3;
    if (cam_page > PAGE_SHORT) {
        return;
    }
    njPrintC(NJM_LOCATION(18, row++), console_mode_names[work->submode]);
    njPrintC(NJM_LOCATION(18, row++), camera_mode_names[cam_mode]);
    njPrintC(NJM_LOCATION(18, row++), camera_level_names[cam_level]);
    njPrintC(NJM_LOCATION(18, row++), element_names[cam_element]);
    njPrintC(NJM_LOCATION(22, row++), camcol_type_names[cam_edited.type]);
    sprintf(buf, "%8.2f", cam_speed);
    njPrintC(NJM_LOCATION(20, row++), buf);
    njPrintC(NJM_LOCATION(18, row++), adjust_mode_names[cam_adjust]);
    if (cam_page != PAGE_ALL) {
        return;
    }
    njPrintD(NJM_LOCATION(18, row++), cam_edited.priority, 10);
    cam_format_float(buf, cam_edited.col_pos.x, ELEMENT_COLPOS, 0);
    njPrintC(NJM_LOCATION(18, row++), buf);
    cam_format_float(buf, cam_edited.col_pos.y, ELEMENT_COLPOS, 1);
    njPrintC(NJM_LOCATION(18, row++), buf);
    cam_format_float(buf, cam_edited.col_pos.z, ELEMENT_COLPOS, 2);
    njPrintC(NJM_LOCATION(18, row++), buf);
    cam_format_float(buf, cam_edited.col_scl.x, ELEMENT_COLSCL, 0);
    njPrintC(NJM_LOCATION(18, row++), buf);
    cam_format_float(buf, cam_edited.col_scl.y, ELEMENT_COLSCL, 1);
    njPrintC(NJM_LOCATION(18, row++), buf);
    cam_format_float(buf, cam_edited.col_scl.z, ELEMENT_COLSCL, 2);
    njPrintC(NJM_LOCATION(18, row++), buf);
    cam_format_hex(buf, cam_edited.col_ang_x, ELEMENT_COLANG, 0);
    njPrintC(NJM_LOCATION(20, row++), buf);
    cam_format_hex(buf, cam_edited.col_ang_y, ELEMENT_COLANG, 1);
    njPrintC(NJM_LOCATION(20, row++), buf);
    cam_format_float(buf, cam_edited.cam_pos.x, ELEMENT_CAMPOS, 0);
    njPrintC(NJM_LOCATION(18, row++), buf);
    cam_format_float(buf, cam_edited.cam_pos.y, ELEMENT_CAMPOS, 1);
    njPrintC(NJM_LOCATION(18, row++), buf);
    cam_format_float(buf, cam_edited.cam_pos.z, ELEMENT_CAMPOS, 2);
    njPrintC(NJM_LOCATION(18, row++), buf);
    cam_format_float(buf, cam_edited.dir_pos.x, ELEMENT_DIRPOS, 0);
    njPrintC(NJM_LOCATION(18, row++), buf);
    cam_format_float(buf, cam_edited.dir_pos.y, ELEMENT_DIRPOS, 1);
    njPrintC(NJM_LOCATION(18, row++), buf);
    cam_format_float(buf, cam_edited.dir_pos.z, ELEMENT_DIRPOS, 2);
    njPrintC(NJM_LOCATION(18, row++), buf);
    cam_format_hex(buf, cam_edited.cam_ang_x, ELEMENT_CAMANG, 0);
    njPrintC(NJM_LOCATION(20, row++), buf);
    cam_format_hex(buf, cam_edited.cam_ang_y, ELEMENT_CAMANG, 1);
    njPrintC(NJM_LOCATION(20, row++), buf);
    cam_format_float(buf, cam_edited.distance, ELEMENT_DISTANCE, 0);
    njPrintC(NJM_LOCATION(18, row++), buf);
}

/* write value into buffer. while the pad edits it, the digit that
 * cam_float_delta changes blinks */
static void cam_format_float(char *buffer, Float value, Sint16 element,
                             Sint16 axis)
{
    sprintf(buffer, "%10.4f", value);
    if (((axis == 0 && (per_info[0]->on & PDD_DGT_TX)) ||
         (axis == 1 && (per_info[0]->on & PDD_DGT_TY)) ||
         (axis == 2 && (per_info[0]->on & PDD_DGT_TB))) &&
        element == cam_element && (cam_float_blink++ & 8)) {
        /* @hack spelling: each constant on the left, which puts the delta first
         * in each FCMP/EQ (FCMP/EQ FR4,FR3 at 0x8c01e578); on the right, 6 of
         * 117 words differ */
        /* @hack redundant: a return in each arm but the last, which gives each
         * arm an epilogue and RTS of its own (0x8c01e586, 0x8c01e59a);
         * without them, 82 of 117 words differ and the code runs 32 bytes
         * short */
        if (0.01F == cam_float_delta) {
            buffer[7] = ' ';
            return;
        } else if (0.1F == cam_float_delta) {
            buffer[6] = ' ';
            return;
        } else if (1.0F == cam_float_delta) {
            buffer[4] = ' ';
            return;
        } else if (10.0F == cam_float_delta) {
            buffer[3] = ' ';
            return;
        } else if (100.0F == cam_float_delta) {
            buffer[2] = ' ';
            return;
        } else if (1000.0F == cam_float_delta) {
            buffer[1] = ' ';
        }
    }
}

/* as cam_format_float, for an angle, with the digit cam_hex_delta changes
 * blinking */
static void cam_format_hex(char *buffer, int value, Sint16 element, Sint16 axis)
{
    sprintf(buffer, "%08x", value);
    if (((axis == 0 && (per_info[0]->on & PDD_DGT_TX)) ||
         (axis == 1 && (per_info[0]->on & PDD_DGT_TY))) &&
        element == cam_element && (cam_hex_blink++ & 8)) {
        /* @hack redundant: a return in each arm but the last, which puts each
         * arm's store in the slot of its own BRA to the epilogue (0x8c01e6c8
         * to 0x8c01e728); without them, 44 of 93 words differ and the code
         * runs 32 bytes short */
        if (cam_hex_delta == 1) {
            buffer[7] = ' ';
            return;
        } else if (cam_hex_delta == 0x10) {
            buffer[6] = ' ';
            return;
        } else if (cam_hex_delta == 0x100) {
            buffer[5] = ' ';
            return;
        } else if (cam_hex_delta == 0x1000) {
            buffer[4] = ' ';
            return;
        } else if (cam_hex_delta == 0x10000) {
            buffer[3] = ' ';
        }
    }
}

void camera_step_editor(struct camera_view *view)
{
    view->pos.x = cam_edited.cam_pos.x;
    view->pos.y = cam_edited.cam_pos.y;
    view->pos.z = cam_edited.cam_pos.z;
    view->target.x = cam_edited.dir_pos.x;
    view->target.y = cam_edited.dir_pos.y;
    view->target.z = cam_edited.dir_pos.z;
    view->ang_x = cam_edited.cam_ang_x;
    view->ang_y = cam_edited.cam_ang_y;
}

/* place the edited collision at the cursor, with a scale of 10 each way and
 * every angle 0. its camera looks at the cursor from 15 above it and 50
 * out, along z turned by the cursor's y angle. the collision keeps its
 * mode, adjust mode, priority, shape and distance */
static void edited_place_at_cursor(struct task_work *work)
{
    cam_edited.col_pos.x = work->pos.x;
    cam_edited.col_pos.y = work->pos.y;
    cam_edited.col_pos.z = work->pos.z;
    cam_edited.col_scl.x = 10.0F;
    cam_edited.col_scl.y = 10.0F;
    cam_edited.col_scl.z = 10.0F;
    cam_edited.col_ang_x = 0;
    cam_edited.col_ang_y = 0;
    cam_edited.cam_ang_x = 0;
    cam_edited.cam_ang_y = 0;
    cam_edited.dir_pos.x = cam_edited.col_pos.x = work->pos.x;
    cam_edited.dir_pos.y = cam_edited.col_pos.y = work->pos.y;
    cam_edited.dir_pos.z = cam_edited.col_pos.z = work->pos.z;
    cam_edited.cam_pos.y = cam_edited.dir_pos.y + 15.0F;
    cam_edited.cam_pos.x = cam_edited.dir_pos.x + (njSin(work->ang.y) * 50.0F);
    cam_edited.cam_pos.z = cam_edited.dir_pos.z + (njCos(work->ang.y) * 50.0F);
}

void camera_editor_create(void)
{
    task_create(TASK_WORK, 1, camera_editor_step);
}

int camera_editor_request_open(void)
{
    struct task_work *work = cam_work;

    if (work == NULL) {
        return -1;
    }
    switch (work->mode) {
    case CAMERA_EDITOR_CLOSED:
        work->mode = CAMERA_EDITOR_OPENING;
        break;
    case CAMERA_EDITOR_CLOSING:
        return -1;
    }
    return 0;
}

/* the message, by its words, for saving the act's collisions. no code
 * calls it. SHC leaves the function out, but keeps its string at
 * 0x8c1292f8 */
/* NOLINTNEXTLINE(clang-diagnostic-unused-function) */
static const char *camera_editor_saving(void)
{
    return "SAVING.........";
}
/* a folder for each stage from STAGE_PRACTICE to STAGE_CHAOS0. nothing reads
 * them */
static char *camera_folders[STAGE_CHAOS0 + 1] = {
    "Stg00_Practice\\", "Stg01_Emerald\\", "Stg02_Windy\\",
    "Stg03_Twinkle\\",  "Stg04_Highway\\", "Stg05_Mountain\\",
    "Stg06_SkyDeck\\",  "Stg07_Ruin\\",    "Stg08_Icecap\\",
    "Stg09_Casino\\",   "camera\\",        "camera\\",
    "camera\\",         "camera\\",        "camera\\",
    "Boss00_Chaos0\\",
};
