/* the set editor. this header also declares the light editor and
 * editor_take_buttons, which save/card.c defines */
#ifndef DEBUG_EDITOR_H
#define DEBUG_EDITOR_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the console rows where the set editor prints the cursor's angles and
 * scale. EDITOR_LABEL is the place right of a row's value, where an object
 * labels what that value means to it. some objects put their label a
 * column further right, at EDITOR_LABEL_RIGHT, and a number of their own
 * at EDITOR_NUMBER */
#define EDITOR_ROW_ANG_X 16
#define EDITOR_ROW_ANG_Y 17
#define EDITOR_ROW_ANG_Z 18
#define EDITOR_ROW_SCL_X 19
#define EDITOR_ROW_SCL_Y 20
#define EDITOR_ROW_SCL_Z 21
#define EDITOR_LABEL(row) NJM_LOCATION(17, (row))
#define EDITOR_LABEL_RIGHT(row) NJM_LOCATION(18, (row))
#define EDITOR_NUMBER(row) NJM_LOCATION(26, (row))

/* 1 while the set editor is open */
extern Sint16 edit_mode;
/* the kind of object the set editor places, by its index in the act's
 * object list */
extern Sint16 editor_kind;
/* the set editor task's work, whose pos, ang and scl are the cursor. null
 * until the task first steps. the task's death clears neither it nor
 * edit_mode */
extern struct task_work *editor_work;
/* a blue box ten across, drawn as a marker */
extern NJS_OBJECT marker_box_object;

/* TRUE while the set editor holds the task */
Bool placement_editing(struct task *task);
/* create the set editor's task, which stays closed until
 * editor_request_open */
void editor_create(void);
/* ask the set editor to open on its task's next step, if it's closed.
 * return -1 while editor_work is null or the editor is closing, and 0
 * otherwise */
int editor_request_open(void);

/* the light editor's modes, by the headings they show */
enum light_editor_mode {
    EDIT_OFF = 0,
    EDIT_PALETTE = 1,
    EDIT_ENV = 2, /* the act's light */
    EDIT_VMS_TO_PC = 3, /* the card's files to the host */
    EDIT_PC_TO_VMS = 4, /* a file of the host's to the card */
    EDIT_FILE = 5, /* the act's sources, to the card and back */
    EDIT_FREE = 7, /* a new source, at the player */
    EDIT_SLAVE = 8
};

/* the light editor's mode, EDIT_OFF while it's closed */
extern enum light_editor_mode light_editor_mode;
/* in EDIT_FREE, draw a ball for the new source at the player and for each
 * source within 800 of the player, each scaled to its range */
void light_editor_draw(void);
/* hide buttons from the game until the pads are next read. they clear from
 * pad_records[port]'s on, old and press, and from per_info[port]'s press.
 * per_info[port]'s on keeps them. the main stick and the triggers read 0
 * in both records. port must be below PAD_PORTS */
void editor_take_buttons(int port, Uint32 buttons);
/* load the act's light sources and palettes, and turn the editor off */
void light_editor_init(void);
/* run a step of the light editor and return 0. while the editor is off
 * and A is held, return -1 without one */
int light_editor_exec(void);

#endif
