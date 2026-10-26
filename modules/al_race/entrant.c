/* the Chao race's entrants: their places, on the chao window's board and at
 * the hall's gates, and the Chao that take them */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "al_main/models.h"
#include "al_race/entrant.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "system/task.h"

#define VIEW_RADIUS 10.0F /* the Chao's radius in the view test */
#define EGG_RISE 0.15F /* the egg's height above the place */
#define EGG_SCALE 0.75F

/* the height above the place of the point entrant_screen_pos projects */
#define SCREEN_RISE 2.5F

struct entrant *entrant_init(struct entrant *entrant, NJS_POINT3 *pos)
{
    entrant->flags |= ENTRANT_HELD;
    if (entrant->flags & ENTRANT_CHAO) {
        entrant->flags &= ~ENTRANT_CHAO;
        entrant->flags |= ENTRANT_READY;
    } else {
        entrant->chao = NULL;
        entrant->work = NULL;
    }
    entrant_reset(entrant, pos);
    return entrant;
}

void entrant_set_chao(struct entrant *entrant, struct task_work *work,
                      struct alife_chao *chao)
{
    entrant->flags |= ENTRANT_CHAO;
    entrant->chao = chao;
    entrant->work = work;
    if (entrant->flags & ENTRANT_HELD) {
        entrant->flags |= ENTRANT_READY;
    }
}

Bool entrant_ready(struct entrant *entrant)
{
    if (entrant->flags & ENTRANT_READY) {
        return TRUE;
    }
    return FALSE;
}

void entrant_clear(struct entrant *entrant)
{
    entrant->flags &= ~ENTRANT_HELD;
    entrant->flags &= ~ENTRANT_CHAO;
    entrant->flags &= ~ENTRANT_READY;
    entrant->flags &= ~ENTRANT_SHOW;
    entrant->flags &= ~ENTRANT_NAME;
    entrant->flags &= ~ENTRANT_EGG;
    entrant->flags &= ~ENTRANT_BEGG;
    entrant->flags &= ~ENTRANT_PAL;
    entrant->flags &= ~ENTRANT_GOLD;
    entrant->chao = NULL;
    entrant->work = NULL;
}

void entrant_remove(struct entrant *entrant)
{
    if (entrant->flags & ENTRANT_READY) {
        task_destroy(entrant->chao->task);
    }
    entrant_clear(entrant);
}

void entrant_reset(struct entrant *entrant, NJS_POINT3 *pos)
{
    entrant->command = 0;
    entrant->state = ENTRANT_RESET;
    entrant->pos = *pos;
}

void entrant_step(struct entrant *entrant)
{
    struct task_work *work;

    if (entrant_shows_egg(entrant)) {
        njSetTexture(&alife_chao_texlist);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPushMatrix(NULL);
        njTranslateV(NULL, &entrant->pos);
        njTranslate(NULL, 0.0F, EGG_RISE, 0.0F);
        njRotateY(NULL, 0x4000);
        njScale(NULL, EGG_SCALE, EGG_SCALE, EGG_SCALE);
        if (entrant->flags & ENTRANT_GOLD) {
            draw_tree(&alife_gold_egg_object);
        } else if (entrant->flags & ENTRANT_PAL) {
            draw_tree(&alife_pal_egg_object);
        } else if (entrant->flags & ENTRANT_BEGG) {
            draw_tree(&alife_begg_egg_object);
        } else {
            draw_tree(&alife_egg_object);
        }
        njPopMatrix(1);
    }
    if (entrant->flags & ENTRANT_READY) {
        work = entrant->work;
        if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, VIEW_RADIUS)) {
            entrant->flags &= ~ENTRANT_SHOW;
            switch (entrant->state) {
            case ENTRANT_RESET:
                entrant->state = ENTRANT_RESET_TOLD;
                entrant->command = ENTRANT_TELL_RESET;
                /* fallthrough */
            case ENTRANT_RESET_TOLD:
                break;
            case ENTRANT_BOARD:
                work->pos = entrant->pos;
                entrant->state = ENTRANT_BOARD_TOLD;
                entrant->command = ENTRANT_TELL_BOARD;
                /* fallthrough */
            case ENTRANT_BOARD_TOLD:
                entrant->flags |= ENTRANT_SHOW;
                break;
            case ENTRANT_CURSOR:
                work->pos = entrant->pos;
                entrant->state = ENTRANT_CURSOR_TOLD;
                entrant->command = ENTRANT_TELL_CURSOR;
                /* fallthrough */
            case ENTRANT_CURSOR_TOLD:
                entrant->flags |= ENTRANT_SHOW;
                break;
            case ENTRANT_ENTERED:
                work->pos = entrant->pos;
                entrant->state = ENTRANT_ENTERED_TOLD;
                entrant->command = ENTRANT_TELL_ENTERED;
                /* fallthrough */
            case ENTRANT_ENTERED_TOLD:
                entrant->flags |= ENTRANT_SHOW;
                break;
            case ENTRANT_GATES:
                entrant->state = ENTRANT_GATES_TOLD;
                entrant->command = ENTRANT_TELL_GATES;
                /* fallthrough */
            case ENTRANT_GATES_TOLD:
                entrant->flags |= ENTRANT_SHOW;
                break;
            }
        }
    }
}

Bool entrant_entered(struct entrant *entrant)
{
    switch (entrant->state) {
    case ENTRANT_ENTERED:
    case ENTRANT_ENTERED_TOLD:
        return TRUE;
    }
    return FALSE;
}

Bool entrant_at_cursor(struct entrant *entrant)
{
    switch (entrant->state) {
    case ENTRANT_CURSOR:
    case ENTRANT_CURSOR_TOLD:
        return TRUE;
    }
    return FALSE;
}

Bool entrant_shows_egg(struct entrant *entrant)
{
    if (entrant->flags & ENTRANT_EGG || entrant->flags & ENTRANT_BEGG ||
        entrant->flags & ENTRANT_PAL || entrant->flags & ENTRANT_GOLD) {
        return TRUE;
    }
    return FALSE;
}

Bool entrant_take_command(struct entrant *entrant, int command)
{
    if (entrant->command == command) {
        entrant->command = 0;
        return TRUE;
    }
    return FALSE;
}

void entrant_screen_pos(struct entrant *entrant, NJS_POINT3 *screen)
{
    NJS_POINT3 pos;
    NJS_POINT2 at;

    /* @hack spelling: pos stored through &pos, whose address SHC works out
     * again for each store (0x0cb8af48 to 0x0cb8af6c), as the image does. as
     * pos.x, pos.y and pos.z, 39 of 47 words differ */
    (&pos)->x = entrant->pos.x;
    (&pos)->y = entrant->pos.y;
    (&pos)->z = entrant->pos.z;
    pos.y += SCREEN_RISE;
    njProjectScreen(NULL, &pos, &at);
    screen->x = at.x;
    screen->y = at.y;
    screen->z = 0.0F;
}
