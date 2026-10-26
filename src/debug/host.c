/* requests between the game and the development host, which reads and
 * writes the game's memory while it runs */
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "debug/editor.h"
#include "debug/host.h"
#include "player/player.h"
#include "system/loader.h"
#include "system/task.h"

/* what the development host asks of the game, in host_request.code */
enum host_code {
    HOST_NONE,
    HOST_CAPTURE_SCREEN,
    HOST_RESTORE_SNAPSHOT,
    HOST_SAVE_EDITOR,
    HOST_RESTORE_EDITOR
};

/* what the game asks of the host, in game_request.code */
enum game_code {
    GAME_NONE,
    GAME_LOAD_FILE,
    GAME_SAVE_FILE,
    GAME_REQUEST_3,
    GAME_REQUEST_4
};

/* host_capture_screen has njFrameBufferBmp read the shown frame into
 * CAPTURE_BUFFER and write it as a 24-bit BMP at CAPTURE_BMP. both lie in
 * WORK_AREA */
#define CAPTURE_BMP ((char *)WORK_AREA)
#define CAPTURE_BUFFER ((char *)0x0ce00000)

#define REQUEST_ARGS 8

/* a request: its maker sets code and args, then pending, and whoever
 * serves it clears pending when it is done */
struct host_request {
    Sint32 code;
    Sint32 args[REQUEST_ARGS]; /* the result comes back in args[0] */
    Bool pending;
};

struct snapshot_place {
    Float pos[3];
    Sint32 ang[3];
};

struct snapshot {
    struct snapshot_place camera;
    struct snapshot_place players[PLAYER_COUNT];
    Sint32 unk_d8;
    Sint32 unk_dc;
    Sint32 unk_e0;
};

/* the set editor's kind and cursor, which HOST_SAVE_EDITOR copies out for
 * the host and HOST_RESTORE_EDITOR copies back while the editor is open */
struct editor_snapshot {
    Uint16 kind;
    Sint16 ang_x, ang_y, ang_z;
    NJS_POINT3 pos;
    NJS_POINT3 scl;
};

static void host_take_snapshot(void);
static void host_restore_snapshot(void);
static void host_save_editor(void);
static void host_restore_editor(void);
static void host_capture_screen(void);

/* the host reads and writes these while the game runs, so each is
 * volatile. host_serve_request serves host_request, and the game's
 * requests wait until the host clears game_request.pending */
static volatile struct host_request host_request;
static volatile struct host_request game_request;
/* the camera and players as host_take_snapshot leaves them for the host,
 * and as the host leaves them for HOST_RESTORE_SNAPSHOT */
static volatile struct snapshot snapshot_out;
static volatile struct snapshot snapshot_in;
static volatile struct editor_snapshot editor_snapshot;

static void host_take_snapshot(void)
{
    struct task *task;
    struct task_work *work;
    int i;

    snapshot_out.camera.pos[0] = camera_work->pos.x;
    snapshot_out.camera.pos[1] = camera_work->pos.y;
    snapshot_out.camera.pos[2] = camera_work->pos.z;
    snapshot_out.camera.ang[0] = camera_work->ang.x;
    snapshot_out.camera.ang[1] = camera_work->ang.y;
    snapshot_out.camera.ang[2] = camera_work->ang.z;
    for (i = 0; i < PLAYER_COUNT; i++) {
        task = player_get_task(i);
        if (task != NULL) {
            work = task->work;
            snapshot_out.players[i].pos[0] = work->pos.x;
            snapshot_out.players[i].pos[1] = work->pos.y;
            snapshot_out.players[i].pos[2] = work->pos.z;
            snapshot_out.players[i].ang[0] = work->ang.x;
            snapshot_out.players[i].ang[1] = work->ang.y;
            snapshot_out.players[i].ang[2] = work->ang.z;
        } else {
            snapshot_out.players[i].pos[0] = 0.0F;
            snapshot_out.players[i].pos[1] = 0.0F;
            snapshot_out.players[i].pos[2] = 0.0F;
            snapshot_out.players[i].ang[0] = 0;
            snapshot_out.players[i].ang[1] = 0;
            snapshot_out.players[i].ang[2] = 0;
        }
    }
}

static void host_restore_snapshot(void)
{
    struct task *task;
    struct task_work *work;
    int i;

    camera_work->pos.x = snapshot_in.camera.pos[0];
    camera_work->pos.y = snapshot_in.camera.pos[1];
    camera_work->pos.z = snapshot_in.camera.pos[2];
    camera_work->ang.x = snapshot_in.camera.ang[0];
    camera_work->ang.y = snapshot_in.camera.ang[1];
    camera_work->ang.z = snapshot_in.camera.ang[2];
    for (i = 0; i < PLAYER_COUNT; i++) {
        task = player_get_task(i);
        if (task != NULL) {
            work = task->work;
            work->pos.x = snapshot_in.players[i].pos[0];
            work->pos.y = snapshot_in.players[i].pos[1];
            work->pos.z = snapshot_in.players[i].pos[2];
            work->ang.x = snapshot_in.players[i].ang[0];
            work->ang.y = snapshot_in.players[i].ang[1];
            work->ang.z = snapshot_in.players[i].ang[2];
        }
    }
}

static void host_save_editor(void)
{
    if (edit_mode == 1) {
        editor_snapshot.kind = editor_kind;
        editor_snapshot.ang_x = editor_work->ang.x;
        editor_snapshot.ang_y = editor_work->ang.y;
        editor_snapshot.ang_z = editor_work->ang.z;
        editor_snapshot.pos.x = editor_work->pos.x;
        editor_snapshot.pos.y = editor_work->pos.y;
        editor_snapshot.pos.z = editor_work->pos.z;
        editor_snapshot.scl.x = editor_work->scl.x;
        editor_snapshot.scl.y = editor_work->scl.y;
        editor_snapshot.scl.z = editor_work->scl.z;
    }
}

static void host_restore_editor(void)
{
    if (edit_mode == 1) {
        editor_kind = editor_snapshot.kind;
        editor_work->ang.x = editor_snapshot.ang_x;
        editor_work->ang.y = editor_snapshot.ang_y;
        editor_work->ang.z = editor_snapshot.ang_z;
        editor_work->pos.x = editor_snapshot.pos.x;
        editor_work->pos.y = editor_snapshot.pos.y;
        editor_work->pos.z = editor_snapshot.pos.z;
        editor_work->scl.x = editor_snapshot.scl.x;
        editor_work->scl.y = editor_snapshot.scl.y;
        editor_work->scl.z = editor_snapshot.scl.z;
    }
}

static void host_snapshot_step(struct task *task)
{
    host_take_snapshot();
}

void host_snapshot_each_step(struct task *task)
{
    task->step = host_snapshot_step;
}

void host_serve_request(void)
{
    if (host_request.pending) {
        switch (host_request.code) {
        case HOST_NONE:
            break;
        case HOST_CAPTURE_SCREEN:
            host_capture_screen();
            break;
        case HOST_RESTORE_SNAPSHOT:
            host_restore_snapshot();
            break;
        case HOST_SAVE_EDITOR:
            host_save_editor();
            break;
        case HOST_RESTORE_EDITOR:
            host_restore_editor();
            break;
        }
        host_request.pending = FALSE;
    }
}

void host_init(void)
{
    int i;

    host_request.code = HOST_NONE;
    game_request.code = GAME_NONE;
    for (i = 0; i < REQUEST_ARGS; i++) {
        host_request.args[i] = 0;
        game_request.args[i] = 0;
    }
    host_request.pending = FALSE;
    game_request.pending = FALSE;
    snapshot_out.unk_d8 = 0xc3a000;
    snapshot_out.unk_dc = 0;
    snapshot_out.unk_e0 = 0;
    snapshot_in.unk_d8 = 0xc3a000;
    snapshot_in.unk_dc = 0;
    snapshot_in.unk_e0 = 0;
}

Sint32 host_load_file(const char *name, void *dest)
{
    Sint32 result;

    game_request.code = GAME_LOAD_FILE;
    game_request.args[0] = (Sint32)name;
    game_request.args[1] = (Sint32)dest;
    game_request.pending = TRUE;
    do {
        njWaitVSync();
    } while (game_request.pending);
    /* @hack spelling: the answer through a local, which loads its address
     * into R4, not R0 (0x8c010a64); returned directly, 3 of 23 words differ */
    result = game_request.args[0];
    return result;
}

void host_save_file(const char *name, void *src, int size)
{
    game_request.code = GAME_SAVE_FILE;
    game_request.args[0] = (Sint32)name;
    game_request.args[1] = (Sint32)src;
    game_request.args[2] = size;
    game_request.pending = TRUE;
    do {
        njWaitVSync();
    } while (game_request.pending);
}

void fun_8c010a9e(Sint32 word0, Sint32 word1, Sint32 word2)
{
    game_request.code = GAME_REQUEST_3;
    game_request.args[0] = word0;
    game_request.args[1] = word1;
    game_request.args[2] = word2;
    game_request.pending = TRUE;
    do {
        njWaitVSync();
    } while (game_request.pending);
}

static void host_capture_screen(void)
{
    /* @hack spelling: the bmp's address through a local, which loads
     * CAPTURE_BUFFER into R5 ahead of it into R4 (0x8c010ad2, 0x8c010ad4) and
     * keeps host_serve_request from inlining the function; passed directly,
     * it is inlined and 11 of 11 words differ */
    char *bmp = CAPTURE_BMP;

    njFrameBufferBmp(bmp, CAPTURE_BUFFER);
    kmfSetTimeOutCount(1, 1);
}

Sint32 fun_8c010ae4(Sint32 word)
{
    Sint32 result;

    game_request.code = GAME_REQUEST_4;
    game_request.args[0] = word;
    game_request.pending = TRUE;
    do {
        njWaitVSync();
    } while (game_request.pending);
    /* @hack spelling: the answer through a local, as in host_load_file
     * (0x8c010b06); returned directly, 3 of 22 words differ */
    result = game_request.args[0];
    return result;
}
