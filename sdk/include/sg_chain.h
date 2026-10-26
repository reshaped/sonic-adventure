/* Shinobi interrupt chains, as far as the sources use them */
#ifndef SG_CHAIN_H
#define SG_CHAIN_H
#include <sg_xpt.h>

/* chain events. an event below 0x0900 is a CPU exception or interrupt code. the
 * vertical and horizontal blanks follow, which the Kamui graphics library's
 * sync callbacks deliver. then come the system bus's interrupt sources, 0x20
 * apart: its normal interrupts from 0x0a00, and its external interrupts from
 * 0x0b00 */
#define SYD_CHAIN_EVENT_IML6_VBLANK 0x0900
#define SYD_CHAIN_EVENT_IML6_HBLANK 0x0920
#define SYD_CHAIN_EVENT_IML4_NRM 0x0a00
/* normal interrupts: Maple's DMA end, a vertical blank during a Maple transfer,
 * and the four G2 DMA channels' ends in SYE_G2_DMA_TYPE's order */
#define SYD_CHAIN_EVENT_IML4_MPDMAE 0x0a00
#define SYD_CHAIN_EVENT_IML4_MPVOVR 0x0a20
#define SYD_CHAIN_EVENT_IML4_AICADM 0x0a60
#define SYD_CHAIN_EVENT_IML4_EX0DMA 0x0a80
#define SYD_CHAIN_EVENT_IML4_EX1DMA 0x0aa0
#define SYD_CHAIN_EVENT_IML4_DADMAE 0x0ac0
/* GD-ROM DMA end */
#define SYD_CHAIN_EVENT_IML4_CDDMAE 0x0a40
#define SYD_CHAIN_EVENT_IML4_EXT 0x0b00
typedef Sint16 SY_CHAIN_EVENT;

/* a handler on an event's chain. the chain calls its handlers by ascending
 * priority, and a handler added at a priority already there goes first.
 * priority has bit 31 set while the handler is in use */
typedef struct SYS_CHAIN_ISR_ {
    void (*usrIsr)(void *);
    struct SYS_CHAIN_ISR_ *next;
    void *arg;
    Uint32 priority;
} SYS_CHAIN_ISR;
/* a handler's number in the handler pool, from 1, as syChainAddHandler returns
 * it; 0 when the pool is full */
typedef Uint32 SYCHAIN;

void syChainInit(void);
/* add usrIsr, with arg, to event's chain, ahead of the first handler of no
 * lower priority. the result is the handler's number, or 0 when the pool is
 * full. an event's first handler sets the event up. a CPU interrupt gets
 * syChainMasterHandler as its handler. the vblank and hblank events get Kamui
 * callbacks. a bus event's source is unmasked at level 4, and
 * syChainClx1Handler becomes the handler of the CPU's level 4 interrupt,
 * through which the bus's interrupts arrive */
SYCHAIN syChainAddHandler(SY_CHAIN_EVENT event, void (*usrIsr)(void *),
                          Uint32 priority, void *arg);
/* remove the handler chain names. when it was its event's last, the vblank and
 * hblank events lose their Kamui callbacks. any other event gets syIntBrkRteRte
 * as its USER_VECT handler, after a bus event's source is masked at level 4. no
 * CPU code shares a bus event's USER_VECT word, so syChainClx1Handler stays the
 * level 4 interrupt's handler. syChainDeleteHandler passes USER_VECT an
 * unsigned code, so for a code below 0x200 it writes another word than
 * syChainAddHandler set, as USER_VECT says */
void syChainDeleteHandler(SYCHAIN chain);

#endif
