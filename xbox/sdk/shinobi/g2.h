/* the port's G2 bus, for sy_g2_read.c, sy_g2_write.c and
 * sy_g2_dma_trigger.c here, which make pc builds in place of Shinobi's: the
 * AICA's registers, the 128 KB from 0x00700000 with the RTC's, are the
 * host's, through host_api's aica calls. the rest of the bus the port
 * serves as memory: the AICA's sound RAM from 0x00800000, at its P2 address
 * 0xa0800000, which the host shares with its AICA */
#ifndef XBOX_SDK_SHINOBI_G2_H
#define XBOX_SDK_SHINOBI_G2_H
#include <sg_xpt.h>

#include "xbox/port/host.h"

#define G2_AICA_REGISTERS 0x00700000u
#define G2_AICA_REGISTERS_END 0x00720000u

/* the G2 address of a pointer the bus takes, by P2 */
static inline Uint32 g2_physical(const void *at)
{
    return (Uint32)at & 0x1fffffffu;
}

static inline int g2_is_aica_register(Uint32 physical)
{
    return physical >= G2_AICA_REGISTERS && physical < G2_AICA_REGISTERS_END;
}

/* a read or a write of size bytes on the bus at at */
static inline Uint32 g2_read(const void *at, int size)
{
    Uint32 physical = g2_physical(at);

    if (g2_is_aica_register(physical)) {
        return HOST->aica_read(physical - G2_AICA_REGISTERS, size);
    }
    at = (const void *)(physical | 0xa0000000u);
    return size == 1 ? *(volatile const Uint8 *)at : size == 2 ? *(volatile const Uint16 *)at
                                                               : *(volatile const Uint32 *)at;
}

static inline void g2_write(void *at, Uint32 value, int size)
{
    Uint32 physical = g2_physical(at);

    if (g2_is_aica_register(physical)) {
        HOST->aica_write(physical - G2_AICA_REGISTERS, value, size);
        return;
    }
    at = (void *)(physical | 0xa0000000u);
    if (size == 1) {
        *(volatile Uint8 *)at = (Uint8)value;
    } else if (size == 2) {
        *(volatile Uint16 *)at = (Uint16)value;
    } else {
        *(volatile Uint32 *)at = value;
    }
}

/* a mode's step in units */
static inline Sint32 g2_step(int mode)
{
    return mode == 1 ? 1 : mode == 2 ? -1 : 0;
}

/* the channels whose transfer has ended, a bit for each SYE_G2_DMA_TYPE:
 * xbox/port/system.c's blank runs each one's end chain, at level 4, as the
 * console's G2 DMA end interrupt does */
extern volatile Uint32 port_g2_dma_ended;

#endif
