/* what the runtime's core shares with a backend: the engine's block, its
 * arenas, handles and events. nothing here crosses the C ABI. a program
 * links the core and one backend. the backend defines the backend_ hooks
 * below and the ABI's services: input, storage, sound, resources and
 * pictures. it defines the clock's calls too, except sa_clock_tick */
#ifndef ENGINE_RUNTIME_H
#define ENGINE_RUNTIME_H

#include "sa_engine.h"

namespace sa {

/* a bump arena over part of the engine's block. take gives null where the
 * arena has no room, and reset frees all it gave at once */
class Arena {
public:
    Arena() = default;
    Arena(uint8_t *begin, uint32_t bytes)
        : begin_(begin), next_(begin), end_(begin + bytes)
    {
    }
    /* align is a power of 2 */
    void *take(uint32_t bytes, uint32_t align);
    void reset() { next_ = begin_; }
    void *at(uint32_t offset) const { return begin_ + offset; }
    uint32_t used() const { return static_cast<uint32_t>(next_ - begin_); }

private:
    uint8_t *begin_ = nullptr;
    uint8_t *next_ = nullptr;
    uint8_t *end_ = nullptr;
};

/* the slots behind a table of handles, each holding its handle's kind and
 * generation. a slot's generation counts from 1 and wraps past 255 to 1,
 * so a handle is never SA_NULL */
class Handles {
public:
    Handles() = default;
    Handles(uint16_t *slots, uint32_t *free, uint32_t count);
    /* a new handle of kind, from 1 to SA_HANDLE_KINDS, or SA_NULL where every
     * slot is taken */
    sa_handle take(uint32_t kind);
    /* handle's slot, or -1 where it is null or gone. a caller that accepts
     * only one kind tests SA_HANDLE_KIND itself */
    int32_t find(sa_handle handle) const;
    /* end handle: SA_OK, or SA_ERR_HANDLE where it is null or gone */
    sa_result give(sa_handle handle);

private:
    uint16_t *slots_ = nullptr; /* kind << 8 | generation, 0 while free */
    /* a stack of the free slots. an entry is a slot, with its last
     * generation in a handle's generation bits */
    uint32_t *free_ = nullptr;
    uint32_t count_ = 0;
    uint32_t free_count_ = 0; /* the stack's depth */
};

} // namespace sa

/* the shifts of a handle's generation and kind, as sa_engine.h packs
 * them */
constexpr uint32_t SA_HANDLE_GENERATION_SHIFT = 20;
constexpr uint32_t SA_HANDLE_KIND_SHIFT = 28;
static_assert(SA_HANDLE_KIND(1U << SA_HANDLE_KIND_SHIFT) == 1,
              "SA_HANDLE_KIND reads another shift");
/* the slots a table may hold, one for each value of a handle's slot. the
 * last generation and the last kind set every bit of their fields */
constexpr uint32_t SA_HANDLE_SLOTS = 1U << SA_HANDLE_GENERATION_SHIFT;
constexpr uint32_t SA_HANDLE_GENERATIONS =
    (1U << (SA_HANDLE_KIND_SHIFT - SA_HANDLE_GENERATION_SHIFT)) - 1;
constexpr uint32_t SA_HANDLE_KINDS = (1U << (32 - SA_HANDLE_KIND_SHIFT)) - 1;

struct sa_engine {
    sa_engine_desc desc;
    uint32_t tick; /* the last frame sa_clock_tick gave */
    /* a ring of desc.events. event_reserved counts promised events, so a
     * post never finds the ring full. event_most is the most held and
     * promised at once */
    sa_event *events;
    uint32_t event_first;
    uint32_t event_count;
    uint32_t event_reserved;
    uint32_t event_most;
    /* desc.pictures arenas of desc.arena bytes, which the backend takes in
     * turn for a picture's copies */
    sa::Arena *pictures;
    /* the resources' handles, desc.resources of them, of every kind. a
     * backend keeps its own record of each by slot */
    sa::Handles resources;
    void *backend; /* the backend's bytes in the block */
};

namespace sa {

constexpr uint32_t handle_generation(uint32_t handle)
{
    return (handle >> SA_HANDLE_GENERATION_SHIFT) & SA_HANDLE_GENERATIONS;
}

/* backend_memory gives the bytes a backend needs for desc. a
 * backend_create that fails undoes what it made, since no backend_destroy
 * follows */
uint32_t backend_memory(const sa_engine_desc &desc);
sa_result backend_create(sa_engine &engine);
void backend_destroy(sa_engine &engine);

/* promise a later event_post a place in the ring. it returns SA_OK, or
 * SA_ERR_FULL where every place is held or promised */
sa_result event_reserve(sa_engine &engine);
/* post an event an event_reserve promised */
void event_post(sa_engine &engine, const sa_event &event);

} // namespace sa

#endif
