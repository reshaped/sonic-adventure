/* G2 bus read: Shinobi's, on the port's bus (g2.h). the AICA's registers
 * read through the host, the rest as memory */
#include <machine.h>
#include <sg_syg2.h>
#include <sg_xpt.h>

#include "xbox/sdk/shinobi/g2.h"

SYE_G2_ERR syG2Read(void *g2_adr, void *buf_ptr, SYE_G2_DATA_WIDTH data_width, Sint32 data_num,
                    SYE_G2_ADR_MODE src_mode, SYE_G2_ADR_MODE dst_mode)
{
    int size = (int)data_width;
    Uint8 *src = g2_adr;
    Uint8 *dst = buf_ptr;
    Sint32 src_step = g2_step(src_mode) * size;
    Sint32 dst_step = g2_step(dst_mode) * size;
    Sint32 mask = 0;
    int i;

    if (sygG2WrkPtr->mask_interrupts == true) {
        mask = get_imask();
        set_imask(IMASK_ALL);
    }
    for (i = 0; i < data_num; i++) {
        Uint32 value = g2_read(src, size);

        if (size == 1) {
            *dst = (Uint8)value;
        } else if (size == 2) {
            *(Uint16 *)dst = (Uint16)value;
        } else {
            *(Uint32 *)dst = value;
        }
        src += src_step;
        dst += dst_step;
    }
    if (sygG2WrkPtr->mask_interrupts == true) {
        set_imask(mask);
    }
    return SYE_G2_ERR_NOTHING;
}
