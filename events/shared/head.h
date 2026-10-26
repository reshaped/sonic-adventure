/* an event module's head. section HEAD places it at the load address, where
 * src/event/play.c reads it. each event's head.c defines four names, then
 * includes this. EVENT_INCLUDE is the header that declares the event's entry
 * and texture list. EVENT_HEAD, EVENT_ENTRY and EVENT_TEXTURES name the head,
 * the entry and the texture list */
#include EVENT_INCLUDE
#include "event/play.h"

#pragma section HEAD
static struct event_module EVENT_HEAD = {EVENT_ENTRY, EVENT_TEXTURES};
#pragma section
