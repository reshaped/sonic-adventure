/* loading and releasing the texture lists of the characters, the regular
 * sets, the stages and their acts. five slots keep a regular set's first
 * list and the current stage's first four. texture_set_regular makes the
 * regular slot's list Ninja's current one, and texture_set_stage the first
 * stage slot's */
#ifndef SYSTEM_TEXTURE_H
#define SYSTEM_TEXTURE_H
#include <ninja.h>
#include <sg_xpt.h>

/* a texture list and the PVM that fills it, named without its .PRS suffix.
 * with a null name, njLoadTexture loads the list's textures by their own
 * names. a list of these ends at a null texlist. an act's list has no such
 * end: its act_textures counts its entries */
struct texture_pvm {
    char *name;
    NJS_TEXLIST *texlist;
};

/* an act's texture lists, loaded for its first user and released after its
 * last */
struct act_textures {
    Uint16 stage_act; /* STAGE_ACT(stage, act) */
    Sint8 count; /* the act's entries, from the one pvms points at */
    Sint8 users;
    struct texture_pvm *pvms;
};

/* the users of each character's texture lists. ADVERTISE's previews count
 * their own lists for a character here too */
extern Sint8 character_users[];

/* load the character's texture lists for its first user, and count the
 * user. texture_release_character counts one off, and releases the lists
 * after the last; with no users it does nothing. character must be below
 * CHARACTER_COUNT, unchecked */
void texture_load_character(int character);
void texture_release_character(int character);
/* release every character's lists that have users, and zero each count */
void texture_release_characters(void);
/* load the regular set numbered set, which must be 0 to 2, unchecked. 0
 * and 1 load the regular pvms, and 2 the shooting stages' two. the set's
 * first list takes the regular slot. its other lists load too, but
 * texture_load_regular leaves none of them recorded. texture_release_regular
 * then releases only the slot's list */
void texture_load_regular(int set);
void texture_release_regular(void);
/* load the current stage's texture lists: its first four into the stage's
 * slots, and the rest after them. a null list leaves its slot empty. most
 * stages' lists lie in their module, so release them before another module
 * replaces it */
void texture_load_stage(void);
/* make the regular slot's list Ninja's current texture list, and return
 * TRUE. FALSE, with the current list left as it was, when the slot is
 * empty. texture_set_stage does the same with the stage's first slot */
Bool texture_set_regular(void);
Bool texture_set_stage(void);
/* TRUE when the stage's first slot holds a list */
Bool texture_has_primary_stage_texlist(void);
/* release the lists in the stage's slots, and the first list after them.
 * the rest stay loaded: texture_release_stage never steps past the first */
void texture_release_stage(void);
/* load the act's texture lists for its first user, and count the user.
 * FALSE for an act without lists here, highway's among them, and TRUE
 * otherwise */
Bool texture_load_act(Uint16 stage_act);
/* TRUE when the act's lists have a user */
Bool texture_act_in_use(Uint16 stage_act);
/* count a user off the act, and release its lists after the last. FALSE
 * for an act without lists here, and TRUE otherwise. the act must have a
 * user: releasing one with none hangs, in an endless loop */
Bool texture_release_act(Uint16 stage_act);
/* release every act's lists that have users, and zero each count */
void texture_release_acts(void);
/* load name.PRS, a PVM, and copy its first texlist->nbTexture textures
 * into texlist, by position. the PVM loads into a scratch list of 512
 * textures, and nbTexture must not pass 512, unchecked. an entry the PVM
 * doesn't fill takes what an earlier load left in the scratch list. every
 * entry does when the file won't open after three tries. the file lands at
 * the end of WORK_AREA, and its last sector can write 1792 bytes past
 * WORK_AREA_END. a read that fails three times goes unchecked, and the PVM
 * then loads from whatever that memory held. the PVM decompresses at
 * WORK_AREA's start, and nothing checks that it stays clear of the file */
void texture_load_pvm(char *name, NJS_TEXLIST *texlist);
/* load the PRS file name, a PVM, into texlist. name is the whole file name,
 * suffix and all. the file lands just below WORK_AREA, in the megabyte from
 * MOTION_AREA, and its last sector can write 1792 bytes into WORK_AREA. a
 * read that fails three times goes unchecked, as in texture_load_pvm. the
 * PVM decompresses at MODULE_AREA, over any module there. -1 when the open
 * fails three times, and njLoadTexturePvmMemory's result otherwise */
Sint32 texture_load_prs_file(char *name, NJS_TEXLIST *texlist);

#endif
