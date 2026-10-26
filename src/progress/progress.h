/* progress: the event flags, and each character's place in its story */
#ifndef PROGRESS_PROGRESS_H
#define PROGRESS_PROGRESS_H
#include <sg_xpt.h>

#include "player/player.h"
#include "stage/stage.h"

struct story_script;
struct story_section;

#define FLAG_COUNT 512
#define EVENT_COUNT 512

#define FLAG_PLAYABLE_SONIC 65
#define FLAG_PLAYABLE_TAILS 66
#define FLAG_PLAYABLE_KNUCKLES 67
#define FLAG_PLAYABLE_AMY 68
#define FLAG_PLAYABLE_E102 69
#define FLAG_PLAYABLE_BIG 70
#define FLAG_ALL_CLEAR 71 /* each of the six stories has ended */
#define FLAG_CLEAR_SONIC 72
#define FLAG_CLEAR_TAILS 73
#define FLAG_CLEAR_KNUCKLES 74
#define FLAG_CLEAR_AMY 75
#define FLAG_CLEAR_E102 76
#define FLAG_CLEAR_BIG 77
#define FLAG_CLEAR_LAST 78 /* the last story has ended */
/* set by alife.c's garden_take_egg alone, as it takes each egg into a Chao
 * garden. leaving the fake egg in the gold egg's place on its stand sets
 * none. each egg's own object leaves the egg out while its flag is set */
#define FLAG_GOLD_EGG_IN 79
#define FLAG_SILVER_EGG_IN 80
#define FLAG_BLACK_EGG_IN 81
/* a flag for each character. story_complete sets it with the character's
 * train, boat and raft flags. each character clears or sets it in its
 * Station Square and Egg Carrier sections, and each but Sonic in its Mystic
 * Ruins sections */
#define FLAG_SONIC_UNK_82 82
#define FLAG_TAILS_UNK_83 83
#define FLAG_KNUCKLES_UNK_84 84
#define FLAG_AMY_UNK_85 85
#define FLAG_E102_UNK_86 86
#define FLAG_BIG_UNK_87 87
#define FLAG_LAST_STORY 88 /* set while the last story plays */
/* each character's flags for Station Square: the ways through its acts,
 * its card and its cart pass. the clears and Egg Carrier sinkings among
 * them open some of those ways */
#define FLAG_SONIC_SS_BARRICADE 128
#define FLAG_SONIC_SS_HOTEL_FRONT 129
#define FLAG_SONIC_SS_STATION_FRONT 130
#define FLAG_SONIC_SS_ENTRANCE_SEWER 131
#define FLAG_SONIC_SS_EXIT_SEWER 132
#define FLAG_SONIC_SS_ICE_STONE 133
#define FLAG_SONIC_SS_HOTEL_BACK 134
#define FLAG_SONIC_SS_ENTRANCE_CASINO 135
#define FLAG_SONIC_SS_STATION_BACK 136
#define FLAG_SONIC_SS_TPARK_ELEVATOR 137
#define FLAG_SONIC_SS_ENTRANCE_CIRCUIT 138
#define FLAG_SONIC_SS_CARD 139
#define FLAG_SONIC_SS_ENTRANCE_HIGHWAY 140
#define FLAG_SONIC_CLEAR_TWINKLE_PARK 169
#define FLAG_TAILS_SS_BARRICADE 192
#define FLAG_TAILS_SS_HOTEL_FRONT 193
#define FLAG_TAILS_SS_HOTEL_BACK 194
#define FLAG_TAILS_SS_ICE_STONE 196
#define FLAG_TAILS_SS_ENTRANCE_CASINO 197
#define FLAG_TAILS_SS_ENTRANCE_HIGHWAY 198
#define FLAG_TAILS_SS_TPARK_ELEVATOR 199
#define FLAG_TAILS_SS_ENTRANCE_CIRCUIT 200
#define FLAG_TAILS_SS_CART_PASSPORT 201
#define FLAG_TAILS_SS_STATION_FRONT 202
#define FLAG_TAILS_SS_STATION_BACK 203
#define FLAG_TAILS_EC_SINK 210
#define FLAG_KNUCKLES_SS_BARRICADE 256
#define FLAG_KNUCKLES_SS_HOTEL_FRONT 257
#define FLAG_KNUCKLES_SS_HOTEL_BACK 258
#define FLAG_KNUCKLES_SS_ENTRANCE_CASINO 259
#define FLAG_KNUCKLES_SS_TPARK_ELEVATOR 261
#define FLAG_KNUCKLES_SS_ENTRANCE_CIRCUIT 262
#define FLAG_KNUCKLES_SS_CART_PASSPORT 263
#define FLAG_KNUCKLES_SS_STATION_FRONT 264
#define FLAG_KNUCKLES_SS_STATION_BACK 265
#define FLAG_KNUCKLES_CLEAR_CHAOS2 291
#define FLAG_KNUCKLES_LOOKOUT 300 /* the lookout's elevator opens */
#define FLAG_AMY_SS_HOTEL_FRONT 320
#define FLAG_AMY_SS_HOTEL_BACK 321
#define FLAG_AMY_SS_ENTRANCE_CASINO 323
#define FLAG_AMY_SS_TPARK_ELEVATOR 324
#define FLAG_AMY_SS_ENTRANCE_CIRCUIT 325
#define FLAG_AMY_SS_STATION_FRONT 326
#define FLAG_AMY_SS_STATION_BACK 327
#define FLAG_AMY_EC_SINK 333
#define FLAG_AMY_CLEAR_TWINKLE_PARK 342
#define FLAG_E102_SS_HOTEL_FRONT 384
#define FLAG_E102_SS_HOTEL_POOL 385
#define FLAG_E102_SS_STATION_FRONT 386
#define FLAG_E102_SS_TPARK_ELEVATOR 387
#define FLAG_E102_SS_ENTRANCE_CIRCUIT 388
#define FLAG_E102_SS_CART_PASSPORT 389
#define FLAG_BIG_SS_HOTEL_FRONT 448
#define FLAG_BIG_SS_HOTEL_BACK 449
#define FLAG_BIG_SS_TPARK_ELEVATOR 450
#define FLAG_BIG_SS_ENTRANCE_CIRCUIT 451
#define FLAG_BIG_SS_STATION_FRONT 452
#define FLAG_BIG_SS_ICE_STONE 453
#define FLAG_BIG_SS_HOTEL_POOL 454
#define FLAG_BIG_CLEAR_TWINKLE_PARK 471
/* the items each character finds, which player_init turns into upgrades */
#define FLAG_SONIC_LIGHT_SHOES 141
#define FLAG_SONIC_CRYSTAL_RING 142
#define FLAG_SONIC_ANCIENT_LIGHT 159
#define FLAG_TAILS_JET_ANKLET 205
#define FLAG_TAILS_RHYTHM_BROOCH 219
#define FLAG_KNUCKLES_SHOVEL_CLAWS 281
#define FLAG_KNUCKLES_FIGHTING_GLOVES 282
#define FLAG_AMY_WARRIOR_FEATHER 338
#define FLAG_AMY_LONG_HAMMER 350
#define FLAG_E102_BOOSTER 393
#define FLAG_E102_LASER 394
#define FLAG_BIG_LIFE_BELT 464
#define FLAG_BIG_POWER_ROD 465
/* Big's lures: each of the first four is found in the place it names */
#define FLAG_BIG_LURE_EGG_CARRIER 478
#define FLAG_BIG_LURE_ICECAP 479
#define FLAG_BIG_LURE_STATION_SQUARE 480
#define FLAG_BIG_LURE_MYSTIC_RUINS 481
#define FLAG_BIG_LURE_5 482
/* each character's flags for the Egg Carrier and Mystic Ruins: the ways that
 * open there, and the clears and battles that open them */
#define FLAG_SONIC_EC_MONORAIL 143
#define FLAG_SONIC_EC_EGG_LIFT 144
#define FLAG_SONIC_EC_TRANSFORM 145
#define FLAG_SONIC_EC_SINK 147
#define FLAG_SONIC_MR_WINDY_STONE 148
#define FLAG_SONIC_MR_ENTRANCE_WINDY 149
#define FLAG_SONIC_MR_WEST_ROCK 150
#define FLAG_SONIC_MR_ICE_STONE 151
#define FLAG_SONIC_MR_ENTRANCE_ICECAP 152
#define FLAG_SONIC_MR_ENTRANCE_MOUNTAIN 153
#define FLAG_SONIC_MR_ISLAND_DOOR 154
#define FLAG_SONIC_MR_TRUCK 155
#define FLAG_SONIC_MR_ENTRANCE_RUIN 156
#define FLAG_SONIC_MR_APPEAR_FINAL_EGG 157
#define FLAG_SONIC_MR_ENTRANCE_SAND_BOARD 158
#define FLAG_SONIC_MR_ENTRANCE_FINAL_EGG 160
#define FLAG_SONIC_MBOSS_KNUCKLES 165
#define FLAG_SONIC_CLEAR_BEACH 166
#define FLAG_SONIC_CLEAR_WINDY 167
#define FLAG_SONIC_CLEAR_CASINO 168
#define FLAG_SONIC_CLEAR_ICECAP 172
#define FLAG_TAILS_EC_MONORAIL 206
#define FLAG_TAILS_EC_EGG_LIFT 207
#define FLAG_TAILS_EC_TRANSFORM 208
#define FLAG_TAILS_MR_WINDY_STONE 211
#define FLAG_TAILS_MR_ENTRANCE_WINDY 212
#define FLAG_TAILS_MR_WEST_ROCK 213
#define FLAG_TAILS_MR_ICE_STONE 214
#define FLAG_TAILS_MR_ENTRANCE_ICECAP 215
#define FLAG_TAILS_MR_TRUCK 216
#define FLAG_TAILS_MR_ENTRANCE_SAND_BOARD 218
#define FLAG_TAILS_MBOSS_KNUCKLES 224
#define FLAG_TAILS_CLEAR_WINDY 225
#define FLAG_TAILS_CLEAR_CASINO 226
#define FLAG_TAILS_CLEAR_ICECAP 228
#define FLAG_TAILS_CLEAR_SAND_BOARD 230
#define FLAG_KNUCKLES_EC_MONORAIL 266
#define FLAG_KNUCKLES_EC_EGG_LIFT 267
#define FLAG_KNUCKLES_EC_TRANSFORM 268
#define FLAG_KNUCKLES_EC_SINK 271
#define FLAG_KNUCKLES_MR_ENTRANCE_MOUNTAIN 272
#define FLAG_KNUCKLES_MR_WEST_ROCK 273
#define FLAG_KNUCKLES_MR_ISLAND_DOOR 274
#define FLAG_KNUCKLES_MR_TRUCK 276
#define FLAG_KNUCKLES_MR_RED_CUBE 277
#define FLAG_KNUCKLES_MR_BLUE_CUBE 278
#define FLAG_KNUCKLES_MR_ENTRANCE_RUIN 279
#define FLAG_KNUCKLES_MR_APPEAR_FINAL_EGG 280
#define FLAG_KNUCKLES_MBOSS_SONIC 286
#define FLAG_KNUCKLES_MR_MONKEY_DOOR_ENTERED 298
#define FLAG_KNUCKLES_MR_MONKEY_CAGE_A_BOMB 299
#define FLAG_AMY_EC_MONORAIL 328
#define FLAG_AMY_EC_EGG_LIFT 329
#define FLAG_AMY_EC_TRANSFORM 330
#define FLAG_AMY_MR_ISLAND_DOOR 334
#define FLAG_AMY_MR_TRUCK 335
#define FLAG_AMY_MR_APPEAR_FINAL_EGG 336
#define FLAG_AMY_MR_ENTRANCE_FINAL_EGG 337
#define FLAG_E102_EC_MONORAIL 390
#define FLAG_E102_EC_EGG_LIFT 391
#define FLAG_E102_EC_TRANSFORM 392
#define FLAG_E102_EC_SINK 396
#define FLAG_E102_EC_HOT_SHELTER 397
#define FLAG_E102_MR_FREE_PASS 399
#define FLAG_E102_MR_WINDY_STONE 400
#define FLAG_E102_MR_ENTRANCE_WINDY 401
#define FLAG_E102_MR_WEST_ROCK 402
#define FLAG_E102_MR_ENTRANCE_MOUNTAIN 403
#define FLAG_E102_MR_ISLAND_DOOR 404
#define FLAG_E102_MR_TRUCK 405
#define FLAG_E102_MR_APPEAR_FINAL_EGG 406
#define FLAG_E102_CLEAR_BEACH 411
#define FLAG_BIG_EC_MONORAIL 455
#define FLAG_BIG_EC_EGG_LIFT 456
#define FLAG_BIG_EC_TRANSFORM 457
#define FLAG_BIG_EC_SINK 459
#define FLAG_BIG_MR_WEST_ROCK 460
#define FLAG_BIG_MR_ICE_STONE 461
#define FLAG_BIG_MR_ENTRANCE_ICECAP 462
#define FLAG_BIG_MR_TRUCK 463
/* each character's other clears and battle wins: progress.c's tables give
 * each the stage or the battle that sets it */
#define FLAG_SONIC_MBOSS_E102 164
#define FLAG_SONIC_CLEAR_HIGHWAY 170
#define FLAG_SONIC_CLEAR_MOUNTAIN 171
#define FLAG_SONIC_CLEAR_SKY_DECK 173
#define FLAG_SONIC_CLEAR_RUIN 174
#define FLAG_SONIC_CLEAR_FINAL_EGG 175
#define FLAG_SONIC_CLEAR_CHAOS0 176
#define FLAG_SONIC_CLEAR_CHAOS4 177
#define FLAG_SONIC_CLEAR_CHAOS6 178
#define FLAG_SONIC_CLEAR_EGGMOBILE1 179
#define FLAG_SONIC_CLEAR_EGGMOBILE3 180
#define FLAG_SONIC_CLEAR_SHOOTING1 181
#define FLAG_SONIC_CLEAR_SHOOTING2 182
#define FLAG_TAILS_MBOSS_E102 223
#define FLAG_TAILS_CLEAR_HIGHWAY 227
#define FLAG_TAILS_CLEAR_SKY_DECK 229
#define FLAG_TAILS_CLEAR_CHAOS4 231
#define FLAG_TAILS_CLEAR_EGGMOBILE2 232
#define FLAG_TAILS_CLEAR_EGGMOBILE1 233
#define FLAG_TAILS_CLEAR_SHOOTING1 234
#define FLAG_TAILS_CLEAR_SHOOTING2 235
#define FLAG_TAILS_CLEAR_BEACH 236
#define FLAG_TAILS_CLEAR_MOUNTAIN 237
#define FLAG_KNUCKLES_CLEAR_CASINO 287
#define FLAG_KNUCKLES_CLEAR_HIGHWAY 288
#define FLAG_KNUCKLES_CLEAR_MOUNTAIN 289
#define FLAG_KNUCKLES_CLEAR_RUIN 290
#define FLAG_KNUCKLES_CLEAR_CHAOS6 292
#define FLAG_KNUCKLES_CLEAR_CHAOS4 293
#define FLAG_KNUCKLES_CLEAR_SKY_DECK 294
#define FLAG_AMY_CLEAR_HOT_SHELTER 343
#define FLAG_AMY_CLEAR_FINAL_EGG 344
#define FLAG_AMY_CLEAR_EGGMANROBO 345
#define FLAG_E102_MBOSS_SONIC 410
#define FLAG_E102_CLEAR_WINDY 412
#define FLAG_E102_CLEAR_MOUNTAIN 413
#define FLAG_E102_CLEAR_HOT_SHELTER 414
#define FLAG_E102_CLEAR_FINAL_EGG 415
#define FLAG_E102_CLEAR_E101 416
#define FLAG_E102_CLEAR_E101_R 417
#define FLAG_BIG_CLEAR_BEACH 469
#define FLAG_BIG_CLEAR_HOT_SHELTER 470
#define FLAG_BIG_CLEAR_ICECAP 472
#define FLAG_BIG_CLEAR_CHAOS6 473
/* each character's train between Station Square and Mystic Ruins, set when
 * it opens */
#define FLAG_SONIC_TRAIN 161
#define FLAG_TAILS_TRAIN 220
#define FLAG_KNUCKLES_TRAIN 283
#define FLAG_AMY_TRAIN 339
#define FLAG_E102_TRAIN 407
#define FLAG_BIG_TRAIN 466
/* each character's boat from Station Square to the Egg Carrier */
#define FLAG_SONIC_BOAT 162
#define FLAG_TAILS_BOAT 221
#define FLAG_KNUCKLES_BOAT 284
#define FLAG_AMY_BOAT 340
#define FLAG_E102_BOAT 408
#define FLAG_BIG_BOAT 467
/* each character's raft from Mystic Ruins out to the Egg Carrier */
#define FLAG_SONIC_RAFT 163
#define FLAG_TAILS_RAFT 222
#define FLAG_KNUCKLES_RAFT 285
#define FLAG_AMY_RAFT 341
#define FLAG_E102_RAFT 409
#define FLAG_BIG_RAFT 468
/* set once Amy's score at the Egg Carrier's Hedgehog Hammer reaches
 * Eggman's best */
#define FLAG_AMY_HAMMER_WON 331
/* while an event has control, the walkers of the crowds in Station Square,
 * the Mystic Ruins and the Past run and draw only while it is set */
#define FLAG_CROWD_IN_EVENT 484

/* the times of day a story_record keeps, as story_get_time gives them */
#define TIME_DAY 0
#define TIME_EVENING 1
#define TIME_NIGHT 2

/* a record's wait_at while its section waits for no stage and act */
#define STORY_NO_WAIT 0xffff

/* a character's place in its story */
struct story_record {
    Sint8 time; /* of day: TIME_DAY, TIME_EVENING or TIME_NIGHT */
    Sint8 unk_01;
    /* the sequence number story_step passes the section's script. it is -1
     * from story_next_section until an act starts in the new section's
     * field */
    Sint16 seq;
    /* the section's index in the story's table, or -1 once the story has
     * ended */
    Sint16 section;
    /* the entrance, and the STAGE_ACT, the story goes on from, as story_save
     * keeps them */
    Sint16 entrance;
    Uint16 stage_act;
    /* the STAGE_ACT the section waits for, or STORY_NO_WAIT */
    Uint16 wait_at;
};

/* TRUE while the last story plays. ADVERTISE's menus set it */
extern Bool last_story;
/* the rival's character in the battle the field holds, or -1 for none */
extern int battle_rival;

/* how many times each character has cleared each stage */
extern Sint8 stage_clears[CHARACTER_COUNT][STAGE_COUNT];
extern Uint8 event_flags[FLAG_COUNT];
extern struct story_record story_records[CHARACTER_COUNT];
/* a byte for each character, which save_store and save_load carry */
extern Uint8 dat_8c7a2718[CHARACTER_COUNT];
extern Sint8 events_played[EVENT_COUNT];
/* only progress.c uses the next eight. they stay extern because bss
 * follows each name's first declaration: as statics, they would move past
 * story_steps, story_ended and story_event_delay */
extern Uint8 dat_8c7a2920[512];
extern Uint8 dat_8c7a2b20[512];
extern Uint8 dat_8c7a2d20[16];
extern Uint8 dat_8c7a2d30[512];
extern Uint8 dat_8c7a2f30[344];
/* the sections of the story that runs, the script of the current one, and
 * the story's character, CHARACTER_TIKAL for the last story */
extern struct story_section *story_table;
extern struct story_script *story_current_script;
extern Sint16 story_character;
/* the story_step calls since the act started, the first two of which do
 * nothing */
extern int story_steps;
/* TRUE after a story_step that finds the record's section -1. each
 * story_step clears it first, so one that returns early leaves it FALSE */
extern Bool story_ended;
/* the countdown to a held-back event, or 0 for none: story_play_event sets
 * it as a fade out begins. each story_step that doesn't return early counts
 * it down. the one that brings it to 1 plays the event */
extern int story_event_delay;

/* start a new game. it clears every flag, story record, clear count and
 * played event, and makes only Sonic playable. player_record becomes NULL */
void progress_init(void);
/* drop player_record and the current script */
void story_clear(void);
void story_enable(void);
void story_disable(void);
Bool story_is_enabled(void);
/* while the story is enabled, make the character's story the one that
 * runs, or the last story while last_story is set. player_record and
 * story_table follow it. FLAG_LAST_STORY is set for the last story, and
 * cleared for any other */
void story_set_character(int character);
/* set the stage, act and entrance the story goes to next. a story still at
 * its start goes to its first section's start, and any other to where
 * story_save left it. an ended story goes to Station Square's station act,
 * and the last story starts over. it then clears battle_rival. it does
 * nothing while the story is disabled or has no table, and ignores section */
void story_set_next_stage(int section);
/* start an act: clear the act's flags and story_steps. while the story is
 * enabled, in the field of the record's section, take up the section's
 * script. hold battle_start's battle again if battle_rival isn't -1, then
 * call the script's init */
void story_start_act(void);
/* run the story for a step of the stage. it does nothing on the act's first
 * two calls. it does nothing while the story is disabled, or while
 * player_record or story_current_script is NULL. it does nothing while a
 * notice, a bitmap page or an event holds the game. it does nothing while
 * the record waits for another stage and act, and clears the wait once they
 * are current. while a held-back event waits, it counts story_event_delay
 * down instead of stepping the script. the call that brings the delay to 1
 * plays the event and steps the script. it then advances seq, unless seq is
 * -1 or the script changed it */
void story_step(void);
/* keep the stage, act and entrance in player_record, and start writing the
 * saves with save_write, whose result it ignores. it does nothing without
 * player_record */
void story_save(void);
/* do nothing */
void fun_8c0955fc(void);
/* set player_record's wait_at, if there is a record */
void story_set_wait(Uint16 stage_act);
/* advance the record's seq, unless it is -1 or a held-back event waits to
 * play */
void story_next_seq(void);
/* move to the next section, unless a held-back event waits to play. past
 * the last section, the story ends: its flags are set and section becomes
 * -1. the story is then saved as story_save saves it, waiting for the drive */
void story_next_section(void);
/* play the event, unless it has played or a held-back event waits to play.
 * progress.c lists the events it holds back: for those, the screen fades to
 * black with the pads off, and story_step plays the event later */
void story_play_event(int event);
/* set the record's time of day, if there is a record */
void story_set_time(Sint8 time);
/* the record's time of day, or TIME_DAY without one */
int story_get_time(void);
Bool playable_check(int character);
void playable_set(int character);
/* flag must be in [0, FLAG_COUNT): none of the three checks it. they act on
 * event_flags in memory, which save_store packs into the selected save */
void event_flag_set(int flag);
void event_flag_clear(int flag);
Bool event_flag_check(int flag);
/* TRUE if the stage and act are the current ones. otherwise FALSE, with
 * player_record->wait_at set to them, so player_record must be set */
Bool story_wait_at(int stage, int act);
/* do nothing */
void fun_8c0958e8(void);
void story_go_to(int stage, int act);
/* clear player_record's wait, and return TRUE if the stage and act are the
 * ones the game goes to next */
Bool story_going_to(Uint8 stage, Uint8 act);
/* battle_start's numbers, each named for player 0's character and then its
 * rival */
#define SONIC_VS_KNUCKLES 0
#define SONIC_VS_E102 1
#define TAILS_VS_KNUCKLES 2
#define TAILS_VS_E102 3
#define KNUCKLES_VS_SONIC 4
#define E102_VS_SONIC 5
/* hold the battle of that number, SONIC_VS_KNUCKLES to E102_VS_SONIC, in the
 * field, and set battle_rival to its rival */
void battle_start(int number);
/* TRUE if the current character has cleared the stage */
Bool stage_cleared(int stage);
/* set the flag the current character's story keeps for clearing the stage.
 * the character must have a story: Eggman's and Tikal's lists are NULL. for
 * Knuckles, a stage his list lacks can set Amy's flag for it */
void story_mark_cleared(int stage);
/* TRUE once the current character's story has sunk the Egg Carrier. for
 * Eggman and Tikal, it tests flag 0, which each act clears */
Bool egg_carrier_has_sunk(void);
/* bring the field's people out, as an event ends */
void field_people_place(void);
/* take the field's people away, as an event or a battle starts */
void field_people_remove(void);

#endif
