/* land textures and night lights for the story's time of day */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/land_time.h"
#include "adv00/station_square.h"
#include "progress/progress.h"
#include "stage/stage.h"

/* the land objects whose materials change with the time of day */
extern NJS_OBJECT dat_0c920a1c;
extern NJS_OBJECT dat_0c921b20;
extern NJS_OBJECT dat_0c922314;
extern NJS_OBJECT dat_0c925ea4;
extern NJS_OBJECT dat_0c927a20;
extern NJS_OBJECT dat_0c92ad70;
extern NJS_OBJECT dat_0c92c6f4;
extern NJS_OBJECT dat_0c92d4b0;
extern NJS_OBJECT dat_0c92ed3c;
extern NJS_OBJECT dat_0c93088c;
extern NJS_OBJECT dat_0c93156c;
extern NJS_OBJECT dat_0c932428;
extern NJS_OBJECT dat_0c932dbc;
extern NJS_OBJECT dat_0c93bf50;
extern NJS_OBJECT dat_0c93c340;
extern NJS_OBJECT dat_0c93c730;
extern NJS_OBJECT dat_0c93cb20;
extern NJS_OBJECT dat_0c93cf10;
extern NJS_OBJECT dat_0c93d300;
extern NJS_OBJECT dat_0c93e024;
extern NJS_OBJECT dat_0c93ed48;
extern NJS_OBJECT dat_0c93f56c;
extern NJS_OBJECT dat_0c93fd90;
extern NJS_OBJECT dat_0c943134;
extern NJS_OBJECT dat_0c943d20;
extern NJS_OBJECT dat_0c9477d4;
extern NJS_OBJECT dat_0c94cbb0;
extern NJS_OBJECT dat_0c94e1b4;
extern NJS_OBJECT dat_0c95150c;
extern NJS_OBJECT dat_0c9530e4;
extern NJS_OBJECT dat_0c9569a8;
extern NJS_OBJECT dat_0c95a994;
extern NJS_OBJECT dat_0c95d250;
extern NJS_OBJECT dat_0c95dd28;
extern NJS_OBJECT dat_0c9626dc;
extern NJS_OBJECT dat_0c963630;
extern NJS_OBJECT dat_0c9642d4;
extern NJS_OBJECT dat_0c964450;
extern NJS_OBJECT dat_0c9645d8;
extern NJS_OBJECT dat_0c96526c;
extern NJS_OBJECT dat_0c967c18;
extern NJS_OBJECT dat_0c9b679c;
extern NJS_OBJECT dat_0c9df304;
extern NJS_OBJECT dat_0c9e4b7c;
extern NJS_OBJECT dat_0c9e611c;
extern NJS_OBJECT dat_0c9f04c8;
extern NJS_OBJECT dat_0c9f0608;
extern NJS_OBJECT dat_0c9f0748;
extern NJS_OBJECT dat_0c9f11c0;
extern NJS_OBJECT dat_0c9f1d68;
extern NJS_OBJECT dat_0c9f4d0c;
extern NJS_OBJECT dat_0c9f4f84;
extern NJS_OBJECT dat_0c9f64c8;
extern NJS_OBJECT dat_0c9f74b0;
extern NJS_OBJECT dat_0c9f84d4;
extern NJS_OBJECT dat_0c9f9d60;
extern NJS_OBJECT dat_0c9fb3e4;
extern NJS_OBJECT dat_0c9fc8a0;
extern NJS_OBJECT dat_0c9fcde4;
extern NJS_OBJECT dat_0c9fe538;
extern NJS_OBJECT dat_0c9ffe18;
extern NJS_OBJECT dat_0ca02510;
extern NJS_OBJECT dat_0ca04b90;
extern NJS_OBJECT dat_0ca05a00;
extern NJS_OBJECT dat_0ca065b8;
extern NJS_OBJECT dat_0ca0a100;
extern NJS_OBJECT dat_0ca0a2f4;
extern NJS_OBJECT dat_0ca0a860;
extern NJS_OBJECT dat_0ca0ae64;
extern NJS_OBJECT dat_0ca0c518;
extern NJS_OBJECT dat_0ca13d28;
extern NJS_OBJECT dat_0ca16f90;
extern NJS_OBJECT dat_0ca170dc;
extern NJS_OBJECT dat_0ca1c450;
extern NJS_OBJECT dat_0ca1ce7c;
extern NJS_OBJECT dat_0ca1f660;
extern NJS_OBJECT dat_0ca1fab8;
extern NJS_OBJECT dat_0ca218d0;
extern NJS_OBJECT dat_0ca258d4;
extern NJS_OBJECT dat_0ca25f9c;
extern NJS_OBJECT dat_0ca267d0;
extern NJS_OBJECT dat_0ca272b4;
extern NJS_OBJECT dat_0ca28f18;
extern NJS_OBJECT dat_0ca2a3a8;
extern NJS_OBJECT dat_0ca2a820;
extern NJS_OBJECT dat_0ca2ad70;
extern NJS_OBJECT dat_0ca2b6c0;
extern NJS_OBJECT dat_0ca2cd6c;
extern NJS_OBJECT dat_0ca52a58;
extern NJS_OBJECT dat_0ca52c10;

#define LENGTH(array) ((int)(sizeof(array) / sizeof((array)[0])))

struct time_texture {
    NJS_OBJECT *object;
    int material;
    /* the material's texture at each time of day, TIME_DAY to TIME_NIGHT */
    Uint32 textures[3];
};

struct time_texture_table {
    int count;
    struct time_texture *textures;
};

/* every material in object's tree that uses texture lights up at night */
struct night_light {
    NJS_OBJECT *object;
    Uint32 texture;
};

static struct time_texture office_time_textures[16] = {
#include "adv00/land_time/office_time_textures.inc"
};
static struct time_texture_table office_time_table = {
    LENGTH(office_time_textures), office_time_textures};

static struct time_texture casino_time_textures[1] = {
#include "adv00/land_time/casino_time_textures.inc"
};
static struct time_texture_table casino_time_table = {
    LENGTH(casino_time_textures), casino_time_textures};

static struct time_texture station_time_textures[5] = {
#include "adv00/land_time/station_time_textures.inc"
};
static struct time_texture_table station_time_table = {
    LENGTH(station_time_textures), station_time_textures};

static struct time_texture hotel_time_textures[2] = {
#include "adv00/land_time/hotel_time_textures.inc"
};
static struct time_texture_table hotel_time_table = {
    LENGTH(hotel_time_textures), hotel_time_textures};

static struct night_light office_night_lights[69] = {
#include "adv00/land_time/office_night_lights.inc"
};

static struct night_light station_night_lights[59] = {
#include "adv00/land_time/station_night_lights.inc"
};

/* turn a light on: each material in object's tree that uses texture ignores
 * the scene's lighting */
static void land_light_on(NJS_OBJECT *object, Uint32 texture)
{
    Uint16 i;
    Uint16 count = object->model->nbMat;
    NJS_MATERIAL *mats = object->model->mats;

    for (i = 0; i < count; i++) {
        if (mats[i].attr_texId == texture) {
            mats[i].attrflags |= NJD_FLAG_IGNORE_LIGHT;
        }
    }
    if (object->child) {
        land_light_on(object->child, texture);
    }
    if (object->sibling) {
        land_light_on(object->sibling, texture);
    }
}

/* turn a light off: each material in object's tree that uses texture takes
 * the scene's lighting again */
static void land_light_off(NJS_OBJECT *object, Uint32 texture)
{
    Uint16 i;
    Uint16 count = object->model->nbMat;
    NJS_MATERIAL *mats = object->model->mats;

    for (i = 0; i < count; i++) {
        if (mats[i].attr_texId == texture) {
            mats[i].attrflags &= ~NJD_FLAG_IGNORE_LIGHT;
        }
    }
    if (object->child) {
        land_light_off(object->child, texture);
    }
    if (object->sibling) {
        land_light_off(object->sibling, texture);
    }
}

/* turn the lights off by day and in the evening, and on at night */
static void land_lights_set(int count, struct night_light *lights, int time)
{
    int i;

    if (time <= TIME_EVENING) {
        for (i = 0; i < count; i++) {
            land_light_off(lights[i].object, lights[i].texture);
        }
    } else {
        for (i = 0; i < count; i++) {
            land_light_on(lights[i].object, lights[i].texture);
        }
    }
}

static void land_textures_set(struct time_texture_table *table, int time)
{
    int i;
    int count = table->count;
    struct time_texture *textures = table->textures;

    for (i = 0; i < count; i++) {
        textures[i].object->model->mats[textures[i].material].attr_texId =
            textures[i].textures[time];
    }
}

void land_time_set(void)
{
    int time;
    Uint8 act;

    switch (story_get_time()) {
    default:
        return;
    case TIME_DAY:
        time = TIME_DAY;
        break;
    case TIME_EVENING:
        time = TIME_EVENING;
        break;
    case TIME_NIGHT:
        /* no break ends the last case. with one, stage_get_stage_act's
         * address takes R3, not R2 (0x0c90f61a), and 2 of 65 words differ */
        time = TIME_NIGHT;
    }
    act = stage_get_stage_act();
    switch (act) {
    case STATION_SQUARE_OFFICE:
        land_textures_set(&office_time_table, time);
        land_lights_set(LENGTH(office_night_lights), office_night_lights, time);
        break;
    case STATION_SQUARE_CASINO:
        land_textures_set(&casino_time_table, time);
        break;
    case STATION_SQUARE_GESUI:
        break;
    case STATION_SQUARE_STATION:
        land_textures_set(&station_time_table, time);
        land_lights_set(LENGTH(station_night_lights), station_night_lights,
                        time);
        break;
    case STATION_SQUARE_HOTEL:
        land_textures_set(&hotel_time_table, time);
        break;
    case STATION_SQUARE_TWINKLE:
        break;
    }
}
