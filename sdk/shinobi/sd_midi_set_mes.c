#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdarg.h>

#define STATUS_KIND 0xf0
#define STATUS_CHANNEL 0x0f
#define STATUS_SYSTEM 0xf0

/* *midi_mes_ptr's m_Member holds, by index:
 *   0: the second data byte, or 0 for a message with one
 *   1: the first data byte
 *   2: the port's number in bits 4 to 7, and the channel in bits 0 to 3
 *   3: the status byte's bits 4 to 6 */
SDE_ERR sdMidiSetMes(SDMIDI handle, SDS_MIDI_MES *midi_mes_ptr,
                     const Uint8 midi_mes, ...)
{
    /* never used. removing it would move the bss after it by 20 bytes */
    static struct sd_host_command unused_command;
    SDE_ERR err = SDE_ERR_NOTHING;
    va_list args;
    Sint8 number;
    Sint8 data1;
    Sint8 data2;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
    } else if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
    } else {
        number = ((struct sd_port *)handle)->number;
        if ((midi_mes & STATUS_KIND) != STATUS_SYSTEM) {
            va_start(args, midi_mes);
            data1 = va_arg(args, int);
            switch (midi_mes & STATUS_KIND) {
            case SDD_MIDI_PRG_CHG:
            case SDD_MIDI_MONO_PRES:
                data2 = 0;
                break;
            default:
                data2 = va_arg(args, int);
            }
            va_end(args);
            sdDebugPrintf("  MIDI Mes :%02X %02X %02X\n", midi_mes, data1,
                          data2);
            midi_mes_ptr->m_Member[0] = data2;
            midi_mes_ptr->m_Member[1] = data1;
            midi_mes_ptr->m_Member[2] =
                (number << 4) | (midi_mes & STATUS_CHANNEL);
            midi_mes_ptr->m_Member[3] = (midi_mes >> 4) & 7;
        } else {
            midi_mes_ptr->m_Member[0] = 0;
            midi_mes_ptr->m_Member[1] = 0;
            midi_mes_ptr->m_Member[2] = 0;
            midi_mes_ptr->m_Member[3] = 0;
        }
        sdDebugPrintf("  Pack Data:(Ptr:%08XH) %02X %02X %02X %02X\n",
                      midi_mes_ptr, midi_mes_ptr->m_Member[0],
                      midi_mes_ptr->m_Member[1], midi_mes_ptr->m_Member[2],
                      midi_mes_ptr->m_Member[3]);
    }
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMidiSetMes, err);
    }
    return err;
}
