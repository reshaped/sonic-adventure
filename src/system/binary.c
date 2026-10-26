/* NJB model and motion banks, and PVB texture banks */
#include <ninja.h>
#include <sg_xpt.h>

#include "system/binary.h"
#include "system/task.h"

struct njb_header {
    char magic[3]; /* the letters NJB, unchecked */
    Sint8 version;
    Sint32 count; /* unread: njb_relocate stops at NJB_END */
};

/* a block's head. a bank is a run of top-level blocks, up to one of kind
 * NJB_END. each top-level block's data is a run of blocks in turn, and the
 * offsets in that run count from its first byte */
struct njb_block {
    Uint16 kind;
    Uint16 count; /* unread: the relocations run to the next block */
    Sint32 size; /* the bytes after this head */
};

enum njb_kind {
    NJB_END = 0,
    NJB_MODEL_RUN = 1,
    NJB_MOTION_RUN = 3,
    NJB_ACTION_RUN = 4
};

/* the kinds of block in a model run. strips, points, normals, uvs and
 * materials hold no offsets */
enum njb_model_kind {
    NJB_STRIPS,
    NJB_POINTS,
    NJB_NORMALS,
    NJB_UVS,
    NJB_MATERIALS,
    NJB_MESHSETS,
    NJB_MODELS,
    NJB_OBJECTS
};

/* the kinds of block in a motion run or an action run. the keys hold no
 * offsets, and a motion run's actions stay as they are */
enum njb_motion_kind {
    NJB_POS_KEYS,
    NJB_ANG_KEYS,
    NJB_ACTIONS,
    NJB_MDATA, /* each motion's list of motion data, an entry for each node */
    NJB_MOTIONS
};

/* the block after block: past its head and its size bytes, rounded up to a
 * multiple of 4 */
#define NJB_NEXT(block)                                                        \
    ((struct njb_block *)(((int)(block) + (int)sizeof(struct njb_block) +      \
                           (block)->size + 3) &                                \
                          ~3))

/* a motion data list's head: two words, its entries' length and its factor
 * count */
#define MDATA_HEAD 8

/* make field, an offset from base, a pointer. an offset of 0 stays NULL */
#define RELOCATE(field, base)                                                  \
    do {                                                                       \
        if ((field) != NULL) {                                                 \
            (field) = (void *)((base) + (int)(field));                         \
        }                                                                      \
    } while (0)

struct pvb_header {
    char magic[3]; /* the letters PVB, unchecked */
    Sint8 version;
    Uint16 count; /* the pvb_texture records that follow */
    Uint8 unk_06[2];
};

struct pvb_texture {
    Uint32 offset; /* the pixels', in bytes from the bank's start */
    Sint32 type;
    Uint32 global_index;
    Sint16 width;
    Sint16 height;
};

static void njb_relocate_models(struct njb_block *block, struct njb_block *end);
static void relocate_meshsets(Uint8 *base, NJS_MESHSET *first,
                              NJS_MESHSET *end);
static void relocate_models(Uint8 *base, NJS_MODEL *first, NJS_MODEL *end);
static void relocate_objects(Uint8 *base, NJS_OBJECT *first, NJS_OBJECT *end);
static NJS_MDATA1 *relocate_mdata1(Uint8 *base, NJS_MDATA1 *mdata, int words);
static NJS_MDATA2 *relocate_mdata2(Uint8 *base, NJS_MDATA2 *mdata, int words);
static NJS_MDATA3 *relocate_mdata3(Uint8 *base, NJS_MDATA3 *mdata, int words);
static NJS_MDATA4 *relocate_mdata4(Uint8 *base, NJS_MDATA4 *mdata, int words);
static void relocate_mdata(Uint8 *base, int *list, const int *end);
static void relocate_motions(Uint8 *base, NJS_MOTION *first, NJS_MOTION *end);
static void njb_relocate_motions(struct njb_block *block,
                                 struct njb_block *end);
static void njb_relocate_actions(struct njb_block *block,
                                 struct njb_block *end);
static void relocate_actions(Uint8 *base, NJS_ACTION *first, NJS_ACTION *end);

Bool njb_relocate(void *file)
{
    struct njb_header *header;
    struct njb_block *block;
    struct njb_block *next;

    header = file;
    block = (struct njb_block *)(header + 1);
    switch (header->version) {
    case 4:
        break;
    /* @hack redundant: case labels for versions 1 to 3, which share the
     * default's return and keep the image's CMP/EQ #1, #2 and #3 (0x8c0a4f34 to
     * 0x8c0a4f3c); without them, 53 of 80 words differ */
    case 1:
    case 2:
    case 3:
    default:
        return FALSE;
    }
    for (; block->kind != NJB_END; block = next) {
        next = NJB_NEXT(block);
        switch (block->kind) {
        case NJB_MODEL_RUN:
            njb_relocate_models(block + 1, next);
            break;
        case NJB_MOTION_RUN:
            njb_relocate_motions(block + 1, next);
            break;
        case NJB_ACTION_RUN:
            njb_relocate_actions(block + 1, next);
            break;
        }
    }
    return TRUE;
}

static void pvb_clear(struct pvb_texlist *textures)
{
    textures->texlist.textures = NULL;
    textures->texlist.nbTexture = 0;
    textures->info = NULL;
    textures->names = NULL;
}

/* a record from the heap with an empty texture list and no arrays, or null
 * when the heap has no room. pvb_load leaves the result unchecked */
static struct pvb_texlist *pvb_alloc(void)
{
    struct pvb_texlist *textures;

    textures = heap_alloc(sizeof(struct pvb_texlist));
    if (textures != NULL) {
        pvb_clear(textures);
    }
    return textures;
}

static void pvb_free_arrays(struct pvb_texlist *textures)
{
    if (textures->info != NULL) {
        heap_free(textures->info);
    }
    if (textures->names != NULL) {
        heap_free(textures->names);
    }
}

void pvb_release(struct pvb_texlist *textures)
{
    struct pvb_texlist *held;

    if (textures != NULL) {
        njReleaseTexture(&textures->texlist);
        /* @hack spelling: the frees read the record through held, a copy of
         * textures in a frame slot of its own (MOV.L R2,@R15 at 0x8c0a50d6);
         * through textures, 33 of 38 words differ */
        held = textures;
        if (held->info != NULL) {
            heap_free(held->info);
        }
        if (held->names != NULL) {
            heap_free(held->names);
        }
        heap_free(held);
    }
}

struct pvb_texlist *pvb_load(void *file)
{
    struct pvb_header *header;
    struct pvb_texture *texture;
    int i;
    int count;
    struct pvb_texlist *textures;
    Uint16 *pixels;
    /* @hack redundant: 4 bytes of frame at @R15 that no code reads, of a type
     * unknown, declared as an int. the image's ADD #-32,R15 reserves them
     * (0x8c0a5144); without them, 47 of 146 words differ */
    int unused;

    header = file;
    texture = (struct pvb_texture *)(header + 1);
    textures = pvb_alloc();
    count = header->count;
    /* @hack spelling: info assigned in the test, which tests heap_alloc's R0
     * (TST R0,R0 at 0x8c0a5176); assigned apart, SHC reloads textures->info
     * to test it, and 15 of 146 words differ */
    if ((textures->info = heap_alloc(count * sizeof(NJS_TEXINFO))) == NULL) {
        pvb_free_arrays(textures);
        heap_free(textures);
        return NULL;
    }
    /* @hack spelling: names assigned in the test, which tests heap_alloc's R0
     * (TST R0,R0 at 0x8c0a51d8); assigned apart, 26 of 146 words differ */
    if ((textures->names = heap_alloc(count * sizeof(NJS_TEXNAME))) == NULL) {
        pvb_free_arrays(textures);
        heap_free(textures);
        return NULL;
    }
    textures->texlist.textures = textures->names;
    textures->texlist.nbTexture = count;
    for (i = 0; i < count; i++) {
        /* @hack spelling: the offset plus file, which loads file (MOV.L
         * @(28,R15),R3 at 0x8c0a5242) before the offset (MOV.L @R1,R2 at
         * 0x8c0a5244); written file + offset, 2 of 146 words differ */
        pixels = (Uint16 *)(texture->offset + (int)file);
        njSetTextureInfo(&textures->info[i], pixels, texture->type,
                         texture->width, texture->height);
        njSetTextureName(&textures->names[i], &textures->info[i],
                         texture->global_index,
                         NJD_TEXATTR_TYPE_MEMORY | NJD_TEXATTR_GLOBALINDEX);
        texture++;
    }
    njLoadTexture(&textures->texlist);
    return textures;
}

static void njb_relocate_models(struct njb_block *block, struct njb_block *end)
{
    Uint8 *base;
    struct njb_block *next;

    base = (Uint8 *)block;
    do {
        next = NJB_NEXT(block);
        switch (block->kind) {
        /* @hack redundant: cases for the kinds that hold no offsets, which keep
         * the image's CMP/EQ #0 to #4 (0x8c0a532c to 0x8c0a533c); without
         * them, the function runs 24 bytes short and 52 of 71 words differ */
        case NJB_STRIPS:
        case NJB_POINTS:
        case NJB_NORMALS:
        case NJB_UVS:
        case NJB_MATERIALS:
            break;
        case NJB_MESHSETS:
            relocate_meshsets(base, (NJS_MESHSET *)(block + 1),
                              (NJS_MESHSET *)next);
            break;
        case NJB_MODELS:
            relocate_models(base, (NJS_MODEL *)(block + 1), (NJS_MODEL *)next);
            break;
        case NJB_OBJECTS:
            relocate_objects(base, (NJS_OBJECT *)(block + 1),
                             (NJS_OBJECT *)next);
            break;
        }
        block = next;
    } while (block < end);
}

static void relocate_meshsets(Uint8 *base, NJS_MESHSET *first, NJS_MESHSET *end)
{
    NJS_MESHSET *meshset;

    for (meshset = first; meshset < end; meshset++) {
        RELOCATE(meshset->meshes, base);
        RELOCATE(meshset->attrs, base);
        RELOCATE(meshset->normals, base);
        RELOCATE(meshset->vertcolor, base);
        RELOCATE(meshset->vertuv, base);
    }
}

static void relocate_models(Uint8 *base, NJS_MODEL *first, NJS_MODEL *end)
{
    NJS_MODEL *model;

    for (model = first; model < end; model++) {
        RELOCATE(model->points, base);
        RELOCATE(model->normals, base);
        RELOCATE(model->meshsets, base);
        RELOCATE(model->mats, base);
    }
}

static void relocate_objects(Uint8 *base, NJS_OBJECT *first, NJS_OBJECT *end)
{
    NJS_OBJECT *object;

    for (object = first; object < end; object++) {
        RELOCATE(object->model, base);
        RELOCATE(object->child, base);
        RELOCATE(object->sibling, base);
    }
}

/* relocate the entries of one motion data list, and return the list's end.
 * words counts each entry's key pointers and its key counts, which hold no
 * offsets. relocate_mdata2 to relocate_mdata4 step words past the key
 * counts in an empty loop */
static NJS_MDATA1 *relocate_mdata1(Uint8 *base, NJS_MDATA1 *mdata, int words)
{
    for (; words > 0; words -= 2, mdata++) {
        RELOCATE(mdata->p[0], base);
    }
    return mdata;
}

static NJS_MDATA2 *relocate_mdata2(Uint8 *base, NJS_MDATA2 *mdata, int words)
{
    int i;

    for (; words > 0; mdata++) {
        for (i = 0; i < 2; i++, words--) {
            RELOCATE(mdata->p[i], base);
        }
        for (i = 0; i < 2; i++, words--) {
        }
    }
    return mdata;
}

static NJS_MDATA3 *relocate_mdata3(Uint8 *base, NJS_MDATA3 *mdata, int words)
{
    int i;

    for (; words > 0; mdata++) {
        for (i = 0; i < 3; i++, words--) {
            RELOCATE(mdata->p[i], base);
        }
        for (i = 0; i < 3; i++, words--) {
        }
    }
    return mdata;
}

static NJS_MDATA4 *relocate_mdata4(Uint8 *base, NJS_MDATA4 *mdata, int words)
{
    int i;

    for (; words > 0; mdata++) {
        for (i = 0; i < 4; i++, words--) {
            RELOCATE(mdata->p[i], base);
        }
        for (i = 0; i < 4; i++, words--) {
        }
    }
    return mdata;
}

/* relocate each motion data list from list up to end. a list's head is its
 * entries' length in words, then its factor count. the count, 1 to 4, picks
 * their type, NJS_MDATA1 to NJS_MDATA4 */
static void relocate_mdata(Uint8 *base, int *list, const int *end)
{
    int *walk;
    int words;

    for (walk = list; walk < end;) {
        words = *walk++;
        switch (*walk++) {
        case 1:
            walk = (int *)relocate_mdata1(base, (NJS_MDATA1 *)walk, words);
            break;
        case 2:
            walk = (int *)relocate_mdata2(base, (NJS_MDATA2 *)walk, words);
            break;
        case 3:
            walk = (int *)relocate_mdata3(base, (NJS_MDATA3 *)walk, words);
            break;
        case 4:
            walk = (int *)relocate_mdata4(base, (NJS_MDATA4 *)walk, words);
            break;
        }
    }
}

/* a motion's mdata offset points at its list's head, so its data starts
 * MDATA_HEAD bytes on */
static void relocate_motions(Uint8 *base, NJS_MOTION *first, NJS_MOTION *end)
{
    NJS_MOTION *motion;

    for (motion = first; motion < end; motion++) {
        if (motion->mdata != NULL) {
            motion->mdata = (void *)(base + (int)motion->mdata + MDATA_HEAD);
        }
    }
}

static void njb_relocate_motions(struct njb_block *block, struct njb_block *end)
{
    Uint8 *base;
    struct njb_block *next;

    base = (Uint8 *)block;
    do {
        next = NJB_NEXT(block);
        switch (block->kind) {
        /* @hack redundant: cases for the kinds left as they are, which keep the
         * image's CMP/EQ #0, #1 and #2 (0x8c0a579c to 0x8c0a57a4); without
         * them, the function runs 16 bytes short and 32 of 57 words differ */
        case NJB_POS_KEYS:
        case NJB_ANG_KEYS:
        case NJB_ACTIONS:
            break;
        case NJB_MDATA:
            relocate_mdata(base, (int *)(block + 1), (int *)next);
            break;
        case NJB_MOTIONS:
            relocate_motions(base, (NJS_MOTION *)(block + 1),
                             (NJS_MOTION *)next);
            break;
        }
        block = next;
    } while (block < end);
}

static void njb_relocate_actions(struct njb_block *block, struct njb_block *end)
{
    Uint8 *base;
    struct njb_block *next;

    base = (Uint8 *)block;
    do {
        next = NJB_NEXT(block);
        /* @hack spelling: the keys' kinds take a case each, and each case its
         * own BRA (0x8c0a5826, 0x8c0a582a); as one case, the function runs 4
         * bytes short and 39 of 67 words differ */
        switch (block->kind) {
        case NJB_POS_KEYS:
            break;
        case NJB_ANG_KEYS:
            break;
        case NJB_ACTIONS:
            relocate_actions(base, (NJS_ACTION *)(block + 1),
                             (NJS_ACTION *)next);
            break;
        case NJB_MDATA:
            relocate_mdata(base, (int *)(block + 1), (int *)next);
            break;
        case NJB_MOTIONS:
            relocate_motions(base, (NJS_MOTION *)(block + 1),
                             (NJS_MOTION *)next);
            break;
        }
        block = next;
    } while (block < end);
}

/* relocate each action's motion. its object stays an offset, for the
 * caller to set */
static void relocate_actions(Uint8 *base, NJS_ACTION *first, NJS_ACTION *end)
{
    NJS_ACTION *action;

    for (action = first; action < end; action++) {
        RELOCATE(action->motion, base);
    }
}
