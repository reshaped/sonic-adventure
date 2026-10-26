#include <sg_xpt.h>
#include <shinobi.h>

/* a sampling rate, and the driver's pitch word for it. the word holds a
 * signed octave in bits 11 to 14 and a fraction in bits 0 to 9. the rate is
 * 44.1 kHz times 2 to the octave times (1 + fraction / 1024) */
struct sd_freq {
    Sint32 hz;
    Uint16 pitch;
};

/* the rates sdPstmPlayMw takes, ending at a rate of -1 */
struct sd_freq sdgFreqTable[] = {
    {48000, 0x005a}, {44100, 0x0000}, {37800, 0x7adb}, {32000, 0x79cd},
    {24000, 0x785a}, {22050, 0x7800}, {18900, 0x72db}, {16000, 0x71cd},
    {12000, 0x705a}, {11025, 0x7000}, {8820, 0x6a65},  {8000, 0x69cd},
    {-1, 0x0000}};

SDE_ERR sdPstmPlayMw(SDPSTM handle, const SDE_PCM_TYPE pcm_type,
                     const Uint16 freq, const Sint8 priority)
{
    SDE_ERR err;
    struct sd_freq *entry;

    /* @hack spelling: err set after the search, which has SHC load the error
     * code only once the search fails (MOV.L at 0x8c67714e). set before it,
     * SHC loads it ahead of the loop and 17 of 34 words differ */
    for (entry = sdgFreqTable; entry->hz > 0; entry++) {
        if (freq == entry->hz) {
            err = sdPstmPlay(handle, pcm_type, entry->pitch, priority);
            goto done;
        }
    }
    err = SDE_ERR_PRM_OVER_RANGE;
done:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPstmPlayMw, err);
    }
    return err;
}
