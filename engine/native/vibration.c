/* Shinobi's vibration calls the game makes. the first pad holds a pack in
 * its first socket where the live pad can shake. the pack's vibrations shake
 * the live pad. a run with no live pad holds no pack, so the game queues no
 * vibration */
#include <stdint.h>

#include "native.h"

#define PDD_PORT_A1 1
#define PDD_VIBERR_OK 0
#define PDD_VIBERR_NO_VIBRATOR (-2)
#define PDD_VIB_FLAG_CONTINUOUS (1 << 0)
#define PDD_VIB_FLAG_CONVERGENCE (1 << 7)
/* the pack's strongest power, as Shinobi's pdVibStart masks it */
#define POWER_MOST 7
#define STRENGTH_MOST 65535

/* PDS_VIBPARAM */
struct vibparam {
    uint8_t unit;
    uint8_t flag;
    int8_t power;
    uint8_t freq;
    uint8_t inc;
    uint8_t reserved[3];
};

int (*native_rumble)(uint32_t strength, uint32_t ms, uint32_t fade_ms);
/* the pack's stop time, in quarter seconds less one */
static uint32_t stop_time;

int32_t pdVibMxIsReady(uint32_t port)
{
    return port == PDD_PORT_A1 && native_rumble && native_rumble(0, 0, 0);
}

int32_t pdVibMxSetStopTime(uint32_t port, uint32_t time)
{
    if (!pdVibMxIsReady(port)) {
        return PDD_VIBERR_NO_VIBRATOR;
    }
    stop_time = time & 0xff;
    return PDD_VIBERR_OK;
}

/* the pack shakes at (freq + 1) / 2 Hz. it shakes for one cycle, or to its
 * stop time where continuous. under convergence it falls a power each inc
 * cycles, which the live pad's fade stands for. the game never sets the
 * exhalation flag, which makes the power rise. Shinobi's pdVibStart puts a
 * negative power in other bits than a positive one. the live pad takes its
 * size alone */
int32_t pdVibMxStart(uint32_t port, const struct vibparam *param)
{
    uint32_t power =
        (uint32_t)(param->power < 0 ? -param->power : param->power);
    uint32_t cycle_ms = 2000 / (param->freq + 1U);
    uint32_t ms = cycle_ms;
    uint32_t fade_ms = 0;

    if (!pdVibMxIsReady(port)) {
        return PDD_VIBERR_NO_VIBRATOR;
    }
    power &= POWER_MOST;
    if ((param->flag & PDD_VIB_FLAG_CONVERGENCE) && param->inc != 0) {
        ms = fade_ms = power * param->inc * cycle_ms;
    } else if (param->flag & PDD_VIB_FLAG_CONTINUOUS) {
        ms = (stop_time + 1) * 250;
    }
    /* a call of 0 ms only answers */
    native_rumble(power * STRENGTH_MOST / POWER_MOST, ms ? ms : 1, fade_ms);
    return PDD_VIBERR_OK;
}
