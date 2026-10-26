/* VMS file system commands. each drive holds one command at a time, which the
 * drive's own fiber runs */
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>
#include <vmsfs.h>

#define STACK_SIZE 4096
/* the most characters of a file name, and the bytes of a volume label */
#define NAME_CHARS 12
#define VOLUME_BYTES 32

Sint32 buLastError[VMSFS_DRIVES];
struct vmsfs_command buVmsCmd[VMSFS_DRIVES];
char vmsfs_stack0[STACK_SIZE];
char vmsfs_stack1[STACK_SIZE];
char vmsfs_stack2[STACK_SIZE];
char vmsfs_stack3[STACK_SIZE];
char vmsfs_stack4[STACK_SIZE];
char vmsfs_stack5[STACK_SIZE];
char vmsfs_stack6[STACK_SIZE];
char vmsfs_stack7[STACK_SIZE];

void vmsfs_c_dispatch(Sint32 drive);
void vmsfs_c_fiber0(void);
void vmsfs_c_fiber1(void);
void vmsfs_c_fiber2(void);
void vmsfs_c_fiber3(void);
void vmsfs_c_fiber4(void);
void vmsfs_c_fiber5(void);
void vmsfs_c_fiber6(void);
void vmsfs_c_fiber7(void);

/* clear each drive's last error and command, and create a fiber for each
 * drive in the drives mask, bit n for drive n. the result is 0, or -1 when
 * no fiber server is free */
Sint32 vmsfs_c_init(Sint32 drives)
{
    /* @hack spelling: the eight drives' errors cleared one store at a time,
     * which SHC writes as eight MOV.L stores of R14's 0 (0x8c11df12 to
     * 0x8c11df20). as a loop over VMSFS_DRIVES, 60 of 123 words differ */
    buLastError[0] = BUD_ERR_OK;
    buLastError[1] = BUD_ERR_OK;
    buLastError[2] = BUD_ERR_OK;
    buLastError[3] = BUD_ERR_OK;
    buLastError[4] = BUD_ERR_OK;
    buLastError[5] = BUD_ERR_OK;
    buLastError[6] = BUD_ERR_OK;
    buLastError[7] = BUD_ERR_OK;
    memset(buVmsCmd, 0, sizeof(buVmsCmd));
    memset(vmsfs_stack0, '0', sizeof(vmsfs_stack0));
    memset(vmsfs_stack1, '1', sizeof(vmsfs_stack1));
    memset(vmsfs_stack2, '2', sizeof(vmsfs_stack2));
    memset(vmsfs_stack3, '3', sizeof(vmsfs_stack3));
    memset(vmsfs_stack4, '4', sizeof(vmsfs_stack4));
    memset(vmsfs_stack5, '5', sizeof(vmsfs_stack5));
    memset(vmsfs_stack6, '6', sizeof(vmsfs_stack6));
    memset(vmsfs_stack7, '7', sizeof(vmsfs_stack7));
    if (!vmsfb_init()) {
        return -1;
    }
    if (drives & (1 << 0)) {
        vmsfb_create(vmsfs_c_fiber0, vmsfs_stack0, sizeof(vmsfs_stack0));
    }
    if (drives & (1 << 1)) {
        vmsfb_create(vmsfs_c_fiber1, vmsfs_stack1, sizeof(vmsfs_stack1));
    }
    if (drives & (1 << 2)) {
        vmsfb_create(vmsfs_c_fiber2, vmsfs_stack2, sizeof(vmsfs_stack2));
    }
    if (drives & (1 << 3)) {
        vmsfb_create(vmsfs_c_fiber3, vmsfs_stack3, sizeof(vmsfs_stack3));
    }
    if (drives & (1 << 4)) {
        vmsfb_create(vmsfs_c_fiber4, vmsfs_stack4, sizeof(vmsfs_stack4));
    }
    if (drives & (1 << 5)) {
        vmsfb_create(vmsfs_c_fiber5, vmsfs_stack5, sizeof(vmsfs_stack5));
    }
    if (drives & (1 << 6)) {
        vmsfb_create(vmsfs_c_fiber6, vmsfs_stack6, sizeof(vmsfs_stack6));
    }
    if (drives & (1 << 7)) {
        vmsfb_create(vmsfs_c_fiber7, vmsfs_stack7, sizeof(vmsfs_stack7));
    }
    return 0;
}

Sint32 vmsfs_c_exit(void)
{
    memset(buVmsCmd, 0, sizeof(buVmsCmd));
    return 0;
}

void vmsfs_c_synch(void)
{
    vmsfb_exec();
}

/* a fiber starts with no argument, so each drive has its own entry function */
void vmsfs_c_fiber0(void)
{
    vmsfs_c_dispatch(0);
}

void vmsfs_c_fiber1(void)
{
    vmsfs_c_dispatch(1);
}

void vmsfs_c_fiber2(void)
{
    vmsfs_c_dispatch(2);
}

void vmsfs_c_fiber3(void)
{
    vmsfs_c_dispatch(3);
}

void vmsfs_c_fiber4(void)
{
    vmsfs_c_dispatch(4);
}

void vmsfs_c_fiber5(void)
{
    vmsfs_c_dispatch(5);
}

void vmsfs_c_fiber6(void)
{
    vmsfs_c_dispatch(6);
}

void vmsfs_c_fiber7(void)
{
    vmsfs_c_dispatch(7);
}

/* drive's fiber, which never returns. it runs the drive's command, or with
 * none, sleeps until the next vmsfs_c_synch. a command's BUD_ERR_NO_DISK
 * unmounts the command's drive. the drive is idle again before the completion
 * callback runs */
void vmsfs_c_dispatch(Sint32 drive)
{
    struct vmsfs_command *command = &buVmsCmd[drive];
    Sint32 err;
    Sint32 op;
    Uint32 param;

    for (;;) {
        switch (command->op) {
        case BUD_OP_CONNECT:
            command->drive = command->param1;
            err = vmsfs_connect(command->param1);
            break;
        case BUD_OP_MOUNT:
            command->drive = command->param1;
            err = vmsfs_mount(command->param1);
            break;
        case BUD_OP_FORMATDISK:
            command->drive = command->param1;
            err =
                vmsfs_format(command->param1, command->volume, command->param3,
                             &command->time, command->param5);
            break;
        case BUD_OP_CLOSEFILE:
            command->drive = ((BUFILE)command->param1)->drive;
            err = vmsfs_fclose((BUFILE)command->param1, command->param2);
            break;
        case BUD_OP_READFILE:
            command->drive = ((BUFILE)command->param1)->drive;
            err = vmsfs_fread((BUFILE)command->param1, (void *)command->param2,
                              command->param3);
            break;
        case BUD_OP_WRITEFILE:
            command->drive = ((BUFILE)command->param1)->drive;
            err = vmsfs_fwrite((BUFILE)command->param1,
                               (const void *)command->param2, command->param3);
            break;
        case BUD_OP_DELETEFILE:
            command->drive = command->param1;
            err = vmsfs_fdelete(command->param1, command->fname);
            break;
        case BUD_OP_RENAMEFILE:
            command->drive = command->param1;
            err = vmsfs_frename(command->param1, command->fname,
                                command->second_name);
            break;
        case BUD_OP_SETFILEATTR:
            command->drive = command->param1;
            err = vmsfs_fsetattr(command->param1, command->fname,
                                 command->param3, command->param4);
            break;
        case BUD_OP_DEFRAGDISK:
            command->drive = command->param1;
            err = vmsfs_xdefrag(command->param1, (void *)command->param2);
            break;
        case BUD_OP_READIMAGE:
            command->drive = command->param1;
            err =
                vmsfs_read_disk_image(command->param1, (void *)command->param2);
            break;
        case BUD_OP_WRITEIMAGE:
            command->drive = command->param1;
            err = vmsfs_write_disk_image(command->param1,
                                         (const void *)command->param2);
            break;
        case BUD_OP_LOADFILE:
            command->drive = command->param1;
            err = vmsfs_load_file(command->param1, command->fname,
                                  (void *)command->param3, command->param4);
            break;
        case BUD_OP_SAVEFILE:
            command->drive = command->param1;
            err = vmsfs_save_file(
                command->param1, command->fname, (const void *)command->param3,
                command->param4, &command->time, command->param6);
            break;
        case BUD_OP_SAVEEXECFILE:
            command->drive = command->param1;
            err = vmsfs_save_exec_file(
                command->param1, command->fname, (const void *)command->param3,
                command->param4, &command->time, command->param6);
            break;
        case BUD_OP_LOADFILEEX:
            command->drive = command->param1;
            err = vmsfs_load_file_ex(command->param1, command->fname,
                                     (void *)command->param3, command->param4,
                                     command->param5);
            break;
        case BUD_OP_DELETEFILEEX:
            command->drive = command->param1;
            err = vmsfs_fdelete_ex(command->param1,
                                   (const char **)command->param2,
                                   command->param3);
            break;
        case BUD_OP_REWRITEEXECFILE:
            command->drive = command->param1;
            err = vmsfs_rewrite_exec_file(command->param1, command->fname,
                                          (const void *)command->param3,
                                          command->param4, command->param5);
            break;
        case BUD_OP_NOP:
        default:
            vmsfb_sleep();
            continue;
        }
        if (err == (Sint32)BUD_ERR_NO_DISK) {
            vmsfs_unmount(command->drive);
        }
        buLastError[drive] = err;
        op = command->op;
        param = command->param;
        command->unk_00 = 0;
        command->busy = false;
        command->op = BUD_OP_NOP;
        command->unk_0c = 0;
        command->drive = 0;
        command->param1 = 0;
        command->param2 = 0;
        command->param3 = 0;
        command->param4 = 0;
        command->param5 = 0;
        command->param6 = 0;
        if (command->complete) {
            command->complete(drive, op, err, param);
        }
    }
}

void vmsfs_c_execcallback(Sint32 drive, Sint32 op, Sint32 status)
{
    struct vmsfs_command *command = &buVmsCmd[drive];

    if (command->complete) {
        command->complete(drive, op, status, command->param);
    }
}

void vmsfs_c_progress(Sint32 drive, Sint32 op, Sint32 count, Sint32 max)
{
    struct vmsfs_command *command = &buVmsCmd[drive];

    if (command->progress) {
        command->progress(drive, op, count, max);
    }
}

void vmsfs_c_setcompletecallback(BU_COMPLETECALLBACK func)
{
    Sint32 drive;

    for (drive = 0; drive < VMSFS_DRIVES; drive++) {
        buVmsCmd[drive].complete = func;
    }
}

void vmsfs_c_setprogresscallback(BU_PROGRESSCALLBACK func)
{
    Sint32 drive;

    for (drive = 0; drive < VMSFS_DRIVES; drive++) {
        buVmsCmd[drive].progress = func;
    }
}

void vmsfs_c_setcallbackparam(Uint32 param)
{
    Sint32 drive;

    for (drive = 0; drive < VMSFS_DRIVES; drive++) {
        buVmsCmd[drive].param = param;
    }
}

Sint32 vmsfs_c_getstate(Sint32 drive)
{
    struct vmsfs_command *command = &buVmsCmd[drive];

    return command->busy;
}

void vmsfs_c_setfunc(Sint32 drive, Sint32 op)
{
    struct vmsfs_command *command = &buVmsCmd[drive];

    command->op = op;
}

/* set the command's arguments. the drive is busy from then until its fiber
 * finishes the command */
void vmsfs_c_setparam(Sint32 drive, Sint32 param1, Sint32 param2, Sint32 param3,
                      Sint32 param4, Sint32 param5, Sint32 param6)
{
    struct vmsfs_command *command = &buVmsCmd[drive];

    command->param1 = param1;
    command->param2 = param2;
    command->param3 = param3;
    command->param4 = param4;
    command->param5 = param5;
    command->param6 = param6;
    command->busy = true;
}

void vmsfs_c_setvolume(Sint32 drive, const Uint8 *volume)
{
    struct vmsfs_command *command = &buVmsCmd[drive];
    Sint32 i;

    for (i = 0; i < VOLUME_BYTES; i++) {
        command->volume[i] = volume[i];
    }
}

/* copy up to NAME_CHARS characters of each name, zero-terminated */
void vmsfs_c_setfname(Sint32 drive, const char *fname, const char *second_name)
{
    struct vmsfs_command *command = &buVmsCmd[drive];
    Sint32 i;

    for (i = 0; i < NAME_CHARS + 1; i++) {
        command->fname[i] = 0;
    }
    for (i = 0; i < NAME_CHARS + 1; i++) {
        command->second_name[i] = 0;
    }
    for (i = 0; i < NAME_CHARS; i++) {
        if (fname[i] == 0) {
            break;
        }
        command->fname[i] = fname[i];
    }
    for (i = 0; i < NAME_CHARS; i++) {
        if (second_name[i] == 0) {
            break;
        }
        command->second_name[i] = second_name[i];
    }
}

void vmsfs_c_settime(Sint32 drive, const BUS_TIME *time)
{
    struct vmsfs_command *command = &buVmsCmd[drive];

    memcpy(&command->time, time, sizeof(BUS_TIME));
}
