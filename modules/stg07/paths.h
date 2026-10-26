/* the stage's paths, and the lists of them for acts 0 and 1 */
#ifndef STG07_PATHS_H
#define STG07_PATHS_H
#include "path/path.h"

/* the boulder's path and the slide's, which boulder.c and slide.c sample by
 * name */
extern struct path ruin_boulder_path;
extern struct path ruin_slide_path;
/* the lists 1ST_READ.BIN's path_lists names for the stage's acts 0 and 1 */
extern struct path *ruin_act0_paths[];
extern struct path *ruin_act1_paths[];

#endif
