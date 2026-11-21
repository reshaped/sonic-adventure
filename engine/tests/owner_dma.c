/* the real Kamui queues and channel-2 code over mapped register pages.
 * complete the first texture transfer when recovery unmasks its interrupt */
#include <assert.h>
#include <stdint.h>
#include <sys/mman.h>

#define REGISTER(address) (*(volatile uint32_t *)(uintptr_t)(address))
#define TARGET REGISTER(0xa05f6800)
#define START REGISTER(0xa05f6808)
#define INTERRUPTS REGISTER(0xa05f6900)
#define ERRORS REGISTER(0xa05f6908)
#define POLYGONS 0x10000000U
#define FIRST_TEXTURE 0x11000100U
#define LAST_TEXTURE 0x11000200U
#define SECOND_QUEUE 8

void kmiInitializeDMA(void);
int kmiDMAtoTARequest(uint32_t target, const uint32_t *source, uint32_t length,
                      uint32_t flags, uint32_t **handle);
int kmiExecuteDMAChain(void);
int kmiQueryFinishDMA(uint32_t handle);
void owner_discard_lists(void);

static int mask, recovering, textures_finished;

int get_imask(void)
{
    return mask;
}

void set_imask(int level)
{
    mask = level;
    if (recovering && !mask && TARGET == FIRST_TEXTURE) {
        assert(START == 1);
        START = 0;
        textures_finished++;
        kmiExecuteDMAChain();
    }
}

static uint32_t queue(uint32_t target, uint32_t flags)
{
    uint32_t *handle;

    kmiDMAtoTARequest(target, (const uint32_t *)0x0c001000, 32, flags, &handle);
    return *handle;
}

static void register_page(uintptr_t address)
{
    void *mapped =
        mmap((void *)address, 4096, PROT_READ | PROT_WRITE,
             MAP_FIXED_NOREPLACE | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(mapped != MAP_FAILED);
}

static void interleaved(int texture_first)
{
    uint32_t texture, first, second, last;

    kmiInitializeDMA();
    if (texture_first) {
        texture = queue(FIRST_TEXTURE, 0);
        first = queue(POLYGONS, 0);
    } else {
        first = queue(POLYGONS, 0);
        texture = queue(FIRST_TEXTURE, 0);
    }
    second = queue(POLYGONS, SECOND_QUEUE);
    last = queue(LAST_TEXTURE, SECOND_QUEUE);
    textures_finished = 0;
    recovering = 1;
    owner_discard_lists();
    recovering = 0;

    assert(mask == 0 && textures_finished == 1);
    assert(kmiQueryFinishDMA(texture) && kmiQueryFinishDMA(first) &&
           kmiQueryFinishDMA(second));
    assert(!kmiQueryFinishDMA(last) && TARGET == LAST_TEXTURE && START == 1);
    /* the reset acknowledged all five old list ends and the old ISP error */
    assert(INTERRUPTS == 0x200780 && ERRORS == 4);
    START = 0;
    kmiExecuteDMAChain();
    assert(kmiQueryFinishDMA(last));
}

int main(void)
{
    uint32_t texture;

    register_page(0xffa00000);
    register_page(0xff000000);
    register_page(0xa05f6000);
    register_page(0xa05f8000);
    interleaved(0);
    interleaved(1);
    kmiInitializeDMA();
    texture = queue(LAST_TEXTURE, 0);
    owner_discard_lists();
    assert(!kmiQueryFinishDMA(texture) && TARGET == LAST_TEXTURE && START == 1);
    return 0;
}
