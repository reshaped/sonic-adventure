/* an event path editor. a key is a point of the path and its angles. the
 * pad moves a cursor through the world, and the camera orbits it. ADD POINT
 * puts a key at the cursor. a selected run of keys can be moved, deleted,
 * subdivided or respaced, and ROT EDIT edits keys' angles. a preview task
 * runs keys as an event path.
 *
 * the editor saves keys as .ept text. the text holds a table of EPATHDAT
 * entries, each an EPATH_ANG in degrees and an EPATH_POS. a tag after the
 * table gives the entries' count and names the table */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdio.h>
#include <string.h>

#include "debug/host.h"
#include "event/event.h"
#include "event/play.h"
#include "hud/hud.h"
#include "player/player.h"
#include "shared/path_editor.h"
#include "system/loader.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/thread.h"
#include "system/util.h"

/* the keys the editor has room for */
#define KEYS 512

/* a 256th of a turn, as an Angle. it is an event path's unit of angle, ROT
 * EDIT's step and the camera's turn in a pass. ANGLE_STEP_SHIFT is its
 * shift, and WHOLE_STEPS masks an angle to its whole steps within a turn */
#define ANGLE_STEP 0x100
#define ANGLE_STEP_SHIFT 8
#define WHOLE_STEPS 0xff00

/* how far the cursor's upright line runs each way, and each key's line down
 * from it */
#define GUIDE_LENGTH 1000.0F

/* the ROT ORDER menu's rotation orders. each names the axes in the order
 * their turns apply */
enum rotation_order {
    ORDER_XYZ,
    ORDER_XZY,
    ORDER_YXZ,
    ORDER_YZX,
    ORDER_ZXY,
    ORDER_ZYX
};

typedef Angle angle_row[3];

/* a key's angles, by name or by axis, 0 to 2 for x to z */
union key_angles {
    struct angles ang;
    angle_row axis;
};

/* a polygon the preview draws at its task, filled in one color */
struct marker {
    NJS_POINT3 *points;
    int count;
    Uint32 color;
};

/* a preview task's vars. its destructor calls release with path */
struct preview_vars {
    Uint8 unk_00[4];
    void (*release)(struct event_path *path);
    struct event_path *path;
};

#define PREVIEW(work) ((struct preview_vars *)&(work)->vars)

struct editor_menu;

/* the choices a command holds: a menu's item, then its submenus' in turn */
#define CHOICES 8

/* a menu item: its label, and the submenu it opens, or null for an item
 * that chooses */
struct editor_item {
    const char *label;
    struct editor_menu *submenu;
};

struct editor_menu {
    struct editor_item *items;
    int count;
};

/* an open menu, at x and y in console cells. *selection is the index of
 * its selected item, and selection[1] its submenu's */
struct editor_menu_state {
    struct editor_menu *menu;
    int x;
    int y;
    int width; /* of its widest label */
    /* its open submenu, or null. heap_alloc's failure is unchecked */
    struct editor_menu_state *child;
    int *selection;
};

/* a pad shortcut: a button, and the command it gives, as the choices a
 * menu would make */
struct editor_shortcut {
    Uint32 buttons;
    int command[CHOICES];
};

/* path_editor_menu_run's results: the menu stays open, an item is chosen,
 * or B closes the menu */
enum menu_choice { CHOICE_PENDING, CHOICE_MADE, CHOICE_CANCELLED };

/* path_editor_run's states: no keys selected, or a run of keys selected */
enum editor_state { STATE_NORMAL, STATE_SELECTED };

/* the menu path_editor_run has open: none, or its state's */
enum open_menu { NO_MENU, NORMAL_MENU_OPEN, SELECTED_MENU_OPEN };

/* the commands while no keys are selected. the main menu's items come
 * first, in order, then NORMAL_MENU and NORMAL_NONE */
enum normal_command {
    NORMAL_ADD, /* add a key at the cursor */
    NORMAL_SELECT, /* select the nearest key, or all of them */
    NORMAL_ROT_EDIT, /* edit the keys' angles */
    NORMAL_ORDER, /* set the rotation order ROT EDIT shows angles in */
    NORMAL_PREVIEW, /* preview the keys as a path */
    NORMAL_SPEED, /* set the preview's speed */
    /* show, hide or toggle the readout of the cursor, the key count, the
     * rotation order and the speed */
    NORMAL_INFO,
    NORMAL_FILE, /* save or load, absolute or relative to the cursor */
    NORMAL_EXIT,
    NORMAL_MENU, /* open the main menu */
    NORMAL_NONE
};

/* the commands while keys are selected. path_editor_selected_menu's items
 * come first, in order, then SELECTED_MENU and SELECTED_NONE */
enum selected_command {
    SELECTED_SELECT, /* select none, which ends the selection, or all */
    SELECTED_DELETE,
    SELECTED_INTERPOLATE, /* subdivide the selection, LINEAR or SAITOLINE3D */
    SELECTED_SPACE, /* AVERAGE VEL: space the keys evenly along their path */
    SELECTED_ROT_EDIT,
    SELECTED_PREVIEW,
    SELECTED_SPEED,
    SELECTED_INFO, /* no case takes it, so it does nothing */
    SELECTED_MENU, /* open path_editor_selected_menu */
    SELECTED_NONE
};

/* the submenus' items by index, as path_editor_run reads them from the
 * command a menu or a shortcut gives */
enum { SELECT_NEAREST, SELECT_ALL };
enum { SELECTION_NONE, SELECTION_ALL };
/* INFO_TOGGLE is no item's: only Y's shortcut gives it */
enum { INFO_SHOW, INFO_HIDE, INFO_TOGGLE };
enum { FILE_SAVE, FILE_LOAD };
enum { INTERPOLATE_LINEAR, INTERPOLATE_CURVE };
enum { CONFIRM_SURE };

/* where saved points and graphed angles count from: 0, or a reference.
 * FILE's reference is the cursor, and ROT EDIT's the path's heading.
 * ORIGINS counts them */
enum origin { ORIGIN_ABSOLUTE, ORIGIN_RELATIVE, ORIGINS };

/* the number of an array's elements, as an int */
#define LENGTH(array) ((int)(sizeof(array) / sizeof((array)[0])))

/* the console's cells, in pixels each way, and the margin of a menu's
 * background past its cells */
#define CELL 16
#define MENU_MARGIN 8

/* ROT EDIT's graph of an axis. its readout is on console row
 * GRAPH_ROW(axis). its box starts GRAPH_TOP pixels below that row's top,
 * and its height spans a turn */
#define GRAPH_ROW(axis) (((axis) * 8) + 3)
#define GRAPH_TOP 18
#define GRAPH_LEFT 48.0F
#define GRAPH_WIDTH 544.0F
#define GRAPH_HEIGHT 92.0F

static NJS_VECTOR path_editor_unit_x = {1.0F, 0.0F, 0.0F};
static NJS_VECTOR path_editor_unit_y = {0.0F, 1.0F, 0.0F};
static NJS_VECTOR path_editor_unit_z = {0.0F, 0.0F, 1.0F};
/* an arrow lying in the task's xz plane and a fin standing up from it, both
 * pointing along +z */
static NJS_POINT3 path_editor_arrow[3] = {
#include "shared/path_editor/path_editor_arrow.inc"
};
static NJS_POINT3 path_editor_fin[3] = {
#include "shared/path_editor/path_editor_fin.inc"
};
/* the arrow in blue and the fin in red, each translucent */
static struct marker path_editor_markers[2] = {
#include "shared/path_editor/path_editor_markers.inc"
};
static const char *path_editor_axis_names[] = {"X", "Y", "Z"};
static const char *path_editor_origin_names[] = {"ABS", "REL"};
static struct editor_item path_editor_confirm_items[] = {
    {"SURE", NULL},
};
static struct editor_menu path_editor_confirm_menu = {
    path_editor_confirm_items, LENGTH(path_editor_confirm_items)};
/* no item opens this menu */
static struct editor_item path_editor_toggle_items[] = {
    {"ON", NULL},
    {"OFF", NULL},
};
static struct editor_menu path_editor_toggle_menu = {
    path_editor_toggle_items, LENGTH(path_editor_toggle_items)};
static struct editor_item path_editor_select_items[] = {
    {"NEAREST", NULL},
    {"ALL", NULL},
};
static struct editor_menu path_editor_select_menu = {
    path_editor_select_items, LENGTH(path_editor_select_items)};
static struct editor_item path_editor_order_items[] = {
    {"XYZ", NULL}, {"XZY", NULL}, {"YXZ", NULL},
    {"YZX", NULL}, {"ZXY", NULL}, {"ZYX", NULL},
};
static struct editor_menu path_editor_order_menu = {
    path_editor_order_items, LENGTH(path_editor_order_items)};
static struct editor_item path_editor_origin_items[] = {
    {"ABSOLUTE", NULL},
    {"RELATIVE", NULL},
};
static struct editor_menu path_editor_origin_menu = {
    path_editor_origin_items, LENGTH(path_editor_origin_items)};
static struct editor_item path_editor_file_items[] = {
    {"SAVE", &path_editor_origin_menu},
    {"LOAD", &path_editor_origin_menu},
};
static struct editor_menu path_editor_file_menu = {
    path_editor_file_items, LENGTH(path_editor_file_items)};
static Float path_editor_speeds[] = {0.01F, 0.02F, 0.05F, 0.10F,
                                     0.20F, 0.50F, 1.0F,  2.0F};
static struct editor_item path_editor_speed_items[] = {
    {"0.01", NULL}, {"0.02", NULL}, {"0.05", NULL}, {"0.10", NULL},
    {"0.20", NULL}, {"0.50", NULL}, {"1.00", NULL}, {"2.00", NULL},
};
static struct editor_menu path_editor_speed_menu = {
    path_editor_speed_items, LENGTH(path_editor_speed_items)};
static struct editor_item path_editor_info_items[] = {
    {"SHOW", NULL},
    {"HIDE", NULL},
};
static struct editor_menu path_editor_info_menu = {
    path_editor_info_items, LENGTH(path_editor_info_items)};
static struct editor_item path_editor_normal_items[] = {
    {"ADD POINT", NULL},
    {"SELECT", &path_editor_select_menu},
    {"ROT EDIT", NULL},
    {"ROT ORDER", &path_editor_order_menu},
    {"PREVIEW", NULL},
    {"SPEED", &path_editor_speed_menu},
    {"INFO", &path_editor_info_menu},
    {"FILE", &path_editor_file_menu},
    {"EXIT", &path_editor_confirm_menu},
};
static struct editor_menu path_editor_normal_menu = {
    path_editor_normal_items, LENGTH(path_editor_normal_items)};
static struct editor_item path_editor_selection_items[] = {
    {"NONE", NULL},
    {"ALL", NULL},
};
static struct editor_menu path_editor_selection_menu = {
    path_editor_selection_items, LENGTH(path_editor_selection_items)};
static int path_editor_divisions[] = {2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16};
static struct editor_item path_editor_division_items[] = {
    {" 2", NULL}, {" 3", NULL}, {" 4", NULL}, {" 5", NULL},
    {" 6", NULL}, {" 7", NULL}, {" 8", NULL}, {"10", NULL},
    {"12", NULL}, {"14", NULL}, {"16", NULL},
};
static struct editor_menu path_editor_division_menu = {
    path_editor_division_items, LENGTH(path_editor_division_items)};
static struct editor_item path_editor_interpolation_items[] = {
    {"LINEAR", &path_editor_division_menu},
    {"SAITOLINE3D", &path_editor_division_menu},
};
static struct editor_menu path_editor_interpolation_menu = {
    path_editor_interpolation_items, LENGTH(path_editor_interpolation_items)};
static struct editor_item path_editor_selected_items[] = {
    {"SELECT", &path_editor_selection_menu},
    {"DELETE", NULL},
    {"INTERPOLATE", &path_editor_interpolation_menu},
    {"AVERAGE VEL", NULL},
    {"ROT EDIT", NULL},
    {"PREVIEW", NULL},
    {"SPEED", &path_editor_speed_menu},
    {"INFO", &path_editor_info_menu},
};
static struct editor_menu path_editor_selected_menu = {
    path_editor_selected_items, LENGTH(path_editor_selected_items)};
/* Y toggles the readout, X selects the nearest key, A adds a key and B
 * opens the menu. a row with button 0 never fires */
static struct editor_shortcut path_editor_normal_shortcuts[] = {
    {PDD_DGT_TY, {NORMAL_INFO, INFO_TOGGLE}},
    {PDD_DGT_TX, {NORMAL_SELECT}},
    {0, {NORMAL_NONE}},
    {PDD_DGT_TA, {NORMAL_ADD}},
    {0, {NORMAL_PREVIEW}},
    {0, {NORMAL_EXIT}},
    {PDD_DGT_TB, {NORMAL_MENU}},
};
/* A ends the selection, and B opens path_editor_selected_menu. Y gives
 * SELECTED_INFO, which does nothing, and a row with button 0 never fires */
static struct editor_shortcut path_editor_selected_shortcuts[] = {
    {PDD_DGT_TY, {SELECTED_INFO, INFO_TOGGLE}},
    {0, {SELECTED_NONE}},
    {PDD_DGT_TA, {SELECTED_SELECT}},
    {0, {SELECTED_DELETE}},
    {0, {SELECTED_PREVIEW}},
    {0, {SELECTED_SPACE}},
    {PDD_DGT_TB, {SELECTED_MENU}},
};

/* @hack spelling: square, a function where SQUARE would do. SHC expands
 * each call, and keeps its product an FMUL that an FADD then adds, as the
 * image does (FMUL FR7,FR9 and FADD FR10,FR9 at 0x0cb83182 and 0x0cb83192
 * in EV001E). through SQUARE, the products fuse into FMACs, and
 * path_editor_curve, path_editor_nearest and path_editor_space each run 8
 * bytes short. summed over the three, 435 of 475 words differ */
static Float square(Float value)
{
    return value * value;
}

/* set the console's text color: white, or red pulsing to yellow for a
 * selected item. a pulse takes 64 steps of play time */
static void path_editor_highlight(int selected)
{
    int phase;
    int green;

    if (selected) {
        phase = play_time_get() << 3;
        if (phase & 0x100) {
            green = phase & 0xff;
        } else {
            green = 255 - (phase & 0xff);
        }
        njPrintColor(0xffff0000 | (green << 8));
    } else {
        njPrintColor(0xffffffff);
    }
}

/* draw a line in the world from one point to another, each end in its own
 * color */
static void path_editor_line(Float x1, Float y1, Float z1, Uint32 color1,
                             Float x2, Float y2, Float z2, Uint32 color2)
{
    NJS_POINT3 points[2];
    NJS_COLOR colors[2];
    NJS_POINT3COL line;

    line.p = points;
    line.col = colors;
    line.tex = NULL;
    line.num = 2;
    points[0].x = x1;
    points[0].y = y1;
    points[0].z = z1;
    colors[0].color = color1;
    points[1].x = x2;
    points[1].y = y2;
    points[1].z = z2;
    colors[1].color = color2;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njDrawLine3D(&line, 1, NJD_TRANSPARENT);
}

/* fill a rectangle of the screen, in pixels, at priority */
static void path_editor_rect(Float left, Float top, Float right, Float bottom,
                             Float priority, Uint32 color)
{
    NJS_POINT2COL polygon;
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];

    polygon.p = points;
    polygon.col = colors;
    polygon.tex = NULL;
    polygon.num = 4;
    colors[0].color = colors[1].color = colors[2].color = colors[3].color =
        color;
    points[0].x = points[1].x = left;
    points[2].x = points[3].x = right;
    points[0].y = points[3].y = top;
    points[1].y = points[2].y = bottom;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njDrawPolygon2D(&polygon, 4, priority, NJD_FILL | NJD_TRANSPARENT);
}

/* rewrite ang, which turns in rotation order from, to make the same turn in
 * order to. the current matrix is left as it was */
static void path_editor_reorder(struct angles *ang, int from, int to)
{
    NJS_VECTOR vector;

    njPushMatrix(&_unit_matrix_);
    switch (from) {
    case ORDER_XYZ:
        njRotateXYZ(NULL, ang->x, ang->y, ang->z);
        break;
    case ORDER_XZY:
        TURN(njRotateY, ang->y);
        TURN(njRotateZ, ang->z);
        TURN(njRotateX, ang->x);
        break;
    case ORDER_YXZ:
        TURN(njRotateZ, ang->z);
        TURN(njRotateX, ang->x);
        TURN(njRotateY, ang->y);
        break;
    case ORDER_YZX:
        TURN(njRotateX, ang->x);
        TURN(njRotateZ, ang->z);
        TURN(njRotateY, ang->y);
        break;
    case ORDER_ZXY:
        TURN(njRotateY, ang->y);
        TURN(njRotateX, ang->x);
        TURN(njRotateZ, ang->z);
        break;
    case ORDER_ZYX:
        TURN(njRotateX, ang->x);
        TURN(njRotateY, ang->y);
        TURN(njRotateZ, ang->z);
        break;
    }
    switch (to) {
    case ORDER_XYZ:
        njCalcVector(NULL, &path_editor_unit_x, &vector);
        ang->y = (Angle)(asinf(-vector.z) * ANGLE_PER_RADIAN);
        ang->z = (Angle)(atan2f(vector.y, vector.x) * ANGLE_PER_RADIAN);
        njInvertMatrix(NULL);
        TURN(njRotateZ, ang->z);
        njCalcVector(NULL, &path_editor_unit_y, &vector);
        ang->x = (Angle)(atan2f(-vector.z, vector.y) * ANGLE_PER_RADIAN);
        break;
    case ORDER_XZY:
        njCalcVector(NULL, &path_editor_unit_x, &vector);
        ang->z = (Angle)(asinf(vector.y) * ANGLE_PER_RADIAN);
        ang->y = (Angle)(atan2f(-vector.z, vector.x) * ANGLE_PER_RADIAN);
        njInvertMatrix(NULL);
        TURN(njRotateY, ang->y);
        njCalcVector(NULL, &path_editor_unit_z, &vector);
        ang->x = (Angle)(atan2f(vector.y, vector.z) * ANGLE_PER_RADIAN);
        break;
    case ORDER_YXZ:
        njCalcVector(NULL, &path_editor_unit_y, &vector);
        ang->x = (Angle)(asinf(vector.z) * ANGLE_PER_RADIAN);
        ang->z = (Angle)(atan2f(-vector.x, vector.y) * ANGLE_PER_RADIAN);
        njInvertMatrix(NULL);
        TURN(njRotateZ, ang->z);
        njCalcVector(NULL, &path_editor_unit_x, &vector);
        ang->y = (Angle)(atan2f(vector.z, vector.y) * ANGLE_PER_RADIAN);
        break;
    case ORDER_YZX:
        njCalcVector(NULL, &path_editor_unit_y, &vector);
        ang->z = (Angle)(asinf(-vector.x) * ANGLE_PER_RADIAN);
        ang->x = (Angle)(atan2f(vector.z, vector.y) * ANGLE_PER_RADIAN);
        njInvertMatrix(NULL);
        TURN(njRotateX, ang->x);
        njCalcVector(NULL, &path_editor_unit_z, &vector);
        ang->y = (Angle)(atan2f(-vector.x, vector.z) * ANGLE_PER_RADIAN);
        break;
    case ORDER_ZXY:
        njCalcVector(NULL, &path_editor_unit_z, &vector);
        ang->x = (Angle)(asinf(-vector.y) * ANGLE_PER_RADIAN);
        ang->y = (Angle)(atan2f(vector.x, vector.z) * ANGLE_PER_RADIAN);
        njInvertMatrix(NULL);
        TURN(njRotateY, ang->y);
        njCalcVector(NULL, &path_editor_unit_x, &vector);
        ang->z = (Angle)(atan2f(-vector.y, vector.x) * ANGLE_PER_RADIAN);
        break;
    case ORDER_ZYX:
        njCalcVector(NULL, &path_editor_unit_z, &vector);
        ang->y = (Angle)(asinf(vector.x) * ANGLE_PER_RADIAN);
        ang->x = (Angle)(atan2f(-vector.y, vector.z) * ANGLE_PER_RADIAN);
        njInvertMatrix(NULL);
        TURN(njRotateX, ang->x);
        njCalcVector(NULL, &path_editor_unit_y, &vector);
        ang->z = (Angle)(atan2f(vector.x, vector.y) * ANGLE_PER_RADIAN);
        break;
    }
    njPopMatrix(1);
}

/* set *out to the curve's point fraction of the way from points[1] to
 * points[2]. fraction runs from 0 to 1. the curve leaves points[1] along
 * points[2] - points[0], and reaches points[2] along points[3] - points[1].
 * it blends a line in each direction by a smoothstep. each direction is
 * scaled to half the segment's length */
static void path_editor_curve(NJS_POINT3 *points, NJS_POINT3 *out,
                              Float fraction)
{
    Float x1 = points[1].x;
    Float y1 = points[1].y;
    Float z1 = points[1].z;
    Float x2 = points[2].x;
    Float y2 = points[2].y;
    Float z2 = points[2].z;
    Float squared_length = square(x2 - x1) + square(y2 - y1) + square(z2 - z1);
    Float ax = x2 - points[0].x;
    Float ay = y2 - points[0].y;
    Float az = z2 - points[0].z;
    Float bx;
    Float by;
    Float bz;
    Float scale;
    Float blend;

    scale = njSqrt(squared_length / ((ax * ax) + (ay * ay) + (az * az))) * 0.5F;
    ax *= scale;
    ay *= scale;
    az *= scale;
    bx = x1 - points[3].x;
    by = y1 - points[3].y;
    bz = z1 - points[3].z;
    scale = njSqrt(squared_length / ((bx * bx) + (by * by) + (bz * bz))) * 0.5F;
    bx *= scale;
    by *= scale;
    bz *= scale;
    blend = 1.0F + (fraction * (fraction * (-3.0F + (2.0F * fraction))));
    out->x = ((x1 + (fraction * ax)) * blend) +
             ((1.0F - blend) * (x2 + ((1.0F - fraction) * bx)));
    out->y = ((y1 + (fraction * ay)) * blend) +
             ((1.0F - blend) * (y2 + ((1.0F - fraction) * by)));
    out->z = ((z1 + (fraction * az)) * blend) +
             ((1.0F - blend) * (z2 + ((1.0F - fraction) * bz)));
}

/* the length of the menu's widest label */
static int path_editor_menu_width(struct editor_menu *menu)
{
    int i;
    int width;
    struct editor_item *items = menu->items;
    int count = menu->count;

    width = 0;
    for (i = 0; i < count; ++i) {
        int length = strlen(items[i].label);

        if (width < length) {
            width = length;
        }
    }
    return width;
}

/* run a pass of the menu, and draw it. an open submenu takes the pass's
 * buttons. otherwise up and down move the selection, and A chooses the
 * item. A on an item with a submenu opens the submenu beside it, at its
 * item 0. the pass returns CHOICE_PENDING while the menu stays open. it
 * returns CHOICE_MADE once this menu or a submenu chooses an item without a
 * submenu. it returns CHOICE_CANCELLED for B. a submenu's CHOICE_CANCELLED
 * closes it and leaves this menu open */
static int path_editor_menu_run(struct editor_menu_state *state)
{
    int x = state->x;
    int y = state->y;
    struct editor_menu_state *child = state->child;
    struct editor_item *items = state->menu->items;
    int count = state->menu->count;
    int right;
    int width = state->width;
    int *selection = state->selection;
    int selected = *selection;
    int location = NJM_LOCATION(x, y);
    int result = CHOICE_PENDING;
    int i;
    Float left_edge;
    Float right_edge;
    Float top_edge;
    Float bottom_edge;

    right = width + x;
    if (child) {
        result = path_editor_menu_run(child);
        switch (result) {
        case CHOICE_CANCELLED:
            result = CHOICE_PENDING;
            /* fall through */
        case CHOICE_MADE:
            heap_free(child);
            child = NULL;
            break;
        case CHOICE_PENDING:
            break;
        }
        for (i = 0; i < count; ++i) {
            njPrintColor(i == selected ? 0xffffff00 : 0xff777777);
            njPrintC(location + i, items[i].label);
            njPrintColor(0xffffffff);
        }
    } else {
        if (per_info[0]->press & PDD_DGT_KU) {
            if (--selected < 0) {
                selected = count - 1;
            }
        }
        if (per_info[0]->press & PDD_DGT_KD) {
            if (++selected >= count) {
                selected = 0;
            }
        }
        if (per_info[0]->press & PDD_DGT_TA) {
            struct editor_menu *menu = items[selected].submenu;

            if (menu) {
                child = heap_alloc(sizeof(*child));
                child->menu = menu;
                child->x = right + 1;
                child->y = y + selected;
                child->width = path_editor_menu_width(menu);
                child->child = NULL;
                child->selection = selection + 1;
                selection[1] = 0;
            } else {
                result = CHOICE_MADE;
            }
        }
        if (per_info[0]->press & PDD_DGT_TB) {
            result = CHOICE_CANCELLED;
        }
        for (i = 0; i < count; ++i) {
            path_editor_highlight(i == selected);
            njPrintC(location + i, items[i].label);
            njPrintColor(0xffffffff);
        }
    }
    *selection = selected;
    state->child = child;
    left_edge = (x * CELL) - MENU_MARGIN;
    right_edge = (right * CELL) + MENU_MARGIN;
    top_edge = (y * CELL) - MENU_MARGIN;
    bottom_edge = ((y + count) * CELL) + MENU_MARGIN;
    path_editor_rect(left_edge, top_edge, right_edge, bottom_edge, -1.0F,
                     0x55000000);
    return result;
}

static void path_editor_preview_destroy(struct task *task)
{
    struct task_work *work = task->work;

    event_free(work);
    PREVIEW(work)->release(PREVIEW(work)->path);
}

/* draw the markers at the task. they turn by its ang.z, then its ang.x,
 * then a half turn less its ang.y. event_paths_step sets ang.y to a half
 * turn less the path's, so the markers take the path's own */
static void path_editor_preview_draw(struct task *task)
{
    NJS_POINT3COL polygon;
    NJS_COLOR colors[8];
    struct task_work *work = task->work;
    unsigned int i;
    int j;
    struct marker *marker;

    polygon.col = colors;
    polygon.tex = NULL;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, -work->ang.y + 0x8000);
    for (i = 0; i < 2; ++i) {
        marker = &path_editor_markers[i];
        for (j = 0; j < marker->count; ++j) {
            colors[j].color = marker->color;
        }
        polygon.p = marker->points;
        polygon.num = marker->count;
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njDrawPolygon3D(&polygon, marker->count, NJD_FILL | NJD_TRANSPARENT);
    }
    njPopMatrix(1);
}

/* follow the path, and draw the markers. once no path is left, kill the
 * task instead. its draw draws the markers as well */
static void path_editor_preview_step(struct task *task)
{
    struct task_work *work = task->work;

    event_paths_step(work);
    if (work->event->paths == NULL) {
        task_kill(task);
    } else {
        path_editor_preview_draw(task);
    }
}

/* start a task that follows path at speed. the task ends with the path, and
 * its destructor calls release with argument. task_create's failure is
 * unchecked */
static void path_editor_preview(struct event_path *path, Float speed,
                                void (*release)(struct event_path *path),
                                struct event_path *argument)
{
    struct task *task = task_create(TASK_WORK, 5, path_editor_preview_step);

    task->draw = path_editor_preview_draw;
    task->step = path_editor_preview_step;
    task->destructor = path_editor_preview_destroy;
    event_alloc(task->work);
    event_set_flags(task, EVENT_IGNORE_GROUND);
    event_set_shadow(task, 1.0F);
    event_paths_add(task->work, path, speed, 0);
    PREVIEW(task->work)->release = release;
    PREVIEW(task->work)->path = argument;
}

/* graph count keys' angles about axis, with the selected key's angle in
 * hex. with origin ORIGIN_RELATIVE, each angle is graphed less its key's
 * baseline. the graph runs from -0x8000 at its bottom to 0x8000 at its top.
 * a curve that leaves one edge comes back in at the other. a line marks
 * each key, the selected key's bright while active. count must be 2 or
 * more */
static void path_editor_graph(union key_angles *angles,
                              union key_angles *baseline, int count, int axis,
                              int origin, int selected, int active)
{
    NJS_POINT2COL polygon;
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];
    int location = GRAPH_ROW(axis);
    int row = location;
    Float left;
    Float top;
    Float value;
    Float previous;
    Float crossing;
    int raw;
    int old_raw;
    int i;

    location |= NJM_LOCATION(3, 0);
    njPrintC(location, path_editor_axis_names[axis]);
    njPrintC(NJM_LOCATION(2, 0) + location, path_editor_origin_names[origin]);
    if (origin == ORIGIN_RELATIVE) {
        /* @hack type: 6U, which makes this arm's sum unsigned and the other
         * arm's int, so SHC adds NJM_LOCATION(6, 0) in each arm as the image
         * does (MOV.L at 0x0cb827a8 and 0x0cb82800 in EV001E). with 6, it
         * adds it once, ahead of the test, the function runs 8 bytes short,
         * and 397 of 448 words differ */
        njPrintH(NJM_LOCATION(6U, 0) + location,
                 angles[selected].axis[axis] - baseline[selected].axis[axis],
                 4);
        njPrintC(NJM_LOCATION(10, 0) + location, "+");
        njPrintH(NJM_LOCATION(11, 0) + location, baseline[selected].axis[axis],
                 4);
    } else {
        njPrintH(NJM_LOCATION(6, 0) + location, angles[selected].axis[axis], 4);
    }
    left = GRAPH_LEFT;
    top = (row * CELL) + GRAPH_TOP;
    polygon.p = points;
    polygon.col = colors;
    polygon.tex = NULL;
    colors[0].color = colors[1].color = colors[2].color = colors[3].color =
        0xbb000000;
    points[0].x = points[1].x = left;
    points[2].x = points[3].x = GRAPH_LEFT + GRAPH_WIDTH;
    points[0].y = points[3].y = top;
    points[1].y = points[2].y = top + GRAPH_HEIGHT;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    polygon.num = 4;
    njDrawPolygon2D(&polygon, 4, -1.3F, NJD_FILL | NJD_TRANSPARENT);
    colors[0].color = colors[1].color = 0xff777777;
    points[0].x = left;
    points[1].x = GRAPH_LEFT + GRAPH_WIDTH;
    polygon.num = 2;
    for (i = 0; i <= 4; ++i) {
        points[0].y = points[1].y = top + (i * (GRAPH_HEIGHT / 4));
        njDrawLine2D(&polygon, 1, -1.2F, 0);
    }
    points[0].y = top;
    points[1].y = top + GRAPH_HEIGHT;
    polygon.num = 2;
    for (i = 0; i < count; ++i) {
        Uint32 color;

        points[0].x = points[1].x = left + (GRAPH_WIDTH * i / (count - 1));
        if (active && i == selected) {
            color = 0xff00ffff;
        } else {
            color = 0xff777777;
        }
        colors[0].color = colors[1].color = color;
        njDrawLine2D(&polygon, 1, -1.1F, 0);
    }
    colors[0].color = colors[1].color = 0xffffff00;
    raw = angles[0].axis[axis];
    if (origin == ORIGIN_RELATIVE) {
        raw -= baseline[0].axis[axis];
    }
    value = (Sint16)raw;
    points[1].x = left;
    points[1].y = top + (GRAPH_HEIGHT * (0.5F - (value * (1.0F / 65536.0F))));
    for (i = 1; i < count; ++i) {
        points[0] = points[1];
        old_raw = raw;
        raw = angles[i].axis[axis];
        previous = value;
        if (origin == ORIGIN_RELATIVE) {
            raw -= baseline[i].axis[axis];
        }
        value = (Sint16)(raw - old_raw) + previous;
        if (value < -32768.0F) {
            crossing = (value + 32768.0F) / (previous - value);
            crossing = left + (GRAPH_WIDTH * (i + crossing) / (count - 1));
            points[1].x = crossing;
            points[1].y = top + GRAPH_HEIGHT;
            njDrawLine2D(&polygon, 1, -1.1F, 0);
            points[0].x = crossing;
            points[0].y = top;
            value += 65536.0F;
        } else if (value >= 32768.0F) {
            crossing = (value - 32768.0F) / (previous - value);
            crossing = left + (GRAPH_WIDTH * (i + crossing) / (count - 1));
            points[1].x = crossing;
            points[1].y = top;
            njDrawLine2D(&polygon, 1, -1.1F, 0);
            points[0].x = crossing;
            points[0].y = top + GRAPH_HEIGHT;
            value -= 65536.0F;
        }
        points[1].x = left + (GRAPH_WIDTH * i / (count - 1));
        points[1].y =
            top + (GRAPH_HEIGHT * (0.5F - (value * (1.0F / 65536.0F))));
        njDrawLine2D(&polygon, 1, -1.1F, 0);
    }
}

/* @hack type: a key's angles through a pointer to the union, which reads
 * values[first]'s angle twice and reference[first]'s twice, as the image
 * does (0x0cb82e02 and 0x0cb82e1a in EV001E). through key.axis[axis], SHC
 * reads each once and keeps it, path_editor_edit_angles runs 4 bytes
 * short, and 177 of its 460 words differ */
#define KEY_AXES(key) ((Angle *)(union key_angles *)&(key).ang)

/* ROT EDIT: graph each axis of count keys' values, and edit them until B.
 * up and down pick the axis, and left and right the key. Y toggles the
 * axis between ORIGIN_ABSOLUTE and ORIGIN_RELATIVE to reference, and each
 * axis starts relative. with A held, up and down step the angle to its
 * next whole ANGLE_STEP either way. it steps once each pass.
 *
 * X held on one key and released on another spans the keys between. their
 * angles take even steps from the first key's to the last's, the short way
 * round. while X is held, up and down add or take away a whole turn. count
 * must be 2 or more */
static void path_editor_edit_angles(union key_angles *values,
                                    union key_angles *reference, int count)
{
    int active = 1;
    int spanning = 0;
    int origins[3] = {ORIGIN_RELATIVE, ORIGIN_RELATIVE, ORIGIN_RELATIVE};
    int axis = 0;
    int selected = 0;
    int anchor;
    int turns;
    int first;
    int i;
    int length;
    Angle value;
    Angle delta;
    Float y;
    Float left;
    Float right;
    Float graph_left;

#ifdef GCC
    /* GCC can't see that anchor and turns are set wherever they are read:
     * the loop reads them only while spanning is 1, and spanning becomes 1
     * only in its case 0, which sets both */
    anchor = 0;
    turns = 0;
#endif
    do {
        thread_yield();
        if (per_info[0]->press & PDD_DGT_TB) {
            active = 0;
        }
        if (per_info[0]->press & PDD_DGT_TY) {
            if (++origins[axis] >= ORIGINS) {
                origins[axis] = ORIGIN_ABSOLUTE;
            }
        }
        if (per_info[0]->on & PDD_DGT_TA) {
            if (spanning == 0) {
                /* @hack type: the key as an array, then the axis, so SHC
                 * adds the key's address to the offset as the image does
                 * (ADD R3,R4 at 0x0cb82c12 in EV001E). through
                 * &values[selected].axis[axis], it copies the address first
                 * (MOV R3,R1), and 43 of 460 words differ */
                Angle *selected_angle = *(angle_row *)&values[selected] + axis;

                if (per_info[0]->on & PDD_DGT_KU) {
                    *selected_angle =
                        (*selected_angle + ANGLE_STEP) & WHOLE_STEPS;
                }
                if (per_info[0]->on & PDD_DGT_KD) {
                    *selected_angle = (*selected_angle - 1) & WHOLE_STEPS;
                }
            }
        } else {
            if (spanning == 1 && (per_info[0]->on & PDD_DGT_TX)) {
                if (per_info[0]->press & PDD_DGT_KU) {
                    turns++;
                }
                if (per_info[0]->press & PDD_DGT_KD) {
                    turns--;
                }
            } else {
                if (per_info[0]->press & PDD_DGT_KU) {
                    if (--axis < 0) {
                        axis = 2;
                    }
                }
                if (per_info[0]->press & PDD_DGT_KD) {
                    if (++axis >= 3) {
                        axis = 0;
                    }
                }
            }
            if (per_info[0]->press & PDD_DGT_KL) {
                if (--selected < 0) {
                    selected = count - 1;
                }
            }
            if (per_info[0]->press & PDD_DGT_KR) {
                if (++selected >= count) {
                    selected = 0;
                }
            }
        }
        switch (spanning) {
        case 0:
            if (per_info[0]->on & PDD_DGT_TX) {
                spanning = 1;
                anchor = selected;
                turns = 0;
            }
            break;
        case 1:
            njPrintD(NJM_LOCATION(23, 2), anchor, 3);
            njPrintC(NJM_LOCATION(26, 2), "-");
            njPrintD(NJM_LOCATION(5, GRAPH_ROW(axis) - 1), turns, 3);
            value = values[anchor].axis[axis];
            if (origins[axis] == ORIGIN_RELATIVE) {
                value -= reference[anchor].axis[axis];
            }
            njPrintH(NJM_LOCATION(9, GRAPH_ROW(axis) - 1), value, 4);
            graph_left = GRAPH_LEFT;
            y = (GRAPH_ROW(axis) * CELL) + GRAPH_TOP;
            left = graph_left + (GRAPH_WIDTH * anchor / (count - 1));
            right = graph_left + (GRAPH_WIDTH * selected / (count - 1));
            path_editor_rect(left, y, right, y + GRAPH_HEIGHT, -1.25F,
                             0xff007700);
            if (!(per_info[0]->on & PDD_DGT_TX)) {
                if (anchor != selected) {
                    if (anchor < selected) {
                        first = anchor;
                        length = selected - anchor;
                    } else {
                        first = selected;
                        length = anchor - selected;
                    }
                    value = KEY_AXES(values[first])[axis];
                    delta = KEY_AXES(values[first + length])[axis] -
                            KEY_AXES(values[first])[axis];
                    if (origins[axis] == ORIGIN_RELATIVE) {
                        value -= KEY_AXES(reference[first])[axis];
                        delta += KEY_AXES(reference[first])[axis] -
                                 KEY_AXES(reference[first + length])[axis];
                    }
                    delta = (Sint16)delta;
                    delta += turns * 65536;
                    for (i = 1; i < length; i++) {
                        Angle interpolated;

                        interpolated = value + (delta * i / length);
                        if (origins[axis] == ORIGIN_RELATIVE) {
                            interpolated +=
                                KEY_AXES(reference[first + i])[axis];
                        }
                        KEY_AXES(values[first + i])[axis] = interpolated;
                    }
                }
                spanning = 0;
            }
            break;
        }
        for (i = 0; i < 3; i++) {
            path_editor_graph(values, reference, count, i, origins[i], selected,
                              i == axis);
        }
        njPrintD(NJM_LOCATION(27, 2), selected, 3);
        njPrintC(NJM_LOCATION(30, 2), "/");
        njPrintD(NJM_LOCATION(31, 2), count - 1, 3);
    } while (active);
}

#undef KEY_AXES

/* draw the cursor at x, y and z. its lines run 2 along x and z, and twice
 * GUIDE_LENGTH along y */
static void path_editor_cursor(Float x, Float y, Float z)
{
    path_editor_line(x - 1.0F, y, z, 0xffff00ff, x + 1.0F, y, z, 0xffff00ff);
    path_editor_line(x, y - GUIDE_LENGTH, z, 0xff00ffff, x, y + GUIDE_LENGTH, z,
                     0xff00ffff);
    path_editor_line(x, y, z - 1.0F, 0xffffff00, x, y, z + 1.0F, 0xffffff00);
}

/* draw the cursor while keys are selected. it is a square of side 1 about
 * x, y and z, in the xz plane */
static void path_editor_selection_cursor(Float x, Float y, Float z)
{
    path_editor_line(x - 0.5F, y, z - 0.5F, 0xffffffff, x + 0.5F, y, z - 0.5F,
                     0xffffffff);
    path_editor_line(x - 0.5F, y, z + 0.5F, 0xffffffff, x + 0.5F, y, z + 0.5F,
                     0xffffffff);
    path_editor_line(x - 0.5F, y, z - 0.5F, 0xffffffff, x - 0.5F, y, z + 0.5F,
                     0xffffffff);
    path_editor_line(x + 0.5F, y, z - 0.5F, 0xffffffff, x + 0.5F, y, z + 0.5F,
                     0xffffffff);
}

/* draw the path through count keys, and a line GUIDE_LENGTH down from each
 * key. a
 * segment between keys first and last runs green to cyan, and any other
 * red to yellow. the lines down from keys first to last are darker. first
 * and last can come in either order, and -1 for both selects no key */
static void path_editor_draw(NJS_POINT3 *points, int count, int first, int last)
{
    int i;
    int swap;
    Float x;
    Float y;
    Float z;
    Float previous_x;
    Float previous_y;
    Float previous_z;
    Uint32 color1;
    Uint32 color2;

    if (first > last) {
        swap = first;
        first = last;
        last = swap;
    }
    x = points[0].x;
    y = points[0].y;
    z = points[0].z;
    for (i = 1; i < count; ++i) {
        previous_x = x;
        previous_y = y;
        previous_z = z;
        x = points[i].x;
        y = points[i].y;
        z = points[i].z;
        if (i > first && i <= last) {
            color1 = 0xff00ff00;
            color2 = 0xff00ffff;
        } else {
            color1 = 0xffff0000;
            color2 = 0xffffff00;
        }
        path_editor_line(previous_x, previous_y, previous_z, color1, x, y, z,
                         color2);
    }
    for (i = 0; i < count; ++i) {
        Float guide_x = points[i].x;
        Float guide_y = points[i].y;
        Float guide_z = points[i].z;
        Float bottom_y = guide_y - GUIDE_LENGTH;

        if (i >= first && i <= last) {
            color1 = 0xdd000000;
            color2 = 0xdd000000;
        } else {
            color1 = 0x88000000;
            color2 = 0x88000000;
        }
        path_editor_line(guide_x, guide_y, guide_z, color1, guide_x, bottom_y,
                         guide_z, color2);
    }
}

/* the index of the key nearest the point. count must be at least 1 */
static int path_editor_nearest(NJS_POINT3 *points, int count, Float x, Float y,
                               Float z)
{
    int i;
    int nearest = 0;
    Float distance;
    Float best;

    best = square(points[0].x - x) + square(points[0].y - y) +
           square(points[0].z - z);
    for (i = 1; i < count; ++i) {
        distance = square(points[i].x - x) + square(points[i].y - y) +
                   square(points[i].z - z);
        if (distance < best) {
            nearest = i;
            best = distance;
        }
    }
    return nearest;
}

/* insert a key at x, y and z, with zero angles, at index. the keys from
 * index on move up one, and the caller raises its count */
static void path_editor_insert(NJS_POINT3 *points, union key_angles *angles,
                               int count, int index, Float x, Float y, Float z)
{
    int i;

    for (i = count; i > index; --i) {
        points[i] = points[i - 1];
        angles[i] = angles[i - 1];
    }
    points[index].x = x;
    points[index].y = y;
    points[index].z = z;
    angles[index].ang.x = 0;
    angles[index].ang.y = 0;
    angles[index].ang.z = 0;
}

/* add a key at x, y and z. with fewer than 2 keys, it goes after them.
 * otherwise it goes beside the nearest key, on the side of the
 * segment the point lies more along. at an end key, the end segment runs on
 * past it to stand in for the missing one. the caller raises its count */
static void path_editor_add(NJS_POINT3 *points, union key_angles *angles,
                            int count, Float x, Float y, Float z)
{
    int index;
    Float cx;
    Float cy;
    Float cz;
    Float ax;
    Float ay;
    Float az;
    Float bx;
    Float by;
    Float bz;
    Float dot_a;
    Float dot_b;
    Float weighted_a;
    Float weighted_b;

    if (count < 2) {
        index = count;
    } else {
        index = path_editor_nearest(points, count, x, y, z);
        cx = points[index].x;
        cy = points[index].y;
        cz = points[index].z;
        if (index == 0) {
            ax = cx - points[1].x;
            ay = cy - points[1].y;
            az = cz - points[1].z;
        } else {
            ax = points[index - 1].x - cx;
            ay = points[index - 1].y - cy;
            az = points[index - 1].z - cz;
        }
        if (index == count - 1) {
            bx = cx - points[index - 1].x;
            by = cy - points[index - 1].y;
            bz = cz - points[index - 1].z;
        } else {
            bx = points[index + 1].x - cx;
            by = points[index + 1].y - cy;
            bz = points[index + 1].z - cz;
        }
        cx = x - cx;
        cy = y - cy;
        cz = z - cz;
        dot_a = (cx * ax) + (cy * ay) + (cz * az);
        dot_b = (cx * bx) + (cy * by) + (cz * bz);
        if (dot_a > 0.0F && dot_b < 0.0F) {
            /* toward the key before and away from the key after: the new
             * key goes before the nearest */
        } else if (dot_a < 0.0F && dot_b > 0.0F) {
            ++index;
        } else {
            weighted_a = ((bx * bx) + (by * by) + (bz * bz)) * dot_a * dot_a;
            weighted_b = ((ax * ax) + (ay * ay) + (az * az)) * dot_b * dot_b;
            if (dot_a > 0.0F ? weighted_b > weighted_a
                             : weighted_a > weighted_b) {
                ++index;
            }
        }
    }
    path_editor_insert(points, angles, count, index, x, y, z);
}

/* move keys first to last, in either order, so that key last lands on x,
 * y and z. they all move by the same amount */
static void path_editor_move(NJS_POINT3 *points, int first, int last, Float x,
                             Float y, Float z)
{
    int i;
    int swap;
    Float dx = x - points[last].x;
    Float dy = y - points[last].y;
    Float dz = z - points[last].z;

    if (first > last) {
        swap = first;
        first = last;
        last = swap;
    }
    for (i = first; i <= last; ++i) {
        points[i].x += dx;
        points[i].y += dy;
        points[i].z += dz;
    }
}

/* delete keys first to last, in either order. the keys after them move
 * down, and the caller lowers its count */
static void path_editor_delete(NJS_POINT3 *points, union key_angles *angles,
                               int count, int first, int last)
{
    int i;
    int removed;
    int swap;

    if (first > last) {
        swap = first;
        first = last;
        last = swap;
    }
    ++last;
    removed = last - first;
    for (i = last; i < count; ++i) {
        points[i - removed] = points[i];
        angles[i - removed] = angles[i];
    }
}

/* a preview's release: free the path and its points */
static void path_editor_release(struct event_path *path)
{
    /* @hack spelling: owned, a second name for path, which SHC keeps in R14
     * across the first heap_free (MOV R4,R14 at 0x0cb83496 in EV001E).
     * through path alone, it keeps the path on the stack, the function runs
     * 4 bytes short, and 11 of 11 words differ */
    struct event_path *owned = path;

    heap_free(owned->points);
    heap_free(owned);
}

/* preview count keys at speed. the path and its points come from the
 * heap, unchecked. each angle rounds to the nearest ANGLE_STEP, the unit of
 * an event path's angles */
static void path_editor_play(NJS_POINT3 *points, union key_angles *angles,
                             int count, Float speed)
{
    struct event_path *path = heap_alloc(sizeof(*path));
    struct event_path_point *buffer = heap_alloc(count * sizeof(*buffer));
    int i;

    path->points = buffer;
    path->count = count;
    for (i = 0; i < count; ++i) {
        buffer[i].pos = points[i];
        buffer[i].ang_x =
            (angles[i].ang.x + (ANGLE_STEP / 2)) >> ANGLE_STEP_SHIFT;
        buffer[i].ang_y =
            (angles[i].ang.y + (ANGLE_STEP / 2)) >> ANGLE_STEP_SHIFT;
        buffer[i].ang_z =
            (angles[i].ang.z + (ANGLE_STEP / 2)) >> ANGLE_STEP_SHIFT;
    }
    path_editor_preview(path, speed, path_editor_release, path);
}

/* ROT EDIT on count keys, their angles shown in rotation order order. the
 * keys keep their angles in ORDER_XYZ. ORIGIN_RELATIVE measures each angle
 * from the path's heading at its key. the heading runs from the key before
 * to the key after. at an end, the end key stands in for the missing one.
 * both buffers come from the heap, unchecked */
static void path_editor_rot_edit(NJS_POINT3 *points, union key_angles *angles,
                                 int count, int order)
{
    /* @hack type: int sizes, so SHC works the byte count out once and
     * reloads it from the stack for the second call, as the image does
     * (MOV.L R4,@(4,R15) and MOV.L @(4,R15),R4 at 0x0cb83574 and 0x0cb8357c
     * in EV001E). with size_t sizes, it works out each, the function runs 4
     * bytes long, and 155 of 173 words differ */
    union key_angles *edited = heap_alloc(count * (int)sizeof(*edited));
    union key_angles *tangents = heap_alloc(count * (int)sizeof(*tangents));
    int i;

    for (i = 0; i < count; ++i) {
        NJS_POINT3 *before;
        NJS_POINT3 *after;
        Float dx;
        Float dy;
        Float dz;

        /* @hack spelling: both addresses formed before the tests of i, as the
         * image does (0x0cb83594 to 0x0cb835b6 in EV001E). at the first key,
         * before points one key below the array until it steps up. set by an
         * if and else for each, the function runs 8 bytes short, and 168 of
         * 173 words differ */
        before = &points[i - 1];
        after = &points[i + 1];
        if (i == 0) {
            ++before;
        }
        if (i == count - 1) {
            --after;
        }
        dx = after->x - before->x;
        dy = after->y - before->y;
        dz = after->z - before->z;
        tangents[i].ang.x = (Angle)(atan2f(-dy, njSqrt((dx * dx) + (dz * dz))) *
                                    ANGLE_PER_RADIAN);
        tangents[i].ang.y = (Angle)(atan2f(dx, dz) * ANGLE_PER_RADIAN);
        tangents[i].ang.z = 0;
        edited[i] = angles[i];
        path_editor_reorder(&tangents[i].ang, ORDER_ZXY, order);
        path_editor_reorder(&edited[i].ang, ORDER_XYZ, order);
    }
    path_editor_edit_angles(edited, tangents, count);
    for (i = 0; i < count; ++i) {
        path_editor_reorder(&edited[i].ang, order, ORDER_XYZ);
        angles[i] = edited[i];
    }
    heap_free(edited);
    heap_free(tangents);
}

/* copy the next word of letters and underscores at *cursor into word, with
 * a NUL after it. the scan skips any other bytes before the word, NULs
 * included. *cursor moves past the word */
static void path_editor_read_word(char **cursor, char *word)
{
    char *at = *cursor;
    char letter;

    while (letter = *at, !((letter >= 'A' && letter <= 'Z') ||
                           (letter >= 'a' && letter <= 'z') || letter == '_')) {
        ++at;
    }
    while (letter = *at, (letter >= 'A' && letter <= 'Z') ||
                             (letter >= 'a' && letter <= 'z') ||
                             letter == '_') {
        *word++ = letter;
        ++at;
    }
    *word = 0;
    *cursor = at;
}

/* the bytes a number's text can hold */
#define NUMBER_BYTES "+-0123456789.EeFfGgLl"

/* read the next number at *cursor with %f: a run of NUMBER_BYTES, past any
 * other bytes before it. the run must fit in 31 bytes. *cursor moves past
 * the run */
static Float path_editor_read_number(char **cursor)
{
    char token[32];
    char *at = *cursor;
    char *out = token;
    Float value;
    char letter;

    /* @bug logic: strchr finds NUMBER_BYTES' own NUL, so both loops take
     * a NUL for a byte of the number, and a run goes on past it (strchr at
     * 0x0cb83724 and 0x0cb8373a in EV001E) */
    while (letter = *at, !strchr(NUMBER_BYTES, letter)) {
        ++at;
    }
    while (letter = *at, strchr(NUMBER_BYTES, letter)) {
        *out++ = letter;
        ++at;
    }
    *out = 0;
    *cursor = at;
    sscanf(token, "%f", &value);
    return value;
}

/* save count keys as name.ept, writing its text at MOTION_AREA first. with
 * origin ORIGIN_RELATIVE, the points are saved less x, y and z */
static void path_editor_save(NJS_POINT3 *points, union key_angles *angles,
                             int count, const char *name, int origin, Float x,
                             Float y, Float z)
{
    char filename[32];
    char *cursor = (char *)MOTION_AREA;
    int i;
    Float px;
    Float py;
    Float pz;

    cursor += sprintf(cursor, "/* %s.ept */\r\n", name);
    cursor += sprintf(cursor, "\r\n");
    cursor += sprintf(cursor, "EPATHTABLE\r\n");
    cursor += sprintf(cursor, "EPATHTABLE_NAME  epathtbl_%s[]\r\n", name);
    cursor += sprintf(cursor, "EPATHTABLE_START\r\n");
    cursor += sprintf(cursor, "\r\n");
    for (i = 0; i < count; ++i) {
        px = points[i].x;
        py = points[i].y;
        pz = points[i].z;
        if (origin == ORIGIN_RELATIVE) {
            px -= x;
            py -= y;
            pz -= z;
        }
        cursor += sprintf(cursor, "EPATHDAT_START  /* %d */\r\n", i);
        cursor += sprintf(cursor, "    EPATH_ANG(% 11.5f,% 11.5f,% 11.5f )\r\n",
                          angles[i].ang.x * (360.0F / 65536.0F),
                          angles[i].ang.y * (360.0F / 65536.0F),
                          angles[i].ang.z * (360.0F / 65536.0F));
        cursor += sprintf(cursor, "    EPATH_POS(% 11.5f,% 11.5f,% 11.5f )\r\n",
                          px, py, pz);
        cursor += sprintf(cursor, "EPATHDAT_END\r\n");
        cursor += sprintf(cursor, "\r\n");
    }
    cursor += sprintf(cursor, "EPATHTABLE_END\r\n");
    cursor += sprintf(cursor, "\r\n");
    cursor += sprintf(cursor, "EPATHTAG\r\n");
    cursor += sprintf(cursor, "EPATHTAG_NAME  epathtag_%s\r\n", name);
    cursor += sprintf(cursor, "EPATHTAG_START\r\n");
    cursor += sprintf(cursor, "    EPATHTAG_NMB  %d\r\n", count);
    cursor += sprintf(cursor, "    EPATHTAG_PTR  epathtbl_%s\r\n", name);
    cursor += sprintf(cursor, "EPATHTAG_END\r\n");
    cursor += sprintf(cursor, "\r\n");
    cursor += sprintf(cursor, "/* EOF */\r\n");
    sprintf(filename, "%s.ept", name);
    host_save_file(filename, MOTION_AREA, cursor - (char *)MOTION_AREA);
}

/* load the keys of name.ept into points and angles, reading its text at
 * MOTION_AREA. return their count, or -1 when the host's answer is
 * negative. with origin ORIGIN_RELATIVE, x, y and z are added to the
 * points. the entries run to EPATHTABLE_END. each needs its EPATH_ANG and
 * EPATH_POS, with their numbers, or the reads run past the text */
static int path_editor_load(NJS_POINT3 *points, union key_angles *angles,
                            const char *name, int origin, Float x, Float y,
                            Float z)
{
    char filename[32];
    char word[224];
    char *cursor;
    int result;
    int count;
    Float px;
    Float py;
    Float pz;

    sprintf(filename, "%s.ept", name);
    result = host_load_file(filename, MOTION_AREA);
    if (result < 0) {
        return -1;
    }
    cursor = (char *)MOTION_AREA;
    count = 0;
    for (;;) {
        path_editor_read_word(&cursor, word);
        if (strcmp(word, "EPATHTABLE_END") == 0) {
            break;
        }
        if (strcmp(word, "EPATHDAT_START") == 0) {
            do {
                path_editor_read_word(&cursor, word);
            } while (strcmp(word, "EPATH_ANG") != 0);
            angles[count].ang.x =
                path_editor_read_number(&cursor) * ANGLE_PER_DEGREE;
            angles[count].ang.y =
                path_editor_read_number(&cursor) * ANGLE_PER_DEGREE;
            angles[count].ang.z =
                path_editor_read_number(&cursor) * ANGLE_PER_DEGREE;
            do {
                path_editor_read_word(&cursor, word);
            } while (strcmp(word, "EPATH_POS") != 0);
            px = path_editor_read_number(&cursor);
            py = path_editor_read_number(&cursor);
            pz = path_editor_read_number(&cursor);
            if (origin == ORIGIN_RELATIVE) {
                px += x;
                py += y;
                pz += z;
            }
            points[count].x = px;
            points[count].y = py;
            points[count].z = pz;
            ++count;
        }
    }
    return count;
}

/* LINEAR: cut each of segments segments from key first into subdivisions
 * parts. the keys from first + segments on move up to make room. the room
 * fills from the top down. only points move: each angle keeps its
 * index. the caller raises its count */
static void path_editor_subdivide(NJS_POINT3 *points, int count, int first,
                                  int segments, int subdivisions)
{
    unsigned char *to =
        (unsigned char *)(points + (count + ((subdivisions - 1) * segments)));
    unsigned char *from = (unsigned char *)(points + count);
    Float x;
    Float y;
    Float z;
    Float old_x;
    Float old_y;
    Float old_z;
    int i;
    int j;
    Float fraction;

    /* @hack spelling: byte cursors that move the points a float at a time from
     * the end down, as the image does (0x0cb83b3e to 0x0cb83b46 in EV001E).
     * a byte cursor stays defined across points, where a Float * would step
     * out of its own. as a loop of point copies, the function runs 8 bytes
     * long, and 91 of 99 words differ */
    for (i = (count - first - segments) * 3; i != 0; --i) {
        *(Float *)(to -= sizeof(Float)) = *(Float *)(from -= sizeof(Float));
    }
    to = (unsigned char *)(points + (first + (segments * subdivisions)));
    from = (unsigned char *)(points + (first + segments)) + (2 * sizeof(Float));
    z = *(Float *)from;
    y = *(Float *)(from -= sizeof(Float));
    x = *(Float *)(from -= sizeof(Float));
    /* @bug memory: the inner loop reads a point for each new point,
     * where it means one for each segment (0x0cb83b88 to 0x0cb83b9a in
     * EV001E). only one segment cut in two comes out right. the reads run
     * below the keys once segments * (subdivisions - 1) passes first */
    for (i = segments; i != 0; --i) {
        for (j = subdivisions; j != 0; --j) {
            old_z = z;
            z = *(Float *)(from -= sizeof(Float));
            old_y = y;
            y = *(Float *)(from -= sizeof(Float));
            old_x = x;
            x = *(Float *)(from -= sizeof(Float));
            fraction = (Float)(j - 1) / subdivisions;
            *(Float *)(to -= sizeof(Float)) = old_z + (fraction * (z - old_z));
            *(Float *)(to -= sizeof(Float)) = old_y + (fraction * (y - old_y));
            *(Float *)(to -= sizeof(Float)) = old_x + (fraction * (x - old_x));
        }
    }
}

/* SAITOLINE3D: cut segments as path_editor_subdivide does, along
 * path_editor_curve. each new point is the curve's for the four points
 * about its segment. the cuts run down from the top segment. only points
 * move, and the caller raises its count */
static void path_editor_subdivide_curve(NJS_POINT3 *points, int count,
                                        int first, int segments,
                                        int subdivisions)
{
    NJS_POINT3 *to;
    NJS_POINT3 *from;
    int i;
    int j;
    unsigned char *tail_to =
        (unsigned char *)(points + (count + ((subdivisions - 1) * segments)));
    unsigned char *tail_from = (unsigned char *)(points + count);

    /* @hack spelling: byte cursors that move the points as
     * path_editor_subdivide does (0x0cb83c0c to 0x0cb83c14 in EV001E). as a
     * loop of point copies, the function runs 8 bytes long, and 82 of 83 words
     * differ */
    for (i = (count - first - segments) * 3; i != 0; --i) {
        *(Float *)(tail_to -= sizeof(Float)) =
            *(Float *)(tail_from -= sizeof(Float));
    }
    to = points + (first + (segments * subdivisions));
    from = points + (first + segments - 1);
    for (i = segments; i != 0; --i) {
        --from;
        for (j = subdivisions; j != 0; --j) {
            --to;
            /* @bug logic: the lowest segment's new point one part above key
             * first comes out off the curve. each call reads the four points
             * about its segment in place (JSR at 0x0cb83c5a in EV001E),
             * where earlier writes have replaced some. cut in two parts,
             * that segment reads key first + 1 for key first + 2. cut in
             * more, it reads its new point two parts above key first
             * instead */
            path_editor_curve(from, to, (Float)(j - 1) / subdivisions);
        }
    }
}

/* AVERAGE VEL: move all but the end keys along their path, to part it
 * into equal lengths. only points move. both buffers come from the heap,
 * unchecked */
static void path_editor_space(NJS_POINT3 *points, int count)
{
    Float *distances = heap_alloc(count * sizeof(*distances));
    NJS_POINT3 *resampled = heap_alloc(count * sizeof(*resampled));
    Float total = 0.0F;
    Float scale;
    Float fraction;
    Float denominator;
    Float previous_distance;
    int i;
    int segment;
    NJS_POINT3 *source;
    NJS_POINT3 *destination;

    for (i = 1; i < count; ++i) {
        total += njSqrt(square(points[i].x - points[i - 1].x) +
                        square(points[i].y - points[i - 1].y) +
                        square(points[i].z - points[i - 1].z));
        distances[i] = total;
    }
    scale = 1.0F / distances[count - 1];
    for (i = 1; i < count - 1; ++i) {
        distances[i] *= scale;
    }
    distances[0] = 0.0F;
    distances[count - 1] = 1.0F;
    segment = 1;
    denominator = count - 1;
    for (i = 1; i < count - 1; ++i) {
        fraction = (Float)i / denominator;
        while (fraction > distances[segment]) {
            ++segment;
        }
        previous_distance = distances[segment - 1];
        fraction = (fraction - previous_distance) /
                   (distances[segment] - previous_distance);
        resampled[i].x =
            points[segment - 1].x +
            ((points[segment].x - points[segment - 1].x) * fraction);
        resampled[i].y =
            points[segment - 1].y +
            ((points[segment].y - points[segment - 1].y) * fraction);
        resampled[i].z =
            points[segment - 1].z +
            ((points[segment].z - points[segment - 1].z) * fraction);
    }
    /* @hack spelling: the new points copied through two pointers as a count
     * runs down, as the image does (0x0cb83de8 to 0x0cb83dfa in EV001E). as
     * points[i] = resampled[i] for i from 1, the function runs 4 bytes long,
     * and 165 of 207 words differ */
    source = resampled + 1;
    destination = points + 1;
    for (i = count - 1; --i != 0;) {
        *destination++ = *source++;
    }
    heap_free(distances);
    heap_free(resampled);
}

/* copy the command of the first of count shortcuts whose button
 * per_info[0] pressed into command. return 1, or 0 when none was */
static int path_editor_shortcut(struct editor_shortcut *shortcuts, int count,
                                int *command)
{
    int i;
    int j;

    for (i = 0; i < count; ++i) {
        if (per_info[0]->press & shortcuts[i].buttons) {
            for (j = 0; j < CHOICES; ++j) {
                command[j] = shortcuts[i].command[j];
            }
            return 1;
        }
    }
    return 0;
}

void path_editor_run(const char *name)
{
    NJS_POINT3 *points = heap_alloc(KEYS * sizeof(*points));
    union key_angles *angles = heap_alloc(KEYS * sizeof(*angles));
    int count;
    int running = 1;
    int show_values = 1;
    Float speed = 1.0F;
    Float step;
    int state = STATE_NORMAL;
    int normal_choice = 0;
    int selected_choice = 0;
    int order = ORDER_ZXY;
    int menu_open = NO_MENU;
    int first;
    int last;
    int lower; /* the lower of first and last */
    int span; /* the segments between keys first and last */
    struct editor_menu_state menu;
    int commands[CHOICES];
    struct task_work *player = player_work[0];
    Float x = player->pos.x;
    Float y = player->pos.y + 10.0F;
    Float z = player->pos.z;
    Float dx = event_camera.pos.x - x;
    Float dy;
    Float dz;
    Float distance;
    Angle pitch;
    Angle yaw;

    /* @hack spelling: dy and dz each read, then differenced, so x, y and z
     * take FR15, FR14 and FR13, and dy FR12, as in the image (0x0cb83ef0 to
     * 0x0cb83f0e in EV001E). as one subtraction each, the registers and
     * frame slots move, and 84 of 1117 words differ */
    dy = event_camera.pos.y;
    dy = dy - y;
    dz = event_camera.pos.z;
    dz = dz - z;
    distance = njSqrt((dx * dx) + (dy * dy) + (dz * dz));
    pitch = -event_camera.ang.x;
    yaw = event_camera.ang.y;
    /* put the camera on its orbit about the cursor. it keeps its distance
     * from the cursor, its pitch and its yaw */
    {
        Float cp;
        Float sp;
        Float cy;
        Float sy;
        Float camera_x;
        Float camera_y;
        Float camera_z;

        cp = njCos(pitch);
        sp = njSin(pitch);
        cy = njCos(yaw);
        sy = njSin(yaw);
        camera_z = z + (distance * cp * cy);
        camera_x = x + (distance * cp * sy);
        camera_y = y + (distance * sp);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, camera_x, camera_y,
                             camera_z);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, -pitch, yaw, 0);
    }
    step = 1.0F;
    count = 0;
    do {
        thread_yield();
        /* with L held, left and right turn the camera about the cursor. up
         * and down tilt it, or with R held move it nearer and farther. with
         * keys selected and X held, left and right move the cursor along
         * them. otherwise, outside the menus, the pad moves the cursor in the
         * camera's frame. with R held, up and down move it vertically */
        if (per_info[0]->on & PDD_DGT_TL) {
            Float cp;
            Float sp;
            Float cy;
            Float sy;
            Float horizontal;
            Float camera_x;
            Float camera_y;
            Float camera_z;

            if (per_info[0]->on & PDD_DGT_KR) {
                yaw -= ANGLE_STEP;
            }
            if (per_info[0]->on & PDD_DGT_KL) {
                yaw += ANGLE_STEP;
            }
            if (per_info[0]->on & PDD_DGT_TR) {
                if (per_info[0]->on & PDD_DGT_KU) {
                    distance -= 1.0F;
                }
                if (per_info[0]->on & PDD_DGT_KD) {
                    distance += 1.0F;
                }
            } else {
                if (per_info[0]->on & PDD_DGT_KU) {
                    pitch += ANGLE_STEP;
                }
                if (per_info[0]->on & PDD_DGT_KD) {
                    pitch -= ANGLE_STEP;
                }
            }
            cp = njCos(pitch);
            sp = njSin(pitch);
            cy = njCos(yaw);
            sy = njSin(yaw);
            horizontal = distance * cp;
            camera_z = z + (horizontal * cy);
            camera_x = x + (horizontal * sy);
            camera_y = y + (distance * sp);
            event_camera_set_pos(EVENT_EASE_LINEAR, 0, camera_x, camera_y,
                                 camera_z);
            event_camera_set_ang(EVENT_EASE_LINEAR, 0, -pitch, yaw, 0);
        } else if (state == STATE_SELECTED && (per_info[0]->on & PDD_DGT_TX)) {
            Float cp;
            Float sp;
            Float cy;
            Float sy;
            Float horizontal;
            Float camera_x;
            Float camera_y;
            Float camera_z;

            if (per_info[0]->press & PDD_DGT_KL) {
                if (--last < 0) {
                    last = 0;
                }
            }
            if (per_info[0]->press & PDD_DGT_KR) {
                if (++last >= count) {
                    last = count - 1;
                }
            }
            x = points[last].x;
            y = points[last].y;
            z = points[last].z;
            cp = njCos(pitch);
            sp = njSin(pitch);
            cy = njCos(yaw);
            sy = njSin(yaw);
            horizontal = distance * cp;
            camera_z = z + (horizontal * cy);
            camera_x = x + (horizontal * sy);
            camera_y = y + (distance * sp);
            /* @hack redundant: a never-run read of step into camera_y, which
             * keeps step at sp+36 (0x0cb83f9e in EV001E); without it, step
             * moves to sp+48, lower, span + 1 and the cursor move's cy each
             * move down a word, name and count - 1 trade sp+72 and sp+68, dx
             * and dz trade sp+76 and sp+80, and 33 of 1117 words differ */
            if (0) {
                camera_y = step;
            }
            event_camera_set_pos(EVENT_EASE_LINEAR, 0, camera_x, camera_y,
                                 camera_z);
        } else if (menu_open == NO_MENU) {
            Float cp;
            Float sp;
            Float cy;
            Float sy;
            Float camera_x;
            Float camera_y;
            Float camera_z;

            dz = dy = dx = 0.0F;
            if (per_info[0]->on & PDD_DGT_KL) {
                dx = -1.0F;
            }
            if (per_info[0]->on & PDD_DGT_KR) {
                dx = step;
            }
            if (per_info[0]->on & PDD_DGT_TR) {
                if (per_info[0]->on & PDD_DGT_KU) {
                    dy = step;
                }
                if (per_info[0]->on & PDD_DGT_KD) {
                    dy = -1.0F;
                }
            } else {
                if (per_info[0]->on & PDD_DGT_KU) {
                    dz = -1.0F;
                }
                if (per_info[0]->on & PDD_DGT_KD) {
                    dz = step;
                }
            }
            cp = njCos(pitch);
            sp = njSin(pitch);
            cy = njCos(yaw);
            sy = njSin(yaw);
            z += (dz * cy) - (dx * sy);
            x += (dz * sy) + (dx * cy);
            y += dy;
            camera_z = z + (distance * cp * cy);
            camera_x = x + (distance * cp * sy);
            camera_y = y + (distance * sp);
            event_camera_set_pos(EVENT_EASE_LINEAR, 0, camera_x, camera_y,
                                 camera_z);
        }
        switch (state) {
        case STATE_NORMAL: {
            int command;
            int result;

            command = NORMAL_NONE;
            if (menu_open == NORMAL_MENU_OPEN) {
                result = path_editor_menu_run(&menu);
                switch (result) {
                case CHOICE_PENDING:
                    break;
                case CHOICE_MADE:
                    normal_choice = command = commands[0];
                    /* fall through */
                case CHOICE_CANCELLED:
                    menu_open = NO_MENU;
                    break;
                }
            } else if (path_editor_shortcut(
                           path_editor_normal_shortcuts,
                           LENGTH(path_editor_normal_shortcuts), commands)) {
                command = commands[0];
            }
            switch (command) {
            case NORMAL_ADD:
                path_editor_add(points, angles, count, x, y, z);
                ++count;
                break;
            case NORMAL_SELECT:
                if (count > 0) {
                    state = STATE_SELECTED;
                    if (commands[1] == SELECT_NEAREST) {
                        first = last =
                            path_editor_nearest(points, count, x, y, z);
                    } else {
                        first = 0;
                        last = count - 1;
                    }
                    x = points[last].x;
                    y = points[last].y;
                    z = points[last].z;
                }
                break;
            case NORMAL_ROT_EDIT:
                /* @bug memory: with no keys, heap_alloc(0) gives null, so
                 * path_editor_edit_angles reads key 0 through null, and A
                 * with up or down writes there (the call at 0x0cb843be in
                 * EV001E) */
                path_editor_rot_edit(points, angles, count, order);
                break;
            case NORMAL_ORDER:
                order = commands[1];
                break;
            case NORMAL_PREVIEW:
                /* @bug memory: with no keys, the preview's path has null
                 * points, and its one step reads key 0 through null (the
                 * call at 0x0cb843d6 in EV001E) */
                path_editor_play(points, angles, count, speed);
                break;
            case NORMAL_INFO:
                switch (commands[1]) {
                case INFO_SHOW:
                    show_values = 1;
                    break;
                case INFO_HIDE:
                    show_values = 0;
                    break;
                case INFO_TOGGLE:
                    show_values = !show_values;
                    break;
                }
                break;
            case NORMAL_FILE:
                switch (commands[1]) {
                case FILE_SAVE:
                    path_editor_save(points, angles, count, name, commands[2],
                                     x, y, z);
                    break;
                case FILE_LOAD:
                    result = path_editor_load(points, angles, name, commands[2],
                                              x, y, z);
                    if (result < 0) {
                        /* @bug mismatched-call: the format's %s has no
                         * argument. the call pushes the format, so %s reads the
                         * stack word above it, sp+0, where the image keeps
                         * &commands[1] (the format's push at 0x0cb8444e, and
                         * the read of sp+0 at 0x0cb8445c, in EV001E). the
                         * name shown is commands[1]'s bytes: LOAD's 1, then
                         * a 0, which ends it */
                        event_message_wait_printf(
                            60, "\t%s \202\252\212J\202\257\202\334\202\271"
                                "\202\361\202\305\202\265\202\275\201B");
                    } else {
                        count = result;
                    }
                    break;
                }
                break;
            /* @hack spelling: NORMAL_SPEED's case after NORMAL_FILE's, where
             * the image has its code (from 0x0cb8445c in EV001E), since SHC
             * lays the cases out in the source's order. between
             * NORMAL_PREVIEW's and NORMAL_INFO's, as the enum orders them,
             * 79 of 1117 words differ */
            case NORMAL_SPEED:
                speed = path_editor_speeds[commands[1]];
                break;
            case NORMAL_EXIT:
                if (commands[1] == CONFIRM_SURE) {
                    running = 0;
                }
                break;
            case NORMAL_MENU:
                menu_open = NORMAL_MENU_OPEN;
                menu.menu = &path_editor_normal_menu;
                menu.x = 1;
                menu.y = 2;
                menu.width = path_editor_menu_width(&path_editor_normal_menu);
                menu.child = NULL;
                menu.selection = commands;
                commands[0] = normal_choice;
                break;
            }
            break;
        }
        case STATE_SELECTED: {
            int command;
            int result;

            command = SELECTED_NONE;
            if (first < last) {
                lower = first;
                span = last - first;
            } else {
                lower = last;
                span = first - last;
            }
            if (menu_open == SELECTED_MENU_OPEN) {
                result = path_editor_menu_run(&menu);
                switch (result) {
                case CHOICE_MADE:
                    command = selected_choice = commands[0];
                    menu_open = NO_MENU;
                    break;
                case CHOICE_CANCELLED:
                    selected_choice = commands[0];
                    menu_open = NO_MENU;
                    break;
                case CHOICE_PENDING:
                    break;
                }
            } else if (path_editor_shortcut(
                           path_editor_selected_shortcuts,
                           LENGTH(path_editor_selected_shortcuts), commands)) {
                command = commands[0];
            }
            /* @hack redundant: a never-run assignment menu.x = 3, which
             * keeps this block's command at sp+132 and normal_choice at
             * sp+128. without it, the two trade. SHC loads both offsets from
             * the pool, so 0 of 1117 words differ, but 5 of the 213 words of
             * its pools, switch tables and pad do */
            if (0) {
                menu.x = 3;
            }
            switch (command) {
            case SELECTED_SELECT:
                switch (commands[1]) {
                case SELECTION_NONE:
                    state = STATE_NORMAL;
                    break;
                case SELECTION_ALL:
                    first = 0;
                    last = count - 1;
                    x = points[last].x;
                    y = points[last].y;
                    z = points[last].z;
                    break;
                }
                break;
            case SELECTED_DELETE:
                path_editor_delete(points, angles, count, first, last);
                count -= span + 1;
                state = STATE_NORMAL;
                break;
            case SELECTED_INTERPOLATE:
                switch (commands[1]) {
                case INTERPOLATE_LINEAR:
                    if (span > 0) {
                        int subdivisions = path_editor_divisions[commands[2]];

                        path_editor_subdivide(points, count, lower, span,
                                              subdivisions);
                        count += (subdivisions - 1) * span;
                        state = STATE_NORMAL;
                    }
                    break;
                case INTERPOLATE_CURVE:
                    /* the curve reads a key past each end of a segment.
                     * so the cut leaves the selection's two end segments as
                     * they are */
                    if (span > 2) {
                        int subdivisions = path_editor_divisions[commands[2]];

                        ++lower;
                        span -= 2;
                        path_editor_subdivide_curve(points, count, lower, span,
                                                    subdivisions);
                        count += (subdivisions - 1) * span;
                        state = STATE_NORMAL;
                    }
                    break;
                }
                break;
            case SELECTED_SPACE:
                if (span > 1) {
                    path_editor_space(points + lower, span + 1);
                }
                break;
            case SELECTED_ROT_EDIT:
                if (span > 0) {
                    path_editor_rot_edit(points + lower, angles + lower,
                                         span + 1, order);
                }
                break;
            case SELECTED_PREVIEW:
                if (span > 0) {
                    /* span keys: the selection less its last key */
                    path_editor_play(points + lower, angles + lower, span,
                                     speed);
                }
                break;
            case SELECTED_SPEED:
                speed = path_editor_speeds[commands[1]];
                break;
            case SELECTED_MENU:
                menu_open = SELECTED_MENU_OPEN;
                menu.menu = &path_editor_selected_menu;
                menu.x = 1;
                menu.y = 2;
                menu.width = path_editor_menu_width(&path_editor_selected_menu);
                menu.child = NULL;
                menu.selection = commands;
                commands[0] = selected_choice;
                break;
            }
            if (state != STATE_SELECTED) {
                first = last = 0;
            }
            break;
        }
        }
        if (show_values) {
            njPrintC(NJM_LOCATION(24, 2), "X  :");
            njPrintF(NJM_LOCATION(28, 2), x, 4);
            njPrintC(NJM_LOCATION(24, 3), "Y  :");
            njPrintF(NJM_LOCATION(28, 3), y, 4);
            njPrintC(NJM_LOCATION(24, 4), "Z  :");
            njPrintF(NJM_LOCATION(28, 4), z, 4);
            njPrintC(NJM_LOCATION(24, 5), "NUM:");
            njPrintD(NJM_LOCATION(28, 5), count, 4);
            njPrintC(NJM_LOCATION(24, 6), "ROT:");
            njPrintC(NJM_LOCATION(28, 6), path_editor_order_items[order].label);
            njPrintC(NJM_LOCATION(24, 7), "SPD:");
            njPrintF(NJM_LOCATION(28, 7), speed, 4);
        }
        switch (state) {
        case STATE_NORMAL:
            path_editor_cursor(x, y, z);
            path_editor_draw(points, count, -1, -1);
            break;
        case STATE_SELECTED:
            path_editor_selection_cursor(x, y, z);
            path_editor_move(points, first, last, x, y, z);
            path_editor_draw(points, count, first, last);
            if (show_values) {
                njPrintC(NJM_LOCATION(24, 8), "SEL:   -");
                njPrintD(NJM_LOCATION(28, 8), first, 3);
                njPrintD(NJM_LOCATION(32, 8), last, 3);
            }
            break;
        }
    } while (running);
    thread_yield();
    heap_free(points);
    heap_free(angles);
}
