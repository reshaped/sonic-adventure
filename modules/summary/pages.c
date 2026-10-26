/* the story so far: the page of a story the player has reached */
#include <sg_xpt.h>

#include "event/play.h"
#include "progress/progress.h"
#include "summary/summary.h"

/* @hack type: summary_scripts with its bound, which summary.h leaves out.
 * with the bound in scope, SHC sign-extends the script's offset as a byte
 * (EXTS.B at 0x8c900840); through summary.h's declaration alone, 50 of 82
 * words differ */
/* NOLINTNEXTLINE(readability-redundant-declaration) */
extern struct summary_script summary_scripts[7];

/* the event flag each story sets as it ends */
static Uint16 summary_end_flags[] = {
    FLAG_CLEAR_SONIC, FLAG_CLEAR_TAILS, FLAG_CLEAR_KNUCKLES, FLAG_CLEAR_AMY,
    FLAG_CLEAR_BIG,   FLAG_CLEAR_E102,  FLAG_CLEAR_LAST,
};

/* a page's flag without its SUMMARY_PLAYED bit: an event's number where
 * that bit is set, and else an event flag */
#define SUMMARY_FLAG(flag) ((flag) & (SUMMARY_PLAYED - 1))

int summary_find_page(int story)
{
    struct summary_script *script;
    /* @hack spelling: number, a copy of story to index the scripts with,
     * which SHC keeps in a frame slot of its own (MOV.L R3,@(4,R15) at
     * 0x8c90081e); indexing by story itself, 73 of 82 words differ */
    int number = story;
    int page = -1;
    int i;
    Uint16 flag;

    if (event_flag_check(summary_end_flags[story])) {
        return -1;
    }
    script = &summary_scripts[number];
    for (i = script->count - 1; i >= 0; i--) {
        flag = script->flags[i];
        if (flag == SUMMARY_NEVER) {
            continue;
        }
        if (flag & SUMMARY_PLAYED) {
            if (event_was_played(SUMMARY_FLAG(flag))) {
                page = i;
                break;
            }
        } else if (event_flag_check(SUMMARY_FLAG(flag))) {
            page = i;
            break;
        }
        page = -1;
    }
    return page;
}
