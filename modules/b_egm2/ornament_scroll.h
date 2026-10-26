/* the ornament's texture scrolls. B_EGM2's O KAZARI and ADV00's casino
 * ornament are copies of one object. each has an ornament_scroll_step of its
 * own */
#ifndef B_EGM2_ORNAMENT_SCROLL_H
#define B_EGM2_ORNAMENT_SCROLL_H
#include <ninja.h>
#include <sg_xpt.h>

/* count UVs from uvs, which a scroll moves together */
struct uv_group {
    NJS_COLOR *uvs;
    int count;
};

/* an ornament_scroll's flags. the scroll moves the UVs' u with SCROLL_U, or
 * else their v with SCROLL_V. it moves them by speed with SCROLL_FORWARD,
 * and by -speed without */
#define SCROLL_U 0x2
#define SCROLL_V 0x4
#define SCROLL_FORWARD 0x8

/* the UVs of count groups, which ornament_scroll_step moves once every
 * delay + 1 calls. timer counts the calls since the last move. each move
 * adds speed to phase, and moves the UVs by speed as the flags say. once
 * phase passes 255, the move takes 256 off both. so the UVs lie as far from
 * where they began as phase lies from its start. with a positive speed,
 * that is under 256 */
struct ornament_scroll {
    struct uv_group *groups;
    Sint16 count;
    Sint16 flags;
    Sint16 speed;
    Sint16 delay;
    Sint16 timer;
    Sint16 phase;
};

#endif
