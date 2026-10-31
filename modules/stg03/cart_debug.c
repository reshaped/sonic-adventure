/* the cart's debug pages, and the motions and settings two of them edit.
 * one page draws the act's routes, and one edits and plays a camera path */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "camera/camera.h"
#include "camera/cart.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "hud/hud.h"
#include "path/curve.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg03/cart.h"
#include "stg03/cart_debug.h"
#include "stg03/cart_routes.h"
#include "stg03/shareobj_textures.h"
#include "system/bitmap.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 17
#define TEXT_2_LENGTH 17
#define TEXT_3_LENGTH 13
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 15
#define TEXT_7_LENGTH 17
#define TEXT_8_LENGTH 17
#define TEXT_9_LENGTH 11
#define TEXT_10_LENGTH 11
#define TEXT_11_LENGTH 11
#define TEXT_12_LENGTH 15
#define TEXT_13_LENGTH 23
#define TEXT_14_LENGTH 15
#define TEXT_15_LENGTH 33
#define TEXT_16_LENGTH 27
#define TEXT_17_LENGTH 35
#define TEXT_18_LENGTH 27
#define TEXT_19_LENGTH 25
#define TEXT_20_LENGTH 25
#define TEXT_21_LENGTH 31
#define TEXT_22_LENGTH 35
#define TEXT_23_LENGTH 31
#define TEXT_24_LENGTH 29
#include "stg03/cart_debug/text.inc"

/* the rows of the motion page and of the settings page */
#define MOTION_ROWS 15
#define SETTINGS_ROWS 9

/* the colors the pages print in */
#define COLOR_WHITE 0xffffffff
#define COLOR_YELLOW 0xffffff00
#define COLOR_ORANGE 0xffffcc44
#define COLOR_BLUE 0xff0000ff
#define COLOR_PALE_BLUE 0xff44ccff

/* the least and the most that right and left scale the amount to */
#define AMOUNT_LEAST 0.0001F
#define AMOUNT_MOST 100000.0F

/* a hint shows itself on the HINT_CALLS-th call after its row changes */
#define HINT_CALLS 60
/* Start moves the page on once more than PAGE_CALLS calls have passed since
 * it last did */
#define PAGE_CALLS 5

/* the records camera_demo_path holds. camera_demo_make samples DEMO_SPAN
 * records for each key, 1 / DEMO_SPAN of a key apart */
#define DEMO_RECORDS 420
#define DEMO_SPAN 20
/* the scale of the cart that marks the edited target */
#define MARK_SCALE 0.3F

/* a value a page edits: a float, or an int shown in decimal or in hex */
enum cart_value_kind { VALUE_FLOAT, VALUE_INT, VALUE_HEX };

/* the camera editor's modes */
enum camera_demo_mode { DEMO_START, DEMO_EDIT, DEMO_PLAY };

int cart_debug_page;
/* the records camera_demo_make samples from the act's keys */
static struct camera_key camera_demo_path[DEMO_RECORDS];
/* the character the motion page edits */
static int cart_debug_character;
static int cart_motion_row; /* the motion page's selected row */
static int cart_settings_row; /* the settings page's selected row */
/* what up and down add to a value, which right and left scale */
static Float cart_debug_amount;
static enum camera_demo_mode camera_demo_mode;
static Uint32 cart_debug_runs; /* cart_debug_run's calls */
/* cart_debug_run's calls since the page last moved on */
static Uint32 cart_page_runs;
/* the motion page's and the settings page's rows at their hints' last
 * calls */
static int cart_motion_hint_row;
static int cart_settings_hint_row;
static int camera_demo_samples; /* the records camera_demo_make last wrote */
static Sint32 camera_demo_key; /* the key the camera editor edits */

struct cart_motion cart_motions[CHARACTER_COUNT] = {
#include "stg03/cart_debug/cart_motions.inc"
};
struct cart_settings cart_settings = {
    40000.0F, 240, 0.02F, 0.03F, 0.03F, 4.0F, 2, 180, 60,
};
/* the route the route page draws, or every route while it is negative */
static int cart_debug_route = 0;
/* where the route page's X, A, B and Y put player 0 */
static NJS_POINT3 cart_debug_warps[4] = {
    {-4589.0F, -4377.0F, 10648.0F},
    {-3000.0F, 1212.0F, 4307.0F},
    {-3354.0F, 424.0F, 7609.0F},
    {-5549.0F, -4751.0F, 12598.0F},
};
static char *cart_character_names[CHARACTER_COUNT] = {
    "SONIC", "EGGMAN", "MILES", "KNUCKLES", "TIKAL", "AMY", "E_102", "BIG",
};
/* the motion hint's calls since its row last changed, from 1 at the start */
static int cart_motion_hint_calls = 1;
/* each row's hint: a page, then a null, for bitmap_show_pages */
static char *cart_motion_hints[MOTION_ROWS][2] = {
    {TEXT_1, NULL},  {TEXT_2, NULL},  {TEXT_3, NULL},  {TEXT_4, NULL},
    {TEXT_5, NULL},  {TEXT_6, NULL},  {TEXT_7, NULL},  {TEXT_8, NULL},
    {TEXT_9, NULL},  {TEXT_10, NULL}, {TEXT_11, NULL}, {TEXT_12, NULL},
    {TEXT_13, NULL}, {TEXT_14, NULL}, {TEXT_15, NULL},
};
/* the settings hint's calls since its row last changed, from 1 at the start */
static int cart_settings_hint_calls = 1;
/* each row's hint, as for the motion page. row 4's text calls its value an
 * enemy cart's acceleration. a cart accelerates by it only while it looks
 * for a route */
static char *cart_settings_hints[SETTINGS_ROWS][2] = {
    {TEXT_16, NULL}, {TEXT_17, NULL}, {TEXT_18, NULL},
    {TEXT_19, NULL}, {TEXT_20, NULL}, {TEXT_21, NULL},
    {TEXT_22, NULL}, {TEXT_23, NULL}, {TEXT_24, NULL},
};
/* cart_routes_draw's calls */
static int cart_routes_draws = 0;
/* a zero vector, which camera_demo_edit starts each call's move from */
static const NJS_VECTOR camera_demo_still;

static void cart_debug_adjust(enum cart_value_kind kind, void *value);
static void cart_debug_print(enum cart_value_kind kind, void *value, int loc,
                             int selected, int row);
static void cart_motion_edit(void);
static void cart_settings_edit(void);
static void cart_motion_print(void);
static void cart_settings_print(void);
static void cart_motion_hint(void);
static void cart_settings_hint(void);
static void cart_routes_draw(int route);
static void camera_demo_edit(void);
static void camera_demo_turn(struct angles *ang, NJS_VECTOR *vector);
static void camera_demo_draw(int count);
static void camera_demo_mark(NJS_POINT3 *pos);
static int camera_demo_make(void *keys, int count);

void cart_debug_run(void)
{
    if (per_info[0]->press & PDD_DGT_KL) {
        ring_add(1);
    } else if (per_info[0]->press & PDD_DGT_KR) {
        ring_add(-1);
        if ((Sint16)ring_count() < 0) {
            ring_add(-ring_count());
        }
    }
    if ((per_info[1]->on & PDD_DGT_TL) && (per_info[1]->on & PDD_DGT_TR)) {
        stage_restart();
    }
    switch (cart_debug_page) {
    case PAGE_START:
        cart_debug_character = get_character();
        cart_motion_row = 0;
        cart_debug_amount = 1.0F;
        cart_debug_page = PAGE_NONE;
        break;
    case PAGE_NONE:
        camera_demo_mode = DEMO_START;
        break;
    case PAGE_MOTION:
        cart_motion_edit();
        cart_motion_print();
        cart_motion_hint();
        break;
    case PAGE_SETTINGS:
        cart_settings_edit();
        cart_settings_print();
        cart_settings_hint();
        break;
    case PAGE_ROUTES:
        cart_routes_draw(cart_debug_route);
        if (pad_records[1].press & PDD_DGT_TR) {
            ++cart_debug_route;
        } else if (pad_records[1].press & PDD_DGT_TL) {
            --cart_debug_route;
        } else if (pad_records[1].press & PDD_DGT_TX) {
            player_work[0]->pos = cart_debug_warps[0];
        } else if (pad_records[1].press & PDD_DGT_TA) {
            player_work[0]->pos = cart_debug_warps[1];
        } else if (pad_records[1].press & PDD_DGT_TB) {
            player_work[0]->pos = cart_debug_warps[2];
        } else if (pad_records[1].press & PDD_DGT_TY) {
            player_work[0]->pos = cart_debug_warps[3];
        }
        break;
    case PAGE_CAMERA:
        njPrintC(NJM_LOCATION(1, 1), "CAMERA DEMO EDIT");
        njPrintD(NJM_LOCATION(20, 1), camera_cart_action, 2);
        if (camera_cart_action != CAMERA_CART_ORBIT) {
            if (pad_records[0].press & PDD_DGT_TA) {
                camera_cart_action = CAMERA_CART_ORBIT;
            }
        }
        if (camera_cart_action == CAMERA_CART_ORBIT) {
            camera_demo_edit();
            if (pad_records[0].press & PDD_DGT_TB) {
                camera_cart_action = 0;
            }
        }
        break;
    }
    if ((pad_records[1].press & PDD_DGT_ST) && cart_page_runs > PAGE_CALLS &&
        camera_cart_action == 0) {
        cart_page_runs = 0;
        ++cart_debug_page;
        if (cart_debug_page >= PAGES) {
            cart_debug_page = PAGE_NONE;
        }
    } else {
        ++cart_page_runs;
    }
    ++cart_debug_runs;
}

/* edit value by pad_records[1]. right and left scale the amount by 0.1 and by
 * 10, between AMOUNT_LEAST and AMOUNT_MOST. up and down add the amount to
 * the value and take it away. a VALUE_INT edit first raises the amount to
 * 1, and adds it truncated. a VALUE_HEX edit adds it whole and truncates the
 * sum */
static void cart_debug_adjust(enum cart_value_kind kind, void *value)
{
    if (pad_records[1].press & PDD_DGT_KR) {
        cart_debug_amount *= 0.1F;
        if (cart_debug_amount < AMOUNT_LEAST) {
            cart_debug_amount = AMOUNT_LEAST;
        }
    } else if (pad_records[1].press & PDD_DGT_KL) {
        cart_debug_amount *= 10.0F;
        if (cart_debug_amount > AMOUNT_MOST) {
            cart_debug_amount = AMOUNT_MOST;
        }
    }
    switch (kind) {
    case VALUE_FLOAT:
        if (pad_records[1].press & PDD_DGT_KU) {
            *(Float *)value += cart_debug_amount;
        } else if (pad_records[1].press & PDD_DGT_KD) {
            *(Float *)value -= cart_debug_amount;
        }
        break;
    case VALUE_INT:
        if (cart_debug_amount < 1.0F) {
            cart_debug_amount = 1.0F;
        }
        if (pad_records[1].press & PDD_DGT_KU) {
            *(int *)value += (int)cart_debug_amount;
        } else if (pad_records[1].press & PDD_DGT_KD) {
            *(int *)value -= (int)cart_debug_amount;
        }
        break;
    case VALUE_HEX:
        if (pad_records[1].press & PDD_DGT_KU) {
            *(int *)value += cart_debug_amount;
        } else if (pad_records[1].press & PDD_DGT_KD) {
            *(int *)value -= cart_debug_amount;
        }
        break;
    }
}

/* print value at loc, in yellow if row is the selected row. the print color
 * is white after */
static void cart_debug_print(enum cart_value_kind kind, void *value, int loc,
                             int selected, int row)
{
    if (row == selected) {
        njPrintColor(COLOR_YELLOW);
        switch (kind) {
        case VALUE_FLOAT:
            njPrintF(loc, *(Float *)value, 8);
            break;
        case VALUE_INT:
            njPrintD(loc, *(int *)value, 8);
            break;
        case VALUE_HEX:
            njPrintH(loc, *(int *)value, 8);
            break;
        }
        njPrintColor(COLOR_WHITE);
    } else {
        njPrintColor(COLOR_WHITE);
        switch (kind) {
        case VALUE_FLOAT:
            njPrintF(loc, *(Float *)value, 8);
            break;
        case VALUE_INT:
            njPrintD(loc, *(int *)value, 8);
            break;
        case VALUE_HEX:
            njPrintH(loc, *(int *)value, 8);
            break;
        }
    }
}

/* pad_records[1]'s X and A select the row above and the row below, wrapping
 * at the ends. cart_debug_adjust then edits the selected row's value */
static void cart_motion_edit(void)
{
    struct cart_motion *motion;

    if (pad_records[1].press & PDD_DGT_TX) {
        --cart_motion_row;
        if (cart_motion_row < 0) {
            cart_motion_row = MOTION_ROWS - 1;
        }
    } else if (pad_records[1].press & PDD_DGT_TA) {
        ++cart_motion_row;
        if (cart_motion_row >= MOTION_ROWS) {
            cart_motion_row = 0;
        }
    }
    motion = &cart_motions[cart_debug_character];
    switch (cart_motion_row) {
    case 0:
        cart_debug_adjust(VALUE_FLOAT, &motion->max_speed);
        break;
    case 1:
        cart_debug_adjust(VALUE_FLOAT, &motion->min_speed);
        break;
    case 2:
        cart_debug_adjust(VALUE_FLOAT, &motion->max_reverse_speed);
        break;
    case 3:
        cart_debug_adjust(VALUE_FLOAT, &motion->slide);
        break;
    case 4:
        cart_debug_adjust(VALUE_FLOAT, &motion->gravity);
        break;
    case 5:
        cart_debug_adjust(VALUE_FLOAT, &motion->max_fall_speed);
        break;
    case 6:
        cart_debug_adjust(VALUE_FLOAT, &motion->hover_height);
        break;
    case 7:
        cart_debug_adjust(VALUE_FLOAT, &motion->grip);
        break;
    case 8:
        cart_debug_adjust(VALUE_FLOAT, &motion->acceleration);
        break;
    case 9:
        cart_debug_adjust(VALUE_FLOAT, &motion->braking);
        break;
    case 10:
        cart_debug_adjust(VALUE_FLOAT, &motion->friction);
        break;
    case 11:
        cart_debug_adjust(VALUE_INT, &motion->max_vitality);
        break;
    case 12:
        cart_debug_adjust(VALUE_INT, &motion->smoke_vitality);
        break;
    case 13:
        cart_debug_adjust(VALUE_FLOAT, &motion->steering);
        break;
    case 14:
        cart_debug_adjust(VALUE_FLOAT, &motion->drift_steering);
        break;
    }
}

/* as cart_motion_edit, for the settings */
static void cart_settings_edit(void)
{
    if (pad_records[1].press & PDD_DGT_TX) {
        --cart_settings_row;
        if (cart_settings_row < 0) {
            cart_settings_row = SETTINGS_ROWS - 1;
        }
    } else if (pad_records[1].press & PDD_DGT_TA) {
        ++cart_settings_row;
        if (cart_settings_row >= SETTINGS_ROWS) {
            cart_settings_row = 0;
        }
    }
    switch (cart_settings_row) {
    case 0:
        cart_debug_adjust(VALUE_FLOAT, &cart_settings.chase_distance);
        break;
    case 1:
        cart_debug_adjust(VALUE_INT, &cart_settings.rest_time);
        break;
    case 2:
        cart_debug_adjust(VALUE_FLOAT, &cart_settings.enemy_acceleration);
        break;
    case 3:
        cart_debug_adjust(VALUE_FLOAT, &cart_settings.enemy_braking);
        break;
    case 4:
        cart_debug_adjust(VALUE_FLOAT, &cart_settings.search_acceleration);
        break;
    case 5:
        cart_debug_adjust(VALUE_FLOAT, &cart_settings.explosion_speed);
        break;
    case 6:
        cart_debug_adjust(VALUE_HEX, &cart_settings.explosion_spin);
        break;
    case 7:
        cart_debug_adjust(VALUE_INT, &cart_settings.dead_wait_time);
        break;
    case 8:
        cart_debug_adjust(VALUE_INT, &cart_settings.ring_loss_time);
        break;
    }
}

/* print row i's label, in orange if i is the selected row. VALUE_LOC(i),
 * below, is where row i's value prints */
#define PRINT_LABEL(i, selected, text)                                         \
    do {                                                                       \
        if ((i) == (selected)) {                                               \
            njPrintColor(COLOR_ORANGE);                                        \
        } else {                                                               \
            njPrintColor(COLOR_WHITE);                                         \
        }                                                                      \
        njPrintC(NJM_LOCATION(4, (i) + 4), (text));                            \
    } while (0)
#define VALUE_LOC(i) NJM_LOCATION(19, (i) + 4)

/* print the character's name and the amount, then each row's label and
 * value */
static void cart_motion_print(void)
{
    int i;

    njPrintColor(COLOR_BLUE);
    njPrintC(NJM_LOCATION(4, 3), cart_character_names[cart_debug_character]);
    njPrintColor(COLOR_PALE_BLUE);
    njPrintF(NJM_LOCATION(19, 3), cart_debug_amount, 4);
    for (i = 0; i < MOTION_ROWS; ++i) {
        switch (i) {
        case 0:
            PRINT_LABEL(i, cart_motion_row, "max_spd");
            cart_debug_print(VALUE_FLOAT,
                             &cart_motions[cart_debug_character].max_speed,
                             VALUE_LOC(i), cart_motion_row, i);
            break;
        case 1:
            PRINT_LABEL(i, cart_motion_row, "min_spd");
            cart_debug_print(VALUE_FLOAT,
                             &cart_motions[cart_debug_character].min_speed,
                             VALUE_LOC(i), cart_motion_row, i);
            break;
        case 2:
            PRINT_LABEL(i, cart_motion_row, "max_back_spd");
            cart_debug_print(
                VALUE_FLOAT,
                &cart_motions[cart_debug_character].max_reverse_speed,
                VALUE_LOC(i), cart_motion_row, i);
            break;
        case 3:
            PRINT_LABEL(i, cart_motion_row, "slide_spd");
            cart_debug_print(VALUE_FLOAT,
                             &cart_motions[cart_debug_character].slide,
                             VALUE_LOC(i), cart_motion_row, i);
            break;
        case 4:
            PRINT_LABEL(i, cart_motion_row, "gravitation");
            cart_debug_print(VALUE_FLOAT,
                             &cart_motions[cart_debug_character].gravity,
                             VALUE_LOC(i), cart_motion_row, i);
            break;
        case 5:
            PRINT_LABEL(i, cart_motion_row, "gravitation_max");
            cart_debug_print(VALUE_FLOAT,
                             &cart_motions[cart_debug_character].max_fall_speed,
                             VALUE_LOC(i), cart_motion_row, i);
            break;
        case 6:
            PRINT_LABEL(i, cart_motion_row, "hover_point");
            cart_debug_print(VALUE_FLOAT,
                             &cart_motions[cart_debug_character].hover_height,
                             VALUE_LOC(i), cart_motion_row, i);
            break;
        case 7:
            PRINT_LABEL(i, cart_motion_row, "grip");
            cart_debug_print(VALUE_FLOAT,
                             &cart_motions[cart_debug_character].grip,
                             VALUE_LOC(i), cart_motion_row, i);
            break;
        case 8:
            PRINT_LABEL(i, cart_motion_row, "spd_accell");
            cart_debug_print(VALUE_FLOAT,
                             &cart_motions[cart_debug_character].acceleration,
                             VALUE_LOC(i), cart_motion_row, i);
            break;
        case 9:
            PRINT_LABEL(i, cart_motion_row, "spd_break");
            cart_debug_print(VALUE_FLOAT,
                             &cart_motions[cart_debug_character].braking,
                             VALUE_LOC(i), cart_motion_row, i);
            break;
        case 10:
            PRINT_LABEL(i, cart_motion_row, "spd_masatu");
            cart_debug_print(VALUE_FLOAT,
                             &cart_motions[cart_debug_character].friction,
                             VALUE_LOC(i), cart_motion_row, i);
            break;
        case 11:
            PRINT_LABEL(i, cart_motion_row, "max_vitality");
            cart_debug_print(VALUE_INT,
                             &cart_motions[cart_debug_character].max_vitality,
                             VALUE_LOC(i), cart_motion_row, i);
            break;
        case 12:
            PRINT_LABEL(i, cart_motion_row, "smoke_vitality");
            cart_debug_print(VALUE_INT,
                             &cart_motions[cart_debug_character].smoke_vitality,
                             VALUE_LOC(i), cart_motion_row, i);
            break;
        case 13:
            PRINT_LABEL(i, cart_motion_row, "cart_handle");
            cart_debug_print(VALUE_FLOAT,
                             &cart_motions[cart_debug_character].steering,
                             VALUE_LOC(i), cart_motion_row, i);
            break;
        case 14:
            PRINT_LABEL(i, cart_motion_row, "drift_rate");
            cart_debug_print(VALUE_FLOAT,
                             &cart_motions[cart_debug_character].drift_steering,
                             VALUE_LOC(i), cart_motion_row, i);
            break;
        }
    }
}

/* print the amount, then each row's label and value */
static void cart_settings_print(void)
{
    struct cart_settings *settings;
    int i;

    njPrintColor(COLOR_PALE_BLUE);
    njPrintF(NJM_LOCATION(19, 3), cart_debug_amount, 4);
    settings = &cart_settings;
    /* @hack redundant: the empty else, which loads the selected row into R3 for
     * row 0's label test (0x0c9162b8), not R2; without it, 2 of 283 words
     * differ */
    /* @hack redundant: a never-run test of settings. SHC counts the
     * references to settings, and with this one and the four in row 0's
     * case, settings takes R8 (0x0c91626e) in a 4-byte frame. otherwise R8
     * holds njPrintC's address, and settings goes to an 8-byte frame.
     * without this test, 259 of 283 words differ */
    if (0) {
        if (settings) {
        }
    } else {
    }
    for (i = 0; i < SETTINGS_ROWS; ++i) {
        switch (i) {
        case 0:
            PRINT_LABEL(i, cart_settings_row, "chase_distance");
            cart_debug_print(VALUE_FLOAT, &settings->chase_distance,
                             VALUE_LOC(i), cart_settings_row, i);
            /* @hack redundant: the empty else, which loads the selected row
             * into R3 for row 1's label test (0x0c916340), not R2; without it,
             * 2 of 283 words differ */
            /* @hack redundant: four never-run tests of settings, which SHC
             * counts with the never-run test above the loop; without them,
             * 246 of 283 words and 24 pool words differ */
            if (0) {
                if (settings) {
                }
                if (settings) {
                }
                if (settings) {
                }
                if (settings) {
                }
            } else {
            }
            break;
        case 1:
            PRINT_LABEL(i, cart_settings_row, "rest_time");
            cart_debug_print(VALUE_INT, &settings->rest_time, VALUE_LOC(i),
                             cart_settings_row, i);
            break;
        case 2:
            PRINT_LABEL(i, cart_settings_row, "cart_enemy_accell");
            cart_debug_print(VALUE_FLOAT, &settings->enemy_acceleration,
                             VALUE_LOC(i), cart_settings_row, i);
            break;
        case 3:
            PRINT_LABEL(i, cart_settings_row, "cart_enemy_break");
            cart_debug_print(VALUE_FLOAT, &settings->enemy_braking,
                             VALUE_LOC(i), cart_settings_row, i);
            break;
        case 4:
            PRINT_LABEL(i, cart_settings_row, "search_addspd");
            cart_debug_print(VALUE_FLOAT, &settings->search_acceleration,
                             VALUE_LOC(i), cart_settings_row, i);
            break;
        case 5:
            PRINT_LABEL(i, cart_settings_row, "explose_spd");
            cart_debug_print(VALUE_FLOAT, &settings->explosion_speed,
                             VALUE_LOC(i), cart_settings_row, i);
            break;
        case 6:
            PRINT_LABEL(i, cart_settings_row, "explose_ang");
            cart_debug_print(VALUE_HEX, &settings->explosion_spin, VALUE_LOC(i),
                             cart_settings_row, i);
            break;
        case 7:
            PRINT_LABEL(i, cart_settings_row, "dead_wait_time");
            cart_debug_print(VALUE_INT, &settings->dead_wait_time, VALUE_LOC(i),
                             cart_settings_row, i);
            break;
        case 8:
            PRINT_LABEL(i, cart_settings_row, "ring_sub_timer");
            cart_debug_print(VALUE_INT, &settings->ring_loss_time, VALUE_LOC(i),
                             cart_settings_row, i);
            break;
        }
    }
}

/* show the selected row's hint on pad_records[1]'s Y, and on the
 * HINT_CALLS-th call after the row changes */
static void cart_motion_hint(void)
{
    ++cart_motion_hint_calls;
    if ((pad_records[1].press & PDD_DGT_TY) ||
        cart_motion_hint_calls == HINT_CALLS) {
        bitmap_show_pages(cart_motion_hints[cart_motion_row]);
    }
    if (cart_motion_hint_row != cart_motion_row) {
        cart_motion_hint_calls = 0;
    }
    cart_motion_hint_row = cart_motion_row;
}

/* as cart_motion_hint, for the settings */
static void cart_settings_hint(void)
{
    ++cart_settings_hint_calls;
    if ((pad_records[1].press & PDD_DGT_TY) ||
        cart_settings_hint_calls == HINT_CALLS) {
        bitmap_show_pages(cart_settings_hints[cart_settings_row]);
    }
    if (cart_settings_hint_row != cart_settings_row) {
        cart_settings_hint_calls = 0;
    }
    cart_settings_hint_row = cart_settings_row;
}

/* the point of a route of count points whose line the route page marks. it
 * moves on a point every ROUTE_MARK_CALLS calls */
#define ROUTE_MARK_CALLS 8
#define MARKED_POINT(count) (cart_routes_draws / ROUTE_MARK_CALLS % (count))

/* draw the act's route in green, closed from its last point to its first.
 * print its number and its points' count. a negative route draws every route
 * of the act. one line of each route is red, or yellow while every route
 * draws */
static void cart_routes_draw(int route)
{
    int act = stage_get_stage_act() & 0xff;
    Sint32 i;
    Sint32 j;
    NJS_COLOR color;

    if (route >= 0) {
        njPrintC(NJM_LOCATION(1, 1), "NOW LINE = ");
        njPrintD(NJM_LOCATION(14, 1), route, 2);
        /* @bug memory: cart_debug_run steps the route up on R with no
         * bound. past route ROUTES - 1, this reads past the act's routes
         * (0x0c916662) */
        njPrintD(NJM_LOCATION(24, 1), cart_routes[act][route].count, 3);
        ++cart_routes_draws;
        for (i = 0; i < cart_routes[act][route].count; ++i) {
            if (MARKED_POINT(cart_routes[act][route].count) == i) {
                color.argb.a = 255;
                color.argb.r = 255;
                color.argb.g = 0;
                color.argb.b = 0;
            } else {
                color.argb.a = 255;
                color.argb.r = 0;
                color.argb.g = 255;
                color.argb.b = 0;
            }
            if (i != cart_routes[act][route].count - 1) {
                draw_line_colored(cart_routes[act][route].points[i].x,
                                  cart_routes[act][route].points[i].y,
                                  cart_routes[act][route].points[i].z, &color,
                                  cart_routes[act][route].points[i + 1].x,
                                  cart_routes[act][route].points[i + 1].y,
                                  cart_routes[act][route].points[i + 1].z,
                                  &color);
            } else {
                draw_line_colored(cart_routes[act][route].points[i].x,
                                  cart_routes[act][route].points[i].y,
                                  cart_routes[act][route].points[i].z, &color,
                                  cart_routes[act][route].points[0].x,
                                  cart_routes[act][route].points[0].y,
                                  cart_routes[act][route].points[0].z, &color);
            }
        }
    } else {
        njPrintC(NJM_LOCATION(1, 1), "ALL LINE DISP");
        ++cart_routes_draws;
        for (j = 0; j < RAILS; ++j) {
            for (i = 0; i < cart_routes[act][j].count; ++i) {
                if (MARKED_POINT(cart_routes[act][j].count) == i) {
                    color.argb.a = 255;
                    color.argb.r = 255;
                    color.argb.g = 255;
                    color.argb.b = 0;
                } else {
                    color.argb.a = 255;
                    color.argb.r = 0;
                    color.argb.g = 255;
                    color.argb.b = 0;
                }
                if (i != cart_routes[act][j].count - 1) {
                    draw_line_colored(cart_routes[act][j].points[i].x,
                                      cart_routes[act][j].points[i].y,
                                      cart_routes[act][j].points[i].z, &color,
                                      cart_routes[act][j].points[i + 1].x,
                                      cart_routes[act][j].points[i + 1].y,
                                      cart_routes[act][j].points[i + 1].z,
                                      &color);
                } else {
                    draw_line_colored(cart_routes[act][j].points[i].x,
                                      cart_routes[act][j].points[i].y,
                                      cart_routes[act][j].points[i].z, &color,
                                      cart_routes[act][j].points[0].x,
                                      cart_routes[act][j].points[0].y,
                                      cart_routes[act][j].points[0].z, &color);
                }
            }
        }
        for (j = RAILS; j < ROUTES; ++j) {
            for (i = 0; i < cart_routes[act][j].count; ++i) {
                if (MARKED_POINT(cart_routes[act][j].count) == i) {
                    color.argb.a = 255;
                    color.argb.r = 255;
                    color.argb.g = 255;
                    color.argb.b = 0;
                } else {
                    color.argb.a = 255;
                    color.argb.r = 0;
                    color.argb.g = 255;
                    color.argb.b = 0;
                }
                if (i != cart_routes[act][j].count - 1) {
                    draw_line_colored(cart_routes[act][j].points[i].x,
                                      cart_routes[act][j].points[i].y,
                                      cart_routes[act][j].points[i].z, &color,
                                      cart_routes[act][j].points[i + 1].x,
                                      cart_routes[act][j].points[i + 1].y,
                                      cart_routes[act][j].points[i + 1].z,
                                      &color);
                } else {
                    draw_line_colored(cart_routes[act][j].points[i].x,
                                      cart_routes[act][j].points[i].y,
                                      cart_routes[act][j].points[i].z, &color,
                                      cart_routes[act][j].points[0].x,
                                      cart_routes[act][j].points[0].y,
                                      cart_routes[act][j].points[0].z, &color);
                }
            }
        }
    }
}

/* @hack type: a key's target by its offset in the key, which adds the 12 to
 * the key's address in place (ADD #12 at 0x0c916ac0 and 0x0c916d78). as
 * &key->target, SHC copies the address first, and 35 of 392 words and 2
 * pool words differ */
#define TARGET_BY_OFFSET(key)                                                  \
    ((NJS_POINT3 *)((char *)(key) + sizeof((key)->pos)))

/* run the camera editor. it starts at key 0, and act 0's key 0 takes player
 * 0's position as its target. camera_place's args point at the edited key's
 * pos and target. the orbiting camera then writes its position into the
 * key, and looks at the key's target. pad_records[1] moves the target. B
 * copies the key into the next and moves on to it, and A moves back. past
 * key 3, B and A first sample the path of the keys before the edited one. Y
 * has the camera play the path */
static void camera_demo_edit(void)
{
    int act = stage_get_stage_act() & 0xff;
    NJS_VECTOR move;

    njPrintC(NJM_LOCATION(1, 6), "START ADDRESS = ");
    njPrintH(NJM_LOCATION(20, 6), (Uint32)camera_demo_keys[act], 8);
    njPrintC(NJM_LOCATION(1, 7), "Length =  ");
    njPrintH(NJM_LOCATION(20, 7), sizeof(camera_demo_keys[act]), 8);
    switch (camera_demo_mode) {
    case DEMO_START:
        camera_demo_key = 0;
        /* @hack redundant: a jump to the next statement, which loads
         * player_work's address after the store of 0 (0x0c916a82); without it,
         * the address loads first, R2 and R3 trade through 0x0c916b54, and 34
         * of 392 words differ */
        goto start;
    start:
        /* @hack type: the target through target.x as a point, which forms
         * the target's address (ADD #12 at 0x0c916a88) and copies through
         * it; as a member copy, it stores at 12, 16 and 20 from the key, and
         * 56 of 392 words differ */
        *(NJS_POINT3 *)&camera_demo_keys[0][0].target.x = player_work[0]->pos;
        camera_place.args[0].point =
            &camera_demo_keys[act][camera_demo_key].pos;
        camera_place.args[1].point =
            TARGET_BY_OFFSET(&camera_demo_keys[act][camera_demo_key]);
        ++camera_demo_mode;
        camera_demo_samples = 0;
        break;
    case DEMO_EDIT:
        njPrintC(NJM_LOCATION(1, 4), "edit_point");
        njPrintD(NJM_LOCATION(15, 4), camera_demo_key, 4);
        move = camera_demo_still;
        if (!(pad_records[1].on & PDD_DGT_TX)) {
            if (pad_records[1].on & PDD_DGT_KU) {
                move.z = -1.0F;
            }
            if (pad_records[1].on & PDD_DGT_KD) {
                move.z = 1.0F;
            }
            if (pad_records[1].on & PDD_DGT_KL) {
                move.x = -1.0F;
            }
            if (pad_records[1].on & PDD_DGT_KR) {
                move.x = 1.0F;
            }
        }
        camera_demo_turn(&camera_work->ang, &move);
        move.y = 0.0F;
        if (njScalor(&move) != 0.0F) {
            njUnitVector(&move);
        }
        if (pad_records[1].on & PDD_DGT_TX) {
            if (pad_records[1].on & PDD_DGT_KU) {
                move.y = 1.0F;
            }
            if (pad_records[1].on & PDD_DGT_KD) {
                move.y = -1.0F;
            }
        }
        /* @bug memory: once B below has stepped the key past
         * DEMO_KEYS - 1, these read and write past the act's keys (0x0c916be6
         * to 0x0c916c2e) */
        camera_demo_keys[act][camera_demo_key].target.x += move.x;
        camera_demo_keys[act][camera_demo_key].target.y += move.y;
        camera_demo_keys[act][camera_demo_key].target.z += move.z;
#ifdef GCC
        /* repairs the reads and writes the tags name: GCC's B steps the key
         * to the act's last at most, DEMO_KEYS - 1 */
        if ((pad_records[1].press & PDD_DGT_TB) &&
            camera_demo_key < DEMO_KEYS - 1) {
#else
        if (pad_records[1].press & PDD_DGT_TB) {
#endif
            if (camera_demo_key > 3) {
                /* @bug memory: B steps the key up with no bound. from
                 * key 21 on, this samples keys past the act's, and from key 22
                 * on, its records run past camera_demo_path (0x0c916ca4) */
                /* @hack spelling: the act's keys by their array's address,
                 * which keeps the key in R5 and the 3 it is tested against in
                 * R4 (0x0c916c34, 0x0c916c3e); as camera_demo_keys[act], the
                 * two trade and 124 of 392 words and 2 pool words differ */
                camera_demo_samples =
                    camera_demo_make(&camera_demo_keys[act], camera_demo_key);
            }
            /* @bug memory: from key DEMO_KEYS - 1 on, this copies past
             * the act's keys (0x0c916cd2 to 0x0c916d0c) */
            camera_demo_keys[act][camera_demo_key + 1].pos =
                camera_demo_keys[act][camera_demo_key].pos;
            camera_demo_keys[act][camera_demo_key + 1].target =
                camera_demo_keys[act][camera_demo_key].target;
            ++camera_demo_key;
        } else if (pad_records[1].press & PDD_DGT_TA) {
            if (camera_demo_key > 3) {
                /* @bug memory: as above, at 0x0c916d20 */
                /* @hack spelling: the act's keys by their array's address
                 * again, which lets this test take the key in R5 and the 3 in
                 * R4 that 0x0c916c34 and 0x0c916c3e load for both
                 * (0x0c916d1a); as camera_demo_keys[act] here alone, the key
                 * goes through the frame, and 143 of 392 words differ */
                camera_demo_samples =
                    camera_demo_make(&camera_demo_keys[act], camera_demo_key);
            }
            --camera_demo_key;
            if (camera_demo_key < 0) {
                camera_demo_key = 0;
            }
        }
        camera_demo_mark(&camera_demo_keys[act][camera_demo_key].target);
        camera_place.args[0].point =
            &camera_demo_keys[act][camera_demo_key].pos;
        camera_place.args[1].point =
            TARGET_BY_OFFSET(&camera_demo_keys[act][camera_demo_key]);
        camera_demo_draw(camera_demo_samples);
        if (pad_records[1].press & PDD_DGT_TY) {
            camera_demo_mode = DEMO_PLAY;
        }
        break;
    case DEMO_PLAY:
        /* the camera plays records DEMO_SPAN to camera_demo_samples - 2 *
         * DEMO_SPAN */
        camera_cart_action = CAMERA_CART_PLAY;
        camera_place.args[0].keys = &camera_demo_path[DEMO_SPAN];
        camera_place.args[1].last = camera_demo_samples - (3 * DEMO_SPAN);
        /* @bug logic: this reads back the CAMERA_CART_PLAY stored above
         * (0x0c916dd2), so it never holds. the editor stays in DEMO_PLAY
         * until PAGE_NONE resets it */
        if (!camera_cart_action) {
            camera_demo_mode = DEMO_EDIT;
            cart_debug_page = PAGE_START;
        }
        break;
    }
}

/* turn vector by ang, through njRotateZ, njRotateX and njRotateY in that
 * order */
static void camera_demo_turn(struct angles *ang, NJS_VECTOR *vector)
{
    if (njScalor(vector) != 0.0F) {
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, ang->z);
        TURN(njRotateX, ang->x);
        TURN(njRotateY, ang->y);
        njCalcVector(NULL, vector, vector);
        njPopMatrix(1);
    }
}

/* @hack type: a key's target through a one-element array, which forms its
 * address (ADD #12 at 0x0c916f4e and 0x0c916f5a) and reads from that. by
 * member, draw_line_colored's address takes R9 and the path's R8, and 89 of
 * 104 words differ */
#define TARGET_IN_ARRAY(key) (*(NJS_POINT3(*)[1])(&(key)->target))[0]

/* draw a line from each of the path's first count records to the next. the
 * line between positions is red, and the one between targets yellow */
static void camera_demo_draw(int count)
{
    /* @hack spelling: count through a local, limit, which keeps the frame at
     * 8 bytes (ADD #-8,R15 at 0x0c916ed0). from count, SHC keeps
     * draw_line_colored's address in R9, the frame grows to 12 bytes, and 77
     * of 104 words and 5 pool words differ */
    int limit = count;
    int i;
    NJS_COLOR color;
    struct camera_key *next;
    const NJS_COLOR *line_color;

    if (limit != 0) {
#ifdef GCC
        /* repairs the read past camera_demo_path the tag below names: GCC's
         * lines end at its last record */
        for (i = 0; i < limit && i < DEMO_RECORDS - 1; ++i) {
#else
        for (i = 0; i < limit; ++i) {
#endif
            color.argb.a = 255;
            color.argb.r = 255;
            color.argb.g = 0;
            color.argb.b = 0;
            /* @hack spelling: the first line's second color through a local,
             * which puts the red's r and g stores among the shifts that scale
             * i + 1 (0x0c916ef4, 0x0c916efc); with &color, each comes a slot
             * earlier and 8 of 104 words differ */
            line_color = &color;
            /* @hack type: the next record by its own byte offset, taken in 16
             * bits, which scales i + 1 itself (0x0c916eea to 0x0c916f04). the
             * offset is &camera_demo_path[i + 1]'s while it fits in 16 bits, as
             * it does for each of the path's DEMO_RECORDS records. that form
             * steps on from record i's address, the two functions after it
             * move, and 85 of 104 words differ */
            next = (struct camera_key *)((char *)camera_demo_path +
                                         (Sint16)((i + 1) *
                                                  sizeof(struct camera_key)));
            /* @bug memory: the last record's lines run to record count,
             * one past the last camera_demo_make wrote. from DEMO_RECORDS on,
             * that record lies past camera_demo_path (0x0c916f0e) */
            draw_line_colored(camera_demo_path[i].pos.x,
                              camera_demo_path[i].pos.y,
                              camera_demo_path[i].pos.z, &color, next->pos.x,
                              next->pos.y, next->pos.z, line_color);
            color.argb.a = 255;
            color.argb.r = 255;
            color.argb.g = 255;
            color.argb.b = 0;
            draw_line_colored(TARGET_IN_ARRAY(&camera_demo_path[i]).x,
                              TARGET_IN_ARRAY(&camera_demo_path[i]).y,
                              TARGET_IN_ARRAY(&camera_demo_path[i]).z, &color,
                              TARGET_IN_ARRAY(next).x, TARGET_IN_ARRAY(next).y,
                              TARGET_IN_ARRAY(next).z, &color);
            /* @hack redundant: a never-run print of i, whose references SHC
             * counts: they keep draw_line_colored's address out of R9 and the
             * path's out of R8, and the frame under 16 bytes; without it, 87 of
             * 104 words differ */
            if (0) {
                njPrintD(0, i, 0);
            }
        }
    }
}

/* @hack spelling: defined after camera_demo_draw, which places it among the
 * constants between camera_demo_edit's strings and camera_demo_make's, at
 * 0x0c9f9378. defined at its declaration above the code, it comes before
 * them: 31 of the code's pool words differ, and 191 of the constants' 521
 * words differ */
static const NJS_VECTOR camera_demo_still = {0.0F, 0.0F, 0.0F};

/* mark pos with the cart, drawn at MARK_SCALE */
static void camera_demo_mark(NJS_POINT3 *pos)
{
    njSetTexture(&obj_shareobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, pos);
    njScale(NULL, MARK_SCALE, MARK_SCALE, MARK_SCALE);
    draw_tree(&cart_object);
    njPopMatrix(1);
}

/* sample the closed uniform cubic b-spline of count keys into
 * camera_demo_path, DEMO_SPAN records to a key. return how many records it
 * wrote */
static int camera_demo_make(void *keys, int count)
{
    /* @hack redundant: an array never used, whose 4 bytes give the image's
     * 28-byte frame (ADD #-28,R15 at 0x0c917002); without it, 2 of 21 words
     * differ */
    char unused[4];
    struct path_curve curve;

    curve.keys = keys;
    curve.count = count;
    curve.format = "ffffff";
    curve.step = 1.0F / DEMO_SPAN;
    curve.out = camera_demo_path;
    curve.kind = PATH_CURVE_BSPLINE;
    return path_curve_sample(&curve);
}
