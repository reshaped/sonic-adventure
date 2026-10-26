/* the middleware ring buffer joint library: its ID, its interfaces, and the
 * calls that start, finish and serve it */
#ifndef MOVIE_SDK_SJRBF_H
#define MOVIE_SDK_SJRBF_H
#include <sg_xpt.h>
#include <sj.h>

/* the library's UUID. the overlay library opens the library by it, and
 * mwSjRbfGetUuid returns it for each joint */
#define SJRBF_LID                                                              \
    {0x9b5f1fe1,                                                               \
     0xde44,                                                                   \
     0x11d1,                                                                   \
     {0xa6, 0xbf, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}}

/* the library's interface, as ovLibGetLibIf returns it */
struct sj_rbf_lif {
    void (*QueryInterface)(void);
    void (*AddRef)(void);
    void (*Release)(void);
    void (*Init)(void);
    void (*Finish)(void);
    void (*ExecServer)(void);
};

/* NULL until mwSjRbfInit points it at mwg_sj_rbf_vtbl, the joints' function
 * table. mwSjRbfInitOvl points it at the opened library's struct sj_rbf_lif
 * instead, which is no SJ_IF to call through */
extern SJ_IF *mwg_sj_rbf_i;

/* open the library through the overlay library, point mwg_sj_rbf_i at its
 * interface and call its Init. it does nothing while the library is open, or
 * when the overlay library can't open it */
void mwSjRbfInitOvl(void);
/* point mwg_sj_rbf_i at mwg_sj_rbf_vtbl, the function table of this image's
 * joints, and clear every joint, which frees it */
void mwSjRbfInit(void);
/* call the Finish of the library mwSjRbfInitOvl opened, close it and set
 * mwg_sj_rbf_i to NULL. with no library open it does nothing, even after
 * mwSjRbfInit */
void mwSjRbfFinish(void);
/* call the ExecServer of the struct sj_rbf_lif at mwg_sj_rbf_i, which must be
 * the interface mwSjRbfInitOvl set. with mwg_sj_rbf_i NULL it does nothing */
void mwSjRbfExecServer(void);

#endif
