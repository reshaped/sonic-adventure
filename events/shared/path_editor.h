/* an event path editor: keys placed at a cursor in the world, edited
 * through menus and pad shortcuts */
#ifndef EVENT_PATH_EDITOR_H
#define EVENT_PATH_EDITOR_H

/* run the editor until EXIT, from no keys, yielding the thread once a
 * pass. it reads per_info[0]'s buttons and moves the event camera. the
 * cursor starts 10 above player_work[0], which must be set. name names the
 * file, name.ept, and the table and tag in it, in 27 characters at most.
 * room for 512 keys comes from the heap, unchecked. adding, subdividing or
 * loading more keys writes past that room. SAVE and LOAD put the file's
 * text at MOTION_AREA, over the character's motions. each waits on the
 * development host as host_save_file and host_load_file do */
void path_editor_run(const char *name);

#endif
