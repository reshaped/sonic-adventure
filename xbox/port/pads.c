/* Shinobi's peripherals, the pads, on the host's: the port layer's side of
 * sdk/shinobi/pdmain.c and njpad.c, which make pc leaves out.
 *
 * each of the four ports, A to D, numbered 0, 6, 12 and 18, holds a
 * controller or nothing, as the host says. each vertical blank takes the
 * host's reading of each port, as the console's Maple DMA does, and
 * pdExecPeripheralServer, which the game calls once a frame, updates each
 * port's state from the last reading, as Shinobi's server does: on holds the
 * buttons held, as PDD_DGT_ bits, off the others, press those held now and
 * not at the last call, release those held then and not now, and old the
 * last call's on. a press so lasts a frame of the game's, two blanks at 30
 * frames a second, as on the console, and a press between calls isn't lost */
#include <shinobi.h>
#include <sg_xpt.h>

#include "xbox/port/host.h"
#include "xbox/port/port.h"

#define PORTS 4

static PDS_PERIPHERAL ports[PORTS];
static PDS_PERIPHERALINFO infos[PORTS];

/* each port's last reading: its buttons, ~0u for no controller, its stick
 * and its triggers */
static struct reading {
    unsigned buttons;
    int x;
    int y;
    int l;
    int r;
    int right_y; /* the right stick's up and down, -128 to 127, down more */
} readings[PORTS];

/* port A's right stick, up and down, as of the game's last
 * pdExecPeripheralServer: -128 to 127, down more, 0 at rest or without one.
 * the camera's height (src/camera/camera.c, under XBOX) */
int port_right_y;

/* the support bits of the console's standard controller, its buttons, both
 * triggers and one stick, which are PDD_DEV_CONTROLLER's: the game takes a
 * port for a controller's where its support holds every one */
#define CONTROLLER_SUPPORT PDD_DEV_CONTROLLER

static void update(int k)
{
    PDS_PERIPHERAL *per = &ports[k];
    const struct reading *in = &readings[k];
    unsigned buttons = in->buttons;
    Uint32 was = per->on;

    if (buttons == ~0u) {
        per->id = 0;
        per->support = 0;
        per->on = per->press = per->release = 0;
        per->off = ~0u;
        per->old = 0;
        return;
    }
    /* the triggers' digital bits, as Shinobi's read_controller sets them
     * (sdk/shinobi/pdmain.c): pressed while the analog value's top bit is
     * set, 128 or more. the game's camera turns and Casinopolis' flippers
     * read these bits, so they must be set */
    if (in->l >= 0x80) {
        buttons |= PDD_DGT_TL;
    }
    if (in->r >= 0x80) {
        buttons |= PDD_DGT_TR;
    }
    if (k == 0) {
        port_right_y = in->right_y;
    }
    per->id = PDD_DEV_CONTROLLER;
    per->support = CONTROLLER_SUPPORT;
    per->old = was;
    per->on = buttons;
    per->off = ~buttons;
    per->press = buttons & ~was;
    per->release = was & ~buttons;
    per->x1 = (Sint16)(in->x - 128);
    per->y1 = (Sint16)(in->y - 128);
    per->x2 = per->y2 = 0;
    per->l = (Uint16)in->l;
    per->r = (Uint16)in->r;
    per->info = &infos[k];
}

static void read_port(int k)
{
    struct reading *in = &readings[k];

    in->buttons = HOST->pad_read(k, &in->x, &in->y, &in->l, &in->r);
    in->right_y = 0;
    if (in->buttons != ~0u && (in->buttons & HOST_PAD_RIGHT_Y)) {
        in->right_y = (int)(in->buttons >> 24) - 128;
        in->buttons &= HOST_PAD_RIGHT_Y - 1;
    }
}

/* the vertical blank's reading, which system.c runs ahead of the chains */
void pads_vblank(void)
{
    int k;

    for (k = 0; k < PORTS; k++) {
        read_port(k);
    }
}

void pdInitPeripheral(Sint32 logic, void *recvbuf, void *sendbuf)
{
    int k;

    (void)logic;
    (void)recvbuf;
    (void)sendbuf;
    for (k = 0; k < PORTS; k++) {
        ports[k].name = "Dreamcast Controller";
        infos[k].type = PDD_DEV_CONTROLLER;
        read_port(k);
        update(k);
        ports[k].press = 0;
    }
}

void pdExitPeripheral(void)
{
}

void pdExecPeripheralServer(void)
{
    int k;

    for (k = 0; k < PORTS; k++) {
        update(k);
    }
}

const PDS_PERIPHERAL *pdGetPeripheral(Uint32 port)
{
    return &ports[(port / 6) % PORTS];
}

const PDS_PERIPHERALINFO *pdGetPeripheralInfo(Uint32 port)
{
    return &infos[(port / 6) % PORTS];
}

const NJS_PERIPHERAL *njGetPeripheral(Uint32 port)
{
    return pdGetPeripheral(port);
}
