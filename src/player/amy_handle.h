/* the handle an object calls Amy to, and how she works it */
#ifndef PLAYER_AMY_HANDLE_H
#define PLAYER_AMY_HANDLE_H
#include <ninja.h>
#include <sg_xpt.h>

/* the handle Amy is called to, and how she works it. amy_offer_handle sets
 * held and pushed, and stick_ang when she pushes. at a press of the action
 * button she takes the handle. she then stands at pos.x + 6 * njSin(ang)
 * and pos.z + 6 * njCos(ang), facing it */
struct amy_handle {
    Bool held; /* she holds it */
    Bool pushed; /* she holds it and pushes her stick */
    /* the way she last pushed her stick, as amy_read_stick gives it */
    Angle stick_ang;
    Angle ang;
    NJS_POINT3 pos;
};

/* TRUE when player 0's stick is pushed more than about 0.46 of the way out.
 * *ang then gets the way it points: 0 for up and 0x4000 for right. FALSE
 * otherwise, leaving *ang as it was */
Bool amy_read_stick(Angle *ang);
/* call player 0 to the handle at pos for this step, as amy_call_to_handle
 * does. ang places her round the handle, as struct amy_handle says. the
 * call keeps ang, and pos unless pos is null. it returns the handle, with
 * held, pushed and stick_ang as she works it. null, with the handle as it
 * was, when the game's character isn't Amy or player 0 has no task. null,
 * with held and pushed FALSE, when player 0 has no record */
struct amy_handle *amy_offer_handle(NJS_POINT3 *pos, Angle ang);
/* the handle as the last amy_offer_handle left it. never null */
struct amy_handle *amy_get_handle(void);

#endif
