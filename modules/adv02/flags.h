/* event flag names, and a debug display of the flags around a cursor */
#ifndef ADV02_FLAGS_H
#define ADV02_FLAGS_H

/* a debug display of the event flags. one mode stores a waiting value in the
 * cursor's flag. another prints the 15 flags around the cursor, with their
 * names. no code calls it or sets a mode */
void flags_print(void);
/* the flag's name, from the lists of Sonic's to Big's flags, or "UNKNOWN" for
 * a flag none of them holds */
char *flags_name(int flag);

#endif
