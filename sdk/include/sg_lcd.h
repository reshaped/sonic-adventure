/* Shinobi LCD library, as far as the sources use it */
#ifndef SG_LCD_H
#define SG_LCD_H
#include <sg_xpt.h>

/* the LCD writes' results: written, busy, and no VMU LCD on the port */
#define PDD_LCDERR_OK 0
#define PDD_LCDERR_BUSY (-1)
#define PDD_LCDERR_NO_LCD (-2)

/* LCD orientations, as pdLcdGetDirection returns them */
#define PDD_LCD_DIRECTION_NORMAL 0
#define PDD_LCD_DIRECTION_FLIP 1
#define PDD_LCD_DIRECTION_LEFT 2
#define PDD_LCD_DIRECTION_RIGHT 3

/* pdVmsLcdWrite flags: flip the image horizontally, vertically or both before
 * writing it. any other flag writes it as it is */
#define PDD_LCD_FLAG_HFLIP 1
#define PDD_LCD_FLAG_VFLIP 2
#define PDD_LCD_FLAG_HVFLIP 3

void pdLcdInit(void);
void pdLcdExit(void);
Sint32 pdLcdGetDirection(Uint32 port);
/* each port's LCD number: 0 to 7 for the expansion sockets, from PDD_PORT_A1 to
 * PDD_PORT_D2, and 255 for any other port */
extern const Uint8 pdgLcdNo[24];

/* VMU LCD calls. an image is 48 by 32 pixels at one bit each, 192 bytes.
 * pdVmsLcdWrite flips data as flag says into a copy it keeps, and writes the
 * copy. pdVmsLcdWrite1 writes data itself, unflipped. each returns a
 * PDD_LCDERR_ code. pdVmsLcdIsReady returns 1 when port holds a VMU with the
 * LCD these calls write, and 0 otherwise */
Sint32 pdVmsLcdWrite(Uint32 port, void *data, Uint32 flag);
Sint32 pdVmsLcdWrite1(Uint32 port, void *data);
Sint32 pdVmsLcdIsReady(Uint32 port);

#endif
