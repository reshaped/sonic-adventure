#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdSndMemReadUint8(Sint32 address, Uint8 *value)
{
    Uint32 byte;
    union {
        Uint32 word;
        Uint8 bytes[4];
    } data;

    if (address < SD_SND_MEM_SIZE) {
        byte = address & 3;
        address &= ~3;
        sdSndMemReadUint32(address, &data.word);
        *value = data.bytes[byte];
    }
    return SDE_ERR_NOTHING;
}
