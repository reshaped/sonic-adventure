/* Shinobi vibration library, as far as the sources use it */
#ifndef SG_PDVIB_H
#define SG_PDVIB_H
#include <sg_xpt.h>

/* return codes: done, busy, and no vibration pack on the port */
#define PDD_VIBERR_OK 0
#define PDD_VIBERR_BUSY (-1)
#define PDD_VIBERR_NO_VIBRATOR (-2)

/* pack orientations, as pdVibGetDirection returns them: as its controller is
 * held, upside down, or on its left or right side */
#define PDD_VIB_DIRECTION_NORMAL 0
#define PDD_VIB_DIRECTION_FLIP 1
#define PDD_VIB_DIRECTION_LEFT 2
#define PDD_VIB_DIRECTION_RIGHT 3

#define PDD_VIB_FLAG_CONTINUOUS (1 << 0)
#define PDD_VIB_FLAG_EXHALATION (1 << 3)
#define PDD_VIB_FLAG_CONVERGENCE (1 << 7)

/* a vibration for one unit of a pack */
typedef struct {
    Uint8 unit;
    Uint8 flag;
    Sint8 power;
    Uint8 freq;
    Uint8 inc;
    Uint8 reserved[3];
} PDS_VIBPARAM;

/* a pack's unit counts, the vn and se bytes of its vibration function data */
typedef struct {
    Uint8 units;
    Uint8 se_units;
} PDS_VIBINFO;

/* a unit's description, from its media info */
typedef struct {
    Uint8 position;
    Uint8 axis;
    Uint8 pow_enable;
    Uint8 cont_enable;
    Uint8 dir_enable;
    Uint8 wave_enable;
    Uint8 min_freq;
    Uint8 max_freq;
} PDS_VIBUNITINFO;

typedef Sint32 (*PD_VIBCALLBACK)(Sint32 stat, Uint32 param);
typedef Sint32 (*PD_VIBENUMUNITCALLBACK)(Uint32 unit, Sint32 stat,
                                         PDS_VIBUNITINFO *info, Uint32 param);

/* single-unit calls. pdVibMxStart starts the one vibration param gives, and
 * pdVibMxStop stops unit 1. pdVibMxSetStopTime sets unit 1's stop time, as
 * pdVibSetStopTime would. each but pdVibMxIsReady returns
 * PDD_VIBERR_NO_VIBRATOR when the port holds no pack */
Sint32 pdVibMxIsReady(Uint32 port);
Sint32 pdVibMxSetStopTime(Uint32 port, Uint32 time);
Sint32 pdVibMxStart(Uint32 port, const PDS_VIBPARAM *param);
Sint32 pdVibMxStop(Uint32 port);

/* calls for any of a pack's units. pdVibSetStopTime writes the stop times of
 * num units to the pack as one block: a 16-bit mask of the units, high byte
 * first, then each unit's time. pdVibStart starts the vibrations of numunits
 * units, one for each param. numunits must be 16 or fewer */
Sint32 pdVibIsReady(Uint32 port);
Sint32 pdVibGetDirection(Uint32 port);
Sint32 pdVibSetStopTime(Uint32 port, Uint8 *unit, Uint8 *time, Uint32 num);
Sint32 pdVibStart(Uint32 port, const PDS_VIBPARAM *param, Uint32 numunits);
Sint32 pdVibGetInfo(Uint32 port, PDS_VIBINFO *info);
/* request each unit's media info in turn, from unit 1 to the pack's unit count,
 * and pass each answer to func with param. a failed answer ends the walk. the
 * call returns PDD_VIBERR_OK once it has made the first request. it returns
 * PDD_VIBERR_BUSY while an earlier walk waits for its first answer, or when
 * Kaede refuses the first request. it returns PDD_VIBERR_NO_VIBRATOR for a port
 * without a pack. a call that fails to make the first request, for no pack or
 * a refusal, leaves the port busy for good */
Sint32 pdVibEnumerateUnit(Uint32 port, PD_VIBENUMUNITCALLBACK func,
                          Uint32 param);

#endif
