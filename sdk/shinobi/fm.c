/* the flash memory manager: packets kept as a log in a partition, and cached
 * whole in a work area */
#include <sg_xpt.h>
#include <shinobi.h>

/* one past the boot ROM's last partition */
#define PARTITIONS 5
/* a packet's size, as a shift and in bytes, and the bytes its CRC covers: its
 * number and data */
#define PACKET_SHIFT 6
#define PACKET_BYTES (1 << PACKET_SHIFT)
#define PACKET_CHECKED (2 + FMD_DATA_BYTES)
/* the bytes a map packet covers, as a shift: a bit for each of 512 packets */
#define MAP_SHIFT 15
/* the number an erased packet reads */
#define ERASED 0xffff
/* a formatted partition's ID, its bytes, and the bytes of it fmMngInit
 * checks */
#define FORMAT_ID "KATANA_FLASH____"
#define ID_BYTES 16
#define ID_CHECKED 12
/* the header bytes fmMngFormat writes: the ID, the partition and the type */
#define HEADER_BYTES (ID_BYTES + 2)

/* flash_work.request's values: the requests fmMngExecServer runs */
#define REQUEST_NONE 0
#define REQUEST_READ 1
#define REQUEST_WRITE 2
/* flash_work.phase's values: the steps of a write. a write appends its packet.
 * when the log is full, it erases the partition and then writes the cache
 * back */
#define PHASE_APPEND 0
#define PHASE_ERASE 1
#define PHASE_STORE 2
/* append_packet's result when the log is full, apart from the FMD_ errors it
 * returns */
#define LOG_FULL (-10)
/* write_step's result while a phase of the write remains */
#define WRITE_UNFINISHED 1

/* a packet: its number, its data, and the CRC of both */
struct flash_packet {
    Uint16 number;
    Uint8 data[FMD_DATA_BYTES];
    Uint16 crc;
};

/* a partition's first packet. fmMngFormat writes its first HEADER_BYTES, and
 * leaves the rest erased */
struct flash_header {
    char id[ID_BYTES];
    Sint8 part;
    Sint8 type;
    Uint8 reserved[PACKET_BYTES - HEADER_BYTES];
};

/* the open partition. on flash, packet 0 is the header, packets 1 to capacity
 * are the log, and the map follows from packet capacity + 1 */
struct flash_work {
    Sint16 part;
    Sint16 type; /* FMD_WRITABLE or FMD_PROTECTED */
    Sint32 address; /* the partition's flash address */
    Sint32 size; /* the partition's bytes */
    /* the work area fmMngInit takes. it holds the partition's layout, but with
     * slots where the log is. slot n takes the place of the log's packet n + 1,
     * and holds the latest packet numbered n */
    Uint8 *cache;
    Sint32 used; /* the packets the log holds */
    /* the most packets the log can hold, which is also how many packet
     * numbers there are */
    Sint32 capacity;
    /* the map's packets. the map has a bit for each place in the log, in order,
     * the most significant bit of each byte first. a bit is clear for each
     * packet the log holds, and set past them */
    Sint32 map_packets;
    Sint32 request; /* the request to run, a REQUEST_ value */
    Sint32 status; /* fmMngExecServer's result, an FMD_STAT_ value */
    Sint32 phase; /* the write's next step, a PHASE_ value */
    Sint32 number; /* the request's packet number */
    Sint32 error; /* the result of the last request to finish */
    void *buffer; /* the request's buffer */
    /* a packet as it is read from flash, or built to be written to it */
    union {
        struct flash_packet packet;
        struct flash_header header;
    } io;
};

/* the open partition, or NULL */
static struct flash_work *active = NULL;
static struct flash_work storage;
const char *const fmgBuild = "\nfmMng Ver 1.01 Build:Oct 12 1998 19:03:00\n";

static Sint32 bytes_equal(const char *left, const char *right, Sint32 count)
{
    while (count-- > 0) {
        if (*left++ != *right++) {
            return 0;
        }
    }
    return 1;
}

/* copy bytes / 4 Sint32 words from source to destination, which must both be
 * aligned to 4 */
static void copy_words(const void *source, void *destination, Sint32 bytes)
{
    const Sint32 *from = source;
    Sint32 *to = destination;

    bytes /= 4;
    while (bytes-- > 0) {
        *to++ = *from++;
    }
}

static void copy_bytes(const Uint8 *from, Uint8 *to, Sint32 count)
{
    while (count-- > 0) {
        *to++ = *from++;
    }
}

static void erase_cache(Sint8 *cache, Sint32 bytes)
{
    for (; bytes > 0; bytes--) {
        *cache++ = -1;
    }
}

/* the number of packets the log holds: the map's clear bits before its first
 * set bit */
static Sint32 count_used(const Sint8 *map, Sint32 map_packets)
{
    Sint32 count = 0;
    Sint32 bytes;

    for (bytes = map_packets * PACKET_BYTES; bytes > 0; bytes--) {
        Sint8 bits = *map++;
        Sint32 bit;

        for (bit = 8; bit > 0; bit--) {
            if (bits & 0x80) {
                return count;
            }
            bits <<= 1;
            count++;
        }
    }
    return count;
}

/* write the map of a log that holds used packets: used clear bits, then set
 * bits to the map's end. it returns 0 */
static Sint32 make_map(Sint32 used, Sint8 *map, Sint32 map_packets)
{
    Sint32 bytes;
    Sint32 bit;

    for (bytes = map_packets * PACKET_BYTES; bytes > 0; bytes--) {
        if (used >= 8) {
            *map++ = 0;
            used -= 8;
        } else if (used > 0) {
            Sint8 bits;

            for (bit = 0; bit < 8; bit++) {
                /* @bug uninitialized: bits is shifted before anything
                 * sets it. SHC keeps bits in R6, which still holds
                 * map_packets here: MOV R6,R0 at 0x8c61d40c copies
                 * map_packets out for the byte count, and only this branch
                 * writes R6, at 0x8c61d42a and 0x8c61d436. the map still
                 * comes out right, since the eight shifts push all of
                 * map_packets's bits out of the byte stored at 0x8c61d43e.
                 * set to map_packets before the loops, 19 of 40 words
                 * differ */
                /* NOLINTNEXTLINE(clang-diagnostic-uninitialized) */
                bits <<= 1;
                if (used > 0) {
                    used--;
                } else {
                    bits |= 1;
                }
            }
            *map++ = bits;
        } else {
            *map++ = -1;
        }
    }
    return 0;
}

/* erase packet whole, as a packet never written is, when its data is all
 * 0xff and it passes its CRC. fmMngRead reads the same data from both. it
 * returns 1 when it erases packet, and 0 otherwise */
static Sint32 erase_if_blank(struct flash_work *work,
                             struct flash_packet *packet)
{
    Uint8 *at = packet->data;
    Sint32 count;
    Uint16 crc;

    for (count = FMD_DATA_BYTES; count > 0; count--) {
        if (*at++ != 0xff) {
            return 0;
        }
    }
    crc = fmcalccrc(PACKET_CHECKED, (Uint8 *)packet);
    if (packet->crc != crc) {
        return 0;
    }
    at = (Uint8 *)packet;
    for (count = PACKET_BYTES; count > 0; count--) {
        *at++ = 0xff;
    }
    return 1;
}

/* read the header and the map into the cache, which the caller has erased.
 * then read the log's packets in order, each into its number's slot, so the
 * latest packet of each number stays. the map gives the log's length, and an
 * erased packet ends the log early. a packet that fails its CRC, or whose
 * number is capacity or more, is left out. it returns 0, or FMD_READ_FAILED
 * when a read fails. it returns FMD_WRONG_TYPE for a partition not
 * FMD_WRITABLE, with its log unread */
static Sint32 load_cache(struct flash_work *work)
{
    Sint32 address = work->address;
    Uint8 *cache = work->cache;
    Sint32 result;
    Sint32 map_offset;
    Sint32 remaining;
    Sint32 scan;
    Uint16 crc;
    struct flash_packet *packet;

    result = syBtFlRead(address, cache, PACKET_BYTES);
    if (result == -1) {
        return FMD_READ_FAILED;
    }
    map_offset = (work->capacity + 1) * PACKET_BYTES;
    result = syBtFlRead(address + map_offset, cache + map_offset,
                        work->map_packets * PACKET_BYTES);
    if (result == -1) {
        return FMD_READ_FAILED;
    }
    remaining = work->used =
        count_used((Sint8 *)(cache + map_offset), work->map_packets);
    scan = address + PACKET_BYTES;
    if (work->type == FMD_WRITABLE) {
        packet = &work->io.packet;
        for (; remaining > 0; remaining--, scan += PACKET_BYTES) {
            result = syBtFlRead(scan, packet, PACKET_BYTES);
            if (result == -1) {
                return FMD_READ_FAILED;
            }
            if (packet->number == ERASED) {
                break;
            }
            crc = fmcalccrc(PACKET_CHECKED, (Uint8 *)packet);
            if (packet->crc == crc && packet->number < work->capacity) {
                copy_words(packet,
                           cache + ((packet->number + 1) * PACKET_BYTES),
                           PACKET_BYTES);
            }
        }
        return 0;
    }
    return FMD_WRONG_TYPE;
}

/* write the cache to the erased partition: the header, then the slots'
 * packets in number order as the new log, then its map. an erased slot is
 * left out, and so is a blank one, which erase_if_blank erases. it returns 0,
 * or FMD_WRITE_FAILED when a write fails. it returns FMD_WRONG_TYPE for a
 * partition not FMD_WRITABLE, and writes nothing */
static Sint32 store_cache(struct flash_work *work)
{
    Sint32 address;
    Sint32 target;
    Sint32 map_offset;
    Sint32 result;
    Sint32 used;
    Sint32 remaining;
    struct flash_packet *packet;

    address = work->address;
    if (work->type == FMD_WRITABLE) {
        packet = (struct flash_packet *)work->cache;
        target = address;
        result = syBtFlWrite(address, packet++, PACKET_BYTES);
        if (result != PACKET_BYTES) {
            return FMD_WRITE_FAILED;
        }
        target += PACKET_BYTES;
        used = 0;
        for (remaining = work->capacity; remaining > 0; remaining--, packet++) {
            /* @hack spelling: two tests, not one &&, which have SHC load 0xffff
             * into R4 before the number (0x8c61d5c8). joined by &&, R3 takes
             * 0xffff after the number, and 4 of 94 words differ */
            if (packet->number != ERASED) {
                if (!erase_if_blank(work, packet)) {
                    result = syBtFlWrite(target, packet, PACKET_BYTES);
                    if (result != PACKET_BYTES) {
                        return FMD_WRITE_FAILED;
                    }
                    target += PACKET_BYTES;
                    used++;
                }
            }
        }
        /* packet is past the last slot, at the map */
        map_offset = (work->capacity + 1) * PACKET_BYTES;
        make_map(used, (Sint8 *)packet, work->map_packets);
        result = syBtFlWrite(address + map_offset, packet,
                             work->map_packets * PACKET_BYTES);
        if (result != work->map_packets * PACKET_BYTES) {
            return FMD_WRITE_FAILED;
        }
        work->used = used;
        return 0;
    }
    return FMD_WRONG_TYPE;
}

/* build the request's packet and copy it into its slot. then append it to
 * the log: the map first, with one more clear bit, and then the packet. it
 * returns 0, or FMD_WRITE_FAILED when a write fails. when the log is full,
 * it returns LOG_FULL with only the slot written. it returns FMD_WRONG_TYPE for
 * a partition not FMD_WRITABLE */
static Sint32 append_packet(struct flash_work *work)
{
    struct flash_packet *packet;
    Sint32 result;
    Sint32 offset;
    Sint32 map_offset;
    Uint16 crc;

    if (work->type == FMD_WRITABLE) {
        packet = &work->io.packet;
        packet->number = work->number;
        copy_bytes(work->buffer, packet->data, FMD_DATA_BYTES);
        crc = fmcalccrc(PACKET_CHECKED, (Uint8 *)packet);
        packet->crc = crc;
        offset = (work->number + 1) * PACKET_BYTES;
        copy_words(packet, work->cache + offset, PACKET_BYTES);
        if (work->used >= work->capacity) {
            return LOG_FULL;
        }
        map_offset = (work->capacity + 1) * PACKET_BYTES;
        make_map(work->used + 1, (Sint8 *)work->cache + map_offset,
                 work->map_packets);
        result =
            syBtFlWrite(work->address + map_offset, work->cache + map_offset,
                        work->map_packets * PACKET_BYTES);
        if (result != work->map_packets * PACKET_BYTES) {
            return FMD_WRITE_FAILED;
        }
        offset = (work->used + 1) * PACKET_BYTES;
        work->used++;
        result = syBtFlWrite(work->address + offset, packet, PACKET_BYTES);
        if (result != PACKET_BYTES) {
            return FMD_WRITE_FAILED;
        }
        return 0;
    }
    return FMD_WRONG_TYPE;
}

/* copy the data of the request's packet from its slot to the request's
 * buffer. it returns FMD_BAD_CRC when the slot fails its CRC, with the data
 * copied all the same, and 0 otherwise. a slot never written gives 0xff bytes,
 * unchecked. it returns FMD_WRONG_TYPE for a partition not FMD_WRITABLE */
static Sint32 read_packet(struct flash_work *work)
{
    struct flash_packet *slot;
    Uint16 crc;

    if (work->type == FMD_WRITABLE) {
        slot = (struct flash_packet *)(work->cache +
                                       ((work->number + 1) * PACKET_BYTES));
        copy_bytes(slot->data, work->buffer, FMD_DATA_BYTES);
        if (slot->number != ERASED) {
            crc = fmcalccrc(PACKET_CHECKED, (Uint8 *)slot);
            if (slot->crc != crc) {
                return FMD_BAD_CRC;
            }
        }
        return 0;
    }
    return FMD_WRONG_TYPE;
}

/* run a write's next phase. PHASE_APPEND appends the packet. when the log is
 * full, PHASE_ERASE erases the partition on the next call, and PHASE_STORE
 * writes the cache back on the call after. it returns WRITE_UNFINISHED while
 * a phase remains, and then the write's result */
static Sint32 write_step(struct flash_work *work)
{
    Sint32 result;
    /* @hack spelling: appended, a local apart from result. SHC passes it
     * through R5 (0x8c61d794), and the append and erase phases share one
     * phase store (0x8c61d7b0). with result in its place, 31 of 43 words
     * differ and the function is 2 bytes shorter */
    Sint32 appended;

    switch (work->phase) {
    case PHASE_APPEND:
        appended = append_packet(work);
        if (appended == LOG_FULL) {
            work->phase = PHASE_ERASE;
            result = WRITE_UNFINISHED;
        } else {
            result = appended;
        }
        break;
    case PHASE_ERASE:
        result = syBtFlDeleteSct(work->address);
        if (result == -1) {
            result = FMD_ERASE_FAILED;
        } else {
            work->phase = PHASE_STORE;
            result = WRITE_UNFINISHED;
        }
        break;
    case PHASE_STORE:
        result = store_cache(work);
        break;
    default:
        result = FMD_WRITE_FAILED;
        break;
    }
    return result;
}

Sint32 fmMngInit(Sint32 part, void *work)
{
    SY_BT_FL_BLKINFO info;
    Sint32 result;
    struct flash_header *header;

    if (part < 1 || part >= PARTITIONS) {
        return FMD_BAD_PARTITION;
    }
    active = &storage;
    syBtFlGetInfo(part, &info);
    active->part = part;
    active->address = info.fmaddr;
    active->size = info.size;
    /* the map takes a packet for every 512 packets of the partition, rounded
     * up. the log takes the packets the header and the map leave */
    active->map_packets = (active->size + ((1 << MAP_SHIFT) - 1)) >> MAP_SHIFT;
    active->capacity = (active->size >> PACKET_SHIFT) - active->map_packets - 1;
    active->cache = work;
    active->request = REQUEST_NONE;
    active->status = FMD_STAT_NONE;
    active->error = 0;
    result = syBtFlRead(active->address, &active->io, PACKET_BYTES);
    if (result == -1) {
        return FMD_READ_FAILED;
    }
    header = &active->io.header;
    result = bytes_equal(header->id, FORMAT_ID, ID_CHECKED);
    if (!result) {
        return FMD_BAD_HEADER;
    }
    if (active->part != header->part) {
        return FMD_BAD_HEADER;
    }
    active->type = (Sint16)header->type;
    if (active->type != FMD_WRITABLE && active->type != FMD_PROTECTED) {
        return FMD_BAD_HEADER;
    }
    erase_cache((Sint8 *)active->cache, active->size);
    result = load_cache(active);
    /* @hack spelling: result is 0 past the test, so both returns give it.
     * the image still tests result and loads a 0 of its own (0x8c61d8bc,
     * 0x8c61d8c4). returned directly, 14 of 118 words differ */
    if (result != 0) {
        return result;
    }
    return 0;
}

Sint32 fmMngFormat(Sint32 type)
{
    struct flash_header *header;
    Sint32 result;

    if (active == NULL) {
        return FMD_NOT_OPEN;
    }
    if (type != FMD_WRITABLE && type != FMD_PROTECTED) {
        return FMD_WRONG_TYPE;
    }
    result = syBtFlDeleteSct(active->address);
    if (result != 0) {
        return FMD_ERASE_FAILED;
    }
    header = &active->io.header;
    copy_bytes((const Uint8 *)FORMAT_ID, (Uint8 *)header->id, ID_BYTES);
    active->type = type;
    header->type = type;
    header->part = active->part;
    result = syBtFlWrite(active->address, header, HEADER_BYTES);
    if (result != HEADER_BYTES) {
        return FMD_WRITE_FAILED;
    }
    erase_cache((Sint8 *)active->cache, active->size);
    result = load_cache(active);
    /* @hack spelling: result is 0 past the test, so both returns give it.
     * the image still tests result and loads a 0 of its own (0x8c61d94c,
     * 0x8c61d954). returned directly, 21 of 72 words differ */
    if (result != 0) {
        return result;
    }
    return 0;
}

Sint32 fmMngRead(Sint32 packet, void *buffer)
{
    if (active == NULL) {
        return FMD_NOT_OPEN;
    }
    if (active->request != REQUEST_NONE) {
        return FMD_REQUEST_PENDING;
    }
    if (packet < 0 || packet >= active->capacity) {
        return FMD_NO_PACKET;
    }
    active->request = REQUEST_READ;
    active->status = FMD_STAT_RUNNING;
    active->number = packet;
    active->buffer = buffer;
    return 0;
}

Sint32 fmMngWrite(Sint32 packet, void *buffer)
{
    if (active == NULL) {
        return FMD_NOT_OPEN;
    }
    if (active->request != REQUEST_NONE) {
        return FMD_REQUEST_PENDING;
    }
    if (packet < 0 || packet >= active->capacity) {
        return FMD_NO_PACKET;
    }
    active->request = REQUEST_WRITE;
    active->status = FMD_STAT_RUNNING;
    active->number = packet;
    active->buffer = buffer;
    active->phase = PHASE_APPEND;
    return 0;
}

Sint32 fmMngExecServer(void)
{
    if (active == NULL) {
        return FMD_STAT_NONE;
    }
    switch (active->request) {
    case REQUEST_NONE:
        break;
    case REQUEST_READ: {
        Sint32 result = read_packet(active);

        active->request = REQUEST_NONE;
        if (result != 0) {
            active->status = FMD_STAT_ERR;
        } else {
            active->status = FMD_STAT_COMPLETE;
        }
        active->error = result;
        break;
    }
    case REQUEST_WRITE: {
        Sint32 result = write_step(active);

        if (result == WRITE_UNFINISHED) {
            break;
        }
        active->request = REQUEST_NONE;
        if (result != 0) {
            active->status = FMD_STAT_ERR;
        } else {
            active->status = FMD_STAT_COMPLETE;
        }
        active->error = result;
        break;
    }
    default:
        active->request = REQUEST_NONE;
        active->status = FMD_STAT_NONE;
        break;
    }
    return active->status;
}

Sint32 fmMngGetErr(void)
{
    if (active == NULL) {
        return FMD_NOT_OPEN;
    }
    return active->error;
}

void fmMngFinish(void)
{
    active = NULL;
}

Sint32 fmMngRefresh(void)
{
    Sint32 result;

    if (active == NULL) {
        return FMD_NOT_OPEN;
    }
    result = syBtFlDeleteSct(active->address);
    if (result == -1) {
        result = FMD_ERASE_FAILED;
    } else {
        result = store_cache(active);
    }
    return result;
}

Sint32 fmMngGetFree(void)
{
    if (active == NULL) {
        return FMD_NOT_OPEN;
    }
    return active->capacity - active->used;
}
