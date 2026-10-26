/* middleware manager, as far as the sources use it
 *
 * the manager runs each middleware library's server, vertical blank and frame
 * start functions */
#ifndef SG_MWMNG_H
#define SG_MWMNG_H

/* mwMngInit and mwMngFinish nest. the outermost mwMngInit empties the tables
 * and the error function, and adds mwMngVsync to the vertical blank chain at
 * MWD_PLY_PRI_VBLNK. the mwMngFinish that balances it removes the handler */
void mwMngInit(void);
void mwMngFinish(void);
/* mwMngEntryErrFn sets the function that receives error messages, and the obj
 * it is called with. mwMngError formats a message with vsprintf into a 256-byte
 * buffer, and passes it to that function, if one is set. the message and its
 * NUL must fit the buffer */
void mwMngEntryErrFn(void (*func)(void *obj, char *msg), void *obj);
void mwMngError(const char *format, ...);
/* three tables of up to 16 functions, each called with the obj it was entered
 * with, in table order. mwMngExecServer runs the server table, mwMngVsync the
 * vertical blank table and mwMngStartFrame the frame start table. each
 * mwMngEntry call takes the first empty slot of its table, and reports an error
 * through mwMngError when there is none. each mwMngDelete call empties every
 * slot of its table that holds func */
void mwMngEntryServerFunc(void (*func)(void *obj), void *obj);
void mwMngDeleteServerFunc(void (*func)(void *obj));
void mwMngExecServer(void);
void mwMngEntryVsyncFunc(void (*func)(void *obj), void *obj);
void mwMngDeleteVsyncFunc(void (*func)(void *obj));
void mwMngVsync(void *obj);
void mwMngEntryStartFrameFunc(void (*func)(void *obj), void *obj);
void mwMngDeleteStartFrameFunc(void (*func)(void *obj));
void mwMngStartFrame(void);

#endif
