/* Maple bus driver: the command list in the frame being built, and each
 * port's and device's state from the responses */
#include <kaede.h>
#include <machine.h>
#include <mpdrv.h>
#include <sg_xpt.h>

/* the tags that start the parts of _mp_Context, 'PORT' to 'END ', as
 * little-endian words */
#define TAG_PORT 0x54524f50
#define TAG_DEV 0x20564544
#define TAG_INFO 0x4f464e49
#define TAG_ENBL 0x4c424e45
#define TAG_STAT 0x54415453
#define TAG_RADR 0x52444152
#define TAG_TOUT 0x54554f54
#define TAG_DRQC 0x43515244
#define TAG_FUNC 0x434e5546
#define TAG_END 0x20444e45

/* a header word. the port is in bits 16 and 17, and the pattern in bits 8 to
 * 10. bit 31 marks the frame's last header word. a command's header has pattern
 * 0, and bits 0 to 7 count its data words. its response address and its command
 * word follow it. a header with pattern 2, 3, 4 or 7 stands alone, with no word
 * after it */
#define PATTERN_GUN_ON (2 << 8)
#define PATTERN_RESET (3 << 8)
#define PATTERN_GUN_OFF (4 << 8)
#define PATTERN_NOP (7 << 8)
#define LAST_COMMAND 0x80000000
/* a command's words ahead of its data: the header, the response address and
 * the command word */
/* @hack type: unsigned long, with which SHC masks both ports in mpdrv_setcmd
 * with a 3 it loads into R5 (MOV #3,R5 at 0x8c124d0a; AND R5,R4 at
 * 0x8c124d28 and 0x8c124d4a); as an int 3, it masks the first with AND #3 in
 * R0 and the second with a 3 of its own in R4, and 56 of mpdrv_setcmd's 291
 * words differ */
#define COMMAND_HEAD 3UL

/* the size of each device's response area in recvbuf, and of each frame
 * buffer in sendbuf */
#define RESPONSE_SIZE 0x400
#define FRAME_SIZE 0x3000
/* the bits of the first n ports, from port A, as port_bits holds them */
#define FIRST_PORTS(n) ((1 << (n)) - 1)

/* the system bus's revision, in its low byte */
/* @hack type: Sint32, whose low byte SHC compares with CMP/GT (0x8c124adc in
 * 1ST_READ.BIN); as Uint32, it compares with CMP/HI, and 1 of 265 words differ
 * in each image */
#define BUS_REVISION (*(volatile Sint32 *)0xa05f689c)
#define MAPLE_REGISTERS ((volatile Sint32 *)0xa05f6c00)

/* a word of the bytes a, b, c and d, a the most significant */
#define PACK_WORD(a, b, c, d)                                                  \
    ((((a) << 24) & 0xff000000) | (((b) << 16) & 0xff0000) |                   \
     (((c) << 8) & 0xff00) | ((d) & 0xff))
/* x with its four bytes in reverse order */
#define SWAP_WORD(x)                                                           \
    ((((x) << 24) & 0xff000000) | (((x) << 8) & 0xff0000) |                    \
     (((x) >> 8) & 0xff00) | (((x) >> 24) & 0xff))
/* the 4-byte words in record, as fill_words counts them */
#define WORDS(record) (sizeof(record) / 4)

static Sint32 decide_presence(struct maple_device *dev, Uint32 *response);
static Sint32 clear_response(struct maple_device *dev, Uint32 *response);
static Sint32 read_response(struct maple_device *dev, Uint32 *response);
static Sint32 update_gun_root(Sint32 port, KDS_DEVICEINFO *info,
                              struct maple_device *dev, Uint32 *response);
static Sint32 update_root(Sint32 port, KDS_DEVICEINFO *info,
                          struct maple_device *dev, Uint32 *response);
static Sint32 update_gun_expansion(Uint8 socket_bit, KDS_PORT *state,
                                   KDS_DEVICEINFO *info,
                                   struct maple_device *dev, Uint32 *response);
static Sint32 update_expansion(Uint8 socket_bit, KDS_PORT *state,
                               KDS_DEVICEINFO *info, struct maple_device *dev,
                               Uint32 *response);
static Sint32 read_info(KDS_DEVICEINFO *info, struct maple_device *dev,
                        Uint32 *response);
static Sint32 clear_info(KDS_DEVICEINFO *info);
static Sint32 fill_words(void *record, Sint32 value, Sint32 count);
static void set_registers(void);

/* an expansion device's connector_dir, by the low four bits of the direction
 * byte in its information. 1 << n gives n, for n from 1 to 3, and any other
 * value gives 0. it isn't const: the image holds it in section D, not with
 * the constants in C */
static char expansion_direction[16] = {0, 0, 1, 0, 2, 0, 0, 0,
                                       3, 0, 0, 0, 0, 0, 0, 0};

void mpdrv_init_globals(void)
{
    Sint32 i;

    _mp->recv_side = 0;
    _mp->send_side = 0;
    for (i = 0; i < KDD_DEV_MAX; i++) {
        _mp->recv_data[0][i] = (Uint32 *)(_mp->recvbuf + (i * RESPONSE_SIZE));
        _mp->recv_data[1][i] =
            (Uint32 *)(_mp->recvbuf + ((i + KDD_DEV_MAX) * RESPONSE_SIZE));
    }
    _mp->send_frame[0] = (Uint8 *)_mp->sendbuf;
    _mp->send_frame[1] = (Uint8 *)(_mp->sendbuf + FRAME_SIZE);
    _mp->send_end = (Uint8 *)(_mp->sendbuf + (2 * FRAME_SIZE));
    _mp->last_cmd = (Uint32 *)_mp->send_frame[0];
    fill_words(_mp->port, 0, WORDS(_mp->port));
    fill_words(_mp->device, 0, WORDS(_mp->device));
    for (i = 0; i < KDD_DEV_MAX; i++) {
        clear_info(&_mp->info[i]);
    }
    for (i = 0; i < KDD_DEV_MAX; i++) {
        _mp->enable[i] = 0;
    }
    /* @hack spelling: a bare scope, in which SHC copies port_bits' initializer
     * after the loops above (JSR @R3 at 0x8c124ad0 in 1ST_READ.BIN); declared
     * at the function's top, the function runs 4 bytes longer and 235 of 265
     * words differ in each image */
    {
        char port_bits[MPDRV_PORTS] = {1, 2, 4, 8};
        char ports;
        Sint32 port;
        Sint32 socket;

        if ((BUS_REVISION & 0xff) <= 1) {
            ports = FIRST_PORTS(3);
        } else {
            ports = FIRST_PORTS(MPDRV_PORTS);
        }
        for (port = 0; port < MPDRV_PORTS; port++) {
            if (port_bits[port] & ports) {
                _mp->enable[port * PORT_DEVICES] = 'R';
                for (socket = 0; socket < 2; socket++) {
                    _mp->enable[(port * PORT_DEVICES) + 1 + socket] = 'E';
                }
            }
        }
    }
    _mp->timeout = (Sint32)MAPLE_TIMEOUT(START_TIMEOUT);
    _mp->send_size = 0;
    _mp->last_cmd_words = 0;
    _mp->next_seq = 1;
    _mp->debugmode = 0;
    _mp->unk_10e4 = 0;
    _mp->recv_pending = 0;
    _mp->nopcount = 5;
    _mp->speed = 0;
    _mp->dreqcount = 10;
    _mp->rootplay = 5;
    _mp->optimize = 0;
    _mp->nops = 0;
    _mp->gunmode = 0;
    _mp->gun_ports = 0;
    _mp->gun_port = 0;
    _mp->tag_port = TAG_PORT;
    _mp->tag_dev = TAG_DEV;
    _mp->tag_info = TAG_INFO;
    _mp->tag_enbl = TAG_ENBL;
    _mp->tag_stat = TAG_STAT;
    _mp->tag_radr = TAG_RADR;
    _mp->tag_tout = TAG_TOUT;
    _mp->tag_drqc = TAG_DRQC;
    _mp->tag_func = TAG_FUNC;
    _mp->tag_end = TAG_END;
    mpdrv_swapmode(0);
    set_registers();
}

Sint32 mpdrv_insertnop(Sint32 count)
{
    Sint32 i;
    Uint32 *word;

    _mp->last_cmd += _mp->last_cmd_words;
    word = _mp->last_cmd;
    for (i = 0; i < count; i++) {
        *word++ = PATTERN_NOP;
    }
    _mp->last_cmd_words = count;
    _mp->send_size += count * 4;
    return 0;
}

Sint32 mpdrv_setcmd(Sint32 device, Uint8 command, const void *data,
                    Sint32 count, const void *extra, Sint32 extra_count)
{
    Sint32 i;
    Sint32 total;
    Sint32 length;
    Sint32 device_port;
    Sint32 last_port;
    Uint32 words;
    Uint8 address;
    Uint8 origin;
    Uint32 *word;
    const Sint32 *from;
    const char *bytes;
    struct mp_context *context;

    if (_mp->device[device].queued) {
        return -1;
    }
    _mp->device[device].queued = command;
    total = count + extra_count;
    words = total + COMMAND_HEAD;
    /* @hack spelling: length, a copy of words, which keeps device in R9 and
     * the 44-byte device stride in R10 (0x8c124cc6, 0x8c124cd2); without it,
     * the two swap and 62 of 291 words differ */
    length = words;
    address = _mp_Per2APTable[device];
    origin = address & ADDRESS_PORT;
    device_port = origin >> 6 & 3;
    /* @hack spelling: _mp through context, which keeps the pointer for
     * last_cmd_words and nopcount; through _mp itself, SHC reads it again,
     * and 243 of 291 words differ */
    context = _mp;
    if (_mp->nopcount > 0 && context->last_cmd_words > 0) {
        /* @hack type: the header's high half as Sint16, which SHC
         * sign-extends before the mask (EXTS.W R4,R4 at 0x8c124d48 in
         * 1ST_READ.BIN); without the cast, 81 of 291 words differ in each
         * image */
        /* @hack spelling: last_port through a local, which holds it in R4;
         * compared in place, it loads into R3, and 37 of 291 words differ */
        last_port = (Sint16)(*context->last_cmd >> 16) & 3;
        if (device_port == last_port) {
            mpdrv_insertnop(context->nopcount);
        }
    }
    _mp->last_cmd += _mp->last_cmd_words;
    word = _mp->last_cmd;
    *word++ = device_port << 16 | (Uint8)total;
    *word++ = (Uint32)mpdrv_v2p(_mp->recv_data[_mp->recv_side][device]);
    from = data;
    if (_mp->swapmode == 1) {
        *word++ = PACK_WORD(command, address, origin, total);
        for (i = 0; i < count; i++) {
            *word++ = *from++;
        }
    } else {
        *word++ = PACK_WORD(total, origin, address, command);
        for (i = 0; i < count; i++) {
            /* @hack spelling: word stepped in a statement of its own; as
             * *word++ = SWAP_WORD(*from), the function is 4 bytes longer and
             * 144 of 291 words differ */
            *word = SWAP_WORD(*from);
            word++;
            from++;
        }
    }
    if (_mp->swapmode == 1) {
        bytes = extra;
        for (i = 0; i < extra_count; i++) {
            *word++ = PACK_WORD(bytes[0], bytes[1], bytes[2], bytes[3]);
            bytes += 4;
        }
    } else {
        from = extra;
        for (i = 0; i < extra_count; i++) {
            *word++ = *from++;
        }
    }
    _mp->last_cmd_words = length;
    _mp->send_size += length * 4;
    _mp->device[device].queued_seq = _mp->next_seq++;
    _mp->next_seq &= 0x7fffffff;
    if (_mp->next_seq == 0) {
        _mp->next_seq = 1;
    }
    return _mp->device[device].queued_seq;
}

Sint32 mpdrv_hreset(Sint32 port)
{
    /* @hack spelling: the command's address through word, which holds it in
     * R6 (0x8c124f46); through _mp->last_cmd itself it takes R2, and 6 of 29
     * words differ */
    Uint32 *word;

    _mp->last_cmd += _mp->last_cmd_words;
    word = _mp->last_cmd;
    *word = port << 16 | PATTERN_RESET;
    _mp->last_cmd_words = 1;
    _mp->send_size += 4;
    return 0;
}

Sint32 mpdrv_sdckbenable(Sint32 port)
{
    /* @hack spelling: the command's address through word, which holds it in
     * R6 (0x8c124f92); through _mp->last_cmd itself it takes R2, and 3 of 29
     * words differ */
    Uint32 *word;

    _mp->last_cmd += _mp->last_cmd_words;
    word = _mp->last_cmd;
    *word = port << 16 | PATTERN_GUN_ON;
    _mp->last_cmd_words = 1;
    _mp->send_size += 4;
    return 0;
}

Sint32 mpdrv_sdckbdisable(Sint32 port)
{
    /* @hack spelling: the command's address through word, which holds it in
     * R6 (0x8c124fce); through _mp->last_cmd itself it takes R2, and 6 of 29
     * words differ */
    Uint32 *word;

    _mp->last_cmd += _mp->last_cmd_words;
    word = _mp->last_cmd;
    *word = port << 16 | PATTERN_GUN_OFF;
    _mp->last_cmd_words = 1;
    _mp->send_size += 4;
    return 0;
}

Sint32 mpdrv_check_port_dmy(Sint32 port)
{
    KDS_PORT *state = &_mp->port[port];

    state->lmold = state->lmon;
    state->lmon = 0;
    state->lmcon = (state->lmold ^ state->lmon) & state->lmon;
    state->lmremv = ~state->lmon & (state->lmold ^ state->lmon);
    return 0;
}

Sint32 mpdrv_check_port_gun(Sint32 port)
{
    KDS_PORT *state = &_mp->port[port];
    Uint32 *head;
    Uint32 response;
    Sint32 result;
    /* the source address in the root device's response header, in which the
     * root device sets the bit of each expansion socket in use */
    Sint32 presence;
    char *bytes;

    head = _mp->recv_data[1 - _mp->recv_side][_mp_RootDeviceNo[port]];
    response = *head;
    switch (response) {
    case 0xffffffff:
    case 0xffffff00:
        state->playcount++;
        if (state->playcount > _mp->rootplay) {
            presence = 0;
            result = -1;
        } else {
            return 0;
        }
        break;
    default:
        /* @hack spelling: a byte cursor, which each branch of the if steps to
         * its byte. as ((char *)&response)[1] and [2], the else branch loads
         * through MOV.B @(2,R6),R0, presence and result trade R4 and R6, and
         * 42 of 92 words differ */
        /* @hack spelling: result cleared in each branch; cleared after the if,
         * its zero is copied to R3, not R2, and 1 of 92 words differ */
        if (_mp->swapmode == 1) {
            bytes = (char *)&response;
            bytes++;
            presence = *bytes; /* NOLINT(bugprone-signed-char-misuse) */
            /* @hack redundant: a step past the byte, which keeps each
             * branch's load of it; without it, the branches share one load
             * after the if, the function loses that load and its pool's .RES.W
             * pad, 4 bytes, and 41 of 92 words differ */
            bytes++;
            state->playcount = result = 0;
        } else {
            bytes = (char *)&response;
            bytes += 2;
            presence = *bytes; /* NOLINT(bugprone-signed-char-misuse) */
            state->playcount = result = 0;
        }
        break;
    }
    state->lmold = state->lmon;
    state->lmon = presence & (ADDRESS_PORT | ADDRESS_ROOT);
    state->lmcon = (state->lmold ^ state->lmon) & state->lmon;
    state->lmremv = ~state->lmon & (state->lmold ^ state->lmon);
    return result;
}

Sint32 mpdrv_check_port(Sint32 port)
{
    KDS_PORT *state = &_mp->port[port];
    Uint32 *head;
    Uint32 response;
    Sint32 result;
    /* the source address in the root device's response header, in which the
     * root device sets the bit of each expansion socket in use */
    Sint32 presence;
    char *bytes;

    head = _mp->recv_data[1 - _mp->recv_side][_mp_RootDeviceNo[port]];
    response = *head;
    switch (response) {
    case 0xffffffff:
    case 0xffffff00:
        state->playcount++;
        if (state->playcount > _mp->rootplay) {
            presence = 0;
            result = -1;
        } else {
            return 0;
        }
        break;
    default:
        /* @hack spelling: a byte cursor, which each branch of the if steps to
         * its byte. as ((char *)&response)[1] and [2], the else branch loads
         * through MOV.B @(2,R6),R0, presence and result trade R4 and R6, and
         * 40 of 90 words differ */
        /* @hack spelling: result cleared in each branch; cleared after the if,
         * its zero is copied to R3, not R2, and 1 of 90 words differ */
        if (_mp->swapmode == 1) {
            bytes = (char *)&response;
            bytes++;
            presence = *bytes; /* NOLINT(bugprone-signed-char-misuse) */
            /* @hack redundant: a step past the byte, which keeps each
             * branch's load of it; without it, the branches share one load
             * after the if, the function loses that load and its pool's .RES.W
             * pad, 4 bytes, and 39 of 90 words differ */
            bytes++;
            state->playcount = result = 0;
        } else {
            bytes = (char *)&response;
            bytes += 2;
            presence = *bytes; /* NOLINT(bugprone-signed-char-misuse) */
            state->playcount = result = 0;
        }
        break;
    }
    state->lmold = state->lmon;
    state->lmon = presence;
    state->lmcon = (state->lmold ^ state->lmon) & state->lmon;
    state->lmremv = ~state->lmon & (state->lmold ^ state->lmon);
    return result;
}

Sint32 mpdrv_make_devicedata(Sint32 device, Sint32 port)
{
    KDS_PORT *state = &_mp->port[port];
    struct maple_device *dev = &_mp->device[device];
    KDS_DEVICEINFO *info = &_mp->info[device];
    Uint32 *response = _mp->recv_data[1 - _mp->recv_side][device];
    Uint8 socket_bit;
    Sint32 result;

    dev->old = dev->on;
    dev->old_command = dev->command;
    if (dev->sent_seq) {
        dev->done_seq = dev->sent_seq;
        dev->sent_seq = 0;
    }
    socket_bit = _mp_Per2APTable[device] & ADDRESS_SOCKETS;
    if (_mp->gun_ports == 0) {
        if (socket_bit) {
            result = update_expansion(socket_bit, state, info, dev, response);
        } else {
            result = update_root(port, info, dev, response);
        }
    } else {
        if (socket_bit) {
            result =
                update_gun_expansion(socket_bit, state, info, dev, response);
        } else {
            result = update_gun_root(port, info, dev, response);
        }
    }
    if (dev->sent != 0) {
        dev->recvdata = _mp->recv_data[1 - _mp->recv_side][device];
    }
    if (dev->recognized) {
        dev->changed = 0;
        dev->recognized = 0;
    }
    dev->changed |= dev->connect;
    dev->changed |= dev->remove;
    return result;
}

static Sint32 decide_presence(struct maple_device *dev, Uint32 *response)
{
    if (dev->sent == MPDRV_CMD_DREQ) {
        if (dev->response == KDD_RES_DEVICESTATUS) {
            dev->on = 1;
        } else {
            dev->on = 0;
        }
    } else {
        switch (dev->response) {
        /* @hack spelling: a body and break of its own for 0x00; sharing the
         * next case's, SHC tests 0xff with BT and adds a BRA to the default,
         * the function is 4 bytes longer and 10 of 26 words differ */
        case 0x00:
            dev->on = 0;
            break;
        case KDD_RES_NORESPONSE:
            dev->on = 0;
            break;
        default:
            dev->on = 1;
            break;
        }
    }
    return 0;
}

static Sint32 clear_response(struct maple_device *dev, Uint32 *response)
{
    dev->response = dev->dst_ap = dev->src_ap = dev->datasize = 0xff;
    dev->errcode = -1;
    return 0;
}

static Sint32 read_response(struct maple_device *dev, Uint32 *response)
{
    Uint32 head = *response++;
    Uint8 *bytes;

    if (_mp->swapmode == 1) {
        bytes = (Uint8 *)&head;
        dev->datasize = *bytes++;
        dev->src_ap = *bytes++;
        dev->dst_ap = *bytes++;
        dev->response = *bytes;
    } else {
        bytes = (Uint8 *)&head;
        dev->response = *bytes++;
        dev->dst_ap = *bytes++;
        dev->src_ap = *bytes++;
        dev->datasize = *bytes;
    }
    if (dev->response == KDD_RES_NORESPONSE) {
        dev->errcode = -1;
    } else if (dev->response < 0xe0) {
        dev->errcode = 0;
    } else {
        dev->errcode = *response;
    }
    return 0;
}

static Sint32 update_gun_root(Sint32 port, KDS_DEVICEINFO *info,
                              struct maple_device *dev, Uint32 *response)
{
    if (_mp->gun_port != port) {
        return 0;
    }
    if (_mp->gun_ports & 1 << port) {
        read_response(dev, response);
    } else {
        clear_response(dev, response);
    }
    decide_presence(dev, response);
    dev->connect = (dev->old ^ dev->on) & dev->on;
    dev->remove = ~dev->on & (dev->old ^ dev->on);
    if (_mp->swapmode == 1) {
        if (dev->response != KDD_RES_NORESPONSE && dev->response != 0) {
            mpdrv_swaplong(response, (Uint8)*response + 1);
        }
    }
    if (dev->on) {
        dev->command = MPDRV_CMD_GETCOND;
    } else {
        dev->command = MPDRV_CMD_DREQ;
    }
    /* @hack redundant: a test of remove with no effect, which keeps the image's
     * read and test of it (MOV.B @(3,R14),R0 at 0x8c125442 in 1ST_READ.BIN);
     * without it, 27 of 94 words differ */
    if (dev->remove) {
    }
    if (dev->connect) {
        info->is_root = 1;
        read_info(info, dev, response);
    }
    if (dev->remove) {
        clear_info(info);
    }
    return 0;
}

static Sint32 update_root(Sint32 port, KDS_DEVICEINFO *info,
                          struct maple_device *dev, Uint32 *response)
{
    read_response(dev, response);
    decide_presence(dev, response);
    dev->connect = (dev->old ^ dev->on) & dev->on;
    dev->remove = ~dev->on & (dev->old ^ dev->on);
    if (_mp->swapmode == 1) {
        if (dev->response != KDD_RES_NORESPONSE && dev->response != 0) {
            mpdrv_swaplong(response, (Uint8)*response + 1);
        }
    }
    if (dev->on) {
        dev->command = MPDRV_CMD_GETCOND;
    } else {
        dev->command = MPDRV_CMD_DREQ;
    }
    /* @hack redundant: a test of remove with no effect, which keeps the image's
     * read and test of it (MOV.B @(3,R14),R0 at 0x8c1254e6 in 1ST_READ.BIN);
     * without it, 21 of 75 words differ */
    if (dev->remove) {
    }
    if (dev->connect) {
        info->is_root = 1;
        read_info(info, dev, response);
    }
    if (dev->remove) {
        clear_info(info);
    }
    return 0;
}

static Sint32 update_gun_expansion(Uint8 socket_bit, KDS_PORT *state,
                                   KDS_DEVICEINFO *info,
                                   struct maple_device *dev, Uint32 *response)
{
    clear_response(dev, response);
    dev->remove = dev->connect = 0;
    if (state->lmremv & socket_bit) {
        /* @hack redundant: MPDRV_CMD_DKILL, which the store of
         * MPDRV_CMD_ABSENT below replaces (0x8c125546, 0x8c125552 in
         * 1ST_READ.BIN); without it, the function runs 4 bytes short and 23 of
         * 47 words differ in each image */
        dev->command = MPDRV_CMD_DKILL;
        dev->on = 0;
        dev->remove = 1;
        dev->retries = 0;
    }
    dev->command = MPDRV_CMD_ABSENT;
    dev->on = 0;
    if (dev->remove) {
        info->is_root = 0;
        clear_info(info);
    }
    return 0;
}

static Sint32 update_expansion(Uint8 socket_bit, KDS_PORT *state,
                               KDS_DEVICEINFO *info, struct maple_device *dev,
                               Uint32 *response)
{
    Uint32 head;

    read_response(dev, response);
    dev->remove = dev->connect = 0;
    if (state->lmcon & socket_bit) {
        dev->command = MPDRV_CMD_DREQ;
        dev->on = 0;
        dev->retries++;
    }
    if (state->lmremv & socket_bit) {
        dev->command = MPDRV_CMD_DKILL;
        dev->on = 0;
        dev->remove = 1;
        dev->retries = 0;
    } else {
        /* @hack redundant: an empty else, which has SHC load state->lmon below
         * into R2 (MOV.B @R10,R2 at 0x8c1255cc); without it, SHC loads
         * state->lmon into R3, and 3 of 158 words differ */
    }
    if (state->lmon & socket_bit) {
        if (dev->sent == MPDRV_CMD_NONE) {
            if (dev->on) {
                dev->command = MPDRV_CMD_NONE;
            }
        } else if (dev->sent == MPDRV_CMD_DREQ) {
            if (dev->response == KDD_RES_DEVICESTATUS) {
                dev->connect = 1;
                dev->on = 1;
                dev->command = MPDRV_CMD_NONE;
            } else {
                dev->retries++;
                if (dev->retries > _mp->dreqcount) {
                    dev->on = 0;
                    dev->command = MPDRV_CMD_ABSENT;
                    dev->retries = 0;
                } else {
                    dev->command = MPDRV_CMD_DREQ;
                }
            }
        } else if (dev->sent == MPDRV_CMD_DKILL) {
            dev->remove = 1;
            dev->on = 0;
            dev->command = MPDRV_CMD_ABSENT;
            dev->retries = 0;
        } else if (dev->sent) {
            dev->command = MPDRV_CMD_NONE;
        }
    } else {
        dev->command = MPDRV_CMD_ABSENT;
        dev->on = 0;
    }
    if (dev->on && dev->sent) {
        /* @hack spelling: the byte order's test on its own; in the &&, SHC
         * moves head's copy to R4 into the BT's delay slot, and 2 of 158
         * words differ */
        if (_mp->swapmode == 1) {
            head = *response;
            if (head != 0xffffffff && head != 0xffffff00) {
                mpdrv_swaplong(response, (Uint8)*response + 1);
            }
        }
    }
    if (dev->connect) {
        info->is_root = 0;
        read_info(info, dev, response);
    }
    if (dev->remove) {
        info->is_root = 0;
        clear_info(info);
    }
    return 0;
}

static Sint32 read_info(KDS_DEVICEINFO *info, struct maple_device *dev,
                        Uint32 *response)
{
    Sint32 i;
    Uint8 *from;
    /* @hack spelling: one zero, in R7, for every terminator; with a 0 at
     * each store, the frame loses R13's save, the loops take other
     * registers, the function is 4 bytes shorter and 145 of 160 words
     * differ */
    char zero = 0;
    Uint8 *to;

    response++;
    from = (Uint8 *)response;
    info->function = PACK_WORD(from[0], from[1], from[2], from[3]);
    from += 4;
    /* NOLINTBEGIN(readability-redundant-control-flow) */
    /* @hack spelling: the counter set ahead of the for; set in it, the counter
     * moves into the entry branch's delay slot, and 3 of 160 words differ */
    i = 0;
    to = info->fd[0];
    for (; i < 4; i++) {
        /* @hack spelling: to and from stepped on their own; as
         * *to++ = *from++, the loop's counter and pointer trade R13 and R14,
         * its byte and the later fd loops' take R2, not R3, and 12 of 160
         * words differ */
        *to = *from;
        to++;
        from++;
        /* @hack redundant: a continue at the loop's end; without it, the next
         * two loops' byte takes R2, not R3, and 4 of 160 words differ */
        continue;
    }
    /* @hack spelling: the counter set ahead of the for; set in it, the counter
     * moves into the entry branch's delay slot, and 3 of 160 words differ */
    i = 0;
    to = info->fd[1];
    for (; i < 4; i++) {
        /* @hack spelling: to and from stepped on their own; as
         * *to++ = *from++, 10 of 160 words differ */
        *to = *from;
        to++;
        from++;
        /* @hack redundant: a continue at the loop's end; without it, the next
         * loop's byte takes R2, not R3, and 2 of 160 words differ */
        continue;
    }
    /* NOLINTEND(readability-redundant-control-flow) */
    /* @hack spelling: the counter set ahead of the for; set in it, the loop
     * counts down, and 7 of 160 words differ */
    i = 0;
    to = info->fd[2];
    for (; i < 4; i++) {
        /* @hack spelling: to and from stepped on their own; as
         * *to++ = *from++, 8 of 160 words differ */
        *to = *from;
        to++;
        from++;
    }
    info->area_code = *from++;
    if (info->is_root) {
        info->connector_dir[0] = *from & 3;
        info->connector_dir[1] = *from >> 2 & 3;
    } else {
        info->connector_dir[0] = expansion_direction[*from & 15];
        /* @hack type: a char store; to the Uint8 field, SHC loads a zero of
         * its own into R2 rather than storing R7, and 65 of 160 words differ */
        ((char *)info->connector_dir)[1] = zero;
    }
    if (_mp->swapmode == 1) {
        /* @hack spelling: the counter set ahead of the for; set in it, the loop
         * counts down, the function is 2 bytes shorter and 82 of 160 words
         * differ */
        i = 0;
        to = (Uint8 *)info->product_name;
        for (; i < 31; i++) {
            *to = *from;
            /* @hack spelling: the step spelled to = to + 1; as to++, fd[2]'s
             * counter and pointer trade R13 and R14, and 7 of 160 words
             * differ */
            to = to + 1;
            from++;
        }
    } else {
        from++;
        /* @hack spelling: the counter set ahead of the for; set in it, the loop
         * counts down, the function is 2 bytes shorter and 62 of 160 words
         * differ */
        i = 0;
        to = (Uint8 *)info->product_name;
        for (; i < 30; i++) {
            *to = *from;
            /* @hack spelling: the step spelled to = to + 1; as to++, fd[1]'s
             * and fd[2]'s counters and pointers trade R13 and R14, this loop's
             * R14 and R6, and 21 of 160 words differ */
            to = to + 1;
            from++;
        }
        info->product_name[30] = zero;
    }
    info->product_name[31] = zero;
    /* @hack redundant: a goto to the next label; without it, SHC tests the
     * fd[2] loop and the loops after it at their ends, keeps in R0 what R13
     * holds and saves no R13, the function is 10 bytes shorter, and 130 of 160
     * words differ */
    goto license;
license:
    /* @hack spelling: the counter set ahead of the for; set in it, the loop
     * counts down, the function is 2 bytes shorter and 47 of 160 words
     * differ */
    i = 0;
    to = (Uint8 *)info->license;
    for (; i < 60; i++) {
        *to = *from;
        /* @hack spelling: the step spelled to = to + 1; as to++, fd[1]'s and
         * fd[2]'s counters and pointers trade R13 and R14, and 14 of 160 words
         * differ */
        to = to + 1;
        from++;
    }
    info->license[60] = zero;
    info->stdby_pow = from[0] | from[1] << 8;
    info->max_pow = from[2] | from[3] << 8;
    return 0;
}

static Sint32 clear_info(KDS_DEVICEINFO *info)
{
    Sint32 i;

    fill_words(info, 0, WORDS(*info));
    info->product_name[0] = '(';
    info->product_name[1] = 'n';
    info->product_name[2] = 'o';
    info->product_name[3] = 'n';
    info->product_name[4] = 'e';
    info->product_name[5] = ')';
    for (i = 6; i < 31; i++) {
        info->product_name[i] = ' ';
    }
    info->product_name[31] = 0;
    return 0;
}

/* defined below mpdrv_init_globals: SHC writes constants to section C in
 * source order, and the image holds port_bits' initializer, which
 * mpdrv_init_globals copies, ahead of this string */
static const char semaphore_error[] = "***FATAL ERROR:Cannot get semaphore!\n";

Sint32 mpdrv_set_getcond(Sint32 device, Sint32 func)
{
    Sint32 result;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(semaphore_error);
        return -2;
    }
    result = mpdrv_set_getcond_ns(device, func);
    *_mp_SemaphoreAddress = 0;
    return result;
}

Sint32 mpdrv_set_getcond_ns(Sint32 device, Sint32 func)
{
    /* @hack spelling: func through a copy; with &func, the zero arguments
     * take R2, not R3, and 3 of 15 words differ */
    Sint32 word = func;

    return mpdrv_setcmd(device, MPDRV_CMD_GETCOND, &word, 1, NULL, 0);
}

Sint32 mpdrv_set_dreq(Sint32 device)
{
    Sint32 result;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(semaphore_error);
        return -2;
    }
    result = mpdrv_set_dreq_ns(device);
    *_mp_SemaphoreAddress = 0;
    return result;
}

Sint32 mpdrv_set_dreq_ns(Sint32 device)
{
    return mpdrv_setcmd(device, MPDRV_CMD_DREQ, NULL, 0, NULL, 0);
}

Sint32 mpdrv_set_dkill(Sint32 device)
{
    Sint32 result;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(semaphore_error);
        return -2;
    }
    result = mpdrv_set_dkill_ns(device);
    *_mp_SemaphoreAddress = 0;
    return result;
}

Sint32 mpdrv_set_dkill_ns(Sint32 device)
{
    return mpdrv_setcmd(device, MPDRV_CMD_DKILL, NULL, 0, NULL, 0);
}

Sint32 mpdrv_set_dreset(Sint32 device)
{
    Sint32 result;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(semaphore_error);
        return -2;
    }
    result = mpdrv_set_dreset_ns(device);
    *_mp_SemaphoreAddress = 0;
    return result;
}

Sint32 mpdrv_set_dreset_ns(Sint32 device)
{
    return mpdrv_setcmd(device, MPDRV_CMD_DRESET, NULL, 0, NULL, 0);
}

Sint32 mpdrv_set_allstatreq(Sint32 device)
{
    Sint32 result;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(semaphore_error);
        return -2;
    }
    result = mpdrv_set_allstatreq_ns(device);
    *_mp_SemaphoreAddress = 0;
    return result;
}

Sint32 mpdrv_set_allstatreq_ns(Sint32 device)
{
    return mpdrv_setcmd(device, MPDRV_CMD_ALLSTATREQ, NULL, 0, NULL, 0);
}

Sint32 mpdrv_set_getminfo(Sint32 device, Sint32 func, Sint32 pt)
{
    Sint32 result;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(semaphore_error);
        return -2;
    }
    result = mpdrv_set_getminfo_ns(device, func, pt);
    *_mp_SemaphoreAddress = 0;
    return result;
}

Sint32 mpdrv_set_getminfo_ns(Sint32 device, Sint32 func, Sint32 pt)
{
    Sint32 words[2];

    words[0] = func;
    words[1] = pt << 24;
    return mpdrv_setcmd(device, MPDRV_CMD_GETMINFO, words, 2, NULL, 0);
}

Sint32 mpdrv_set_bread(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                       Sint32 block)
{
    Sint32 result;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(semaphore_error);
        return -2;
    }
    result = mpdrv_set_bread_ns(device, func, pt, phase, block);
    *_mp_SemaphoreAddress = 0;
    return result;
}

Sint32 mpdrv_set_bread_ns(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                          Sint32 block)
{
    Sint32 words[2];

    words[0] = func;
    words[1] = pt << 24 | phase << 16 | block;
    return mpdrv_setcmd(device, MPDRV_CMD_BREAD, words, 2, NULL, 0);
}

Sint32 mpdrv_set_bwrite(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                        Sint32 block, const void *src, Sint32 size)
{
    Sint32 result;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(semaphore_error);
        return -2;
    }
    result = mpdrv_set_bwrite_ns(device, func, pt, phase, block, src, size);
    *_mp_SemaphoreAddress = 0;
    return result;
}

Sint32 mpdrv_set_bwrite_ns(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                           Sint32 block, const void *src, Sint32 size)
{
    Sint32 words[2];

    words[0] = func;
    words[1] = pt << 24 | phase << 16 | block;
    return mpdrv_setcmd(device, MPDRV_CMD_BWRITE, words, 2, src, size);
}

Sint32 mpdrv_set_getlerr(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                         Sint32 block)
{
    Sint32 result;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(semaphore_error);
        return -2;
    }
    result = mpdrv_set_getlerr_ns(device, func, pt, phase, block);
    *_mp_SemaphoreAddress = 0;
    return result;
}

Sint32 mpdrv_set_getlerr_ns(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                            Sint32 block)
{
    Sint32 words[2];

    words[0] = func;
    words[1] = pt << 24 | phase << 16 | block;
    return mpdrv_setcmd(device, MPDRV_CMD_GETLERR, words, 2, NULL, 0);
}

Sint32 mpdrv_set_transagain(Sint32 device)
{
    Sint32 result;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(semaphore_error);
        return -2;
    }
    result = mpdrv_set_transagain_ns(device);
    *_mp_SemaphoreAddress = 0;
    return result;
}

Sint32 mpdrv_set_transagain_ns(Sint32 device)
{
    return mpdrv_setcmd(device, MPDRV_CMD_TRANSAGAIN, NULL, 0, NULL, 0);
}

Sint32 mpdrv_set_arctrl(Sint32 device, const void *buf)
{
    Sint32 result;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(semaphore_error);
        return -2;
    }
    result = mpdrv_set_arctrl_ns(device, buf);
    *_mp_SemaphoreAddress = 0;
    return result;
}

Sint32 mpdrv_set_arctrl_ns(Sint32 device, const void *buf)
{
    Sint32 func = KDD_FUNC_ARGUN;

    return mpdrv_setcmd(device, MPDRV_CMD_ARCTRL, &func, 1, buf, 1);
}

Sint32 mpdrv_set_ft4ctrl(Sint32 device, const void *buf)
{
    Sint32 result;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(semaphore_error);
        return -2;
    }
    result = mpdrv_set_ft4ctrl_ns(device, buf);
    *_mp_SemaphoreAddress = 0;
    return result;
}

Sint32 mpdrv_set_ft4ctrl_ns(Sint32 device, const void *buf)
{
    Sint32 func = KDD_FUNC_SOUNDINPUT;

    return mpdrv_setcmd(device, MPDRV_CMD_FT4CTRL, &func, 1, buf, 1);
}

Sint32 mpdrv_set_setcond(Sint32 device, Sint32 func, const void *buf,
                         Sint32 size)
{
    Sint32 result;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(semaphore_error);
        return -2;
    }
    result = mpdrv_set_setcond_ns(device, func, buf, size);
    *_mp_SemaphoreAddress = 0;
    return result;
}

Sint32 mpdrv_set_setcond_ns(Sint32 device, Sint32 func, const void *buf,
                            Sint32 size)
{
    /* @hack spelling: func through a copy; &func spills func to the top of
     * the frame, and 9 of 18 words differ */
    Sint32 word = func;

    return mpdrv_setcmd(device, MPDRV_CMD_SETCOND, &word, 1, buf, size);
}

Sint32 mpdrv_set_hreset(Sint32 port)
{
    return mpdrv_set_hreset_ns(port);
}

Sint32 mpdrv_set_hreset_ns(Sint32 port)
{
    return mpdrv_hreset(port);
}

Sint32 mpdrv_set_end(void)
{
    Sint32 result;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(semaphore_error);
        return -2;
    }
    result = mpdrv_set_end_ns();
    *_mp_SemaphoreAddress = 0;
    return result;
}

Sint32 mpdrv_set_end_ns(void)
{
    /* @hack spelling: the command's address through word, which holds it in
     * R4; through _mp->last_cmd itself it takes R3, and 7 of 10 words differ */
    Uint32 *word = _mp->last_cmd;

    *word |= LAST_COMMAND;
    return 0;
}

/* store count words of value at record, and return count */
static Sint32 fill_words(void *record, Sint32 value, Sint32 count)
{
    Sint32 i;
    Sint32 *p;

    /* @hack spelling: p, a cursor SHC keeps in R0 and steps in the loop's delay
     * slot (ADD #4,R0 at 0x8c125ce6 in 1ST_READ.BIN); as p[i], the function
     * runs 4 bytes longer and 9 of 12 words differ in each image */
    for (i = 0, p = record; i < count; i++) {
        *p++ = value;
    }
    return count;
}

void *mpdrv_v2p(void *addr)
{
    return (void *)((Uint32)addr & 0x0fffffff);
}

void *mpdrv_p2v(void *addr)
{
    return addr;
}

static void set_registers(void)
{
    _mp->reg = MAPLE_REGISTERS;
}
