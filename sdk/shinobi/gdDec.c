/* GD-ROM subcode Q channel position decoding */
#include <sg_xpt.h>

/* the subcode as the drive returns it: a four-byte header, then a byte for
 * each of a frame's 96 symbols. the header's bytes 2 and 3 hold the size, high
 * byte first. a channel takes one bit of every symbol, 12 bytes in all, and
 * the Q channel's is bit 6 */
#define SUBCODE_HEADER 4
#define SUBCODE_SIZE 100
#define SYMBOL_Q 0x40
#define CHANNEL_SIZE 12

/* the Q channel's twelve bytes: control and mode, track and index, the time
 * within the track, a zero, the absolute time, and a CRC */
#define Q_CONTROL 0
#define Q_TRACK 1
#define Q_INDEX 2
#define Q_MINUTE 3
#define Q_SECOND 4
#define Q_FRAME 5
#define Q_DISC_TIME 7
#define Q_CRC 10
/* the mode, in the control byte's low four bits; mode 1 carries a
 * position */
#define Q_MODE_MASK 0x0f
#define Q_MODE_POSITION 1

#define FRAMES_PER_SECOND 75

/* gdDecSubcode's failures */
#define BAD_HEADER (-1) /* the header's first byte isn't 0 */
#define BAD_SIZE (-2) /* the size isn't SUBCODE_SIZE */
#define BAD_CRC (-3) /* the Q channel fails its CRC */
#define NO_POSITION (-5) /* the mode isn't Q_MODE_POSITION */

/* the head position, from a frame's subcode Q channel, as gdfs.h declares it
 * for gdDecSubcode's callers. minute, second and frame are the time within
 * the track */
struct subcode_position {
    Uint32 track;
    Uint32 minute;
    Uint32 second;
    Uint32 frame;
    /* the control byte in bits 24 to 31, above the absolute time in
     * frames */
    Uint32 disc_frame;
    Uint32 index;
};

/* CRC-16 table for the polynomial 0x1021, most significant bit first: each
 * entry is its index times x^16, modulo the polynomial */
static const Uint16 crc_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 0x8108,
    0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef, 0x1231, 0x0210,
    0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6, 0x9339, 0x8318, 0xb37b,
    0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de, 0x2462, 0x3443, 0x0420, 0x1401,
    0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee,
    0xf5cf, 0xc5ac, 0xd58d, 0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6,
    0x5695, 0x46b4, 0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d,
    0xc7bc, 0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b, 0x5af5,
    0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc,
    0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a, 0x6ca6, 0x7c87, 0x4ce4,
    0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd,
    0xad2a, 0xbd0b, 0x8d68, 0x9d49, 0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13,
    0x2e32, 0x1e51, 0x0e70, 0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a,
    0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e,
    0xe16f, 0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 0x02b1,
    0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256, 0xb5ea, 0xa5cb,
    0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d, 0x34e2, 0x24c3, 0x14a0,
    0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8,
    0xe75f, 0xf77e, 0xc71d, 0xd73c, 0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657,
    0x7676, 0x4615, 0x5634, 0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9,
    0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882,
    0x28a3, 0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92, 0xfd2e,
    0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07,
    0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1, 0xef1f, 0xff3e, 0xcf5d,
    0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74,
    0x2e93, 0x3eb2, 0x0ed1, 0x1ef0};

/* the CRC-16 of size bytes at data, from seed */
static Uint16 crc_update(Uint32 seed, Sint8 *data, Sint32 size)
{
    Uint32 crc = seed;
    /* @hack spelling: byte, a copy of data to walk, which SHC moves to R13
     * (MOV R5,R13 at 0x8c65e7dc in 1ST_READ.BIN, 0x8ce0b42a in LOADER.BIN).
     * walking data itself leaves it in R5, and 27 of 29 words differ in
     * 1ST_READ.BIN and 29 of 31 in LOADER.BIN */
    Sint8 *byte = data;

    while (size-- > 0) {
        crc = (((Uint16)crc << 8) & 0xff00) ^
              (Sint16)crc_table[(Uint8)((Uint16)crc >> 8) ^ (Uint8)*byte++];
    }
    return crc;
}

/* check crc, the CRC a Q channel stores after its first Q_CRC bytes. the
 * channel stores their CRC-16 inverted. it returns 0 when crc matches, and -1
 * otherwise. size is never read */
static Sint32 crc_check(Sint32 size, Sint8 *data, Uint16 crc)
{
    /* @hack spelling: actual, the inverted CRC through a local of its own.
     * SHC reads crc back from the stack after the call (MOV.W @R15,R2 at
     * 0x8c65e81e in 1ST_READ.BIN, 0x8ce0b470 in LOADER.BIN). as one
     * conditional expression, SHC holds crc across the call in R8, which it
     * saves. the function is then 4 bytes longer, and 21 of 21 words differ
     * in each image */
    Uint16 actual = ~crc_update(0, data, Q_CRC);

    if (crc != actual) {
        return -1;
    }
    return 0;
}

static Sint32 unpack_bcd(Sint8 value)
{
    return (((value & 0xf0) >> 4) * 10) + (value & 0x0f);
}

/* a BCD minute, second and frame, as a count of frames */
static Sint32 time_to_frames(Sint8 *time)
{
    Sint32 minute;
    Sint32 second;
    Sint32 frame;

    minute = unpack_bcd(*time++);
    second = unpack_bcd(*time++);
    frame = unpack_bcd(*time);
    return (((minute * 60) + second) * FRAMES_PER_SECOND) + frame;
}

Sint32 gdDecSubcode(Sint8 *subcode, struct subcode_position *position)
{
    struct {
        Sint8 p[CHANNEL_SIZE];
        Sint8 q[CHANNEL_SIZE];
    } channels;
    Sint8 *symbol;
    int i;
    int bit;

    if (subcode[0] != 0) {
        return BAD_HEADER;
    }
    if ((subcode[2] * 256) + subcode[3] != SUBCODE_SIZE) {
        return BAD_SIZE;
    }
    symbol = subcode + SUBCODE_HEADER;
    for (i = 0; i < CHANNEL_SIZE; i++) {
        /* the P channel is only cleared: the loop gathers the Q bit alone */
        channels.p[i] = 0;
        channels.q[i] = 0;
        for (bit = 7; bit >= 0; bit--) {
            if (bit == 7) {
                channels.q[i] |= (*symbol++ & SYMBOL_Q) << 1;
            } else {
                channels.q[i] |= (*symbol++ & SYMBOL_Q) >> (6 - bit);
            }
        }
        if ((channels.q[Q_CONTROL] & Q_MODE_MASK) != Q_MODE_POSITION) {
            return NO_POSITION;
        }
    }
    if (crc_check(CHANNEL_SIZE, channels.q,
                  ((Uint8)channels.q[Q_CRC] << 8) |
                      (Uint8)channels.q[Q_CRC + 1])) {
        return BAD_CRC;
    }
    position->track = unpack_bcd(channels.q[Q_TRACK]);
    position->minute = unpack_bcd(channels.q[Q_MINUTE]);
    position->second = unpack_bcd(channels.q[Q_SECOND]);
    position->frame = unpack_bcd(channels.q[Q_FRAME]);
    position->disc_frame = ((Uint32)channels.q[Q_CONTROL] << 24) |
                           time_to_frames(channels.q + Q_DISC_TIME);
    position->index = unpack_bcd(channels.q[Q_INDEX]);
    return 0;
}
