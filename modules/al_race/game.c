/* what the race's objects ask of the Chao race's game, and the windows they
 * open in it. the file also holds the cheers' voices and the debug page's
 * rows of bits */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/chao.h"
#include "al_race/entrant.h"
#include "al_race/game.h"
#include "al_race/race_camera.h"
#include "al_race/racer.h"
#include "al_race/standings.h"
#include "chao/chao.h"
#include "sound/stream.h"
#include "system/util.h"

/* a group of voices, the first ones and then the rest */
struct race_voices {
    const Sint16 *voices;
    Uint16 count;
    Uint16 first; /* how many are the first ones */
};

static const Sint16 race_voice_list[18] = {
#include "al_race/game/race_voice_list.inc"
};
static const struct race_voices race_voice_groups[6] = {
    {&race_voice_list[0], 3, 2},  {&race_voice_list[3], 2, 1},
    {&race_voice_list[5], 3, 2},  {&race_voice_list[8], 4, 2},
    {&race_voice_list[12], 3, 2}, {&race_voice_list[15], 3, 2},
};

int race_get_kind(int *kind)
{
    struct race_game *game = race_game_current;

    if (game == NULL) {
        return 0;
    }
    /* kind holds 0 to 5, and widening it to int keeps the value */
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    *kind = game->kind;
    return 1;
}

int race_get_followed_lane(int *lane)
{
    struct race_game *game = race_game_current;

    if (game == NULL) {
        return 0;
    }
    /* lane holds 0 to 7, and widening it to int keeps the value */
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    *lane = game->lane;
    return 1;
}

int fun_0cb8b510(int *timer, int which)
{
    struct race_game *game = race_game_current;

    if (game == NULL || game->places == NULL) {
        return 0;
    }
    /* @hack spelling: a switch of one case, which SHC tests with CMP/EQ #0,R0
     * (0x0cb8b522); as an if, 11 of 22 words differ */
    switch (which) {
    case 0:
        *timer = game->places->timer;
        return 1;
    }
    return 0;
}

int fun_0cb8b566(void)
{
    struct race_game *game = race_game_current;

    if (game == NULL) {
        return 0;
    }
    switch (game->camera.mode) {
    case RACE_CAMERA_AHEAD:
    case RACE_CAMERA_COURSE_FIXED:
    case RACE_CAMERA_WATCH:
    case RACE_CAMERA_FINISH:
        return 1;
    }
    return 2;
}

int race_get_entry_kind(int *kind, int lane)
{
    struct race_game *game = race_game_current;

    if (game == NULL) {
        return 0;
    }
    switch (game->entries[lane].kind) {
    case GARDEN_STATION_SQUARE:
        *kind = GARDEN_STATION_SQUARE;
        break;
    case GARDEN_EGG_CARRIER:
        *kind = GARDEN_EGG_CARRIER;
        break;
    case GARDEN_MYSTIC_RUINS:
        *kind = GARDEN_MYSTIC_RUINS;
        break;
    case ENTRY_CARD:
        *kind = ENTRY_CARD;
        break;
    case ENTRY_RIVAL:
        *kind = ENTRY_RIVAL;
        break;
    default:
        return 0;
    }
    return 1;
}

int race_open_race_window(void)
{
    struct race_game *game = race_game_current;

    if (game == NULL ||
        (game->state != STATE_ENTRANCE && game->state != STATE_CUPS_WINDOW)) {
        return 0;
    }
    if (game->places != NULL) {
        game->state = STATE_RACE_WINDOW_INI;
        game->timer = 0;
        game->places->step = 0;
        game->places->timer = 0;
    }
    return 1;
}

int race_open_cups_window(void)
{
    struct race_game *game = race_game_current;

    if (game == NULL || (game->state != STATE_RACE_WINDOW &&
                         game->state != STATE_GARDEN_WINDOW)) {
        return 0;
    }
    if (game->places != NULL) {
        game->state = STATE_CUPS_WINDOW_INI;
        game->timer = 0;
        game->places->step = 0;
        game->places->timer = 0;
    }
    return 1;
}

int race_open_garden_window(void)
{
    struct race_game *game = race_game_current;

    if (game == NULL || (game->state != STATE_CUPS_WINDOW &&
                         game->state != STATE_GARDEN_WINDOW &&
                         game->state != STATE_CHAO_WINDOW)) {
        return 0;
    }
    if (game->places != NULL) {
        game->state = STATE_GARDEN_WINDOW_INI;
        game->timer = 0;
        game->places->step = 0;
        game->places->timer = 0;
    }
    return 1;
}

int race_open_chao_window(void)
{
    struct race_game *game = race_game_current;

    if (game == NULL || game->state != STATE_GARDEN_WINDOW) {
        return 0;
    }
    if (game->places != NULL) {
        game->state = STATE_CHAO_WINDOW_INI;
        game->timer = 0;
        game->places->step = 0;
        game->places->timer = 0;
    }
    return 1;
}

int fun_0cb8b7ea(void)
{
    struct race_game *game = race_game_current;

    if (game != NULL) {
        return 0;
    }
    return -1;
}

struct racer *race_racer_at_place(int place)
{
    struct race_game *game = race_game_current;

    if (game != NULL) {
        return &game->racers[game->standings.lanes[place]];
    }
    return NULL;
}

int fun_0cb8b860(int place, int what)
{
    struct racer *racer = race_racer_at_place(place);

    if (racer != NULL) {
        switch (what) {
        case 0:
            return racer->lane;
        case 1:
            return racer->distance;
        case 2:
            return racer->chao->record->skills.run;
        case 3:
            return racer->chao->record->skills.swim;
        case 4:
            return racer->chao->record->skills.fly;
        case 5:
            return racer->chao->record->skills.power;
        }
    }
    return -1;
}

void race_request_voice(int group, int later)
{
    int voice;

    if (group <= 5) {
        if (later == 0) {
            voice = RANDOM(0, race_voice_groups[group].first);
        } else {
            voice = RANDOM(race_voice_groups[group].first,
                           race_voice_groups[group].count);
        }
        stream_request_voice(race_voice_groups[group].voices[voice]);
    }
}

int race_cracker_kind(void)
{
    struct race_game *game = race_game_current;
    int rank;
    int lane;
    struct race_entry *entry;

    if (game != NULL) {
        for (rank = 0; rank <= 2; rank++) {
            lane = standings_lane(&game->standings, rank);
            /* @hack spelling: the entry through a pointer, whose address SHC
             * adds up before the load (ADD R3,R4 at 0x0cb8ba28); indexed in
             * place, it loads through @(R0,R3), and 23 of 41 words differ */
            entry = &game->entries[lane];
            if (entry->kind != ENTRY_RIVAL) {
                if (rank == 0) {
                    return 2;
                }
                return 1;
            }
        }
    }
    return 0;
}

void race_print_bits8(void *value, int x, int y)
{
    int bit;
    Uint8 *bits = value;

    for (bit = 0; bit < 8; bit++) {
        if (*bits & (1 << bit)) {
            njPrintC(NJM_LOCATION(x + bit, y), "O");
        } else {
            njPrintC(NJM_LOCATION(x + bit, y), "X");
        }
        if ((bit & 3) == 0) {
            njPrintC(NJM_LOCATION(x + bit, y + 1), "^");
        }
    }
}

void race_print_bits16(void *value, int x, int y)
{
    int bit;
    Uint16 *bits = value;

    for (bit = 0; bit < 16; bit++) {
        if (*bits & (1 << bit)) {
            njPrintC(NJM_LOCATION(x + bit, y), "O");
        } else {
            njPrintC(NJM_LOCATION(x + bit, y), "X");
        }
        if ((bit & 3) == 0) {
            njPrintC(NJM_LOCATION(x + bit, y + 1), "^");
        }
    }
}

void race_print_bits32(void *value, int x, int y)
{
    int bit;
    Uint32 *bits = value;

    for (bit = 0; bit < 32; bit++) {
        if (*bits & (1 << bit)) {
            njPrintC(NJM_LOCATION(x + bit, y), "O");
        } else {
            njPrintC(NJM_LOCATION(x + bit, y), "X");
        }
        if ((bit & 3) == 0) {
            njPrintC(NJM_LOCATION(x + bit, y + 1), "^");
        }
    }
}
