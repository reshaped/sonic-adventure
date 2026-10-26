/* GD file system directory reads and searches */
#include <sg_xpt.h>
#include <shinobi.h>

#include "shinobi/gdfs.h"

/* byte offsets in an ISO 9660 directory record. the record holds its
 * extent and size in both byte orders. DR_EXTENT and DR_SIZE are the
 * little-endian halves */
#define DR_LEN 0
#define DR_EXTENT 2
#define DR_SIZE 10
#define DR_FLAGS 25
#define DR_NAME_LEN 32
#define DR_NAME 33
/* the shortest record with a name */
#define DR_MIN_LEN 34
/* the one-byte names of a directory's own record and its parent's */
#define DR_NAME_SELF 0
#define DR_NAME_PARENT 1
/* the bytes of the version that ends a file's name: ;1 */
#define VERSION_LEN 2

/* a record of the type L ISO 9660 path table, whose numbers are
 * little-endian */
struct path_record {
    Uint8 name_len;
    Uint8 ext_len; /* the extended attribute record's length */
    Uint8 extent[4]; /* the directory's first sector */
    Uint8 parent[2]; /* the parent's record number, from 1 */
    /* name_len bytes, with no NUL, and a pad byte after an odd name_len */
    char name[1];
};
/* bytes before a path table record's name */
#define PT_HEADER 8

static char name_upper(char c)
{
    if (c >= 'a' && c <= 'z') {
        c -= 'a' - 'A';
    }
    return c;
}

void gdFsMemcpy(const void *src, void *dst, Sint32 size)
{
    const char *from = src;
    char *to = dst;

    while (size > 0) {
        *to++ = *from++;
        size--;
    }
}

/* whether name and str match in their first size characters, ignoring
 * case. where name ends sooner, str must end at the same place */
static Bool name_starts(const char *name, const char *str, Sint32 size)
{
    while (size > 0) {
        if (*name != '\0') {
            if (name_upper(*name++) != name_upper(*str++)) {
                return FALSE;
            }
        } else {
            if (*str == '\0') {
                return TRUE;
            }
            return FALSE;
        }
        size--;
    }
    return TRUE;
}

static Bool name_same(const char *name, const char *str)
{
    while (*name != '\0') {
        if (*name++ != *str++) {
            return FALSE;
        }
    }
    if (*str != '\0') {
        return FALSE;
    }
    return TRUE;
}

static Bool name_same_any_case(const char *name, const char *str)
{
    while (*name != '\0') {
        if (name_upper(*name++) != name_upper(*str++)) {
            return FALSE;
        }
    }
    if (*str != '\0') {
        return FALSE;
    }
    return TRUE;
}

/* the length of the name that starts path: its characters up to the first
 * that is in sep, or to its end */
static Sint32 name_len(const char *path, const char *sep)
{
    const char *s;
    Sint32 len = 0;

    while (*path != '\0') {
        for (s = sep; *s != '\0'; s++) {
            if (*path == *s) {
                return len;
            }
        }
        path++;
        len++;
    }
    return len;
}

/* a name's search ID: one byte, the same for names that differ only in
 * case. gdFsSearchName compares IDs before it compares names */
static Sint32 name_sid(const char *name)
{
    Uint32 sum = 218;

    while (*name != '\0') {
        sum += (Uint8)name_upper(*name++);
    }
    return (Uint8)(sum >> 2);
}

/* copy the directory record at dr into ent, and return the record's length.
 * it returns 0 for a length byte below DR_MIN_LEN, as the zeros after a
 * sector's last record are. ent is then left as it was */
static Sint32 read_dirrec(Uint8 *dr, GDFS_DIRREC_ENT *ent)
{
    Sint32 len;

    if (dr[DR_LEN] < DR_MIN_LEN) {
        return 0;
    }
    gdFsMemcpy(dr + DR_EXTENT, &ent->sector, 4);
    gdFsMemcpy(dr + DR_SIZE, &ent->fsize, 4);
    ent->flag = dr[DR_FLAGS];
    len = dr[DR_NAME_LEN];
    if (!(ent->flag & GDD_FF_DIRECTORY)) {
        len -= VERSION_LEN;
    }
    gdFsMemcpy(dr + DR_NAME, ent->fname, len);
    ent->fname[len] = '\0';
    if (ent->flag & GDD_FF_DIRECTORY) {
        if (ent->fname[0] == DR_NAME_SELF) {
            ent->fname[0] = '.';
        } else if (ent->fname[0] == DR_NAME_PARENT) {
            ent->fname[0] = '.';
            ent->fname[1] = '.';
            ent->fname[2] = '\0';
        }
    }
    ent->sid = name_sid((const char *)ent->fname);
    return dr[DR_LEN];
}

Sint32 gdFsGetDirEnt(Uint8 *buf, Sint32 fad, GDFS_DIRREC gf_dirrec, Sint32 size)
{
    Uint8 *dr;
    GDFS_DIRREC_ENT *ent = gf_dirrec->dirrec_tbl;
    Uint8 *end;
    Sint32 num;
    Sint32 max_ent;
    Sint32 result;

    if (gf_dirrec->dir_fad == fad) {
        return GDD_ERR_OK;
    }
    gf_dirrec->dir_fad = fad;
    num = 0;
    max_ent = gf_dirrec->max_ent;
    end = buf + GDD_FS_SCTSIZE;
    while (size > 0) {
        result = gdFsReadW(fad, 1, buf);
        if (result != GDD_ERR_OK) {
            gf_dirrec->dir_fad = 0;
            return result;
        }
        dr = buf;
        while (num < max_ent) {
            /* @hack spelling: the length is assigned in the test. SHC tests it
             * in R0 (TST R0,R0 at 0x8c65ebc8), and copies it in the delay
             * slot. assigned first, it is copied to R4 and tested there, ent
             * and dr trade R12 and R13, and 15 of 99 words differ */
            if ((result = read_dirrec(dr, ent)) == 0) {
                break;
            }
            if (num == 0 && ent->fsize != size) {
                size = ent->fsize;
            }
            dr += result;
            ent++;
            num++;
            if (dr >= end) {
                break;
            }
        }
        if (num >= max_ent) {
            break;
        }
        size -= GDD_FS_SCTSIZE;
        fad++;
    }
    gf_dirrec->dir_num = num;
    /* a full table holds the whole directory only when its last entry is
     * the last sector's last record */
    if (num == max_ent && size != 0) {
        if (size != GDD_FS_SCTSIZE) {
            /* @hack spelling: the goto into the test's body reloads end into R2
             * (MOV.L @(8,R15),R2 at 0x8c65ec08). as one test with ||, it is
             * reloaded into R3, and 2 of 99 words differ */
            goto over;
        }
        if (dr < end) {
            if (*dr != 0) {
            over:
                gf_dirrec->dir_fad = 0;
                return GDD_ERR_DIROVER;
            }
        }
    }
    return GDD_ERR_OK;
}

Sint32 gdFsSearchName(const char *name, GDFS_DIRREC gf_dirrec, Bool exact)
{
    Sint32 dir_num = gf_dirrec->dir_num;
    GDFS_DIRREC_ENT *ent = gf_dirrec->dirrec_tbl;
    Uint8 sid = name_sid(name);
    Sint32 i;

    for (i = 0; i < dir_num; i++, ent++) {
        if (ent->sid == sid) {
            /* @hack spelling: both branches compare the name test's result
             * with TRUE, which SHC does by CMP/EQ #1,R0 (0x8c65ec6e,
             * 0x8c65ec7e). name_same and name_same_any_case return only TRUE
             * or FALSE, so a plain test of the result is equal. tested
             * plainly, the results take TST R0,R0, and 4 of 56 words differ */
            if (exact == TRUE) {
                if (name_same(name, (const char *)ent->fname) == TRUE) {
                    return i + 1;
                }
            } else {
                if (name_same_any_case(name, (const char *)ent->fname) ==
                    TRUE) {
                    return i + 1;
                }
            }
        }
    }
    return 0;
}

Sint32 gdFsSearchPath(Uint8 *buf, Sint32 fad, Sint32 size, const char *path)
{
    Sint32 parent;
    Sint32 no;
    Sint32 len;
    const char *name;
    Uint8 *pt;
    Uint8 *end;
    struct path_record *cur;
    struct path_record *rec;
    Uint16 rec_parent;
    Sint32 result;
    Sint32 step;
    Sint32 done;

    name = path + 1;
    pt = buf + GDD_FS_SCTSIZE;
    no = 1;
    parent = 1;
    len = 0;
    done = 0;
    /* @hack spelling: end is taken through cur. SHC then keeps it in the
     * frame's first slot (MOV.L R14,@R15 at 0x8c65ecd0). taken from pt, it is
     * kept at @(8,R15), fad moves to @(4,R15), the function is 4 bytes shorter,
     * and 83 of 133 words differ */
    cur = (struct path_record *)pt;
    end = (Uint8 *)cur;
    for (;;) {
        cur = (struct path_record *)pt;
        if (pt >= end ||
            cur->name_len + (cur->name_len % 2) + PT_HEADER + pt >= end) {
            step = buf + GDD_FS_SCTSIZE - pt;
            cur = (struct path_record *)(buf - step);
            gdFsMemcpy(pt, cur, step);
            result = gdFsReadW(fad, 1, buf);
            if (result != GDD_ERR_OK) {
                break;
            }
            pt = (Uint8 *)cur;
            fad++;
        }
        /* @hack spelling: rec is a second pointer to the record, which keeps
         * pt in R14 and the record in R13 (0x8c65ed16, 0x8c65ed94). through
         * cur alone, the two trade, and 20 of 133 words differ */
        rec = cur;
        gdFsMemcpy(rec->parent, &rec_parent, 2);
        if (rec_parent == parent &&
            (no == 1 ||
             (rec->name_len == len && name_starts(rec->name, name, len)))) {
            name += len;
            parent = no;
            if (*name == '\0') {
                return gdFsGetLE4(rec->extent);
            }
            if (no != 1) {
                name++;
            }
            len = name_len(name, "/\\");
        }
        no++;
        /* @hack spelling: the step and the count are one statement, which loads
         * the name's length into R5 (MOV.B @R13,R5 at 0x8c65ed74). as two
         * statements, it is loaded into R4, and 5 of 133 words differ */
        step = rec->name_len + (rec->name_len % 2) + PT_HEADER, done += step;
        pt += step;
        if (done >= size) {
            break;
        }
    }
    return 0;
}

Sint32 gdFsSearchPath_dds(Uint8 *buf, Sint32 fad, Sint32 size, const char *path,
                          const char *first_name)
{
    Sint32 parent;
    Sint32 no;
    Sint32 done;
    Sint32 len;
    Bool first;
    Sint32 first_len;
    Sint32 want;
    const char *match;
    const char *s;
    const char *name;
    Uint8 *pt;
    Uint8 *end;
    struct path_record *cur;
    struct path_record *rec;
    Uint16 rec_parent;
    Sint32 step;
    Sint32 result;

    name = path;
    first = TRUE;
    first_len = 0;
    s = first_name;
    while (*s++ != '\0') {
        first_len++;
    }
    name++;
    len = 0;
    parent = 1;
    no = 1;
    done = 0;
    pt = buf + GDD_FS_SCTSIZE;
    /* @hack spelling: end is taken through cur. SHC then keeps it at @(8,R15)
     * (MOV.L R14,@(8,R15) at 0x8c65ee04). taken from pt, it is kept at
     * @(20,R15), the other slots move, two locals trade R10 and R11, and 22
     * of 162 words differ */
    cur = (struct path_record *)pt;
    end = (Uint8 *)cur;
    do {
        cur = (struct path_record *)pt;
        if (pt >= end ||
            cur->name_len + (cur->name_len % 2) + PT_HEADER + pt >= end) {
            step = buf + GDD_FS_SCTSIZE - pt;
            cur = (struct path_record *)(buf - step);
            gdFsMemcpy(pt, cur, step);
            result = gdFsReadW(fad, 1, buf);
            if (result != GDD_ERR_OK) {
                break;
            }
            pt = (Uint8 *)cur;
            fad++;
        }
        /* @hack spelling: rec is a second pointer to the record, which keeps
         * pt in R14 and the record in R13 (0x8c65ee4c, 0x8c65eeda). through
         * cur alone, the two trade, the frame's slots move, and 67 of 162
         * words differ */
        rec = cur;
        gdFsMemcpy(rec->parent, &rec_parent, 2);
        if (rec_parent == parent) {
            if (no == 1) {
                parent = no;
            } else {
                if (first == TRUE) {
                    want = first_len;
                    match = first_name;
                } else {
                    want = len;
                    match = name;
                }
                if (rec->name_len == want &&
                    name_starts((const char *)rec->name, match, want)) {
                    if (!first) {
                        name += len;
                    }
                    if (*name == '\0') {
                        return gdFsGetLE4(rec->extent);
                    }
                    if (first == TRUE) {
                        first = FALSE;
                    } else if (no != 1) {
                        name++;
                    }
                    len = name_len(name, "/\\");
                    parent = no;
                }
            }
        }
        no++;
        /* @hack spelling: the step and the count are one statement, which loads
         * the name's length into R5 (MOV.B @R13,R5 at 0x8c65eeda). as two
         * statements, it is loaded into R4, and 10 of 162 words differ */
        step = rec->name_len + (rec->name_len % 2) + PT_HEADER, done += step;
        pt += step;
    } while (done < size);
    return 0;
}
