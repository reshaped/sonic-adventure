/* the cast: Knuckles, Big, or Amy and Tails, drawn over movie list 7 */
#include <ninja.h>
#include <sg_xpt.h>

#include "event/event.h"
#include "main.h"
#include "movie/cast.h"
#include "player/player.h"
#include "player/textures.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"

/* a character of a cast, which runs down a path while its action loops */
struct cast_member {
    NJS_ACTION *action;
    Float action_speed; /* the action's frames each movie frame */
    NJS_TEXLIST *texlist;
    struct event_path *path;
    Float path_speed; /* the path's points each movie frame */
    int cue; /* the movie frame the member starts at */
    NJS_POINT3 offset; /* from its place on the path, turned by the path */
};

struct cast {
    struct cast_member *members;
    int count;
};

/* where a member is in its run */
struct cast_state {
    Bool active; /* from its cue until it reaches its path's last point */
    Float path_frame; /* its place on the path, in points */
    Float action_frame; /* its action's frame */
};

/* the view the cast is drawn from: its turn about y and its place */
#define VIEW_TURN 0xb300
#define VIEW_X 284.8277F
#define VIEW_Y 65.06F
#define VIEW_Z (-301.6F)

/* the Angles in a 256th of a turn, the unit of a path point's angles. an
 * Angle is a 65536th of a turn */
#define ANGLE_PER_256TH 256.0F

extern NJS_ACTION knuckles_glide_action;
/* the models cast_amy_action and cast_tails_action move */
extern NJS_OBJECT amy_object;
extern NJS_OBJECT tails_object;
extern NJS_ACTION cast_big_action;

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 6
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 7
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 7
#define TEXT_11_LENGTH 7
#define TEXT_12_LENGTH 7
#define TEXT_13_LENGTH 9
#define TEXT_14_LENGTH 8
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 9
#define TEXT_17_LENGTH 9
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 9
#define TEXT_20_LENGTH 8
#define TEXT_21_LENGTH 9
#define TEXT_22_LENGTH 10
#define TEXT_23_LENGTH 8
#define TEXT_24_LENGTH 8
#define TEXT_25_LENGTH 10
#define TEXT_26_LENGTH 10
#define TEXT_27_LENGTH 11
#define TEXT_28_LENGTH 10
#define TEXT_29_LENGTH 10
#define TEXT_30_LENGTH 10
#define TEXT_31_LENGTH 10
#define TEXT_32_LENGTH 10
#define TEXT_33_LENGTH 10
#define TEXT_34_LENGTH 10
#define TEXT_35_LENGTH 9
#define TEXT_36_LENGTH 10
#define TEXT_37_LENGTH 10
#define TEXT_38_LENGTH 10
#define TEXT_39_LENGTH 10
#define TEXT_40_LENGTH 10
#define TEXT_41_LENGTH 10
#define TEXT_42_LENGTH 10
#define TEXT_43_LENGTH 9
#define TEXT_44_LENGTH 9
#define TEXT_45_LENGTH 9
#define TEXT_46_LENGTH 9
#define TEXT_47_LENGTH 8
#define TEXT_48_LENGTH 8
#define TEXT_49_LENGTH 10
#define TEXT_50_LENGTH 10
#define TEXT_51_LENGTH 10
#define TEXT_52_LENGTH 13
#define TEXT_53_LENGTH 13
#define TEXT_54_LENGTH 12
#include "movie/cast/text.inc"

/* Big's textures and texlist, then Amy's and Tails' motions and actions,
 * then the three paths the cast runs down */
static NJS_TEXNAME cast_big_textures[54] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0},
    {TEXT_37, 0, 0}, {TEXT_38, 0, 0}, {TEXT_39, 0, 0}, {TEXT_40, 0, 0},
    {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_43, 0, 0}, {TEXT_44, 0, 0},
    {TEXT_45, 0, 0}, {TEXT_46, 0, 0}, {TEXT_47, 0, 0}, {TEXT_48, 0, 0},
    {TEXT_49, 0, 0}, {TEXT_50, 0, 0}, {TEXT_51, 0, 0}, {TEXT_52, 0, 0},
    {TEXT_53, 0, 0}, {TEXT_54, 0, 0},
};
static NJS_TEXLIST cast_big_texlist = {
#include "movie/cast/cast_big_texlist.inc"
};
static NJS_MKEY_F dat_8cec9e68[32] = {
#include "movie/cast/dat_8cec9e68.inc"
};
static NJS_MKEY_A dat_8ceca068[2] = {
#include "movie/cast/dat_8ceca068.inc"
};
static NJS_MKEY_A dat_8ceca088[32] = {
#include "movie/cast/dat_8ceca088.inc"
};
static NJS_MKEY_A dat_8ceca288[32] = {
#include "movie/cast/dat_8ceca288.inc"
};
static NJS_MKEY_A dat_8ceca488[2] = {
#include "movie/cast/dat_8ceca488.inc"
};
static NJS_MKEY_A dat_8ceca4a8[2] = {
#include "movie/cast/dat_8ceca4a8.inc"
};
static NJS_MKEY_A dat_8ceca4c8[2] = {
#include "movie/cast/dat_8ceca4c8.inc"
};
static NJS_MKEY_A dat_8ceca4e8[2] = {
#include "movie/cast/dat_8ceca4e8.inc"
};
static NJS_MKEY_A dat_8ceca508[2] = {
#include "movie/cast/dat_8ceca508.inc"
};
static NJS_MKEY_A dat_8ceca528[2] = {
#include "movie/cast/dat_8ceca528.inc"
};
static NJS_MKEY_A dat_8ceca548[2] = {
#include "movie/cast/dat_8ceca548.inc"
};
static NJS_MKEY_A dat_8ceca568[31] = {
#include "movie/cast/dat_8ceca568.inc"
};
static NJS_MKEY_A dat_8ceca758[31] = {
#include "movie/cast/dat_8ceca758.inc"
};
static NJS_MKEY_A dat_8ceca948[31] = {
#include "movie/cast/dat_8ceca948.inc"
};
static NJS_MKEY_A dat_8cecab38[2] = {
#include "movie/cast/dat_8cecab38.inc"
};
static NJS_MKEY_A dat_8cecab58[2] = {
#include "movie/cast/dat_8cecab58.inc"
};
static NJS_MKEY_A dat_8cecab78[2] = {
#include "movie/cast/dat_8cecab78.inc"
};
static NJS_MKEY_A dat_8cecab98[2] = {
#include "movie/cast/dat_8cecab98.inc"
};
static NJS_MKEY_A dat_8cecabb8[31] = {
#include "movie/cast/dat_8cecabb8.inc"
};
static NJS_MKEY_A dat_8cecada8[32] = {
#include "movie/cast/dat_8cecada8.inc"
};
static NJS_MKEY_A dat_8cecafa8[25] = {
#include "movie/cast/dat_8cecafa8.inc"
};
static NJS_MKEY_A dat_8cecb138[2] = {
#include "movie/cast/dat_8cecb138.inc"
};
static NJS_MKEY_A dat_8cecb158[2] = {
#include "movie/cast/dat_8cecb158.inc"
};
static NJS_MKEY_A dat_8cecb178[29] = {
#include "movie/cast/dat_8cecb178.inc"
};
static NJS_MKEY_A dat_8cecb348[29] = {
#include "movie/cast/dat_8cecb348.inc"
};
static NJS_MKEY_A dat_8cecb518[29] = {
#include "movie/cast/dat_8cecb518.inc"
};
static NJS_MKEY_A dat_8cecb6e8[31] = {
#include "movie/cast/dat_8cecb6e8.inc"
};
static NJS_MKEY_A dat_8cecb8d8[2] = {
#include "movie/cast/dat_8cecb8d8.inc"
};
static NJS_MKEY_A dat_8cecb8f8[31] = {
#include "movie/cast/dat_8cecb8f8.inc"
};
static NJS_MKEY_A dat_8cecbae8[17] = {
#include "movie/cast/dat_8cecbae8.inc"
};
static NJS_MKEY_A dat_8cecbbf8[26] = {
#include "movie/cast/dat_8cecbbf8.inc"
};
static NJS_MKEY_A dat_8cecbd98[31] = {
#include "movie/cast/dat_8cecbd98.inc"
};
static NJS_MKEY_A dat_8cecbf88[2] = {
#include "movie/cast/dat_8cecbf88.inc"
};
static NJS_MDATA2 dat_8cecbfa8[78] = {
#include "movie/cast/dat_8cecbfa8.inc"
};
static NJS_MOTION dat_8cecc488 = {
#include "movie/cast/dat_8cecc488.inc"
};
static NJS_ACTION cast_amy_action = {
#include "movie/cast/cast_amy_action.inc"
};
static NJS_MKEY_F dat_8cecc49c[2] = {
#include "movie/cast/dat_8cecc49c.inc"
};
static NJS_MKEY_A dat_8cecc4bc[32] = {
#include "movie/cast/dat_8cecc4bc.inc"
};
static NJS_MKEY_A dat_8cecc6bc[31] = {
#include "movie/cast/dat_8cecc6bc.inc"
};
static NJS_MKEY_A dat_8cecc8ac[2] = {
#include "movie/cast/dat_8cecc8ac.inc"
};
static NJS_MKEY_A dat_8cecc8cc[2] = {
#include "movie/cast/dat_8cecc8cc.inc"
};
static NJS_MKEY_A dat_8cecc8ec[32] = {
#include "movie/cast/dat_8cecc8ec.inc"
};
static NJS_MKEY_A dat_8ceccaec[25] = {
#include "movie/cast/dat_8ceccaec.inc"
};
static NJS_MKEY_A dat_8ceccc7c[2] = {
#include "movie/cast/dat_8ceccc7c.inc"
};
static NJS_MKEY_A dat_8ceccc9c[2] = {
#include "movie/cast/dat_8ceccc9c.inc"
};
static NJS_MKEY_A dat_8cecccbc[2] = {
#include "movie/cast/dat_8cecccbc.inc"
};
static NJS_MKEY_A dat_8cecccdc[32] = {
#include "movie/cast/dat_8cecccdc.inc"
};
static NJS_MKEY_A dat_8ceccedc[23] = {
#include "movie/cast/dat_8ceccedc.inc"
};
static NJS_MKEY_A dat_8cecd04c[2] = {
#include "movie/cast/dat_8cecd04c.inc"
};
static NJS_MKEY_A dat_8cecd06c[2] = {
#include "movie/cast/dat_8cecd06c.inc"
};
static NJS_MKEY_A dat_8cecd08c[2] = {
#include "movie/cast/dat_8cecd08c.inc"
};
static NJS_MKEY_A dat_8cecd0ac[32] = {
#include "movie/cast/dat_8cecd0ac.inc"
};
static NJS_MKEY_A dat_8cecd2ac[29] = {
#include "movie/cast/dat_8cecd2ac.inc"
};
static NJS_MKEY_A dat_8cecd47c[31] = {
#include "movie/cast/dat_8cecd47c.inc"
};
static NJS_MKEY_A dat_8cecd66c[32] = {
#include "movie/cast/dat_8cecd66c.inc"
};
static NJS_MKEY_A dat_8cecd86c[32] = {
#include "movie/cast/dat_8cecd86c.inc"
};
static NJS_MKEY_A dat_8cecda6c[29] = {
#include "movie/cast/dat_8cecda6c.inc"
};
static NJS_MKEY_A dat_8cecdc3c[31] = {
#include "movie/cast/dat_8cecdc3c.inc"
};
static NJS_MKEY_A dat_8cecde2c[32] = {
#include "movie/cast/dat_8cecde2c.inc"
};
static NJS_MKEY_A dat_8cece02c[2] = {
#include "movie/cast/dat_8cece02c.inc"
};
static NJS_MKEY_A dat_8cece04c[2] = {
#include "movie/cast/dat_8cece04c.inc"
};
static NJS_MKEY_A dat_8cece06c[2] = {
#include "movie/cast/dat_8cece06c.inc"
};
static NJS_MKEY_A dat_8cece08c[2] = {
#include "movie/cast/dat_8cece08c.inc"
};
static NJS_MDATA2 dat_8cece0ac[68] = {
#include "movie/cast/dat_8cece0ac.inc"
};
static NJS_MOTION dat_8cece4ec = {
#include "movie/cast/dat_8cece4ec.inc"
};
static NJS_ACTION cast_tails_action = {
#include "movie/cast/cast_tails_action.inc"
};
static struct event_path_point dat_8cece500[66] = {
#include "movie/cast/dat_8cece500.inc"
};
static struct event_path cast_solo_path = {
#include "movie/cast/cast_solo_path.inc"
};
static struct event_path_point dat_8cece928[58] = {
#include "movie/cast/dat_8cece928.inc"
};
static struct event_path cast_pair_first_path = {
#include "movie/cast/cast_pair_first_path.inc"
};
static struct event_path_point dat_8cececd0[36] = {
#include "movie/cast/dat_8cececd0.inc"
};
static struct event_path cast_pair_second_path = {
#include "movie/cast/cast_pair_second_path.inc"
};

/* clang-format off */
static struct cast_member cast_knuckles_members[1] = {
    {&knuckles_glide_action, 1.0F, &knuckles_texlist, &cast_solo_path,
     0.44F, 180, {0.0F, 0.0F, 0.0F}},
};
static struct cast_member cast_big_members[1] = {
    {&cast_big_action, 1.0F, &cast_big_texlist, &cast_solo_path,
     0.44F, 180, {0.0F, 0.0F, 0.0F}},
};
static struct cast_member cast_amy_tails_members[4] = {
    {&cast_amy_action, 1.0F, &amy_texlist, &cast_pair_first_path,
     0.56F, 170, {-30.0F, 45.0F, 0.0F}},
    {&cast_tails_action, 1.0F, &tails_texlist, &cast_pair_first_path,
     0.56F, 170, {-30.0F, 45.0F, 0.0F}},
    {&cast_amy_action, 1.0F, &amy_texlist, &cast_pair_second_path,
     0.49F, 260, {0.0F, 0.0F, 0.0F}},
    {&cast_tails_action, 1.0F, &tails_texlist, &cast_pair_second_path,
     0.49F, 260, {0.0F, 0.0F, 0.0F}},
};
/* clang-format on */
static struct cast cast_knuckles = {cast_knuckles_members, 1};
static struct cast cast_big = {cast_big_members, 1};
static struct cast cast_amy_tails = {cast_amy_tails_members, 4};

/* the black: a quad over the 640 by 480 screen, whose colors cast_draw sets
 * to cast_fade's alpha */
static NJS_POINT2 cast_cover_points[4] = {
#include "movie/cast/cast_cover_points.inc"
};
static NJS_COLOR cast_cover_colors[4] = {0};
static NJS_POINT2COL cast_cover = {
#include "movie/cast/cast_cover.inc"
};

int cast_fade;
static struct cast_member *cast_members;
static int cast_count;
static struct cast_state *cast_states;

/* count states from the heap, none active, each at its path's start */
static struct cast_state *cast_alloc_states(int count)
{
    struct cast_state *states;
    struct cast_state *state;
    int remaining;

    states = heap_alloc(count * sizeof(struct cast_state));
    state = states;
    for (remaining = count; remaining != 0; remaining--) {
        state->active = FALSE;
        state->path_frame = 0.0F;
        /* @hack spelling: state stepped at the body's end, which lets SHC
         * count the loop down with DT R5 at 0x8ceb0764; stepped in the
         * loop's head, 10 of 26 words differ */
        state++;
    }
    return states;
}

/* angle, in Angle units, plus a half and truncated toward 0, which rounds
 * an angle at or above 0 to the nearest */
/* @hack spelling: the rounding as an inline function, which keeps its add
 * apart from the multiply by ANGLE_PER_256TH (FMUL at 0x8ceb083e, then
 * FADD). written in place in cast_draw, each multiply and add fuse into an
 * FMAC, with both constants held in registers, and 280 of 304 words
 * differ */
#pragma inline(round_angle)
static int round_angle(Float angle)
{
    return (int)(angle + 0.5F);
}

void cast_draw(int frame)
{
    struct cast_member *member;
    struct cast_state *state;
    int count;
    int remaining;

    if (frame < 0) {
        return;
    }
    count = cast_count;
    member = cast_members;
    state = cast_states;
    for (remaining = count; remaining != 0; remaining--) {
        if (member->cue == frame) {
            state->active = TRUE;
            state->path_frame = 0.0F;
        }
        if (state->active) {
            Float step;
            Float path_frame;
            Float fraction;
            int points;
            int index;
            struct event_path_point *from;
            struct event_path_point *to;
            struct event_path *path;
            Float x;
            Float y;
            Float z;
            int ang_x;
            int ang_y;
            int ang_z;
            Float off_x;
            Float off_y;
            Float off_z;
            Float action_frame;
            Float action_speed;
            Float length;
            NJS_ACTION *action;
            NJS_TEXLIST *texlist;

            step = member->path_speed;
            path = member->path;
            points = path->count;
            path_frame = state->path_frame;
            index = (int)path_frame;
            fraction = path_frame - index;
            from = &path->points[index];
            to = from + 1;
            x = from->pos.x + (fraction * (to->pos.x - from->pos.x));
            y = from->pos.y + (fraction * (to->pos.y - from->pos.y));
            z = from->pos.z + (fraction * (to->pos.z - from->pos.z));
            /* each angle turns the short way round from one point to the
             * next, since its difference wraps to a Sint8 */
            ang_x = round_angle(
                (from->ang_x + (fraction * (Sint8)(to->ang_x - from->ang_x))) *
                ANGLE_PER_256TH);
            ang_y = round_angle(
                (from->ang_y + (fraction * (Sint8)(to->ang_y - from->ang_y))) *
                ANGLE_PER_256TH);
            ang_z = round_angle(
                (from->ang_z + (fraction * (Sint8)(to->ang_z - from->ang_z))) *
                ANGLE_PER_256TH);
            off_x = member->offset.x;
            off_y = member->offset.y;
            off_z = member->offset.z;
            /* @bug uninitialized: a member's first draw poses its action at
             * the frame the heap left (FMOV.S @(R0,R13),FR15 at 0x8ceb08a4,
             * R0 8). cast_alloc_states stores only active and path_frame
             * (0x8ceb0762, 0x8ceb0766), and so does the cue above */
            action_frame = state->action_frame;
            action_speed = member->action_speed;
            action = member->action;
            length = action->motion->nbFrame;
            texlist = member->texlist;
            njPushMatrix(NULL);
            njTranslate(NULL, x, y, z);
            njRotateXYZ(NULL, ang_x, ang_y, ang_z);
            njTranslate(NULL, off_x, off_y, off_z);
            njSetTexture(texlist);
            njSimpleAction(action, action_frame);
            njPopMatrix(1);
            action_frame += action_speed;
            if (action_frame >= length) {
                action_frame -= length;
            }
            path_frame += step;
            state->action_frame = action_frame;
            state->path_frame = path_frame;
            if (path_frame >= points - 1) {
                state->active = FALSE;
            }
        }
        member++;
        state++;
    }
    if (cast_fade < 0) {
        njDrawPolygon2D(&cast_cover, 4, -1.0F, NJD_FILL);
    } else if (cast_fade != 0) {
        Uint32 color = (Uint32)cast_fade << 24;
        NJS_COLOR *colors = cast_cover_colors;

        colors[0].color = colors[1].color = colors[2].color = colors[3].color =
            color;
        njDrawPolygon2D(&cast_cover, 4, -1.0F, NJD_FILL | NJD_TRANSPARENT);
    }
}

static void cast_bind(struct cast *cast)
{
    cast_members = cast->members;
    cast_count = cast->count;
    cast_states = cast_alloc_states(cast->count);
}

void cast_load(void)
{
    int character;

    njSetPerspective(NJM_DEG_ANG(70));
    njRotateViewYAbsolute(&view, VIEW_TURN);
    njTranslateViewAbsolute(&view, VIEW_X, VIEW_Y, VIEW_Z);
    njSetView(&view);
    character = get_character();
    if (character == CHARACTER_KNUCKLES) {
        texture_load_prs_file("KNUCKLES.PRS", &knuckles_texlist);
        cast_bind(&cast_knuckles);
    } else if (character == CHARACTER_BIG) {
        texture_load_prs_file("EV_TR2_BIG.PRS", &cast_big_texlist);
        cast_bind(&cast_big);
    } else {
        texture_load_prs_file("AMY.PRS", &amy_texlist);
        texture_load_prs_file("MILES.PRS", &tails_texlist);
        cast_bind(&cast_amy_tails);
    }
}

void cast_free(void)
{
    /* @hack redundant: a loop of one pass round the call, which keeps it a
     * JSR (0x8ceb0b66) ahead of the function's own return; as a plain call,
     * SHC makes it a tail jump and 8 of 8 words differ */
    do {
        heap_free(cast_states);
    } while (0);
}
