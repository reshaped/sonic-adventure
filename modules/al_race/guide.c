/* the behaviors race_guide_racers gives the racers: toward their lanes'
 * race_course targets, and for the curious, to their items */
#include <ninja.h>

#include "al_race/course.h"
#include "al_race/deal.h"
#include "al_race/game.h"
#include "al_race/racer.h"

/* a guiding behavior's states */
enum guide_state { GUIDE_GOING, GUIDE_DONE };

/* how near a racer's Chao must come to its target, and the steps it has
 * to get there */
#define TARGET_RADIUS 10.0F
#define TARGET_STEPS 1800
/* in kinds 0 and 1, a racer this far along the course stops going for its
 * target */
#define TARGET_DISTANCE 216.0F
/* the same for the item dealt to a curious racer's lane */
#define ITEM_RADIUS 1.0F
#define ITEM_STEPS 480

static void racer_to_target(struct racer *racer)
{
    struct racer_behavior *behavior = &racer->behavior;
    NJS_POINT3 *target = &race_course.targets[racer->lane];

    switch (behavior->state) {
    case GUIDE_GOING:
        if (racer_go_to(racer, target, TARGET_RADIUS, TARGET_STEPS) ||
            racer->distance > TARGET_DISTANCE) {
            ++behavior->state;
            behavior->timer = 0;
        }
        break;
    case GUIDE_DONE:
        break;
    }
}

static void racer_to_target_4(struct racer *racer)
{
    struct racer_behavior *behavior = &racer->behavior;
    NJS_POINT3 *target = &race_course.targets[racer->lane];

    switch (behavior->state) {
    case GUIDE_GOING:
        if (racer_go_to(racer, target, TARGET_RADIUS, TARGET_STEPS)) {
            ++behavior->state;
            behavior->timer = 0;
        }
        break;
    case GUIDE_DONE:
        break;
    }
}

static void racer_to_item(struct racer *racer)
{
    struct racer_behavior *behavior = &racer->behavior;
    NJS_POINT3 item;
    int result;

    switch (behavior->state) {
    case GUIDE_GOING:
        if (deal_take(&item, racer->lane) != 1) {
            behavior->state = GUIDE_DONE;
            behavior->timer = 0;
            return;
        } else {
            result = racer_go_to(racer, &item, ITEM_RADIUS, ITEM_STEPS);
            switch (result) {
            case 0:
                break;
            default:
                behavior->state = GUIDE_DONE;
                behavior->timer = 0;
                return;
            }
        }
        break;
    case GUIDE_DONE:
        racer->flags &= ~RACER_CURIOUS;
        racer->route = ROUTE_TARGET;
        racer_clear_behavior(behavior);
        break;
    }
}

void race_guide_racers(struct race_game *game)
{
    struct racer *racer;
    int i;

    for (i = 0; i < RACE_LANES; i++) {
        racer = &game->racers[i];
        switch (racer->route) {
        case ROUTE_START:
            if (game->kind == 0 || game->kind == 1) {
                racer_set_behavior(&racer->behavior, racer_to_target, 1);
            } else if (game->kind == RACE_KIND_LAST) {
                racer_set_behavior(&racer->behavior, racer_to_target_4, 1);
            }
            racer->route = ROUTE_TARGET;
            break;
        case ROUTE_TARGET:
            if (racer->in_zone == 1 && (racer->flags & RACER_CURIOUS)) {
                racer_override_behavior(&racer->behavior, racer_to_item);
                racer->route = ROUTE_ITEM;
            }
            break;
        case ROUTE_ITEM:
            break;
        }
    }
}
