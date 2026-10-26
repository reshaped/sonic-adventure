/* placement: the objects an act's set file lists, and the tasks that run
 * them */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "debug/editor.h"
#include "object/placement.h"
#include "player/fishing.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg00/object_list.h"
#include "system/loader.h"
#include "system/task.h"

/* the leave range, squared, of a task without a placement */
#define LEAVE_RANGE (410.0F * 410.0F)
/* the reach of player 1, squared. the manager places an object within it,
 * and a range check keeps a task within it while player 1 is Sonic */
#define NEAR_RANGE (40.0F * 40.0F)

/* each act's set file, where the loader puts it */
#define SET_FILE_0 ((struct set_file *)SET_AREA_0)
#define SET_FILE_1 ((struct set_file *)SET_AREA_1)
#define SET_FILE_2 ((struct set_file *)SET_AREA_2)
#define SET_FILE_3 ((struct set_file *)SET_AREA_3)
#define SET_FILE_4 ((struct set_file *)SET_AREA_4)
#define SET_FILE_5 ((struct set_file *)SET_AREA_5)

/* each stage's object lists sit in the module it loads */
#define LIST(address) ((struct object_list *)(address))
/* the practice stage's list, which acts that don't exist point at too */
#define PRACTICE (&practice_object_list)

enum manager_mode {
    MANAGER_INIT,
    MANAGER_PLACE, /* place the act's objects */
    MANAGER_CHANGE /* change to the act placement_change_act asks for */
};

/* the change's stages, one a step, in the manager's submode */
enum change_step { CHANGE_TAKE, CHANGE_WAIT, CHANGE_COMPACT, CHANGE_PLACE };

/* player_work, as a pair */
struct players {
    struct task_work *work[2];
};

static void start_asked_change(struct task_work *work);
static void load_act(void);
static void place_near_players(void);
static void change_act(struct task_work *work);
static void clear_placements(void);
static void add_placements(void);
static void release_placements(void);
static void compact_placements(void);

/* FALSE from placement_change_act until the new act's placements go in,
 * and TRUE otherwise. nothing reads it */
static Bool act_ready = TRUE;
/* TRUE from placement_change_act until the manager starts the change */
static Bool change_asked = FALSE;
/* TRUE once the manager has placed objects since MANAGER_INIT. until then,
 * a KIND_FIRST kind is placed at any distance */
static Bool placed_once = FALSE;
static int dat_8c1d722c = 0;

static struct set_file *set_files[STAGE_COUNT][STAGE_ACTS] = {
    {SET_FILE_0, SET_FILE_1, SET_FILE_2},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2, SET_FILE_3},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2, SET_FILE_3},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2, SET_FILE_3, SET_FILE_4, SET_FILE_5},
    {NULL},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2, SET_FILE_3},
    {NULL},
    {NULL},
    {SET_FILE_0},
    {SET_FILE_0},
    {SET_FILE_0},
    {SET_FILE_0, SET_FILE_1},
    {SET_FILE_0},
    {SET_FILE_0},
    {SET_FILE_0},
    {SET_FILE_0},
    {SET_FILE_0},
    {SET_FILE_0},
    {SET_FILE_0},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2, SET_FILE_3, SET_FILE_4, SET_FILE_5},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2, SET_FILE_3, SET_FILE_4, SET_FILE_5},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2, SET_FILE_3, SET_FILE_4, SET_FILE_5},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2, SET_FILE_3, SET_FILE_4, SET_FILE_5},
    {SET_FILE_0},
    {SET_FILE_0},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2, SET_FILE_3, SET_FILE_4, SET_FILE_5},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2, SET_FILE_3, SET_FILE_4},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2},
    {SET_FILE_0, SET_FILE_1, SET_FILE_2, SET_FILE_3, SET_FILE_4, SET_FILE_5},
    {SET_FILE_0, SET_FILE_1},
    {SET_FILE_0},
    {SET_FILE_0, SET_FILE_1},
    {SET_FILE_0},
    {SET_FILE_0},
    {SET_FILE_0},
    {SET_FILE_0, SET_FILE_1},
};

static struct object_list *object_lists[STAGE_COUNT][STAGE_ACTS] = {
    {PRACTICE, PRACTICE, PRACTICE, PRACTICE, PRACTICE},
    {LIST(0x0caa92d8), LIST(0x0caa92d8), LIST(0x0caa92d8), PRACTICE, PRACTICE},
    {LIST(0x0c9ec3a0), LIST(0x0c9ec3a0), LIST(0x0c9ec3a0), PRACTICE, PRACTICE},
    {LIST(0x0c9c3510), LIST(0x0c9c3510), LIST(0x0c9c3510), PRACTICE, PRACTICE},
    {LIST(0x0ca790b0), LIST(0x0ca790b0), LIST(0x0ca790b0), PRACTICE, PRACTICE},
    {LIST(0x0cadb9d4), LIST(0x0cadb9d4), LIST(0x0cadb9d4), PRACTICE, PRACTICE},
    {LIST(0x0cadd3a4), LIST(0x0cadd3a4), LIST(0x0cadd3a4), PRACTICE, PRACTICE},
    {LIST(0x0ca5b910), LIST(0x0ca5b910), LIST(0x0ca5b910), PRACTICE, PRACTICE},
    {LIST(0x0cabe85c), LIST(0x0cabe85c), LIST(0x0cabe85c), LIST(0x0cabe85c),
     PRACTICE},
    {LIST(0x0cb13968), LIST(0x0cb13968), LIST(0x0cb13968), LIST(0x0cb13968),
     PRACTICE},
    {LIST(0x0cafeb8c), LIST(0x0cafeb8c), LIST(0x0cafeb8c), LIST(0x0cafeb8c),
     LIST(0x0cafeb8c), LIST(0x0cafeb8c)},
    {PRACTICE, PRACTICE, PRACTICE, PRACTICE, PRACTICE},
    {LIST(0x0ca89fc4), LIST(0x0ca8a6c4), LIST(0x0ca8ae00), LIST(0x0ca8b4d4),
     PRACTICE},
    {PRACTICE, PRACTICE, PRACTICE, PRACTICE, PRACTICE},
    {PRACTICE, PRACTICE, PRACTICE, PRACTICE, PRACTICE},
    {LIST(0x0c900200), PRACTICE, PRACTICE, PRACTICE, PRACTICE},
    {LIST(0x0c9002e0), PRACTICE, PRACTICE, PRACTICE, PRACTICE},
    {LIST(0x0c900288), PRACTICE, PRACTICE, PRACTICE, PRACTICE},
    {LIST(0x0c900368), LIST(0x0c900368), PRACTICE, PRACTICE, PRACTICE},
    {LIST(0x0c901278), PRACTICE, PRACTICE, PRACTICE, PRACTICE},
    {LIST(0x0c900158), LIST(0x0c900158), LIST(0x0c900158), LIST(0x0c900158),
     LIST(0x0c900158)},
    {LIST(0x0c900258), LIST(0x0c900258), LIST(0x0c900258), LIST(0x0c900258),
     LIST(0x0c900258)},
    {LIST(0x0c9588cc), LIST(0x0c9588cc), LIST(0x0c9588cc), LIST(0x0c9588cc),
     LIST(0x0c9588cc)},
    {LIST(0x0c900110), PRACTICE, PRACTICE, PRACTICE, PRACTICE},
    {LIST(0x0c900098), PRACTICE, PRACTICE, PRACTICE, PRACTICE},
    {LIST(0x0c9001b0), PRACTICE, PRACTICE, PRACTICE, PRACTICE},
    {LIST(0x0ca5ec4c), LIST(0x0ca5ec4c), LIST(0x0ca5ec4c), LIST(0x0ca5ec4c),
     LIST(0x0ca5ec4c), LIST(0x0ca5ec4c)},
    {LIST(0x0ca5ec4c), LIST(0x0ca5ec4c), LIST(0x0ca5ec4c), LIST(0x0ca5ec4c),
     LIST(0x0ca5ec4c), LIST(0x0ca5ec4c)},
    {LIST(0x0ca5ec4c), LIST(0x0ca5ec4c), LIST(0x0ca5ec4c), LIST(0x0ca5ec4c),
     LIST(0x0ca5ec4c), LIST(0x0ca5ec4c)},
    {LIST(0x0c923b58), LIST(0x0c923b58), LIST(0x0c923b58), LIST(0x0c923b58),
     LIST(0x0c923b58), LIST(0x0c923b58)},
    {PRACTICE, PRACTICE, PRACTICE, PRACTICE, PRACTICE},
    {PRACTICE, PRACTICE, PRACTICE, PRACTICE, PRACTICE},
    {LIST(0x0c920b80), LIST(0x0c920b80), LIST(0x0c920b80), LIST(0x0c920b80),
     LIST(0x0c920b80), LIST(0x0c920b80)},
    {LIST(0x0c9001c8), LIST(0x0c9001c8), LIST(0x0c9001c8), LIST(0x0c9001c8),
     LIST(0x0c9001c8)},
    {LIST(0x0c9002f0), LIST(0x0c9002f0), LIST(0x0c9002f0)},
    {LIST(0x0c90a0d8), LIST(0x0c90a0d8), LIST(0x0c90a0d8), LIST(0x0c90a0d8),
     LIST(0x0c90a0d8), LIST(0x0c90a0d8)},
    {LIST(0x0c955b70), LIST(0x0c955b70), LIST(0x0c955b70), LIST(0x0c955b70),
     LIST(0x0c955b70)},
    {LIST(0x0c955b70), LIST(0x0c955b70), LIST(0x0c955b70), LIST(0x0c955b70),
     LIST(0x0c955b70)},
    {LIST(0x0c902e60), LIST(0x0c902e60)},
    {LIST(0x0cb931a0), LIST(0x0cb931a0), LIST(0x0cb931a0), LIST(0x0cb931a0),
     LIST(0x0cb931a0)},
    {LIST(0x0cb90210), LIST(0x0cb90210), LIST(0x0cb90210), LIST(0x0cb90210),
     LIST(0x0cb90210)},
    {LIST(0x0cb9b8dc), LIST(0x0cb9b8dc), LIST(0x0cb9b8dc), LIST(0x0cb9b8dc),
     LIST(0x0cb9b8dc)},
    {LIST(0x0cb80e98), LIST(0x0cb80e98), LIST(0x0cb80e98), LIST(0x0cb80e98),
     LIST(0x0cb80e98)},
};

struct object_list *placement_list;
struct set_file *placement_file;
Uint32 *placement_file_count;
struct set_entry *placement_entries;
Sint16 placement_count;
struct placement placements[PLACEMENT_COUNT];
/* the stage and act whose set file the manager places */
static Sint8 placed_stage;
static Sint8 placed_act;

void placement_manager_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case MANAGER_INIT:
        placement_file = NULL;
        placed_stage = 0;
        placed_act = 0;
        act_ready = TRUE;
        change_asked = FALSE;
        load_act();
        work->mode = MANAGER_PLACE;
        placed_once = FALSE;
        break;
    case MANAGER_PLACE:
        place_near_players();
        start_asked_change(work);
        break;
    case MANAGER_CHANGE:
        change_act(work);
        break;
    }
}

/* start the act change placement_change_act asked for, if it asked */
static void start_asked_change(struct task_work *work)
{
    if (change_asked) {
        work->mode = MANAGER_CHANGE;
        work->submode = CHANGE_TAKE;
        change_asked = FALSE;
    }
}

/* clear the placements, and take the object list and set file of the stage
 * and act the game is in. then add a placement for each entry */
static void load_act(void)
{
    Uint16 stage_act = stage_get_stage_act();

    placed_stage = STAGE_OF(stage_act);
    placed_act = stage_act & 0xff;
    clear_placements();
    placement_list = object_lists[placed_stage][placed_act];
    placement_file = set_files[placed_stage][placed_act];
    if (placement_file == NULL) {
        placement_file_count = NULL;
        placement_entries = NULL;
    } else {
        placement_file_count = &placement_file->count;
        placement_entries = (struct set_entry *)(placement_file + 1);
    }
    add_placements();
}

/* TRUE when the point at pos is within range of x, y and z. TRUE too when
 * other isn't null and the point at it is within NEAR_RANGE of pos. FALSE
 * otherwise. range is squared */
static Bool in_range_or_near(Float *pos, Float x, Float y, Float z, Float range,
                             Float *other)
{
    Float px;
    Float py;
    Float pz;
    Float distance;

    /* @hack spelling: pos and other each address a point's x, and the reads
     * walk on to its y and z, as the image's @R4+ and @R5+ loads do
     * (0x8c096240, 0x8c096268). ISO C leaves the walk undefined, since each
     * member is an object of its own. through NJS_POINT3 members, 45 of 45
     * words differ */
    px = *pos++;
    py = *pos++;
    pz = *pos;
    x -= px;
    distance = x * x;
    if (distance <= range) {
        y -= py;
        distance += y * y;
        if (distance <= range) {
            z -= pz;
            distance += z * z;
            if (distance <= range) {
                return TRUE;
            }
        }
    }
    if (other != NULL) {
        /* @hack spelling: range, reused for NEAR_RANGE, which keeps it in FR7
         * (FMOV.S @R0,FR7 at 0x8c09626c); with NEAR_RANGE in each test, SHC
         * loads it into FR5 and 4 of 45 words differ */
        range = NEAR_RANGE;
        px -= *other++;
        distance = px * px;
        if (distance <= range) {
            py -= *other++;
            distance += py * py;
            if (distance <= range) {
                pz -= *other;
                distance += pz * pz;
                if (distance <= range) {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

/* TRUE when the point at pos is within range of x, y and z, else FALSE.
 * range is squared */
static Bool in_range(NJS_POINT3 *pos, Float x, Float y, Float z, Float range)
{
    Float px;
    Float py;
    Float pz;
    Float distance;
    const char *rest;

    px = pos->x;
    /* @hack spelling: y and z read through a byte cursor past x, for the
     * image's @R4+ loads (FMOV.S @R4+ at 0x8c09629a, 0x8c09629e); through
     * the members, the function runs 4 bytes long and 23 of 23 words
     * differ */
    rest = (const char *)pos + sizeof(Float);
    py = *(const Float *)rest;
    rest += sizeof(Float);
    pz = *(const Float *)rest;
    x -= px;
    distance = x * x;
    if (distance <= range) {
        y -= py;
        distance += y * y;
        if (distance <= range) {
            z -= pz;
            distance += z * z;
            if (distance <= range) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* give a task to each active placement without a live one that is in
 * range of player 0, or within NEAR_RANGE of player 1. the camera stands in
 * for player 0 while there is none or the set editor is open */
static void place_near_two(void)
{
    struct players *players;
    /* @hack redundant: register on other, x, y, z and the rest below, which
     * puts x, y and z in FR14, FR13 and FR15, range in FR12, other in R12
     * and placement in R11, in a 52-byte frame (0x8c0962e0 to 0x8c096320);
     * without them, 142 of 227 words differ */
    register Float *other;
    register Float x;
    register Float y;
    register Float z;
    Float range;
    register struct placement *placement;
    register struct set_entry *entry;
    register struct object_kind *kind;
    register struct task *task;
    register struct task_work *work;
    register int flags;
    int saved;
    register Bool near;
    int attr;
    register int i;

    /* @hack type: player_work read as a pair, so player 1 loads at @(4,R4)
     * from its base (0x8c096306); through player_work itself, 21 of 227
     * words differ */
    players = (struct players *)player_work;
    if (players->work[0] == NULL || edit_mode == 1) {
        if (camera_work == NULL) {
            return;
        }
        x = camera_work->pos.x;
        y = camera_work->pos.y;
        z = camera_work->pos.z;
    } else {
        x = players->work[0]->pos.x;
        y = players->work[0]->pos.y;
        z = players->work[0]->pos.z;
    }
    /* @hack spelling: a loop around the test, which keeps z's load ahead of it
     * (FMOV.S @(R0,R5),FR15 at 0x8c096310); as an if, SHC loads z in the
     * test's delay slot and 4 of 227 words differ */
    while (players->work[1] == NULL) {
        goto no_other;
    }
    other = &players->work[1]->pos.x;
    goto have_other;
no_other:
    other = NULL;
have_other:
    placement = placements;
    for (i = placement_count - 1; i >= 0; placement++, i--) {
        flags = placement->flags;
        /* @hack spelling: PLACEMENT_ACTIVE tested as flags' sign bit, which SHC
         * tests with CMP/PZ (0x8c096364); as flags & PLACEMENT_ACTIVE, SHC
         * loads 0x8000 from the pool and 51 of 227 words differ */
        if (flags >= 0 || (flags & PLACEMENT_LIVE)) {
            continue;
        }
        entry = placement->entry;
        kind = &placement_list->kinds[entry->id & SET_KIND];
        saved = flags & PLACEMENT_SAVED;
        /* @hack spelling: the kind's tests as a loop of one pass, which the
         * kinds placed at any distance break out of. it keeps other in R12
         * and placement in R11 (0x8c096316, 0x8c096320); with a goto past
         * the tests, they take R14 and R13 and 124 of 227 words differ */
        do {
            attr = kind->attr;
            if (attr & KIND_RANGE) {
                range = kind->range;
            } else if (attr & KIND_ALWAYS) {
                /* -LEAVE_MARGIN gives the placement a leave.range of 0,
                 * which the range checks read as always in range */
                range = -LEAVE_MARGIN;
                near = TRUE;
                break;
            } else {
                range = PLACE_RANGE;
            }
            if ((attr & KIND_FIRST) && !placed_once) {
                near = TRUE;
                break;
            } else if (!saved) {
                near = in_range_or_near(&entry->pos.x, x, y, z, range, other);
            } else {
                near = in_range_or_near(&placement->leave.pose->pos.x, x, y, z,
                                        range, other);
            }
        } while (0);
        if (!near) {
            continue;
        }
        task = task_create(kind->create, kind->level, kind->step);
        if (task != NULL) {
            placement->tasks++;
            placement->flags |= PLACEMENT_LIVE;
            task->placement = placement;
            work = task->work;
            if (work != NULL) {
                if (saved) {
                    work->pos = placement->leave.pose->pos;
                    work->ang = placement->leave.pose->ang;
                    work->scl = placement->leave.pose->scl;
                    heap_free(placement->leave.pose);
                    placement->flags &= ~PLACEMENT_SAVED;
                } else {
                    /* @hack type: the copies through &pos.x and &scl.x,
                     * which form each base in a register first (ADD #8,R1 at
                     * 0x8c096498, ADD #20,R3 at 0x8c0964a8); as entry->pos
                     * and entry->scl, SHC copies at offsets from entry and
                     * 61 of 227 words differ */
                    work->pos = *(NJS_POINT3 *)&entry->pos.x;
                    work->ang.x = entry->ang_x;
                    work->ang.y = entry->ang_y;
                    work->ang.z = entry->ang_z;
                    work->scl = *(NJS_POINT3 *)&entry->scl.x;
                }
                work->vars.p[0] = kind->param;
            }
            placement->leave.range = range + LEAVE_MARGIN;
            placement->task = task;
        }
    }
    placed_once = TRUE;
}

/* give a task to each active placement without a live one that is in
 * range of player 0. the camera stands in for player 0 while there is none
 * or the set editor is open. the camera also stands in during the alife
 * race, and for Big while the fishing game has the camera */
static void place_near_one(void)
{
    /* @hack redundant: register on center, x, y, z and the rest below, which
     * puts x, y and z in FR14, FR13 and FR15, the count in R9 and placement
     * in R12, in a 52-byte frame (0x8c096538 to 0x8c096596); without them,
     * 133 of 237 words differ */
    register struct task_work *center;
    register Float x;
    register Float y;
    register Float z;
    Float range;
    register struct placement *placement;
    register struct set_entry *entry;
    register struct object_kind *kind;
    register struct task *task;
    register struct task_work *work;
    register int flags;
    int saved;
    register Bool near;
    int attr;
    register int i;

    if (player_work[0] == NULL || edit_mode == 1 ||
        IN_STAGE(STAGE_ALIFE_RACE) ||
        (get_character() == CHARACTER_BIG &&
         (fishing_flags & FISHING_CAMERA))) {
        center = camera_work;
        if (center == NULL) {
            return;
        }
    } else {
        center = player_work[0];
    }
    x = center->pos.x;
    y = center->pos.y;
    z = center->pos.z;
    placement = placements;
    for (i = placement_count - 1; i >= 0; placement++, i--) {
        flags = placement->flags;
        /* @hack spelling: PLACEMENT_ACTIVE tested as the sign bit, as in
         * place_near_two (CMP/PZ at 0x8c0965e4); as flags & PLACEMENT_ACTIVE,
         * 51 of 237 words differ */
        if (flags >= 0 || (flags & PLACEMENT_LIVE)) {
            continue;
        }
        entry = placement->entry;
        kind = &placement_list->kinds[entry->id & SET_KIND];
        saved = flags & PLACEMENT_SAVED;
        /* @hack spelling: the kind's tests as a loop of one pass, as in
         * place_near_two. it keeps the 1 that both breaks store in near in
         * R8, set ahead of the loop (MOV #1,R8 at 0x8c09659a); with a goto
         * past the tests, 203 of 237 words differ */
        do {
            attr = kind->attr;
            if (attr & KIND_RANGE) {
                range = kind->range;
            } else if (attr & KIND_ALWAYS) {
                range = -LEAVE_MARGIN;
                near = TRUE;
                break;
            } else {
                range = PLACE_RANGE;
            }
            if ((attr & KIND_FIRST) && !placed_once) {
                near = TRUE;
                break;
            } else if (!saved) {
                near = in_range(&entry->pos, x, y, z, range);
            } else {
                near = in_range(&placement->leave.pose->pos, x, y, z, range);
            }
        } while (0);
        if (!near) {
            continue;
        }
        task = task_create(kind->create, kind->level, kind->step);
        if (task != NULL) {
            placement->tasks++;
            placement->flags |= PLACEMENT_LIVE;
            task->placement = placement;
            work = task->work;
            if (work != NULL) {
                if (saved) {
                    work->pos = placement->leave.pose->pos;
                    work->ang = placement->leave.pose->ang;
                    work->scl = placement->leave.pose->scl;
                    heap_free(placement->leave.pose);
                    placement->flags &= ~PLACEMENT_SAVED;
                } else {
                    /* @hack type: the copies through &pos.x and &scl.x, as
                     * in place_near_two (ADD #8,R1 at 0x8c096714, ADD #20,R3
                     * at 0x8c096724); as entry->pos and entry->scl, 58 of
                     * 237 words differ */
                    work->pos = *(NJS_POINT3 *)&entry->pos.x;
                    work->ang.x = entry->ang_x;
                    work->ang.y = entry->ang_y;
                    work->ang.z = entry->ang_z;
                    work->scl = *(NJS_POINT3 *)&entry->scl.x;
                }
                work->vars.p[0] = kind->param;
            }
            placement->leave.range = range + LEAVE_MARGIN;
            placement->task = task;
        }
    }
}

/* place around player 0 alone while there is no player 1, the set editor
 * is open or player 1 is Tails, and around both otherwise */
static void place_near_players(void)
{
    if (player_work[1] == NULL || edit_mode == 1 ||
        PLAYER_VARS(player_work[1])->character == CHARACTER_TAILS) {
        place_near_one();
    } else {
        place_near_two();
    }
    placed_once = TRUE;
}

/* the act change, a stage a step in submode. CHANGE_TAKE takes the new
 * act's object list and set file, and releases the old placements.
 * CHANGE_COMPACT compacts the placements, and CHANGE_PLACE adds the new
 * act's */
static void change_act(struct task_work *work)
{
    switch (work->submode) {
    case CHANGE_TAKE:
        placement_list = object_lists[placed_stage][placed_act];
        placement_file = set_files[placed_stage][placed_act];
        if (placement_file == NULL) {
            placement_file_count = NULL;
            placement_entries = NULL;
        } else {
            placement_file_count = &placement_file->count;
            placement_entries = (struct set_entry *)(placement_file + 1);
        }
        release_placements();
        break;
    case CHANGE_COMPACT:
        compact_placements();
        break;
    case CHANGE_PLACE:
        act_ready = TRUE;
        work->mode = MANAGER_PLACE;
        add_placements();
        break;
    }
    work->submode++;
}

static void clear_placements(void)
{
    Sint16 i = PLACEMENT_COUNT - 1;

    placement_count = 0;
    while (i >= 0) {
        placements[i].tasks = 0;
        placements[i].state = 0;
        placements[i].flags = 0;
        placements[i].entry = NULL;
        placements[i].leave.pose = NULL;
        i--;
    }
}

/* give each entry of the set file an active placement, and set
 * placement_count to their count, up to PLACEMENT_COUNT. i counts from
 * placement_count, but placement from placements[0]. the two agree only
 * because placement_count is 0 whenever add_placements runs */
static void add_placements(void)
{
    struct set_entry *entry;
    struct placement *placement;
    Uint32 count;
    Sint16 first;
    int last;
    int i;

    if (placement_entries == NULL) {
        placement_count = 0;
        return;
    }
    entry = placement_entries;
    count = *placement_file_count;
    /* @hack spelling: first set inside the sum, which keeps it and count in the
     * frame (ADD #-8,R15 at 0x8c0968de); set apart, first takes R12 and 35
     * of 48 words differ */
    last = (first = placement_count) + count > PLACEMENT_COUNT ? PLACEMENT_COUNT
                                                               : first + count;
    /* @bug memory: the loop runs through last, one placement past the
     * entries (CMP/GT R2,R6 at 0x8c096950). at the cap, last is
     * PLACEMENT_COUNT, and the stores land past placements, on placed_stage,
     * placed_act and the bytes after them */
    for (i = first, placement = placements; i <= last;
         i++, entry++, placement++) {
        placement->flags |= PLACEMENT_ACTIVE;
        placement->tasks = 0;
        placement->state = 0;
        placement->entry = entry;
    }
    placement_count = last;
}

/* stop placing every placement but the held ones, and destroy their live
 * tasks. it clears PLACEMENT_SAVED without freeing the saved pose */
static void release_placements(void)
{
    int kept;
    int i;

    /* @bug logic: the held placements are lost. kept, their count, never
     * counts up, so placement_count ends at 0. compact_placements then has
     * nothing to compact, and add_placements writes the new act's
     * placements from placements[0] on, over any held one there. the image
     * keeps kept's 0 in the frame (MOV.L R7,@R15 at 0x8c096988) and stores
     * it as the count from there (MOV.L @R15,R3 at 0x8c0969b6, then
     * MOV.W R3,@R14 at 0x8c0969b8) */
    for (kept = i = 0; i < placement_count; i++) {
        if (!(placements[i].flags & PLACEMENT_HELD)) {
            placements[i].flags &= ~(PLACEMENT_ACTIVE | PLACEMENT_KEEP |
                                     PLACEMENT_USED | PLACEMENT_SAVED);
            if (placements[i].flags & PLACEMENT_LIVE) {
                placements[i].task->step = task_destroy;
            }
        }
    }
    placement_count = kept;
}

/* move the held placements to the front, and clear the rest */
static void compact_placements(void)
{
    int kept;
    int i;

    for (kept = i = 0; i < placement_count; i++) {
        if (placements[i].flags & PLACEMENT_HELD) {
            placements[i].flags &= ~PLACEMENT_HELD;
            placements[kept++] = placements[i];
        } else if (!(placements[i].flags & PLACEMENT_SAVED)) {
            placements[i].flags = 0;
            placements[i].entry = NULL;
        } else {
            placements[i].flags = 0;
            placements[i].entry = NULL;
            heap_free(placements[i].leave.pose);
            placements[i].leave.pose = NULL;
        }
    }
    placement_count = kept;
}

void placement_reset(void)
{
    struct placement *placement;
    Uint32 count;
    int last;
    Sint16 flags;
    int i;

    if (placement_entries == NULL) {
        return;
    }
    count = *placement_file_count;
    last = placement_count + count > PLACEMENT_COUNT ? PLACEMENT_COUNT
                                                     : placement_count + count;
    placement = placements;
    for (i = 0; i < last; i++) {
        flags = placement->flags;
        if (!(flags & PLACEMENT_KEEP)) {
            placement->flags = PLACEMENT_ACTIVE;
            placement->state = 0;
        }
        placement++;
    }
}

void placement_change_act(Sint16 change)
{
    act_ready = FALSE;
    change_asked = TRUE;
    placed_act += change;
}

Bool placement_far_from_camera(struct task *task)
{
    if (task->placement != NULL) {
        return placement_far_from_camera_by(task, task->placement->leave.range);
    }
    return placement_far_from_camera_by(task, LEAVE_RANGE);
}

Bool placement_far_from_camera_by(struct task *task, Float range)
{
    struct placement *placement = task->placement;
    struct task_work *work;
    Float dx;
    Float dy;
    Float dz;
    Float distance;

    if (placement != NULL && (placement->flags & PLACEMENT_IN_RANGE)) {
        return FALSE;
    }
    if (range == 0.0F) {
        return FALSE;
    }
    work = task->work;
    dx = work->pos.x - camera_work->pos.x;
    dy = work->pos.y - camera_work->pos.y;
    dz = work->pos.z - camera_work->pos.z;
    distance = (dx * dx) + (dy * dy) + (dz * dz);
    if (distance > range) {
        return TRUE;
    }
    return FALSE;
}

Bool placement_try_leave(struct task *task)
{
    if (task->placement != NULL) {
        return placement_try_leave_by(task, task->placement->leave.range);
    }
    return placement_try_leave_by(task, LEAVE_RANGE);
}

Bool placement_try_leave_by(struct task *task, Float range)
{
    struct placement *placement = task->placement;
    struct task_work *viewer;
    Float x;
    Float y;
    Float z;

    if (placement != NULL && (placement->flags & PLACEMENT_IN_RANGE)) {
        return FALSE;
    }
    if (range == 0.0F) {
        return FALSE;
    }
    x = task->work->pos.x;
    y = task->work->pos.y;
    z = task->work->pos.z;
    /* @hack spelling: each viewer set in its test, which stores it to the frame
     * and loads it back (MOV.L R3,@R15 at 0x8c096c40, MOV.L @R15,R4 at
     * 0x8c096c42); set apart, it stays in R13 and 79 of 85 words differ */
    if ((viewer = camera_work) != NULL &&
        in_range(&viewer->pos, x, y, z, range)) {
        return FALSE;
    }
    if ((viewer = player_work[0]) != NULL &&
        in_range(&viewer->pos, x, y, z, range)) {
        return FALSE;
    }
    if (player_work[1] != NULL &&
        PLAYER_VARS(player_work[1])->character == CHARACTER_SONIC &&
        in_range(&player_work[1]->pos, x, y, z, NEAR_RANGE)) {
        return FALSE;
    }
    task->step = task_destroy;
    return TRUE;
}

Bool placement_try_leave_saving(struct task *task)
{
    if (task->placement != NULL) {
        return placement_try_leave_saving_by(task,
                                             task->placement->leave.range);
    }
    return placement_try_leave_saving_by(task, LEAVE_RANGE);
}

Bool placement_try_leave_saving_by(struct task *task, Float range)
{
    struct placement *placement = task->placement;
    struct placement_pose *pose;
    struct task_work *work;

    if (placement == NULL || !(placement->flags & PLACEMENT_IN_RANGE)) {
        if (placement_try_leave_by(task, range)) {
            /* @hack spelling: pose assigned in the test, which tests
             * heap_alloc's R0 and copies it to R4 in the BT/S's slot
             * (0x8c096d14, 0x8c096d18); assigned apart, SHC tests R4 and 3 of
             * 54 words differ */
            if ((pose = heap_alloc(sizeof(*pose))) != NULL) {
                work = task->work;
                pose->pos = work->pos;
                pose->ang = work->ang;
                pose->scl = work->scl;
                /* @bug memory: a task without a placement gets here too, and
                 * the pose and PLACEMENT_SAVED are stored through its null
                 * placement (0x8c096d42, 0x8c096d4a) */
                task->placement->leave.pose = pose;
                task->placement->flags |= PLACEMENT_SAVED;
            }
            task->step = task_destroy;
            return TRUE;
        }
    }
    return FALSE;
}

void placement_set_used(struct task *task)
{
    task->placement->flags |= PLACEMENT_USED;
}

void placement_keep(struct task *task)
{
    task->placement->flags |= PLACEMENT_KEEP;
}

int placement_used(struct task *task)
{
    return task->placement->flags & PLACEMENT_USED;
}

void placement_hold(struct task *task)
{
    task->placement->flags |= PLACEMENT_HELD;
}

int placement_held(struct task *task)
{
    return task->placement->flags & PLACEMENT_HELD;
}

void placement_stop(struct task *task)
{
    task->placement->flags &= ~PLACEMENT_ACTIVE;
    task->placement->flags &= ~PLACEMENT_LIVE;
}

void placement_kill(struct task *task)
{
    task->placement->flags &= ~PLACEMENT_ACTIVE;
    task->placement->flags &= ~PLACEMENT_LIVE;
    task->step = task_destroy;
}

Uint16 placement_get_kind(struct task *task)
{
    return task->placement->entry->id & SET_KIND;
}

void placement_set_state(struct task *task, Sint8 state)
{
    if (task->placement != NULL) {
        task->placement->state = state;
    }
}

Sint8 placement_get_state(struct task *task)
{
    if (task->placement != NULL) {
        return task->placement->state;
    }
    /* @bug uninitialized: without a placement, the function returns the R0
     * its caller left, as the BT at 0x8c096e34 skips to the RTS */
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}
