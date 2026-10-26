#include <sg_syg2.h>
#include <sg_xpt.h>

SYE_G2_ERR syG2AicaFifoWaitEmpty(void)
{
    Bool empty;

    do {
        syG2AicaFifoIsEmpty(&empty);
        if (empty == true) {
            sygG2WrkPtr->aica_fifo_waits++;
        }
    } while (empty != true);
    return SYE_G2_ERR_NOTHING;
}
