/* keyboard peripheral data */
#include <kaede.h>
#include <sg_pdkbd.h>
#include <sg_xpt.h>
#include <shinobi.h>

/* one keyboard per connector */
#define KEYBOARDS 4
/* a condition's offset in a device's response, after its header and function
 * words */
#define CONDITION 8

/* each connector's keyboard, by the connector's number from 1, so
 * pdgKbdData[0] goes unused */
#ifdef GCC
/* room for connector 4's record as well, which pdSetKbdData's tag says the
 * image writes past the array's end. in 1ST_READ.BIN that record covers
 * the start of isrPool[0], which syChain.c never hands out, so the extra
 * record changes nothing the game does. in LOADER.BIN it repairs the
 * image's write over syCache.c's cache_ram_pages and cache_ram_page_offset */
PDS_KEYBOARD pdgKbdData[KEYBOARDS + 1];
#else
PDS_KEYBOARD pdgKbdData[KEYBOARDS];
#endif

void pdSetKbdData(Sint32 connector, PDS_PERIPHERAL *per,
                  const KDS_DEVICE *device, const PDS_PERIPHERALINFO *info)
{
    /* @hack spelling: the locals in this order, from which SHC at -optimize=0
     * sets their frame slots: i at @(12,R15), data at @(8,R15), key at
     * @(4,R15) and kbd at @R15 (0x8c65ef2a to 0x8c65ef80 in 1ST_READ.BIN);
     * declared in the order of their first use, 18 of 73 words differ in each
     * image */
    int i;
    Uint8 *data;
    Uint8 *key;
    PDS_KEYBOARD *kbd;

    data = device->recvdata;
    data += CONDITION;
    per->support = 0;
    per->id = PDD_DEV_KEYBOARD;
    per->name = (char *)info->product_name;
    /* @bug memory: pdExecPeripheralServer numbers the connectors from
     * 1 (ADD #1,R1 at 0x8c61b84c in 1ST_READ.BIN), so connector 4's keyboard
     * lands at pdgKbdData[4], past the array's end (0x8c65ef48 to 0x8c65ef50
     * in 1ST_READ.BIN). there it covers isrPool's first 8 bytes, at
     * 0x8c8c8044, and in LOADER.BIN cache_ram_pages and the first 4 bytes of
     * cache_ram_page_offset, at 0x8ce50640 */
    kbd = &pdgKbdData[connector];
    per->extend = kbd;
    kbd->ctrl = *data++;
    kbd->led = *data++;
    key = kbd->key;
    for (i = 0; i < (int)sizeof(kbd->key); i++) {
        *key++ = *data++;
    }
}

const PDS_KEYBOARD *pdKbdGetData(Uint32 port)
{
    const PDS_PERIPHERAL *per = pdGetPeripheral(port);

    if (per->info->type & PDD_DEVTYPE_KEYBOARD) {
        return per->extend;
    }
    return NULL;
}
