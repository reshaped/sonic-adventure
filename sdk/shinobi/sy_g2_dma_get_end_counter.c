#include <sg_syg2.h>
#include <sg_xpt.h>

SYE_G2_ERR syG2DmaGetEndCounter(SYE_G2_DMA_TYPE type, Uint32 *counter)
{
    *counter = sygG2WrkPtr->end_counts[type];
    return SYE_G2_ERR_NOTHING;
}
