/* the PC host's video: an SDL2 window with OpenGL 2.1 behind host_api's
 * video calls, and the keyboard as port A's pad.
 *
 * a vertex's position goes to clip space with w = 1 / rhw, so that GL
 * interpolates its colors and texture coordinates as the PowerVR does, in
 * perspective. a fragment's depth is 1 - log2(1 + 100000 rhw) / 34, which
 * falls as rhw grows, so that each of the PowerVR's compares on rhw is GL's
 * opposite compare on it. the log keeps rhw's small differences apart, as
 * Flycast's depth does: the menus draw their panels at rhw 1.724385e-05 and
 * the sky after them at 1.724197e-05, which the PowerVR's float compare keeps
 * behind, and which 1 / (1 + rhw) put on one value of a 24-bit depth buffer,
 * so the sky drew over the panels. rhw is gl_FragCoord.w, 1/w, which GL interpolates across
 * the screen linearly, as the PowerVR interpolates rhw: a depth from the
 * vertices' own, interpolated, would be wrong between them, since the
 * depth isn't linear in rhw. the fragment shader applies
 * the TSP's texture/shading instruction, the offset color, the fog and the
 * punch-through list's alpha test */
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../port/host.h"
#include "../../port/texhash.h"
#include "video.h"

#define WIDTH 640
#define HEIGHT 480
#define TEXTURES 2048

static SDL_Window *window;
static SDL_GLContext context;
static GLuint program;
static GLuint textures[TEXTURES];
static unsigned char texture_levels[TEXTURES]; /* a texture's mipmap levels, 1 for none */
static GLint u_textured, u_shading, u_use_alpha, u_ignore_alpha, u_use_offset, u_punch, u_ref;
static GLint u_fog, u_fog_table_color, u_fog_vertex_color, u_fog_density;
static GLuint fog_texture;
static int scale = 1;
static void open_pad(void);
static unsigned keys;
static int quit;

static const char *vertex_source =
    "#version 120\n"
    "attribute vec3 a_pos;\n"
    "attribute vec2 a_uv;\n"
    "attribute vec4 a_base;\n"
    "attribute vec4 a_offset;\n"
    "varying vec2 v_uv;\n"
    "varying vec4 v_base;\n"
    "varying vec4 v_offset;\n"
    "uniform vec4 u_row_x, u_row_y, u_row_z;\n"
    "uniform int u_model;\n"
    "void main() {\n"
    "    vec2 at = a_pos.xy;\n"
    "    float rhw = a_pos.z;\n"
    "    if (u_model != 0) {\n"
    "        /* a model's point (video_vertices3): x and y over the depth, rhw 1 over it */\n"
    "        vec4 p = vec4(a_pos, 1.0);\n"
    "        rhw = 1.0 / dot(u_row_z, p);\n"
    "        at = vec2(dot(u_row_x, p), dot(u_row_y, p)) * rhw;\n"
    "    }\n"
    "    rhw = max(rhw, 1e-7);\n"
    "    float w = 1.0 / rhw;\n"
    "    float d = 1.0 / (1.0 + rhw);\n"
    "    gl_Position = vec4((at.x / 320.0 - 1.0) * w, (1.0 - at.y / 240.0) * w, (2.0 * d - 1.0) * w, w);\n"
    "    v_uv = a_uv;\n"
    "    v_base = a_base.bgra;\n"
    "    v_offset = a_offset.bgra;\n"
    "}\n";

static const char *fragment_source =
    "#version 120\n"
    "uniform sampler2D tex;\n"
    "uniform int u_textured, u_shading, u_use_alpha, u_ignore_alpha, u_use_offset, u_punch;\n"
    "uniform float u_ref;\n"
    "uniform int u_fog;\n"
    "uniform sampler2D fog_table;\n"
    "uniform vec3 u_fog_table_color, u_fog_vertex_color;\n"
    "uniform float u_fog_density;\n"
    "varying vec2 v_uv;\n"
    "varying vec4 v_base;\n"
    "varying vec4 v_offset;\n"
    /* the PowerVR's table fog: the density times rhw, from 1 to 256, on a
     * log scale of 16 entries to a doubling, 128 in all, each two bytes
     * that the fraction blends between */
    "float fog_table_value(float rhw) {\n"
    "    float z = clamp(rhw * u_fog_density, 1.0, 255.9999);\n"
    "    float e = floor(log2(z));\n"
    "    float m = z * 16.0 / exp2(e) - 16.0;\n"
    "    float i = floor(m) + e * 16.0;\n"
    "    float hi = texture2D(fog_table, vec2((2.0 * i + 0.5) / 256.0, 0.5)).r;\n"
    "    float lo = texture2D(fog_table, vec2((2.0 * i + 1.5) / 256.0, 0.5)).r;\n"
    "    return mix(hi, lo, m - floor(m));\n"
    "}\n"
    "void main() {\n"
    "    vec4 base = v_base;\n"
    "    vec4 c;\n"
    "    if (u_use_alpha == 0) base.a = 1.0;\n"
    "    if (u_textured == 1) {\n"
    "        vec4 t = texture2D(tex, v_uv);\n"
    "        if (u_ignore_alpha == 1) t.a = 1.0;\n"
    "        if (u_shading == 0) c = t;\n"
    "        else if (u_shading == 1) c = vec4(t.rgb * base.rgb, t.a);\n"
    "        else if (u_shading == 2) c = vec4(mix(base.rgb, t.rgb, t.a), base.a);\n"
    "        else c = t * base;\n"
    "        if (u_use_offset == 1) c.rgb += v_offset.rgb;\n"
    "    } else {\n"
    "        c = base;\n"
    "    }\n"
    "    if (u_fog == 0) c.rgb = mix(c.rgb, u_fog_table_color, fog_table_value(gl_FragCoord.w));\n"
    "    else if (u_fog == 1) c.rgb = mix(c.rgb, u_fog_vertex_color, v_offset.a);\n"
    "    else if (u_fog == 3) c = vec4(u_fog_table_color, fog_table_value(gl_FragCoord.w));\n"
    "    if (u_punch == 1 && c.a < u_ref) discard;\n"
    "    gl_FragColor = clamp(c, 0.0, 1.0);\n"
    "    gl_FragDepth = clamp(1.0 - log2(1.0 + 100000.0 * gl_FragCoord.w) / 34.0, 0.0, 1.0);\n"
    "}\n";

static GLuint compile(GLenum kind, const char *source)
{
    GLuint shader = glCreateShader(kind);
    GLint ok;

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];

        glGetShaderInfoLog(shader, sizeof log, NULL, log);
        fprintf(stderr, "host: shader: %s\n", log);
    }
    return shader;
}

int video_open(void)
{
    GLint ok;

    if (getenv("HOST_SCALE") != NULL) {
        scale = atoi(getenv("HOST_SCALE"));
        scale = scale < 1 ? 1 : scale > 4 ? 4 : scale;
    }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0 && SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "host: SDL: %s\n", SDL_GetError());
        return -1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    window = SDL_CreateWindow("Sonic Adventure", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH * scale,
                              HEIGHT * scale, SDL_WINDOW_OPENGL);
    if (window == NULL || (context = SDL_GL_CreateContext(window)) == NULL) {
        fprintf(stderr, "host: SDL: %s\n", SDL_GetError());
        return -1;
    }
    SDL_GL_SetSwapInterval(0);
    program = glCreateProgram();
    glAttachShader(program, compile(GL_VERTEX_SHADER, vertex_source));
    glAttachShader(program, compile(GL_FRAGMENT_SHADER, fragment_source));
    glBindAttribLocation(program, 0, "a_pos");
    glBindAttribLocation(program, 1, "a_uv");
    glBindAttribLocation(program, 2, "a_base");
    glBindAttribLocation(program, 3, "a_offset");
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        fprintf(stderr, "host: the shaders don't link\n");
        return -1;
    }
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "tex"), 0);
    u_textured = glGetUniformLocation(program, "u_textured");
    u_shading = glGetUniformLocation(program, "u_shading");
    u_use_alpha = glGetUniformLocation(program, "u_use_alpha");
    u_ignore_alpha = glGetUniformLocation(program, "u_ignore_alpha");
    u_use_offset = glGetUniformLocation(program, "u_use_offset");
    u_punch = glGetUniformLocation(program, "u_punch");
    u_ref = glGetUniformLocation(program, "u_ref");
    u_fog = glGetUniformLocation(program, "u_fog");
    u_fog_table_color = glGetUniformLocation(program, "u_fog_table_color");
    u_fog_vertex_color = glGetUniformLocation(program, "u_fog_vertex_color");
    u_fog_density = glGetUniformLocation(program, "u_fog_density");
    glUniform1i(glGetUniformLocation(program, "fog_table"), 1);
    glGenTextures(1, &fog_texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, fog_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(TEXTURES, textures);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glViewport(0, 0, WIDTH * scale, HEIGHT * scale);
    open_pad();
    fprintf(stderr, "host: video driver %s\n", SDL_GetCurrentVideoDriver());
    fprintf(stderr, "host: video: %s, %s\n", (const char *)glGetString(GL_RENDERER),
            (const char *)glGetString(GL_VERSION));
    return 0;
}

void video_begin(unsigned clear)
{
    glDepthMask(GL_TRUE);
    glClearColor(((clear >> 16) & 255) / 255.0F, ((clear >> 8) & 255) / 255.0F, (clear & 255) / 255.0F, 1.0F);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void video_texture(unsigned id, int width, int height, const unsigned *argb)
{
    if (id == 0 || id >= TEXTURES) {
        return;
    }
    glBindTexture(GL_TEXTURE_2D, textures[id]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, argb);
    texture_levels[id] = 1;
}

/* the Xbox's pack of textures converted ahead (xbox/host/xbox/xboxtex.c),
 * where HOST_PACK names it: the PC then draws with the textures the Xbox
 * draws with, their mipmaps too, for the two to be compared. without
 * HOST_PACK the renderer decodes every texture, as the reference */
static struct texpack_entry *pack_entries;
static unsigned pack_count;
static FILE *pack_file;

static const struct texpack_entry *pack_find(unsigned key_lo, unsigned key_hi)
{
    static int opened;
    unsigned low = 0, high;

    if (!opened) {
        struct texpack_header header;
        const char *path = getenv("HOST_PACK");

        opened = 1;
        if (path != NULL && (pack_file = fopen(path, "rb")) != NULL) {
            if (fread(&header, sizeof header, 1, pack_file) == 1 && header.magic == TEXPACK_MAGIC &&
                (pack_entries = malloc(header.count * sizeof *pack_entries)) != NULL &&
                fread(pack_entries, sizeof *pack_entries, header.count, pack_file) == header.count) {
                pack_count = header.count;
            }
            fprintf(stderr, "host: video: %u textures in %s\n", pack_count, path);
        }
    }
    high = pack_count;
    while (low < high) {
        unsigned mid = (low + high) / 2;
        const struct texpack_entry *m = &pack_entries[mid];

        if (m->hi == key_hi && m->lo == key_lo) {
            return m;
        }
        if (m->hi < key_hi || (m->hi == key_hi && m->lo < key_lo)) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    return NULL;
}

int video_texture_known(unsigned key_lo, unsigned key_hi)
{
    return pack_find(key_lo, key_hi) != NULL;
}

/* with have_video 0, for a headless run, only whether the pack has it */
int video_texture_packed(unsigned id, unsigned key_lo, unsigned key_hi, int have_video)
{
    const struct texpack_entry *e = pack_find(key_lo, key_hi);
    unsigned char *blob;
    unsigned at = 0, level;
    unsigned w, h;

    if (e == NULL || id == 0 || id >= TEXTURES) {
        return 0;
    }
    if (!have_video) {
        return 1;
    }
    blob = malloc(e->bytes);
    if (blob == NULL || fseek(pack_file, (long)e->offset, SEEK_SET) != 0 || fread(blob, 1, e->bytes, pack_file) != e->bytes) {
        free(blob);
        return 0;
    }
    w = 1u << e->log_w;
    h = 1u << e->log_h;
    glBindTexture(GL_TEXTURE_2D, textures[id]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, e->levels - 1);
    for (level = 0; level < e->levels; level++) {
        if (e->format == TEXPACK_DXT1 || e->format == TEXPACK_DXT3) {
            unsigned size = ((w + 3) / 4) * ((h + 3) / 4) * (e->format == TEXPACK_DXT1 ? 8 : 16);

            glCompressedTexImage2D(GL_TEXTURE_2D, (GLint)level,
                                   e->format == TEXPACK_DXT1 ? GL_COMPRESSED_RGB_S3TC_DXT1_EXT
                                                             : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
                                   (GLsizei)w, (GLsizei)h, 0, (GLsizei)size, blob + at);
            at += size;
        } else {
            /* swizzled 16 bits: x's bits lowest, the longer side's rest after */
            unsigned *argb = malloc(w * h * 4);
            unsigned x, y;

            for (y = 0; y < h; y++) {
                for (x = 0; x < w; x++) {
                    unsigned i = 0, bit = 0, k;
                    unsigned t, a, r, g, b;

                    for (k = 0; (1u << k) < w || (1u << k) < h; k++) {
                        if ((1u << k) < w) {
                            i |= ((x >> k) & 1) << bit++;
                        }
                        if ((1u << k) < h) {
                            i |= ((y >> k) & 1) << bit++;
                        }
                    }
                    t = blob[at + 2 * i] | blob[at + 2 * i + 1] << 8;
                    if (e->format == TEXPACK_R5G6B5) {
                        a = 255, r = (t >> 11) * 255 / 31, g = ((t >> 5) & 63) * 255 / 63, b = (t & 31) * 255 / 31;
                    } else if (e->format == TEXPACK_A1R5G5B5) {
                        a = (t & 0x8000) ? 255 : 0;
                        r = ((t >> 10) & 31) * 255 / 31, g = ((t >> 5) & 31) * 255 / 31, b = (t & 31) * 255 / 31;
                    } else {
                        a = (t >> 12) * 17, r = ((t >> 8) & 15) * 17, g = ((t >> 4) & 15) * 17, b = (t & 15) * 17;
                    }
                    argb[y * w + x] = a << 24 | r << 16 | g << 8 | b;
                }
            }
            glTexImage2D(GL_TEXTURE_2D, (GLint)level, GL_RGBA, (GLsizei)w, (GLsizei)h, 0, GL_BGRA, GL_UNSIGNED_BYTE, argb);
            free(argb);
            at += w * h * 2;
        }
        w = w > 1 ? w / 2 : 1;
        h = h > 1 ? h / 2 : 1;
    }
    free(blob);
    texture_levels[id] = e->levels;
    return 1;
}

static GLenum blend(int code, int is_src)
{
    static const GLenum factors[8] = {GL_ZERO, GL_ONE, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR,
                                      GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA};

    if (!is_src && code == 2) {
        return GL_SRC_COLOR;
    }
    if (!is_src && code == 3) {
        return GL_ONE_MINUS_SRC_COLOR;
    }
    return factors[code & 7];
}

/* the PowerVR's compare on rhw, as GL's on the depth that falls as rhw
 * grows */
static const GLenum depth_funcs[8] = {GL_NEVER, GL_GREATER, GL_EQUAL, GL_GEQUAL,
                                      GL_LESS, GL_NOTEQUAL, GL_LEQUAL, GL_ALWAYS};

/* scissor to the console's pixels x0, y0 to x1, y1, y down */
static void scissor(int x0, int y0, int x1, int y1)
{
    x1 = x1 > WIDTH ? WIDTH : x1;
    y1 = y1 > HEIGHT ? HEIGHT : y1;
    if (x1 < x0) {
        x1 = x0;
    }
    if (y1 < y0) {
        y1 = y0;
    }
    glEnable(GL_SCISSOR_TEST);
    glScissor(x0 * scale, (HEIGHT - y1) * scale, (x1 - x0) * scale, (y1 - y0) * scale);
}

/* the vertices the draws name by index: the screen's, and a model's with
 * its transform */
static const struct host_vertex *draw_vertices;
static const struct host_vertex *model_vertices;
static float model_transform[12];

void video_vertices(const struct host_vertex *vertices, int count)
{
    (void)count;
    draw_vertices = vertices;
}

void video_vertices3(const struct host_vertex *vertices, int count, const float transform[12], float rhw_near,
                     float rhw_mid)
{
    (void)count;
    (void)rhw_near;
    (void)rhw_mid;
    model_vertices = vertices;
    memcpy(model_transform, transform, sizeof model_transform);
}

/* the models' vertices of the frame the port is making: GL draws from the
 * port's memory as it's called, so one room serves every frame */
struct host_vertex *video_arena(int *vertices)
{
    static struct host_vertex room[0x8000];
    /* HOST_ARENA=N: an arena of N vertices, for a test of the drawer past
     * its room (model.c's gpu_fits), as the Xbox's 20,000 fills */
    const char *size = getenv("HOST_ARENA");

    *vertices = 0x8000;
    if (size != NULL && atoi(size) > 0 && atoi(size) < 0x8000) {
        *vertices = atoi(size);
    }
    return room;
}

/* the kept models' vertices, which hold from frame to frame: GL draws from
 * the port's memory, so any room does */
struct host_vertex *video_store(int *vertices)
{
    static struct host_vertex room[65520];

    *vertices = 65520;
    return room;
}

static void draw_with(const struct host_draw *draw, const struct host_vertex *vertices, const unsigned short *indices,
                      int count, int model);

void video_draw(const struct host_draw *draw, const unsigned short *indices, int count)
{
    draw_with(draw, draw_vertices, indices, count, 0);
}

void video_draw3(const struct host_draw *draw, const unsigned short *indices, int count)
{
    draw_with(draw, model_vertices, indices, count, 1);
}

/* a CMP field's value: bits signed, over largest, at least -1 */
static float cmp_field(unsigned word, int shift, int bits, int largest)
{
    int v = (int)((word >> shift) & ((1u << bits) - 1));
    float f;

    if (v & (1 << (bits - 1))) {
        v -= 1 << bits;
    }
    f = (float)v / (float)largest;
    return f < -1.0F ? -1.0F : f;
}

/* a lit item's vertices, colored from its palette as host.h's host_item
 * says, per vertex as the game's drawer shades them */
static const struct host_vertex *lit_vertices(const struct host_item *item)
{
    static struct host_vertex room[0x10000];
    int k;

    for (k = 0; k < item->vertex_count && k < 0x10000; k++) {
        const struct host_vertex *v = &item->vertices[k];
        unsigned n = v->offset;
        float d = cmp_field(n, 0, 11, 1023) * item->light[0] + cmp_field(n, 11, 11, 1023) * item->light[1] +
                  cmp_field(n, 22, 10, 511) * item->light[2];
        int entry = (int)(d * 127.5F + 127.5F);

        entry = entry < 0 ? 0 : entry > 255 ? 255 : entry;
        room[k] = *v;
        room[k].base = (v->base & 0xff000000u) | (item->palette[entry] & 0x00ffffffu);
        room[k].offset = item->palette[256 + entry];
    }
    return room;
}

void video_draw_items(const struct host_item *items, int count)
{
    int k;

    for (k = 0; k < count; k++) {
        const struct host_item *item = &items[k];

        if (item->vertices != NULL) {
            video_vertices3(item->palette != NULL ? lit_vertices(item) : item->vertices, item->vertex_count,
                            item->transform, item->rhw_near, item->rhw_mid);
            video_draw3(&item->draw, item->indices, item->count);
        } else {
            video_draw(&item->draw, item->indices, item->count);
        }
    }
}

static void draw_with(const struct host_draw *draw, const struct host_vertex *vertices, const unsigned short *indices,
                      int count, int model)
{
    const char *base = (const char *)vertices;
    GLsizei stride = sizeof *vertices;

    glUniform1i(glGetUniformLocation(program, "u_model"), model);
    if (model) {
        glUniform4fv(glGetUniformLocation(program, "u_row_x"), 1, model_transform);
        glUniform4fv(glGetUniformLocation(program, "u_row_y"), 1, model_transform + 4);
        glUniform4fv(glGetUniformLocation(program, "u_row_z"), 1, model_transform + 8);
    }
    /* a model's triangles culled as the renderer culls the screen's: 2
     * drops the counterclockwise as the screen shows them, which are GL's
     * counterclockwise too, its front faces here; HOST_CULL_FLIP=1 swaps
     * the two */
    if (model && (draw->cull == 2 || draw->cull == 3)) {
        static int flip = -1;

        if (flip < 0) {
            flip = getenv("HOST_CULL_FLIP") != NULL;
        }
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glCullFace((draw->cull == 2) != flip ? GL_FRONT : GL_BACK);
    } else {
        glDisable(GL_CULL_FACE);
    }

    if (draw->src_blend == 1 && draw->dst_blend == 0) {
        glDisable(GL_BLEND);
    } else {
        glEnable(GL_BLEND);
        glBlendFunc(blend(draw->src_blend, 1), blend(draw->dst_blend, 0));
    }
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(depth_funcs[draw->depth_compare & 7]);
    glDepthMask(draw->depth_write ? GL_TRUE : GL_FALSE);
    glUniform1i(u_textured, draw->texture != 0);
    if (draw->texture != 0 && draw->texture < TEXTURES) {
        GLint filter = draw->filter ? GL_LINEAR : GL_NEAREST;
        /* with mipmaps, the nearest level's texel or both levels' blend, as
         * the Xbox host samples them */
        GLint min_filter = texture_levels[draw->texture] > 1
                               ? (draw->filter ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST)
                               : filter;

        glBindTexture(GL_TEXTURE_2D, textures[draw->texture]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                        draw->clamp_u ? GL_CLAMP_TO_EDGE : draw->flip_u ? GL_MIRRORED_REPEAT : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                        draw->clamp_v ? GL_CLAMP_TO_EDGE : draw->flip_v ? GL_MIRRORED_REPEAT : GL_REPEAT);
    }
    glUniform1i(u_shading, draw->shading);
    glUniform1i(u_use_alpha, draw->use_alpha);
    glUniform1i(u_ignore_alpha, draw->ignore_texture_alpha);
    glUniform1i(u_use_offset, draw->use_offset);
    glUniform1i(u_punch, draw->punch_through);
    glUniform1f(u_ref, draw->alpha_ref);
    glUniform1i(u_fog, draw->fog);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, base);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, base + 12);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, base + 20);
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, base + 24);
    if (draw->clip_mode == 2) {
        scissor(draw->clip_x0, draw->clip_y0, draw->clip_x1, draw->clip_y1);
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
        glDisable(GL_SCISSOR_TEST);
    } else if (draw->clip_mode == 3) {
        /* outside the rectangle: the bands above, below, left and right */
        int x0 = draw->clip_x0, y0 = draw->clip_y0, x1 = draw->clip_x1, y1 = draw->clip_y1;

        scissor(0, 0, WIDTH, y0);
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
        scissor(0, y1, WIDTH, HEIGHT);
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
        scissor(0, y0, x0, y1);
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
        scissor(x1, y0, WIDTH, y1);
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
        glDisable(GL_SCISSOR_TEST);
    } else {
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
    }
}

void video_fog(unsigned table_color, unsigned vertex_color, float density, const unsigned short table[128])
{
    unsigned char bytes[256];
    int k;

    /* each entry's high byte first, as the shader reads them */
    for (k = 0; k < 128; k++) {
        bytes[2 * k] = (unsigned char)(table[k] >> 8);
        bytes[2 * k + 1] = (unsigned char)table[k];
    }
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, fog_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 256, 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, bytes);
    glActiveTexture(GL_TEXTURE0);
    glUniform3f(u_fog_table_color, ((table_color >> 16) & 255) / 255.0F, ((table_color >> 8) & 255) / 255.0F,
                (table_color & 255) / 255.0F);
    glUniform3f(u_fog_vertex_color, ((vertex_color >> 16) & 255) / 255.0F, ((vertex_color >> 8) & 255) / 255.0F,
                (vertex_color & 255) / 255.0F);
    glUniform1f(u_fog_density, density);
}

/* the keyboard as port A: the arrows the analog stick, by which the
 * characters move and the menus' cursors too, I, K, J and L the
 * directional pad, Z A, X B, A X, S Y, Q and W the triggers, Enter Start */
static int stick_left, stick_right, stick_up, stick_down, trigger_l, trigger_r;
/* a game controller, the first SDL finds, as port A beside the keyboard */
static SDL_GameController *pad;

static void open_pad(void)
{
    int k;

    if (pad != NULL) {
        return;
    }
    for (k = 0; k < SDL_NumJoysticks(); k++) {
        if (SDL_IsGameController(k) && (pad = SDL_GameControllerOpen(k)) != NULL) {
            fprintf(stderr, "host: controller: %s\n", SDL_GameControllerName(pad));
            return;
        }
    }
}
static void poll(void)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            quit = 1;
        } else if (event.type == SDL_CONTROLLERDEVICEADDED) {
            open_pad();
        } else if (event.type == SDL_CONTROLLERDEVICEREMOVED && pad != NULL &&
                   !SDL_GameControllerGetAttached(pad)) {
            SDL_GameControllerClose(pad);
            pad = NULL;
        } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            unsigned bit = 0;

            int down = event.type == SDL_KEYDOWN;

            if (down && !event.key.repeat && getenv("HOST_KEYLOG") != NULL) {
                fprintf(stderr, "host: key %s\n", SDL_GetKeyName(event.key.keysym.sym));
            }

            switch (event.key.keysym.sym) {
            case SDLK_UP:
                stick_up = down;
                break;
            case SDLK_DOWN:
                stick_down = down;
                break;
            case SDLK_LEFT:
                stick_left = down;
                break;
            case SDLK_RIGHT:
                stick_right = down;
                break;
            case SDLK_q:
                trigger_l = down;
                break;
            case SDLK_w:
                trigger_r = down;
                break;
            case SDLK_i:
                bit = 1u << 4;
                break;
            case SDLK_k:
                bit = 1u << 5;
                break;
            case SDLK_j:
                bit = 1u << 6;
                break;
            case SDLK_l:
                bit = 1u << 7;
                break;
            case SDLK_z:
                bit = 1u << 2;
                break;
            case SDLK_x:
                bit = 1u << 1;
                break;
            case SDLK_a:
                bit = 1u << 10;
                break;
            case SDLK_s:
                bit = 1u << 9;
                break;
            case SDLK_RETURN:
                bit = 1u << 3;
                break;
            case SDLK_ESCAPE:
                quit = event.type == SDL_KEYDOWN;
                break;
            default:
                break;
            }
            keys = event.type == SDL_KEYDOWN ? keys | bit : keys & ~bit;
        }
    }
}

/* the microseconds the presents have taken, the swap's wait with them, for
 * host_api's stats */
unsigned video_present_us;

void video_present(void)
{
    Uint64 before = SDL_GetPerformanceCounter();

    SDL_GL_SwapWindow(window);
    video_present_us += (unsigned)((SDL_GetPerformanceCounter() - before) * 1000000 / SDL_GetPerformanceFrequency());
    poll();
}

unsigned video_keys(void)
{
    static const struct {
        SDL_GameControllerButton button;
        unsigned bit;
    } map[] = {
        {SDL_CONTROLLER_BUTTON_A, 1u << 2},          {SDL_CONTROLLER_BUTTON_B, 1u << 1},
        {SDL_CONTROLLER_BUTTON_X, 1u << 10},         {SDL_CONTROLLER_BUTTON_Y, 1u << 9},
        {SDL_CONTROLLER_BUTTON_START, 1u << 3},      {SDL_CONTROLLER_BUTTON_DPAD_UP, 1u << 4},
        {SDL_CONTROLLER_BUTTON_DPAD_DOWN, 1u << 5},  {SDL_CONTROLLER_BUTTON_DPAD_LEFT, 1u << 6},
        {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, 1u << 7},
    };
    unsigned buttons = keys;
    unsigned k;

    if (pad != NULL) {
        for (k = 0; k < sizeof map / sizeof map[0]; k++) {
            if (SDL_GameControllerGetButton(pad, map[k].button)) {
                buttons |= map[k].bit;
            }
        }
    }
    return buttons;
}

void video_stick(int *x, int *y, int *l, int *r)
{
    if (pad != NULL) {
        /* SDL's -32768 to 32767 as the console's 0 to 255, 128 at rest,
         * past a dead zone (host_stick; HOST_DEADZONE=PERCENT of the throw,
         * 0 for none), and its triggers' 0 to 32767 as 0 to 255 */
        int percent = getenv("HOST_DEADZONE") != NULL ? atoi(getenv("HOST_DEADZONE")) : HOST_STICK_DEAD;

        host_stick(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX),
                   SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY),
                   (percent < 0 ? 0 : percent > 90 ? 90 : percent) * 32767 / 100, x, y);
        *l = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 128;
        *r = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 128;
    }
    if (stick_left != stick_right) {
        *x = stick_left ? 0 : 255;
    }
    if (stick_up != stick_down) {
        *y = stick_up ? 0 : 255;
    }
    if (trigger_l) {
        *l = 255;
    }
    if (trigger_r) {
        *r = 255;
    }
}

int video_quit(void)
{
    return quit;
}

/* the frame drawn, before it shows, as a binary PPM */
int video_shot(const char *path)
{
    static unsigned char pixels[WIDTH * 4 * HEIGHT * 4 * 3];
    int width = WIDTH * scale;
    int height = HEIGHT * scale;
    FILE *file = fopen(path, "wb");
    int y;

    if (file == NULL) {
        return -1;
    }
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    fprintf(file, "P6\n%d %d\n255\n", width, height);
    for (y = height - 1; y >= 0; y--) {
        fwrite(pixels + (size_t)y * width * 3, 3, (size_t)width, file);
    }
    fclose(file);
    return 0;
}
