/* peripheral server, which updates each port's state from Kaede's Maple
 * bus frames */
#include <kaede.h>
#include <sg_pdkbd.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

/* 24 ports: six on each of the four connectors */
#define PORTS 24
#define PORTS_PER_CONNECTOR 6
/* a condition's offset in a device's response, after its header and function
 * words */
#define CONDITION 8
/* the bit of a trigger's value that is set from 128 up, where the trigger also
 * presses the digital L or R */
#define TRIGGER_PRESSED 0x80
/* a stick axis' value at rest */
#define AXIS_CENTER 128

const char pdgBuild[] = "\npd Ver 1.07 Build:Sep 07 1998 11:53:37\n";
static const char no_device[] = "(no device)";
Uint8 pdl_threshold_down = 192;
Uint8 pdl_threshold_up = 64;
Sint8 pdgEnableSupport;
Sint8 pdgKaedeInitFlag;
PDS_PERIPHERAL pdgPeripheral[PORTS];
Sint32 pdgPerError[PORTS];
Sint32 pdgLogic;

static void clear_peripheral(PDS_PERIPHERAL *per);
static void read_controller(PDS_PERIPHERAL *per, const KDS_DEVICE *device);
static Uint32 type_a_support(Uint32 support);
void _pd_peripheral_set_id(PDS_PERIPHERAL *per, const PDS_PERIPHERALINFO *info);
Sint32 _pd_peripheral_error(PDS_PERIPHERAL *per, const PDS_PERIPHERALINFO *info,
                            const KDS_DEVICE *device);

void pdInitPeripheral(Sint32 logic, void *recvbuf, void *sendbuf)
{
    int i;

    pdgLogic = logic;
    pdgKaedeInitFlag = false;
    pdgEnableSupport = true;
    memset(pdgPeripheral, 0, sizeof(pdgPeripheral));
    for (i = 0; i < PORTS; i++) {
        clear_peripheral(&pdgPeripheral[i]);
        pdgPerError[i] = PDD_ERR_OK;
    }
    if (sendbuf != NULL && recvbuf != NULL) {
        pdgKaedeInitFlag = true;
        kdInitSystem(sendbuf, recvbuf);
    }
}

/* reset per to no device, with no button pressed in the current logic */
static void clear_peripheral(PDS_PERIPHERAL *per)
{
    per->id = 0;
    per->support = 0;
    if (pdgLogic == PDD_PLOGIC_ACTIVE) {
        per->on = 0;
        per->off = 0xffffffff;
        per->press = 0;
        per->release = 0;
    } else {
        per->on = 0xffffffff;
        per->off = 0;
        per->press = 0xffffffff;
        per->release = 0xffffffff;
    }
    per->x1 = per->y1 = per->x2 = per->y2 = per->l = per->r = 0;
    per->extend = NULL;
    per->name = (char *)no_device;
}

/* read a controller's condition into per. the response clears a button's bit
 * while the button is pressed. per->on keeps that sense, except under
 * PDD_PLOGIC_ACTIVE, which inverts it */
static void read_controller(PDS_PERIPHERAL *per, const KDS_DEVICE *device)
{
    Uint8 *data = device->recvdata;
    Uint32 on;

    data += CONDITION;
    on = (data[1] << 8) | data[0] | 0xffff0000;
    if (data[3] & TRIGGER_PRESSED) {
        on &= ~PDD_DGT_TL;
    }
    if (data[2] & TRIGGER_PRESSED) {
        on &= ~PDD_DGT_TR;
    }
    if (pdgLogic == PDD_PLOGIC_ACTIVE) {
        on ^= 0xffffffff;
    }
    per->on = on;
    per->r = (per->support & PDD_DEV_SUPPORT_AR) ? data[2] : 0;
    per->l = (per->support & PDD_DEV_SUPPORT_AL) ? data[3] : 0;
    per->x1 = (per->support & PDD_DEV_SUPPORT_AX1) ? data[4] - AXIS_CENTER : 0;
    per->y1 = (per->support & PDD_DEV_SUPPORT_AY1) ? data[5] - AXIS_CENTER : 0;
    per->x2 = (per->support & PDD_DEV_SUPPORT_AX2) ? data[6] - AXIS_CENTER : 0;
    per->y2 = (per->support & PDD_DEV_SUPPORT_AY2) ? data[7] - AXIS_CENTER : 0;
}

/* return support rearranged into PDD_MODE_TYPE_A's layout */
static Uint32 type_a_support(Uint32 support)
{
    /* @hack redundant: register on type_a, which keeps it in R14 (MOV.L
     * @R15,R14 at 0x8c61b68a); without it, type_a takes the frame and 26 of 32
     * words differ */
    register Uint32 type_a;
    /* @hack redundant: two register locals never used, which with type_a have
     * the function save R12 to R14 (0x8c61b680 to 0x8c61b684); without them,
     * 29 of 32 words differ */
    register Uint32 spare1;
    register Uint32 spare2;

    type_a = (support >> 10) & 0x3fff00;
    if (support & PDD_DEV_SUPPORT_AL) {
        type_a |= 0xf0;
    }
    if (support & PDD_DEV_SUPPORT_AR) {
        type_a |= 0x0f;
    }
    return type_a;
}

void _pd_peripheral_set_id(PDS_PERIPHERAL *per, const PDS_PERIPHERALINFO *info)
{
    Uint32 support;

    if (info->type & PDD_DEVTYPE_CONTROLLER) {
        if (kdDevGetControllerInfo((KDS_CONTROLLERINFO *)&support,
                                   (const KDS_DEVICEINFO *)info) < 0) {
            per->support = 0;
            per->id = 0;
            return;
        }
    } else if (info->type & PDD_DEVTYPE_ARGUN) {
        if (kdDevGetGunInfo((KDS_GUNINFO *)&support,
                            (const KDS_DEVICEINFO *)info) < 0) {
            per->support = 0;
            per->id = 0;
            return;
        }
    }
    if (kdGetDriverMode(KDD_MODE_DEVENDIAN) == PDD_MODE_TYPE_A) {
        per->support = type_a_support(support);
    } else {
        per->support = support;
    }
    per->id = per->support;
    per->name = (char *)info->product_name;
    /* @hack redundant: a return at the end, which gives the function a second
     * epilogue that nothing reaches (0x8c61b782 to 0x8c61b78a in 1ST_READ.BIN).
     * without it, that epilogue's 12 bytes go, so every function after it in
     * the object moves 12 bytes, though none of this one's reached words
     * changes: 0 of 83 words differ in each image */
    return; /* NOLINT(readability-redundant-control-flow) */
}

/* the PDD_ERR_ code for a device's response */
Sint32 _pd_peripheral_error(PDS_PERIPHERAL *per, const PDS_PERIPHERALINFO *info,
                            const KDS_DEVICE *device)
{
    switch (device->response) {
    case KDD_RES_TRANSMITAGAIN:
        return (Sint32)PDD_ERR_RETRY;
    case 0:
    case KDD_RES_DEVICESTATUS:
    case KDD_RES_DEVICEALLSTATUS:
    case KDD_RES_DATATRANSFER:
    case KDD_RES_NORESPONSE:
        return PDD_ERR_OK;
    default:
        return (Sint32)PDD_ERR_GENERIC;
    }
}

void pdExecPeripheralServer(void)
{
    int i;
    int connector;
    Uint32 changed;
    const KDS_DEVICE *device;
    const PDS_PERIPHERALINFO *info;
    PDS_PERIPHERAL *per;

    /* @hack redundant: per stepped in the for, though the body sets it again
     * (ADD #52,R2 at 0x8c61b946 in 1ST_READ.BIN); without the step, the
     * function runs 4 bytes short and 16 of 176 words differ in each image */
    for (i = 0, connector = 0; i < PORTS; i++, per++) {
        device = kdGetDevice(i);
        info = (const PDS_PERIPHERALINFO *)kdGetDeviceInfo(i);
        per = &pdgPeripheral[i];
        per->info = (PDS_PERIPHERALINFO *)info;
        per->old = per->on;
        pdgPerError[i] = _pd_peripheral_error(per, info, device);
        if (i % PORTS_PER_CONNECTOR == 0) {
            connector++;
            if ((info->type & (PDD_DEVTYPE_CONTROLLER | PDD_DEVTYPE_ARGUN)) &&
                device->connect == 0 &&
                device->response == KDD_RES_DATATRANSFER) {
                _pd_peripheral_set_id(per, info);
                read_controller(per, device);
            } else if ((info->type & PDD_DEVTYPE_KEYBOARD) &&
                       device->connect == 0 &&
                       device->response == KDD_RES_DATATRANSFER) {
                pdSetKbdData(connector, per, device, info);
            } else {
                clear_peripheral(per);
            }
            per->off = ~per->on;
            if (pdgLogic == PDD_PLOGIC_ACTIVE) {
                changed = per->on ^ per->old;
                per->press = changed & per->on;
                per->release = changed & per->off;
            } else {
                changed = per->on ^ per->old;
                changed ^= 0xffffffff;
                per->press = changed | per->on;
                per->release = changed | per->off;
            }
        } else {
            per->name = (char *)info->product_name;
        }
    }
}

const PDS_PERIPHERAL *pdGetPeripheral(Uint32 port)
{
    return &pdgPeripheral[port];
}

void pdExitPeripheral(void)
{
    int i;

    if (pdgKaedeInitFlag) {
        kdExitSystem();
    }
    memset(pdgPeripheral, 0, sizeof(pdgPeripheral));
    for (i = 0; i < PORTS; i++) {
        clear_peripheral(&pdgPeripheral[i]);
        pdgPerError[i] = PDD_ERR_OK;
    }
}

Sint32 pdGetPeripheralError(Uint32 port)
{
    return pdgPerError[port];
}

const PDS_PERIPHERALINFO *pdGetPeripheralInfo(Uint32 port)
{
    return (const PDS_PERIPHERALINFO *)kdGetDeviceInfo(port);
}

void pdSetMode(Sint32 mode)
{
    kdSetDriverMode(KDD_MODE_DEVENDIAN, mode);
}

Sint32 pdGetMode(void)
{
    return kdGetDriverMode(KDD_MODE_DEVENDIAN);
}
