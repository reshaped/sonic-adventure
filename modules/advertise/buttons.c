/* the menus' buttons: A or Start chooses, and B or X backs out */
#include <shinobi.h>

#include "advertise/buttons.h"
#include "system/pad.h"

int pad_choose_pressed(void)
{
    int press = pad_press_repeat[0];

    return (press & PDD_DGT_TA) | (press & PDD_DGT_ST);
}

int pad_back_pressed(void)
{
    int press = pad_press_repeat[0];

    return (press & PDD_DGT_TB) | (press & PDD_DGT_TX);
}
