#include <sg_syg2.h>
#include <sg_xpt.h>

SYE_G2_ERR syG2FifoWaitEmpty(void)
{
    Bool empty;

    do {
        syG2FifoIsEmpty(&empty);
        if (empty == true) {
            sygG2WrkPtr->fifo_waits++;
        }
    } while (empty != true);
    return SYE_G2_ERR_NOTHING;
}
