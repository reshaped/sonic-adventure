/* video cable detection */
#include <sg_xpt.h>
#include <shinobi.h>

/* the CPU's port A data register; bits 8 and 9 are the video connector's
 * cable pins */
#define PORT_A_DATA (*(Uint16 *)0xff800030)
#define CABLE_PINS 0x0300
#define CABLE_SHIFT 8

char *sygCblBuild = "\nsyCbl Ver 1.00 Build:Sep 29 1998 13:22:53\n";

Uint32 syCblCheckCableType(void);

SYE_CBL_CABLE syCblCheckCable(void)
{
    /* @hack spelling: cable through a local, which takes the mask into R4
     * (MOV #3,R4 at 0x8c61d352, and at 0x8ce0d22a in LOADER.BIN); returned
     * directly, it is masked in R0, the function is 4 bytes shorter and 5
     * of 9 words differ */
    SYE_CBL_CABLE cable;

    cable = (SYE_CBL_CABLE)((syCblCheckCableType() >> CABLE_SHIFT) &
                            (CABLE_PINS >> CABLE_SHIFT));
    return cable;
}

/* PORT_A_DATA's cable pins, left in bits 8 and 9 */
Uint32 syCblCheckCableType(void)
{
    /* @hack spelling: pins through a local, which loads the port into R4
     * (MOV.W @R3,R4 at 0x8c61d35e, and at 0x8ce0d236 in LOADER.BIN); read
     * directly, it loads into R0 and 2 of 6 words differ */
    Uint16 pins;

    pins = PORT_A_DATA;
    return pins & CABLE_PINS;
}
