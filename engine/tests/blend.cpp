/* the blend list's resolve, engine/pc/shaders/resolve.comp, run on the
 * host with no device. resolve.h is the shader's text less its #version
 * line. this file includes it as C++ over GLSL's types, as far as the shader
 * uses them. engine-test runs it. main returns 0, or says the line of the
 * first check that fails and returns 1 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <vector>

#include "sa_pc.h"

namespace glsl {

using uint = uint32_t;

struct vec4 {
    float r, g, b, a;
    vec4() : r(0), g(0), b(0), a(0) {}
    vec4(float all) : r(all), g(all), b(all), a(all) {}
    vec4(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
};
struct uvec2 {
    uint x, y;
};
struct ivec2 {
    int x, y;
    ivec2(int x, int y) : x(x), y(y) {}
    explicit ivec2(uvec2 from)
        : x(static_cast<int>(from.x)), y(static_cast<int>(from.y))
    {
    }
};
struct bvec2 {
    bool x, y;
};
struct uvec4 {
    uint r, g, b, a;
};
/* an image is its texels by rows: a word each, or four bytes, RGBA */
struct uimage2D {
    int width, height;
    uint *texels;
};
struct image2D {
    int width, height;
    uint8_t *texels;
};

vec4 operator+(vec4 x, vec4 y)
{
    return {x.r + y.r, x.g + y.g, x.b + y.b, x.a + y.a};
}

vec4 operator-(vec4 x, vec4 y)
{
    return {x.r - y.r, x.g - y.g, x.b - y.b, x.a - y.a};
}

vec4 operator*(vec4 x, vec4 y)
{
    return {x.r * y.r, x.g * y.g, x.b * y.b, x.a * y.a};
}

vec4 operator/(vec4 x, float y)
{
    return {x.r / y, x.g / y, x.b / y, x.a / y};
}

float clamp(float x, float low, float high)
{
    return x < low ? low : x > high ? high : x;
}

vec4 clamp(vec4 x, float low, float high)
{
    return {clamp(x.r, low, high), clamp(x.g, low, high), clamp(x.b, low, high),
            clamp(x.a, low, high)};
}

bvec2 greaterThanEqual(ivec2 a, ivec2 b)
{
    return {a.x >= b.x, a.y >= b.y};
}

bool any(bvec2 v)
{
    return v.x || v.y;
}

ivec2 imageSize(const image2D &image)
{
    return {image.width, image.height};
}

uvec4 imageLoad(const uimage2D &image, ivec2 at)
{
    return {image.texels[at.y * image.width + at.x], 0, 0, 0};
}

vec4 imageLoad(const image2D &image, ivec2 at)
{
    const uint8_t *t = &image.texels[(at.y * image.width + at.x) * 4];

    return vec4(t[0], t[1], t[2], t[3]) / 255.0F;
}

/* a store rounds each channel to the nearest of its 256 steps */
void imageStore(image2D &image, ivec2 at, vec4 color)
{
    uint8_t *t = &image.texels[(at.y * image.width + at.x) * 4];
    const float channels[4] = {color.r, color.g, color.b, color.a};

    for (int i = 0; i < 4; i++) {
        t[i] = static_cast<uint8_t>(clamp(channels[i], 0.0F, 1.0F) * 255.0F +
                                    0.5F);
    }
}

uint atomicAdd(uint &to, uint more)
{
    uint was = to;

    to += more;
    return was;
}

struct {
    uvec2 xy;
} gl_GlobalInvocationID;

/* GLSL's words that C++ lacks, as far as resolve.h uses them. a layout
 * reads as nothing, and a uniform reads as a plain variable. the buffer's
 * block is a struct, and list is what lists points at. the local size's line
 * declares an unused struct, and main is resolve. the host's C++ warns of
 * the block's unsized last array and of the brackets around its name */
#define layout(...)
#define uniform
#define buffer struct
#define in struct in_
#define main resolve
#define list (*lists)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wparentheses"
#include "resolve.h"
#pragma GCC diagnostic pop
#undef layout
#undef uniform
#undef buffer
#undef in
#undef main
#undef list

static_assert(MOST == SA_PC_PIXEL_FRAGMENTS,
              "the resolve composes as many of a pixel's fragments as "
              "sa_pc.h says");

} // namespace glsl

#define CHECK(test)                                                            \
    do {                                                                       \
        if (!(test)) {                                                         \
            return __LINE__;                                                   \
        }                                                                      \
    } while (0)

namespace {

/* a fragment as a test gives it. color is RGBA from the low byte, and
 * depth is 1/z. src and dst are its blend's instructions, and triangle is
 * its triangle's place in the order drawn */
struct Given {
    uint32_t color;
    float depth;
    uint32_t src;
    uint32_t dst;
    uint32_t triangle;
};

/* the PowerVR's blend instructions the tests use */
constexpr uint32_t ZERO = SA_BLEND_ZERO;
constexpr uint32_t ONE = SA_BLEND_ONE;
constexpr uint32_t OTHER = SA_BLEND_OTHER;
constexpr uint32_t INV_OTHER = SA_BLEND_INV_OTHER;
constexpr uint32_t SRC_ALPHA = SA_BLEND_SRC_ALPHA;
constexpr uint32_t INV_SRC_ALPHA = SA_BLEND_INV_SRC_ALPHA;
constexpr uint32_t DST_ALPHA = SA_BLEND_DST_ALPHA;
constexpr uint32_t INV_DST_ALPHA = SA_BLEND_INV_DST_ALPHA;

constexpr uint32_t END = 0xffffffffU;
constexpr uint32_t RED = 0x000000ffU;
constexpr uint32_t GREEN = 0x0000ff00U;
constexpr uint32_t BLUE = 0x00ff0000U;

/* how many fragments the last picture's resolve cut */
uint32_t cut;

/* resolve a one-row picture over back, linking each pixel's fragments in
 * the order given. it returns the pixels after, each RGBA from the low byte */
std::vector<uint32_t> resolve(const std::vector<std::vector<Given>> &pixels,
                              uint32_t back)
{
    size_t count = 0;

    for (const std::vector<Given> &given : pixels) {
        count += given.size();
    }
    auto width = static_cast<int>(pixels.size());
    std::vector<uint32_t> firsts(pixels.size(), END);
    std::vector<uint8_t> target(pixels.size() * 4);
    std::vector<uint32_t> after(pixels.size());
    auto *list = static_cast<glsl::Fragments *>(
        calloc(1, sizeof(glsl::Fragments) + count * sizeof(glsl::Fragment)));
    uint32_t at = 0;

    for (size_t x = 0; x < pixels.size(); x++) {
        memcpy(&target[x * 4], &back, 4);
        for (size_t i = 0; i < pixels[x].size(); i++) {
            const Given &given = pixels[x][i];

            (i == 0 ? firsts[x] : list->fragments[at - 1].next) = at;
            list->fragments[at] = {given.color, given.depth, END,
                                   given.src | given.dst << 3 |
                                       given.triangle << 6};
            at++;
        }
    }
    list->count = at;
    list->capacity = at;
    glsl::heads = {width, 1, firsts.data()};
    glsl::target = {width, 1, target.data()};
    glsl::lists = list;
    /* the last three calls run past the target's edge. a group 8 wide does so
     * where the width is no multiple of 8 */
    for (uint32_t x = 0; x < pixels.size() + 3; x++) {
        glsl::gl_GlobalInvocationID.xy = {x, 0};
        glsl::resolve();
    }
    cut = list->cut;
    free(list);
    memcpy(after.data(), target.data(), target.size());
    return after;
}

uint32_t resolve(const std::vector<Given> &given, uint32_t back)
{
    return resolve(std::vector<std::vector<Given>>{given}, back)[0];
}

/* three orders the list could link given's fragments in: as given,
 * reversed, and by a step of 37. the step reaches each fragment once while
 * their count is no multiple of 37 */
std::vector<std::vector<Given>> orders(const std::vector<Given> &given)
{
    std::vector<Given> turned(given.rbegin(), given.rend());
    std::vector<Given> dealt;

    for (size_t i = 0; i < given.size(); i++) {
        dealt.push_back(given[(i * 37 + 11) % given.size()]);
    }
    return {given, turned, dealt};
}

/* a pixel blends each fragment over what lies behind it by its
 * instructions. it composes them from the farthest, ties in the triangles'
 * order, however the list links them */
int composed()
{
    /* opaque, so the last composed is the pixel: the nearest */
    for (const std::vector<Given> &given :
         orders({{RED, 0.1F, ONE, ZERO, 0},
                 {GREEN, 0.3F, ONE, ZERO, 1},
                 {BLUE, 0.2F, ONE, ZERO, 2}})) {
        CHECK(resolve(given, 0xff000000U) == GREEN && cut == 0);
    }
    /* at one depth, the later triangle */
    for (const std::vector<Given> &given :
         orders({{RED, 0.5F, ONE, ZERO, 9},
                 {GREEN, 0.5F, ONE, ZERO, 5},
                 {BLUE, 0.5F, ONE, ZERO, 7}})) {
        CHECK(resolve(given, 0) == RED && cut == 0);
    }
    /* added, and held to 1 */
    CHECK(resolve({{0x0a, 0.1F, ONE, ONE, 0},
                   {0x14, 0.2F, ONE, ONE, 1},
                   {0x1e, 0.3F, ONE, ONE, 2}},
                  0x00030201U) == 0x0003023dU);
    CHECK(resolve({{0xc8, 0.1F, ONE, ONE, 0}, {0xc8, 0.2F, ONE, ONE, 1}}, 0) ==
          0xffU);
    /* by its alpha: half of white over black, then half of red over that */
    CHECK(resolve({{0x80ffffffU, 0.1F, SRC_ALPHA, INV_SRC_ALPHA, 0}}, 0) ==
          0x40808080U);
    CHECK(resolve({{0x80ffffffU, 0.1F, SRC_ALPHA, INV_SRC_ALPHA, 0},
                   {0x800000ffU, 0.2F, SRC_ALPHA, INV_SRC_ALPHA, 1}},
                  0) == 0x604040c0U);
    /* by the other side's color, the first multiplies what lies behind it.
     * the second adds itself to what lies behind it, less its own share. the
     * third blends by the alpha behind it */
    CHECK(resolve({{0xff8040ffU, 0.1F, OTHER, ZERO, 0}}, 0xff808080U) ==
          0xff402080U);
    CHECK(resolve({{0xff0040ffU, 0.1F, ONE, INV_OTHER, 0}}, 0xff808080U) ==
          0xff80a0ffU);
    CHECK(resolve({{0xffffffffU, 0.1F, DST_ALPHA, INV_DST_ALPHA, 0}},
                  0x40000000U) == 0x70404040U);
    /* a pixel with no fragment keeps what it had */
    CHECK(resolve(std::vector<Given>{}, 0x12345678U) == 0x12345678U &&
          cut == 0);
    return 0;
}

/* a pixel composes the last SA_PC_PIXEL_FRAGMENTS of its fragments in the
 * order it composes them. each one before those is cut. a cut fragment is
 * counted, and the pixel shows without what it would have added */
int bounded()
{
    std::vector<Given> given;

    /* each adds 1 to red, and the farthest of them all green */
    for (uint32_t i = 0; i < SA_PC_PIXEL_FRAGMENTS; i++) {
        given.push_back(
            {0x01, 0.5F + static_cast<float>(i) * 0.001F, ONE, ONE, i + 1});
    }
    for (const std::vector<Given> &linked : orders(given)) {
        CHECK(resolve(linked, 0) == SA_PC_PIXEL_FRAGMENTS && cut == 0);
    }
    given.push_back({GREEN, 0.25F, ONE, ONE, 0});
    for (const std::vector<Given> &linked : orders(given)) {
        CHECK(resolve(linked, 0) == SA_PC_PIXEL_FRAGMENTS && cut == 1);
    }
    /* and the nearest of them all blue, which takes the place of the
     * farthest still kept */
    given.push_back({BLUE, 0.75F, ONE, ONE, 99});
    for (const std::vector<Given> &linked : orders(given)) {
        CHECK(resolve(linked, 0) == (BLUE | (SA_PC_PIXEL_FRAGMENTS - 1)) &&
              cut == 2);
    }
    /* at one depth the triangles' order decides which come last. with six
     * past the bound, the six drawn first are cut however the list links
     * them */
    given.clear();
    for (uint32_t i = 0; i < SA_PC_PIXEL_FRAGMENTS + 6; i++) {
        given.push_back({i < 6 ? 0x1000U : 0x01U, 0.5F, ONE, ONE, i});
    }
    for (const std::vector<Given> &linked : orders(given)) {
        CHECK(resolve(linked, 0) == SA_PC_PIXEL_FRAGMENTS && cut == 6);
    }
    /* a hundred fragments blend by alpha at depths in no order. the pixel is
     * what their last SA_PC_PIXEL_FRAGMENTS compose alone */
    given.clear();
    for (uint32_t i = 0; i < 100; i++) {
        uint32_t word = i * 2654435761U;

        given.push_back({0x60000000U | (word >> 8),
                         static_cast<float>(word % 23) / 23.0F, SRC_ALPHA,
                         INV_SRC_ALPHA, i});
    }
    std::vector<Given> last = given;

    std::stable_sort(
        last.begin(), last.end(),
        [](const Given &a, const Given &b) { return a.depth < b.depth; });
    last.erase(last.begin(), last.end() - SA_PC_PIXEL_FRAGMENTS);
    uint32_t pixel = resolve(last, 0xff204060U);

    CHECK(cut == 0);
    for (const std::vector<Given> &linked : orders(given)) {
        CHECK(resolve(linked, 0xff204060U) == pixel &&
              cut == 100 - SA_PC_PIXEL_FRAGMENTS);
    }
    /* each pixel past the bound adds what it cut to the picture's count */
    given.resize(SA_PC_PIXEL_FRAGMENTS + 5);
    CHECK(resolve({given, {}, given, {{RED, 0.5F, ONE, ZERO, 0}}}, 0)[3] ==
              RED &&
          cut == 10);
    return 0;
}

} // namespace

int main()
{
    int line = composed();

    if (line == 0) {
        line = bounded();
    }
    if (line != 0) {
        fprintf(stderr, "blend: line %d\n", line);
    }
    return line != 0;
}
