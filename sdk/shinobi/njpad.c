/* Ninja peripheral access and peripheral state printing */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

/* a number's print location, one column right when the number is not
 * negative, so that its digits align with a negative number's */
#define NUMBER_LOCATION(x, y, n) NJM_LOCATION((x) + ((n) >= 0), y)

const NJS_PERIPHERAL *njGetPeripheral(Uint32 port)
{
    return pdGetPeripheral(port);
}

void njPrintPeripheralInfo(Int loc, Uint32 port)
{
    const NJS_PERIPHERAL *per;
    Sint32 x;
    Sint32 y;

    x = (Sint16)(loc >> 16);
    y = (Sint16)loc;
    per = njGetPeripheral(port);

    njPrintC(NJM_LOCATION(x, y), "SUPPORT:");
    njPrintC(NJM_LOCATION(x, y + 1), "ON     :");
    njPrintC(NJM_LOCATION(x, y + 2), "OFF    :");
    njPrintC(NJM_LOCATION(x, y + 3), "PRESS  :");
    njPrintC(NJM_LOCATION(x, y + 4), "RELEASE:");
    njPrintC(NJM_LOCATION(x, y + 5), "R      :");
    njPrintC(NJM_LOCATION(x, y + 6), "L      :");
    njPrintC(NJM_LOCATION(x, y + 7), "X1     :");
    njPrintC(NJM_LOCATION(x, y + 8), "Y1     :");
    njPrintC(NJM_LOCATION(x, y + 9), "X2     :");
    njPrintC(NJM_LOCATION(x, y + 10), "Y2     :");

    x += 8;
    njPrintH(NJM_LOCATION(x, y), per->support, 8);
    njPrintH(NJM_LOCATION(x, y + 1), per->on, 8);
    njPrintH(NJM_LOCATION(x, y + 2), per->off, 8);
    njPrintH(NJM_LOCATION(x, y + 3), per->press, 8);
    njPrintH(NJM_LOCATION(x, y + 4), per->release, 8);
    njPrintD(NUMBER_LOCATION(x, y + 5, per->r), per->r, 5);
    njPrintD(NUMBER_LOCATION(x, y + 6, per->l), per->l, 5);
    njPrintD(NUMBER_LOCATION(x, y + 7, per->x1), per->x1, 5);
    njPrintD(NUMBER_LOCATION(x, y + 8, per->y1), per->y1, 5);
    njPrintD(NUMBER_LOCATION(x, y + 9, per->x2), per->x2, 5);
    njPrintD(NUMBER_LOCATION(x, y + 10, per->y2), per->y2, 5);
}
