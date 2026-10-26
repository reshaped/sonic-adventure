/* the texture records as Ninja's texture loads search and enter them, and the
 * disc sectors they read files by */
#ifndef NJTEXMEM_H
#define NJTEXMEM_H

/* where a name's global index stands in the texture records */
#define OTHER_BANK 0 /* only in records of other palette banks */
#define LISTED 1 /* in a record, which holds a palettized name's bank */
#define UNLISTED 2 /* in no record */

/* the global index and bank of a free record */
#define TEXMEM_FREE 0xffffffff

/* a disc sector's bytes, GDD_FS_SCTSIZE, as a shift */
#define SECTOR_SHIFT 11

/* the record whose address *handle holds */
#define RECORD(handle) ((NJS_TEXMEMLIST *)*(handle))

#endif
