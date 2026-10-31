/* downloads: a card's event file. it adds items and a timed mission to the
 * acts it names, and textures, a sound bank and a model for the items */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "collision/mesh.h"
#include "collision/world.h"
#include "draw/model.h"
#include "draw/text.h"
#include "draw/tree.h"
#include "event/play.h"
#include "hud/hud.h"
#include "object/download.h"
#include "player/knuckles.h"
#include "player/player.h"
#include "save/card.h"
#include "save/mail.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/bitmap.h"
#include "system/loader.h"
#include "system/pad.h"
#include "system/prs.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 14
#define TEXT_2_LENGTH 14
#define TEXT_3_LENGTH 14
#define TEXT_4_LENGTH 14
#define TEXT_5_LENGTH 14
#define TEXT_6_LENGTH 14
#define TEXT_7_LENGTH 14
#define TEXT_8_LENGTH 14
#define TEXT_9_LENGTH 14
#define TEXT_10_LENGTH 14
#define TEXT_11_LENGTH 14
#define TEXT_12_LENGTH 14
#define TEXT_13_LENGTH 14
#define TEXT_14_LENGTH 14
#define TEXT_15_LENGTH 14
#define TEXT_16_LENGTH 14
#define TEXT_17_LENGTH 14
#define TEXT_18_LENGTH 14
#define TEXT_19_LENGTH 14
#define TEXT_20_LENGTH 14
#define TEXT_21_LENGTH 14
#define TEXT_22_LENGTH 14
#define TEXT_23_LENGTH 14
#define TEXT_24_LENGTH 14
#define TEXT_25_LENGTH 14
#define TEXT_26_LENGTH 14
#define TEXT_27_LENGTH 14
#define TEXT_28_LENGTH 14
#define TEXT_29_LENGTH 14
#define TEXT_30_LENGTH 14
#define TEXT_31_LENGTH 14
#define TEXT_32_LENGTH 14
#define TEXT_33_LENGTH 16
#define TEXT_34_LENGTH 9
#define TEXT_35_LENGTH 9
#define TEXT_36_LENGTH 9
#include "object/download/text.inc"

/* the event file as save_read_file leaves it in the work area: a header of
 * FILE_HEADER_SIZE bytes, then the body. from its start, the header's words
 * are:
 * - the item block's offset, and its count of entries;
 * - the messages' offset and count;
 * - the textures' offset, 1 or 2 for bank a or b, and count;
 * - the sound bank's offset, and 1 to load it;
 * - the model's offset, and 1 or 2 for model a or b.
 * a 0 in place of a 1 or 2 loads nothing. the word at FILE_SUM holds ~sum,
 * where sum adds the body's bytes, each a signed char */
#define FILE_BUFFER ((int *)WORK_AREA)
#define FILE_SUM ((int *)(WORK_AREA + 0x2c))
#define FILE_HEADER_SIZE 64
#define FILE_BODY ((char *)(WORK_AREA + FILE_HEADER_SIZE))
/* the event file's offsets count from the card file's start, 640 bytes
 * before WORK_AREA. save_read_file leaves out the card file's own header
 * and its icon */
#define FILE_BASE ((int)WORK_AREA - 640)

/* download_decrypt xors the file's nth word with FILE_KEY ^ (n *
 * FILE_KEY_STEP) */
#define FILE_KEY 0x5a455441
#define FILE_KEY_STEP 0x4e414e41

/* where download_load_model unpacks the file's model a or b */
#define MODEL_A_BUFFER ((struct model_file *)0x8cca4000)
#define MODEL_B_BUFFER ((struct model_file *)0x8cca6000)

/* in the item block's character mask, a bit for each character the file
 * admits, the low bit of a nibble each */
#define FOR_SONIC 0x1
#define FOR_TAILS 0x10
#define FOR_KNUCKLES 0x100
#define FOR_E102 0x1000
#define FOR_AMY 0x10000
#define FOR_BIG 0x100000

/* in its area mask, a bit for each console area the file admits, by the
 * area's digit */
#define FOR_AREA_0 0x1
#define FOR_AREA_1 0x2
#define FOR_AREA_2 0x4

/* the file holds up to MESSAGES messages of MESSAGE_SIZE bytes, by number.
 * the mission shows the first four itself */
#define MESSAGES 16
#define MESSAGE_SIZE 64
#define MESSAGE_ALL_TAKEN 0 /* a take brings the count to items_needed */
#define MESSAGE_CLEARED 1 /* the goal is reached in time */
#define MESSAGE_TOO_SLOW 2 /* the goal is reached past the limit */
#define MESSAGE_NO_RINGS 3 /* a costly item is touched with too few rings */

/* the counted items' numbers run below this, a bit each in items_taken */
#define COUNTED_ITEMS 128

/* the mission's time counts steps, STEPS_PER_SECOND to the hud's second.
 * it stops at TIME_MAX, which the hud shows as 99:59:99 */
#define STEPS_PER_SECOND 60
#define STEPS_PER_MINUTE (60 * STEPS_PER_SECOND)
#define TIME_MAX (100 * STEPS_PER_MINUTE)
/* a start's number gives the time limit in units of 10 seconds */
#define LIMIT_UNIT (10 * STEPS_PER_SECOND)

/* while this bit of hud_steps is set, the hud shows the items taken, and
 * while it's clear, the limit */
#define HUD_SHOWS_COUNT 0x100
/* the hud shows the limit 1 second and 1 hundredth longer than it is. it
 * adds LIMIT_SHOWN_OVER to the limit's count of seconds, and to its count
 * of hundredths */
#define LIMIT_SHOWN_OVER 1.0F
/* while the mission runs, a light runs along the time label's 8 cells. at
 * time t it is on cell WAVE_CELL(t), which moves on every 4 steps */
#define WAVE_CELL(t) (((t) >> 2) & 7)
/* while this bit of retry_draws is set, the hud shows the retry prompt */
#define RETRY_SHOWN 16

enum mission_state {
    MISSION_NONE = 0,
    MISSION_RUNNING = -1,
    MISSION_CLEARED = -2
};

/* the rings an item with NUMBER_COSTS_RINGS takes */
#define RING_COST 50

/* the bits of an item's kind */
#define ITEM_SOLID 0x01 /* the world collides with its model */
#define ITEM_SOUND 0x02 /* a touch plays a sound */
#define ITEM_MESSAGE 0x04 /* a touch shows a message */
#define ITEM_DUG 0x08 /* only Knuckles finds it, by digging */
#define ITEM_DOOR 0x10 /* a touch goes to another stage */
#define ITEM_COUNTED 0x20 /* the mission counts it */
#define ITEM_GOAL 0x40 /* the mission ends at it */
#define ITEM_START 0x80 /* the mission starts at it */

/* in a counted item's number, above its index: the item takes RING_COST
 * rings, and can't be taken with fewer */
#define NUMBER_COSTS_RINGS 0x80

/* an item's model: the file's model a or b, or one of the two signs */
enum item_model {
    MODEL_FILE_A = 0,
    MODEL_FILE_B = 1,
    MODEL_SQUARE = 128,
    MODEL_NARROW = 129
};

enum item_mode {
    ITEM_INIT,
    ITEM_WAIT, /* for a player to touch it */
    ITEM_TAKEN, /* shrinks and comes back, a dug one rising first */
    ITEM_GONE
};

/* an item's timer, in steps. a waiting item holds TIMER_WAIT, or TIMER_DUG
 * from when Knuckles digs out while it waits. a taken item counts down to
 * 0, and waits again on the step it reaches 0. it shrinks while the timer
 * is at TIMER_TAKEN or below. a dug item first rises for TIMER_TAKEN steps,
 * and then holds */
#define TIMER_WAIT 999
#define TIMER_DUG 180
#define TIMER_TAKEN 60

/* an item farther than this from player 0 skips its step */
#define ITEM_RANGE 1000.0F
/* how far a dug item rises a step */
#define DUG_RISE 0.6F
/* a taken item's scale each step, as a share of the last */
#define TAKEN_SHRINK 0.95F
/* a player within its model's radius plus this of a solid item has the
 * world test it */
#define WORLD_REACH 20.0F

/* steps from a clear in time to MESSAGE_CLEARED */
#define CLEARED_DELAY 180

/* the work mode of download_task, and of mission_clear_step's task */
enum task_mode { MODE_START, MODE_RUN };

/* the button that starts the mission over */
#define BUTTON_RETRY PDD_DGT_TY

/* an unpacked model file, which opens with a pointer to its object */
struct model_file {
    NJS_OBJECT *object;
};

/* an item's entry in the file's item block */
struct item_entry {
    Uint8 stage;
    Uint8 act;
    Sint8 scale[3]; /* the model's scale, in tenths */
    Sint8 spin[3];
    Uint8 model;
    Uint8 texture;
    Uint8 unk_0a;
    Uint8 kind;
    Uint8 number;
    Sint8 unk_0d;
    Uint8 message;
    Uint8 radius;
    Uint8 arg1;
    Uint8 arg2;
    Sint16 ang[3];
    Sint16 x;
    Sint16 y;
    Sint16 z;
};

/* what a touch does. number is a start's time limit in LIMIT_UNITs, or the
 * goal's count of items needed. for a counted item, it is the item's index
 * in items_taken, ORed with NUMBER_COSTS_RINGS where the item costs
 * rings */
struct item_info {
    Sint8 message;
    Uint8 number;
    Sint8 arg1; /* a sound's bank, or the stage a door goes to */
    Sint8 arg2; /* the sound, or the door's act */
};

/* an item's view of its task_work. touched takes submode's place, the
 * item's own fields take vars', and object takes event's */
struct item_work {
    Sint8 mode;
    Sint8 touched; /* 1 from a touch until no player is in reach */
    Uint8 unk_02[2];
    Sint16 flags;
    Uint16 timer;
    Uint8 model;
    /* the texture's bank, 0 or 1, in the high nibble, and its index in the
     * low */
    Uint8 texture;
    Uint8 unk_0a;
    Uint8 kind;
    Sint8 spin[3]; /* each angle's turn a step, in units of 16 */
    Uint8 radius; /* how near a player touches it */
    struct item_info info;
    struct angles ang;
    NJS_POINT3 pos;
    NJS_POINT3 scl;
    struct collision_work *collision; /* NULL: an item has no collision */
    NJS_OBJECT *object; /* a solid item's object in the world, or NULL */
};

static void download_step(struct task *task);
static void download_draw(struct task *task);
static void download_destroy(struct task *task);
static Bool download_read(int length);
static void download_item_step(struct task *task);
static void download_item_draw(struct task *task);
static void download_item_destroy(struct task *task);
static void download_item_collide(struct task *task);
static void mission_clear_step(struct task *task);

/* the two signs an item can draw, a square and a narrower one */
static NJS_MATERIAL square_materials[1] = {
#include "object/download/square_materials.inc"
};

static Sint16 square_meshes0[5] = {
#include "object/download/square_meshes0.inc"
};

static NJS_COLOR square_uvs0[4] = {
#include "object/download/square_uvs0.inc"
};

static NJS_MESHSET square_meshsets[1] = {
#include "object/download/square_meshsets.inc"
};

static NJS_POINT3 square_points[4] = {
#include "object/download/square_points.inc"
};

static NJS_VECTOR square_normals[4] = {
#include "object/download/square_normals.inc"
};

static NJS_MODEL square_model = {
#include "object/download/square_model.inc"
};

static NJS_OBJECT square_object = {
#include "object/download/square_object.inc"
};

/* the banks of sixteen textures that the file's textures fill, a and b */
static NJS_TEXNAME bank_a_texnames[16] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
};
static NJS_TEXLIST bank_a_texlist = {
#include "object/download/bank_a_texlist.inc"
};

static NJS_MATERIAL narrow_materials[1] = {
#include "object/download/narrow_materials.inc"
};

static Sint16 narrow_meshes0[5] = {
#include "object/download/narrow_meshes0.inc"
};

static NJS_COLOR narrow_uvs0[4] = {
#include "object/download/narrow_uvs0.inc"
};

static NJS_MESHSET narrow_meshsets[1] = {
#include "object/download/narrow_meshsets.inc"
};

static NJS_POINT3 narrow_points[4] = {
#include "object/download/narrow_points.inc"
};

static NJS_VECTOR narrow_normals[4] = {
#include "object/download/narrow_normals.inc"
};

static NJS_MODEL narrow_model = {
#include "object/download/narrow_model.inc"
};

static NJS_OBJECT narrow_object = {
#include "object/download/narrow_object.inc"
};

static NJS_TEXNAME bank_b_texnames[16] = {
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
};
static NJS_TEXLIST bank_b_texlist = {
#include "object/download/bank_b_texlist.inc"
};
static NJS_TEXNAME blank_texnames[1] = {
    {TEXT_33, 0, 0},
};
static NJS_TEXLIST blank_texlist = {
#include "object/download/blank_texlist.inc"
};

/* the mission's progress, which carries over from act to act while the
 * file's mission stays the same */
static enum mission_state mission_state = MISSION_NONE;
static int mission_time = 0; /* in steps */
static int mission_limit = 0; /* in steps */
static int items_needed = 0;
static int mission_kept = 0; /* the mission the progress is for, or 0 */
static int item_ring_cost = RING_COST;
/* the task the items are made under, and whether it runs */
static struct task *download_task = NULL;
static Bool download_running = FALSE;
/* sixteen chars, not a string: no null follows */
char card_hex_digits[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                            '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

static Bool banks_loaded[2];
static Bool sound_loaded;
static Bool models_loaded[2];
static Bool files_read[10];
static int dat_8c751494;
static Bool messages_loaded[MESSAGES];
static char messages[MESSAGES][MESSAGE_SIZE];
/* a bit for each counted item taken */
static int items_taken[COUNTED_ITEMS / 32];
/* the mission of the file read this act, or 0 */
static int file_mission;
/* the hud's clocks: the steps it has shown the mission, and download_draw's
 * calls while a mission is on */
static int hud_steps;
static int retry_draws;
/* TRUE when the file puts an item in this act */
static Bool items_in_act;
static NJS_OBJECT *file_model_a;
static NJS_OBJECT *file_model_b;
/* the message shown, one page and a null */
static char *message_pages[2];
/* the scale and height a taken item comes back to, which every item
 * shares */
static Float taken_scale[3];
static Float taken_y;

/* a card file's name. the words give the union a long's alignment, so one
 * assignment copies it as four longs */
union file_name {
    char name[16];
    Sint32 words[4];
};

/* the name of the file read, its number in hex in place of the 999 */
static const union file_name download_file_name = {"SONICADV_999"};

/* the hud's words, as arrays of whole words in the order and padding of
 * the image's constant section */
static const char time_label[12] = {
#include "object/download/time_label.inc"
};
static const char time_colons[12] = {
#include "object/download/time_colons.inc"
};
static const char wave_dash[4] = {
#include "object/download/wave_dash.inc"
};
static const char wave_space[4] = {
#include "object/download/wave_space.inc"
};
static const char wave_t[4] = {
#include "object/download/wave_t.inc"
};
static const char wave_i[4] = {
#include "object/download/wave_i.inc"
};
static const char wave_m[4] = {
#include "object/download/wave_m.inc"
};
static const char wave_e[4] = {
#include "object/download/wave_e.inc"
};
static const char wave_colon[4] = {
#include "object/download/wave_colon.inc"
};
static const char target_label[12] = {
#include "object/download/target_label.inc"
};
static const char count_slash[12] = {
#include "object/download/count_slash.inc"
};

void download_reset(void)
{
    int i;

    mission_kept = 0;
    mission_state = MISSION_NONE;
    for (i = 0; i < COUNTED_ITEMS / 32; i++) {
        items_taken[i] = 0;
    }
    mission_time = 0;
    mission_limit = 0;
    items_needed = 0;
    item_ring_cost = RING_COST;
}

/* start the mission over: none running, no items taken and no time */
static void mission_retry(void)
{
    int i;

    mission_state = MISSION_NONE;
    for (i = 0; i < COUNTED_ITEMS / 32; i++) {
        items_taken[i] = 0;
    }
    mission_time = 0;
}

void download_start(void)
{
    union file_name file;
    int number;
    int length;
    int i;

    banks_loaded[0] = FALSE;
    banks_loaded[1] = FALSE;
    sound_loaded = FALSE;
    models_loaded[0] = FALSE;
    models_loaded[1] = FALSE;
    dat_8c751494 = 0;
    for (i = 0; i < 10; i++) {
        files_read[i] = FALSE;
    }
    for (i = 0; i < MESSAGES; i++) {
        messages_loaded[i] = FALSE;
    }
    if (download_running) {
        task_destroy(download_task);
    }
    download_running = FALSE;
    download_task = task_create(TASK_WORK, 3, download_step);
    file_mission = 0;
    number = card_newest_download(save_drive);
    if (number >= 0) {
        /* @hack spelling: the copy filled through a pointer, which SHC holds in
         * R14 (MOV R15,R14 at 0x8c0362f6); through file itself, the digits
         * store through R15 and 58 of 122 words differ */
        union file_name *name;

        file = download_file_name;
        name = &file;
        name->name[9] = card_hex_digits[number / 256];
        name->name[10] = card_hex_digits[number / 16 % 16];
        name->name[11] = card_hex_digits[number % 16];
        length = save_read_file(name->name);
        if (length >= 0) {
            if (download_read(length)) {
                files_read[0] = TRUE;
            }
        }
    }
    if (file_mission == 0) {
        download_reset();
    }
}

void download_end(void)
{
    if (download_running) {
        task_destroy(download_task);
    }
    download_running = FALSE;
}

/* make a task under download_task for each of the count entries in table
 * that name this stage and act. items_in_act says whether any does */
static void download_items_create(struct item_entry *table, int count)
{
    struct item_entry *entry;
    struct task *task;
    struct item_work *work;
    int stage;
    int act;
    int i;

    items_in_act = FALSE;
    if (count == 0) {
        return;
    }
    stage = stage_get_stage_act();
    act = stage & 255;
    stage = stage >> 8;
    entry = table;
    for (i = 0; i < count; i++) {
        if (stage == entry->stage && act == entry->act) {
            items_in_act = TRUE;
            task = task_create_child(TASK_WORK | TASK_FORCE, download_item_step,
                                     download_task);
            work = (struct item_work *)task->work;
            work->pos.x = (Float)entry->x;
            work->pos.y = (Float)entry->y;
            work->pos.z = (Float)entry->z;
            work->ang.x = entry->ang[0];
            work->ang.y = entry->ang[1];
            work->ang.z = entry->ang[2];
            work->scl.x = (Float)entry->scale[0] * 0.1F;
            work->scl.y = (Float)entry->scale[1] * 0.1F;
            work->scl.z = (Float)entry->scale[2] * 0.1F;
            work->spin[0] = entry->spin[0];
            work->spin[1] = entry->spin[1];
            work->spin[2] = entry->spin[2];
            work->radius = entry->radius;
            work->model = entry->model;
            work->texture = entry->texture;
            work->unk_0a = entry->unk_0a;
            work->kind = entry->kind;
            /* @hack type: the message stored through a byte cursor 16 bytes
             * into work, info.message's offset, which SHC forms apart from the
             * rest of info; as work->info.message, 27 of 152 words differ */
            *(Sint8 *)((char *)work + 16) = entry->message;
            work->info.number = entry->number;
            work->info.arg1 = entry->arg1;
            work->info.arg2 = entry->arg2;
            if (work->kind & ITEM_GOAL) {
                items_needed = work->info.number;
            }
        }
        entry++;
    }
}

/* copy the first count of the file's messages, MESSAGES at most, each once
 * an act. each character of a message takes two bytes. a 't' or 'n' byte
 * where a character starts stands for a tab or a new line, and takes one */
static void download_load_messages(char *text, int count)
{
    int i;
    int j;
    int odd;
    char ch;

    if (count == 0) {
        return;
    }
    if (count > MESSAGES) {
        count = MESSAGES;
    }
    for (i = 0; i < count; i++) {
        if (!messages_loaded[i]) {
            messages_loaded[i] = TRUE;
            odd = 0;
            for (j = 0; j < MESSAGE_SIZE; j++) {
                ch = *text++;
                if (ch == 't' && odd == 0) {
                    ch = '\t';
                    odd--;
                }
                if (ch == 'n' && odd == 0) {
                    ch = '\n';
                    odd--;
                }
                messages[i][j] = ch;
                odd++;
                odd &= 1;
            }
        }
    }
}

/* load the pvm's count textures into bank a for bank 1, or bank b for 2.
 * each loads once an act, and only with an item in the act. bank 0 loads
 * none */
static void download_load_textures(void *pvm, int bank, int count)
{
    if (bank != 0 && items_in_act) {
        if (bank == 1 && !banks_loaded[0]) {
            bank_a_texlist.nbTexture = count;
            njLoadTexturePvmMemory(pvm, &bank_a_texlist);
            banks_loaded[0] = TRUE;
        }
        if (bank == 2 && !banks_loaded[1]) {
            bank_b_texlist.nbTexture = count;
            njLoadTexturePvmMemory(pvm, &bank_b_texlist);
            banks_loaded[1] = TRUE;
        }
    }
}

/* load the file's sound bank for kind 1, once an act and only with an item
 * in the act. kind 0 loads none */
static void download_load_sound(void *data, int kind)
{
    if (kind == 0) {
        return;
    }
    if (!items_in_act) {
        return;
    }
    /* @hack type: the flag read and set through a Uint32 view of itself,
     * which keeps its address in R2 and loads sound_download's into R1 ahead
     * of the store (0x8c0366e6, 0x8c0366ee); as sound_loaded, 7 of 20 words
     * differ */
    if (kind == 1 && *(Uint32 *)&sound_loaded == 0) {
        *(Uint32 *)&sound_loaded = TRUE;
        sound_download(data);
    }
}

/* unpack the file's model into MODEL_A_BUFFER for which 1, or into
 * MODEL_B_BUFFER for 2, once an act. which 0 unpacks none */
static void download_load_model(void *prs, int which)
{
    /* @hack spelling: each object through a local, which takes R4 and puts
     * its store ahead of the flag's (0x8c03676e, 0x8c036772); stored
     * directly, 17 of 47 words differ */
    NJS_OBJECT *object;

    if (which == 0) {
        return;
    }
    if (which == 1 && !models_loaded[0]) {
        prs_decompress(prs, MODEL_A_BUFFER);
        object = MODEL_A_BUFFER->object;
        file_model_a = object;
        models_loaded[0] = TRUE;
    }
    if (which == 2 && !models_loaded[1]) {
        prs_decompress(prs, MODEL_B_BUFFER);
        object = MODEL_B_BUFFER->object;
        file_model_b = object;
        models_loaded[1] = TRUE;
    }
}

/* decrypt length bytes at buf in place, a word at a time */
/* @hack redundant: register on buf, which puts FILE_KEY in R0 and its step
 * in R5 (0x8c0367b2, 0x8c0367b0); without it, the two trade and 2 of 24
 * words differ */
static void download_decrypt(register int *buf, int length)
{
    int key;
    int i;
    int words;

    /* @hack spelling: i and key zeroed in one expression ahead of the loop,
     * which zeroes R6 and copies it to R7 (0x8c0367a6, 0x8c0367aa); as
     * key = 0 and the for's i = 0, 16 of 24 words differ */
    i = 0, key = 0;
    words = length / 4;
    for (; i < words; i++) {
        *buf ^= key ^ FILE_KEY;
        key += FILE_KEY_STEP;
        buf++;
    }
}

/* decrypt the file length bytes long in FILE_BUFFER, and add what it holds
 * to the act. returns FALSE, adding nothing, for a bad sum or when the act
 * has a mission already. it does the same for a character or console area
 * that the file's masks leave out. else TRUE */
static Bool download_read(int length)
{
    struct console_record *console;
    int *header;
    char *byte;
    char area;
    int count;
    int sum;
    int i;
    int id;
    int characters;
    int areas;
    int *table;
    int saved;
    int check;
    int value;
    int textures;
    void *data;

    kbCfgIni(0);
    console = kbCfgGetId();
    area = console->area;
    header = FILE_BUFFER;
    download_decrypt(header, length);
    saved = *FILE_SUM;
    sum = 0;
    byte = FILE_BODY;
    count = length - FILE_HEADER_SIZE;
    for (i = 0; i < count; i++) {
        sum += *byte++;
    }
    check = ~sum;
    if (check != saved) {
        return FALSE;
    }
    table = (int *)(*header++ + FILE_BASE);
    count = *header++;
    /* the item block opens with the file's mission, or 0 for none, then its
     * character and area masks, a word each. the entries follow */
    id = *table;
    table++;
    characters = *table;
    table++;
    areas = *table;
    table++;
    switch (get_character()) {
    case CHARACTER_SONIC:
        if (!(characters & FOR_SONIC)) {
            return FALSE;
        }
        break;
    case CHARACTER_TAILS:
        if (!(characters & FOR_TAILS)) {
            return FALSE;
        }
        break;
    case CHARACTER_KNUCKLES:
        if (!(characters & FOR_KNUCKLES)) {
            return FALSE;
        }
        break;
    case CHARACTER_E102:
        if (!(characters & FOR_E102)) {
            return FALSE;
        }
        break;
    case CHARACTER_AMY:
        if (!(characters & FOR_AMY)) {
            return FALSE;
        }
        break;
    case CHARACTER_BIG:
        if (!(characters & FOR_BIG)) {
            return FALSE;
        }
        break;
    default:
        break;
    }
    switch (area) {
    case '0':
        if (!(areas & FOR_AREA_0)) {
            return FALSE;
        }
        break;
    case '1':
        if (!(areas & FOR_AREA_1)) {
            return FALSE;
        }
        break;
    case '2':
        if (!(areas & FOR_AREA_2)) {
            return FALSE;
        }
        break;
    default:
        break;
    }
    if (id != 0) {
        if (file_mission != 0) {
            return FALSE;
        } else {
            file_mission = id;
            if (mission_kept != file_mission) {
                download_reset();
                mission_kept = file_mission;
            }
        }
    }
    download_items_create((struct item_entry *)table, count);
    data = (void *)(*header++ + FILE_BASE);
    value = *header++;
    download_load_messages((char *)data, value);
    data = (void *)(*header++ + FILE_BASE);
    value = *header++;
    textures = *header++;
    download_load_textures(data, value, textures);
    data = (void *)(*header++ + FILE_BASE);
    value = *header++;
    download_load_sound(data, value);
    data = (void *)(*header++ + FILE_BASE);
    value = *header;
    download_load_model(data, value);
    return TRUE;
}

/* download_task's step: step the items. then, while a mission is on and
 * no event has control, count the mission's time while it runs and show
 * the hud. the time shows at rows 25 and 26, red past the limit. above it,
 * row 24 shows the items taken and the limit by turns, 256 steps each */
static void download_step(struct task *task)
{
    struct task_work *work;
    Sint8 *mode;
    Bool late;
    int minutes;
    int seconds;
    int hundredths;
    int count;
    int id;
    Float time;
    Float in_seconds;
    Float in_hundredths;
    Float in_minutes;

    work = task->work;
    /* @hack spelling: the mode through a pointer, which keeps the 44-byte stack
     * frame and mission_state's address in R8 (ADD #-8,R15 at 0x8c0369ae,
     * 0x8c036a06); as work->mode, 193 of 402 words differ */
    mode = &work->mode;
    download_running = TRUE;
    switch (*mode) {
    case MODE_START:
        task->draw = download_draw;
        /* @hack redundant: task->draw = task->draw, which SHC drops, but which
         * leaves the time's sums in FR6 and FR5 (FLDI1 FR6 at 0x8c036db0);
         * without it, the two trade and 8 of 402 words differ */
        task->draw = task->draw;
        task->destructor = download_destroy;
        *mode = MODE_RUN;
        break;
    case MODE_RUN: {
        /* @hack spelling: the task through a local, which keeps the
         * seconds in R9 and the hundredths in R10 (MOV #59,R9 at
         * 0x8c036b8e); passed directly, the two trade and 18 of 402 words
         * differ */
        struct task *parent = task;

        task_step_children(parent);
    }
    }
    if (mission_state == MISSION_NONE) {
        return;
    }
    if (event_in_control() != 0) {
        return;
    }
    late = FALSE;
    text_set_color(0xf0f0c040);
    text_print(NJM_LOCATION(31, 25), time_label);
    text_set_color(0xf080e0f0);
    text_print(NJM_LOCATION(31, 26), time_colons);
    if (mission_state == MISSION_RUNNING) {
        mission_time++;
        if (mission_time > TIME_MAX) {
            mission_time = TIME_MAX;
        }
        if (mission_time > mission_limit) {
            late = TRUE;
        }
        if (mission_time % STEPS_PER_MINUTE == 0 && mission_time != TIME_MAX) {
            sound_play(SE_BEEP, 0, 0, 0);
        }
        /* the colons light with the label's first cell */
        text_set_color(0xf0f04000);
        /* @hack type: wave_dash and wave_space passed as their arrays'
         * addresses, which SHC loads from the pool at each draw (0x8c036b22,
         * 0x8c036b5a); by name, it loads them into R9 and R5 ahead of the
         * switch and 354 of 402 words differ */
        switch (WAVE_CELL(mission_time)) {
        case 0:
            text_print(NJM_LOCATION(31, 25), (const char *)&wave_dash);
            break;
        case 1:
            text_print(NJM_LOCATION(32, 25), (const char *)&wave_space);
            break;
        case 2:
            text_print(NJM_LOCATION(33, 25), wave_t);
            break;
        case 3:
            text_print(NJM_LOCATION(34, 25), wave_i);
            break;
        case 4:
            text_print(NJM_LOCATION(35, 25), wave_m);
            break;
        case 5:
            text_print(NJM_LOCATION(36, 25), wave_e);
            break;
        case 6:
            text_print(NJM_LOCATION(37, 25), (const char *)&wave_space);
            break;
        case 7:
            text_print(NJM_LOCATION(38, 25), (const char *)&wave_dash);
            break;
        }
        text_set_color(0xf0f04000);
        switch (WAVE_CELL(mission_time)) {
        case 0:
            text_print(NJM_LOCATION(33, 26), wave_colon);
            text_print(NJM_LOCATION(36, 26), wave_colon);
            break;
        }
    }
    if (mission_time == TIME_MAX) {
        minutes = 99;
        seconds = 59;
        hundredths = 99;
    } else {
        time = (Float)mission_time;
        in_hundredths = time * (100.0F * (1.0F / STEPS_PER_SECOND));
        in_seconds = time * (1.0F / STEPS_PER_SECOND);
        in_minutes = in_seconds * (1.0F / 60.0F);
        minutes = (int)in_minutes;
        seconds = (int)in_seconds;
        hundredths = (int)in_hundredths;
        seconds = seconds % 60;
        hundredths = hundredths % 100;
    }
    if (!late) {
        text_set_color(0xf0b0e0f0);
    } else {
        text_set_color(0xf0f06060);
    }
    text_print_number(NJM_LOCATION(31, 26), minutes, 2);
    text_print_number(NJM_LOCATION(34, 26), seconds, 2);
    text_print_number(NJM_LOCATION(37, 26), hundredths, 2);
    text_set_color(0xf0f0f040);
    /* @hack redundant: an empty test, which keeps mission_state's address in
     * R8 for the reads after it (0x8c036a06); without it, 175 of 402 words
     * differ */
    if (mission_state == MISSION_RUNNING) {
    } else {
    }
    text_print(NJM_LOCATION(31, 23), target_label);
    if (hud_steps++ & HUD_SHOWS_COUNT) {
        text_set_color(0xf040f060);
        text_print(NJM_LOCATION(31, 24), count_slash);
        count = 0;
        for (id = 0; id < COUNTED_ITEMS; id++) {
            if (items_taken[id / 32] & (1 << (id % 32))) {
                count++;
            }
        }
        text_set_color(0xf050c0f0);
        text_print_number(NJM_LOCATION(34, 24), count, 2);
        text_print_number(NJM_LOCATION(37, 24), items_needed, 2);
        return;
    } else {
        text_set_color(0xf040f060);
        text_print(NJM_LOCATION(31, 24), time_colons);
        time = (Float)mission_limit;
        in_hundredths =
            (time * (100.0F * (1.0F / STEPS_PER_SECOND))) + LIMIT_SHOWN_OVER;
        in_seconds = (time * (1.0F / STEPS_PER_SECOND)) + LIMIT_SHOWN_OVER;
        in_minutes = in_seconds * (1.0F / 60.0F);
        minutes = (int)in_minutes;
        seconds = (int)in_seconds;
        hundredths = (int)in_hundredths;
        seconds = seconds % 60;
        hundredths = hundredths % 100;
        text_set_color(0xf060f080);
        text_print_number(NJM_LOCATION(31, 24), minutes, 2);
        text_print_number(NJM_LOCATION(34, 24), seconds, 2);
        text_print_number(NJM_LOCATION(37, 24), hundredths, 2);
    }
    text_set_color(0xf0ffffff);
}

/* download_task's draw: while a mission runs or after it clears, show the
 * retry prompt, and start the mission over on BUTTON_RETRY. the draw, not
 * the step, reads the press */
static void download_draw(struct task *task)
{
#ifdef GCC
    /* the split frame's second picture shows no retry prompt and retries
     * no mission: the prompt is the pause's */
    if (frame_redrawing) {
        return;
    }
#endif
    if (mission_state != MISSION_NONE) {
        retry_draws++;
        text_set_color(0xf0a0f0a0);
        text_print(NJM_LOCATION(30, 23), TEXT_34);
        if (retry_draws & RETRY_SHOWN) {
            text_set_color(0xf080f080);
            text_print(NJM_LOCATION(30, 24), TEXT_35);
            text_print(NJM_LOCATION(30, 24), TEXT_36);
        }
        if (per_info[0]->press & BUTTON_RETRY) {
            mission_retry();
        }
    }
}

/* download_task's destructor: release the banks' textures, and mark the
 * sound, the banks and the models unloaded. it leaves download_task NULL
 * and download_running FALSE */
static void download_destroy(struct task *task)
{
    /* @hack spelling: the sound flag cleared through a pointer, which loads its
     * address into R4 at the store (0x8c036ef2); cleared directly, R3, and
     * 9 of 43 words differ */
    Bool *loaded;

    loaded = &sound_loaded;
    *loaded = FALSE;
    if (banks_loaded[0]) {
        njReleaseTexture(&bank_a_texlist);
        banks_loaded[0] = FALSE;
    }
    if (banks_loaded[1]) {
        njReleaseTexture(&bank_b_texlist);
        banks_loaded[1] = FALSE;
    }
    if (models_loaded[0]) {
        models_loaded[0] = FALSE;
    }
    if (models_loaded[1]) {
        models_loaded[1] = FALSE;
    }
    download_running = FALSE;
    download_task = NULL;
}

/* TRUE while the item is in play, else FALSE. a counted item plays while
 * the mission runs, until it's taken. a start plays before the mission, and
 * a goal while it runs. any other item always plays */
static Bool download_item_in_play(struct task *task)
{
    struct item_work *work;
    int bit;
    int id;
    enum mission_state state;

    work = (struct item_work *)task->work;
    state = mission_state;
    if (work->kind & ITEM_COUNTED) {
        id = work->info.number & (COUNTED_ITEMS - 1);
        bit = 1 << (id % 32);
        if (items_taken[id / 32] & bit) {
            return FALSE;
        }
        if (state != MISSION_RUNNING) {
            return FALSE;
        }
    }
    if (work->kind & ITEM_START) {
        if (state != MISSION_NONE) {
            return FALSE;
        }
    }
    if (work->kind & ITEM_GOAL) {
        if (state == MISSION_RUNNING) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

/* an item's step. a waiting item acts on a touch. the touch can start the
 * mission, reach its goal or count the item as taken. it can also show a
 * message, play a sound or go to another stage. a taken item shrinks away
 * and comes back */
static void download_item_step(struct task *task)
{
    struct item_work *work;
    Bool shown;
    Bool found;
    int id;
    int count;
    int number;
    int bit;
    Bool all_taken;
    Sint8 *args;
    Bool no_rings;
    Bool too_slow;
    struct task_work *player;
    struct item_info *info;
    Float dx;
    Float dy;
    Float dz;

    work = (struct item_work *)task->work;
    shown = TRUE;
    all_taken = FALSE;
    too_slow = FALSE;
    no_rings = FALSE;
    player = player_work[0];
    dx = work->pos.x - player->pos.x;
    dy = work->pos.y - player->pos.y;
    dz = work->pos.z - player->pos.z;
    if (vector_length_squared(dx, dy, dz) > ITEM_RANGE * ITEM_RANGE) {
        return;
    }
    if (!download_item_in_play(task)) {
        if (work->mode == ITEM_WAIT) {
            return;
        }
    }
    if (work->mode == ITEM_GONE) {
        return;
    }
    switch (work->mode) {
    case ITEM_INIT:
        task->draw = download_item_draw;
        task->destructor = download_item_destroy;
        work->mode = ITEM_WAIT;
        work->touched = 0;
        work->timer = TIMER_WAIT;
        break;
    case ITEM_WAIT:
        if (work->kind & ITEM_DUG) {
            found = FALSE;
            if (get_character() == CHARACTER_KNUCKLES) {
                if (knuckles_get_activity(0) == KNUCKLES_DIGGING_OUT) {
                    found = TRUE;
                    work->timer = TIMER_DUG;
                }
            }
        } else {
            found = TRUE;
        }
        if (found && player_in_range(&work->pos, (Float)work->radius)) {
            if (work->touched == 0) {
                work->touched = 1;
                if (work->kind & ITEM_START) {
                    shown = FALSE;
                    if (mission_state == MISSION_NONE) {
                        mission_state = MISSION_RUNNING;
                        hud_steps = 0;
                        mission_limit = work->info.number * LIMIT_UNIT;
                        shown = TRUE;
                        work->mode = ITEM_TAKEN;
                        if (work->timer != TIMER_DUG) {
                            work->timer = TIMER_TAKEN;
                        }
                        taken_scale[0] = work->scl.x;
                        taken_scale[1] = work->scl.y;
                        taken_scale[2] = work->scl.z;
                        taken_y = work->pos.y;
                    }
                }
                if (work->kind & ITEM_GOAL) {
                    shown = FALSE;
                    if (mission_state == MISSION_RUNNING) {
                        count = 0;
                        for (id = 0; id < COUNTED_ITEMS; id++) {
                            if (items_taken[id / 32] & (1 << (id % 32))) {
                                count++;
                            }
                        }
                        if (count >= items_needed) {
                            mission_state = MISSION_CLEARED;
                            hud_steps = HUD_SHOWS_COUNT;
                            shown = TRUE;
                            if (mission_time <= mission_limit) {
                                task_create(TASK_WORK, 3, mission_clear_step);
                                save_write_mission_mail(0, mission_kept,
                                                        mission_time);
                            } else {
                                too_slow = TRUE;
                            }
                            work->mode = ITEM_TAKEN;
                            if (work->timer != TIMER_DUG) {
                                work->timer = TIMER_TAKEN;
                            }
                            taken_scale[0] = work->scl.x;
                            taken_scale[1] = work->scl.y;
                            taken_scale[2] = work->scl.z;
                            taken_y = work->pos.y;
                        }
                    }
                }
                if (work->kind & ITEM_COUNTED) {
                    shown = FALSE;
                    if (mission_state == MISSION_RUNNING) {
                        /* @hack type: the number tested as a Sint8, which
                         * SHC stores to the stack and reads back for the
                         * test (MOV.B R0,@(8,R15) at 0x8c03732c); as a Uint8,
                         * 16 of 642 words differ */
                        number = work->info.number & (COUNTED_ITEMS - 1);
                        if (((Sint8)work->info.number & NUMBER_COSTS_RINGS) &&
                            ring_count() < item_ring_cost) {
                            shown = TRUE;
                            no_rings = TRUE;
                        } else {
                            /* @hack type: the number tested as a Sint8
                             * again (TST #128,R0 at 0x8c0373a4); as a Uint8,
                             * SHC adds an EXTU.B ahead of the TST and 147 of
                             * 642 words differ */
                            if ((Sint8)work->info.number & NUMBER_COSTS_RINGS) {
                                ring_add(-RING_COST);
                            }
                            bit = 1 << (number % 32);
                            number = number / 32;
                            items_taken[number] |= bit;
                            hud_steps = HUD_SHOWS_COUNT;
                            work->mode = ITEM_TAKEN;
                            shown = TRUE;
                            if (work->timer != TIMER_DUG) {
                                work->timer = TIMER_TAKEN;
                            }
                            taken_scale[0] = work->scl.x;
                            taken_scale[1] = work->scl.y;
                            taken_scale[2] = work->scl.z;
                            taken_y = work->pos.y;
                            count = 0;
                            for (id = 0; id < COUNTED_ITEMS; id++) {
                                if (items_taken[id / 32] & (1 << (id % 32))) {
                                    count++;
                                }
                            }
                            if (count >= items_needed) {
                                all_taken = TRUE;
                            }
                        }
                    }
                }
                if ((work->kind & ITEM_MESSAGE) && shown) {
                    info = &work->info;
                    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
                    id = info->message;
                    if (all_taken) {
                        id = MESSAGE_ALL_TAKEN;
                    }
                    if (too_slow) {
                        id = MESSAGE_TOO_SLOW;
                    }
                    if (no_rings) {
                        id = MESSAGE_NO_RINGS;
                    }
                    if (messages_loaded[id]) {
                        message_pages[0] = messages[id];
                        message_pages[1] = NULL;
                        bitmap_show_pages(message_pages);
                    }
                }
                if ((work->kind & ITEM_SOUND) && shown) {
                    Uint8 second;

                    /* @hack type: info's two arguments read as bytes 2 and 3
                     * of a Sint8 pointer to it; through the members, 138 of
                     * 642 words differ */
                    /* @hack spelling: args assigned in the expression, which
                     * SHC keeps at @(4,R15) and reads back for args[2]
                     * (0x8c037536, 0x8c03753a); assigned apart, SHC holds
                     * it in R12 and 141 of 642 words differ */
                    second = (args = (Sint8 *)&work->info)[3];
                    sound_play_bank(args[2], second);
                }
                if (work->kind & ITEM_DOOR) {
                    Uint8 second;

                    /* @hack type: the same for the door; through the
                     * members, 123 of 642 words differ */
                    /* @hack spelling: args assigned in the expression, as for
                     * the sound (0x8c037552, 0x8c037556); assigned apart,
                     * 126 of 642 words differ */
                    second = (args = (Sint8 *)&work->info)[3];
                    stage_fade_to(args[2], second);
                }
            }
        } else {
            work->touched = 0;
        }
        if (work->kind & ITEM_SOLID) {
            download_item_collide(task);
        }
        break;
    case ITEM_TAKEN:
        if (work->timer > TIMER_DUG - TIMER_TAKEN) {
            work->pos.y += DUG_RISE;
        } else if (work->timer <= TIMER_TAKEN) {
            work->scl.x *= TAKEN_SHRINK;
            work->scl.y *= TAKEN_SHRINK;
            work->scl.z *= TAKEN_SHRINK;
        }
        if (work->timer-- == 0) {
            work->mode = ITEM_WAIT;
            work->timer = TIMER_WAIT;
            work->scl.x = taken_scale[0];
            work->scl.y = taken_scale[1];
            work->scl.z = taken_scale[2];
            work->pos.y = taken_y;
        }
        break;
    }
    work->ang.x += work->spin[0] << 4;
    work->ang.y += work->spin[1] << 4;
    work->ang.z += work->spin[2] << 4;
    download_item_draw(task);
}

/* draw the item's model in its bank's texture, at its pos, angles and
 * scale. a taken item fades out as it shrinks, and a dug item shows only
 * once taken */
static void download_item_draw(struct task *task)
{
    struct item_work *work;
    NJS_OBJECT *model;
    NJS_MATERIAL *material;
    NJS_TEXLIST *texlist;
    int texture;
    Uint32 saved;
    int bank;

    work = (struct item_work *)task->work;
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 100.0F)) {
        return;
    }
    if (!download_item_in_play(task)) {
        if (work->mode == ITEM_WAIT) {
            return;
        }
    }
    if (work->mode == ITEM_GONE) {
        return;
    }
    if (work->kind & ITEM_DUG) {
        if (work->mode == ITEM_WAIT) {
            return;
        }
    }
    switch (work->model) {
    case MODEL_SQUARE:
        model = &square_object;
        break;
    case MODEL_NARROW:
        model = &narrow_object;
        break;
    case MODEL_FILE_A:
        if (!models_loaded[0]) {
            return;
        }
        model = file_model_a;
        break;
    case MODEL_FILE_B:
        if (!models_loaded[1]) {
            return;
        }
        model = file_model_b;
        break;
    default:
        return;
    }
    bank = work->texture >> 4;
    texture = work->texture & 15;
    /* @bug uninitialized: a bank past 1 leaves texlist unset, and
     * njSetTexture takes R12 as the caller left it (MOV R12,R4 at
     * 0x8c0377d8) */
    switch (bank) {
    case 0:
        if (!banks_loaded[0]) {
            return;
        }
        texlist = &bank_a_texlist;
        break;
    case 1:
        if (!banks_loaded[1]) {
            return;
        }
        texlist = &bank_b_texlist;
        break;
#ifdef GCC
    default:
        /* repairs the unset texlist the tag names: GCC draws nothing, as the
         * model switch's default does */
        return;
#endif
    }
    if (work->timer <= TIMER_TAKEN) {
        NJS_ARGB color;
        Float fade;

        fade = (Float)(work->timer - TIMER_TAKEN);
        color.a = fade * (1.0F / TIMER_TAKEN);
        color.r = color.g = color.b = 0.0F;
        njSetConstantMaterial(&color);
    }
    material = model->model->mats;
    saved = material->attr_texId;
    material->attr_texId = texture;
    njSetTexture(texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateX, work->ang.x);
    njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
    draw_tree(model);
    njPopMatrix(1);
    model->model->mats->attr_texId = saved;
    {
        NJS_ARGB color;

        color.a = color.r = color.g = color.b = 0.0F;
        njSetConstantMaterial(&color);
    }
}

static void download_item_destroy(struct task *task)
{
    struct item_work *work;

    work = (struct item_work *)task->work;
    if (work->object != NULL) {
        world_remove(task, work->object);
        world_object_free(work->object);
        work->object = NULL;
    }
}

/* a solid item's collision. the first call lends a world object for its
 * model, posed as the item, and adds it to the world. later calls set
 * WORLD_TESTED while a player is within the model's radius plus WORLD_REACH
 * of its center, else clear it */
static void download_item_collide(struct task *task)
{
    struct item_work *work;
    NJS_OBJECT *model;
    NJS_OBJECT *object;
    NJS_POINT3 center;

    work = (struct item_work *)task->work;
    switch (work->model) {
    case MODEL_SQUARE:
        model = &square_object;
        break;
    case MODEL_NARROW:
        model = &narrow_object;
        break;
    case MODEL_FILE_A:
        if (!models_loaded[0]) {
            return;
        }
        model = file_model_a;
        break;
    case MODEL_FILE_B:
        if (!models_loaded[1]) {
            return;
        }
        model = file_model_b;
        break;
    default:
        return;
    }
    /* @hack redundant: an else after the return, which loads work->object into
     * R1 (MOV.L @(60,R14),R1 at 0x8c037996); without it, R3, and 2 of 199
     * words differ */
    if (model == NULL) {
        return;
    } else if (work->object == NULL) {
        object = world_object_alloc();
        work->object = object;
        object->evalflags = NJD_EVAL_BREAK;
        object->scl[0] = work->scl.x;
        object->scl[1] = work->scl.y;
        object->scl[2] = work->scl.z;
        object->ang[0] = work->ang.x;
        object->ang[1] = work->ang.y;
        object->ang[2] = work->ang.z;
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y;
        object->pos[2] = work->pos.z;
        object->model = model->model;
        object->child = NULL;
        object->sibling = NULL;
        world_add(WORLD_SOLID, task, object);
    } else {
        center.x = model->model->center.x;
        center.y = model->model->center.y;
        center.z = model->model->center.z;
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateY, work->ang.y);
        TURN(njRotateX, work->ang.x);
        njCalcVector(NULL, &center, &center);
        njPopMatrix(1);
        center.x += work->pos.x;
        center.y += work->pos.y;
        center.z += work->pos.z;
        if (player_in_range(&center, model->model->r + WORLD_REACH)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
}

/* a task that shows MESSAGE_CLEARED CLEARED_DELAY steps after a clear in
 * time, then ends. a retry ends it first */
static void mission_clear_step(struct task *task)
{
    struct task_work *work;

    work = task->work;
    if (mission_state != MISSION_CLEARED) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case MODE_START:
        work->mode = MODE_RUN;
        work->timer = CLEARED_DELAY;
        break;
    case MODE_RUN:
        work->timer--;
        if (work->timer == 0) {
            /* @bug logic: the test reads message 0's flag, not
             * MESSAGE_CLEARED's (MOV.L @R2,R3 at 0x8c037b84, R2 the address
             * of messages_loaded). with one message loaded, it shows
             * MESSAGE_CLEARED's bytes, which this file didn't load */
            if (messages_loaded[0]) {
                message_pages[0] = messages[MESSAGE_CLEARED];
                message_pages[1] = NULL;
                bitmap_show_pages(message_pages);
            }
            task_kill(task);
            return;
        }
        break;
    }
}
