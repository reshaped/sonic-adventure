/* the list of paths 1ST_READ.BIN's path_lists names for act 2 */
#include "stg10/act2_paths.h"
#include "path/path.h"

/* act 2's two paths, which the stage's generated data defines */
extern struct path dat_0cafd790;
extern struct path dat_0cafd2a8;

/* path_create_tasks stops at the null */
struct path *final_act2_paths[3] = {
    &dat_0cafd790,
    &dat_0cafd2a8,
    NULL,
};
