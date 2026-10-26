/* VMS, the gardens' memory card station. a press of its button starts a
 * take, which moves a Chao or an egg off a card into the garden. while
 * player 0 holds something, the press starts a put instead, which moves the
 * Chao or egg let go on the VMS onto a card */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "al_main/camera.h"
#include "al_main/card.h"
#include "al_main/card_menu.h"
#include "al_main/chao.h"
#include "al_main/entry.h"
#include "al_main/models.h"
#include "al_main/physics.h"
#include "al_main/tree.h"
#include "al_main/vms.h"
#include "chao/chao.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/common.h"
#include "player/player.h"
#include "save/mail.h"
#include "sound/sound.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* the doors on top, which slide apart along x as vms_open_action plays from
 * frame 0 to 1 */
static NJS_MATERIAL vms_door_minus_x_materials[1] = {
#include "al_main/vms/vms_door_minus_x_materials.inc"
};

static Sint16 vms_door_minus_x_meshes0[5] = {
#include "al_main/vms/vms_door_minus_x_meshes0.inc"
};

static NJS_COLOR vms_door_minus_x_uvs0[4] = {
#include "al_main/vms/vms_door_minus_x_uvs0.inc"
};

static NJS_MESHSET vms_door_minus_x_meshsets[1] = {
#include "al_main/vms/vms_door_minus_x_meshsets.inc"
};

static NJS_POINT3 vms_door_minus_x_points[4] = {
#include "al_main/vms/vms_door_minus_x_points.inc"
};

static NJS_VECTOR vms_door_minus_x_normals[4] = {
#include "al_main/vms/vms_door_minus_x_normals.inc"
};

static NJS_MODEL vms_door_minus_x_model = {
#include "al_main/vms/vms_door_minus_x_model.inc"
};

static NJS_OBJECT vms_door_minus_x_object = {
#include "al_main/vms/vms_door_minus_x_object.inc"
};

static NJS_MATERIAL vms_door_plus_x_materials[1] = {
#include "al_main/vms/vms_door_plus_x_materials.inc"
};

static Sint16 vms_door_plus_x_meshes0[5] = {
#include "al_main/vms/vms_door_plus_x_meshes0.inc"
};

static NJS_COLOR vms_door_plus_x_uvs0[4] = {
#include "al_main/vms/vms_door_plus_x_uvs0.inc"
};

static NJS_MESHSET vms_door_plus_x_meshsets[1] = {
#include "al_main/vms/vms_door_plus_x_meshsets.inc"
};

static NJS_POINT3 vms_door_plus_x_points[4] = {
#include "al_main/vms/vms_door_plus_x_points.inc"
};

static NJS_VECTOR vms_door_plus_x_normals[4] = {
#include "al_main/vms/vms_door_plus_x_normals.inc"
};

static NJS_MODEL vms_door_plus_x_model = {
#include "al_main/vms/vms_door_plus_x_model.inc"
};

static NJS_OBJECT vms_door_plus_x_object = {
#include "al_main/vms/vms_door_plus_x_object.inc"
};

/* the button, which a player pushes down */
static NJS_MATERIAL vms_button_materials[1] = {
#include "al_main/vms/vms_button_materials.inc"
};

static Sint16 vms_button_meshes0[36] = {
#include "al_main/vms/vms_button_meshes0.inc"
};

static NJS_COLOR vms_button_uvs0[32] = {
#include "al_main/vms/vms_button_uvs0.inc"
};

static NJS_MESHSET vms_button_meshsets[1] = {
#include "al_main/vms/vms_button_meshsets.inc"
};

static NJS_POINT3 vms_button_points[17] = {
#include "al_main/vms/vms_button_points.inc"
};

static NJS_VECTOR vms_button_normals[17] = {
#include "al_main/vms/vms_button_normals.inc"
};

static NJS_MODEL vms_button_model = {
#include "al_main/vms/vms_button_model.inc"
};

static NJS_OBJECT vms_button_object = {
#include "al_main/vms/vms_button_object.inc"
};

/* the monitor, whose screen is SCREEN_MATERIAL */
static NJS_MATERIAL vms_monitor_materials[4] = {
#include "al_main/vms/vms_monitor_materials.inc"
};

static Sint16 vms_monitor_meshes0[25] = {
#include "al_main/vms/vms_monitor_meshes0.inc"
};

static Sint16 vms_monitor_meshes1[9] = {
#include "al_main/vms/vms_monitor_meshes1.inc"
};

static Sint16 vms_monitor_meshes2[10] = {
#include "al_main/vms/vms_monitor_meshes2.inc"
};

static Sint16 vms_monitor_meshes3[5] = {
#include "al_main/vms/vms_monitor_meshes3.inc"
};

static NJS_COLOR vms_monitor_uvs0[22] = {
#include "al_main/vms/vms_monitor_uvs0.inc"
};

static NJS_COLOR vms_monitor_uvs1[8] = {
#include "al_main/vms/vms_monitor_uvs1.inc"
};

static NJS_COLOR vms_monitor_uvs2[8] = {
#include "al_main/vms/vms_monitor_uvs2.inc"
};

static NJS_COLOR vms_monitor_uvs3[4] = {
#include "al_main/vms/vms_monitor_uvs3.inc"
};

static NJS_MESHSET vms_monitor_meshsets[4] = {
#include "al_main/vms/vms_monitor_meshsets.inc"
};

static NJS_POINT3 vms_monitor_points[20] = {
#include "al_main/vms/vms_monitor_points.inc"
};

static NJS_VECTOR vms_monitor_normals[20] = {
#include "al_main/vms/vms_monitor_normals.inc"
};

static NJS_MODEL vms_monitor_model = {
#include "al_main/vms/vms_monitor_model.inc"
};

static NJS_OBJECT vms_monitor_object = {
#include "al_main/vms/vms_monitor_object.inc"
};

/* the base, whose children are the monitor, the button and the doors */
static NJS_MATERIAL vms_materials[8] = {
#include "al_main/vms/vms_materials.inc"
};

static Sint16 vms_meshes0[7] = {
#include "al_main/vms/vms_meshes0.inc"
};

static Sint16 vms_meshes1[19] = {
#include "al_main/vms/vms_meshes1.inc"
};

static Sint16 vms_meshes2[20] = {
#include "al_main/vms/vms_meshes2.inc"
};

static Sint16 vms_meshes3[9] = {
#include "al_main/vms/vms_meshes3.inc"
};

static Sint16 vms_meshes4[19] = {
#include "al_main/vms/vms_meshes4.inc"
};

static Sint16 vms_meshes5[38] = {
#include "al_main/vms/vms_meshes5.inc"
};

static Sint16 vms_meshes6[11] = {
#include "al_main/vms/vms_meshes6.inc"
};

static Sint16 vms_meshes7[5] = {
#include "al_main/vms/vms_meshes7.inc"
};

static NJS_COLOR vms_uvs0[6] = {
#include "al_main/vms/vms_uvs0.inc"
};

static NJS_COLOR vms_uvs1[18] = {
#include "al_main/vms/vms_uvs1.inc"
};

static NJS_COLOR vms_uvs2[18] = {
#include "al_main/vms/vms_uvs2.inc"
};

static NJS_COLOR vms_uvs3[8] = {
#include "al_main/vms/vms_uvs3.inc"
};

static NJS_COLOR vms_uvs4[16] = {
#include "al_main/vms/vms_uvs4.inc"
};

static NJS_COLOR vms_uvs5[32] = {
#include "al_main/vms/vms_uvs5.inc"
};

static NJS_COLOR vms_uvs6[10] = {
#include "al_main/vms/vms_uvs6.inc"
};

static NJS_COLOR vms_uvs7[4] = {
#include "al_main/vms/vms_uvs7.inc"
};

static NJS_MESHSET vms_meshsets[8] = {
#include "al_main/vms/vms_meshsets.inc"
};

static NJS_POINT3 vms_points[58] = {
#include "al_main/vms/vms_points.inc"
};

static NJS_VECTOR vms_normals[58] = {
#include "al_main/vms/vms_normals.inc"
};

static NJS_MODEL vms_model = {
#include "al_main/vms/vms_model.inc"
};

static NJS_OBJECT vms_object = {
#include "al_main/vms/vms_object.inc"
};

static NJS_MKEY_F vms_open_pos3[2] = {
#include "al_main/vms/vms_open_pos3.inc"
};

static NJS_MKEY_F vms_open_pos4[2] = {
#include "al_main/vms/vms_open_pos4.inc"
};

static NJS_MDATA2 vms_open_mdata[5] = {
#include "al_main/vms/vms_open_mdata.inc"
};

static NJS_MOTION vms_open_motion = {
#include "al_main/vms/vms_open_motion.inc"
};

static NJS_ACTION vms_open_action = {
#include "al_main/vms/vms_open_action.inc"
};

/* the base's three tiers and a box, which push everything, then a sphere
 * over the button. a player's touch on the sphere presses the button */
static const struct collision_shape vms_shapes[5] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     19.5F,
     0.75F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     18.0F,
     5.3F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     16.5F,
     7.5F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, -11.25F},
     3.6F,
     24.0F,
     1.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 5.0F, 9.0F},
     4.5F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* the record of the Chao or egg vms_take last took off a card. vms_take
 * empties the fruit of the garden's record, and drops them from this copy */
static struct chao vms_taken;
/* 512 bytes of bss that no word in AL_MAIN, AL_RACE, the gardens or
 * 1ST_READ.BIN points to, so nothing shows their type */
static Uint8 dat_0cb1eb98[0x200];
/* whether pad 0 held Y as the put menu closed on a drive. vms_put then also
 * writes a Chao as mail, with save_write_chao_mail */
static Bool vms_mail;

struct vms_vars {
    /* the monitor's phase. vms_init sets it to 0x4000, and nothing changes
     * it. vms_init places the monitor by njCos(monitor), which is 0, halfway
     * up. vms_step places it by njCos(monitor - 0x4000), which is 1, so the
     * monitor stands at MONITOR_TOP */
    Angle monitor;
    /* the steps with no touch on the button's sphere since a player last
     * touched it */
    Sint16 release_wait;
    Sint16 fruit_left; /* vms_taken's fruit slots still to drop */
    /* the task of the Chao or egg the put writes to a card. the put takes
     * it out of the garden once the write ends */
    struct task *chao;
};

#define VARS(work) ((struct vms_vars *)&(work)->vars)

/* the VMS's own record, at its task's motion */
struct vms_motion {
    Uint8 unk_00[0x28];
    /* the phases of the VMS's shake. its x moves by the sine of shake_x,
     * its z by that of shake_z and its y by that of their sum. the put
     * advances them while it waits on a card */
    Angle shake_x;
    Uint8 unk_2c[4];
    Angle shake_z;
    Uint8 unk_34[4];
    Float frame; /* of vms_open_action: 0 with the doors shut, 1 open */
    Uint8 unk_3c[4];
};

#define VMS_MOTION(task) ((struct vms_motion *)(task)->motion)

/* the work's submode: what the VMS does */
enum { VMS_IDLE, VMS_TAKING, VMS_PUTTING };

/* in the work's flags. vms_step sets VMS_PRESSED as the button bottoms
 * out, then takes the press as the start of a take or a put */
#define VMS_PRESSED 0x1000
/* set with VMS_PRESSED. no press counts until it clears, on the 32nd step
 * with no touch on the button's sphere since a player's last */
#define VMS_DOWN 0x2000

/* the monitor's material that shows its screen. vms_init gives it texture 1
 * of alife_objects_texlist, monita02. as a put ends, vms_put flips it to
 * texture 2, monita03, and back three times */
#define SCREEN_MATERIAL 3

/* the monitor's y is MONITOR_LOW plus (cos + 1) * MONITOR_SWING, so it runs
 * from MONITOR_LOW to MONITOR_TOP. MONITOR_LOW is spelled from the other
 * two because SHC folds that expression to the image's float, 0xc0466664
 * (0x0c91abe0, 0x0c91b8d8). -3.1F would be 0xc0466666 */
#define MONITOR_SWING 4.3F
#define MONITOR_TOP 5.5F
#define MONITOR_LOW (MONITOR_TOP - (2.0F * MONITOR_SWING))

#define SE_FLASH 0x258 /* plays as the screen flips to texture 2 */
/* plays as a put's doors start to open or shut, and every 20 steps while
 * the put waits on a card */
#define SE_WORKING 0x25b
#define SE_BUTTON 0x25c /* plays as the button bottoms out */
#define SE_DROP 0x25d /* plays as each of a taken record's fruit drops out */

/* the card menu's notices: the garden is full, and a put has failed */
#define NOTICE_FULL 1
#define NOTICE_FAILED 4

/* where the card menu's camera stands: MENU_VIEW_OUT out from the VMS along
 * its ang.y less MENU_VIEW_TURN, and MENU_VIEW_UP above it. it looks at
 * MENU_VIEW_AIM above the VMS */
#define MENU_VIEW_OUT 29.0F
#define MENU_VIEW_TURN 0x2b00
#define MENU_VIEW_UP 23.0F
#define MENU_VIEW_AIM 5.0F

static void vms_destroy(struct task *task);
static void vms_draw(struct task *task);
static void vms_step(struct task *task);

void vms_init(struct task *task)
{
    struct task_work *work = task->work;
    struct vms_motion *motion = VMS_MOTION(task);
    /* @hack spelling: njCos called through cosine, a pointer to it, which puts
     * the image's ADD #12,R8 at 0x0c91aae2 ahead of the frame's first store;
     * called by name, the two trade and 2 of 140 words differ */
    Float (*cosine)(Angle);

    if (!placement_editing(task)) {
        collision_init(task, vms_shapes, COLLISION_OBJECT, 1, 5);
        task->step = vms_step;
    } else {
        task->step = vms_draw;
        task->extra = NULL;
    }
    alife_entry_add(task, ALIFE_PLACES, ALIFE_VMS, 18.0F);
    work->tick = 0;
    motion->frame = 0.0F;
    motion->shake_x = 0;
    motion->shake_z = 0;
    task->destructor = vms_destroy;
    task->draw = vms_draw;
    /* @hack redundant: the frame stored again, as the image stores it at
     * 0x0c91aae4 and again at 0x0c91aaf2; without it, the function runs 2
     * bytes short and 92 of 140 words differ */
    motion->frame = 0.0F;
    cosine = njCos;
    VARS(work)->monitor = 0x4000;
    /* @hack spelling: the monitor's y written through the object's address,
     * which the image loads into R8 and adds 12 to (0x0c91aade, 0x0c91aae2);
     * as vms_monitor_object.pos[1], the function runs 2 bytes short and 97
     * of 140 words differ */
    (&vms_monitor_object)->pos[1] =
        ((cosine(VARS(work)->monitor) + 1.0F) * MONITOR_SWING) + MONITOR_LOW;
    /* nothing in AL_MAIN reads this bit of the work's flags */
    work->flags |= 0x800;
    vms_monitor_materials[SCREEN_MATERIAL].attr_texId = 1;
    /* @hack redundant: an else that never runs, whose call through cosine
     * keeps the store below, for which the image loads the screen material's
     * address and 1 again (0x0c91ab18, 0x0c91ab20). without it, SHC drops
     * that store, and the function runs 6 bytes short and 76 of 140 words
     * differ */
    if (1) {
    } else {
        cosine(0);
    }
    /* @hack redundant: texture 1 stored again, as the image stores it at
     * 0x0c91ab1e and again at 0x0c91ab22; without it, the function runs 6
     * bytes short and 85 of 140 words differ */
    vms_monitor_materials[SCREEN_MATERIAL].attr_texId = 1;
    work->submode = VMS_IDLE;
    work->flags &= ~VMS_PRESSED;
    VARS(work)->release_wait = 0;
    garden_camera_fixed_pos.x =
        work->pos.x + (MENU_VIEW_OUT * cosine(work->ang.y - MENU_VIEW_TURN));
    garden_camera_fixed_pos.y = work->pos.y + MENU_VIEW_UP;
    garden_camera_fixed_pos.z =
        work->pos.z - (MENU_VIEW_OUT * njSin(work->ang.y - MENU_VIEW_TURN));
    garden_camera_fixed_target.x = work->pos.x;
    garden_camera_fixed_target.y = work->pos.y + MENU_VIEW_AIM;
    garden_camera_fixed_target.z = work->pos.z;
    garden_camera_fixed = FALSE;
    alife_paused = 0;
}

static void vms_destroy(struct task *task)
{
    struct alife_entry *entry = task->extra;
    alife_entry_remove(entry);
}

/* draw the VMS, shaken by its motion's phases, with its doors at their
 * frame */
static void vms_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct vms_motion *motion = VMS_MOTION(task);

    njSetTexture(&alife_objects_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x + (njSin(motion->shake_x) * 0.1F),
                work->pos.y +
                    (njSin(motion->shake_x + motion->shake_z) * 0.05F),
                work->pos.z + (njSin(motion->shake_z) * 0.1F));
    TURN(njRotateY, work->ang.y);
    njScale(NULL, 1.5F, 1.5F, 1.5F);
    draw_action(&vms_open_action, motion->frame);
    njPopMatrix(1);
}

int card_admit_type(Uint8 *type)
{
    if (*type >= 128) {
        *type -= 128;
    }
    switch (*type) {
    case CHAO_CHILD:
    case CHAO_STANDARD:
    case CHAO_SWIM:
    case CHAO_FLY:
    case CHAO_RUN:
    case CHAO_POWER:
    case CHAO_CHAOS:
    case CHAO_EGG:
    case CHAO_PARTNER:
        return 1;
    }
    return 0;
}

/* a step of the take: 1 while it runs, 0 once it ends. in a garden with
 * ALIFE_CHAO_FULL Chao and eggs, it shows NOTICE_FULL and ends. otherwise it
 * opens the card menu for taking, and ends where the menu fails or closes
 * with no slot chosen. once the player chooses a slot:
 * - the VMS opens its doors
 * - it copies the slot's record into an empty one, and throws the record's
 *   Chao or egg out
 * - it empties the card's slot. where the record's CHAO_DELETE_EXEC is set,
 *   it deletes the card's executable file instead, and clears the bit
 * - once the card is done, it calls alife_save, and drops the fruit
 *   vms_taken keeps, one slot every 21 steps
 * - once the save ends, it shuts the doors */
static int vms_take(struct task *task, struct vms_motion *motion)
{
    struct task_work *work = task->work;
    Sint32 cursor = card_menu.cursor;
    int garden = alife_get_garden();
    struct chao *chao;
    struct task *child;
    struct alife_chao *child_chao;
    int kind;
    int byte;
    int i;
    NJS_POINT3 position;
    NJS_VECTOR velocity;

    switch (work->mode) {
    case 0:
        if (alife_get_chao_count() < ALIFE_CHAO_FULL) {
            card_menu_take_create();
            work->mode++;
        } else {
            card_menu_notice_create(NOTICE_FULL);
            return 0;
        }
        if (card_menu.failed) {
            return 0;
        }
        break;
    case 1:
        if (card_menu.closing) {
            if (card_menu.chosen) {
                work->mode++;
            } else {
                return 0;
            }
        }
        if (card_menu.failed) {
            return 0;
        }
        break;
    case 2:
        motion->frame += 0.08F;
        if (motion->frame >= 1.0F) {
            motion->frame = 0.999F;
            work->mode++;
            work->timer = 0;
        }
        break;
    case 3:
        chao = chao_find_empty();
        if (!chao) {
            work->mode += 3;
        }
        /* @bug memory: with no empty record, chao is null. the mode moves on
         * toward the close, but the rest of the case still runs on chao,
         * first copying the slot's record through it at 0x0c91aed8 */
        *chao = *card_menu.record;
        vms_taken = *chao;
        for (i = 0; i < CHAO_FRUIT; i++) {
            chao->fruit[i] = 0;
        }
        chao->garden = garden;
        child = alife_chao_create(chao);
        child->work->ang.y = work->ang.y;
        child->work->pos = work->pos;
        child->work->pos.y += 3.0F;
        child_chao = (struct alife_chao *)child->motion;
        child_chao->record->pos = work->pos;
        child_chao->speed.x = njCos(work->ang.y + 0x8000) * 0.5F;
        child_chao->speed.y = 1.0F;
        child_chao->speed.z = njSin(work->ang.y + 0x8000) * 0.5F;
        child_chao->handling = HANDLING_THROWN;
        alife_paused = 0;
        if (chao->flags & CHAO_DELETE_EXEC) {
            chao->flags &= ~CHAO_DELETE_EXEC;
            card_delete_exec(card_menu.cursor / 2);
        } else if (chao->type == CHAO_EGG) {
            /* @hack type: volatile on cursor, which makes SHC read it
             * again in each arm (0x0c91af7e, 0x0c91af96, 0x0c91b000) rather
             * than take cursor / 2 once, ahead of the type test; read plainly,
             * the code after the function starts 32 bytes earlier, and 171 of
             * 377 words differ */
            fun_0c902760(*(volatile Sint32 *)&card_menu.cursor / 2);
        } else {
            card_clear_chao(card_menu.cursor / 2);
        }
        chao_list_add(garden, chao);
        VARS(work)->fruit_left = CHAO_FRUIT;
        work->mode++;
        break;
    case 4:
        if (buStat(cursor / 2) == 0) {
            work->mode++;
            alife_save();
        }
        break;
    case 5:
        if (TASK_TIMER(work) > 20) {
            /* NOLINTBEGIN(bugprone-signed-char-misuse) */
            /* @hack type: the fruit's kind read into byte, an int, and cast
             * back to Sint8, so that SHC sign-extends the loaded byte again,
             * EXTS.B R4,R12 at 0x0c91b09a; read straight into kind, the code
             * after the function starts 32 bytes earlier, and 116 of 377 words
             * differ */
            byte = vms_taken.fruit[--VARS(work)->fruit_left];
            kind = (Sint8)byte;
            /* NOLINTEND(bugprone-signed-char-misuse) */
            if (kind) {
                position.x = work->pos.x;
                position.y = work->pos.y + 4.0F;
                position.z = work->pos.z;
                velocity.x = njCos(work->ang.y + 0x8000) * 0.5F;
                velocity.y = 1.0F;
                velocity.z = njSin(work->ang.y + 0x8000) * 0.5F;
                alife_fruit_create(&position, &velocity, kind);
                sound_play(SE_DROP, 0, 0, 0);
            }
            work->timer = 0;
            if (VARS(work)->fruit_left == 0) {
                work->mode++;
            }
        }
        break;
    case 6:
        if (buStat(card_get_save_drive()) == 0) {
            work->mode++;
        }
        break;
    case 7:
        motion->frame -= 0.03F;
        if (motion->frame <= 0.0F) {
            motion->frame = 0.0F;
            work->mode = 0;
            return 0;
        }
        break;
    }
    return 1;
}

/* a step of the put: 1 while it runs, 0 once it ends. where the first entry
 * touching the VMS is a Chao or an egg that no player holds, the put opens the
 * card menu for putting. on a step nothing touches the VMS, it ends instead
 * once the work's timer has passed 120. that step counts the timer twice, once
 * here and once in vms_step, so with nothing there the put ends at its 61st
 * step.
 *
 * once the player chooses a drive, the VMS draws the Chao or egg in and opens
 * its doors. it drops the Chao or egg in, writes it to the card and shuts its
 * doors. once the card is done, it takes the Chao or egg out of the garden and
 * calls alife_save. it shakes while it waits on each. then it plays SE_CONFIRM,
 * rumbles the pad in the card's port and flashes the screen. a write that won't
 * start throws the Chao or egg back out, and a wait past 1800 steps ends the
 * put. each shows NOTICE_FAILED */
static int vms_put(struct task *task, struct vms_motion *motion)
{
    struct task_work *work = task->work;
    struct card_menu *menu = &card_menu;
    struct alife_entry *entry = task->extra;
    struct alife_chao *child_chao;
    struct task *child;
    struct chao *held;
    NJS_POINT3 *position;
    /* @hack redundant: 16 bytes of the frame that no code reads, for the
     * image's ADD #-16,R15 at 0x0c91b1cc; without them, the function runs 2
     * bytes short and 88 of 601 words differ */
    char unused[16];
    Float target_x;
    Float target_z;
    int kind;

    /* @hack redundant: a never-run test of motion, which keeps vms_mail's
     * address in R4 and the screen material's in R5 (0x0c91b1de,
     * 0x0c91b1e6); without it, the two trade and 11 of 601 words differ */
    if (1) {
    } else if (motion) {
    }
    switch (work->mode) {
    case 0:
        if (entry->flags & ENTRY_TOUCHING) {
            kind = entry->hits[0]->kind;
            if (kind == ALIFE_HATCHED || kind == ALIFE_EGG) {
                if (!(entry->hits[0]->task->work->flags & CARRY_HELD)) {
                    menu->held =
                        ((struct alife_chao *)entry->hits[0]->task->motion)
                            ->record;
                    if (entry->hits[0]->kind == ALIFE_HATCHED) {
                        menu->held_chao = TRUE;
                    } else {
                        menu->held_chao = FALSE;
                    }
                    card_menu_put_create();
                    work->mode++;
                }
            }
        } else {
            if (TASK_TIMER(work)++ > 120) {
                return 0;
            }
        }
        if (menu->failed) {
            return 0;
        }
        break;
    case 1:
        if (menu->closing) {
            if (menu->placed) {
                if (per_info[0]->on & PDD_DGT_TY) {
                    vms_mail = TRUE;
                } else {
                    vms_mail = FALSE;
                }
                work->mode++;
                work->timer = 0;
            } else {
                return 0;
            }
        }
        if (menu->failed) {
            return 0;
        }
        break;
    case 2:
        if (TASK_TIMER(work) > 40) {
            sound_play(SE_WORKING, 0, 0, 0);
            work->mode++;
            work->timer = 0;
        }
        kind = entry->hits[0]->kind;
        if (kind == ALIFE_HATCHED || kind == ALIFE_EGG) {
            /* @hack redundant: the cosine and the sine each multiplied by 1.0F,
             * which the image does with FR15, set to 1 by FLDI1 at 0x0c91b1ec:
             * FMUL FR15,FR0 at 0x0c91b3b8, and FR0 copied from FR15 for the
             * FMAC at 0x0c91b3d6. without them, the code after the function
             * starts 96 bytes later, and 506 of 601 words differ */
            target_x = work->pos.x - (njCos(work->ang.y) * 1.0F);
            target_z = work->pos.z + (njSin(work->ang.y) * 1.0F);
            position = &entry->hits[0]->task->work->pos;
            if (position->x > target_x) {
                position->x -= 0.06F;
            } else if (position->x < target_x) {
                position->x += 0.06F;
            }
            if (position->z > target_z) {
                position->z -= 0.06F;
            } else if (position->z < target_z) {
                position->z += 0.06F;
            }
        }
        break;
    case 3:
        motion->frame += 0.08F;
        if (motion->frame >= 1.0F) {
            motion->frame = 0.999F;
            work->mode++;
            work->timer = 0;
            if (entry->flags & ENTRY_TOUCHING) {
                held = menu->held;
                /* @hack redundant: a never-run test of held, which keeps
                 * card_menu_notice_create's address in R8 (0x0c91b1e2); without
                 * it, njCos takes R8, and the code after the function starts
                 * 128 bytes later, and 463 of 601 words differ */
                if (1) {
                } else if (held) {
                }
                entry->hits[0]->flags |= ENTRY_LET_GO;
                if (menu->held_chao) {
                    if (vms_mail) {
                        /* @hack spelling: the record read as menu->held again,
                         * which the image loads at 0x0c91b490, after the
                         * let-go's store at 0x0c91b47c; through held, 12 of
                         * 601 words differ */
                        save_write_chao_mail(0, menu->held);
                    }
                    if (!card_write_chao(menu->cursor / 2, held)) {
                        child = entry->hits[0]->task;
                        child->work->ang.y = work->ang.y - 0x4000;
                        child->work->pos = work->pos;
                        child->work->pos.y += 3.0F;
                        child_chao = (struct alife_chao *)child->motion;
                        child_chao->record->pos = work->pos;
                        child_chao->speed.x =
                            njCos(work->ang.y + 0x8000) * 0.5F;
                        child_chao->speed.y = 1.0F;
                        child_chao->speed.z =
                            njSin(work->ang.y + 0x8000) * 0.5F;
                        child_chao->handling = HANDLING_THROWN;
                        card_menu_notice_create(NOTICE_FAILED);
                        work->mode = 100;
                    } else {
                        child = entry->hits[0]->task;
                        VARS(work)->chao = child;
                    }
                } else {
                    if (!card_write_egg(menu->cursor / 2, held)) {
                        child = entry->hits[0]->task;
                        child->work->ang.y = work->ang.y - 0x4000;
                        child->work->pos = work->pos;
                        child->work->pos.y += 3.0F;
                        child_chao = (struct alife_chao *)child->motion;
                        child_chao->record->pos = work->pos;
                        child_chao->speed.x =
                            njCos(work->ang.y + 0x8000) * 0.5F;
                        child_chao->speed.y = 1.0F;
                        child_chao->speed.z =
                            njSin(work->ang.y + 0x8000) * 0.5F;
                        child_chao->handling = HANDLING_THROWN;
                        card_menu_notice_create(NOTICE_FAILED);
                        work->mode = 100;
                    } else {
                        child = entry->hits[0]->task;
                        VARS(work)->chao = child;
                    }
                }
            } else {
                return 0;
            }
        }
        break;
    case 4:
        if (TASK_TIMER(work) > 60) {
            work->mode++;
            work->timer = 0;
            sound_play(SE_WORKING, 0, 0, 0);
        }
        break;
    case 5:
        motion->frame -= 0.03F;
        if (motion->frame <= 0.0F) {
            motion->frame = 0.0F;
            work->mode++;
            work->timer = 0;
        }
        break;
    case 6:
        if (TASK_TIMER(work) % 20 == 0) {
            sound_play(SE_WORKING, 0, 0, 0);
        }
        if (buStat(menu->cursor / 2) == 0) {
            alife_chao_remove(VARS(work)->chao);
            alife_save();
            work->mode++;
            work->timer = 0;
        } else {
            motion->shake_x += 0x1000;
            motion->shake_z += 0x1900;
            if (TASK_TIMER(work) > 1800) {
                card_menu_notice_create(NOTICE_FAILED);
                work->mode = 100;
            }
        }
        break;
    case 7:
        if (buStat(card_get_save_drive()) == 0) {
            work->mode++;
            work->timer = 0;
        } else {
            motion->shake_x += 0x1000;
            motion->shake_z += 0x1900;
            if (TASK_TIMER(work) > 1800) {
                card_menu_notice_create(NOTICE_FAILED);
                work->mode = 100;
            }
        }
        break;
    case 8:
        if (TASK_TIMER(work) > 60) {
            work->mode++;
            work->timer = 0;
            sound_play(SE_CONFIRM, 0, 0, 0);
            /* the pad whose port holds the card's drive */
            vibration_start(menu->cursor / 4, 4, 30, 1);
        }
        break;
    case 9:
        if (TASK_TIMER(work) > 40) {
            vms_monitor_materials[SCREEN_MATERIAL].attr_texId = 1;
            work->mode++;
            work->timer = 0;
            if (work->tick++ > 2) {
                work->tick = 0;
                work->mode = 0;
                motion->shake_x = 0;
                motion->shake_z = 0;
                /* @hack redundant: the mode cleared again, as the image
                 * clears it at 0x0c91b764 and again at 0x0c91b76c; without
                 * it, 32 of 601 words differ */
                work->mode = 0;
                return 0;
            }
        }
        break;
    case 10:
        if (TASK_TIMER(work) > 40) {
            /* @hack redundant: a never-run test of the screen material's
             * address, which keeps that address in R5 and 40 in R6 (0x0c91b1e6,
             * 0x0c91b1e4); without it, the two trade, and 22 of 601 words and 4
             * of the function's 204 words of data differ */
            if (1) {
            } else if (&vms_monitor_materials[SCREEN_MATERIAL]) {
            }
            vms_monitor_materials[SCREEN_MATERIAL].attr_texId = 2;
            work->mode--;
            work->timer = 0;
            sound_play(SE_FLASH, 0, 0, 0);
        }
        break;
    case 100:
        motion->frame -= 0.03F;
        if (motion->frame <= 0.0F) {
            motion->frame = 0.0F;
            work->mode++;
            work->timer = 0;
        }
        break;
    case 101:
        work->tick = 0;
        work->mode = 0;
        motion->shake_x = 0;
        motion->shake_z = 0;
        return 0;
    }
    return 1;
}

/* a player's touch on the button's sphere pushes the button down. as it
 * bottoms out, it starts a put while player 0 holds something, and a take
 * otherwise. as either ends, the step undoes what the card menu leaves. it
 * calls card_init through card_drives_end, clears garden_camera_fixed and
 * alife_paused, and gives player 0 its pad back */
static void vms_step(struct task *task)
{
    struct task_work *work = task->work;
    struct vms_motion *motion = VMS_MOTION(task);
    struct collision_work *collision = work->collision;
    NJS_OBJECT *button = &vms_button_object;
    Float *button_y = &button->pos[1];

    /* @hack spelling: the monitor's y written through the object's address,
     * which the image adds 12 to (0x0c91b81a, 0x0c91b81e) and keeps on the
     * stack across njCos; as vms_monitor_object.pos[1], 83 of 256 words and 1
     * of the function's 111 words of data differ */
    (&vms_monitor_object)->pos[1] =
        ((njCos(VARS(work)->monitor - 0x4000) + 1.0F) * MONITOR_SWING) +
        MONITOR_LOW;
    switch (work->submode) {
    case VMS_IDLE:
        garden_camera_fixed = FALSE;
        if (collision->shapes[4].hit) {
            if (collision->shapes[4].hit->kind == COLLISION_PLAYER) {
                *button_y -= 0.13F;
                if (*button_y < 3.5F) {
                    *button_y = 3.5F;
                    if (!(work->flags & VMS_DOWN)) {
                        sound_play(SE_BUTTON, 0, 0, 0);
                        work->flags |= VMS_PRESSED;
                        work->flags |= VMS_DOWN;
                    }
                }
                VARS(work)->release_wait = 0;
            }
        } else {
            *button_y += 0.08F;
            if (*button_y > 4.55F) {
                *button_y = 4.55F;
            }
            if (VARS(work)->release_wait++ > 30) {
                work->flags &= ~VMS_DOWN;
            }
        }
        if (work->flags & VMS_PRESSED) {
            work->flags &= ~VMS_PRESSED;
            if (player_work[0]->flags & PLAYER_HOLDING) {
                work->submode = VMS_PUTTING;
            } else {
                work->submode = VMS_TAKING;
            }
            work->mode = 0;
            work->timer = 0;
        }
        break;
    case VMS_TAKING:
        if (!vms_take(task, motion)) {
            work->submode = VMS_IDLE;
            work->mode = 0;
            card_drives_end();
            garden_camera_fixed = FALSE;
            alife_paused = 0;
            pad_enable_player(0);
        }
        break;
    case VMS_PUTTING:
        if (!vms_put(task, motion)) {
            work->submode = VMS_IDLE;
            work->mode = 0;
            card_drives_end();
            garden_camera_fixed = FALSE;
            alife_paused = 0;
            pad_enable_player(0);
        }
        break;
    }
    njSetTexture(&alife_objects_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x + (njSin(motion->shake_x) * 0.1F),
                work->pos.y +
                    (njSin(motion->shake_x + motion->shake_z) * 0.05F),
                work->pos.z + (njSin(motion->shake_z) * 0.1F));
    TURN(njRotateY, work->ang.y);
    njScale(NULL, 1.5F, 1.5F, 1.5F);
    draw_action(&vms_open_action, motion->frame);
    njPopMatrix(1);
    work->timer++;
    collision_register(work);
    /* @hack redundant: a never-run test of task, which keeps task in R11
     * and button_y in R10 (0x0c91b818, 0x0c91b826); without it, the two
     * trade and 14 of 256 words differ */
    if (1) {
    } else if (task) {
    }
    task_step_children(task);
}
