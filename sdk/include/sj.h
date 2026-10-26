/* CRI stream joints, as far as the sources use them */
#ifndef SJ_H
#define SJ_H
#include <sg_xpt.h>

/* a joint's two lines: free space to write into, and data to read */
#define SJ_LIN_FREE 0
#define SJ_LIN_DATA 1

/* error codes passed to a joint's error function: an internal inconsistency,
 * and an invalid parameter */
#define SJ_ERR_INTERNAL (-2)
#define SJ_ERR_PRM (-3)

#ifndef UUID_DEFINED
#define UUID_DEFINED
typedef struct _UUID {
    Uint32 Data1;
    Uint16 Data2;
    Uint16 Data3;
    Uint8 Data4[8];
} UUID;
#endif

/* a chunk: len bytes at data, on one of a joint's lines */
typedef struct {
    char *data;
    Sint32 len;
} SJCK;

/* a stream joint, a buffer that passes chunks between a writer and a reader. it
 * begins with its kind's function table */
typedef struct {
    struct _sj_vtbl *vtbl;
} SJ_OBJ;
typedef SJ_OBJ *SJ;

/* a joint's function table; id is a line, SJ_LIN_FREE or SJ_LIN_DATA */
typedef struct _sj_vtbl {
    void (*QueryInterface)();
    void (*AddRef)();
    void (*Release)();
    void (*Destroy)(SJ sj);
    UUID *(*GetUuid)(SJ sj);
    void (*Reset)(SJ sj);
    void (*GetChunk)(SJ sj, Sint32 id, Sint32 nbyte, SJCK *ck);
    void (*UngetChunk)(SJ sj, Sint32 id, SJCK *ck);
    void (*PutChunk)(SJ sj, Sint32 id, SJCK *ck);
    Sint32 (*GetNumData)(SJ sj, Sint32 id);
    Sint32 (*IsGetChunk)(SJ sj, Sint32 id, Sint32 nbyte, Sint32 *rbyte);
    void (*EntryErrFunc)(SJ sj, void (*func)(void *obj, Sint32 ecode),
                         void *obj);
} SJ_IF;

#define SJ_Destroy(sj) (*(sj)->vtbl->Destroy)(sj)
#define SJ_Reset(sj) (*(sj)->vtbl->Reset)(sj)
#define SJ_GetNumData(sj, id) (*(sj)->vtbl->GetNumData)((sj), (id))

/* split *ck after nbyte bytes, or at its end when shorter: *ck1 gets the
 * first part and *ck2 the rest. ck1 and ck2 must differ, and either may be
 * ck. mwSjSplitChunk only calls SJ_SplitChunk */
void SJ_SplitChunk(SJCK *ck, Sint32 nbyte, SJCK *ck1, SJCK *ck2);
void mwSjSplitChunk(SJCK *ck, Sint32 nbyte, SJCK *ck1, SJCK *ck2);

/* ring buffer joints: bsize bytes at buf, followed by xsize extra bytes that
 * mirror the buffer's start, so that a chunk can run past its end. SJRBF_Create
 * returns NULL when all 32 are in use */
void SJRBF_Init(void);
void SJRBF_Finish(void);
SJ SJRBF_Create(char *buf, Sint32 bsize, Sint32 xsize);
char *SJRBF_GetBufPtr(SJ sj);
Sint32 SJRBF_GetBufSize(SJ sj);
Sint32 SJRBF_GetXtrSize(SJ sj);

/* memory joints: bsize bytes at data, read through once. SJMEM_Create returns
 * NULL when all 32 are in use */
void SJMEM_Init(void);
SJ SJMEM_Create(char *data, Sint32 bsize);

#endif
