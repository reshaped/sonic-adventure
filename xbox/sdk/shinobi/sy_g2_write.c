/* G2 bus write: Shinobi's, on the port's bus (g2.h). the AICA's registers
 * write through the host, the rest as memory */
#include <machine.h>
#include <sg_syg2.h>
#include <sg_xpt.h>

#include "xbox/sdk/shinobi/g2.h"

SYE_G2_ERR syG2Write(void *buf_ptr, void *g2_adr, SYE_G2_DATA_WIDTH data_width, Sint32 data_num,
                     SYE_G2_ADR_MODE src_mode, SYE_G2_ADR_MODE dst_mode)
{
    int size = (int)data_width;
    Uint8 *src = buf_ptr;
    Uint8 *dst = g2_adr;
    Sint32 src_step = g2_step(src_mode) * size;
    Sint32 dst_step = g2_step(dst_mode) * size;
    Sint32 mask = 0;
    int i;

    if (sygG2WrkPtr->mask_interrupts == true) {
        mask = get_imask();
        set_imask(IMASK_ALL);
    }
    for (i = 0; i < data_num; i++) {
        Uint32 value = size == 1 ? *src : size == 2 ? *(Uint16 *)src : *(Uint32 *)src;

        g2_write(dst, value, size);
        src += src_step;
        dst += dst_step;
    }
    if (sygG2WrkPtr->mask_interrupts == true) {
        set_imask(mask);
    }
    return SYE_G2_ERR_NOTHING;
}
