/* Shinobi keyboard library, as far as the sources use it */
#ifndef SG_PDKBD_H
#define SG_PDKBD_H
#include <kaede.h>
#include <sg_xpt.h>
#include <shinobi.h>

/* a keyboard's state from its last response: the modifier keys, the LEDs, and
 * up to six keys held */
typedef struct {
    Uint8 ctrl;
    Uint8 led;
    Uint8 key[6];
} PDS_KEYBOARD;

/* the keyboard state of port, or NULL unless the port holds a keyboard. port
 * D's state is the record past the end of pd_kbd.c's pdgKbdData, as its @bug
 * tag says, so it shares its bytes with the data after the array */
const PDS_KEYBOARD *pdKbdGetData(Uint32 port);
/* fill per, and the connector's PDS_KEYBOARD, from a keyboard's response; the
 * peripheral server calls it, counting connectors from 1 */
void pdSetKbdData(Sint32 connector, PDS_PERIPHERAL *per,
                  const KDS_DEVICE *device, const PDS_PERIPHERALINFO *info);

#endif
