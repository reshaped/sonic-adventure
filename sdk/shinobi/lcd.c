/* the direction of an LCD in a controller's expansion socket, and each
 * port's LCD number */
#include <kaede.h>
#include <sg_lcd.h>
#include <sg_xpt.h>
#include <shinobi.h>

const char pdgLcdBuild[] = "\npdLcd Ver 1.01 Build:Sep 07 1998 11:53:36\n";

/* pdLcdGetDirection's results, indexed by 4 times the connector direction of
 * the controller's socket, plus the connector direction of the device in it */
static const Uint8 directions[16] = {
    PDD_LCD_DIRECTION_FLIP,  PDD_LCD_DIRECTION_NORMAL, PDD_LCD_DIRECTION_LEFT,
    PDD_LCD_DIRECTION_RIGHT, PDD_LCD_DIRECTION_NORMAL, PDD_LCD_DIRECTION_FLIP,
    PDD_LCD_DIRECTION_RIGHT, PDD_LCD_DIRECTION_LEFT,   PDD_LCD_DIRECTION_RIGHT,
    PDD_LCD_DIRECTION_LEFT,  PDD_LCD_DIRECTION_FLIP,   PDD_LCD_DIRECTION_NORMAL,
    PDD_LCD_DIRECTION_LEFT,  PDD_LCD_DIRECTION_RIGHT,  PDD_LCD_DIRECTION_NORMAL,
    PDD_LCD_DIRECTION_FLIP,
};

/* pdgLcdNo's entry for any port but the first two expansion sockets of a
 * controller, which alone have LCD numbers */
#define NO_LCD 255

const Uint8 pdgLcdNo[24] = {
    NO_LCD, 0, 1, NO_LCD, NO_LCD, NO_LCD, NO_LCD, 2, 3, NO_LCD, NO_LCD, NO_LCD,
    NO_LCD, 4, 5, NO_LCD, NO_LCD, NO_LCD, NO_LCD, 6, 7, NO_LCD, NO_LCD, NO_LCD,
};

/* the port of the controller on each port's connector */
static const Uint8 root_ports[24] = {
    PDD_PORT_A0, PDD_PORT_A0, PDD_PORT_A0, PDD_PORT_A0, PDD_PORT_A0,
    PDD_PORT_A0, PDD_PORT_B0, PDD_PORT_B0, PDD_PORT_B0, PDD_PORT_B0,
    PDD_PORT_B0, PDD_PORT_B0, PDD_PORT_C0, PDD_PORT_C0, PDD_PORT_C0,
    PDD_PORT_C0, PDD_PORT_C0, PDD_PORT_C0, PDD_PORT_D0, PDD_PORT_D0,
    PDD_PORT_D0, PDD_PORT_D0, PDD_PORT_D0, PDD_PORT_D0,
};

void pdLcdInit(void)
{
}

Sint32 pdLcdGetDirection(Uint32 port)
{
    const KDS_DEVICEINFO *root;
    const KDS_DEVICEINFO *lcd;
    Sint32 socket;

    root = kdGetDeviceInfo(root_ports[port]);
    lcd = kdGetDeviceInfo(port);
    switch (port) {
    case PDD_PORT_A2:
    case PDD_PORT_B2:
    case PDD_PORT_C2:
    case PDD_PORT_D2:
        socket = 1;
        break;
    default:
        socket = 0;
        break;
    }
    return directions[(4 * root->connector_dir[socket]) +
                      lcd->connector_dir[0]];
}

void pdLcdExit(void)
{
}
