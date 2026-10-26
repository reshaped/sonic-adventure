/* drawing helpers: a model tree with its own draws for some nodes, the
 * hud's big digits and a line, and measures of points, spheres and angles */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/helpers.h"
#include "draw/model.h"
#include "hud/hud.h"
#include "system/task.h"
#include "system/util.h"

/* the colon's sprite in time_digit_texanims, after the ten digits' */
#define COLON 10

static NJS_TEXANIM time_digit_texanims[COLON + 1] = {
#include "draw/helpers/time_digit_texanims.inc"
};

void draw_object(NJS_OBJECT *object, const struct draw_override *overrides)
{
    Angle ang;

    njPushMatrix(NULL);
    if (object->model != NULL) {
        int i;

        for (i = 0; overrides[i].object != NULL; i++) {
            if (overrides[i].object == object) {
                break;
            }
        }
        if (overrides[i].object == NULL) {
            njTranslateV(NULL, (NJS_VECTOR *)object->pos);
            TURN_VIA(njRotateZ, object->ang[2], ang);
            TURN_VIA(njRotateX, object->ang[0], ang);
            TURN_VIA(njRotateY, object->ang[1], ang);
            njScaleV(NULL, (NJS_VECTOR *)object->scl);
            draw_model(object->model);
        } else if (overrides[i].draw != NULL) {
            overrides[i].draw(object);
        }
    } else {
        njTranslateV(NULL, (NJS_VECTOR *)object->pos);
        TURN_VIA(njRotateZ, object->ang[2], ang);
        TURN_VIA(njRotateX, object->ang[0], ang);
        TURN_VIA(njRotateY, object->ang[1], ang);
        njScaleV(NULL, (NJS_VECTOR *)object->scl);
    }
    if (object->child != NULL) {
        njPushMatrix(NULL);
        draw_object(object->child, overrides);
        njPopMatrix(1);
    }
    njPopMatrix(1);
    if (object->sibling != NULL) {
        draw_object(object->sibling, overrides);
    }
}

void point_toward(NJS_POINT3 *start, NJS_POINT3 *target, Float distance,
                  NJS_POINT3 *out)
{
    NJS_VECTOR way;

    way.x = target->x - start->x;
    way.y = target->y - start->y;
    way.z = target->z - start->z;
    if (njUnitVector(&way) == 0.0F) {
        *out = *start;
    } else {
        way.x *= distance;
        way.y *= distance;
        way.z *= distance;
        way.x += start->x;
        way.y += start->y;
        way.z += start->z;
        *out = way;
    }
}

Bool spheres_overlap(NJS_POINT3 *center1, NJS_POINT3 *center2, Float radius1,
                     Float radius2)
{
    NJS_SPHERE spheres[2];

    spheres[0].c.x = center1->x;
    spheres[0].c.y = center1->y;
    spheres[0].c.z = center1->z;
    spheres[0].r = radius1;
    spheres[1].c.x = center2->x;
    spheres[1].c.y = center2->y;
    spheres[1].c.z = center2->z;
    spheres[1].r = radius2;
    return njCollisionCheckSS(&spheres[0], &spheres[1]);
}

Angle angle_forward_distance(Angle angle, Angle target)
{
    angle &= 0xffff;
    target &= 0xffff;
    target |= 0x10000;
    target -= angle;
    return target & 0xffff;
}

Angle angle_shortest_distance(Angle angle, Angle target)
{
    Angle distance = angle_forward_distance(angle, target);

    if (distance > 0x8000) {
        distance = 0x10000 - distance;
    }
    return distance;
}

Angle angle_turn_toward(Angle angle, Angle target, Uint32 delta)
{
    Angle distance;

    angle &= 0xffff;
    target &= 0xffff;
    if (angle == target) {
        return angle;
    }
    distance = angle_forward_distance(angle, target);
    if (distance > 0x8000) {
        angle = (angle - delta) & 0xffff;
        if (angle_forward_distance(angle, target) < 0x8000) {
            angle = target;
        }
    } else {
        angle = (angle + delta) & 0xffff;
        if (angle_forward_distance(angle, target) > 0x8000) {
            angle = target;
        }
    }
    return angle & 0xffff;
}

/* draw value's last digits, as many as digits says and five at most, each
 * sprite at its own depth. value is clamped to 0 through 99999, and then to
 * one digit more than it draws. with two digits, 150 draws as 50, and 1500
 * as 99 */
static void hud_draw_digits(struct hud_layout *layout, int value, int digits)
{
    NJS_SPRITE sprite;
    int limit;
    int i;
    int digit;
    /* @hack spelling: at, places and number, copies of the parameters, which
     * put digits' MOV R6,R12 ahead of value's MOV R5,R13 (0x8c05a7b4,
     * 0x8c05a7b8); without them the two trade and 2 of 212 words differ */
    struct hud_layout *at = layout;
    int places = digits;
    int number = value;

    njSetTexture(&hud_digit_texlist);
    sprite.tlist = &hud_digit_texlist;
    sprite.tanim = time_digit_texanims;
    sprite.sx = at->scale.x;
    sprite.sy = at->scale.y;
    sprite.ang = 0;
    sprite.p.x = at->pos.x;
    sprite.p.y = at->pos.y;
    if (number >= 100000) {
        number = 99999;
    }
    if (number <= 0) {
        number = 0;
    }
    limit = 10;
    for (i = 0; i < places; i++) {
        limit *= 10;
    }
    limit--;
    if (number > limit) {
        number = limit;
    }
    if (places >= 5) {
        /* @hack spelling: a comma after each digit's assignment, which stores
         * the digit at @R15 and loads it back into R5 for the call, as at
         * 0x8c05a866 and 0x8c05a86a; with semicolons it goes straight to R5,
         * 184 of 212 words differ and the code runs 32 bytes short */
        digit = number / 10000,
        njDrawSprite2D(&sprite, digit, -10.0F, NJD_SPRITE_ALPHA);
        sprite.p.x += at->spacing.x;
        sprite.p.y += at->spacing.y;
    }
    if (places >= 4) {
        digit = number % 10000 / 1000,
        njDrawSprite2D(&sprite, digit, -10.001F, NJD_SPRITE_ALPHA);
        sprite.p.x += at->spacing.x;
        sprite.p.y += at->spacing.y;
    }
    if (places >= 3) {
        digit = number % 1000 / 100,
        njDrawSprite2D(&sprite, digit, -10.002F, NJD_SPRITE_ALPHA);
        sprite.p.x += at->spacing.x;
        sprite.p.y += at->spacing.y;
    }
    if (places >= 2) {
        digit = number % 100 / 10,
        njDrawSprite2D(&sprite, digit, -10.003F, NJD_SPRITE_ALPHA);
        sprite.p.x += at->spacing.x;
        sprite.p.y += at->spacing.y;
    }
    if (places >= 1) {
        digit = number % 10,
        njDrawSprite2D(&sprite, digit, -10.004F, NJD_SPRITE_ALPHA);
    }
}

void hud_draw_time(struct hud_layout *layout, int minutes, int seconds)
{
    struct hud_layout next;
    NJS_SPRITE sprite;

    next = *layout;
    hud_draw_digits(&next, minutes, 2);
    next.pos.x += layout->spacing.x * 2.0F;
    njSetTexture(&hud_digit_texlist);
    sprite.tlist = &hud_digit_texlist;
    sprite.tanim = time_digit_texanims;
    sprite.sx = next.scale.x;
    sprite.sy = next.scale.y;
    sprite.ang = 0;
    sprite.p.x = next.pos.x;
    sprite.p.y = next.pos.y;
    njDrawSprite2D(&sprite, COLON, -10.0F, NJD_SPRITE_ALPHA);
    next.pos.x += layout->spacing.x;
    hud_draw_digits(&next, seconds, 2);
}

void draw_line_facing(struct angles *ang, NJS_POINT3 *pos, Float length)
{
    NJS_VECTOR way;
    NJS_POINT3 end;

    way.x = 0.0F;
    way.y = 0.0F;
    way.z = length;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njRotateXYZ(NULL, ang->x, ang->y, ang->z);
    njCalcVector(NULL, &way, &end);
    end.x += pos->x;
    end.y += pos->y;
    end.z += pos->z;
    njPopMatrix(1);
    draw_line(pos->x, pos->y, pos->z, end.x, end.y, end.z);
}
