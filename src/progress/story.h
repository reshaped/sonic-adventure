/* story: each character's sections, and the scripts in the field modules
 * they run */
#ifndef PROGRESS_STORY_H
#define PROGRESS_STORY_H
#include <sg_xpt.h>

/* a section's script, which lies in a field module that must be loaded
 * while either function is called. story_start_act calls init as an act
 * starts in the section's field, and story_step calls step with the record's
 * seq */
struct story_script {
    void (*init)(void);
    void (*step)(int seq);
};

/* a section of a story: its script, and the stage, act and entrance where
 * it begins */
struct story_section {
    struct story_script *script;
    Uint8 stage;
    Uint8 act;
    Sint16 entrance;
    char name[32]; /* as the SEQ EDITOR lists it */
};

/* the character's story table, and its length in sections. while
 * last_story is set, both give the last story's, whatever the character. a
 * character with no story gives NULL and 0 */
struct story_section *story_get_table(int character);
int story_get_length(int character);

#endif
