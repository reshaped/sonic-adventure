#define VK_NO_PROTOTYPES
#include "raster.h"

#include <math.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
/* minwindef.h defines near and far as nothing, which would erase
 * sa_engine.h's fields of those names */
#undef near
#undef far
#else
#include <dlfcn.h>
#endif

#include <map>
#include <tuple>

#include <vulkan/vulkan_core.h>

#include "area.comp.h"
#include "draw.frag.h"
#include "draw.vert.h"
#include "resolve.comp.h"

/* a create info names its type and leaves the rest zero, which -Wextra
 * would call a missing initializer */
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

namespace pc {

namespace {

/* Vulkan's loader, which the raster opens as it starts */
#ifdef _WIN32
#define LOADER "vulkan-1.dll"
void *loader_open()
{
    return reinterpret_cast<void *>(LoadLibraryA(LOADER));
}
void *loader_find(void *loader, const char *name)
{
    return reinterpret_cast<void *>(
        GetProcAddress(static_cast<HMODULE>(loader), name));
}
void loader_close(void *loader)
{
    FreeLibrary(static_cast<HMODULE>(loader));
}
#else
#define LOADER "libvulkan.so.1"
void *loader_open()
{
    return dlopen(LOADER, RTLD_NOW | RTLD_LOCAL);
}
void *loader_find(void *loader, const char *name)
{
    return dlsym(loader, name);
}
void loader_close(void *loader)
{
    dlclose(loader);
}
#endif

/* the calls the raster makes, which open takes from the loader.
 * SURFACE_CALLS and SWAPCHAIN_CALLS are a window's: an instance and a device
 * made for none lack them */
#define GLOBAL_CALLS(X) X(vkCreateInstance)
#define INSTANCE_CALLS(X)                                                      \
    X(vkDestroyInstance)                                                       \
    X(vkEnumeratePhysicalDevices)                                              \
    X(vkGetPhysicalDeviceProperties)                                           \
    X(vkGetPhysicalDeviceQueueFamilyProperties)                                \
    X(vkGetPhysicalDeviceMemoryProperties)                                     \
    X(vkGetPhysicalDeviceFeatures)                                             \
    X(vkCreateDevice)                                                          \
    X(vkGetDeviceProcAddr)                                                     \
    X(vkEnumerateDeviceExtensionProperties)
#define SURFACE_CALLS(X)                                                       \
    X(vkDestroySurfaceKHR)                                                     \
    X(vkGetPhysicalDeviceSurfaceSupportKHR)                                    \
    X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)                               \
    X(vkGetPhysicalDeviceSurfaceFormatsKHR)                                    \
    X(vkGetPhysicalDeviceSurfacePresentModesKHR)
#define DEVICE_CALLS(X)                                                        \
    X(vkDestroyDevice)                                                         \
    X(vkGetDeviceQueue)                                                        \
    X(vkCreateCommandPool)                                                     \
    X(vkDestroyCommandPool)                                                    \
    X(vkAllocateCommandBuffers)                                                \
    X(vkFreeCommandBuffers)                                                    \
    X(vkBeginCommandBuffer)                                                    \
    X(vkEndCommandBuffer)                                                      \
    X(vkQueueSubmit)                                                           \
    X(vkDeviceWaitIdle)                                                        \
    X(vkCreateImage)                                                           \
    X(vkDestroyImage)                                                          \
    X(vkCreateImageView)                                                       \
    X(vkDestroyImageView)                                                      \
    X(vkGetImageMemoryRequirements)                                            \
    X(vkBindImageMemory)                                                       \
    X(vkCreateBuffer)                                                          \
    X(vkDestroyBuffer)                                                         \
    X(vkGetBufferMemoryRequirements)                                           \
    X(vkBindBufferMemory)                                                      \
    X(vkAllocateMemory)                                                        \
    X(vkFreeMemory)                                                            \
    X(vkMapMemory)                                                             \
    X(vkUnmapMemory)                                                           \
    X(vkCreateRenderPass)                                                      \
    X(vkDestroyRenderPass)                                                     \
    X(vkCreateFramebuffer)                                                     \
    X(vkDestroyFramebuffer)                                                    \
    X(vkCreateShaderModule)                                                    \
    X(vkDestroyShaderModule)                                                   \
    X(vkCreateDescriptorSetLayout)                                             \
    X(vkDestroyDescriptorSetLayout)                                            \
    X(vkCreatePipelineLayout)                                                  \
    X(vkDestroyPipelineLayout)                                                 \
    X(vkCreateGraphicsPipelines)                                               \
    X(vkDestroyPipeline)                                                       \
    X(vkCreateDescriptorPool)                                                  \
    X(vkDestroyDescriptorPool)                                                 \
    X(vkAllocateDescriptorSets)                                                \
    X(vkUpdateDescriptorSets)                                                  \
    X(vkCreateSampler)                                                         \
    X(vkDestroySampler)                                                        \
    X(vkCmdBeginRenderPass)                                                    \
    X(vkCmdEndRenderPass)                                                      \
    X(vkCmdBindPipeline)                                                       \
    X(vkCmdBindDescriptorSets)                                                 \
    X(vkCmdBindVertexBuffers)                                                  \
    X(vkCmdPushConstants)                                                      \
    X(vkCmdDraw)                                                               \
    X(vkCmdSetViewport)                                                        \
    X(vkCmdSetScissor)                                                         \
    X(vkCmdPipelineBarrier)                                                    \
    X(vkCmdCopyBufferToImage)                                                  \
    X(vkCmdCopyImageToBuffer)                                                  \
    X(vkCmdBlitImage)                                                          \
    X(vkCmdClearColorImage)                                                    \
    X(vkCmdDispatch)                                                           \
    X(vkCreateComputePipelines)                                                \
    X(vkFreeDescriptorSets)                                                    \
    X(vkCmdUpdateBuffer)                                                       \
    X(vkCmdCopyBuffer)                                                         \
    X(vkCreateFence)                                                           \
    X(vkDestroyFence)                                                          \
    X(vkResetFences)                                                           \
    X(vkWaitForFences)                                                         \
    X(vkGetFenceStatus)                                                        \
    X(vkCreateSemaphore)                                                       \
    X(vkDestroySemaphore)                                                      \
    X(vkCreateQueryPool)                                                       \
    X(vkDestroyQueryPool)                                                      \
    X(vkCmdResetQueryPool)                                                     \
    X(vkCmdWriteTimestamp)                                                     \
    X(vkGetQueryPoolResults)
#define SWAPCHAIN_CALLS(X)                                                     \
    X(vkCreateSwapchainKHR)                                                    \
    X(vkDestroySwapchainKHR)                                                   \
    X(vkGetSwapchainImagesKHR)                                                 \
    X(vkAcquireNextImageKHR)                                                   \
    X(vkQueuePresentKHR)

#define DECLARE(name) PFN_##name name = nullptr;
struct Calls {
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
    GLOBAL_CALLS(DECLARE)
    INSTANCE_CALLS(DECLARE)
    SURFACE_CALLS(DECLARE)
    DEVICE_CALLS(DECLARE)
    SWAPCHAIN_CALLS(DECLARE)
};
#undef DECLARE

struct Buffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void *mapped = nullptr;
    VkDeviceSize size = 0;
};

/* textures take slots cut from blocks of BLOCK bytes. a block is allocated
 * only where no free slot fits. a slot's bytes are the least power of two that
 * holds the texture and its alignment. a texture whose slot would pass a
 * quarter of a block allocates memory of its own. a block lives as long as
 * the raster */
constexpr VkDeviceSize BLOCK = VkDeviceSize{4} << 20;
struct Slot {
    VkDeviceMemory block = VK_NULL_HANDLE;
    VkDeviceSize at = 0;
    uint32_t type = 0; /* the block's memory type */
    uint32_t size = 0; /* log2 of the slot's bytes */
};

struct Texture {
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE; /* its own, or none for a slot */
    Slot slot;
    uint64_t load = 0; /* the number of the load that sends its texels */
};

/* the staging's least size. a texture larger than the staging makes
 * it again at the texture's size */
constexpr VkDeviceSize STAGING = VkDeviceSize{16} << 20;

/* the times waited and idle make a wait before they give it up. each makes
 * it again only where it fails for want of memory */
constexpr int WAITS = 4;

/* what one picture in flight owns. the target and the lists are shared, as
 * one queue runs the pictures in turn */
struct Frame {
    VkCommandBuffer commands = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    /* a picture went to the device on the fence, which signals as it
     * retires. else the frame holds none, and nothing signals the fence */
    bool sent = false;
    VkSemaphore acquired = VK_NULL_HANDLE;
    VkDescriptorSet picture_set = VK_NULL_HANDLE;
    Buffer uniform;
    Buffer vertices;
    Buffer readback;
    Buffer stats; /* the fragments' head as the picture left it */
};

/* the words of the fragments' head, as the shaders' Fragments holds them,
 * and its bytes. the head lies ahead of the fragments in their buffer */
enum { HEAD_COUNT, HEAD_CAPACITY, HEAD_LOST, HEAD_CUT, HEAD_WORDS };
constexpr VkDeviceSize LIST_HEAD = HEAD_WORDS * sizeof(uint32_t);
/* a batch's push constants, as the shaders' Batch holds them. blend holds
 * the source's blend instruction, then the destination's, three bits each */
struct Push {
    uint32_t flags;
    uint32_t blend;
};
/* a fragment's bytes, as the shaders' Fragment. FRAGMENTS_A_PIXEL is the
 * lists' room, in fragments a pixel of the target on average */
constexpr VkDeviceSize FRAGMENT = 16;
constexpr VkDeviceSize FRAGMENTS_A_PIXEL = 16;
/* the compute shaders' local size, each way */
constexpr uint32_t GROUP = 8;
/* the most sets, each a texture under a sampler, live at once */
constexpr uint32_t TEXTURE_SETS = 8192;

/* an image the area filter writes from a larger target. readout takes the
 * picture at WIDTH by HEIGHT for read, and fitted at the window's region */
struct Scaled {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkDescriptorSet set = VK_NULL_HANDLE;
    VkExtent2D extent{};
};

/* a PowerVR blend instruction as Vulkan's factor for the source where
 * source, else for the destination. 2 and 3 take the other one's color */
VkBlendFactor factor(uint32_t instruction, bool source, bool alpha)
{
    switch (instruction) {
    case 0:
        return VK_BLEND_FACTOR_ZERO;
    case 1:
        return VK_BLEND_FACTOR_ONE;
    case 2:
        return source ? (alpha ? VK_BLEND_FACTOR_DST_ALPHA
                               : VK_BLEND_FACTOR_DST_COLOR)
                      : (alpha ? VK_BLEND_FACTOR_SRC_ALPHA
                               : VK_BLEND_FACTOR_SRC_COLOR);
    case 3:
        return source ? (alpha ? VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA
                               : VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR)
                      : (alpha ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA
                               : VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR);
    case 4:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case 5:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case 6:
        return VK_BLEND_FACTOR_DST_ALPHA;
    default:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    }
}

/* the PowerVR's compare modes, on 1/z, which the depth holds as is */
VkCompareOp compare(uint32_t mode)
{
    static const VkCompareOp ops[8] = {VK_COMPARE_OP_NEVER,
                                       VK_COMPARE_OP_LESS,
                                       VK_COMPARE_OP_EQUAL,
                                       VK_COMPARE_OP_LESS_OR_EQUAL,
                                       VK_COMPARE_OP_GREATER,
                                       VK_COMPARE_OP_NOT_EQUAL,
                                       VK_COMPARE_OP_GREATER_OR_EQUAL,
                                       VK_COMPARE_OP_ALWAYS};
    return ops[mode & 7];
}

/* sample less the bits that change no sampler, so samples that sample
 * alike share one. a flip under a clamp is one, as the clamp alone decides.
 * at most 432 samplers exist: three filters by three ways for u, three for v
 * and 16 adjusts */
uint32_t sampler_of(uint32_t sample)
{
    uint32_t filter = sample_of(sample & SAMPLE_FILTER, 0);

    sample &= SAMPLE_CLAMP_U | SAMPLE_CLAMP_V | SAMPLE_FLIP_U | SAMPLE_FLIP_V |
              SAMPLE_ADJUST;
    if (sample & SAMPLE_CLAMP_U) {
        sample &= ~SAMPLE_FLIP_U;
    }
    if (sample & SAMPLE_CLAMP_V) {
        sample &= ~SAMPLE_FLIP_V;
    }
    return sample | filter;
}

/* a call's result as words, for a reason */
std::string words(VkResult result)
{
    switch (result) {
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "out of host memory";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "out of device memory";
    case VK_ERROR_DEVICE_LOST:
        return "the device is lost";
    case VK_ERROR_MEMORY_MAP_FAILED:
        return "the memory can't be mapped";
    case VK_ERROR_OUT_OF_POOL_MEMORY:
    case VK_ERROR_FRAGMENTED_POOL:
        return "the descriptor pool is full";
    case VK_ERROR_SURFACE_LOST_KHR:
        return "the surface is lost";
    default:
        return "result " + std::to_string(static_cast<int>(result));
    }
}

} // namespace

struct Raster::Impl {
    Calls vk;
    void *loader = nullptr;
    /* why the raster draws no more, or empty while it draws. it names the
     * first call that failed with the device lost or holding a load or
     * picture. every texture and picture after is refused with it */
    std::string broken;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties properties{};
    VkPhysicalDeviceMemoryProperties memory_properties{};
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t family = 0;
    VkCommandPool pool = VK_NULL_HANDLE;
    /* a load is the commands that copy the textures added since the last,
     * sent in one submission. it is sent before a picture, where the staging
     * lacks room, and before an image it names ends. its texels lie in
     * staging from its start */
    VkCommandBuffer commands = VK_NULL_HANDLE;
    VkFence loaded = VK_NULL_HANDLE;
    uint64_t loads = 1; /* the load recording's number, or the next one's */
    bool loading = false;
    Buffer staging;
    VkDeviceSize staged = 0;
    std::vector<VkDeviceMemory> blocks;
    /* the free slots, by memory type and size */
    std::map<std::pair<uint32_t, uint32_t>, std::vector<Slot>> slots;
    std::vector<Frame> frames;
    bool readback = false;
    bool vsync = true;
    Window *window = nullptr;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swap_format = VK_FORMAT_UNDEFINED;
    VkExtent2D swap_extent{};
    VkExtent2D made_for{}; /* the window's size as the swapchain was made */
    std::vector<VkImage> swap_images;
    std::vector<VkSemaphore> rendered; /* one for each image */
    VkQueryPool timer = VK_NULL_HANDLE;
    float timer_period = 0.0F; /* nanoseconds a tick, 0 where none */
    uint32_t resolution = 1; /* RasterOptions', 0 only with a window */
    bool wide = false; /* the picture shows 16:9 */
    VkExtent2D target{}; /* the target's size, which the images below take */
    VkImage color = VK_NULL_HANDLE;
    VkImage depth = VK_NULL_HANDLE;
    VkDeviceMemory color_memory = VK_NULL_HANDLE;
    VkDeviceMemory depth_memory = VK_NULL_HANDLE;
    VkImageView color_view = VK_NULL_HANDLE;
    VkImageView depth_view = VK_NULL_HANDLE;
    VkRenderPass pass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkDescriptorSetLayout picture_layout = VK_NULL_HANDLE;
    VkDescriptorSetLayout texture_layout = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkShaderModule vert = VK_NULL_HANDLE;
    VkShaderModule frag = VK_NULL_HANDLE;
    VkShaderModule resolve_module = VK_NULL_HANDLE;
    VkPipeline resolve = VK_NULL_HANDLE;
    /* the area filter: its pipeline, and the images it writes */
    VkDescriptorSetLayout area_set_layout = VK_NULL_HANDLE;
    VkPipelineLayout area_layout = VK_NULL_HANDLE;
    VkShaderModule area_module = VK_NULL_HANDLE;
    VkPipeline area = VK_NULL_HANDLE;
    Scaled readout;
    Scaled fitted;
    /* the per-pixel lists of blending fragments. heads holds each pixel's
     * first fragment, and fragments holds them after the fragments' head */
    VkImage heads = VK_NULL_HANDLE;
    VkDeviceMemory heads_memory = VK_NULL_HANDLE;
    VkImageView heads_view = VK_NULL_HANDLE;
    Buffer fragments;
    uint32_t capacity = 0;
    VkDescriptorPool descriptors = VK_NULL_HANDLE;
    std::vector<Texture> textures;
    std::vector<uint32_t> free_textures;
    std::map<uint32_t, VkSampler> samplers;
    /* the samplers' bias, log2 of the target's width over the width the
     * picture shows at. with it, a picture larger than where it shows samples
     * each texture at the level it would there. the area filter then smooths
     * its edges and leaves its texels as they were */
    float bias = 0.0F;
    std::map<std::pair<uint32_t, uint32_t>, VkDescriptorSet> sets;
    std::map<
        std::tuple<uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, bool>,
        VkPipeline>
        pipelines;
    /* each batch's pipeline and texture set, for the picture being sent */
    std::vector<std::pair<VkPipeline, VkDescriptorSet>> drawn;

    bool took(VkResult result, const char *call, std::string &why);
    /* took, for a call that makes handle. a failing call leaves handle
     * undefined, so made sets it null, which is safe to destroy */
    template <typename Handle>
    bool made(VkResult result, Handle &handle, const char *call,
              std::string &why)
    {
        if (took(result, call, why)) {
            return true;
        }
        handle = VK_NULL_HANDLE;
        return false;
    }
    bool breaks(const std::string &why);
    bool waited(VkFence fence, std::string &why);
    bool idle();
    /* true where f holds a picture the raster answers for. a broken
     * raster's device may hold part of a picture, or have lost one and
     * signaled its fence */
    bool holds(const Frame &f) const { return f.sent && broken.empty(); }
    uint32_t memory_type(uint32_t bits, VkMemoryPropertyFlags want) const;
    bool make_buffer(Buffer &b, VkDeviceSize size, VkBufferUsageFlags usage,
                     std::string &why, bool on_device = false);
    void free_buffer(Buffer &b);
    bool make_frame(Frame &f, std::string &why);
    void bind_target(Frame &f);
    bool make_target(VkExtent2D size, std::string &why);
    void free_target();
    bool make_scaled(Scaled &s, VkExtent2D size, std::string &why);
    void free_scaled(Scaled &s);
    VkRect2D region() const;
    bool fit(std::string &why);
    void filter(VkCommandBuffer c, const Scaled &to);
    bool make_swapchain(std::string &why);
    void free_swapchain_images();
    bool make_image(VkImage &image, VkDeviceMemory &memory, VkFormat format,
                    uint32_t w, uint32_t h, uint32_t levels,
                    VkImageUsageFlags usage, std::string &why);
    bool make_view(VkImageView &view, VkImage image, VkFormat format,
                   VkImageAspectFlags aspect, uint32_t levels,
                   std::string &why);
    bool make_texture(Texture &t, uint32_t w, uint32_t h, uint32_t levels,
                      std::string &why);
    void end_texture(Texture &t);
    VkCommandBuffer load(std::string &why);
    bool flush(std::string &why);
    VkPipeline pipeline(const State &s, bool collect, std::string &why);
    VkDescriptorSet texture_set(uint32_t texture, uint32_t sample,
                                std::string &why);
    void barrier(VkCommandBuffer c, VkImage image, VkImageLayout from,
                 VkImageLayout to, uint32_t level, uint32_t levels,
                 VkAccessFlags src, VkAccessFlags dst,
                 VkPipelineStageFlags src_stage,
                 VkPipelineStageFlags dst_stage);
};

uint32_t Raster::Impl::memory_type(uint32_t bits,
                                   VkMemoryPropertyFlags want) const
{
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((bits & (1U << i)) &&
            (memory_properties.memoryTypes[i].propertyFlags & want) == want) {
            return i;
        }
    }
    return UINT32_MAX;
}

/* true where a call's result is success. else why names the call and says
 * how it failed. a lost device takes nothing more, so the first call it
 * fails breaks the raster. a call that fails on a broken raster gives the
 * first reason */
bool Raster::Impl::took(VkResult result, const char *call, std::string &why)
{
    if (result == VK_SUCCESS) {
        return true;
    }
    why = std::string(call) + " failed: " + words(result);
    if (result == VK_ERROR_DEVICE_LOST && broken.empty()) {
        broken = why;
    }
    if (!broken.empty()) {
        why = broken;
    }
    return false;
}

/* break the raster after a call fails where the device may hold part of a
 * load or picture. it returns false */
bool Raster::Impl::breaks(const std::string &why)
{
    if (broken.empty()) {
        broken = why;
    }
    return false;
}

/* wait for a fence. a wait that fails for want of memory is tried WAITS
 * times, since giving up breaks the raster. it returns false, with why,
 * where the device is lost or the wait is given up. nothing a fence given up
 * guards ends before the raster's end, as the device may read it still */
bool Raster::Impl::waited(VkFence fence, std::string &why)
{
    VkResult result = VK_SUCCESS;

    for (int made = 0; made < WAITS; made++) {
        result = vk.vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
        if (result != VK_ERROR_OUT_OF_HOST_MEMORY &&
            result != VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            break;
        }
    }
    return took(result, "vkWaitForFences", why) || breaks(why);
}

/* true once the device reads nothing the raster made: it is idle, or lost.
 * a wait for its idle that fails for want of memory is made again, WAITS
 * times in all */
bool Raster::Impl::idle()
{
    for (int made = 0; made < WAITS; made++) {
        VkResult result = vk.vkDeviceWaitIdle(device);

        if (result == VK_SUCCESS || result == VK_ERROR_DEVICE_LOST) {
            return true;
        }
    }
    return false;
}

/* a buffer of size in b, which holds none, bound to memory of its own. it
 * is mapped unless on_device. false, with why, where the device refuses a
 * part, and b holds none still */
bool Raster::Impl::make_buffer(Buffer &b, VkDeviceSize size,
                               VkBufferUsageFlags usage, std::string &why,
                               bool on_device)
{
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    VkMemoryRequirements need;
    Buffer one;

    info.size = size;
    info.usage = usage;
    if (!made(vk.vkCreateBuffer(device, &info, nullptr, &one.buffer),
              one.buffer, "vkCreateBuffer", why)) {
        return false;
    }
    vk.vkGetBufferMemoryRequirements(device, one.buffer, &need);
    VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    VkMemoryPropertyFlags host = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    alloc.allocationSize = need.size;
    /* the host's buffers take memory the CPU caches, where the device has
     * some. an RTX can read write-combined memory just allocated as 64-byte
     * lines of zeros where the host wrote. a texture staged from it keeps
     * them for its life */
    alloc.memoryTypeIndex =
        on_device ? memory_type(need.memoryTypeBits,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                  : memory_type(need.memoryTypeBits,
                                host | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    if (!on_device && alloc.memoryTypeIndex == UINT32_MAX) {
        alloc.memoryTypeIndex = memory_type(need.memoryTypeBits, host);
    }
    if (alloc.memoryTypeIndex == UINT32_MAX) {
        why = "the device has no memory of the type a buffer takes";
    }
    if (alloc.memoryTypeIndex == UINT32_MAX ||
        !made(vk.vkAllocateMemory(device, &alloc, nullptr, &one.memory),
              one.memory, "vkAllocateMemory", why) ||
        !took(vk.vkBindBufferMemory(device, one.buffer, one.memory, 0),
              "vkBindBufferMemory", why) ||
        (!on_device &&
         !made(vk.vkMapMemory(device, one.memory, 0, size, 0, &one.mapped),
               one.mapped, "vkMapMemory", why))) {
        free_buffer(one);
        return false;
    }
    one.size = size;
    b = one;
    return true;
}

void Raster::Impl::free_buffer(Buffer &b)
{
    if (b.buffer != VK_NULL_HANDLE) {
        vk.vkDestroyBuffer(device, b.buffer, nullptr);
        vk.vkFreeMemory(device, b.memory, nullptr);
    }
    b = Buffer{};
}

/* an image and its memory, where the two hold none. false, with why, where
 * the device refuses a part of it, and the two hold none still */
bool Raster::Impl::make_image(VkImage &image, VkDeviceMemory &memory,
                              VkFormat format, uint32_t w, uint32_t h,
                              uint32_t levels, VkImageUsageFlags usage,
                              std::string &why)
{
    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    VkMemoryRequirements need;

    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.extent = {w, h, 1};
    info.mipLevels = levels;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usage;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (!made(vk.vkCreateImage(device, &info, nullptr, &image), image,
              "vkCreateImage", why)) {
        return false;
    }
    vk.vkGetImageMemoryRequirements(device, image, &need);
    VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc.allocationSize = need.size;
    alloc.memoryTypeIndex =
        memory_type(need.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (alloc.memoryTypeIndex == UINT32_MAX) {
        alloc.memoryTypeIndex = memory_type(need.memoryTypeBits, 0);
    }
    if (alloc.memoryTypeIndex == UINT32_MAX) {
        why = "the device has no memory of the type an image takes";
    }
    if (alloc.memoryTypeIndex == UINT32_MAX ||
        !made(vk.vkAllocateMemory(device, &alloc, nullptr, &memory), memory,
              "vkAllocateMemory", why) ||
        !took(vk.vkBindImageMemory(device, image, memory, 0),
              "vkBindImageMemory", why)) {
        vk.vkDestroyImage(device, image, nullptr);
        vk.vkFreeMemory(device, memory, nullptr);
        image = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

/* an image's view, where view holds none: false, with why, and none still */
bool Raster::Impl::make_view(VkImageView &view, VkImage image, VkFormat format,
                             VkImageAspectFlags aspect, uint32_t levels,
                             std::string &why)
{
    VkImageViewCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};

    info.image = image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = format;
    info.subresourceRange = {aspect, 0, levels, 0, 1};
    return made(vk.vkCreateImageView(device, &info, nullptr, &view), view,
                "vkCreateImageView", why);
}

/* a texture's image and its view, in t. false, with why, where the device
 * refuses a part. nothing of it is left then, and its slot is free again */
bool Raster::Impl::make_texture(Texture &t, uint32_t w, uint32_t h,
                                uint32_t levels, std::string &why)
{
    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    VkMemoryRequirements need;
    Texture one;
    bool bound;

    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = VK_FORMAT_R8G8B8A8_UNORM;
    info.extent = {w, h, 1};
    info.mipLevels = levels;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                 VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (!made(vk.vkCreateImage(device, &info, nullptr, &one.image), one.image,
              "vkCreateImage", why)) {
        return false;
    }
    vk.vkGetImageMemoryRequirements(device, one.image, &need);
    uint32_t type =
        memory_type(need.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    uint32_t size = 0;

    if (type == UINT32_MAX) {
        type = memory_type(need.memoryTypeBits, 0);
    }
    if (type == UINT32_MAX) {
        why = "the device has no memory of the type a texture takes";
        end_texture(one);
        return false;
    }
    /* a slot lies at a multiple of its size, so a size at least the
     * alignment aligns it */
    while ((VkDeviceSize{1} << size) < need.size ||
           (VkDeviceSize{1} << size) < need.alignment) {
        size++;
    }
    VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};

    alloc.memoryTypeIndex = type;
    if ((VkDeviceSize{1} << size) > BLOCK / 4) {
        alloc.allocationSize = need.size;
        bound = made(vk.vkAllocateMemory(device, &alloc, nullptr, &one.memory),
                     one.memory, "vkAllocateMemory", why) &&
                took(vk.vkBindImageMemory(device, one.image, one.memory, 0),
                     "vkBindImageMemory", why);
    } else {
        std::vector<Slot> &free = slots[{type, size}];
        VkDeviceMemory block = VK_NULL_HANDLE;

        alloc.allocationSize = BLOCK;
        if (free.empty() &&
            !made(vk.vkAllocateMemory(device, &alloc, nullptr, &block), block,
                  "vkAllocateMemory", why)) {
            end_texture(one);
            return false;
        }
        /* a new block's slots, the lowest taken first */
        if (block != VK_NULL_HANDLE) {
            for (VkDeviceSize at = BLOCK; at > 0;) {
                at -= VkDeviceSize{1} << size;
                free.push_back(Slot{block, at, type, size});
            }
            blocks.push_back(block);
        }
        one.slot = free.back();
        free.pop_back();
        bound = took(vk.vkBindImageMemory(device, one.image, one.slot.block,
                                          one.slot.at),
                     "vkBindImageMemory", why);
    }
    if (!bound || !make_view(one.view, one.image, VK_FORMAT_R8G8B8A8_UNORM,
                             VK_IMAGE_ASPECT_COLOR_BIT, levels, why)) {
        end_texture(one);
        return false;
    }
    t = one;
    return true;
}

/* end a texture. its slot goes back to the free slots, and its block
 * stays */
void Raster::Impl::end_texture(Texture &t)
{
    vk.vkDestroyImageView(device, t.view, nullptr);
    vk.vkDestroyImage(device, t.image, nullptr);
    if (t.memory != VK_NULL_HANDLE) {
        vk.vkFreeMemory(device, t.memory, nullptr);
    } else if (t.slot.block != VK_NULL_HANDLE) {
        slots[{t.slot.type, t.slot.size}].push_back(t.slot);
    }
    t = Texture{};
}

/* the load's commands, begun where none is recording: null, with why, where
 * the device won't begin them */
VkCommandBuffer Raster::Impl::load(std::string &why)
{
    if (!loading) {
        VkCommandBufferBeginInfo begin{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (!took(vk.vkBeginCommandBuffer(commands, &begin),
                  "vkBeginCommandBuffer", why)) {
            return VK_NULL_HANDLE;
        }
        loading = true;
    }
    return commands;
}

/* send the load and wait for it, which frees its staging and lets what it
 * names end. a load the device won't take leaves its textures with no
 * texels, so it breaks the raster. it returns false, with why, then and on a
 * broken raster. the fence the last load signaled is reset just before the
 * send, so a failed reset sends nothing */
bool Raster::Impl::flush(std::string &why)
{
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};

    if (!broken.empty()) {
        why = broken;
        return false;
    }
    if (!loading) {
        return true;
    }
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commands;
    if (!took(vk.vkEndCommandBuffer(commands), "vkEndCommandBuffer", why) ||
        !took(vk.vkResetFences(device, 1, &loaded), "vkResetFences", why) ||
        !took(vk.vkQueueSubmit(queue, 1, &submit, loaded), "vkQueueSubmit",
              why) ||
        !waited(loaded, why)) {
        return breaks(why);
    }
    loading = false;
    loads++;
    staged = 0;
    return true;
}

void Raster::Impl::barrier(VkCommandBuffer c, VkImage image, VkImageLayout from,
                           VkImageLayout to, uint32_t level, uint32_t levels,
                           VkAccessFlags src, VkAccessFlags dst,
                           VkPipelineStageFlags src_stage,
                           VkPipelineStageFlags dst_stage)
{
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};

    b.srcAccessMask = src;
    b.dstAccessMask = dst;
    b.oldLayout = from;
    b.newLayout = to;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = image;
    b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, level, levels, 0, 1};
    vk.vkCmdPipelineBarrier(c, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr,
                            1, &b);
}

/* the pipeline a batch's state draws by, made the first time one asks and
 * kept. under collect it writes no color or depth, as BATCH_COLLECT's
 * fragments go to the lists. null, with why, where the device won't make
 * it */
VkPipeline Raster::Impl::pipeline(const State &s, bool collect,
                                  std::string &why)
{
    auto key = std::make_tuple(s.blend, s.src, s.dst, s.depth, s.depth_write,
                               s.cull, collect);
    auto found = pipelines.find(key);

    if (found != pipelines.end()) {
        return found->second;
    }
    VkPipelineShaderStageCreateInfo stages[2] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO}};
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert;
    stages[0].pName = "main";
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag;
    stages[1].pName = "main";
    VkVertexInputBindingDescription binding{0, sizeof(Vertex),
                                            VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attributes[4] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, at)},
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
        {2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(Vertex, base)},
        {3, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(Vertex, offset)}};
    VkPipelineVertexInputStateCreateInfo input{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    input.vertexBindingDescriptionCount = 1;
    input.pVertexBindingDescriptions = &binding;
    input.vertexAttributeDescriptionCount = 4;
    input.pVertexAttributeDescriptions = attributes;
    VkPipelineInputAssemblyStateCreateInfo assembly{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo viewport{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo raster{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = s.cull ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0F;
    VkPipelineMultisampleStateCreateInfo multisample{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depth_state{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth_state.depthTestEnable = VK_TRUE;
    depth_state.depthWriteEnable =
        s.depth_write && !collect ? VK_TRUE : VK_FALSE;
    depth_state.depthCompareOp = compare(s.depth);
    VkPipelineColorBlendAttachmentState attachment{};
    attachment.blendEnable = s.blend && !collect ? VK_TRUE : VK_FALSE;
    attachment.srcColorBlendFactor = factor(s.src, true, false);
    attachment.dstColorBlendFactor = factor(s.dst, false, false);
    attachment.colorBlendOp = VK_BLEND_OP_ADD;
    attachment.srcAlphaBlendFactor = factor(s.src, true, true);
    attachment.dstAlphaBlendFactor = factor(s.dst, false, true);
    attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    attachment.colorWriteMask =
        collect ? 0
                : VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo blend{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = 1;
    blend.pAttachments = &attachment;
    VkDynamicState dynamic_states[2] = {VK_DYNAMIC_STATE_VIEWPORT,
                                        VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = dynamic_states;
    VkGraphicsPipelineCreateInfo info{
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    info.stageCount = 2;
    info.pStages = stages;
    info.pVertexInputState = &input;
    info.pInputAssemblyState = &assembly;
    info.pViewportState = &viewport;
    info.pRasterizationState = &raster;
    info.pMultisampleState = &multisample;
    info.pDepthStencilState = &depth_state;
    info.pColorBlendState = &blend;
    info.pDynamicState = &dynamic;
    info.layout = layout;
    info.renderPass = pass;
    VkPipeline one = VK_NULL_HANDLE;
    if (!made(vk.vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info,
                                           nullptr, &one),
              one, "vkCreateGraphicsPipelines", why)) {
        return VK_NULL_HANDLE;
    }
    pipelines[key] = one;
    return one;
}

/* the set that gives texture under sample's sampler. each is made the first
 * time one asks, and kept. null, with why, where the device won't make one */
VkDescriptorSet Raster::Impl::texture_set(uint32_t texture, uint32_t sample,
                                          std::string &why)
{
    sample = sampler_of(sample);
    auto key = std::make_pair(texture, sample);
    auto found = sets.find(key);

    if (found != sets.end()) {
        return found->second;
    }
    auto kept = samplers.find(sample);
    VkSampler sampler = kept != samplers.end() ? kept->second : VK_NULL_HANDLE;

    if (sampler == VK_NULL_HANDLE) {
        VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        uint32_t adjust = (sample & SAMPLE_ADJUST) >> SAMPLE_ADJUST_SHIFT;
        float most = properties.limits.maxSamplerLodBias;
        bool point = (sample & SAMPLE_FILTER) == SA_FILTER_POINT;
        bool between = (sample & SAMPLE_FILTER) == SA_FILTER_TRILINEAR;
        VkFilter filter = point ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
        /* the PowerVR scales the texels a pixel spans by the D adjust
         * before it picks the level. each level halves the texels, so an
         * adjust adds its log2 to the level. that is -2 at 0.25, 0 at 1.0
         * and near 1.9 at 3.75. the picture's bias adds to it, within the
         * most the device takes */
        float level = adjust != 0 ? bias + log2f(static_cast<float>(adjust) /
                                                 static_cast<float>(ADJUST_ONE))
                                  : 0.0F;
        auto mode = [&](uint32_t clamp, uint32_t flip) {
            return sample & clamp  ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
                   : sample & flip ? VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT
                                   : VK_SAMPLER_ADDRESS_MODE_REPEAT;
        };
        info.magFilter = filter;
        info.minFilter = filter;
        info.mipmapMode = between ? VK_SAMPLER_MIPMAP_MODE_LINEAR
                                  : VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.addressModeU = mode(SAMPLE_CLAMP_U, SAMPLE_FLIP_U);
        info.addressModeV = mode(SAMPLE_CLAMP_V, SAMPLE_FLIP_V);
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        /* an adjust of 0 has no log2 to add, and its sampler keeps to the
         * largest level */
        info.maxLod = adjust != 0 ? 16.0F : 0.0F;
        info.mipLodBias = level < -most ? -most : level > most ? most : level;
        if (!made(vk.vkCreateSampler(device, &info, nullptr, &sampler), sampler,
                  "vkCreateSampler", why)) {
            return VK_NULL_HANDLE;
        }
        samplers[sample] = sampler;
    }
    VkDescriptorSetAllocateInfo alloc{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    VkDescriptorSet set = VK_NULL_HANDLE;
    alloc.descriptorPool = descriptors;
    alloc.descriptorSetCount = 1;
    alloc.pSetLayouts = &texture_layout;
    if (!made(vk.vkAllocateDescriptorSets(device, &alloc, &set), set,
              "vkAllocateDescriptorSets", why)) {
        return VK_NULL_HANDLE;
    }
    VkDescriptorImageInfo image{sampler, textures[texture].view,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = set;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image;
    vk.vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    sets[key] = set;
    return set;
}

/* what a frame owns, but its vertices, which submit sizes.
 * false, with why, where the device refuses a part: what was made of it
 * ends with the raster */
bool Raster::Impl::make_frame(Frame &f, std::string &why)
{
    VkCommandBufferAllocateInfo command_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkSemaphoreCreateInfo semaphore_info{
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkDescriptorSetAllocateInfo alloc{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};

    command_info.commandPool = pool;
    command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_info.commandBufferCount = 1;
    alloc.descriptorPool = descriptors;
    alloc.descriptorSetCount = 1;
    alloc.pSetLayouts = &picture_layout;
    if (!made(vk.vkAllocateCommandBuffers(device, &command_info, &f.commands),
              f.commands, "vkAllocateCommandBuffers", why) ||
        !made(vk.vkCreateFence(device, &fence_info, nullptr, &f.fence), f.fence,
              "vkCreateFence", why) ||
        !made(
            vk.vkCreateSemaphore(device, &semaphore_info, nullptr, &f.acquired),
            f.acquired, "vkCreateSemaphore", why) ||
        !made(vk.vkAllocateDescriptorSets(device, &alloc, &f.picture_set),
              f.picture_set, "vkAllocateDescriptorSets", why) ||
        !make_buffer(f.uniform, sizeof(PictureUniform),
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, why) ||
        !make_buffer(f.stats, LIST_HEAD, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     why) ||
        (readback && !make_buffer(f.readback, WIDTH * HEIGHT * 4,
                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT, why))) {
        return false;
    }
    memset(f.stats.mapped, 0, LIST_HEAD);
    VkDescriptorBufferInfo uniform_info{f.uniform.buffer, 0,
                                        sizeof(PictureUniform)};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

    write.dstSet = f.picture_set;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &uniform_info;
    vk.vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    return true;
}

/* f's set names the lists and the target from binding 1, after its uniform
 * in 0 */
void Raster::Impl::bind_target(Frame &f)
{
    VkDescriptorImageInfo heads_info{VK_NULL_HANDLE, heads_view,
                                     VK_IMAGE_LAYOUT_GENERAL};
    VkDescriptorBufferInfo fragments_info{fragments.buffer, 0, VK_WHOLE_SIZE};
    VkDescriptorImageInfo target_info{VK_NULL_HANDLE, color_view,
                                      VK_IMAGE_LAYOUT_GENERAL};
    VkWriteDescriptorSet writes[3] = {};

    for (uint32_t i = 0; i < 3; i++) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = f.picture_set;
        writes[i].dstBinding = i + 1;
        writes[i].descriptorCount = 1;
    }
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[0].pImageInfo = &heads_info;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &fragments_info;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[2].pImageInfo = &target_info;
    vk.vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);
}

void Raster::Impl::free_target()
{
    vk.vkDestroyFramebuffer(device, framebuffer, nullptr);
    vk.vkDestroyImageView(device, color_view, nullptr);
    vk.vkDestroyImageView(device, depth_view, nullptr);
    vk.vkDestroyImageView(device, heads_view, nullptr);
    vk.vkDestroyImage(device, color, nullptr);
    vk.vkDestroyImage(device, depth, nullptr);
    vk.vkDestroyImage(device, heads, nullptr);
    vk.vkFreeMemory(device, color_memory, nullptr);
    vk.vkFreeMemory(device, depth_memory, nullptr);
    vk.vkFreeMemory(device, heads_memory, nullptr);
    free_buffer(fragments);
    framebuffer = VK_NULL_HANDLE;
    color_view = depth_view = heads_view = VK_NULL_HANDLE;
    color = depth = heads = VK_NULL_HANDLE;
    color_memory = depth_memory = heads_memory = VK_NULL_HANDLE;
    target = {};
}

/* the target at size, in place of any before it, once nothing draws. it
 * holds color, depth and the lists' heads and fragments. readout joins them
 * where pictures are read from a target other than WIDTH by HEIGHT. the load
 * recording may name the images before these, so it is sent before they
 * end. false, with why, where the device refuses a part. the raster has no
 * target then, and the next fit makes one */
bool Raster::Impl::make_target(VkExtent2D size, std::string &why)
{
    if (!flush(why) ||
        !took(vk.vkDeviceWaitIdle(device), "vkDeviceWaitIdle", why)) {
        return false;
    }
    free_target();
    free_scaled(readout);
    free_scaled(fitted);
    /* room for FRAGMENTS_A_PIXEL fragments a pixel on average, up to the
     * most a storage buffer holds. only the device writes the fragments, so
     * they lie in its memory */
    VkDeviceSize most =
        (properties.limits.maxStorageBufferRange - LIST_HEAD) / FRAGMENT;
    VkDeviceSize room =
        VkDeviceSize{size.width} * size.height * FRAGMENTS_A_PIXEL;
    VkCommandBuffer c = load(why);
    VkImageView views[2];
    VkFramebufferCreateInfo frame_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};

    capacity = static_cast<uint32_t>(room < most ? room : most);
    frame_info.renderPass = pass;
    frame_info.attachmentCount = 2;
    frame_info.pAttachments = views;
    frame_info.width = size.width;
    frame_info.height = size.height;
    frame_info.layers = 1;
    bool whole =
        c != VK_NULL_HANDLE &&
        make_image(color, color_memory, VK_FORMAT_R8G8B8A8_UNORM, size.width,
                   size.height, 1,
                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                       VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                       VK_IMAGE_USAGE_STORAGE_BIT,
                   why) &&
        make_image(heads, heads_memory, VK_FORMAT_R32_UINT, size.width,
                   size.height, 1,
                   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                   why) &&
        make_image(depth, depth_memory, VK_FORMAT_D32_SFLOAT, size.width,
                   size.height, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                   why) &&
        make_view(color_view, color, VK_FORMAT_R8G8B8A8_UNORM,
                  VK_IMAGE_ASPECT_COLOR_BIT, 1, why) &&
        make_view(depth_view, depth, VK_FORMAT_D32_SFLOAT,
                  VK_IMAGE_ASPECT_DEPTH_BIT, 1, why) &&
        make_view(heads_view, heads, VK_FORMAT_R32_UINT,
                  VK_IMAGE_ASPECT_COLOR_BIT, 1, why);

    views[0] = color_view;
    views[1] = depth_view;
    whole =
        whole &&
        made(vk.vkCreateFramebuffer(device, &frame_info, nullptr, &framebuffer),
             framebuffer, "vkCreateFramebuffer", why) &&
        make_buffer(fragments,
                    LIST_HEAD + static_cast<VkDeviceSize>(capacity) * FRAGMENT,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    why, true) &&
        (!readback || (size.width == WIDTH && size.height == HEIGHT) ||
         make_scaled(readout, {WIDTH, HEIGHT}, why));
    if (!whole) {
        free_target();
        return false;
    }
    for (Frame &f : frames) {
        bind_target(f);
    }
    /* the load recording takes the heads to GENERAL, where they stay. it
     * names them only once nothing of the target can fail */
    barrier(c, heads, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 0, 1,
            0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);
    target = size;
    return true;
}

void Raster::Impl::free_scaled(Scaled &s)
{
    if (s.set != VK_NULL_HANDLE) {
        vk.vkFreeDescriptorSets(device, descriptors, 1, &s.set);
    }
    vk.vkDestroyImageView(device, s.view, nullptr);
    vk.vkDestroyImage(device, s.image, nullptr);
    vk.vkFreeMemory(device, s.memory, nullptr);
    s = Scaled{};
}

/* an image at size for the area filter to write from the target, in place
 * of s's. the load recording takes it to GENERAL. the caller sends any load
 * that names s's image first. false, with why, where the device refuses a
 * part, and s holds none then */
bool Raster::Impl::make_scaled(Scaled &s, VkExtent2D size, std::string &why)
{
    VkDescriptorSetAllocateInfo alloc{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    VkCommandBuffer c = load(why);
    Scaled one;

    free_scaled(s);
    alloc.descriptorPool = descriptors;
    alloc.descriptorSetCount = 1;
    alloc.pSetLayouts = &area_set_layout;
    if (c == VK_NULL_HANDLE ||
        !make_image(one.image, one.memory, VK_FORMAT_R8G8B8A8_UNORM, size.width,
                    size.height, 1,
                    VK_IMAGE_USAGE_STORAGE_BIT |
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                    why) ||
        !make_view(one.view, one.image, VK_FORMAT_R8G8B8A8_UNORM,
                   VK_IMAGE_ASPECT_COLOR_BIT, 1, why) ||
        !made(vk.vkAllocateDescriptorSets(device, &alloc, &one.set), one.set,
              "vkAllocateDescriptorSets", why)) {
        free_scaled(one);
        return false;
    }
    one.extent = size;
    VkDescriptorImageInfo images[2] = {
        {VK_NULL_HANDLE, color_view, VK_IMAGE_LAYOUT_GENERAL},
        {VK_NULL_HANDLE, one.view, VK_IMAGE_LAYOUT_GENERAL}};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

    write.dstSet = one.set;
    write.descriptorCount = 2;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.pImageInfo = images;
    vk.vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    barrier(c, one.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 0,
            1, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    s = one;
    return true;
}

/* where the picture shows in the window's image. it is the largest
 * rectangle of the picture's aspect, in the middle */
VkRect2D Raster::Impl::region() const
{
    uint32_t w = swap_extent.width;
    uint32_t h = swap_extent.height;
    uint32_t across = wide ? 16 : 4;
    uint32_t down = wide ? 9 : 3;
    uint32_t fit_w = w * down <= h * across ? w : h * across / down;
    uint32_t fit_h = w * down <= h * across ? w * down / across : h;

    return {{static_cast<int32_t>((w - fit_w) / 2),
             static_cast<int32_t>((h - fit_h) / 2)},
            {fit_w, fit_h}};
}

/* make the target the resolution asks for the window as it stands, and
 * bias to match. where the target is larger than the window's region, make
 * fitted at the region's size. a window of no size keeps the target and the
 * bias it had */
bool Raster::Impl::fit(std::string &why)
{
    VkRect2D shows = region();
    VkExtent2D want{WIDTH * resolution, HEIGHT * resolution};

    if (resolution == 0) {
        want = shows.extent.width >= WIDTH && shows.extent.height >= HEIGHT
                   ? shows.extent
               : shows.extent.width == 0 && target.width != 0
                   ? target
                   : VkExtent2D{WIDTH, HEIGHT};
    }
    if ((want.width != target.width || want.height != target.height) &&
        !make_target(want, why)) {
        return false;
    }
    bool larger =
        shows.extent.width != 0 && (target.width > shows.extent.width ||
                                    target.height > shows.extent.height);

    /* the load recording may name the image fitted before, so it is sent
     * before that one ends */
    if (larger && (fitted.extent.width != shows.extent.width ||
                   fitted.extent.height != shows.extent.height)) {
        if (!flush(why) ||
            !took(vk.vkDeviceWaitIdle(device), "vkDeviceWaitIdle", why) ||
            !make_scaled(fitted, shows.extent, why)) {
            return false;
        }
    } else if (!larger && fitted.image != VK_NULL_HANDLE) {
        if (!flush(why) ||
            !took(vk.vkDeviceWaitIdle(device), "vkDeviceWaitIdle", why)) {
            return false;
        }
        free_scaled(fitted);
    }
    /* the width the picture shows at: the window region's, or WIDTH without
     * a window */
    uint32_t to = window != nullptr ? shows.extent.width : WIDTH;
    float level = to != 0 && target.width > to
                      ? fminf(log2f(static_cast<float>(target.width) /
                                    static_cast<float>(to)),
                              properties.limits.maxSamplerLodBias)
                      : 0.0F;

    /* a sampler's bias is fixed as it is made */
    if (to != 0 && level != bias) {
        if (!took(vk.vkDeviceWaitIdle(device), "vkDeviceWaitIdle", why)) {
            return false;
        }
        for (auto &set : sets) {
            vk.vkFreeDescriptorSets(device, descriptors, 1, &set.second);
        }
        for (auto &sampler : samplers) {
            vk.vkDestroySampler(device, sampler.second, nullptr);
        }
        sets.clear();
        samplers.clear();
        bias = level;
    }
    return true;
}

/* the area filter, from the target into to, after what the picture drew
 * and before what reads to */
void Raster::Impl::filter(VkCommandBuffer c, const Scaled &to)
{
    VkMemoryBarrier drawn{VK_STRUCTURE_TYPE_MEMORY_BARRIER};

    drawn.srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    drawn.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vk.vkCmdPipelineBarrier(c,
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &drawn,
                            0, nullptr, 0, nullptr);
    vk.vkCmdBindPipeline(c, VK_PIPELINE_BIND_POINT_COMPUTE, area);
    vk.vkCmdBindDescriptorSets(c, VK_PIPELINE_BIND_POINT_COMPUTE, area_layout,
                               0, 1, &to.set, 0, nullptr);
    vk.vkCmdDispatch(c, (to.extent.width + GROUP - 1) / GROUP,
                     (to.extent.height + GROUP - 1) / GROUP, 1);
    barrier(c, to.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, 0, 1,
            VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);
}

void Raster::Impl::free_swapchain_images()
{
    for (VkSemaphore s : rendered) {
        vk.vkDestroySemaphore(device, s, nullptr);
    }
    rendered.clear();
    swap_images.clear();
}

/* the swapchain for the window as it stands, in place of any before it.
 * a window of no size gets none. false, with why, where the surface won't
 * say what it takes, and the swapchain before stays. false too where the
 * device won't make the new one whole, and then there is none */
bool Raster::Impl::make_swapchain(std::string &why)
{
    VkSurfaceCapabilitiesKHR has;
    uint32_t count = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> modes;

    if (!took(vk.vkDeviceWaitIdle(device), "vkDeviceWaitIdle", why) ||
        !took(vk.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface,
                                                           &has),
              "vkGetPhysicalDeviceSurfaceCapabilitiesKHR", why)) {
        return false;
    }
    window_size(window, width, height);
    VkExtent2D extent = has.currentExtent;

    if (extent.width == UINT32_MAX) {
        extent = {width, height};
    }
    bool sized = extent.width != 0 && extent.height != 0;

    if (sized) {
        if (!took(vk.vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface,
                                                          &count, nullptr),
                  "vkGetPhysicalDeviceSurfaceFormatsKHR", why)) {
            return false;
        }
        formats.resize(count);
        if (!took(vk.vkGetPhysicalDeviceSurfaceFormatsKHR(
                      physical, surface, &count, formats.data()),
                  "vkGetPhysicalDeviceSurfaceFormatsKHR", why)) {
            return false;
        }
        formats.resize(count);
        if (formats.empty()) {
            why = "the surface offers no format";
            return false;
        }
        /* a surface that won't list its modes still has FIFO, as each does */
        if (vk.vkGetPhysicalDeviceSurfacePresentModesKHR(
                physical, surface, &count, nullptr) == VK_SUCCESS) {
            modes.resize(count);
            if (vk.vkGetPhysicalDeviceSurfacePresentModesKHR(
                    physical, surface, &count, modes.data()) != VK_SUCCESS) {
                count = 0;
            }
            modes.resize(count);
        }
    }
    /* from here the swapchain before is gone, whatever comes of the new one */
    VkSwapchainKHR old = swapchain;

    made_for = {width, height};
    swap_extent = extent;
    swapchain = VK_NULL_HANDLE;
    free_swapchain_images();
    if (!sized) {
        vk.vkDestroySwapchainKHR(device, old, nullptr);
        return true;
    }
    /* the picture holds what the PowerVR wrote, and a UNORM format shows it
     * with no sRGB step. a surface that offers none takes its first */
    VkSurfaceFormatKHR format = formats[0];
    for (const VkSurfaceFormatKHR &f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM ||
            f.format == VK_FORMAT_R8G8B8A8_UNORM) {
            format = f;
            break;
        }
    }
    /* the game keeps its own clock, where FIFO would hold it to the
     * display's rate. mailbox, where the surface offers it, shows the newest
     * picture at each vertical blank. without vsync, immediate shows each at
     * once, where offered */
    VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;
    for (VkPresentModeKHR m : modes) {
        if ((!vsync && m == VK_PRESENT_MODE_IMMEDIATE_KHR) ||
            (m == VK_PRESENT_MODE_MAILBOX_KHR &&
             mode == VK_PRESENT_MODE_FIFO_KHR)) {
            mode = m;
        }
    }
    uint32_t images = has.minImageCount + 1;
    if (has.maxImageCount != 0 && images > has.maxImageCount) {
        images = has.maxImageCount;
    }
    VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    info.surface = surface;
    info.minImageCount = images;
    info.imageFormat = format.format;
    info.imageColorSpace = format.colorSpace;
    info.imageExtent = swap_extent;
    info.imageArrayLayers = 1;
    info.imageUsage =
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = has.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (!(has.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)) {
        info.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    }
    info.presentMode = mode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = old;
    bool whole =
        made(vk.vkCreateSwapchainKHR(device, &info, nullptr, &swapchain),
             swapchain, "vkCreateSwapchainKHR", why);

    /* the old one is retired whether or not the new one was made */
    vk.vkDestroySwapchainKHR(device, old, nullptr);
    if (!whole) {
        return false;
    }
    swap_format = format.format;
    /* its images, and a semaphore for each. a swapchain short of one of
     * them shows nothing, so it ends here */
    whole = took(vk.vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr),
                 "vkGetSwapchainImagesKHR", why);
    swap_images.resize(whole ? count : 0);
    whole = whole && took(vk.vkGetSwapchainImagesKHR(device, swapchain, &count,
                                                     swap_images.data()),
                          "vkGetSwapchainImagesKHR", why);
    rendered.assign(whole ? swap_images.size() : 0, VK_NULL_HANDLE);
    VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    for (VkSemaphore &s : rendered) {
        whole =
            whole && made(vk.vkCreateSemaphore(device, &semaphore, nullptr, &s),
                          s, "vkCreateSemaphore", why);
    }
    if (!whole) {
        free_swapchain_images();
        vk.vkDestroySwapchainKHR(device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

Raster::~Raster()
{
    if (impl_ == nullptr) {
        return;
    }
    Impl &r = *impl_;
    Calls &vk = r.vk;

    /* a device that won't say it is idle may still read what the raster
     * made. the raster then ends none of it, and leaves it all to the
     * process's end */
    if (r.device != VK_NULL_HANDLE && !r.idle()) {
        delete impl_;
        return;
    }
    if (r.device != VK_NULL_HANDLE) {
        r.free_swapchain_images();
        if (r.swapchain != VK_NULL_HANDLE) {
            vk.vkDestroySwapchainKHR(r.device, r.swapchain, nullptr);
        }
        for (Frame &f : r.frames) {
            vk.vkDestroyFence(r.device, f.fence, nullptr);
            vk.vkDestroySemaphore(r.device, f.acquired, nullptr);
            r.free_buffer(f.uniform);
            r.free_buffer(f.vertices);
            r.free_buffer(f.readback);
            r.free_buffer(f.stats);
        }
        vk.vkDestroyQueryPool(r.device, r.timer, nullptr);
        for (auto &p : r.pipelines) {
            vk.vkDestroyPipeline(r.device, p.second, nullptr);
        }
        for (auto &s : r.samplers) {
            vk.vkDestroySampler(r.device, s.second, nullptr);
        }
        for (Texture &t : r.textures) {
            vk.vkDestroyImageView(r.device, t.view, nullptr);
            vk.vkDestroyImage(r.device, t.image, nullptr);
            vk.vkFreeMemory(r.device, t.memory, nullptr);
        }
        for (VkDeviceMemory block : r.blocks) {
            vk.vkFreeMemory(r.device, block, nullptr);
        }
        r.free_buffer(r.staging);
        vk.vkDestroyFence(r.device, r.loaded, nullptr);
        vk.vkDestroyPipeline(r.device, r.resolve, nullptr);
        vk.vkDestroyShaderModule(r.device, r.resolve_module, nullptr);
        vk.vkDestroyPipeline(r.device, r.area, nullptr);
        vk.vkDestroyShaderModule(r.device, r.area_module, nullptr);
        vk.vkDestroyPipelineLayout(r.device, r.area_layout, nullptr);
        vk.vkDestroyDescriptorSetLayout(r.device, r.area_set_layout, nullptr);
        r.free_scaled(r.readout);
        r.free_scaled(r.fitted);
        r.free_target();
        vk.vkDestroyDescriptorPool(r.device, r.descriptors, nullptr);
        vk.vkDestroyShaderModule(r.device, r.vert, nullptr);
        vk.vkDestroyShaderModule(r.device, r.frag, nullptr);
        vk.vkDestroyPipelineLayout(r.device, r.layout, nullptr);
        vk.vkDestroyDescriptorSetLayout(r.device, r.picture_layout, nullptr);
        vk.vkDestroyDescriptorSetLayout(r.device, r.texture_layout, nullptr);
        vk.vkDestroyRenderPass(r.device, r.pass, nullptr);
        vk.vkDestroyCommandPool(r.device, r.pool, nullptr);
        vk.vkDestroyDevice(r.device, nullptr);
    }
    if (r.instance != VK_NULL_HANDLE) {
        if (r.surface != VK_NULL_HANDLE) {
            vk.vkDestroySurfaceKHR(r.instance, r.surface, nullptr);
        }
        vk.vkDestroyInstance(r.instance, nullptr);
    }
    if (r.loader != nullptr) {
        loader_close(r.loader);
    }
    delete impl_;
}

std::string Raster::device_name() const
{
    return impl_ != nullptr ? impl_->properties.deviceName : "";
}

bool Raster::open(const RasterOptions &options, std::string &why)
{
    if (impl_ != nullptr) {
        why = "the raster was opened before";
        return false;
    }
    impl_ = new Impl;
    Impl &r = *impl_;
    Calls &vk = r.vk;
    const std::string &prefer = options.device;
    const char *lacks = nullptr; /* the first call the loader has none of */

    r.window = options.window;
    r.vsync = options.vsync;
    r.readback = options.readback;
    r.resolution =
        options.resolution != 0 || r.window != nullptr ? options.resolution : 1;

    r.loader = loader_open();
    if (r.loader == nullptr) {
        why = "no Vulkan: " LOADER ", its loader, isn't installed";
        return false;
    }
    vk.vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        loader_find(r.loader, "vkGetInstanceProcAddr"));
    if (vk.vkGetInstanceProcAddr == nullptr) {
        why = "no Vulkan: " LOADER " has no vkGetInstanceProcAddr";
        return false;
    }
#define TAKE(name, from, of)                                                   \
    vk.name = reinterpret_cast<PFN_##name>(from(of, #name));                   \
    if (vk.name == nullptr && lacks == nullptr) {                              \
        lacks = #name;                                                         \
    }
#define GLOBAL(name) TAKE(name, vk.vkGetInstanceProcAddr, VK_NULL_HANDLE)
    GLOBAL_CALLS(GLOBAL)
#undef GLOBAL
    if (lacks != nullptr) {
        why = std::string("no Vulkan: " LOADER " has no ") + lacks;
        return false;
    }
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "sonic-adventure";
    app.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_info.pApplicationInfo = &app;
    if (r.window != nullptr) {
        instance_info.ppEnabledExtensionNames =
            window_extensions(instance_info.enabledExtensionCount);
    }
    VkResult created =
        vk.vkCreateInstance(&instance_info, nullptr, &r.instance);

    if (created != VK_SUCCESS) {
        r.instance = VK_NULL_HANDLE;
        if (created == VK_ERROR_INCOMPATIBLE_DRIVER) {
            why = "no Vulkan device: no driver gives Vulkan 1.1";
        } else {
            r.took(created, "vkCreateInstance", why);
        }
        return false;
    }
#define INSTANCE(name) TAKE(name, vk.vkGetInstanceProcAddr, r.instance)
    INSTANCE_CALLS(INSTANCE)
    if (r.window != nullptr) {
        SURFACE_CALLS(INSTANCE)
    }
#undef INSTANCE
    /* the raster's end would call what the instance lacks, so an instance
     * short of a call ends here. it ends by vkDestroyInstance, unless that
     * is the call it lacks */
    if (lacks != nullptr) {
        why = std::string("the Vulkan instance has no ") + lacks;
        if (vk.vkDestroyInstance != nullptr) {
            vk.vkDestroyInstance(r.instance, nullptr);
        }
        r.instance = VK_NULL_HANDLE;
        return false;
    }
    if (r.window != nullptr) {
        uint64_t surface = 0;

        if (!window_surface(r.window, r.instance, surface)) {
            why = "can't make the window's surface";
            return false;
        }
        r.surface = reinterpret_cast<VkSurfaceKHR>(surface);
    }
    uint32_t count = 0;
    if (!r.took(vk.vkEnumeratePhysicalDevices(r.instance, &count, nullptr),
                "vkEnumeratePhysicalDevices", why)) {
        return false;
    }
    std::vector<VkPhysicalDevice> devices(count);
    VkResult listed =
        vk.vkEnumeratePhysicalDevices(r.instance, &count, devices.data());

    /* a device that came between the two calls is left for the next run */
    if (listed != VK_INCOMPLETE &&
        !r.took(listed, "vkEnumeratePhysicalDevices", why)) {
        return false;
    }
    devices.resize(count);
    for (VkPhysicalDevice d : devices) {
        VkPhysicalDeviceProperties p;

        vk.vkGetPhysicalDeviceProperties(d, &p);
        bool named =
            !prefer.empty() && strstr(p.deviceName, prefer.c_str()) != nullptr;

        if (r.physical == VK_NULL_HANDLE || named) {
            r.physical = d;
            r.properties = p;
        }
        if (named) {
            break;
        }
    }
    if (r.physical == VK_NULL_HANDLE) {
        why = "no Vulkan device: the drivers list none";
        return false;
    }
    vk.vkGetPhysicalDeviceMemoryProperties(r.physical, &r.memory_properties);
    vk.vkGetPhysicalDeviceQueueFamilyProperties(r.physical, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vk.vkGetPhysicalDeviceQueueFamilyProperties(r.physical, &count,
                                                families.data());
    r.family = UINT32_MAX;
    for (uint32_t i = 0; i < count && r.family == UINT32_MAX; i++) {
        VkBool32 shows = VK_TRUE;

        if (r.surface != VK_NULL_HANDLE &&
            !r.took(vk.vkGetPhysicalDeviceSurfaceSupportKHR(r.physical, i,
                                                            r.surface, &shows),
                    "vkGetPhysicalDeviceSurfaceSupportKHR", why)) {
            return false;
        }
        if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && shows) {
            r.family = i;
        }
    }
    if (r.family == UINT32_MAX) {
        why = "no graphics queue that can show the window";
        return false;
    }
    /* a family with no timestamp bits can't say what a picture took */
    if (families[r.family].timestampValidBits != 0) {
        r.timer_period = r.properties.limits.timestampPeriod;
    }
    float priority = 1.0F;
    VkDeviceQueueCreateInfo queue_info{
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue_info.queueFamilyIndex = r.family;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;
    VkPhysicalDeviceFeatures has;
    VkPhysicalDeviceFeatures features{};
    vk.vkGetPhysicalDeviceFeatures(r.physical, &has);
    if (!has.fragmentStoresAndAtomics) {
        why = "the device can't store from a fragment shader";
        return false;
    }
    if (!has.shaderClipDistance) {
        why = "the device can't clip by a distance";
        return false;
    }
    features.fragmentStoresAndAtomics = VK_TRUE;
    features.shaderClipDistance = VK_TRUE;
    VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.pEnabledFeatures = &features;
    const char *swapchain_extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    if (r.window != nullptr) {
        device_info.enabledExtensionCount = 1;
        device_info.ppEnabledExtensionNames = &swapchain_extension;
    }
    if (!r.made(vk.vkCreateDevice(r.physical, &device_info, nullptr, &r.device),
                r.device, "vkCreateDevice", why)) {
        return false;
    }
#define DEVICE(name) TAKE(name, vk.vkGetDeviceProcAddr, r.device)
    DEVICE_CALLS(DEVICE)
    if (r.window != nullptr) {
        SWAPCHAIN_CALLS(DEVICE)
    }
#undef DEVICE
#undef TAKE
    /* a device short of a call ends here, as an instance does */
    if (lacks != nullptr) {
        why = std::string("the Vulkan device has no ") + lacks;
        if (vk.vkDestroyDevice != nullptr) {
            vk.vkDestroyDevice(r.device, nullptr);
        }
        r.device = VK_NULL_HANDLE;
        return false;
    }
    vk.vkGetDeviceQueue(r.device, r.family, 0, &r.queue);
    VkCommandPoolCreateInfo pool_info{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = r.family;
    if (!r.made(vk.vkCreateCommandPool(r.device, &pool_info, nullptr, &r.pool),
                r.pool, "vkCreateCommandPool", why)) {
        return false;
    }
    VkCommandBufferAllocateInfo command_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    command_info.commandPool = r.pool;
    command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_info.commandBufferCount = 1;
    VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (!r.made(
            vk.vkAllocateCommandBuffers(r.device, &command_info, &r.commands),
            r.commands, "vkAllocateCommandBuffers", why) ||
        !r.made(vk.vkCreateFence(r.device, &fence_info, nullptr, &r.loaded),
                r.loaded, "vkCreateFence", why)) {
        return false;
    }

    if (WIDTH * r.resolution > r.properties.limits.maxImageDimension2D) {
        why = "the device can't hold a picture at that resolution";
        return false;
    }
    /* the target's pass: color and depth, which a picture clears */
    VkAttachmentDescription attachments[2] = {};
    attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    attachments[1] = attachments[0];
    attachments[1].format = VK_FORMAT_D32_SFLOAT;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference color_ref{0,
                                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth_ref{
        1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_ref;
    subpass.pDepthStencilAttachment = &depth_ref;
    /* the resolve and the readback wait for the pass's writes */
    VkSubpassDependency after{};
    after.srcSubpass = 0;
    after.dstSubpass = VK_SUBPASS_EXTERNAL;
    after.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    after.dstStageMask =
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
    after.srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    after.dstAccessMask = VK_ACCESS_SHADER_READ_BIT |
                          VK_ACCESS_SHADER_WRITE_BIT |
                          VK_ACCESS_TRANSFER_READ_BIT;
    VkRenderPassCreateInfo pass_info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    pass_info.attachmentCount = 2;
    pass_info.pAttachments = attachments;
    pass_info.subpassCount = 1;
    pass_info.pSubpasses = &subpass;
    pass_info.dependencyCount = 1;
    pass_info.pDependencies = &after;
    if (!r.made(vk.vkCreateRenderPass(r.device, &pass_info, nullptr, &r.pass),
                r.pass, "vkCreateRenderPass", why)) {
        return false;
    }

    /* the layouts: the picture's constants in set 0, and a texture in set 1.
     * the batch's flags and blend instructions are pushed */
    VkShaderStageFlags list_stages =
        VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
    VkDescriptorSetLayoutBinding picture_bindings[4] = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, list_stages, nullptr},
        {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, list_stages, nullptr},
        {3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT,
         nullptr}};
    VkDescriptorSetLayoutBinding texture_binding{
        0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo set_info{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    set_info.bindingCount = 4;
    set_info.pBindings = picture_bindings;
    if (!r.made(vk.vkCreateDescriptorSetLayout(r.device, &set_info, nullptr,
                                               &r.picture_layout),
                r.picture_layout, "vkCreateDescriptorSetLayout", why)) {
        return false;
    }
    set_info.bindingCount = 1;
    set_info.pBindings = &texture_binding;
    if (!r.made(vk.vkCreateDescriptorSetLayout(r.device, &set_info, nullptr,
                                               &r.texture_layout),
                r.texture_layout, "vkCreateDescriptorSetLayout", why)) {
        return false;
    }
    VkDescriptorSetLayout set_layouts[2] = {r.picture_layout, r.texture_layout};
    VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT |
                                 VK_SHADER_STAGE_FRAGMENT_BIT,
                             0, sizeof(Push)};
    VkPipelineLayoutCreateInfo layout_info{
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout_info.setLayoutCount = 2;
    layout_info.pSetLayouts = set_layouts;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push;
    auto module = [&](const uint32_t *code, size_t size, VkShaderModule &to) {
        VkShaderModuleCreateInfo module_info{
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};

        module_info.codeSize = size;
        module_info.pCode = code;
        return r.made(
            vk.vkCreateShaderModule(r.device, &module_info, nullptr, &to), to,
            "vkCreateShaderModule", why);
    };
    if (!r.made(vk.vkCreatePipelineLayout(r.device, &layout_info, nullptr,
                                          &r.layout),
                r.layout, "vkCreatePipelineLayout", why) ||
        !module(draw_vert, sizeof draw_vert, r.vert) ||
        !module(draw_frag, sizeof draw_frag, r.frag) ||
        !module(resolve_comp, sizeof resolve_comp, r.resolve_module)) {
        return false;
    }
    VkComputePipelineCreateInfo compute{
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    compute.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    compute.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    compute.stage.module = r.resolve_module;
    compute.stage.pName = "main";
    compute.layout = r.layout;
    if (!r.made(vk.vkCreateComputePipelines(r.device, VK_NULL_HANDLE, 1,
                                            &compute, nullptr, &r.resolve),
                r.resolve, "vkCreateComputePipelines", why)) {
        return false;
    }
    /* the area filter: the target and the image it writes, in set 0 */
    VkDescriptorSetLayoutBinding area_bindings[2] = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT,
         nullptr},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT,
         nullptr}};
    set_info.bindingCount = 2;
    set_info.pBindings = area_bindings;
    if (!r.made(vk.vkCreateDescriptorSetLayout(r.device, &set_info, nullptr,
                                               &r.area_set_layout),
                r.area_set_layout, "vkCreateDescriptorSetLayout", why)) {
        return false;
    }
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &r.area_set_layout;
    layout_info.pushConstantRangeCount = 0;
    if (!r.made(vk.vkCreatePipelineLayout(r.device, &layout_info, nullptr,
                                          &r.area_layout),
                r.area_layout, "vkCreatePipelineLayout", why) ||
        !module(area_comp, sizeof area_comp, r.area_module)) {
        return false;
    }
    compute.stage.module = r.area_module;
    compute.layout = r.area_layout;
    if (!r.made(vk.vkCreateComputePipelines(r.device, VK_NULL_HANDLE, 1,
                                            &compute, nullptr, &r.area),
                r.area, "vkCreateComputePipelines", why)) {
        return false;
    }
    /* the pool's sets: TEXTURE_SETS for textures, one a frame, and one each
     * for readout and fitted. a frame's set names its uniform, the lists'
     * buffer and two storage images. readout's and fitted's name two storage
     * images each */
    uint32_t frames = options.frames;
    VkDescriptorPoolSize sizes[4] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frames},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, TEXTURE_SETS},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, frames * 2 + 4},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frames}};
    VkDescriptorPoolCreateInfo pool{
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool.maxSets = TEXTURE_SETS + frames + 2;
    pool.poolSizeCount = 4;
    pool.pPoolSizes = sizes;
    if (!r.made(
            vk.vkCreateDescriptorPool(r.device, &pool, nullptr, &r.descriptors),
            r.descriptors, "vkCreateDescriptorPool", why)) {
        return false;
    }
    r.frames.resize(frames);
    for (Frame &f : r.frames) {
        if (!r.make_frame(f, why)) {
            return false;
        }
    }
    if (r.timer_period != 0.0F) {
        VkQueryPoolCreateInfo timer{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
        std::string untimed;

        timer.queryType = VK_QUERY_TYPE_TIMESTAMP;
        timer.queryCount = frames * 2;
        /* a device that won't make the timer draws without one */
        r.made(vk.vkCreateQueryPool(r.device, &timer, nullptr, &r.timer),
               r.timer, "vkCreateQueryPool", untimed);
    }
    if ((r.window != nullptr && !r.make_swapchain(why)) || !r.fit(why)) {
        return false;
    }
    /* texture 0, white, for the batches without one */
    Image white;
    white.width = 1;
    white.height = 1;
    white.rgba.assign(4, 255);
    add_texture(white, why);
    return !r.textures.empty();
}

uint32_t Raster::add_texture(const Image &image, std::string &why)
{
    Impl &r = *impl_;
    Calls &vk = r.vk;
    Texture t;
    uint32_t levels = 1;
    VkDeviceSize bytes = image.rgba.size();

    while ((image.width >> levels) > 0 && (image.height >> levels) > 0) {
        levels++;
    }
    if (!r.broken.empty()) {
        why = r.broken;
        return 0;
    }
    /* the texels join the load's staging, and the load is sent first where
     * they would pass its end */
    if (r.staged + bytes > r.staging.size && !r.flush(why)) {
        return 0;
    }
    if (bytes > r.staging.size) {
        r.free_buffer(r.staging);
        if (!r.make_buffer(r.staging, bytes > STAGING ? bytes : STAGING,
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT, why)) {
            return 0;
        }
    }
    /* the texture is whole before the load names it or the raster counts
     * it. one the device refuses changes neither */
    VkCommandBuffer c = r.load(why);

    if (c == VK_NULL_HANDLE ||
        !r.make_texture(t, image.width, image.height, levels, why)) {
        return 0;
    }
    memcpy(static_cast<uint8_t *>(r.staging.mapped) + r.staged,
           image.rgba.data(), bytes);
    t.load = r.loads;
    r.barrier(c, t.image, VK_IMAGE_LAYOUT_UNDEFINED,
              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, levels, 0,
              VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
              VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy copy{};
    copy.bufferOffset = r.staged;
    copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copy.imageExtent = {image.width, image.height, 1};
    vk.vkCmdCopyBufferToImage(c, r.staging.buffer, t.image,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
    /* the smaller levels, each a box filter of the one above */
    for (uint32_t level = 1; level < levels; level++) {
        r.barrier(c, t.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, level - 1, 1,
                  VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                  VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkImageBlit blit{};
        blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 0, 1};
        blit.srcOffsets[1] = {static_cast<int32_t>(image.width >> (level - 1)),
                              static_cast<int32_t>(image.height >> (level - 1)),
                              1};
        blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, 0, 1};
        blit.dstOffsets[1] = {static_cast<int32_t>(image.width >> level),
                              static_cast<int32_t>(image.height >> level), 1};
        vk.vkCmdBlitImage(c, t.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          t.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                          &blit, VK_FILTER_LINEAR);
        r.barrier(c, t.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, level - 1, 1,
                  VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }
    r.barrier(c, t.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, levels - 1, 1,
              VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
              VK_PIPELINE_STAGE_TRANSFER_BIT,
              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    /* a copy's offset must be a multiple of a texel's 4 bytes. 16 keeps it
     * one */
    r.staged += (bytes + 15) & ~VkDeviceSize{15};
    if (!r.free_textures.empty()) {
        uint32_t at = r.free_textures.back();

        r.free_textures.pop_back();
        r.textures[at] = t;
        return at;
    }
    r.textures.push_back(t);
    return static_cast<uint32_t>(r.textures.size() - 1);
}

void Raster::free_texture(uint32_t texture)
{
    Impl &r = *impl_;
    Calls &vk = r.vk;
    Texture &t = r.textures[texture];
    std::string unsent;

    /* the load recording may name it, so it is sent first. a broken
     * raster's device may read the texture, from a load or picture whose
     * wait was given up. the texture then stays until the raster's end */
    if (r.loading && t.load == r.loads) {
        r.flush(unsent);
    }
    if (!r.broken.empty()) {
        return;
    }
    for (auto i = r.sets.lower_bound({texture, 0});
         i != r.sets.end() && i->first.first == texture;) {
        vk.vkFreeDescriptorSets(r.device, r.descriptors, 1, &i->second);
        i = r.sets.erase(i);
    }
    r.end_texture(t);
    r.free_textures.push_back(texture);
}

bool Raster::submit(uint32_t frame, const PictureUniform &picture,
                    uint32_t clear, const std::vector<Vertex> &vertices,
                    const std::vector<Batch> &batches, bool sorted,
                    std::string &why)
{
    Impl &r = *impl_;
    Calls &vk = r.vk;
    Frame &f = r.frames[frame];
    /* one more than the vertices, as a buffer of 0 bytes can't be made */
    VkDeviceSize need = (vertices.size() + 1) * sizeof(Vertex);

    if (!r.broken.empty()) {
        why = r.broken;
        return false;
    }
    /* a window that changed size, or shrank to nothing and came back, gets
     * a swapchain to fit. fit then makes the target its resolution asks */
    if (r.window != nullptr) {
        uint32_t width = 0;
        uint32_t height = 0;

        window_size(r.window, width, height);
        if ((r.swapchain == VK_NULL_HANDLE || width != r.made_for.width ||
             height != r.made_for.height) &&
            !r.make_swapchain(why)) {
            return false;
        }
        if (!r.fit(why)) {
            return false;
        }
    }
    /* the textures it may draw reach the device first. the frame's last
     * picture retires before its buffers and fence are this one's */
    if (!r.flush(why) || (f.sent && !r.waited(f.fence, why))) {
        return false;
    }
    f.sent = false;
    if (f.vertices.size < need) {
        r.free_buffer(f.vertices);
        if (!r.make_buffer(f.vertices, need * 2,
                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, why)) {
            return false;
        }
    }
    /* each batch's pipeline and texture set are made before any command. a
     * batch the device can't draw then leaves no commands begun and no image
     * acquired */
    r.drawn.clear();
    for (const Batch &b : batches) {
        VkPipeline p = r.pipeline(b.state, sorted && b.state.blend, why);
        VkDescriptorSet set =
            p != VK_NULL_HANDLE
                ? r.texture_set(b.state.texture, b.state.sample, why)
                : VK_NULL_HANDLE;

        if (set == VK_NULL_HANDLE) {
            return false;
        }
        r.drawn.emplace_back(p, set);
    }
    memcpy(f.vertices.mapped, vertices.data(),
           vertices.size() * sizeof(Vertex));
    memcpy(f.uniform.mapped, &picture, sizeof picture);
    if (!r.took(vk.vkResetFences(r.device, 1, &f.fence), "vkResetFences",
                why)) {
        return false;
    }
    uint32_t swap_image = UINT32_MAX;
    if (r.window != nullptr) {
        if (r.swapchain != VK_NULL_HANDLE) {
            VkResult got = vk.vkAcquireNextImageKHR(
                r.device, r.swapchain, UINT64_MAX, f.acquired, VK_NULL_HANDLE,
                &swap_image);

            if (got == VK_ERROR_OUT_OF_DATE_KHR) {
                swap_image = UINT32_MAX;
                if (!r.make_swapchain(why)) {
                    return false;
                }
            } else if (got != VK_SUBOPTIMAL_KHR &&
                       !r.took(got, "vkAcquireNextImageKHR", why)) {
                return false;
            }
        }
    }
    /* from here the device may hold an acquired image and then part of the
     * picture's commands. a failure to record or send them breaks the
     * raster */
    VkCommandBufferBeginInfo start{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VkCommandBuffer c = f.commands;

    start.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (!r.took(vk.vkBeginCommandBuffer(c, &start), "vkBeginCommandBuffer",
                why)) {
        return r.breaks(why);
    }
    /* the pictures in flight share the target and the lists. each waits for
     * the one before it on the queue */
    VkMemoryBarrier all{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    all.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    all.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    vk.vkCmdPipelineBarrier(c, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &all, 0,
                            nullptr, 0, nullptr);
    if (r.timer != VK_NULL_HANDLE) {
        vk.vkCmdResetQueryPool(c, r.timer, frame * 2, 2);
        vk.vkCmdWriteTimestamp(c, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, r.timer,
                               frame * 2);
    }
    uint32_t head[HEAD_WORDS] = {};
    head[HEAD_CAPACITY] = r.capacity;
    vk.vkCmdUpdateBuffer(c, r.fragments.buffer, 0, sizeof head, head);
    /* each pixel's head starts with no fragment, the resolve's NONE */
    VkClearColorValue none{};
    none.uint32[0] = 0xffffffffU;
    VkImageSubresourceRange whole{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vk.vkCmdClearColorImage(c, r.heads, VK_IMAGE_LAYOUT_GENERAL, &none, 1,
                            &whole);
    all.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    all.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    vk.vkCmdPipelineBarrier(c, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 1, &all,
                            0, nullptr, 0, nullptr);
    VkClearValue clears[2];
    clears[0].color.float32[0] = ((clear >> 16) & 0xff) / 255.0F;
    clears[0].color.float32[1] = ((clear >> 8) & 0xff) / 255.0F;
    clears[0].color.float32[2] = (clear & 0xff) / 255.0F;
    clears[0].color.float32[3] = ((clear >> 24) & 0xff) / 255.0F;
    clears[1].depthStencil = {0.0F, 0};
    VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    begin.renderPass = r.pass;
    begin.framebuffer = r.framebuffer;
    begin.renderArea = {{0, 0}, r.target};
    begin.clearValueCount = 2;
    begin.pClearValues = clears;
    vk.vkCmdBeginRenderPass(c, &begin, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport viewport{0.0F,
                        0.0F,
                        static_cast<float>(r.target.width),
                        static_cast<float>(r.target.height),
                        0.0F,
                        1.0F};
    VkRect2D scissor{{0, 0}, r.target};
    vk.vkCmdSetViewport(c, 0, 1, &viewport);
    vk.vkCmdSetScissor(c, 0, 1, &scissor);
    VkDeviceSize zero = 0;
    vk.vkCmdBindVertexBuffers(c, 0, 1, &f.vertices.buffer, &zero);
    vk.vkCmdBindDescriptorSets(c, VK_PIPELINE_BIND_POINT_GRAPHICS, r.layout, 0,
                               1, &f.picture_set, 0, nullptr);
    VkPipeline bound = VK_NULL_HANDLE;
    for (size_t i = 0; i < batches.size(); i++) {
        const Batch &b = batches[i];
        bool collect = sorted && b.state.blend;
        VkPipeline p = r.drawn[i].first;
        Push push{b.state.flags | (collect ? BATCH_COLLECT : 0),
                  (b.state.src & 7U) | (b.state.dst & 7U) << 3};
        VkDescriptorSet set = r.drawn[i].second;

        if (p != bound) {
            vk.vkCmdBindPipeline(c, VK_PIPELINE_BIND_POINT_GRAPHICS, p);
            bound = p;
        }
        vk.vkCmdBindDescriptorSets(c, VK_PIPELINE_BIND_POINT_GRAPHICS, r.layout,
                                   1, 1, &set, 0, nullptr);
        vk.vkCmdPushConstants(c, r.layout,
                              VK_SHADER_STAGE_VERTEX_BIT |
                                  VK_SHADER_STAGE_FRAGMENT_BIT,
                              0, sizeof push, &push);
        vk.vkCmdDraw(c, b.count, 1, b.first, 0);
    }
    vk.vkCmdEndRenderPass(c);
    if (sorted) {
        vk.vkCmdBindPipeline(c, VK_PIPELINE_BIND_POINT_COMPUTE, r.resolve);
        vk.vkCmdBindDescriptorSets(c, VK_PIPELINE_BIND_POINT_COMPUTE, r.layout,
                                   0, 1, &f.picture_set, 0, nullptr);
        vk.vkCmdDispatch(c, (r.target.width + GROUP - 1) / GROUP,
                         (r.target.height + GROUP - 1) / GROUP, 1);
        r.barrier(c, r.color, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
                  0, 1, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                  VK_PIPELINE_STAGE_TRANSFER_BIT);
    }
    /* copy the fragments' head to f.stats for lost and cut, after the draws
     * and the resolve write it */
    all.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    all.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    vk.vkCmdPipelineBarrier(c,
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &all, 0,
                            nullptr, 0, nullptr);
    VkBufferCopy list_head{0, 0, LIST_HEAD};
    vk.vkCmdCopyBuffer(c, r.fragments.buffer, f.stats.buffer, 1, &list_head);
    if (r.readback) {
        VkBufferImageCopy copy{};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageExtent = {WIDTH, HEIGHT, 1};
        /* a larger picture is read at WIDTH by HEIGHT by the area filter */
        if (r.readout.image != VK_NULL_HANDLE) {
            r.filter(c, r.readout);
        }
        vk.vkCmdCopyImageToBuffer(
            c, r.readout.image != VK_NULL_HANDLE ? r.readout.image : r.color,
            VK_IMAGE_LAYOUT_GENERAL, f.readback.buffer, 1, &copy);
    }
    if (swap_image != UINT32_MAX) {
        VkImage to = r.swap_images[swap_image];
        VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VkClearColorValue black{};

        r.barrier(c, to, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 1, 0,
                  VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                  VK_PIPELINE_STAGE_TRANSFER_BIT);
        vk.vkCmdClearColorImage(c, to, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                &black, 1, &range);
        r.barrier(c, to, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 1,
                  VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                  VK_PIPELINE_STAGE_TRANSFER_BIT);
        /* the whole picture goes to the window's region. one larger than the
         * region passes through the area filter first, and a smaller one is
         * stretched */
        VkRect2D shows = r.region();
        bool fitted = r.fitted.image != VK_NULL_HANDLE;
        VkExtent2D from = fitted ? r.fitted.extent : r.target;
        VkImageBlit blit{};

        if (fitted) {
            r.filter(c, r.fitted);
        }
        blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        blit.srcOffsets[1] = {static_cast<int32_t>(from.width),
                              static_cast<int32_t>(from.height), 1};
        blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        blit.dstOffsets[0] = {shows.offset.x, shows.offset.y, 0};
        blit.dstOffsets[1] = {
            shows.offset.x + static_cast<int32_t>(shows.extent.width),
            shows.offset.y + static_cast<int32_t>(shows.extent.height), 1};
        vk.vkCmdBlitImage(
            c, fitted ? r.fitted.image : r.color, VK_IMAGE_LAYOUT_GENERAL, to,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
        r.barrier(c, to, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, 1,
                  VK_ACCESS_TRANSFER_WRITE_BIT, 0,
                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    }
    if (r.timer != VK_NULL_HANDLE) {
        vk.vkCmdWriteTimestamp(c, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, r.timer,
                               frame * 2 + 1);
    }
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    VkPipelineStageFlags waits = VK_PIPELINE_STAGE_TRANSFER_BIT;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &c;
    if (swap_image != UINT32_MAX) {
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &f.acquired;
        submit.pWaitDstStageMask = &waits;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &r.rendered[swap_image];
    }
    if (!r.took(vk.vkEndCommandBuffer(c), "vkEndCommandBuffer", why) ||
        !r.took(vk.vkQueueSubmit(r.queue, 1, &submit, f.fence), "vkQueueSubmit",
                why)) {
        return r.breaks(why);
    }
    f.sent = true;
    if (swap_image != UINT32_MAX) {
        VkPresentInfoKHR show{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        VkResult shown;

        show.waitSemaphoreCount = 1;
        show.pWaitSemaphores = &r.rendered[swap_image];
        show.swapchainCount = 1;
        show.pSwapchains = &r.swapchain;
        show.pImageIndices = &swap_image;
        shown = vk.vkQueuePresentKHR(r.queue, &show);
        if (shown == VK_ERROR_OUT_OF_DATE_KHR || shown == VK_SUBOPTIMAL_KHR) {
            if (!r.make_swapchain(why)) {
                return false;
            }
        } else if (!r.took(shown, "vkQueuePresentKHR", why)) {
            return r.breaks(why);
        }
    }
    return true;
}

void Raster::wide(bool on)
{
    impl_->wide = on;
}

bool Raster::broken(std::string &why) const
{
    if (impl_ == nullptr || impl_->broken.empty()) {
        return false;
    }
    why = impl_->broken;
    return true;
}

bool Raster::done(uint32_t frame)
{
    Impl &r = *impl_;
    const Frame &f = r.frames[frame];
    std::string lost;

    if (!f.sent) {
        return true;
    }
    VkResult status = r.vk.vkGetFenceStatus(r.device, f.fence);

    /* a lost device runs nothing more of what it was given */
    if (status == VK_ERROR_DEVICE_LOST) {
        r.took(status, "vkGetFenceStatus", lost);
        return true;
    }
    return status == VK_SUCCESS;
}

void Raster::wait(uint32_t frame)
{
    Impl &r = *impl_;
    const Frame &f = r.frames[frame];
    std::string lost;

    if (f.sent) {
        r.waited(f.fence, lost);
    }
}

uint32_t Raster::lost(uint32_t frame)
{
    const Frame &f = impl_->frames[frame];

    return impl_->holds(f) ? static_cast<uint32_t *>(f.stats.mapped)[HEAD_LOST]
                           : 0;
}

uint32_t Raster::cut(uint32_t frame)
{
    const Frame &f = impl_->frames[frame];

    return impl_->holds(f) ? static_cast<uint32_t *>(f.stats.mapped)[HEAD_CUT]
                           : 0;
}

uint64_t Raster::gpu_ns(uint32_t frame)
{
    Impl &r = *impl_;
    uint64_t ticks[2] = {0, 0};

    if (!r.holds(r.frames[frame]) || r.timer == VK_NULL_HANDLE ||
        r.vk.vkGetQueryPoolResults(r.device, r.timer, frame * 2, 2,
                                   sizeof ticks, ticks, sizeof ticks[0],
                                   VK_QUERY_RESULT_64_BIT) != VK_SUCCESS) {
        return 0;
    }
    return static_cast<uint64_t>(static_cast<double>(ticks[1] - ticks[0]) *
                                 r.timer_period);
}

bool Raster::read(uint32_t frame, void *rgba)
{
    Frame &f = impl_->frames[frame];

    if (!impl_->holds(f) || f.readback.mapped == nullptr) {
        return false;
    }
    memcpy(rgba, f.readback.mapped, WIDTH * HEIGHT * 4);
    return true;
}

} // namespace pc
