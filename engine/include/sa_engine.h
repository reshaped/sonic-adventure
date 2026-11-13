/* the engine's C ABI, version 0: the calls a client such as the game makes,
 * and their records. ENGINE.md#c-abi gives its rules. version 0 changes with
 * the engine, so a client builds against the engine of its own commit.
 *
 * it knows no game. a game's header adds its own records and calls, under
 * the numbers this one leaves. engine/sa1/sa_sa1.h is SA1's.
 *
 * C17 and C++17 compile it alike. a record has one layout on every target,
 * and the asserts at the end check each record's size. a descriptor names
 * memory by pointer, so it lives in one process and is never written out */
#ifndef SA_ENGINE_H
#define SA_ENGINE_H

#include <stddef.h>
#include <stdint.h>

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error the records are little-endian, as every target is
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SA_ENGINE_ABI_VERSION 0U

/* a call that can fail returns a result, and writes its outputs only when
 * it returns SA_OK. 0 and above are outcomes, and below 0 errors. an error
 * leaves the engine as the call found it */
typedef int32_t sa_result;
enum {
    SA_OK = 0,
    SA_EMPTY = 1, /* nothing to take: no event or pad */
    SA_ERR_INVALID = -1, /* an argument outside the call's contract */
    SA_ERR_HANDLE = -2, /* a handle null, gone or of another kind */
    SA_ERR_FULL = -3, /* a bounded store has no room. nothing grows */
    SA_ERR_UNSUPPORTED = -4, /* the backend lacks the service */
    SA_ERR_VERSION = -5, /* the client built against another ABI */
    SA_ERR_IO = -6, /* storage failed */
    SA_ERR_MISSING = -7 /* storage holds no such name */
};

/* a handle holds its object's kind in bits 28 to 31. a generation takes bits
 * 20 to 27, and a slot those below. SA_NULL names nothing. a handle whose
 * object is gone gives SA_ERR_HANDLE, never a fault, until its slot's
 * generation wraps */
typedef uint32_t sa_handle;
#define SA_NULL 0U
#define SA_HANDLE_KIND(handle) ((uint32_t)(handle) >> 28)
enum {
    SA_KIND_MESH = 1,
    SA_KIND_TEXTURE = 2,
    SA_KIND_MATERIAL = 3,
    SA_KIND_REQUEST = 4 /* a storage request, until its event */
};

/* a name's id: 32-bit FNV-1a over its bytes. a function's or a static
 * object's id is its C name's. a file's is its path on the disc, as
 * "SONICADV/ADV00.PRS" */
static inline uint32_t sa_name_id(const char *name)
{
    uint32_t id = 2166136261U;

    while (*name != '\0') {
        id = (id ^ (uint8_t)*name++) * 16777619U;
    }
    return id;
}

/* what a record names in place of a pointer: nothing, or a number the game
 * chose. no record holds an address */
typedef struct sa_ref {
    uint32_t space; /* SA_SPACE_* */
    uint32_t id; /* within the space, as below */
    uint32_t offset; /* 0 under both spaces */
} sa_ref;
enum {
    SA_SPACE_NONE = 0, /* nothing: id 0 */
    SA_SPACE_VALUE = 1 /* a number the game chose, in id */
    /* 2 to 10 and 15 are reserved */
};

/* a record opens with a head, which gives its kind and its size */
typedef struct sa_record_head {
    uint16_t kind; /* SA_REC_* */
    uint16_t size; /* bytes, the head's own among them: a multiple of 4 */
} sa_record_head;
enum {
    /* 1, 2, 11, 13 and 15 are reserved */
    /* 3 to 10, 23, 24 and 64 on are a game's */
    SA_REC_PAD = 12, /* sa_input_sample's answer */
    SA_REC_AUDIO = 14, /* sa_audio_submit's command */
    SA_REC_PICTURE = 16, /* a picture's, as each after it is */
    SA_REC_DRAW = 17,
    SA_REC_PAYLOAD = 18,
    SA_REC_SHADE = 19,
    SA_REC_SPRITE = 20,
    SA_REC_TRIANGLES = 21,
    SA_REC_FOG = 22,
    SA_REC_LIGHTS = 25
};

/* a pad as the game's step sampled it, which is the input service's answer.
 * the buttons are in Shinobi's PDD_DGT bits */
typedef struct sa_pad {
    sa_record_head head;
    uint8_t port;
    uint8_t present; /* 0 where the port holds no pad */
    uint16_t reserved; /* 0 */
    uint32_t on; /* held */
    uint32_t press; /* down since the sample before */
    uint32_t release; /* up since the sample before */
    int16_t x1;
    int16_t y1;
    int16_t x2;
    int16_t y2;
    uint16_t r;
    uint16_t l;
} sa_pad;

/* the audio service's command: one call the game made of its sound library
 * or its streams. commands keep the order of the calls. 1 to 10 are
 * reserved */
enum {
    /* a midi message on port key.id, as the game's sound library takes one.
     * value holds its status and two data bytes, from the low byte up */
    SA_AUDIO_MIDI = 11,
    /* port key.id plays sequence sound & 0xff of sequence bank sound >> 8
     * at priority; stops; or takes value as its volume over updates */
    SA_AUDIO_SEQUENCE = 12,
    SA_AUDIO_SEQUENCE_STOP = 13,
    SA_AUDIO_SEQUENCE_VOLUME = 14,
    /* stream key.id stops, or plays at value tenths of a decibel */
    SA_AUDIO_STREAM_STOP = 15,
    SA_AUDIO_STREAM_VOLUME = 16,
    /* the AICA's DSP takes fx program sound and fx output set value, of
     * the loaded fx banks. fx output key.id moves its level from the set's
     * by value, in eighths of a step. it moves its pan by sound. qsound
     * channel key.id takes place value & 0x7f, and value & 0x80 marks the
     * place changed. each value is as the library sends it */
    SA_AUDIO_FX_PROGRAM = 17,
    SA_AUDIO_FX_OUTPUT = 18,
    SA_AUDIO_QSOUND = 19,
    /* the output plays in stereo where value is 0. where value is 1 it
     * plays in mono, and every pan sounds centred */
    SA_AUDIO_SPACE = 20,
    /* stream key.id's next start is heard value milliseconds after it */
    SA_AUDIO_STREAM_WAIT = 21
};
typedef struct sa_audio_command {
    sa_record_head head;
    uint16_t kind; /* SA_AUDIO_* */
    uint16_t flags; /* 0 */
    int32_t sound; /* a sequence, an fx program or a pan, as kind says */
    /* the port, stream, fx output or channel kind names, a number */
    sa_ref key;
    int32_t priority; /* a sequence's */
    int32_t value; /* as kind says */
    int32_t updates; /* the updates a volume takes to change, or 0 */
    float pos[3]; /* 0 */
    float radius; /* 0 */
    int32_t result; /* 0 */
} sa_audio_command;

/* a picture: what one shows, at the tick and step whose state it shows. its
 * matrices are render-only, so no game step reads them. the screen is 640
 * by 480 whatever the display. view space has x right, y down and z ahead.
 * a point shows at center + focal * (x, y) / z */
typedef struct sa_view {
    float world_to_view[12]; /* a 3 by 4 matrix, by rows */
    float focal[2];
    float center[2];
    /* Ninja's near and far clips. a facing sprite whose z lies outside
     * them drops; a far of 0 is none */
    float near_z;
    float far_z;
} sa_view;
/* a picture's flags */
enum {
    SA_PICTURE_CUT = 0x1 /* the camera cut: blend nothing from before */
};
typedef struct sa_picture {
    sa_record_head head;
    uint32_t tick;
    uint16_t step;
    uint16_t flags; /* SA_PICTURE_* */
    /* the view to look through, in the world the picture's draws stand in.
     * a game may change its view inside a frame. so each draw names the
     * view it was made under: this one, or one sa_present_view gave */
    sa_view view;
    /* the background's ARGB at the screen's top left, top right and bottom
     * left. it is a plane across the screen, as njSetBackColor gives one */
    uint32_t back[3];
} sa_picture;
/* a picture's fog, as the game gives the PowerVR it. a pixel's 1/z times
 * density is held to 1 to 256. its exponent and its mantissa's top four
 * bits pick an entry of table. the rest of the mantissa moves from the
 * entry's high alpha to its low. the pixel blends to color by that alpha,
 * as a draw's fog mode asks */
typedef struct sa_fog {
    sa_record_head head;
    uint32_t color; /* ARGB */
    float density;
    uint16_t table[128]; /* each two 8-bit alphas, high then low */
} sa_fog;
/* a material's list. the lists compose in this order. within a list, draws
 * keep the game's order, but a 2D draw's pri orders SA_LIST_SCREEN's. a mesh's
 * meshsets may lie in different lists, so the list is its material's, never
 * the draw's */
enum {
    SA_LIST_OPAQUE = 1,
    SA_LIST_PUNCH = 2, /* cut out: drawn or not, per pixel */
    /* see-through: composed per pixel, as the PowerVR sorts them */
    SA_LIST_BLEND = 3,
    SA_LIST_SCREEN = 4 /* 2D, in screen coordinates, over the rest */
};
/* a material as a draw took it, after Ninja's constant attributes and
 * constant material. it gives the meaning of what the PowerVR's polygon
 * header and its points' colors held */
enum {
    SA_MAT_TEXTURE = 0x0001, /* textured; else the points' colors alone */
    SA_MAT_BLEND = 0x0002, /* the blend list, its points the alpha below */
    SA_MAT_ENV = 0x0004, /* uv from the normal turned into view space */
    SA_MAT_DOUBLE = 0x0008, /* drawn from both sides */
    SA_MAT_CULL = 0x0010, /* a single-sided face that turns away is dropped */
    /* lit by no light. under SA_SHADE_LINEAR and _LIGHTS the points take
     * diffuse, and under SA_SHADE_RAMP it changes nothing. a game's own
     * mode reads it as its header says */
    SA_MAT_UNLIT = 0x0020,
    SA_MAT_SPECULAR = 0x0040, /* a highlight, as a game's mode gives one */
    SA_MAT_DECAL = 0x0080, /* the texel alone, not times the point's color */
    SA_MAT_OFFSET = 0x0100, /* the point's offset color adds to the texel */
    SA_MAT_CLAMP_U = 0x0200,
    SA_MAT_CLAMP_V = 0x0400,
    SA_MAT_FLIP_U = 0x0800, /* u mirrors past 1 */
    SA_MAT_FLIP_V = 0x1000,
    /* under a game's mode, the points take the draw's tint */
    SA_MAT_CONSTANT = 0x2000,
    SA_MAT_WHITE = 0x4000 /* the points take white, as a game's mode says */
};
/* blend factors, in the PowerVR's order */
enum {
    SA_BLEND_ZERO = 0,
    SA_BLEND_ONE = 1,
    SA_BLEND_OTHER = 2, /* the other side's color */
    SA_BLEND_INV_OTHER = 3,
    SA_BLEND_SRC_ALPHA = 4,
    SA_BLEND_INV_SRC_ALPHA = 5,
    SA_BLEND_DST_ALPHA = 6,
    SA_BLEND_INV_DST_ALPHA = 7
};
enum { SA_FILTER_POINT = 0, SA_FILTER_BILINEAR = 1, SA_FILTER_TRILINEAR = 2 };
typedef struct sa_material {
    sa_handle texture; /* SA_NULL for none */
    uint32_t specular; /* ARGB */
    uint16_t flags; /* SA_MAT_* */
    uint8_t alpha; /* the points' alpha under SA_MAT_BLEND */
    uint8_t filter; /* SA_FILTER_* */
    uint8_t src; /* SA_BLEND_*, under SA_MAT_BLEND */
    uint8_t dst;
    uint8_t mip_bias; /* the mipmap's D adjust, in quarters: 4 is 1.0 */
    /* under SA_SHADE_LIGHTS, the highlight's power as Ninja steps it. step
     * 0 is power 0, and a step past 16 is 128. steps 1 to 16 are 0.5, 1,
     * 1.5, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96 and 128. it is 0
     * elsewhere */
    uint8_t exponent;
    /* ARGB, the color SA_SHADE_RAMP and _LINEAR scale. under SA_SHADE_RAMP
     * a textured material's is Ninja's light tint. another's is its diffuse
     * color times the tint. under SA_SHADE_LIGHTS the lights scale it */
    uint32_t diffuse;
} sa_material;
/* a draw's depth test, and its fog mode. the test passes where the draw's
 * 1/z compares with the stored one as named */
enum {
    SA_DEPTH_NEVER = 0,
    SA_DEPTH_LESS = 1,
    SA_DEPTH_EQUAL = 2,
    SA_DEPTH_LEQUAL = 3,
    SA_DEPTH_GREATER = 4,
    SA_DEPTH_NOTEQUAL = 5,
    SA_DEPTH_GEQUAL = 6,
    SA_DEPTH_ALWAYS = 7
};
enum {
    SA_FOG_TABLE = 0,
    SA_FOG_VERTEX = 1,
    SA_FOG_NONE = 2,
    SA_FOG_TABLE2 = 3
};
/* how a draw's points take their colors. 0 is a game's own mode, which its
 * header names */
enum {
    /* Ninja's: a point takes its material's color times ambient plus
     * diffuse times the dot, held to 1. the dot's term counts only where it
     * is above 0 */
    SA_SHADE_RAMP = 1,
    /* Ninja's simple drawer: the same without the floor on the dot */
    SA_SHADE_LINEAR = 2,
    /* Ninja's full drawer: a point takes the sum of the lights its shade's
     * sa_lights gives. the material's diffuse and specular scale each */
    SA_SHADE_LIGHTS = 3
};
enum { SA_SHADE_NO_DEPTH_WRITE = 0x1 };
/* what draws shade by and how their pixels meet the picture. most of a
 * picture's draws share a few of these. so one goes in once, and draws
 * name it */
typedef struct sa_shade {
    sa_record_head head;
    uint32_t shade; /* what the draws name it by */
    /* in world space, at the length the game gave it, which scales the dot */
    float light[3];
    float ambient; /* under SA_SHADE_RAMP and _LINEAR */
    float diffuse;
    uint8_t mode; /* SA_SHADE_* */
    uint8_t palette; /* under a game's mode, which of its tables */
    uint8_t depth; /* SA_DEPTH_* */
    uint8_t fog; /* SA_FOG_* */
    uint8_t flags; /* SA_SHADE_NO_DEPTH_WRITE */
    uint8_t reserved[3]; /* 0 */
} sa_shade;
/* the lights of Ninja's queue, in its order, as its full drawer lights by
 * them. they go in before a draw of their SA_SHADE_LIGHTS shade. a type
 * is Ninja's, in SA_LIGHT_* bits. a backend refuses a type it doesn't
 * light */
enum {
    SA_LIGHT_AMBIENT = 0x01, /* adds ambient */
    SA_LIGHT_DIRECTION = 0x02, /* adds diffuse by the dot, where it lights */
    /* adds diffuse by the dot with the unit way to its point. it lights
     * where the distance's square is under far. past near, it falls by near
     * over the distance's square */
    SA_LIGHT_POINT = 0x04,
    /* adds diffuse as SA_LIGHT_DIRECTION does, and specular where the dot's
     * square passes outer. the highlight is whole past inner. short of
     * inner it is the square over inner, to the material's power */
    SA_LIGHT_DIRECTION_SPECULAR = 0x10,
    /* given with SA_LIGHT_POINT: the point light adds the same with it or
     * without */
    SA_LIGHT_SIMPLE = 0x80
};
typedef struct sa_light {
    uint32_t type;
    float point[3]; /* in world space */
    float direction[3]; /* in world space, the way it shines, unit */
    float ambient[3]; /* RGB, the light's own */
    float diffuse[3];
    float specular[4]; /* ARGB: the highlight's alpha adds to the point's */
    float inner, outer; /* the squares of the cosines that bound the edge */
    float near, far; /* the squares of the distances that bound the reach */
} sa_light;
/* the most lights an sa_lights holds */
enum { SA_LIGHTS = 8 };
typedef struct sa_lights {
    sa_record_head head;
    uint32_t shade; /* the shade that takes them */
    uint32_t count;
    sa_light light[SA_LIGHTS];
} sa_lights;
/* a mesh's draw carries the game's name for what it draws, so later pictures
 * can find it. the name is the drawer's instance, the draw's thing and its
 * nth. a backend keeps them as given. an instance is a task's serial. a draw
 * no task owns takes a number with SA_INSTANCE_UNOWNED set. all such draws
 * count as one drawer's. a scene's part has an instance of its own, which
 * names the part whole. a thing is the game's number for what the draw shows,
 * the same in each picture. a game's adapter says what it numbers. SA1's
 * numbers the shape of the mesh the draw sends, which a pose leaves alone. nth
 * is the draw's place, from 0, among its picture's draws of that thing by that
 * drawer. so a name belongs to one draw of its picture. where a drawer draws a
 * thing as often in two pictures, the names pair its draws. where the counts
 * differ, no name tells which draw came or went. a draw whose thing is 0 names
 * none. a backend that keeps instances names it by its mesh and place among
 * the drawer's draws of it. it names a sprite and triangles by their place
 * too, since they carry only who draws */
#define SA_INSTANCE_UNOWNED 0x80000000U
#define SA_NO_PAYLOAD 0xffffffffU
/* a draw's flags */
enum {
    /* one of the game's own tests left the model out, so the call sent
     * nothing. a picture of what the game showed draws nothing for it */
    SA_DRAW_OUT = 0x1,
    /* the mesh, a resource, is one the game's code wrote. a backend that
     * keeps a scene keeps a copy of it, as of a payload. it names the mesh
     * by its shape */
    SA_DRAW_WRITTEN = 0x2
};
typedef struct sa_draw {
    sa_record_head head;
    sa_handle mesh;
    /* a material for each of the mesh's own, in its order, as the call took
     * them. they hold the constant attributes and texlist of the moment */
    sa_handle materials;
    /* a task's serial, or a number with SA_INSTANCE_UNOWNED or _SCENE set */
    uint32_t instance;
    /* bumps where the instance's motion breaks, so nothing blends across */
    uint16_t generation;
    uint16_t flags; /* SA_DRAW_* */
    /* the constant material the call set, ARGB, or 0xffffffff for none */
    uint32_t tint;
    /* under a mesh of SA_NULL, a model the game's code wrote for this call.
     * it names an sa_present_copy of the model's bytes in SA_FORMAT_MESH's
     * form. else it is SA_NO_PAYLOAD */
    uint32_t payload;
    uint32_t shade; /* the sa_shade it takes */
    /* a 3 by 4 matrix, by rows, to the game's world, whatever view the
     * draw was made under */
    float object_to_world[12];
    /* the view it was made under, through which the game's own picture
     * shows it. it is 0 for the picture's own, or an sa_present_view's id. a
     * record that ends before it names 0 */
    uint8_t view;
    uint8_t reserved[3]; /* 0 */
    /* which of the drawer's things it is, or 0. nth is its place among the
     * picture's draws of that thing. a record that ends before them names 0 */
    uint32_t thing;
    uint32_t nth;
} sa_draw;
/* bytes copied at a draw's call, as posed or written vertices, which the
 * picture keeps until it retires. its bytes follow, to head.size */
typedef struct sa_payload {
    sa_record_head head;
    uint32_t payload; /* what the draws name it by */
} sa_payload;

/* a sprite: a textured rectangle facing the view at a point, as
 * njDrawSprite3D draws one. under SA_SPRITE_SCREEN it lies on the screen in
 * pixels, as njDrawSprite2D draws the HUD and the lens flare */
enum {
    SA_SPRITE_ANGLE = 0x01,
    SA_SPRITE_COLOR = 0x02,
    SA_SPRITE_HFLIP = 0x04,
    SA_SPRITE_VFLIP = 0x08,
    SA_SPRITE_SCALE = 0x10,
    SA_SPRITE_ALPHA = 0x20,
    SA_SPRITE_SCREEN = 0x40
};
typedef struct sa_sprite {
    sa_record_head head;
    sa_handle texture; /* SA_NULL for none */
    uint32_t instance; /* as sa_draw's */
    /* under SA_SPRITE_SCREEN, its first two rows take each turned corner's
     * pixel to the screen's. they carry the 2D draws' 16:9 squeeze, where
     * those draws squeeze */
    float object_to_world[12];
    float at[3]; /* in the object's space, or in pixels with z 0 */
    float scale[2];
    float corner[2][2]; /* the rectangle's, around at, before the scale */
    float uv[2][2]; /* the corners', from 0 to 1 */
    float angle; /* in radians, under SA_SPRITE_ANGLE */
    /* added to its depth in view space alone. under SA_SPRITE_SCREEN it is
     * Ninja's pri, and 1/z is 1 where pri is over -1, and -1/pri otherwise.
     * that 1/z orders the screen's sprites as 1/z orders the blend list's
     * fragments */
    float bias;
    uint32_t color; /* ARGB, under SA_SPRITE_COLOR */
    uint8_t flags; /* SA_SPRITE_* */
    uint8_t depth; /* SA_DEPTH_* */
    uint8_t fog; /* SA_FOG_* */
    uint8_t src; /* SA_BLEND_* */
    uint8_t dst;
    uint8_t filter; /* SA_FILTER_* */
    /* as sa_draw's, which under SA_SPRITE_SCREEN places nothing */
    uint8_t view;
    uint8_t reserved; /* 0 */
} sa_sprite;

/* triangles a task writes point by point, as njDrawTriangle3D draws an
 * effect's ribbon, which no file holds. on the screen they are what
 * njDrawTexture, njDrawPolygon and njDrawPolygon2D draw, as a fade or a
 * panel. a strip's triangle i takes points i to i + 2. a fan's takes point 0
 * and points 2i + 1 and 2i + 2. a list's takes points 3i to 3i + 2. lines
 * are njDrawLine3D's, count of them, and line i runs from point 2i to point
 * 2i + 1. each is a strip on the screen 1.575 pixels across, as Ninja sends
 * one */
enum {
    SA_TRIANGLES_LIST = 0,
    SA_TRIANGLES_STRIP = 1,
    SA_TRIANGLES_FAN = 2,
    SA_TRIANGLES_LINES = 3
};
enum {
    SA_TRIANGLES_TEXTURE = 0x01, /* else the points' colors alone */
    SA_TRIANGLES_BLEND = 0x02, /* the blend list */
    /* at in pixels, and at[2] the depth 1/z stands for, as SA_SPRITE_SCREEN's;
     * object_to_world is unread */
    SA_TRIANGLES_SCREEN = 0x04
};
typedef struct sa_point {
    float at[3]; /* in the object's space, or on the screen */
    float uv[2];
    uint32_t color; /* ARGB */
} sa_point;
typedef struct sa_triangles {
    sa_record_head head;
    sa_handle texture; /* SA_NULL for none */
    uint32_t instance; /* as sa_draw's */
    float object_to_world[12];
    uint16_t count; /* its triangles, or its lines */
    uint8_t form; /* SA_TRIANGLES_LIST, _STRIP, _FAN or _LINES */
    uint8_t flags; /* SA_TRIANGLES_* */
    uint8_t depth; /* SA_DEPTH_* */
    uint8_t fog; /* SA_FOG_* */
    uint8_t src; /* SA_BLEND_*, under SA_TRIANGLES_BLEND */
    uint8_t dst;
    uint8_t filter; /* SA_FILTER_* */
    /* as sa_draw's, which under SA_TRIANGLES_SCREEN places nothing */
    uint8_t view;
    uint8_t reserved[2]; /* 0 */
} sa_triangles;
/* the points a form takes for count triangles, or lines */
static inline uint32_t sa_triangles_points(uint8_t form, uint32_t count)
{
    return form == SA_TRIANGLES_LIST    ? 3 * count
           : form == SA_TRIANGLES_FAN   ? 2 * count + 1
           : form == SA_TRIANGLES_LINES ? 2 * count
                                        : count + 2;
}

/* the runtime. its core takes all its memory from the client as the engine
 * is made, and allocates none after. a backend may take its platform's
 * memory beside the block. the PC's takes it for its scene and its device.
 * one thread calls an engine */
typedef struct sa_engine sa_engine;

/* what the client asks for. the stores' sizes bound the engine, and a full
 * one refuses with SA_ERR_FULL */
typedef struct sa_engine_desc {
    uint32_t abi; /* SA_ENGINE_ABI_VERSION, as the client built */
    uint32_t size; /* sizeof (sa_engine_desc), as the client built */
    uint32_t game; /* the game's number, as its header names it: not 0 */
    uint32_t flags; /* 0 */
    uint32_t arena; /* bytes a picture's draws and payloads may take */
    uint32_t pictures; /* the pictures in flight, from 1 */
    uint32_t resources; /* the most live resources */
    uint32_t events; /* the event queue's room */
    /* the backend's own options, as its header gives them, or null */
    const void *platform;
} sa_engine_desc;

/* the ABI the engine was built against */
uint32_t sa_engine_abi(void);
/* the bytes an engine needs for desc */
sa_result sa_engine_memory(const sa_engine_desc *desc, uint32_t *bytes);
/* make an engine in memory, aligned to 32 bytes, which stays the engine's
 * until destroy returns. desc is read during the call alone */
sa_result sa_engine_create(const sa_engine_desc *desc, void *memory,
                           uint32_t bytes, sa_engine **engine);
/* wait for the backend to retire every picture and resource, and end */
void sa_engine_destroy(sa_engine *engine);

/* what the engine reports, in the order it happened */
enum {
    /* a released resource is gone: its bytes are the client's again */
    SA_EVENT_RETIRED = 1,
    /* a storage request finished: result is the bytes moved, or an error */
    SA_EVENT_STORAGE = 2,
    /* a picture showed: tick and result give its tick and step, and
     * time_ns when */
    SA_EVENT_PRESENTED = 3,
    /* the platform asks the game to end, as a window's close does */
    SA_EVENT_QUIT = 4
};
typedef struct sa_event {
    uint32_t kind; /* SA_EVENT_* */
    sa_handle handle; /* what it concerns, or SA_NULL */
    int32_t result;
    /* a picture's tick, or else the last frame sa_clock_tick gave */
    uint32_t tick;
    uint64_t time_ns; /* the clock's time when it happened */
} sa_event;
/* take the oldest event: SA_OK, or SA_EMPTY where none waits */
sa_result sa_event_poll(sa_engine *engine, sa_event *event);

/* the platform's monotonic time, in nanoseconds */
uint64_t sa_clock_ns(sa_engine *engine);
/* a logical frame began. presentation reads ticks from here, and never
 * counts its own */
sa_result sa_clock_tick(sa_engine *engine, uint32_t tick);
/* sleep until the clock reaches time_ns; past it, return at once */
sa_result sa_clock_wait(sa_engine *engine, uint64_t time_ns);

/* sample the pad in port now: SA_OK, or SA_EMPTY where none is there */
sa_result sa_input_sample(sa_engine *engine, uint32_t port, sa_pad *pad);
/* shake the pad in port for ms from now. low and high are its motors'
 * strengths, from 0 to 65535. the shaking falls to none over its last
 * fade_ms. it returns SA_OK, or SA_EMPTY where no pad there shakes. a call
 * replaces the shaking before it. a call of 0 ms changes nothing and only
 * answers */
sa_result sa_input_rumble(sa_engine *engine, uint32_t port, uint16_t low,
                          uint16_t high, uint32_t ms, uint32_t fade_ms);

/* begin moving a save. name is read during the call alone. buffer or data
 * stays the client's to keep, unchanged, until the request's
 * SA_EVENT_STORAGE */
sa_result sa_storage_read(sa_engine *engine, const char *name, void *buffer,
                          uint32_t bytes, sa_handle *request);
sa_result sa_storage_write(sa_engine *engine, const char *name,
                           const void *data, uint32_t bytes,
                           sa_handle *request);

/* queue commands, copied during the call, to sound at the logical time the
 * last sa_clock_tick gave. a queue with no room for them all takes none of
 * them, and the call gives SA_ERR_FULL */
sa_result sa_audio_submit(sa_engine *engine, const sa_audio_command *commands,
                          uint32_t count);
/* a bank of the sound library's, in its own format, as SEGA's driver reads
 * it from sound memory */
enum {
    SA_BANK_PROGRAMS = 1, /* SMPB: a midi channel's programs */
    SA_BANK_SEQUENCES = 2, /* SMSB */
    SA_BANK_OTHER = 3 /* fx programs and outputs, and one-shots */
};
typedef struct sa_audio_bank {
    uint32_t kind; /* SA_BANK_* */
    uint32_t number;
    uint32_t address; /* in the driver's 2 MB of sound memory */
    uint32_t bytes;
    const void *data; /* read during the call alone */
} sa_audio_bank;
/* place a bank, copied during the call, for the commands queued after it.
 * SA_ERR_INVALID refuses a bank that passes sound memory. it refuses a
 * program bank that lacks its SMPB mark or its 0x30-byte header. it refuses
 * one whose tables pass its bytes too */
sa_result sa_audio_load(sa_engine *engine, const sa_audio_bank *bank);
/* play stream from the start of an ADX file's bytes, copied during the
 * call. it starts in order with the commands. a stream whose file holds a
 * loop plays to the loop's end and then the loop, over and over, until it is
 * stopped. any other ends after its file's samples. a file holds a loop where
 * it has 48 bytes or more, its data starts no sooner, and its count of loops
 * is 1. the file's header is big-endian. its data starts 4 past the 16 bits
 * at 2. the count is the 16 bits at 0x16. the 32 bits at 0x1c give the loop's
 * first sample, and those at 0x24 the sample it ends before. a loop that ends
 * at or before its first sample is none. nor is one that ends past the file's
 * samples, which the 32 bits at 12 count */
sa_result sa_audio_stream(sa_engine *engine, uint32_t stream, const void *data,
                          uint32_t bytes);

/* what a resource's bytes hold. each is a layout of the engine's own and no
 * file's, but SA_FORMAT_PVR holds a file's bytes */
enum {
    /* SA_KIND_MESH: sa_mesh, its points, its normals if it has any, then each
     * meshset as sa_meshset says */
    SA_FORMAT_MESH = 1,
    /* SA_KIND_TEXTURE: sa_pixels, then RGBA rows, top first */
    SA_FORMAT_PIXELS = 2,
    /* SA_KIND_TEXTURE: a PVR as a PVM holds it, GBIX chunk or not */
    SA_FORMAT_PVR = 3,
    /* SA_KIND_MATERIAL: sa_material after sa_material, one for each of a
     * mesh's own, which sa_draw.materials names */
    SA_FORMAT_MATERIALS = 4
};
/* a mesh: points as three floats, normals the same */
typedef struct sa_mesh {
    uint32_t points;
    uint32_t normals; /* points, or 0 */
    uint32_t meshsets;
    uint32_t materials; /* how long a draw's list of materials must be */
} sa_mesh;
/* a meshset's triangles. its corners' point indices come first, as uint16_t
 * padded to 4 bytes. under SA_MESHSET_UV two floats a corner follow. three
 * corners make a triangle, each turning the way the strip it came from turns
 * on the screen */
enum { SA_MESHSET_UV = 0x1 };
typedef struct sa_meshset {
    uint16_t material; /* its place in a draw's list */
    uint16_t flags; /* SA_MESHSET_* */
    uint32_t corners; /* a multiple of 3 */
} sa_meshset;
typedef struct sa_pixels {
    uint32_t width;
    uint32_t height;
} sa_pixels;
/* an asset the engine draws from. data is borrowed, and stays unchanged
 * until release's SA_EVENT_RETIRED. that event follows every picture and
 * every piece of GPU work that reads it */
typedef struct sa_resource_desc {
    uint32_t kind; /* SA_KIND_MESH, _TEXTURE or _MATERIAL */
    uint32_t file; /* the name id of the file it came from */
    uint32_t format; /* SA_FORMAT_*, which a backend may refuse */
    uint32_t bytes;
    const void *data;
} sa_resource_desc;
sa_result sa_resource_create(sa_engine *engine, const sa_resource_desc *desc,
                             sa_handle *resource);
/* retire resource once nothing reads it. its handle is gone at once */
sa_result sa_resource_release(sa_engine *engine, sa_handle resource);

/* a picture's calls, and those a game's header adds to it, come between
 * sa_present_begin and sa_present_end. each call copies what it is given, or
 * reads it during the call alone. so a draw's matrix or a payload's bytes may
 * change once the call returns. a draw that names a resource gone, or a shade
 * the picture never had, is refused with SA_ERR_HANDLE. of draws given
 * together, none is taken. a resource a picture took stays until the picture
 * retires, whatever is released after */
sa_result sa_present_begin(sa_engine *engine, const sa_picture *picture);
/* the picture's fog, where it has one */
sa_result sa_present_fog(sa_engine *engine, const sa_fog *fog);
/* copy bytes into the picture's arena, and name them for its draws */
sa_result sa_present_copy(sa_engine *engine, const void *data, uint32_t bytes,
                          uint32_t *payload);
/* add a shade for the picture's draws after it. a later one with the same
 * id takes its place, and a new picture starts with none */
sa_result sa_present_shade(sa_engine *engine, const sa_shade *shade);
/* the lights of the SA_SHADE_LIGHTS shade with lights->shade's id, for its
 * draws after them */
sa_result sa_present_lights(sa_engine *engine, const sa_lights *lights);
sa_result sa_present_draw(sa_engine *engine, const sa_draw *draws,
                          uint32_t count);
sa_result sa_present_sprite(sa_engine *engine, const sa_sprite *sprites,
                            uint32_t count);
/* points holds sa_triangles_points of triangles' form and count */
sa_result sa_present_triangles(sa_engine *engine, const sa_triangles *triangles,
                               const sa_point *points);
/* a draw of a game's own geometry. the game's drawer reads it during the call
 * alone, in the transform live at the call. a descriptor: it holds pointers,
 * and is never written out.
 *
 * a backend's header defines one of two names for how long the backend reads a
 * draw's borrowed bytes. a game's adapter tests it by #ifdef. under
 * SA_BORROW_CALL the backend reads them during the call alone, and takes a
 * live draw itself. under SA_BORROW_PICTURE it reads them until the picture
 * retires, and draws only resources and payloads. the game's adapter then
 * takes the live draw. at the call it copies what the game's drawer would read
 * into resources and payloads.
 *
 * the backend's header gives the call too, since a program compiles against
 * one backend.
 *
 *   sa_result sa_present_live(sa_engine *engine, const sa_draw_live *draw);
 *
 * it is declared where the adapter gives it out of line. it is defined inline
 * where the backend draws in the call, so a call compiles into the drawer */
typedef struct sa_draw_live {
    const void *geometry; /* the game's model, laid out as format says */
    const void *state; /* the words its drawer reads, taken at the call */
    uint32_t format; /* the game's, as its header names it */
    uint32_t instance; /* as sa_draw's */
} sa_draw_live;
/* a kept draw, in 16 bytes: a draw a later picture draws again without the
 * game's call. the Dreamcast's second picture of a frame so draws step 0's.
 * it is never written out. what the call read lies in its picture's store,
 * in tables. a run of draws that shares a state or a matrix adds one entry
 * for it. the fields name the entries. each draw's instance lies apart, in
 * an array of words that a search for one reads alone */
typedef struct sa_draw_kept {
    uint8_t kind; /* as its keeper names it */
    uint8_t view; /* its world to view, among its keeper's */
    uint16_t state; /* the words its drawer read */
    /* object to view where the backend draws in the call, as the
     * Dreamcast's. object to world is then the view's inverse times it. it
     * is object to world where draws come in the game's world, as the PC's */
    uint16_t matrix;
    uint16_t shade; /* how its points take their colors */
    uint32_t attr; /* the call's own word */
    /* what it draws, as kind says: an entry, a resource's handle or the
     * game's geometry's address. it holds an address only where addresses
     * take 32 bits. a picture that draws it again reads the geometry as it
     * then lies */
    uint32_t geometry;
} sa_draw_kept;
/* the picture is whole: the backend shows it when its schedule says */
sa_result sa_present_end(sa_engine *engine);
/* an instance's owner ended, in a picture or between two. a backend may
 * draw pictures between the game's. it then shows an instance that one
 * picture drew and the next didn't at its last place. it doesn't where the
 * instance's owner ended while that picture was open */
sa_result sa_present_ended(sa_engine *engine, uint32_t instance);

/* the pictures' aspect and rate. the game asks for them once a frame, and the
 * engine applies them from the next. a rate counts pictures a second, never
 * steps. 30 is the game's own, and 60 adds pictures but no step. applied is
 * what the engine gives, which may hold 60 at 30. a mode the backend never
 * shows gives SA_ERR_UNSUPPORTED */
enum { SA_ASPECT_4_3 = 0, SA_ASPECT_16_9 = 1 };
typedef struct sa_mode {
    uint8_t aspect; /* SA_ASPECT_* */
    uint8_t rate; /* 30 or 60 */
    /* asked holds 0. applied holds 0 or a letter of the backend's own for why
     * it held the rate */
    uint8_t held;
    uint8_t reserved; /* 0 */
} sa_mode;
sa_result sa_present_mode(sa_engine *engine, const sa_mode *asked,
                          sa_mode *applied);
/* the aspect, SA_ASPECT_*, of the window the game's tests of what is in
 * view take. the window is play's and not the picture's. where the picture
 * shows wider, the engine covers its sides with bands */
sa_result sa_present_window(sa_engine *engine, uint32_t aspect);

/* a scene: what an act holds, whether a picture draws it or not. a game
 * that draws only what lies near its camera gives it whole, once, as the act
 * loads. sa_scene_begin names it, and its shades, parts and lights follow.
 * sa_scene_end closes it, and sa_scene_ended ends it as the act goes. scene
 * is the game's name for the act, as its header says */
enum { SA_REC_SCENE = 26 };
typedef struct sa_scene {
    sa_record_head head;
    uint32_t scene;
} sa_scene;
/* a part's instance is the game's own number for the part, as its header
 * says. it has this bit set and SA_INSTANCE_UNOWNED clear. a picture's draw
 * of the part gives the same */
#define SA_INSTANCE_SCENE 0x40000000U
/* a draw's flag, beside SA_DRAW_OUT and SA_DRAW_WRITTEN. it marks a scene's
 * part the game moves, given where it stood as the scene was given. the
 * game leaves a part without it where it was given. a reader may so build
 * such a part once, before a picture draws it */
enum { SA_DRAW_MOVING = 0x4 };
/* a part is the draw the game would make of it, an sa_draw. it gives its mesh,
 * its materials, its tint, a shade the scene was given and its place. its
 * instance is of SA_INSTANCE_SCENE. its payload is SA_NO_PAYLOAD, since a
 * part's mesh is a resource. its view is unread, since a part stands in the
 * world under no view. each call copies what it is given, or reads it during
 * the call alone. a part borrows the resources it names as a picture's draw
 * does. it stays the scene's until the scene ends, whether a picture draws it
 * or not. a resource released sooner ends each part that still reads it. a
 * part given under an instance the scene holds takes that part's place. a
 * picture's draw under a part's instance is the part drawn. the part takes the
 * mesh, materials, shade and place of the picture's first draw of it. neither
 * that draw nor a later one makes an instance of its own.
 *
 * a scene's calls are apart from a picture's, and may come while a picture is
 * open. a picture's shade is none of the scene's, and a scene's none of a
 * picture's. a scene that ends while a picture is open ends its parts then.
 * that picture's draws of them leave no instance. one scene stays at a time.
 * SA_ERR_INVALID refuses sa_scene_begin while a scene is open or stays. it
 * refuses sa_scene_shade, sa_scene_part and sa_scene_end with none open, and
 * sa_scene_ended with none staying or under another name. it refuses a part
 * whose instance isn't SA_INSTANCE_SCENE's or that has a payload. it refuses a
 * shade of SA_SHADE_LIGHTS, since no call gives a scene's shade its lights.
 * SA_ERR_HANDLE refuses a part that names a resource gone, or a shade the open
 * scene never had. SA_ERR_FULL refuses a shade or parts past the room a
 * backend has for them. of parts given together, none is taken. a backend that
 * keeps no scene holds each call to these rules, keeps no part and answers
 * SA_OK. a backend that draws in the call, as the Dreamcast's, is given none
 * of these calls */
sa_result sa_scene_begin(sa_engine *engine, const sa_scene *scene);
/* add a shade for the open scene's parts after it. a later one with the
 * same id takes its place */
sa_result sa_scene_shade(sa_engine *engine, const sa_shade *shade);
sa_result sa_scene_part(sa_engine *engine, const sa_draw *parts,
                        uint32_t count);
/* the scene is whole: it stays until sa_scene_ended */
sa_result sa_scene_end(sa_engine *engine);
/* the act went: the scene that stays, which scene names, and its parts end */
sa_result sa_scene_ended(sa_engine *engine, uint32_t scene);

/* a picture's further views. a game may set its view more than once a frame,
 * as a camera that steps twice does. a task draws under the view of its
 * moment, so a picture's draws may lie under several views. every draw stands
 * in one world, the game's, and names its view. a backend that draws the
 * game's own picture draws each draw through the view it names. a backend that
 * makes a picture of its own looks through sa_picture.view alone. the game's
 * adapter chooses that view as its header says */
enum { SA_REC_VIEW = 27 };
/* the views a picture may be given, beside its own */
enum { SA_VIEWS = 8 };
typedef struct sa_picture_view {
    sa_record_head head;
    uint32_t view; /* what the draws name it by: 1 to SA_VIEWS */
    sa_view value;
} sa_picture_view;
/* add a view for the open picture's draws after it. a new picture starts with
 * its own alone. an id of 0, one over SA_VIEWS and one the picture has are
 * refused with SA_ERR_INVALID. so is a view given outside a picture, or with a
 * float that is no number. SA_ERR_HANDLE refuses a draw, a sprite or triangles
 * that names a view the picture never had. a backend may draw a whole picture
 * through one of its views, with sa_picture.view's projection. such a backend
 * refuses with SA_ERR_INVALID a draw placed by a view it cannot bring to that
 * one. a backend that draws in the call, as the Dreamcast's, is given none. it
 * draws under the view it finds */
sa_result sa_present_view(sa_engine *engine, const sa_picture_view *view);

/* a scene's lights, as the act's own data gave them at its load. they are
 * in the game's world, as the game holds them when it gives the scene. one
 * light shines one way over the whole act. each of the act's sources is a
 * point that lights what lies within its range. this is not the light a
 * picture is drawn under, which play changes. each picture's shades, with
 * what a game's header adds to a picture, carry that light. so a backend
 * that draws the game's own picture reads nothing here. one that lights the
 * scene itself, as one that follows a ray would, starts from it. nor is it
 * an sa_light, a light of Ninja's queue. an act's data holds no type, edge
 * or reach's near. it holds one color under three strengths, where an
 * sa_light has three */
enum { SA_REC_LIGHTING = 28 };
/* the sources a scene's lights may hold */
enum { SA_LIGHTING_SOURCES = 30 };
typedef struct sa_light_source {
    float point[3];
    float range; /* how far from its point it reaches */
    float color[3]; /* RGB */
    float rate; /* the game's own number for it, as its header says */
} sa_light_source;
typedef struct sa_lighting {
    sa_record_head head;
    float direction[3]; /* the way the one light shines, unit */
    float color[3]; /* RGB */
    /* how strongly it adds to a highlight, to a face by the dot, and to
     * every face */
    float specular, diffuse, ambient;
    uint32_t count; /* the sources that follow, the rest 0 */
    sa_light_source source[SA_LIGHTING_SOURCES];
} sa_lighting;
/* give the open scene its lights, copied during the call. a scene takes
 * them once, anywhere from sa_scene_begin to sa_scene_end, and a scene given
 * none has none. SA_ERR_INVALID refuses the call with no scene open, or for
 * a scene that has its lights. it refuses more sources than
 * SA_LIGHTING_SOURCES. it refuses a float that is no number in the light or
 * a counted source. the lights stay the scene's until sa_scene_ended. a
 * backend that keeps no scene holds the call to these rules, answers SA_OK
 * and keeps nothing. one that draws in the call, as the Dreamcast's, is
 * given none */
sa_result sa_scene_lighting(sa_engine *engine, const sa_lighting *lighting);

#ifdef __cplusplus
}
#endif

/* each record's size, the same on every target that compiles this */
#ifdef __cplusplus
#define SA_LAYOUT(type, bytes)                                                 \
    static_assert(sizeof(type) == (bytes), #type "'s layout")
#else
#define SA_LAYOUT(type, bytes)                                                 \
    _Static_assert(sizeof(type) == (bytes), #type "'s layout")
#endif
SA_LAYOUT(sa_ref, 12);
SA_LAYOUT(sa_record_head, 4);
SA_LAYOUT(sa_pad, 32);
SA_LAYOUT(sa_audio_command, 56);
SA_LAYOUT(sa_view, 72);
SA_LAYOUT(sa_picture, 96);
SA_LAYOUT(sa_fog, 268);
SA_LAYOUT(sa_material, 20);
SA_LAYOUT(sa_shade, 36);
SA_LAYOUT(sa_light, 84);
SA_LAYOUT(sa_lights, 684);
SA_LAYOUT(sa_draw, 92);
SA_LAYOUT(sa_payload, 8);
SA_LAYOUT(sa_sprite, 132);
SA_LAYOUT(sa_point, 24);
SA_LAYOUT(sa_triangles, 72);
SA_LAYOUT(sa_mesh, 16);
SA_LAYOUT(sa_meshset, 8);
SA_LAYOUT(sa_pixels, 8);
SA_LAYOUT(sa_event, 24);
SA_LAYOUT(sa_mode, 4);
SA_LAYOUT(sa_draw_kept, 16);
SA_LAYOUT(sa_scene, 8);
SA_LAYOUT(sa_picture_view, 80);
SA_LAYOUT(sa_light_source, 32);
SA_LAYOUT(sa_lighting, 1004);
#undef SA_LAYOUT

#endif
