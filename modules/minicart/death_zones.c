/* the zones in which a player dies, for each act */
#include "minicart/cart.h"
#include "minicart/death_zones.h"
#include "player/player.h"

extern struct death_zone cart_death_zone_list[];

#pragma section HEAD
struct death_zone *cart_death_zones[CART_ACTS] = {
    cart_death_zone_list, cart_death_zone_list, cart_death_zone_list,
    cart_death_zone_list, cart_death_zone_list, cart_death_zone_list,
};
#pragma section
