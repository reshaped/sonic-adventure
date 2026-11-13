/* what a draw is, as sa_capture.h's records. each record takes Ninja's state
 * and the draw's own arguments as the call begins. so the game's own store
 * into Ninja's state between two draws counts as a call to its setter. a mesh
 * is its model's bytes at the call. a model changed later, or one whose uvs
 * lie on a stack, keeps the mesh of this call.
 *
 * the math here only says where a draw lies and what it looks like. no play
 * reads it. it runs under the rounding the game's own C runs under */
#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "collision/world.h"
#include "live.h"
#include "native.h"
#include "ninja/njkamui.h"
#include "play_math.h"
#include "sa1.h"

/* what the records read of the game's task, in the host's layout */
struct task {
    struct task *next, *prev, *parent, *child;
    void *step, *draw, *destructor, *placement;
    void *work, *motion, *force, *extra;
};

extern int frame_step;
extern Bool draw_off;
extern int16_t stage_current;
extern uint32_t play_time;
extern struct nj_parameterkm _nj_parameterkm_;
extern NJS_MATRIX _nj_view_matrix_;
extern float _nj_far_clip_;

/* a material's flip flags, which ninja.h leaves unnamed */
#define FLIP_U 0x00040000U
#define FLIP_V 0x00020000U

/* the PVR registers a picture's fog reads, which Ninja's setters write */
#define FOG_COLOR 0xa05f80b0UL
#define FOG_DENSITY 0xa05f80b8UL
#define FOG_TABLE 0xa05f8200UL
#define REGISTER(address) (*(volatile uint32_t *)(NATIVE_DOMAIN + (address)))

/* the records of a picture and of what its draws first name, in order.
 * records_refused counts the records left out as larger than a record may be.
 * a mesh or list left out takes no name. each draw or scene part that needs it
 * counts it again */
static uint8_t *records;
static size_t records_used, records_room, records_most;
static uint32_t records_refused;
static FILE *capture;
static int recording;

/* the bytes that follow an sa_cap_mesh in its record: a model as
 * SA_MESH_NINJA_INLINE lays it out. MESH_SETS is the most meshsets they hold,
 * past the model's 40 bytes at 24 each */
#define MESH_ROOM (SA_CAP_RECORD_MOST - sizeof(sa_cap_mesh))
#define MESH_SETS ((MESH_ROOM - 40) / 24)
static uint8_t mesh_bytes[MESH_ROOM];

/* the open picture's draws, and those it lost. picture_unowned numbers its
 * draws no task owns */
static uint32_t picture_draws, picture_lost, picture_unowned;
/* the open picture's number among those begun, and how many drawers' things
 * its draws have named, for nth_of */
static uint32_t picture_names, names_held;
static int picture_open;
/* where the open picture's record lies, and the views its draws were made
 * under, in the order met. a view's id is its place from 1. views_placed
 * counts the draws each places */
static size_t picture_at;
static sa_view views[SA_VIEWS];
static uint32_t views_placed[SA_VIEWS];
static uint32_t view_count;
static int draw_level;
static int palettes_id = -1;
static uint32_t
    palettes_words[SA_CAP_PALETTES_COUNT * SA_CAP_PALETTE_ENTRIES * 2 +
                   SA_CAP_UNLIT_OFFSETS];

static struct task *sa1_step_task;
void (*native_picture_sink)(const uint8_t *records, size_t bytes);

/* the game's statics the adapter reads, by the names the program keeps for
 * them (native.h). sa1_statics_find sets every one or none, so a static found
 * is never null. it returns NULL, or the first name it can't find */
enum {
    LIGHT_CONTROL,
    CAMERA_CONTROL,
    CAMERA_STEPS,
    EASY_CALC,
    EASY_TINT,
    SIMPLE_DIRECTION,
    SIMPLE_SCALE,
    SIMPLE_AMBIENT,
    ACT_LIGHTS,
    STATICS
};
static const char *const static_names[STATICS] = {
    [LIGHT_CONTROL] = "_nj_light_control_",
    [CAMERA_CONTROL] = "camera_control_record",
    [CAMERA_STEPS] = "camera_steps",
    [EASY_CALC] = "calc",
    [EASY_TINT] = "light_tint",
    [SIMPLE_DIRECTION] = "base_light_direction",
    [SIMPLE_SCALE] = "base_light_scale",
    [SIMPLE_AMBIENT] = "iamb",
    [ACT_LIGHTS] = "sources_file",
};
static char *statics[STATICS];

const char *sa1_statics_find(void)
{
    char *found[STATICS];

    for (int i = 0; i < STATICS; i++) {
        if (!(found[i] = native_symbol(static_names[i]))) {
            return static_names[i];
        }
    }
    memcpy(statics, found, sizeof statics);
    return NULL;
}

/* a record's bytes, and tail_bytes of tail after them, under a head that says
 * the size of both. a record past SA_CAP_RECORD_MOST is left out and counted.
 * its head would say less, and a reader would lose its place in every record
 * after it. each writer holds its record to that before it names anything */
static void emit(uint16_t kind, void *record, uint32_t bytes, const void *tail,
                 uint32_t tail_bytes)
{
    sa_record_head head = {kind, (uint16_t)(bytes + tail_bytes)};
    size_t need = records_used + bytes + tail_bytes;

    if ((uint64_t)bytes + tail_bytes > SA_CAP_RECORD_MOST) {
        records_refused++;
        return;
    }
    if (need > records_room) {
        records_room = need * 2;
        records = realloc(records, records_room);
    }
    records_most = need > records_most ? need : records_most;
    memcpy(record, &head, sizeof head);
    memcpy(records + records_used, record, bytes);
    if (tail_bytes) {
        memcpy(records + records_used + bytes, tail, tail_bytes);
    }
    records_used = need;
}

void native_capture_open(const char *path)
{
    sa_cap_head head = {.magic = SA_CAPTURE_MAGIC,
                        .version = SA_CAPTURE_VERSION,
                        .game = SA_GAME_SA1};

    if (!(capture = fopen(path, "wb"))) {
        fprintf(stderr, "sa1: can't write %s\n", path);
        exit(2);
    }
    emit(SA_CAP_HEAD, &head, sizeof head, NULL, 0);
}

/* a table of names, each by a 64-bit key. a name forgotten leaves its slot
 * GONE, which a search passes over. kind is what an SA_CAP_FREE of its names
 * gives: SA_KIND_MESH, _TEXTURE or _MATERIAL, SA_CAP_MATERIAL or SA_REC_SHADE.
 * taken counts the slots named or GONE, which table_name keeps under
 * TABLE_SLOTS / 2. live counts the names held now, and most the most held at
 * once. named is the key last named, as the table holds it */
#define TABLE_SLOTS (1U << 16)
#define TABLE_GONE UINT64_MAX
struct table {
    uint64_t keys[TABLE_SLOTS];
    uint32_t ids[TABLE_SLOTS];
    uint32_t used[TABLE_SLOTS]; /* the picture that last named it */
    /* how many parts of the act's scene read it now: it stays while one does.
     * each part holds at most one name of a table. SCENE_PARTS keeps the held
     * names under a quarter of the slots */
    uint16_t holds[TABLE_SLOTS];
    uint64_t named;
    uint32_t next, taken, kind, live, most;
};

static uint32_t picture_number;

static uint32_t table_home(uint64_t key)
{
    return (uint32_t)(key ^ key >> 32) % TABLE_SLOTS;
}

/* a reader lets go of what the slot named */
static void table_forget(struct table *table, uint32_t slot)
{
    if (recording) {
        sa_cap_life life = {.kind = table->kind, .id = table->ids[slot]};

        emit(SA_CAP_FREE, &life, sizeof life, NULL, 0);
    }
    table->keys[slot] = TABLE_GONE;
    table->live--;
}

/* empty the GONE slots, and put each name back from its home. where the live
 * names fill a quarter of the slots, all go but those the act's scene reads */
static void table_tidy(struct table *table)
{
    static uint64_t keys[TABLE_SLOTS];
    static uint32_t ids[TABLE_SLOTS], used[TABLE_SLOTS];
    static uint16_t holds[TABLE_SLOTS];
    int full = table->live >= TABLE_SLOTS / 4;
    uint32_t live = 0;

    for (uint32_t i = 0; i < TABLE_SLOTS; i++) {
        if (table->keys[i] == 0 || table->keys[i] == TABLE_GONE) {
            continue;
        }
        if (full && table->holds[i] == 0) {
            table_forget(table, i);
            continue;
        }
        keys[live] = table->keys[i];
        ids[live] = table->ids[i];
        holds[live] = table->holds[i];
        used[live++] = table->used[i];
    }
    memset(table->keys, 0, sizeof table->keys);
    memset(table->holds, 0, sizeof table->holds);
    for (uint32_t i = 0; i < live; i++) {
        uint32_t slot = table_home(keys[i]);

        while (table->keys[slot] != 0) {
            slot = (slot + 1) % TABLE_SLOTS;
        }
        table->keys[slot] = keys[i];
        table->ids[slot] = ids[i];
        table->holds[slot] = holds[i];
        table->used[slot] = used[i];
    }
    table->taken = live;
}

/* the slot that holds key, or the empty one that ends its search */
static uint32_t table_find(const struct table *table, uint64_t key)
{
    uint32_t slot = table_home(key);

    while (table->keys[slot] != 0 && table->keys[slot] != key) {
        slot = (slot + 1) % TABLE_SLOTS;
    }
    return slot;
}

/* put key's id in *id: a new one where the table lacks key. it returns 1 for a
 * new id, and 0 otherwise. ids go on rising, so a reader never meets an id
 * twice with two meanings */
static int table_name(struct table *table, uint64_t key, uint32_t *id)
{
    uint32_t slot;

    /* 0 and GONE mark slots, so a key takes neither */
    key = key == 0 || key == TABLE_GONE ? 1 : key;
    slot = table_find(table, key);
    table->named = key;

    if (table->keys[slot] == key) {
        *id = table->ids[slot];
        table->used[slot] = picture_number;
        return 0;
    }
    if (table->taken + 1 >= TABLE_SLOTS / 2) {
        table_tidy(table);
        slot = table_find(table, key);
    }
    table->keys[slot] = key;
    table->ids[slot] = *id = table->next++;
    table->used[slot] = picture_number;
    table->taken++;
    if (++table->live > table->most) {
        table->most = table->live;
    }
    return 1;
}

/* forget what no picture has named for keep pictures, but what a part of
 * the act's scene reads */
static void table_age(struct table *table, uint32_t keep)
{
    for (uint32_t i = 0; i < TABLE_SLOTS; i++) {
        if (table->keys[i] != 0 && table->keys[i] != TABLE_GONE &&
            table->holds[i] == 0 && picture_number - table->used[i] > keep) {
            table_forget(table, i);
        }
    }
}

/* a part of the act's scene reads key's name from now, so no age or tidy
 * forgets it. it returns key, or 0 where the table holds no such name */
static uint64_t table_hold(struct table *table, uint64_t key)
{
    uint32_t slot = table_find(table, key);

    if (key == 0 || table->keys[slot] != key) {
        return 0;
    }
    table->holds[slot]++;
    return key;
}

/* a part of the act's scene reads key's name no longer. key is
 * table_hold's */
static void table_release(struct table *table, uint64_t key)
{
    if (key != 0) {
        table->holds[table_find(table, key)]--;
    }
}

/* hash moves off small numbers first, since seeds, ids and counts are small. a
 * word equal to hash would zero it */
static inline uint64_t mix(uint64_t hash, const void *data, size_t bytes)
{
    const uint8_t *at = data;

    hash ^= 0x9ae16a3b2f90404fULL;

    while (bytes >= 8) {
        uint64_t word;

        memcpy(&word, at, 8);
        hash = (hash ^ word) * 0x9e3779b97f4a7c15ULL;
        hash ^= hash >> 29;
        at += 8;
        bytes -= 8;
    }
    while (bytes-- > 0) {
        hash = (hash ^ *at++) * 0x9e3779b97f4a7c15ULL;
    }
    return hash ^ hash >> 29;
}

/* the bytes one meshset's polygons take: index words, and uv corners */
struct set_size {
    uint32_t halves, corners;
};

static void set_size(const NJS_MESHSET *set, struct set_size *size)
{
    uint32_t kind = set->type_matId >> MESHSET_TYPE_SHIFT;
    uint32_t polygons = set->nbMesh;

    size->halves = size->corners = 0;
    if (kind <= 1) {
        size->corners = size->halves = polygons * (kind == 0 ? 3 : 4);
        return;
    }
    const uint16_t *at = (const uint16_t *)set->meshes;

    for (uint32_t i = 0; i < polygons; i++) {
        uint32_t count = *at & STRIP_COUNT;

        size->halves += 1 + count;
        size->corners += count;
        at += 1 + count;
    }
}

static uint64_t model_hash(const NJS_MODEL *model, struct set_size *sizes)
{
    uint32_t points = (uint32_t)model->nbPoint;
    uint64_t hash = 0x51a1a1a1a1a1a1a1ULL;

    hash = mix(hash, &model->center, 16);
    hash = mix(hash, &model->nbMat, sizeof model->nbMat);
    hash = mix(hash, model->points, 12 * points);
    if (model->normals) {
        hash = mix(hash, model->normals, 12 * points);
    }
    for (uint32_t i = 0; i < model->nbMeshset; i++) {
        const NJS_MESHSET *set = &model->meshsets[i];

        set_size(set, &sizes[i]);
        hash = mix(hash, set, 4);
        hash = mix(hash, set->meshes, 2 * sizes[i].halves);
        if (set->vertuv) {
            hash = mix(hash, set->vertuv, 4 * sizes[i].corners);
        }
    }
    return hash;
}

static inline void put32(uint8_t *at, uint32_t word)
{
    memcpy(at, &word, 4);
}

/* the count of bytes model takes as an SA_MESH_NINJA_INLINE mesh. a non-null
 * out has room for them, and takes them laid out, each pointer counted from
 * SA_CAP_INLINE_BASE */
static uint64_t model_bytes(const NJS_MODEL *model,
                            const struct set_size *sizes, uint8_t *out)
{
    uint64_t points = (uint32_t)model->nbPoint;
    uint32_t sets = model->nbMeshset;
    uint64_t size = 40;
    uint64_t at_points = size;
    uint64_t at_normals = 0;
    uint64_t at_sets;

    size += 12 * points;
    if (model->normals) {
        at_normals = size;
        size += 12 * points;
    }
    at_sets = size;
    size += 24 * (uint64_t)sets;
    uint64_t at_data = size;
    for (uint32_t i = 0; i < sets; i++) {
        size += (2 * (uint64_t)sizes[i].halves + 3) & ~(uint64_t)3;
        if (model->meshsets[i].vertuv) {
            size += 4 * (uint64_t)sizes[i].corners;
        }
    }
    if (!out) {
        return size;
    }
    memset(out, 0, size);
    put32(out, SA_CAP_INLINE_BASE + (uint32_t)at_points);
    put32(out + 4, at_normals ? SA_CAP_INLINE_BASE + (uint32_t)at_normals : 0);
    put32(out + 8, (uint32_t)points);
    put32(out + 12, SA_CAP_INLINE_BASE + (uint32_t)at_sets);
    memcpy(out + 20, &model->nbMeshset, 2);
    memcpy(out + 22, &model->nbMat, 2);
    memcpy(out + 24, &model->center, 16);
    memcpy(out + at_points, model->points, 12 * points);
    if (model->normals) {
        memcpy(out + at_normals, model->normals, 12 * points);
    }
    uint32_t data = (uint32_t)at_data;
    for (uint32_t i = 0; i < sets; i++) {
        const NJS_MESHSET *set = &model->meshsets[i];
        uint8_t *record = out + at_sets + 24 * i;

        memcpy(record, set, 4);
        put32(record + 4, SA_CAP_INLINE_BASE + data);
        memcpy(out + data, set->meshes, 2 * sizes[i].halves);
        data += (2 * sizes[i].halves + 3) & ~3U;
        if (set->vertuv) {
            put32(record + 20, SA_CAP_INLINE_BASE + data);
            memcpy(out + data, set->vertuv, 4 * sizes[i].corners);
            data += 4 * sizes[i].corners;
        }
    }
    return size;
}

/* a reader lets go of a mesh that no picture has named for MESH_KEEP pictures.
 * lists of materials, materials and shades age the same way. a draw names its
 * materials before their list, so a list named lately keeps them */
#define MESH_KEEP 30
#define AGE_EVERY 16
static struct table meshes = {.kind = SA_KIND_MESH};
static struct table lists = {.kind = SA_KIND_MATERIAL};
static struct table materials = {.kind = SA_CAP_MATERIAL};
static struct table shades = {.kind = SA_REC_SHADE};

/* model's mesh, or SA_CAP_NONE for a model too large for one record. such a
 * model is counted, and takes no name. a name with no record would leave its
 * later draws naming a mesh no reader has */
static uint32_t mesh_of(const NJS_MODEL *model)
{
    static struct set_size sizes[MESH_SETS];
    uint64_t hash;
    uint32_t id;

    if (model->nbMeshset > MESH_SETS) {
        records_refused++;
        return SA_CAP_NONE;
    }
    hash = model_hash(model, sizes);
    if (model_bytes(model, sizes, NULL) > MESH_ROOM) {
        records_refused++;
        return SA_CAP_NONE;
    }
    if (table_name(&meshes, hash, &id)) {
        sa_cap_mesh mesh = {.mesh = id, .format = SA_MESH_NINJA_INLINE};

        emit(SA_CAP_MESH, &mesh, sizeof mesh, mesh_bytes,
             (uint32_t)model_bytes(model, sizes, mesh_bytes));
    }
    return id;
}

/* a texture is keyed by its record in Ninja's texture memory and its global
 * index. a load makes the record, and a release ends it. a texture named by
 * its global index alone takes INDEX_ALONE for the record's word, since no
 * memory record lies at the top of the address space */
static struct table textures = {.kind = SA_KIND_TEXTURE};
#define INDEX_ALONE 0xffffffffU

static uint64_t texture_key(const NJS_TEXLIST *list, uint32_t index)
{
    const NJS_TEXMEMLIST *memory;

    if (!list || index >= list->nbTexture || !list->textures[index].texaddr) {
        return 0;
    }
    memory = native_widen(list->textures[index].texaddr);
    return (uint64_t)(uintptr_t)memory << 32 | memory->globalIndex;
}

/* the bytes of a PVR texture's data as a PVRT chunk holds them. the levels run
 * from the smallest up, after a VQ texture's codebook. the levels of a
 * twiddled texture with mipmaps follow a texel's 2 bytes of padding. a small
 * VQ's codebook holds as many codes as its size needs. it returns 0 for a
 * layout PVR files don't have */
static uint32_t texels_bytes(uint32_t layout, uint32_t w, uint32_t h)
{
    enum {
        TWIDDLED = 1,
        TWIDDLED_MIPS = 2,
        VQ = 3,
        VQ_MIPS = 4,
        RECTANGLE = 9,
        STRIDE = 11,
        BOTH = 13,
        SMALL_VQ = 16,
        SMALL_VQ_MIPS = 17,
        TWIDDLED_MIPS_ALT = 18
    };
    int mips = layout == TWIDDLED_MIPS || layout == TWIDDLED_MIPS_ALT ||
               layout == VQ_MIPS || layout == SMALL_VQ_MIPS;
    uint32_t smaller = 0;
    uint32_t codes = 256;

    switch (layout) {
    case TWIDDLED:
    case RECTANGLE:
    case STRIDE:
    case BOTH:
        return w * h * 2;
    case TWIDDLED_MIPS:
    case TWIDDLED_MIPS_ALT:
        for (uint32_t size = 1; size < w; size <<= 1) {
            smaller += size * size * 2;
        }
        return 2 + smaller + w * h * 2;
    case SMALL_VQ:
    case SMALL_VQ_MIPS:
        codes = w <= 16            ? 16
                : w == 32          ? (mips ? 64 : 32)
                : w == 64 && !mips ? 128
                                   : 256;
        /* fall through */
    case VQ:
    case VQ_MIPS:
        if (mips) {
            smaller = 1;
            for (uint32_t size = 2; size < w; size <<= 1) {
                smaller += (size / 2) * (size / 2);
            }
        }
        return codes * 8 + smaller + (w / 2) * (h / 2);
    }
    return 0;
}

/* send texture id's PVRT chunk: w by h texels from data, of the PVR's pixel
 * type and layout. it goes in parts that each fit a record. a record's size
 * is a multiple of 4. so the chunk ends in zeros where its texels' bytes are
 * no multiple of 4, and its length counts them. the texels' last bytes then
 * go with the zeros in a part of their own. a layout PVR files don't have
 * sends nothing, and a reader then looks in the disc's files */
static void texels_emit(uint32_t id, uint32_t type, uint32_t layout, uint16_t w,
                        uint16_t h, const uint8_t *data)
{
    enum { PART = 0x8000 };
    _Static_assert(sizeof(sa_cap_texels) + PART <= SA_CAP_RECORD_MOST &&
                       PART % 4 == 0,
                   "a part of a texture's texels outgrows a record, or "
                   "is no multiple of 4");
    uint32_t bytes = texels_bytes(layout, w, h);
    uint32_t whole = bytes & ~3U;
    uint32_t length = 8 + ((bytes + 3) & ~3U);
    uint8_t pvrt[16] = {'P', 'V', 'R', 'T'};
    uint8_t last[4] = {0};
    sa_cap_texels part = {.texture = id};

    if (bytes == 0) {
        return;
    }
    memcpy(pvrt + 4, &length, 4);
    pvrt[8] = (uint8_t)type;
    pvrt[9] = (uint8_t)layout;
    memcpy(pvrt + 12, &w, 2);
    memcpy(pvrt + 14, &h, 2);
    emit(SA_CAP_TEXELS, &part, sizeof part, pvrt, sizeof pvrt);
    for (uint32_t at = 0; at < whole; at += PART) {
        emit(SA_CAP_TEXELS, &part, sizeof part, data + at,
             whole - at < PART ? whole - at : PART);
    }
    if (bytes != whole) {
        memcpy(last, data + whole, bytes - whole);
        emit(SA_CAP_TEXELS, &part, sizeof part, last, sizeof last);
    }
}

/* the PVRT chunk of texture id, which the game made in memory as info says */
static void texture_texels(uint32_t id, const NJS_TEXINFO *info)
{
    uint32_t type = info->texsurface.Type >> 16;

    texels_emit(id, type, type >> 8 & 0xff, info->texsurface.nWidth,
                info->texsurface.nHeight, (const uint8_t *)info->texaddr);
}

/* name is the texture's, or null for the name its list holds. a texture the
 * game made in memory has none: its list holds its NJS_TEXINFO there.
 * native_texture_file finds its record's file by its texture memory, whichever
 * list names it */
static uint32_t texture_named(const NJS_TEXLIST *list, uint32_t index,
                              const char *name)
{
    uint64_t key = texture_key(list, index);
    uint32_t attr;
    uint32_t id;

    if (key == 0) {
        return SA_CAP_NONE;
    }
    attr = list->textures[index].attr;
    if (!name && !(attr & NJD_TEXATTR_TYPE_MEMORY)) {
        name = list->textures[index].filename;
    }
    if (table_name(&textures, key, &id)) {
        const NJS_TEXMEMLIST *memory =
            native_widen(list->textures[index].texaddr);

        sa_cap_texture texture = {
            .texture = id,
            .global_index = memory->globalIndex,
            .file = native_texture_file(list->textures[index].texaddr)};

        if (name) {
            strncpy(texture.name, name, sizeof texture.name - 1);
        }
        emit(SA_CAP_TEXTURE, &texture, sizeof texture, NULL, 0);
        if (attr & NJD_TEXATTR_TYPE_MEMORY) {
            texture_texels(id, list->textures[index].filename);
        }
    }
    return id;
}

static uint32_t texture_of(const NJS_TEXLIST *list, uint32_t index)
{
    return texture_named(list, index, NULL);
}

/* the game loaded texlist. each texture is named here, so a reader makes it at
 * the load, not at its first draw */
void sa1_textures_loaded(const void *texlist, uint32_t count,
                         const char *const *names)
{
    const NJS_TEXLIST *list = texlist;

    if (!recording) {
        return;
    }
    for (uint32_t i = 0; i < count && i < list->nbTexture; i++) {
        uint32_t id = texture_named(list, i, names ? names[i] : NULL);

        if (id != SA_CAP_NONE) {
            sa_cap_life life = {.kind = SA_KIND_TEXTURE, .id = id};

            emit(SA_CAP_LOAD, &life, sizeof life, NULL, 0);
        }
    }
}

/* the game released texlist. a later draw of the same record names it
 * again */
void sa1_textures_released(const void *texlist)
{
    const NJS_TEXLIST *list = texlist;

    for (uint32_t i = 0; i < list->nbTexture; i++) {
        uint64_t key = texture_key(list, i);
        uint32_t slot = table_find(&textures, key);

        if (key != 0 && textures.keys[slot] == key) {
            table_forget(&textures, slot);
        }
    }
}

static uint32_t material_of(sa_cap_material material)
{
    uint32_t id;

    /* a material is what follows its id */
    if (table_name(&materials,
                   mix(1, &material.texture,
                       sizeof material - offsetof(sa_cap_material, texture)),
                   &id)) {
        material.material = id;
        emit(SA_CAP_MATERIAL, &material, sizeof material, NULL, 0);
    }
    return id;
}

/* a list of count materials is named by their keys, not by their ids. a list a
 * part of the act's scene reads may outlive its materials' names. the same
 * materials named anew are still that list */
static uint32_t materials_of(const uint32_t *ids, const uint64_t *keys,
                             uint32_t count)
{
    uint32_t id;

    if (table_name(&lists, mix(count + 1, keys, 8 * count), &id)) {
        sa_cap_materials list = {.materials = id, .count = (uint16_t)count};

        emit(SA_CAP_MATERIALS, &list, sizeof list, ids, 4 * count);
    }
    return id;
}

/* Ninja's light queue as njSetQueue makes it at a full draw. it holds each
 * created light that is on, in order, put in world space by its own matrix.
 * Ninja's list holds the SH-4's 32-bit pointers, as njCreateLight writes
 * them */
static void lights_now(sa_lights *out)
{
    const uint32_t *control = (const uint32_t *)statics[LIGHT_CONTROL];

    memset(out, 0, sizeof *out);
    for (uint32_t i = 0; i < control[NJD_LIGHT_MAX] && i < NJD_LIGHT_MAX; i++) {
        const NJS_LIGHT *l = native_widen(control[i]);
        const float *m;
        sa_light *s;

        if (!l || l->stat != 1) {
            continue;
        }
        m = l->mtrx;
        if (out->count == SA_LIGHTS) {
            picture_lost++;
            break;
        }
        s = &out->light[out->count++];
        /* njCreateLight, which asm2c.py turns into C, writes the type where
         * the SH-4 lays attr, just after ltcal. on the host, the pointer in
         * attr aligns attr 4 bytes later */
        memcpy(&s->type, (const char *)&l->ltcal + sizeof l->ltcal,
               sizeof s->type);
        for (int j = 0; j < 3; j++) {
            s->point[j] = l->pnt.x * m[j] + l->pnt.y * m[4 + j] +
                          l->pnt.z * m[8 + j] + m[12 + j];
            s->direction[j] =
                l->vctr.x * m[j] + l->vctr.y * m[4 + j] + l->vctr.z * m[8 + j];
        }
        s->ambient[0] = l->ltcal.amb.r;
        s->ambient[1] = l->ltcal.amb.g;
        s->ambient[2] = l->ltcal.amb.b;
        s->diffuse[0] = l->ltcal.dif.r;
        s->diffuse[1] = l->ltcal.dif.g;
        s->diffuse[2] = l->ltcal.dif.b;
        s->specular[0] = l->ltcal.spc.a;
        s->specular[1] = l->ltcal.spc.r;
        s->specular[2] = l->ltcal.spc.g;
        s->specular[3] = l->ltcal.spc.b;
        s->inner = l->ltcal.cosi;
        s->outer = l->ltcal.cose;
        s->near = l->ltcal.nrr;
        s->far = l->ltcal.frr;
    }
}

/* shade's id. under SA_SHADE_LIGHTS, the lights on now join its key. a new
 * shade's SA_REC_LIGHTS goes just before it */
static uint32_t shade_of(sa_shade shade)
{
    sa_lights lights;
    /* a shade is what follows its id, and lights their count and each one */
    uint64_t key =
        mix(1, shade.light, sizeof shade - offsetof(sa_shade, light));
    uint32_t id;

    if (shade.mode == SA_SHADE_LIGHTS) {
        lights_now(&lights);
        key =
            mix(key, &lights.count, sizeof lights - offsetof(sa_lights, count));
    }
    if (table_name(&shades, key, &id)) {
        shade.shade = id;
        if (shade.mode == SA_SHADE_LIGHTS) {
            lights.shade = id;
            emit(SA_REC_LIGHTS, &lights, sizeof lights, NULL, 0);
        }
        emit(SA_REC_SHADE, &shade, sizeof shade, NULL, 0);
    }
    return id;
}

/* the ISP and TSP words the next draw's header takes. a draw that calls
 * njGetkmPointer finds the context's, and a draw inside a run takes
 * _nj_parameterkm_'s copy */
static void header_words(enum sa1_header from, uint32_t *isp, uint32_t *tsp)
{
    if (from == SA1_RUN) {
        *isp = _nj_parameterkm_.ISPPARAMBUFFER;
        *tsp = _nj_parameterkm_.TSPPARAMBUFFER;
    } else {
        *isp = (uint32_t)_nj_current_ctx_->ISPPARAMBUFFER;
        *tsp = (uint32_t)_nj_current_ctx_->TSPPARAMBUFFER;
    }
}

/* the filter a TSP word's field asks for. a material's flags hold the field
 * where the TSP word does. the PowerVR's 2 and 3 are trilinear's two passes */
static uint8_t filter_of(uint32_t tsp)
{
    uint32_t filter = (tsp & TSP_FILTER) >> TSP_FILTER_SHIFT;

    return (uint8_t)(filter > SA_FILTER_TRILINEAR ? SA_FILTER_TRILINEAR
                                                  : filter);
}

/* how a meshset's alpha takes Ninja's constant material, as the Dreamcast's
 * walk in engine/dc/sa1/model.c names the modes */
enum alpha_mode { ALPHA_OWN, ALPHA_CONSTANT, ALPHA_OFFSET };

/* a translucent meshset's alpha byte, as the Dreamcast's point_alpha gives it.
 * ALPHA_CONSTANT puts the constant's alpha in the material's place.
 * ALPHA_OFFSET adds the two, held to 0 to 1. the value times 255 rounds toward
 * zero, as FTRC does, and keeps its low byte. a constant alpha of -0.5 goes as
 * 0x81 */
static uint32_t point_alpha(uint32_t alpha, enum alpha_mode mode,
                            float constant)
{
    float value = (float)((double)alpha * (double)INV_255);

    if (mode == ALPHA_CONSTANT) {
        value = constant;
    } else if (mode == ALPHA_OFFSET) {
        value += constant;
        if (value > 1.0F) {
            value = 1.0F;
        }
        if (!(value > 0.0F)) {
            value = 0.0F;
        }
    }
    return (uint32_t)sa_ftrc((float)((double)value * 255.0)) & 0xff;
}

static uint32_t channel(double value)
{
    double scaled = floor(value * 255.0);

    return scaled < 0.0 ? 0 : scaled > 255.0 ? 255 : (uint32_t)scaled;
}

static uint32_t argb(double a, double r, double g, double b)
{
    return channel(a) << 24 | channel(r) << 16 | channel(g) << 8 | channel(b);
}

static inline double dot3(double a0, double a1, double a2, double b0, double b1,
                          double b2)
{
    return a0 * b0 + a1 * b1 + a2 * b2;
}

/* object to the game's world, as 3 rows of 4. XMTRX is object to view, and
 * Ninja's view now is world to view, whatever the picture opened under.
 * Ninja's rows take a point as p * M. the view is rigid, so W = M V^-1. the
 * record's row j is W's column j */
static void to_world(float world[12])
{
    const float *m = native_xmtrx();
    const float *v = _nj_view_matrix_;
    double t[3] = {(double)m[12] - v[12], (double)m[13] - v[13],
                   (double)m[14] - v[14]};

    for (int j = 0; j < 3; j++) {
        const float *row = &v[4 * j];

        world[4 * j] = (float)dot3(m[0], m[1], m[2], row[0], row[1], row[2]);
        world[4 * j + 1] =
            (float)dot3(m[4], m[5], m[6], row[0], row[1], row[2]);
        world[4 * j + 2] =
            (float)dot3(m[8], m[9], m[10], row[0], row[1], row[2]);
        world[4 * j + 3] =
            (float)dot3(t[0], t[1], t[2], row[0], row[1], row[2]);
    }
}

/* the palettes the picture about to open shades from. where they changed since
 * the last version given, they get a new version. the adapter keeps only the
 * last, so a value held before gets a new version too */
static void snap_palettes(void)
{
    static uint32_t last[sizeof palettes_words / 4];

    _Static_assert(sizeof palettes_words == sizeof sa1_live_colors,
                   "the capture's palettes aren't the layer's");
    memcpy(palettes_words, sa1_live_colors, sizeof palettes_words);
    if (palettes_id < 0 || memcmp(last, palettes_words, sizeof last) != 0) {
        uint32_t id = (uint32_t)++palettes_id;
        /* sa_cap_palettes' head and id, which its words follow */
        uint8_t record[offsetof(sa_cap_palettes, words)];

        memcpy(last, palettes_words, sizeof last);
        memcpy(record + offsetof(sa_cap_palettes, palettes), &id, sizeof id);
        emit(SA_CAP_PALETTES, record, sizeof record, palettes_words,
             sizeof palettes_words);
    }
}

/* camera.h's camera_control, as far as a cut reads it, laid out by the
 * same members */
struct camera_control_head {
    int32_t changed;
    int16_t collision;
    int8_t mode, type, level, adjust;
    uint8_t unk_0a;
    int8_t flags;
    void *step, *adjust_step;
    int32_t steps;
};
#define CAMERA_ADJUST_NONE 0

/* whether the view cut in the steps since the picture before, by the game's
 * own word. it cut where the camera started again, as camera_steps counts. it
 * also cut where the camera took a mode with no adjust mode to carry it over.
 * a camera that started again just after the picture before has run all those
 * steps. so camera_steps falling below its last count is a cut too */
static int camera_cut_since(uint32_t steps)
{
    const struct camera_control_head *control =
        (const struct camera_control_head *)statics[CAMERA_CONTROL];
    const uint32_t *camera_steps = (const uint32_t *)statics[CAMERA_STEPS];
    static uint32_t last = UINT32_MAX;
    uint32_t now = *camera_steps;

    if (now < last && last != UINT32_MAX) {
        last = now;
        return 1;
    }
    last = now;
    return now < steps || (control->steps < (int32_t)steps &&
                           control->adjust == CAMERA_ADJUST_NONE);
}

/* split_shown is TRUE from the end of a split frame's first picture to the
 * frame's end. a split frame shows two pictures for one frame of play, as 60
 * fps asks. its second picture takes the next step's tick, since play_time
 * counts the frame's steps as it ends. last_tick is the last picture's tick,
 * and tick_before the one before */
static int split_shown;
static uint32_t last_tick, tick_before;

/* Ninja's view of the moment, and what live.c's walk projects by, as a
 * record holds a view */
static sa_view view_now(void)
{
    const float *v = _nj_view_matrix_;
    sa_view now = {.focal = {sa1_screen.scale_x, sa1_screen.scale_y},
                   .center = {sa1_screen.cx, sa1_screen.cy},
                   .near_z = sa1_screen.near_clip,
                   .far_z = _nj_far_clip_};

    for (int j = 0; j < 3; j++) {
        now.world_to_view[4 * j] = v[j];
        now.world_to_view[4 * j + 1] = v[4 + j];
        now.world_to_view[4 * j + 2] = v[8 + j];
        now.world_to_view[4 * j + 3] = v[12 + j];
    }
    return now;
}

/* the id of the view a draw of the open picture is made under: Ninja's view
 * now. the first draw under a view gives it a record. placed is TRUE where the
 * view places the draw, as for all but a draw on the screen. where the picture
 * holds SA_VIEWS views and now's is new, it returns 0 for a placed draw. a
 * draw on the screen needs no view of its own, and then takes SA_VIEWS. a
 * view counts each draw it places, so a caller asks once nothing else can
 * lose its draw */
static uint8_t view_id(int placed)
{
    sa_view now = view_now();
    uint32_t id = view_count;

    while (id > 0 && memcmp(&views[id - 1], &now, sizeof now) != 0) {
        id--;
    }
    if (id == 0) {
        sa_picture_view given = {.view = view_count + 1, .value = now};

        if (view_count == SA_VIEWS) {
            return (uint8_t)(placed ? 0 : SA_VIEWS);
        }
        views[view_count] = now;
        views_placed[view_count] = 0;
        id = ++view_count;
        emit(SA_REC_VIEW, &given, sizeof given, NULL, 0);
    }
    views_placed[id - 1] += placed != 0;
    return (uint8_t)id;
}

/* write the open picture's own view into its record, as its end knows it. it
 * is the view most of the picture's placed draws were made under. the first
 * met wins a tie. where no draw is placed, it is the first view. a picture
 * with no view keeps the one it opened under */
static void picture_view_settle(void)
{
    uint32_t own = 0;

    if (view_count == 0) {
        return;
    }
    for (uint32_t i = 1; i < view_count; i++) {
        own = views_placed[i] > views_placed[own] ? i : own;
    }
    memcpy(records + picture_at + offsetof(sa_cap_picture, view), &views[own],
           sizeof views[own]);
}

/* write the open picture's background into its record, as its end finds it. it
 * is the plane's three colors, opaque as the PowerVR draws its plane. the
 * Dreamcast renders over the plane last set, so a color set mid-picture is
 * that picture's */
static void picture_back_settle(void)
{
    static const size_t corners[3] = {offsetof(sa_cap_picture, clear),
                                      offsetof(sa_cap_picture, clear_right),
                                      offsetof(sa_cap_picture, clear_below)};

    for (int i = 0; i < 3; i++) {
        uint32_t color = sa1_back[i] | 0xff000000U;

        memcpy(records + picture_at + corners[i], &color, sizeof color);
    }
}

static void begin_picture(void)
{
    uint32_t tick = play_time + (uint32_t)split_shown;
    sa_cap_picture picture = {
        .picture = picture_number, .tick = tick, .view = view_now()};
    uint32_t density = REGISTER(FOG_DENSITY);
    int mantissa = (density >> 8) & 0xff;
    int exponent = (int)(int8_t)(density & 0xff);

    snap_palettes();
    picture.fog = REGISTER(FOG_COLOR) | 0xff000000U;
    picture.fog_density = (float)(mantissa / 128.0 * pow(2.0, exponent));
    for (size_t i = 0;
         i < sizeof picture.fog_table / sizeof picture.fog_table[0]; i++) {
        picture.fog_table[i] = (uint16_t)REGISTER(FOG_TABLE + 4 * i);
    }
    picture.palettes = (uint32_t)palettes_id;
    if (camera_cut_since(tick - last_tick) && picture_number != 0) {
        picture.flags = SA_PICTURE_CUT;
    }
    tick_before = last_tick;
    last_tick = tick;
    picture_at = records_used;
    emit(SA_CAP_PICTURE, &picture, sizeof picture, NULL, 0);
    picture_open = 1;
    picture_draws = picture_lost = picture_unowned = 0;
    picture_names++;
    names_held = 0;
    view_count = 0;
}

/* a task's serial: from 1, in the order the records first meet it */
#define TASK_SLOTS (1U << 14)
static struct {
    const struct task *task;
    uint32_t serial;
} serials[TASK_SLOTS];
static uint32_t serial_count, serial_live, serial_most;

static uint32_t task_slot(const struct task *task)
{
    uint32_t slot =
        (uint32_t)(((uintptr_t)task / 16) * 2654435761U) % TASK_SLOTS;

    while (serials[slot].task && serials[slot].task != task) {
        slot = (slot + 1) % TASK_SLOTS;
    }
    return slot;
}

static uint32_t serial_of(const struct task *task)
{
    uint32_t slot = task_slot(task);
    sa_task_create create = {0};

    if (serials[slot].task) {
        return serials[slot].serial;
    }
    serials[slot].task = task;
    serials[slot].serial = ++serial_count;
    serial_most = ++serial_live > serial_most ? serial_live : serial_most;
    create.task = serial_count;
    if (task->parent) {
        uint32_t parent = task_slot(task->parent);

        create.parent = serials[parent].task ? serials[parent].serial : 0;
    }
    create.step = native_name_id(task->step);
    create.level = task->parent ? SA_TASK_CHILD : (uint8_t)draw_level;
    create.records = (uint8_t)((task->work ? SA_TASK_WORK : 0) |
                               (task->motion ? SA_TASK_MOTION : 0) |
                               (task->force ? SA_TASK_FORCE : 0) |
                               (task->extra ? SA_TASK_EXTRA : 0));
    emit(SA_REC_TASK_CREATE, &create, sizeof create, NULL, 0);
    return serial_count;
}

/* a freed slot would end the search for an entry later in its run. so each
 * later entry of the run is put back from its home */
static void serial_forget(uint32_t slot)
{
    uint32_t next = (slot + 1) % TASK_SLOTS;

    serial_live--;
    serials[slot].task = NULL;
    while (serials[next].task) {
        const struct task *task = serials[next].task;
        uint32_t serial = serials[next].serial;

        serials[next].task = NULL;
        uint32_t home = task_slot(task);

        serials[home].task = task;
        serials[home].serial = serial;
        next = (next + 1) % TASK_SLOTS;
    }
}

void native_task_destroy(struct task *task)
{
    uint32_t slot = task_slot(task);

    if (recording && serials[slot].task) {
        sa_task_destroy destroy = {.task = serials[slot].serial,
                                   .destructor =
                                       native_name_id(task->destructor)};

        emit(SA_REC_TASK_DESTROY, &destroy, sizeof destroy, NULL, 0);
    }
    if (serials[slot].task) {
        serial_forget(slot);
    }
}

/* present.c's runs of a task's step and draw callback. in the native slice,
 * task.c calls native_task_step and native_task_draw in their place */
void present_run(struct task *task, void (*step)(struct task *task));
void present_draw(struct task *task, void (*draw)(struct task *task));

void native_task_step(struct task *task, void (*step)(struct task *task))
{
    struct task *outer = sa1_step_task;

    sa1_step_task = task;
    present_run(task, step);
    sa1_step_task = outer;
}

/* a draw callback's draws are its task's, as a step's are. so a split frame's
 * second picture names the task its first picture drew */
void native_task_draw(struct task *task, void (*draw)(struct task *task))
{
    struct task *outer = sa1_step_task;

    if (recording) {
        (void)serial_of(task);
    }
    sa1_step_task = task;
    present_draw(task, draw);
    sa1_step_task = outer;
}

/* the level task_draw_level draws, which a root task met there takes. the
 * levels run from 0 to LEVEL_LAST. task.h's TASK_LEVEL_FIRST, 8, names the
 * last, and any other level outside them goes as the last too */
#define LEVEL_LAST 7
void native_draw_level(int level)
{
    draw_level = level < 0 || level > LEVEL_LAST ? LEVEL_LAST : level;
}

static uint32_t instance(void)
{
    if (sa1_step_task) {
        return serial_of(sa1_step_task);
    }
    return SA_INSTANCE_UNOWNED | ++picture_unowned;
}

/* the adapter's number for the shape of a model's mesh, which the draw carries
 * as its thing. it hashes the model's count of points and each meshset's
 * material and polygons, which a pose leaves alone. a model whose points move
 * is a new mesh at each pose, but stays one thing. two models of one shape are
 * one thing, whose draws their order tells apart. it is never 0 */
static uint32_t shape_of(const NJS_MODEL *model)
{
    uint64_t hash = 0x7368617065736861ULL;

    hash = mix(hash, &model->nbPoint, sizeof model->nbPoint);
    hash = mix(hash, &model->nbMeshset, sizeof model->nbMeshset);
    for (uint32_t i = 0; i < model->nbMeshset; i++) {
        const NJS_MESHSET *set = &model->meshsets[i];
        struct set_size size;

        set_size(set, &size);
        hash = mix(hash, set, 4);
        hash = mix(hash, set->meshes, 2 * size.halves);
    }
    uint32_t shape = (uint32_t)(hash ^ hash >> 32);

    return shape != 0 ? shape : 1;
}

/* the count of each drawer's draws of each thing in the open picture: the next
 * one's nth. a slot is the open picture's where its picture equals
 * picture_names. begin_picture steps picture_names, so no slot is cleared.
 * every draw no task owns counts as one drawer's */
#define NAME_SLOTS (1U << 17)
static struct {
    uint64_t name; /* the drawer's instance, then the thing */
    uint32_t picture, count;
} names[NAME_SLOTS];

/* put in *nth the nth of the open picture's next draw of thing by instance. it
 * returns 1, or 0 where the picture names NAME_SLOTS / 2 drawers' things
 * already */
static int nth_of(uint32_t instance, uint32_t thing, uint32_t *nth)
{
    uint64_t name =
        (uint64_t)(instance & SA_INSTANCE_UNOWNED ? SA_INSTANCE_UNOWNED
                                                  : instance)
            << 32 |
        thing;
    uint32_t slot = (uint32_t)((name ^ name >> 32) * 2654435761U) % NAME_SLOTS;

    while (names[slot].picture == picture_names && names[slot].name != name) {
        slot = (slot + 1) % NAME_SLOTS;
    }
    if (names[slot].picture != picture_names) {
        if (names_held == NAME_SLOTS / 2) {
            return 0;
        }
        names_held++;
        names[slot].name = name;
        names[slot].picture = picture_names;
        names[slot].count = 0;
    }
    *nth = names[slot].count++;
    return 1;
}

/* the start of njModelE.c's struct calc_work, laid out as it is there */
struct calc_work {
    NJS_VECTOR light_direction;
    float negative_diffuse;
    int32_t point_count;
    void *queue_points;
    NJS_POINT3 *points;
    NJS_VECTOR *normals;
    float ambient;
};

const float *sa1_light_tint(void)
{
    return (const float *)statics[EASY_TINT];
}

static struct calc_work *easy_calc(void)
{
    return (struct calc_work *)statics[EASY_CALC];
}

/* dreamcast.h's port_easy_light. here the light lies in calc, the work of the
 * njModelE.c that the native slice builds */
NJS_VECTOR *port_easy_light(void)
{
    return &easy_calc()->light_direction;
}

void sa1_easy_light(double light[3], float *ambient, float *diffuse)
{
    const struct calc_work *calc = easy_calc();

    light[0] = calc->light_direction.x;
    light[1] = calc->light_direction.y;
    light[2] = calc->light_direction.z;
    *ambient = calc->ambient;
    *diffuse = -calc->negative_diffuse;
}

void sa1_simple_light(double light[3], float *ambient, float *diffuse)
{
    const float *direction = (const float *)statics[SIMPLE_DIRECTION];
    double length = sqrt(dot3(direction[0], direction[1], direction[2],
                              direction[0], direction[1], direction[2]));

    /* SimpleCalcModelLight holds it to a unit's length */
    for (int i = 0; i < 3; i++) {
        light[i] = length > 0.0 ? direction[i] / length : direction[i];
    }
    *diffuse = *(const float *)statics[SIMPLE_SCALE];
    *ambient = *(const float *)statics[SIMPLE_AMBIENT];
}

/* the state a model's draw reads as it is called. its model, its drawer's
 * light, XMTRX and Ninja's view are read elsewhere */
struct draw_state {
    const NJS_TEXLIST *texlist;
    NJS_ARGB material; /* Ninja's constant material */
    const struct sa1_attrs *attrs;
    uint32_t isp, tsp; /* the header words */
    uint8_t palette; /* under SA_SHADE_PALETTE, which of the eight */
};

/* a material's flags, own, as a draw under attrs takes them */
static uint32_t flags_as_drawn(const struct sa1_attrs *attrs, uint32_t own)
{
    return attrs->constant ? (own & attrs->and_bits) | attrs->or_bits : own;
}

/* a material as the draw takes it, by the drawer it goes through */
static sa_cap_material material_as_drawn(const NJS_MATERIAL *own_material,
                                         uint8_t mode,
                                         const struct draw_state *state,
                                         uint32_t specular_on)
{
    const struct sa1_attrs *attrs = state->attrs;
    uint32_t own = own_material->attrflags;
    uint32_t drawn = flags_as_drawn(attrs, own);
    uint32_t flags = attrs->flags;
    int ninja = attrs->forms;
    enum alpha_mode alpha_mode =
        flags & NJD_CONTROL_3D_CONSTANT_MATERIAL ? ALPHA_CONSTANT
        : flags & NJD_CONTROL_3D_OFFSET_MATERIAL ? ALPHA_OFFSET
                                                 : ALPHA_OWN;
    uint32_t diffuse = own_material->diffuse.color;
    uint32_t texture = SA_CAP_NONE;
    uint32_t bits = 0;
    int translucent = (drawn & NJD_FLAG_USE_ALPHA) != 0;
    int unlit = (drawn & NJD_FLAG_IGNORE_LIGHT) != 0;
    sa_cap_material out = {0};
    double a = (double)((diffuse >> 24) & 255) / 255;
    double r = (double)((diffuse >> 16) & 255) / 255;
    double g = (double)((diffuse >> 8) & 255) / 255;
    double b = (double)(diffuse & 255) / 255;
    uint32_t color = diffuse;

    if ((drawn & NJD_FLAG_USE_TEXTURE) && (own & NJD_FLAG_USE_TEXTURE)) {
        texture = texture_of(state->texlist,
                             own_material->attr_texId & MATERIAL_TEXTURE);
    }
    bits |= texture != SA_CAP_NONE ? SA_MAT_TEXTURE : 0;
    bits |= translucent ? SA_MAT_BLEND : 0;
    bits |= own & NJD_FLAG_USE_ENV ? SA_MAT_ENV : 0;
    bits |= drawn & NJD_FLAG_DOUBLE_SIDE ? SA_MAT_DOUBLE : 0;
    bits |= ninja && !(drawn & NJD_FLAG_DOUBLE_SIDE) ? SA_MAT_CULL : 0;
    bits |= unlit ? SA_MAT_UNLIT : 0;
    bits |= specular_on ? SA_MAT_SPECULAR : 0;
    /* a textured opaque unlit meshset shades as a decal, but under the simple
     * drawer, SA_SHADE_LINEAR. that drawer gives an unlit one its diffuse as
     * the intensity the texel takes, as njModelS.c's
     * njStartVertexTexOpaqueIntens does. a textured meshset adds its offset
     * color, but for a translucent unlit one in the Ninja forms */
    bits |= texture != SA_CAP_NONE && unlit && !translucent &&
                    mode != SA_SHADE_LINEAR
                ? SA_MAT_DECAL
                : 0;
    bits |= texture != SA_CAP_NONE && !(ninja && translucent && unlit)
                ? SA_MAT_OFFSET
                : 0;
    bits |= drawn & NJD_FLAG_CLAMP_U ? SA_MAT_CLAMP_U : 0;
    bits |= drawn & NJD_FLAG_CLAMP_V ? SA_MAT_CLAMP_V : 0;
    bits |= drawn & FLIP_U ? SA_MAT_FLIP_U : 0;
    bits |= drawn & FLIP_V ? SA_MAT_FLIP_V : 0;
    bits |= ninja ? SA_MAT_CONSTANT : 0;
    /* the walk's unlit translucent meshset writes white into every point's
     * base color, as model.src's does. the model's later meshsets draw from
     * those points, whatever their materials. SA_MAT_WHITE marks the material
     * whose meshset whitens, and a backend carries the white on from that
     * meshset. so a material an earlier meshset names too is drawn both ways.
     * the Ninja forms give every point the constant color instead, and Ninja's
     * own drawers shade each meshset alone */
    bits |= mode == SA_SHADE_PALETTE && !ninja && translucent && unlit
                ? SA_MAT_WHITE
                : 0;
    if (mode == SA_SHADE_LIGHTS) {
        /* the full drawer: a lit textured meshset adds its specular as the
         * offset. a face of a one-sided material that turns from the eye is
         * skipped */
        bits &= ~(uint32_t)(SA_MAT_DECAL | SA_MAT_OFFSET);
        bits |= texture != SA_CAP_NONE && !unlit &&
                        !(drawn & NJD_FLAG_IGNORE_SPECULAR)
                    ? SA_MAT_OFFSET
                    : 0;
        bits |= drawn & NJD_FLAG_DOUBLE_SIDE ? 0 : SA_MAT_CULL;
    }
    if (mode == SA_SHADE_RAMP) {
        const float *tint = sa1_light_tint();

        color = drawn & NJD_FLAG_USE_TEXTURE
                    ? argb(a, tint[1], tint[2], tint[3])
                    : argb(a, r * tint[1], g * tint[2], b * tint[3]);
    } else if (mode == SA_SHADE_LINEAR || mode == SA_SHADE_LIGHTS) {
        const NJS_ARGB *c = &state->material;

        if ((drawn & NJD_FLAG_USE_TEXTURE) &&
            (flags & NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL)) {
            r = g = b = 1.0;
        }
        if (flags & NJD_CONTROL_3D_CONSTANT_MATERIAL) {
            a = c->a;
            r = c->r;
            g = c->g;
            b = c->b;
        } else if (flags & NJD_CONTROL_3D_OFFSET_MATERIAL) {
            a += c->a;
            r += c->r;
            g += c->g;
            b += c->b;
        }
        color = argb(a, r, g, b);
    }
    out.texture = texture;
    out.specular = own_material->specular.color;
    out.flags = (uint16_t)bits;
    out.alpha = (uint8_t)point_alpha((diffuse >> 24) & 0xff, alpha_mode,
                                     state->material.a);
    out.filter = filter_of(drawn);
    out.src = (uint8_t)((drawn & TSP_SRC) >> TSP_SRC_SHIFT);
    out.dst = (uint8_t)((drawn & TSP_DST) >> TSP_DST_SHIFT);
    out.mip_bias = (uint8_t)((drawn & TSP_MIPMAP_D) >> TSP_MIPMAP_D_SHIFT);
    out.diffuse = color;
    if (mode == SA_SHADE_LIGHTS) {
        /* the highlight's step, as njCalcLightMaterials truncates it */
        float step = own_material->exponent;

        out.exponent = !(step >= 0.0F) ? 0
                       : step > 255.0F ? 255
                                       : (uint8_t)step;
    }
    return out;
}

/* how many materials a model's meshsets name. it is nbMat, or one past the
 * highest id a meshset names where that is more. the game's drawers take a
 * meshset's material from the list whatever the count says. Sky Deck's cannon
 * counts none and draws with its list's first */
static uint32_t materials_named(const NJS_MODEL *model)
{
    uint32_t count = model->nbMat;

    for (uint32_t i = 0; i < model->nbMeshset; i++) {
        uint32_t id = model->meshsets[i].type_matId & MATERIAL_ID;

        count = id >= count ? id + 1 : count;
    }
    return count;
}

/* the state a draw finds as it is called. header says where its header words
 * come from, and attrs are its constant attributes */
static struct draw_state state_now(enum sa1_header header,
                                   const struct sa1_attrs *attrs)
{
    struct draw_state state = {
        .texlist = _nj_current_texlist,
        .material = _nj_constant_material_,
        .attrs = attrs,
        /* the palette whose words the layer's pointer names, of the eight */
        .palette = (uint8_t)(((sa1_live_palette - sa1_live_colors) /
                              (SA_PALETTE_ENTRIES * 2)) &
                             7)};

    header_words(header, &state.isp, &state.tsp);
    return state;
}

/* the keys in meshes and lists of the mesh and the list of materials a draw
 * reads */
struct reads {
    uint64_t mesh, list;
};

/* the most materials a list holds: as many ids as follow an
 * sa_cap_materials in its record */
#define LIST_MOST ((SA_CAP_RECORD_MOST - sizeof(sa_cap_materials)) / 4)

/* give draw what a draw of model under state names: its mesh, materials, tint
 * and shade. reads takes the keys of its mesh and list. it returns 1, or 0
 * where the capture can't name it */
static int model_draw(const NJS_MODEL *model, uint8_t mode,
                      const struct draw_state *state, sa_draw *draw,
                      struct reads *reads)
{
    static uint32_t ids[LIST_MOST];
    static uint64_t keys[LIST_MOST];
    uint32_t mesh;
    uint32_t count = materials_named(model);
    double light[3] = {0, 0, 0};
    sa_shade shade = {.mode = mode};
    const NJS_ARGB *c = &state->material;
    const float *v = _nj_view_matrix_;

    /* a list one record can't hold is counted, and leaves the draw out
     * ahead of its mesh's name */
    if (count > LIST_MOST) {
        records_refused++;
        return 0;
    }
    mesh = mesh_of(model);
    if (mesh == SA_CAP_NONE) {
        return 0;
    }
    reads->mesh = meshes.named;
    const NJS_MATERIAL *first =
        &model->mats[model->meshsets->type_matId & MATERIAL_ID];
    uint32_t first_flags = flags_as_drawn(state->attrs, first->attrflags);
    uint32_t specular =
        !(first_flags & NJD_FLAG_IGNORE_SPECULAR) && first->specular.color != 0;

    for (uint32_t i = 0; i < count; i++) {
        ids[i] = material_of(
            material_as_drawn(&model->mats[i], mode, state, specular));
        keys[i] = materials.named;
    }
    if (mode == SA_SHADE_RAMP) {
        sa1_easy_light(light, &shade.ambient, &shade.diffuse);
    } else if (mode == SA_SHADE_LINEAR) {
        sa1_simple_light(light, &shade.ambient, &shade.diffuse);
    } else if (mode == SA_SHADE_PALETTE) {
        light[0] = sa1_live_light.x;
        light[1] = sa1_live_light.y;
        light[2] = sa1_live_light.z;
    }
    /* the light lies in Ninja's view now. it goes into the game's world as its
     * dot with each of the view's rows */
    for (int j = 0; j < 3; j++) {
        shade.light[j] = (float)dot3(v[4 * j], v[4 * j + 1], v[4 * j + 2],
                                     light[0], light[1], light[2]);
    }
    shade.palette = state->palette;
    shade.depth = (uint8_t)((state->isp & ISP_DEPTH) >> ISP_DEPTH_SHIFT);
    shade.fog = (uint8_t)((state->tsp & TSP_FOG) >> TSP_FOG_SHIFT);
    shade.flags =
        state->isp & ISP_Z_WRITE_DISABLE ? SA_SHADE_NO_DEPTH_WRITE : 0;
    draw->mesh = mesh;
    draw->materials = materials_of(ids, keys, count);
    reads->list = lists.named;
    draw->tint = argb(c->a, c->r, c->g, c->b);
    draw->shade = shade_of(shade);
    return 1;
}

/* the act's scene: the land the land's task holds. each entry it may draw is a
 * part, as is each model of its moving parts. a draw by the land's task finds
 * its part again by the model. a land's entries may share a model, so a part
 * keeps where it stood.
 *
 * a reader's part reads its mesh and its list of materials, and a release of
 * either ends it. so the tables hold just the two a part reads now. at first
 * those are what it was given with. after a kept picture draws it, they are
 * what that picture's first draw of it named. a draw in a dropped picture
 * never reaches a reader, and moves no hold. a reader copies shades and
 * materials as parts and lists are given. so they go as any name does. a
 * material named again finds its list by materials_of's key */
#define SCENE_PARTS 8192
_Static_assert(SCENE_PARTS < TABLE_SLOTS / 4 && SCENE_PARTS <= UINT16_MAX,
               "the scene's parts outgrow what a table holds for them");
struct part {
    const NJS_MODEL *model;
    uint32_t instance;
    uint32_t next; /* the next part of the model's chain, from 1, or 0 */
    float world[12];
    struct reads reads; /* held in the tables, each key table_hold's */
    struct reads drawn; /* by the open picture's first draw of it */
    int in_picture; /* TRUE once the open picture drew it */
};
static struct part parts[SCENE_PARTS];
static uint32_t part_heads[SCENE_PARTS]; /* a chain's first part, from 1 */
static uint32_t part_count;
static uint32_t parts_drawn[SCENE_PARTS]; /* in the open picture, from 0 */
static uint32_t parts_drawn_count;
/* TRUE from native_land_loaded's scene until native_land_released, and that
 * scene's name */
static int scene_stays;
static uint32_t scene_name;
static const struct task *scene_task; /* the land's task */

static uint32_t part_home(const NJS_MODEL *model)
{
    return (uint32_t)(((uintptr_t)model / 4) * 2654435761U) % SCENE_PARTS;
}

/* the part the land's task draws as it draws model at world, or null. of the
 * model's parts it is the one that stood nearest, and it moves to world */
static struct part *part_drawn(const NJS_MODEL *model, const float world[12])
{
    struct part *found = NULL;
    double least = 0;

    if (!scene_task || sa1_step_task != scene_task) {
        return NULL;
    }
    for (uint32_t at = part_heads[part_home(model)]; at != 0;
         at = parts[at - 1].next) {
        struct part *part = &parts[at - 1];
        double apart = 0;

        if (part->model != model) {
            continue;
        }
        for (int i = 0; i < 12; i++) {
            double d = (double)part->world[i] - world[i];

            apart += d * d;
        }
        if (!found || apart < least) {
            found = part;
            least = apart;
        }
    }
    if (found) {
        memcpy(found->world, world, 12 * sizeof world[0]);
    }
    return found;
}

/* the open picture ends, kept or dropped. where it is kept, each part it drew
 * holds what the picture's first draw of it named. the part lets go of what it
 * held before */
static void parts_settle(int kept)
{
    for (uint32_t i = 0; i < parts_drawn_count; i++) {
        struct part *part = &parts[parts_drawn[i]];
        struct reads before = part->reads;

        part->in_picture = 0;
        if (kept) {
            part->reads.mesh = table_hold(&meshes, part->drawn.mesh);
            part->reads.list = table_hold(&lists, part->drawn.list);
            table_release(&meshes, before.mesh);
            table_release(&lists, before.list);
        }
    }
    parts_drawn_count = 0;
}

void sa1_record_model(const NJS_MODEL *model, uint8_t mode,
                      enum sa1_header header, const struct sa1_attrs *attrs,
                      uint16_t flags)
{
    struct part *part;
    struct reads reads;
    sa_draw draw = {.payload = SA_NO_PAYLOAD};

    if (!(flags & SA_DRAW_OUT)) {
        port_draws_sent++;
    }
    if (!recording) {
        return;
    }
    if (!picture_open) {
        begin_picture();
    }
    struct draw_state state = state_now(header, attrs);

    to_world(draw.object_to_world);
    part = part_drawn(model, draw.object_to_world);
    if (!model_draw(model, mode, &state, &draw, &reads)) {
        picture_lost++;
        return;
    }
    draw.flags = flags;
    /* the task is born to a reader by its first draw, a part's too */
    draw.instance = instance();
    /* a part's draw takes the part's own instance, which names it whole. any
     * other draw is its drawer's nth of the model's shape */
    if (part) {
        draw.instance = part->instance;
    } else {
        draw.thing = shape_of(model);
        if (!nth_of(draw.instance, draw.thing, &draw.nth)) {
            picture_lost++;
            return;
        }
    }
    draw.view = view_id(1);
    if (draw.view == 0) {
        picture_lost++;
        return;
    }
    if (part && !part->in_picture) {
        part->in_picture = 1;
        part->drawn = reads;
        parts_drawn[parts_drawn_count++] = (uint32_t)(part - parts);
    }
    emit(SA_REC_DRAW, &draw, sizeof draw, NULL, 0);
    picture_draws++;
}

/* a place as Ninja's rows hold one. a point goes as p * M. rows 0 to 2 are the
 * axes, and row 3 is the origin. each move goes ahead of M, as Ninja's
 * njTranslate, njRotateX to Z and njScale make theirs */
struct place {
    double m[4][3];
};
/* the place that moves nothing */
static const struct place origin = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};

static void place_translate(struct place *place, const Float *at)
{
    for (int j = 0; j < 3; j++) {
        place->m[3][j] += at[0] * place->m[0][j] + at[1] * place->m[1][j] +
                          at[2] * place->m[2][j];
    }
}

/* turn about axis by angle. sa_fsca gives turn the sine, then the cosine */
static void place_turn(struct place *place, int axis, Angle angle)
{
    int a = (axis + 1) % 3;
    int b = (axis + 2) % 3;
    float turn[2];

    sa_fsca((uint32_t)angle, turn);
    for (int j = 0; j < 3; j++) {
        double first = place->m[a][j];
        double second = place->m[b][j];

        place->m[a][j] = turn[1] * first + turn[0] * second;
        place->m[b][j] = turn[1] * second - turn[0] * first;
    }
}

/* njRotateXYZ's turn, or njRotateZXY's */
static void place_rotate(struct place *place, const Angle *ang, int zxy)
{
    static const int orders[2][3] = {{2, 1, 0}, {1, 0, 2}};

    for (int i = 0; i < 3; i++) {
        place_turn(place, orders[zxy][i], ang[orders[zxy][i]]);
    }
}

static void place_scale(struct place *place, const Float *scale)
{
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            place->m[i][j] *= scale[i];
        }
    }
}

/* give a reader model as a part of the scene, under state, at place. number
 * names the part in the scene. a part the capture can't name, or one past
 * SCENE_PARTS, counts in lost */
static void part_give(const NJS_MODEL *model, const struct draw_state *state,
                      const struct place *place, uint32_t number,
                      uint16_t flags, uint32_t *lost)
{
    struct part *part = &parts[part_count];
    uint32_t home = part_home(model);
    struct reads reads;
    sa_draw draw = {.payload = SA_NO_PAYLOAD,
                    .flags = flags,
                    .instance = SA_INSTANCE_SCENE | number};

    if (part_count == SCENE_PARTS ||
        !model_draw(model, SA_SHADE_PALETTE, state, &draw, &reads)) {
        (*lost)++;
        return;
    }
    for (int j = 0; j < 3; j++) {
        for (int k = 0; k < 4; k++) {
            draw.object_to_world[4 * j + k] = (float)place->m[k][j];
        }
    }
    *part = (struct part){
        .model = model, .instance = draw.instance, .next = part_heads[home]};
    memcpy(part->world, draw.object_to_world, sizeof part->world);
    part->reads.mesh = table_hold(&meshes, reads.mesh);
    part->reads.list = table_hold(&lists, reads.list);
    part_heads[home] = ++part_count;
    emit(SA_REC_DRAW, &draw, sizeof draw, NULL, 0);
}

/* a motion at a frame, as src/draw/action.c's draw_motion walks it. a keyed
 * node's record is count key pointers, one for each factor, then each one's
 * count of keys. type says which of position, angle and scale the factors are.
 * node is the node whose record comes next */
struct pose {
    const uint32_t *records;
    uint32_t count, type, node;
    float frame;
    int zxy; /* the root's order of turns, which every keyed node takes */
};

/* put a factor's three values at the pose's frame in out, as action.c's
 * action_find_key and action_sample_vector do. the last key at or before the
 * frame blends toward the next. after the last key, the next is the first. it
 * returns 1, or 0 where the node has no key of the factor. a key is its frame,
 * then three words */
static int pose_sample(const struct pose *pose, uint32_t factor, int angles,
                       double out[3])
{
    const uint32_t *record = pose->records + 2 * pose->count * pose->node;
    const uint32_t *keys = native_widen(record[factor]);
    uint32_t count = record[pose->count + factor];
    uint32_t low = 0;
    uint32_t high = count;
    double weight;
    int32_t span;

    if (!keys || count == 0) {
        return 0;
    }
    while (high - low - 1 > 0) {
        uint32_t middle = (high + low) >> 1;

        if ((uint32_t)(int32_t)pose->frame >= keys[4 * middle]) {
            low = middle;
        } else {
            high = middle;
        }
    }
    const uint32_t *key = &keys[4 * low];
    const uint32_t *next = low + 1 < count ? key + 4 : keys;

    weight = (double)pose->frame - (double)(int32_t)key[0];
    span = (int32_t)(next[0] - key[0]);
    if (span > 0) {
        weight /= span;
    }
    for (int i = 0; i < 3; i++) {
        float from, to;

        if (angles) {
            from = (float)(int32_t)key[1 + i];
            to = (float)(int32_t)next[1 + i];
        } else {
            memcpy(&from, &key[1 + i], sizeof from);
            memcpy(&to, &next[1 + i], sizeof to);
        }
        out[i] = from * (1.0 - weight) + to * weight;
    }
    return 1;
}

/* give each model of object's tree as a moving part of the scene. a part
 * stands where pose places its node under parent, as action.c's
 * action_draw_motion_tree does. a node under NJD_EVAL_SKIP takes its own
 * position, angles and scale, and its own order of turns. any other takes each
 * from its keys, or its own where it has none, and the root's order */
static void parts_of_tree(const NJS_OBJECT *object, struct pose *pose,
                          const struct place *parent,
                          const struct draw_state *state, uint32_t *number,
                          uint32_t *lost)
{
    for (; object; object = object->sibling) {
        struct place place = *parent;
        uint32_t flags = object->evalflags;
        int keyed = !(flags & NJD_EVAL_SKIP) && pose->count != 0;
        uint32_t factor = 0;
        double value[3];
        Float at[3];
        Angle ang[3];

        memcpy(at, object->pos, sizeof at);
        if (keyed && (pose->type & NJD_MTYPE_POS_0) &&
            pose_sample(pose, factor++, 0, value)) {
            for (int i = 0; i < 3; i++) {
                at[i] = (Float)value[i];
            }
        }
        place_translate(&place, at);
        memcpy(ang, object->ang, sizeof ang);
        if (keyed && (pose->type & NJD_MTYPE_ANG_1) &&
            pose_sample(pose, factor++, 1, value)) {
            for (int i = 0; i < 3; i++) {
                ang[i] = (Angle)(float)value[i];
            }
        }
        place_rotate(&place, ang,
                     flags & NJD_EVAL_SKIP ? (flags & NJD_EVAL_ZXY_ANG) != 0
                                           : pose->zxy);
        memcpy(at, object->scl, sizeof at);
        if (keyed && (pose->type & NJD_MTYPE_SCL_2) &&
            pose_sample(pose, factor, 0, value)) {
            for (int i = 0; i < 3; i++) {
                at[i] = (Float)value[i];
            }
        }
        place_scale(&place, at);
        pose->node += keyed;
        if (object->model && !(flags & NJD_EVAL_HIDE)) {
            part_give(object->model, state, &place, (*number)++, SA_DRAW_MOVING,
                      lost);
        }
        parts_of_tree(object->child, pose, &place, state, number, lost);
    }
}

/* card.c's sources_file, as far as the scene's lights read it, laid out as a
 * sources file holds it. the act's sources come first, then its light's turn,
 * color and strengths */
struct act_lights {
    struct {
        int32_t used;
        float pos[3];
        float range, rate;
        float color[3];
        float touch_pos[3];
    } sources[SA_LIGHTING_SOURCES];
    int32_t ry, rz;
    float color[3];
    float specular, diffuse, ambient;
};
_Static_assert(offsetof(struct act_lights, ry) == 0x5a0,
               "the turn starts 0x5a0 into a sources file");

/* give the scene's lights, as sa_sa1.h says: the act's light and the sources
 * its file marks used. light_load_act filled them before the land's task took
 * the land. the light shines straight down, turned as light_load_act turns
 * it */
static void scene_lighting(void)
{
    const struct act_lights *act =
        (const struct act_lights *)statics[ACT_LIGHTS];
    struct place turn = origin;
    sa_lighting lighting = {.count = 0};

    place_turn(&turn, 1, act->ry);
    place_turn(&turn, 2, act->rz);
    for (int j = 0; j < 3; j++) {
        lighting.direction[j] = (float)-turn.m[1][j];
        lighting.color[j] = act->color[j];
    }
    lighting.specular = act->specular;
    lighting.diffuse = act->diffuse;
    lighting.ambient = act->ambient;
    for (int i = 0; i < SA_LIGHTING_SOURCES; i++) {
        sa_light_source *source = &lighting.source[lighting.count];

        if (!act->sources[i].used) {
            continue;
        }
        memcpy(source->point, act->sources[i].pos, sizeof source->point);
        source->range = act->sources[i].range;
        memcpy(source->color, act->sources[i].color, sizeof source->color);
        source->rate = act->sources[i].rate;
        lighting.count++;
    }
    emit(SA_CAP_SCENE_LIGHTING, &lighting, sizeof lighting, NULL, 0);
}

/* whether the run's pictures are recorded now: a window's are, and a
 * capture's while its stage plays */
static int recorded(void)
{
    return native_picture_sink ||
           (capture && stage_current == native_target_stage);
}

/* the land's task took land, its act's land, as its step is about to draw it.
 * name is the game's name for the act, and gray the gray of its constant
 * material. the adapter gives the land whole as the act's scene, its lights
 * first. each entry the task may draw is a part, numbered by its place in the
 * table. the moving parts' models follow, numbered on in the order the task
 * draws them. each takes world_land_run's constant material and palette, and
 * the texlist world_land_draw_visible sets or a moving part's own. the rest of
 * its state is as the call finds it. recording changes only at a picture's
 * end, so the scene asks recorded() itself */
void native_land_loaded(const struct land_table *land, unsigned int name,
                        float gray)
{
    struct sa1_attrs attrs;
    struct draw_state state;
    sa_scene scene = {.scene = name};
    sa_cap_scene_end end = {.scene = name};
    uint32_t number = (uint32_t)land->count;

    if (scene_stays || !recorded()) {
        return;
    }
    attrs = sa1_attrs_now();
    state = state_now(SA1_CONTEXT, &attrs);
    emit(SA_CAP_SCENE, &scene, sizeof scene, NULL, 0);
    scene_stays = 1;
    scene_name = name;
    scene_task = sa1_step_task;
    scene_lighting();
    state.material = (NJS_ARGB){0.0F, gray, gray, gray};
    state.palette = 0;
    if (land->flags & (LAND_TEXLIST | LAND_PVM)) {
        state.texlist = land->texlist;
    }
    for (int i = 0; i < land->count; i++) {
        const struct land_entry *entry = &land->entries[i];
        const NJS_OBJECT *object = entry->object;
        struct place place = origin;

        if (!(entry->flags & LAND_DRAWN) || !object || !object->model) {
            continue;
        }
        place_translate(&place, object->pos);
        place_rotate(&place, object->ang, 0);
        part_give(object->model, &state, &place, (uint32_t)i, 0, &end.lost);
    }
    for (int i = 0; (land->flags & LAND_MOTIONS) && i < land->motion_count;
         i++) {
        const struct land_motion *moving = &land->motions[i];
        const NJS_OBJECT *object = land->flags & LAND_FILE
                                       ? moving->object
                                       : moving->play.action->object;
        const NJS_MOTION *motion = land->flags & LAND_FILE
                                       ? moving->play.motion
                                       : moving->play.action->motion;
        struct pose pose = {.frame = moving->frame,
                            .zxy = (object->evalflags & NJD_EVAL_ZXY_ANG) != 0};

        /* at a frame at or past the motion's last, the game's motion draws
         * nothing. here its part takes no key, and stands as its nodes do */
        if (motion && moving->frame < (float)(int32_t)motion->nbFrame) {
            pose.records = motion->mdata;
            pose.count = motion->inp_fn & ~(uint32_t)NJD_MTYPE_MASK;
            pose.type = motion->type;
        }
        state.texlist = moving->texlist;
        parts_of_tree(object, &pose, &origin, &state, &number, &end.lost);
    }
    end.parts = part_count;
    emit(SA_CAP_SCENE_END, &end, sizeof end, NULL, 0);
}

/* the land's task lets its land go. the scene ends, and what its parts read
 * may be forgotten as any name is */
void native_land_released(void)
{
    sa_scene ended = {.scene = scene_name};

    if (!scene_stays) {
        return;
    }
    emit(SA_CAP_SCENE_ENDED, &ended, sizeof ended, NULL, 0);
    scene_stays = 0;
    scene_task = NULL;
    part_count = 0;
    parts_drawn_count = 0;
    memset(part_heads, 0, sizeof part_heads);
    memset(meshes.holds, 0, sizeof meshes.holds);
    memset(lists.holds, 0, sizeof lists.holds);
}

/* Ninja's attributes of a sprite, NJD_SPRITE_ANGLE to NJD_SPRITE_ALPHA, each
 * the bit of the SA_SPRITE_ flag of its name */
#define SPRITE_ATTRS                                                           \
    (NJD_SPRITE_ANGLE | NJD_SPRITE_COLOR | NJD_SPRITE_HFLIP |                  \
     NJD_SPRITE_VFLIP | NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA)

void sa1_record_sprite(const NJS_SPRITE *sprite, int entry, uint32_t attr,
                       float y_scale, float bias, int screen)
{
    static const float identity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    const NJS_TEXANIM *frame;
    const NJS_ARGB *c = &_nj_constant_material_;
    uint32_t isp, tsp;
    sa_sprite out = {0};

    port_draws_sent++;
    if (!recording) {
        return;
    }
    if (!picture_open) {
        begin_picture();
    }
    out.view = view_id(!screen);
    if (out.view == 0) {
        picture_lost++;
        return;
    }
    frame = &sprite->tanim[entry];
    if (screen) {
        memcpy(out.object_to_world, identity, sizeof identity);
        /* where the 2D draws squeeze, x squeezes after the turn, about
         * port_wide_x's pivot, as the Dreamcast's sprite_send_2d does */
        if (port_wide_squeezes()) {
            out.object_to_world[0] = port_draw_aspect;
            out.object_to_world[3] = port_wide_x(0.0F);
        }
    } else {
        to_world(out.object_to_world);
    }
    header_words(SA1_CONTEXT, &isp, &tsp);
    out.texture = texture_of(sprite->tlist, (uint32_t)(int32_t)frame->texid);
    out.instance = instance();
    out.at[0] = sprite->p.x;
    out.at[1] = sprite->p.y;
    /* a sprite on the screen takes its depth from the bias, and no z */
    out.at[2] = screen ? 0.0F : sprite->p.z;
    out.scale[0] = sprite->sx;
    out.scale[1] = y_scale;
    out.corner[0][0] = (float)-frame->cx;
    out.corner[0][1] = (float)-frame->cy;
    out.corner[1][0] = (float)(frame->sx - frame->cx);
    out.corner[1][1] = (float)(frame->sy - frame->cy);
    out.uv[0][0] = (float)((double)frame->u1 / 256.0);
    out.uv[0][1] = (float)((double)frame->v1 / 256.0);
    out.uv[1][0] = (float)((double)frame->u2 / 256.0);
    out.uv[1][1] = (float)((double)frame->v2 / 256.0);
    out.angle = (float)((double)((uint32_t)sprite->ang & 0xffff) *
                        (2 * M_PI / 65536.0));
    out.bias = bias;
    out.color = argb(c->a, c->r, c->g, c->b);
    out.flags =
        (uint8_t)((attr & SPRITE_ATTRS) | (screen ? SA_SPRITE_SCREEN : 0));
    out.depth = (uint8_t)((isp & ISP_DEPTH) >> ISP_DEPTH_SHIFT);
    out.fog = (uint8_t)((tsp & TSP_FOG) >> TSP_FOG_SHIFT);
    out.src = (uint8_t)((tsp & TSP_SRC) >> TSP_SRC_SHIFT);
    out.dst = (uint8_t)((tsp & TSP_DST) >> TSP_DST_SHIFT);
    out.filter = filter_of(tsp);
    emit(SA_CAP_SPRITE, &out, sizeof out, NULL, 0);
    picture_draws++;
}

/* a texture by its global index, as Ninja's texture memory finds it. the
 * current list's entry comes first, then any texture keyed by its texture
 * memory. E-101 mkII's jets draw a texture their list doesn't hold. it returns
 * SA_CAP_NONE for none */
static uint32_t texture_global(uint32_t global_index)
{
    const NJS_TEXLIST *list = _nj_current_texlist;

    for (uint32_t i = 0; list && i < list->nbTexture; i++) {
        const NJS_TEXMEMLIST *memory = native_widen(list->textures[i].texaddr);

        if (memory && memory->globalIndex == global_index) {
            return texture_of(list, i);
        }
    }
    for (uint32_t i = 0; i < TABLE_SLOTS; i++) {
        uint64_t key = textures.keys[i];

        if (key != 0 && key != TABLE_GONE && key >> 32 != INDEX_ALONE &&
            (uint32_t)key == global_index) {
            textures.used[i] = picture_number;
            return textures.ids[i];
        }
    }
    return SA_CAP_NONE;
}

/* the current texture, which Ninja's triangles take by global index */
static uint32_t current_texture(void)
{
    return texture_global(_nj_current_texture);
}

/* how triangles' pixels meet the picture: the depth test, fog, blend and
 * filter. they come from the header words in the context */
static void triangles_header(sa_triangles *out)
{
    uint32_t isp, tsp;

    header_words(SA1_CONTEXT, &isp, &tsp);
    out->depth = (uint8_t)((isp & ISP_DEPTH) >> ISP_DEPTH_SHIFT);
    out->fog = (uint8_t)((tsp & TSP_FOG) >> TSP_FOG_SHIFT);
    out->src = (uint8_t)((tsp & TSP_SRC) >> TSP_SRC_SHIFT);
    out->dst = (uint8_t)((tsp & TSP_DST) >> TSP_DST_SHIFT);
    out->filter = filter_of(tsp);
}

/* how many of left triangles or lines of form the next record takes. it takes
 * all where their points fit a record, else the most that fit. a strip split
 * across records takes an even count, as sa_capture.h says */
static uint32_t triangles_part(uint8_t form, uint32_t left)
{
    uint32_t most = (uint32_t)SA_CAP_POINTS_MOST;

    if (sa_triangles_points(form, left) <= most) {
        return left;
    }
    return form == SA_TRIANGLES_LIST    ? most / 3
           : form == SA_TRIANGLES_FAN   ? (most - 1) / 2
           : form == SA_TRIANGLES_LINES ? most / 2
                                        : (most - 2) & ~1U;
}

/* put out's triangles or lines over points into SA_CAP_TRIANGLES records, as
 * many as they need, in order. each record is out but for its count. a strip's
 * next record opens with the two points the one before closed with. a fan's
 * next opens with the fan's first point, written into points just ahead of the
 * record's own. points is the caller's scratch, and the point that write
 * replaces has gone already */
static void triangles_emit(sa_triangles *out, sa_point *points)
{
    /* the points a triangle or a line takes past the one before it */
    uint32_t step = out->form == SA_TRIANGLES_LIST    ? 3
                    : out->form == SA_TRIANGLES_STRIP ? 1
                                                      : 2;
    uint32_t count = out->count;
    uint32_t first = 0;

    do {
        uint32_t part = triangles_part(out->form, count - first);
        sa_point *from = points + step * first;

        if (out->form == SA_TRIANGLES_FAN && first != 0) {
            *from = points[0];
        }
        out->count = (uint16_t)part;
        emit(SA_CAP_TRIANGLES, out, sizeof *out, from,
             sa_triangles_points(out->form, part) * (uint32_t)sizeof *from);
        picture_draws++;
        first += part;
    } while (first < count);
}

void sa1_record_triangles(const NJS_POINT3COL *p, int count, uint32_t attr)
{
    static sa_point points[1 << 12];
    int form = attr & NJD_DRAW_CONNECTED ? SA_TRIANGLES_STRIP
               : attr & NJD_DRAW_FAN     ? SA_TRIANGLES_FAN
                                         : SA_TRIANGLES_LIST;
    int total = form == SA_TRIANGLES_STRIP ? count + 2
                : form == SA_TRIANGLES_FAN ? 2 * count + 1
                                           : 3 * count;
    int textured = (attr & NJD_USE_TEXTURE) != 0;
    sa_triangles out = {0};

    port_draws_sent++;
    if (!recording) {
        return;
    }
    if (!picture_open) {
        begin_picture();
    }
    if (!(attr & NJD_FILL) || count < 0 ||
        total > (int)(sizeof points / sizeof points[0])) {
        picture_lost++;
        return;
    }
    out.view = view_id(1);
    if (out.view == 0) {
        picture_lost++;
        return;
    }
    to_world(out.object_to_world);
    out.texture = textured ? current_texture() : SA_CAP_NONE;
    out.instance = instance();
    out.count = (uint16_t)count;
    out.form = (uint8_t)form;
    out.flags = (uint8_t)((textured ? SA_TRIANGLES_TEXTURE : 0) |
                          (attr & NJD_TRANSPARENT ? SA_TRIANGLES_BLEND : 0));
    triangles_header(&out);
    for (int i = 0; i < total; i++) {
        memcpy(points[i].at, &p->p[i], sizeof points[i].at);
        points[i].uv[0] =
            textured ? (float)((double)p->tex[i].tex.u / 256.0) : 0.0F;
        points[i].uv[1] =
            textured ? (float)((double)p->tex[i].tex.v / 256.0) : 0.0F;
        points[i].color = p->col[i].color;
    }
    triangles_emit(&out, points);
}

/* njDrawLine3D's n lines as they leave Kamui's context: each line's two points
 * in the object's space. the backend projects and widens them as Ninja does */
void sa1_record_lines(const NJS_POINT3COL *p, int n, uint32_t attr)
{
    static sa_point points[1 << 12];
    int textured = (attr & NJD_USE_TEXTURE) != 0;
    int step = attr & NJD_DRAW_MASK ? 1 : 2;
    sa_triangles out = {0};

    port_draws_sent++;
    if (!recording) {
        return;
    }
    if (!picture_open) {
        begin_picture();
    }
    if (n < 1 || 2 * n > (int)(sizeof points / sizeof points[0])) {
        picture_lost++;
        return;
    }
    out.view = view_id(1);
    if (out.view == 0) {
        picture_lost++;
        return;
    }
    to_world(out.object_to_world);
    out.texture = textured ? current_texture() : SA_CAP_NONE;
    out.instance = instance();
    out.count = (uint16_t)n;
    out.form = SA_TRIANGLES_LINES;
    out.flags = (uint8_t)((textured ? SA_TRIANGLES_TEXTURE : 0) |
                          (attr & NJD_TRANSPARENT ? SA_TRIANGLES_BLEND : 0));
    triangles_header(&out);
    for (int k = 0; k < 2 * n; k++) {
        int i = k % 2                 ? (k / 2) * step + 1
                : attr & NJD_DRAW_FAN ? 0
                                      : (k / 2) * step;

        memcpy(points[k].at, &p->p[i], sizeof points[k].at);
        points[k].uv[0] =
            textured ? (float)((double)p->tex[i].tex.u / 256.0) : 0.0F;
        points[k].uv[1] =
            textured ? (float)((double)p->tex[i].tex.v / 256.0) : 0.0F;
        points[k].color = p->col[i].color;
    }
    triangles_emit(&out, points);
}

/* the most corners of a polygon the Dreamcast's layer squeezes,
 * njDrawPolygon2D.c's CORNERS */
#define SQUEEZED_CORNERS 32

/* njDrawPolygon2D's filled polygon, as a strip on the screen of its corners
 * from front and back in turn. x and y are in pixels. z is the depth 1/z
 * stands for: -pri where pri is below -1, and 1 otherwise. an outline is lost.
 * where the 2D draws squeeze, x squeezes as the Dreamcast's layer squeezes it.
 * a polygon of over SQUEEZED_CORNERS corners keeps its x, as does an
 * untextured one covering the screen */
void sa1_record_polygon_2d(const NJS_POINT2COL *p, int n, float pri,
                           uint32_t attr)
{
    static sa_point points[64];
    int textured = (attr & NJD_USE_TEXTURE) != 0;
    int squeeze;
    int front = 0;
    int back = n - 1;
    sa_triangles out = {0};

    port_draws_sent++;
    if (!recording) {
        return;
    }
    if (!picture_open) {
        begin_picture();
    }
    if (!(attr & NJD_FILL) || n < 3 ||
        n > (int)(sizeof points / sizeof points[0])) {
        picture_lost++;
        return;
    }
    out.view = view_id(0);
    squeeze = n <= SQUEEZED_CORNERS && port_wide_squeezes() &&
              (textured ||
               !port_wide_covers(&p->p->x, n, sizeof *p->p / sizeof(Float)));
    out.object_to_world[0] = out.object_to_world[5] = out.object_to_world[10] =
        1.0F;
    out.texture = textured ? current_texture() : SA_CAP_NONE;
    out.instance = instance();
    out.count = (uint16_t)(n - 2);
    out.form = SA_TRIANGLES_STRIP;
    out.flags =
        (uint8_t)(SA_TRIANGLES_SCREEN | (textured ? SA_TRIANGLES_TEXTURE : 0) |
                  (attr & NJD_TRANSPARENT ? SA_TRIANGLES_BLEND : 0));
    triangles_header(&out);
    for (int k = 0; k < n; k++) {
        int i = k % 2 ? back-- : front++;

        points[k].at[0] = squeeze ? port_wide_x(p->p[i].x) : p->p[i].x;
        points[k].at[1] = p->p[i].y;
        points[k].at[2] = pri < -1.0F ? -pri : 1.0F;
        points[k].uv[0] =
            textured ? (float)((double)p->tex[i].tex.u / 256.0) : 0.0F;
        points[k].uv[1] =
            textured ? (float)((double)p->tex[i].tex.v / 256.0) : 0.0F;
        points[k].color = p->col[i].color;
    }
    triangles_emit(&out, points);
}

/* a texture by its global index, as njDrawTexture names it. texture_global
 * finds it first, else it is named by the index alone */
static uint32_t texture_by_index(uint32_t global_index)
{
    uint32_t id = texture_global(global_index);

    if (id != SA_CAP_NONE) {
        return id;
    }
    if (table_name(&textures, (uint64_t)INDEX_ALONE << 32 | global_index,
                   &id)) {
        sa_cap_texture texture = {.texture = id, .global_index = global_index};

        emit(SA_CAP_TEXTURE, &texture, sizeof texture, NULL, 0);
    }
    return id;
}

/* njDrawTexture's and njDrawPolygon's strip, under the header words the call
 * finds. each vertex is x, y and 1/z, then u and v for a texture's, then the
 * color. it goes as a strip on the screen whose z is the depth 1/z stands for.
 * where the 2D draws squeeze, x squeezes as the Dreamcast's layer squeezes it.
 * an untextured strip that covers the screen keeps its x */
void sa1_record_screen_strip(const void *vertices, int count, int textured,
                             uint32_t global_index, int blend)
{
    static sa_point points[1 << 12];
    const NJS_POLYGON_VTX *first = vertices;
    int squeeze;
    sa_triangles out = {0};

    port_draws_sent++;
    if (!recording) {
        return;
    }
    if (!picture_open) {
        begin_picture();
    }
    if (count < 3 || count > (int)(sizeof points / sizeof points[0])) {
        picture_lost++;
        return;
    }
    out.view = view_id(0);
    squeeze = port_wide_squeezes() &&
              (textured || !port_wide_covers(&first->x, count,
                                             sizeof *first / sizeof(Float)));
    for (int i = 0; i < count; i++) {
        const NJS_TEXTURE_VTX *t = (const NJS_TEXTURE_VTX *)vertices + i;
        const NJS_POLYGON_VTX *v = (const NJS_POLYGON_VTX *)vertices + i;
        float ooz = textured ? t->z : v->z;

        points[i].at[0] = textured ? t->x : v->x;
        if (squeeze) {
            points[i].at[0] = port_wide_x(points[i].at[0]);
        }
        points[i].at[1] = textured ? t->y : v->y;
        points[i].at[2] = ooz > 0.0F ? (float)(1.0 / (double)ooz) : 1.0F;
        points[i].uv[0] = textured ? t->u : 0.0F;
        points[i].uv[1] = textured ? t->v : 0.0F;
        points[i].color = textured ? t->col : v->col;
    }
    out.object_to_world[0] = out.object_to_world[5] = out.object_to_world[10] =
        1.0F;
    out.texture = textured ? texture_by_index(global_index) : SA_CAP_NONE;
    out.instance = instance();
    out.count = (uint16_t)(count - 2);
    out.form = SA_TRIANGLES_STRIP;
    out.flags =
        (uint8_t)(SA_TRIANGLES_SCREEN | (textured ? SA_TRIANGLES_TEXTURE : 0) |
                  (blend ? SA_TRIANGLES_BLEND : 0));
    triangles_header(&out);
    triangles_emit(&out, points);
}

/* the overlay's text, one square of side pixels for each character from x, y,
 * in color. its glyphs come from font, the texels of Ninja's built-in font as
 * njInitPrint loads them. it lies over all else, as the Dreamcast's print at
 * 1/w 1 lies over what drew before */
void sa1_record_text(const char *text, float x, float y, float side,
                     uint32_t color, const void *font)
{
    /* the font's key: global index -2, which no texture takes. Ninja's -2
     * marks its texture to be set again */
    static const uint64_t font_key = (uint64_t)INDEX_ALONE << 32 | 0xfffffffeU;
    /* CHARACTERS is the most a call records. the rest describe Ninja's font:
     * glyphs to a row, PVR type and layout, and side in texels */
    enum {
        CHARACTERS = 64,
        ROW = 16,
        ARGB1555 = 0,
        TWIDDLED = 1,
        FONT_SIDE = 128
    };
    static sa_point points[6 * CHARACTERS];
    static const int corners[6][2] = {{0, 0}, {0, 1}, {1, 0},
                                      {1, 0}, {0, 1}, {1, 1}};
    int count = (int)strlen(text);
    uint32_t id;
    sa_triangles out = {0};

    port_draws_sent += (uint32_t)count;
    if (!recording) {
        return;
    }
    if (!picture_open) {
        begin_picture();
    }
    out.view = view_id(0);
    if (count > CHARACTERS) {
        picture_lost++;
        count = CHARACTERS;
    }
    if (table_name(&textures, font_key, &id)) {
        sa_cap_texture texture = {.texture = id,
                                  .global_index = (uint32_t)font_key};

        emit(SA_CAP_TEXTURE, &texture, sizeof texture, NULL, 0);
        texels_emit(id, ARGB1555, TWIDDLED, FONT_SIDE, FONT_SIDE, font);
    }
    for (int i = 0; i < count; i++) {
        uint8_t glyph = (uint8_t)text[i];

        for (int k = 0; k < 6; k++) {
            sa_point *p = &points[6 * i + k];

            p->at[0] = x + side * (float)(i + corners[k][0]);
            p->at[1] = y + side * (float)corners[k][1];
            p->at[2] = 1.0F;
            p->uv[0] = (float)(glyph % ROW + corners[k][0]) / ROW;
            p->uv[1] = (float)(glyph / ROW + corners[k][1]) / ROW;
            p->color = color;
        }
    }
    out.object_to_world[0] = out.object_to_world[5] = out.object_to_world[10] =
        1.0F;
    out.texture = id;
    out.instance = instance();
    out.count = (uint16_t)(2 * count);
    out.form = SA_TRIANGLES_LIST;
    out.flags = SA_TRIANGLES_SCREEN | SA_TRIANGLES_TEXTURE | SA_TRIANGLES_BLEND;
    out.depth = SA_DEPTH_ALWAYS;
    out.fog = SA_FOG_NONE;
    out.src = SA_BLEND_SRC_ALPHA;
    out.dst = SA_BLEND_INV_SRC_ALPHA;
    out.filter = SA_FILTER_POINT;
    triangles_emit(&out, points);
}

/* forget the open picture's draws, and keep what they named, which the next
 * picture's records carry */
static void sa1_picture_drop(void)
{
    size_t kept = 0;

    if (!picture_open) {
        return;
    }
    int in_scene = 0;

    for (size_t at = 0; at + sizeof(sa_record_head) <= records_used;) {
        sa_record_head head;

        memcpy(&head, records + at, sizeof head);
        /* a scene given while the picture was open stays, its parts too */
        in_scene = head.kind == SA_CAP_SCENE       ? 1
                   : head.kind == SA_CAP_SCENE_END ? 0
                                                   : in_scene;
        if (in_scene ||
            (head.kind != SA_CAP_PICTURE && head.kind != SA_REC_VIEW &&
             head.kind != SA_REC_DRAW && head.kind != SA_CAP_SPRITE &&
             head.kind != SA_CAP_TRIANGLES)) {
            memmove(records + kept, records + at, head.size);
            kept += head.size;
        }
        at += head.size;
    }
    records_used = kept;
    picture_open = 0;
    last_tick = tick_before;
    parts_settle(0);
}

void __real_frame_wait(void);
int32_t __real_kmRender(void);
int sa1_frame_shown;

/* the picture ends as frame_wait begins, with the overlay over it. the next
 * picture begins with its first draw. a frame draw_off holds back, as on a
 * skip or a load, is dropped. so is a split frame's second picture that
 * present.c dropped. on the Dreamcast, frame_wait's reset of Kamui's lists
 * drops both, and the picture before stays. the Dreamcast draws the overlay in
 * frame_wait's frame end, after the wait. the PC's picture doesn't wait. until
 * frame_wait returns, sa1_frame_shown is TRUE and the overlay draws nothing */
void __wrap_frame_wait(void)
{
    if (draw_off == TRUE || (split_shown && present_shown(0) != SHOWN_BOTH)) {
        sa1_picture_drop();
    } else {
        overlay_draw();
        sa1_picture_end();
    }
    split_shown = 0;
    sa1_frame_shown = 1;
    __real_frame_wait();
    sa1_frame_shown = 0;
}

/* a split frame's first picture is over as its render begins, in
 * present_step. frame_wait's render finds no picture open */
int32_t __wrap_kmRender(void)
{
    if (picture_open) {
        sa1_picture_end();
        split_shown = 1;
    }
    return __real_kmRender();
}

void sa1_picture_end(void)
{
    if (picture_open) {
        sa_cap_end end = {.picture = picture_number,
                          .draws = picture_draws,
                          .lost = picture_lost};

        picture_view_settle();
        picture_back_settle();
        emit(SA_CAP_END, &end, sizeof end, NULL, 0);
        picture_number++;
        picture_open = 0;
        parts_settle(1);
        if (picture_number % AGE_EVERY == 0) {
            table_age(&meshes, MESH_KEEP);
            table_age(&lists, MESH_KEEP);
            table_age(&materials, MESH_KEEP);
            table_age(&shades, MESH_KEEP);
        }
    }
    if (capture && records_used) {
        fwrite(records, 1, records_used, capture);
        fflush(capture);
    }
    if (native_picture_sink && records_used) {
        native_picture_sink(records, records_used);
    }
    records_used = 0;
    recording = recorded();
}

/* report the most names each table held in the run, against a quarter of its
 * slots. past that, a tidy forgets all but the names the act's scene reads. it
 * also reports how many versions of the palettes it gave */
void sa1_names_report(void)
{
    static const struct {
        const char *name;
        const struct table *table;
    } tables[] = {{"meshes", &meshes},
                  {"lists", &lists},
                  {"materials", &materials},
                  {"shades", &shades},
                  {"textures", &textures}};

    for (size_t i = 0; i < sizeof tables / sizeof tables[0]; i++) {
        fprintf(stderr,
                "sa1: the adapter's %s: at most %u named of %u, %u at the "
                "end, %u in all\n",
                tables[i].name, tables[i].table->most, TABLE_SLOTS / 4,
                tables[i].table->live, tables[i].table->next);
    }
    fprintf(stderr, "sa1: the adapter's palettes: %d versions given\n",
            palettes_id + 1);
    fprintf(stderr,
            "sa1: the adapter's tasks took at most %u slots of %u, and a "
            "picture's records at most %zu bytes\n",
            serial_most, TASK_SLOTS, records_most);
    fprintf(stderr,
            "sa1: the adapter left out %u records as more than a record "
            "holds, a mesh's or a list's once for each draw or part of a "
            "scene that needs it\n",
            records_refused);
}
