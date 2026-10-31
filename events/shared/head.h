/* an event module's head. section HEAD places it at the load address, where
 * src/event/play.c reads it. each event's head.c defines four names, then
 * includes this. EVENT_INCLUDE is the header that declares the event's entry
 * and texture list. EVENT_HEAD, EVENT_ENTRY and EVENT_TEXTURES name the head,
 * the entry and the texture list */
#include EVENT_INCLUDE
#include "event/play.h"

#pragma section HEAD
#ifdef GCC
/* only the head's record in the memory contract names it, so GCC keeps it as
 * used */
static struct event_module EVENT_HEAD __attribute__((used));
#endif
static struct event_module EVENT_HEAD = {EVENT_ENTRY, EVENT_TEXTURES};
#pragma section
