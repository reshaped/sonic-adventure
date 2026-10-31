/* sound: the sound driver, and the effects the game queues for it */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/mesh.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/loader.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/timer.h"

/* the queue's slots. slots 0 to 31 are the channels of midi ports 0 and 1,
 * and each plays one note. slot SLOT_CHANNELS + n plays sequences on port
 * SEQUENCE_PORT_FIRST + n. the sound test plays on the last slot */
#define QUEUE_SLOTS 38
#define SLOT_CHANNELS 32
#define SLOT_TEST 37
#define PORT_COUNT 8
#define SEQUENCE_PORT_FIRST 2

/* a slot's flags, which sound_send_queue reads at each update */
#define SLOT_START 0x1 /* start se at the next update */
#define SLOT_FOLLOW_ONCE 0x2 /* SLOT_FOLLOW for one update. nothing sets it */
#define SLOT_STOP 0x10 /* stop se as the slot's time runs out */
#define SLOT_FOLLOW 0x20 /* the falloff at every update, not only the start */
#define SLOT_DISTANCE 0x100 /* volume less the falloff by distance */
#define SLOT_VOLUME 0x200 /* volume as sound_set_volume set it */
#define SLOT_UNK_400 0x400 /* nothing sets it, and sound_set_pan clears it */
#define SLOT_PAN 0x800 /* send the pan at the next update */
#define SLOT_QSOUND 0x1000 /* placed by qsound, if qsound places se */
#define SLOT_PITCH 0x2000 /* send the pitch at the next update */

#define QSOUND_SES 35 /* the effects qsound places */
#define QSOUND_COPIES 4 /* the most copies of one that can play at once */
/* the places qsound_pos keeps, of which sdQsndSetPos sends the first seven */
#define QSOUND_CHANNELS 8
/* the qsound channels qsound_info gives out, 0 to 3, whose places
 * sound_send_queue compares with the places it sent last */
#define QSOUND_USED 4
#define QSOUND_CHANGED 0x80 /* marks a place changed since the last send */
/* the place qsound_reset gives each channel. qsound_place gives only
 * multiples of 4, so never this one */
#define QSOUND_RESET_PLACE 15

/* midi controller numbers */
#define MIDI_MODULATION 1
#define MIDI_VOLUME 7
#define MIDI_PAN 10
#define MIDI_BANK 32
#define MIDI_RESET_CONTROLLERS 121

/* the middle of a channel's pan, and of its bend */
#define MIDI_PAN_MIDDLE 64
#define MIDI_BEND_MIDDLE 0x2000
/* how far to each side of the middle midi_program pans bank 0's program 7 */
#define MIDI_PAN_SIDE 30

/* the midi volume a channel sends for volume 0 */
#define CHANNEL_VOLUME 106
/* the least volume, which sound_mute sends */
#define VOLUME_LEAST (-127)

/* the falloff of a slot's volume by its distance from the camera. it is 0
 * within FALLOFF_NEAR, and rises evenly to FALLOFF_MOST at FALLOFF_FAR and
 * on past it */
#define FALLOFF_NEAR 20.0F
#define FALLOFF_FAR 600.0F
#define FALLOFF_MOST 127.0F

/* the fx level sound_reset gives outputs 0 and 1, and the request for none */
#define FX_RESET (-6)
#define FX_MAX 15 /* the highest fx level */
#define FX_STEP 8 /* the output level sdSndSetFxOutPrm takes per fx level */

/* the updates sound_play and sound_play_at give an effect before its slot
 * stops it */
#define ONCE_UPDATES 120

/* the last effect in the banks. effect SE_LAST + 1 + n is the disc's voice
 * VOICE_FIRST + n, which plays as a stream */
#define SE_LAST 1274
#define VOICE_FIRST 594

/* the debug page that lists the queue, and the row it lists slot 0 on, below
 * its heading */
#define DEBUG_SLOTS 3
#define SLOT_ROW 7

/* how near the camera an orbit's effect plays */
#define ORBIT_REACH 200.0F

/* the sound test's bank for the disc's voices, the last it steps to */
#define TEST_VOICES 15
/* sound_play_bank's bank for the music, and its first bank of voices */
#define BANK_MUSIC 15
#define BANK_VOICES 16

struct sound_slot {
    Sint32 priority; /* a request of this priority or higher can take it */
    Sint32 time; /* updates left. 0 when free, below 0 until stopped */
    Sint32 id; /* who asked: any number the caller chooses */
    Sint32 flags;
    Sint32 se;
    Sint32 pan;
    Sint32 volume;
    Sint32 loudest; /* the most volume the falloff leaves */
    Sint32 pitch;
    NJS_POINT3 pos; /* where se plays, for the falloff and qsound */
    Sint32 qsound; /* the qsound channel it holds, or -1 */
};

/* how qsound places one effect, whose copy 0 plays the effect itself */
struct qsound_se {
    Sint16 copies; /* the copies that can play at once */
    Sint16 channel; /* copy n plays on qsound channel channel + n - 1 */
    Sint16 range; /* heard within range squared of the camera */
    Sint16 next; /* copy n from 1 on plays effect next + n - 1 */
};

static int sound_test_bank = 0;
static int sound_test_number = 0;
/* the key every channel's note plays at */
static int midi_key = 60;
static int dat_8c1d7cfc = 0; /* nothing reads or writes it */
static int midi_side = 0;
static int sound_muted = 0;

/* the effects' banks, as rows of a base and a bank by falling base. effect
 * se is in the bank of the first row whose base it is above, as number
 * se - base - 1. an effect at or below every base is number se of bank 0. a
 * negative base ends the table */
static Sint32 se_ranges[][2] = {
    {1272, 8}, {1263, 7}, {1214, 6}, {1199, 6}, {1186, 6}, {1168, 6}, {1144, 6},
    {1111, 6}, {1106, 5}, {1095, 5}, {1079, 5}, {1058, 5}, {1046, 5}, {1038, 5},
    {1026, 5}, {1008, 5}, {997, 5},  {987, 5},  {978, 5},  {974, 5},  {970, 5},
    {968, 5},  {955, 5},  {945, 5},  {940, 5},  {936, 5},  {931, 5},  {925, 5},
    {837, 4},  {811, 4},  {795, 3},  {776, 3},  {751, 3},  {736, 3},  {722, 3},
    {698, 2},  {689, 1},  {666, 1},  {660, 1},  {599, 1},  {580, 1},  {552, 1},
    {521, 1},  {507, 1},  {486, 1},  {467, 1},  {453, 1},  {427, 1},  {408, 1},
    {377, 1},  {354, 1},  {330, 1},  {304, 1},  {279, 1},  {238, 1},  {207, 1},
    {191, 1},  {159, 1},  {123, 1},  {91, 1},   {65, 1},   {53, 1},   {39, 1},
    {-1, -1},
};

/* the effects qsound places, in rising order, and how it places each */
static Sint16 qsound_ses[QSOUND_SES] = {
    47,  56,  89,  96,  105, 107, 125, 152, 153, 154,  263,  289,
    334, 372, 381, 393, 397, 441, 442, 457, 500, 509,  510,  511,
    518, 519, 540, 696, 772, 794, 806, 808, 813, 1002, 1105,
};

static struct qsound_se qsound_info[QSOUND_SES] = {
    {1, 2, 200, 0},   {1, 4, 200, 0},   {1, 2, 200, 0},   {2, 3, 200, 122},
    {1, 3, 200, 0},   {2, 1, 100, 108}, {1, 1, 200, 0},   {1, 2, 200, 0},
    {1, 3, 200, 0},   {1, 4, 200, 0},   {1, 1, 200, 0},   {1, 3, 200, 0},
    {1, 1, 200, 0},   {1, 1, 200, 0},   {1, 1, 200, 0},   {1, 2, 200, 0},
    {1, 3, 200, 0},   {1, 1, 200, 0},   {1, 2, 200, 0},   {1, 1, 200, 0},
    {1, 1, 200, 0},   {1, 2, 200, 0},   {1, 3, 200, 0},   {1, 4, 200, 0},
    {1, 3, 200, 0},   {1, 4, 200, 0},   {4, 1, 100, 549}, {1, 3, 200, 0},
    {1, 1, 200, 0},   {1, 1, 200, 0},   {1, 1, 200, 0},   {1, 1, 200, 0},
    {2, 3, 100, 836}, {1, 2, 200, 0},   {1, 2, 200, 0},
};

static int dat_8c7a7178; /* nothing reads or writes it */
static SDS_MIDI_MES midi_message;
/* the fx level asked for, which the next update sends, and the request
 * that asks for none */
static int sound_fx_request;
static int sound_fx_none;
static SDMIDI sound_ports[PORT_COUNT];
static struct sound_slot sound_slots[QUEUE_SLOTS];
/* the key each channel last played, for its note off */
static int midi_keys[SLOT_CHANNELS];
/* where the next search for a channel, and for a sequence slot, starts */
static int sound_next_channel;
static int sound_next_sequence;
static int dat_8c7a79e8; /* nothing reads or writes it */
/* the volume each slot was last sent */
static int sound_volumes[QUEUE_SLOTS];
static int dat_8c7a7a84[QUEUE_SLOTS]; /* nothing reads or writes it */
/* each qsound channel's place, and the place last sent */
static Sint8 qsound_pos[QSOUND_CHANNELS];
static Sint8 qsound_sent[QSOUND_CHANNELS];
/* the id each copy of each placed effect plays for, plus 1, or 0 when
 * free */
static int qsound_owners[QSOUND_SES][QSOUND_COPIES];

static void midi_reset_all(void);
static void midi_reset(int channel);
static void midi_program(int channel, int bank, int program);
static void midi_volume(int channel, int volume);
static void midi_pan(int channel, int pan);
static void midi_bend(int channel, int bend);
static void midi_note_on(int channel, int key, int velocity);
static void midi_note_off(int channel);
static void sound_slot_set_volume(int volume, int slot);
static void midi_open_ports(void);
static void sound_send_multi_unit(void *data);
static void sound_send_shot_bank(void *data, int bank, int size);
static void sound_send_sequence_bank(void *data, int bank, int size);
static void sound_send_program_bank(void *data, int bank, int size);
static void sound_init_driver(void);
static int sound_slot_alloc(int priority, int se);
static int sound_slot_find(int se, int id);
static int sound_distance_volume(int slot);
static void sound_set_fx_01(int level);
static void sound_set_fx_23(int level);
static void sound_reset(void);
static void qsound_reset(void);
static int qsound_range(int se);
static int qsound_alloc(int slot);
static void qsound_free(int slot);
static int qsound_place(int slot);
static Bool se_is_alt(int se);

void sound_system_init(void)
{
    int i;

    sound_muted = 0;
    sound_init_driver();
    midi_open_ports();
    sound_set_space();
    sound_reset();
    sound_next_channel = 0;
    sound_next_sequence = SLOT_CHANNELS;
    for (i = 0; i < QUEUE_SLOTS; i++) {
        sound_slots[i].time = 0;
    }
    midi_reset_all();
}

void sound_set_space(void)
{
    char work[SYD_BT_CFG_WORKSIZE];
    Sint32 mode;
    int tries;

    tries = 0;
    while (syCfgInit(work) != SYD_CFG_OK) {
        if (tries++ > CFG_TRIES) {
            break;
        }
    }
    tries = 0;
    while (syCfgGetSoundMode(&mode) != SYD_CFG_OK) {
        if (tries++ > CFG_TRIES) {
            mode = SYD_CFG_STEREO;
            break;
        }
    }
    if (mode == SYD_CFG_STEREO) {
        sdSndSetSpace(SDE_SPACE_STEREO);
    } else {
        sdSndSetSpace(SDE_SPACE_MONO);
    }
    syCfgExit();
}

static void midi_reset_all(void)
{
    int channel;

    midi_side = 0;
    for (channel = 0; channel < SLOT_CHANNELS; channel++) {
        midi_reset(channel);
    }
}

/* reset channel's controllers. each midi_ function takes channels 0 to 15
 * on port 0, and 16 to 31 on port 1, as channel & 15 */
static void midi_reset(int channel)
{
    int port;

    port = 0;
    if (channel > 15) {
        port = 1;
    }
    channel &= 15;
    sdMidiSetMes(sound_ports[port], &midi_message, channel | SDD_MIDI_CTL_CHG,
                 MIDI_RESET_CONTROLLERS, 0);
    sdMidiSendMes(sound_ports[port], &midi_message, 0);
}

/* ready channel to play program from bank, panned to the middle, with no
 * bend and full modulation */
static void midi_program(int channel, int bank, int program)
{
    int port;
    int pan;

    port = 0;
    pan = MIDI_PAN_MIDDLE;
    if (channel > 15) {
        port = 1;
    }
    channel &= 15;
    sdMidiSetMes(sound_ports[port], &midi_message, channel | SDD_MIDI_CTL_CHG,
                 MIDI_BANK, bank);
    sdMidiSendMes(sound_ports[port], &midi_message, 0);
    sdMidiSetMes(sound_ports[port], &midi_message, channel | SDD_MIDI_PRG_CHG,
                 program);
    sdMidiSendMes(sound_ports[port], &midi_message, 0);
    /* bank 0's program 7 pans to each side in turn instead */
    if (bank == 0 && program == 7) {
        if (midi_side != 0) {
            pan = MIDI_PAN_MIDDLE + MIDI_PAN_SIDE;
        } else {
            pan = MIDI_PAN_MIDDLE - MIDI_PAN_SIDE;
        }
        midi_side++;
        midi_side &= 1;
    }
    sdMidiSetMes(sound_ports[port], &midi_message, channel | SDD_MIDI_CTL_CHG,
                 MIDI_PAN, pan);
    sdMidiSendMes(sound_ports[port], &midi_message, 0);
    sdMidiSetMes(sound_ports[port], &midi_message,
                 channel | SDD_MIDI_PITCH_BEND, MIDI_BEND_MIDDLE & 127,
                 MIDI_BEND_MIDDLE >> 7);
    sdMidiSendMes(sound_ports[port], &midi_message, 0);
    sdMidiSetMes(sound_ports[port], &midi_message, channel | SDD_MIDI_CTL_CHG,
                 MIDI_MODULATION, 127);
    sdMidiSendMes(sound_ports[port], &midi_message, 0);
}

static void midi_volume(int channel, int volume)
{
    int port;

    port = 0;
    if (channel > 15) {
        port = 1;
    }
    channel &= 15;
    sdMidiSetMes(sound_ports[port], &midi_message, channel | SDD_MIDI_CTL_CHG,
                 MIDI_VOLUME, volume);
    sdMidiSendMes(sound_ports[port], &midi_message, 0);
}

/* pan from -64 to 63 */
static void midi_pan(int channel, int pan)
{
    int port;

    port = 0;
    if (channel > 15) {
        port = 1;
    }
    channel &= 15;
    pan += MIDI_PAN_MIDDLE;
    sdMidiSetMes(sound_ports[port], &midi_message, channel | SDD_MIDI_CTL_CHG,
                 MIDI_PAN, pan);
    sdMidiSendMes(sound_ports[port], &midi_message, 0);
}

/* bend from -8192 to 8191 */
static void midi_bend(int channel, int bend)
{
    int port;

    port = 0;
    if (channel > 15) {
        port = 1;
    }
    channel &= 15;
    bend += MIDI_BEND_MIDDLE;
    sdMidiSetMes(sound_ports[port], &midi_message,
                 channel | SDD_MIDI_PITCH_BEND, bend & 127, bend >> 7);
    sdMidiSendMes(sound_ports[port], &midi_message, 0);
}

static void midi_note_on(int channel, int key, int velocity)
{
    int port;

    port = 0;
    if (channel > 15) {
        port = 1;
    }
    midi_keys[channel] = key;
    channel &= 15;
    sdMidiSetMes(sound_ports[port], &midi_message, channel | SDD_MIDI_NOTE_ON,
                 key, velocity);
    sdMidiSendMes(sound_ports[port], &midi_message, 0);
}

static void midi_note_off(int channel)
{
    int port;
    int key;

    port = 0;
    if (channel > 15) {
        port = 1;
    }
    key = midi_keys[channel];
    channel &= 15;
    sdMidiSetMes(sound_ports[port], &midi_message, channel | SDD_MIDI_NOTE_OFF,
                 key, 0);
    sdMidiSendMes(sound_ports[port], &midi_message, 0);
}

/* the casino and the Chao gardens load files of their own for each
 * character, in place of its v_ and p_ files. a stage with no file of its
 * own loads drymountain.mlt */
void sound_load_stage(void)
{
    Uint16 stage;
    int character;
    const char *name;

    sound_muted = 0;
    stage = stage_get_stage_act() >> 8;
    switch (stage) {
    case STAGE_PRACTICE:
        sound_load_file("eggcarrier.mlt");
        break;
    case STAGE_BEACH:
        sound_load_file("emeraldcoast.mlt");
        break;
    case STAGE_WINDY:
        sound_load_file("windyvalley.mlt");
        break;
    case STAGE_TWINKLE:
        sound_load_file("twinklepark.mlt");
        break;
    case STAGE_HIGHWAY:
        sound_load_file("highway.mlt");
        break;
    case STAGE_MOUNTAIN:
        sound_load_file("drymountain.mlt");
        break;
    case STAGE_SKY_DECK:
        sound_load_file("skydeck.mlt");
        break;
    case STAGE_RUIN:
        sound_load_file("lostworld.mlt");
        break;
    case STAGE_ICECAP:
        sound_load_file("icecap.mlt");
        break;
    case STAGE_CASINO:
        sound_load_file("casinopolis.mlt");
        character = get_character();
        /* @hack spelling: Sonic and Tails, who load the same files, in cases of
         * their own, which keep the names in R13 and R14 (0x8c098812,
         * 0x8c098816) and give each a call (0x8c098834, 0x8c098880); as one
         * case, 224 of 279 words differ */
        switch (character) {
        case CHARACTER_SONIC:
            sound_load_file("casino_sonictails.mlt");
            sound_load_file("casino_sonictailsvoice.mlt");
            break;
        case CHARACTER_TAILS:
            sound_load_file("casino_sonictails.mlt");
            sound_load_file("casino_sonictailsvoice.mlt");
            break;
        case CHARACTER_KNUCKLES:
            sound_load_file("casino_knuckles.mlt");
            sound_load_file("casino_knucklesvoice.mlt");
            break;
        case CHARACTER_AMY:
            sound_load_file("casino_amy.mlt");
            sound_load_file("casino_amyvoice.mlt");
            break;
        case CHARACTER_E102:
            sound_load_file("casino_e102.mlt");
            sound_load_file("casino_e102voice.mlt");
            break;
        case CHARACTER_BIG:
            sound_load_file("casino_big.mlt");
            sound_load_file("casino_bigvoice.mlt");
            break;
        default:
            sound_load_file("casino_sonictails.mlt");
            sound_load_file("casino_sonictailsvoice.mlt");
            break;
        }
        goto done;
    case STAGE_FINAL:
        sound_load_file("finalegg.mlt");
        break;
    case STAGE_SHELTER:
        sound_load_file("hotshelter.mlt");
        break;
    case STAGE_CHAOS0:
        sound_load_file("chaos0.mlt");
        break;
    case STAGE_CHAOS2:
        sound_load_file("chaos2.mlt");
        break;
    case STAGE_CHAOS4:
        sound_load_file("chaos4.mlt");
        break;
    case STAGE_CHAOS6:
        sound_load_file("chaos6.mlt");
        break;
    case STAGE_CHAOS7:
        sound_load_file("chaos7b.mlt");
        break;
    case STAGE_EGGMOBILE1:
        sound_load_file("mobile1.mlt");
        break;
    case STAGE_EGGMOBILE2:
        sound_load_file("mobile2.mlt");
        break;
    case STAGE_EGGMOBILE3:
        sound_load_file("mobile3.mlt");
        break;
    case STAGE_E101:
        sound_load_file("e101.mlt");
        break;
    case STAGE_E101_R:
        sound_load_file("e101r.mlt");
        break;
    case STAGE_STATION_SQUARE:
    case STAGE_STATION_SQUARE_2:
    case STAGE_STATION_SQUARE_3:
        sound_load_file("stationsquare.mlt");
        break;
    case STAGE_EGG_CARRIER:
    case STAGE_EGG_CARRIER_2:
    case STAGE_EGG_CARRIER_3:
    case STAGE_EGG_CARRIER_INSIDE:
        sound_load_file("eggcarrier.mlt");
        break;
    case STAGE_MYSTIC_RUINS:
    case STAGE_PAST:
        sound_load_file("mysticruin.mlt");
        break;
    case STAGE_ALIFE_SS:
    case STAGE_ALIFE_EG:
    case STAGE_ALIFE_MR:
    case STAGE_ALIFE_RACE:
        sound_load_file("alife.mlt");
        character = get_character();
        switch (character) {
        case CHARACTER_SONIC:
            sound_load_file("alife_sonic.mlt");
            break;
        case CHARACTER_TAILS:
            sound_load_file("alife_tails.mlt");
            break;
        case CHARACTER_KNUCKLES:
            sound_load_file("alife_knuckles.mlt");
            break;
        case CHARACTER_AMY:
            sound_load_file("alife_amy.mlt");
            break;
        case CHARACTER_E102:
            sound_load_file("alife_e102.mlt");
            break;
        case CHARACTER_BIG:
            sound_load_file("alife_big.mlt");
            break;
        default:
            sound_load_file("alife_sonic.mlt");
            break;
        }
        goto done;
    /* @hack spelling: the minigames set name and share the call at load, which
     * SHC merges with the default's and other stages' calls (BSR at
     * 0x8c098a40); with a call in each case, 95 of 279 words differ */
    case STAGE_CART:
        name = "cart.mlt";
        goto load;
    case STAGE_SHOOTING1:
    case STAGE_SHOOTING2:
        name = "shooting.mlt";
        goto load;
    case STAGE_SUNABOARD:
        name = "sandhill.mlt";
        goto load;
    default:
        sound_load_file("drymountain.mlt");
        break;
    load:
        sound_load_file(name);
        break;
    }
    character = get_character();
    /* @hack spelling: Sonic and Tails, who load the same files, in cases of
     * their own, which keep the names in R13 and R14 (0x8c098a48,
     * 0x8c098a4c); with one case, 93 of 279 words differ */
    switch (character) {
    case CHARACTER_SONIC:
        sound_load_file("v_sonictails.mlt");
        sound_load_file("p_sonictails.mlt");
        break;
    case CHARACTER_TAILS:
        sound_load_file("v_sonictails.mlt");
        sound_load_file("p_sonictails.mlt");
        break;
    case CHARACTER_KNUCKLES:
        sound_load_file("v_knuckles.mlt");
        sound_load_file("p_knuckles.mlt");
        break;
    case CHARACTER_AMY:
        sound_load_file("v_amy.mlt");
        sound_load_file("p_amy.mlt");
        break;
    case CHARACTER_E102:
        sound_load_file("v_e102.mlt");
        sound_load_file("p_e102.mlt");
        break;
    case CHARACTER_BIG:
        sound_load_file("v_big.mlt");
        sound_load_file("p_big.mlt");
        break;
    default:
        sound_load_file("v_sonictails.mlt");
        sound_load_file("p_sonictails.mlt");
        break;
    }
done:
    sound_reset();
    midi_reset_all();
}

void sound_load_file(const char *name)
{
    Sint32 size;
    GDFS file;
    Sint32 sectors;

    file = gdFsOpen(name, NULL);
    gdFsGetFileSize(file, &size);
    sectors = SECTORS(size);
    gdFsRead(file, sectors, WORK_AREA);
    gdFsClose(file);
    sound_send_multi_unit(WORK_AREA);
}

void sound_load_shot_bank(int bank, const char *name)
{
    Sint32 size;
    GDFS file;
    Sint32 sectors;

    file = gdFsOpen(name, NULL);
    gdFsGetFileSize(file, &size);
    sectors = SECTORS(size);
    gdFsRead(file, sectors, WORK_AREA);
    gdFsClose(file);
    sound_send_shot_bank(WORK_AREA, bank, size);
}

void sound_load_sequence_bank(int bank, const char *name)
{
    Sint32 size;
    GDFS file;
    Sint32 sectors;

    file = gdFsOpen(name, NULL);
    gdFsGetFileSize(file, &size);
    sectors = SECTORS(size);
    gdFsRead(file, sectors, WORK_AREA);
    gdFsClose(file);
    sound_send_sequence_bank(WORK_AREA, bank, size);
}

void sound_load_program_bank(int bank, const char *name)
{
    Sint32 size;
    GDFS file;
    Sint32 sectors;

    file = gdFsOpen(name, NULL);
    gdFsGetFileSize(file, &size);
    sectors = SECTORS(size);
    gdFsRead(file, sectors, WORK_AREA);
    gdFsClose(file);
    sound_send_program_bank(WORK_AREA, bank, size);
}

void sound_download(void *data)
{
    sound_send_multi_unit(data);
}

/* a sequence slot plays sequence number of bank at priority. a channel
 * takes number of bank as its program, and plays midi_key at full
 * velocity */
void sound_slot_play(int bank, int number, int priority, int slot)
{
    if (slot >= SLOT_CHANNELS) {
        sdMidiPlay(sound_ports[slot - (SLOT_CHANNELS - SEQUENCE_PORT_FIRST)],
                   bank, number, priority);
    } else {
        midi_program(slot, bank, number);
        midi_note_on(slot, midi_key, 127);
    }
}

void sound_slot_stop(int slot)
{
    if (slot >= SLOT_CHANNELS) {
        sdMidiStop(sound_ports[slot - (SLOT_CHANNELS - SEQUENCE_PORT_FIRST)]);
        return;
    }
    midi_note_off(slot);
}

/* a channel sends CHANNEL_VOLUME for volume 0, one less for each step
 * below and one more for each six above, within 0 to 127. a sequence slot
 * sends volume to its port as it is */
static void sound_slot_set_volume(int volume, int slot)
{
#ifdef GCC
    /* a skip sends the least volume, so the queue runs on unheard */
    if (event_skipping) {
        volume = VOLUME_LEAST;
    }
#endif
    if (slot >= SLOT_CHANNELS) {
        /* @hack spelling: the port as the slot's index among the sequence
         * slots plus SEQUENCE_PORT_FIRST, which SHC indexes from
         * sound_ports[SEQUENCE_PORT_FIRST] (ADD #-32,R4 at 0x8c098d72, and
         * 0x8c7a7190 in the pool); as in sound_slot_play, 4 of 41 words
         * differ */
        sdMidiSetVol(sound_ports[slot - SLOT_CHANNELS + SEQUENCE_PORT_FIRST],
                     volume, 0);
        return;
    }
    if (volume > 0) {
        volume /= 6;
    }
    volume += CHANNEL_VOLUME;
    if (volume < 0) {
        volume = 0;
    }
    if (volume > 127) {
        volume = 127;
    }
    midi_volume(slot, volume);
}

void sound_slot_set_pan(int pan, int slot)
{
    if (slot < SLOT_CHANNELS) {
        midi_pan(slot, pan / 2);
    }
}

void sound_slot_set_pitch(int pitch, int slot)
{
    /* @hack spelling: an empty if with the call in its else, which keeps
     * midi_bend's address in R2 where sound_send_queue inlines this
     * (0x8c09a168, 0x8c09a238); as a plain if, it takes R1 and 4 of 533
     * words differ */
    if (slot >= SLOT_CHANNELS) {
    } else {
        midi_bend(slot, pitch);
    }
}

void sound_set_qsound(Sint8 *pos)
{
    sdQsndSetPos(pos);
}

static void midi_open_ports(void)
{
    int i;

    for (i = 0; i < PORT_COUNT; i++) {
        sdMidiOpenPort(&sound_ports[i]);
    }
}

static void sound_send_multi_unit(void *data)
{
    SDMEMBLK block;
    SDE_ERR error;

    block = NULL;
    error = sdMemBlkCreate(&block);
    if (error != SDE_ERR_NOTHING) {
        goto done;
    }
    error = sdMemBlkSetPrm(block, data, 0, SDD_MEMBLK_SYNC_FUNC, NULL);
    if (error != SDE_ERR_NOTHING) {
        goto done;
    }
    error = sdMultiUnitDownload(block);
    /* @hack redundant: a last test whose goto reaches the next statement,
     * which keeps a MOV R0,R4 and TST R4,R4 with no branch (0x8c098e94);
     * without it, 14 of 38 words differ */
    if (error != SDE_ERR_NOTHING) {
        goto done;
    }
done:
    if (block != NULL) {
        sdMemBlkDestroy(block);
    }
}

static void sound_send_shot_bank(void *data, int bank, int size)
{
    SDMEMBLK block;
    SDE_ERR error;

    block = NULL;
    error = sdMemBlkCreate(&block);
    if (error != SDE_ERR_NOTHING) {
        goto done;
    }
    error = sdMemBlkSetPrm(block, data, size, SDD_MEMBLK_NO_FUNC, NULL);
    if (error != SDE_ERR_NOTHING) {
        goto done;
    }
    error = sdBankDownload(block, SDE_DATA_TYPE_SHOT_BANK, bank);
    /* @hack redundant: a last test whose goto reaches the next statement,
     * which keeps a MOV R0,R4 and TST R4,R4 with no branch (0x8c098ee8);
     * without it, 20 of 42 words differ */
    if (error != SDE_ERR_NOTHING) {
        goto done;
    }
done:
    if (block != NULL) {
        sdMemBlkDestroy(block);
    }
}

static void sound_send_sequence_bank(void *data, int bank, int size)
{
    SDMEMBLK block;
    SDE_ERR error;

    block = NULL;
    error = sdMemBlkCreate(&block);
    if (error != SDE_ERR_NOTHING) {
        goto done;
    }
    error = sdMemBlkSetPrm(block, data, size, SDD_MEMBLK_NO_FUNC, NULL);
    if (error != SDE_ERR_NOTHING) {
        goto done;
    }
    error = sdBankDownload(block, SDE_DATA_TYPE_MIDI_SEQ_BANK, bank);
    /* @hack redundant: a last test whose goto reaches the next statement,
     * which keeps a MOV R0,R4 and TST R4,R4 with no branch (0x8c098f3c);
     * without it, 20 of 42 words differ */
    if (error != SDE_ERR_NOTHING) {
        goto done;
    }
done:
    if (block != NULL) {
        sdMemBlkDestroy(block);
    }
}

static void sound_send_program_bank(void *data, int bank, int size)
{
    SDMEMBLK block;
    SDE_ERR error;

    block = NULL;
    error = sdMemBlkCreate(&block);
    if (error != SDE_ERR_NOTHING) {
        goto done;
    }
    error = sdMemBlkSetPrm(block, data, size, SDD_MEMBLK_NO_FUNC, NULL);
    if (error != SDE_ERR_NOTHING) {
        goto done;
    }
    error = sdBankDownload(block, SDE_DATA_TYPE_MIDI_PRG_BANK, bank);
    /* @hack redundant: a last test whose goto reaches the next statement,
     * which keeps a MOV R0,R4 and TST R4,R4 with no branch (0x8c098f90);
     * without it, 20 of 42 words differ */
    if (error != SDE_ERR_NOTHING) {
        goto done;
    }
done:
    if (block != NULL) {
        sdMemBlkDestroy(block);
    }
}

/* read masaru.drv into WORK_AREA and start the driver from it, then serve
 * the sound library from the vsync. a call that fails skips the rest of the
 * start. sdDrvInit runs again after the block is destroyed, with its handle
 * or NULL */
static void sound_init_driver(void)
{
    SDMEMBLK block;
    Sint32 size;
    GDFS file;
    Sint32 sectors;
    SDE_ERR error;

    block = NULL;
    file = gdFsOpen("masaru.drv", NULL);
    gdFsGetFileSize(file, &size);
    sectors = SECTORS(size);
    gdFsRead(file, sectors, WORK_AREA);
    gdFsClose(file);
    error = sdLibInit(NULL, 0, 0);
    if (error != SDE_ERR_NOTHING) {
        goto done;
    }
    sdMemBlkSetTransferMode(SDE_MEMBLK_TRANSFER_MODE_DMA);
    error = sdMemBlkCreate(&block);
    if (error != SDE_ERR_NOTHING) {
        goto done;
    }
    error = sdMemBlkSetPrm(block, WORK_AREA, 0, SDD_MEMBLK_SYNC_FUNC, NULL);
    if (error != SDE_ERR_NOTHING) {
        goto done;
    }
    error = sdDrvInit(block);
    /* @hack redundant: a last test whose goto reaches the next statement,
     * which keeps a MOV R0,R4 and TST R4,R4 with no branch (0x8c099058);
     * without it, 22 of 79 words differ */
    if (error != SDE_ERR_NOTHING) {
        goto done;
    }
done:
    if (block != NULL) {
        sdMemBlkDestroy(block);
    }
    sdDrvInit(block);
    njSetVSyncFunction(frame_vsync);
}

/* a slot for se, or -1 for none. an effect that is a sequence takes a
 * sequence slot below SLOT_TEST, and any other a channel. the search starts
 * after the slot taken last, and takes the first slot that is free or held
 * at no more than priority */
static int sound_slot_alloc(int priority, int se)
{
    int slot;
    int found;
    int i;

#ifdef GCC
    /* the split frame's second picture plays no sound */
    if (frame_redrawing) {
        return -1;
    }
#endif
    found = -1;
    if (se == 0 || se == 66 || se == 67 || se == 124 || se == 161 ||
        se == 160 || se == 239 || se == 553 || se == 554 || se == 1059 ||
        se == 1039 || se == 1080 || se == 667 || se == 1027 || se == 1029 ||
        (se >= 777 && se <= 781) || (se >= 1215 && se <= 1222) ||
        (se >= 1264 && se <= 1271) || se == 1273) {
        slot = sound_next_sequence;
        for (i = SLOT_TEST; found < 0 && i > SLOT_CHANNELS; i--) {
            if (sound_slots[slot].time == 0) {
                found = slot;
            }
            if (sound_slots[slot].priority <= priority) {
                found = slot;
            }
            if (++slot >= SLOT_TEST) {
                slot = SLOT_CHANNELS;
            }
        }
        /* @bug logic: a search that finds no slot sets sound_next_sequence to
         * 0, found + 1 (ADD #1,R1 at 0x8c0991fa, MOV.L R1,@R11 at 0x8c099202).
         * the next sequence search then looks at channels 0 to 4, and can give
         * a sequence effect a channel */
        sound_next_sequence = found + 1;
        if (sound_next_sequence >= SLOT_TEST) {
            sound_next_sequence = SLOT_CHANNELS;
        }
    } else {
        slot = sound_next_channel;
        for (i = SLOT_CHANNELS; found < 0 && i > 0; i--) {
            if (sound_slots[slot].time == 0) {
                found = slot;
            }
            if (sound_slots[slot].priority <= priority) {
                found = slot;
            }
            if (++slot >= SLOT_CHANNELS) {
                slot = 0;
            }
        }
        sound_next_channel = found + 1;
        if (sound_next_channel >= SLOT_CHANNELS) {
            sound_next_channel = 0;
        }
    }
    return found;
}

int sound_play(int se, int id, int priority, int volume)
{
    int slot;

    slot = sound_slot_alloc(priority, se);
    if (slot < 0) {
        return -1;
    }
    sound_slots[slot].time = ONCE_UPDATES;
    sound_slots[slot].priority = -1;
    sound_slots[slot].id = id;
    sound_slots[slot].flags = SLOT_STOP | SLOT_START;
    sound_slots[slot].se = se;
    sound_slots[slot].pan = 0;
    sound_slots[slot].volume = volume;
    sound_slots[slot].loudest = 0;
    sound_slots[slot].pitch = 0;
    sound_slots[slot].pos.x = 0.0F;
    sound_slots[slot].pos.y = 0.0F;
    sound_slots[slot].pos.z = 0.0F;
    /* @hack spelling: an accepted request returns the 44 left in R0, the offset
     * of pos.z, which MOV #44,R0 at 0x8c0992be loads for the store above. no
     * caller reads the result, so with return slot, 12 of 49 words differ */
#ifdef GCC
    /* repairs the return the tag names: GCC's is slot, the one the request
     * took */
    return slot;
#endif
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}

int sound_play_at(int se, int id, int priority, int volume, Float x, Float y,
                  Float z)
{
    int slot;
    int range;
    int qsound;
    Float reach;
    Float dx;
    Float dy;
    Float dz;

    qsound = 0;
    range = qsound_range(se);
    if (range >= 0) {
        dx = x - camera_work->pos.x;
        dy = y - camera_work->pos.y;
        dz = z - camera_work->pos.z;
        reach = (Float)range;
        reach *= reach;
        /* reach is range squared. the test squares it again against the
         * squared distance, so the limit is reach, not range */
        if (vector_length_squared(dx, dy, dz) > reach * reach) {
            return -1;
        }
        qsound = SLOT_QSOUND;
    }
    slot = sound_slot_alloc(priority, se);
    if (slot < 0) {
        return -1;
    }
    sound_slots[slot].time = ONCE_UPDATES;
    sound_slots[slot].priority = -1;
    sound_slots[slot].id = id;
    sound_slots[slot].flags = qsound | SLOT_DISTANCE | SLOT_STOP | SLOT_START;
    sound_slots[slot].se = se;
    sound_slots[slot].pan = 0;
    sound_slots[slot].volume = volume;
    sound_slots[slot].loudest = volume;
    sound_slots[slot].pitch = 0;
    sound_slots[slot].pos.x = x;
    sound_slots[slot].pos.y = y;
    sound_slots[slot].pos.z = z;
    /* @hack spelling: the 44 left in R0, as in sound_play (MOV #44,R0 at
     * 0x8c0993b8). no caller reads the result, so with return slot, 81 of 101
     * words differ */
#ifdef GCC
    /* repairs the return the tag names: GCC's is slot, the one the request
     * took */
    return slot;
#endif
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}

int sound_play_held(int se, int id, int priority, int volume)
{
    int slot;

    slot = sound_slot_alloc(priority, se);
    if (slot < 0) {
        return -1;
    }
    sound_slots[slot].time = -1;
    sound_slots[slot].priority = priority;
    sound_slots[slot].id = id;
    sound_slots[slot].flags = SLOT_STOP | SLOT_START;
    sound_slots[slot].se = se;
    sound_slots[slot].pan = 0;
    sound_slots[slot].volume = volume;
    sound_slots[slot].loudest = 0;
    sound_slots[slot].pitch = 0;
    sound_slots[slot].pos.x = 0.0F;
    sound_slots[slot].pos.y = 0.0F;
    sound_slots[slot].pos.z = 0.0F;
    /* @hack spelling: the 44 left in R0, as in sound_play (MOV #44,R0 at
     * 0x8c099456). no caller reads the result, so with return slot, 12 of 49
     * words differ */
#ifdef GCC
    /* repairs the return the tag names: GCC's is slot, the one the request
     * took */
    return slot;
#endif
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}

/* the highest slot below SLOT_TEST playing se for id, or -1 */
static int sound_slot_find(int se, int id)
{
    int i;
    int found;

#ifdef GCC
    /* the split frame's second picture refreshes no sound */
    if (frame_redrawing) {
        return -1;
    }
#endif
    found = -1;
    for (i = 0; i < SLOT_TEST; i++) {
        if (sound_slots[i].id == id) {
            if (sound_slots[i].se == se) {
                found = i;
            }
        }
    }
    return found;
}

int sound_play_for(int se, int id, int priority, int volume, int updates)
{
    int slot;
    int start;

    slot = sound_slot_find(se, id);
    if (slot >= 0) {
        start = 0;
    } else {
        slot = sound_slot_alloc(priority, se);
        if (slot < 0) {
            return -1;
        }
        start = SLOT_START;
    }
    sound_slots[slot].time = updates + 1;
    sound_slots[slot].priority = priority;
    sound_slots[slot].id = id;
    sound_slots[slot].flags = start | SLOT_STOP;
    sound_slots[slot].se = se;
    sound_slots[slot].pan = 0;
    sound_slots[slot].volume = volume;
    sound_slots[slot].loudest = 0;
    sound_slots[slot].pitch = 0;
    sound_slots[slot].pos.x = 0.0F;
    sound_slots[slot].pos.y = 0.0F;
    sound_slots[slot].pos.z = 0.0F;
    /* @hack spelling: the 44 left in R0, as in sound_play (MOV #44,R0 at
     * 0x8c0995d8). no caller reads the result, so with return slot, 6 of 60
     * words differ */
#ifdef GCC
    /* repairs the return the tag names: GCC's is slot, the one the request
     * took */
    return slot;
#endif
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}

int sound_play_at_for(int se, int id, int priority, int volume, Float x,
                      Float y, Float z, int updates)
{
    int slot;
    int range;
    int flags;
    Float reach;
    Float dx;
    Float dy;
    Float dz;

    flags = 0;
    range = qsound_range(se);
    if (range >= 0) {
        dx = x - camera_work->pos.x;
        dy = y - camera_work->pos.y;
        dz = z - camera_work->pos.z;
        reach = (Float)range;
        reach *= reach;
        if (vector_length_squared(dx, dy, dz) > reach * reach) {
            return -1;
        }
        flags = SLOT_QSOUND;
    }
    slot = sound_slot_find(se, id);
    if (slot < 0) {
        slot = sound_slot_alloc(priority, se);
        if (slot < 0) {
            return -1;
        }
        flags |= SLOT_START;
    }
    sound_slots[slot].time = updates + 1;
    sound_slots[slot].priority = priority;
    sound_slots[slot].id = id;
    sound_slots[slot].flags = flags | SLOT_DISTANCE | SLOT_FOLLOW | SLOT_STOP;
    sound_slots[slot].se = se;
    sound_slots[slot].pan = 0;
    sound_slots[slot].volume = volume;
    sound_slots[slot].loudest = volume;
    sound_slots[slot].pitch = 0;
    sound_slots[slot].pos.x = x;
    sound_slots[slot].pos.y = y;
    sound_slots[slot].pos.z = z;
    /* @hack spelling: the 44 left in R0, as in sound_play (MOV #44,R0 at
     * 0x8c0996e4). no caller reads the result, so with return slot, 8 of 107
     * words differ */
#ifdef GCC
    /* repairs the return the tag names: GCC's is slot, the one the request
     * took */
    return slot;
#endif
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}

int sound_play_around(int se, int id, int priority, int volume, Float x,
                      Float y, Float z, Float radius, int updates)
{
    int slot;
    int start;
    Float dx;
    Float dy;
    Float dz;

    dx = x - camera_work->pos.x;
    dy = y - camera_work->pos.y;
    dz = z - camera_work->pos.z;
    if (vector_length_squared(dx, dy, dz) > radius * radius) {
        return -1;
    }
    slot = sound_slot_find(se, id);
    if (slot >= 0) {
        start = 0;
    } else {
        slot = sound_slot_alloc(priority, se);
        if (slot < 0) {
            return -1;
        }
        sound_slots[slot].qsound = -1;
        start = SLOT_START;
    }
    sound_slots[slot].time = updates + 1;
    sound_slots[slot].priority = priority;
    sound_slots[slot].id = id;
    sound_slots[slot].flags =
        start | SLOT_QSOUND | SLOT_DISTANCE | SLOT_FOLLOW | SLOT_STOP;
    sound_slots[slot].se = se;
    sound_slots[slot].pan = 0;
    sound_slots[slot].volume = volume;
    sound_slots[slot].loudest = volume;
    sound_slots[slot].pitch = 0;
    sound_slots[slot].pos.x = x;
    sound_slots[slot].pos.y = y;
    sound_slots[slot].pos.z = z;
    /* @hack spelling: the 44 left in R0, as in sound_play (MOV #44,R0 at
     * 0x8c0997f0). no caller reads the result, so with return slot, 6 of 103
     * words differ */
#ifdef GCC
    /* repairs the return the tag names: GCC's is slot, the one the request
     * took */
    return slot;
#endif
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}

void sound_stop(int se)
{
    int slot;

    for (slot = 0; slot < QUEUE_SLOTS; slot++) {
        if (sound_slots[slot].se == se) {
            sound_slots[slot].time = 0;
            sound_slots[slot].flags |= SLOT_STOP;
        }
    }
}

void sound_stop_id(int id)
{
    int slot;

    for (slot = 0; slot < QUEUE_SLOTS; slot++) {
        if (sound_slots[slot].id == id) {
            sound_slots[slot].time = 0;
            sound_slots[slot].flags |= SLOT_STOP;
        }
    }
}

void sound_set_volume(int id, int volume)
{
    int slot;

    for (slot = 0; slot < QUEUE_SLOTS; slot++) {
        if (sound_slots[slot].id == id) {
            sound_slots[slot].volume = volume;
            sound_slots[slot].flags &= ~SLOT_DISTANCE;
            sound_slots[slot].flags |= SLOT_VOLUME;
        }
    }
}

void sound_set_pan(int id, int pan)
{
    int slot;

    for (slot = 0; slot < QUEUE_SLOTS; slot++) {
        if (sound_slots[slot].id == id) {
            sound_slots[slot].pan = pan;
            sound_slots[slot].flags &= ~SLOT_UNK_400;
            sound_slots[slot].flags |= SLOT_PAN;
        }
    }
}

void sound_set_pitch(int id, int pitch)
{
    int slot;

    for (slot = 0; slot < QUEUE_SLOTS; slot++) {
        if (sound_slots[slot].id == id) {
            sound_slots[slot].pitch = pitch;
            sound_slots[slot].flags |= SLOT_PITCH;
        }
    }
}

void sound_mute(void)
{
    int slot;

    sound_muted = -1;
    for (slot = 0; slot < QUEUE_SLOTS; slot++) {
        sound_slot_set_volume(VOLUME_LEAST, slot);
    }
    stream_mute();
}

void sound_unmute(void)
{
    int slot;

    sound_muted = 0;
    for (slot = 0; slot < QUEUE_SLOTS; slot++) {
        sound_slot_set_volume(sound_volumes[slot], slot);
    }
    stream_unmute();
}

void sound_kill_all(void)
{
    int slot;

    for (slot = 0; slot < QUEUE_SLOTS; slot++) {
        sound_slots[slot].time = 0;
        sound_slots[slot].flags |= SLOT_STOP;
        sound_slot_stop(slot);
    }
}

/* the slot's volume less the falloff by its distance from the camera, from
 * VOLUME_LEAST to loudest. without a camera there is no falloff */
static int sound_distance_volume(int slot)
{
    Float squared;
    Float fade;
    int volume;
    Float dx;
    Float dy;
    Float dz;

    if (camera_work != NULL) {
        dx = sound_slots[slot].pos.x - camera_work->pos.x;
        dy = sound_slots[slot].pos.y - camera_work->pos.y;
        dz = sound_slots[slot].pos.z - camera_work->pos.z;
        dx *= dx;
        dy *= dy;
        dz *= dz;
        squared = dx + dy + dz;
    } else {
        squared = 0.0F;
    }
    if (squared < FALLOFF_NEAR * FALLOFF_NEAR) {
        volume = 0;
    } else {
        fade = njSqrt(squared);
        fade = (fade - FALLOFF_NEAR) * (1.0F / (FALLOFF_FAR - FALLOFF_NEAR));
        if (fade < 0.0F) {
            fade = 0.0F;
        }
        /* @hack spelling: the falloff taken from 0.0F, not negated, which
         * keeps 0.0F in FR15 for it (FLDI0 FR15 at 0x8c099e1a, FSUB FR4,FR15
         * at 0x8c099e82); negated, 85 of 85 words differ */
        volume = 0.0F - (fade * FALLOFF_MOST);
    }
    volume += sound_slots[slot].volume;
    if (volume < VOLUME_LEAST) {
        volume = VOLUME_LEAST;
    }
    if (volume > sound_slots[slot].loudest) {
        volume = sound_slots[slot].loudest;
    }
    return volume;
}

void sound_send_queue(void)
{
    int slot;
    int flags;
    int volume;
    int se;
    int bank;
    int copy;
    int base;
    int line;
    int by_distance;
    int i;
    Bool changed;
    Sint32 *range;
    int place;

    if (sound_muted != 0) {
        return;
    }
    if (debug_page == DEBUG_SLOTS) {
        njPrintC(NJM_LOCATION(2, SLOT_ROW - 1), "PR TIME SE VOL ID");
    }
    for (slot = 0; slot < QUEUE_SLOTS; slot++) {
        flags = sound_slots[slot].flags;
        line = slot + SLOT_ROW;
        by_distance = flags & SLOT_DISTANCE;
        volume = 0;
        if (flags & SLOT_START) {
            if (debug_page == DEBUG_SLOTS) {
                /* @hack spelling: the row worked out again, not read from
                 * line, which keeps the qsound_sent loop's store pointer in R5
                 * and its load pointer in R6 (0x8c09a352, 0x8c09a35c); with
                 * line, the two trade and 19 of 520 words differ */
                njPrintC(NJM_LOCATION(1, slot + SLOT_ROW), "*");
            }
            se = sound_slots[slot].se;
            if (se > SE_LAST) {
                stream_request_voice(se - SE_LAST - 1 + VOICE_FIRST);
                flags &= ~SLOT_STOP;
                sound_slots[slot].time = 0;
            } else {
                bank = 0;
                base = 0;
                /* @hack redundant: the end test reads base, which is 0 at
                 * every test, and SHC tests it (TST R9,R9 at 0x8c099fae,
                 * 0x8c09a028); without it, 117 of 520 words differ */
                for (range = se_ranges[0]; *range >= 0 || base; range += 2) {
                    if (se > *range) {
                        base = *range++;
                        bank = *range;
                        break;
                    }
                }
                /* the alt character's sounds play from banks 1 and 4, in
                 * place of the player's 6 and 3 */
                if (se_is_alt(se)) {
                    if (bank == 6) {
                        bank = 1;
                    }
                    if (bank == 3) {
                        bank = 4;
                    }
                }
                if (flags & SLOT_QSOUND) {
                    copy = qsound_alloc(slot);
                    if (copy >= 0) {
                        /* @hack spelling: the place taken through place, which
                         * reads the channel after the call (BSR at
                         * 0x8c09a056, MOV.L @(48,R3),R2 at 0x8c09a062); in
                         * one statement, SHC keeps the channel's address on
                         * the stack across it, and 60 of 520 words differ */
                        place = qsound_place(slot);
                        qsound_pos[sound_slots[slot].qsound] = place;
                    } else {
                        copy = 0;
                    }
                    if (base != 0) {
                        sound_slot_play(bank, se + copy - base - 1, 0, slot);
                    } else {
                        sound_slot_play(0, se + copy, 0, slot);
                    }
                } else {
                    if (base != 0) {
                        sound_slot_play(bank, se - base - 1, 0, slot);
                    } else {
                        sound_slot_play(0, se, 0, slot);
                    }
                }
                if (by_distance) {
                    volume = sound_distance_volume(slot);
                } else if (flags & SLOT_VOLUME) {
                    volume = sound_slots[slot].volume;
                } else {
                    volume = 0;
                }
                sound_slot_set_volume(volume, slot);
                sound_volumes[slot] = volume;
                if (flags & SLOT_PAN) {
                    sound_slot_set_pan(sound_slots[slot].pan, slot);
                }
                if (flags & SLOT_PITCH) {
                    sound_slot_set_pitch(sound_slots[slot].pitch, slot);
                }
            }
            flags &= ~SLOT_START;
        } else {
            if (sound_slots[slot].time > 0) {
                if (flags & (SLOT_FOLLOW | SLOT_FOLLOW_ONCE)) {
                    if (by_distance) {
                        volume = sound_distance_volume(slot);
                        if (sound_volumes[slot] != volume) {
                            sound_slot_set_volume(volume, slot);
                            sound_volumes[slot] = volume;
                        }
                    }
                    flags &= ~SLOT_FOLLOW_ONCE;
                }
                if (flags & SLOT_VOLUME) {
                    volume = sound_slots[slot].volume;
                    if (sound_volumes[slot] != volume) {
                        sound_slot_set_volume(volume, slot);
                        sound_volumes[slot] = volume;
                    }
                }
                if (flags & SLOT_PAN) {
                    sound_slot_set_pan(sound_slots[slot].pan, slot);
                    flags &= ~SLOT_PAN;
                }
                if (flags & SLOT_PITCH) {
                    sound_slot_set_pitch(sound_slots[slot].pitch, slot);
                    flags &= ~SLOT_PITCH;
                }
                if (flags & SLOT_QSOUND) {
                    if (sound_slots[slot].qsound >= 0) {
                        /* @hack spelling: the place taken through place, as at
                         * the start (BSR at 0x8c09a252); in one statement,
                         * 205 of 520 words differ */
                        place = qsound_place(slot);
                        qsound_pos[sound_slots[slot].qsound] = place;
                    }
                }
                sound_slots[slot].time--;
            }
            if (sound_slots[slot].time == 0) {
                if (flags & SLOT_STOP) {
                    sound_slot_stop(slot);
                }
                if (sound_slots[slot].flags & SLOT_QSOUND) {
                    qsound_free(slot);
                }
                flags = 0;
                sound_slots[slot].id = 0;
                sound_slots[slot].se = -1;
                sound_slots[slot].time = 0;
            }
        }
        sound_slots[slot].flags = flags;
        if (debug_page == DEBUG_SLOTS) {
            njPrintD(NJM_LOCATION(2, line), sound_slots[slot].priority, 1);
            njPrintD(NJM_LOCATION(5, line), sound_slots[slot].time, 3);
            njPrintD(NJM_LOCATION(9, line), sound_slots[slot].se, 2);
            njPrintH(NJM_LOCATION(14, line), volume, 2);
            njPrintH(NJM_LOCATION(17, line), sound_slots[slot].id, 6);
        }
    }
    changed = FALSE;
    for (i = 0; i < QSOUND_USED; i++) {
        if (qsound_pos[i] != qsound_sent[i]) {
            changed = TRUE;
            qsound_sent[i] = qsound_pos[i];
            qsound_pos[i] |= QSOUND_CHANGED;
        }
    }
    if (changed) {
        sdQsndSetPos(qsound_pos);
    }
    for (i = 0; i < QSOUND_USED; i++) {
        qsound_pos[i] &= ~QSOUND_CHANGED;
    }
    if (sound_fx_request != sound_fx_none) {
        sound_set_fx_01(sound_fx_request);
        sound_fx_request = FX_RESET;
    }
}

void fun_8c09a47e(void)
{
}

void sound_end_act(void)
{
}

void sound_pause(void)
{
}

void sound_resume(int unused)
{
}

/* set fx outputs 0 and 1 to level */
static void sound_set_fx_01(int level)
{
    sdSndSetFxOutPrm(0, 0, level * FX_STEP);
    sdSndSetFxOutPrm(1, 0, level * FX_STEP);
}

/* set fx outputs 2 and 3 to level */
static void sound_set_fx_23(int level)
{
    sdSndSetFxOutPrm(2, 0, level * FX_STEP);
    sdSndSetFxOutPrm(3, 0, level * FX_STEP);
}

static void sound_reset(void)
{
    sdSndSetFxPrg(0, 0);
    sound_set_fx_01(FX_RESET);
    sound_fx_request = sound_fx_none = FX_RESET;
    qsound_reset();
}

void sound_request_fx(int level)
{
    sound_fx_request = level;
}

void sound_request_fx_max(void)
{
    sound_fx_request = FX_MAX;
}

void sound_test(void)
{
    njPrintC(NJM_LOCATION(10, 11), "SOUND TEST:B[  ] N[  ]");
    njPrintD(NJM_LOCATION(23, 11), sound_test_bank, 2);
    njPrintD(NJM_LOCATION(29, 11), sound_test_number, 2);
    if ((pad_records[0].press & PDD_DGT_KU) |
        (pad_records[3].press & PDD_DGT_KU)) {
        sound_test_number++;
    }
    if (sound_test_number > 99) {
        sound_test_number = 0;
    }
    if ((pad_records[0].press & PDD_DGT_KD) |
        (pad_records[3].press & PDD_DGT_KD)) {
        sound_test_number--;
    }
    if (sound_test_number < 0) {
        sound_test_number = 99;
    }
    if ((pad_records[0].press & PDD_DGT_KL) |
        (pad_records[3].press & PDD_DGT_KL)) {
        sound_test_bank--;
    }
    if (sound_test_bank < 0) {
        sound_test_bank = TEST_VOICES;
    }
    if ((pad_records[0].press & PDD_DGT_KR) |
        (pad_records[3].press & PDD_DGT_KR)) {
        sound_test_bank++;
    }
    if (sound_test_bank > TEST_VOICES) {
        sound_test_bank = 0;
    }
    if ((pad_records[0].press & PDD_DGT_TY) |
        (pad_records[3].press & PDD_DGT_TY)) {
        if (sound_test_bank == TEST_VOICES) {
            stream_request_voice(sound_test_number + VOICE_FIRST);
        } else {
            sound_slot_play(sound_test_bank, sound_test_number, 0, SLOT_TEST);
            sound_slot_set_volume(64, SLOT_TEST);
        }
    }
    if ((pad_records[0].release & PDD_DGT_TY) |
        (pad_records[3].release & PDD_DGT_TY)) {
        sound_slot_stop(SLOT_TEST);
    }
}

void sound_load_voice(void)
{
    int character;

    character = get_character();
    /* @hack spelling: Sonic and Tails, who load the same file, in cases of
     * their own, which keep the name in R14 (0x8c09a6d2) and give Tails a
     * branch of his own (0x8c09a6f2); with one case, 37 of 40 words differ */
    switch (character) {
    case CHARACTER_SONIC:
        sound_load_file("v_sonictails.mlt");
        break;
    case CHARACTER_TAILS:
        sound_load_file("v_sonictails.mlt");
        break;
    case CHARACTER_KNUCKLES:
        sound_load_file("v_knuckles.mlt");
        break;
    case CHARACTER_AMY:
        sound_load_file("v_amy.mlt");
        break;
    case CHARACTER_E102:
        sound_load_file("v_e102.mlt");
        break;
    case CHARACTER_BIG:
        sound_load_file("v_big.mlt");
        break;
    default:
        sound_load_file("v_sonictails.mlt");
        break;
    }
}

void sound_fx_zone_step(struct task *task)
{
    struct task_work *work;
    NJS_ARGB color;
    Float scale;
    Float radius;
    Float dx;
    Float dy;
    Float dz;

    work = task->work;
    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        scale = work->scl.x * 0.1F;
        njScale(NULL, scale, scale, scale);
        draw_tree(&marker_object);
        njPopMatrix(1);
    } else if (edit_mode == 1) {
        color.a = -0.6F;
        color.r = color.g = color.b = 0.0F;
        njSetConstantMaterial(&color);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        scale = work->scl.x * 0.1F;
        njScale(NULL, scale, scale, scale);
        draw_tree(&marker_object);
        njPopMatrix(1);
        color.a = 0.0F;
        njSetConstantMaterial(&color);
    }
    dx = work->pos.x - camera_work->pos.x;
    dy = work->pos.y - camera_work->pos.y;
    dz = work->pos.z - camera_work->pos.z;
    radius = work->scl.x * work->scl.x;
    if (vector_length_squared(dx, dy, dz) < radius) {
        sound_request_fx(work->scl.y);
    }
}

void sound_orbit_step(struct task *task)
{
    struct task_work *work;
    NJS_ARGB color;
    Float x;
    Float z;

    work = task->work;
    work->ang.z += work->ang.y;
    x = njCos(work->ang.z) * work->scl.x;
    z = njSin(work->ang.z) * work->scl.x;
    sound_play_around(work->ang.x, (int)task, 1, work->scl.y, x + work->pos.x,
                      work->pos.y, z + work->pos.z, ORBIT_REACH, 2);
    if (placement_editing(task)) {
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        njScale(NULL, 1.0F, 1.0F, 1.0F);
        draw_tree(&marker_object);
        njPopMatrix(1);
    } else if (edit_mode == 1) {
        color.a = -0.6F;
        color.r = color.g = color.b = 0.0F;
        njSetConstantMaterial(&color);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        njScale(NULL, 1.0F, 1.0F, 1.0F);
        draw_tree(&marker_object);
        njPopMatrix(1);
        color.a = 0.0F;
        njSetConstantMaterial(&color);
    }
}

static void qsound_reset(void)
{
    int i;
    int j;

    sound_set_fx_23(0);
    for (i = 0; i < QSOUND_SES; i++) {
        for (j = 0; j < QSOUND_COPIES; j++) {
            qsound_owners[i][j] = 0;
        }
    }
    for (i = 0; i < QSOUND_CHANNELS; i++) {
        qsound_pos[i] = QSOUND_RESET_PLACE;
        qsound_sent[i] = 0;
    }
}

/* se's range in qsound_info, or -1 for one qsound doesn't place */
static int qsound_range(int se)
{
    int i;
    int found;

    found = -1;
    for (i = 0; i < QSOUND_SES; i++) {
        if (se < qsound_ses[i]) {
            break;
        }
        if (se == qsound_ses[i]) {
            found = i;
            break;
        }
    }
    if (found < 0) {
        return -1;
    }
    return qsound_info[found].range;
}

/* take a free copy of the slot's se, and set the slot's qsound to the
 * copy's channel. it returns what to add to se to play the copy. it returns
 * -1, and sets qsound to -1, when qsound doesn't place se or no copy is
 * free */
static int qsound_alloc(int slot)
{
    int i;
    int found;
    int copy;
    int channel;

    found = -1;
    copy = -1;
    for (i = 0; i < QSOUND_SES; i++) {
        if (sound_slots[slot].se == qsound_ses[i]) {
            found = i;
            break;
        }
    }
    if (found < 0) {
        sound_slots[slot].qsound = -1;
        return -1;
    }
    for (i = 0; i < qsound_info[found].copies; i++) {
        if (qsound_owners[found][i] == 0) {
            copy = i;
            break;
        }
    }
    if (copy < 0) {
        sound_slots[slot].qsound = -1;
        return -1;
    }
    qsound_owners[found][copy] = sound_slots[slot].id + 1;
    channel = qsound_info[found].channel + copy - 1;
    sound_slots[slot].qsound = channel;
    if (copy == 0) {
        return 0;
    }
    return qsound_info[found].next - qsound_ses[found] + copy - 1;
}

/* give back a copy of the slot's se that the slot's id holds */
static void qsound_free(int slot)
{
    int i;
    int found;
    int copy;

    found = -1;
    copy = -1;
    for (i = 0; i < QSOUND_SES; i++) {
        if (sound_slots[slot].se == qsound_ses[i]) {
            found = i;
            break;
        }
    }
    if (found >= 0) {
        for (i = 0; i < qsound_info[found].copies; i++) {
            if (qsound_owners[found][i] == sound_slots[slot].id + 1) {
                copy = i;
                break;
            }
        }
        if (copy >= 0) {
            qsound_owners[found][copy] = 0;
        }
    }
}

/* the slot's qsound place, 0 to 124 in steps of 4, by the side pos lies to
 * in the matrix of the fpu's back bank. a pos straight along -x is 0, and
 * one along +x 124 */
static int qsound_place(int slot)
{
    NJS_VECTOR way;
    /* @hack spelling: side and place as locals, which put the product in FR4
     * and the place in R4 before R0 (0x8c09aca2, 0x8c09acaa, 0x8c09acb2); in
     * one expression, they stay in FR2 and R0 and 9 of 42 words differ */
    Float side;
    int place;

    way.x = sound_slots[slot].pos.x;
    way.y = sound_slots[slot].pos.y;
    way.z = sound_slots[slot].pos.z;
    matrix_calc_point(&way);
    way.y = 0.0F;
    njUnitVector(&way);
    side = (way.x + 1.0F) * 15.5F;
    place = (int)side * 4;
    return place;
}

/* TRUE if se is one of the alt character's sounds while player_work[1] is
 * set. the alt character is battle_rival, and only Sonic, Knuckles and
 * E-102 have such sounds */
static Bool se_is_alt(int se)
{
    if (battle_rival < 0) {
        return FALSE;
    }
    if (player_work[1] != NULL) {
        switch (battle_rival) {
        case CHARACTER_SONIC:
            if ((se > 722 && se < 736) || (se > 1111 && se < 1144)) {
                return TRUE;
            }
            break;
        case CHARACTER_KNUCKLES:
            if ((se > 736 && se < 751) || (se > 1144 && se < 1168)) {
                return TRUE;
            }
            break;
        case CHARACTER_E102:
            if ((se > 776 && se < 795) || (se > 1186 && se < 1199)) {
                return TRUE;
            }
            break;
        }
    }
    return FALSE;
}

void sound_load_alt_voice(void)
{
    switch (battle_rival) {
    case CHARACTER_SONIC:
        sound_load_file("alt_sonictails.mlt");
        break;
    case CHARACTER_KNUCKLES:
        sound_load_file("alt_knuckles.mlt");
        break;
    case CHARACTER_E102:
        sound_load_file("alt_e102.mlt");
        break;
    }
}

void sound_load_alt_field(void)
{
    int stage;

    stage = stage_get_stage_act() >> 8;
    switch (stage) {
    case STAGE_EGG_CARRIER:
    case STAGE_EGG_CARRIER_2:
    case STAGE_EGG_CARRIER_3:
    case STAGE_EGG_CARRIER_INSIDE:
        sound_load_file("alt_eggcarrier.mlt");
        break;
    case STAGE_MYSTIC_RUINS:
    case STAGE_PAST:
        sound_load_file("alt_mysticruin.mlt");
        break;
    }
}

void sound_play_bank(Uint8 bank, Uint8 number)
{
    int voice;

    if (bank == BANK_MUSIC) {
        stream_request_music(number);
    } else if (bank >= BANK_VOICES) {
        bank -= BANK_VOICES;
        voice = ((bank & 0xf0) << 4) | number;
        stream_request_voice(voice);
    } else {
        sound_slot_play(bank, number, 0, SLOT_TEST);
        sound_slot_set_volume(127, SLOT_TEST);
    }
}
