/* Shinobi G2 bus library, as far as the sources use it */
#ifndef SG_SYG2_H
#define SG_SYG2_H
#include <sg_chain.h>
#include <sg_xpt.h>

enum SYE_G2_ERR {
    SYE_G2_ERR_NOTHING = 0x00000000,
    SYE_G2_ERR_DMA_ALREADY_OPEN = 0x00090101,
    SYE_G2_ERR_DMA_BUSY = 0x00090180
};
typedef enum SYE_G2_ERR SYE_G2_ERR;

enum SYE_G2_DATA_WIDTH {
    SYE_G2_DATA_WIDTH_8BIT = 0x01,
    SYE_G2_DATA_WIDTH_16BIT = 0x02,
    SYE_G2_DATA_WIDTH_32BIT = 0x04
};
typedef enum SYE_G2_DATA_WIDTH SYE_G2_DATA_WIDTH;

/* address modes: an address stays fixed, or steps up or down by one unit after
 * each transfer */
enum SYE_G2_ADR_MODE {
    SYE_G2_ADR_MODE_FIX,
    SYE_G2_ADR_MODE_ADD,
    SYE_G2_ADR_MODE_SUB
};
typedef enum SYE_G2_ADR_MODE SYE_G2_ADR_MODE;

/* the four G2 DMA channels: the AICA's, two for the expansion port, and the
 * development channel */
enum SYE_G2_DMA_TYPE {
    SYE_G2_DMA_TYPE_AICA,
    SYE_G2_DMA_TYPE_EXT0,
    SYE_G2_DMA_TYPE_EXT1,
    SYE_G2_DMA_TYPE_DEV
};
typedef enum SYE_G2_DMA_TYPE SYE_G2_DMA_TYPE;

enum SYE_G2_DMA_DIR { SYE_G2_DMA_DIR_ROOT_TO_G2, SYE_G2_DMA_DIR_G2_TO_ROOT };
typedef enum SYE_G2_DMA_DIR SYE_G2_DMA_DIR;

/* transfer triggers: the CPU, which starts the transfer at once, or an
 * interrupt */
enum SYE_G2_DMA_TRIGGER {
    SYE_G2_DMA_TRIGGER_CPU = 0x00000000,
    SYE_G2_DMA_TRIGGER_INT = 0x00000002
};
typedef enum SYE_G2_DMA_TRIGGER SYE_G2_DMA_TRIGGER;

typedef void (*SYE_G2_CALLBACK_FUNC)(void *arg);

/* a SYG2DMA, an open channel's handle, is the address of the channel's struct
 * g2_dma_channel */
struct SYS_G2_DMA_HANDLE {
    void *m_Member; /* never read */
};
typedef struct SYS_G2_DMA_HANDLE *SYG2DMA;

SYE_G2_ERR syG2SysInit(void *prm_ptr);
SYE_G2_ERR syG2Read(void *g2_adr, void *buf_ptr, SYE_G2_DATA_WIDTH data_width,
                    Sint32 data_num, SYE_G2_ADR_MODE src_mode,
                    SYE_G2_ADR_MODE dst_mode);
SYE_G2_ERR syG2Write(void *buf_ptr, void *g2_adr, SYE_G2_DATA_WIDTH data_width,
                     Sint32 data_num, SYE_G2_ADR_MODE src_mode,
                     SYE_G2_ADR_MODE dst_mode);
SYE_G2_ERR syG2AicaFifoIsEmpty(Bool *flg);
SYE_G2_ERR syG2FifoIsEmpty(Bool *flg);
SYE_G2_ERR syG2AicaFifoWaitEmpty(void);
SYE_G2_ERR syG2FifoWaitEmpty(void);
SYE_G2_ERR syG2DmaOpen(SYG2DMA *handle, SYE_G2_DMA_TYPE type);
SYE_G2_ERR syG2DmaSetPrm(SYG2DMA handle, void *g2_adr, void *root_bus_adr,
                         Sint32 sz, SYE_G2_DMA_DIR dir, Bool transfer_ctl,
                         Bool suspend_mode, SYE_G2_CALLBACK_FUNC callback,
                         void *callback_1st_arg);
/* write the operand cache back over the transfer's memory, then start the
 * transfer syG2DmaSetPrm set up. the result is SYE_G2_ERR_NOTHING, or
 * SYE_G2_ERR_DMA_BUSY, with nothing done, while a transfer runs on the
 * channel */
SYE_G2_ERR syG2DmaTrigger(SYG2DMA handle, SYE_G2_DMA_TRIGGER triger_mode);
/* call syG2DmaTrigger, under the misspelled name Triger, which the library
 * exports beside the right one */
SYE_G2_ERR syG2DmaTriger(SYG2DMA handle, SYE_G2_DMA_TRIGGER triger_mode);
/* suspend the channel's transfer, if one runs. the result is
 * SYE_G2_ERR_NOTHING */
SYE_G2_ERR syG2DmaSuspend(SYG2DMA handle);
SYE_G2_ERR syG2DmaSuspendAll(void);
SYE_G2_ERR syG2DmaResume(SYG2DMA handle);
SYE_G2_ERR syG2DmaResumeAll(void);
SYE_G2_ERR syG2DmaIsBusy(SYG2DMA handle, Bool *flg);
/* *counter gets the channel's end count, end_counts[type], not its trigger
 * count. the result is SYE_G2_ERR_NOTHING */
SYE_G2_ERR syG2DmaGetTriggerCounter(SYE_G2_DMA_TYPE type, Uint32 *counter);
SYE_G2_ERR syG2DmaGetEndCounter(SYE_G2_DMA_TYPE type, Uint32 *counter);

#define G2_DMA_CHANNELS 4

/* a channel's G2 DMA registers, in the system bus's register area */
struct g2_dma_registers {
    Uint32 device;
    Uint32 memory;
    Uint32 length;
    Uint32 direction;
    Uint32 trigger;
    Uint32 enable;
    Uint32 start;
    Uint32 suspend;
};

/* @hack type: G2_DMA_CHANNEL_REGISTERS casts sygG2WrkPtr->registers, a
 * volatile pointer, to a plain pointer to the banks, and takes channel type's
 * registers as the address of their device field. the view sets the order of
 * two functions' loads. syG2DmaSuspend loads the 2 it stores in
 * unk_1c before the work pointer (MOV #2,R1 at 0x8c126efa, and at 0x8ce03ebe
 * in LOADER.BIN). syG2DmaTrigger loads syG2DmaIsBusy's address just before
 * the call (0x8c126f48), and the 1 and the work pointer's address before the
 * trigger store (0x8c126f86, 0x8c126f88). as &sygG2WrkPtr->registers[type],
 * 6 of syG2DmaSuspend's 27 words differ and 11 of syG2DmaTrigger's 71. as the
 * bank's own address, 4 and 10 differ, and through a volatile pointer, 9 and
 * 9 differ */
struct g2_dma_register_banks {
    struct g2_dma_registers channels[G2_DMA_CHANNELS];
};
#define G2_DMA_CHANNEL_REGISTERS(type)                                         \
    ((struct g2_dma_registers *)&(                                             \
         (struct g2_dma_register_banks *)sygG2WrkPtr->registers)               \
         ->channels[type]                                                      \
         .device)

struct g2_dma_channel {
    SYE_G2_DMA_TYPE type;
    Uint8 unk_04[4];
    Uint32 g2_address;
    Uint32 root_bus_address;
    Uint32 length;
    Uint32 direction;
    Uint32 flags;
    /* 3 after syG2DmaReset and syG2DmaOpen, and 2 once syG2DmaSuspend suspends
     * a transfer. nothing reads it */
    Sint32 unk_1c;
    SYE_G2_CALLBACK_FUNC callback;
    void *callback_arg;
    SYCHAIN chain; /* the handler of the channel's DMA end interrupt */
    Bool open;
    Bool busy;
    /* set by syG2DmaEndIntrruptHandler, and cleared by syG2DmaSetPrm,
     * syG2DmaOpen and syG2DmaReset. nothing reads it */
    Uint32 ended;
};
/* bit 31 of g2_dma_channel.length, which syG2DmaSetPrm sets for every
 * transfer */
#define LENGTH_END 0x80000000UL

struct g2_work {
    volatile struct g2_dma_registers *registers;
    struct g2_dma_channel channels[G2_DMA_CHANNELS];
    /* whether syG2Read and syG2Write mask every interrupt; syG2SysInit sets
     * it */
    Bool mask_interrupts;
    Uint32 unk_e8;
    Uint32 trigger_counts[G2_DMA_CHANNELS];
    Uint32 end_counts[G2_DMA_CHANNELS];
    Uint32 unk_10c[G2_DMA_CHANNELS];
    /* the times syG2FifoWaitEmpty and syG2AicaFifoWaitEmpty have found their
     * FIFO empty */
    Uint32 fifo_waits;
    Uint32 aica_fifo_waits;
};

/* the work, which syG2SysInit places at the first 32-byte boundary in
 * sygG2WrkImg */
extern struct g2_work *sygG2WrkPtr;
extern Uint8 sygG2WrkImg[sizeof(struct g2_work) + 32];
/* set by syG2DmaSuspendAll, and cleared by syG2DmaResumeAll and syG2SysInit */
extern Uint32 sygG2DmaSuspendAll;
/* the last value written to the G2 DMA protection register */
extern Uint32 syG2DmaProtect;

/* reset a channel's state and disable its DMA */
SYE_G2_ERR syG2DmaReset(struct g2_dma_channel *channel);
/* the chain handler of a channel's DMA end interrupt, with the channel as arg.
 * it disables the channel's DMA, marks the channel idle and ended, and counts
 * the end. it then clears the interrupt and calls the channel's callback, if
 * one is set */
void syG2DmaEndIntrruptHandler(void *arg);
/* allow G2 DMA from the megabyte of main memory that holds first through the
 * one that holds last */
SYE_G2_ERR syG2DmaSetProtect(void *first, void *last);

#endif
