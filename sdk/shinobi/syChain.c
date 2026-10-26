/* interrupt chains: each event's handlers, called in ascending order of
 * priority */
#include <kamui.h>
#include <machine.h>
#include <sg_chain.h>
#include <sg_xpt.h>
#include <shinobi.h>

/* the pool's handlers. syChainAddHandler hands them out from 1 up, since 0
 * means a full pool */
#define ISR_MAX 256
/* an event's index in keyTbl: its code divided by 16. EVENT gives a key's
 * code back. KEY_MAX keys cover the codes below 0xc00, the bus events'
 * among them */
#define KEY(event) ((event) >> 4)
#define EVENT(key) ((key) << 4)
#define KEY_MAX 192
/* bit 31 of a handler's priority marks it in use */
#define ISR_USED 0x80000000

/* the CPU interrupt events through which the system bus's interrupts
 * arrive, at levels 6, 4 and 2 */
#define EVENT_IML6 0x320
#define EVENT_IML4 0x360
#define EVENT_IML2 0x3a0

/* the system bus's interrupt status and level 4 mask registers, through
 * P2, each a normal and an external word */
#define INT_STATUS_NORMAL (*(Uint32 *)0xa05f6900)
#define INT_STATUS_EXTERNAL (*(Uint32 *)0xa05f6904)
#define INT_LEVEL4_NORMAL (*(Uint32 *)0xa05f6920)
#define INT_LEVEL4_EXTERNAL (*(Uint32 *)0xa05f6924)
/* a bus event's kind, normal or external, and its source's number in that
 * kind. a normal source's bit in the status and mask words is its number
 * plus NORMAL_FIRST. an external source's bit is its number.
 * syChainClx1Handler checks NORMAL_SOURCES and EXTERNAL_SOURCES of them */
#define KIND(event) ((event) >> 8)
#define KIND_NORMAL 0xa
#define KIND_EXTERNAL 0xb
#define SOURCE(event) ((Uint8)(event) >> 5)
#define NORMAL_FIRST 12
#define NORMAL_SOURCES 7
#define EXTERNAL_SOURCES 4

char *sygChainBuild = "\r\nsyChain Ver 1.01 Build:Oct 01 1998 16:37:03\r\n";
const Uint16 sygChainVer[2] = {1, 1};
SYS_CHAIN_ISR isrPool[ISR_MAX];
SYS_CHAIN_ISR *keyTbl[KEY_MAX];

void syChainMasterHandler(SY_CHAIN_EVENT event);
void syChainMasterHandlerPVR_V(void *arg);
void syChainMasterHandlerPVR_H(void *arg);
void syChainClx1Handler(SY_CHAIN_EVENT event);

void syChainInit(void)
{
    Uint16 mask = get_imask();
    Uint32 i;

    set_imask(IMASK_ALL);
    for (i = 0; i < ISR_MAX; i++) {
        isrPool[i].priority = 0;
    }
    for (i = 0; i < KEY_MAX; i++) {
        keyTbl[i] = NULL;
    }
    set_imask(mask);
}

SYCHAIN syChainAddHandler(SY_CHAIN_EVENT event, void (*usrIsr)(void *),
                          Uint32 priority, void *arg)
{
    Uint16 mask = get_imask();
    SYS_CHAIN_ISR *isr;
    SYS_CHAIN_ISR *last;
    Uint32 i;
    SYCHAIN chain;

    set_imask(IMASK_ALL);
    isr = keyTbl[KEY(event)];
    last = isr;
    while (isr != NULL && isr->priority < (priority | ISR_USED)) {
        last = isr;
        isr = isr->next;
    }
    for (i = 1; i < ISR_MAX; i++) {
        if (!(isrPool[i].priority & ISR_USED)) {
            break;
        }
    }
    if (i < ISR_MAX) {
        /* @hack spelling: each arm stores usrIsr, arg and priority itself.
         * SHC keeps the first arm's stores ahead of its unmask (0x8c65f158
         * to 0x8c65f15c, and 0x8ce0c82c to 0x8ce0c830 in LOADER.BIN), and
         * merges the other two arms' copies. stored once after the arms, the
         * three follow the unmask, the function is 12 bytes shorter and 163
         * of 204 words differ */
        if (isr == NULL && last == NULL) {
            if (event < SYD_CHAIN_EVENT_IML6_VBLANK) {
                USER_VECT(event) = (Uint32)syChainMasterHandler;
            } else if (event == SYD_CHAIN_EVENT_IML6_VBLANK) {
                kmSetVSyncCallback(syChainMasterHandlerPVR_V, NULL);
            } else if (event == SYD_CHAIN_EVENT_IML6_HBLANK) {
                kmSetHSyncCallback(syChainMasterHandlerPVR_H, NULL);
            } else {
                USER_VECT(EVENT_IML4) = (Uint32)syChainClx1Handler;
            }
            keyTbl[KEY(event)] = &isrPool[i];
            isrPool[i].next = NULL;
            isrPool[i].usrIsr = usrIsr;
            isrPool[i].arg = arg;
            isrPool[i].priority = priority | ISR_USED;
            if (KIND(event) == KIND_NORMAL) {
                INT_LEVEL4_NORMAL |= 1 << (SOURCE(event) + NORMAL_FIRST);
            } else if (KIND(event) == KIND_EXTERNAL) {
                INT_LEVEL4_EXTERNAL |= 1 << SOURCE(event);
            }
        } else if (isr == keyTbl[KEY(event)]) {
            isrPool[i].next = isr;
            keyTbl[KEY(event)] = &isrPool[i];
            isrPool[i].usrIsr = usrIsr;
            isrPool[i].arg = arg;
            isrPool[i].priority = priority | ISR_USED;
        } else {
            last->next = &isrPool[i];
            isrPool[i].next = isr;
            isrPool[i].usrIsr = usrIsr;
            isrPool[i].arg = arg;
            isrPool[i].priority = priority | ISR_USED;
        }
        chain = i;
    } else {
        chain = 0;
    }
    set_imask(mask);
    return chain;
}

/* call an event's handlers in order */
void syChainMasterHandler(SY_CHAIN_EVENT event)
{
    SYS_CHAIN_ISR *isr = keyTbl[KEY(event)];

    while (isr != NULL) {
        isr->usrIsr(isr->arg);
        isr = isr->next;
    }
}

/* Kamui's vertical sync callback: call the vertical blank handlers with
 * the interrupt mask at 6 */
void syChainMasterHandlerPVR_V(void *arg)
{
    SYS_CHAIN_ISR *isr;

    set_imask(6);
    isr = keyTbl[KEY(SYD_CHAIN_EVENT_IML6_VBLANK)];
    while (isr != NULL) {
        isr->usrIsr(isr->arg);
        isr = isr->next;
    }
}

/* Kamui's horizontal sync callback: call the horizontal blank handlers with
 * the interrupt mask at 6 */
void syChainMasterHandlerPVR_H(void *arg)
{
    SYS_CHAIN_ISR *isr;

    set_imask(6);
    isr = keyTbl[KEY(SYD_CHAIN_EVENT_IML6_HBLANK)];
    while (isr != NULL) {
        isr->usrIsr(isr->arg);
        isr = isr->next;
    }
}

/* the system bus's interrupt handler. at level 4 it calls the handlers of
 * each normal source that is pending and unmasked. with none, it calls those
 * of each such external source. it ignores level 6 and level 2 */
void syChainClx1Handler(SY_CHAIN_EVENT event)
{
    Uint32 pending;
    int source;

    switch (KEY(event)) {
    case KEY(EVENT_IML6):
        break;
    case KEY(EVENT_IML4):
        set_imask(4);
        pending = (INT_STATUS_NORMAL & INT_LEVEL4_NORMAL) >> NORMAL_FIRST;
        if (pending != 0) {
            source = 0;
            do {
                if (pending & (1 << source)) {
                    syChainMasterHandler(SYD_CHAIN_EVENT_IML4_NRM +
                                         (source << 5));
                }
                source++;
            } while ((1 << source) != (1 << NORMAL_SOURCES));
        } else {
            pending = INT_STATUS_EXTERNAL & INT_LEVEL4_EXTERNAL;
            source = 0;
            do {
                if (pending & (1 << source)) {
                    syChainMasterHandler(SYD_CHAIN_EVENT_IML4_EXT +
                                         (source << 5));
                }
                source++;
            } while ((1 << source) != (1 << EXTERNAL_SOURCES));
        }
        break;
    case KEY(EVENT_IML2):
        break;
    }
}

void syChainDeleteHandler(SYCHAIN chain)
{
    Uint16 mask = get_imask();
    Uint32 i;

    set_imask(IMASK_ALL);
    for (i = 1; i < ISR_MAX; i++) {
        if (isrPool[i].next == &isrPool[chain] &&
            (isrPool[i].priority & ISR_USED)) {
            break;
        }
    }
    if (i != ISR_MAX) {
        isrPool[i].next = isrPool[chain].next;
        isrPool[chain].priority = 0;
    } else {
        for (i = 1; i < KEY_MAX; i++) {
            if (keyTbl[i] == &isrPool[chain]) {
                keyTbl[i] = isrPool[chain].next;
                isrPool[chain].priority = 0;
                if (keyTbl[i] == NULL) {
                    if (i == KEY(SYD_CHAIN_EVENT_IML6_VBLANK)) {
                        kmSetVSyncCallback(NULL, NULL);
                    } else if (i == KEY(SYD_CHAIN_EVENT_IML6_HBLANK)) {
                        kmSetHSyncCallback(NULL, NULL);
                    } else {
                        SY_CHAIN_EVENT event = EVENT(i);

                        if (KIND(event) == KIND_NORMAL) {
                            INT_LEVEL4_NORMAL &=
                                ~(1 << (SOURCE(event) + NORMAL_FIRST));
                        } else if (KIND(event) == KIND_EXTERNAL) {
                            INT_LEVEL4_EXTERNAL &= ~(1 << SOURCE(event));
                        }
                        USER_VECT(EVENT(i)) = (Uint32)syIntBrkRteRte;
                    }
                }
                break;
            }
        }
        /* @hack redundant: an empty test of i, which keeps the handler's offset
         * in R12 (MOV R4,R12 at 0x8c65f394); without it, the offset takes R13
         * and 104 of 160 words differ */
        if (i == KEY_MAX) {
        }
    }
    set_imask(mask);
}
