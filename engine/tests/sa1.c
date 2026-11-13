/* tests of the game adapter, engine/sa1, on models of the test's own, linked
 * with Ninja's njkm.c. it tests the game's statics the adapter finds, and what
 * a live draw reads at its call. it tests what a draw leaves in Ninja's
 * context on each way out, and a record's bytes. it tests the draws Ninja's
 * own drawers send nothing for. it tests the file a texture's record names,
 * the scene a land's load gives, and the sound calls.
 * it needs no disc. the test calls sa_present_live as src/draw/model.c, SA1's
 * layer, does. it defines the light, palette and colors the layer defines */
#include <kamui.h>
#include <math.h>
#include <ninja.h>
#include <shinobi.h>
#include <sofdec.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "collision/world.h"
#include "draw/sprite.h"
#include "live.h"
#include "native.h"
#include "ninja/njkamui.h"
#include "sa1.h"
#include "sa_capture.h"

/* what the adapter reads of the game */
int frame_step;
Bool draw_off;
int16_t stage_current = 1;
int play_time = 100;
int native_target_stage = 1;
unsigned long native_vsyncs;
NJS_VECTOR sa1_live_light;
Uint32 sa1_live_colors[SA_PALETTES * SA_PALETTE_ENTRIES * 2 + SA_UNLIT_OFFSETS];
const Uint32 *sa1_live_palette;
void present_run(struct task *task, void (*step)(struct task *task))
{
    step(task);
}
void present_draw(struct task *task, void (*draw)(struct task *task))
{
    if (draw) {
        draw(task);
    }
}
/* the split frame's hooks, which keep nothing and refuse no draw. a split
 * frame is a frame of play that shows two pictures */
int frame_later_draws;
Uint32 port_sprites_sent, port_draws_sent;
Int present_record(enum present_kind kind, const void *what, Int count, Int a,
                   Float pri, Uint32 attr)
{
    (void)kind, (void)what, (void)count, (void)a, (void)pri, (void)attr;
    return TRUE;
}
Int present_sprite(enum present_kind kind, const void *what, Int count, Int a,
                   Float pri, Uint32 attr)
{
    return present_record(kind, what, count, a, pri, attr);
}
enum present_shown present_shown(int late)
{
    (void)late;
    return SHOWN_UNWANTED;
}
NJS_MATRIX _nj_view_matrix_;
NJS_TEXLIST *_nj_current_texlist;
Uint32 _nj_current_texture;
Uint32 _nj_current_texture_bank;
KMVERTEXBUFFDESC _nj_vertex_buffer_;
Uint32 *_nj_color_mode_;
struct tagKMVERTEXCONTEXT *_nj_current_ctx_;
Uint32 _nj_control_3d_flag_, _nj_constant_attr_and_, _nj_constant_attr_or_;
NJS_ARGB _nj_constant_material_;
NJS_SCREEN _nj_screen_;
NJS_POINT2 _nj_2d_clip_[2];
Float _nj_near_clip_, _nj_far_clip_, _nj_scr_aspect_x_, _nj_scr_aspect_y_;
/* the 16:9 picture's hooks, set as at 4:3, where no 2D draw squeezes */
Float port_draw_aspect = 1.0F;
Int port_wide_squeezes(void)
{
    return FALSE;
}
Float port_wide_x(Float x)
{
    return x;
}
Int port_wide_covers(const Float *x, Int count, Int stride)
{
    (void)x, (void)count, (void)stride;
    return FALSE;
}
Sint32 _nj_texglobal_color_;
NJS_TEXMEMLIST *_nj_texmemlist;
Uint32 _nj_texmemsize;
Sint32 njSearchTexList(NJS_TEXMEMLIST *list, Uint32 size, Uint32 index)
{
    (void)list, (void)size, (void)index;
    return 0;
}
void __real_frame_wait(void)
{
}
int32_t __real_kmRender(void)
{
    return 0;
}
/* what ninja.c calls of Ninja's own. the drawers draw nothing here, and
 * njInitGlobal sets nothing. njCalcPoint puts a point through XMTRX, as it
 * does with no matrix given. the other matrix calls move nothing, since no
 * test draws a tree. _nj_vertex_num_ is the points a drawer takes, more than
 * any model here has unless a test lowers it */
Int _nj_vertex_num_ = 2048;
void njInitGlobal(void)
{
}
void ninja_easy_draw_model(NJS_MODEL *model)
{
    (void)model;
}
void ninja_simple_draw_model(NJS_MODEL *model)
{
    (void)model;
}
void __real_njDrawTriangle3D(NJS_POINT3COL *p, Int n, Uint32 attr)
{
    (void)p, (void)n, (void)attr;
}
void __real_njDrawPolygon2D(NJS_POINT2COL *p, Int n, Float pri, Uint32 attr)
{
    (void)p, (void)n, (void)pri, (void)attr;
}
void __real_njDrawLine3D(NJS_POINT3COL *p, Int n, Uint32 attr)
{
    (void)p, (void)n, (void)attr;
}
void __real_njDrawTexture(NJS_TEXTURE_VTX *polygon, Int count, Int tex,
                          Int flag)
{
    (void)polygon, (void)count, (void)tex, (void)flag;
}
void __real_njDrawPolygon(NJS_POLYGON_VTX *polygon, Int count, Int trans)
{
    (void)polygon, (void)count, (void)trans;
}
void njCalcPoint(NJS_MATRIX *m, NJS_POINT3 *ps, NJS_POINT3 *pd)
{
    const float *x = native_xmtrx();

    (void)m;
    *pd = (NJS_POINT3){ps->x * x[0] + ps->y * x[4] + ps->z * x[8] + x[12],
                       ps->x * x[1] + ps->y * x[5] + ps->z * x[9] + x[13],
                       ps->x * x[2] + ps->y * x[6] + ps->z * x[10] + x[14]};
}
Bool njPushMatrix(NJS_MATRIX *m)
{
    (void)m;
    return TRUE;
}
Bool njPopMatrix(Uint32 n)
{
    (void)n;
    return TRUE;
}
void njTranslate(NJS_MATRIX *m, Float x, Float y, Float z)
{
    (void)m, (void)x, (void)y, (void)z;
}
void njRotateXYZ(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz)
{
    (void)m, (void)angx, (void)angy, (void)angz;
}
void njRotateZXY(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz)
{
    (void)m, (void)angx, (void)angy, (void)angz;
}
void njScale(NJS_MATRIX *m, Float sx, Float sy, Float sz)
{
    (void)m, (void)sx, (void)sy, (void)sz;
}
KMSTATUS kmSetCullingRegister(KMFLOAT size)
{
    (void)size;
    return KMSTATUS_SUCCESS;
}
void __wrap_frame_wait(void);
/* the entries of Ninja's simple and easy drawers that ninja.c defines */
void _njSimpleDrawModelSub(NJS_MODEL *model);
void _njEasyDrawModel(NJS_MODEL *model);
/* the overlay, which draws nothing here */
void overlay_draw(void)
{
}
/* the test's own engine, which the adapter's sound calls reach. it counts the
 * stops, streams and banks they give it, and keeps the last bank */
static struct {
    int stops, streams, banks;
    sa_audio_bank bank;
} heard;
sa_result sa_audio_submit(sa_engine *engine, const sa_audio_command *commands,
                          uint32_t count)
{
    (void)engine;
    for (uint32_t i = 0; i < count; i++) {
        heard.stops += commands[i].kind == SA_AUDIO_STREAM_STOP;
    }
    return SA_OK;
}
sa_result sa_audio_load(sa_engine *engine, const sa_audio_bank *bank)
{
    (void)engine;
    heard.bank = *bank;
    heard.banks++;
    return SA_OK;
}
sa_result sa_audio_stream(sa_engine *engine, uint32_t stream, const void *data,
                          uint32_t bytes)
{
    (void)engine, (void)stream, (void)data, (void)bytes;
    heard.streams++;
    return SA_OK;
}
/* any name opens the bytes a test gave, or no file where it gave none */
static const uint8_t *disc_bytes;
static size_t disc_size;
FILE *native_disc_file(const char *name)
{
    FILE *file = disc_bytes ? tmpfile() : NULL;

    (void)name;
    if (file) {
        fwrite(disc_bytes, 1, disc_size, file);
        rewind(file);
    }
    return file;
}

static float xmtrx[16];
static float light_tint[4] = {1, 1, 1, 1};
/* camera.h's camera_control, as far as a cut reads it, and camera_steps */
static struct {
    int32_t changed;
    int16_t collision;
    int8_t mode, type, level, adjust;
    uint8_t unk_0a;
    int8_t flags;
    void *step, *adjust_step;
    int32_t steps;
} camera_control_record = {.adjust = 1, .steps = 1000};
static uint32_t camera_steps = 1000;

float *native_xmtrx(void)
{
    return xmtrx;
}
uint32_t native_name_id(void *function)
{
    return function != NULL;
}
/* Ninja's created lights as njCreateLight lists them: the SH-4's 32-bit
 * pointers, then a count. the full drawer's test fills them */
static uint32_t light_control[NJD_LIGHT_MAX + 1];
/* card.c's sources_file, as far as a sources file fills it. the act's 30
 * sources come first, then its light's turn, color and strengths. the scene's
 * test fills it */
static struct {
    struct {
        Sint32 used;
        Float pos[3], range, rate, color[3], touch_pos[3];
    } sources[30];
    Angle ry, rz;
    Float color[3], specular, diffuse, ambient;
} sources_file;
/* njModelE.c's calc, as far as the easy light reads it. it holds the light's
 * direction and the negative of its diffuse, then room for the rest. the line
 * below holds njModelS.c's light, its scale and its ambient */
static float easy_work[16];
static float simple_direction[3], simple_scale, simple_ambient;
/* each of the game's statics the adapter finds by name, and where the test
 * holds it. no two share their bytes, so a read shows which name it found */
static const struct {
    const char *name;
    void *at;
} named[] = {
    {"_nj_light_control_", light_control},
    {"camera_control_record", &camera_control_record},
    {"camera_steps", &camera_steps},
    {"calc", easy_work},
    {"light_tint", light_tint},
    {"base_light_direction", simple_direction},
    {"base_light_scale", &simple_scale},
    {"iamb", &simple_ambient},
    {"sources_file", &sources_file},
};
#define NAMED (sizeof named / sizeof named[0])
/* the name the test's program lacks, as a stripped one would, or null where
 * it names every static */
static const char *lacked;
char *native_symbol(const char *name)
{
    for (size_t i = 0; i < NAMED; i++) {
        if (!strcmp(name, named[i].name)) {
            return lacked && !strcmp(name, lacked) ? NULL : named[i].at;
        }
    }
    return NULL;
}
Sint32 njSetTexture(NJS_TEXLIST *texlist)
{
    _nj_current_texlist = texlist;
    return 1;
}

#define CHECK(test)                                                            \
    do {                                                                       \
        if (!(test)) {                                                         \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #test);         \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

/* sa1_statics_find sets every static or none, so no draw reads some of them.
 * a program that lacks one is told its name. one that names them all has each
 * found, and the adapter asks for no name but named's. the easy and simple
 * drawers' lights then read the statics of their names */
static void statics_found(void)
{
    double light[3];
    float ambient, diffuse;

    for (size_t i = 0; i < NAMED; i++) {
        const char *lacking;

        lacked = named[i].name;
        lacking = sa1_statics_find();
        CHECK(lacking && !strcmp(lacking, named[i].name));
        CHECK(sa1_light_tint() == NULL);
    }
    lacked = NULL;
    CHECK(sa1_statics_find() == NULL);
    CHECK(sa1_light_tint() == light_tint);
    easy_work[0] = 1;
    easy_work[1] = 2;
    easy_work[2] = 3;
    easy_work[3] = -0.5F;
    sa1_easy_light(light, &ambient, &diffuse);
    CHECK(light[0] == 1 && light[1] == 2 && light[2] == 3 && diffuse == 0.5F);
    /* a direction of length 5, which the simple light gives at length 1 */
    simple_direction[1] = 3;
    simple_direction[2] = 4;
    simple_scale = 0.25F;
    simple_ambient = 0.75F;
    sa1_simple_light(light, &ambient, &diffuse);
    CHECK(light[0] == 0 && light[1] == 3.0 / 5 && light[2] == 4.0 / 5);
    CHECK(diffuse == 0.25F && ambient == 0.75F);
    /* dark again, as the draws after this find them */
    memset(easy_work, 0, sizeof easy_work);
    memset(simple_direction, 0, sizeof simple_direction);
    simple_scale = simple_ambient = 0;
}

/* src/draw/model.c's entries as at 30 fps, where draw_model and
 * draw_model_nested draw on step 0 alone */
static void live(NJS_MODEL *model, Uint32 how)
{
    struct sa1_live_state state = {how};
    sa_draw_live draw = {model, &state, SA_SA1_BASIC, 0};

    CHECK(sa_present_live(NULL, &draw) == SA_OK);
}

static void draw_model_init(void)
{
    sa1_live_screen(_nj_scr_aspect_x_, NULL);
    sa1_live_palette = sa1_live_colors;
}

static void draw_model(NJS_MODEL *model)
{
    if (frame_step == 0) {
        live(model, SA1_LIVE_MODEL);
    }
}

static void draw_model_nested(NJS_MODEL *model)
{
    if (frame_step == 0) {
        live(model, SA1_LIVE_NESTED);
    }
}

static void draw_models_begin(void)
{
    sa1_live_run();
    njGetkmPointer();
}

static void draw_model_unculled(NJS_MODEL *model)
{
    live(model, SA1_LIVE_LAND);
}

static void draw_models_end(void)
{
    _nj_parameterkm_.TSPPARAMBUFFER |= MODULATE_ALPHA;
    njSetkmPointer();
}

/* a square of four points that each of two meshsets draws as a strip. the
 * first's material is textured and opaque, and the second's blended */
static NJS_POINT3 points[4] = {{-1, -1, 0}, {1, -1, 0}, {-1, 1, 0}, {1, 1, 0}};
static NJS_VECTOR normals[4] = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
static Sint16 strip[5] = {4, 0, 1, 2, 3};
static NJS_COLOR uvs[4];
static NJS_MATERIAL materials[2];
static NJS_MESHSET meshsets[2];
static NJS_MODEL model = {points, normals, 4,         meshsets, materials,
                          2,      2,       {0, 0, 0}, 2};
static NJS_TEXMEMLIST memory[2] = {
    {.globalIndex = 7, .tspparambuffer = 0x1b, .texparambuffer = 0x1234},
    {.globalIndex = 9, .tspparambuffer = 0x2d, .texparambuffer = 0x5678}};
static NJS_TEXNAME texnames[2];
static NJS_TEXLIST texlist = {texnames, 2};
static KMVERTEXCONTEXT context;
static Uint32 color_mode;
static PKMDWORD lists[8];

static void reset(void)
{
    memset(&context, 0, sizeof context);
    memset(&_nj_parameterkm_, 0, sizeof _nj_parameterkm_);
    _nj_current_texture = 0;
    _nj_control_3d_flag_ = 0;
    _nj_constant_attr_and_ = ~0U;
    _nj_constant_attr_or_ = 0;
    frame_step = 0;
    /* the eye at the origin looks down z, and the model lies at z = 10 */
    memset(xmtrx, 0, sizeof xmtrx);
    xmtrx[0] = xmtrx[5] = xmtrx[10] = xmtrx[15] = 1.0F;
    xmtrx[14] = 10.0F;
}

static void setup(void)
{
    /* njkm.c writes the store queue area, and QACR0 and QACR1 among the SH-4's
     * control registers. records.c reads the fog from the PVR's registers */
    if (mmap((void *)0xe0000000UL, 0x4000000, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1,
             0) == MAP_FAILED ||
        mmap((void *)0xff000000UL, 0x10000, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1,
             0) == MAP_FAILED ||
        mmap((void *)0xa05f8000UL, 0x1000, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1,
             0) == MAP_FAILED) {
        perror("the test's registers");
        exit(2);
    }
    for (int i = 0; i < 2; i++) {
        meshsets[i].type_matId = (Uint16)(NJD_MESHSET_TRIMESH | i);
        meshsets[i].nbMesh = 1;
        meshsets[i].meshes = strip;
        meshsets[i].vertuv = uvs;
        materials[i].attr_texId = (Uint32)i;
        texnames[i].filename = i ? "second" : "first";
        texnames[i].texaddr = (Uint32)(uintptr_t)&memory[i];
    }
    materials[0].attrflags = NJD_FLAG_USE_TEXTURE;
    materials[1].attrflags =
        NJD_FLAG_USE_TEXTURE | NJD_FLAG_USE_ALPHA | NJD_SA_INV_SRC;
    _nj_vertex_buffer_.pCurrentPtr = lists;
    _nj_current_ctx_ = &context;
    _nj_color_mode_ = &color_mode;
    _nj_current_texlist = &texlist;
    _nj_screen_ = (NJS_SCREEN){500.0F, 640.0F, 480.0F, 320.0F, 240.0F};
    _nj_near_clip_ = 1.0F;
    _nj_far_clip_ = 60000.0F;
    _nj_scr_aspect_x_ = _nj_scr_aspect_y_ = 1.0F;
    _nj_2d_clip_[1].x = 639.0F;
    _nj_2d_clip_[1].y = 479.0F;
    for (int i = 0; i < 16; i += 5) {
        _nj_view_matrix_[i] = 1.0F;
    }
    draw_model_init();
}

/* the records of the pictures so far */
static uint8_t file[1 << 24];
static size_t file_size;
static const char *path;

static void read_file(void)
{
    FILE *in = fopen(path, "rb");

    CHECK(in != NULL);
    file_size = fread(file, 1, sizeof file, in);
    fclose(in);
    /* the whole of it: a file that fills the room may run on past it */
    CHECK(file_size < sizeof file);
}

/* the count-th record of kind, from 0, or null */
static const uint8_t *record(uint16_t kind, int count)
{
    for (size_t at = 0; at + sizeof(sa_record_head) <= file_size;) {
        sa_record_head head;

        memcpy(&head, file + at, sizeof head);
        if (head.kind == kind && count-- == 0) {
            return file + at;
        }
        at += head.size;
    }
    return NULL;
}

static sa_cap_material material_of(const sa_draw *draw, int index)
{
    const uint8_t *list;
    sa_cap_material material;

    for (int i = 0; (list = record(SA_CAP_MATERIALS, i)); i++) {
        sa_cap_materials head;

        memcpy(&head, list, sizeof head);
        if (head.materials == draw->materials) {
            uint32_t id;

            memcpy(&id, list + sizeof head + 4 * (size_t)index, 4);
            for (int k = 0; record(SA_CAP_MATERIAL, k); k++) {
                memcpy(&material, record(SA_CAP_MATERIAL, k), sizeof material);
                if (material.material == id) {
                    return material;
                }
            }
        }
    }
    CHECK(!"a draw's material has no record");
    return material;
}

static sa_draw draw_of(int count)
{
    sa_draw draw;
    const uint8_t *at = record(SA_REC_DRAW, count);

    CHECK(at != NULL);
    memcpy(&draw, at, sizeof draw);
    return draw;
}

static sa_shade shade_of(uint32_t id)
{
    sa_shade shade;

    for (int i = 0; record(SA_REC_SHADE, i); i++) {
        memcpy(&shade, record(SA_REC_SHADE, i), sizeof shade);
        if (shade.shade == id) {
            return shade;
        }
    }
    CHECK(!"a draw's shade has no record");
    return shade;
}

/* a draw reads Ninja's state as it is called, so a direct store counts as a
 * setter's */
static void state_at_the_call(void)
{
    reset();
    draw_model(&model);
    _nj_control_3d_flag_ = NJD_CONTROL_3D_CONSTANT_ATTR;
    _nj_constant_attr_or_ = NJD_FLAG_DOUBLE_SIDE;
    context.ISPPARAMBUFFER = 7U << ISP_DEPTH_SHIFT;
    draw_model(&model);
    _nj_parameterkm_.ISPPARAMBUFFER =
        3U << ISP_DEPTH_SHIFT | ISP_Z_WRITE_DISABLE;
    _nj_parameterkm_.TSPPARAMBUFFER = (Uint32)KM_NOFOG << TSP_FOG_SHIFT;
    draw_model_nested(&model);
    __wrap_frame_wait();
    read_file();
    sa_draw first = draw_of(0);
    sa_draw second = draw_of(1);
    sa_draw third = draw_of(2);
    CHECK(!(material_of(&first, 0).flags & SA_MAT_DOUBLE));
    CHECK(material_of(&second, 0).flags & SA_MAT_DOUBLE);
    CHECK(shade_of(first.shade).depth == 0);
    /* draw_model takes the context's words, and a nested draw
     * _nj_parameterkm_'s */
    CHECK(shade_of(second.shade).depth == 7);
    CHECK(shade_of(second.shade).fog == SA_FOG_TABLE);
    CHECK(!(shade_of(second.shade).flags & SA_SHADE_NO_DEPTH_WRITE));
    CHECK(shade_of(third.shade).depth == 3);
    CHECK(shade_of(third.shade).fog == SA_FOG_NONE);
    CHECK(shade_of(third.shade).flags & SA_SHADE_NO_DEPTH_WRITE);
}

/* what the game would see after a draw, on each way out */
static void state_left_behind(void)
{
    /* in view: the last meshset's words and texture stay in the context */
    reset();
    draw_model(&model);
    CHECK(_nj_current_texture == 9);
    CHECK((context.TSPPARAMBUFFER & TSP_UV_SIZE) == 0x2d);
    CHECK(context.TexturePARAMBUFFER == 0x5678);
    CHECK((context.TSPPARAMBUFFER >> TSP_SRC_SHIFT) == BLEND_INV_SRC_ALPHA);
    /* the second meshset blends, and closing the context sets
     * MODULATE_ALPHA */
    CHECK(context.GLOBALPARAMBUFFER & GLOBAL_TRANSLUCENT);
    CHECK((context.TSPPARAMBUFFER & TSP_SHADING) == MODULATE_ALPHA);

    /* out of view: nothing moves */
    reset();
    xmtrx[14] = -100.0F;
    draw_model(&model);
    CHECK(_nj_current_texture == 0);
    CHECK(context.TSPPARAMBUFFER == 0);

    /* in view by its sphere, but every point is nearer than the near clip.
     * nothing moves, but a nested draw still resets the shading */
    reset();
    xmtrx[14] = 0.5F;
    draw_model(&model);
    CHECK(_nj_current_texture == 0);
    CHECK(context.TSPPARAMBUFFER == 0);
    draw_model_nested(&model);
    CHECK(_nj_current_texture == 0);
    CHECK(_nj_parameterkm_.TSPPARAMBUFFER == MODULATE_ALPHA);

    /* a later step draws nothing, and an unculled run draws on every step */
    reset();
    frame_step = 1;
    draw_model(&model);
    draw_model_nested(&model);
    CHECK(_nj_current_texture == 0);
    draw_models_begin();
    draw_model_unculled(&model);
    CHECK(_nj_current_texture == 9);
    /* the run's words wait in _nj_parameterkm_ until its end */
    CHECK(context.TSPPARAMBUFFER == 0);
    draw_models_end();
    CHECK((context.TSPPARAMBUFFER & TSP_SHADING) == MODULATE_ALPHA);

    /* a run's draws take the attributes it began with */
    reset();
    draw_models_begin();
    _nj_control_3d_flag_ = NJD_CONTROL_3D_CONSTANT_ATTR;
    _nj_constant_attr_and_ = 0;
    draw_model_unculled(&model);
    CHECK(_nj_current_texture == 9);
    draw_models_end();
}

/* a draw the view test drops still has a record, flagged SA_DRAW_OUT. a
 * picture draws nothing for it */
static void what_the_call_sent(void)
{
    int before = 0;

    __wrap_frame_wait();
    read_file();
    while (record(SA_REC_DRAW, before)) {
        before++;
    }
    reset();
    draw_model(&model);
    draw_model_nested(&model);
    xmtrx[14] = -100.0F;
    draw_model(&model);
    draw_model_nested(&model);
    /* an unculled run sends what it's given */
    draw_models_begin();
    draw_model_unculled(&model);
    draw_models_end();
    __wrap_frame_wait();
    read_file();
    CHECK(!(draw_of(before).flags & SA_DRAW_OUT));
    CHECK(!(draw_of(before + 1).flags & SA_DRAW_OUT));
    CHECK(draw_of(before + 2).flags & SA_DRAW_OUT);
    CHECK(draw_of(before + 3).flags & SA_DRAW_OUT);
    CHECK(!(draw_of(before + 4).flags & SA_DRAW_OUT));
}

/* njDrawPolygon's strip, as the adapter records it. its depth is the one 1/z
 * stands for, or 1 where 1/z is 0 */
static void screen_strip(void)
{
    static NJS_POLYGON_VTX strip[4] = {{0, 0, 0.5F, 0xff000001},
                                       {640, 0, 0.5F, 0xff000002},
                                       {0, 480, 0.0F, 0xff000003},
                                       {640, 480, 0.5F, 0xff000004}};
    const uint8_t *at;
    int before = 0;
    sa_triangles t;
    sa_point p[4];

    __wrap_frame_wait();
    read_file();
    while (record(SA_CAP_TRIANGLES, before)) {
        before++;
    }
    reset();
    sa1_record_screen_strip(strip, 4, 0, 0, 1);
    sa1_record_screen_strip(strip, 2, 0, 0, 1); /* no triangle: lost */
    __wrap_frame_wait();
    read_file();
    CHECK((at = record(SA_CAP_TRIANGLES, before)) != NULL);
    memcpy(&t, at, sizeof t);
    memcpy(p, at + sizeof t, sizeof p);
    CHECK(t.form == SA_TRIANGLES_STRIP && t.count == 2 &&
          t.flags == (SA_TRIANGLES_SCREEN | SA_TRIANGLES_BLEND) &&
          t.texture == SA_CAP_NONE && t.object_to_world[0] == 1.0F);
    CHECK(p[0].at[2] == 2.0F && p[2].at[2] == 1.0F && p[3].color == 0xff000004);
    CHECK(record(SA_CAP_TRIANGLES, before + 1) == NULL);
}

/* a mesh is the model's bytes at the call. a writer's change makes a new mesh.
 * what a stack lent is safe once the stack moves on */
static void borrowed_data(void)
{
    /* a model's references must lie in the game's 4 GB of addresses. the
     * game's stack does, and the test's doesn't, so the uvs lent here are
     * static */
    static NJS_COLOR lent[4];
    NJS_COLOR *kept = meshsets[0].vertuv;

    reset();
    memset(lent, 0x11, sizeof lent);
    meshsets[0].vertuv = lent;
    draw_model(&model);
    memset(lent, 0x22, sizeof lent);
    meshsets[0].vertuv = kept;
    points[0].x = -2.0F;
    draw_model(&model);
    points[0].x = -1.0F;
    __wrap_frame_wait();
    read_file();
    int count = 0;
    const uint8_t *found = NULL;
    const uint8_t *mesh;

    for (int i = 0; (mesh = record(SA_CAP_MESH, i)); i++) {
        sa_record_head head;

        memcpy(&head, mesh, sizeof head);
        /* the lent uvs, as the call found them */
        for (uint32_t at = 0; at + 4 <= head.size; at += 4) {
            if (!memcmp(mesh + at, "\x11\x11\x11\x11", 4)) {
                found = mesh;
            }
        }
        count++;
    }
    CHECK(count >= 2);
    CHECK(found != NULL);
}

/* a fade's polygon is a strip on the screen at the depth its pri gives. its
 * corners alternate from the front and from the back. an outline sends
 * nothing */
static void screen_polygons(void)
{
    static NJS_POINT2 corners[4] = {{0, 0}, {640, 0}, {640, 480}, {0, 480}};
    static NJS_COLOR colors[4] = {
        {0x80000001}, {0x80000002}, {0x80000003}, {0x80000004}};
    NJS_POINT2COL polygon = {corners, colors, NULL, 4};
    static const int order[4] = {0, 3, 1, 2};
    const uint8_t *at;
    const uint8_t *last = NULL;
    int found = 0;
    sa_triangles head;

    reset();
    sa1_record_polygon_2d(&polygon, 4, -2.0F, NJD_DRAW_NORMAL);
    sa1_record_polygon_2d(&polygon, 4, -2.0F, NJD_FILL | NJD_TRANSPARENT);
    __wrap_frame_wait();
    read_file();
    for (int i = 0; (at = record(SA_CAP_TRIANGLES, i)); i++) {
        memcpy(&head, at, sizeof head);
        if (head.flags & SA_TRIANGLES_SCREEN) {
            last = at;
            found++;
        }
    }
    CHECK(found == 1);
    memcpy(&head, last, sizeof head);
    CHECK(head.count == 2 && head.form == SA_TRIANGLES_STRIP);
    CHECK(head.flags == (SA_TRIANGLES_SCREEN | SA_TRIANGLES_BLEND));
    for (int k = 0; k < 4; k++) {
        sa_point point;

        memcpy(&point, last + sizeof head + k * sizeof point, sizeof point);
        CHECK(point.at[0] == corners[order[k]].x &&
              point.at[1] == corners[order[k]].y && point.at[2] == 2.0F &&
              point.color == colors[order[k]].color);
    }
}

/* Ninja's lines go as two points each, in the object's space. a strip's line i
 * joins points i and i + 1. a fan's joins points 0 and i + 1, and any other's
 * joins 2i and 2i + 1 */
static void lines(void)
{
    static NJS_POINT3 at[4] = {{0, 0, 5}, {1, 0, 5}, {1, 1, 5}, {0, 1, 5}};
    static NJS_COLOR colors[4] = {{1}, {2}, {3}, {4}};
    NJS_POINT3COL line = {at, colors, NULL, 4};
    static const int ends[3][6] = {
        {0, 1, 1, 2, 2, 3}, {0, 1, 0, 2, 0, 3}, {0, 1, 2, 3, -1, -1}};
    static const uint32_t attrs[3] = {NJD_DRAW_CONNECTED, NJD_DRAW_FAN, 0};
    static const int counts[3] = {3, 3, 2};
    const uint8_t *at_record;
    int first = 0;

    reset();
    while (record(SA_CAP_TRIANGLES, first)) {
        first++;
    }
    for (int k = 0; k < 3; k++) {
        sa1_record_lines(&line, counts[k], attrs[k]);
    }
    __wrap_frame_wait();
    read_file();
    for (int k = 0; k < 3; k++) {
        sa_triangles head;

        CHECK((at_record = record(SA_CAP_TRIANGLES, first + k)) != NULL);
        memcpy(&head, at_record, sizeof head);
        CHECK(head.form == SA_TRIANGLES_LINES && head.count == counts[k]);
        for (int i = 0; i < 2 * counts[k]; i++) {
            const NJS_POINT3 *end = &at[ends[k][i]];
            sa_point point;

            memcpy(&point, at_record + sizeof head + i * sizeof point,
                   sizeof point);
            /* reset()'s matrix moves z alone, so z tells the object's space
             * from the world's */
            CHECK(point.color == colors[ends[k][i]].color &&
                  point.at[0] == end->x && point.at[1] == end->y &&
                  point.at[2] == end->z);
        }
    }
}

/* a sprite's y scales as its drawer scales it on the Dreamcast. facing the
 * view, under NJD_SPRITE_SCALE, njDrawSprite3D takes sy and sprite_draw_3d
 * takes sx. the sx is sprite_send_facing's @bug. in the plane or on the
 * screen, each takes sy. a sprite on the screen records no z */
static void sprite_scales(void)
{
    static NJS_TEXANIM frame = {16, 16, 8, 8, 0, 0, 255, 255, 0, 0};
    NJS_SPRITE sprite = {{0, 0, 7.0F}, 3.0F, 5.0F, 0, &texlist, &frame};
    static const float y_scales[5] = {5.0F, 3.0F, 5.0F, 5.0F, 5.0F};
    int before = 0;

    __wrap_frame_wait();
    read_file();
    while (record(SA_CAP_SPRITE, before)) {
        before++;
    }
    reset();
    njDrawSprite3D(&sprite, 0, NJD_SPRITE_SCALE);
    sprite_draw_3d(&sprite, 0, NJD_SPRITE_SCALE);
    njDrawSprite3D(&sprite, 0, 0);
    sprite_draw_3d(&sprite, 0, 0);
    njDrawSprite2D(&sprite, 0, -1.0F, NJD_SPRITE_SCALE);
    __wrap_frame_wait();
    read_file();
    for (int i = 0; i < 5; i++) {
        const uint8_t *at = record(SA_CAP_SPRITE, before + i);
        sa_sprite out;

        CHECK(at != NULL);
        memcpy(&out, at, sizeof out);
        CHECK(out.scale[0] == 3.0F && out.scale[1] == y_scales[i]);
        CHECK(out.at[2] == (i == 4 ? 0.0F : 7.0F));
    }
}

/* only a nested draw takes the Ninja forms, while the layer has them on, as on
 * the Dreamcast. under them a one-sided material culls and takes the constant
 * color. a translucent unlit one adds no offset color. the flags and
 * attributes set here are the forms', but alone put no draw in them */
static void ninja_forms(void)
{
    const uint16_t forms = SA_MAT_CULL | SA_MAT_CONSTANT;
    int before = 0;

    __wrap_frame_wait();
    read_file();
    while (record(SA_REC_DRAW, before)) {
        before++;
    }
    reset();
    _nj_control_3d_flag_ =
        NJD_CONTROL_3D_CONSTANT_ATTR | NJD_CONTROL_3D_CONSTANT_MATERIAL;
    _nj_constant_attr_or_ = NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_ALPHA;
    draw_model(&model);
    draw_model_nested(&model);
    draw_models_begin();
    draw_model_unculled(&model);
    draw_models_end();
    sa1_live_forms(TRUE);
    draw_model(&model);
    draw_model_nested(&model);
    /* a land draw reads the attributes the nested draw took, and no forms */
    draw_model_unculled(&model);
    sa1_live_forms(FALSE);
    draw_model_nested(&model);
    __wrap_frame_wait();
    read_file();
    for (int i = 0; i < 7; i++) {
        sa_draw draw = draw_of(before + i);
        uint16_t flags = material_of(&draw, 0).flags;

        CHECK((flags & forms) == (i == 4 ? forms : 0));
        CHECK(!(flags & SA_MAT_OFFSET) ==
              (i == 4 || !(flags & SA_MAT_TEXTURE)));
    }
}

/* a constant material's alpha goes as the Dreamcast sends it. it is the low
 * byte of the alpha times 255, rounded toward zero. so an alpha below 0 or
 * over 1 wraps */
static void constant_alpha(void)
{
    static const float alphas[3] = {0.5F, -0.5F, 2.0F};
    static const uint8_t bytes[3] = {0x7f, 0x81, 0xfe};
    int before = 0;

    __wrap_frame_wait();
    read_file();
    while (record(SA_REC_DRAW, before)) {
        before++;
    }
    reset();
    _nj_control_3d_flag_ = NJD_CONTROL_3D_CONSTANT_MATERIAL;
    for (int i = 0; i < 3; i++) {
        _nj_constant_material_.a = alphas[i];
        draw_model(&model);
    }
    _nj_constant_material_.a = 0.0F;
    __wrap_frame_wait();
    read_file();
    for (int i = 0; i < 3; i++) {
        sa_draw draw = draw_of(before + i);

        CHECK(material_of(&draw, 1).alpha == bytes[i]);
    }
}

/* a material's record takes the texture filter and mipmap D adjust its flags
 * hold. the filter goes as an SA_FILTER_ value, and the adjust in quarters, so
 * NJD_D_100's 1.00 is 4 */
static void filter_and_adjust(void)
{
    Uint32 kept = materials[0].attrflags;
    int before = 0;

    __wrap_frame_wait();
    read_file();
    while (record(SA_REC_DRAW, before)) {
        before++;
    }
    reset();
    draw_model(&model);
    materials[0].attrflags = kept | NJD_FILTER_BILINEAR | NJD_D_100;
    draw_model(&model);
    materials[0].attrflags = kept;
    __wrap_frame_wait();
    read_file();
    sa_draw plain = draw_of(before);
    sa_draw set = draw_of(before + 1);
    CHECK(material_of(&plain, 0).filter == SA_FILTER_POINT);
    CHECK(material_of(&plain, 0).mip_bias == 0);
    CHECK(material_of(&set, 0).filter == SA_FILTER_BILINEAR);
    CHECK(material_of(&set, 0).mip_bias == 4);
}

/* on the Dreamcast, the walk's unlit translucent meshset leaves every point
 * white for later meshsets. the fourth meshset here names the first's
 * material, and draws white too. the whitening meshset's material alone is
 * SA_MAT_WHITE, and a backend carries the white on from it. so the mesh keeps
 * the meshsets' order and the materials they name. a material is unlit by its
 * flags as drawn. the Ninja forms and Ninja's own drawers whiten none */
static void white_after_unlit(void)
{
    static const Uint16 named[4] = {0, 1, 2, 0};
    static NJS_MATERIAL mats[3];
    static NJS_MESHSET sets[4];
    static NJS_MODEL order = {points, normals, 4,         sets, mats,
                              4,      3,       {0, 0, 0}, 2};
    /* where the SH-4's NJS_MODEL holds its meshsets' reference, and the bytes
     * of its NJS_MESHSET */
    enum { MODEL_MESHSETS = 12, MESHSET_SIZE = 24 };
    const uint16_t whitens = SA_MAT_WHITE | SA_MAT_UNLIT | SA_MAT_BLEND;
    struct sa1_attrs attrs = {0};
    const uint8_t *mesh = NULL;
    uint32_t at_sets;
    int before = 0;

    __wrap_frame_wait();
    read_file();
    while (record(SA_REC_DRAW, before)) {
        before++;
    }
    for (int i = 0; i < 4; i++) {
        sets[i] = meshsets[0];
        sets[i].type_matId = (Uint16)(NJD_MESHSET_TRIMESH | named[i]);
    }
    mats[1].attrflags = NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_ALPHA;
    reset();
    draw_model(&order);
    sa1_live_forms(TRUE);
    draw_model_nested(&order);
    sa1_live_forms(FALSE);
    sa1_record_model(&order, SA_SHADE_LINEAR, SA1_RUN, &attrs, 0);
    /* the constant attributes light the second material: none is white */
    _nj_control_3d_flag_ = NJD_CONTROL_3D_CONSTANT_ATTR;
    _nj_constant_attr_and_ = ~(Uint32)NJD_FLAG_IGNORE_LIGHT;
    draw_model(&order);
    __wrap_frame_wait();
    read_file();
    for (int i = 0; i < 4; i++) {
        sa_draw draw = draw_of(before + i);

        CHECK(!(material_of(&draw, 0).flags & whitens));
        CHECK((material_of(&draw, 1).flags & whitens) ==
              (i == 0   ? whitens
               : i == 3 ? SA_MAT_BLEND
                        : SA_MAT_UNLIT | SA_MAT_BLEND));
        CHECK(!(material_of(&draw, 2).flags & whitens));
    }
    /* the four draws' one mesh: its meshsets name 0, 1, 2 and 0, in order */
    for (int i = 0; record(SA_CAP_MESH, i); i++) {
        sa_cap_mesh head;

        memcpy(&head, record(SA_CAP_MESH, i), sizeof head);
        if (head.mesh == draw_of(before).mesh) {
            mesh = record(SA_CAP_MESH, i) + sizeof head;
        }
    }
    CHECK(mesh != NULL && draw_of(before + 3).mesh == draw_of(before).mesh);
    memcpy(&at_sets, mesh + MODEL_MESHSETS, sizeof at_sets);
    for (int i = 0; i < 4; i++) {
        Uint16 type;

        memcpy(&type, mesh + (at_sets - SA_CAP_INLINE_BASE) + MESHSET_SIZE * i,
               sizeof type);
        CHECK((type & MATERIAL_ID) == named[i]);
    }
}

/* the palettes go in where they changed since the last given. each change
 * takes a new id, even back to a value they held before. each picture names
 * the last version ahead of it */
static void palette_versions(void)
{
    static const Uint32 values[4] = {0x11, 0x22, 0x11, 0x11};
    Uint32 kept = sa1_live_colors[0];
    uint32_t last = SA_CAP_NONE;
    int before = 0;
    int versions = 0;

    __wrap_frame_wait();
    read_file();
    while (record(SA_CAP_PALETTES, before)) {
        before++;
    }
    for (int i = 0; i < 4; i++) {
        reset();
        sa1_live_colors[0] = values[i];
        draw_model(&model);
        __wrap_frame_wait();
    }
    sa1_live_colors[0] = kept;
    read_file();
    for (size_t at = 0; at + sizeof(sa_record_head) <= file_size;) {
        sa_record_head head;

        memcpy(&head, file + at, sizeof head);
        if (head.kind == SA_CAP_PALETTES) {
            uint32_t id, word;

            memcpy(&id, file + at + offsetof(sa_cap_palettes, palettes), 4);
            memcpy(&word, file + at + offsetof(sa_cap_palettes, words), 4);
            /* ids go on rising, so none comes twice */
            CHECK(last == SA_CAP_NONE || id > last);
            CHECK(versions < before || word == values[versions - before]);
            last = id;
            versions++;
        } else if (head.kind == SA_CAP_PICTURE) {
            sa_cap_picture picture;

            memcpy(&picture, file + at, sizeof picture);
            CHECK(picture.palettes == last);
        }
        at += head.size;
    }
    /* the fourth picture's are the third's, and take no record */
    CHECK(versions == before + 3);
}

/* a full draw's shade takes Ninja's lights, recorded ahead of it. each light
 * that is on goes into world space by its own matrix. its type is the word
 * njCreateLight writes past ltcal, where the SH-4 lays attr. a one-sided
 * material culls, and the highlight's exponent truncates as
 * njCalcLightMaterials truncates it */
static void full_drawer(void)
{
    static NJS_LIGHT on;
    static NJS_LIGHT off;
    const uint32_t type = NJD_PHONG_DIR;
    struct sa1_attrs attrs = {0};
    const uint8_t *at;
    sa_lights lights;
    sa_shade shade;
    sa_draw draw;
    int draws = 0;
    int found = -1;

    reset();
    for (int i = 0; i < 16; i += 5) {
        on.mtrx[i] = 1.0F;
    }
    on.mtrx[12] = 7.0F;
    on.vctr.y = -1.0F;
    on.stat = 1;
    on.ltcal.amb.r = 0.25F;
    memcpy((char *)&on.ltcal + sizeof on.ltcal, &type, sizeof type);
    light_control[0] = (uint32_t)(uintptr_t)&off;
    light_control[1] = (uint32_t)(uintptr_t)&on;
    light_control[NJD_LIGHT_MAX] = 2;
    materials[0].exponent = 6.5F;
    sa1_record_model(&model, SA_SHADE_LIGHTS, SA1_RUN, &attrs, 0);
    __wrap_frame_wait();
    materials[0].exponent = 0.0F;
    memset(light_control, 0, sizeof light_control);
    read_file();
    while (record(SA_REC_DRAW, draws)) {
        draws++;
    }
    draw = draw_of(draws - 1);
    shade = shade_of(draw.shade);
    CHECK(shade.mode == SA_SHADE_LIGHTS);
    for (int i = 0; (at = record(SA_REC_LIGHTS, i)); i++) {
        memcpy(&lights, at, sizeof lights);
        if (lights.shade == draw.shade) {
            found = i;
            break;
        }
    }
    CHECK(found >= 0 && lights.count == 1 && lights.light[0].type == type);
    for (int i = 0; (at = record(SA_REC_SHADE, i)); i++) {
        memcpy(&shade, at, sizeof shade);
        if (shade.shade == draw.shade) {
            CHECK(record(SA_REC_LIGHTS, found) < at);
        }
    }
    CHECK(lights.light[0].point[0] == 7.0F &&
          lights.light[0].direction[1] == -1.0F &&
          lights.light[0].ambient[0] == 0.25F);
    CHECK(material_of(&draw, 0).exponent == 6 &&
          (material_of(&draw, 0).flags & SA_MAT_CULL));
}

/* a draw by one of Ninja's own drawers is flagged SA_DRAW_OUT where the
 * drawer's tests send nothing. each sends nothing for a model of more points
 * than _nj_vertex_num_. under NJD_CONTROL_3D_MODEL_CLIP, each sends nothing
 * for a sphere outside the clips. the full and the simple drawer then send
 * nothing where every point holds one clip flag, unless
 * NJD_CONTROL_3D_NO_CLIP_CHECK is set. points that lie past different edges
 * share no flag. the easy drawer's last test is another: it sends nothing
 * where every point lies at or behind the eye */
static void drawers_out(void)
{
    enum { FULL = 1, SIMPLE = 2, EASY = 4, ALL = FULL | SIMPLE | EASY };
    /* where XMTRX puts the model along x and z, the control flags, whether
     * the drawers take fewer points than the model's, and the drawers that
     * send nothing. the square spans 100 pixels at z 10, and the screen's
     * middle is at x 320 */
    static const struct {
        float x, z;
        Uint32 control;
        int few;
        int out;
    } cases[] = {
        {0.0F, 10.0F, 0, 0, 0},
        /* past the left edge, by its points and then by its sphere */
        {-100.0F, 10.0F, 0, 0, FULL | SIMPLE},
        {-100.0F, 10.0F, NJD_CONTROL_3D_NO_CLIP_CHECK, 0, 0},
        {-100.0F, 10.0F,
         NJD_CONTROL_3D_NO_CLIP_CHECK | NJD_CONTROL_3D_MODEL_CLIP, 0, ALL},
        /* across the left edge */
        {-6.4F, 10.0F, 0, 0, 0},
        /* so near that each point lies past two edges, and no two past the
         * same two */
        {0.0F, 1.5F, 0, 0, 0},
        /* nearer than the near clip, behind the eye, and past the far clip */
        {0.0F, 0.5F, 0, 0, FULL | SIMPLE},
        {0.0F, -5.0F, 0, 0, ALL},
        {0.0F, 70000.0F, 0, 0, FULL | SIMPLE},
        {0.0F, 10.0F, 0, 1, ALL},
    };
    const int count = (int)(sizeof cases / sizeof cases[0]);
    const Int most = _nj_vertex_num_;
    int before = 0;

    __wrap_frame_wait();
    read_file();
    while (record(SA_REC_DRAW, before)) {
        before++;
    }
    for (int i = 0; i < count; i++) {
        reset();
        xmtrx[12] = cases[i].x;
        xmtrx[14] = cases[i].z;
        _nj_control_3d_flag_ = cases[i].control;
        _nj_vertex_num_ = cases[i].few ? model.nbPoint - 1 : most;
        njDrawModel(&model);
        _njSimpleDrawModelSub(&model);
        _njEasyDrawModel(&model);
    }
    _nj_vertex_num_ = most;
    reset();
    __wrap_frame_wait();
    read_file();
    for (int i = 0; i < count; i++) {
        for (int k = 0; k < 3; k++) {
            sa_draw draw = draw_of(before + 3 * i + k);

            CHECK(!(draw.flags & SA_DRAW_OUT) == !(cases[i].out & 1 << k));
        }
    }
    CHECK(record(SA_REC_DRAW, before + 3 * count) == NULL);
}

/* whether a free of kind names id */
static int freed(uint32_t kind, uint32_t id)
{
    const uint8_t *at;

    for (int i = 0; (at = record(SA_CAP_FREE, i)); i++) {
        sa_cap_life life;

        memcpy(&life, at, sizeof life);
        if (life.kind == kind && life.id == id) {
            return 1;
        }
    }
    return 0;
}

/* a name no picture has named for more than 30 pictures goes at the next age.
 * an age comes every 16 pictures. a mesh, a list, a material and a shade each
 * go by a free of its own kind. what the pictures still name stays */
static void names_forgotten(void)
{
    sa_draw first;
    sa_draw last;
    int draws = 0;

    reset();
    points[0].x = -3.0F;
    materials[0].diffuse.color = 0x12345678;
    sa1_live_light.x = 0.5F;
    draw_model(&model);
    __wrap_frame_wait();
    read_file();
    while (record(SA_REC_DRAW, draws)) {
        draws++;
    }
    first = draw_of(draws - 1);
    sa_cap_material material = material_of(&first, 0);
    points[0].x = -1.0F;
    materials[0].diffuse.color = 0;
    sa1_live_light.x = 0.0F;
    for (int i = 0; i < 48; i++) {
        reset();
        draw_model(&model);
        __wrap_frame_wait();
    }
    read_file();
    while (record(SA_REC_DRAW, draws)) {
        draws++;
    }
    last = draw_of(draws - 1);
    CHECK(first.mesh != last.mesh && first.materials != last.materials &&
          first.shade != last.shade);
    CHECK(material.material != material_of(&last, 0).material);
    CHECK(freed(SA_KIND_MESH, first.mesh));
    CHECK(freed(SA_KIND_MATERIAL, first.materials));
    CHECK(freed(SA_CAP_MATERIAL, material.material));
    CHECK(freed(SA_REC_SHADE, first.shade));
    CHECK(!freed(SA_KIND_MESH, last.mesh));
    CHECK(!freed(SA_KIND_MATERIAL, last.materials));
    CHECK(!freed(SA_CAP_MATERIAL, material_of(&last, 0).material));
    CHECK(!freed(SA_REC_SHADE, last.shade));
}

/* three records of texture memory, and the file that gave each its texels, or
 * 0. native_texture_file finds the file by the record's address, and returns
 * 0 for any other */
static NJS_TEXMEMLIST filed[3] = {
    {.globalIndex = 41}, {.globalIndex = 42}, {.globalIndex = 43}};
static uint32_t filed_files[3];

uint32_t native_texture_file(uint32_t texaddr)
{
    for (int i = 0; i < 3; i++) {
        if (texaddr == (Uint32)(uintptr_t)&filed[i]) {
            return filed_files[i];
        }
    }
    return 0;
}

/* a texture's record names the file of its texture memory, or 0. a load names
 * its textures by the names it gives, and a draw by its list's. a second list
 * on the same texture memory names it no second time */
static void textures_of_files(void)
{
    static const char *const loaded_as[2] = {"one", "two"};
    NJS_TEXNAME names[3];
    NJS_TEXNAME again;
    NJS_TEXLIST list = {names, 3};
    NJS_TEXLIST shared = {&again, 1};
    sa_cap_texture texture;
    int before = 0;

    for (int i = 0; i < 3; i++) {
        names[i] = (NJS_TEXNAME){"drawn", 0, (Uint32)(uintptr_t)&filed[i]};
    }
    again = names[0];
    filed_files[0] = sa_name_id("SONICADV/FIRST.PRS");
    filed_files[1] = 0;
    filed_files[2] = sa_name_id("SONICADV/THIRD.PVR");
    __wrap_frame_wait();
    read_file();
    while (record(SA_CAP_TEXTURE, before)) {
        before++;
    }
    sa1_textures_loaded(&list, 2, loaded_as);
    sa1_textures_loaded(&shared, 1, NULL);
    /* the model's first material takes the list's third texture, and its
     * second the list's second */
    reset();
    _nj_current_texlist = &list;
    materials[0].attr_texId = 2;
    draw_model(&model);
    materials[0].attr_texId = 0;
    _nj_current_texlist = &texlist;
    __wrap_frame_wait();
    read_file();
    CHECK(record(SA_CAP_TEXTURE, before + 2) &&
          !record(SA_CAP_TEXTURE, before + 3));
    memcpy(&texture, record(SA_CAP_TEXTURE, before), sizeof texture);
    CHECK(texture.global_index == 41 && texture.file == filed_files[0] &&
          !strcmp(texture.name, "one"));
    memcpy(&texture, record(SA_CAP_TEXTURE, before + 1), sizeof texture);
    CHECK(texture.global_index == 42 && texture.file == 0 &&
          !strcmp(texture.name, "two"));
    memcpy(&texture, record(SA_CAP_TEXTURE, before + 2), sizeof texture);
    CHECK(texture.global_index == 43 && texture.file == filed_files[2] &&
          !strcmp(texture.name, "drawn"));
    sa1_textures_released(&list);
}

/* a texture the game made in memory comes with its PVRT chunk in parts, and
 * each part's size is a multiple of 4, as every record's is. an 8 by 8 small
 * VQ with mipmaps holds 150 bytes: 16 codes of 8 bytes, then 1, 1, 4 and 16
 * for its levels. so its chunk ends in 2 zeros, which its length counts, and
 * the parts joined are the chunk whole */
static void texels_in_fours(void)
{
    enum {
        TEXELS = 16 * 8 + 1 + 1 + 4 + 16,
        PADDED = (TEXELS + 3) & ~3,
        HEAD = 16, /* a PVRT chunk's bytes before its texels */
        SMALL_VQ_MIPS = 0x11,
        RGB565 = 1
    };
    static NJS_TEXMEMLIST made = {.globalIndex = 51};
    /* the two bytes past the texels aren't the texture's, and aren't 0 */
    static uint8_t data[PADDED];
    static uint8_t chunk[HEAD + PADDED + 4];
    NJS_TEXINFO info = {.texaddr = data};
    NJS_TEXNAME name = {&info, NJD_TEXATTR_TYPE_MEMORY,
                        (Uint32)(uintptr_t)&made};
    NJS_TEXLIST list = {&name, 1};
    uint32_t id = SA_CAP_NONE;
    uint32_t length;
    uint16_t side;
    size_t held = 0;

    for (int i = 0; i < PADDED; i++) {
        data[i] = (uint8_t)(i + 1);
    }
    info.texsurface.Type = (Uint32)(SMALL_VQ_MIPS << 8 | RGB565) << 16;
    info.texsurface.nWidth = info.texsurface.nHeight = 8;
    sa1_textures_loaded(&list, 1, NULL);
    sa1_textures_released(&list);
    __wrap_frame_wait();
    read_file();
    for (size_t at = 0; at + sizeof(sa_record_head) <= file_size;) {
        sa_record_head head;
        sa_cap_texture texture;
        sa_cap_texels part;

        memcpy(&head, file + at, sizeof head);
        CHECK(head.size >= sizeof head && head.size % 4 == 0);
        if (head.kind == SA_CAP_TEXTURE) {
            memcpy(&texture, file + at, sizeof texture);
            id = texture.global_index == 51 ? texture.texture : id;
        } else if (head.kind == SA_CAP_TEXELS) {
            memcpy(&part, file + at, sizeof part);
            if (part.texture == id) {
                CHECK(held + (head.size - sizeof part) <= sizeof chunk);
                memcpy(chunk + held, file + at + sizeof part,
                       head.size - sizeof part);
                held += head.size - sizeof part;
            }
        }
        at += head.size;
    }
    CHECK(id != SA_CAP_NONE && held == HEAD + PADDED);
    memcpy(&length, chunk + 4, sizeof length);
    memcpy(&side, chunk + 12, sizeof side);
    CHECK(!memcmp(chunk, "PVRT", 4) && length == 8 + PADDED &&
          chunk[8] == RGB565 && chunk[9] == SMALL_VQ_MIPS && side == 8);
    CHECK(!memcmp(chunk + HEAD, data, TEXELS));
    for (int i = TEXELS; i < PADDED; i++) {
        CHECK(chunk[HEAD + i] == 0);
    }
}

/* a picture is a cut where the camera started again since the picture before.
 * so is one where the camera took a new mode with no adjust mode to carry it
 * over */
static void camera_cuts(void)
{
    static const int cut_by[] = {0, 0, 1, 0, 1, 0, 1, 0};
    const uint8_t *at;
    int pictures = 0;

    for (int i = 0; i < 8; i++) {
        reset();
        play_time += 2;
        camera_steps += 2;
        camera_control_record.steps += 2;
        if (i == 2) {
            /* a new mode with no adjust mode */
            camera_control_record.steps = 1;
            camera_control_record.adjust = 0;
        } else if (i == 3) {
            /* a new mode an adjust mode carries over */
            camera_control_record.steps = 1;
            camera_control_record.adjust = 1;
        } else if (i == 4) {
            /* the camera started again at the player */
            camera_steps = 1;
        } else if (i == 6) {
            /* the camera started again just after the picture before, so its
             * steps match the picture's ticks */
            camera_steps = 2;
        }
        draw_model(&model);
        __wrap_frame_wait();
    }
    read_file();
    for (int i = 0; (at = record(SA_CAP_PICTURE, i)); i++) {
        pictures = i + 1;
    }
    CHECK(pictures >= 8);
    for (int i = 0; i < 8; i++) {
        sa_cap_picture picture;

        memcpy(&picture, record(SA_CAP_PICTURE, pictures - 8 + i),
               sizeof picture);
        CHECK((picture.flags == SA_PICTURE_CUT) == cut_by[i]);
    }
}

/* an act's land. the land's task may draw entries 0 and 2, which share a
 * model, but never entry 1. entry 3's model has more meshsets than the adapter
 * has room to name. the motion's keys carry the moving part's one node from 0
 * to 100 along x over ten frames */
static NJS_POINT3 land_points[4] = {
    {-5, -5, 0}, {5, -5, 0}, {-5, 5, 0}, {5, 5, 0}};
static NJS_MODEL land_model = {
    land_points, normals, 4, meshsets, materials, 2, 2, {0, 0, 0}, 8};
static NJS_POINT3 moving_points[4] = {
    {-7, -7, 0}, {7, -7, 0}, {-7, 7, 0}, {7, 7, 0}};
static NJS_MODEL moving_model = {
    moving_points, normals, 4, meshsets, materials, 2, 2, {0, 0, 0}, 10};
static NJS_MESHSET unnamed_sets[4097];
static NJS_MODEL unnamed_model = {
    land_points, normals, 4, unnamed_sets, materials, 4097, 2, {0, 0, 0}, 8};
static NJS_OBJECT land_objects[4] = {
    {0, &land_model, {10, 0, 0}, {0, 0, 0}, {1, 1, 1}, NULL, NULL},
    {0, &land_model, {0, 0, 0}, {0, 0, 0}, {1, 1, 1}, NULL, NULL},
    {0, &land_model, {0, 0, 50}, {0, 0x4000, 0}, {1, 1, 1}, NULL, NULL},
    {0, &unnamed_model, {0, 0, 90}, {0, 0, 0}, {1, 1, 1}, NULL, NULL}};
static struct land_entry land_entries[4] = {
    {.object = &land_objects[0], .flags = (Sint32)LAND_DRAWN},
    {.object = &land_objects[1]},
    {.object = &land_objects[2], .flags = (Sint32)LAND_DRAWN},
    {.object = &land_objects[3], .flags = (Sint32)LAND_DRAWN}};
static NJS_MKEY_F moving_keys[2] = {{0, {0, 0, 0}}, {10, {100, 0, 0}}};
static NJS_MDATA1 moving_data = {{moving_keys}, {2}};
static NJS_MOTION moving_motion = {&moving_data, 20, NJD_MTYPE_POS_0, 1};
static NJS_OBJECT moving_object = {
    0, &moving_model, {1, 2, 3}, {0, 0, 0}, {1, 1, 1}, NULL, NULL};
static NJS_ACTION moving_action = {&moving_object, &moving_motion};
static struct land_motion land_motions[1] = {
    {.frame = 5.0F,
     .speed = 1.0F,
     .frames = 20.0F,
     .play = {.action = &moving_action},
     .texlist = &texlist}};
static struct land_table land = {.count = 4,
                                 .motion_count = 1,
                                 .flags = LAND_MOTIONS | LAND_TEXLIST,
                                 .entries = land_entries,
                                 .motions = land_motions,
                                 .texlist = &texlist};
#define LAND_NAME 0x0100U
/* the parts a scene holds, records.c's SCENE_PARTS */
#define SCENE_PARTS 8192
/* a land of one entry more than a scene holds parts. each entry is a copy of
 * the first land's entry 0 */
static struct land_entry crowd_entries[SCENE_PARTS + 1];
static struct land_table crowd = {
    .count = SCENE_PARTS + 1, .entries = crowd_entries, .texlist = &texlist};
/* the pointers of records.c's struct task, which is all the adapter reads of
 * a task */
#define TASK_POINTERS 12
/* the land's task and another's, as far as the adapter reads a task */
static void *land_task[TASK_POINTERS], *other_task[TASK_POINTERS];
/* what the land's task finds as it draws: the light, and constant attributes
 * its materials take */
static float land_light;
static Uint32 land_attrs;

/* the land's task loads the land while a picture is open, in a frame draw_off
 * drops. the state differs from its draws': no texture list, palette 3, and a
 * light no draw has */
static void land_loads(struct task *task)
{
    (void)task;
    _nj_current_texlist = NULL;
    sa1_live_palette = sa1_live_colors + 3 * SA_PALETTE_ENTRIES * 2;
    sa1_live_light.x = 0.25F;
    native_land_loaded(&land, LAND_NAME, 0.5F);
    sa1_live_light.x = 0.0F;
    _nj_current_texlist = &texlist;
    sa1_live_palette = sa1_live_colors;
}

static void crowd_loads(struct task *task)
{
    (void)task;
    native_land_loaded(&crowd, LAND_NAME + 1, 0.5F);
}

/* the land's task draws the land after the camera moves the view by
 * (-10, 0, 50). entry 0 stands where it was given. entry 2 stands where it was
 * given, but unturned. the moving part has moved on, out of view */
static void land_draws(struct task *task)
{
    (void)task;
    sa1_live_light.x = land_light;
    _nj_control_3d_flag_ = land_attrs ? NJD_CONTROL_3D_CONSTANT_ATTR : 0;
    _nj_constant_attr_or_ = land_attrs;
    _nj_view_matrix_[12] = -10.0F;
    _nj_view_matrix_[14] = 50.0F;
    xmtrx[12] = 10.0F - 10.0F;
    xmtrx[14] = 0.0F + 50.0F;
    draw_models_begin();
    draw_model_unculled(&land_model);
    xmtrx[12] = 0.0F - 10.0F;
    xmtrx[14] = 50.0F + 50.0F;
    draw_model_unculled(&land_model);
    draw_models_end();
    xmtrx[12] = 260.0F - 10.0F;
    xmtrx[14] = 20.0F + 50.0F;
    draw_model_nested(&moving_model);
    _nj_view_matrix_[12] = 0.0F;
    _nj_view_matrix_[14] = 0.0F;
    sa1_live_light.x = 0.0F;
    _nj_control_3d_flag_ = 0;
    _nj_constant_attr_or_ = 0;
}

static void other_draws(struct task *task)
{
    (void)task;
    xmtrx[12] = 10.0F;
    xmtrx[14] = 0.0F;
    draw_model(&land_model);
}

/* the draws from the count-th record of kind to the next of end, or to the
 * file's end. it returns how many, and copies the first most into out */
static int draws_between(uint16_t kind, int count, uint16_t end, sa_draw *out,
                         int most)
{
    const uint8_t *first = record(kind, count);
    const uint8_t *at;
    int found = 0;
    int ended = 0;

    CHECK(first != NULL);
    for (size_t offset = (size_t)(first - file);
         offset + sizeof(sa_record_head) <= file_size;) {
        sa_record_head head;

        at = file + offset;
        memcpy(&head, at, sizeof head);
        ended |= at != first && head.kind == end;
        if (!ended && head.kind == SA_REC_DRAW) {
            if (found < most) {
                memcpy(&out[found], at, sizeof out[0]);
            }
            found++;
        }
        offset += head.size;
    }
    return found;
}

/* forty-eight pictures, more than a name no picture names is kept for. each
 * draws the test's model with its materials clamped in v, as no land draw's
 * are */
static void pictures_pass(void)
{
    for (int i = 0; i < 48; i++) {
        reset();
        _nj_control_3d_flag_ = NJD_CONTROL_3D_CONSTANT_ATTR;
        _nj_constant_attr_or_ = NJD_FLAG_CLAMP_V;
        draw_model(&model);
        __wrap_frame_wait();
    }
    read_file();
}

/* how many records of kind follow the first of after */
static int records_after(uint16_t kind, uint16_t after)
{
    const uint8_t *first = record(after, 0);
    int found = 0;

    CHECK(first != NULL);
    for (size_t at = (size_t)(first - file);
         at + sizeof(sa_record_head) <= file_size;) {
        sa_record_head head;

        memcpy(&head, file + at, sizeof head);
        found += head.kind == kind;
        at += head.size;
    }
    return found;
}

/* the count-th picture's view records. it returns how many, and copies the
 * first most into out and the picture's own record into picture */
static int views_of(int count, sa_cap_picture *picture, sa_picture_view *out,
                    int most)
{
    const uint8_t *first = record(SA_CAP_PICTURE, count);
    int found = 0;

    CHECK(first != NULL);
    memcpy(picture, first, sizeof *picture);
    for (size_t at = (size_t)(first - file);
         at + sizeof(sa_record_head) <= file_size;) {
        sa_record_head head;

        memcpy(&head, file + at, sizeof head);
        if (head.kind == SA_CAP_END) {
            break;
        }
        if (head.kind == SA_REC_VIEW) {
            if (found < most) {
                memcpy(&out[found], file + at, sizeof out[0]);
            }
            found++;
        }
        at += head.size;
    }
    return found;
}

/* a draw of more points than a record holds. each point's x and color are its
 * place among them. so the pictures test can tell which corners a client
 * draws */
#define MANY 4097
static NJS_POINT3 many_points[MANY];
static NJS_COLOR many_colors[MANY];
static NJS_POLYGON_VTX many_vertices[MANY];

/* where among a draw's points corner k of its triangle or line i lies */
static uint32_t corner_of(uint8_t form, uint32_t i, int k)
{
    return form == SA_TRIANGLES_LIST    ? 3 * i + (uint32_t)k
           : form == SA_TRIANGLES_FAN   ? (k ? 2 * i + (uint32_t)k : 0)
           : form == SA_TRIANGLES_LINES ? 2 * i + (uint32_t)k
                                        : i + (uint32_t)k;
}

/* check a draw of count triangles or lines of form in triangles records from
 * the from-th on. it returns how many records hold it. each record's size fits
 * the points its count takes, and each shares the draw's names. a strip's
 * record opens at an even place in the strip. in order, the records give each
 * triangle of the draw once, its corners in order */
static int parts_of(int from, uint8_t form, uint32_t count)
{
    sa_triangles whole;
    uint32_t first = 0;
    int parts = 0;

    do {
        const uint8_t *at = record(SA_CAP_TRIANGLES, from + parts);
        sa_triangles part;

        CHECK(at != NULL);
        memcpy(&part, at, sizeof part);
        if (parts == 0) {
            whole = part;
        }
        CHECK(part.form == form && part.count <= count - first);
        CHECK(sa_triangles_points(form, part.count) <= SA_CAP_POINTS_MOST);
        CHECK(part.head.size ==
              sizeof part +
                  sa_triangles_points(form, part.count) * sizeof(sa_point));
        CHECK(part.instance == whole.instance &&
              part.texture == whole.texture && part.view == whole.view &&
              part.flags == whole.flags && part.depth == whole.depth);
        CHECK(form != SA_TRIANGLES_STRIP || first % 2 == 0);
        for (uint32_t i = 0; i < part.count; i++) {
            for (int k = 0; k < (form == SA_TRIANGLES_LINES ? 2 : 3); k++) {
                sa_point point;

                memcpy(&point,
                       at + sizeof part + corner_of(form, i, k) * sizeof point,
                       sizeof point);
                CHECK(point.at[0] == (float)corner_of(form, first + i, k));
            }
        }
        first += part.count;
        parts++;
    } while (first < count);
    return parts;
}

/* a draw of more points than one record holds goes whole into as many records
 * as it needs. each of five forms goes at three counts: one record's most, one
 * more, and the adapter's most. a draw past that most is lost and counted, as
 * is a negative count */
static void draws_in_parts(void)
{
    const int most = (int)SA_CAP_POINTS_MOST;
    NJS_POINT3COL many = {many_points, many_colors, NULL, MANY};
    /* the counts of each form: one record's, the next, and the adapter's */
    const int strips[3] = {most - 2, most - 1, MANY - 3};
    const int lists[3] = {most / 3, most / 3 + 1, (MANY - 1) / 3};
    const int fans[3] = {(most - 1) / 2, (most - 1) / 2 + 1, (MANY - 2) / 2};
    const int pairs[3] = {most / 2, most / 2 + 1, (MANY - 1) / 2};
    sa_cap_end end;
    int at = 0;
    int ends;

    for (int i = 0; i < MANY; i++) {
        many_points[i] = (NJS_POINT3){(Float)i, 0.0F, 5.0F};
        many_colors[i].color = (Uint32)i;
        many_vertices[i] = (NJS_POLYGON_VTX){(Float)i, 0.0F, 0.5F, (Uint32)i};
    }
    __wrap_frame_wait();
    read_file();
    while (record(SA_CAP_TRIANGLES, at)) {
        at++;
    }
    reset();
    for (int i = 0; i < 3; i++) {
        sa1_record_screen_strip(many_vertices, strips[i] + 2, 0, 0, 0);
    }
    sa1_record_screen_strip(many_vertices, MANY, 0, 0, 0);
    for (int i = 0; i < 3; i++) {
        sa1_record_triangles(&many, strips[i], NJD_FILL | NJD_DRAW_CONNECTED);
    }
    sa1_record_triangles(&many, MANY - 2, NJD_FILL | NJD_DRAW_CONNECTED);
    for (int i = 0; i < 3; i++) {
        sa1_record_triangles(&many, lists[i], NJD_FILL);
    }
    sa1_record_triangles(&many, lists[2] + 1, NJD_FILL);
    sa1_record_triangles(&many, -1, NJD_FILL);
    for (int i = 0; i < 3; i++) {
        sa1_record_triangles(&many, fans[i], NJD_FILL | NJD_DRAW_FAN);
    }
    sa1_record_triangles(&many, fans[2] + 1, NJD_FILL | NJD_DRAW_FAN);
    for (int i = 0; i < 3; i++) {
        sa1_record_lines(&many, pairs[i], 0);
    }
    sa1_record_lines(&many, pairs[2] + 1, 0);
    __wrap_frame_wait();
    read_file();
    /* one record holds the first of each, and two each of the others */
    for (int form = 0; form < 5; form++) {
        static const uint8_t forms[5] = {SA_TRIANGLES_STRIP, SA_TRIANGLES_STRIP,
                                         SA_TRIANGLES_LIST, SA_TRIANGLES_FAN,
                                         SA_TRIANGLES_LINES};
        const int *counts = form < 2    ? strips
                            : form == 2 ? lists
                            : form == 3 ? fans
                                        : pairs;

        for (int i = 0; i < 3; i++) {
            int parts = parts_of(at, forms[form], (uint32_t)counts[i]);

            CHECK(parts == (i == 0 ? 1 : 2));
            at += parts;
        }
    }
    CHECK(record(SA_CAP_TRIANGLES, at) == NULL);
    ends = records_after(SA_CAP_END, SA_CAP_HEAD);
    memcpy(&end, record(SA_CAP_END, ends - 1), sizeof end);
    CHECK(end.draws == 5 * (1 + 2 + 2) && end.lost == 6);
}

/* a model whose bytes just fill a record goes whole. one four bytes longer
 * takes no name, and no mesh comes for it. its draw is lost in each picture
 * that draws it. the next model named takes the next id. a list of the most
 * materials a record holds goes whole too. one material more leaves the draw
 * out before its mesh is named */
static void models_at_the_bound(void)
{
    /* a model of four points with normals and one meshset takes 160 bytes
     * before its strip. the strip's first word counts the words after it */
    const uint32_t room = SA_CAP_RECORD_MOST - (uint32_t)sizeof(sa_cap_mesh);
    const uint32_t most =
        (SA_CAP_RECORD_MOST - (uint32_t)sizeof(sa_cap_materials)) / 4;
    static Sint16 long_strip[1 << 15];
    static NJS_MATERIAL plain[1];
    static NJS_MATERIAL crowd[1 << 14];
    static NJS_MESHSET long_set = {
        .type_matId = NJD_MESHSET_TRIMESH, .nbMesh = 1, .meshes = long_strip};
    static NJS_MESHSET crowd_set = {.nbMesh = 1, .meshes = strip};
    static NJS_MODEL filling = {points, normals, 4,         &long_set, plain,
                                1,      1,       {0, 0, 0}, 2};
    static NJS_MODEL listed = {points, normals, 4,         &crowd_set, crowd,
                               1,      1,       {0, 0, 0}, 2};
    struct sa1_attrs attrs = {0};
    const uint8_t *at;
    sa_record_head head;
    sa_cap_mesh mesh;
    sa_cap_materials list;
    sa_cap_end end;
    sa_draw draw;
    Sint16 last;
    int meshes = 0;
    int draws = 0;
    int ends;

    __wrap_frame_wait();
    read_file();
    while (record(SA_CAP_MESH, meshes)) {
        meshes++;
    }
    while (record(SA_REC_DRAW, draws)) {
        draws++;
    }
    for (int i = 1; i < 1 << 15; i++) {
        long_strip[i] = (Sint16)(i % 4);
    }
    long_strip[0] = (Sint16)((room - 160) / 2 - 1);
    reset();
    sa1_record_model(&filling, SA_SHADE_PALETTE, SA1_RUN, &attrs, 0);
    __wrap_frame_wait();
    /* two words more: four bytes past the record, in two pictures */
    long_strip[0] += 2;
    for (int i = 0; i < 2; i++) {
        reset();
        sa1_record_model(&filling, SA_SHADE_PALETTE, SA1_RUN, &attrs, 0);
        __wrap_frame_wait();
    }
    read_file();
    CHECK((at = record(SA_CAP_MESH, meshes)) != NULL &&
          record(SA_CAP_MESH, meshes + 1) == NULL);
    memcpy(&head, at, sizeof head);
    memcpy(&mesh, at, sizeof mesh);
    CHECK(head.size == sizeof mesh + room);
    /* the strip's last word is the record's last */
    memcpy(&last, at + head.size - 2, sizeof last);
    CHECK(last == long_strip[long_strip[0] - 2]);
    draw = draw_of(draws);
    CHECK(draw.mesh == mesh.mesh && record(SA_REC_DRAW, draws + 1) == NULL);
    ends = records_after(SA_CAP_END, SA_CAP_HEAD);
    for (int i = 0; i < 2; i++) {
        memcpy(&end, record(SA_CAP_END, ends - 1 - i), sizeof end);
        CHECK(end.draws == 0 && end.lost == 1);
    }
    /* a meshset names the last of the most materials a record holds, and then
     * one past it */
    crowd_set.type_matId = (Uint16)(NJD_MESHSET_TRIMESH | (most - 1));
    reset();
    sa1_record_model(&listed, SA_SHADE_PALETTE, SA1_RUN, &attrs, 0);
    crowd_set.type_matId = (Uint16)(NJD_MESHSET_TRIMESH | most);
    sa1_record_model(&listed, SA_SHADE_PALETTE, SA1_RUN, &attrs, 0);
    __wrap_frame_wait();
    read_file();
    /* the one draw's mesh takes the id after the filling model's, and the
     * draw left out named none */
    CHECK((at = record(SA_CAP_MESH, meshes + 1)) != NULL &&
          record(SA_CAP_MESH, meshes + 2) == NULL);
    draw = draw_of(draws + 1);
    CHECK(draw.mesh == mesh.mesh + 1 && record(SA_REC_DRAW, draws + 2) == NULL);
    at = NULL;
    for (int i = 0; record(SA_CAP_MATERIALS, i); i++) {
        memcpy(&list, record(SA_CAP_MATERIALS, i), sizeof list);
        if (list.materials == draw.materials) {
            at = record(SA_CAP_MATERIALS, i);
        }
    }
    CHECK(at != NULL);
    memcpy(&head, at, sizeof head);
    memcpy(&list, at, sizeof list);
    CHECK(list.count == most && head.size == sizeof list + 4 * most);
    memcpy(&end, record(SA_CAP_END, ends), sizeof end);
    CHECK(end.draws == 1 && end.lost == 1);
}

/* a picture's background is the plane's three colors as the picture's end
 * finds them. each is opaque, whatever alpha the game set. colors set after a
 * picture's first draw are that picture's */
static void background_plane(void)
{
    static const uint32_t colors[3] = {0x00112233, 0x80445566, 0x7f778899};
    sa_cap_picture picture;
    int pictures;

    __wrap_frame_wait();
    read_file();
    pictures = records_after(SA_CAP_PICTURE, SA_CAP_HEAD);
    reset();
    draw_model(&model);
    memcpy(sa1_back, colors, sizeof colors);
    __wrap_frame_wait();
    reset();
    draw_model(&model);
    memset(sa1_back, 0, sizeof sa1_back);
    __wrap_frame_wait();
    read_file();
    CHECK(records_after(SA_CAP_PICTURE, SA_CAP_HEAD) == pictures + 2);
    memcpy(&picture, record(SA_CAP_PICTURE, pictures), sizeof picture);
    CHECK(picture.clear == (colors[0] | 0xff000000U) &&
          picture.clear_right == (colors[1] | 0xff000000U) &&
          picture.clear_below == (colors[2] | 0xff000000U));
    memcpy(&picture, record(SA_CAP_PICTURE, pictures + 1), sizeof picture);
    CHECK(picture.clear == 0xff000000U && picture.clear_right == 0xff000000U &&
          picture.clear_below == 0xff000000U);
}

/* the game's camera sets Ninja's view at each step, inside a frame. each draw
 * stands in the game's world and names the view it was made under. a picture
 * gives each view one record, ahead of its first draw. a light turned into
 * each view is one light in the world. a polygon on the screen names the view
 * it found, which places nothing. the picture's own view is the one most
 * placed draws were made under, the first of a tie. a draw under a view past
 * the SA_VIEWS a picture holds is lost */
static void views_in_a_frame(void)
{
    static NJS_POINT2 corners[4] = {{0, 0}, {640, 0}, {640, 480}, {0, 480}};
    static NJS_COLOR colors[4] = {
        {0x80000001}, {0x80000002}, {0x80000003}, {0x80000004}};
    NJS_POINT2COL polygon = {corners, colors, NULL, 4};
    /* the camera's step: the eye goes to (0, 0, 5) and turns a quarter about
     * y. the world's x then lies along the view's -z, and its z along the
     * view's x. the matrix is in Ninja's rows */
    static const float stepped[16] = {0, 0, -1, 0, 0,  1, 0, 0,
                                      1, 0, 0,  0, -5, 0, 0, 1};
    /* the model where it stood, 10 along the world's z, under that view */
    static const float under[16] = {0, 0, -1, 0, 0, 1, 0, 0,
                                    1, 0, 0,  0, 5, 0, 0, 1};
    float opened[16];
    sa_cap_picture picture;
    sa_picture_view views[3];
    sa_draw drawn[4];
    sa_triangles fade;
    sa_cap_end end;
    int pictures;

    __wrap_frame_wait();
    read_file();
    pictures = records_after(SA_CAP_PICTURE, SA_CAP_HEAD);
    memcpy(opened, _nj_view_matrix_, sizeof opened);
    /* before the camera's step, a fade covers the screen. a model 10 ahead is
     * drawn under a light of (0.6, 0, 0.8) */
    reset();
    sa1_record_polygon_2d(&polygon, 4, -2.0F, NJD_FILL | NJD_TRANSPARENT);
    sa1_live_light = (NJS_VECTOR){0.6F, 0.0F, 0.8F};
    draw_model(&model);
    /* the camera steps, and the game turns the light into the new view. the
     * same model is drawn twice where it stood */
    memcpy(_nj_view_matrix_, stepped, sizeof stepped);
    memcpy(xmtrx, under, sizeof under);
    sa1_live_light = (NJS_VECTOR){0.8F, 0.0F, -0.6F};
    draw_model(&model);
    draw_model(&model);
    memcpy(_nj_view_matrix_, opened, sizeof opened);
    sa1_live_light = (NJS_VECTOR){0.0F, 0.0F, 0.0F};
    __wrap_frame_wait();
    read_file();
    CHECK(views_of(pictures, &picture, views, 3) == 2);
    CHECK(views[0].view == 1 && views[1].view == 2);
    CHECK(views[0].value.world_to_view[0] == 1.0F &&
          views[0].value.world_to_view[3] == 0.0F);
    CHECK(views[1].value.world_to_view[2] == 1.0F &&
          views[1].value.world_to_view[3] == -5.0F &&
          views[1].value.world_to_view[8] == -1.0F);
    CHECK(views[1].value.focal[0] == picture.view.focal[0] &&
          views[1].value.near_z == 1.0F && views[1].value.far_z == 60000.0F);
    /* two of the three draws a view places lie under the second */
    CHECK(memcmp(&picture.view, &views[1].value, sizeof picture.view) == 0);
    CHECK(draws_between(SA_CAP_PICTURE, pictures, SA_CAP_END, drawn, 4) == 3);
    CHECK(drawn[0].view == 1 && drawn[1].view == 2 && drawn[2].view == 2);
    CHECK(drawn[0].object_to_world[0] == 1.0F &&
          drawn[0].object_to_world[3] == 0.0F &&
          drawn[0].object_to_world[11] == 10.0F);
    for (int i = 0; i < 12; i++) {
        CHECK(drawn[0].object_to_world[i] == drawn[1].object_to_world[i]);
    }
    CHECK(drawn[0].shade == drawn[1].shade &&
          shade_of(drawn[0].shade).light[0] == 0.6F &&
          shade_of(drawn[0].shade).light[2] == 0.8F);
    memcpy(&fade,
           record(SA_CAP_TRIANGLES,
                  records_after(SA_CAP_TRIANGLES, SA_CAP_HEAD) - 1),
           sizeof fade);
    CHECK((fade.flags & SA_TRIANGLES_SCREEN) && fade.view == 1);
    /* of two views that tie, each with one draw, a picture takes the first as
     * its own. in the next, the first view holds only a polygon on the screen.
     * so the picture takes the second, where its one placed draw lies */
    reset();
    draw_model(&model);
    memcpy(_nj_view_matrix_, stepped, sizeof stepped);
    memcpy(xmtrx, under, sizeof under);
    draw_model(&model);
    memcpy(_nj_view_matrix_, opened, sizeof opened);
    __wrap_frame_wait();
    reset();
    sa1_record_polygon_2d(&polygon, 4, -2.0F, NJD_FILL | NJD_TRANSPARENT);
    memcpy(_nj_view_matrix_, stepped, sizeof stepped);
    memcpy(xmtrx, under, sizeof under);
    draw_model(&model);
    memcpy(_nj_view_matrix_, opened, sizeof opened);
    __wrap_frame_wait();
    read_file();
    CHECK(views_of(pictures + 1, &picture, views, 3) == 2);
    CHECK(memcmp(&picture.view, &views[0].value, sizeof picture.view) == 0);
    CHECK(views_of(pictures + 2, &picture, views, 3) == 2);
    CHECK(memcmp(&picture.view, &views[1].value, sizeof picture.view) == 0);
    /* a draw under a view past the picture's SA_VIEWS is lost, and counted. an
     * outline is lost before it takes a view, so SA_VIEWS models' views fit. a
     * polygon on the screen under the view after them names view SA_VIEWS */
    reset();
    /* a view no model's draw has */
    _nj_view_matrix_[13] = -1.0F;
    sa1_record_polygon_2d(&polygon, 4, -2.0F, NJD_TRANSPARENT);
    for (int i = 0; i <= SA_VIEWS; i++) {
        _nj_view_matrix_[13] = (float)i;
        xmtrx[13] = (float)i;
        draw_model(&model);
    }
    sa1_record_polygon_2d(&polygon, 4, -2.0F, NJD_FILL | NJD_TRANSPARENT);
    memcpy(_nj_view_matrix_, opened, sizeof opened);
    __wrap_frame_wait();
    read_file();
    CHECK(views_of(pictures + 3, &picture, views, 3) == SA_VIEWS);
    memcpy(&end, record(SA_CAP_END, records_after(SA_CAP_END, SA_CAP_HEAD) - 1),
           sizeof end);
    CHECK(end.draws == SA_VIEWS + 1 && end.lost == 2);
    memcpy(&fade,
           record(SA_CAP_TRIANGLES,
                  records_after(SA_CAP_TRIANGLES, SA_CAP_HEAD) - 1),
           sizeof fade);
    CHECK((fade.flags & SA_TRIANGLES_SCREEN) && fade.view == SA_VIEWS);
    /* a picture whose one draw is lost holds no view: its own is the one
     * it opened under */
    reset();
    _nj_view_matrix_[13] = 3.0F;
    sa1_record_polygon_2d(&polygon, 4, -2.0F, NJD_TRANSPARENT);
    memcpy(_nj_view_matrix_, opened, sizeof opened);
    __wrap_frame_wait();
    read_file();
    CHECK(views_of(pictures + 4, &picture, views, 3) == 0);
    CHECK(picture.view.world_to_view[7] == 3.0F);
}

/* a land's load gives the act's scene. its lights come first: the act's light
 * and the sources its file marks used. each entry the task may draw is a part,
 * as is each model of its moving parts. a part takes a number, where it stands
 * and the state the task draws it in. a later draw by the task is the part's
 * own, and another task's is not. until the scene ends, no free names the
 * mesh or the list of materials a part reads. the shade it was given goes as
 * any name does */
static void land_scene(void)
{
    sa_draw part[3];
    sa_draw drawn[5];
    sa_draw kept[3];
    sa_draw dropped;
    sa_scene named;
    sa_cap_scene_end whole;
    sa_lighting lit;
    int pictures;

    reset();
    __wrap_frame_wait();
    draw_model(&model);
    /* the act's file turns its light a quarter about y and a quarter about z.
     * of three sources that hold numbers, the second is unused */
    sources_file.ry = sources_file.rz = 0x4000;
    sources_file.color[0] = 0.5F;
    sources_file.color[1] = 0.25F;
    sources_file.color[2] = 0.125F;
    sources_file.specular = 0.8F;
    sources_file.diffuse = 1.0F;
    sources_file.ambient = 0.25F;
    for (int i = 3; i < 8; i += 2) {
        sources_file.sources[i].used = i != 5;
        for (int j = 0; j < 3; j++) {
            sources_file.sources[i].pos[j] = (Float)(10 * i + j);
            sources_file.sources[i].color[j] = (Float)(i + j) / 16.0F;
            sources_file.sources[i].touch_pos[j] = (Float)(100 * i + j);
        }
        sources_file.sources[i].range = (Float)(20 * i);
        sources_file.sources[i].rate = (Float)i / 8.0F;
    }
    native_task_step((struct task *)land_task, land_loads);
    draw_off = TRUE;
    __wrap_frame_wait();
    draw_off = FALSE;
    /* a draw ahead of the camera's step opens the next picture */
    reset();
    draw_model(&model);
    native_task_step((struct task *)land_task, land_draws);
    native_task_step((struct task *)other_task, other_draws);
    __wrap_frame_wait();
    read_file();
    /* the frame's picture went, and the scene stayed whole */
    CHECK(record(SA_CAP_SCENE, 0) != NULL && record(SA_CAP_SCENE, 1) == NULL);
    memcpy(&named, record(SA_CAP_SCENE, 0), sizeof named);
    CHECK(named.scene == LAND_NAME);
    CHECK(draws_between(SA_CAP_SCENE, 0, SA_CAP_SCENE_END, part, 3) == 3);
    /* entry 3, whose meshsets the adapter has no room to name, is lost, and
     * counted */
    memcpy(&whole, record(SA_CAP_SCENE_END, 0), sizeof whole);
    CHECK(whole.scene == LAND_NAME && whole.parts == 3 && whole.lost == 1);
    /* the scene gives its lights once, inside it. the light points straight
     * down, turned about z and then about y. it takes the file's color and
     * strengths. the two used sources follow in the file's order. nothing
     * comes of the unused one between them, or of a source's second
     * position */
    CHECK(record(SA_CAP_SCENE_LIGHTING, 0) > record(SA_CAP_SCENE, 0) &&
          record(SA_CAP_SCENE_LIGHTING, 0) < record(SA_CAP_SCENE_END, 0) &&
          record(SA_CAP_SCENE_LIGHTING, 1) == NULL);
    memcpy(&lit, record(SA_CAP_SCENE_LIGHTING, 0), sizeof lit);
    CHECK(lit.head.size == sizeof lit);
    CHECK(fabsf(lit.direction[0]) < 1e-4F && fabsf(lit.direction[1]) < 1e-4F &&
          fabsf(lit.direction[2] + 1.0F) < 1e-4F);
    CHECK(lit.color[0] == 0.5F && lit.color[1] == 0.25F &&
          lit.color[2] == 0.125F && lit.specular == 0.8F &&
          lit.diffuse == 1.0F && lit.ambient == 0.25F);
    CHECK(lit.count == 2);
    for (int i = 0; i < 2; i++) {
        int from = i ? 7 : 3;

        CHECK(lit.source[i].point[0] == (Float)(10 * from) &&
              lit.source[i].point[2] == (Float)(10 * from + 2) &&
              lit.source[i].range == (Float)(20 * from) &&
              lit.source[i].color[0] == (Float)from / 16.0F &&
              lit.source[i].color[2] == (Float)(from + 2) / 16.0F &&
              lit.source[i].rate == (Float)from / 8.0F);
    }
    CHECK(lit.source[2].range == 0.0F && lit.source[2].point[0] == 0.0F);
    /* an entry is numbered by its place in the table, the moving part after
     * them */
    CHECK(part[0].instance == (SA_INSTANCE_SCENE | 0) &&
          part[1].instance == (SA_INSTANCE_SCENE | 2) &&
          part[2].instance == (SA_INSTANCE_SCENE | 4));
    CHECK(part[0].flags == 0 && part[1].flags == 0 &&
          part[2].flags == SA_DRAW_MOVING);
    CHECK(part[0].payload == SA_NO_PAYLOAD && part[0].mesh == part[1].mesh &&
          part[0].mesh != part[2].mesh);
    /* an entry stands and turns as its object does. the moving part stands
     * where its keys put it at its frame, not at its node's own place */
    CHECK(part[0].object_to_world[0] == 1.0F &&
          part[0].object_to_world[3] == 10.0F);
    CHECK(fabsf(part[1].object_to_world[0]) < 1e-4F &&
          fabsf(fabsf(part[1].object_to_world[2]) - 1.0F) < 1e-4F &&
          part[1].object_to_world[11] == 50.0F);
    CHECK(part[2].object_to_world[3] == 50.0F &&
          part[2].object_to_world[7] == 0.0F &&
          part[2].object_to_world[11] == 0.0F);
    /* the land's own constant material, texlist and palette, whatever
     * Ninja's state held at the load */
    CHECK(part[0].tint == 0x007f7f7fU);
    CHECK(material_of(&part[0], 0).texture != SA_CAP_NONE);
    CHECK(shade_of(part[0].shade).palette == 0);
    /* the task's draws name the parts, in view or out of it. of two parts that
     * share a model, a draw names the one nearest it. each draw stands where
     * it was drawn in the game's world, under the picture's second view.
     * another task's draw of the model is its own, under the view the picture
     * opened under */
    CHECK(draws_between(SA_CAP_SCENE_END, 0, 0, drawn, 5) == 5);
    CHECK(drawn[1].instance == part[0].instance &&
          drawn[1].object_to_world[3] == 10.0F &&
          drawn[1].object_to_world[11] == 0.0F);
    CHECK(drawn[2].instance == part[1].instance &&
          drawn[2].object_to_world[3] == 0.0F &&
          drawn[2].object_to_world[11] == 50.0F);
    CHECK(drawn[3].instance == part[2].instance &&
          (drawn[3].flags & SA_DRAW_OUT) &&
          drawn[3].object_to_world[3] == 260.0F &&
          drawn[3].object_to_world[11] == 20.0F);
    CHECK(drawn[0].view == 1 && drawn[1].view == 2 && drawn[2].view == 2 &&
          drawn[3].view == 2 && drawn[4].view == 1);
    CHECK(!(drawn[4].instance & (SA_INSTANCE_SCENE | SA_INSTANCE_UNOWNED)) &&
          drawn[4].object_to_world[3] == 10.0F &&
          drawn[4].object_to_world[11] == 0.0F);
    /* each task gets its creation record at its first draw. the land's task
     * gets one though its draws name parts, so two follow the scene */
    CHECK(records_after(SA_REC_TASK_CREATE, SA_CAP_SCENE_END) == 2);
    /* for 48 pictures, none names what the parts read. a part keeps its mesh
     * and its list of materials. the shade it was given under goes as any name
     * does, since a reader copied it */
    pictures_pass();
    CHECK(!freed(SA_KIND_MESH, part[0].mesh) &&
          !freed(SA_KIND_MESH, part[2].mesh));
    CHECK(!freed(SA_KIND_MATERIAL, part[0].materials) &&
          !freed(SA_KIND_MATERIAL, part[2].materials));
    CHECK(freed(SA_REC_SHADE, part[0].shade));
    /* a kept picture draws the parts under a light of its own, their materials
     * double-sided. from then, each part reads that draw's list. the list the
     * part was given with goes, as the draw's shade does */
    pictures = records_after(SA_CAP_PICTURE, SA_CAP_HEAD);
    land_light = 0.5F;
    land_attrs = NJD_FLAG_DOUBLE_SIDE;
    reset();
    native_task_step((struct task *)land_task, land_draws);
    __wrap_frame_wait();
    /* a dropped picture draws them under another light, clamped in u, which no
     * reader sees. the parts read what they read before, and that draw's list
     * goes */
    land_light = 0.75F;
    land_attrs = NJD_FLAG_DOUBLE_SIDE | NJD_FLAG_CLAMP_U;
    reset();
    native_task_step((struct task *)land_task, land_draws);
    draw_off = TRUE;
    __wrap_frame_wait();
    draw_off = FALSE;
    land_light = 0.0F;
    land_attrs = 0;
    pictures_pass();
    CHECK(draws_between(SA_CAP_PICTURE, pictures, SA_CAP_END, kept, 3) == 3);
    CHECK(kept[0].instance == part[0].instance &&
          kept[2].instance == part[2].instance);
    CHECK(kept[0].materials != part[0].materials &&
          kept[2].materials != part[2].materials);
    CHECK(material_of(&kept[0], 0).flags & SA_MAT_DOUBLE);
    /* the dropped picture's list is the last named, so its id is the highest.
     * its draws went with the picture, but its names lie in the next picture's
     * records */
    dropped = kept[0];
    for (int i = 0; record(SA_CAP_MATERIALS, i); i++) {
        sa_cap_materials list;

        memcpy(&list, record(SA_CAP_MATERIALS, i), sizeof list);
        dropped.materials = list.materials > dropped.materials
                                ? list.materials
                                : dropped.materials;
    }
    CHECK(dropped.materials > kept[2].materials &&
          (material_of(&dropped, 0).flags & SA_MAT_CLAMP_U));
    CHECK(freed(SA_KIND_MATERIAL, part[0].materials) &&
          freed(SA_KIND_MATERIAL, part[2].materials));
    CHECK(!freed(SA_KIND_MATERIAL, kept[0].materials) &&
          !freed(SA_KIND_MATERIAL, kept[2].materials));
    CHECK(freed(SA_KIND_MATERIAL, dropped.materials));
    CHECK(freed(SA_REC_SHADE, kept[0].shade));
    CHECK(!freed(SA_KIND_MESH, part[0].mesh) &&
          !freed(SA_KIND_MESH, part[2].mesh));
    /* the task lets the land go. the scene ends, the task's draws are its own
     * again, and the names age as any do */
    native_land_released();
    reset();
    native_task_step((struct task *)land_task, land_draws);
    pictures_pass();
    CHECK(record(SA_CAP_SCENE_ENDED, 0) != NULL);
    memcpy(&named, record(SA_CAP_SCENE_ENDED, 0), sizeof named);
    CHECK(named.scene == LAND_NAME);
    CHECK(draws_between(SA_CAP_SCENE_ENDED, 0, 0, drawn, 4) >= 4);
    CHECK(!(drawn[0].instance & (SA_INSTANCE_SCENE | SA_INSTANCE_UNOWNED)) &&
          drawn[0].instance == drawn[2].instance);
    CHECK(freed(SA_KIND_MESH, part[0].mesh) &&
          freed(SA_KIND_MESH, part[2].mesh));
    CHECK(freed(SA_KIND_MATERIAL, kept[0].materials) &&
          freed(SA_KIND_MATERIAL, kept[2].materials));
    /* the scene of a land with more entries than SCENE_PARTS takes
     * SCENE_PARTS, and counts the rest lost */
    for (int i = 0; i < SCENE_PARTS + 1; i++) {
        crowd_entries[i] = land_entries[0];
    }
    /* here sources_file marks no source used, and leaves the light unturned */
    sources_file.ry = sources_file.rz = 0;
    sources_file.sources[3].used = sources_file.sources[7].used = FALSE;
    native_task_step((struct task *)land_task, crowd_loads);
    native_land_released();
    reset();
    draw_model(&model);
    __wrap_frame_wait();
    read_file();
    CHECK(record(SA_CAP_SCENE_END, 1) != NULL);
    memcpy(&whole, record(SA_CAP_SCENE_END, 1), sizeof whole);
    CHECK(whole.scene == LAND_NAME + 1 && whole.parts == SCENE_PARTS &&
          whole.lost == 1);
    /* its lights are the one light alone, straight down */
    CHECK(record(SA_CAP_SCENE_LIGHTING, 1) != NULL &&
          record(SA_CAP_SCENE_LIGHTING, 2) == NULL);
    memcpy(&lit, record(SA_CAP_SCENE_LIGHTING, 1), sizeof lit);
    CHECK(lit.count == 0 && lit.source[0].range == 0.0F);
    CHECK(lit.direction[0] == 0.0F && lit.direction[1] == -1.0F &&
          lit.direction[2] == 0.0F && lit.color[0] == 0.5F &&
          lit.ambient == 0.25F);
}

/* a task of the test's own draws one model twice, with a pose of it between. a
 * pose is a model of its shape whose points lie elsewhere. from its second
 * picture, it first draws a model of another shape */
static void *things_task[TASK_POINTERS];
static NJS_POINT3 posed_points[4] = {
    {-2, -2, 0}, {2, -2, 0}, {-2, 2, 0}, {2, 2, 0}};
static NJS_MODEL posed_model, lone_model;
static int lone_drawn;

static void things_draw(struct task *task)
{
    (void)task;
    if (lone_drawn) {
        draw_model(&lone_model);
    }
    draw_model(&model);
    draw_model(&posed_model);
    draw_model(&model);
}

/* a draw names its drawer as instance, and its mesh's shape as thing. nth
 * counts the drawer's draws of that thing before it in the picture */
static void names_of_things(void)
{
    sa_draw first[3], second[4], unowned[1];
    int pictures;

    posed_model = lone_model = model;
    posed_model.points = posed_points;
    lone_model.nbMeshset = 1;
    __wrap_frame_wait();
    read_file();
    pictures = records_after(SA_CAP_PICTURE, SA_CAP_HEAD);
    reset();
    native_task_step((struct task *)things_task, things_draw);
    __wrap_frame_wait();
    reset();
    lone_drawn = 1;
    native_task_step((struct task *)things_task, things_draw);
    __wrap_frame_wait();
    reset();
    draw_model(&posed_model);
    __wrap_frame_wait();
    read_file();
    CHECK(draws_between(SA_CAP_PICTURE, pictures, SA_CAP_END, first, 3) == 3);
    CHECK(draws_between(SA_CAP_PICTURE, pictures + 1, SA_CAP_END, second, 4) ==
          4);
    CHECK(draws_between(SA_CAP_PICTURE, pictures + 2, SA_CAP_END, unowned, 1) ==
          1);
    /* one drawer and one shape: the pose is another mesh but the same thing.
     * so the three draws are nth 0, 1 and 2, as drawn */
    CHECK(first[0].mesh == first[2].mesh && first[0].mesh != first[1].mesh);
    CHECK(!(first[0].instance & (SA_INSTANCE_SCENE | SA_INSTANCE_UNOWNED)) &&
          first[0].instance == first[1].instance &&
          first[1].instance == first[2].instance);
    CHECK(first[0].thing != 0 && first[1].thing == first[0].thing &&
          first[2].thing == first[0].thing);
    CHECK(first[0].nth == 0 && first[1].nth == 1 && first[2].nth == 2);
    /* the next picture names each as the first did. the model of another
     * shape, drawn ahead of them, is another thing with nth 0 */
    for (int i = 0; i < 3; i++) {
        CHECK(second[i + 1].instance == first[i].instance &&
              second[i + 1].thing == first[i].thing &&
              second[i + 1].nth == first[i].nth);
    }
    CHECK(second[0].instance == first[0].instance && second[0].thing != 0 &&
          second[0].thing != first[0].thing && second[0].nth == 0);
    /* a draw no task owns names its mesh's shape all the same. it is nth 0,
     * the picture's first unowned draw of that shape */
    CHECK((unowned[0].instance & SA_INSTANCE_UNOWNED) &&
          unowned[0].thing == first[0].thing && unowned[0].nth == 0);
}

/* start music on a file of size bytes, which plays. it returns the state the
 * game reads two seconds of vsyncs later */
static Sint32 stream_later(ADXT music, const uint8_t *file, size_t size)
{
    static char name[] = "MUSIC.ADX";

    disc_bytes = file;
    disc_size = size;
    ADXT_StartFname(music, name);
    CHECK(ADXT_GetStat(music) == ADXT_STAT_PLAYING);
    native_vsyncs += 120;
    return ADXT_GetStat(music);
}

/* a stream reads as ended once its file's samples have played, unless the
 * file holds a loop. the adapter takes a loop from the header as
 * sa_audio_stream says the engine does. so the game reads no stream the
 * engine loops as ended, and none it plays out as playing on */
static void stream_loops(void)
{
    /* an ADX file's header of 48 bytes, the least that holds a loop: its
     * mark, its data from 48, a rate of 44100, as many samples and version
     * 3. its loop, of count 1 and type 0, runs from sample 256 to the last */
    static const uint8_t looped[0x30] = {
        [0] = 0x80,  [3] = 0x2c,    [10] = 0xac,  [11] = 0x44,
        [14] = 0xac, [15] = 0x44,   [0x12] = 3,   [0x17] = 1,
        [0x1e] = 1,  [0x26] = 0xac, [0x27] = 0x44};
    /* the header with no loop: a count of 0, whatever its type of 1 says */
    static const uint8_t plain[0x30] = {
        [0] = 0x80,  [3] = 0x2c,    [10] = 0xac,  [11] = 0x44,
        [14] = 0xac, [15] = 0x44,   [0x12] = 3,   [0x1b] = 1,
        [0x1e] = 1,  [0x26] = 0xac, [0x27] = 0x44};
    /* looped with one byte changed, and whether it loops still */
    static const struct {
        int at;
        uint8_t byte;
        int loops;
    } changed[] = {
        {0x12, 5, 1}, /* another version */
        {0x1b, 1, 1}, /* a type of 1 */
        {0x17, 2, 0}, /* a count of 2 */
        {0x03, 0x28, 0}, /* data from 44, short of a loop's header */
        {0x1d, 1, 0}, /* a first sample past the loop's end */
        {0x27, 0x45, 0}, /* an end past the file's samples */
    };
    unsigned long vsyncs = native_vsyncs;
    uint8_t file[sizeof looped];
    ADXT music;

    native_sound_engine = (struct sa_engine *)&heard;
    CHECK((music = ADXT_Create(2, NULL, 0)) != NULL);
    CHECK(stream_later(music, looped, sizeof looped) == ADXT_STAT_PLAYING);
    CHECK(stream_later(music, plain, sizeof plain) == ADXT_STAT_PLAYEND);
    for (size_t i = 0; i < sizeof changed / sizeof changed[0]; i++) {
        memcpy(file, looped, sizeof file);
        file[changed[i].at] = changed[i].byte;
        CHECK(stream_later(music, file, sizeof file) ==
              (changed[i].loops ? ADXT_STAT_PLAYING : ADXT_STAT_PLAYEND));
    }
    /* a file of fewer than 48 bytes holds no loop */
    CHECK(stream_later(music, looped, sizeof looped - 1) == ADXT_STAT_PLAYEND);
    ADXT_Destroy(music);
    native_vsyncs = vsyncs;
    native_sound_engine = NULL;
}

/* a playing stream stops where it is started again on what the adapter can't
 * play. such is a missing file, bytes of no ADX file, or a voice past the
 * archive's files. the game reads it stopped, and the engine hears one stop. a
 * stopped stream sends no further stop */
static void stream_refused(void)
{
    /* an ADX file's header of 44 bytes, the least the adapter starts: its
     * mark, a rate of 44100 and as many samples */
    static const uint8_t adx[0x2c] = {
        [0] = 0x80, [10] = 0xac, [11] = 0x44, [14] = 0xac, [15] = 0x44};
    static const uint8_t noise[sizeof adx] = {0};
    static const struct {
        const uint8_t *bytes;
        int voice;
    } refused[3] = {{NULL, 0}, {noise, 0}, {adx, 1}};
    static char name[] = "MUSIC.ADX";
    ADXT music;

    native_sound_engine = (struct sa_engine *)&heard;
    memset(&heard, 0, sizeof heard);
    disc_size = sizeof adx;
    CHECK((music = ADXT_Create(2, NULL, 0)) != NULL);
    for (int i = 0; i < 3; i++) {
        disc_bytes = adx;
        ADXT_StartFname(music, name);
        CHECK(ADXT_GetStat(music) == ADXT_STAT_PLAYING &&
              heard.streams == i + 1 && heard.stops == i);
        disc_bytes = refused[i].bytes;
        if (refused[i].voice) {
            ADXT_StartAfs(music, 0, 0);
        } else {
            ADXT_StartFname(music, name);
        }
        CHECK(ADXT_GetStat(music) == ADXT_STAT_STOP && heard.streams == i + 1 &&
              heard.stops == i + 1);
    }
    /* stopped, it starts on no file again and is destroyed */
    disc_bytes = NULL;
    ADXT_StartFname(music, name);
    ADXT_Destroy(music);
    CHECK(heard.streams == 3 && heard.stops == 3);
    native_sound_engine = NULL;
}

/* a movie's sound starts from memory, its length read from its header. bytes
 * that start as an ADX file's mark does and then differ start nothing. nor
 * does a header that says its file ends before a header does. each stops what
 * the stream played before, and the game reads it stopped, as movie.c does to
 * end its list. a stopped stream sends no further stop. movie.c first asks
 * sa1_adx_whole whether its bytes reach that length. bytes a byte short of
 * it don't, nor do bytes short of the header's count of samples, nor bytes
 * that aren't an ADX file */
static void stream_from_memory(void)
{
    /* an ADX file of one 18-byte block for each of two channels. its header
     * holds its mark, the data's offset less 4, the channels, a rate of 44100
     * and 32 samples */
    static uint8_t adx[0x2c + 2 * 18] = {[0] = 0x80,  [3] = 0x28,  [7] = 2,
                                         [10] = 0xac, [11] = 0x44, [15] = 32};
    static uint8_t half_marked[sizeof adx] = {[0] = 0x80, [1] = 0x01};
    static uint8_t short_file[sizeof adx] = {[0] = 0x80};
    static uint8_t *const refused[2] = {half_marked, short_file};
    ADXT sound;

    native_sound_engine = (struct sa_engine *)&heard;
    memset(&heard, 0, sizeof heard);
    CHECK((sound = ADXT_Create(2, NULL, 0)) != NULL);
    for (int i = 0; i < 2; i++) {
        ADXT_StartMem(sound, adx);
        CHECK(ADXT_GetStat(sound) == ADXT_STAT_PLAYING &&
              heard.streams == i + 1 && heard.stops == i);
        ADXT_StartMem(sound, refused[i]);
        CHECK(ADXT_GetStat(sound) == ADXT_STAT_STOP && heard.streams == i + 1 &&
              heard.stops == i + 1);
    }
    ADXT_StartMem(sound, half_marked);
    ADXT_Destroy(sound);
    CHECK(heard.streams == 2 && heard.stops == 2);
    native_sound_engine = NULL;
    CHECK(sa1_adx_whole(adx, sizeof adx) && sa1_adx_whole(adx, sizeof adx + 1));
    CHECK(!sa1_adx_whole(adx, sizeof adx - 1) && !sa1_adx_whole(adx, 15) &&
          !sa1_adx_whole(half_marked, sizeof half_marked));
}

/* a multi unit's download gives each bank its place in sound memory, then its
 * bytes. it sends the bytes through a block it takes and lets go. with no
 * block free, each bank takes its place and none its bytes */
static void multi_unit(void)
{
    /* a unit of one bank: the programs' bank 3, 8 bytes at 0x1000 in sound
     * memory. the bytes lie after the unit's header and the bank's record */
    static uint8_t unit[0x20 + 0x20 + 8] = {'S', 'M', 'L', 'T', 1, [8] = 1};
    static const int32_t bank[6] = {
        SDE_DATA_TYPE_MIDI_PRG_BANK, 3, 0x1000, 8, 0x40, 8};
    /* more blocks than the adapter keeps */
    SDMEMBLK blocks[65];
    int count = 0;

    native_sound_engine = (struct sa_engine *)&heard;
    memset(&heard, 0, sizeof heard);
    memcpy(unit + 0x20, bank, sizeof bank);
    /* take every block the adapter has. the first holds the unit, and the
     * second the bank's bytes for the game's own download */
    while (count < 65 && sdMemBlkCreate(&blocks[count]) == SDE_ERR_NOTHING) {
        count++;
    }
    CHECK(count > 1 && count < 65);
    sdMemBlkSetPrm(blocks[0], unit, sizeof unit, SDD_MEMBLK_NO_FUNC, NULL);
    sdMemBlkSetPrm(blocks[1], unit + 0x40, 8, SDD_MEMBLK_NO_FUNC, NULL);
    /* the bank has no place until the unit's download gives it one. with no
     * block free, the download gives no bytes */
    CHECK(sdBankDownload(blocks[1], SDE_DATA_TYPE_MIDI_PRG_BANK, 3) ==
          SDE_ERR_UNKNOW_NUM);
    CHECK(sdMultiUnitDownload(blocks[0]) == SDE_ERR_NOTHING &&
          heard.banks == 0);
    /* the game's own download of the bank goes to that place */
    CHECK(sdBankDownload(blocks[1], SDE_DATA_TYPE_MIDI_PRG_BANK, 3) ==
              SDE_ERR_NOTHING &&
          heard.banks == 1 && heard.bank.kind == SA_BANK_PROGRAMS &&
          heard.bank.number == 3 && heard.bank.address == 0x1000 &&
          heard.bank.bytes == 8);
    /* with a block free, the unit's download gives the bank's bytes too. the
     * block is free again after */
    sdMemBlkDestroy(blocks[--count]);
    heard.bank.data = NULL;
    CHECK(sdMultiUnitDownload(blocks[0]) == SDE_ERR_NOTHING &&
          heard.banks == 2 && heard.bank.data == unit + 0x40 &&
          heard.bank.bytes == 8);
    CHECK(sdMemBlkCreate(&blocks[count]) == SDE_ERR_NOTHING);
    for (int i = 0; i <= count; i++) {
        sdMemBlkDestroy(blocks[i]);
    }
    native_sound_engine = NULL;
}

/* the drawers' things a picture names, records.c's NAME_SLOTS / 2 */
#define NAMES_MOST (1 << 16)
static void *passing_task[TASK_POINTERS];

static void passing_draws(struct task *task)
{
    (void)task;
    draw_model(&model);
}

/* a picture names the things of NAMES_MOST drawers. a draw past them is lost
 * and counted before it asks for a view. so a view that draw alone was made
 * under gets no record. each task here draws the model once, out of view, and
 * ends. main runs this last, so no other test reads through its draws */
static void names_at_the_bound(void)
{
    sa_cap_picture picture;
    sa_picture_view views[2];
    sa_cap_end end;
    int pictures;

    __wrap_frame_wait();
    read_file();
    pictures = records_after(SA_CAP_PICTURE, SA_CAP_HEAD);
    reset();
    xmtrx[14] = -100.0F;
    for (int i = 0; i <= NAMES_MOST; i++) {
        /* the last draws under a view of its own */
        _nj_view_matrix_[13] = i == NAMES_MOST ? 1.0F : 0.0F;
        native_task_step((struct task *)passing_task, passing_draws);
        native_task_destroy((struct task *)passing_task);
    }
    _nj_view_matrix_[13] = 0.0F;
    __wrap_frame_wait();
    read_file();
    CHECK(views_of(pictures, &picture, views, 2) == 1);
    memcpy(&end, record(SA_CAP_END, records_after(SA_CAP_END, SA_CAP_HEAD) - 1),
           sizeof end);
    CHECK(end.draws == NAMES_MOST && end.lost == 1);
}

int main(int argc, char **argv)
{
    (void)argc;
    path = argv[1];
    setup();
    statics_found();
    native_capture_open(path);
    __wrap_frame_wait();
    state_at_the_call();
    state_left_behind();
    what_the_call_sent();
    borrowed_data();
    names_forgotten();
    textures_of_files();
    texels_in_fours();
    screen_polygons();
    lines();
    screen_strip();
    sprite_scales();
    ninja_forms();
    constant_alpha();
    filter_and_adjust();
    white_after_unlit();
    palette_versions();
    full_drawer();
    drawers_out();
    camera_cuts();
    background_plane();
    views_in_a_frame();
    draws_in_parts();
    models_at_the_bound();
    names_of_things();
    land_scene();
    stream_loops();
    stream_refused();
    stream_from_memory();
    multi_unit();
    names_at_the_bound();
    return 0;
}
