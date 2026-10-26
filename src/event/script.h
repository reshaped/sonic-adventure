/* message scripts, which pick what a stage's hints and a field's people say
 * by the story so far, and the message window that shows it */
#ifndef EVENT_SCRIPT_H
#define EVENT_SCRIPT_H
#include <sg_xpt.h>

/* a page of a message. a run of pages ends at a page whose text is null */
struct script_page {
    char *text;
    Sint32 steps; /* to show for, or 0 to wait for a jump or grab button */
};

/* a script's words. a script is a list of branches, each ended by
 * SCRIPT_BRANCH and the last by SCRIPT_END. a branch picks its run when all
 * its tests pass, and a failed test moves on to the next branch. a test of
 * a list passes when any entry matches. a word these don't name, where one
 * of them belongs, stops the script, which then picks nothing */
enum script_word {
    SCRIPT_END = -1,
    SCRIPT_BRANCH = -2,
    SCRIPT_FLAG = -3, /* then the flag to set or clear */
    SCRIPT_VOICE = -4, /* then the voice */
    SCRIPT_CHARACTERS = -5, /* then a bit per character the branch is for */
    SCRIPT_EVENTS = -6, /* then events, until a word below 0 */
    SCRIPT_FLAGS = -7 /* then flags, until a word below 0 */
};

/* in a script, a flag with this bit means set and an event with it means
 * played. without the bit, they mean clear and not played */
#define SCRIPT_ON 0x4000

/* a row of a script table. a null script passes at once. runs holds a run
 * of pages or of lines for each branch, one after another, each with at
 * least one before its null. a row with null runs picks none */
struct script_row {
    Sint16 *script;
    void *runs;
};

/* a field's message file, which the field loads whole past its module's
 * sections and whose table it gives script_set_lines_table */
struct field_messages {
    /* the rows of the people the set places. the walkers' rows follow */
    Sint32 people;
    struct script_row *table;
};

/* what a row's script picks */
struct script_pages {
    struct script_page *pages; /* or null for none */
    Sint32 voice; /* or -1 for none */
    Sint32 flag; /* to set or clear, or -1 for none */
};

/* what a row's script picks from a table of lines */
struct script_lines {
    /* or null for none. each line shows until a jump or grab button */
    char **lines;
    Sint32 flag; /* as script_pages' */
};

/* a message shows a run of pages or lines in the message window, on a
 * thread of its own. starting one stops the voice playing and cuts the
 * message showing short. the new message shows once the window is free,
 * and its pages or lines have to last until it ends */

/* show lines, which end at a null, 60 steps each */
void message_show_timed(char **lines);
/* the next message started sets *shown to 1 at once, and to 0 as it ends */
void message_set_shown(Uint8 *shown);
/* set the field's table of lines, and the table script_pick runs for a
 * stage without its own. each also forgets the message holding the window,
 * and drops a cut asked for. that message's thread runs on, and the next
 * message started need not wait for it */
void script_set_lines_table(struct script_row *table);
void script_set_table(struct script_row *table);
/* run the script of row in the stage's table, or in the field's for a stage
 * without one, and put what it picks in *pick. return TRUE when a branch
 * passes, though it may pick no pages. pick's voice and flag are the last
 * the script read, which may be in a branch that failed. with no table,
 * return FALSE and leave *pick as it was */
Bool script_pick(int row, struct script_pages *pick);
/* the same for row of the field's table of lines. unless the script reads a
 * SCRIPT_FLAG, lines->flag is a stack word no path wrote */
Bool script_pick_lines(int row, struct script_lines *lines);
/* start the voice and the message a script picked, where it picked them,
 * and set or clear its flag at once. a voiced message's window hides while
 * messages_shown is clear */
void script_show(struct script_pages *pick);
void script_show_lines(struct script_lines *lines);
/* TRUE while a voice plays or a message holds the window. a voice that
 * has played to its end is stopped */
Bool message_busy(void);
/* cut the message showing short, and stop the voice playing. a message
 * waiting for the window still shows */
void message_cut(void);

#endif
