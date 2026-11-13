/* SA1's capture client, which engine/sa1/client.cpp gives the play
 * program. C17 and C++17 compile it alike, as they do sa_pc.h */
#ifndef SA_CLIENT_H
#define SA_CLIENT_H

#include "sa_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/* a capture's records (sa_capture.h), given to an engine one at a time, as
 * the game's adapter makes them. a mesh and a texture go in as
 * sa_resource_*, and a picture through sa_present_*. a resource's bytes stay
 * the client's until SA_EVENT_RETIRED. roots lists the trees the disc's
 * files are read from, ending in a null. a file comes from the first root
 * that holds it */
typedef struct sa_pc_client sa_pc_client;
sa_pc_client *sa_pc_client_create(sa_engine *engine, const char *const *roots);
/* take one record. where the record ends a picture, it returns SA_PC_ENDED,
 * or SA_PC_QUIT once the platform has asked to quit. below 0, the record can't
 * be read or the engine refused its picture. any other record returns SA_OK.
 * a record the engine refused is counted in sa_pc_client_report */
enum { SA_PC_ENDED = 2, SA_PC_QUIT = 3 };
sa_result sa_pc_client_record(sa_pc_client *client, const void *record);
/* print what the client saw: its failures and its totals */
void sa_pc_client_report(sa_pc_client *client);
/* call it between pictures, while its engine lives. it releases all the
 * client holds and has the engine retire each resource by sa_pc_retire. it
 * frees the client once every byte it lent is back, and returns SA_OK. the
 * engine's next picture stands alone. on an error the client stays and frees
 * no byte it lent. it returns sa_pc_retire's error, as SA_ERR_INVALID with a
 * picture open. SA_ERR_INVALID also says a retirement never reached the
 * client, as where another reader took the engine's events. a client
 * refused at every call stays for the process's life, with what it lent */
sa_result sa_pc_client_destroy(sa_pc_client *client);

#ifdef __cplusplus
}
#endif

#endif
