/* the ray tracing side of the raster's device. it keeps a bottom-level
 * acceleration structure for each mesh, builds a top-level one over a picture's
 * instances, and traces the picture with ray queries in a compute pass. two
 * passes then denoise the indirect light, one over time and one over space, and
 * a last composes each pixel as display values. a fullscreen triangle copies
 * the result into the target under the picture's batches, with each camera
 * ray's depth. ENGINE.md#ray-tracing describes the model */
#include "device.h"

#include <math.h>
#include <string.h>

#include <algorithm>
#include <iterator>

#include "atrous.comp.h"
#include "compose.comp.h"
#include "enclosure.comp.h"
#include "fullscreen.vert.h"
#include "taa.comp.h"
#include "temporal.comp.h"
#include "trace.comp.h"
#include "traced.frag.h"

/* a create info names its type and leaves the rest zero, which -Wextra
 * would call a missing initializer */
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

namespace pc {

namespace {

/* the size of a heap's blocks, from which the meshes' data and acceleration
 * structures take spans. a mesh larger than a block gets a block of its own */
constexpr VkDeviceSize HEAP_BLOCK = VkDeviceSize{16} << 20;
/* the scratch buffer that the builds of one load share, each with a span of its
 * own */
constexpr VkDeviceSize SCRATCH = VkDeviceSize{8} << 20;
/* the size of the shaders' texture array. a texture's index in it is its index
 * in the raster */
constexpr uint32_t RAY_TEXTURES = 16384;
/* the shaders' samplers: point or linear, with one of three address modes on
 * each axis. a ray computes its own mip level, so a sampler has every level and
 * no bias */
constexpr VkSamplerAddressMode ADDRESS_MODES[3] = {
    VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};
constexpr uint32_t RAY_SAMPLERS = 2 * 3 * 3;
/* the storage buffers in the compute stage's descriptor sets: a frame's
 * instances, materials, ramps and lamps, and the enclosure measure */
constexpr uint32_t RAY_BUFFERS = 5;

/* the index of the sampler for sample, a batch's SAMPLE_* bits. a clamp takes
 * precedence over a flip, as in the raster's own samplers */
constexpr uint32_t sampler_place(uint32_t sample)
{
    uint32_t u = sample & SAMPLE_CLAMP_U ? 2 : sample & SAMPLE_FLIP_U ? 1 : 0;
    uint32_t v = sample & SAMPLE_CLAMP_V ? 2 : sample & SAMPLE_FLIP_V ? 1 : 0;

    return ((sample & SAMPLE_FILTER) == SA_FILTER_POINT ? 9 : 0) + u * 3 + v;
}
/* the instance capacity of the first top-level structure */
constexpr uint32_t FIRST_ROOM = 1024;
/* the sizes of a mesh's data: a point's six floats, a triangle's three indices,
 * and a triangle's Corner as the shaders declare it */
constexpr VkDeviceSize POINT = 24;
constexpr VkDeviceSize TRIANGLE = 12;
constexpr VkDeviceSize CORNER = 32;
/* instance masks, as trace.comp describes them */
constexpr uint32_t MASK_SEEN = 1;
constexpr uint32_t MASK_HIDDEN = 2;
constexpr uint32_t MASK_BACKDROP = 4;
/* set beside MASK_SEEN or MASK_HIDDEN: at least one of its materials doesn't
 * blend, so enclosure.comp's probes hit it */
constexpr uint32_t MASK_SOLID = 8;
/* set beside MASK_SOLID: it is an object, and no scenery, so it casts an
 * overhead shadow */
constexpr uint32_t MASK_OBJECT = 16;

/* a span of one of a heap's blocks */
struct Span {
    uint32_t block = UINT32_MAX;
    VkDeviceSize at = 0;
    VkDeviceSize size = 0;
};

struct Block {
    Buffer buffer;
    VkDeviceAddress address = 0;
    /* its free spans, each an offset and a size, sorted by offset */
    std::vector<std::pair<VkDeviceSize, VkDeviceSize>> free;
};

/* device memory that the meshes share. a block is allocated only where no free
 * span fits, and is freed with the raster */
struct Heap {
    VkBufferUsageFlags usage = 0;
    std::vector<Block> blocks;
};

struct Mesh {
    VkAccelerationStructureKHR structure = VK_NULL_HANDLE;
    VkDeviceAddress address = 0; /* the structure's */
    Span bytes; /* its points, then its indices, then its corners */
    Span room; /* the structure's storage */
    VkDeviceAddress points = 0;
    VkDeviceAddress indices = 0;
    VkDeviceAddress corners = 0;
    uint64_t load = 0; /* the number of the load that builds it */
};

/* an instance, a material and the trace's uniforms, laid out as trace.comp's
 * Instance and Material and rays.glsl's Rays */
struct InstanceWords {
    float to_world[12];
    float before[12];
    float light_way[4];
    uint64_t points;
    uint64_t indices;
    uint64_t corners;
    uint32_t materials;
    uint32_t flags;
    uint32_t light;
    float ambient;
    float diffuse;
    uint32_t reserved;
};
static_assert(sizeof(InstanceWords) == 160, "the shaders' Instance");
struct MaterialWords {
    uint32_t texture;
    uint32_t sampling;
    uint32_t flags;
    uint32_t color;
    uint32_t specular;
    uint32_t blend;
    uint32_t gloss;
    uint32_t reserved;
};
static_assert(sizeof(MaterialWords) == 32, "the shaders' Material");
struct Constants {
    float eye[12];
    float to_sun[4];
    float back[3][4];
    uint32_t counts[4];
    float size[4];
    float before_view[12];
    float before_project[4];
    float clock[4];
    float lamp[4];
    float shown[4];
    float jitter[4];
};
static_assert(sizeof(Constants) == 272, "the shaders' Rays");
/* the size of a picture's ramps, as the shaders' Ramps declares them */
constexpr VkDeviceSize RAMPS = RAY_LIGHTS * RAY_RAMP * 2 * sizeof(uint32_t);
/* an a-trous iteration's push constants, as atrous.comp's Pass declares them */
struct Pass {
    int32_t stride;
    int32_t turn;
};

/* the ray tracing images: an image's binding in the images' descriptor set,
 * its count and its format. a row is named for the image as the shaders
 * declare it, which tests/test_shaders.py checks. gbuffer, history, moments
 * and past are pairs that alternate by picture, and an a-trous iteration reads
 * one of filtered's pair and writes the other. each is the traced picture's
 * size, but for those at_target() names, which are the target's */
struct Held {
    uint32_t binding;
    uint32_t count;
    VkFormat format;
};
constexpr Held HELD[] = {
    {0, 1, VK_FORMAT_R16G16B16A16_SFLOAT}, /* direct */
    {1, 1, VK_FORMAT_R16G16B16A16_SFLOAT}, /* albedo */
    {2, 1, VK_FORMAT_R16G16B16A16_SFLOAT}, /* indirect */
    {3, 1, VK_FORMAT_R16G16B16A16_SFLOAT}, /* motion */
    {4, 1, VK_FORMAT_R32_SFLOAT}, /* depth */
    {5, 2, VK_FORMAT_R16G16B16A16_SFLOAT}, /* gbuffer */
    {6, 2, VK_FORMAT_R16G16B16A16_SFLOAT}, /* history */
    {7, 2, VK_FORMAT_R16G16B16A16_SFLOAT}, /* moments */
    {8, 2, VK_FORMAT_R16G16B16A16_SFLOAT}, /* filtered */
    {9, 1, VK_FORMAT_R8G8B8A8_UNORM}, /* shown */
    {11, 1, VK_FORMAT_R16G16B16A16_SFLOAT}, /* highlight */
    {12, 1, VK_FORMAT_R16G16B16A16_SFLOAT}, /* layer_adds */
    {13, 1, VK_FORMAT_R16G16B16A16_SFLOAT}, /* layer_keeps */
    {14, 1, VK_FORMAT_R16G16B16A16_SFLOAT}, /* ambient */
    {15, 1, VK_FORMAT_R16G16B16A16_SFLOAT}, /* composed */
    {16, 2, VK_FORMAT_R16G16B16A16_SFLOAT}}; /* past */
/* whether a binding's images are the target's size: the picture the
 * antialiasing pass writes, and its history */
constexpr bool at_target(uint32_t binding)
{
    return binding == 9 || binding == 16;
}
/* the index among the images of a binding's first image, or the image count for
 * a binding that HELD lacks */
constexpr uint32_t image_of(uint32_t binding)
{
    uint32_t at = 0;

    for (const Held &held : HELD) {
        if (held.binding == binding) {
            return at;
        }
        at += held.count;
    }
    return at;
}
constexpr uint32_t IMAGES = image_of(UINT32_MAX);
constexpr uint32_t IMAGE_DEPTH = image_of(4);
constexpr uint32_t IMAGE_SHOWN = image_of(9);
/* the binding of the enclosure measure's buffer in the images' descriptor
 * set */
constexpr uint32_t ENCLOSURE_BINDING = 10;
/* the a-trous iterations, each with twice the stride of the last. compose.comp
 * reads filtered[0], where an even count leaves the result */
constexpr int32_t STEPS = 4;
static_assert(STEPS % 2 == 0, "compose.comp reads filtered[0]");
/* the compute passes, in the order a picture runs them, then the two shaders
 * that copy the traced picture into the target */
enum {
    PASS_ENCLOSURE,
    PASS_TRACE,
    PASS_TEMPORAL,
    PASS_ATROUS,
    PASS_COMPOSE,
    PASS_TAA,
    PASSES,
    MODULE_VERTEX = PASSES,
    MODULE_FRAGMENT,
    MODULES
};
/* from this many rays a pixel a picture is not denoised. trace.comp copies the
 * value */
constexpr uint32_t UNFILTERED = 64;

/* the points of the Halton sequence that the camera rays' place in their
 * pixels steps through */
constexpr uint32_t JITTERS = 16;

/* the radical inverse of index in base: the Halton sequence's point */
float halton(uint32_t index, uint32_t base)
{
    float point = 0.0F;
    float share = 1.0F / static_cast<float>(base);

    for (; index != 0; index /= base) {
        point += static_cast<float>(index % base) * share;
        share /= static_cast<float>(base);
    }
    return point;
}

VkDeviceSize up(VkDeviceSize value, VkDeviceSize to)
{
    return (value + to - 1) / to * to;
}

/* a mesh's triangle geometry for a build, without the addresses of its points
 * and its indices */
VkAccelerationStructureGeometryKHR triangles_of(uint32_t points)
{
    VkAccelerationStructureGeometryKHR geometry{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    VkAccelerationStructureGeometryTrianglesDataKHR &held =
        geometry.geometry.triangles;

    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    /* a ray query returns each candidate triangle once. a shadow ray multiplies
     * in the transmittance of each blended triangle, so a duplicate would
     * attenuate it twice */
    geometry.flags = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
    held.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    held.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    held.vertexStride = POINT;
    held.maxVertex = points - 1;
    held.indexType = VK_INDEX_TYPE_UINT32;
    return geometry;
}

/* the build of a mesh's bottom-level structure over geometry, without the
 * structure and the scratch address */
VkAccelerationStructureBuildGeometryInfoKHR
build_of(const VkAccelerationStructureGeometryKHR *geometry)
{
    VkAccelerationStructureBuildGeometryInfoKHR build{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};

    build.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    build.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    build.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build.geometryCount = 1;
    build.pGeometries = geometry;
    return build;
}

/* the top-level build over a picture's instances, without the instances'
 * address, the structure and the scratch address. the size query and every
 * build must use the same parameters, so both get them here. build points at
 * geometry, so the caller owns both */
void top_build(VkAccelerationStructureGeometryKHR &geometry,
               VkAccelerationStructureBuildGeometryInfoKHR &build)
{
    geometry = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    build = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    build.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    build.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
    build.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build.geometryCount = 1;
    build.pGeometries = &geometry;
}

/* a build that the load being recorded has yet to run: a mesh added since the
 * last load, with its point count, its triangle count and the offset of its
 * span of the scratch buffer */
struct Build {
    uint32_t mesh;
    uint32_t points;
    uint32_t triangles;
    VkDeviceSize scratch;
};

} // namespace

struct Raster::Impl::Rays {
    Heap bytes;
    Heap structures;
    /* the scratch buffer for a load's builds, usable from scratch_address, each
     * build with a span of its own. scratched is how much the builds of the
     * load being recorded have taken */
    Buffer scratch;
    VkDeviceAddress scratch_address = 0;
    VkDeviceSize scratch_room = 0;
    VkDeviceSize scratched = 0;
    VkDeviceSize scratch_align = 1;
    std::vector<Build> builds;
    /* ray_build's arguments to the one command that runs the builds */
    std::vector<VkAccelerationStructureGeometryKHR> geometries;
    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> infos;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges;
    std::vector<const VkAccelerationStructureBuildRangeInfoKHR *> ranged;
    /* a picture's lamps as lamp_table() lays them out, kept to reuse its
     * storage */
    std::vector<uint32_t> lamp_words;
    std::vector<Mesh> meshes; /* by index, from 1 */
    std::vector<uint32_t> free_meshes;
    /* the top-level structure over a picture's instances, rebuilt for each
     * picture. the pictures share it, as they share the target */
    VkAccelerationStructureKHR top = VK_NULL_HANDLE;
    Buffer top_buffer;
    Buffer top_scratch;
    VkDeviceAddress top_scratch_address = 0;
    uint32_t top_room = 0; /* its capacity in instances */
    uint32_t count = 0; /* the instances of the picture being sent */
    VkDescriptorSetLayout frame_layout = VK_NULL_HANDLE;
    VkDescriptorSetLayout texture_layout = VK_NULL_HANDLE;
    VkDescriptorSetLayout shown_set_layout = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipelineLayout shown_layout = VK_NULL_HANDLE;
    VkDescriptorSetLayout image_layout = VK_NULL_HANDLE;
    VkShaderModule modules[MODULES] = {};
    VkPipeline passes[PASSES] = {};
    VkPipeline show = VK_NULL_HANDLE;
    VkDescriptorPool pool = VK_NULL_HANDLE;
    VkDescriptorSet texture_set = VK_NULL_HANDLE;
    VkDescriptorSet image_set = VK_NULL_HANDLE;
    VkDescriptorSet shown_set = VK_NULL_HANDLE;
    /* the view bound at each index of the texture array, or null for none
     * yet */
    std::vector<VkImageView> bound;
    VkSampler samplers[RAY_SAMPLERS] = {}; /* by sampler_place() */
    /* the ray tracing images, in HELD's order */
    VkImage images[IMAGES] = {};
    VkDeviceMemory memories[IMAGES] = {};
    VkImageView views[IMAGES] = {};
    /* enclosure.comp's measure, which it carries from picture to picture */
    Buffer enclosure;
    /* the traced picture's size: the target's, times the render scale */
    VkExtent2D traced = {};
    uint32_t samples = 1; /* the rays a pixel of the picture being sent */
    /* the passes' history is not of the picture before: the target is new, or a
     * rasterized picture came between */
    bool stale = true;
    /* the last traced picture was denoised, so its history exists. a picture of
     * UNFILTERED rays a pixel or more is not denoised, and the next denoised
     * picture starts without history */
    bool filtered = false;
    uint32_t pictures = 0; /* traced so far: a picture's random seed */

    static VkDeviceAddress address(Impl &r, const Buffer &b);
    bool make_scratch(Impl &r, Buffer &b, VkDeviceSize size,
                      VkDeviceAddress &at, std::string &why);
    bool take(Impl &r, Heap &heap, VkDeviceSize size, VkDeviceSize align,
              Span &span, std::string &why);
    void give(Heap &heap, Span &span);
    bool make_top(Impl &r, uint32_t room, std::string &why);
    bool grown(Impl &r, Buffer &b, VkDeviceSize size, VkBufferUsageFlags usage,
               bool &made, std::string &why);
};

VkDeviceAddress Raster::Impl::Rays::address(Impl &r, const Buffer &b)
{
    VkBufferDeviceAddressInfo info{
        VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};

    info.buffer = b.buffer;
    return r.vk.vkGetBufferDeviceAddress(r.device, &info);
}

/* replace b with a scratch buffer of size bytes. at is the first address in it
 * that is aligned for a build */
bool Raster::Impl::Rays::make_scratch(Impl &r, Buffer &b, VkDeviceSize size,
                                      VkDeviceAddress &at, std::string &why)
{
    r.free_buffer(b);
    if (!r.make_buffer(b, size + scratch_align,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                           VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                       why, true)) {
        return false;
    }
    at = up(address(r, b), scratch_align);
    return true;
}

/* take a span of size bytes from heap, at a multiple of align. false, with why,
 * where no span is free and the device refuses a new block */
bool Raster::Impl::Rays::take(Impl &r, Heap &heap, VkDeviceSize size,
                              VkDeviceSize align, Span &span, std::string &why)
{
    /* cut the span from the first free span of block i that fits it */
    auto cut = [&](uint32_t i) {
        Block &block = heap.blocks[i];

        for (size_t j = 0; j < block.free.size(); j++) {
            VkDeviceSize start = block.free[j].first;
            VkDeviceSize end = start + block.free[j].second;
            VkDeviceSize at = up(start, align);

            if (at + size > end) {
                continue;
            }
            block.free.erase(block.free.begin() +
                             static_cast<std::ptrdiff_t>(j));
            if (at + size < end) {
                block.free.insert(block.free.begin() +
                                      static_cast<std::ptrdiff_t>(j),
                                  {at + size, end - at - size});
            }
            if (at > start) {
                block.free.insert(block.free.begin() +
                                      static_cast<std::ptrdiff_t>(j),
                                  {start, at - start});
            }
            span = Span{i, at, size};
            return true;
        }
        return false;
    };

    for (uint32_t i = 0; i < heap.blocks.size(); i++) {
        if (cut(i)) {
            return true;
        }
    }
    /* a new block fits the span at offset 0, which any alignment divides */
    Block block;
    VkDeviceSize room = size > HEAP_BLOCK ? size : HEAP_BLOCK;

    if (!r.make_buffer(block.buffer, room, heap.usage, why, true)) {
        return false;
    }
    block.address = address(r, block.buffer);
    block.free.emplace_back(0, room);
    heap.blocks.push_back(block);
    return cut(static_cast<uint32_t>(heap.blocks.size() - 1));
}

/* return a span to its block, merged with the free spans next to it */
void Raster::Impl::Rays::give(Heap &heap, Span &span)
{
    if (span.block == UINT32_MAX) {
        return;
    }
    auto &free = heap.blocks[span.block].free;
    auto at =
        std::lower_bound(free.begin(), free.end(),
                         std::pair<VkDeviceSize, VkDeviceSize>{span.at, 0});

    at = free.insert(at, {span.at, span.size});
    if (at + 1 != free.end() && at->first + at->second == (at + 1)->first) {
        at->second += (at + 1)->second;
        free.erase(at + 1);
    }
    if (at != free.begin() && (at - 1)->first + (at - 1)->second == at->first) {
        (at - 1)->second += at->second;
        free.erase(at);
    }
    span = Span{};
}

/* make b at least size bytes, replacing a smaller buffer, which nothing may
 * still be reading. made is set where b is a new buffer */
bool Raster::Impl::Rays::grown(Impl &r, Buffer &b, VkDeviceSize size,
                               VkBufferUsageFlags usage, bool &made,
                               std::string &why)
{
    made = false;
    if (b.size >= size) {
        return true;
    }
    r.free_buffer(b);
    made = true;
    return r.make_buffer(b, size * 2, usage, why);
}

bool Raster::Impl::ray_device(RayNeeds &needs)
{
    static const char *const names[3] = {
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME};
    uint32_t count = 0;

    if (properties.apiVersion < VK_API_VERSION_1_2 ||
        vk.vkGetPhysicalDeviceFeatures2 == nullptr ||
        vk.vkGetPhysicalDeviceProperties2 == nullptr ||
        vk.vkEnumerateDeviceExtensionProperties(physical, nullptr, &count,
                                                nullptr) != VK_SUCCESS ||
        count == 0) {
        return false;
    }
    std::vector<VkExtensionProperties> has(count);

    if (vk.vkEnumerateDeviceExtensionProperties(physical, nullptr, &count,
                                                has.data()) != VK_SUCCESS) {
        return false;
    }
    has.resize(count);
    for (const char *name : names) {
        bool found = false;

        for (const VkExtensionProperties &e : has) {
            found = found || strcmp(e.extensionName, name) == 0;
        }
        if (!found) {
            return false;
        }
    }
    VkPhysicalDeviceRayQueryFeaturesKHR query{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR structure{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        &query};
    VkPhysicalDeviceVulkan12Features core{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, &structure};
    VkPhysicalDeviceFeatures2 all{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                                  &core};
    const VkPhysicalDeviceLimits &limits = properties.limits;

    vk.vkGetPhysicalDeviceFeatures2(physical, &all);
    /* the compute stage's descriptor sets hold every texture and sampler, the
     * ray tracing images, RAY_BUFFERS storage buffers, two uniform buffers and
     * the top-level structure */
    if (!query.rayQuery || !structure.accelerationStructure ||
        !core.bufferDeviceAddress || !core.descriptorIndexing ||
        !core.runtimeDescriptorArray ||
        !core.shaderSampledImageArrayNonUniformIndexing ||
        !core.descriptorBindingPartiallyBound ||
        !core.descriptorBindingUpdateUnusedWhilePending ||
        !all.features.shaderSampledImageArrayDynamicIndexing ||
        limits.maxPerStageDescriptorSampledImages < RAY_TEXTURES ||
        limits.maxDescriptorSetSampledImages < RAY_TEXTURES ||
        limits.maxPerStageDescriptorSamplers < RAY_SAMPLERS ||
        limits.maxDescriptorSetSamplers < RAY_SAMPLERS ||
        limits.maxPerStageDescriptorStorageImages < IMAGES ||
        limits.maxPerStageDescriptorStorageBuffers < RAY_BUFFERS ||
        limits.maxPerStageResources <
            RAY_TEXTURES + RAY_SAMPLERS + IMAGES + RAY_BUFFERS + 3) {
        return false;
    }
    /* open creates the device with these features alone, not with every feature
     * the device supports */
    needs = RayNeeds{};
    needs.query.sType = query.sType;
    needs.structure.sType = structure.sType;
    needs.core.sType = core.sType;
    needs.query.rayQuery = VK_TRUE;
    needs.structure.accelerationStructure = VK_TRUE;
    needs.core.bufferDeviceAddress = VK_TRUE;
    needs.core.descriptorIndexing = VK_TRUE;
    needs.core.runtimeDescriptorArray = VK_TRUE;
    needs.core.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    needs.core.descriptorBindingPartiallyBound = VK_TRUE;
    needs.core.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
    needs.indexing = true;
    memcpy(needs.extensions, names, sizeof names);
    return true;
}

bool Raster::Impl::make_rays(std::string &why)
{
    rays = new Rays;
    Rays &y = *rays;
    uint32_t count = static_cast<uint32_t>(frames.size());
    VkPhysicalDeviceAccelerationStructurePropertiesKHR held{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
    VkPhysicalDeviceProperties2 all{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, &held};

    vk.vkGetPhysicalDeviceProperties2(physical, &all);
    y.scratch_align = held.minAccelerationStructureScratchOffsetAlignment != 0
                          ? held.minAccelerationStructureScratchOffsetAlignment
                          : 1;
    y.bytes.usage =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    y.structures.usage =
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    y.meshes.emplace_back(); /* index 0 names no mesh */
    /* a frame's descriptor set: the top-level structure, the picture's uniforms
     * and the trace's, and the frame's instances, materials, ramps and lamps */
    VkShaderStageFlags stage = VK_SHADER_STAGE_COMPUTE_BIT;
    VkDescriptorSetLayoutBinding frame_bindings[7] = {
        {0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, stage, nullptr},
        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, stage, nullptr},
        {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, stage, nullptr},
        {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, stage, nullptr},
        {4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, stage, nullptr},
        {5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, stage, nullptr},
        {6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, stage, nullptr}};
    /* every texture and sampler, in one descriptor set. a picture reads only
     * the textures its materials use, so an index may be unbound, and may be
     * rebound while a picture that doesn't use it is in flight */
    VkDescriptorSetLayoutBinding texture_bindings[2] = {
        {0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RAY_TEXTURES, stage, nullptr},
        {1, VK_DESCRIPTOR_TYPE_SAMPLER, RAY_SAMPLERS, stage, nullptr}};
    VkDescriptorBindingFlags loose[2] = {
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
        0};
    VkDescriptorSetLayoutBindingFlagsCreateInfo flags{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
    /* the ray tracing images, which the pictures share as they share the
     * target, and the enclosure measure */
    VkDescriptorSetLayoutBinding image_bindings[std::size(HELD) + 1];
    VkDescriptorSetLayoutBinding shown_bindings[2] = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
         nullptr},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
         nullptr}};
    VkDescriptorSetLayoutCreateInfo set_info{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};

    for (uint32_t i = 0; i < std::size(HELD); i++) {
        image_bindings[i] = {HELD[i].binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                             HELD[i].count, stage, nullptr};
    }
    image_bindings[std::size(HELD)] = {ENCLOSURE_BINDING,
                                       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                       stage, nullptr};
    set_info.bindingCount = static_cast<uint32_t>(std::size(frame_bindings));
    set_info.pBindings = frame_bindings;
    if (!made(vk.vkCreateDescriptorSetLayout(device, &set_info, nullptr,
                                             &y.frame_layout),
              y.frame_layout, "vkCreateDescriptorSetLayout", why)) {
        return false;
    }
    flags.bindingCount = 2;
    flags.pBindingFlags = loose;
    set_info.pNext = &flags;
    set_info.bindingCount = 2;
    set_info.pBindings = texture_bindings;
    if (!made(vk.vkCreateDescriptorSetLayout(device, &set_info, nullptr,
                                             &y.texture_layout),
              y.texture_layout, "vkCreateDescriptorSetLayout", why)) {
        return false;
    }
    set_info.pNext = nullptr;
    set_info.bindingCount = static_cast<uint32_t>(std::size(image_bindings));
    set_info.pBindings = image_bindings;
    if (!made(vk.vkCreateDescriptorSetLayout(device, &set_info, nullptr,
                                             &y.image_layout),
              y.image_layout, "vkCreateDescriptorSetLayout", why)) {
        return false;
    }
    set_info.bindingCount = 2;
    set_info.pBindings = shown_bindings;
    if (!made(vk.vkCreateDescriptorSetLayout(device, &set_info, nullptr,
                                             &y.shown_set_layout),
              y.shown_set_layout, "vkCreateDescriptorSetLayout", why)) {
        return false;
    }
    VkDescriptorSetLayout set_layouts[3] = {y.frame_layout, y.texture_layout,
                                            y.image_layout};
    VkPushConstantRange push{stage, 0, sizeof(Pass)};
    VkPipelineLayoutCreateInfo layout_info{
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

    layout_info.setLayoutCount = 3;
    layout_info.pSetLayouts = set_layouts;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push;
    if (!made(
            vk.vkCreatePipelineLayout(device, &layout_info, nullptr, &y.layout),
            y.layout, "vkCreatePipelineLayout", why)) {
        return false;
    }
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &y.shown_set_layout;
    layout_info.pushConstantRangeCount = 0;
    if (!made(vk.vkCreatePipelineLayout(device, &layout_info, nullptr,
                                        &y.shown_layout),
              y.shown_layout, "vkCreatePipelineLayout", why)) {
        return false;
    }
    const uint32_t *codes[MODULES] = {
        enclosure_comp, trace_comp, temporal_comp,   atrous_comp,
        compose_comp,   taa_comp,   fullscreen_vert, traced_frag};
    size_t code_sizes[MODULES] = {
        sizeof enclosure_comp, sizeof trace_comp,      sizeof temporal_comp,
        sizeof atrous_comp,    sizeof compose_comp,    sizeof taa_comp,
        sizeof fullscreen_vert, sizeof traced_frag};
    VkComputePipelineCreateInfo compute{
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};

    for (uint32_t i = 0; i < MODULES; i++) {
        VkShaderModuleCreateInfo module_info{
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};

        module_info.codeSize = code_sizes[i];
        module_info.pCode = codes[i];
        if (!made(vk.vkCreateShaderModule(device, &module_info, nullptr,
                                          &y.modules[i]),
                  y.modules[i], "vkCreateShaderModule", why)) {
            return false;
        }
    }
    compute.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    compute.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    compute.stage.pName = "main";
    compute.layout = y.layout;
    for (uint32_t i = 0; i < PASSES; i++) {
        compute.stage.module = y.modules[i];
        if (!made(vk.vkCreateComputePipelines(device, VK_NULL_HANDLE, 1,
                                              &compute, nullptr, &y.passes[i]),
                  y.passes[i], "vkCreateComputePipelines", why)) {
            return false;
        }
    }
    /* the pipeline that copies the traced picture into the target, in the
     * target's render pass: it has no vertex input, and writes depth
     * unconditionally */
    Drawing under;

    under.vertex = y.modules[MODULE_VERTEX];
    under.fragment = y.modules[MODULE_FRAGMENT];
    under.layout = y.shown_layout;
    under.depth.depthTestEnable = VK_TRUE;
    under.depth.depthWriteEnable = VK_TRUE;
    under.depth.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    under.attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    y.show = make_pipeline(under, why);
    if (y.show == VK_NULL_HANDLE) {
        return false;
    }
    /* the pool's descriptor sets: one a frame, the textures', the images' and
     * the copy's */
    VkDescriptorPoolSize sizes[6] = {
        {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, count},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count * 2},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count * (RAY_BUFFERS - 1) + 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMAGES + 2},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RAY_TEXTURES},
        {VK_DESCRIPTOR_TYPE_SAMPLER, RAY_SAMPLERS}};
    VkDescriptorPoolCreateInfo pool_info{
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    VkDescriptorSetAllocateInfo alloc{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};

    pool_info.maxSets = count + 3;
    pool_info.poolSizeCount = 6;
    pool_info.pPoolSizes = sizes;
    if (!made(vk.vkCreateDescriptorPool(device, &pool_info, nullptr, &y.pool),
              y.pool, "vkCreateDescriptorPool", why)) {
        return false;
    }
    alloc.descriptorPool = y.pool;
    alloc.descriptorSetCount = 1;
    alloc.pSetLayouts = &y.texture_layout;
    if (!made(vk.vkAllocateDescriptorSets(device, &alloc, &y.texture_set),
              y.texture_set, "vkAllocateDescriptorSets", why)) {
        return false;
    }
    alloc.pSetLayouts = &y.image_layout;
    if (!made(vk.vkAllocateDescriptorSets(device, &alloc, &y.image_set),
              y.image_set, "vkAllocateDescriptorSets", why)) {
        return false;
    }
    alloc.pSetLayouts = &y.shown_set_layout;
    if (!made(vk.vkAllocateDescriptorSets(device, &alloc, &y.shown_set),
              y.shown_set, "vkAllocateDescriptorSets", why)) {
        return false;
    }
    y.bound.assign(RAY_TEXTURES, VK_NULL_HANDLE);
    VkDescriptorImageInfo sampling[RAY_SAMPLERS];
    VkWriteDescriptorSet samplers{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

    for (uint32_t i = 0; i < RAY_SAMPLERS; i++) {
        VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

        info.magFilter = i < 9 ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        info.minFilter = info.magFilter;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.addressModeU = ADDRESS_MODES[i % 9 / 3];
        info.addressModeV = ADDRESS_MODES[i % 3];
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.maxLod = VK_LOD_CLAMP_NONE;
        if (!made(vk.vkCreateSampler(device, &info, nullptr, &y.samplers[i]),
                  y.samplers[i], "vkCreateSampler", why)) {
            return false;
        }
        sampling[i] = {y.samplers[i], VK_NULL_HANDLE,
                       VK_IMAGE_LAYOUT_UNDEFINED};
    }
    samplers.dstSet = y.texture_set;
    samplers.dstBinding = 1;
    samplers.descriptorCount = RAY_SAMPLERS;
    samplers.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    samplers.pImageInfo = sampling;
    vk.vkUpdateDescriptorSets(device, 1, &samplers, 0, nullptr);
    /* the enclosure measure, bound in the images' descriptor set from the
     * start. only the shaders read and write it. the first traced picture is a
     * cut, so the measure is written before it is read */
    if (!make_buffer(y.enclosure, sizeof(float),
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, why, true)) {
        return false;
    }
    VkDescriptorBufferInfo enclosure{y.enclosure.buffer, 0, VK_WHOLE_SIZE};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

    write.dstSet = y.image_set;
    write.dstBinding = ENCLOSURE_BINDING;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo = &enclosure;
    vk.vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    if (!y.make_scratch(*this, y.scratch, SCRATCH, y.scratch_address, why)) {
        return false;
    }
    y.scratch_room = SCRATCH;
    return true;
}

/* free everything ray tracing holds, once the device is idle */
void Raster::Impl::free_rays()
{
    if (rays == nullptr) {
        return;
    }
    Rays &y = *rays;

    free_ray_target();
    for (Mesh &m : y.meshes) {
        vk.vkDestroyAccelerationStructureKHR(device, m.structure, nullptr);
    }
    vk.vkDestroyAccelerationStructureKHR(device, y.top, nullptr);
    free_buffer(y.top_buffer);
    free_buffer(y.top_scratch);
    free_buffer(y.scratch);
    free_buffer(y.enclosure);
    for (Block &block : y.bytes.blocks) {
        free_buffer(block.buffer);
    }
    for (Block &block : y.structures.blocks) {
        free_buffer(block.buffer);
    }
    for (VkSampler sampler : y.samplers) {
        vk.vkDestroySampler(device, sampler, nullptr);
    }
    for (VkPipeline pipeline : y.passes) {
        vk.vkDestroyPipeline(device, pipeline, nullptr);
    }
    vk.vkDestroyPipeline(device, y.show, nullptr);
    for (VkShaderModule module : y.modules) {
        vk.vkDestroyShaderModule(device, module, nullptr);
    }
    vk.vkDestroyDescriptorPool(device, y.pool, nullptr);
    vk.vkDestroyPipelineLayout(device, y.layout, nullptr);
    vk.vkDestroyPipelineLayout(device, y.shown_layout, nullptr);
    vk.vkDestroyDescriptorSetLayout(device, y.frame_layout, nullptr);
    vk.vkDestroyDescriptorSetLayout(device, y.texture_layout, nullptr);
    vk.vkDestroyDescriptorSetLayout(device, y.image_layout, nullptr);
    vk.vkDestroyDescriptorSetLayout(device, y.shown_set_layout, nullptr);
    delete rays;
    rays = nullptr;
}

void Raster::Impl::drop_rays()
{
    delete rays;
    rays = nullptr;
}

/* a frame's descriptor set, uniform buffer and ramps. its other buffers are
 * made when a picture first needs them */
bool Raster::Impl::make_ray_frame(Frame &f, std::string &why)
{
    Rays &y = *rays;
    VkDescriptorSetAllocateInfo alloc{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};

    alloc.descriptorPool = y.pool;
    alloc.descriptorSetCount = 1;
    alloc.pSetLayouts = &y.frame_layout;
    if (!made(vk.vkAllocateDescriptorSets(device, &alloc, &f.ray_set),
              f.ray_set, "vkAllocateDescriptorSets", why) ||
        !make_buffer(f.ray_uniform, sizeof(Constants),
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, why) ||
        !make_buffer(f.ray_ramps, RAMPS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     why)) {
        return false;
    }
    VkDescriptorBufferInfo buffers[2] = {
        {f.uniform.buffer, 0, sizeof(PictureUniform)},
        {f.ray_uniform.buffer, 0, sizeof(Constants)}};
    VkDescriptorBufferInfo shades{f.ray_ramps.buffer, 0, RAMPS};
    VkWriteDescriptorSet writes[2] = {{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET},
                                      {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}};

    writes[0].dstSet = f.ray_set;
    writes[0].dstBinding = 1;
    writes[0].descriptorCount = 2;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = buffers;
    writes[1].dstSet = f.ray_set;
    writes[1].dstBinding = 5;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &shades;
    vk.vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
    return true;
}

void Raster::Impl::free_ray_frame(Frame &f)
{
    free_buffer(f.ray_places);
    free_buffer(f.ray_instances);
    free_buffer(f.ray_materials);
    free_buffer(f.ray_uniform);
    free_buffer(f.ray_ramps);
    free_buffer(f.ray_lamps);
}

void Raster::Impl::free_ray_target()
{
    Rays &y = *rays;

    for (uint32_t i = 0; i < IMAGES; i++) {
        vk.vkDestroyImageView(device, y.views[i], nullptr);
        vk.vkDestroyImage(device, y.images[i], nullptr);
        vk.vkFreeMemory(device, y.memories[i], nullptr);
        y.views[i] = VK_NULL_HANDLE;
        y.images[i] = VK_NULL_HANDLE;
        y.memories[i] = VK_NULL_HANDLE;
    }
}

bool Raster::Impl::make_ray_target(VkExtent2D size, VkCommandBuffer c,
                                   std::string &why)
{
    Rays &y = *rays;
    VkDescriptorImageInfo named[IMAGES];
    uint32_t at = 0;

    y.traced = {std::max(static_cast<uint32_t>(ceilf(
                             static_cast<float>(size.width) * render_scale)),
                         1U),
                std::max(static_cast<uint32_t>(ceilf(
                             static_cast<float>(size.height) * render_scale)),
                         1U)};
    for (const Held &held : HELD) {
        VkExtent2D its = at_target(held.binding) ? size : y.traced;

        for (uint32_t i = 0; i < held.count; i++, at++) {
            if (!make_image(y.images[at], y.memories[at], held.format,
                            its.width, its.height, 1,
                            VK_IMAGE_USAGE_STORAGE_BIT, why) ||
                !make_view(y.views[at], y.images[at], held.format,
                           VK_IMAGE_ASPECT_COLOR_BIT, 1, why)) {
                free_ray_target();
                return false;
            }
            named[at] = {VK_NULL_HANDLE, y.views[at], VK_IMAGE_LAYOUT_GENERAL};
        }
    }
    VkDescriptorImageInfo under[2] = {named[IMAGE_SHOWN], named[IMAGE_DEPTH]};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.dstSet = y.image_set;
    at = 0;
    for (const Held &held : HELD) {
        write.dstBinding = held.binding;
        write.descriptorCount = held.count;
        write.pImageInfo = &named[at];
        vk.vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        at += held.count;
    }
    write.dstSet = y.shown_set;
    write.dstBinding = 0;
    write.descriptorCount = 2;
    write.pImageInfo = under;
    vk.vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    for (VkImage image : y.images) {
        barrier(c, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 0,
                1, 0, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    }
    y.stale = true;
    return true;
}

void Raster::Impl::ray_skipped()
{
    rays->stale = true;
}

uint32_t Raster::Impl::ray_add_mesh(const RayMesh &mesh, std::string &why)
{
    Rays &y = *rays;
    uint32_t triangles = static_cast<uint32_t>(mesh.corners.size() / 3);
    VkDeviceSize points_at = 0;
    VkDeviceSize indices_at = up(mesh.count * POINT, 16);
    VkDeviceSize corners_at = indices_at + up(triangles * TRIANGLE, 16);
    VkDeviceSize bytes = corners_at + up(triangles * CORNER, 16);

    if (!broken.empty()) {
        why = broken;
        return 0;
    }
    if (triangles == 0 || mesh.count == 0 ||
        mesh.uv.size() != mesh.corners.size() * 2 ||
        mesh.materials.size() != triangles) {
        why = "a mesh for ray tracing has no complete triangle";
        return 0;
    }
    VkAccelerationStructureGeometryKHR geometry = triangles_of(mesh.count);
    VkAccelerationStructureBuildGeometryInfoKHR build = build_of(&geometry);
    VkAccelerationStructureBuildSizesInfoKHR sizes{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};

    vk.vkGetAccelerationStructureBuildSizesKHR(
        device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build,
        &triangles, &sizes);
    VkDeviceSize scratch = up(sizes.buildScratchSize, y.scratch_align);

    /* the mesh's data is staged with the load, and its build is queued on the
     * load with a span of the scratch buffer. the load is flushed first where
     * either buffer would overflow */
    if ((staged + bytes > staging.size ||
         y.scratched + scratch > y.scratch_room) &&
        !flush(why)) {
        return 0;
    }
    if (bytes > staging.size) {
        free_buffer(staging);
        if (!make_buffer(staging, bytes > STAGING ? bytes : STAGING,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT, why)) {
            return 0;
        }
    }
    if (scratch > y.scratch_room) {
        y.scratch_room = 0;
        if (!y.make_scratch(*this, y.scratch, scratch, y.scratch_address,
                            why)) {
            return 0;
        }
        y.scratch_room = scratch;
    }
    /* the mesh is complete before the load refers to it or it joins the meshes,
     * so one that the device refuses changes neither */
    VkCommandBuffer c = load(why);
    Mesh one;
    VkAccelerationStructureCreateInfoKHR made_of{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};

    if (c == VK_NULL_HANDLE ||
        !y.take(*this, y.bytes, bytes, 16, one.bytes, why) ||
        !y.take(*this, y.structures, sizes.accelerationStructureSize, 256,
                one.room, why)) {
        y.give(y.bytes, one.bytes);
        return 0;
    }
    made_of.buffer = y.structures.blocks[one.room.block].buffer.buffer;
    made_of.offset = one.room.at;
    made_of.size = sizes.accelerationStructureSize;
    made_of.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (!made(vk.vkCreateAccelerationStructureKHR(device, &made_of, nullptr,
                                                  &one.structure),
              one.structure, "vkCreateAccelerationStructureKHR", why)) {
        y.give(y.bytes, one.bytes);
        y.give(y.structures, one.room);
        return 0;
    }
    auto *to = static_cast<uint8_t *>(staging.mapped) + staged;
    auto *point = reinterpret_cast<float *>(to + points_at);
    auto *corner = reinterpret_cast<float *>(to + corners_at);

    memset(to, 0, bytes);
    for (uint32_t i = 0; i < mesh.count; i++) {
        memcpy(&point[i * 6], &mesh.points[i * 3], 3 * sizeof(float));
        if (mesh.normals != nullptr) {
            memcpy(&point[i * 6 + 3], &mesh.normals[i * 3], 3 * sizeof(float));
        }
    }
    memcpy(to + indices_at, mesh.corners.data(), triangles * TRIANGLE);
    for (uint32_t i = 0; i < triangles; i++) {
        uint32_t material = mesh.materials[i];

        memcpy(&corner[i * 8], &mesh.uv[i * 6], 6 * sizeof(float));
        memcpy(&corner[i * 8 + 6], &material, sizeof material);
    }
    const Block &block = y.bytes.blocks[one.bytes.block];
    VkBufferCopy copy{staged, one.bytes.at, bytes};
    VkAccelerationStructureDeviceAddressInfoKHR of{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    uint32_t index = static_cast<uint32_t>(y.meshes.size());

    one.points = block.address + one.bytes.at + points_at;
    one.indices = block.address + one.bytes.at + indices_at;
    one.corners = block.address + one.bytes.at + corners_at;
    vk.vkCmdCopyBuffer(c, staging.buffer, block.buffer.buffer, 1, &copy);
    of.accelerationStructure = one.structure;
    one.address = vk.vkGetAccelerationStructureDeviceAddressKHR(device, &of);
    one.load = loads;
    staged += bytes;
    if (!y.free_meshes.empty()) {
        index = y.free_meshes.back();
        y.free_meshes.pop_back();
        y.meshes[index] = one;
    } else {
        y.meshes.push_back(one);
    }
    /* the build is deferred to the load's end, where one command runs them
     * all */
    y.builds.push_back(Build{index, mesh.count, triangles, y.scratched});
    y.scratched += scratch;
    return index;
}

void Raster::Impl::ray_build(VkCommandBuffer c)
{
    Rays &y = *rays;
    uint32_t count = static_cast<uint32_t>(y.builds.size());
    VkMemoryBarrier before{VK_STRUCTURE_TYPE_MEMORY_BARRIER};

    y.scratched = 0;
    if (count == 0) {
        return;
    }
    y.geometries.resize(count);
    y.infos.resize(count);
    y.ranges.resize(count);
    y.ranged.resize(count);
    for (uint32_t i = 0; i < count; i++) {
        const Build &owed = y.builds[i];
        const Mesh &m = y.meshes[owed.mesh];
        VkAccelerationStructureGeometryKHR &geometry = y.geometries[i];
        VkAccelerationStructureBuildGeometryInfoKHR &build = y.infos[i];

        geometry = triangles_of(owed.points);
        geometry.geometry.triangles.vertexData.deviceAddress = m.points;
        geometry.geometry.triangles.indexData.deviceAddress = m.indices;
        build = build_of(&geometry);
        build.dstAccelerationStructure = m.structure;
        build.scratchData.deviceAddress = y.scratch_address + owed.scratch;
        y.ranges[i] = {owed.triangles, 0, 0, 0};
        y.ranged[i] = &y.ranges[i];
    }
    /* the builds read what the load's copies wrote */
    before.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    before.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vk.vkCmdPipelineBarrier(
        c, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &before,
        0, nullptr, 0, nullptr);
    vk.vkCmdBuildAccelerationStructuresKHR(c, count, y.infos.data(),
                                           y.ranged.data());
    y.builds.clear();
}

void Raster::Impl::ray_free_mesh(uint32_t mesh)
{
    Rays &y = *rays;
    Mesh &m = y.meshes[mesh];
    std::string unsent;

    /* the load being recorded may still build it, so the load is flushed first.
     * a broken raster's device may still read the mesh, which then stays until
     * the raster is freed */
    if (loading && m.load == loads) {
        flush(unsent);
    }
    if (!broken.empty() || m.structure == VK_NULL_HANDLE) {
        return;
    }
    vk.vkDestroyAccelerationStructureKHR(device, m.structure, nullptr);
    y.give(y.bytes, m.bytes);
    y.give(y.structures, m.room);
    m = Mesh{};
    y.free_meshes.push_back(mesh);
}

void Raster::Impl::ray_free_texture(uint32_t texture)
{
    if (texture < rays->bound.size()) {
        rays->bound[texture] = VK_NULL_HANDLE;
    }
}

/* replace the top-level structure with one that has capacity for room
 * instances, once no picture is in flight, and bind it in each frame's
 * descriptor set */
bool Raster::Impl::Rays::make_top(Impl &r, uint32_t room, std::string &why)
{
    Calls &vk = r.vk;
    VkAccelerationStructureGeometryKHR geometry;
    VkAccelerationStructureBuildGeometryInfoKHR build;
    VkAccelerationStructureBuildSizesInfoKHR sizes{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    VkAccelerationStructureCreateInfoKHR made_of{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};

    top_build(geometry, build);
    vk.vkGetAccelerationStructureBuildSizesKHR(
        r.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build,
        &room, &sizes);
    if (!r.took(vk.vkDeviceWaitIdle(r.device), "vkDeviceWaitIdle", why)) {
        return false;
    }
    vk.vkDestroyAccelerationStructureKHR(r.device, top, nullptr);
    top = VK_NULL_HANDLE;
    top_room = 0;
    r.free_buffer(top_buffer);
    if (!r.make_buffer(top_buffer, sizes.accelerationStructureSize,
                       VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                           VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                       why, true) ||
        !make_scratch(r, top_scratch, sizes.buildScratchSize,
                      top_scratch_address, why)) {
        return false;
    }
    made_of.buffer = top_buffer.buffer;
    made_of.size = sizes.accelerationStructureSize;
    made_of.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    if (!r.made(vk.vkCreateAccelerationStructureKHR(r.device, &made_of, nullptr,
                                                    &top),
                top, "vkCreateAccelerationStructureKHR", why)) {
        return false;
    }
    VkWriteDescriptorSetAccelerationStructureKHR named{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

    named.accelerationStructureCount = 1;
    named.pAccelerationStructures = &top;
    write.pNext = &named;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    for (Frame &f : r.frames) {
        write.dstSet = f.ray_set;
        vk.vkUpdateDescriptorSets(r.device, 1, &write, 0, nullptr);
    }
    top_room = room;
    return true;
}

bool Raster::Impl::ray_ready(Frame &f, const RayPicture &p, std::string &why)
{
    Rays &y = *rays;
    uint32_t most = static_cast<uint32_t>(p.instances.size());
    uint32_t room = most != 0 ? most : 1;
    VkDeviceSize materials =
        p.materials.empty() ? 1 : static_cast<VkDeviceSize>(p.materials.size());
    bool placed = false;
    bool instanced = false;
    bool listed = false;
    bool lamped = false;

    if (room > y.top_room &&
        !y.make_top(*this, room * 2 > FIRST_ROOM ? room * 2 : FIRST_ROOM,
                    why)) {
        return false;
    }
    if (!y.grown(
            *this, f.ray_places,
            room * sizeof(VkAccelerationStructureInstanceKHR),
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            placed, why)) {
        return false;
    }
    /* the top-level build reads the instances by address. the address is taken
     * as soon as the buffer is made, so it is right for the next picture even
     * if this one is refused below */
    if (placed) {
        f.ray_address = y.address(*this, f.ray_places);
    }
    if (!y.grown(*this, f.ray_instances, room * sizeof(InstanceWords),
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, instanced, why) ||
        !y.grown(*this, f.ray_materials, materials * sizeof(MaterialWords),
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, listed, why)) {
        return false;
    }
    /* the picture's lamps, as the hash grid the trace pass reads */
    lamp_table(p.lamps, p.patches, y.lamp_words);
    if (!y.grown(*this, f.ray_lamps, y.lamp_words.size() * sizeof(uint32_t),
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, lamped, why)) {
        return false;
    }
    if (lamped) {
        VkDescriptorBufferInfo buffer{f.ray_lamps.buffer, 0, VK_WHOLE_SIZE};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

        write.dstSet = f.ray_set;
        write.dstBinding = 6;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo = &buffer;
        vk.vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
    if (instanced || listed) {
        VkDescriptorBufferInfo buffers[2] = {
            {f.ray_instances.buffer, 0, VK_WHOLE_SIZE},
            {f.ray_materials.buffer, 0, VK_WHOLE_SIZE}};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

        write.dstSet = f.ray_set;
        write.dstBinding = 3;
        write.descriptorCount = 2;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo = buffers;
        vk.vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
    /* bind each texture that has changed at its index. no picture in flight
     * uses an index whose texture has changed */
    for (uint32_t i = 0; i < textures.size() && i < RAY_TEXTURES; i++) {
        if (textures[i].view == VK_NULL_HANDLE ||
            textures[i].view == y.bound[i]) {
            continue;
        }
        VkDescriptorImageInfo image{VK_NULL_HANDLE, textures[i].view,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

        write.dstSet = y.texture_set;
        write.dstBinding = 0;
        write.dstArrayElement = i;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.pImageInfo = &image;
        vk.vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        y.bound[i] = textures[i].view;
    }
    auto *list = static_cast<MaterialWords *>(f.ray_materials.mapped);

    for (size_t i = 0; i < p.materials.size(); i++) {
        const RayMaterial &m = p.materials[i];
        MaterialWords &w = list[i];
        bool live = m.texture < textures.size() && m.texture < RAY_TEXTURES &&
                    textures[m.texture].view != VK_NULL_HANDLE;

        w = MaterialWords{};
        w.sampling = sampler_place(m.sample);
        w.texture = live ? m.texture : 0;
        w.flags = m.flags;
        w.color = m.color;
        w.specular = m.specular;
        w.gloss = m.gloss;
        w.blend = (m.src & 7U) | (m.dst & 7U) << 3;
    }
    auto *places =
        static_cast<VkAccelerationStructureInstanceKHR *>(f.ray_places.mapped);
    auto *words = static_cast<InstanceWords *>(f.ray_instances.mapped);
    uint32_t count = 0;

    for (const RayInstance &in : p.instances) {
        const float *m = in.object_to_world;
        float turn = m[0] * (m[5] * m[10] - m[6] * m[9]) -
                     m[1] * (m[4] * m[10] - m[6] * m[8]) +
                     m[2] * (m[4] * m[9] - m[5] * m[8]);

        /* an instance is left out where its mesh has no acceleration structure
         * or its matrix is singular. so is a backdrop that the picture didn't
         * draw, which its mask would let camera rays hit */
        if (in.mesh == 0 || in.mesh >= y.meshes.size() ||
            y.meshes[in.mesh].structure == VK_NULL_HANDLE ||
            !(fabsf(turn) > 1e-30F) ||
            (in.flags & (RAY_BACKDROP | RAY_SEEN)) == RAY_BACKDROP) {
            continue;
        }
        const Mesh &mesh = y.meshes[in.mesh];
        VkAccelerationStructureInstanceKHR &place = places[count];
        InstanceWords &w = words[count];

        place = VkAccelerationStructureInstanceKHR{};
        memcpy(&place.transform, m, sizeof place.transform);
        place.instanceCustomIndex = count;
        place.mask = in.flags & RAY_BACKDROP ? MASK_BACKDROP
                     : in.flags & RAY_SEEN   ? MASK_SEEN
                                             : MASK_HIDDEN;
        if (!(in.flags & (RAY_BACKDROP | RAY_TRANSLUCENT))) {
            place.mask |=
                in.flags & RAY_SCENERY ? MASK_SOLID : MASK_SOLID | MASK_OBJECT;
        }
        place.flags =
            VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR |
            (in.flags & RAY_OPAQUE ? VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR
                                   : 0);
        place.accelerationStructureReference = mesh.address;
        w = InstanceWords{};
        memcpy(w.to_world, m, sizeof w.to_world);
        memcpy(w.before, in.before, sizeof w.before);
        memcpy(w.light_way, in.light_way, sizeof in.light_way);
        w.points = mesh.points;
        w.indices = mesh.indices;
        w.corners = mesh.corners;
        w.materials = in.materials;
        w.flags = in.flags;
        w.light = in.light;
        w.ambient = in.ambient;
        w.diffuse = in.diffuse;
        count++;
    }
    y.count = count;
    Constants constants{};

    memcpy(constants.eye, p.eye, sizeof constants.eye);
    memcpy(constants.to_sun, p.to_sun, sizeof p.to_sun);
    for (uint32_t i = 0; i < 3; i++) {
        constants.back[i][0] = ((p.back[i] >> 16) & 0xff) / 255.0F;
        constants.back[i][1] = ((p.back[i] >> 8) & 0xff) / 255.0F;
        constants.back[i][2] = (p.back[i] & 0xff) / 255.0F;
    }
    /* ray_trace advances the picture count and the history's state once the
     * picture's commands are recorded, so a picture that is refused before then
     * can be prepared again */
    y.samples = p.samples != 0 ? p.samples : 1;
    constants.counts[0] = y.pictures;
    constants.counts[2] = y.samples;
    constants.counts[3] =
        p.cut || y.stale || (y.samples < UNFILTERED && !y.filtered) ? 1 : 0;
    constants.clock[0] = static_cast<float>(p.time);
    constants.clock[1] = p.step;
    constants.lamp[0] = LAMP_FLOOR;
    constants.lamp[1] = LAMP_REACH;
    memcpy(f.ray_lamps.mapped, y.lamp_words.data(),
           y.lamp_words.size() * sizeof(uint32_t));
    memcpy(constants.before_view, p.before_view, sizeof constants.before_view);
    memcpy(constants.before_project, p.before_project,
           sizeof constants.before_project);
    constants.size[0] = static_cast<float>(y.traced.width);
    constants.size[1] = static_cast<float>(y.traced.height);
    constants.size[2] = static_cast<float>(WIDTH) / constants.size[0];
    constants.size[3] = static_cast<float>(HEIGHT) / constants.size[1];
    constants.shown[0] = static_cast<float>(target.width);
    constants.shown[1] = static_cast<float>(target.height);
    constants.shown[2] = static_cast<float>(WIDTH) / constants.shown[0];
    constants.shown[3] = static_cast<float>(HEIGHT) / constants.shown[1];
    /* each picture's camera rays cross their pixels at the next point of a
     * Halton sequence, for the antialiasing pass. the picture before is the
     * one traced before this */
    constants.jitter[0] = halton(y.pictures % JITTERS + 1, 2) - 0.5F;
    constants.jitter[1] = halton(y.pictures % JITTERS + 1, 3) - 0.5F;
    constants.jitter[2] = halton((y.pictures + JITTERS - 1) % JITTERS + 1, 2) -
                          0.5F;
    constants.jitter[3] = halton((y.pictures + JITTERS - 1) % JITTERS + 1, 3) -
                          0.5F;
    memcpy(f.ray_uniform.mapped, &constants, sizeof constants);
    static_assert(sizeof p.ramps == RAMPS, "the shaders' Ramps");
    memcpy(f.ray_ramps.mapped, p.ramps, RAMPS);
    return true;
}

void Raster::Impl::ray_trace(VkCommandBuffer c, Frame &f)
{
    Rays &y = *rays;
    VkAccelerationStructureGeometryKHR geometry;
    VkAccelerationStructureBuildGeometryInfoKHR build;
    VkAccelerationStructureBuildRangeInfoKHR range{y.count, 0, 0, 0};
    const VkAccelerationStructureBuildRangeInfoKHR *ranges = &range;
    VkMemoryBarrier after{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    VkDescriptorSet sets[3] = {f.ray_set, y.texture_set, y.image_set};
    /* each pass reads what the pass before it wrote, and runs over the traced
     * picture's pixels, or the antialiasing pass over the target's */
    auto next = [&](uint32_t pass) {
        VkExtent2D over = pass == PASS_TAA ? target : y.traced;

        vk.vkCmdPipelineBarrier(c, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1,
                                &after, 0, nullptr, 0, nullptr);
        vk.vkCmdBindPipeline(c, VK_PIPELINE_BIND_POINT_COMPUTE, y.passes[pass]);
        vk.vkCmdDispatch(c, (over.width + GROUP - 1) / GROUP,
                         (over.height + GROUP - 1) / GROUP, 1);
    };

    y.pictures++;
    y.stale = false;
    y.filtered = y.samples < UNFILTERED;
    top_build(geometry, build);
    geometry.geometry.instances.data.deviceAddress = f.ray_address;
    build.dstAccelerationStructure = y.top;
    build.scratchData.deviceAddress = y.top_scratch_address;
    vk.vkCmdBuildAccelerationStructuresKHR(c, 1, &build, &ranges);
    after.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    after.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    vk.vkCmdPipelineBarrier(
        c, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &after, 0, nullptr, 0,
        nullptr);
    vk.vkCmdBindDescriptorSets(c, VK_PIPELINE_BIND_POINT_COMPUTE, y.layout, 0,
                               3, sets, 0, nullptr);
    /* the enclosure pass runs first, since the trace pass reads its measure */
    vk.vkCmdBindPipeline(c, VK_PIPELINE_BIND_POINT_COMPUTE,
                         y.passes[PASS_ENCLOSURE]);
    vk.vkCmdDispatch(c, 1, 1, 1);
    after.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    after.dstAccessMask =
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    next(PASS_TRACE);
    /* below UNFILTERED rays a pixel the picture is denoised: the temporal pass,
     * then the a-trous iterations with doubling stride */
    if (y.samples < UNFILTERED) {
        next(PASS_TEMPORAL);
        for (int32_t i = 0; i < STEPS; i++) {
            Pass step{1 << i, i & 1};

            vk.vkCmdPushConstants(c, y.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                                  sizeof step, &step);
            next(PASS_ATROUS);
        }
    }
    next(PASS_COMPOSE);
    next(PASS_TAA);
    /* the fragment shader that copies the picture reads what the antialiasing
     * pass wrote */
    vk.vkCmdPipelineBarrier(c, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 1, &after,
                            0, nullptr, 0, nullptr);
}

void Raster::Impl::ray_show(VkCommandBuffer c)
{
    Rays &y = *rays;

    vk.vkCmdBindPipeline(c, VK_PIPELINE_BIND_POINT_GRAPHICS, y.show);
    vk.vkCmdBindDescriptorSets(c, VK_PIPELINE_BIND_POINT_GRAPHICS,
                               y.shown_layout, 0, 1, &y.shown_set, 0, nullptr);
    vk.vkCmdDraw(c, 3, 1, 0, 0);
}

} // namespace pc
