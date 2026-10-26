/* the fog editor: a menu of three pages on the first pad. with X held, up
 * and down turn the page. the edit page draws one table's levels, the fog
 * color and the density, and runs the picked editor of the three. the file
 * page saves the tables to the development host and loads them back. on
 * the exit page, A leaves the menu */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdio.h>

#include "debug/editor.h"
#include "debug/fog_editor.h"
#include "debug/host.h"
#include "system/pad.h"

#define FOG_FILE "FOG_MR02.bin"

/* 1 when value / delta is within a tenth of 1 */
#define NEAR(value, delta)                                                     \
    ((value) / (delta) > 0.9F && (value) / (delta) < 1.1F)

enum fog_mode {
    FOG_OPEN = 2, /* opens the menu on its first page */
    FOG_RUN, /* runs fog_menu until it returns -1 */
    FOG_CLOSE
};

enum fog_page { FOG_PAGE_EDIT, FOG_PAGE_FILE, FOG_PAGE_EXIT };
#define FOG_PAGES 3

enum fog_editing { FOG_EDIT_LEVELS, FOG_EDIT_COLOR, FOG_EDIT_DENSITY };
#define FOG_EDITORS 3

/* fog_edit_color's channels, as bits of its mask */
#define FOG_BLUE 1
#define FOG_GREEN 2
#define FOG_RED 4

/* the steps between a held direction's first change or move and its
 * second. after that it repeats each step */
#define FOG_REPEAT_WAIT 8

/* the bit of a print count that blinks a digit. it flips every 16 calls, so
 * the digit is blank for 16 calls of every 32 */
#define BLINK_BIT 0x10

static int fog_menu(void);
static void fog_file_page(void);
static void fog_edit_page(void);
static void fog_edit_levels(void);
static void fog_edit_color(void);
static void fog_draw(void);
static void fog_edit_density(Float *density);
static void fog_print_value(const Float *value, int x, int y, int digit);
static void fog_set_box(Float x0, Float y0, Float x1, Float y1,
                        NJS_COLOR *colors);
static void fog_draw_frame(Float x0, Float y0, Float x1, Float y1,
                           NJS_COLOR *colors);
static void fog_fill_box(Float x0, Float y0, Float x1, Float y1,
                         NJS_COLOR *colors);
static void fog_blink_frame(void);

static int fog_page;
/* the table the edit page shows and the levels editor edits */
static int fog_table_index;
/* the picked editor: the one the edit page runs. A, with X held, picks the
 * next */
static enum fog_editing fog_editing;
static enum fog_mode fog_mode;
/* the frame around the picked editor's box, which fog_blink_frame makes
 * blink */
static NJS_COLOR fog_lit_frame[4];
/* fog_print_value calls, which time the density digit's blink */
static int fog_blink_count;
/* the box fog_set_box sets, for fog_draw_frame and fog_fill_box */
static NJS_POINT2 fog_box_points[4];
static NJS_COLOR fog_box_colors[4];

static Uint8 *fog_tables = NULL;
static int fog_table_count = 0;
static Uint32 *fog_color = NULL;
static Float *fog_density = NULL;
/* the entry of the table that the levels editor's cursor is on */
static int fog_cursor = 0;
/* fog_draw prints LINEAR while it is 0, and nothing writes it */
static int dat_8c1dab24 = 0;
/* the mark: the entry the cursor was on when Start went down. it follows
 * the cursor while fog_marking is 0 */
static int fog_mark = 0;
/* steps until a held direction moves the cursor again */
static int fog_move_wait = 0;
/* steps until a held direction changes a level or the color again */
static int fog_change_wait = 0;
/* 1 if Start was held, and A was not, when the levels editor last ran */
static int fog_marking = 0;
/* 1 once a held direction has changed a level or the color */
static int fog_change_repeat = 0;
/* 1 once a held direction has moved the cursor */
static int fog_move_repeat = 0;
/* nothing reads it, and only a store in fog_draw that never runs writes it */
static Uint32 dat_8c1dab40 = 0;
/* the place of fog_delta's digit, counted from the point: 1 the ones, -1
 * the tenths. it runs from -2 to 4, and is never 0 */
static int fog_digit = 1;
/* the density's change for each step that up or down is held */
static Float fog_delta = 1.0F;
/* the file page's question: -1 save, 1 load, or 0 for none */
static int fog_file_choice = 0;
/* the frame around the FOG: and TBL: box, and around each editor's box
 * but the picked one's */
static NJS_COLOR fog_frame[4] = {
#include "debug/fog_editor/fog_frame.inc"
};
static NJS_POINT2COL fog_box = {fog_box_points, fog_box_colors, NULL, 4};
static Angle fog_blink_angle = 0;

void fog_editor_init(void)
{
    fog_mode = FOG_OPEN;
}

int fog_editor_exec(void)
{
    switch (fog_mode) {
    case FOG_OPEN:
        fog_page = FOG_PAGE_EDIT;
        fog_mode = FOG_RUN;
        break;
    case FOG_RUN:
        if (fog_menu() == -1) {
            fog_mode = FOG_CLOSE;
        }
        break;
    case FOG_CLOSE:
        fog_mode = FOG_OPEN;
        return -1;
    }
    return 0;
}

void fog_editor_reset(void)
{
    fog_tables = NULL;
    fog_table_count = 0;
    fog_color = NULL;
    fog_density = NULL;
}

void fun_8c0a8d62(void)
{
}

void fog_editor_set_tables(Uint8 *tables, int count)
{
    fog_tables = tables;
    fog_table_count = count;
}

void fog_editor_set_color(Uint32 *color)
{
    fog_color = color;
}

void fog_editor_set_density(Float *density)
{
    fog_density = density;
}

void fun_8c0a8da0(NJS_FOG_TABLE table, int index)
{
    Uint8 *row;
    Float *value;
    int level;
    int i;

    if (fog_tables != NULL && index < fog_table_count && index >= 0) {
        value = table;
        row = &fog_tables[index * FOG_TABLE_ENTRIES];
        for (i = 0; i < FOG_TABLE_ENTRIES; i++) {
            level = *row++;
            *value++ = level * (1.0F / 255.0F);
        }
    }
}

void fog_editor_make_table(NJS_FOG_TABLE table, int index, Float scale)
{
    Uint8 *row;
    Float *value;
    int level;
    int i;

    if (fog_tables != NULL && index < fog_table_count && index >= 0) {
        if (scale < 0.0F) {
            scale = 0.0F;
        } else if (scale > 1.0F) {
            scale = 1.0F;
        }
        value = table;
        row = &fog_tables[index * FOG_TABLE_ENTRIES];
        for (i = 0; i < FOG_TABLE_ENTRIES; i++) {
            level = *row++;
            *value++ = level * scale * (1.0F / 255.0F);
        }
    }
}

static int fog_menu(void)
{
    if (per_info[0]->on & PDD_DGT_TX) {
        if (per_info[0]->press & PDD_DGT_KU) {
            fog_page--;
        } else if (per_info[0]->press & PDD_DGT_KD) {
            fog_page++;
        }
    }
    if (fog_page < 0) {
        fog_page = FOG_PAGES - 1;
    } else if (fog_page >= FOG_PAGES) {
        fog_page = 0;
    }
    switch (fog_page) {
    case FOG_PAGE_EDIT:
        fog_blink_frame();
        fog_edit_page();
        break;
    case FOG_PAGE_FILE:
        fog_file_page();
        break;
    case FOG_PAGE_EXIT:
        njPrintC(NJM_LOCATION(10, 20), "PRESS \"A\" TO EXIT");
        if (per_info[0]->press & PDD_DGT_TA) {
            return -1;
        }
        break;
    }
    editor_take_buttons(0, PDD_DGT_TL | PDD_DGT_TR | PDD_DGT_TX | PDD_DGT_TY |
                               PDD_DGT_ST | PDD_DGT_TA | PDD_DGT_TB);
    return 0;
}

/* with X held, left asks to save and right to load. the same direction
 * again saves or loads, and the other cancels */
static void fog_file_page(void)
{
    int dir;

    dir = 0;
    if (per_info[0]->on & PDD_DGT_TX) {
        if (per_info[0]->press & PDD_DGT_KL) {
            dir = -1;
        } else if (per_info[0]->press & PDD_DGT_KR) {
            dir = 1;
        }
    }
    if (fog_file_choice == 0) {
        njPrintC(NJM_LOCATION(10, 22), "SAVE < [FILE] > LOAD");
        if (dir < 0) {
            fog_file_choice--;
        } else if (dir > 0) {
            fog_file_choice++;
        }
    } else if (fog_file_choice < 0) {
        njPrintC(NJM_LOCATION(10, 22), "< [SAVE ?]  ");
        if (dir < 0) {
            host_save_file(FOG_FILE, fog_tables,
                           fog_table_count * FOG_TABLE_ENTRIES);
            fog_file_choice = 0;
        } else if (dir > 0) {
            fog_file_choice = 0;
        }
    } else {
        njPrintC(NJM_LOCATION(10, 22), "  [LOAD ?] >");
        if (dir < 0) {
            fog_file_choice = 0;
        } else if (dir > 0) {
            host_load_file(FOG_FILE, fog_tables);
            fog_file_choice = 0;
        }
    }
}

/* with X held, A picks the next editor, and left and right the table
 * before or after, wrapping. without X, the picked editor runs. the page
 * draws either way */
static void fog_edit_page(void)
{
    if (per_info[0]->on & PDD_DGT_TX) {
        if (per_info[0]->press & PDD_DGT_TA) {
            fog_editing++;
            if (fog_editing >= FOG_EDITORS) {
                fog_editing = FOG_EDIT_LEVELS;
            }
        } else {
            if (per_info[0]->press & PDD_DGT_KL) {
                fog_table_index--;
            } else if (per_info[0]->press & PDD_DGT_KR) {
                fog_table_index++;
            }
            if (fog_table_index < 0) {
                fog_table_index = fog_table_count - 1;
            } else if (fog_table_index >= fog_table_count) {
                fog_table_index = 0;
            }
        }
    } else {
        switch (fog_editing) {
        case FOG_EDIT_LEVELS:
            fog_edit_levels();
            break;
        case FOG_EDIT_COLOR:
            fog_edit_color();
            break;
        case FOG_EDIT_DENSITY:
            if (fog_density != NULL) {
                fog_edit_density(fog_density);
            }
            break;
        }
    }
    fog_draw();
}

/* edit the table at fog_table_index. with A held, up raises and down lowers
 * the level at the cursor, or every level while L is held. without A, left
 * and right move the cursor, which wraps. each goes by 1 while R is held,
 * and otherwise by 4. a held direction acts at once, again FOG_REPEAT_WAIT
 * steps later, and then every step. Start marks the cursor's entry. its
 * release sets the levels from the mark to the cursor on the straight line
 * between the levels at the two ends. holding A drops the mark */
static void fog_edit_levels(void)
{
    int marking;
    int all;
    Uint8 *row;
    int x;
    int y;
    int speed;
    int delta;
    int start;
    int end;
    int i;
    int value;
    int from;
    int to;
    Float ratio;
    Uint8 low;
    Uint8 high;

    /* @hack spelling: all set ahead of marking, which stores its 0 at sp+4
     * (0x8c0a912c) ahead of marking's load and store at sp+0 (0x8c0a9130,
     * 0x8c0a9132); set with x and y, 14 of 262 words differ */
    all = 0;
    marking = fog_marking;
    fog_marking = 0;
    row = fog_tables + (fog_table_index * FOG_TABLE_ENTRIES);
    x = 0;
    y = 0;
    if (pad_records[0].on & PDD_DGT_KL) {
        x = -1;
    } else if (pad_records[0].on & PDD_DGT_KR) {
        /* @hack redundant: +1 rather than 1, here and for y, with which SHC
         * keeps the 1 in R14, copies it to R5 (0x8c0a9126, 0x8c0a914c) and
         * sets x and y from R5 (0x8c0a9156, 0x8c0a9166); with 1 for either,
         * the code runs 32 bytes long and 255 of 262 words differ */
        x = +1;
    }
    if (pad_records[0].on & PDD_DGT_KU) {
        y = -1;
    } else if (pad_records[0].on & PDD_DGT_KD) {
        y = +1;
    }
    if (pad_records[0].on & PDD_DGT_TR) {
        speed = 1;
    } else {
        speed = 4;
    }
    if (pad_records[0].on & PDD_DGT_TL) {
        all = 1;
    }
    if (pad_records[0].on & PDD_DGT_TA) {
        delta = -y;
        if (delta < 0) {
            delta = -speed;
        } else if (delta > 0) {
            delta = speed;
        }
        if (delta != 0) {
            fog_change_wait--;
            if (fog_change_wait <= 0) {
                if (fog_change_repeat != 0) {
                    fog_change_wait = 1;
                } else {
                    fog_change_wait = FOG_REPEAT_WAIT;
                }
                fog_change_repeat = 1;
                if (all) {
                    start = 0;
                    end = FOG_TABLE_ENTRIES - 1;
                } else {
                    start = end = fog_cursor;
                }
                for (i = start; i <= end; i++) {
                    /* @hack spelling: the level read in a statement of its
                     * own, which gives the loop two pointers from row +
                     * start, R13 to store through and R1 to read with
                     * MOV.B @R1+ (0x8c0a9200, 0x8c0a9206); as row[i] +
                     * delta, 174 of 262 words differ and the code runs 32
                     * bytes long */
                    value = row[i];
                    value += delta;
                    if (value < 0) {
                        value = 0;
                    } else if (value > 255) {
                        value = 255;
                    }
                    row[i] = value;
                }
            }
        } else {
            fog_change_wait = 0;
            fog_change_repeat = 0;
        }
        marking = 0;
    } else {
        if (x < 0) {
            x = -speed;
        } else if (x > 0) {
            x = speed;
        }
        if (x != 0) {
            fog_move_wait--;
            if (fog_move_wait <= 0) {
                if (fog_move_repeat != 0) {
                    fog_move_wait = 1;
                } else {
                    fog_move_wait = FOG_REPEAT_WAIT;
                }
                fog_move_repeat = 1;
                /* back 4 from the last entry lands on a multiple of 4 */
                if (fog_cursor == FOG_TABLE_ENTRIES - 1 && x == -4) {
                    fog_cursor = FOG_TABLE_ENTRIES - 4;
                } else {
                    fog_cursor += x;
                }
            }
        } else {
            fog_move_wait = 0;
            fog_move_repeat = 0;
        }
        if (fog_cursor < 0) {
            fog_cursor = FOG_TABLE_ENTRIES - 1;
        } else if (fog_cursor > FOG_TABLE_ENTRIES - 1) {
            fog_cursor = 0;
        }
        if (pad_records[0].press & PDD_DGT_ST) {
            fog_mark = fog_cursor;
        }
        if (pad_records[0].on & PDD_DGT_ST) {
            fog_marking = 1;
        }
    }
    if (marking == 1 && fog_marking == 0 && fog_mark != fog_cursor) {
        if (fog_mark < fog_cursor) {
            from = fog_mark;
            to = fog_cursor;
        } else {
            from = fog_cursor;
            to = fog_mark;
        }
        low = row[from];
        high = row[to];
        for (i = from; i < to; i++) {
            ratio = (Float)(i - from) / (to - from);
            row[i] = low + (ratio * (high - low));
        }
    }
    if (fog_marking == 0) {
        fog_mark = fog_cursor;
    }
}

/* up raises and down lowers each channel whose button is held: Y red, B
 * green and A blue. each goes by 1 while R is held, and otherwise by 8. a
 * held direction repeats as in the levels editor, on the same
 * fog_change_wait and fog_change_repeat. a change sets the alpha to 0xff */
static void fog_edit_color(void)
{
    int dir;
    int mask;
    int speed;
    int delta;
    int red;
    int green;
    int blue;

    mask = 0;
    if (fog_color == NULL) {
        return;
    }
    dir = 0;
    if (pad_records[0].on & PDD_DGT_KU) {
        dir = -1;
    } else if (pad_records[0].on & PDD_DGT_KD) {
        dir = 1;
    }
    if (pad_records[0].on & PDD_DGT_TY) {
        mask = FOG_RED;
    }
    if (pad_records[0].on & PDD_DGT_TB) {
        mask |= FOG_GREEN;
    }
    if (pad_records[0].on & PDD_DGT_TA) {
        mask |= FOG_BLUE;
    }
    if (pad_records[0].on & PDD_DGT_TR) {
        speed = 1;
    } else {
        speed = 8;
    }
    if (mask == 0) {
        return;
    }
    delta = -dir;
    if (delta < 0) {
        delta = -speed;
    } else if (delta > 0) {
        delta = speed;
    }
    if (delta != 0) {
        fog_change_wait--;
        if (fog_change_wait <= 0) {
            if (fog_change_repeat != 0) {
                fog_change_wait = 1;
            } else {
                fog_change_wait = FOG_REPEAT_WAIT;
            }
            fog_change_repeat = 1;
            red = (*fog_color >> 16) & 0xff;
            green = (*fog_color >> 8) & 0xff;
            blue = *fog_color & 0xff;
            if (mask & FOG_RED) {
                red += delta;
                if (red < 0) {
                    red = 0;
                } else if (red > 255) {
                    red = 255;
                }
            }
            if (mask & FOG_GREEN) {
                green += delta;
                if (green < 0) {
                    green = 0;
                } else if (green > 255) {
                    green = 255;
                }
            }
            if (mask & FOG_BLUE) {
                blue += delta;
                if (blue < 0) {
                    blue = 0;
                } else if (blue > 255) {
                    blue = 255;
                }
            }
            *fog_color = (red << 16) | (green << 8) | blue | 0xff000000;
        }
    } else {
        fog_change_wait = 0;
        fog_change_repeat = 0;
    }
}

/* draw the boxes of the levels, the color and the density, framing the
 * picked editor's in fog_lit_frame. the levels show as a graph of lines
 * under a strip of grays, with the marked span behind them. fog_draw turns
 * fog off to draw, and on after, whether or not it was on before. without
 * tables, it draws nothing */
static void fog_draw(void)
{
    NJS_COLOR col[4];
    NJS_POINT2COL poly;
    NJS_COLOR *frame;
    Uint8 *row;
    int value;
    int i;
    Uint32 color;

    if (fog_tables == NULL) {
        return;
    }
    njFogDisable();
    njPrintColor(0xffbfbfbf);
    fog_draw_frame(13.0F, 349.0F, 115.0F, 435.0F, fog_frame);
    col[0].color = 0xff000000;
    col[1].color = 0xff303030;
    col[2].color = 0xff000000;
    col[3].color = 0xff303030;
    /* @hack redundant: a call that never runs, whose use of &col[3] puts the
     * addresses of col[0] to col[3] at sp+28, sp+24, sp+32 and sp+20
     * (0x8c0a957a to 0x8c0a959c); without it, they take other slots and 32
     * of 889 words differ */
    if (0) {
        fog_editor_set_color(&col[3].color);
    }
    fog_fill_box(14.0F, 350.0F, 114.0F, 434.0F, col);
    if (fog_marking != 0) {
        njPrintH(NJM_LOCATION(2, 21), fog_mark, 2);
        njPrintC(NJM_LOCATION(4, 21), ">");
        njPrintH(NJM_LOCATION(5, 21), fog_cursor, 2);
    }
    value = fog_tables[(fog_table_index * FOG_TABLE_ENTRIES) + fog_cursor];
    njPrintC(NJM_LOCATION(1, 23), "FOG:");
    njPrintH(NJM_LOCATION(5, 23), value, 2);
    njPrintC(NJM_LOCATION(1, 26), "TBL:");
    njPrintH(NJM_LOCATION(5, 26), fog_table_index, 2);
    switch (dat_8c1dab24) {
    case 0:
        njPrintC(NJM_LOCATION(1, 22), "LINEAR");
    }
    fog_draw_frame(125.0F, 349.0F, 387.0F, 435.0F,
                   fog_editing == FOG_EDIT_LEVELS ? fog_lit_frame : fog_frame);
    col[0].color = 0xff101010;
    col[1].color = 0xff101010;
    col[2].color = 0xff404040;
    col[3].color = 0xff404040;
    fog_fill_box(126.0F, 350.0F, 386.0F, 434.0F, col);
    col[2].color = 0xff80ff80;
    col[3].color = 0xff80ff80;
    col[0].color = 0xff008000;
    col[1].color = 0xff008000;
    fog_fill_box((fog_mark * 2) + 128.0F, 432.0F, (fog_cursor * 2) + 129.0F,
                 368.0F, col);
    {
        NJS_POINT2 point[2];
        NJS_COLOR line_col[2];
        Uint8 prev;
        Uint8 next;
        int right_x;

        row = fog_tables + (fog_table_index * FOG_TABLE_ENTRIES);
        poly.p = point;
        poly.col = line_col;
        /* @hack type: the null texture as a plain 0, without NULL's cast,
         * which keeps &col[0] at sp+28 and &col[2] at sp+32 (0x8c0a957a,
         * 0x8c0a9592); with NULL, the two swap places and 16 of 889 words
         * differ */
        poly.tex = 0;
        poly.num = 2;
        prev = *row;
        line_col[0].color = 0xffc0c0c0;
        /* @hack redundant: a store of the line color that never runs, which
         * keeps fog_tables' address in R11 and i in R9 (0x8c0a9544,
         * 0x8c0a979e); without it, 552 of 889 words differ */
        if (0) {
            line_col[1].color = 0xffc0c0c0;
        }
        /* @hack redundant: a store of the same color to fog_change_wait that
         * never runs, which keeps &col[0] at sp+28 and &col[2] at sp+32
         * (0x8c0a957a, 0x8c0a9592); without it, the two swap places and 35 of
         * 889 words differ */
        if (0) {
            fog_change_wait = 0xffc0c0c0;
        }
        line_col[1].color = 0xffc0c0c0;
        /* @hack redundant: a test of row that never runs, which keeps
         * fog_tables' address in R11 (0x8c0a9544), &point in R6 and
         * &line_col in R5 (0x8c0a9766, 0x8c0a976a); without it, 652 of 889
         * words differ and the code runs 32 bytes long */
        if (0) {
            if (row) {
            }
        }
        for (i = 1, right_x = 2; i < FOG_TABLE_ENTRIES; i++) {
            next = row[i];
            point[0].x = ((i - 1) * 2) + 128.0F;
            point[0].y = 432.0F - (prev >> 2);
            point[1].x = right_x + 128.0F;
            point[1].y = 432.0F - (next >> 2);
            njDrawLine2D(&poly, 1, -0.5F, NJD_DRAW_NORMAL);
            prev = next;
            /* @hack spelling: the step spelled right_x = right_x + 2, which
             * keeps (i - 1) * 2 in R13 and right_x in R14 (0x8c0a9794,
             * 0x8c0a9798); with +=, the two swap places and 377 of 889
             * words differ */
            right_x = right_x + 2;
        }
        /* @hack redundant: a test of right_x that never runs, which keeps
         * (i - 1) * 2 in R13 and right_x in R14 (0x8c0a9794, 0x8c0a9798);
         * without it, the two swap places and 369 of 889 words differ */
        if (0) {
            if (right_x) {
            }
        }
    }
    if (fog_marking != 0) {
        col[1].color = 0xff808080;
        col[0].color = 0xff202020;
        col[3].color = 0xffc0c0c0;
        col[2].color = 0xff404040;
        fog_draw_frame(13.0F, 333.0F, 115.0F, 352.0F, col);
        col[0].color = 0xff000000;
        col[1].color = 0xff303030;
        col[2].color = 0xff000000;
        col[3].color = 0xff303030;
        fog_fill_box(14.0F, 334.0F, 114.0F, 352.0F, col);
    }
    {
        NJS_POINT2 quad[4];
        NJS_COLOR quad_col[4];
        Uint32 level;
        Uint32 gray;
        Float x;

        row = fog_tables + (fog_table_index * FOG_TABLE_ENTRIES);
        poly.p = quad;
        poly.col = quad_col;
        /* @hack type: 0 again, which keeps &col[1] at sp+24 and &col[2] at
         * sp+32 (0x8c0a9586, 0x8c0a9592) and fog_table_index's address in R3
         * (0x8c0a95da); with NULL, 408 of 889 words differ, and with NULL at
         * both, 35 */
        poly.tex = 0;
        poly.num = 4;
        level = *row;
        gray = 0xff00;
        /* @hack spelling: the first or spelled gray = gray | ..., which keeps
         * the graph's prev in R8 and i in R9 (0x8c0a9796, 0x8c0a979e); with
         * |=, the two swap places and 36 of 889 words differ */
        gray = gray | level * 255 >> 8;
        gray <<= 8;
        gray |= level * 255 >> 8;
        gray <<= 8;
        gray |= level * 255 >> 8;
        quad[0].x = 128.0F;
        quad[0].y = 366.0F;
        quad_col[0].color = gray;
        quad[1].x = 128.0F;
        quad[1].y = 354.0F;
        quad_col[1].color = gray;
        /* @hack spelling: i set ahead of the calls below and of the loop, which
         * keeps &quad in R7 and &quad_col[3] in R2 (0x8c0a9908, 0x8c0a9986);
         * set in the for, 11 of 889 words differ */
        i = 1;
        /* @hack redundant: two calls that never run, each a use of &quad[0],
         * which keep 366.0 in FR14 and 354.0 in FR13 (0x8c0a995e,
         * 0x8c0a9974) and, together, &quad in R7 and 0xff00 in R9 (0x8c0a9908,
         * 0x8c0a9912); without one, 33 of 889 words differ, and without
         * both, 108 */
        if (0) {
            fog_editor_set_color((Uint32 *)&quad[0]);
        }
        /* @hack redundant: the same call again, a second use of &quad[0] for
         * the registers above; without it, 33 of 889 words differ */
        if (0) {
            fog_editor_set_color((Uint32 *)&quad[0]);
        }
        for (; i < FOG_TABLE_ENTRIES; i++) {
            /* level = *row before the loop doesn't step row, so pass i reads
             * entry i - 1. the quad's right edge, at x = 128 + i * 2, takes
             * that entry's gray, and the last entry's gray never shows */
            level = *row++;
            gray = 0xff00;
            /* @hack spelling: the first or spelled out here too, which has R8
             * hold &quad_col[2] (0x8c0a9958, 0x8c0a9994); with |=, R8 holds
             * &quad_col and 346 of 889 words differ */
            gray = gray | level * 255 >> 8;
            gray <<= 8;
            gray |= level * 255 >> 8;
            gray <<= 8;
            gray |= level * 255 >> 8;
            x = (i * 2) + 128.0F;
            quad[2].x = x;
            quad[2].y = 366.0F;
            quad_col[2].color = gray;
            quad[3].x = x;
            quad[3].y = 354.0F;
            quad_col[3].color = gray;
            njDrawPolygon2D(&poly, 4, -1.0F, NJD_FILL);
            quad[0].x = quad[2].x;
            quad[0].y = quad[2].y;
            quad_col[0].color = quad_col[2].color;
            quad[1].x = quad[3].x;
            quad[1].y = quad[3].y;
            quad_col[1].color = quad_col[3].color;
        }
    }
    color = *fog_color;
    njPrintColor(0xffbf0000);
    njPrintC(NJM_LOCATION(26, 22), "R:");
    njPrintColor(0xff00bf00);
    njPrintC(NJM_LOCATION(26, 23), "G:");
    njPrintColor(0xff0000bf);
    njPrintC(NJM_LOCATION(26, 24), "B:");
    njPrintColor(0xffbfbfbf);
    njPrintH(NJM_LOCATION(28, 22), (color >> 16) & 0xff, 2);
    njPrintH(NJM_LOCATION(28, 23), (color >> 8) & 0xff, 2);
    njPrintH(NJM_LOCATION(28, 24), color & 0xff, 2);
    frame = fog_editing == FOG_EDIT_COLOR ? fog_lit_frame : fog_frame;
    fog_draw_frame(413.0F, 349.0F, 547.0F, 403.0F, frame);
    col[0].color = 0xff000000;
    col[1].color = 0xff000000;
    col[2].color = 0xff000000;
    col[3].color = 0xff000000;
    fog_fill_box(414.0F, 350.0F, 546.0F, 402.0F, col);
    col[0].color = color;
    col[1].color = color;
    col[2].color = color;
    col[3].color = color;
    fog_fill_box(488.0F, 360.0F, 536.0F, 392.0F, col);
    njPrintColor(0xffbfbfbf);
    njPrintC(NJM_LOCATION(26, 26), "DEN:");
    if (fog_editing == FOG_EDIT_DENSITY) {
        fog_print_value(fog_density, 30, 26, fog_digit);
        frame = fog_lit_frame;
    } else {
        fog_print_value(fog_density, 30, 26, 0);
        frame = fog_frame;
        /* @hack redundant: a test of njPrintC's address that never runs,
         * which keeps &col[0] at sp+28 and &col[2] at sp+32 (0x8c0a957a,
         * 0x8c0a9592); without it, the two swap places and 28 of 889 words
         * differ */
        if (0) {
            if (&njPrintC) {
            }
        }
        /* @hack redundant: two stores of the print color that never run,
         * which keep &col[0], &col[1] and &col[2] at sp+28, sp+24 and sp+32
         * (0x8c0a957a, 0x8c0a9586, 0x8c0a9592); without one, 24 of 889 words
         * differ, and without both, 28 */
        if (0) {
            fog_move_repeat = 0xffbfbfbf;
        }
        /* @hack redundant: the second store of the print color, to
         * dat_8c1dab40, for the slots above; without it, 24 of 889 words
         * differ */
        if (0) {
            dat_8c1dab40 = 0xffbfbfbf;
        }
        /* @hack redundant: a step of frame that never runs, which keeps
         * fog_tables' address in R11 and the graph's prev in R8 (0x8c0a9544,
         * 0x8c0a9796); without it, the two swap places and 362 of 889 words
         * differ */
        if (0) {
            frame = frame + 1;
        }
        /* @hack redundant: a store of 413.0 that never runs, which keeps
         * fog_table_index's address in R3 (0x8c0a95da); without it, the
         * address takes R9 and 525 of 889 words differ */
        if (0) {
            fog_delta = 413.0F;
        }
    }
    fog_draw_frame(413.0F, 413.0F, 595.0F, 435.0F, frame);
    col[0].color = 0xff000000;
    col[1].color = 0xff303030;
    col[2].color = 0xff000000;
    col[3].color = 0xff303030;
    fog_fill_box(414.0F, 414.0F, 594.0F, 434.0F, col);
    njPrintColor(0xffffffff);
    njFogEnable();
}

/* right divides fog_delta by 10, down to 0.01, and left multiplies it by
 * 10, up to 1000. fog_digit follows its place. while up is held, each step
 * adds fog_delta to the density, and while down is held, takes it away */
static void fog_edit_density(Float *density)
{
    if (pad_records[0].press & PDD_DGT_KR) {
        if (fog_delta >= 0.1F) {
            fog_digit--;
            if (fog_digit == 0) {
                fog_digit = -1;
            }
            if (NEAR(0.1F, fog_delta)) {
                fog_delta = 0.01F;
            } else if (NEAR(1.0F, fog_delta)) {
                fog_delta = 0.1F;
            } else if (NEAR(10.0F, fog_delta)) {
                fog_delta = 1.0F;
            } else if (NEAR(100.0F, fog_delta)) {
                fog_delta = 10.0F;
            } else if (NEAR(1000.0F, fog_delta)) {
                fog_delta = 100.0F;
            }
        }
    }
    if (pad_records[0].press & PDD_DGT_KL) {
        if (fog_delta <= 101.0F) {
            fog_digit++;
            if (fog_digit == 0) {
                fog_digit = 1;
            }
            if (NEAR(0.01F, fog_delta)) {
                fog_delta = 0.1F;
            } else if (NEAR(0.1F, fog_delta)) {
                fog_delta = 1.0F;
            } else if (NEAR(1.0F, fog_delta)) {
                fog_delta = 10.0F;
            } else if (NEAR(10.0F, fog_delta)) {
                fog_delta = 100.0F;
            } else if (100.0F / fog_delta > 0.0F && 100.0F / fog_delta < 1.1F) {
                /* NEAR(100.0F, fog_delta), but with a lower bound of 0, not
                 * 0.9 */
                fog_delta = 1000.0F;
            }
        }
    }
    if (pad_records[0].on & PDD_DGT_KU) {
        *density += fog_delta;
    } else if (pad_records[0].on & PDD_DGT_KD) {
        *density -= fog_delta;
    }
}

/* print *value at x, y as %04.1f. unless digit is 0, the text's character
 * at index 5 - digit blinks by BLINK_BIT of fog_blink_count. digit counts
 * places as fog_digit does, but the blank lands on that place only where
 * five characters stand before the point, as in -2400.0 */
static void fog_print_value(const Float *value, int x, int y, int digit)
{
    char buf[20];

    fog_blink_count++;
    sprintf(buf, "%04.1f", *value);
    if (digit != 0) {
        if (fog_blink_count & BLINK_BIT) {
            /* @bug uninitialized: where the index 5 - digit equals the
             * string's length, the blank (0x8c0aa090) overwrites the
             * terminator, as digit -2 does for -2400.0 and digit 1 for 01.0.
             * njPrintC (0x8c0aa09c) then reads on, through bytes of buf
             * sprintf never wrote, to the first 0 */
            /* @hack spelling: the blank stored through buf + 5 - digit, which
             * takes digit from buf's address and stores at a displacement of
             * 5 (0x8c0aa08e, 0x8c0aa090); as buf[5 - digit], 4 of 46 words
             * differ */
            *(buf + 5 - digit) = ' ';
        }
    }
    njPrintC(NJM_LOCATION(x, y), buf);
}

/* set fog_box to the corners x0 y0, x1 y0, x1 y1 and x0 y1, colored
 * colors[0] to colors[3] */
static void fog_set_box(Float x0, Float y0, Float x1, Float y1,
                        NJS_COLOR *colors)
{
    NJS_COLOR *color;
    NJS_POINT2 *point;

    point = fog_box_points;
    color = fog_box_colors;
    point[0].x = x0;
    point[0].y = y0;
    color[0] = colors[0];
    point[1].x = x1;
    point[1].y = y0;
    color[1] = colors[1];
    point[2].x = x1;
    point[2].y = y1;
    color[2] = colors[2];
    point[3].x = x0;
    point[3].y = y1;
    color[3] = colors[3];
}

static void fog_draw_frame(Float x0, Float y0, Float x1, Float y1,
                           NJS_COLOR *colors)
{
    fog_set_box(x0, y0, x1, y1, colors);
    njDrawPolygon2D(&fog_box, 4, -1.0F, NJD_DRAW_NORMAL);
}

static void fog_fill_box(Float x0, Float y0, Float x1, Float y1,
                         NJS_COLOR *colors)
{
    fog_set_box(x0, y0, x1, y1, colors);
    njDrawPolygon2D(&fog_box, 4, -1.0F, NJD_FILL);
}

static void fog_blink_frame(void)
{
    int green;
    int blue;

    green = 0xc0 + (int)(njSin(fog_blink_angle) * 64.0F);
    /* @hack spelling: blue set after the first call, which keeps 0x80 in R13
     * (0x8c0aa17a) and loads 0xc0 from the pool for each green (0x8c0aa172,
     * 0x8c0aa1a0); with 0x80 written in, 41 of 56 words differ, and with
     * blue set first, 50 */
    blue = 0x80;
    fog_lit_frame[0].color = fog_lit_frame[2].color =
        green << 8 | blue | 0xffff0000;
    /* the color from the cosine below replaces the sine's above, so the
     * sine's never shows. nothing sets fog_lit_frame[1] and [3], which stay
     * 0, a transparent black */
    green = 0xc0 + (int)(njCos(fog_blink_angle) * 64.0F);
    fog_lit_frame[0].color = fog_lit_frame[2].color =
        green << 8 | blue | 0xffff0000;
    fog_blink_angle += 0x400;
    fog_blink_angle &= 0xffff;
}
