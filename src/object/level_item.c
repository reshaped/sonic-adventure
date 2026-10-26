/* the level items, O LevelItem: the upgrades and Big's lures. each waits on
 * its stand in a field until its character takes it */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/sparkle.h"
#include "event/play.h"
#include "hud/hud.h"
#include "object/level_item.h"
#include "object/models.h"
#include "object/placement.h"
#include "player/big_models.h"
#include "player/player.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/bitmap.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

#define TEXT_1_LENGTH 21
#define TEXT_2_LENGTH 25
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 10
#define TEXT_9_LENGTH 10
#define TEXT_10_LENGTH 10
#define TEXT_11_LENGTH 9
#define TEXT_12_LENGTH 9
#define TEXT_13_LENGTH 9
#define TEXT_14_LENGTH 9
#define TEXT_15_LENGTH 9
#define TEXT_16_LENGTH 9
#define TEXT_17_LENGTH 9
#define TEXT_18_LENGTH 9
#include "object/level_item/text.inc"

/* the steps the waves rise for once the item is taken. at TAKE_STEPS / 2
 * the item leaves its stand, and its character gets it */
#define TAKE_STEPS 180
/* counted from the take: the steps before a lure's hint shows, and those
 * before the stand ends */
#define HINT_STEPS 90
#define DONE_STEPS 180

/* the waves that rise round a taken item, and the sparks that circle the
 * ancient light */
#define WAVES 3
#define SPARKS 4

#define LAST_UPGRADE LEVEL_ITEM_LIFE_BELT

/* player 0 looks at the item while he is within WATCH_RANGE of it */
#define WATCH_RANGE 40.0F

/* a model in the module of the field its upgrade is found in, at the
 * address it takes once that module loads */
#define MODULE_OBJECT(address) ((NJS_OBJECT *)(address))

/* the stand's modes, in its work */
enum level_item_mode {
    LEVEL_ITEM_INIT,
    LEVEL_ITEM_WAIT, /* for a player's touch */
    LEVEL_ITEM_TAKEN, /* the waves rise, and the item goes */
    LEVEL_ITEM_HINT, /* only the hint runs, until the stand goes */
    LEVEL_ITEM_STILL, /* drawn, and nothing more */
    LEVEL_ITEM_KILL
};

/* the flags in a stand's record */
#define ITEM_SHOWN 0x1 /* the item is still on its stand */
/* while set, the stand steps its children. nothing sets it */
#define ITEM_STEP_CHILDREN 0x2
#define ITEM_WATCHED 0x4 /* player 0 is within WATCH_RANGE, looking at it */

/* the hint's modes once the item is taken. HINT_WAIT ends after HINT_STEPS,
 * and shows a lure's hint as it ends. HINT_SHOWN ends the stand once
 * DONE_STEPS have passed since the take */
enum level_item_hint { HINT_WAIT, HINT_SHOWN };

/* the kinds of field an item is found in */
enum level_item_area {
    AREA_STATION_SQUARE,
    AREA_MYSTIC_RUINS,
    AREA_EGG_CARRIER,
    AREA_OTHER
};

/* a spark round the ancient light */
struct level_item_spark {
    struct angles ang; /* its angles about the item */
    struct angles spin; /* added to ang.x and ang.y each step */
    Float frame; /* its sprite's frame */
    Float rate; /* the frames it plays a step */
};

/* the stand's own record, through its task's extra */
struct level_item {
    Uint32 flags;
    Sint32 kind;
    Sint32 time; /* steps since it was taken */
    Float wave_frames[WAVES]; /* each wave's frame in level_item_wave_action */
    /* each wave's constant material alpha, an offset. it lies between -1,
     * which clears the wave, and 0, which draws it as it is. the take's fade
     * then scales it */
    Float wave_fades[WAVES];
    NJS_POINT3 pos; /* where the item floats, 10 above the stand */
    struct angles ang;
    Sint32 hint_mode; /* the hint's mode, a level_item_hint */
    Sint32 hint_time; /* steps since the take, as the hint counts them */
    Uint8 unk_44[24];
    struct level_item_spark sparks[SPARKS];
};

struct level_item_extra {
    struct level_item *item;
};

/* @hack spelling: the record reached through task->extra at every use, which
 * the image reads again each time (MOV.L @(44,R14) at 0x8c06d324 and
 * 0x8c06d32e); held in a local in each function, 295 of 328 words differ */
#define EXTRA(task) ((struct level_item_extra *)(task)->extra)

/* the item the placement gives, in scl.x */
#define KIND(work) ((work)->scl.x)

struct level_item_model {
    NJS_OBJECT *object;
    NJS_TEXLIST *texlist;
};

struct level_item_place {
    Sint16 character;
    Sint32 area; /* the kind of field it's found in, a level_item_area */
};

/* an NJS_SPRITE's fields after its position */
struct sprite_look {
    Float sx, sy;
    Angle ang;
    NJS_TEXLIST *tlist;
    NJS_TEXANIM *tanim;
};

/* the event that plays as an upgrade is taken */
struct level_item_event {
    Sint32 unk_00;
    Sint32 event;
};

/* Big's lures, by how many he has */
extern NJS_OBJECT big_lure_0;
extern NJS_OBJECT big_lure_1;
extern NJS_OBJECT big_lure_2;
extern NJS_OBJECT big_lure_3;
extern NJS_OBJECT big_lure_4;

static void level_item_destroy(struct task *task);
static void level_item_draw(struct task *task);
static struct level_item *level_item_alloc(void);
static Bool level_item_in_area(Sint16 kind, int area);
static void level_item_find(Sint16 character, int area, Sint32 *kind);
static Bool level_item_pick(int kind);
static void level_item_draw_stand(struct task *task);
static void level_item_draw_waves(struct task *task);
static void level_item_animate(struct task *task);
static void level_item_draw_item(struct task *task);
static void level_item_touch(struct task *task);
static void level_item_float(struct task *task);
static void level_item_take(struct task *task);
static void level_item_watch(struct task *task);
static void level_item_set_flag(int kind);
static void level_item_print(struct task *task);
static void level_item_draw_ancient_light(void);
static void level_item_draw_glow(void);
static void level_item_draw_spark(int spark);
static void level_item_init_sparks(void);
static void level_item_hint(struct task *task);

/* the flag each item sets as it's taken. a lure sets its field's instead */
static const Sint32 level_item_flags[LEVEL_ITEM_COUNT] = {
    FLAG_SONIC_CRYSTAL_RING,
    FLAG_SONIC_LIGHT_SHOES,
    FLAG_SONIC_ANCIENT_LIGHT,
    FLAG_TAILS_JET_ANKLET,
    FLAG_TAILS_RHYTHM_BROOCH,
    FLAG_KNUCKLES_FIGHTING_GLOVES,
    FLAG_KNUCKLES_SHOVEL_CLAWS,
    FLAG_AMY_LONG_HAMMER,
    FLAG_AMY_WARRIOR_FEATHER,
    FLAG_E102_LASER,
    FLAG_E102_BOOSTER,
    FLAG_BIG_POWER_ROD,
    FLAG_BIG_LIFE_BELT,
    FLAG_BIG_LURE_STATION_SQUARE,
    FLAG_BIG_LURE_MYSTIC_RUINS,
    FLAG_BIG_LURE_EGG_CARRIER,
    FLAG_BIG_LURE_5,
    FLAG_BIG_LURE_ICECAP,
};

/* each item's model and the textures it draws with. the ancient light has
 * none: level_item_draw_ancient_light draws its glow and sparks instead */
static const struct level_item_model level_item_models[LEVEL_ITEM_COUNT] = {
    {MODULE_OBJECT(0x0caafc1c), &sonic_texlist},
    {MODULE_OBJECT(0x0cab1440), &sonic_texlist},
    {NULL, NULL},
    {MODULE_OBJECT(0x0cab1e84), &tails_texlist},
    {MODULE_OBJECT(0x0c927c5c), &tails_texlist},
    {MODULE_OBJECT(0x0cb1ef80), &knuckles_texlist},
    {MODULE_OBJECT(0x0cb1db9c), &knuckles_texlist},
    {MODULE_OBJECT(0x0ca18768), &amy_texlist},
    {MODULE_OBJECT(0x0cb1d1dc), &amy_texlist},
    {MODULE_OBJECT(0x0ca17c44), &e102_texlist},
    {MODULE_OBJECT(0x0ca16fa4), &e102_texlist},
    {&big_power_rod_object, &big_texlist},
    {MODULE_OBJECT(0x0cb1f514), &big_texlist},
    {&big_lure_0, &fishing_texlist},
    {&big_lure_1, &fishing_texlist},
    {&big_lure_2, &fishing_texlist},
    {&big_lure_3, &fishing_texlist},
    {&big_lure_4, &fishing_texlist},
};

static const struct level_item_place level_item_places[LEVEL_ITEM_COUNT] = {
    {CHARACTER_SONIC, AREA_STATION_SQUARE},
    {CHARACTER_SONIC, AREA_STATION_SQUARE},
    {CHARACTER_SONIC, AREA_MYSTIC_RUINS},
    {CHARACTER_TAILS, AREA_STATION_SQUARE},
    {CHARACTER_TAILS, AREA_MYSTIC_RUINS},
    {CHARACTER_KNUCKLES, AREA_MYSTIC_RUINS},
    {CHARACTER_KNUCKLES, AREA_MYSTIC_RUINS},
    {CHARACTER_AMY, AREA_EGG_CARRIER},
    {CHARACTER_AMY, AREA_MYSTIC_RUINS},
    {CHARACTER_E102, AREA_EGG_CARRIER},
    {CHARACTER_E102, AREA_EGG_CARRIER},
    {CHARACTER_BIG, AREA_MYSTIC_RUINS},
    {CHARACTER_BIG, AREA_MYSTIC_RUINS},
    {CHARACTER_BIG, AREA_OTHER},
    {CHARACTER_BIG, AREA_OTHER},
    {CHARACTER_BIG, AREA_OTHER},
    {CHARACTER_BIG, AREA_OTHER},
    {CHARACTER_BIG, AREA_OTHER},
};

/* the textures in OBJ_REGULAR that the stand's light and the waves step
 * through by the play time. the light's are luitem01's. the waves' first
 * row is nijiune01's, for an upgrade, and the second aoune01's, for a
 * lure */
static const Uint32 level_item_light_textures[8] = {
#include "object/level_item/level_item_light_textures.inc"
};
static const Uint32 level_item_wave_textures[2][4] = {
#include "object/level_item/level_item_wave_textures.inc"
};

static const char level_item_label[] = "-> ITEM NUMBER";

/* const places the texanims among the constants, where the image has them.
 * NJS_SPRITE's tanim takes no const, so the sprites' bodies cast it away */
static const NJS_TEXANIM level_item_glow_texanims[8] = {
#include "object/level_item/level_item_glow_texanims.inc"
};

static const NJS_TEXANIM level_item_spark_texanims[8] = {
#include "object/level_item/level_item_spark_texanims.inc"
};

static const NJS_VECTOR level_item_spark_offset = {0.0F, 0.0F, 8.0F};

/* the stand's one shape, for a touch */
/* @hack type: an array of one, passed by its first element; as a plain
 * record, level_item_step saves one more register, R9, and 218 of its 265
 * words differ */
/* clang-format off */
static struct collision_shape level_item_shape[1] = {
    {SHAPE_SPHERE, 0, PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F}, 4.0F, 0.0F, 0.0F, {0}, 0, NULL}};
/* clang-format on */

/* a lure's hint, two lines, the same for every lure */
#define LURE_HINT {TEXT_1, TEXT_2, NULL}
static char *level_item_lure_hints[5][3] = {
    LURE_HINT, LURE_HINT, LURE_HINT, LURE_HINT, LURE_HINT,
};

/* each lure's hint, by its kind less LEVEL_ITEM_LURE */
static char **level_item_lure_pages[5] = {
    level_item_lure_hints[2], level_item_lure_hints[3],
    level_item_lure_hints[0], level_item_lure_hints[4],
    level_item_lure_hints[1],
};

/* the ancient light's textures: the glow's 8 frames, then the sparks' 8 */
static NJS_TEXNAME ancient_light_textures[16] = {
    {TEXT_3, 0, 0},  {TEXT_4, 0, 0},  {TEXT_5, 0, 0},  {TEXT_6, 0, 0},
    {TEXT_7, 0, 0},  {TEXT_8, 0, 0},  {TEXT_9, 0, 0},  {TEXT_10, 0, 0},
    {TEXT_11, 0, 0}, {TEXT_12, 0, 0}, {TEXT_13, 0, 0}, {TEXT_14, 0, 0},
    {TEXT_15, 0, 0}, {TEXT_16, 0, 0}, {TEXT_17, 0, 0}, {TEXT_18, 0, 0},
};
NJS_TEXLIST ancient_light_texlist = {
#include "object/level_item/ancient_light_texlist.inc"
};

static struct level_item_event level_item_events[LEVEL_ITEM_LURE] = {
    {0x320, 357}, {0x321, 358}, {0x322, 359}, {0x323, 360}, {0x324, 361},
    {0x325, 362}, {0x326, 363}, {0x327, 364}, {0x328, 365}, {0x329, 366},
    {0x330, 367}, {0x331, 368}, {0x332, 369},
};

/* each item's name, as the set editor shows it, then DASENAIYO for a kind
 * past the last */
static char *level_item_names[LEVEL_ITEM_COUNT + 1] = {
    "SONIC_CRYSTAL",
    "SONIC_RIGHT_SHOOSE",
    "SONIC_ANCIENT_LIGHT",
    "MILES_JET_ANCLET",
    "MILES_RHYTHM_BROACH",
    "KNUCKLES_FIGHTING_GLOVE",
    "KNUCKLES_SHOVEL_CROW",
    "AMY_LONG_HAMMER",
    "AMY_WING_OF_WARRIOR",
    "E102_THREEWAY_LASER",
    "E102_HI_CAPACITY_BOOSTER",
    "BIG_POWER_ROD",
    "BIG_LIFE_BELT",
    "RUAR_NORMAL",
    "RUAR_RED",
    "RUAR_BLUE",
    "RUAR_SILVER",
    "RUAR_GOLD",
    "DASENAIYO",
};

static NJS_SPRITE level_item_glow_sprite = {
#include "object/level_item/level_item_glow_sprite.inc"
};

/* @hack type: a spark's sprite as its position's three Floats and the
 * rest, drawn through a pointer to the first. level_item_draw_spark loads
 * each Float's own address from the pool (0x8c06e45a, 0x8c06e46e,
 * 0x8c06e47c). ISO C leaves the read past level_item_spark_x undefined, but
 * SHC lays the four out in a row. as one NJS_SPRITE, 20 of the function's
 * 129 words differ */
static Float level_item_spark_x = 0.0F;
static Float level_item_spark_y = 0.0F;
static Float level_item_spark_z = 0.0F;
static struct sprite_look level_item_spark_look = {
#include "object/level_item/level_item_spark_look.inc"
};
#define SPARK_SPRITE ((NJS_SPRITE *)&level_item_spark_x)

struct level_item *level_item_current;
int big_lure_count;

void level_item_create(int kind, NJS_POINT3 *pos)
{
    struct task *task = task_create(
        TASK_MOTION | TASK_WORK | TASK_FORCE | TASK_EXTRA, 3, level_item_step);
    struct task_work *work = task->work;

    KIND(work) = kind;
    work->pos = *pos;
}

void level_item_step(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    if (placement_far_from_camera(task)) {
        task->draw = NULL;
    } else {
        task->draw = level_item_draw;
        level_item_current = EXTRA(task)->item;
        if ((int)KIND(work) < 0) {
            KIND(work) = LEVEL_ITEM_COUNT - 1;
        } else if ((int)KIND(work) >= LEVEL_ITEM_COUNT) {
            KIND(work) = 0;
        }
        if (placement_editing(task)) {
            if (work->mode == LEVEL_ITEM_INIT) {
                work->mode++;
                EXTRA(task)->item = level_item_alloc();
                task->destructor = level_item_destroy;
                task->draw = level_item_draw;
            }
            level_item_current = EXTRA(task)->item;
            level_item_current->pos = work->pos;
            level_item_current->ang = work->ang;
            level_item_current->flags = ITEM_SHOWN;
            level_item_pick((int)KIND(work));
            level_item_init_sparks();
            level_item_draw(task);
            level_item_print(task);
        } else {
            switch (work->mode) {
            case LEVEL_ITEM_INIT:
                work->mode = LEVEL_ITEM_WAIT;
                EXTRA(task)->item = level_item_alloc();
                level_item_current = EXTRA(task)->item;
                task->destructor = level_item_destroy;
                /* @bug memory: collision_init takes a count of 3
                 * (MOV #3,R2 at 0x8c06d336) for the stand's one shape, so it
                 * copies two more from the 104 bytes past level_item_shape */
                collision_init(task, &level_item_shape[0], COLLISION_OBJECT, 1,
                               3);
                level_item_wave_object->model->mats->attrflags &=
                    ~(NJD_FLAG_CLAMP_U | NJD_FLAG_CLAMP_V);
                level_item_current->pos = work->pos;
                level_item_current->ang = work->ang;
                level_item_current->time = 0;
                level_item_current->flags = ITEM_SHOWN;
                level_item_init_sparks();
                for (i = 0; i < WAVES; i++) {
                    level_item_current->wave_frames[i] = 0.0F;
                    level_item_current->wave_fades[i] = -1.0F;
                }
                if (level_item_pick((int)KIND(work))) {
                    work->mode = LEVEL_ITEM_KILL;
                }
                break;
            case LEVEL_ITEM_WAIT:
                level_item_draw(task);
                level_item_touch(task);
                level_item_animate(task);
                level_item_float(task);
                level_item_watch(task);
                break;
            case LEVEL_ITEM_TAKEN:
                level_item_draw(task);
                level_item_take(task);
                level_item_animate(task);
                level_item_hint(task);
                break;
            case LEVEL_ITEM_HINT:
                level_item_hint(task);
                break;
            case LEVEL_ITEM_STILL:
                level_item_draw(task);
                break;
            case LEVEL_ITEM_KILL:
            default:
                placement_kill(task);
                break;
            }
        }
        if (level_item_current->flags & ITEM_STEP_CHILDREN) {
            task_step_children(task);
        }
    }
}

static void level_item_destroy(struct task *task)
{
    if (EXTRA(task)->item != NULL) {
        heap_free(EXTRA(task)->item);
        EXTRA(task)->item = NULL;
    }
}

static void level_item_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        level_item_current = EXTRA(task)->item;
        switch (work->mode) {
        case LEVEL_ITEM_WAIT:
            level_item_draw_stand(task);
            level_item_draw_item(task);
            break;
        case LEVEL_ITEM_TAKEN:
            level_item_draw_waves(task);
            level_item_draw_item(task);
            break;
        }
    }
}

static struct level_item *level_item_alloc(void)
{
    struct level_item *item;
    Uint8 *bytes;
    int i;

    /* @hack redundant: the new record tested with nothing to do (TST R0,R0 at
     * 0x8c06d558); assigned plainly, 17 of 25 words differ */
    if ((item = heap_alloc(sizeof(*item))) != NULL) {
    }
    bytes = (Uint8 *)item;
    /* NOLINTNEXTLINE(clang-diagnostic-sign-compare) */
    for (i = 0; i < sizeof(*item); i++) {
        bytes[i] = 0;
    }
    return item;
}

/* TRUE when the item is found in the kind of field. a lure is found in
 * any */
static Bool level_item_in_area(Sint16 kind, int area)
{
    if (kind < LEVEL_ITEM_LURE) {
        return level_item_places[kind].area == area;
    }
    return TRUE;
}

static Bool level_item_is_for(Sint16 kind, Sint16 character)
{
    return level_item_places[kind].character == character;
}

/* set *kind to the first of the character's items found in the kind of
 * field */
static void level_item_find(Sint16 character, int area, Sint32 *kind)
{
    Sint16 i;

    i = 0;
    /* @bug memory: when none of the character's items is found in
     * the field, the search runs past level_item_places' entries and reads
     * on (MOV.W @R13,R13 at 0x8c06d5f0) */
    while (!level_item_is_for(i, character) || !level_item_in_area(i, area)) {
        i++;
    }
    *kind = i;
}

/* pick the stand's item, and return TRUE when the stand is to end, else
 * FALSE. the item is kind, the placed one, when that is the character's and
 * found in this field, and otherwise the character's first item found
 * here. a lure's stand ends at once when this field's lure flag is set.
 * otherwise it counts Big's lures into big_lure_count, and shows the lure
 * of that number. a stand then ends when its item's flag in
 * level_item_flags is set, or when that item isn't the character's. for an
 * upgrade, that item is the placed one, even where another was picked */
static Bool level_item_pick(int kind)
{
    Sint16 character = get_character();
    int area;

    switch (stage_get_stage_act() >> 8) {
    case STAGE_STATION_SQUARE:
    case STAGE_STATION_SQUARE_2:
    case STAGE_STATION_SQUARE_3:
        area = AREA_STATION_SQUARE;
        break;
    case STAGE_EGG_CARRIER:
    case STAGE_EGG_CARRIER_2:
    case STAGE_EGG_CARRIER_3:
    case STAGE_EGG_CARRIER_INSIDE:
        area = AREA_EGG_CARRIER;
        break;
    case STAGE_MYSTIC_RUINS:
    case STAGE_PAST:
        area = AREA_MYSTIC_RUINS;
        break;
    default:
        area = AREA_OTHER;
    }
    if (!level_item_is_for(kind, character) ||
        !level_item_in_area(kind, area)) {
        level_item_find(character, area, &level_item_current->kind);
    } else {
        level_item_current->kind = kind;
    }
    if (level_item_current->kind >= LEVEL_ITEM_LURE) {
        if (IN_STAGE(STAGE_EGG_CARRIER_INSIDE)) {
            if (event_flag_check(FLAG_BIG_LURE_EGG_CARRIER)) {
                return TRUE;
            }
        } else if (IN_STAGE(STAGE_ICECAP)) {
            if (event_flag_check(FLAG_BIG_LURE_ICECAP)) {
                return TRUE;
            }
        } else if (IN_STAGE(STAGE_STATION_SQUARE) ||
                   IN_STAGE(STAGE_STATION_SQUARE_2) ||
                   IN_STAGE(STAGE_STATION_SQUARE_3)) {
            if (event_flag_check(FLAG_BIG_LURE_STATION_SQUARE)) {
                return TRUE;
            }
        } else if (IN_STAGE(STAGE_MYSTIC_RUINS)) {
            if (event_flag_check(FLAG_BIG_LURE_MYSTIC_RUINS)) {
                return TRUE;
            }
        }
        /* @bug logic: the count tests FLAG_BIG_LURE_EGG_CARRIER twice and
         * never tests FLAG_BIG_LURE_STATION_SQUARE or FLAG_BIG_LURE_ICECAP.
         * so it counts the egg carrier's lure twice and those two not at
         * all, where fishing_count_lures counts each once. both tests pass
         * R8, which holds the egg carrier's flag from 0x8c06d6f6 (MOV R8,R4
         * at 0x8c06d7c4 and 0x8c06d7da) */
        big_lure_count = 1;
        if (event_flag_check(FLAG_BIG_LURE_EGG_CARRIER)) {
            big_lure_count++;
        }
        if (event_flag_check(FLAG_BIG_LURE_MYSTIC_RUINS)) {
            big_lure_count++;
        }
        if (event_flag_check(FLAG_BIG_LURE_EGG_CARRIER)) {
            big_lure_count++;
        }
        if (event_flag_check(FLAG_BIG_LURE_5)) {
            big_lure_count++;
        }
        if (level_item_current->kind >= LEVEL_ITEM_LURE) {
            level_item_current->kind = big_lure_count + LEVEL_ITEM_LURE;
            kind = level_item_current->kind;
        }
    }
    /* @hack spelling: the two tests joined by |, which compares the
     * characters whatever the flag holds (MOVT R1 at 0x8c06d82e). each side
     * is 0 or 1, as event_flag_check returns a != test, and the compare has
     * no effect to skip, so | gives what || does; with ||, 29 of 221 words
     * differ */
    return event_flag_check(level_item_flags[kind]) |
           (level_item_places[kind].character != character);
}

static void level_item_draw_stand(struct task *task)
{
    struct task_work *work = task->work;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    texture_set_regular();
    draw_tree(level_item_light_object);
    draw_tree(&level_item_floor_object);
    njPopMatrix(1);
}

static void level_item_draw_waves(struct task *task)
{
    struct task_work *work = task->work;
    Float fade;
    int i;

    if (level_item_current->time < TAKE_STEPS / 2) {
        fade = level_item_current->time * (2.0F / TAKE_STEPS);
    } else {
        fade = (TAKE_STEPS - level_item_current->time) * (2.0F / TAKE_STEPS);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    texture_set_regular();
    for (i = 0; i < WAVES; i++) {
        draw_set_material(((level_item_current->wave_fades[i] + 1.0F) * fade) -
                              1.0F,
                          0.0F, 0.0F, 0.0F);
        draw_action(level_item_wave_action, level_item_current->wave_frames[i]);
    }
    if (level_item_current->time < TAKE_STEPS / 2) {
        fade = 1.0F;
    }
    /* @bug arithmetic: the floor's tint, green for an upgrade and blue for a
     * lure, divides time by TAKE_STEPS in ints (JSR to __divls at 0x8c06da5e
     * and 0x8c06da74), so it is 0 until time reaches TAKE_STEPS */
    /* NOLINTBEGIN(bugprone-integer-division) */
    if (level_item_current->kind >= LEVEL_ITEM_LURE) {
        draw_set_material(fade - 1.0F, 0.0F, 0.0F,
                          level_item_current->time / TAKE_STEPS);
    } else {
        draw_set_material(fade - 1.0F, 0.0F,
                          level_item_current->time / TAKE_STEPS, 0.0F);
    }
    /* NOLINTEND(bugprone-integer-division) */
    draw_tree(&level_item_floor_object);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPopMatrix(1);
}

static void level_item_animate(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    switch (work->mode) {
    case LEVEL_ITEM_WAIT:
        level_item_light_object->model->mats->attr_texId =
            level_item_light_textures[((Uint32)play_time_get() >> 1) & 7];
        break;
    case LEVEL_ITEM_TAKEN:
        if (level_item_current->kind >= LEVEL_ITEM_LURE) {
            level_item_wave_object->model->mats->attr_texId =
                level_item_wave_textures[1][((Uint32)play_time_get() >> 1) & 3];
        } else {
            level_item_wave_object->model->mats->attr_texId =
                level_item_wave_textures[0][((Uint32)play_time_get() >> 1) & 3];
        }
        for (i = 0; i < WAVES; i++) {
            level_item_current->wave_frames[i] += 0.25F;
            if (level_item_current->wave_frames[i] >=
                level_item_wave_action->motion->nbFrame - 1.0F) {
                level_item_current->wave_frames[i] = 0.0F;
            }
        }
        for (i = 0; i < WAVES; i++) {
            if (level_item_current->wave_frames[i] < 6.0F) {
                level_item_current->wave_fades[i] =
                    (level_item_current->wave_frames[i] * (1.0F / 6.0F)) - 1.0F;
            } else if (level_item_current->wave_frames[i] >
                       level_item_wave_action->motion->nbFrame - 7.0F) {
                level_item_current->wave_fades[i] =
                    ((level_item_wave_action->motion->nbFrame -
                      level_item_current->wave_frames[i] - 1.0F) *
                     (1.0F / 6.0F)) -
                    1.0F;
            } else {
                level_item_current->wave_fades[i] = 0.0F;
            }
        }
        break;
    }
    for (i = 0; i < SPARKS; i++) {
        level_item_current->sparks[i].frame +=
            level_item_current->sparks[i].rate;
        level_item_current->sparks[i].ang.x +=
            level_item_current->sparks[i].spin.x;
        level_item_current->sparks[i].ang.y +=
            level_item_current->sparks[i].spin.y;
    }
}

static void level_item_draw_item(struct task *task)
{
    if (level_item_current->kind >= LEVEL_ITEM_COUNT) {
        return;
    }
    if (!(level_item_current->flags & ITEM_SHOWN)) {
        return;
    }
    if (level_item_models[level_item_current->kind].object != NULL) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &level_item_current->pos);
        TURN(njRotateZ, level_item_current->ang.z);
        TURN(njRotateX, level_item_current->ang.x);
        TURN(njRotateY, level_item_current->ang.y);
        njSetTexture(level_item_models[level_item_current->kind].texlist);
        draw_tree(level_item_models[level_item_current->kind].object);
        njPopMatrix(1);
    } else if (level_item_current->kind == LEVEL_ITEM_ANCIENT_LIGHT) {
        level_item_draw_ancient_light();
    }
}

static void level_item_touch(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision = work->collision;
    struct collision_work *hit = collision->hit;

    if ((collision->flags & COLLISION_HIT) && hit->kind == COLLISION_PLAYER) {
        sound_play(SE_ITEM, 0, 0, 0);
        work->mode = LEVEL_ITEM_TAKEN;
    }
    collision_register(work);
}

static void level_item_float(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR speed;
    NJS_POINT3 pos;

    level_item_current->ang.y += NJM_DEG_ANG(3);
    level_item_current->pos.y = work->pos.y + 10.0F;
    if (rand() * RAND_SCALE < 0.1F) {
        /* @hack redundant: speed zeroed, though sparkle_create_glint takes
         * none (FMOV.S FR4,@R4 at 0x8c06deca); without the stores, 75 of 109
         * words differ */
        speed.x = 0.0F;
        speed.y = 0.0F;
        speed.z = 0.0F;
        pos.x = level_item_current->pos.x + (rand() * RAND_SCALE * 6.0F) - 3.0F;
        pos.y = level_item_current->pos.y + (rand() * RAND_SCALE * 6.0F) - 3.0F;
        pos.z = level_item_current->pos.z + (rand() * RAND_SCALE * 6.0F) - 3.0F;
        sparkle_create_glint(&pos, (rand() * RAND_SCALE * 0.25F) + 0.1F,
                             ((rand() * RAND_SCALE) + 0.5F) * 0.5F);
    }
}

static void level_item_take(struct task *task)
{
    struct task_work *work = task->work;
    int i;
    int last_upgrade;

    if (level_item_current->time == 0) {
        for (i = 0; i < WAVES; i++) {
            level_item_current->wave_frames[i] =
                i * (level_item_wave_action->motion->nbFrame - 1.0F) *
                (1.0F / WAVES);
            level_item_current->wave_fades[i] = -1.0F;
        }
    } else {
        /* @hack spelling: LAST_UPGRADE through a local, which SHC keeps in
         * R12 (MOV #12,R12 at 0x8c06e068); as LAST_UPGRADE, 140 of 185 words
         * differ */
        last_upgrade = LAST_UPGRADE;
        if (level_item_current->time == TAKE_STEPS / 2) {
            if (level_item_current->kind <= last_upgrade) {
                player_get(0)->upgrades |= 1 << level_item_current->kind;
                /* @hack redundant: the upgrade tested again (CMP/GT R12,R3 at
                 * 0x8c06e08e); tested once, 157 of 185 words differ */
                if (level_item_current->kind <= last_upgrade) {
                    player_event_end(0);
                    event_load(
                        level_item_events[level_item_current->kind].event);
                    event_start();
                }
            } else {
                big_lure_count++;
                if (big_lure_count > 4) {
                    big_lure_count = 4;
                }
            }
            level_item_set_flag(level_item_current->kind);
            level_item_current->flags &= ~ITEM_SHOWN;
        } else if (level_item_current->time == TAKE_STEPS) {
            work->mode = LEVEL_ITEM_HINT;
        } else if (level_item_current->time < TAKE_STEPS / 2 &&
                   level_item_current->kind <= last_upgrade) {
            player_event_run(0, work->pos.x, work->pos.z);
        }
    }
    level_item_current->time++;
}

static void level_item_watch(struct task *task)
{
    NJS_VECTOR away;

    away.x = player_work[0]->pos.x - level_item_current->pos.x;
    away.y = player_work[0]->pos.y - level_item_current->pos.y;
    away.z = player_work[0]->pos.z - level_item_current->pos.z;
    if (njScalor(&away) < WATCH_RANGE) {
        if (!(level_item_current->flags & ITEM_WATCHED)) {
            player_set_look_point(0, &level_item_current->pos);
        }
        level_item_current->flags |= ITEM_WATCHED;
    } else {
        if (level_item_current->flags & ITEM_WATCHED) {
            player_clear_look(0);
        }
        level_item_current->flags &= ~ITEM_WATCHED;
    }
    if (task->work->mode != LEVEL_ITEM_WAIT) {
        player_clear_look(0);
        level_item_current->flags &= ~ITEM_WATCHED;
    }
}

static void level_item_set_flag(int kind)
{
    if (kind <= LAST_UPGRADE) {
        event_flag_set(level_item_flags[kind]);
    } else if (IN_STAGE(STAGE_EGG_CARRIER_INSIDE)) {
        event_flag_set(FLAG_BIG_LURE_EGG_CARRIER);
    } else if (IN_STAGE(STAGE_ICECAP)) {
        event_flag_set(FLAG_BIG_LURE_ICECAP);
    } else if (IN_STAGE(STAGE_STATION_SQUARE) ||
               IN_STAGE(STAGE_STATION_SQUARE_2) ||
               IN_STAGE(STAGE_STATION_SQUARE_3)) {
        event_flag_set(FLAG_BIG_LURE_STATION_SQUARE);
    } else if (IN_STAGE(STAGE_MYSTIC_RUINS)) {
        event_flag_set(FLAG_BIG_LURE_MYSTIC_RUINS);
    }
}

static void level_item_print(struct task *task)
{
    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), level_item_label);
    njPrintC(NJM_LOCATION(20, 20), level_item_names[level_item_current->kind]);
}

static void level_item_draw_ancient_light(void)
{
    int i;

    level_item_draw_glow();
    for (i = 0; i < SPARKS; i++) {
        level_item_draw_spark(i);
    }
}

static void level_item_draw_glow(void)
{
    level_item_glow_sprite.p = level_item_current->pos;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njDrawSprite3D(&level_item_glow_sprite, play_time_get() & 7,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

static void level_item_draw_spark(int spark)
{
    NJS_VECTOR offset;

    offset = level_item_spark_offset;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateX, level_item_current->sparks[spark].ang.x);
    TURN(njRotateY, level_item_current->sparks[spark].ang.y);
    njCalcVector(NULL, &offset, &offset);
    njPopMatrix(1);
    level_item_spark_x = level_item_current->pos.x + offset.x;
    level_item_spark_y = level_item_current->pos.y + offset.y;
    level_item_spark_z = level_item_current->pos.z + offset.z;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njDrawSprite3D(SPARK_SPRITE,
                   (int)level_item_current->sparks[spark].frame % 8U,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
}

static void level_item_init_sparks(void)
{
    int i;

    for (i = 0; i < SPARKS; i++) {
        level_item_current->sparks[i].ang.x =
            (Angle)((i * 45) * ANGLE_PER_DEGREE);
        level_item_current->sparks[i].spin.y =
            (Angle)((3.0F + (rand() * RAND_SCALE * 2.0F)) * ANGLE_PER_DEGREE);
        if (rand() * RAND_SCALE > 0.5F) {
            level_item_current->sparks[i].spin.y =
                -level_item_current->sparks[i].spin.y;
        }
        level_item_current->sparks[i].rate =
            1.0F / (1.0F + (rand() * RAND_SCALE * 5.0F));
    }
}

static void level_item_hint(struct task *task)
{
    level_item_current->hint_time++;
    switch (level_item_current->hint_mode) {
    case HINT_WAIT:
        if (level_item_current->hint_time > HINT_STEPS) {
            level_item_current->hint_mode = HINT_SHOWN;
            if (level_item_current->kind >= LEVEL_ITEM_LURE) {
                bitmap_show_pages(
                    level_item_lure_pages[level_item_current->kind -
                                          LEVEL_ITEM_LURE]);
            }
        }
        break;
    case HINT_SHOWN:
        if (level_item_current->hint_time > DONE_STEPS) {
            task->work->mode = LEVEL_ITEM_KILL;
            level_item_current->flags &= ~ITEM_STEP_CHILDREN;
        }
        break;
    }
}
