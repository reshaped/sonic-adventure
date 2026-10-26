/* Windy Valley's script table */
#ifndef STG02_SCRIPTS_H
#define STG02_SCRIPTS_H

#include "event/script.h"

#define WINDY_SCRIPT_ROWS 7

/* 1ST_READ.BIN's stage_scripts points at this table for the stage. it
 * reaches the table by its address, 0x0ca158f0, not by its name */
extern const struct script_row windy_script_table[WINDY_SCRIPT_ROWS];

#endif
