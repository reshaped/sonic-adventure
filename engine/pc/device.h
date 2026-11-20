/* the raster's device, which raster.cpp creates and rays.cpp traces on: the
 * calls it loads from Vulkan, what a frame and a texture hold, and the raster's
 * own state. only those two sources include this, so Vulkan's types stay out of
 * raster.h */
#ifndef ENGINE_PC_DEVICE_H
#define ENGINE_PC_DEVICE_H

#define VK_NO_PROTOTYPES
#include <stdint.h>

#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "raster.h"
#include "rays.h"

namespace pc {

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

/* the calls ray tracing makes. open loads them where the device may trace, and
 * requires none of them: a raster whose instance or device lacks one only
 * rasterizes */
#define RAY_INSTANCE_CALLS(X)                                                  \
    X(vkGetPhysicalDeviceFeatures2)                                            \
    X(vkGetPhysicalDeviceProperties2)
#define RAY_CALLS(X)                                                           \
    X(vkGetBufferDeviceAddress)                                                \
    X(vkCreateAccelerationStructureKHR)                                        \
    X(vkDestroyAccelerationStructureKHR)                                       \
    X(vkGetAccelerationStructureBuildSizesKHR)                                 \
    X(vkGetAccelerationStructureDeviceAddressKHR)                              \
    X(vkCmdBuildAccelerationStructuresKHR)

#define DECLARE(name) PFN_##name name = nullptr;
struct Calls {
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
    GLOBAL_CALLS(DECLARE)
    INSTANCE_CALLS(DECLARE)
    SURFACE_CALLS(DECLARE)
    DEVICE_CALLS(DECLARE)
    SWAPCHAIN_CALLS(DECLARE)
    RAY_INSTANCE_CALLS(DECLARE)
    RAY_CALLS(DECLARE)
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
    /* a traced picture's buffers, on a device that traces: its instances as the
     * top-level build reads them, by address, then as the shaders read them,
     * its materials and its uniforms. ray_set binds the top-level structure,
     * the picture's uniforms and these buffers */
    Buffer ray_places;
    /* ray_places' address, for the top-level build */
    VkDeviceAddress ray_address = 0;
    Buffer ray_instances;
    Buffer ray_materials;
    Buffer ray_uniform;
    Buffer ray_ramps; /* each light's ramp, as the shaders' Ramps declares it */
    Buffer ray_lamps; /* the picture's lamps, as lamp_table() lays them out */
    VkDescriptorSet ray_set = VK_NULL_HANDLE;
};

/* the compute shaders' local size, each way */
constexpr uint32_t GROUP = 8;

/* what differs between the pipelines that draw in the target's render pass: the
 * shaders and their layout, the vertex input, and a triangle's culling, depth
 * test and blend */
struct Drawing {
    VkShaderModule vertex = VK_NULL_HANDLE;
    VkShaderModule fragment = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipelineVertexInputStateCreateInfo input = {};
    VkCullModeFlags cull = VK_CULL_MODE_NONE;
    VkPipelineDepthStencilStateCreateInfo depth = {};
    VkPipelineColorBlendAttachmentState attachment = {};

    Drawing()
    {
        input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        depth.sType =
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    }
};

/* what a device that traces is created with, as ray_device() fills it in: the
 * features ray queries need, each in a struct that enables nothing else, one
 * core feature, and the extensions. open chains the three structs in place and
 * enables the extensions */
struct RayNeeds {
    VkPhysicalDeviceRayQueryFeaturesKHR query = {};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR structure = {};
    VkPhysicalDeviceVulkan12Features core = {};
    /* VkPhysicalDeviceFeatures' shaderSampledImageArrayDynamicIndexing */
    bool indexing = false;
    const char *extensions[3] = {};
};

/* an image the area filter writes from a larger target. readout takes the
 * picture at WIDTH by HEIGHT for read, and fitted at the window's region */
struct Scaled {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkDescriptorSet set = VK_NULL_HANDLE;
    VkExtent2D extent{};
};

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
    /* a load is the commands that copy the textures and build the meshes
     * added since the last, sent in one submission. it is sent before a
     * picture, where the staging lacks room, and before a thing it names
     * ends. its bytes lie in staging from its start */
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
    uint64_t timer_mask = 0; /* a timestamp's valid bits, each set */
    uint32_t resolution = 1; /* RasterOptions', 0 only with a window */
    float render_scale = 1.0F; /* RasterOptions' */
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
    /* rays.cpp's state: the meshes, the top-level structure and the passes that
     * trace a picture. null on a device created without ray queries */
    struct Rays;
    Rays *rays = nullptr;

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
    /* a pipeline that draws triangles in the target's render pass as d
     * describes. null, with why, where the device refuses it */
    VkPipeline make_pipeline(const Drawing &d, std::string &why);
    VkPipeline pipeline(const State &s, bool collect, std::string &why);
    VkDescriptorSet texture_set(uint32_t texture, uint32_t sample,
                                std::string &why);
    void barrier(VkCommandBuffer c, VkImage image, VkImageLayout from,
                 VkImageLayout to, uint32_t level, uint32_t levels,
                 VkAccessFlags src, VkAccessFlags dst,
                 VkPipelineStageFlags src_stage,
                 VkPipelineStageFlags dst_stage);

    /* rays.cpp's functions. open calls ray_device and make_rays. free_rays,
     * drop_rays and free_ray_frame are safe on a raster without ray tracing,
     * and every other one requires rays */
    /* whether the chosen device can trace: it has ray queries, the features and
     * extensions they need, and limits that fit the ray tracing descriptor
     * sets. needs then holds what open creates the device with */
    bool ray_device(RayNeeds &needs);
    /* create the ray tracing layouts, pipelines and descriptor sets, before the
     * frames. false, with why, where the device refuses one. what was created
     * is then freed with the raster */
    bool make_rays(std::string &why);
    void free_rays();
    /* drop the ray tracing state without a call to the device, which may still
     * be reading it. what the device holds is left until the process exits */
    void drop_rays();
    bool make_ray_frame(Frame &f, std::string &why);
    void free_ray_frame(Frame &f);
    /* create the ray tracing images at the target's size. c, the load being
     * recorded, moves them to the GENERAL layout */
    bool make_ray_target(VkExtent2D size, VkCommandBuffer c, std::string &why);
    void free_ray_target();
    /* record the builds of the meshes added since the last load, as the last
     * commands of the load c. the next load's builds reuse the scratch
     * buffer */
    void ray_build(VkCommandBuffer c);
    /* a picture was rasterized without tracing, so the next traced picture
     * reuses no history */
    void ray_skipped();
    uint32_t ray_add_mesh(const RayMesh &mesh, std::string &why);
    void ray_free_mesh(uint32_t mesh);
    /* texture's image was freed, so its index in the shaders' texture array is
     * marked unbound, and a later texture with that index is bound afresh */
    void ray_free_texture(uint32_t texture);
    /* prepare f to trace p, before the picture's commands: capacity for its
     * instances, the buffers' contents, and the textures it uses. false, with
     * why, where the device refuses, and no command has been recorded then */
    bool ray_ready(Frame &f, const RayPicture &p, std::string &why);
    /* record the top-level build and the passes that trace f's picture */
    void ray_trace(VkCommandBuffer c, Frame &f);
    /* inside the target's render pass: draw the traced picture and its depth,
     * before the batches */
    void ray_show(VkCommandBuffer c);
};

} // namespace pc

#endif
