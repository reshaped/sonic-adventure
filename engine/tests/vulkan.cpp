/* vulkan.h's device: Vulkan's loader and a driver behind it, as far as the
 * PC's raster calls them. an object is never freed, so a handle destroyed
 * is told from one no call made */
#include "vulkan.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <set>

namespace {

TestDevice control;
/* true from the call a test fails as a loss */
bool lost = false;
/* what a call that fails leaves in a handle it would have made, which
 * Vulkan leaves undefined */
char poison;

enum Kind {
    INSTANCE,
    PHYSICAL,
    DEVICE,
    QUEUE,
    POOL,
    COMMANDS,
    FENCE,
    SEMAPHORE,
    BUFFER,
    IMAGE,
    VIEW,
    MEMORY,
    PASS,
    FRAMEBUFFER,
    MODULE,
    SET_LAYOUT,
    LAYOUT,
    PIPELINE,
    DESCRIPTORS,
    SET,
    SAMPLER,
    QUERIES,
    SWAPCHAIN
};
const char *const KINDS[] = {
    "instance",        "physical device", "device",          "queue",
    "command pool",    "command buffer",  "fence",           "semaphore",
    "buffer",          "image",           "image view",      "memory",
    "render pass",     "framebuffer",     "shader module",   "set layout",
    "pipeline layout", "pipeline",        "descriptor pool", "descriptor set",
    "sampler",         "query pool",      "swapchain"};

/* a command buffer's states */
enum { INITIAL, RECORDING, EXECUTABLE, INVALID };

struct Object;

/* a command whose work a submission does */
struct Op {
    enum { UPDATE, COPY, DRAW, DISPATCH } what = UPDATE;
    Object *from = nullptr;
    Object *to = nullptr;
    VkDeviceSize from_at = 0;
    VkDeviceSize to_at = 0;
    VkDeviceSize size = 0;
    std::vector<uint8_t> bytes; /* an update's */
    TestDraw draw{};
    Object *list = nullptr; /* the blend list's buffer, in the set bound */
};

struct Object {
    Kind kind = INSTANCE;
    bool live = true;
    Object *owner = nullptr; /* a pool or a swapchain, whose end ends it */
    /* an instance or a device with the window's extensions */
    bool shows = false;
    /* a command buffer: its state and bindings, the objects its commands
     * name and the ops a submission runs */
    int state = INITIAL;
    bool once = false;
    bool in_pass = false;
    Object *pipeline[2] = {};
    Object *sets[2][2] = {};
    Object *vertices = nullptr;
    uint32_t push[2] = {};
    std::vector<Object *> names;
    std::vector<Op> ops;
    /* submitted, with nothing yet to tell the host the device has finished
     * it, and the fence that will */
    bool running = false;
    Object *until = nullptr;
    /* the stages of shaders that wrote the blend list since a barrier let
     * what follows them read it */
    VkPipelineStageFlags wrote = 0;
    /* a fence or a semaphore */
    bool signaled = false;
    /* a buffer or an image: its memory, and where in it */
    Object *memory = nullptr;
    VkDeviceSize at = 0;
    VkDeviceSize size = 0;
    /* memory: its bytes, its type, and whether the host has it mapped */
    uint8_t *bytes = nullptr;
    uint32_t type = 0;
    bool mapped = false;
    /* a view's image, a framebuffer's views, and a set's bindings and its
     * texture's sampler. each binding is a view or a buffer. sampler_at is a
     * sampler's index in control.samplers */
    Object *image = nullptr;
    Object *views[2] = {};
    Object *bound[4] = {};
    Object *sampler = nullptr;
    uint32_t sampler_at = 0;
    bool compute = false; /* a pipeline */
    /* a swapchain's images, the one acquired and not shown or -1, and
     * whether it is retired */
    std::vector<Object *> images;
    uint32_t next = 0;
    int out = -1;
    bool retired = false;
};

std::set<const void *> made;
/* the physical devices listed: one for each of control.names, or the one */
std::vector<Object> physical(1);
Object queue;

__attribute__((format(printf, 1, 2))) void fault(const char *format, ...)
{
    va_list args;

    if (control.faults++ == 0) {
        va_start(args, format);
        vsnprintf(control.fault, sizeof control.fault, format, args);
        va_end(args);
    }
}

Object *make(Kind kind, Object *owner = nullptr)
{
    auto *object = new Object;

    object->kind = kind;
    object->owner = owner;
    made.insert(object);
    control.live++;
    return object;
}

/* the object a handle names, where it is a live one of kind: else a fault,
 * and null */
Object *use(const void *handle, Kind kind, const char *call)
{
    auto *object = static_cast<Object *>(const_cast<void *>(handle));

    if (handle == nullptr) {
        fault("%s takes no %s", call, KINDS[kind]);
    } else if (made.count(handle) == 0) {
        fault("%s takes for its %s a handle no call made", call, KINDS[kind]);
    } else if (object->kind != kind) {
        fault("%s takes for its %s a handle of another kind", call,
              KINDS[kind]);
    } else if (!object->live) {
        fault("%s takes for its %s one already destroyed", call, KINDS[kind]);
    } else {
        return object;
    }
    return nullptr;
}

void end(Object *object)
{
    object->live = false;
    free(object->bytes);
    object->bytes = nullptr;
    control.live--;
}

/* whether a call fails, as the test asked, and with what */
bool fails(const char *call, VkResult &result)
{
    control.calls.push_back(call);
    if (lost) {
        result = VK_ERROR_DEVICE_LOST;
        return true;
    }
    if (control.fail_nth == 0 ||
        (control.fail_name != nullptr &&
         strcmp(control.fail_name, call) != 0) ||
        --control.fail_nth != 0) {
        return false;
    }
    control.failed = call;
    result = control.fail_result;
    lost = result == VK_ERROR_DEVICE_LOST;
    if (control.fail_run > 1) {
        control.fail_run--;
        control.fail_nth = 1;
    }
    return true;
}

/* the host has seen fence signaled, so the submissions that signal it have
 * finished. for a null fence, the device is idle and all have finished */
void finished(const Object *fence)
{
    for (const void *each : made) {
        auto *object = static_cast<Object *>(const_cast<void *>(each));

        if (object->kind == COMMANDS &&
            (fence == nullptr || object->until == fence)) {
            object->running = false;
        }
    }
}

/* whether a submission that signals fence may still be running, or any for
 * a null fence. a lost device runs nothing */
bool running(const Object *fence = nullptr)
{
    for (const void *each : made) {
        auto *object = static_cast<const Object *>(each);

        if (!lost && object->kind == COMMANDS && object->live &&
            object->running && (fence == nullptr || object->until == fence)) {
            return true;
        }
    }
    return false;
}

/* count a fault where object ends while a running submission names it or
 * is it */
void unread(const Object *object, const char *call)
{
    if (!running()) {
        return;
    }
    if (object->kind == COMMANDS && object->running) {
        fault("%s ends a command buffer the device may still run", call);
        return;
    }
    for (const void *each : made) {
        auto *commands = static_cast<const Object *>(each);

        if (commands->kind != COMMANDS || !commands->live ||
            !commands->running) {
            continue;
        }
        for (const Object *named : commands->names) {
            if (named == object) {
                fault("%s ends a %s the device may still read", call,
                      KINDS[object->kind]);
                return;
            }
        }
    }
}

/* a call that makes one object of device's */
template <typename Handle>
VkResult create(VkDevice device, const char *call, Kind kind, Handle *handle,
                Object **object = nullptr)
{
    VkResult result;

    if (use(device, DEVICE, call) == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (fails(call, result)) {
        *handle = reinterpret_cast<Handle>(&poison);
        return result;
    }
    Object *one = make(kind);

    *handle = reinterpret_cast<Handle>(one);
    if (object != nullptr) {
        *object = one;
    }
    return VK_SUCCESS;
}

/* a call that destroys one, where a null handle names none */
void destroy(VkDevice device, const void *handle, Kind kind, const char *call)
{
    Object *object = nullptr;

    if (use(device, DEVICE, call) != nullptr && handle != nullptr &&
        (object = use(handle, kind, call)) != nullptr) {
        if (object->owner != nullptr && object->owner->kind == SWAPCHAIN) {
            fault("%s takes a swapchain's image", call);
            return;
        }
        unread(object, call);
        end(object);
    }
}

/* end what a pool or a swapchain owns with it */
void end_owned(Object *owner, const char *call)
{
    for (const void *each : made) {
        auto *object = static_cast<Object *>(const_cast<void *>(each));

        if (object->live && object->owner == owner) {
            unread(object, call);
            end(object);
        }
    }
}

/* the buffer commands names, where it may record a command. in_pass is 1
 * inside a render pass, 0 outside it and -1 either */
Object *recording(VkCommandBuffer commands, int in_pass, const char *call)
{
    Object *c = use(commands, COMMANDS, call);

    if (c == nullptr) {
        return nullptr;
    }
    if (c->state != RECORDING) {
        fault("%s takes a command buffer that isn't recording", call);
        return nullptr;
    }
    if (in_pass >= 0 && c->in_pass != (in_pass != 0)) {
        fault("%s is %s a render pass", call, in_pass ? "outside" : "inside");
        return nullptr;
    }
    return c;
}

/* a command names an object, which must live until its buffer has run */
Object *name(Object *commands, const void *handle, Kind kind, const char *call)
{
    Object *object = use(handle, kind, call);

    if (object != nullptr) {
        commands->names.push_back(object);
    }
    return object;
}

/* a buffer or an image a command reads or writes has its memory */
Object *backed(Object *commands, const void *handle, Kind kind,
               const char *call)
{
    Object *object = name(commands, handle, kind, call);

    if (object != nullptr && object->memory == nullptr &&
        object->owner == nullptr) {
        fault("%s takes for its %s one with no memory", call, KINDS[kind]);
        return nullptr;
    }
    return object;
}

/* the sets bound for a draw or a dispatch name live objects alone */
bool bound(Object *commands, int point, uint32_t count, const char *call)
{
    for (uint32_t i = 0; i < count; i++) {
        Object *set = commands->sets[point][i];

        if (set == nullptr || !set->live) {
            fault("%s runs with no set %u bound", call, i);
            return false;
        }
        commands->names.push_back(set);
        for (Object *each : {set->bound[0], set->bound[1], set->bound[2],
                             set->bound[3], set->sampler}) {
            if (each != nullptr && !each->live) {
                fault("%s runs with a set that names a destroyed %s", call,
                      KINDS[each->kind]);
                return false;
            }
            if (each != nullptr) {
                commands->names.push_back(each);
            }
        }
    }
    return true;
}

/* bind a buffer or an image to memory: once, within it, and over no other
 * that lives there */
VkResult bind(Object *object, VkDeviceMemory handle, VkDeviceSize at,
              const char *call)
{
    Object *memory = use(handle, MEMORY, call);
    VkResult result;

    if (object == nullptr || memory == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (fails(call, result)) {
        return result;
    }
    if (object->memory != nullptr) {
        fault("%s binds its %s a second time", call, KINDS[object->kind]);
    }
    if (object->kind == IMAGE && memory->type != 0) {
        fault("%s binds an image to a memory type it can't take", call);
    }
    if (at + object->size > memory->size) {
        fault("%s binds past the end of its memory", call);
    }
    for (const void *each : made) {
        auto *other = static_cast<Object *>(const_cast<void *>(each));

        if (other->live && other != object && other->memory == memory &&
            at < other->at + other->size && other->at < at + object->size) {
            fault("%s binds over memory a live %s holds", call,
                  KINDS[other->kind]);
        }
    }
    object->memory = memory;
    object->at = at;
    return VK_SUCCESS;
}

/* a buffer's bytes, where the host or a submission may reach them */
uint8_t *bytes_of(Object *buffer)
{
    return buffer != nullptr && buffer->memory != nullptr &&
                   buffer->memory->live
               ? buffer->memory->bytes + buffer->at
               : nullptr;
}

void run(Object *commands)
{
    for (const Op &op : commands->ops) {
        uint8_t *to = bytes_of(op.to);
        const uint8_t *from = bytes_of(op.from);
        auto *head = reinterpret_cast<uint32_t *>(bytes_of(op.list));

        switch (op.what) {
        case Op::UPDATE:
            if (to != nullptr) {
                memcpy(to + op.to_at, op.bytes.data(), op.size);
            }
            break;
        case Op::COPY:
            if (to != nullptr && from != nullptr) {
                memcpy(to + op.to_at, from + op.from_at, op.size);
            }
            break;
        case Op::DRAW:
            control.draws.push_back(op.draw);
            if (control.shaded != nullptr && head != nullptr) {
                control.shaded(head);
            }
            break;
        case Op::DISPATCH:
            if (control.resolved != nullptr && head != nullptr) {
                control.resolved(head);
            }
            break;
        }
    }
}

/* a semaphore a call waits for is signaled, and one it signals is not */
void turn(const VkSemaphore *semaphores, uint32_t count, bool to,
          const char *call)
{
    for (uint32_t i = 0; i < count; i++) {
        Object *semaphore = use(semaphores[i], SEMAPHORE, call);

        if (semaphore != nullptr && semaphore->signaled == to) {
            fault("%s %s", call,
                  to ? "signals a semaphore still signaled"
                     : "waits for a semaphore nothing signals");
        }
        if (semaphore != nullptr) {
            semaphore->signaled = to;
        }
    }
}

bool window_call(const char *name)
{
    return strstr(name, "KHR") != nullptr;
}

} // namespace

extern "C" TestDevice *test_device(void)
{
    return &control;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateInstance(const VkInstanceCreateInfo *info,
                 const VkAllocationCallbacks *, VkInstance *instance)
{
    VkResult result;

    lost = false;
    if (fails(__func__, result)) {
        *instance = reinterpret_cast<VkInstance>(&poison);
        return result;
    }
    Object *one = make(INSTANCE);

    one->shows = info->enabledExtensionCount != 0;
    *instance = reinterpret_cast<VkInstance>(one);
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(VkInstance instance,
                                             const VkAllocationCallbacks *)
{
    Object *one = use(instance, INSTANCE, __func__);

    for (const void *each : made) {
        auto *object = static_cast<const Object *>(each);

        if (one != nullptr && object->live && object->kind == DEVICE) {
            fault("%s ends an instance whose device still lives", __func__);
        }
    }
    if (one != nullptr) {
        end(one);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(
    VkInstance instance, uint32_t *count, VkPhysicalDevice *devices)
{
    VkResult result;

    if (use(instance, INSTANCE, __func__) == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (fails(__func__, result)) {
        return result;
    }
    size_t listed = control.names.empty() ? 1 : control.names.size();

    physical.resize(listed);
    for (uint32_t i = 0; devices != nullptr && i < *count && i < listed; i++) {
        devices[i] = reinterpret_cast<VkPhysicalDevice>(&physical[i]);
    }
    *count = static_cast<uint32_t>(listed);
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(
    VkPhysicalDevice device, VkPhysicalDeviceProperties *properties)
{
    size_t at = static_cast<size_t>(reinterpret_cast<Object *>(device) -
                                    physical.data());

    memset(properties, 0, sizeof *properties);
    properties->apiVersion = VK_API_VERSION_1_1;
    snprintf(properties->deviceName, sizeof properties->deviceName, "%s",
             at < control.names.size() ? control.names[at]
                                       : "a device of the test's own");
    properties->limits.maxImageDimension2D = 16384;
    properties->limits.maxStorageBufferRange = 1U << 20;
    properties->limits.maxSamplerLodBias = control.max_bias;
    properties->limits.timestampPeriod = 1.0F;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice, uint32_t *count, VkQueueFamilyProperties *families)
{
    if (families != nullptr && *count != 0) {
        families[0] = {
            VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 1, 64, {1, 1, 1}};
    }
    *count = 1;
}

/* two types of memory: the device's own, and the host's, cached */
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice, VkPhysicalDeviceMemoryProperties *properties)
{
    memset(properties, 0, sizeof *properties);
    properties->memoryTypeCount = 2;
    properties->memoryTypes[0].propertyFlags =
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    properties->memoryTypes[1].propertyFlags =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    properties->memoryHeapCount = 1;
    properties->memoryHeaps[0].size = VkDeviceSize{1} << 32;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures(
    VkPhysicalDevice, VkPhysicalDeviceFeatures *features)
{
    memset(features, 0, sizeof *features);
    features->fragmentStoresAndAtomics = VK_TRUE;
    features->shaderClipDistance = VK_TRUE;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice,
                                              const VkDeviceCreateInfo *info,
                                              const VkAllocationCallbacks *,
                                              VkDevice *device)
{
    VkResult result;

    if (fails(__func__, result)) {
        *device = reinterpret_cast<VkDevice>(&poison);
        return result;
    }
    Object *one = make(DEVICE);

    one->shows = info->enabledExtensionCount != 0;
    *device = reinterpret_cast<VkDevice>(one);
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(VkDevice device,
                                           const VkAllocationCallbacks *)
{
    Object *one = use(device, DEVICE, __func__);

    for (const void *each : made) {
        auto *object = static_cast<const Object *>(each);

        if (one != nullptr && object->live && object->kind != DEVICE &&
            object->kind != INSTANCE) {
            fault("%s ends a device whose %s still lives", __func__,
                  KINDS[object->kind]);
        }
    }
    if (one != nullptr) {
        end(one);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice, const char *, uint32_t *count, VkExtensionProperties *)
{
    *count = 0;
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkDestroySurfaceKHR(VkInstance instance,
                                               VkSurfaceKHR,
                                               const VkAllocationCallbacks *)
{
    use(instance, INSTANCE, __func__);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceSupportKHR(
    VkPhysicalDevice, uint32_t, VkSurfaceKHR, VkBool32 *shows)
{
    VkResult result;

    if (fails(__func__, result)) {
        return result;
    }
    *shows = VK_TRUE;
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    VkPhysicalDevice, VkSurfaceKHR, VkSurfaceCapabilitiesKHR *has)
{
    VkResult result;

    if (fails(__func__, result)) {
        return result;
    }
    memset(has, 0, sizeof *has);
    has->minImageCount = 2;
    has->currentExtent = {control.width, control.height};
    has->currentTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    has->supportedCompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceFormatsKHR(
    VkPhysicalDevice, VkSurfaceKHR, uint32_t *count,
    VkSurfaceFormatKHR *formats)
{
    VkResult result;

    if (fails(__func__, result)) {
        return result;
    }
    if (formats != nullptr && *count != 0) {
        formats[0] = {VK_FORMAT_B8G8R8A8_UNORM,
                      VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }
    *count = 1;
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfacePresentModesKHR(
    VkPhysicalDevice, VkSurfaceKHR, uint32_t *count, VkPresentModeKHR *modes)
{
    VkResult result;

    if (fails(__func__, result)) {
        return result;
    }
    if (modes != nullptr && *count != 0) {
        modes[0] = VK_PRESENT_MODE_FIFO_KHR;
    }
    *count = 1;
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(VkDevice device, uint32_t, uint32_t,
                                            VkQueue *out)
{
    use(device, DEVICE, __func__);
    *out = reinterpret_cast<VkQueue>(&queue);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *,
                    const VkAllocationCallbacks *, VkCommandPool *pool)
{
    return create(device, __func__, POOL, pool);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyCommandPool(VkDevice device,
                                                VkCommandPool pool,
                                                const VkAllocationCallbacks *)
{
    if (pool != VK_NULL_HANDLE && use(pool, POOL, __func__) != nullptr) {
        end_owned(reinterpret_cast<Object *>(pool), __func__);
    }
    destroy(device, pool, POOL, __func__);
}

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(
    VkDevice device, const VkCommandBufferAllocateInfo *info,
    VkCommandBuffer *commands)
{
    Object *pool = use(info->commandPool, POOL, __func__);
    VkResult result;

    if (use(device, DEVICE, __func__) == nullptr || pool == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (fails(__func__, result)) {
        for (uint32_t i = 0; i < info->commandBufferCount; i++) {
            commands[i] = VK_NULL_HANDLE;
        }
        return result;
    }
    for (uint32_t i = 0; i < info->commandBufferCount; i++) {
        commands[i] = reinterpret_cast<VkCommandBuffer>(make(COMMANDS, pool));
    }
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkFreeCommandBuffers(VkDevice device, VkCommandPool,
                                                uint32_t count,
                                                const VkCommandBuffer *commands)
{
    for (uint32_t i = 0; i < count; i++) {
        Object *c = commands[i] != VK_NULL_HANDLE
                        ? use(commands[i], COMMANDS, __func__)
                        : nullptr;

        if (use(device, DEVICE, __func__) != nullptr && c != nullptr) {
            end(c);
        }
    }
}

VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer(
    VkCommandBuffer commands, const VkCommandBufferBeginInfo *info)
{
    Object *c = use(commands, COMMANDS, __func__);
    VkResult result;

    if (c == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (fails(__func__, result)) {
        return result;
    }
    if (c->state == RECORDING) {
        fault("%s takes a command buffer that is recording", __func__);
    }
    if (c->running && !lost) {
        fault("%s takes a command buffer the device may still run", __func__);
    }
    c->state = RECORDING;
    c->once = (info->flags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) != 0;
    c->in_pass = false;
    c->vertices = nullptr;
    for (int point = 0; point < 2; point++) {
        c->pipeline[point] = nullptr;
        c->sets[point][0] = c->sets[point][1] = nullptr;
    }
    c->names.clear();
    c->ops.clear();
    c->wrote = 0;
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEndCommandBuffer(VkCommandBuffer commands)
{
    Object *c = use(commands, COMMANDS, __func__);
    VkResult result;

    if (c == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (c->state != RECORDING) {
        fault("%s takes a command buffer that isn't recording", __func__);
    } else if (c->in_pass) {
        fault("%s ends a command buffer inside its render pass", __func__);
    }
    if (fails(__func__, result)) {
        c->state = INVALID;
        return result;
    }
    c->state = EXECUTABLE;
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(VkQueue, uint32_t count,
                                             const VkSubmitInfo *submits,
                                             VkFence fence)
{
    Object *signals =
        fence != VK_NULL_HANDLE ? use(fence, FENCE, __func__) : nullptr;
    VkResult result;

    if (fails(__func__, result)) {
        return result;
    }
    if (signals != nullptr && signals->signaled) {
        fault("%s takes a fence still signaled", __func__);
    }
    for (uint32_t i = 0; i < count; i++) {
        const VkSubmitInfo &submit = submits[i];

        turn(submit.pWaitSemaphores, submit.waitSemaphoreCount, false,
             __func__);
        for (uint32_t k = 0; k < submit.commandBufferCount; k++) {
            Object *c = use(submit.pCommandBuffers[k], COMMANDS, __func__);

            if (c == nullptr) {
                continue;
            }
            if (c->state != EXECUTABLE) {
                fault("%s takes a command buffer with no commands ended",
                      __func__);
                continue;
            }
            for (const Object *named : c->names) {
                if (!named->live) {
                    fault("%s runs commands that name a destroyed %s", __func__,
                          KINDS[named->kind]);
                }
            }
            run(c);
            c->state = c->once ? INVALID : EXECUTABLE;
            c->running = true;
            c->until = signals;
        }
        turn(submit.pSignalSemaphores, submit.signalSemaphoreCount, true,
             __func__);
    }
    if (signals != nullptr) {
        signals->signaled = true;
    }
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkDeviceWaitIdle(VkDevice device)
{
    VkResult result;

    if (use(device, DEVICE, __func__) == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (fails(__func__, result)) {
        return result;
    }
    finished(nullptr);
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateImage(VkDevice device,
                                             const VkImageCreateInfo *info,
                                             const VkAllocationCallbacks *,
                                             VkImage *image)
{
    Object *one = nullptr;
    VkResult result = create(device, __func__, IMAGE, image, &one);

    if (result == VK_SUCCESS) {
        one->size = VkDeviceSize{info->extent.width} * info->extent.height *
                    (info->mipLevels > 1 ? 6 : 4);
        one->size = (one->size + 4095) & ~VkDeviceSize{4095};
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyImage(VkDevice device, VkImage image,
                                          const VkAllocationCallbacks *)
{
    destroy(device, image, IMAGE, __func__);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateImageView(VkDevice device, const VkImageViewCreateInfo *info,
                  const VkAllocationCallbacks *, VkImageView *view)
{
    Object *image = use(info->image, IMAGE, __func__);
    Object *one = nullptr;

    if (image != nullptr && image->memory == nullptr &&
        image->owner == nullptr) {
        fault("%s takes an image with no memory", __func__);
    }
    VkResult result = create(device, __func__, VIEW, view, &one);

    if (result == VK_SUCCESS) {
        one->image = image;
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyImageView(VkDevice device, VkImageView view,
                                              const VkAllocationCallbacks *)
{
    destroy(device, view, VIEW, __func__);
}

VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements(
    VkDevice device, VkImage image, VkMemoryRequirements *need)
{
    Object *one = use(image, IMAGE, __func__);

    use(device, DEVICE, __func__);
    *need = {one != nullptr ? one->size : 4096, 4096, 1};
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory(VkDevice device, VkImage image,
                                                 VkDeviceMemory memory,
                                                 VkDeviceSize at)
{
    use(device, DEVICE, __func__);
    return bind(use(image, IMAGE, __func__), memory, at, __func__);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateBuffer(VkDevice device,
                                              const VkBufferCreateInfo *info,
                                              const VkAllocationCallbacks *,
                                              VkBuffer *buffer)
{
    Object *one = nullptr;
    VkResult result = create(device, __func__, BUFFER, buffer, &one);

    if (result == VK_SUCCESS) {
        one->size = info->size;
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(VkDevice device, VkBuffer buffer,
                                           const VkAllocationCallbacks *)
{
    destroy(device, buffer, BUFFER, __func__);
}

VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements(
    VkDevice device, VkBuffer buffer, VkMemoryRequirements *need)
{
    Object *one = use(buffer, BUFFER, __func__);

    use(device, DEVICE, __func__);
    *need = {one != nullptr ? (one->size + 255) & ~VkDeviceSize{255} : 256, 256,
             3};
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory(VkDevice device,
                                                  VkBuffer buffer,
                                                  VkDeviceMemory memory,
                                                  VkDeviceSize at)
{
    use(device, DEVICE, __func__);
    return bind(use(buffer, BUFFER, __func__), memory, at, __func__);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo *info,
                 const VkAllocationCallbacks *, VkDeviceMemory *memory)
{
    Object *one = nullptr;

    if (info->memoryTypeIndex > 1) {
        fault("%s takes a memory type the device lacks", __func__);
    }
    VkResult result = create(device, __func__, MEMORY, memory, &one);

    if (result == VK_SUCCESS) {
        one->size = info->allocationSize;
        one->type = info->memoryTypeIndex;
        one->bytes = static_cast<uint8_t *>(calloc(1, info->allocationSize));
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkFreeMemory(VkDevice device, VkDeviceMemory memory,
                                        const VkAllocationCallbacks *)
{
    destroy(device, memory, MEMORY, __func__);
}

VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory(VkDevice device,
                                           VkDeviceMemory memory, VkDeviceSize,
                                           VkDeviceSize, VkMemoryMapFlags,
                                           void **mapped)
{
    Object *one = use(memory, MEMORY, __func__);
    VkResult result;

    if (use(device, DEVICE, __func__) == nullptr || one == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (fails(__func__, result)) {
        *mapped = reinterpret_cast<void *>(uintptr_t{16});
        return result;
    }
    if (one->type != 1) {
        fault("%s maps memory the host can't reach", __func__);
    }
    if (one->mapped) {
        fault("%s maps memory a second time", __func__);
    }
    one->mapped = true;
    *mapped = one->bytes;
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkUnmapMemory(VkDevice device, VkDeviceMemory memory)
{
    Object *one = use(memory, MEMORY, __func__);

    if (use(device, DEVICE, __func__) != nullptr && one != nullptr) {
        one->mapped = false;
    }
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *,
                   const VkAllocationCallbacks *, VkRenderPass *pass)
{
    return create(device, __func__, PASS, pass);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyRenderPass(VkDevice device,
                                               VkRenderPass pass,
                                               const VkAllocationCallbacks *)
{
    destroy(device, pass, PASS, __func__);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo *info,
                    const VkAllocationCallbacks *, VkFramebuffer *framebuffer)
{
    Object *one = nullptr;
    Object *views[2] = {};

    use(info->renderPass, PASS, __func__);
    for (uint32_t i = 0; i < info->attachmentCount && i < 2; i++) {
        views[i] = use(info->pAttachments[i], VIEW, __func__);
    }
    VkResult result = create(device, __func__, FRAMEBUFFER, framebuffer, &one);

    if (result == VK_SUCCESS) {
        one->views[0] = views[0];
        one->views[1] = views[1];
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyFramebuffer(VkDevice device,
                                                VkFramebuffer framebuffer,
                                                const VkAllocationCallbacks *)
{
    destroy(device, framebuffer, FRAMEBUFFER, __func__);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *,
                     const VkAllocationCallbacks *, VkShaderModule *module)
{
    return create(device, __func__, MODULE, module);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyShaderModule(VkDevice device,
                                                 VkShaderModule module,
                                                 const VkAllocationCallbacks *)
{
    destroy(device, module, MODULE, __func__);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorSetLayout(
    VkDevice device, const VkDescriptorSetLayoutCreateInfo *,
    const VkAllocationCallbacks *, VkDescriptorSetLayout *layout)
{
    return create(device, __func__, SET_LAYOUT, layout);
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout layout,
                             const VkAllocationCallbacks *)
{
    destroy(device, layout, SET_LAYOUT, __func__);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *info,
                       const VkAllocationCallbacks *, VkPipelineLayout *layout)
{
    for (uint32_t i = 0; i < info->setLayoutCount; i++) {
        use(info->pSetLayouts[i], SET_LAYOUT, __func__);
    }
    return create(device, __func__, LAYOUT, layout);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineLayout(
    VkDevice device, VkPipelineLayout layout, const VkAllocationCallbacks *)
{
    destroy(device, layout, LAYOUT, __func__);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache, uint32_t,
                          const VkGraphicsPipelineCreateInfo *info,
                          const VkAllocationCallbacks *, VkPipeline *pipeline)
{
    use(info->layout, LAYOUT, __func__);
    use(info->renderPass, PASS, __func__);
    for (uint32_t i = 0; i < info->stageCount; i++) {
        use(info->pStages[i].module, MODULE, __func__);
    }
    VkResult result = create(device, __func__, PIPELINE, pipeline);

    /* a pipeline that isn't made is null */
    if (result != VK_SUCCESS) {
        *pipeline = VK_NULL_HANDLE;
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateComputePipelines(VkDevice device, VkPipelineCache, uint32_t,
                         const VkComputePipelineCreateInfo *info,
                         const VkAllocationCallbacks *, VkPipeline *pipeline)
{
    Object *one = nullptr;

    use(info->layout, LAYOUT, __func__);
    use(info->stage.module, MODULE, __func__);
    VkResult result = create(device, __func__, PIPELINE, pipeline, &one);

    if (result != VK_SUCCESS) {
        *pipeline = VK_NULL_HANDLE;
    } else {
        one->compute = true;
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyPipeline(VkDevice device,
                                             VkPipeline pipeline,
                                             const VkAllocationCallbacks *)
{
    destroy(device, pipeline, PIPELINE, __func__);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *,
                       const VkAllocationCallbacks *, VkDescriptorPool *pool)
{
    return create(device, __func__, DESCRIPTORS, pool);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorPool(
    VkDevice device, VkDescriptorPool pool, const VkAllocationCallbacks *)
{
    if (pool != VK_NULL_HANDLE && use(pool, DESCRIPTORS, __func__) != nullptr) {
        end_owned(reinterpret_cast<Object *>(pool), __func__);
    }
    destroy(device, pool, DESCRIPTORS, __func__);
}

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateDescriptorSets(
    VkDevice device, const VkDescriptorSetAllocateInfo *info,
    VkDescriptorSet *sets)
{
    Object *pool = use(info->descriptorPool, DESCRIPTORS, __func__);
    VkResult result;

    for (uint32_t i = 0; i < info->descriptorSetCount; i++) {
        use(info->pSetLayouts[i], SET_LAYOUT, __func__);
    }
    if (use(device, DEVICE, __func__) == nullptr || pool == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (fails(__func__, result)) {
        for (uint32_t i = 0; i < info->descriptorSetCount; i++) {
            sets[i] = VK_NULL_HANDLE;
        }
        return result;
    }
    for (uint32_t i = 0; i < info->descriptorSetCount; i++) {
        sets[i] = reinterpret_cast<VkDescriptorSet>(make(SET, pool));
    }
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkFreeDescriptorSets(VkDevice device,
                                                    VkDescriptorPool pool,
                                                    uint32_t count,
                                                    const VkDescriptorSet *sets)
{
    use(device, DEVICE, __func__);
    use(pool, DESCRIPTORS, __func__);
    for (uint32_t i = 0; i < count; i++) {
        Object *set =
            sets[i] != VK_NULL_HANDLE ? use(sets[i], SET, __func__) : nullptr;

        if (set != nullptr) {
            unread(set, __func__);
            end(set);
        }
    }
    return VK_SUCCESS;
}

/* a write past its binding's one descriptor runs on into the next */
VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSets(
    VkDevice device, uint32_t count, const VkWriteDescriptorSet *writes,
    uint32_t, const VkCopyDescriptorSet *)
{
    use(device, DEVICE, __func__);
    for (uint32_t i = 0; i < count; i++) {
        const VkWriteDescriptorSet &write = writes[i];
        Object *set = use(write.dstSet, SET, __func__);

        for (uint32_t k = 0; set != nullptr && k < write.descriptorCount; k++) {
            uint32_t binding = write.dstBinding + k;
            bool buffer =
                write.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                write.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

            if (binding >= 4) {
                fault("%s writes a binding the layouts lack", __func__);
                continue;
            }
            set->bound[binding] =
                buffer ? use(write.pBufferInfo[k].buffer, BUFFER, __func__)
                       : use(write.pImageInfo[k].imageView, VIEW, __func__);
            if (buffer && set->bound[binding] != nullptr &&
                set->bound[binding]->memory == nullptr) {
                fault("%s writes a buffer with no memory", __func__);
            }
            if (write.descriptorType ==
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                set->sampler =
                    use(write.pImageInfo[k].sampler, SAMPLER, __func__);
            }
        }
    }
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSampler(VkDevice device,
                                               const VkSamplerCreateInfo *info,
                                               const VkAllocationCallbacks *,
                                               VkSampler *sampler)
{
    Object *one = nullptr;
    VkResult result = create(device, __func__, SAMPLER, sampler, &one);

    if (result == VK_SUCCESS) {
        if (fabsf(info->mipLodBias) > control.max_bias) {
            fault("%s takes a bias past the device's most", __func__);
        }
        one->sampler_at = static_cast<uint32_t>(control.samplers.size());
        control.samplers.push_back(*info);
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroySampler(VkDevice device, VkSampler sampler,
                                            const VkAllocationCallbacks *)
{
    destroy(device, sampler, SAMPLER, __func__);
}

VKAPI_ATTR void VKAPI_CALL
vkCmdBeginRenderPass(VkCommandBuffer commands,
                     const VkRenderPassBeginInfo *info, VkSubpassContents)
{
    Object *c = recording(commands, 0, __func__);

    if (c == nullptr) {
        return;
    }
    Object *framebuffer = name(c, info->framebuffer, FRAMEBUFFER, __func__);

    name(c, info->renderPass, PASS, __func__);
    for (int i = 0; framebuffer != nullptr && i < 2; i++) {
        if (framebuffer->views[i] == nullptr || !framebuffer->views[i]->live) {
            fault("%s takes a framebuffer whose view is destroyed", __func__);
        } else {
            c->names.push_back(framebuffer->views[i]);
        }
    }
    c->in_pass = true;
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderPass(VkCommandBuffer commands)
{
    Object *c = recording(commands, 1, __func__);

    if (c != nullptr) {
        c->in_pass = false;
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindPipeline(VkCommandBuffer commands,
                                             VkPipelineBindPoint point,
                                             VkPipeline pipeline)
{
    Object *c = recording(commands, -1, __func__);
    Object *one =
        c != nullptr ? name(c, pipeline, PIPELINE, __func__) : nullptr;
    bool compute = point == VK_PIPELINE_BIND_POINT_COMPUTE;

    if (one != nullptr && one->compute != compute) {
        fault("%s binds a pipeline at the other bind point", __func__);
    }
    if (c != nullptr) {
        c->pipeline[compute] = one;
    }
}

VKAPI_ATTR void VKAPI_CALL
vkCmdBindDescriptorSets(VkCommandBuffer commands, VkPipelineBindPoint point,
                        VkPipelineLayout layout, uint32_t first, uint32_t count,
                        const VkDescriptorSet *sets, uint32_t, const uint32_t *)
{
    Object *c = recording(commands, -1, __func__);

    if (c == nullptr) {
        return;
    }
    name(c, layout, LAYOUT, __func__);
    for (uint32_t i = 0; i < count && first + i < 2; i++) {
        c->sets[point == VK_PIPELINE_BIND_POINT_COMPUTE][first + i] =
            name(c, sets[i], SET, __func__);
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers(VkCommandBuffer commands,
                                                  uint32_t, uint32_t,
                                                  const VkBuffer *buffers,
                                                  const VkDeviceSize *)
{
    Object *c = recording(commands, -1, __func__);

    if (c != nullptr) {
        c->vertices = backed(c, buffers[0], BUFFER, __func__);
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdPushConstants(VkCommandBuffer commands,
                                              VkPipelineLayout layout,
                                              VkShaderStageFlags, uint32_t,
                                              uint32_t size, const void *words)
{
    Object *c = recording(commands, -1, __func__);

    if (c != nullptr) {
        name(c, layout, LAYOUT, __func__);
        memcpy(c->push, words, size < sizeof c->push ? size : sizeof c->push);
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdDraw(VkCommandBuffer commands, uint32_t count,
                                     uint32_t, uint32_t first, uint32_t)
{
    Object *c = recording(commands, 1, __func__);

    if (c == nullptr) {
        return;
    }
    if (c->pipeline[0] == nullptr || !c->pipeline[0]->live) {
        fault("%s runs with no pipeline bound", __func__);
        return;
    }
    if (c->vertices == nullptr || !c->vertices->live) {
        fault("%s runs with no vertex buffer bound", __func__);
        return;
    }
    if (!bound(c, 0, 2, __func__)) {
        return;
    }
    const Object *texture = c->sets[0][1];
    Op op;

    if (texture->bound[0] == nullptr || texture->sampler == nullptr) {
        fault("%s runs with a texture's set no write filled", __func__);
        return;
    }
    op.what = Op::DRAW;
    op.list = c->sets[0][0]->bound[2];
    if (op.list != nullptr) {
        c->wrote |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    op.draw.push[0] = c->push[0];
    op.draw.push[1] = c->push[1];
    op.draw.first = first;
    op.draw.count = count;
    op.draw.view = reinterpret_cast<VkImageView>(texture->bound[0]);
    op.draw.sampler = control.samplers[texture->sampler->sampler_at];
    c->ops.push_back(op);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetViewport(VkCommandBuffer commands, uint32_t,
                                            uint32_t, const VkViewport *)
{
    recording(commands, -1, __func__);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetScissor(VkCommandBuffer commands, uint32_t,
                                           uint32_t, const VkRect2D *)
{
    recording(commands, -1, __func__);
}

/* a barrier over all memory lets what follows read what its stages wrote */
VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier(
    VkCommandBuffer commands, VkPipelineStageFlags from, VkPipelineStageFlags,
    VkDependencyFlags, uint32_t all, const VkMemoryBarrier *, uint32_t,
    const VkBufferMemoryBarrier *, uint32_t count,
    const VkImageMemoryBarrier *images)
{
    Object *c = recording(commands, 0, __func__);

    for (uint32_t i = 0; c != nullptr && i < count; i++) {
        backed(c, images[i].image, IMAGE, __func__);
    }
    if (c != nullptr && all != 0) {
        c->wrote &= from & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT ? 0 : ~from;
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyBufferToImage(VkCommandBuffer commands,
                                                  VkBuffer from, VkImage to,
                                                  VkImageLayout, uint32_t,
                                                  const VkBufferImageCopy *)
{
    Object *c = recording(commands, 0, __func__);

    if (c != nullptr) {
        backed(c, from, BUFFER, __func__);
        backed(c, to, IMAGE, __func__);
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyImageToBuffer(VkCommandBuffer commands,
                                                  VkImage from, VkImageLayout,
                                                  VkBuffer to, uint32_t,
                                                  const VkBufferImageCopy *)
{
    Object *c = recording(commands, 0, __func__);

    if (c != nullptr) {
        backed(c, from, IMAGE, __func__);
        backed(c, to, BUFFER, __func__);
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdBlitImage(VkCommandBuffer commands,
                                          VkImage from, VkImageLayout,
                                          VkImage to, VkImageLayout, uint32_t,
                                          const VkImageBlit *, VkFilter)
{
    Object *c = recording(commands, 0, __func__);

    if (c != nullptr) {
        backed(c, from, IMAGE, __func__);
        backed(c, to, IMAGE, __func__);
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdClearColorImage(VkCommandBuffer commands,
                                                VkImage image, VkImageLayout,
                                                const VkClearColorValue *,
                                                uint32_t,
                                                const VkImageSubresourceRange *)
{
    Object *c = recording(commands, 0, __func__);

    if (c != nullptr) {
        backed(c, image, IMAGE, __func__);
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdDispatch(VkCommandBuffer commands, uint32_t,
                                         uint32_t, uint32_t)
{
    Object *c = recording(commands, 0, __func__);
    Op op;

    if (c == nullptr) {
        return;
    }
    if (c->pipeline[1] == nullptr || !c->pipeline[1]->live) {
        fault("%s runs with no pipeline bound", __func__);
        return;
    }
    if (!bound(c, 1, 1, __func__)) {
        return;
    }
    op.what = Op::DISPATCH;
    op.list = c->sets[1][0]->bound[2];
    if (op.list != nullptr) {
        c->wrote |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    c->ops.push_back(op);
}

VKAPI_ATTR void VKAPI_CALL vkCmdUpdateBuffer(VkCommandBuffer commands,
                                             VkBuffer to, VkDeviceSize at,
                                             VkDeviceSize size,
                                             const void *bytes)
{
    Object *c = recording(commands, 0, __func__);
    Op op;

    if (c == nullptr || (op.to = backed(c, to, BUFFER, __func__)) == nullptr) {
        return;
    }
    if (at + size > op.to->size) {
        fault("%s writes past its buffer's end", __func__);
        return;
    }
    op.what = Op::UPDATE;
    op.to_at = at;
    op.size = size;
    op.bytes.assign(static_cast<const uint8_t *>(bytes),
                    static_cast<const uint8_t *>(bytes) + size);
    c->ops.push_back(op);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyBuffer(VkCommandBuffer commands,
                                           VkBuffer from, VkBuffer to, uint32_t,
                                           const VkBufferCopy *regions)
{
    Object *c = recording(commands, 0, __func__);
    Op op;

    if (c == nullptr ||
        (op.from = backed(c, from, BUFFER, __func__)) == nullptr ||
        (op.to = backed(c, to, BUFFER, __func__)) == nullptr) {
        return;
    }
    if (regions[0].srcOffset + regions[0].size > op.from->size ||
        regions[0].dstOffset + regions[0].size > op.to->size) {
        fault("%s copies past a buffer's end", __func__);
        return;
    }
    for (int point = 0; point < 2; point++) {
        const Object *set = c->sets[point][0];

        if (c->wrote != 0 && set != nullptr && set->bound[2] == op.from) {
            fault("%s copies the blend list with no barrier after a shader "
                  "that wrote it",
                  __func__);
        }
    }
    op.what = Op::COPY;
    op.from_at = regions[0].srcOffset;
    op.to_at = regions[0].dstOffset;
    op.size = regions[0].size;
    c->ops.push_back(op);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateFence(VkDevice device,
                                             const VkFenceCreateInfo *info,
                                             const VkAllocationCallbacks *,
                                             VkFence *fence)
{
    Object *one = nullptr;
    VkResult result = create(device, __func__, FENCE, fence, &one);

    if (result == VK_SUCCESS) {
        one->signaled = (info->flags & VK_FENCE_CREATE_SIGNALED_BIT) != 0;
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyFence(VkDevice device, VkFence fence,
                                          const VkAllocationCallbacks *)
{
    destroy(device, fence, FENCE, __func__);
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetFences(VkDevice device, uint32_t count,
                                             const VkFence *fences)
{
    VkResult result;

    if (use(device, DEVICE, __func__) == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (fails(__func__, result)) {
        return result;
    }
    for (uint32_t i = 0; i < count; i++) {
        Object *fence = use(fences[i], FENCE, __func__);

        if (fence != nullptr && running(fence)) {
            fault("%s takes a fence the device has yet to signal", __func__);
        }
        if (fence != nullptr) {
            fence->signaled = false;
        }
    }
    return VK_SUCCESS;
}

/* a submission is done as it is made, so a fence not signaled never is */
VKAPI_ATTR VkResult VKAPI_CALL vkWaitForFences(VkDevice device, uint32_t count,
                                               const VkFence *fences, VkBool32,
                                               uint64_t)
{
    VkResult result;

    if (use(device, DEVICE, __func__) == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (fails(__func__, result)) {
        return result;
    }
    for (uint32_t i = 0; i < count; i++) {
        const Object *fence = use(fences[i], FENCE, __func__);

        if (fence != nullptr && !fence->signaled) {
            fault("%s waits on a fence nothing will signal", __func__);
        }
        if (fence != nullptr) {
            finished(fence);
        }
    }
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceStatus(VkDevice device, VkFence fence)
{
    const Object *one = use(fence, FENCE, __func__);
    VkResult result;

    if (use(device, DEVICE, __func__) == nullptr || one == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (fails(__func__, result)) {
        return result;
    }
    if (one->signaled) {
        finished(one);
    }
    return one->signaled ? VK_SUCCESS : VK_NOT_READY;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSemaphore(VkDevice device,
                                                 const VkSemaphoreCreateInfo *,
                                                 const VkAllocationCallbacks *,
                                                 VkSemaphore *semaphore)
{
    return create(device, __func__, SEMAPHORE, semaphore);
}

VKAPI_ATTR void VKAPI_CALL vkDestroySemaphore(VkDevice device,
                                              VkSemaphore semaphore,
                                              const VkAllocationCallbacks *)
{
    destroy(device, semaphore, SEMAPHORE, __func__);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateQueryPool(VkDevice device,
                                                 const VkQueryPoolCreateInfo *,
                                                 const VkAllocationCallbacks *,
                                                 VkQueryPool *pool)
{
    return create(device, __func__, QUERIES, pool);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyQueryPool(VkDevice device, VkQueryPool pool,
                                              const VkAllocationCallbacks *)
{
    destroy(device, pool, QUERIES, __func__);
}

VKAPI_ATTR void VKAPI_CALL vkCmdResetQueryPool(VkCommandBuffer commands,
                                               VkQueryPool pool, uint32_t,
                                               uint32_t)
{
    Object *c = recording(commands, 0, __func__);

    if (c != nullptr) {
        name(c, pool, QUERIES, __func__);
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp(VkCommandBuffer commands,
                                               VkPipelineStageFlagBits,
                                               VkQueryPool pool, uint32_t)
{
    Object *c = recording(commands, -1, __func__);

    if (c != nullptr) {
        name(c, pool, QUERIES, __func__);
    }
}

/* a picture takes 1,000 ticks, of a nanosecond each */
VKAPI_ATTR VkResult VKAPI_CALL vkGetQueryPoolResults(VkDevice device,
                                                     VkQueryPool pool, uint32_t,
                                                     uint32_t, size_t,
                                                     void *ticks, VkDeviceSize,
                                                     VkQueryResultFlags)
{
    const uint64_t took[2] = {0, 1000};
    VkResult result;

    if (use(device, DEVICE, __func__) == nullptr ||
        use(pool, QUERIES, __func__) == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (fails(__func__, result)) {
        return result;
    }
    memcpy(ticks, took, sizeof took);
    return VK_SUCCESS;
}

/* a swapchain made in an older one's place retires it, made or not */
VKAPI_ATTR VkResult VKAPI_CALL
vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *info,
                     const VkAllocationCallbacks *, VkSwapchainKHR *swapchain)
{
    Object *one = nullptr;
    Object *old = info->oldSwapchain != VK_NULL_HANDLE
                      ? use(info->oldSwapchain, SWAPCHAIN, __func__)
                      : nullptr;

    if (old != nullptr) {
        old->retired = true;
    }
    if (info->imageExtent.width == 0 || info->imageExtent.height == 0) {
        fault("%s takes an extent of no size", __func__);
    }
    VkResult result = create(device, __func__, SWAPCHAIN, swapchain, &one);

    for (uint32_t i = 0; result == VK_SUCCESS && i < info->minImageCount; i++) {
        one->images.push_back(make(IMAGE, one));
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroySwapchainKHR(VkDevice device,
                                                 VkSwapchainKHR swapchain,
                                                 const VkAllocationCallbacks *)
{
    if (swapchain != VK_NULL_HANDLE &&
        use(swapchain, SWAPCHAIN, __func__) != nullptr) {
        end_owned(reinterpret_cast<Object *>(swapchain), __func__);
    }
    destroy(device, swapchain, SWAPCHAIN, __func__);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainImagesKHR(VkDevice device,
                                                       VkSwapchainKHR swapchain,
                                                       uint32_t *count,
                                                       VkImage *images)
{
    const Object *one = use(swapchain, SWAPCHAIN, __func__);
    VkResult result;

    if (use(device, DEVICE, __func__) == nullptr || one == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (fails(__func__, result)) {
        return result;
    }
    for (uint32_t i = 0;
         images != nullptr && i < *count && i < one->images.size(); i++) {
        images[i] = reinterpret_cast<VkImage>(one->images[i]);
    }
    *count = static_cast<uint32_t>(one->images.size());
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImageKHR(VkDevice device,
                                                     VkSwapchainKHR swapchain,
                                                     uint64_t,
                                                     VkSemaphore semaphore,
                                                     VkFence, uint32_t *image)
{
    Object *one = use(swapchain, SWAPCHAIN, __func__);
    VkResult result = VK_SUCCESS;

    if (use(device, DEVICE, __func__) == nullptr || one == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (fails(__func__, result) && result < 0) {
        return result;
    }
    if (one->retired) {
        fault("%s takes a swapchain a newer one retired", __func__);
    }
    if (one->out >= 0) {
        fault("%s acquires an image while one it never showed is out",
              __func__);
    }
    turn(&semaphore, 1, true, __func__);
    one->out = static_cast<int>(one->next++ % one->images.size());
    *image = static_cast<uint32_t>(one->out);
    return result;
}

/* a present takes its image back whatever it returns */
VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(VkQueue,
                                                 const VkPresentInfoKHR *info)
{
    Object *one = use(info->pSwapchains[0], SWAPCHAIN, __func__);
    VkResult result = VK_SUCCESS;

    fails(__func__, result);
    turn(info->pWaitSemaphores, info->waitSemaphoreCount, false, __func__);
    if (one != nullptr) {
        if (one->out != static_cast<int>(info->pImageIndices[0])) {
            fault("%s shows an image it didn't acquire", __func__);
        }
        one->out = -1;
    }
    return result;
}

namespace {

#define ENTRY(name) {#name, reinterpret_cast<PFN_vkVoidFunction>(name)}
const struct {
    const char *name;
    PFN_vkVoidFunction call;
} ENTRIES[] = {
    ENTRY(vkCreateInstance),
    ENTRY(vkDestroyInstance),
    ENTRY(vkEnumeratePhysicalDevices),
    ENTRY(vkGetPhysicalDeviceProperties),
    ENTRY(vkGetPhysicalDeviceQueueFamilyProperties),
    ENTRY(vkGetPhysicalDeviceMemoryProperties),
    ENTRY(vkGetPhysicalDeviceFeatures),
    ENTRY(vkCreateDevice),
    ENTRY(vkGetDeviceProcAddr),
    ENTRY(vkEnumerateDeviceExtensionProperties),
    ENTRY(vkDestroySurfaceKHR),
    ENTRY(vkGetPhysicalDeviceSurfaceSupportKHR),
    ENTRY(vkGetPhysicalDeviceSurfaceCapabilitiesKHR),
    ENTRY(vkGetPhysicalDeviceSurfaceFormatsKHR),
    ENTRY(vkGetPhysicalDeviceSurfacePresentModesKHR),
    ENTRY(vkDestroyDevice),
    ENTRY(vkGetDeviceQueue),
    ENTRY(vkCreateCommandPool),
    ENTRY(vkDestroyCommandPool),
    ENTRY(vkAllocateCommandBuffers),
    ENTRY(vkFreeCommandBuffers),
    ENTRY(vkBeginCommandBuffer),
    ENTRY(vkEndCommandBuffer),
    ENTRY(vkQueueSubmit),
    ENTRY(vkDeviceWaitIdle),
    ENTRY(vkCreateImage),
    ENTRY(vkDestroyImage),
    ENTRY(vkCreateImageView),
    ENTRY(vkDestroyImageView),
    ENTRY(vkGetImageMemoryRequirements),
    ENTRY(vkBindImageMemory),
    ENTRY(vkCreateBuffer),
    ENTRY(vkDestroyBuffer),
    ENTRY(vkGetBufferMemoryRequirements),
    ENTRY(vkBindBufferMemory),
    ENTRY(vkAllocateMemory),
    ENTRY(vkFreeMemory),
    ENTRY(vkMapMemory),
    ENTRY(vkUnmapMemory),
    ENTRY(vkCreateRenderPass),
    ENTRY(vkDestroyRenderPass),
    ENTRY(vkCreateFramebuffer),
    ENTRY(vkDestroyFramebuffer),
    ENTRY(vkCreateShaderModule),
    ENTRY(vkDestroyShaderModule),
    ENTRY(vkCreateDescriptorSetLayout),
    ENTRY(vkDestroyDescriptorSetLayout),
    ENTRY(vkCreatePipelineLayout),
    ENTRY(vkDestroyPipelineLayout),
    ENTRY(vkCreateGraphicsPipelines),
    ENTRY(vkDestroyPipeline),
    ENTRY(vkCreateDescriptorPool),
    ENTRY(vkDestroyDescriptorPool),
    ENTRY(vkAllocateDescriptorSets),
    ENTRY(vkUpdateDescriptorSets),
    ENTRY(vkCreateSampler),
    ENTRY(vkDestroySampler),
    ENTRY(vkCmdBeginRenderPass),
    ENTRY(vkCmdEndRenderPass),
    ENTRY(vkCmdBindPipeline),
    ENTRY(vkCmdBindDescriptorSets),
    ENTRY(vkCmdBindVertexBuffers),
    ENTRY(vkCmdPushConstants),
    ENTRY(vkCmdDraw),
    ENTRY(vkCmdSetViewport),
    ENTRY(vkCmdSetScissor),
    ENTRY(vkCmdPipelineBarrier),
    ENTRY(vkCmdCopyBufferToImage),
    ENTRY(vkCmdCopyImageToBuffer),
    ENTRY(vkCmdBlitImage),
    ENTRY(vkCmdClearColorImage),
    ENTRY(vkCmdDispatch),
    ENTRY(vkCreateComputePipelines),
    ENTRY(vkFreeDescriptorSets),
    ENTRY(vkCmdUpdateBuffer),
    ENTRY(vkCmdCopyBuffer),
    ENTRY(vkCreateFence),
    ENTRY(vkDestroyFence),
    ENTRY(vkResetFences),
    ENTRY(vkWaitForFences),
    ENTRY(vkGetFenceStatus),
    ENTRY(vkCreateSemaphore),
    ENTRY(vkDestroySemaphore),
    ENTRY(vkCreateQueryPool),
    ENTRY(vkDestroyQueryPool),
    ENTRY(vkCmdResetQueryPool),
    ENTRY(vkCmdWriteTimestamp),
    ENTRY(vkGetQueryPoolResults),
    ENTRY(vkCreateSwapchainKHR),
    ENTRY(vkDestroySwapchainKHR),
    ENTRY(vkGetSwapchainImagesKHR),
    ENTRY(vkAcquireNextImageKHR),
    ENTRY(vkQueuePresentKHR),
};
#undef ENTRY

/* the call named name, or null. a window call needs an object made with
 * the window's extensions */
PFN_vkVoidFunction entry(const char *name, const Object *of)
{
    if ((window_call(name) && (of == nullptr || !of->shows)) ||
        (control.lacks != nullptr && strcmp(control.lacks, name) == 0)) {
        return nullptr;
    }
    for (const auto &each : ENTRIES) {
        if (strcmp(each.name, name) == 0) {
            return each.call;
        }
    }
    return nullptr;
}

} // namespace

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device,
                                                             const char *name)
{
    return entry(name, use(device, DEVICE, __func__));
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetInstanceProcAddr(VkInstance instance, const char *name)
{
    return entry(name, instance != VK_NULL_HANDLE
                           ? use(instance, INSTANCE, __func__)
                           : nullptr);
}
