#include <sg_syg2.h>
#include <sg_xpt.h>

SYE_G2_ERR syG2DmaGetTriggerCounter(SYE_G2_DMA_TYPE type, Uint32 *counter)
{
    /* @bug logic: the trigger counter reads end_counts[type], not
     * trigger_counts[type]. the pool word at 0x8c11c3f0 is 0xfc, end_counts'
     * offset, as is syG2DmaGetEndCounter's at 0x8c11c3d4 */
    *counter = sygG2WrkPtr->end_counts[type];
    return SYE_G2_ERR_NOTHING;
}
