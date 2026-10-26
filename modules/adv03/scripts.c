/* the scripts of seven of the past's story sections, in HEAD.
 * script_amy_tikal.c and script_last_angry.c define the past's other two */
#include "adv03/scripts.h"
#include "adv03/script_big_emerald.h"
#include "adv03/script_e102_chaos.h"
#include "adv03/script_knuckles_chaos.h"
#include "adv03/script_knuckles_fire.h"
#include "adv03/script_knuckles_tikal.h"
#include "adv03/script_tails_tikal.h"
#include "adv03/script_sonic_fire.h"
#include "progress/story.h"

#pragma section HEAD
struct story_script script_sonic_fire = {script_sonic_fire_init,
                                         script_sonic_fire_step};
struct story_script script_tails_tikal = {script_tails_tikal_init,
                                          script_tails_tikal_step};
struct story_script script_knuckles_tikal = {script_knuckles_tikal_init,
                                             script_knuckles_tikal_step};
struct story_script script_knuckles_chaos = {script_knuckles_chaos_init,
                                             script_knuckles_chaos_step};
struct story_script script_knuckles_fire = {script_knuckles_fire_init,
                                            script_knuckles_fire_step};
struct story_script script_e102_chaos = {script_e102_chaos_init,
                                         script_e102_chaos_step};
struct story_script script_big_emerald = {script_big_emerald_init,
                                          script_big_emerald_step};
#pragma section
