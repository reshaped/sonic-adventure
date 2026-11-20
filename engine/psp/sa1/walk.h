/* SA1's model walk on the PSP, which records.c includes there, as the
 * Dreamcast's walk compiles inside the layer's object (engine/dc/sa1/). the
 * PSP has no memory for the records' way with a model: a name by its bytes,
 * a copy of it in the client, and its triangles made again for each picture.
 * so the walk draws a model at the game's call, from the game's own bytes,
 * through draw.h: it lights each point from the picture's palettes, and the
 * engine carries the strips from the model's space by Ninja's matrix.
 *
 * a meshset that blends draws at the picture's end, after every other, the
 * farthest model's first, where the records' blend list sorts triangle by
 * triangle. one that blends its texture's alpha over what lies behind, at
 * its own alpha of 255, cuts where its texture's texels are each clear or
 * solid but for an edge's few: it draws at the call, less the clear pixels,
 * which needs no order.
 *
 * it takes the game's own drawer's models, which shade by the palettes, and
 * those of Ninja's easy and simple drawers, which scale a material's color
 * by each point's light (SA_SHADE_RAMP and SA_SHADE_LINEAR, as the scene
 * shades them in engine/pc/scene.cpp).
 *
 * and Ninja's full drawer's, which lights each point by its queue of lights
 * (SA_SHADE_LIGHTS, by engine/pc/ninja.h's ninja_lit).
 *
 * a model whose ball reaches past the raster's reach of the frame goes a
 * triangle at a time, each cut to what lies within (psp_draw_cut): the
 * engine drops a triangle whole where a corner lies too far to a side.
 *
 * the walk leaves a model out, and counts it, where it has more points or
 * materials than the walk's room, or a texture the raster can't make.
 *
 * the walk leaves out what the PSP's raster does: the offset color, fog, and
 * a texture's mirrored repeat */
#include "draw.h"

#define WALK_POINTS 2048
#define WALK_MATERIALS 256
/* as many as the shim's records of texture memory, engine/native/shim.c's */
#define WALK_TEXTURES 2048
/* the most corners of a strip that goes a triangle at a time */
#define WALK_STRIP 1024

_Static_assert(sizeof(sa_material) ==
                   sizeof(sa_cap_material) - offsetof(sa_cap_material, texture),
               "a recorded material's bytes from its texture on aren't an "
               "sa_material's");

/* the raster's texture of each record of texture memory the game drew from,
 * 0 where it made none. tried is set from the first try to the record's
 * release, so a texture the raster can't make is asked for once */
static struct {
    uint16_t made;
    uint8_t tried;
} walk_textures[WALK_TEXTURES];
/* set where a draw asked for a texture the raster couldn't make */
static int walk_refused;
/* whether the raster holds the open picture's commands open for the walk,
 * as begin_picture found */
static int walk_open;

/* the raster's texture of the record of texture memory at texaddr, made at
 * its first draw: from the disc, by the record's index, named's name and
 * its file, or under NJD_TEXATTR_TYPE_MEMORY from the texels named's
 * NJS_TEXINFO holds. PSP_TEXTURE_FAILED where the raster makes none */
static uint32_t walk_texture_at(uint32_t texaddr, uint32_t attr,
                                const void *named)
{
    uint32_t slot = native_texture_slot(texaddr);

    if (slot == 0 || slot >= WALK_TEXTURES) {
        walk_refused = 1;
        return PSP_TEXTURE_FAILED;
    }
    if (!walk_textures[slot].tried) {
        const NJS_TEXMEMLIST *memory = native_widen(texaddr);
        uint32_t game = native_fpu_host();
        uint32_t made = 0;

        if (attr & NJD_TEXATTR_TYPE_MEMORY) {
            const NJS_TEXINFO *info = named;

            if (info) {
                uint32_t type = info->texsurface.Type >> 16;
                uint32_t layout = type >> 8 & 0xff;

                made = psp_texture_texels(
                    type & 0xff, layout, info->texsurface.nWidth,
                    info->texsurface.nHeight, info->texaddr,
                    texels_bytes(layout, info->texsurface.nWidth,
                                 info->texsurface.nHeight));
            }
        } else {
            made = psp_texture_load(memory->globalIndex, named,
                                    native_texture_file(texaddr));
        }
        native_fpu_game(game);
        if (made == 0) {
            const NJS_TEXINFO *info =
                attr & NJD_TEXATTR_TYPE_MEMORY ? named : NULL;

            fprintf(stderr,
                    "sa1: the raster made no texture of global index %u: %s "
                    "%s, type %x, %u by %u\n",
                    (unsigned)memory->globalIndex,
                    info ? "the game's own texels" : "the disc's",
                    info || !named ? "" : (const char *)named,
                    info ? (unsigned)(info->texsurface.Type >> 16) : 0,
                    info ? (unsigned)info->texsurface.nWidth : 0,
                    info ? (unsigned)info->texsurface.nHeight : 0);
        }
        walk_textures[slot].tried = 1;
        walk_textures[slot].made = made <= UINT16_MAX ? (uint16_t)made : 0;
        if (made > UINT16_MAX) {
            psp_texture_free(made);
        }
    }
    if (walk_textures[slot].made == 0) {
        walk_refused = 1;
        return PSP_TEXTURE_FAILED;
    }
    return walk_textures[slot].made;
}

static uint32_t walk_texture(const NJS_TEXLIST *list, uint32_t index)
{
    if (!list || index >= list->nbTexture || !list->textures[index].texaddr) {
        return PSP_NO_TEXTURE;
    }
    return walk_texture_at(list->textures[index].texaddr,
                           list->textures[index].attr,
                           list->textures[index].filename);
}

/* the game released list's records of texture memory: the textures the
 * raster made of them go too */
static void walk_released(const NJS_TEXLIST *list)
{
    for (uint32_t i = 0; i < list->nbTexture; i++) {
        uint32_t slot = native_texture_slot(list->textures[i].texaddr);

        if (slot != 0 && slot < WALK_TEXTURES) {
            if (walk_textures[slot].made != 0) {
                psp_texture_free(walk_textures[slot].made);
            }
            walk_textures[slot].made = 0;
            walk_textures[slot].tried = 0;
        }
    }
}

/* ARGB as a word to RGBA in memory, as a corner holds a color */
static inline uint32_t walk_rgba(uint32_t argb)
{
    return (argb & 0xff00ff00U) | (argb >> 16 & 0xff) | (argb & 0xff) << 16;
}

/* a corner's u or v from a number of 1 for the texture's side */
static inline int16_t walk_uv(float value)
{
    float scaled = value * 255.0F;

    return (int16_t)(scaled > 32767.0F    ? 32767
                     : scaled < -32768.0F ? -32768
                                          : (int)scaled);
}

/* a meshset's polygons as strips: n takes a polygon's count of points and
 * reversed whether its strip runs reversed, and at moves to its first point's
 * index */
static inline void walk_polygon(const NJS_MESHSET *set, const Sint16 **at,
                                uint32_t *n, int *reversed)
{
    uint32_t type = set->type_matId >> MESHSET_TYPE_SHIFT;

    *reversed = 0;
    *n = type == 0 ? 3 : 4;
    if (type > 1) {
        uint32_t head = (uint16_t)*(*at)++;

        *n = head & STRIP_COUNT;
        *reversed = (head & NJD_TRIMESH_END) != 0;
    }
}

/* what a meshset's corners are made by */
struct walk_set {
    const NJS_MODEL *model;
    const NJS_COLOR *uv; /* the meshset's, by corner, or null */
    const sa_cap_material *material;
    uint32_t *colors; /* by point: a color, or under Ninja's easy and simple
                       * drawers how much of the material's the point takes */
    int16_t (*env)[2];
    uint32_t points;
    uint8_t mode;
    int by_normal; /* its u and v are the normal's, in view space */
    /* under the palettes: the constant color in every point's place, or
     * each point's with whiten's bits set, keep's kept and alpha added */
    int same;
    uint32_t one, whiten, keep, alpha;
    /* under Ninja's drawers: the material's color, and whether a point
     * takes it whole */
    uint32_t own;
    int whole;
    /* under Ninja's full drawer: its lights, the matrix to the world, and
     * the material each point's color was last lit for, by key */
    const sa_lights *lights;
    const float *world;
    uint16_t *lit_for;
    uint16_t key;
};

/* corner's place in the meshset, of point i, into out */
static inline void walk_corner(const struct walk_set *w, struct psp_corner *out,
                               uint32_t i, uint32_t corner)
{
    const NJS_POINT3 *p;
    uint32_t color;

    if (i >= w->points) {
        i = 0;
    }
    p = &w->model->points[i];
    out->u = w->by_normal ? w->env[i][0] : w->uv ? w->uv[corner].tex.u : 0;
    out->v = w->by_normal ? w->env[i][1] : w->uv ? w->uv[corner].tex.v : 0;
    if (w->mode == SA_SHADE_PALETTE) {
        color = w->same ? w->one
                        : ((w->colors[i] | w->whiten) & w->keep) | w->alpha;
    } else if (w->whole) {
        color = w->own;
    } else if (w->mode == SA_SHADE_LIGHTS) {
        /* a point is lit once for each material, as njModel.c's lit_key
         * keeps it */
        if (w->lit_for[i] != w->key) {
            const float *to = w->world;
            float at[3], normal[3] = {0.0F, 0.0F, 0.0F};
            uint32_t base, offset;

            for (int j = 0; j < 3; j++) {
                at[j] = to[4 * j] * p->x + to[4 * j + 1] * p->y +
                        to[4 * j + 2] * p->z + to[4 * j + 3];
            }
            if (w->model->normals) {
                const NJS_VECTOR *n = &w->model->normals[i];

                for (int j = 0; j < 3; j++) {
                    normal[j] = to[4 * j] * n->x + to[4 * j + 1] * n->y +
                                to[4 * j + 2] * n->z;
                }
            }
            psp_ninja_lit(w->lights, &w->material->texture, at, normal, &base,
                          &offset);
            w->colors[i] = walk_rgba(base);
            w->lit_for[i] = w->key;
        }
        color = w->colors[i];
    } else {
        /* each of the color's three by the light over 255, as ninja_scaled
         * divides */
        uint32_t by = w->colors[i];
        uint32_t red = (w->own & 0xff) * by;
        uint32_t green = (w->own >> 8 & 0xff) * by;
        uint32_t blue = (w->own >> 16 & 0xff) * by;

        color = (w->own & 0xff000000U) | ((blue + 1 + (blue >> 8)) >> 8) << 16 |
                ((green + 1 + (green >> 8)) >> 8) << 8 |
                ((red + 1 + (red >> 8)) >> 8);
    }
    out->color = color;
    out->x = p->x;
    out->y = p->y;
    out->z = p->z;
}

/* the log's word of a model the walk leaves out, for the first few it
 * leaves */
static void walk_left(const NJS_MODEL *model, const char *why)
{
    static const NJS_MODEL *said[8];
    static int count;

    for (int i = 0; i < count; i++) {
        if (said[i] == model) {
            return;
        }
    }
    if (count < (int)(sizeof said / sizeof said[0])) {
        said[count++] = model;
        fprintf(stderr,
                "sa1: the walk leaves out the model at %p, of %d points, %d "
                "meshsets and %u materials: %s\n",
                (const void *)model, (int)model->nbPoint, (int)model->nbMeshset,
                (unsigned)materials_named(model), why);
    }
}

/* draw model under state, 1, or leave it out, 0 */
static int walk_model(const NJS_MODEL *model, uint8_t mode,
                      const struct draw_state *state)
{
    static uint32_t colors[WALK_POINTS];
    static int16_t env[WALK_POINTS][2];
    static uint16_t lit_for[WALK_POINTS];
    static struct psp_corner strip[WALK_STRIP];
    /* the walk draws one model at a time, and a thread's stack is small:
     * its materials lie off the stack, with its points' colors */
    static sa_cap_material materials[WALK_MATERIALS];
    static uint8_t cuts[WALK_MATERIALS];
    const float *m = native_xmtrx();
    const uint32_t *words =
        palettes_words + state->palette * SA_PALETTE_ENTRIES * 2;
    uint32_t points = (uint32_t)model->nbPoint;
    uint32_t count = materials_named(model);
    float light[3], eye[3], longest = 0.0F, far = 0.0F, radius;
    float across = psp_draw_reach() * 320.0F / sa1_screen.scale_x;
    float down = psp_draw_reach() * 240.0F / sa1_screen.scale_y;
    const NJS_POINT3 *c = &model->center;
    /* the light in view space, and what Ninja's drawers scale a color by */
    double seen[3] = {sa1_live_light.x, sa1_live_light.y, sa1_live_light.z};
    float ambient = 0.0F, diffuse = 0.0F;
    int by_palette = mode == SA_SHADE_PALETTE;
    int by_lights = mode == SA_SHADE_LIGHTS;
    sa_lights lights;
    float world[12];
    uint32_t constant;
    /* SA1's shade: a meshset under SA_MAT_WHITE leaves the draw's points
     * white for itself and each meshset after it */
    int mapped = 0, white = 0, cutting;

    if (!walk_open || points > WALK_POINTS || count > WALK_MATERIALS ||
        model->nbMeshset == 0) {
        walk_left(model, !walk_open               ? "no picture is open"
                         : points > WALK_POINTS   ? "it has too many points"
                         : count > WALK_MATERIALS ? "it has too many materials"
                                                  : "it has no meshes");
        return 0;
    }
    walk_refused = 0;
    for (uint32_t i = 0; i < count; i++) {
        const sa_cap_material *made = &materials[i];

        materials[i] = material_as_drawn(&model->mats[i], mode, state, 0);
        cuts[i] = (made->flags & SA_MAT_BLEND) &&
                  (made->flags & SA_MAT_TEXTURE) &&
                  made->src == SA_BLEND_SRC_ALPHA &&
                  made->dst == SA_BLEND_INV_SRC_ALPHA &&
                  (by_palette ? made->alpha : made->diffuse >> 24) == 255 &&
                  psp_texture_cuts(made->texture);
        mapped |= made->flags & SA_MAT_ENV;
    }
    if (walk_refused) {
        walk_left(model, "the raster made no texture one of its materials "
                         "takes");
        return 0;
    }
    if (mode == SA_SHADE_RAMP) {
        sa1_easy_light(seen, &ambient, &diffuse);
    } else if (mode == SA_SHADE_LINEAR) {
        sa1_simple_light(seen, &ambient, &diffuse);
    } else if (by_lights) {
        lights_now(&lights);
        to_world(world);
        memset(lit_for, 0, points * sizeof lit_for[0]);
    }
    /* the light in the model's space, by its dots with the rows that carry
     * the model's axes to the view's */
    for (int k = 0; k < 3; k++) {
        float length = m[4 * k] * m[4 * k] + m[4 * k + 1] * m[4 * k + 1] +
                       m[4 * k + 2] * m[4 * k + 2];

        light[k] = m[4 * k] * (float)seen[0] + m[4 * k + 1] * (float)seen[1] +
                   m[4 * k + 2] * (float)seen[2];
        longest = length > longest ? length : longest;
    }
    /* each point's color, from the palette's entry its normal's dot with the
     * light picks, or under Ninja's easy and simple drawers how much of a
     * material's color the point takes, of 255. and how far the farthest
     * point lies from the model's middle */
    for (uint32_t i = 0; i < points; i++) {
        const NJS_POINT3 *p = &model->points[i];
        float dx = p->x - c->x, dy = p->y - c->y, dz = p->z - c->z;
        float apart = dx * dx + dy * dy + dz * dz;

        far = apart > far ? apart : far;
        if (!model->normals) {
            colors[i] = by_palette ? 0xffffffffU : 255;
            continue;
        }
        const NJS_VECTOR *n = &model->normals[i];
        float dot = n->x * light[0] + n->y * light[1] + n->z * light[2];

        if (mapped) {
            /* an env map's uv: the normal turned into view space */
            env[i][0] = walk_uv(
                (n->x * m[0] + n->y * m[4] + n->z * m[8]) * 0.5F + 0.5F);
            env[i][1] = walk_uv(
                (n->x * m[1] + n->y * m[5] + n->z * m[9]) * 0.5F + 0.5F);
        }
        if (by_palette) {
            float scaled = dot * 127.5F + 127.5F;
            int entry =
                scaled > 0.0F ? (scaled < 255.0F ? (int)scaled : 255) : 0;

            colors[i] = walk_rgba(words[entry * 2]);
        } else if (!by_lights) {
            /* ninja_intensity's, as ninja_scaled holds it */
            float product = -diffuse * dot;
            int held;

            if (mode == SA_SHADE_RAMP && !(product > 0.0F)) {
                product = 0.0F;
            }
            held = (int)((ambient + product) * 255.0F);
            colors[i] = held < 0 ? 0 : held > 255 ? 255 : (uint32_t)held;
        }
    }
    /* the model's ball in view space. within the raster's reach of the frame
     * on each side, its strips go as they are. else each goes a triangle at
     * a time, cut to what lies within */
    for (int j = 0; j < 3; j++) {
        eye[j] = c->x * m[j] + c->y * m[4 + j] + c->z * m[8 + j] + m[12 + j];
    }
    radius = sqrtf(far * longest);
    cutting =
        !(across * eye[2] - fabsf(eye[0]) >=
          radius * sqrtf(1.0F + across * across)) ||
        !(down * eye[2] - fabsf(eye[1]) >= radius * sqrtf(1.0F + down * down));
    /* the view counts the draw, as it does a recorded one, so the picture's
     * own view stays the one most of its draws lie under */
    view_id(1);
    constant = walk_rgba(
        argb(1.0, state->material.r, state->material.g, state->material.b));
    psp_draw_screen(sa1_screen.scale_x, sa1_screen.scale_y, sa1_screen.cx,
                    sa1_screen.cy);
    psp_draw_place(m);
    for (uint32_t s = 0; s < model->nbMeshset; s++) {
        const NJS_MESHSET *set = &model->meshsets[s];
        uint32_t named = set->type_matId & MATERIAL_ID;
        const sa_cap_material *material = &materials[named];
        int blended = (material->flags & SA_MAT_BLEND) != 0;
        int cut = cuts[named];
        struct walk_set w = {
            .model = model,
            .uv = set->vertuv,
            .material = material,
            .colors = colors,
            .env = env,
            .points = points,
            .mode = mode,
            .by_normal = (material->flags & SA_MAT_ENV) && model->normals,
            .same = (material->flags & SA_MAT_CONSTANT) != 0,
            .own = walk_rgba(material->diffuse),
            .whole = !by_palette && (material->flags & SA_MAT_UNLIT) != 0 &&
                     mode != SA_SHADE_RAMP,
            .lights = &lights,
            .world = world,
            .lit_for = lit_for,
            .key = (uint16_t)(named + 1)};
        struct psp_state drawn = {
            .texture = material->flags & SA_MAT_TEXTURE ? material->texture : 0,
            .decal = (material->flags & SA_MAT_DECAL) != 0,
            .clamp_u = (material->flags & SA_MAT_CLAMP_U) != 0,
            .clamp_v = (material->flags & SA_MAT_CLAMP_V) != 0,
            .point = material->filter == SA_FILTER_POINT,
            .depth = (uint8_t)((state->isp & ISP_DEPTH) >> ISP_DEPTH_SHIFT),
            .depth_write = !(state->isp & ISP_Z_WRITE_DISABLE),
            .cull = material->flags & SA_MAT_CULL ? 1 : 0,
            .cut = (uint8_t)cut,
            .blend = (uint8_t)(blended && !cut),
            .src = material->src,
            .dst = material->dst};
        const Sint16 *at = set->meshes;
        uint32_t corners = 0, strips = 0, corner = 0;
        struct psp_corner *out, *first;

        white |= (material->flags & SA_MAT_WHITE) != 0;
        /* under the palettes a point's color is its entry's: white where a
         * meshset that blends is unlit, and once a meshset whitened. one
         * that blends takes its material's alpha */
        w.whiten = white || (blended && (material->flags & SA_MAT_UNLIT))
                       ? 0x00ffffffU
                       : 0;
        w.keep = blended ? 0x00ffffffU : 0xffffffffU;
        w.alpha = blended ? (uint32_t)material->alpha << 24 : 0;
        w.one = (constant & w.keep) | w.alpha;
        if (cutting) {
            if (!drawn.blend) {
                psp_draw_state(&drawn);
            }
            for (uint32_t p = 0; p < set->nbMesh; p++) {
                uint32_t n;
                int reversed;

                walk_polygon(set, &at, &n, &reversed);
                if (n >= 3 && n <= WALK_STRIP) {
                    for (uint32_t k = 0; k < n; k++) {
                        walk_corner(&w, &strip[k], (uint16_t)at[k], corner + k);
                    }
                    psp_draw_cut(strip, n, reversed,
                                 drawn.blend ? &drawn : NULL, eye[2]);
                }
                at += n;
                corner += n;
            }
            continue;
        }
        /* a meshset that drops no side goes as one strip: its polygons'
         * strips, each joined to the next by its last point and the next's
         * first again, which make triangles of no area */
        for (uint32_t p = 0; p < set->nbMesh; p++) {
            uint32_t n;
            int reversed;

            walk_polygon(set, &at, &n, &reversed);
            at += n;
            if (n >= 3) {
                corners += n;
                strips++;
            }
        }
        if (strips == 0) {
            continue;
        }
        if (!drawn.blend) {
            psp_draw_state(&drawn);
        }
        if (!(first = psp_draw_room(drawn.cull ? corners
                                               : corners + 2 * (strips - 1),
                                    drawn.blend))) {
            continue;
        }
        out = first;
        at = set->meshes;
        for (uint32_t p = 0; p < set->nbMesh; p++) {
            struct psp_corner *start = out;
            uint32_t n;
            int reversed;

            walk_polygon(set, &at, &n, &reversed);
            if (n < 3) {
                at += n;
                corner += n;
                continue;
            }
            if (!drawn.cull && out != first) {
                out[0] = out[-1];
                out += 2;
            }
            for (uint32_t k = 0; k < n; k++) {
                walk_corner(&w, &out[k], (uint16_t)at[k], corner + k);
            }
            if (!drawn.cull && start != first) {
                out[-1] = out[0];
            }
            out += n;
            at += n;
            corner += n;
            if (drawn.cull) {
                /* a strip that runs reversed turns the other way */
                drawn.cull = (uint8_t)(reversed ? 2 : 1);
                if (drawn.blend) {
                    psp_draw_later(eye[2], &drawn, start, n);
                } else {
                    psp_draw_state(&drawn);
                    psp_draw_strip(start, n);
                }
            }
        }
        if (drawn.cull) {
            continue;
        }
        if (drawn.blend) {
            psp_draw_later(eye[2], &drawn, first, (uint32_t)(out - first));
        } else {
            psp_draw_strip(first, (uint32_t)(out - first));
        }
    }
    return 1;
}
