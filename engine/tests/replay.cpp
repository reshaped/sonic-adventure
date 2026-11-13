/* the PC's readers of the disc, Ninja's lights and the triangles' forms, on
 * bytes and directories made here. it includes sa_capture.h so a C++17
 * compiler checks that header's layout asserts too. main returns 0, or the
 * number of the first check that fails */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "sa_capture.h"

#include "basic.h"
#include "disc.h"
#include "ninja.h"
#include "pvr.h"
#include "scene.h"

namespace {

void put16(std::vector<uint8_t> &b, uint32_t at, uint32_t v)
{
    b[at] = static_cast<uint8_t>(v);
    b[at + 1] = static_cast<uint8_t>(v >> 8);
}

void put32(std::vector<uint8_t> &b, uint32_t at, uint32_t v)
{
    put16(b, at, v & 0xffff);
    put16(b, at + 2, v >> 16);
}

/* a PVRT chunk's 16-byte header for a w by h texture, then bytes of zeroed
 * texels */
std::vector<uint8_t> pvrt(uint32_t pixel, uint32_t layout, uint32_t w,
                          uint32_t h, uint32_t bytes)
{
    std::vector<uint8_t> b(16 + bytes, 0);

    memcpy(b.data(), "PVRT", 4);
    put32(b, 4, 8 + bytes);
    b[8] = static_cast<uint8_t>(pixel);
    b[9] = static_cast<uint8_t>(layout);
    put16(b, 12, w);
    put16(b, 14, h);
    return b;
}

/* write the uv the models here give a meshset's kth strip point, at offset
 * uvs. its u is 255 (k - 1) and its v is -2 u, so each takes either sign.
 * no two points share a u or a v */
void put_uv(std::vector<uint8_t> &b, uint32_t uvs, uint32_t k)
{
    int u = 255 * (static_cast<int>(k) - 1);

    put16(b, uvs + k * 4, static_cast<uint16_t>(u));
    put16(b, uvs + k * 4 + 2, static_cast<uint16_t>(-2 * u));
}

/* whether uv, a corner's, is the kth point's words over 255 */
bool uv_is(const float *uv, uint32_t k)
{
    float u = static_cast<float>(k) - 1.0F;

    return fabsf(uv[0] - u) < 1e-5F && fabsf(uv[1] + 2.0F * u) < 1e-5F;
}

int check()
{
    /* PRS: "ABC" as literals, a short copy of 5 from 3 back, and the end */
    std::vector<uint8_t> prs = {0x67, 'A', 'B', 'C', 0xfd, 0x01, 0x00, 0x00};
    std::vector<uint8_t> plain;
    if (!pc::prs_decode(prs, plain) ||
        plain != std::vector<uint8_t>{'A', 'B', 'C', 'A', 'B', 'C', 'A', 'B'}) {
        return 1;
    }

    /* a twiddled 4 by 4 in ARGB4444, where texel (x, y) has red x and green
     * y. its place interleaves the bits of y and x, y's lowest bit lowest */
    std::vector<uint8_t> tw = pvrt(2, 0x01, 4, 4, 32);
    for (uint32_t y = 0; y < 4; y++) {
        for (uint32_t x = 0; x < 4; x++) {
            uint32_t at = (y & 1) | (x & 1) << 1 | (y & 2) << 1 | (x & 2) << 2;
            put16(tw, 16 + at * 2, 0xf000 | x << 8 | y << 4);
        }
    }
    pc::Image image;
    std::string why;
    if (!pc::pvr_decode(tw, 0, image, why)) {
        return 2;
    }
    for (uint32_t y = 0; y < 4; y++) {
        for (uint32_t x = 0; x < 4; x++) {
            const uint8_t *p = &image.rgba[(y * 4 + x) * 4];
            if (p[0] != x * 17 || p[1] != y * 17 || p[3] != 255) {
                return 3;
            }
        }
    }

    /* VQ: a codebook of 256 blocks of 2 by 2, then a byte a block in
     * twiddled order. block (bx, by) takes code bx + 2 by. code c's texels
     * are RGB565 with red c & 31 and green their place in the block. that
     * place counts down, then across */
    std::vector<uint8_t> vq = pvrt(1, 0x03, 4, 4, 2048 + 4);
    for (uint32_t c = 0; c < 256; c++) {
        for (uint32_t k = 0; k < 4; k++) {
            put16(vq, 16 + c * 8 + k * 2, (c & 31) << 11 | k << 5);
        }
    }
    for (uint32_t by = 0; by < 2; by++) {
        for (uint32_t bx = 0; bx < 2; bx++) {
            vq[16 + 2048 + (by | bx << 1)] = static_cast<uint8_t>(bx + 2 * by);
        }
    }
    if (!pc::pvr_decode(vq, 0, image, why)) {
        return 4;
    }
    for (uint32_t y = 0; y < 4; y++) {
        for (uint32_t x = 0; x < 4; x++) {
            const uint8_t *p = &image.rgba[(y * 4 + x) * 4];
            uint32_t c = x / 2 + 2 * (y / 2);
            if (p[0] != ((c << 3) | (c >> 2)) ||
                p[1] != ((y & 1) | (x & 1) << 1) << 2) {
                return 5;
            }
        }
    }

    /* a twiddled 2 by 2 with its 1 by 1 level. a texel's padding comes
     * first, then the smaller level, the larger and padding past it */
    std::vector<uint8_t> mips = pvrt(1, 0x02, 2, 2, 16);
    for (uint32_t k = 0; k < 4; k++) {
        put16(mips, 16 + 4 + k * 2, (k + 1) << 11);
    }
    if (!pc::pvr_decode(mips, 0, image, why) ||
        image.rgba[0] != ((1 << 3) | (1 >> 2)) ||
        image.rgba[4] != ((3 << 3) | (3 >> 2))) {
        return 13;
    }

    /* rows: a 3 by 2 in ARGB4444 whose texel (x, y) has red x and green
     * y. its rows run from the top */
    std::vector<uint8_t> rows = pvrt(2, 0x09, 3, 2, 12);
    for (uint32_t i = 0; i < 6; i++) {
        put16(rows, 16 + i * 2, 0xf000 | (i % 3) << 8 | (i / 3) << 4);
    }
    if (!pc::pvr_decode(rows, 0, image, why)) {
        return 38;
    }
    for (uint32_t i = 0; i < 6; i++) {
        if (image.rgba[i * 4] != (i % 3) * 17 ||
            image.rgba[i * 4 + 1] != (i / 3) * 17) {
            return 16;
        }
    }
    /* YUV422: a pair of words, U and Y0 then V and Y1, is two texels of a
     * row. a word holds its Y in the high byte. words holds a 2 by 2's two
     * pairs in the order rows store them. a twiddled square and a VQ code
     * store them down, then across. seen is each texel's red, green and
     * blue */
    const uint32_t words[4] = {192 | 100 << 8, 160 | 50 << 8, 64 | 150 << 8,
                               96 | 200 << 8};
    const uint8_t seen[4][3] = {
        {144, 56, 210}, {94, 6, 160}, {106, 194, 40}, {156, 244, 90}};
    const struct {
        uint32_t layout, bytes, order[4];
    } pairs[] = {
        {0x09, 8, {0, 1, 2, 3}},
        {0x01, 8, {0, 2, 1, 3}},
        {0x03, 2048 + 1, {0, 2, 1, 3}},
    };
    for (const auto &c : pairs) {
        std::vector<uint8_t> yuv = pvrt(3, c.layout, 2, 2, c.bytes);
        for (uint32_t k = 0; k < 4; k++) {
            put16(yuv, 16 + k * 2, words[c.order[k]]);
        }
        if (!pc::pvr_decode(yuv, 0, image, why)) {
            return 39;
        }
        for (uint32_t i = 0; i < 4; i++) {
            const uint8_t *p = &image.rgba[i * 4];
            if (memcmp(p, seen[i], 3) != 0 || p[3] != 255) {
                return 17;
            }
        }
    }

    /* each layout's texels end where its chunk does, after the smaller
     * levels and their padding. a codebook takes 8 bytes a code, a VQ block
     * a byte and any other texel 2 bytes. a small VQ's codebook is 16, 32,
     * 64, 128 or 256 codes by its size and levels. a chunk a byte shorter
     * decodes to nothing, though the bytes go on past it */
    const struct {
        uint32_t pixel, layout, w, h, bytes;
    } held[] = {
        {2, 0x01, 4, 4, 32},
        {1, 0x0d, 8, 2, 32},
        {1, 0x02, 4, 4, 2 + 2 + 8 + 32},
        {1, 0x12, 2, 2, 2 + 2 + 8},
        {0, 0x09, 3, 2, 12},
        {1, 0x0b, 5, 1, 10},
        {3, 0x09, 4, 1, 8},
        {3, 0x01, 2, 2, 8},
        {1, 0x03, 4, 4, 2048 + 4},
        {3, 0x03, 2, 2, 2048 + 1},
        {1, 0x04, 4, 4, 2048 + 1 + 1 + 4},
        {1, 0x10, 16, 16, 128 + 64},
        {1, 0x10, 32, 32, 256 + 256},
        {1, 0x10, 64, 64, 1024 + 1024},
        {1, 0x10, 128, 128, 2048 + 4096},
        {1, 0x11, 32, 32, 512 + 1 + 1 + 4 + 16 + 64 + 256},
        {1, 0x11, 64, 64, 2048 + 1 + 1 + 4 + 16 + 64 + 256 + 1024},
    };
    for (const auto &c : held) {
        std::vector<uint8_t> whole = pvrt(c.pixel, c.layout, c.w, c.h, c.bytes);
        if (!pc::pvr_decode(whole, 0, image, why) || image.width != c.w ||
            image.height != c.h || image.rgba.size() != c.w * c.h * 4) {
            return 30;
        }
        put32(whole, 4, 8 + c.bytes - 1);
        if (pc::pvr_decode(whole, 0, image, why)) {
            return 31;
        }
    }
    /* a size its layout or pixel format can't have decodes to nothing. so do
     * a side of 0 and one past PVR_SIDE_MOST. each leaves the image as it
     * was */
    const struct {
        uint32_t pixel, layout, w, h;
    } refused[] = {
        {3, 0x01, 1, 1}, {3, 0x09, 3, 2},  {3, 0x0b, 1, 4},    {1, 0x01, 3, 3},
        {1, 0x01, 4, 6}, {1, 0x0d, 12, 4}, {1, 0x02, 4, 2},    {1, 0x12, 2, 4},
        {1, 0x03, 1, 1}, {1, 0x03, 6, 6},  {1, 0x10, 2, 1},    {1, 0x04, 4, 2},
        {1, 0x11, 8, 4}, {1, 0x01, 0, 4},  {1, 0x09, 4, 1025},
    };
    for (const auto &c : refused) {
        pc::Image kept;
        kept.width = 7;
        kept.height = 9;
        kept.rgba = {1, 2, 3};
        if (pc::pvr_decode(pvrt(c.pixel, c.layout, c.w, c.h, 4096), 0, kept,
                           why) ||
            kept.width != 7 || kept.height != 9 ||
            kept.rgba != std::vector<uint8_t>{1, 2, 3}) {
            return 32;
        }
    }
    /* a chunk whose length passes the bytes, by any amount, decodes to
     * nothing. so does one whose header passes them or would wrap to their
     * start */
    std::vector<uint8_t> past = pvrt(2, 0x01, 4, 4, 32);
    put32(past, 4, 8 + 33);
    if (pc::pvr_decode(past, 0, image, why)) {
        return 33;
    }
    put32(past, 4, 0xfffffff8);
    if (pc::pvr_decode(past, 0, image, why)) {
        return 34;
    }
    std::vector<uint8_t> wrapped = {8, 0, 0, 0, 1, 9, 0, 0, 1, 0, 1, 0, 0, 0};
    if (pc::pvr_decode(wrapped, 0xfffffffc, image, why) ||
        pc::pvr_decode(past, static_cast<uint32_t>(past.size()) - 15, image,
                       why)) {
        return 35;
    }
    /* nor does a chunk that starts past the bytes' end, though their storage
     * still holds one. here the bytes are two chunks cut off with the first's
     * last byte */
    std::vector<uint8_t> cut = tw;
    cut.insert(cut.end(), tw.begin(), tw.end());
    cut.resize(tw.size() - 1);
    if (pc::pvr_decode(cut, static_cast<uint32_t>(tw.size()), image, why)) {
        return 14;
    }
    /* a length counts the header's last 8 bytes. one under 8 decodes to
     * nothing, whether texels follow the header or the bytes end with it */
    for (uint32_t length : {7U, 0U}) {
        std::vector<uint8_t> bare = pvrt(2, 0x01, 4, 4, 0);

        put32(past, 4, length);
        put32(bare, 4, length);
        if (pc::pvr_decode(past, 0, image, why) ||
            pc::pvr_decode(bare, 0, image, why)) {
            return 40;
        }
    }
    /* the index walks a file's chunks by their lengths. a PVR's GBIX and its
     * PVRT are one texture. a chunk whose length passes the file ends the
     * walk. here one such length would wrap the walk back to the file's
     * start */
    std::vector<uint8_t> lone(16, 0);
    pc::Textures textures;
    memcpy(lone.data(), "GBIX", 4);
    put32(lone, 4, 8);
    lone.insert(lone.end(), tw.begin(), tw.end());
    textures.add("A.PVR", false, lone);
    if (textures.count() != 1) {
        return 36;
    }
    put32(lone, 4, 0xfffffff8);
    textures.add("B.PVR", false, lone);
    put32(lone, 4, 0xfffffff0);
    textures.add("C.PVR", false, lone);
    if (textures.count() != 1) {
        return 37;
    }
    /* a PVM's chunks follow its header, here its status, its count of 1 and
     * an entry. a header whose length passes the file leaves no texture,
     * whether or not 8 plus that length wraps */
    std::vector<uint8_t> pvm(16, 0);
    memcpy(pvm.data(), "PVMH", 4);
    put32(pvm, 4, 8);
    put16(pvm, 10, 1);
    pvm.insert(pvm.end(), tw.begin(), tw.end());
    textures.add("D.PVM", false, pvm);
    if (textures.count() != 2) {
        return 41;
    }
    put32(pvm, 4, 0xfffffff8);
    textures.add("E.PVM", false, pvm);
    put32(pvm, 4, static_cast<uint32_t>(pvm.size()));
    textures.add("F.PVM", false, pvm);
    if (textures.count() != 2) {
        return 15;
    }

    /* a model of 4 points and one trimesh meshset, a strip of 4, loaded at
     * 0x0c900000. the strip's triangles alternate their winding, as the
     * PowerVR's do */
    const uint32_t base = 0x0c900000;
    std::vector<uint8_t> file(160, 0);
    put32(file, 0, base + 40); /* points */
    put32(file, 8, 4);
    put32(file, 12, base + 88); /* meshsets */
    put16(file, 20, 1);
    put16(file, 22, 1);
    for (uint32_t i = 0; i < 12; i++) {
        float v = static_cast<float>(i);
        memcpy(&file[40 + i * 4], &v, 4);
    }
    put16(file, 88, 0xc000);
    put16(file, 90, 1);
    put32(file, 92, base + 120); /* meshes */
    const uint16_t strip[5] = {4, 0, 1, 2, 3};
    for (uint32_t i = 0; i < 5; i++) {
        put16(file, 120 + i * 2, strip[i]);
    }
    pc::Model model;
    if (!pc::ninja_model(file, base, 0, model, why) ||
        model.meshsets.size() != 1 ||
        model.meshsets[0].corners != std::vector<uint16_t>{0, 1, 2, 2, 1, 3} ||
        model.points[11] != 11.0F) {
        return 6;
    }
    /* its converted form reads back as the backend reads it */
    std::vector<uint8_t> converted;
    pc::MeshView view;
    pc::mesh_bytes(model, converted);
    if (!pc::mesh_view(converted.data(),
                       static_cast<uint32_t>(converted.size()), view) ||
        view.count != model.points.size() / 3 || view.meshsets != 1 ||
        view.triangles != 2 || view.points[11] != 11.0F) {
        return 20;
    }
    /* a meshset's uvs hold a signed u and v for each of its strip's points.
     * a corner at the point takes them over 255. here they are the file's
     * last 16 bytes, and the strip's kth point is the model's kth */
    pc::Model mapped;
    put32(file, 108, base + 144);
    for (uint32_t k = 0; k < 4; k++) {
        put_uv(file, 144, k);
    }
    if (!pc::ninja_model(file, base, 0, mapped, why) ||
        mapped.meshsets[0].corners != model.meshsets[0].corners ||
        mapped.meshsets[0].uv.size() != 12) {
        return 42;
    }
    for (uint32_t c = 0; c < 6; c++) {
        if (!uv_is(&mapped.meshsets[0].uv[c * 2],
                   mapped.meshsets[0].corners[c])) {
            return 42;
        }
    }
    /* a uv a corner reads outside the file fails the model as one that runs
     * past it. here the last point's uv alone passes the end, then all pass
     * it, then all precede the start */
    for (uint32_t uvs : {base + 148, base + 160, base - 16}) {
        pc::Model none;
        put32(file, 108, uvs);
        why.clear();
        if (pc::ninja_model(file, base, 0, none, why) ||
            why != "a model that runs past its file") {
            return 43;
        }
    }
    /* a model whose meshsets precede each meshset's polygons and uvs, in
     * turn. here two strips of 3 take 6 uvs. a last meshset of no
     * polygons points both its pointers at the file's end. a strip's uvs
     * follow those of the strip before it. the second strip's corners thus
     * take uvs 3 to 5, whatever their points. the last meshset reads through
     * neither pointer, so the model is taken and converts */
    std::vector<uint8_t> laid(176, 0);
    pc::Model ended;
    const uint16_t strips[8] = {3, 0, 1, 2, 3, 1, 2, 3};
    put32(laid, 0, base + 40); /* points */
    put32(laid, 8, 4);
    put32(laid, 12, base + 88); /* meshsets */
    put16(laid, 20, 2);
    put16(laid, 22, 2);
    put16(laid, 88, 0xc000);
    put16(laid, 90, 2);
    put32(laid, 92, base + 136); /* meshes */
    put32(laid, 108, base + 152); /* uvs */
    for (uint32_t i = 0; i < 8; i++) {
        put16(laid, 136 + i * 2, strips[i]);
    }
    for (uint32_t k = 0; k < 6; k++) {
        put_uv(laid, 152, k);
    }
    put16(laid, 112, 0xc001);
    put32(laid, 116, base + 176);
    put32(laid, 132, base + 176);
    if (!pc::ninja_model(laid, base, 0, ended, why) ||
        ended.meshsets.size() != 2 ||
        ended.meshsets[0].corners != std::vector<uint16_t>{0, 1, 2, 1, 2, 3} ||
        ended.meshsets[0].uv.size() != 12 || ended.meshsets[1].material != 1 ||
        !ended.meshsets[1].corners.empty() || !ended.meshsets[1].uv.empty()) {
        return 44;
    }
    for (uint32_t c = 0; c < 6; c++) {
        if (!uv_is(&ended.meshsets[0].uv[c * 2], c)) {
            return 44;
        }
    }
    pc::mesh_bytes(ended, converted);
    if (!pc::mesh_view(converted.data(),
                       static_cast<uint32_t>(converted.size()), view) ||
        view.meshsets != 2 || view.triangles != 2 || view.materials != 2) {
        return 45;
    }
    /* with one polygon the last meshset reads it, past the file */
    put16(laid, 114, 1);
    why.clear();
    if (pc::ninja_model(laid, base, 0, ended, why) ||
        why != "a model that runs past its file") {
        return 46;
    }
    /* a face turned from the light has a dot above 0. SA_SHADE_RAMP stays at
     * ambient there, and SA_SHADE_LINEAR falls below it */
    sa_shade shade{};
    shade.ambient = 0.25F;
    shade.diffuse = 0.5F;
    shade.mode = SA_SHADE_RAMP;
    if (pc::ninja_intensity(shade, -1.0F) != 0.75F ||
        pc::ninja_intensity(shade, 1.0F) != 0.25F) {
        return 7;
    }
    shade.mode = SA_SHADE_LINEAR;
    if (pc::ninja_intensity(shade, 1.0F) != -0.25F) {
        return 8;
    }
    /* an intensity past 0 to 1 is held, and alpha never moves */
    if (pc::ninja_scaled(0x80ff4000, 2.0F) != 0x80ff4000 ||
        pc::ninja_scaled(0x80ff4000, -1.0F) != 0x80000000) {
        return 9;
    }
    /* the full drawer's light of njLtSrcs.c's type 0x13: ambient, diffuse
     * and a highlight */
    sa_lights lights{};
    sa_light &light = lights.light[0];
    sa_material material{};
    const float at[3] = {0.0F, 0.0F, 0.0F};
    const float facing[3] = {0.0F, 0.0F, 1.0F};
    const float away[3] = {0.0F, 0.0F, -1.0F};
    const float slanted[3] = {0.0F, 0.6F, 0.8F};
    uint32_t lit = 0;
    uint32_t offset = 0;

    lights.count = 1;
    light.type =
        SA_LIGHT_AMBIENT | SA_LIGHT_DIRECTION | SA_LIGHT_DIRECTION_SPECULAR;
    light.direction[2] = -1.0F;
    for (int k = 0; k < 3; k++) {
        light.ambient[k] = 0.2F;
        light.diffuse[k] = 0.4F;
        light.specular[k + 1] = 0.8F;
    }
    light.specular[0] = 0.5F;
    light.inner = 0.9F;
    light.outer = 0.5F;
    material.diffuse = 0x80404040;
    material.specular = 0x00404040;
    material.exponent = 2; /* the power 1 */
    if (!pc::ninja_light_known(light.type) ||
        pc::ninja_light_known(SA_LIGHT_DIRECTION |
                              SA_LIGHT_DIRECTION_SPECULAR) ||
        !pc::ninja_light_known(SA_LIGHT_SIMPLE | SA_LIGHT_POINT) ||
        pc::ninja_light_known(0x08 /* Ninja's spot light */)) {
        return 21;
    }
    /* red, green and blue each take 0.25 times the sum of 0.2 and 0.4. the
     * highlight adds 0.25 times 0.8. alpha is the material's 0.5 plus the
     * highlight's 0.5 */
    pc::ninja_lit(lights, material, at, facing, lit, offset);
    if (lit != 0xff595959 || offset != 0) {
        return 22;
    }
    /* a textured material takes the highlight as its offset color */
    material.flags = SA_MAT_TEXTURE;
    pc::ninja_lit(lights, material, at, facing, lit, offset);
    if (lit != 0xff262626 || offset != 0x00333333) {
        return 23;
    }
    /* from behind, the light gives ambient alone. a dot of -0.8 squares to
     * 0.64, between outer and inner. the highlight there is 0.64 / 0.9 of
     * itself */
    pc::ninja_lit(lights, material, at, away, lit, offset);
    if (lit != 0x800c0c0c || offset != 0) {
        return 24;
    }
    pc::ninja_lit(lights, material, at, slanted, lit, offset);
    if (offset != 0x00242424) {
        return 25;
    }
    /* an inner of 0.4 to the power 128 is too small for a float. a dot of
     * -0.6, whose square is short of inner, takes 0.9 to that power of the
     * highlight. that packs as 0 */
    const float steep[3] = {0.0F, 0.8F, 0.6F};
    sa_material sharp = material;

    light.inner = 0.4F;
    light.outer = 0.1F;
    sharp.exponent = 16;
    pc::ninja_lit(lights, sharp, at, steep, lit, offset);
    if (offset != 0) {
        return 28;
    }
    /* a point light 2 away, whose near 1 and far 9 are squares. the facing
     * point takes diffuse times 1 / 4, near over the distance's square. once
     * far is 3, it takes nothing */
    lights.light[0] = sa_light{};
    light.type = SA_LIGHT_POINT;
    light.point[2] = 2.0F;
    for (int k = 0; k < 3; k++) {
        light.diffuse[k] = 1.0F;
    }
    light.near = 1.0F;
    light.far = 9.0F;
    material.flags = 0;
    material.diffuse = 0xff808080;
    pc::ninja_lit(lights, material, at, facing, lit, offset);
    if (lit != 0xff202020) {
        return 26;
    }
    light.far = 3.0F;
    pc::ninja_lit(lights, material, at, facing, lit, offset);
    if (lit != 0xff000000) {
        return 27;
    }
    /* triangle 2's points in each form, as sa_engine.h numbers them, and the
     * points 3 triangles take */
    uint32_t corners[3];
    pc::triangle_corners(SA_TRIANGLES_STRIP, 2, corners);
    if (corners[0] != 2 || corners[2] != 4 ||
        sa_triangles_points(SA_TRIANGLES_STRIP, 3) != 5) {
        return 10;
    }
    pc::triangle_corners(SA_TRIANGLES_FAN, 2, corners);
    if (corners[0] != 0 || corners[1] != 5 || corners[2] != 6 ||
        sa_triangles_points(SA_TRIANGLES_FAN, 3) != 7) {
        return 11;
    }
    pc::triangle_corners(SA_TRIANGLES_LIST, 2, corners);
    if (corners[0] != 6 || corners[2] != 8 ||
        sa_triangles_points(SA_TRIANGLES_LIST, 3) != 9) {
        return 12;
    }
    return 0;
}

bool write_file(const std::string &path, const std::vector<uint8_t> &bytes)
{
    FILE *file = fopen(path.c_str(), "wb");

    if (file == nullptr) {
        return false;
    }
    bool wrote = fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size();

    return fclose(file) == 0 && wrote;
}

/* make a directory under TMPDIR or /tmp. it returns its path, or "" where
 * none can be made */
std::string temporary()
{
    const char *tmp = getenv("TMPDIR");
    std::string made =
        std::string(tmp != nullptr && *tmp != '\0' ? tmp : "/tmp") +
        "/replay.XXXXXX";

    return mkdtemp(&made[0]) != nullptr ? made : "";
}

/* a disc of two roots. its list sorts each root's files by name, whatever the
 * host's order, and the first root's come first. a path both roots hold is
 * listed once and read from the first. an empty file reads as no bytes */
int roots()
{
    const char *const names[2][8] = {
        {"H.PVM", "D.PVM", "B.PVM", "G.PVM", "E.PVR", "C.PVM", "EMPTY.BIN"},
        {"F.PVM", "A.PVM", "C.PVM"}};
    const std::vector<std::string> ordered = {
        "DIR/B.PVM", "DIR/C.PVM", "DIR/D.PVM", "DIR/G.PVM",
        "DIR/H.PVM", "DIR/A.PVM", "DIR/F.PVM"};
    std::string made = temporary();
    pc::Disc disc;
    std::vector<uint8_t> bytes;
    int failed = 0;

    if (made.empty()) {
        return 50;
    }
    for (int root = 0; root < 2; root++) {
        std::string dir = made + "/" + std::to_string(root);

        disc.add_root(dir);
        if (mkdir(dir.c_str(), 0700) != 0 ||
            mkdir((dir + "/DIR").c_str(), 0700) != 0) {
            failed = 50;
        }
        for (const char *name : names[root]) {
            if (name == nullptr) {
                continue;
            }
            /* each file but EMPTY.BIN holds its root's number */
            std::vector<uint8_t> held;

            if (strcmp(name, "EMPTY.BIN") != 0) {
                held.push_back(static_cast<uint8_t>('0' + root));
            }
            if (!write_file(dir + "/DIR/" + name, held)) {
                failed = 50;
            }
        }
    }
    if (failed == 0 && disc.list("DIR", ".PVM") != ordered) {
        failed = 51;
    }
    if (failed == 0 &&
        (!disc.read("DIR/C.PVM", bytes) || bytes != std::vector<uint8_t>{'0'} ||
         !disc.read("DIR/A.PVM", bytes) || bytes != std::vector<uint8_t>{'1'} ||
         !disc.read("DIR/EMPTY.BIN", bytes) || !bytes.empty() ||
         disc.read("DIR/NONE.PVM", bytes))) {
        failed = 52;
    }
    for (int root = 0; root < 2; root++) {
        std::string dir = made + "/" + std::to_string(root);

        for (const char *name : names[root]) {
            if (name != nullptr) {
                unlink((dir + "/DIR/" + name).c_str());
            }
        }
        rmdir((dir + "/DIR").c_str());
        rmdir(dir.c_str());
    }
    rmdir(made.c_str());
    return failed;
}

/* a PVM of one 4 by 4 texture whose texel bytes all hold texel, so 0xff is
 * white. its entry holds the fields status sets, as pvr.cpp's PVMH_* bits
 * name them. a chunk of a tag no reader knows comes before the texture's,
 * and its bytes spell PVRT */
std::vector<uint8_t> archive(uint32_t status, const char *name, uint32_t index,
                             uint8_t texel)
{
    const uint8_t other[16] = {'N', 'O', 'N', 'E', 8, 0, 0, 0,
                               'P', 'V', 'R', 'T', 8, 0, 0, 0};
    std::vector<uint8_t> texture = pvrt(2, 0x01, 4, 4, 32);
    std::vector<uint8_t> b(14, 0);

    memcpy(b.data(), "PVMH", 4);
    put16(b, 8, status);
    put16(b, 10, 1);
    if (status & 0x1) {
        b.resize(b.size() + 28, 0);
        memcpy(&b[b.size() - 28], name, strlen(name));
    }
    b.resize(b.size() + (status & 0x2 ? 2 : 0) + (status & 0x4 ? 2 : 0), 0xff);
    if (status & 0x8) {
        b.resize(b.size() + 4, 0);
        put32(b, static_cast<uint32_t>(b.size() - 4), index);
    }
    put32(b, 4, static_cast<uint32_t>(b.size() - 8));
    b.insert(b.end(), other, other + sizeof other);
    memset(&texture[16], texel, 32);
    b.insert(b.end(), texture.begin(), texture.end());
    return b;
}

/* the index reads a PVM's entries as Ninja's loader does, whichever fields
 * their status sets. one entry holds only a global index, one only a name,
 * and one every field. each entry's texture loads. a chunk of another tag is
 * passed by its length, whatever its bytes spell */
int archives()
{
    struct {
        const char *file;
        uint32_t status;
        const char *name;
        uint32_t index;
    } const each[3] = {{"INDEX.PVM", 0x8, "", 77},
                       {"NAME.PVM", 0x1, "only", 0},
                       {"WHOLE.PVM", 0xf, "whole", 78}};
    std::string made = temporary();
    pc::Disc disc;
    pc::Textures textures;
    int failed = 0;

    if (made.empty() || mkdir((made + "/DIR").c_str(), 0700) != 0) {
        return 53;
    }
    for (const auto &e : each) {
        if (!write_file(made + "/DIR/" + e.file,
                        archive(e.status, e.name, e.index, 0xff))) {
            failed = 53;
        }
    }
    disc.add_root(made);
    textures.index(disc, "DIR");
    if (failed == 0 && textures.count() != 3) {
        failed = 54;
    }
    for (const auto &e : each) {
        pc::Image image;
        std::string why;

        if (failed == 0 &&
            (!textures.load(disc, e.index, e.name, 0, image, why) ||
             image.width != 4 || image.rgba != std::vector<uint8_t>(64, 255))) {
            failed = 55;
        }
        unlink((made + "/DIR/" + e.file).c_str());
    }
    rmdir((made + "/DIR").c_str());
    rmdir(made.c_str());
    return failed;
}

/* two files hold a texture under one global index and name, and their
 * texels differ. a load that names no file takes the copy the disc lists
 * first, though B.PVM was written first. a load that names a file takes that
 * file's copy. it matches the index, or the name where the index is 0 or no
 * file holds it. where that file lacks the texture, the load fails and why
 * names the file's path. for a file the index lacks, why names its id. a
 * file's name id is sa_name_id of its path in upper case */
int twins()
{
    const std::vector<uint8_t> white(64, 255);
    const std::vector<uint8_t> clear(64, 0);
    const uint32_t first = pc::pvr_file_id("DIR/A.PVM");
    const uint32_t second = pc::pvr_file_id("dir/b.pvm");
    const uint32_t third = pc::pvr_file_id("DIR/C.PVM");
    const uint32_t absent = pc::pvr_file_id("DIR/D.PVM");
    std::string made = temporary();
    pc::Disc disc;
    pc::Textures textures;
    pc::Image image;
    std::string why;
    int failed = 0;

    if (made.empty() || mkdir((made + "/DIR").c_str(), 0700) != 0) {
        return 56;
    }
    if (!write_file(made + "/DIR/B.PVM", archive(0xf, "twin", 79, 0x00)) ||
        !write_file(made + "/DIR/A.PVM", archive(0xf, "twin", 79, 0xff)) ||
        !write_file(made + "/DIR/C.PVM", archive(0xf, "other", 80, 0xff))) {
        failed = 56;
    }
    disc.add_root(made);
    textures.index(disc, "DIR");
    if (failed == 0 && (textures.count() != 3 ||
                        !textures.load(disc, 79, "twin", 0, image, why) ||
                        image.rgba != white)) {
        failed = 57;
    }
    if (failed == 0 && (second != sa_name_id("DIR/B.PVM") || first == second ||
                        first == 0 || second == 0)) {
        failed = 58;
    }
    if (failed == 0 && (!textures.load(disc, 79, "twin", second, image, why) ||
                        image.rgba != clear ||
                        !textures.load(disc, 79, "twin", first, image, why) ||
                        image.rgba != white ||
                        !textures.load(disc, 0, "twin", second, image, why) ||
                        image.rgba != clear ||
                        !textures.load(disc, 999, "twin", second, image, why) ||
                        image.rgba != clear)) {
        failed = 59;
    }
    if (failed == 0 && (textures.load(disc, 79, "twin", third, image, why) ||
                        why.find("DIR/C.PVM") == std::string::npos ||
                        textures.load(disc, 0, "twin", third, image, why) ||
                        textures.load(disc, 999, "twin", third, image, why) ||
                        textures.load(disc, 80, "other", second, image, why) ||
                        why.find("DIR/B.PVM") == std::string::npos ||
                        textures.load(disc, 79, "twin", absent, image, why) ||
                        why.find(std::to_string(absent)) == std::string::npos ||
                        why.find("DIR/") != std::string::npos ||
                        !textures.load(disc, 80, "other", third, image, why))) {
        failed = 60;
    }
    unlink((made + "/DIR/A.PVM").c_str());
    unlink((made + "/DIR/B.PVM").c_str());
    unlink((made + "/DIR/C.PVM").c_str());
    rmdir((made + "/DIR").c_str());
    rmdir(made.c_str());
    return failed;
}

} // namespace

int main()
{
    int failed = check();

    if (failed == 0) {
        failed = roots();
    }
    if (failed == 0) {
        failed = archives();
    }
    if (failed == 0) {
        failed = twins();
    }
    if (failed != 0) {
        fprintf(stderr, "replay: check %d failed\n", failed);
    }
    return failed;
}
