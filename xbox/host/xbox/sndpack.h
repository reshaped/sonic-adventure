/* the Xbox's sound pack, sounds.pak: xboxsnd.c writes it, aica_apu.c reads
 * it. a header, an entry a bank file sorted by name, and at each entry's
 * offset its samples' records, then their data */
#ifndef XBOX_HOST_XBOX_SNDPACK_H
#define XBOX_HOST_XBOX_SNDPACK_H

#include <stdint.h>

#define SNDPACK_MAGIC 0x4b504e53u /* "SNPK" */

struct sndpack_header {
    uint32_t magic;
    uint32_t count;
};

struct sndpack_file {
    char name[24]; /* the bank file's, in capitals: E_0009.MLT */
    uint32_t offset; /* from the pack's start */
    uint32_t samples;
    uint32_t bytes; /* the records and the data */
};

struct sndpack_sample {
    uint32_t sa; /* the sample's address in sound RAM */
    uint32_t count; /* in samples: the loop's end and one */
    uint32_t pcms; /* 1 PCM8, 2 ADPCM: as the AICA has it */
    uint8_t head[8]; /* the AICA's sample's first bytes, to tell it by */
    uint32_t data; /* from the records' end */
    uint32_t bytes; /* converted: unsigned 8-bit, or 16-bit */
};

#endif
