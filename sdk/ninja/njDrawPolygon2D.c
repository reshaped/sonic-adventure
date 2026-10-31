/* 2D polygons, filled or outlined */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>

/* step sq back a word and store value there */
#define SQ_PUT(type, value) (*(type *)--sq = (value))

/* half the edge width, in pixels */
#define HALF_WIDTH 0.525F

/* NJS_COLOR's tex.u and tex.v count 256 to the texture's side */
#define TEX_SCALE (1.0F / 256.0F)

/* the most corners a filled polygon may have */
#define CORNERS 32

/* a 32-byte vertex as the tile accelerator takes it */
struct vertex {
    Uint32 pcw;
    Float x, y, z;
    Float u, v;
    Uint32 col;
    Uint32 offset;
};

static void begin_plain(Float pri, Int trans);
static void begin_textured(Float pri, Int trans);
static void outline_plain(NJS_POINT2COL *p, Int n);
static void outline_textured(NJS_POINT2COL *p, Int n);
static void fill_plain(NJS_POINT2COL *p, Int n);
static void fill_textured(NJS_POINT2COL *p, Int n);

/* begin is indexed by texture, and draw by texture plus 2 for a fill. the color
 * modes are indexed by transparency */
static void (*begin[])(Float pri, Int trans) = {begin_plain, begin_textured};
static void (*draw[])(NJS_POINT2COL *p, Int n) = {
    outline_plain, outline_textured, fill_plain, fill_textured};
static Uint32 polygon_plain_modes[] = {COLOR_MODE_OPAQUE, COLOR_MODE_TRANS};
static Uint32 polygon_textured_modes[] = {
    COLOR_MODE_OPAQUE | COLOR_MODE_TEXTURE,
    COLOR_MODE_TRANS | COLOR_MODE_TEXTURE};
static Float depth;
static Uint32 *sq;
/* an outline's corners: the two an edge joins, then a copy of the first */
static struct vertex plain_edge[3];
static struct vertex textured_edge[3];

static void begin_plain(Float pri, Int trans)
{
    njSetkmColorMode(polygon_plain_modes[trans]);
    if (trans) {
        sq = njStartVertexNonTexTrans();
    } else {
        sq = njStartVertexNonTexOpaque();
    }
    depth = pri < -1.0F ? -1.0F / pri : 1.0F;
}

static void begin_textured(Float pri, Int trans)
{
    njSetkmColorMode(polygon_textured_modes[trans]);
    if (trans) {
        sq = njStartVertexTexTrans();
    } else {
        sq = njStartVertexTexOpaque();
    }
    depth = pri < -1.0F ? -1.0F / pri : 1.0F;
}

/* send the edge from a to b as a strip of four vertices, HALF_WIDTH to either
 * side of it */
static void edge_plain(struct vertex *a, struct vertex *b)
{
    Float dx;
    Float dy;
    Float inv_length;
    Float side_x;
    Float side_y;

    dx = b->x - a->x;
    dy = b->y - a->y;
    inv_length = 1.0F / _builtin_sqrtf((dx * dx) + (dy * dy));
    side_x = dy * inv_length * HALF_WIDTH;
    side_y = -(dx * inv_length * HALF_WIDTH);
    /* without a texture, u and v, each vertex's words 4 and 5, go unwritten */
    sq += 7;
    SQ_PUT(Uint32, a->col);
    sq -= 2;
    SQ_PUT(Float, depth);
    SQ_PUT(Float, a->y + side_y);
    SQ_PUT(Float, a->x + side_x);
    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(sq);
    sq += 8;
    sq += 7;
    SQ_PUT(Uint32, a->col);
    sq -= 2;
    SQ_PUT(Float, depth);
    SQ_PUT(Float, a->y - side_y);
    SQ_PUT(Float, a->x - side_x);
    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(sq);
    sq += 8;
    sq += 7;
    SQ_PUT(Uint32, b->col);
    sq -= 2;
    SQ_PUT(Float, depth);
    SQ_PUT(Float, b->y + side_y);
    SQ_PUT(Float, b->x + side_x);
    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(sq);
    sq += 8;
    sq += 7;
    SQ_PUT(Uint32, b->col);
    sq -= 2;
    SQ_PUT(Float, depth);
    SQ_PUT(Float, b->y - side_y);
    SQ_PUT(Float, b->x - side_x);
    SQ_PUT(Uint32, KM_VERTEXPARAM_ENDOFSTRIP);
    prefetch(sq);
    sq += 8;
}

static void edge_textured(struct vertex *a, struct vertex *b)
{
    Float dx;
    Float dy;
    Float inv_length;
    Float side_x;
    Float side_y;

    dx = b->x - a->x;
    dy = b->y - a->y;
    inv_length = 1.0F / _builtin_sqrtf((dx * dx) + (dy * dy));
    side_x = dy * inv_length * HALF_WIDTH;
    side_y = -(dx * inv_length * HALF_WIDTH);
    sq += 7;
    SQ_PUT(Uint32, a->col);
    SQ_PUT(Float, a->v);
    SQ_PUT(Float, a->u);
    SQ_PUT(Float, depth);
    SQ_PUT(Float, a->y + side_y);
    SQ_PUT(Float, a->x + side_x);
    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(sq);
    sq += 8;
    sq += 7;
    SQ_PUT(Uint32, a->col);
    SQ_PUT(Float, a->v);
    SQ_PUT(Float, a->u);
    SQ_PUT(Float, depth);
    SQ_PUT(Float, a->y - side_y);
    SQ_PUT(Float, a->x - side_x);
    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(sq);
    sq += 8;
    sq += 7;
    SQ_PUT(Uint32, b->col);
    SQ_PUT(Float, b->v);
    SQ_PUT(Float, b->u);
    SQ_PUT(Float, depth);
    SQ_PUT(Float, b->y + side_y);
    SQ_PUT(Float, b->x + side_x);
    SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
    prefetch(sq);
    sq += 8;
    sq += 7;
    SQ_PUT(Uint32, b->col);
    SQ_PUT(Float, b->v);
    SQ_PUT(Float, b->u);
    SQ_PUT(Float, depth);
    SQ_PUT(Float, b->y - side_y);
    SQ_PUT(Float, b->x - side_x);
    SQ_PUT(Uint32, KM_VERTEXPARAM_ENDOFSTRIP);
    prefetch(sq);
    sq += 8;
}

/* send a strip of n triangles from the first n + 2 points of p */
static void polygon_strip_textured(NJS_POINT2COL *p, Int n)
{
    Int i;

    n++;
    for (i = 0; i < n; i++) {
        sq += 7;
        SQ_PUT(Uint32, p->col[i].color);
        SQ_PUT(Float, p->tex[i].tex.v * TEX_SCALE);
        SQ_PUT(Float, p->tex[i].tex.u * TEX_SCALE);
        SQ_PUT(Float, depth);
        SQ_PUT(Float, p->p[i].y);
        SQ_PUT(Float, p->p[i].x);
        SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
        prefetch(sq);
        sq += 8;
    }
    sq += 7;
    SQ_PUT(Uint32, p->col[i].color);
    SQ_PUT(Float, p->tex[i].tex.v * TEX_SCALE);
    SQ_PUT(Float, p->tex[i].tex.u * TEX_SCALE);
    SQ_PUT(Float, depth);
    SQ_PUT(Float, p->p[i].y);
    SQ_PUT(Float, p->p[i].x);
    SQ_PUT(Uint32, KM_VERTEXPARAM_ENDOFSTRIP);
    prefetch(sq);
    sq += 8;
}

static void polygon_strip_plain(NJS_POINT2COL *p, Int n)
{
    Int i;

    n++;
    for (i = 0; i < n; i++) {
        sq += 7;
        SQ_PUT(Uint32, p->col[i].color);
        sq -= 2;
        SQ_PUT(Float, depth);
        SQ_PUT(Float, p->p[i].y);
        SQ_PUT(Float, p->p[i].x);
        SQ_PUT(Uint32, KM_VERTEXPARAM_NORMAL);
        prefetch(sq);
        sq += 8;
    }
    sq += 7;
    SQ_PUT(Uint32, p->col[i].color);
    sq -= 2;
    SQ_PUT(Float, depth);
    SQ_PUT(Float, p->p[i].y);
    SQ_PUT(Float, p->p[i].x);
    SQ_PUT(Uint32, KM_VERTEXPARAM_ENDOFSTRIP);
    prefetch(sq);
    sq += 8;
}

/* fill a polygon of n corners as a strip that takes its corners in turn from
 * the front and from the back */
static void fill_plain(NJS_POINT2COL *p, Int n)
{
    NJS_POINT2COL strip;
    NJS_POINT2 points[CORNERS];
    NJS_COLOR colors[CORNERS];
    Int front;
    Int back;
    Int i;

    for (front = i = 0, back = n - 1; front < ((n + 1) >> 1);) {
        points[i] = p->p[front];
        colors[i++].color = p->col[front++].color;
        points[i] = p->p[back];
        colors[i++].color = p->col[back--].color;
    }
    strip.p = points;
    strip.col = colors;
    polygon_strip_plain(&strip, n - 2);
}

static void fill_textured(NJS_POINT2COL *p, Int n)
{
    NJS_POINT2COL strip;
    NJS_POINT2 points[CORNERS];
    NJS_COLOR colors[CORNERS];
    NJS_COLOR texs[CORNERS];
    Int front;
    Int back;
    Int i;

    for (front = i = 0, back = n - 1; front < ((n + 1) >> 1);) {
        points[i] = p->p[front];
        texs[i].tex.u = p->tex[front].tex.u;
        texs[i].tex.v = p->tex[front].tex.v;
        colors[i++].color = p->col[front++].color;
        points[i] = p->p[back];
        texs[i].tex.u = p->tex[back].tex.u;
        texs[i].tex.v = p->tex[back].tex.v;
        colors[i++].color = p->col[back--].color;
    }
    strip.p = points;
    strip.col = colors;
    strip.tex = texs;
    polygon_strip_textured(&strip, n - 2);
}

/* outline a polygon of n corners. each corner from the second replaces the
 * older end, and an edge joins the two ends. a last edge, back to the first
 * corner, closes the outline */
static void outline_plain(NJS_POINT2COL *p, Int n)
{
    Int i;
    Int place = 0;

    plain_edge[0].x = p->p[0].x;
    plain_edge[0].y = p->p[0].y;
    plain_edge[0].col = p->col[0].color;
    plain_edge[0].pcw = KM_VERTEXPARAM_NORMAL;
    plain_edge[2] = plain_edge[0];
    for (i = 1; i < n; i++) {
        place = i % 2;
        plain_edge[place].x = p->p[i].x;
        plain_edge[place].y = p->p[i].y;
        plain_edge[place].col = p->col[i].color;
        plain_edge[place].pcw = KM_VERTEXPARAM_NORMAL;
        edge_plain(&plain_edge[0], &plain_edge[1]);
    }
    edge_plain(&plain_edge[place], &plain_edge[2]);
}

static void outline_textured(NJS_POINT2COL *p, Int n)
{
    Int i;
    Int place = 0;

    textured_edge[0].x = p->p[0].x;
    textured_edge[0].y = p->p[0].y;
    textured_edge[0].u = p->tex[0].tex.u * TEX_SCALE;
    textured_edge[0].v = p->tex[0].tex.v * TEX_SCALE;
    textured_edge[0].col = p->col[0].color;
    textured_edge[0].pcw = KM_VERTEXPARAM_NORMAL;
    textured_edge[2] = textured_edge[0];
    for (i = 1; i < n; i++) {
        place = i % 2;
        textured_edge[place].x = p->p[i].x;
        textured_edge[place].y = p->p[i].y;
        textured_edge[place].u = p->tex[i].tex.u * TEX_SCALE;
        textured_edge[place].v = p->tex[i].tex.v * TEX_SCALE;
        textured_edge[place].col = p->col[i].color;
        textured_edge[place].pcw = KM_VERTEXPARAM_NORMAL;
        edge_textured(&textured_edge[0], &textured_edge[1]);
    }
    edge_textured(&textured_edge[place], &textured_edge[2]);
}

/* draw a polygon of n corners on the screen at priority pri. attr selects
 * whether it is filled, by NJD_FILL, and whether it is textured and
 * translucent. the vertices' z is -1 / pri for pri below -1, else 1 */
void njDrawPolygon2D(NJS_POINT2COL *p, Int n, Float pri, Uint32 attr)
{
    Int kind;
#ifdef GCC
    /* the split frame keeps the draw for a picture to come. where the 2D
     * draws squeeze, the draw takes a copy of p with its points' x
     * squeezed, but an untextured polygon that covers the screen keeps x.
     * the copy has room for CORNERS points, the most a filled polygon may
     * have, so a draw of more keeps its x */
    NJS_POINT2COL squeezed;
    NJS_POINT2 points[CORNERS];
    Int i;

    present_record(PRESENT_POLYGON_2D, p, n, 0, pri, attr);
    if (n <= CORNERS && port_wide_squeezes() &&
        ((attr & NJD_USE_TEXTURE) ||
         !port_wide_covers(&p->p->x, n, sizeof(*p->p) / sizeof(Float)))) {
        for (i = 0; i < n; i++) {
            points[i].x = port_wide_x(p->p[i].x);
            points[i].y = p->p[i].y;
        }
        squeezed = *p;
        squeezed.p = points;
        p = &squeezed;
    }
#endif

    njGetkmPointer();
    kind = (attr & NJD_USE_TEXTURE) > 0;
    begin[kind](pri, (attr & NJD_TRANSPARENT) > 0);
    kind += ((attr & NJD_FILL) > 0) * 2;
    draw[kind](p, n);
    njEndVertex(sq);
    njSetkmPointer();
}
