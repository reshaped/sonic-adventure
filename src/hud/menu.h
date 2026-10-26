/* menus: the debug menu titled <SELECT PROGRAM>, and the menu task that
 * grows a menu open, moves its cursor and shrinks it closed */
#ifndef HUD_MENU_H
#define HUD_MENU_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* left and right move a row's cursor, and up and down a column's. the kind
 * also sets the textures and colors of the cursor and the items */
enum menu_kind { MENU_ROW, MENU_COLUMN };

/* the sounds a menu plays, as sound_play numbers them */
struct menu_sounds {
    Sint32 open;
    Sint32 close;
    Sint32 move;
    Sint32 select;
    Sint32 cancel;
};

struct menu_item {
    Float x, y; /* its middle's offset from the menu's, at full size */
    Uint8 texture; /* its label, an index into the menu's texlist */
};

/* what the menu passes its draw callback, valid only during the call. it
 * holds the menu's place, its scale as it grows and shrinks, and the cursor */
struct menu_view {
    Float x, y, z;
    Float scale_x, scale_y;
    Sint8 cursor;
};

struct menu {
    enum menu_kind kind;
    /* called after the cursor is drawn, or NULL */
    void (*draw)(struct menu_view *view);
    NJS_TEXLIST *texlist; /* the items' labels */
    const struct menu_item *items;
    const struct menu_sounds *sounds; /* or NULL for silence */
    Uint32 frame_top; /* the frame's color at its top, 0xAARRGGBB */
    Uint32 frame_bottom; /* and at its bottom */
    Float x, y, z; /* the menu's middle, and its depth */
    Float frame_w, frame_h; /* the frame's size in pixels, at full size */
    /* an item plate's scale at the menu's full size. the cursor is a fifth
     * larger */
    Float item_scale_x, item_scale_y;
    Sint8 count;
    /* the item B and X choose. choosing it by any button plays
     * sounds->cancel, not sounds->select */
    Sint8 cancel;
};

/* the sounds every menu in the game plays */
extern const struct menu_sounds menu_default_sounds;
/* the textures of each kind's cursor and item plates, which the modules
 * load */
extern NJS_TEXLIST menu_row_texlist;
extern NJS_TEXLIST menu_column_texlist;

/* ready the debug menu and each program on it, and the light and fog
 * editors */
void program_select_init(void);
/* the debug menu's step. while X is held, it shows the menu and turns
 * pausing off, and at each step without X it turns pausing back on. up and
 * down move the cursor, wrapping, and start runs the program under it. the
 * program then steps in the menu's place until its exec returns -1.
 * nothing calls this: no image holds its address */
void program_select_step(void);

/* step a cursor. blink turns 0x2d8, a whole blink in 90 steps, near
 * enough. pulse falls by 0.05 while it is above 1, then is 1. alpha follows
 * pulse, from 0 at 1.5 to 255 at 1. the falls from 1.5 end one step below
 * 1, where alpha passes 255 and keeps only its low byte */
void menu_cursor_step(Uint8 *alpha, Angle *blink, Float *pulse);
/* blend two 0xAARRGGBB colors a channel at a time: part 0 gives from, and
 * 1 gives to */
Uint32 menu_blend_color(Uint32 from, Uint32 to, Float part);
/* draw a row's or a column's cursor with its middle at x and y, at its
 * texture's size times scale_x and scale_y. only alpha's low byte counts.
 * menu_draw_column_cursor has an old-style definition, so pass each
 * argument but alpha as a Float */
void menu_draw_row_cursor(int alpha, Float x, Float y, Float z, Float scale_x,
                          Float scale_y);
/* NOLINTNEXTLINE(*-strict-prototypes,*-deprecated-non-prototype) */
void menu_draw_column_cursor();
/* draw a row's or a column's item plate in color, with its middle at x and
 * y, at its texture's size times scale_x and scale_y */
void menu_draw_row_item(Uint32 color, Float x, Float y, Float z, Float scale_x,
                        Float scale_y);
void menu_draw_column_item(Uint32 color, Float x, Float y, Float z,
                           Float scale_x, Float scale_y);
/* draw a box of screen_box_texlist width by height pixels, with its middle
 * at x and y */
void menu_draw_box(Float x, Float y, Float z, Float width, Float height);
/* create the menu task, which the other menu calls act on, or return NULL
 * when task_create fails. heap_alloc's result for the work goes unchecked.
 * create one at a time: a second takes the other calls over, and the first
 * runs on */
struct task *menu_create(void);
/* kill the menu task, which destroys itself on its next step. the other
 * menu calls still use it, so call menu_create again before any of them */
void menu_kill(void);
/* show menu at once, waiting for a choice, with the cursor on the first
 * item from cursor on that isn't disabled, wrapping past the last. count
 * must be 1 or more, and cursor and cancel must be items. disabled lists at
 * most 15 items to skip, ended by a negative, or is NULL for none. the task
 * copies the list, and a longer one runs past the copy. the task reads menu
 * and what it points to until another menu replaces it or the task is
 * destroyed */
void menu_show(const struct menu *menu, Sint8 cursor, Sint8 *disabled);
/* show menu as menu_show does, but grow it open with the open sound before
 * it waits for a choice */
void menu_popup(const struct menu *menu, Sint8 cursor, Sint8 *disabled);
/* menu_popup with the cursor on item 0, and no item disabled */
void menu_popup_default(const struct menu *menu);
/* the item chosen once the menu has closed, or -1 */
int menu_result(void);
/* choose the item under the cursor of a menu waiting for a choice, without
 * the select sound. the next step plays the close sound, and the menu
 * shrinks closed from the step after. it returns FALSE, doing nothing,
 * before any menu has shown and while one grows open. it returns TRUE
 * otherwise, and leaves a menu already chosen as it is */
Bool menu_close(void);
/* the steps a menu has waited for a choice since it showed or its cursor
 * last moved, up to 0x7fff. menu_set_idle sets the count */
Sint16 menu_get_idle(void);
void menu_set_idle(Sint16 idle);

#endif
