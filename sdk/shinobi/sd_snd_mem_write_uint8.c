#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

/* @hack type: the word as a union of its bytes, which SHC places at R15 + 8,
 * above byte and value (MOV R15,R13 and ADD #8,R13 at 0x8c676c76). as a
 * Uint32 written through a byte cursor, the frame's slots move, the function
 * runs 4 bytes short and 20 of 33 words differ */
union sound_word {
    Uint32 word;
    Uint8 bytes[4];
};

SDE_ERR sdSndMemWriteUint8(Sint32 address, Uint8 value)
{
    Sint32 word_address;
    Sint32 byte;
    union sound_word data;

    if (address < SD_SND_MEM_SIZE) {
        /* @hack spelling: the address copied to word_address, which SHC makes
         * after the bound test (MOV R4,R14 at 0x8c676c6a). masked in address
         * itself, SHC copies it to R14 at the entry and 14 of 33 words
         * differ */
        word_address = address;
        byte = word_address & 3;
        word_address &= ~3;
        sdSndMemReadUint32(word_address, &data.word);
        data.bytes[byte] = value;
        sdSndMemWriteUint32(word_address, data.word);
    }
    return SDE_ERR_NOTHING;
}
