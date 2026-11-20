/* for a source whose loads are all of what the game can't go on without: a
 * program it then runs, or the data a stage, a character or a demo plays
 * from. the source includes this after its other headers. under NONMATCHING
 * its loader_read_prs and loader_read_file are then loader.h's
 * loader_must_read_prs and loader_must_read_file, which don't return from a
 * file that won't open. the calls keep their shipped spelling, which the
 * build's tools read */
#ifndef SYSTEM_MUST_LOAD_H
#define SYSTEM_MUST_LOAD_H
#include "system/loader.h"

#ifdef NONMATCHING
#define loader_read_prs loader_must_read_prs
#define loader_read_file loader_must_read_file
#endif

#endif
