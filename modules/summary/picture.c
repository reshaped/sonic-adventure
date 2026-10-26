/* the summary's pictures, one for each story, and the tasks that draw
 * one */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/screen.h"
#include "summary/picture.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 10
#define TEXT_9_LENGTH 10
#define TEXT_10_LENGTH 10
#define TEXT_11_LENGTH 10
#define TEXT_12_LENGTH 10
#define TEXT_13_LENGTH 10
#define TEXT_14_LENGTH 10
#define TEXT_15_LENGTH 10
#define TEXT_16_LENGTH 10
#define TEXT_17_LENGTH 13
#define TEXT_18_LENGTH 13
#define TEXT_19_LENGTH 13
#define TEXT_20_LENGTH 13
#define TEXT_21_LENGTH 13
#define TEXT_22_LENGTH 13
#define TEXT_23_LENGTH 13
#define TEXT_24_LENGTH 13
#define TEXT_25_LENGTH 8
#define TEXT_26_LENGTH 8
#define TEXT_27_LENGTH 8
#define TEXT_28_LENGTH 8
#define TEXT_29_LENGTH 8
#define TEXT_30_LENGTH 8
#define TEXT_31_LENGTH 8
#define TEXT_32_LENGTH 8
#define TEXT_33_LENGTH 9
#define TEXT_34_LENGTH 9
#define TEXT_35_LENGTH 9
#define TEXT_36_LENGTH 9
#define TEXT_37_LENGTH 9
#define TEXT_38_LENGTH 9
#define TEXT_39_LENGTH 9
#define TEXT_40_LENGTH 9
#define TEXT_41_LENGTH 8
#define TEXT_42_LENGTH 8
#define TEXT_43_LENGTH 8
#define TEXT_44_LENGTH 8
#define TEXT_45_LENGTH 8
#define TEXT_46_LENGTH 8
#define TEXT_47_LENGTH 8
#define TEXT_48_LENGTH 8
#define TEXT_49_LENGTH 15
#define TEXT_50_LENGTH 15
#define TEXT_51_LENGTH 15
#define TEXT_52_LENGTH 15
#define TEXT_53_LENGTH 15
#define TEXT_54_LENGTH 15
#define TEXT_55_LENGTH 15
#define TEXT_56_LENGTH 15
#include "summary/picture/text.inc"

/* the stories with a picture: the six characters', then Super Sonic's */
#define STORIES 7

/* the picture a task draws, which its extra holds */
struct picture {
    char *file;
    NJS_TEXLIST *texlist;
    struct picture_parts *parts;
    Uint32 color;
};

static char *picture_files[STORIES] = {
    "smrybg_sonic", "smrybg_tails", "smrybg_knuckles",   "smrybg_amy",
    "smrybg_big",   "smrybg_e102",  "smrybg_supersonic",
};
static NJS_TEXNAME sonic_picture_textures[8] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};
static NJS_TEXLIST sonic_picture_texlist = {
#include "summary/picture/sonic_picture_texlist.inc"
};
static NJS_TEXNAME tails_picture_textures[8] = {
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
};
static NJS_TEXLIST tails_picture_texlist = {
#include "summary/picture/tails_picture_texlist.inc"
};
static NJS_TEXNAME knuckles_picture_textures[8] = {
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
};
static NJS_TEXLIST knuckles_picture_texlist = {
#include "summary/picture/knuckles_picture_texlist.inc"
};
static NJS_TEXNAME amy_picture_textures[8] = {
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
};
static NJS_TEXLIST amy_picture_texlist = {
#include "summary/picture/amy_picture_texlist.inc"
};
static NJS_TEXNAME e102_picture_textures[8] = {
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0},
    {TEXT_37, 0, 0}, {TEXT_38, 0, 0}, {TEXT_39, 0, 0}, {TEXT_40, 0, 0},
};
static NJS_TEXLIST e102_picture_texlist = {
#include "summary/picture/e102_picture_texlist.inc"
};
static NJS_TEXNAME big_picture_textures[8] = {
    {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_43, 0, 0}, {TEXT_44, 0, 0},
    {TEXT_45, 0, 0}, {TEXT_46, 0, 0}, {TEXT_47, 0, 0}, {TEXT_48, 0, 0},
};
static NJS_TEXLIST big_picture_texlist = {
#include "summary/picture/big_picture_texlist.inc"
};
static NJS_TEXNAME supersonic_picture_textures[8] = {
    {TEXT_49, 0, 0}, {TEXT_50, 0, 0}, {TEXT_51, 0, 0}, {TEXT_52, 0, 0},
    {TEXT_53, 0, 0}, {TEXT_54, 0, 0}, {TEXT_55, 0, 0}, {TEXT_56, 0, 0},
};
static NJS_TEXLIST supersonic_picture_texlist = {
#include "summary/picture/supersonic_picture_texlist.inc"
};
static NJS_TEXLIST *picture_texlists[STORIES] = {
    &sonic_picture_texlist,      &tails_picture_texlist,
    &knuckles_picture_texlist,   &amy_picture_texlist,
    &big_picture_texlist,        &e102_picture_texlist,
    &supersonic_picture_texlist,
};
static struct picture_parts sonic_picture_parts = {
    8,
    {{0, 0, 0},
     {1, 0, 256},
     {2, 256, 0},
     {3, 256, 256},
     {4, 512, 0},
     {5, 512, 128},
     {6, 512, 256},
     {7, 512, 384}},
};
static struct picture_parts tails_picture_parts = {
    8,
    {{0, 0, 0},
     {1, 0, 256},
     {2, 256, 0},
     {3, 256, 256},
     {4, 512, 0},
     {5, 512, 128},
     {6, 512, 256},
     {7, 512, 384}},
};
static struct picture_parts knuckles_picture_parts = {
    8,
    {{0, 0, 0},
     {1, 0, 256},
     {2, 256, 0},
     {3, 256, 256},
     {4, 512, 0},
     {5, 512, 128},
     {6, 512, 256},
     {7, 512, 384}},
};
static struct picture_parts amy_picture_parts = {
    8,
    {{0, 0, 0},
     {1, 0, 256},
     {2, 256, 0},
     {3, 256, 256},
     {4, 512, 0},
     {5, 512, 128},
     {6, 512, 256},
     {7, 512, 384}},
};
static struct picture_parts e102_picture_parts = {
    8,
    {{0, 0, 0},
     {1, 0, 256},
     {2, 256, 0},
     {3, 256, 256},
     {4, 512, 0},
     {5, 512, 128},
     {6, 512, 256},
     {7, 512, 384}},
};
static struct picture_parts big_picture_parts = {
    8,
    {{0, 0, 0},
     {1, 0, 256},
     {2, 256, 0},
     {3, 256, 256},
     {4, 512, 0},
     {5, 512, 128},
     {6, 512, 256},
     {7, 512, 384}},
};
static struct picture_parts supersonic_picture_parts = {
    8,
    {{0, 0, 0},
     {1, 0, 256},
     {2, 256, 0},
     {3, 256, 256},
     {4, 512, 0},
     {5, 512, 128},
     {6, 512, 256},
     {7, 512, 384}},
};
static struct picture_parts *picture_parts_table[STORIES] = {
    &sonic_picture_parts,      &tails_picture_parts, &knuckles_picture_parts,
    &amy_picture_parts,        &big_picture_parts,   &e102_picture_parts,
    &supersonic_picture_parts,
};
static Uint32 picture_colors[STORIES] = {
    0xff8080ff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff,
};

/* the picture picture_load loaded, which picture_draw draws */
static NJS_TEXLIST *picture_texlist;
static struct picture_parts *picture_parts;
static Uint32 picture_color;

static void picture_task_draw(struct task *task);
static void picture_task_destroy(struct task *task);

NJS_TEXLIST *picture_load(int story)
{
    char *file;
    NJS_TEXLIST *texlist;

    picture_texlist = NULL;
    picture_parts = NULL;
    picture_color = 0xffffffff;
    file = picture_files[story];
    /* @hack spelling: the null tests nested, each with an else, which put the
     * return of NULL after the loading (MOV #0,R0 at 0x8c900924); as early
     * returns, it follows the second test, and 26 of 40 words differ */
    if (file) {
        texlist = picture_texlists[story];
        if (texlist) {
            picture_texlist = texlist;
            texture_load_pvm(file, texlist);
            picture_parts = picture_parts_table[story];
            picture_color = picture_colors[story];
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
    return picture_texlist;
}

void picture_release(void)
{
    if (picture_texlist) {
        njReleaseTexture(picture_texlist);
    }
}

void picture_draw(Float x, Float y, Float z)
{
    screen_quad_tint(0xffffffff);
    screen_blend_alpha();
    njSetTexture(picture_texlist);
    screen_quad_color(picture_color);
    screen_parts_draw((struct screen_parts *)picture_parts, x, y, z);
    screen_quad_tint(0xffffffff);
}

static void picture_task_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct picture *picture = task->extra;

    if (work && picture) {
        screen_quad_tint(0xffffffff);
        screen_blend_alpha();
        screen_quad_color(picture->color);
        njSetTexture(picture->texlist);
        screen_parts_draw((struct screen_parts *)picture->parts, work->pos.x,
                          work->pos.y, work->pos.z);
    }
}

static void picture_task_destroy(struct task *task)
{
    struct picture *picture = task->extra;

    if (picture->texlist) {
        njReleaseTexture(picture->texlist);
        picture->texlist = NULL;
    }
}

struct task *picture_create(int level, int story)
{
    struct task *task;
    struct task_work *work;
    struct picture *picture;

    task = task_create(TASK_WORK, level, picture_task_draw);
    if (task == NULL) {
        return NULL;
    }
    work = task->work;
    work->pos.x = 0.0F;
    work->pos.y = 0.0F;
    work->pos.z = 65536.0F;
    task->extra = heap_calloc(1, sizeof(struct picture));
    picture = task->extra;
    picture->file = picture_files[story];
    picture->texlist = picture_texlists[story];
    picture->parts = picture_parts_table[story];
    picture->color = 0xffffffff;
    if (picture->file && picture->texlist) {
        texture_load_pvm(picture->file, picture->texlist);
    }
    task->destructor = picture_task_destroy;
    return task;
}
