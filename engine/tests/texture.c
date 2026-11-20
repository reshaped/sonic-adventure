/* the game's texture lists, src/system/texture.c, on a host. Ice Cap loads
 * its stage's lists by texture_load_stage from a table of the test's own.
 * texture_release_stage_within then releases those a load would write over,
 * and texture_release_stage the rest as the stage ends. engine-test runs
 * this on x86-64 Linux. it returns 0, or prints the line of the first check
 * that fails and returns 1 */
#include <stdio.h>

#include <ninja.h>

#include "stage/stage.h"
#include "stg08/pvms.h"
#include "system/texture.h"

/* two ranges of bytes a load could write over. the first runs from the end
 * of before to the start of after. inside lies within it, and so do the
 * names of named. the second runs from the end of the names of under to the
 * start of those of over. before, inside and after keep their names in
 * elsewhere. named, under and over keep their records outside memory */
static NJS_TEXNAME elsewhere[1];
static struct {
    NJS_TEXLIST before;
    NJS_TEXLIST inside;
    NJS_TEXNAME named_names[1];
    NJS_TEXLIST after;
    NJS_TEXNAME under_names[1];
    NJS_TEXNAME gap[1];
    NJS_TEXNAME over_names[1];
} memory = {
    .before = {elsewhere, 1},
    .inside = {elsewhere, 1},
    .after = {elsewhere, 1},
};
static NJS_TEXLIST named = {memory.named_names, 1};
static NJS_TEXLIST under = {memory.under_names, 1};
static NJS_TEXLIST over = {memory.over_names, 1};

struct texture_pvm icecap_pvms[] = {
    /* the stage's slots, each empty */
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    /* its lists after them, and the null that ends them */
    {NULL, &memory.before},
    {NULL, &memory.inside},
    {NULL, &named},
    {NULL, &under},
    {NULL, &over},
    {NULL, &memory.after},
    {NULL, NULL},
};

Uint16 stage_get_stage_act(void)
{
    return STAGE_ACT(STAGE_ICECAP, 0);
}

Sint32 njLoadTexture(NJS_TEXLIST *texlist)
{
    (void)texlist;
    return 1;
}

/* the lists released, in the order of their releases */
#define RELEASES 8
static NJS_TEXLIST *released[RELEASES];
static int releases;

Sint32 njReleaseTexture(NJS_TEXLIST *texlist)
{
    if (releases < RELEASES) {
        released[releases] = texlist;
    }
    releases++;
    return 1;
}

#define CHECK(test)                                                            \
    do {                                                                       \
        if (!(test)) {                                                         \
            return __LINE__;                                                   \
        }                                                                      \
    } while (0)

static int check(void)
{
    const Uint8 *start = (const Uint8 *)(&memory.before + 1);
    const Uint8 *end = (const Uint8 *)&memory.after;

    texture_load_stage();
    texture_release_stage_within(start, end);
    CHECK(releases == 2);
    CHECK(released[0] == &memory.inside && released[1] == &named);
    /* a second release finds none of the lists that stay among the bytes */
    texture_release_stage_within(start, end);
    CHECK(releases == 2);
    /* the names of under and over meet the second range at its ends alone */
    texture_release_stage_within((const Uint8 *)(memory.under_names + 1),
                                 (const Uint8 *)memory.over_names);
    CHECK(releases == 2);
    /* the stage's own release takes the four that stay, in their order, and
     * stops at the null after them */
    texture_release_stage();
    CHECK(releases == 6);
    CHECK(released[2] == &memory.before && released[3] == &under);
    CHECK(released[4] == &over && released[5] == &memory.after);
    return 0;
}

int main(void)
{
    int line = check();

    if (line != 0) {
        fprintf(stderr, "texture: line %d\n", line);
    }
    return line != 0;
}
