/* the running title's play history: its play time by hour of the day, the
 * peripherals it has seen, and its saves, loads and network plays */
/* fh_status.h gives this object its own copy of the safe refresh status
 * strings, though no function here reads them */
#include "shinobi/fh_status.h" /* NOLINT(misc-include-cleaner) */
#include "shinobi/uf_work.h"
#include <sg_uf.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

/* each title's record is TITLE_PACKETS packets of the history partition,
 * the first title's from packet TITLE_FIRST_PACKET */
#define TITLE_FIRST_PACKET 24
#define TITLE_PACKETS 4
/* the console's lists of the controller IDs and the expansion device types
 * it has seen. each list takes two packets, from PORT_PACKET or
 * DEVICE_PACKET, and holds its IDs in the first LIST_ENTRIES of their longs.
 * an entry never written is an erased long, ERASED32 */
#define PORT_PACKET 20
#define DEVICE_PACKET 22
#define PACKET_LONGS (FMD_DATA_BYTES / 4)
#define LIST_ENTRIES 24
/* the bytes of each list's mask in PeriIdx */
#define MASK_BYTES 3
/* flash erases to all ones, so a field never written reads as these */
#define ERASED8 0xff
#define ERASED16 0xffff
#define ERASED32 0xffffffff
/* the largest value a count is raised to */
#define COUNT_MAX 0xfffeU

/* where packets 1 and 3 of the running title's record start in
 * ufgPlayHistBuffer, counting the record's packets from 0 */
#define PACKET1 (ufgPlayHistBuffer.ProdName2)
#define PACKET3 (&ufgPlayHistBuffer.SaveCount)

/* the four ports, and their two expansion sockets each */
#define PORTS 4
#define SOCKETS 8

static const Uint8 ports[PORTS] = {PDD_PORT_A0, PDD_PORT_B0, PDD_PORT_C0,
                                   PDD_PORT_D0};
static const Uint8 sockets[SOCKETS] = {PDD_PORT_A1, PDD_PORT_A2, PDD_PORT_B1,
                                       PDD_PORT_B2, PDD_PORT_C1, PDD_PORT_C2,
                                       PDD_PORT_D1, PDD_PORT_D2};

static Uint32 synch_rtc; /* the RTC count at the last synch */
static Sint32 synch_left; /* ufPlayTime calls left before the next synch */
static Sint32 synch_every; /* calls per synch, as ufAutoSynch set it */
static Sint32 dat_8c8c7ac8; /* zeroed by ufPlayInit and never read */
static SYS_RTC_DATE synch_date; /* the RTC date at the last synch */
static SYS_RTC_DATE now_date;
static Uint32 connect_time; /* the time passed to ufConnect */
static Uint32 connect_count; /* the RTC count when the network play started */
static Sint16 synch_num; /* synchs since ufPlayInit */
/* whether ufConnect has started a network play. ufDisConnect leaves it set */
static Uint16 connected;
static Uint32 action_count; /* the RTC count at the last save or load */

/* add n to a 16-bit count. a sum past 0xffff wraps */
static void add_count(Uint16 *count, Uint16 n)
{
    Uint16 cur;

    cur = *count;
    if (n > 0) {
        /* @bug logic: cur + n is an int, so for n above 0 the test
         * (CMP/GT R3,R2 at 0x8c65cb7a) always holds: a sum past 0xffff wraps
         * in the store at 0x8c65cb82, and the COUNT_MAX store at 0x8c65cb86
         * never runs */
        if (cur + n > cur) {
            *count = cur + n;
        } else {
            *count = COUNT_MAX;
        }
    }
}

/* when fewer than two packets are left to write, run a safe refresh and
 * open the history partition again. false, with UFD_ERR_MEMORY_FAIL, when
 * the refresh is due and ufgReflash3 or ufgReflash4 is NULL. true
 * otherwise, whatever the refresh returns */
static Bool make_room(void)
{
    Bool ready;

    ready = true;
    if (ufgRestPacket < 2) {
        if (ufgReflash3 != NULL && ufgReflash4 != NULL) {
            fhSafeRefresh(ufgWorkBuff, ufgReadBuff, ufgReflash3, ufgReflash4);
            ufSetBank(-3);
        } else {
            ufgErrno = UFD_ERR_MEMORY_FAIL;
            ready = false;
        }
    }
    return ready;
}

/* add the network play's length in seconds to NetPlayTotalTime, and save
 * packet 3 of the record. an erased total takes the length alone */
static Bool end_net_play(Sint32 packet)
{
    Uint32 now;
    Uint32 seconds;

    syRtcGetDate(&now_date);
    syRtcDateToCount(&now_date, &now);
    if (ufgPlayHistBuffer.NetPlayTotalTime == ERASED16) {
        ufgPlayHistBuffer.NetPlayTotalTime = now - connect_count;
    } else {
        seconds = now - connect_count;
        add_count(&ufgPlayHistBuffer.NetPlayTotalTime, seconds);
    }
    return ufSavePacket(packet + 3, PACKET3);
}

/* count the network play ufConnect starts in NetPlayCount, and save packet
 * 3 of the record. connect_time becomes PrevNetPlay, and FirstNetPlay too
 * while that is erased. connect_count takes the RTC count, for
 * end_net_play */
static Bool start_net_play(Sint32 packet)
{
    Uint16 *count;
    Uint16 cur;

    if (ufgPlayHistBuffer.FirstNetPlay == ERASED32) {
        ufgPlayHistBuffer.FirstNetPlay = connect_time;
    }
    ufgPlayHistBuffer.PrevNetPlay = connect_time;
    syRtcGetDate(&now_date);
    syRtcDateToCount(&now_date, &connect_count);
    if (ufgPlayHistBuffer.NetPlayCount == ERASED16) {
        ufgPlayHistBuffer.NetPlayCount = 1;
    } else {
        count = &ufgPlayHistBuffer.NetPlayCount;
        cur = *count;
        if (cur < COUNT_MAX) {
            (*count)++;
        }
    }
    return ufSavePacket(packet + 3, PACKET3);
}

/* fill the record's packets with ERASED8 bytes, as if erased. false, with
 * UFD_ERR_WRITE_FAIL, at the first packet that fails to save */
static Bool delete_title(Sint32 packet)
{
    Sint32 i;

    memset(ufgReadBuff, ERASED8, FMD_DATA_BYTES);
    for (i = 0; i < TITLE_PACKETS; i++) {
        if (ufSavePacket(packet++, ufgReadBuff) == false) {
            ufgErrno = UFD_ERR_WRITE_FAIL;
            return false;
        }
    }
    return true;
}

/* add the minutes since the last synch to PlayTime, hour by hour from the
 * last synch's. PlayTime holds the minutes played in each hour of the day.
 * SaveOccur takes synch_num, the synchs since ufPlayInit, this one included.
 * then save packets 2 and 3 of the record. true when both save, after
 * ufgRestPacket is set from ufGetFreeBlock */
static Bool synch_history(Sint32 packet)
{
    Uint32 last;
    Uint32 minutes;
    Sint32 hour;
    Sint32 room;
    UFS_AllPlayHistory *history;

    last = synch_rtc;
    synch_left = synch_every;
    synch_num++;
    syRtcGetDate(&now_date);
    syRtcDateToCount(&now_date, &synch_rtc);
    minutes = (synch_rtc - last) / 60;
    hour = synch_date.hour;
    history = &ufgPlayHistBuffer;
    room = 60 - synch_date.minute;
    if ((Uint32)room < minutes) {
        add_count(&history->PlayTime[hour], room);
        minutes = minutes - room;
        hour++;
        if (hour > 23) {
            hour = 0;
        }
        /* @hack redundant: a store of synch_num > 23 to synch_num that never
         * runs, which adds uses of synch_num and 23 and keeps the argument in
         * R8 and the pointer loaded at 0x8c65cd88 in R10 (0x8c65cd7e,
         * 0x8c65cd88); without it, they take R10 and R9 and 20 of 128 words
         * differ */
        if (0) {
            synch_num = synch_num > 23;
        }
        while (minutes > 60) {
            add_count(&history->PlayTime[hour], 60);
            hour++;
            minutes -= 60;
            if (hour > 23) {
                hour = 0;
            }
        }
    }
    add_count(&history->PlayTime[hour], minutes);
    packet += 2;
    history->SaveOccur = synch_num;
    memcpy(&synch_date, &now_date, sizeof(SYS_RTC_DATE));
    /* NOLINTNEXTLINE(bugprone-inc-dec-in-conditions) */
    if (ufSavePacket(packet++, &history->PrevStartTime) == true &&
        ufSavePacket(packet, &history->SaveCount) == true) {
        ufgRestPacket = ufGetFreeBlock();
        return true;
    }
    return false;
}

Bool ufSaveAction(Bool (*action)(Sint32 packet), Sint32 need)
{
    Sint32 packet;

    packet = ufFindTitleIdx(ufgProdNum);
    if (packet >= 0) {
        if (ufgRestPacket < need && make_room() == false) {
            return false;
        }
        packet = (packet * TITLE_PACKETS) + TITLE_FIRST_PACKET;
        ufLoadPacket(packet, &ufgPlayHistBuffer);
        if (ufgUserFlag.data != 0 && ufgUserFlag.auto_save != 0) {
            if (ufgPlayHistBuffer.AutoSave != 0) {
                return (*action)(packet);
            } else {
                ufgErrno = UFD_ERR_PROTECT_FLAG;
            }
        } else {
            ufgErrno = UFD_ERR_PROTECT_COMMON_FLAG;
        }
    } else {
        ufgErrno = UFD_ERR_NOT_FOUND;
    }
    return false;
}

/* the big-endian value of the size bytes at data */
static Uint32 load_network_endian(const Uint8 *data, Sint32 size)
{
    Uint32 value;

    value = 0;
    if (size > 0) {
        do {
            value <<= 8;
            value |= *data++;
        } while (--size != 0);
    }
    return value;
}

void SaveNetworkEndiane(Uint32 value, Sint32 size, Uint8 *data)
{
    if (size > 0) {
        data += size - 1;
        do {
            *data = (Uint8)value;
            --data;
            value >>= 8;
        } while (--size != 0);
    }
}

Bool ufPlayInit(void)
{
    Sint32 port_ids[2 * PACKET_LONGS];
    Sint32 device_ids[2 * PACKET_LONGS];
    Sint32 port_now[PORTS];
    Sint32 device_now[SOCKETS];
    Sint32 packet;
    Uint32 seen_ports;
    Uint32 seen_devices;
    Bool changed;
    Sint32 i;
    Sint32 j;
    Sint32 k;
    Sint32 port_bits;
    Sint32 device_bits;
    const PDS_PERIPHERAL *per;
    const PDS_PERIPHERALINFO *info;
    Sint32 *ids;
    Bool ready;

    syRtcGetDate(&synch_date);
    syRtcDateToCount(&synch_date, &synch_rtc);
    connected = false;
    if (ufReadAllHead(ufgProdNum, &ufgPlayHistBuffer) == false) {
        ready = false;
    } else {
        packet =
            (ufFindTitleIdx(ufgProdNum) * TITLE_PACKETS) + TITLE_FIRST_PACKET;
        if (ufgPlayHistBuffer.LoadCount == ERASED16) {
            ufgPlayHistBuffer.LoadCount = 0;
        }
        if (ufgPlayHistBuffer.SaveCount == ERASED16) {
            ufgPlayHistBuffer.SaveCount = 0;
        }
        if (ufgPlayHistBuffer.GameEst == ERASED8) {
            ufgPlayHistBuffer.GameEst = 0;
        }
        if (ufgPlayHistBuffer.GameCarry == ERASED8) {
            ufgPlayHistBuffer.GameCarry = 0;
        }
        if (ufgPlayHistBuffer.StartCount == ERASED16) {
            ufgPlayHistBuffer.StartCount = COUNT_MAX;
        }
        ufgRestPacket = ufGetFreeBlock();
        if (ufgRestPacket > 2) {
            changed = false;
            ids = port_ids;
            ufLoadPacket(PORT_PACKET, ids);
            ufLoadPacket(PORT_PACKET + 1, ids + PACKET_LONGS);
            ids = device_ids;
            ufLoadPacket(DEVICE_PACKET, ids);
            ufLoadPacket(DEVICE_PACKET + 1, ids + PACKET_LONGS);
            for (i = 0; i < PORTS; i++) {
                per = pdGetPeripheral(ports[i]);
                if (per == NULL) {
                    ufgErrno = UFD_ERR_PERIINITFAIL;
                    return false;
                }
                port_now[i] = per->id;
            }
            for (i = 0; i < PORTS; i++) {
                if (port_now[i] != 0) {
                    for (j = 0; j < LIST_ENTRIES; j++) {
                        if (port_ids[j] == port_now[i]) {
                            break;
                        }
                    }
                    if (j == LIST_ENTRIES) {
                        for (j = 0; j < LIST_ENTRIES; j++) {
                            if (port_ids[j] == (Sint32)ERASED32) {
                                port_ids[j] = port_now[i];
                                changed = true;
                                break;
                            }
                        }
                    }
                }
            }
            /* @hack redundant: a store of UFD_ERR_PERIINITFAIL to ufgErrno that
             * never runs, which with the store below adds uses of the constant,
             * 12, and keeps the pointer loaded at 0x8c65d070 in R14 and the
             * frame's cursor at sp+56 in R13 (0x8c65d076); without it, the two
             * trade and 32 of 473 words differ */
            if (0) {
                ufgErrno = UFD_ERR_PERIINITFAIL;
            }
            if (changed == true) {
                ids = port_ids;
                ufSavePacket(PORT_PACKET, ids);
                /* @hack redundant: a store of UFD_ERR_PERIINITFAIL through ids
                 * that never runs, the second of the two uses of the constant,
                 * 12, which keeps sp+0xc0 in R9 (0x8c65d040 to 0x8c65d04c);
                 * without it, R10 takes it and 20 of 473 words differ */
                if (0) {
                    *ids = UFD_ERR_PERIINITFAIL;
                }
                ufSavePacket(PORT_PACKET + 1, ids + PACKET_LONGS);
            }
            for (i = 0; i < SOCKETS; i++) {
                info = pdGetPeripheralInfo(sockets[i]);
                if (info == NULL) {
                    ufgErrno = UFD_ERR_PERIINITFAIL;
                    return false;
                }
                device_now[i] = info->type;
            }
            changed = false;
            for (i = 0; i < SOCKETS; i++) {
                if (device_now[i] != 0) {
                    for (j = 0; j < LIST_ENTRIES; j++) {
                        if (device_ids[j] == device_now[i]) {
                            break;
                        }
                    }
                    if (j == LIST_ENTRIES) {
                        for (j = 0; j < LIST_ENTRIES; j++) {
                            if (device_ids[j] == (Sint32)ERASED32) {
                                device_ids[j] = device_now[i];
                                changed = true;
                                break;
                            }
                        }
                    }
                }
            }
            if (changed == true) {
                ids = device_ids;
                ufSavePacket(DEVICE_PACKET, ids);
                ufSavePacket(DEVICE_PACKET + 1, ids + PACKET_LONGS);
            }
            port_bits = 0;
            /* @hack redundant: a store of 0 to port_bits that never runs, which
             * keeps port_bits's register allocation, with sp+0xc0 in R9 and 12
             * in R11 (0x8c65d040, 0x8c65d072); without it, the two trade and 25
             * of 473 words differ */
            if (0) {
                port_bits = 0;
            }
            device_bits = 0;
            /* PeriIdx holds a big-endian mask for each list, the controllers'
             * then the devices', with entry 0 in the high bit. a cleared bit
             * marks an entry the title has seen, so an erased mask marks
             * none */
            seen_ports =
                ~load_network_endian(&ufgPlayHistBuffer.PeriIdx[0], MASK_BYTES);
            seen_devices = ~load_network_endian(
                &ufgPlayHistBuffer.PeriIdx[MASK_BYTES], MASK_BYTES);
            for (k = 0; k < LIST_ENTRIES; k++) {
                for (i = 0; i < PORTS; i++) {
                    if (port_now[i] != 0) {
                        if (port_ids[k] == port_now[i]) {
                            port_bits |= 1;
                        }
                    }
                }
                port_bits <<= 1;
                for (i = 0; i < SOCKETS; i++) {
                    if (device_now[i] != 0) {
                        if (device_ids[k] == device_now[i]) {
                            device_bits |= 1;
                        }
                    }
                }
                /* @hack spelling: the shift spelled device_bits =
                 * device_bits << 1, which keeps port_bits in R11 and
                 * device_bits in R13 (SHLL R11 at 0x8c65d2a6, SHLL R13 at
                 * 0x8c65d31e); as <<=, the two trade and 23 of 473 words
                 * differ */
                device_bits = device_bits << 1;
            }
            port_bits = ~((port_bits >> 1) | seen_ports);
            device_bits = ~((device_bits >> 1) | seen_devices);
            SaveNetworkEndiane(port_bits, MASK_BYTES,
                               &ufgPlayHistBuffer.PeriIdx[0]);
            SaveNetworkEndiane(device_bits, MASK_BYTES,
                               &ufgPlayHistBuffer.PeriIdx[MASK_BYTES]);
            ufSavePacket(packet + 1, PACKET1);
        }
        dat_8c8c7ac8 = 0;
        synch_every = 0;
        synch_num = 0;
        ready = true;
    }
    /* @hack redundant: a store of false to ready that never runs, which keeps
     * ready's register allocation, with sp+0xc0 in R9 (0x8c65d040); without it,
     * R10 takes it and 21 of 473 words differ */
    if (0) {
        ready = false;
    }
    return ready;
}

Sint32 ufPlayTime(Uint8 mode, Uint8 est, Uint8 carry, Uint8 param[32])
{
    Sint32 rest;

    rest = 0;
    if (ufgSafeFlag) {
        switch (mode) {
        case UFD_SAVE:
            ufgPlayHistBuffer.SaveCount++;
            /* @hack spelling: the save's count reaches the time stamp by a
             * goto; with the stamp written out in each case, syRtcGetDate's
             * address loads into R1, not R2 (MOV.L at 0x8c65d3e2), and 2 of
             * 97 words differ */
            goto stamp;
        case UFD_LOAD:
            ufgPlayHistBuffer.LoadCount++;
        stamp:
            syRtcGetDate(&now_date);
            syRtcDateToCount(&now_date, &action_count);
            break;
        case UFD_PLAY:
            break;
        }
        if (mode != UFD_LOAD) {
            ufgPlayHistBuffer.GameEst = est;
            ufgPlayHistBuffer.GameCarry = carry;
        }
        if (param != NULL) {
            memcpy(ufgPlayHistBuffer.FreeArea, param, 32);
        }
        rest = ufgRestPacket;
        /* every call sets UFD_ERR_NO_PACKET_LEFT, whether or not packets are
         * left */
        ufgErrno = UFD_ERR_NO_PACKET_LEFT;
        if (--synch_left == 0 && synch_every != 0) {
            rest = ufSynch();
            if (rest == true) {
                rest = ufgRestPacket;
            }
        }
    } else {
        ufgErrno = UFD_ERR_NO_INIT;
    }
    return rest;
}

Bool ufSynch(void)
{
    Bool synched;

    synched = false;
    if (ufgSafeFlag) {
        synched = ufSaveAction(synch_history, 2);
    } else {
        ufgErrno = UFD_ERR_NO_INIT;
    }
    return synched;
}

Bool ufAutoSynch(Uint32 count)
{
    Bool set;

    set = false;
    if (ufgSafeFlag) {
        synch_every = count;
        synch_left = count;
        set = true;
    } else {
        ufgErrno = UFD_ERR_NO_INIT;
    }
    return set;
}

Bool ufChangeAutoSave(Uint8 Flag)
{
    Bool changed;
    Sint32 title;

    changed = false;
    if (ufgSafeFlag) {
        title = ufFindTitleIdx(ufgProdNum);
        if (title >= 0) {
            ufgPlayHistBuffer.AutoSave = Flag;
            make_room();
            /* @bug logic: the record's packet 0 is saved to packet title, the
             * title's index, not to where the record starts, title *
             * TITLE_PACKETS + TITLE_FIRST_PACKET, as ufSaveAction and
             * ufPlayInit count it (MOV R14,R4 at 0x8c65d50c, in the slot of the
             * JSR to ufSavePacket). flash keeps the old AutoSave, and packet
             * title takes the record's packet 0. for title 0, that overwrites
             * the shared flags in packet 0 */
            ufSavePacket(title, &ufgPlayHistBuffer);
            changed = true;
        } else {
            ufgErrno = UFD_ERR_NOT_FOUND;
        }
    } else {
        ufgErrno = UFD_ERR_NO_INIT;
    }
    return changed;
}

Bool ufTitleDelete(void)
{
    Bool deleted;

    deleted = false;
    if (ufgSafeFlag) {
        deleted = ufSaveAction(delete_title, 4);
    } else {
        ufgErrno = UFD_ERR_NO_INIT;
    }
    return deleted;
}

Bool ufConnect(Uint32 time)
{
    Bool saved;

    saved = false;
    if (ufgSafeFlag) {
        connect_time = time;
        saved = ufSaveAction(start_net_play, 1);
        if (saved == true) {
            connected = true;
        }
    } else {
        ufgErrno = UFD_ERR_NO_INIT;
    }
    return saved;
}

Bool ufDisConnect(void)
{
    Bool saved;

    saved = false;
    if (ufgSafeFlag) {
        if (connected == true) {
            saved = ufSaveAction(end_net_play, 1);
        }
    } else {
        ufgErrno = UFD_ERR_NO_INIT;
    }
    return saved;
}
