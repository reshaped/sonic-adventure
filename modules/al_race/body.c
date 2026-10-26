/* a racing Chao's heading and lean, the reset of its racer's gait and
 * voice, and the drawing of its body, its shadow and its name */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "al_main/chao_name.h"
#include "al_main/physics.h"
#include "al_main/shape.h"
#include "al_race/game.h"
#include "al_race/race_chao.h"
#include "al_race/racer.h"
#include "collision/polygon.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/models.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

static void race_chao_draw_name(struct task_work *work, struct alife_chao *chao,
                                Float scale, NJS_COLOR *color);
static void race_chao_draw_shadow(struct task_work *work,
                                  struct alife_chao *chao);

void race_chao_face_target(struct task_work *work, struct alife_chao *chao)
{
    work->ang.y = TARGET_HEADING(work, chao);
}

int race_chao_angle_to_target(struct task *task, Angle *angle)
{
    struct task_work *work;
    struct alife_chao *chao;
    Angle heading;
    Angle turn;

    if (!task) {
        return 0;
    }
    work = task->work;
    chao = (struct alife_chao *)task->motion;
    heading = TARGET_HEADING(work, chao);
    turn = work->ang.y - heading;
    /* @hack spelling: turn's low 16 bits sign-extended by hand, which SHC tests
     * and ORs (TST R4,R1 at 0x0cb81480), as the image does. as (Sint16)turn,
     * SHC writes EXTS.W, and 20 of 48 words differ */
    *angle = turn & 0x8000 ? turn | 0xffff0000 : (Uint16)turn;
    return 1;
}

static void race_chao_draw_name(struct task_work *work, struct alife_chao *chao,
                                Float scale, NJS_COLOR *color)
{
    NJS_POINT3 pos;

    /* @hack spelling: pos stored through &pos, whose address SHC works out
     * again for each store (0x0cb814c6 to 0x0cb814e8), as the image does. as
     * pos.x, pos.y and pos.z, 35 of 38 words differ */
    (&pos)->x = work->pos.x;
    (&pos)->y = work->pos.y;
    (&pos)->z = work->pos.z;
    pos.y += 6.5F;
    chao_name_draw_at(chao->record, &pos, scale, color);
}

void race_chao_clear_gait(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;

    racer->gait = GAIT_FOOT;
    racer->voice = 0;
}

void race_chao_lean(struct task_work *work, struct alife_chao *chao)
{
    struct sides_query *query = &chao->terrain.query;

    if ((work->flags & CHAO_TILT) && query->sides[SIDE_BELOW].found) {
        work->ang.x =
            angle_approach(work->ang.x, query->sides[SIDE_BELOW].ang_x, 364);
        work->ang.z =
            angle_approach(work->ang.z, query->sides[SIDE_BELOW].ang_z, 364);
    } else {
        work->ang.x = angle_approach(work->ang.x, 0, 364);
        work->ang.z = angle_approach(work->ang.z, 0, 182);
    }
}

/* draw the Chao's shadow on the ground below it, or nothing without
 * ground. the shadow's scale falls from 0.5 to 0.2 as the Chao rises to
 * 1 / 0.07 above the ground. over that rise, the shadow's constant material
 * alpha offset falls from 1 to -1, and at -1 the shadow is clear. while the
 * Chao swims or has water below, the offset is -0.8 */
static void race_chao_draw_shadow(struct task_work *work,
                                  struct alife_chao *chao)
{
    struct sides_query *query = &chao->terrain.query;
    Float rise;
    Float alpha;
    Float scale;
    /* @hack spelling: fade, alpha's factor, which SHC multiplies and adds by
     * FMAC (0x0cb81626), as the image does. in the expression, SHC writes
     * FMUL and FSUB, and 82 of 128 words differ */
    Float fade;
    /* @hack spelling: shrink, scale's factor, which SHC multiplies and adds by
     * FMAC (0x0cb81636), as the image does. in the expression, SHC writes
     * FMUL and FSUB, and 3 of 128 words differ */
    Float shrink;

    if (!query->sides[SIDE_BELOW].found) {
        return;
    }
    rise = (work->pos.y - query->sides[SIDE_BELOW].level) * 0.07F;
    if (rise >= 1.0F) {
        rise = 1.0F;
    } else if (rise <= 0.0F) {
        rise = 0.0F;
    }
    if (work->mode == MODE_SWIM ||
        (chao->terrain.flags & TERRAIN_WATER_BELOW)) {
        alpha = -0.8F;
    } else {
        fade = -1.0F - 1.0F;
        alpha = 1.0F + (fade * rise);
    }
    shrink = 0.2F - 0.5F;
    scale = 0.5F + (shrink * rise);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, query->sides[SIDE_BELOW].level + 0.1F,
                work->pos.z);
    njRotateXYZ(NULL, query->sides[SIDE_BELOW].ang_x, 0,
                query->sides[SIDE_BELOW].ang_z);
    texture_set_regular();
    njScale(NULL, scale, scale, scale);
    draw_set_material(alpha, -1.0F, -1.0F, -1.0F);
    draw_tree(shadow_object);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPopMatrix(1);
}

void race_chao_show(struct task_work *work, struct alife_chao *chao)
{
    struct racer *racer = RACER_VARS(work)->racer;
    NJS_COLOR color;
    int kind;

    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    if (racer->flags & RACER_SHOWN) {
        alife_chao_show(work, chao);
        race_chao_draw_shadow(work, chao);
    }
    if (racer->flags & RACER_NAMED) {
        color.argb.a = 255;
        color.argb.r = 0;
        color.argb.g = 0;
        /* a rival's name shows blue, and any other black */
        if (race_get_entry_kind(&kind, racer->lane)) {
            if (kind == ENTRY_RIVAL) {
                color.argb.b = 250;
            } else {
                color.argb.b = 0;
            }
        } else {
            color.argb.b = 0;
        }
        race_chao_draw_name(work, chao, 0.85F, &color);
        racer->flags &= ~RACER_NAMED;
    }
}
