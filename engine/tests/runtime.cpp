/* engine-test runs these checks of the runtime's core under a backend that
 * keeps nothing. main returns 0, or prints the line of the first check that
 * fails and returns 1 */
#include <stdio.h>

#include "runtime.h"

namespace sa {

uint32_t backend_memory(const sa_engine_desc &)
{
    return 64;
}

sa_result backend_create(sa_engine &)
{
    return SA_OK;
}

void backend_destroy(sa_engine &)
{
}

} // namespace sa

#define CHECK(test)                                                            \
    do {                                                                       \
        if (!(test)) {                                                         \
            return __LINE__;                                                   \
        }                                                                      \
    } while (0)

namespace {

alignas(32) uint8_t block[1 << 16];

int check()
{
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = 1; /* the core takes any game but 0 */
    desc.arena = 1024;
    desc.pictures = 2;
    desc.resources = 2;
    desc.events = 2;
    CHECK(sa_engine_abi() == SA_ENGINE_ABI_VERSION);
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);

    sa_engine_desc other = desc;

    other.abi++;
    CHECK(sa_engine_create(&other, block, bytes, &engine) == SA_ERR_VERSION);
    other = desc;
    other.game = 0;
    CHECK(sa_engine_create(&other, block, bytes, &engine) == SA_ERR_INVALID);
    /* bounds whose sum, past 64 bits, would wrap to a small block. the
     * refused call takes an out of its own, so bytes stays desc's */
    uint32_t refused = 0;

    other = desc;
    other.arena = 0xffffffe0;
    other.pictures = 0xffffffff;
    other.events = 0x60000001;
    CHECK(sa_engine_memory(&other, &refused) == SA_ERR_INVALID);
    CHECK(sa_engine_create(&desc, block, bytes - 1, &engine) == SA_ERR_INVALID);
    CHECK(sa_engine_create(&desc, block + 4, bytes, &engine) == SA_ERR_INVALID);
    CHECK(engine == nullptr);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);

    CHECK(sa_clock_tick(engine, 7) == SA_OK && engine->tick == 7);
    CHECK(sa_clock_tick(nullptr, 8) == SA_ERR_INVALID);

    /* events come in order, and a reserve past the queue's room refuses */
    sa_event event{};

    CHECK(sa_event_poll(engine, &event) == SA_EMPTY);
    CHECK(sa::event_reserve(*engine) == SA_OK);
    CHECK(sa::event_reserve(*engine) == SA_OK);
    CHECK(sa::event_reserve(*engine) == SA_ERR_FULL);
    event.kind = SA_EVENT_RETIRED;
    sa::event_post(*engine, event);
    event.kind = SA_EVENT_PRESENTED;
    sa::event_post(*engine, event);
    CHECK(sa_event_poll(engine, &event) == SA_OK &&
          event.kind == SA_EVENT_RETIRED);
    CHECK(sa_event_poll(engine, &event) == SA_OK &&
          event.kind == SA_EVENT_PRESENTED);
    CHECK(sa_event_poll(engine, &event) == SA_EMPTY);

    /* a handle given back is gone, and its slot's next handle is another */
    sa_handle mesh = engine->resources.take(SA_KIND_MESH);
    sa_handle texture = engine->resources.take(SA_KIND_TEXTURE);

    CHECK(mesh != SA_NULL && SA_HANDLE_KIND(mesh) == SA_KIND_MESH);
    CHECK(SA_HANDLE_KIND(texture) == SA_KIND_TEXTURE);
    CHECK(engine->resources.take(SA_KIND_MESH) == SA_NULL);
    CHECK(engine->resources.find(mesh) >= 0);
    CHECK(engine->resources.give(mesh) == SA_OK);
    CHECK(engine->resources.find(mesh) == -1);
    CHECK(engine->resources.give(mesh) == SA_ERR_HANDLE);

    sa_handle again = engine->resources.take(SA_KIND_MESH);

    CHECK(again != SA_NULL && again != mesh);
    CHECK((again & (SA_HANDLE_SLOTS - 1)) == (mesh & (SA_HANDLE_SLOTS - 1)));
    CHECK(engine->resources.find(SA_NULL) == -1);

    /* a kind outside 1 to SA_HANDLE_KINDS takes no handle, with a slot free */
    CHECK(engine->resources.give(again) == SA_OK);
    CHECK(engine->resources.take(0) == SA_NULL);
    CHECK(engine->resources.take(SA_HANDLE_KINDS + 1) == SA_NULL);

    sa_handle last = engine->resources.take(SA_HANDLE_KINDS);

    CHECK(SA_HANDLE_KIND(last) == SA_HANDLE_KINDS);
    CHECK(engine->resources.find(last) >= 0);

    /* a slot's generation counts from 1 to SA_HANDLE_GENERATIONS, then wraps
     * to 1. so each take gives another handle until the
     * SA_HANDLE_GENERATIONS-th, which gives last again */
    sa_handle next = last;

    for (uint32_t i = 1; i <= SA_HANDLE_GENERATIONS; i++) {
        CHECK(engine->resources.give(next) == SA_OK);
        next = engine->resources.take(SA_HANDLE_KINDS);
        CHECK(next != SA_NULL);
        CHECK((next == last) == (i == SA_HANDLE_GENERATIONS));
    }

    /* a picture's arena gives what fits, and all again once reset */
    sa::Arena &arena = engine->pictures[1];

    CHECK(arena.take(1000, 4) != nullptr);
    CHECK(arena.take(100, 4) == nullptr);
    arena.reset();
    CHECK(arena.take(1024, 32) != nullptr && arena.used() == 1024);

    sa_engine_destroy(engine);
    return 0;
}

} // namespace

int main()
{
    int line = check();

    if (line != 0) {
        fprintf(stderr, "runtime: line %d\n", line);
    }
    return line != 0;
}
