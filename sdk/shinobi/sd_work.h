/* sound library records and internal calls */
#ifndef SD_WORK_H
#define SD_WORK_H
#include <sg_syg2.h>
#include <sg_xpt.h>
#include <shinobi.h>

/* port types, in sd_port.type */
#define SD_PORT_MIDI 1
#define SD_PORT_SHOT 2
#define SD_PORT_PSTM 3
#define SD_PORT_SYSTEM 9

/* a port, which a MIDI, one-shot or stream handle points to */
struct sd_port {
    Uint8 type;
    Sint8 number; /* the index among the ports of its type */
    Sint8 delay_time; /* sdSetHostCmdDelayTime's time, which nothing reads */
    Sint32 used;
};

#define SD_PORT_MAX                                                            \
    (1 + SDD_MIDI_PORT_MAX + SDD_SHOT_PORT_MAX + SDD_PSTM_PORT_MAX)

union sd_ports {
    struct sd_port all[SD_PORT_MAX];
    struct {
        struct sd_port system;
        struct sd_port midi[SDD_MIDI_PORT_MAX];
        struct sd_port shot[SDD_SHOT_PORT_MAX];
        struct sd_port pstm[SDD_PSTM_PORT_MAX];
    } kind;
};

struct sd_stream {
    SDE_PCM_TYPE pcm_type;
    Sint8 ch_num; /* channels the port opened with */
    Sint8 use_ch; /* channels in use */
    Sint8 base_vol; /* sdPstmSetBaseVol's volume as sdPstmPlay sends it */
    Sint8 ch[SDD_PSTM_PORT_CH_MAX]; /* each channel's driver channel */
    Sint8 unk_09[3];
    Sint32 stopped[SDD_PSTM_PORT_CH_MAX];
    Sint32 unk_14[SDD_PSTM_PORT_CH_MAX];
    /* each channel's pan as sdPstmSetPan last took it, which sdPstmSetUseCh
     * sends again */
    Sint32 pan[SDD_PSTM_PORT_CH_MAX];
    Sint32 unk_24[SDD_PSTM_PORT_CH_MAX];
};

/* the eight words of SDS_MEM_BLK, as the library uses them */
struct sd_memblk {
    Sint32 used;
    SDE_MEMBLK_STAT stat;
    void *data; /* the source */
    Sint32 size;
    SD_MEMBLK_CALLBACK_FUNC callback; /* called once the data arrives */
    void *callback_first_arg;
    Bool done; /* set once a block sent with SDD_MEMBLK_SYNC_FUNC arrives */
    Uint32 unk_1c;
};
#define SD_MEMBLK_MAX 64 /* blocks in the default table */

/* a block waiting in the DMA queue */
struct sd_memblk_queue {
    SDMEMBLK handle;
    Sint32 dst_ptr; /* its offset in sound memory */
};

struct sd_host_command {
    Uint32 command;
    Uint8 params[14];
};

/* an entry of the second host command list */
struct sd_host_command_entry {
    struct sd_host_command command;
    Bool pending; /* set while the command waits */
    Uint32 unk_18;
};

/* a primary host command list entry, in the driver's 16-byte layout */
struct sd_primary_command {
    Uint8 command;
    Uint8 unk_01;
    Uint8 params[14];
};
#define SD_PRIMARY_COMMAND_MAX 32

/* the two host command lists. sdSetHostCmd queues commands on the second
 * list, and sd2ndHostCmdSendList moves up to SD_PRIMARY_COMMAND_MAX of them
 * to the primary list, which goes to the driver */
struct sd_host_commands {
    struct sd_primary_command *primary;
    Sint32 primary_num;
    /* set from the primary list's transfer until the driver is signaled */
    Bool busy;
    struct sd_host_command_entry *second; /* a ring of second_max entries */
    Sint32 second_max;
    Sint32 second_num;
    Sint32 second_write;
    Sint32 second_read;
};

/* a bank's offset and size in sound memory */
struct sd_bank {
    Sint32 address;
    Sint32 size;
};
#define SD_BANK_MAX 16

/* the bank table, kept in the work and in the driver's data map in sound
 * memory */
struct sd_banks {
    struct sd_bank midi_seq[SD_BANK_MAX];
    struct sd_bank midi_prg[SD_BANK_MAX];
    struct sd_bank shot[SD_BANK_MAX];
    struct sd_bank pstm_ring[SD_BANK_MAX];
    struct sd_bank fx_prg;
    Uint8 unk_208[120];
    struct sd_bank fx_out;
    struct sd_bank fx_wrk;
};

struct sd_work {
    union sd_ports ports;
    struct sd_stream streams[SDD_PSTM_PORT_MAX];
    struct sd_host_commands host;
    SDMEMBLK primary_block; /* carries the primary list */
    struct sd_memblk *memblk;
    Sint32 memblk_max;
    Sint32 memblk_num;
    Uint32 transfer_num; /* the transfers started */
    /* the transfers ended, twice over: sdMemBlkEndTransferHandller adds 2 for
     * each end */
    Uint32 transfer_end_num;
    SDMEMBLK transfer; /* the block in transfer, or NULL */
    /* the DMA queue, a ring of queue_max entries. sdMemBlkQuePush stores at
     * queue_write, and sdMemBlkQuePop takes from queue_read */
    struct sd_memblk_queue *queue;
    Sint32 queue_max;
    Sint32 queue_write;
    Sint32 queue_read;
    Sint32 queue_num;
    SDE_MEMBLK_TRANSFER_MODE transfer_mode;
    SYG2DMA dma;
    /* sound memory offsets of the driver's tables, read by sdDrvGetInfo and
     * named as it prints them */
    Sint32 system_info;
    Sint32 tool_interface;
    Sint32 sound_status;
    Sint32 status;
    Sint32 data_map;
    Sint32 host_command;
    Sint32 command_history;
    Sint32 driver; /* the driver's offset in sound memory */
    SDS_VER drv_ver;
    Uint8 unk_1d4[16];
    struct sd_banks banks;
};
/* the driver's status byte, at sound memory offset sd_work.status, while a
 * host command list waits for it. the driver clears the byte once it has taken
 * the list */
#define LIST_WAITING 1

/* the 2 MB of sound memory, at its address on the G2 bus */
#define SD_SND_MEM 0xa0800000
#define SD_SND_MEM_SIZE 0x200000
/* the AICA's ARM7 control register, on the G2 bus, and its bit that holds the
 * ARM7 in reset while set. after ARM_RESET changes, an empty loop of
 * ARM_RESET_DELAY passes gives the ARM7 time to stop or start */
#define ARM_CONTROL ((void *)0xa0702c00)
#define ARM_RESET 1
#define ARM_RESET_DELAY 0x100

/* a channel's entry in the driver's sound status */
struct sd_ch_status {
    Uint8 flg;
    Sint8 vol;
    Sint8 pan;
    Uint8 unk_03;
    Sint32 speed;
    Sint32 pitch;
    Uint8 unk_0c[4];
    Sint8 fx_lev;
    Sint8 drct_lev;
    Uint8 unk_12[2];
    Uint32 total; /* MIDI beats or stream sample frames */
    Sint32 cur_adr; /* play position; for a stream channel, in samples */
    Uint32 err;
};

/* the driver's sound status, at the sound_status offset. sound memory is
 * read only through the sdSndMemRead calls, so this struct gives offsets and
 * is never dereferenced */
struct sd_sound_status {
    struct sd_ch_status midi[SDD_MIDI_PORT_MAX];
    Uint8 unk_100[0x300];
    struct sd_ch_status pstm[SDD_PSTM_PORT_MAX][SDD_PSTM_PORT_CH_MAX];
};
/* the sound memory offset of field in the status of the MIDI port handle */
#define SD_MIDI_STATUS(work, handle, field)                                    \
    ((Sint32)(&((struct sd_sound_status *)(work)->sound_status)                \
                   ->midi[((struct sd_port *)(handle))->number]                \
                   .field))
/* the sound memory offset of field in the status of channel ch of the stream
 * port whose number is number */
#define SD_PSTM_PORT_STATUS(work, number, ch, field)                           \
    ((Sint32)(&((struct sd_sound_status *)(work)->sound_status)                \
                   ->pstm[number][ch]                                          \
                   .field))
/* the same offset, for the stream port handle */
#define SD_PSTM_STATUS(work, handle, ch, field)                                \
    SD_PSTM_PORT_STATUS(work, ((struct sd_port *)(handle))->number, ch, field)

/* the sound library work; NULL until sdLibInit */
extern struct sd_work *sdgWrkPtr;
/* the second list's entries in sdgDef2ndHostCmdBuf, and the most commands
 * sdSetHostCmd queues, whatever second_max is */
#define SD_2ND_HOST_CMD_MAX 256
/* the default buffers sdSysWrkInit sets the work up in. the work and the
 * primary list each start at the first 32-byte boundary in their buffers */
extern Uint8 sdgDefaultWrkImg[0x58c];
extern struct SDS_MEM_BLK sdgDefMemBlkBuf[SD_MEMBLK_MAX];
extern struct sd_memblk_queue sdgDefMemBlkQue[SD_MEMBLK_MAX];
extern Uint8 sdgDefPrimaryHostCmdBuf[(SD_PRIMARY_COMMAND_MAX *
                                      sizeof(struct sd_primary_command)) +
                                     32];
extern struct sd_host_command_entry sdgDef2ndHostCmdBuf[SD_2ND_HOST_CMD_MAX];
/* the G2 DMA channel to sound memory, opened by sdSysWrkInit */
extern SYG2DMA sdgG2DmaHandle;
/* the G2 bus library's result for the last transfer to sound memory */
extern SYE_G2_ERR sdgG2LibResult;

/* set the work up in the default buffers, ignoring wrk_ptr. memory blocks
 * then transfer by the CPU until sdMemBlkSetTransferMode chooses DMA, though
 * the AICA's G2 DMA channel opens here */
SDE_ERR sdSysWrkInit(void *wrk_ptr);
/* point the work at count entries of buffer, and make each an empty entry.
 * each returns SDE_ERR_NO_INIT before sdgWrkPtr is set */
SDE_ERR sdMemBlkInit(struct SDS_MEM_BLK *buffer, Sint32 count);
SDE_ERR sdMemBlkQueInit(struct sd_memblk_queue *buffer, Sint32 count);
/* the empty second-list entry */
extern struct sd_host_command_entry sdg2ndHostCmdDefault;
extern struct sd_host_command_entry *sdg2ndHostCmdBufPtr;

/* queue a copy of command on the second list. returns
 * SDE_ERR_HOST_CMD_BUF_NO_ENOUGH once SD_2ND_HOST_CMD_MAX commands wait */
SDE_ERR sdSetHostCmd(struct sd_host_command *command);
/* make count entries of buffer the empty second list */
SDE_ERR sd2ndHostCmdInitList(struct sd_host_command_entry *buffer,
                             Uint32 count);
/* move command from the second list to the primary list. returns
 * SDE_ERR_HOST_CMD_BUF_NO_ENOUGH once the primary list holds
 * SD_PRIMARY_COMMAND_MAX commands */
SDE_ERR sdPrimaryHostCmdWrite(struct sd_host_command_entry *command);
/* return SDE_ERR_PRM_OVER_RANGE, after passing it to sdSysErrTrap, for a num
 * outside 0 to 32, and otherwise SDE_ERR_NOTHING. port goes unread, and
 * nothing is stored */
SDE_ERR sdSetHostCmdRsvNum(struct sd_port *port, Sint8 num);
/* return SDE_ERR_PRM_OVER_RANGE for a time outside 0 to 16. otherwise the
 * time goes to a port that fails sdModuleCheckPort, and the result is 1. for
 * an open port, nothing is stored and the result is SDE_ERR_NOTHING. a result
 * other than SDE_ERR_NOTHING goes to sdSysErrTrap first */
SDE_ERR sdSetHostCmdDelayTime(struct sd_port *port, Sint8 time);
/* check that port is open: SDE_ERR_HANDLE_NULL for a NULL port, and
 * SDE_ERR_HANDLE_ILLEGAL_VALUE for one not open */
SDE_ERR sdModuleCheckPort(struct sd_port *port);
/* check port, then mark it free */
SDE_ERR sdModuleClosePort(struct sd_port *port);
/* open a free port of type SD_PORT_MIDI, SD_PORT_SHOT or SD_PORT_PSTM, and
 * store its handle in *handle. when every port of the type is open, *handle
 * is NULL and the result is SDE_ERR_HANDLE_NO_ENOUGH. any other type returns
 * SDE_ERR_UNKNOW_NUM. sdgWrkPtr must be set, as it isn't checked */
SDE_ERR sdModuleOpenPort(SDHANDLE **handle, Sint32 type);

#endif
