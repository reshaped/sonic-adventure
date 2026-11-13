/* the runtime's core: an engine made in the client's block, and its events.
 * ENGINE.md#c-abi gives the rules it keeps. runtime.h declares what a
 * backend adds */
#include "runtime.h"

#include <new>

namespace {

constexpr uint32_t ALIGN = 32;

/* bytes rounded up to a multiple of ALIGN, in 64 bits so that lay_out's
 * sums never wrap */
uint64_t aligned(uint64_t bytes)
{
    return (bytes + ALIGN - 1) & ~static_cast<uint64_t>(ALIGN - 1);
}

/* where each part of an engine lies in its block, from its start */
struct Layout {
    uint64_t events;
    uint64_t pictures;
    uint64_t arenas;
    uint64_t slots;
    uint64_t free;
    uint64_t backend;
    uint64_t end;
};

Layout lay_out(const sa_engine_desc &desc)
{
    Layout at;

    at.events = aligned(sizeof(sa_engine));
    at.pictures = at.events + aligned(uint64_t{desc.events} * sizeof(sa_event));
    at.arenas =
        at.pictures + aligned(uint64_t{desc.pictures} * sizeof(sa::Arena));
    at.slots = at.arenas + (uint64_t{desc.pictures} * aligned(desc.arena));
    at.free = at.slots + aligned(uint64_t{desc.resources} * sizeof(uint16_t));
    at.backend = at.free + aligned(uint64_t{desc.resources} * sizeof(uint32_t));
    at.end = at.backend + aligned(sa::backend_memory(desc));
    return at;
}

/* test desc's bounds. call it before desc is used */
sa_result check(const sa_engine_desc *desc)
{
    if (desc == nullptr) {
        return SA_ERR_INVALID;
    }
    if (desc->abi != SA_ENGINE_ABI_VERSION ||
        desc->size != sizeof(sa_engine_desc)) {
        return SA_ERR_VERSION;
    }
    if (desc->game == 0 || desc->flags != 0 || desc->pictures == 0 ||
        desc->events == 0 || desc->resources > SA_HANDLE_SLOTS) {
        return SA_ERR_INVALID;
    }
    /* the pictures' arenas alone can come near 2^64, so they are bounded
     * before lay_out adds to them */
    if (uint64_t{desc->pictures} * aligned(desc->arena) > UINT32_MAX) {
        return SA_ERR_INVALID;
    }
    return lay_out(*desc).end > UINT32_MAX ? SA_ERR_INVALID : SA_OK;
}

} // namespace

namespace sa {

void *Arena::take(uint32_t bytes, uint32_t align)
{
    uintptr_t at = (reinterpret_cast<uintptr_t>(next_) + align - 1) &
                   ~static_cast<uintptr_t>(align - 1);

    if (at > reinterpret_cast<uintptr_t>(end_) ||
        bytes > reinterpret_cast<uintptr_t>(end_) - at) {
        return nullptr;
    }
    next_ = reinterpret_cast<uint8_t *>(at) + bytes;
    return reinterpret_cast<void *>(at);
}

Handles::Handles(uint16_t *slots, uint32_t *free, uint32_t count)
    : slots_(slots), free_(free), count_(count), free_count_(count)
{
    for (uint32_t i = 0; i < count; i++) {
        slots[i] = 0;
        free[i] = count - 1 - i;
    }
}

sa_handle Handles::take(uint32_t kind)
{
    if (free_count_ == 0 || kind == 0 || kind > SA_HANDLE_KINDS) {
        return SA_NULL;
    }
    uint32_t entry = free_[--free_count_];
    uint32_t slot = entry & (SA_HANDLE_SLOTS - 1);
    uint32_t generation = handle_generation(entry) % SA_HANDLE_GENERATIONS + 1;

    slots_[slot] = static_cast<uint16_t>((kind << 8) | generation);
    return (kind << SA_HANDLE_KIND_SHIFT) |
           (generation << SA_HANDLE_GENERATION_SHIFT) | slot;
}

int32_t Handles::find(sa_handle handle) const
{
    uint32_t slot = handle & (SA_HANDLE_SLOTS - 1);
    uint32_t entry = (SA_HANDLE_KIND(handle) << 8) | handle_generation(handle);

    if (handle == SA_NULL || slot >= count_ || slots_[slot] != entry) {
        return -1;
    }
    return static_cast<int32_t>(slot);
}

sa_result Handles::give(sa_handle handle)
{
    int32_t slot = find(handle);

    if (slot < 0) {
        return SA_ERR_HANDLE;
    }
    /* the slot's next handle carries on from this one's generation */
    free_[free_count_++] =
        (handle_generation(handle) << SA_HANDLE_GENERATION_SHIFT) |
        static_cast<uint32_t>(slot);
    slots_[slot] = 0;
    return SA_OK;
}

sa_result event_reserve(sa_engine &engine)
{
    if (engine.event_count + engine.event_reserved >= engine.desc.events) {
        return SA_ERR_FULL;
    }
    engine.event_reserved++;
    if (engine.event_count + engine.event_reserved > engine.event_most) {
        engine.event_most = engine.event_count + engine.event_reserved;
    }
    return SA_OK;
}

void event_post(sa_engine &engine, const sa_event &event)
{
    uint32_t at =
        (engine.event_first + engine.event_count) % engine.desc.events;

    engine.events[at] = event;
    engine.event_count++;
    engine.event_reserved--;
}

} // namespace sa

extern "C" {

uint32_t sa_engine_abi(void)
{
    return SA_ENGINE_ABI_VERSION;
}

sa_result sa_engine_memory(const sa_engine_desc *desc, uint32_t *bytes)
{
    sa_result result = check(desc);

    if (result != SA_OK) {
        return result;
    }
    if (bytes == nullptr) {
        return SA_ERR_INVALID;
    }
    *bytes = static_cast<uint32_t>(lay_out(*desc).end);
    return SA_OK;
}

sa_result sa_engine_create(const sa_engine_desc *desc, void *memory,
                           uint32_t bytes, sa_engine **engine)
{
    sa_result result = check(desc);

    if (result != SA_OK) {
        return result;
    }
    Layout at = lay_out(*desc);

    if (memory == nullptr || engine == nullptr ||
        reinterpret_cast<uintptr_t>(memory) % ALIGN != 0 || bytes < at.end) {
        return SA_ERR_INVALID;
    }
    uint8_t *block = static_cast<uint8_t *>(memory);
    sa_engine *made = new (block) sa_engine();

    made->desc = *desc;
    made->events = reinterpret_cast<sa_event *>(block + at.events);
    made->pictures = reinterpret_cast<sa::Arena *>(block + at.pictures);
    for (uint32_t i = 0; i < desc->pictures; i++) {
        new (&made->pictures[i]) sa::Arena(
            block + at.arenas + (i * aligned(desc->arena)), desc->arena);
    }
    made->resources = sa::Handles(
        reinterpret_cast<uint16_t *>(block + at.slots),
        reinterpret_cast<uint32_t *>(block + at.free), desc->resources);
    made->backend = block + at.backend;
    result = sa::backend_create(*made);
    if (result != SA_OK) {
        return result;
    }
    *engine = made;
    return SA_OK;
}

void sa_engine_destroy(sa_engine *engine)
{
    if (engine != nullptr) {
        sa::backend_destroy(*engine);
    }
}

sa_result sa_event_poll(sa_engine *engine, sa_event *event)
{
    if (engine == nullptr || event == nullptr) {
        return SA_ERR_INVALID;
    }
    if (engine->event_count == 0) {
        return SA_EMPTY;
    }
    *event = engine->events[engine->event_first];
    engine->event_first = (engine->event_first + 1) % engine->desc.events;
    engine->event_count--;
    return SA_OK;
}

sa_result sa_clock_tick(sa_engine *engine, uint32_t tick)
{
    if (engine == nullptr) {
        return SA_ERR_INVALID;
    }
    engine->tick = tick;
    return SA_OK;
}

} // extern "C"
