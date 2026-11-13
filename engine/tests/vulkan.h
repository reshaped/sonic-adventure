/* a device of a test's own. vulkan.cpp builds a library named as Vulkan's
 * loader is, which a test's run finds first. the raster opens it as it opens
 * Vulkan, so every call it makes lands here. the device draws nothing. it
 * keeps what the calls made and fails the call a test names. faults counts
 * each use the device forbids, which a driver need not catch. a submission
 * does its work as it is made. it counts as running until vkWaitForFences on
 * its fence succeeds or vkGetFenceStatus finds that fence signaled.
 * every submission counts as finished once vkDeviceWaitIdle succeeds. while
 * a submission runs, nothing its commands name may end. its command buffer
 * may not begin again, nor its fence reset. nothing runs while the device is
 * lost */
#ifndef ENGINE_TESTS_VULKAN_H
#define ENGINE_TESTS_VULKAN_H

#include <stdint.h>

#include <vector>

#include <vulkan/vulkan_core.h>

/* a draw a submission ran, with its push constants and vertices. view and
 * sampler are those set 1 binds for its texture */
struct TestDraw {
    uint32_t push[2];
    uint32_t first;
    uint32_t count;
    VkImageView view;
    VkSamplerCreateInfo sampler;
};

struct TestDevice {
    /* max_bias is the largest sampler bias the device allows. width and
     * height are its surface's size, which a test keeps as its window's */
    float max_bias = 4.0F;
    uint32_t width = 0;
    uint32_t height = 0;
    /* the names of the devices the next open finds listed, in order. with
     * none, it finds the one device, under a name of the test's own */
    std::vector<const char *> names;
    /* fail_nth fails the nth call from now that returns a result, or the
     * nth named fail_name, with fail_result. 0 fails none.
     * vkEnumerateDeviceExtensionProperties and vkFreeDescriptorSets never
     * fail. after a VK_ERROR_DEVICE_LOST every call that returns a result
     * fails the same way until vkCreateInstance. a fail_run past 1 fails
     * that many in turn. fail_nth stays 1 while more of the run would fail.
     * vkAcquireNextImageKHR still does its work under a result that is no
     * error, and vkQueuePresentKHR under any */
    const char *fail_name = nullptr;
    uint32_t fail_nth = 0;
    uint32_t fail_run = 1;
    VkResult fail_result = VK_ERROR_OUT_OF_DEVICE_MEMORY;
    /* a call the loader has none of, or null */
    const char *lacks = nullptr;
    /* each call made that can fail, in order, and the one failed */
    std::vector<const char *> calls;
    const char *failed = nullptr;
    uint32_t faults = 0;
    char fault[256] = ""; /* the first */
    uint32_t live = 0; /* objects made and not destroyed */
    std::vector<VkSamplerCreateInfo> samplers; /* each made, in order */
    std::vector<TestDraw> draws; /* each run, in order */
    /* the shaders, as a test plays them. a draw calls shaded and a dispatch
     * calls resolved, with the head of the blend list bound. with none
     * bound, neither is called */
    void (*shaded)(uint32_t *head) = nullptr;
    void (*resolved)(uint32_t *head) = nullptr;
};

extern "C" TestDevice *test_device(void);

#endif
