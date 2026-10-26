/* the lists of the stage's paths for acts 0 and 1 */
#include "stg01/paths.h"
#include "path/path.h"

extern struct path dat_0caa9948;
extern struct path dat_0caa9f48;
extern struct path dat_0caaa200;
extern struct path dat_0caaa6c0;
extern struct path path_orca_path;
extern struct path dat_0caab6b8;
extern struct path dat_0caabb28;
extern struct path dat_0caabfc0;
extern struct path dat_0caac87c;
extern struct path dat_0caad23c;
extern struct path dat_0caad800;
extern struct path dat_0caae10c;

struct path *beach_act0_paths[6] = {
    &dat_0caa9948, &dat_0caa9f48,   &dat_0caaa200,
    &dat_0caaa6c0, &path_orca_path, NULL,
};
struct path *beach_act1_paths[8] = {
    &dat_0caab6b8, &dat_0caabb28, &dat_0caabfc0, &dat_0caac87c,
    &dat_0caad23c, &dat_0caad800, &dat_0caae10c, NULL,
};
