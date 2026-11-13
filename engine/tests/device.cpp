/* the PC's raster over a device of the test's own, vulkan.h's, so the test
 * needs no Vulkan. the raster opens the test's library where it would open
 * Vulkan's loader. engine-test runs it. main returns 0, or says the line of
 * the first check that fails and returns 1 */
#include <dlfcn.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "raster.h"
#include "vulkan.h"

namespace pc {

/* the window, as the raster asks of one: its size, which the test sets */
struct Window {
    uint32_t width;
    uint32_t height;
};

const char *const *window_extensions(uint32_t &count)
{
    static const char *const names[] = {"VK_KHR_surface"};

    count = 1;
    return names;
}

bool window_surface(Window *, void *, uint64_t &surface)
{
    static char one;

    surface = reinterpret_cast<uint64_t>(&one);
    return true;
}

void window_size(Window *window, uint32_t &width, uint32_t &height)
{
    width = window->width;
    height = window->height;
}

} // namespace pc

#define CHECK(test)                                                            \
    do {                                                                       \
        if (!(test)) {                                                         \
            return __LINE__;                                                   \
        }                                                                      \
    } while (0)

namespace {

TestDevice *device;

void reset()
{
    device->fail_name = nullptr;
    device->fail_nth = 0;
    device->fail_run = 1;
    device->fail_result = VK_ERROR_OUT_OF_DEVICE_MEMORY;
    device->max_bias = 4.0F;
    device->names.clear();
    device->lacks = nullptr;
    device->calls.clear();
    device->failed = nullptr;
    device->faults = 0;
    device->fault[0] = 0;
    device->samplers.clear();
    device->draws.clear();
    device->shaded = nullptr;
    device->resolved = nullptr;
}

bool among(const char *call, std::initializer_list<const char *> names)
{
    for (const char *name : names) {
        if (strcmp(call, name) == 0) {
            return true;
        }
    }
    return false;
}

/* the calls whose failure a step survives. a picture is drawn without the
 * timer, and a wait is made again. a fence's status is asked again later.
 * a surface that won't list its present modes still has FIFO */
bool tolerated(const char *call)
{
    return among(call, {"vkCreateQueryPool", "vkGetQueryPoolResults",
                        "vkWaitForFences", "vkGetFenceStatus",
                        "vkGetPhysicalDeviceSurfacePresentModesKHR"});
}

/* the calls that hand the device a load's or a picture's commands. one
 * that fails may leave part of them on the device. the raster may then draw
 * no more. any other call fails with the raster as it was */
bool commits(const char *call)
{
    return among(call, {"vkBeginCommandBuffer", "vkEndCommandBuffer",
                        "vkResetFences", "vkQueueSubmit", "vkQueuePresentKHR"});
}

/* a run of the raster under a failing call, and the first rule it breaks */
struct Run {
    bool loss = false; /* the call fails as the device's loss */
    bool again = true; /* a step that fails is made again */
    std::string first; /* the first reason a step gave */
    bool owed = false; /* a step that says nothing met the failing call */
    bool readback = false; /* the raster keeps each picture for read */
    bool held[2] = {}; /* a picture of the frame's went to the device */
    int line = 0;
    const char *rule = "";

    void breaks(int at, const char *which)
    {
        if (line == 0) {
            line = at;
            rule = which;
        }
    }

    /* a step that says whether it took, and why not. a failed step is made
     * once more, unless the device is lost or can or again is false */
    template <typename Step> bool step(int at, bool can, Step make)
    {
        const char *before = device->failed;
        std::string why;
        bool ok = make(why);
        bool struck = before == nullptr && device->failed != nullptr;
        bool must = owed || (struck && !tolerated(device->failed)) ||
                    (loss && before != nullptr);

        owed = false;
        if (ok) {
            if (must) {
                breaks(at, "a step took though a call it needs failed");
            }
            return true;
        }
        if (device->failed == nullptr) {
            breaks(at, "a step failed though no call did");
            return false;
        }
        if (why.empty()) {
            breaks(at, "a step failed and gave no reason");
        }
        if (!first.empty() && why != first) {
            breaks(at, "a later step gives another reason than the first");
        }
        if (first.empty() && !loss && struck &&
            why.find(device->failed) == std::string::npos) {
            breaks(at, "the reason doesn't name the call that failed");
        }
        if (first.empty()) {
            first = why;
        }
        if (loss || !can || !again) {
            return false;
        }
        why.clear();
        if (make(why)) {
            return true;
        }
        if (!commits(device->failed)) {
            breaks(at, "a call that makes a thing failed once, and the step "
                       "fails again");
        } else if (why != first) {
            breaks(at, "a step made again gives another reason");
        }
        return false;
    }

    /* a step that says nothing */
    template <typename Step> void quiet(Step make)
    {
        const char *before = device->failed;

        make();
        if (before == nullptr && device->failed != nullptr &&
            !tolerated(device->failed)) {
            owed = true;
        }
    }
};

/* the device's shaders for a run. every picture's draws and resolve leave
 * counts, so counts of 0 show that a frame gave no picture */
void count()
{
    device->shaded = [](uint32_t *head) { head[2] += 5; };
    device->resolved = [](uint32_t *head) { head[3] += 7; };
}

pc::Image image_of(uint32_t width, uint32_t height)
{
    pc::Image image;

    image.width = width;
    image.height = height;
    image.rgba.assign(size_t{width} * height * 4, 128);
    return image;
}

/* a texture, or 0, white, where the device refuses one */
uint32_t texture(Run &run, int at, pc::Raster &raster, const pc::Image &image)
{
    uint32_t made = 0;

    run.step(at, true, [&](std::string &why) {
        made = raster.add_texture(image, why);
        return made != 0;
    });
    return made;
}

/* a material's usual sample: bilinear, at a D adjust of 1.0 */
constexpr uint32_t BILINEAR = pc::sample_of(SA_FILTER_BILINEAR, pc::ADJUST_ONE);

/* a batch of count triangles from first, of a texture under a sample,
 * blended or not */
pc::Batch batch(uint32_t first, uint32_t count, uint32_t texture,
                uint32_t sample, bool blend)
{
    pc::Batch made;

    made.state.flags =
        (texture != 0 ? pc::BATCH_TEXTURE : 0) | (blend ? 0 : pc::BATCH_OPAQUE);
    made.state.texture = texture;
    made.state.sample = sample;
    made.state.blend = blend ? 1 : 0;
    made.state.src = blend ? 4 : 1;
    made.state.dst = blend ? 5 : 0;
    made.first = first * 3;
    made.count = count * 3;
    return made;
}

bool submit(Run &run, int at, pc::Raster &raster, uint32_t frame,
            uint32_t triangles, const std::vector<pc::Batch> &batches,
            bool sorted)
{
    std::vector<pc::Vertex> vertices(triangles * 3);
    pc::PictureUniform picture{};
    bool sent = run.step(at, true, [&](std::string &why) {
        return raster.submit(frame, picture, 0xff102030, vertices, batches,
                             sorted, why);
    });

    run.held[frame] = run.held[frame] || sent;
    return sent;
}

/* what a frame gives once it is waited for. it is done, whatever came of
 * its picture, unless its fence's status fails for want of memory. a frame
 * that sent the device no picture gives no picture, counts or time. no frame
 * gives them once the device is lost. a lost device may signal a fence and
 * leave what it guards undefined. with no call failed, a frame that holds a
 * picture gives it under readback */
void retire(Run &run, int at, pc::Raster &raster, uint32_t frame)
{
    static uint8_t rgba[pc::WIDTH * pc::HEIGHT * 4];

    run.quiet([&]() {
        raster.wait(frame);
        const char *before = device->failed;
        bool done = raster.done(frame);
        bool asked = before == nullptr && device->failed != nullptr;
        bool none = !run.held[frame] || (run.loss && device->failed != nullptr);
        bool read = raster.read(frame, rgba);
        uint32_t counts = raster.lost(frame) + raster.cut(frame);
        uint64_t time = raster.gpu_ns(frame);

        if (!done && (run.loss || !asked)) {
            run.breaks(at, "a frame waited for isn't done");
        }
        if (none && (read || counts != 0 || time != 0)) {
            run.breaks(at, "a frame gives a picture it doesn't hold");
        }
        if (device->failed == nullptr && run.held[frame] &&
            read != run.readback) {
            run.breaks(at, "a frame doesn't give the picture it holds");
        }
    });
}

/* a headless raster at twice the size, read back. its textures take slots,
 * two of one size at once. the huge ones take memory of their own and
 * together fill a load's staging. one texture is freed while its load
 * records. the pictures come sorted and not, and the last outgrows its
 * frame's vertex buffer */
void headless(Run &run)
{
    static const pc::Image small = image_of(8, 8);
    static const pc::Image large = image_of(512, 512);
    static const pc::Image huge = image_of(1024, 1024);
    pc::Raster raster;
    pc::RasterOptions options;

    options.frames = 2;
    options.readback = true;
    options.resolution = 2;
    run.readback = true;
    if (!run.step(__LINE__, false, [&](std::string &why) {
            return raster.open(options, why);
        })) {
        return;
    }
    uint32_t first = texture(run, __LINE__, raster, small);
    uint32_t second = texture(run, __LINE__, raster, large);

    submit(run, __LINE__, raster, 0, 3,
           {batch(0, 1, 0, 0, false),
            batch(1, 1, first, BILINEAR | pc::SAMPLE_CLAMP_U, false),
            batch(2, 1, second, BILINEAR, true)},
           true);
    retire(run, __LINE__, raster, 0);
    uint32_t filling[4];

    for (uint32_t &each : filling) {
        each = texture(run, __LINE__, raster, huge);
    }
    if (first != 0) {
        run.quiet([&]() { raster.free_texture(first); });
    }
    uint32_t third = texture(run, __LINE__, raster, small);

    if (third != 0) {
        run.quiet([&]() { raster.free_texture(third); });
    }
    third = texture(run, __LINE__, raster, small);
    uint32_t fourth = texture(run, __LINE__, raster, small);
    pc::Batch culled = batch(1, 1, filling[0], 0, false);

    culled.state.cull = 1;
    submit(run, __LINE__, raster, 1, 3,
           {batch(0, 1, third, pc::SAMPLE_FLIP_V, false), culled,
            batch(2, 1, fourth, 0, false)},
           false);
    submit(run, __LINE__, raster, 0, 4096,
           {batch(0, 4096, second, BILINEAR, true)}, true);
    retire(run, __LINE__, raster, 0);
    retire(run, __LINE__, raster, 1);
}

/* a window's raster. the window grows, shows 16:9 and shrinks to nothing.
 * it then takes two sizes under the picture's own, and grows past it. at
 * resolution 2 the picture is larger than the window shows it until the
 * last. at 0 the picture follows the window, but never under WIDTH by
 * HEIGHT. each size under that scales the picture down to an image of its
 * own */
void windowed(Run &run, uint32_t resolution)
{
    static const pc::Image small = image_of(8, 8);
    pc::Window window = {800, 600};
    pc::Raster raster;
    pc::RasterOptions options;
    auto size = [&](uint32_t width, uint32_t height) {
        window = {width, height};
        device->width = width;
        device->height = height;
    };

    size(800, 600);
    options.window = &window;
    options.frames = 2;
    options.resolution = resolution;
    if (!run.step(__LINE__, false, [&](std::string &why) {
            return raster.open(options, why);
        })) {
        return;
    }
    uint32_t first = texture(run, __LINE__, raster, small);
    std::vector<pc::Batch> batches = {batch(0, 1, 0, 0, false),
                                      batch(1, 1, first, BILINEAR, true)};

    size(1024, 768);
    submit(run, __LINE__, raster, 0, 2, batches, true);
    submit(run, __LINE__, raster, 1, 2, batches, true);
    run.quiet([&]() { raster.wide(true); });
    submit(run, __LINE__, raster, 0, 2, batches, false);
    size(0, 0);
    submit(run, __LINE__, raster, 1, 2, batches, true);
    size(400, 300);
    submit(run, __LINE__, raster, 0, 2, batches, true);
    size(500, 375);
    submit(run, __LINE__, raster, 1, 2, batches, true);
    size(2000, 1500);
    submit(run, __LINE__, raster, 0, 2, batches, true);
    retire(run, __LINE__, raster, 0);
    retire(run, __LINE__, raster, 1);
}

void fitted(Run &run)
{
    windowed(run, 2);
}

/* a refused picture isn't made again, and the window moves on. the load a
 * refused picture's fit left may name its fitted image. a later fit sends
 * that load before it ends the image */
void sized(Run &run)
{
    run.again = false;
    windowed(run, 0);
}

/* a raster opened once the last has ended draws */
void after(Run &run)
{
    static const pc::Image small = image_of(8, 8);
    pc::Raster raster;
    pc::RasterOptions options;

    if (!run.step(__LINE__, false, [&](std::string &why) {
            return raster.open(options, why);
        })) {
        return;
    }
    uint32_t first = texture(run, __LINE__, raster, small);

    submit(run, __LINE__, raster, 0, 1, {batch(0, 1, first, 0, false)}, true);
    retire(run, __LINE__, raster, 0);
}

/* what a run left wrong, as words, or null */
const char *wrong(const Run &run)
{
    static char words[512];

    if (device->faults != 0) {
        snprintf(words, sizeof words, "the device faults: %s", device->fault);
    } else if (run.line != 0) {
        snprintf(words, sizeof words, "line %d: %s", run.line, run.rule);
    } else if (device->live != 0) {
        snprintf(words, sizeof words, "%u objects outlive the raster",
                 device->live);
    } else {
        return nullptr;
    }
    return words;
}

/* run through with no call failing, then once with each of its calls
 * failing. each failing run has a process of its own, so one that dies takes
 * no other with it. it returns the count of runs left wrong */
int sweep(const char *name, void (*through)(Run &), bool loss)
{
    Run clean;
    int failures = 0;

    reset();
    count();
    through(clean);
    if (wrong(clean) != nullptr) {
        fprintf(stderr, "device: %s, no call failing: %s\n", name,
                wrong(clean));
        return 1;
    }
    std::vector<const char *> calls = device->calls;

    for (size_t k = 0; k < calls.size(); k++) {
        char which[160];
        int status = 0;

        snprintf(which, sizeof which, "device: %s, %s %s as call %zu", name,
                 calls[k], loss ? "lost" : "failing", k + 1);
        fflush(stderr);
        pid_t child = fork();

        if (child == 0) {
            Run run;
            Run fresh;

            alarm(30);
            reset();
            count();
            run.loss = loss;
            device->fail_nth = static_cast<uint32_t>(k + 1);
            device->fail_result = loss ? VK_ERROR_DEVICE_LOST
                                  : strcmp(calls[k], "vkMapMemory") == 0
                                      ? VK_ERROR_MEMORY_MAP_FAILED
                                      : VK_ERROR_OUT_OF_DEVICE_MEMORY;
            through(run);
            const char *left = wrong(run);

            if (left == nullptr && device->failed == nullptr) {
                left = "the call never came";
            }
            if (left != nullptr) {
                fprintf(stderr, "%s: %s\n", which, left);
                _exit(1);
            }
            reset();
            after(fresh);
            if (wrong(fresh) != nullptr) {
                fprintf(stderr, "%s: a raster opened after it: %s\n", which,
                        wrong(fresh));
                _exit(1);
            }
            _exit(0);
        }
        if (child < 0 || waitpid(child, &status, 0) != child) {
            fprintf(stderr, "%s: no process to run it in\n", which);
            return failures + 1;
        }
        if (WIFSIGNALED(status)) {
            fprintf(stderr, "%s: the raster dies of signal %d\n", which,
                    WTERMSIG(status));
        }
        failures += !WIFEXITED(status) || WEXITSTATUS(status) != 0;
    }
    return failures;
}

/* how often a call was made */
uint32_t made(const char *call)
{
    uint32_t count = 0;

    for (const char *each : device->calls) {
        count += strcmp(each, call) == 0;
    }
    return count;
}

/* a swapchain the window has outgrown is no failure. the picture is drawn,
 * a new swapchain takes the old one's place, and the next picture shows. one
 * that only fits less well shows the image it gave */
int outgrown()
{
    pc::Window window = {800, 600};
    pc::Raster raster;
    pc::RasterOptions options;
    pc::PictureUniform picture{};
    std::vector<pc::Vertex> vertices(3);
    std::vector<pc::Batch> batches = {batch(0, 1, 0, 0, false)};
    std::string why;
    auto shows = [&](const char *call, VkResult result, uint32_t presents,
                     uint32_t remade) {
        uint32_t swapchains = made("vkCreateSwapchainKHR") + remade;
        uint32_t shown = made("vkQueuePresentKHR") + presents;

        device->fail_name = call;
        device->fail_nth = 1;
        device->fail_result = result;
        return raster.submit(0, picture, 0, vertices, batches, true, why) &&
               made("vkCreateSwapchainKHR") == swapchains &&
               made("vkQueuePresentKHR") == shown &&
               raster.submit(0, picture, 0, vertices, batches, true, why) &&
               made("vkCreateSwapchainKHR") == swapchains &&
               made("vkQueuePresentKHR") == shown + 1;
    };

    reset();
    device->width = window.width;
    device->height = window.height;
    options.window = &window;
    CHECK(raster.open(options, why));
    CHECK(shows("vkAcquireNextImageKHR", VK_ERROR_OUT_OF_DATE_KHR, 0, 1));
    CHECK(shows("vkQueuePresentKHR", VK_ERROR_OUT_OF_DATE_KHR, 1, 1));
    CHECK(shows("vkQueuePresentKHR", VK_SUBOPTIMAL_KHR, 1, 1));
    CHECK(shows("vkAcquireNextImageKHR", VK_SUBOPTIMAL_KHR, 1, 0));
    CHECK(device->faults == 0);
    return 0;
}

/* a sampler as a sample asks it of Vulkan, at a bias */
bool samples(const VkSamplerCreateInfo &made, uint32_t sample, float bias)
{
    uint32_t filter = sample & pc::SAMPLE_FILTER;
    uint32_t adjust = (sample & pc::SAMPLE_ADJUST) >> pc::SAMPLE_ADJUST_SHIFT;
    VkFilter texels =
        filter == SA_FILTER_POINT ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    auto edge = [&](uint32_t clamp, uint32_t flip) {
        return sample & clamp  ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
               : sample & flip ? VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT
                               : VK_SAMPLER_ADDRESS_MODE_REPEAT;
    };

    return made.magFilter == texels && made.minFilter == texels &&
           made.mipmapMode == (filter == SA_FILTER_TRILINEAR
                                   ? VK_SAMPLER_MIPMAP_MODE_LINEAR
                                   : VK_SAMPLER_MIPMAP_MODE_NEAREST) &&
           made.addressModeU == edge(pc::SAMPLE_CLAMP_U, pc::SAMPLE_FLIP_U) &&
           made.addressModeV == edge(pc::SAMPLE_CLAMP_V, pc::SAMPLE_FLIP_V) &&
           made.minLod == 0.0F && made.maxLod == (adjust != 0 ? 16.0F : 0.0F) &&
           fabsf(made.mipLodBias - bias) < 1e-5F;
}

/* how a batch's sample reaches Vulkan. the filter sets the sampler's
 * filters and mipmap mode, and trilinear alone blends two levels. a D adjust
 * adds the log2 of its quarter to the level. a picture drawn larger than it
 * shows adds its bias too, up to the most the device takes. an adjust of 0
 * takes the largest level alone. each draw's set names its texture's view and
 * its sample's sampler. samples alike share a sampler. so 1024 samples take
 * 432 samplers: three filters, three edges each for u and v, and 16 adjusts */
int sampling()
{
    static const pc::Image small = image_of(8, 8);
    static const struct {
        uint32_t adjust;
        float level; /* log2 of adjust over 4 */
        float larger; /* and at a picture twice the size, held to 2 */
    } ADJUSTS[] = {{0, 0.0F, 0.0F},       {1, -2.0F, -1.0F},
                   {2, -1.0F, 0.0F},      {3, -0.4150375F, 0.5849625F},
                   {4, 0.0F, 1.0F},       {8, 1.0F, 2.0F},
                   {15, 1.9068906F, 2.0F}};
    const uint32_t edges[] = {0, pc::SAMPLE_CLAMP_U, pc::SAMPLE_FLIP_U,
                              pc::SAMPLE_CLAMP_V | pc::SAMPLE_FLIP_V};
    pc::PictureUniform picture{};
    std::string why;

    for (uint32_t resolution : {1U, 2U}) {
        pc::Raster raster;
        pc::RasterOptions options;
        std::vector<pc::Batch> batches;
        std::vector<pc::Vertex> vertices(1024 * 3);

        reset();
        device->max_bias = 2.0F;
        options.resolution = resolution;
        CHECK(raster.open(options, why));
        uint32_t first = raster.add_texture(small, why);
        uint32_t second = raster.add_texture(small, why);

        CHECK(first != 0 && second != 0);
        /* every sample there is, of one texture */
        for (uint32_t sample = 0; sample < 1024; sample++) {
            batches.push_back(batch(sample, 1, first, sample, false));
        }
        CHECK(raster.submit(0, picture, 0, vertices, batches, false, why));
        CHECK(made("vkCreateSampler") == 432 && device->draws.size() == 1024);
        /* each filter at each adjust, then bilinear at each edge, then the
         * other texture: no sampler more */
        batches.clear();
        device->draws.clear();
        for (uint32_t filter = 0; filter < 3; filter++) {
            for (const auto &each : ADJUSTS) {
                batches.push_back(batch(
                    0, 1, first, pc::sample_of(filter, each.adjust), false));
            }
        }
        for (uint32_t edge : edges) {
            batches.push_back(batch(0, 1, first, BILINEAR | edge, false));
        }
        batches.push_back(batch(0, 1, second, BILINEAR, false));
        CHECK(raster.submit(0, picture, 0, vertices, batches, false, why));
        CHECK(made("vkCreateSampler") == 432 &&
              device->draws.size() == batches.size());
        const TestDraw *draw = device->draws.data();

        for (uint32_t filter = 0; filter < 3; filter++) {
            for (const auto &each : ADJUSTS) {
                CHECK(samples(draw->sampler, pc::sample_of(filter, each.adjust),
                              resolution == 1 ? each.level : each.larger));
                CHECK(draw->view == device->draws[0].view);
                draw++;
            }
        }
        for (uint32_t edge : edges) {
            CHECK(samples(draw->sampler, BILINEAR | edge,
                          resolution == 1 ? 0.0F : 1.0F));
            draw++;
        }
        /* the other texture's view, under the sampler the first drew by */
        CHECK(draw->view != device->draws[0].view &&
              samples(draw->sampler, BILINEAR, resolution == 1 ? 0.0F : 1.0F));
        CHECK(device->faults == 0);
    }
    return 0;
}

/* the device's shaders leave two counts in the blend list's head, which the
 * raster gives apart. lost gives the fragments the draws found no room for,
 * and cut gives those the resolve left out. the head starts each picture at
 * 0, so a picture's counts are its own. a picture composed in the batches'
 * order has no resolve to cut any. the device faults a copy of the head made
 * before a barrier lets it read both shaders' writes */
int counted()
{
    pc::Raster raster;
    pc::RasterOptions options;
    pc::PictureUniform picture{};
    std::vector<pc::Vertex> vertices(6);
    std::vector<pc::Batch> batches = {batch(0, 1, 0, 0, true),
                                      batch(1, 1, 0, BILINEAR, true)};
    std::string why;

    reset();
    options.frames = 2;
    CHECK(raster.open(options, why));
    device->shaded = [](uint32_t *head) { head[2] += 5; };
    device->resolved = [](uint32_t *head) { head[3] += 7; };
    CHECK(raster.submit(0, picture, 0, vertices, batches, true, why));
    raster.wait(0);
    CHECK(raster.lost(0) == 10 && raster.cut(0) == 7);
    CHECK(raster.submit(1, picture, 0, vertices, batches, false, why));
    raster.wait(1);
    CHECK(raster.lost(1) == 10 && raster.cut(1) == 0);
    device->shaded = nullptr;
    CHECK(raster.submit(1, picture, 0, vertices, batches, true, why));
    raster.wait(1);
    CHECK(raster.lost(1) == 0 && raster.cut(1) == 7);
    /* the first frame's picture keeps its own */
    CHECK(raster.lost(0) == 10 && raster.cut(0) == 7);
    CHECK(device->faults == 0);
    return 0;
}

/* a present the device fails leaves an acquired image that nothing shows,
 * so the raster breaks. it refuses each later picture and texture with that
 * reason, and its end leaves no object alive */
int unshown()
{
    static const pc::Image small = image_of(8, 8);
    pc::Window window = {800, 600};
    pc::PictureUniform picture{};
    std::vector<pc::Vertex> vertices(3);
    std::vector<pc::Batch> batches = {batch(0, 1, 0, 0, false)};
    std::string why;

    reset();
    device->width = window.width;
    device->height = window.height;
    {
        pc::Raster raster;
        pc::RasterOptions options;

        options.window = &window;
        options.frames = 2;
        CHECK(raster.open(options, why));
        device->fail_name = "vkQueuePresentKHR";
        device->fail_nth = 1;
        CHECK(!raster.submit(0, picture, 0, vertices, batches, true, why) &&
              why.find("vkQueuePresentKHR") != std::string::npos);
        std::string first = why;

        why.clear();
        CHECK(!raster.submit(1, picture, 0, vertices, batches, true, why) &&
              why == first);
        why.clear();
        CHECK(raster.add_texture(small, why) == 0 && why == first);
    }
    CHECK(device->faults == 0 && device->live == 0);
    return 0;
}

/* a frame gives the picture it holds, with its counts and its time, and
 * none else. a frame never used gives none. a picture the device refuses
 * still displaces its frame's last one, so the frame gives none. once made
 * again and taken, the refused picture is the one the frame gives */
int given()
{
    static uint8_t rgba[pc::WIDTH * pc::HEIGHT * 4];
    pc::PictureUniform picture{};
    std::vector<pc::Vertex> vertices(3);
    std::vector<pc::Batch> batches = {batch(0, 1, 0, 0, true)};
    std::vector<pc::Batch> culled = batches;
    std::string why;

    culled[0].state.cull = 1;
    reset();
    count();
    {
        pc::Raster raster;
        pc::RasterOptions options;
        auto gives = [&](uint32_t frame) {
            return raster.read(frame, rgba) && raster.lost(frame) == 5 &&
                   raster.cut(frame) == 7 && raster.gpu_ns(frame) != 0;
        };
        auto none = [&](uint32_t frame) {
            return raster.done(frame) && !raster.read(frame, rgba) &&
                   raster.lost(frame) == 0 && raster.cut(frame) == 0 &&
                   raster.gpu_ns(frame) == 0;
        };

        options.frames = 2;
        options.readback = true;
        CHECK(raster.open(options, why));
        CHECK(none(0) && none(1));
        CHECK(raster.submit(0, picture, 0, vertices, batches, true, why));
        raster.wait(0);
        CHECK(raster.done(0) && gives(0) && none(1));
        device->fail_name = "vkCreateGraphicsPipelines";
        device->fail_nth = 1;
        CHECK(!raster.submit(0, picture, 0, vertices, culled, true, why));
        CHECK(none(0));
        CHECK(raster.submit(0, picture, 0, vertices, culled, true, why));
        raster.wait(0);
        CHECK(raster.done(0) && gives(0));
    }
    CHECK(device->faults == 0 && device->live == 0);
    return 0;
}

/* the raster gives up a wait the device keeps failing for want of memory.
 * otherwise it would wait for ever. this holds at a frame's wait, and where a
 * picture waits for its frame's last one. the raster then breaks with the
 * picture still on the device. until its own end it ends nothing the picture
 * names and starts nothing over in its frame. the frame gives none of the
 * picture. the raster's end waits for the device's idle first */
int starved()
{
    static const pc::Image small = image_of(8, 8);
    static uint8_t rgba[pc::WIDTH * pc::HEIGHT * 4];
    pc::PictureUniform picture{};
    std::vector<pc::Vertex> vertices(3);
    std::string why;

    for (bool submits : {false, true}) {
        reset();
        count();
        {
            pc::Raster raster;
            pc::RasterOptions options;

            options.readback = true;
            CHECK(raster.open(options, why));
            uint32_t texture = raster.add_texture(small, why);
            std::vector<pc::Batch> batches = {
                batch(0, 1, texture, BILINEAR, true)};

            CHECK(texture != 0 &&
                  raster.submit(0, picture, 0, vertices, batches, true, why));
            device->fail_name = "vkWaitForFences";
            device->fail_nth = 1;
            device->fail_run = 1000;
            device->fail_result = VK_ERROR_OUT_OF_HOST_MEMORY;
            if (submits) {
                CHECK(!raster.submit(0, picture, 0, vertices, batches, true,
                                     why));
            } else {
                raster.wait(0);
            }
            /* the device would fail more waits than the raster made */
            CHECK(device->fail_nth == 1);
            raster.free_texture(texture);
            why.clear();
            CHECK(!raster.submit(0, picture, 0, vertices, batches, true, why) &&
                  why.find("vkWaitForFences") != std::string::npos);
            CHECK(raster.add_texture(small, why) == 0);
            CHECK(!raster.read(0, rgba) && raster.lost(0) == 0 &&
                  raster.cut(0) == 0 && raster.gpu_ns(0) == 0);
            CHECK(device->faults == 0);
        }
        CHECK(device->faults == 0 && device->live == 0);
    }
    return 0;
}

/* a device that fails its idle wait for want of memory may still read what
 * the raster made. what it may read includes a picture's buffers. the
 * raster's end then ends none of it, and gives up asking. this runs in a
 * process of its own, as what stays would count against every later raster */
int unsettled()
{
    int status = 0;

    fflush(stderr);
    pid_t child = fork();

    if (child == 0) {
        pc::PictureUniform picture{};
        std::vector<pc::Vertex> vertices(3);
        std::vector<pc::Batch> batches = {batch(0, 1, 0, 0, false)};
        std::string why;
        bool sent = false;

        alarm(30);
        reset();
        {
            pc::Raster raster;
            pc::RasterOptions options;

            sent = raster.open(options, why) &&
                   raster.submit(0, picture, 0, vertices, batches, true, why);
            device->fail_name = "vkDeviceWaitIdle";
            device->fail_nth = 1;
            device->fail_run = 1000;
            device->fail_result = VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        _exit(sent && device->faults == 0 && device->live != 0 &&
                      device->fail_nth == 1
                  ? 0
                  : 1);
    }
    CHECK(child > 0 && waitpid(child, &status, 0) == child);
    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    return 0;
}

/* a texture that ends leaves its slot to the next of its size. so textures
 * made and ended in turn take no memory past the first's. that holds though
 * they outnumber a block's slots */
int reused()
{
    static const pc::Image middling = image_of(256, 256);
    std::string why;

    reset();
    {
        pc::Raster raster;
        pc::RasterOptions options;

        CHECK(raster.open(options, why));
        uint32_t texture = raster.add_texture(middling, why);
        uint32_t taken = made("vkAllocateMemory");

        for (int i = 0; i < 64; i++) {
            CHECK(texture != 0);
            raster.free_texture(texture);
            texture = raster.add_texture(middling, why);
        }
        CHECK(texture != 0 && made("vkAllocateMemory") == taken);
    }
    CHECK(device->faults == 0 && device->live == 0);
    return 0;
}

/* a raster opens once. a second open is refused and makes no call, whether
 * the first failed or took. what the first made ends with the raster */
int reopened()
{
    for (bool fails : {true, false}) {
        std::string why;

        reset();
        {
            pc::Raster raster;
            pc::RasterOptions options;

            device->fail_name = "vkCreateCommandPool";
            device->fail_nth = fails ? 1 : 0;
            CHECK(raster.open(options, why) == !fails);
            size_t calls = device->calls.size();

            why.clear();
            CHECK(!raster.open(options, why) && !why.empty() &&
                  device->calls.size() == calls);
        }
        CHECK(device->faults == 0 && device->live == 0);
    }
    return 0;
}

/* a loader, instance or device that lacks a call opens no raster, and the
 * reason names the call. neither the open nor the raster's end calls it */
int lacking()
{
    for (const char *call : {"vkCreateInstance", "vkCreateDevice",
                             "vkCreateSampler", "vkQueuePresentKHR"}) {
        pc::Window window = {800, 600};
        std::string why;

        reset();
        device->width = window.width;
        device->height = window.height;
        device->lacks = call;
        {
            pc::Raster raster;
            pc::RasterOptions options;

            options.window = &window;
            CHECK(!raster.open(options, why));
            CHECK(why.find(call) != std::string::npos);
        }
        CHECK(device->faults == 0 && device->live == 0);
    }
    return 0;
}

/* the raster takes the first device listed whose name holds the one asked
 * for. with none asked for, or no name that holds it, it takes the first
 * listed */
int named()
{
    static const struct {
        const char *asked;
        const char *taken;
    } CASES[] = {{"", "one"},
                 {"two", "two, the first"},
                 {"the second", "two, the second"},
                 {"three", "one"}};

    for (const auto &each : CASES) {
        std::string why;

        reset();
        device->names = {"one", "two, the first", "two, the second"};
        {
            pc::Raster raster;
            pc::RasterOptions options;

            options.device = each.asked;
            CHECK(raster.open(options, why));
            CHECK(raster.device_name() == each.taken);
        }
        CHECK(device->faults == 0 && device->live == 0);
    }
    return 0;
}

int failing()
{
    int failures = 0;

    for (bool loss : {false, true}) {
        failures += sweep("headless", headless, loss);
        failures += sweep("a window fitted", fitted, loss);
        failures += sweep("a window sized", sized, loss);
    }
    if (failures != 0) {
        fprintf(stderr, "device: %d failing calls leave the raster wrong\n",
                failures);
    }
    CHECK(failures == 0);
    return 0;
}

} // namespace

int main()
{
    void *loader = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    auto find = reinterpret_cast<TestDevice *(*)(void)>(
        loader != nullptr ? dlsym(loader, "test_device") : nullptr);

    if (find == nullptr) {
        fprintf(stderr, "device: libvulkan.so.1 is not the test's own, which "
                        "make engine-test puts first where a loader is "
                        "looked for\n");
        return 1;
    }
    device = find();
    int line = outgrown();

    if (line == 0) {
        line = unshown();
    }
    if (line == 0) {
        line = given();
    }
    if (line == 0) {
        line = starved();
    }
    if (line == 0) {
        line = unsettled();
    }
    if (line == 0) {
        line = reused();
    }
    if (line == 0) {
        line = reopened();
    }
    if (line == 0) {
        line = lacking();
    }
    if (line == 0) {
        line = sampling();
    }
    if (line == 0) {
        line = counted();
    }
    if (line == 0) {
        line = named();
    }
    if (line == 0) {
        line = failing();
    }

    if (line != 0) {
        fprintf(stderr, "device: line %d\n", line);
    }
    return line != 0;
}
