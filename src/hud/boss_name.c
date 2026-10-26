/* the boss's name, typed a letter at a time as a fight starts */
#include <ninja.h>
#include <sg_xpt.h>
#include <string.h>

#include "draw/draw.h"
#include "hud/boss_name.h"
#include "main.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"

#define TEXT_1_LENGTH 12
#include "hud/boss_name/text.inc"

/* b32ascii's letters are 32-pixel squares, numbered across rows of eight */
#define LETTER_SIZE 32
#define ROW_SHIFT 3
#define LETTERS_PER_ROW (1 << ROW_SHIFT)
/* the square of anything b32ascii has no letter for */
#define LETTER_NONE 63

/* the steps from one letter to the next, the steps the whole name holds,
 * and the steps its fade takes */
#define LETTER_STEPS 20
#define HOLD_STEPS 120
#define FADE_STEPS 30

/* the click of a letter, in each boss stage's own sounds */
#define SE_LETTER_CHAOS0 331
#define SE_LETTER_CHAOS4 355
#define SE_LETTER_CHAOS6 378
#define SE_LETTER_CHAOS7 409
#define SE_LETTER_CHAOS7_2 428 /* Chaos 7's second act */
#define SE_LETTER_MOUNTAIN 151
#define SE_LETTER_EGGMOBILE1 454
#define SE_LETTER_EGGMOBILE2 468
#define SE_LETTER_EGGMOBILE3 487

enum boss_name_mode {
    BOSS_NAME_INIT,
    BOSS_NAME_TYPE, /* a letter every LETTER_STEPS */
    BOSS_NAME_WHOLE,
    BOSS_NAME_HOLD,
    BOSS_NAME_FADE
};

struct boss_name_vars {
    Uint8 unk_00[4];
    Sint32 steps; /* to the next letter, or the end of the hold */
};

#define VARS(work) ((struct boss_name_vars *)&(work)->vars)

/* the name task's extra. the fade's end frees text and letters. the task
 * has no destructor, so they leak if it is destroyed sooner */
struct boss_name {
    char *text;
    Sint8 length;
    Sint8 shown; /* letters typed so far */
    NJS_TEXANIM *letters; /* each one's square in b32ascii */
};

static NJS_TEXNAME b32ascii_textures[] = {
    {TEXT_1, NJD_TEXATTR_GLOBALINDEX, 400999}};
NJS_TEXLIST b32ascii_texlist = {
#include "hud/boss_name/b32ascii_texlist.inc"
};
static NJS_SPRITE boss_name_sprite = {
#include "hud/boss_name/boss_name_sprite.inc"
};
static NJS_ARGB boss_name_color = {1.0F, 1.0F, 1.0F, 1.0F};

/* the stage and act the fight is in, for its sounds */
static Uint16 boss_name_stage;
static Uint16 boss_name_act;

/* type the name, hold it whole, then fade it out and kill the task. each
 * step but the last draws the letters typed so far */
static void boss_name_step(struct task *task)
{
    struct task_work *work = task->work;
    struct boss_name *name = task->extra;
    NJS_TEXANIM *letter;
    int attr;
    int i;

    switch (work->mode) {
    case BOSS_NAME_INIT:
        boss_name_stage = stage_get_stage_act();
        boss_name_act = (Uint8)boss_name_stage;
        boss_name_stage >>= 8;
        work->mode = BOSS_NAME_TYPE;
        /* fall through */
    case BOSS_NAME_TYPE:
        VARS(work)->steps--;
        if (VARS(work)->steps == 0) {
            name->shown++;
            /* @hack spelling: the letter just shown, through a pointer sum,
             * which the image adds before its MOV.B (0x8c0fcf34,
             * 0x8c0fcf38); as name->text[name->shown - 1], SHC indexes with
             * @(R0,R3), an instruction shorter, and 38 of 270 words
             * differ */
            if (*(name->text + name->shown - 1) != ' ') {
                switch (boss_name_stage) {
                case STAGE_CHAOS0:
                case STAGE_CHAOS2:
                    sound_play(SE_LETTER_CHAOS0, 0, 0, 0);
                    break;
                case STAGE_CHAOS4:
                    sound_play(SE_LETTER_CHAOS4, 0, 0, 0);
                    break;
                case STAGE_CHAOS6:
                    sound_play(SE_LETTER_CHAOS6, 0, 0, 0);
                    break;
                case STAGE_CHAOS7:
                    if (boss_name_act == 0) {
                        sound_play(SE_LETTER_CHAOS7, 0, 0, 0);
                    } else {
                        sound_play(SE_LETTER_CHAOS7_2, 0, 0, 0);
                    }
                    break;
                case STAGE_MOUNTAIN:
                    sound_play(SE_LETTER_MOUNTAIN, 0, 0, 0);
                    break;
                case STAGE_EGGMOBILE1:
                    sound_play(SE_LETTER_EGGMOBILE1, 0, 0, 0);
                    break;
                case STAGE_EGGMOBILE2:
                    sound_play(SE_LETTER_EGGMOBILE2, 0, 0, 0);
                    break;
                case STAGE_EGGMOBILE3:
                    sound_play(SE_LETTER_EGGMOBILE3, 0, 0, 0);
                    break;
                }
            }
            if (name->shown < name->length) {
                VARS(work)->steps = LETTER_STEPS;
            } else {
                boss_name_color.a = 1.0F;
                work->mode = BOSS_NAME_WHOLE;
            }
        }
        break;
    case BOSS_NAME_WHOLE:
        VARS(work)->steps = HOLD_STEPS;
        work->mode = BOSS_NAME_HOLD;
        break;
    case BOSS_NAME_HOLD:
        VARS(work)->steps--;
        if (VARS(work)->steps == 0) {
            work->mode = BOSS_NAME_FADE;
        }
        break;
    case BOSS_NAME_FADE:
        /* @hack spelling: the fade through boss_name_color's address, which it
         * loads into R1 at 0x8c0fd09c; as boss_name_color.a, SHC keeps the
         * address in R10 from the start, the function runs 28 bytes long and
         * 258 of 270 words differ */
        (&boss_name_color)->a -= 1.0F / FADE_STEPS;
        if (boss_name_color.a <= 0.0F) {
            heap_free(name->text);
            heap_free(name->letters);
            task_kill(task);
            return;
        }
        break;
    }
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njSetTexture(&b32ascii_texlist);
    njSetConstantMaterial(&boss_name_color);
    boss_name_sprite.p.x = work->pos.x;
    boss_name_sprite.p.y = work->pos.y;
    attr = NJD_SPRITE_ALPHA;
    if (work->mode >= BOSS_NAME_FADE) {
        attr = NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR;
    }
    letter = name->letters;
    for (i = 0; i < name->shown; i++, letter++) {
        boss_name_sprite.tanim = letter;
        if (name->text[i] != ' ') {
            njDrawSprite2D(&boss_name_sprite, 0, 0.0F, attr);
        }
        boss_name_sprite.p.x += letter->sx;
    }
}

void boss_name_create(const char *text, int x, int y, int delay)
{
    struct task *task;
    struct boss_name *name;
    NJS_TEXANIM *letter;
    int i;
    Sint8 square;
    int left;
    int top;

    task = task_create(TASK_WORK, 1, boss_name_step);
    task->extra = heap_calloc(1, sizeof(struct boss_name));
    VARS(task->work)->steps = delay + 1;
    name = task->extra;
    name->length = strlen(text);
    name->text = heap_alloc(name->length + 1);
    strcpy(name->text, text);
    name->letters = heap_calloc(name->length, sizeof(NJS_TEXANIM));
    letter = name->letters;
    for (i = 0; i < name->length; i++, letter++, text++) {
        letter->sx = LETTER_SIZE;
        letter->sy = LETTER_SIZE;
        letter->cx = 0;
        letter->cy = 0;
        if (*text >= '0' && *text <= '9') {
            square = *text - '0';
        } else if (*text >= 'A' && *text <= 'Z') {
            square = *text - 'A' + 10;
        } else if (*text >= 'a' && *text <= '|') {
            square = *text - 'a' + 36;
        } else {
            square = LETTER_NONE;
        }
        left = (square & (LETTERS_PER_ROW - 1)) * LETTER_SIZE;
        letter->u1 = left;
        top = (square >> ROW_SHIFT) * LETTER_SIZE;
        letter->v1 = top;
        letter->u2 = left + LETTER_SIZE - 1;
        letter->v2 = top + LETTER_SIZE - 1;
    }
    if (x >= 0) {
        task->work->pos.x = x;
    } else {
        /* a negative x centers the name. its width is a multiple of 32, so
         * the integer division loses nothing */
        /* NOLINTNEXTLINE(bugprone-integer-division) */
        task->work->pos.x = SCREEN_MIDDLE_X - (name->length * LETTER_SIZE / 2);
    }
    task->work->pos.y = y;
}
