/* a table of 32 objects, each a single node with a model. no word in the
 * image or in 1ST_READ.BIN points into the table. only the table points at
 * the objects */
#include <ninja.h>

#include "stg06/object_table.h"

extern NJS_OBJECT dat_0cac2874;
extern NJS_OBJECT dat_0cac2a5c;
extern NJS_OBJECT dat_0cac2d50;
extern NJS_OBJECT dat_0cac3044;
extern NJS_OBJECT dat_0cac322c;
extern NJS_OBJECT dat_0cac360c;
extern NJS_OBJECT dat_0cac4cf4;
extern NJS_OBJECT dat_0cac5e18;
extern NJS_OBJECT dat_0cac6da8;
extern NJS_OBJECT dat_0cac7780;
extern NJS_OBJECT dat_0cac7ae8;
extern NJS_OBJECT dat_0cac7cd0;
extern NJS_OBJECT dat_0cac7f0c;
extern NJS_OBJECT dat_0cac8868;
extern NJS_OBJECT dat_0cac8c38;
extern NJS_OBJECT dat_0cac90c4;
extern NJS_OBJECT dat_0cac93b8;
extern NJS_OBJECT dat_0cac95fc;
extern NJS_OBJECT dat_0cac9964;
extern NJS_OBJECT dat_0cacaa80;
extern NJS_OBJECT dat_0cacac68;
extern NJS_OBJECT dat_0cacbd4c;
extern NJS_OBJECT dat_0cacc0b4;
extern NJS_OBJECT dat_0cacc2f8;
extern NJS_OBJECT dat_0cacc5ec;
extern NJS_OBJECT dat_0cacca14;
extern NJS_OBJECT dat_0caccc80;
extern NJS_OBJECT dat_0caccee4;
extern NJS_OBJECT dat_0cacd0d4;
extern NJS_OBJECT dat_0cacd24c;
extern NJS_OBJECT dat_0cacd52c;
extern NJS_OBJECT dat_0cacd790;

NJS_OBJECT *sky_deck_object_table[32] = {
    &dat_0cac2874, &dat_0cac2a5c, &dat_0cac2d50, &dat_0cac3044, &dat_0cac322c,
    &dat_0cac360c, &dat_0cac4cf4, &dat_0cac5e18, &dat_0cac6da8, &dat_0cac7780,
    &dat_0cac7ae8, &dat_0cac7cd0, &dat_0cac7f0c, &dat_0cac8868, &dat_0cac8c38,
    &dat_0cac90c4, &dat_0cac93b8, &dat_0cac95fc, &dat_0cac9964, &dat_0cacaa80,
    &dat_0cacac68, &dat_0cacbd4c, &dat_0cacc0b4, &dat_0cacc2f8, &dat_0cacc5ec,
    &dat_0cacca14, &dat_0caccc80, &dat_0caccee4, &dat_0cacd0d4, &dat_0cacd24c,
    &dat_0cacd52c, &dat_0cacd790,
};
