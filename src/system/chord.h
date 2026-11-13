/* what passes between port.c and the overlay.c that draws the port's
 * overlay on each machine. only port.c, engine/dc/overlay.c and
 * engine/sa1/overlay.c include it. these stay out of port.h, which ninja.h
 * includes, since no other source uses them */
#ifndef SYSTEM_CHORD_H
#define SYSTEM_CHORD_H
#ifdef NONMATCHING
#include <shinobi.h>

/* read the toggles' chord on pad, the first pad's record, each time a
 * machine fills it. the chord is L and R held with a fresh press of the
 * d-pad. up steps the port's timing readout through its pages. right flips
 * 16:9 and down flips 60 fps. from that press until L, R and the d-pad are
 * all up, the game sees none of them */
void port_chord(PDS_PERIPHERAL *pad);
/* each machine's overlay.c defines overlay_page and overlay_note for
 * port_chord. overlay_page steps the readout to its next page as up steps
 * it. it returns TRUE where a page then shows, and the readout's toggle
 * takes that */
int overlay_page(void);
/* name toggle, a port_toggle, and its new state near the screen's top for
 * two seconds */
void overlay_note(int toggle);
/* the drawn frames the curtain takes to fall or lift */
#define CURTAIN_FADE 8
/* the drawn frames the curtain has faded in. it is 0 where the curtain has
 * lifted, and CURTAIN_FADE where it has fallen. a machine draws the curtain
 * with this over CURTAIN_FADE for its opacity */
int port_curtain_level(void);

#endif
#endif
