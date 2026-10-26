/* 2D lines, each drawn as a strip of two triangles around it */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>

/* a 32-byte vertex as the tile accelerator takes it */
struct line_vertex {
    Uint32 control;
    Float x, y, z;
    Float u, v;
    Uint32 color;
    Uint32 offset;
};

/* half the line width, in pixels */
#define LINE_HALF_WIDTH 0.525F
/* NJS_COLOR's tex.u and tex.v count 256 to the texture's side */
#define UV_SCALE (1.0F / 256.0F)

static void start_plain(Float pri, Int trans);
static void start_textured(Float pri, Int trans);
static void lines_plain(NJS_POINT2COL *p, Int n);
static void fan_plain(NJS_POINT2COL *p, Int n);
static void strip_plain(NJS_POINT2COL *p, Int n);
static void lines_textured(NJS_POINT2COL *p, Int n);
static void fan_textured(NJS_POINT2COL *p, Int n);
static void strip_textured(NJS_POINT2COL *p, Int n);

/* the line functions, by texture: plain, then textured. line_start holds one
 * for each, and line_draw three for each, by how the points join:
 * NJD_DRAW_NORMAL's pairs, NJD_DRAW_FAN's fan from the first point, and
 * NJD_DRAW_CONNECTED's strip, each point joined to the one before.
 * plain_modes and textured_modes hold the color modes, opaque then
 * translucent */
static void (*line_start[])(Float pri, Int trans) = {start_plain,
                                                     start_textured};
static void (*line_draw[])(NJS_POINT2COL *p, Int n) = {
    lines_plain,    fan_plain,    strip_plain,
    lines_textured, fan_textured, strip_textured};
static Uint32 plain_modes[] = {COLOR_MODE_OPAQUE, COLOR_MODE_TRANS};
static Uint32 textured_modes[] = {COLOR_MODE_OPAQUE | COLOR_MODE_TEXTURE,
                                  COLOR_MODE_TRANS | COLOR_MODE_TEXTURE};
static Float line_z;
static Uint32 *line_out;
/* a line's two ends. the image reserves three vertices for each array, and
 * nothing reads the third */
static struct line_vertex plain_ends[3];
static struct line_vertex textured_ends[3];

/* step line_out back a word and store value there */
#define PUT(type, value) (*(type *)--line_out = (value))

static void start_plain(Float pri, Int trans)
{
    njSetkmColorMode(plain_modes[trans]);
    if (trans) {
        line_out = njStartVertexNonTexTrans();
    } else {
        line_out = njStartVertexNonTexOpaque();
    }
    line_z = pri < -1.0F ? -1.0F / pri : 1.0F;
}

static void start_textured(Float pri, Int trans)
{
    njSetkmColorMode(textured_modes[trans]);
    if (trans) {
        line_out = njStartVertexTexTrans();
    } else {
        line_out = njStartVertexTexOpaque();
    }
    line_z = pri < -1.0F ? -1.0F / pri : 1.0F;
}

/* send the line from a to b as a strip of four vertices, LINE_HALF_WIDTH to
 * either side of each end. each vertex is written from its color back to its
 * control word, then sent by the prefetch of its first word. u and v go
 * unwritten */
static void send_plain(struct line_vertex *a, struct line_vertex *b)
{
    Float dx;
    Float dy;
    Float scale;
    Float ox;
    Float oy;
    /* @hack redundant: spare, never used, gives the function its 32-byte
     * frame (ADD #-32,R15 at 0x8c616c42), which nothing reads or writes;
     * without it, njDrawLine2D.c's code runs 4 bytes short and 143 of 175
     * words differ */
    struct line_vertex spare;

    dx = b->x - a->x;
    dy = b->y - a->y;
    scale = 1.0F / _builtin_sqrtf((dx * dx) + (dy * dy));
    ox = dy * scale * LINE_HALF_WIDTH;
    oy = -(dx * scale * LINE_HALF_WIDTH);
    line_out += 7;
    PUT(Uint32, a->color);
    line_out -= 2;
    PUT(Float, line_z);
    PUT(Float, a->y + oy);
    PUT(Float, a->x + ox);
    PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(line_out);
    line_out += 8;
    line_out += 7;
    PUT(Uint32, a->color);
    line_out -= 2;
    PUT(Float, line_z);
    PUT(Float, a->y - oy);
    PUT(Float, a->x - ox);
    PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(line_out);
    line_out += 8;
    line_out += 7;
    PUT(Uint32, b->color);
    line_out -= 2;
    PUT(Float, line_z);
    PUT(Float, b->y + oy);
    PUT(Float, b->x + ox);
    PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(line_out);
    line_out += 8;
    line_out += 7;
    PUT(Uint32, b->color);
    line_out -= 2;
    PUT(Float, line_z);
    PUT(Float, b->y - oy);
    PUT(Float, b->x - ox);
    PUT(Uint32, KM_VERTEXPARAM_ENDOFSTRIP);
    prefetch(line_out);
    line_out += 8;
}

static void send_textured(struct line_vertex *a, struct line_vertex *b)
{
    Float dx;
    Float dy;
    Float scale;
    Float ox;
    Float oy;
    /* @hack redundant: spare, never used, gives the function its 32-byte
     * frame (ADD #-32,R15 at 0x8c616dd6), which nothing reads or writes;
     * without it, njDrawLine2D.c's code runs 4 bytes short and 180 of 215
     * words differ */
    struct line_vertex spare;

    dx = b->x - a->x;
    dy = b->y - a->y;
    scale = 1.0F / _builtin_sqrtf((dx * dx) + (dy * dy));
    ox = dy * scale * LINE_HALF_WIDTH;
    oy = -(dx * scale * LINE_HALF_WIDTH);
    line_out += 7;
    PUT(Uint32, a->color);
    PUT(Float, a->v);
    PUT(Float, a->u);
    PUT(Float, line_z);
    PUT(Float, a->y + oy);
    PUT(Float, a->x + ox);
    PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(line_out);
    line_out += 8;
    line_out += 7;
    PUT(Uint32, a->color);
    PUT(Float, a->v);
    PUT(Float, a->u);
    PUT(Float, line_z);
    PUT(Float, a->y - oy);
    PUT(Float, a->x - ox);
    PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(line_out);
    line_out += 8;
    line_out += 7;
    PUT(Uint32, b->color);
    PUT(Float, b->v);
    PUT(Float, b->u);
    PUT(Float, line_z);
    PUT(Float, b->y + oy);
    PUT(Float, b->x + ox);
    PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(line_out);
    line_out += 8;
    line_out += 7;
    PUT(Uint32, b->color);
    PUT(Float, b->v);
    PUT(Float, b->u);
    PUT(Float, line_z);
    PUT(Float, b->y - oy);
    PUT(Float, b->x - ox);
    PUT(Uint32, KM_VERTEXPARAM_ENDOFSTRIP);
    prefetch(line_out);
    line_out += 8;
}

/* draw n lines, each between two points of its own */
static void lines_textured(NJS_POINT2COL *p, Int n)
{
    Int i;
    Int point = 0;

    for (i = 0; i < n; i++) {
        textured_ends[0].x = p->p[point].x;
        textured_ends[0].y = p->p[point].y;
        textured_ends[0].u = p->tex[point].tex.u * UV_SCALE;
        textured_ends[0].v = p->tex[point].tex.v * UV_SCALE;
        textured_ends[0].color = p->col[point].color;
        textured_ends[0].control = KM_VERTEXPARAM_NORMAL;
        point++;
        textured_ends[1].x = p->p[point].x;
        textured_ends[1].y = p->p[point].y;
        textured_ends[1].u = p->tex[point].tex.u * UV_SCALE;
        textured_ends[1].v = p->tex[point].tex.v * UV_SCALE;
        textured_ends[1].color = p->col[point].color;
        textured_ends[1].control = KM_VERTEXPARAM_NORMAL;
        point++;
        send_textured(&textured_ends[0], &textured_ends[1]);
    }
}

static void lines_plain(NJS_POINT2COL *p, Int n)
{
    Int i;
    Int point = 0;

    for (i = 0; i < n; i++) {
        plain_ends[0].x = p->p[point].x;
        plain_ends[0].y = p->p[point].y;
        plain_ends[0].color = p->col[point].color;
        plain_ends[0].control = KM_VERTEXPARAM_NORMAL;
        point++;
        plain_ends[1].x = p->p[point].x;
        plain_ends[1].y = p->p[point].y;
        plain_ends[1].color = p->col[point].color;
        plain_ends[1].control = KM_VERTEXPARAM_NORMAL;
        point++;
        send_plain(&plain_ends[0], &plain_ends[1]);
    }
}

/* draw a line from each of n points to the one before. each new point replaces
 * the older end */
static void strip_textured(NJS_POINT2COL *p, Int n)
{
    Int i;
    Int end;

    textured_ends[0].x = p->p[0].x;
    textured_ends[0].y = p->p[0].y;
    textured_ends[0].u = p->tex[0].tex.u * UV_SCALE;
    textured_ends[0].v = p->tex[0].tex.v * UV_SCALE;
    textured_ends[0].color = p->col[0].color;
    textured_ends[0].control = KM_VERTEXPARAM_NORMAL;
    for (i = 1; i < n; i++) {
        end = i % 2;
        textured_ends[end].x = p->p[i].x;
        textured_ends[end].y = p->p[i].y;
        textured_ends[end].u = p->tex[i].tex.u * UV_SCALE;
        textured_ends[end].v = p->tex[i].tex.v * UV_SCALE;
        textured_ends[end].color = p->col[i].color;
        /* this stores KM_VERTEXPARAM_ENDOFSTRIP where strip_plain stores
         * KM_VERTEXPARAM_NORMAL, but send_textured never reads control */
        textured_ends[end].control = KM_VERTEXPARAM_ENDOFSTRIP;
        send_textured(&textured_ends[0], &textured_ends[1]);
    }
}

static void strip_plain(NJS_POINT2COL *p, Int n)
{
    Int i;
    Int end;

    plain_ends[0].x = p->p[0].x;
    plain_ends[0].y = p->p[0].y;
    plain_ends[0].color = p->col[0].color;
    plain_ends[0].control = KM_VERTEXPARAM_NORMAL;
    for (i = 1; i < n; i++) {
        end = i % 2;
        plain_ends[end].x = p->p[i].x;
        plain_ends[end].y = p->p[i].y;
        plain_ends[end].color = p->col[i].color;
        plain_ends[end].control = KM_VERTEXPARAM_NORMAL;
        send_plain(&plain_ends[0], &plain_ends[1]);
    }
}

/* draw lines from the first of n points to each of the others */
static void fan_textured(NJS_POINT2COL *p, Int n)
{
    Int i;

    textured_ends[0].x = p->p[0].x;
    textured_ends[0].y = p->p[0].y;
    textured_ends[0].u = p->tex[0].tex.u * UV_SCALE;
    textured_ends[0].v = p->tex[0].tex.v * UV_SCALE;
    textured_ends[0].color = p->col[0].color;
    textured_ends[0].control = KM_VERTEXPARAM_NORMAL;
    for (i = 1; i < n; i++) {
        textured_ends[1].x = p->p[i].x;
        textured_ends[1].y = p->p[i].y;
        textured_ends[1].u = p->tex[i].tex.u * UV_SCALE;
        textured_ends[1].v = p->tex[i].tex.v * UV_SCALE;
        textured_ends[1].color = p->col[i].color;
        textured_ends[1].control = KM_VERTEXPARAM_NORMAL;
        send_textured(&textured_ends[0], &textured_ends[1]);
    }
}

static void fan_plain(NJS_POINT2COL *p, Int n)
{
    Int i;

    plain_ends[0].x = p->p[0].x;
    plain_ends[0].y = p->p[0].y;
    plain_ends[0].color = p->col[0].color;
    plain_ends[0].control = KM_VERTEXPARAM_NORMAL;
    for (i = 1; i < n; i++) {
        plain_ends[1].x = p->p[i].x;
        plain_ends[1].y = p->p[i].y;
        plain_ends[1].color = p->col[i].color;
        plain_ends[1].control = KM_VERTEXPARAM_NORMAL;
        send_plain(&plain_ends[0], &plain_ends[1]);
    }
}

/* draw lines on the screen at priority pri. attr selects how the points join,
 * by NJD_DRAW_MASK, and whether the lines are textured and translucent. n
 * counts the lines for pairs, and the points for a fan or a strip. the lines'
 * z is -1 / pri for pri below -1, else 1 */
void njDrawLine2D(NJS_POINT2COL *p, Int n, Float pri, Uint32 attr)
{
    Int textured;
    Int draw;

    njGetkmPointer();
    textured = (attr & NJD_USE_TEXTURE) > 0;
    line_start[textured](pri, (attr & NJD_TRANSPARENT) > 0);
    draw = (textured * 3) + (attr & NJD_DRAW_MASK);
    line_draw[draw](p, n);
    njEndVertex(line_out);
    njSetkmPointer();
}
