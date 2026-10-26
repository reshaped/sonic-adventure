/* vibration: shaking a pad through the pack in one of its sockets */
#include <sg_pdvib.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "progress/demo.h"
#include "system/task.h"
#include "vmu/vibration.h"

/* for a pad with no pack ready */
#define NO_PORT 0xffffffff

/* the expansion sockets in a pad */
#define SOCKETS 2

/* the bounds vibration.h gives power, freq and time */
#define POWER_MAX 4
#define FREQ_MIN 7
#define FREQ_MAX 59
#define TIME_MAX 255

/* the fade's whole inc, which vibration_start_fading divides by its span */
#define FADE_INC 255

/* the task that starts a vibration keeps these in its extra */
struct vibration_extra {
    Uint32 port;
    Sint32 steps; /* taken so far */
};

#define EXTRA(task) ((struct vibration_extra *)(task)->extra)

/* each pad's sockets, side by side */
static const Uint32 vibration_ports[] = {
    PDD_PORT_A1, PDD_PORT_A2, PDD_PORT_B1, PDD_PORT_B2,
    PDD_PORT_C1, PDD_PORT_C2, PDD_PORT_D1, PDD_PORT_D2,
};

/* on the task's second step, start the vibration and destroy the task.
 * task_destroy frees the parameters in its force */
static void vibration_step(struct task *task)
{
    PDS_VIBPARAM *param;

    param = task->force;
    /* @hack spelling: the extra read through EXTRA at each use, which loads
     * task->extra again for the port (MOV.L @(44,R14),R4 at 0x8c118eba);
     * through a local, read once, 20 of 24 words differ */
    if (EXTRA(task)->steps++ == 1) {
        pdVibMxStart(EXTRA(task)->port, param);
        task_destroy(task);
    }
}

/* start a vibration of the pack at port, through a task that waits a step.
 * nothing starts when an allocation fails */
static void vibration_queue(Uint32 port, int flag, int power, int freq, int inc)
{
    struct task *task;
    PDS_VIBPARAM *param;

    /* @hack spelling: task assigned inside the test, which tests task_create's
     * R0 and copies it to R14 in the BT/S's slot (0x8c118ee8, 0x8c118eec);
     * assigned apart, SHC tests R14 and 3 of 46 words differ */
    if ((task = task_create(TASK_EXTRA, 0, vibration_step)) != NULL) {
        /* @hack spelling: param assigned inside the test, as task is
         * (0x8c118ef4, 0x8c118ef8); assigned apart, 3 of 46 words differ */
        if ((param = heap_alloc(sizeof(*param))) != NULL) {
            task->force = param;
            EXTRA(task)->port = port;
            param->unit = 1;
            param->flag = flag;
            param->power = power;
            param->freq = freq;
            param->inc = inc;
        } else {
            task_destroy(task);
        }
    }
}

void vibration_start(int pad, int power, int freq, int time)
{
    Uint32 port;
    int socket;
    int i;
    int flag;

    if (demo_playing) {
        return;
    }
    port = NO_PORT;
    /* @hack spelling: socket, a counter apart from i, which the loop
     * compares with SOCKETS (MOV #2,R2 at 0x8c118f6e); with i alone, SHC
     * keeps its bound, pad * SOCKETS + SOCKETS, in the frame and 99 of 100
     * words differ */
    for (socket = 0, i = pad * SOCKETS; socket < SOCKETS; socket++, i++) {
        if (pdVibMxIsReady(vibration_ports[i])) {
            port = vibration_ports[i];
            break;
        }
    }
    if (port == NO_PORT) {
        return;
    }
    if (power > POWER_MAX) {
        power = POWER_MAX;
    } else if (power < -POWER_MAX) {
        power = -POWER_MAX;
    }
    if (power == 1) {
        power = 2;
    } else if (power == -1) {
        power = -2;
    }
    if (freq > FREQ_MAX) {
        freq = FREQ_MAX;
    } else if (freq < FREQ_MIN) {
        freq = FREQ_MIN;
    }
    if (time > TIME_MAX) {
        time = TIME_MAX;
    }
    if (time < 0) {
        time = 0;
    }
    if (time == 0) {
        flag = 0;
    } else {
        flag = PDD_VIB_FLAG_CONTINUOUS;
        pdVibMxSetStopTime(port, time);
    }
    vibration_queue(port, flag, power, freq, 0);
}

void vibration_start_fading(int pad, int power, int freq, int length)
{
    Uint32 port;
    int socket;
    int i;
    int span;
    int flag;
    /* @hack redundant: eight bytes nothing reads or writes, which make the
     * frame 12 bytes (ADD #-12,R15 at 0x8c119050); without them, it is 4 and
     * 2 of 98 words differ */
    Uint8 unused[8];

    if (demo_playing) {
        return;
    }
    port = NO_PORT;
    /* @hack spelling: socket, a counter apart from i, as in vibration_start;
     * with i alone, 85 of 98 words differ */
    for (socket = 0, i = pad * SOCKETS; socket < SOCKETS; socket++, i++) {
        if (pdVibMxIsReady(vibration_ports[i])) {
            port = vibration_ports[i];
            break;
        }
    }
    if (port == NO_PORT) {
        return;
    }
    if (power > POWER_MAX) {
        power = POWER_MAX;
    } else if (power < -POWER_MAX) {
        power = -POWER_MAX;
    }
    if (power == 1) {
        power = 2;
    } else if (power == -1) {
        power = -2;
    }
    if (freq > FREQ_MAX) {
        freq = FREQ_MAX;
    } else if (freq < FREQ_MIN) {
        freq = FREQ_MIN;
    }
    flag = PDD_VIB_FLAG_CONVERGENCE;
    span = freq * length / (power * 4);
    if (span <= 0) {
        span = 1;
    }
    vibration_queue(port, flag, power, freq, FADE_INC / span);
}
